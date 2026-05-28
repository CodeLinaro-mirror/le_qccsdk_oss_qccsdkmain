/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "data_path.h"
#include "network_al.h"
#include "defines.h"
#include "nt_osal.h"
#include "hal_int_sys.h"
#include "wifi_cmn.h"

#include "fwconfig_wlan.h"
#include "nt_flags.h"
#include "safeAPI.h"

#if defined(LWIP)

#include "lwip/pbuf.h"
#include "lwip/netif.h"
#include "lwip/tcpip.h"
#include "ip_addr.h"
#include "netifapi.h"
#include "nt_devcfg.h"
#include "nt_logger_api.h"
#if NT_FN_DHCPS_V4
#include "lwip/apps/nt_dhcps.h"
#include "dns.h"
#include "ip_addr.h"
#endif /* NT_FN_DHCPS_V4 */
#if NT_FN_DHCP6
#include "dhcp6.h"
#endif
#if LWIP_AUTOIP
#include "autoip.h"
#endif
#include "nt_hosted_app.h"

/*access pcb->flags*/
#include "lwip/priv/tcp_priv.h"

#ifdef SUPPORT_RING_IF
#include "wifi_evt_hndlr.h"
#include "nt_wfm_wmi_interface.h"
#include "data_path.h"
#include "data_svc_internal_api.h"

extern wmi_msg_struct_t g_Cmd_Translation_wifi_hndl;
#else
#include "wlan_wmi.h"
#endif

#include "hal_int_modules.h"
#include "mlme_api.h"
#include "cnxmgmt_internal.h"
#include "nt_socpm_sleep.h"
#include "wlan_power.h"

#define MAX_NUM_APPS_CONFIG 2
WMI_IF_ADD_CMD *g_apps_ip_config[MAX_NUM_APPS_CONFIG] = {NULL, NULL};
#define GET_APPS_CONFIG_INDEX(net_if_idx) (net_if_idx % MAX_NUM_APPS_CONFIG)

#define SIZEOF_PBUF LWIP_MEM_ALIGN_SIZE(sizeof(struct pbuf))

bool wakeup_cb(uint16_t type, bool bm_cast, void *pbuf, uint16_t len);

bool (*wakeup_cb_net)(uint16_t type, bool bm_cast, void *pbuf, uint16_t len);

ip_addr_t ip_address[MAX_ROLE];
ip_addr_t _ip_address[MAX_ROLE];
ip_addr_t netmask[MAX_ROLE];
ip_addr_t default_gw[MAX_ROLE];
nt_osal_semaphore_handle_t ntSleepSemaphoreHandle = NULL;
static uint32_t last_ip_32 = 0;

/**
 * @ingroup network_al
 * Fetch IP address from network interface in network order.
 *
 * @param type IP version type to be fetch IPADDR_TYPE_V6 or IPADDR_TYPE_V4.
 * @param idx Index to select IP for IPv6 idx = 0 (link_local), idx = 1 (site_local), idx = 2 (global_uincast)
 *                                  for IPV4 idx = 0 (ipv4 address), idx = 1 (subnet mask), idx = 2 (default gateway)
 * @return pointer to IP address else NULL on failure.
 */
const ip_addr_t *nt_dpm_get_ip(struct netif *netif, u8_t type, s8_t idx)
{
    if (netif == NULL) {
        NT_LOG_DPM_ERR("Default network interface not initialized", 0, 0, 0);
        return NULL;
    }

    if (!((type == IPADDR_TYPE_V4) || (type == IPADDR_TYPE_V6)) && ((idx < 0) && (idx > 2))) {
        NT_LOG_DPM_ERR("Invalid type  or Invalid index ", 0, 0, 0);
        return NULL;
    }

#if LWIP_IPV6
    if (type == IPADDR_TYPE_V6) {
        return &netif->ip6_addr[idx];  // ipv6_address
    }
#endif

#if LWIP_IPV4
    if (type == IPADDR_TYPE_V4) {
        if (idx == IPv4_IP_IDX) {  // ipv4 address
            return netif_ip_addr4(netif);
        } else if (idx == IPv4_NETMASK_IDX) {  // ipv4 subnet mask
            return netif_ip_netmask4(netif);
        } else if (idx == IPv4_GATEWAY_IDX) {  // ipv4 default gateway
            return netif_ip_gw4(netif);
        }
    }
#endif
    return NULL;
}

/**
 * @ingroup network_al
 * set IP address from network interface.
 *
 * @param ip IP address pointer having IP stored in network order.
 * @param idx Index to select IP for IPv6 idx = 0 (link_local), idx = 1 (site_local), idx = 2 (multicast)
 *                                  for IPV4 idx = 0 (ipv4 address), idx = 1 (subnet mask), idx = 2 (default gateway)
 * @return (void) set IP address on netif_default.
 *
 */
void nt_dpm_set_ip(struct netif *netif, ip_addr_t *ip, s8_t idx)
{
    if (netif == NULL) {
        netif = netif_get_by_index(DEFAULT_NETIF_IDX);
    }

    if (netif == NULL && ip_addr_isany(ip) && ((idx < 0) && (idx > 2))) {
        NT_LOG_DPM_ERR("Default network interface not initialized", 0, 0, 0);
        return;
    }

#if LWIP_IPV6
    if (IP_IS_V6_VAL(*ip)) {
        netif_ip6_addr_set(netif, idx, (const ip6_addr_t *)ip_2_ip6(ip));
        netif_ip6_addr_set_state(netif, idx, IP6_ADDR_VALID);
    }
#endif
#if LWIP_IPV4

    if (IP_IS_V4_VAL(*ip)) {
        if (idx == IPv4_IP_IDX)
            netif_set_ipaddr(netif, (const ip4_addr_t *)ip_2_ip4(ip));
        else if (idx == IPv4_NETMASK_IDX)
            netif_set_netmask(netif, (const ip4_addr_t *)ip_2_ip4(ip));
        else if (idx == IPv4_GATEWAY_IDX)
            netif_set_gw(netif, (const ip4_addr_t *)ip_2_ip4(ip));
    }
#endif
    return;
}

void nt_dpm_set_ip4(struct netif *netif, ip_addr_t *ip_addr, ip_addr_t *netmask, ip_addr_t *gw)
{
    if (ip_addr != NULL && !ip_addr_isany_val(*ip_addr)) {
        nt_dpm_set_ip(netif, ip_addr, IPv4_IP_IDX);
    }

    if (netmask != NULL && !ip_addr_isany_val(*netmask) && ip_addr_netmask_valid(ip_2_ip4(netmask))) {
        nt_dpm_set_ip(netif, netmask, IPv4_NETMASK_IDX);
    }

    if (gw != NULL && !ip_addr_isany_val(*gw)) {
        nt_dpm_set_ip(netif, gw, IPv4_GATEWAY_IDX);
    }
}
#ifdef NT_HOSTED_SDK
int response_log = 0;
#endif

/**
 * @ingroup network_al
 * Display IP address for network interface.
 */
