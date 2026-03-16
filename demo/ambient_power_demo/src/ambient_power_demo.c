/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "qapi_wlan.h"
#include "qapi_console.h"
#include "qapi_net_status.h"
#include "lwip/sockets.h"
#include "netifapi.h"
#include "data_path.h"
#include "wmi.h"

#define IPv4_IP_IDX				0
#define IPv4_NETMASK_IDX		1
#define IPv4_GATEWAY_IDX		2
#define IPv6_LINK_LOCAL_IDX		0
#define DEFAULT_NETIF_IDX   netif_get_index(netif_default) /* Default netif idx */
#define info_printf(msg,...)     printf("WLAN: " msg, ##__VA_ARGS__)
#ifndef DEV_STA_ID
#define DEV_STA_ID			1
#endif

#define WLAN_AP_SSID "ambient_demo_2g"
#define WLAN_STATIC_IP_ADDR "192.168.1.200"
#define WLAN_DEFAULT_GATEWAY "192.168.1.1"
#define SERVER_IP_ADDR "192.168.1.101"  /* IP address of the server */
#define SERVER_PORT 8000
#define SHARED_MEMORY_WITH_HOST_ADDR 0x60000

/**********************************************************************/
typedef struct wifi_cxt_s {
    qbool_t         connected;
    char            ssid[__QAPI_WLAN_MAX_SSID_LEN + 1];
} wifi_cxt_s;

static wifi_cxt_s g_wifi_cxt = {0};
nt_osal_task_handle_t  ambient_power_task_hdl;
int32_t sock_peer;

typedef struct ambient_power_sensor_data {
    int         temperature;
    uint32_t    humidity;
    uint32_t    accelerate;
    uint32_t    atmosphere;
} ambient_power_sensor_data_t;

extern qapi_Status_t qapi_pm_enable(uint8_t enable);
extern qapi_Status_t wmi_cmd_send(WMI_COMMAND_ID cmd_id, void *p_data, uint32_t data_len);
extern void qurt_thread_sleep(uint32 duration);
extern void ping_init();

/**********************************************************************/
void pm_enable()
{
    info_printf("====  sleep  ====\n"); 
    qapi_pm_enable(1);

    WMI_BMPS_IGNORE_BCMC ignore_bcmc_data;
    memset(&ignore_bcmc_data, 0, sizeof(ignore_bcmc_data));
    ignore_bcmc_data.enable = 1;
    wmi_cmd_send(WMI_BMPS_IGNORE_BCMC_CMDID, &ignore_bcmc_data, sizeof(ignore_bcmc_data)); 
    
    /* idle time is set to 200ms on previous demo, reduce to 50ms here */
    qapi_bmps_cfg(1, 50);
}

/**********************************************************************/
static uint8_t
get_netifidx_by_devmode(u8_t devid)
{
  struct netif *netif;

  if (devid<= AP_DEVICE) {
    NETIF_FOREACH(netif) {
      if (devid == ((device_t *)netif->state)->role) {
        return netif->num+1; /* found! */
      }
    }
  }

  return 0;
}

/**********************************************************************/
static struct netif * ambient_get_netif_by_device(int devid)
{
    uint8_t netid;

    netid = get_netifidx_by_devmode(devid);
    return netif_get_by_index(netid);
}

static qapi_Status_t
ambient_net_set_ip(struct netif *netif, ip_addr_t *ip, s8_t idx)
{
    if (netif == NULL) {
        netif = netif_get_by_index(DEFAULT_NETIF_IDX);
    }

    if (netif == NULL && ip_addr_isany(ip) && ((idx < 0) && (idx > 2))) {
        info_printf("Default network interface not initialized\n");
        return QAPI_NET_ERR_CANNOT_GET_SCOPEID;
    }

#if LWIP_IPV6
    if (IP_IS_V6_VAL(*ip)) {
        netif_ip6_addr_set(netif, idx, (const ip6_addr_t*)ip_2_ip6(ip));
        netif_ip6_addr_set_state(netif,idx,IP6_ADDR_VALID);
    }
#endif
#if LWIP_IPV4
    if (IP_IS_V4_VAL(*ip)) {
        if(idx == IPv4_IP_IDX)
            netif_set_ipaddr(netif, (const ip4_addr_t*)ip_2_ip4(ip));
        else if(idx == IPv4_NETMASK_IDX)
            netif_set_netmask(netif, (const ip4_addr_t*)ip_2_ip4(ip));
        else if(idx == IPv4_GATEWAY_IDX)
            netif_set_gw(netif, (const ip4_addr_t*)ip_2_ip4(ip));
    }
#endif
    return QAPI_OK;
}

