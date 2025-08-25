/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/* -- Includes -- */
#include "fwconfig_cmn.h"
#include "nt_flags.h"
#ifdef NT_FN_LOGGER
#include <string.h> /* For sets all of the bytes in the specified buffer
                        to the same value and copies a sequence of bytes
                         from another place to the buffer.*/
#include <stdio.h>
#include "nt_logger_api.h"
#include "nt_logger.h"
#include "uart.h" /* nt_dbg_print*/
#include "semphr.h"
#include "nt_osal.h"
#include <inttypes.h>
#include "wlan_dev.h"
//#include "lfs.h"
#include "nt_lfs.h"
#ifdef NT_HOSTED_SDK
#include "nt_hosted_app.h"
#endif
#include "nt_devcfg.h"
#ifdef SUPPORT_COEX
#include "coex_utils.h"
extern BTCOEX_UTIL_STRUCT *gpBtCoexUtilDev;
#endif

//#define NT_DEFAULT_VAL  2
//#define NT_HALPHY_DEFAULT_VAL 1
// extern unsigned int _logs_start_address;

#if NT_FN_CRTLOG_FS
static lfs_file_t crt_file;
#endif

/**`
 *  Global variables
 *
 */
#define MSGBUF_LEN 200
/*!< Block size */
static uint16_t block_size;

/*!< Log level and module id declared through cli*/
uint8_t set_loglvl[NT_MAX_MODULE_ID];

/* message length is control by the 44 characters. */

#define NT_MAX_MSG_LEN 38
/**
 * Global pointer array for all the buffers.
 */

log_ringdesc_t *logs_buffer_array[4] = {0};
// All the module names configured here. if its any new module added in nt_common.h.New module name should be add here.
char *module_name[NT_MAX_MODULE_ID] = {"HAL",
                                       "SME",
                                       "MLM",
                                       "DPM",
                                       "WFM",
                                       "WMI",
                                       "HALPHY",
                                       "CLI",
                                       "SYSTEM",
                                       "WIFI_APP",
                                       "COMMON",
                                       "SECURITY",
                                       "HAL_API",
                                       "COMMISIONING_APP",
                                       "ONBOARDING_APP",
                                       "WPS",
                                       "AWS",
                                       "COEX",
                                       "PDC",
                                       "RCLI",
                                       "HOSTED_APP",
                                       "TLS",
                                       "DEV_CFG",
                                       "SOCPM",
                                       "WPM",
                                       "RA",
                                       "ANI"};
/**
 *...Semaphore handle...
 */
SemaphoreHandle_t xSemaphore;

/**
 * @FUNCTION    : nt_log_cfg
 * @description : Initialize All the Buffers.Calling all the cfg functions.
 *  Initialize the block size, from the user or default value.
 *  create semaphore.
 * @Param      : Pointer variable size.changing of buffers into blocks.
 * @return     : NONE
 */

void nt_log_cfg(
    /* !@Block size configuration*/
    log_ringdesc_t *size)
{
    // allocate memory for log structure buffer
    log_ringdesc_t *log_mem_alloc = (log_ringdesc_t *)nt_osal_calloc(4, sizeof(log_ringdesc_t));  // 12kb
    if (log_mem_alloc == NULL) {
#ifdef NT_DEBUG
        NT_LOG_SYSTEM_INFO("ERROR: INFO LOG Couldn't allocate memory", 0, 0, 0);
#endif
    }
    logs_buffer_array[0] = log_mem_alloc;
    logs_buffer_array[1] = log_mem_alloc + 1;
    logs_buffer_array[2] = log_mem_alloc + 2;
    /* Critical log designate to Heap in cMEM build */

#if NT_FN_CRTLOG_FS
    // lfs file open
    int cr_err = lfs_file_open(&lfs_init, &crt_file, "Critical_log.txt", LFS_O_RDWR | LFS_O_CREAT);
    if (cr_err != 0) {
#ifdef NT_DEBUG
        NT_LOG_SYSTEM_INFO("Error opening Critical_log.txt\r\n", 0, 0, 0);
#endif
    }
    // lfs file close
    lfs_file_close(&lfs_init, &crt_file);
#ifdef NT_DEBUG
    NT_LOG_SYSTEM_INFO("Critical Log in FS\r\n", 0, 0, 0);
#endif
#else  // NT_FN_CRTLOG_FS
    logs_buffer_array[3] = log_mem_alloc + 3;

    WLAN_DBG0_PRINT("Critical Log in cMEM\r\n");

#endif  // NT_FN_CRTLOG_FS

    // Condition checking for block size
    if (size) {  // Memory initialize to buffer
        block_size = (uint32_t)(size - 1);
    } else {  // Default block size
        block_size = NT_LOG_MAX_LOG_SIZE - 1;
    }
    // Config index position
    for (uint8_t loglvl = 0; loglvl < NT_LOG_LVL_MAX_LOG_LEVEL; loglvl++) {
#if NT_FN_CRTLOG_FS
        if (loglvl == NT_LOG_LVL_CRIT) {
#if ((defined(NT_CMEM_BUILD)) || (!defined(NT_FN_LFS)))
            // Initialize logs buffer head index with '0'
            logs_buffer_array[loglvl]->head_index = 0;
            // Clear logs buffer.
            memset(logs_buffer_array[loglvl], 0x0, NT_LOG_MAX_BUFF_SIZE - 1);
#endif
        } else
#endif  // NT_FN_CRTLOG_FS
        {
            // Initialize logs buffer head index with '0'
            logs_buffer_array[loglvl]->head_index = 0;
            // Clear logs buffer.
            memset(logs_buffer_array[loglvl], 0x0, NT_LOG_MAX_BUFF_SIZE - 1);
        }
    }
    uint8_t id = 0;
    while (id < NT_MAX_MODULE_ID) {
        //	        set_loglvl[id] = NT_DEFAULT_VAL; //NT_DEV_DEFAULT_LOG
        set_loglvl[id] = *((uint8_t *)(nt_devcfg_get_config(NT_DEVCFG_DEFAULT_LOG_LVL)));

        id++;
    }
    // Create a mutex type Semaphore.
    xSemaphore = xSemaphoreCreateMutex();
}

