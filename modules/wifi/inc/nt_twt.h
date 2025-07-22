/*
Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#ifndef CORE_WIFI_SME_INC_NT_TWT_H_
#define CORE_WIFI_SME_INC_NT_TWT_H_
#include <stdio.h>
#include "wifi_cmn.h"

#include "fwconfig_wlan.h"
#include "nt_flags.h"

#ifdef NT_FN_TWT
#include "nt_common.h"
#include "wlan_dev.h"
#include "wlan_framegen.h"
#ifdef SUPPORT_RING_IF
#include "wifi_fw_mgmt_api.h"
#endif
#include "mlme_al.h"

/* Send Null Frame with PM=1 before starting of first SP
 *  Issue: NT AP buffers all traffic when STA sends NULL PM=1 frame.
 *  So, in TWT Case, NT AP wont send the traffic during SP.
 *  Macro WAR_TWT_NT_FERM_SAP added for that issue is not helping, hence, disabling this macro.
 */

//#define WAR_TWT_NT_FERM_SAP

#define TWT_PRINT_LOG_ERR(...)  NT_LOG_PRINT(COMMON, ERR, __VA_ARGS__)
#define TWT_PRINT_LOG_INFO(...) NT_LOG_PRINT(COMMON, INFO, __VA_ARGS__)
/* MAX TWT SESSIONS */
#define MAX_TWT_SESSIONS 1

/* If twt sleep time is not set by command then, it will set by default value i.e 120 sec */
//#define STARTING_TWT_ALIGNMENT				120

/* TWT_EXTENDED_ID : This id is not defined in the specs 11h or 11ax so hardcoded now */
#define TWT_EXTENDED_ID 40

#define TWT_ACTION_TYPE_ENTER 0 /* Action type to enter into TWT mode */
#define TWT_ACTION_TYPE_EXIT  1 /* Action type to exit from TWT sleep mode */

/*Config Begin*/  // used  for condition checking

#define NT_TWT_IMPLICIT 1
#define NT_TWT_EXPLICIT 0

#define NT_TWT_UNANNOUNCED 1
#define NT_TWT_ANNOUNCED   0

#define NT_TWT_MAX_WAKE_INTERVAL_LIMIT 0x20C47AE147

#define TWT_WAKE_DURATION_TIME_UNIT_256US  256   // TU=256us
#define TWT_WAKE_DURATION_TIME_UNIT_1024US 1024  // TU=1024us

#define TWT_SETUP_RESPONSE_RECV_WAIT_TIME_DEFAULT_MS 500  // TU=500ms
/*Config End*/

#define STA_TWT_SAFE_MINUS(a, b) (((a) > (b)) ? ((a) - (b)) : 0)

/* Enum Declaration */
/*enum declaration begin*/
typedef enum {
    TWT_REQUEST = 0,
    TWT_SUGGEST = 1,
    TWT_DEMAND = 2,
    TWT_GROUPING = 3,
    TWT_ACCEPT = 4,
    TWT_ALTERNATE = 5,
    TWT_DICTATE = 6,
    TWT_REJECT = 7
} twt_setup_cmd;

/* Function Declaration */
/* @struct	: twt_control_t
 * @brief	: control parameter for twt power-save parameter
 * */
typedef struct {
    uint8_t ndp_paging : 1;            ///< ndp paging for twt
    uint8_t responder_pm_mode : 1;     ///< rsponder pm mode i.e responder can sleep or not
    uint8_t negotiatn_type : 2;        ///< negotiation type for individual and broadcast
    uint8_t twt_info_frm_disable : 1;  ///< Flag for reception for info frame disable
    uint8_t wake_duration_uint : 1;    ///< wakeup unit used used i.e TU or 256us
    uint8_t reserved : 2;
} __ATTRIB_PACK twt_control_t;

/* @struct	: twt_req_type_t
 * @brief	: twt req type for twt parameter
 * */
typedef struct {
    uint16_t twt_req : 1;            ///< twt request type i.e sta or ap
    uint16_t twt_setup_cmd : 3;      ///< twt setup command type i.e 0->req,
                                     ///< 1->suggest,2->demand,3->grouping,4->accept,5->alternate,6->dictate,7->reject
    uint16_t trigger_type : 1;       ///< 0->no tigger support, 1->trigger,
    uint16_t implicite_type : 1;     ///< 0->explicite, 1->implicite,
    uint16_t flow_type : 1;          ///< 0->announce, 1->un-announce,
    uint16_t flow_identifier : 3;    ///< type of frame used for announcement
    uint16_t twt_wake_exponent : 5;  ///< wake interval exponent
    uint16_t protection : 1;         ///< 0->no Portection, 1->Protection disable,
} __ATTRIB_PACK twt_req_type_t;

typedef struct _twt_flow_field {
    union {
        uint8_t u8;
        struct {
            uint8_t reserved : 5, wake_tbtt_nego : 1, broadcast : 1, teardown_all : 1;
        } cmn;
        struct {
            uint8_t flow_id : 3, reserved : 2, wake_tbtt_nego : 1, broadcast : 1, teardown_all : 1;
        } i_twt;
        struct {
            uint8_t b_twt_id : 5, wake_tbtt_nego : 1, broadcast : 1, teardown_all : 1;
        } b_twt;
    } u;
} __ATTRIB_PACK twt_flow_field;

