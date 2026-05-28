/* 
Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
SPDX-License-Identifier: BSD-3-Clause-Clear
*/

/*-------------------------------------------------------------------------
 * Include Files
 *-----------------------------------------------------------------------*/
#include <stdio.h>
#include <stdarg.h>
#include "string.h"
#include "qat.h"
#include "qat_api.h"
#include "qurt_internal.h"
#include "nt_osal.h"
#include "qurt_mutex.h"
#include "qapi_wlan.h"
#include "qapi_lowpower.h"
#include "lowpower_internal.h"
#include "wlan_power.h"

/*-------------------------------------------------------------------------
 * Function Declarations
 *-----------------------------------------------------------------------*/
static QAT_Command_Status_t Extend_Command_PS_Enable(uint32_t Op_Type, uint32_t Parameter_Count,
                                                    QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_PS_STAListenInterval(uint32_t Op_Type, uint32_t Parameter_Count,
                                                    QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_PS_PeriodAwake(uint32_t Op_Type, uint32_t Parameter_Count,
                                                    QAT_Parameter_t *Parameter_List);                                                         

extern bool wakeup_cb_bcmc_filter_dtim(uint16_t type, bool bm_cast, void *wifi_frame, uint16_t len);
extern uint64_t hres_timer_curr_time_us(void);
extern devh_t *gdevp;
qurt_signal_t http_sem;
uint8_t powersave_active = 0;
extern TimerHandle_t s_timer_handle;

static uint32_t ps_bmps_start;
static nt_osal_timer_handle_t ps_bmps_timer;

static void ps_bmps_timer_cb(void)
{
    uint32_t now = hres_timer_curr_time_us();
    uint32_t delta = now - ps_bmps_start;
    printf("BMPS timer expired. curr: %u, delta: %u\n", now, delta);
    qapi_bmps_cfg(0, 0);
    nt_delete_timer(ps_bmps_timer);
    ps_bmps_timer = NULL;
}

/* The following is the complete command list for the QAT powersave demo. */
/** List of global commands that are supported when in a group. */
static QAT_Command_t QAT_POWERSAVE_Command_List[] = {
    {"+PSENABLE", Extend_Command_PS_Enable, QAT_OP_EXEC | QAT_OP_EXEC_W_PARAM},
    {"+PSSTALI", Extend_Command_PS_STAListenInterval, QAT_OP_EXEC | QAT_OP_QUERY | QAT_OP_EXEC_W_PARAM},
    {"+PSPERAWAKE", Extend_Command_PS_PeriodAwake, QAT_OP_EXEC | QAT_OP_EXEC_W_PARAM},
};

/*-------------------------------------------------------------------------
 * Parameters define
 *-----------------------------------------------------------------------*/
#define NORMAL_RESPONSE_BUFFER_LENGTH 1024
#define POWERSAVE_COMMAND_LIST_SIZE (sizeof(QAT_POWERSAVE_Command_List) / sizeof(QAT_Command_t))
#define PS_CALLBACK_QAT_PRIORITY 11

/*-------------------------------------------------------------------------
 * Function Definitions
 *-----------------------------------------------------------------------*/
/**
   @brief power callback function during presleep/postawake events for qat demo

   Called after system .

   @param[in] evt          Indicates the Sleep Event
   @param[in] p_args       Arguments if any
*/
void qat_notify_pm_state_cb(uint8_t evt, void *p_args)
{
    (void)p_args;
    QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;

    if (evt == PWR_EVT_WMAC_PRE_SLEEP) {
    }

    if (evt == PWR_EVT_WMAC_POST_AWAKE) {
        if (spi_is_ext_wakeup()) {
            pm_set_powersave_policy(gdevp, PS_POLICY_NOT_ALLOWED_SLEEP);
            /* Disable bmps and imps when woken up by external pin */
            qapi_imps_enter_sleep(0,0,0);
            if (qapi_bmps_cfg(0, 0) != QAPI_OK) {
                rc = QAT_Response_Str(QAT_RC_ERROR, "+PSENABLE: bmps cfg failed");
            } else {
                /* Notify Host that device has woken up */
                QAT_Response_Str(QAT_RC_OK, "+EVT:wakeup\r\n");
                powersave_active = 0;
            }
            spi_clear_ext_wakeup_flag();
        } else {
            printf("QAT: Wakeup not from external pin, skip notification\n");
        }
        qurt_signal_set(&http_sem, 1);
    }
}

/**
   @brief Processes the +PSENABLE command from the QAT.

   Enables or disables BMPS (DTIM) power save for WLAN.

   @param[in] Op_Type          The input command type.
   @param[in] Parameter_Count  Number of parameters that were entered into the
                               command line.
   @param[in] Parameter_List   List of parameters entered into the command line.
*/
static QAT_Command_Status_t Extend_Command_PS_Enable(uint32_t Op_Type, uint32_t Parameter_Count,
                                                         QAT_Parameter_t *Parameter_List)
{
    char buffer[NORMAL_RESPONSE_BUFFER_LENGTH] = {0};
    QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
    qapi_Status_t ret;

    switch (Op_Type) {
        case QAT_OP_EXEC: /* AT+PSENABLE */
        {
            snprintf(buffer, NORMAL_RESPONSE_BUFFER_LENGTH,
                     "+PSENABLE=<1:enable|0:disable>[timeout in ms to exit power save]");
            rc = QAT_Response_Str(QAT_RC_OK, buffer);
            break;
        }

        case QAT_OP_EXEC_W_PARAM: /* AT+PSENABLE= */
        {
            if ((Parameter_Count != 1 && Parameter_Count != 2) || !Parameter_List ||
                !Parameter_List[0].Integer_Is_Valid) {
                rc = QAT_Response_Str(QAT_RC_ERROR, "+PSENABLE=<1:enable|0:disable>[timeout in ms to exit power save]");
                return rc;
            }

#ifdef FLASH_XIP_SUPPORT
            rc = QAT_Response_Str(QAT_RC_ERROR, "+PSENABLE: Don't support sleep in XIP mode");
            return rc;
#endif
            ret = qapi_pm_enable(Parameter_List[0].Integer_Value ? 1 : 0);
            if (ret != QAPI_OK) {
                QAT_Response_Str(QAT_RC_ERROR, "+PSENABLE: ps enable fail");
                return rc;
            }

            /* enable imps, default sleep time is 10 hours */
            qapi_imps_enter_sleep(Parameter_List[0].Integer_Value, 100, 36000000);

            /* bmps timeout settings */
            if (Parameter_Count == 2) {
                if (!Parameter_List[1].Integer_Is_Valid || Parameter_List[1].Integer_Value == 0) {
                    rc = QAT_Response_Str(QAT_RC_ERROR, "+PSENABLE=<1:enable|0:disable>[timeout in ms to exit power save]");
                    return rc;
                }
                ps_bmps_timer = nt_create_timer(ps_bmps_timer_cb, NULL, Parameter_List[1].Integer_Value, FALSE);
                if (!ps_bmps_timer) {
                    rc = QAT_Response_Str(QAT_RC_ERROR, "+PSENABLE: create timer failed");
                    return rc;
                }
                if (nt_start_timer(ps_bmps_timer) != NT_TIMER_SUCCESS) {
                    rc = QAT_Response_Str(QAT_RC_ERROR, "+PSENABLE: start timer failed");
                    return rc;
                }
                ps_bmps_start = hres_timer_curr_time_us();
                snprintf(buffer, NORMAL_RESPONSE_BUFFER_LENGTH, "+PSENABLE: BMPS timer started! curr: %u", ps_bmps_start);
                rc = QAT_Response_Str(QAT_RC_QUIET_NO_CR, buffer);
            }

            /* WMI_BMPS_IGNORE_BCMC_CMDID */
            qapi_bmps_rx_filter_enable(true);
            qapi_bmps_bcmc_rx_filter_cb_register(wakeup_cb_bcmc_filter_dtim, NULL);
            WMI_BMPS_IGNORE_BCMC ignore_bcmc_data;
            memset(&ignore_bcmc_data, 0, sizeof(ignore_bcmc_data));
            ignore_bcmc_data.enable = 1;
            wmi_cmd_send(WMI_BMPS_IGNORE_BCMC_CMDID, &ignore_bcmc_data, sizeof(ignore_bcmc_data)); 

            fpci_evt_cb_reg((ps_evt_cb_t)&qat_notify_pm_state_cb, PWR_EVT_WMAC_PRE_SLEEP | PWR_EVT_WMAC_POST_AWAKE | PWR_EVT_WMAC_SLEEP_ABORT, PS_CALLBACK_QAT_PRIORITY, NULL);
            
            /* http keep alive */
            qurt_signal_create(&http_sem);

            powersave_active = Parameter_List[0].Integer_Value ? 1 : 0;
            if (powersave_active) {
                pm_set_powersave_policy(gdevp, PS_POLICY_ALLOWED_SLEEP);
                spi_clear_ext_wakeup_flag();
                printf("QAT: External wakeup flag cleared\r\n");
            } else {
                pm_set_powersave_policy(gdevp, PS_POLICY_NOT_ALLOWED_SLEEP);
            }

            if (qapi_bmps_cfg(powersave_active, 0) != QAPI_OK) {
                rc = QAT_Response_Str(QAT_RC_ERROR, "+PSENABLE: bmps cfg failed");
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
   @brief Processes the Extend command from the QAT.

   This command will change the current group to its parent. No parameters are
   expected for this command.

   @param[in] Op_Type          The input command type.
   @param[in] Parameter_Count  Number of parameters that were entered into the
                               command line.
   @param[in] Parameter_List   List of parameters entered into the command line.
*/
static QAT_Command_Status_t Extend_Command_PS_STAListenInterval(uint32_t Op_Type, uint32_t Parameter_Count,
                                                    QAT_Parameter_t *Parameter_List)
{
    char buffer[NORMAL_RESPONSE_BUFFER_LENGTH] = {0};
    QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
    uint8_t deviceId = 0;

    switch (Op_Type) {
        case QAT_OP_EXEC: /* AT+PSSTALI */
        {
            snprintf(buffer, NORMAL_RESPONSE_BUFFER_LENGTH,
            "+PSSTALI: Set STA listen interval in TU which will round up/down to DTIM interval, 1TU=1024us\r\n"
                    "+PSSTALI=<listen_interval_in_TU>,<0: ronud up|1: round down>");
            rc = QAT_Response_Str(QAT_RC_OK, buffer);
            break;
        }

        case QAT_OP_QUERY: /* AT+PSSTALI? */
        {
            uint32_t listen_interval = 0;
            uint32_t length = sizeof(listen_interval);
            deviceId = get_active_device();
            if (QAPI_OK == qapi_WLAN_Get_Param(deviceId,
                                               __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                                               __QAPI_WLAN_PARAM_GROUP_WIRELESS_STA_LISTEN_INTERVAL_IN_TU,
                                               &listen_interval,
                                               &length)) {
                snprintf(buffer, NORMAL_RESPONSE_BUFFER_LENGTH, "+PSSTALI: %u TU", listen_interval);
                rc = QAT_Response_Str(QAT_RC_OK, buffer);
            } else {
                rc = QAT_Response_Str(QAT_RC_ERROR, "get STA listen interval fail");
            }
            break;
        }

        case QAT_OP_EXEC_W_PARAM: /* AT+PSSTALI= */
        {
            if (Parameter_Count != 2 || !Parameter_List || !Parameter_List[0].Integer_Is_Valid ||
                !Parameter_List[1].Integer_Is_Valid) {
                rc = QAT_Response_Str(QAT_RC_ERROR,
                                      "+PSSTALI=<listen_interval_in_TU>,<0: ronud up|1: round down>");
                return rc;
            }

            if (Parameter_List[0].Integer_Value > UINT16_MAX || Parameter_List[0].Integer_Value < 0) {
                rc = QAT_Response_Str(QAT_RC_ERROR, "listen interval need set 0-65535 TU");
                return rc;
            }
            if (!(Parameter_List[1].Integer_Value == 0 || Parameter_List[1].Integer_Value == 1)) {
                rc = QAT_Response_Str(QAT_RC_ERROR, "round type need set to 0 or 1");
                return rc;
            }

            qapi_WLAN_Listen_Interval_Params_t listen_interval;
            listen_interval.time = (uint16_t)Parameter_List[0].Integer_Value;
            listen_interval.round_type = (uint16_t)Parameter_List[1].Integer_Value;
            deviceId = get_active_device();

            if (QAPI_OK == qapi_WLAN_Set_Param(deviceId,
                                               __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                                               __QAPI_WLAN_PARAM_GROUP_WIRELESS_STA_LISTEN_INTERVAL_IN_TU,
                                               &listen_interval,
                                               sizeof(listen_interval),
                                               FALSE)) {
                rc = QAT_Response_Str(QAT_RC_OK, NULL);
            } else {
                rc = QAT_Response_Str(QAT_RC_ERROR, "set STA listen interval fail");
            }
            break;
        }

        default:;
    }

    return rc;
}

static void timer_cb(xTimerHandle xTimer)
{
    static int cnt = 0;
    cnt++;
    printf("TICK %d\r\n", cnt);
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
static QAT_Command_Status_t Extend_Command_PS_PeriodAwake(uint32_t Op_Type, uint32_t Parameter_Count,
                                                      QAT_Parameter_t *Parameter_List)
{
    char buffer[NORMAL_RESPONSE_BUFFER_LENGTH] = {0};
    QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
    qapi_Status_t ret;
    uint32_t period_ms = 1000;

    switch (Op_Type) {
        case QAT_OP_EXEC: /* AT+PSPERAWAKE */
        {
            snprintf(buffer, NORMAL_RESPONSE_BUFFER_LENGTH,
                     "+PSPERAWAKE=<1/0> [period in ms to awake], Enable BMPS(DTIM) period awake");
            rc = QAT_Response_Str(QAT_RC_OK, buffer);
            break;
        }

        case QAT_OP_EXEC_W_PARAM: /* AT+PSPERAWAKE= */
        {
            if (Parameter_Count == 2 && Parameter_List[1].Integer_Is_Valid) {
                period_ms = Parameter_List[1].Integer_Value;
            }
            if (!s_timer_handle) {
                s_timer_handle =
                    xTimerCreate("MyTimer", (period_ms / portTICK_PERIOD_MS), 1 /* uxAutoReload */, NULL, timer_cb);
            }

            if (Parameter_List[0].Integer_Value) {
                xTimerStart(s_timer_handle, portMAX_DELAY);
            } else {
                xTimerStop(s_timer_handle, portMAX_DELAY);
            }
            rc = QAT_Response_Str(QAT_RC_OK, NULL);
            break;
        }
        default:;
    }

    return rc;
}

void Initialize_QAT_POWERSAVE_Demo(void)
{
    qbool_t RetVal;
    RetVal = QAT_Register_Command_Group(QAT_POWERSAVE_Command_List, POWERSAVE_COMMAND_LIST_SIZE);
    if (RetVal == false) {
        printf("Failed to register powersave command group.\n");
    }
}
