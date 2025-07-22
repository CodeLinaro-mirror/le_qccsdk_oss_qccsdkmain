/*
Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#ifndef NETWORK_AL_H_
#define NETWORK_AL_H_
#include "data_path.h"
#include "ip_addr.h"
#include "uart.h"
#include "err.h"
#include "wlan_dev.h"
//#include "autoconf.h"

#define IPv4_IP_IDX      0
#define IPv4_NETMASK_IDX 1
#define IPv4_GATEWAY_IDX 2

#define IPv6_LINK_LOCAL_IDX 0

#define DEFAULT_NETIF_IDX netif_get_index(netif_default) /* Default netif idx */

extern portBASE_TYPE xInsideISR;

/*
#define STA_NETIF				DEFAULT_NETIF_IDX
#define AP_NETIF				DEFAULT_NETIF_IDX
*/

typedef enum {
    NETIF_IP_VER_V4 = 0U,
    NETIF_IP_VER_V6 = 1U,
} netif_ipaddr_type_t;

typedef enum {
    NETIF_IP_TYPE_STATIC = 0U,
    NETIF_IP_TYPE_DYNAMIC = 1U,
    NETIF_IP_TYPE_LOCAL = 2U,
} netif_dhcp_type_t;

#ifdef NT_FN_RRAM_PERF_BUILD
__attribute__((section(".perf_cm_txt"))) void *nt_dpm_allocate_network_buffer(uint32_t length);
__attribute__((section(".perf_cm_txt"))) void nt_dpm_free_network_buffer(void *buf);
__attribute__((section(".perf_cm_txt"))) void nt_dpm_realloc_network_buffer(void *buf, uint32_t length);
__attribute__((section(".perf_rx_txt"))) uint8_t nt_dpm_forward_eth_packet_to_stack(p_ndpA ad, void *rx_frame,
                                                                                    void *eth_frame, uint32_t length,
                                                                                    device_t *dev);

#ifdef SUPPORT_RING_IF
__attribute__((section(".perf_nc_txt"))) void nt_dpm_add_dev_to_stack(device_t *dev, void *buffer);
else __attribute__((section(".perf_nc_txt"))) void nt_dpm_add_dev_to_stack(device_t *dev);
#endif  // SUPPORT_RING_IF
__attribute__((section(".perf_nc_txt"))) void nt_dpm_remove_dev_from_stack(device_t *dev);
__attribute__((section(".perf_nc_txt"))) void nt_dpm_network_init();
#ifdef SUPPORT_RING_IF
__attribute__((section(".perf_nc_txt"))) nt_status_t nt_dpm_process_eth_packet_from_stack(void *frame, uint32_t length,
                                                                                          uint8_t qos_for_non_ip);
#else
__attribute__((section(".perf_tx_txt"))) nt_status_t nt_dpm_process_eth_packet_from_stack(void *frame, uint32_t length);
#endif  // SUPPORT_RING_IF
__attribute__((section(".perf_nc_txt"))) const ip_addr_t *nt_dpm_get_ip(struct netif *netif, u8_t type, s8_t idx);
__attribute__((section(".perf_nc_txt"))) void nt_dpm_set_ip(struct netif *netif, ip_addr_t *ip, s8_t idx);
__attribute__((section(".perf_nc_txt"))) void nt_dpm_set_ip4(struct netif *netif, ip_addr_t *ip_addr,
                                                             ip_addr_t *netmask, ip_addr_t *gw);
__attribute__((section(".perf_nc_txt"))) uint8_t nt_dpm_enable_disable_dhcp(struct netif *netif, NT_BOOL is_server,
                                                                            uint8_t state, uint8_t cmd_cli);
