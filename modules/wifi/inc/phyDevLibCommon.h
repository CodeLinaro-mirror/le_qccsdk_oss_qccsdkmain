/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _PHYDEVLIB_COMMON_H_
#define _PHYDEVLIB_COMMON_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __GNUC__
#include <nt_flags.h>
#include <nt_logger_api.h>
#include <nt_osal.h>
#include <hal_int_sys.h>
#endif

#ifdef _WIN32
#include <stdio.h>
#include <Windows.h>
#endif

#ifdef __GNUC__
#define __ATTRIB_PACK    __attribute__((packed))
#define phyLog(fmt, ...) NT_LOG_PRINT(HALPHY, INFO, fmt, ##__VA_ARGS__)

#ifndef A_ASSERT
#define A_ASSERT configASSERT
#endif

#endif

#ifdef _WIN32
#define __ATTRIB_PACK
#define phyLog(format, ...)          \
    {                                \
        printf(format, __VA_ARGS__); \
        printf("\n");                \
    }
#endif

#endif /* _PHYDEVLIB_COMMON_H_ */
