/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*========================================================================
 *
 * @brief Function definitions for UDP connections
 *=======================================================================*/

/*------------------------------------------------------------------------
 * Include Files
 * ----------------------------------------------------------------------*/
#include <stdlib.h>
#include "err.h"
#include "netif.h"
#include "dhcp6.h"
#include "prot/dhcp6.h"
#include "udp.h"

#include "nt_osal.h"
#include "nt_common.h"
#include "nt_logger_api.h"
#include "data_svc_udp_priv.h"
#include "ring_svc_api.h"
#include "network_al.h"

#ifdef SUPPORT_RING_IF
/*
 *Process request from Apps to close a udp sever/client
 *@param req_msg :   The message buffer from Apps with all relevant fields filled
 *@return        :   TRUE if a response is successfully sent back to Apps
 *                   FALSE in case unable to send a response to Apps
 */

bool data_svc_udp_close_req(ip_udp_close_req_t *p_req_msg)
{
    configASSERT(p_req_msg != NULL);

    ip_udp_close_cfm_t *p_rsp_msg = NULL;
    conn_handler_t *p_conn_handle = NULL;
    listen_handler_t *p_listen_handle = NULL;
    uint8_t handle_type = DATA_SVC_INVALID_TYPE;
    bool b_err = FALSE;
    uint8_t current_bit = 0;
    err_t error_code = ERR_OK;

    p_rsp_msg = (ip_udp_close_cfm_t *)ringif_get_ctrl_buf(0, NULL);

    if (p_rsp_msg != NULL) {
        if (p_req_msg->conn_id < DATA_SVC_MAX_CONN) {
            // close a udp client
            p_conn_handle = data_svc_get_conn_handler_from_id(p_req_msg->conn_id);

            if (p_conn_handle != NULL) {
                handle_type = p_conn_handle->type;
                if (handle_type == DATA_SVC_UDP) {
                    if (DATA_SVC_INVALID_RING_ID == p_conn_handle->f2a_ring_id) {
                        // This is an A2F conn id. remove udp pcb
                        udp_disconnect(p_conn_handle->pcb);
                        b_err = ringif_delete_a2f_udp_ring(p_conn_handle->a2f_ring_id);
                        configASSERT(b_err);

                        udp_remove(p_conn_handle->pcb);
                        data_svc_free_conn_handler(p_req_msg->conn_id);
                        error_code = ERR_OK;
                    } else {
                        // this is an f2a connection.
                        RINGIF_PRINT_LOG_ERR("data_svc_udp_close_req: invalid conn id. f2a conn id received");
                        error_code = ERR_ARG;
                    }

                } else {
                    error_code = ERR_ARG;
                }
            } else {
                error_code = ERR_ARG;
            }
        } else {
            // close a udp listen

            p_listen_handle = data_svc_get_listen_handler_from_id(p_req_msg->listen_id);
            if (p_listen_handle != NULL) {
                handle_type = p_listen_handle->type;
                if (handle_type == DATA_SVC_UDP) {
                    udp_recv(p_listen_handle->pcb, NULL, NULL);

                    // go through bitmap and close all related conn_ids
                    while (current_bit < DATA_SVC_MAX_CONN) {
                        if (IS_BIT_SET(current_bit, p_listen_handle->bitmap)) {
                            // get conn_handler and free associated rings

                            p_conn_handle = data_svc_get_conn_handler_from_id(current_bit);
                            configASSERT(p_conn_handle != NULL);

                            b_err = ringif_delete_f2a_udp_ring(p_conn_handle->f2a_ring_id);
                            configASSERT(b_err);

                            data_svc_free_conn_handler(current_bit);
                        }

                        current_bit = current_bit + 1;
                    }

                    udp_remove(p_listen_handle->pcb);
                    data_svc_free_listen_handler(p_req_msg->listen_id);
                    error_code = ERR_OK;
                } else {
                    error_code = ERR_ARG;
                }
            } else {
                error_code = ERR_ARG;
            }
        }

        // prepare response message
        p_rsp_msg->api_hdr.api_id = IP_UDP_CLOSE_CFM;
        p_rsp_msg->conn_id = p_req_msg->conn_id;
        p_rsp_msg->listen_id = p_req_msg->listen_id;
        p_rsp_msg->err_code = error_code;

        // inform ctrl ring
        b_err = ringif_send_ip_conn_config((uint32_t *)p_rsp_msg, sizeof(ip_udp_close_cfm_t));

        if (FALSE == b_err) {
            RINGIF_PRINT_LOG_ERR("data_svc_udp_close_req fail due to no mem for rsp conn_id %d\r\n",
                                 p_req_msg->conn_id);
            ringif_free_ctrl_buf(p_rsp_msg, 0);
        } else {
            RINGIF_PRINT_LOG_ERR("Success: rsp msg - %d, error_code - %d, conn_id - %d listen_id %d\r\n",
                                 (uint32_t)p_rsp_msg, error_code, p_rsp_msg->conn_id, p_rsp_msg->listen_id);
        }

        return b_err;
    } else {
        RINGIF_PRINT_LOG_ERR("data_svc_udp_close_req fail due to no mem for conn:%d\r\n", p_req_msg->conn_id);
        return FALSE;
    }
}