#if NT_FN_FIXED_ARG_LOGGER

/**
 * @FUNCTION    :  nt_log_write
 * @Description :  Check the buffer size and log level and module id,pass the data to buffer.
 *  Check the log level is in with the range or not. And
 *  check the buffer size is greater then block size or not
 * @Param       :  log level(0 - 3):  like info,warn,err,critical.
 *                 moduleid        : (8 - bit) module id
                   p1,p2,p3        : user calling parameters.
 * @return      :  PASS/FAILURE/nt_err_loglvl/nt_err_buff_size.
 */

uint8_t nt_log_write(
    /*!@module id like SME,MLME,HAL.etc...*/
    uint8_t mod_id,
    /*!@ loglevel like info,warning.etc...*/
    uint8_t loglvl,
/*@ for file name*/
#if (NT_FN_FUNCTION_LINE_NUM_FLAG == 1)
    char *fn,
    /*@ for line number*/
    uint16_t ln,
#endif
    /*!@ data */
    const char *msg,
    /* user provided parameters */
    uint32_t p1, uint32_t p2, uint32_t p3

)
{
    uint8_t calc_msg_len;
    // Read length of the string
    calc_msg_len = (uint8_t)strlen(msg);
    // Read the xTaskGetTickCount.
    uint32_t xStartTime;
    xStartTime = qurt_timer_get_ticks();
    // check the loglevel. does not exceed the max value.
    LOG_LEVEL_CHECK(loglvl);
    // block size compare to buffer size. Control the block size does not exceed the buffer size.
    BUFFER_SIZE_CHECK(block_size);

    // filtering the module id and loglevel.
    if (loglvl >= set_loglvl[mod_id]) {
        nt_log_data_pass_to_buff(logs_buffer_array[loglvl], mod_id, loglvl,
#if (NT_FN_FUNCTION_LINE_NUM_FLAG == 1)
                                 fn, ln,
#endif
                                 msg, p1, p2, p3, calc_msg_len, xStartTime);

    } else {
        return NT_OK;
    }
    return NT_OK;
}
/**
 * @FUNCTION    :  nt_log_data_pass_to_buff
 * @Description :  Initialize all the structure members.And clear the respective block, Write data to buffer.
                   Based on module id and log level, We are fill the buffer. Each buffer we are splitting
                   to blocks. Each block consist of time stamp and module id and log level and task name function
                   name and function line number and message and p1,p2,p3 parameters.
 * @Param       :  log level(0 - 3): like info,warn,err,critical.
 *                 moduleid        : 8 - bit module id
 *                 message         : String parameter pointer
                   p1,p2,p3        : User calling parameters..
                   log_buffer      : Buffer,like info,warn,err,critical.
                   calc_msg_len    : It indicates message length.
                   xStartTime      : Timer stamp
 * @return     :   PASS/FAILURE.
 */

uint8_t nt_log_data_pass_to_buff(
    /*@ by using buffers store the logs,like info,warning,err,etc... store the data*/
    log_ringdesc_t *log_buffer, uint8_t mod_id, uint8_t loglvl,
#if (NT_FN_FUNCTION_LINE_NUM_FLAG == 1)
    char *fn, uint16_t ln,
#endif
    const char *msg, uint32_t p1, uint32_t p2, uint32_t p3, uint8_t calc_msg_len, TickType_t xStartTime

)
{
    char pbuf[MSGBUF_LEN];
    uint32_t index;
    // Read the Current task Name.
    char *taskName = NULL;
    log_blockdesc_t log_msg;
#if NT_FN_CRTLOG_FS
    uint32_t total_buff_size = sizeof(logs_buffer_array[0]->buffer);
    // Divide the buffer into blocks.
    uint32_t total_blocks = total_buff_size / block_size;

    static uint32_t log_count = (NT_LOG_MAX_BUFF_SIZE - 1) / (NT_LOG_MAX_LOG_SIZE - 1);
    static uint32_t log_index = 0;
#endif  // NT_FN_CRTLOG_FS

    taskName = qurt_thread_get_name(NULL);
    // Call the semaphore
    if (xSemaphore != NULL) {
        /* See if we can obtain the semaphore.  If the semaphore is not
        available wait 10 ticks to see if it becomes free. */
        if (xSemaphoreTake(xSemaphore, (TickType_t)10) == pdTRUE) {
            // Clear the structure memory.
            memset(&log_msg, 0x0, sizeof(log_msg));
            // remain size - excluding message length, all members allocated bytes decreased by block size.
            uint8_t remain_size =
                (uint8_t)(block_size - (sizeof(log_msg.module_id) - sizeof(log_msg.loglvl)
#if (NT_FN_FUNCTION_LINE_NUM_FLAG == 1)
                                        - sizeof(log_msg.func_name) - sizeof(log_msg.ln)
#endif
                                        - sizeof(log_msg.p1) - sizeof(log_msg.p2) - sizeof(log_msg.p3)));
            if (remain_size > NT_MAX_MSG_LEN) {
                remain_size = NT_MAX_MSG_LEN;
            }
            // Initialize the all the structure members.
            log_msg.time_stamp = xStartTime;
            memscpy(log_msg.taskName, NT_LOG_MAX_TASK_NAME_LENGTH, taskName, NT_LOG_MAX_TASK_NAME_LENGTH);
#if (NT_FN_FUNCTION_LINE_NUM_FLAG == 1)
            memscpy(log_msg.func_name, NT_LOG_MAX_FUNCTION_NAME_LENGTH, fn, NT_LOG_MAX_FUNCTION_NAME_LENGTH);
            log_msg.ln = ln;
#endif
            log_msg.loglvl = loglvl;
            log_msg.module_id = mod_id;
            log_msg.p1 = p1;
            log_msg.p2 = p2;
            log_msg.p3 = p3;
            // Check the condition, string length and remaining size. Control the message in within the block.
            if (calc_msg_len > remain_size) {
                memscpy(log_msg.msg, NT_LOG_MAX_MSG_LENGTH, msg, remain_size);
            } else {
                memscpy(log_msg.msg, NT_LOG_MAX_MSG_LENGTH, msg, calc_msg_len);
            }
            snprintf(pbuf, sizeof(pbuf),
                     "%08u %d %d %s"
#if (NT_FN_FUNCTION_LINE_NUM_FLAG == 1)
                     "(%s:%u)"
#endif
                     ":%s 0x%08x(%d) 0x%08x(%d) 0x%08x(%d)\r\n",
                     (unsigned int)log_msg.time_stamp, (uint8_t)log_msg.module_id, (uint8_t)log_msg.loglvl,
                     log_msg.taskName,
#if (NT_FN_FUNCTION_LINE_NUM_FLAG == 1)
                     log_msg.func_name, (uint16_t)ln,
#endif
                     log_msg.msg, (unsigned int)log_msg.p1, (int)log_msg.p1, (unsigned int)log_msg.p2, (int)log_msg.p2,
                     (unsigned int)log_msg.p3, (int)log_msg.p3);
            nt_dbg_print(pbuf);

            // Continuously monitor buffer overflow.
            index = nt_calc_for_num_blocks(log_buffer);
            // erase block size.
            memset(&log_buffer->buffer[index], 0x0, block_size);

            /* Critical log File system integration */
#if NT_FN_CRTLOG_FS
            if (log_msg.loglvl == NT_LOG_LVL_CRIT) {
                int cr_err = lfs_file_open(&lfs_init, &crt_file, "Critical_log.txt", LFS_O_RDWR);

                if (cr_err != 0) {
#ifdef NT_DEBUG
                    NT_LOG_SYSTEM_INFO("Error opening\r\n", 0, 0, 0);
#endif
                    lfs_file_close(&lfs_init, &crt_file);
                    return (uint8_t)cr_err;
                } else {
                    uint32_t filesize = nt_find_filesize("Critical_log.txt");
                    int err_seek;
                    if (filesize >= (NT_LOG_MAX_BUFF_SIZE - 1)) {
                        if (log_count == 0)
                            log_count = total_blocks;

                        log_index = (total_blocks - log_count);
                        log_count--;

                        err_seek = lfs_file_seek(&lfs_init, &crt_file, (long unsigned int)(log_index * block_size),
                                                 LFS_SEEK_SET);
                    } else {
                        err_seek = lfs_file_seek(&lfs_init, &crt_file, 0, LFS_SEEK_END);
                    }

                    (void)err_seek;
#ifdef NT_DEBUG
                    // NT_LOG_SYSTEM_INFO("Seek:", err_seek,0,0);
#endif
                    int wr_err = lfs_file_write(&lfs_init, &crt_file, &log_msg, block_size);
                    (void)wr_err;
                }
                lfs_file_close(&lfs_init, &crt_file);

            } else
#endif  // NT_FN_CRTLOG_FS
            {
                // total data copied to expected buffer.like info,error,etc..
                memscpy(&log_buffer->buffer[index], (NT_LOG_MAX_BUFF_SIZE - 1), &log_msg, block_size);
            }

            /* Critical log File system integration */

            // we have finished accessing the shared resource. Release the semaphore.
            xSemaphoreGive(xSemaphore);
        }
    }
    return NT_OK;
}

