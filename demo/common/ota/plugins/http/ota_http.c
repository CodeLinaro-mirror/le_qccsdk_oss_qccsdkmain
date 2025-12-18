/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "qapi_types.h"
#include "qapi_status.h"
//#include "qapi_httpc.h"
#include "qapi_console.h"
#include "nt_timer.h"

//#include "qurt_internal.h"
//#include "qurt_mutex.h"
//#include "qurt_signal.h"
#include "qurt_internal.h"
#include "sockets.h"
#include "timer.h"
#include "qapi_firmware_upgrade.h"
#include "safeAPI.h"

#include "qat_api.h"
#include "qat_httpc_demo.h"

#include "httpc_demo.h"
#include "ota_http.h"

/**********************************************************************************************************/
/* Preprocessor Definitions and Constants											                      */
/**********************************************************************************************************/
struct ota_http_client_demo_s {
    qapi_Net_HTTPc_handle_t client;
    uint32_t num;
    uint32_t total_len;
    qapi_Ssl_Config_t *sslCfg;
    qapi_Ssl_Cert_t *sslCert;
} ota_http_client_demo[HTTPC_OTA_DEMO_MAX_NUM];

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

#define UNUSED(x) (void)(x)

#define OTA_HTTPC_PRINTF(...) printf(__VA_ARGS__)

#ifdef CONFIG_QAT_OTA_DEMO
#define OTA_CHUNK_SIZE 1024
extern void ota_http_client_timer(void);
#endif
/**********************************************************************************************************/
/* Type Declarations																                      */
/**********************************************************************************************************/

/**********************************************************************************************************/
/* Globals variables															                                  */
/**********************************************************************************************************/
http_session_info_t *ota_http_sess;

/**********************************************************************************************************/
/* External variables																                      */
/**********************************************************************************************************/

extern struct at_https_global_config g_https_cfg;

/**********************************************************************************************************/
/* External Functions																                      */
/**********************************************************************************************************/

extern qbool_t getpathURL(const char *url, char *pathURL);

/**********************************************************************************************************/
/* Internal Functions																                      */
/**********************************************************************************************************/
qapi_Status_t ota_httpc_get();
static void ota_http_fin();
qapi_Status_t ota_httpc_conn(const char *url);

// Function to free the memory of the queue
void ota_http_free_rcv_queue(HTTP_Queue_t *queue)
{
    HTTP_Queue_Node_t *current = queue->front;
    HTTP_Queue_Node_t *next;
    while (current != NULL) {
        next = current->next;

        if(current->buffer){
           free(current->buffer); 
        }
        
        free(current);
        current = next;
    }
    free(queue);
}

static void ota_http_fin()
{   
    if (ota_http_sess != NULL) {
        if (ota_http_sess->task_handle != NULL) {
            nt_osal_thread_delete(ota_http_sess->task_handle);
            ota_http_sess->task_handle = NULL;
        }
        if (ota_http_sess->url != NULL) {
            free(ota_http_sess->url);
            ota_http_sess->url = NULL;
        }
        if (ota_http_sess->http_recv_temp_buf != NULL) {
            free(ota_http_sess->http_recv_temp_buf);
            ota_http_sess->http_recv_temp_buf = NULL;
        }
        if(ota_http_sess->http_rx_queue){
            ota_http_free_rcv_queue(ota_http_sess->http_rx_queue);
        }
        free(ota_http_sess);
        ota_http_sess = NULL;
    }
}

