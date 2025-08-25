/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*=========================================================================
 * @file wifi_lib.c
 * @brief contain definitation of command and event handler from APPs
 *==========================================================================*/

/*-------------------------------------------------------------------------
 * Include Files
 * ----------------------------------------------------------------------*/

#include "nt_osal.h"
#include "wifi_fw_mgmt_api.h"
#include <nt_wlan_task_manager.h> /* NT_MAX_QUEUE_WAIT_TIME */
#include "nt_logger_api.h"
#include "wmi.h"
#include "ctrl_ring_hdlr.h"
#include "com_api.h" /* CM_CONNECT_WITHOUT_SCAN */
#include "nt_wfm_wmi_interface.h"
#include "ring_ctx_holder.h"
#include "discovery_api.h"
#include "wlan_dev.h"
#include "wlan_power.h"
#include "wifi_evt_hndlr.h"
#include "netif.h"
#include "network_al.h"
#include "NT_Wfm_Task_Manager.h"
#include "wlan_encode_ie_api.h"
#include "nt_imps.h"
#include "nt_timer.h"
#include "timer.h"

#ifdef SUPPORT_TWT_STA
#include "nt_twt.h"
#endif
#include "chop_api.h"
#include "halphy_dbgmem.h"
#ifdef SUPPORT_COEX
#include "coex_wlan.h"
#include "coex_utils.h"
extern BTCOEX_WLAN_INFO_STRUCT *gpBtCoexWlanInfoDev;
#endif
#ifdef PHY_POWER_SWITCH
#include "hal_int_modules.h"
#endif

#ifdef SUPPORT_RING_IF
/*-------------------------------------------------------------------------
 * Global Data Definitions
 * ----------------------------------------------------------------------*/
wmi_msg_struct_t g_Cmd_Translation_wifi_hndl;
command_struct g_command_struct;
uint8_t get_wifi_phymode_from_wmi_phymode(WLAN_PHY_MODE phymode);
/*-----------------------------------------------------------------------
 * Type Declerations
 *-----------------------------------------------------------------------*/
typedef WIFIReturnCode_t (*func_table)(void *, uint16_t);

/*-------------------------------------------------------------------------
 * Static Variable Definitions
 * ----------------------------------------------------------------------*/
static WMI_CONNECT_CMD connect_ap;
static uint8_t network_id = 0xFF;

/*-------------------------------------------------------------------------
 * Externalised variable  Definitions
 * ----------------------------------------------------------------------*/
extern dev_common_t *gpDevCommon;
extern PM_STRUCT pPmStruct;
extern ppm_common_t g_ppm_common_struct;
extern uint8_t phydbg_data[PHYDBG_BUFFER_SIZE];

/*------------------------------------------------------------------------
* This table must be in sync with the event_t tables.
* If there is any mismatch, the event dispatcher  will fail
-------------------------------------------------------------------------*/

static const evt_fn_table evt_dispatcher_fn_table[] = {
    NULL,
    NULL,
    NULL,
    NULL,
    wlan_disconnect_event,
    wlan_join_comp_event,
    NULL, /* UTF event is sent by utfCmdReplyEvent after command processing */
    wlan_mode_event,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    wlan_join_comp_event,
    NULL,
    wlan_enable_event,
    wlan_disable_event,
    wlan_if_add_event,
    wlan_scan_comp_event,
    wlan_scan_fail_event,
    wlan_set_param_event,
    wlan_scan_start_event,
    wlan_scan_stop_event,
    wlan_scan_stop_event,
    wlan_unit_test_event,
    wlan_twt_setup_event,
    wlan_twt_teardown_event,
    wlan_twt_status_event,
    wlan_phydbgdump_event,
    wlan_update_bi_event,
    wlan_set_reset_wakelock_event,
    wlan_periodic_tsf_sync_start_event,
    wlan_periodic_tsf_sync_event,
    wlan_f2a_on_wakeup_config_event,
    wlan_update_bmtt_event,
    wlan_coex_event,
    wlan_periodic_traffic_setup_event,
    wlan_periodic_traffic_status_event,
    wlan_periodic_traffic_teardown_event,
    wlan_clk_latency_event,
    NULL};

/*-------------------------------------------------------------------------
 * Externalized Function Definitions
 * ----------------------------------------------------------------------*/

extern qurt_pipe_t msg_wfm_wmi_id;
extern app_mode_id_t nt_get_app_mode(void);

void wmi_response_handler(void *msg)
{
    if (!msg) {
        NT_LOG_WFM_ERR("msg is NULL", 0, 0, 0);
        return;
    }

    void *data = ((wmi_msg_struct_t *)msg)->msg_struct.vo_data;
    event_t event_id = (event_t)(((wmi_msg_struct_t *)msg)->msg_struct.id);
    if (event_id >= sizeof(evt_dispatcher_fn_table) / sizeof(evt_dispatcher_fn_table[0])) {
        NT_LOG_WFM_ERR("Invalid event id", 0, 0, 0);
        return;
    }
    if (!(event_id == wifi_scan_comp_event_id || event_id == disconnect_event_id)) {
        g_command_struct.in_use = FALSE;
    }
    if (NULL != evt_dispatcher_fn_table[event_id]) {
        (*evt_dispatcher_fn_table[event_id])(data);
    }
}

/*-------------------------------------------------------------------------
 * Static Function Declarations and Definitions
 * ----------------------------------------------------------------------*/

#if 1
/* TBD: Remove after CLI interface is compiled out */
extern wmi_msg_struct_t *Cmd_Translation_wlan;
wmi_msg_struct_t Cmd_Translation_dummy;
static void wifi_dummy_event_notify(WIFIReturnCode_t return_type, event_t id, void *data)
{
    (void)return_type;
    (void)id;
    (void)data;
    return;
}
static void wifi_dummy_result(void *msg)
{
    (void)msg;
}
/* Dummy initialization for CLI interface to avoid crashes*/
/* TBD: remove this after CLI interface is compiled out */
void wifi_dummy_cli_init()
{
    Cmd_Translation_dummy.msg_struct.return_status = eWiFiNotSupported;
    memset(&Cmd_Translation_dummy, 0x0, sizeof(wmi_msg_struct_t));
    Cmd_Translation_dummy.msg_struct.event_notify = &wifi_dummy_event_notify;
    Cmd_Translation_dummy.msg_struct.result_function = &wifi_dummy_result;
    Cmd_Translation_wlan = &Cmd_Translation_dummy;
}
#endif

void wifi_svc_init(void)
{
    memset(&g_Cmd_Translation_wifi_hndl, 0x0, sizeof(wmi_msg_struct_t));
    memset(&connect_ap, 0x0, sizeof(WMI_CONNECT_CMD));
    g_Cmd_Translation_wifi_hndl.msg_struct.event_notify = &wlan_wmi_event_notification;
    g_Cmd_Translation_wifi_hndl.msg_struct.result_function = &wmi_response_handler;
    g_command_struct.in_use = FALSE;
    uint16_t ring_buffer_size = ringif_get_ctrl_buf_size();
    g_command_struct.command_buffer = nt_osal_calloc(1, ring_buffer_size);
    g_Cmd_Translation_wifi_hndl.msg_struct.ringif_in_use = TRUE;
    wifi_dummy_cli_init();
}

/**
 *@brief Called as a result of WLAN_ENABLE_CMD from APPS
 *
 *This function is called as if the APPS sends WLAN_ENABLE_CMD command,
 *it converts the structrure from APPS to WMI structure and triggers,
 *WMI_WLAN_ON_CMDID WMI command.
 *
 * @param msg: it does not need any data.
 * @param len: length of message buffer.
 * @return returns one of WIFIReturnCode_t enums
 *   eWiFiNotSupported in case there is some fatal error.
 *   eWiFiSuccess in case of Success
 *   eWiFiTimeout if timeout happens, while processing the WMI command
 *   eWiFiFailure in case of Failure
 */

WIFIReturnCode_t wlan_enable(__unused void *msg, __unused uint16_t len)
{
    g_Cmd_Translation_wifi_hndl.msg_struct.vo_data = NULL;

    g_Cmd_Translation_wifi_hndl.msg_struct.return_status = eWiFiNotSupported;
    g_Cmd_Translation_wifi_hndl.trans_wmi_message_id = WMI_WLAN_ON_CMDID;
    g_Cmd_Translation_wifi_hndl.prot_flg = WMI_WLAN_ON_CMDID;

    NT_LOG_WFM_ERR("wlan_enable cmd", 0, 0, 0);

    if (NT_QUEUE_FAIL ==
        nt_osal_queue_send(msg_wfm_wmi_id, (void *)&g_Cmd_Translation_wifi_hndl, NT_MAX_QUEUE_WAIT_TIME)) {
        NT_LOG_WFM_ERR("Queue send failed", 0, 0, 0);
    }

    ringif_free_ctrl_buf(msg, len);
    return (g_Cmd_Translation_wifi_hndl.msg_struct.return_status);
}

/**
 *@brief Called as a result of WLAN_SCAN_CMD from APPS
 *
 *This function is called as if the APPS sends WLAN_SCAN_CMD command,
 *it converts the structrure from APPS to WMI structure and triggers,
 *WMI_START_SCAN_CMDID WMI command.
 *
 * @param msg: contains function specific data sent from APPS
 * @param len: length of message  buffer
 * @return returns one of WIFIReturnCode_t enums
 *   eWiFiNotSupported in case there is some fatal error.
 *   eWiFiSuccess in case of Success
 *   eWiFiTimeout if timeout happens, while processing WMI cmd.
 *   eWiFiFailure in case of Failure
 */

WIFIReturnCode_t start_scan(void *msg, uint16_t len)
{
    if (g_command_struct.in_use == TRUE) {
        NT_LOG_WFM_ERR("Command buffer already in use", 0, 0, 0);
        ringif_free_ctrl_buf(msg, len);
        return eWiFiFailure;
    }
#ifdef ENABLE_IMPS_TIMER_ON_BOOTUP
    devh_t *pVdev = gpDevCommon->devp[NT_DEV_STA_ID];
    if ((pVdev->ic_opmode == WHAL_M_STA) && (!DEVICE_CONNECTED(pVdev))) {
        /*reload cnx wait timer when there is a scan cmd because scan will take some time to process(avoid unnecessary
         * entry of IMPS) */
        start_imps_cnx_wait_timer(g_ppm_common_struct.imps_struct_ctx.cnx_wait_time_ms);
    }
#endif /* ENABLE_IMPS_TIMER_ON_BOOTUP */
    g_command_struct.in_use = TRUE;
    memset(g_command_struct.command_buffer, 0x0, CTRL_RING_BUFF_SIZE);
    WMI_START_SCAN_CMD *scan_param = (WMI_START_SCAN_CMD *)(g_command_struct.command_buffer);
    g_Cmd_Translation_wifi_hndl.msg_struct.return_status = eWiFiNotSupported;
    for (uint8_t i = 0; i < ((wlan_scan_cmd_t *)msg)->num_ssid; i++) {
        scan_param->ssid[i].ssid_len = ((wlan_scan_cmd_t *)msg)->ssid[i].ssid_len;
        memcpy(scan_param->ssid[i].ssid, ((wlan_scan_cmd_t *)msg)->ssid[i].ssid,
               ((ScanConfig_t *)msg)->ssid[i].ssid_len);
    }

    switch (((wlan_scan_cmd_t *)msg)->security_mode) {
        case eWiFiSecurityWEP: {
            scan_param->auth_mode = WMI_NONE_AUTH;
            scan_param->crypto_type = WEP_CRYPT;
            break;
        }
        case eWiFiSecurityWPA: {
            scan_param->auth_mode = WMI_WPA_PSK_AUTH;
            scan_param->crypto_type = TKIP_CRYPT;
            scan_param->group_crypto = TKIP_CRYPT;
            break;
        }
        case eWiFiSecurityWPA2: {
            scan_param->auth_mode = WMI_WPA2_PSK_AUTH;
            scan_param->crypto_type = AES_CRYPT;
            scan_param->group_crypto = (AES_CRYPT | TKIP_CRYPT);
            break;
        }
        case eWiFiSecurityMixed: {
            scan_param->auth_mode = (WMI_WPA2_PSK_AUTH | WMI_WPA_PSK_AUTH);
            scan_param->crypto_type = (AES_CRYPT | TKIP_CRYPT);
            scan_param->group_crypto = (AES_CRYPT | TKIP_CRYPT);
            break;
        }
        default: {
            NT_LOG_WFM_ERR("setting auth mode as open \r\n", 0, 0, 0);
            scan_param->auth_mode = WMI_NONE_AUTH;
            scan_param->crypto_type = NONE_CRYPT;
            break;
        }
    }

    /* TODO: need to check channel list as it is declared as uint8_t*/
    if (((wlan_scan_cmd_t *)msg)->num_ssid == 0) {
        scan_param->scan_type |= any_profile;
    } else {
        scan_param->scan_type |= specific_ssid;
    }
    scan_param->probe_type = ((wlan_scan_cmd_t *)msg)->probe_type;
    scan_param->num_channels = ((wlan_scan_cmd_t *)msg)->num_chan;
    if (scan_param->probe_type == active_probe) {
        scan_param->chdwell_active_duration = ((wlan_scan_cmd_t *)msg)->chdwell_duration;
        scan_param->chdwell_passive_duration = 0;
#ifdef ENABLE_IMPS_TIMER_ON_BOOTUP
        uint32_t total_active_duration = scan_param->chdwell_active_duration * scan_param->num_channels;
        if (total_active_duration > g_ppm_common_struct.imps_struct_ctx.cnx_wait_time_ms) {
            stop_imps_cnx_wait_timer();
            NT_LOG_PRINT(WPM, ERR, "configured cnx wait time is less and IMPS abort total_active_duration: %d\n",
                         total_active_duration);
        }
#endif /* ENABLE_IMPS_TIMER_ON_BOOTUP */
    } else if (scan_param->probe_type == passive_probe) {
        scan_param->chdwell_passive_duration = ((wlan_scan_cmd_t *)msg)->chdwell_duration;
        scan_param->chdwell_active_duration = 0;
#ifdef ENABLE_IMPS_TIMER_ON_BOOTUP
        uint32_t total_passive_duration = scan_param->chdwell_passive_duration * scan_param->num_channels;
        if (total_passive_duration > g_ppm_common_struct.imps_struct_ctx.cnx_wait_time_ms) {
            stop_imps_cnx_wait_timer();
            NT_LOG_PRINT(WPM, ERR, "configured cnx wait time is less and IMPS abort total_passive_duration: %d\n",
                         total_passive_duration);
        }
#endif /* ENABLE_IMPS_TIMER_ON_BOOTUP */
    }
    memcpy(scan_param->channel_list, ((wlan_scan_cmd_t *)msg)->chan_list, scan_param->num_channels);
    /* Accept the arguments in the form of channel numbers and convert to index internally.
       otherwise user should give index which is misleading to the user */
    for (int i = 0; i < scan_param->num_channels; i++) {
        scan_param->channel_list[i] =
            dc_freq_to_chindex(gpDevCommon, IEEE_ieee2freq(scan_param->channel_list[i], FALSE));
    }
    scan_param->cnt_prof = ((wlan_scan_cmd_t *)msg)->num_ssid;
    scan_param->scan_only = TRUE;
    scan_param->scan_type = ((wlan_scan_cmd_t *)msg)->ctrl_flag;

    g_Cmd_Translation_wifi_hndl.msg_struct.netif_id = ((wlan_cmd_hdr *)msg)->network_id;
    g_Cmd_Translation_wifi_hndl.trans_wmi_message_id = WMI_START_SCAN_CMDID;
    g_Cmd_Translation_wifi_hndl.msg_struct.vo_data = scan_param;
    g_Cmd_Translation_wifi_hndl.msg_struct.result_function = &wmi_response_handler;

    NT_LOG_WFM_ERR("wlan_scan cmd", 0, 0, 0);

    if (NT_QUEUE_FAIL == nt_osal_queue_send(msg_wfm_wmi_id, (void *)&g_Cmd_Translation_wifi_hndl, portMAX_DELAY)) {
        NT_LOG_WFM_ERR("Queue send failed", 0, 0, 0);
    }

    ringif_free_ctrl_buf(msg, len);

    return (g_Cmd_Translation_wifi_hndl.msg_struct.return_status);
}

/**
 *@brief Called as a result of WLAN_JOIN_CMD from APPS
 *
 *This function is called as if the APPS sends WLAN_JOIN_CMD command,
 *it converts the structrure from APPS to WMI structure and triggers,
 *WMI_CONNECT_CMDID WMI command.
 *
 * @param msg: contains function specific data sent from APPS
 * @param len: length of message  buffer
 * @return returns one of WIFIReturnCode_t enums
 *   eWiFiNotSupported in case there is some fatal error.
 *   eWiFiSuccess in case of Success
 *   eWiFiTimeout if timeout happens, during one of task.
 *   eWiFiFailure in case of Failure
 */