#else

/**
 * @FUNCTION    :  nt_log_write
 * @Description :  Check the buffer size and log level and module id,pass the data to buffer.
 *  Check the log level is in with the range or not. And
 *  check the buffer size is greater then block size or not
 * @Param       :  log level(0 - 3):  like info,warn,err,critical.
 *                 moduleid        : (8 - bit) module id
                   msg             : string format message
 * @return      :  PASS/FAILURE/nt_err_loglvl/nt_err_buff_size.
 */

uint8_t nt_log_write(
    /*!@module id like SME,MLME,HAL.etc...*/
    uint8_t mod_id,
    /*!@ loglevel like info,warning.etc...*/
    uint8_t loglvl,
/*@ for file name*/
#if (NT_FN_FUNCTION_LINE_NUM_FLAG == 1)
    char *fn,
    /*@ for line number*/
    uint16_t ln,
#endif
    /*!@ data */
    const char *fmt, ...

)
{
    // Read the Current task Name.
    char *taskName = NULL;
    char pbuf[MSGBUF_LEN];
    uint32_t index;
    va_list argp;
#if NT_FN_CRTLOG_FS
    uint32_t total_buff_size = sizeof(logs_buffer_array[0]->buffer);
    // Divide the buffer into blocks.
    uint32_t total_blocks = total_buff_size / block_size;

    static uint32_t log_count = (NT_LOG_MAX_BUFF_SIZE - 1) / (NT_LOG_MAX_LOG_SIZE - 1);
    static uint32_t log_index = 0;
#endif  // NT_FN_CRTLOG_FS
    taskName = qurt_thread_get_name(NULL);
    // Read the xTaskGetTickCount.
    uint32_t xStartTime;
    xStartTime = qurt_timer_get_ticks();
    log_blockdesc_t log_msg;
    // check the loglevel. does not exceed the max value.
    LOG_LEVEL_CHECK(loglvl);
    // block size compare to buffer size. Control the block size does not exceed the buffer size.
    BUFFER_SIZE_CHECK(block_size);

    // filtering the module id and loglevel.
    if (loglvl >= set_loglvl[mod_id]) {
        // Call the semaphore
        if (xSemaphore != NULL) {
            /* See if we can obtain the semaphore.  If the semaphore is not
            available wait 10 ticks to see if it becomes free. */
            if (xSemaphoreTake(xSemaphore, (TickType_t)10) == pdTRUE) {
                int bytes = 0;
                // Clear the structure memory.
                memset(&log_msg, 0x0, sizeof(log_msg));
                // Initialize the all the structure members.
                log_msg.time_stamp = xStartTime;
                memscpy(log_msg.taskName, NT_LOG_MAX_TASK_NAME_LENGTH, taskName, NT_LOG_MAX_TASK_NAME_LENGTH);
#if (NT_FN_FUNCTION_LINE_NUM_FLAG == 1)
                memscpy(log_msg.func_name, NT_LOG_MAX_FUNCTION_NAME_LENGTH, fn, NT_LOG_MAX_FUNCTION_NAME_LENGTH);
                log_msg.ln = ln;
#endif
                log_msg.loglvl = loglvl;
                log_msg.module_id = mod_id;

                va_start(argp, fmt);
                bytes = vsnprintf(pbuf, sizeof(pbuf), fmt, argp);

                if ((bytes >= NT_LOG_MAX_MSG_LENGTH)) {
                    bytes = (NT_LOG_MAX_MSG_LENGTH - 2);
                }
                memscpy(log_msg.msg, NT_LOG_MAX_MSG_LENGTH, &pbuf, bytes);

                snprintf(pbuf, sizeof(pbuf),
                         "%08u %d %d %s: "
#if (NT_FN_FUNCTION_LINE_NUM_FLAG == 1)
                         "(%s:%u)"
#endif
                         "%s",
                         (unsigned int)xStartTime, mod_id, (uint8_t)loglvl, (char *)taskName
#if (NT_FN_FUNCTION_LINE_NUM_FLAG == 1)
                         ,
                         fn, (uint16_t)ln

#endif
                         ,
                         log_msg.msg);

                nt_dbg_print(pbuf);
                va_end(argp);
                snprintf(pbuf, sizeof(pbuf), "\r\n");
                nt_dbg_print(pbuf);
                // Continuously monitor buffer overflow.
                index = nt_calc_for_num_blocks(logs_buffer_array[loglvl]);
                // erase block size.
                memset(&logs_buffer_array[loglvl]->buffer[index], 0x0, block_size);
                // total data copied to expected buffer.like info,error,etc..
                /* Critical log File system integration */
#if NT_FN_CRTLOG_FS
                if (log_msg.loglvl == NT_LOG_LVL_CRIT) {
                    int cr_err = lfs_file_open(&lfs_init, &crt_file, "Critical_log.txt", LFS_O_RDWR);

                    if (cr_err != 0) {
#ifdef NT_DEBUG
                        NT_LOG_SYSTEM_INFO("Error opening\r\n", 0, 0, 0);
#endif
                        lfs_file_close(&lfs_init, &crt_file);
                        return cr_err;
                    } else {
                        uint32_t filesize = nt_find_filesize("Critical_log.txt");
                        int err_seek;
                        if (filesize >= (NT_LOG_MAX_BUFF_SIZE - 1)) {
                            if (log_count == 0)
                                log_count = total_blocks;

                            log_index = (total_blocks - log_count);
                            log_count--;

                            err_seek = lfs_file_seek(&lfs_init, &crt_file, (log_index * block_size), LFS_SEEK_SET);
                        } else {
                            err_seek = lfs_file_seek(&lfs_init, &crt_file, 0, LFS_SEEK_END);
                        }

                        (void)err_seek;
#ifdef NT_DEBUG
                        // NT_LOG_SYSTEM_INFO("Seek:", err_seek,0,0);
#endif
                        int wr_err = lfs_file_write(&lfs_init, &crt_file, &log_msg, block_size);
                        (void)wr_err;
                    }
                    lfs_file_close(&lfs_init, &crt_file);
                } else
#endif  // NT_FN_CRTLOG_FS
                {
                    // total data copied to expected buffer.like info,error,etc..
                    memscpy(&logs_buffer_array[loglvl]->buffer[index], (NT_LOG_MAX_BUFF_SIZE - 1), &log_msg,
                            block_size);
                }
                xSemaphoreGive(xSemaphore);
            }
        }
    }

    return NT_OK;
}
#endif  // NT_FN_FIXED_ARG_LOGGER
/**
 *   @FUNCTION    : nt_log_read
 *   @Description : Read Data from all the buffers
 *   @Param       : Moduleid for filtering of logs
 *   @return      : NONE
 */

