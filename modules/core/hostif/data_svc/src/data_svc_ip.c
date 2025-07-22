/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*========================================================================
 *
 * @brief Function definitions for IP layer connections
 *=======================================================================*/

/*------------------------------------------------------------------------
 * Include Files
 * ----------------------------------------------------------------------*/
#include <stdlib.h>
#include "ip_addr.h"
#include "ip4_addr.h"
#include "err.h"
#include "netif.h"
#include "tcp.h"
#include "udp.h"
#include "dhcp.h"
#include "prot/dhcp.h"
#include "dhcp6.h"
#include "prot/dhcp6.h"
#include "ip4_addr.h"
#include "autoip.h"

#include "nt_osal.h"
#include "nt_common.h"
#include "nt_logger_api.h"
#include "data_svc_udp_priv.h"
#include "data_svc_tcp_priv.h"
#include "ring_svc_api.h"
#include "network_al.h"

#ifdef SUPPORT_RING_IF
/*
*Indicates to apps the IP address that has been allocated for the network interface
*@param netif_id    :   id of the network interface that has been assigned an IP address
*@param dhcp_type   :   Whether the Static IP/ DHCP dynamic/ Local
*@param ip_ver      :   Whether the IP address is IPv4/IPv6
*@param addr_upper1 :   first 2 octets of ipv6 addr or ipv4
*@param addr_upper2 :   second 2 octets of ipv6 addr
*@param addr_lower1 :   third 2 octets of ipv6 addr
*@param addr_lower2 :   fourth 2 octets of ipv6 addr
*@return            :   TRUE if reponse sent successfully
                    :   FALSE if not successful

*/
static bool data_svc_ip_addr_ready_ind(uint8_t netif_id, uint8_t dhcp_type, uint8_t ip_ver, uint32_t ipv4_addr,
                                       const uint32_t *ipv6_addr)
{
    ip_addr_ready_ind_t *p_rsp_msg;
    p_rsp_msg = (ip_addr_ready_ind_t *)ringif_get_ctrl_buf(0, NULL);
    if (NULL == p_rsp_msg) {
        return FALSE;
    }

    bool b_err = FALSE;
    p_rsp_msg->api_hdr.api_id = IP_ADDR_READY_IND;
    p_rsp_msg->netif_id = netif_id;
    p_rsp_msg->dhcp_type = dhcp_type;
    p_rsp_msg->ip_ver = ip_ver;
    p_rsp_msg->ipv4_addr = ipv4_addr;
    if (NULL != ipv6_addr) {
        p_rsp_msg->ipv6_addr[0] = ipv6_addr[0];
        p_rsp_msg->ipv6_addr[1] = ipv6_addr[1];
        p_rsp_msg->ipv6_addr[2] = ipv6_addr[2];
        p_rsp_msg->ipv6_addr[3] = ipv6_addr[3];
    }

    b_err = ringif_send_ip_conn_config((uint32_t *)p_rsp_msg, sizeof(ip_addr_ready_ind_t));

    if (dhcp_type != RINGIF_IP_TYPE_STATIC) {
        extern void nt_show_ip(void);
        nt_show_ip();
    }

    return b_err;
}

/*
 * Handles the link change indication from netif
 *@param p_netif     :   Pointer to the netif structure
 *@return None
 */
static void data_svc_netif_link_change_hdlr(struct netif *p_netif)
{
    if (p_netif == NULL) {
        NT_LOG_PRINT(COMMON, ERR, "Err: netif NULL\r\n");
        return;
    }

    if (netif_is_link_up(p_netif)) {
        NT_LOG_PRINT(COMMON, INFO, "netif link UP\r\n");

        if (RINGIF_IP_TYPE_STATIC == nt_dpm_get_ip_type(p_netif)) {
            uint8_t netif_id = netif_get_index(p_netif);
            nt_dpm_enable_disable_dhcp(p_netif, 0, TURN_OFF_DHCP, 0);

            if (IPADDR_TYPE_V4 == nt_dpm_get_ip_ver(p_netif)) {
                uint32_t ip_32 = ip_addr_get_ip4_u32(&p_netif->ip_addr);
                data_svc_ip_addr_ready_ind(netif_id, RINGIF_IP_TYPE_STATIC, RINGIF_IP_VER_V4, ip_32, NULL);
                NT_LOG_PRINT(COMMON, ERR, "Static IP v4 indication sent %d %d %d\r\n", netif_id, RINGIF_IP_TYPE_STATIC,
                             RINGIF_IP_VER_V4);
            } else {
                const ip6_addr_t *p_ipv6 = netif_ip6_addr(p_netif, 1);
                data_svc_ip_addr_ready_ind(netif_id, RINGIF_IP_TYPE_STATIC, RINGIF_IP_VER_V6, 0, p_ipv6->addr);
                NT_LOG_PRINT(COMMON, ERR, "Static IP v6 indication sent %d %d %d\r\n", netif_id, RINGIF_IP_TYPE_STATIC,
                             RINGIF_IP_VER_V4);
            }
        } else if (RINGIF_IP_TYPE_DYNAMIC == nt_dpm_get_ip_type(p_netif)) {
            nt_dpm_enable_disable_dhcp(p_netif, 0, TURN_ON_DHCP, 0);
            NT_LOG_PRINT(COMMON, ERR, "DHCP Kicked in from data_svc_netif_link_change\r\n");
        } else {
            NT_LOG_PRINT(COMMON, ERR, "TBD_TBD_TBD: Kick-in Local IP creation \r\n");
        }
    } else {
        NT_LOG_PRINT(COMMON, ERR, "TBD: Link down. Delete all IP connections\r\n");
    }
}

