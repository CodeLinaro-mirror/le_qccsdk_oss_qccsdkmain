/*
Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
*/

//
// This file contains the api for the WLAN managment module
//
// $Id:
//
//

#ifndef _MLME_API_H_
#define _MLME_API_H_

#include "wlan_dev.h"
#include "wlan_bss.h"
#include "wlan_conn.h"
#include "ieee80211_var.h"
#include "wlan_framegen.h"
#include "wifi_cmn.h"

#include "fwconfig_wlan.h"
#include "nt_flags.h"

#ifdef NT_FN_FTM
#include "nt_ftm.h"
#endif  // NT_FN_FTM

#define NT_MGMT_FRM_DUMP
/*
 * Disconnect Event
 */
typedef enum {
    MLME_NO_NETWORK_AVAIL = 0x01,
    MLME_LOST_LINK = 0x02, /* bmiss */
    MLME_DISCONNECT_CMD = 0x03,
    MLME_BSS_DISCONNECTED = 0x04,
    MLME_AUTH_FAILED = 0x05,
    MLME_ASSOC_FAILED = 0x06,
    MLME_NO_RESOURCES_AVAIL = 0x07,
    MLME_CSERV_DISCONNECT = 0x08,
    MLME_INVALID_PROFILE = 0x0a,
    MLME_DOT11H_CHANNEL_SWITCH = 0x0b,
    MLME_PROFILE_MISMATCH = 0x0c,
    MLME_CONNECTION_EVICTED = 0x0d,
    MLME_EXCESS_TX_RETRY = 0x0f,   /* TX frames failed after excessive retries */
    MLME_SEC_HS_TO_RECV_M1 = 0x10, /* Security 4-way handshake timed out waiting for M1 */
    MLME_SEC_HS_TO_RECV_M3 = 0x11, /* Security 4-way handshake timed out waiting for M3 */
    MLME_TKIP_COUNTERMEASURES = 0x12,
    MLME_CCKM_ROAMING_INDICATION = 0xfe, /* hack for CCKM fast roaming */
} MLME_DISCONNECT_REASON;

/*
 * Is short preamble Enabled
 */
#define IS_SHPRE_EN(dev, conn, bss)                                               \
    ((conn) && (bss) && ((bss)->ni_capinfo & IEEE80211_CAPINFO_SHORT_PREAMBLE) && \
     (!((bss)->ni_erp & IEEE80211_ERP_LONG_PREAMBLE)))

/*
 * Is Protection enabled
 */
#define IS_PROT_EN(bss) ((bss)->ni_erp & IEEE80211_ERP_USE_PROTECTION)

typedef struct {
    uint8_t ieLen;
    uint8_t *ieInfo;
} MLME_IE_INFO;

typedef enum {
    MLME_IE_RSN = 1,
    MLME_IE_HT_CAP = 2,
    MLME_IE_HT_INFO = 3,
    //	MLME_IE_WPA             = 4,
} MLME_IE_ID;

typedef enum {
    nt_pm_bit_reset = 0,
    nt_pm_bit_set,
} nt_pm_bit_state;

typedef enum mlme_option {
    MLME_DEFAULT_ACTION = 0,      /* No modification to the default action */
    MLME_NO_FRAME = 1,            /* Do not Send a Mgmt Frame */
    MLME_SEND_DEAUTH_FRAME = 2,   /* Send DEAUTH frame */
    MLME_SEND_DISASSOC_FRAME = 3, /* Send Disassoc frame */
} MLME_DISCONNECT_ACTION;

typedef enum dtim_option { DTIM_DISABLE = 0, DTIM_ENABLE, DTIM_AUTO } DTIM_SLEEP_POLICY;

typedef enum {
    DISCONNECTED_STATE = 0,
    CONNECTED_STATE,
    CONNECTING_STATE,
} connection_state;

#ifdef FEATURE_TX_COMPLETE
typedef struct s_wmi_callback_msg {
    pfn_callback src_callback;
    e_tx_compl_status_dp_msg tx_status;
    void *params;
} t_wmi_callback_msg;
#endif

