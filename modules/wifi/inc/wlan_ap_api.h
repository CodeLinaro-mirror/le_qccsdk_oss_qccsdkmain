/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _WLAN_AP_API_H_
#define _WLAN_AP_API_H_

#include "wlan_dev.h"
#include "suppl_auth_api.h"

void ap_set_protection(devh_t *dev, WLAN_PHY_MODE phy_mode, uint8_t aid, NT_BOOL sta_join);
void ap_profile_commit(devh_t *dev, conn_profile_t *cp, uint8_t channel);
void ap_profile_remove(devh_t *dev);
void ap_remove_all_sta(devh_t *dev, uint8_t reason, uint16_t fType, NT_BOOL removeBss);
void ap_clear_all_sta(devh_t *dev, uint8_t reason, uint16_t fType);
void ap_connect_event(devh_t *dev, bss_t *bss, conn_t *conn, NT_BOOL base);
void ap_set_mlme(devh_t *dev, WMI_AP_SET_MLME_CMD *mlme);
void ap_set_pvb(devh_t *dev, WMI_AP_SET_PVB_CMD *buffer);

uint8_t nt_ap_get_assoc_id(devh_t *dev, uint8_t hal_sta_idx);
void ap_check_sta_inactivity(TimerHandle_t thandle);
void ap_disconnect_event(devh_t *dev, conn_t *conn, NT_BOOL bssConn);
nt_status_t ap_alloc_bss_start_ap(devh_t *dev, channel_t *ch);
void *nt_ap_init(devh_t *dev);
void nt_ap_deinit(devh_t *dev);

NT_BOOL ap_check_ps_sta(uint8_t *ps_sta_list, uint8_t ps_sta_cnt, uint8_t hal_sta_idx);
void ap_restart(devh_t *dev, uint16_t channel);
void ap_teardown(devh_t *dev, uint8_t removeBss);
void ap_set_phymode(devh_t *dev, uint8_t wmi_phyMode);
void ap_wmi_disconnect_event(devh_t *dev, uint16_t info, WMI_DISCONNECT_REASON reason, uint8_t *bssid,
                             uint8_t assocRespLen, uint8_t *assocRespBuf, uint16_t protoReasonStatus);
void _ap_post_sta_inact_timeout_msg(TimerHandle_t thandle);

/*
 * AP Authenticator callback functions
 */
uint16_t ap_form_eapol_frame(void *, uint8_t *, uint8_t *, uint8_t);
void ap_hs_compl_evt(void *, uint8_t *, WMI_ADD_CIPHER_KEY_CMD *, SUPPL_STATUS);

#endif /* _WLAN_AP_API_H_ */
