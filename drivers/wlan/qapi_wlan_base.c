/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "wlan_drv.h"
#include "wmi_api.h"
#include "wlan_qapi_helper.h"
#include "safeAPI.h"
#include <unistd.h>

typedef enum {
    WPS_NONE,
    WPS_SCAN,
    WPS_CONNECTED
} WPS_STAGE_TYPE;

qapi_Status_t qapi_WLAN_Error (void)
{
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
    qurt_mutex_lock(&p_cxt->wlan_qapi_cxt_mutex);
    qapi_Status_t wlan_error = get_wlan_qapi_error();
    qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);
    return wlan_error;
}

qapi_Status_t qapi_WLAN_Enabled (qapi_WLAN_Enable_e *enable)
{
    if (!enable) {
        PRINT_ERR_INVALID_PARAM;
        return QAPI_WLAN_ERR_EINVAL;
    }

    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
    qurt_mutex_lock(&p_cxt->wlan_qapi_cxt_mutex);
    *enable = (qapi_WLAN_Enable_e)p_cxt->wlanEnabled;
    qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);
    return QAPI_OK;
}

qapi_Status_t qapi_WLAN_Enable (qapi_WLAN_Enable_e enable)
{
    qapi_Status_t ret = QAPI_WLAN_ERROR;

    WLAN_QAPI_LOCK();
    PRINT_LOG_FUNC_LINE_ENTRY;
    if (QAPI_WLAN_ENABLE_E==enable) {
        ret = wmi_on();
    } else if (QAPI_WLAN_DISABLE_E==enable) {
    	qapi_WLAN_Disconnect(0);
        ret = wmi_off();
    } else {
        PRINT_ERR_INVALID_PARAM;
        ret = QAPI_WLAN_ERR_EINVAL;
        goto exit;
    }
exit:
    PRINT_LOG_FUNC_LINE_EXIT;
    WLAN_QAPI_UNLOCK();
    return ret;
}

qapi_Status_t qapi_WLAN_Add_Device (uint8_t device_ID)
{
    qapi_Status_t ret = QAPI_WLAN_ERROR;

    WLAN_QAPI_LOCK();
    ret = wmi_add_device(device_ID);
    WLAN_QAPI_UNLOCK();
    return ret;
}

qapi_Status_t qapi_WLAN_Set_Callback (qapi_WLAN_Callback_t callback, void *application_Context)
{
    qapi_Status_t ret = QAPI_WLAN_ERROR;

    WLAN_QAPI_LOCK();
    ret = wlan_drv_set_cb(callback, application_Context);
    WLAN_QAPI_UNLOCK();
    return ret;
}

qapi_Status_t qapi_WLAN_Start_Scan(uint8_t device_ID, const qapi_WLAN_Start_Scan_Params_t *scan_Params)
{
    qapi_Status_t ret = QAPI_WLAN_ERROR;

    WLAN_QAPI_LOCK();
    ret = wmi_start_scan(device_ID, scan_Params);
    WLAN_QAPI_UNLOCK();
    return ret;
}

qapi_Status_t qapi_WLAN_Get_Scan_Results (uint8_t __attribute__((__unused__)) device_ID, qapi_WLAN_Scan_Comp_Evt_t *scan_Res, int16_t *num_Bss)
{
    qapi_Status_t ret = QAPI_WLAN_ERROR;

    if (!scan_Res || !num_Bss) {
        PRINT_ERR_INVALID_PARAM;
        return  QAPI_WLAN_ERR_EINVAL;
    }

    WLAN_QAPI_LOCK();
    ret = wlan_get_scan_results(device_ID, scan_Res, num_Bss);
    WLAN_QAPI_UNLOCK();
    return ret;
}

qapi_Status_t qapi_WLAN_Disconnect (uint8_t __attribute__((__unused__)) device_ID)
{
    qapi_Status_t ret = QAPI_OK;
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;

    WLAN_QAPI_LOCK();
    if (p_cxt->connected==true 
        || p_cxt->connect_in_progress 
        || p_cxt->wlan_roaming_started) {
        ret = wmi_disconnect();
    }

    qurt_mutex_lock(&p_cxt->wlan_qapi_cxt_mutex);
    memset(&p_cxt->connect_cmd, 0, sizeof(WMI_CONNECT_CMD));
    memset(&p_cxt->passphrase_cmd, 0, sizeof(WMI_SET_PASSPHRASE_CMD));
    wlan_clear_privacy();
    wlan_preset_specific_param();
    qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);

    WLAN_QAPI_UNLOCK();
    return ret;
}

