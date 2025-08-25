/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*========================================================================
 *
 * @brief Function definitions for TCP connections
 *=======================================================================*/

/*------------------------------------------------------------------------
 * Include Files
 * ----------------------------------------------------------------------*/
#include <stdlib.h>
#include "err.h"
#include "netif.h"
#include "dhcp6.h"
#include "prot/dhcp6.h"
#include "tcp.h"

#include "nt_osal.h"
#include "nt_common.h"
#include "nt_logger_api.h"
#include "data_svc_tcp_priv.h"
#include "ring_svc_api.h"
#include "network_al.h"

#ifdef SUPPORT_RING_IF
/*
*Response from Apps to ip_tcp_closed_ind. On receiving this response
*we can delete the rings associated with the terminated connection
*@param req_msg :   The message buffer from Apps with all relevant fields filled
*@return        :   TRUE if processed correctly
                    FALSE if unable to process request
*/

bool data_svc_tcp_closed_rsp(ip_tcp_closed_rsp_t *p_req_msg)
{
    configASSERT(p_req_msg);

    uint8_t conn_id = DATA_SVC_MAX_CONN;
    conn_handler_t *p_conn_handle = NULL;
    bool b_err = FALSE;

    // get handle from connn id
    conn_id = p_req_msg->conn_id;
    configASSERT(conn_id < DATA_SVC_MAX_CONN);

    p_conn_handle = data_svc_get_conn_handler_from_id(conn_id);
    configASSERT(p_conn_handle != NULL);

    configASSERT(DATA_SVC_TCP == p_conn_handle->type);

    // close rings
    b_err = ringif_delete_a2f_tcp_ring(p_conn_handle->a2f_ring_id);
    configASSERT(b_err);
    b_err = ringif_delete_f2a_tcp_ring(p_conn_handle->f2a_ring_id);
    configASSERT(b_err);

    // free connection handle
    data_svc_free_conn_handler(conn_id);

    return TRUE;
}

/*
 *Callback function when the connection has unexpectedly closed or
 *has been aborted. ip_tcp_closed_ind is called if an unexpected error
 *occured on an existing tcp connection. ip_tcp_client_connect_ind is called
 *if handshake for a client connect has failed.
 *@param arg     :   The conn_handler_t structure is passed as arg to this callback.
 *@param err     :   lwip error code
 *@return        :   lwip error code
 */

static void data_svc_tcp_err(void *arg, err_t err)
{
    configASSERT(arg != NULL);
    conn_handler_t *p_conn_handle = NULL;
    bool b_err = FALSE;

    p_conn_handle = (conn_handler_t *)arg;
    configASSERT(IP_DATA_SVC_SERVER == p_conn_handle->type);

    if (IP_DATA_SVC_NOT_CONNECTED == p_conn_handle->conn_state) {
        ip_tcp_client_connect_ind_t *p_rsp_msg;

        p_rsp_msg = (ip_tcp_client_connect_ind_t *)ringif_get_ctrl_buf(0, NULL);
        if (p_rsp_msg == NULL) {
            RINGIF_PRINT_LOG_ERR("data_svc_tcp_err no mem conn:%d\r\n", p_conn_handle->idx);
            return;
        }

        p_rsp_msg->api_hdr.api_id = IP_TCP_CLIENT_CONNECT_IND;
        p_rsp_msg->conn_id = p_conn_handle->idx;
        p_rsp_msg->error_code = ERR_CONN;
        p_rsp_msg->netif_id = p_conn_handle->netif_id;
        p_rsp_msg->port = p_conn_handle->local_port;
        p_rsp_msg->a2f_ring_id = DATA_SVC_INVALID_RING_ID;
        p_rsp_msg->a2f_ring_addr = 0;
        p_rsp_msg->a2f_ring_num_elem = 0;
        p_rsp_msg->f2a_ring_id = DATA_SVC_INVALID_RING_ID;
        p_rsp_msg->f2a_ring_addr = 0;
        p_rsp_msg->f2a_ring_num_elem = 0;

        // inform control ring
        b_err = ringif_send_ip_conn_config((uint32_t *)p_rsp_msg, sizeof(ip_tcp_client_connect_ind_t));
        if (FALSE == b_err) {
            RINGIF_PRINT_LOG_ERR("data_svc_tcp_err config send fail:%d\r\n", p_conn_handle->idx);
            ringif_free_ctrl_buf(p_rsp_msg, 0);
            return;
        }
    } else {
        ip_tcp_closed_ind_t *p_rsp_msg;

        p_rsp_msg = (ip_tcp_closed_ind_t *)ringif_get_ctrl_buf(0, NULL);
        ;
        if (NULL == p_rsp_msg) {
            RINGIF_PRINT_LOG_ERR("data_svc_tcp_err config send fail:%d\r\n", p_conn_handle->idx);
            return;
        }

        // prepare response message
        p_rsp_msg->api_hdr.api_id = IP_TCP_CLOSED_IND;
        p_rsp_msg->conn_id = p_conn_handle->idx;
        p_rsp_msg->err_code = err;

        // inform control ring
        b_err = ringif_send_ip_conn_config((uint32_t *)p_rsp_msg, sizeof(ip_tcp_closed_ind_t));
        if (FALSE == b_err) {
            RINGIF_PRINT_LOG_ERR("data_svc_tcp_err config send fail:%d\r\n", p_conn_handle->idx);
            ringif_free_ctrl_buf(p_rsp_msg, 0);
            return;
        }
    }
}

/*
*Send as much data out as possible
*@param p_conn_handle   :   conn_handle for connection on which
                            we're trying to send out data
*/

