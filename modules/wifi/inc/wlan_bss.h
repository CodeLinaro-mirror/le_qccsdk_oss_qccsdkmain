/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _WLAN_BSS_H_
#define _WLAN_BSS_H_

//#error wlan_bss called
#include <stdint.h>
#include "wifi_cmn.h"

#include "fwconfig_wlan.h"
#include "nt_flags.h"
#include "wlan_dev.h"
#include "ieee80211_defs.h"
#include "txrx_api.h"
#include "roaming_api.h"

#define IEEE80211_EXPONENT_TO_VALUE(_exp) (1 << (uint32_t)(_exp)) - 1
#define IEEE80211_TXOP_TO_US(_txop)       (uint32_t)(_txop) << 5
#define IEEE80211_US_TO_TXOP(_us)         (uint16_t)((uint32_t)(_us)) >> 5

typedef void (*BSS_JOIN_COMPLETION_CB)(void *dev, void *bss, int status);

#ifdef SUPPORT_COEX
struct coex_rx_rate_monitor_s {
    uint32_t last_rxrate;
    uint32_t raw_last_rxrate;
    uint32_t raw_last_rxrate_timestamp;
    uint32_t min_rx_rate;
    uint32_t max_rx_rate;
    uint32_t frame_length_threshold;
    uint8_t last_event;
    uint8_t num_mpdus_below_min_rx_rate;
    uint8_t num_mpdus_above_max_rx_rate;
    uint8_t trigger_lower_thresh;
    uint8_t trigger_upper_thresh;
};
#endif  // #ifdef SUPPORT_COEX

typedef struct bss_s {
    uint8_t ni_macaddr[IEEE80211_ADDR_LEN];
    uint8_t ni_bssid[IEEE80211_ADDR_LEN];
    ssid_t ni_ssid;
    int8_t ni_bss_idx;
    struct devh_s *pdev;
    struct channel *ni_chan;
    uint16_t ni_capinfo;               /* capabilities */
    uint8_t ni_erp;                    /* ERP from beacon/probe resp */
    struct ieee80211_rateset ni_rates; /*rate set */

    union {
        uint8_t data[8];
        uint64_t tsf;
    } ni_tstamp;        /* from last rcv'd beacon */
    uint16_t ni_intval; /* beacon interval */
    uint16_t ni_txpower;
    uint16_t ni_dtimperiod;
    uint16_t ni_dtimcount;

    uint8_t ni_wpaie_flag;
    uint8_t ni_preauth_capable;
#ifdef NT_FN_HT
    uint8_t ni_ht_capable;
#endif                       // NT_FN_HT
    CRYPTO_TYPE ni_ucipher;  // to save the info. received from Probe resp
    CRYPTO_TYPE ni_mcipher;
    AUTH_MODE ni_authmode;
#ifdef NT_FN_WMM
    uint8_t ni_wmm_capable;   /* contains WMM ie */
    uint8_t ni_uapsd_capable; /* WMM ie has U-APSD bit set */
#endif                        // NT_FN_WMM
    uint16_t ni_flags;
    struct chanAccParams ni_bssChanParams;

#ifdef NT_FN_ROAMING
    ROAM_METRICS ni_roam_metrics;
#endif  // NT_FN_ROAMING

#ifdef NT_FN_HT
    struct ieee80211_ie_htcap_cmn ni_ht_cap_cmn;
    struct ieee80211_ie_htinfo_cmn ni_ht_info_cmn;
#endif  // NT_FN_HT
#if (defined NT_FN_WUR_AP || defined NT_FN_WUR_STA)
    uint8_t ni_wur_capable; /* wur ie */
#endif                      /* (defined NT_FN_WUR_AP || defined NT_FN_WUR_STA) */

#ifdef NT_FN_WNM_POWERSAVE_MODE
    uint8_t ni_wnm_capable; /* wnm ie */
#endif                      /* NT_FN_WNM_POWERSAVE_MODE */
#ifdef NT_FN_TWT
    uint8_t ni_twt_capable; /* twt ie */
#endif                      /* NT_FN_TWT */
#ifdef SUPPORT_COEX
    struct coex_rx_rate_monitor_s ni_crrm;
#endif  // #ifdef SUPPORT_COEX
    uint32_t ni_last_rcv_bcn_tstamp;
    uint64_t ni_next_dtim_bcn_tsf;  // uses AP Tsf to calculcate next bcn tsf
    uint64_t ni_next_bcn_tsf;       // uses AP Tsf to calculcate next bcn tsf
#ifdef NT_FN_WPA3
    uint8_t ni_rsnxe_ie[3]; /* rsnx ie */
#endif
} bss_t;

#define NT_BSS_IDX_FREE -1

/*
 * ni_flags definitions
 */
