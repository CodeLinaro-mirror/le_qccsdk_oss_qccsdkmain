/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*========================================================================
 *
 * @file wifi_fw_logger.h
 * @brief Declarations for the Fermion_logger_framework
 *========================================================================*/
#ifndef WIFI_FW_LOGGER_H
#define WIFI_FW_LOGGER_H
#ifdef SUPPORT_FERMION_LOGGER
#include <stdbool.h>
#include "wifi_fw_mgmt_api.h"

#include <stdbool.h>

// LOGGER_FLAGS
#define FILENAME_FIELD    // Enable filename prepending to the logs at compile time
#define LOGGER_TIMESTAMP  // Enable Timestamp prepending to logs at compile time
#define LOGGER_METADATA   // Enable addition of Metadata (Taskid and number of words the log would occupy)
#define LOGGER_SEQ_NUM    // Enable sequence number at runtime
#if (NT_FN_FUNCTION_LINE_NUM_FLAG == 1)
#define LINE_FIELD  // Enable Line number prepending to the logs at compile time
#else
#undef LINE_FIELD
#endif

#ifndef LOGGER_TIMESTAMP
#define TIMESTAMP_FMT_SPFR
#else
#define TIMESTAMP_FMT_SPFR "%08u "
#endif

#ifndef LOGGER_SEQ_NUM
#define SEQ_NUM_FMT_SPFR
#else
#define SEQ_NUM_FMT_SPFR "%04u "
#endif

#ifndef LOGGER_METADATA
#define METADATA_FMT_SPFR
#else
#define METADATA_FMT_SPFR "%04x "
#endif

#define FMT_SPFRS SEQ_NUM_FMT_SPFR "" METADATA_FMT_SPFR "" TIMESTAMP_FMT_SPFR

#ifndef FILENAME_FIELD
#define FILENAME_STRING
#else
#define FILENAME_STRING STRINGIZE(FILENAME) "" STRINGIZE(:)
#endif

#ifndef LINE_FIELD
#define LINE_STRING
#else
#define LINE_STRING \
    STRINGIZE(__LINE__)"" " "
#endif

#define STRINGIZE_VALUE(x) #x
#define STRINGIZE(x)       STRINGIZE_VALUE(x)
#define PINPOINT           FILENAME_STRING "" LINE_STRING

#define DEBUG_LOG_BUF_SIZE            1024
#define DEBUG_LOG_MAX_LEVEL           4
#define DEBUG_LOG_BUF_MASK            (DEBUG_LOG_BUF_SIZE - 1)
#define DEBUG_LOG_STRING_ATTR         __attribute__((section(".DBG_STRING")))
#define DEBUG_LOG_STRING(label, text) DEBUG_LOG_STRING_IMPL(label, text)
#define DEBUG_LOG_STRING_IMPL(label, text) \
    DEBUG_LOG_STRING_ATTR static const char label[] = FMT_SPFRS "" PINPOINT "" text

// Priority of flush callback among presleep callbacks
#define FERM_LOG_FLUSH_PRIO 1
// 75% of the buffer, as threshold for BMPS
#define FERM_LOG_FLUSH_THR_PERC_BMPS 75
// 50% of the buffer, as threshold, if idle
#define FERM_LOG_FLUSH_THR_PERC_IDLE 50

typedef enum ferm_uart_logger_state {
    FERM_UART_LOGGER_DEFAULT = 0,
    FERM_UART_LOGGER_FORCE_ENABLE = 1,
    FERM_UART_LOGGER_FORCE_DISABLE = 2,
    FERM_UART_LOGGER_MAX
} ferm_uart_logger_state_t;

/*Typedefs*/
typedef unsigned int debug_log_elem;

/*Global function definitions*/
void fw_logger_init(void);

void log_to_buffer_fixed(const char *event_key, debug_log_elem p1, debug_log_elem p2, debug_log_elem p3);

void log_to_buffer_variadic(const char *event_key, size_t n_args, ...);

void log_arr_to_buffer(const char *event_key, const uint8_t *ptr, const uint16_t len);

void log_dynamic_string_to_buffer(const char *event_key, char *string_ptr);

void log_dynamic_string_to_buffer_with_len(const char *event_key, char *string_ptr, uint16_t string_length);

void log_to_buffer_minimal(const char *event_key, size_t n_args, ...);

void clear_debug_buffer(void);

void fw_logger_print_status(void);

void fw_logger_set_host_attached(host_logger_state input);

bool is_debugBuffer_read(void);

void fw_logger_over_uart_set(uint8_t uart_option);
#ifdef FEATURE_FPCI
void host_log_flush_cb(uint8_t evt, void *p_args);
#endif

#define HAS_NOT_FORMAT_SPECIFIER(str) !!(__builtin_strchr(str, '%'))
// NT_LOG_PRINT_FORMAT_CHK
// +-------------------------------+------------------+-----------+
// |             Cdtn1             |      Cdtn2       |  Sanity   |
// +-------------------------------+------------------+-----------+
// | Log has no format specifier   | Log has no args  | Valid     |
// | Log has no format specifier   | Log has args     | Not valid |
// | Log has format specifier      | Log has no args  | Not valid |
// | Log has format specifier      | Log has args     | Valid     |
// +-------------------------------+------------------+-----------+
#define NT_LOG_PRINT_FORMAT_CHK(msg, n_args)                                                                \
    _Static_assert(LOG_HAS_NO_FMT_SPFRS_AND_NO_ARGS(msg, n_args) || LOG_HAS_FMT_SPFR_AND_ARGS(msg, n_args), \
                   "Format specifier error!");
#define LOG_HAS_NO_FMT_SPFRS_AND_NO_ARGS(msg, n_args) \
    ((true == (n_args == 0)) && (false == HAS_NOT_FORMAT_SPECIFIER(msg)))
#define LOG_HAS_FMT_SPFR_AND_ARGS(msg, n_args) ((true == HAS_NOT_FORMAT_SPECIFIER(msg)) && (false == (n_args == 0)))
#define NT_LOG_MOD_LVL_FORMAT_CHK(msg)                         \
    _Static_assert((false == (HAS_NOT_FORMAT_SPECIFIER(msg))), \
                   "This type of log shall not contain any format specifier!");

#define WLAN_DBG_PRINT_FORMAT_CHK NT_LOG_MOD_LVL_FORMAT_CHK

#endif  // SUPPORT_FERMION_LOGGER
#endif  // WIFI_FW_LOGGER_H
