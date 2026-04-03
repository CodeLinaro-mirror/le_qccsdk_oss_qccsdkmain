/*
 */
/*
 */

/*
 * wpa_supplicant/hostapd / common helper functions, etc.
 * Copyright (c) 2002-2005, Jouni Malinen <jkmaline@cc.hut.fi>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Alternatively, this software may be distributed under the terms of BSD
 * license.
 *
 * See README and COPYING for more details.
 */

#ifndef WPA_COMMON_H
#define WPA_COMMON_H

#include <stdint.h>
#include <stddef.h>

#ifdef __linux__
#include <endian.h>
#include <byteswap.h>
#endif /* __linux__ */

#if defined(__FreeBSD__) || defined(__NetBSD__)
#include <sys/types.h>
#include <sys/endian.h>
#define __BYTE_ORDER    _BYTE_ORDER
#define __LITTLE_ENDIAN _LITTLE_ENDIAN
#define __BIG_ENDIAN    _BIG_ENDIAN
#define bswap_16        bswap16
#define bswap_32        bswap32
#define bswap_64        bswap64
#endif /* defined(__FreeBSD__) || defined(__NetBSD__) */

#ifdef CONFIG_NATIVE_WINDOWS
#include <winsock2.h>

static inline int daemon(int nochdir, int noclose)
{
    printf("Windows - daemon() not supported yet\n");
    return -1;
}

static inline void sleep(int seconds)
{
    Sleep(seconds * 1000);
}

static inline void usleep(unsigned long usec)
{
    Sleep(usec / 1000);
}

#ifndef timersub
#define timersub(a, b, res)                           \
    do {                                              \
        (res)->tv_sec = (a)->tv_sec - (b)->tv_sec;    \
        (res)->tv_usec = (a)->tv_usec - (b)->tv_usec; \
        if ((res)->tv_usec < 0) {                     \
            (res)->tv_sec--;                          \
            (res)->tv_usec += 1000000;                \
        }                                             \
    } while (0)
#endif

struct timezone {
    int tz_minuteswest;
    int tz_dsttime;
};

int gettimeofday(struct timeval *tv, struct timezone *tz);

typedef int gid_t;
typedef int socklen_t;

#ifndef MSG_DONTWAIT
#define MSG_DONTWAIT 0 /* not supported */
#endif

#endif /* CONFIG_NATIVE_WINDOWS */

#if defined(__CYGWIN__) || defined(CONFIG_NATIVE_WINDOWS)

#define le_to_host16(n) (n)
#define host_to_le16(n) (n)
#define be_to_host16(n) wpa_swap_16(n)
#define host_to_be16(n) wpa_swap_16(n)
#define le_to_host32(n) (n)
#define be_to_host32(n) wpa_swap_32(n)
#define host_to_be32(n) wpa_swap_32(n)

#else /* Little endian support */

#define le_to_host16(n) (n)
#define host_to_le16(n) (n)
#define be_to_host16(n) bswap_16(n)
#define host_to_be16(n) bswap_16(n)
#define le_to_host32(n) (n)
#define be_to_host32(n) bswap_32(n)
#define host_to_be32(n) bswap_32(n)

#endif /* Little endian support */

/* Macros for handling unaligned 16-bit variables */
#define WPA_GET_BE16(a) ((uint16_t)(((a)[0] << 8) | (a)[1]))

#define WPA_GET_BE24(a) ((((uint32_t)(a)[0]) << 16) | (((uint32_t)(a)[1]) << 8) | ((uint32_t)(a)[2]))

#define WPA_GET_BE32(a) \
    ((((uint32_t)(a)[0]) << 24) | (((uint32_t)(a)[1]) << 16) | (((uint32_t)(a)[2]) << 8) | ((uint32_t)(a)[3]))

#define WPA_PUT_BE16(a, val)               \
    do {                                   \
        (a)[0] = ((uint16_t)(val)) >> 8;   \
        (a)[1] = ((uint16_t)(val)) & 0xff; \
    } while (0)

#define WPA_PUT_BE24(a, val)                                  \
    do {                                                      \
        (a)[0] = (uint8_t)((((uint32_t)(val)) >> 16) & 0xff); \
        (a)[1] = (uint8_t)((((uint32_t)(val)) >> 8) & 0xff);  \
        (a)[2] = (uint8_t)(((uint32_t)(val)) & 0xff);         \
    } while (0)

#define WPA_PUT_BE32(a, val)                                  \
    do {                                                      \
        (a)[0] = (uint8_t)((((uint32_t)(val)) >> 24) & 0xff); \
        (a)[1] = (uint8_t)((((uint32_t)(val)) >> 16) & 0xff); \
        (a)[2] = (uint8_t)((((uint32_t)(val)) >> 8) & 0xff);  \
        (a)[3] = (uint8_t)(((uint32_t)(val)) & 0xff);         \
    } while (0)

#define WPA_PUT_BE64(a, val) \
    do {                     \
        (a)[0] = val >> 56;  \
        (a)[1] = val >> 48;  \
        (a)[2] = val >> 40;  \
        (a)[3] = val >> 32;  \
        (a)[4] = val >> 24;  \
        (a)[5] = val >> 16;  \
        (a)[6] = val >> 8;   \
        (a)[7] = val & 0xff; \
    } while (0)

#define WPA_GET_LE16(a) ((uint16_t)(((a)[1] << 8) | (a)[0]))
#define WPA_PUT_LE16(a, val)               \
    do {                                   \
        (a)[1] = ((uint16_t)(val)) >> 8;   \
        (a)[0] = ((uint16_t)(val)) & 0xff; \
    } while (0)

