/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*========================================================================
 *
 * @brief Function definitions for rawEth connections
 *=======================================================================*/

/*------------------------------------------------------------------------
 * Include Files
 * ----------------------------------------------------------------------*/
#include <stdlib.h>
#include "data_svc_priv.h"
#include "nt_logger_api.h"
#include "dpm_ip.h"
#include "prot/ethernet.h"
#include "data_path.h"
#include "network_al.h"
#include "ieee80211.h"

#if defined(SUPPORT_COEX)
#include "coex_wghts.h"
#endif
#ifdef SUPPORT_RAWETH_IPERF
#include "rawEth_iperf.h"
#endif /* SUPPORT_RAWETH_IPERF */

#ifdef SUPPORT_RING_IF

/*
 *Sets the dest MAC address from the netif in ethernet header
 *@param p_eth_hdr   :   pointer to Ethernet header
 *@param p_netif     :   pointer to network interface
 */

static void fill_dest_mac_addr(ethernet_header_t *p_eth_hdr, const uint8_t *p_dest_mac_addr)
{
    configASSERT(p_eth_hdr != NULL);
    configASSERT(p_dest_mac_addr);

    for (uint8_t idx = 0; idx < IEEE80211_ADDR_LENGTH; idx++) {
        p_eth_hdr->xDestinationAddress[idx] = p_dest_mac_addr[idx];
    }
}

/*
 *Sets the src MAC address in the ethernet header
 *@param p_eth_hdr   :   pointer to Ethernet header
 *@param p_netif     :   pointer to network interface
 */

static void fill_src_mac_addr(ethernet_header_t *p_eth_hdr, const struct netif *p_netif)
{
    configASSERT(p_netif != NULL);
    configASSERT(p_eth_hdr != NULL);

    for (uint8_t idx = 0; idx < IEEE80211_ADDR_LENGTH; idx++) {
        p_eth_hdr->xSourceAddress[idx] = p_netif->hwaddr[idx];
    }
}

/*
 *Processes a request to close a rawEth conn.Sends response to Apps.
 *@param req_msg :   The message buffer from Apps with all relevant fields filled
 *                   to start a raw eth conn
 *@return        :   TRUE if a response is successfully sent back to Apps
 *                   FALSE in case unable to send a response to Apps
 */
static bool raweth_close_conn_req(raweth_close_conn_req_t *p_req_msg)
{
    raweth_close_conn_cfm_t *p_rsp_msg = NULL;
    conn_handler_t *p_conn_handle = NULL;
    bool b_err = FALSE;
    err_t err_code = ERR_OK;

    p_rsp_msg = (raweth_close_conn_cfm_t *)ringif_get_ctrl_buf(0, NULL);

    if (p_rsp_msg != NULL) {
        p_conn_handle = data_svc_get_conn_handler_from_id(p_req_msg->conn_id);
        if (p_conn_handle != NULL) {
            b_err = ringif_delete_a2f_raweth_ring(p_conn_handle->a2f_ring_id);
            configASSERT(b_err);

            b_err = ringif_delete_f2a_raweth_ring(p_conn_handle->f2a_ring_id);
            configASSERT(b_err);
#ifdef SUPPORT_COEX
            coex_update_vbc_prio(FALSE, 0, p_conn_handle->netif_id);
#endif
            data_svc_free_conn_handler(p_req_msg->conn_id);
            err_code = ERR_OK;
        } else {
            err_code = ERR_ARG;
        }

        p_rsp_msg->api_hdr.api_id = RAW_ETH_CLOSE_CONN_CFM;
        p_rsp_msg->conn_id = p_req_msg->conn_id;
        p_rsp_msg->err_code = err_code;

        b_err = ringif_send_raweth_config((uint32_t *)p_rsp_msg, sizeof(raweth_close_conn_cfm_t));

        if (FALSE == b_err) {
            RINGIF_PRINT_LOG_ERR("raweth_close_conn_req fail due to no mem for rsp conn_id %d \r\n",
                                 p_req_msg->conn_id);
            ringif_free_ctrl_buf(p_rsp_msg, 0);
        } else {
            RINGIF_PRINT_LOG_ERR("Success: rsp msg - %d, error_code - %d, conn_id - %d\r\n", (uint32_t)p_rsp_msg,
                                 err_code, p_req_msg->conn_id);
        }
        return b_err;

    } else {
        RINGIF_PRINT_LOG_ERR("raweth_close_conn_req fail due to no mem for conn:%d\r\n", p_req_msg->conn_id);
        return FALSE;
    }
}

