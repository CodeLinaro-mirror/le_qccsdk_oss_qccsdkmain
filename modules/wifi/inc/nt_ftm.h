/*
Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
*/

/*************************Calculation of timing values from raw values******************************/
/*
 *
 *  		scaled t1 = t1raw_value*(1/(60*1000000))  in sec  1/6000000=1.666*10^8sec=16.66ns
 *  		(1/(60 * 1000000))*1000000000 ns = 1000/60ns=10000/60 (100ps units)
 *
 *  		t1 = t1raw_value*16.66 ns
 *
 *
 *  		scaled_t4=((((t1_absolute_value+t4_t1_delta_value)*(256/3))+fac)*125)/64;
 *					t4=(t1_absolute_value+t4_t1_delta_value)->gives rawt4
 *					fac value resolution is 50ns/256
 *
 *					256/3= 16.66 / (50/256)       converting t4 into fc resolution of 50ns/256
 *
 *
 *					125/64 = (50/256)/0.1 == converting to 100ps range
 *					speed of light = 0.0299792458 meters / (100 picoseconds)
 *
 *
 *
 *
 * ***************************************************************************************************/

#ifndef CORE_WIFI_MLM_INCLUDE_NT_FTM_H_
#define CORE_WIFI_MLM_INCLUDE_NT_FTM_H_
#include "wifi_cmn.h"

#include "fwconfig_wlan.h"
#include "nt_flags.h"
#ifdef NT_FN_FTM
#include <stdint.h>

#define LOCATION_ON                         TRUE
#define LOCATION_OFF                        FALSE
#define FTM_IMPLEMENTED                     TRUE
#define FTM_ON                              TRUE
#define FTM_OFF                             FALSE
#define INITIATOR_FTM_SESSION_TIMEOUT_VALUE 8

#define NT_MIN_DELTA_TIME      1
#define NT_FTM_DTSF_TIME       5
#define FTM_BURST_DURATION_VAL 10
#define FTM_BURST_PERIOD_VAL   40
#define FTM_BURST_DUR_NO_PREF  15
#define NT_ASAP_TIME           1
#define NT_FTM_RETRY           3
#define NT_FTM_PM_SERVICE_ID   0

enum LOCATION_TYPE {
    FTM_LOCATION = 1,
    RSSI_LOCATION = 2,
    OFF_LOCATION = 0,
};

#ifdef NT_FN_FTM_2016V
enum FTM_ASAP {
    FTM_NON_ASAP = 0,
    FTM_ASAP = 1,
};

enum IFTMR {
    iFTMR_TRUE = 0,
    iFTMR_FALSE = 1,
};

enum FTM_BURST_DURATION {
    FTM_BD_RESERVED_0 = 0,
    FTM_BD_RESERVED_1 = 1,
    FTM_BD_RESERVED_250us = 2,
    FTM_BD_RESERVED_500us = 3,
    FTM_BD_RESERVED_1ms = 4,
    FTM_BD_RESERVED_2ms = 5,
    FTM_BD_RESERVED_4ms = 6,
    FTM_BD_RESERVED_8ms = 7,
    FTM_BD_RESERVED_16ms = 8,
    FTM_BD_RESERVED_32ms = 9,
    FTM_BD_RESERVED_64ms = 10,
    FTM_BD_RESERVED_128ms = 11,
    FTM_BD_RESERVED_12 = 12,
    FTM_BD_RESERVED_13 = 13,
    FTM_BD_RESERVED_14 = 14,
    FTM_BD_NO_PRFERENCE_15 = 15
};

enum FTM_STATUS_INDICATION {
    FTM_STATUS_RESERVED = 0,
    FTM_STATUS_OK = 1,
    FTM_STATUS_REQ_INCAP = 2,
    FTM_STATUS_REQ_FAILED = 3
};

typedef enum { FTM_UNINIT = 0, FTM_INIT, FTM_START, FTM_PROGRESS, FTM_RESEND, FTM_STOP, FTM_FUP } FTM_STATE;

typedef struct {
    FTM_STATE curr_state;
    FTM_STATE next_state;
    FTM_STATE prev_state;
    void (*ftm_fun)();
    void *data;
} FTM_STATE_SESSION;

#endif  // NT_FN_FTM_2016V

typedef enum {
    FTM_SESSION_START = 0,
    FTM_SESSION_STOP = 1,
    FTM_SESSION_CONTINUE = 2,
} FTM_SESSION;