qapi_Status_t qapi_WLAN_Commit (uint8_t  __attribute__((__unused__)) device_ID)
{
    qapi_Status_t ret = QAPI_WLAN_ERROR;
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
    uint16_t authMode = p_cxt->connect_cmd.authMode;

    WLAN_QAPI_LOCK();
    if ((authMode==WMI_WPA_PSK_AUTH) || 
        (authMode==WMI_WPA2_PSK_AUTH)  || 
        (authMode==WMI_WPA3_SHA256_AUTH) || 
        (authMode==(WMI_WPA2_PSK_AUTH | WMI_WPA3_SHA256_AUTH)) || 
        (authMode==(WMI_WPA_PSK_AUTH | WMI_WPA2_PSK_AUTH)) ||
        (authMode==(WMI_WPA3_SHA256_AUTH | WMI_WPA2_PSK_AUTH | WMI_WPA_PSK_AUTH))) {
        wmi_set_passphrase();
    }
    ret = wmi_connect();
    WLAN_QAPI_UNLOCK();
    return ret;
}

qapi_Status_t qapi_WLAN_Get_Regulatory_Info(qapi_WLAN_Reg_Evt_t *reg)
{
	return wlan_sta_get_reg_info(reg);
}

qapi_Status_t qapi_WLAN_Set_Rate (qapi_WLAN_Set_Rate_Params_t *prate_para)
{
    qapi_Status_t ret = QAPI_WLAN_ERROR;
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;

    memscpy(&(p_cxt->rate_param), sizeof(p_cxt->rate_param), prate_para, sizeof(qapi_WLAN_Set_Rate_Params_t));
    
    WLAN_QAPI_LOCK();
    ret = wmi_set_rate();
    WLAN_QAPI_UNLOCK();
    return ret;
}

qapi_Status_t qapi_WLAN_Get_Rate (qapi_WLAN_Set_Rate_Params_t *prate_para)
{
    qapi_Status_t ret = QAPI_WLAN_ERROR;
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;

    memscpy(&(p_cxt->rate_param), \
                  sizeof(p_cxt->rate_param), \
                  prate_para, \
                  sizeof(qapi_WLAN_Set_Rate_Params_t));

    WLAN_QAPI_LOCK();
    ret = wmi_get_rate();

    memscpy(prate_para, \
                 sizeof(*prate_para),
                 &(gp_wlan_qapi_cxt->rate_param), \
                 sizeof(qapi_WLAN_Set_Rate_Params_t));
    WLAN_QAPI_UNLOCK();
    return ret;
}

qapi_Status_t qapi_WLAN_Raw_Send(qapi_WLAN_Raw_Send_Params_t        *raw_Params)
{
    qapi_Status_t ret = QAPI_WLAN_ERROR;
	wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;	
	uint32_t channel[2] = {0, 0};
	SEND_RAW_FRAME raw_frame;
	qapi_WLAN_Set_Rate_Params_t set_rate_cfg;

	memset(&set_rate_cfg, 0, sizeof(qapi_WLAN_Set_Rate_Params_t));
	memset(&raw_frame, 0, sizeof(SEND_RAW_FRAME));

	raw_frame.deviceId = 2; //default device id
	
    if(raw_Params->rate_Index > 31 ||
	   raw_Params->num_Tries > 14 || raw_Params->num_Tries < 1 ||
       raw_Params->payload_Size > 1400 || 
       raw_Params->channel > 165 || raw_Params->channel == 0 ||
        ((raw_Params->header_Type > 3) && (raw_Params->header_Type != 0xff)))
    {
		  ret = QAPI_WLAN_ERROR;
          return ret;
    }

	if((raw_Params->header_Type == 0xff)
		&&(raw_Params->data == NULL))
	{
		ret = QAPI_WLAN_ERROR;
        return ret;
	}

	channel[0] = raw_Params->channel;
	channel[1] = 0;

	if(p_cxt->connected != true)
	{		
		if( raw_Params->channel != 0 )
	    {
	        ret = qapi_WLAN_Set_Param (raw_frame.deviceId,
	                         __QAPI_WLAN_PARAM_GROUP_WIRELESS,
	                         __QAPI_WLAN_PARAM_GROUP_WIRELESS_CHANNEL,
	                         (void *) &channel,
	                         sizeof(channel),
	                         QAPI_WLAN_NO_WAIT_E);
	        if (QAPI_OK != ret)
	        {
	            return ret;
	        }
	    }
		sleep(0.1);
	}
   
	raw_frame.num_Tries = raw_Params->num_Tries;
	raw_frame.payload_Size = raw_Params->payload_Size;
	raw_frame.header_Type = raw_Params->header_Type;
	raw_frame.rate_Index = raw_Params->rate_Index;
	raw_frame.seq = raw_Params->seq;
	memcpy(&raw_frame.addr1, &raw_Params->addr1, __QAPI_WLAN_MAC_LEN);
    memcpy(&raw_frame.addr2, &raw_Params->addr2, __QAPI_WLAN_MAC_LEN);
    memcpy(&raw_frame.addr3, &raw_Params->addr3, __QAPI_WLAN_MAC_LEN);
    memcpy(&raw_frame.addr4, &raw_Params->addr4, __QAPI_WLAN_MAC_LEN);
	raw_frame.data = raw_Params->data;
	raw_frame.data_Length = raw_Params->data_Length;
	
	memscpy(&(p_cxt->raw_pkt_frame), sizeof(SEND_RAW_FRAME), &raw_frame, sizeof(SEND_RAW_FRAME));
	
	WLAN_QAPI_LOCK();
	ret = wmi_send_raw();
	WLAN_QAPI_UNLOCK();

    return ret;
}