/*
 *Check if the requested connection already exists.
 *@param req_msg :   The message buffer from Apps with all relevant fields filled
 *                   to start a raw eth conn
 *@return        :   TRUE if connection already exists
 *                   FALSE if connection does not exist
 */

static bool raweth_existing_conn_chk(raweth_start_conn_req_t *p_req_msg)
{
    conn_handler_t *p_conn_handle = NULL;

    for (int idx = 0; idx < DATA_SVC_MAX_CONN; idx++) {
        p_conn_handle = data_svc_get_conn_handler_from_id(idx);
        if (NULL != p_conn_handle) {
            if (DATA_SVC_RAW_ETHER == p_conn_handle->type) {
                if ((p_conn_handle->netif_id == p_req_msg->netif_id) &&
                    (p_conn_handle->eth_type == p_req_msg->eth_type) &&
                    (MAC_EQUAL(p_conn_handle->dest_mac, p_req_msg->dest_mac))) {
                    return TRUE;
                }
            }
        }
    }

    return FALSE;
}

/*
 *Check if the requested MAC ID is valid
 *@param req_msg :   The message buffer from Apps with all relevant fields filled
 *                   to start the connection
 *@return        :   TRUE if MAC ID is valid
 *                   FALSE if MAC ID is not valid
 */
static bool raweth_valid_mac_id_chk(struct netif *p_netif, uint8 *dest_mac)
{
    if (TRUE == IEEE80211_IS_MULTICAST(dest_mac)) {
        RINGIF_PRINT_LOG_ERR("raweth_err: Mcast/Bcast MAC ID\r\n");
        return FALSE;
    }

    if (FALSE == nt_dpm_macid_connected_chk(p_netif, dest_mac)) {
        return FALSE;
    }

    return TRUE;
}

/*
 *Processes a request to open a rawEth conn.Sends response to Apps.
 *@param req_msg :   The message buffer from Apps with all relevant fields filled
 *                   to start a raw eth conn
 *@return        :   TRUE if a response is successfully sent back to Apps
 *                   FALSE in case unable to send a response to Apps
 */