void nt_show_ip(void)
{
#ifdef NT_HOSTED_SDK
    response_log = 1;
    char c = '>';
    nt_at_spi_send_of_len(&c, 1);
#endif

    char pbuf[100];
    NT_BOOL state = FALSE;
    ip_addr_t *ip_addr = NULL;
    char addr_type[16] = {0};
    char *addr = NULL;
    struct netif *netif = NULL;
    uint8_t ip_type = 0;
    char *padding = ". . . . . . . . . . . . . . . . . :";
#if ((defined NT_FN_DNS) && (defined NT_FN_DHCPS_V4))
    ip_addr_t *server_addr = NULL;
#endif  // #if  ((defined NT_FN_DNS) && (defined NT_FN_DHCPS_V4))

    NETIF_FOREACH(netif)
    {
        if (netif == NULL) {
            NT_LOG_DPM_ERR("Default network interface not initialized", 0, 0, 0);
            return;
        } else {
            while (netif->next != NULL) {
                netif = netif->next;
            }
        }

#if NT_FN_DHCPS_V4
        struct dhcp *dhcp = netif_dhcp_data(netif);
        state = (dhcp != NULL && dhcp->state != DHCP_STATE_OFF) ? TRUE : FALSE;
#endif /* NT_FN_DHCPS_V4 */

        nt_dbg_print("**************************************************************\r\n");
        nt_dbg_print("Neutrino IP Configuration \r\n");

        for (int j = 0; j < 2; j++) {
            ip_type = (j == 0) ? IPADDR_TYPE_V6 : IPADDR_TYPE_V4;
            for (int i = 0; i < LWIP_IPV6_NUM_ADDRESSES || i < 3; i++) {
                memset(addr_type, 0, sizeof(addr_type));
                ip_addr = (ip_addr_t *)nt_dpm_get_ip(netif, ip_type, i);
                if (ip_addr != NULL) {
#if LWIP_IPV6
                    if (ip_type == IPADDR_TYPE_V6) {
                        if (ip6_addr_isglobal(ip_2_ip6(ip_addr))) {
                            strlcpy(addr_type, "Global-local", strlen("Global-local"));
                        } else if (ip6_addr_islinklocal(ip_2_ip6(ip_addr))) {
                            strlcpy(addr_type, "Link-local", strlen("Link-local"));
                        } else if (ip6_addr_issitelocal(ip_2_ip6(ip_addr))) {
                            strlcpy(addr_type, "Site-local", strlen("Site-local"));
                        } else if (ip6_addr_isuniquelocal(ip_2_ip6(ip_addr))) {
                            strlcpy(addr_type, "Unique-local", strlen("Unique-local"));
                        } else if (ip6_addr_isipv4mappedipv6(ip_2_ip6(ip_addr))) {
                            strlcpy(addr_type, "v4mapped-v6", strlen("v4mapped-v6"));
                        }
                    } else
#endif
                    {
                        if (ip_type == IPADDR_TYPE_V4) {
                            if (i == IPv4_IP_IDX) {
                                strlcpy(addr_type, "IPv4 Address", sizeof("IPv4 Address"));
                            } else if (i == IPv4_NETMASK_IDX) {
                                strlcpy(addr_type, "Subnet Mask", sizeof("Subnet Mask"));
                            } else if (i == IPv4_GATEWAY_IDX) {
                                strlcpy(addr_type, "Default Gateway", sizeof("Default Gateway"));
                            }
                        }
                    }
                    if (!ip_addr_isany_val(*ip_addr)) {
                        addr = ipaddr_ntoa(ip_addr);
                        if (addr != NULL) {
                            snprintf(pbuf, sizeof(pbuf), "%s %s\r\n", padding, addr);
                            if (ip_type == IPADDR_TYPE_V6) {
                                snprintf(pbuf, sizeof(pbuf), "%s IPv6 Address", addr_type);
                            } else {
                                snprintf(pbuf, sizeof(pbuf), "%s", addr_type);
                            }
                            pbuf[strlen(pbuf)] = (strlen(pbuf) % 2 == 0) ? '.' : ' ';
                            nt_dbg_print(pbuf);
#ifdef NT_HOSTED_SDK
                            nt_at_spi_send_of_len(pbuf, strlen(pbuf));
#endif
                            addr = NULL;
                        }
                    }
                }
            }
        }
#if LWIP_IPV4
        snprintf(pbuf, sizeof(pbuf), "DHCP Enabled. . . . . . . . . . . : %s\r\n", (state ? "yes" : "no"));
        nt_dbg_print(pbuf);
#endif
#if ((defined NT_FN_DNS) && (defined NT_FN_DHCPS_V4))
        server_addr = dhcp_dns_getserver();

        if (!ip_addr_isany_val(*server_addr)) {
            snprintf(pbuf, sizeof(pbuf), "DNS configured in DHCP server IP address . . . . . : %s\r\n",
                     ipaddr_ntoa(server_addr));
            nt_dbg_print(pbuf);
        }

        for (int indx = 0; indx < 2; indx++) {
            server_addr = (ip_addr_t *)dns_getserver(indx);
            if (!ip_addr_isany_val(*server_addr)) {
                snprintf(pbuf, sizeof(pbuf), "DNS client IP  configured statically  of index %d. : %s\r\n", indx,
                         ipaddr_ntoa(server_addr));
                nt_dbg_print(pbuf);
            }
        }

#endif  // (defined NT_FN_DNS) && (defined NT_FN_DHCPS_V4)
    }

#ifdef NT_HOSTED_SDK
    response_log = 0;
    c = '<';
    nt_at_spi_send_of_len(&c, 1);
#endif
}

#elif defined(FREERTOS_TCP_IP)
// FreeRTOS+TCP includes.
#include "FreeRTOS.h"
#include "list.h"

#include "FreeRTOS_IP.h"
#include "FreeRTOS_IP_Private.h"
#include "NetworkBufferManagement.h"
#include "NetworkInterface.h"
/*
#include "FreeRTOS_Sockets.h"
 */

#endif

/**
 * @ingroup network_al
 * Default IPV6 and IPV4 address initialization.
 */
void nt_dpm_network_init()
{
#if defined(LWIP)

    uint8_t AP_MAC[NT_MAC_ADDR_SIZE] = {0x42, 0x42, 0x42, 0x42, 0x42, 0x42};   // MAC_address for AP
    uint8_t STA_MAC[NT_MAC_ADDR_SIZE] = {0x40, 0x40, 0x40, 0x40, 0x40, 0x40};  // MAC_address for STA
    device_t *dev;
#if LWIP_IPV6
    ip_addr_t staipaddr = IPADDR6_INIT_HOST(0xfec00000ul, 0x0ul, 0x0ul, 0x01ul);  // IPV6_adderss for netif
    ip_addr_t apipaddr = IPADDR6_INIT_HOST(0xfec00000ul, 0x0ul, 0x0ul, 0x02ul);   // IPV6_adderss for netif
#endif

#if LWIP_IPV4
    ip_addr_t staipaddr_2 = IPADDR4_INIT_BYTES(127, 0, 0, 1);     // IPV4_adderss for netif (STA)
    ip_addr_t apipaddr_2 = IPADDR4_INIT_BYTES(192, 168, 0, 2);      // IPV4_adderss for netif (AP)
    ip_addr_t net_mask_ap = IPADDR4_INIT_BYTES(255, 255, 255, 0);   // IPV4_netmask adderss for netif (AP)
    ip_addr_t net_mask_sta = IPADDR4_INIT_BYTES(255, 0, 0, 0);  // IPV4_netmask adderss for netif (STA)
#endif                                                              /* LWIP_IPV4 */

    dev = pnDpA->neutrino_device;

    if (dev->role == AP_DEVICE) {
        memscpy(dev->mac_address, NT_MAC_ADDR_SIZE, AP_MAC, NT_MAC_ADDR_SIZE);
    } else
        memscpy(dev->mac_address, NT_MAC_ADDR_SIZE, STA_MAC, NT_MAC_ADDR_SIZE);

        /* Static Ip assigned */
#if LWIP_IPV4
    ip_addr_copy(_ip_address[0], staipaddr_2);
    ip_addr_copy(_ip_address[1], apipaddr_2);
    ip_addr_copy(default_gw[0], staipaddr_2);
    ip_addr_copy(default_gw[1], apipaddr_2);
    ip_addr_copy(netmask[0], net_mask_sta);
    ip_addr_copy(netmask[1], net_mask_ap);
#endif /* LWIP_IPV4 */
#if LWIP_IPV6
    ip_addr_copy(ip_address[0], staipaddr);
    ip_addr_copy(ip_address[1], apipaddr);
#endif /* LWIP_IPV6 */

#elif defined(FREERTOS_TCP_IP)
#endif

    /* Create Sleep semaphore to be used to hold network stack during sleep */
    if (NULL == ntSleepSemaphoreHandle) {
        nt_osal_semaphore_create_binary(ntSleepSemaphoreHandle);
        if (NULL == ntSleepSemaphoreHandle) {
            NT_LOG_CLI_ERR("Sleep semaphore creation failed", 0, 0, 0);
        }
    }
}

