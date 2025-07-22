/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*========================================================================
 * @file wifi_fw_ip_conn_api.h
 * @brief IP Connection related APIs/Defs to be shared with Apps
 *========================================================================*/
#ifndef WIFI_FW_IP_CONN_API_H
#define WIFI_FW_IP_CONN_API_H

/*------------------------------------------------------------------------
 * Include Files
 * ----------------------------------------------------------------------*/
#include "wifi_fw_cmn_api.h"

/*------------------------------------------------------------------------
 * Type Declarations
 * ----------------------------------------------------------------------*/

// API ID LIST

typedef enum api_req_id {
    IP_TCP_LISTEN_START_REQ = 0,
    IP_TCP_LISTEN_CLOSE_REQ = 1,
    IP_TCP_CLIENT_CONNECT_REQ = 2,
    IP_TCP_CONNECTION_CLOSE_REQ = 3,
    IP_TCP_CLOSED_RSP = 4,
    IP_UDP_SERVER_START_REQ = 5,
    IP_UDP_CLIENT_START_REQ = 6,
    IP_UDP_CLOSE_REQ = 7,
    RAW_ETH_START_CONN_REQ = 8,
    RAW_ETH_CLOSE_CONN_REQ = 9,
} api_req_id_t;

typedef enum api_rsp_id {
    IP_TCP_LISTEN_START_CFM = 0,
    IP_TCP_CLIENT_ACCEPTED_IND = 1,
    IP_TCP_LISTEN_CLOSE_CFM = 2,
    IP_TCP_CLIENT_CONNECT_CFM = 3,
    IP_TCP_CLIENT_CONNECT_IND = 4,
    IP_TCP_CONNECTION_CLOSE_CFM = 5,
    IP_TCP_SENT_IND = 6,
    IP_TCP_CLOSED_IND = 7,
    IP_UDP_SERVER_START_CFM = 8,
    IP_UDP_CLIENT_START_CFM = 9,
    IP_UDP_CLOSE_CFM = 10,
    IP_UDP_NEW_REMOTE_IP_IND = 11,
    IP_ADDR_READY_IND = 12,
    RAW_ETH_START_CONN_CFM = 13,
    RAW_ETH_CLOSE_CONN_CFM = 14,
} api_rsp_id_t;

// API STRUCTURE - REQUESTS:

typedef struct lwip_svc_api_hdr_s {
    uint8_t api_id;
    uint8_t reserved[Reserve_24_BIT];
} lwip_svc_api_hdr_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(lwip_svc_api_hdr_t)
WIFI_FW_STRUCT_SIZE_SYNC(lwip_svc_api_hdr_t, 4)

typedef struct ip_tcp_listen_start_req_s {
    lwip_svc_api_hdr_t api_hdr;
    uint8_t netif_id;
    uint8_t backlog;
    uint16_t local_port;
    uint16_t flags;
    uint8_t tls_params;
    uint8_t reserved;
} ip_tcp_listen_start_req_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(ip_tcp_listen_start_req_t)
WIFI_FW_STRUCT_SIZE_SYNC(ip_tcp_listen_start_req_t, 12)

typedef struct ip_tcp_listen_close_req_s {
    lwip_svc_api_hdr_t api_hdr;
    uint8_t listen_id;
    uint8_t reserved[Reserve_24_BIT];
} ip_tcp_listen_close_req_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(ip_tcp_listen_close_req_t)
WIFI_FW_STRUCT_SIZE_SYNC(ip_tcp_listen_close_req_t, 8)

typedef struct ip_tcp_client_connect_req_s {
    lwip_svc_api_hdr_t api_hdr;

    uint8_t netif_id;
    uint8_t tls_params;
    uint8_t flags;
    uint8_t ip_ver;

    uint32_t remote_ipv4_addr;    /* If ip_ver is IP_VER_V4 */
    uint32_t remote_ipv6_addr[4]; /* If ip_ver is IP_VER_V6 */

    uint16_t local_port;
    uint16_t remote_port;
} ip_tcp_client_connect_req_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(ip_tcp_client_connect_req_t)
WIFI_FW_STRUCT_SIZE_SYNC(ip_tcp_client_connect_req_t, 32)