/*
 *Process a request to start a udp client to send data out.
 *Send response to Apps
 *@param req_msg :   The message buffer from Apps with all relevant fields filled
 *@return        :   TRUE if a response is successfully sent back to Apps
 *                   FALSE in case unable to send a response to Apps
 */

bool data_svc_udp_client_start_req(ip_udp_client_start_req_t *p_req_msg)
{
    configASSERT(p_req_msg != NULL);

    ip_udp_client_start_cfm_t *p_rsp_msg = NULL;
    struct netif *p_udp_netif = NULL;
    uint8_t ip_ver = 0;
    ip_addr_t local_ip;
    struct udp_pcb *p_new_pcb = NULL;
    err_t error_code = ERR_OK;
    uint8_t remote_ip_ver = 0;
    ip_addr_t remote_addr;
    conn_handler_t *p_conn_handle = NULL;
    uint8_t a2f_ring_id = DATA_SVC_INVALID_RING_ID;
    uint16_t a2f_ring_num_elem = 0;
    uint32_t a2f_ring_addr = 0;
    uint8_t a2f_ring_elem_size = 0;
    bool b_err = FALSE;

    p_rsp_msg = (ip_udp_client_start_cfm_t *)ringif_get_ctrl_buf(0, NULL);

    if (p_rsp_msg != NULL) {
        // get netif structure from netif_id
        p_udp_netif = netif_get_by_index(p_req_msg->netif_id);

        if (TRUE == nt_dpm_is_netif_ready(p_udp_netif)) {
            // find out ip_ver
            ip_ver = nt_dpm_get_ip_ver(p_udp_netif);

            if (ip_ver == IPADDR_TYPE_V4) {
                local_ip = p_udp_netif->ip_addr;
            } else {
                local_ip = p_udp_netif->ip6_addr[1];
            }

            // call udp_new_ip_type
            p_new_pcb = udp_new_ip_type(ip_ver);
            if (p_new_pcb != NULL) {
                // udp_bind
                error_code = udp_bind(p_new_pcb, &local_ip, p_req_msg->local_port);
                if (error_code == ERR_OK) {
                    udp_bind_netif(p_new_pcb, p_udp_netif);
                    remote_ip_ver = p_req_msg->ip_ver;
                    if (remote_ip_ver == IPADDR_TYPE_V6) {
                        IP_ADDR6(&remote_addr, p_req_msg->remote_ipv6_addr[0], p_req_msg->remote_ipv6_addr[1],
                                 p_req_msg->remote_ipv6_addr[2], p_req_msg->remote_ipv6_addr[3]);
                    } else {
                        ip_addr_set_ip4_u32_val(remote_addr, p_req_msg->remote_ipv4_addr);
                    }

                    error_code = udp_connect(p_new_pcb, &remote_addr, p_req_msg->remote_port);
                    if (error_code == ERR_OK) {
                        // create connection handler
                        p_conn_handle = data_svc_get_new_conn_handler();

                        if (p_conn_handle != NULL) {
                            // get the ring id, addr etc
                            b_err = ringif_create_a2f_udp_ring(&a2f_ring_id);

                            if (FALSE == b_err) {
                                RINGIF_PRINT_LOG_ERR("data_svc_udp_client_start_req a2f ring:%d create fail\r\n",
                                                     a2f_ring_id);
                                return FALSE;
                            }

                            a2f_ring_num_elem = ringif_a2f_num_ring_elems(a2f_ring_id);
                            a2f_ring_addr = (uint32_t)ringif_a2f_ring_addr(a2f_ring_id);
                            a2f_ring_elem_size = ringif_a2f_elem_size(a2f_ring_id);

                            p_conn_handle->type = DATA_SVC_UDP;
                            p_conn_handle->netif_id = p_req_msg->netif_id;
                            p_conn_handle->local_port = p_req_msg->local_port;
                            p_conn_handle->pcb = p_new_pcb;
                            p_conn_handle->a2f_ring_id = a2f_ring_id;
                            p_conn_handle->f2a_ring_id = DATA_SVC_INVALID_RING_ID;
                            p_conn_handle->server = IP_DATA_SVC_CLIENT;
                            p_conn_handle->tls_params = 0;
                            p_conn_handle->conn_state = IP_DATA_SVC_CONNECTED;

                            error_code = ERR_OK;
                        } else  // p_conn_handle != NULL
                        {
                            error_code = ERR_MEM;
                        }

                    }  // error_code == ERR_OK (udp_connect)

                }  // error_code  == ERR_OK (udp_bind)

            } else  // p_new_pcb != NULL
            {
                error_code = ERR_MEM;
            }

        }  // p_udp_netif!=NULL
        else {
            error_code = ERR_CONN;
        }

        // prepare response message
        p_rsp_msg->api_hdr.api_id = IP_UDP_CLIENT_START_CFM;
        if (p_conn_handle != NULL) {
            p_rsp_msg->conn_id = p_conn_handle->idx;
        }
        p_rsp_msg->netif_id = p_req_msg->netif_id;
        p_rsp_msg->local_port = p_req_msg->local_port;
        p_rsp_msg->err_code = error_code;
        p_rsp_msg->a2f_ring_id = a2f_ring_id;
        p_rsp_msg->a2f_ring_addr = a2f_ring_addr;
        p_rsp_msg->a2f_ring_num_elem = a2f_ring_num_elem;
        p_rsp_msg->a2f_ring_elem_size = a2f_ring_elem_size;

        // inform ctrl ring
        b_err = ringif_send_ip_conn_config((uint32_t *)p_rsp_msg, sizeof(ip_udp_client_start_cfm_t));

        if (FALSE == b_err) {
            RINGIF_PRINT_LOG_ERR("data_svc_udp_client_start_req fail for netif:%d lport:%d rport:%d\r\n",
                                 p_req_msg->netif_id, p_req_msg->local_port, p_req_msg->remote_port);
            ringif_free_ctrl_buf(p_rsp_msg, 0);
        } else {
            RINGIF_PRINT_LOG_ERR("Success: rsp msg - %d, error_code - %d, conn_id - %d ring-id - %d\r\n",
                                 (uint32_t)p_rsp_msg, error_code, p_rsp_msg->conn_id, p_rsp_msg->a2f_ring_id);
        }

        if (error_code != ERR_OK) {
            // cleanup
            if (p_new_pcb != NULL) {
                udp_remove(p_new_pcb);
            }
        }

        return b_err;
    } else  // p_rsp_msg != NULL
    {
        RINGIF_PRINT_LOG_ERR("data_svc_udp_client_start_req fail due to no mem for netif:%d lport:%d rport:%d\r\n",
                             p_req_msg->netif_id, p_req_msg->local_port, p_req_msg->remote_port);
        return FALSE;
    }
}