static void send_data_out(conn_handler_t *p_conn_handle)
{
    configASSERT(p_conn_handle);

    uint16_t avail_len = 0;
    uint16_t rem_len = 0;
    uint16_t write_len = 0;
    uint8_t *p_write_ptr = 0;
    tcp_send_buff_t *p_send_buff = NULL;
    err_t err = ERR_OK;
    uint8_t api_flags = 0;

    p_send_buff = p_conn_handle->send_buff;

    if (p_send_buff != NULL) {
        if (NULL == p_conn_handle->pcb) {
            RINGIF_PRINT_LOG_ERR("pcb null");
            return;
        }
        // query available space in snd_queue
        avail_len = tcp_sndbuf((struct tcp_pcb *)p_conn_handle->pcb);
        rem_len = p_send_buff->p_pbuf->len - p_conn_handle->sent_length;

        if (rem_len < avail_len) {
            write_len = rem_len;
        } else {
            write_len = avail_len;
        }

        // calculate write_ptr
        p_write_ptr = ((uint8_t *)(p_send_buff->p_pbuf->payload)) + (p_conn_handle->sent_length);

        // try sending the data out
        do {
            err = tcp_write(p_conn_handle->pcb, p_write_ptr, write_len, api_flags);

            if (err != ERR_OK) {
                write_len = write_len / 2;
            }
        } while ((err != ERR_OK) && (write_len > 0));

        if (write_len > 0)  // succesfully sent some data out
        {
            p_conn_handle->sent_length = p_conn_handle->sent_length + write_len;
        }

        // check if buffer completley sent
        if (p_conn_handle->sent_length == p_send_buff->p_pbuf->len) {
            // add buffer to end of list waiting for ack
            tcp_send_buff_t *head = p_conn_handle->ack_list;

            if (NULL == head) {
                p_conn_handle->ack_list = p_send_buff;
            } else {
                while (NULL != head->next) {
                    head = head->next;
                }
                head->next = p_send_buff;
            }
#ifdef SUPPORT_DATA_LOOPBACK
            if (TRUE == data_svc_get_loopback_state()) {
                p_conn_handle->send_buff = p_conn_handle->send_buff->next;
                p_conn_handle->sent_length = 0;
            } else {
                p_conn_handle->send_buff = NULL;
                p_conn_handle->sent_length = 0;

                // query ring for more data
                ringif_tickle_a2f_ring(p_conn_handle->a2f_ring_id, FALSE);
            }

#else
            p_conn_handle->send_buff = NULL;
            p_conn_handle->sent_length = 0;

            // query ring for more data
            ringif_tickle_a2f_ring(p_conn_handle->a2f_ring_id, FALSE);
#endif
        }
    }
    return;
}

/* Callback function for when tcp data has been ACK'd
 *Inform Apps of the number of bytes that have been ACK'd.
 *@param arg     :   The conn_handler_t structure is passed as arg to this callback.
 *@param tcp_pcb :   lwip's connection context for the connection
 *@len           :   Length of ACK'd data
 *@return        :   lwip error code
 */

static err_t data_svc_tcp_sent(void *arg, __unused struct tcp_pcb *p_tpcb, uint16_t len)
{
    configASSERT(arg != NULL);
    conn_handler_t *p_conn_handle = NULL;
    ip_tcp_sent_ind_t *p_rsp_msg = NULL;
    bool b_err = FALSE;

    p_conn_handle = (conn_handler_t *)arg;
    configASSERT(DATA_SVC_TCP == p_conn_handle->type);

    p_rsp_msg = (ip_tcp_sent_ind_t *)ringif_get_ctrl_buf(0, NULL);
    if (p_rsp_msg == NULL) {
        RINGIF_PRINT_LOG_ERR("data_svc_tcp_sent no mem conn:%d\r\n", p_conn_handle->idx);
        return ERR_MEM;
    }

    if (p_rsp_msg != NULL) {
        // prepare response message
        p_rsp_msg->api_hdr.api_id = IP_TCP_SENT_IND;
        p_rsp_msg->conn_id = p_conn_handle->idx;
        p_rsp_msg->len = len;

        // inform ctrl ring
        b_err = ringif_send_ip_conn_config((uint32_t *)p_rsp_msg, sizeof(ip_tcp_sent_ind_t));

        if (FALSE == b_err) {
            RINGIF_PRINT_LOG_ERR("data_svc_tcp_sent config mem fail conn:%d\r\n", p_conn_handle->idx);
            ringif_free_ctrl_buf(p_rsp_msg, 0);
            return ERR_MEM;
        }

        p_conn_handle->ack_length = p_conn_handle->ack_length + len;

        // check if a buffer in ack list has been completley ACK'd
        if (p_conn_handle->ack_list != NULL) {
            tcp_send_buff_t *p_ptr = NULL;

            while (p_conn_handle->ack_length > p_conn_handle->ack_list->p_pbuf->len) {
                p_ptr = p_conn_handle->ack_list;
                p_conn_handle->ack_list = p_conn_handle->ack_list->next;

                p_conn_handle->ack_length = p_conn_handle->ack_length - p_ptr->p_pbuf->len;

                pbuf_free(p_ptr->p_pbuf);
                nt_osal_free_memory(p_ptr);
            }
        }

        // If there is a buff still being processed, send data out again
        send_data_out(p_conn_handle);

        return ERR_OK;
    } else {
        RINGIF_PRINT_LOG_ERR("data_svc_tcp_sent fail due to no mem for conn:%d\r\n", p_conn_handle->idx);
        return ERR_MEM;
    }
}

#ifdef SUPPORT_DATA_LOOPBACK
/*
 *Function to send recieved data back to remote peer when loopback is enabled
 *@param p_conn_handle   :   The conn_handler_t structure for conn details
 *@param pbuf            :   structure containg the payload that was received by lwip
 *@return                :   lwip error code
 */
static void loopback_tcp_data(struct pbuf *p_pbuf, conn_handler_t *p_conn_handle)
{
    configASSERT(p_conn_handle != NULL);

    tcp_send_buff_t *send_buff = (tcp_send_buff_t *)nt_osal_allocate_memory(sizeof(tcp_send_buff_t));

    if (NULL != send_buff) {
        send_buff->p_pbuf = p_pbuf;
        send_buff->next = NULL;

        if (NULL != p_conn_handle->send_buff) {
            // attach newly recived data to end of list
            tcp_send_buff_t *head = p_conn_handle->send_buff;
            while (head->next != NULL) {
                head = head->next;
            }
            head->next = send_buff;
        } else {
            p_conn_handle->send_buff = send_buff;
        }

        send_data_out(p_conn_handle);
    } else {
        RINGIF_PRINT_LOG_ERR(" Insufficeient memory");
    }
}
#endif

/*
 *Callback when data has been receievd on a tcp connection. If pbuf is NULL
 *the remote peer has requested that the connection be closed. Otherwise there
 *is data to be sent to apps
 *@param arg     :   The conn_handler_t structure is passed as arg to this callback.
 *@param tpcb    :   lwip's connection context for the connection
 *@param pbuf    :   structure containg the payload that was received by lwip
 *@param err     :   lwip error code
 *@return        :   lwip error code
 */