typedef struct _ieee80211_action_teardown {
    struct ieee80211_action act_header;
    twt_flow_field twt_flow;
} __ATTRIB_PACK ieee80211_action_teardown_t;

/* @struct	: twt_param_info_t
 * @brief	: twt parameter info for twt power-save parameter
 * */
typedef struct {
    twt_req_type_t req_type;     ///< twt request type
    uint64_t target_wake_time;   ///< wake time tsf value
    uint8_t min_wake_duration;   ///< min wake duration as per uint difined in control type
    uint16_t twt_wake_mentissa;  ///< wakeup unit used used i.e TU or 256us
    uint8_t twt_channel;         ///< channel type used for twt
} __ATTRIB_PACK twt_param_info_t;

/* @struct	: twt_req_type_t
 * @brief	: twt req type for twt parameter
 * */
typedef struct {
    uint16_t twt_req : 1;        ///< twt request type i.e sta or ap
    uint16_t twt_setup_cmd : 3;  ///< twt setup command type i.e 0->req,
                                 ///< 1->suggest,2->demand,3->grouping,4->accept,5->alternate,6->dictate,7->reject
    uint16_t trigger_type : 1;   ///< 0->no tigger support, 1->trigger,
    uint16_t last_broadcast_param_set : 1;  ///< 0->explicite, 1->implicite,
    uint16_t flow_type : 1;                 ///< 0->announce, 1->un-announce,
    uint16_t broad_twt_recommendation : 3;  ///< type of frame used for announcement
    uint16_t twt_wake_exponent : 5;         ///< wake interval exponent
    uint16_t reserved : 1;                  ///< 0->no Portection, 1->Protection disable,
} twt_broad_req_type_t;

/* @struct	: twt_req_type_t
 * @brief	: twt req type for twt parameter
 * */
typedef struct {
    uint16_t reserved : 3;
    uint16_t broad_twt_id : 5;
    uint16_t broad_twt_persistence : 8;
} twt_broad_info_t;

/* @struct	: twt_param_info_t
 * @brief	: twt parameter info for twt power-save parameter
 * */
typedef struct {
    twt_broad_req_type_t req_type;    ///< twt request type
    uint16_t target_wake_time;        ///< wake time tsf value
    uint8_t min_wake_duration;        ///< min wake duration as per uint difined in control type
    uint16_t twt_wake_mentissa;       ///< wakeup unit used used i.e TU or 256us
    twt_broad_info_t twt_broad_info;  ///< channel type used for twt
} twt_broad_param_info_t;

/* @struct	: twt_elemtn_id
 * @brief	: twt elemtn ie consist control and twt param info
 * */
typedef struct {
    uint8_t twt_eid;         ///< twt element id
    uint8_t length;          ///< twt elemtn lenght
    twt_control_t twt_ctrl;  ///< twt control parameter
    void *twt_param_info;    ///< Flag for reception for info frame disable
} __ATTRIB_PACK twt_element_t;

#ifdef NT_TWT_ADD_DEL_CODE

/* @struct	: twt_elemtn_id
 * @brief	: twt elemtn ie consist control and twt param info
 * */
typedef struct {
    uint8_t twt_eid;                        ///< twt element id
    uint8_t length;                         ///< twt elemtn lenght
    twt_control_t twt_ctrl;                 ///< twt control parameter
    twt_broad_param_info_t twt_param_info;  ///< Flag for reception for info frame disable
} twt_element_broadcast_del_t;
#endif

/* @struct	: twt_config_t
 * @brief	: parameter for twt power-save configuration
 * */
typedef struct {
    NT_BOOL is_requested_ap_have_twt_capabilities;    ///< check for twt capability at ap side
    NT_BOOL is_requesting_sta_have_twt_capabilities;  ///< check for twt capability at sta side
    NT_BOOL
        allow_wake_up_when_sta_data_availablity;  ///< Permission for Wakeup enable due to data availability at sta side
} twt_config_t;

#ifdef SUPPORT_TWT_AP
#define MAX_TWT_AP_PENDING_FRM 3
typedef struct {
    nt_osal_timer_handle_t twt_ap_sp_start_timer_hndl;
    nt_osal_timer_handle_t twt_ap_sp_end_timer_hndl;
    NT_BOOL disable_ap_twt_timers;
    uint8_t *twt_pending_frame_buf[MAX_TWT_AP_PENDING_FRM];
    uint16_t buffer_len[MAX_TWT_AP_PENDING_FRM];
#ifdef FEATURE_TX_COMPLETE
    pfn_callback pending_frm_mgmt_cb[MAX_TWT_AP_PENDING_FRM];
    void *pending_frm_mgmt_args[MAX_TWT_AP_PENDING_FRM];
#endif
} twt_ap_struct_t;

#endif /* SUPPORT_TWT_AP */

typedef enum {
    CLK_GATED_SLP_REQ = 1,      // clk_gated without wlan sleep
    CLK_GATED_SLP_MAC_OFF_REQ,  // clk_gated with wlan sleep
    MCU_SLP_REQ,                // mcu sleep with wlan sleep
#ifdef PLATFORM_FERMION
    LIGHT_SLP_REQ,  // light sleep with wlan sleep
#endif
    SLP_TYPE_REQUEST_MAX,
} twt_soc_sleep_type_request_t;

