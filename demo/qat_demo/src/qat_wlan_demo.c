/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*-------------------------------------------------------------------------
 * Include Files
 *-----------------------------------------------------------------------*/
#include <stdio.h>
#include <stdarg.h>
#include "string.h"
#include "qapi_version.h"
#include "qapi_rtc.h"
#include "qat.h"
#include "qat_api.h"
#include "qurt_internal.h"
#include "nt_osal.h"
#include "qurt_mutex.h"
#include "wifi_fw_version.h"
#include "wifi_fw_pmu_ts_cfg.h"

#include "qapi_wlan.h"
#include "wmi.h"
#include "qapi_wlan_base.h"
#include "wifi_cmn.h"
#include "safeAPI.h"

#ifndef NT_DEV_AP_ID
#define NT_DEV_AP_ID			      0
#endif
#ifndef NT_DEV_STA_ID
#define NT_DEV_STA_ID		      1
#endif
#ifndef NT_DEFAULT_HAL_STA_ID
#define NT_DEFAULT_HAL_STA_ID		2
#endif
#ifndef NT_DEV_INV_ID
#define NT_DEV_INV_ID		      3
#endif
/*-------------------------------------------------------------------------
 * Function Declarations
 *-----------------------------------------------------------------------*/
static QAT_Command_Status_t Extend_Command_Wifisp(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_Enable(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_Disable(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_SetOperatingMode(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_Scan(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_SetWpaPassphrase(uint32_t Op_Type, uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_SetWpaParameters(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_Connect(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_Disconnect(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_SetModeOption(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_EventMessage(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_PyhMode(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_CountryCode(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List);

/* The following is the complete command list for the QAT common command demo. */
/** List of global commands that are supported when in a group. */
static QAT_Command_t QAT_Wifi_Command_List[] =
{
   {"+WIFISP",   Extend_Command_Wifisp,   QAT_OP_EXEC},
   {"+CWENABLE", Extend_Command_Enable,   QAT_OP_EXEC},
   {"+CWQABLE",  Extend_Command_Disable,  QAT_OP_EXEC},
   {"+CWMODE",   Extend_Command_SetOperatingMode,      QAT_OP_EXEC_W_PARAM | QAT_OP_QUERY | QAT_OP_EXEC},
   {"+CWLAP",    Extend_Command_Scan,     QAT_OP_EXEC},
   {"+CWWPA",    Extend_Command_SetWpaParameters,      QAT_OP_EXEC_W_PARAM | QAT_OP_EXEC},
   {"+CWPWD",    Extend_Command_SetWpaPassphrase,      QAT_OP_EXEC_W_PARAM | QAT_OP_EXEC},
   {"+CWJAP",    Extend_Command_Connect,      QAT_OP_EXEC_W_PARAM | QAT_OP_QUERY | QAT_OP_EXEC},
   {"+CWQAP",    Extend_Command_Disconnect,   QAT_OP_EXEC},
   {"+CWSOFTAP", Extend_Command_SetModeOption,     QAT_OP_EXEC_W_PARAM | QAT_OP_EXEC},
   {"+WEVT",     Extend_Command_EventMessage,      QAT_OP_EXEC_W_PARAM | QAT_OP_QUERY},
   {"+CWPHYMODE",Extend_Command_PyhMode,      QAT_OP_EXEC_W_PARAM | QAT_OP_EXEC | QAT_OP_QUERY},
   {"+CWCOUNTRY",Extend_Command_CountryCode,  QAT_OP_EXEC_W_PARAM | QAT_OP_EXEC | QAT_OP_QUERY},
};

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
	uint8_t			 active_device;
   uint8_t         wlan_enabled;
} wifi_shell_cxt_t;

/*-------------------------------------------------------------------------
 * Parameters define
 *-----------------------------------------------------------------------*/

#define WIFI_COMMAND_LIST_SIZE                    (sizeof(QAT_Wifi_Command_List) / sizeof(QAT_Command_t))

#define WLAN_RESPONSE_BUFFER_LENGTH					  128
#define SCAN_PRINT_BUFFER_LENGTH					     3200					     
#define SCAN_MODE_BLOCKING      1
#define SCAN_MODE_UNBLOCKING    2

static wifi_shell_cxt_t g_wifi_shell_cxt;
static wifi_shell_cxt_t *pg_wifi_shell_cxt;
qbool_t enable_event_reporting = true;

uint8_t qat_get_active_device()
{
	wifi_shell_cxt_t *p_cxt = pg_wifi_shell_cxt;
	return p_cxt->active_device;
}

qbool_t qat_get_device_connect_state(void)
{
	wifi_shell_cxt_t *p_cxt = pg_wifi_shell_cxt;
       
	return p_cxt->connected;
}

static void scan_results(qapi_WLAN_Scan_Comp_Evt_t *scan_coml_evt)
{
   int16_t i = 0;
   uint8_t temp_ssid[33] = {0};
   qapi_WLAN_BSS_Scan_Info_t *list = scan_coml_evt->scan_bss_info;
   int16_t num_scan = scan_coml_evt->num_bss_cur;
   int offset = 0;

   char buffer[SCAN_PRINT_BUFFER_LENGTH] = {0};

   offset += snprintf(buffer + offset, SCAN_PRINT_BUFFER_LENGTH - offset, "Scan result count:%d\r\n", num_scan);
   QAT_Response_Str(QAT_RC_QUIET, buffer);
   for (i = 0; i < num_scan; i++) {
      memscpy(temp_ssid,list[i].ssid_Length,list[i].ssid,list[i].ssid_Length);
      temp_ssid[list[i].ssid_Length] = '\0';
      if (list[i].ssid_Length == 0) {
         offset += snprintf(buffer + offset, SCAN_PRINT_BUFFER_LENGTH - offset, "ssid = SSID Not available");
      } else
      {
         offset += snprintf(buffer + offset, SCAN_PRINT_BUFFER_LENGTH - offset, "ssid = %s\r\n bssid = %.2x:%.2x:%.2x:%.2x:%.2x:%.2x\r\n channel = %d\r\n rssi = %d\r\n security = \r\n", temp_ssid, 
            list[i].bssid[0],list[i].bssid[1],list[i].bssid[2],list[i].bssid[3],list[i].bssid[4],list[i].bssid[5], list[i].channel, list[i].rssi);
         if(list[i].security_Enabled) {
            if(list[i].rsn_Auth || list[i].rsn_Cipher) {
               if((list[i].rsn_Auth & __QAPI_WLAN_SECURITY_AUTH_1X) || (list[i].rsn_Auth & __QAPI_WLAN_SECURITY_AUTH_PSK))
                  offset += snprintf(buffer + offset, SCAN_PRINT_BUFFER_LENGTH - offset, "RSN/WPA2=");
               if(list[i].rsn_Auth & __QAPI_WLAN_SECURITY_AUTH_SAE)
                  offset += snprintf(buffer + offset, SCAN_PRINT_BUFFER_LENGTH - offset, "WPA3=");
            }

            if(list[i].rsn_Auth) {
               offset += snprintf(buffer + offset, SCAN_PRINT_BUFFER_LENGTH - offset, " {");
               if(list[i].rsn_Auth & __QAPI_WLAN_SECURITY_AUTH_1X) {
                  offset += snprintf(buffer + offset, SCAN_PRINT_BUFFER_LENGTH - offset, "802.1X");
               }
               if(list[i].rsn_Auth & __QAPI_WLAN_SECURITY_AUTH_PSK){
                  offset += snprintf(buffer + offset, SCAN_PRINT_BUFFER_LENGTH - offset, "PSK");
               }
               if(list[i].rsn_Auth & __QAPI_WLAN_SECURITY_AUTH_SAE){
                  offset += snprintf(buffer + offset, SCAN_PRINT_BUFFER_LENGTH - offset, "SAE");
               }
               offset += snprintf(buffer + offset, SCAN_PRINT_BUFFER_LENGTH - offset, "}\r\n");
            }

            if(list[i].rsn_Cipher){
               offset += snprintf(buffer + offset, SCAN_PRINT_BUFFER_LENGTH - offset, " {");
               /* AP security can support multiple options hence we check each one separately. Note rsn == wpa2 */
               if(list[i].rsn_Cipher & __QAPI_WLAN_CIPHER_TYPE_WEP){
                  offset += snprintf(buffer + offset, SCAN_PRINT_BUFFER_LENGTH - offset, "WEP");
               }
               if(list[i].rsn_Cipher & __QAPI_WLAN_CIPHER_TYPE_TKIP){
                  offset += snprintf(buffer + offset, SCAN_PRINT_BUFFER_LENGTH - offset, "TKIP");
               }
               if(list[i].rsn_Cipher & __QAPI_WLAN_CIPHER_TYPE_CCMP){
                  offset += snprintf(buffer + offset, SCAN_PRINT_BUFFER_LENGTH - offset, "AES");
               }
               offset += snprintf(buffer + offset, SCAN_PRINT_BUFFER_LENGTH - offset, "}\r\n");
            }

            if(list[i].wpa_Auth || list[i].wpa_Cipher) {
               offset += snprintf(buffer + offset, SCAN_PRINT_BUFFER_LENGTH - offset, "WPA=");
            }

            if(list[i].wpa_Auth) {
               offset += snprintf(buffer + offset, SCAN_PRINT_BUFFER_LENGTH - offset, " {");
               if(list[i].wpa_Auth & __QAPI_WLAN_SECURITY_AUTH_1X){
                  offset += snprintf(buffer + offset, SCAN_PRINT_BUFFER_LENGTH - offset, "802.1X");
               }
               if(list[i].wpa_Auth & __QAPI_WLAN_SECURITY_AUTH_PSK){
                  offset += snprintf(buffer + offset, SCAN_PRINT_BUFFER_LENGTH - offset, "PSK");
               }
               offset += snprintf(buffer + offset, SCAN_PRINT_BUFFER_LENGTH - offset, "}\r\n");
            }

            if(list[i].wpa_Cipher) {
               offset += snprintf(buffer + offset, SCAN_PRINT_BUFFER_LENGTH - offset, " {");
               if(list[i].wpa_Cipher & __QAPI_WLAN_CIPHER_TYPE_WEP){
                  offset += snprintf(buffer + offset, SCAN_PRINT_BUFFER_LENGTH - offset, "WEP");
               }
               if(list[i].wpa_Cipher & __QAPI_WLAN_CIPHER_TYPE_TKIP){
                  offset += snprintf(buffer + offset, SCAN_PRINT_BUFFER_LENGTH - offset, "TKIP");
               }
               if(list[i].wpa_Cipher & __QAPI_WLAN_CIPHER_TYPE_CCMP){
                  offset += snprintf(buffer + offset, SCAN_PRINT_BUFFER_LENGTH - offset, "AES");
               }
               offset += snprintf(buffer + offset, SCAN_PRINT_BUFFER_LENGTH - offset, "}\r\n");
            }
         } else {
            offset += snprintf(buffer + offset, SCAN_PRINT_BUFFER_LENGTH - offset, "NONE! ");
         }
      }

      if(i!= num_scan - 1) {
         offset += snprintf(buffer + offset, SCAN_PRINT_BUFFER_LENGTH - offset, "\n \r");
      } else {
         offset += snprintf(buffer + offset, SCAN_PRINT_BUFFER_LENGTH - offset, "shell> ");
      }
   }

   QAT_Response_Str(QAT_RC_QUIET, buffer);

   return;
}


static void wlan_shell_event_handler(__unused uint8_t deviceId, uint32_t cbId, void __unused *pApplicationContext, void *payload, uint32_t payload_Length)
{
   wifi_shell_cxt_t *p_cxt = pg_wifi_shell_cxt;
   char buffer[WLAN_RESPONSE_BUFFER_LENGTH] = {0};
   int offset = 0;
   
   switch(cbId) {
      case QAPI_WLAN_SCAN_COMPLETE_CB_E: {
         if (!payload || !payload_Length) {
            QAT_Response_Str(QAT_RC_QUIET, "QAPI_WLAN_SCAN_COMPLETE_CB_E event error");
            break;
         }

         qapi_WLAN_Scan_Comp_Evt_t *p_scan_compl_evt = (qapi_WLAN_Scan_Comp_Evt_t*)payload;
         offset += snprintf(buffer + offset, WLAN_RESPONSE_BUFFER_LENGTH - offset, "EVT:wlan received Scan complete event, found bss count:%d\r\n", p_scan_compl_evt->num_bss_cur);
         if (p_cxt->scan_mode==SCAN_MODE_BLOCKING) {
            offset += snprintf(buffer + offset, WLAN_RESPONSE_BUFFER_LENGTH - offset, "blocking mode");
         } else if (p_cxt->scan_mode==SCAN_MODE_UNBLOCKING) {
            offset += snprintf(buffer + offset, WLAN_RESPONSE_BUFFER_LENGTH - offset, "unblocking mode");
            scan_results(p_scan_compl_evt);
         } else {
            offset += snprintf(buffer + offset, WLAN_RESPONSE_BUFFER_LENGTH - offset, "unknown mode = %d, ignore", p_cxt->scan_mode);
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
         if(cxnInfo->evt_hdr.status == QAPI_OK) {
            qapi_WLAN_Auth_Mode_e e_wpa_ver = p_cxt->auth;
            if(cxnInfo->bss_Connection_Status)
               p_cxt->connected = true;
            offset += snprintf(buffer + offset, WLAN_RESPONSE_BUFFER_LENGTH - offset, "devid_id:%d, EVT:wlan_connected,  CONNECTED MAC addr %02x:%02x:%02x:%02x:%02x:%02x\r\n",
               p_cxt->active_device, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            if (e_wpa_ver==QAPI_WLAN_AUTH_WPA_PSK_E || e_wpa_ver==QAPI_WLAN_AUTH_WPA2_PSK_E) {
               offset += snprintf(buffer + offset, WLAN_RESPONSE_BUFFER_LENGTH - offset, "4 way handshake success for device=1\r\n");
            }
         } else {
            offset += snprintf(buffer + offset, WLAN_RESPONSE_BUFFER_LENGTH - offset, "WiFi disconnect reason code is %d\r\n", cxnInfo->reason_code);
            if (cxnInfo->reason_code == RECEIVED_DEAUTH) {
               offset += snprintf(buffer + offset, WLAN_RESPONSE_BUFFER_LENGTH - offset, "EVT:wlan pask error\r\n");
            }
            if (cxnInfo->reason_code == NO_NETWORK_AVAIL) {
               offset += snprintf(buffer + offset, WLAN_RESPONSE_BUFFER_LENGTH - offset, "EVT:wlan no network available\r\n");
            }
            if(cxnInfo->bss_Connection_Status) {
               p_cxt->connected = false;
               offset += snprintf(buffer + offset, WLAN_RESPONSE_BUFFER_LENGTH - offset, "devId: %d, Disconnected MAC addr %02x:%02x:%02x:%02x:%02x:%02x \r\n",
                  p_cxt->active_device, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            } else {
               offset += snprintf(buffer + offset, WLAN_RESPONSE_BUFFER_LENGTH - offset, "REF_STA Disconnected MAC addr %02x:%02x:%02x:%02x:%02x:%02x devId %d\r\n",
                  mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], p_cxt->active_device);
            }
         }
         offset += snprintf(buffer + offset, WLAN_RESPONSE_BUFFER_LENGTH - offset, "channel_frequency=%d\r\n ssid = %s\r\n assoc_id=%d\r\n host_initiated=%d\r\n",
                  cxnInfo->channel_frequency, p_cxt->ssid, cxnInfo->assoc_id, cxnInfo->host_initiated);
         break;
      }
      case QAPI_WLAN_DISCONNECT_CB_E: {
         qapi_WLAN_Join_Comp_Evt_t *cxnInfo = (qapi_WLAN_Join_Comp_Evt_t *)(payload);
         if(cxnInfo->bss_Connection_Status) {
            p_cxt->connected = false;
         }
         
         if(p_cxt->ssid_length) {
            snprintf(buffer, WLAN_RESPONSE_BUFFER_LENGTH, "EVT:wlan disconnect_cmd, devId %d disconnected from ssid = %s", p_cxt->active_device, p_cxt->ssid);
         }
         break;
      }
      case QAPI_WLAN_CHANNEL_SWITCH_CB_E: {
         qapi_WLAN_Chan_Switch_Evt_t *ecsa = (qapi_WLAN_Chan_Switch_Evt_t *)payload;
         if(ecsa->evt_hdr.status == QAPI_OK) {
            p_cxt->channel_frequency = ecsa->freq;
            snprintf(buffer, WLAN_RESPONSE_BUFFER_LENGTH, "EVT:wlan switch channel, devId %d channel switch to %d success", p_cxt->active_device, ecsa->freq);
         } else {
            snprintf(buffer, WLAN_RESPONSE_BUFFER_LENGTH, "EVT:wlan switch channel, devId %d channel switch fail, reason %d", p_cxt->active_device, ecsa->reason);
         }
         break;
      }
   }

   if (!enable_event_reporting) {
      QAT_Response_Str(QAT_RC_OK, "Event reporting has been disabled");
   }
   else {
      QAT_Response_Str(QAT_RC_QUIET, buffer);
   }

   return;
}

static QAT_Command_Status_t Extend_Command_EventMessage(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List)
{
   char buffer[WLAN_RESPONSE_BUFFER_LENGTH] = {0};
   QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;

   switch (Op_Type)
   {
      case QAT_OP_QUERY: 	     /* AT+WEVT */
      {
         snprintf(buffer, WLAN_RESPONSE_BUFFER_LENGTH, "AT+WEVT=%d", enable_event_reporting);
		   QAT_Response_Str(QAT_RC_QUIET, buffer);
         break;
      }

      case QAT_OP_EXEC_W_PARAM:
      {
         enable_event_reporting = Parameter_List[0].Integer_Value;
         break;
      }
      default:
      ;
   }
   
   rc = QAT_Response_Str(QAT_RC_OK, NULL);
   return rc;
}

/*-------------------------------------------------------------------------
 * Function Definitions
 *-----------------------------------------------------------------------*/

/**
   @brief Processes the Extend command from the QAT.

   This command will change the current group to its parent. No parameters are
   expected for this command.

   @param[in] Op_Type          The input command type.
   @param[in] Parameter_Count  Number of parameters that were entered into the
                               command line.
   @param[in] Parameter_List   List of parameters entered into the command line.
*/
static QAT_Command_Status_t Extend_Command_Wifisp(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List)
{
   QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;

   switch (Op_Type)
   {
      case QAT_OP_EXEC: 	     /* AT+WIFISP */
      {
         if (qapi_WLAN_Enable(true) != QAPI_OK) {
            QAT_Response_Str(QAT_RC_ERROR, "get wlan mode fail");
            return rc;
         }	
         break;
      }
      default:
      ;
   }
   qapi_WLAN_Enable(false);
   rc = QAT_Response_Str(QAT_RC_OK, NULL);
   return rc;
}

/*-------------------------------------------------------------------------
 * Function Definitions
 *-----------------------------------------------------------------------*/

/**
   @brief Processes the Extend command from the QAT.

   This command will change the current group to its parent. No parameters are
   expected for this command.

   @param[in] Op_Type          The input command type.
   @param[in] Parameter_Count  Number of parameters that were entered into the
                               command line.
   @param[in] Parameter_List   List of parameters entered into the command line.
*/
static QAT_Command_Status_t Extend_Command_Enable(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List)
{
   qapi_Status_t ret;
	qapi_WLAN_DEV_Mode_e devMode = DEV_MODE_STATION_E;
	wifi_shell_cxt_t *p_cxt = pg_wifi_shell_cxt;
   QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;

   switch (Op_Type)
   {
      case QAT_OP_EXEC:   /* AT+ CWENABLE */
      {
         if(p_cxt->wlan_enabled) {
            rc = QAT_Response_Str(QAT_RC_OK, NULL);
            return rc;
         }

         qapi_WLAN_Set_Callback(wlan_shell_event_handler, &g_wifi_shell_cxt);
         ret = qapi_WLAN_Enable(true);
         if (QAPI_OK != ret) {
            QAT_Response_Str(QAT_RC_ERROR, "Cmd failed");
            return rc;
         }	
         p_cxt->wlan_enabled = 1;
         
	      //TODO To maintain consistency of Auto test tool, the default mode set to station
	      ret = qapi_WLAN_Set_Param(0, 
							__QAPI_WLAN_PARAM_GROUP_WIRELESS,
							__QAPI_WLAN_PARAM_GROUP_WIRELESS_OPERATION_MODE,
							&devMode,
							sizeof(devMode),
							FALSE);				
	      if(ret != QAPI_OK) {
            QAT_Response_Str(QAT_RC_ERROR, "set mode station fail");
            return rc;           
	      } else {
		      p_cxt->active_device = NT_DEV_STA_ID;
	      }
         break;
      }
      default:
      ;
   }
   rc = QAT_Response_Str(QAT_RC_OK, "enabled");
   return rc;
}

/*-------------------------------------------------------------------------
 * Function Definitions
 *-----------------------------------------------------------------------*/

/**
   @brief Processes the Extend command from the QAT.

   This command will change the current group to its parent. No parameters are
   expected for this command.

   @param[in] Op_Type          The input command type.
   @param[in] Parameter_Count  Number of parameters that were entered into the
                               command line.
   @param[in] Parameter_List   List of parameters entered into the command line.
*/
static QAT_Command_Status_t Extend_Command_Disable(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List)
{
    wifi_shell_cxt_t *p_cxt = pg_wifi_shell_cxt;
    qapi_Status_t ret = QAPI_ERROR;
    QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;

    switch (Op_Type)
    {
      case QAT_OP_EXEC:  /* AT+CWQABLE */
      {
         if(p_cxt->wlan_enabled == 0) {
            rc = QAT_Response_Str(QAT_RC_OK, NULL);
            return rc;
         }

         ret = qapi_WLAN_Enable(false);
         if (QAPI_OK != ret) {
            QAT_Response_Str(QAT_RC_ERROR, "Cmd failed");
            return rc;
         }
         p_cxt->wlan_enabled = 0;
         break;
      }
      default:
      ;
    }
    rc = QAT_Response_Str(QAT_RC_OK, "disabled");
    return rc;
}

static int32_t qat_set_op_mode(char *opmode, char *hiddenSsid)
{
	int32_t ret = -1;
   QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
	uint8_t hidden_flag = 0;
	qapi_WLAN_DEV_Mode_e devMode;
	wifi_shell_cxt_t *p_cxt = pg_wifi_shell_cxt;
   char buffer[WLAN_RESPONSE_BUFFER_LENGTH] = {0};

	if(!strcmp(opmode,"ap")) {
		devMode = DEV_MODE_AP_E;
		if(strcmp(hiddenSsid,"hidden") == 0) {
			hidden_flag = 1;
		}
		else if(strcmp(hiddenSsid,"0") == 0 || strcmp(hiddenSsid,"") == 0) {
			hidden_flag = 0;
		}
		else {
         QAT_Response_Str(QAT_RC_ERROR, "error input");
			return rc;
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
	else if(!strcmp(opmode,"no_ap_sta")) {
      devMode = DEV_MODE_NO_CONC_E;
   }
   else {
		snprintf(buffer, WLAN_RESPONSE_BUFFER_LENGTH, "unknown mode %s",opmode);
      QAT_Response_Str(QAT_RC_QUIET, buffer);
		return rc;
	}
	
	ret = qapi_WLAN_Set_Param(0, 
							__QAPI_WLAN_PARAM_GROUP_WIRELESS,
							__QAPI_WLAN_PARAM_GROUP_WIRELESS_OPERATION_MODE,
							&devMode,
							sizeof(devMode),
							FALSE);

	if(ret != QAPI_OK) {
		snprintf(buffer, WLAN_RESPONSE_BUFFER_LENGTH,"set mode %s fail",opmode);
      QAT_Response_Str(QAT_RC_QUIET, buffer);
		return rc;
	} else {
		if(devMode == DEV_MODE_AP_E)
			p_cxt->active_device = NT_DEV_AP_ID;
		else if(devMode == DEV_MODE_STATION_E)
			p_cxt->active_device = NT_DEV_STA_ID;
      else if (devMode == DEV_MODE_AP_STA_E)
         p_cxt->active_device = NT_DEFAULT_HAL_STA_ID;
      else if (devMode == DEV_MODE_NO_CONC_E)
         p_cxt->active_device = NT_DEV_INV_ID;
	}
	
   rc = QAT_Response_Str(QAT_RC_QUIET, NULL);
	return rc;
}

/**
   @brief Processes the Extend command from the QAT.

   This command will change the current group to its parent. No parameters are
   expected for this command.

   @param[in] Op_Type          The input command type.
   @param[in] Parameter_Count  Number of parameters that were entered into the
                               command line.
   @param[in] Parameter_List   List of parameters entered into the command line.
*/
static QAT_Command_Status_t Extend_Command_Scan(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List)
{
   QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
   qapi_Status_t ret = QAPI_OK;
   wifi_shell_cxt_t *p_cxt = pg_wifi_shell_cxt;
   qapi_WLAN_Start_Scan_Params_t scan_param = {0};
   qbool_t scan_ssid = false;
	qapi_WLAN_DEV_Mode_e opmode;
	uint32_t length = sizeof(qapi_WLAN_DEV_Mode_e);
	uint8_t deviceId = qat_get_active_device();
   char buffer[WLAN_RESPONSE_BUFFER_LENGTH] = {0};

   if (0 == p_cxt->wlan_enabled)
   {
      QAT_Response_Str(QAT_RC_ERROR, "Enable WLAN before scan");
      return rc;
   }

   switch (Op_Type)
   {
      case QAT_OP_EXEC:  /* AT+CWLAP */
      {
         qurt_mutex_lock(&p_cxt->wifi_shell_cxt_mutex);
         p_cxt->scan_mode = SCAN_MODE_BLOCKING;
         if(Parameter_Count >= 1 && Parameter_List[0].Integer_Is_Valid) {
            int32_t param_scan_mode = Parameter_List[0].Integer_Value;
            if((param_scan_mode < SCAN_MODE_BLOCKING) || (param_scan_mode > SCAN_MODE_UNBLOCKING)) {
               snprintf(buffer, WLAN_RESPONSE_BUFFER_LENGTH, "Invalid scan mode (%d)", param_scan_mode);
               QAT_Response_Str(QAT_RC_ERROR, buffer);
               qurt_mutex_unlock(&p_cxt->wifi_shell_cxt_mutex);
               return rc;
            }
            p_cxt->scan_mode = param_scan_mode;
         }
         if(Parameter_Count >= 2 && !Parameter_List[1].Integer_Is_Valid) {
            uint8_t ssid_Length = strlen((char *) Parameter_List[1].String_Value);
            if(ssid_Length > __QAPI_WLAN_MAX_SSID_LEN) {
               QAT_Response_Str(QAT_RC_ERROR, "SSID length exceeds Maximum value");
               qurt_mutex_unlock(&p_cxt->wifi_shell_cxt_mutex);
               return rc;
            }
            scan_param.ssid_Length = ssid_Length;
            memscpy(scan_param.ssid, ssid_Length, Parameter_List[1].String_Value, ssid_Length);
            scan_ssid = true;
         }
         if(QAT_STATUS_SUCCESS_E != qapi_WLAN_Get_Param (deviceId,
									__QAPI_WLAN_PARAM_GROUP_WIRELESS,
									__QAPI_WLAN_PARAM_GROUP_WIRELESS_OPERATION_MODE,
									&opmode,
									&length)) {							
            snprintf(buffer, WLAN_RESPONSE_BUFFER_LENGTH, "get operation mode fail for device %d",deviceId);
            QAT_Response_Str(QAT_RC_ERROR, buffer);
		      return rc;
         }	
	      if(opmode != DEV_MODE_STATION_E) {
            snprintf(buffer, WLAN_RESPONSE_BUFFER_LENGTH, "current operation mode %d do not support scan, need to set station mode",opmode);
            QAT_Response_Str(QAT_RC_ERROR, buffer);
		      return rc;
	      }
         snprintf(buffer, WLAN_RESPONSE_BUFFER_LENGTH, "scan_mode=%d", p_cxt->scan_mode);
         QAT_Response_Str(QAT_RC_QUIET, buffer);
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

         qapi_WLAN_Get_Scan_Results(deviceId, &scan_complete_evt, &bss_cnt);
         bss_cnt = scan_complete_evt.num_bss_cur;
         qapi_WLAN_Scan_Comp_Evt_t *scan_complete_evt_total = malloc(sizeof(qapi_WLAN_Scan_Comp_Evt_t) + bss_cnt*sizeof(qapi_WLAN_BSS_Scan_Info_t));
         qapi_WLAN_Get_Scan_Results(deviceId, scan_complete_evt_total, &bss_cnt);
         if (scan_complete_evt_total) {
            scan_results(scan_complete_evt_total);
            // usleep(1000);
            free(scan_complete_evt_total);
         } else {
            QAT_Response_Str(QAT_RC_QUIET, "Failed to allocate memory to scan");
         }
      }
      break;
   }
      default:
      ;
   }
   rc = QAT_Response_Str(QAT_RC_OK, NULL);
   return rc;
}

/*-------------------------------------------------------------------------
 * Function Definitions
 *-----------------------------------------------------------------------*/

/**
   @brief Processes the Extend command from the QAT.

   This command will change the current group to its parent. No parameters are
   expected for this command.

   @param[in] Op_Type          The input command type.
   @param[in] Parameter_Count  Number of parameters that were entered into the
                               command line.
   @param[in] Parameter_List   List of parameters entered into the command line.
*/
static QAT_Command_Status_t Extend_Command_SetWpaPassphrase(uint32_t Op_Type, uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
   QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
   uint8_t deviceId = qat_get_active_device();
   char buffer[WLAN_RESPONSE_BUFFER_LENGTH] = {0};

   switch (Op_Type)
   {
      case QAT_OP_EXEC_W_PARAM: /* AT+CWPWD */
      {
         char* passphrase = Parameter_List[0].String_Value;
         uint32_t len = strlen(passphrase);

         if( Parameter_Count < 1 || !Parameter_List) {
            QAT_Response_Str(QAT_RC_ERROR, NULL);
            return rc;
         }

         if((len < 8) || (len >64)) {
            snprintf(buffer, WLAN_RESPONSE_BUFFER_LENGTH, "Wrong passphrase length=%d, the length should be between 8 and 64", len);
            QAT_Response_Str(QAT_RC_ERROR, buffer);
            return rc;
         }

         if(len == 64) {
            uint32_t i = 0;
            for (i = 0; i < len; i++) {
               if(!isxdigit((int)passphrase[i])) {
                  QAT_Response_Str(QAT_RC_ERROR, "passphrase in hex, please enter [0-9] or [A-F]");
                  return rc;
               }
            }
         }

         qapi_WLAN_Set_Param (deviceId, __QAPI_WLAN_PARAM_GROUP_WIRELESS_SECURITY,
            __QAPI_WLAN_PARAM_GROUP_SECURITY_PASSPHRASE,
         (void *)passphrase, len, FALSE);
         break;
      }

      case QAT_OP_EXEC:
      {
         QAT_Response_Str(QAT_RC_QUIET, "+CWPWD=<PASSWORD>(The PASSWORD length should be between 8 and 64)");
         break;
      }
      default:
      ;
   }
    
   rc = QAT_Response_Str(QAT_RC_OK, NULL);
   return rc;
}

/*-------------------------------------------------------------------------
 * Function Definitions
 *-----------------------------------------------------------------------*/

/**
   @brief Processes the Extend command from the QAT.

   This command will change the current group to its parent. No parameters are
   expected for this command.

   @param[in] Op_Type          The input command type.
   @param[in] Parameter_Count  Number of parameters that were entered into the
                               command line.
   @param[in] Parameter_List   List of parameters entered into the command line.
*/
static QAT_Command_Status_t Extend_Command_SetWpaParameters(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List)
{
   QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
   char buffer[WLAN_RESPONSE_BUFFER_LENGTH] = {0};
   uint8_t deviceId = qat_get_active_device();
 
   qapi_WLAN_Auth_Mode_e e_wpa_ver;
   qapi_WLAN_Crypt_Type_e e_cipher;

   switch (Op_Type)
   {
      case QAT_OP_EXEC_W_PARAM:   /* AT+CWWPA */
      {
         char *wpaVer = Parameter_List[0].String_Value;
         char *ucipher = Parameter_List[1].String_Value;
         char *mcipher = Parameter_List[2].String_Value;

         if( Parameter_Count != 3 || !Parameter_List || Parameter_List[0].Integer_Is_Valid || Parameter_List[1].Integer_Is_Valid || Parameter_List[2].Integer_Is_Valid) {
            QAT_Response_Str(QAT_RC_ERROR, NULL);
            return rc;
         }

         if(!strcmp(wpaVer,"WPA")) {
            e_wpa_ver = QAPI_WLAN_AUTH_WPA_PSK_E;
         } else if (!strcmp(wpaVer,"WPA2")) {
            e_wpa_ver = QAPI_WLAN_AUTH_WPA2_PSK_E;
         } else if (!strcmp(wpaVer, "SAE")) {
            e_wpa_ver = QAPI_WLAN_AUTH_WPA3_SAE_E;
	      } else if (!strcmp(wpaVer,"SAE_WPA2")) {
            e_wpa_ver = QAPI_WLAN_AUTH_WPA2_SAE_MIXED_E;		
         } else {
            snprintf(buffer, WLAN_RESPONSE_BUFFER_LENGTH, "invalid wpa ver =%s", wpaVer);
            QAT_Response_Str(QAT_RC_ERROR, buffer);
            return rc;
         }
         if (strcmp(ucipher, mcipher)) {
            snprintf(buffer, WLAN_RESPONSE_BUFFER_LENGTH, "invaid uchipher mcipher, should be same");
            QAT_Response_Str(QAT_RC_ERROR, buffer);
            return rc;
         }
         if (!strcmp(ucipher, "TKIP")) {
            e_cipher = QAPI_WLAN_CRYPT_TKIP_CRYPT_E;
         } else if (!strcmp(ucipher, "CCMP")) {
            e_cipher = QAPI_WLAN_CRYPT_AES_CRYPT_E;
         } else {
            snprintf(buffer, WLAN_RESPONSE_BUFFER_LENGTH, "invaid uchipher mcipher, should be TKIP or CCMP");
            QAT_Response_Str(QAT_RC_ERROR, buffer);
            return rc;
         }
         pg_wifi_shell_cxt->auth = e_wpa_ver;
         qapi_WLAN_Set_Param (deviceId, __QAPI_WLAN_PARAM_GROUP_WIRELESS_SECURITY,
            __QAPI_WLAN_PARAM_GROUP_SECURITY_AUTH_MODE,
         (void *) &e_wpa_ver, sizeof(qapi_WLAN_Auth_Mode_e), FALSE);
         qapi_WLAN_Set_Param(deviceId, __QAPI_WLAN_PARAM_GROUP_WIRELESS_SECURITY,
            __QAPI_WLAN_PARAM_GROUP_SECURITY_ENCRYPTION_TYPE,
         (void *) &e_cipher, sizeof(qapi_WLAN_Crypt_Type_e), FALSE);
      
         break;
      }
      case QAT_OP_EXEC:
      {
         QAT_Response_Str(QAT_RC_QUIET, "+CWWPA=WPA/WPA2/SEA/SAE_WPA2, CCMP, CCMP/TKIP, TKIP");
         break;
      }
      default:
      ;
   }
   rc = QAT_Response_Str(QAT_RC_OK, NULL);
   return rc;
}

static int32_t qat_set_channel(int32_t channeldata)
{
   char buffer[WLAN_RESPONSE_BUFFER_LENGTH] = {0};
	qapi_Status_t ret = QAPI_OK;
   QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
	uint8_t deviceId =qat_get_active_device();
   int32_t channel[2] = {0, 0};

   channel[0] = channeldata;
#ifdef CONFIG_6GHZ
	channel[1] = 0;
#else
   QAT_Response_Str(QAT_RC_ERROR, "cannot set 6g channel since 6g is not enabled");
#endif

   if(!pg_wifi_shell_cxt->wlan_enabled) {
      QAT_Response_Str(QAT_RC_ERROR, "Enable WLAN before set channel");
      return rc;
   }

	ret = qapi_WLAN_Set_Param(deviceId,
								__QAPI_WLAN_PARAM_GROUP_WIRELESS,
								__QAPI_WLAN_PARAM_GROUP_WIRELESS_CHANNEL,
								(void *) &channel,
								sizeof(channel),
								FALSE);
	if(ret != QAPI_OK) {
      snprintf(buffer, WLAN_RESPONSE_BUFFER_LENGTH, "set channel %d fail \n",channel[0]);
      QAT_Response_Str(QAT_RC_ERROR, buffer);
	}

   rc = QAT_Response_Str(QAT_RC_QUIET, NULL);
	return rc;
}

static int32_t qat_set_11nht_cap(char *ht_config)
{
	qapi_Status_t ret= QAPI_OK;
   QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
	uint8_t deviceId =qat_get_active_device();
	qapi_WLAN_11n_HT_Config_e htconfig;

	if(!strcmp(ht_config,"disable"))
		htconfig = QAPI_WLAN_11N_DISABLED_E;
	else if(!strcmp(ht_config,"ht20"))
		htconfig = QAPI_WLAN_11N_HT20_E;
	else {
      QAT_Response_Str(QAT_RC_ERROR, "Unknown ht config, only support disable/ht20");
      return rc;
	}
	ret = qapi_WLAN_Set_Param(deviceId, 
               __QAPI_WLAN_PARAM_GROUP_WIRELESS,
               __QAPI_WLAN_PARAM_GROUP_WIRELESS_11N_HT,
               &htconfig,
               sizeof(htconfig),
               FALSE);
   if (ret != QAPI_OK) {
      QAT_Response_Str(QAT_RC_ERROR, NULL);
      return rc;
   }

   rc = QAT_Response_Str(QAT_RC_QUIET, NULL);
   return rc;

}

int32_t qat_get_phy_mode()
{
   QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
   char buffer[WLAN_RESPONSE_BUFFER_LENGTH] = {0};
	qapi_WLAN_Phy_Mode_e phy_mode;
	char data[32+1] = {'\0'};
	uint32_t length = sizeof(qapi_WLAN_Phy_Mode_e);
	uint32_t deviceId = 0;
   if(QAPI_OK != qapi_WLAN_Get_Param (deviceId, __QAPI_WLAN_PARAM_GROUP_WIRELESS, 
         __QAPI_WLAN_PARAM_GROUP_WIRELESS_PHY_MODE, &phy_mode, &length)) {
      QAT_Response_Str(QAT_RC_ERROR, "get phy mode fail");
		return rc;
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
      snprintf(buffer, WLAN_RESPONSE_BUFFER_LENGTH, "Phy mode    = unknown (%d)",(int)phy_mode);
      QAT_Response_Str(QAT_RC_ERROR, buffer);
		return rc;
	} 

   snprintf(buffer, WLAN_RESPONSE_BUFFER_LENGTH, "Phy mode    = %s",data);
   rc = QAT_Response_Str(QAT_RC_QUIET, buffer);
	return rc;
}

void qat_regulatory_info(qapi_WLAN_Reg_Evt_t *reg_info)
{
	int idx = 0, num;
   uint16_t max_bw = 20;
   char data[32+1] = {'\0'};
	qapi_WLAN_Reg_t *reg;
   char buffer[WLAN_RESPONSE_BUFFER_LENGTH] = {0};

	if(reg_info) 
	{
      snprintf(buffer, WLAN_RESPONSE_BUFFER_LENGTH, "Country Code: %s\n", reg_info->alpha);
      QAT_Response_Str(QAT_RC_QUIET, buffer);
		reg = reg_info->reg_rules;
		num = (reg_info->num_2g_reg_rules) + (reg_info->num_5g_reg_rules);
		for(idx = 0;idx < num;idx++) {
			memset(data, 0, sizeof(data));
			if(reg[idx].ant_gain == 0)
				strlcpy(data, "N/A", sizeof(data));
			else
				snprintf(data, sizeof(data), "%d", reg[idx].ant_gain);
            snprintf(buffer, WLAN_RESPONSE_BUFFER_LENGTH, "(%d - %d @ %d),(%s,%d)\n",reg[idx].start_freq,reg[idx].end_freq,max_bw,
				data,reg[idx].reg_power,reg[idx].flag_info);
            QAT_Response_Str(QAT_RC_QUIET, buffer);
		}
	}

}

int32_t qat_get_country_code()
{
    qapi_Status_t ret = QAPI_OK;
    wifi_shell_cxt_t *p_cxt = pg_wifi_shell_cxt;
    qapi_WLAN_Reg_Evt_t reg_info;
    QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
	
    if (0 == p_cxt->wlan_enabled)
    {
        QAT_Response_Str(QAT_RC_ERROR, "Enable WLAN before get country code");
        return rc;
    }
	
    ret = qapi_WLAN_Get_Regulatory_Info(&reg_info);
    if(ret == QAPI_OK) {
        qat_regulatory_info(&reg_info);
    }

    rc = QAT_Response_Str(QAT_RC_QUIET, NULL);
    return rc;
}

int32_t qat_get_wifi_power_mode()
{
	uint8_t power_mode = 0;
	uint32_t length = sizeof(power_mode);
	uint32_t deviceId = qat_get_active_device();
	char data[64+1] = {'\0'};	
   QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
   char buffer[WLAN_RESPONSE_BUFFER_LENGTH] = {0};

   if(QAPI_OK != qapi_WLAN_Get_Param (deviceId, __QAPI_WLAN_PARAM_GROUP_WIRELESS, 
         __QAPI_WLAN_PARAM_GROUP_WIRELESS_POWER_MODE_PARAMS, &power_mode, &length)) {
      snprintf(buffer, WLAN_RESPONSE_BUFFER_LENGTH, "get wifi power mode fail for device %d",deviceId);
      QAT_Response_Str(QAT_RC_ERROR, buffer);
		return rc;
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
   snprintf(buffer, WLAN_RESPONSE_BUFFER_LENGTH, "Power mode  = %s",data);
   rc = QAT_Response_Str(QAT_RC_QUIET, buffer);

	return rc;
}

int32_t qat_get_device_mac_address()
{
   QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
   char buffer[WLAN_RESPONSE_BUFFER_LENGTH] = {0};
	uint8_t mac[__QAPI_WLAN_MAC_LEN] = {0};
	uint32_t length = __QAPI_WLAN_MAC_LEN;
	uint8_t deviceId = qat_get_active_device();	

   if(QAPI_OK != qapi_WLAN_Get_Param (deviceId, __QAPI_WLAN_PARAM_GROUP_WIRELESS,
         __QAPI_WLAN_PARAM_GROUP_WIRELESS_MAC_ADDRESS, &mac[0], &length)) {
      snprintf(buffer, WLAN_RESPONSE_BUFFER_LENGTH, "get mac address fail for device %d",deviceId);
		QAT_Response_Str(QAT_RC_ERROR, buffer);
		return rc;
	}

	snprintf(buffer, WLAN_RESPONSE_BUFFER_LENGTH, "Mac Addr    = %02x:%02x:%02x:%02x:%02x:%02x",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
   rc = QAT_Response_Str(QAT_RC_QUIET, buffer);

	return rc;
}

int32_t qat_get_rssi()
{
	qapi_Status_t ret = QAPI_ERROR;
   QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
   char buffer[WLAN_RESPONSE_BUFFER_LENGTH] = {0};
	uint8_t rssi = 0;
	uint32_t length = sizeof(rssi);
	uint32_t deviceId =qat_get_active_device();

    if(!pg_wifi_shell_cxt->wlan_enabled) {
        QAT_Response_Str(QAT_RC_ERROR, "wlan is not enabled");
        return rc;
    }

	ret = qapi_WLAN_Get_Param(deviceId,
							__QAPI_WLAN_PARAM_GROUP_WIRELESS,
							__QAPI_WLAN_PARAM_GROUP_WIRELESS_RSSI,
							&rssi,
							&length);
	if(QAPI_OK == ret) {
      snprintf(buffer, WLAN_RESPONSE_BUFFER_LENGTH, "rssi        = %d dB",rssi);
      rc = QAT_Response_Str(QAT_RC_QUIET, buffer);
   }

	return rc;
}

int32_t qat_get_op_mode()
{
   QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
   char buffer[WLAN_RESPONSE_BUFFER_LENGTH] = {0};
	qapi_WLAN_DEV_Mode_e conc_mode, opmode;
	uint32_t length = sizeof(qapi_WLAN_DEV_Mode_e);
	uint8_t deviceId = qat_get_active_device();

	if(QAPI_OK != qapi_WLAN_Get_Param (deviceId, 
								__QAPI_WLAN_PARAM_GROUP_WIRELESS,
								__QAPI_WLAN_PARAM_GROUP_WIRELESS_CONCURRENCY_MODE,
								&conc_mode,
								&length)) {
      QAT_Response_Str(QAT_RC_ERROR, "get concurrency mode fail");
		return rc;
	}
	
	if(conc_mode == DEV_MODE_AP_STA_E) {
      QAT_Response_Str(QAT_RC_QUIET, "mode       = concurrency mode");
      return rc;
	}
								
	if(QAPI_OK != qapi_WLAN_Get_Param (deviceId, 
								__QAPI_WLAN_PARAM_GROUP_WIRELESS,
								__QAPI_WLAN_PARAM_GROUP_WIRELESS_OPERATION_MODE,
								&opmode,
								&length)) {
      snprintf(buffer, WLAN_RESPONSE_BUFFER_LENGTH, "get operation mode fail for device %d",deviceId);
		QAT_Response_Str(QAT_RC_ERROR, buffer);
		return rc;
	}
	
	if(opmode == DEV_MODE_STATION_E) {
      rc = QAT_Response_Str(QAT_RC_QUIET, "mode        = station");
	}
	else if(opmode == DEV_MODE_AP_E) {
      rc = QAT_Response_Str(QAT_RC_QUIET, "mode        = softap");
	}
   else if(opmode == DEV_MODE_NO_CONC_E) {
      rc = QAT_Response_Str(QAT_RC_QUIET, "softap+station no support");
   }

	return rc;
}

/*-------------------------------------------------------------------------
 * Function Definitions
 *-----------------------------------------------------------------------*/

/**
   @brief Processes the Extend command from the QAT.

   This command will change the current group to its parent. No parameters are
   expected for this command.

   @param[in] Op_Type          The input command type.
   @param[in] Parameter_Count  Number of parameters that were entered into the
                               command line.
   @param[in] Parameter_List   List of parameters entered into the command line.
*/
static QAT_Command_Status_t Extend_Command_PyhMode(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List)
{
	qapi_Status_t ret= QAPI_OK;
   QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
	uint8_t deviceId =qat_get_active_device();
	qapi_WLAN_Phy_Mode_e phyMode = 0;
   char * wmode;

   wmode = (char *) Parameter_List[0].String_Value;
   switch(Op_Type) 
   {
      case QAT_OP_EXEC_W_PARAM:
      {

         if(!strcmp(wmode,"a")) {

            phyMode = QAPI_WLAN_11A_MODE_E;
         }
      
      else if(!strcmp(wmode,"b"))
         phyMode = QAPI_WLAN_11B_MODE_E;
      else if(!strcmp(wmode,"g"))
         phyMode = QAPI_WLAN_11G_MODE_E;
      else if(!strcmp(wmode,"ng"))
         phyMode = QAPI_WLAN_11NG_HT20_MODE_E;
      else if(!strcmp(wmode,"abgn"))
         phyMode = QAPI_WLAN_11ABGN_HT20_MODE_E;
      else {
         QAT_Response_Str(QAT_RC_ERROR, "Unknown wmode, only support a/b/g/ng/abgn/");
         return rc;
      }
      
      ret = qapi_WLAN_Set_Param(deviceId, 
                  __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                  __QAPI_WLAN_PARAM_GROUP_WIRELESS_PHY_MODE,
                  &phyMode,
                  sizeof(phyMode),
                  FALSE);
      if (ret != QAPI_OK) {
         QAT_Response_Str(QAT_RC_ERROR, NULL);
         return rc;
      }
      break;
      }
      case QAT_OP_EXEC:
      {
         QAT_Response_Str(QAT_RC_QUIET, "+CWPYHMODE=a/b/g/ng/abgn");
         break;
      }
      case QAT_OP_QUERY:
      {
         qat_get_phy_mode();
      }
      default:
      ;
   }
   rc = QAT_Response_Str(QAT_RC_OK, NULL);
   return rc;
}

/*-------------------------------------------------------------------------
 * Function Definitions
 *-----------------------------------------------------------------------*/

/**
   @brief Processes the Extend command from the QAT.

   This command will change the current group to its parent. No parameters are
   expected for this command.

   @param[in] Op_Type          The input command type.
   @param[in] Parameter_Count  Number of parameters that were entered into the
                               command line.
   @param[in] Parameter_List   List of parameters entered into the command line.
*/
static QAT_Command_Status_t Extend_Command_CountryCode(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List)
{
   qapi_Status_t ret = QAPI_OK;
   QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
	wifi_shell_cxt_t *p_cxt = pg_wifi_shell_cxt;
   char buffer[WLAN_RESPONSE_BUFFER_LENGTH] = {0};
	
    if (0 == p_cxt->wlan_enabled)
    {
        QAT_Response_Str(QAT_RC_ERROR, "Enable WLAN before set country code\r\n");
        return rc;
    }

	switch(Op_Type) 
   {

      case QAT_OP_EXEC_W_PARAM:
      {
	      if( Parameter_Count != 1 || !Parameter_List || Parameter_List[0].Integer_Is_Valid) {
            QAT_Response_Str(QAT_RC_ERROR, NULL);
		      return rc;
         }

	      ret = set_country_code((char *) Parameter_List[0].String_Value);
	      if(ret != QAPI_OK) {
		      snprintf(buffer, WLAN_RESPONSE_BUFFER_LENGTH, "set country code %s fail\n", (char *) Parameter_List[0].String_Value);
            QAT_Response_Str(QAT_RC_ERROR, NULL);
            return rc;
	      }
         break;
      }
      case QAT_OP_EXEC:
      {
         QAT_Response_Str(QAT_RC_QUIET, "+CWCOUNTRY=<countrycode>, e.g. US/CN");
         break;
      }
      case QAT_OP_QUERY:
      {
         qat_get_country_code();
      }
      default:
      ;
   }

   rc = QAT_Response_Str(QAT_RC_OK, NULL);
	return rc;
}

/**
   @brief Processes the Extend command from the QAT.

   This command will change the current group to its parent. No parameters are
   expected for this command.

   @param[in] Op_Type          The input command type.
   @param[in] Parameter_Count  Number of parameters that were entered into the
                               command line.
   @param[in] Parameter_List   List of parameters entered into the command line.
*/
static QAT_Command_Status_t Extend_Command_SetModeOption(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List)
{
   QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;

   if(pg_wifi_shell_cxt->wlan_enabled == 0) {
      QAT_Response_Str(QAT_RC_ERROR, "wlan is not enabled");
      return rc;
   }

   switch (Op_Type)
   {
      case QAT_OP_EXEC_W_PARAM:    /* AT+CWSOFTAP */
      {
         if(Parameter_Count < 2 || !Parameter_List) {
            QAT_Response_Str(QAT_RC_ERROR, "parameter error");
		      return rc;
	      }

         rc = (QAT_Command_Status_t)qat_set_11nht_cap((char *) Parameter_List[0].String_Value);
         if (rc != QAT_STATUS_SUCCESS_E) {
            QAT_Response_Str(QAT_RC_ERROR, "set 11nht error");
            return rc;
         }

         rc = (QAT_Command_Status_t)qat_set_channel(Parameter_List[1].Integer_Value);
         if (rc != QAT_STATUS_SUCCESS_E) {
            QAT_Response_Str(QAT_RC_ERROR, "set channel error");
            return rc;
         }

         break;
      }
      case QAT_OP_EXEC:
      {
         QAT_Response_Str(QAT_RC_QUIET, "+CWSOFTAP=<param1>,<param2>(param1:disable/ht20, param2:1-14, 36-165)");
         break;
      }
      default:
      ;
   }
   rc = QAT_Response_Str(QAT_RC_OK, NULL);
   return rc;
}

/**
   @brief Processes the Extend command from the QAT.

   This command will change the current group to its parent. No parameters are
   expected for this command.

   @param[in] Op_Type          The input command type.
   @param[in] Parameter_Count  Number of parameters that were entered into the
                               command line.
   @param[in] Parameter_List   List of parameters entered into the command line.
*/
static QAT_Command_Status_t Extend_Command_SetOperatingMode(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List)
{
   QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
   char *hidden = "";

   if(pg_wifi_shell_cxt->wlan_enabled == 0) {
      QAT_Response_Str(QAT_RC_ERROR, "wlan is not enabled");
      return rc;
   }
	
   switch (Op_Type)
   {
      case QAT_OP_EXEC_W_PARAM:    /* AT+CWMODE */
      {
         if(Parameter_Count < 1 || !Parameter_List) {
            QAT_Response_Str(QAT_RC_ERROR, "wlan is not enabled");
		      return rc;
	      }
	
	      if(Parameter_Count >= 2) {
            hidden = (char *) Parameter_List[1].String_Value;
         }
     
	      rc = (QAT_Command_Status_t)qat_set_op_mode((char *)Parameter_List[0].String_Value, hidden);
         if (rc != QAT_STATUS_SUCCESS_E) {
            QAT_Response_Str(QAT_RC_ERROR, "set op mode error");
            return rc;
         }

         break;
      }

      case QAT_OP_QUERY:
      {
         qat_get_op_mode();
         break;
      }
      case QAT_OP_EXEC:
      {
         QAT_Response_Str(QAT_RC_QUIET, "+CWMODE=station/ap");
         break;
      }
      default:
      ;
   }
   rc = QAT_Response_Str(QAT_RC_OK, NULL);
   return rc;
}

/**
   @brief Processes the Extend command from the QAT.

   This command will change the current group to its parent. No parameters are
   expected for this command.

   @param[in] Op_Type          The input command type.
   @param[in] Parameter_Count  Number of parameters that were entered into the
                               command line.
   @param[in] Parameter_List   List of parameters entered into the command line.
*/
static QAT_Command_Status_t Extend_Command_Connect(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List)
{
   QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
   char *bssid = NULL;
   int ssidLength = 0;
   char *ssid = NULL;
	uint8_t deviceId = qat_get_active_device();
	wifi_shell_cxt_t *p_cxt = pg_wifi_shell_cxt;
   char buffer[WLAN_RESPONSE_BUFFER_LENGTH] = {0};
   qapi_Status_t ret = QAPI_OK;

   switch (Op_Type)
   {
      case QAT_OP_QUERY: /* AT+CWJAP? */
      {
         wifi_shell_cxt_t *p_cxt = pg_wifi_shell_cxt;
         if (0 == p_cxt->wlan_enabled)
         {
            QAT_Response_Str(QAT_RC_ERROR, "Enable WLAN before get the WLAN infomation");
            return rc;
         }
	
         if(p_cxt->connected == true)
         {
            snprintf(buffer, WLAN_RESPONSE_BUFFER_LENGTH, "ssid        = %s\r\n\r\nchannel     = %d", p_cxt->ssid, p_cxt->channel_frequency);
            QAT_Response_Str(QAT_RC_QUIET, buffer);
         }
 
         qat_get_phy_mode();
         qat_get_wifi_power_mode();
         qat_get_device_mac_address();
	      qat_get_op_mode();
         qat_get_rssi();

         break;
      }

      case QAT_OP_EXEC:
      {
         QAT_Response_Str(QAT_RC_QUIET, "+CWJAP=<ssid>");
         break;
      }

      case QAT_OP_EXEC_W_PARAM:    /* AT+CWJAP */
      {
         if(!p_cxt->wlan_enabled) {
            QAT_Response_Str(QAT_RC_ERROR, "wlan is not enabled ");
            return rc;
         }

         if( Parameter_Count < 1 || !Parameter_List ) {
            QAT_Response_Str(QAT_RC_ERROR, NULL);
            return rc;
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
               QAT_Response_Str(QAT_RC_ERROR, "Invalid BSSID to connect");
               return rc;
            }
            qapi_WLAN_Set_Param (0, __QAPI_WLAN_PARAM_GROUP_WIRELESS,
               __QAPI_WLAN_PARAM_GROUP_WIRELESS_BSSID,
               (void *)bssidToConnect, __QAPI_WLAN_MAC_LEN, FALSE);
         }

         snprintf(buffer, WLAN_RESPONSE_BUFFER_LENGTH, "connecting to ssid %s", ssid);
         QAT_Response_Str(QAT_RC_QUIET, buffer);
         ret = qapi_WLAN_Commit(deviceId);
         if (ret != QAPI_OK) {
            QAT_Response_Str(QAT_RC_ERROR, NULL);
            return rc;
         }

	      if(deviceId == NT_DEV_AP_ID && ret == QAPI_OK) {
		      memscpy(p_cxt->ssid, ssidLength, ssid, ssidLength);
            p_cxt->ssid[ssidLength] = 0;
            p_cxt->ssid_length = ssidLength;
	      }
         break;
      }
      default:
      ;
   }

   rc = QAT_Response_Str(QAT_RC_OK, NULL);
   return rc;
}

/**
   @brief Processes the Extend command from the QAT.

   This command will change the current group to its parent. No parameters are
   expected for this command.

   @param[in] Op_Type          The input command type.
   @param[in] Parameter_Count  Number of parameters that were entered into the
                               command line.
   @param[in] Parameter_List   List of parameters entered into the command line.
*/
static QAT_Command_Status_t Extend_Command_Disconnect(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List)
{
   QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
   
   switch (Op_Type)
   {
      case QAT_OP_EXEC:     /* AT+CWQAP */
      {
	      uint8_t deviceId = qat_get_active_device();

         if(!pg_wifi_shell_cxt->wlan_enabled) {
            QAT_Response_Str(QAT_RC_ERROR, "wlan is not enabled");
            return rc;
         }

         pg_wifi_shell_cxt->auth = QAPI_WLAN_AUTH_NONE_E;
         qapi_WLAN_Disconnect(deviceId);
         break;
      } 
      default:
      ;
   }

   rc = QAT_Response_Str(QAT_RC_OK, NULL);
   return rc;
}

void Initialize_QAT_Wlan_Demo (void)
{
	qbool_t RetVal;

   pg_wifi_shell_cxt = &g_wifi_shell_cxt;
   wifi_shell_cxt_t *p_cxt = pg_wifi_shell_cxt;
   memset(&g_wifi_shell_cxt, 0, sizeof(wifi_shell_cxt_t));
   qurt_mutex_create(&p_cxt->wifi_shell_cxt_mutex);
   pg_wifi_shell_cxt->auth = QAPI_WLAN_AUTH_NONE_E;

	RetVal = QAT_Register_Command_Group(QAT_Wifi_Command_List, WIFI_COMMAND_LIST_SIZE);
	if(RetVal == false)
   {
      QAT_Response_Str(QAT_RC_ERROR, "Failed to register common command group.");
   }
}


