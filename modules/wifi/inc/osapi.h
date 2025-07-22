/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __OSAPI_H__
#define __OSAPI_H__

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define NT_COMPILE_TIME_ASSERT(assertion_name, predicate) typedef char assertion_name[(predicate) ? 1 : -1];

#if !defined(LOCAL)
#if 0 /* At least for now, simplify debugging. */
#define LOCAL static
#else
#define LOCAL
#endif
#endif

#if !defined(NULL)
#define NULL (void *)0
#endif

#if !defined(FALSE)
#define FALSE 0
#endif

#if !defined(TRUE)
#define TRUE 1
#endif

#ifdef __GNUC__
//#define __ATTRIB_PACK           __attribute__ ((packed))
#define __ATTRIB_PRINTF   __attribute__((format(printf, 1, 2)))
#define __ATTRIB_NORETURN __attribute__((noreturn))
#define __ATTRIB_ALIGN(x) __attribute__((aligned((x))))
#ifndef INLINE
#define INLINE __inline__
#endif
#else /* Not GCC */
#define __ATTRIB_PACK
#define __ATTRIB_PRINTF
#define __ATTRIB_NORETURN
#define __ATTRIB_ALIGN(x)
#define INLINE __inline
#endif /* End __GNUC__ */

/*
 * Packing support.
 * Structures that are shared between Host and Target are packed
 * for several reasons:
 *  1) reduces over-the wire transmission cost and
 *  2) allows for Host compilers with different native structure layouts
 *  3) allows a 64-bit Host to work with a 32-bit Target
 *
 * In order to accommodate various compilers, we use both a PREPACK and
 * a POSTPACK macro -- one before and one after the declaration. For a
 * given compiler, only one of these is defined.
 *
 * The *64 variety of packing is used only when a structure is known
 * a priori to not need any packing on a 32-bit system (e.g. a structure
 * of 32-bit quantities). Such a structure may need packing exclusively
 * so that members can be referenced properly on a 64-bit Host.
 */
#define PREPACK
#define POSTPACK __ATTRIB_PACK
#define PREPACK64
#define POSTPACK64

/* Utility macros */
#define NT_SWAB32(_x)                                                                                                \
    (({                                                                                                              \
        uint32_t __x = (_x);                                                                                         \
        (uint32_t)(                                                                                                  \
            (((uint32_t)(__x) & (uint32_t)0x000000ffUL) << 24) | (((uint32_t)(__x) & (uint32_t)0x0000ff00UL) << 8) | \
            (((uint32_t)(__x) & (uint32_t)0x00ff0000UL) >> 8) | (((uint32_t)(__x) & (uint32_t)0xff000000UL) >> 24)); \
    }))

#define NT_SWAB16(_x)                                                                                            \
    ({                                                                                                           \
        uint16_t __x = (_x);                                                                                     \
        (uint16_t)((((uint16_t)(__x) & (uint16_t)0x00ffU) << 8) | (((uint16_t)(__x) & (uint16_t)0xff00U) >> 8)); \
    })

/* unaligned little endian access */
#define NT_LE_READ_2(p) ((uint16_t)((((uint8_t *)(p))[0]) | (((uint8_t *)(p))[1] << 8)))

#define NT_LE_READ_4(p)                                                                            \
    ((uint32_t)((((uint8_t *)(p))[0]) | (((uint8_t *)(p))[1] << 8) | (((uint8_t *)(p))[2] << 16) | \
                (((uint8_t *)(p))[3] << 24)))

