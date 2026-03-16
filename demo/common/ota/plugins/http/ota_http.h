/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _OTA_HTTP_H_
#define _OTA_HTTP_H_

#include "qapi_httpc.h"
#include "qurt_mutex.h"
#include "qurt_signal.h"
#include "qurt_internal.h"

/* Preprocessor Definitions and Constants											                      */
/**********************************************************************************************************/
/*
 * Firmware Upgrade TFTP Status codes
 */
#define QAPI_FW_UPGRADE_ERR_HTTP_SESSION_ALREADY_START __QAPI_ERROR(QAPI_MOD_FWUP, 100)
#define QAPI_FW_UPGRADE_ERR_HTTP_SESSION_NOT_START     __QAPI_ERROR(QAPI_MOD_FWUP, 101)
#define QAPI_FW_UPGRADE_ERR_HTTP_URL_FORMAT            __QAPI_ERROR(QAPI_MOD_FWUP, 102)
#define QAPI_FW_UPGRADE_ERR_HTTP_NO_MEMORY             __QAPI_ERROR(QAPI_MOD_FWUP, 103)
#define QAPI_FW_UPGRADE_ERR_HTTP_GET                   __QAPI_ERROR(QAPI_MOD_FWUP, 104)
#define QAPI_FW_UPGRADE_ERR_HTTP_CONNECT_FAIL          __QAPI_ERROR(QAPI_MOD_FWUP, 105)
#define QAPI_FW_UPGRADE_ERR_HTTP_TIMEOUT_TIMER_FAILED  __QAPI_ERROR(QAPI_MOD_FWUP, 106)
#define QAPI_FW_UPGRADE_ERR_HTTP_STOP_FAIL             __QAPI_ERROR(QAPI_MOD_FWUP, 107)
#define QAPI_FW_UPGRADE_ERR_HTTP_START_FAIL            __QAPI_ERROR(QAPI_MOD_FWUP, 108)
#define QAPI_FW_UPGRADE_ERR_HTTP_START_NEW_SESS_FAIL   __QAPI_ERROR(QAPI_MOD_FWUP, 109)
#define QAPI_FW_UPGRADE_ERR_HTTP_RX_QUEUE_EMPTY        __QAPI_ERROR(QAPI_MOD_FWUP, 110)

/* HTTP misc */
#define HTTP_TIMEOUT           20000  // in milliseconds
#define HTTP_PAYLOAD_SIZE      512
#define HTTP_FILE_NAME_LENGTH  128
#define HTTPC_OTA_DEMO_MAX_NUM 1

#define HTTP_RX_BUF_READY_SIG_MASK 0x1
#define HTTP_RX_TIMEOUT_SIG_MASK   0x2

#define HTTP_RX_ALL_SIG_MASK (HTTP_RX_BUF_READY_SIG_MASK | HTTP_RX_TIMEOUT_SIG_MASK)

/**********************************************************************************************************/
/* Function Declarations                											                      */
/**********************************************************************************************************/
qapi_Status_t plugin_http_init(const char *interface_name, const char *url, void *init_param);
qapi_Status_t plugin_http_fin(void);
qapi_Status_t plugin_http_recv_data(uint8_t *buffer, uint32_t buf_len, uint32_t *ret_size, void *init_param);
qapi_Status_t plugin_http_abort(void);

qapi_Status_t plugin_http_resume(const char *interface_name, const char *url, uint32_t offset);
/**********************************************************************************************************/
/* Strcut Declarations                											                      */
/**********************************************************************************************************/
typedef struct {
    char *interface_name;
    char *url;
    char *cfg_file;
    uint32_t flags;
    uint32_t timeout_time;
    uint32_t process_state_cnt;
    uint64_t total_len;
} qat_fw_upgrade_params_t;

typedef struct HTTP_Queue_Node_s {
    uint8_t *buffer;
    uint32_t buffer_len;
    struct HTTP_Queue_Node_s *next;
} HTTP_Queue_Node_t;

typedef struct HTTP_Queue_s {
    HTTP_Queue_Node_t *front;
    HTTP_Queue_Node_t *rear;
} HTTP_Queue_t;

typedef enum {
    HTTP_OTA_STATUS_NOT_STARTED,
    HTTP_OTA_STATUS_RUNNING,
    HTTP_OTA_STATUS_RUNNING_WAITING_FOR_STOP,
    HTTP_OTA_STATUS_STOP,
} http_ota_status_t;

typedef struct {
    int32_t status;
    int32_t error_code;
    uint8_t getting_started;
    TaskHandle_t task_handle;
    int8_t retry_count;
    qapi_Net_HTTPc_handle_t client;
    uint32_t num;
    uint32_t total_len;
    char *url;
    qapi_Ssl_Config_t *sslCfg;
    qapi_Ssl_Cert_t *sslCert;
    HTTP_Queue_t *http_rx_queue;
    uint32_t http_timeout;
    uint16_t http_recv_count;
    int32_t resp_code;
    int32_t http_range_offset;
    int8_t flags;
    uint8_t *http_recv_temp_buf;
    uint16_t http_recv_temp_buf_offset;
} http_session_info_t;

#endif /*_OTA_HTTP_H_ */