#define TWT_SOC_SLP_REQ_SET(twtstruct, req) ((twtstruct)->twt_slp_request = req)
#define TWT_SOC_SLP_REQ_GET(twtstruct)      ((twtstruct)->twt_slp_request)

typedef enum {
    TWT_ST_SP_INTR,
    TWT_ST_SP_START,
    TWT_ST_SP_END,
    TWT_ST_WLAN_SLEEP,
    TWT_ST_RRI_RESTORE,
    TWT_ST_WLAN_RX_DISABLE,
    TWT_ST_WLAN_RX_ENABLE,
    TWT_ST_WL_FAKE_SLEEP,
} twt_state_t;

#define TWT_STATE_SET(twtstruct, st) ((twtstruct)->twt_current_state = st)
#define TWT_STATE_GET(twtstruct)     ((twtstruct)->twt_current_state)

#define INVALID_SLEEP_MODE (uint32_t)(-1)

/* @struct	: twt_sp_log_t
 * @brief	: struct to hold SP logging data
 * */

typedef struct {
    uint32_t twt_sp_start_tsf;
    uint32_t twt_sp_end_tsf;
} twt_cyclic_log_t;

/* @struct	: twt_persist_t
 * @brief	: struct to hold all user globals and it will persist even after teardown
 * */

typedef struct {
    uint32_t user_forced_twt_sleep;
    uint32_t twt_clk_gated_pre_wakeup_time_us;
    uint32_t twt_mcu_pre_wakeup_time_us;
#if defined(SUPPORT_LIGHT_SLEEP_FOR_TWT)
    uint32_t twt_light_sleep_pre_wakeup_time_us;
#endif /*SUPPORT_LIGHT_SLEEP_FOR_TWT*/
    uint16_t twt_cyclic_log_max_sps;
    uint16_t twt_cyclic_log_sp_num;
    twt_cyclic_log_t *twt_cyclic_log_array;

    NT_BOOL twt_debug_print_enabled;
    NT_BOOL twt_pm_bit_enabled;  // enable pm bit in all frames
} twt_persist_t;

/* @struct	: twt_data_stats_t
 * @brief	: tx and rx stats for both data and mgmt for the last ref TWT SP.
 * */
typedef struct {
    uint16_t data_tx;
    uint16_t data_rx;
    uint16_t mgmt_tx;
    uint16_t mgmt_rx;
    uint32_t last_ref_twt_sp;
} twt_data_stats_t;

/* @struct	: twt_struct_t
 * @brief	: parameter for twt power-save parameter
 * */
typedef struct {
    uint8_t twt_enable;      ///< To check sleep mode is feature is enable/disable
    uint8_t twt_negotiated;  // TWT session is established
    uint8_t inside_sp;
    uint8_t twt_broadcast;           ///< To check twt is individual or broadcast
    uint8_t twt_mode_set_initiated;  ///< Flag to check the sta is currently in Enter sleep mode stat
    uint8_t twt_dialog_id;           // unique id between host and FW to represent TWT session
    uint8_t twt_flow_id;             // unique id between AP and STA to represent TWT session
    uint8_t twt_setup_cmd;
    uint8_t twt_trigger_type;
    uint8_t twt_flow_type;
    uint8_t twt_min_wake_duration;
    uint8_t twt_wakeup_type;  // 1: if periodic ; 0: if aperiodic.
    uint8_t twt_dtim_enable_policy;
    uint8_t twt_setup_action_dialog_token;  // For STA, its filled locally and For AP, we receive it from STA.
    uint8_t twt_send_null_before_first_sp;
    uint8_t twt_teardown_cmd_pending;
    uint8_t dpm_data_availability_interrupt_received : 1,  ///< Flag for data availability interrupt
        is_connected_ap_have_proxy_arp_support : 1, twt_sleep_aborted_wakeup : 1, twt_cb_due_to_wakelock : 1,
        flag_rsvd : 4;
#ifdef SUPPORT_STA_TWT_RENEG
    /* Bitmap of enum VS_AF_TYPE */
    uint8_t twt_vs_af_types_rcvd;
#endif
    uint16_t twt_wake_tu;  // minimum wake duration unit in units of 256us or 1TUs
    uint16_t twt_wake_time_alignment;
    uint32_t twt_bmiss_cnt;  ///< Beacon miss count during TWT session. Its for debugging, Count should not increase
    uint64_t twt_wake_time_tsf;
    uint64_t twt_sp_end_time_tsf;
    uint64_t twt_wake_interval;
    uint64_t twt_usr_config_start_tsf;
#ifdef SUPPORT_STA_TWT_RENEG
    uint8_t twt_new_min_wake_duration;
    uint16_t twt_new_wake_tu;  // minimum wake duration unit in units of 256us or 1TUs
    uint32_t twt_new_offset_from_tsf;
    uint64_t twt_new_param_effective_tsf;
    uint64_t twt_new_wake_interval;
#endif
    twt_config_t twt_config;  ///< configuration for twt sleep mode feature
    twt_soc_sleep_type_request_t twt_slp_request;
    twt_state_t twt_current_state;
#ifdef SUPPORT_TWT_AP
    twt_ap_struct_t *twt_ap_struct;
#endif /* SUPPORT_TWT_AP */
#ifdef SUPPORT_TWT_STA
    nt_osal_timer_handle_t twt_sta_twt_setup_timer_hndl;
#endif /* SUPPORT_TWT_STA */
    twt_data_stats_t twt_dp;
} twt_struct_t;

