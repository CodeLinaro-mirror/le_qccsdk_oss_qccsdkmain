/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*========================================================================
 * @file wifi_fw_logger.c
 * @brief This file contains the implementation of the Fermion_logger_framework
 *========================================================================*/
/*-------------------------------------------------------------------------
 * Include Files
 * ----------------------------------------------------------------------*/
#include <stdio.h>
#include <stddef.h>
#include <inttypes.h>
#include <stdarg.h>
#include "nt_osal.h"
#include "nt_timer.h"
#include "task.h"
#include "portmacro.h"
#include "wifi_fw_logger.h"
#include "stdint.h"
#include "semphr.h"
#include "fwconfig_cmn.h"
#include "nt_flags.h"
#include "nt_hw_support.h"
#include "uart.h"
#include "wifi_fw_internal_api.h"
#include "wifi_fw_pwr_cb_infra.h"
#ifdef FIRMWARE_APPS_INFORMED_WAKE
#include "wifi_fw_ext_intr.h"
#endif /* FIRMWARE_APPS_INFORMED_WAKE */
#ifdef SUPPORT_FERMION_LOGGER
/*-------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * ----------------------------------------------------------------------*/
// The following macro helps finding a null byte in a 32bit word, in an efficient way than an iterative approach
#define WORD_HAS_NULL_BYTE(X) ~((((*((uint32_t *)X) & 0x7F7F7F7F) + 0x7F7F7F7F) | *((uint32_t *)X)) | 0x7F7F7F7F)
#define SIZE_UINT32_T         4
#define NT_IS_ISR             ((HW_REG_RD(NT_ICSR_REG)) & NT_SCB_ICSR_VECTACTIVE_Msk) != 0
// Following macro is to assume attachment of SPI logger with any change in read pointer.
// To be used only when the host is not capable of sending set_param command
#define WAR_HOST_LOGGER_ATTACH_DETECT
/*-------------------------------------------------------------------------
 * Global Data Definitions
 * ----------------------------------------------------------------------*/

/* DO NOT CHANGE NAMES of the below variables. They are as expected by PyDbg scripts. */
volatile debug_log_elem debugBuffer[DEBUG_LOG_BUF_SIZE];
volatile size_t debugBufferSize = DEBUG_LOG_BUF_SIZE;
uint8_t set_host_loglvl[NT_MAX_MODULE_ID];
volatile size_t debugBufferPos = 0;

/* Index holding Host-through-SPI read position */
volatile uint32_t g_SPI_host_read_pos;
#if (defined CONFIG_NT_RCLI)
extern xTaskHandle xRCLICommandConsoleTask;
#endif
nt_osal_semaphore_handle_t logger_semaphore_handle = NULL;
/*-------------------------------------------------------------------------
 * Static Variable Definitions
 * ----------------------------------------------------------------------*/
static uint32_t logger_seq_num = 0;
static host_logger_state host_logger = HOST_LOGGER_DETACHED;
static ferm_uart_logger_state_t uart_logger = FERM_UART_LOGGER_DEFAULT;

/*-------------------------------------------------------------------------
 * Static Function Definitions
 * ----------------------------------------------------------------------*/
/**
 * @brief   Tells whether the write pointer is beyond the input threshold
 * @return  boolean True - if the difference b/w read & write pointer is greater
 *                  than the given threshold
 *                  False - otherwise
 */
static bool fw_logger_unread_chk(uint8_t threshold_percentage)
{
    uint16_t threshold = DEBUG_LOG_BUF_SIZE * threshold_percentage / 100;
    uint16_t unread_buf_size = 0;
    if (debugBufferPos >= g_SPI_host_read_pos) {
        unread_buf_size = debugBufferPos - g_SPI_host_read_pos;
    } else {
        unread_buf_size = DEBUG_LOG_BUF_SIZE + debugBufferPos - g_SPI_host_read_pos;
    }
    if (unread_buf_size > threshold) {
        return true;
    } else {
        return false;
    }
}

/**
 * @brief   If Host logger is attached, inform to Host based on threshold
 * @param   threshold based on which inform to host is decided.
 * @return  none
 */