// Submodule defines for error codes
// Ranges from 0x1000 to 0xF000
#define NT_STATUS_MLM_CM     (0x1000)
#define NT_STATUS_MLM_FRMGEN (0x2000)
#define NT_STATUS_MLM_BEACON (0x3000)
#define NT_STATUS_MLM_PS     (0x4000)
#define NT_STATUS_MLM_SEC    (0x5000)
#define NT_STATUS_MLM_DSC    (0x6000)
#define NT_STATUS_MLM_RA     (0x7000)

// WiFi driver
#define AON_CNTL_SLEEP_REG_WUR_AND_WIFI_TURN_OFF \
    0x31  // CFG_AON register configurations for wur and wifi sleep state registers
#define CFG_WUR_SLEEP_STATE_RESOURCE_REQ_REG 0x1E  // register configurations for wur and wifi sleep state registers
#define WUR_NEXT_SLEEP_STATE                 0x4   // Setting the WUR next state to WUR sleep in WUR_SS_STATE register
#define WIFI_NEXT_SLEEP_STATE                0x1  // Setting the WIFI next state to WIFI sleep in WIFI_SS_STATE register

void umlme_cmn_init(dev_common_t *pDevCmn);
void umlme_cmn_deinit(dev_common_t *pDevCmn);
void *umlme_init(devh_t *dev);
void mlme_assoc_comeback_timeout_handler(TimerHandle_t timer_handle);
void mlme_mgmt_rsp_timeout_handler(TimerHandle_t timer_handle);
void mlme_deinit(devh_t *devh);
/*@brief: mlme_sta_cipher_check(), performs decision related to auth-modes, pairwise & group ciphers for the STA
 * depending upon the bss structure it maintains from received Probe Response
 * @param: global bss structure, conn. profile, conn. structure(local to the STA)
 * @returns: nothing
 */
void mlme_sta_cipher_check(bss_t *selbs, conn_profile_t *conn_profile, conn_t *conn);
nt_status_t mlme_connect_bss(devh_t *devh, bss_t *bssid, conn_profile_t *conn_profile);
nt_status_t mlme_start_bss(devh_t *dev, bss_t *selbs, conn_profile_t *conn_profile);
nt_status_t mlme_sta_init(devh_t *dev, conn_t *conn);
void mlme_disconnect_bss(devh_t *dev, WMI_DISCONNECT_REASON reason, NT_BOOL cleanKeys,
                         MLME_DISCONNECT_ACTION disAction);
void fill_rand_bytes(uint8_t *buff, uint8_t num);
nt_status_t mlme_send_ADDBA_resp(devh_t *dev, conn_t *conn, uint8_t dialogtoken, uint16_t statuscode, uint8_t tid,
                                 uint16_t buffersize, uint16_t timeout, uint8_t bapolicy, uint8_t amsdusupported);
nt_status_t mlme_send_ADDBA_req(devh_t *dev, conn_t *conn, uint8_t dialogtoken, uint8_t tid, uint16_t ssn,
                                uint16_t buffersize, uint16_t timeout, uint8_t bapolicy, uint8_t amsdusupported);
nt_status_t mlme_send_delba(devh_t *dev, conn_t *conn, uint8_t tid, uint8_t initiator, uint16_t reason_code);

void mlme_update_rateset(devh_t *devh, conn_t *conn, uint8_t *rates, uint8_t *xrates);
void mlme_update_bss_rateset(devh_t *devh, struct ieee80211_rateset *rs, uint8_t *rates, uint8_t *xrates);
nt_status_t mlme_create_send_mgmt(devh_t *devh, conn_t *conn, int32_t subtype, mgmt_frm_arg_t *mgmt_frm_arg);
nt_status_t mlme_create_send_mgmt_by_addr(devh_t *devh, uint8_t *pAddr1, int32_t subtype, mgmt_frm_arg_t *mgmt_frm_arg);
void wlan_set_wsc_status_cmd(devh_t *dev, NT_BOOL status);
void mlme_get_ie(devh_t *dev, MLME_IE_ID id, MLME_IE_INFO *pIe);
conn_t *mlme_create_conn(devh_t *dev, uint8_t *ta_addr);
void mlme_delete_conn(devh_t *dev, conn_t *conn, WMI_DISCONNECT_REASON reason, uint16_t protoReasonStatus,
                      MLME_DISCONNECT_ACTION disAction, NT_BOOL cleanKeys);