#define wpa_swap_16(x) (((x & 0xff) << 8) | (x >> 8))
#define wpa_swap_32(x) (((x & 0xff) << 24) | ((x & 0xff00) << 8) | ((x & 0xff0000) >> 8) | (x >> 24));

#ifndef ETH_ALEN
#define ETH_ALEN 6
#endif

int hostapd_get_rand(uint8_t *buf, size_t len);
void hostapd_hexdump(const char *title, const uint8_t *buf, size_t len);
int hwaddr_aton(const char *txt, uint8_t *addr);
int hexstr2bin(const char *hex, uint8_t *buf, size_t len);
char *rel2abs_path(const char *rel_path);
void inc_byte_array(uint8_t *counter, size_t len);
// void print_char(char c);
// void fprint_char(FILE *f, char c);

/* Debugging function - conditional printf and hex dump. Driver wrappers can
 *  use these for debugging purposes. */

enum { MSG_EXCESSIVE, MSG_MSGDUMP, MSG_DEBUG, MSG_INFO, MSG_WARNING, MSG_ERROR };

#ifdef CONFIG_NO_STDOUT_DEBUG

#define wpa_debug_print_timestamp() \
    do {                            \
    } while (0)
#define wpa_printf(args...) \
    do {                    \
    } while (0)
#define wpa_hexdump(args...) \
    do {                     \
    } while (0)
#define wpa_hexdump_key(args...) \
    do {                         \
    } while (0)
#define wpa_hexdump_ascii(args...) \
    do {                           \
    } while (0)
#define wpa_hexdump_ascii_key(args...) \
    do {                               \
    } while (0)

static inline void wpa_hexdump_buf(int level, const char *title,
				   const struct wpabuf *buf)
{
}

static inline void wpa_hexdump_buf_key(int level, const char *title,
				       const struct wpabuf *buf)
{
}

#else /* CONFIG_NO_STDOUT_DEBUG */

/**
 * wpa_debug_printf_timestamp - Print timestamp for debug output
 *
 * This function prints a timestamp in <seconds from 1970>.<microsoconds>
 * format if debug output has been configured to include timestamps in debug
 * messages.
 */
void wpa_debug_print_timestamp(void);

/**
 * wpa_printf - conditional printf
 * @level: priority level (MSG_*) of the message
 * @fmt: printf format string, followed by optional arguments
 *
 * This function is used to print conditional debugging and error messages. The
 * output may be directed to stdout, stderr, and/or syslog based on
 * configuration.
 *
 * Note: New line '\n' is added to the end of the text when printing to stdout.
 */
void wpa_printf(int level, char *fmt, ...) __attribute__((format(printf, 2, 3)));

/**
 * wpa_hexdump - conditional hex dump
 * @level: priority level (MSG_*) of the message
 * @title: title of for the message
 * @buf: data buffer to be dumped
 * @len: length of the buf
 *
 * This function is used to print conditional debugging and error messages. The
 * output may be directed to stdout, stderr, and/or syslog based on
 * configuration. The contents of buf is printed out has hex dump.
 */
void wpa_hexdump(int level, const char *title, const uint8_t *buf, size_t len);

/**
 * wpa_hexdump_key - conditional hex dump, hide keys
 * @level: priority level (MSG_*) of the message
 * @title: title of for the message
 * @buf: data buffer to be dumped
 * @len: length of the buf
 *
 * This function is used to print conditional debugging and error messages. The
 * output may be directed to stdout, stderr, and/or syslog based on
 * configuration. The contents of buf is printed out has hex dump. This works
 * like wpa_hexdump(), but by default, does not include secret keys (passwords,
 * etc.) in debug output.
 */
void wpa_hexdump_key(int level, const char *title, const uint8_t *buf, size_t len);

/**
 * wpa_hexdump_ascii - conditional hex dump
 * @level: priority level (MSG_*) of the message
 * @title: title of for the message
 * @buf: data buffer to be dumped
 * @len: length of the buf
 *
 * This function is used to print conditional debugging and error messages. The
 * output may be directed to stdout, stderr, and/or syslog based on
 * configuration. The contents of buf is printed out has hex dump with both
 * the hex numbers and ASCII characters (for printable range) are shown. 16
 * bytes per line will be shown.
 */
void wpa_hexdump_ascii(int level, const char *title, const uint8_t *buf, size_t len);

/**
 * wpa_hexdump_ascii_key - conditional hex dump, hide keys
 * @level: priority level (MSG_*) of the message
 * @title: title of for the message
 * @buf: data buffer to be dumped
 * @len: length of the buf
 *
 * This function is used to print conditional debugging and error messages. The
 * output may be directed to stdout, stderr, and/or syslog based on
 * configuration. The contents of buf is printed out has hex dump with both
 * the hex numbers and ASCII characters (for printable range) are shown. 16
 * bytes per line will be shown. This works like wpa_hexdump_ascii(), but by
 * default, does not include secret keys (passwords, etc.) in debug output.
 */
void wpa_hexdump_ascii_key(int level, const char *title, const uint8_t *buf, size_t len);

#endif /* CONFIG_NO_STDOUT_DEBUG */

#ifdef EAPOL_TEST
#define WPA_ASSERT(a)                                 \
    do {                                              \
        if (!(a)) {                                   \
            printf("WPA_ASSERT FAILED '" #a           \
                   "' "                               \
                   "%s %s:%d\n",                      \
                   __FUNCTION__, __FILE__, __LINE__); \
            exit(1);                                  \
        }                                             \
    } while (0)
#else
#define WPA_ASSERT(a) \
    do {              \
    } while (0)
#endif

#ifdef EAP_WSC
extern int g_wsc;
#endif

#endif /* WPA_COMMON_H */
