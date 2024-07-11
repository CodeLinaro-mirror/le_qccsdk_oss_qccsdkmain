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
        break;
    }
    case QAPI_WLAN_DISCONNECT_CB_E: {
        qapi_WLAN_Join_Comp_Evt_t *cxnInfo = (qapi_WLAN_Join_Comp_Evt_t *)(payload);
		if(cxnInfo->bss_Connection_Status) {
            p_cxt->connected = false;
        }
        
        if(p_cxt->ssid_length) 
            info_printf("devId %d disconnected from ssid = %s\n", p_cxt->active_device, p_cxt->ssid);

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


