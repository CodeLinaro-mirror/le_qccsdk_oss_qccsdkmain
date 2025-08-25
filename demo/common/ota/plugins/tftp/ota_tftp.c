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
#include "qurt_internal.h"
#include "qurt_mutex.h"
#include "qurt_signal.h"
#include "sockets.h"
#include "timer.h"
#include "qapi_firmware_upgrade.h"
#include "ota_tftp.h"
#include "safeAPI.h"

/**********************************************************************************************************/
/* Preprocessor Definitions and Constants											                      */
/**********************************************************************************************************/
/* TFTP misc */
#define TFTP_TIMEOUT           5000  // in milliseconds
#define TFTP_RECV_TIMEOUT      1     // in seconds
#define TFTP_SERVER_PORT       69
#define TFTP_CLIENT_PORT       5260
#define TFTP_PAYLOAD_SIZE      512
#define TFTP_HEADER_SIZE       4
#define TFTP_RX_SIZE           (TFTP_PAYLOAD_SIZE + TFTP_HEADER_SIZE)
#define TFTP_MAX_RETRY_COUNT   3
#define TFTP_RCV_BUF_NUM       4
#define TFTP_RCV_BUF_SIZE      (TFTP_RCV_BUF_NUM * TFTP_PAYLOAD_SIZE)
#define TFTP_SEND_RCV_BUF_SIZE 1024
#define TFTP_FILE_NAME_LENGTH  128

/* TFTP operations. */
#define TFTP_OP_RRQ   1
#define TFTP_OP_WRQ   2
#define TFTP_OP_DATA  3
#define TFTP_OP_ACK   4
#define TFTP_OP_ERROR 5
#define TFTP_OP_OACK  6

/* TFTP states */
#define TFTP_ST_RRQ       1
#define TFTP_ST_DATA      2
#define TFTP_ST_TOO_LARGE 3
#define TFTP_ST_BAD_MAGIC 4
#define TFTP_ST_ERROR     5
#define TFTP_ST_OACK      6

/*  TFTP ERROR States */
#define TFTP_FILE_NOT_FOUND      1
#define TFTP_ACCESS_VIOLATION    2
#define TFTP_DISK_FULL           3
#define TFTP_ILLEGAL_OPERATION   4
#define TFTP_UNKNOWN_TRANSFER_ID 5
#define TFTP_FILE_EXISTS         6
#define TFTP_NO_SUCH_USER        7

/* TFTP signals */
#define TFTP_RX_BUF_READY_SIG_MASK  0x1
#define TFTP_RX_DATA_READY_SIG_MASK 0x2
#define TFTP_RX_ERROR_SIG_MASK      0x4
#define TFTP_RX_FINISH_SIG_MASK     0x8
#define TFTP_RX_ALL_SIG_MASK        (TFTP_RX_DATA_READY_SIG_MASK | TFTP_RX_ERROR_SIG_MASK | TFTP_RX_FINISH_SIG_MASK)

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

#define UNUSED(x) (void)(x)

/**********************************************************************************************************/
/* Type Declarations																                      */
/**********************************************************************************************************/
typedef enum {
    OTA_STATUS_NOT_STARTED,
    OTA_STATUS_RUNNING,
    OTA_STATUS_RUNNING_WAITING_FOR_STOP,
    OTA_STATUS_STOP,
} tftp_ota_status_t;

typedef struct {
    int32_t status;
    int32_t error_code;
    int32_t tftp_state;
    int32_t pkt_seq;
    int32_t pkt_seq_last;
    char filename[TFTP_FILE_NAME_LENGTH];
    int32_t sock;
    struct sockaddr_storage foreign_addr;
    uint8_t *snd_rcv_buffer;
    uint8_t *rcv_buffer;
    uint32_t receive_data;
    uint32_t read_pointer;
    uint32_t write_pointer;
    uint32_t send_time;
    qurt_signal_t signal;
    TaskHandle_t task_handle;
    qurt_mutex_t mutex;
    int8_t retry_count;
} tftp_session_info_t;

/**********************************************************************************************************/
/* Globals																                                  */
/**********************************************************************************************************/
tftp_session_info_t *ota_tftp_sess;