void *nt_dpm_allocate_network_buffer_pool(uint32_t length)
{
    struct pbuf *p;

    p = pbuf_alloc(PBUF_RAW, length, PBUF_POOL);

    if (p != NULL)
        return p->payload;
    else
        return NULL;
}

void *nt_dpm_allocate_network_buffer(uint32_t length)
{
    struct pbuf *p;

    p = pbuf_alloc(PBUF_RAW, length, PBUF_RAM);

    if (p != NULL)
        return p->payload;
    else
        return NULL;
}

void nt_dpm_free_network_buffer(void *buf)
{
    struct pbuf *p;
    uint32_t ret;

    p = (struct pbuf *)((uint8_t *)buf - SIZEOF_PBUF);
    ret = pbuf_free(p);

    if (ret == 0) {
        NT_LOG_DPM_ERR("!!! DPM PBUF FREE ERROR\r\n", 0, 0, 0);
    }
    return;
}

void nt_dpm_realloc_network_buffer(void *buf, uint32_t length)
{
    struct pbuf *p;

    p = (struct pbuf *)((uint8_t *)buf - SIZEOF_PBUF);
    pbuf_realloc(p, length);
    return;
}

/**
 * @ingroup network_al
 * Forward Ethernet packet from data path to stack.
 *
 * @param Pointer to data path adapter.
 * @param Pointer to data frame.
 * @param length of data frame.
 *
 * @return NT_OK on success.
 *
 */
static const uint8_t bc_add[NT_MAC_ADDR_SIZE] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
uint8_t nt_dpm_forward_eth_packet_to_stack(p_ndpA ad, void *rx_frame, void *eth_frame, uint32_t length, device_t *dev)
{
#if defined(LWIP)
    struct netif *netif;
    struct pbuf *p;
    err_t err = ERR_VAL;

    (void)ad;

    LINK_STATS_INC(link.recv);

    netif = dev->netif;
    p = (struct pbuf *)((uint8_t *)rx_frame - SIZEOF_PBUF);

    if (p != NULL) {
        p->payload = eth_frame;
        p->len = length;
        p->tot_len = length;
#ifdef NT_TST_TIME_STAMP_ENABLE
        if ((nt_dpm_tm.rx_stat[NETIF_INPUT].valid == 0) &&
            (*(uint32_t *)((uint8_t *)eth_frame + nt_dpm_tm.rx_stat[RX_INTERRUPT].offset) == nt_dpm_tm.rx_marker)) {
            nt_dpm_tm.rx_stat[NETIF_INPUT].value = nt_hal_get_curr_time();
            nt_dpm_tm.rx_stat[NETIF_INPUT].valid = 1;
        }
#endif
        struct eth_hdr *ethhdr;
        ethhdr = (struct eth_hdr *)p->payload;
        u16_t type = ntohs(ethhdr->type);
        NT_BOOL bm_cast = FALSE;
        // NT_BOOL in_whitelist = FALSE;

        if ((!memcmp(ethhdr->dest.addr, bc_add, NT_MAC_ADDR_SIZE)) ||
            ((ethhdr->dest.addr[0] & NT_DPM_MULTICAST_BIT) == 0x1)) {
            bm_cast = TRUE;
        }

        // NT_LOG_PRINT(COMMON, ERR,"forward2stack, bm_cast:%d\r\n", bm_cast);
        if (bm_cast) {
            PM_STRUCT *pPmStruct = NULL;
            pPmStruct = (PM_STRUCT *)gdevp->pPmStruct;
            if (pPmStruct->bmps_rx_filter_enabled) {
                if (wakeup_cb_net) {
                    wakeup_cb_net(type, bm_cast, p->payload, p->len);
                }
            }
        }

        err = netif->input(p, netif);
        if (err != ERR_OK) {
            // NT_LOG_DPM_CRIT("\r\nunable to receive packet on current network interface\r\n",err,0,0);
            LINK_STATS_INC(link.drop);
            LINK_STATS_INC(link.cachehit);
            if (err == ERR_MEM) {
                LINK_STATS_INC(link.memerr);
            }
            pbuf_free(p);
            return err;
        }
    } else {
        NT_LOG_DPM_CRIT("Pbuf allocation failed Out of memory\r\n", 0, 0, 0);
        LINK_STATS_INC(link.drop);
        LINK_STATS_INC(link.memerr);
        return NT_ENOMEM;
    }

    return err;
#elif defined(FREERTOS_TCP_IP)
    IPStackEvent_t xRxEvent;
    NetworkBufferDescriptor_t *pxBufferDescriptor;

    if (length == 0) {
        return;
    }

    pxBufferDescriptor = pxGetNetworkBufferWithDescriptor(length, 0);

    if (pxBufferDescriptor != NULL) {
        vReleaseNetworkBuffer(pxBufferDescriptor->pucEthernetBuffer);
        pxBufferDescriptor->pucEthernetBuffer = frame;
        pxBufferDescriptor->xDataLength = length;

        if (eConsiderFrameForProcessing(pxBufferDescriptor->pucEthernetBuffer) == eProcessBuffer) {
            /* The event about to be sent to the TCP/IP is an Rx event. */
            xRxEvent.eEventType = eNetworkRxEvent;

            /* pvData is used to point to the network buffer descriptor that
             * now references the received data. */
            xRxEvent.pvData = (void *)pxBufferDescriptor;

            /* Send the data to the TCP/IP stack. */
            if (xSendEventStructToIPTask(&xRxEvent, 0) == pdFALSE) {
                /* The buffer could not be sent to the IP task so the buffer
                must be released. */
                vReleaseNetworkBufferAndDescriptor(pxBufferDescriptor);

                /* Make a call to the standard trace macro to log the
                occurrence. */
                iptraceETHERNET_RX_EVENT_LOST();
            } else {
                /* The message was successfully sent to the TCP/IP stack.
                Call the standard trace macro to log the occurrence. */
                iptraceNETWORK_INTERFACE_RECEIVE();
            }
        } else {
            /* The Ethernet frame can be dropped, but the Ethernet buffer
            must be released. */
            vReleaseNetworkBufferAndDescriptor(pxBufferDescriptor);
        }
    }
#endif
}