static void fw_logger_chk_inform_host(uint8_t threshold)
{
    if (host_logger == HOST_LOGGER_ATTACHED && fw_logger_unread_chk(threshold)) {
#ifdef FIRMWARE_APPS_INFORMED_WAKE
        wifi_fw_ext_f2a_signal_assert(F2A_SHORT_REASON_RING_TX_RX);
#else  /*  FIRMWARE_APPS_INFORMED_WAKE */
        wifi_fw_f2a_interrupt();
#endif /* FIRMWARE_APPS_INFORMED_WAKE */
    }
#ifdef WAR_HOST_LOGGER_ATTACH_DETECT
    else if (g_SPI_host_read_pos) {
        // NT_LOG_PRINT(WMI, ERR, "Warn: Host logger Read Ptr change detected, assuming host logger attached");
        host_logger = HOST_LOGGER_ATTACHED;
    }
#endif /* WAR_HOST_LOGGER_ATTACH_DETECT */
}

/**
 * @brief   returns of UART logging to be done or not based on other params
 * @param   none
 * @return  bool whether UART logging shall be done or not
 */
static bool fw_logger_over_uart_enabled(void)
{
    if (FERM_UART_LOGGER_FORCE_DISABLE == uart_logger) {
        return FALSE;
    } else if (FERM_UART_LOGGER_FORCE_ENABLE == uart_logger) {
        return TRUE;
    } else {
        /* If Uart is set to default, dont enable it if host logger is already on */
        if (HOST_LOGGER_ATTACHED == host_logger) {
            return FALSE;
        }
    }
    return TRUE;
}

/**
 * @brief   Trigger UART Read if uart logger is enabled
 * @param   none
 * @return  none
 */
static void fw_logger_chk_trigger_uart_read(void)
{
    if (TRUE != fw_logger_over_uart_enabled()) {
        return;
    }
    xTaskNotify(xRCLICommandConsoleTask, (1 << RCLI_SIGNAL_LOGGER_MSG), eSetBits);
}

/*-------------------------------------------------------------------------
 * Externalized Function Definitions
 * ----------------------------------------------------------------------*/
/**
 * @brief   initialize diag_framework
 * @return  none
 */
void fw_logger_init(void)
{
    if (logger_semaphore_handle == NULL) {
        nt_osal_semaphore_create_binary(logger_semaphore_handle);
    }
#ifdef FEATURE_FPCI
    fpci_evt_cb_reg((ps_evt_cb_t)&host_log_flush_cb, PWR_EVT_WMAC_PRE_SLEEP | PWR_EVT_PRE_IMPS_TRIGGER,
                    FERM_LOG_FLUSH_PRIO, NULL);
#endif  // FEATURE_FPCI
}
/**
 * @brief   Store logs in buffer - 3 parameters
 * @return  none
 */
void log_to_buffer_fixed(const char *event_key, debug_log_elem p1, debug_log_elem p2, debug_log_elem p3)
{
    log_to_buffer_variadic(event_key, 3, p1, p2, p3);
}

/**
 * @brief   Store logs in buffer - variadic
 * @return  none
 */

