
/*
 * Dynamic data buffer
 * Copyright (c) 2007-2009, Jouni Malinen <j@w1.fi>
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

#ifndef WPSBUF_H
#define WPSBUF_H

/*
 * Internal data structure for wpsbuf. Please do not touch this directly from
 * elsewhere. This is only defined in header file to allow inline functions
 * from this file to access data.
 */
#include "wps_api.h"

#define broadcast_ether_addr (const uint8_t *)"\xff\xff\xff\xff\xff\xff"

struct wpsbuf *wpsbuf_alloc(size_t len);
struct wpsbuf *wpsbuf_alloc_copy(const void *data, size_t len);
struct wpsbuf *wpsbuf_dup(const struct wpsbuf *src);
void wpsbuf_free(struct wpsbuf *buf);
void *wpsbuf_put(struct wpsbuf *buf, size_t len);
struct wpsbuf *wpsbuf_zeropad(struct wpsbuf *buf, size_t len);

// NEUTRINO FIX-ME: The following functions are originally
// defined as inline static functions but they are throwing
// unused error warnings, hence making them non-static functions
// and moved them to wpsbuf.c file, once wpa/wpa2 in place
// these functions can be changed backed to inline functions
size_t wpsbuf_len(const struct wpsbuf *buf);
size_t wpsbuf_tailroom(const struct wpsbuf *buf);
const void *wpsbuf_head(const struct wpsbuf *buf);
const uint8_t *wpsbuf_head_u8(const struct wpsbuf *buf);
void *wpsbuf_mhead(struct wpsbuf *buf);
uint8_t *wpsbuf_mhead_u8(struct wpsbuf *buf);
void wpsbuf_put_u8(struct wpsbuf *buf, uint8_t data);
void wpsbuf_put_be16(struct wpsbuf *buf, uint16_t data);
void wpsbuf_put_be24(struct wpsbuf *buf, uint32_t data);
void wpsbuf_put_be32(struct wpsbuf *buf, uint32_t data);
void wpsbuf_put_data(struct wpsbuf *buf, const void *data, size_t len);
void wpsbuf_put_buf(struct wpsbuf *dst, const struct wpsbuf *src);
void wpsbuf_set(struct wpsbuf *buf, const void *data, size_t len);

#ifdef ATH_KF
/**
 * wpsbuf_size - Get the currently allocated size of a wpsbuf buffer
 * @buf: wpsbuf buffer
 * Returns: Currently allocated size of the buffer
 */
static inline size_t wpsbuf_size(const struct wpsbuf *buf)
{
    return buf->size;
}

/**
 * wpsbuf_len - Get the current length of a wpsbuf buffer data
 * @buf: wpsbuf buffer
 * Returns: Currently used length of the buffer
 */
static inline size_t wpsbuf_len(const struct wpsbuf *buf)
{
    return buf->used;
}

/**
 * wpsbuf_tailroom - Get size of available tail room in the end of the buffer
 * @buf: wpsbuf buffer
 * Returns: Tail room (in bytes) of available space in the end of the buffer
 */
static inline size_t wpsbuf_tailroom(const struct wpsbuf *buf)
{
    return buf->size - buf->used;
}

/**
 * wpsbuf_head - Get pointer to the head of the buffer data
 * @buf: wpsbuf buffer
 * Returns: Pointer to the head of the buffer data
 */
static inline const void *wpsbuf_head(const struct wpsbuf *buf)
{
    if (buf->ext_data)
        return buf->ext_data;
    return buf + 1;
}

static inline const uint8_t *wpsbuf_head_u8(const struct wpsbuf *buf)
{
    return wpsbuf_head(buf);
}

/**
 * wpsbuf_mhead - Get modifiable pointer to the head of the buffer data
 * @buf: wpsbuf buffer
 * Returns: Pointer to the head of the buffer data
 */
static inline void *wpsbuf_mhead(struct wpsbuf *buf)
{
    if (buf->ext_data)
        return buf->ext_data;
    return buf + 1;
}

static inline uint8_t *wpsbuf_mhead_u8(struct wpsbuf *buf)
{
    return wpsbuf_mhead(buf);
}

static inline void wpsbuf_put_u8(struct wpsbuf *buf, uint8_t data)
{
    uint8_t *pos = wpsbuf_put(buf, 1);
    *pos = data;
}

static inline void wpsbuf_put_le16(struct wpsbuf *buf, uint16_t data)
{
    uint8_t *pos = wpsbuf_put(buf, 2);
    WPA_PUT_LE16(pos, data);
}

static inline void wpsbuf_put_be16(struct wpsbuf *buf, uint16_t data)
{
    uint8_t *pos = wpsbuf_put(buf, 2);
    WPA_PUT_BE16(pos, data);
}

static inline void wpsbuf_put_be24(struct wpsbuf *buf, uint32_t data)
{
    uint8_t *pos = wpsbuf_put(buf, 3);
    WPA_PUT_BE24(pos, data);
}

static inline void wpsbuf_put_be32(struct wpsbuf *buf, uint32_t data)
{
    uint8_t *pos = wpsbuf_put(buf, 4);
    WPA_PUT_BE32(pos, data);
}

static inline void wpsbuf_put_data(struct wpsbuf *buf, const void *data, size_t len)
{
    if (data)
        memcpy(wpsbuf_put(buf, len), data, len);
}

static inline void wpsbuf_put_buf(struct wpsbuf *dst, const struct wpsbuf *src)
{
    wpsbuf_put_data(dst, wpsbuf_head(src), wpsbuf_len(src));
}

static inline void wpsbuf_set(struct wpsbuf *buf, const void *data, size_t len)
{
    buf->ext_data = (uint8_t *)data;
    buf->size = buf->used = len;
}

static inline void wpsbuf_put_str(struct wpsbuf *dst, const char *str)
{
    wpsbuf_put_data(dst, str, strlen(str));
}
#endif  // ATH_KF

#endif /* WPSBUF_H */
