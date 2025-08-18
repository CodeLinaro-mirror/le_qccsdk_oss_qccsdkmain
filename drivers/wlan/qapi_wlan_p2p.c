/* 
Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#ifdef CONFIG_ENABLE_P2P_MODE
#include <wmi.h>
#include "qapi_wlan_p2p.h"
#include "wmi_api.h"
#include "wlan_qapi_helper.h"
#include "qapi_wlan_param_group.h"
#include "wlan_drv.h"

//------------------------------------------------------------------------------------------------------------
qapi_Status_t qapi_WLAN_P2P_Enable(uint8_t device_ID, qapi_WLAN_Enable_e enable)
{
    qapi_WLAN_P2P_Config_Params_t p2pConfig;

    if (!enable) {
        qapi_WLAN_P2P_Cancel(device_ID);
    }

    memset(&p2pConfig, 0, sizeof(qapi_WLAN_P2P_Config_Params_t));

    wlan_p2p_enable(device_ID, enable);
    p2pConfig.go_Intent = 0;
    p2pConfig.listen_Chan = 1;
    p2pConfig.op_Chan = 36;
    p2pConfig.age = 3000;
    p2pConfig.reg_Class = 81;
    p2pConfig.op_Reg_Class = 115;
    p2pConfig.max_Node_Count = 5;
    qapi_WLAN_Set_Param(device_ID, __QAPI_WLAN_PARAM_GROUP_P2P, __QAPI_WLAN_PARAM_GROUP_P2P_CONFIG_PARAMS,
                        (void *)&p2pConfig, sizeof(p2pConfig), QAPI_WLAN_NO_WAIT_E);

    return QAPI_OK;
}

qapi_Status_t qapi_WLAN_P2P_Cancel(uint8_t device_ID)
{
    qapi_Status_t error = QAPI_OK;
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;

    wmi_cmd_send(WMI_P2P_CANCEL_CMDID, NULL, 0);
    if (p_cxt->wlan_disable_block_mode) {
        qurt_signal_wait(&p_cxt->wlan_cmd_done, WLAN_WMI_CMD_SIG_MASK_DISABLE_DONE, QURT_SIGNAL_ATTR_CLEAR_MASK);
        log_printf("block mode, WMI cmd done\n");
    } else {
        log_printf("unblock mode, should check WMI cmd done in event cb\n");
    }

    qurt_mutex_lock(&p_cxt->wlan_qapi_cxt_mutex);
    error = get_wlan_qapi_error();
    qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);

    return error;
}

qapi_Status_t qapi_WLAN_P2P_Auth(uint8_t device_ID, int32_t dev_Auth, qapi_WLAN_P2P_WPS_Method_e wps_Method,
                                 const uint8_t *peer_Mac, qapi_WLAN_P2P_Persistent_e persistent)
{
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
    qapi_Status_t error = QAPI_OK;
    WMI_P2P_FW_CONNECT_CMD p2p_connect = p_cxt->p2p_conn;

    memset(&p2p_connect, 0, sizeof(WMI_P2P_FW_CONNECT_CMD));

    if (persistent == QAPI_WLAN_P2P_PERSISTENT_E)
        p2p_connect.dev_capab |= P2P_PERSISTENT_FLAG;
    p2p_connect.dev_auth = dev_Auth;
    p2p_connect.wps_method = wps_Method;
    memcpy(p2p_connect.peer_addr, peer_Mac, sizeof(p2p_connect.peer_addr));
    /* If go_intent <= 0, wlan firmware will use the intent value configured via
     * qapi_WLAN_param_set( P2P, P2P_INTENT) */
    p2p_connect.go_intent = 0;

    wmi_cmd_send(WMI_P2P_AUTH_GO_NEG_CMDID, &p2p_connect, sizeof(WMI_P2P_FW_CONNECT_CMD));
