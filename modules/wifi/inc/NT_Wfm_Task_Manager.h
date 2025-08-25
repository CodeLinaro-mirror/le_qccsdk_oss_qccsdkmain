/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef WFM_INC_NT_WFM_TASK_MANAGER_H_
#define WFM_INC_NT_WFM_TASK_MANAGER_H_
#include "wifi_cmn.h"
#include "nt_wlan_task_manager.h"
#include "nt_osal.h"
#include "nt_wfm_wmi_interface.h"
#include "fwconfig_cmn.h"
#include "nt_flags.h"

#ifdef NT_FN_FTM
#include "nt_ftm.h"
#endif  // NT_FN_FTM

/*Macro declaration begin**/
#define STA         0
#define AP          1
#define DEVICE_MODE STA /* Device Mode : Station = 0 and AP = 1*/

/*WIFI DEF START*/
#define AP_SSID      "XERUS"
#define AP_SSID_LEN  32
#define AP_BSSID     "NTW_AP"
#define AP_BSSID_LEN 0x6

#define ST_MAC_ADDR     "NTW_ST"
#define ST_MAC_ADDR_LEN 0x6

/*CLI DEF*/
#define CLI_RX_MESSAGE_QUEUE_SIZE 50
#define CLI_TX_MESSAGE_QUEUE_SIZE 10

#ifdef NT_HOSTLESS_SDK
#define _nt_register_app_event(event_id) (nt_event_register(event_id))
#endif
#ifdef NT_HOSTED_SDK
#define _nt_register_hosted_app_event(event_id) (nt_event_register(event_id));
#endif

extern qurt_pipe_t wfm_cli_messge_id;
extern qurt_pipe_t cli_response_messge_id;

/*Tasks*/
void NT_Wfm_Station_task(void *);
void NT_Wfm_Ap_Task(void *);
void nt_wifi_manager(void *);

/* Create Tasks Functions*/
BaseType_t NT_Create_Wfm_Station_Task();
BaseType_t NT_Wfm_Create_Ap_Task();

/*
 ************ Caution: Please read **************************
 * This enum should be in sync with the nt_wfmlib_dispatcher
 * defined in NT_Wfm_Task_Manager.c
 * If there is any mismatch wfm dispatcher will fail.
 * Please be careful when you define compilation flags
 ************************************************************
 */
typedef enum {
    wifi_on = 0,
    wifi_set_mode,
    wifi_connect_ap,
    wifi_config_ap,
    wifi_start_ap,
    wifi_set_power_sleep_mode,  // Setting the sleep mode
    wifi_disconnect,            // wifi_disconnect
    wifi_set_wep_key,
    wifi_set_def_wep_key_idx,
#ifdef NT_FN_PRODUCTION_STATS
    wifi_get_80211_stats,
#endif
    wifi_off,
    wifi_reset,
    wifi_target_reset,
#ifdef NT_EN_WPA_OUI
    wifi_set_wpaie,
#endif
#ifdef NT_FN_RMF
    wifi_rmf_enable,
#endif  // NT_FN_RMF
#ifdef NT_FN_AMPDU
    wifi_ba_add,  // add BA session
    wifi_ba_del,  // delete BA session
#endif            // NT_FN_AMPDU
#if (defined NT_FN_WUR_STA) || (defined NT_FN_WUR_AP)
    wifi_wur_inf,
#endif
    wifi_set_config, /**< Generic command id used for configuring various settings of wifi */
#if (defined NT_FN_WUR_STA) || (defined NT_FN_WUR_AP)
    wifi_get_wur_id,
#endif
    wifi_set_idle_timer,
    wifi_set_force_dtim,
#if (defined NT_FN_WUR_STA) || (defined NT_FN_WUR_AP)
    wifi_set_vendor_id,
#endif
    wifi_get_config,
    wifi_set_channel,
    wifi_NTEnable,  // Enable or Disable IMPS or BMPS.
#ifdef NT_FN_RA
    wifi_set_ra_config,
#endif  // NT_FN_RA

#ifdef NT_FN_FTM
    wifi_ftm_send_req,
    wifi_location_configure,
#endif  // NT_FN_FTM

    wifi_wur_stats,  // To display the wur stats.
#if (defined NT_FN_WUR_STA) || (defined NT_FN_WUR_AP)
    wifi_wur_config,   // To set wur enable or disable.
#endif                 // NT_FN_WUR_STA || NT_FN_WUR_AP
    wifi_send_bc_frm,  // To send single Broadcast frame
#if ((defined NT_FN_AP_HAL_DPH_DEBUG_STATS) || (defined NT_FN_STA_HAL_DPH_DEBUG_STATS) || \
     (defined NT_FN_AP_HAL_DPH_PRODUCTION_STATS) || (defined NT_FN_STA_HAL_DPH_PRODUCTION_STATS))
    wifi_dpm_hal_stats,