typedef struct ip_tcp_connection_close_req_s {
    lwip_svc_api_hdr_t api_hdr;
    uint8_t conn_id;
    uint8_t reserved[Reserve_24_BIT];
} ip_tcp_connection_close_req_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(ip_tcp_connection_close_req_t)
WIFI_FW_STRUCT_SIZE_SYNC(ip_tcp_connection_close_req_t, 8)

typedef struct ip_tcp_closed_rsp_s {
    lwip_svc_api_hdr_t api_hdr;
    uint8_t conn_id;
    uint8_t reserved[Reserve_24_BIT];
} ip_tcp_closed_rsp_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(ip_tcp_closed_rsp_t)
WIFI_FW_STRUCT_SIZE_SYNC(ip_tcp_closed_rsp_t, 8)

typedef struct ip_udp_server_start_req_s {
    lwip_svc_api_hdr_t api_hdr;
    uint8_t netif_id;
    uint8_t tls_params;
    uint16_t local_port;
    uint16_t flags;
    uint16_t reserved;
} ip_udp_server_start_req_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(ip_udp_server_start_req_t)
WIFI_FW_STRUCT_SIZE_SYNC(ip_udp_server_start_req_t, 12)

typedef struct ip_udp_client_start_req_s {
    lwip_svc_api_hdr_t api_hdr;

    uint8_t netif_id;
    uint8_t tls_params;
    uint8_t flags;
    uint8_t ip_ver;

    uint32_t remote_ipv4_addr;    /* If ip_ver is IP_VER_V4 */
    uint32_t remote_ipv6_addr[4]; /* If ip_ver is IP_VER_V6 */

    uint16_t local_port;
    uint16_t remote_port;
} ip_udp_client_start_req_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(ip_udp_client_start_req_t)
WIFI_FW_STRUCT_SIZE_SYNC(ip_udp_client_start_req_t, 32)

typedef struct ip_udp_close_req_s {
    lwip_svc_api_hdr_t api_hdr;
    uint8_t conn_id;
    uint8_t listen_id;
    uint16_t reserved;

} ip_udp_close_req_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(ip_udp_close_req_t)
WIFI_FW_STRUCT_SIZE_SYNC(ip_udp_close_req_t, 8)

typedef struct raweth_start_conn_req_s {
    lwip_svc_api_hdr_t api_hdr;
    uint8_t netif_id;
    uint8_t qos;
    uint16_t eth_type;
    uint16_t flags;
    uint16_t reserved1;
    uint32_t reserved2;
    uint8_t dest_mac[IEEE80211_ADDR_LENGTH];
    uint16_t reserved3;
} raweth_start_conn_req_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(raweth_start_conn_req_t)
WIFI_FW_STRUCT_SIZE_SYNC(raweth_start_conn_req_t, 24)

typedef struct raweth_close_conn_req_s {
    lwip_svc_api_hdr_t api_hdr;
    uint8_t conn_id;
    uint8_t reserved[Reserve_24_BIT];
} raweth_close_conn_req_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(raweth_close_conn_req_t)
WIFI_FW_STRUCT_SIZE_SYNC(raweth_close_conn_req_t, 8)

// API STRUCTURE - RESPONSES:

typedef struct ip_tcp_listen_start_cfm_s {
    lwip_svc_api_hdr_t api_hdr;
    uint8_t netif_id;
    int8_t err_code;
    uint16_t local_port;
    uint8_t listen_id;
    uint8_t reserved[Reserve_24_BIT];
} ip_tcp_listen_start_cfm_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(ip_tcp_listen_start_cfm_t)
WIFI_FW_STRUCT_SIZE_SYNC(ip_tcp_listen_start_cfm_t, 12)