qapi_Status_t ota_httpc_get_with_offset(uint32_t offset, uint32_t max_len) 
{
    qapi_Status_t ret;
    uint8_t header_index = 0;

    uint32_t remain = ota_http_sess->total_len - offset;
    uint32_t expected_len = (remain < max_len) ? remain : max_len;

    qapi_Net_HTTPc_Clear_Header(ota_http_sess->client);
    g_https_cfg.header_field[header_index].name  = malloc(QAT_HTTP_HEADER_NAME_LEN);
    g_https_cfg.header_field[header_index].value = malloc(QAT_HTTP_HEADER_VALUE_LEN);
    memset(g_https_cfg.header_field[header_index].name, 0, QAT_HTTP_HEADER_NAME_LEN);
    memset(g_https_cfg.header_field[header_index].value, 0, QAT_HTTP_HEADER_VALUE_LEN);
    strlcpy(g_https_cfg.header_field[header_index].name, "Range", QAT_HTTP_HEADER_NAME_LEN);
    snprintf(g_https_cfg.header_field[header_index].value, QAT_HTTP_HEADER_VALUE_LEN, "bytes=%u-%u", offset, offset + expected_len - 1);

    // add header field
    ret = at_httpc_addheaderfield(header_index);
    if (ret != QAPI_OK) {
        OTA_HTTPC_PRINTF("[HTTP] Failed to add Range header\r\n");
        free(g_https_cfg.header_field[header_index].name);
        free(g_https_cfg.header_field[header_index].value);
        return QAPI_FW_UPGRADE_ERR_HTTP_GET;
    }

    // send get request
    ret = ota_httpc_get();
    if (ret != QAPI_OK) {
        OTA_HTTPC_PRINTF("[HTTP] GET with offset failed\r\n");
        free(g_https_cfg.header_field[header_index].name);
        free(g_https_cfg.header_field[header_index].value);
        return QAPI_FW_UPGRADE_ERR_HTTP_GET;
    }

    free(g_https_cfg.header_field[header_index].name);
    free(g_https_cfg.header_field[header_index].value);
    return QAPI_OK;
}

/**********************************************************************************************************/
/*      															                                      */
/**********************************************************************************************************/
/*
 * OTA TFTP plugin receive data
 *    buffer:    received data buffer
 *   buf_len:    received data buffer size in bytes
 *  ret_size:    data size in buffer after receiving done
 */
qapi_Status_t plugin_http_recv_data(uint8_t *buffer, uint32_t buf_len, uint32_t *ret_size, void *init_param)
{
    uint8_t *receive_buf = buffer;
    qapi_Status_t ret;
    HTTP_Queue_Node_t *temp = NULL;
    uint32_t signal;
    uint32_t http_ota_recv_start_ms = 0;
    uint32_t http_ota_recv_curr_ms = 0;
    
    char buf[HTTP_STR_BUFFER_LENGTH] = {0};
    uint32_t offset = 0;

    if (buffer == NULL || buf_len == 0 || init_param == NULL) {
        return QAPI_FW_UPGRADE_ERR_INVALID_PARAM;
    }

    if (ota_http_sess == NULL || ota_http_sess->status == HTTP_OTA_STATUS_NOT_STARTED) {
        return QAPI_FW_UPGRADE_ERR_HTTP_SESSION_NOT_START;
    }

    if (ota_http_sess->error_code != 0) {
        return ((qapi_Status_t)ota_http_sess->error_code);
    }

    if (ret_size != NULL) {
        *ret_size = 0;
    }

    if (ota_http_sess->flags & QAPI_FW_UPGRADE_FLAG_RANGE_HEADER) {
        if (ota_http_sess->http_range_offset >= ota_http_sess->total_len) {
            return QAPI_OK;
        }
        ret = ota_httpc_get_with_offset(ota_http_sess->http_range_offset, OTA_CHUNK_SIZE);
        if (ret != QAPI_OK) {
            return ret;
        }
    } else {
        if (ota_http_sess->getting_started == 0) {
            // If it is the first request, a Range field can be added to the HTTP request header to specify the starting position
            ret = ota_httpc_get();
            if (ret != QAPI_OK) {
                return ret;
            }
            ota_http_sess->getting_started = 1;
        }
    }

    if (ota_http_sess->status == HTTP_OTA_STATUS_RUNNING) {
        http_ota_recv_start_ms = (uint32_t)hres_timer_curr_time_us() / 1000;

        while (ota_http_sess->http_rx_queue->front == NULL) {
            http_ota_recv_curr_ms = (uint32_t)hres_timer_curr_time_us() / 1000;
            if (http_ota_recv_curr_ms - http_ota_recv_start_ms > ota_http_sess->http_timeout) {
                if (ota_http_sess->flags & QAPI_FW_UPGRADE_FLAG_RANGE_HEADER) {
                    uint32_t remain = ota_http_sess->total_len - ota_http_sess->http_range_offset;
                    uint32_t expected_len = (remain < OTA_CHUNK_SIZE) ? remain : OTA_CHUNK_SIZE;
                    ota_http_sess->retry_count++;
                    if (ota_http_sess->retry_count == 1) {
                        offset +=
                            snprintf(buf + offset, HTTP_STR_BUFFER_LENGTH - offset, "+EVT:OTAFWUP_RETRY:It was timeout once and retry...");
                        QAT_Response_Str(QAT_RC_QUIET_NO_CR, buf);
                        ret = ota_httpc_get_with_offset(ota_http_sess->http_range_offset, expected_len);
                        if (ret != QAPI_OK) return ret;
                        http_ota_recv_start_ms = (uint32_t)hres_timer_curr_time_us() / 1000;
                    } else {
                        OTA_HTTPC_PRINTF("[HTTP] Second timeout, aborting.\r\n");
                        ota_http_sess->retry_count = 0;
                        return QAPI_FW_UPGRADE_ERR_HTTP_RX_QUEUE_EMPTY;
                    }
                } else {
                    return QAPI_FW_UPGRADE_ERR_HTTP_RX_QUEUE_EMPTY;
                }        
            }

            /*No MACRO for resp_code for http client now, just use magic number for now*/
            /*for image not found*/
            if (ota_http_sess->resp_code == 404) {
                return QAPI_FW_UPGRADE_ERR_IMAGE_NOT_FOUND;
            }

            qurt_thread_sleep(5);
        }

        if (ota_http_sess->http_rx_queue->front == NULL) {
            return QAPI_FW_UPGRADE_ERR_HTTP_RX_QUEUE_EMPTY;
        }

        temp = ota_http_sess->http_rx_queue->front;
        if (temp == NULL || temp->buffer == NULL || temp->buffer_len == 0) {
            return QAPI_ERROR;
        }

        // Check if the buffer is large enough to hold the data
        if (buf_len < temp->buffer_len) {
            return QAPI_ERROR;
        }

        *ret_size = temp->buffer_len;
        memscpy(buffer, temp->buffer_len, temp->buffer, temp->buffer_len);

        // ota_http_sess->http_rx_queue->front = ota_http_sess->http_rx_queue->front->next;
        ota_http_sess->http_rx_queue->front = temp->next;

        if (ota_http_sess->http_rx_queue->front == NULL) {
            ota_http_sess->http_rx_queue->rear = NULL;
        }
        free(temp->buffer);
        free(temp);

        ota_http_sess->http_range_offset += *ret_size;
        ota_http_sess->retry_count = 0;
    }

    return QAPI_OK;
}