void nt_log_read(uint8_t moduleid)
{
    char pbuf[MSGBUF_LEN];
    uint32_t a;
    uint8_t row = 0, j;
#if NT_FN_CRTLOG_FS
    // file size it tells the total file system allocated memory.
    uint32_t filesize = nt_find_filesize("Critical_log.txt");
    // dividing the blocks from file system allocated memory.
    uint32_t crtlog_count = filesize / block_size;
    int crlo_err, crlr_err;
    lfs_file_t crtical_log;
#endif

    uint16_t idx = 0;
    // Call the semaphore.
    if (xSemaphore != NULL) {
        /* See if we can obtain the semaphore.  If the semaphore is not
        available wait 10 ticks to see if it becomes free. */
        if (xSemaphoreTake(xSemaphore, (TickType_t)10) == pdTRUE) {
            uint8_t loglvl;
            // Read the total blocks.
            uint32_t total_buff_size = sizeof(logs_buffer_array[0]->buffer);
            uint8_t total_size = (uint8_t)(((NT_LOG_MAX_BUFF_SIZE - 1) / block_size) * 4);
            // Divide the buffer into blocks.
            uint32_t total_blocks = total_buff_size / block_size;
            uint32_t *time_stamp_ptr = (uint32_t *)nt_osal_calloc(
                total_blocks, ((sizeof(uint32_t)) * (sizeof(uint32_t)) * (sizeof(uint16_t))));
            if (time_stamp_ptr == NULL) {
                xSemaphoreGive(xSemaphore);
                return;
            }
#if NT_FN_CRTLOG_FS
            // allocating the memory for reading the data from file systems/
            uint8_t *loc_log = (uint8_t *)nt_osal_calloc(1, filesize);
            crlo_err = lfs_file_open(&lfs_init, &crtical_log, "Critical_log.txt", LFS_O_RDWR);
            if (crlo_err != 0) {
#ifdef NT_DEBUG
                NT_LOG_SYSTEM_INFO("Opening Error\r\n", 0, 0, 0);
#endif
                // little file system is close, when the file is not create.
                lfs_file_close(&lfs_init, &crtical_log);
                xSemaphoreGive(xSemaphore);
                return;
            } else {
                // file system reading..
                crlr_err = lfs_file_read(&lfs_init, &crtical_log, loc_log, filesize);
                (void)crlr_err;
#ifdef NT_DEBUG
                NT_LOG_SYSTEM_INFO("Read data:\r\n", (uint32_t)crlr_err, 0, 0);
#endif
                lfs_file_close(&lfs_init, &crtical_log);
            }
#endif

            // uint8_t total_size = (((NT_LOG_MAX_BUFF_SIZE -1) / block_size) * 4);
            for (loglvl = 0; loglvl < NT_LOG_LVL_MAX_LOG_LEVEL; loglvl++) {
                for (uint8_t tail_index = 0; tail_index < total_blocks; tail_index++) {
                    log_blockdesc_t loc_stru_msg;

                    idx = 0;
                    idx = (uint16_t)(tail_index * block_size);

#if NT_FN_CRTLOG_FS
                    if (loglvl != NT_LOG_LVL_CRIT)
#endif  // NT_FN_CRTLOG_FS
                    {
                        // copy the logs data from RAM.
                        memscpy(&loc_stru_msg, sizeof(log_blockdesc_t),
                                (log_blockdesc_t *)&(logs_buffer_array[loglvl]->buffer[idx]), block_size);
                    }
#if NT_FN_CRTLOG_FS
                    else if (tail_index < crtlog_count) {
                        // copy the logger data from file system.
                        memscpy(&loc_stru_msg, sizeof(log_blockdesc_t), &loc_log[idx], block_size);
                    } else {
                        break;
                    }
#endif  // NT_FN_CRTLOG_FS

                    // Control unknown data.
                    if ((loc_stru_msg.module_id == 0) || (loc_stru_msg.loglvl >= NT_LOG_LVL_MAX_LOG_LEVEL)) {
                        break;
                    }
                    // Read the time stamp from the buffer.
                    time_stamp_ptr[row] = (unsigned int)(loc_stru_msg.time_stamp);
                    row++;
                }
            }
            for (uint8_t row = 0; row < total_size; row++) {
                for (j = (uint8_t)(row + 1); j < total_size; ++j) {
                    if (time_stamp_ptr[row] > time_stamp_ptr[j]) {
                        a = time_stamp_ptr[row];
                        time_stamp_ptr[row] = time_stamp_ptr[j];
                        time_stamp_ptr[j] = a;
                    }
                }
            }
            for (uint8_t row = 0; row < total_size; row++) {
                for (loglvl = 0; loglvl < NT_LOG_LVL_MAX_LOG_LEVEL; loglvl++) {
                    for (uint8_t tail_index = 0; tail_index < total_blocks; tail_index++) {
                        log_blockdesc_t loc_stru_msg;
                        idx = 0;
                        idx = (uint16_t)(tail_index * block_size);

#if NT_FN_CRTLOG_FS
                        if (loglvl != NT_LOG_LVL_CRIT)
#endif  // NT_FN_CRTLOG_FS
                        {
                            // copy the logs data form RAM
                            memscpy(&loc_stru_msg, sizeof(log_blockdesc_t),
                                    (log_blockdesc_t *)&(logs_buffer_array[loglvl]->buffer[idx]), block_size);
                        }
#if NT_FN_CRTLOG_FS
                        else if (tail_index < crtlog_count) {
                            // copy the logs data from file system.
                            memscpy(&loc_stru_msg, sizeof(log_blockdesc_t), &loc_log[idx], block_size);
                        } else {
                            break;
                        }
#endif  // NT_FN_CRTLOG_FS
        // Control unknown data.
                        if ((loc_stru_msg.module_id == 0) || (loc_stru_msg.loglvl >= NT_LOG_LVL_MAX_LOG_LEVEL)) {
                            break;
                        } else if ((moduleid == 0) && (time_stamp_ptr[row] == loc_stru_msg.time_stamp)) {
                            // Read the user required parameter values (p1,p2,p3).

                            snprintf(pbuf, sizeof(pbuf),
                                     "%08u %d %d %s "
#if (NT_FN_FUNCTION_LINE_NUM_FLAG == 1)
                                     "(%s:%u) "
#endif
                                     "%s "
#if NT_FN_FIXED_ARG_LOGGER
                                     "0x%08x(%d) 0x%08x(%d) 0x%08x(%d)"
#endif  // NT_FN_FIXED_ARG_LOGGER
                                     "\r\n",
                                     (unsigned int)loc_stru_msg.time_stamp, (uint8_t)loc_stru_msg.module_id,
                                     (uint8_t)loc_stru_msg.loglvl, (char *)loc_stru_msg.taskName,
#if (NT_FN_FUNCTION_LINE_NUM_FLAG == 1)
                                     loc_stru_msg.func_name, (uint16_t)loc_stru_msg.ln,
#endif
                                     loc_stru_msg.msg
#if NT_FN_FIXED_ARG_LOGGER
                                     ,
                                     (unsigned int)loc_stru_msg.p1, (int)loc_stru_msg.p1, (unsigned int)loc_stru_msg.p2,
                                     (int)loc_stru_msg.p2, (unsigned int)loc_stru_msg.p3, (int)loc_stru_msg.p3
#endif  // NT_FN_FIXED_ARG_LOGGER
                            );
                            nt_dbg_print(pbuf);
                            memset(pbuf, 0x0, MSGBUF_LEN);
                            break;
                        }
                        if ((moduleid == loc_stru_msg.module_id) && (time_stamp_ptr[row] == loc_stru_msg.time_stamp)) {
                            // Read the time stamp from the buffer.
                            snprintf(pbuf, sizeof(pbuf),
                                     "%08u %d %d %s "
#if (NT_FN_FUNCTION_LINE_NUM_FLAG == 1)
                                     "(%s:%u) "
#endif
                                     "%s "
#if NT_FN_FIXED_ARG_LOGGER
                                     "0x%08x(%d) 0x%08x(%d) 0x%08x(%d)"
#endif  // NT_FN_FIXED_ARG_LOGGER
                                     "\r\n",
                                     (unsigned int)loc_stru_msg.time_stamp, (uint8_t)loc_stru_msg.module_id,
                                     (uint8_t)loc_stru_msg.loglvl, (char *)loc_stru_msg.taskName,
#if (NT_FN_FUNCTION_LINE_NUM_FLAG == 1)
                                     loc_stru_msg.func_name, (uint16_t)loc_stru_msg.ln,
#endif
                                     loc_stru_msg.msg
#if NT_FN_FIXED_ARG_LOGGER
                                     ,
                                     (unsigned int)loc_stru_msg.p1, (int)loc_stru_msg.p1, (unsigned int)loc_stru_msg.p2,
                                     (int)loc_stru_msg.p2, (unsigned int)loc_stru_msg.p3, (int)loc_stru_msg.p3
#endif  // NT_FN_FIXED_ARG_LOGGER
                            );
                            nt_dbg_print(pbuf);
                            memset(pbuf, 0x0, MSGBUF_LEN);
                            break;
                        }
                    }
                }
            }

#if NT_FN_CRTLOG_FS
            nt_osal_free_memory(loc_log);
#endif
            memset(time_stamp_ptr, 0x00,
                   ((total_blocks) * ((((sizeof(uint32_t)) * (sizeof(uint32_t)))) * (sizeof(uint16_t)))));
            nt_osal_free_memory(time_stamp_ptr);
            time_stamp_ptr = NULL;
            // We have finished accessing the shared resource. Release the semaphore.
            xSemaphoreGive(xSemaphore);
        }
    }
}