/**
 * @ingroup network_al
 * Forward Ethernet packet from stack to data path.
 *
 * @param Pointer to data frame.
 * @param length of data frame.
 *
 * @return NT_OK on success.
 *
 */
#ifdef NT_FN_RRAM_PERF_BUILD
nt_status_t
#else
nt_status_t __attribute__((section(".after_ram_vectors")))
#endif
#ifdef SUPPORT_RING_IF
nt_dpm_process_eth_packet_from_stack(void *frame, uint32_t length, uint8_t qos_for_non_ip)
#else
nt_dpm_process_eth_packet_from_stack(void *frame, uint32_t length)
#endif
{
    nt_status_t ret = NT_OK;

#if NT_FN_DPM_WMM || ((defined NT_FN_AP_HAL_DPH_DEBUG_STATS) || (defined NT_FN_STA_HAL_DPH_DEBUG_STATS))
    ethernet_header_t *eth_hdr = NULL;
    eth_hdr = (ethernet_header_t *)((uint8_t *)frame + NT_TX_BUFFER_OFFSET);
#endif
#if ((defined NT_FN_AP_HAL_DPH_DEBUG_STATS) || (defined NT_FN_STA_HAL_DPH_DEBUG_STATS))
    sta_entry_t *sta_entry;
    sta_entry = nt_dpm_find_sta_entry_for_eth_pkt(pnDpA, eth_hdr->xSourceAddress, eth_hdr->xDestinationAddress);
    if (sta_entry != NULL) {
        if (sta_entry->sta_id < NT_MAX_NO_OF_STA_ENTRY)
            ++pnDpA->dp_stats[sta_entry->sta_id].fwd_down;
        else
            NT_LOG_DPM_ERR("sta_id > NT_MAX_NO_OF_STA_ENTRY ", 0, 0, 0);
    }
#endif /* (defined NT_FN_AP_HAL_DPH_DEBUG_STATS)||(defined NT_FN_STA_HAL_DPH_DEBUG_STATS) */

#if NT_FN_DPM_WMM
#ifdef SUPPORT_RING_IF
    if ((pnDpA->wmm_enable == NT_DPM_WMM_ENABLE) && ((is_ip_packet(eth_hdr)) || (qos_for_non_ip < MAX_AC_NUM)))
        ret = nt_dpm_add_to_wmm_queue(pnDpA, frame, length, qos_for_non_ip);
#else
    if ((pnDpA->wmm_enable == NT_DPM_WMM_ENABLE) && ((eth_hdr->usFrameType == dp_htons(NT_IP_FRAME_TYPE)) ||
                                                     (eth_hdr->usFrameType == dp_htons(NT_IPV6_FRAME_TYPE))||
                                                     (eth_hdr->usFrameType == dp_htons(NT_ARP_FRAME_TYPE))))
        ret = nt_dpm_add_to_wmm_queue(pnDpA, frame, length);
#endif /* SUPPORT_RING_IF */
    else
#endif /* NT_FN_DPM_WMM */
        ret = nt_dpm_process_packet(pnDpA, frame, length);

    return ret;
}

#ifdef SUPPORT_RING_IF
/**
 * @ingroup network_al
 * API to save the IP config param structure for future reference
 *
 * @param pointer to device
 * @param pointer to buffer holding the params
 * @param pointer to netif
 *
 * @return None
 *
 */
static void nt_dpm_save_ip_param_from_apps(device_t *dev, void *buffer, struct netif *p_netif)
{
    (void)dev;
    uint8_t apps_cfg_idx = (p_netif ? GET_APPS_CONFIG_INDEX(p_netif->num) : 0);

    if (NULL == buffer) {
        NT_LOG_DPM_ERR("Err: Buff NULL", 0, 0, 0);
        return;
    }

    if (FALSE == g_Cmd_Translation_wifi_hndl.msg_struct.is_ringif_cmd) {
        return;
    }

    if (NULL == g_apps_ip_config[apps_cfg_idx]) {
        g_apps_ip_config[apps_cfg_idx] = nt_osal_calloc(1, sizeof(WMI_IF_ADD_CMD));
    }

    memscpy(g_apps_ip_config[apps_cfg_idx], sizeof(WMI_IF_ADD_CMD), buffer, sizeof(WMI_IF_ADD_CMD));
}

/**
 * @ingroup network_al
 * API to save the IP config params from ring if to local variables
 *
 * @param pointer to device
 * @param pointer to buffer holding the params
 * @param pointer to netif
 *
 * @return None
 *
 */
static NT_BOOL nt_dpm_copy_ip_params(device_t *dev, void *buffer)
{
    WMI_IF_ADD_CMD *ringif_ip_config = (WMI_IF_ADD_CMD *)buffer;
    uint8_t role = dev->role;

    if (NULL == buffer) {
        NT_LOG_DPM_ERR("Err: Buff NULL", 0, 0, 0);
        return TRUE;
    }

    if (FALSE == g_Cmd_Translation_wifi_hndl.msg_struct.is_ringif_cmd) {
        return TRUE;
    }

    if (RINGIF_IP_TYPE_DYNAMIC == ringif_ip_config->dhcp_type) {
#if LWIP_DHCP
        *((uint8_t *)(nt_devcfg_get_config(NT_DEVCFG_DEFAULT_DHCP_ENABLE))) = 1;
#else  /* LWIP_DHCP */
        NT_LOG_DPM_ERR("Warning: Ignoring DHCP config since its not compiled-in\r\n", 0, 0, 0);
#endif /* LWIP_DHCP */
    } else if (RINGIF_IP_TYPE_STATIC == ringif_ip_config->dhcp_type) {
        *((uint8_t *)(nt_devcfg_get_config(NT_DEVCFG_DEFAULT_DHCP_ENABLE))) = 0;
    } else {
        *((uint8_t *)(nt_devcfg_get_config(NT_DEVCFG_DEFAULT_DHCP_ENABLE))) = 0;
        NT_LOG_DPM_ERR("Warning: DHCP config only Static/Dynamic supported now\r\n", 0, 0, 0);
        return FALSE;
    }

    if (IPADDR_TYPE_V4 == ringif_ip_config->ip_ver) {
#if LWIP_IPV4
        ip_addr_set_ip4_u32_val(_ip_address[role], ringif_ip_config->ipv4_addr);
        ip_addr_set_ip4_u32_val(default_gw[role], ringif_ip_config->gateway);
        ip_addr_set_ip4_u32_val(netmask[role], ringif_ip_config->netmask);
#else  /* LWIP_IPV4 */
        NT_LOG_DPM_ERR("Warning: Ignoring IPv4 config since its not compiled-in\r\n", 0, 0, 0););
        return FALSE;
#endif /* LWIP_IPV4 */
    } else if (IPADDR_TYPE_V6 == ringif_ip_config->ip_ver) {
#if LWIP_IPV6
        ip6_addr_t *p_ip6 = (void *)&ip_address[role];
        IP6_ADDR(p_ip6, ringif_ip_config->ipv6_addr[0], ringif_ip_config->ipv6_addr[1], ringif_ip_config->ipv6_addr[2],
                 ringif_ip_config->ipv6_addr[3]);
#else  /* LWIP_IPV6 */
        NT_LOG_DPM_ERR("Warning: Ignoring IPv6 config since its not compiled-in\r\n", 0, 0, 0);
        return FALSE;
#endif /* LWIP_IPV6 */
    } else {
        NT_LOG_DPM_ERR("Warning: Only IPv4/v6 supported \r\n", 0, 0, 0);
        return FALSE;
    }

    return TRUE;
}