void http_client_cb_ota(void *arg, int32_t state, void *http_resp)
{
    (void)arg;
    qapi_Net_HTTPc_Response_t *temp = (qapi_Net_HTTPc_Response_t *)http_resp;
    struct ota_http_client_demo_s *hc = (struct ota_http_client_demo_s *)arg;
    uint32_t *ptotal_len = NULL;
    uint32_t tmp_len;
    uint32_t contentlength = 0;

    if (arg) {
        ptotal_len = &hc->total_len;
    } else {
        OTA_HTTPC_PRINTF("HTTP Client Demo arg error %d\n", state);
        return;
    }

    qapi_Net_HTTPc_Clear_Header(hc->client);

    if (state >= QAPI_NET_HTTPC_RX_FINISHED) {
        int32_t resp_code = temp->resp_Code;

        /*No MACRO for resp_code for http client now, just use magic number for now*/
        /*for image not found*/
        if (resp_code == 404) {
            ota_http_sess->resp_code = resp_code;
            OTA_HTTPC_PRINTF("HTTP Client Demo No Image Found\n");
            return;
        }

        if (temp->length && temp->data) {
            if (!ota_http_sess || !ota_http_sess->http_recv_temp_buf) {
                OTA_HTTPC_PRINTF("HTTP Client Demo invalid session state\n");
                return;
            }

            if(ota_http_sess->http_recv_count == 0){
                memset(ota_http_sess->http_recv_temp_buf,0,OTA_CHUNK_SIZE);
                ota_http_sess->http_recv_temp_buf_offset = 0;
            }

            if (ota_http_sess->http_recv_temp_buf_offset + temp->length > OTA_CHUNK_SIZE) {
                OTA_HTTPC_PRINTF("HTTP Client Demo buffer overflow prevented\n");
                if (ota_http_sess->http_recv_temp_buf) {
                    free(ota_http_sess->http_recv_temp_buf);
                    ota_http_sess->http_recv_temp_buf = NULL;
                    ota_http_sess->http_recv_temp_buf_offset = 0;
                }
                return;
            }
            
            memcpy(ota_http_sess->http_recv_temp_buf+ota_http_sess->http_recv_temp_buf_offset, temp->data, temp->length);
            ota_http_sess->http_recv_temp_buf_offset += temp->length;
            ota_http_sess->http_recv_count++;

            *ptotal_len += temp->length;
            contentlength = temp->contentlength;
        }
        if(state == QAPI_NET_HTTPC_RX_FINISHED || !(ota_http_sess->flags & QAPI_FW_UPGRADE_FLAG_RANGE_HEADER)){
            ota_http_sess->http_recv_count = 0;
            HTTP_Queue_Node_t *node = NULL;
            node = (HTTP_Queue_Node_t *)malloc(sizeof(HTTP_Queue_Node_t));

            if (node == NULL) {
                OTA_HTTPC_PRINTF("HTTP Client Demo malloc node error %d\n", state);
                return;
            }
            memset(node, 0, sizeof(HTTP_Queue_Node_t));

            node->buffer = NULL;
            node->buffer = (uint8_t *)malloc(ota_http_sess->http_recv_temp_buf_offset);

            if (node->buffer == NULL) {
                OTA_HTTPC_PRINTF("HTTP Client Demo malloc buffer error %d\n", state);
                free(node);  // Free the node to avoid memory leak
                return;
            }

            memset(node->buffer, 0, ota_http_sess->http_recv_temp_buf_offset);

            memcpy(node->buffer, ota_http_sess->http_recv_temp_buf, ota_http_sess->http_recv_temp_buf_offset);

            node->buffer_len = ota_http_sess->http_recv_temp_buf_offset;
            node->next = NULL;

            if (ota_http_sess->http_rx_queue->rear == NULL) {
                ota_http_sess->http_rx_queue->front = ota_http_sess->http_rx_queue->rear = node;
            } else {
                ota_http_sess->http_rx_queue->rear->next = node;
                ota_http_sess->http_rx_queue->rear = node;
            }
        }
        else if (state == QAPI_NET_HTTPC_RX_TUNNEL_ESTABLISHED) {
            OTA_HTTPC_PRINTF("#### TUNNEL ESTABLISHED: received %d bytes ####\n", *ptotal_len);
            *ptotal_len = 0;
        } else if (state == QAPI_NET_HTTPC_RX_DATA_FROM_TUNNEL) {
            OTA_HTTPC_PRINTF("#### Received %d bytes from TUNNEL ####\n", *ptotal_len);
            *ptotal_len = 0;
        } else if (state == QAPI_NET_HTTPC_RX_TUNNEL_CLOSED) {
            OTA_HTTPC_PRINTF("!!!! TUNNEL CLOSED !!!!\n");
            *ptotal_len = 0;
        } else if (state == QAPI_NET_HTTPC_RX_CHUNK_CONTINUE) {
            OTA_HTTPC_PRINTF("!!!! CONTINUE RECV !!!!\n");
            *ptotal_len = 0;
        }
    } else {
        if (QAPI_NET_HTTPC_RX_ERROR_SERVER_CLOSED == state)
            OTA_HTTPC_PRINTF("HTTP Client server closed on client[%d].\n", hc->num);
        else
            OTA_HTTPC_PRINTF("HTTP Client Receive error: %d\nPlease input 'httpc disconnect %d'\n", state, hc->num);
        *ptotal_len = 0;
    }
    /*small delay to prevent lwip pool exhaustion*/
    qurt_thread_sleep(10);
}

