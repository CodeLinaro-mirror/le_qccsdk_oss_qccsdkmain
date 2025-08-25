/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _WLAN_AP_H_
#define _WLAN_AP_H_

#define WLAN_BROADCAST_ADDR (uint8_t *)"\xff\xff\xff\xff\xff\xff"

#define DEF_AP_DTIM_COUNT  0;
#define DEF_AP_DTIM_PERIOD 2;

#define DEF_AP_COUNTRY_CODE "US "

typedef struct ap_dev_struct {
    TimerHandle_t sta_inact_timer;

    uint8_t countryCode[3];

    uint32_t inact_period;        /* sta inactivity period */
    uint32_t ps_sta_inact_period; /* power save sta inactivity period */
    uint8_t erp_prot_bss;         /* Bit map of B STA */
    uint8_t ht_prot_bss;          /* Bit map of G STA */
    uint8_t profile_commit;

    uint8_t numStaConn;
    uint8_t num_sta;
    uint8_t dtim_count;
    uint8_t dtim_period;
    uint8_t hidden_ssid;

#ifdef NT_FN_WMM_PS_AP
    uint8_t apsd_enable; /* uAPSD enable/disable */
#endif                   // NT_FN_WMM_PS_AP
} AP_DEV_STRUCT;

/*
 * Internal functions
 */
void ap_start_bss(void *arg, nt_status_t status);

// for enabling WPA_IE on AP-side;for testing purpose only
#ifdef NT_TST_FN_WPA_IE
void ap_en_wpa_ie(void *arg, uint8_t oui_en);
#endif  // NT_TST_FN_WPA_IE

#endif /* _WLAN_AP_H_ */
