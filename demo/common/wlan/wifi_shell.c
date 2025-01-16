/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <stdio.h>
#include <ctype.h>
#include "wifi_cmn.h"
#include "qapi_wlan.h"
#include "qapi_console.h"

#include "qurt_internal.h"
#include "qurt_mutex.h"

#include "safeAPI.h"
#ifdef CONFIG_MGMT_FILTER_DEMO
#include "mgmt_filter_demo.h"
#endif

#ifdef CONFIG_QCSPI_HFC_ETH_ENABLE
#include "data_svc_hfc.h"
#include "qapi_hfc.h"
#endif

#define WIFI_SHELL_INFO 1
#define WIFI_SHELL_LOG  0

#define WLAN_SHELL_GROUP_NAME    "WLAN"
#define WLAN_SHELL_GROUP_PRINTF_SUFFIX  "WLAN: "

#define LOG_PREFIX  "[LOG] "

#if WIFI_SHELL_INFO
#define info_printf(msg,...)     printf(WLAN_SHELL_GROUP_PRINTF_SUFFIX msg, ##__VA_ARGS__)
#else
#define info_printf(args...)     do { } while (0)
#endif

#if WIFI_SHELL_LOG
#define log_printf(msg,...)     printf(WLAN_SHELL_GROUP_PRINTF_SUFFIX LOG_PREFIX msg, ##__VA_ARGS__)
#else
#define log_printf(args...)     do { } while (0)
#endif

#define PRINT_ERR_NOT_SUPPORTED  info_printf("Not supported yet\n")
#define PRINT_ERR_CMD_FAILED     info_printf("Cmd failed\n")

#define SCAN_MODE_BLOCKING      1
#define SCAN_MODE_UNBLOCKING    2

#ifndef NT_MAX_DEVICES
#define NT_MAX_DEVICES			2
#endif
#ifndef NT_DEV_AP_ID
#define NT_DEV_AP_ID			0
#endif
#ifndef NT_DEV_STA_ID
#define NT_DEV_STA_ID			1
#endif
typedef struct wifi_shell_cxt_s {
    qurt_mutex_t    wifi_shell_cxt_mutex;
    int32_t         scan_mode;
    qapi_WLAN_Auth_Mode_e auth;
    qapi_WLAN_Phy_Mode_e phy_mode;
    qapi_WLAN_11n_HT_Config_e htcfg;
    qbool_t         connected;
    char            ssid[__QAPI_WLAN_MAX_SSID_LEN+1];
    int32_t         ssid_length;
    uint8           bssid[6];
    uint16_t        channel_frequency;
	uint8_t			active_device;
        uint8_t                 wlan_enabled;
} wifi_shell_cxt_t;

static wifi_shell_cxt_t g_wifi_shell_cxt;
static wifi_shell_cxt_t *pg_wifi_shell_cxt;

uint8_t get_active_device()
{
	wifi_shell_cxt_t *p_cxt = pg_wifi_shell_cxt;
	return p_cxt->active_device;
}

qbool_t get_device_connect_state(void)
{
	wifi_shell_cxt_t *p_cxt = pg_wifi_shell_cxt;
       
	return p_cxt->connected;
}

static void print_scan_results(qapi_WLAN_Scan_Comp_Evt_t *scan_coml_evt)
{
    int16_t i = 0;
    uint8_t temp_ssid[33] = {0};
    qapi_WLAN_BSS_Scan_Info_t *list = scan_coml_evt->scan_bss_info;
    int16_t num_scan = scan_coml_evt->num_bss_cur;

    info_printf("Scan result count:%d\r\n", num_scan);
    log_printf("list=0x%x\n", list);
    for (i = 0;i<num_scan;i++) {
        memscpy(temp_ssid,list[i].ssid_Length,list[i].ssid,list[i].ssid_Length);
        temp_ssid[list[i].ssid_Length] = '\0';
        if (list[i].ssid_Length == 0) {
            info_printf("ssid = SSID Not available\r\n\n");
        } else {
            {
                info_printf("ssid = %s\r\n",temp_ssid);
                info_printf("bssid = %.2x:%.2x:%.2x:%.2x:%.2x:%.2x\r\n",list[i].bssid[0],list[i].bssid[1],list[i].bssid[2],list[i].bssid[3],list[i].bssid[4],list[i].bssid[5]);
                info_printf("channel = %d\r\n",list[i].channel);
                info_printf("indicator = %d\r\n",list[i].rssi);
                info_printf("security = ");
                if(list[i].security_Enabled){
                    if(list[i].rsn_Auth || list[i].rsn_Cipher){
                        printf("\r\n\r");
                        if((list[i].rsn_Auth & __QAPI_WLAN_SECURITY_AUTH_1X) || (list[i].rsn_Auth & __QAPI_WLAN_SECURITY_AUTH_PSK))
                            printf("RSN/WPA2= ");
                        if(list[i].rsn_Auth & __QAPI_WLAN_SECURITY_AUTH_SAE)
                            printf("WPA3= ");
                    }
                    if(list[i].rsn_Auth){
                        printf(" {");
                        if(list[i].rsn_Auth & __QAPI_WLAN_SECURITY_AUTH_1X){
                             printf("802.1X ");
                        }
                        if(list[i].rsn_Auth & __QAPI_WLAN_SECURITY_AUTH_PSK){
                            printf("PSK ");
                        }
                        if(list[i].rsn_Auth & __QAPI_WLAN_SECURITY_AUTH_SAE){
                            printf("SAE");
                        }
                        printf("}");
                    }
                    if(list[i].rsn_Cipher){
                        printf(" {");
                        /* AP security can support multiple options hence
                         * we check each one separately. Note rsn == wpa2 */
                        if(list[i].rsn_Cipher & __QAPI_WLAN_CIPHER_TYPE_WEP){
                            printf("WEP ");
                        }
                        if(list[i].rsn_Cipher & __QAPI_WLAN_CIPHER_TYPE_TKIP){
                            printf("TKIP ");
                        }
                        if(list[i].rsn_Cipher & __QAPI_WLAN_CIPHER_TYPE_CCMP){
                            printf("AES ");
                        }
                        printf("}");
                    }
                    if(list[i].wpa_Auth || list[i].wpa_Cipher){
                        printf("\r\n\r");
                        printf("WPA= ");
                    }
                    if(list[i].wpa_Auth){
                         printf(" {");
                         if(list[i].wpa_Auth & __QAPI_WLAN_SECURITY_AUTH_1X){
                             printf("802.1X ");
                         }
                         if(list[i].wpa_Auth & __QAPI_WLAN_SECURITY_AUTH_PSK){
                             printf("PSK ");
                         }
                         printf("}");
                    }
                    if(list[i].wpa_Cipher){
                        printf(" {");
                        if(list[i].wpa_Cipher & __QAPI_WLAN_CIPHER_TYPE_WEP){
                            printf("WEP ");
                        }
                        if(list[i].wpa_Cipher & __QAPI_WLAN_CIPHER_TYPE_TKIP){
                            printf("TKIP ");
                        }
                        if(list[i].wpa_Cipher & __QAPI_WLAN_CIPHER_TYPE_CCMP){
                            printf("AES ");
                        }
                        printf("}");
                    }
                }else{
                    printf("NONE! ");
                }
            }
        }

        if(i!= num_scan-1) {
            printf("\n ");
            printf("\n \r");
        } else {
            printf("\nshell> ");
        }
    }
}

#ifdef CONFIG_QCSPI_HFC_ETH_ENABLE
int qcspi_hfc_send_wlan_event(uint32_t event_id)
{
    f2a_event_type info;
	if (event_id == QAPI_WLAN_CONNECT_CB_E) {
        info = WLAN_CONNECT_EVENT;
	} else if (event_id == QAPI_WLAN_DISCONNECT_CB_E) {
        info = WLAN_DISCONNECT_EVENT;
	} else {
        return -1;
	}
	
	qapi_hfc_set_gpio_assert_info(info);
	return 0;  
}
#endif

