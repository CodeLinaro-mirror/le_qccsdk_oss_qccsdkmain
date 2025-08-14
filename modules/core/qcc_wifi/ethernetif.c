/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file
 * Ethernet Interface Skeleton
 *
 */

/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 */

/*
 * This file is a skeleton for developing Ethernet network interface
 * drivers for lwIP. Add code to the low_level functions and do a
 * search-and-replace for the word "ethernetif" to replace it with
 * something that better describes your network interface.
 */

#include "defines.h"
#include "safeAPI.h"

#ifdef LWIP /* don't build, this is only a skeleton, see previous comment */

#include "lwip/opt.h"

#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/stats.h"
#include "lwip/snmp.h"
#include "lwip/ethip6.h"
#include "lwip/etharp.h"
#include "data_path.h"
#include "network_al.h"
#include "nt_osal.h"
#include "nt_logger_api.h"
#include "hal_int_sys.h"
#include "data_path.h"
#ifdef SUPPORT_RING_IF
#include "data_svc_internal_api.h"
#endif
#include "nt_twt.h"
#include "wlan_power.h"
#if defined(FEATURE_STA_ECSA) || defined(FEATURE_AP_ECSA)
#include "ecsa.h"
#endif

extern devh_t *gdevp;

#ifdef NT_FN_RRAM_PERF_BUILD
__attribute__((section(".perf_nc_txt"))) static void nt_low_level_init(struct netif *netif);
__attribute__((section(".perf_tx_txt"))) static err_t nt_low_level_output(struct netif *netif, struct pbuf *p);
#else
void nt_low_level_init(struct netif *netif);
err_t nt_low_level_output(struct netif *netif, struct pbuf *p);
#endif
/* Define those to better describe your network interface. */
//#define IFNAME0 'nwlan1'
//#define IFNAME1 'nwlan2'

#ifdef SUPPORT_DATAPATH_FLUSH_BEFORE_BMPS_SLEEP
/**
 * In this function, checks for topdown wakeup and notifies if required.
 * Called from nt_low_level_output().
 *
 * @param none
 */
void nt_notify_top_down_wakeup_required()
{
    /* Doing a Tops down wake up to process the packet while transitioning
     * to sleep in BMPS by notifying the nt_wlan task.
     */
    bool ptsm_active = FALSE;

#ifdef FEATURE_PERIODIC_WAKE_SLEEP
    ptsm_active = nt_periodic_wake_infra_is_session_active(gdevp);
#endif /* FEATURE_PERIODIC_WAKE_SLEEP */

#ifdef SUPPORT_TWT_STA
    if ((pnDpA->ps_ctrl.dpm_sleep == 1) && (nt_twt_is_negotiated() == FALSE) && nt_pm_is_bmps_enabled() &&
        (ptsm_active == FALSE))
#else
    if (pnDpA->ps_ctrl.dpm_sleep == 1 && nt_pm_is_bmps_enabled() && (ptsm_active == FALSE))
#endif /*SUPPORT_TWT_STA*/
    {
        nt_pm_notify_top_down_wakeup(gdevp);
    }
}
#endif /* SUPPORT_DATAPATH_FLUSH_BEFORE_BMPS_SLEEP */

/**
 * In this function, the hardware should be initialized.
 * Called from ethernetif_init().
 *
 * @param netif the already initialized lwip network interface structure
 *        for this ethernetif
 */
void nt_low_level_init(struct netif *netif)
{
    device_t *dev;
    dev = netif->state;

    netif->hwaddr_len = NT_MAC_ADDR_SIZE;
    memscpy(netif->hwaddr, NT_MAC_ADDR_SIZE, dev->mac_address, NT_MAC_ADDR_SIZE);
    if (dev->role == AP_DEVICE) {
        netif->name[0] = 'a';
        netif->name[1] = 'p';
        netif->flags |=
            NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP | NETIF_FLAG_IGMP | NETIF_FLAG_MLD6;
    } else {
        netif->name[0] = 's';
        netif->name[1] = 't';
        netif->flags |= NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_IGMP | NETIF_FLAG_MLD6;
    }
    /* maximum transfer unit */
    netif->mtu = 1500;

    netif_set_up(netif);  // NETIF_FLAG_UP
}