void log_to_buffer_variadic(const char *event_key, size_t n_args, ...)
{
    size_t pos;
    debug_log_elem log_event_key = (debug_log_elem)event_key;
    TickType_t wait_ticks = portMAX_DELAY;
    // initializing variadic parameter variable
    va_list argp;
    va_start(argp, n_args);

    // length cannot be more than buffer size, so cutoff till buffer size
    if (n_args > DEBUG_LOG_BUF_SIZE - 1) {
        n_args = DEBUG_LOG_BUF_SIZE - 1;
    }

    if (logger_semaphore_handle != NULL) {
        /* See if we can obtain the semaphore.  If the semaphore is not
        available wait portMAX_DELAY to see if it becomes free. */
        BaseType_t sem_ret = pdFALSE;
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        wait_ticks = (xTaskGetSchedulerState() == taskSCHEDULER_SUSPENDED) ? 0 : portMAX_DELAY;
        sem_ret = (NT_IS_ISR) ? xSemaphoreTakeFromISR(logger_semaphore_handle, &xHigherPriorityTaskWoken)
                              : xSemaphoreTake(logger_semaphore_handle, wait_ticks);
        if (pdTRUE == sem_ret) {
            pos = debugBufferPos;
            // Wrap-over
            if (pos >= DEBUG_LOG_BUF_SIZE) {
                pos = 0;
            }
            // Push the event key (format string's address).
            debugBuffer[pos] = log_event_key;

#ifdef LOGGER_SEQ_NUM
            // sequence number
            if (++pos >= DEBUG_LOG_BUF_SIZE) {
                pos = 0;
            }
            debugBuffer[pos] = logger_seq_num;
#endif  // LOGGER_SEQ_NUM

#ifdef LOGGER_METADATA
            // Function line
            if (++pos >= DEBUG_LOG_BUF_SIZE) {
                pos = 0;
            }
            debugBuffer[pos] = (uint32_t)((uint8_t)n_args | (uint8_t)xTaskGetCurrentTaskId() << 8);
#endif  // LOGGER_METADATA

#ifdef LOGGER_TIMESTAMP
            if (++pos >= DEBUG_LOG_BUF_SIZE) {
                pos = 0;
            }
            debugBuffer[pos] = qurt_timer_get_ticks();
#endif  // LOGGER_TIMESTAMP

            // Copy over variadic params
            while (n_args--) {
                if (++pos >= DEBUG_LOG_BUF_SIZE) {
                    pos = 0;
                }
                debugBuffer[pos] = va_arg(argp, debug_log_elem);
            }
            if (++pos >= DEBUG_LOG_BUF_SIZE) {
                pos = 0;
            }
            // increment sequence number for every iteration
            logger_seq_num++;
            debugBufferPos = pos;

            fw_logger_chk_inform_host(FERM_LOG_FLUSH_THR_PERC_IDLE);

            (NT_IS_ISR) ? xSemaphoreGiveFromISR(logger_semaphore_handle, &xHigherPriorityTaskWoken)
                        : xSemaphoreGive(logger_semaphore_handle);
            if (!(NT_IS_ISR) && wait_ticks) {
                fw_logger_chk_trigger_uart_read();
            }
        }
    }
}

/**
 * @brief   Log dynamic strings to the buffer
 * @return  none
 */