static void nt_dpm_send_if_add_event(device_t *dev, void *buffer, struct netif *netif)
{
    if (netif) {
        netif_set_link_callback(netif, data_svc_netif_link_change);
        netif_set_status_callback(netif, data_svc_netif_callback_function);
        nt_dpm_save_ip_param_from_apps(dev, buffer, netif);
    }

    if (g_Cmd_Translation_wifi_hndl.msg_struct.event_notify) {
        wifi_if_add_event if_add_evt;
        memset(&if_add_evt, 0x0, sizeof(wifi_if_add_event));

        if (netif) {
            if_add_evt.net_id = netif_get_index(netif);
            if_add_evt.status = 0;
        } else {
            if_add_evt.status = 1;
        }

        g_Cmd_Translation_wifi_hndl.msg_struct.event_notify(eWiFiSuccess, wifi_interface_add_event_id, &if_add_evt);
    } else {
        NT_LOG_DPM_WARN("event_notify not registered", 0, 0, 0);
    }
}
#endif /* SUPPORT_RING_IF */

#ifndef SUPPORT_RING_IF
static bool nt_dpm_ip_addr_ready_ind(uint8_t netif_id, uint8_t dhcp_type, uint8_t ip_ver, uint32_t ipv4_addr,
                                     const uint32_t *ipv6_addr)
{
    WMI_IP_DDR_EVT ip_ready_evt;

    ip_ready_evt.netif_id = netif_id;
    ip_ready_evt.dhcp_type = dhcp_type;
    ip_ready_evt.ip_ver = ip_ver;
    ip_ready_evt.ipv4_addr = ipv4_addr;

    if (NULL != ipv6_addr) {
        ip_ready_evt.ipv6_addr[0] = ipv6_addr[0];
        ip_ready_evt.ipv6_addr[1] = ipv6_addr[1];
        ip_ready_evt.ipv6_addr[2] = ipv6_addr[2];
        ip_ready_evt.ipv6_addr[3] = ipv6_addr[3];
    }

    wlan_wmi_local_evt_notification(WMI_IP_ADDR_READY_EVTID, (void *)&ip_ready_evt, sizeof(ip_ready_evt));
    return TRUE;
}

static void nt_dpm_netif_link_change_hdlr(struct netif *p_netif)
{
    if (p_netif == NULL) {
        return;
    }
}

static void nt_dpm_netif_dhcp_hdlr(struct netif *p_netif)
{
    if (p_netif != NULL) {
        // Check for DHCP assigned address
        if ((netif_dhcp_data(p_netif) != NULL)) {
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
                if (ip_32 != last_ip_32) {
                    last_ip_32 = ip_32;
                    nt_dpm_ip_addr_ready_ind(netif_id, NETIF_IP_TYPE_DYNAMIC, NETIF_IP_VER_V4, ip_32, NULL);
                }
                return;
            } else {
                last_ip_32 = 0;
                struct dhcp *dhcp = netif_dhcp_data(p_netif);
                NT_LOG_PRINT(COMMON, ERR, "DHCP v4 indication, dhcp state:%d\r\n", dhcp->state);
            }
        }
#if LWIP_IPV6_DHCP6
        else if ((netif_dhcp6_data(p_netif) != NULL)) {
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
                nt_dpm_ip_addr_ready_ind(netif_id, NETIF_IP_TYPE_DYNAMIC, NETIF_IP_VER_V6, 0, p_ipv6->addr);

                return;
            } else {
                NT_LOG_PRINT(COMMON, ERR, "DHCP v6 indication FAIL\r\n");
            }
        }
#endif

        // check for ipv4 link local address
#if 0
        if (netif_autoip_data(p_netif) != NULL)
        {
            uint8_t netif_id;
            struct autoip* p_autoip_struct;
            ip4_addr_t ll_addr;
            netif_id=netif_get_index(p_netif);
            p_autoip_struct=netif_autoip_data(p_netif);
            ll_addr=p_autoip_struct->llipaddr;

            nt_dpm_ip_addr_ready_ind(netif_id, NETIF_IP_TYPE_LOCAL, NETIF_IP_VER_V4,ll_addr.addr,NULL);
        }
        //check ipv6 link local address
        else
#endif
        {
#if LWIP_IPV6
            // link local address for ipv6 is allways stored in index 0
            uint8_t netif_id;
            const ip6_addr_t *p_ipv6;
            netif_id = netif_get_index(p_netif);
            p_ipv6 = netif_ip6_addr(p_netif, 0);

            nt_dpm_ip_addr_ready_ind(netif_id, NETIF_IP_TYPE_LOCAL, NETIF_IP_VER_V6, 0, p_ipv6->addr);
            return;
#endif
        }
    }
}

void nt_dpm_netif_link_change(struct netif *p_netif)
{
    tcpip_try_callback((tcpip_callback_fn)nt_dpm_netif_link_change_hdlr, p_netif);
}

void nt_dpm_netif_callback_function(struct netif *p_netif)
{
    tcpip_try_callback((tcpip_callback_fn)nt_dpm_netif_dhcp_hdlr, p_netif);
}

void nt_dpm_netif_add_event(device_t *dev, struct netif *netif)
{
    WMI_NETIF_ADD_EVT if_add_evt;

    (void)dev;

    if (netif) {
        if_add_evt.net_id = netif_get_index(netif);
        if_add_evt.status = 0;
    } else {
        if_add_evt.net_id = 0;
        if_add_evt.status = 1;
    }

    wlan_wmi_local_evt_notification(WMI_NETIF_ADD_EVTID, (void *)&if_add_evt, sizeof(if_add_evt));

    return;
}
#endif

/**
 * @ingroup network_al
 * Adding device to stack allocating netif  and setting respective field bringing up the network interface.
 */