typedef enum {
    FTM_INITIATOR = 0,
    FTM_RESPONDER = 1,
} FTM_MODE;
typedef enum {
    FTM_TRIGGER_STOP = 0,
    FTM_TRIGGER_START = 1,
} FTM_TRIGGER;

typedef enum {
    initiator_ftm_Session_timeout_timer_create = 1,
    initiator_ftm_Session_timeout_timer_start = 2,
    initiator_ftm_Session_timeout_timer_stop = 3,
    initiator_ftm_Session_timeout_timer_delete = 4,
} initiator_ftm_Session_timeout_timer_handler;

#pragma pack(push, 1)

typedef struct ftm_peer_info {
    uint8_t ftm_in_probeResp : 1;
    uint8_t ftm_in_assocResp : 1;
    uint8_t ftm_in_probeReq : 1;
    uint8_t ftm_in_assocReq : 1;
    uint8_t remote_ftm_mode : 2;
} FTM_PEER_INFO;

typedef struct ftm_info_bss {
    uint8_t ftm_in_probeResp : 1;
    uint8_t ftm_in_assocResp : 1;
    uint8_t ftm_in_probeReq : 1;
    uint8_t ftm_in_assocReq : 1;
    uint8_t remote_ftm_mode : 1;

} FTM_BSS_INFO;

typedef struct ftm_info_sta {
    uint8_t ftm_in_probeReq : 1;
    uint8_t ftm_in_assocReq : 1;
    uint8_t remote_ftm_mode : 1;
} FTM_STA_INFO;

/*
 * FTM Info Structure
 */

/*struct ftm_info
{
    uint8_t ftm_in_beacon : 1;
    uint8_t ftm_in_probeReq : 1;
    uint8_t ftm_in_probeResp : 1;
    uint8_t ftm_in_assocReq : 1;
    uint8_t ftm_in_assocResp : 1;
    uint8_t remote_ftm_mode : 1;
//	uint8_t ftm_req_frm : 1;
//	uint8_t ftm_resp_frm : 1;
#ifdef NT_FN_FTM_2016V
    uint8_t asap : 1;
    uint8_t trigger : 1;

    //uint8_t ftms_per_burst;
    //uint32_t burst_count;	// This Is Actual Value calculated from exponent value
    //uint8_t burst_period;
    uint32_t curr_burst;
    uint8_t curr_ftm;


     * Create FTM Timer
     * Timer Callback Function
    uint32_t delta_ftm_timer_id;
    TimerHandle_t info_delta_ftmTimer;

    uint32_t burst_duration_ftm_timer_id;
    TimerHandle_t info_burst_duration_ftmTimer;

    uint64_t tsf_timer;
#endif //NT_FN_FTM_2016V
#ifdef NT_FN_FTM_11V
    uint32_t t1_tod_ftm;
    uint32_t t4_toa_ack;
#endif //NT_FN_FTM_11V
#ifdef NT_FN_FTM_2016V
    uint64_t t1_tod_ftm;
    uint64_t t4_toa_ack;
    uint32_t ftm_fail_count;
#endif //NT_FN_FTM_2016V


};*/

#ifdef NT_FN_FTM_2016V

typedef struct ftm_sync_ie {
    uint8_t elementID;  // 255
    uint8_t length;
    uint8_t eID_extention;
    uint32_t tsf_sync_info;
} FTM_SYNC_IE;

typedef struct ftm_fields_t {
    uint8_t status_indication : 2;
    uint8_t value : 5;
    uint8_t reserved_1 : 1;
    uint8_t no_of_burst_exp : 4;
    uint8_t burst_dur : 4;
    uint8_t min_delta_ftm : 8;
    uint16_t partial_tsf_timer;
    uint8_t partial_tsf_timer_no_pref : 1;
    uint8_t asap_capable : 1;
    uint8_t asap : 1;
    uint8_t ftms_per_burst : 5;
    uint8_t reserved_2 : 2;
    uint8_t format_and_bw : 6;
    uint16_t burst_period;
} FTM_FIELDS_T;

typedef struct ftm_ie_t {
    uint8_t elementID;
    uint8_t length;
    FTM_FIELDS_T ftm_fileds;
} FTM_IE_T;

typedef struct ftm_full_ie {
    FTM_SYNC_IE sync_ie;
    FTM_IE_T params_ie;
} FTM_FULL_IE;
#endif  // NT_FN_FTM_2016V

