/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

//
// This file contains the internal structures and definitions used by
// the mlme module.
//
//

#ifndef _WLAN_MLME_H_
#define _WLAN_MLME_H_
#include "wifi_cmn.h"
#include "mlme_api.h"
#include "wlan_framegen.h"
#include "wlan_dev.h"

//#define MLME_SM_RETRY_CNT			3

/*
 * Sequence number minus one is saved
 */
#define MLME_GET_CONN_AUTHSEQ(conn)      (((conn)->mgmt_flags & MLME_CONN_AUTHSEQ) + 1)
#define MLME_SET_CONN_AUTHSEQ(conn, val) ((conn)->mgmt_flags = ((conn)->mgmt_flags | (((val)-1) & MLME_CONN_AUTHSEQ)))

//#define WLAN_MLME_TO_MS   10000   /* 1.5 s MLME response Time Out *///sayandev-2sec
#define MAX_WPA_IE_LEN 224

#if ((defined NT_FN_RMF) || (defined NT_FN_WPA3))
#define ASSOC_COMEBACK_TIMEOUT 2000 /* 2sec Assoc Comeback Time out */
#endif                              // NT_FN_RMF

/* frm[1] has length of IE in bytes excluding element ID and length field */
#define NEXT_IE(frm) ((frm[1]) + 2)

typedef enum {
    MLME_SM_OK = 0,   /* SM has progressed */
    MLME_SM_ERR = 1,  /* SM has encountered an error */
    MLME_SM_HLT = 2,  /* SM should be halted */
    MLME_SM_HOLD = 3, /* SM is on hold awaiting decision
                       * (e.g. from host) & consumes the buffer
                       */
} MLME_SM_STATUS;

typedef struct umlme_struct {
    uint8_t numConn;
    uint8_t challenge_ie[IEEE80211_CHALLENGE_LEN + 2]; /* AP mode shared auth */
    uint16_t rsn_ie_len;
    uint8_t rsn_ie[MAX_WPA_IE_LEN];
#ifdef FM_PMK_CACHING
    uint16_t temp_rsn_ie_len;
    uint8_t temp_rsn_ie[MAX_WPA_IE_LEN];
#endif /*FM_PMK_CACHING*/
       /*  uint16_t                            wpa_ie_len;
         uint8_t                             wpa_ie[MAX_WPA_IE_LEN];*/
    uint8_t ieee80211_auth_alg;
    uint16_t sm_timeout_value;            /* auth/assoc timeout value, value  read from dev config*/
    uint8_t assoc_comeback_timeout_value; /* assoc comeback timeout value,  read from dev config*/
} UMLME_STRUCT;

typedef struct {
    MLME_DISCONNECT_ACTION disAction;
    uint16_t reason;
} MLME_STATEINIT_ARG;

typedef enum {
    ht_disable,
    ht_enable,
} ht_status;
/* Internal function declarations */

void mlme_set_auth_alg(devh_t *dev, DOT11_AUTH_MODE wlan_dot11_authmode);
void mlme_create_adhoc_bssid(devh_t *dev, uint8_t *ni_bssid);
uint8_t mlme_mac_cmp_wild(uint8_t *acl_mac, uint8_t *sta_mac, uint8_t wild);
void mlme_deauth_blocked_sta(devh_t *dev);
uint8_t mlme_acl_mac_blocked(devh_t *dev, uint8_t *mac);
nt_status_t mlme_newstate_init(devh_t *dev, conn_t *conn, IEEE80211_CONN_STATE ostate, uint8_t *arg, uint32_t argLen);
nt_status_t mlme_newstate_auth(devh_t *dev, conn_t *conn, IEEE80211_CONN_STATE ostate, uint8_t *arg, uint32_t argLen);
nt_status_t mlme_newstate_assoc(devh_t *dev, conn_t *conn, IEEE80211_CONN_STATE ostate, uint8_t *arg, uint32_t argLen);
#ifdef NT_FN_RMF
nt_status_t mlme_newstate_run(devh_t *dev, conn_t *conn, IEEE80211_CONN_STATE ostate);
#else
nt_status_t mlme_newstate_run(devh_t *dev, conn_t *conn, __attribute__((__unused__)) IEEE80211_CONN_STATE ostate);
#endif  // NT_FN_RMF
nt_status_t mlme_newstate(devh_t *dev, conn_t *conn, IEEE80211_CONN_STATE ostate, uint8_t *arg, uint32_t argLen);
void mlme_sm_halt(devh_t *dev, conn_t *conn, WMI_DISCONNECT_REASON reason, uint16_t protoReasonStatus,
                  MLME_DISCONNECT_ACTION mlmeOp);