/* @struct	: twt_ps_struct_stats_ap_t
 * @brief	: parameter for twt power save statistics for ap
 * */
typedef struct {
    uint32_t twt_power_save_enter_count;                             ///< count for enter into twt power save mode
    uint32_t twt_power_save_exit_count;                              ///< count for exit from twt power save mode
    uint32_t twt_power_save_total_enter_sleep_mode_req_frame_recv;   ///< total enter sleep mode req recv
    uint32_t twt_power_save_total_enter_sleep_mode_resp_frame_sent;  ///< total enter sleep mode resp sent
    uint32_t twt_power_save_total_exit_sleep_mode_req_frame_recv;    ///< total exit sleep mode req recv
    uint32_t twt_power_save_total_exit_sleep_mode_resp_frame_sent;   ///< total exit sleep mode resp sent
} twt_struct_stats_ap_t;

/* @struct	: twt_ps_struct_stats_sta_t
 * @brief	: parameter for twt power save statistics for sta side
 * */
typedef struct {
    uint32_t twt_power_save_enter_count;                                 ///< count for enter into twt power save mode
    uint32_t twt_power_save_exit_count_due_to_sta_data_avail;            ///< count for exit from twt mode due to data
                                                                         ///< availability from sta
    uint32_t twt_power_save_total_enter_sleep_mode_req_frame_sent;       ///< total enter sleep mode req sent
    uint32_t twt_power_save_total_enter_sleep_mode_resp_frame_received;  ///< total enter sleep mode resp received
    uint32_t twt_power_save_total_exit_sleep_mode_req_frame_sent;        ///< total exit sleep mode req sent
    uint32_t twt_power_save_total_exit_sleep_mode_resp_frame_received;   ///< total exit sleep mode resp recv

    uint32_t twt_frame_drop_cnt;  ///< frame drops during TWT session. Its for debugging count should not increase
    uint32_t twt_frame_sent_during_sp_cnt;   ///< Total number of frame sent within TWT SP window, once TWT negotiated
    uint32_t twt_frame_sent_outside_sp_cnt;  ///< Total number of frame sent outside TWT window, once TWT negotiated
    uint64_t twt_last_sp_end_tsf;
    uint64_t twt_last_sp_start_tsf;
    uint64_t twt_si_duration;
    uint64_t twt_sp_duration;
    uint32_t twt_stats_print_intv;   // Interval between each stats prints
    uint8_t twt_stats_print_enable;  // Enable or disable periodic twt stats printing
    uint32_t twt_sp_count;           // Total number of SP
    uint32_t twt_eosp_recvd_cnt;     // No of EOSP received
    uint64_t twt_stats_print_tsf;    // TSF time to print stats

} twt_struct_stats_sta_t;

#ifdef SUPPORT_TWT_STA
typedef struct {
    uint8_t dialog_id;         /* unique identifier between Fw and App to representing a twt session > 0 */
    uint8_t negotiation_type;  /* 0: Broadcast TWT, 1: Individual TWT */
    uint8_t flow_type;         /* 0: Announced, 1: unannounced */
    uint8_t trigger_type;      /* 0: non-triggered, 1: triggered twt */
    uint32_t wake_duration;    /* wake duration for device. Its TWT SP (Service period)in ms */
    uint32_t wake_interval;    /* Time between wake duration. Its TWT SI (Service Interval) in ms */
    uint32_t twt_start_tsf_lo; /* Its lower 32 bit of Start TSF. App to specify when first twt starts, TWT Offset tsf.
                                  When its set to 0 (hi and lo), FW will decide the value */
    uint32_t twt_start_tsf_hi; /* Its higher 32 bit of start tsf. */
} twt_setup_frame_details;
#endif

#define TWT_STEP_ROUND_DN(from, step, until)                \
    do {                                                    \
        if (((step) != 0) && ((from) < (until))) {          \
            (from) += ((until) - (from)) / (step) * (step); \
        }                                                   \
    } while (0)

#define TWT_STEP_ROUND_UP(from, step, until)                          \
    do {                                                              \
        if (((step) != 0) && ((from) < (until))) {                    \
            if (((until) - (from)) % (step) == 0) {                   \
                (from) = (until);                                     \
            } else {                                                  \
                (from) += (((until) - (from)) / (step) + 1) * (step); \
            }                                                         \
        }                                                             \
    } while (0)

#define TWT_STEP_ROUND_UP2(from, step, until)         \
    do {                                              \
        uint64_t from_bak = (from);                   \
        TWT_STEP_ROUND_DN((from), (step), (until));   \
        if ((from) == from_bak || (from) < (until)) { \
            (from) += (step);                         \
        }                                             \
    } while (0)