/**
 *   @FUNCTION    : nt_log_lvl_read
 *   @Description : Through CLI command, we will get the respective log related log information from respective module
 * id.
 *   @Param       : moduleid.
 *   				log level.
 *   @return      : Null
 */
void nt_log_lvl_read(uint8_t moduleid, uint8_t loglevel)
{
    char pbuf[MSGBUF_LEN];
    uint32_t a;
    uint8_t row = 0, j;
#if NT_FN_CRTLOG_FS
    // file size it tells the total file system allocated memory.
    uint32_t filesize = nt_find_filesize("Critical_log.txt");
    // dividing the blocks from file system allocated memory.
    uint32_t crtlog_count = filesize / block_size;
    int crlo_err, crlr_err;
    lfs_file_t crtical_log;
#endif  // NT_FN_CRTLOG_FS

    uint16_t idx = 0;
    // Call the semaphore.
    if (xSemaphore != NULL) {
        /* See if we can obtain the semaphore.  If the semaphore is not
        available wait 10 ticks to see if it becomes free. */
        if (xSemaphoreTake(xSemaphore, (TickType_t)10) == pdTRUE) {
            uint8_t loglvl;
            // Read the total blocks.
            uint32_t total_buff_size = sizeof(logs_buffer_array[0]->buffer);
            uint8_t total_size = (uint8_t)(((NT_LOG_MAX_BUFF_SIZE - 1) / block_size) * 4);
            // Divide the buffer into blocks.
            uint32_t total_blocks = total_buff_size / block_size;
            uint32_t *time_stamp_ptr = (uint32_t *)nt_osal_calloc(
                total_blocks, ((sizeof(uint32_t)) * (sizeof(uint32_t)) * (sizeof(uint16_t))));
            if (time_stamp_ptr == NULL) {
                xSemaphoreGive(xSemaphore);
                return;
            }
#if NT_FN_CRTLOG_FS
            // allocating the memory for reading the data from file systems/
            uint8_t *loc_log = (uint8_t *)nt_osal_calloc(1, filesize);
            crlo_err = lfs_file_open(&lfs_init, &crtical_log, "Critical_log.txt", LFS_O_RDWR);
            if (crlo_err != 0) {
#ifdef NT_DEBUG
                NT_LOG_SYSTEM_INFO("Opening Error\r\n", 0, 0, 0);
#endif
                // little file system is close, when the file is not create.
                lfs_file_close(&lfs_init, &crtical_log);
                xSemaphoreGive(xSemaphore);
                return;
            } else {
                // file system reading..
                crlr_err = lfs_file_read(&lfs_init, &crtical_log, loc_log, filesize);
                (void)crlr_err;
#ifdef NT_DEBUG
                NT_LOG_SYSTEM_INFO("Read data:\r\n", (uint32_t)crlr_err, 0, 0);
#endif
                lfs_file_close(&lfs_init, &crtical_log);
            }
#endif  // NT_FN_CRTLOG_FS

            // uint8_t total_size = (((NT_LOG_MAX_BUFF_SIZE -1) / block_size) * 4);
            for (loglvl = 0; loglvl < NT_LOG_LVL_MAX_LOG_LEVEL; loglvl++) {
                for (uint8_t tail_index = 0; tail_index < total_blocks; tail_index++) {
                    log_blockdesc_t loc_stru_msg;

                    idx = 0;
                    idx = (uint16_t)(tail_index * block_size);

#if NT_FN_CRTLOG_FS
                    if (loglvl != NT_LOG_LVL_CRIT)
#endif  // NT_FN_CRTLOG_FS
                    {
                        // copy the logs data from RAM.
                        memscpy(&loc_stru_msg, sizeof(log_blockdesc_t),
                                (log_blockdesc_t *)&(logs_buffer_array[loglvl]->buffer[idx]), block_size);
                    }
#if NT_FN_CRTLOG_FS
                    else if (tail_index < crtlog_count) {
                        // copy the logger data from file system.
                        memscpy(&loc_stru_msg, sizeof(log_blockdesc_t), &loc_log[idx], block_size);
                    } else {
                        break;
                    }
#endif  // NT_FN_CRTLOG_FS

                    // Control unknown data.
                    if ((loc_stru_msg.module_id == 0) || (loc_stru_msg.loglvl >= NT_LOG_LVL_MAX_LOG_LEVEL)) {
                        break;
                    }
                    // Read the time stamp from the buffer.
                    time_stamp_ptr[row] = (unsigned int)(loc_stru_msg.time_stamp);
                    row++;
                }
            }
            for (uint8_t row = 0; row < total_size; row++) {
                for (j = (uint8_t)(row + 1); j < total_size; ++j) {
                    if (time_stamp_ptr[row] > time_stamp_ptr[j]) {
                        a = time_stamp_ptr[row];
                        time_stamp_ptr[row] = time_stamp_ptr[j];
                        time_stamp_ptr[j] = a;
                    }
                }
            }
            for (uint8_t row = 0; row < total_size; row++) {
                for (loglvl = 0; loglvl < NT_LOG_LVL_MAX_LOG_LEVEL; loglvl++) {
                    for (uint8_t tail_index = 0; tail_index < total_blocks; tail_index++) {
                        log_blockdesc_t loc_stru_msg;
                        idx = 0;
                        idx = (uint16_t)(tail_index * block_size);

#if NT_FN_CRTLOG_FS
                        if (loglvl != NT_LOG_LVL_CRIT)
#endif  // NT_FN_CRTLOG_FS
                        {
                            // copy the logs data form RAM
                            memscpy(&loc_stru_msg, sizeof(log_blockdesc_t),
                                    (log_blockdesc_t *)&(logs_buffer_array[loglvl]->buffer[idx]), block_size);
                        }
#if NT_FN_CRTLOG_FS
                        else if (tail_index < crtlog_count) {
                            // copy the logs data from file system.
                            memscpy(&loc_stru_msg, sizeof(log_blockdesc_t), &loc_log[idx], block_size);
                        } else {
                            break;
                        }
#endif  // NT_FN_CRTLOG_FS
        // Control unknown data.
                        if ((loc_stru_msg.module_id == 0) || (loc_stru_msg.loglvl >= NT_LOG_LVL_MAX_LOG_LEVEL)) {
                            break;
                        }
                        if (((moduleid == loc_stru_msg.module_id) && (loc_stru_msg.loglvl == loglevel)) &&
                            (time_stamp_ptr[row] == loc_stru_msg.time_stamp)) {
                            // Read the time stamp from the buffer.
                            snprintf(pbuf, sizeof(pbuf),
                                     "%08u %d %d %s "
#if (NT_FN_FUNCTION_LINE_NUM_FLAG == 1)
                                     "(%s:%u) "
#endif
                                     "%s "
#if NT_FN_FIXED_ARG_LOGGER
                                     "0x%08x(%d) 0x%08x(%d) 0x%08x(%d)"
#endif  // NT_FN_FIXED_ARG_LOGGER
                                     "\r\n",
                                     (unsigned int)loc_stru_msg.time_stamp, (uint8_t)loc_stru_msg.module_id,
                                     (uint8_t)loc_stru_msg.loglvl, (char *)loc_stru_msg.taskName,
#if (NT_FN_FUNCTION_LINE_NUM_FLAG == 1)
                                     loc_stru_msg.func_name, (uint16_t)loc_stru_msg.ln,
#endif
                                     loc_stru_msg.msg
#if NT_FN_FIXED_ARG_LOGGER
                                     ,
                                     (unsigned int)loc_stru_msg.p1, (int)loc_stru_msg.p1, (unsigned int)loc_stru_msg.p2,
                                     (int)loc_stru_msg.p2, (unsigned int)loc_stru_msg.p3, (int)loc_stru_msg.p3
#endif  // NT_FN_FIXED_ARG_LOGGER
                            );
                            nt_dbg_print(pbuf);

                            memset(pbuf, 0x0, MSGBUF_LEN);
                            break;
                        }
                    }
                }
            }

#if NT_FN_CRTLOG_FS
            nt_osal_free_memory(loc_log);
#endif  // NT_FN_CRTLOG_FS
            memset(time_stamp_ptr, 0x00,
                   ((total_blocks) * ((((sizeof(uint32_t)) * (sizeof(uint32_t)))) * (sizeof(uint16_t)))));
            nt_osal_free_memory(time_stamp_ptr);
            time_stamp_ptr = NULL;
            // We have finished accessing the shared resource. Release the semaphore.
            xSemaphoreGive(xSemaphore);
        }
    }
}