uint8_t mlme_validate_sta_rates(devh_t *dev, conn_t *conn, uint8_t *rates, uint8_t *xrates);
uint8_t mlme_reached_max_conn(devh_t *dev);
MLME_SM_STATUS mlme_recv_auth_open(devh_t *dev, conn_t *conn, struct ieee80211_frame *wh, uint16_t seq,
                                   uint16_t exp_seq, uint16_t alg, uint16_t status);
MLME_SM_STATUS mlme_recv_auth_shared(devh_t *dev, conn_t *conn, struct ieee80211_frame *wh, uint8_t *frm, uint8_t *efrm,
                                     uint16_t seq, uint16_t exp_seq, uint16_t alg, uint16_t status);
MLME_SM_STATUS mlme_recv_auth(devh_t *dev, conn_t *conn, uint8_t *bufPtr, uint32_t bufLen, uint16_t *protoStatusCode);

MLME_SM_STATUS mlme_recv_assoc_req(devh_t *dev, conn_t *conn, uint8_t *bufPtr, uint16_t bufLen, uint16_t subtype,
                                   uint16_t *protoStatusCode);

MLME_SM_STATUS mlme_recv_assoc_resp(devh_t *dev, conn_t *conn, uint8_t *bufPtr, uint16_t bufLen, NT_BOOL isReAssoc,
                                    uint16_t *protoStatusCode);
MLME_SM_STATUS mlme_recv_deauth(devh_t *dev, conn_t *conn, uint8_t *bufPtr, uint16_t bufLen, uint16_t *protoReasonCode);
MLME_SM_STATUS mlme_recv_disassoc(devh_t *dev, conn_t *conn, uint8_t *bufPtr, uint16_t bufLen,
                                  uint16_t *protoReasonCode);
MLME_SM_STATUS mlme_recv_tspec(devh_t *dev, conn_t *conn, uint8_t *frm, uint8_t *efrm);
#ifdef CONFIG_WLAN_80211H
MLME_SM_STATUS mlme_recv_specmgmt(devh_t *dev, uint8_t *frm, uint8_t *bufPtr, uint16_t bufLen, int32_t rssi);
#endif /* CONFIG_WLAN_80211H */

MLME_SM_STATUS mlme_recv_ba(devh_t *dev, uint8_t *frm, conn_t *conn);
void mlme_update_htrateset(conn_t *conn, struct ieee80211_mcsrateset *rs, uint8_t *ht_cap);

#ifdef WLAN_CONFIG_RRM
MLME_SM_STATUS mlme_recv_rm(devh_t *dev, uint8_t *frm, uint8_t *efrm);
#endif /* WLAN_CONFIG_RRM */
#ifdef NT_FN_FTM
MLME_SM_STATUS mlme_recv_action_frame(devh_t *dev, conn_t *conn, uint8_t *bufPtr, uint16_t bufLen);
#endif  //  NT_FN_FTM
nt_status_t umlme_recv_mgmt(devh_t *dev, conn_t *conn, uint8_t *bufPtr, uint32_t bufLen, int32_t subtype);
nt_status_t mlme_create_send_mgmt_conn_addr(devh_t *dev, conn_t *conn, uint8_t *pAddr1, int32_t subtype,
                                            mgmt_frm_arg_t *mgmt_frm_arg);