qapi_Status_t ota_httpc_conn(const char *url)
{
    qapi_Status_t rlt = QAPI_OK;
    char host[HTTP_HOST_STR_BUFFER_LENGTH];

    // reset flag at_httpc_stop
    // at_rec_state = QAPI_NET_HTTPC_RX_MORE_DATA;

    // Construct connect Command
    // httpc conn <client_num> <origin_server or proxy> [<port>]
    uint32_t Parameter_Count = 0;
    QAPI_Console_Parameter_t Parameter_List[QAT_HTTPC_MAXIMUM_NUMBER_OF_PARAMETERS];
    Parameter_List[Parameter_Count].Integer_Is_Valid = false;
    Parameter_List[Parameter_Count].String_Value = "conn";
    Parameter_Count++;

    Parameter_List[Parameter_Count].Integer_Is_Valid = true;
    Parameter_List[Parameter_Count].Integer_Value = QAT_HTTPC_CLIENT_INDEX;
    Parameter_Count++;

    gethostURL(url, host);
    Parameter_List[Parameter_Count].Integer_Is_Valid = false;
    Parameter_List[Parameter_Count].String_Value = host;
    Parameter_Count++;
    printf("conn host:%s\r\n", host);

    Parameter_List[Parameter_Count].Integer_Is_Valid = true;
    if (isSecureSession(url)) {
        Parameter_List[Parameter_Count].Integer_Value = g_https_cfg.https_port;
    } else {
        Parameter_List[Parameter_Count].Integer_Value = g_https_cfg.http_port;
    }

    Parameter_Count++;

    rlt = httpc_command_handler(Parameter_Count, Parameter_List);

    return rlt;
}

