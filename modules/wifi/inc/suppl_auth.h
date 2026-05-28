/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

//#ifdef SECURITY

#ifndef __SUPPL_AUTH_H__
#define __SUPPL_AUTH_H__
#include "wifi_cmn.h"
#include "wlan_dev.h"
#include "ieee80211_defs.h"
//#include "nt_timer.h"
#include "suppl_auth_api.h"

//#define SEC_DEBUG

#define PMK_LEN   32
#define GTK_LEN   32
#define PTK_LEN   64
#define NONCE_LEN 32

#define SUPPL_AUTH_NUM_OF_LINKS AP_MAX_NUM_STA

#ifdef SEC_DEBUG
#define SUPPL_SESSION_TIMER  60000  /* 60 seconds */
#define AUTH_INTER_FRM_TIMER 10000  // 10000    /* 10 seconds */
#else
#define SUPPL_SESSION_TIMER 30000 /* 30 seconds */
//#define AUTH_INTER_FRM_TIMER    1000   //1000    /* 1 second */
#endif

typedef enum {
    SUPPL_STATE_NULL = 0xA0,
    SUPPL_STATE_INIT,
    SUPPL_STATE_SENT_M2,
    SUPPL_STATE_CONNECTED,
} SUPPL_STATE;

typedef enum {
    AUTH_STATE_INIT = 0xE0,
    AUTH_STATE_SENT_M1,
    AUTH_STATE_SENT_M3,
    AUTH_STATE_CONNECTED,
    AUTH_STATE_SENDING_M5,
} AUTH_STATE;

#define PLUMB_UKEY 0x01
#define PLUMB_MKEY 0x02

#define KEYDATA_KEYTYPE_BIT 0x0008
#define KEYDATA_WPA_KEYID   0x0030
#define KEYDATA_MIC_BIT     0x0100
#define KEYDATA_SEC_BIT     0x0200
#define KEYDATA_ERR_BIT     0x0400
#define KEYDATA_REQ_BIT     0x0800
#define KEYDATA_ENC_BIT     0x1000

#define AUTH_IS_8021X(auth) ((auth) & (WMI_WPA_AUTH | WMI_WPA2_AUTH | WMI_WPA2_SHA256_AUTH | WMI_WPA3_ENTERPRISE_ONLY_AUTH))

#define STATUS_M1_NEED_HOLD (1 << 0)

typedef struct supp_auth_rec_t {
    SUPPL_AUTH_FN side;
    NT_BOOL valid;
    uint8_t state;
    uint8_t peer[IEEE80211_ADDR_LEN];
    uint8_t keyType;
    uint16_t auth;
    uint8_t ucipher;
    uint8_t mcipher;
    uint8_t *rsn_ie;
    uint8_t pmk[PMK_LEN];
    uint8_t gtk[GTK_LEN];
#ifdef NT_FN_RMF
    uint8_t igtk[IGTK_LEN];
    uint8_t igtk_keyix;
#endif
    uint8_t gtk_keyix;
    uint8_t ptk[PTK_LEN];
    uint8_t s_nonce[NONCE_LEN];
    uint8_t a_nonce[NONCE_LEN];

    TimerHandle_t timer;
    TimerHandle_t tkip_cm_timer;

    uint64_t replay_counter;
    uint8_t num_of_retries; /* Num of retries for a Mx */
    uint8_t plumb_keys;
    void *calling_ctxt;
    uint8_t lower_layer_hdr_len; /* 802.11 3-addr frm (qos/non-qos)[24/26] vs 4-addr qos[32] + LLC */
    LL_HDR_FN link_layer_dot11_hdr_fn;

    SUPPL_AUTH_COMPL_FN compl_fn;

    uint32_t last_mic_err_ms;
    NT_BOOL mic_err_to;
    uint8_t prev_gtk[GTK_LEN]; /* In order to avoid installing same GTK after rekeying - CR 2122517 */
    uint8_t prev_gtk_keyix;
    uint8_t prev_ptk[PTK_LEN]; /* In order to avoid installing same PTK after rekeying - CR 2130539 */
    uint8_t *frm;              /* backup pointer to the received frame */
    uint16_t frm_len;          /* backup length of the received frame */
    /* 8021X M1 hold buffer */
    uint8_t *m1_hold_buf;      /* heap buffer holding a copy of M1 frame */
    uint16_t m1_frame_len;     /* length of the held M1 frame */
    uint8_t pmk_len;           /* PMK length */
    uint8_t rec_id;            /* offset of rec in array */
    uint8_t rekeying_flag;     /* Flag to specify general handshake and Group key handshake */

} SUPPL_AUTH_REC;