typedef struct ip_tcp_client_accepted_ind_s {
    lwip_svc_api_hdr_t api_hdr;
    int8_t err_code;
    uint8_t reserved;
    uint8_t conn_id;
    uint8_t listen_id;
    uint8_t a2f_ring_id;
    uint8_t f2a_ring_id;
    uint8_t a2f_ring_elem_size;
    uint8_t f2a_ring_elem_size;
    uint16_t a2f_ring_num_elem;
    uint16_t f2a_ring_num_elem;
    uint32_t a2f_ring_addr;
    uint32_t f2a_ring_addr;
    uint8_t ip_ver;
    uint8_t reserved2;
    uint16_t remote_port;
    uint32_t remote_ipv4_addr;    /* If ip_ver is IP_VER_V4 */
    uint32_t remote_ipv6_addr[4]; /* If ip_ver is IP_VER_V6 */
} ip_tcp_client_accepted_ind_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(ip_tcp_client_accepted_ind_t)
WIFI_FW_STRUCT_SIZE_SYNC(ip_tcp_client_accepted_ind_t, 48)

typedef struct ip_tcp_listen_close_cfm_s {
    lwip_svc_api_hdr_t api_hdr;
    uint8_t listen_id;
    int8_t err_code;
    uint16_t reserved;
} ip_tcp_listen_close_cfm_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(ip_tcp_listen_close_cfm_t)
WIFI_FW_STRUCT_SIZE_SYNC(ip_tcp_listen_close_cfm_t, 8)

typedef struct ip_tcp_client_connect_cfm_s {
    lwip_svc_api_hdr_t api_hdr;
    uint8_t netif_id;
    int8_t err_code;
    uint16_t port;
    uint8_t conn_id;
    uint8_t reserved[Reserve_24_BIT];
} ip_tcp_client_connect_cfm_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(ip_tcp_client_connect_cfm_t)
WIFI_FW_STRUCT_SIZE_SYNC(ip_tcp_client_connect_cfm_t, 12)

typedef struct ip_tcp_client_connect_ind_s {
    lwip_svc_api_hdr_t api_hdr;
    uint8_t conn_id;
    int8_t error_code;
    uint8_t netif_id;
    uint8_t reserved;
    uint16_t port;
    uint16_t reserved2;
    uint8_t a2f_ring_id;
    uint8_t f2a_ring_id;
    uint8_t a2f_ring_elem_size;
    uint8_t f2a_ring_elem_size;
    uint16_t a2f_ring_num_elem;
    uint16_t f2a_ring_num_elem;
    uint32_t a2f_ring_addr;
    uint32_t f2a_ring_addr;
} ip_tcp_client_connect_ind_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(ip_tcp_client_connect_ind_t)
WIFI_FW_STRUCT_SIZE_SYNC(ip_tcp_client_connect_ind_t, 28)

typedef struct ip_tcp_connection_close_cfm_s {
    lwip_svc_api_hdr_t api_hdr;
    uint8_t conn_id;
    int8_t err_code;
    uint16_t reserved;
} ip_tcp_connection_close_cfm_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(ip_tcp_connection_close_cfm_t)
WIFI_FW_STRUCT_SIZE_SYNC(ip_tcp_connection_close_cfm_t, 8)

typedef struct ip_tcp_sent_ind_s {
    lwip_svc_api_hdr_t api_hdr;
    uint8_t conn_id;
    uint8_t reserved;
    uint16_t len;
} ip_tcp_sent_ind_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(ip_tcp_sent_ind_t)
WIFI_FW_STRUCT_SIZE_SYNC(ip_tcp_sent_ind_t, 8)

typedef struct ip_tcp_closed_ind_s {
    lwip_svc_api_hdr_t api_hdr;
    uint8_t conn_id;
    int8_t err_code;
    uint16_t reserved;
} ip_tcp_closed_ind_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(ip_tcp_closed_ind_t)
WIFI_FW_STRUCT_SIZE_SYNC(ip_tcp_closed_ind_t, 8)

typedef struct ip_udp_server_start_cfm_s {
    lwip_svc_api_hdr_t api_hdr;
    uint8_t netif_id;
    int8_t err_code;
    uint16_t port;
    uint8_t listen_id;
    uint8_t reserved[Reserve_24_BIT];

} ip_udp_server_start_cfm_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(ip_udp_server_start_cfm_t)
WIFI_FW_STRUCT_SIZE_SYNC(ip_udp_server_start_cfm_t, 12)