static bool raweth_start_conn_req(raweth_start_conn_req_t *p_req_msg)
{
    raweth_start_conn_cfm_t *p_rsp_msg = NULL;
    err_t error_code = ERR_OK;
    conn_handler_t *p_conn_handle = NULL;
    struct netif *p_netif = NULL;
    bool b_err = FALSE;
    uint8_t a2f_ring_id = DATA_SVC_INVALID_RING_ID;
    uint32_t a2f_ring_addr = 0;
    uint16_t a2f_ring_num_elem = 0;
    uint8_t a2f_ring_elem_size = 0;
    uint8_t f2a_ring_id = DATA_SVC_INVALID_RING_ID;
    uint32_t f2a_ring_addr = 0;
    uint16_t f2a_ring_num_elem = 0;
    uint8_t f2a_ring_elem_size = 0;

    uint8_t conn_id = DATA_SVC_MAX_CONN;

    p_rsp_msg = (raweth_start_conn_cfm_t *)ringif_get_ctrl_buf(0, NULL);

    if (NULL == p_rsp_msg) {
        RINGIF_PRINT_LOG_ERR("raweth_start_conn_req fail due to no mem for p_rsp_msg\r\n");
        error_code = ERR_MEM;
        return FALSE;
    }

    while (1) {
        p_netif = netif_get_by_index(p_req_msg->netif_id);
        if (FALSE == nt_dpm_is_netif_ready(p_netif)) {
            RINGIF_PRINT_LOG_ERR("raweth_start_conn_req fail due to netif:%d not ready\r\n", p_req_msg->netif_id);
            error_code = ERR_CONN;
            break;
        }

        if (FALSE == raweth_valid_mac_id_chk(p_netif, p_req_msg->dest_mac)) {
            RINGIF_PRINT_LOG_ERR("raweth_start_conn_req fail...Unconnected/Invalid Destination MAC ID\r\n");
            error_code = ERR_CONN;
            break;
        }

        if (TRUE == raweth_existing_conn_chk(p_req_msg)) {
            RINGIF_PRINT_LOG_ERR("raweth_start_conn_req fail....conn already exists\r\n");
            error_code = ERR_CONN;
            break;
        }

        p_conn_handle = data_svc_get_new_conn_handler();
        if (NULL == p_conn_handle) {
            RINGIF_PRINT_LOG_ERR("raweth_start_conn_req fail....No mem for new connection\r\n");
            error_code = ERR_MEM;
            break;
        }

        // get the ring id, addr etc
        b_err = ringif_create_a2f_raweth_ring(&a2f_ring_id);
        if (TRUE != b_err) {
            RINGIF_PRINT_LOG_ERR("raweth_start_conn_req fail....A2F ring create fail\r\n");
            error_code = ERR_MEM;
            break;
        }

        b_err = ringif_create_f2a_raweth_ring(&f2a_ring_id);
        if (TRUE != b_err) {
            RINGIF_PRINT_LOG_ERR("raweth_start_conn_req fail....F2A ring create fail\r\n");
            error_code = ERR_MEM;
            break;
        }

        p_conn_handle->netif_id = p_req_msg->netif_id;
        p_conn_handle->type = DATA_SVC_RAW_ETHER;
        p_conn_handle->qos = p_req_msg->qos;
        p_conn_handle->eth_type = p_req_msg->eth_type;
#ifdef SUPPORT_COEX
        coex_update_vbc_prio(TRUE, p_conn_handle->qos, p_conn_handle->netif_id);
#endif
        for (int idx = 0; idx < IEEE80211_ADDR_LENGTH; idx++) {
            p_conn_handle->dest_mac[idx] = p_req_msg->dest_mac[idx];
        }
        p_conn_handle->a2f_ring_id = a2f_ring_id;
        p_conn_handle->f2a_ring_id = f2a_ring_id;
        conn_id = p_conn_handle->idx;

        f2a_ring_num_elem = ringif_f2a_num_ring_elems(f2a_ring_id);
        f2a_ring_addr = (uint32_t)ringif_f2a_ring_addr(f2a_ring_id);
        f2a_ring_elem_size = ringif_f2a_elem_size(f2a_ring_id);

        a2f_ring_num_elem = ringif_a2f_num_ring_elems(a2f_ring_id);
        a2f_ring_addr = (uint32_t)ringif_a2f_ring_addr(a2f_ring_id);
        a2f_ring_elem_size = ringif_a2f_elem_size(a2f_ring_id);

        error_code = ERR_OK;
        break;
    }

    p_rsp_msg->api_hdr.api_id = RAW_ETH_START_CONN_CFM;
    p_rsp_msg->netif_id = p_req_msg->netif_id;
    p_rsp_msg->ether_type = p_req_msg->eth_type;
    for (int idx = 0; idx < IEEE80211_ADDR_LENGTH; idx++) {
        p_rsp_msg->dest_mac[idx] = p_req_msg->dest_mac[idx];
    }
    p_rsp_msg->conn_id = conn_id;
    p_rsp_msg->err_code = error_code;
    p_rsp_msg->a2f_ring_id = a2f_ring_id;
    p_rsp_msg->f2a_ring_id = f2a_ring_id;
    p_rsp_msg->a2f_ring_num_elem = a2f_ring_num_elem;
    p_rsp_msg->a2f_ring_addr = a2f_ring_addr;
    p_rsp_msg->a2f_ring_elem_size = a2f_ring_elem_size;
    p_rsp_msg->f2a_ring_num_elem = f2a_ring_num_elem;
    p_rsp_msg->f2a_ring_addr = f2a_ring_addr;
    p_rsp_msg->f2a_ring_elem_size = f2a_ring_elem_size;

    if (ERR_OK != error_code) {
        if (DATA_SVC_INVALID_RING_ID != a2f_ring_id) {
            b_err = ringif_delete_a2f_raweth_ring(a2f_ring_id);
            configASSERT(b_err);
        }

        if (DATA_SVC_INVALID_RING_ID != f2a_ring_id) {
            b_err = ringif_delete_a2f_raweth_ring(a2f_ring_id);
            configASSERT(b_err);
        }
    }

    RINGIF_PRINT_LOG_ERR("Data A2F Ring(%d): Addr:%x NumElem:%d ElemSize:%d", p_rsp_msg->a2f_ring_id,
                         p_rsp_msg->a2f_ring_addr, p_rsp_msg->a2f_ring_num_elem, p_rsp_msg->a2f_ring_elem_size);
    RINGIF_PRINT_LOG_ERR("Data F2A Ring(%d): Addr:%x NumElem:%d ElemSize:%d", p_rsp_msg->f2a_ring_id,
                         p_rsp_msg->f2a_ring_addr, p_rsp_msg->f2a_ring_num_elem, p_rsp_msg->f2a_ring_elem_size);

    // inform ctrl ring
    b_err = ringif_send_raweth_config((uint32_t *)p_rsp_msg, sizeof(raweth_start_conn_cfm_t));

    if (FALSE == b_err) {
        RINGIF_PRINT_LOG_ERR("raweth_start_conn_req fail due to no mem for rsp for netif:%d\r\n", p_req_msg->netif_id);
        ringif_free_ctrl_buf(p_rsp_msg, 0);
        return FALSE;
    } else {
        RINGIF_PRINT_LOG_ERR("raweth_start_conn_req success eth_type:%x qos:%x\r\n", p_conn_handle->eth_type,
                             p_conn_handle->qos);
        return TRUE;
    }
}

