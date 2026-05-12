/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "common.h"
#include "timer.h"
#include <time.h>
#include "rng_api.h"
#include "qapi_wlan_misc.h"
#include "qcc730_os.h"
#include "rng_api.h"
#include "fs.h"
#include "littlefs_fs.h"
#include "fcntl.h"
#include "wpa_buf.h"
#include "wpa_common.h"
#include "wpa_debug.h"
#include "supplicant_cfg.h"
#include "printfext.h"
#include "sockets.h"

struct eloop_data eloop;
cb_printf_t wlan_dbg_Printf;
void *wlan_dbg_group;

#define MAX_EVENT_WAIT_TIME_MSEC (0xFFFFFFFF)
#define SOCKET_SELECT_WAIT_FOREVER (0xFFFFFFFF)

time_ms_t os_get_sys_time_ms(void) { return hres_timer_curr_time_ms(); }

char *os_readfile(const char *name, size_t *len) {
  size_t read_len = 0;
  size_t file_len = 0;
  int file = open(name, O_RDONLY, 0);

  if (file == -1) {
    warn_printf("Error opening file:%s.\n", name);
    return NULL;
  }
  info_printf("open %s for read\r\n", name);

  file_len = lseek(file, 0, SEEK_END);

  char *buf = (char *)malloc(file_len);
  if (buf == NULL) {
    warn_printf("ERROR: no enough memory, len = %d\r\n", file_len);
    close(file);
    return NULL;
  }
  lseek(file, 0, SEEK_SET);

  int len_read = 0;
  len_read = read(file, buf, file_len);
  if (len_read <= 0) {
    warn_printf("Fail to read from: %s, %d\r\n", name, len_read);
    close(file);
    return NULL;
  }
  *len = file_len;

  close(file);
  return buf;
}

int wlan_dbg_init(void *Group_Handle, void *cb_printf_f) {
  if (Group_Handle && cb_printf_f) {
    wlan_dbg_group = Group_Handle;
    wlan_dbg_Printf = (cb_printf_t)cb_printf_f;
    return QAPI_OK;
  } else {
    return QAPI_ERROR;
  }
}
#if 0
void wlan_rx_mgmt_enqueue(mgt_frame_chain_t *rxq, mgt_frame_buf_t *data)
{
    //firstly enqueue
    if(rxq->head == NULL && rxq->tail == NULL)
    {
        rxq->tail = data;
        rxq->head = data;
        data->frame_next = NULL;
    }
    else
    {
        rxq->tail->frame_next = data;
        data->frame_next = NULL;
        rxq->tail = data;
    }
    rxq->num_of_bufs++;
}

//get from head
mgt_frame_buf_t* wlan_rx_mgmt_dequeue(mgt_frame_chain_t *rxq)
{
    mgt_frame_buf_t *data = NULL;
    
    if(rxq == NULL || rxq->head == NULL)
    {
        return NULL;
    }
    data = rxq->head;
    rxq->head = data->frame_next;

    if(rxq->head == NULL)
    {
        rxq->tail = NULL;
    }
    rxq->num_of_bufs--;

    return data;
}
#endif

int eloop_register_timeout(unsigned int secs, unsigned int usecs,
                           eloop_timeout_handler handler, void *eloop_data,
                           void *user_data) {
  struct eloop_timeout *timeout, *tmp;

  log_printf("start timeout=%ds-%dus\n", secs, usecs);
  timeout = os_zalloc(sizeof(*timeout));
  if (timeout == NULL) {
    warn_printf("%s %d\n", __FUNCTION__, __LINE__);
    return -1;
  }
  timeout->time.sec = secs;
  timeout->time.usec = usecs;
  timeout->ms = secs * 1000 + usecs / 1000;
  timeout->eloop_data = eloop_data;
  timeout->user_data = user_data;
  timeout->handler = handler;
  timeout->ms_real = os_get_sys_time_ms() + timeout->ms;

  /* Maintain timeouts in order of increasing time */
  dl_list_for_each(tmp, &eloop.timeout, struct eloop_timeout, list) {
    if (TIME_MS_BEFORE(timeout->ms_real, tmp->ms_real)) {
      dl_list_add(tmp->list.prev, &timeout->list);
      return 0;
    }
  }
  dl_list_add_tail(&eloop.timeout, &timeout->list);
  return 0;
}

void eloop_remove_timeout(struct eloop_timeout *timeout) {
  // WLPRINT_INFO("%s timeout=%dms\n", __FUNCTION__, timeout->msec);
  dl_list_del(&timeout->list);
  os_free(timeout);
}

