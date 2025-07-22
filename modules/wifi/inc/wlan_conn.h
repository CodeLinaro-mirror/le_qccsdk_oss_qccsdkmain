/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

//
// This file contains the api for the Wireless Module Interface (WMI) on the
// target.
//
// $Id:
//
//

#ifndef _WLAN_CONN_H_
#define _WLAN_CONN_H_

#include <stdint.h>
#include "wifi_cmn.h"
#include "ieee80211.h"
#include "wlan_defs.h"
#include "wlan_bss.h"
#include "nt_timer.h"
#include "hal_api_sys.h"
#include "tx_aggr_api.h"
#include "hal_int_rates.h"
#include "wlan_ra_api.h"
#ifdef NT_FN_WPA3
#include "wpa3_sae.h"
#endif  // NT_FN_WPA3

/* ni flags */
#define IEEE80211_NODE_QOS             0x000001 /* QoS enabled */
#define IEEE80211_NODE_GK_INST         0x000002 /* group Key Installed */
#define IEEE80211_NODE_PRIV            0x000004 /* Encryption Enabled */
#define IEEE80211_NODE_PK_INST         0x000008 /* Pairwise Key Installed */
#define IEEE80211_NODE_TKIP_CM_ENABLED 0x000010 /* TKIP CounterMeasures */
#define IEEE80211_NODE_APSD            0x000020 /* APSD supported */
#define IEEE80211_NODE_HT              0x000040 /* HT IEs arrived in ASSOC_RESP */
#define IEEE80211_NODE_HT_ALLOWED      0x000080 /* HT mode can be disallowed by firmware */
#define IEEE80211_NODE_STBC            0x000100 /* STBC */
#define IEEE80211_NODE_LDPC            0x000200 /* LDPC */
#define IEEE80211_NODE_WPS             0x000400 /* Enable WPS */
#define IEEE80211_NODE_MFP             0x000800 /* Enable MFP based on BSS RSN IE */

/* txrx data frame policy */
#define TXRX_DROP_ALL_DATA_FRAMES 0
#define TXRX_ALLOW_EAPOL_FRAMES   1
#define TXRX_ALLOW_ALL_FRAMES     2

typedef enum ieee80211_conn_state {
    IEEE80211_CONN_S_INIT = 0,  /* default state */
    IEEE80211_CONN_S_AUTH = 1,  /* try to authenticate */
    IEEE80211_CONN_S_ASSOC = 2, /* try to assoc */
    IEEE80211_CONN_S_RUN = 3,   /* associated */
#if ((defined NT_FN_RMF) || (defined NT_FN_WPA3))
    IEEE80211_CONN_S_SA = 4, /* SA query state */
#endif                       // NT_FN_RMF
} IEEE80211_CONN_STATE;

typedef enum {
    APSD_SP_END = 0,
    APSD_SP_ACTIVE = 1,
} APSD_STATE;

#define START_APSD_SP(conn)      ((conn)->apsd_state = APSD_SP_ACTIVE)
#define END_APSD_SP(conn)        ((conn)->apsd_state = APSD_SP_END)
#define IS_APSD_SP_STARTED(conn) ((conn)->apsd_state == APSD_SP_ACTIVE)

/*
 * Connection Structure
 */

typedef struct conn_s {
    uint8_t ni_macaddr[IEEE80211_ADDR_LEN]; /* peer mac address */
    enum ieee80211_conn_state conn_state;
    uint8_t connid;
    TimerHandle_t mlme_timer;
    void *pDev;              /* pointer to dev */
    struct bss_s *pBss;      /* pointer to BSS */
    nt_hal_sta_t halStaInfo; /* HAL STA Structure */

    uint16_t mgmt_flags; /* flags used by mgmt. See definition below */
    uint8_t cipher;
    CRYPTO_TYPE ucipher;
    CRYPTO_TYPE mcipher;
    AUTH_MODE auth_mode;
    uint8_t keymgmt;
    uint16_t lint; /* AP Mode: listen Intvl for the STA */

    uint16_t ni_associd; /* assoc response */
    uint8_t ni_prev_ap_addr[IEEE80211_ADDR_LEN];
    uint8_t station_id;                /* station identifier used by dpm */
    struct ieee80211_rateset ni_rates; /* negotiated rate set */
#ifdef NT_FN_RA
    nt_ra_crl_t ni_crl;               /* candidate rate index list - derived from negotiated rate set */
    nt_ra_crl_t ni_crl_backup;        /* candidate rate index list - derived from negotiated rate set */
    nt_ra_crl_vector_t ra_crl_vector; /* pri, sec, ter crl index */
    nt_hal_sta_rate_stat_t ra_stats;  /* rate adaptation related tx stats */
    uint8_t ra_uprate_sampling_cnt;   /* rate adaptation uprate sampling cnt */
#endif                                // NT_FN_RA
    uint32_t ni_flags;

    union {
        ssid_t ssid_ie;
        uint8_t challenge_ie[IEEE80211_CHALLENGE_LEN + 2]; /* AP mode shared auth */
    } shared_ie;

    WLAN_PHY_MODE phymode;
    uint16_t capinfo;
#ifdef NT_FN_HT
    struct ieee80211_mcsrateset ni_ratesHT; /* negotiated HT rate set */

    uint8_t mpdudensity;
#endif  // NT_FN_HT
    TX_AGGR tx_aggr;
    RX_AGGR rx_aggr;
#ifdef NT_FN_HT
    uint8_t ba_status[8];
#endif  // NT_FN_HT
    struct aggtx_tid aggtx_tid;
    struct aggrx_tid aggrx_tid;

    TimerHandle_t assoc_comeback_timer;

    uint16_t sa_query_transid;

    uint8_t apsd_info;
    uint32_t last_mic_err_us; /* used for wpa coutermeasure */
    uint32_t last_tx_rx_time; /* last data activity */

#ifdef NT_FN_WPA3
    struct sae_data sae_data;
#endif  // NT_FN_WPA3
    uint8_t dialog_id;
    bool xpan_sap;                         /*STA connected to XPAN SAP*/
    uint16_t ignore_assoc_resp_rates : 1,  // ignore_assoc_resp_rates
        reserved : 15;
#ifdef SUPPORT_COEX
    uint32_t backup_data_rate;
#endif
} conn_t;

#define ni_challenge_ie shared_ie.challenge_ie
#define ni_ssid_ie      shared_ie.ssid_ie

#define MAX_TIDS 8

/* MGMT_FLAGS used by MLME */
#define MLME_CONN_AUTHSEQ     0x0003 /* Auth seq number */
#define MLME_CONN_AUTH_SHARED 0x0004 /* If 802.11 auth is not LEAP, this bit indicates shared or open auth */
#define MLME_CONN_RECONNECT   0x0008 /* Do reassoc instead of assoc */
#define MLME_CONN_INITIATOR   0x0010 /* Connection request/accept   */
#define MLME_CONN_PREAUTH     0x0020 /* Conn has pre-auth enabled (Not used) */
#define MLME_CONN_INIT_DONE   0x0040 /* MLME connection object is initialized */
#define MLME_CONN_BSS         0x0080 /* BSS connection object */
#define MLME_CONN_NO_DISASSOC 0x0100 /* No diassoc at disconnect */
#define MLME_CONN_PMK_CACHING 0x0200 /* bit indicates PMK caching */
#define MLME_CONN_AUTH_SAE    0x0400 /* bit indicates sae ot open auth */

#endif /* _WLAN_CONN_H_ */