#define NT_LE64_TO_CPU(_x)  ((uint64_t)(_x))
#define NT_LE32_TO_CPU(_x)  ((uint32_t)(_x))
#define NT_CPU_TO_LE32(_x)  ((uint32_t)(_x))
#define NT_BE32_TO_CPU(_x)  NT_SWAB32(_x)
#define NT_CPU_TO_BE32(_x)  NT_SWAB32(_x)
#define NT_LE16_TO_CPU(_x)  ((uint16_t)(_x))
#define NT_CPU_TO_LE16(_x)  ((uint16_t)(_x))
#define NT_BE16_TO_CPU(_x)  NT_SWAB16(_x)
#define NT_CPU_TO_BE16(_x)  NT_SWAB16(_x)
#define NT_CPU_TO_NET16(_x) NT_SWAB16(_x)
#define NT_NET16_TO_CPU(_x) NT_SWAB16(_x)
#define NT_CPU_TO_NET32(_x) NT_SWAB32(_x)
#define NT_NET32_TO_CPU(_x) NT_SWAB32(_x)

#define NT_LE32TOH(_x) NT_LE32_TO_CPU(_x)
#define NT_HTOLE32(_x) NT_CPU_TO_LE32(_x)
#define NT_BE32TOH(_x) NT_BE32_TO_CPU(_x)
#define NT_HTOBE32(_x) NT_CPU_TO_BE32(_x)
#define NT_LE16TOH(_x) NT_LE16_TO_CPU(_x)
#define NT_HTOLE16(_x) NT_CPU_TO_LE16(_x)
#define NT_BE16TOH(_x) NT_BE16_TO_CPU(_x)
#define NT_HTOBE16(_x) NT_CPU_TO_BE16(_x)

#define NT_MAX(_x, _y)             \
    ({                             \
        __typeof__(_x) __x = (_x); \
        __typeof__(_y) __y = (_y); \
        __x > __y ? __x : __y;     \
    })

#define NT_MIN(_x, _y)             \
    ({                             \
        __typeof__(_x) __x = (_x); \
        __typeof__(_y) __y = (_y); \
        __x < __y ? __x : __y;     \
    })

#define NT_ABS(_x)                 \
    ({                             \
        __typeof__(_x) __x = (_x); \
        (__x >= 0) ? __x : -__x;   \
    })

#define NT_ROUND_UP(x, y)            ((((x) + ((y)-1)) / (y)) * (y))
#define NT_ROUND_UP_PAD(x, y)        (NT_ROUND_UP(x, y) - (x))
#define NT_ROUND_UP_PWR2(x, align)   (((int)(x) + ((align)-1)) & ~((align)-1))
#define NT_ROUND_DOWN_PWR2(x, align) ((int)(x) & ~((align)-1))

#define NT_TOLOWER(c) (((c) >= 'A' && (c) <= 'Z') ? ((c) - 'A' + 'a') : (c))
#define NT_TOUPPER(c) (((c) >= 'a' && (c) <= 'z') ? ((c) - 'a' + 'A') : (c))

#define NT_ARRAY_NUM_ENTRIES(a)      (sizeof(a) / sizeof(*(a)))
#define NT_FIELD_OFFSET(type, field) ((int)(&((type *)0)->field))

#define NT_MSECS_PER_SECOND 1000       /* Milliseconds */
#define NT_USECS_PER_SECOND 1000000    /* Microseconds */
#define NT_NSECS_PER_SECOND 1000000000 /* Nanoseconds  */

#define NT_LONG_MAX 0xFFFFFFFF

/* align a pointer a boundary that is a power of 2 */
#define NT_ALIGN_PTR_PWR2(p, align) (void *)(((uint32_t)(p) + ((align)-1)) & ~((align)-1))

/*
 * For addition operation that requires overflow checks
 * The code will assert if overflow happens
 */
#define ADD_UINT32_WITH_OVERFLOW_ASSERT(sum, a, b)                               \
    do {                                                                         \
        uint32_t sum_to_be_checked = a + b;                                      \
        NT_ASSERT_ALWAYS(1 && sum_to_be_checked >= a && sum_to_be_checked >= b); \
        sum = sum_to_be_checked;                                                 \
    } while (0)

#endif /* __OSAPI_H__ */