int eloop_cancel_timeout(eloop_timeout_handler handler, void *eloop_data,
                         void *user_data) {
  struct eloop_timeout *timeout, *prev;
  int removed = 0;

  dl_list_for_each_safe(timeout, prev, &eloop.timeout, struct eloop_timeout,
                        list) {
    if (timeout->handler == handler && (timeout->eloop_data == eloop_data) &&
        (timeout->user_data == user_data)) {
      info_printf("%s timeout=%dms\n", __FUNCTION__, timeout->ms);
      eloop_remove_timeout(timeout);
      removed++;
    }
  }

  return removed;
}

static struct eloop_sock_table *eloop_get_sock_table(eloop_event_type type) {
  switch (type) {
  case EVENT_TYPE_READ:
    return &eloop.readers;
  }

  return NULL;
}

static int eloop_sock_table_add_sock(struct eloop_sock_table *table, int sock,
                                     eloop_sock_handler handler,
                                     void *eloop_data, void *user_data) {
  struct eloop_sock *tmp;
  int i;

  if ((table == NULL) || (table->real_count >= table->max_count)) {
    return -1;
  }
  for (i = 0; i < table->max_count; i++) {
    if (table->table[i].sock == -1) {
      break;
    }
  }
  if (i == table->max_count) {
    return -1;
  }
  tmp = &table->table[i];
  table->real_count++;
  tmp->sock = sock;
  tmp->eloop_data = eloop_data;
  tmp->user_data = user_data;
  tmp->handler = handler;
  return 0;
}

static void eloop_sock_table_remove_sock(struct eloop_sock_table *table,
                                         int sock) {
  int i;

  if (table == NULL || table->table == NULL || table->real_count == 0)
    return;

  for (i = 0; i < table->max_count; i++) {
    if (table->table[i].sock == sock)
      break;
  }
  if (i == table->max_count)
    return;
  table->real_count--;
  os_memzero(&table->table[i], sizeof(struct eloop_sock));
}

int eloop_register_sock(int sock, eloop_event_type type,
                        eloop_sock_handler handler, void *eloop_data,
                        void *user_data) {
  struct eloop_sock_table *table;

  table = eloop_get_sock_table(type);
  return eloop_sock_table_add_sock(table, sock, handler, eloop_data, user_data);
}

void eloop_unregister_sock(int sock, eloop_event_type type) {
  struct eloop_sock_table *table;

  table = eloop_get_sock_table(type);
  eloop_sock_table_remove_sock(table, sock);
}

int eloop_register_read_sock(int sock, eloop_sock_handler handler,
                             void *eloop_data, void *user_data) {
  return eloop_register_sock(sock, EVENT_TYPE_READ, handler, eloop_data,
                             user_data);
}

void eloop_unregister_read_sock(int sock) {
  eloop_unregister_sock(sock, EVENT_TYPE_READ);
}

static int eloop_sock_table_set_fds(struct eloop_sock_table *table,
                                    fd_set *fds) {
  int i;
  int real_count = 0;
  int maxfd = -1;

  FD_ZERO(fds);

  if (table->table == NULL)
    return 0;

  for (i = 0; i < table->max_count; i++) {
    int sock = table->table[i].sock;
    if (sock >= 0 && sock < FD_SETSIZE) {
      FD_SET(sock, fds);
      real_count++;

      if (sock > maxfd)
        maxfd = sock;
    }
  }
  if (real_count != table->real_count) {
    info_printf("eloop_sock_table_set_fds, real count: %d, table real "
                "count:%d, max count: %d\n",
                real_count, table->real_count, table->max_count);
  }

  return maxfd + 1;
}

static void eloop_sock_table_dispatch(struct eloop_sock_table *table,
                                      fd_set *fds) {
  int i;

  if (table == NULL || table->table == NULL)
    return;

  for (i = 0; i < table->max_count; i++) {
    if ((table->table[i].sock >= 0) && FD_ISSET(table->table[i].sock, fds)) {
      table->table[i].handler(table->table[i].sock, table->table[i].eloop_data,
                              table->table[i].user_data);
    }
  }
}

static void eloop_sock_table_destroy(struct eloop_sock_table *table) {
  if (table) {
    int i;
    for (i = 0; i < table->real_count && table->table; i++) {
      wpa_printf(MSG_INFO,
                 "ELOOP: remaining socket: "
                 "sock=%d eloop_data=%p user_data=%p "
                 "handler=%p",
                 table->table[i].sock, table->table[i].eloop_data,
                 table->table[i].user_data, table->table[i].handler);
    }
    os_free(table->table);
    table->table = NULL;
    table->real_count = 0;
    table->max_count = 0;
  }
}

