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

#include "qapi_firmware_upgrade.h"
#include "qat.h"
#include "qat_api.h"
#include "qurt_internal.h"
#include "nt_osal.h"
#include "nt_socpm_sleep.h"
#include "qurt_mutex.h"
#include "at_web_server.h"
#include "ota_http.h"
#include "fw_upgrade.h"

/**********************************************************************************************************/
/* Type Declarations											                                          */
/**********************************************************************************************************/
/*-------------------------------------------------------------------------
 * Function Declarations
 *-----------------------------------------------------------------------*/
static QAT_Command_Status_t Extend_Command_OTA_FWD(uint32_t Op_Type, uint32_t Parameter_Count,
                                                   QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_OTA_FWUP(uint32_t Op_Type, uint32_t Parameter_Count,
                                                    QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_OTA_TRIAL(uint32_t Op_Type, uint32_t Parameter_Count,
                                                     QAT_Parameter_t *Parameter_List);

void QAT_display_sub_image_info(qapi_Part_Hdl_t hdl, uint8_t *buffer, int *offset);
static void fw_upgrade_HTTP_upgrade_task(void __attribute__((__unused__)) * pvParameters);

/* The following is the complete command list for the QAT common command demo. */
/** List of global commands that are supported when in a group. */
static QAT_Command_t QAT_OTA_Command_List[] = {
    //{"+OTAFWD",      Extend_Command_OTA_FWD,        QAT_OP_EXEC | QAT_OP_QUERY | QAT_OP_EXEC_W_PARAM},
    {"+OTAFWUP", Extend_Command_OTA_FWUP, QAT_OP_EXEC | QAT_OP_QUERY | QAT_OP_EXEC_W_PARAM},
    {"+OTATRIAL", Extend_Command_OTA_TRIAL, QAT_OP_EXEC | QAT_OP_QUERY | QAT_OP_EXEC_W_PARAM},
};
/*-------------------------------------------------------------------------
 * External parameters
 *-----------------------------------------------------------------------*/
extern HTC_Context_t HTC_Context;

/*-------------------------------------------------------------------------
 * Parameters define
 *-----------------------------------------------------------------------*/

#define OTA_COMMAND_LIST_SIZE (sizeof(QAT_OTA_Command_List) / sizeof(QAT_Command_t))

#define DISPLAY_FWD_BUFFER_LENGTH 1024

qat_fw_upgrade_params_t *qat_upgrade_params = NULL;
TaskHandle_t qat_fw_upgrade_task_handle = NULL;

/* @brief Register Power Event Mask as Enum */
typedef enum qat_pwr_evt {
    PWR_EVT_WMAC_PREPARE_TO_SLEEP = 1 << 1U,
    PWR_EVT_WMAC_FAIL_TO_SLEEP = 1 << 2U,
} qat_pwr_evt_t;

/*-------------------------------------------------------------------------
 * Function Definitions
 *-----------------------------------------------------------------------*/

void QAT_display_sub_image_info(qapi_Part_Hdl_t hdl, uint8_t *buffer, int *offset)
{
    uint32_t result = 0;

    qapi_Fw_Upgrade_Get_Image_ID(hdl, &result);
    *offset += snprintf(buffer + *(offset), DISPLAY_FWD_BUFFER_LENGTH - *(offset), ",0x%X", result);
    qapi_Fw_Upgrade_Get_Image_Version(hdl, &result);
    *offset += snprintf(buffer + *(offset), DISPLAY_FWD_BUFFER_LENGTH - *(offset), ",0x%X", result);
    qapi_Fw_Upgrade_Get_Partition_Start(hdl, &result);
    *offset += snprintf(buffer + *(offset), DISPLAY_FWD_BUFFER_LENGTH - *(offset), ",0x%X", result);
    qapi_Fw_Upgrade_Get_Partition_Size(hdl, &result);
    *offset += snprintf(buffer + *(offset), DISPLAY_FWD_BUFFER_LENGTH - *(offset), ",0x%X", result);
}
void qat_fw_upgrade_callback(int32_t state, int32_t status)
{
    QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
    char buffer[DISPLAY_FWD_BUFFER_LENGTH] = {0};
    uint32_t offset = 0;

    if (status != 0) {
        offset +=
            snprintf(buffer + offset, DISPLAY_FWD_BUFFER_LENGTH - offset, "+EVT:OTAFWUP_ERROR:%d,%d", state, status);
        rc = QAT_Response_Str(QAT_RC_QUIET_NO_CR, buffer);
        return;
    }

    if (state == FW_UPGRADE_STATE_NOT_START_E) {
        offset += snprintf(buffer + offset, DISPLAY_FWD_BUFFER_LENGTH - offset,
                           "+EVT:OTAFWUP_START: OTA started, please wait");
    } else if (state == FW_UPGRADE_STATE_CONNECT_SERVER_E) {
        // offset += snprintf(buffer + offset, DISPLAY_FWD_BUFFER_LENGTH - offset, "+EVT:OTAFWUP_INIT: Connecting to Server");
    } else if ((state == FW_UPGRADE_STATE_RECEIVE_DATA_E) || (state == FW_UPGRADE_STATE_PROCESS_CONFIG_FILE_E) ||
               (state == FW_UPGRADE_STATE_PROCESS_IMAGE_E)) {
        if (qat_upgrade_params->process_state_cnt == 0) {
            // offset += snprintf(buffer + offset, DISPLAY_FWD_BUFFER_LENGTH - offset, "+EVT:OTAFWUP_RUN: Firmware downloading, please wait...");
        } else if (qat_upgrade_params->process_state_cnt % 20 == 0) {
            // offset += snprintf(buffer + offset, DISPLAY_FWD_BUFFER_LENGTH - offset, ".");
        }
        qat_upgrade_params->process_state_cnt++;
    }
    /*
    else
    {
        offset += snprintf(buffer + offset, DISPLAY_FWD_BUFFER_LENGTH - offset, "+EVT:OTAFWUP_RUN: %d, %d", state, status);
    }
    */
    rc = QAT_Response_Str(QAT_RC_QUIET_NO_CR, buffer);
    return;
}

static void qat_fw_upgrade_free_memory()
{
    if (qat_upgrade_params != NULL) {
        if (qat_upgrade_params->interface_name != NULL) {
            free(qat_upgrade_params->interface_name);
            qat_upgrade_params->interface_name = NULL;
        }
        if (qat_upgrade_params->url != NULL) {
            free(qat_upgrade_params->url);
            qat_upgrade_params->url = NULL;
        }
        if (qat_upgrade_params->cfg_file != NULL) {
            free(qat_upgrade_params->cfg_file);
            qat_upgrade_params->cfg_file = NULL;
        }
        free(qat_upgrade_params);
        qat_upgrade_params = NULL;
    }
}

static void qat_fw_upgrade_HTTP_upgrade_task(void __attribute__((__unused__)) * pvParameters)
{
    qapi_Status_t resp_code;
    qapi_Fw_Upgrade_Plugin_t plugin = {plugin_http_init, plugin_http_recv_data, plugin_http_abort, plugin_http_resume,
                                       plugin_http_fin};
    char buffer[DISPLAY_FWD_BUFFER_LENGTH] = {0};
    int offset = 0;

    if (qat_upgrade_params == NULL) {
        goto http_thread_end;
    }
    resp_code = qapi_Fw_Upgrade(qat_upgrade_params->interface_name, &plugin, qat_upgrade_params->url,
                                qat_upgrade_params->cfg_file, qat_upgrade_params->flags, qat_fw_upgrade_callback,
                                qat_upgrade_params);

    if (QAPI_OK != resp_code) {
        offset += snprintf(buffer + offset, DISPLAY_FWD_BUFFER_LENGTH - offset,
                           "+EVT:OTAFWUP_ERROR: Firmware Upgrade Image Download Failed ERR:%d\r\n", resp_code);

        if (resp_code == QAPI_FW_UPGRADE_ERR_TRIAL_IS_RUNNING) {
            offset +=
                snprintf(buffer + offset, DISPLAY_FWD_BUFFER_LENGTH - offset,
                         "+EVT:OTAFWUP_ERROR: Trial Partition is running, need reboot to do Firmware Upgrade.\r\n");
        }
    } else {
        offset += snprintf(buffer + offset, DISPLAY_FWD_BUFFER_LENGTH - offset,
                           "+EVT:OTAFWUP_FIN: Firmware Upgrade Image Download Completed successfully\r\n");
    }
    QAT_Response_Str(QAT_RC_QUIET_NO_CR, buffer);
http_thread_end:
    qat_fw_upgrade_free_memory();
    vTaskDelete(NULL);
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
static QAT_Command_Status_t Extend_Command_OTA_FWD(uint32_t Op_Type, uint32_t Parameter_Count,
                                                   QAT_Parameter_t *Parameter_List)
{
    char buffer[DISPLAY_FWD_BUFFER_LENGTH] = {0};
    int offset = 0;
    QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
    int32_t Index;
    uint32_t boot_type, fwd_present, magic = 0, Result_u32 = 0;
    uint8_t Result_u8 = 0;
    qapi_Part_Hdl_t hdl = NULL, hdl_next;

    switch (Op_Type) {
        case QAT_OP_EXEC: {
            rc = QAT_Response_Str(QAT_RC_OK,
                                  "AT+OTAFWD: get usage of command\r\n"
                                  "AT+OTAFWD?: display FWD\r\n");
            break;
        }
        case QAT_OP_QUERY: /* AT+OTAFWD? */
        {
            if (qapi_Fw_Upgrade_init() != QAPI_OK) {
                QAT_Response_Str(QAT_RC_ERROR, NULL);
                return rc;
            }

            // get active FWD
            Index = qapi_Fw_Upgrade_Get_Active_FWD(&boot_type, &fwd_present);
            snprintf(buffer, DISPLAY_FWD_BUFFER_LENGTH, "+OTAFWD:%s,%d,%d",
                     (boot_type == QAPI_FW_UPGRADE_FWD_BOOT_TYPE_TRIAL)     ? "Trial"
                     : (boot_type == QAPI_FW_UPGRADE_FWD_BOOT_TYPE_CURRENT) ? "Current"
                                                                            : "Golden",
                     Index, fwd_present);

            rc = QAT_Response_Str(QAT_RC_QUIET_NO_CR, buffer);

            memset(buffer, 0, DISPLAY_FWD_BUFFER_LENGTH);

            for (Index = 0; Index < 3; Index++) {
                offset += snprintf(buffer + offset, DISPLAY_FWD_BUFFER_LENGTH - offset, "+OTAFWD:%d", Index);

                qapi_Fw_Upgrade_Get_FWD_Magic(Index, &magic);
                offset += snprintf(buffer + offset, DISPLAY_FWD_BUFFER_LENGTH - offset, ",0x%X", magic);

                qapi_Fw_Upgrade_Get_FWD_Rank(Index, &Result_u32);
                offset += snprintf(buffer + offset, DISPLAY_FWD_BUFFER_LENGTH - offset, ",0x%X", Result_u32);

                qapi_Fw_Upgrade_Get_FWD_Version(Index, &Result_u32);
                offset += snprintf(buffer + offset, DISPLAY_FWD_BUFFER_LENGTH - offset, ",0x%X", Result_u32);

                qapi_Fw_Upgrade_Get_FWD_Status(Index, &Result_u8);
                offset += snprintf(buffer + offset, DISPLAY_FWD_BUFFER_LENGTH - offset, ",0x%X", Result_u8);

                qapi_Fw_Upgrade_Get_FWD_Total_Images(Index, &Result_u8);
                offset += snprintf(buffer + offset, DISPLAY_FWD_BUFFER_LENGTH - offset, ",0x%X", Result_u8);

                if (magic == 0x54445746) {
                    if (qapi_Fw_Upgrade_First_Partition(Index, &hdl) == QAPI_OK) {
                        QAT_display_sub_image_info(hdl, buffer, &offset);
                        while (qapi_Fw_Upgrade_Next_Partition(hdl, &hdl_next) == QAPI_OK) {
                            qapi_Fw_Upgrade_Close_Partition(hdl);
                            hdl = hdl_next;
                            QAT_display_sub_image_info(hdl, buffer, &offset);
                        }
                        qapi_Fw_Upgrade_Close_Partition(hdl);
                    }
                }
                offset += snprintf(buffer + offset, DISPLAY_FWD_BUFFER_LENGTH - offset, "\r\n");
            }
            // rc = QAT_Response_Str(QAT_RC_QUIET_NO_CR, buffer);

            rc = QAT_Response_Str(QAT_RC_OK, buffer);

            break;
        }
        case QAT_OP_EXEC_W_PARAM: /* AT+OTAFWD= */
        {
            char *cmd = NULL;

            if (Parameter_Count != 2) {
                QAT_Response_Str(QAT_RC_ERROR, "+OTAFWD:usage: AT+OTAFWD=<\"del\">,<fwd num>");
                return rc;
            }
            cmd = Parameter_List[0].String_Value;
            if (strncasecmp(cmd, "del", 3) == 0) {
                if (Parameter_List[1].Integer_Is_Valid == 0) {
                    QAT_Response_Str(QAT_RC_ERROR, "+OTAFWD:usage: AT+OTAFWD=<\"del\">,<fwd num>");
                    return rc;
                }

                if (Parameter_List[0].Integer_Value > 2) {
                    QAT_Response_Str(QAT_RC_ERROR, "+OTAFWD:invalid fwd number");
                    return rc;
                }

                Index = Parameter_List[0].Integer_Value;

                if (qapi_Fw_Upgrade_init() != QAPI_OK) {
                    QAT_Response_Str(QAT_RC_ERROR, "+OTAFWD:FU Init Error");
                    return rc;
                }

                if (qapi_Fw_Upgrade_Erase_FWD(Index) != QAPI_OK) {
                    QAT_Response_Str(QAT_RC_ERROR, "+OTAFWD:Erase Error");
                    return rc;
                }
                snprintf(buffer, DISPLAY_FWD_BUFFER_LENGTH, "+OTAFWD:Delete FWD%d", Index);
                rc = QAT_Response_Str(QAT_RC_OK, buffer);
            } else {
                QAT_Response_Str(QAT_RC_ERROR, "+OTAFWD:usage: AT+OTAFWD=<\"del\">,<fwd num>");
                return rc;
            }
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
static QAT_Command_Status_t Extend_Command_OTA_FWUP(uint32_t Op_Type, uint32_t Parameter_Count,
                                                    QAT_Parameter_t *Parameter_List)
{
    char buffer[DISPLAY_FWD_BUFFER_LENGTH] = {0};
    int offset = 0;
    QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
    int32_t Index;
    uint32_t boot_type, fwd_present, magic = 0, Result_u32 = 0;
    uint8_t Result_u8 = 0;
    qapi_Part_Hdl_t hdl = NULL, hdl_next;

    switch (Op_Type) {
        case QAT_OP_EXEC: {
            rc = QAT_Response_Str(
                QAT_RC_OK,
                "AT+OTAFWUP: get usage of command\r\n"
                "AT+OTAFWUP=<protocol(only http now)>,<url>,<fw filename>,[flag],[timeout(in ms)]\r\n");
            break;
        }
        case QAT_OP_EXEC_W_PARAM: /* AT+OTAFWD= */
        {
            uint32_t interface_len;
            uint32_t url_len;
            uint32_t cfg_len;
            uint32_t flags = (QAPI_FW_UPGRADE_FLAG_AUTO_REBOOT | QAPI_FW_UPGRADE_FLAG_DUPLICATE_ACTIVE_FS | QAPI_FW_UPGRADE_FLAG_RANGE_HEADER);
            char *cmd = NULL;

            if (Parameter_Count > 6) {
                rc = QAT_Response_Str(
                    QAT_RC_ERROR,
                    "AT+OTAFWUP=<protocol(only http now)>,<url>,<fw filename>,[flag],[timeout(in ms)]\r\n");
                return rc;
            }

            if (Parameter_Count == 4 &&
                ((!Parameter_List[3].Integer_Is_Valid) ||
                 (!(Parameter_List[3].Integer_Value & flags) && Parameter_List[3].Integer_Value != 0))) {
                rc = QAT_Response_Str(QAT_RC_ERROR, "AT+OTAFWUP: Flag bit0: auto reboot, bit1: dup fs\r\n");
                return rc;
            }

            if (qat_upgrade_params != NULL) {
                rc = QAT_Response_Str(QAT_RC_ERROR, "AT+OTAFWUP: fw upgrade is ongoing\r\n");
                return rc;
            }

            qat_upgrade_params = malloc(sizeof(qat_fw_upgrade_params_t));
            if (qat_upgrade_params == NULL) {
                rc = QAT_Response_Str(QAT_RC_ERROR, "AT+OTAFWUP: No memory\r\n");
                return rc;
            }
            memset(qat_upgrade_params, 0, sizeof(qat_fw_upgrade_params_t));
            cmd = Parameter_List[0].String_Value;
            if (strncasecmp(cmd, "http", 4) == 0) {
                interface_len = strlen("wlan1") + 1;
                qat_upgrade_params->interface_name = malloc(interface_len);
                if (qat_upgrade_params->interface_name == NULL) {
                    rc = QAT_Response_Str(QAT_RC_ERROR, "AT+OTAFWUP: No memory\r\n");
                    goto qat_fw_upgrade_end;
                }
                memset(qat_upgrade_params->interface_name, 0, interface_len);
                memscpy(qat_upgrade_params->interface_name, interface_len, "wlan1", interface_len);

                url_len = strlen(Parameter_List[1].String_Value) + 1;
                qat_upgrade_params->url = malloc(url_len);
                if (qat_upgrade_params->url == NULL) {
                    rc = QAT_Response_Str(QAT_RC_ERROR, "AT+OTAFWUP: No memory\r\n");
                    goto qat_fw_upgrade_end;
                }
                memset(qat_upgrade_params->url, 0, url_len);
                memscpy(qat_upgrade_params->url, url_len, Parameter_List[1].String_Value, url_len);

                cfg_len = strlen(Parameter_List[2].String_Value) + 1;
                qat_upgrade_params->cfg_file = malloc(cfg_len);
                if (qat_upgrade_params->cfg_file == NULL) {
                    rc = QAT_Response_Str(QAT_RC_ERROR, "AT+OTAFWUP: No memory\r\n");
                    goto qat_fw_upgrade_end;
                }
                memset(qat_upgrade_params->cfg_file, 0, cfg_len);
                memscpy(qat_upgrade_params->cfg_file, cfg_len, Parameter_List[2].String_Value, cfg_len);

                if (Parameter_Count == 3) {
                    qat_upgrade_params->flags = QAPI_FW_UPGRADE_FLAG_AUTO_REBOOT;
                    qat_upgrade_params->timeout_time = HTTP_TIMEOUT;
                } else if (Parameter_Count == 4) {
                    qat_upgrade_params->flags = Parameter_List[3].Integer_Value;
                    qat_upgrade_params->timeout_time = HTTP_TIMEOUT;
                } else if (Parameter_Count == 5) {
                    qat_upgrade_params->flags = Parameter_List[3].Integer_Value;
                    qat_upgrade_params->timeout_time = Parameter_List[4].Integer_Value;
                    ;
                }
                else if (Parameter_Count == 6) {
                    if (Parameter_List[3].Integer_Is_Valid) {
                        qat_upgrade_params->flags = Parameter_List[3].Integer_Value;
                    }
                    else {
                        qat_upgrade_params->flags = QAPI_FW_UPGRADE_FLAG_AUTO_REBOOT;
                    }
                    if (qat_upgrade_params->flags & QAPI_FW_UPGRADE_FLAG_RANGE_HEADER) {
                        if (Parameter_List[5].Integer_Is_Valid) {
                            qat_upgrade_params->total_len = Parameter_List[5].Integer_Value;
                        } else {
                            rc = QAT_Response_Str(QAT_RC_ERROR, "AT+OTAFWUP: Invalid size parameter\r\n");
                            goto qat_fw_upgrade_end;
                        }
                    }
                    
                    if (Parameter_List[4].Integer_Is_Valid) {
                        qat_upgrade_params->timeout_time = Parameter_List[4].Integer_Value;
                    } else {
                        qat_upgrade_params->timeout_time = HTTP_TIMEOUT;
                    }
                }
                else {
                    rc = QAT_Response_Str(QAT_RC_ERROR, "AT+OTAFWUP: Invalid parameter\r\n");
                    goto qat_fw_upgrade_end;
                }
                rc = QAT_Response_Str(QAT_RC_OK, NULL);

                nt_qurt_thread_create(qat_fw_upgrade_HTTP_upgrade_task, "qat_fw_upgrade_demo", 1024 * 4, NULL, 7,
                                      &qat_fw_upgrade_task_handle);
            }
        }
        default:;
    }

    return rc;

qat_fw_upgrade_end:
    qat_fw_upgrade_free_memory();

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
static QAT_Command_Status_t Extend_Command_OTA_TRIAL(uint32_t Op_Type, uint32_t Parameter_Count,
                                                     QAT_Parameter_t *Parameter_List)
{
    char buffer[DISPLAY_FWD_BUFFER_LENGTH] = {0};
    int offset = 0;
    QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
    int32_t Index;

    switch (Op_Type) {
        case QAT_OP_EXEC: /* AT+OTATRIAL */
        {
            rc = QAT_Response_Str(QAT_RC_OK,
                                  "AT+OTATRIAL: get usage of command\r\n"
                                  "AT+OTATRIAL=<1|0(accept|deny)>,<reboot_flag>");
            break;
        }

        case QAT_OP_EXEC_W_PARAM: /* AT+OTATRIAL */
        {
            if ((Parameter_Count != 2) || (Parameter_List[0].Integer_Is_Valid == 0) ||
                (Parameter_List[1].Integer_Is_Valid == 0)) {
                rc = QAT_Response_Str(QAT_RC_ERROR, "AT+OTATRIAL=<1|0(accept|deny)>,<reboot_flag>");
                return rc;
            }

            if ((Parameter_List[0].Integer_Value > 1) || (Parameter_List[1].Integer_Value > 1)) {
                rc = QAT_Response_Str(QAT_RC_ERROR, "AT+OTATRIAL:Invalid parameter\r\n");
                return rc;
            }

            if (Parameter_List[0].Integer_Value == 1) {
                if (qapi_Fw_Upgrade_Done(1, Parameter_List[1].Integer_Value) != QAPI_OK) {
                    rc = QAT_Response_Str(QAT_RC_ERROR, "AT+OTATRIAL:Fail to Accept Trial FWD");
                } else {
                    rc = QAT_Response_Str(QAT_RC_OK, "AT+OTATRIAL:Success to Accept Trial FWD");
                }
            } else {
                if (qapi_Fw_Upgrade_Done(0, Parameter_List[1].Integer_Value) != QAPI_OK) {
                    rc = QAT_Response_Str(QAT_RC_ERROR, "AT+OTATRIAL:Fail to Reject Trial FWD");
                } else {
                    rc = QAT_Response_Str(QAT_RC_OK, "AT+OTATRIAL:Success to Reject Trial FWD");
                }
            }
            break;
        }

        default:;
    }

    return rc;
}

void Initialize_QAT_OTA_Demo(void)
{
    qbool_t RetVal;
    RetVal = QAT_Register_Command_Group(QAT_OTA_Command_List, OTA_COMMAND_LIST_SIZE);
    if (RetVal == false) {
        printf("Failed to register common command group.\n");
    }
}