typedef struct ip_udp_client_start_cfm_s {
    lwip_svc_api_hdr_t api_hdr;
    uint8_t netif_id;
    int8_t err_code;
    uint16_t local_port;
    uint8_t conn_id;
    uint8_t reserved1;
    uint8_t a2f_ring_id;
    uint8_t a2f_ring_elem_size;
    uint32_t a2f_ring_addr;
    uint16_t a2f_ring_num_elem;
    uint16_t reserved2;
} ip_udp_client_start_cfm_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(ip_udp_client_start_cfm_t)
WIFI_FW_STRUCT_SIZE_SYNC(ip_udp_client_start_cfm_t, 20)

typedef struct ip_udp_new_remote_ip_ind_s {
    lwip_svc_api_hdr_t api_hdr;

    uint8_t conn_id;
    uint8_t listen_id;
    int8_t error_code;
    uint8_t ip_ver;

    uint32_t remote_ipv4_addr;    /* If ip_ver is IP_VER_V4 */
    uint32_t remote_ipv6_addr[4]; /* If ip_ver is IP_VER_V6 */

    uint16_t port_num;
    uint16_t reserved1;

    uint8_t f2a_ring_id;
    uint8_t f2a_ring_elem_size;
    uint16_t f2a_ring_num_elem;
    uint32_t f2a_ring_addr;
} ip_udp_new_remote_ip_ind_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(ip_udp_new_remote_ip_ind_t)
WIFI_FW_STRUCT_SIZE_SYNC(ip_udp_new_remote_ip_ind_t, 40)

typedef struct ip_udp_close_cfm_s {
    lwip_svc_api_hdr_t api_hdr;
    uint8_t conn_id;
    uint8_t listen_id;
    int8_t err_code;
    uint8_t reserved;
} ip_udp_close_cfm_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(ip_udp_close_cfm_t)
WIFI_FW_STRUCT_SIZE_SYNC(ip_udp_close_cfm_t, 8)

typedef struct ip_addr_ready_ind_s {
    lwip_svc_api_hdr_t api_hdr;

    uint8_t reserved1;
    uint8_t netif_id;
    uint8_t dhcp_type; /* check ringif_dhcp_type */
    uint8_t ip_ver;    /* check ringif_ipaddr_type */

    uint32_t ipv4_addr;    /* If ip_ver is IP_VER_V4 */
    uint32_t ipv6_addr[4]; /* If ip_ver is IP_VER_V6 */

} ip_addr_ready_ind_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(ip_addr_ready_ind_t)
WIFI_FW_STRUCT_SIZE_SYNC(ip_addr_ready_ind_t, 28)

typedef struct raweth_start_conn_cfm_s {
    lwip_svc_api_hdr_t api_hdr;
    uint8_t netif_id;
    uint8_t reserved1;
    uint16_t ether_type;
    int8_t err_code;
    uint8_t conn_id;
    uint8_t a2f_ring_id;
    uint8_t f2a_ring_id;
    uint8_t a2f_ring_elem_size;
    uint8_t f2a_ring_elem_size;
    uint16_t a2f_ring_num_elem;
    uint16_t f2a_ring_num_elem;
    uint16_t reserved;
    uint32_t a2f_ring_addr;
    uint32_t f2a_ring_addr;
    uint32_t reserved2;
    uint8_t dest_mac[IEEE80211_ADDR_LENGTH];
    uint16_t reserved3;
} raweth_start_conn_cfm_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(raweth_start_conn_cfm_t)
WIFI_FW_STRUCT_SIZE_SYNC(raweth_start_conn_cfm_t, 40)

typedef struct raweth_close_conn_cfm_s {
    lwip_svc_api_hdr_t api_hdr;
    uint8_t conn_id;
    int8_t err_code;
    uint16_t reserved;
} raweth_close_conn_cfm_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(raweth_close_conn_cfm_t)
WIFI_FW_STRUCT_SIZE_SYNC(raweth_close_conn_cfm_t, 8)

#endif /* WIFI_FW_IP_CONN_API_H */
