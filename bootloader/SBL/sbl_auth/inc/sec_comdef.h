/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/
#ifndef __BOOT_COMDEF_H
#define __BOOT_COMDEF_H
/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*
                 Boot Common Definition Header File

GENERAL DESCRIPTION
  Common definitions used throughout the boot code.

EXTERNALIZED FUNCTIONS
  None

INITIALIZATION AND SEQUENCING REQUIREMENTS
  None

*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/


/*===========================================================================

                        EDIT HISTORY FOR MODULE

  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.

when       who     what, where, why
--------   ---     ----------------------------------------------------------
11/04/11   dxiang  Initial version for MSM8974
===========================================================================*/

/* ------------------------------------------------------------------------
** Constants
** ------------------------------------------------------------------------ */


#define TRUE   1   /* Boolean true value. */
#define FALSE  0   /* Boolean false value. */

#define  ON   1    /* On value. */
#define  OFF  0    /* Off value. */

#ifdef _lint
  #define NULL 0
#endif

#ifndef NULL
  #define NULL  0
#endif

/* For unrestricted/untruncated conversion between void* and uint32_t, so that they can be used interchangeably for pointer arithematic */
#define INT_TO_PTR(x) (void *)(uintptr_t)(x)


/* -----------------------------------------------------------------------
** Standard Types
** ----------------------------------------------------------------------- */

/* The following definitions are the same accross platforms.  This first
** group are the sanctioned types.
*/
#ifndef _BOOLEAN_DEFINED
typedef  unsigned int      boolean;     /* Boolean value type. */
#define _BOOLEAN_DEFINED
#endif

typedef  unsigned long long uint64;      /* Unsigned 64 bit value */
typedef  unsigned long int  uint32;      /* Unsigned 32 bit value */
typedef  unsigned short     uint16;      /* Unsigned 16 bit value */
typedef  unsigned char      uint8;       /* Unsigned 8  bit value */

typedef  signed long long   int64;       /* Signed 64 bit value */
typedef  signed long int    int32;       /* Signed 32 bit value */
typedef  signed short       int16;       /* Signed 16 bit value */
typedef  signed char        int8;        /* Signed 8  bit value */

/* This group are the deprecated types.  Their use should be
** discontinued and new code should use the types above
*/
typedef  unsigned char     byte;         /* Unsigned 8  bit value type. */
typedef  unsigned short    word;         /* Unsigned 16 bit value type. */
typedef  unsigned long     dword;        /* Unsigned 32 bit value type. */

typedef  unsigned char     uint1;        /* Unsigned 8  bit value type. */
typedef  unsigned short    uint2;        /* Unsigned 16 bit value type. */
typedef  unsigned long     uint4;        /* Unsigned 32 bit value type. */

typedef  signed char       int1;         /* Signed 8  bit value type. */
typedef  signed short      int2;         /* Signed 16 bit value type. */
typedef  long int          int4;         /* Signed 32 bit value type. */

typedef  signed long       sint31;       /* Signed 32 bit value */
typedef  signed short      sint15;       /* Signed 16 bit value */
typedef  signed char       sint7;        /* Signed 8  bit value */

typedef unsigned long       uintnt;      /* Unsigned long that will be:
                                            32 bit value for AARCH32 */

#ifdef LOCAL
#undef LOCAL
#endif /* LOCAL */

#define LOCAL static

#ifndef MIN
   #define  MIN( x, y ) ( ((x) < (y)) ? (x) : (y) )
#endif

#define UNUSED_PARAMETER(param) (void)(param)
#define PBL_DONE TRUE
#define PBL_FAIL FALSE


// Bala is putting the entire content below for PACKED_STRUCT under #if 0
// Reason: In istari, we have it in shared/comdef.h, and in quartz also,
// we have it defined there.
#if 0
#if defined(__ARMCC_VERSION)
#define PACKED_STRUCT struct __attribute__((packed))
#elif defined(__GNUC__)
#define PACKED_STRUCT struct __attribute__((packed))
#else
#error Unknown compiler
#endif
#endif

#if defined(__ARMCC_VERSION)
#define INLINE __inline
#define inline __inline
#elif defined(__GNUC__)
#define INLINE static inline
#else
#error Unknown compiler
#endif

#if defined(__ARMCC_VERSION)
#define PACKED_STRUCT __packed struct
#elif defined(__GNUC__)
#define PACKED_STRUCT struct __attribute__((packed))
#else
#error Unknown compiler
#endif

#if defined(__ARMCC_VERSION)
#define ALIGNED(size, decl) __align(size) decl
#elif defined(__GNUC__)
#define ALIGNED(size, decl) decl __attribute__((aligned(size))
#define restrict __restrict
#else
#error Unknown compiler
#endif

#if defined(__ARMCC_VERSION) || defined(__GNUC__)
#define SECTION(name) __attribute__((section (name)))
#else
#error Unknown compiler
#endif

#define PACKED_POST
#if 1
#define memcpy(des, src, len)  (void)qmemcpy(des, src, len)
#define memset(des, val, len)  (void)qmemset(des, val, len)
#define memcmp(des, val, len)  bByteCompare(des, val, len)
#endif

#include "sec_util.h"

#endif /* __BOOT_COMDEF_H */