static err_t data_svc_tcp_recv(void *arg, __unused struct tcp_pcb *p_tpcb, struct pbuf *p_pbuf, __unused err_t err)
{
    configASSERT(arg != NULL);

    conn_handler_t *p_conn_handle = NULL;
    bool b_err = FALSE;
    err_t error_code = ERR_OK;

    if (p_pbuf == NULL)  // remote has requested to close the connection
    {
        ip_tcp_closed_ind_t *p_rsp_msg = NULL;

        p_rsp_msg = (ip_tcp_closed_ind_t *)ringif_get_ctrl_buf(0, NULL);
        if (p_rsp_msg == NULL) {
            RINGIF_PRINT_LOG_ERR("data_svc_tcp_recv no mem err:%d\r\n", err);
            return ERR_MEM;
        }

        if (p_rsp_msg != NULL) {
            p_conn_handle = (conn_handler_t *)arg;
            configASSERT(DATA_SVC_TCP == p_conn_handle->type);

            // close the connection pcb
            error_code = tcp_close(p_conn_handle->pcb);
            NT_LOG_PRINT(COMMON, ERR, "closed tcp pcb");
            if (error_code != ERR_OK) {
                tcp_abort(p_conn_handle->pcb);
                NT_LOG_PRINT(COMMON, ERR, "aborted tcp pcb");
            }

            // prepare response message
            p_rsp_msg->api_hdr.api_id = IP_TCP_CLOSED_IND;
            p_rsp_msg->conn_id = p_conn_handle->idx;
            p_rsp_msg->err_code = ERR_OK;

            // send ressponse to ctrl ring
            b_err = ringif_send_ip_conn_config((uint32_t *)p_rsp_msg, sizeof(ip_tcp_closed_ind_t));

            if (FALSE == b_err) {
                ringif_free_ctrl_buf(p_rsp_msg, 0);
                RINGIF_PRINT_LOG_ERR("data_svc_tcp_recv send err idx:%d\r\n", p_conn_handle->idx);
                return ERR_MEM;
            }

            return ERR_OK;
        } else  // p_rsp_msg != NULL
        {
            RINGIF_PRINT_LOG_ERR("data_svc_tcp_recv no mem idx:%d\r\n", p_conn_handle->idx);
            return ERR_MEM;
        }
    }

    p_conn_handle = (conn_handler_t *)arg;

#ifdef SUPPORT_DATA_LOOPBACK

    if (TRUE == data_svc_get_loopback_state()) {
        b_err = TRUE;
        loopback_tcp_data(p_pbuf, p_conn_handle);
    } else {
        b_err = ringif_f2a_pkt_attach(p_conn_handle->f2a_ring_id, (uint32_t *)p_pbuf, (uint32_t *)p_pbuf->payload,
                                      p_pbuf->len, (uint16_t)p_conn_handle->idx);
    }
#else
    b_err = ringif_f2a_pkt_attach(p_conn_handle->f2a_ring_id, (uint32_t *)p_pbuf, (uint32_t *)p_pbuf->payload,
                                  p_pbuf->len, (uint16_t)p_conn_handle->idx);
#endif

    tcp_recved(p_tpcb, p_pbuf->len);
    if (b_err == TRUE) {
        return ERR_OK;
    } else {
        return ERR_MEM;
    }
}

/*
 *Process a request from client to close a tcp connection.
 *Send response to Apps
 *@param req_msg :   The message buffer from Apps with all relevant fields filled
 *@return        :   TRUE if a response is successfully sent back to Apps
 *                   FALSE in case unable to send a response to Apps
 */

bool data_svc_tcp_connection_close_req(ip_tcp_connection_close_req_t *p_req_msg)
{
    configASSERT(p_req_msg);

    ip_tcp_connection_close_cfm_t *p_rsp_msg = NULL;
    uint8_t conn_id = DATA_SVC_MAX_CONN;
    conn_handler_t *p_conn_handle = NULL;
    err_t error_code = ERR_OK;
    bool b_err = FALSE;

    p_rsp_msg = (ip_tcp_connection_close_cfm_t *)ringif_get_ctrl_buf(0, NULL);
    if (p_rsp_msg == NULL) {
        RINGIF_PRINT_LOG_ERR("data_svc_tcp_connection_close_req no mem conn:%d\r\n", p_req_msg->conn_id);
        return FALSE;
    }

    if (p_rsp_msg != NULL) {
        // get connection handler from id
        conn_id = p_req_msg->conn_id;

        if (conn_id < DATA_SVC_MAX_CONN) {
            p_conn_handle = data_svc_get_conn_handler_from_id(conn_id);
            if (p_conn_handle != NULL) {
                if (p_conn_handle->type != IP_DATA_SVC_LISTEN) {
                    // set callbacks to NULL
                    tcp_recv(p_conn_handle->pcb, NULL);
                    tcp_arg(p_conn_handle->pcb, NULL);
                    tcp_sent(p_conn_handle->pcb, NULL);
                    tcp_err(p_conn_handle->pcb, NULL);

                    // close the connection pcb
                    error_code = tcp_close(p_conn_handle->pcb);
                    if (error_code != ERR_OK) {
                        tcp_abort(p_conn_handle->pcb);
                    }

                    // close rings

                    if (DATA_SVC_INVALID_RING_ID != p_conn_handle->a2f_ring_id) {
                        b_err = ringif_delete_a2f_tcp_ring(p_conn_handle->a2f_ring_id);
                        configASSERT(b_err);
                    }

                    if (DATA_SVC_INVALID_RING_ID != p_conn_handle->f2a_ring_id) {
                        b_err = ringif_delete_f2a_tcp_ring(p_conn_handle->f2a_ring_id);
                        configASSERT(b_err);
                    }

                    data_svc_free_conn_handler(conn_id);

                    error_code = ERR_OK;

                } else  // p_conn_handle->type!=LWIP_SVC_LISTEN
                {
                    error_code = ERR_ARG;
                }
            } else  // p_conn_handle != NULL
            {
                error_code = ERR_ARG;
            }
        } else  // conn_id < LWIP_SVC_INVALID_CONN
        {
            error_code = ERR_ARG;
        }

        // prepare response message
        p_rsp_msg->api_hdr.api_id = IP_TCP_CONNECTION_CLOSE_CFM;
        p_rsp_msg->conn_id = conn_id;
        p_rsp_msg->err_code = error_code;

        // inform ctrl ring
        b_err = ringif_send_ip_conn_config((uint32_t *)p_rsp_msg, sizeof(ip_tcp_connection_close_cfm_t));
        if (FALSE == b_err) {
            ringif_free_ctrl_buf(p_rsp_msg, 0);
            RINGIF_PRINT_LOG_ERR("data_svc_tcp_connection_close_req send fail conn:%d\r\n", p_req_msg->conn_id);
            return FALSE;
        }

        return b_err;
    } else  // p_rsp_msg != NULL
    {
        return FALSE;
    }
}

