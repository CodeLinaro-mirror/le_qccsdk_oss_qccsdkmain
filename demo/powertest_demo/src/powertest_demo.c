/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <string.h>
#include <stdint.h>
#include "wifi_cmn.h"
#include "qapi_wlan.h"
#include "qapi_console.h"

#include "qurt_internal.h"
#include "qurt_mutex.h"

#include "qapi_net_status.h"
#include "lwip/pbuf.h"
#include "lwip/netif.h"
#include "lwip/tcpip.h"
#include "lwip/sockets.h"
#include "ip_addr.h"
#include "netifapi.h"
#include "data_path.h"
#if NT_FN_DHCPS_V4
#include "lwip/apps/nt_dhcps.h"
#include "dns.h"
#include "ip_addr.h"
#endif /* NT_FN_DHCPS_V4 */
#if NT_FN_DHCP6
#include "dhcp6.h"
#endif
#if LWIP_AUTOIP
#include "autoip.h"
#endif
#if LWIP_DNS
#include "dns.h"
#endif
#include "network_al.h"
#include "wifi_fw_pmu_ts_cfg.h"
#include "iperf.h"
#include "tcpip.h"
#include "nt_timer.h"
#include "wmi.h"
#include "wlan_drv.h"

#define DEFAULT_NETIF_IDX netif_get_index(netif_default) /* Default netif idx */

#ifdef FERMION_SILICON
extern uint32_t UART_Send_direct(char *txbuf, uint32_t buflen);
#define UART_SEND_DIRECT(str) UART_Send_direct((str), strlen(str))
#else
#define UART_SEND_DIRECT(str)
#endif

#define info_printf(msg, ...) printf("WLAN: " msg, ##__VA_ARGS__)

#ifndef DEV_STA_ID
#define DEV_STA_ID 1
#endif
#define min(a, b) (((a) < (b)) ? (a) : (b))

/**********************************************************************/

/**
Data structure used by the api layer to pass lowpower configurations to the driver.
*/
typedef union {
    WMI_IMPS_CFG imps_cfg;
    /**< IMPS cfg, used in qapi_imps_cfg. */
    struct {
        WMI_BMPS_IDLE_TIME bmps_idle_time;
        /**< The idle timeout in ms, used in qapi_bmps_cfg. */
        WMI_BMPS_ENABLE bmps_enable;
        /**< To enable/disable BMPS, used in qapi_bmps_cfg. */
        WMI_BMPS_LOG_ENABLE bmps_log_enable;
        /**< To enable/disable BMPS Log, used in qapi_bmps_log_enable. */
    } bmps_cfg;
    /**< BMPS cfg, used in qapi_bmps_cfg. */
    WMI_BMPS_IGNORE_BCMC bmps_ignore_bcmc;
    /**< To config ignore group-cast traffic during BMPS. */
    WMI_BMPS_TIMING_CFG bmps_timing;
    /**< Internal timing parameters in BMPS. */
    WMI_SLP_CLK_CAL_CFG slp_clk_cal;
    /**< Enable/disable 32k clock calibration in sleep mode. */
    WMI_SLP_CLK_CAL_ACT slp_clk_cal_act;
    /**< Enable/disable 32k clock calibration in active mode. */
    uint32_t force_dtim;
    /**< Force dtim period */
} lpr_wmi_t;

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
} wifi_shell_cxt_t;

/**
Data structure used by the api layer to store the connection info.
*/
typedef struct wifi_demo_cxt_s {
    char    ssid[__QAPI_WLAN_MAX_SSID_LEN+1];
    uint32_t ssid_len;
    uint8_t passphrase[WMI_PASSPHRASE_LEN+1];
    uint8_t passphrase_len;
    uint8_t dot11AuthMode;
    uint8_t authMode;
    uint8_t pairwiseCryptoType;
    uint8_t groupCryptoType;
} wifi_demo_cxt_t;

QAPI_Console_Group_Handle_t powertest_shell_cmd_group_handle;
TimerHandle_t iperf_timer;
TimerHandle_t roaming_timer;
TaskHandle_t net_send_task_handle;
THROUGHPUT_CXT *dtim_iperf_tCxt = NULL;
static wifi_shell_cxt_t *pg_wifi_shell_cxt;
static wifi_shell_cxt_t g_wifi_shell_cxt;
static wifi_demo_cxt_t pg_wifi_demo_cxt;
uint8_t g_wifi_ready = 0;
uint8_t log_enable = 1;
extern lpr_wmi_t g_lowpower_wmi;
extern wlan_qapi_cxt_t *gp_wlan_qapi_cxt;

