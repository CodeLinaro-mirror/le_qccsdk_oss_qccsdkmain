/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#ifndef DATA_SVC_PRIV_H
#define DATA_SVC_PRIV_H

/*========================================================================
 * @file lwip_svc.h
 * @brief struct, function declarations common to TCP/UDP/rawEth connections
 *========================================================================*/

/*------------------------------------------------------------------------
 * Include Files
 * ----------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include "tcpip.h"
#include "data_svc_internal_api.h"
#include "netif.h"
#include "pbuf.h"
#include "ring_ctx_holder.h"

/*------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * ----------------------------------------------------------------------*/
#define DATA_SVC_INVALID_RING_ID 0xff
#define DATA_SVC_MAX_LISTEN      64
#define DATA_SVC_MAX_CONN        64

/*------------------------------------------------------------------------
 * Type Declarations
 * ----------------------------------------------------------------------*/

typedef enum {
    DATA_SVC_UDP = 0,
    DATA_SVC_TCP = 1,
    DATA_SVC_RAW_ETHER = 2,
    DATA_SVC_HFC = 3,
    DATA_SVC_INVALID_TYPE = 4,
} data_svc_conn_type_t;

typedef struct tcp_send_buff_s {
    struct pbuf *p_pbuf;
    struct tcp_send_buff_s *next;
} tcp_send_buff_t;

// CONNECTION HANDLER

typedef struct conn_handler_s {
    uint8_t idx;
    uint8_t type;        // 0-udp 1-tcp 2-rawEther
    uint8_t server;      // 0-client 1-server
    void *pcb;           // pcb depending on type
    uint8_t conn_state;  // 0-not connected yet 1-connected
    uint8_t netif_id;
    uint16_t local_port;
    uint8_t a2f_ring_id;
    uint8_t f2a_ring_id;
    uint8_t tls_params;
    tcp_send_buff_t *send_buff;
    tcp_send_buff_t *ack_list;
    uint16_t sent_length;
    uint16_t ack_length;
    ip_addr_t remote_addr;
    uint16_t remote_port;
    uint16_t eth_type;
    uint8_t dest_mac[NT_MAC_ADDR_SIZE];
    uint8_t qos;
#ifdef SUPPORT_RAWETH_IPERF
    void *test_ctx;
#endif /* SUPPORT_RAWETH_IPERF */
} conn_handler_t;

typedef struct listen_handler_s {
    uint8_t idx;
    uint8_t type;  // 0-udp 1-tcp
    void *pcb;     // pcb depending on type
    uint8_t netif_id;
    uint16_t local_port;
    uint8_t tls_params;
    uint64_t bitmap;
} listen_handler_t;

/*------------------------------------------------------------------------
 * Function Declarations and Documentation
 * ----------------------------------------------------------------------*/

conn_handler_t *data_svc_get_new_conn_handler();
conn_handler_t *data_svc_get_conn_handler_from_id(uint8_t conn_id);
void data_svc_free_conn_handler(uint8_t conn_id);

listen_handler_t *data_svc_get_new_listen_handler();
listen_handler_t *data_svc_get_listen_handler_from_id(uint8_t conn_id);
void data_svc_free_listen_handler(uint8_t conn_id);

void data_svc_init();

bool data_svc_process_ip_config_pkt(void *p_buff, uint16_t len);
bool data_svc_process_raweth_config_pkt(void *p_buff, uint16_t len);

extern err_t nt_low_level_output(struct netif *netif, struct pbuf *p);

#endif /* DATA_SVC_PRIV_H */