/*
 *Callback function when a client request has succeeded. Finish setting up the
 *connection. Create rings for apps to send data on this connection.
 *Send response to Apps
 *@param arg     :   The conn_handler_t structure is passed as arg to this callback.
 *@param tcp_pcb :   lwip's connection context for the connection
 *@err           :   error code from lwip
 *@return        :   lwip error code
 */

static err_t data_svc_tcp_client_connect_callback(void *arg, __unused struct tcp_pcb *p_tcp_pcb, __unused err_t err)
{
    configASSERT(arg != NULL);
    configASSERT(p_tcp_pcb);

    conn_handler_t *p_conn_handle = NULL;
    ip_tcp_client_connect_ind_t *p_rsp_msg = NULL;
    uint8_t a2f_ring_id = DATA_SVC_INVALID_RING_ID;
    uint32_t a2f_ring_addr = 0;
    uint16_t a2f_ring_num_elem = 0;
    uint8_t a2f_ring_elem_size = 0;
    uint8_t f2a_ring_id = DATA_SVC_INVALID_RING_ID;
    uint32_t f2a_ring_addr = 0;
    uint16_t f2a_ring_num_elem = 0;
    uint8_t f2a_ring_elem_size = 0;
    bool b_err = FALSE;

    p_conn_handle = (conn_handler_t *)arg;

    p_rsp_msg = (ip_tcp_client_connect_ind_t *)ringif_get_ctrl_buf(0, NULL);
    if (p_rsp_msg == NULL) {
        RINGIF_PRINT_LOG_ERR("data_svc_tcp_client_connect_callback no mem err:%d\r\n", err);
        return ERR_MEM;
    }

    if (p_rsp_msg != NULL) {
        // get the ring id, addr etc
        b_err = ringif_create_a2f_tcp_ring(&a2f_ring_id);
        if (FALSE == b_err) {
            RINGIF_PRINT_LOG_ERR("data_svc_tcp_client_connect_callback a2f ring create fail\r\n");
            return ERR_MEM;
        }

        b_err = ringif_create_f2a_tcp_ring(&f2a_ring_id);
        if (FALSE == b_err) {
            ringif_delete_a2f_tcp_ring(a2f_ring_id);
            RINGIF_PRINT_LOG_ERR("data_svc_tcp_client_connect_callback a2f ring create fail\r\n");
            return ERR_MEM;
        }

        a2f_ring_addr = (uint32_t)ringif_a2f_ring_addr(a2f_ring_id);
        f2a_ring_addr = (uint32_t)ringif_f2a_ring_addr(f2a_ring_id);

        a2f_ring_num_elem = ringif_a2f_num_ring_elems(a2f_ring_id);
        f2a_ring_num_elem = ringif_f2a_num_ring_elems(f2a_ring_id);

        a2f_ring_elem_size = ringif_a2f_elem_size(a2f_ring_id);
        f2a_ring_elem_size = ringif_f2a_elem_size(f2a_ring_id);

        p_conn_handle->a2f_ring_id = a2f_ring_id;
        p_conn_handle->f2a_ring_id = f2a_ring_id;

        p_conn_handle->conn_state = IP_DATA_SVC_CONNECTED;

        // prepare message to send back to Apps
        p_rsp_msg->api_hdr.api_id = IP_TCP_CLIENT_CONNECT_IND;
        p_rsp_msg->conn_id = p_conn_handle->idx;
        p_rsp_msg->error_code = ERR_OK;
        p_rsp_msg->netif_id = p_conn_handle->netif_id;
        p_rsp_msg->port = p_conn_handle->local_port;
        p_rsp_msg->a2f_ring_id = a2f_ring_id;
        p_rsp_msg->a2f_ring_addr = a2f_ring_addr;
        p_rsp_msg->a2f_ring_num_elem = a2f_ring_num_elem;
        p_rsp_msg->a2f_ring_elem_size = a2f_ring_elem_size;
        p_rsp_msg->f2a_ring_id = f2a_ring_id;
        p_rsp_msg->f2a_ring_addr = f2a_ring_addr;
        p_rsp_msg->f2a_ring_num_elem = f2a_ring_num_elem;
        p_rsp_msg->f2a_ring_elem_size = f2a_ring_elem_size;

        // inform ctrl ring
        b_err = ringif_send_ip_conn_config((uint32_t *)p_rsp_msg, sizeof(ip_tcp_client_connect_ind_t));
        if (FALSE == b_err) {
            RINGIF_PRINT_LOG_ERR("data_svc_tcp_client_connect_callback no mem conn:%d", p_conn_handle->idx);
            ringif_free_ctrl_buf(p_rsp_msg, 0);
            return ERR_MEM;
        }

        RINGIF_PRINT_LOG_ERR("Data A2F Ring(%d): Addr:%x NumElem:%d ElemSize:%d", p_rsp_msg->a2f_ring_id,
                             p_rsp_msg->a2f_ring_addr, p_rsp_msg->a2f_ring_num_elem, p_rsp_msg->a2f_ring_elem_size);

        RINGIF_PRINT_LOG_ERR("Data F2A Ring(%d): Addr:%x NumElem:%d ElemSize:%d", p_rsp_msg->f2a_ring_id,
                             p_rsp_msg->f2a_ring_addr, p_rsp_msg->f2a_ring_num_elem, p_rsp_msg->f2a_ring_elem_size);

        return ERR_OK;
    } else  // p_rsp_msg != NULL
    {
        return ERR_MEM;
    }
}

/*
 *Process a request to start a client and connect to a remote peer.This function
 *initiates the process to connect. ip_tcp_client_connect_callback() is registered
 *as callback for successful conncetion through tcp_connect().
 *Send response to Apps.
 *@param req_msg :   The message buffer from Apps with all relevant fields filled
 *@return        :   TRUE if a response is successfully sent back to Apps
 *                   FALSE in case unable to send a response to Apps
 */