typedef enum EVENT_TYPE_E {
    TWT_EVENT_WKE_CB_SPSTART = 0,
    TWT_EVENT_WKE_UP_HAL_SP_END_CAL,            // 1
    TWT_EVENT_WKE_UP_PROCESS_TMR3_CONFIGURED,   // 2
    TWT_EVENT_WKE_UP_TRNS_WAKE,                 // 3
    TWT_EVENT_SLP_CB_SP_END_START_TMR3_EXPIRE,  // 4
    TWT_EVENT_ENTR_PS_MODE_CONFIG,              // 5
    TWT_EVENT_SP_END_CONFIGURE_SLP,             // 6
    TWT_EVENT_SP_END,                           // 7
    TWT_EVENT_SLPABORT_WAKEUP,                  // 8
    TWT_EVENT_SP_END_MGMT_RECV,                 // 9
    TWT_EVENT_SP_END_BCN_MISS_CAL,              // 10
    TWT_EVENT_TEARDOWN,                         // 11
    TWT_EVENT_EOSP_INT,                         // 12
    TWT_EVENT_SLP_CB_FN_CALLED,                 // 13
    TWT_EVENT_WLAN_SLEEP_CONFIGURED,            // 14
    TWT_EVENT_WLAN_RX_DISABLE,                  // 15
    TWT_EVENT_WLAN_RX_ENABLE,                   // 16
    TWT_EVENT_MIN_CB_FN_CALLED,                 // 17
    TWT_EVENT_RRI_RESTORED,                     // 18
    TWT_EVENT_SUPURIOUS_WAKUP_INTR,             // 19
    TWT_EVENT_AON_TMR_TRIGGERED,                // 20
    TWT_EVENT_AON_TMR_CONFIGURED,               // 21
    TWT_EVENT_MAX,
} EVENT_TYPE_T;

#ifdef ENABLE_TWT_EVENT_LOGGING
typedef struct event_log_s {
    EVENT_TYPE_T evt_type;
    uint64_t evt_timestmp;
    uint64_t msg1;
    int64_t msg2;
} event_log_t;

#define EVENT_LOG_TBL_SIZE 32
extern event_log_t g_twt_event_log_tbl[EVENT_LOG_TBL_SIZE];
extern uint16_t g_twt_event_log_tbl_index;

#endif

#if (!defined(SUPPORT_RING_IF) && !defined(SUPPORT_RING_IF_ONLY))
#if defined(__GNUC__)
#define PACKSTRUCT __attribute__((packed))
#else
#define PACKSTRUCT
#endif

typedef struct {
    uint16_t msg_id;    /* The message id identifying the wlan cmd */
    uint8_t network_id; /* network_id associated with the command */
    uint8_t reserved;   /* reserved */
} wlan_cmd_hdr;

typedef struct {
    uint16_t msg_id;    /* The message id identifying the wlan evt */
    uint8_t network_id; /* network_id associated with the command */
    uint8_t status;     /* over all status */
} wlan_evt_hdr;

#ifdef SUPPORT_TWT_STA
typedef struct {
    wlan_cmd_hdr cmd_hdr;
    uint16_t reserved_1;
    uint8_t dialog_id;         /* unique identifier between WLAN Firmware and App to representing a twt session > 0 */
    uint8_t negotiation_type;  /* 0: Individual TWT, 1: Broadcast TWT */
    uint32_t wake_duration;    /* wake duration for device. Its TWT SP (Service period)in ms */
    uint32_t wake_interval;    /* Time between wake duration. Its TWT SI (Service Interval) in ms */
    uint32_t twt_start_tsf_lo; /* Its lower 32 bit of Start TSF. App to specify when first twt starts, TWT Offset tsf.
                                  When its set to 0 (hi and lo), FW will decide the value */
    uint32_t twt_start_tsf_hi; /* Its higher 32 bit of start tsf. */
    uint8_t flow_type;         /* 0: Announced, 1: unannounced */
    uint8_t trigger_type;      /* 0: non-triggered, 1: triggered twt */
    uint16_t reserved_2;
} PACKSTRUCT wlan_twt_setup_cmd_t;

typedef struct {
    wlan_evt_hdr evt_hdr;
    uint16_t reserved_1;
    uint8_t dialog_id;         /* unique identifier between WLAN Firmware and App to representing a twt session > 0 */
    uint8_t negotiation_type;  /* 0: Individual TWT, 1: Broadcast TWT */
    uint32_t wake_duration;    /* wake duration for device. Its TWT SP (Service period)in ms */
    uint32_t wake_interval;    /* Time between wake duration. Its TWT SI (Service Interval) in ms */
    uint32_t twt_start_tsf_lo; /* Its lower 32 bit of Start TSF. App to specify when first twt starts, TWT Offset tsf.
                                  When its set to 0 (hi and lo), FW will decide the value */
    uint32_t twt_start_tsf_hi; /* Its higher 32 bit of start tsf. */
    uint8_t flow_type;         /* 0: Announced, 1: unannounced */
    uint8_t trigger_type;      /* 0: non-triggered, 1: triggered twt */
    uint8_t reason_code;       /* reason code wlan_twt_evt_status_t */
    uint8_t reserved_2;        /* reserved */
} PACKSTRUCT wlan_twt_setup_evt_t;