WIFIReturnCode_t join_ap(void *msg, uint16_t len)
{
    g_Cmd_Translation_wifi_hndl.msg_struct.return_status = eWiFiNotSupported;

    if (g_command_struct.in_use == TRUE) {
        NT_LOG_WFM_ERR("Command buffer already in use", 0, 0, 0);
        ringif_free_ctrl_buf(msg, len);
        return eWiFiFailure;
    }

    uint8_t ssid_len = (((wlan_join_cmd_t *)msg)->ssid).ssid_len;

    if ((ssid_len == 0) || (ssid_len > MAX_SSID_LEN)) {
        send_wifi_cnx_event(NULL, INVALID_PROFILE, FALSE);
        return eWiFiFailure;
    }
#ifdef SUPPORT_IMPS_IMPROVEMENTS
    /*stopping the IMPS cnx wait timer and it restarts when there is any cnx failure */
    stop_imps_cnx_wait_timer();
#endif /* SUPPORT_IMPS_IMPROVEMENTS */
    g_command_struct.in_use = TRUE;
    memset(g_command_struct.command_buffer, 0x0, CTRL_RING_BUFF_SIZE);
    WMI_SET_PASSPHRASE_CMD *set_passphrase = (WMI_SET_PASSPHRASE_CMD *)(g_command_struct.command_buffer);
    uint8_t chindex;

    g_Cmd_Translation_wifi_hndl.msg_struct.result_function = &wmi_response_handler;

    if (eWiFiSecurityOpen == ((wlan_join_cmd_t *)msg)->security_mode) {
        connect_ap.dot11AuthMode = OPEN_AUTH;
        connect_ap.authMode = WMI_NONE_AUTH;
        connect_ap.pairwiseCryptoType = NONE_CRYPT;
        memcpy(connect_ap.ssid, (((wlan_join_cmd_t *)msg)->ssid).ssid, (((wlan_join_cmd_t *)msg)->ssid).ssid_len);
        connect_ap.ssidLength = (((wlan_join_cmd_t *)msg)->ssid).ssid_len;
    }

    else if (eWiFiSecurityWEP == ((wlan_join_cmd_t *)msg)->security_mode) {
        connect_ap.dot11AuthMode = OPEN_AUTH;
        connect_ap.authMode = WMI_NONE_AUTH;
        memcpy(connect_ap.ssid, (((wlan_join_cmd_t *)msg)->ssid).ssid, (((wlan_join_cmd_t *)msg)->ssid).ssid_len);
        connect_ap.ssidLength = (((wlan_join_cmd_t *)msg)->ssid).ssid_len;

        connect_ap.pairwiseCryptoType = WEP_CRYPT;
        memcpy(connect_ap.channel_list, ((WIFINetworkParams_t *)msg)->cChannelList, sizeof(connect_ap.channel_list));
    }

    else if ((eWiFiSecurityWPA2 == ((wlan_join_cmd_t *)msg)->security_mode) ||
             (eWiFiSecurityWPA == ((wlan_join_cmd_t *)msg)->security_mode) ||
             (eWiFiSecurityMixed == ((wlan_join_cmd_t *)msg)->security_mode) ||
             (eWiFiSecurityWPA3_transition == ((wlan_join_cmd_t *)msg)->security_mode) ||
             (eWiFiSecurityWPA3 == ((wlan_join_cmd_t *)msg)->security_mode)) {
        uint8_t pass_len = ((wlan_join_cmd_t *)msg)->password_len;
        if ((pass_len == 0) || (pass_len > WIFI_PASSPHRASE_LEN)) {
            g_command_struct.in_use = FALSE;
            send_wifi_cnx_event(NULL, INVALID_PROFILE, FALSE);
            return eWiFiFailure;
        }

        if (eWiFiSecurityWPA == ((wlan_join_cmd_t *)msg)->security_mode) {
            NT_LOG_WFM_INFO("Inside WPA", 0, 0, 0);
            connect_ap.dot11AuthMode = OPEN_AUTH;
            connect_ap.authMode = (WMI_WPA2_PSK_AUTH | WMI_WPA_PSK_AUTH);
            connect_ap.pairwiseCryptoType = (AES_CRYPT | TKIP_CRYPT);
            connect_ap.groupCryptoType = TKIP_CRYPT;
        } else if (eWiFiSecurityWPA2 == ((wlan_join_cmd_t *)msg)->security_mode) {
            NT_LOG_WFM_INFO("Inside WPA2", 0, 0, 0);
            connect_ap.dot11AuthMode = OPEN_AUTH;
            connect_ap.authMode = WMI_WPA2_PSK_AUTH;
            connect_ap.pairwiseCryptoType = AES_CRYPT;
            connect_ap.groupCryptoType = (AES_CRYPT | TKIP_CRYPT);
        } else if (eWiFiSecurityMixed == ((wlan_join_cmd_t *)msg)->security_mode) {
            NT_LOG_WFM_INFO("Inside Mixed", 0, 0, 0);
            connect_ap.dot11AuthMode = OPEN_AUTH;
            connect_ap.authMode = (WMI_WPA2_PSK_AUTH | WMI_WPA_PSK_AUTH);
            connect_ap.pairwiseCryptoType = (AES_CRYPT | TKIP_CRYPT);
            connect_ap.groupCryptoType =
                (AES_CRYPT | TKIP_CRYPT);  // for now group ciphers in mixed mode will be done in TKIP
        }
#ifdef NT_FN_WPA3
        else if (eWiFiSecurityWPA3_transition == ((wlan_join_cmd_t *)msg)->security_mode) {
            NT_LOG_WFM_INFO("Inside  transition", 0, 0, 0);
            connect_ap.dot11AuthMode = (OPEN_AUTH | SAE_AUTH);
            connect_ap.authMode = (WMI_WPA2_PSK_AUTH | WMI_WPA3_SHA256_AUTH);
            connect_ap.pairwiseCryptoType = AES_CRYPT;
            connect_ap.groupCryptoType = AES_CRYPT;
        } else if (eWiFiSecurityWPA3 == ((wlan_join_cmd_t *)msg)->security_mode) {
            NT_LOG_WFM_INFO("Inside Connect_ap eWiFiSecurityWPA3", 0, 0, 0);
            connect_ap.dot11AuthMode = SAE_AUTH;
            connect_ap.authMode = WMI_WPA3_SHA256_AUTH;
            connect_ap.pairwiseCryptoType = AES_CRYPT;
            connect_ap.groupCryptoType = AES_CRYPT;
        }
#endif  // NT_FN_WPA3
        memcpy(connect_ap.ssid, (((wlan_join_cmd_t *)msg)->ssid).ssid, (((wlan_join_cmd_t *)msg)->ssid).ssid_len);
        connect_ap.ssidLength = (((wlan_join_cmd_t *)msg)->ssid).ssid_len;
        connect_ap.pairwiseCryptoLen = ((wlan_join_cmd_t *)msg)->password_len;
        connect_ap.groupCryptoLen = ((wlan_join_cmd_t *)msg)->grp_passwd_len;
        // connect_ap.ctrl_flags |= CM_CONNECT_WITHOUT_SCAN;

        g_Cmd_Translation_wifi_hndl.msg_struct.netif_id = ((wlan_cmd_hdr *)msg)->network_id;
        g_Cmd_Translation_wifi_hndl.msg_struct.return_status = eWiFiNotSupported;
        memcpy(set_passphrase->passphrase, ((wlan_join_cmd_t *)msg)->password, ((wlan_join_cmd_t *)msg)->password_len);
        set_passphrase->passphrase_len = ((wlan_join_cmd_t *)msg)->password_len;
        memcpy(set_passphrase->ssid, (((wlan_join_cmd_t *)msg)->ssid).ssid, (((wlan_join_cmd_t *)msg)->ssid).ssid_len);
        set_passphrase->ssid_len = (((wlan_join_cmd_t *)msg)->ssid).ssid_len;
        g_Cmd_Translation_wifi_hndl.msg_struct.vo_data = set_passphrase;
        g_Cmd_Translation_wifi_hndl.trans_wmi_message_id = WMI_SET_PASSPHRASE_CMDID;

        if (NT_QUEUE_FAIL == nt_osal_queue_send(msg_wfm_wmi_id, (void *)&g_Cmd_Translation_wifi_hndl, portMAX_DELAY)) {
            NT_LOG_WFM_ERR("Queue send failed", 0, 0, 0);
        }

        g_Cmd_Translation_wifi_hndl.msg_struct.return_status = eWiFiNotSupported;
    }
#ifdef SUPPORT_5GHZ
    uint8_t wlan_mode = ((wlan_join_cmd_t *)msg)->wlan_mode;
    uint16_t freq = ((wlan_join_cmd_t *)msg)->channel;

    /* Aria platform will send wlan_mode = 5 for both 2G and 5G channel */
    if (IS_MODE_11ABGN_HT20(wlan_mode)) {
        if (FREQ_IS_5G(freq)) {
            connect_ap.wlan_mode = get_wifi_phymode_from_wmi_phymode(WLAN_MODE_11A_HT20);

        } else if (FREQ_IS_2G(freq)) {
            connect_ap.wlan_mode = get_wifi_phymode_from_wmi_phymode(WLAN_MODE_11NG_HT20);
        } else {
            NT_LOG_WFM_ERR("Invalid frequency", freq, 0, 0);
            g_Cmd_Translation_wifi_hndl.msg_struct.return_status = eWiFiFailure;
            ringif_free_ctrl_buf(msg, len);
            return (g_Cmd_Translation_wifi_hndl.msg_struct.return_status);
        }
    } else if ((FREQ_IS_5G(freq) && (PHYMODE_IS_5G(wlan_mode))) || (FREQ_IS_2G(freq) && (PHYMODE_IS_2G(wlan_mode)))) {
        connect_ap.wlan_mode = wlan_mode;
    } else {
        NT_LOG_WFM_ERR("Wrong combination of phymode is issued with a wrong channel", wlan_mode, freq, 0);
        g_Cmd_Translation_wifi_hndl.msg_struct.return_status = eWiFiFailure;
        ringif_free_ctrl_buf(msg, len);
        return (g_Cmd_Translation_wifi_hndl.msg_struct.return_status);
    }
#endif
    /* enable the socpm state based on the INI values */
    nt_configure_socpm_state();
    /* configuring the imps for hosted mode */
    nt_configure_host_mode_in_imps();
    connect_ap.num_channels = 1;
    chindex = dc_freq_to_chindex(gpDevCommon, (((wlan_join_cmd_t *)msg)->channel));
    connect_ap.channel_list[0] = chindex;
    g_Cmd_Translation_wifi_hndl.msg_struct.netif_id = ((wlan_cmd_hdr *)msg)->network_id;
    g_Cmd_Translation_wifi_hndl.trans_wmi_message_id = WMI_CONNECT_CMDID;
    g_Cmd_Translation_wifi_hndl.msg_struct.vo_data = &connect_ap;

    NT_LOG_WFM_ERR("wlan_join cmd", 0, 0, 0);

    if (NT_QUEUE_FAIL ==
        nt_osal_queue_send(msg_wfm_wmi_id, (void *)&g_Cmd_Translation_wifi_hndl, NT_MAX_QUEUE_WAIT_TIME)) {
        NT_LOG_WFM_ERR("Queue send failed", 0, 0, 0);
    }

    ringif_free_ctrl_buf(msg, len);
    return (g_Cmd_Translation_wifi_hndl.msg_struct.return_status);
}

/**
 *@brief Called as a result of WLAN_DISABLE_CMD from APPS
 *
 *This function is called as if the APPS sends WLAN_DISABLE_CMD command,
 *it converts the structrure from APPS to WMI structure and triggers,
 *WMI_WLAN_OFF_CMDID WMI command.
 *
 * @param msg: contains function specific data sent from APPS
 * @param len: length of message  buffer
 * @return returns one of WIFIReturnCode_t enums
 *   eWiFiNotSupported in case there is some fatal error.
 *   eWiFiSuccess in case of Success
 *   eWiFiTimeout if timeout happens, during one of task.
 *   eWiFiFailure in case of Failure
 */

WIFIReturnCode_t wlan_disable(void *msg, uint16_t len)
{
    g_Cmd_Translation_wifi_hndl.msg_struct.return_status = eWiFiNotSupported;

    g_Cmd_Translation_wifi_hndl.msg_struct.netif_id = ((wlan_cmd_hdr *)msg)->network_id;
    g_Cmd_Translation_wifi_hndl.trans_wmi_message_id = WMI_WLAN_OFF_CMDID;
    g_Cmd_Translation_wifi_hndl.msg_struct.vo_data = NULL;
    g_Cmd_Translation_wifi_hndl.msg_struct.result_function = &wmi_response_handler;
    g_Cmd_Translation_wifi_hndl.prot_flg = WMI_WLAN_OFF_CMDID;

    NT_LOG_WFM_ERR("wlan_disable cmd", 0, 0, 0);

    if (NT_QUEUE_FAIL == nt_osal_queue_send(msg_wfm_wmi_id, &g_Cmd_Translation_wifi_hndl, NT_MAX_QUEUE_WAIT_TIME)) {
        NT_LOG_WFM_ERR("Queue send failed", 0, 0, 0);
    }

    if (eWiFiSuccess == g_Cmd_Translation_wifi_hndl.msg_struct.return_status) {
        xQueueReset(msg_wfm_wmi_id);
    }

    ringif_free_ctrl_buf(msg, len);
    return (g_Cmd_Translation_wifi_hndl.msg_struct.return_status);
}

/**
 *@brief Called as a result of WLAN_DISCONNECT_CMD from APPS
 *
 *This function is called as if the APPS sends WLAN_DISCONNECT_CMD command,
 * it converts the structrure from APPS to WMI structure and triggers,
 * WMI_DISCONNECT_CMDID WMI command.
 *
 * @param msg: contains function specific data sent from APPS
 * @param len: length of message  buffer
 * @return returns one of WIFIReturnCode_t enums
 *   eWiFiNotSupported in case there is some fatal error.
 *   eWiFiSuccess in case of Success
 *   eWiFiTimeout if timeout happens, during one of task.
 *   eWiFiFailure in case of Failure
 */

WIFIReturnCode_t wlan_disconnect(void *msg, uint16_t len)
{
    g_Cmd_Translation_wifi_hndl.msg_struct.return_status = eWiFiNotSupported;

    g_Cmd_Translation_wifi_hndl.msg_struct.netif_id = ((wlan_cmd_hdr *)msg)->network_id;
    g_Cmd_Translation_wifi_hndl.trans_wmi_message_id = WMI_DISCONNECT_CMDID;
    // g_Cmd_Translation_wifi_hndl.msg_struct.event_notify = &wmi_disconnect_event_notification;
    g_Cmd_Translation_wifi_hndl.msg_struct.vo_data = ((wlan_disconnect_cmd_t *)msg);
    g_Cmd_Translation_wifi_hndl.msg_struct.result_function = &wmi_response_handler;

    NT_LOG_WFM_ERR("wlan_disconnect cmd", 0, 0, 0);

    if (NT_QUEUE_FAIL ==
        nt_osal_queue_send(msg_wfm_wmi_id, (void *)&g_Cmd_Translation_wifi_hndl, NT_MAX_QUEUE_WAIT_TIME)) {
        NT_LOG_WFM_ERR("Queue send failed", 0, 0, 0);
    }

    ringif_free_ctrl_buf(msg, len);
    return (g_Cmd_Translation_wifi_hndl.msg_struct.return_status);
}

/**
 *@brief Called as a result of WLAN_IF_ADD_CMD from APPS
 *
 *This function is called as if the APPS sends WLAN_IF_ADD_CMD command,
 * it converts the structrure from APPS to WMI structure and triggers,
 *WMI_IF_ADD_CMDID WMI command.
 *
 * @param msg: contains function specific data sent from APPS
 * @param len: length of message  buffer
 * @return returns one of WIFIReturnCode_t enums
 *   eWiFiNotSupported in case there is some fatal error.
 *   eWiFiSuccess in case of Success
 *   eWiFiTimeout if timeout happens, during one of task.
 *   eWiFiFailure in case of Failure
 */

WIFIReturnCode_t wlan_if_add(void *msg, uint16_t len)
{
    if (g_command_struct.in_use == TRUE) {
        NT_LOG_WFM_ERR("Command buffer already in use", 0, 0, 0);
        ringif_free_ctrl_buf(msg, len);
        return eWiFiFailure;
    }
    g_command_struct.in_use = TRUE;
    memset(g_command_struct.command_buffer, 0x0, CTRL_RING_BUFF_SIZE);
    WMI_IF_ADD_CMD *if_config = (WMI_IF_ADD_CMD *)(g_command_struct.command_buffer);
    g_Cmd_Translation_wifi_hndl.msg_struct.return_status = eWiFiNotSupported;

    g_Cmd_Translation_wifi_hndl.trans_wmi_message_id = WMI_IF_ADD_CMDID;
    g_Cmd_Translation_wifi_hndl.msg_struct.result_function = &wmi_response_handler;

    if (eWiFiModeAP == ((wlan_if_add_cmd_t *)msg)->network_type) {
        connect_ap.networkType = AP_NETWORK;
        if_config->network_type = AP_NETWORK;

    } else if (eWiFiModeStation == ((wlan_if_add_cmd_t *)msg)->network_type) {
        connect_ap.networkType = INFRA_NETWORK;
        if_config->network_type = INFRA_NETWORK;
    }

    if_config->dhcp_type = ((wlan_if_add_cmd_t *)msg)->dhcp_type;

    if (RINGIF_IP_TYPE_STATIC == if_config->dhcp_type) {
        if_config->gateway = ((wlan_if_add_cmd_t *)msg)->gateway;
        if_config->netmask = ((wlan_if_add_cmd_t *)msg)->netmask;
        if (RINGIF_IP_VER_V4 == ((wlan_if_add_cmd_t *)msg)->ip_ver)
            if_config->ip_ver = IPADDR_TYPE_V4;
        else
            if_config->ip_ver = IPADDR_TYPE_V6;

        if (if_config->ip_ver == IPADDR_TYPE_V4) {
            if_config->ipv4_addr = ((wlan_if_add_cmd_t *)msg)->ipv4_addr;
        } else if (if_config->ip_ver == IPADDR_TYPE_V6) {
            memcpy(if_config->ipv6_addr, ((wlan_if_add_cmd_t *)msg)->ipv6_addr, sizeof(if_config->ipv6_addr));
        }
    }

    g_Cmd_Translation_wifi_hndl.msg_struct.vo_data = if_config;

    NT_LOG_WFM_ERR("wlan_if_add cmd", 0, 0, 0);

    if (NT_QUEUE_FAIL == nt_osal_queue_send(msg_wfm_wmi_id, &g_Cmd_Translation_wifi_hndl, NT_MAX_QUEUE_WAIT_TIME)) {
        NT_LOG_WFM_ERR("Queue send failed", 0, 0, 0);
    }

    ringif_free_ctrl_buf(msg, len);
    return (g_Cmd_Translation_wifi_hndl.msg_struct.return_status);
}

/**
 *@brief Called as a result of WLAN_SET_PARAM_CMD from APPS
 *
 *This function is called as if the APPS sends WLAN_SET_PARAM_CMD command,
 * it converts the structrure from APPS to WMI structure and triggers,
 *WMI_SET_PARAM_CMDID WMI command.
 *
 * @param msg: contains function specific data sent from APPS
 * @param len: length of message  buffer
 * @return returns one of WIFIReturnCode_t enums
 *   eWiFiNotSupported in case there is some fatal error.
 *   eWiFiSuccess in case of Success
 *   eWiFiTimeout if timeout happens, during one of task.
 *   eWiFiFailure in case of Failure
 */