/*
 *
 */
static void update_remote_ip_udp_new_remote_ip_ind(ip_udp_new_remote_ip_ind_t *p_rsp_msg, const ip_addr_t *remote_ip)
{
    configASSERT(p_rsp_msg != NULL && remote_ip != NULL);

    if (IP_IS_V4(remote_ip)) {
        p_rsp_msg->ip_ver = IPADDR_TYPE_V4;
        p_rsp_msg->remote_ipv4_addr = ip_addr_get_ip4_u32(remote_ip);
    } else {
        const ip6_addr_t *p_ipv6;
        p_rsp_msg->ip_ver = IPADDR_TYPE_V6;
        p_ipv6 = ip_2_ip6(remote_ip);

        p_rsp_msg->remote_ipv6_addr[0] = p_ipv6->addr[0];
        p_rsp_msg->remote_ipv6_addr[1] = p_ipv6->addr[1];
        p_rsp_msg->remote_ipv6_addr[2] = p_ipv6->addr[2];
        p_rsp_msg->remote_ipv6_addr[3] = p_ipv6->addr[3];
    }
}

static err_t create_udp_connection(conn_handler_t **p_conn_handle, listen_handler_t *p_listen_handle,
                                   const ip_addr_t *p_addr, uint16_t port)
{
    configASSERT(p_conn_handle != NULL);
    configASSERT(p_listen_handle != NULL);
    configASSERT(p_addr != NULL);

    *p_conn_handle = data_svc_get_new_conn_handler();
    bool b_err = FALSE;
    uint8_t f2a_ring_id = DATA_SVC_MAX_LISTEN;

    if (NULL != *p_conn_handle) {
        b_err = ringif_create_f2a_udp_ring(&f2a_ring_id);
        if (FALSE == b_err) {
            RINGIF_PRINT_LOG_ERR("create_udp_connection ring create fail for ring:%d\r\n", f2a_ring_id);
            return ERR_MEM;
        }

        (*p_conn_handle)->netif_id = (p_listen_handle)->netif_id;
        (*p_conn_handle)->local_port = (p_listen_handle)->local_port;
        (*p_conn_handle)->conn_state = IP_DATA_SVC_CONNECTED;
        (*p_conn_handle)->tls_params = 0;
        (*p_conn_handle)->type = DATA_SVC_UDP;
        (*p_conn_handle)->server = IP_DATA_SVC_SERVER;
        (*p_conn_handle)->f2a_ring_id = f2a_ring_id;
        (*p_conn_handle)->a2f_ring_id = DATA_SVC_INVALID_RING_ID;
        ip_addr_set(&((*p_conn_handle)->remote_addr), p_addr);
        (*p_conn_handle)->remote_port = port;

        // set bit in listen_handle
        (p_listen_handle)->bitmap = SET_BIT((*p_conn_handle)->idx, (p_listen_handle)->bitmap);
        return ERR_OK;
    } else {
        RINGIF_PRINT_LOG_ERR("create_udp_connection mem fail for port:%d\r\n", port);
        return ERR_MEM;
    }
}