void log_dynamic_string_to_buffer(const char *event_key, char *string_ptr)
{
    TickType_t wait_ticks = portMAX_DELAY;
    uint8_t *metadata_addr;
    uint8_t iter = 0;
    char *last_char_dst;
    char *last_char_src;
    if (logger_semaphore_handle != NULL) {
        /* See if we can obtain the semaphore.  If the semaphore is not
        available wait portMAX_DELAY to see if it becomes free. */
        BaseType_t sem_ret = pdFALSE;
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        wait_ticks = (xTaskGetSchedulerState() == taskSCHEDULER_SUSPENDED) ? 0 : portMAX_DELAY;
        sem_ret = (NT_IS_ISR) ? xSemaphoreTakeFromISR(logger_semaphore_handle, &xHigherPriorityTaskWoken)
                              : xSemaphoreTake(logger_semaphore_handle, wait_ticks);
        if (pdTRUE == sem_ret) {  // Push the event key (aka format string's address)
            size_t pos = debugBufferPos;

            if (pos >= DEBUG_LOG_BUF_SIZE) {
                pos = 0;
            }
            debugBuffer[pos] = (debug_log_elem)event_key;

#ifdef LOGGER_SEQ_NUM
            if (++pos >= DEBUG_LOG_BUF_SIZE) {
                pos = 0;
            }
            debugBuffer[pos] = logger_seq_num;
#endif  // LOGGER_SEQ_NUM

#ifdef LOGGER_METADATA
            // line number
            if (++pos >= DEBUG_LOG_BUF_SIZE) {
                pos = 0;
            }
            debugBuffer[pos] = 0;
            metadata_addr = (uint8_t *)(&debugBuffer[pos]);
            *(metadata_addr) = (uint8_t)0;
            *(metadata_addr + 1) = (uint8_t)xTaskGetCurrentTaskId();
#endif  // LOGGER_METADATA

#ifdef LOGGER_TIMESTAMP
            // Timestamp
            if (++pos >= DEBUG_LOG_BUF_SIZE) {
                pos = 0;
            }
            debugBuffer[pos] = qurt_timer_get_ticks();
#endif  // LOGGER_TIMESTAMP

            volatile uint16_t word_index = 0;
            uint8_t word_count = 0;
            volatile uint32_t word_has_zero_byte = WORD_HAS_NULL_BYTE(string_ptr + word_index);
            // Do long word copy, if that does not contain null byte in it
            while (!word_has_zero_byte && word_index < DEBUG_LOG_BUF_SIZE - 1) {
                if (++pos >= DEBUG_LOG_BUF_SIZE) {
                    pos = 0;
                }
                debugBuffer[pos] = *((uint32_t *)(string_ptr) + word_index);
                word_index += 1;
                word_has_zero_byte = WORD_HAS_NULL_BYTE(string_ptr + word_index);
            }
            word_count = (uint8_t)word_index;
            // byte-by-byte copy for the last long word until a null byte
            // check if last byte is null, if so, do a long word copy
            if (++pos >= DEBUG_LOG_BUF_SIZE) {
                pos = 0;
            }
            // if first byte is null in the last word, do a byte copy
            debugBuffer[pos] = 0;
            last_char_dst = (char *)&debugBuffer[pos];
            last_char_src = string_ptr + word_index * 4;

            while (*(last_char_src + iter) != '\0') {
                *(last_char_dst + iter) = *(last_char_src + iter);
                ++iter;
            }

#ifdef LOGGER_METADATA
            word_count += (uint8_t)(iter ? 1 : 0);
            *(metadata_addr) += word_count;
#endif
            if (++pos >= DEBUG_LOG_BUF_SIZE) {
                pos = 0;
            }
            logger_seq_num++;

            /* Update global buffer position afomplete */
            debugBufferPos = pos;

            fw_logger_chk_inform_host(FERM_LOG_FLUSH_THR_PERC_IDLE);

            (NT_IS_ISR) ? xSemaphoreGiveFromISR(logger_semaphore_handle, &xHigherPriorityTaskWoken)
                        : xSemaphoreGive(logger_semaphore_handle);
            if (!(NT_IS_ISR) && wait_ticks) {
                fw_logger_chk_trigger_uart_read();
            }
        }
    }
}
/**
 * @brief   Log dynamic strings to the buffer, accepts string length as paameter
 * @return  none
 */