WIFIReturnCode_t wlan_set_pdev_param(void *msg, uint16_t len)
{
    if (g_command_struct.in_use == TRUE) {
        NT_LOG_WFM_ERR("Command buffer already in use", 0, 0, 0);
        ringif_free_ctrl_buf(msg, len);
        return eWiFiFailure;
    }
    g_command_struct.in_use = TRUE;
    memset(g_command_struct.command_buffer, 0x0, CTRL_RING_BUFF_SIZE);
    WMI_SET_PDEV_PARAM_CMD *set_param = (WMI_SET_PDEV_PARAM_CMD *)(g_command_struct.command_buffer);

    g_Cmd_Translation_wifi_hndl.msg_struct.return_status = eWiFiNotSupported;
    g_Cmd_Translation_wifi_hndl.msg_struct.result_function = &wmi_response_handler;

    set_param->pdev_param_id = ((wlan_set_pdev_param_cmd_t *)msg)->pdev_param_id;
    set_param->pdev_param_value = ((wlan_set_pdev_param_cmd_t *)msg)->pdev_param_value;
    // TODO: need to figure out how to send param data
    g_Cmd_Translation_wifi_hndl.msg_struct.netif_id = ((wlan_cmd_hdr *)msg)->network_id;
    g_Cmd_Translation_wifi_hndl.trans_wmi_message_id = WMI_SET_PDEV_PARAM_CMDID;
    g_Cmd_Translation_wifi_hndl.msg_struct.result_function = &wmi_response_handler;

    g_Cmd_Translation_wifi_hndl.msg_struct.vo_data = set_param;

    NT_LOG_WFM_ERR("wlan_set_pdev_param cmd", 0, 0, 0);

    if (NT_QUEUE_FAIL == nt_osal_queue_send(msg_wfm_wmi_id, &g_Cmd_Translation_wifi_hndl, NT_MAX_QUEUE_WAIT_TIME)) {
        NT_LOG_WFM_ERR("Queue send failed", 0, 0, 0);
    }

    ringif_free_ctrl_buf(msg, len);
    return (g_Cmd_Translation_wifi_hndl.msg_struct.return_status);
}

WIFIReturnCode_t wlan_scan_stop_cmd(void *msg, uint16_t len)
{
    if (g_command_struct.in_use == TRUE) {
        NT_LOG_WFM_ERR("Command buffer already in use", 0, 0, 0);
        ringif_free_ctrl_buf(msg, len);
        return eWiFiFailure;
    }
    g_command_struct.in_use = TRUE;
    memset(g_command_struct.command_buffer, 0x0, CTRL_RING_BUFF_SIZE);
    WMI_SCAN_STOP_CMD *scan_stop = (WMI_SCAN_STOP_CMD *)(g_command_struct.command_buffer);

    g_Cmd_Translation_wifi_hndl.msg_struct.return_status = eWiFiNotSupported;
    g_Cmd_Translation_wifi_hndl.msg_struct.result_function = &wmi_response_handler;

    scan_stop->scan_id = ((wlan_scan_stop_cmd_t *)msg)->scan_id;
    g_Cmd_Translation_wifi_hndl.msg_struct.netif_id = ((wlan_cmd_hdr *)msg)->network_id;
    g_Cmd_Translation_wifi_hndl.trans_wmi_message_id = WMI_SCAN_STOP_CMDID;
    g_Cmd_Translation_wifi_hndl.msg_struct.result_function = &wmi_response_handler;

    g_Cmd_Translation_wifi_hndl.msg_struct.vo_data = scan_stop;

    NT_LOG_WFM_ERR("wlan_scan_stop cmd", 0, 0, 0);

    if (NT_QUEUE_FAIL == nt_osal_queue_send(msg_wfm_wmi_id, &g_Cmd_Translation_wifi_hndl, NT_MAX_QUEUE_WAIT_TIME)) {
        NT_LOG_WFM_ERR("Queue send failed", 0, 0, 0);
    }

    ringif_free_ctrl_buf(msg, len);
    return (g_Cmd_Translation_wifi_hndl.msg_struct.return_status);
}

/**
 *@brief Called as a result of WLAN_PDEV_UTF_CMD from APPS
 *
 *This function is called as if the APPS sends WLAN_PDEV_UTF_CMD command,
 * it converts the structrure from APPS to WMI structure and triggers,
 *WMI_PDEV_UTF_CMDID WMI command.
 *
 * @param msg: contains function specific data sent from APPS
 * @param len: length of message  buffer
 * @return returns one of WIFIReturnCode_t enums
 *   eWiFiNotSupported in case there is some fatal error.
 *   eWiFiSuccess in case of Success
 *   eWiFiTimeout if timeout happens, during one of task.
 *   eWiFiFailure in case of Failure
 */

WIFIReturnCode_t wlan_pdev_utf(void *msg, uint16_t len)
{
    wlan_pdev_utf_cmd_t *pdev_utf_cmd_ptr;
    pdev_utf_cmd_ptr = (wlan_pdev_utf_cmd_t *)msg;
    void *tlv20_buffer_loc;
    if (nt_get_app_mode() == APP_MODE_MM) {
        A_ASSERT(0);
    }
    if (g_command_struct.in_use == TRUE) {
        NT_LOG_WFM_ERR("Command buffer already in use", 0, 0, 0);
        ringif_free_ctrl_buf(msg, len);
        return eWiFiFailure;
    }
    g_command_struct.in_use = TRUE;
    memset(g_command_struct.command_buffer, 0x0, FTM_CTRL_RING_BUFF_SIZE);

    g_Cmd_Translation_wifi_hndl.msg_struct.return_status = eWiFiNotSupported;
    g_Cmd_Translation_wifi_hndl.trans_wmi_message_id = WMI_PDEV_UTF_CMDID;
    g_Cmd_Translation_wifi_hndl.msg_struct.result_function = &wmi_response_handler;

    /* input variable len is length of buffer received in bytes
     * over all length is having fermion wifi header and tlv2.0 buffer
     * for further processing length of tlv2.0 buffer is needed hence
     * subtracting fermion wifi header length */
    tlv20_buffer_loc = g_command_struct.command_buffer;
    /* copy the tlv2.0 command to the wmi message */
    memcpy(tlv20_buffer_loc, pdev_utf_cmd_ptr->tlv20_buff, len - sizeof(wlan_cmd_hdr));
    // NT_LOG_WFM_ERR("QDART debug: mem location of buff", (uint32_t)tlv20_buffer_loc,
    // (uint32_t)pdev_utf_cmd_ptr->tlv20_buff, len - sizeof(wlan_cmd_hdr));
    g_Cmd_Translation_wifi_hndl.msg_struct.vo_data = tlv20_buffer_loc;
    g_Cmd_Translation_wifi_hndl.msg_struct.vo_data_len = len - sizeof(wlan_cmd_hdr);

    NT_LOG_WFM_ERR("wlan_pdev_utf cmd", 0, 0, 0);
    if (NT_QUEUE_FAIL == nt_osal_queue_send(msg_wfm_wmi_id, &g_Cmd_Translation_wifi_hndl, NT_MAX_QUEUE_WAIT_TIME)) {
        NT_LOG_WFM_ERR("Queue send failed", 0, 0, 0);
    }
    /* TODO: Need to check with HALPHY if they need the orignal buffer before cleaning. */
    ringif_free_ctrl_buf(msg, len);
    return (g_Cmd_Translation_wifi_hndl.msg_struct.return_status);
}

/**
 *@brief Called as a result of WLAN_MODE_CMD from APPS
 *
 *This function is called as if the APPS sends WLAN_MODE_CMD command,
 * it converts the structrure from APPS to WMI structure and triggers,
 *WMI_MODE_CMDID WMI command.
 *
 * @param msg: contains function specific data sent from APPS
 * @param len: length of message  buffer
 * @return returns one of WIFIReturnCode_t enums
 *   eWiFiNotSupported in case there is some fatal error.
 *   eWiFiSuccess in case of Success
 *   eWiFiTimeout if timeout happens, during one of task.
 *   eWiFiFailure in case of Failure
 */

WIFIReturnCode_t wlan_set_mode(void *msg, uint16_t len)
{
    if (g_command_struct.in_use == TRUE) {
        NT_LOG_WFM_ERR("Command buffer already in use", 0, 0, 0);
        ringif_free_ctrl_buf(msg, len);
        return eWiFiFailure;
    }
    g_command_struct.in_use = TRUE;
    memset(g_command_struct.command_buffer, 0x0, CTRL_RING_BUFF_SIZE);
    WMI_MODE_CMD *mode_params = (WMI_MODE_CMD *)(g_command_struct.command_buffer);

    g_Cmd_Translation_wifi_hndl.trans_wmi_message_id = WMI_MODE_CMDID;
    g_Cmd_Translation_wifi_hndl.msg_struct.result_function = &wmi_response_handler;

    mode_params->requested_mode = ((wlan_mode_cmd_t *)msg)->requested_mode;
    g_Cmd_Translation_wifi_hndl.msg_struct.vo_data = mode_params;

    NT_LOG_WFM_ERR("wlan_set_mode cmd", 0, 0, 0);

    if (NT_QUEUE_FAIL == nt_osal_queue_send(msg_wfm_wmi_id, &g_Cmd_Translation_wifi_hndl, NT_MAX_QUEUE_WAIT_TIME)) {
        NT_LOG_WFM_ERR("Queue send failed", 0, 0, 0);
    }

    ringif_free_ctrl_buf(msg, len);
    return (g_Cmd_Translation_wifi_hndl.msg_struct.return_status);
}

/**
 *@brief Called as a result of WLAN_UNIT_TEST_CMD from APPS
 *
 *This function is called as if the APPS sends WLAN_UNIT_TEST_CMD command,
 * it converts the structrure from APPS to WMI structure and triggers,
 * corrosponding WMI command.
 *
 * @param msg: contains function specific data sent from APPS
 * @param len: length of message  buffer
 * @return returns one of WIFIReturnCode_t enums
 *   eWiFiNotSupported in case there is some fatal error.
 *   eWiFiSuccess in case of Success
 *   eWiFiTimeout if timeout happens, during one of task.
 *   eWiFiFailure in case of Failure
 */
WMI_UNIT_TEST_CMD rng_unit_test_cmd;
WIFIReturnCode_t wlan_unit_test_cmd(void *msg, uint16_t len)
{
#ifdef UNIT_TEST_SUPPORT
    int arg_idx;
    if (len - sizeof(wlan_cmd_hdr) > sizeof(WMI_UNIT_TEST_CMD)) {
        return eWiFiFailure;
    }

    rng_unit_test_cmd.vdev_id = ((wlan_unit_test_cmd_t *)msg)->vdev_id;
    rng_unit_test_cmd.module_id = ((wlan_unit_test_cmd_t *)msg)->module_id;
    rng_unit_test_cmd.num_args = ((wlan_unit_test_cmd_t *)msg)->num_args;

    for (arg_idx = 0; arg_idx < rng_unit_test_cmd.num_args; arg_idx++) {
        rng_unit_test_cmd.args[arg_idx] = ((wlan_unit_test_cmd_t *)msg)->args[arg_idx];
    }

    g_Cmd_Translation_wifi_hndl.trans_wmi_message_id = WMI_UNIT_TEST_CMDID;
    g_Cmd_Translation_wifi_hndl.msg_struct.vo_data = &rng_unit_test_cmd;
    g_Cmd_Translation_wifi_hndl.msg_struct.result_function = &wmi_response_handler;

    NT_LOG_WFM_ERR("wlan_unit_test cmd", 0, 0, 0);

    if (NT_QUEUE_FAIL == nt_osal_queue_send(msg_wfm_wmi_id, (void *)&g_Cmd_Translation_wifi_hndl, portMAX_DELAY)) {
        NT_LOG_WFM_ERR("Queue send failed", 0, 0, 0);
    }

    ringif_free_ctrl_buf(msg, len);
    return (g_Cmd_Translation_wifi_hndl.msg_struct.return_status);
#else
    NT_LOG_WFM_ERR("Command WMI_UNIT_TEST_CMD not supported", 0, 0, 0);
    ringif_free_ctrl_buf(msg, len);
    return eWiFiFailure;
#endif  // UNIT_TEST_SUPPORT
}

/**
 *@brief Called as a result of from APPS
 *
 *This function is called as if the APPS sends WLAN_TWT_SETUP_CMD command,
 * it converts the structrure from APPS to WMI structure and triggers,
 *corrosponding WMI command.
 *
 * @param msg: contains function specific data sent from APPS
 * @param len: length of message  buffer
 * @return returns one of WIFIReturnCode_t enums
 *   eWiFiNotSupported in case there is some fatal error.
 *   eWiFiSuccess in case of Success
 *   eWiFiTimeout if timeout happens, during one of task.
 *   eWiFiFailure in case of Failure
 */

WIFIReturnCode_t wlan_twt_setup_cmd(void *msg, uint16_t len)
{
#ifdef SUPPORT_TWT_STA
    if (gdevp != NULL) {
        PM_STRUCT *pPmStruct = gdevp->pPmStruct;
        if (pPmStruct != NULL && !(pPmStruct->twt_struct.twt_enable)) {
            NT_LOG_WFM_ERR("Failed to setup TWT", 0, 0, 0);
            return eWiFiFailure;
        }
    }

    if (len != sizeof(wlan_twt_setup_cmd_t)) {
        return eWiFiFailure;
    }
    g_Cmd_Translation_wifi_hndl.msg_struct.return_status = eWiFiNotSupported;
    g_Cmd_Translation_wifi_hndl.trans_wmi_message_id = WMI_TWT_SETUP_CMDID;
    g_Cmd_Translation_wifi_hndl.msg_struct.vo_data = msg;
    g_Cmd_Translation_wifi_hndl.msg_struct.result_function = &wmi_response_handler;

    NT_LOG_WFM_ERR("wlan_twt_setup cmd", 0, 0, 0);

    if (NT_QUEUE_FAIL == nt_osal_queue_send(msg_wfm_wmi_id, (void *)&g_Cmd_Translation_wifi_hndl, portMAX_DELAY)) {
        NT_LOG_WFM_ERR("Queue send failed", 0, 0, 0);
    }
    ringif_free_ctrl_buf(msg, len);
    return (g_Cmd_Translation_wifi_hndl.msg_struct.return_status);
#else
    NT_LOG_WFM_ERR("Command WMI_TWT_SETUP_CMDID not supported", 0, 0, 0);
    ringif_free_ctrl_buf(msg, len);
    return eWiFiFailure;
#endif
}

/**
 *@brief Called as a result of from APPS
 *
 *This function is called as if the APPS sends WLAN_TWT_TEARDOWN_CMD
 *command, it converts the structrure from APPS to WMI structure and triggers,
 *corrosponding WMI command.
 *
 * @param msg: contains function specific data sent from APPS
 * @param len: length of message  buffer
 * @return returns one of WIFIReturnCode_t enums
 *   eWiFiNotSupported in case there is some fatal error.
 *   eWiFiSuccess in case of Success
 *   eWiFiTimeout if timeout happens, during one of task.
 *   eWiFiFailure in case of Failure
 */

WIFIReturnCode_t wlan_twt_teardown_cmd(void *msg, uint16_t len)
{
#ifdef SUPPORT_TWT_STA
    if (len != sizeof(wlan_twt_teardown_cmd_t)) {
        return eWiFiFailure;
    }
    g_Cmd_Translation_wifi_hndl.msg_struct.return_status = eWiFiNotSupported;
    g_Cmd_Translation_wifi_hndl.trans_wmi_message_id = WMI_TWT_TEARDOWN_CMDID;
    g_Cmd_Translation_wifi_hndl.msg_struct.vo_data = msg;
    g_Cmd_Translation_wifi_hndl.msg_struct.result_function = &wmi_response_handler;

    NT_LOG_WFM_ERR("wlan_teardown cmd", 0, 0, 0);

    if (NT_QUEUE_FAIL == nt_osal_queue_send(msg_wfm_wmi_id, (void *)&g_Cmd_Translation_wifi_hndl, portMAX_DELAY)) {
        NT_LOG_WFM_ERR("Queue send failed", 0, 0, 0);
    }
    ringif_free_ctrl_buf(msg, len);
    return (g_Cmd_Translation_wifi_hndl.msg_struct.return_status);
#else
    NT_LOG_WFM_ERR("Command WMI_TWT_TEARDOWN_CMDID not supported", 0, 0, 0);
    ringif_free_ctrl_buf(msg, len);
    return eWiFiFailure;
#endif
}

/**
 *@brief Called as a result of from APPS
 *
 *This function is called as if the APPS sends WLAN_TWT_STATUS_CMD
 *command, it converts the structrure from APPS to WMI structure and triggers,
 *corrosponding WMI command.
 *
 * @param msg: contains function specific data sent from APPS
 * @param len: length of message  buffer
 * @return returns one of WIFIReturnCode_t enums
 *   eWiFiNotSupported in case there is some fatal error.
 *   eWiFiSuccess in case of Success
 *   eWiFiTimeout if timeout happens, during one of task.
 *   eWiFiFailure in case of Failure
 */

WIFIReturnCode_t wlan_twt_status_cmd(void *msg, uint16_t len)
{
#ifdef SUPPORT_TWT_STA
    if (len != sizeof(wlan_twt_status_cmd_t)) {
        return eWiFiFailure;
    }
    g_Cmd_Translation_wifi_hndl.msg_struct.return_status = eWiFiNotSupported;
    g_Cmd_Translation_wifi_hndl.trans_wmi_message_id = WMI_TWT_STATUS_CMDID;
    g_Cmd_Translation_wifi_hndl.msg_struct.vo_data = msg;
    g_Cmd_Translation_wifi_hndl.msg_struct.result_function = &wmi_response_handler;

    NT_LOG_WFM_ERR("wlan_twt_status cmd", 0, 0, 0);

    if (NT_QUEUE_FAIL == nt_osal_queue_send(msg_wfm_wmi_id, (void *)&g_Cmd_Translation_wifi_hndl, portMAX_DELAY)) {
        NT_LOG_WFM_ERR("Queue send failed", 0, 0, 0);
    }
    ringif_free_ctrl_buf(msg, len);
    return (g_Cmd_Translation_wifi_hndl.msg_struct.return_status);
#else
    NT_LOG_WFM_ERR("Command WMI_TWT_STATUS_CMDID not supported", 0, 0, 0);
    ringif_free_ctrl_buf(msg, len);
    return eWiFiFailure;
#endif
}

WIFIReturnCode_t wlan_phydbgdump_cmd(void *msg, uint16_t len)
{
    g_Cmd_Translation_wifi_hndl.msg_struct.return_status = eWiFiNotSupported;
    g_Cmd_Translation_wifi_hndl.trans_wmi_message_id = WMI_PHYDBGDUMP_CMD;
    g_Cmd_Translation_wifi_hndl.msg_struct.vo_data = NULL;
    g_Cmd_Translation_wifi_hndl.msg_struct.result_function = &wmi_response_handler;

    NT_LOG_WFM_ERR("wlan_phydbgdump cmd", 0, 0, 0);

    if (NT_QUEUE_FAIL ==
        nt_osal_queue_send(msg_wfm_wmi_id, (void *)&g_Cmd_Translation_wifi_hndl, NT_MAX_QUEUE_WAIT_TIME)) {
        NT_LOG_WFM_ERR("Queue send failed", 0, 0, 0);
    }

    ringif_free_ctrl_buf(msg, len);
    return (g_Cmd_Translation_wifi_hndl.msg_struct.return_status);
}

