/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*
 * DALStdDef.h
 *
 *  Created on: 02-Jun-2020
 *      Author: HIMADRI
 */

#ifndef CORE_COMMON_DALSTDDEF_H_
#define CORE_COMMON_DALSTDDEF_H_

#ifndef _UINT32_DEFINED
typedef unsigned long int uint32; /* Unsigned 32 bit value */
#define _UINT32_DEFINED
#endif

#if defined(__GNUC__)
#define __int64 long long
#endif

#ifndef _UINT64_DEFINED
typedef unsigned __int64 uint64; /* Unsigned 64 bit value */
#define _UINT64_DEFINED
#endif

#ifndef _INT64_DEFINED
typedef __int64 int64; /* Signed 64 bit value */
#define _INT64_DEFINED
#endif

#ifndef _BYTE_DEFINED
typedef unsigned char byte; /* byte type */
#define _BYTE_DEFINED
#endif

typedef uint32 DALBOOL;
typedef uint32 DALDEVICEID;
typedef uint32 DalPowerCmd;
typedef uint32 DalPowerDomain;
typedef uint32 DalSysReq;
typedef uint32 DALHandle;
typedef int DALResult;
typedef void *DALEnvHandle;
typedef void *DALSYSEventHandle;
typedef uint32 DALMemAddr;
typedef uint32 DALSYSMemAddr;
typedef uint64 DALSYSPhyAddr;
typedef uint32 DALInterfaceVersion;

#ifndef TRUE
#define TRUE 1 /* Boolean true value. */
#endif

#ifndef FALSE
#define FALSE 0 /* Boolean false value. */
#endif

#ifndef NULL
#define NULL 0
#endif

#endif /* CORE_COMMON_DALSTDDEF_H_ */