void log_dynamic_string_to_buffer_with_len(const char *event_key, char *string_ptr, uint16_t string_length)
{
    uint16_t byte_index = 0;
    uint8_t lastByteCount = string_length % 4;
    uint8_t n_words = string_length / 4 + lastByteCount;
    uint8_t i = 0;
    char *byte_iterator;
    TickType_t wait_ticks = portMAX_DELAY;

    // length cannot be more than buffer size, so cutoff till buffer size
    if (string_length > DEBUG_LOG_BUF_SIZE - 1) {
        string_length = DEBUG_LOG_BUF_SIZE - 1;
    }

    if (logger_semaphore_handle != NULL) {
        /* See if we can obtain the semaphore.  If the semaphore is not
        available wait portMAX_DELAY to see if it becomes free. */
        BaseType_t sem_ret = pdFALSE;
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        wait_ticks = (xTaskGetSchedulerState() == taskSCHEDULER_SUSPENDED) ? 0 : portMAX_DELAY;
        sem_ret = (NT_IS_ISR) ? xSemaphoreTakeFromISR(logger_semaphore_handle, &xHigherPriorityTaskWoken)
                              : xSemaphoreTake(logger_semaphore_handle, wait_ticks);
        if (pdTRUE == sem_ret) {
            size_t pos = debugBufferPos;

            if (pos >= DEBUG_LOG_BUF_SIZE) {
                pos = 0;
            }
            debugBuffer[pos] = (debug_log_elem)event_key;

#ifdef LOGGER_SEQ_NUM
            if (++pos >= DEBUG_LOG_BUF_SIZE) {
                pos = 0;
            }
            debugBuffer[pos] = logger_seq_num;
#endif  // LOGGER_SEQ_NUM

#ifdef LOGGER_METADATA
            // line number
            if (++pos >= DEBUG_LOG_BUF_SIZE) {
                pos = 0;
            }
            debugBuffer[pos] = (uint32_t)((uint8_t)n_words | (uint8_t)xTaskGetCurrentTaskId() << 8);
#endif  // LOGGER_METADATA

#ifdef LOGGER_TIMESTAMP
            // Timestamp
            if (++pos >= DEBUG_LOG_BUF_SIZE) {
                pos = 0;
            }
            debugBuffer[pos] = qurt_timer_get_ticks();
#endif  // LOGGER_TIMESTAMP

            // memcopy strings directly
            while ((byte_index + 1) * 4 <= string_length) {
                if (++pos >= DEBUG_LOG_BUF_SIZE) {
                    pos = 0;
                }
                debugBuffer[pos] = *((uint32_t *)(string_ptr) + byte_index);
                byte_index++;
            }
            // For 4 byte alignment
            if (lastByteCount) {
                byte_iterator = (char *)&debugBuffer[++pos];
                byte_index *= 4;
                if (pos >= DEBUG_LOG_BUF_SIZE) {
                    pos = 0;
                }
                // copy last byte

                while (i < lastByteCount) {  // byte_iterator[lastByteCount] = 0;
                    *(byte_iterator + i) = string_ptr[byte_index + i];
                    i++;
                }
                // zero out 4 - number of last bytes
                while (4 - lastByteCount) {
                    *(byte_iterator + lastByteCount) = 0;
                    lastByteCount++;
                }
            } else {
                if (++pos >= DEBUG_LOG_BUF_SIZE) {
                    pos = 0;
                }
                debugBuffer[pos] = 0;
            }
            if (++pos >= DEBUG_LOG_BUF_SIZE) {
                pos = 0;
            }
            logger_seq_num++;
            /* Update global buffer position after message write complete */
            debugBufferPos = pos;

            fw_logger_chk_inform_host(FERM_LOG_FLUSH_THR_PERC_IDLE);

            (NT_IS_ISR) ? xSemaphoreGiveFromISR(logger_semaphore_handle, &xHigherPriorityTaskWoken)
                        : xSemaphoreGive(logger_semaphore_handle);
            if (!(NT_IS_ISR) && wait_ticks) {
                fw_logger_chk_trigger_uart_read();
            }
        }
    }
}

/**
 * @brief   Log byte sized array elements to the buffer
 *          4 array elaments are packed in a debugBuffer element
 * @return  none
 */

