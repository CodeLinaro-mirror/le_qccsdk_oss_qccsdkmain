/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __WLAN_WPA_API__
#define __WLAN_WPA_API__
#include "wifi_cmn.h"
#include "wlan_dev.h"
#include "ieee80211_var.h"

uint16_t wlan_wpa_gen_rsn_ie(devh_t *dev, bss_t *bss, conn_profile_t *prof, uint8_t *wpa_ie);
NT_BOOL wlan_wpa_validate_wpa_ie(devh_t *dev, conn_profile_t *prof, struct ieee80211_common_ie *cie,
                                 NT_BOOL ignore_grp_cipher, uint8_t *mcipher, uint8_t *ucipher, uint16_t *authmode);
NT_BOOL wlan_wpa_process_wpa_ie(devh_t *dev, struct ieee80211_common_ie *cie, uint8_t *mcipher, uint8_t *ucipher,
                                uint16_t *authmode);
NT_BOOL wlan_wpa_process_rsn_ie(devh_t *dev, struct ieee80211_common_ie *cie, uint8_t *rsn_cap_flag, uint8_t *mcipher,
                                uint8_t *ucipher, uint16_t *authmode);
#if ((defined NT_FN_RMF) || (defined NT_FN_WPA3))
NT_BOOL wlan_wpa_validate_rsn_ie(devh_t *dev, conn_profile_t *prof, struct ieee80211_common_ie *cie,
                                 NT_BOOL ignore_grp_cipher, uint8_t *rsn_cap_flag, uint8_t *mcipher, uint8_t *ucipher,
                                 uint16_t *authmode);
#else
NT_BOOL wlan_wpa_validate_rsn_ie(devh_t *dev, conn_profile_t *prof, struct ieee80211_common_ie *cie,
                                 NT_BOOL ignore_grp_cipher, __unused uint8_t *rsn_cap_flag, uint8_t *mcipher,
                                 uint8_t *ucipher, uint16_t *authmode);
#endif
NT_BOOL wlan_wpa_wpa2_preauth_capable(devh_t *dev, struct ieee80211_common_ie *cie);
NT_BOOL wlan_wpa_find_grp_cipher(devh_t *dev, uint8_t *ie, uint8_t ie_len, uint8_t *cipher_len, CRYPTO_TYPE *cipher);
uint16_t wlan_wpa_ap_validate_wpa_ie(devh_t *dev, uint8_t *frm, conn_profile_t *prof, uint8_t *sta_cipher);
#if ((defined NT_FN_RMF) || (defined NT_FN_WPA3))
uint16_t wlan_wpa_ap_validate_rsn_ie(devh_t *dev, uint8_t *frm, conn_profile_t *prof, conn_t *conn, uint8_t *sta_cipher,
                                     uint8_t *cap_flag);
#else
uint16_t wlan_wpa_ap_validate_rsn_ie(devh_t *dev, uint8_t *frm, conn_profile_t *prof, conn_t *conn,
                                     uint8_t *sta_cipher);
#endif

#ifdef SUPPORT_11W
NT_BOOL (*_wlan_rsn_check_mfp_cap)(devh_t *dev, conn_t *conn);
#endif

// void rc4_skip(const uint8_t *key, size_t keylen, size_t skip,uint8_t *data, size_t data_len);

void wpa_pmk_to_ptk(devh_t *dev, const uint8_t *pmk, size_t pmk_len, const uint8_t *addr1, const uint8_t *addr2,
                    const uint8_t *nonce1, const uint8_t *nonce2, uint8_t *ptk, size_t ptk_len, uint16_t auth);
void wpa_eapol_key_mic(const uint8_t *key, int ver, const uint8_t *buf, size_t len, uint8_t *mic);
void wpa_gmk_to_gtk(uint8_t *gmk, uint8_t *addr, uint8_t *gnonce, uint8_t *gtk, size_t gtk_len);

#if defined(SUPPORT_RING_IF) || defined(CONFIG_WMI_EVENT)
nt_status_t wpa_find_authmode(struct ieee80211_common_ie *cie, AUTH_MODE *authmode);
AUTH_MODE wlan_wpa_get_auth_algorithm(CRYPTO_TYPE cipher, uint8_t *sel);
#endif

nt_status_t security_ie_parse(devh_t *dev, uint8_t *pie, uint16_t *cipher, uint16_t *auth, uint8_t ie_type);
void wlan_wpa3_tdi_update(devh_t *dev, uint8_t mode);
void wlan_wpa3_tdi_clear(devh_t *dev);
uint8_t wlan_wpa3_tdi_mode(devh_t *dev);

void wlan_wpa_disable_wpa_wpa2(devh_t *dev);
void wlan_wpa_transition_disable(devh_t *dev, uint8_t mode);

#endif /* __WLAN_WPA_API__ */