/*
 *Check if a conn_id exists that has the required eth_type
 *@param eth_type    :   The ethernet header + payload
 *@return            :   conn_id of connection that has required eth_ytpe or
 *                       DATA_SVC_MAX_CONN if a conn_id with required eth_type
 *                           doesn't exist
 */
static conn_handler_t *get_conn_handle_from_eth_type(uint16_t eth_type, uint8_t *local_mac, uint8_t *remote_mac)
{
    conn_handler_t *p_conn_handle = NULL;
    struct netif *p_netif = NULL;

    for (uint8_t idx = 0; idx < DATA_SVC_MAX_CONN; idx++) {
        p_conn_handle = data_svc_get_conn_handler_from_id(idx);

        if (NULL != p_conn_handle) {
            p_netif = netif_get_by_index(p_conn_handle->netif_id);
            configASSERT(NULL != p_netif);

            if ((p_conn_handle->eth_type == eth_type) && (MAC_EQUAL(remote_mac, p_conn_handle->dest_mac)) &&
                (MAC_EQUAL(local_mac, p_netif->hwaddr))) {
                return p_conn_handle;
            }
        }
    }

    return p_conn_handle;
}

#ifdef SUPPORT_DATA_LOOPBACK
static void swap_dest_src_mac_address(ethernet_header_t *p_eth_hdr)
{
    configASSERT(p_eth_hdr != NULL) uint8_t temp = 0;

    for (int idx = 0; idx < IEEE80211_ADDR_LENGTH; idx++) {
        temp = p_eth_hdr->xSourceAddress[idx];
        p_eth_hdr->xSourceAddress[idx] = p_eth_hdr->xDestinationAddress[idx];
        p_eth_hdr->xDestinationAddress[idx] = temp;
    }
}

