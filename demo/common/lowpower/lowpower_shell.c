/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <stdio.h>
#include "qapi_types.h"
#include "qapi_version.h"
#include "qapi_status.h"
#include "qapi_console.h"
#include "qapi_lowpower.h"
#include "nt_hw.h"
#include "nt_hw_support.h"
#include "nt_socpm_sleep.h"
#include "wlan_drv.h"
#include "wmi_api.h"
#include "lowpower_internal.h"


#define TEST_SLP_TYPE_MCU       1
#define TEST_SLP_TYPE_LIGHT     2
#define DEEP_SLP_WKUP_AON_TMR       1
#define DEEP_SLP_WKUP_EXT           2


void nt_dpm_stop_network_stack();

int32_t test_sleep_list_no = -1;
uint32_t test_sleep_wkup_delay;
uint32_t test_sleep_start_time;
uint32_t test_sleep_min_time;

static uint32_t bmps_start;
static nt_osal_timer_handle_t bmps_timer;

extern lpr_wmi_t g_lowpower_wmi;

void test_sleep_cb()
{
	HAL_REG_WR(QWLAN_PMU_CFG_WIFI_SS_STATE_REG, NT_PMU_CFG_WIFI_SLEEP_OFFSET);
}

uint64_t __attribute__((section(".__sect_ps_txt"))) test_min_cb(uint32_t wkup_delay)
{
    test_sleep_wkup_delay = wkup_delay;
    test_sleep_min_time = (uint32_t)hres_timer_curr_time_us();
    return 0;
}

void test_wkup_cb(soc_wkup_reason wkup_reason)
{
    uint32_t now = (uint32_t)hres_timer_curr_time_us();
    uint32_t slp_time = test_sleep_min_time - test_sleep_start_time;
    uint32_t delta = now - test_sleep_min_time;
    printf("Wakeup! Reason: %d, %u, %u, %u\n",wkup_reason,test_sleep_wkup_delay,slp_time,delta);
    nt_socpm_sleep_lst_delete(test_sleep_list_no);
    test_sleep_list_no = -1;
    qapi_pm_enable(0);
}

static qapi_Status_t pm_enable(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    if( Parameter_Count != 1 || !Parameter_List || !Parameter_List[0].Integer_Is_Valid) {
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }
    return qapi_pm_enable(Parameter_List[0].Integer_Value ? 1 : 0);
}

static qapi_Status_t test_sleep(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    if (0) {
        nt_socpm_sleep_t test_sleep_timer;
        if( Parameter_Count != 2 || !Parameter_List ||
            !Parameter_List[0].Integer_Is_Valid || !Parameter_List[1].Integer_Is_Valid) {
            return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
        }
        memset(&test_sleep_timer, 0, sizeof(test_sleep_timer));

    if (Parameter_List[0].Integer_Value == TEST_SLP_TYPE_MCU) {
        test_sleep_timer.slp_mode = mcu_sleep;
    } else if (Parameter_List[0].Integer_Value == TEST_SLP_TYPE_LIGHT) {
        test_sleep_timer.slp_mode = Lightsleep;
    } else
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;

        test_sleep_timer.slp_time = Parameter_List[1].Integer_Value;
        test_sleep_timer.slp_cb_fn = test_sleep_cb;
        test_sleep_timer.min_cb_fn = test_min_cb;
        test_sleep_timer.wkup_cb_fn = test_wkup_cb;
        qapi_pm_enable(1);
        nt_dpm_stop_network_stack();
        test_sleep_start_time = (uint32_t)hres_timer_curr_time_us();
        test_sleep_list_no = nt_socpm_sleep_register(&test_sleep_timer, -1);
        return QAPI_OK;
    } else
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;

}

static qapi_Status_t deepsleep(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    if (Parameter_Count != 2 || !Parameter_List ||
        !Parameter_List[0].Integer_Is_Valid || !Parameter_List[1].Integer_Is_Valid) {
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }
    if (Parameter_List[0].Integer_Value == DEEP_SLP_WKUP_EXT) {
        printf("Ext wakeup not supported yet");
        return QAPI_ERR_NOT_SUPPORTED;
        //nt_socpm_en_indef_deep_sleep(TRUE);
    }
    uint64_t slp_time = (uint64_t)Parameter_List[1].Integer_Value;
    qapi_pm_enable(1);
    return qapi_deepsleep_enter(1, slp_time);
}

static qapi_Status_t slp_clk_cal_cfg(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    WMI_SLP_CLK_CAL_CFG *pdata = (WMI_SLP_CLK_CAL_CFG *)&g_lowpower_wmi;
    if( Parameter_Count != 1 || !Parameter_List || !Parameter_List[0].Integer_Is_Valid) {
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }
    memset(pdata, 0, sizeof(*pdata));
    pdata->enable = Parameter_List[0].Integer_Value ? 1 : 0;
    wmi_cmd_send(WMI_SLP_CLK_CAL_CFG_CMDID, pdata, sizeof(*pdata));
    return QAPI_OK;
}

