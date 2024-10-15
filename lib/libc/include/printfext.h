/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/



#ifndef __PRINTF_EXT__
#define __PRINTF_EXT__

#include <stdio.h>
#include <autoconf.h>

/*
//defined in Kconfig

#define CONFIG_PRINTF_ERR  1
#define CONFIG_PRINTF_WARN 1
#define CONFIG_PRINTF_INFO 1
#define CONFIG_PRINTF_LOG  0
#define CONFIG_PRINTF_DUMP 0

#define CONFIG_ERROR_PREFIX  "[ERROR] "
#define CONFIG_WARN_PREFIX  "[WARNING] "
#define CONFIG_INFO_PREFIX  "[INFO] "
#define CONFIG_LOG_PREFIX  "[LOG] "
#define CONFIG_DUMP_PREFIX  "[DUMP] "
*/

#if CONFIG_PRINTF_ERR
#define err_printf(msg,...)     printf(CONFIG_ERROR_PREFIX msg, ##__VA_ARGS__)
#else
#define err_printf(args...)     do { } while (0)
#endif

#if CONFIG_PRINTF_WARN
#define warn_printf(msg,...)     printf(CONFIG_WARN_PREFIX msg, ##__VA_ARGS__)
#else
#define warn_printf(args...)     do { } while (0)
#endif

#if CONFIG_PRINTF_INFO
#define info_printf(msg,...)     printf(CONFIG_INFO_PREFIX msg, ##__VA_ARGS__)
#else
#define info_printf(args...)     do { } while (0)
#endif

#if CONFIG_PRINTF_LOG
#define log_printf(msg,...)     printf(CONFIG_LOG_PREFIX msg, ##__VA_ARGS__)
#else
#define log_printf(args...)     do { } while (0)
#endif

#if CONFIG_PRINTF_DUMP
#define dump_printf(msg,...)     printf(CONFIG_DUMP_PREFIX msg, ##__VA_ARGS__)
#else
#define dump_printf(args...)     do { } while (0)
#endif

#define PRINT_ERR_NOT_SUPPORTED         err_printf("Not supported yet\n")
#define PRINT_ERR_INVALID_PARAM         err_printf("Invalid paramter\n")
#define PRINT_ERR_INVALID_PARAM1(msg, argx)         err_printf("Invalid paramter: " msg "=0x%x\n", argx)
#define PRINT_ERR_ALREADY_EXIST         err_printf("Already exist\n")
#define PRINT_ERR_WMI_CMD_SEND_FAILED   err_printf("WMI command send failed\n")
#define PRINT_ERR_NO_RESOURCE           err_printf("No resource\n")

#define PRINT_WARN_SKIP                 warn_printf("Skip\n")

#define PRINT_LOG_FUNC_LINE             log_printf("%s %d\n", __FUNCTION__, __LINE__)
#define PRINT_LOG_FUNC_LINE_ENTRY       log_printf("%s %d entry\n", __FUNCTION__, __LINE__)
#define PRINT_LOG_FUNC_LINE_EXIT        log_printf("%s %d exit\n", __FUNCTION__, __LINE__)

#endif //__PRINTF_EXT__

