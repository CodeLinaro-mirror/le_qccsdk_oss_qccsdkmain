/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#ifndef _WFM_WMI_INTERFACE_
#define _WFM_WMI_INTERFACE_

#include "nt_event_reg.h"
#include "wifi_cmn.h"
#include "fwconfig_cmn.h"
#include "nt_flags.h"

//#include "wmi.h"
//#include "nt_wlan.h"
//#include "wlan_wmi.h"
//#include "iot_wifi.h"

#define MAX_INTERFCE 2

// typedef struct{
//	uint16_t type;
//	uint16_t id;
//	void* data;
//	uint8_t evt_timer_flag;
//}WurInfoEvnt_t;

typedef struct _CMD_TRANSLATED_PROCESS_ {
    struct {
        void *vo_data;
        uint32_t vo_data_len;
        uint32_t timeout_id;
        WIFIReturnCode_t return_status;
        resp_function result_function;
        event event_notify;
        event_t id;
        TimerHandle_t hnd;
#if (defined NT_FN_WUR_AP) || (defined NT_FN_WUR_STA)
        uint64_t wur_data;  ///< send wur data through queue
        uint16_t wur_id;    //	send wur id through queue
        uint8_t wur_flag;   //	wur flag for notification
#endif
#ifdef NT_FN_WMM_PS_STA
        uint8_t data_ac : 3;  // AC in which buffered data available to send
#endif                        // NT_FN_WMM_PS_STA
#if defined(SUPPORT_RING_IF) || defined(CONFIG_WMI_EVENT)
        uint8_t netif_id;
#endif
#if defined(SUPPORT_RING_IF) || defined(SUPPORT_5GHZ)
        NT_BOOL is_ringif_cmd;  // Set to 1 when a command is coming from ring interface, and 0 if it is a uart command
        NT_BOOL ringif_in_use;  // Set to 1 if ring interface is being used, else set to 0
#endif
    } msg_struct;

    WMI_COMMAND_ID trans_wmi_message_id;
    uint32_t prot_flg;

} wmi_msg_struct_t;

extern wmi_msg_struct_t *Cmd_Translation_wlan;

// wifi_disconnect
typedef struct WLAN_WMI_DISCONN_s {
    int32_t sta_id;
} WLAN_WMI_DISCONN_t;

// set_rate
typedef struct WMI_SET_RATE_CFG_s {
    uint8_t ra_ON;
    uint8_t set_ra_staid;
    uint8_t set_rate_config_p_rate;
    uint8_t set_rate_config_s_rate;
    uint8_t set_rate_config_t_rate;
} WFM_SET_RATE_CFG_t;

// AUTO_BA
#ifdef NT_FN_STA_ADDBA_SUPPORT
typedef enum auto_ba_ac_type {
    AUTO_BA_BE_ENABLED = 0,
    AUTO_BA_BK_ENABLED = 1,
    AUTO_BA_BE_BK_ENABLED = 2,
} WMI_BA_AC_TYPE;

typedef enum auto_ba_en {
    AUTO_BA_CONN_DISABLED = 0,
    AUTO_BA_CONN_ENABLED = 1,
    AUTO_BA_TRAFFIC_ENABLED = 2,
} WMI_BA;

typedef struct WMI_AUTO_BA_CMD_s {
    uint8_t auto_ba_enable;
    uint8_t auto_ba_ac_type;
} WMI_AUTO_BA_CMD_t;
#endif  // NT_FN_STA_ADDBA_SUPPORT

/**
 * WFM_TWT_CONFIG_CMD
 */
typedef struct {
    uint16_t resp_type;              ///< response type of twt
    uint8_t twt_type;                // individual/broadcast
    uint64_t twt_wake_interval;      // span of time between first wake up and next wake up
    uint32_t twt_min_wake_duration;  // span of time between point of wake up and point of sleep
    uint8_t twt_wakeup_type;         // implicit/explicit
    uint8_t twt_flow_type;           // announced /unannounced
    uint16_t twt_alignment;
    uint8_t twt_dtim_enable_disable_auto;
} WFM_TWT_CONFIG_CMD;

typedef struct {
    uint8_t bg_scan_status; /*enabled or disabled*/
    int32_t bg_period;      /* frequency in seconds */
    uint8_t full_scan_freq; /* full scan frequency */
    uint8_t probe_type;
    uint8_t trigger_type;     /* rssi or rate or mixed or periodic */
    uint8_t rssi_min_thresh;  /* minimum rssi threshold */
    uint8_t rssi_max_thresh;  /* maximum rssi threshold */
    uint16_t rate_min_thresh; /* minimum rate threshold */
    uint16_t rate_max_thresh; /* maximum rate threshold */
    uint8_t ssid;
    uint8_t num_ch;
} WFM_BG_SCAN_CMD;

#define XPA_ENABLE  2
#define XPA_DISABLE 1
#endif
