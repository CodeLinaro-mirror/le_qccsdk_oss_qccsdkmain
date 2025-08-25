/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __WLAN_FRAMEGEN_INTERNAL_H__
#define __WLAN_FRAMEGEN_INTERNAL_H__

#ifdef ATH_KF
uint8_t *AddWmm(devh_t *dev, uint8_t *frm, struct apsdConfig *apsd, uint8_t *apsdConfigInWmmIe);
#endif  // ATH_KF
uint8_t *AddErp(bss_t *bss, uint8_t *frm, uint16_t *msg_len);
#ifdef NT_FN_WMM
uint8_t *addACParams(uint8_t *frm, uint32_t wmm_ac);
uint8_t *AddWmmParams(devh_t *dev, bss_t *bss, uint8_t *frm);
#endif  // NT_FN_WMM

void FrameInit(struct ieee80211_frame *wh, uint16_t seqno, uint16_t dur_aid, uint8_t fc_0, uint8_t fc_1,
               uint8_t *pAddr1, uint8_t *pAddr2, uint8_t *pAddr3);
#if ((defined NT_FN_RMF) || (defined NT_FN_WPA3))
void ieee80211_mgmt_header(devh_t *dev, uint8_t *bf, uint32_t type, uint8_t *pAddr1, uint8_t *pAddr3,
                           int bHeaderPresent, uint32_t protect_flag);
#else
void ieee80211_mgmt_header(devh_t *dev, uint8_t *bf, uint32_t type, uint8_t *pAddr1, uint8_t *pAddr3,
                           int bHeaderPresent, __attribute__((__unused__)) uint32_t protect_flag);
#endif  // NT_FN_RMF
#if ((defined NT_FN_RMF) || (defined NT_FN_WPA3))
void aad_bip_frame_init(struct ieee80211_aad *wh, uint8_t fc_0, uint8_t fc_1, uint8_t *pAddr1, uint8_t *pAddr2,
                        uint8_t *pAddr3);
#endif

#endif /*__WLAN_FRAMEGEN_INTERNAL_H__*/