int eloop_init(void) {
  log_printf("%s\n", __FUNCTION__);
  os_memset(&eloop, 0, sizeof(eloop));
  dl_list_init(&eloop.timeout);
  eloop.terminate = 0;
  eloop.readers.type = EVENT_TYPE_READ;
  eloop.readers.max_count = ELOOP_READ_SOCK_MAX_COUNT;
  eloop.readers.real_count = 0;
  eloop.readers.table =
      os_malloc(sizeof(struct eloop_sock) * eloop.readers.max_count);
  if (!eloop.readers.table) {
    return -1;
  }
  os_memzero(eloop.readers.table,
             sizeof(struct eloop_sock) * eloop.readers.max_count);

  for (int i = 0; i < eloop.readers.max_count; i++) {
    eloop.readers.table[i].sock = -1;
  }
  return 0;
}

void eloop_run(void) {
  fd_set rfds;
  time_ms_t timeout = 0;
  int res = 0;
  struct eloop_timeout *timer = NULL;
  time_ms_t now_ms = 0;

  log_printf("%s +++\n", __FUNCTION__);

  while (1) {
    do {
      if (eloop.terminate) {
        info_printf("%s terminate\n", __FUNCTION__);
        goto end;
      }
      timeout = SOCKET_SELECT_WAIT_FOREVER;
      timer = dl_list_first(&eloop.timeout, struct eloop_timeout, list);
      if (timer) {
        now_ms = os_get_sys_time_ms();
        if (TIME_MS_BEFORE(now_ms, timer->ms_real)) {
          timeout = TIME_MS_SUB(timer->ms_real, now_ms);
        } else {
          timeout = 0;
        }
      }
      // if (timeout > CFG_ELOOP_MAX_TIMEOUT_MS) {
      //   timeout = CFG_ELOOP_MAX_TIMEOUT_MS;
      // }

      int maxfdp1 = eloop_sock_table_set_fds(&eloop.readers, &rfds);

      struct timeval tv;
      struct timeval *ptv = NULL;

      if (timeout != SOCKET_SELECT_WAIT_FOREVER) {
        tv.tv_sec = timeout / 1000;
        tv.tv_usec = (timeout % 1000) * 1000;
        ptv = &tv;
      }

      // info_printf("SELECT: %d\n", maxfdp1);
      res = lwip_select(maxfdp1, &rfds, NULL, NULL, ptv);
      if (res < 0) {
        warn_printf("%s %d select error\n", __FUNCTION__, __LINE__);
        continue;
      }

      timer = dl_list_first(&eloop.timeout, struct eloop_timeout, list);
      if (timer) {
        now_ms = os_get_sys_time_ms();
        if (!TIME_MS_BEFORE(now_ms, timer->ms_real)) {
          void *eloop_data = timer->eloop_data;
          void *user_data = timer->user_data;
          eloop_timeout_handler handler = timer->handler;
          eloop_remove_timeout(timer);
          handler(eloop_data, user_data);
        }
      }
    } while (res == 0);

    log_printf("eloop readers received\n");
    eloop_sock_table_dispatch(&eloop.readers, &rfds);
  };

end:
  return;
}

void eloop_terminate(void) { eloop.terminate = 1; }

int eloop_terminated(void) { return (eloop.terminate == 1); }

void eloop_destroy(void) {
  struct eloop_timeout *timeout, *prev;
  info_printf("%s+++\n", __FUNCTION__);
  dl_list_for_each_safe(timeout, prev, &eloop.timeout, struct eloop_timeout,
                        list) {
    eloop_remove_timeout(timeout);
  }
  eloop_sock_table_destroy(&eloop.readers);
  info_printf("%s---\n", __FUNCTION__);
}

#define def_scanf
#ifdef def_scanf

/* This routine doesn't support floating point conversion */
#define NO_FLOATING_POINT

#define EOF (-1)
#define BUF 513 /* Maximum length of numeric string. */

/*
 * Flags used during conversion.
 */
#define LONG 0x0001       /* l: long or double */
#define LONGDBL 0x0002    /* L: long double */
#define SHORT 0x0004      /* h: short */
#define SUPPRESS 0x0008   /* *: suppress assignment */
#define POINTER 0x0010    /* p: void * (as hex) */
#define NOSKIP 0x0020     /* [ or c: do not skip blanks */
#define LONGLONG 0x0400   /* ll: long long (+ deprecated q: quad) */
#define INTMAXT 0x0800    /* j: intmax_t */
#define PTRDIFFT 0x1000   /* t: ptrdiff_t */
#define SIZET 0x2000      /* z: size_t */
#define SHORTSHORT 0x4000 /* hh: char */
#define UNSIGNED 0x8000   /* %[oupxX] conversions */