/**********************************************************************/
int ambient_power_net_static_ip()
{
    struct netif *netif = NULL;
    netif = ambient_get_netif_by_device(STA_DEVICE);
    ip_addr_t ip_addr;
    ip_addr_t netmask;   
    ip_addr_t gw_addr;

    info_printf("==== set static ip address ====\n");
	if (!ipaddr_aton(WLAN_STATIC_IP_ADDR, &ip_addr)) {
		info_printf("Please try again invalid IP Address \n");
		return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
	}
    ambient_net_set_ip(netif, &ip_addr, IPv4_IP_IDX);

    IP_ADDR4(&netmask, 255, 255, 0, 0);
    info_printf("Received subnet mask: %s\n", ipaddr_ntoa(&netmask));
    ambient_net_set_ip(netif, &netmask, IPv4_NETMASK_IDX);  

    if (!ipaddr_aton(WLAN_DEFAULT_GATEWAY, &gw_addr)) {
       info_printf("Please try again invalid gateway IP Address \n");
       return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }
    ambient_net_set_ip(netif, &gw_addr, IPv4_GATEWAY_IDX);

    return 0;
}

/**********************************************************************/
static void wlan_event_handler(__unused uint8_t deviceId, uint32_t cbId, void __unused *pApplicationContext, void *payload, uint32_t payload_Length)
{
    switch(cbId) {
        case QAPI_WLAN_CONNECT_CB_E: {
            qapi_WLAN_Join_Comp_Evt_t *cxnInfo  = (qapi_WLAN_Join_Comp_Evt_t *)(payload);
            uint8_t * mac = cxnInfo->bssid;

            if (cxnInfo->evt_hdr.status == QAPI_OK) {
    			if(cxnInfo->bss_Connection_Status)
                info_printf("devid -  %d CONNECTED MAC addr %02x:%02x:%02x:%02x:%02x:%02x\n",
                     cxnInfo->bss_Connection_Status, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
                /* set ip address */
                ambient_power_net_static_ip();
            
                /* enter sleep */  
                pm_enable();   
            } else {
    			info_printf("WiFi disconnect reason code is %d\n", cxnInfo->reason_code);
    			if (cxnInfo->bss_Connection_Status) {
    				info_printf("devId  Disconnected MAC addr %02x:%02x:%02x:%02x:%02x:%02x \n",
    					 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    			} else {
    				info_printf("REF_STA Disconnected MAC addr %02x:%02x:%02x:%02x:%02x:%02x devId \n",
                         mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    			}
            }
            info_printf("channel_frequency=%d\n", cxnInfo->channel_frequency);
            info_printf("ssid = %s\n", cxnInfo->ssid);
            info_printf("assoc_id=%d\n", cxnInfo->assoc_id);
            info_printf("host_initiated=%d\n", cxnInfo->host_initiated);
                        
            break;
        }
        case QAPI_WLAN_DISCONNECT_CB_E: {
            qapi_WLAN_Join_Comp_Evt_t *cxnInfo = (qapi_WLAN_Join_Comp_Evt_t *)(payload);

            if (cxnInfo->ssid_Length)
                info_printf("devId  disconnected from ssid = %s\n", cxnInfo->ssid);

            break;
        }
    }
}

/*****************************************************************/
static qapi_Status_t wlan_Enable()
{
    qapi_Status_t ret = QAPI_OK;
	wifi_cxt_s *p_cxt = &g_wifi_cxt;
    qapi_WLAN_DEV_Mode_e devMode = DEV_MODE_STATION_E;

    qapi_WLAN_Set_Callback(wlan_event_handler, p_cxt);
    ret = qapi_WLAN_Enable(true);
    if (QAPI_OK != ret) {
        info_printf("Wifi Enable Failed\n");
        return ret;
    }

	ret = qapi_WLAN_Set_Param(DEV_STA_ID, 
							__QAPI_WLAN_PARAM_GROUP_WIRELESS,
							__QAPI_WLAN_PARAM_GROUP_WIRELESS_OPERATION_MODE,
							&devMode,
							sizeof(devMode),
							FALSE);
							
	if (ret != QAPI_OK) {
		info_printf("set device to station mode fail!\n");
	} else {
        info_printf("set device to station mode success!\n");
	}

    qapi_WLAN_Listen_Interval_Params_t listen_interval;
    listen_interval.time = 1000;
    listen_interval.round_type = 0;
    ret = qapi_WLAN_Set_Param (DEV_STA_ID,
                                __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                                __QAPI_WLAN_PARAM_GROUP_WIRELESS_STA_LISTEN_INTERVAL_IN_TU,
                                &listen_interval,
                                sizeof(listen_interval),
                                FALSE);

    if (ret != QAPI_OK) {
        info_printf("set STA listen interval fail\n");
    } else {
        info_printf("set STA listen interval success\n");
    }

    return ret;
}

/*****************************************************************/
static qapi_Status_t wlan_Connect()
{
    qapi_Status_t ret= QAPI_OK;
    int ssidLength = 0;
    char *ssid = NULL;
    
    ssid = WLAN_AP_SSID;
    info_printf("connect to ssid %s\n", ssid);
    ssidLength = strlen(ssid);
    qapi_WLAN_Set_Param (DEV_STA_ID, __QAPI_WLAN_PARAM_GROUP_WIRELESS,
        __QAPI_WLAN_PARAM_GROUP_WIRELESS_SSID,
        (void *)ssid, ssidLength, FALSE);

    ret = qapi_WLAN_Commit(DEV_STA_ID);
    return ret;
}

void ambient_power_connection_init()
{
    tcpip_init(NULL, NULL);
    ping_init();

    info_printf("==== wlan enable ====\n");
    wlan_Enable();
    
    info_printf("==== wlan connect ====\n");
    wlan_Connect();
}

void spi_rx_data_indication()
{
    if (ambient_power_task_hdl == NULL) {
        return;
    }

    char data=1;
    BaseType_t wakeup_task=pdFALSE;
    BaseType_t ok = xTaskNotifyFromISR(ambient_power_task_hdl, data, eSetBits, &wakeup_task);

    portYIELD_FROM_ISR(wakeup_task);
}

/**********************************************************************/
void ambient_power_main()
{
    BaseType_t xResult=pdFAIL;
    uint32_t notified_value = 0;

    ambient_power_connection_init();

    if ((sock_peer = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        info_printf("Socket creation failed\n");
    } else {
        struct sockaddr_in si_other;
        memset((char *) &si_other, 0, sizeof(si_other));
        si_other.sin_family = AF_INET;
        si_other.sin_port = htons(SERVER_PORT);
        if (inet_aton(SERVER_IP_ADDR, &si_other.sin_addr) == 0) {
            info_printf("inet_aton() failed\n");
        }
        if (connect(sock_peer, (struct sockaddr *)&si_other, sizeof(si_other)) == -1) {
            info_printf("Connection failed\n");
        }
    }
    
    for ( ;; ) {
        xResult=xTaskNotifyWait(pdFALSE,0xFFFFFFFF,&notified_value,portMAX_DELAY);
        if (xResult != pdPASS) {
            info_printf("ambient_power_main rx failed\n");
            continue;
        }
        if (notified_value==1) {
            uint32_t sensor_data_addr=SHARED_MEMORY_WITH_HOST_ADDR;
            ambient_power_sensor_data_t sensor_data;
            sensor_data = *(ambient_power_sensor_data_t *)sensor_data_addr;
            info_printf("spi has data: %d %u %u %u \n", sensor_data.temperature, sensor_data.humidity, sensor_data.accelerate, sensor_data.atmosphere);

            int32_t send_bytes;
            send_bytes = send(sock_peer, &sensor_data, sizeof(sensor_data), 0);
            info_printf("==== send %u bytes ====\n",send_bytes);              
        }
    }
}

/**********************************************************************/
void ambient_power_demo_main()
{
    if (nt_qurt_thread_create(ambient_power_main, "ambient_power_task", 2048, NULL, 6, &ambient_power_task_hdl) == pdPASS) {
        info_printf("thread create success!\n");
    }
}


