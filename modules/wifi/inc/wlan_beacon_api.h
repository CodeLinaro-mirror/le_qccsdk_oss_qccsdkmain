/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _WLAN_BEACON_API_H_
#define _WLAN_BEACON_API_H_

#include "wlan_dev.h"

#define PERIODIC_TSF_PROBE_REQ_RETRY_LIMIT 1

void nt_wlan_beacon_init(devh_t *);
nt_status_t wlan_send_probe_req(devh_t *dev, uint8_t *dst_addr, uint8_t *bssid, ssid_t *ssid_info);
NT_BOOL wlan_send_probe_resp(devh_t *dev, uint8_t *dstAddr);
nt_status_t wlan_beacon_probe_recv_mgmt(devh_t *dev, uint8_t *bufPtr, uint16_t bufLen, int32_t subtype, uint8_t rssi);
#ifdef NT_FN_WMM
NT_BOOL wlan_parse_wmm_info(devh_t *dev, uint8_t *ie, NT_BOOL *uapsd);
NT_BOOL wlan_parse_wmm_param(devh_t *dev, uint8_t *ie, struct chanAccParams *chanParams, NT_BOOL *uapsd);
#endif  // NT_FN_WMM
uint16_t wlan_get_listen_interval(devh_t *dev, uint16_t beaconInterval);
uint16_t wlan_get_assoc_listen_interval(devh_t *dev, uint16_t beaconInterval);
void wlan_beacon_connection_notify(devh_t *dev, NT_BOOL connected, uint16_t beacon_interval);
#ifdef SUPPORT_COEX
void coex_bmiss_monitor(devh_t *dev, uint8_t beacon_event);
void coex_set_bmiss_threshold(uint32_t value);
#endif
#ifdef SUPPORT_PERIODIC_TSF_SYNC
void wlan_queue_tsf_probe_req(devh_t *dev, tsf_periodic_sync_ctx_t *p_tsf_sync_ctx);
#endif
void nt_wlan_beacon_deinit(devh_t *dev, void **wlan_beacon_buffer_inst);

#if defined(SUPPORT_SAP_POWERSAVE)
/**
 * brief   : Add qcn ie for sap ps case with the next tbtt
 * param   : @dev   : the device pointer
 *         : @frm   : Pointer to frame
 *         : @len   : Frame length
 * return  : NT_OK if qcn ie is added sucessfully else failure
 */
nt_status_t add_next_tbtt_for_sap_ps(devh_t *dev, uint8_t *frm, uint16_t *len);
#endif

#endif /* _WLAN_BEACON_API_H */