#define BSS_VALID         0x0001
#define BSS_CONNECTED     0x0002 /* we have previously connected with this bss */
#define BSS_11H_ENABLED   0x0004
#define BSS_ERP_ENABLED   0X0008 /* ERP Protection enabled */
#define BSS_HTP_ENABLED   0X0010 /* HT Protection enabled */
#define BSS_WUR_ENABLED   0x0020 /* wur enabled */
#define BSS_UAPSD_ENABLED 0x0040 /* U-APSD enabled */
#define BSS_WPA_IE_CAP    0x0080 /* WPA OUI IE for backward compatibility */
#define BSS_WNM_ENABLED   0x0100 /* wnm enabled */
#define BSS_FTM_ENABLED   0x0200 /* ftm enabled */
#define BSS_TWT_ENABLED   0x0400 /* twt enabled */

#ifdef SUPPORT_EVENT_HANDLERS
#define BSS_CRIT_PROTO_HINT_ENABLED 0x0800 /* critical high-level protocol */
#endif                                     /* SUPPORT_EVENT_HANDLERS */

#define BSS_IS_VALID(bss)        (bss->ni_flags & BSS_VALID)
#define BSS_SET_VALID(bss)       (bss->ni_flags = BSS_VALID)
#define BSS_SET_INVALID(bss)     (bss->ni_flags = 0)
#define BSS_IS_CONNECTED(bss)    (bss->ni_flags & BSS_CONNECTED)
#define BSS_SET_CONNECTED(bss)   (bss->ni_flags |= BSS_CONNECTED)
#define BSS_CLR_CONNECTED(bss)   (bss->ni_flags &= ~BSS_CONNECTED)
#define BSS_IS_11H_ENABLED(bss)  (bss->ni_flags & BSS_11H_ENABLED)
#define BSS_SET_11H_ENABLED(bss) (bss->ni_flags |= BSS_11H_ENABLED)
#define BSS_CLR_11H_ENABLED(bss) (bss->ni_flags &= ~BSS_11H_ENABLED)
#define BSS_IS_ERP_ENABLED(bss)  (bss->ni_flags & BSS_ERP_ENABLED)
#define BSS_SET_ERP_ENABLED(bss) (bss->ni_flags |= BSS_ERP_ENABLED)
#define BSS_CLR_ERP_ENABLED(bss) (bss->ni_flags &= ~BSS_ERP_ENABLED)
#define BSS_IS_HTP_ENABLED(bss)  (bss->ni_flags & BSS_HTP_ENABLED)
#define BSS_SET_HTP_ENABLED(bss) (bss->ni_flags |= BSS_HTP_ENABLED)
#define BSS_CLR_HTP_ENABLED(bss) (bss->ni_flags &= ~BSS_HTP_ENABLED)
#define BSS_IS_WUR_ENABLED(bss)  (bss->ni_flags & BSS_WUR_ENABLED)
#define BSS_SET_WUR_ENABLED(bss) (bss->ni_flags |= BSS_WUR_ENABLED)
#define BSS_CLR_WUR_ENABLED(bss) (bss->ni_flags &= ~BSS_WUR_ENABLED)
#define BSS_IS_WNM_ENABLED(bss)  (bss->ni_flags & BSS_WNM_ENABLED)
#define BSS_SET_WNM_ENABLED(bss) (bss->ni_flags |= BSS_WNM_ENABLED)
#define BSS_CLR_WNM_ENABLED(bss) (bss->ni_flags &= ~BSS_WNM_ENABLED)
#define BSS_IS_FTM_ENABLED(bss)  (bss->ni_flags & BSS_FTM_ENABLED)
#define BSS_SET_FTM_ENABLED(bss) (bss->ni_flags |= BSS_FTM_ENABLED)
#define BSS_CLR_FTM_ENABLED(bss) (bss->ni_flags &= ~BSS_FTM_ENABLED)
#define BSS_IS_TWT_ENABLED(bss)  (bss->ni_flags & BSS_TWT_ENABLED)
#define BSS_SET_TWT_ENABLED(bss) (bss->ni_flags |= BSS_TWT_ENABLED)
#define BSS_CLR_TWT_ENABLED(bss) (bss->ni_flags &= ~BSS_TWT_ENABLED)

#ifdef SUPPORT_EVENT_HANDLERS
#define BSS_SET_CRIT_PROTO_ENABLED(bss) (bss->ni_flags |= BSS_CRIT_PROTO_HINT_ENABLED)
#define BSS_CLR_CRIT_PROTO_ENABLED(bss) (bss->ni_flags &= ~BSS_CRIT_PROTO_HINT_ENABLED)
#endif /* SUPPORT_EVENT_HANDLERS */

#define BSS_AGE_CNT_RESET 5

void wlan_bss_module_init(struct devh_s *dev);
void wlan_bss_module_deinit(struct devh_s *dev);
bss_t *wlan_alloc_bss(struct devh_s *dev);
void wlan_free_bss(bss_t *bss);
bss_t *wlan_bss_lookup(struct devh_s *dev, const uint8_t *macaddr);

#ifdef NT_FN_PROTECTION
void wlan_print_prot_setting(struct devh_s *dev);
#endif  // NT_FN_PROTECTION

#endif /* _WLAN_BSS_H_ */