qapi_Status_t ota_httpc_disconn(int32_t client_num)
{
    qapi_Status_t rlt = QAPI_OK;
    char host[HTTP_HOST_STR_BUFFER_LENGTH];

    // Construct connect Command
    // httpc disconnect <client_num>
    uint32_t Parameter_Count = 0;
    QAPI_Console_Parameter_t Parameter_List[QAT_HTTPC_MAXIMUM_NUMBER_OF_PARAMETERS];
    Parameter_List[Parameter_Count].Integer_Is_Valid = false;
    Parameter_List[Parameter_Count].String_Value = "disconnect";
    Parameter_Count++;

    Parameter_List[Parameter_Count].Integer_Is_Valid = true;
    Parameter_List[Parameter_Count].Integer_Value = client_num;
    Parameter_Count++;

    rlt = httpc_command_handler(Parameter_Count, Parameter_List);

    return rlt;
}

qapi_Status_t ota_httpc_stop()
{
    qapi_Status_t rlt = QAPI_OK;

    // Construct start Command
    // httpc stop
    uint32_t Parameter_Count = 1;
    QAPI_Console_Parameter_t Parameter_List[Parameter_Count];
    Parameter_List[0].Integer_Is_Valid = false;
    Parameter_List[0].String_Value = "stop";
    rlt = httpc_command_handler(Parameter_Count, Parameter_List);

    return rlt;
}

qapi_Status_t ota_httpc_start()
{
    qapi_Status_t rlt = QAPI_OK;

    // Construct start Command
    // httpc start
    uint32_t Parameter_Count = 1;
    QAPI_Console_Parameter_t Parameter_List[Parameter_Count];
    Parameter_List[0].Integer_Is_Valid = false;
    Parameter_List[0].String_Value = "start";
    rlt = httpc_command_handler(Parameter_Count, Parameter_List);

    return rlt;
}

qapi_Status_t ota_httpc_get()
{
    qapi_Status_t rlt = QAPI_OK;
    char path_url[HTTP_URL_STR_BUFFER_LENGTH] = {0};
    uint32_t Parameter_Count = 0;
    QAPI_Console_Parameter_t Parameter_List[QAT_HTTPC_MAXIMUM_NUMBER_OF_PARAMETERS];

    Parameter_List[0].Integer_Is_Valid = false;
    Parameter_List[0].String_Value = "get";
    Parameter_Count++;

    Parameter_List[Parameter_Count].Integer_Is_Valid = true;
    Parameter_List[Parameter_Count].Integer_Value = QAT_HTTPC_CLIENT_INDEX;
    Parameter_Count++;

    if (!getpathURL(ota_http_sess->url, path_url)) {
        rlt = QAPI_FW_UPGRADE_ERR_HTTP_URL_FORMAT;
        return rlt;
    }

    Parameter_List[Parameter_Count].Integer_Is_Valid = false;
    Parameter_List[Parameter_Count].String_Value = path_url;
    Parameter_Count++;

    rlt = httpc_command_handler(Parameter_Count, Parameter_List);
    if (rlt != QAPI_OK) {
        rlt = QAPI_FW_UPGRADE_ERR_HTTP_GET;
        return rlt;
    }
    return rlt;
}