#if (defined NT_FN_FTM_11V) || (defined NT_FN_FTM_2016V)
typedef struct ieee80211_ftm_request_frame {
    struct ieee80211_action ftm_header;
    uint8_t trigger;

#ifdef NT_FN_FTM_2016V

#if (LCI_MSMT_REQUEST)
    /*To_Do: May Need To Implement LCI Measurement Request*/
#endif

#if (LOCATION_CIVIC_MSMT_REQUEST)
    /*To_Do: May Need To Implement Civic Measurement Request*/
#endif

    FTM_IE_T ftm_req_params;
#endif  // NT_FN_FTM_2016V

} IEEE80211_2016_FTM_REQUEST_FRAME;

typedef struct ieee80211_ftm_action_frame {
#if (defined NT_FN_FTM_11V) || (defined NT_FN_FTM_2016V)
    struct ieee80211_action ftm_header;
    uint8_t dialog_token;
    uint8_t fUp_dialog_token;
#ifdef NT_FN_FTM_11V
    uint32_t time_of_depart;
    uint32_t time_of_arrival;
    uint8_t tod_error;
    uint8_t toa_error;
#endif  // NT_FN_FTM_11V
#ifdef NT_FN_FTM_2016V
    uint8_t time_of_depart[6];
    uint8_t time_of_arrival[6];
    uint16_t tod_error;
    uint16_t toa_error;
#endif  // NT_FN_FTM_2016V
#endif  // (defined NT_FN_FTM_11V) || (defined NT_FN_FTM_2016V)

#if (LCI_REPORT)
    /*To_Do: May Need To Implement LCI Measurement Report*/
#endif

#if (LOCATION_CIVIC_REPORT)
    /*To_Do: May Need To Implement Civic Measurement Request*/
#endif

#ifdef NT_FN_FTM_2016V
    FTM_IE_T ftm_ie_params;
    FTM_SYNC_IE ftm_sync_info_ie;
#endif  // NT_FN_FTM_2016V

} IEEE80211_2016_FTM_ACTION_FRAME;
#endif  //(defined NT_FN_FTM_11V) || (defined NT_FN_FTM_2016V)
        /*
         typedef struct ftm_config
       {
       #if (defined NT_FN_FTM_11V) ||(defined NT_FN_FTM_2016V)
           uint8_t ftm_mode : 1;	// Initiator Or Responder
           uint8_t location : 1;
           uint8_t location_type : 2;
           uint8_t trigger : 1;
           TickType_t ftm_period_time;
       #endif  //( NT_FN_FTM_11V) ||(NT_FN_FTM_2016V)
       #ifdef NT_FN_FTM_11V
           uint16_t ftm_count;
           uint8_t curr_ftm;
       #endif
       #ifdef NT_FN_FTM_2016V
           //uint8_t asap : 1;
           //uint8_t asap_cap : 1;
           uint8_t iFTM : 1;
           uint8_t ftm_asap_iftm : 1;
        
           //uint8_t no_of_bur_exp;
           //uint8_t ftms_per_burst;
           uint32_t burst_count;	// This Is Actual Value calculated from exponent value
           uint32_t curr_burst;
           //uint8_t burst_duration;
           //uint8_t status;
           //uint8_t min_delta_ftm;
        
           FTM_FIELDS_T ftm_params_config;
       //	uint32_t t1_tod_ftm;
       //	uint32_t t4_toa_ack;
        *
        */

/*
 * Create FTM Timer
 * Timer Callback Function
 */
/*
TimerHandle_t config_ftm_10_ms_timer;

#endif //NT_FN_FTM_2016V
    TickType_t min_delta_ftmTimer_value;
    uint32_t delta_ftm_timer_id;
    TimerHandle_t config_min_delta_ftmTimer_handle;

#ifdef NT_FN_FTM_2016V
    TickType_t burst_duration_ftmTimer_value;
    uint32_t burst_duration_ftm_timer_id;
    TimerHandle_t config_burst_duration_ftmTimer_handle;


    TickType_t burst_period_ftmTimer_value;
    uint32_t burst_period_ftm_timer_id;
    TimerHandle_t config_burst_period_ftmTimer_handle; //burst cnt initiator

    TimerHandle_t config_asap_ftmTimer;
#endif //#ifdef NT_FN_FTM_2016V

}FTM_CONFIG;
*/
typedef struct ftm_config_intiator {
    uint8_t iFTMR : 1;
#ifdef NT_FN_FTM_2016V
    TickType_t burst_period_ftmTimer_value;
    // uint32_t burst_period_ftm_timer_id;
    // TimerHandle_t config_burst_period_ftmTimer_handle; //burst cnt initiator
#endif  // NT_FN_FTM_2016V
    uint32_t prv_t1;
    uint32_t prv_t4;
    uint32_t t2_toa;
    uint32_t t3_tod;
    uint16_t fac;
    uint8_t dup_flag;
} FTM_INITIATOR_CNFG;