bool data_svc_tcp_client_connect_req(ip_tcp_client_connect_req_t *p_req_msg)
{
    configASSERT(p_req_msg != NULL);

    ip_tcp_client_connect_cfm_t *p_rsp_msg = NULL;
    struct netif *p_tcp_netif = NULL;
    uint8_t local_ip_ver = 0;
    ip_addr_t local_ip;
    struct tcp_pcb *p_conn_pcb = NULL;
    err_t error_code = ERR_OK;
    uint8_t remote_ip_ver = 0;
    ip_addr_t remote_addr;
    conn_handler_t *p_conn_handle = NULL;
    bool b_err = FALSE;

    p_rsp_msg = (ip_tcp_client_connect_cfm_t *)ringif_get_ctrl_buf(0, NULL);
    if (p_rsp_msg == NULL) {
        RINGIF_PRINT_LOG_ERR("data_svc_tcp_client_connect_req no mem netif:%d\r\n", p_req_msg->netif_id);
        return FALSE;
    }

    if (p_rsp_msg != NULL) {
        // get netif structure from netif_id
        p_tcp_netif = netif_get_by_index(p_req_msg->netif_id);
        if (TRUE == nt_dpm_is_netif_ready(p_tcp_netif)) {
            local_ip_ver = nt_dpm_get_ip_ver(p_tcp_netif);
            ip_addr_t staipaddr =
                IPADDR6_INIT_HOST(0xfec00000ul, 0x0ul, 0x0ul, 0x01ul);  // default IPV6_adderss for netif
            ip_addr_t apipaddr = IPADDR6_INIT_HOST(0xfec00000ul, 0x0ul, 0x0ul, 0x02ul);  // default IPV6 address

            if (local_ip_ver == IPADDR_TYPE_V4) {
                local_ip = p_tcp_netif->ip_addr;
            } else {
                // dhcp
                // TBD: Check DHCP assigns IP addr in index 1 of netif->ip6_addr array
                if (netif_dhcp6_data(p_tcp_netif) != NULL) {
                    local_ip = p_tcp_netif->ip6_addr[1];
                }
                // static
                else if (!ip_addr_cmp(&staipaddr, &p_tcp_netif->ip6_addr[1]) &&
                         !ip_addr_cmp(&apipaddr, &p_tcp_netif->ip6_addr[1])) {
                    local_ip = p_tcp_netif->ip6_addr[1];
                }
                // link local
                else {
                    local_ip = p_tcp_netif->ip6_addr[0];
                }
            }

            // call tcp_new_ip_type
            p_conn_pcb = tcp_new_ip_type(local_ip_ver);
            if (p_conn_pcb != NULL) {
                error_code = tcp_bind(p_conn_pcb, &local_ip, p_req_msg->local_port);
                if (error_code == ERR_OK) {
                    tcp_bind_netif(p_conn_pcb, p_tcp_netif);
                    // get remote ip as ip_addr_t remote_addr
                    remote_ip_ver = p_req_msg->ip_ver;

                    if (remote_ip_ver == IPADDR_TYPE_V6) {
                        IP_ADDR6(&remote_addr, p_req_msg->remote_ipv6_addr[0], p_req_msg->remote_ipv6_addr[1],
                                 p_req_msg->remote_ipv6_addr[2], p_req_msg->remote_ipv6_addr[3]);
                    } else {
                        ip_addr_set_ip4_u32_val(remote_addr, p_req_msg->remote_ipv4_addr);
                    }

                    // Create connection handler
                    p_conn_handle = data_svc_get_new_conn_handler();
                    if (p_conn_handle != NULL) {
                        p_conn_handle->type = DATA_SVC_TCP;
                        p_conn_handle->netif_id = p_req_msg->netif_id;
                        p_conn_handle->local_port = p_req_msg->local_port;
                        p_conn_handle->pcb = p_conn_pcb;
                        p_conn_handle->server = IP_DATA_SVC_CLIENT;
                        p_conn_handle->tls_params = p_req_msg->tls_params;
                        p_conn_handle->conn_state = IP_DATA_SVC_NOT_CONNECTED;
                        p_conn_handle->a2f_ring_id = DATA_SVC_INVALID_RING_ID;
                        p_conn_handle->f2a_ring_id = DATA_SVC_INVALID_RING_ID;
                        p_conn_handle->send_buff = NULL;
                        p_conn_handle->sent_length = 0;
                        p_conn_handle->ack_list = NULL;
                        p_conn_handle->ack_length = 0;

                        // set up callbacks
                        tcp_arg(p_conn_pcb, p_conn_handle);
                        tcp_sent(p_conn_pcb, data_svc_tcp_sent);
                        tcp_err(p_conn_pcb, data_svc_tcp_err);
                        tcp_recv(p_conn_pcb, data_svc_tcp_recv);

                        // Initiate connect to remote address
                        error_code = tcp_connect(p_conn_pcb, &remote_addr, p_req_msg->remote_port,
                                                 data_svc_tcp_client_connect_callback);

                    } else  // p_conn_handle != NULL
                    {
                        error_code = ERR_MEM;
                    }

                }  // error_code ==ERR_OK

            } else  // p_conn_pcb!=NULL
            {
                error_code = ERR_MEM;
            }

        } else  // p_tcp_netif !=NULL
        {
            error_code = ERR_CONN;
        }

        // prepare response to send back to Apps
        p_rsp_msg->api_hdr.api_id = IP_TCP_CLIENT_CONNECT_CFM;
        p_rsp_msg->netif_id = p_req_msg->netif_id;
        p_rsp_msg->port = p_req_msg->local_port;
        p_rsp_msg->err_code = error_code;
        if (p_conn_handle != NULL) {
            p_rsp_msg->conn_id = p_conn_handle->idx;
        }
        // inform ctrl ring
        b_err = ringif_send_ip_conn_config((uint32_t *)p_rsp_msg, sizeof(ip_tcp_client_connect_cfm_t));
        if (FALSE == b_err) {
            if (p_conn_handle != NULL) {
                RINGIF_PRINT_LOG_ERR("data_svc_tcp_client_connect_req no mem conn:%d", p_conn_handle->idx);
            }
            ringif_free_ctrl_buf(p_rsp_msg, 0);
            return ERR_MEM;
        }

        if (error_code != ERR_OK) {
            // cleanup

            if (p_conn_pcb != NULL) {
                error_code = tcp_close(p_conn_pcb);
                if (error_code != ERR_OK) {
                    tcp_abort(p_conn_pcb);
                }
            }

            if (p_conn_handle != NULL) {
                data_svc_free_conn_handler(p_conn_handle->idx);
            }
        }

        return b_err;
    } else  // p_rsp_msg != NULL
    {
        return FALSE;
    }
}

/*
 *Process request from apps to close a listen port. Send response to Apps.
 *@param req_msg :   The message buffer from Apps with all relevant fields filled
 *@return        :   TRUE if a response is successfully sent back to Apps
 *                   FALSE in case unable to send a response to Apps
 */