#ifdef SUPPORT_RING_IF
void nt_dpm_add_dev_to_stack(device_t *dev, void *buffer)
#else
void nt_dpm_add_dev_to_stack(device_t *dev)
#endif
{
    uint8_t role = dev->role;
#ifdef SUPPORT_RING_IF
    NT_BOOL ringif_ip_parm_valid = nt_dpm_copy_ip_params(dev, buffer);
    if (FALSE == ringif_ip_parm_valid) {
        nt_dpm_send_if_add_event(dev, buffer, NULL);
        return;
    }
#endif /* SUPPORT_RING_IF */

#ifdef LWIP
    struct netif *netif;

    netif = nt_osal_calloc(1, sizeof(struct netif));
    if (netif == NULL) {
#ifdef SUPPORT_RING_IF
        nt_dpm_send_if_add_event(dev, buffer, NULL);
#else
        nt_dpm_netif_add_event(dev, NULL);
#endif /* SUPPORT_RING_IF */
        NT_LOG_DPM_CRIT("Allocation Failed\r\n", 0, 0, 0);
        return;
    }

    if (role >= MAX_ROLE) {
        NT_LOG_DPM_CRIT("Invalid device role\r\n", 0, 0, 0);
        return;
    }

#if NT_FN_DHCPS_V4 && LWIP_DHCP
    NT_BOOL is_server = 0;
    if (role == AP_DEVICE) {
        is_server = 1;
    } else if (role == STA_DEVICE) {
        is_server = 0;
    }
#endif /* NT_FN_DHCPS_V4 && LWIP_DHCP */

#if NO_SYS /* NO_SYS */
    netif_add(netif, &_ip_address[role], &netmask[role], &default_gw[role], (void *)dev, nt_ethernetif_init, ip_input);
#else /* NO_SYS */
#if LWIP_IPV4 || LWIP_IPV6
#if LWIP_IPV4
    if (IPADDR_TYPE_V4 == nt_dpm_get_ip_ver(netif)) {
        netif_add(netif, (const ip4_addr_t *)ip_2_ip4(&_ip_address[role]), (const ip4_addr_t *)ip_2_ip4(&netmask[role]),
                  (const ip4_addr_t *)ip_2_ip4(&default_gw[role]), (void *)dev, nt_ethernetif_init, tcpip_input);
#if LWIP_IPV6

        netif->ip6_autoconfig_enabled = 1;
        netif_create_ip6_linklocal_address(netif, 1);
        netif_ip6_addr_set(netif, 1, (const ip6_addr_t *)ip_2_ip6(&ip_address[role]));
        netif_ip6_addr_set_state(netif, 1, IP6_ADDR_VALID);

#if LWIP_IPV6_SEND_ROUTER_ADVERTISE
        if (is_server) {
            ip6_addr_t addr;
            ip6addr_aton("2001:db8::1", &addr); /* cfg from cmd or conf, by default use 2001:db8 prefix */
            netif_ip6_addr_set(netif, 1, &addr);
            netif->ip6_autoconfig_enabled = 1;
            netif_ip6_addr_set_state(netif, 1, IP6_ADDR_TENTATIVE);  // set to tentative will active the address
        }
#endif /* LWIP_IPV6_SEND_ROUTER_ADVERTISE */

#endif /* LWIP_IPV6 */
    }
#endif /* LWIP_IPV4 */
#if LWIP_IPV6
    if (IPADDR_TYPE_V6 == nt_dpm_get_ip_ver(netif)) {
        netif_add_noaddr(netif, (void *)dev, nt_ethernetif_init, tcpip_input);
        netif->ip6_autoconfig_enabled = 1;
        netif_create_ip6_linklocal_address(netif, 1);
        netif_ip6_addr_set(netif, 1, (const ip6_addr_t *)ip_2_ip6(&ip_address[role]));
        netif_ip6_addr_set_state(netif, 1, IP6_ADDR_VALID);
    }
#endif /* LWIP_IPV6 */
#endif /* LWIP_IPV4 && LWIP_IPV6 */
#endif /* NO_SYS */
    if (role == AP_DEVICE) {
        nt_dpm_netif_set_link_up(netif);
    }
    dev->netif = netif;
    netifapi_netif_set_default(netif);
#if NT_FN_DHCPS_V4 && LWIP_DHCP
    if (IPADDR_TYPE_V4 == nt_dpm_get_ip_ver(netif)) {
        if (*((uint8_t *)(nt_devcfg_get_config(NT_DEVCFG_DEFAULT_DHCP_ENABLE))))
            nt_dpm_enable_disable_dhcp(netif, is_server, TURN_ON_DHCP, 0);
        else
            nt_dpm_enable_disable_dhcp(netif, is_server, TURN_OFF_DHCP, 0);
    }
#endif /* NT_FN_DHCPS_V4 && LWIP_DHCP */
#if NT_FN_DHCP6 && LWIP_IPV6_DHCP6
    if (IPADDR_TYPE_V6 == nt_dpm_get_ip_ver(netif)) {
        dhcp6_enable_stateless(netif);
    }
#endif
#ifdef SUPPORT_RING_IF
    nt_dpm_send_if_add_event(dev, buffer, netif);
#else
    netif_set_link_callback(netif, nt_dpm_netif_link_change);
    netif_set_status_callback(netif, nt_dpm_netif_callback_function);
    nt_dpm_netif_add_event(dev, netif);
#endif /* SUPPORT_RING_IF */
#endif
}

/**
 * @ingroup network_al
 * Remove device from stack deallocate netif  and setting respective field to bring down the network interface.
 */
void nt_dpm_remove_dev_from_stack(device_t *dev)
{
#ifdef LWIP
    struct netif *netif;
#if NT_FN_DHCPS_V4 && LWIP_DHCP
    uint8_t role = dev->role;
#endif  // NT_FN_DHCPS_V4 && LWIP_DHCP
    netif = dev->netif;

#if NT_FN_DHCPS_V4 && LWIP_DHCP
    NT_BOOL is_server = 0;
    if (role == AP_DEVICE) {
        is_server = 1;
    } else if (role == STA_DEVICE) {
        is_server = 0;
    }
#endif /* NT_FN_DHCPS_V4 && LWIP_DHCP */
#if NT_FN_DHCPS_V4 && LWIP_DHCP
    nt_dpm_enable_disable_dhcp(netif, is_server, TURN_OFF_DHCP, 0);
#endif /* NT_FN_DHCPS_V4 && LWIP_DHCP */
#if NT_FN_DHCP6 && LWIP_IPV6_DHCP6
    dhcp6_disable(netif);
#endif
    netif_remove(netif);

    netifapi_netif_set_default(NULL);
    dev->netif = NULL;

    nt_osal_free_memory(netif);
#endif
}

#if NT_FN_DHCPS_V4 && LWIP_DHCP
/**
 * @ingroup network_al
 * API to enable/disable DHCP.
 *
 * @param pointer to netif on which dhcp server/client need to be enabled.
 * @param To select server/client :: if is_server = 1 start as server else if is_server = 0 start as client.
 * @param state of the DHCP (TURN_ON_DHCP/TURN_OFF_DHCP).
 * @param To set the value of dhcp_enable from CLI or from dev_config.
 *
 * @return NT_OK on success.
 *
 */