/**********************************************************************************************************/
/* Internal Functions																                      */
/**********************************************************************************************************/
static void ota_tftp_fin()
{
    if (ota_tftp_sess != NULL) {
        if (ota_tftp_sess->sock >= 0) {
            closesocket(ota_tftp_sess->sock);
        }

        if (ota_tftp_sess->signal != 0) {
            qurt_signal_delete(&ota_tftp_sess->signal);
        }

        if (ota_tftp_sess->mutex != 0) {
            qurt_mutex_delete(&ota_tftp_sess->mutex);
        }

        if (ota_tftp_sess->task_handle != NULL) {
            nt_osal_thread_delete(ota_tftp_sess->task_handle);
        }

        if (ota_tftp_sess->rcv_buffer != NULL) {
            free(ota_tftp_sess->rcv_buffer);
            ota_tftp_sess->rcv_buffer = NULL;
        }
        if (ota_tftp_sess->snd_rcv_buffer != NULL) {
            free(ota_tftp_sess->snd_rcv_buffer);
            ota_tftp_sess->snd_rcv_buffer = NULL;
        }
        free(ota_tftp_sess);
        ota_tftp_sess = NULL;
    }
}

static int32_t ota_tftp_pkt_rrq(const char *file_name)
{
    int32_t rrq_len = 0;
    unsigned short *rrq;
    unsigned char *p;
    uint32_t tolen;

    rrq_len = 2 + strlen(file_name) + 1 + strlen("octet") + 1;
    memset(&ota_tftp_sess->snd_rcv_buffer[0], 0, rrq_len);
    rrq = (unsigned short *)&ota_tftp_sess->snd_rcv_buffer[0];
    *rrq++ = htons(TFTP_OP_RRQ);
    p = (unsigned char *)rrq;
    memscpy((void *)p, strlen(file_name), file_name, strlen(file_name));
    p += strlen(file_name);
    *p++ = '\0';
    memscpy((void *)p, strlen("octet"), "octet", strlen("octet"));
    p += 5;
    *p = '\0';

    if (ota_tftp_sess->foreign_addr.ss_family == AF_INET) {
        tolen = sizeof(struct sockaddr_in);
    } else {
#if LWIP_IPV6
        tolen = sizeof(struct sockaddr_in6);
#else
        return -1;
#endif
    }

    ota_tftp_sess->send_time = hres_timer_curr_time_ms();

    return sendto(ota_tftp_sess->sock, &ota_tftp_sess->snd_rcv_buffer[0], rrq_len, 0,
                  (struct sockaddr *)(&ota_tftp_sess->foreign_addr), tolen);
}

static int32_t ota_tftp_pkt_error(uint32_t error_code, char *error_message)
{
    int32_t err_len = 0;
    unsigned short *err;

    err_len = 4 + strlen(error_message) + 1; /*2 byte opcode ,2byte block number, ErrMsg + 1 */
    memset(&ota_tftp_sess->snd_rcv_buffer[0], 0, err_len);
    err = (unsigned short *)&ota_tftp_sess->snd_rcv_buffer[0];
    *err++ = htons(TFTP_OP_ERROR);
    *err++ = htons(error_code);
    memscpy((void *)err, strlen(error_message) + 1, error_message, strlen(error_message) + 1);

    return send(ota_tftp_sess->sock, &ota_tftp_sess->snd_rcv_buffer[0], err_len, 0);
}

static int32_t ota_tftp_pkt_ack(uint16_t block_seq)
{
    int32_t ack_len;
    unsigned short *ack;

    ack_len = 4; /*2 byte opcode ,2byte block number  */
    memset(&ota_tftp_sess->snd_rcv_buffer[0], 0, ack_len);
    ack = (unsigned short *)&ota_tftp_sess->snd_rcv_buffer[0];
    *ack++ = htons(TFTP_OP_ACK);
    *ack++ = htons(block_seq);

    ota_tftp_sess->send_time = hres_timer_curr_time_ms();

    return send(ota_tftp_sess->sock, &ota_tftp_sess->snd_rcv_buffer[0], ack_len, 0);
}

