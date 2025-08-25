/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __WLAN_WPA_INTERNAL__
#define __WLAN_WPA_INTERNAL__

#include <stdint.h>

typedef struct rsn_cipher_akm_s {
    uint8_t mcastcipher;
    uint8_t mcastcipherlen;
    uint8_t ucastcipherset;
    uint8_t ucastcipherlen;
    uint8_t akmset;
} rsn_cipher_akm_t;

#if 0
static uint16_t     wlan_wpa_setup_wpa_ie(rsn_cipher_akm_t *rsn_cipher_akm_info, uint16_t wpa_caps, uint8_t *ie);
#endif
uint16_t wlan_wpa_setup_rsn_ie(devh_t *dev, bss_t *bss, rsn_cipher_akm_t *rsn_cipher_akm_info, uint16_t wpa2_caps,
                               uint8_t *ie);
CRYPTO_TYPE wlan_wpa_cipher_wpa(uint8_t *sel, uint8_t *keylen);
AUTH_MODE wlan_wpa_keymgmt_wpa(devh_t *dev, uint8_t *sel);
NT_BOOL wlan_wpa_cipher_match(CRYPTO_TYPE cipher1, uint8_t cipher1_len, CRYPTO_TYPE cipher2, uint8_t cipher2_len);
CRYPTO_TYPE wlan_wpa_cipher_rsn(uint8_t *sel, uint8_t *keylen);
#ifdef NT_FN_WPA3
AUTH_MODE wlan_wpa_keymgmt_rsn(devh_t *dev, uint8_t *sel);
#else
AUTH_MODE wlan_wpa_keymgmt_rsn(devh_t *dev, uint8_t *sel);
#endif  // NT_FN_WPA3
#endif  /* __WLAN_WPA_INTERNAL__ */