#ifdef NT_FN_RRAM_PERF_BUILD
err_t
#else
err_t __attribute__((section(".after_ram_vectors")))
#endif
nt_low_level_output(struct netif *netif, struct pbuf *p)
{
    struct pbuf *q;
    void *buf = NULL;
    uint8_t *buf_ptr;
    uint32_t len = 0;
    ethernet_header_t *eth_hdr;
    ip_header_t *ip_header;
    ip6_header_t *ip6_header;
    nt_status_t err;
    uint8_t val = 0;
    uint8_t dpm_tid = 0;
    bool ptsm_active = FALSE;
    NT_BOOL ecsa_active_with_blocking_traffic = FALSE;
#if NT_FN_DPM_WMM
    uint8_t ac;
    nt_dpm_wmm_queue_t *wmm_queue;
#ifdef SUPPORT_RING_IF
    bool b_is_ip_pkt = FALSE;
    uint8_t qos_for_non_ip = MAX_AC_NUM;
#endif
#endif

    (void)netif; /* Avoid compiler warning*/

#ifdef SUPPORT_DATAPATH_FLUSH_BEFORE_BMPS_SLEEP
    /* Tops down wake up will be only valid if system has an active connection with AP */
    if (DEVICE_CONNECTED(gdevp)) {
        nt_notify_top_down_wakeup_required();
    } else {
        NT_LOG_PRINT(DPM, INFO, "Packet dropped, device already disconnected");
        return ERR_CONN;
    }
#endif /* SUPPORT_DATAPATH_FLUSH_BEFORE_BMPS_SLEEP */
    /* Wait till device is up */
    nt_dpm_wait_till_device_wakeup();

    if (p->tot_len > (NT_DPM_MAC_MTU_SIZE + sizeof(ethernet_header_t))) {
        NT_LOG_DPM_ERR("packet total length greater than MTU size ", p->tot_len, 0, 0);
        return ERR_BUF;
    }

    if (p->tot_len < sizeof(ethernet_header_t)) {
        NT_LOG_DPM_ERR("packet total length less than eth_hdr size", p->tot_len, 0, 0);
        return ERR_BUF;
    }

    eth_hdr = (ethernet_header_t *)(p->payload);

    if (eth_hdr->usFrameType == dp_htons(NT_IP_FRAME_TYPE)) {
        ip_header = (ip_header_t *)((uint8_t *)eth_hdr + sizeof(ethernet_header_t));
        dpm_tid = (ip_header->ucDifferentiatedServicesCode >> 5) & 0x7;
    } else if (eth_hdr->usFrameType == dp_htons(NT_IPV6_FRAME_TYPE)) {
        ip6_header = (ip6_header_t *)((uint8_t *)eth_hdr + sizeof(ethernet_header_t));
        dpm_tid = (((dp_ntohl(ip6_header->ulVersionTCFlowLabel) >> 20) & 0xff) & 0x7);
    } else if (eth_hdr->usFrameType == dp_htons(NT_ARP_FRAME_TYPE)) {
        dpm_tid = 0;
    }
#ifdef SUPPORT_RING_IF
    b_is_ip_pkt = is_ip_packet(eth_hdr);
    if (FALSE == b_is_ip_pkt) {
        qos_for_non_ip = data_svc_get_qos_for_non_ip(eth_hdr);
    }
#endif

#ifdef NT_TST_TIME_STAMP_ENABLE
    if (nt_dpm_tm.tx_stat[LOW_LVL_OUTPUT].valid == 0) {
        uint32_t p_len = 0;
        struct pbuf *buf;
        uint32_t offset =
            sizeof(ethernet_header_t) +
            ((eth_hdr->usFrameType == dp_htons(NT_IP_FRAME_TYPE)) ? sizeof(ip_header_t) : sizeof(ip6_header_t));
        if (eth_hdr->usFrameType == dp_htons(NT_IP_FRAME_TYPE)) {
            if (ip_header->ucProtocol == NT_IP_PROTO_ICMP) {
                offset += sizeof(icmp_header_t);
            } else {
                offset += sizeof(udp_header_t);
            }
        } else {
            if (ip6_header->ucNextHeader == NT_IP6_PROTO_ICMP6) {
                offset += sizeof(icmp_header_t);
            } else {
                offset += sizeof(udp_header_t);
            }
        }

        offset += 4;

        if (p->tot_len > offset) {
            buf = p;
            while ((p_len + buf->len) < offset) {
                p_len += buf->len;
                buf = buf->next;
                if (buf == NULL) {
                    nt_dpm_tm_node_reset(TX_TIMESTAMP);
                    return ERR_BUF;
                }
            }
            if (*(uint32_t *)(((uint8_t *)buf->payload) + (offset - p_len)) == nt_dpm_tm.tx_marker) {
                nt_dpm_tm.tx_stat[LOW_LVL_OUTPUT].value = nt_hal_get_curr_time();
                nt_dpm_tm.tx_stat[LOW_LVL_OUTPUT].valid = 1;
                nt_dpm_tm.tx_stat[LOW_LVL_OUTPUT].offset = offset;
            }
        }
    }
#endif

#ifdef FEATURE_PERIODIC_WAKE_SLEEP
    ptsm_active = nt_periodic_wake_infra_is_session_active(gdevp);
#endif /* FEATURE_PERIODIC_WAKE_SLEEP */

#if defined(FEATURE_STA_ECSA) || defined(FEATURE_AP_ECSA)
    ecsa_active_with_blocking_traffic = is_ecsa_active_with_blocking_traffic();
#endif
#ifdef SUPPORT_TWT_STA
    if ((pnDpA->ps_ctrl.dpm_sleep == 1) && (nt_twt_is_negotiated() == FALSE) &&
        (ecsa_active_with_blocking_traffic == FALSE) && (ptsm_active == FALSE))
#else
    if ((pnDpA->ps_ctrl.dpm_sleep == 1) && (ecsa_active_with_blocking_traffic == FALSE) && (ptsm_active == FALSE))
#endif /*SUPPORT_TWT_STA*/
    {
#ifdef SUPPORT_RING_IF
        if ((!b_is_ip_pkt) && qos_for_non_ip < MAX_AC_NUM) {
            val = qos_for_non_ip;
        } else {
            val = pnDpA->tid_to_ac_map.ac_per_tid[dpm_tid];
        }
#else
        val = pnDpA->tid_to_ac_map.ac_per_tid[dpm_tid];
#endif
#ifdef SUPPORT_DATAPATH_FLUSH_BEFORE_BMPS_SLEEP
        if (pnDpA->ps_ctrl.data_available_cb)
            pnDpA->ps_ctrl.data_available_cb(val);
#else
        if (pnDpA->ps_ctrl.ps_pbuf == NULL) {
            pnDpA->ps_ctrl.ps_pbuf = p;
            pbuf_ref(p);
            if (pnDpA->ps_ctrl.data_available_cb)
                pnDpA->ps_ctrl.data_available_cb(val);
            return ERR_OK;
        } else {
            if (pnDpA->ps_ctrl.data_available_cb)
                pnDpA->ps_ctrl.data_available_cb(val);
            return ERR_INPROGRESS;
        }

#endif /* SUPPORT_DATAPATH_FLUSH_BEFORE_BMPS_SLEEP */
    }

#if NT_FN_DPM_WMM
    if ((pnDpA->wmm_enable == NT_DPM_WMM_ENABLE) && ((eth_hdr->usFrameType == dp_htons(NT_IP_FRAME_TYPE)) ||
                                                     (eth_hdr->usFrameType == dp_htons(NT_IPV6_FRAME_TYPE))||
                                                     (eth_hdr->usFrameType == dp_htons(NT_ARP_FRAME_TYPE)))) {
        ac = pnDpA->tid_to_ac_map.ac_per_tid[dpm_tid];
        wmm_queue = &pnDpA->wmm_queue[ac];

        if (p->tot_len + wmm_queue->queue_length < wmm_queue->bsl) {
            if ((wmm_queue->queue_length >= wmm_queue->csl) &&
                ((p->tot_len + pnDpA->total_burst_len) >= (MAX_BURST_HEAP_SIZE - pnDpA->total_csl))) {
                wmm_queue->queue_full_count1++;
                nt_dpm_packet_available_notify(ac);
                return ERR_MEM;
            }
        } else {
            wmm_queue->queue_full_count2++;
            nt_dpm_packet_available_notify(ac);
            return ERR_MEM;
        }
    }
#endif

    buf = nt_dpm_allocate_buffer((uint32_t)(NT_TX_BUFFER_OFFSET + p->tot_len));

    if (buf == NULL) {
#if NT_FN_DPM_WMM
#ifdef SUPPORT_RING_IF
        if ((pnDpA->wmm_enable == NT_DPM_WMM_ENABLE) && (b_is_ip_pkt || qos_for_non_ip < MAX_AC_NUM))
#else
        if ((pnDpA->wmm_enable == NT_DPM_WMM_ENABLE) && ((eth_hdr->usFrameType == dp_htons(NT_IP_FRAME_TYPE)) ||
                                                         eth_hdr->usFrameType == dp_htons(NT_IPV6_FRAME_TYPE)))
#endif
        {
#ifdef SUPPORT_RING_IF
            if (!b_is_ip_pkt && qos_for_non_ip < MAX_AC_NUM) {
                ac = qos_for_non_ip;
            } else {
                ac = pnDpA->tid_to_ac_map.ac_per_tid[dpm_tid];
            }
#else
            ac = pnDpA->tid_to_ac_map.ac_per_tid[dpm_tid];
#endif
            pnDpA->wmm_queue[ac].tx_allocation_failed++;
            nt_dpm_tickle_all_wmm_queues();
        } else
#endif
        {
            NT_LOG_DPM_CRIT("buf allocation failed Out of memory\r\n", 0, 0, 0);
        }
        return ERR_MEM;
    }

    buf_ptr = (uint8_t *)buf + NT_TX_BUFFER_OFFSET;

    for (q = p; (q != NULL) && (len < p->tot_len); q = q->next) {
#ifdef MEM_CPY_VIA_DXE
        if (nt_dpm_memcpy(buf_ptr, q->payload, q->len) == NULL) {
            NT_LOG_DPM_ERR("dpm_memcpy failed\r\n", (uint32_t)buf_ptr, (uint32_t)q->payload, (uint32_t)q->len);
        }
#else
        memscpy(buf_ptr, q->len, q->payload, q->len);
#endif /* MEM_CPY_VIA_DXE */

        buf_ptr += q->len;
        len += q->len;

        if (len > p->tot_len) {
            NT_LOG_DPM_CRIT("pbuf buffer length greater than packet total length\r\n", p->tot_len, len, 0);
            nt_dpm_free_buffer(buf);
            return ERR_BUF;
        }
    }

#ifdef NT_TST_TIME_STAMP_ENABLE
    nt_dpm_read_ts(PKT_FROM_STACK, buf);
#endif

#ifdef SUPPORT_RING_IF
    err = nt_dpm_process_eth_packet_from_stack(buf, len, qos_for_non_ip);
#else
    err = nt_dpm_process_eth_packet_from_stack(buf, len);
#endif

    if (err != NT_OK) {
#ifdef NT_TST_TIME_STAMP_ENABLE
        if (*(uint32_t *)((uint8_t *)buf + NT_TX_BUFFER_OFFSET + nt_dpm_tm.tx_stat[LOW_LVL_OUTPUT].offset) ==
            nt_dpm_tm.tx_marker) {
            nt_dpm_tm.dpm_tx_node_reset_count++;
            nt_dpm_tm_node_reset(TX_TIMESTAMP);
        }
#endif
        LINK_STATS_INC(link.drop);
        nt_dpm_free_buffer(buf); /* Releasing buffer as packet in not transmitted */
        if (err == NT_ECONN) {
            return ERR_CONN; /* using LWIP ERROR constant to notify LWIP stack for state of connection */
        } else if ((err == NT_ENOMEM) || (err == NT_ENORES) || (err == NT_ETXFAIL)) {
            return ERR_MEM; /* using LWIP ERROR constant to notify LWIP stack for error in transmitting */
        } else {
            return ERR_IF;  // TODO: any error log should be added
        }
    }

    LINK_STATS_INC(link.xmit);

    return ERR_OK;
}