static void ota_tftp_pkt_parse(uint8_t *pkt_ptr, int32_t pkt_len, int32_t *pkt_seq, int32_t *pkt_state)
{
    int32_t len = pkt_len;
    unsigned short opcode, *s;
    unsigned char *pkt = pkt_ptr;

    if (len < 2) {
        return;
    }
    len -= 2;

    s = (unsigned short *)pkt;
    opcode = *s++;
    pkt = (unsigned char *)s;

    switch (ntohs(opcode)) {
        case TFTP_OP_RRQ:
        case TFTP_OP_WRQ:
        case TFTP_OP_ACK:
            break;
        case TFTP_OP_OACK:
            *pkt_state = TFTP_ST_OACK;
            break;
        case TFTP_OP_DATA:
            if (len < 2)
                return;
            *pkt_seq = ntohs(*(unsigned short *)pkt);
            if (*pkt_state == TFTP_ST_RRQ || *pkt_state == TFTP_ST_OACK) {
                *pkt_state = TFTP_ST_DATA;
            }
            break;
        case TFTP_OP_ERROR:
            *pkt_state = TFTP_ST_ERROR;
            break;
        default:
            break;
    }
}

static void ota_tftp_timeout()
{
    if (ota_tftp_sess == NULL || ota_tftp_sess->status != OTA_STATUS_RUNNING) {
        return;
    }
    /* timeout happens. send rrq again */
    if (ota_tftp_sess->tftp_state == TFTP_ST_RRQ) {
        if (ota_tftp_sess->retry_count < TFTP_MAX_RETRY_COUNT) {
            ota_tftp_sess->retry_count++;
            ota_tftp_pkt_rrq(ota_tftp_sess->filename);
        } else {
            /* error status */
            ota_tftp_sess->status = OTA_STATUS_STOP;
            ota_tftp_sess->error_code = QAPI_FW_UPGRADE_ERR_TFTP_SERVER_RESP_TIMEOUT;
        }
    } else if (ota_tftp_sess->tftp_state == TFTP_ST_DATA) {
        if (ota_tftp_sess->retry_count < TFTP_MAX_RETRY_COUNT) {
            ota_tftp_sess->retry_count++;
            ota_tftp_pkt_ack(ota_tftp_sess->pkt_seq);
        } else {
            /* error status */
            ota_tftp_sess->status = OTA_STATUS_STOP;
            ota_tftp_sess->error_code = QAPI_FW_UPGRADE_ERR_TFTP_SERVER_RESP_TIMEOUT;
        }
    }
}