extern qapi_Status_t qapi_pm_enable(uint8_t enable);
extern qapi_Status_t wmi_cmd_send(WMI_COMMAND_ID cmd_id, void *p_data, uint32_t data_len);
extern void qurt_thread_sleep(uint32 duration);

uint8_t get_demo_active_device()
{
	return DEV_STA_ID;
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
                DEV_STA_ID, cxnInfo->bss_Connection_Status, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            if (e_wpa_ver==QAPI_WLAN_AUTH_WPA_PSK_E || e_wpa_ver==QAPI_WLAN_AUTH_WPA2_PSK_E) {
                info_printf("4 way handshake success for device=1\n");
            }
            g_wifi_ready = 1;
            if(roaming_timer != NULL) {
                nt_stop_timer(roaming_timer);
            }	
        } else {
			info_printf("WiFi disconnect reason code is %d\n", cxnInfo->reason_code);
			if(cxnInfo->bss_Connection_Status) {
				p_cxt->connected = false;
                g_wifi_ready = 0;
                qapi_WLAN_Disconnect(get_demo_active_device());
				info_printf("devId %d Disconnected MAC addr %02x:%02x:%02x:%02x:%02x:%02x \n",
					DEV_STA_ID, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
                if(roaming_timer != NULL) {
                    nt_start_timer(roaming_timer);
                }
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

void pm_enable()
{
    info_printf("=====  sleep  =====\r\n");
    qapi_pm_enable(1);

    WMI_BMPS_IGNORE_BCMC *pdata = (WMI_BMPS_IGNORE_BCMC *)&g_lowpower_wmi;
    memset(pdata, 0, sizeof(*pdata));
    pdata->enable = 1;
    wmi_cmd_send(WMI_BMPS_IGNORE_BCMC_CMDID, pdata, sizeof(*pdata));

    WMI_BMPS_ENABLE *pdata2 = (WMI_BMPS_ENABLE *)&g_lowpower_wmi;
    memset(pdata2, 0, sizeof(*pdata2));
    pdata2->enable = 1;
    wmi_cmd_send(WMI_BMPS_ENABLE_CMDID, pdata2, sizeof(*pdata2));
}

void function_net_send_cb()
{
    xTaskNotify(net_send_task_handle, 1, eSetBits);
}

uint8_t function_reconnect_cb()
{
    uint8_t ret;
    uint8_t deviceId = DEV_STA_ID;

    if(!g_wifi_ready) {
        qapi_WLAN_Set_Param (deviceId, __QAPI_WLAN_PARAM_GROUP_WIRELESS_SECURITY,
            __QAPI_WLAN_PARAM_GROUP_SECURITY_PASSPHRASE,
            (void *)pg_wifi_demo_cxt.passphrase, pg_wifi_demo_cxt.passphrase_len, FALSE);

        /* Set auth mode and encryption type */
        gp_wlan_qapi_cxt->connect_cmd.authMode = pg_wifi_demo_cxt.authMode;
        gp_wlan_qapi_cxt->connect_cmd.dot11AuthMode = pg_wifi_demo_cxt.dot11AuthMode;
        gp_wlan_qapi_cxt->connect_cmd.pairwiseCryptoType = pg_wifi_demo_cxt.pairwiseCryptoType;
        gp_wlan_qapi_cxt->connect_cmd.groupCryptoType = pg_wifi_demo_cxt.groupCryptoType;

        qapi_WLAN_Set_Param (0, __QAPI_WLAN_PARAM_GROUP_WIRELESS,
            __QAPI_WLAN_PARAM_GROUP_WIRELESS_SSID,
            (void *)pg_wifi_demo_cxt.ssid, pg_wifi_demo_cxt.ssid_len, FALSE);

        ret = qapi_WLAN_Commit(deviceId);
        info_printf("connect to ssid return %d\n", ret);
    }
    return ret;
}

void function_net_send_task()
{
    BaseType_t xResult;
	uint32_t notified_value = 0;
    uint32_t send_bytes;
    uint32_t send_times = 0;

    while(1) {
        xResult = xTaskNotifyWait( pdFALSE, ULONG_MAX, &notified_value, portMAX_DELAY);
        if (xResult == pdPASS) {
            if (notified_value && g_wifi_ready) {
                do {
                    if (dtim_iperf_tCxt->buffer == NULL) {
                        while ((dtim_iperf_tCxt->buffer = malloc(dtim_iperf_tCxt->params.tx_params.packet_size)) == NULL) {
                            qurt_thread_sleep(100);
                        }
                    }
                    pattern(dtim_iperf_tCxt->buffer, dtim_iperf_tCxt->params.tx_params.packet_size);
                    send_bytes =
                        send(dtim_iperf_tCxt->sock_peer, dtim_iperf_tCxt->buffer, dtim_iperf_tCxt->params.tx_params.packet_size, 0);
                    send_times++;
                    if(log_enable)
                        info_printf("===== sent %u bytes for %u times =====\r\n", send_bytes, send_times); 
                    if ((dtim_iperf_tCxt->params.tx_params.interval_us > 0) && (send_times < dtim_iperf_tCxt->params.tx_params.packet_number)) {
                        qurt_thread_sleep(dtim_iperf_tCxt->params.tx_params.interval_us);
                    }
                } while (send_times < dtim_iperf_tCxt->params.tx_params.packet_number);
                send_times = 0;
            }
        }
    }

    vTaskDelete(NULL);
}

qapi_Status_t iperf_for_powertest(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    BaseType_t xResult = pdFAIL;
    uint32_t notified_value = 0;
    unsigned int index = 0;
    TickType_t dtim_time = 0;
    TickType_t roaming_time = 5000;
    char *receiver_ip;
    unsigned int ipAddress = 0;
    unsigned int pktSize = 0;
    g_wifi_ready = 1;

    /* Wifi shell init*/
    pg_wifi_shell_cxt = &g_wifi_shell_cxt;
    memset(&g_wifi_shell_cxt, 0, sizeof(wifi_shell_cxt_t));
    pg_wifi_shell_cxt->auth = QAPI_WLAN_AUTH_NONE_E;
    
    gp_wlan_qapi_cxt->qapi_event_handler = NULL;
    qapi_WLAN_Set_Callback(wlan_shell_event_handler, NULL);

    /* Save connection info for potential reconnection */
    memscpy(pg_wifi_demo_cxt.ssid, __QAPI_WLAN_MAX_SSID_LEN+1, gp_wlan_qapi_cxt->connect_cmd.ssid, __QAPI_WLAN_MAX_SSID_LEN+1);
    pg_wifi_demo_cxt.ssid_len = gp_wlan_qapi_cxt->connect_cmd.ssidLength;
    memscpy(pg_wifi_demo_cxt.passphrase, WMI_PASSPHRASE_LEN+1, gp_wlan_qapi_cxt->passphrase_cmd.passphrase, WMI_PASSPHRASE_LEN+1);
    pg_wifi_demo_cxt.passphrase_len = gp_wlan_qapi_cxt->passphrase_cmd.passphrase_len;
    pg_wifi_demo_cxt.authMode = gp_wlan_qapi_cxt->connect_cmd.authMode;
    pg_wifi_demo_cxt.dot11AuthMode = gp_wlan_qapi_cxt->connect_cmd.dot11AuthMode;
    pg_wifi_demo_cxt.groupCryptoType = gp_wlan_qapi_cxt->connect_cmd.groupCryptoType;
    pg_wifi_demo_cxt.pairwiseCryptoType = gp_wlan_qapi_cxt->connect_cmd.pairwiseCryptoType;

    if (nt_qurt_thread_create(function_net_send_task, "net_send_task", STA_TASK_STACK_SIZE, NULL, 5, &net_send_task_handle) != pdPASS) {
        info_printf("net_send_task create fail\n");
        goto ERROR_1;
    }
	
    dtim_iperf_tCxt = malloc(sizeof(THROUGHPUT_CXT));
    if (dtim_iperf_tCxt == NULL) {
        info_printf("Memory alloc failed\n");
        return QAPI_ERR_NO_MEMORY;
    }
    memset(dtim_iperf_tCxt, 0, sizeof(THROUGHPUT_CXT));

    dtim_iperf_tCxt->protocol = TCP;
    dtim_iperf_tCxt->params.tx_params.port = IPERF_DEFAULT_PORT;
    while (index < Parameter_Count) {
        if (0 == strcmp(Parameter_List[index].String_Value, "-u")) {
            index++;
            dtim_iperf_tCxt->protocol = UDP;
        } else if (0 == strcmp(Parameter_List[index].String_Value, "-d")) {
            index++;
            dtim_time = Parameter_List[index].Integer_Value;
            index++;
        } else if (0 == strcmp(Parameter_List[index].String_Value, "-p")) {
            index++;
            dtim_iperf_tCxt->params.tx_params.port = Parameter_List[index].Integer_Value;
            index++;
        } else if (0 == strcmp(Parameter_List[index].String_Value, "-c")) {
            index++;
            receiver_ip = Parameter_List[index].String_Value;
            if (inet_pton(AF_INET, receiver_ip, &ipAddress) != 1) {
                info_printf("Incorrect IP address %s\n", receiver_ip);
                return QAPI_ERR_INVALID_PARAM;
            }
            index++;
        } else if (0 == strcmp(Parameter_List[index].String_Value, "-l")) {
            index++;
            pktSize = Parameter_List[index].Integer_Value;
            index++;
            pktSize = pktSize < 12 ? 12 : pktSize;
        } else if (0 == strcmp(Parameter_List[index].String_Value, "-n")) {
            index++;
            dtim_iperf_tCxt->params.tx_params.packet_number = Parameter_List[index].Integer_Value;
            index++;
        } else if (0 == strcmp(Parameter_List[index].String_Value, "-i")) {
            index++;
            dtim_iperf_tCxt->params.tx_params.interval_us = Parameter_List[index].Integer_Value;
            index++;
        } else if (0 == strcmp(Parameter_List[index].String_Value, "-log")) {
            index++;
            log_enable = Parameter_List[index].Integer_Value;
            index++;
        } else {
            index++;
        }
    }

    if (pktSize > 0) {
        if (dtim_iperf_tCxt->protocol == TCP) {
            dtim_iperf_tCxt->params.tx_params.packet_size = min(pktSize, IPERF_MAX_PACKET_SIZE_TCP);
        } else {
            dtim_iperf_tCxt->params.tx_params.packet_size = min(pktSize, IPERF_MAX_PACKET_SIZE_UDP);
        }
    } else {
        if (dtim_iperf_tCxt->protocol == TCP) {
            dtim_iperf_tCxt->params.tx_params.packet_size = IPERF_MAX_PACKET_SIZE_TCP;
        } else {
            dtim_iperf_tCxt->params.tx_params.packet_size = IPERF_MAX_PACKET_SIZE_UDP;
        }
    }

    iperf_timer = nt_create_timer(function_net_send_cb, NULL, NT_MS_TO_TICKS(dtim_time), TRUE);
    roaming_timer = nt_create_timer(function_reconnect_cb, NULL, NT_MS_TO_TICKS(roaming_time), TRUE);

    if (dtim_iperf_tCxt->protocol == TCP) {
        if ((dtim_iperf_tCxt->sock_peer = socket(AF_INET, SOCK_STREAM, 0)) == A_ERROR) {
            info_printf("Socket creation failed\n");
            goto ERROR_1;
        }
    } else {
        if ((dtim_iperf_tCxt->sock_peer = socket(AF_INET, SOCK_DGRAM, 0)) == A_ERROR) {
            info_printf("Socket creation failed\n");
            goto ERROR_1;
        }
    }

    struct sockaddr_in si_other;
    memset((char *)&si_other, 0, sizeof(si_other));
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(dtim_iperf_tCxt->params.tx_params.port);
    dtim_iperf_tCxt->params.tx_params.ip_address = ipAddress;
    si_other.sin_addr.s_addr = dtim_iperf_tCxt->params.tx_params.ip_address;

    if (connect(dtim_iperf_tCxt->sock_peer, (struct sockaddr *)&si_other, sizeof(si_other)) == -1) {
        info_printf("Connection failed\n");
        goto ERROR_1;
    }

    pm_enable();
    nt_start_timer(iperf_timer);

ERROR_1:

    return QAPI_OK;
}

const QAPI_Console_Command_t Powertest_Command_List[] = {
    /* cmd_function                     cmd_string      usage_string              description */
    {iperf_for_powertest, "iperf_for_powertest", "\n\niperf_for_powertest", "iperf in dtim sleep"},
};

const QAPI_Console_Command_Group_t Powertest_Command_Group = {
    "Powertest", /* Power Test */
    sizeof(Powertest_Command_List) / sizeof(QAPI_Console_Command_t),
    Powertest_Command_List,
};

void Initialize_powertest_Demo(void)
{
    powertest_shell_cmd_group_handle = QAPI_Console_Register_Command_Group(NULL, &Powertest_Command_Group);
    if (powertest_shell_cmd_group_handle) {
        info_printf("Powertest Registered \n");
    }
}