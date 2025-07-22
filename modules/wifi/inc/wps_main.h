/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _WPS_MAIN_H_
#define _WPS_MAIN_H_

#ifndef __weak
#define __weak __attribute__((weak))
#endif

#ifdef __cplusplus
extern "C" {
#endif
#include "wps_def.h"
#ifdef NT_FN_WPS
void wps_do_disconnect(devh_t *dev);
void wps_send_profile_event_to_host(devh_t *dev, WPS_CONTEXT *wps, uint8_t status, uint8_t reason_code);
void wps_pack_ie_for_probereq_assocreq(devh_t *dev, WPS_CONTEXT *wps, NT_BOOL pack);
void wps_pack_ie_for_proberesp_beacon(devh_t *dev, WPS_CONTEXT *wps, NT_BOOL pack);

void wps_start_connect_process(devh_t *dev, WPS_CONTEXT *wps, uint8_t auth_type, uint16_t encr_type,
                               uint8_t pack_wps_ie, WPS_CREDENTIAL *wpsCred);
void wps_retry_timer_handler(TimerHandle_t alarm, void *data);
void wps_send_packet(devh_t *dev, WPS_CONTEXT *wps, uint8_t eapol_type, struct wpsbuf *eap_resp);

void wps_save_remote_info(WPS_CONTEXT *wps, uint8_t *ssid, uint16_t ssid_len, uint8_t *bssid, uint8_t channel,
                          uint8_t auth, uint16_t encrypt);
uint8_t wps_check_input_info(WPS_CONTEXT *wps, uint8_t *ssid, uint16_t ssid_len, uint8_t *bssid, uint8_t channel,
                             uint8_t auth, uint16_t encrypt);
int wps_add_to_bad_ap_list(WPS_CONTEXT *wps, uint8_t *bad_ap_mac);
uint8_t wps_check_for_bad_ap(WPS_CONTEXT *wps, uint8_t *ap_mac);
void wps_parse_ie(devh_t *dev, WPS_CONTEXT *wps, uint8_t *pBuffer, uint16_t length);
void wps_association_complete_event(devh_t *dev);
void wps_clean_state(WPS_CONTEXT *wps);
void wps_start_8way_handshake(devh_t *dev);
void wps_start_scan_process(devh_t *dev);
void wps_walk_timer_handler(TimerHandle_t alarm, void *data);
void wps_search_finished(void *arg, nt_status_t status);
void wps_bcon_start(void);
void wps_yield_processor(void);
void wps_deauth_timer_handler(TimerHandle_t alarm, void *data);
void wps_send_cb(devh_t *dev, uint8_t *bufPtr, nt_status_t status);
void wps_eap_code_failure_timer_handler(TimerHandle_t alarm, void *arg);
nt_status_t wps_eap_code_failure_handle(devh_t *dev);
void wps_pin_lock_timeout_handler(TimerHandle_t alarm, void *arg);
void wmi_wps_start(devh_t *dev, WPS_CONTEXT *wps, WMI_WPS_START_CMD *pWpsStart);
void wmi_wps_set_config(devh_t *dev, WMI_WPS_START_CMD *buf, WPS_CONTEXT *wps);
#endif  // NT_FN_WPS

#ifdef ATH_KF
WMI_FILTER_ACTION wps_event_filter(uint16_t EventId, uint16_t info, uint8_t *pBuffer, int length);
#endif  // ATH_KF

#ifdef __cplusplus
}
#endif

#endif /* _WPS_MAIN_H */