static void wlan_shell_event_handler(__unused uint8_t deviceId, uint32_t cbId, void __unused *pApplicationContext, void *payload, uint32_t payload_Length)
{
    wifi_shell_cxt_t *p_cxt = pg_wifi_shell_cxt;

    switch(cbId) {
    case QAPI_WLAN_SCAN_COMPLETE_CB_E: {
        if (!payload || !payload_Length) {
            info_printf("QAPI_WLAN_SCAN_COMPLETE_CB_E event error\n");
            break;
        }

        qapi_WLAN_Scan_Comp_Evt_t *p_scan_compl_evt = (qapi_WLAN_Scan_Comp_Evt_t*)payload;
        info_printf("Received Scan complete event, found bss count:%d\n", p_scan_compl_evt->num_bss_cur);
        if (p_cxt->scan_mode==SCAN_MODE_BLOCKING){
            info_printf("blocking mode\n");
        } else if (p_cxt->scan_mode==SCAN_MODE_UNBLOCKING) {
            info_printf("unblocking mode\n");
            print_scan_results(p_scan_compl_evt);
        } else {
            info_printf("unknown mode=%d, ignore\n", p_cxt->scan_mode);
        }
        break;
    }
    case QAPI_WLAN_CONNECT_CB_E: {
        qapi_WLAN_Join_Comp_Evt_t *cxnInfo  = (qapi_WLAN_Join_Comp_Evt_t *)(payload);
        uint8_t * mac = cxnInfo->bssid;
		if(cxnInfo->ssid_Length) {
			memscpy(p_cxt->ssid, cxnInfo->ssid_Length, cxnInfo->ssid, cxnInfo->ssid_Length);
			p_cxt->ssid[cxnInfo->ssid_Length] = 0;
			p_cxt->ssid_length = cxnInfo->ssid_Length;
			memscpy(p_cxt->bssid, 6, cxnInfo->bssid, 6);
		}
        p_cxt->channel_frequency = cxnInfo->channel_frequency;
        if(cxnInfo->evt_hdr.status == QAPI_OK){
            qapi_WLAN_Auth_Mode_e e_wpa_ver = p_cxt->auth;
			if(cxnInfo->bss_Connection_Status)
				p_cxt->connected = true;
            info_printf("devid - %d %d CONNECTED MAC addr %02x:%02x:%02x:%02x:%02x:%02x\n",
                p_cxt->active_device, cxnInfo->bss_Connection_Status, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            if (e_wpa_ver==QAPI_WLAN_AUTH_WPA_PSK_E || e_wpa_ver==QAPI_WLAN_AUTH_WPA2_PSK_E) {
                info_printf("4 way handshake success for device=1\n");
            }	
        } else {
			info_printf("WiFi disconnect reason code is %d\n", cxnInfo->reason_code);
			if(cxnInfo->bss_Connection_Status) {
				p_cxt->connected = false;
				info_printf("devId %d Disconnected MAC addr %02x:%02x:%02x:%02x:%02x:%02x \n",
					p_cxt->active_device, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
			} else {
				info_printf("REF_STA Disconnected MAC addr %02x:%02x:%02x:%02x:%02x:%02x devId %d\r\n",
                     mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], p_cxt->active_device);
			}
        }
        info_printf("channel_frequency=%d\n", cxnInfo->channel_frequency);
        info_printf("ssid = %s\n", p_cxt->ssid);
        info_printf("assoc_id=%d\n", cxnInfo->assoc_id);
        info_printf("host_initiated=%d\n", cxnInfo->host_initiated);
#ifdef CONFIG_QCSPI_HFC_ETH_ENABLE		
        qcspi_hfc_send_wlan_event(QAPI_WLAN_CONNECT_CB_E);
#endif		
		
        break;
    }
    case QAPI_WLAN_DISCONNECT_CB_E: {
        qapi_WLAN_Join_Comp_Evt_t *cxnInfo = (qapi_WLAN_Join_Comp_Evt_t *)(payload);
		if(cxnInfo->bss_Connection_Status) {
            p_cxt->connected = false;
        }
        
        if(p_cxt->ssid_length) 
            info_printf("devId %d disconnected from ssid = %s\n", p_cxt->active_device, p_cxt->ssid);

#ifdef CONFIG_QCSPI_HFC_ETH_ENABLE		
        qcspi_hfc_send_wlan_event(QAPI_WLAN_DISCONNECT_CB_E);
#endif		
        break;
    }
	case QAPI_WLAN_CHANNEL_SWITCH_CB_E: {
		qapi_WLAN_Chan_Switch_Evt_t *ecsa = (qapi_WLAN_Chan_Switch_Evt_t *)payload;
		if(ecsa->evt_hdr.status == QAPI_OK) {
			p_cxt->channel_frequency = ecsa->freq;
			info_printf("devId %d channel switch to %d success\n", p_cxt->active_device, ecsa->freq);
		} else {
			info_printf("devId %d channel switch fail, reason %d\n", p_cxt->active_device, ecsa->reason);
		}
		break;
	}
    }
}

static qapi_Status_t Enable(uint32_t __attribute__((__unused__)) Parameter_Count, QAPI_Console_Parameter_t __attribute__((__unused__)) *Parameter_List)
{
    qapi_Status_t ret;
	qapi_WLAN_DEV_Mode_e devMode = DEV_MODE_STATION_E;
	wifi_shell_cxt_t *p_cxt = pg_wifi_shell_cxt;

    if(p_cxt->wlan_enabled){
        return QAPI_OK;
    }

    qapi_WLAN_Set_Callback(wlan_shell_event_handler, &g_wifi_shell_cxt);
    ret = qapi_WLAN_Enable(true);
    if (QAPI_OK != ret){
        PRINT_ERR_CMD_FAILED;
        return ret;
    }
    p_cxt->wlan_enabled = 1;
    //qapi_WLAN_Add_Device(0);
    info_printf("enabled\n");
	//TODO currently only support station mode
	ret = qapi_WLAN_Set_Param(0,
							__QAPI_WLAN_PARAM_GROUP_WIRELESS,
							__QAPI_WLAN_PARAM_GROUP_WIRELESS_OPERATION_MODE,
							&devMode,
							sizeof(devMode),
							FALSE);

	if(ret != QAPI_OK) {
		info_printf("set mode station fail\n");
	} else {
		p_cxt->active_device = NT_DEV_STA_ID;
	}
    return ret;
}

static qapi_Status_t Disable(uint32_t __attribute__((__unused__)) Parameter_Count, QAPI_Console_Parameter_t __attribute__((__unused__)) *Parameter_List)
{
    qapi_Status_t ret;
    wifi_shell_cxt_t *p_cxt = pg_wifi_shell_cxt;

    if(p_cxt->wlan_enabled == 0){
        return QAPI_OK;
    }
    ret = qapi_WLAN_Enable(false);

    if (QAPI_OK != ret){
        PRINT_ERR_CMD_FAILED;
        return ret;
    }
    p_cxt->wlan_enabled = 0;
    info_printf("disabled\n");
    return ret;
}

int32_t get_phy_mode()
{
	qapi_WLAN_Phy_Mode_e phy_mode;
	char data[32+1] = {'\0'};
	uint32_t length = sizeof(qapi_WLAN_Phy_Mode_e);
	uint32_t deviceId = 0;
	if(QAPI_OK != qapi_WLAN_Get_Param (deviceId,
								__QAPI_WLAN_PARAM_GROUP_WIRELESS,
								__QAPI_WLAN_PARAM_GROUP_WIRELESS_PHY_MODE,
								&phy_mode,
								&length)){
		info_printf("get phy mode fail\n");
		return -1;
	}

	if(phy_mode == QAPI_WLAN_11B_MODE_E)
		strlcpy(data, "b", sizeof(data));
	else if(phy_mode == QAPI_WLAN_11G_MODE_E)
		strlcpy(data, "g", sizeof(data));
	else if(phy_mode == QAPI_WLAN_11NG_HT20_MODE_E)
		strlcpy(data, "ng", sizeof(data));
	else if(phy_mode == QAPI_WLAN_11A_MODE_E)
		strlcpy(data, "a", sizeof(data));
	else if(phy_mode == QAPI_WLAN_11A_HT20_MODE_E)
		strlcpy(data, "a", sizeof(data));
	else if(phy_mode == QAPI_WLAN_11ABGN_HT20_MODE_E)
		strlcpy(data, "abgn", sizeof(data));
	else {
		info_printf("Phy mode    = unknown (%d)\n",(int)phy_mode);
		return -1;
	}
	info_printf("Phy mode    = %s\n",data);
	return 0;
}

int32_t get_wifi_power_mode()
{
	uint8_t power_mode = 0;
	uint32_t length = sizeof(power_mode);
	uint32_t deviceId = get_active_device();
	char data[64+1] = {'\0'};
	if(QAPI_OK != qapi_WLAN_Get_Param (deviceId,
								__QAPI_WLAN_PARAM_GROUP_WIRELESS,
								__QAPI_WLAN_PARAM_GROUP_WIRELESS_POWER_MODE_PARAMS,
								&power_mode,
								&length)){
		info_printf("get wifi power mode fail for device %d\n",deviceId);
		return -1;
	}

	if (power_mode == 0){
		strlcpy(data, "Max Perf", sizeof(data));
	} else {
		strlcpy(data, "Power Save ", sizeof(data));
		if ((power_mode&1) == 1) {
			strlcat(data, "(bmps enabled) ", sizeof(data));
		}
		if ((power_mode&2) == 2) {
			strlcat(data, "(IMPS enabled) ", sizeof(data));
		}
		if ((power_mode&4) == 4) {
			strlcat(data, "(WUR enabled) ", sizeof(data));
		}
		if ((power_mode&8) == 8) {
			strlcat(data, "(WNM enabled) ", sizeof(data));
		}
	}
	info_printf("Power mode  = %s\n",data);
	return 0;
}
int32_t get_device_mac_address()
{
	uint8_t mac[__QAPI_WLAN_MAC_LEN] = {0};
	uint32_t length = __QAPI_WLAN_MAC_LEN;
	uint8_t deviceId = get_active_device();
	if(QAPI_OK != qapi_WLAN_Get_Param (deviceId,
								__QAPI_WLAN_PARAM_GROUP_WIRELESS,
								__QAPI_WLAN_PARAM_GROUP_WIRELESS_MAC_ADDRESS,
								&mac[0],
								&length)){
		info_printf("get mac address fail for device %d\n",deviceId);
		return -1;
	}
	info_printf("Mac Addr    = %02x:%02x:%02x:%02x:%02x:%02x\n",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
	return 0;
}
int32_t get_op_mode()
{
	qapi_WLAN_DEV_Mode_e conc_mode, opmode;
	uint32_t length = sizeof(qapi_WLAN_DEV_Mode_e);
	uint8_t deviceId = get_active_device();
	if(QAPI_OK != qapi_WLAN_Get_Param (deviceId,
								__QAPI_WLAN_PARAM_GROUP_WIRELESS,
								__QAPI_WLAN_PARAM_GROUP_WIRELESS_CONCURRENCY_MODE,
								&conc_mode,
								&length)){
		info_printf("get concurrency mode fail\n");
		return -1;
	}

	if(conc_mode == DEV_MODE_AP_STA_E) {
		info_printf("concurrency mode\n");
	}

	if(QAPI_OK != qapi_WLAN_Get_Param (deviceId,
								__QAPI_WLAN_PARAM_GROUP_WIRELESS,
								__QAPI_WLAN_PARAM_GROUP_WIRELESS_OPERATION_MODE,
								&opmode,
								&length)){
		info_printf("get operation mode fail for device %d\n",deviceId);
		return -1;
	}

	if(opmode == DEV_MODE_STATION_E) {
		info_printf("mode        = station\n");
	}
	else if(opmode == DEV_MODE_AP_E) {
		info_printf("mode        = softap\n");
	}
	return 0;
}

static qapi_Status_t Info(uint32_t __attribute__((__unused__)) Parameter_Count, QAPI_Console_Parameter_t __attribute__((__unused__)) *Parameter_List)
{
    wifi_shell_cxt_t *p_cxt = pg_wifi_shell_cxt;

    if (0 == p_cxt->wlan_enabled)
    {
        info_printf("Enable WLAN before get the WLAN infomation\r\n");
        return QAPI_ERROR;
    }

    if(p_cxt->connected == true)
    {
        info_printf("ssid        = %s\n", p_cxt->ssid);
        info_printf("channel     = %d \n", p_cxt->channel_frequency);
        info_printf("bssid       = %02x:%02x:%02x:%02x:%02x:%02x\n",p_cxt->bssid[0],p_cxt->bssid[1],p_cxt->bssid[2],p_cxt->bssid[3],p_cxt->bssid[4],p_cxt->bssid[5]);
    }

    get_phy_mode();
    get_wifi_power_mode();
    get_device_mac_address();
	get_op_mode();
    return QAPI_OK;
}

qapi_Status_t set_active_deviceid(uint8_t deviceId)
{
#ifdef NT_FN_CONCURRENCY
	wifi_shell_cxt_t *p_cxt = pg_wifi_shell_cxt;
	qapi_WLAN_DEV_Mode_e conc_mode = DEV_MODE_NO_CONC_E;
	uint32_t length = sizeof(qapi_WLAN_DEV_Mode_e);

	if(deviceId >= NT_MAX_DEVICES)
	{
		info_printf("the maximum device ID is %d\n",NT_MAX_DEVICES-1);
		return QAPI_ERROR;
	}

	if(QAPI_OK != qapi_WLAN_Get_Param (0,
								__QAPI_WLAN_PARAM_GROUP_WIRELESS,
								__QAPI_WLAN_PARAM_GROUP_WIRELESS_CONCURRENCY_MODE,
								&conc_mode,
								&length)){
		info_printf("get concurrency mode fail\n");
		return QAPI_ERROR;
	}
	if(conc_mode == DEV_MODE_AP_STA_E) {
		p_cxt->active_device = deviceId;
		return QAPI_OK;
	}
#endif

	info_printf("DUT work in single device mode\n");
	return QAPI_ERROR;
}

static qapi_Status_t SetDevice(uint32_t __attribute__((__unused__)) Parameter_Count, QAPI_Console_Parameter_t __attribute__((__unused__)) *Parameter_List)
{
	if( Parameter_Count < 1 || !Parameter_List || !Parameter_List[0].Integer_Is_Valid){
		return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
	}
	return set_active_deviceid(Parameter_List[0].Integer_Value);
}

static qapi_Status_t Scan(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    qapi_Status_t ret = QAPI_OK;
    wifi_shell_cxt_t *p_cxt = pg_wifi_shell_cxt;
    qapi_WLAN_Start_Scan_Params_t scan_param = {0};
    qbool_t scan_ssid = false;
	qapi_WLAN_DEV_Mode_e opmode;
	uint32_t length = sizeof(qapi_WLAN_DEV_Mode_e);
	uint8_t deviceId = get_active_device();

    if (0 == p_cxt->wlan_enabled)
    {
        info_printf("Enable WLAN before scan\r\n");
        return QAPI_ERROR;
    }


    qurt_mutex_lock(&p_cxt->wifi_shell_cxt_mutex);
    p_cxt->scan_mode = SCAN_MODE_BLOCKING;
    if(Parameter_Count >= 1 && Parameter_List[0].Integer_Is_Valid) {
        int32_t param_scan_mode = Parameter_List[0].Integer_Value;
        if((param_scan_mode<SCAN_MODE_BLOCKING) || (param_scan_mode>SCAN_MODE_UNBLOCKING)) {
            info_printf("Invalid scan mode (%d)\n", param_scan_mode);
            ret = QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
            qurt_mutex_unlock(&p_cxt->wifi_shell_cxt_mutex);
            goto exit;
        }
        p_cxt->scan_mode = param_scan_mode;
    }
    if(Parameter_Count >= 2 && !Parameter_List[1].Integer_Is_Valid) {
        uint8_t ssid_Length = strlen((char *) Parameter_List[1].String_Value);
        if(ssid_Length > __QAPI_WLAN_MAX_SSID_LEN) {
            info_printf("SSID length exceeds Maximum value\r\n");
            ret = QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
            qurt_mutex_unlock(&p_cxt->wifi_shell_cxt_mutex);
            goto exit;
        }
        scan_param.ssid_Length = ssid_Length;
        memscpy(scan_param.ssid, ssid_Length, Parameter_List[1].String_Value, ssid_Length);
        scan_ssid = true;
    }
    if(QAPI_OK != qapi_WLAN_Get_Param (deviceId,
									__QAPI_WLAN_PARAM_GROUP_WIRELESS,
									__QAPI_WLAN_PARAM_GROUP_WIRELESS_OPERATION_MODE,
									&opmode,
									&length)){
		info_printf("get operation mode fail for device %d\n",deviceId);
		goto exit;
    }
	if(opmode != DEV_MODE_STATION_E) {
		info_printf("current operation mode %d do not support scan, need to set station mode\n",opmode);
		goto exit;
	}
    info_printf("scan_mode=%d\n", p_cxt->scan_mode);
    qurt_mutex_unlock(&p_cxt->wifi_shell_cxt_mutex);

    if (scan_ssid) {
        ret = qapi_WLAN_Start_Scan(deviceId, &scan_param);
    } else {
        ret = qapi_WLAN_Start_Scan(deviceId, NULL);
    }

    if ((ret == QAPI_OK) && \
        (SCAN_MODE_BLOCKING == p_cxt->scan_mode)) {
        qapi_WLAN_Scan_Comp_Evt_t scan_complete_evt = {0};
        int16_t bss_cnt = 0;

        ret = qapi_WLAN_Get_Scan_Results(deviceId, &scan_complete_evt, &bss_cnt);
        bss_cnt = scan_complete_evt.num_bss_cur;
        qapi_WLAN_Scan_Comp_Evt_t *scan_complete_evt_total = malloc(sizeof(qapi_WLAN_Scan_Comp_Evt_t) + bss_cnt*sizeof(qapi_WLAN_BSS_Scan_Info_t));
        ret = qapi_WLAN_Get_Scan_Results(deviceId, scan_complete_evt_total, &bss_cnt);
        if (scan_complete_evt_total) {
            print_scan_results(scan_complete_evt_total);
            free(scan_complete_evt_total);
        } else {
            info_printf("Failed to allocate memory to scan\n");
        }
    }

exit:
    return ret;
}

static qapi_Status_t SetWpaPassphrase(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    if( Parameter_Count < 1 || !Parameter_List){
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }
	uint8_t deviceId = get_active_device();
    char* passphrase = Parameter_List[0].String_Value;
    uint32_t len = strlen(passphrase);
    if((len < 8) || (len >64)) {
        info_printf("Wrong passphrase length=%d, the length should be between 8 and 64\n", len);
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }

    if(len == 64) {
        uint32_t i = 0;
        for (i = 0; i < len; i++) {
            if(!isxdigit((int)passphrase[i])) {
                info_printf("passphrase in hex, please enter [0-9] or [A-F]\n");
                return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
            }
        }
    }
    qapi_WLAN_Set_Param (deviceId, __QAPI_WLAN_PARAM_GROUP_WIRELESS_SECURITY,
        __QAPI_WLAN_PARAM_GROUP_SECURITY_PASSPHRASE,
        (void *)passphrase, len, FALSE);

    return QAPI_OK;
}

static qapi_Status_t SetWpaParameters(uint32_t __attribute__((__unused__)) Parameter_Count, QAPI_Console_Parameter_t __attribute__((__unused__)) *Parameter_List)
{
    if(  Parameter_Count != 3 || !Parameter_List || Parameter_List[0].Integer_Is_Valid || Parameter_List[1].Integer_Is_Valid || Parameter_List[2].Integer_Is_Valid) {
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }

	uint8_t deviceId = get_active_device();
    char *wpaVer = Parameter_List[0].String_Value;
    char *ucipher = Parameter_List[1].String_Value;
    char *mcipher = Parameter_List[2].String_Value;
    qapi_WLAN_Auth_Mode_e e_wpa_ver;
    qapi_WLAN_Crypt_Type_e e_cipher;
    if(!strcmp(wpaVer,"WPA")) {
        e_wpa_ver = QAPI_WLAN_AUTH_WPA_PSK_E;
    } else if (!strcmp(wpaVer,"WPA2")) {
        e_wpa_ver = QAPI_WLAN_AUTH_WPA2_PSK_E;
	} else if (!strcmp(wpaVer, "SAE")) {
        e_wpa_ver = QAPI_WLAN_AUTH_WPA3_SAE_E;
	} else if (!strcmp(wpaVer,"SAE_WPA2")) {
        e_wpa_ver = QAPI_WLAN_AUTH_WPA2_SAE_MIXED_E;
    } else {
        info_printf("invalid wpa ver =%s\n", wpaVer);
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }
    if (strcmp(ucipher, mcipher)) {
        info_printf("invaid uchipher mcipher, should be same\n");
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }
    if (!strcmp(ucipher, "TKIP")) {
        e_cipher = QAPI_WLAN_CRYPT_TKIP_CRYPT_E;
    } else if (!strcmp(ucipher, "CCMP")) {
        e_cipher = QAPI_WLAN_CRYPT_AES_CRYPT_E;
    } else {
        info_printf("invaid uchipher mcipher, should be TKIP or CCMP\n");
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }
    pg_wifi_shell_cxt->auth = e_wpa_ver;
    qapi_WLAN_Set_Param (deviceId, __QAPI_WLAN_PARAM_GROUP_WIRELESS_SECURITY,
        __QAPI_WLAN_PARAM_GROUP_SECURITY_AUTH_MODE,
        (void *) &e_wpa_ver, sizeof(qapi_WLAN_Auth_Mode_e), FALSE);
    qapi_WLAN_Set_Param(deviceId, __QAPI_WLAN_PARAM_GROUP_WIRELESS_SECURITY,
        __QAPI_WLAN_PARAM_GROUP_SECURITY_ENCRYPTION_TYPE,
        (void *) &e_cipher, sizeof(qapi_WLAN_Crypt_Type_e), FALSE);

    return QAPI_OK;
}

int32_t ether_aton(const char *orig, uint8_t *eth)
{
  const char *bufp;
  int i;

  i = 0;
  for(bufp = orig; *bufp != '\0'; ++bufp) {
    unsigned int val;
    unsigned char c = *bufp++;
    if (isdigit(c)) val = c - '0';
    else if (c >= 'a' && c <= 'f') val = c - 'a' + 10;
    else if (c >= 'A' && c <= 'F') val = c - 'A' + 10;
    else break;

    val <<= 4;
    c = *bufp++;
    if (isdigit(c)) val |= c - '0';
    else if (c >= 'a' && c <= 'f') val |= c - 'a' + 10;
    else if (c >= 'A' && c <= 'F') val |= c - 'A' + 10;
    else break;

    eth[i] = (unsigned char) (val & 0377);
    if(++i == 6) //MAC_LEN
    {
        /* That's it.  Any trailing junk? */
        if (*bufp != '\0') {
            //QCLI_Printf(qcli_wlan_group, "iw_ether_aton(%s): trailing junk!\r\n", orig);
            return(-1);
        }
        return(0);
    }
    if (*bufp != ':')
        break;
  }
  return(-1);
}

static qapi_Status_t Connect(uint32_t __attribute__((__unused__)) Parameter_Count, QAPI_Console_Parameter_t __attribute__((__unused__)) *Parameter_List)
{
    qapi_Status_t ret= QAPI_OK;
    char *bssid = NULL;
    int ssidLength = 0;
    char *ssid = NULL;
	uint8_t deviceId = get_active_device();
	wifi_shell_cxt_t *p_cxt = pg_wifi_shell_cxt;

    if(!p_cxt->wlan_enabled) {
        info_printf("wlan is not enabled \n");
        return QAPI_WLAN_ERR_DEVICE_NOT_FOUND;
    }

    if( Parameter_Count < 1 || !Parameter_List ){
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }

    ssid = Parameter_List[0].String_Value;

    if (Parameter_Count >= 2) {
        bssid = Parameter_List[1].String_Value;
    }

    ssidLength = strlen(ssid);
    qapi_WLAN_Set_Param (0, __QAPI_WLAN_PARAM_GROUP_WIRELESS,
        __QAPI_WLAN_PARAM_GROUP_WIRELESS_SSID,
        (void *)ssid, ssidLength, FALSE);

    if (bssid) {
        uint8_t bssidToConnect[__QAPI_WLAN_MAC_LEN] = {0};
        if (ether_aton(bssid, bssidToConnect) < 0) {
            info_printf("Invalid BSSID to connect\n");
            return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
        }
        qapi_WLAN_Set_Param (0, __QAPI_WLAN_PARAM_GROUP_WIRELESS,
            __QAPI_WLAN_PARAM_GROUP_WIRELESS_BSSID,
            (void *)bssidToConnect, __QAPI_WLAN_MAC_LEN, FALSE);
    }

    info_printf("connect to ssid %s\n", ssid);
    ret = qapi_WLAN_Commit(deviceId);
	if(deviceId == NT_DEV_AP_ID && ret == QAPI_OK) {
		memscpy(p_cxt->ssid, ssidLength, ssid, ssidLength);
        p_cxt->ssid[ssidLength] = 0;
        p_cxt->ssid_length = ssidLength;
	}
    return ret;
}

static qapi_Status_t GetRssi(uint32_t __attribute__((__unused__)) Parameter_Count, QAPI_Console_Parameter_t __attribute__((__unused__)) *Parameter_List)
{
	qapi_Status_t ret = QAPI_ERROR;
	uint8_t rssi = 0;
	uint32_t length = sizeof(rssi);
	uint32_t deviceId = get_active_device();

    if(!pg_wifi_shell_cxt->wlan_enabled) {
        info_printf("wlan is not enabled \n");
        return QAPI_WLAN_ERR_DEVICE_NOT_FOUND;
    }

	ret = qapi_WLAN_Get_Param(deviceId,
							__QAPI_WLAN_PARAM_GROUP_WIRELESS,
							__QAPI_WLAN_PARAM_GROUP_WIRELESS_RSSI,
							&rssi,
							&length);
	if(QAPI_OK == ret)
		info_printf("indicator = %d dB\r\n",rssi);
	return ret;
}

static qapi_Status_t Disconnect(uint32_t __attribute__((__unused__)) Parameter_Count, QAPI_Console_Parameter_t __attribute__((__unused__)) *Parameter_List)
{
    qapi_Status_t ret= QAPI_OK;
	uint8_t deviceId = get_active_device();

    if(!pg_wifi_shell_cxt->wlan_enabled) {
        info_printf("wlan is not enabled \n");
        return QAPI_WLAN_ERR_DEVICE_NOT_FOUND;
    }

    pg_wifi_shell_cxt->auth = QAPI_WLAN_AUTH_NONE_E;
    ret = qapi_WLAN_Disconnect(deviceId);
    return ret;
}

static qapi_Status_t SetChannel(uint32_t __attribute__((__unused__)) Parameter_Count, QAPI_Console_Parameter_t __attribute__((__unused__)) *Parameter_List)
{
	qapi_Status_t ret= QAPI_OK;
	uint8_t deviceId = get_active_device();
	uint32_t channel[2] = {0, 0};

    if(!pg_wifi_shell_cxt->wlan_enabled) {
        info_printf("wlan is not enabled \n");
        return QAPI_WLAN_ERR_DEVICE_NOT_FOUND;
    }

	if( Parameter_Count < 1 || !Parameter_List || !Parameter_List[0].Integer_Is_Valid) {
		return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
	}
	
	channel[0] = Parameter_List[0].Integer_Value;
    if( Parameter_Count >= 2 ) {
#ifdef CONFIG_6GHZ
	    channel[1] = Parameter_List[1].Integer_Value;
#else
        info_printf("cannot set 6g channel since 6g is not enabled \n");
        return QAPI_WLAN_ERR_EINVAL;
#endif
    }
	ret = qapi_WLAN_Set_Param(deviceId,
								__QAPI_WLAN_PARAM_GROUP_WIRELESS,
								__QAPI_WLAN_PARAM_GROUP_WIRELESS_CHANNEL,
								(void *) &channel,
								sizeof(channel),
								FALSE);
	if(ret != QAPI_OK) {
		info_printf("set channel %d fail \n",channel[0]);
	}
	return ret;
}

static qapi_Status_t SetPhyMode(uint32_t __attribute__((__unused__)) Parameter_Count, QAPI_Console_Parameter_t __attribute__((__unused__)) *Parameter_List)
{
	qapi_Status_t ret= QAPI_OK;
	uint8_t deviceId = get_active_device();
	qapi_WLAN_Phy_Mode_e phyMode;
	char *wmode;
	if( Parameter_Count != 1 || !Parameter_List) {
		return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
	}

	wmode = (char *) Parameter_List[0].String_Value;
	if(!strcmp(wmode,"a"))
		phyMode = QAPI_WLAN_11A_MODE_E;
	else if(!strcmp(wmode,"b"))
		phyMode = QAPI_WLAN_11B_MODE_E;
	else if(!strcmp(wmode,"g"))
		phyMode = QAPI_WLAN_11G_MODE_E;
	else if(!strcmp(wmode,"ng"))
		phyMode = QAPI_WLAN_11NG_HT20_MODE_E;
	else if(!strcmp(wmode,"abgn"))
		phyMode = QAPI_WLAN_11ABGN_HT20_MODE_E;
	else {
		info_printf("Unknown wmode, only support a/b/g/ng/abgn/\r\n");
		return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
	}

	ret = qapi_WLAN_Set_Param(deviceId,
               __QAPI_WLAN_PARAM_GROUP_WIRELESS,
               __QAPI_WLAN_PARAM_GROUP_WIRELESS_PHY_MODE,
               &phyMode,
               sizeof(phyMode),
               FALSE);
    return ret;
}

static qapi_Status_t Set11nHTCap(uint32_t __attribute__((__unused__)) Parameter_Count, QAPI_Console_Parameter_t __attribute__((__unused__)) *Parameter_List)
{
	qapi_Status_t ret= QAPI_OK;
	uint8_t deviceId = get_active_device();
	qapi_WLAN_11n_HT_Config_e htconfig;
	char *ht_config;
	if( Parameter_Count != 1 || !Parameter_List || Parameter_List[0].Integer_Is_Valid) {
		return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
	}

	ht_config = (char *)Parameter_List[0].String_Value;
	if(!strcmp(ht_config,"disable"))
		htconfig = QAPI_WLAN_11N_DISABLED_E;
	else if(!strcmp(ht_config,"ht20"))
		htconfig = QAPI_WLAN_11N_HT20_E;
	else {
		info_printf("Unknown ht config, only support disable/ht20\r\n");
		return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
	}
	ret = qapi_WLAN_Set_Param(deviceId,
               __QAPI_WLAN_PARAM_GROUP_WIRELESS,
               __QAPI_WLAN_PARAM_GROUP_WIRELESS_11N_HT,
               &htconfig,
               sizeof(htconfig),
               FALSE);
	return ret;
}

static int32_t set_op_mode(char *opmode, char *hiddenSsid)
{
	int32_t ret = -1;
	uint8_t hidden_flag = 0;
	qapi_WLAN_DEV_Mode_e devMode;
	wifi_shell_cxt_t *p_cxt = pg_wifi_shell_cxt;

	if(!strcmp(opmode,"ap")) {
		devMode = DEV_MODE_AP_E;
		if(strcmp(hiddenSsid,"hidden") == 0) {
			hidden_flag = 1;
		}
		else if(strcmp(hiddenSsid,"0") == 0 || strcmp(hiddenSsid,"") == 0) {
			hidden_flag = 0;
		}
		else {
			return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
		}
	}
	else if(!strcmp(opmode,"station")) {
		devMode = DEV_MODE_STATION_E;
	}
	#ifdef NT_FN_CONCURRENCY
	else if(!strcmp(opmode,"ap_sta")) {
		devMode = DEV_MODE_AP_STA_E;
	}
	#endif
	else {
		info_printf("unknown mode %s\n",opmode);
		return ret;
	}

	ret = qapi_WLAN_Set_Param(0,
							__QAPI_WLAN_PARAM_GROUP_WIRELESS,
							__QAPI_WLAN_PARAM_GROUP_WIRELESS_OPERATION_MODE,
							&devMode,
							sizeof(devMode),
							FALSE);

	if(ret != QAPI_OK) {
		info_printf("set mode %s fail\n",opmode);
		return QAPI_ERROR_CONSOLE_COMMAND_STATUS_ERROR;
	} else {
		if(devMode == DEV_MODE_AP_E)
			p_cxt->active_device = NT_DEV_AP_ID;
		else if(devMode == DEV_MODE_STATION_E)
			p_cxt->active_device = NT_DEV_STA_ID;
	}
	
	if(devMode == DEV_MODE_AP_E) {
		ret = qapi_WLAN_Set_Param(NT_DEV_AP_ID, 
								__QAPI_WLAN_PARAM_GROUP_WIRELESS,
								__QAPI_WLAN_PARAM_GROUP_WIRELESS_AP_ENABLE_HIDDEN_MODE,
								&hidden_flag,
								sizeof(hidden_flag),
								FALSE);
		if(ret != 0) {
			info_printf("Not able to set hidden mode for AP \r\n");
			return QAPI_ERROR_CONSOLE_COMMAND_STATUS_ERROR;
		}
	}
	return ret;
}

static qapi_Status_t SetOperatingMode(uint32_t __attribute__((__unused__)) Parameter_Count, QAPI_Console_Parameter_t __attribute__((__unused__)) *Parameter_List)
{
	char *hidden = "";
    if(!pg_wifi_shell_cxt->wlan_enabled) {
        info_printf("wlan is not enabled \n");
        return QAPI_WLAN_ERR_DEVICE_NOT_FOUND;
    }
	
	if(Parameter_Count < 1 || !Parameter_List) {
		return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
	}

	if(Parameter_Count >= 2)
		hidden = (char *) Parameter_List[1].String_Value;
	return (qapi_Status_t) set_op_mode((char *)Parameter_List[0].String_Value, hidden);
}

static qapi_Status_t SetPowerMode(uint32_t __attribute__((__unused__)) Parameter_Count, QAPI_Console_Parameter_t __attribute__((__unused__)) *Parameter_List)
{
    PRINT_ERR_NOT_SUPPORTED;
    return QAPI_WLAN_ERROR;
}

static qapi_Status_t SetAggregationParameters(uint32_t __attribute__((__unused__)) Parameter_Count, QAPI_Console_Parameter_t __attribute__((__unused__)) *Parameter_List)
{
	qapi_Status_t ret= QAPI_OK;
	uint8_t deviceId = get_active_device();
    qapi_WLAN_Aggregation_Params_t param;
	if( Parameter_Count != 2 || !Parameter_List 
        || !Parameter_List[0].Integer_Is_Valid || !Parameter_List[1].Integer_Is_Valid) {
		return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
	}
	
	param.tx_TID_Mask = Parameter_List[0].Integer_Value;
    param.rx_TID_Mask = Parameter_List[1].Integer_Value;
	if(param.tx_TID_Mask > 0xFF || param.rx_TID_Mask > 0xFF) {
		info_printf("Tha MAX value of tx_TID_Mask and rx_TID_Mask is 0xFF\r\n");
		return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
	}
	ret = qapi_WLAN_Set_Param(deviceId, 
               __QAPI_WLAN_PARAM_GROUP_WIRELESS,
               __QAPI_WLAN_PARAM_GROUP_WIRELESS_ALLOW_TX_RX_AGGR_SET_TID,
               &param,
               sizeof(qapi_WLAN_Aggregation_Params_t),
               FALSE);
    if(ret != QAPI_OK)
        info_printf("Set failed. WLAN should be enabled and please set the parameter before connecting.\r\n");
	return ret;
}


static qapi_Status_t SetAMSDU(uint32_t __attribute__((__unused__)) Parameter_Count, QAPI_Console_Parameter_t __attribute__((__unused__)) *Parameter_List)
{
	qapi_Status_t ret= QAPI_OK;
	uint8_t deviceId = get_active_device();
    uint8_t amsdu_rx_enable = 0;
    
	if( Parameter_Count != 2 || !Parameter_List ) {
		return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
	}
    if(strcmp(Parameter_List[0].String_Value,"rx"))
    {
        info_printf("Parameter should be rx\r\n");
		return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }
    
    if(!strcmp(Parameter_List[1].String_Value,"enable"))
    {
        amsdu_rx_enable = 1;
    }
    else if (!strcmp(Parameter_List[1].String_Value,"disable"))
    {
        amsdu_rx_enable = 0;
    }
	else {
	    return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }

	ret = qapi_WLAN_Set_Param(deviceId, 
               __QAPI_WLAN_PARAM_GROUP_WIRELESS,
               __QAPI_WLAN_PARAM_GROUP_WIRELESS_AMSDU_RX,
               &amsdu_rx_enable,
               sizeof(amsdu_rx_enable),
               FALSE);
    if(ret != QAPI_OK)
        info_printf("Set failed. WLAN should be enabled and please set the parameter before connecting.\r\n");
	return ret;
}

static qapi_Status_t SetPromiscuous(uint32_t __attribute__((__unused__)) Parameter_Count, QAPI_Console_Parameter_t __attribute__((__unused__)) *Parameter_List)
{
    PRINT_ERR_NOT_SUPPORTED;
    return QAPI_WLAN_ERROR;
}

static qapi_Status_t Enable80211v(uint32_t __attribute__((__unused__)) Parameter_Count, QAPI_Console_Parameter_t __attribute__((__unused__)) *Parameter_List)
{
    PRINT_ERR_NOT_SUPPORTED;
    return QAPI_WLAN_ERROR;
}

static qapi_Status_t EnableSuspend(uint32_t __attribute__((__unused__)) Parameter_Count, QAPI_Console_Parameter_t __attribute__((__unused__)) *Parameter_List)
{
    PRINT_ERR_NOT_SUPPORTED;
    return QAPI_WLAN_ERROR;
}

static qapi_Status_t Suspend(uint32_t __attribute__((__unused__)) Parameter_Count, QAPI_Console_Parameter_t __attribute__((__unused__)) *Parameter_List)
{
    PRINT_ERR_NOT_SUPPORTED;
    return QAPI_WLAN_ERROR;
}

qapi_Status_t set_country_code(char *country)
{
	qapi_Status_t ret = QAPI_OK;
    char country_code[4] = {'\0'};

    if (strlen(country) > 3)
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_ERROR;

    memset(&country_code[0], 0, sizeof(country_code));
    memscpy(country_code, strlen(country), country, strlen(country));

    ret = qapi_WLAN_Set_Param(get_active_device(),
                        __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                        __QAPI_WLAN_PARAM_GROUP_WIRELESS_COUNTRY_CODE,
                        &country_code[0],
                        sizeof(country_code),
                        FALSE);

    return ret;
}

static qapi_Status_t SetCountryCode(uint32_t __attribute__((__unused__)) Parameter_Count, QAPI_Console_Parameter_t __attribute__((__unused__)) *Parameter_List)
{
	qapi_Status_t ret = QAPI_OK;
	wifi_shell_cxt_t *p_cxt = pg_wifi_shell_cxt;

    if (0 == p_cxt->wlan_enabled)
    {
        info_printf("Enable WLAN before set country code\r\n");
        return QAPI_ERROR;
    }

	if( Parameter_Count != 1 || !Parameter_List || Parameter_List[0].Integer_Is_Valid) {
		return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }

	ret = set_country_code((char *) Parameter_List[0].String_Value);
	if(ret != QAPI_OK) {
		info_printf("set country code %s fail\n", (char *) Parameter_List[0].String_Value);
	}
	return ret;
}

void print_regulatory_info(qapi_WLAN_Reg_Evt_t *reg_info)
{
	int idx = 0, num;
        uint16_t max_bw = 20;
        char data[32+1] = {'\0'};
	qapi_WLAN_Reg_t *reg;
	if(reg_info)
	{
		info_printf("Country Code: %s\n", reg_info->alpha);
		reg = reg_info->reg_rules;
		num = (reg_info->num_2g_reg_rules) + (reg_info->num_5g_reg_rules);
		for(idx = 0;idx < num;idx++) {
			memset(data, 0, sizeof(data));
			if(reg[idx].ant_gain == 0)
				strlcpy(data, "N/A", sizeof(data));
			else
				snprintf(data, sizeof(data), "%d", reg[idx].ant_gain);
			info_printf("(%d - %d @ %d),(%s,%d)\n",reg[idx].start_freq,reg[idx].end_freq,max_bw,
				data,reg[idx].reg_power,reg[idx].flag_info);
		}
	}
}

static qapi_Status_t GetCountryCode(uint32_t __attribute__((__unused__)) Parameter_Count, QAPI_Console_Parameter_t __attribute__((__unused__)) *Parameter_List)
{
    qapi_Status_t ret = QAPI_OK;
    wifi_shell_cxt_t *p_cxt = pg_wifi_shell_cxt;
    qapi_WLAN_Reg_Evt_t reg_info;

    if (0 == p_cxt->wlan_enabled)
    {
        info_printf("Enable WLAN before get country code\r\n");
        return QAPI_ERROR;
    }

    ret = qapi_WLAN_Get_Regulatory_Info(&reg_info);
    if(ret == QAPI_OK) {
        print_regulatory_info(&reg_info);
    }
    return ret;
}

qapi_Status_t set_tx_power(uint32 txpower, qapi_WLAN_TX_Power_Policy_e policy)
{
	qapi_Status_t ret = QAPI_OK;
    qapi_WLAN_Set_Txpower_Params_t set_tx_power_cfg;
    set_tx_power_cfg.txpower = txpower;
    set_tx_power_cfg.policy = policy;
     
    ret = qapi_WLAN_Set_Param(get_active_device(),
                        __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                        __QAPI_WLAN_PARAM_GROUP_WIRELESS_TX_POWER_IN_DBM,
                        &set_tx_power_cfg,
                        sizeof(set_tx_power_cfg),
                        FALSE);

    return ret;
}

static qapi_Status_t SetTxPower(uint32_t __attribute__((__unused__)) Parameter_Count, QAPI_Console_Parameter_t __attribute__((__unused__)) *Parameter_List)
{
	qapi_Status_t ret = QAPI_OK;
	wifi_shell_cxt_t *p_cxt = pg_wifi_shell_cxt;
	uint8_t policy = 0;
	
    if (0 == p_cxt->wlan_enabled)
    {
        info_printf("Enable WLAN before set tx power\r\n");
        return QAPI_ERROR;
    }

	if ((!Parameter_List) || ( Parameter_Count < 1 ))
    {
		return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }

    if(Parameter_List[0].Integer_Value > UINT8_MAX)
    {
        info_printf("set tx power to %d fail\n", Parameter_List[0].Integer_Value);
        return QAPI_ERROR;
    }

    if (Parameter_Count == 2)
    {
        policy = Parameter_List[1].Integer_Value;
    }
	if (policy >= QAPI_WLAN_POLICY_NUM_E)
		return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;

	ret = set_tx_power( Parameter_List[0].Integer_Value, policy);
    
	if(ret != QAPI_OK) {
		info_printf("set tx power to %d fail\n", Parameter_List[0].Integer_Value);
	}
	return ret;
}

static qapi_Status_t GetTxPower(uint32_t __attribute__((__unused__)) Parameter_Count, QAPI_Console_Parameter_t __attribute__((__unused__)) *Parameter_List)
{
	wifi_shell_cxt_t *p_cxt = pg_wifi_shell_cxt;
	
    if (0 == p_cxt->wlan_enabled)
    {
        info_printf("Enable WLAN before get tx power\r\n");
        return QAPI_ERROR;
    }

	uint8_t deviceId = get_active_device();
    qapi_WLAN_Get_Power_Evt_t power;
    uint32_t length = sizeof(power);

    if(QAPI_OK != qapi_WLAN_Get_Param (deviceId,
                                __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                                __QAPI_WLAN_PARAM_GROUP_WIRELESS_TX_POWER_IN_DBM,
                                &power,
                                &length)){
        info_printf("get tx power fail for device %d\n",deviceId);
        return QAPI_ERROR;
    } else {
        info_printf("get real_power: %d dbm\r\n", power.real_power);
        info_printf("get ctl_power: %d dbm\r\n", power.ctl_power);
        info_printf("get reg_power: %d dbm\r\n", power.reg_power);
        info_printf("get target_power: %d dbm\r\n", power.target_power);
    }
    return QAPI_OK;
}

#if CONFIG_DEBUG_CMD_XPA
extern uint8_t halphy_xpa_enabled(uint8_t enable, uint8_t band);
extern uint8_t halphy_xpa_enable(uint8_t enable, uint8_t band);
static qapi_Status_t EnableXpa(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
	uint8_t enable, band;
	if(Parameter_Count < 2 || !Parameter_List || !Parameter_List[0].Integer_Is_Valid || !Parameter_List[1].Integer_Is_Valid) {
		return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
	}

	enable = Parameter_List[0].Integer_Value;
	band = Parameter_List[1].Integer_Value;
	halphy_xpa_enable(enable, band);

	return QAPI_OK;
}
#endif

static qapi_Status_t SetRate(uint32_t __attribute__((__unused__)) Parameter_Count, QAPI_Console_Parameter_t __attribute__((__unused__)) *Parameter_List)
{
    qapi_WLAN_Set_Rate_Params_t set_rate_cfg;

    memset(&set_rate_cfg, 0, sizeof(qapi_WLAN_Set_Rate_Params_t));

    if(!pg_wifi_shell_cxt->wlan_enabled) {
        info_printf("wlan is not enabled \n");
        return QAPI_WLAN_ERR_DEVICE_NOT_FOUND;
    }

    if (!Parameter_List)
    {
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }

    if (memcmp(Parameter_List[0].String_Value, "auto", sizeof("auto")) == 0)
    {
        if (Parameter_Count < 1)
        {
            return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
        }

        set_rate_cfg.ra_ON = 1;
    } else
    {
        if (Parameter_Count < 4)
        {
            return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
        }

        set_rate_cfg.ra_ON = 0;
        set_rate_cfg.rate_staid = Parameter_List[0].Integer_Value;
        set_rate_cfg.rate_p_rate = Parameter_List[1].Integer_Value;
        set_rate_cfg.rate_s_rate = Parameter_List[2].Integer_Value;
        set_rate_cfg.rate_t_rate = Parameter_List[3].Integer_Value;
    }

    qapi_WLAN_Set_Rate(&set_rate_cfg);

    return QAPI_OK;
}

static qapi_Status_t GetRate(uint32_t __attribute__((__unused__)) Parameter_Count, QAPI_Console_Parameter_t __attribute__((__unused__)) *Parameter_List)
{
    qapi_Status_t ret = QAPI_OK;
    qapi_WLAN_Set_Rate_Params_t set_rate_cfg;

    memset(&set_rate_cfg, 0, sizeof(qapi_WLAN_Set_Rate_Params_t));

    if(!pg_wifi_shell_cxt->wlan_enabled) {
        info_printf("wlan is not enabled \n");
        return QAPI_WLAN_ERR_DEVICE_NOT_FOUND;
    }

    if ((!Parameter_List) || (Parameter_Count < 1))
    {
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }

    set_rate_cfg.rate_staid = Parameter_List[0].Integer_Value;

    ret = qapi_WLAN_Get_Rate(&set_rate_cfg);
    if(ret == QAPI_OK) {
        info_printf("p_rate=%d, s_rate=%d, t_rate=%d\n", \
                         set_rate_cfg.rate_p_rate, \
                         set_rate_cfg.rate_s_rate, \
                         set_rate_cfg.rate_t_rate);
    }

    return QAPI_OK;
}

static qapi_Status_t setSTAListenInterval(uint32_t __attribute__((__unused__)) Parameter_Count, QAPI_Console_Parameter_t __attribute__((__unused__)) *Parameter_List)
{
    uint8_t deviceId = get_active_device();
    qapi_WLAN_Listen_Interval_Params_t listen_interval;

    if(Parameter_Count != 2 || !Parameter_List || !Parameter_List[0].Integer_Is_Valid || !Parameter_List[1].Integer_Is_Valid) {
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }

    if (Parameter_List[0].Integer_Value > UINT16_MAX || Parameter_List[0].Integer_Value < 0) {
        info_printf("listen interval need set 0-65535 TU\r\n");
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }
    if (!(Parameter_List[1].Integer_Value == 0 || Parameter_List[1].Integer_Value == 1)) {
        info_printf("round type need set to 0 or 1\r\n");
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }

    listen_interval.time = (uint16_t)Parameter_List[0].Integer_Value;
    listen_interval.round_type = (uint16_t)Parameter_List[1].Integer_Value;

    if (0 != qapi_WLAN_Set_Param (deviceId,
                                __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                                __QAPI_WLAN_PARAM_GROUP_WIRELESS_STA_LISTEN_INTERVAL_IN_TU,
                                &listen_interval,
                                sizeof(listen_interval),
                                FALSE))
    {
        info_printf("set STA listen interval fail\r\n");
        return -1;
    }
    return 0;
}

static qapi_Status_t getSTAListenInterval(uint32_t __attribute__((__unused__)) Parameter_Count, QAPI_Console_Parameter_t __attribute__((__unused__)) *Parameter_List)
{
    uint8_t deviceId = get_active_device();
    uint32_t listen_interval;
    uint32_t length = sizeof(listen_interval);
    if(QAPI_OK != qapi_WLAN_Get_Param (deviceId,
                                __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                                __QAPI_WLAN_PARAM_GROUP_WIRELESS_STA_LISTEN_INTERVAL_IN_TU,
                                &listen_interval,
                                &length)){
        info_printf("get listen interval fail for device %d\n",deviceId);
        return -1;
    } else {
        info_printf("get listen interval: %d TU\r\n", listen_interval);
    }
    return 0;
}

#ifdef CONFIG_MGMT_FILTER_DEMO
static void print_mgmt_frames(void)
{
    uint32_t i, j;

	for (i=0; i<10; i++)
	{
	    if (mgmt_frame_recv_buf[i].len == 0)
	    {
	        continue;
	    }
		
	    printf("Recv Frame len %d: ", mgmt_frame_recv_buf[i].len);
      	for (j=0; j<mgmt_frame_recv_buf[i].len; j++)
      	{
            if (j%16 == 0)
			{
				printf("\r\n");
			}
      	    printf("%02x", *(mgmt_frame_recv_buf[i].data+j));
      	}		
		printf("\r\n\r\n");
	}
}


static qapi_Status_t setMgmtFilter(uint32_t __attribute__((__unused__)) Parameter_Count, QAPI_Console_Parameter_t __attribute__((__unused__)) *Parameter_List)
{
    if(Parameter_Count < 1 || !Parameter_List || !Parameter_List[0].Integer_Is_Valid || Parameter_Count > 1 ) {
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }

    if (Parameter_List[0].Integer_Value == -1)
    {
		print_mgmt_frames();
		return QAPI_OK;
    }
	
    if ((Parameter_List[0].Integer_Value != QAPI_WLAN_MGMT_NONE_E)
		  && (Parameter_List[0].Integer_Value != QAPI_WLAN_MGMT_ASSOC_RESP_E) 
		  && (Parameter_List[0].Integer_Value != QAPI_WLAN_MGMT_PROBE_RESP_E)
		  && ((Parameter_List[0].Integer_Value != (QAPI_WLAN_MGMT_ASSOC_RESP_E | QAPI_WLAN_MGMT_PROBE_RESP_E)))) {
        info_printf("management frame type need set 0, 1, 2, 3 or -1\r\n");
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }

	mgmt_frame_recv_filter = Parameter_List[0].Integer_Value;

	if (mgmt_frame_recv_filter != QAPI_WLAN_MGMT_NONE_E)
	{
		mgmt_frame_recv_enabled = 1;		
        qurt_signal_set(&mgmt_filter_start, MGMT_FILTER_MASK_START);
	}
	else
	{
		mgmt_frame_recv_enabled = 0;
	}
    return QAPI_OK;
}
#endif

int32_t set_ap_beacon_interval(uint32_t beacon_int_in_tu)
{
	uint32_t length = sizeof(qapi_WLAN_DEV_Mode_e);
	qapi_WLAN_DEV_Mode_e opmode;
	uint8_t deviceId = get_active_device();
	
	if(QAPI_OK != qapi_WLAN_Get_Param (deviceId, 
								__QAPI_WLAN_PARAM_GROUP_WIRELESS,
								__QAPI_WLAN_PARAM_GROUP_WIRELESS_OPERATION_MODE,
								&opmode,
								&length)){
		info_printf("get operation mode fail for device %d\n",deviceId);
		return -1;
	}
	if(opmode != DEV_MODE_AP_E) {
		info_printf("Please Set AP Mode to apply AP settings\r\n");
		return -1;
	}
	
	if((beacon_int_in_tu < 100) || (beacon_int_in_tu > 1000)) {
		info_printf("beacon interval has to be within 100-1000 in units of ms \r\n");
		return -1;
	}
	if (0 != qapi_WLAN_Set_Param (deviceId,
								__QAPI_WLAN_PARAM_GROUP_WIRELESS,
								__QAPI_WLAN_PARAM_GROUP_WIRELESS_AP_BEACON_INTERVAL_IN_TU,
								&beacon_int_in_tu,
								sizeof(beacon_int_in_tu),  
								FALSE))
	{
		info_printf("set beacon interval fail\r\n");
		return -1;
	}
	return 0;	
}

static qapi_Status_t setAPBeaconInterval(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
	if(!pg_wifi_shell_cxt->wlan_enabled) {
        info_printf("wlan is not enabled \n");
        return QAPI_WLAN_ERR_DEVICE_NOT_FOUND;
    }
	
    if (Parameter_Count < 1 || !Parameter_List || !Parameter_List[0].Integer_Is_Valid) {
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }
    
	if(0 != set_ap_beacon_interval(Parameter_List[0].Integer_Value)){
		return QAPI_ERROR_CONSOLE_COMMAND_STATUS_ERROR;
	}

    return QAPI_OK;
}

int32_t set_ap_dtim_period(uint32_t dtim_period)
{
	uint32_t length = sizeof(qapi_WLAN_DEV_Mode_e);
	qapi_WLAN_DEV_Mode_e opmode;
	uint8_t deviceId = get_active_device();
	if(QAPI_OK != qapi_WLAN_Get_Param (deviceId, 
								__QAPI_WLAN_PARAM_GROUP_WIRELESS,
								__QAPI_WLAN_PARAM_GROUP_WIRELESS_OPERATION_MODE,
								&opmode,
								&length)){
		info_printf("get operation mode fail for device %d\n",deviceId);
		return -1;
	}
	if(opmode != DEV_MODE_AP_E) {
		info_printf("Please Set AP Mode to apply AP settings\r\n");
		return -1;
	}
	
	if((dtim_period < 1) || (dtim_period > 255)) {
		info_printf("DTIM period has to be within 1-255\r\n");
		return -1;
	}
	if (0 != qapi_WLAN_Set_Param (deviceId,
								__QAPI_WLAN_PARAM_GROUP_WIRELESS,
								__QAPI_WLAN_PARAM_GROUP_WIRELESS_AP_DTIM_INTERVAL,
								&dtim_period,
								sizeof(dtim_period),  
								FALSE))
	{
		info_printf("set DTIM period fail\r\n");
		return -1;
	}
	return 0;	
}

static qapi_Status_t setAPDtimPeriod(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    if(!pg_wifi_shell_cxt->wlan_enabled) {
        info_printf("wlan is not enabled \n");
        return QAPI_WLAN_ERR_DEVICE_NOT_FOUND;
    }
	
    if (Parameter_Count < 1 || !Parameter_List || !Parameter_List[0].Integer_Is_Valid) {
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }
    
	if(0 != set_ap_dtim_period(Parameter_List[0].Integer_Value)){
		return QAPI_ERROR_CONSOLE_COMMAND_STATUS_ERROR;
	}

    return QAPI_OK;
}

int32_t set_ap_inactivity_period(uint32_t inactivity_time_in_mins)
{
	uint32_t length = sizeof(qapi_WLAN_DEV_Mode_e);
	qapi_WLAN_DEV_Mode_e opmode;
	uint8_t deviceId = get_active_device();
	if(QAPI_OK != qapi_WLAN_Get_Param (deviceId, 
								__QAPI_WLAN_PARAM_GROUP_WIRELESS,
								__QAPI_WLAN_PARAM_GROUP_WIRELESS_OPERATION_MODE,
								&opmode,
								&length)){
		info_printf("get operation mode fail for device %d\n",deviceId);
		return -1;
	}
	if(opmode != DEV_MODE_AP_E) {
		info_printf("Please Set AP Mode to apply AP settings\r\n");
		return -1;
	}
	
	if(inactivity_time_in_mins < 1) {
		info_printf("inactivity time should not be 0\r\n");
		return -1;
	}
	if (0 != qapi_WLAN_Set_Param (deviceId,
								__QAPI_WLAN_PARAM_GROUP_WIRELESS,
								__QAPI_WLAN_PARAM_GROUP_WIRELESS_AP_INACTIVITY_TIME_IN_MINS,
								&inactivity_time_in_mins,
								sizeof(inactivity_time_in_mins),  
								FALSE))
	{
		info_printf("set inactivity period fail\r\n");
		return -1;
	}
	return 0;	
}

static qapi_Status_t setAPInactivityPeriod(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    if(!pg_wifi_shell_cxt->wlan_enabled) {
        info_printf("wlan is not enabled \n");
        return QAPI_WLAN_ERR_DEVICE_NOT_FOUND;
    }
	
    if (Parameter_Count < 1 || !Parameter_List || !Parameter_List[0].Integer_Is_Valid) {
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }
    
	if(0 != set_ap_inactivity_period(Parameter_List[0].Integer_Value)){
		return QAPI_ERROR_CONSOLE_COMMAND_STATUS_ERROR;
	}

    return QAPI_OK;
}

extern uint8_t ecsa_ap_chan_switch(uint8_t mode,uint8_t count,uint8_t ch_no,uint8_t is_6g);
extern void ecsa_set_type(int type);

static qapi_Status_t setCSAType(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    if(!pg_wifi_shell_cxt->wlan_enabled) {
        info_printf("wlan is not enabled \n");
        return QAPI_WLAN_ERR_DEVICE_NOT_FOUND;
    }
	
    if (Parameter_Count < 1 || !Parameter_List || !Parameter_List[0].Integer_Is_Valid) {
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }
    
	ecsa_set_type(Parameter_List[0].Integer_Value);

    return QAPI_OK;
}

static qapi_Status_t channelSwitch(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
	uint8_t mode, count, ch_no, is_6g = 0;
    if(!pg_wifi_shell_cxt->wlan_enabled) {
        info_printf("wlan is not enabled \n");
        return QAPI_WLAN_ERR_DEVICE_NOT_FOUND;
    }
	
    if (Parameter_Count < 3 || !Parameter_List || !Parameter_List[0].Integer_Is_Valid || !Parameter_List[1].Integer_Is_Valid || !Parameter_List[2].Integer_Is_Valid) {
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }

	ch_no = Parameter_List[0].Integer_Value;
	count = Parameter_List[1].Integer_Value;
	mode = Parameter_List[2].Integer_Value;

	if(Parameter_Count > 3 && Parameter_List[0].Integer_Is_Valid)
		is_6g = Parameter_List[3].Integer_Value;
	
	if(ecsa_ap_chan_switch(mode, count, ch_no, is_6g) != 0) {
		return QAPI_ERROR_CONSOLE_COMMAND_STATUS_ERROR;
	}
	
    return QAPI_OK;
}
static qapi_Status_t sendRawFrame(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    uint8_t rate_index = 0, tries = 0, header_type = 0;
	//uint8_t deviceId = get_active_device();
    uint32_t i = 0, chan = 0, size = 0;
    int32_t status = -1;
    uint8_t addr[4][6];
    qapi_WLAN_Raw_Send_Params_t rawSendParams;
	
	/*Only for test of self-defined frame*/
	uint8_t probe_req_str[70]={ 
						/*FC*/
						0x40, 0x00,
						/*Duration*/
						0x00, 0x00,
						/*Addr1*/
						0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
						/*Addr2*/
						0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 
						/*Addr3*/
						0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
						/*SEQ*/
						0x00, 0x00,
						/*SSID */
						0x00,0x00,
						/*supported rates*/
						0x01, 0x08, 0x02, 0x04 ,0x0b, 0x16, 0x8c, 0x92, 0x98, 0xa4,
						/*extended supported Rates*/
						0x32 ,0xC8, 0x5C ,0x9B ,0x31, 0xB6, 0x16, 0x0D ,0xFC, 0xB2, 
						0xC0, 0x8B, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00 ,0x00 ,0x98 ,
						0x0D ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0xC0 ,0x06 ,0x00 ,0x88 ,0x01,0x66,0x15,0x0B 
			};
  

    if((Parameter_Count < 5) || (Parameter_Count > 9)){
      goto raw_usage;
    }

    rate_index = Parameter_List[0].Integer_Value;
    tries = Parameter_List[1].Integer_Value;
    size = Parameter_List[2].Integer_Value;
    chan = Parameter_List[3].Integer_Value;
    header_type = (uint8_t)strtol((const char *)Parameter_List[4].String_Value, NULL, 16);
    memset (addr, 0, sizeof(addr));
	
	if(header_type == 0xff){
		rawSendParams.data = malloc(sizeof(probe_req_str));
		memcpy(rawSendParams.data, probe_req_str, sizeof(probe_req_str));
    	rawSendParams.data_Length = sizeof(probe_req_str);
	}
	else{
		rawSendParams.data = NULL;
    	rawSendParams.data_Length = 0;
	}
	
    for(i = 0; i < (Parameter_Count-5);i++) {
        if(ether_aton((const char *)Parameter_List[5+i].String_Value, &(addr[i][0])))
        {
            info_printf("ERROR: MAC address translation failed.\r\n");
            return status;
        }
    }

    if( Parameter_Count == 5 )
    {
            addr[0][0] = 0xff;
            addr[0][1] = 0xff;
            addr[0][2] = 0xff;
            addr[0][3] = 0xff;
            addr[0][4] = 0xff;
            addr[0][5] = 0xff;
            addr[1][0] = 0x00;
            addr[1][1] = 0x03;
            addr[1][2] = 0x7f;
            addr[1][3] = 0xdd;
            addr[1][4] = 0xdd;
            addr[1][5] = 0xdd;
            addr[2][0] = 0x00;
            addr[2][1] = 0x03;
            addr[2][2] = 0x7f;
            addr[2][3] = 0xdd;
            addr[2][4] = 0xdd;
            addr[2][5] = 0xdd;
            addr[3][0] = 0x00;
            addr[3][1] = 0x03;
            addr[3][2] = 0x7f;
            addr[3][3] = 0xee;
            addr[3][4] = 0xee;
            addr[3][5] = 0xee;
            if(header_type == 2) {
                memcpy(&addr[0][0], &addr[1][0], __QAPI_WLAN_MAC_LEN);
                //change destination address
                addr[2][3] = 0xaa;
                addr[2][4] = 0xaa;
                addr[2][5] = 0xaa;
            }
    }

    rawSendParams.rate_Index = rate_index;
    rawSendParams.num_Tries = tries;
    rawSendParams.payload_Size = size;
    rawSendParams.channel = chan;
    rawSendParams.header_Type = header_type;
    rawSendParams.seq = 0;
    memcpy(&rawSendParams.addr1[0], addr[0], __QAPI_WLAN_MAC_LEN);
    memcpy(&rawSendParams.addr2[0], addr[1], __QAPI_WLAN_MAC_LEN);
    memcpy(&rawSendParams.addr3[0], addr[2], __QAPI_WLAN_MAC_LEN);
    memcpy(&rawSendParams.addr4[0], addr[3], __QAPI_WLAN_MAC_LEN);
    

    status = qapi_WLAN_Raw_Send(&rawSendParams);
    if( status == QAPI_WLAN_ERROR)
    {
raw_usage:
       info_printf("raw input error\r\n");
       info_printf("usage = WLAN SendRawFrame rate num_tries num_bytes channel header_type [addr1 [addr2 [addr3 [addr4]]]]\r\n");
	   info_printf("example = sendrawframe 1 10 30 11 1 00:03:7f:cc:cc:cc 00:03:7f:dd:dd:dd 00:03:7f:dd:dd:dd 00:aa:bb:28:43:91 \r\n");
	   info_printf("NOTICE: 1. if the addr not given, will use default addr; 2. broadcast frame will only be sent once\r\n");
       info_printf("rate = rate index where 0==1mbps; 1==2mbps; 2==5.5mbps etc(this value will not take effect when connected)\r\n");
       info_printf("num_tries = number of transmits 1 - 14(broadcast frames will be sent only once)\r\n");
       info_printf("num_bytes = payload size 0 to 1400\r\n");
       info_printf("channel = 0 - 11 for 2g; 36- for 5g (this value will not take effect when connected)\r\n");
       info_printf("header_type = 0==beacon frame; 1== Probe Request; 2==QOS data frame; 3==4 address data framel; ff==self-defined frame\r\n");
       info_printf("addr1 = mac address xx:xx:xx:xx:xx:xx, default ff:ff:ff:ff:ff:ff\r\n");
       info_printf("addr2 = mac address xx:xx:xx:xx:xx:xx, default 00:03:7f:dd:dd:dd\r\n");
       info_printf("addr3 = mac address xx:xx:xx:xx:xx:xx, default 00:03:7f:dd:dd:dd, QoS changed to 00:03:7f:aa:aa:aa\r\n");
       info_printf("addr4 = mac address xx:xx:xx:xx:xx:xx, default 00:03:7f:ee:ee:ee\r\n");              
    }

	if((header_type == 0xff) 
		&& (rawSendParams.data != NULL))
	{	
		free(rawSendParams.data);
	}
    
    return status;
}

uint8_t ascii_to_hex(char val)
{
    if('0' <= val && '9' >= val)
    {
        return (uint8_t)(val - '0');
    }
    else if('a' <= val && 'f' >= val)
    {
        return (uint8_t)((val - 'a') + 0x0a);
    }
    else if('A' <= val && 'F' >= val)
    {
        return (uint8_t)((val - 'A') + 0x0a);
    }
    return 0xff;/* Error */
}

int32_t set_app_ie(uint32_t __attribute__((__unused__)) Parameter_Count, QAPI_Console_Parameter_t __attribute__((__unused__)) *Parameter_List)
{
    uint8_t enet_device = get_active_device();
    uint32_t dataLen = sizeof(qapi_WLAN_DEV_Mode_e);
    qapi_WLAN_DEV_Mode_e wifimode;
    int32_t return_code = 0;
    uint32_t length = 0, i = 0;
    uint8 tmpvalue1 = 0, tmpvalue2 = 0;
    qapi_WLAN_App_Ie_Params_t ie_params;

    if(QAPI_OK != qapi_WLAN_Get_Param (enet_device, 
                                __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                                __QAPI_WLAN_PARAM_GROUP_WIRELESS_OPERATION_MODE,
                                &wifimode,
                                &dataLen)){
        info_printf("get operation mode fail for device %d\n", enet_device);
        return -1;
    }

    ie_params.mgmt_Frame_Type = Parameter_List[0].Integer_Value;

    if ((wifimode == DEV_MODE_STATION_E) && (ie_params.mgmt_Frame_Type != QAPI_WLAN_FRAME_ASSOC_REQ_E))
    {
        info_printf("In station mode, application specified information element can be added only in association request frames\r\n");
        return -1;
    }

    if ((wifimode == DEV_MODE_AP_E) &&
        ((ie_params.mgmt_Frame_Type != QAPI_WLAN_FRAME_BEACON_E) && (ie_params.mgmt_Frame_Type != QAPI_WLAN_FRAME_PROBE_RESP_E)))
    {
        info_printf("In soft-AP mode, application specified information element can be added only in beacon and probe response frames\r\n");
        return -1;
    }

    length= strlen((char *)Parameter_List[1].String_Value);
    if (length < 2)
    {
        info_printf("Invalid application specified information element length. Application specified information element must start with 'dd'\r\n");
        return -1;
    }

    if (length % 2 !=0)
    {
        info_printf("Invalid application specified information element length. The length must be a multiple of two.\r\n");
        return -1;
    }

    /* The length must be not less than 10 as every two input characters are converted into a hex number 
     * and a valid application information element at least has element ID, length and OUI per 802.11 spec.
     */
    if(length > 2 && length < 10)
    {
        info_printf("The input characters cannot be converted into a valid application element information.\r\n");
        info_printf("The input characters should follow the format:Element ID(1 byte)|Length(1 byte)|OUI(3 bytes)|Vendor-specific content((Length-3)bytes).\r\n");
        return -1;
    }

    ie_params.ie_Len = length/2;
	
    if ((strncmp((char *)(Parameter_List[1].String_Value), "dd", 2) != 0))
    {
        info_printf("Application specified information element must start with 'dd'\r\n");
        return -1;
    }

    ie_params.ie_Info = (uint8_t *)malloc(ie_params.ie_Len + 1);
    for(i = 0; i < ie_params.ie_Len; i++)
    {
        tmpvalue1 = ascii_to_hex(Parameter_List[1].String_Value[2*i]);
        tmpvalue2 = ascii_to_hex(Parameter_List[1].String_Value[2*i+1]);
        if(tmpvalue1 == 0xff ||tmpvalue2 == 0xff)
        {
            free(ie_params.ie_Info);
            info_printf("The characters of Application specified information element only be '0-9', 'a-f' and 'A-F'.\r\n");
            return -1;
        }
        ie_params.ie_Info[i] = ((tmpvalue1<<4)&0xf0)|(tmpvalue2&0xf);
    }

    /* The length in application information element should be the length of OUI + vendor-specific content*/
    if((ie_params.ie_Len > 1) && (ie_params.ie_Info[1] != (ie_params.ie_Len -2)))
    {	
        free(ie_params.ie_Info);
        info_printf("The length in application information element is not correct, it should be the length of OUI + vendor-specific content. \r\n");
        return -1;
    }
	
    ie_params.ie_Info[ie_params.ie_Len] = '\0';
    return_code = qapi_WLAN_Set_Param (enet_device,
                                       __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                                       __QAPI_WLAN_PARAM_GROUP_WIRELESS_APP_IE,
                                       &ie_params,
                                       sizeof(qapi_WLAN_App_Ie_Params_t),
                                       FALSE);
    free(ie_params.ie_Info);
    return return_code;
}

static qapi_Status_t setApplicationIe(uint32_t __attribute__((__unused__)) Parameter_Count, QAPI_Console_Parameter_t __attribute__((__unused__)) *Parameter_List)
{
    if(Parameter_Count < 2 || !Parameter_List || !Parameter_List[0].Integer_Is_Valid)
    {
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }
    if (0 == set_app_ie(Parameter_Count, Parameter_List))
    {
       return QAPI_OK;
    }
    return QAPI_ERROR;
}

#define RT_IDX_11B_LONG_1_MBPS 0
#define RT_IDX_11A_6_MBPS 1
#define RT_IDX_11A_12_MBPS 2
// use this command after 2G connection
static qapi_Status_t setAntiInfParam(uint32_t __attribute__((__unused__)) Parameter_Count, QAPI_Console_Parameter_t __attribute__((__unused__)) *Parameter_List)
{
    uint8_t deviceId = get_active_device();
    uint32_t enable = 1;
    uint32_t rts_rate = RT_IDX_11B_LONG_1_MBPS;
    qapi_WLAN_Edca_Params_t edca_param_cfg;
    uint32_t threshold = 60;
    qapi_WLAN_BA_Window_Params_t ba_win_size_cfg;
    uint32_t slot_time = 20;

    edca_param_cfg.qid = 0xff; //set queue 0 - 7
    edca_param_cfg.aifsn = 0x3;
    edca_param_cfg.cw_min = 0x2;  // cwmin = 2^2 -1
    edca_param_cfg.cw_max = 0x4;  // cwmax = 2^4 - 1
    edca_param_cfg.txop_limit = 200;

    ba_win_size_cfg.ack_timeout = 128; //128us, should less than 4096
    ba_win_size_cfg.delay = 10; //10 * 2 * SM clock cycles, should less than 64

    if(Parameter_Count < 1 || !Parameter_List || !Parameter_List[0].Integer_Is_Valid)
    {
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }

    rts_rate = Parameter_List[0].Integer_Value;

    if (0 != qapi_WLAN_Set_Param (deviceId,
                                __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                                __QAPI_WLAN_PARAM_GROUP_WIRELESS_RTS,
                                &enable,
                                sizeof(enable),
                                FALSE))
    {
        info_printf("Enable RTS/CTS fail\r\n");
        info_printf("1:enable  0:disable\r\n");
        return -1;
    }

    if (0 != qapi_WLAN_Set_Param (deviceId,
                                __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                                __QAPI_WLAN_PARAM_GROUP_WIRELESS_RTS_RATE_2G,
                                &rts_rate,
                                sizeof(rts_rate),
                                FALSE))
    {
        info_printf("fix RTS rate fail\r\n");
        info_printf("0:1Mbps  1:6Mbps 2:12Mbps\r\n");
        return -1;
    }

    if (0 != qapi_WLAN_Set_Param (deviceId,
                                __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                                __QAPI_WLAN_PARAM_GROUP_WIRELESS_EDCA_PARAM,
                                &edca_param_cfg,
                                sizeof(edca_param_cfg),
                                FALSE))
    {
        info_printf("set edca param fail\r\n");
        info_printf("set qid = 0xff for all queue; set qid = 0-7 for single queue\r\n");
        return -1;
    }

    if (0 != qapi_WLAN_Set_Param (deviceId,
                                __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                                __QAPI_WLAN_PARAM_GROUP_WIRELESS_PER_UPPER_THRESHOLD,
                                &threshold,
                                sizeof(threshold),
                                FALSE))
    {
        info_printf("set per upper threshold fail\r\n");
        info_printf("threshold should less than 100\r\n");
        return -1;
    }

    if (0 != qapi_WLAN_Set_Param (deviceId,
                                __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                                __QAPI_WLAN_PARAM_GROUP_WIRELESS_BA_WINDOW,
                                &ba_win_size_cfg,
                                sizeof(ba_win_size_cfg),
                                FALSE))
    {
        info_printf("set BA window size fail\r\n");
        info_printf("ack_timeout should less than 4096\r\n");
        info_printf("delay should less than 64\r\n");
        return -1;
    }

    if (0 != qapi_WLAN_Set_Param (deviceId,
                                __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                                __QAPI_WLAN_PARAM_GROUP_WIRELESS_SLOT_TIME,
                                &slot_time,
                                sizeof(slot_time),
                                FALSE))
    {
        info_printf("set slot time fail\r\n");
        info_printf("set slot time to 9us or 20us\r\n");
        return -1;
    }

    info_printf("setAntiInfParam success\r\n");
    return 0;
}

static qapi_Status_t getAntiInfParam(uint32_t __attribute__((__unused__)) Parameter_Count, QAPI_Console_Parameter_t __attribute__((__unused__)) *Parameter_List)
{
    uint8_t deviceId = get_active_device();
    uint32_t enable;
    uint32_t rts_rate;
    qapi_WLAN_Edca_Params_t edca_param_cfg;
    uint32_t threshold;
    qapi_WLAN_BA_Window_Params_t ba_win;
    uint32_t slot_time;
    uint32_t length;
    edca_param_cfg.qid = 0xff;
    length = sizeof(enable);
    if(QAPI_OK != qapi_WLAN_Get_Param (deviceId,
                                __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                                __QAPI_WLAN_PARAM_GROUP_WIRELESS_RTS,
                                &enable,
                                &length)){
        info_printf("get RTS enable fail for device %d\n",deviceId);
        return -1;
    } else {
        if (enable)
            info_printf("RTS enable\r\n");
        else
            info_printf("RTS disable\r\n");;
    }

    length = sizeof(rts_rate);
    if(QAPI_OK != qapi_WLAN_Get_Param (deviceId,
                                __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                                __QAPI_WLAN_PARAM_GROUP_WIRELESS_RTS_RATE_2G,
                                &rts_rate,
                                &length)){
        info_printf("get RTS rate fail for device %d\n",deviceId);
        return -1;
    } else {
        info_printf("RTS rate: %dMbps\r\n", rts_rate);
    }

    length = sizeof(edca_param_cfg);
    if(QAPI_OK != qapi_WLAN_Get_Param (deviceId,
                                __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                                __QAPI_WLAN_PARAM_GROUP_WIRELESS_EDCA_PARAM,
                                &edca_param_cfg,
                                &length)){
        info_printf("get contention window size fail for device %d\n",deviceId);
        return -1;
    } else {
        info_printf("contention window size -- qid:%d, cw_min:%d, cw_max:%d\r\n", edca_param_cfg.qid, edca_param_cfg.cw_min, edca_param_cfg.cw_max);
    }

    length = sizeof(threshold);
    if(QAPI_OK != qapi_WLAN_Get_Param (deviceId,
                                __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                                __QAPI_WLAN_PARAM_GROUP_WIRELESS_PER_UPPER_THRESHOLD,
                                &threshold,
                                &length)){
        info_printf("get per upper threshold fail for device %d\n",deviceId);
        return -1;
    } else {
        info_printf("per upper threshold:%d\r\n", threshold);
    }

    length = sizeof(ba_win);
    if(QAPI_OK != qapi_WLAN_Get_Param (deviceId,
                                __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                                __QAPI_WLAN_PARAM_GROUP_WIRELESS_BA_WINDOW,
                                &ba_win,
                                &length)){
        info_printf("get per upper ba window size fail for device %d\n",deviceId);
        return -1;
    } else {
        info_printf("ba window size -- ack_timeout:%dus, delay:%d SM clock cycles\r\n", ba_win.ack_timeout, 2 * ba_win.delay);
    }

    length = sizeof(slot_time);
    if(QAPI_OK != qapi_WLAN_Get_Param (deviceId,
                                __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                                __QAPI_WLAN_PARAM_GROUP_WIRELESS_SLOT_TIME,
                                &slot_time,
                                &length)){
        info_printf("get slot time fail for device %d\n",deviceId);
        return -1;
    } else {
        info_printf("slot time:%dus\r\n", slot_time);
    }
    return 0;
}

static qapi_Status_t setEdcaParam(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    uint8_t deviceId = get_active_device();
    qapi_WLAN_Edca_Params_t edca_param_cfg;
    if(!pg_wifi_shell_cxt->wlan_enabled) {
        /* edca should be set after connectting */
        info_printf("wlan is not enabled \n");
        return QAPI_WLAN_ERR_DEVICE_NOT_FOUND;
    }
    if(Parameter_Count < 5 || !Parameter_List || !Parameter_List[0].Integer_Is_Valid || !Parameter_List[1].Integer_Is_Valid ||
        !Parameter_List[2].Integer_Is_Valid || !Parameter_List[3].Integer_Is_Valid || !Parameter_List[4].Integer_Is_Valid)
    {
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }

    edca_param_cfg.qid = Parameter_List[0].Integer_Value;
    edca_param_cfg.aifsn = Parameter_List[1].Integer_Value;
    edca_param_cfg.cw_min = Parameter_List[2].Integer_Value;
    edca_param_cfg.cw_max = Parameter_List[3].Integer_Value;
    edca_param_cfg.txop_limit = Parameter_List[4].Integer_Value;

    if (0 != qapi_WLAN_Set_Param (deviceId,
                                __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                                __QAPI_WLAN_PARAM_GROUP_WIRELESS_EDCA_PARAM,
                                &edca_param_cfg,
                                sizeof(edca_param_cfg),
                                FALSE))
    {
        info_printf("set edca param fail, check the wlan connection\r\n");
        info_printf("set qid = 0xff for all queue; set qid = 0-7 for single queue\r\n");
        return QAPI_ERROR;
    }
        return QAPI_OK;
}

static qapi_Status_t getEdcaParam(uint32_t __attribute__((__unused__)) Parameter_Count, QAPI_Console_Parameter_t __attribute__((__unused__)) *Parameter_List)
{
    uint32_t length;
    uint8_t deviceId = get_active_device();
    qapi_WLAN_Edca_Params_t edca_param_cfg;
    if(!pg_wifi_shell_cxt->wlan_enabled) {
        /* edca should be set after connectting */
        info_printf("wlan is not enabled \n");
        return QAPI_WLAN_ERR_DEVICE_NOT_FOUND;
    }
    if(Parameter_Count < 1 || !Parameter_List || !Parameter_List[0].Integer_Is_Valid)
    {
        info_printf("need a valid qtid: 0~7 or 255");
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }

    edca_param_cfg.qid = Parameter_List[0].Integer_Value;
    length = sizeof(edca_param_cfg);
    if(QAPI_OK != qapi_WLAN_Get_Param (deviceId,
                                    __QAPI_WLAN_PARAM_GROUP_WIRELESS,
				    __QAPI_WLAN_PARAM_GROUP_WIRELESS_EDCA_PARAM,
                                    &edca_param_cfg,
                                    &length)){
        info_printf("get edcca param fail for device %d\n",deviceId);
        return QAPI_ERROR;
    } else {
        info_printf("edca param -- qid:%d, aifs:%d cw_min:%d, cw_max:%d, txop_limit:%d\r\n",
            edca_param_cfg.qid, edca_param_cfg.aifsn, edca_param_cfg.cw_min, edca_param_cfg.cw_max, edca_param_cfg.txop_limit);
    }
    return QAPI_OK;
}

static qapi_Status_t setEdccaThreshold(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    uint8_t deviceId = get_active_device();
    uint8_t edcca_param_cfg;
    if(!pg_wifi_shell_cxt->wlan_enabled) {
        /* edca should be set after connectting */
        info_printf("wlan is not enabled \n");
        return QAPI_WLAN_ERR_DEVICE_NOT_FOUND;
    }
    if(Parameter_Count < 1 || !Parameter_List || !Parameter_List[0].Integer_Is_Valid)
    {
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }

    edcca_param_cfg = Parameter_List[0].Integer_Value;

    if (0 != qapi_WLAN_Set_Param (deviceId,
                                __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                                __QAPI_WLAN_PARAM_GROUP_WIRELESS_EDCCA_THRESHOLD,
                                &edcca_param_cfg,
                                sizeof(edcca_param_cfg),
                                FALSE))
    {
        info_printf("set edcca param fail, check the wlan connection or data validation\r\n");
        info_printf("default edcca thres is 38\r\n");
        return QAPI_ERROR;
    }
    info_printf("edcca threshold is set to %ddBm\n",edcca_param_cfg - 100);
    return QAPI_OK;
}

static qapi_Status_t getEdccaThreshold(uint32_t __attribute__((__unused__)) Parameter_Count, QAPI_Console_Parameter_t __attribute__((__unused__)) *Parameter_List)
{
    uint32_t length;
    uint8_t deviceId = get_active_device();
    uint8_t edcca_threshold;
    if(!pg_wifi_shell_cxt->wlan_enabled) {
        /* edca should be set after connectting */
        info_printf("wlan is not enabled \n");
        return QAPI_WLAN_ERR_DEVICE_NOT_FOUND;
    }

    length = sizeof(edcca_threshold);
    if(QAPI_OK != qapi_WLAN_Get_Param (deviceId,
                                    __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                                    __QAPI_WLAN_PARAM_GROUP_WIRELESS_EDCCA_THRESHOLD,
                                    &edcca_threshold,
                                    &length)){
        info_printf("get edcca threshold fail for device %d\n",deviceId);
        return QAPI_ERROR;
    } else {
        info_printf("edcca threshold:%d\r\n", edcca_threshold);
    }
    return QAPI_OK;
}

static qapi_Status_t setBmissThreshold(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    uint8_t deviceId = get_active_device();
    uint8_t bmiss;
    if(Parameter_Count != 1 || !Parameter_List || !Parameter_List[0].Integer_Is_Valid) {
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }

    if (Parameter_List[0].Integer_Value > UINT8_MAX || Parameter_List[0].Integer_Value < 0) {
        info_printf("beacon miss threshold need set 0-255\r\n");
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }

    bmiss = Parameter_List[0].Integer_Value;

    if (0 != qapi_WLAN_Set_Param (deviceId,
                                __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                                __QAPI_WLAN_PARAM_GROUP_WIRELESS_STA_BMISS_CONFIG,
                                &bmiss,
                                sizeof(bmiss),
                                FALSE))
    {
        info_printf("set bmiss threshold fail, check the wlan connection or data validation\r\n");
        return QAPI_ERROR;
    }
    info_printf("bmiss threshold is set to %d\n",bmiss);
    return QAPI_OK;
}

static qapi_Status_t getBmissThreshold(uint32_t __attribute__((__unused__)) Parameter_Count, QAPI_Console_Parameter_t __attribute__((__unused__)) *Parameter_List)
{
    uint32_t length;
    uint8_t deviceId = get_active_device();
    uint8_t bmiss_threshold;
    if(!pg_wifi_shell_cxt->wlan_enabled) {
        /* edca should be set after connectting */
        info_printf("wlan is not enabled \n");
        return QAPI_WLAN_ERR_DEVICE_NOT_FOUND;
    }

    length = sizeof(bmiss_threshold);
    if(QAPI_OK != qapi_WLAN_Get_Param (deviceId,
                                __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                                __QAPI_WLAN_PARAM_GROUP_WIRELESS_STA_BMISS_CONFIG,
                                &bmiss_threshold,
                                &length)){
        info_printf("get bmiss threshold fail for device %d\n",deviceId);
        return QAPI_ERROR;
    } else {
        info_printf("bmiss threshold:%d\r\n", bmiss_threshold);
    }
    return QAPI_OK;
}

const QAPI_Console_Command_t wifi_shell_cmds[] =
{
    // cmd_function    cmd_string               usage_string             description
    { Enable,          "Enable",                "",                      "Enables WLAN module"},
    { Disable,         "Disable",               "",                      "Disables WLAN module"},
    { Info,            "Info",                  "",                      "Info on WLAN state"},
    { SetDevice,       "SetDevice",             "<device = 0:AP|GO, 1:STA|P2P client",    "Set the active device"},
    { Scan,            "Scan",                  "<mode = 0: blocking| 1: non-blocking| 2:non-buffering> [ssid]",    "Scan for networks, using blocking/non-blocking/non-buffering modes. If ssid is provided, scan for specific ssid only."},
    { SetWpaPassphrase,"SetWpaPassphrase",      "<passphrase>",          "Set WPA passphrase"},
    { SetWpaParameters,"SetWpaParameters",      "<version=WPA|WPA2|WPACERT|WPA2CERT|SAE|SAE_WPA2> <ucipher> <mcipher>",    "Set WPA specific parameters"},
    { Connect,         "Connect",               "<ssid> [bssid]",        "Connect to a given ssid and given bssid(bssid option applicable to STA mode only. if AP mode connect command shouldnt take BSSID)"},
    { GetRssi,         "GetRssi",               "",                      "Get link quality indicator (SNR in dB) between AP and STA."},
    { Disconnect,      "Disconnect",            "",                      "Disconnect from AP or peer"},
    { SetChannel,      "SetChannel",            "<channel> [<is_6g_index = 0:no, 1:yes>]",      "Set a channel hint."},
    { SetPhyMode,      "SetPhyMode",            "<mode = a|b|g|ng|abgn>","Set the wireless mode"},
    { Set11nHTCap,     "Set11nHTCap",           "<HTCap = disable|ht20>","Set 11n HT parameter"},
    { SetOperatingMode,"SetOperatingMode",      "<ap|station> [<hidden|0> <wps|0>]",  "Set the operating mode to either Soft-AP or STA. Hidden and wps parameters only apply to AP mode."},
    { SetPowerMode,    "SetPowerMode",          "<mode = 0: Max performance, 1: Power Save>",    "Set the device power mode."},
    { SetAggregationParameters,"SetAggregationParameters",  "<tx_tid_mask> <rx_tid_mask>",    "Set aggregation on RX or TX or both. Enabled via TID bit mask (0x00-0xff)"}, 
    { SetAMSDU,        "SetAMSDU",              "<rx> <enable|disable>",    "Enable/Disable receive AMSDU"},
    { SetPromiscuous,  "SetPromiscuous",        "<enable|filter> [config|reset]",    "Enable/disable promoscuous mode and configure, reset filters."},
    { Enable80211v,    "Enable80211v",          "<1: enable| 0: disable>", "Enable/Disable 802.11v features"},
    { EnableSuspend,   "EnableSuspend",         "",                      "Enable WLAN Suspend. Should be done before connecting to a network."},
    { Suspend,         "Suspend",               "<time_in_ms>",          "Suspends the WLAN"},
    { SetCountryCode,  "SetCountryCode",        "<country_code_string>", "Set country code"},
    { GetCountryCode,  "GetCountryCode",        "",                      "Query country code from OTP"},
#ifdef CONFIG_DEBUG_CMD_XPA
    { EnableXpa,		"EnableXpa",        	"<1: enable| 0: disable> <1: 2G band| 0: 5G band>", "Enable/disable 2G or 5G xPA"},
#endif
    { SetRate,		"SetRate",       "rate : 0 ~ 27", 	"<sta_id/auto> <rate_1> <rate_2> <rate_3>"},
    { GetRate,		"GetRate",       "", 	"<sta_id>"},
	{ setAPBeaconInterval,		"SetAPBeaconInterval",          "<beacon_interval_in_ms>", "Set the beacon interval in ms."},
	{ setAPDtimPeriod,			"SetAPDtimPeriod",              "<dtim_period>",           "Set the DTIM period"},
	{ setAPInactivityPeriod,	"SetAPInactivityPeriod",        "<inactivity_period_in_mins>",  "Set inactivity period "},
	{ setCSAType,		"setCSAType",		"<0:csa | 1:ecsa>",	"set CSA type to CSA or ECSA"},
	{ channelSwitch,	"channelSwitch",	"<new channel num> <switch count> <switch mode> [is 6G]",	"channel switch in AP mode"},
	{ setSTAListenInterval,	"setSTAListenInterval",        "<listen_interval_in_TU> <0: ronud up|1: round down>",  "Set STA listen interval in TU which will round up/down to DTIM interval, 1TU=1024us"},
	{ getSTAListenInterval,	"getSTAListenInterval",        "",  "Get STA listen interval in TU"},
	{ sendRawFrame,	"sendRawFrame",        "",  "<rate_index> <num_tries = 1-14> <num_bytes = 0-1400> <channel: 1-11 or 36-> <type = 0:Beacon, 1:Probe Request, 2: QoS Data, 3: 4-addr data, ff:self-defined> [addr1 [addr2 [addr3 [addr4]]]]"},
#ifdef CONFIG_MGMT_FILTER_DEMO	
	{ setMgmtFilter,	"setMgmtFilter",        "0:None, 1:Asso Resp, 2:Probe Resp, 3:Asso and Probe Resp, -1:print mgmt frames",  "Set management frames filter"},
#endif	
	{ setApplicationIe, "setApplicationIe", "<0:beacon/1:probe request/2:probe response/3:asssociation request> <IE starting with dd>",  "Set application specified IE in specified management frame. Every input character is a nibble which means every 2 character is a byte, two characters are converted into a hex number before putting it in the frame. The length of application specified IE should be multiple of 2. if user has single digit value he need to prepend with 0 for ex: 0x5 should be 0x05. To remove IE, input only 'dd'"},
	{ setAntiInfParam,	"setAntiInfParam",        "0: 1M RTS, 1: 6M RTS, 2: 12M RTS",  "Set default anti-interference parameters to improve 2g throughput in noisy environment"},
	{ getAntiInfParam,	"getAntiInfParam",        "",  "Get default anti-interference parameters"},
    { setEdcaParam, "setEdcaParam",        "<qtid:0~7 or 255> <aifsn> <cwmin:exp> <cwmax:exp> <txop_limit>",  "set edca params for qtids, 255:all tids"},
    { getEdcaParam, "getEdcaParam",        "<qtid:0~7 or 255>",  "Get Edca parameters for qtid, 255:tid0"},
    { setEdccaThreshold, "setEdccaThreshold", "<EDCCA value, euqals real value plus 100>", "set EDCCA threshold to filter the non-wifi signal"},
    { getEdccaThreshold, "getEdccaThreshold", "", "get the EDCCA threshold"},
    { SetTxPower,           "SetTxPower",     "<txPower> [<policy = 0:SAFETY>]",   "Set the transmit power in dbm. The default policy is SAFETY(SAFETY is the minimum value among reg domain, CTL and target power). Set value to 100 to restore default settings. Tx power range, xpa: 10-SAFETY; ipa:3-SAFETY. Due to limited range in DAC gain with one designated Tx gain index, need to change PowerMode in BDF while setting power"   },
    { GetTxPower,           "GetTxPower",     "",                                  "Get the transmit power, reg_power, target power and CTL power"   },
    { setBmissThreshold, "setBmissThreshold", "<bmiss_threshold: 0~255>", "set beacon miss threshold"},
    { getBmissThreshold, "getBmissThreshold", "", "get beacon miss threshold"},

};

const QAPI_Console_Command_Group_t wifi_shell_cmd_group = {WLAN_SHELL_GROUP_NAME, sizeof(wifi_shell_cmds) / sizeof(QAPI_Console_Command_t), wifi_shell_cmds};

QAPI_Console_Group_Handle_t wifi_shell_cmd_group_handle;

void wifi_shell_init (void)
{
    pg_wifi_shell_cxt = &g_wifi_shell_cxt;
    wifi_shell_cxt_t *p_cxt = pg_wifi_shell_cxt;
    memset(&g_wifi_shell_cxt, 0, sizeof(wifi_shell_cxt_t));
    qurt_mutex_create(&p_cxt->wifi_shell_cxt_mutex);
    pg_wifi_shell_cxt->auth = QAPI_WLAN_AUTH_NONE_E;
    wifi_shell_cmd_group_handle = QAPI_Console_Register_Command_Group(NULL, &wifi_shell_cmd_group);
}