static err_t loopback_raweth_data(struct pbuf *p_pbuf, struct netif *p_netif)
{
    ethernet_header_t *p_eth_hdr = NULL;
    err_t error_code = ERR_OK;

    p_eth_hdr = (ethernet_header_t *)(p_pbuf->payload);

    if (!MAC_EQUAL(p_netif->hwaddr, p_eth_hdr->xDestinationAddress)) {
        uint8_t *p_dest_mac = p_eth_hdr->xDestinationAddress;

        RINGIF_PRINT_LOG_ERR("Dropping wrong addressed frame: %x %x %x %x %x %x", p_dest_mac[0], p_dest_mac[1],
                             p_dest_mac[2], p_dest_mac[3], p_dest_mac[4], p_dest_mac[5]);
        return ERR_RTE;
    }

    swap_dest_src_mac_address(p_eth_hdr);
    error_code = nt_low_level_output(p_netif, p_pbuf);

    if (error_code != ERR_OK) {
        uint8_t *p_dest_mac = p_eth_hdr->xDestinationAddress;
        uint32_t *p_payload32 = (uint32 *)((uint8_t *)p_eth_hdr + sizeof(ethernet_header_t));
        RINGIF_PRINT_LOG_ERR("Fail to loop back frame(1st word: %x) from:%x %x %x %x %x %x  due to err: %d",
                             p_payload32[0], p_dest_mac[0], p_dest_mac[1], p_dest_mac[2], p_dest_mac[3], p_dest_mac[4],
                             p_dest_mac[5], error_code);
        return error_code;
    } else {
        /* Free the buffer only for success cases since failed cases are handled later */
        pbuf_free(p_pbuf);
        return ERR_OK;
    }
}
#endif /* SUPPORT_DATA_LOOPBACK */

/*
 *Function to return Qos to be used from Tx pkt
 *@param p_tx_eth_hdr     :   Pointer to Tx Pkt Ethernet header
 *@return         :   QOS Value to be returned
 */
uint8_t data_svc_get_qos_for_non_ip(void *p_tx_eth_hdr)
{
    ethernet_header_t *p_eth_hdr = p_tx_eth_hdr;
    conn_handler_t *p_conn_handle = NULL;

    p_conn_handle = get_conn_handle_from_eth_type(PP_HTONS(p_eth_hdr->usFrameType), p_eth_hdr->xSourceAddress,
                                                  p_eth_hdr->xDestinationAddress);

    if (p_conn_handle != NULL) {
        return p_conn_handle->qos;
    } else {
        RINGIF_PRINT_LOG_INFO("No conn found for EthType:%x Local:%x Remote:%x", PP_HTONS(p_eth_hdr->usFrameType),
                              p_eth_hdr->xSourceAddress[5], p_eth_hdr->xDestinationAddress[5]);
        return MAX_AC_NUM;
    }
}

/*
 *Function to return TID to be used given QoS
 *@param qos_for_non_ip  :   QoS used for Tx pkt
 *@return         :   TID value to be returned
 */
uint8_t data_svc_default_tid_from_qos(uint8_t qos_for_non_ip)
{
    switch (qos_for_non_ip) {
        case AC_BK:
            return 1;
        case AC_VI:
            return 4;
        case AC_VO:
            return 6;
        default:
            return 0;
    }
}

/*
 *Function to return TID to be used from Tx pkt
 *@param p_tx_eth_hdr     :   Pointer to Tx Pkt Ethernet header
 *@return         :   TID value to be returned
 */
uint8_t data_svc_get_tid_for_non_ip(void *p_tx_eth_hdr)
{
    uint8_t qos_for_non_ip = data_svc_get_qos_for_non_ip(p_tx_eth_hdr);
    return data_svc_default_tid_from_qos(qos_for_non_ip);
}

/*
 *Function to supply buffers to the ring
 *@param len     :   len of the payload required
 *@return         :   TRUE if memory allocated successfully
 *                   FALSE if memory could not be allocated
 */

bool data_svc_get_rawEth_data_buff(void *p_element, uint16_t len)
{
    configASSERT(NULL != p_element);

    ring_element_t *p_elem = (ring_element_t *)p_element;
    uint8_t *payload = NULL;
    uint16_t payload_len = len + sizeof(ethernet_header_t);

    struct pbuf *p_pbuf = pbuf_alloc(PBUF_RAW, payload_len, PBUF_RAM);

    if (p_pbuf != NULL) {
        p_elem->p_buf_start = p_pbuf;
        p_elem->len = len;

        configASSERT(NULL != p_pbuf->payload) payload = (uint8_t *)p_pbuf->payload + sizeof(ethernet_header_t);
        p_elem->p_buf = (uint32_t *)payload;

        return TRUE;
    } else {
        return FALSE;
    }
}

/*
 *Free buffer used for rawEth receive
 *@param     :   pointer to ring element
 *@return    :   TRUE if buffer freed
 *           :   FALSE if buffer not freed
 */