/*Callback triggered when some data is receievd on a UDP listen.
*Check to see if we this remote peer has been seen before.
*If not, generate a new conn_id and add to bitmap of listen.
@param arg      :   listen_handler
@param pcb      :   pcb of udp listen
@param pb       :   pbuf with received data
@param addr     :   ip addr of remote peer
@param port     :   port of remote peer
*/
static void data_svc_udp_recv_data_pkt(void *arg, struct udp_pcb *pcb, struct pbuf *pb, const ip_addr_t *addr,
                                       u16_t port)
{
    LWIP_UNUSED_ARG(pcb);
    configASSERT(arg != NULL);
    configASSERT(pb != NULL);
    configASSERT(addr != NULL);

    // get listen handler structure
    listen_handler_t *p_listen_handle = (listen_handler_t *)arg;

    uint64_t bitmap = p_listen_handle->bitmap;
    uint8_t current_bit = 0;
    uint8_t conn_id = DATA_SVC_MAX_CONN;
    conn_handler_t *p_conn_handle = NULL;
    bool b_err = FALSE;
    uint8_t f2a_ring_id = DATA_SVC_INVALID_RING_ID;

    // check if we have seen this remote peer before
    while (current_bit < DATA_SVC_MAX_CONN) {
        if (IS_BIT_SET(current_bit, bitmap)) {
            p_conn_handle = data_svc_get_conn_handler_from_id(current_bit);
            if (p_conn_handle == NULL) {
                RINGIF_PRINT_LOG_ERR("data_svc_udp_recv_data_pkt conn handle null for port:%d\r\n", port);
                return;
            }

            if (ip_addr_cmp(&p_conn_handle->remote_addr, addr) && (port == p_conn_handle->remote_port)) {
                break;
            }
        }

        current_bit = current_bit + 1;
    }

    if (DATA_SVC_MAX_CONN == current_bit) {
        RINGIF_PRINT_LOG_ERR("Create new connection");
        // new remote peer, set up rings and inform Apps
        err_t error_code = ERR_OK;
        ip_udp_new_remote_ip_ind_t *p_rsp_msg = NULL;

        p_rsp_msg = (ip_udp_new_remote_ip_ind_t *)ringif_get_ctrl_buf(0, NULL);

        if (NULL != p_rsp_msg) {
            // create a new connection
            error_code = create_udp_connection(&p_conn_handle, p_listen_handle, addr, port);
            if (error_code == ERR_OK) {
                conn_id = p_conn_handle->idx;
                f2a_ring_id = p_conn_handle->f2a_ring_id;
            }

            p_rsp_msg->api_hdr.api_id = IP_UDP_NEW_REMOTE_IP_IND;
            p_rsp_msg->conn_id = conn_id;
            p_rsp_msg->listen_id = p_listen_handle->idx;
            p_rsp_msg->error_code = error_code;
            p_rsp_msg->port_num = port;
            update_remote_ip_udp_new_remote_ip_ind(p_rsp_msg, addr);
            p_rsp_msg->f2a_ring_id = f2a_ring_id;
            p_rsp_msg->f2a_ring_num_elem = ringif_f2a_num_ring_elems(f2a_ring_id);
            p_rsp_msg->f2a_ring_addr = (uint32_t)ringif_f2a_ring_addr(f2a_ring_id);
            p_rsp_msg->f2a_ring_elem_size = ringif_f2a_elem_size(f2a_ring_id);

            // inform ctrl ring
            b_err = ringif_send_ip_conn_config((uint32_t *)p_rsp_msg, sizeof(ip_udp_new_remote_ip_ind_t));

            if (FALSE == b_err) {
                RINGIF_PRINT_LOG_ERR("data_svc_udp_recv_data_pkt conn create fail port:%d\r\n", port);
                ringif_free_ctrl_buf(p_rsp_msg, 0);
                pbuf_free(pb);
                return;
            }
        } else {
            RINGIF_PRINT_LOG_ERR("data_svc_udp_recv_data_pkt mem fail for port:%d\r\n", port);
            return;
        }
    } else  // DATA_SVC_MAX_CONN == current_bit
    {
        conn_id = current_bit;
    }

    p_conn_handle = data_svc_get_conn_handler_from_id(conn_id);
    if (p_conn_handle == NULL) {
        RINGIF_PRINT_LOG_ERR("data_svc_udp_recv_data_pkt conn handle null for port:%d\r\n", port);
        return;
    }

    b_err = ringif_f2a_pkt_attach(p_conn_handle->f2a_ring_id, (uint32_t *)pb, (uint32_t *)pb->payload, pb->len,
                                  (uint16_t)conn_id);

    if (FALSE == b_err) {
        pbuf_free(pb);
        RINGIF_PRINT_LOG_INFO("data_svc_udp_recv_data_pkt pkt send fail for conn:%d\r\n", conn_id);
        return;
    } else {
        RINGIF_PRINT_LOG_INFO("Sent udp packet to ring conn_id:%d ring_id:%d", conn_id, p_conn_handle->f2a_ring_id);
    }
}