typedef struct supp_auth_info_t {
    void *dev;
    uint8_t hwaddr[IEEE80211_ADDR_LEN];
    SUPPL_AUTH_REC rec[SUPPL_AUTH_NUM_OF_LINKS];
    uint8_t tkip_cm_timer;       /*mic failure during 4way handshake, value read from dev config*/
    uint8_t auth_interval;       /*M1/M3 timer (authenticator side), value read from dev config*/
    uint8_t suppl_session_timer; /*4way handshake timer (supplicant side), value read from dev config*/
    uint8_t auth_retry_count;    /* M1/M3 retry count (authenticator side), value read from dev config*/
} SUPPL_AUTH_INFO;

typedef struct info_params_t {
    SUPPL_AUTH_INFO *info;
    SUPPL_AUTH_REC *rec;
    SUPPL_AUTH_EVENT evt;
    uint8_t *frm;
    uint16_t sz;
} INFO_PARAMS;

#define KEY_REPLAY_COUNTER_START  9
#define KEY_REPLAY_COUNTER_OFFSET 16
#define NONCE_OFFSET              17

#define MIC_LENGTH (16)
#define MIC_OFFSET (17 + 32 + 16 + 8 + 8)

void sec_timeout(TimerHandle_t alarm);
void sec_tkip_cm_timeout(TimerHandle_t alarm);
SUPPL_AUTH_REC *suppl_auth_alloc_node(SUPPL_AUTH_INFO *info);
SUPPL_AUTH_REC *suppl_auth_find_node(SUPPL_AUTH_INFO *info, uint8_t *sa);
void suppl_process_timeout(INFO_PARAMS *param);
void suppl_auth_free_node(SUPPL_AUTH_REC *rec);
void suppl_process_m1_evt(INFO_PARAMS *param);
void suppl_process_sent_m2(INFO_PARAMS *param);
void suppl_process_m3_evt(INFO_PARAMS *param);
void suppl_process(INFO_PARAMS *param);
void suppl_process_null_state(INFO_PARAMS *param);
void suppl_process_init_state(INFO_PARAMS *param);
void suppl_process_connected(INFO_PARAMS *param);
void suppl_auth_rand(uint8_t *addr, uint16_t len);
void suppl_auth_plumb_keys(INFO_PARAMS *param);
void suppl_auth_close_auth(INFO_PARAMS *param);
#ifdef NT_CFG_WLAN_AP
void auth_process(INFO_PARAMS *param);
void auth_process_init_state(INFO_PARAMS *param);
void auth_process_connected(INFO_PARAMS *param);
void auth_process_sent_m1(INFO_PARAMS *param);
void auth_process_sent_m3(INFO_PARAMS *param);
void auth_process_sent_m5(INFO_PARAMS *param);
void auth_process_m2_evt(INFO_PARAMS *param);
void auth_process_m4_evt(INFO_PARAMS *param);
void auth_process_m1_timeout(INFO_PARAMS *param);
void auth_process_m3_timeout(INFO_PARAMS *param);
#endif  // NT_CFG_WLAN_AP
void suppl_process_tkip_cm(INFO_PARAMS *param);

void auth_process_grp_key_handshake(devh_t *dev, uint8_t *source_addr);

#ifdef CONFIG_WLAN_8021X_LIB
void suppl_auth_update_pmk(void *ctxt, uint8_t *peer, uint8_t *pmk, uint8_t pmk_len);
void suppl_8021X_hold_m1(SUPPL_AUTH_INFO *info, SUPPL_AUTH_REC *rec);
void suppl_8021X_drain_m1(SUPPL_AUTH_INFO *info, SUPPL_AUTH_REC *rec);
void suppl_8021X_resume_m1(SUPPL_AUTH_INFO *info, SUPPL_AUTH_REC *rec);
#endif

#endif /* __SUPPL_AUTH_H__ */
//#endif /* SECURITY */