#endif  //((defined NT_FN_AP_HAL_DPH_DEBUG_STATS)||(defined NT_FN_STA_HAL_DPH_DEBUG_STATS)||(defined
        //NT_FN_AP_HAL_DPH_PRODUCTION_STATS)||(defined NT_FN_STA_HAL_DPH_PRODUCTION_STATS))
        // wifi_prot_enable,
#ifdef NT_FN_ROAMING
    wifi_set_bg_scan,    // start bg scan
    wifi_start_fg_scan,  // start fg scan
    wifi_set_probed_ssid,
#endif                 // NT_FN_ROAMING
    wifi_enable_omps,  // to goto omps mode
#ifdef NT_FN_WMM_PS_AP
    wifi_uapsd_ap_enable, /**< id to enable or disable uapsd on AP side */
#endif                    // NT_FN_WMM_PS_AP
#ifdef NT_FN_WMM_PS_STA
    wifi_uapsd_sta_settings,   /**< id for setting uapsd related params on sta side */
    wifi_uapsd_trigger_enable, /**< id for starting or stopping uapsd trigger timer*/
    wifi_pm_timers_policy,     /**id for power manager related timers policy setting*/
    wifi_pspoll_threshold,     /**id for pspoll threshold setting*/
#endif                         // NT_FN_WMM_PS_STA

#ifdef NT_FN_WNM_POWERSAVE_MODE
    wifi_wnm_config,             ///<	to set wnm enable/disable
    wifi_set_bss_idle_time,      ///<	to set max bss idle time
    wifi_set_sleep_time,         ///<	to set sleep time at STA side
    wifi_wnm_dtim_sleep_config,  ///
#if defined(NT_FN_PRODUCTION_STATS) || defined(NT_FN_DEBUG_STATS)
    wifi_wnm_stats,  ///<	wnm stats
#endif               // NT_FN_PRODUCTION_STATS || NT_FN_DEBUG_STATS
#endif               /*  NT_FN_WNM_POWERSAVE_MODE */
#ifdef NT_FN_PRODUCTION_STATS
    wifi_get_wlan_prod_stats, /**id to get wlan production stats*/
#endif                        // NT_FN_PRODUCTION_STATS
#ifdef NT_FN_WPS
    wps_set_config,
    cred_wps,
    wps_start,
#endif  // NT_FN_WPS
#ifdef NT_FN_XPA
    en_xpa,
#endif  // NT_FN_XPA
#ifdef NT_FN_STA_ADDBA_SUPPORT
    en_auto_ba,
#endif  // NT_FN_STA_ADDBA_SUPPORT
#ifdef NT_FN_TWT
    wifi_twt_resp_type,
    wifi_twt_enable,
    wifi_Set_Configure_TWT,
    wifi_set_twt_alignment,
    wifi_twt_dtim_sleep_config,
#if defined(NT_FN_PRODUCTION_STATS) || defined(NT_FN_DEBUG_STATS)
    wifi_twt_stats,
#endif  // NT_FN_PRODUCTION_STATS || NT_FN_DEBUG_STATS
#endif  // NT_FN_TWT
    wifi_set_rate,
    wifi_cfg_rate_idx,
#ifdef UNIT_TEST_SUPPORT
    wifi_unit_test_cmd,
#endif
    wifi_invalid_cmd = 255,
} cli_command_id;

/**
 * These are the values used in the set_configure cli command as first parameter
 * for particular configuring feature
 */