typedef struct {
    wlan_cmd_hdr cmd_hdr;
    uint16_t reserved_1;
    uint8_t dialog_id;  /* unique identifier between WLAN Firmware and App to representing a twt session > 0 */
    uint8_t reserved_2; /* reserved */
} PACKSTRUCT wlan_twt_teardown_cmd_t;

typedef struct {
    wlan_evt_hdr evt_hdr;
    uint8_t reserved_1;
    uint8_t host_initiated;
    uint8_t dialog_id;   /* unique identifier between WLAN Firmware and App to representing a twt session > 0 */
    uint8_t reason_code; /* reason code wlan_twt_evt_status_t */
} PACKSTRUCT wlan_twt_teardown_evt_t;

typedef struct {
    wlan_cmd_hdr cmd_hdr;
    uint16_t reserved_1;
    uint8_t dialog_id;  /* unique identifier between WLAN Firmware and App to representing a twt session > 0 */
    uint8_t reserved_2; /* reserved */
} PACKSTRUCT wlan_twt_status_cmd_t;

typedef struct {
    wlan_evt_hdr evt_hdr;
    uint16_t reserved_1;
    uint8_t dialog_id;         /* unique identifier between WLAN Firmware and App to representing a twt session > 0 */
    uint8_t negotiation_type;  /* 0: Broadcast TWT, 1: Individual TWT */
    uint32_t wake_duration;    /* wake duration for device. Its TWT SP (Service period)in ms */
    uint32_t wake_interval;    /* Time between wake duration. Its TWT SI (Service Interval) in ms */
    uint32_t twt_start_tsf_lo; /* Its lower 32 bit of Start TSF. App to specify when first twt starts, TWT Offset tsf.
                                  When its set to 0 (hi and lo), FW will decide the value */
    uint32_t twt_start_tsf_hi; /* Its higher 32 bit of start tsf. */
    uint8_t flow_type;         /* 0: Announced, 1: unannounced */
    uint8_t trigger_type;      /* 0: non-triggered, 1: triggered twt */
    uint8_t reason_code;       /* reason code wlan_twt_evt_status_t */
    uint8_t reserved_2;        /* reserved */
} PACKSTRUCT wlan_twt_status_evt_t;

typedef enum {
    WLAN_TWT_EVT_STATUS_OK = 0,
    WLAN_TWT_EVT_DIALOG_ID_NOT_EXIST = 1,
    WLAN_TWT_EVT_INVALID_PARAM = 2,
    WLAN_TWT_EVT_NO_RESOURCE = 3,
    WLAN_TWT_EVT_FW_NOT_READY = 4,
    WLAN_TWT_EVT_NO_ACK = 5,
    WLAN_TWT_EVT_NO_RESPONSE = 6,
    WLAN_TWT_EVT_DENIED = 7,
    WLAN_TWT_EVT_UNKNOWN_ERROR = 8,
    WLAN_TWT_EVT_STA_NOT_ASSOCIATED = 9,
} wlan_twt_evt_status_t;

#endif  // SUPPORT_TWT_STA
#endif  // SUPPORT_RING_IF

/**
 * @func nt_twt_setup_timeout_handler
 * @brief This function is used to handle nt_twt_setup_timeout_handler
 * @Return  void
 * @Param dev nt_osal_timer_handle_t thandle
 */
void nt_twt_setup_timeout_handler(nt_osal_timer_handle_t thandle);

/**
 * @func nt_twt_setup_timeout
 * @brief This function is used to handle nt_twt_setup_timeout
 * @Return void
 * @Param dev nt_osal_timer_handle_t thandle
 */
void nt_twt_setup_timeout(nt_osal_timer_handle_t thandle);

void nt_twt_update_event(EVENT_TYPE_T evt, uint64_t timestmp, uint64_t msg1, int64_t msg2);

/* Function declarations */
/**
 *	@func	nt_twt_init
 *	@brief	This function is used to initialize parameter related with twt feature.
 * 	@Return none
 * 	@Param	none
 */
void nt_twt_init(devh_t *dev);

/* Function declarations */
/**
 *	@func	nt_twt_debug_print_enabled
 *	@brief	This function returns whether nt_log_print needs to enabled while we are in TWT Path.
 * 	@Return none
 * 	@Param	none
 */
NT_BOOL nt_twt_debug_print_enabled();

/**
 *	@func	nt_twt_add_constraint_parameter_element
 *	@brief	This function is used to add twt constraint parameter IE in the frame.
 * 	@Param	frm : pointer to the frm in which IE is to be added
 * 			start_twt_align : start twt alignment time
 * 			max_twt_session : max twt session
 * 	@Return pointer to the frame after adding twt constraint parameter IE
 */
uint8_t *nt_twt_add_constraint_parameter_element(uint8_t *frm, uint16_t start_twt_align, uint8_t max_twt_session);

/**
 *	@func	nt_twt_sleep_mode_enter_frame
 *	@brief	This function is used to create and send twt sleep mode enter frame.
 * 	@Return NT_OK -> send successfully , NT_ETXFAIL -> Transmission failed
 * 	@Param	dev -> pointer to device structure
            cb -> callback function
            params -> user params
 */