bool data_svc_free_rawEth_data_buff(void *p_element)
{
    configASSERT(NULL != p_element);

    struct pbuf *p_pbuf = NULL;
    ring_element_t *p_elem = (ring_element_t *)p_element;

    p_pbuf = p_elem->p_buf_start;
    pbuf_free(p_pbuf);

    p_elem->p_buf = NULL;
    p_elem->p_buf_start = NULL;

    return TRUE;
}

/*
 *Callback for data packets with unknown ether type. Check if ether type
 *is valid and pass on to Apps
 *@param p_pbuf  :   The ethernet header + payload
 *@param p_netif :   Network interface one which p_pbuf arrived
 *@return        :   ERR_OK if eth_type recognized and sent to Apps
 *                   error code if failuer occurs
 */
err_t data_svc_recv_raweth_data_pkt(struct pbuf *p_pbuf, struct netif *p_netif)
{
    ethernet_header_t *p_eth_hdr = NULL;
    uint8_t conn_id = DATA_SVC_MAX_CONN;
    ;
    uint8_t *p_payload = NULL;
    conn_handler_t *p_conn_handle = NULL;
    bool b_result = FALSE;
    uint16_t len = 0;
    err_t error_code = ERR_OK;
    uint16_t eth_type;

    if (NULL == p_pbuf || NULL == p_netif) {
        return ERR_ARG;
    }

    p_eth_hdr = (ethernet_header_t *)(p_pbuf->payload);
    eth_type = PP_HTONS(p_eth_hdr->usFrameType);

#ifdef SUPPORT_DATA_LOOPBACK
    if (TRUE == data_svc_get_loopback_state()) {
        error_code = loopback_raweth_data(p_pbuf, p_netif);
        return error_code;
    }
#endif /* SUPPORT_DATA_LOOPBACK */

    p_conn_handle = get_conn_handle_from_eth_type(eth_type, p_eth_hdr->xDestinationAddress, p_eth_hdr->xSourceAddress);
    if (p_conn_handle != NULL) {
#ifdef SUPPORT_RAWETH_IPERF
        if (TRUE == is_raweth_perf_test_in_progress()) {
            nt_raweth_server_recv(p_conn_handle, p_pbuf);
        } else
#endif /* SUPPORT_RAWETH_IPERF */
        {
            conn_id = p_conn_handle->idx;
            p_payload = (uint8_t *)p_eth_hdr + sizeof(ethernet_header_t);
            len = p_pbuf->len - sizeof(ethernet_header_t);

            b_result = ringif_f2a_pkt_attach(p_conn_handle->f2a_ring_id, (uint32_t *)p_pbuf, (uint32_t *)p_payload, len,
                                             (uint16_t)conn_id);
            if (FALSE == b_result) {
                RINGIF_PRINT_LOG_INFO("dropping rawEth pkt as ring full");
                return ERR_MEM;
            }
            RINGIF_PRINT_LOG_INFO("Rx Pkt Sent to ring(%d) len:%d conn:%d ", p_conn_handle->f2a_ring_id, len, conn_id);
        }
    } else {
        error_code = ERR_ARG;
        uint8_t *p_local_mac = p_eth_hdr->xDestinationAddress;
        uint8_t *p_remote_mac = p_eth_hdr->xSourceAddress;

#ifdef SUPPORT_RAWETH_IPERF
        if ((int32_t)ntohl(*(u32_t *)(p_pbuf->payload + sizeof(ethernet_header_t))) < 0) {
            RINGIF_PRINT_LOG_INFO("End of Iperf Packet pbuf_len %d, %d eth_type", p_pbuf->len, eth_type);
            return error_code;
        }
#endif

        RINGIF_PRINT_LOG_ERR("Dropping Rx pkt (len:%d EthType:%x) as No conn exists", p_pbuf->len, eth_type);
        RINGIF_PRINT_LOG_ERR("Local MAC: %x %x %x %x %x %x", p_local_mac[0], p_local_mac[1], p_local_mac[2],
                             p_local_mac[3], p_local_mac[4], p_local_mac[5]);
        RINGIF_PRINT_LOG_ERR("Remote MAC: %x %x %x %x %x %x", p_remote_mac[0], p_remote_mac[1], p_remote_mac[2],
                             p_remote_mac[3], p_remote_mac[4], p_remote_mac[5]);
    }

    return error_code;
}