WIFIReturnCode_t wlan_update_bi_cmd(void *msg, uint16_t len)
{
#ifdef SUPPORT_SAP_POWERSAVE
    if (g_command_struct.in_use == TRUE) {
        NT_LOG_WFM_ERR("Command buffer already in use", 0, 0, 0);
        ringif_free_ctrl_buf(msg, len);
        return eWiFiFailure;
    }
    g_command_struct.in_use = TRUE;
    memset(g_command_struct.command_buffer, 0x0, CTRL_RING_BUFF_SIZE);
    WMI_UPDATE_BI_CMD *update_bi = (WMI_UPDATE_BI_CMD *)(g_command_struct.command_buffer);

    update_bi->next_tbtt_hi = ((wlan_update_bi_cmd_t *)msg)->next_tbtt_high;
    update_bi->next_tbtt_lo = ((wlan_update_bi_cmd_t *)msg)->next_tbtt_low;
    update_bi->beacon_multiplier = ((wlan_update_bi_cmd_t *)msg)->beacon_multiplier;
    update_bi->freq = ((wlan_update_bi_cmd_t *)msg)->freq;
    g_Cmd_Translation_wifi_hndl.msg_struct.return_status = eWiFiNotSupported;
    g_Cmd_Translation_wifi_hndl.trans_wmi_message_id = WMI_UPDATE_BI_CMDID;
    g_Cmd_Translation_wifi_hndl.msg_struct.vo_data = update_bi;
    g_Cmd_Translation_wifi_hndl.msg_struct.result_function = &wmi_response_handler;

    NT_LOG_WFM_ERR("wlan_update_bi cmd", 0, 0, 0);

    if (NT_QUEUE_FAIL == nt_osal_queue_send(msg_wfm_wmi_id, &g_Cmd_Translation_wifi_hndl, NT_MAX_QUEUE_WAIT_TIME)) {
        NT_LOG_WFM_ERR("Queue send failed", 0, 0, 0);
    }

    ringif_free_ctrl_buf(msg, len);
    return (g_Cmd_Translation_wifi_hndl.msg_struct.return_status);
#else
    NT_LOG_WFM_ERR("Command WMI_UPDATE_BI_CMDID not supported", 0, 0, 0);
    ringif_free_ctrl_buf(msg, len);
    return eWiFiFailure;

#endif /* SUPPORT_SAP_POWERSAVE */
}

/**
 *@brief funtion to update the BMTT configuration
 *
 *This function is called as if the APPS sends WLAN_UPDATE_NMTT_CMD
 *command, it converts the structrure from APPS to WMI structure and triggers,
 *corrosponding WMI command.
 *
 * @param msg: contains function specific data sent from APPS
 * @param len: length of message  buffer
 * @return returns one of WIFIReturnCode_t enums
 *   eWiFiNotSupported in case there is some fatal error.
 *   eWiFiSuccess in case of Success
 *   eWiFiTimeout if timeout happens, during one of task.
 *   eWiFiFailure in case of Failure
 */
WIFIReturnCode_t wlan_update_bmtt_cmd(void *msg, uint16_t len)
{
#ifdef SUPPORT_BEACON_MISS_THRESHOLD_TIME
    if (g_command_struct.in_use == TRUE) {
        NT_LOG_WFM_ERR("Command buffer already in use", 0, 0, 0);
        ringif_free_ctrl_buf(msg, len);
        return eWiFiFailure;
    }
    g_command_struct.in_use = TRUE;
    memset(g_command_struct.command_buffer, 0x0, CTRL_RING_BUFF_SIZE);
    WMI_UPDATE_BMTT_CMD *update_bmtt = (WMI_UPDATE_BMTT_CMD *)(g_command_struct.command_buffer);

    update_bmtt->beacon_miss_thr_time_us = ((wlan_update_bmtt_cmd_t *)msg)->beacon_miss_thr_time_us;
    g_Cmd_Translation_wifi_hndl.msg_struct.return_status = eWiFiNotSupported;
    g_Cmd_Translation_wifi_hndl.trans_wmi_message_id = WMI_UPDATE_BMTT_CMDID;
    g_Cmd_Translation_wifi_hndl.msg_struct.vo_data = update_bmtt;
    g_Cmd_Translation_wifi_hndl.msg_struct.result_function = &wmi_response_handler;

    NT_LOG_WFM_ERR("wlan_update_bmtt cmd", 0, 0, 0);

    if (NT_QUEUE_FAIL == nt_osal_queue_send(msg_wfm_wmi_id, &g_Cmd_Translation_wifi_hndl, NT_MAX_QUEUE_WAIT_TIME)) {
        NT_LOG_WFM_ERR("Queue send failed", 0, 0, 0);
    }

    ringif_free_ctrl_buf(msg, len);
    return (g_Cmd_Translation_wifi_hndl.msg_struct.return_status);
#else
    NT_LOG_WFM_ERR("Command WMI_UPDATE_BMTT_CMDID not supported", 0, 0, 0);
    ringif_free_ctrl_buf(msg, len);
    return eWiFiFailure;

#endif /* SUPPORT_BEACON_MISS_THRESHOLD_TIME */
}

WIFIReturnCode_t wlan_set_reset_wakelock_cmd(void *msg, uint16_t len)
{
    if (g_command_struct.in_use == TRUE) {
        NT_LOG_WFM_ERR("Command buffer already in use", 0, 0, 0);
        ringif_free_ctrl_buf(msg, len);
        return eWiFiFailure;
    }
    g_command_struct.in_use = TRUE;
    memset(g_command_struct.command_buffer, 0x0, CTRL_RING_BUFF_SIZE);
    uint8_t *wakelock_command_type = (uint8_t *)(g_command_struct.command_buffer);
    *wakelock_command_type = ((wlan_set_reset_wakelock_cmd_t *)msg)->wakelock_command_type;
    g_Cmd_Translation_wifi_hndl.msg_struct.return_status = eWiFiNotSupported;
    g_Cmd_Translation_wifi_hndl.trans_wmi_message_id = WMI_SET_RESET_WAKELOCK_CMDID;
    g_Cmd_Translation_wifi_hndl.msg_struct.vo_data = wakelock_command_type;
    g_Cmd_Translation_wifi_hndl.msg_struct.result_function = &wmi_response_handler;

    NT_LOG_WFM_ERR("wlan_set_reset_wakelock cmd", 0, 0, 0);

    if (NT_QUEUE_FAIL == nt_osal_queue_send(msg_wfm_wmi_id, &g_Cmd_Translation_wifi_hndl, NT_MAX_QUEUE_WAIT_TIME)) {
        NT_LOG_WFM_ERR("Queue send failed", 0, 0, 0);
    }

    ringif_free_ctrl_buf(msg, len);
    return (g_Cmd_Translation_wifi_hndl.msg_struct.return_status);
}

/**
 *@brief Called as a result of from APPS
 *
 *This function is called as if the APPS sends WLAN_PERIODIC_TSF_SYNC_CMD
 *command, it converts the structrure from APPS to WMI structure and triggers,
 *corrosponding WMI command.
 *
 * @param msg: contains function specific data sent from APPS
 * @param len: length of message  buffer
 * @return returns one of WIFIReturnCode_t enums
 *   eWiFiNotSupported in case there is some fatal error.
 *   eWiFiSuccess in case of Success
 *   eWiFiTimeout if timeout happens, during one of task.
 *   eWiFiFailure in case of Failure
 */

WIFIReturnCode_t wlan_periodic_tsf_sync_cmd(void *msg, uint16_t len)
{
#ifdef SUPPORT_PERIODIC_TSF_SYNC
    if (g_command_struct.in_use == TRUE) {
        NT_LOG_WFM_ERR("Command buffer already in use", 0, 0, 0);
        ringif_free_ctrl_buf(msg, len);
        return eWiFiFailure;
    }

    g_Cmd_Translation_wifi_hndl.msg_struct.return_status = eWiFiNotSupported;
    g_Cmd_Translation_wifi_hndl.trans_wmi_message_id = WMI_PERIODIC_TSF_SYNC_CMDID;
    g_Cmd_Translation_wifi_hndl.msg_struct.vo_data = msg;
    g_Cmd_Translation_wifi_hndl.msg_struct.result_function = &wmi_response_handler;

    NT_LOG_WFM_ERR("wlan_periodic_tsf_sync cmd", 0, 0, 0);

    if (NT_QUEUE_FAIL == nt_osal_queue_send(msg_wfm_wmi_id, (void *)&g_Cmd_Translation_wifi_hndl, portMAX_DELAY)) {
        NT_LOG_WFM_ERR("Queue send failed", 0, 0, 0);
    }
    ringif_free_ctrl_buf(msg, len);
    return (g_Cmd_Translation_wifi_hndl.msg_struct.return_status);
#else
    NT_LOG_WFM_ERR("Command WMI_PERIODIC_TSF_SYNC_CMDID not supported", 0, 0, 0);
    ringif_free_ctrl_buf(msg, len);
    return eWiFiFailure;
#endif /* SUPPORT_PERIODIC_TSF_SYNC */
}

/**
 *@brief Called as a result of f2a_pulse_on_twt_wakeup cmd from APPS
 *
 *This function is called when APPS sends WLAN_F2A_PULSE_ON_TWT_WAKEUP_CMD
 *command, it converts the structrure from APPS to WMI structure and triggers,
 *corrosponding WMI command.
 *
 * @param msg: contains function specific data sent from APPS
 * @param len: length of message  buffer
 * @return returns one of WIFIReturnCode_t enums
 *   eWiFiNotSupported in case there is some fatal error.
 *   eWiFiSuccess in case of Success
 *   eWiFiTimeout if timeout happens, during one of task.
 *   eWiFiFailure in case of Failure
 */
WIFIReturnCode_t wlan_f2a_on_wkup_config_cmd(void *msg, uint16_t len)
{
#ifdef FIRMWARE_APPS_INFORMED_WAKE
    if (g_command_struct.in_use == TRUE) {
        NT_LOG_WFM_ERR("Command buffer already in use", 0, 0, 0);
        ringif_free_ctrl_buf(msg, len);
        return eWiFiFailure;
    }
    g_command_struct.in_use = TRUE;
    memset(g_command_struct.command_buffer, 0x0, CTRL_RING_BUFF_SIZE);

    uint8_t *f2a_pulse_enable = (uint8_t *)(g_command_struct.command_buffer);
    *f2a_pulse_enable = ((wlan_f2a_pulse_on_twt_wakeup_cmd_t *)msg)->enable;

    g_Cmd_Translation_wifi_hndl.msg_struct.return_status = eWiFiNotSupported;
    g_Cmd_Translation_wifi_hndl.trans_wmi_message_id = WMI_F2A_PULSE_ON_TWT_WAKEUP_CMDID;
    g_Cmd_Translation_wifi_hndl.msg_struct.vo_data = f2a_pulse_enable;
    g_Cmd_Translation_wifi_hndl.msg_struct.result_function = &wmi_response_handler;

    NT_LOG_WFM_ERR("wlan_f2a_pulse_on_twt_wakeup cmd", 0, 0, 0);

    if (NT_QUEUE_FAIL == nt_osal_queue_send(msg_wfm_wmi_id, &g_Cmd_Translation_wifi_hndl, NT_MAX_QUEUE_WAIT_TIME)) {
        NT_LOG_WFM_ERR("Queue send failed", 0, 0, 0);
    }

    ringif_free_ctrl_buf(msg, len);
    return (g_Cmd_Translation_wifi_hndl.msg_struct.return_status);
#else
    NT_LOG_WFM_ERR("Command WMI_F2A_PULSE_ON_TWT_WAKEUP_CMDID not supported", 0, 0, 0);
    ringif_free_ctrl_buf(msg, len);
    return eWiFiFailure;

#endif /* FIRMWARE_APPS_INFORMED_WAKE */
}

WIFIReturnCode_t wlan_coex_evt_status(void *msg, uint16_t len)
{
#ifdef SUPPORT_COEX
    if (g_command_struct.in_use == TRUE) {
        NT_LOG_WFM_ERR("Command buffer already in use", 0, 0, 0);
        ringif_free_ctrl_buf(msg, len);
        return eWiFiFailure;
    }
    g_command_struct.in_use = TRUE;
    memset(g_command_struct.command_buffer, 0x0, CTRL_RING_BUFF_SIZE);
    uint8_t *coex_evt_status = (uint8_t *)(g_command_struct.command_buffer);
    *coex_evt_status = ((wlan_cxc_evt_status_t *)msg)->status;
    g_Cmd_Translation_wifi_hndl.msg_struct.return_status = eWiFiNotSupported;
    g_Cmd_Translation_wifi_hndl.trans_wmi_message_id = WMI_CXC_CMDID;
    g_Cmd_Translation_wifi_hndl.msg_struct.vo_data = coex_evt_status;

    NT_LOG_WFM_ERR("wlan_coex_evt_status cmd", 0, 0, 0);

    if (NT_QUEUE_FAIL == nt_osal_queue_send(msg_wfm_wmi_id, &g_Cmd_Translation_wifi_hndl, NT_MAX_QUEUE_WAIT_TIME)) {
        NT_LOG_WFM_ERR("Queue send failed", 0, 0, 0);
    }

    ringif_free_ctrl_buf(msg, len);
    return (g_Cmd_Translation_wifi_hndl.msg_struct.return_status);
#else
    NT_LOG_WFM_ERR("Command WMI_CXC_CMDID not supported", 0, 0, 0);
    ringif_free_ctrl_buf(msg, len);
    return eWiFiFailure;
#endif /* SUPPORT_COEX */
}

/**
 *@brief Setup periodic traffic session.
 *
 *This function is called as if the APPS sends WLAN_PERIODIC_TRAFFIC_SETUP_CMDID
 *command, it converts the structrure from APPS to WMI structure and triggers,
 *corrosponding WMI command.
 *
 * @param msg: contains function specific data sent from APPS
 * @param len: length of message  buffer
 * @return returns one of WIFIReturnCode_t enums
 *   eWiFiNotSupported in case there is some fatal error.
 *   eWiFiSuccess in case of Success
 *   eWiFiTimeout if timeout happens, during one of task.
 *   eWiFiFailure in case of Failure
 */
WIFIReturnCode_t wlan_periodic_traffic_setup_cmd(void *msg, uint16_t len)
{
#ifdef FEATURE_PERIODIC_WAKE_SLEEP
    if (g_command_struct.in_use == TRUE) {
        NT_LOG_WFM_ERR("Command buffer already in use", 0, 0, 0);
        ringif_free_ctrl_buf(msg, len);
        return eWiFiFailure;
    }
    g_command_struct.in_use = TRUE;
    memset(g_command_struct.command_buffer, 0x0, CTRL_RING_BUFF_SIZE);
    g_Cmd_Translation_wifi_hndl.msg_struct.return_status = eWiFiNotSupported;
    g_Cmd_Translation_wifi_hndl.trans_wmi_message_id = WMI_PERIODIC_TRAFFIC_SETUP_CMDID;
    g_Cmd_Translation_wifi_hndl.msg_struct.vo_data = msg;

    if (NT_QUEUE_FAIL == nt_osal_queue_send(msg_wfm_wmi_id, &g_Cmd_Translation_wifi_hndl, NT_MAX_QUEUE_WAIT_TIME)) {
        NT_LOG_WFM_ERR("Queue send failed", 0, 0, 0);
    }

    ringif_free_ctrl_buf(msg, len);
    return (g_Cmd_Translation_wifi_hndl.msg_struct.return_status);
#else
    NT_LOG_WFM_ERR("Command WMI_PERIODIC_TRAFFIC_SETUP_CMDID not supported", 0, 0, 0);
    ringif_free_ctrl_buf(msg, len);
    return eWiFiFailure;
#endif /* FEATURE_PERIODIC_WAKE_SLEEP */
}

/**
 *@brief To get the periodic traffic session's status
 *
 *This function is called as if the APPS sends WLAN_PERIODIC_TRAFFIC_STATUS_CMDID
 *command, it converts the structrure from APPS to WMI structure and triggers,
 *corrosponding WMI command.
 *
 * @param msg: contains function specific data sent from APPS
 * @param len: length of message  buffer
 * @return returns one of WIFIReturnCode_t enums
 *   eWiFiNotSupported in case there is some fatal error.
 *   eWiFiSuccess in case of Success
 *   eWiFiTimeout if timeout happens, during one of task.
 *   eWiFiFailure in case of Failure
 */
WIFIReturnCode_t wlan_periodic_traffic_status_cmd(void *msg, uint16_t len)
{
#ifdef FEATURE_PERIODIC_WAKE_SLEEP
    if (g_command_struct.in_use == TRUE) {
        NT_LOG_WFM_ERR("Command buffer already in use", 0, 0, 0);
        ringif_free_ctrl_buf(msg, len);
        return eWiFiFailure;
    }
    g_command_struct.in_use = TRUE;
    memset(g_command_struct.command_buffer, 0x0, CTRL_RING_BUFF_SIZE);
    g_Cmd_Translation_wifi_hndl.msg_struct.return_status = eWiFiNotSupported;
    g_Cmd_Translation_wifi_hndl.trans_wmi_message_id = WMI_PERIODIC_TRAFFIC_STATUS_CMDID;
    g_Cmd_Translation_wifi_hndl.msg_struct.vo_data = msg;

    if (NT_QUEUE_FAIL == nt_osal_queue_send(msg_wfm_wmi_id, &g_Cmd_Translation_wifi_hndl, NT_MAX_QUEUE_WAIT_TIME)) {
        NT_LOG_WFM_ERR("Queue send failed", 0, 0, 0);
    }

    ringif_free_ctrl_buf(msg, len);
    return (g_Cmd_Translation_wifi_hndl.msg_struct.return_status);
#else
    NT_LOG_WFM_ERR("Command WMI_PERIODIC_TRAFFIC_STATUS_CMDID not supported", 0, 0, 0);
    ringif_free_ctrl_buf(msg, len);
    return eWiFiFailure;
#endif /* FEATURE_PERIODIC_WAKE_SLEEP */
}