bool data_svc_tcp_listen_close_req(ip_tcp_listen_close_req_t *p_req_msg)
{
    configASSERT(p_req_msg != NULL);

    ip_tcp_listen_close_cfm_t *p_rsp_msg = NULL;
    listen_handler_t *p_listen_handle = NULL;
    uint8_t listen_id;
    err_t err = ERR_OK;
    bool b_err = FALSE;

    p_rsp_msg = (ip_tcp_listen_close_cfm_t *)ringif_get_ctrl_buf(0, NULL);
    if (p_rsp_msg == NULL) {
        RINGIF_PRINT_LOG_ERR("data_svc_tcp_listen_close_req no mem listen:%d\r\n", p_req_msg->listen_id);
        return FALSE;
    }

    if (p_rsp_msg != NULL) {
        // get the handler of the connection
        listen_id = p_req_msg->listen_id;

        p_listen_handle = data_svc_get_listen_handler_from_id(listen_id);

        if (NULL != p_listen_handle) {
            if (DATA_SVC_TCP == p_listen_handle->type) {
                // setting all callbacks to NULL
                tcp_arg(p_listen_handle->pcb, NULL);
                tcp_accept(p_listen_handle->pcb, NULL);

                // call tcp_close
                err = tcp_close(p_listen_handle->pcb);
                if (err != ERR_OK) {
                    tcp_abort(p_listen_handle->pcb);
                }

                // free connection handler
                data_svc_free_listen_handler(listen_id);

                err = ERR_OK;

            }  // p_conn_handle->type == LWIP_SVC_TCP
            else {
                err = ERR_ARG;
            }
        } else  // NULL != p_listen_handle
        {
            err = ERR_ARG;
        }
        // send error message back
        p_rsp_msg->api_hdr.api_id = IP_TCP_LISTEN_CLOSE_CFM;
        if (p_listen_handle != NULL) {
            p_rsp_msg->listen_id = p_listen_handle->idx;
        }
        p_rsp_msg->err_code = err;

        // inform ctrl ring
        b_err = ringif_send_ip_conn_config((uint32_t *)p_rsp_msg, sizeof(ip_tcp_listen_close_cfm_t));
        if (FALSE == b_err) {
            if (p_listen_handle != NULL) {
                RINGIF_PRINT_LOG_ERR("data_svc_tcp_listen_close_req no mem conn:%d", p_listen_handle->idx);
            }
            ringif_free_ctrl_buf(p_rsp_msg, 0);
        }

        return b_err;
    } else  // p_rsp_msg != NULL
    {
        return FALSE;
    }
}

/*
 *This is called when a remote peer is trying to connect to a listen port.
 *Inform Apps that a client is tring to connect to listen.
 *Set up rings and connection ctx for the accepted connection
 *@param arg     :   The conn_handler_t structure is passed as arg to this callback.
 *@param new_pcb :   lwip's connection context for the new connection
 *@err_code      :   error code from lwip
 *@return        :   lwip error code
 */
