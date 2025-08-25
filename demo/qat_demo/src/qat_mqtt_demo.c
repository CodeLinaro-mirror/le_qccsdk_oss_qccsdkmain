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
#include "qapi_heap_status.h"
#include "qat.h"
#include "qat_api.h"
#include "qurt_internal.h"
#include "fcntl.h"
#include "nt_osal.h"
#include "qurt_mutex.h"
#include "wifi_fw_version.h"
#include "wifi_fw_pmu_ts_cfg.h"
#include "mqtt_client_demo.h"

#include "qapi_status.h"
#include "qapi_console.h"

#include "qcli_api.h"

/* MQTT API headers. */
#include "core_mqtt.h"
#include "core_mqtt_state.h"

/*Include backoff algorithm header for retry logic.*/
#include "backoff_algorithm.h"

/*-------------------------------------------------------------------------
 * Function Declarations
 *-----------------------------------------------------------------------*/
static QAT_Command_Status_t Extend_Command_MqttLongClientID(uint32_t Op_Type, uint32_t Parameter_Count,
                                                            QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_MqttLongUserName(uint32_t Op_Type, uint32_t Parameter_Count,
                                                            QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_MqttLongPassword(uint32_t Op_Type, uint32_t Parameter_Count,
                                                            QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_MqttInit(uint32_t Op_Type, uint32_t Parameter_Count,
                                                    QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_MqttAlpn(uint32_t Op_Type, uint32_t Parameter_Count,
                                                    QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_MqttSni(uint32_t Op_Type, uint32_t Parameter_Count,
                                                   QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_MqttConn(uint32_t Op_Type, uint32_t Parameter_Count,
                                                    QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_MqttPub(uint32_t Op_Type, uint32_t Parameter_Count,
                                                   QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_MqttPubRaw(uint32_t Op_Type, uint32_t Parameter_Count,
                                                      QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_MqttSub(uint32_t Op_Type, uint32_t Parameter_Count,
                                                   QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_MqttUnSub(uint32_t Op_Type, uint32_t Parameter_Count,
                                                     QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_MqttDisconnect(uint32_t Op_Type, uint32_t Parameter_Count,
                                                          QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_MqttDestroy(uint32_t Op_Type, uint32_t Parameter_Count,
                                                       QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_MqttMode(uint32_t Op_Type, uint32_t Parameter_Count,
                                                    QAT_Parameter_t *Parameter_List);

/* The following is the complete command list for the QAT common command demo. */
/** List of global commands that are supported when in a group. */
static QAT_Command_t QAT_MQTT_Command_List[] = {
    {"+MQTTINIT", Extend_Command_MqttInit, QAT_OP_EXEC | QAT_OP_EXEC_W_PARAM},
    // {"+MQTTLONGCLIENTID",   Extend_Command_MqttLongClientID,   QAT_OP_EXEC|QAT_OP_EXEC_W_PARAM},
    // {"+MQTTLONGUSERNAME",   Extend_Command_MqttLongUserName,   QAT_OP_EXEC|QAT_OP_EXEC_W_PARAM},
    // {"+MQTTLONGPASSWORD",   Extend_Command_MqttLongPassword,   QAT_OP_EXEC|QAT_OP_EXEC_W_PARAM},
    // {"+MQTTALPN",   Extend_Command_MqttAlpn,   QAT_OP_EXEC|QAT_OP_EXEC_W_PARAM},
    // {"+MQTTSNI",   Extend_Command_MqttSni,   QAT_OP_EXEC|QAT_OP_EXEC_W_PARAM},
    {"+MQTTCONN", Extend_Command_MqttConn, QAT_OP_EXEC | QAT_OP_QUERY | QAT_OP_EXEC_W_PARAM},
    {"+MQTTPUB", Extend_Command_MqttPub, QAT_OP_EXEC | QAT_OP_EXEC_W_PARAM},
    {"+MQTTPUBRAW", Extend_Command_MqttPubRaw, QAT_OP_EXEC | QAT_OP_EXEC_W_PARAM},
    {"+MQTTSUB", Extend_Command_MqttSub, QAT_OP_EXEC | QAT_OP_QUERY | QAT_OP_EXEC_W_PARAM},
    {"+MQTTUNSUB", Extend_Command_MqttUnSub, QAT_OP_EXEC | QAT_OP_EXEC_W_PARAM},
    {"+MQTTDISCONN", Extend_Command_MqttDisconnect, QAT_OP_EXEC | QAT_OP_EXEC_W_PARAM},
    {"+MQTTDESTROY", Extend_Command_MqttDestroy, QAT_OP_EXEC | QAT_OP_EXEC_W_PARAM},
    {"+MQTTMODE", Extend_Command_MqttMode, QAT_OP_EXEC | QAT_OP_EXEC_W_PARAM},
};

/*-------------------------------------------------------------------------
 * External parameters
 *-----------------------------------------------------------------------*/
extern HTC_Context_t HTC_Context;

/*-------------------------------------------------------------------------
 * Parameters define
 *-----------------------------------------------------------------------*/

#define MQTT_COMMAND_LIST_SIZE (sizeof(QAT_MQTT_Command_List) / sizeof(QAT_Command_t))

#define VERSION_STR_BUFFER_LENGTH 256
#define INFO_STR_BUFFER_LENGTH    256
#define MQTT_STR_BUFFER_LENGTH    128
#define CMD_STR_BUFFER_LENGTH     1024

/*-------------------------------------------------------------------------
 * Preprocessor Definitions, Constants, and Type Declarations
 *-----------------------------------------------------------------------*/

#define MQTT_CLIENT_PRINTF(...) printf(__VA_ARGS__)

/*-------------------------------------------------------------------------
 * Function Definitions
 *-----------------------------------------------------------------------*/

qbool_t QAT_Data_Transfer_Mode_Handle(uint32_t Length, uint8_t *Buffer);
uint8_t isRecvHex = 0;

static QAT_Command_Status_t Extend_Command_MqttDestroy(uint32_t Op_Type, uint32_t Parameter_Count,
                                                       QAT_Parameter_t *Parameter_List)
{
    char *buffer_test;
    QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
    qapi_Status_t result = QAPI_OK;
    uint32_t sessionIndex = 0;
    qbool_t isIntegerValid = false;
    char buffer[MQTT_STR_BUFFER_LENGTH];

    switch (Op_Type) {
        case QAT_OP_EXEC: /* AT+WRTMEM */
        {
            snprintf(buffer, MQTT_STR_BUFFER_LENGTH, "+MQTTDESTROY=<session_id>\r\n");
            rc = QAT_Response_Str(QAT_RC_OK, buffer);
            memset((void *)buffer, 0, MQTT_STR_BUFFER_LENGTH);
            break;
        }

        case QAT_OP_EXEC_W_PARAM: /* AT+WRTMEM */
        {
            sessionIndex = Parameter_List[0].Integer_Value;
            isIntegerValid = Parameter_List[0].Integer_Is_Valid;

            if (isIntegerValid == false || sessionIndex >= MQTT_DEMO_SESSION_NUM) {
                snprintf(buffer, MQTT_STR_BUFFER_LENGTH, "Invalid session id %s\n", Parameter_List[0].String_Value);
                rc = QAT_Response_Str(QAT_RC_ERROR, buffer);

            } else {
                result = mqttc_destroy(sessionIndex);
                if (result == QAPI_OK) {
                    rc = QAT_Response_Str(QAT_RC_OK, NULL);
                } else {
                    snprintf(buffer, MQTT_STR_BUFFER_LENGTH, "+MQTTCONN:FAIL,%d\r\n", result);
                    rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
                }
            }

            memset((void *)buffer, 0, MQTT_STR_BUFFER_LENGTH);
            break;
        }

        default:;
    }

    return rc;
}

static QAT_Command_Status_t Extend_Command_MqttMode(uint32_t Op_Type, uint32_t Parameter_Count,
                                                    QAT_Parameter_t *Parameter_List)
{
    QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
    qapi_Status_t result = QAPI_OK;
    char buffer[MQTT_STR_BUFFER_LENGTH];

    switch (Op_Type) {
        case QAT_OP_EXEC: /* AT+MQTTMODE */
        {
            snprintf(buffer, MQTT_STR_BUFFER_LENGTH, "+MQTTMODE=<0|1: string|hex>");
            rc = QAT_Response_Str(QAT_RC_OK, buffer);
            memset((void *)buffer, 0, MQTT_STR_BUFFER_LENGTH);
            break;
        }

        case QAT_OP_EXEC_W_PARAM: /* AT+MQTTMODE */
        {
            if ((Parameter_List[0].Integer_Value != 0) && (Parameter_List[0].Integer_Value != 1)) {
                rc = QAT_Response_Str(QAT_RC_ERROR, "Invalid parameter!");
            } else {
                isRecvHex = Parameter_List[0].Integer_Value;
                rc = QAT_Response_Str(QAT_RC_OK, NULL);
            }

            break;
        }

        default:;
    }

    return rc;
}

static QAT_Command_Status_t Extend_Command_MqttAlpn(uint32_t Op_Type, uint32_t Parameter_Count,
                                                    QAT_Parameter_t *Parameter_List)
{
    QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
    return rc;
}

static QAT_Command_Status_t Extend_Command_MqttSni(uint32_t Op_Type, uint32_t Parameter_Count,
                                                   QAT_Parameter_t *Parameter_List)
{
    QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
    return rc;
}

static QAT_Command_Status_t Extend_Command_MqttConn(uint32_t Op_Type, uint32_t Parameter_Count,
                                                    QAT_Parameter_t *Parameter_List)
{
    char *buffer_test;
    QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
    qapi_Status_t result = QAPI_OK;
    uint32_t sessionIndex = 0;
    qbool_t isIntegerValid = false;
    char buffer[MQTT_STR_BUFFER_LENGTH];

    switch (Op_Type) {
        case QAT_OP_EXEC: /* AT+WRTMEM */
        {
            snprintf(buffer, MQTT_STR_BUFFER_LENGTH,
                     "+MQTTCONN=<session_id>,<host>,<port>,<clientid>,<username>,<password>,<keepalive>,<clean "
                     "session>\r\n");
            rc = QAT_Response_Str(QAT_RC_OK, buffer);

            break;
        }

        case QAT_OP_EXEC_W_PARAM: /* AT+WRTMEM */
        {
            sessionIndex = Parameter_List[0].Integer_Value;
            isIntegerValid = Parameter_List[0].Integer_Is_Valid;

            if (isIntegerValid == false || sessionIndex >= MQTT_DEMO_SESSION_NUM) {
                snprintf(buffer, MQTT_STR_BUFFER_LENGTH, "+MQTTCONN:Invalid session id %s\n",
                         Parameter_List[0].String_Value);
                rc = QAT_Response_Str(QAT_RC_ERROR, buffer);

            } else {
                result = mqttc_connect(Parameter_Count, (QAPI_Console_Parameter_t *)Parameter_List);
                if (result == QAPI_OK) {
                    rc = QAT_Response_Str(QAT_RC_OK, NULL);
                } else {
                    snprintf(buffer, MQTT_STR_BUFFER_LENGTH, "+MQTTCONN:FAIL,%d\r\n", result);
                    rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
                }
            }
            break;
        }

        case QAT_OP_QUERY: /* AT+WRTMEM */
        {
            mqttc_connect_info_query();
            QAT_Response_Str(QAT_RC_OK, NULL);
            break;
        }

        default:;
    }

    return rc;
}

static QAT_Command_Status_t Extend_Command_MqttPub(uint32_t Op_Type, uint32_t Parameter_Count,
                                                   QAT_Parameter_t *Parameter_List)
{
    char *buffer_test;
    QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
    qapi_Status_t result = QAPI_OK;
    uint32_t sessionIndex = 0;
    qbool_t isIntegerValid = false;
    char buffer[MQTT_STR_BUFFER_LENGTH];

    switch (Op_Type) {
        case QAT_OP_EXEC: /* AT+WRTMEM */
        {
            snprintf(buffer, MQTT_STR_BUFFER_LENGTH,
                     "+MQTTPUB=<session_id>,<\"topic\">,<qos_level>,<\"message\">,<retain>\r\n");
            rc = QAT_Response_Str(QAT_RC_OK, buffer);
            break;
        }

        case QAT_OP_EXEC_W_PARAM: /* AT+WRTMEM */
        {
            sessionIndex = Parameter_List[0].Integer_Value;
            isIntegerValid = Parameter_List[0].Integer_Is_Valid;

            if (isIntegerValid == false || sessionIndex >= MQTT_DEMO_SESSION_NUM) {
                snprintf(buffer, MQTT_STR_BUFFER_LENGTH, "Invalid session id %s\n", Parameter_List[0].String_Value);
                rc = QAT_Response_Str(QAT_RC_ERROR, buffer);

            } else {
                result = mqttc_publish(Parameter_Count, (QAPI_Console_Parameter_t *)Parameter_List);
                if (result == QAPI_OK) {
                    rc = QAT_Response_Str(QAT_RC_OK, NULL);
                } else {
                    snprintf(buffer, MQTT_STR_BUFFER_LENGTH, "+MQTTPUB:FAIL,%d\r\n", result);
                    rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
                }
            }

            break;
        }

        default:;
    }

    return rc;
}

static QAT_Command_Status_t Extend_Command_MqttPubRaw(uint32_t Op_Type, uint32_t Parameter_Count,
                                                      QAT_Parameter_t *Parameter_List)
{
    char *buffer_test;
    QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
    qapi_Status_t result = QAPI_OK;
    uint32_t sessionIndex = 0;
    qbool_t isIntegerValid = false;
    char buffer[MQTT_STR_BUFFER_LENGTH];

    switch (Op_Type) {
        case QAT_OP_EXEC: /* AT+WRTMEM */
        {
            snprintf(buffer, MQTT_STR_BUFFER_LENGTH,
                     "+MQTTPUBRAW=<session_id>,<\"topic\">,<length>,<qos_level>,<retain>\r\n");
            rc = QAT_Response_Str(QAT_RC_OK, buffer);
            memset((void *)buffer, 0, MQTT_STR_BUFFER_LENGTH);
            break;
        }

        case QAT_OP_EXEC_W_PARAM: /* AT+WRTMEM */
        {
            sessionIndex = Parameter_List[0].Integer_Value;
            isIntegerValid = Parameter_List[0].Integer_Is_Valid;

            if (Parameter_Count < 3) {
                snprintf(buffer, MQTT_STR_BUFFER_LENGTH, "Invalid Parameter Count %d\n", Parameter_Count);
                rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
            } else if (isIntegerValid == false || sessionIndex >= MQTT_DEMO_SESSION_NUM) {
                snprintf(buffer, MQTT_STR_BUFFER_LENGTH, "Invalid session id %s\n", Parameter_List[0].String_Value);
                rc = QAT_Response_Str(QAT_RC_ERROR, buffer);

            } else {
                // cache parameter for pub in data model
                result = mqttc_publishRaw_Cache(Parameter_Count, (QAPI_Console_Parameter_t *)Parameter_List);
                if (result == QAPI_OK) {
                    rc = QAT_Response_Str(QAT_RC_OK, NULL);

                    extern Cur_Data_Mode_Cmd_t Cur_Data_Mode_Cmd;
                    memcpy(Cur_Data_Mode_Cmd.cur_data_mode_commnd, "+MQTTPUBRAW", strlen("+MQTTPUBRAW"));

                    QAT_Transfer_Mode_set(QAT_Transfer_Mode_ONLINE_DATA_E, QAT_Data_Transfer_Mode_Handle);
                    QAT_Response_Str(QAT_RC_OK, NULL);
                    snprintf(buffer, MQTT_STR_BUFFER_LENGTH, ">\r\n");
                    QAT_Response_Str(QAT_RC_QUIET_NO_CR, buffer);

                } else {
                    snprintf(buffer, MQTT_STR_BUFFER_LENGTH, "+MQTTPUBRAW:FAIL,%d\r\n", result);
                    rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
                }
            }

            memset((void *)buffer, 0, MQTT_STR_BUFFER_LENGTH);
            break;
        }

        case QAT_OP_EXEC_IN_DATA_MODEL: {
            uint32_t len = Parameter_Count;
            char *pubBuffer = (char *)Parameter_List;

            result = mqttc_publishRaw_Block(len, pubBuffer);
            if (result == QAPI_OK) {
                // rc = QAT_Response_Str(QAT_RC_OK, NULL);
            } else {
                snprintf(buffer, MQTT_STR_BUFFER_LENGTH, "+MQTTPUBRAW:FAIL,%d\r\n", result);
                rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
            }

            break;
        }

        default:;
    }

    return rc;
}

static QAT_Command_Status_t Extend_Command_MqttSub(uint32_t Op_Type, uint32_t Parameter_Count,
                                                   QAT_Parameter_t *Parameter_List)
{
    char *buffer_test;
    QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
    qapi_Status_t result = QAPI_OK;
    uint32_t sessionIndex = 0;
    qbool_t isIntegerValid = false;
    char buffer[MQTT_STR_BUFFER_LENGTH];

    switch (Op_Type) {
        case QAT_OP_QUERY: {
            mqttc_sub_info_query();
            QAT_Response_Str(QAT_RC_OK, NULL);
            break;
        }

        case QAT_OP_EXEC: /* AT+WRTMEM */
        {
            snprintf(buffer, MQTT_STR_BUFFER_LENGTH, "+MQTTSUB=<session_id>,<\"topic\">,<requested_qos>\r\n");
            rc = QAT_Response_Str(QAT_RC_OK, buffer);
            memset((void *)buffer, 0, MQTT_STR_BUFFER_LENGTH);
            break;
        }

        case QAT_OP_EXEC_W_PARAM: /* AT+WRTMEM */
        {
            sessionIndex = Parameter_List[0].Integer_Value;
            isIntegerValid = Parameter_List[0].Integer_Is_Valid;

            if (isIntegerValid == false || sessionIndex >= MQTT_DEMO_SESSION_NUM) {
                snprintf(buffer, MQTT_STR_BUFFER_LENGTH, "Invalid session id %s\n", Parameter_List[0].String_Value);
                rc = QAT_Response_Str(QAT_RC_ERROR, buffer);

            } else {
                result = mqttc_subscribe(Parameter_Count, (QAPI_Console_Parameter_t *)Parameter_List);
                if (result == QAPI_OK) {
                    rc = QAT_Response_Str(QAT_RC_OK, NULL);
                } else {
                    snprintf(buffer, MQTT_STR_BUFFER_LENGTH, "+MQTTSUB:FAIL,%d\r\n", result);
                    rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
                }
            }

            memset((void *)buffer, 0, MQTT_STR_BUFFER_LENGTH);

            break;
        }

        default:;
    }

    return rc;
}

static QAT_Command_Status_t Extend_Command_MqttUnSub(uint32_t Op_Type, uint32_t Parameter_Count,
                                                     QAT_Parameter_t *Parameter_List)
{
    char *buffer_test;
    QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
    qapi_Status_t result = QAPI_OK;
    uint32_t sessionIndex = 0;
    qbool_t isIntegerValid = false;
    char buffer[MQTT_STR_BUFFER_LENGTH];

    switch (Op_Type) {
        case QAT_OP_EXEC: /* AT+WRTMEM */
        {
            snprintf(buffer, MQTT_STR_BUFFER_LENGTH, "+MQTTUNSUB=<session_id>,<\"topic\">\r\n");
            rc = QAT_Response_Str(QAT_RC_OK, buffer);
            memset((void *)buffer, 0, MQTT_STR_BUFFER_LENGTH);
            break;
        }

        case QAT_OP_EXEC_W_PARAM: /* AT+WRTMEM */
        {
            sessionIndex = Parameter_List[0].Integer_Value;
            isIntegerValid = Parameter_List[0].Integer_Is_Valid;

            if (isIntegerValid == false || sessionIndex >= MQTT_DEMO_SESSION_NUM) {
                snprintf(buffer, MQTT_STR_BUFFER_LENGTH, "Invalid session id %s\n", Parameter_List[0].String_Value);
                rc = QAT_Response_Str(QAT_RC_ERROR, buffer);

            } else {
                result = mqttc_unsubscribe(Parameter_Count, (QAPI_Console_Parameter_t *)Parameter_List);
                if (result == QAPI_OK) {
                    rc = QAT_Response_Str(QAT_RC_OK, NULL);
                } else {
                    snprintf(buffer, MQTT_STR_BUFFER_LENGTH, "+MQTTUNSUB:FAIL,%d\r\n", result);
                    rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
                }
            }

            memset((void *)buffer, 0, MQTT_STR_BUFFER_LENGTH);
            break;
        }

        default:;
    }

    return rc;
}

static QAT_Command_Status_t Extend_Command_MqttDisconnect(uint32_t Op_Type, uint32_t Parameter_Count,
                                                          QAT_Parameter_t *Parameter_List)
{
    char *buffer_test;
    QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
    qapi_Status_t result = QAPI_OK;
    uint32_t sessionIndex = 0;
    qbool_t isIntegerValid = false;
    char buffer[MQTT_STR_BUFFER_LENGTH];

    switch (Op_Type) {
        case QAT_OP_EXEC: /* AT+WRTMEM */
        {
            snprintf(buffer, MQTT_STR_BUFFER_LENGTH, "+MQTTDISCONN=<session_id>\r\n");
            rc = QAT_Response_Str(QAT_RC_OK, buffer);
            memset((void *)buffer, 0, MQTT_STR_BUFFER_LENGTH);
            break;
        }

        case QAT_OP_EXEC_W_PARAM: /* AT+WRTMEM */
        {
            sessionIndex = Parameter_List[0].Integer_Value;
            isIntegerValid = Parameter_List[0].Integer_Is_Valid;

            if (isIntegerValid == false || sessionIndex >= MQTT_DEMO_SESSION_NUM) {
                snprintf(buffer, MQTT_STR_BUFFER_LENGTH, "Invalid session id %s\n", Parameter_List[0].String_Value);
                rc = QAT_Response_Str(QAT_RC_ERROR, buffer);

            } else {
                result = mqttc_disconnect(Parameter_Count, (QAPI_Console_Parameter_t *)Parameter_List);
                if (result == QAPI_OK) {
                    rc = QAT_Response_Str(QAT_RC_OK, NULL);
                } else {
                    snprintf(buffer, MQTT_STR_BUFFER_LENGTH, "+MQTTDISCONN:FAIL,%d\r\n", result);
                    rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
                }
            }

            memset((void *)buffer, 0, MQTT_STR_BUFFER_LENGTH);
            break;
        }

        default:;
    }

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
static QAT_Command_Status_t Extend_Command_MqttLongClientID(uint32_t Op_Type, uint32_t Parameter_Count,
                                                            QAT_Parameter_t *Parameter_List)
{
    heap_status hs;
    qapi_Time_t tm;
    char *buffer;
    QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;

    switch (Op_Type) {
        case QAT_OP_EXEC: /* AT+INFO */
        {
            buffer = malloc(VERSION_STR_BUFFER_LENGTH);

            if (!buffer) {
                QAT_Response_Str(QAT_RC_ERROR, NULL);
                return rc;
            }

            if (qapi_Heap_Status(&hs) != QAPI_OK) {
                QAT_Response_Str(QAT_RC_ERROR, NULL);
                return rc;
            }

            snprintf(buffer, VERSION_STR_BUFFER_LENGTH,
                     "Show system information\r\n\
		Temperature=%dC\r\nVbat=%dmV\r\n\
		get heap status\r\n\
		           total       used       free       min_free\r\n\
		Heap:   %8d   %8d   %8d       %8d\r\n",
                     pmu_ts_get_current_temperature(), tv_monitor_get_vbat_mV(), hs.total_Bytes,
                     hs.total_Bytes - hs.free_Bytes, hs.free_Bytes, hs.min_ever_free_bytes);

            rc = QAT_Response_Str(QAT_RC_OK, buffer);
            memset((void *)buffer, 0, VERSION_STR_BUFFER_LENGTH);
            free(buffer);
            buffer = NULL;

            break;
        }

        default:;
    }

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
static QAT_Command_Status_t Extend_Command_MqttLongUserName(uint32_t Op_Type, uint32_t Parameter_Count,
                                                            QAT_Parameter_t *Parameter_List)
{
    char *buffer;
    QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;

    switch (Op_Type) {
        case QAT_OP_EXEC: /* AT+RST */
        {
            rc = QAT_Response_Str(QAT_RC_OK, NULL);
            /*Wait for sending OK*/
            sleep(1);
            nt_system_sw_reset();
            break;
        }

        default:;
    }

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
static QAT_Command_Status_t Extend_Command_MqttLongPassword(uint32_t Op_Type, uint32_t Parameter_Count,
                                                            QAT_Parameter_t *Parameter_List)
{
    QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;

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
static QAT_Command_Status_t Extend_Command_MqttInit(uint32_t Op_Type, uint32_t Parameter_Count,
                                                    QAT_Parameter_t *Parameter_List)
{
    char *buffer_test;
    QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
    qapi_Status_t result = QAPI_OK;
    uint32_t sessionIndex = 0;
    qbool_t isIntegerValid = false;
    char buffer[MQTT_STR_BUFFER_LENGTH];
    switch (Op_Type) {
        case QAT_OP_EXEC: /* AT+WRTMEM */
        {
            snprintf(buffer, MQTT_STR_BUFFER_LENGTH,
                     "+MQTTINIT=<session_id>,<transport scheme>,<ca_file>,<cert_file>,<key_file>,<sni>,<alpn "
                     "protocol_name>\r\n");
            rc = QAT_Response_Str(QAT_RC_OK, buffer);
            memset((void *)buffer, 0, MQTT_STR_BUFFER_LENGTH);
            break;
        }

        case QAT_OP_EXEC_W_PARAM: /* AT+WRTMEM */
        {
            sessionIndex = Parameter_List[0].Integer_Value;
            isIntegerValid = Parameter_List[0].Integer_Is_Valid;

            if (isIntegerValid == false || sessionIndex >= MQTT_DEMO_SESSION_NUM) {
                snprintf(buffer, MQTT_STR_BUFFER_LENGTH, "+MQTTINIT:Invalid session id %s\n",
                         Parameter_List[0].String_Value);
                rc = QAT_Response_Str(QAT_RC_ERROR, buffer);

            } else {
                result = mqttc_init(Parameter_Count, (QAPI_Console_Parameter_t *)Parameter_List);
                if (result == QAPI_OK) {
                    rc = QAT_Response_Str(QAT_RC_OK, NULL);
                } else {
                    snprintf(buffer, MQTT_STR_BUFFER_LENGTH, "+MQTTINIT:FAIL,%d\r\n", result);
                    rc = QAT_Response_Str(QAT_RC_ERROR, buffer);
                }
            }

            memset((void *)buffer, 0, MQTT_STR_BUFFER_LENGTH);
            break;
        }

        default:;
    }

    return rc;
}

void Initialize_QAT_Mqtt_Demo(void)
{
    qbool_t RetVal;
    RetVal = QAT_Register_Command_Group(QAT_MQTT_Command_List, MQTT_COMMAND_LIST_SIZE);
    if (RetVal == false) {
        printf("Failed to register MQTT command group.\n");
    }
}