void log_arr_to_buffer(const char *event_key, const uint8_t *ptr, uint16_t len)
{
    size_t pos;
    uint16_t str_len = len * 2;
    uint8_t padding = str_len % 4, i = 0;
    uint8_t n_words = str_len / 4 + padding;
    uint16_t iter = 0;
    char *byte_iterator;
    debug_log_elem log_event_key = (debug_log_elem)event_key;
    TickType_t wait_ticks = portMAX_DELAY;
    char *buff;
    uint16_t offset = 0;

    // length cannot be more than buffer size, so cutoff till buffer size
    if (len > DEBUG_LOG_BUF_SIZE - 1) {
        len = DEBUG_LOG_BUF_SIZE - 1;
    }

    if (logger_semaphore_handle != NULL) {
        /* See if we can obtain the semaphore.  If the semaphore is not
        available wait portMAX_DELAY to see if it becomes free. */
        BaseType_t sem_ret = pdFALSE;
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        wait_ticks = (xTaskGetSchedulerState() == taskSCHEDULER_SUSPENDED) ? 0 : portMAX_DELAY;
        sem_ret = (NT_IS_ISR) ? xSemaphoreTakeFromISR(logger_semaphore_handle, &xHigherPriorityTaskWoken)
                              : xSemaphoreTake(logger_semaphore_handle, wait_ticks);
        if (pdTRUE == sem_ret) {
            pos = debugBufferPos;
            // Wrap-over
            if (pos >= DEBUG_LOG_BUF_SIZE) {
                pos = 0;
            }
            // Push the event key (format string's address).
            debugBuffer[pos] = log_event_key;

#ifdef LOGGER_SEQ_NUM
            // sequence number
            if (++pos >= DEBUG_LOG_BUF_SIZE) {
                pos = 0;
            }
            debugBuffer[pos] = logger_seq_num;
#endif  // LOGGER_SEQ_NUM

#ifdef LOGGER_METADATA
            // Function line
            if (++pos >= DEBUG_LOG_BUF_SIZE) {
                pos = 0;
            }
            debugBuffer[pos] = (uint32_t)((uint8_t)n_words | (uint8_t)xTaskGetCurrentTaskId() << 8);
#endif  // LOGGER_METADATA

#ifdef LOGGER_TIMESTAMP
            if (++pos >= DEBUG_LOG_BUF_SIZE) {
                pos = 0;
            }
            debugBuffer[pos] = qurt_timer_get_ticks();
#endif  // LOGGER_TIMESTAMP

            // Copy over arr elems
            for (int i = 0; i < len / 2; i++) {
                if (++pos > DEBUG_LOG_BUF_SIZE) {
                    pos = 0;
                }

                buff = (char *)&debugBuffer[pos];
                offset = snprintf(buff, 3, "%02x", *(ptr + iter));
                ++iter;
                snprintf(buff + offset, 3, "%02x", *(ptr + iter));
                ++iter;
            }

            if (padding) {
                if (++pos >= DEBUG_LOG_BUF_SIZE) {
                    pos = 0;
                }

                byte_iterator = (char *)&debugBuffer[pos];
                offset = snprintf(byte_iterator, 3, "%02x", ptr[iter]);
                ++i;
                *(uint16_t *)(byte_iterator + offset) = 0;
                ++i;
            } else {
                if (++pos >= DEBUG_LOG_BUF_SIZE) {
                    pos = 0;
                }
                debugBuffer[pos] = 0;
            }
            // increment sequence number for every iteration
            logger_seq_num++;
            if (++pos >= DEBUG_LOG_BUF_SIZE) {
                pos = 0;
            }
            /* Update global buffer position after message write complete */
            debugBufferPos = pos;

            fw_logger_chk_inform_host(FERM_LOG_FLUSH_THR_PERC_IDLE);
            (NT_IS_ISR) ? xSemaphoreGiveFromISR(logger_semaphore_handle, &xHigherPriorityTaskWoken)
                        : xSemaphoreGive(logger_semaphore_handle);
            if (!(NT_IS_ISR) && wait_ticks) {
                fw_logger_chk_trigger_uart_read();
            }
        }
    }
}

/**
 * @brief   Print the logger variables
 * @return  none
 */
void fw_logger_print_status(void)
{
    NT_LOG_PRINT(WMI, ERR, "BufPos:%d, Max:%d, Seq:%d SpiHostReadPos:%d", debugBufferPos, DEBUG_LOG_BUF_SIZE,
                 logger_seq_num, g_SPI_host_read_pos);
}

/**
 * @brief   Clears out the buffer and resets the write index
 * @return  none
 */
void clear_debug_buffer(void)
{
    if (logger_semaphore_handle != NULL) {
        /* See if we can obtain the semaphore.  If the semaphore is not
        available wait portMAX_DELAY to see if it becomes free. */
        BaseType_t sem_ret = pdFALSE;
        TickType_t wait_ticks = portMAX_DELAY;
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        wait_ticks = (xTaskGetSchedulerState() == taskSCHEDULER_SUSPENDED) ? 0 : portMAX_DELAY;
        sem_ret = (NT_IS_ISR) ? xSemaphoreTakeFromISR(logger_semaphore_handle, &xHigherPriorityTaskWoken)
                              : xSemaphoreTake(logger_semaphore_handle, wait_ticks);
        if (pdTRUE == sem_ret) {
            size_t pos = debugBufferPos;

            memset((void *)&debugBuffer, 0, sizeof(debugBuffer[0]) * DEBUG_LOG_BUF_SIZE);
            pos = 0;
            logger_seq_num = 0;

            /* Update global buffer position  */
            debugBufferPos = pos;
            (NT_IS_ISR) ? xSemaphoreGiveFromISR(logger_semaphore_handle, &xHigherPriorityTaskWoken)
                        : xSemaphoreGive(logger_semaphore_handle);
        }
    }
}