typedef enum {
    wur_conf_id = 0,
    wmm_conf_id,         /**< wmm feature id for generic command */
    ht_conf_id,          /**< ht_capability id for generic command */
    beacon_threshold_id, /**< id for generic_configure command for changing beacon miss threashold*/
#ifdef NT_FN_WNM_POWERSAVE_MODE
    wnm_conf_bss_id, /**wnm bss idle time**/
    wnm_conf_id,     /**wnm configuration id for sleep interval**/
#endif               // NT_FN_WNM_POWERSAVE_MODE
} set_config_commands;

#define MAX_IND 0xa

typedef long (*fTable_t)();

typedef struct cli_rx_message_s {
    cli_command_id msg_id;
    cli_command_id prv_msg_id;
    WIFIReturnCode_t resp_id;
    resp_function result_function;
    void *vo_data_resp;
    void *vo_data_query;
} cli_command_message_t;

extern qurt_pipe_t cli_messge_id;

void nt_cli_test_timer_callback();

BaseType_t nt_create_cli_response_message_queue(void);
/**
 * brief  : creates task and queue related to the wifi manager
 * params : none
 * return : nt_pass on successful creation of task and queue, nt_fail on failure to do so.
 */
BaseType_t nt_create_wifi_manager_task(void);

// internal handlers
WIFIReturnCode_t WIFI_AddBA(void *msg);
WIFIReturnCode_t WIFI_DelBA(void *msg);
/**
 * @brief enable or disable uapsd on ap side
 * @param msg : void type pointer used to receive WiFiNetworkConfiguration
 *               structure type information.
 * @return eWiFiSuccess if command is excuted successfully, otherwise eWiFiFailure
 */
WIFIReturnCode_t WIFI_uapsd_ap_enable(void *msg);
/**
 * @brief sets sta side related settings
 * @param msg : void type pointer used to receive WiFiNetworkConfiguration
 *               structure type information.
 * @return eWiFiSuccess if command is excuted successfully, otherwise eWiFiFailure
 */
#ifdef NT_FN_WMM_PS_STA
WIFIReturnCode_t WIFI_uapsd_sta_settings(void *msg);
#endif
/**
 * @brief enable or disable uapsd trigger timer
 * @param msg : void type pointer used to receive WiFiNetworkConfiguration
 *               structure type information.
 * @return eWiFiSuccess if command is excuted successfully, otherwise eWiFiFailure
 */
#ifdef NT_FN_WMM_PS_STA
WIFIReturnCode_t WIFI_uapsd_trigger_settings(void *msg);
#endif  // NT_FN_WMM_PS_STA

/**
 * @brief sets the timeout value for power manager timers
 * @param msg : void type pointer used to receive WiFiNetworkConfiguration
 *               structure type information.
 * @return eWiFiSuccess if command is excuted successfully, otherwise eWiFiFailure
 */
WIFIReturnCode_t WIFI_pm_timers_policy(void *msg);

/**
 * @brief Sets the threshold value
 * @param msg : void type pointer used to receive WiFiNetworkConfiguration
 *               structure type information.
 * @return eWiFiSuccess if command is excuted successfully, otherwise eWiFiFailure
 */
#ifdef NT_FN_WMM_PS_STA
WIFIReturnCode_t WIFI_set_pspoll_threshold(void *msg);
#endif  // NT_FN_WMM_PS_STA

/**
 * @brief passes the received production stats query  to wlan task
 * @params none
 *
 * @return eWiFiSuccess if command is excuted successfully, otherwise eWiFiFailure
 */
WIFIReturnCode_t WIFI_get_wlan_prod_stats(void *msg);
#ifdef NT_FN_TWT
WIFIReturnCode_t WIFI_Twt_Resp_type(void *msg);
WIFIReturnCode_t WIFI_Twt_Enable(void *msg);
WIFIReturnCode_t WIFI_Set_Configure_TWT(void *msg);
WIFIReturnCode_t WIFI_Set_TWT_Alignment(void *msg);
WIFIReturnCode_t WIFI_Set_TWT_sleep_dtim_config(void *msg);
#endif
WIFIReturnCode_t WIFI_Set_wnm_sleep_config(void *msg);
#ifdef SUPPORT_RING_IF
void WIFI_initiaize_wlan_cmd_var(NT_BOOL is_ringif_cmd, wmi_msg_struct_t *var);
#endif
#endif /* WFM_INC_NT_WFM_TASK_MANAGER_H_ */