/**
 *@brief Teardown the periodic traffic session.
 *
 *This function is called as if the APPS sends WLAN_PERIODIC_TRAFFIC_TEARDOWN_CMDID
 *command, it converts the structrure from APPS to WMI structure and triggers,
 *corrosponding WMI command.
 *
 * @param msg: contains function specific data sent from APPS
 * @param len: length of message  buffer
 * @return returns one of WIFIReturnCode_t enums
 *   eWiFiNotSupported in case there is some fatal error.
 *   eWiFiSuccess in case of Success
 *   eWiFiTimeout if timeout happens, during one of task.
 *   eWiFiFailure in case of Failure
 */
WIFIReturnCode_t wlan_periodic_traffic_teardown_cmd(void *msg, uint16_t len)
{
#ifdef FEATURE_PERIODIC_WAKE_SLEEP
    if (g_command_struct.in_use == TRUE) {
        NT_LOG_WFM_ERR("Command buffer already in use", 0, 0, 0);
        ringif_free_ctrl_buf(msg, len);
        return eWiFiFailure;
    }
    g_command_struct.in_use = TRUE;
    memset(g_command_struct.command_buffer, 0x0, CTRL_RING_BUFF_SIZE);
    g_Cmd_Translation_wifi_hndl.msg_struct.return_status = eWiFiNotSupported;
    g_Cmd_Translation_wifi_hndl.trans_wmi_message_id = WMI_PERIODIC_TRAFFIC_TEARDOWN_CMDID;
    g_Cmd_Translation_wifi_hndl.msg_struct.vo_data = msg;

    if (NT_QUEUE_FAIL == nt_osal_queue_send(msg_wfm_wmi_id, &g_Cmd_Translation_wifi_hndl, NT_MAX_QUEUE_WAIT_TIME)) {
        NT_LOG_WFM_ERR("Queue send failed", 0, 0, 0);
    }

    ringif_free_ctrl_buf(msg, len);
    return (g_Cmd_Translation_wifi_hndl.msg_struct.return_status);
#else
    NT_LOG_WFM_ERR("Command WMI_PERIODIC_TRAFFIC_TEARDOWN_CMDID not supported", 0, 0, 0);
    ringif_free_ctrl_buf(msg, len);
    return eWiFiFailure;
#endif /* FEATURE_PERIODIC_WAKE_SLEEP */
}

/**
 *@brief Set the clock latency.
 *
 *This function is called as if the APPS sends WLAN_CLK_LATENCY_CMDID
 *command, it converts the structrure from APPS to WMI structure and triggers,
 *corrosponding WMI command.
 *
 * @param msg: contains function specific data sent from APPS
 * @param len: length of message  buffer
 * @return returns one of WIFIReturnCode_t enums
 *   eWiFiNotSupported in case there is some fatal error.
 *   eWiFiSuccess in case of Success
 *   eWiFiTimeout if timeout happens, during one of task.
 *   eWiFiFailure in case of Failure
 */
WIFIReturnCode_t wlan_clk_latency_cmd(void *msg, uint16_t len)
{
#ifdef SUPPORT_RING_IF
    if (g_command_struct.in_use == TRUE) {
        NT_LOG_WFM_ERR("Command buffer already in use", 0, 0, 0);
        ringif_free_ctrl_buf(msg, len);
        return eWiFiFailure;
    }
    g_command_struct.in_use = TRUE;
    memset(g_command_struct.command_buffer, 0x0, CTRL_RING_BUFF_SIZE);
    g_Cmd_Translation_wifi_hndl.msg_struct.return_status = eWiFiNotSupported;
    g_Cmd_Translation_wifi_hndl.trans_wmi_message_id = WMI_CLK_LATENCY_CMDID;
    g_Cmd_Translation_wifi_hndl.msg_struct.vo_data = msg;

    if (NT_QUEUE_FAIL == nt_osal_queue_send(msg_wfm_wmi_id, &g_Cmd_Translation_wifi_hndl, NT_MAX_QUEUE_WAIT_TIME)) {
        NT_LOG_WFM_ERR("Queue send failed", 0, 0, 0);
    }

    ringif_free_ctrl_buf(msg, len);
    return (g_Cmd_Translation_wifi_hndl.msg_struct.return_status);
#else
    NT_LOG_WFM_ERR("Command WMI_CLK_LATENCY_CMD not supported", 0, 0, 0);
    ringif_free_ctrl_buf(msg, len);
    return eWiFiFailure;
#endif
}

/*--------------------------------------------------------------------------
* This table must be in sync with the enum table shared with APPS
* If there is any mismatch, the wifi svc dispatcher will fail
---------------------------------------------------------------------------*/

static const func_table wifi_svc_dispatcher[] = {wlan_enable,
                                                 wlan_disable,
                                                 wlan_if_add,
                                                 start_scan,
                                                 join_ap,
                                                 wlan_disconnect,
                                                 wlan_pdev_utf,
                                                 wlan_set_mode,
                                                 wlan_set_pdev_param,
                                                 wlan_unit_test_cmd,
                                                 wlan_scan_stop_cmd,
                                                 wlan_twt_setup_cmd,
                                                 wlan_twt_teardown_cmd,
                                                 wlan_twt_status_cmd,
                                                 wlan_phydbgdump_cmd,
                                                 wlan_update_bi_cmd,
                                                 wlan_set_reset_wakelock_cmd,
                                                 wlan_periodic_tsf_sync_cmd,
                                                 wlan_f2a_on_wkup_config_cmd,
                                                 wlan_update_bmtt_cmd,
                                                 wlan_coex_evt_status,
                                                 wlan_periodic_traffic_setup_cmd,
                                                 wlan_periodic_traffic_status_cmd,
                                                 wlan_periodic_traffic_teardown_cmd,
                                                 wlan_clk_latency_cmd,
                                                 NULL};

/*
 * @brief  Validate the message id
 * @param  msg_id     : Config command id to be validated
 * @return bool       : TRUE if message id is valid
 *
 */
static NT_BOOL wifi_config_msg_id_validate(uint8_t msg_id)
{
    if (nt_get_app_mode() == APP_MODE_FTM) {
        switch (msg_id) {
            case WLAN_PDEV_UTF_CMD:
            case WLAN_MODE_CMD:
            case WLAN_UNIT_TEST_CMD:
            case WLAN_PHYDBGDUMP_CMD:
                return TRUE;
            default:
                return FALSE;
        }
    } else if (msg_id < WLAN_INVALID_CMD) {
        return TRUE;
    }

    return FALSE;
}

/*
 * @brief  Validate the network id
 * @param  msg_id     : Config command id to be validated
 * @param  msg_id     : Network id to be validated
 * @return bool       : TRUE if message id is valid
 *
 */
static NT_BOOL wlan_netif_id_validate(uint8_t msg_id, uint8_t netif_id)
{
    switch (msg_id) {
        case WLAN_PDEV_UTF_CMD:
        case WLAN_MODE_CMD:
        case WLAN_ENABLE_CMD:
        case WLAN_IF_ADD_CMD:
        case WLAN_DISABLE_CMD:
        case WLAN_SET_PDEV_PARAM_CMD:
        case WLAN_PHYDBGDUMP_CMD:
        case WLAN_UNIT_TEST_CMD:
            return TRUE;
    }

    if (NULL == netif_get_by_index(netif_id)) {
        return FALSE;
    }

    return TRUE;
}

/*
 * @brief  Get corresponding event id
 * @param  msg_id     : Config command id to be validated
 * @return bool       : TRUE if message id is valid
 *
 */
static uint8_t wlan_get_event_id_for_cmd(uint8_t msg_id)
{
    switch (msg_id) {
        case WLAN_ENABLE_CMD:
            return WLAN_ENABLE_EVT;
        case WLAN_DISABLE_CMD:
            return WLAN_DISABLE_EVT;
        case WLAN_IF_ADD_CMD:
            return WLAN_IF_ADD_COMP_EVT;
        case WLAN_SCAN_CMD:
            return WLAN_SCAN_COMP_EVT;
        case WLAN_JOIN_CMD:
            return WLAN_JOIN_COMP_EVT;
        case WLAN_DISCONNECT_CMD:
            return WLAN_DISCONNECT_EVT;
        case WLAN_PDEV_UTF_CMD:
            return WLAN_PDEV_UTF_EVT;
        case WLAN_MODE_CMD:
            return WLAN_MODE_EVT;
        case WLAN_SET_PDEV_PARAM_CMD:
            return WLAN_SET_PDEV_PARAMS_EVT;
        case WLAN_UNIT_TEST_CMD:
            return WLAN_UNIT_TEST_EVT;
        case WLAN_SCAN_STOP_CMD:
            return WLAN_SCAN_STOP_EVT;
        case WLAN_TWT_SETUP_CMD:
            return WLAN_TWT_SETUP_EVT;
        case WLAN_TWT_TEARDOWN_CMD:
            return WLAN_TWT_TEARDOWN_EVT;
        case WLAN_TWT_STATUS_CMD:
            return WLAN_TWT_STATUS_EVT;
        case WLAN_PHYDBGDUMP_CMD:
            return WLAN_PHYDBGDUMP_EVT;
        case WLAN_SET_RESET_WAKELOCK_CMD:
            return WLAN_SET_RESET_WAKELOCK_EVT;
        case WLAN_PERIODIC_TSF_SYNC_CMD:
            return WLAN_PERIODIC_TSF_SYNC_START_EVT;
        case WLAN_F2A_PULSE_ON_TWT_WAKEUP_CMD:
            return WLAN_F2A_PULSE_ON_TWT_WAKEUP_EVT;
        case WLAN_UPDATE_BMTT_CMD:
            return WLAN_UPDATE_BMTT_EVT;
        case WLAN_PERIODIC_TRAFFIC_SETUP_CMD:
            return WLAN_PERIODIC_TRAFFIC_SETUP_EVT;
        case WLAN_PERIODIC_TRAFFIC_STATUS_CMD:
            return WLAN_PERIODIC_TRAFFIC_STATUS_EVT;
        case WLAN_PERIODIC_TRAFFIC_TEARDOWN_CMD:
            return WLAN_PERIODIC_TRAFFIC_TEARDOWN_EVT;
        case WLAN_CLK_LATENCY_CMD:
            return WLAN_CLK_LATENCY_EVT;
        default:
            return WLAN_SET_PDEV_PARAMS_EVT;
    }
}

/*
 * @brief  Returns the length of the event
 * @param  event_id   : Event id
 * @return uint8_t    : Length of the event
 *
 */
static uint16_t wlan_get_config_evt_length(uint8_t event_id)
{
    switch (event_id) {
        case WLAN_ENABLE_EVT:
            return sizeof(wlan_enable_evt_t);
        case WLAN_DISABLE_EVT:
            return sizeof(wlan_disable_evt_t);
        case WLAN_IF_ADD_COMP_EVT:
            return sizeof(wlan_if_add_comp_evt_t);
        case WLAN_SCAN_COMP_EVT:
            return sizeof(wlan_scan_comp_evt_t);
        case WLAN_JOIN_COMP_EVT:
            return sizeof(wlan_join_comp_evt_t);
        case WLAN_DISCONNECT_EVT:
            return sizeof(wlan_disconnect_evt_t);
        case WLAN_PDEV_UTF_EVT:
            return sizeof(wlan_pdev_utf_evt_t);
        case WLAN_MODE_EVT:
            return sizeof(wlan_mode_evt_t);
        case WLAN_SET_PDEV_PARAMS_EVT:
            return sizeof(wlan_set_pdev_param_evt_t);
        case WLAN_UNIT_TEST_EVT:
            return sizeof(wlan_evt_hdr);
        case WLAN_SCAN_START_EVT:
            return sizeof(wlan_scan_start_evt_t);
        case WLAN_TWT_SETUP_EVT:
            return sizeof(wlan_twt_setup_evt_t);
        case WLAN_TWT_TEARDOWN_EVT:
            return sizeof(wlan_twt_teardown_evt_t);
        case WLAN_TWT_STATUS_EVT:
            return sizeof(wlan_twt_status_evt_t);
        case WLAN_UPDATE_BI_CMD:
            return sizeof(wlan_update_bi_evt_t);
        case WLAN_SET_RESET_WAKELOCK_EVT:
            return sizeof(wlan_set_reset_wakelock_evt_t);
        case WLAN_PERIODIC_TSF_SYNC_START_EVT:
            return sizeof(wlan_periodic_tsf_sync_start_evt_t);
        case WLAN_PERIODIC_TSF_SYNC_EVT:
            return sizeof(wlan_periodic_tsf_sync_evt_t);
        case WLAN_F2A_PULSE_ON_TWT_WAKEUP_EVT:
            return sizeof(wlan_f2a_pulse_on_twt_wakeup_evt_t);
        case WLAN_UPDATE_BMTT_EVT:
            return sizeof(wlan_update_bmtt_evt_t);
        case WLAN_PERIODIC_TRAFFIC_SETUP_EVT:
            return sizeof(wlan_periodic_traffic_setup_evt_t);
        case WLAN_PERIODIC_TRAFFIC_STATUS_EVT:
            return sizeof(wlan_periodic_traffic_status_evt_t);
        case WLAN_PERIODIC_TRAFFIC_TEARDOWN_EVT:
            return sizeof(wlan_periodic_traffic_teardown_evt_t);
        case WLAN_CLK_LATENCY_EVT:
            return sizeof(wlan_clk_latency_evt_t);
        default:
            return sizeof(wlan_evt_hdr);
    }
    return 0;
}

/*
 * @brief  Returns the length of the event
 * @param  event_id   : Event id
 * @return uint8_t    : Length of the event
 *
 */
static uint16_t wlan_get_config_cmd_length(uint8_t cmd_id)
{
    switch (cmd_id) {
        case WLAN_ENABLE_CMD:
            return sizeof(wlan_cmd_hdr);
        case WLAN_DISABLE_CMD:
            return sizeof(wlan_cmd_hdr);
        case WLAN_IF_ADD_CMD:
            return sizeof(wlan_if_add_cmd_t);
        case WLAN_SCAN_CMD:
            return sizeof(wlan_scan_cmd_t);
        case WLAN_JOIN_CMD:
            return sizeof(wlan_join_cmd_t);
        case WLAN_DISCONNECT_CMD:
            return sizeof(wlan_disconnect_cmd_t);
        case WLAN_PDEV_UTF_CMD:
            // return sizeof(wlan_pdev_utf_cmd_t);
            /* cmd len expected from Host is not same as entire UTF buf */
            return sizeof(wlan_cmd_hdr);
        case WLAN_MODE_CMD:
            return sizeof(wlan_mode_cmd_t);
        case WLAN_SET_PDEV_PARAM_CMD:
            return sizeof(wlan_set_pdev_param_cmd_t);
        case WLAN_UNIT_TEST_CMD:
            return sizeof(wlan_unit_test_cmd_t);
        case WLAN_SCAN_STOP_CMD:
            return sizeof(wlan_scan_stop_cmd_t);
        case WLAN_TWT_SETUP_CMD:
            return sizeof(wlan_twt_setup_cmd_t);
        case WLAN_TWT_TEARDOWN_CMD:
            return sizeof(wlan_twt_teardown_cmd_t);
        case WLAN_TWT_STATUS_CMD:
            return sizeof(wlan_twt_status_cmd_t);
        case WLAN_UPDATE_BI_CMD:
            return sizeof(wlan_update_bi_cmd_t);
        case WLAN_SET_RESET_WAKELOCK_CMD:
            return sizeof(wlan_set_reset_wakelock_cmd_t);
        case WLAN_PERIODIC_TSF_SYNC_CMD:
            return sizeof(wlan_periodic_tsf_sync_cmd_t);
        case WLAN_F2A_PULSE_ON_TWT_WAKEUP_CMD:
            return sizeof(wlan_f2a_pulse_on_twt_wakeup_cmd_t);
        case WLAN_UPDATE_BMTT_CMD:
            return sizeof(wlan_update_bmtt_cmd_t);
        case WLAN_PERIODIC_TRAFFIC_SETUP_CMD:
            return sizeof(wlan_periodic_traffic_setup_cmd_t);
        case WLAN_PERIODIC_TRAFFIC_STATUS_CMD:
            return sizeof(wlan_periodic_traffic_status_cmd_t);
        case WLAN_PERIODIC_TRAFFIC_TEARDOWN_CMD:
            return sizeof(wlan_periodic_traffic_teardown_cmd_t);
        case WLAN_CLK_LATENCY_CMD:
            return sizeof(wlan_clk_latency_cmd_t);
        default:
            return sizeof(wlan_cmd_hdr);
    }
}

/*
 * @brief  Validates length of the config command
 * @param  msg_id   : Config command id
 * @return bool     : True if length is as expected
 *
 */
static NT_BOOL wlan_cmd_length_validate(uint8_t msg_id, uint16_t length)
{
    if (length >= wlan_get_config_cmd_length(msg_id)) {
        return TRUE;
    }

    return FALSE;
}

/*
 * @brief  Send an event/response for the config command
 * @param  event_id   : Event id in the response frame
 * @param  network_id : Network id in the response frame
 * @param  rsp_status : Status in the response frame
 * @return none
 *
 */
static void wlan_send_config_rsp_err(uint8_t event_id, uint8_t network_id, uint8_t rsp_status)
{
    uint16_t len = wlan_get_config_evt_length(event_id);
    wlan_evt_hdr *event_hdr = (wlan_evt_hdr *)ringif_get_ctrl_buf(0, NULL);

    if (NULL == event_hdr) {
        NT_LOG_WMI_ERR("Memory allocation failed", 0, 0, 0);
        A_ASSERT(0);
    }
    event_hdr->msg_id = event_id;
    event_hdr->network_id = network_id;
    event_hdr->status = rsp_status;
    ringif_send_wifi_config((void *)event_hdr, len);
}

/*
 * @brief  Validate if the command received is in correct format
 * @param  buf      : Buffer pointer
 * @param  length   : Length of the config command
 * @return bool     : TRUE if message id is valid
 *
 */
