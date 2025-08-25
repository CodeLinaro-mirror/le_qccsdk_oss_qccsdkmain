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
#include "nt_osal.h"
#include "nt_socpm_sleep.h"
#include "qurt_mutex.h"
#include "wifi_fw_version.h"
#include "wifi_fw_pmu_ts_cfg.h"
#include "dns.h"
#include "ip_addr.h"
#include "at_web_server.h"

#define WEBSERVER_STR_BUFFER_LENGTH 64
/*-------------------------------------------------------------------------
 * Function Declarations
 *-----------------------------------------------------------------------*/

static QAT_Command_Status_t Extend_Command_WEBSERVER(uint32_t Op_Type, uint32_t Parameter_Count,
                                                     QAT_Parameter_t *Parameter_List);

static QAT_Command_Status_t Extend_Command_Cmd(uint32_t Op_Type, uint32_t Parameter_Count,
                                               QAT_Parameter_t *Parameter_List);

static uint32_t mem_test = 123;

/* The following is the complete command list for the QAT common command demo. */
/** List of global commands that are supported when in a group. */
static QAT_Command_t QAT_Http_Server_Command_List[] = {
    {"+WEBSERVER", Extend_Command_WEBSERVER, QAT_OP_EXEC | QAT_OP_EXEC_W_PARAM},
};
/*-------------------------------------------------------------------------
 * External parameters
 *-----------------------------------------------------------------------*/
extern HTC_Context_t HTC_Context;

/*-------------------------------------------------------------------------
 * Parameters define
 *-----------------------------------------------------------------------*/

#define HTTP_SERVER_COMMAND_LIST_SIZE (sizeof(QAT_Http_Server_Command_List) / sizeof(QAT_Command_t))

#define CMD_STR_BUFFER_LENGTH         1024
#define NORMAL_RESPONSE_BUFFER_LENGTH 1024

QAT_Command_Status_t Extend_Command_WEBSERVER(uint32_t Op_Type, uint32_t Parameter_Count,
                                              QAT_Parameter_t *Parameter_List)
{
    char *buffer = NULL;
    QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
    uint8_t enable = 0;
    uint32_t port = 80;
    int ret = 0;

    switch (Op_Type) {
        case QAT_OP_EXEC: /* AT+WEBSERVER */
        {
            buffer = malloc(WEBSERVER_STR_BUFFER_LENGTH);

            if (!buffer) {
                QAT_Response_Str(QAT_RC_ERROR, NULL);
                return rc;
            }

            memset(buffer, 0, WEBSERVER_STR_BUFFER_LENGTH);
            snprintf(buffer, WEBSERVER_STR_BUFFER_LENGTH, "+WEBSERVER: <enable>,<server_port>\r\n");
            rc = QAT_Response_Str(QAT_RC_OK, buffer);
            memset((void *)buffer, 0, WEBSERVER_STR_BUFFER_LENGTH);
            free(buffer);
            buffer = NULL;
            break;
        }
        case QAT_OP_EXEC_W_PARAM: /* AT+WEBSERVER */
        {
            if (!Parameter_List || (Parameter_Count > 2) ||
                (Parameter_Count == 1 && !Parameter_List[0].Integer_Is_Valid) ||
                (Parameter_Count == 1 && (Parameter_List[0].Integer_Value == 1)) ||
                (Parameter_Count == 2 &&
                 (!Parameter_List[0].Integer_Is_Valid || !Parameter_List[1].Integer_Is_Valid))) {
                rc = QAT_Response_Str(QAT_RC_ERROR, "+WEBSERVER:Wrong Input, AT+WEBSERVER? for hint\r\n");
                return rc;
            }

            enable = Parameter_List[0].Integer_Value;
            port = Parameter_List[1].Integer_Value;
            if (enable) {
                ret = at_web_start(port);
            } else {
                ret = at_web_stop();
            }

            if (ret == 0) {
                rc = QAT_Response_Str(QAT_RC_OK, NULL);
            } else {
                rc = QAT_Response_Str(QAT_RC_ERROR, NULL);
            }

            break;
        }
        default:;
    }

    return rc;
}

void Initialize_QAT_Http_Server_Demo(void)
{
    qbool_t RetVal;
    RetVal = QAT_Register_Command_Group(QAT_Http_Server_Command_List, HTTP_SERVER_COMMAND_LIST_SIZE);
    if (RetVal == false) {
        printf("Failed to register common command group.\n");
    }
}