static void bmps_timer_cb(void)
{
    WMI_BMPS_ENABLE *pdata = (WMI_BMPS_ENABLE *)&g_lowpower_wmi;
    uint32_t now = hres_timer_curr_time_us();
    uint32_t delta = now - bmps_start;
    printf("BMPS timer expired. curr: %u, delta: %u\n", now, delta);
    memset(pdata, 0, sizeof(*pdata));
    pdata->enable = 0;
    wmi_cmd_send(WMI_BMPS_ENABLE_CMDID, pdata, sizeof(*pdata));
    nt_delete_timer(bmps_timer);
    bmps_timer = NULL;
}

static qapi_Status_t bmps_enable(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    if((Parameter_Count != 1 && Parameter_Count != 2) || !Parameter_List || !Parameter_List[0].Integer_Is_Valid) {
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }
	#ifdef FLASH_XIP_SUPPORT
	printf("Don't support sleep in XIP mode. %u\n");
	return QAPI_OK;
	#endif
    if (Parameter_Count == 2) {
        if (!Parameter_List[1].Integer_Is_Valid || Parameter_List[1].Integer_Value == 0)
            return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
        bmps_timer = nt_create_timer(bmps_timer_cb, NULL, Parameter_List[1].Integer_Value, FALSE); // ms
        if (!bmps_timer)
            return QAPI_ERROR;
        if (nt_start_timer(bmps_timer) != NT_TIMER_SUCCESS)
            return QAPI_ERROR;
        bmps_start = hres_timer_curr_time_us();
        printf("BMPS timer started! curr: %u\n", bmps_start);
    }
    return qapi_bmps_cfg(Parameter_List[0].Integer_Value ? 1 : 0, 0);
}

static qapi_Status_t bmps_ignore_bcmc(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    WMI_BMPS_IGNORE_BCMC *pdata = (WMI_BMPS_IGNORE_BCMC *)&g_lowpower_wmi;
    if( Parameter_Count != 1 || !Parameter_List || !Parameter_List[0].Integer_Is_Valid) {
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }
    memset(pdata, 0, sizeof(*pdata));
    pdata->enable = Parameter_List[0].Integer_Value ? 1 : 0;
    wmi_cmd_send(WMI_BMPS_IGNORE_BCMC_CMDID, pdata, sizeof(*pdata));
    return QAPI_OK;
}

static qapi_Status_t bmps_idle_time(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    if( Parameter_Count != 1 || !Parameter_List || !Parameter_List[0].Integer_Is_Valid) {
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }
    uint32_t idle_time = Parameter_List[0].Integer_Value;
    return qapi_bmps_cfg(2, idle_time);
}

static qapi_Status_t bmps_timing_cfg(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    WMI_BMPS_TIMING_CFG *pdata = (WMI_BMPS_TIMING_CFG *)&g_lowpower_wmi;
    if( Parameter_Count != 4 || !Parameter_List || !Parameter_List[0].Integer_Is_Valid ||
        !Parameter_List[1].Integer_Is_Valid || !Parameter_List[2].Integer_Is_Valid ||
        !Parameter_List[3].Integer_Is_Valid) {
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }
    memset(pdata, 0, sizeof(*pdata));
    pdata->pre_bcn_wkup = Parameter_List[0].Integer_Value;
    pdata->bcn_wait_time = Parameter_List[1].Integer_Value;
    pdata->tele_pre_bcn_inc = Parameter_List[2].Integer_Value;
    pdata->tele_bcn_wait_inc = Parameter_List[3].Integer_Value;
    wmi_cmd_send(WMI_BMPS_TIMING_CFG_CMDID, pdata, sizeof(*pdata));
    return QAPI_OK;
}

static qapi_Status_t imps_cfg(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    if( Parameter_Count != 5 || !Parameter_List || !Parameter_List[0].Integer_Is_Valid ||
        !Parameter_List[1].Integer_Is_Valid || !Parameter_List[2].Integer_Is_Valid ||
        !Parameter_List[3].Integer_Is_Valid || !Parameter_List[4].Integer_Is_Valid) {
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }
    uint8_t enable;
    uint32_t slp_time, recnx_wait, wmi_wait, cnx_wait;
    enable = Parameter_List[0].Integer_Value ? 1 : 0;
    slp_time = Parameter_List[1].Integer_Value;
    recnx_wait = Parameter_List[2].Integer_Value;
    wmi_wait = Parameter_List[3].Integer_Value;
    cnx_wait = Parameter_List[4].Integer_Value;
    return qapi_imps_cfg(enable, slp_time, recnx_wait, wmi_wait, cnx_wait);
}