nt_status_t wlan_mlme_update_ht_rate(uint8 rate_max, uint8 rate_mask);
void mlme_parse_ht_cap(devh_t *dev, conn_t *conn, struct ieee80211_ie_htcap_cmn *ht_ie);

conn_t *mlme_find_conn_with_aid(devh_t *dev, uint8_t aid);
#ifdef NT_FN_FTM

#if 0
MLME_SM_STATUS mlme_send_FTM_frame(devh_t *dev,conn_t *conn, FTM_IE_T *ftmIE, uint8_t ftmType, uint8_t trigger);
#else
#ifdef NT_FN_FTM_2016V
MLME_SM_STATUS mlme_send_FTM_req_frame(devh_t *dev, conn_t *conn, FTM_IE_T *ftmIE, uint8_t ftmType, uint8_t trigger);
MLME_SM_STATUS mlme_send_FTM_resp_frame(devh_t *dev, conn_t *conn, IEEE80211_2016_FTM_ACTION_FRAME *ftm_resp_frm,
                                        FTM_IE_T *ftmIE, FTM_SYNC_IE *ftmSyncIE);
MLME_SM_STATUS mlme_send_FTM_Fup_frame(devh_t *dev, conn_t *conn, FTM_IE_T *ftmIE, uint64_t toa, uint64_t tod,
                                       uint8_t ftmType);
#endif  // NT_FN_FTM_2016V
#ifdef NT_FN_FTM_11V
MLME_SM_STATUS mlme_send_FTM_req_frame(devh_t *dev, conn_t *conn, IEEE80211_2016_FTM_REQUEST_FRAME ftp_req_frm);
MLME_SM_STATUS mlme_send_FTM_resp_frame(devh_t *dev, conn_t *conn, IEEE80211_2016_FTM_ACTION_FRAME *ftm_resp_frm);
#endif  // NT_FN_FTM_11V
#endif
void send_rtt_ftm_notification(FTM_NOTIFICATION *ftm_notif);
#endif  // NT_FN_FTM

#ifdef NT_TST_FN_RMF
void rmf_test_send_mgmt_frame(devh_t *dev, int16_t frmType, int8_t aid);
conn_t *mlme_find_conn_with_aid(devh_t *dev, uint8_t aid);
#endif  // NT_RMF_TEST

void target_reset_device(void **dev);

conn_t *mlme_umlme_create_conn(devh_t *dev, uint8_t *ta_addr);
void conn_assoc_complete(devh_t *dev, conn_t *conn, int *ret, uint8_t *buff, uint32_t buffLen);
#ifdef SUPPORT_11W
MLME_SM_STATUS mlme_recv_sq(devh_t *dev, conn_t *conn, uint8_t *bufPtr, uint16_t bufLen);
#endif
void umlme_conn_cb(void *arg, int status);
void mlme_get_ie(devh_t *dev, MLME_IE_ID id, MLME_IE_INFO *pIe);
void wlan_set_ht_cap_cmd(devh_t *dev, WMI_SET_HT_CAP_CMD *buffer);

/**
 * brief  : QOS NULL trigger timer call back function for BE category.
 * params : @timer_param device structure of devh_t type
 * return : none
 */
void nt_mlme_qos_null_be_trigger_timer_cb_fn(TimerHandle_t timer_param);

/**
 * brief  : QOS NULL trigger timer call back function for BK category.
 * params : @timer_param device structure of devh_t type
 * return : none
 */
void nt_mlme_qos_null_bk_trigger_timer_cb_fn(TimerHandle_t timer_param);

/**
 * brief  : QOS NULL trigger timer call back function for VI category.
 * params : @timer_param device structure of devh_t type
 * return : none
 */
void nt_mlme_qos_null_vi_trigger_timer_cb_fn(TimerHandle_t timer_param);