/**
 *  @FUNCTION    : nt_log_lvl_set
 *  @Description : It indicates the, which log level is set.
 *    The log level and module id set through cli command.
 *  @Param       : log level(0 - 3)
 *   			   Moduleid - moduleid set through cli
 *  @return      : PASS/FAILURE
 */

uint8_t nt_log_lvl_set(uint8_t moduleid, int8_t loglvl)
{
    if (((loglvl >= 0) && (loglvl <= 3)) && (((moduleid > 0) && (moduleid < NT_MAX_MODULE_ID)) || (moduleid == 255))) {
        if (moduleid == 255) {
            uint8_t id = 1;
            while (id < NT_MAX_MODULE_ID) {
                set_loglvl[id] = (uint8_t)loglvl;
                id++;
            }

        } else {
            set_loglvl[moduleid] = (uint8_t)loglvl;
        }
        return NT_OK;
    } else {
        return NT_EFAIL;
    }
}
/**
 *  @FUNCTION    : nt_log_lvl_get
 *  @Description : Get the which log level is enabled.
 *  @Param       : NONE
 *  @return      : PASS/FAILURE
 */
uint8_t nt_log_lvl_get(void)
{
    char pbuf[MSGBUF_LEN];
    uint8_t module_id;
    snprintf(pbuf, sizeof(pbuf), "\r\n%s %s %s\r\n", "ModuleId", "ModuleName", "LogLvL");
    nt_dbg_print(pbuf);
    for (module_id = 1; module_id < NT_MAX_MODULE_ID; module_id++) {
        snprintf(pbuf, sizeof(pbuf), "[%d]\t %s\t    [%d]\r\n", module_id, module_name[module_id - 1],
                 set_loglvl[module_id]);
        nt_dbg_print(pbuf);
    }
    // Get Current enabled log level.
    return NT_OK;
}

