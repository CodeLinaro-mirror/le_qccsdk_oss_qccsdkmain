/**
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __LIB_PRINTF_EXT__
#define __LIB_PRINTF_EXT__

#include <stdio.h>

#define LIB_PRINTF_ERR  1
#define LIB_PRINTF_WARN 1
#define LIB_PRINTF_INFO 1
#define LIB_PRINTF_LOG  0
#define LIB_PRINTF_DUMP 0

#define LIB_ERROR_PREFIX  "[LIBERROR] "
#define LIB_WARN_PREFIX  "[LIBWARNING] "
#define LIB_INFO_PREFIX  "[LIBINFO] "
#define LIB_LOG_PREFIX  "[LIBLOG] "
#define LIB_DUMP_PREFIX  "[LIBDUMP] "

#if LIB_PRINTF_ERR
#define err_lib_printf(msg,...)     printf(LIB_ERROR_PREFIX msg, ##__VA_ARGS__)
#else
#define err_lib_printf(args...)     do { } while (0)
#endif

#if LIB_PRINTF_WARN
#define warn_lib_printf(msg,...)     printf(LIB_WARN_PREFIX msg, ##__VA_ARGS__)
#else
#define warn_lib_printf(args...)     do { } while (0)
#endif

#if LIB_PRINTF_INFO
#define info_lib_printf(msg,...)     printf(LIB_INFO_PREFIX msg, ##__VA_ARGS__)
#else
#define info_lib_printf(args...)     do { } while (0)
#endif

#if LIB_PRINTF_LOG
#define log_lib_printf(msg,...)     printf(LIB_LOG_PREFIX msg, ##__VA_ARGS__)
#else
#define log_lib_printf(args...)     do { } while (0)
#endif

#if LIB_PRINTF_DUMP
#define dump_lib_printf(msg,...)     printf(LIB_DUMP_PREFIX msg, ##__VA_ARGS__)
#else
#define dump_lib_printf(args...)     do { } while (0)
#endif

#define LIB_PRINT_ERR_NOT_SUPPORTED         err_lib_printf("Not supported yet\n")
#define LIB_PRINT_ERR_INVALID_PARAM         err_lib_printf("Invalid paramter\n")
#define LIB_PRINT_ERR_INVALID_PARAM1(msg, argx)         err_lib_printf("Invalid paramter: " msg "=0x%x\n", argx)
#define LIB_PRINT_ERR_ALREADY_EXIST         err_lib_printf("Already exist\n")
#define LIB_PRINT_ERR_WMI_CMD_SEND_FAILED   err_lib_printf("WMI command send failed\n")
#define LIB_PRINT_ERR_NO_RESOURCE           err_lib_printf("No resource\n")

#define LIB_PRINT_WARN_SKIP                 warn_lib_printf("Skip\n")

#define LIB_PRINT_LOG_FUNC_LINE             log_lib_printf("%s %d\n", __FUNCTION__, __LINE__)
#define LIB_PRINT_LOG_FUNC_LINE_ENTRY       log_lib_printf("%s %d entry\n", __FUNCTION__, __LINE__)
#define LIB_PRINT_LOG_FUNC_LINE_EXIT        log_lib_printf("%s %d exit\n", __FUNCTION__, __LINE__)

#endif //__LIB_PRINTF_EXT__

