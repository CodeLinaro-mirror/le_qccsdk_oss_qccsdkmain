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
#include "fs.h"
#include "safeAPI.h"

#ifndef NT_DEV_AP_ID
#define NT_DEV_AP_ID 0
#endif
#ifndef NT_DEV_STA_ID
#define NT_DEV_STA_ID 1
#endif
#ifndef NT_DEFAULT_HAL_STA_ID
#define NT_DEFAULT_HAL_STA_ID 2
#endif
#ifndef NT_DEV_INV_ID
#define NT_DEV_INV_ID 3
#endif
/*-------------------------------------------------------------------------
 * Function Declarations
 *-----------------------------------------------------------------------*/
static QAT_Command_Status_t Extend_Command_Wifisp(uint32_t Op_Type, uint32_t Parameter_Count,
                                                  QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_Enable(uint32_t Op_Type, uint32_t Parameter_Count,
                                                  QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_Disable(uint32_t Op_Type, uint32_t Parameter_Count,
                                                   QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_SetOperatingMode(uint32_t Op_Type, uint32_t Parameter_Count,
                                                            QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_Scan(uint32_t Op_Type, uint32_t Parameter_Count,
                                                QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_SetWpaPassphrase(uint32_t Op_Type, uint32_t Parameter_Count,
                                                            QAPI_Console_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_SetWpaParameters(uint32_t Op_Type, uint32_t Parameter_Count,
                                                            QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_Connect(uint32_t Op_Type, uint32_t Parameter_Count,
                                                   QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_Disconnect(uint32_t Op_Type, uint32_t Parameter_Count,
                                                      QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_SetModeOption(uint32_t Op_Type, uint32_t Parameter_Count,
                                                         QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_EventMessage(uint32_t Op_Type, uint32_t Parameter_Count,
                                                        QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_PyhMode(uint32_t Op_Type, uint32_t Parameter_Count,
                                                   QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_CountryCode(uint32_t Op_Type, uint32_t Parameter_Count,
                                                       QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_ANTIINF(uint32_t Op_Type, uint32_t Parameter_Count,
                                                   QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_EDCA(uint32_t Op_Type, uint32_t Parameter_Count,
                                                QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_EDCCATHR(uint32_t Op_Type, uint32_t Parameter_Count,
                                                    QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_BMISSTHR(uint32_t Op_Type, uint32_t Parameter_Count,
                                                    QAT_Parameter_t *Parameter_List);
#ifdef CONFIG_WPS
static QAT_Command_Status_t Extend_Command_WPS(uint32_t Op_Type, uint32_t Parameter_Count,
                                               QAT_Parameter_t *Parameter_List);
#endif
static QAT_Command_Status_t Extend_Command_CWSave(uint32_t Op_Type, uint32_t Parameter_Count,
                                                  QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_CWLoad(uint32_t Op_Type, uint32_t Parameter_Count,
                                                  QAT_Parameter_t *Parameter_List);

/*-------------------------------------------------------------------------
 * WiFi credential persistence (AT+CWSAVE / AT+CWLOAD)
 *-----------------------------------------------------------------------*/
#define WIFI_CONF_PATH    "/lfs/wifi.conf"
#define WIFI_CONF_MAGIC   "WCFG"
#define WIFI_CONF_VERSION 1

typedef struct {
    uint8_t  magic[4];
    uint8_t  version;
    uint8_t  ssid_len;
    uint8_t  passphrase_len;
    uint8_t  _pad;
    uint32_t auth_mode;
    uint32_t cipher;
    char     ssid[__QAPI_WLAN_MAX_SSID_LEN + 1];
    char     passphrase[65];
} wifi_cred_t;
/* The following is the complete command list for the QAT common command demo. */
/** List of global commands that are supported when in a group. */
static QAT_Command_t QAT_Wifi_Command_List[] = {
    {"+WIFISP", Extend_Command_Wifisp, QAT_OP_EXEC},
    {"+CWENABLE", Extend_Command_Enable, QAT_OP_EXEC},
    {"+CWQABLE", Extend_Command_Disable, QAT_OP_EXEC},
    {"+CWMODE", Extend_Command_SetOperatingMode, QAT_OP_EXEC_W_PARAM | QAT_OP_QUERY | QAT_OP_EXEC},
    {"+CWLAP", Extend_Command_Scan, QAT_OP_EXEC_W_PARAM | QAT_OP_EXEC},
    {"+CWWPA", Extend_Command_SetWpaParameters, QAT_OP_EXEC_W_PARAM | QAT_OP_EXEC},
    {"+CWPWD", Extend_Command_SetWpaPassphrase, QAT_OP_EXEC_W_PARAM | QAT_OP_EXEC},
    {"+CWJAP", Extend_Command_Connect, QAT_OP_EXEC_W_PARAM | QAT_OP_QUERY | QAT_OP_EXEC},
    {"+CWQAP", Extend_Command_Disconnect, QAT_OP_EXEC},
    {"+CWSOFTAP", Extend_Command_SetModeOption, QAT_OP_EXEC_W_PARAM | QAT_OP_EXEC},
    {"+WEVT", Extend_Command_EventMessage, QAT_OP_EXEC_W_PARAM | QAT_OP_EXEC | QAT_OP_QUERY},
    {"+CWPHYMODE", Extend_Command_PyhMode, QAT_OP_EXEC_W_PARAM | QAT_OP_EXEC | QAT_OP_QUERY},
    {"+CWCOUNTRY", Extend_Command_CountryCode, QAT_OP_EXEC_W_PARAM | QAT_OP_EXEC | QAT_OP_QUERY},
    {"+ANTIINF", Extend_Command_ANTIINF, QAT_OP_QUERY | QAT_OP_EXEC_W_PARAM | QAT_OP_EXEC},
    {"+EDCA", Extend_Command_EDCA, QAT_OP_QUERY | QAT_OP_EXEC_W_PARAM | QAT_OP_EXEC},
    {"+EDCCATHR", Extend_Command_EDCCATHR, QAT_OP_QUERY | QAT_OP_EXEC_W_PARAM | QAT_OP_EXEC},
    {"+BMISSTHR", Extend_Command_BMISSTHR, QAT_OP_QUERY | QAT_OP_EXEC_W_PARAM | QAT_OP_EXEC},
#ifdef CONFIG_WPS
    {"+WPS", Extend_Command_WPS, QAT_OP_EXEC_W_PARAM},
#endif
    {"+CWSAVE", Extend_Command_CWSave, QAT_OP_EXEC},
    {"+CWLOAD", Extend_Command_CWLoad, QAT_OP_EXEC},
};

typedef struct wifi_shell_cxt_s {
    qurt_mutex_t wifi_shell_cxt_mutex;
    int32_t scan_mode;
    qapi_WLAN_Auth_Mode_e auth;
    qapi_WLAN_Phy_Mode_e phy_mode;
    qapi_WLAN_11n_HT_Config_e htcfg;
    qbool_t connected;
    char ssid[__QAPI_WLAN_MAX_SSID_LEN + 1];
    int32_t ssid_length;
    uint16_t channel_frequency;
    uint8_t active_device;
    uint8_t wlan_enabled;
#ifdef CONFIG_WPS
    uint8_t wps_stage;
#endif
    char     passphrase[65];   /* shadow for AT+CWSAVE */
    uint32_t cipher;           /* shadow for AT+CWSAVE (qapi_WLAN_Crypt_Type_e) */
} wifi_shell_cxt_t;

#ifdef CONFIG_WPS
typedef struct {
    uint8_t wps_in_progress;
    uint8_t connect_flag;
    uint8_t wps_pbc_interrupt;
    qapi_WLAN_Netparams_t netparams;
} wps_context_t;

typedef enum { WPS_NONE, WPS_SCAN, WPS_CONNECTED } WPS_STAGE_TYPE;
#endif
/*-------------------------------------------------------------------------
 * Parameters define
 *-----------------------------------------------------------------------*/

#define WIFI_COMMAND_LIST_SIZE (sizeof(QAT_Wifi_Command_List) / sizeof(QAT_Command_t))

#define WLAN_RESPONSE_BUFFER_LENGTH 128
#define WLAN_STR_BUFFER_LENGTH      1500
#define SCAN_MODE_BLOCKING          1
#define SCAN_MODE_UNBLOCKING        2
#define CMD_STR_BUFFER_LENGTH       1024

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

    char buffer[WLAN_STR_BUFFER_LENGTH] = {0};
    for (i = 0; i < num_scan; i++) {
        int offset = 0;

        memscpy(temp_ssid, list[i].ssid_Length, list[i].ssid, list[i].ssid_Length);
        temp_ssid[list[i].ssid_Length] = '\0';
        if (list[i].ssid_Length == 0) {
            QAT_Response_Str(QAT_RC_QUIET, "+CWLAP:ssid = SSID Not available");
        } else {
            offset += snprintf(buffer + offset, WLAN_STR_BUFFER_LENGTH - offset,
                               "+CWLAP:%s,%.2x-%.2x-%.2x-%.2x-%.2x-%.2x,%d,%d,", temp_ssid, list[i].bssid[0],
                               list[i].bssid[1], list[i].bssid[2], list[i].bssid[3], list[i].bssid[4], list[i].bssid[5],
                               list[i].channel, list[i].rssi);
            if (list[i].security_Enabled) {
                if ((list[i].rsn_Auth || list[i].rsn_Cipher)) {
                    if ((list[i].rsn_Auth & __QAPI_WLAN_SECURITY_AUTH_1X) ||
                        (list[i].rsn_Auth & __QAPI_WLAN_SECURITY_AUTH_PSK)) {
                        if (list[i].wpa_Auth || list[i].wpa_Cipher)
                            offset += snprintf(buffer + offset, WLAN_STR_BUFFER_LENGTH - offset, "WPA/WPA2,");
                        else if (list[i].rsn_Auth & __QAPI_WLAN_SECURITY_AUTH_SAE)
                            offset += snprintf(buffer + offset, WLAN_STR_BUFFER_LENGTH - offset, "WPA2/WPA3,");
                        else
                            offset += snprintf(buffer + offset, WLAN_STR_BUFFER_LENGTH - offset, "WPA2,");
                    } else if (list[i].rsn_Auth & __QAPI_WLAN_SECURITY_AUTH_SAE) {
                        offset += snprintf(buffer + offset, WLAN_STR_BUFFER_LENGTH - offset, "WPA3,");
                    }
                    if (list[i].rsn_Cipher) {
                        if (list[i].rsn_Cipher & __QAPI_WLAN_CIPHER_TYPE_WEP) {
                            offset += snprintf(buffer + offset, WLAN_STR_BUFFER_LENGTH - offset, "WEP,");
                        }
                        if (list[i].rsn_Cipher & __QAPI_WLAN_CIPHER_TYPE_TKIP) {
                            if (list[i].rsn_Cipher & __QAPI_WLAN_CIPHER_TYPE_CCMP)
                                offset += snprintf(buffer + offset, WLAN_STR_BUFFER_LENGTH - offset, "TKIP/CCMP");
                            else
                                offset += snprintf(buffer + offset, WLAN_STR_BUFFER_LENGTH - offset, "TKIP");
                        } else if (list[i].rsn_Cipher & __QAPI_WLAN_CIPHER_TYPE_CCMP) {
                            offset += snprintf(buffer + offset, WLAN_STR_BUFFER_LENGTH - offset, "CCMP");
                        }
                    }
                } else if (list[i].wpa_Auth || list[i].wpa_Cipher) {
                    offset += snprintf(buffer + offset, WLAN_STR_BUFFER_LENGTH - offset, "WPA,");

                    if (list[i].wpa_Cipher) {
                        if (list[i].wpa_Cipher & __QAPI_WLAN_CIPHER_TYPE_WEP) {
                            offset += snprintf(buffer + offset, WLAN_STR_BUFFER_LENGTH - offset, "WEP,");
                        }
                        if (list[i].wpa_Cipher & __QAPI_WLAN_CIPHER_TYPE_TKIP) {
                            if (list[i].wpa_Cipher & __QAPI_WLAN_CIPHER_TYPE_CCMP)
                                offset += snprintf(buffer + offset, WLAN_STR_BUFFER_LENGTH - offset, "TKIP/CCMP");
                            else
                                offset += snprintf(buffer + offset, WLAN_STR_BUFFER_LENGTH - offset, "TKIP");
                        }
                        if (list[i].wpa_Cipher & __QAPI_WLAN_CIPHER_TYPE_CCMP) {
                            offset += snprintf(buffer + offset, WLAN_STR_BUFFER_LENGTH - offset, "CCMP");
                        }
                    }
                }
            } else {
                offset += snprintf(buffer + offset, WLAN_STR_BUFFER_LENGTH - offset, "NONE,NONE");
            }
            QAT_Response_Str(QAT_RC_QUIET, buffer);
            sys_msleep(10);
        }
    }

    return;
}

static void wlan_shell_event_handler(__unused uint8_t deviceId, uint32_t cbId, void __unused *pApplicationContext,
                                     void *payload, uint32_t payload_Length)
{
    wifi_shell_cxt_t *p_cxt = pg_wifi_shell_cxt;
    char buffer[WLAN_RESPONSE_BUFFER_LENGTH] = {0};
    int offset = 0;

    switch (cbId) {
        case QAPI_WLAN_SCAN_COMPLETE_CB_E: {
            if (!payload || !payload_Length) {
                QAT_Response_Str(QAT_RC_QUIET, "+EVT:wlan_scancmplt: error");
                break;
            }

            qapi_WLAN_Scan_Comp_Evt_t *p_scan_compl_evt = (qapi_WLAN_Scan_Comp_Evt_t *)payload;
            offset += snprintf(buffer + offset, WLAN_RESPONSE_BUFFER_LENGTH - offset, "+EVT:wlan_scancmplt");
            if (p_cxt->scan_mode == SCAN_MODE_UNBLOCKING) {
                scan_results(p_scan_compl_evt);
            }
            break;
        }
        case QAPI_WLAN_CONNECT_CB_E: {
            qapi_WLAN_Join_Comp_Evt_t *cxnInfo = (qapi_WLAN_Join_Comp_Evt_t *)(payload);
            uint8_t *mac = cxnInfo->bssid;
            if (cxnInfo->ssid_Length) {
                memscpy(p_cxt->ssid, cxnInfo->ssid_Length, cxnInfo->ssid, cxnInfo->ssid_Length);
                p_cxt->ssid[cxnInfo->ssid_Length] = 0;
                p_cxt->ssid_length = cxnInfo->ssid_Length;
            }
            p_cxt->channel_frequency = cxnInfo->channel_frequency;
            if (cxnInfo->evt_hdr.status == QAPI_OK) {
                qapi_WLAN_Auth_Mode_e e_wpa_ver = p_cxt->auth;
                if (cxnInfo->bss_Connection_Status)
                    p_cxt->connected = true;
                offset += snprintf(buffer + offset, WLAN_RESPONSE_BUFFER_LENGTH - offset,
                                   "+EVT:wlan_conned:%d,%02x-%02x-%02x-%02x-%02x-%02x,", p_cxt->active_device, mac[0],
                                   mac[1], mac[2], mac[3], mac[4], mac[5]);
#ifdef CONFIG_WPS
                p_cxt->wps_stage = WPS_CONNECTED;
#endif
            } else {
                offset += snprintf(buffer + offset, WLAN_RESPONSE_BUFFER_LENGTH - offset,
                                   "+EVT:wlan_disconn:%d,%d,%02x-%02x-%02x-%02x-%02x-%02x,", cxnInfo->reason_code,
                                   cxnInfo->bss_Connection_Status, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            }
            offset += snprintf(buffer + offset, WLAN_RESPONSE_BUFFER_LENGTH - offset, "%d,%d,%d,%s,%s",
                               cxnInfo->channel_frequency, cxnInfo->assoc_id, cxnInfo->host_initiated, p_cxt->ssid,
                               cxnInfo->passphrase);
            break;
        }
        case QAPI_WLAN_DISCONNECT_CB_E: {
            qapi_WLAN_Join_Comp_Evt_t *cxnInfo = (qapi_WLAN_Join_Comp_Evt_t *)(payload);
            if (cxnInfo->bss_Connection_Status) {
                p_cxt->connected = false;
            }

            if (p_cxt->ssid_length) {
                snprintf(buffer, WLAN_RESPONSE_BUFFER_LENGTH, "+EVT:wlan_disconcmd:%d,%s", p_cxt->active_device,
                         p_cxt->ssid);
#ifdef CONFIG_WPS
                p_cxt->wps_stage = WPS_NONE;
#endif
            }
            break;
        }
        case QAPI_WLAN_CHANNEL_SWITCH_CB_E: {
            qapi_WLAN_Chan_Switch_Evt_t *ecsa = (qapi_WLAN_Chan_Switch_Evt_t *)payload;
            if (ecsa->evt_hdr.status == QAPI_OK) {
                p_cxt->channel_frequency = ecsa->freq;
                snprintf(buffer, WLAN_RESPONSE_BUFFER_LENGTH, "+EVT:wlan_switchchan:%d,%d", p_cxt->active_device,
                         ecsa->freq);
            } else {
                snprintf(buffer, WLAN_RESPONSE_BUFFER_LENGTH, "+EVT:wlan_switchchan:FAIL,%d,%d", p_cxt->active_device,
                         ecsa->reason);
            }
            break;
        }
        case QAPI_WLAN_ENABLE_CB_E: {
            QAT_Response_Str(QAT_RC_QUIET_NO_CR, "+EVT:wlan_enable");
            return;
        }
        case QAPI_WLAN_SCAN_START_CB_E: {
            QAT_Response_Str(QAT_RC_QUIET, "+EVT:wlan_scanstart");
            return;
        }
        case QAPI_WLAN_DISABLE_CB_E: {
            QAT_Response_Str(QAT_RC_QUIET_NO_CR, "+EVT:wlan_disabled\r\n");
            return;
        }
#ifdef CONFIG_WPS
        case QAPI_WLAN_WPS_FAIL_CB_E: {
            int reason = *(int *)payload;
            p_cxt->wps_stage = WPS_NONE;
            offset +=
                snprintf(buffer + offset, WLAN_RESPONSE_BUFFER_LENGTH - offset, "+EVT:wlan_wps_failed:%d,", reason);
            break;
        }
#endif
    }

    if (!enable_event_reporting) {
        QAT_Response_Str(QAT_RC_QUIET, "Event reporting has been disabled");
    } else {
        QAT_Response_Str(QAT_RC_QUIET, buffer);
    }

    return;
}

static QAT_Command_Status_t Extend_Command_EventMessage(uint32_t Op_Type, uint32_t Parameter_Count,
                                                        QAT_Parameter_t *Parameter_List)
{
    char buffer[WLAN_RESPONSE_BUFFER_LENGTH] = {0};
    QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;

    switch (Op_Type) {
        case QAT_OP_QUERY: /* AT+WEVT */
        {
            snprintf(buffer, WLAN_RESPONSE_BUFFER_LENGTH, "+WEVT:%d", enable_event_reporting);
            QAT_Response_Str(QAT_RC_QUIET, buffer);
            break;
        }

        case QAT_OP_EXEC_W_PARAM: {
            enable_event_reporting = Parameter_List[0].Integer_Value;
            break;
        }

        case QAT_OP_EXEC: {
            QAT_Response_Str(QAT_RC_QUIET, "+WEVT=0/1");
            break;
        }

        default:;
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
static QAT_Command_Status_t Extend_Command_Wifisp(uint32_t Op_Type, uint32_t Parameter_Count,
                                                  QAT_Parameter_t *Parameter_List)
{
    QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
    wifi_shell_cxt_t *p_cxt = pg_wifi_shell_cxt;

    if (p_cxt->wlan_enabled) {
        rc = QAT_Response_Str(QAT_RC_OK, "+WIFISP: Supported");
        return rc;
    }

    switch (Op_Type) {
        case QAT_OP_EXEC: /* AT+WIFISP */
        {
            if (qapi_WLAN_Enable(true) != QAPI_OK) {
                QAT_Response_Str(QAT_RC_ERROR, "+WIFISP:get wlan mode fail");
                return rc;
            }
            break;
        }
        default:;
    }
    qapi_WLAN_Enable(false);
    rc = QAT_Response_Str(QAT_RC_OK, "+WIFISP: Supported");
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
static QAT_Command_Status_t Extend_Command_Enable(uint32_t Op_Type, uint32_t Parameter_Count,
                                                  QAT_Parameter_t *Parameter_List)
{
    qapi_Status_t ret;
    qapi_WLAN_DEV_Mode_e devMode = DEV_MODE_STATION_E;
    wifi_shell_cxt_t *p_cxt = pg_wifi_shell_cxt;
    QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;

    switch (Op_Type) {
        case QAT_OP_EXEC: /* AT+ CWENABLE */
        {
            if (p_cxt->wlan_enabled) {
                rc = QAT_Response_Str(QAT_RC_OK, NULL);
                return rc;
            }

            qapi_WLAN_Set_Callback(wlan_shell_event_handler, &g_wifi_shell_cxt);
            ret = qapi_WLAN_Enable(true);
            if (QAPI_OK != ret) {
                QAT_Response_Str(QAT_RC_ERROR, "+CWENABLE:cmd failed");
                return rc;
            }
            p_cxt->wlan_enabled = 1;

            // TODO To maintain consistency of Auto test tool, the default mode set to station
            ret =
                qapi_WLAN_Set_Param(0, __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                                    __QAPI_WLAN_PARAM_GROUP_WIRELESS_OPERATION_MODE, &devMode, sizeof(devMode), FALSE);
            if (ret != QAPI_OK) {
                QAT_Response_Str(QAT_RC_ERROR, "+CWENABLE:set mode station fail");
                return rc;
            } else {
                p_cxt->active_device = NT_DEV_STA_ID;
            }
            break;
        }
        default:;
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
static QAT_Command_Status_t Extend_Command_Disable(uint32_t Op_Type, uint32_t Parameter_Count,
                                                   QAT_Parameter_t *Parameter_List)
{
    wifi_shell_cxt_t *p_cxt = pg_wifi_shell_cxt;
    qapi_Status_t ret = QAPI_ERROR;
    QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;

    if (!p_cxt->wlan_enabled) {
        rc = QAT_Response_Str(QAT_RC_OK, "+CWQABLE:Wlan is disabled");
        return rc;
    }

    switch (Op_Type) {
        case QAT_OP_EXEC: /* AT+CWQABLE */
        {
            ret = qapi_WLAN_Enable(false);
            if (QAPI_OK != ret) {
                QAT_Response_Str(QAT_RC_ERROR, "+CWQABLE:cmd failed");
                return rc;
            }
            p_cxt->wlan_enabled = 0;
            break;
        }
        default:;
    }
    rc = QAT_Response_Str(QAT_RC_OK, NULL);
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

    if (!strcmp(opmode, "ap")) {
        devMode = DEV_MODE_AP_E;
        if (strcmp(hiddenSsid, "hidden") == 0) {
            hidden_flag = 1;
        } else if (strcmp(hiddenSsid, "0") == 0 || strcmp(hiddenSsid, "") == 0) {
            hidden_flag = 0;
        } else {
            QAT_Response_Str(QAT_RC_ERROR, "+CWMODE:error input");
            return rc;
        }
    } else if (!strcmp(opmode, "sta")) {
        devMode = DEV_MODE_STATION_E;
    }
#ifdef NT_FN_CONCURRENCY
    else if (!strcmp(opmode, "ap_sta")) {
        devMode = DEV_MODE_AP_STA_E;
    }
#endif
    else if (!strcmp(opmode, "no_ap_sta")) {
        devMode = DEV_MODE_NO_CONC_E;
    } else {
        snprintf(buffer, WLAN_RESPONSE_BUFFER_LENGTH, "+CWMODE:unknown mode, %s", opmode);
        QAT_Response_Str(QAT_RC_QUIET, buffer);
        return rc;
    }

    ret = qapi_WLAN_Set_Param(0, __QAPI_WLAN_PARAM_GROUP_WIRELESS, __QAPI_WLAN_PARAM_GROUP_WIRELESS_OPERATION_MODE,
                              &devMode, sizeof(devMode), FALSE);

    if (ret != QAPI_OK) {
        snprintf(buffer, WLAN_RESPONSE_BUFFER_LENGTH, "+CWMODE:set mode %s fail", opmode);
        QAT_Response_Str(QAT_RC_QUIET, buffer);
        return rc;
    } else {
        if (devMode == DEV_MODE_AP_E)
            p_cxt->active_device = NT_DEV_AP_ID;
        else if (devMode == DEV_MODE_STATION_E)
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
static QAT_Command_Status_t Extend_Command_Scan(uint32_t Op_Type, uint32_t Parameter_Count,
                                                QAT_Parameter_t *Parameter_List)
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

    if (0 == p_cxt->wlan_enabled) {
        QAT_Response_Str(QAT_RC_ERROR, "+CWLAP:Enable WLAN before scan");
        return rc;
    }

    switch (Op_Type) {
        case QAT_OP_EXEC_W_PARAM: {
            if (Parameter_Count >= 1 && !Parameter_List[1].Integer_Is_Valid) {
                uint8_t ssid_Length = strlen((char *)Parameter_List[0].String_Value);
                if (ssid_Length > __QAPI_WLAN_MAX_SSID_LEN) {
                    QAT_Response_Str(QAT_RC_ERROR, "+CWLAP:SSID length exceeds Maximum value");
                    qurt_mutex_unlock(&p_cxt->wifi_shell_cxt_mutex);
                    return rc;
                }
                scan_param.ssid_Length = ssid_Length;
                memscpy(scan_param.ssid, ssid_Length, Parameter_List[0].String_Value, ssid_Length);
                scan_ssid = true;
            }
        }

        case QAT_OP_EXEC: /* AT+CWLAP */
        {
            qurt_mutex_lock(&p_cxt->wifi_shell_cxt_mutex);
            p_cxt->scan_mode = SCAN_MODE_BLOCKING;
            if (QAT_STATUS_SUCCESS_E != qapi_WLAN_Get_Param(deviceId, __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                                                            __QAPI_WLAN_PARAM_GROUP_WIRELESS_OPERATION_MODE, &opmode,
                                                            &length)) {
                snprintf(buffer, WLAN_RESPONSE_BUFFER_LENGTH, "+CWLAP:get operation mode fail");
                QAT_Response_Str(QAT_RC_ERROR, buffer);
                return rc;
            }
            if (opmode != DEV_MODE_STATION_E) {
                snprintf(buffer, WLAN_RESPONSE_BUFFER_LENGTH, "+CWLAP:FAIL, %d", opmode);
                QAT_Response_Str(QAT_RC_ERROR, buffer);
                return rc;
            }

            qurt_mutex_unlock(&p_cxt->wifi_shell_cxt_mutex);

            if (scan_ssid) {
                ret = qapi_WLAN_Start_Scan(deviceId, &scan_param);
            } else {
                ret = qapi_WLAN_Start_Scan(deviceId, NULL);
            }

            if ((ret == QAPI_OK) && (SCAN_MODE_BLOCKING == p_cxt->scan_mode)) {
                qapi_WLAN_Scan_Comp_Evt_t scan_complete_evt = {0};
                int16_t bss_cnt = 0;

                qapi_WLAN_Get_Scan_Results(deviceId, &scan_complete_evt, &bss_cnt);
                bss_cnt = scan_complete_evt.num_bss_cur;
                qapi_WLAN_Scan_Comp_Evt_t *scan_complete_evt_total =
                    malloc(sizeof(qapi_WLAN_Scan_Comp_Evt_t) + bss_cnt * sizeof(qapi_WLAN_BSS_Scan_Info_t));
                qapi_WLAN_Get_Scan_Results(deviceId, scan_complete_evt_total, &bss_cnt);
                qurt_mutex_lock(&p_cxt->wifi_shell_cxt_mutex);
                if (scan_complete_evt_total) {
                    snprintf(buffer, WLAN_RESPONSE_BUFFER_LENGTH, "+EVT:wlan_scanresultstart:%d,%d", bss_cnt,
                             p_cxt->scan_mode);
                    QAT_Response_Str(QAT_RC_QUIET, buffer);
                    scan_results(scan_complete_evt_total);
                    snprintf(buffer, WLAN_RESPONSE_BUFFER_LENGTH, "+EVT:wlan_scanresultend");
                    qurt_mutex_unlock(&p_cxt->wifi_shell_cxt_mutex);
                    QAT_Response_Str(QAT_RC_QUIET, buffer);
                    free(scan_complete_evt_total);
                } else {
                    QAT_Response_Str(QAT_RC_QUIET, "+CWLAP:failed to allocate memory to scan");
                }
            }
            break;
        }

        default:;
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
static QAT_Command_Status_t Extend_Command_SetWpaPassphrase(uint32_t Op_Type, uint32_t Parameter_Count,
                                                            QAPI_Console_Parameter_t *Parameter_List)
{
    QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
    uint8_t deviceId = qat_get_active_device();
    char buffer[WLAN_RESPONSE_BUFFER_LENGTH] = {0};

    if (!pg_wifi_shell_cxt->wlan_enabled) {
        QAT_Response_Str(QAT_RC_ERROR, "+CWPWD:Enable WLAN before set pwd");
        return rc;
    }

    switch (Op_Type) {
        case QAT_OP_EXEC_W_PARAM: /* AT+CWPWD */
        {
            char *passphrase = Parameter_List[0].String_Value;
            uint32_t len = strlen(passphrase);

            if (Parameter_Count < 1 || !Parameter_List) {
                QAT_Response_Str(QAT_RC_ERROR, NULL);
                return rc;
            }

            if ((len < 8) || (len > 64)) {
                snprintf(buffer, WLAN_RESPONSE_BUFFER_LENGTH, "+CWPWD:FAIL,%d", len);
                QAT_Response_Str(QAT_RC_ERROR, buffer);
                return rc;
            }

            if (len == 64) {
                uint32_t i = 0;
                for (i = 0; i < len; i++) {
                    if (!isxdigit((int)passphrase[i])) {
                        QAT_Response_Str(QAT_RC_ERROR, "+CWPWD:passphrase in hex, please enter [0-9] or [A-F]");
                        return rc;
                    }
                }
            }

            qapi_WLAN_Set_Param(deviceId, __QAPI_WLAN_PARAM_GROUP_WIRELESS_SECURITY,
                                __QAPI_WLAN_PARAM_GROUP_SECURITY_PASSPHRASE, (void *)passphrase, len, FALSE);
            memscpy(pg_wifi_shell_cxt->passphrase, sizeof(pg_wifi_shell_cxt->passphrase),
                    passphrase, len);
            pg_wifi_shell_cxt->passphrase[len] = '\0';
            break;
        }

        case QAT_OP_EXEC: {
            QAT_Response_Str(QAT_RC_QUIET, "+CWPWD=<PASSWORD>(The PASSWORD length should be between 8 and 64)");
            break;
        }
        default:;
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
static QAT_Command_Status_t Extend_Command_SetWpaParameters(uint32_t Op_Type, uint32_t Parameter_Count,
                                                            QAT_Parameter_t *Parameter_List)
{
    QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
    char buffer[WLAN_RESPONSE_BUFFER_LENGTH] = {0};
    uint8_t deviceId = qat_get_active_device();

    qapi_WLAN_Auth_Mode_e e_wpa_ver;
    qapi_WLAN_Crypt_Type_e e_cipher;

    if (!pg_wifi_shell_cxt->wlan_enabled) {
        QAT_Response_Str(QAT_RC_ERROR, "+CWWPA:Enable WLAN before set wpa");
        return rc;
    }

    switch (Op_Type) {
        case QAT_OP_EXEC_W_PARAM: /* AT+CWWPA */
        {
            if (Parameter_Count < 1 || !Parameter_List || Parameter_List[0].Integer_Is_Valid) {
                QAT_Response_Str(QAT_RC_ERROR, NULL);
                return rc;
            }

            char *wpaVer = Parameter_List[0].String_Value;

            if (!strcmp(wpaVer, "WPA")) {
                e_wpa_ver = QAPI_WLAN_AUTH_WPA_PSK_E;
            } else if (!strcmp(wpaVer, "WPA2")) {
                e_wpa_ver = QAPI_WLAN_AUTH_WPA2_PSK_E;
            } else if (!strcmp(wpaVer, "SAE")) {
                e_wpa_ver = QAPI_WLAN_AUTH_WPA3_SAE_E;
            } else if (!strcmp(wpaVer, "SAE_WPA2")) {
                e_wpa_ver = QAPI_WLAN_AUTH_WPA2_SAE_MIXED_E;
            } else if(!strcmp(wpaVer,"SAE_WPA2_WPA")) {
                e_wpa_ver = QAPI_WLAN_AUTH_WPA_WPA2_SAE_MIXED_E;
            } else {
                snprintf(buffer, WLAN_RESPONSE_BUFFER_LENGTH, "+CWWPA:FAIL, %s", wpaVer);
                QAT_Response_Str(QAT_RC_ERROR, buffer);
                return rc;
            }

            if (((e_wpa_ver != QAPI_WLAN_AUTH_WPA2_SAE_MIXED_E) && (e_wpa_ver != QAPI_WLAN_AUTH_WPA_WPA2_SAE_MIXED_E)) && 
                (Parameter_Count != 3 || Parameter_List[1].Integer_Is_Valid || Parameter_List[2].Integer_Is_Valid)) {
                QAT_Response_Str(QAT_RC_ERROR, NULL);
                return rc;
            }
            if ((e_wpa_ver == QAPI_WLAN_AUTH_WPA2_SAE_MIXED_E) || (e_wpa_ver == QAPI_WLAN_AUTH_WPA_WPA2_SAE_MIXED_E)) {
                e_cipher = QAPI_WLAN_CRYPT_AUTO;
            } else {
                char *ucipher = Parameter_List[1].String_Value;
                char *mcipher = Parameter_List[2].String_Value;

                if (strcmp(ucipher, mcipher)) {
                    snprintf(buffer, WLAN_RESPONSE_BUFFER_LENGTH, "+CWWPA:invaid uchipher mcipher, should be same");
                    QAT_Response_Str(QAT_RC_ERROR, buffer);
                    return rc;
                }
                if (!strcmp(ucipher, "TKIP")) {
                    e_cipher = QAPI_WLAN_CRYPT_TKIP_CRYPT_E;
                } else if (!strcmp(ucipher, "CCMP")) {
                    e_cipher = QAPI_WLAN_CRYPT_AES_CRYPT_E;
                } else {
                    snprintf(buffer, WLAN_RESPONSE_BUFFER_LENGTH,
                             "+CWWPA:invaid uchipher mcipher, should be TKIP or CCMP");
                    QAT_Response_Str(QAT_RC_ERROR, buffer);
                    return rc;
                }
            }
            pg_wifi_shell_cxt->auth = e_wpa_ver;
            qapi_WLAN_Set_Param(deviceId, __QAPI_WLAN_PARAM_GROUP_WIRELESS_SECURITY,
                                __QAPI_WLAN_PARAM_GROUP_SECURITY_AUTH_MODE, (void *)&e_wpa_ver,
                                sizeof(qapi_WLAN_Auth_Mode_e), FALSE);
            qapi_WLAN_Set_Param(deviceId, __QAPI_WLAN_PARAM_GROUP_WIRELESS_SECURITY,
                                __QAPI_WLAN_PARAM_GROUP_SECURITY_ENCRYPTION_TYPE, (void *)&e_cipher,
                                sizeof(qapi_WLAN_Crypt_Type_e), FALSE);
            pg_wifi_shell_cxt->cipher = (uint32_t)e_cipher;

            break;
        }
        case QAT_OP_EXEC: {
            QAT_Response_Str(QAT_RC_QUIET, "+CWWPA=WPA/WPA2/SEA,CCMP,CCMP/TKIP,TKIP. For mix mode, +CWWPA=SAE_WPA2/SAE_WPA2_WPA,CCMP and TKIP are not required");
            break;
        }
        default:;
    }
    rc = QAT_Response_Str(QAT_RC_OK, NULL);
    return rc;
}

static int32_t qat_set_channel(int32_t channeldata)
{
    char buffer[WLAN_RESPONSE_BUFFER_LENGTH] = {0};
    qapi_Status_t ret = QAPI_OK;
    QAT_Command_Status_t rc = QAT_STATUS_SUCCESS_E;
    uint8_t deviceId = qat_get_active_device();
    int32_t channel[2] = {0, 0};

    channel[0] = channeldata;
#ifdef CONFIG_6GHZ
    channel[1] = 0;
#else
    QAT_Response_Str(QAT_RC_ERROR, "cannot set 6g channel since 6g is not enabled");
#endif

    ret = qapi_WLAN_Set_Param(deviceId, __QAPI_WLAN_PARAM_GROUP_WIRELESS, __QAPI_WLAN_PARAM_GROUP_WIRELESS_CHANNEL,
                              (void *)&channel, sizeof(channel), FALSE);
    if (ret != QAPI_OK) {
        snprintf(buffer, WLAN_RESPONSE_BUFFER_LENGTH, "FAIL, %d", channel[0]);
        QAT_Response_Str(QAT_RC_QUIET_NO_CR, buffer);
    }

    return rc;
}

static int32_t qat_set_11nht_cap(char *ht_config)
{
    qapi_Status_t ret = QAPI_OK;
    QAT_Command_Status_t rc = QAT_STATUS_SUCCESS_E;
    uint8_t deviceId = qat_get_active_device();

	qapi_WLAN_11n_HT_Config_t config;



	if(!strcmp(ht_config,"disable"))
		config.htconfig = QAPI_WLAN_11N_DISABLED_E;
	else if(!strcmp(ht_config,"ht20")) {
		config.htconfig = QAPI_WLAN_11N_HT20_E;
        config.sgi = 1;
        config.mpdu_density = 0;
    } else {
		QAT_Response_Str(QAT_RC_ERROR, "Unknown ht config, only support disable/ht20");
		return 1;
	}
    ret = qapi_WLAN_Set_Param(deviceId, __QAPI_WLAN_PARAM_GROUP_WIRELESS, __QAPI_WLAN_PARAM_GROUP_WIRELESS_11N_HT,
                              &config, sizeof(config), FALSE);
    if (ret != QAPI_OK) {
        QAT_Response_Str(QAT_RC_ERROR, NULL);
        return 1;
    }

    return rc;
}

int32_t qat_get_phy_mode(char *buffer, int offset)
{
    QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
    qapi_WLAN_Phy_Mode_e phy_mode;
    char data[32 + 1] = {'\0'};
    uint32_t length = sizeof(qapi_WLAN_Phy_Mode_e);
    uint32_t deviceId = 0;
    if (QAPI_OK != qapi_WLAN_Get_Param(deviceId, __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                                       __QAPI_WLAN_PARAM_GROUP_WIRELESS_PHY_MODE, &phy_mode, &length)) {
        QAT_Response_Str(QAT_RC_ERROR, "get phy mode fail");
        return rc;
    }

    if (phy_mode == QAPI_WLAN_11B_MODE_E)
        strlcpy(data, "b", sizeof(data));
    else if (phy_mode == QAPI_WLAN_11G_MODE_E)
        strlcpy(data, "g", sizeof(data));
    else if (phy_mode == QAPI_WLAN_11NG_HT20_MODE_E)
        strlcpy(data, "ng", sizeof(data));
    else if (phy_mode == QAPI_WLAN_11A_MODE_E)
        strlcpy(data, "a", sizeof(data));
    else if (phy_mode == QAPI_WLAN_11A_HT20_MODE_E)
        strlcpy(data, "a", sizeof(data));
    else if (phy_mode == QAPI_WLAN_11ABGN_HT20_MODE_E)
        strlcpy(data, "abgn", sizeof(data));
    else {
        offset += snprintf(buffer + offset, WLAN_RESPONSE_BUFFER_LENGTH - offset, "FAIL, %d", (int)phy_mode);
        return rc;
    }

    offset += snprintf(buffer + offset, WLAN_RESPONSE_BUFFER_LENGTH - offset, "%s", data);
    return offset;
}

int32_t qat_regulatory_info(qapi_WLAN_Reg_Evt_t *reg_info, char *buffer, int offset)
{
    int idx = 0, num;
    uint16_t max_bw = 20;
    char data[32 + 1] = {'\0'};
    qapi_WLAN_Reg_t *reg;

    if (reg_info) {
        offset += snprintf(buffer + offset, WLAN_STR_BUFFER_LENGTH - offset, "%s,", reg_info->alpha);
        reg = reg_info->reg_rules;
        num = (reg_info->num_2g_reg_rules) + (reg_info->num_5g_reg_rules);
        for (idx = 0; idx < num - 1; idx++) {
            memset(data, 0, sizeof(data));
            if (reg[idx].ant_gain == 0)
                strlcpy(data, "N/A", sizeof(data));
            else
                snprintf(data, sizeof(data), "%d", reg[idx].ant_gain);
            offset += snprintf(buffer + offset, WLAN_STR_BUFFER_LENGTH - offset, "(%d-%d@%d,%s,%d),",
                               reg[idx].start_freq, reg[idx].end_freq, max_bw, data, reg[idx].reg_power);
        }
        offset += snprintf(buffer + offset, WLAN_STR_BUFFER_LENGTH - offset, "(%d-%d@%d,%s,%d)",
                           reg[num - 1].start_freq, reg[num - 1].end_freq, max_bw, data, reg[num - 1].reg_power);
    }

    return offset;
}

int32_t qat_get_country_code(char *buffer, int offset)
{
    qapi_Status_t ret = QAPI_OK;
    wifi_shell_cxt_t *p_cxt = pg_wifi_shell_cxt;
    qapi_WLAN_Reg_Evt_t reg_info;
    QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;

    ret = qapi_WLAN_Get_Regulatory_Info(&reg_info);
    if (ret == QAPI_OK) {
        offset = qat_regulatory_info(&reg_info, buffer, offset);
    }

    return offset;
}

int32_t qat_get_wifi_power_mode(char *buffer, int offset)
{
    uint8_t power_mode = 0;
    uint32_t length = sizeof(power_mode);
    uint32_t deviceId = qat_get_active_device();
    char data[64 + 1] = {'\0'};
    QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;

    if (QAPI_OK != qapi_WLAN_Get_Param(deviceId, __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                                       __QAPI_WLAN_PARAM_GROUP_WIRELESS_POWER_MODE_PARAMS, &power_mode, &length)) {
        offset += snprintf(buffer + offset, WLAN_RESPONSE_BUFFER_LENGTH - offset,
                           "get wifi power mode fail for device %d", deviceId);
        return rc;
    }

    if (power_mode == 0) {
        strlcpy(data, "Max Perf", sizeof(data));
    } else {
        strlcpy(data, "Power Save ", sizeof(data));
        if ((power_mode & 1) == 1) {
            strlcat(data, "bmps enabled", sizeof(data));
        }
        if ((power_mode & 2) == 2) {
            strlcat(data, "IMPS enabled", sizeof(data));
        }
        if ((power_mode & 4) == 4) {
            strlcat(data, "WUR enabled", sizeof(data));
        }
        if ((power_mode & 8) == 8) {
            strlcat(data, "WNM enabled", sizeof(data));
        }
    }
    offset += snprintf(buffer + offset, WLAN_RESPONSE_BUFFER_LENGTH - offset, "%s", data);
    return offset;
}

int32_t qat_get_device_mac_address(char *buffer, int offset)
{
    QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
    uint8_t mac[__QAPI_WLAN_MAC_LEN] = {0};
    uint32_t length = __QAPI_WLAN_MAC_LEN;
    uint8_t deviceId = qat_get_active_device();

    if (QAPI_OK != qapi_WLAN_Get_Param(deviceId, __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                                       __QAPI_WLAN_PARAM_GROUP_WIRELESS_MAC_ADDRESS, &mac[0], &length)) {
        offset += snprintf(buffer + offset, WLAN_RESPONSE_BUFFER_LENGTH - offset, "get mac address fail for device %d",
                           deviceId);
        return rc;
    }

    offset += snprintf(buffer + offset, WLAN_RESPONSE_BUFFER_LENGTH - offset, "%02x-%02x-%02x-%02x-%02x-%02x", mac[0],
                       mac[1], mac[2], mac[3], mac[4], mac[5]);

    return offset;
}

int32_t qat_get_rssi(char *buffer, int offset)
{
    qapi_Status_t ret = QAPI_ERROR;
    QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
    uint8_t rssi = 0;
    uint32_t length = sizeof(rssi);
    uint32_t deviceId = qat_get_active_device();

    ret = qapi_WLAN_Get_Param(deviceId, __QAPI_WLAN_PARAM_GROUP_WIRELESS, __QAPI_WLAN_PARAM_GROUP_WIRELESS_RSSI, &rssi,
                              &length);
    if (QAPI_OK == ret) {
        offset += snprintf(buffer + offset, WLAN_RESPONSE_BUFFER_LENGTH - offset, "%d", rssi);
    } else {
        offset += snprintf(buffer + offset, WLAN_RESPONSE_BUFFER_LENGTH - offset, "NA");
    }

    return offset;
}

int32_t qat_get_op_mode(char *buffer, int offset)
{
    QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
    qapi_WLAN_DEV_Mode_e conc_mode, opmode;
    uint32_t length = sizeof(qapi_WLAN_DEV_Mode_e);
    uint8_t deviceId = qat_get_active_device();

    if (QAPI_OK != qapi_WLAN_Get_Param(deviceId, __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                                       __QAPI_WLAN_PARAM_GROUP_WIRELESS_CONCURRENCY_MODE, &conc_mode, &length)) {
        QAT_Response_Str(QAT_RC_ERROR, "get concurrency mode fail");
        return rc;
    }

    if (conc_mode == DEV_MODE_AP_STA_E) {
        offset += snprintf(buffer + offset, WLAN_RESPONSE_BUFFER_LENGTH - offset, "concurrency mode");
        return rc;
    }

    if (QAPI_OK != qapi_WLAN_Get_Param(deviceId, __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                                       __QAPI_WLAN_PARAM_GROUP_WIRELESS_OPERATION_MODE, &opmode, &length)) {
        offset += snprintf(buffer + offset, WLAN_STR_BUFFER_LENGTH - offset, "get operation mode fail");
        return rc;
    }

    if (opmode == DEV_MODE_STATION_E) {
        offset += snprintf(buffer + offset, WLAN_RESPONSE_BUFFER_LENGTH - offset, "station");
    } else if (opmode == DEV_MODE_AP_E) {
        offset += snprintf(buffer + offset, WLAN_RESPONSE_BUFFER_LENGTH - offset, "softap");
    } else if (opmode == DEV_MODE_NO_CONC_E) {
        offset += snprintf(buffer + offset, WLAN_RESPONSE_BUFFER_LENGTH - offset, "non_softap+station");
    }

    return offset;
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
static QAT_Command_Status_t Extend_Command_PyhMode(uint32_t Op_Type, uint32_t Parameter_Count,
                                                   QAT_Parameter_t *Parameter_List)
{
    qapi_Status_t ret = QAPI_OK;
    QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
    uint8_t deviceId = qat_get_active_device();
    qapi_WLAN_Phy_Mode_e phyMode = 0;
    char buffer[WLAN_RESPONSE_BUFFER_LENGTH] = {0};
    int offset = 0;
    char *wmode;

    if (0 == pg_wifi_shell_cxt->wlan_enabled) {
        QAT_Response_Str(QAT_RC_ERROR, "+CWPHYMODE:Enable WLAN before set phymode");
        return rc;
    }

    wmode = (char *)Parameter_List[0].String_Value;
    switch (Op_Type) {
        case QAT_OP_EXEC_W_PARAM: {
            if (!strcmp(wmode, "a"))
                phyMode = QAPI_WLAN_11A_MODE_E;
            else if (!strcmp(wmode, "b"))
                phyMode = QAPI_WLAN_11B_MODE_E;
            else if (!strcmp(wmode, "g"))
                phyMode = QAPI_WLAN_11G_MODE_E;
            else if (!strcmp(wmode, "ng"))
                phyMode = QAPI_WLAN_11NG_HT20_MODE_E;
            else if (!strcmp(wmode, "abgn"))
                phyMode = QAPI_WLAN_11ABGN_HT20_MODE_E;
            else {
                QAT_Response_Str(QAT_RC_ERROR, "+CWPHYMODE:Unknown wmode, only support a/b/g/ng/abgn/");
                return rc;
            }

            ret = qapi_WLAN_Set_Param(deviceId, __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                                      __QAPI_WLAN_PARAM_GROUP_WIRELESS_PHY_MODE, &phyMode, sizeof(phyMode), FALSE);
            if (ret != QAPI_OK) {
                QAT_Response_Str(QAT_RC_ERROR, NULL);
                return rc;
            }
            break;
        }
        case QAT_OP_EXEC: {
            QAT_Response_Str(QAT_RC_QUIET, "+CWPHYMODE=a/b/g/ng/abgn");
            break;
        }
        case QAT_OP_QUERY: {
            offset += snprintf(buffer + offset, WLAN_RESPONSE_BUFFER_LENGTH - offset, "+CWPHYMODE:");
            qat_get_phy_mode(&buffer, offset);
        }
        default:;
    }
    rc = QAT_Response_Str(QAT_RC_OK, buffer);
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
static QAT_Command_Status_t Extend_Command_CountryCode(uint32_t Op_Type, uint32_t Parameter_Count,
                                                       QAT_Parameter_t *Parameter_List)
{
    qapi_Status_t ret = QAPI_OK;
    QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
    wifi_shell_cxt_t *p_cxt = pg_wifi_shell_cxt;
    char buffer[WLAN_RESPONSE_BUFFER_LENGTH] = {0};
    int offset = 0;

    if (0 == p_cxt->wlan_enabled) {
        QAT_Response_Str(QAT_RC_ERROR, "+CWCOUNTRY:Enable WLAN before set country code");
        return rc;
    }

    switch (Op_Type) {
        case QAT_OP_EXEC_W_PARAM: {
            if (Parameter_Count != 1 || !Parameter_List || Parameter_List[0].Integer_Is_Valid) {
                QAT_Response_Str(QAT_RC_ERROR, NULL);
                return rc;
            }

            ret = set_country_code((char *)Parameter_List[0].String_Value);
            if (ret != QAPI_OK) {
                snprintf(buffer, WLAN_RESPONSE_BUFFER_LENGTH, "+CWCOUNTRY: FAIL, %s\n",
                         (char *)Parameter_List[0].String_Value);
                QAT_Response_Str(QAT_RC_ERROR, NULL);
                return rc;
            }
            break;
        }
        case QAT_OP_EXEC: {
            QAT_Response_Str(QAT_RC_QUIET, "+CWCOUNTRY=<countrycode>, e.g. US/CN");
            break;
        }
        case QAT_OP_QUERY: {
            offset += snprintf(buffer + offset, WLAN_RESPONSE_BUFFER_LENGTH - offset, "\r\n+CWCOUNTRY:");
            qat_get_country_code(&buffer, offset);
            QAT_Response_Str(QAT_RC_QUIET, buffer);
        }
        default:;
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
static QAT_Command_Status_t Extend_Command_SetModeOption(uint32_t Op_Type, uint32_t Parameter_Count,
                                                         QAT_Parameter_t *Parameter_List)
{
    QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
    qapi_Status_t ret = QAPI_OK;
    uint8_t deviceId = qat_get_active_device();
    wifi_shell_cxt_t *p_cxt = pg_wifi_shell_cxt;
    char *param = NULL;
    char *ssid = NULL;
    int ssidlen = 0;

    if (pg_wifi_shell_cxt->wlan_enabled == 0) {
        QAT_Response_Str(QAT_RC_ERROR, "+CWSOFTAP:wlan is not enabled");
        return rc;
    }

    switch (Op_Type) {
        case QAT_OP_EXEC_W_PARAM: /* AT+CWSOFTAP */
        {
            if (Parameter_Count < 3 || !Parameter_List) {
                param = (char *)Parameter_List[0].String_Value;
                if (!strcmp(param, "disable")) {
                    rc = (QAT_Command_Status_t)qat_set_11nht_cap(param);
                    if (rc == QAT_STATUS_SUCCESS_E) {
                        QAT_Response_Str(QAT_RC_QUIET, "+CWSOFTAP:disabled ap");
                        return rc;
                    }
                }
                QAT_Response_Str(QAT_RC_ERROR, "+CWSOFTAP:parameter error");
                return rc;
            }

            rc = (QAT_Command_Status_t)qat_set_11nht_cap((char *)Parameter_List[0].String_Value);
            if (rc != QAT_STATUS_SUCCESS_E) {
                QAT_Response_Str(QAT_RC_ERROR, "+CWSOFTAP:set 11nht error");
                return rc;
            }

            rc = (QAT_Command_Status_t)qat_set_channel(Parameter_List[1].Integer_Value);
            if (rc != QAT_STATUS_SUCCESS_E) {
                QAT_Response_Str(QAT_RC_ERROR, "+CWSOFTAP:set channel error");
                return rc;
            }

            ssid = Parameter_List[2].String_Value;
            ssidlen = strlen(ssid);
            qapi_WLAN_Set_Param(0, __QAPI_WLAN_PARAM_GROUP_WIRELESS, __QAPI_WLAN_PARAM_GROUP_WIRELESS_SSID,
                                (void *)ssid, ssidlen, FALSE);

            ret = qapi_WLAN_Commit(deviceId);
            if (ret != QAPI_OK) {
                QAT_Response_Str(QAT_RC_ERROR, NULL);
                return rc;
            }

            if (deviceId == NT_DEV_AP_ID && ret == QAPI_OK) {
                memscpy(p_cxt->ssid, ssidlen, ssid, ssidlen);
                p_cxt->ssid[ssidlen] = 0;
                p_cxt->ssid_length = ssidlen;
            }

            break;
        }
        case QAT_OP_EXEC: {
            QAT_Response_Str(QAT_RC_QUIET,
                             "+CWSOFTAP=<param1>,<param2>,<ssid>(param1:disable/ht20, param2:1-14, 36-165)");
            break;
        }
        default:;
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
static QAT_Command_Status_t Extend_Command_SetOperatingMode(uint32_t Op_Type, uint32_t Parameter_Count,
                                                            QAT_Parameter_t *Parameter_List)
{
    QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
    char *hidden = "";
    int offset = 0;
    char buffer[WLAN_RESPONSE_BUFFER_LENGTH] = {0};

    if (pg_wifi_shell_cxt->wlan_enabled == 0) {
        QAT_Response_Str(QAT_RC_ERROR, "+CWMODE:wlan is not enabled");
        return rc;
    }

    switch (Op_Type) {
        case QAT_OP_EXEC_W_PARAM: /* AT+CWMODE */
        {
            if (Parameter_Count < 1 || !Parameter_List) {
                QAT_Response_Str(QAT_RC_ERROR, "+CWMODE:wlan is not enabled");
                return rc;
            }

            if (Parameter_Count >= 2) {
                hidden = (char *)Parameter_List[1].String_Value;
            }

            rc = (QAT_Command_Status_t)qat_set_op_mode((char *)Parameter_List[0].String_Value, hidden);
            if (rc != QAT_STATUS_SUCCESS_E) {
                QAT_Response_Str(QAT_RC_ERROR, "+CWMODE:set op mode error");
                return rc;
            }

            break;
        }

        case QAT_OP_QUERY: {
            offset += snprintf(buffer + offset, WLAN_RESPONSE_BUFFER_LENGTH - offset, "+CWMODE:");
            qat_get_op_mode(&buffer, offset);
            break;
        }
        case QAT_OP_EXEC: {
            QAT_Response_Str(QAT_RC_QUIET, "+CWMODE=sta/ap");
            break;
        }
        default:;
    }
    rc = QAT_Response_Str(QAT_RC_OK, buffer);
    return rc;
}

/* Forward declaration — defined later in this file. */
static int load_wifi_cred_from_flash(wifi_shell_cxt_t *p_cxt, uint8_t deviceId);

/**
   @brief Processes the Extend command from the QAT.

   This command will change the current group to its parent. No parameters are
   expected for this command.

   @param[in] Op_Type          The input command type.
   @param[in] Parameter_Count  Number of parameters that were entered into the
                               command line.
   @param[in] Parameter_List   List of parameters entered into the command line.
*/
static QAT_Command_Status_t Extend_Command_Connect(uint32_t Op_Type, uint32_t Parameter_Count,
                                                   QAT_Parameter_t *Parameter_List)
{
    QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
    char *bssid = NULL;
    int ssidLength = 0;
    char *ssid = NULL;
    uint8_t deviceId = qat_get_active_device();
    wifi_shell_cxt_t *p_cxt = pg_wifi_shell_cxt;
    char buffer[WLAN_RESPONSE_BUFFER_LENGTH] = {0};
    qapi_Status_t ret = QAPI_OK;
    int32_t offset = 0;

    if (0 == p_cxt->wlan_enabled) {
        QAT_Response_Str(QAT_RC_ERROR, "+CWJAP:Enable WLAN before get the WLAN infomation");
        return rc;
    }

    switch (Op_Type) {
        case QAT_OP_QUERY: /* AT+CWJAP? */
        {
            if (p_cxt->connected == true) {
                offset += snprintf(buffer + offset, WLAN_RESPONSE_BUFFER_LENGTH - offset, "+CWJAP:%s,%d,", p_cxt->ssid,
                                   p_cxt->channel_frequency);
            } else {
                offset += snprintf(buffer + offset, WLAN_RESPONSE_BUFFER_LENGTH - offset, "+CWJAP:NA,NA,");
            }

            offset = qat_get_rssi(&buffer, offset);
            offset += snprintf(buffer + offset, WLAN_RESPONSE_BUFFER_LENGTH - offset, ",");

            offset = qat_get_wifi_power_mode(&buffer, offset);
            offset += snprintf(buffer + offset, WLAN_RESPONSE_BUFFER_LENGTH - offset, ",");

            offset = qat_get_device_mac_address(&buffer, offset);
            offset += snprintf(buffer + offset, WLAN_RESPONSE_BUFFER_LENGTH - offset, ",");

            offset = qat_get_op_mode(&buffer, offset);
            offset += snprintf(buffer + offset, WLAN_RESPONSE_BUFFER_LENGTH - offset, ",");

            qat_get_phy_mode(&buffer, offset);

            QAT_Response_Str(QAT_RC_QUIET, buffer);

            break;
        }

        case QAT_OP_EXEC: {
            QAT_Response_Str(QAT_RC_QUIET, "+CWJAP=<ssid>");
            break;
        }

        case QAT_OP_EXEC_W_PARAM: /* AT+CWJAP */
        {
            if (Parameter_Count < 1 || !Parameter_List) {
                QAT_Response_Str(QAT_RC_ERROR, NULL);
                return rc;
            }

            /* AT+CWJAP=@saved  →  connect using flash-stored credentials */
            if (strcmp(Parameter_List[0].String_Value, "@saved") == 0) {
                int load_ret = load_wifi_cred_from_flash(p_cxt, deviceId);
                if (load_ret < 0) {
                    const char *msg =
                        (load_ret == -1) ? "+CWJAP: filesystem not mounted" :
                        (load_ret == -2) ? "+CWJAP: no saved credentials"   :
                                           "+CWJAP: saved credentials corrupt";
                    QAT_Response_Str(QAT_RC_ERROR, msg);
                    return rc;
                }
                snprintf(buffer, WLAN_RESPONSE_BUFFER_LENGTH, "+CWJAP:connecting to ssid %s (@saved)", p_cxt->ssid);
                QAT_Response_Str(QAT_RC_QUIET, buffer);
                ret = qapi_WLAN_Commit(deviceId);
                if (ret != QAPI_OK) {
                    QAT_Response_Str(QAT_RC_ERROR, NULL);
                    return rc;
                }
                break;
            }

            ssid = Parameter_List[0].String_Value;

            if (Parameter_Count >= 2) {
                bssid = Parameter_List[1].String_Value;
            }

            ssidLength = strlen(ssid);
            qapi_WLAN_Set_Param(0, __QAPI_WLAN_PARAM_GROUP_WIRELESS, __QAPI_WLAN_PARAM_GROUP_WIRELESS_SSID,
                                (void *)ssid, ssidLength, FALSE);

            if (bssid) {
                uint8_t bssidToConnect[__QAPI_WLAN_MAC_LEN] = {0};
                if (ether_aton(bssid, bssidToConnect) < 0) {
                    QAT_Response_Str(QAT_RC_ERROR, "+CWJAP:Invalid BSSID to connect");
                    return rc;
                }
                qapi_WLAN_Set_Param(0, __QAPI_WLAN_PARAM_GROUP_WIRELESS, __QAPI_WLAN_PARAM_GROUP_WIRELESS_BSSID,
                                    (void *)bssidToConnect, __QAPI_WLAN_MAC_LEN, FALSE);
            }

            snprintf(buffer, WLAN_RESPONSE_BUFFER_LENGTH, "+CWJAP:connecting to ssid %s", ssid);
            QAT_Response_Str(QAT_RC_QUIET, buffer);

            /* Always save SSID to context for AT+CWSAVE persistence, regardless of device mode. */
            memscpy(p_cxt->ssid, sizeof(p_cxt->ssid), ssid, ssidLength);
            p_cxt->ssid[ssidLength] = 0;
            p_cxt->ssid_length = ssidLength;

            ret = qapi_WLAN_Commit(deviceId);
            if (ret != QAPI_OK) {
                QAT_Response_Str(QAT_RC_ERROR, NULL);
                return rc;
            }

            break;
        }
        default:;
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
static QAT_Command_Status_t Extend_Command_Disconnect(uint32_t Op_Type, uint32_t Parameter_Count,
                                                      QAT_Parameter_t *Parameter_List)
{
    QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
    uint8_t deviceId = qat_get_active_device();

    if (!pg_wifi_shell_cxt->wlan_enabled) {
        QAT_Response_Str(QAT_RC_ERROR, "+CWQAP:wlan is not enabled");
        return rc;
    }

    switch (Op_Type) {
        case QAT_OP_EXEC: /* AT+CWQAP */
        {
            pg_wifi_shell_cxt->auth = QAPI_WLAN_AUTH_NONE_E;
            qapi_WLAN_Disconnect(deviceId);
            break;
        }
        default:;
    }

    rc = QAT_Response_Str(QAT_RC_OK, NULL);
    return rc;
}
/**
   @brief ANTIINF

   This command will change the current group to its parent. No parameters are
   expected for this command.

   @param[in] Op_Type          The input command type.
   @param[in] Parameter_Count  Number of parameters that were entered into the
                               command line.
   @param[in] Parameter_List   List of parameters entered into the command line.
*/
#define RT_IDX_11B_LONG_1_MBPS 0
#define RT_IDX_11A_6_MBPS      1
#define RT_IDX_11A_12_MBPS     2
static QAT_Command_Status_t Extend_Command_ANTIINF(uint32_t Op_Type, uint32_t Parameter_Count,
                                                   QAT_Parameter_t *Parameter_List)
{
    char buffer[CMD_STR_BUFFER_LENGTH] = {0};
    QAT_Command_Status_t rc = QAT_STATUS_SUCCESS_E;
    int offset = 0;
    uint8_t deviceId = qat_get_active_device();
    uint32_t enable = 1;
    uint32_t rts_rate = RT_IDX_11B_LONG_1_MBPS;
    qapi_WLAN_Edca_Params_t edca_param_cfg;
    uint32_t threshold = 60;
    uint32_t slot_time = 20;

    switch (Op_Type) {
        case QAT_OP_EXEC: /* AT+ANTIINF */
        {
            rc = QAT_Response_Str(QAT_RC_OK,
                                  "AT+ANTIINF=<0: 1M RTS|1:6M RTS|2: 12M RTS>\r\n"
                                  "AT+ANTIINF?: get ANTIINF");
            break;
        }
        case QAT_OP_QUERY: /* AT+ANTIINF? */
        {
            qapi_WLAN_BA_Window_Params_t ba_win;
            uint32_t length = 0;

            edca_param_cfg.qid = 0xff;
            length = sizeof(enable);
            if (QAPI_OK != qapi_WLAN_Get_Param(deviceId, __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                                               __QAPI_WLAN_PARAM_GROUP_WIRELESS_RTS, &enable, &length)) {
                memset((void *)buffer, 0, CMD_STR_BUFFER_LENGTH);
                snprintf(buffer, CMD_STR_BUFFER_LENGTH, "+ANTIINF:get RTS enable fail for device %d", deviceId);
                rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
                return rc;
            }

            length = sizeof(rts_rate);
            rts_rate = 0;

            if (QAPI_OK != qapi_WLAN_Get_Param(deviceId, __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                                               __QAPI_WLAN_PARAM_GROUP_WIRELESS_RTS_RATE_2G, &rts_rate, &length)) {
                memset((void *)buffer, 0, CMD_STR_BUFFER_LENGTH);
                snprintf(buffer, CMD_STR_BUFFER_LENGTH, "+ANTIINF:get RTS enable fail for device %d", deviceId);
                rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
                return rc;
            }

            length = sizeof(edca_param_cfg);
            if (QAPI_OK != qapi_WLAN_Get_Param(deviceId, __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                                               __QAPI_WLAN_PARAM_GROUP_WIRELESS_EDCA_PARAM, &edca_param_cfg, &length)) {
                memset((void *)buffer, 0, CMD_STR_BUFFER_LENGTH);
                snprintf(buffer, CMD_STR_BUFFER_LENGTH, "+ANTIINF:get contention window size fail for device %d",
                         deviceId);
                rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
                return rc;
            }

            length = sizeof(threshold);
            if (QAPI_OK != qapi_WLAN_Get_Param(deviceId, __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                                               __QAPI_WLAN_PARAM_GROUP_WIRELESS_PER_UPPER_THRESHOLD, &threshold,
                                               &length)) {
                memset((void *)buffer, 0, CMD_STR_BUFFER_LENGTH);
                snprintf(buffer, CMD_STR_BUFFER_LENGTH, "+ANTIINF:get per upper threshold fail for device %d",
                         deviceId);
                rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
                return rc;
            }

            length = sizeof(ba_win);
            if (QAPI_OK != qapi_WLAN_Get_Param(deviceId, __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                                               __QAPI_WLAN_PARAM_GROUP_WIRELESS_BA_WINDOW, &ba_win, &length)) {
                memset((void *)buffer, 0, CMD_STR_BUFFER_LENGTH);
                snprintf(buffer, CMD_STR_BUFFER_LENGTH, "+ANTIINF:get per upper ba window size fail for device %d",
                         deviceId);
                rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
                return rc;
            }

            length = sizeof(slot_time);
            if (QAPI_OK != qapi_WLAN_Get_Param(deviceId, __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                                               __QAPI_WLAN_PARAM_GROUP_WIRELESS_SLOT_TIME, &slot_time, &length)) {
                memset((void *)buffer, 0, CMD_STR_BUFFER_LENGTH);
                snprintf(buffer, CMD_STR_BUFFER_LENGTH, "+ANTIINF:get slot time fail for device %d", deviceId);
                rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
                return rc;
            }

            offset = snprintf(buffer + offset, CMD_STR_BUFFER_LENGTH - offset, "+ANTIINF:%s,%d,%d,%d,%d,%d,%d,%d,%d",
                              enable ? "enable" : "disable", rts_rate, edca_param_cfg.qid, edca_param_cfg.cw_min,
                              edca_param_cfg.cw_max, threshold, ba_win.ack_timeout, 2 * ba_win.delay, slot_time);
            rc = QAT_Response_Str(QAT_RC_OK, buffer);
            break;
        }
        case QAT_OP_EXEC_W_PARAM: /* AT+ANTIINF= */
        {
            qapi_WLAN_BA_Window_Params_t ba_win_size_cfg;

            edca_param_cfg.qid = 0xff;  // set queue 0 - 7
            edca_param_cfg.aifsn = 0x3;
            edca_param_cfg.cw_min = 0x2;  // cwmin = 2^2 -1
            edca_param_cfg.cw_max = 0x4;  // cwmax = 2^4 - 1
            edca_param_cfg.txop_limit = 200;

            ba_win_size_cfg.ack_timeout = 128;  // 128us, should less than 4096
            ba_win_size_cfg.delay = 10;         // 10 * 2 * SM clock cycles, should less than 64

            rts_rate = Parameter_List[0].Integer_Value;

            if (0 != qapi_WLAN_Set_Param(deviceId, __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                                         __QAPI_WLAN_PARAM_GROUP_WIRELESS_RTS, &enable, sizeof(enable), FALSE)) {
                rc = QAT_Response_Str(QAT_RC_ERROR,
                                      "+ANTIINF:Enable RTS/CTS fail\r\n"
                                      "1:enable  0:disable");
                return rc;
            }

            if (0 != qapi_WLAN_Set_Param(deviceId, __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                                         __QAPI_WLAN_PARAM_GROUP_WIRELESS_RTS_RATE_2G, &rts_rate, sizeof(rts_rate),
                                         FALSE)) {
                rc = QAT_Response_Str(QAT_RC_ERROR,
                                      "+ANTIINF:fix RTS rate fail\r\n"
                                      "0:1Mbps  1:6Mbps 2:12Mbps");
                return rc;
            }

            if (0 != qapi_WLAN_Set_Param(deviceId, __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                                         __QAPI_WLAN_PARAM_GROUP_WIRELESS_EDCA_PARAM, &edca_param_cfg,
                                         sizeof(edca_param_cfg), FALSE)) {
                rc = QAT_Response_Str(QAT_RC_ERROR,
                                      "+ANTIINF:set edca param fail\r\n"
                                      "set qid = 0xff for all queue; set qid = 0-7 for single queue");
                return rc;
            }

            if (0 != qapi_WLAN_Set_Param(deviceId, __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                                         __QAPI_WLAN_PARAM_GROUP_WIRELESS_PER_UPPER_THRESHOLD, &threshold,
                                         sizeof(threshold), FALSE)) {
                rc = QAT_Response_Str(QAT_RC_ERROR,
                                      "+ANTIINF:set per upper threshold fail\r\n"
                                      "threshold should less than 100");
                return rc;
            }

            if (0 != qapi_WLAN_Set_Param(deviceId, __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                                         __QAPI_WLAN_PARAM_GROUP_WIRELESS_BA_WINDOW, &ba_win_size_cfg,
                                         sizeof(ba_win_size_cfg), FALSE)) {
                rc = QAT_Response_Str(QAT_RC_ERROR,
                                      "+ANTIINF:set per upper threshold fail\r\n"
                                      "ack_timeout should less than 4096\r\n"
                                      "delay should less than 64");
                return rc;
            }

            if (0 != qapi_WLAN_Set_Param(deviceId, __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                                         __QAPI_WLAN_PARAM_GROUP_WIRELESS_SLOT_TIME, &slot_time, sizeof(slot_time),
                                         FALSE)) {
                rc = QAT_Response_Str(QAT_RC_ERROR,
                                      "+ANTIINF:set slot time fail\r\n"
                                      "aet slot time to 9us or 20us");
                return rc;
            }

            rc = QAT_Response_Str(QAT_RC_OK, NULL);
            break;
        }

        default:;
    }

    return rc;
}

/**
   @brief ANTIINF

   This command will change the current group to its parent. No parameters are
   expected for this command.

   @param[in] Op_Type          The input command type.
   @param[in] Parameter_Count  Number of parameters that were entered into the
                               command line.
   @param[in] Parameter_List   List of parameters entered into the command line.
*/
static QAT_Command_Status_t Extend_Command_EDCA(uint32_t Op_Type, uint32_t Parameter_Count,
                                                QAT_Parameter_t *Parameter_List)
{
    char buffer[CMD_STR_BUFFER_LENGTH] = {0};
    QAT_Command_Status_t rc = QAT_STATUS_SUCCESS_E;
    int offset = 0;
    uint8_t deviceId = qat_get_active_device();
    qapi_WLAN_Edca_Params_t edca_param_cfg;
    char *cmd = NULL;

    switch (Op_Type) {
        case QAT_OP_EXEC: /* AT+EDCA */
        {
            rc = QAT_Response_Str(QAT_RC_OK,
                                  "AT+EDCA=setparam,<qtid:0~7 or 255>,<aifsn>,<cwmin:exp>,<cwmax:exp>,<txop_limit>\r\n"
                                  "AT+EDCA=getparam,<qtid:0~7 or 255>");
            break;
        }
        case QAT_OP_EXEC_W_PARAM: /* AT+EDCA= */
        {
            memset((void *)buffer, 0, CMD_STR_BUFFER_LENGTH);

            if (!pg_wifi_shell_cxt->wlan_enabled) {
                /* edca should be set after connectting */
                rc = QAT_Response_Str(QAT_RC_ERROR, "+EDCA:wlan is not enabled");
                return rc;
            }

            cmd = Parameter_List[0].String_Value;
            if (strncmp(cmd, "setparam", 8) == 0) {
                if (Parameter_Count < 6 || !Parameter_List || !Parameter_List[1].Integer_Is_Valid ||
                    !Parameter_List[2].Integer_Is_Valid || !Parameter_List[3].Integer_Is_Valid ||
                    !Parameter_List[4].Integer_Is_Valid || !Parameter_List[5].Integer_Is_Valid) {
                    return QAT_Response_Str(
                        QAT_RC_ERROR,
                        "+EDCA:AT+EDCA=setparam,<qtid:0~7 or 255>,<aifsn>,<cwmin:exp>,<cwmax:exp>,<txop_limit>");
                }

                edca_param_cfg.qid = Parameter_List[1].Integer_Value;
                edca_param_cfg.aifsn = Parameter_List[2].Integer_Value;
                edca_param_cfg.cw_min = Parameter_List[3].Integer_Value;
                edca_param_cfg.cw_max = Parameter_List[4].Integer_Value;
                edca_param_cfg.txop_limit = Parameter_List[5].Integer_Value;

                if (0 != qapi_WLAN_Set_Param(deviceId, __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                                             __QAPI_WLAN_PARAM_GROUP_WIRELESS_EDCA_PARAM, &edca_param_cfg,
                                             sizeof(edca_param_cfg), FALSE)) {
                    offset = snprintf(buffer + offset, CMD_STR_BUFFER_LENGTH - offset,
                                      "+EDCA:set edca param fail, check the wlan connection\r\n"
                                      "set qid = 0xff for all queue; set qid = 0-7 for single queue\r\n");
                    return QAT_Response_Str(QAT_RC_ERROR, buffer);
                }
                rc = QAT_Response_Str(QAT_RC_OK, NULL);
            } else if (strncmp(cmd, "getparam", 8) == 0) {
                uint32_t length = 0;

                if (Parameter_Count < 2 || !Parameter_List || !Parameter_List[1].Integer_Is_Valid) {
                    rc = QAT_Response_Str(QAT_RC_ERROR, "+EDCA:AT+EDCA=getparam,<qtid:0~7 or 255>");
                    return rc;
                }

                edca_param_cfg.qid = Parameter_List[1].Integer_Value;
                length = sizeof(edca_param_cfg);

                if (QAPI_OK != qapi_WLAN_Get_Param(deviceId, __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                                                   __QAPI_WLAN_PARAM_GROUP_WIRELESS_EDCA_PARAM, &edca_param_cfg,
                                                   &length)) {
                    offset = snprintf(buffer + offset, CMD_STR_BUFFER_LENGTH - offset,
                                      "+EDCA:get edcca param fail for device %d", deviceId);
                    rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
                    return rc;
                } else {
                    offset = snprintf(buffer + offset, CMD_STR_BUFFER_LENGTH - offset, "+EDCA:%d,%d,%d,%d,%d",
                                      edca_param_cfg.qid, edca_param_cfg.aifsn, edca_param_cfg.cw_min,
                                      edca_param_cfg.cw_max, edca_param_cfg.txop_limit);
                    rc = QAT_Response_Str(QAT_RC_OK, buffer);
                }
            }

            break;
        }

        default:;
    }

    return rc;
}
/**
   @brief ANTIINF

   This command will change the current group to its parent. No parameters are
   expected for this command.

   @param[in] Op_Type          The input command type.
   @param[in] Parameter_Count  Number of parameters that were entered into the
                               command line.
   @param[in] Parameter_List   List of parameters entered into the command line.
*/
static QAT_Command_Status_t Extend_Command_EDCCATHR(uint32_t Op_Type, uint32_t Parameter_Count,
                                                    QAT_Parameter_t *Parameter_List)
{
    char buffer[CMD_STR_BUFFER_LENGTH] = {0};
    QAT_Command_Status_t rc = QAT_STATUS_SUCCESS_E;
    int offset = 0;
    uint8_t deviceId = qat_get_active_device();
    uint8_t edcca_threshold;

    switch (Op_Type) {
        case QAT_OP_EXEC: /* AT+EDCCATHR */
        {
            rc = QAT_Response_Str(QAT_RC_OK,
                                  "AT+EDCCATHR=<EDCCA value, euqals real value plus 100>\r\n"
                                  "AT+EDCCATHR?: get EDCCATHR");
            break;
        }
        case QAT_OP_EXEC_W_PARAM: /* AT+EDCCA= */
        {
            memset((void *)buffer, 0, CMD_STR_BUFFER_LENGTH);

            if (!pg_wifi_shell_cxt->wlan_enabled) {
                /* edcca should be set after connectting */
                rc = QAT_Response_Str(QAT_RC_ERROR, "+EDCCATHR:wlan is not enabled");
                return rc;
            }

            if (Parameter_Count < 1 || !Parameter_List || !Parameter_List[0].Integer_Is_Valid) {
                return rc = QAT_Response_Str(QAT_RC_ERROR,
                                             "+EDCCATHR: AT+EDCA=<EDCCA value, euqals real value plus 100>");
            }

            edcca_threshold = Parameter_List[0].Integer_Value;

            if (0 != qapi_WLAN_Set_Param(deviceId, __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                                         __QAPI_WLAN_PARAM_GROUP_WIRELESS_EDCCA_THRESHOLD, &edcca_threshold,
                                         sizeof(edcca_threshold), FALSE)) {
                offset = snprintf(buffer + offset, CMD_STR_BUFFER_LENGTH - offset,
                                  "+EDCCATHR: set edcca param fail, check the wlan connection or data validation\r\n"
                                  "default edcca thres is 38");
                return QAT_Response_Str(QAT_RC_ERROR, buffer);
            }
            rc = QAT_Response_Str(QAT_RC_OK, NULL);
            break;
        }
        case QAT_OP_QUERY: /* AT+EDCCA? */
        {
            uint32_t length = 0;

            if (!pg_wifi_shell_cxt->wlan_enabled) {
                /* edcca should be set after connectting */
                rc = QAT_Response_Str(QAT_RC_ERROR, "+EDCCA:wlan is not enabled");
                return rc;
            }

            length = sizeof(edcca_threshold);
            if (QAPI_OK != qapi_WLAN_Get_Param(deviceId, __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                                               __QAPI_WLAN_PARAM_GROUP_WIRELESS_EDCCA_THRESHOLD, &edcca_threshold,
                                               &length)) {
                offset = snprintf(buffer + offset, CMD_STR_BUFFER_LENGTH - offset,
                                  "+EDCCA: get edcca threshold fail for device %d", deviceId);
                return QAT_Response_Str(QAT_RC_ERROR, buffer);
            } else {
                offset = snprintf(buffer + offset, CMD_STR_BUFFER_LENGTH - offset, "+EDCCA:%d", edcca_threshold);
                rc = QAT_Response_Str(QAT_RC_OK, buffer);
            }
            break;
        }

        default:;
    }

    return rc;
}
/**
   @brief ANTIINF

   This command will change the current group to its parent. No parameters are
   expected for this command.

   @param[in] Op_Type          The input command type.
   @param[in] Parameter_Count  Number of parameters that were entered into the
                               command line.
   @param[in] Parameter_List   List of parameters entered into the command line.
*/
static QAT_Command_Status_t Extend_Command_BMISSTHR(uint32_t Op_Type, uint32_t Parameter_Count,
                                                    QAT_Parameter_t *Parameter_List)
{
    char buffer[CMD_STR_BUFFER_LENGTH] = {0};
    QAT_Command_Status_t rc = QAT_STATUS_SUCCESS_E;
    int offset = 0;
    uint8_t deviceId = qat_get_active_device();
    uint8_t bmiss_threshold;

    switch (Op_Type) {
        case QAT_OP_EXEC: /* AT+BMISSTHR */
        {
            rc = QAT_Response_Str(QAT_RC_OK,
                                  "AT+BMISSTHR=<bmiss_threshold: 0~255>\r\n"
                                  "AT+BMISSTHR?: get BMISSTHR");
            break;
        }
        case QAT_OP_EXEC_W_PARAM: /* AT+BMISSTHR= */
        {
            memset((void *)buffer, 0, CMD_STR_BUFFER_LENGTH);

            if (!pg_wifi_shell_cxt->wlan_enabled) {
                /* edcca should be set after connectting */
                rc = QAT_Response_Str(QAT_RC_ERROR, "+BMISSTHR:wlan is not enabled");
                return rc;
            }

            if (Parameter_Count < 1 || !Parameter_List || !Parameter_List[0].Integer_Is_Valid) {
                return rc = QAT_Response_Str(QAT_RC_ERROR, "+BMISSTHR:AT+BMISSTHR=<bmiss_threshold: 0~255>");
            }

            if (Parameter_List[0].Integer_Value > UINT8_MAX || Parameter_List[0].Integer_Value < 0) {
                return rc = QAT_Response_Str(QAT_RC_ERROR, "+BMISSTHR:AT+BMISSTHR=<bmiss_threshold: 0~255>");
            }

            bmiss_threshold = Parameter_List[0].Integer_Value;

            if (0 != qapi_WLAN_Set_Param(deviceId, __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                                         __QAPI_WLAN_PARAM_GROUP_WIRELESS_STA_BMISS_CONFIG, &bmiss_threshold,
                                         sizeof(bmiss_threshold), FALSE)) {
                return QAT_Response_Str(
                    QAT_RC_ERROR, "+BMISSTHR:set bmiss threshold fail, check the wlan connection or data validation");
            }
            rc = QAT_Response_Str(QAT_RC_OK, NULL);
            break;
        }
        case QAT_OP_QUERY: /* AT+BMISSTHR? */
        {
            uint32_t length = 0;

            if (!pg_wifi_shell_cxt->wlan_enabled) {
                /* edcca should be set after connectting */
                rc = QAT_Response_Str(QAT_RC_ERROR, "+BMISSTHR:wlan is not enabled");
                return rc;
            }

            length = sizeof(bmiss_threshold);
            if (QAPI_OK != qapi_WLAN_Get_Param(deviceId, __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                                               __QAPI_WLAN_PARAM_GROUP_WIRELESS_STA_BMISS_CONFIG, &bmiss_threshold,
                                               &length)) {
                offset = snprintf(buffer + offset, CMD_STR_BUFFER_LENGTH - offset,
                                  "+BMISSTHR: get bmiss threshold fail for device %d\n", deviceId);
                return QAT_Response_Str(QAT_RC_ERROR, buffer);
            } else {
                offset = snprintf(buffer + offset, CMD_STR_BUFFER_LENGTH - offset, "+BMISSTHR:%d", bmiss_threshold);
                rc = QAT_Response_Str(QAT_RC_OK, buffer);
            }
            break;
        }

        default:;
    }

    return rc;
}

#ifdef CONFIG_WPS
/**
   @brief WPS

   This command will enable and start WPS PBC connect, or disable WPS PBC.

   @param[in] Op_Type          The input command type.
   @param[in] Parameter_Count  Number of parameters that were entered into the
                               command line.
   @param[in] Parameter_List   List of parameters entered into the command line.
*/

static QAT_Command_Status_t Extend_Command_WPS(uint32_t Op_Type, uint32_t Parameter_Count,
                                               QAT_Parameter_t *Parameter_List)
{
    QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
    uint8_t auth_floor = 0;
    uint8_t wps_enable = 0;
    uint8_t deviceId = qat_get_active_device();
    wifi_shell_cxt_t *p_cxt = pg_wifi_shell_cxt;
    wps_context_t wps_context;
    char buffer[WLAN_RESPONSE_BUFFER_LENGTH] = {0};
    qapi_Status_t ret = QAPI_OK;
    uint8_t wps_mode = 0;
    char wps_pin[32];

    if (0 == p_cxt->wlan_enabled) {
        QAT_Response_Str(QAT_RC_ERROR, "+WPS:Enable WLAN before get the WLAN infomation");
        return rc;
    }

    switch (Op_Type) {
        case QAT_OP_EXEC_W_PARAM: /* AT+WPS=0/1 */
        {
            if (Parameter_Count < 1 || !Parameter_List) {
                QAT_Response_Str(QAT_RC_ERROR, NULL);
                return rc;
            }

            wps_enable = Parameter_List[0].Integer_Value;
            if (wps_enable != 0 && wps_enable != 1) {
                QAT_Response_Str(QAT_RC_ERROR, NULL);
                return rc;
            }
            if (Parameter_Count == 2) {
                auth_floor = Parameter_List[1].Integer_Value;
            }

            wps_mode = QAPI_WLAN_WPS_PBC_MODE_E;
            wps_context.connect_flag = 1;
            memset(wps_pin, 0, 32);

            if (wps_enable == 1) {
                if (qapi_WLAN_Start_Wps(deviceId, wps_context.connect_flag, wps_mode, wps_pin, auth_floor) != 0) {
                    snprintf(buffer, WLAN_RESPONSE_BUFFER_LENGTH, "+WPS:WPS failed\r\n");
                    rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
                    return rc;
                }
                p_cxt->wps_stage = WPS_SCAN;
                wps_context.wps_in_progress = true;
            } else {
                qapi_WLAN_Stop_Wps(deviceId, p_cxt->wps_stage);
                p_cxt->wps_stage = WPS_NONE;
                wps_context.wps_in_progress = false;
            }
        }
        default:;
    }
    rc = QAT_Response_Str(QAT_RC_OK, NULL);
    return rc;
}
#endif

/*-------------------------------------------------------------------------
 * AT+CWSAVE — persist WiFi credentials to /lfs/wifi.conf
 *-----------------------------------------------------------------------*/
static QAT_Command_Status_t Extend_Command_CWSave(uint32_t Op_Type, uint32_t Parameter_Count,
                                                  QAT_Parameter_t *Parameter_List)
{
    (void)Parameter_Count;
    (void)Parameter_List;

    if (Op_Type != QAT_OP_EXEC) {
        return QAT_STATUS_SUCCESS_E;
    }

    wifi_shell_cxt_t *p_cxt = pg_wifi_shell_cxt;

    if (!p_cxt->ssid_length) {
        QAT_Response_Str(QAT_RC_ERROR, "+CWSAVE: no credentials to save");
        return QAT_STATUS_SUCCESS_E;
    }

    if (p_cxt->ssid_length > __QAPI_WLAN_MAX_SSID_LEN) {
        QAT_Response_Str(QAT_RC_ERROR, "+CWSAVE: SSID length invalid");
        return QAT_STATUS_SUCCESS_E;
    }

    if (!is_fs_mounted()) {
        QAT_Response_Str(QAT_RC_ERROR, "+CWSAVE: filesystem not mounted");
        return QAT_STATUS_SUCCESS_E;
    }

    wifi_cred_t cred;
    memset(&cred, 0, sizeof(cred));
    memcpy(cred.magic, WIFI_CONF_MAGIC, 4);
    cred.version        = WIFI_CONF_VERSION;
    cred.ssid_len       = (uint8_t)p_cxt->ssid_length;
    cred.passphrase_len = (uint8_t)strlen(p_cxt->passphrase);
    cred.auth_mode      = (uint32_t)p_cxt->auth;
    cred.cipher         = p_cxt->cipher;
    memscpy(cred.ssid, sizeof(cred.ssid), p_cxt->ssid, cred.ssid_len);
    memscpy(cred.passphrase, sizeof(cred.passphrase), p_cxt->passphrase, cred.passphrase_len);

    struct fs_file_t fp;
    fs_file_t_init(&fp);
    int ret = vfs_open(&fp, WIFI_CONF_PATH, FS_O_CREATE | FS_O_WRITE);
    if (ret < 0) {
        char buf[48];
        snprintf(buf, sizeof(buf), "+CWSAVE: open failed (%d)", ret);
        QAT_Response_Str(QAT_RC_ERROR, buf);
        return QAT_STATUS_SUCCESS_E;
    }
    vfs_truncate(&fp, 0);
    int32_t written = vfs_write(&fp, &cred, sizeof(cred));
    vfs_close(&fp);
    if (written != (int32_t)sizeof(cred)) {
        char buf[48];
        snprintf(buf, sizeof(buf), "+CWSAVE: write failed (%d)", (int)written);
        QAT_Response_Str(QAT_RC_ERROR, buf);
        return QAT_STATUS_SUCCESS_E;
    }

    QAT_Response_Str(QAT_RC_OK, NULL);
    return QAT_STATUS_SUCCESS_E;
}

/*-------------------------------------------------------------------------
 * Helper: load WiFi credentials from /lfs/wifi.conf into context + QAPI.
 * Returns 0 on success; -1 no fs, -2 no file, -3 I/O error, -4 corrupt, -5 bad len.
 *-----------------------------------------------------------------------*/
static int load_wifi_cred_from_flash(wifi_shell_cxt_t *p_cxt, uint8_t deviceId)
{
    if (!is_fs_mounted()) {
        return -1;
    }

    wifi_cred_t cred;
    struct fs_file_t fp;
    fs_file_t_init(&fp);
    int ret = vfs_open(&fp, WIFI_CONF_PATH, FS_O_READ);
    if (ret < 0) {
        return -2;
    }

    int32_t nread = vfs_read(&fp, &cred, sizeof(cred));
    vfs_close(&fp);

    if (nread != (int32_t)sizeof(cred)) {
        return -3;
    }
    if (memcmp(cred.magic, WIFI_CONF_MAGIC, 4) != 0 || cred.version != WIFI_CONF_VERSION) {
        return -4;
    }
    if (cred.ssid_len > __QAPI_WLAN_MAX_SSID_LEN || cred.passphrase_len > 64) {
        return -5;
    }

    memscpy(p_cxt->ssid, sizeof(p_cxt->ssid), cred.ssid, cred.ssid_len);
    p_cxt->ssid[cred.ssid_len] = '\0';
    p_cxt->ssid_length = cred.ssid_len;
    memscpy(p_cxt->passphrase, sizeof(p_cxt->passphrase), cred.passphrase, cred.passphrase_len);
    p_cxt->passphrase[cred.passphrase_len] = '\0';
    p_cxt->auth   = (qapi_WLAN_Auth_Mode_e)cred.auth_mode;
    p_cxt->cipher = cred.cipher;

    qapi_WLAN_Auth_Mode_e  auth_mode = (qapi_WLAN_Auth_Mode_e)cred.auth_mode;
    qapi_WLAN_Crypt_Type_e cipher    = (qapi_WLAN_Crypt_Type_e)cred.cipher;

    qapi_WLAN_Set_Param(deviceId, __QAPI_WLAN_PARAM_GROUP_WIRELESS_SECURITY,
                        __QAPI_WLAN_PARAM_GROUP_SECURITY_AUTH_MODE,
                        (void *)&auth_mode, sizeof(auth_mode), FALSE);
    qapi_WLAN_Set_Param(deviceId, __QAPI_WLAN_PARAM_GROUP_WIRELESS_SECURITY,
                        __QAPI_WLAN_PARAM_GROUP_SECURITY_ENCRYPTION_TYPE,
                        (void *)&cipher, sizeof(cipher), FALSE);
    qapi_WLAN_Set_Param(deviceId, __QAPI_WLAN_PARAM_GROUP_WIRELESS_SECURITY,
                        __QAPI_WLAN_PARAM_GROUP_SECURITY_PASSPHRASE,
                        (void *)p_cxt->passphrase, cred.passphrase_len, FALSE);
    qapi_WLAN_Set_Param(deviceId, __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                        __QAPI_WLAN_PARAM_GROUP_WIRELESS_SSID,
                        (void *)p_cxt->ssid, cred.ssid_len, FALSE);
    return 0;
}

/*-------------------------------------------------------------------------
 * AT+CWLOAD — restore WiFi credentials from /lfs/wifi.conf
 *-----------------------------------------------------------------------*/
static QAT_Command_Status_t Extend_Command_CWLoad(uint32_t Op_Type, uint32_t Parameter_Count,
                                                  QAT_Parameter_t *Parameter_List)
{
    (void)Parameter_Count;
    (void)Parameter_List;

    if (Op_Type != QAT_OP_EXEC) {
        return QAT_STATUS_SUCCESS_E;
    }

    wifi_shell_cxt_t *p_cxt = pg_wifi_shell_cxt;
    uint8_t deviceId = qat_get_active_device();

    int ret = load_wifi_cred_from_flash(p_cxt, deviceId);
    if (ret < 0) {
        const char *msg =
            (ret == -1) ? "+CWLOAD: filesystem not mounted" :
            (ret == -2) ? "+CWLOAD: no saved credentials"   :
            (ret == -5) ? "+CWLOAD: file corrupt (len)"     :
                          "+CWLOAD: file corrupt";
        QAT_Response_Str(QAT_RC_ERROR, msg);
        return QAT_STATUS_SUCCESS_E;
    }

    char response[64];
    snprintf(response, sizeof(response), "+CWLOAD: loaded %s", p_cxt->ssid);
    QAT_Response_Str(QAT_RC_QUIET, response);
    QAT_Response_Str(QAT_RC_OK, NULL);
    return QAT_STATUS_SUCCESS_E;
}

void Initialize_QAT_Wlan_Demo(void)
{
    qbool_t RetVal;

    pg_wifi_shell_cxt = &g_wifi_shell_cxt;
    wifi_shell_cxt_t *p_cxt = pg_wifi_shell_cxt;
    memset(&g_wifi_shell_cxt, 0, sizeof(wifi_shell_cxt_t));
    qurt_mutex_create(&p_cxt->wifi_shell_cxt_mutex);
    pg_wifi_shell_cxt->auth = QAPI_WLAN_AUTH_NONE_E;

    RetVal = QAT_Register_Command_Group(QAT_Wifi_Command_List, WIFI_COMMAND_LIST_SIZE);
    if (RetVal == false) {
        QAT_Response_Str(QAT_RC_ERROR, "Failed to register common command group.");
    }
}