typedef struct ftm_config_responder {
    uint8_t iFTM : 1;
    uint32_t t1_tod_ftm;
    uint32_t t4_toa_ack;
    TickType_t min_delta_ftmTimer_value;
    uint32_t delta_ftm_timer_id;
    TimerHandle_t config_min_delta_ftmTimer_handle;

#ifdef NT_FN_FTM_2016V

    TickType_t burst_duration_ftmTimer_value;
    uint32_t burst_duration_ftm_timer_id;
    TimerHandle_t config_burst_duration_ftmTimer_handle;
#endif
} FTM_RESP_CNFG;

typedef struct ftm_dev {
    uint8_t ftm_mode : 1;       // Initiator Or Responder
    uint8_t location : 1;       // Location ON or OFF
    uint8_t location_type : 2;  // off or RTT or RSSI
    uint8_t trigger : 1;
    conn_t *conn;
    void *ftm_cfg;
    FTM_PEER_INFO *rx_cap_info;
#ifdef NT_FN_FTM_2016V
    FTM_FIELDS_T *dev_ftm_params;
    FTM_FIELDS_T *negotiated_ftm_params;

    uint16_t burst_count;  // This Is Actual Value calculated from exponent value
    uint16_t curr_burst;
#endif  // NT_FN_FTM_2016V
#ifdef NT_FN_FTM_11V
    uint16_t ftm_cnt;
#endif  // NT_FN_FTM_11V
    uint8_t fup_dailog_tok;
    uint16_t curr_ftm_cnt;
    uint16_t cnt_valid_rtt;
    uint32_t cal_val;

} FTM_DEV;

typedef struct usr_ftm_cfg {
    uint8_t ftm_mode : 1;       // Initiator Or Responder
    uint8_t location : 1;       // Location ON or OFF
    uint8_t location_type : 2;  // FTM or OFF
    uint8_t asap : 1;
    uint8_t ftms_per_burst;
    uint8_t no_of_bur_exp;
    uint8_t min_delta_ftm;
    uint8_t burst_duration;
    uint16_t burst_period;
    uint32_t cal_val;
    uint8_t format_and_bw;
} usr_ftm;

#pragma pack(pop)
typedef struct ftm_notification {
    uint8_t not_type;
    uint64_t data;
} FTM_NOTIFICATION;

#ifdef NT_FN_FTM_2016V
#define FTM_FIELDS_SIZE  sizeof(FTM_FIELDS_T)
#define FTM_IE_SIZE      sizeof(FTM_IE_T)
#define FTM_SYNC_IE_SIZE sizeof(FTM_SYNC_IE)
#endif  // NT_FN_FTM_2016V
#define FTM_REQUEST_FRAME_SIZE sizeof(IEEE80211_2016_FTM_REQUEST_FRAME)
#define FTM_ACTION_FRAME_SIZE  sizeof(IEEE80211_2016_FTM_ACTION_FRAME)
NT_BOOL nt_initiator_start_ftm_session(devh_t *dev, void *msg, uint8_t ftmCode);
NT_BOOL nt_initiator_stop_ftm_session(devh_t *dev);

WIFIReturnCode_t WIFI_Location_On();
WIFIReturnCode_t WIFI_Location_Off();
WIFIReturnCode_t WIFI_FTM_On();
WIFIReturnCode_t WIFI_FTM_Off();
WIFIReturnCode_t WIFI_Start_FTM_Req_Frame();
WIFIReturnCode_t WIFI_Location_Configure(void *msg);

NT_BOOL isLocationON(void);
void setLocationON(void);
void setLocationOFF(void);
NT_BOOL isFTMON(void);
void setFTMON(void);
void setFTMOFF(void);
NT_BOOL isFTMImplemented(void);
NT_BOOL set_ftm_rssi_config(void *ftm_config_info, NT_BOOL __attribute__((__unused__)) usr_input, devh_t *dev);