qapi_Status_t ota_httpc_new_session(const char *url, int32_t timeout)
{
    qapi_Status_t rlt = QAPI_OK;

    // Construct new session Command
    // httpc new [-t <timeout_ms> -b <body_buffer_size> -h <header_buffer_size> -r <rx_buffer_size> -s -c <calist>]
    uint32_t Parameter_Count = 0;
    QAPI_Console_Parameter_t Parameter_List[QAT_HTTPC_MAXIMUM_NUMBER_OF_PARAMETERS];
    Parameter_List[Parameter_Count].Integer_Is_Valid = false;
    Parameter_List[Parameter_Count].String_Value = "new";
    Parameter_Count++;

    Parameter_List[Parameter_Count].String_Value = "-t";
    Parameter_Count++;
    Parameter_List[Parameter_Count].Integer_Is_Valid = true;
    Parameter_List[Parameter_Count].Integer_Value = timeout;
    Parameter_Count++;

    if (isSecureSession(url)) {
        int scheme = g_https_cfg.https_auth_type;
        if (scheme == AT_HTTPS_NOT_AUTH) {
            // nothing to do
        } else if (scheme == AT_HTTPS_SERVER_AUTH) {
            Parameter_List[Parameter_Count].String_Value = "-s";
            Parameter_Count++;
            Parameter_List[Parameter_Count].String_Value = "-a";
            Parameter_Count++;
            Parameter_List[Parameter_Count].String_Value = g_https_cfg.ca_file;
            Parameter_Count++;

        } else if (scheme == AT_HTTPS_CLIENT_AUTH) {
            Parameter_List[Parameter_Count].String_Value = "-s";
            Parameter_Count++;
            Parameter_List[Parameter_Count].String_Value = "-c";
            Parameter_Count++;
            Parameter_List[Parameter_Count].String_Value = g_https_cfg.cert_file;
            Parameter_Count++;
            Parameter_List[Parameter_Count].String_Value = "-k";
            Parameter_Count++;
            Parameter_List[Parameter_Count].String_Value = g_https_cfg.key_file;
            Parameter_Count++;

        } else if (scheme == AT_HTTPS_BOTH_AUTH) {
            Parameter_List[Parameter_Count].String_Value = "-s";
            Parameter_Count++;
            Parameter_List[Parameter_Count].String_Value = "-c";
            Parameter_Count++;
            Parameter_List[Parameter_Count].String_Value = g_https_cfg.cert_file;
            Parameter_Count++;
            Parameter_List[Parameter_Count].String_Value = "-k";
            Parameter_Count++;
            Parameter_List[Parameter_Count].String_Value = g_https_cfg.key_file;
            Parameter_Count++;
            Parameter_List[Parameter_Count].String_Value = "-a";
            Parameter_Count++;
            Parameter_List[Parameter_Count].String_Value = g_https_cfg.ca_file;
            Parameter_Count++;
        }
    }

    rlt = httpc_command_handler(Parameter_Count, Parameter_List);

    return rlt;
}

/*
 * OTA TFTP plugin Init
 * interface_name:    interface name, such as wlan1
 *            url:    parameters, format: <server>/<url>
 *      int param:    optional init parameters
 */