#if 0
    if (p_cxt->wlan_set_param_block_mode) {
        qurt_signal_wait(&p_cxt->wlan_cmd_done, WLAN_WMI_CMD_SIG_MASK_SET_MODE, QURT_SIGNAL_ATTR_CLEAR_MASK);
        log_printf("block mode, WMI cmd done\n");
    } else {
        log_printf("unblock mode, should check WMI cmd done in event cb\n");
    }
#endif

    qurt_mutex_lock(&p_cxt->wlan_qapi_cxt_mutex);
    error = get_wlan_qapi_error();
    qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);

    return error;
}

qapi_Status_t qapi_WLAN_P2P_Connect(uint8_t device_ID, qapi_WLAN_P2P_WPS_Method_e wps_Method, const uint8_t *peer_Mac,
                                    qapi_WLAN_P2P_Persistent_e persistent)
{
    qapi_Status_t error = QAPI_OK;
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
    WMI_P2P_FW_CONNECT_CMD p2p_connect = p_cxt->p2p_conn;

    memset(&p2p_connect, 0, sizeof(WMI_P2P_FW_CONNECT_CMD));
    p2p_connect.wps_method = wps_Method;
    memscpy(p2p_connect.peer_addr, sizeof(p2p_connect.peer_addr), peer_Mac, __QAPI_WLAN_MAC_LEN);

    /* Setting Default Value for now !!! */
    p2p_connect.dialog_token = 1;
    p2p_connect.go_intent = 0;
    p2p_connect.go_dev_dialog_token = 0;
    p2p_connect.dev_capab = 0x23;
    if (persistent == QAPI_WLAN_P2P_PERSISTENT_E) {
        p2p_connect.dev_capab |= P2P_PERSISTENT_FLAG;
    }

    if((p2p_connect.go_intent >= 10) || (p2p_connect.go_intent == 15))
    {
        p_cxt->connect_cmd.networkType = AP_NETWORK;
    }
    if((p2p_connect.go_intent <= 9) || (p2p_connect.go_intent == 0))
    {
        p_cxt->connect_cmd.networkType = INFRA_NETWORK;
    }

    qurt_mutex_lock(&p_cxt->wlan_qapi_cxt_mutex);
    memset(&p_cxt->p2p_Event_Cb_Info, 0, sizeof(qapi_WLAN_P2P_Event_Cb_Info_t));
    p_cxt->p2p_connect_in_progress = true;
    qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);

    wmi_cmd_send(WMI_P2P_CONNECT_CMDID, &p2p_connect, sizeof(WMI_P2P_FW_CONNECT_CMD));
    if (p_cxt->wlan_p2p_block_mode) {
        qurt_signal_wait(&p_cxt->wlan_cmd_done, WLAN_WMI_CMD_SIG_MASK_GO_NEG_RESULT, QURT_SIGNAL_ATTR_CLEAR_MASK);
        log_printf("block mode, WMI cmd done\n");
    } else {
        log_printf("unblock mode, should check WMI cmd done in event cb\n");
    }

    if (p_cxt->wlan_p2p_block_mode) {
        qurt_mutex_lock(&p_cxt->wlan_qapi_cxt_mutex);
        error = get_wlan_qapi_error();
        qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);
    }
    return error;
}