/*
 *Call back function when network interface is updated. Used to identify
 *when the IP address of the network interface has been allocated
 *@param netif   :   The netif structure that has been updated
 */

static void data_svc_netif_dhcp_hdlr(struct netif *p_netif)
{
    if (p_netif != NULL) {
        NT_LOG_PRINT(COMMON, INFO, "DHCP Handlr\r\n");

        if (RINGIF_IP_TYPE_STATIC == nt_dpm_get_ip_type(p_netif)) {
            NT_LOG_PRINT(COMMON, ERR, "Warn: Static IP set by Host, not sending DHCP ind\r\n");
            return;
        }
        // Check for DHCP assigned address
        else if ((netif_dhcp_data(p_netif) != NULL)) {
            // handle dhcp4 ip address
            uint8_t netif_id;
            uint8_t supplied;
            uint32_t ip_32;

            netif_id = netif_get_index(p_netif);

            // check if IP is available
            supplied = dhcp_supplied_address(p_netif);
            if (supplied == 1) {
                ip_addr_t *p_ip;
                p_ip = &(p_netif->ip_addr);
                ip_32 = ip_addr_get_ip4_u32(p_ip);
                data_svc_ip_addr_ready_ind(netif_id, RINGIF_IP_TYPE_DYNAMIC, RINGIF_IP_VER_V4, ip_32, NULL);
                NT_LOG_PRINT(COMMON, ERR, "DHCP v4 indication sent\r\n");
                return;
            } else {
                NT_LOG_PRINT(COMMON, ERR, "DHCP v4 indication FAIL\r\n");
            }

        } else if ((netif_dhcp6_data(p_netif) != NULL)) {
            // handle dhcp6 IP
            uint8_t netif_id;
            struct dhcp6 *dhcp_struct;

            netif_id = netif_get_index(p_netif);

            // check if IP is available
            dhcp_struct = netif_dhcp6_data(p_netif);

            if ((dhcp_struct->state == DHCP_STATE_BOUND) || (dhcp_struct->state == DHCP_STATE_RENEWING) ||
                (dhcp_struct->state == DHCP_STATE_REBINDING)) {
                const ip6_addr_t *p_ipv6;

                // TBD: check how this work since there are 3 ipv6 addresses?
                p_ipv6 = netif_ip6_addr(p_netif, 1);

                data_svc_ip_addr_ready_ind(netif_id, RINGIF_IP_TYPE_DYNAMIC, RINGIF_IP_VER_V6, 0, p_ipv6->addr);
                NT_LOG_PRINT(COMMON, ERR, "DHCP v6 indication sent\r\n");
                return;
            } else {
                NT_LOG_PRINT(COMMON, ERR, "DHCP v6 indication FAIL\r\n");
            }
        }

        // check for ipv4 link local address
        if (netif_autoip_data(p_netif) != NULL) {
            uint8_t netif_id;
            struct autoip *p_autoip_struct;
            ip4_addr_t ll_addr;
            netif_id = netif_get_index(p_netif);
            p_autoip_struct = netif_autoip_data(p_netif);
            ll_addr = p_autoip_struct->llipaddr;

            data_svc_ip_addr_ready_ind(netif_id, RINGIF_IP_TYPE_LOCAL, RINGIF_IP_VER_V4, ll_addr.addr, NULL);
            NT_LOG_PRINT(COMMON, ERR, "LINK LOCAL v4 indication SENT\r\n");
        }
        // check ipv6 link local address
        else {
            // link local address for ipv6 is allways stored in index 0
            uint8_t netif_id;
            const ip6_addr_t *p_ipv6;
            netif_id = netif_get_index(p_netif);
            p_ipv6 = netif_ip6_addr(p_netif, 0);
            data_svc_ip_addr_ready_ind(netif_id, RINGIF_IP_TYPE_LOCAL, RINGIF_IP_VER_V6, 0, p_ipv6->addr);
            NT_LOG_PRINT(COMMON, ERR, "LINK LOCAL v6 indication SENT\r\n");
            return;
        }
    }
}