/*
*Processs the ring element and try to send out data
*@param p_element   :   element of the ring
*@return            :   TRUE, if succesfully sent out
                    :   FALSE, if not able to send
*/

bool data_svc_process_raweth_pkt(void *p_element)
{
    RINGIF_PRINT_LOG_INFO("tcpip: Processing rawEth packet");
    ring_element_t *p_elem = NULL;
    uint8_t conn_id = 0;
    conn_handler_t *p_conn_handle = NULL;
    err_t error_code = ERR_OK;
    struct netif *p_netif = NULL;
    ethernet_header_t *p_eth_hdr;

    p_elem = (ring_element_t *)p_element;
    conn_id = (uint8_t)p_elem->info;
    p_conn_handle = data_svc_get_conn_handler_from_id(conn_id);

    struct pbuf *p_pbuf = p_elem->p_buf_start;
    p_pbuf->len = p_elem->len + sizeof(ethernet_header_t);
    p_pbuf->tot_len = p_elem->len + sizeof(ethernet_header_t);

    if (NULL != p_conn_handle) {
        p_netif = netif_get_by_index(p_conn_handle->netif_id);
        if (p_netif != NULL) {
            // fill in the ethernet header
            p_eth_hdr = (ethernet_header_t *)(p_pbuf->payload);
            configASSERT(p_eth_hdr);
            p_eth_hdr->usFrameType = PP_HTONS(p_conn_handle->eth_type);
            fill_src_mac_addr(p_eth_hdr, p_netif);
            fill_dest_mac_addr(p_eth_hdr, p_conn_handle->dest_mac);

            error_code = nt_low_level_output(p_netif, p_elem->p_buf_start);
        } else {
            RINGIF_PRINT_LOG_ERR("tcpip: rawEth send failed, netif_id not recognized");
            pbuf_free(p_elem->p_buf_start);
            return TRUE;
        }

        if (error_code != ERR_OK) {
            RINGIF_PRINT_LOG_INFO("tcpip: rawEth send failed");
            return FALSE;
        }

        RINGIF_PRINT_LOG_INFO("tcpip: rawEth send succeeded");
        pbuf_free(p_elem->p_buf_start);
        return TRUE;
    } else {
        RINGIF_PRINT_LOG_ERR("tcpip: rawEth send failed, conn_id not recognized");
        pbuf_free(p_elem->p_buf_start);
        return TRUE;
    }
}

/*
 *Start processing the message from ring. Check api id of
 *message and call correct function to begin processing
 *the request from Apps
 *
 *@param p_buff    :   buffer containing the request message
 *@param len     :   length of data written into buffer
 *
 *@return        :   TRUE if able to process the packet
 *                   FALSE on failure to process correctly
 */

bool data_svc_process_raweth_config_pkt(void *p_buff, uint16_t len)
{
    lwip_svc_api_hdr_t *p_api_hdr = NULL;
    uint8_t api_id;

    p_api_hdr = (lwip_svc_api_hdr_t *)p_buff;
    api_id = p_api_hdr->api_id;

    // Check API Id of the msg and call corresponding function
    switch (api_id) {
        case RAW_ETH_START_CONN_REQ:
            if (len != sizeof(raweth_start_conn_req_t)) {
                RINGIF_PRINT_LOG_ERR("Length of request is incorrect");
                return FALSE;
            }
            raweth_start_conn_req((raweth_start_conn_req_t *)p_buff);
            break;
        case RAW_ETH_CLOSE_CONN_REQ:
            if (len != sizeof(raweth_close_conn_req_t)) {
                RINGIF_PRINT_LOG_ERR("Length of request is incorrect");
                return FALSE;
            }
            raweth_close_conn_req((raweth_close_conn_req_t *)p_buff);
            break;
        default:
            RINGIF_PRINT_LOG_ERR("invalid api_id");
            break;
    }

    return TRUE;
}

#endif  // SUPPORT_RING_IF