qapi_Status_t qapi_WLAN_P2P_Start_Go(uint8_t device_ID, qapi_WLAN_P2P_Go_Params_t *params, int32_t channel,
                                     qapi_WLAN_P2P_Persistent_e persistent)
{
    qapi_Status_t error = QAPI_OK;
    uint8_t wps_flag;
    qapi_WLAN_Auth_Mode_e authMode = QAPI_WLAN_AUTH_WPA2_PSK_E;
    qapi_WLAN_Crypt_Type_e encrType = QAPI_WLAN_CRYPT_AES_CRYPT_E;
    uint32_t opMode = QAPI_WLAN_DEV_MODE_AP_E;

    qapi_WLAN_Set_Param(device_ID, __QAPI_WLAN_PARAM_GROUP_WIRELESS_SECURITY,
                        __QAPI_WLAN_PARAM_GROUP_SECURITY_AUTH_MODE, (void *)&authMode, sizeof(authMode),
                        QAPI_WLAN_NO_WAIT_E);

    qapi_WLAN_Set_Param(device_ID, __QAPI_WLAN_PARAM_GROUP_WIRELESS_SECURITY,
                        __QAPI_WLAN_PARAM_GROUP_SECURITY_ENCRYPTION_TYPE, (void *)&encrType, sizeof(encrType),
                        QAPI_WLAN_NO_WAIT_E);
    if (params != NULL) {
        qapi_WLAN_Set_Param(device_ID, __QAPI_WLAN_PARAM_GROUP_P2P, __QAPI_WLAN_PARAM_GROUP_P2P_GO_PARAMS,
                            (void *)params, sizeof(qapi_WLAN_P2P_Go_Params_t), QAPI_WLAN_NO_WAIT_E);
    }
    qapi_WLAN_Set_Param(device_ID, __QAPI_WLAN_PARAM_GROUP_WIRELESS, __QAPI_WLAN_PARAM_GROUP_WIRELESS_OPERATION_MODE,
                        (void *)&opMode, sizeof(opMode), QAPI_WLAN_NO_WAIT_E);

    if (channel != __QAPI_WLAN_P2P_AUTO_CHANNEL) {
        qapi_WLAN_Set_Param(device_ID, __QAPI_WLAN_PARAM_GROUP_WIRELESS, __QAPI_WLAN_PARAM_GROUP_WIRELESS_CHANNEL,
                            (void *)&channel, sizeof(channel), QAPI_WLAN_NO_WAIT_E);
    }

    wps_flag = 0x01;
#if 0
    if (wlan_set_ap_params(device_ID, ATH_AP_WPS_FLAG, &wps_flag) != QAPI_OK)
    {
        return QAPI_ERROR;
    }
#endif

    qapi_WLAN_Set_Param(device_ID, __QAPI_WLAN_PARAM_GROUP_WIRELESS, __QAPI_WLAN_PARAM_GROUP_WIRELESS_OPERATION_MODE,
                        (void *)&opMode, sizeof(opMode), QAPI_WLAN_NO_WAIT_E);

    if (wlan_p2p_group_init(device_ID, persistent, 1) != QAPI_OK) {
        return QAPI_ERROR;
    }

#if 0
    qapi_WLAN_Power_Mode_Params_t pwrMode;
    /* Set MAX PERF */
    pwrMode.power_Mode = QAPI_WLAN_POWER_MODE_MAX_PERF_E;
    pwrMode.power_Module = QAPI_WLAN_POWER_MODULE_P2P_E;
    qapi_WLAN_Set_Param (device_ID,
                         __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                         __QAPI_WLAN_PARAM_GROUP_WIRELESS_POWER_MODE_PARAMS,
                         (void *) &pwrMode,
                         sizeof(pwrMode),
                         QAPI_WLAN_NO_WAIT_E);
#endif
    return error;
}

qapi_Status_t qapi_WLAN_P2P_Invite(uint8_t device_ID, const char *ssid, qapi_WLAN_P2P_WPS_Method_e wps_Method,
                                   const uint8_t *mac, qapi_WLAN_P2P_Persistent_e persistent,
                                   qapi_WLAN_P2P_Inv_Role_e role)
{
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
    qapi_Status_t error = QAPI_OK;
    WMI_P2P_INVITE_CMD p2pInvite = p_cxt->p2p_invite;

    memset(&p2pInvite, 0, sizeof(p2pInvite));

    memcpy(p2pInvite.ssid.ssid, ssid, sizeof(p2pInvite.ssid.ssid));
    p2pInvite.ssid.ssidLength = strlen(ssid);
    memcpy(p2pInvite.peer_addr, mac, sizeof(p2pInvite.peer_addr));
    p2pInvite.wps_method = (uint8_t)wps_Method;
    p2pInvite.is_persistent = (uint8_t)persistent;
    p2pInvite.role = (uint8_t)role;
    p2pInvite.dialog_token  = 1;

    wmi_cmd_send(WMI_P2P_INVITE_CMDID, &p2pInvite, sizeof(WMI_P2P_INVITE_CMD));
#if 0
    if (p_cxt->wlan_p2p_block_mode) {
        qurt_signal_wait(&p_cxt->wlan_cmd_done, WLAN_WMI_CMD_SIG_MASK_INVITE_REQ, QURT_SIGNAL_ATTR_CLEAR_MASK);
        log_printf("block mode, WMI cmd done\n");
    } else {
        log_printf("unblock mode, should check WMI cmd done in event cb\n");
    }
#endif
    if (p_cxt->wlan_p2p_block_mode) {
    qurt_mutex_lock(&p_cxt->wlan_qapi_cxt_mutex);
    error = get_wlan_qapi_error();
    qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);
    }

    return error;
}