qapi_Status_t qapi_WLAN_Enable_Mgmt_Filter (uint8_t device_ID, uint32_t mgmt_filter)
{
    qapi_Status_t ret = QAPI_WLAN_ERROR;
	WMI_MGMT_FRAME_FILTER *p_mgmt_filter = &(gp_wlan_qapi_cxt->mgmt_filter);

    if (NULL == p_mgmt_filter->recv_queue)
    {
        return ret;
    }

	p_mgmt_filter->dev_id = device_ID;
	p_mgmt_filter->enable = 1;
	p_mgmt_filter->filter = mgmt_filter;
	
    WLAN_QAPI_LOCK();
    ret = wmi_set_mgmt_filter();
    WLAN_QAPI_UNLOCK();
    return ret;
}

qapi_Status_t qapi_WLAN_Disable_Mgmt_Filter (uint8_t device_ID)
{    
    qapi_Status_t ret = QAPI_WLAN_ERROR;
	WMI_MGMT_FRAME_FILTER *p_mgmt_filter = &(gp_wlan_qapi_cxt->mgmt_filter);

	p_mgmt_filter->enable = 0;
    p_mgmt_filter->dev_id = device_ID;
	
    WLAN_QAPI_LOCK();
    ret = wmi_set_mgmt_filter();
    WLAN_QAPI_UNLOCK();
	wlan_clear_mgmt_frame_queue();

	return ret;
}

qapi_Status_t qapi_WLAN_Recv_Mgmt_Frames (uint8_t *buffer, uint32_t buffer_len, uint32_t *frame_len, uint32_t timeout)
{
	WMI_MGMT_FRAME_FILTER *p_mgmt_filter = &(gp_wlan_qapi_cxt->mgmt_filter);
	
	if ((!p_mgmt_filter->enable) || (NULL == p_mgmt_filter->recv_queue))
	{
	    return QAPI_WLAN_ERROR;
	}
    
    return wlan_recv_mgmt_frame(buffer, buffer_len, frame_len, timeout);
}

#ifdef CONFIG_WPS
qapi_Status_t qapi_WLAN_Start_Wps(uint8_t device_ID,
                             qapi_WLAN_WPS_Connect_Action_e connect_Action,
                             qapi_WLAN_WPS_Mode_e mode,
                             const char  *pin,
                             uint8_t auth_floor)
{
    qapi_Status_t ret = QAPI_WLAN_ERROR;
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
    p_cxt->wps_in_progress = 1;
    p_cxt->wps_stage = WPS_SCAN;
    WLAN_QAPI_LOCK();
    ret = wmi_start_wps_process(device_ID, connect_Action, mode, pin, auth_floor);
    WLAN_QAPI_UNLOCK();
    return ret;
}

qapi_Status_t qapi_WLAN_Stop_Wps (uint8_t device_ID, uint8_t wps_stage)
{
    qapi_Status_t ret = QAPI_OK;
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
    p_cxt->wps_stage = wps_stage;

    if (p_cxt->wps_stage == WPS_CONNECTED && p_cxt->wps_in_progress)
    {
        WLAN_QAPI_LOCK();
        ret = wmi_disconnect();
        qurt_mutex_lock(&p_cxt->wlan_qapi_cxt_mutex);
        memset(&p_cxt->connect_cmd, 0, sizeof(WMI_CONNECT_CMD));
        memset(&p_cxt->passphrase_cmd, 0, sizeof(WMI_SET_PASSPHRASE_CMD));
        wlan_clear_privacy();
        wlan_preset_specific_param();
        qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);
        WLAN_QAPI_UNLOCK();
    }
    else if (p_cxt->wps_stage == WPS_SCAN && p_cxt->wps_in_progress)
    {
        WLAN_QAPI_LOCK();
        ret = wmi_stop_scan();
        WLAN_QAPI_UNLOCK();
    }
    p_cxt->wps_in_progress = 0;
    return ret;
}

#endif