/* Process a request from Apps to start udp port for listening to data.
Send reponse back to Apps.
*@param req_msg :   The message buffer from Apps with all relevant fields filled
*@return        :   TRUE if a response is successfully sent back to Apps
*                   FALSE in case unable to send a response to Apps
*/
bool data_svc_udp_server_start_req(ip_udp_server_start_req_t *p_req_msg)
{
    ip_udp_server_start_cfm_t *p_rsp_msg = NULL;
    struct netif *p_udp_netif = NULL;
    ip_addr_t local_ip;
    uint8_t ip_ver = 0;
    struct udp_pcb *p_new_pcb = NULL;
    err_t error_code = ERR_OK;
    listen_handler_t *p_listen_handle = NULL;
    bool b_err = FALSE;

    p_rsp_msg = (ip_udp_server_start_cfm_t *)ringif_get_ctrl_buf(0, NULL);

    if (p_rsp_msg != NULL) {
        // get netif structure from netif_id
        p_udp_netif = netif_get_by_index(p_req_msg->netif_id);

        if (TRUE == nt_dpm_is_netif_ready(p_udp_netif)) {
            // find out ip_ver
            ip_addr_t staipaddr =
                IPADDR6_INIT_HOST(0xfec00000ul, 0x0ul, 0x0ul, 0x01ul);  // default IPV6_adderss for netif
            ip_addr_t apipaddr = IPADDR6_INIT_HOST(0xfec00000ul, 0x0ul, 0x0ul, 0x02ul);  // default IPV6 address

            ip_ver = 0;
            if (ip_ver == IPADDR_TYPE_V4) {
                local_ip = p_udp_netif->ip_addr;
            } else {
                // dhcp
                // TBD: Check DHCP assigns IP addr in index 1 of netif->ip6_addr array
                if (netif_dhcp6_data(p_udp_netif) != NULL) {
                    local_ip = p_udp_netif->ip6_addr[1];
                }
                // static
                else if (!ip_addr_cmp(&staipaddr, &p_udp_netif->ip6_addr[1]) &&
                         !ip_addr_cmp(&apipaddr, &p_udp_netif->ip6_addr[1])) {
                    local_ip = p_udp_netif->ip6_addr[1];
                }
                // link local
                else {
                    local_ip = p_udp_netif->ip6_addr[0];
                }
            }

            // call udp_new_ip_type
            p_new_pcb = udp_new_ip_type(ip_ver);
            if (p_new_pcb != NULL) {
                // udp_bind
                error_code = ERR_OK;
                error_code = udp_bind(p_new_pcb, &local_ip, p_req_msg->local_port);
                if (error_code == ERR_OK) {
                    udp_bind_netif(p_new_pcb, p_udp_netif);
                    // create and fill conection handler
                    p_listen_handle = data_svc_get_new_listen_handler();
                    if (p_listen_handle != NULL) {
                        p_listen_handle->type = DATA_SVC_UDP;
                        p_listen_handle->netif_id = p_req_msg->netif_id;
                        p_listen_handle->local_port = p_req_msg->local_port;
                        p_listen_handle->pcb = p_new_pcb;
                        p_listen_handle->tls_params = 0;
                        p_listen_handle->bitmap = 0;

                        udp_recv(p_new_pcb, data_svc_udp_recv_data_pkt, p_listen_handle);

                        error_code = ERR_OK;

                    } else  // p_conn_handle!=NULL
                    {
                        error_code = ERR_MEM;
                    }

                }  // error_code == ERR_OK

            } else  // p_new_pcb!=NULL
            {
                error_code = ERR_MEM;
            }

        }  // p_udp_netif!=NULL
        else {
            error_code = ERR_CONN;
        }

        // prepare response message
        p_rsp_msg->api_hdr.api_id = IP_UDP_SERVER_START_CFM;
        p_rsp_msg->netif_id = p_req_msg->netif_id;
        p_rsp_msg->port = p_req_msg->local_port;
        p_rsp_msg->err_code = error_code;
        if (p_listen_handle != NULL) {
            p_rsp_msg->listen_id = p_listen_handle->idx;
        }

        // inform ctrl ring
        b_err = ringif_send_ip_conn_config((uint32_t *)p_rsp_msg, sizeof(ip_udp_server_start_cfm_t));

        if (FALSE == b_err) {
            ringif_free_ctrl_buf(p_rsp_msg, 0);
            RINGIF_PRINT_LOG_ERR("data_svc_udp_server_start_req send to ring fail for netif:%d port:%d\r\n",
                                 p_req_msg->netif_id, p_req_msg->local_port);
        } else {
            RINGIF_PRINT_LOG_ERR("Success: rsp msg - %d, error_code - %d, conn_id - %d\r\n", (uint32_t)p_rsp_msg,
                                 error_code, p_rsp_msg->listen_id);
        }

        if (error_code != ERR_OK) {
            if (p_new_pcb != NULL) {
                udp_remove(p_new_pcb);
            }
        }
        return b_err;
    } else  // p_rsp_msg != NULL
    {
        RINGIF_PRINT_LOG_ERR("data_svc_udp_server_start_req no mem for netif:%d port:%d\r\n", p_req_msg->netif_id,
                             p_req_msg->local_port);
        return FALSE;
    }
}

