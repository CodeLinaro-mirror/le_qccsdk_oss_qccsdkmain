/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _WLAN_DEV_API_H_
#define _WLAN_DEV_API_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "wlan_bss.h"

struct devh_s;

nt_status_t wlan_dev_start(struct devh_s *dev, bss_t *bss, channel_t *ch, CSERV_COMPLETION_CB startCb);
void wlan_dev_stop(struct devh_s *dev);
nt_status_t wlan_dev_conn_start(struct devh_s *dev);

NT_BOOL wlan_dev_no_ap_mode_dev(devh_t *dev);

#ifdef SUPPORT_COEX
// TODO: Move to coex header files
void coex_dev_mgr_cb(devh_t *dev, wlan_dev_event *event);
void coex_update_wlanstate(devh_t *dev, uint32_t pInput, void *pInput1, void *pInput2);
#endif /* SUPPORT_COEX */
#ifdef SUPPORT_EVENT_HANDLERS
void wlan_dev_deliver_event(devh_t *dev, uint32_t event_type, channel_t *chan, bss_t *bss);
nt_status_t wlan_dev_register_event_handler(devh_t *dev, wlan_dev_event_handler evhandler);
nt_status_t wlan_dev_unregister_event_handler(devh_t *dev, wlan_dev_event_handler evhandler);
#endif /* SUPPORT_EVENT_HANDLERS */

#ifdef SUPPORT_5GHZ
uint16_t nt_wifi_get_set_freq(uint16_t freq, NT_BOOL set_freq);
void nt_wifi_set_channel_idx(uint8_t chidx);
WLAN_PHY_MODE nt_wifi_get_phymode(void);
void nt_wifi_set_phymode(WLAN_PHY_MODE phymode);
#endif
void nt_wifi_get_set_country_code(char *code, NT_BOOL set);
#ifdef SUPPORT_SAP_POWERSAVE
uint16_t nt_wifi_get_set_sap_bcn_interval(uint16_t beacon_interval, NT_BOOL set_bcn_intv);
#endif
#ifdef __cplusplus
}
#endif

#endif /* _WLAN_DEV_API_H_ */