#ifdef FEATURE_TX_COMPLETE
nt_status_t nt_twt_sleep_mode_enter_frame(devh_t *dev, pfn_callback pfn_cb, void *params);
#else
nt_status_t nt_twt_sleep_mode_enter_frame(devh_t *dev);
#endif /* FEATURE_TX_COMPLETE */

/**
 *	@func	nt_twt_recv_twt_action_frame
 *	@brief	This function is used to handle received twt action frame.
 * 	@Return NT_OK -> send successfully , NT_ERXFAIL -> Error in Rx frame
 * 	@Param	dev -> Pointer to device structure
 * 			frm -> received frame pointer
 * 			frmend -> pointer to end of frame
 * 			conn -> pointer to connection structure
 */
nt_status_t nt_twt_recv_twt_action_frame(devh_t *dev, uint8_t *frm, uint8_t __attribute__((__unused__)) * frmend,
                                         conn_t *conn);

/**
 *	@Func 	:	nt_twt_wake_timeout_cb
 *	@Brief 	: 	functin for twt wake timer expire
 *	@Param	:	timer handler
 *	@Return	:	none
 */
void nt_twt_wake_timeout_cb(TimerHandle_t __attribute__((__unused__)) timer_handle);

/**
 *	@Func 	:	nt_twt_sleep_timeout_msg
 *	@Brief 	: 	This API Posts twt sleep timeout message to wmi
 *	@Param	:	timer handler
 *	@Return	:	none
 */
void nt_twt_sleep_timeout_msg(TimerHandle_t thandle);

/**
 *	@Func 	:	calculate_wakeup_time_mentissa_exponent
 *	@Brief 	: 	functin to calculate wakeup time from mentissa and exponent
 *	@Param	:	wakeuptime
 *	@Param	:	output mentissa
 *	@Param	:	output exponent
 *	@Return	:	none
 */
uint64_t nt_twt_calculate_next_twt_twbtt(devh_t *dev);

/**
 * @Func : nt_twt_compute_start_time
 * @Brief : functin to calculate twt start time of SP
 * @Param : dev
 * @Return : return start time in tsf (in us)
 */
uint64_t nt_twt_compute_start_time(devh_t *dev);

/**
 *	@Func 	:	calculate_wakeup_time_mentissa_exponent
 *	@Brief 	: 	functin to calculate wakeup time from mentissa and exponent
 *	@Param	:	wakeuptime
 *	@Param	:	output mentissa
 *	@Param	:	output exponent
 *	@Return	:	none
 */
void nt_twt_calculate_wakeup_time_mentissa_exponent(uint64_t *waketime, uint16_t mentis, uint8_t exponent);

/**
 *	@Func 	:	calculate_mentissa_exponent_from_wakeup_time
 *	@Brief 	: 	functin to calculate mentissa and exponent of wakeup time
 *	@Param	:	wakeuptime
 *	@Param	:	output mentissa
 *	@Param	:	output exponent
 *	@Return	:	none
 */
void nt_twt_calculate_mentissa_exponent_from_wakeup_time(uint64_t wake_interval, uint16_t *mentis, uint8_t *exponent);

/**
 *	@func	nt_twt_enter_sleep_callback
 *	@brief	This function is call back function to enter into sleep mode.
 * 	@Return none
 * 	@Param	none
 */
void nt_twt_sleep_cb(void);

/**
 *	@func	nt_twt_wakeup_callback
 *	@brief	This function is call back function wakeup from TWT sleep mode.
 * 	@Return none
 * 	@Param	none
 */
void nt_twt_wakeup_cb(void);

/**
 *	@func	nt_twt_wakeup_session_processing
 *	@brief	This function is call back function for TWT session wakeup.
 * 	@Return none
 * 	@Param	wkup_delay which is a delay to service the wakeup session
 */
uint64_t nt_twt_wakeup_session_processing(__unused uint32_t wkup_delay);

/**
 *	@func	nt_wnm_enter_sleep_callback
 *	@brief	This function is call back function to enter into sleep mode.
 * 	@Return none
 * 	@Param	none
 */
void nt_twt_enter_sleep_callback(void);

/**
 *	@func	nt_twt_wakeup_callback
 *	@brief	This function is call back function wakeup from TWT sleep mode.
 * 	@Return none
 * 	@Param	none
 */
uint64_t nt_twt_wakeup_callback(void);

/**
 *	@func	nt_twt_transition_to_awake
 *	@brief	This function is used to notify connection manager for becon receive and set
 *			the tramsition state to AWAKE.
 * 	@Return NT_OK -> send successfully , NT_EFAIL -> Operation failed
 * 	@Param	dev -> Pointer to device structure
 */
void nt_twt_transition_to_awake(devh_t *dev);

/**
 *	@func	nt_twt_wakeup_callback
 *	@brief	This function is call back function wakeup from TWT sleep mode.
 * 	@Return none
 * 	@Param	wkup_reason  : SOC reason for wakeup
 */
void nt_twt_sleep_wakeup_callback(soc_wkup_reason wkup_reason);

/**
 *	@func	nt_twt_wakeup_processing
 *	@brief	This function is call back function wakeup from TWT sleep mode.
 * 	@Return none
 * 	@Param	none
 */
void nt_twt_wakeup_processing(devh_t *dev);