qapi_Status_t qapi_WLAN_P2P_Join(uint8_t device_ID, qapi_WLAN_P2P_WPS_Method_e wps_Method, const uint8_t *mac,
                                 const char *pin, uint16_t channel)
{
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
    qapi_Status_t error = QAPI_OK;
    WLAN_P2P_JOIN_STRUCT *p2p_join_info = &p_cxt->p2p_join;
    uint8_t wps_mode = 0;

    memset(p2p_join_info, 0, sizeof(WLAN_P2P_JOIN_STRUCT));
    memscpy(p2p_join_info->peer_profile.peer_addr, sizeof(p2p_join_info->peer_profile.peer_addr), mac,
            __QAPI_WLAN_MAC_LEN);
    p2p_join_info->peer_profile.go_oper_freq = channel;
    p2p_join_info->peer_profile.wps_method = (uint8_t)wps_Method;

    if (wps_Method != QAPI_WLAN_P2P_WPS_PBC_E) {
        strlcpy((char *)(p2p_join_info->wps_pin.pin), pin, sizeof(pin));
        p2p_join_info->wps_pin.pin_length = WPS_PIN_LEN;
    }

    WMI_P2P_FW_CONNECT_CMD *pP2PConnect;
    WMI_BACKGROUND_SCAN_CMD scan_param_cmd;
    pP2PConnect = &(p2p_join_info->peer_profile);

    memscpy(p_cxt->wps_param.ssid_info.macaddress, sizeof(p_cxt->wps_param.ssid_info.macaddress),
            pP2PConnect->peer_addr, 6);
    wmi_cmd_send(WMI_P2P_SET_JOIN_PROFILE_CMDID, pP2PConnect, sizeof(WMI_P2P_FW_CONNECT_CMD));
#if 0
    qurt_mutex_lock(&p_cxt->wlan_qapi_cxt_mutex);
        if (p_cxt->wlan_set_param_block_mode) {
            p_cxt->param_id = WLAN_P2P_JOIN;
            qurt_signal_wait(&p_cxt->wlan_cmd_done, WLAN_WMI_CMD_SIG_MASK_SET_MODE, QURT_SIGNAL_ATTR_CLEAR_MASK);
            log_printf("block mode, WMI cmd done\n");
        } else {
            log_printf("unblock mode, should check WMI cmd done in event cb\n");
        }
    qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);
#endif
    printf("SET_JOIN_PROFILE successful\n");

    if (pP2PConnect->peer_go_ssid.ssidLength != 0) {
        memscpy(p_cxt->wps_param.ssid_info.ssid, sizeof(p_cxt->wps_param.ssid_info.ssid),
                pP2PConnect->peer_go_ssid.ssid, pP2PConnect->peer_go_ssid.ssidLength);
        p_cxt->wps_param.ssid_info.ssid_len = pP2PConnect->peer_go_ssid.ssidLength;
    }

    /* prevent background scan during WPS */

    if (pP2PConnect->wps_method == WPS_PBC) {
        wps_mode = QAPI_WLAN_WPS_PBC_MODE_E;
    } else if (pP2PConnect->wps_method == WPS_PIN_DISPLAY || pP2PConnect->wps_method == WPS_PIN_KEYPAD) {
        wps_mode = QAPI_WLAN_WPS_PIN_MODE_E;
        memscpy(p_cxt->wps_param.wps_pin.pin, sizeof(p_cxt->wps_param.wps_pin.pin), p2p_join_info->wps_pin.pin,
                WPS_PIN_LEN);
        p_cxt->wps_param.wps_pin.pin_length = WPS_PIN_LEN;
    }
    if (qapi_WLAN_Start_Wps(device_ID, 1, wps_mode, p_cxt->wps_param.wps_pin.pin, 0 /* AUTH_OPEN */) != 0) {
        info_printf("P2P WPS failed\r\n");
        return QAPI_ERROR;
    }