static qapi_Status_t slp_clk_cal_act(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    WMI_SLP_CLK_CAL_ACT *pdata = (WMI_SLP_CLK_CAL_ACT *)&g_lowpower_wmi;
    if( Parameter_Count != 1 || !Parameter_List || !Parameter_List[0].Integer_Is_Valid) {
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }
    memset(pdata, 0, sizeof(*pdata));
    pdata->enable = Parameter_List[0].Integer_Value ? 1 : 0;
    wmi_cmd_send(WMI_SLP_CLK_CAL_ACT_CMDID, pdata, sizeof(*pdata));
    return QAPI_OK;
}

static qapi_Status_t bmps_force_dtim(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    uint32_t *pdata = (uint32_t *)&g_lowpower_wmi;
    if(Parameter_Count != 1 || !Parameter_List || !Parameter_List[0].Integer_Is_Valid) {
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }
    memset(pdata, 0, sizeof(*pdata));
    *pdata = Parameter_List[0].Integer_Value;
    wmi_cmd_send(WMI_SET_FORCE_DTIM, pdata, sizeof(*pdata));
    return QAPI_OK;
}

#ifdef CONFIG_CPR_ENABLE
static qapi_Status_t cpr_enable(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    extern void wifi_fw_cpr_reenable(void);
    extern void wifi_fw_cpr_disable(void);
    
    if(Parameter_Count != 1 || !Parameter_List || !Parameter_List[0].Integer_Is_Valid) {
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }

    if (Parameter_List[0].Integer_Value == 0) {
        wifi_fw_cpr_disable();
        printf("CPR Disabled");
    } else if (Parameter_List[0].Integer_Value == 1) {
        wifi_fw_cpr_reenable();
        printf("CPR Reenabled");
    } else {
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }
    return QAPI_OK;
}
#endif //CONFIG_CPR_ENABLE

const QAPI_Console_Command_t lowpower_shell_cmds[] =
{
   // cmd_function    cmd_string               usage_string             description
    {pm_enable, "pm_enable", "<1/0>", "Enable/disable system power management\n"},
//    {test_sleep, "test_sleep", "<1:mcu_sleep|2:lightsleep> <sleep duration in us>", "Cfg and enable sleep\n"},
    {test_sleep, "test_sleep", "", "Not support now\n"},
    {deepsleep, "deepsleep", "<1:AON timer wkup|2:Ext wkup> <sleep duration in us>", "Cfg and enable deepsleep\n"},
    {slp_clk_cal_cfg, "slp_clk_cal_cfg", "<1/0>", "Enable/disable slp_clk_cal in sleep mode\n"},
    {bmps_enable, "bmps_enable", "<1/0> [timeout in ms to exit BMPS]", "Enable BMPS(DTIM) sleep for WLAN\n"},
    {bmps_ignore_bcmc, "bmps_ignore_bcmc", "<1/0>", "Ignore Bcast/Mcast wakeup during BMPS(DTIM)\n"},
    {bmps_idle_time, "bmps_idle_time", "<Idle time in ms>", "Cfg max idle time prior entering into BMPS(DTIM) sleep\n"},
    {bmps_timing_cfg, "bmps_timing_cfg", "<preBcn in us> <bcnWait in us> <telePreBcnInc in us> <teleBcnWaitInc in us>", "Cfg BMPS timing parameters\n"},
    {imps_cfg, "imps_cfg", "<1:Enable|0:Disable> <deepsleep time in ms> <recnx timeout in ms> <cmd proc in ms> <cnx timeout in ms>", "Cfg BMPS timing parameters\n"},
    {slp_clk_cal_act, "slp_clk_cal_act", "<1/0>", "Enable/disable slp_clk_cal in active mode\n"},
    {bmps_force_dtim, "bmps_force_dtim", "<Forced DTIM count>", "Force DTIM count\n"},
#ifdef CONFIG_CPR_ENABLE
    {cpr_enable, "cpr_enable", "<1/0>", "Enable CPR for Power save (This qcli is only for debugging. CPR enabled for default)\n"},
#endif //CONFIG_CPR_ENABLE
};

const QAPI_Console_Command_Group_t lowpower_shell_cmd_group = {"lowpower", sizeof(lowpower_shell_cmds) / sizeof(QAPI_Console_Command_t), lowpower_shell_cmds};

QAPI_Console_Group_Handle_t lowpower_shell_cmd_group_handle;

void lowpower_shell_init(void)
{
    lowpower_shell_cmd_group_handle = QAPI_Console_Register_Command_Group(NULL, &lowpower_shell_cmd_group);
}