__attribute__((section(".perf_nc_txt"))) void nt_dpm_stop_network_stack();
__attribute__((section(".perf_nc_txt"))) void nt_dpm_start_network_stack();
#ifdef NT_FN_DHCPS_V4
__attribute__((section(".perf_nc_txt"))) NT_BOOL nt_dpm_get_ip_info(uint8_t if_index, struct ip_info *info);
#endif  // NT_FN_DHCPS_V4
__attribute__((section(".perf_nc_txt"))) uint8_t nt_get_netifidx_by_devmode(u8_t devid);
__attribute__((section(".perf_nc_txt"))) uint8_t nt_get_opmode(void);
__attribute__((section(".perf_nc_txt"))) void nt_dpm_netif_set_link_up(struct netif *netif);
__attribute__((section(".perf_nc_txt"))) err_t nt_ethernetif_init(struct netif *netif);
#ifdef SUPPORT_RING_IF
__attribute__((section(".perf_nc_txt"))) nt_status_t get_netif_hwaddr_from_netif_id(uint8_t netif_id, uint8_t *addr);
__attribute__((section(".perf_nc_txt"))) uint8_t nt_dpm_get_ip_type(struct netif *p_netif);
__attribute__((section(".perf_nc_txt"))) bool nt_dpm_is_netif_ready(struct netif *p_netif);
__attribute__((section(".perf_nc_txt"))) void nt_dpm_netif_set_link_down(struct netif *p_netif);
#else
__attribute__((section(".perf_nc_txt"))) void nt_dpm_netif_set_link_down(struct netif *netif);
#endif /* SUPPORT_RING_IF */
/*-----------------------------RCLI----------------------------------------*/
#if (defined CONFIG_NT_RCLI)
__attribute__((section(".perf_nc_txt"))) void nt_show_ip(void);
#endif
/*-----------------------------RCLI----------------------------------------*/
#else
void *nt_dpm_allocate_network_buffer(uint32_t length);
void *nt_dpm_allocate_network_buffer_pool(uint32_t length);
void nt_dpm_free_network_buffer(void *buf);
void nt_dpm_realloc_network_buffer(void *buf, uint32_t length);
uint8_t nt_dpm_forward_eth_packet_to_stack(p_ndpA ad, void *rx_frame, void *eth_frame, uint32_t length, device_t *dev);

#ifdef SUPPORT_RING_IF
void nt_dpm_add_dev_to_stack(device_t *dev, void *buffer);
#else
void nt_dpm_add_dev_to_stack(device_t *dev);
#endif
void nt_dpm_remove_dev_from_stack(device_t *dev);
void nt_dpm_network_init();
#ifdef SUPPORT_RING_IF
nt_status_t nt_dpm_process_eth_packet_from_stack(void *frame, uint32_t length, uint8_t qos_for_non_ip);
#else
nt_status_t nt_dpm_process_eth_packet_from_stack(void *frame, uint32_t length);
#endif

const ip_addr_t *nt_dpm_get_ip(struct netif *netif, u8_t type, s8_t idx);
void nt_dpm_set_ip(struct netif *netif, ip_addr_t *ip, s8_t idx);
void nt_dpm_set_ip4(struct netif *netif, ip_addr_t *ip_addr, ip_addr_t *netmask, ip_addr_t *gw);
uint8_t nt_dpm_enable_disable_dhcp(struct netif *netif, NT_BOOL is_server, uint8_t state, uint8_t cmd_cli);
void nt_dpm_stop_network_stack();
void nt_dpm_start_network_stack();
#ifdef NT_FN_DHCPS_V4
NT_BOOL nt_dpm_get_ip_info(uint8_t if_index, struct ip_info *info);
#endif  // NT_FN_DHCPS_V4
uint8_t nt_get_netifidx_by_devmode(u8_t devid);
uint8_t nt_get_opmode(void);
void nt_dpm_netif_set_link_up(struct netif *netif);

err_t nt_ethernetif_init(struct netif *netif);
uint8_t nt_dpm_get_ip_ver(struct netif *p_netif);
/*-----------------------------RCLI----------------------------------------*/
#if (defined CONFIG_NT_RCLI)
void nt_show_ip(void);
#endif
/*-----------------------------RCLI----------------------------------------*/
#if defined(SUPPORT_RING_IF) || defined(CONFIG_WMI_EVENT)
nt_status_t get_netif_hwaddr_from_netif_id(uint8_t netif_id, uint8_t *addr);
uint8_t nt_dpm_get_ip_type(struct netif *p_netif);
bool nt_dpm_is_netif_ready(struct netif *p_netif);
void nt_dpm_netif_set_link_down(struct netif *p_netif);
#else
void nt_dpm_netif_set_link_down(struct netif *netif);
#endif  /* SUPPORT_RING_IF */
#endif  // NT_FN_RRAM_PERF_BUILD
void nt_dpm_wait_till_device_wakeup(void);
bool nt_dpm_macid_connected_chk(struct netif *p_netif, uint8 *mac_addr);
#endif /* NETWORK_AL_H_ */