/*
 * Posts link change event to TCP/IP Thread
 *@param p_netif     :   Pointer to the netif structure
 *@return None
 */
void data_svc_netif_link_change(struct netif *p_netif)
{
    if (ERR_OK == tcpip_try_callback((tcpip_callback_fn)data_svc_netif_link_change_hdlr, p_netif)) {
        NT_LOG_PRINT(COMMON, INFO, "Link change posted to TCP/IP\r\n");
    } else {
        NT_LOG_PRINT(COMMON, ERR, "Link change post to TCP/IP FAIL\r\n");
    }
}

/*
 *Call back function when network interface is updated. Used to identify
 *when the IP address of the network interface has been allocated
 *@param netif   :   The netif structure that has been updated
 */

void data_svc_netif_callback_function(struct netif *p_netif)
{
    if (ERR_OK == tcpip_try_callback((tcpip_callback_fn)data_svc_netif_dhcp_hdlr, p_netif)) {
        NT_LOG_PRINT(COMMON, INFO, "DHCP update posted to TCP/IP\r\n");
    } else {
        NT_LOG_PRINT(COMMON, ERR, "DHCP update post to TCP/IP FAIL\r\n");
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
bool data_svc_process_ip_config_pkt(void *p_buff, uint16_t len)
{
    lwip_svc_api_hdr_t *p_api_hdr = NULL;
    uint8_t api_id;

    p_api_hdr = (lwip_svc_api_hdr_t *)p_buff;
    api_id = p_api_hdr->api_id;

    // Check API Id of the msg and call corresponding function
    switch (api_id) {
        case IP_TCP_LISTEN_START_REQ:
            if (len != sizeof(ip_tcp_listen_start_req_t)) {
                return FALSE;
            }
            data_svc_tcp_listen_start_req((ip_tcp_listen_start_req_t *)p_buff);
            break;

        case IP_TCP_LISTEN_CLOSE_REQ:
            if (len != sizeof(ip_tcp_listen_close_req_t)) {
                return FALSE;
            }
            data_svc_tcp_listen_close_req((ip_tcp_listen_close_req_t *)p_buff);
            break;

        case IP_TCP_CLIENT_CONNECT_REQ:
            if (len != sizeof(ip_tcp_client_connect_req_t)) {
                return FALSE;
            }
            data_svc_tcp_client_connect_req((ip_tcp_client_connect_req_t *)p_buff);
            break;

        case IP_TCP_CONNECTION_CLOSE_REQ:
            if (len != sizeof(ip_tcp_connection_close_req_t)) {
                return FALSE;
            }
            data_svc_tcp_connection_close_req((ip_tcp_connection_close_req_t *)p_buff);
            break;

        case IP_TCP_CLOSED_RSP:
            if (len != sizeof(ip_tcp_closed_rsp_t)) {
                return FALSE;
            }
            data_svc_tcp_closed_rsp((ip_tcp_closed_rsp_t *)p_buff);
            break;

        case IP_UDP_SERVER_START_REQ:
            if (len != sizeof(ip_udp_server_start_req_t)) {
                return FALSE;
            }
            data_svc_udp_server_start_req((ip_udp_server_start_req_t *)p_buff);
            break;

        case IP_UDP_CLIENT_START_REQ:
            if (len != sizeof(ip_udp_client_start_req_t)) {
                return FALSE;
            }
            data_svc_udp_client_start_req((ip_udp_client_start_req_t *)p_buff);
            break;

        case IP_UDP_CLOSE_REQ:
            if (len != sizeof(ip_udp_close_req_t)) {
                return FALSE;
            }
            data_svc_udp_close_req((ip_udp_close_req_t *)p_buff);
            break;

        default:
            NT_LOG_WMI_INFO("invalid api_id", 0, 0, 0);
            break;
    }

    return TRUE;
}

#endif  // SUPPORT_RING_IF