/*
 *Function to supply buffers to the ring
 *@param len     :   len of the payload required
 *@return         :   TRUE if memory allocated successfully
 *                   FALSE if memory could not be allocated
 */
bool data_svc_get_udp_data_buff(void *p_element, uint16_t len)
{
    ring_element_t *p_elem = (ring_element_t *)p_element;

    struct pbuf *p_pbuf = pbuf_alloc(PBUF_RAW, len, PBUF_RAM);
    if (p_pbuf != NULL) {
        p_elem->p_buf_start = p_pbuf;
        p_elem->len = len;
        p_elem->p_buf = p_pbuf->payload;
        return TRUE;
    } else {
        return FALSE;
    }
}

/*
 *Free buffer used for udp receive
 *@param     :   pointer to ring element
 *@return    :   TRUE if buffer freed
 *           :   FALSE if buffer not freed
 */
bool data_svc_free_udp_data_buff(void *p_element)
{
    struct pbuf *p_pbuf = NULL;
    ring_element_t *p_elem = (ring_element_t *)p_element;

    p_pbuf = p_elem->p_buf_start;
    pbuf_free(p_pbuf);

    p_elem->p_buf_start = NULL;
    p_elem->p_buf = NULL;
    p_elem->len = 0;

    return TRUE;
}