uint8_t nt_dpm_enable_disable_dhcp(struct netif *netif, NT_BOOL is_server, uint8_t state, __unused uint8_t cmd_cli)
{
    uint8_t err = NT_EFAIL;
    uint8_t role = 0, dhcp_enable = TURN_OFF_DHCP;

    if (netif == NULL) {
        NT_LOG_DPM_ERR("netif == NULL \r\n", 0, 0, 0);
        return NT_EIF;
    }

    role = ((device_t *)netif->state)->role;
    dhcp_enable = state;

    if ((role == AP_DEVICE) && is_server) {
        NT_BOOL status = FALSE;

        /* IP address for the device in AP mode before starting DHCP */
        ip_addr_t apipaddr = IPADDR4_INIT_BYTES(192, 168, 0, 1);       // IPV4_adderss for netif (AP)
        ip_addr_t net_mask_ap = IPADDR4_INIT_BYTES(255, 255, 255, 0);  // IPV4_netmask adderss for netif (AP)

        if (dhcp_enable == TURN_ON_DHCP) {
            netif_set_addr(netif, (const ip4_addr_t *)ip_2_ip4(&apipaddr), (const ip4_addr_t *)ip_2_ip4(&net_mask_ap),
                           (const ip4_addr_t *)ip_2_ip4(&apipaddr));
            status = nt_ap_dhcps_start(netif);
            if (status != TRUE) {
                NT_LOG_DPM_ERR("DHCP server start failed\r\n", 0, 0, 0);
                goto Default;
            }
            err = NT_OK;
            NT_LOG_DPM_INFO("DHCP server start success\r\n", 0, 0, 0);
        } else if (dhcp_enable == TURN_OFF_DHCP) {
            if (nt_dhcps_netif_status(netif) == DHCP_STARTED) {
                status = nt_ap_dhcps_stop(netif);
                if (status != TRUE) {
                    NT_LOG_DPM_ERR("DHCP server stop failed\r\n", 0, 0, 0);
                } else {
                    err = NT_OK;
                    NT_LOG_DPM_INFO("DHCP server stop success\r\n", 0, 0, 0);
                    goto Default;
                }
            }
        } else {
            NT_LOG_DPM_INFO("Invalid command for DHCP Server\r\n", 0, 0, 0);
        }
    } else if ((role == STA_DEVICE) && !is_server) {
        err_t status;

        if (dhcp_enable == TURN_ON_DHCP) {
            netif_set_addr(netif, IP4_ADDR_ANY4, IP4_ADDR_ANY4, IP4_ADDR_ANY4);
            status = dhcp_start(netif);
            if (status != ERR_OK) {
                NT_LOG_DPM_ERR("DHCP client start failed\r\n", 0, 0, 0);
                goto Default;
            }
            err = NT_OK;
            NT_LOG_DPM_INFO("DHCP client start success\r\n", 0, 0, 0);
        } else if (dhcp_enable == TURN_OFF_DHCP) {
            status = dhcp_release(netif);
            if (status != ERR_OK) {
                NT_LOG_DPM_ERR("DHCP client release failed\r\n", 0, 0, 0);
            } else {
                err = NT_OK;
                NT_LOG_DPM_INFO("DHCP client release success\r\n", 0, 0, 0);
                goto Default;
            }
        } else {
            NT_LOG_DPM_INFO("Invalid command for DHCP Server\r\n", 0, 0, 0);
        }
    } else {
        NT_LOG_DPM_ERR("DHCP Start/stop Failed as mode of operation is not correct\r\n", 0, 0, 0);
    }

    return err;

Default:
    netif_set_addr(netif, (const ip4_addr_t *)ip_2_ip4(&_ip_address[role]),
                   (const ip4_addr_t *)ip_2_ip4(&netmask[role]), (const ip4_addr_t *)ip_2_ip4(&default_gw[role]));
    if (err != NT_OK)
        return NT_ECANCELED;
    else
        return err;
}
#endif /* NT_FN_DHCPS_V4 && LWIP_DHCP */

uint8_t nt_get_netifidx_by_devmode(u8_t devid)
{
    struct netif *netif;

    if (devid <= AP_DEVICE) {
        NETIF_FOREACH(netif)
        {
            if (devid == ((device_t *)netif->state)->role) {
                return netif->num + 1; /* found! */
            }
        }
    }

    return 0;
}
/**
 * @ingroup network_al
 * API to check if IPv4 is being used
 *
 * @param pointer to netif on which dhcp server/client need to be enabled.
 *
 * @return NT_BOOL TRUE if ip_v4 is being used
 *
 */
uint8_t nt_dpm_get_ip_ver(struct netif *p_netif)
{
#ifdef SUPPORT_RING_IF
    uint8_t apps_cfg_idx = (p_netif ? GET_APPS_CONFIG_INDEX(p_netif->num) : 0);

    if (NULL != g_apps_ip_config[apps_cfg_idx]) {
        if (IPADDR_TYPE_V6 == g_apps_ip_config[apps_cfg_idx]->ip_ver) {
            return IPADDR_TYPE_V6;
        } else {
            return IPADDR_TYPE_V4;
        }
    }
#else  /* SUPPORT_RING_IF */
    (void)p_netif;
#endif /* SUPPORT_RING_IF */

#if LWIP_IPV4
    return IPADDR_TYPE_V4;
#else  /* LWIP_IPV4 */
    return IPADDR_TYPE_V6;
#endif /* LWIP_IPV4 */
}

/**
 * @ingroup network_al.c
 * API to get ip information from current netif.
 */
#ifdef NT_FN_DHCPS_V4
NT_BOOL
nt_dpm_get_ip_info(uint8_t if_index, struct ip_info *info)
{
    NT_BOOL val = FALSE;
    struct netif *netif = NULL;
    memset(info, 0, sizeof(struct ip_info));

    netif = netif_get_by_index(if_index);
    if (netif == NULL) {
        return val;
    }

    info->ip = *nt_dpm_get_ip(netif, IPADDR_TYPE_V4, IPv4_IP_IDX);
    info->netmask = *nt_dpm_get_ip(netif, IPADDR_TYPE_V4, IPv4_NETMASK_IDX);
    info->gw = *nt_dpm_get_ip(netif, IPADDR_TYPE_V4, IPv4_GATEWAY_IDX);
    if (!ip_addr_isany_val(info->ip)) {
        if (!ip_addr_isany_val(info->netmask)) {
            if (!ip_addr_isany_val(info->gw)) {
                val = TRUE;
            }
        }
    }
    return val;
}
#endif  // NT_FN_DHCPS_V4
/**
 * @ingroup network_al.c
 * API to get operation mode of the neutrino device.
 */
uint8_t nt_get_opmode(void)
{
    device_t *dev;

    dev = pnDpA->neutrino_device;

    if (dev->role == AP_DEVICE) {
        return AP_MODE;
    } else if (dev->role == STA_DEVICE)
        return STA_MODE;
    else {
        return NT_EFAIL;
    }
}

void nt_dpm_stop_network_stack()
{
    if (NULL == ntSleepSemaphoreHandle) {
        NT_LOG_WIFI_APP_ERR("StopNtwkStk: SleepSemaphore NULL", 0, 0, 0);
    } else {
        /* To be safe, attempt to give the semaphore once just in case if it was already taken */
        if (nt_fail == nt_osal_semaphore_give(ntSleepSemaphoreHandle)) {
            NT_LOG_WIFI_APP_INFO("SleepSemaphore give failed as expected", 0, 0, 0);
        } else {
            NT_LOG_WIFI_APP_INFO("Warn: SleepSemaphore give passed in stop", 0, 0, 0);
        }

        /* Now take the semaphore so that network stack is paused to send more packets */
        if (nt_fail == nt_osal_semaphore_take(ntSleepSemaphoreHandle, 0)) {
            NT_LOG_WIFI_APP_INFO("StopNtwkStk: SleepSemaphore take failed", 0, 0, 0);
        }
    }

    tcpip_callback((tcpip_callback_fn)sys_timeouts_pause_all, NULL);
}

void nt_dpm_start_network_stack()
{
    /* Take back the semaphore so that network stack can now acquire it to send packets */
    if (NULL != ntSleepSemaphoreHandle) {
        if (nt_fail == nt_osal_semaphore_give(ntSleepSemaphoreHandle)) {
            NT_LOG_WIFI_APP_INFO("StartNtwkStk: SleepSemaphore give failed", 0, 0, 0);
        }
    } else {
        NT_LOG_WIFI_APP_ERR("StartNtwkStk: SleepSemaphore NULL", 0, 0, 0);
    }

    tcpip_callback((tcpip_callback_fn)sys_timeouts_unpause_all, NULL);
}

void nt_dpm_netif_set_link_up(struct netif *netif)
{
    tcpip_callback((tcpip_callback_fn)netif_set_link_up, netif);
    return;
}

#if defined(LWIP)

#elif defined(FREERTOS_TCP_IP)

BaseType_t xNetworkInterfaceInitialise(void)
{
    BaseType_t status = pdPASS;
    /*
    if(nwlan_dpm_interface_up(pnDpA) == NDP_PASS) {
        status = pdTRUE;
    } else {
        status = pdFALSE;
    }
     */
    return status;
}

