/* 
Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#ifndef __WLAN_DRV_H__
#define __WLAN_DRV_H__

#include <stdio.h>
#include "printfext.h"

#include "qapi_wlan.h"
#include "wmi.h"
#include "nt_wfm_wmi_interface.h"
#include "wlan_dev.h"

#include "qurt_internal.h"
#include "qurt_mutex.h"
#include "nt_osal.h"
#include "qapi_wlan_p2p.h"
#include "wlan_qapi_helper.h"

#define QAPI_EVENT_LARGE_PAYLOAD_LENGTH_MAX 1000
#define QAPI_EVENT_LARGE_PAYLOAD_BUF_NUM 3
#define QAPI_EVENT_SMALL_PAYLOAD_LENGTH_MAX 256
#define QAPI_EVENT_SMALL_PAYLOAD_BUF_NUM 5

#ifndef DEF_AP_COUNTRY_CODE
#define DEF_AP_COUNTRY_CODE "US "
#endif

typedef enum { EVT_LARGE_PAYLOAD, EVT_SMALL_PAYLOAD, EVT_PAYLOAD_MAX } EVT_PAYLOAD_TYPE;

typedef struct {
    uint8_t *buf;
    uint16_t buf_length;
    int16_t buf_num;
    int16_t buf_used;
    uint16_t buf_write_pointer;
} wlan_evt_payload_t;

typedef struct wlan_qapi_cxt_s {
    qbool_t wlanEnabled;
    qapi_WLAN_Callback_t qapi_event_handler;
    void *event_application_Context;
    qurt_mutex_t wlan_qapi_cxt_mutex; /* protect wlan_qapi_cxt_t safe in multi-thread */
    qurt_signal_t wlan_cmd_done;      /* will be ready if wlan cmd is done */
    qurt_mutex_t wlan_qapi_block_mutex;
    uint32_t wlan_enable_block_mode : 1;
    uint32_t wlan_disable_block_mode : 1;
    uint32_t wlan_if_add_block_mode : 1;
    uint32_t wlan_scan_start_block_mode : 1;
    uint32_t wlan_connect_block_mode : 1;
    uint32_t wlan_disconnect_block_mode : 1;
    uint32_t wlan_get_stat_block_mode : 1;
    uint32_t wlan_set_param_block_mode : 1;
    uint32_t wlan_get_regulatory_block_mode : 1;
#ifdef CONFIG_WPS
    uint32_t wlan_scan_stop_block_mode : 1;
    uint32_t wlan_start_wps_block_mode : 1;
#endif
    uint32_t wlan_roaming_started : 1;
    uint32_t wlan_set_rate_block_mode : 1;
    uint32_t wlan_get_rate_block_mode : 1;
    uint32_t wlan_send_raw_block_mode : 1;
    uint32_t wlan_set_mgmt_filter_block_mode : 1;
    uint32_t wlan_get_tx_power_block_mode : 1;
    uint32_t wlan_get_bmps_stats_block_mode : 1;
    qapi_Status_t wlan_qapi_error;
    dev_common_t *dev_common;
    wlan_evt_payload_t event_payload_buf[EVT_PAYLOAD_MAX];
    WMI_CONNECT_CMD connect_cmd;
    WMI_START_SCAN_CMD scan_cmd;
    WMI_SET_PASSPHRASE_CMD passphrase_cmd;
#ifdef CONFIG_WPS
    WMI_WPS_START_CMD wps_process_cmd;
#endif
    WLAN_WMI_DISCONN_t discon_cmd;
    WMI_SET_PDEV_PARAM_CMD dev_param_cmd;
    qapi_WLAN_Join_Comp_Evt_t connect_result;
    qapi_WLAN_Reg_Evt_t reg_result;
    uint8_t param_id;
    qbool_t connect_in_progress;
    qbool_t disconnect_in_progress;
    qbool_t scan_in_progress;
#ifdef CONFIG_WPS
    qbool_t stop_scan_in_progress;
    qbool_t wps_in_progress;
    qbool_t wps_stage;
#endif
    qbool_t wait_scan_comp_evt;
    qbool_t connected;
    uint8_t *pScanOut; /* callers buffer to hold results. */
    uint16_t pScanOutSize;
    uint16_t scanBssMaxCount;
    uint8_t conc_mode;
    uint8_t opmode;
    uint8_t rssi;
    uint8_t network_id;
    uint32_t roaming_time_out;
    TimerHandle_t roaming_timer;
    char country_code[3];
    uint8_t frame_queued_flag;
    qapi_WLAN_Set_Rate_Params_t rate_param;
    qapi_WLAN_Raw_Send_Params_t raw_pkt_frame;
    WMI_MGMT_FRAME_FILTER mgmt_filter;
    WMI_SET_APPIE_CMD appie_cmd;
    WMI_SET_TX_POWER_CMD tx_power;
    qapi_WLAN_Get_Power_Evt_t get_tx_power_result;
#ifdef CONFIG_WPS
    WMI_WPS_START_CMD wps_param;
#endif
    WMI_BMPS_GET_STATS cmd_bmps_stats;
#ifdef CONFIG_ENABLE_P2P_MODE
    qapi_WLAN_P2P_Node_List_Params_t get_p2p_nodelist;
    qapi_WLAN_P2P_Event_Cb_Info_t p2p_Event_Cb_Info;
    uint32_t wlan_get_nodelist_block_mode : 1;
    uint32_t wlan_p2p_block_mode : 1;
    qbool_t p2p_connect_in_progress;
    WMI_P2P_FW_INVITE_REQ_RSP_CMD p2p_inv_rsp;
    WMI_P2P_SET_PROFILE_CMD p2p_set_profile;
    WMI_P2P_GRP_INIT_CMD p2p_grp_init;
    WMI_P2P_FIND_CMD p2p_find;
    WMI_P2P_FW_CONNECT_CMD p2p_conn;
    WMI_P2P_FW_PROV_DISC_REQ_CMD p2p_prov_disc_req;
    WMI_P2P_FW_SET_CONFIG_CMD p2p_fw_set_conf;
    WMI_P2P_SET_CMD p2p_set;
    WMI_NOA_INFO p2p_noa_info;
    WMI_OPPPS_INFO p2p_oppps_info;
    WMI_P2P_INVITE_CMD p2p_invite;
    WLAN_P2P_JOIN_STRUCT p2p_join;
#endif
} wlan_qapi_cxt_t;

extern wlan_qapi_cxt_t *gp_wlan_qapi_cxt;

#define set_wlan_qapi_error(x) (gp_wlan_qapi_cxt->wlan_qapi_error = (x))
#define get_wlan_qapi_error() (gp_wlan_qapi_cxt->wlan_qapi_error)
#define clr_wlan_qapi_error() set_wlan_qapi_error(QAPI_OK)

#define CONFIG_WLAN_QAPI_BLOCK_ENABLE 1

#if CONFIG_WLAN_QAPI_BLOCK_ENABLE
#define WLAN_QAPI_LOCK() qurt_mutex_lock(&gp_wlan_qapi_cxt->wlan_qapi_block_mutex)
#define WLAN_QAPI_UNLOCK() qurt_mutex_unlock(&gp_wlan_qapi_cxt->wlan_qapi_block_mutex)
#else
#define WLAN_QAPI_LOCK()
#define WLAN_QAPI_UNLOCK()
#endif

qapi_Status_t wlan_drv_set_cb(qapi_WLAN_Callback_t callback, void *application_Context);
qapi_Status_t wlan_drv_roaming_start(void);
qapi_Status_t wlan_drv_roaming_stop(void);

#endif //__WLAN_DRV_H__