#if 0
    p_cxt->wps_param.ctl_flag |= 0x1;
    wmi_cmd_send(WMI_WPS_START_CMDID, &p_cxt->wps_param, sizeof(WMI_WPS_START_CMD));

    if (p_cxt->wlan_start_wps_block_mode) {
        qurt_signal_wait(&p_cxt->wlan_cmd_done,
        WLAN_WMI_CMD_SIG_MASK_STARTED_WPS_PROCESS,QURT_SIGNAL_ATTR_CLEAR_MASK); log_printf("block mode, WMI cmd
        done\n");
    } else {
        log_printf("unblock mode, should check WMI cmd done in event cb\n");
    }
    printf("WPS_START successful\n");

    if(p_cxt->wlan_start_wps_block_mode) {
    	qurt_mutex_lock(&p_cxt->wlan_qapi_cxt_mutex);
    	error = get_wlan_qapi_error();
    	qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);
    }
#endif

    return error;
}

qapi_Status_t qapi_WLAN_P2P_Prov(uint8_t device_ID, uint16_t wps_Method, const uint8_t *mac)
{
    qapi_Status_t error = QAPI_OK;
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
    WMI_P2P_FW_PROV_DISC_REQ_CMD p2p_prov_disc = p_cxt->p2p_prov_disc_req;

    qurt_mutex_lock(&p_cxt->wlan_qapi_cxt_mutex);
    memset(&p2p_prov_disc, 0, sizeof(p2p_prov_disc));
    p2p_prov_disc.dialog_token = 1;
    p2p_prov_disc.wps_method = wps_Method;
    memcpy(p2p_prov_disc.peer, mac, sizeof(p2p_prov_disc.peer));
    qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);

    wmi_cmd_send(WMI_P2P_FW_PROV_DISC_REQ_CMDID, &p2p_prov_disc, sizeof(WMI_P2P_FW_PROV_DISC_REQ_CMD));
#if 0
    if (p_cxt->wlan_set_param_block_mode) {
        qurt_signal_wait(&p_cxt->wlan_cmd_done, WLAN_WMI_CMD_SIG_MASK_PROV_DISC_REQ, QURT_SIGNAL_ATTR_CLEAR_MASK);
        log_printf("block mode, WMI cmd done\n");
    } else {
        log_printf("unblock mode, should check WMI cmd done in event cb\n");
    }
#endif
    error = get_wlan_qapi_error();

    return error;
}

qapi_Status_t qapi_WLAN_P2P_Find(uint8_t device_ID, qapi_WLAN_P2P_Disc_Type_e disc_Type, uint32_t timeout_In_Secs)
{
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
    WMI_P2P_FIND_CMD *find_params = &p_cxt->p2p_find;
    qapi_Status_t error = QAPI_OK;

    find_params->type = (uint8_t)disc_Type;
    find_params->timeout = (uint32_t)(timeout_In_Secs);

    wmi_cmd_send(WMI_P2P_FIND_CMDID, find_params, sizeof(WMI_P2P_FIND_CMD));
#if 0
    if (p_cxt->wlan_set_param_block_mode) {
        p_cxt->param_id = WLAN_P2P_FIND;
        qurt_signal_wait(&p_cxt->wlan_cmd_done, WLAN_WMI_CMD_SIG_MASK_SET_MODE, QURT_SIGNAL_ATTR_CLEAR_MASK);
        log_printf("block mode, WMI cmd done\n");
    } else {
        log_printf("unblock mode, should check WMI cmd done in event cb\n");
    }
#endif
    qurt_mutex_lock(&p_cxt->wlan_qapi_cxt_mutex);
    error = get_wlan_qapi_error();
    qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);
    PRINT_LOG_FUNC_LINE_EXIT;

    return error;
}