qapi_Status_t plugin_http_init(const char *interface_name, const char *url, void *init_param)
{
    qapi_Status_t ret;
    char path_url[HTTP_URL_STR_BUFFER_LENGTH] = {0};
    uint32_t Parameter_Count = 0;
    uint32_t http_timeout = 0;
    uint8_t http_connect_retry_count = 3;
    QAPI_Console_Parameter_t Parameter_List[QAT_HTTPC_MAXIMUM_NUMBER_OF_PARAMETERS];

    UNUSED(interface_name);

    if (ota_http_sess != NULL) {
        return QAPI_FW_UPGRADE_ERR_HTTP_SESSION_ALREADY_START;
    }

    ota_http_sess = malloc(sizeof(http_session_info_t));
    if (ota_http_sess == NULL) {
        return QAPI_FW_UPGRADE_ERR_HTTP_NO_MEMORY;
    }

    memset(ota_http_sess, 0, sizeof(http_session_info_t));

    if (init_param != NULL) {
        /*we will use init_param as timeout time of http*/
        ota_http_sess->http_timeout = ((qat_fw_upgrade_params_t *)init_param)->timeout_time;
        ota_http_sess->flags = ((qat_fw_upgrade_params_t *)init_param)->flags;
    } else {
        ota_http_sess->http_timeout = HTTP_TIMEOUT;
    }

    ota_http_sess->url = NULL;
    ota_http_sess->url = malloc(strlen(url) + 1);
    if (ota_http_sess->url == NULL) {
        ret = QAPI_FW_UPGRADE_ERR_HTTP_NO_MEMORY;
        goto http_init_end;
    }

    memset(ota_http_sess->url, 0, strlen(url) + 1);
    memcpy(ota_http_sess->url, url, strlen(url) + 1);

    ota_http_sess->http_recv_temp_buf = NULL;
    ota_http_sess->http_recv_temp_buf = malloc(OTA_CHUNK_SIZE);
    if (ota_http_sess->http_recv_temp_buf == NULL) {
        ret = QAPI_FW_UPGRADE_ERR_HTTP_NO_MEMORY;
        goto http_init_end;
    }
    memset(ota_http_sess->http_recv_temp_buf, 0, OTA_CHUNK_SIZE);
    ota_http_sess->http_recv_count = 0;

    ota_http_sess->http_rx_queue = malloc(sizeof(HTTP_Queue_t));
    if (ota_http_sess->http_rx_queue == NULL) {
        ret = QAPI_FW_UPGRADE_ERR_HTTP_NO_MEMORY;
        goto http_init_end;
    }
    ota_http_sess->http_rx_queue->front = ota_http_sess->http_rx_queue->rear = NULL;

    ota_http_sess->status = HTTP_OTA_STATUS_RUNNING;
    ota_http_sess->getting_started = 0;
    if (ota_http_sess->flags & QAPI_FW_UPGRADE_FLAG_RANGE_HEADER) {
        ota_http_sess->total_len = ((qat_fw_upgrade_params_t *)init_param)->total_len;
    }

    // httpc connect
    while (http_connect_retry_count--) {
        /*when something wrong happened during receiving, a disconnection would be needed to close the socket*/
        ota_httpc_disconn(1);

        ret = ota_httpc_stop();
        if (ret) {
            ret = QAPI_FW_UPGRADE_ERR_HTTP_STOP_FAIL;
            goto http_init_end;
        }

        ret = ota_httpc_start();
        if (ret) {
            ret = QAPI_FW_UPGRADE_ERR_HTTP_START_FAIL;
            goto http_init_end;
        }

        // httpc new session
        ret = ota_httpc_new_session(url, ota_http_sess->http_timeout * 2);
        if (ret) {
            ret = QAPI_FW_UPGRADE_ERR_HTTP_START_NEW_SESS_FAIL;
            goto http_init_end;
        }

        ret = ota_httpc_conn(url);
        if (!ret) {
            break;
        } else {
            OTA_HTTPC_PRINTF("HTTP connect failed, Try %d time !!!!\n", http_connect_retry_count);
            qurt_thread_sleep(10);
        }
    }

    if (ret) {
        ret = QAPI_FW_UPGRADE_ERR_HTTP_CONNECT_FAIL;
        OTA_HTTPC_PRINTF("HTTP connect failed!\n");
        goto http_init_end;
    }

    /*
    Parameter_List[0].Integer_Is_Valid =false;
    Parameter_List[0].String_Value = "get";
    Parameter_Count++;

    Parameter_List[Parameter_Count].Integer_Is_Valid = true;
    Parameter_List[Parameter_Count].Integer_Value = QAT_HTTPC_CLIENT_INDEX;
    Parameter_Count++;

    if(!getpathURL(url,path_url))
    {
        ret = QAPI_FW_UPGRADE_ERR_HTTP_URL_FORMAT;
        ota_http_sess->status = HTTP_OTA_STATUS_NOT_STARTED;
        goto http_init_end;
    }


    Parameter_List[Parameter_Count].Integer_Is_Valid =false;
    Parameter_List[Parameter_Count].String_Value = path_url;
    Parameter_Count++;

    //Parameter_List[Parameter_Count].Integer_Is_Valid =true;
    //Parameter_List[Parameter_Count].Integer_Value = QAT_HTTP_GET;
    //Parameter_Count++;

    ret = httpc_command_handler(Parameter_Count,Parameter_List);
    if(ret != QAPI_OK)
    {
        ret = QAPI_FW_UPGRADE_ERR_HTTP_GET;
        ota_http_sess->status = HTTP_OTA_STATUS_NOT_STARTED;
        goto http_init_end;
    }
    */
    return QAPI_OK;
http_init_end:
    if (ret != QAPI_OK) {
        ota_http_fin();
    }
    return ret;
}

qapi_Status_t plugin_http_fin(void)
{
    ota_httpc_stop();
    ota_http_fin();

    return QAPI_OK;
}

qapi_Status_t plugin_http_abort(void)
{
    plugin_http_fin();
    return QAPI_OK;
}

/* HTTP doesn't support resume. */
qapi_Status_t plugin_http_resume(const char *interface_name, const char *url, uint32_t offset)
{
    UNUSED(interface_name);
    UNUSED(url);
    UNUSED(offset);
    return QAPI_OK;
}