BaseType_t xNetworkInterfaceOutput(NetworkBufferDescriptor_t *const pxDescriptor, BaseType_t bReleaseAfterSend)
{
    uint8_t *buff_ptr;
    uint32_t length;

    buff_ptr = pxDescriptor->pucEthernetBuffer;
    length = pxDescriptor->xDataLength;

    nwlan_dpm_process_eth_packet_from_stack((void *)buff_ptr, length);

    if (bReleaseAfterSend != pdFALSE) {
        pxDescriptor->pucEthernetBuffer = NULL;
        vReleaseNetworkBufferAndDescriptor(pxDescriptor);
    }
    return pdTRUE;
}
#endif

#if defined(SUPPORT_RING_IF) || defined(CONFIG_WMI_EVENT)
nt_status_t get_netif_hwaddr_from_netif_id(uint8_t netif_id, uint8_t *addr)
{
    struct netif *netif;
    if (addr == NULL)
        return NT_EINVPTR;
    netif = netif_get_by_index(netif_id);
    if (NULL == netif)
        return NT_EINVPTR;
    memscpy(addr, IEEE80211_ADDR_LEN, netif->hwaddr, IEEE80211_ADDR_LEN);
    return NT_OK;
}

/**
 * @ingroup network_al
 * API to check if static IP address is being used
 *
 * @param pointer to netif on which dhcp server/client need to be enabled.
 *
 * @return uint8_t  (IP address type)
 *
 */
uint8_t nt_dpm_get_ip_type(struct netif *p_netif)
{
    uint8_t apps_cfg_idx = (p_netif ? GET_APPS_CONFIG_INDEX(p_netif->num) : 0);

    if (NULL != g_apps_ip_config[apps_cfg_idx]) {
        return g_apps_ip_config[apps_cfg_idx]->dhcp_type;
    }

    if (p_netif == NULL) {
#if defined(SUPPORT_RING_IF)
        return RINGIF_IP_TYPE_LOCAL;
#else
        return NETIF_IP_TYPE_LOCAL;
#endif
    }

#if LWIP_DHCP
    {
        struct dhcp *dhcp = netif_dhcp_data(p_netif);
        NT_BOOL state = (dhcp != NULL && dhcp->state != DHCP_STATE_OFF) ? TRUE : FALSE;

        if (TRUE == state) {
            NT_LOG_DPM_ERR("DHCP ON\r\n", dhcp->state, 0, 0);
#if defined(SUPPORT_RING_IF)
            return RINGIF_IP_TYPE_DYNAMIC;
#else
            return NETIF_IP_TYPE_DYNAMIC;
#endif
        }
    }
#endif

#if LWIP_IPV4 && LWIP_AUTOIP
    {
        struct autoip *autoip = netif_autoip_data(p_netif);
        NT_BOOL state = (autoip != NULL && autoip->state != 0) ? TRUE : FALSE;
        if (TRUE == state) {
            NT_LOG_DPM_ERR("AUTOIP LINK LOCAL ON\r\n", autoip->state, 0, 0);
#if defined(SUPPORT_RING_IF)
            return RINGIF_IP_TYPE_LOCAL;
#else
            return NETIF_IP_TYPE_LOCAL;
#endif
        }
    }
#endif

#if defined(SUPPORT_RING_IF)
    return RINGIF_IP_TYPE_STATIC;
#else
    return NETIF_IP_TYPE_STATIC;
#endif
}

/*
 * Check if NETIF is ready to accept new IP/RawEth connections
 *@param netif   :   The netif structure that has been updated
 */
bool nt_dpm_is_netif_ready(struct netif *p_netif)
{
    if (NULL != p_netif) {
        if (netif_is_link_up(p_netif)) {
            return TRUE;
        }
    }
    return FALSE;
}

/*
 * If netif was already up, set it to down
 *@param netif   :   The netif structure that has been updated
 */
void nt_dpm_netif_set_link_down(struct netif *p_netif)
{
    if (FALSE == nt_dpm_is_netif_ready(p_netif)) {
        return;
    }
    tcpip_callback((tcpip_callback_fn)netif_set_link_down, p_netif);
}
#else
void nt_dpm_netif_set_link_down(struct netif *netif)
{
    tcpip_callback((tcpip_callback_fn)netif_set_link_down, netif);
    return;
}
#endif  // SUPPORT_RING_IF

/*
 * If netif was already up, wait till it is awake
 *@param netif   :   The netif structure of interest
 */
void nt_dpm_wait_till_device_wakeup(void)
{
    if (NULL == ntSleepSemaphoreHandle) {
        NT_LOG_WIFI_APP_ERR("Sleep Semaphore not initialized", 0, 0, 0);
        return;
    }

    /* Waits until wakeup is done */
    if (nt_fail == nt_osal_semaphore_take(ntSleepSemaphoreHandle, portMAX_DELAY)) {
        NT_LOG_WIFI_APP_INFO("Sleep Semaphore take failed", 0, 0, 0);
    }

    /* Release immediately so that sleep is not effected */
    if (nt_fail == nt_osal_semaphore_give(ntSleepSemaphoreHandle)) {
        NT_LOG_WIFI_APP_INFO("Sleep Semaphore give failed", 0, 0, 0);
    }

    return;
}

/*
 * check if MAC ID belongs to any of the connected stations
 *@param netif   :   The netif structure of interest
 */
bool nt_dpm_macid_connected_chk(struct netif *p_netif, uint8 *mac_addr)
{
    if ((NULL == mac_addr) || (NULL == p_netif->hwaddr)) {
        NT_LOG_WIFI_APP_ERR("nt_dpm_macid_connected_chk Fail NULL addr", 0, 0, 0);
        return FALSE;
    }

    if (MAC_EQUAL(p_netif->hwaddr, mac_addr)) {
        NT_LOG_WIFI_APP_ERR("nt_dpm_macid_connected_chk same as device addr", 0, 0, 0);
        return FALSE;
    }

    if (NULL == nt_dpm_find_sta_entry(pnDpA, mac_addr)) {
        NT_LOG_WIFI_APP_ERR("nt_dpm_macid_connected_chk addr not found in list", 0, 0, 0);
        return FALSE;
    }
    return TRUE;
}


#if defined(LWIP)
/**
 * @brief Checks if there are pending TCP Delayed ACKs.
 *        Called from vPortSuppressTicksAndSleep to decide if sleep should be aborted.
 */
bool nt_tcp_has_pending_acks(void)
{
    struct tcp_pcb *pcb;
    /* Safe to access tcp_active_pcbs here because:
     * 1. We are in Scheduler Suspended state.
     * 2. tcpip_thread is blocked and has released the core lock.
     */
    for (pcb = tcp_active_pcbs; pcb != NULL; pcb = pcb->next) {
        if (pcb->flags & TF_ACK_DELAY) {
            return TRUE;
        }
    }
    return FALSE;
}

/**
 * @brief Callback to flush ACKs, running in tcpip_thread context.
 */
void nt_tcp_flush_acks_cb(void *ctx)
{
    struct tcp_pcb *pcb;
    (void)ctx;
    
    /* tcpip_thread holds the lock when calling this callback */
    for (pcb = tcp_active_pcbs; pcb != NULL; pcb = pcb->next) {
        if (pcb->flags & TF_ACK_DELAY) {
            tcp_ack_now(pcb);
            tcp_output(pcb);
            tcp_clear_flags(pcb, TF_ACK_DELAY | TF_ACK_NOW);
        }
    }
}
#endif /* LWIP */