qapi_Status_t qapi_WLAN_P2P_Stop_Find(uint8_t device_ID)
{
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
    qapi_Status_t error = QAPI_OK;

    wmi_cmd_send(WMI_P2P_STOP_FIND_CMDID, NULL, 0);
#if 0
    if (p_cxt->wlan_set_param_block_mode) {
        qurt_signal_wait(&p_cxt->wlan_cmd_done, WLAN_WMI_CMD_SIG_MASK_SET_MODE, QURT_SIGNAL_ATTR_CLEAR_MASK);
        log_printf("block mode, WMI cmd done\n");
    } else {
        log_printf("unblock mode, should check WMI cmd done in event cb\n");
    }
#endif
    qurt_mutex_lock(&p_cxt->wlan_qapi_cxt_mutex);
    error = get_wlan_qapi_error();
    qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);

    return error;
}

qapi_Status_t qapi_WLAN_P2P_Invite_Auth(uint8_t device_ID, const qapi_WLAN_P2P_Invite_Info_t *invite_Info)
{
    qapi_Status_t error = QAPI_OK;
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
    WMI_P2P_FW_INVITE_REQ_RSP_CMD inv_rsp_cmd = p_cxt->p2p_inv_rsp;

    memset(&inv_rsp_cmd, 0, sizeof(WMI_P2P_FW_INVITE_REQ_RSP_CMD));
    inv_rsp_cmd.force_freq = invite_Info->force_Freq;
    inv_rsp_cmd.status = invite_Info->status;
    inv_rsp_cmd.dialog_token = invite_Info->dialog_Token;
    inv_rsp_cmd.is_go = invite_Info->is_GO;
    memscpy(inv_rsp_cmd.group_bssid, sizeof(inv_rsp_cmd.group_bssid), invite_Info->group_Bss_ID,
            sizeof(invite_Info->group_Bss_ID));

    qurt_mutex_lock(&p_cxt->wlan_qapi_cxt_mutex);
    memset(&p_cxt->p2p_Event_Cb_Info, 0, sizeof(qapi_WLAN_P2P_Event_Cb_Info_t));
    qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);

    if (wmi_cmd_send(WMI_P2P_INVITE_REQ_RSP_CMDID, &inv_rsp_cmd, sizeof(inv_rsp_cmd)) != QAPI_OK) {
        error = QAPI_ERROR;
    }

    if (p_cxt->wlan_p2p_block_mode) {
        qurt_mutex_lock(&p_cxt->wlan_qapi_cxt_mutex);
        error = get_wlan_qapi_error();
        qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);
    }
    return error;
}

qapi_Status_t qapi_WLAN_P2P_Listen(uint8_t device_ID, uint32_t timeout_In_Secs)
{
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
    qapi_Status_t error = QAPI_OK;
    uint32_t tout;

    tout = timeout_In_Secs;

    wmi_cmd_send(WMI_P2P_LISTEN_CMDID, &tout, sizeof(tout));
#if 0
    if (p_cxt->wlan_set_param_block_mode) {
        qurt_signal_wait(&p_cxt->wlan_cmd_done, WLAN_WMI_CMD_SIG_MASK_SET_MODE, QURT_SIGNAL_ATTR_CLEAR_MASK);
        log_printf("block mode, WMI cmd done\n");
    } else {
        log_printf("unblock mode, should check WMI cmd done in event cb\n");
    }
#endif
    qurt_mutex_lock(&p_cxt->wlan_qapi_cxt_mutex);
    error = get_wlan_qapi_error();
    qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);

    return error;
}
#endif // CONFIG_ENABLE_P2P_MODE