/**
 *  @FUNCTION    : nt_log_lvl_clr
 *  @Description : Set the all log level is 0.
 *  @Param       : NONE
 *  @return      : PASS/FAILURE
 */
uint8_t nt_log_lvl_clear(void)
{
    char pbuf[MSGBUF_LEN];
    uint8_t moduleid;
    snprintf(pbuf, sizeof(pbuf), "\r\n%s %s %s\r\n", "ModuleId", "ModuleName", "LogLvL");
    nt_dbg_print(pbuf);
    for (moduleid = 1; moduleid < NT_MAX_MODULE_ID; moduleid++) {
        set_loglvl[moduleid] = 0;
        snprintf(pbuf, sizeof(pbuf), "[%d]\t %s\t    [%d]\r\n", moduleid, module_name[moduleid - 1],
                 set_loglvl[moduleid]);
        nt_dbg_print(pbuf);
    }
    // Get Current enabled log level.
    return NT_OK;
}

/**
 *   @FUNCTION    :    nt_log_deinit
 *   @Description :    De initialize the initialized memory
 *   @Param       :    NONE
 *   @return      :    PASS
 */

uint8_t nt_log_deinit(void)
{
    for (uint8_t loglvl = 0; loglvl < NT_LOG_LVL_MAX_LOG_LEVEL; loglvl++) {
#if NT_FN_CRTLOG_FS
        if (loglvl == NT_LOG_LVL_CRIT) {
            lfs_remove(&lfs_init, "Critical_log.txt");
        } else
#endif
        {
            // Clear logs buffer.
            memset(logs_buffer_array[loglvl], 0x0, NT_LOG_MAX_BUFF_SIZE - 1);
        }
    }
    return NT_OK;
}

