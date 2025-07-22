/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __WLAN_ENCODE_IE_API_H__
#define __WLAN_ENCODE_IE_API_H__

uint8_t *ieee80211_add_power_caps(uint8_t *, uint8_t);
uint8_t *ieee80211_add_suppchan_ie(uint8_t *, const struct ieee80211_suppchan_ie *);
#ifdef NT_BRINGUP_TEST
uint8_t *ieee80211_add_11b_rates(devh_t *dev, uint8_t *frm);
#endif  // NT_BRINGUP_TEST
uint8_t *ieee80211_add_rates(uint8_t *, struct ieee80211_rateset *);

#if defined(FEATURE_STA_ECSA) || defined(SUPPORT_SAP_POWERSAVE)
uint8_t *ieee80211_add_qcnie(devh_t *dev, uint8_t *frm, uint8_t len);
#endif

uint8_t *ieee80211_add_xrates(uint8_t *, const struct ieee80211_rateset *);
uint8_t *ieee80211_add_ssid(uint8_t *, const uint8_t *, uint8_t);
uint8_t *ieee80211_add_tspec(uint8_t *, WMM_TSPEC_INFO *, uint16_t);

uint8_t *ieee80211_add_ht_cap(uint8_t *frm, const struct ieee80211_ie_htcap *ht_cap);
uint8_t *ieee80211_add_ht_op(uint8_t *, struct ieee80211_ie_htinfo *, const struct ieee80211_ie_htinfo_cmn *);
/**
 *	@func	ieee80211_add_ext_cap
 *	@brief	This function is used to add extended capability IE in the frame.
 * 	@Return pointer to the frame after adding bss max idle time IE
 * 	@Param	frm : pointer to the frm in which IE is to be added
 * 			ext_cap : extended capability
 */
uint8_t *ieee80211_add_ext_cap(uint8_t *frm, struct ieee80211_ie_ext_cap_filed *ext_cap_info);

/**
 *	@func	ieee80211_add_bss_max_idle_time
 *	@brief	This function is used to add bss max idle time IE in the frame.
 * 	@Return pointer to the frame after adding bss max idle time IE
 * 	@Param	frm : pointer to the frm in which IE is to be added
 * 			bss_time : bss max idle time value
 */
uint8_t *ieee80211_add_bss_max_idle_time(uint8_t *frm, uint16_t bss_time);

int32_t IEEE_ieee2freq(int32_t chan, NT_BOOL is_6G);
uint32_t IEEE_chan2ieee(channel_t *chan);

#endif /* __WLAN_ENCODE_IE_API_H__ */