/*
 * The following are used in integral conversions only:
 * SIGNOK, NDIGITS, PFXOK, and NZDIGITS
 */
#define SIGNOK 0x00040   /* +/- is (still) legal */
#define NDIGITS 0x00080  /* no digits detected */
#define PFXOK 0x00100    /* 0x prefix is (still) legal */
#define NZDIGITS 0x00200 /* no zero digits detected */
#define HAVESIGN 0x10000 /* sign detected */

/*
 * Conversion types.
 */
#define CT_CHAR 0   /* %c conversion */
#define CT_CCL 1    /* %[...] conversion */
#define CT_STRING 2 /* %s conversion */
#define CT_INT 3    /* %[dioupxX] conversion */
#define CT_FLOAT 4  /* %[efgEFG] conversion */

typedef int64_t intmax_t;
typedef uint64_t uintmax_t;
typedef uint32_t uint_ptr_t;
typedef unsigned char u_char;
typedef unsigned int size_t;

extern long strtol(const char *str, char **endptr, int base);
extern unsigned long strtoul(const char *str, char **endptr, int base);

#undef isspace
#define isspace(c)                                                             \
  (c == ' ' || c == '\f' || c == '\n' || c == '\r' || c == '\t' || c == '\v')

#define __collate_load_error /*CONSTCOND*/ 0

static int __collate_range_cmp(int c1, int c2) {
  static char s1[2], s2[2];

  s1[0] = c1;
  s2[0] = c2;

  if (s1 == NULL || s2 == NULL)
    return -1;

  return os_strcmp(s1, s2);
}

typedef struct {
  char *buffer_data;
  int length;
  unsigned char *up;
} input_data;

int __srefill(input_data *fp) {

/* we don't need this now. But keeping the code for future use */
#if 0
	/* make sure stdio is set up */
	if (!__sdidinit)
		__sinit();

	fp->_r = 0;		/* largely a convenience for callers */

	/* SysV does not make this test; take it out for compatibility */
	if (fp->_flags & __SEOF)
		return (EOF);

    else {
		/*
		 * We were reading.  If there is an my_ungetc buffer,
		 * we must have been reading from that.  Drop it,
		 * restoring the previous buffer (if any).  If there
		 * is anything in that buffer, return.
		 */
		if (HASUB(fp)) {
			FREEUB(fp);
			if ((fp->_r = fp->_ur) != 0) {
				fp->_p = fp->_up;
				return (0);
			}
		}
	}

	if (fp->_bf._base == NULL)
		__smakebuf(fp);

	/*
	 * Before reading from a line buffered or unbuffered file,
	 * flush all line buffered output files, per the ANSI C
	 * standard.
	 */
	if (fp->_flags & (__SLBF|__SNBF)) {
		rwlock_rdlock(&__sfp_lock);
		(void) _fwalk(lflush);
		rwlock_unlock(&__sfp_lock);
	}
	fp->_p = fp->_bf._base;
	fp->_r = (*fp->_read)(fp->_cookie, (char *)fp->_p, fp->_bf._size);
	fp->_flags &= ~__SMOD;	/* buffer contents are again pristine */
	if (fp->_r <= 0) {
		if (fp->_r == 0)
			fp->_flags |= __SEOF;
		else {
			fp->_r = 0;
			fp->_flags |= __SERR;
		}
		return (EOF);
	}
#endif

  return (0);
}

int my_ungetc(int c, input_data *fp) {

  if (c == EOF)
    return (EOF);

  c = (unsigned char)c;

  /*
   * Ruby - We just to have one case for our simple implementation.
   * If we can handle this by simply backing up, do so,
   * but never replace the original character.
   * (This makes sscanf() work when scanning `const' data.)
   */
  fp->buffer_data--;
  fp->length++;
  return (c);
}

/*
 * Fill in the given table from the scanset at the given format
 * (just after `[').  Return a pointer to the character past the
 * closing `]'.  The table has a 1 wherever characters should be
 * considered part of the scanset.
 */