#ifndef FEATURE_TX_COMPLETE
uint32_t mlme_send_mgmt(devh_t *dev, uint8_t *bufPtr, uint16_t bufLen);
#else
uint32_t mlme_send_mgmt(devh_t *dev, uint8_t *bufPtr, uint16_t bufLen, pfn_callback src_callback, void *params);
nt_status_t mlme_send_data_frame(uint8_t *bufPtr, uint16_t bufLen, pfn_callback src_callback, void *params);
#endif

nt_status_t mlme_send_null_frame(devh_t *dev, uint8_t *dest_addr, uint8_t *src_addr, uint8_t ps);

/* lmlme APIs */
devh_t *mlme_find_dev(dev_common_t *pDevCmn, uint8_t *dev_id, uint8_t *bufPtr);
conn_t *mlme_find_conn(devh_t *dev, uint8_t *node_addr);
conn_t *mlme_alloc_new_conn(devh_t *dev, uint8_t *node_addr);
void mlme_connection_init(devh_t *dev, conn_t *conn, bss_t *bssid);
void mlme_connection_complete(devh_t *dev, conn_t *conn);
void mlme_connection_close(devh_t *dev, conn_t *conn);
void nt_wlan_sleep_driver(void);
nt_status_t mlme_process_mgmt(dev_common_t *pDevCmn, uint8_t *bufPtr, uint32_t bufLen, uint32_t enc_flag, uint8_t rssi,
                              uint8_t *dev_id);
channel_t *wlan_get_home_channel(devh_t *dev);
NT_BOOL wlan_check_ht_association(devh_t *dev, bss_t *bss);
void wlan_set_ht_cap(devh_t *dev, struct ieee80211_ie_htcap *pHTCap);
void mlme_go_to_sleep(devh_t *dev);
conn_t *mlme_find_conn_by_station_id(devh_t *dev, uint8_t station_id);
#ifdef NT_MGMT_FRM_DUMP
void mlme_dump_mgmt_frame(char *pmsg, uint8_t *frm, uint16_t len);
#endif  // NT_MGMT_FRM_DUMP
#ifdef NT_FN_WMM_PS_STA
/**
 * @brief  call back function for eosp interrupt
 * @params tid: tid to which the raised interrupt corresponds
 *         more_bit: more bit status of the received packet
 *         sta_id: station id from which packet received
 * @return none
 */
void nt_mlme_eosp_isr_cb_fn(uint8_t tid, uint8_t more_bit, uint8_t staid);
#endif  // NT_FN_WMM_PS_STA

void test_read_dxe_frame(devh_t *dev, int **);
void nt_twt_set_dtim_stats(devh_t *dev, void *twt_dtim);
void nt_wnm_set_dtim_stats(devh_t *dev, void *wnm_dtim);
int8_t mlme_get_connection_state(uint8_t *mac_addr);

#ifdef SUPPORT_5GHZ
uint16_t mlme_get_mgmt_ctrl_rate(devh_t *dev);
void mlme_set_mgmt_rate(devh_t *dev, channel_t *ch);
#endif  // SUPPORT_5GHZ
uint8_t mlme_get_mgmt_rate();

/*****************************************************************
 * @brief Routine to check if the wake up for nt timer can be ignored
 *
 * @param slp_lst_head  head of the sleep list
 * @return TRUE: Ignore timers
 *         FALSE: Don't ignore timers
 ****************************************************************/

NT_BOOL nt_ignore_timer_wakeup(int slp_lst_head);
#endif /* _MLME_API_H_ */