static err_t tcp_accept_client(void *arg, struct tcp_pcb *p_new_pcb, err_t err)
{
    ip_tcp_client_accepted_ind_t *p_rsp_msg = NULL;
    listen_handler_t *p_listen_handle = NULL;
    conn_handler_t *p_new_conn_handle = NULL;
    uint8_t a2f_ring_id = DATA_SVC_INVALID_RING_ID;
    uint32_t a2f_ring_addr = 0;
    uint16_t a2f_ring_num_elem = 0;
    uint8_t a2f_ring_elem_size = 0;
    uint8_t f2a_ring_id = DATA_SVC_INVALID_RING_ID;
    uint32_t f2a_ring_addr = 0;
    uint16_t f2a_ring_num_elem = 0;
    uint8_t f2a_ring_elem_size = 0;
    bool b_err = FALSE;

    configASSERT((p_new_pcb != NULL) && (arg != NULL))

        // prepare response
        p_rsp_msg = (ip_tcp_client_accepted_ind_t *)ringif_get_ctrl_buf(0, NULL);
    if (p_rsp_msg == NULL) {
        RINGIF_PRINT_LOG_ERR("data_svc_tcp_listen_close_req no mem err:%d\r\n", err);
        return ERR_MEM;
    }

    if (p_rsp_msg != NULL) {
        // get associated handle for listen pcb
        p_listen_handle = (listen_handler_t *)arg;

        if ((err == ERR_OK)) {
            // get connection handler id for incoming connection
            p_new_conn_handle = data_svc_get_new_conn_handler();

            if (p_new_conn_handle != NULL) {
                // get ring paramteters
                b_err = ringif_create_f2a_tcp_ring(&f2a_ring_id);
                if (FALSE == b_err) {
                    RINGIF_PRINT_LOG_ERR("tcp_accept_client f2a ring create fail\r\n");
                    return ERR_MEM;
                }

                b_err = ringif_create_a2f_tcp_ring(&a2f_ring_id);
                if (FALSE == b_err) {
                    ringif_delete_f2a_tcp_ring(f2a_ring_id);
                    RINGIF_PRINT_LOG_ERR("tcp_accept_client a2f ring create fail\r\n");
                    return ERR_MEM;
                }

                a2f_ring_addr = (uint32_t)ringif_a2f_ring_addr(a2f_ring_id);
                f2a_ring_addr = (uint32_t)ringif_f2a_ring_addr(f2a_ring_id);

                a2f_ring_num_elem = ringif_a2f_num_ring_elems(a2f_ring_id);
                f2a_ring_num_elem = ringif_f2a_num_ring_elems(f2a_ring_id);

                a2f_ring_elem_size = ringif_a2f_elem_size(a2f_ring_id);
                f2a_ring_elem_size = ringif_f2a_elem_size(f2a_ring_id);

                // fill connection handler
                p_new_conn_handle->type = DATA_SVC_TCP;
                p_new_conn_handle->netif_id = p_listen_handle->netif_id;
                p_new_conn_handle->local_port = p_listen_handle->local_port;
                p_new_conn_handle->pcb = p_new_pcb;
                p_new_conn_handle->a2f_ring_id = a2f_ring_id;
                p_new_conn_handle->f2a_ring_id = f2a_ring_id;
                p_new_conn_handle->server = IP_DATA_SVC_SERVER;
                p_new_conn_handle->tls_params = p_listen_handle->tls_params;
                p_new_conn_handle->conn_state = IP_DATA_SVC_CONNECTED;
                p_new_conn_handle->send_buff = NULL;
                p_new_conn_handle->sent_length = 0;
                p_new_conn_handle->ack_list = NULL;
                p_new_conn_handle->ack_length = 0;

                // set up the callback function
                tcp_arg(p_new_pcb, p_new_conn_handle);
                tcp_recv(p_new_pcb, data_svc_tcp_recv);
                tcp_err(p_new_pcb, data_svc_tcp_err);
                tcp_sent(p_new_pcb, data_svc_tcp_sent);

            }  // p_new_conn_handle !=NULL

            else {
                err = ERR_MEM;
            }
        }  // err==ERR_OK

        // send message back
        p_rsp_msg->api_hdr.api_id = IP_TCP_CLIENT_ACCEPTED_IND;
        if (p_new_conn_handle != NULL) {
            p_rsp_msg->conn_id = p_new_conn_handle->idx;
        }
        p_rsp_msg->err_code = err;
        p_rsp_msg->a2f_ring_id = a2f_ring_id;
        p_rsp_msg->a2f_ring_addr = a2f_ring_addr;
        p_rsp_msg->a2f_ring_num_elem = a2f_ring_num_elem;
        p_rsp_msg->f2a_ring_id = f2a_ring_id;
        p_rsp_msg->f2a_ring_addr = f2a_ring_addr;
        p_rsp_msg->f2a_ring_num_elem = f2a_ring_num_elem;
        p_rsp_msg->a2f_ring_elem_size = a2f_ring_elem_size;
        p_rsp_msg->f2a_ring_elem_size = f2a_ring_elem_size;
        p_rsp_msg->listen_id = p_listen_handle->idx;

        ip_addr_t *ipaddress = &(p_new_pcb->remote_ip);
        if (ipaddress->type == IPADDR_TYPE_V4) {
            p_rsp_msg->ip_ver = IPADDR_TYPE_V4;
            p_rsp_msg->remote_ipv4_addr = ip_addr_get_ip4_u32(ipaddress);
            p_rsp_msg->remote_port = p_new_pcb->remote_port;
        } else {
            p_rsp_msg->ip_ver = IPADDR_TYPE_V6;
            p_rsp_msg->remote_port = p_new_pcb->remote_port;

            ip6_addr_t *p_ipv6 = ip_2_ip6(ipaddress);
            p_rsp_msg->remote_ipv6_addr[0] = p_ipv6->addr[0];
            p_rsp_msg->remote_ipv6_addr[1] = p_ipv6->addr[1];
            p_rsp_msg->remote_ipv6_addr[2] = p_ipv6->addr[2];
            p_rsp_msg->remote_ipv6_addr[3] = p_ipv6->addr[3];
        }

        // inform ctrl ring
        b_err = ringif_send_ip_conn_config((uint32_t *)p_rsp_msg, sizeof(ip_tcp_client_accepted_ind_t));
        if (FALSE == b_err) {
            RINGIF_PRINT_LOG_ERR("tcp_accept_client no mem conn:%d", p_listen_handle->idx);
            ringif_free_ctrl_buf(p_rsp_msg, 0);
            return FALSE;
        }

        RINGIF_PRINT_LOG_ERR("Data A2F Ring(%d): Addr:%x NumElem:%d ElemSize:%d", p_rsp_msg->a2f_ring_id,
                             p_rsp_msg->a2f_ring_addr, p_rsp_msg->a2f_ring_num_elem, p_rsp_msg->a2f_ring_elem_size);

        RINGIF_PRINT_LOG_ERR("Data F2A Ring(%d): Addr:%x NumElem:%d ElemSize:%d", p_rsp_msg->f2a_ring_id,
                             p_rsp_msg->f2a_ring_addr, p_rsp_msg->f2a_ring_num_elem, p_rsp_msg->f2a_ring_elem_size);

        return ERR_OK;
    }

    else  // p_rsp_msg != NULL
    {
        return ERR_MEM;
    }
}

/*
 *Processes a request to open a tcp listen port.Sends response to Apps.
 *@param req_msg :   The message buffer from Apps with all relevant fields filled
 *                   to start a tcp listen
 *@return        :   TRUE if a response is successfully sent back to Apps
 *                   FALSE in case unable to send a response to Apps
 */