/**
 *	@func	nt_twt_is_negotiated
 *	@brief	This function will return the TWT negotiated status.
 * 	@Return Boolean
 * 	@Param	none
 */
NT_BOOL nt_twt_is_negotiated(void);

/**
 * @func nt_twt_is_in_sp
 * @brief Function will return TRUE if TWT is in SP.
 * @Return Boolean
 * @Param none
 */
NT_BOOL nt_twt_is_in_sp(void);

/**
 * @func nt_twt_is_sleep_allowed
 * @brief Function will return TRUE either TWT is not negotiated or twt is not in SP
 * @Return Boolean
 * @Param none
 */
NT_BOOL nt_twt_is_sleep_allowed(void);

/**
 *	@func	nt_twt_enter_into_sleep
 *	@brief	This function is used to go twt sleep.
 * 	@Return none
 * 	@Param	device structure
 */
void nt_twt_enter_into_sleep(devh_t *dev);

/**
 *	@func	nt_twt_teardown_done
 *	@brief	This function is used to deinit all twt data structures.
 * 	@Return none
 * 	@Param	device structure
 */
void nt_twt_teardown_done(devh_t *dev);

/**
 *	@func	nt_twt_teardown_send
 *	@brief	This function is used to create and send twt teardown frame.
 * 	@Return NT_OK -> send successfully , NT_ETXFAIL -> Transmission failed
 * 	@Param	device structure
 */
nt_status_t nt_twt_teardown_send(devh_t *dev);

/**
 *	@func   nt_twt_send_teardown_frame
 *	@brief	This function is used to create and send twt teardown frame.
 * 	@Return NT_OK -> send successfully , NT_ETXFAIL -> Transmission failed
 * 	@Param	device structure
 */
nt_status_t nt_twt_send_teardown_frame(devh_t *dev);

/**
 *	@Func 	:	nt_twt_add_broadcast_parameter_element
 *	@Brief 	: 	This api create and add twt broadcast element
 *	@Param	:	frm - pointer to frame buffer
 *	@Param	:	len - len of twt IE
 *	@Return pointer to the frame after adding twt broadcast element
 */
uint8_t *nt_twt_add_broadcast_parameter_element(devh_t *dev, uint8_t *frm, int8_t len);

/* Name : nt_twt_process_data_available_interrupt
 * Routine description  : process data path tx data available interrupt
 * Arguments            : None
 * Return value         : None
 *
 */
void nt_twt_process_data_available_interrupt(devh_t *dev);

/* Name : nt_twt_display_twt_stats
 * Routine description  : To check twt stats on Neutrino
 * Arguments            : dev - Pointer to device structure
 * Return value         : None
 *
 */
void nt_twt_display_twt_stats(devh_t *dev);

#ifdef SUPPORT_TWT_STA

/* Name : nt_twt_display_min_stats
 * Routine description  : display minimum needed twt stats
 * Arguments            : dev - Pointer to device structure
 * Return value         : None
 *
 */
void nt_twt_display_min_stats(devh_t *dev);
#endif

uint16_t nt_twt_get_alignment(void);

void nt_twt_set_alignment(uint8_t flag, uint16_t alignment);

#if (!defined(SUPPORT_RING_IF) && !defined(SUPPORT_RING_IF_ONLY))
#ifdef SUPPORT_TWT_STA
nt_status_t nt_twt_setup_cmd_hdl(devh_t *dev, wlan_twt_setup_cmd_t *details);
nt_status_t nt_twt_status_cmd_hdl(devh_t *dev, wlan_twt_status_cmd_t *details);
nt_status_t nt_twt_teardown_cmd_hdl(devh_t *dev, wlan_twt_teardown_cmd_t *cmd);
void nt_twt_setup_evt(devh_t *dev, wlan_twt_evt_status_t status);
void nt_twt_status_evt(devh_t *dev, wlan_twt_evt_status_t status);
void nt_twt_teardown_evt(devh_t *dev, wlan_twt_evt_status_t status);
void nt_send_twt_event_with_lock(devh_t *dev, event_t event_id, wlan_twt_evt_status_t status);
void nt_release_twt_sleep_lock(void);
#ifdef WAR_TWT_NT_FERM_SAP
NT_BOOL nt_twt_sap_war_required(devh_t *dev);
#endif

/* Function will return the sleep mode that is forced*/
uint32_t nt_twt_get_forced_sleep_mode();
#endif
#endif
void nt_twt_process_eosp(devh_t *dev);
void nt_twt_log_sp_start_time(void *pPmStruct, uint32_t start_time);
void nt_twt_log_sp_end_time(void *pPmStruct, uint32_t end_time);

#ifdef FEATURE_TX_COMPLETE
void nt_twt_tx_completion_handler(void *params, e_tx_compl_status_dp_msg status);
#endif /* FEATURE_TX_COMPLETE */

/* Name : nt_twt_teardown_frame_cmpl_hdlr
 * Routine description  : Pending wmi commands handler function
 *                        post twt teardown completion
 * Arguments            : None
 * Return value         : None
 */
void nt_twt_teardown_frame_cmpl_hdlr(void);
#endif /* NT_FN_TWT */

#endif /* CORE_WIFI_SME_INC_NT_TWT_POWER_SAVE_H_ */