#ifdef NT_FN_FTM_11V
void nt_rtt_update_conn_id(uint8_t conn_id);
void nt_rtt_update_conn_tbl(conn_t *conn);
#endif  // NT_FN_FTM_11V

/*extern FTM_CONFIG nt_global_ftm_config;
extern struct ftm_info nt_rx_ftmInfo;*/
void nt_ftm_config(devh_t *dev, usr_ftm *ftm_config_info);
void nt_ftm_init(devh_t *dev);

void nt_ftm_min_delta_timeout_msg(TimerHandle_t __attribute__((__unused__)) timer_param);

void nt_ftm_min_delta_expiry_cb_fn(TimerHandle_t __attribute__((__unused__)) timer_handle);

#ifdef NT_FN_FTM_2016V
// extern FTM_FIELDS_T nt_dev_ftm_params;

NT_BOOL nt_create_and_send_ftm_req_frame(devh_t *dev, uint8_t ftmSession);
NT_BOOL create_and_send_ftm_response_frame(devh_t *dev, conn_t *conn, uint8_t dialog_token, uint8_t fup_dialog_token,
                                           FTM_IE_T *ftmIE, FTM_SYNC_IE *ftmSyncIE, uint64_t t4_toa, uint64_t t1_tod);

void nt_ftm_burst_period_timeout_msg(TimerHandle_t timer_param);
void nt_ftm_burst_duration_timeout_msg(TimerHandle_t timer_param);
void nt_ftm_tsf_corrected_burst_period_timeout_msg(TimerHandle_t timer_param);

void nt_prepare_ftm_ie(devh_t *dev, FTM_IE_T *ftm_req_params, uint8_t ftmSession);
void add_ftm_sync_ie(FTM_SYNC_IE *ftmSyncIE);

void nt_ftm_partial_tsf_bur_dur_resp_start_asap_timeout_msg(TimerHandle_t);
void nt_ftm_partial_tsf_bur_dur_rep_start_asap_cb_fn(TimerHandle_t);
void nt_ftm_partial_tsf_bur_start_non_asap_timeout_msg(TimerHandle_t);
void nt_ftm_partial_tsf_bur_start_non_asap_cb_fn(TimerHandle_t __attribute__((__unused__)) timer_handle);
void nt_ftm_tsf_corrected_burst_period_expiry_asap_cb_fn(TimerHandle_t __attribute__((__unused__)) timer_handle);
void nt_ftm_burst_period_expiry_cb_fn(TimerHandle_t __attribute__((__unused__)) timer_handle);
void nt_ftm_burst_duration_expiry_cb_fn(TimerHandle_t __attribute__((__unused__)) timer_handle);

TickType_t tx_ftm_get_burst_tm(uint16_t burst_dur);
NT_BOOL nt_ftm_initiator_params_negotiation(devh_t *dev, FTM_FIELDS_T *rxed_ftm_response_fileds);
void nt_rtt_update_gdev_conn(conn_t *conn, devh_t *dev);
#endif  // NT_FN_FTM_2016V
#ifdef NT_FN_FTM_11V
NT_BOOL tx_ftm_req(devh_t *dev, uint8_t ftmSession);
NT_BOOL tx_ftm_resp(devh_t *dev, conn_t *conn, uint8_t dialog_token, uint8_t fup_dialog_token, uint64_t t4_toa,
                    uint64_t t1_tod);
NT_BOOL rx_ftm_req(devh_t *dev, IEEE80211_2016_FTM_REQUEST_FRAME *rx_ftm_req_frame, conn_t *conn);
NT_BOOL rx_ftm_resp(IEEE80211_2016_FTM_ACTION_FRAME *rx_ftm_resp_frame, devh_t *dev);
#endif  // NT_FN_FTM_11V

uint64_t nt_rtt_calculation(uint32_t t1, uint32_t t2, uint32_t t3, uint32_t t4, uint32_t fac);
uint64_t nt_distance_cal(uint64_t rtt_ftm);
void nt_ftm_deinit(devh_t *dev);
void nt_create_ftm_burst_period_timer(devh_t *dev, FTM_FIELDS_T *rxed_ftm_response_fileds);
void nt_initiator_ftm_session_timout_timer_handler(devh_t *dev,
                                                   initiator_ftm_Session_timeout_timer_handler timer_state);
void nt_rtt_clear_buffers();
#endif
#endif /* CORE_WIFI_MLM_INCLUDE_NT_FTM_H_ */
