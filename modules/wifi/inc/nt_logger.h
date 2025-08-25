/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _NT_LOGGER_H
#define _NT_LOGGER_H

#ifdef NT_FN_LOGGER
/* -- Includes -- */
#include <stdint.h> /* for uint32_t data type */
#include "nt_flags.h"
/*@ for max msg length */
#if NT_FN_FIXED_ARG_LOGGER
#define NT_LOG_MAX_MSG_LENGTH 39
#else
#define NT_LOG_MAX_MSG_LENGTH 50
#endif  // NT_FN_FIXED_ARG_LOGGER
/*@ for maximum function length*/
#define NT_LOG_MAX_FUNCTION_NAME_LENGTH 16

/*@ for maximum task name length */
#define NT_LOG_MAX_TASK_NAME_LENGTH 16
/*! @max log levels */
#define NT_LOG_LVL_MAX_LOG_LEVEL 4
/*@ maximum size of log */
#define NT_LOG_MAX_LOG_SIZE 92

/*! @Max buffer size. */
/* In cMEM build logger buffer size is 2KB
 * and in other build size will be 4KB per
 * buffer
 * */
#ifdef NT_CMEM_BUILD
#define NT_LOG_MAX_BUFF_SIZE 2003
#else
/* Size of the loggers is now changed to 3KB from 4KB
 * and total logger size for all logs will be 12KB
 * Below macro will define the buffer size.
 * */
#define NT_LOG_MAX_BUFF_SIZE 3005
#endif

#define NT_LOG_TOTAL_BLOCKS ((NT_LOG_MAX_BUFF_SIZE - 1) / (NT_LOG_MAX_LOG_SIZE - 1))
/**
 * This structure used to create a file and fill the buffer
 */

typedef struct log_ringdesc_s {
    /**< for position of buffer index */
    uint16_t head_index;

    /**< save data to buffer*/
    char buffer[NT_LOG_MAX_BUFF_SIZE - 1];
} log_ringdesc_t;

#if NT_FN_FIXED_ARG_LOGGER
typedef struct __attribute__((packed)) log_blockdesc_s {
    /*@ for log level initialize*/
    uint8_t loglvl;
    /*@ all module ids initialized by module_id*/
    uint8_t module_id;
    /*@ user provided parameters.*/
    unsigned int long p1;
    unsigned int long p2;
    unsigned int long p3;
    /* for function line number */
#if (NT_FN_FUNCTION_LINE_NUM_FLAG == 1)
    uint16_t ln;
#endif  // NT_FN_FUNCTION_LINE_NUM_FLAG
    /*@ xTaskGetTickCount initialized by time_stamp*/
    uint32_t time_stamp;
    /*@ for function name  */
#if (NT_FN_FUNCTION_LINE_NUM_FLAG == 1)
    char func_name[NT_LOG_MAX_FUNCTION_NAME_LENGTH];
#endif  // NT_FN_FUNCTION_LINE_NUM_FLAG
    /*@ taskName provided by task handler*/
    char taskName[NT_LOG_MAX_TASK_NAME_LENGTH];
    /**< save data to buffer*/
    uint8_t msg[NT_LOG_MAX_MSG_LENGTH];

} log_blockdesc_t;

#else

typedef struct __attribute__((packed)) log_blockdesc_s {
    /*@ for log level initialize*/
    uint8_t loglvl;
    /*@ all module ids initialized by module_id*/
    uint8_t module_id;
    /* for function line number */
#if (NT_FN_FUNCTION_LINE_NUM_FLAG == 1)
    uint16_t ln;
#endif  // NT_FN_FUNCTION_LINE_NUM_FLAG
    /*@ xTaskGetTickCount initialized by time_stamp*/
    uint32_t time_stamp;
    /*@ for function name  */
#if (NT_FN_FUNCTION_LINE_NUM_FLAG == 1)
    char func_name[NT_LOG_MAX_FUNCTION_NAME_LENGTH];
#endif  // NT_FN_FUNCTION_LINE_NUM_FLAG
    /*@ taskName provided by task handler*/
    char taskName[NT_LOG_MAX_TASK_NAME_LENGTH];
    /**< save data to buffer*/
    uint8_t msg[NT_LOG_MAX_MSG_LENGTH];

} log_blockdesc_t;

#endif  // NT_FN_FIXED_ARG_LOGGER

#endif  // NT_FN_LOGGER
#endif  //_NT_LOGGER_H
