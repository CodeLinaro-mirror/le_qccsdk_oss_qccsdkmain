/* 
Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include <stdio.h>
#include <ctype.h>
#include "wifi_cmn.h"
#include "qapi_wlan.h"
#include "qapi_console.h"
#include "qapi_wlan_p2p.h"

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

#ifdef CONFIG_WLAN_8021X
#include "wlan_8021x.h"
#include "qapi_wlan_8021x.h"
#include "qcli_api.h"
#include "fcntl.h"
#endif

#define WIFI_SHELL_INFO 1
#define WIFI_SHELL_LOG  0

// #define WIFI_SHELL_INFO 1
#define WIFI_SHELL_LOG  0

#define WLAN_SHELL_GROUP_NAME    "WLAN"
#define WLAN_SHELL_GROUP_PRINTF_SUFFIX  "WLAN: "

#define P2P_SHELL_GROUP_NAME     "P2P"
#define P2P_SHELL_GROUP_PRINTF_SUFFIX  "P2P: "

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

#if P2P_SHELL_LOG
#define log_printf(msg,...)     printf(P2P_SHELL_GROUP_PRINTF_SUFFIX LOG_PREFIX msg, ##__VA_ARGS__)
#else
#define log_printf(args...)     do { } while (0)
#endif

#define PRINT_ERR_NOT_SUPPORTED  info_printf("Not supported yet\n")
#define PRINT_ERR_CMD_FAILED     info_printf("Cmd failed\n")

#define SCAN_MODE_BLOCKING      1
#define SCAN_MODE_UNBLOCKING    2
#define DEV_NUM 2

#define MAX_WPS_PIN_SIZE        32

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
    uint8_t         active_device;
    uint8_t         wlan_enabled;
#ifdef CONFIG_WPS
    uint8_t         wps_stage;
#endif
#ifdef CONFIG_ENABLE_P2P_MODE
    uint8_t         p2p_cancel_enable;
    uint8_t         p2p_persistent_done;
    qbool_t         p2pMode;
    qbool_t         autogo_newpp;
    uint8_t         p2p_intent;
    uint8_t         p2p_session_in_progress; 
    uint8_t         p2p_join_session_active;
    uint8_t         p2p_persistent_go;   /* Used to remember persistent information while waiting for GO_NEG complete event */
    uint32_t        set_channel_p2p;
    uint8_t         wps_flag;
    uint8_t         invitation_index;
#endif
} wifi_shell_cxt_t;

typedef struct {
    uint8_t wps_in_progress;
    uint8_t connect_flag;
    uint8_t wps_pbc_interrupt;
    qapi_WLAN_Netparams_t netparams;
} wps_context_t;

static wifi_shell_cxt_t g_wifi_shell_cxt;
static wifi_shell_cxt_t *pg_wifi_shell_cxt;

#ifdef CONFIG_ENABLE_P2P_MODE

/* P2P Event Queue Nodes */
typedef struct _CMD_P2P_EVENT_INFO_{
    struct _CMD_P2P_EVENT_INFO_ *nextEvent; // link to next event in the queue
    uint8_t device_id;
    uint16_t event_id;
    uint32_t length;
    uint8_t pBuffer[1]; //variable size buffer. Allocation based on the Length
} CMD_P2P_EVENT_INFO;

/* P2P Event Queue */
typedef struct {
    qurt_mutex_t eventQueueMutex; //queue mutex
    CMD_P2P_EVENT_INFO *pEventHead; //queue head
    CMD_P2P_EVENT_INFO *pEventTail; // queue tail
} CMD_P2P_EVENT_LIST;

CMD_P2P_EVENT_LIST p2pEventNode;
char p2p_wps_pin[__QAPI_WLAN_WPS_PIN_LEN];
qapi_WLAN_P2P_Persistent_Mac_List_t p2p_peers_data[__QAPI_WLAN_P2P_MAX_LIST_COUNT];
uint8_t p2p_join_mac_addr[__QAPI_WLAN_MAC_LEN];
volatile uint8_t wifi_state[DEV_NUM] = {0,0};
qapi_WLAN_P2P_Connect_Cmd_t p2p_join_profile_cmd;
char wpa_passphrase[DEV_NUM][__QAPI_WLAN_PASSPHRASE_LEN + 1];

uint8_t inv_response_evt_index = 0;

#if CONFIG_ENABLE_P2P_MODE
#define   P2P_CONNECT_OPERATION     1
#define   P2P_PROVISION_OPERATION   2
#define   P2P_AUTH_OPERATION        3
#define   P2P_INVITE_OPERATION      4
#endif