static void ota_tftp_recv(void __attribute__((__unused__)) * pvParameters)
{
    fd_set sockset, master;
    int32_t conn_sock;
    struct timeval tv;
    int32_t received;
    struct sockaddr_storage from;
    uint32_t fromlen;
    uint32_t total_copy, tmp_copy;
    uint8_t *tmp_buf;
    int32_t state_before;
    uint8_t addr_match;
    uint32_t cur_time;

    if (ota_tftp_sess == NULL) {
        goto recv_end;
    }

    memset(&master, 0, sizeof(fd_set));
    FD_SET(ota_tftp_sess->sock, &master);

    tv.tv_sec = TFTP_RECV_TIMEOUT;
    tv.tv_usec = 0;

    while (ota_tftp_sess->status == OTA_STATUS_RUNNING ||
           ota_tftp_sess->status == OTA_STATUS_RUNNING_WAITING_FOR_STOP) {
        /* check if there are enough buffer to store data */
        if ((ota_tftp_sess->receive_data + TFTP_PAYLOAD_SIZE) > TFTP_RCV_BUF_SIZE) {
            qurt_signal_wait(&ota_tftp_sess->signal, TFTP_RX_BUF_READY_SIG_MASK, QURT_SIGNAL_ATTR_CLEAR_MASK);
        }

        sockset = master;
        conn_sock = select(ota_tftp_sess->sock + 1, &sockset, NULL, NULL, &tv);
        cur_time = hres_timer_curr_time_ms();

        if (conn_sock < 0) {
            /* finish */
            ota_tftp_sess->status = OTA_STATUS_STOP;
            ota_tftp_sess->error_code = QAPI_FW_UPGRADE_ERR_TFTP_CONNECT_FAIL;
            break;
        } else if ((conn_sock > 0) && FD_ISSET(ota_tftp_sess->sock, &sockset)) {
            received = recvfrom(ota_tftp_sess->sock, (char *)(&ota_tftp_sess->snd_rcv_buffer[0]),
                                TFTP_SEND_RCV_BUF_SIZE, 0, (struct sockaddr *)&from, &fromlen);

            if (received > 0) {
                /* check address */
                if (ota_tftp_sess->tftp_state == TFTP_ST_RRQ) {
                    addr_match = 0;
                    if (from.ss_family == ota_tftp_sess->foreign_addr.ss_family) {
                        if (from.ss_family == AF_INET) {
                            if (memcmp(&(((struct sockaddr_in *)&from)->sin_addr),
                                       &(((struct sockaddr_in *)&ota_tftp_sess->foreign_addr)->sin_addr),
                                       sizeof(((struct sockaddr_in *)&from)->sin_addr)) == 0) {
                                addr_match = 1;
                            }
                        }
#if LWIP_IPV6
                        else {
                            if (memcmp(&(((struct sockaddr_in6 *)&from)->sin6_addr),
                                       &(((struct sockaddr_in6 *)&ota_tftp_sess->foreign_addr)->sin6_addr),
                                       sizeof(((struct sockaddr_in6 *)&from)->sin6_addr)) == 0) {
                                addr_match = 1;
                            }
                        }
#endif
                    }
                    if (addr_match == 0) {
                        if (cur_time - ota_tftp_sess->send_time >= TFTP_TIMEOUT) {
                            ota_tftp_timeout();
                        }
                        continue;
                    }
                }

                /* Reset retries */
                ota_tftp_sess->retry_count = 0;

                /* Make sure we are not overflowing RX buffer */
                if (received > (TFTP_PAYLOAD_SIZE + TFTP_HEADER_SIZE)) {
                    ota_tftp_pkt_error(TFTP_ILLEGAL_OPERATION, "ERROR");
                    ota_tftp_sess->status = OTA_STATUS_STOP;
                    ota_tftp_sess->error_code = QAPI_FW_UPGRADE_ERR_TFTP_IMAGE_DOWNLOAD_FAIL;
                    break;
                }

                state_before = ota_tftp_sess->tftp_state;
                ota_tftp_pkt_parse(&ota_tftp_sess->snd_rcv_buffer[0], received, &(ota_tftp_sess->pkt_seq),
                                   &(ota_tftp_sess->tftp_state));

                if (ota_tftp_sess->tftp_state == TFTP_ST_ERROR) {
                    /* response: 0x1002 (file download fail) */
                    uint8_t err = ota_tftp_sess->snd_rcv_buffer[4];
                    ota_tftp_sess->status = OTA_STATUS_STOP;
                    if (err == TFTP_FILE_NOT_FOUND) {
                        ota_tftp_sess->error_code = QAPI_FW_UPGRADE_ERR_TFTP_IMAGE_NOT_FOUND;

                    } else {
                        ota_tftp_sess->error_code = QAPI_FW_UPGRADE_ERR_TFTP_IMAGE_DOWNLOAD_FAIL;
                    }
                    break;
                }

                if (ota_tftp_sess->tftp_state != TFTP_ST_DATA) {
                    /* do nothing */
                    if (cur_time - ota_tftp_sess->send_time >= TFTP_TIMEOUT) {
                        ota_tftp_timeout();
                    }
                    continue;
                }

                if (state_before == TFTP_ST_RRQ) {
                    if (connect(ota_tftp_sess->sock, (struct sockaddr *)&from, fromlen) == -1) {
                        ota_tftp_sess->status = OTA_STATUS_STOP;
                        ota_tftp_sess->error_code = QAPI_FW_UPGRADE_ERR_TFTP_SOCKET_CONNECT;
                        break;
                    }
                }

                if (ota_tftp_sess->pkt_seq > ota_tftp_sess->pkt_seq_last) {
                    if (ota_tftp_sess->pkt_seq == 0) {
                        ota_tftp_pkt_error(TFTP_ILLEGAL_OPERATION, "ERROR");
                        ota_tftp_sess->status = OTA_STATUS_STOP;
                        ota_tftp_sess->error_code = QAPI_FW_UPGRADE_ERR_TFTP_IMAGE_DOWNLOAD_FAIL;
                        break;
                    }

                    ota_tftp_sess->pkt_seq_last = ota_tftp_sess->pkt_seq;

                    /* store data */
                    qurt_mutex_lock(&ota_tftp_sess->mutex);
                    total_copy = (received - TFTP_HEADER_SIZE);
                    tmp_buf = &ota_tftp_sess->snd_rcv_buffer[TFTP_HEADER_SIZE];
                    while (total_copy) {
                        if ((ota_tftp_sess->write_pointer + total_copy) > TFTP_RCV_BUF_SIZE) {
                            tmp_copy = TFTP_RCV_BUF_SIZE - ota_tftp_sess->write_pointer;
                        } else {
                            tmp_copy = total_copy;
                        }

                        memscpy(&ota_tftp_sess->rcv_buffer[ota_tftp_sess->write_pointer], tmp_copy, tmp_buf, tmp_copy);
                        ota_tftp_sess->write_pointer = ((ota_tftp_sess->write_pointer + tmp_copy) % TFTP_RCV_BUF_SIZE);
                        total_copy -= tmp_copy;
                        tmp_buf += tmp_copy;
                        ota_tftp_sess->receive_data += tmp_copy;
                    }
                    qurt_mutex_unlock(&ota_tftp_sess->mutex);

                    /* send ack */
                    ota_tftp_pkt_ack(ota_tftp_sess->pkt_seq);

                    /* indicate data ready */
                    qurt_signal_set(&ota_tftp_sess->signal, TFTP_RX_DATA_READY_SIG_MASK);

                    /* transfer done if pkt length is less than 512 */
                    if (received < (TFTP_PAYLOAD_SIZE + TFTP_HEADER_SIZE)) {
                        /* finish */
                        ota_tftp_sess->status = OTA_STATUS_RUNNING_WAITING_FOR_STOP;
                        continue;
                    }
                } else if (ota_tftp_sess->pkt_seq == ota_tftp_sess->pkt_seq_last) {
                    /* Server resent the last packt again. It might not have received ACK.
                       Re-send ACK */
                    if (ota_tftp_sess->status == OTA_STATUS_RUNNING_WAITING_FOR_STOP) {
                        /*continue to receive*/
                        continue;
                    } else
                        ota_tftp_pkt_ack(ota_tftp_sess->pkt_seq);
                } else {
                    if (cur_time - ota_tftp_sess->send_time >= TFTP_TIMEOUT) {
                        ota_tftp_timeout();
                    }
                }

            } else if (received == 0) {
                /* finish */
                ota_tftp_sess->status = OTA_STATUS_STOP;
                break;
            } else if (received < 0) {
                ota_tftp_sess->status = OTA_STATUS_STOP;
                ota_tftp_sess->error_code = QAPI_FW_UPGRADE_ERR_TFTP_CONNECT_FAIL;
                break;
            }
        } else {
            if (cur_time - ota_tftp_sess->send_time >= TFTP_TIMEOUT) {
                if (ota_tftp_sess->status == OTA_STATUS_RUNNING_WAITING_FOR_STOP) {
                    /*no pkt to receive*/
                    ota_tftp_sess->status = OTA_STATUS_STOP;
                } else
                    ota_tftp_timeout();
            }
        }
    }

recv_end:
    if (ota_tftp_sess) {
        if (ota_tftp_sess->error_code != 0) {
            qurt_signal_set(&ota_tftp_sess->signal, TFTP_RX_ERROR_SIG_MASK);
        } else {
            qurt_signal_set(&ota_tftp_sess->signal, TFTP_RX_FINISH_SIG_MASK);
        }
        ota_tftp_sess->task_handle = NULL;
    }
    nt_osal_thread_delete(NULL);
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
qapi_Status_t plugin_tftp_recv_data(uint8_t *buffer, uint32_t buf_len, uint32_t *ret_size, void *init_param)
{
    uint32_t signal;
    uint32_t total_copy;
    uint32_t tmp_copy;
    uint8_t *receive_buf = buffer;
    uint32_t signal_mask = TFTP_RX_BUF_READY_SIG_MASK;

    UNUSED(init_param);

    if (buffer == NULL || buf_len == 0) {
        return QAPI_FW_UPGRADE_ERR_INVALID_PARAM;
    }

    if (ota_tftp_sess == NULL || ota_tftp_sess->status == OTA_STATUS_NOT_STARTED) {
        return QAPI_FW_UPGRADE_ERR_TFTP_SESSION_NOT_START;
    }

    if (ota_tftp_sess->error_code != 0) {
        return ((qapi_Status_t)ota_tftp_sess->error_code);
    }

    if (ret_size != NULL) {
        *ret_size = 0;
    }

    if (ota_tftp_sess->status == OTA_STATUS_RUNNING) {
        do {
            signal = qurt_signal_wait(&ota_tftp_sess->signal, TFTP_RX_ALL_SIG_MASK, QURT_SIGNAL_ATTR_CLEAR_MASK);

            if (signal & TFTP_RX_DATA_READY_SIG_MASK) {
                qurt_mutex_lock(&ota_tftp_sess->mutex);

                total_copy = MIN(buf_len, ota_tftp_sess->receive_data);
                if (ret_size != NULL) {
                    *ret_size = total_copy;
                }
                while (total_copy) {
                    if (total_copy + ota_tftp_sess->read_pointer > TFTP_RCV_BUF_SIZE) {
                        tmp_copy = (TFTP_RCV_BUF_SIZE - ota_tftp_sess->read_pointer);
                    } else {
                        tmp_copy = total_copy;
                    }
                    memscpy(receive_buf, tmp_copy, &ota_tftp_sess->rcv_buffer[ota_tftp_sess->read_pointer], tmp_copy);
                    receive_buf += tmp_copy;
                    ota_tftp_sess->read_pointer = ((ota_tftp_sess->read_pointer + tmp_copy) % TFTP_RCV_BUF_SIZE);
                    total_copy -= tmp_copy;
                    ota_tftp_sess->receive_data -= tmp_copy;
                }

                if (ota_tftp_sess->receive_data > 0) {
                    signal_mask |= TFTP_RX_DATA_READY_SIG_MASK;
                }

                qurt_mutex_unlock(&ota_tftp_sess->mutex);
                qurt_signal_set(&ota_tftp_sess->signal, signal_mask);
            }

            if (signal & TFTP_RX_FINISH_SIG_MASK) {
            }
        } while (0);
    }

    return QAPI_OK;
}

/*
 * OTA TFTP plugin Init
 * interface_name:    interface name, such as wlan1
 *            url:    parameters, format: <server>/<url>
 *      int param:    optional init parameters
 */
qapi_Status_t plugin_tftp_init(const char *interface_name, const char *url, void *init_param)
{
    qapi_Status_t ret;
    char *ptr;
    char ip_addr[32];
    int family;
#if LWIP_IPV4
    struct sockaddr_in *foreign_addr;
    struct sockaddr_in local_addr;
#endif
#if LWIP_IPV6
    struct sockaddr_in6 *foreign_addr6;
    struct sockaddr_in6 local_addr6;
#endif
    struct sockaddr *addr;
    uint32_t addrlen;
    uint32_t set_signal;

    UNUSED(interface_name);
    UNUSED(init_param);

    if (ota_tftp_sess != NULL) {
        return QAPI_FW_UPGRADE_ERR_TFTP_SESSION_ALREADY_START;
    }

    ota_tftp_sess = malloc(sizeof(tftp_session_info_t));
    if (ota_tftp_sess == NULL) {
        return QAPI_FW_UPGRADE_ERR_TFTP_NO_MEMORY;
    }

    memset(ota_tftp_sess, 0, sizeof(tftp_session_info_t));
    ota_tftp_sess->sock = -1;

    ota_tftp_sess->snd_rcv_buffer = malloc(TFTP_SEND_RCV_BUF_SIZE);
    if (ota_tftp_sess->snd_rcv_buffer == NULL) {
        ret = QAPI_FW_UPGRADE_ERR_TFTP_NO_MEMORY;
        goto tftp_init_end;
    }
    ota_tftp_sess->rcv_buffer = malloc(TFTP_RCV_BUF_SIZE);
    if (ota_tftp_sess->rcv_buffer == NULL) {
        ret = QAPI_FW_UPGRADE_ERR_TFTP_NO_MEMORY;
        goto tftp_init_end;
    }

    ptr = strchr(url, '/');
    if (ptr == NULL || (ptr - url >= 32)) {
        ret = QAPI_FW_UPGRADE_ERR_TFTP_URL_FORMAT;
        goto tftp_init_end;
    }
    memscpy((void *)ip_addr, (ptr - url), url, (ptr - url));
    ip_addr[ptr - url] = '\0';

    ptr = ptr + 1;
    if (strlen(ptr) == 0 || strlen(ptr) >= TFTP_FILE_NAME_LENGTH) {
        ret = QAPI_FW_UPGRADE_ERR_TFTP_URL_FORMAT;
        goto tftp_init_end;
    }
    memscpy(ota_tftp_sess->filename, (strlen(ptr) + 1), ptr, (strlen(ptr) + 1));

    ptr = strchr((char *)ip_addr, ':');
    if (ptr != NULL) {  // IPV6
#if LWIP_IPV6
        family = AF_INET6;
        foreign_addr6 = (struct sockaddr_in6 *)(&ota_tftp_sess->foreign_addr);
        if (inet_pton(family, ip_addr, &foreign_addr6->sin6_addr) != 1) {
            ret = QAPI_FW_UPGRADE_ERR_TFTP_URL_FORMAT;
            goto tftp_init_end;
        }

        foreign_addr6->sin6_port = htons(TFTP_SERVER_PORT);
        foreign_addr6->sin6_family = family;

        memset(&local_addr6, 0, sizeof(local_addr6));
        local_addr6.sin6_port = htons(TFTP_CLIENT_PORT);
        local_addr6.sin6_family = family;
        addr = (struct sockaddr *)&local_addr6;
        addrlen = sizeof(struct sockaddr_in6);
#else
        ret = QAPI_FW_UPGRADE_ERR_TFTP_URL_FORMAT;
        goto tftp_init_end;
#endif
    } else {
#if LWIP_IPV4
        family = AF_INET;
        foreign_addr = (struct sockaddr_in *)(&ota_tftp_sess->foreign_addr);
        if (inet_pton(family, ip_addr, &foreign_addr->sin_addr) != 1) {
            ret = QAPI_FW_UPGRADE_ERR_TFTP_URL_FORMAT;
            goto tftp_init_end;
        }
        foreign_addr->sin_port = htons(TFTP_SERVER_PORT);
        foreign_addr->sin_family = family;

        memset(&local_addr, 0, sizeof(local_addr));
        local_addr.sin_port = htons(TFTP_CLIENT_PORT);
        local_addr.sin_family = family;
        addr = (struct sockaddr *)&local_addr;
        addrlen = sizeof(struct sockaddr_in);
#else
        ret = QAPI_FW_UPGRADE_ERR_TFTP_URL_FORMAT;
        goto tftp_init_end;
#endif
    }

    if ((ota_tftp_sess->sock = socket(family, SOCK_DGRAM, 0)) == -1) {
        ret = QAPI_FW_UPGRADE_ERR_TFTP_CREATE_SOCKET;
        goto tftp_init_end;
    }

    if (bind(ota_tftp_sess->sock, addr, addrlen) != 0) {
        ret = QAPI_FW_UPGRADE_ERR_TFTP_BIND_FAIL;
        goto tftp_init_end;
    }

    ota_tftp_sess->status = OTA_STATUS_RUNNING;

    /* send RRQ */
    ota_tftp_sess->tftp_state = TFTP_ST_RRQ;
    ota_tftp_pkt_rrq(ota_tftp_sess->filename);

    qurt_signal_create(&ota_tftp_sess->signal);
    qurt_mutex_create(&ota_tftp_sess->mutex);

    /* create a thread to receive data and send ack */
    if (nt_qurt_thread_create(ota_tftp_recv, "tftp_receive", 1024, NULL, TCPIP_THREAD_PRIO,
                              &ota_tftp_sess->task_handle) == -1) {
        ret = QAPI_FW_UPGRADE_ERR_TFTP_THREAD_FAIL;
        goto tftp_init_end;
    }

    /* wait for receiving 1st data */
    set_signal = qurt_signal_wait(&ota_tftp_sess->signal, TFTP_RX_ALL_SIG_MASK, QURT_SIGNAL_ATTR_WAIT_ANY);
    if (set_signal & TFTP_RX_ERROR_SIG_MASK) {
        ret = (qapi_Status_t)ota_tftp_sess->error_code;
    } else {
        return QAPI_OK;
    }

tftp_init_end:
    ota_tftp_fin();
    return ret;
}

qapi_Status_t plugin_tftp_fin(void)
{
    uint8_t *buffer = NULL;
    uint32_t *ret_size = 0;

    /*When finish, receive one last time, if server sent something, just drop it*/
    if ((buffer = malloc(TFTP_RCV_BUF_SIZE)) == NULL) {
        printf("Out of memory error\r\n");
        return QAPI_FW_UPGRADE_ERR_TFTP_NO_MEMORY;
    }

    plugin_tftp_recv_data(buffer, TFTP_RCV_BUF_SIZE, ret_size, NULL);
    free(buffer);

    ota_tftp_fin();
    return QAPI_OK;
}

qapi_Status_t plugin_tftp_abort(void)
{
    ota_tftp_fin();
    return QAPI_OK;
}

/* TFTP doesn't support resume. */
qapi_Status_t plugin_tftp_resume(const char *interface_name, const char *url, uint32_t offset)
{
    UNUSED(interface_name);
    UNUSED(url);
    UNUSED(offset);
    return QAPI_OK;
}
