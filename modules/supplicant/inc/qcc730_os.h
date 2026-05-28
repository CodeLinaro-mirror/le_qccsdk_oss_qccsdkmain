/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _FERMION_OS_H_
#define _FERMION_OS_H_

#include "utils/list.h"
#include "utils/eloop.h"
#include "wlan_8021x_cfg.h"
#include "os.h"
#include <stdlib.h>
#include <string.h>

/*********************** os.h start ******************************/

typedef unsigned int time_ms_t;

#define TIME_MS_BEFORE(x, y) ((x) < (y))
#define TIME_MS_SUB(x, y) ((x) - (y))

int os_mktime(int year, int month, int day, int hour, int min, int sec,
              os_time_t *t);
int os_get_time(struct os_time *t);
time_ms_t os_get_sys_time_ms(void);
int os_strcasecmp(const char *s1, const char *s2);
int os_strncasecmp(const char *s1, const char *s2, size_t n);
char *os_strchr(const char *s, int c);
char *os_strdup(const char *s);
int os_strcmp(const char *s1, const char *s2);
int os_strncmp(const char *s1, const char *s2, size_t n);
char *os_strrchr(const char *s, int c);
char *os_strstr(const char *haystack, const char *needle);

#define os_snprintf snprintf
#define os_malloc(s) malloc(s)
#define os_free(p) free((p))
#define os_memcpy(d, s, n) memcpy((d), (s), (n))
#define os_memmove(d, s, n) memmove((d), (s), (n))
#define os_memset(s, c, n) memset(s, c, n)
#define os_memcmp(s1, s2, n) memcmp((s1), (s2), (n))
#define os_strlen(s) strlen(s)
#define os_zalloc(s) calloc(1, s)
#define os_memzero(s, n) memset(s, 0, n)
#ifndef os_memdup
#define os_memdup(h, n) memdup((h), (n))

static inline void *os_realloc(void *ptr, size_t new_size) {
  if (new_size == 0) {
    free(ptr);
    return NULL;
  }

  if (ptr == NULL) {
    return malloc(new_size);
  }

  void *new_ptr = malloc(new_size);
  if (new_ptr == NULL) {
    return NULL;
  }

  memcpy(new_ptr, ptr, new_size);

  free(ptr);

  return new_ptr;
}

static inline void *memdup(const void *src, size_t len) {
  void *r = os_malloc(len);

  if (r && src)
    os_memcpy(r, src, len);
  return r;
}
#endif

static inline int os_snprintf_error(size_t size, int res) {
  return res < 0 || (unsigned int)res >= size;
}

static inline void *os_realloc_array(void *ptr, size_t nmemb, size_t size) {
  if (size && nmemb > (~(size_t)0) / size)
    return NULL;
  return os_realloc(ptr, nmemb * size);
}

int os_memcmp_const(const void *a, const void *b, size_t len);
size_t os_strlcpy(char *dest, const char *src, size_t siz);
int os_get_random(unsigned char *buf, size_t len);
char *os_readfile(const char *name, size_t *len);

#define TEST_FAIL() 0
/*********************** os.h end ******************************/

/*********************** eloop.h start ******************************/

typedef void (*eloop_timeout_handler)(void *eloop_data, void *user_ctx);

struct eloop_timeout {
  struct dl_list list;
  struct os_time time;
  time_ms_t ms;
  time_ms_t ms_real;
  void *eloop_data;
  void *user_data;
  eloop_timeout_handler handler;
};

typedef void (*eloop_sock_handler)(int sock, void *eloop_ctx, void *sock_ctx);

struct eloop_sock {
  int sock;
  void *eloop_data;
  void *user_data;
  eloop_sock_handler handler;
};

#define ELOOP_READ_SOCK_MAX_COUNT 4

struct eloop_sock_table {
  int real_count;
  int max_count;
  struct eloop_sock *table;
  eloop_event_type type;
};

struct eloop_data {
  struct dl_list timeout;
  struct eloop_sock_table readers;
  int terminate;
};

extern struct eloop_data eloop;

/**
 * eloop_init() - Initialize global event loop data
 * Returns: 0 on success, -1 on failure
 *
 * This function must be called before any other eloop_* function.
 */
int eloop_init(void);

void eloop_run(void);
void eloop_terminate(void);
int eloop_terminated(void);

/**
 * eloop_destroy - Free any resources allocated for the event loop
 *
 * After calling eloop_destroy(), other eloop_* functions must not be called
 * before re-running eloop_init().
 */
void eloop_destroy(void);

/**
 * eloop_register_timeout - Register timeout
 * @secs: Number of seconds to the timeout
 * @usecs: Number of microseconds to the timeout
 * @handler: Callback function to be called when timeout occurs
 * @eloop_data: Callback context data (eloop_ctx)
 * @user_data: Callback context data (sock_ctx)
 * Returns: 0 on success, -1 on failure
 *
 * Register a timeout that will cause the handler function to be called after
 * given time.
 */
int eloop_register_timeout(unsigned int secs, unsigned int usecs,
                           eloop_timeout_handler handler, void *eloop_data,
                           void *user_data);

/**
 * eloop_cancel_timeout - Cancel timeouts
 * @handler: Matching callback function
 * @eloop_data: Matching eloop_data or %ELOOP_ALL_CTX to match all
 * @user_data: Matching user_data or %ELOOP_ALL_CTX to match all
 * Returns: Number of cancelled timeouts
 *
 * Cancel matching <handler,eloop_data,user_data> timeouts registered with
 * eloop_register_timeout(). ELOOP_ALL_CTX can be used as a wildcard for
 * cancelling all timeouts regardless of eloop_data/user_data.
 */
int eloop_cancel_timeout(eloop_timeout_handler handler, void *eloop_data,
                         void *user_data);

/**
 * eloop_remove_timeout - remove timeout
 * @timeout: the timeout entity
 *
 * Remove timeout link from the global timeout root.
 */
void eloop_remove_timeout(struct eloop_timeout *timeout);

int eloop_register_read_sock(int sock, eloop_sock_handler handler,
                             void *eloop_data, void *user_data);
void eloop_unregister_read_sock(int sock);

/*********************** eloop.h end ******************************/

int my_sscanf(const char *s, const char *format, ...);

#endif /* _FERMION_OS_H_ */