static NT_BOOL wifi_config_cmd_validate(uint32_t *buf, uint16_t length)
{
    uint8_t validate_status = WIFI_STATUS_SUCCESS;
    wlan_cmd_hdr *hdr = (wlan_cmd_hdr *)buf;

    if (FALSE == wifi_config_msg_id_validate(hdr->msg_id)) {
        NT_LOG_PRINT(WFM, ERR, "RingIfErr WiFi: Invalid msg id:%d len: %d buf:%x\r\n", hdr->msg_id, length,
                     (uint32_t)buf);
        return FALSE;
    }

    if (FALSE == wlan_netif_id_validate(hdr->msg_id, hdr->network_id)) {
        validate_status = WIFI_STATUS_INVALID_NETIF;
        NT_LOG_PRINT(WFM, ERR, "RingIfErr WiFi: Invalid netif:%d msg id:%d len: %d\r\n", hdr->network_id, hdr->msg_id,
                     length);
    }

    if (FALSE == wlan_cmd_length_validate(hdr->msg_id, length)) {
        validate_status = WIFI_STATUS_INVALID_LEN;
        NT_LOG_PRINT(WFM, ERR, "RingIfErr WiFi: Invalid length: %d for msg_id:%d\r\n", length, hdr->msg_id);
    }

    if (validate_status != WIFI_STATUS_SUCCESS) {
        uint8_t event_id = wlan_get_event_id_for_cmd(hdr->msg_id);
        wlan_send_config_rsp_err(event_id, hdr->network_id, validate_status);
        return FALSE;
    } else {
        return TRUE;
    }
}

NT_BOOL process_wifi_config_pkt(uint32_t *buf, uint16_t length)
{
    if (length == 0 || !buf) {
        NT_LOG_PRINT(WFM, ERR, "RingIfErr in WiFi: Invalid buf:%x len:%d\r\n", (uint32_t)buf, length);
        ringif_free_ctrl_buf(buf, length);
        return TRUE;
    }

    uint32_t msg_id = ((wlan_cmd_hdr *)buf)->msg_id;
    if (FALSE == wifi_config_cmd_validate(buf, length)) {
        ringif_free_ctrl_buf(buf, length);
        return TRUE;
    } else {
        NT_LOG_PRINT(WFM, INFO, "RingIf WiFi: Config Command (buf: %x len:%d buf[0]:%x)", (uint32_t)buf, length,
                     (uint32_t)buf[0]);
        NT_LOG_PRINT(WFM, INFO, "RingIf WiFi: Config Command (buf[1]:%x buf[2]:%x buf[3]:%x)\r\n", buf[1], buf[2],
                     buf[3]);
    }

    if (NULL != wifi_svc_dispatcher[msg_id]) {
#ifdef SUPPORT_IMPS_IMPROVEMENTS
        g_ppm_common_struct.imps_struct_ctx.cmd_recv_timestamp = hres_timer_curr_time_us();
        if (g_ppm_common_struct.imps_struct_ctx.imps_registered == TRUE) {
            g_ppm_common_struct.imps_struct_ctx.imps_registered = FALSE;
            /*reload cnx wait timer when there is a WMI cmd before entring into IMPS */
            start_imps_cnx_wait_timer(g_ppm_common_struct.imps_struct_ctx.cmd_proc_wait_time_ms);
        }
#endif /* SUPPORT_IMPS_IMPROVEMENTS */
        WIFI_initiaize_wlan_cmd_var(TRUE, &Cmd_Translation_dummy);
        g_Cmd_Translation_wifi_hndl.msg_struct.is_ringif_cmd = TRUE;
        (*wifi_svc_dispatcher[msg_id])(buf, length);
    } else {
        NT_LOG_WFM_ERR("Msg id is not registered", 0, 0, 0);
        ringif_free_ctrl_buf(buf, length);
        return TRUE;
    }
    return TRUE;
}

/*--------------------------------------
 * Event Handlers
 *---------------------------------------*/

void wlan_enable_event(void *msg)
{
    uint16_t len = sizeof(wlan_enable_evt_t);
    wlan_enable_evt_t *enable_evt = (wlan_enable_evt_t *)ringif_get_ctrl_buf(0, NULL);
    if (NULL == enable_evt) {
        NT_LOG_WFM_ERR("Memory allocation failed", 0, 0, 0);
        A_ASSERT(0);
    }
    enable_evt->evt_hdr.msg_id = WLAN_ENABLE_EVT;
    enable_evt->evt_hdr.network_id = network_id;
    if (!msg) {
        enable_evt->evt_hdr.status = WIFI_STATUS_FALIURE;
        ringif_send_wifi_config((void *)enable_evt, len);
        NT_LOG_WFM_ERR("wlan_enable_event FAILED event sent\r\n", enable_evt->evt_hdr.status, 0, 0);
        return;
    }

    dev_common_t *dev_common = (dev_common_t *)msg;
    devh_t *dev = dev_common->devp[NT_DEV_AP_ID];
    if (dev == NULL) {
        enable_evt->evt_hdr.status = WIFI_STATUS_FALIURE;
        ringif_send_wifi_config((void *)enable_evt, len);
        NT_LOG_WFM_ERR("wlan_enable_event dev NULL FAIL event sent\r\n", (uint32_t)dev, enable_evt->evt_hdr.status, 0);
        return;
    }
    memcpy(enable_evt->mac_addr, dev->ic_myaddr, IEEE80211_ADDR_LENGTH);
    enable_evt->num_networks = dev->numConn;
    enable_evt->evt_hdr.status = WIFI_STATUS_SUCCESS;
    enable_evt->cap_info = dev->ic_flags;
    enable_evt->cap_info2 = dev->ic_flags2;
    NT_LOG_WFM_ERR("wlan_enable_event sent to ring \r\n", enable_evt->num_networks, enable_evt->cap_info, len);
    ringif_send_wifi_config((void *)enable_evt, len);
}

void wlan_disable_event(__unused void *msg)
{
    uint16_t len = sizeof(wlan_disable_evt_t);
    wlan_disable_evt_t *disable_evt = (wlan_disable_evt_t *)ringif_get_ctrl_buf(0, NULL);
    if (NULL == disable_evt) {
        NT_LOG_WFM_ERR("Memory allocation failed", 0, 0, 0);
        A_ASSERT(0);
    }
#ifdef SUPPORT_IMPS_IMPROVEMENTS
    /*reload cnx wait timer when there is disable cmd from aria) */
    start_imps_cnx_wait_timer(g_ppm_common_struct.imps_struct_ctx.recnx_wait_time_ms);
#endif /* SUPPORT_IMPS_IMPROVEMENTS */
    disable_evt->evt_hdr.status = WIFI_STATUS_SUCCESS;
    disable_evt->evt_hdr.msg_id = WLAN_DISABLE_EVT;
    disable_evt->evt_hdr.network_id = network_id;
    NT_LOG_WFM_ERR("wlan_disable_event sent to ring \r\n", 0, 0, 0);
    ringif_send_wifi_config((void *)disable_evt, len);
}

void wlan_if_add_event(void *msg)
{
    uint16_t len = sizeof(wlan_if_add_comp_evt_t);
    if (!msg) {
        NT_LOG_WFM_ERR("msg is NULL", 0, 0, 0);
        return;
    }
    wlan_if_add_comp_evt_t *if_comp_evt = (wlan_if_add_comp_evt_t *)ringif_get_ctrl_buf(0, NULL);
    if (NULL == if_comp_evt) {
        NT_LOG_WFM_ERR("Memory allocation failed", 0, 0, 0);
        A_ASSERT(0);
    }
    network_id = ((wifi_if_add_event *)msg)->net_id;

    if (0 == ((wifi_if_add_event *)msg)->status) {
        if_comp_evt->evt_hdr.status = WIFI_STATUS_SUCCESS;
    } else {
        if_comp_evt->evt_hdr.status = WIFI_STATUS_INVALID_ARGS;
    }
    if_comp_evt->evt_hdr.msg_id = WLAN_IF_ADD_COMP_EVT;
    if_comp_evt->evt_hdr.network_id = network_id;
    if_comp_evt->network_id = ((wifi_if_add_event *)msg)->net_id;
    NT_LOG_WFM_ERR("wlan_if_add_event sent to ring \r\n", if_comp_evt->network_id, (uint32_t)if_comp_evt, len);
    ringif_send_wifi_config((void *)if_comp_evt, len);
}

void wlan_scan_start_event(void *msg)
{
    uint16_t len = sizeof(wlan_scan_start_evt_t);
    if (!msg) {
        NT_LOG_WFM_ERR("msg is NULL", 0, 0, 0);
        return;
    }

    if ((g_Cmd_Translation_wifi_hndl.trans_wmi_message_id != WMI_START_SCAN_CMDID) &&
        (Cmd_Translation_wlan->trans_wmi_message_id != WMI_START_SCAN_CMDID)) {
        NT_LOG_PRINT(WFM, ERR, "Scan start event while %d command is in progress",
                     g_Cmd_Translation_wifi_hndl.trans_wmi_message_id, 0, 0);
        return;
    }

    if (((Cmd_Translation_wlan->trans_wmi_message_id == WMI_START_SCAN_CMDID) ||
         (Cmd_Translation_wlan->trans_wmi_message_id == WMI_UNIT_TEST_CMDID)) &&
        g_Cmd_Translation_wifi_hndl.msg_struct.is_ringif_cmd == FALSE) {
        extern nt_osal_semaphore_handle_t app_lib_semaphoreHandle;
        NT_LOG_WFM_INFO("Scan start event, giving back UT, SS semaphore", 0, 0, 0);
        if (nt_fail == nt_osal_semaphore_give(app_lib_semaphoreHandle)) {
            NT_LOG_WFM_ERR("semaphore give failed", 0, 0, 0);
        }
    }

    wlan_scan_start_evt_t *scan_start_evt = (wlan_scan_start_evt_t *)ringif_get_ctrl_buf(0, NULL);
    if (NULL == scan_start_evt) {
        NT_LOG_WFM_ERR("Memory allocation failed", 0, 0, 0);
        A_ASSERT(0);
    }
    scan_start_evt->evt_hdr.status = WIFI_STATUS_SUCCESS;
    scan_start_evt->evt_hdr.msg_id = WLAN_SCAN_START_EVT;
    scan_start_evt->evt_hdr.network_id = network_id;
    scan_start_evt->scan_id = *((uint8_t *)msg);
    NT_LOG_PRINT(WFM, ERR, "wlan_scan_start_event sent scan_id:%d status:%d\r\n", scan_start_evt->scan_id,
                 scan_start_evt->evt_hdr.status);
    ringif_send_wifi_config((void *)scan_start_evt, len);
}

void wlan_scan_stop_event(void *msg)
{
    uint16_t len = sizeof(wlan_scan_stop_evt_t);
    if (!msg) {
        NT_LOG_WFM_ERR("msg is NULL", 0, 0, 0);
        return;
    }
    wlan_scan_stop_evt_t *scan_stop_evt = (wlan_scan_stop_evt_t *)ringif_get_ctrl_buf(0, NULL);
    if (NULL == scan_stop_evt) {
        NT_LOG_WFM_ERR("Memory allocation failed", 0, 0, 0);
        A_ASSERT(0);
    }

    wifi_stop_event *evt_msg = (wifi_stop_event *)msg;
    scan_stop_evt->evt_hdr.status = evt_msg->status;
    scan_stop_evt->evt_hdr.msg_id = WLAN_SCAN_STOP_EVT;
    scan_stop_evt->evt_hdr.network_id = network_id;
    scan_stop_evt->scan_id = evt_msg->scan_id;
    NT_LOG_PRINT(WFM, ERR, "wlan_scan_stop_event sent scan_id:%d status:%d\r\n", scan_stop_evt->scan_id,
                 scan_stop_evt->evt_hdr.status);
    ringif_send_wifi_config((void *)scan_stop_evt, len);
}

void wlan_scan_comp_event(void *msg)
{
    uint8_t num_resp_events = 0;
    int rem_bss;
    uint16_t len = sizeof(wlan_scan_comp_evt_t);
    if (!msg) {
        NT_LOG_WFM_ERR("msg is null", 0, 0, 0);
        return;
    }
    SCAN_RESULT *buffer = (SCAN_RESULT *)msg;
    if (NULL == buffer) {
        NT_LOG_WFM_ERR("Scan Results Empty", 0, 0, 0);
        return;
    }
    rem_bss = buffer->num_entries;
    if (rem_bss > MAX_SCAN_SSID) {
        NT_LOG_WFM_ERR("Invalid number of bss", rem_bss, 0, 0);
        return;
    }

    while (rem_bss > 0 || (num_resp_events == 0)) {
        uint8_t count = 0, base = 0;
        wlan_scan_comp_evt_t *scan_comp_evt = (wlan_scan_comp_evt_t *)ringif_get_ctrl_buf(0, NULL);
        if (NULL == scan_comp_evt) {
            NT_LOG_WFM_ERR("Memory allocation failed", 0, 0, 0);
            A_ASSERT(0);
        }
        memset(scan_comp_evt, 0x0, sizeof(wlan_scan_comp_evt_t));
        scan_comp_evt->evt_hdr.msg_id = WLAN_SCAN_COMP_EVT;
        scan_comp_evt->evt_hdr.network_id = network_id;
        scan_comp_evt->evt_hdr.status = WIFI_STATUS_SUCCESS;
        scan_comp_evt->num_bss_curr_evt = (rem_bss > MAX_SCAN_RESULT) ? MAX_SCAN_RESULT : rem_bss;
        scan_comp_evt->scan_id = buffer->scan_id;
        if (rem_bss > MAX_SCAN_RESULT) {
            rem_bss -= MAX_SCAN_RESULT;
            scan_comp_evt->num_bss_rem = rem_bss;
        } else {
            rem_bss = 0;
            scan_comp_evt->num_bss_rem = rem_bss;
        }
        base = num_resp_events * MAX_SCAN_RESULT;
        for (count = 0; count < scan_comp_evt->num_bss_curr_evt; count++) {
            scan_comp_evt->scan_bss_info[count].channel = buffer->scan_bss_info[base + count].chan_freq;
            scan_comp_evt->scan_bss_info[count].security_mode =
                get_wifi_authmode_from_wmi_authmode(buffer->scan_bss_info[base + count].security_mode);
            scan_comp_evt->scan_bss_info[count].rssi = buffer->scan_bss_info[base + count].rssi;
            scan_comp_evt->scan_bss_info[count].wlan_mode = buffer->scan_bss_info[base + count].wlan_mode;
            memcpy(scan_comp_evt->scan_bss_info[count].bssid, buffer->scan_bss_info[base + count].bssid,
                   IEEE80211_ADDR_LENGTH);
            ((scan_comp_evt->scan_bss_info[count]).ssid).ssid_len =
                ((buffer->scan_bss_info[base + count]).ssid).ssid_len;
            memcpy(((scan_comp_evt->scan_bss_info[count]).ssid).ssid, (buffer->scan_bss_info[base + count].ssid).ssid,
                   ((buffer->scan_bss_info[base + count]).ssid).ssid_len);
        }
        ringif_send_wifi_config((void *)scan_comp_evt, len);
        NT_LOG_PRINT(WFM, ERR,
                     "wlan_scan_comp_event no.%d sent for scan_id:%d, networks:%d with len:%d rem_netwks:%d\r\n",
                     num_resp_events, scan_comp_evt->scan_id, scan_comp_evt->num_bss_curr_evt, len, rem_bss);
        num_resp_events++;
    }
}

void wlan_scan_fail_event(void *msg)
{
    uint16_t len = sizeof(wlan_scan_comp_evt_t);
    if (!msg) {
        NT_LOG_WFM_ERR("msg is null", 0, 0, 0);
        return;
    }

    if (g_Cmd_Translation_wifi_hndl.trans_wmi_message_id != WMI_START_SCAN_CMDID) {
        NT_LOG_PRINT(WFM, ERR, "Scan fail event while %d command is in progress",
                     g_Cmd_Translation_wifi_hndl.trans_wmi_message_id);
        return;
    }

    wlan_scan_comp_evt_t *scan_comp_evt = (wlan_scan_comp_evt_t *)ringif_get_ctrl_buf(0, NULL);
    if (NULL == scan_comp_evt) {
        NT_LOG_WFM_ERR("Memory allocation failed", 0, 0, 0);
        A_ASSERT(0);
    }
    scan_comp_evt->evt_hdr.msg_id = WLAN_SCAN_COMP_EVT;
    scan_comp_evt->evt_hdr.network_id = network_id;
    scan_comp_evt->num_bss_rem = 0;
    scan_comp_evt->evt_hdr.status = *((int8_t *)msg);
    scan_comp_evt->num_bss_curr_evt = 0;
    NT_LOG_PRINT(WFM, ERR, "wlan_scan_comp_event FAILED sent to ring \r\n");
    ringif_send_wifi_config((void *)scan_comp_evt, len);
}

void wlan_join_comp_event(void *msg)
{
    uint16_t len = sizeof(wlan_join_comp_evt_t);
    if (!msg) {
        NT_LOG_WFM_ERR("msg is NULL", 0, 0, 0);
        return;
    }
    wifi_join_event *conn = (wifi_join_event *)msg;
    wlan_join_comp_evt_t *join_evt = (wlan_join_comp_evt_t *)ringif_get_ctrl_buf(0, NULL);
    if (NULL == join_evt) {
        NT_LOG_WFM_ERR("Memory allocation failed", 0, 0, 0);
        A_ASSERT(0);
    }
    join_evt->evt_hdr.msg_id = WLAN_JOIN_COMP_EVT;
    join_evt->evt_hdr.network_id = network_id;
    if (conn->status == WIFI_STATUS_SUCCESS) {
        memcpy(join_evt->bssid, conn->bssid, IEEE80211_ADDR_LENGTH);
        (join_evt->ssid).ssid_len = (conn->ssid).ssid_len;
        memcpy(join_evt->ssid.ssid, conn->ssid.ssid, (join_evt->ssid).ssid_len);
        join_evt->evt_hdr.status = WIFI_STATUS_SUCCESS;
        join_evt->assoc_id = conn->assoc_id;
        join_evt->reason_code = conn->reason_code;
        join_evt->channel_frequency = conn->channel_frequency;
        NT_LOG_PRINT(WFM, ERR, "wlan_join_comp SUCCESS event sent aid:%d rsn:%d hosti:%d ssid_len:%d\r\n",
                     join_evt->assoc_id, join_evt->reason_code, conn->host_initiated, (join_evt->ssid).ssid_len);
    } else {
        join_evt->evt_hdr.status = WIFI_STATUS_FALIURE;
        join_evt->reason_code = conn->reason_code;
        join_evt->assoc_id = INVALID_ASSOC_ID;
        NT_LOG_PRINT(WFM, ERR, "wlan_join_comp FAILURE event sent aid:%d rsn:%d hosti:%d ssid_len:%d\r\n",
                     join_evt->assoc_id, join_evt->reason_code, conn->host_initiated, (join_evt->ssid).ssid_len);
    }
    if (conn->host_initiated)
        join_evt->host_initiated = 1;
    ringif_send_wifi_config((void *)join_evt, len);

    /* standby mode starts in hosted mode if IMPS INI enabled
       and configured in hosted mode */
#ifdef SUPPORT_IMPS_IMPROVEMENTS
    if ((!conn->status == WIFI_STATUS_SUCCESS) && g_ppm_common_struct.imps_struct_ctx.host_mode_configured) {
        /*reload cnx wait timer when there is any cnx failures) */
        start_imps_cnx_wait_timer(g_ppm_common_struct.imps_struct_ctx.recnx_wait_time_ms);
    }
#else
    if ((!conn->status == WIFI_STATUS_SUCCESS) && nt_imps_is_enabled_in_hosted_mode()) {
        nt_wpm_register_imps_standby();
    }
#endif /* SUPPORT_IMPS_IMPROVEMENTS */
}