/**
 *   @FUNCTION    : nt_calc_for_num_blocks
 *   @Description : Check the buffer is fill or not. buffer is fill, restart head position.
 *   @Param       : log_buffer : buffer
 *   @return      : Index
 */

uint32_t nt_calc_for_num_blocks(log_ringdesc_t *log_buffer

)
{
    uint32_t index;
    uint8_t total_blocks;
    total_blocks = (uint8_t)((NT_LOG_MAX_BUFF_SIZE - 1) / block_size);
    // Check the buffer overflow. Buffer is fill restart the head index. and control the blocks.
    if (log_buffer->head_index >= total_blocks) {
        log_buffer->head_index = 0;
    }
    index = (uint32_t)((log_buffer->head_index++) * (block_size));
    return index;
}

/**
 * @FUNCTION    :  nt_log_printf
 * @Description :  Check the  log level and module id,pass the data to buffer.
 *  Check the log level is in with the range or not. pass the variable arguments list.
 * @Param       :  log level(0 - 3):  like info,warn,err,critical.
 *                 moduleid        : (8 - bit) module id
 *                 fmt             : message
                   num             : number of variable arguments.
                   ...             : variable arguments list.
 * @return      :  PASS/FAILURE/nt_err_loglvl/nt_err_buff_size.
 */

#define PRINT_TASK_NAME 0

uint8_t nt_log_printf(uint8_t mod_id, uint8_t loglvl,
#if (NT_FN_FUNCTION_LINE_NUM_FLAG == 1)
                      char *func_name,
                      /*@ for line number*/
                      uint16_t ln,
#endif
                      const char *fmt, uint8_t num, ...)
{
    // Read the xTaskGetTickCount.
    uint32_t xStartTime;
    char pbuf[MSGBUF_LEN];
    xStartTime = qurt_timer_get_ticks();
#if PRINT_TASK_NAME
    // Read the Current task Name.
    char *taskName = NULL;
    taskName = qurt_thread_get_name(NULL);
#endif
    // check the loglevel. does not exceed the max value.
    LOG_LEVEL_CHECK(loglvl);
    if (loglvl >= set_loglvl[mod_id]) {
        va_list argp;

        snprintf(pbuf, sizeof(pbuf),
                 "%08u %d %d"
#if PRINT_TASK_NAME
                 " %s"
#endif
                 ": "
#if (NT_FN_FUNCTION_LINE_NUM_FLAG == 1)
                 "(%s:%u) "
#endif
                 ,
                 (unsigned int)xStartTime, mod_id, (uint8_t)loglvl
#if PRINT_TASK_NAME
                 ,
                 (char *)taskName
#endif
#if (NT_FN_FUNCTION_LINE_NUM_FLAG == 1)
                 ,
                 func_name, (uint16_t)ln
#endif
        );
        nt_dbg_print(pbuf);
        va_start(argp, num);
        vsnprintf(pbuf, sizeof(pbuf), fmt, argp);
        nt_dbg_print(pbuf);
        snprintf(pbuf, sizeof(pbuf), "\r\n");
        va_end(argp);

        nt_dbg_print(pbuf);
#ifdef NT_HOSTED_SDK
        extern uint8_t resp_log;
        if (resp_log == 1) {
            va_start(argp, num);
            vsnprintf(pbuf, sizeof(pbuf), fmt, argp);
            nt_at_spi_send_of_len(pbuf, strlen(pbuf));
            snprintf(pbuf, sizeof(pbuf), "\r\n");
            va_end(argp);
            nt_at_spi_send_of_len(pbuf, strlen(pbuf));
        }

#endif
    }
    return NT_OK;
}

uint8_t nt_log_array_printf(uint8_t mod_id, uint8_t loglvl,
#if (NT_FN_FUNCTION_LINE_NUM_FLAG == 1)
                            char *func_name,
                            /*@ for line number*/
                            uint16_t ln,
#endif
                            const char *s, const uint8_t *ptr, const uint16_t len)
{
    // Read the xTaskGetTickCount.
    uint32_t xStartTime;
    char pbuf[MSGBUF_LEN];
    xStartTime = qurt_timer_get_ticks();

    // Read the Current task Name.
    char *taskName = NULL;
    taskName = qurt_thread_get_name(NULL);

    // check the loglevel. does not exceed the max value.
    LOG_LEVEL_CHECK(loglvl);

    if (loglvl >= set_loglvl[mod_id]) {
        snprintf(pbuf, sizeof(pbuf),
                 "%08u %d %d %s: "
#if (NT_FN_FUNCTION_LINE_NUM_FLAG == 1)
                 "(%s:%u) "
#endif
                 ,
                 (unsigned int)xStartTime, mod_id, (uint8_t)loglvl, (char *)taskName
#if (NT_FN_FUNCTION_LINE_NUM_FLAG == 1)
                 ,
                 func_name, (uint16_t)ln
#endif
        );

        nt_dbg_print(pbuf);
        snprintf(pbuf, sizeof(pbuf), " %s \r\n", s);
        WLAN_DEBUG(pbuf);

        if (ptr == NULL)
            return FALSE;

        uint8_t i;
        for (i = 0; i < len; i++) {
            snprintf(pbuf, sizeof(pbuf), "0x%02x ", (uint8_t) * (ptr + i));
            nt_dbg_print(pbuf);

            if ((i + 1) % 16 == 0) {
                nt_dbg_print("\r\n");
            }
        }
        nt_dbg_print("\r\n");
    }
    return NT_OK;
}

#endif
