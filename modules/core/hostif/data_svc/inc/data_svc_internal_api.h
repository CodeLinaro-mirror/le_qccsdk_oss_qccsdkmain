/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*========================================================================
 * @file	lwip_svc.h
 * @brief Lwip Handler param, struct, function declarations exposed to
 * other modules
 *========================================================================*/

#ifndef DATA_SVC_INTERNAL_API_H
#define DATA_SVC_INTERNAL_API_H

/*------------------------------------------------------------------------
 * Include Files
 * ----------------------------------------------------------------------*/
#include <netif.h>
#include <ctrl_ring_hdlr.h>
#include <data_ring_hdlr.h>
#include <wifi_fw_ip_conn_api.h>

/*------------------------------------------------------------------------
 * Type Declarations
 * ----------------------------------------------------------------------*/
#ifdef SUPPORT_DATA_LOOPBACK
typedef enum {
    DATA_SVC_LOOPBACK_DISABLE = 0,
    DATA_SVC_LOOPBACK_ENABLE = 1,
} loopback_state;
#endif

/*------------------------------------------------------------------------
 * Function Declarations and Documentation
 * ----------------------------------------------------------------------*/

/* API Functions exposed by LWIP Config layer */
bool data_svc_post2thread_config_pkt(void *p_elem);
void data_svc_netif_callback_function(struct netif *netif);
void data_svc_netif_link_change(struct netif *p_netif);

uint8_t data_svc_get_qos(uint16_t eth_type, uint8_t *dest_mac_addr, uint8_t *src_mac_addr);

bool data_svc_process_udp_pkt(void *p_element);
bool data_svc_process_tcp_pkt(void *p_element);
bool data_svc_process_raweth_pkt(void *p_element);

bool data_svc_get_udp_data_buff(void *p_element, uint16_t len);
bool data_svc_free_udp_data_buff(void *p_element);

bool data_svc_free_tcp_data_buff(void *p_element);
bool data_svc_get_tcp_data_buff(void *p_element, uint16_t len);

bool data_svc_get_rawEth_data_buff(void *p_element, uint16_t len);
bool data_svc_free_rawEth_data_buff(void *p_element);

uint8_t data_svc_get_qos_for_non_ip(void *p_tx_eth_hdr);
uint8_t data_svc_get_tid_for_non_ip(void *p_tx_eth_hdr);
uint8_t data_svc_default_tid_from_qos(uint8_t qos_for_non_ip);

err_t data_svc_recv_raweth_data_pkt(struct pbuf *p_pbuf, struct netif *p_netif);
#ifdef SUPPORT_DATA_LOOPBACK
bool data_svc_enable_disable_data_loopback(uint8_t enable);
bool data_svc_get_loopback_state();
#endif

void data_svc_init();

#endif /* DATA_SVC_INTERNAL_API_H */