void wlan_disconnect_event(void *msg)
{
    if (!msg) {
        NT_LOG_WFM_ERR("msg is NULL", 0, 0, 0);
        return;
    }
    wifi_disconnect_event *disconn_evt = (wifi_disconnect_event *)msg;
    uint16_t len = sizeof(wlan_disconnect_evt_t);
    wlan_disconnect_evt_t *disscon_comp_evt = (wlan_disconnect_evt_t *)ringif_get_ctrl_buf(0, NULL);
    if (NULL == disscon_comp_evt) {
        NT_LOG_WFM_ERR("Memory allocation failed", 0, 0, 0);
        A_ASSERT(0);
    }
    disscon_comp_evt->evt_hdr.msg_id = WLAN_DISCONNECT_EVT;
    disscon_comp_evt->evt_hdr.network_id = network_id;
    if (disconn_evt->assoc_id == INVALID_ASSOC_ID || disconn_evt->reason == BSS_DISCONNECTED) {
        disscon_comp_evt->evt_hdr.status = WIFI_STATUS_FALIURE;
    } else {
        disscon_comp_evt->evt_hdr.status = WIFI_STATUS_SUCCESS;
    }
    disscon_comp_evt->assoc_id = disconn_evt->assoc_id;
    if (disconn_evt->is_host_initiated) {
        disscon_comp_evt->host_initiated = 1;
    }
    disscon_comp_evt->reason_code = get_wifi_reason_from_wmi_reason(disconn_evt->reason);

    NT_LOG_PRINT(WFM, ERR, "wlan_disconnect evt aid:%d rsn:%d host_ini:%d status: %d \r\n", disscon_comp_evt->assoc_id,
                 disscon_comp_evt->reason_code, disscon_comp_evt->host_initiated, disscon_comp_evt->evt_hdr.status);

    ringif_send_wifi_config((void *)disscon_comp_evt, len);
#ifdef SUPPORT_IMPS_IMPROVEMENTS
    /*reload cnx wait timer when there is any WMI disconnection cmd */
    start_imps_cnx_wait_timer(g_ppm_common_struct.imps_struct_ctx.recnx_wait_time_ms);
#endif /* SUPPORT_IMPS_IMPROVEMENTS */
}

void wlan_set_param_event(void *msg)
{
    if (!msg) {
        NT_LOG_WFM_ERR("msg is NULL", 0, 0, 0);
        return;
    }
    SET_PDEV_PARAM_RESULT *buffer = (SET_PDEV_PARAM_RESULT *)msg;
    uint16_t len = sizeof(wlan_set_pdev_param_evt_t);
    wlan_set_pdev_param_evt_t *set_param_evt = (wlan_set_pdev_param_evt_t *)ringif_get_ctrl_buf(0, NULL);
    if (NULL == set_param_evt) {
        NT_LOG_WFM_ERR("Memory allocation failed", 0, 0, 0);
        A_ASSERT(0);
    }
    set_param_evt->evt_hdr.msg_id = WLAN_SET_PDEV_PARAMS_EVT;
    set_param_evt->evt_hdr.network_id = network_id;
    set_param_evt->evt_hdr.status = buffer->status;
    set_param_evt->param_id = buffer->param_id;

    NT_LOG_PRINT(WFM, ERR, "wlan_set_pdev_params evt \r\n");
    ringif_send_wifi_config((void *)set_param_evt, len);
}

/*-----------------------------
 * Helper functions
 *------------------------------*/
void send_wifi_disc_event(uint16_t associd, WMI_DISCONNECT_REASON reason)
{
    if ((g_Cmd_Translation_wifi_hndl.msg_struct.event_notify) &&
        (WMI_WLAN_OFF_CMDID != g_Cmd_Translation_wifi_hndl.trans_wmi_message_id)) {
        wifi_disconnect_event disconn_evt = {0};

        if (WMI_DISCONNECT_CMDID == g_Cmd_Translation_wifi_hndl.trans_wmi_message_id) {
            struct netif *p_netif = NULL;
            disconn_evt.is_host_initiated = TRUE;
            p_netif = netif_get_by_index(g_Cmd_Translation_wifi_hndl.msg_struct.netif_id);

            /* If Disconnection was requested by Host, update netif status as well */
            /* TBD for future: Network needs to be set to down only for STA Mode disconnection */
            // if(STA Mode)
            nt_dpm_netif_set_link_down(p_netif);
        }
        disconn_evt.assoc_id = associd;
        disconn_evt.reason = reason;
        g_Cmd_Translation_wifi_hndl.msg_struct.event_notify(eWiFiSuccess, disconnect_event_id, &disconn_evt);
    }
}

void send_wifi_cnx_event(void *conn, WMI_DISCONNECT_REASON reason, NT_BOOL cnx_sucess)
{
    extern wmi_msg_struct_t g_Cmd_Translation_wifi_hndl;
    // send connect event to both of hostif and QAPI if valid
    // if(g_Cmd_Translation_wifi_hndl.msg_struct.event_notify)
    // printf("connect event reason=%d status=%d\n", reason, cnx_sucess);
    if (1) {
        event_t event;
        wifi_join_event join_evt;
        memset(&join_evt, 0, sizeof(join_evt));

        if (g_Cmd_Translation_wifi_hndl.trans_wmi_message_id == WMI_CONNECT_CMDID) {
            join_evt.host_initiated = TRUE;

            // Make the global variable invalid.
            g_Cmd_Translation_wifi_hndl.trans_wmi_message_id = WMI_CMD_MAX;
        } else {
            join_evt.host_initiated = FALSE;
        }

        if (cnx_sucess) {
            event = cnx_success_event_id;
            join_evt.status = WIFI_STATUS_SUCCESS;
            join_evt.reason_code = reason;
        } else {
            event = cnx_failure_event_id;
            join_evt.status = WIFI_STATUS_FALIURE;
            join_evt.reason_code = get_wifi_reason_from_wmi_reason(reason);
        }

        if (NULL == conn) {
            if (g_Cmd_Translation_wifi_hndl.msg_struct.event_notify) {
                g_Cmd_Translation_wifi_hndl.msg_struct.event_notify(eWiFiSuccess, event, &join_evt);
            }

            return;
        }

        devh_t *dev = ((conn_t *)conn)->pDev;
        channel_t *ch = co_get_current_channel(dev);
        join_evt.channel_frequency = ch->ch_halchan.channel;
        join_evt.assoc_id = ((conn_t *)conn)->ni_associd;
        join_evt.ssid.ssid_len = (((conn_t *)conn)->pBss->ni_ssid).ssid_len;

        if ((join_evt.ssid.ssid_len > 0) && (join_evt.ssid.ssid_len < MAX_SSID_LEN)) {
            memcpy(join_evt.ssid.ssid, (((conn_t *)conn)->pBss->ni_ssid).ssid, join_evt.ssid.ssid_len);
        } else if (join_evt.ssid.ssid_len > MAX_SSID_LEN) {
            NT_LOG_WFM_ERR("send_wifi_cnx_event: Invalid SSID", join_evt.ssid.ssid_len, 0, 0);
            join_evt.ssid.ssid_len = 0;
        }

        memcpy(join_evt.bssid, ((conn_t *)conn)->pBss->ni_bssid, IEEE80211_ADDR_LENGTH);

        if (g_Cmd_Translation_wifi_hndl.msg_struct.event_notify) {
            g_Cmd_Translation_wifi_hndl.msg_struct.event_notify(eWiFiSuccess, event, &join_evt);
        }
    }
}

/*
 * @brief FTM / MM mode switch event sent
 * @param msg - unused
 * @return none
 *
 */
void wlan_mode_event(void *msg)
{
    (void)msg;
    uint16_t len = sizeof(wlan_mode_evt_t);
    wlan_mode_evt_t *phy_mode_evt = (wlan_mode_evt_t *)ringif_get_ctrl_buf(0, NULL);
    if (NULL == phy_mode_evt) {
        NT_LOG_WFM_ERR("Memory allocation failed", 0, 0, 0);
        A_ASSERT(0);
    }
    phy_mode_evt->evt_hdr.msg_id = WLAN_MODE_EVT;
    phy_mode_evt->evt_hdr.network_id = network_id;
    phy_mode_evt->actual_mode = nt_get_app_mode();

    NT_LOG_PRINT(WFM, ERR, "wlan_mode evt \r\n");
    ringif_send_wifi_config((void *)phy_mode_evt, len);
    return;
}

/*
 * @brief FTM / MM mode switch event sent after boot is done
 * @param none
 * @return none
 *
 */
void wlan_send_mode_event(void)
{
    extern wmi_msg_struct_t g_Cmd_Translation_wifi_hndl;
    if (g_Cmd_Translation_wifi_hndl.msg_struct.event_notify) {
        NT_LOG_WFM_ERR("sending mode switch event", nt_get_app_mode(), 0, 0);
        g_Cmd_Translation_wifi_hndl.msg_struct.event_notify(eWiFiSuccess, set_mode_event_id, NULL);
    }
    return;
}

uint8_t get_wifi_authmode_from_wmi_authmode(AUTH_MODE authmode)
{
    uint8_t wifi_authmode;
    switch (authmode) {
        case WMI_NONE_AUTH:
            wifi_authmode = eWiFiSecurityOpen;
            break;
        case WMI_WPA_AUTH:
        case WMI_WPA_PSK_AUTH:
        case WMI_WPA_AUTH_CCKM:
            wifi_authmode = eWiFiSecurityWPA;
            break;
        case WMI_WPA2_AUTH:
        case WMI_WPA2_PSK_AUTH:
        case WMI_WPA2_AUTH_CCKM:
            wifi_authmode = eWiFiSecurityWPA2;
            break;
        case WMI_WPA3_SHA256_AUTH:
            wifi_authmode = eWiFiSecurityWPA3;
            break;
        default:
            wifi_authmode = eWiFiSecurityNotSupported;
            break;
    }
    return wifi_authmode;
}

uint8_t get_wifi_reason_from_wmi_reason(WMI_DISCONNECT_REASON reason)
{
    uint8_t wifi_reason;
    switch (reason) {
        case NO_NETWORK_AVAIL:
            wifi_reason = WIFI_NO_NETWORK_AVAIL;
            break;
        case LOST_LINK:
            wifi_reason = WIFI_LOST_LINK;
            break;
        case DISCONNECT_CMD:
            wifi_reason = WIFI_DISCONNECT_CMD;
            break;
        case BSS_DISCONNECTED:
            wifi_reason = WIFI_BSS_DISCONNECTED;
            break;
        case AUTH_FAILED:
            wifi_reason = WIFI_AUTH_FAILED;
            break;
        case ASSOC_FAILED:
            wifi_reason = WIFI_ASSOC_FAILED;
            break;
        case SEC_HS_TO_RECV_M1:
            wifi_reason = WIFI_SEC_HS_TO_RECV_M1;
            break;
        case SEC_HS_TO_RECV_M3:
            wifi_reason = WIFI_SEC_HS_TO_RECV_M3;
            break;
        case MIC_FAILURE_4WAY_HANDSHAKE:
            wifi_reason = WIFI_IEEE80211_REASON_MIC_FAILURE;
            break;
        case RECEIVED_DISASSOC:
            wifi_reason = WIFI_RECEIVED_DISASSOC;
            break;
        case RECEIVED_DEAUTH:
            wifi_reason = WIFI_RECEIVED_DEAUTH;
            break;
        case CHANNEL_SWITCH_SUCCESS:
            wifi_reason = WIFI_ECSA_SUCCESS;
            break;
        case CHANNEL_SWITCH_FAILED:
            wifi_reason = WIFI_ECSA_FAILURE;
            break;
        default:
            wifi_reason = WIFI_REASON_UNKNOWN;
    }
    return wifi_reason;
}

uint8_t get_wifi_phymode_from_wmi_phymode(WLAN_PHY_MODE phymode)
{
    uint8_t wlan_phymode;
    switch (phymode) {
        case WLAN_MODE_11B:
            wlan_phymode = MODE_11B;
            break;
        case WLAN_MODE_11G:
            wlan_phymode = MODE_11G;
            break;
        case WLAN_MODE_11NG_HT20:
            wlan_phymode = MODE_11NG_HT20;
            break;
        case WLAN_MODE_11A_ONLY:
            wlan_phymode = MODE_11A_ONLY;
            break;
        case WLAN_MODE_11A_HT20:
            wlan_phymode = MODE_11A_HT20;
            break;
        case WLAN_MODE_11ABGN_HT20:
            wlan_phymode = MODE_11ABGN_HT20;
            break;
        case WLAN_MODE_UNKNOWN:
            wlan_phymode = MODE_UNKNOWN;
            break;
        default:
            wlan_phymode = WLAN_MODE_MAX;
    }
    return wlan_phymode;
}

void wlan_unit_test_event(__unused void *msg)
{
#ifdef SUPPORT_UNIT_TEST_CMD
    uint16_t len = sizeof(wlan_unit_test_evt_t);
    if (!msg) {
        NT_LOG_WFM_ERR("msg is NULL", 0, 0, 0);
        return;
    }
    wifi_unit_test_event *utest_evt = (wifi_unit_test_event *)msg;
    wlan_unit_test_evt_t *ut_evt = (wlan_unit_test_evt_t *)ringif_get_ctrl_buf(0, NULL);
    if (NULL == ut_evt) {
        NT_LOG_WFM_ERR("Memory allocation failed", 0, 0, 0);
        A_ASSERT(0);
    }
    ut_evt->evt_hdr.msg_id = WLAN_UNIT_TEST_EVT;
    ut_evt->evt_hdr.network_id = network_id;
    if (utest_evt->status == WIFI_STATUS_SUCCESS)
        ut_evt->evt_hdr.status = WIFI_STATUS_SUCCESS;
    else
        ut_evt->evt_hdr.status = WIFI_STATUS_FALIURE;

    NT_LOG_PRINT(WFM, ERR, "wlan_unit_test evt \r\n");

    ringif_send_wifi_config((void *)ut_evt, len);
#endif
}

void wlan_twt_setup_event(__unused void *msg)
{
#ifdef SUPPORT_TWT_STA
    uint16_t len = sizeof(wlan_twt_setup_evt_t);
    if (!msg) {
        NT_LOG_WFM_ERR("msg is NULL", 0, 0, 0);
        return;
    }
    wlan_twt_setup_evt_t *twt_setup_evt = (wlan_twt_setup_evt_t *)ringif_get_ctrl_buf(0, NULL);
    if (NULL == twt_setup_evt) {
        NT_LOG_WFM_ERR("Memory allocation failed", 0, 0, 0);
        A_ASSERT(0);
    }
    memscpy(twt_setup_evt, sizeof(wlan_twt_setup_evt_t), msg, len);
    twt_setup_evt->evt_hdr.msg_id = WLAN_TWT_SETUP_EVT;
    twt_setup_evt->evt_hdr.network_id = network_id;
    twt_setup_evt->evt_hdr.status =
        (twt_setup_evt->reason_code != WLAN_TWT_EVT_STATUS_OK) ? WIFI_STATUS_FALIURE : WIFI_STATUS_SUCCESS;
    NT_LOG_WFM_ERR("wlan_twt_setup_event status", twt_setup_evt->evt_hdr.status, 0, 0);
    ringif_send_wifi_config((void *)twt_setup_evt, len);
    nt_release_twt_sleep_lock();
#endif  // SUPPORT_TWT_STA
}

void wlan_twt_teardown_event(__unused void *msg)
{
#ifdef SUPPORT_TWT_STA
    uint16_t len = sizeof(wlan_twt_teardown_evt_t);
    if (!msg) {
        NT_LOG_WFM_ERR("msg is NULL", 0, 0, 0);
        return;
    }
    wlan_twt_teardown_evt_t *twt_teardown_evt = (wlan_twt_teardown_evt_t *)ringif_get_ctrl_buf(0, NULL);
    if (NULL == twt_teardown_evt) {
        NT_LOG_WFM_ERR("Memory allocation failed", 0, 0, 0);
        A_ASSERT(0);
    }

    memscpy(twt_teardown_evt, sizeof(wlan_twt_teardown_evt_t), msg, len);
    twt_teardown_evt->evt_hdr.msg_id = WLAN_TWT_TEARDOWN_EVT;
    twt_teardown_evt->evt_hdr.network_id = network_id;
    twt_teardown_evt->evt_hdr.status =
        (twt_teardown_evt->reason_code != WLAN_TWT_EVT_STATUS_OK) ? WIFI_STATUS_FALIURE : WIFI_STATUS_SUCCESS;

    NT_LOG_PRINT(WFM, ERR, "wlan_twt_teardown evt \r\n");
    ringif_send_wifi_config((void *)twt_teardown_evt, len);
#endif  // SUPPORT_TWT_STA
}

void wlan_twt_status_event(__unused void *msg)
{
#ifdef SUPPORT_TWT_STA
    uint16_t len = sizeof(wlan_twt_status_evt_t);
    if (!msg) {
        NT_LOG_WFM_ERR("msg is NULL", 0, 0, 0);
        return;
    }
    wlan_twt_status_evt_t *twt_status_evt = (wlan_twt_status_evt_t *)ringif_get_ctrl_buf(0, NULL);
    if (NULL == twt_status_evt) {
        NT_LOG_WFM_ERR("Memory allocation failed", 0, 0, 0);
        A_ASSERT(0);
    }

    memscpy(twt_status_evt, sizeof(wlan_twt_status_evt_t), msg, len);
    twt_status_evt->evt_hdr.msg_id = WLAN_TWT_STATUS_EVT;
    twt_status_evt->evt_hdr.network_id = network_id;
    twt_status_evt->evt_hdr.status =
        (twt_status_evt->reason_code != WLAN_TWT_EVT_STATUS_OK) ? WIFI_STATUS_FALIURE : WIFI_STATUS_SUCCESS;

    NT_LOG_PRINT(WFM, ERR, "wlan_twt_status evt \r\n");

    ringif_send_wifi_config((void *)twt_status_evt, len);
#endif  // SUPPORT_TWT_STA
}