static const u_char *__sccl(char *tab, const u_char *fmt) {
  int c, n, v, i;

  //_DIAGASSERT(tab != NULL);
  //_DIAGASSERT(fmt != NULL);

  /* first `clear' the whole table */
  c = *fmt++; /* first char hat => negated scanset */
  if (c == '^') {
    v = 1;      /* default => accept */
    c = *fmt++; /* get new first char */
  } else
    v = 0; /* default => reject */

  /* XXX: Will not work if sizeof(tab*) > sizeof(char) */
  (void)memset(tab, v, 256);

  if (c == 0)
    return (fmt - 1); /* format ended before closing ] */

  /*
   * Now set the entries corresponding to the actual scanset
   * to the opposite of the above.
   *
   * The first character may be ']' (or '-') without being special;
   * the last character may be '-'.
   */
  v = 1 - v;
  for (;;) {
    tab[c] = v; /* take character c */
  doswitch:
    n = *fmt++; /* and examine the next */
    switch (n) {

    case 0: /* format ended too soon */
      return (fmt - 1);

    case '-':
      /*
       * A scanset of the form
       *	[01+-]
       * is defined as `the digit 0, the digit 1,
       * the character +, the character -', but
       * the effect of a scanset such as
       *	[a-zA-Z0-9]
       * is implementation defined.  The V7 Unix
       * scanf treats `a-z' as `the letters a through
       * z', but treats `a-a' as `the letter a, the
       * character -, and the letter a'.
       *
       * For compatibility, the `-' is not considerd
       * to define a range if the character following
       * it is either a close bracket (required by ANSI)
       * or is not numerically greater than the character
       * we just stored in the table (c).
       */
      n = *fmt;
      if (n == ']' ||
          (__collate_load_error ? n < c : __collate_range_cmp(n, c) < 0)) {
        c = '-';
        break; /* resume the for(;;) */
      }
      fmt++;
      /* fill in the range */
      if (__collate_load_error) {
        do
          tab[++c] = v;
        while (c < n);
      } else {
        for (i = 0; i < 256; i++)
          if (__collate_range_cmp(c, i) < 0 && __collate_range_cmp(i, n) <= 0)
            tab[i] = v;
      }
#if 1 /* XXX another disgusting compatibility hack */
      c = n;
      /*
       * Alas, the V7 Unix scanf also treats formats
       * such as [a-c-e] as `the letters a through e'.
       * This too is permitted by the standard....
       */
      goto doswitch;
#else
      c = *fmt++;
      if (c == 0)
        return (fmt - 1);
      if (c == ']')
        return (fmt);
#endif

    case ']': /* end of scanset */
      return (fmt);

    default: /* just another character */
      c = n;
      break;
    }
  }
  /* NOTREACHED */
}

#define SCANF_SKIP_SPACE()                                                     \
  do {                                                                         \
    while ((s->length > 0 || __srefill(s) == 0) && isspace(*s->buffer_data))   \
      nread++, s->length--, s->buffer_data++;                                  \
  } while (/*CONSTCOND*/ 0)

#define ns_isdigit(c) (((c) >= '0' && (c) <= '9') ? 1 : 0)

int ns_atoi(char *str) {
  int res = 0, i;

  /* bypass white spaces */
  while (isspace(*str)) {
    ++str;
  }

  for (i = 0; str[i] != '\0' && ns_isdigit(str[i]); ++i) {
    res = res * 10 + (str[i] - '0');
  }

  return res;
}

// only suppport base=10
#define strtol(s, e, b) ns_atoi((s))

/*
 *  vfscanf
 */