/**
 * brief  : QOS NULL trigger timer call back function for VO category.
 * params : @timer_param device structure of devh_t type
 * return : none
 */
void nt_mlme_qos_null_vo_trigger_timer_cb_fn(TimerHandle_t timer_param);

/**
 * brief  : eosp expiry call back function for BE category.
 * params : none
 * return : none
 */
void nt_mlme_be_eosp_expiry_timer_cb_fn(TimerHandle_t timer_param);

/**
 * brief  : eosp expiry call back function for BK category.
 * params : none
 * return : none
 */
void nt_mlme_bk_eosp_expiry_timer_cb_fn(TimerHandle_t timer_param);

/**
 * brief  : eosp expiry call back function for VI category.
 * params : none
 * return : none
 */
void nt_mlme_vi_eosp_expiry_timer_cb_fn(TimerHandle_t timer_param);

/**
 * brief  : eosp expiry call back function for VO category.
 * params : none
 * return : none
 */
void nt_mlme_vo_eosp_expiry_timer_cb_fn(TimerHandle_t timer_param);

/**
 * brief  : common function to initialize qos trigger timer for each individual uapsd enabled AC.
 * params : @ dev : device structure of type devh_t.
 * 			@ ac  : AC for which need to initialize timer.
 * 		    @ ms  : qos trigger timer expiry time in milli seconds.
 * 		    @ fn  : function pointer for call back function that gets called on timer expiry.
 * return : TRUE on successful initialization of timer for specified uapsd category, FALSE on failure to initialize
 * timer.
 */
NT_BOOL mlme_common_initialize_qos_null_trigger_timer(devh_t *dev, uint8_t ac, uint16_t ms, void(*fn));

/**
 * brief  : common function to initialize eosp expiry interrupt timer for each individual uapsd enabled AC.
 * params : @ dev : device structure of type devh_t.
 * 			@ ac  : AC for which need to initialize timer.
 * 		    @ ms  : eosp interrupt timer expiry time in milli seconds.
 * 		    @ fn  : function pointer for call back function that gets called on timer expiry.
 * return : TRUE on successful initialization of timer for specified uapsd category, FALSE on failure to initialize
 * timer.
 */
NT_BOOL mlme_common_initialize_eosp_data_wait_timer(devh_t *dev, uint8_t ac, uint16_t ms, void(*fn));

/**
 * brief  : initializes qos trigger timers for all uapsd enabled ACs.
 * params : @ dev : device structure of type devh_t.
 * 		    @ ms  : qos trigger timer expiry time in milli seconds.
 * return : TRUE on successful initialization of timers of uapsd categories, FALSE on failure to initialize timers
 */
NT_BOOL mlme_initialize_qos_null_trigger_timers(devh_t *dev, uint16_t ms);
/**
 * brief  : initializes eosp interrupt wait timers for all uapsd enabled ACs.
 * params : @ dev : device structure of type devh_t.
 * 		    @ ms  : eosp interrupt expiry time in milli seconds.
 * return : TRUE on successful initialization of timers of uapsd categories, FALSE on failure to initialize timers.
 */
NT_BOOL mlme_initialize_eosp_data_wait_timer(devh_t *dev, uint16_t ms);

#ifdef SUPPORT_SAP_POWERSAVE
/**
 * brief  : Wrapper function to send probe resp with next beacon tbtt to test the SAP ps case
 * params : @ dev       : device structure of type devh_t.
 * 		    @ dstAddr   : Mac address of the destination node
            @ bssid     : Bssid of the connected AP
            @ ssidInfo  : ssid of the current bss
 * return : This function does not return anything
 */
void mlme_send_probe_req_for_sap_ps(devh_t *dev, uint8_t *dstAddr, uint8_t *bssid, ssid_t *ssidInfo);
#endif

#ifdef FM_PMK_CACHING
void mlme_reset_pmkcach_parameters(devh_t *dev);
#endif  // FM_PMK_CACHING

#endif /* _WLAN_MLME_H_ */