/**
 * @brief   Store logs in buffer minimal
 * @return  none
 */
void log_to_buffer_minimal(const char *event_key, size_t n_args, ...)
{
    size_t pos;
    // char pbuff[60];
    debug_log_elem log_event_key = (debug_log_elem)event_key;
    // initializing variadic parameter variable
    va_list argp;
    va_start(argp, n_args);

    // length cannot be more than buffer size, so cutoff till buffer size
    if (n_args > DEBUG_LOG_BUF_SIZE - 1) {
        n_args = DEBUG_LOG_BUF_SIZE - 1;
    }
    pos = debugBufferPos;
    // Wrap-over
    if (pos >= DEBUG_LOG_BUF_SIZE) {
        pos = 0;
    }
    // Push the event key (format string's address).
    debugBuffer[pos] = log_event_key;

#ifdef LOGGER_SEQ_NUM
    // sequence number
    if (++pos >= DEBUG_LOG_BUF_SIZE) {
        pos = 0;
    }
    debugBuffer[pos] = logger_seq_num;
#endif  // LOGGER_SEQ_NUM

#ifdef LOGGER_METADATA
    // Function line
    if (++pos >= DEBUG_LOG_BUF_SIZE) {
        pos = 0;
    }
    debugBuffer[pos] = (uint32_t)((uint8_t)n_args | (uint8_t)xTaskGetCurrentTaskId() << 8);
#endif  // LOGGER_METADATA

#ifdef LOGGER_TIMESTAMP
    if (++pos >= DEBUG_LOG_BUF_SIZE) {
        pos = 0;
    }
    debugBuffer[pos] = 0;
#endif  // LOGGER_TIMESTAMP

    // Copy over variadic params
    while (n_args--) {
        if (++pos >= DEBUG_LOG_BUF_SIZE) {
            pos = 0;
        }
        debugBuffer[pos] = va_arg(argp, debug_log_elem);
    }
    if (++pos >= DEBUG_LOG_BUF_SIZE) {
        pos = 0;
    }
    // increment sequence number for every iteration
    logger_seq_num++;
    debugBufferPos = pos;

    fw_logger_chk_inform_host(FERM_LOG_FLUSH_THR_PERC_IDLE);
}
#ifdef FEATURE_FPCI
/**
 * @brief   To be called before sleep. So that the host flushes logs, with an
 *          F2A indication
 * @return  none
 */
void host_log_flush_cb(uint8_t evt, void *p_args)
{
    uint8_t sleep_cxt = evt;
    (void)p_args;
    // Log level to be changed to INFO before submitting
    NT_LOG_PRINT(COMMON, INFO, "Logger flush for sleep event %d read ptr %d & write ptr %d", evt, g_SPI_host_read_pos,
                 debugBufferPos);

    switch (sleep_cxt) {
        // BMPS or TWT
        case PWR_EVT_WMAC_PRE_SLEEP:
            fw_logger_chk_inform_host(FERM_LOG_FLUSH_THR_PERC_BMPS);
            break;
        // IMPS
        case PWR_EVT_PRE_IMPS_TRIGGER:
            fw_logger_chk_inform_host(0);
            break;
        default:
            break;
    }
}
#endif  // FEATURE_FPCI

/**
 * @brief   To set the host logger with desired value
 * @param   host_logger_state: indicates whether host logger is attached
 * @return  none
 */
void fw_logger_set_host_attached(host_logger_state input)
{
    if (FERM_UART_LOGGER_DEFAULT == uart_logger) {
        NT_LOG_PRINT(WMI, ERR, "HostLogger changed to:%d UART-RCLI logger<%d> may get effected", input, uart_logger);
    } else {
        NT_LOG_PRINT(WMI, ERR, "HostLogger changed to:%d UART-RCLI logger<%d> remains same", input, uart_logger);
    }

    host_logger = input;
}

/**
 * @brief   To set the uart logger with desired value
 * @param   uart_option: indicates whether uart logger is default/ForceOn/ForceOff
 * @return  none
 */
void fw_logger_over_uart_set(uint8_t uart_option)
{
    uart_logger = uart_option;
}
#endif