void wlan_phydbgdump_event(void *msg)
{
    (void)msg;
    uint16_t len = sizeof(wlan_phydbgdump_evt_t);

    wlan_phydbgdump_evt_t *wlan_phydbgdump_evt = (wlan_phydbgdump_evt_t *)ringif_get_ctrl_buf(0, NULL);
    if (NULL == wlan_phydbgdump_evt) {
        NT_LOG_WFM_ERR("Memory allocation failed", 0, 0, 0);
        A_ASSERT(0);
    }

#ifdef PHY_POWER_SWITCH
    hal_phy_power_switch_to_cfg();
#endif
    dbgmem_read_capture_mem((uint32_t *)phydbg_data, 1024, 0, false, 0);
#ifdef PHY_POWER_SWITCH
    hal_phy_power_switch_to_listen();
#endif

    wlan_phydbgdump_evt->evt_hdr.msg_id = WLAN_PHYDBGDUMP_EVT;
    wlan_phydbgdump_evt->evt_hdr.network_id = network_id;
    wlan_phydbgdump_evt->evt_hdr.status = WIFI_STATUS_SUCCESS;
    wlan_phydbgdump_evt->address = (uint32_t)phydbg_data;
    wlan_phydbgdump_evt->length = PHYDBG_BUFFER_SIZE;

    NT_LOG_PRINT(WFM, ERR, "wlan_phydbgdump evt \r\n");

    ringif_send_wifi_config((void *)wlan_phydbgdump_evt, len);
}

void wlan_update_bi_event(__unused void *msg)
{
#ifdef SUPPORT_SAP_POWERSAVE
    uint16_t len = sizeof(wlan_update_bi_evt_t);
    wlan_update_bi_evt_t *update_bi_evt = (wlan_update_bi_evt_t *)ringif_get_ctrl_buf(0, NULL);
    if (NULL == update_bi_evt) {
        NT_LOG_WFM_ERR("Memory allocation failed", 0, 0, 0);
        A_ASSERT(0);
    }
    update_bi_evt->evt_hdr.msg_id = WLAN_UPDATE_BI_EVT;
    update_bi_evt->evt_hdr.network_id = network_id;
    update_bi_evt->evt_hdr.status = (*((nt_status_t *)msg) == NT_OK) ? WIFI_STATUS_SUCCESS : WIFI_STATUS_FALIURE;

    NT_LOG_PRINT(WFM, ERR, "wlan_update_bi evt \r\n");
    ringif_send_wifi_config((void *)update_bi_evt, len);
#endif
}

/*
 * @brief bmtt configuration response event
 * @param msg - status of the received command
 * @return none
 *
 */
void wlan_update_bmtt_event(__unused void *msg)
{
#ifdef SUPPORT_BEACON_MISS_THRESHOLD_TIME
    uint16_t len = sizeof(wlan_update_bmtt_evt_t);
    wlan_update_bmtt_evt_t *update_bmtt_evt = (wlan_update_bmtt_evt_t *)ringif_get_ctrl_buf(0, NULL);
    if (NULL == update_bmtt_evt) {
        NT_LOG_WFM_ERR("Memory allocation failed", 0, 0, 0);
        A_ASSERT(0);
    }
    update_bmtt_evt->evt_hdr.msg_id = WLAN_UPDATE_BMTT_EVT;
    update_bmtt_evt->evt_hdr.network_id = network_id;
    update_bmtt_evt->evt_hdr.status = (*((nt_status_t *)msg) == NT_OK) ? WIFI_STATUS_SUCCESS : WIFI_STATUS_FALIURE;

    NT_LOG_PRINT(WFM, ERR, "wlan_update_bmtt evt \r\n");
    ringif_send_wifi_config((void *)update_bmtt_evt, len);
#endif /* SUPPORT_BEACON_MISS_THRESHOLD_TIME */
}

void wlan_set_reset_wakelock_event(void *msg)
{
    uint16_t len = sizeof(wlan_set_reset_wakelock_evt_t);
    wlan_set_reset_wakelock_evt_t *set_reset_wakelock_evt =
        (wlan_set_reset_wakelock_evt_t *)ringif_get_ctrl_buf(0, NULL);
    if (NULL == set_reset_wakelock_evt) {
        NT_LOG_WFM_ERR("Memory allocation failed", 0, 0, 0);
        A_ASSERT(0);
    }

    set_reset_wakelock_evt->evt_hdr.status =
        (((wlan_set_reset_wakelock_evt_t *)msg)->evt_hdr.status == NT_OK) ? WIFI_STATUS_SUCCESS : WIFI_STATUS_FALIURE;
    set_reset_wakelock_evt->evt_hdr.msg_id = WLAN_SET_RESET_WAKELOCK_EVT;
    set_reset_wakelock_evt->evt_hdr.network_id = network_id;
    set_reset_wakelock_evt->wakelock_event_type = ((wlan_set_reset_wakelock_evt_t *)msg)->wakelock_event_type;

    NT_LOG_PRINT(WFM, ERR, "wlan_set_reset_wakelock evt \r\n");
    ringif_send_wifi_config((void *)set_reset_wakelock_evt, len);
}

void wlan_periodic_tsf_sync_start_event(__unused void *msg)
{
#ifdef SUPPORT_PERIODIC_TSF_SYNC
    uint16_t len = sizeof(wlan_periodic_tsf_sync_start_evt_t);
    wlan_periodic_tsf_sync_start_evt_t *tsf_sync_start_evt =
        (wlan_periodic_tsf_sync_start_evt_t *)ringif_get_ctrl_buf(0, NULL);
    if (NULL == tsf_sync_start_evt) {
        NT_LOG_WFM_ERR("Memory allocation failed", 0, 0, 0);
        A_ASSERT(0);
    }
    tsf_sync_start_evt->evt_hdr.status = WIFI_STATUS_SUCCESS;
    tsf_sync_start_evt->evt_hdr.msg_id = WLAN_PERIODIC_TSF_SYNC_START_EVT;
    tsf_sync_start_evt->evt_hdr.network_id = network_id;

    NT_LOG_PRINT(WFM, ERR, "wlan_periodic_tsf_sync_start evt \r\n");
    ringif_send_wifi_config((void *)tsf_sync_start_evt, len);
#endif
}

void wlan_periodic_tsf_sync_event(__unused void *msg)
{
#ifdef SUPPORT_PERIODIC_TSF_SYNC
    if (!msg) {
        NT_LOG_WFM_ERR("msg is NULL", 0, 0, 0);
        return;
    }

    uint16_t len = sizeof(wlan_periodic_tsf_sync_evt_t);
    wlan_periodic_tsf_sync_evt_t *wlan_tsf_evt = (wlan_periodic_tsf_sync_evt_t *)ringif_get_ctrl_buf(0, NULL);

    if (NULL == wlan_tsf_evt) {
        NT_LOG_WFM_ERR("Memory allocation failed", 0, 0, 0);
        A_ASSERT(0);
    }

    wlan_tsf_evt->evt_hdr.msg_id = WLAN_PERIODIC_TSF_SYNC_EVT;
    wlan_tsf_evt->evt_hdr.network_id = network_id;
    wlan_tsf_evt->evt_hdr.status = WIFI_STATUS_SUCCESS;
    wlan_tsf_evt->tsf_lo = ((tsf_periodic_sync *)msg)->updated_tsf_lo;
    wlan_tsf_evt->tsf_hi = ((tsf_periodic_sync *)msg)->updated_tsf_hi;

    NT_LOG_PRINT(WFM, INFO, "wlan_periodic_tsf_sync evt \r\n");
    ringif_send_wifi_config((void *)wlan_tsf_evt, len);
#endif  // SUPPORT_PERIODIC_TSF_SYNC
}

/**
 *@brief Send f2a_pulse_on_twt_wakeup evt to APPS
 *
 * This function is called to send WLAN_F2A_PULSE_ON_TWT_WAKEUP_EVT event to
 * APPS. It converts the WMI structrure to APPS structure and triggers the
 * event.
 *
 * @param msg: contains event specific data to send to APPS
 * @return   : NONE
 */
void wlan_f2a_on_wakeup_config_event(__unused void *msg)
{
#ifdef FIRMWARE_APPS_INFORMED_WAKE
    uint16_t len = sizeof(wlan_f2a_pulse_on_twt_wakeup_evt_t);
    wlan_f2a_pulse_on_twt_wakeup_evt_t *f2a_pulse_on_twt_wakeup_evt =
        (wlan_f2a_pulse_on_twt_wakeup_evt_t *)ringif_get_ctrl_buf(0, NULL);
    if (NULL == f2a_pulse_on_twt_wakeup_evt) {
        NT_LOG_WFM_ERR("Memory allocation failed", 0, 0, 0);
        A_ASSERT(0);
    }

    f2a_pulse_on_twt_wakeup_evt->evt_hdr.status = (((wlan_f2a_pulse_on_twt_wakeup_evt_t *)msg)->evt_hdr.status == NT_OK)
                                                      ? WIFI_STATUS_SUCCESS
                                                      : WIFI_STATUS_FALIURE;
    f2a_pulse_on_twt_wakeup_evt->evt_hdr.msg_id = WLAN_F2A_PULSE_ON_TWT_WAKEUP_EVT;
    f2a_pulse_on_twt_wakeup_evt->evt_hdr.network_id = network_id;
    f2a_pulse_on_twt_wakeup_evt->status = ((wlan_f2a_pulse_on_twt_wakeup_evt_t *)msg)->status;

    NT_LOG_PRINT(WFM, ERR, "wlan_f2a_pulse_on_twt_wakeup evt \r\n");
    ringif_send_wifi_config((void *)f2a_pulse_on_twt_wakeup_evt, len);
#endif /* FIRMWARE_APPS_INFORMED_WAKE */
}

void wlan_coex_event(__unused void *msg)
{
#ifdef SUPPORT_COEX
    NT_LOG_WFM_INFO("wlan_coex_event", 0, 0, 0);
    uint16_t len = sizeof(wlan_cxc_evt_t);
    wlan_cxc_evt_t *coex_evt = (wlan_cxc_evt_t *)ringif_get_ctrl_buf(0, NULL);
    if (NULL == coex_evt) {
        NT_LOG_WFM_ERR("Memory allocation failed", 0, 0, 0);
        A_ASSERT(0);
    }

    coex_evt->evt_hdr.status =
        (((wlan_cxc_evt_t *)msg)->evt_hdr.status == NT_OK) ? WIFI_STATUS_SUCCESS : WIFI_STATUS_FALIURE;
    coex_evt->evt_hdr.msg_id = WLAN_COEX_EVT;
    coex_evt->evt_hdr.network_id = network_id;
    coex_evt->event_type = ((wlan_cxc_evt_t *)msg)->event_type;
#ifndef EMULATION_BUILD
    nt_start_timer(gpBtCoexWlanInfoDev->BtRespCXCResetTimer);
#endif /* EMULATION_BUILD */

    NT_LOG_PRINT(WFM, ERR, "wlan_coex evt \r\n");
    ringif_send_wifi_config((void *)coex_evt, len);
#endif /* SUPPORT_COEX */
}

/*
 * @brief this function sends periodic traffic setup event.
 * @param msg: It contains wake interval, first_sp_start_tsf, traffic_type and session_id.
 * @return This function does not return anything.
 */
void wlan_periodic_traffic_setup_event(__unused void *msg)
{
#ifdef FEATURE_PERIODIC_WAKE_SLEEP
    uint16_t len = sizeof(wlan_periodic_traffic_setup_evt_t);
    wlan_periodic_traffic_setup_evt_t *traffic_setup_evt =
        (wlan_periodic_traffic_setup_evt_t *)ringif_get_ctrl_buf(0, NULL);
    if (NULL == traffic_setup_evt) {
        NT_LOG_WMI_ERR("Memory allocation failed", 0, 0, 0);
        A_ASSERT(0);
    }

    traffic_setup_evt->evt_hdr.status = (((wlan_periodic_traffic_setup_evt_t *)msg)->evt_hdr.status == NT_OK)
                                            ? WIFI_STATUS_SUCCESS
                                            : WIFI_STATUS_FALIURE;
    traffic_setup_evt->evt_hdr.msg_id = WLAN_PERIODIC_TRAFFIC_SETUP_EVT;
    traffic_setup_evt->evt_hdr.network_id = network_id;
    traffic_setup_evt->wake_interval = ((wlan_periodic_traffic_setup_evt_t *)msg)->wake_interval;
    traffic_setup_evt->start_tsf_lo = ((wlan_periodic_traffic_setup_evt_t *)msg)->start_tsf_lo;
    traffic_setup_evt->start_tsf_hi = ((wlan_periodic_traffic_setup_evt_t *)msg)->start_tsf_hi;
    traffic_setup_evt->traffic_type = ((wlan_periodic_traffic_setup_evt_t *)msg)->traffic_type;
    traffic_setup_evt->session_id = ((wlan_periodic_traffic_setup_evt_t *)msg)->session_id;
    traffic_setup_evt->status = ((wlan_periodic_traffic_setup_evt_t *)msg)->status;
    ringif_send_wifi_config((void *)traffic_setup_evt, len);
#else
    NT_LOG_WMI_ERR("WLAN_PERIODIC_TRAFFIC_SETUP_EVT not supported", 0, 0, 0);
#endif /* FEATURE_PERIODIC_WAKE_SLEEP */
}

/*
 * @brief this function sends periodic traffic status event.
 * @param msg: It contains wake interval, next_sp_start_tsf, traffic_type and session_id.
 * @return This function does not return anything.
 */
void wlan_periodic_traffic_status_event(__unused void *msg)
{
#ifdef FEATURE_PERIODIC_WAKE_SLEEP
    uint16_t len = sizeof(wlan_periodic_traffic_status_evt_t);
    wlan_periodic_traffic_status_evt_t *traffic_status_evt =
        (wlan_periodic_traffic_status_evt_t *)ringif_get_ctrl_buf(0, NULL);
    if (NULL == traffic_status_evt) {
        NT_LOG_WMI_ERR("Memory allocation failed", 0, 0, 0);
        A_ASSERT(0);
    }

    traffic_status_evt->evt_hdr.status = (((wlan_periodic_traffic_status_evt_t *)msg)->evt_hdr.status == NT_OK)
                                             ? WIFI_STATUS_SUCCESS
                                             : WIFI_STATUS_FALIURE;
    traffic_status_evt->evt_hdr.msg_id = WLAN_PERIODIC_TRAFFIC_STATUS_EVT;
    traffic_status_evt->evt_hdr.network_id = network_id;
    traffic_status_evt->wake_interval = ((wlan_periodic_traffic_status_evt_t *)msg)->wake_interval;
    traffic_status_evt->next_sp_tsf_lo = ((wlan_periodic_traffic_status_evt_t *)msg)->next_sp_tsf_lo;
    traffic_status_evt->next_sp_tsf_hi = ((wlan_periodic_traffic_status_evt_t *)msg)->next_sp_tsf_hi;
    traffic_status_evt->traffic_type = ((wlan_periodic_traffic_status_evt_t *)msg)->traffic_type;
    traffic_status_evt->session_id = ((wlan_periodic_traffic_status_evt_t *)msg)->session_id;
    ringif_send_wifi_config((void *)traffic_status_evt, len);
#else
    NT_LOG_WMI_ERR("WLAN_PERIODIC_TRAFFIC_STATUS_EVT not supported", 0, 0, 0);
#endif /* FEATURE_PERIODIC_WAKE_SLEEP */
}

/*
 * @brief this function sends periodic traffic teardown event.
 * @param msg: It contains teardown_reason, traffic_type and session_id.
 * @return This function does not return anything.
 */
void wlan_periodic_traffic_teardown_event(__unused void *msg)
{
#ifdef FEATURE_PERIODIC_WAKE_SLEEP
    uint16_t len = sizeof(wlan_periodic_traffic_teardown_evt_t);
    wlan_periodic_traffic_teardown_evt_t *traffic_teardown_evt =
        (wlan_periodic_traffic_teardown_evt_t *)ringif_get_ctrl_buf(0, NULL);
    if (NULL == traffic_teardown_evt) {
        NT_LOG_WMI_ERR("Memory allocation failed", 0, 0, 0);
        A_ASSERT(0);
    }

    traffic_teardown_evt->evt_hdr.status = (((wlan_periodic_traffic_teardown_evt_t *)msg)->evt_hdr.status == NT_OK)
                                               ? WIFI_STATUS_SUCCESS
                                               : WIFI_STATUS_FALIURE;
    traffic_teardown_evt->evt_hdr.msg_id = WLAN_PERIODIC_TRAFFIC_TEARDOWN_EVT;
    traffic_teardown_evt->evt_hdr.network_id = network_id;
    traffic_teardown_evt->teardown_reason = ((wlan_periodic_traffic_teardown_evt_t *)msg)->teardown_reason;
    traffic_teardown_evt->traffic_type = ((wlan_periodic_traffic_teardown_evt_t *)msg)->traffic_type;
    traffic_teardown_evt->session_id = ((wlan_periodic_traffic_teardown_evt_t *)msg)->session_id;
    ringif_send_wifi_config((void *)traffic_teardown_evt, len);
#else
    NT_LOG_WMI_ERR("WLAN_PERIODIC_TRAFFIC_TEARDOWN_EVT not supported", 0, 0, 0);
#endif /* FEATURE_PERIODIC_WAKE_SLEEP */
}

/*
 * @brief this function sends clk latency event.
 * @param msg: It contains status in event header.
 * @return This function does not return anything.
 */
void wlan_clk_latency_event(__unused void *msg)
{
    uint16_t len = sizeof(wlan_clk_latency_evt_t);
    wlan_clk_latency_evt_t *clk_lat_evt = (wlan_clk_latency_evt_t *)ringif_get_ctrl_buf(0, NULL);
    if (NULL == clk_lat_evt) {
        NT_LOG_WMI_ERR("Memory allocation failed", 0, 0, 0);
        A_ASSERT(0);
    }

    clk_lat_evt->evt_hdr.status =
        (((wlan_clk_latency_evt_t *)msg)->evt_hdr.status == NT_OK) ? WIFI_STATUS_SUCCESS : WIFI_STATUS_FALIURE;
    clk_lat_evt->evt_hdr.msg_id = WLAN_CLK_LATENCY_EVT;
    clk_lat_evt->evt_hdr.network_id = network_id;
    ringif_send_wifi_config((void *)clk_lat_evt, len);
}

#endif  // SUPPORT_RING_IF