int my_vfscanf(input_data *s, const char *fmt0, va_list ap) {
  const u_char *fmt = (const u_char *)fmt0;
  int c;            /* character from format, or conversion */
  size_t width;     /* field width, or 0 */
  char *p;          /* points into all kinds of strings */
  size_t n;         /* handy size_t */
  int flags;        /* flags as defined above */
  char *p0;         /* saves original value of p when necessary */
  int nassigned;    /* number of fields assigned */
  int nconversions; /* number of conversions */
  int nread;        /* number of characters consumed from fp */
  int base;         /* base argument to conversion function */
  char ccltab[256]; /* character class table for %[...] */
  char buf[BUF];    /* buffer for numeric and mb conversions */
  // wchar_t *wcp;		/* handy wide-character pointer */
  // size_t nconv;		/* length of multibyte sequence converted */

  /* `basefix' is used to avoid `if' tests in the integer scanner */
  static const short basefix[17] = {10, 1,  2,  3,  4,  5,  6,  7, 8,
                                    9,  10, 11, 12, 13, 14, 15, 16};

  nassigned = 0;
  nconversions = 0;
  nread = 0;
  base = 0;
  for (;;) {
    c = (unsigned char)*fmt++;
    if (c == 0)
      return (nassigned);
    if (isspace(c)) {
      while ((s->length > 0 || __srefill(s) == 0) && isspace(*s->buffer_data))
        nread++, s->length--, s->buffer_data++;
      continue;
    }
    if (c != '%')
      goto literal;
    width = 0;
    flags = 0;
    /*
     * switch on the format.  continue if done;
     * break once format type is derived.
     */
  again:
    c = *fmt++;
    switch (c) {
    case '%':
      SCANF_SKIP_SPACE();
    literal:
      if (s->length <= 0 && __srefill(s))
        goto input_failure;
      if (*s->buffer_data != c)
        goto match_failure;
      s->length--, s->buffer_data++;
      nread++;
      continue;

    case '*':
      flags |= SUPPRESS;
      goto again;
    case 'j':
      flags |= INTMAXT;
      goto again;
    case 'l':
      if (flags & LONG) {
        flags &= ~LONG;
        flags |= LONGLONG;
      } else
        flags |= LONG;
      goto again;
    case 'q':
      flags |= LONGLONG; /* not quite */
      goto again;
    case 't':
      flags |= PTRDIFFT;
      goto again;
    case 'z':
      flags |= SIZET;
      goto again;
    case 'L':
      flags |= LONGDBL;
      goto again;
    case 'h':
      if (flags & SHORT) {
        flags &= ~SHORT;
        flags |= SHORTSHORT;
      } else
        flags |= SHORT;
      goto again;

    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      width = width * 10 + c - '0';
      goto again;

    /*
     * Conversions.
     */
    case 'd':
      c = CT_INT;
      base = 10;
      break;

    case 'i':
      c = CT_INT;
      base = 0;
      break;

    case 'o':
      c = CT_INT;
      flags |= UNSIGNED;
      base = 8;
      break;

    case 'u':
      c = CT_INT;
      flags |= UNSIGNED;
      base = 10;
      break;

    case 'X':
    case 'x':
      flags |= PFXOK; /* enable 0x prefixing */
      c = CT_INT;
      flags |= UNSIGNED;
      base = 16;
      break;

#ifndef NO_FLOATING_POINT
    case 'A':
    case 'E':
    case 'F':
    case 'G':
    case 'a':
    case 'e':
    case 'f':
    case 'g':
      c = CT_FLOAT;
      break;
#endif

    case 'S':
      flags |= LONG;
      /* FALLTHROUGH */
    case 's':
      c = CT_STRING;
      break;

    case '[':
      fmt = __sccl(ccltab, fmt);
      flags |= NOSKIP;
      c = CT_CCL;
      break;

    case 'C':
      flags |= LONG;
      /* FALLTHROUGH */
    case 'c':
      flags |= NOSKIP;
      c = CT_CHAR;
      break;

    case 'p': /* pointer format is like hex */
      flags |= POINTER | PFXOK;
      c = CT_INT;        /* assumes sizeof(uintmax_t) */
      flags |= UNSIGNED; /*      >= sizeof(uintptr_t) */
      base = 16;
      break;

    case 'n':
      nconversions++;
      if (flags & SUPPRESS) /* ??? */
        continue;
      if (flags & SHORTSHORT)
        *va_arg(ap, char *) = nread;
      else if (flags & SHORT)
        *va_arg(ap, short *) = nread;
      else if (flags & LONG)
        *va_arg(ap, long *) = nread;
      else if (flags & LONGLONG)
        *va_arg(ap, long long *) = nread;
      else if (flags & INTMAXT)
        *va_arg(ap, intmax_t *) = nread;
      else if (flags & SIZET)
        *va_arg(ap, size_t *) = nread;
      else if (flags & PTRDIFFT)
        *va_arg(ap, ptrdiff_t *) = nread;
      else
        *va_arg(ap, int *) = nread;
      continue;

    default:
      goto match_failure;

    /*
     * Disgusting backwards compatibility hack.	XXX
     */
    case '\0': /* compat */
      return (EOF);
    }

    /*
     * We have a conversion that requires input.
     */
    if (s->length <= 0 && __srefill(s))
      goto input_failure;

    /*
     * Consume leading white space, except for formats
     * that suppress this.
     */
    if ((flags & NOSKIP) == 0) {
      while (isspace(*s->buffer_data)) {
        nread++;
        if (--s->length > 0)
          s->buffer_data++;
        else if (__srefill(s))
          goto input_failure;
      }
      /*
       * Note that there is at least one character in
       * the buffer, so conversions that do not set NOSKIP
       * ca no longer result in an input failure.
       */
    }

    /*
     * Do the conversion.
     */
    switch (c) {

    case CT_CHAR:
      /* scan arbitrary characters (sets NOSKIP) */
      if (width == 0)
        width = 1;

      if (flags & SUPPRESS) {
        size_t sum = 0;
        for (;;) {
          if ((n = s->length) < width) {
            sum += n;
            width -= n;
            s->buffer_data += n;
            if (__srefill(s)) {
              if (sum == 0)
                goto input_failure;
              break;
            }
          } else {
            sum += width;
            s->length -= width;
            s->buffer_data += width;
            break;
          }
        }
        nread += sum;
      } else {

        os_memcpy(va_arg(ap, char *), s->buffer_data,
                  width); /* note to change this */

        if (width == 0)
          goto input_failure;
        nread += width;
        nassigned++;
      }
      nconversions++;
      break;

    case CT_CCL:
      /* scan a (nonempty) character class (sets NOSKIP) */
      if (width == 0)
        width = (size_t)~0; /* `infinity' */

      if (flags & SUPPRESS) {
        n = 0;
        while (ccltab[*s->buffer_data]) {
          n++, s->length--, s->buffer_data++;
          if (--width == 0)
            break;
          if (s->length <= 0 && __srefill(s)) {
            if (n == 0)
              goto input_failure;
            break;
          }
        }
        if (n == 0)
          goto match_failure;
      } else {
        p0 = p = va_arg(ap, char *);
        while (ccltab[*s->buffer_data]) {
          s->length--;
          *p++ = *s->buffer_data++;
          if (--width == 0)
            break;
          if (s->length <= 0 && __srefill(s)) {
            if (p == p0)
              goto input_failure;
            break;
          }
        }
        n = p - p0;
        if (n == 0)
          goto match_failure;
        *p = 0;
        nassigned++;
      }
      nread += n;
      nconversions++;
      break;

    case CT_STRING:
      /* like CCL, but zero-length string OK, & no NOSKIP */
      if (width == 0)
        width = (size_t)~0;

      if (flags & SUPPRESS) {
        n = 0;
        while (!isspace(*s->buffer_data)) {
          n++, s->length--, s->buffer_data++;
          if (--width == 0)
            break;
          if (s->length <= 0 && __srefill(s))
            break;
        }
        nread += n;
      } else {
        p0 = p = va_arg(ap, char *);
        while (!isspace(*s->buffer_data)) {
          s->length--;
          *p++ = *s->buffer_data++;
          if (--width == 0)
            break;
          if (s->length <= 0 && __srefill(s))
            break;
        }
        *p = 0;
        nread += p - p0;
        nassigned++;
      }
      nconversions++;
      continue;

    case CT_INT:
      /* scan an integer as if by the conversion function */
#ifdef hardway
      if (width == 0 || width > sizeof(buf) - 1)
        width = sizeof(buf) - 1;
#else
      /* size_t is unsigned, hence this optimisation */
      if (--width > sizeof(buf) - 2)
        width = sizeof(buf) - 2;
      width++;
#endif
      flags |= SIGNOK | NDIGITS | NZDIGITS;
      for (p = buf; width; width--) {
        c = *s->buffer_data;
        /*
         * Switch on the character; `goto ok'
         * if we accept it as a part of number.
         */
        switch (c) {

        /*
         * The digit 0 is always legal, but is
         * special.  For %i conversions, if no
         * digits (zero or nonzero) have been
         * scanned (only signs), we will have
         * base==0.  In that case, we should set
         * it to 8 and enable 0x prefixing.
         * Also, if we have not scanned zero digits
         * before this, do not turn off prefixing
         * (someone else will turn it off if we
         * have scanned any nonzero digits).
         */
        case '0':
          if (base == 0) {
            base = 8;
            flags |= PFXOK;
          }
          if (flags & NZDIGITS)
            flags &= ~(SIGNOK | NZDIGITS | NDIGITS);
          else
            flags &= ~(SIGNOK | PFXOK | NDIGITS);
          goto ok;

        /* 1 through 7 always legal */
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
          base = basefix[base];
          flags &= ~(SIGNOK | PFXOK | NDIGITS);
          goto ok;

        /* digits 8 and 9 ok iff decimal or hex */
        case '8':
        case '9':
          base = basefix[base];
          if (base <= 8)
            break; /* not legal here */
          flags &= ~(SIGNOK | PFXOK | NDIGITS);
          goto ok;

        /* letters ok iff hex */
        case 'A':
        case 'B':
        case 'C':
        case 'D':
        case 'E':
        case 'F':
        case 'a':
        case 'b':
        case 'c':
        case 'd':
        case 'e':
        case 'f':
          /* no need to fix base here */
          if (base <= 10)
            break; /* not legal here */
          flags &= ~(SIGNOK | PFXOK | NDIGITS);
          goto ok;

        /* sign ok only as first character */
        case '+':
        case '-':
          if (flags & SIGNOK) {
            flags &= ~SIGNOK;
            flags |= HAVESIGN;
            goto ok;
          }
          break;

        /*
         * x ok iff flag still set & 2nd char (or
         * 3rd char if we have a sign).
         */
        case 'x':
        case 'X':
          if (flags & PFXOK && p == buf + 1 + !!(flags & HAVESIGN)) {
            base = 16; /* if %i */
            flags &= ~PFXOK;
            goto ok;
          }
          break;
        }

        /*
         * If we got here, c is not a legal character
         * for a number.  Stop accumulating digits.
         */
        break;
      ok:
        /*
         * c is legal: store it and look at the next.
         */
        *p++ = c;
        if (--s->length > 0)
          s->buffer_data++;
        else if (s->length <= 0 || __srefill(s))
          break; /* EOF */
      }
      /*
       * If we had only a sign, it is no good; push
       * back the sign.  If the number ends in `x',
       * it was [sign] '0' 'x', so push back the x
       * and treat it as [sign] '0'.
       */
      if (flags & NDIGITS) {
        if (p > buf)
          (void)my_ungetc(*(u_char *)--p, s);
        goto match_failure;
      }
      c = ((u_char *)p)[-1];
      if (c == 'x' || c == 'X') {
        --p;
        (void)my_ungetc(c, s);
      }
      if ((flags & SUPPRESS) == 0) {
        uintmax_t res;

        *p = 0;
        if ((flags & UNSIGNED) == 0)
          res = strtol(buf, (char **)NULL, base);
        else
          res = strtoul(buf, (char **)NULL, base);
        if (flags & POINTER)
          *va_arg(ap, void **) = (void *)(uint_ptr_t)res;
        else if (flags & SHORTSHORT)
          *va_arg(ap, char *) = (char)res;
        else if (flags & SHORT)
          *va_arg(ap, short *) = (short)res;
        else if (flags & LONG)
          *va_arg(ap, long *) = (long)res;
        else if (flags & LONGLONG)
          *va_arg(ap, long long *) = res;
        else if (flags & INTMAXT)
          *va_arg(ap, intmax_t *) = res;
        else if (flags & PTRDIFFT)
          *va_arg(ap, ptrdiff_t *) = (ptrdiff_t)res;
        else if (flags & SIZET)
          *va_arg(ap, size_t *) = (size_t)res;
        else
          *va_arg(ap, int *) = (int)res;
        nassigned++;
      }
      nread += p - buf;
      nconversions++;
      break;

#ifndef NO_FLOATING_POINT
    case CT_FLOAT:
      /* scan a floating point number as if by strtod */
      if (width == 0 || width > sizeof(buf) - 1)
        width = sizeof(buf) - 1;
      if ((width = parsefloat(fp, buf, buf + width)) == 0)
        goto match_failure;
      if ((flags & SUPPRESS) == 0) {
        if (flags & LONGDBL) {
          long double res = strtold(buf, &p);
          *va_arg(ap, long double *) = res;
        } else if (flags & LONG) {
          double res = strtod(buf, &p);
          *va_arg(ap, double *) = res;
        } else {
          float res = strtof(buf, &p);
          *va_arg(ap, float *) = res;
        }
        if (__scanfdebug && (size_t)(p - buf) != width)
          abort();
        nassigned++;
      }
      nread += width;
      nconversions++;
      break;
#endif /* !NO_FLOATING_POINT */
    }
  }
input_failure:
  return (nconversions != 0 ? nassigned : EOF);
match_failure:
  return (nassigned);
}

int my_sscanf(const char *s, const char *format, ...) {
  int rc;                 // return code
  va_list ap;             // for variable args
  input_data buffer_data; /* Replace fake file with a char buffer */

  os_memzero(&buffer_data, sizeof(input_data));

  buffer_data.buffer_data = (char *)s;
  buffer_data.length = os_strlen(s);
  // fake_file.current_pos = 0;

  va_start(ap, format); // init specifying last non-var arg

  rc = my_vfscanf(&buffer_data, format, ap);

  va_end(ap); // end var args

  return rc;
}

#endif // !A_SIMOS -- MOST OF THIS FILE
