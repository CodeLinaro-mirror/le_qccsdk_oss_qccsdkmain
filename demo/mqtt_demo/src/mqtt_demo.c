/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "qapi_wlan.h"
#include "qurt_internal.h"
#include "safeAPI.h"
#include "ip_addr.h"
#include "netif.h"
#include "dhcp.h"
#include "network_al.h"
#include "mqtt_demo.h"


#define info_printf(msg,...)     printf("WLAN: " msg, ##__VA_ARGS__)

#ifndef DEV_STA_ID
#define DEV_STA_ID			1
#endif

/**********************************************************************/
typedef struct wifi_cxt_s {
    qbool_t         connected;
    char            ssid[__QAPI_WLAN_MAX_SSID_LEN + 1];
} wifi_cxt_s;

static wifi_cxt_s g_wifi_cxt = {0};


/**********************************************************************/
void wlan_Event_Callback(__unused uint8_t deviceId, uint32_t cbId, void __unused *pApplicationContext, void *payload, uint32_t __unused payload_Length)
{
    wifi_cxt_s *p_cxt = &g_wifi_cxt;
    qapi_WLAN_Join_Comp_Evt_t *cxnInfo  = (qapi_WLAN_Join_Comp_Evt_t *)(payload);
    int ssidLength = 0;
    if(cxnInfo->ssid_Length) 
    {
        memscpy(p_cxt->ssid, __QAPI_WLAN_MAX_SSID_LEN, cxnInfo->ssid, cxnInfo->ssid_Length);
        ssidLength = (cxnInfo->ssid_Length <= __QAPI_WLAN_MAX_SSID_LEN) ? cxnInfo->ssid_Length : __QAPI_WLAN_MAX_SSID_LEN;
        p_cxt->ssid[ssidLength] = 0;
    }
    
    if(QAPI_WLAN_CONNECT_CB_E == cbId)
    {
        if(cxnInfo->evt_hdr.status == QAPI_OK)
        {
            p_cxt->connected = true;
            info_printf("WiFi connect to %s successful!\n", p_cxt->ssid);
        }
        else
        {
            info_printf("WiFi disconnect reason code is %d\n", cxnInfo->reason_code);
        }
    }
}

/*****************************************************************/
static qapi_Status_t wlan_Enable()
{
    qapi_Status_t ret = QAPI_OK;
	wifi_cxt_s *p_cxt = &g_wifi_cxt;
    qapi_WLAN_DEV_Mode_e devMode = DEV_MODE_STATION_E;

    qapi_WLAN_Set_Callback(wlan_Event_Callback, p_cxt);
    ret = qapi_WLAN_Enable(true);
    if (QAPI_OK != ret){
        info_printf("Wifi Enable Failed\n");
        return ret;
    }

	ret = qapi_WLAN_Set_Param(DEV_STA_ID, 
							__QAPI_WLAN_PARAM_GROUP_WIRELESS,
							__QAPI_WLAN_PARAM_GROUP_WIRELESS_OPERATION_MODE,
							&devMode,
							sizeof(devMode),
							FALSE);
							
	if(ret != QAPI_OK) {
		info_printf("set device to station mode fail!\n");
	} else {
        info_printf("set device to station mode success!\n");
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
    ssidLength = strlen(ssid);
    qapi_WLAN_Set_Param (DEV_STA_ID, __QAPI_WLAN_PARAM_GROUP_WIRELESS,
        __QAPI_WLAN_PARAM_GROUP_WIRELESS_SSID,
        (void *)ssid, ssidLength, FALSE);

    ret = qapi_WLAN_Commit(DEV_STA_ID);
    return ret;
}


/**********************************************************************/
static qapi_Status_t dhcpEnable()
{
    qapi_Status_t returnStatus = QAPI_ERROR;
    struct netif *netif = NULL;
    uint8_t netid;
    netid = nt_get_netifidx_by_devmode(STA_DEVICE);
	netif = netif_get_by_index(netid);

    if(netif == NULL){
        info_printf("netif is NULL!\n");
        return QAPI_ERR_INVALID_PARAM;
    }

    err_t status;
    netif_set_addr(netif, IP4_ADDR_ANY4, IP4_ADDR_ANY4, IP4_ADDR_ANY4);
    status = dhcp_start(netif);
    if( status == QAPI_OK){
        info_printf("DHCP client start success!\n");
        returnStatus = QAPI_OK;
    }
    else{
        info_printf("DHCP client start failed!\n");
    }
    return returnStatus;
}

/**********************************************************************/
static boolean isDhcpSuccess()
{
    boolean returnStatus = FALSE;
    struct netif* netif = NULL;
    struct dhcp *dhcp = NULL;
    uint8_t netid;

    netid = nt_get_netifidx_by_devmode(STA_DEVICE);
	netif = netif_get_by_index(netid);

    if(netif == NULL)
    {
        info_printf("The DHCP failure reason is that the netif is NULL!\n");
    }
    else
    {
        dhcp = netif_dhcp_data(netif);
        while((dhcp == NULL) || (dhcp->state != DHCP_STATE_BOUND))
        {
            qurt_thread_sleep(200);
        }

        info_printf("DHCP is success!\n");
        returnStatus = TRUE;
    }
    return returnStatus;
}

/**********************************************************************/
extern int mqtt_plaintext_demo();
extern int mqtt_basic_tls_demo();
extern int mqtt_mutual_auth_demo();

void mqtt_main()
{
    int returnStatus = EXIT_FAILURE;

    #ifdef CONFIG_MQTT_PLAINTEXT_DEMO
        returnStatus = mqtt_plaintext_demo();
    #endif

    #ifdef CONFIG_MQTT_BASIC_TLS_DEMO
        returnStatus = mqtt_basic_tls_demo();
    #endif

    #ifdef CONFIG_MQTT_MUTUAL_AUTH_DEMO
        returnStatus = mqtt_mutual_auth_demo();
    #endif

    if( returnStatus == EXIT_SUCCESS )
    {
        info_printf("mqtt demo success!\n");
    }
    else
    {
        info_printf("mqtt demo fail!\n");
    }
    nt_osal_thread_delete(NULL);
}

/**********************************************************************/
void mqtt_demo_main()
{
    wlan_Enable();
    wlan_Connect();

    wifi_cxt_s *p_cxt = &g_wifi_cxt;
    while(!p_cxt->connected)
    {
        qurt_thread_sleep(200);
    }
    info_printf("wlan is connected!\n");

    dhcpEnable();
    if(isDhcpSuccess())
    {
        if (nt_qurt_thread_create(mqtt_main, "mqtt_client_task", 2048, NULL, 6, NULL) == pdPASS)
        {
            info_printf("thread create success!\n");
        }
        else
        {
            info_printf("thread create fail!\n");
        }
    }
}