bool data_svc_tcp_listen_start_req(ip_tcp_listen_start_req_t *p_req_msg)
{
    configASSERT(p_req_msg != NULL);

    ip_tcp_listen_start_cfm_t *p_rsp_msg = NULL;
    struct netif *p_tcp_netif = NULL;
    uint8_t ip_ver;
    ip_addr_t local_ip;
    struct tcp_pcb *p_new_pcb = NULL;
    err_t error_code = ERR_OK;
    struct tcp_pcb *p_listen_pcb = NULL;
    listen_handler_t *p_listen_handle = NULL;
    bool b_err = FALSE;

    // prepare response ip_tcp_listen_start_cfm
    p_rsp_msg = (ip_tcp_listen_start_cfm_t *)ringif_get_ctrl_buf(0, NULL);
    if (p_rsp_msg == NULL) {
        RINGIF_PRINT_LOG_ERR("data_svc_tcp_listen_close_req no mem port:%d\r\n", p_req_msg->local_port);
        return FALSE;
    }

    if (p_rsp_msg != NULL) {
        // get netif structure from netif_id
        p_tcp_netif = netif_get_by_index(p_req_msg->netif_id);
        if (TRUE == nt_dpm_is_netif_ready(p_tcp_netif)) {
            // find out which ip_ver and get value
            ip_ver = nt_dpm_get_ip_ver(p_tcp_netif);

            ip_addr_t staipaddr =
                IPADDR6_INIT_HOST(0xfec00000ul, 0x0ul, 0x0ul, 0x01ul);  // default IPV6_adderss for netif
            ip_addr_t apipaddr = IPADDR6_INIT_HOST(0xfec00000ul, 0x0ul, 0x0ul, 0x02ul);  // default IPV6 address

            if (ip_ver == IPADDR_TYPE_V4) {
                local_ip = p_tcp_netif->ip_addr;
            } else {
                // dhcp
                // TBD: Check DHCP assigns IP addr in index 1 of netif->ip6_addr array
                if (netif_dhcp6_data(p_tcp_netif) != NULL) {
                    local_ip = p_tcp_netif->ip6_addr[1];
                }
                // static
                else if (!ip_addr_cmp(&staipaddr, &p_tcp_netif->ip6_addr[1]) &&
                         !ip_addr_cmp(&apipaddr, &p_tcp_netif->ip6_addr[1])) {
                    local_ip = p_tcp_netif->ip6_addr[1];
                }
                // link local
                else {
                    local_ip = p_tcp_netif->ip6_addr[0];
                }

            }  // ip_ver==IPADDR_TYPE_V4

            p_new_pcb = tcp_new_ip_type(ip_ver);
            if (p_new_pcb != NULL) {
                error_code = ERR_OK;
                error_code = tcp_bind(p_new_pcb, &local_ip, p_req_msg->local_port);

                if (error_code == ERR_OK) {
                    tcp_bind_netif(p_new_pcb, p_tcp_netif);
                    p_listen_pcb = tcp_listen_with_backlog(p_new_pcb, p_req_msg->backlog);
                    if (p_listen_pcb != NULL) {
                        p_listen_handle = data_svc_get_new_listen_handler();
                        if (p_listen_handle != NULL) {
                            p_listen_handle->type = DATA_SVC_TCP;
                            p_listen_handle->netif_id = p_req_msg->netif_id;
                            p_listen_handle->local_port = p_req_msg->local_port;
                            p_listen_handle->pcb = p_listen_pcb;
                            p_listen_handle->tls_params = p_req_msg->tls_params;

                            // set up tcp_arg and tcp_accept
                            tcp_arg(p_listen_pcb, p_listen_handle);
                            tcp_accept(p_listen_pcb, tcp_accept_client);

                            // server started succesfully
                            error_code = ERR_OK;

                        }  // p_conn_handle != NULL
                        else {
                            error_code = ERR_MEM;
                        }

                    }  // p_listen_pcb != NULL
                    else {
                        error_code = ERR_MEM;
                    }
                }  // error_code == ERR_OK

            }  // p_new_pcb !=NULL
            else {
                error_code = ERR_MEM;
            }

        }  // p_tcp_netif != NULL
        else {
            error_code = ERR_CONN;
        }

        // Fill message to send to Apps

        p_rsp_msg->api_hdr.api_id = IP_TCP_LISTEN_START_CFM;
        p_rsp_msg->netif_id = p_req_msg->netif_id;
        p_rsp_msg->local_port = p_req_msg->local_port;
        p_rsp_msg->err_code = error_code;
        if (p_listen_handle != NULL) {
            p_rsp_msg->listen_id = p_listen_handle->idx;
        }

        // inform ctrl ring
        b_err = ringif_send_ip_conn_config((uint32_t *)p_rsp_msg, sizeof(ip_tcp_listen_start_cfm_t));
        if (FALSE == b_err) {
            if (p_listen_handle != NULL) {
                RINGIF_PRINT_LOG_ERR("data_svc_tcp_listen_start_req no mem conn:%d", p_listen_handle->idx);
            }
            ringif_free_ctrl_buf(p_rsp_msg, 0);
            return FALSE;
        }

        if (error_code != ERR_OK) {
            // cleanup
            if (p_listen_pcb != NULL) {
                tcp_abort(p_listen_pcb);
            }
        }

        return b_err;

    }  // p_rsp_msg != NULL

    else  // p_rsp_msg != NULL
    {
        return FALSE;
    }
}

/*
 *Free buffer used for tcp receive
 *@param     :   pointer to ring element
 *@return    :   TRUE if buffer freed
 *           :   FALSE if buffer not freed
 */

bool data_svc_free_tcp_data_buff(void *p_element)
{
    struct pbuf *p_pbuf = NULL;
    ring_element_t *p_elem = NULL;

    p_elem = (ring_element_t *)p_element;

    p_pbuf = p_elem->p_buf_start;

    pbuf_free(p_pbuf);

    p_elem->p_buf = NULL;
    p_elem->p_buf_start = NULL;

    return TRUE;
}

/*
 *Function to supply buffers to the ring
 *@param len     :   len of the payload required
 *@return         :   TRUE if memory allocated successfully
 *                   FALSE if memory could not be allocated
 */
bool data_svc_get_tcp_data_buff(void *p_element, uint16_t len)
{
    ring_element_t *p_elem = (ring_element_t *)p_element;
    struct pbuf *p_pbuf = pbuf_alloc(PBUF_RAW, len, PBUF_RAM);

    if (p_pbuf != NULL) {
        p_elem->p_buf = p_pbuf->payload;
        p_elem->p_buf_start = p_pbuf;
        p_elem->len = len;
        return TRUE;
    }

    return FALSE;
}

/*
*Processs the ring element and try to send out data
*@param p_element   :   element of the ring
*@return            :   TRUE, if succesfully sent out
                    :   FALSE, if not able to send
*/

bool data_svc_process_tcp_pkt(void *p_element)
{
    bool return_val = FALSE;
    ring_element_t *p_elem = NULL;
    uint8_t conn_id = DATA_SVC_MAX_CONN;
    conn_handler_t *p_conn_handle = NULL;
    struct pbuf *p_pbuf = NULL;

    p_elem = (ring_element_t *)p_element;

    if (NULL == p_elem) {
        RINGIF_PRINT_LOG_ERR("Elem NULL");
        return FALSE;
    }

    conn_id = (uint8_t)p_elem->info;

    p_conn_handle = data_svc_get_conn_handler_from_id(conn_id);
    if (NULL != p_conn_handle) {
        if (NULL == p_conn_handle->send_buff) {
            tcp_send_buff_t *send_buff = (tcp_send_buff_t *)nt_osal_allocate_memory(sizeof(tcp_send_buff_t));
            if (NULL != send_buff) {
                // store buffer in connection handle
                p_pbuf = p_elem->p_buf_start;
                p_pbuf->len = p_elem->len;
                p_pbuf->tot_len = p_elem->len;

                send_buff->p_pbuf = p_pbuf;
                send_buff->next = NULL;

                p_conn_handle->send_buff = send_buff;

                send_data_out(p_conn_handle);

                return_val = TRUE;
            } else  // NULL != send_buff
            {
                RINGIF_PRINT_LOG_ERR("No mem for TCP Send buf, postponing pkt len:%d info:%d", p_elem->len,
                                     p_elem->info);
                return_val = FALSE;
            }

        }  // NULL == conn_handle->send_buff
        else {
            return_val = FALSE;
        }
    } else  // NULL != p_conn_handle
    {
        RINGIF_PRINT_LOG_ERR("No conn exist to send data len:%d info:%d", p_elem->len, p_elem->info);
        return_val = FALSE;
    }

    return return_val;
}
#endif  // SUPPORT_RING_IF