/**
 * Should be called at the beginning of the program to set up the
 * network interface. It calls the function low_level_init() to do the
 * actual setup of the hardware.
 *
 * This function should be passed as a parameter to netif_add().
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return ERR_OK if the loopif is initialized
 *         ERR_MEM if private data couldn't be allocated
 *         any other err_t on error
 */
err_t nt_ethernetif_init(struct netif *netif)
{
    LWIP_ASSERT("netif != NULL", (netif != NULL));

#if LWIP_NETIF_HOSTNAME
    /* Initialize interface hostname */
    netif->hostname = "lwip";
#endif /* LWIP_NETIF_HOSTNAME */

    /*
     * Initialize the snmp variables and counters inside the struct netif.
     * The last argument should be replaced with your link speed, in units
     * of bits per second.
     */
    MIB2_INIT_NETIF(netif, snmp_ifType_ethernet_csmacd, LINK_SPEED_OF_YOUR_NETIF_IN_BPS);

    /* We directly use etharp_output() here to save a function call.
     * You can instead declare your own function an call etharp_output()
     * from it if you have to do some checks before sending (e.g. if link
     * is available...) */
#if LWIP_IPV4
    netif->output = etharp_output;
#endif /* LWIP_IPV4 */
#if LWIP_IPV6
    netif->output_ip6 = ethip6_output;
#endif /* LWIP_IPV6 */
    netif->linkoutput = nt_low_level_output;

    /* initialize the hardware */
    nt_low_level_init(netif);

    return ERR_OK;
}

#endif /* 0 */