/*
*Processs the ring element and try to send out data
*@param p_element   :   element of the ring
*@return            :   TRUE, if succesfully sent out
                    :   FALSE, if not able to send
*/

bool data_svc_process_udp_pkt(void *p_element)
{
    ring_element_t *p_elem = NULL;
    uint8_t conn_id = 0;
    conn_handler_t *conn_handler = NULL;
    err_t error_code = ERR_OK;

    p_elem = (ring_element_t *)p_element;
    conn_id = (uint8_t)p_elem->info;
    conn_handler = data_svc_get_conn_handler_from_id(conn_id);

    struct pbuf *p_pbuf = p_elem->p_buf_start;
    p_pbuf->len = p_elem->len;
    p_pbuf->tot_len = p_elem->len;

    if (NULL != conn_handler) {
        error_code = udp_send(conn_handler->pcb, p_elem->p_buf_start);

        if (error_code != ERR_OK) {
            RINGIF_PRINT_LOG_INFO("tcpip: udp send failed");
            return FALSE;
        }

        RINGIF_PRINT_LOG_INFO("tcpip: udp send succeeded conn_id:%d", conn_id);
        pbuf_free(p_elem->p_buf_start);
        return TRUE;
    } else {
        RINGIF_PRINT_LOG_ERR("tcpip: udp send failed, conn_id not recognized");
        pbuf_free(p_elem->p_buf_start);
        return FALSE;
    }
}
#endif  // SUPPORT_RING_IF
