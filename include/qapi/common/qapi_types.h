/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/

/**
 * @file qapi_types.h
 *
 * @brief QAPI base type definitions
 *
 * @details This file provides the base type definitions used by the QAPI.
 *          This includes the basic integer types (based on stdint.h and
 *          stddef.h) and a basic boolean type.
 */

#ifndef __QAPI_TYPES_H__ // [
#define __QAPI_TYPES_H__

#include <stdint.h>
#include <stddef.h>
/** boolean data type. */
typedef uint32_t qbool_t;

#ifndef false
   #define false     (0)
#endif

#ifndef true
   #define true      (1)
#endif

#endif // ] #ifndef __QAPI_TYPES_H__