uint8_t p2pScratchBuff[__QAPI_WLAN_P2P_EVT_BUF_SIZE];
uint8_t original_ssid[__QAPI_WLAN_MAX_SSID_LENGTH]; 
#endif
/***********************************************************************************************/
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
                        qbool_t has_wpa2 = false;
                        qbool_t has_wpa3 = false;
                        if((list[i].rsn_Auth & __QAPI_WLAN_SECURITY_AUTH_1X) ||
                            (list[i].rsn_Auth & __QAPI_WLAN_SECURITY_AUTH_PSK)) {
                                has_wpa2 = true;
                        }
                        if((list[i].rsn_Auth & __QAPI_WLAN_SECURITY_AUTH_SAE) || (list[i].rsn_Auth & __QAPI_WLAN_SECURITY_AUTH_WPA3_1X)) {
                            has_wpa3 = true;
                        }

                        if (has_wpa2 && has_wpa3) {
                            printf("RSN/WPA2/WPA3= ");
                        } else if (has_wpa2) {
                            info_printf("RSN/WPA2= ");
                        } else if (has_wpa3) {
                            info_printf("RSN/WPA3= ");
                        }
                    }
                    if(list[i].rsn_Auth){
                        printf(" {");
                        if(list[i].rsn_Auth & __QAPI_WLAN_SECURITY_AUTH_1X){
                             printf("WPA2-802.1X ");
                        }
                        if(list[i].rsn_Auth & __QAPI_WLAN_SECURITY_AUTH_PSK){
                            printf("PSK ");
                        }
                        if(list[i].rsn_Auth & __QAPI_WLAN_SECURITY_AUTH_SAE){
                            printf("SAE");
                        }
                        if(list[i].rsn_Auth & __QAPI_WLAN_SECURITY_AUTH_WPA3_1X){
                             printf("WPA3-802.1X ");
                        }
                        if(list[i].rsn_Auth & __QAPI_WLAN_SECURITY_AUTH_WPA3_1X_B_192){
                             printf("WPA3-802.1X (192-bit)");
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
                             printf("WPA-802.1X ");
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

uint32_t chan_to_frequency(uint32_t channel)
{
    if (channel < 1 || channel > 165)
    {
        return 0;
    }
    if (channel < 27) {
        channel = __QAPI_WLAN_CHAN_FREQ_1 + (channel-1)*5;
    } else {
        channel = (5000 + (channel*5));
    }
    return channel;

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
        wifi_fw_defaults_table_init();
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

#if CONFIG_ENABLE_P2P_MODE
    case QAPI_WLAN_P2P_CB_E:
       {
           qapi_wlan_p2p_event_cb(deviceId, payload, &payload_Length);
           break;
       }
#endif
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

#ifdef CONFIG_ENABLE_SUPPLICANT
    ret = wlan_supplicant_init();
    if (QAPI_OK != ret) {
        info_printf("wlan supplicant init fail\n");
    }
#endif
#ifdef CONFIG_WLAN_8021X
    ret = qapi_WLAN_8021x_Enable(QAPI_WLAN_8021X_ENABLE_E);
    if (QAPI_OK != ret) {
        info_printf("wlan 802.1x init fail\n");
    }
#endif
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
#ifdef CONFIG_WLAN_8021X
    qapi_WLAN_8021x_Enable(QAPI_WLAN_8021X_DISABLE_E);
#endif
#ifdef CONFIG_ENABLE_SUPPLICANT
    wlan_supplicant_exit();
#endif
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

uint32_t set_power_mode(qbool_t pwr_mode, uint8_t pwr_module)
{
    uint32_t deviceId = get_active_device();
    qapi_WLAN_Power_Mode_Params_t pwrMode;

    pwrMode.power_Mode = pwr_mode;
    pwrMode.power_Module = pwr_module;
    return qapi_WLAN_Set_Param(deviceId,
            __QAPI_WLAN_PARAM_GROUP_WIRELESS,
            __QAPI_WLAN_PARAM_GROUP_WIRELESS_POWER_MODE_PARAMS,
            (void *) &pwrMode,
            sizeof(pwrMode),
            FALSE);
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
int32_t get_op_mode(qapi_WLAN_DEV_Mode_e *mode)
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

    *mode = opmode;

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
    qapi_WLAN_DEV_Mode_e opmode;

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
	get_op_mode(&opmode);
    if (opmode == DEV_MODE_STATION_E && p_cxt->connected == true)
        info_printf("bssid       = %02x:%02x:%02x:%02x:%02x:%02x\n",p_cxt->bssid[0],p_cxt->bssid[1],p_cxt->bssid[2],p_cxt->bssid[3],p_cxt->bssid[4],p_cxt->bssid[5]);
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

	info_printf("DUT work in single device mode\r\n");
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

/* e.g.
 * SetWpaParameters WPA2 CCMP CCMP 
 * SetWpaParameters SAE CCMP CCMP 
 * SetWpaParameters SAE_WPA2
 * SetWpaParameters SAE_WPA2_WPA
 */
static qapi_Status_t SetWpaParameters(uint32_t __attribute__((__unused__)) Parameter_Count, QAPI_Console_Parameter_t __attribute__((__unused__)) *Parameter_List)
{
    if (Parameter_Count < 1 || !Parameter_List || Parameter_List[0].Integer_Is_Valid) {
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }

	uint8_t deviceId = get_active_device();
    qapi_WLAN_DEV_Mode_e opmode = 0;
    uint32_t dataLen = sizeof(opmode);
    char *wpaVer = Parameter_List[0].String_Value;
    qapi_WLAN_Auth_Mode_e e_wpa_ver;

#ifdef CONFIG_WLAN_8021X
    if (QAPI_OK != qapi_WLAN_Get_Param (deviceId,
                         __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                         __QAPI_WLAN_PARAM_GROUP_WIRELESS_OPERATION_MODE,
                         &opmode,
                         &dataLen))
    {
        info_printf("set_wpa failed.\r\n");
        return -1;
    }
#endif

    if(!strcmp(wpaVer,"WPA")) {
        e_wpa_ver = QAPI_WLAN_AUTH_WPA_PSK_E;
    } else if (!strcmp(wpaVer,"WPA2")) {
        e_wpa_ver = QAPI_WLAN_AUTH_WPA2_PSK_E;
	} else if (!strcmp(wpaVer, "SAE")) {
        e_wpa_ver = QAPI_WLAN_AUTH_WPA3_SAE_E;
	} else if (!strcmp(wpaVer,"SAE_WPA2")) {
        e_wpa_ver = QAPI_WLAN_AUTH_WPA2_SAE_MIXED_E;
    } else if (!strcmp(wpaVer,"WPA2_WPA")) {
        e_wpa_ver = QAPI_WLAN_AUTH_WPA_WPA2_MIXED_E;
    } else if(!strcmp(wpaVer,"SAE_WPA2_WPA")) {
        e_wpa_ver = QAPI_WLAN_AUTH_WPA_WPA2_SAE_MIXED_E;
#ifdef CONFIG_WLAN_8021X
    } else if ((opmode == DEV_MODE_STATION_E) && !(strcmp(wpaVer, "WPACERT"))) {
        e_wpa_ver = QAPI_WLAN_AUTH_WPA_E;
    } else if ((opmode == DEV_MODE_STATION_E) && (!strcmp(wpaVer, "WPA2CERT"))) {
        e_wpa_ver = QAPI_WLAN_AUTH_WPA2_E;
    } else if ((opmode == DEV_MODE_STATION_E) && (!strcmp(wpaVer, "WPA3CERT"))) {
        e_wpa_ver = QAPI_WLAN_AUTH_WPA3_EAP_ONLY_E;
    } else if ((opmode == DEV_MODE_STATION_E) && (!strcmp(wpaVer, "WPA2_WPA3CERT"))) {
        e_wpa_ver = QAPI_WLAN_AUTH_WPA3_EAP_TRANSITION_E;
#endif
    } else {
        info_printf("invalid wpa ver =%s\n", wpaVer);
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }

    if (((e_wpa_ver != QAPI_WLAN_AUTH_WPA2_SAE_MIXED_E) && (e_wpa_ver != QAPI_WLAN_AUTH_WPA_WPA2_SAE_MIXED_E) && 
        (e_wpa_ver != QAPI_WLAN_AUTH_WPA_WPA2_MIXED_E)) &&
        (Parameter_Count != 3 || Parameter_List[1].Integer_Is_Valid || Parameter_List[2].Integer_Is_Valid)) {
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }

    qapi_WLAN_Crypt_Type_e e_cipher;
    if ((e_wpa_ver == QAPI_WLAN_AUTH_WPA2_SAE_MIXED_E) || (e_wpa_ver == QAPI_WLAN_AUTH_WPA_WPA2_SAE_MIXED_E) || 
        (e_wpa_ver == QAPI_WLAN_AUTH_WPA_WPA2_MIXED_E)) {
        e_cipher = QAPI_WLAN_CRYPT_AUTO;
    }
    else {
        char *ucipher = Parameter_List[1].String_Value;
        char *mcipher = Parameter_List[2].String_Value;

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

#ifdef CONFIG_WLAN_8021X
/* e.g.
 * SetWpaCertParameters TTLS-MSCHAPV2 qguest user password
 */
static qapi_Status_t setWpaCertParams(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    if (Parameter_Count < 1 || !Parameter_List || Parameter_List[0].Integer_Is_Valid) {
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }
    uint32_t deviceId = get_active_device();
    uint8_t param_index = 0;
    uint8_t param_index_max = Parameter_Count - 1;
    char *method = NULL, *id = NULL, *username = NULL, *password = NULL;
    char *param_str = NULL;
    qapi_WLAN_8021X_Method_e q_method = QAPI_WLAN_8021X_METHOD_UNKNOWN;
    int integer_val = 0, debug_level = -1, priv = 0;
    char *ca_path = NULL, *cert_path = NULL, *private_key_path = NULL, *private_key_pwd = NULL;
	char* srv_auth = NULL;
    int file = -1;

    while (param_index <= param_index_max) {
        param_str = Parameter_List[param_index].String_Value;
        integer_val = Parameter_List[param_index].Integer_Value;
        switch (param_index) {
            case 0:
                method = param_str;
                break;
            case 1:
                id = param_str;
                break;
            case 2:
                username = param_str;
                break;
            case 3:
                password = param_str;
                break;
            case 4:
                debug_level = integer_val;
                break;
            case 5:
                ca_path = param_str;
                break;
            case 6:
                cert_path = param_str;
                break;
            case 7:
                private_key_path = param_str;
                break;
            case 8:
                private_key_pwd = param_str;
                break;
            case 9:
                priv = integer_val;
				break;
			case 10:
                srv_auth = param_str;   
                break;
            default:
                break;
        }
        param_index++;
    }

    if (debug_level >= 0 && debug_level <= 5) {
        qapi_WLAN_Set_Param (deviceId,
                             __QAPI_WLAN_PARAM_GROUP_WIRELESS_SECURITY,
                             __QAPI_WLAN_PARAM_GROUP_SECURITY_DEBUG_LEVEL,
                             (void *)&debug_level, sizeof(int), FALSE);
    }
    else {
        info_printf("invaid debug level, it should be an integer within [0, 5]\r\n");
        return QAPI_ERROR;
    }

    /********** wpa version ****************/
    if(!strcmp(method, "TLS")) {
        q_method = QAPI_WLAN_8021X_METHOD_EAP_TLS_E;
    } else if(!strcmp(method,"TTLS-MSCHAPV2")) {
        q_method = QAPI_WLAN_8021X_METHOD_EAP_TTLS_MSCHAPV2_E;
    } else if(!strcmp(method,"PEAP-MSCHAPV2")) {
        q_method = QAPI_WLAN_8021X_METHOD_EAP_PEAP_MSCHAPV2_E;
    } else if(!strcmp(method,"TTLS-MD5")) {
        q_method = QAPI_WLAN_8021X_METHOD_EAP_TTLS_MD5_E;
    } else {
        info_printf("invaid method\r\n");
        return QAPI_ERROR;
    }

    switch (q_method) {
    case QAPI_WLAN_8021X_METHOD_EAP_TLS_E:
        if (!ca_path || !cert_path) {
            info_printf("Need ca_path, cert_path\r\n");
            return QAPI_ERROR;
        }
        break;
    case QAPI_WLAN_8021X_METHOD_EAP_TTLS_MSCHAPV2_E:
    case QAPI_WLAN_8021X_METHOD_EAP_PEAP_MSCHAPV2_E:
    case QAPI_WLAN_8021X_METHOD_EAP_TTLS_MD5_E:
        if (!username || !password) {
            info_printf("Need username and password\r\n");
            return QAPI_ERROR;
        }
        break;
    default:
        info_printf("invaid method\r\n");
        return QAPI_ERROR;
    }

    if (0 != qapi_WLAN_Set_Param (deviceId,
                         __QAPI_WLAN_PARAM_GROUP_WIRELESS_SECURITY,
                         __QAPI_WLAN_PARAM_GROUP_SECURITY_8021X_METHOD,
                         (void *) &q_method, sizeof(q_method), FALSE)) {
        info_printf("set method failed.\r\n");
        return QAPI_ERROR;
    }

    if (0 != qapi_WLAN_Set_Param (deviceId,
                         __QAPI_WLAN_PARAM_GROUP_WIRELESS_SECURITY,
                         __QAPI_WLAN_PARAM_GROUP_SECURITY_8021X_IDENTITY,
                         (void *) id, strlen(id)+1, FALSE)) {
        info_printf("set id failed.\r\n");
        return QAPI_ERROR;
    }

    switch (q_method) {
    case QAPI_WLAN_8021X_METHOD_EAP_TLS_E: {
        qapi_WLAN_Security_8021x_Private_Key_t private_key;

        memset(&private_key, 0, sizeof(private_key));
        private_key.Private_Key_filename = private_key_path;
        private_key.Private_Key_Password = private_key_pwd;

        file = open(ca_path, O_RDONLY, 0);
        if (file != -1) {
            close(file);
            if (0 != qapi_WLAN_Set_Param (deviceId,
                             __QAPI_WLAN_PARAM_GROUP_WIRELESS_SECURITY,
                             __QAPI_WLAN_PARAM_GROUP_SECURITY_8021X_CA_CER,
                             (void *) ca_path, strlen(ca_path)+1, FALSE)) {
                info_printf("set ca_path failed.\r\n");
                return QAPI_ERROR;
            }
        } else {
            info_printf("Invalid ca path, no server auth\r\n");
        }

        file = open(cert_path, O_RDONLY, 0);
        if (file != -1) {
            close(file);
            if (0 != qapi_WLAN_Set_Param (deviceId,
                             __QAPI_WLAN_PARAM_GROUP_WIRELESS_SECURITY,
                             __QAPI_WLAN_PARAM_GROUP_SECURITY_8021X_CER,
                             (void *) cert_path, strlen(cert_path)+1, FALSE)) {
            info_printf("set cert_path failed.\r\n");
            return QAPI_ERROR;
            }
        } else {
            info_printf("Invalid cert path.\r\n");
        }

        file = open(private_key_path, O_RDONLY, 0);
        if (file != -1) {
            close(file);
            if (0 != qapi_WLAN_Set_Param (deviceId,
                             __QAPI_WLAN_PARAM_GROUP_WIRELESS_SECURITY,
                             __QAPI_WLAN_PARAM_GROUP_SECURITY_8021X_PRIVATE_KEY,
                             (void *) &private_key, sizeof(private_key), FALSE)) {
            info_printf("set private key failed.\r\n");
            return QAPI_ERROR;
            }
        } else {
            info_printf("Invalid private key path.\r\n");
        }

        break;
    }
    case QAPI_WLAN_8021X_METHOD_EAP_TTLS_MSCHAPV2_E:
    case QAPI_WLAN_8021X_METHOD_EAP_PEAP_MSCHAPV2_E:
    case QAPI_WLAN_8021X_METHOD_EAP_TTLS_MD5_E: {
        if (0 != qapi_WLAN_Set_Param (deviceId,
                             __QAPI_WLAN_PARAM_GROUP_WIRELESS_SECURITY,
                             __QAPI_WLAN_PARAM_GROUP_SECURITY_8021X_USERNAME,
                             (void *) username, strlen(username)+1, FALSE)) {
            info_printf("set username failed.\r\n");
            return QAPI_ERROR;
        }
        if (0 != qapi_WLAN_Set_Param (deviceId,
                             __QAPI_WLAN_PARAM_GROUP_WIRELESS_SECURITY,
                             __QAPI_WLAN_PARAM_GROUP_SECURITY_8021X_PASSWORD,
                             (void *) password, strlen(password)+1, FALSE)) {
            info_printf("set password failed.\r\n");
            return QAPI_ERROR;
        }
        file = open(ca_path, O_RDONLY, 0);
        if (file != -1) {
            close(file);
            if (0 != qapi_WLAN_Set_Param (deviceId,
                             __QAPI_WLAN_PARAM_GROUP_WIRELESS_SECURITY,
                             __QAPI_WLAN_PARAM_GROUP_SECURITY_8021X_CA_CER,
                             (void *) ca_path, strlen(ca_path)+1, FALSE)) {
                info_printf("set ca_path failed.\r\n");
                return QAPI_ERROR;
            }
        }
        break;
    }
    default:
        info_printf("invalid method\r\n");
        return QAPI_ERROR;
    }

    return QAPI_OK;

}
#endif

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

    if(deviceId == NT_DEV_STA_ID) {
        uint8_t bssidToConnect[__QAPI_WLAN_MAC_LEN] = {0};
        if (bssid && (ether_aton(bssid, bssidToConnect) < 0)) {
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
	qapi_WLAN_11n_HT_Config_t config;

	char *ht_config;
	if( Parameter_Count < 1 || Parameter_Count > 3 || !Parameter_List || Parameter_List[0].Integer_Is_Valid) {
		return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
	}

	ht_config = (char *)Parameter_List[0].String_Value;
	if(!strcmp(ht_config,"disable"))
		config.htconfig = QAPI_WLAN_11N_DISABLED_E;
	else if(!strcmp(ht_config,"ht20")) {
		config.htconfig = QAPI_WLAN_11N_HT20_E;
        if (Parameter_Count == 3) {
            config.sgi = Parameter_List[1].Integer_Value;
            config.mpdu_density = Parameter_List[2].Integer_Value;
        } else {
            config.sgi = 1;
            config.mpdu_density = 0;
        }
	} else {
		info_printf("Unknown ht config, only support disable/ht20\r\n");
		return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
	}
	ret = qapi_WLAN_Set_Param(deviceId,
               __QAPI_WLAN_PARAM_GROUP_WIRELESS,
               __QAPI_WLAN_PARAM_GROUP_WIRELESS_11N_HT,
               &config,
               sizeof(config),
               FALSE);
    if (ret) {
        info_printf("<HTCap = disable|ht20> <is_sgi = 0:no, 1:yes> <mpdu_density = 0:0us, 4:2us, 5:4us, 6:8us, 7:16us>\r\n");
    }
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
    qapi_Status_t ret = QAPI_OK;
    qapi_WLAN_Set_Rate_Params_t set_rate_cfg;

    memset(&set_rate_cfg, 0, sizeof(qapi_WLAN_Set_Rate_Params_t));

    if(!pg_wifi_shell_cxt->wlan_enabled) {
        info_printf("wlan is not enabled \n");
        return QAPI_WLAN_ERR_DEVICE_NOT_FOUND;
    }

    if (!Parameter_List) {
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }

    if (Parameter_Count < 1) {
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }

    if (memcmp(Parameter_List[0].String_Value, "auto", sizeof("auto")) == 0) {
        set_rate_cfg.ra_ON = 1;
    } else if(memcmp(Parameter_List[0].String_Value, "htOnly", sizeof("htOnly")) == 0) {
        if (Parameter_Count < 2) {
            return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
        }

        if(memcmp(Parameter_List[1].String_Value, "enable", sizeof("enable")) == 0) {
            set_rate_cfg.ra_ON = 2;
        } else if(memcmp(Parameter_List[1].String_Value, "disable", sizeof("disable")) == 0) {
            set_rate_cfg.ra_ON = 3;
        } else {
            return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
        }
    } else {
        if (Parameter_Count < 4) {
            return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
        }

        set_rate_cfg.ra_ON = 0;
        set_rate_cfg.rate_staid = Parameter_List[0].Integer_Value;
        set_rate_cfg.rate_p_rate = Parameter_List[1].Integer_Value;
        set_rate_cfg.rate_s_rate = Parameter_List[2].Integer_Value;
        set_rate_cfg.rate_t_rate = Parameter_List[3].Integer_Value;
    }

    ret = qapi_WLAN_Set_Rate(&set_rate_cfg);

    if(ret != QAPI_OK) {
        if(set_rate_cfg.ra_ON == 2 || set_rate_cfg.ra_ON == 3) {
            info_printf("SetRate htOnly fail \n");
        }
    }

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
 
#ifdef CONFIG_WPS
wps_context_t wps_context;
char wpsPin[MAX_WPS_PIN_SIZE];

int32_t wps_push_setup(uint32_t __attribute__((__unused__)) Parameter_Count, QAPI_Console_Parameter_t __attribute__((__unused__)) *Parameter_List)
{
    uint8_t val = 0, wps_mode = 0;
    qapi_WLAN_WPS_Credentials_t wpsScan, *wpsScan_p = NULL;
    int j = 0;
    qapi_WLAN_DEV_Mode_e wifi_mode;
    uint32_t data_len = sizeof(qapi_WLAN_DEV_Mode_e);
    uint32_t error = 0, deviceId = 0;
    char data[32+1] = {'\0'};

    deviceId = get_active_device();
    error = qapi_WLAN_Get_Param (deviceId,
                         __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                         __QAPI_WLAN_PARAM_GROUP_WIRELESS_OPERATION_MODE,
                         &wifi_mode,
                         &data_len);
    if (error != QAPI_OK)
    {
        info_printf("WPS failed\r\n");
        return QAPI_ERROR;
    }
    if (wifi_mode == DEV_MODE_AP_E)
    {
        error = qapi_WLAN_Get_Param (deviceId,
                             __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                             __QAPI_WLAN_PARAM_GROUP_WIRELESS_SSID,
                             (void *) data,
                             &data_len);
        if (data_len == 0)
        {
            return QAPI_ERROR;
        }
        if (error != 0)
        {
            return QAPI_ERROR;
        }
    }

    /* Initialize context */
    wps_context.wps_in_progress = 0;
    /* Connect flag */
    wps_context.connect_flag = Parameter_List[0].Integer_Value;
    /* Mode */
    wps_mode = QAPI_WLAN_WPS_PBC_MODE_E;
    /* Pin not used for WPS push mode */
    memset(wpsPin, 0, MAX_WPS_PIN_SIZE);

    if (Parameter_Count > 1)
    {
        /* SSID */
        if (strlen(Parameter_List[1].String_Value) > __QAPI_WLAN_MAX_SSID_LEN)
        {
                info_printf("Invalid SSID length\r\n");
                return QAPI_ERROR;
        }
        memset(wpsScan.ssid, 0, __QAPI_WLAN_MAX_SSID_LEN);
        wpsScan.ssid_Length = strlen(Parameter_List[1].String_Value);
        strlcpy((char*)(wpsScan.ssid), Parameter_List[1].String_Value, wpsScan.ssid_Length + 1);

        /* MAC address */
        if(strlen((char *) Parameter_List[2].String_Value) != 12)
        {
            info_printf("Invalid MAC address\r\n");
            return QAPI_ERROR;
        }
        memset(wpsScan.mac_Addr, 0, __QAPI_WLAN_MAC_LEN);
        for(j=0; j < strlen((char *) Parameter_List[2].String_Value); j++)
        {
            val = ascii_to_hex(Parameter_List[2].String_Value[j]);
            if(val == 0xff)
            {
                info_printf("Invalid character\r\n");
                return QAPI_ERROR;
            }
            else
            {
                if((j&1) == 0)
                {
                    val <<= 4;
                }
                wpsScan.mac_Addr[j>>1] |= val;
            }
        }

        /* Wireless channel */
        wpsScan.ap_Channel = Parameter_List[3].Integer_Value;
        wpsScan_p = &wpsScan;
    }

    if (0 != qapi_WLAN_Set_Param (deviceId,
                __QAPI_WLAN_PARAM_GROUP_WIRELESS_SECURITY,
                __QAPI_WLAN_PARAM_GROUP_SECURITY_WPS_CREDENTIALS,
                wpsScan_p,
                sizeof(qapi_WLAN_WPS_Credentials_t),
                FALSE))
    {
        info_printf("WPS failed\r\n");
        return QAPI_ERROR;
    }

    if(qapi_WLAN_Start_Wps(deviceId, wps_context.connect_flag, wps_mode, wpsPin, 0/* AUTH_OPEN */) != 0)
    {
        info_printf("WPS failed\r\n");
        return QAPI_ERROR;
    }

    wps_context.wps_in_progress = true;
    return QAPI_OK;
}

static qapi_Status_t wpsPushSetup(uint32_t __attribute__((__unused__)) Parameter_Count, QAPI_Console_Parameter_t __attribute__((__unused__)) *Parameter_List)
{
    if( Parameter_Count < 1 || !Parameter_List || !Parameter_List[0].Integer_Is_Valid) {
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }
    if((Parameter_List[0].Integer_Value != 0) && (Parameter_List[0].Integer_Value != 1)) {
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }
    if (0 == wps_push_setup(Parameter_Count, Parameter_List))
    {
        return QAPI_OK;
    }
    return QAPI_ERROR;
}
#endif
static qapi_Status_t setRspRate(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    uint8_t deviceId = get_active_device();
    uint8_t rate_idx;
    if(Parameter_Count != 1 || !Parameter_List || !Parameter_List[0].Integer_Is_Valid) {
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }

    if (Parameter_List[0].Integer_Value != 8 && Parameter_List[0].Integer_Value != 16) {
        info_printf("RspRate only support set to 8:6Mbps or 16:6.5Mbps\r\n");
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }

    rate_idx = Parameter_List[0].Integer_Value;

    if (0 != qapi_WLAN_Set_Param (deviceId,
                                __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                                __QAPI_WLAN_PARAM_GROUP_WIRELESS_RSP_RATE,
                                &rate_idx,
                                sizeof(rate_idx),
                                FALSE))
    {
        info_printf("set RspRate fail, check the wlan connection or data validation\r\n");
        return QAPI_ERROR;
    }
    info_printf("RspRate is set to 6Mbps\n");
    return QAPI_OK;
}

static qapi_Status_t setBaWinSize(uint32_t __attribute__((__unused__)) Parameter_Count, QAPI_Console_Parameter_t __attribute__((__unused__)) *Parameter_List)
{
	qapi_Status_t ret= QAPI_OK;
	uint8_t deviceId = get_active_device();
    qapi_WLAN_BA_Window_Size_t param;

	if( Parameter_Count != 2 || !Parameter_List 
        || !Parameter_List[0].Integer_Is_Valid || !Parameter_List[1].Integer_Is_Valid) {
		return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
	}
	
	param.tx_size = Parameter_List[0].Integer_Value;
	param.rx_size = Parameter_List[1].Integer_Value;
	
	if(param.tx_size > 64 || param.rx_size > 64) {
		info_printf("Tha MAX value of tx_ba_window_size and rx_ba_window_size is 64\r\n");
		return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
	}
	
	ret = qapi_WLAN_Set_Param(deviceId, 
               __QAPI_WLAN_PARAM_GROUP_WIRELESS,
               __QAPI_WLAN_PARAM_GROUP_WIRELESS_BA_WINDOW_SIZE,
               &param,
               sizeof(qapi_WLAN_BA_Window_Size_t),
               FALSE);

    if(ret != QAPI_OK) {
        info_printf("Set failed. WLAN should be enabled and please set the parameter before connecting.\r\n");
	}

	return ret;
}

static qapi_Status_t EnableCtsToSelf(uint32_t __attribute__((__unused__)) Parameter_Count, QAPI_Console_Parameter_t __attribute__((__unused__)) *Parameter_List)
{
	qapi_Status_t ret = QAPI_OK;
	uint8_t deviceId = get_active_device();
	uint32_t enable;

	if(!pg_wifi_shell_cxt->wlan_enabled) {
		info_printf("wlan is not enabled \n");
		return QAPI_WLAN_ERR_DEVICE_NOT_FOUND;
	}

	if(Parameter_Count != 1 || !Parameter_List || !Parameter_List[0].Integer_Is_Valid) {
		return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
	}

	enable = Parameter_List[0].Integer_Value;

	if(enable != 0 && enable != 1) {
		info_printf("Invalid parameter. Use 0 to disable or 1 to enable CTS-to-self\r\n");
		return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
	}

	ret = qapi_WLAN_Set_Param(deviceId,
				__QAPI_WLAN_PARAM_GROUP_WIRELESS,
				__QAPI_WLAN_PARAM_GROUP_WIRELESS_PROTECTION_MODE,
				&enable,
				sizeof(enable),
				FALSE);

	if(ret != QAPI_OK) {
		info_printf("Set CTS-to-self failed\r\n");
	} else {
		info_printf("CTS-to-self %s successfully\r\n", enable ? "enabled" : "disabled");
	}

	return ret;
}

#ifdef CONFIG_ENABLE_P2P_MODE
void app_p2p_process_persistent_list_event(uint8_t *pData)
{
    uint8_t *local_ptr = NULL;
    uint32_t loop_index = 0;

    if(!pData)
    {
        return;
    }
    local_ptr = pData;

    memset(p2p_peers_data, 0,
            (__QAPI_WLAN_P2P_MAX_LIST_COUNT * sizeof(qapi_WLAN_P2P_Persistent_Mac_List_t)));

    memscpy(p2p_peers_data, (__QAPI_WLAN_P2P_MAX_LIST_COUNT * sizeof(qapi_WLAN_P2P_Persistent_Mac_List_t)), local_ptr,
           (__QAPI_WLAN_P2P_MAX_LIST_COUNT * sizeof(qapi_WLAN_P2P_Persistent_Mac_List_t)));

    info_printf("\r\n");
    if(pg_wifi_shell_cxt->p2p_cancel_enable == 0)
    {
        for(loop_index = 0; loop_index < __QAPI_WLAN_P2P_MAX_LIST_COUNT; loop_index++)
        {
            info_printf("mac_addr[%d] : %02x:%02x:%02x:%02x:%02x:%02x\r\n", loop_index,
                    ((qapi_WLAN_P2P_Persistent_Mac_List_t *)local_ptr)->macaddr[0],
                    ((qapi_WLAN_P2P_Persistent_Mac_List_t *)local_ptr)->macaddr[1],
                    ((qapi_WLAN_P2P_Persistent_Mac_List_t *)local_ptr)->macaddr[3],
                    ((qapi_WLAN_P2P_Persistent_Mac_List_t *)local_ptr)->macaddr[4],
                    ((qapi_WLAN_P2P_Persistent_Mac_List_t *)local_ptr)->macaddr[5]);

            info_printf("ssid[%d] : %s\r\n", loop_index,
                    ((qapi_WLAN_P2P_Persistent_Mac_List_t *)local_ptr)->ssid);

            if(((qapi_WLAN_P2P_Persistent_Mac_List_t *)local_ptr)->role ==
                    QAPI_WLAN_P2P_INV_ROLE_ACTIVE_GO_E)
            {
                info_printf("passphrase[%d] : %s\r\n", loop_index,
                        ((qapi_WLAN_P2P_Persistent_Mac_List_t *)local_ptr)->passphrase);
            }
            local_ptr += sizeof(qapi_WLAN_P2P_Persistent_Mac_List_t);
        }
    }
    else
    {
        pg_wifi_shell_cxt->p2p_cancel_enable = 0;
    }
    return;
}

void app_p2p_process_node_list_event(uint8_t *pData)
{
    uint8_t *local_ptr = NULL, *temp_ptr = NULL;
    uint8_t index = 0, temp_val = 0;
    qapi_WLAN_P2P_Set_Cmd_t p2p_set_params;
    uint32_t deviceId = 0, temp_device_id = 0;

    if(!pData)
    {
        return;
    }

    local_ptr = pData;
    temp_val = *local_ptr;
    local_ptr++;

    temp_ptr = local_ptr;
    pg_wifi_shell_cxt->p2p_session_in_progress = 1;

    deviceId = get_active_device();
    if(deviceId != 0)
    {
        /* P2P device should always send/receive events/commands on dev 0 if
           the app has switched to dev 1 while event is in dev 0 send command
           via dev 0 and then switch to dev 1*/
        temp_device_id = deviceId;
        deviceId = 0;
        set_active_deviceid(deviceId);
    }
    if (temp_val > 0)
    {
        for (index = 0; index < temp_val; index++)
        {
            if(pg_wifi_shell_cxt->p2p_join_session_active)
            {
                if(memcmp(((qapi_WLAN_P2P_Device_Lite_t *)(local_ptr))->interface_Addr,
                            p2p_join_mac_addr, __QAPI_WLAN_MAC_LEN) == 0)
                {
                    p2p_join_profile_cmd.go_Oper_Freq = ((qapi_WLAN_P2P_Device_Lite_t*)(local_ptr))->oper_Freq;
                    break;
                }
            }
            else
            {
                info_printf("\r\n\t p2p_config_method     : %x \r\n",
                        (((qapi_WLAN_P2P_Device_Lite_t *)(local_ptr))->config_Methods));

                info_printf(" \t p2p_device_name       : %s \r\n",
                        ((qapi_WLAN_P2P_Device_Lite_t *)(local_ptr))->device_Name);

                info_printf("\t p2p_primary_dev_type  : %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\r\n ",
                        ((qapi_WLAN_P2P_Device_Lite_t *)(local_ptr))->pri_Dev_Type[0],
                        ((qapi_WLAN_P2P_Device_Lite_t *)(local_ptr))->pri_Dev_Type[1],
                        ((qapi_WLAN_P2P_Device_Lite_t *)(local_ptr))->pri_Dev_Type[2],
                        ((qapi_WLAN_P2P_Device_Lite_t *)(local_ptr))->pri_Dev_Type[3],
                        ((qapi_WLAN_P2P_Device_Lite_t *)(local_ptr))->pri_Dev_Type[4],
                        ((qapi_WLAN_P2P_Device_Lite_t *)(local_ptr))->pri_Dev_Type[5],
                        ((qapi_WLAN_P2P_Device_Lite_t *)(local_ptr))->pri_Dev_Type[6],
                        ((qapi_WLAN_P2P_Device_Lite_t *)(local_ptr))->pri_Dev_Type[7]);

                info_printf("\t p2p_interface_addr    : %02x:%02x:%02x:%02x:%02x:%02x \r\n",
                        ((qapi_WLAN_P2P_Device_Lite_t *)(local_ptr))->interface_Addr[0],
                        ((qapi_WLAN_P2P_Device_Lite_t *)(local_ptr))->interface_Addr[1],
                        ((qapi_WLAN_P2P_Device_Lite_t *)(local_ptr))->interface_Addr[2],
                        ((qapi_WLAN_P2P_Device_Lite_t *)(local_ptr))->interface_Addr[3],
                        ((qapi_WLAN_P2P_Device_Lite_t *)(local_ptr))->interface_Addr[4],
                        ((qapi_WLAN_P2P_Device_Lite_t *)(local_ptr))->interface_Addr[5]);

                info_printf("\t p2p_device_addr       : %02x:%02x:%02x:%02x:%02x:%02x \r\n",
                        ((qapi_WLAN_P2P_Device_Lite_t *)(local_ptr))->p2p_Device_Addr[0],
                        ((qapi_WLAN_P2P_Device_Lite_t *)(local_ptr))->p2p_Device_Addr[1],
                        ((qapi_WLAN_P2P_Device_Lite_t *)(local_ptr))->p2p_Device_Addr[2],
                        ((qapi_WLAN_P2P_Device_Lite_t *)(local_ptr))->p2p_Device_Addr[3],
                        ((qapi_WLAN_P2P_Device_Lite_t *)(local_ptr))->p2p_Device_Addr[4],
                        ((qapi_WLAN_P2P_Device_Lite_t *)(local_ptr))->p2p_Device_Addr[5]);

                info_printf("\t p2p_device_capability : %x \r\n",
                        ((qapi_WLAN_P2P_Device_Lite_t *)(local_ptr))->dev_Capab);

                info_printf("\t p2p_group_capability  : %x \r\n",
                        ((qapi_WLAN_P2P_Device_Lite_t *)(local_ptr))->group_Capab);

                info_printf("\t p2p_wps_method        : %x \r\n",
                        ((qapi_WLAN_P2P_Device_Lite_t *)(local_ptr))->wps_Method);

                info_printf("\t Peer Oper   channel   : %d \r\n",
                        ((qapi_WLAN_P2P_Device_Lite_t *)(local_ptr))->oper_Freq);
            }

            local_ptr += sizeof(qapi_WLAN_P2P_Device_Lite_t);
        }

        if(pg_wifi_shell_cxt->p2p_join_session_active)
        {
            pg_wifi_shell_cxt->p2p_join_session_active = 0;
            p2p_set_params.val.mode.p2pmode = __QAPI_WLAN_P2P_CLIENT;

            if (0 != qapi_WLAN_Set_Param(deviceId, __QAPI_WLAN_PARAM_GROUP_P2P,
                        __QAPI_WLAN_PARAM_GROUP_P2P_OP_MODE,
                        &p2p_set_params.val.mode.p2pmode,
                        sizeof(p2p_set_params.val.mode.p2pmode),
                        FALSE))
            {
                info_printf("\r\nStartP2P JOIN SET command did not execute properly\r\n");
                goto set_temp_device;
            }

            if(qapi_WLAN_P2P_Join(deviceId, p2p_join_profile_cmd.wps_Method,
                        &p2p_join_mac_addr[0], p2p_wps_pin,
                        p2p_join_profile_cmd.go_Oper_Freq) != 0)
            {
                info_printf("\r\nP2P JOIN command did not execute properly\r\n");
#if ENABLE_SCC_MODE
                info_printf("support single concurrent channel only....\r\n");
#endif /* ENABLE_SCC_MODE */
                goto set_temp_device;
            }
        }
    }
    local_ptr = temp_ptr;

set_temp_device:
    if(temp_device_id != 0)
    {
        set_active_deviceid(temp_device_id);
    }
    pg_wifi_shell_cxt->p2p_session_in_progress = 0;
    return;
}

void P2P_Event_Handler_Prov_Disc_Req(CMD_P2P_EVENT_INFO *pEventInfo)
{
    qapi_WLAN_P2P_Prov_Disc_Req_Event_t *local_ptr = NULL;
    uint16_t wps_method = 0;

    local_ptr = (qapi_WLAN_P2P_Prov_Disc_Req_Event_t *) pEventInfo->pBuffer;
    wps_method = local_ptr->wps_Config_Method;
    info_printf("\r\n source addr : %02x:%02x:%02x:%02x:%02x:%02x \r\n", local_ptr->sa[0], local_ptr->sa[1],
            local_ptr->sa[2], local_ptr->sa[3],
            local_ptr->sa[4], local_ptr->sa[5]);

    info_printf("\r\n wps_config_method : %x \r\n", local_ptr->wps_Config_Method);

    if(__QAPI_WLAN_P2P_WPS_CONFIG_DISPLAY == wps_method)
    {
        info_printf("Provisional Disc Request - Display WPS PIN [%s] \r\n",p2p_wps_pin);
    }
    else if(__QAPI_WLAN_P2P_WPS_CONFIG_KEYPAD == wps_method)
    {
        info_printf("Provisional Disc Request - Enter WPS PIN \r\n");
    }
    else if(__QAPI_WLAN_P2P_WPS_CONFIG_PUSHBUTTON == wps_method)
    {
        info_printf("Provisional Disc Request - Push Button \r\n");
    }
    else
    {
        info_printf("Invalid Provisional Request \r\n");
    }
    return;
}

void P2P_Event_Handler_Prov_Disc_Resp(CMD_P2P_EVENT_INFO *pEventInfo)
{
    qapi_WLAN_P2P_Prov_Disc_Resp_Event_t *local_ptr = NULL;

    local_ptr = (qapi_WLAN_P2P_Prov_Disc_Resp_Event_t *) pEventInfo->pBuffer;
    info_printf("\r\n peer addr : %02x:%02x:%02x:%02x:%02x:%02x \r\n",
            local_ptr->peer[0], local_ptr->peer[1], local_ptr->peer[2],
            local_ptr->peer[3], local_ptr->peer[4], local_ptr->peer[5]);

    if(__QAPI_WLAN_P2P_WPS_CONFIG_KEYPAD == local_ptr->config_Methods)
    {
        info_printf("Provisional Disc Response Keypad - WPS PIN [%s] \r\n",p2p_wps_pin);
    }
    else if(__QAPI_WLAN_P2P_WPS_CONFIG_DISPLAY == local_ptr->config_Methods)
    {
        info_printf("Provisional Disc Response Display \r\n");
    }
    else if(__QAPI_WLAN_P2P_WPS_CONFIG_PUSHBUTTON == local_ptr->config_Methods)
    {
        info_printf("Provisional Disc Response Push Button.\r\n");
    }
    else
    {
        info_printf("Invalid Provisional Response.\r\n");
    }
    return;
}

void P2P_Event_Handler_Req_To_Auth(CMD_P2P_EVENT_INFO *pEventInfo)
{
    qapi_WLAN_P2P_Req_To_Auth_Event_t *local_ptr = (qapi_WLAN_P2P_Req_To_Auth_Event_t *) pEventInfo->pBuffer;

    info_printf("\r\n source addr : %02x:%02x:%02x:%02x:%02x:%02x \r\n", local_ptr->sa[0], local_ptr->sa[1],
            local_ptr->sa[2], local_ptr->sa[3], local_ptr->sa[4], local_ptr->sa[5]);

    info_printf("\r\n dev_password_id : %x \r\n", local_ptr->dev_Password_Id);
    return;
}

void P2P_Event_Handler_Sdpd_Rx(CMD_P2P_EVENT_INFO *pEventInfo)
{
    qapi_WLAN_P2P_Sdpd_Rx_Event_t *local_ptr = (qapi_WLAN_P2P_Sdpd_Rx_Event_t *) pEventInfo->pBuffer;

    info_printf("Custom_Api_p2p_serv_disc_req event \r\n");
    info_printf("type : %d   frag id : %x \r\n", local_ptr->type, local_ptr->frag_Id);
    info_printf("transaction_status : %x \r\n", local_ptr->transaction_Status);
    info_printf("freq : %d status_code : %d comeback_delay : %d tlv_length : %d update_indic : %d \r\n",
            local_ptr->freq, local_ptr->status_Code, local_ptr->comeback_Delay, local_ptr->tlv_Length, local_ptr->update_Indic);

    info_printf("source addr : %02x:%02x:%02x:%02x:%02x:%02x \r\n",
            local_ptr->peer_Addr[0], local_ptr->peer_Addr[1],
            local_ptr->peer_Addr[2], local_ptr->peer_Addr[3],
            local_ptr->peer_Addr[4], local_ptr->peer_Addr[5]);

    return;
}

void P2P_Event_Handler_Invite_Sent_Result(CMD_P2P_EVENT_INFO *pEventInfo)
{
    qapi_WLAN_P2P_Invite_Sent_Result_Event_t *local_ptr = NULL;
    uint32_t deviceId = 0, temp_device_id = 0, dataLen = 0;
    int32_t channel = __QAPI_WLAN_P2P_AUTO_CHANNEL;

    local_ptr = (qapi_WLAN_P2P_Invite_Sent_Result_Event_t *) pEventInfo->pBuffer;
    info_printf("Invitation Result %d\r\n", local_ptr->status);

    if(local_ptr->status == 0)
    {
        info_printf("SSID %02x:%02x:%02x:%02x:%02x:%02x \r\n", local_ptr->bssid[0], local_ptr->bssid[1],
                local_ptr->bssid[2], local_ptr->bssid[3],
                local_ptr->bssid[4], local_ptr->bssid[5]);
    }

    if((p2p_peers_data[pg_wifi_shell_cxt->invitation_index].role == QAPI_WLAN_P2P_INV_ROLE_ACTIVE_GO_E) && (pg_wifi_shell_cxt->p2p_persistent_done == 0) && (local_ptr->status == 0))
    {
        pg_wifi_shell_cxt->p2p_session_in_progress = 1;
        deviceId = get_active_device();
        if(deviceId != 0)
        {
            /* P2P device should always send/receive events/commands on dev 0 if
             * the app has switched to dev 1 while event is in dev 0 send command
             * via dev 0 and then switch to dev 1 */
            temp_device_id = deviceId;
            /*
             * If the device 1 is connected, we start GO on the home channel of device 1.
             */
            if(temp_device_id ==1 && wifi_state[temp_device_id] ==1)
            {
                dataLen = 4;
                qapi_WLAN_Get_Param(temp_device_id,
                        __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                        __QAPI_WLAN_PARAM_GROUP_WIRELESS_CHANNEL,
                        &channel,
                        &dataLen);
            }
            deviceId = 0;
            set_active_deviceid(deviceId);
        }
        else
        {
            temp_device_id = deviceId;
            deviceId = 1;
            set_active_deviceid(deviceId);
            /*
             * If the device 1 is connected, we start GO on the home channel of device 1.
             */
            if(wifi_state[deviceId] ==1)
            {
                qapi_WLAN_Get_Param(deviceId,
                        __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                        __QAPI_WLAN_PARAM_GROUP_WIRELESS_CHANNEL,
                        &channel,
                        &dataLen);
            }
            deviceId = temp_device_id;
            set_active_deviceid(temp_device_id);
        }
        pg_wifi_shell_cxt->wps_flag = 0x01;

        info_printf("Starting Autonomous GO \r\n");
        if(qapi_WLAN_P2P_Start_Go(deviceId, NULL, channel, 1) != 0)
        {
            info_printf("\r\nStartP2P command did not execute properly\r\n");
            goto set_temp_device;
        }

        pg_wifi_shell_cxt->p2p_persistent_done = 1;

set_temp_device:
        if(temp_device_id != 0){
            set_active_deviceid(temp_device_id);
        }
        pg_wifi_shell_cxt->p2p_session_in_progress = 0;
    }
    return;
}

void P2P_Event_Handler_Invite_Rcvd_Result(CMD_P2P_EVENT_INFO *pEventInfo)
{
    qapi_WLAN_P2P_Invite_Rcvd_Result_Event_t *local_ptr = NULL;
    qapi_WLAN_P2P_Go_Params_t goParams;
    uint32_t deviceId = 0, temp_device_id = 0, dataLen = 0;
    int32_t channel = __QAPI_WLAN_P2P_AUTO_CHANNEL;
    int i = 0;

    deviceId = get_active_device();
    if(deviceId != 0)
    {
        /* P2P device should always send/receive events/commands on dev 0 if
         * the app has switched to dev 1 while event is in dev 0 send command
         * via dev 0 and then switch to dev 1 */
        temp_device_id = deviceId;
        /*
         * If the device 1 is connected, we start GO on the home channel of device 1.
         */
        if(temp_device_id ==1 && wifi_state[temp_device_id] ==1)
        {
            qapi_WLAN_Get_Param(temp_device_id,
                    __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                    __QAPI_WLAN_PARAM_GROUP_WIRELESS_CHANNEL,
                    &channel,
                    &dataLen);
        }
        deviceId = 0;
        set_active_deviceid(deviceId);
    }
    else
    {
        temp_device_id = deviceId;
        deviceId = 1;
        set_active_deviceid(deviceId);
        /*
         * If the device 1 is connected, we start GO on the home channel of device 1.
         */
        if(wifi_state[deviceId] ==1)
        {
            qapi_WLAN_Get_Param(deviceId,
                    __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                    __QAPI_WLAN_PARAM_GROUP_WIRELESS_CHANNEL,
                    &channel,
                    &dataLen);
        }
        deviceId = temp_device_id;
        set_active_deviceid(temp_device_id);
    }

    local_ptr = (qapi_WLAN_P2P_Invite_Rcvd_Result_Event_t *) pEventInfo->pBuffer;
    memset(&goParams,0, sizeof(qapi_WLAN_P2P_Go_Params_t));
    info_printf("Invite Result Status : %x \r\n", local_ptr->status);

    if (local_ptr->status == 0)
    {
        for (i=0;i<__QAPI_WLAN_MAC_LEN;i++)
        {
            info_printf(" [%x] ", local_ptr->sa[i]);
        }
    }
    else
    {
        qapi_WLAN_P2P_Stop_Find(deviceId);
    }
    info_printf("\r\n");

    if((p2p_peers_data[inv_response_evt_index].role == QAPI_WLAN_P2P_INV_ROLE_ACTIVE_GO_E) && (pg_wifi_shell_cxt->p2p_persistent_done == 0) &&
            (local_ptr->status == 0))
    {
        // make AP Mode and WPS default settings for P2P GO
        pg_wifi_shell_cxt->p2p_session_in_progress = 1;

        pg_wifi_shell_cxt->wps_flag = 0x01;
        info_printf("Starting Autonomous GO \r\n");

        goParams.ssid_Len = strlen((char *) p2p_peers_data[inv_response_evt_index].ssid);
        goParams.passphrase_Len = strlen((char *) p2p_peers_data[inv_response_evt_index].passphrase);
        memcpy(goParams.ssid, p2p_peers_data[inv_response_evt_index].ssid,
                goParams.ssid_Len);
        memcpy(goParams.passphrase, p2p_peers_data[inv_response_evt_index].passphrase,
                goParams.passphrase_Len);

        if(qapi_WLAN_P2P_Start_Go(deviceId, &goParams, channel, 1) != 0)
        {
            info_printf("\r\nStartP2P command did not execute properly\r\n");
            goto set_temp_device;
        }

        pg_wifi_shell_cxt->p2p_persistent_done = 1;
        inv_response_evt_index = 0;
    }

set_temp_device:
    if(temp_device_id != 0)
    {
        set_active_deviceid(temp_device_id);
    }
    pg_wifi_shell_cxt->p2p_session_in_progress = 0;
    return;
}

void P2P_Event_Handler_Invite_Req(CMD_P2P_EVENT_INFO *pEventInfo)
{
    qapi_WLAN_P2P_Invite_Req_Event_t *local_ptr = (qapi_WLAN_P2P_Invite_Req_Event_t *) pEventInfo->pBuffer;
    qapi_WLAN_P2P_Invite_Info_t invite_rsp_cmd;
    uint32_t deviceId = 0, temp_device_id = 0;
    int i = 0;

    info_printf("Invitation Req Received From : ");
    for (i = 0; i < __QAPI_WLAN_MAC_LEN; i++)
    {
        info_printf(" %x: ",local_ptr->sa[i]);
    }
    info_printf("\r\n");
    
    memset(&invite_rsp_cmd, 0, sizeof(qapi_WLAN_P2P_Invite_Info_t));

    if (local_ptr->is_Persistent)
    {
        for (i = 0; i < __QAPI_WLAN_P2P_MAX_LIST_COUNT; i++)
        {
            if(memcmp(local_ptr->sa, p2p_peers_data[i].macaddr, __QAPI_WLAN_MAC_LEN) == 0)
            {
                invite_rsp_cmd.status = 0;
                inv_response_evt_index = i;
                memscpy(invite_rsp_cmd.group_Bss_ID, __QAPI_WLAN_MAC_LEN, p2p_peers_data[i].macaddr, __QAPI_WLAN_MAC_LEN);
                break;
            }
        }

        if(i == __QAPI_WLAN_P2P_MAX_LIST_COUNT)
        {
            invite_rsp_cmd.status = 1;
            i = 0;
            memscpy(invite_rsp_cmd.group_Bss_ID, __QAPI_WLAN_MAC_LEN, local_ptr->sa, __QAPI_WLAN_MAC_LEN);
        }
    }
    else
    {
        invite_rsp_cmd.status = 0;
        memscpy(invite_rsp_cmd.group_Bss_ID, __QAPI_WLAN_MAC_LEN, local_ptr->sa, __QAPI_WLAN_MAC_LEN);
    }
    pg_wifi_shell_cxt->p2p_session_in_progress = 1;
    deviceId = get_active_device();
    if(deviceId != 0)
    {
        /* P2P device should always send/receive events/commands on dev 0 if
         * the app has switched to dev 1 while event is in dev 0 send command
         * via dev 0 and then switch to dev 1 */
        temp_device_id = deviceId;
        deviceId = 0;
        set_active_deviceid(deviceId);
    }

    /* send invite auth event */
    if(qapi_WLAN_P2P_Invite_Auth(deviceId, (qapi_WLAN_P2P_Invite_Info_t *)&invite_rsp_cmd) != 0)
    {
        info_printf("\r\nStartP2P (P2P invite auth persistent)command did not execute properly\r\n");
    }

    if(temp_device_id != 0)
    {
        set_active_deviceid(temp_device_id);
    }
    pg_wifi_shell_cxt->p2p_session_in_progress = 0;
    return;
}

void P2P_Event_Handler_Go_Neg_Result(CMD_P2P_EVENT_INFO *pEventInfo)
{
    qapi_WLAN_P2P_Go_Neg_Result_Event_t *p2pNeg = (qapi_WLAN_P2P_Go_Neg_Result_Event_t *) pEventInfo->pBuffer;
    qapi_WLAN_P2P_Go_Params_t goParams;
    uint32_t deviceId = 0, temp_device_id = 0, chnl = 0, signal_set = 0;
    int32_t error = 0, result = 0;
    uint8_t wps_mode = 0;

    if (0 == p2pNeg->freq) {
        return;
    }
    memset(p2p_wps_pin, 0, __QAPI_WLAN_WPS_PIN_LEN);
	strlcpy(p2p_wps_pin, "12345670", sizeof(p2p_wps_pin));

    info_printf("P2P GO Negotiation Result\r\n");
    info_printf("      Status: %s\r\n",(p2pNeg->status) ? "FAILURE":"SUCCESS");

    /* If group negotiation result was a failure then stop processing further. */
    if(p2pNeg->status != 0)
    {
        pg_wifi_shell_cxt->p2p_persistent_go = QAPI_WLAN_P2P_NON_PERSISTENT_E;
        qapi_WLAN_P2P_Stop_Find(deviceId);
        return;
    }

    info_printf("    P2P Role: %s\r\n",(p2pNeg->role_Go) ? "P2P GO": "P2P Client");
    info_printf("        SSID: %s\r\n", p2pNeg->ssid);
    info_printf("     Channel: %d\r\n", p2pNeg->freq);
    info_printf("  WPS Method: %s\r\n",
            (p2pNeg->wps_Method == QAPI_WLAN_P2P_WPS_PBC_E) ? "PBC": "PIN");

    pg_wifi_shell_cxt->p2p_session_in_progress = 1;
    deviceId = get_active_device();
    if(deviceId != 0)
    {

        /* P2P device should always send/receive events/commands on dev 0 if
         * the app has switched to dev 1 while event is in dev 0 send command
         * via dev 0 and then switch to dev 1 */

        temp_device_id = deviceId;
        deviceId = 0;
        set_active_deviceid(deviceId);
    }

    memset(&goParams, 0, sizeof(qapi_WLAN_P2P_Go_Params_t));
    if(p2pNeg->role_Go == 1)
    {
        pg_wifi_shell_cxt->wps_flag = 0x01;
        chnl = (p2pNeg->freq-2412)/5 + 1;

        goParams.ssid_Len = p2pNeg->ssid_Len;
        goParams.passphrase_Len = p2pNeg->passphrase_Len;
        memcpy(goParams.ssid, p2pNeg->ssid, goParams.ssid_Len);
        memcpy(goParams.passphrase, p2pNeg->pass_Phrase, goParams.passphrase_Len);

        /* Reset global p2p_persistent_go variable */
        pg_wifi_shell_cxt->p2p_persistent_go = QAPI_WLAN_P2P_NON_PERSISTENT_E;
        if(qapi_WLAN_P2P_Start_Go(0, &goParams, chnl, p2pNeg->persistent_Grp) != 0)
        {
            info_printf("\r\nP2P connect command did not execute properly\r\n");
            goto set_temp_device;
        }
        printf("p2p startt go end\r\n");
    }
    else if(p2pNeg->role_Go == 0)
    {
        uint8_t ssid[__QAPI_WLAN_MAX_SSID_LENGTH];
        qapi_WLAN_Dev_Mode_e opMode = DEV_MODE_STATION_E;
        qapi_WLAN_Set_Param(deviceId, __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                __QAPI_WLAN_PARAM_GROUP_WIRELESS_OPERATION_MODE,
                &opMode, sizeof(qapi_WLAN_Dev_Mode_e), FALSE);

        memset(ssid, 0, sizeof(ssid));
        memcpy(ssid, p2pNeg->ssid, sizeof(p2pNeg->ssid));
        error = qapi_WLAN_Set_Param(0,
                __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                __QAPI_WLAN_PARAM_GROUP_WIRELESS_SSID,
                (void *) ssid,
                sizeof(ssid),
                FALSE);
        if(error != 0)
        {
            info_printf("Unable to set SSID\r\n");
            goto set_temp_device;
        }
    }

    /* WPS */
    qapi_WLAN_WPS_Credentials_t wpsCreden;
    qapi_WLAN_WPS_Start_t wps_start;

    memset(&wpsCreden, 0, sizeof(qapi_WLAN_WPS_Credentials_t));
    memset(&wps_start, 0, sizeof(qapi_WLAN_WPS_Start_t));

    wps_context.connect_flag = (p2pNeg->role_Go) ? 0:8;

    if(p2pNeg->wps_Method != QAPI_WLAN_P2P_WPS_PBC_E)
    {
        info_printf("pin mode\r\n");
        wps_start.wps_Mode = QAPI_WLAN_WPS_PIN_MODE_E;
        wps_start.pin_Length = 9;

        //FIXME: This hardcoded pin value needs to be changed
        // for production to reflect what is on a sticker/label
        memcpy (wps_start.pin, p2p_wps_pin, wps_start.pin_Length);
        wps_start.pin[wps_start.pin_Length - 1] = '\0';
    }
    else
    {
        info_printf("pbc mode, deviceId = %d, freq = %d\r\n", deviceId, p2pNeg->freq);
        wps_start.wps_Mode = QAPI_WLAN_WPS_PBC_MODE_E;
    }

    if (p2pNeg->freq < 3000) {
        chnl = (p2pNeg->freq - 2412) / 5 + 1;
    }
    else {
        chnl = (p2pNeg->freq - 5000) / 5;
    }

    memcpy(wpsCreden.ssid, p2pNeg->ssid, sizeof(p2pNeg->ssid));
    memcpy(wpsCreden.mac_Addr, p2pNeg->peer_Interface_Addr, __QAPI_WLAN_MAC_LEN);
    wpsCreden.ap_Channel  = chnl;
    wpsCreden.ssid_Length = p2pNeg->ssid_Len;

    if (0 != qapi_WLAN_Set_Param(deviceId,
                __QAPI_WLAN_PARAM_GROUP_WIRELESS_SECURITY,
                __QAPI_WLAN_PARAM_GROUP_SECURITY_WPS_CREDENTIALS,
                &wpsCreden,
                sizeof(qapi_WLAN_WPS_Credentials_t),
                FALSE))
    {
        info_printf("set WPS failed\r\n");
        return;
    }
    else {
        info_printf("set WPS success\r\n");
    }

    /* Start WPS on the Aheros wifi */
    
    wps_mode = QAPI_WLAN_WPS_PBC_MODE_E;
    /* Initialize context */
    wps_context.wps_in_progress = 0;
    /* Connect flag */
    wps_context.connect_flag = 1;
    info_printf("WPS started.\r\n");

    if(qapi_WLAN_Start_Wps(deviceId, wps_context.connect_flag, wps_mode, wpsPin, 0/* AUTH_OPEN */) != 0)
    {
        info_printf("WPS failed\r\n");
        return;
    }

set_temp_device:
    if(temp_device_id != 0)
    {
        set_active_deviceid(temp_device_id);
    }
    pg_wifi_shell_cxt->p2p_session_in_progress = 0;
    wps_context.wps_in_progress = 1;
    return;
}

void P2P_Event_Handler(CMD_P2P_EVENT_INFO *pEventInfo)
{
    if (pEventInfo->event_id == __QAPI_WLAN_P2P_PROV_DISC_REQ_EVENTID)
    {
        P2P_Event_Handler_Prov_Disc_Req(pEventInfo);
    }
    else if (pEventInfo->event_id == __QAPI_WLAN_P2P_REQ_TO_AUTH_EVENTID)
    {
        P2P_Event_Handler_Req_To_Auth(pEventInfo);
    }
    else if (pEventInfo->event_id == __QAPI_WLAN_P2P_SDPD_RX_EVENTID)
    {
        P2P_Event_Handler_Sdpd_Rx(pEventInfo);
    }
    else if (pEventInfo->event_id == __QAPI_WLAN_P2P_PROV_DISC_RESP_EVENTID)
    {
        P2P_Event_Handler_Prov_Disc_Resp(pEventInfo);
    }
    else if (pEventInfo->event_id == __QAPI_WLAN_P2P_INVITE_SENT_RESULT_EVENTID)
    {
        P2P_Event_Handler_Invite_Sent_Result(pEventInfo);
    }
    else if (pEventInfo->event_id == __QAPI_WLAN_P2P_INVITE_RCVD_RESULT_EVENTID)
    {
        P2P_Event_Handler_Invite_Rcvd_Result(pEventInfo);
    }
    else if (pEventInfo->event_id == __QAPI_WLAN_P2P_INVITE_REQ_EVENTID)
    {
        P2P_Event_Handler_Invite_Req(pEventInfo);
    }
    else if (pEventInfo->event_id == __QAPI_WLAN_P2P_GO_NEG_RESULT_EVENTID)
    {
        P2P_Event_Handler_Go_Neg_Result(pEventInfo);
    }
    else
    {
        /* Should not come here */
        info_printf("Unknown P2P Event %d\n", pEventInfo->event_id);
    }
}

/* Add event info to the tail of the p2p event queue */
uint32_t qapi_p2p_queueP2PEventInfo(uint8_t device_id, uint16_t event_id,
        uint8_t *pBuffer, uint32_t Length)
{
    CMD_P2P_EVENT_INFO *pNewP2pEventInfo = NULL;

    /* Allocate memory for the new p2p event.
     * pNewP2pEventInfo->pBuffer[0] is pointer to the variable size pBuffer that is allocated based on the length
     */
    if((pNewP2pEventInfo = ((CMD_P2P_EVENT_INFO *)malloc(sizeof(CMD_P2P_EVENT_INFO) + Length))) == NULL){
        /* Failure to allocate memory will drop the event at caller */
        return -1;
    }

    pNewP2pEventInfo->nextEvent = NULL;
    pNewP2pEventInfo->device_id = device_id;
    pNewP2pEventInfo->event_id = event_id;
    pNewP2pEventInfo->length = Length;

    /* Copy the pBuffer to the variable length buffer pointer */
    memcpy(&(pNewP2pEventInfo->pBuffer[0]), pBuffer, pNewP2pEventInfo->length);

    /* aqucire mutex to update the p2p event queue */
    qurt_mutex_lock(&(p2pEventNode.eventQueueMutex));

    /* If empty, add to head
     * else add the event to the tail of the queue
     */
    if((NULL == p2pEventNode.pEventHead) && (NULL == p2pEventNode.pEventTail))
    {
        p2pEventNode.pEventHead = p2pEventNode.pEventTail = pNewP2pEventInfo;
    }
    else
    {
        p2pEventNode.pEventTail->nextEvent = pNewP2pEventInfo;
        p2pEventNode.pEventTail = pNewP2pEventInfo;
    }

    /* Release the mutex */
    qurt_mutex_unlock(&(p2pEventNode.eventQueueMutex));
    return 0;
}

/* Function to fetch event info at the head of the p2p event queue */
CMD_P2P_EVENT_INFO *app_p2p_getNextP2PEventInfo(void)
{
    CMD_P2P_EVENT_INFO *pTemp = NULL;

    /* Validate for empty queue */
    qurt_mutex_lock(&(p2pEventNode.eventQueueMutex));
    if(NULL != p2pEventNode.pEventHead)
    {
        /* aqucire mutex to update the p2p event queue */
        pTemp =  p2pEventNode.pEventHead;
        /* Update the queue head to the next or mark it empty */
        if(p2pEventNode.pEventHead == p2pEventNode.pEventTail)
        {
            p2pEventNode.pEventHead = p2pEventNode.pEventTail = NULL;
        }
        else
        {
            p2pEventNode.pEventHead = p2pEventNode.pEventHead->nextEvent;
        }
        /* Release the mutex */
    }
    qurt_mutex_unlock(&(p2pEventNode.eventQueueMutex));

    /* Return pointer to the event or NULL*/
    return(pTemp);
}


void app_handle_p2p_pending_events()
{
    CMD_P2P_EVENT_INFO *pTemp = NULL;

    /* Process all the pending P2P events */
    while ((pTemp = app_p2p_getNextP2PEventInfo()))
    {
        if (0 == pTemp->length)
        {
            free(pTemp);
            continue;
        }
        P2P_Event_Handler(pTemp);
        free(pTemp);
    }
    return;
}

void qapi_wlan_p2p_event_cb(uint8_t device_Id, void *pData, uint32_t *pLength)
{
    qapi_WLAN_P2P_Event_Cb_Info_t *pP2p_Event_Cb_Info = (qapi_WLAN_P2P_Event_Cb_Info_t *)pData;
    uint32_t status = 0;

    switch(pP2p_Event_Cb_Info->event_ID)
    {
        case __QAPI_WLAN_P2P_GO_NEG_RESULT_EVENTID:
        case __QAPI_WLAN_P2P_REQ_TO_AUTH_EVENTID:
        case __QAPI_WLAN_P2P_PROV_DISC_RESP_EVENTID:
        case __QAPI_WLAN_P2P_PROV_DISC_REQ_EVENTID:
        case __QAPI_WLAN_P2P_INVITE_REQ_EVENTID:
        case __QAPI_WLAN_P2P_INVITE_RCVD_RESULT_EVENTID:
        case __QAPI_WLAN_P2P_INVITE_SENT_RESULT_EVENTID:
        case __QAPI_WLAN_P2P_SDPD_RX_EVENTID:
            {
                /* This callback is executed in the proxy thread context.
                 * No blocking event handler as each event is run to completion.
                 * Copy the event into queue that is process at APP context. */
                if(-1 == (status = qapi_p2p_queueP2PEventInfo(device_Id,
                                (uint16_t)pP2p_Event_Cb_Info->event_ID,
                                (uint8_t *)&pP2p_Event_Cb_Info->WLAN_P2P_Event_Info.go_Neg_Result_Event,
                                *pLength)))
                {
                    info_printf(" Out of Memory: Dropping the P2P event\n");
                    return;
                }
                app_handle_p2p_pending_events();
                break;
            }

        default:
            info_printf("Unknown P2P event %d\n", pP2p_Event_Cb_Info->event_ID);
            break;
    }
    return;
}

void app_free_p2p_pending_events()
{
    CMD_P2P_EVENT_INFO *pTemp = NULL;

    /* The check for 'p2pMode' is added to make sure that the mutex is
     * initialized before trying to lock it */
    if (pg_wifi_shell_cxt->p2pMode)
    {
        /* Free all the pending P2P events */
        qurt_mutex_lock(&(p2pEventNode.eventQueueMutex));
        while(p2pEventNode.pEventHead)
        {
            pTemp = p2pEventNode.pEventHead;
            p2pEventNode.pEventHead = p2pEventNode.pEventHead->nextEvent;
            free(pTemp);
        }
        p2pEventNode.pEventHead = NULL;
        p2pEventNode.pEventTail = NULL;
        qurt_mutex_unlock(&p2pEventNode.eventQueueMutex);
        qurt_mutex_memheap_destroy(&p2pEventNode.eventQueueMutex);
    }
    return;
}

static qapi_Status_t P2p_enable(uint32_t __attribute__((__unused__)) Parameter_Count, QAPI_Console_Parameter_t __attribute__((__unused__)) *Parameter_List)
{
    uint32_t deviceId = get_active_device();
    wifi_shell_cxt_t *p_cxt = pg_wifi_shell_cxt;

    if(deviceId != 0)
    {
        deviceId = 0;
    }

    if(!p_cxt->wlan_enabled) {
        info_printf("wlan is not enabled \n");
        return QAPI_WLAN_ERR_DEVICE_NOT_FOUND;
    }

    /* Following call sets the wlan_callback_handler() as the callback for asynchronous events */

    if(qapi_WLAN_P2P_Enable(deviceId, true) != 0)
    {
        info_printf("P2P not enabled.\r\n");
        return -1;
    }

    p2pEventNode.pEventHead = NULL;
    p2pEventNode.pEventTail = NULL;
    qurt_mutex_memheap_init(&p2pEventNode.eventQueueMutex);

    pg_wifi_shell_cxt->p2pMode = TRUE;
    pg_wifi_shell_cxt->p2p_intent = 0;    /* Default group owner intent */
    pg_wifi_shell_cxt->autogo_newpp = FALSE;
    pg_wifi_shell_cxt->p2p_cancel_enable = 0;
    pg_wifi_shell_cxt->p2p_persistent_go = QAPI_WLAN_P2P_NON_PERSISTENT_E;

    memset(p2p_wps_pin, 0, __QAPI_WLAN_WPS_PIN_LEN);
    strlcpy(p2p_wps_pin, "12345670", sizeof(p2p_wps_pin));

    /* Autonomous GO configurations */
    wlan_set_passphrase("1234567890", 11);
    memset(original_ssid, 0, __QAPI_WLAN_MAX_SSID_LENGTH);
    strlcpy(original_ssid,"DIRECT-iO", sizeof(original_ssid));

    memset(p2p_join_mac_addr, 0, __QAPI_WLAN_MAC_LEN);
    memset(p2pScratchBuff, 0, sizeof(p2pScratchBuff));

    return QAPI_OK;
}


static qapi_Status_t P2p_disable(uint32_t __attribute__((__unused__)) Parameter_Count, QAPI_Console_Parameter_t __attribute__((__unused__)) *Parameter_List)
{
    qapi_WLAN_Dev_Mode_e opMode;
    wifi_shell_cxt_t *p_cxt = pg_wifi_shell_cxt;

    uint32_t deviceId = get_active_device();

    if(deviceId != 0)
    {
        deviceId = 0;
    }

    if(!p_cxt->wlan_enabled) {
        info_printf("wlan is not enabled \n");
        return QAPI_WLAN_ERR_DEVICE_NOT_FOUND;
    }

    if(!wlan_p2p_enable(deviceId, false))
    {
        info_printf("Disabling P2P mode failed.\r\n");
        return -1;
    }
    opMode = DEV_MODE_STATION_E;
    qapi_WLAN_Set_Param(deviceId, __QAPI_WLAN_PARAM_GROUP_WIRELESS,
            __QAPI_WLAN_PARAM_GROUP_WIRELESS_OPERATION_MODE,
            &opMode, sizeof(opMode), FALSE);
    /* Free the event queue and destroy the mutex before disabling P2P. */
    // app_free_p2p_pending_events();

    pg_wifi_shell_cxt->p2pMode = FALSE;
    pg_wifi_shell_cxt->connected = FALSE;
    pg_wifi_shell_cxt->set_channel_p2p = 0;
    return QAPI_OK;
}

static qapi_Status_t P2p_set_config(uint32_t __attribute__((__unused__)) Parameter_Count, QAPI_Console_Parameter_t __attribute__((__unused__)) *Parameter_List)
{
    qapi_WLAN_P2P_Config_Params_t p2pConfig;
    wifi_shell_cxt_t *p_cxt = pg_wifi_shell_cxt;

    if(!p_cxt->wlan_enabled) {
        info_printf("wlan is not enabled \n");
        return QAPI_WLAN_ERR_DEVICE_NOT_FOUND;
    }

    uint32_t deviceId = get_active_device();
    if((Parameter_Count < 5))
    {
        info_printf("Usage: \n");
        info_printf("wlan p2p setconfig <GO_intent> <listen channel> <operating channel> <country> <node_timeout> \n");
		return QAPI_ERROR;
    }

     if(deviceId != 0)
    {
        deviceId = 0;
    }

    if(!wlan_p2p_enable(deviceId, false))
    {
        info_printf("Disabling P2P mode failed.\r\n");
        return -1;
    }

    pg_wifi_shell_cxt->set_channel_p2p = Parameter_List[2].Integer_Value; // for autogo

    pg_wifi_shell_cxt->p2p_intent = Parameter_List[0].Integer_Value;
    p2pConfig.go_Intent      = pg_wifi_shell_cxt->p2p_intent;
    p2pConfig.listen_Chan    = (uint8_t)Parameter_List[1].Integer_Value;
    p2pConfig.op_Chan	     = (uint8_t)Parameter_List[2].Integer_Value;
    p2pConfig.age		     = (uint32_t)Parameter_List[4].Integer_Value;
    p2pConfig.reg_Class      = 81;
    p2pConfig.op_Reg_Class   = 115;
    p2pConfig.max_Node_Count = 5;

    if (0 != qapi_WLAN_Set_Param(deviceId, __QAPI_WLAN_PARAM_GROUP_P2P,
                __QAPI_WLAN_PARAM_GROUP_P2P_CONFIG_PARAMS,
                &p2pConfig, sizeof(p2pConfig), FALSE))
    {
        info_printf("P2P configuration failed.\r\n");
        return -1;
    }

    info_printf("Device configuration set successfully.\r\n");
    info_printf("Note: Cannot set country code.\r\n");
    info_printf("Use board data file or tuneables instead.\r\n");

    return QAPI_OK;
}

static qapi_Status_t P2p_find(uint32_t __attribute__((__unused__)) Parameter_Count, QAPI_Console_Parameter_t __attribute__((__unused__)) *Parameter_List)
{
    qapi_WLAN_P2P_Find_Cmd_t find_params;
    wifi_shell_cxt_t *p_cxt = pg_wifi_shell_cxt;

    if(!p_cxt->wlan_enabled) {
        info_printf("wlan is not enabled \n");
        return QAPI_WLAN_ERR_DEVICE_NOT_FOUND;
    }

    uint32_t deviceId = get_active_device();

    if (0 == pg_wifi_shell_cxt->p2pMode)
    {
        info_printf("Enable P2P before p2p find\r\n");
        return QAPI_ERROR;
    }

    if(deviceId != 0)
    {
        deviceId = 0;
    }

    memset(&find_params, 0, sizeof(qapi_WLAN_P2P_Find_Cmd_t));

    if(Parameter_Count == 1)
    {
        if(strcmp(Parameter_List[0].String_Value,"1") == 0)
        {
            find_params.type = (QAPI_WLAN_P2P_DISC_START_WITH_FULL_E);
            find_params.timeout = (300);
        }
        else if(strcmp(Parameter_List[0].String_Value,"2") == 0)
        {
            find_params.type = (QAPI_WLAN_P2P_DISC_ONLY_SOCIAL_E);
            find_params.timeout = (300);
        }
        else if(strcmp(Parameter_List[0].String_Value,"3") == 0)
        {
            find_params.type = (QAPI_WLAN_P2P_DISC_PROGRESSIVE_E);
            find_params.timeout = (300);
        }
        else
        {
            info_printf("Wrong option. Enter option 1,2 or 3\r\n");
            return -1;
        }
    }

    else if(Parameter_Count == 2)
    {
        if(strcmp(Parameter_List[0].String_Value,"1") == 0)
        {
            find_params.type = (QAPI_WLAN_P2P_DISC_START_WITH_FULL_E);
            find_params.timeout = (Parameter_List[1].Integer_Value);
        }
        else if(strcmp(Parameter_List[0].String_Value,"2") == 0)
        {
            find_params.type = (QAPI_WLAN_P2P_DISC_ONLY_SOCIAL_E);
            find_params.timeout = (Parameter_List[1].Integer_Value);
        }
        else if(strcmp(Parameter_List[0].String_Value,"3") == 0)
        {
            find_params.type = (QAPI_WLAN_P2P_DISC_PROGRESSIVE_E);
            find_params.timeout = (Parameter_List[1].Integer_Value);
        }
        else
        {
            info_printf("Wrong option. Enter option 1,2 or 3\r\n");
            return -1;
        }
    }

    else
    {
        find_params.type = (QAPI_WLAN_P2P_DISC_ONLY_SOCIAL_E);
        find_params.timeout = (300);
    }

    if(qapi_WLAN_P2P_Find(deviceId, find_params.type, find_params.timeout) != 0)
    {
        info_printf("P2P find command failed.\r\n");
        return -1;
    }

    return QAPI_OK;
}

uint8_t P2P_Check_Peer_Is_Found(const uint8_t *peer_addr, uint8_t p2p_operation)
{
    uint32_t deviceId = 0; 
    uint32_t dataLen = 0;
    uint8_t *local_ptr = NULL, index = 0, temp_val = 0, peer_found = TRUE;
    qapi_WLAN_P2P_Node_List_Params_t p2pNodeList;

    deviceId = get_active_device();

    if(deviceId != 0)
    {
        deviceId = 0;
    }

    memset(p2pScratchBuff, 0, sizeof(p2pScratchBuff));
    p2pNodeList.buffer_Length = __QAPI_WLAN_P2P_EVT_BUF_SIZE;
    p2pNodeList.node_List_Buffer = p2pScratchBuff;
    dataLen = sizeof(p2pNodeList);

    if (0 != qapi_WLAN_Get_Param(deviceId,
                __QAPI_WLAN_PARAM_GROUP_P2P,
                __QAPI_WLAN_PARAM_GROUP_P2P_NODE_LIST,
                &p2pNodeList, &dataLen))
    {
        info_printf("P2P node list command failed.\r\n");
        return FALSE;
    }

    if(!p2pNodeList.node_List_Buffer)
    {
        return FALSE;
    }
    local_ptr = p2pNodeList.node_List_Buffer;

    temp_val = *local_ptr;
    local_ptr++;
    if (temp_val > 0)
    {
        for (index = 0; index < temp_val; index++)
        {
            if(memcmp(((qapi_WLAN_P2P_Device_Lite_t *)(local_ptr))->p2p_Device_Addr,
                        peer_addr, __QAPI_WLAN_MAC_LEN) == 0)
            {
                peer_found = TRUE;
                break;
            }
            if(p2p_operation == P2P_PROVISION_OPERATION)
            {
                if(memcmp(((qapi_WLAN_P2P_Device_Lite_t *)(local_ptr))->interface_Addr,
                            peer_addr, __QAPI_WLAN_MAC_LEN) == 0)
                {
                    peer_found = TRUE;
                    break;
                }
            }
            local_ptr += sizeof(qapi_WLAN_P2P_Device_Lite_t);
        }
    }

    if(!peer_found)
    {
        info_printf("The Peer Device is not found, please do P2P Find again.\r\n");
    }

    return peer_found;
}

static qapi_Status_t P2p_connect(uint32_t __attribute__((__unused__)) Parameter_Count, QAPI_Console_Parameter_t __attribute__((__unused__)) *Parameter_List)
{
    uint32_t deviceId = 0; 
    uint32_t dataLen = __QAPI_WLAN_MAC_LEN;
    qapi_WLAN_P2P_Connect_Cmd_t p2p_connect;
    wifi_shell_cxt_t *p_cxt = pg_wifi_shell_cxt;

    if(!p_cxt->wlan_enabled) {
        info_printf("wlan is not enabled \n");
        return QAPI_WLAN_ERR_DEVICE_NOT_FOUND;
    }

    deviceId = get_active_device();

    if(deviceId != 0)
    {
        deviceId = 0;
    }

    if (0 == pg_wifi_shell_cxt->p2pMode)
    {
        info_printf("Enable P2P before p2p connect\r\n");
        return QAPI_ERROR;
    }

    memset(&p2p_connect, 0, sizeof(qapi_WLAN_P2P_Connect_Cmd_t));

    if (0 != qapi_WLAN_Get_Param(deviceId,
                __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                __QAPI_WLAN_PARAM_GROUP_WIRELESS_MAC_ADDRESS,
                p2p_connect.own_Interface_Addr, &dataLen))
    {
        info_printf("Unable to obtain device mac address\r\n");
        return -1;
    }

    info_printf("Own MAC addr : %02x:%02x:%02x:%02x:%02x:%02x \r\n",
            p2p_connect.own_Interface_Addr[0], p2p_connect.own_Interface_Addr[1],
            p2p_connect.own_Interface_Addr[2], p2p_connect.own_Interface_Addr[3],
            p2p_connect.own_Interface_Addr[4], p2p_connect.own_Interface_Addr[5]);

    if ((ether_aton((const char *)(Parameter_List[0].String_Value),p2p_connect.peer_Addr)) != 0)
    {
        info_printf("Invalid PEER MAC Address\r\n");
        return -1;
    }

    if(!P2P_Check_Peer_Is_Found(p2p_connect.peer_Addr, P2P_CONNECT_OPERATION))
    {
        return -1;
    }

    info_printf("Peer MAC addr : %02x:%02x:%02x:%02x:%02x:%02x \r\n",
            p2p_connect.peer_Addr[0], p2p_connect.peer_Addr[1],
            p2p_connect.peer_Addr[2], p2p_connect.peer_Addr[3],
            p2p_connect.peer_Addr[4], p2p_connect.peer_Addr[5]);

    if(strcmp(Parameter_List[1].String_Value,"push") == 0)
    {
        p2p_connect.wps_Method = QAPI_WLAN_P2P_WPS_PBC_E;

        /* Check if user has given "persistent" option */
        if((Parameter_Count == 3) &&
                strcmp(Parameter_List[2].String_Value,"persistent") == 0)
        {
            p2p_connect.dev_Capab |= __QAPI_WLAN_P2P_PERSISTENT_FLAG;
        }
    }
    else
    {
        /* Check if user has provided WPS pin (8 characters) */
        if(strlen((char *)Parameter_List[2].String_Value) == 8)
        {
            memset(p2p_wps_pin, 0, __QAPI_WLAN_WPS_PIN_LEN);
            strlcpy(p2p_wps_pin, (const char *)(Parameter_List[2].String_Value), sizeof(p2p_wps_pin));

            /* Check if user has given "persistent" option */
            if((Parameter_Count == 4) &&
                    strcmp(Parameter_List[3].String_Value,"persistent") == 0)
            {
                p2p_connect.dev_Capab |= __QAPI_WLAN_P2P_PERSISTENT_FLAG;
            }
        }

        if(strcmp(Parameter_List[1].String_Value,"display") == 0)
        {
            p2p_connect.wps_Method = QAPI_WLAN_P2P_WPS_PIN_DISPLAY_E;
            info_printf("WPS PIN %s \r\n",p2p_wps_pin);
        }
        else if(strcmp(Parameter_List[1].String_Value,"keypad") == 0)
        {
            p2p_connect.wps_Method = QAPI_WLAN_P2P_WPS_PIN_KEYPAD_E;
        }
    }

    /* Save P2P persistent flag as it will be needed while starting
       the group capability in beacons if role is P2P GO  */
    if (p2p_connect.dev_Capab & __QAPI_WLAN_P2P_PERSISTENT_FLAG)
    {
        pg_wifi_shell_cxt->p2p_persistent_go = QAPI_WLAN_P2P_PERSISTENT_E;
    }
    else
    {
        pg_wifi_shell_cxt->p2p_persistent_go = QAPI_WLAN_P2P_NON_PERSISTENT_E;
    }

    if(qapi_WLAN_P2P_Connect(deviceId, p2p_connect.wps_Method,
                p2p_connect.peer_Addr,
                pg_wifi_shell_cxt->p2p_persistent_go) != 0)
    {
        info_printf("P2P connect command failed.\r\n");
        pg_wifi_shell_cxt->p2p_persistent_go = QAPI_WLAN_P2P_NON_PERSISTENT_E;
        return -1;
    }

    return QAPI_OK;
}


static qapi_Status_t P2p_provision(uint32_t __attribute__((__unused__)) Parameter_Count, QAPI_Console_Parameter_t __attribute__((__unused__)) *Parameter_List)
{
    qapi_WLAN_P2P_Prov_Disc_Req_Cmd_t p2p_prov_disc;
    qapi_WLAN_P2P_Connect_Cmd_t p2p_connect;

    uint32_t deviceId = get_active_device();
    if(deviceId != 0)
    {
        deviceId = 0;
    }

    memset(&p2p_prov_disc, 0, sizeof(qapi_WLAN_P2P_Prov_Disc_Req_Cmd_t));
    if(strcmp(Parameter_List[1].String_Value, "push") == 0)
    {
        p2p_prov_disc.wps_Method = (uint16_t)__QAPI_WLAN_P2P_WPS_CONFIG_PUSHBUTTON;
    }
    else if(strcmp(Parameter_List[1].String_Value, "display") == 0)
    {
        p2p_prov_disc.wps_Method = (uint16_t)__QAPI_WLAN_P2P_WPS_CONFIG_DISPLAY;
    }
    else if(strcmp(Parameter_List[1].String_Value,"keypad") == 0)
    {
        p2p_prov_disc.wps_Method = (uint16_t)__QAPI_WLAN_P2P_WPS_CONFIG_KEYPAD;
    }
    else
    {
        info_printf("Incorrect WPS method\r\n");
        return -1;
    }

    p2p_prov_disc.dialog_Token = 1;

    if ((ether_aton((const char *)(Parameter_List[0].String_Value),p2p_prov_disc.peer)) != 0)
    {
        info_printf("Invalid PEER MAC Address\r\n");
        return -1;
    }

    if(!P2P_Check_Peer_Is_Found(p2p_prov_disc.peer, P2P_PROVISION_OPERATION))
    {
        return -1;
    }

    if( qapi_WLAN_P2P_Prov(deviceId, p2p_prov_disc.wps_Method, p2p_prov_disc.peer) != 0 )
    {
        info_printf("P2P provision command failed.\r\n");
        return -1;
    }

    /* Authorize P2P Device */
    memset(&p2p_connect, 0, sizeof(qapi_WLAN_P2P_Connect_Cmd_t));
    if(strcmp(Parameter_List[1].String_Value, "push") == 0)
    {
        p2p_connect.wps_Method = QAPI_WLAN_P2P_WPS_PBC_E;
    }
    else if(strcmp(Parameter_List[1].String_Value, "display") == 0)
    {
        p2p_connect.wps_Method = QAPI_WLAN_P2P_WPS_PIN_DISPLAY_E;
    }
    else if(strcmp(Parameter_List[1].String_Value,"keypad") == 0)
    {
        p2p_connect.wps_Method = QAPI_WLAN_P2P_WPS_PIN_KEYPAD_E;
    }

    if ((ether_aton((const char *)(Parameter_List[0].String_Value),p2p_connect.peer_Addr)) != 0)
    {
        info_printf("Invalid PEER MAC Address\r\n");
        return -1;
    }

    p2p_connect.go_Intent = pg_wifi_shell_cxt->p2p_intent;
    if(p2p_connect.wps_Method != QAPI_WLAN_P2P_WPS_NOT_READY_E)
    {
        if(qapi_WLAN_P2P_Auth(deviceId, p2p_connect.dev_Auth,
                    p2p_connect.wps_Method, p2p_connect.peer_Addr,
                    ((p2p_connect.dev_Capab & __QAPI_WLAN_P2P_PERSISTENT_FLAG) ?
                     QAPI_WLAN_P2P_PERSISTENT_E : QAPI_WLAN_P2P_NON_PERSISTENT_E)) != 0)
        {
            info_printf("P2P provision command failed.\r\n");
            return -1;
        }
    }
    return QAPI_OK;
}

static qapi_Status_t P2p_listen(uint32_t __attribute__((__unused__)) Parameter_Count, QAPI_Console_Parameter_t __attribute__((__unused__)) *Parameter_List)
{
    uint32_t timeout_val = 0;

    uint32_t deviceId = get_active_device();

    if(deviceId != 0)
    {
        deviceId = 0;
    }

    if (Parameter_Count == 1 && Parameter_List[0].Integer_Is_Valid)
    {
        timeout_val = Parameter_List[0].Integer_Value;
    }
    else
    {
        timeout_val = 300;
    }

    if(qapi_WLAN_P2P_Listen(deviceId, timeout_val) != 0)
    {
        info_printf("P2P listen command failed.\r\n");
        return -1;
    }
    return QAPI_OK;
}

static qapi_Status_t P2p_cancel(uint32_t __attribute__((__unused__)) Parameter_Count, QAPI_Console_Parameter_t __attribute__((__unused__)) *Parameter_List)
{
    uint32_t deviceId = 0, dataLen = 0;
    qapi_WLAN_P2P_Network_List_Params_t p2pNetworkList;
    qapi_WLAN_Dev_Mode_e opMode;

    deviceId = get_active_device();

    if(deviceId != 0)
    {
        deviceId = 0;
    }

    if(qapi_WLAN_P2P_Cancel(deviceId) != 0)
    {
        info_printf("P2P cancel command failed.\r\n");
        return -1;
    }

    pg_wifi_shell_cxt->autogo_newpp = FALSE;
    pg_wifi_shell_cxt->p2p_cancel_enable = 1;

    memset(p2pScratchBuff, 0, sizeof(p2pScratchBuff));
    p2pNetworkList.network_List_Buffer = p2pScratchBuff;
    p2pNetworkList.buffer_Length = __QAPI_WLAN_P2P_EVT_BUF_SIZE;

    if (0 != qapi_WLAN_Get_Param(deviceId,
                __QAPI_WLAN_PARAM_GROUP_P2P,
                __QAPI_WLAN_PARAM_GROUP_P2P_NETWORK_LIST,
                &p2pNetworkList, &dataLen))
    {
        info_printf("P2P cancel command did not execute properly\r\n");
        return -1;
    }

    /* Following call to app_p2p_process_persistent_list_event() is made to make
     * p2p_cancel() blocking so that no other asynchronous p2p operations happen before
     * p2p_cancel() completes. */
    app_p2p_process_persistent_list_event(p2pNetworkList.network_List_Buffer);

    /* Reset to mode station */
    opMode = DEV_MODE_STATION_E;
    qapi_WLAN_Set_Param(deviceId,
            __QAPI_WLAN_PARAM_GROUP_WIRELESS,
            __QAPI_WLAN_PARAM_GROUP_WIRELESS_OPERATION_MODE,
            &opMode,
            sizeof(qapi_WLAN_Dev_Mode_e),
            FALSE);

    pg_wifi_shell_cxt->p2p_persistent_done = 0;
    set_power_mode(QAPI_WLAN_POWER_MODE_REC_POWER_E, QAPI_WLAN_POWER_MODULE_P2P_E);

    return QAPI_OK;
}

static qapi_Status_t P2p_join(uint32_t __attribute__((__unused__)) Parameter_Count, QAPI_Console_Parameter_t __attribute__((__unused__)) *Parameter_List)
{
    uint32_t deviceId = 0, dataLen = 0;
    qapi_WLAN_P2P_Node_List_Params_t p2pNodeList;

    deviceId = get_active_device();
    if(deviceId != 0)
    {
        deviceId = 0;
    }

    if ((ether_aton((const char *)(Parameter_List[0].String_Value),p2p_join_mac_addr)) != 0)
    {
        info_printf("Invalid PEER MAC Address\r\n");
        return -1;
    }

    info_printf("Interface MAC addr : %02x:%02x:%02x:%02x:%02x:%02x \r\n",
            p2p_join_mac_addr[0], p2p_join_mac_addr[1], p2p_join_mac_addr[2],
            p2p_join_mac_addr[3], p2p_join_mac_addr[4], p2p_join_mac_addr[5]);

    /* Update join profile */
    memset(&p2p_join_profile_cmd, 0, sizeof( qapi_WLAN_P2P_Connect_Cmd_t));
    if (0 != qapi_WLAN_Get_Param(deviceId,
                __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                __QAPI_WLAN_PARAM_GROUP_WIRELESS_MAC_ADDRESS,
                p2p_join_profile_cmd.own_Interface_Addr,
                &dataLen))
    {
        info_printf("Unable to obtain device mac address\r\n");
        return -1;
    }

    info_printf("Own MAC addr : %02x:%02x:%02x:%02x:%02x:%02x \r\n",
            p2p_join_profile_cmd.own_Interface_Addr[0],
            p2p_join_profile_cmd.own_Interface_Addr[1],
            p2p_join_profile_cmd.own_Interface_Addr[2],
            p2p_join_profile_cmd.own_Interface_Addr[3],
            p2p_join_profile_cmd.own_Interface_Addr[4],
            p2p_join_profile_cmd.own_Interface_Addr[5]);

    if(strcmp(Parameter_List[1].String_Value,"push") == 0)
    {
        p2p_join_profile_cmd.wps_Method = QAPI_WLAN_P2P_WPS_PBC_E;
    }
    else if(strcmp(Parameter_List[1].String_Value,"display") == 0)
    {
        p2p_join_profile_cmd.wps_Method = QAPI_WLAN_P2P_WPS_PIN_DISPLAY_E;
    }
    else if(strcmp(Parameter_List[1].String_Value,"keypad") == 0)
    {
        p2p_join_profile_cmd.wps_Method = QAPI_WLAN_P2P_WPS_PIN_KEYPAD_E;
    }

    if(p2p_join_profile_cmd.wps_Method == QAPI_WLAN_P2P_WPS_PIN_DISPLAY_E ||
            p2p_join_profile_cmd.wps_Method == QAPI_WLAN_P2P_WPS_PIN_KEYPAD_E)
    {
        memset(p2p_wps_pin, 0, __QAPI_WLAN_WPS_PIN_LEN);
        strlcpy(p2p_wps_pin, (const char *)(Parameter_List[2].String_Value), sizeof(p2p_wps_pin));
    }

    memset(p2pScratchBuff, 0, sizeof(p2pScratchBuff));
    pg_wifi_shell_cxt->p2p_join_session_active = 1;
    p2pNodeList.buffer_Length = __QAPI_WLAN_P2P_EVT_BUF_SIZE;
    p2pNodeList.node_List_Buffer = p2pScratchBuff;

    if (0 != qapi_WLAN_Get_Param(deviceId,
                __QAPI_WLAN_PARAM_GROUP_P2P,
                __QAPI_WLAN_PARAM_GROUP_P2P_NODE_LIST,
                &p2pNodeList, &dataLen))
    {
        info_printf("P2P join command did not execute properly\r\n");
        return -1;
    }

    app_p2p_process_node_list_event(p2pNodeList.node_List_Buffer);
    return QAPI_OK;
}

static qapi_Status_t P2p_auth(uint32_t __attribute__((__unused__)) Parameter_Count, QAPI_Console_Parameter_t __attribute__((__unused__)) *Parameter_List)
{
    qapi_WLAN_P2P_Connect_Cmd_t p2p_connect;

    uint32_t deviceId = get_active_device();
    if(deviceId != 0)
    {
        deviceId = 0;
    }

    memset(&p2p_connect, 0, sizeof(qapi_WLAN_P2P_Connect_Cmd_t));

    if(strlen((char *)Parameter_List[2].String_Value) == 8)
    {
        memset(p2p_wps_pin, 0, __QAPI_WLAN_WPS_PIN_LEN);
        strlcpy(p2p_wps_pin, (const char *)(Parameter_List[2].String_Value), sizeof(p2p_wps_pin));
        info_printf("WPS Pin %s\r\n",p2p_wps_pin);

        /* Check if user has given "persistent" option */
        if((Parameter_Count == 4) &&
                strcmp(Parameter_List[3].String_Value,"persistent") == 0)
        {
            p2p_connect.dev_Capab |= __QAPI_WLAN_P2P_PERSISTENT_FLAG;
        }
    }
    if(strcmp(Parameter_List[1].String_Value, "deauth") == 0)
    {
        p2p_connect.dev_Auth = 1;
    }
    if(strcmp(Parameter_List[1].String_Value, "push") == 0)
    {
        p2p_connect.wps_Method = QAPI_WLAN_P2P_WPS_PBC_E;

        /* Check if user has given "persistent" option */
        if((Parameter_Count == 3) &&
                strcmp(Parameter_List[2].String_Value,"persistent") == 0)
        {
            p2p_connect.dev_Capab |= __QAPI_WLAN_P2P_PERSISTENT_FLAG;
        }
    }
    else if(strcmp(Parameter_List[1].String_Value, "display") == 0)
    {
        p2p_connect.wps_Method = QAPI_WLAN_P2P_WPS_PIN_DISPLAY_E;
        info_printf("WPS PIN %s \r\n",p2p_wps_pin);
    }
    else if(strcmp(Parameter_List[1].String_Value,"keypad") == 0)
    {
        p2p_connect.wps_Method = QAPI_WLAN_P2P_WPS_PIN_KEYPAD_E;
    }

    if ((ether_aton((const char *)(Parameter_List[0].String_Value),p2p_connect.peer_Addr)) != 0)
    {
        info_printf("Invalid PEER MAC Address\r\n");
        return -1;
    }

    if(!P2P_Check_Peer_Is_Found(p2p_connect.peer_Addr,P2P_AUTH_OPERATION))
    {
        return -1;
    }

    /* Save P2P persistent flag as it will be needed while starting
       the group capability in beacons if role is P2P GO  */
    if (p2p_connect.dev_Capab & __QAPI_WLAN_P2P_PERSISTENT_FLAG)
    {
        pg_wifi_shell_cxt->p2p_persistent_go = QAPI_WLAN_P2P_PERSISTENT_E;
    }
    else
    {
        pg_wifi_shell_cxt->p2p_persistent_go = QAPI_WLAN_P2P_NON_PERSISTENT_E;
    }

    if(qapi_WLAN_P2P_Auth(deviceId,
                p2p_connect.dev_Auth,
                (qapi_WLAN_P2P_WPS_Method_e) p2p_connect.wps_Method,
                p2p_connect.peer_Addr, pg_wifi_shell_cxt->p2p_persistent_go) != 0)
    {
        info_printf("StartP2P command did not execute properly\r\n");
        pg_wifi_shell_cxt->p2p_persistent_go = QAPI_WLAN_P2P_NON_PERSISTENT_E;
        return -1;
    }

    return QAPI_OK;
}

static qapi_Status_t P2p_auto_go(uint32_t __attribute__((__unused__)) Parameter_Count, QAPI_Console_Parameter_t __attribute__((__unused__)) *Parameter_List)
{
    uint8_t persistent_Group = 0;
    int32_t go_chan = __QAPI_WLAN_P2P_DEFAULT_CHAN;
    uint32_t channel = 0, dataLen = 0;
    uint32_t deviceId = get_active_device();

    if(deviceId != 0)
    {
        deviceId = 0;
    }

    /* if set_channel_p2p is valid, prefer to use set_channel_p2p */
    if (pg_wifi_shell_cxt->set_channel_p2p != 0) {
        go_chan = pg_wifi_shell_cxt->set_channel_p2p;
    }

    /*
     * If the device 1 is connected, we start GO on the home channel of device 1.
     */

    if(wifi_state[deviceId] ==1)
    {
        set_active_deviceid(1);
        dataLen = 4;
        qapi_WLAN_Get_Param(1,
                __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                __QAPI_WLAN_PARAM_GROUP_WIRELESS_CHANNEL,
                &channel,
                &dataLen);
        go_chan = channel;
        set_active_deviceid(0);
    }

    pg_wifi_shell_cxt->p2pMode = TRUE;
    pg_wifi_shell_cxt->wps_flag = 0x01;

    info_printf("Starting Autonomous GO.\r\n");

    /* Check if user has given "persistent" option */
    if(Parameter_Count && strcmp(Parameter_List[0].String_Value,"persistent") == 0)
    {
        persistent_Group = 1;
    }
    else
    {
        persistent_Group = 0;
    }

    if(FALSE == pg_wifi_shell_cxt->autogo_newpp)
    {
        qapi_WLAN_P2P_Go_Params_t goParams;
        memset(&goParams, 0, sizeof(qapi_WLAN_P2P_Go_Params_t));
        goParams.ssid_Len = strlen("DIRECT-iO");
        goParams.passphrase_Len = strlen(wpa_passphrase[deviceId]);

        memcpy(goParams.ssid, "DIRECT-iO", goParams.ssid_Len);
        memcpy(goParams.passphrase, wpa_passphrase[deviceId], goParams.passphrase_Len);

        if(qapi_WLAN_P2P_Start_Go(deviceId, &goParams, go_chan, persistent_Group) != 0)
        {
            info_printf("P2P auto GO command did not execute properly.\r\n");
            return -1;
        }
    }

    else
    {
        if(qapi_WLAN_P2P_Start_Go(deviceId, NULL, go_chan,
                    persistent_Group) != 0)
        {
            info_printf("P2P auto GO command did not execute properly.\r\n");
            return -1;
        }
    }
    return QAPI_OK;
}

static qapi_Status_t P2p_invite_auth(uint32_t __attribute__((__unused__)) Parameter_Count, QAPI_Console_Parameter_t __attribute__((__unused__)) *Parameter_List)
{
    if(Parameter_Count < 4)
	{
		return  QAPI_ERROR;
	}
    qapi_WLAN_P2P_Invite_Cmd_t p2pInvite;
    qapi_WLAN_P2P_Go_Params_t ssidParams;
    qapi_WLAN_Auth_Mode_e authMode = QAPI_WLAN_AUTH_WPA2_PSK_E;
    qapi_WLAN_Crypt_Type_e encrType = QAPI_WLAN_CRYPT_AES_CRYPT_E;
    uint32_t deviceId = 0, wifimode = 0, dataLen = 0, k = 0;
    uint8_t p2p_invite_role;

    deviceId = get_active_device();

    if(deviceId != 0)
    {
        deviceId = 0;
    }

    memset(&p2pInvite, 0, sizeof(qapi_WLAN_P2P_Invite_Cmd_t));
    memset(&ssidParams, 0, sizeof(qapi_WLAN_P2P_Go_Params_t));

    for(k = 0; k < __QAPI_WLAN_P2P_MAX_LIST_COUNT; k++)
    {
        if(strncmp((char *)p2p_peers_data[k].ssid,
                    (const char *)(Parameter_List[0].String_Value),
                    strlen((const char *)(Parameter_List[0].String_Value))) == 0)
        {
            break;
        }
    }
    if(k == __QAPI_WLAN_P2P_MAX_LIST_COUNT)
    {
        dataLen = 4;
        qapi_WLAN_Get_Param(deviceId,
                __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                __QAPI_WLAN_PARAM_GROUP_WIRELESS_OPERATION_MODE,
                &wifimode, &dataLen);
        // if((wifimode == DEV_MODE_STATION_E) && (get_dev_stat(deviceId) == UP))
        if((wifimode == DEV_MODE_STATION_E))
        {
            p2p_invite_role = QAPI_WLAN_P2P_INV_ROLE_CLIENT_E;
            p2pInvite.is_Persistent=0;
        }
        k = 0;
    }
    else
    {
        pg_wifi_shell_cxt->invitation_index = k;
        p2p_invite_role = p2p_peers_data[k].role;
        p2pInvite.is_Persistent=1;
    }

    if ((ether_aton((const char *)(Parameter_List[1].String_Value), p2pInvite.peer_Addr)) != 0)
    {
        info_printf("Invalid Invitation MAC Address\r\n");
        return -1;
    }

    if(!P2P_Check_Peer_Is_Found(p2pInvite.peer_Addr,P2P_INVITE_OPERATION))
    {
        return -1;
    }

    if(strcmp(Parameter_List[2].String_Value, "push") == 0)
    {
        p2pInvite.wps_Method = QAPI_WLAN_P2P_WPS_PBC_E;
    }
    else if(strcmp(Parameter_List[2].String_Value, "display") == 0)
    {
        p2pInvite.wps_Method = QAPI_WLAN_P2P_WPS_PIN_KEYPAD_E;
    }
    else if(strcmp(Parameter_List[2].String_Value,"keypad") == 0)
    {
        p2pInvite.wps_Method = QAPI_WLAN_P2P_WPS_PIN_DISPLAY_E;
    }
    else
    {
        info_printf("Incorrect wps method not proper P2P Invite \r\n");
        return -1;
    }

    if( qapi_WLAN_P2P_Invite(deviceId, (const char *)(Parameter_List[0].String_Value),
                p2pInvite.wps_Method, p2pInvite.peer_Addr,
                p2pInvite.is_Persistent, p2p_invite_role) != 0 )
    {
        info_printf("P2P command did not execute properly\r\n");
        return -1;
    }

    if(p2p_invite_role == QAPI_WLAN_P2P_INV_ROLE_ACTIVE_GO_E)
    {
        if(0 != qapi_WLAN_Set_Param(deviceId,
                    __QAPI_WLAN_PARAM_GROUP_WIRELESS_SECURITY,
                    __QAPI_WLAN_PARAM_GROUP_SECURITY_ENCRYPTION_TYPE,
                    (void *) &encrType, //QCOM_WLAN_CRYPT_AES_CRYPT
                    sizeof(qapi_WLAN_Crypt_Type_e), FALSE))
        {
            return -1;
        }

        if ( 0 != qapi_WLAN_Set_Param(deviceId,
                    __QAPI_WLAN_PARAM_GROUP_WIRELESS_SECURITY,
                    __QAPI_WLAN_PARAM_GROUP_SECURITY_AUTH_MODE,
                    (void *) &authMode, //QCOM_WLAN_AUTH_WPA2_PSK
                    sizeof(qapi_WLAN_Auth_Mode_e), FALSE))
        {
            return -1;
        }

        memcpy(ssidParams.passphrase, (char *)p2p_peers_data[k].passphrase,
                strlen((char *)p2p_peers_data[k].passphrase));
        memcpy(ssidParams.ssid, (char *)p2p_peers_data[k].ssid,
                strlen((char *)p2p_peers_data[k].ssid));
        ssidParams.ssid_Len = strlen((char *)p2p_peers_data[k].ssid);
        ssidParams.passphrase_Len = strlen((char *)p2p_peers_data[k].passphrase);

        if ( 0 != qapi_WLAN_Set_Param(deviceId,

                    __QAPI_WLAN_PARAM_GROUP_P2P,
                    __QAPI_WLAN_PARAM_GROUP_P2P_GO_PARAMS,
                    &ssidParams, sizeof(ssidParams), FALSE))
        {
            info_printf("P2P command did not execute properly\r\n");
            return -1;
        }
    }

    return QAPI_OK;
}

static qapi_Status_t P2p_get_nodelist(uint32_t __attribute__((__unused__)) Parameter_Count, QAPI_Console_Parameter_t __attribute__((__unused__)) *Parameter_List)
{
    uint32_t deviceId = 0; 
    uint32_t dataLen = sizeof(qapi_WLAN_P2P_Node_List_Params_t);
    qapi_WLAN_P2P_Node_List_Params_t p2pNodeList;

    deviceId = get_active_device();
    if(deviceId != 0)
    {
        deviceId = 0;
    }

    memset(p2pScratchBuff, 0, sizeof(p2pScratchBuff));
    p2pNodeList.buffer_Length = __QAPI_WLAN_P2P_EVT_BUF_SIZE;
    p2pNodeList.node_List_Buffer = p2pScratchBuff;

    if (0 != qapi_WLAN_Get_Param(deviceId,
                __QAPI_WLAN_PARAM_GROUP_P2P,
                __QAPI_WLAN_PARAM_GROUP_P2P_NODE_LIST,
                &p2pNodeList, &dataLen))
    {
        info_printf("P2P node list command failed.\r\n");
        return -1;
    }

    app_p2p_process_node_list_event(p2pNodeList.node_List_Buffer);
    return QAPI_OK;
}

static qapi_Status_t P2p_get_networklist(uint32_t __attribute__((__unused__)) Parameter_Count, QAPI_Console_Parameter_t __attribute__((__unused__)) *Parameter_List)
{
    uint32_t deviceId = 0, dataLen = 0;
    qapi_WLAN_P2P_Network_List_Params_t p2pNetworkList;

    deviceId = get_active_device();
    if(deviceId != 0)
    {
        deviceId = 0;
    }

    memset(p2pScratchBuff, 0, sizeof(p2pScratchBuff));
    p2pNetworkList.network_List_Buffer = p2pScratchBuff;
    p2pNetworkList.buffer_Length = __QAPI_WLAN_P2P_EVT_BUF_SIZE;

    if (0 != qapi_WLAN_Get_Param(deviceId,
                __QAPI_WLAN_PARAM_GROUP_P2P,
                __QAPI_WLAN_PARAM_GROUP_P2P_NETWORK_LIST,
                &p2pNetworkList, &dataLen))
    {
        info_printf("P2P command did not execute properly\r\n");
        return -1;
    }

    app_p2p_process_persistent_list_event(p2pNetworkList.network_List_Buffer);

    return QAPI_OK;
}

static qapi_Status_t P2p_set_noa_params(uint32_t __attribute__((__unused__)) Parameter_Count, QAPI_Console_Parameter_t __attribute__((__unused__)) *Parameter_List)
{
    qapi_WLAN_P2P_Noa_Params_t noaParams;

    uint32_t deviceId = get_active_device();
    if(deviceId != 0)
    {
        deviceId = 0;
    }

    memset(&noaParams, 0, sizeof(qapi_WLAN_P2P_Noa_Params_t));

    noaParams.noa_Desc_Params[0].type_Count = Parameter_List[0].Integer_Value;
    noaParams.noa_Desc_Params[0].start_Offset_Us = Parameter_List[1].Integer_Value;
    noaParams.noa_Desc_Params[0].duration_Us = Parameter_List[2].Integer_Value;
    noaParams.noa_Desc_Params[0].interval_Us = Parameter_List[3].Integer_Value;
    noaParams.enable = 1;
    noaParams.count = 1;

    if (0 != qapi_WLAN_Set_Param(deviceId,
                __QAPI_WLAN_PARAM_GROUP_P2P,
                __QAPI_WLAN_PARAM_GROUP_P2P_NOA_PARAMS,
                &noaParams,
                sizeof(noaParams),
                FALSE))
    {
        info_printf("P2P command did not execute properly\r\n");
        return -1;
    }

    return QAPI_OK;
}

static qapi_Status_t P2p_set_oops_params(uint32_t __attribute__((__unused__)) Parameter_Count, QAPI_Console_Parameter_t __attribute__((__unused__)) *Parameter_List)
{
    qapi_WLAN_P2P_Opps_Params_t opps;

    uint32_t deviceId = get_active_device();
    if(deviceId != 0)
    {
        deviceId = 0;
    }

    memset(&opps, 0, sizeof(qapi_WLAN_P2P_Opps_Params_t));
    opps.ct_Win	= Parameter_List[0].Integer_Value;
    opps.enable = Parameter_List[1].Integer_Value;
    if (0 != qapi_WLAN_Set_Param(deviceId,
                __QAPI_WLAN_PARAM_GROUP_P2P,
                __QAPI_WLAN_PARAM_GROUP_P2P_OPPS_PARAMS,
                &opps, sizeof(opps), FALSE))
    {
        info_printf("P2P command did not execute properly\r\n");
        return -1;
    }
    return QAPI_OK;
}

static qapi_Status_t P2p_set_operating_class(uint32_t __attribute__((__unused__)) Parameter_Count, QAPI_Console_Parameter_t __attribute__((__unused__)) *Parameter_List)
{
    qapi_WLAN_P2P_Config_Params_t p2pConfig;

    uint32_t deviceId = get_active_device();
    if(deviceId != 0)
    {
        deviceId = 0;
    }

    memset(&p2pConfig, 0, sizeof(qapi_WLAN_P2P_Config_Params_t));

    pg_wifi_shell_cxt->p2p_intent = Parameter_List[0].Integer_Value;
    pg_wifi_shell_cxt->set_channel_p2p = Parameter_List[2].Integer_Value;

    p2pConfig.go_Intent      = pg_wifi_shell_cxt->p2p_intent;
    p2pConfig.listen_Chan    = 6;
    p2pConfig.op_Chan	     = Parameter_List[2].Integer_Value;
    p2pConfig.age            = 3000;
    p2pConfig.reg_Class      = 81;
    p2pConfig.op_Reg_Class	 = Parameter_List[1].Integer_Value;
    p2pConfig.max_Node_Count = 4;

    if (0 != qapi_WLAN_Set_Param(deviceId,
                __QAPI_WLAN_PARAM_GROUP_P2P,
                __QAPI_WLAN_PARAM_GROUP_P2P_CONFIG_PARAMS,
                &p2pConfig, sizeof(p2pConfig), FALSE))
    {
        info_printf("P2P command did not execute properly\r\n");
        return -1;
    }
    return QAPI_OK;
}

static qapi_Status_t P2p_stop_find(uint32_t __attribute__((__unused__)) Parameter_Count, QAPI_Console_Parameter_t __attribute__((__unused__)) *Parameter_List)
{
    uint32_t deviceId = get_active_device();
    if(deviceId != 0)
    {
        deviceId = 0;
    }

    if(qapi_WLAN_P2P_Stop_Find(deviceId) != 0)
    {
        info_printf("P2P stop command did not execute properly\r\n");
        return -1;
    }
    return QAPI_OK;
}

static qapi_Status_t P2p_passphrase(uint32_t __attribute__((__unused__)) Parameter_Count, QAPI_Console_Parameter_t __attribute__((__unused__)) *Parameter_List)
{
    qapi_WLAN_Auth_Mode_e authMode = QAPI_WLAN_AUTH_WPA2_PSK_E;
    qapi_WLAN_Crypt_Type_e encrType = QAPI_WLAN_CRYPT_AES_CRYPT_E;
    qapi_WLAN_P2P_Go_Params_t ssidParams;

    uint32_t deviceId = get_active_device();
    if(deviceId != 0)
    {
        deviceId = 0;
    }

    memset(&ssidParams, 0, sizeof(qapi_WLAN_P2P_Go_Params_t));

    if(0 != qapi_WLAN_Set_Param(deviceId,
                __QAPI_WLAN_PARAM_GROUP_WIRELESS_SECURITY,
                __QAPI_WLAN_PARAM_GROUP_SECURITY_ENCRYPTION_TYPE,
                (void *) &encrType,
                sizeof(qapi_WLAN_Crypt_Type_e),
                FALSE))
    {
        return -1;
    }

    if (0 != qapi_WLAN_Set_Param(deviceId,
                __QAPI_WLAN_PARAM_GROUP_WIRELESS_SECURITY,
                __QAPI_WLAN_PARAM_GROUP_SECURITY_AUTH_MODE,
                &authMode,
                sizeof(qapi_WLAN_Auth_Mode_e),
                FALSE))

    {
        return -1;
    }

    pg_wifi_shell_cxt->autogo_newpp = TRUE;
    memcpy(ssidParams.passphrase, (char *)(Parameter_List[0].String_Value),
            strlen((char *)(Parameter_List[0].String_Value)));
    memcpy(ssidParams.ssid, (char *)(Parameter_List[1].String_Value),
            strlen((char *)(Parameter_List[1].String_Value)));

    ssidParams.ssid_Len = strlen((char *)(Parameter_List[1].String_Value));
    ssidParams.passphrase_Len = strlen((char *)(Parameter_List[0].String_Value));

    if ( 0 != qapi_WLAN_Set_Param(deviceId,
                __QAPI_WLAN_PARAM_GROUP_P2P,
                __QAPI_WLAN_PARAM_GROUP_P2P_GO_PARAMS,
                &ssidParams,	sizeof(ssidParams), FALSE))
    {
        info_printf("P2P command did not execute properly\r\n");
        return -1;
    }
    return QAPI_OK;
}

static qapi_Status_t P2p_set(uint32_t __attribute__((__unused__)) Parameter_Count, QAPI_Console_Parameter_t __attribute__((__unused__)) *Parameter_List)
{
    uint32_t deviceId = 0, len = 0;
    qapi_WLAN_P2P_Set_Cmd_t p2p_set_params;

    deviceId = get_active_device();
    if(deviceId != 0)
    {
        deviceId = 0;
    }

    memset(&p2p_set_params, 0, sizeof(qapi_WLAN_P2P_Set_Cmd_t));

    if(strcmp(Parameter_List[0].String_Value,"p2pmode") == 0)
    {
        if (Parameter_Count < 2)
        {
            info_printf("Incorrect parameters\r\n", deviceId);
            return -1;
        }

        p2p_set_params.config_Id = __QAPI_WLAN_PARAM_GROUP_P2P_OP_MODE;

        if(strcmp(Parameter_List[1].String_Value,"p2pdev") == 0)
        {
            p2p_set_params.val.mode.p2pmode = __QAPI_WLAN_P2P_DEV;
        }
        else if(strcmp(Parameter_List[1].String_Value,"p2pclient") == 0)
        {
            p2p_set_params.val.mode.p2pmode = __QAPI_WLAN_P2P_CLIENT;
        }
        else if(strcmp(Parameter_List[1].String_Value,"p2pgo") == 0)
        {
            p2p_set_params.val.mode.p2pmode = __QAPI_WLAN_P2P_GO;
        }
        else
        {
            info_printf("Input can be \"p2pdev/p2pclient/p2pgo\"");
            return -1;
        }

        len = sizeof(p2p_set_params.val.mode);
        info_printf("p2p mode :%x, Config Id %x\r\n",p2p_set_params.val.mode.p2pmode,p2p_set_params.config_Id);
    }

    else if(strcmp(Parameter_List[0].String_Value,"postfix") == 0)
    {
        p2p_set_params.config_Id = __QAPI_WLAN_PARAM_GROUP_P2P_SSID_POSTFIX;
        if(strlen((char *)Parameter_List[1].String_Value)) {
            memcpy(p2p_set_params.val.ssid_Postfix.ssid_Postfix,
                    (char *)Parameter_List[1].String_Value,
                    strlen((char *)Parameter_List[1].String_Value));

            p2p_set_params.val.ssid_Postfix.ssid_Postfix_Length = strlen((char *)Parameter_List[1].String_Value);
            len = sizeof(p2p_set_params.val.ssid_Postfix);
            info_printf("PostFix string %s, Len %d\r\n",
                    p2p_set_params.val.ssid_Postfix.ssid_Postfix,
                    p2p_set_params.val.ssid_Postfix.ssid_Postfix_Length);
        }
    }

    else if(strcmp(Parameter_List[0].String_Value, "intrabss") == 0)
    {
        p2p_set_params.config_Id = __QAPI_WLAN_PARAM_GROUP_P2P_INTRA_BSS;
        p2p_set_params.val.intra_Bss.flag = Parameter_List[1].Integer_Value;
        len = sizeof(p2p_set_params.val.intra_Bss);
    }

    else if(strcmp(Parameter_List[0].String_Value, "gointent") == 0)
    {
        p2p_set_params.config_Id = __QAPI_WLAN_PARAM_GROUP_P2P_GO_INTENT;
        p2p_set_params.val.go_Intent.value = Parameter_List[1].Integer_Value;
        len = sizeof(p2p_set_params.val.go_Intent);
    }

    else if (strcmp(Parameter_List[0].String_Value, "cckrates") == 0){
        p2p_set_params.config_Id = __QAPI_WLAN_PARAM_GROUP_P2P_CCK_RATES;
        p2p_set_params.val.cck_Rates.enable = Parameter_List[1].Integer_Value;
        len = sizeof(p2p_set_params.val.cck_Rates);
    }
    else if (strcmp(Parameter_List[0].String_Value, "listenchannel") == 0) {
        if ((Parameter_Count < 3) ||
                (Parameter_List[1].Integer_Value < 0) ||
                (Parameter_List[2].Integer_Value < 0)){
            info_printf("Incorrect parameters\r\n", deviceId);
            return -1;
        }

        p2p_set_params.config_Id = __QAPI_WLAN_PARAM_GROUP_P2P_LISTEN_CHANNEL;
        p2p_set_params.val.listen_Channel.reg_Class = Parameter_List[1].Integer_Value;
        p2p_set_params.val.listen_Channel.channel = Parameter_List[2].Integer_Value;
        len = sizeof(p2p_set_params.val.cck_Rates);
    }

    else if (strcmp(Parameter_List[0].String_Value, "devname") == 0) {
        if (Parameter_Count < 2) {
            info_printf("Incorrect parameters\r\n");
            return -1;
        }

        len = strlen((char *)Parameter_List[1].String_Value);
        if (len > __QAPI_WLAN_P2P_WPS_MAX_DEVNAME_LEN) {
            info_printf("Device name exceeds the allowed length\r\n");
            return -1;
        }

        p2p_set_params.config_Id = __QAPI_WLAN_PARAM_GROUP_P2P_DEV_NAME;
        memset(p2p_set_params.val.device_Name.dev_Name, 0, __QAPI_WLAN_P2P_WPS_MAX_DEVNAME_LEN + 1);
        memcpy(p2p_set_params.val.device_Name.dev_Name, (char *)Parameter_List[1].String_Value, len);
        p2p_set_params.val.device_Name.dev_Name_Len = len;
    }
    else if (strcmp(Parameter_List[0].String_Value, "discint") == 0) {
        if (Parameter_Count < 3) {
            info_printf("Incorrect parameters\r\n");
            return -1;
        }

        uint8_t min_interval = Parameter_List[1].Integer_Value;
        uint8_t max_interval = Parameter_List[2].Integer_Value;

        if (min_interval == 0 || max_interval == 0) {
            info_printf("The values for DiscoverableInterval shall be nonzero\r\n");
            return -1;
        }

        if (min_interval > max_interval) {
            info_printf("Minimum discoverable interval must be less than or equal to maximum\r\n");
            return -1;
        }

        if (max_interval > 255) {
            info_printf("Discoverable interval values must not exceed 255\r\n");
            return -1;
        }

        p2p_set_params.config_Id = __QAPI_WLAN_PARAM_GROUP_P2P_DISCOVERABLE_INTERVAL;
        p2p_set_params.val.discoverable_interval.min_discoverable_interval = min_interval;
        p2p_set_params.val.discoverable_interval.max_discoverable_interval = max_interval;
        len = sizeof(p2p_set_params.val.discoverable_interval);
    }

    else
    {
        info_printf("Incorrect parameters\r\n", deviceId);
        return -1;
    }

    if (0 != qapi_WLAN_Set_Param(deviceId, __QAPI_WLAN_PARAM_GROUP_P2P,
                p2p_set_params.config_Id,	&p2p_set_params.val,
                len, FALSE))
    {
        info_printf("P2P set command did not execute properly\r\n");
        return -1;
    }
    return QAPI_OK;
}

#endif /* CONFIG_ENABLE_P2P_MODE */

const QAPI_Console_Command_t wifi_shell_cmds[] =
{
    // cmd_function    cmd_string               usage_string             description
    { Enable,          "Enable",                "",                      "Enables WLAN module"},
    { Disable,         "Disable",               "",                      "Disables WLAN module"},
    { Info,            "Info",                  "",                      "Info on WLAN state"},
    { SetDevice,       "SetDevice",             "<device = 0:AP|GO, 1:STA|P2P client",    "Set the active device"},
    { Scan,            "Scan",                  "<mode = 1: blocking| 2: non-blocking> [ssid]",    "Scan for networks, using blocking/non-blocking modes. If ssid is provided, scan for specific ssid only."},
    { SetWpaPassphrase,"SetWpaPassphrase",      "<passphrase>",          "Set WPA passphrase"},
    { SetWpaParameters,"SetWpaParameters",      "<version=WPA|WPA2|WPACERT|WPA2CERT|WPA3CERT|SAE> <ucipher> <mcipher>\n    For mix mode, <version=WPA2_WPA|SAE_WPA2|SAE_WPA2_WPA>, <ucipher> and <mcipher> are not required",  "Set WPA specific parameters"},
#ifdef CONFIG_WLAN_8021X
    { setWpaCertParams,"SetWpaCertParameters",  "<method=TLS|TTLS-MSCHAPV2|PEAP-MSCHAPV2|TTLS-MD5> <id> <username> <password> <dbglevel> <ca_path> <cert_path> <key_path> <key_pwd>",    "Set WPA enterprise specific parameters"},
#endif
    { Connect,         "Connect",               "<ssid> [bssid]",        "Connect to a given ssid and given bssid(bssid option applicable to STA mode only. if AP mode connect command shouldnt take BSSID)"},
    { GetRssi,         "GetRssi",               "",                      "Get link quality indicator (SNR in dB) between AP and STA."},
    { Disconnect,      "Disconnect",            "",                      "Disconnect from AP or peer"},
    { SetChannel,      "SetChannel",            "<channel> [<is_6g_index = 0:no, 1:yes>]",      "Set a channel hint."},
    { SetPhyMode,      "SetPhyMode",            "<mode = a|b|g|ng|abgn>","Set the wireless mode"},
    { Set11nHTCap,     "Set11nHTCap",           "<HTCap = disable|ht20> <is_sgi = 0:no, 1:yes> <mpdu_density = 0:0us, 4:2us, 5:4us, 6:8us, 7:16us>","Set 11n HT parameter"},
    { SetOperatingMode,"SetOperatingMode",      "<ap|station> [<hidden|0> <wps|0>]",  "Set the operating mode to either Soft-AP or STA. Hidden and wps parameters only apply to AP mode."},
    { SetPowerMode,    "SetPowerMode",          "<mode = 0: Max performance, 1: Power Save>",    "Set the device power mode."},
    { SetAggregationParameters,"SetAggregationParameters",  "<tx_tid_mask> <rx_tid_mask>",    "Set aggregation on RX or TX or both. Enabled via TID bit mask (0x00-0xff)"}, 
    { SetAMSDU,        "SetAMSDU",              "<rx> <enable|disable>",    "Enable/Disable receive AMSDU"},
    { SetPromiscuous,  "SetPromiscuous",        "<enable|filter> [config|reset]",    "Enable/disable promoscuous mode and configure, reset filters."},
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
#ifdef CONFIG_WPS
    { wpsPushSetup, 	 "WpsPush", 		  "<connectFlag> [<ssid> <mac> <channel>]",    "Setup and start a WPS connection using the Push method"   },
#endif
    { setRspRate, 	 "setRspRate", 		  "<rate_idx = 8:11g 6Mbps 16:11n 6.5Mbps>",    "setRspRate to 11g 6Mbps or 11n 6.5Mbps"   },
    { setBaWinSize, 	 "setBaWinSize", 		  "<tx_ba_window_size> <rx_ba_window_size>",    "Set BA window size of RX or TX or both."   },
    {EnableCtsToSelf, "EnableCtsToSelf", 		  "<1: enable| 0: disable>",    "Enable/disable CTS-to-self."}
};

#ifdef CONFIG_ENABLE_P2P_MODE
const QAPI_Console_Command_t p2p_shell_cmds[] =
{
    // cmd_function           cmd_string         usage_string         description
    { P2p_enable,                "On",              "",                  "Enable P2P"   },
    { P2p_find,                  "Find",            "<channel_options = 1|2|3> <timeoutInSecs>",   "Initiates search for P2P peers. Channel_options = { 1: Scan all the channels from regulatory domain channel list,  2: Scan only the social channels (default), 3: Continue channel scan from the last scanned channel index}. Default value for timeoutInSecs = 60. When the timeout period expires, the find operation is stopped. "   },
    { P2p_get_nodelist,           "ListNodes",       "",                  "Display the results of P2P find operation."   },
    { P2p_connect,               "Connect",         "<peer_dev_mac> <wps_method = push|display|keypad> [WPS pin if keypad] [persistent]",      "Initiate connection request with a given peer MAC address using given WPS configuration method."   },

    { P2p_disable,               "Off",             "",                  "Disable P2P"   },
    { P2p_set_config,            "SetConfig",       "<GO_intent> <listen channel> <operating channel> <country> <node_timeout>", "Disable/Enable P2P"   },
    { P2p_provision,             "Provision",       "<peer_dev_mac> <wps_method = push|display|keypad>",   "Provision the WPS configuration method between the DUT and the peer."   },
    { P2p_listen,                "Listen",          "<timeout>",               "Initiate P2P listen process.When the timeout period expires, the listen operation is stopped. Default value is 300 seconds."   },
    { P2p_cancel,                "Cancel",          "",                  "Cancels ongoing P2P operation"   },
    { P2p_join,                  "Join",            "<GO_intf_mac> <wps_method = push|display|keypad> [WPS pin if keypad] [persistent]",   "Join a P2P client to an existing P2P Group Owner."   },
    { P2p_auth,                  "Auth",            "<peer_dev_mac> <wps_method = push|display|keypad|deauth> [WPS pin if keypad] [persistent]",   "Authenticate/Reject a connection request from a given peer MAC address using the given WPS configuration method."   },
    { P2p_auto_go,                "AutoGO",          "[persistent]",      "Start P2P device in Autonomous Group Owner mode."   },
    { P2p_invite_auth,            "Invite",          "<ssid> <peer_dev_mac> <wps_method= push|display|keypad> [persistent]",   "Invite a peer, from persistent database, to connect"   },
    { P2p_get_networklist,        "ListNetworks",    "",                  "Display the list of persistent P2P connections that are saved in the persistent media."   },
    { P2p_set_oops_params,	   	    "SetOPPSParams",	"<ctwin> <enable>",  "Set Opportunistics Power Save parameters."   },
    { P2p_set_noa_params,          "SetNOAParams",    "<count> <start_offset_in_usec> <duration_in_usec> <interval_in_usec> ",      "Set NOA parameters."   },
    { P2p_set_operating_class,     "SetOpClass",      "<GO_intent> <oper_reg_class> <oper_reg_channel>",      "Set Operating class parameters."   },
    { P2p_set,                   "Set",             "p2pmode <p2pdev|p2pclient|p2pgo> | postfix <postfix_string> | intrabss <flag> | gointent <Intent> | cckrates <1:Enable|0:Disable> >", "Set P2P parameters" },
    { P2p_stop_find,              "StopFind",        "",                  "Stop P2P operation" },
    { P2p_passphrase,            "SetPassphrase",   "<passphrase> <SSID>",  "Set P2P passphrase" },

};
#endif

const QAPI_Console_Command_Group_t wifi_shell_cmd_group = {WLAN_SHELL_GROUP_NAME, sizeof(wifi_shell_cmds) / sizeof(QAPI_Console_Command_t), wifi_shell_cmds};

QAPI_Console_Group_Handle_t wifi_shell_cmd_group_handle;

#ifdef CONFIG_ENABLE_P2P_MODE
const QAPI_Console_Command_Group_t p2p_shell_cmd_group = {P2P_SHELL_GROUP_NAME, sizeof(p2p_shell_cmds) / sizeof(QAPI_Console_Command_t), p2p_shell_cmds};

QAPI_Console_Group_Handle_t p2p_shell_cmd_group_handle;
#endif

void wifi_shell_init (void)
{
    pg_wifi_shell_cxt = &g_wifi_shell_cxt;
    wifi_shell_cxt_t *p_cxt = pg_wifi_shell_cxt;
    memset(&g_wifi_shell_cxt, 0, sizeof(wifi_shell_cxt_t));
    qurt_mutex_create(&p_cxt->wifi_shell_cxt_mutex);
    pg_wifi_shell_cxt->auth = QAPI_WLAN_AUTH_NONE_E;
    wifi_shell_cmd_group_handle = QAPI_Console_Register_Command_Group(NULL, &wifi_shell_cmd_group);
#ifdef CONFIG_WLAN_8021X
    wlan_dbg_init(wifi_shell_cmd_group_handle, QCLI_Printf);
#endif

#ifdef CONFIG_ENABLE_P2P_MODE
     p2p_shell_cmd_group_handle = QAPI_Console_Register_Command_Group(wifi_shell_cmd_group_handle, &p2p_shell_cmd_group);

    if (p2p_shell_cmd_group_handle)
    {
        info_printf("WLAN P2P Registered = %d\r\n", sizeof(p2p_shell_cmds)/ sizeof(QAPI_Console_Command_t));
    }
#endif
}
