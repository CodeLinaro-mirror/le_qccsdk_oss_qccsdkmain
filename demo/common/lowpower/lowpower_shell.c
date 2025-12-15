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
#include "timer.h"
#include "wlan_drv.h"
#include "wlan_power.h"
#include "wmi.h"
#include "wmi_api.h"
#include "lowpower_internal.h"
#include "ethernet.h"
#include "ip4.h"
#include "udp.h"

#define TEST_SLP_TYPE_MCU     1
#define TEST_SLP_TYPE_LIGHT   2
#define DEEP_SLP_WKUP_AON_TMR 1
#define DEEP_SLP_WKUP_EXT     2
#define UDP_WHITELIST_LEN     4
#define PP_HTONS(x)           ((u16_t)((((x) & (u16_t)0x00ffU) << 8) | (((x) & (u16_t)0xff00U) >> 8)))

#define WIFI_MAC_HEADER_LEN 24

#define LLC_SNAP_HEADER_LEN 8

#define LOWPOWER_CB_SIGNAL 0x1

void nt_dpm_stop_network_stack();

int32_t test_sleep_list_no = -1;
uint32_t test_sleep_wkup_delay;
uint32_t test_sleep_start_time;
uint32_t test_sleep_min_time;
uint32_t udp_whitelist_arr[UDP_WHITELIST_LEN] = {7777, 0, 0, 0};

TimerHandle_t s_timer_handle = NULL;

static uint32_t bmps_start;
static nt_osal_timer_handle_t bmps_timer;

static uint32_t bmps_cb_exit_start;
static nt_osal_timer_handle_t bmps_cb_exit_timer;
qurt_signal_t bmps_lowpower_cb_exit_task_signal;
uint8_t bmps_cb_uc_bc_wakeup = 0;
bool bmps_lowpower_infinite_task_created = false;

extern lpr_wmi_t g_lowpower_wmi;

bool wakeup_cb_bcmc_filter_dtim(uint16_t type, bool bm_cast, void *wifi_frame, uint16_t len);
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
    printf("Wakeup! Reason: %d, %u, %u, %u\n", wkup_reason, test_sleep_wkup_delay, slp_time, delta);
    nt_socpm_sleep_lst_delete(test_sleep_list_no);
    test_sleep_list_no = -1;
    qapi_pm_enable(0);
}

static qapi_Status_t pm_enable(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    if (Parameter_Count != 1 || !Parameter_List || !Parameter_List[0].Integer_Is_Valid) {
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }
    return qapi_pm_enable(Parameter_List[0].Integer_Value ? 1 : 0);
}

static qapi_Status_t test_sleep(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    if (0) {
        nt_socpm_sleep_t test_sleep_timer;
        if (Parameter_Count != 2 || !Parameter_List || !Parameter_List[0].Integer_Is_Valid ||
            !Parameter_List[1].Integer_Is_Valid) {
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
    if (Parameter_Count != 2 || !Parameter_List || !Parameter_List[0].Integer_Is_Valid ||
        !Parameter_List[1].Integer_Is_Valid) {
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }
    if (Parameter_List[0].Integer_Value == DEEP_SLP_WKUP_EXT) {
        printf("Ext wakeup indefinite deepsleep supported");
        nt_socpm_en_indef_deep_sleep(TRUE);
    }
    uint64_t slp_time = ((uint64_t)Parameter_List[1].Integer_Value * 1000);
    qapi_pm_enable(1);
    return qapi_deepsleep_enter(Parameter_List[0].Integer_Value, slp_time);
}

static qapi_Status_t slp_clk_cal_cfg(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    WMI_SLP_CLK_CAL_CFG *pdata = (WMI_SLP_CLK_CAL_CFG *)&g_lowpower_wmi;
    if (Parameter_Count != 1 || !Parameter_List || !Parameter_List[0].Integer_Is_Valid) {
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

static void timer_cb(xTimerHandle xTimer)
{
    static int cnt = 0;
    cnt++;
    // printf("TICK %d\r\n", cnt);
}

static qapi_Status_t bmps_period_awake(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    uint32_t period_ms = 1000;
    if ((Parameter_Count != 1 && Parameter_Count != 2) || !Parameter_List || !Parameter_List[0].Integer_Is_Valid) {
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }

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
    return QAPI_OK;
}
static void bmps_callback_exit_timer_cb(void)
{
    WMI_BMPS_ENABLE *pdata = (WMI_BMPS_ENABLE *)&g_lowpower_wmi;
    uint32_t now = hres_timer_curr_time_us();
    uint32_t delta = now - bmps_cb_exit_start;

    printf("BMPS callback exit timer expired. curr: %u, delta: %u\n", now, delta);

    memset(pdata, 0, sizeof(*pdata));
    pdata->enable = 0;

    qapi_bmps_cfg(pdata->enable, 0);
    nt_delete_timer(bmps_cb_exit_timer);
    bmps_cb_exit_timer = NULL;
}
static qapi_Status_t bmps_enable(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    if ((Parameter_Count != 1 && Parameter_Count != 2) || !Parameter_List || !Parameter_List[0].Integer_Is_Valid) {
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }
#ifdef FLASH_XIP_SUPPORT
    printf("Don't support sleep in XIP mode. %u\n");
    return QAPI_OK;
#endif
    if (Parameter_Count == 2) {
        if (!Parameter_List[1].Integer_Is_Valid || Parameter_List[1].Integer_Value == 0)
            return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
        bmps_timer = nt_create_timer(bmps_timer_cb, NULL, Parameter_List[1].Integer_Value, FALSE);  // ms
        if (!bmps_timer)
            return QAPI_ERROR;
        if (nt_start_timer(bmps_timer) != NT_TIMER_SUCCESS)
            return QAPI_ERROR;
        bmps_start = hres_timer_curr_time_us();
        printf("BMPS timer started! curr: %u\n", bmps_start);
    }
    qapi_bmps_rx_filter_enable(true);
    qapi_bmps_bcmc_rx_filter_cb_register(wakeup_cb_bcmc_filter_dtim, NULL);

    return qapi_bmps_cfg(Parameter_List[0].Integer_Value ? 1 : 0, 0);
}

static qapi_Status_t bmps_ignore_bcmc(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    WMI_BMPS_IGNORE_BCMC *pdata = (WMI_BMPS_IGNORE_BCMC *)&g_lowpower_wmi;
    if (Parameter_Count != 1 || !Parameter_List || !Parameter_List[0].Integer_Is_Valid) {
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }
    memset(pdata, 0, sizeof(*pdata));
    pdata->enable = Parameter_List[0].Integer_Value ? 1 : 0;
    wmi_cmd_send(WMI_BMPS_IGNORE_BCMC_CMDID, pdata, sizeof(*pdata));
    return QAPI_OK;
}

static qapi_Status_t bmps_idle_time(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    if (Parameter_Count != 1 || !Parameter_List || !Parameter_List[0].Integer_Is_Valid) {
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }
    uint32_t idle_time = Parameter_List[0].Integer_Value;
    return qapi_bmps_cfg(2, idle_time);
}

static qapi_Status_t bmps_timing_cfg(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    WMI_BMPS_TIMING_CFG *pdata = (WMI_BMPS_TIMING_CFG *)&g_lowpower_wmi;
    if (Parameter_Count != 4 || !Parameter_List || !Parameter_List[0].Integer_Is_Valid ||
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
    if (Parameter_Count != 6 || !Parameter_List || !Parameter_List[0].Integer_Is_Valid ||
        !Parameter_List[1].Integer_Is_Valid || !Parameter_List[2].Integer_Is_Valid ||
        !Parameter_List[3].Integer_Is_Valid || !Parameter_List[4].Integer_Is_Valid) {
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }
    uint8_t enable;
    uint32_t slp_time, recnx_wait, wmi_wait, cnx_wait, sleep_mode;
    enable = Parameter_List[0].Integer_Value ? 1 : 0;
    slp_time = Parameter_List[1].Integer_Value;
    recnx_wait = Parameter_List[2].Integer_Value;
    wmi_wait = Parameter_List[3].Integer_Value;
    cnx_wait = Parameter_List[4].Integer_Value;
    sleep_mode = Parameter_List[5].Integer_Value;
    return qapi_imps_cfg(enable, slp_time, recnx_wait, wmi_wait, cnx_wait, sleep_mode);
}

static qapi_Status_t imps_sleep(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    if (!Parameter_List ) {
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }

    uint8_t enable;
    uint32_t slp_time, recnx_wait, wmi_wait, cnx_wait, sleep_mode;

    if (Parameter_List[0].Integer_Value) {
        if (Parameter_Count != 3 || !Parameter_List || !Parameter_List[0].Integer_Is_Valid || !Parameter_List[1].Integer_Is_Valid || !Parameter_List[2].Integer_Is_Valid ) {
            return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
        }
        enable = Parameter_List[0].Integer_Value;
        recnx_wait = Parameter_List[1].Integer_Value;
        slp_time = Parameter_List[2].Integer_Value;

        return qapi_imps_enter_sleep(enable, recnx_wait, slp_time);
    } else {
        return qapi_imps_disable_sleep();
    }
}

static qapi_Status_t slp_clk_cal_act(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    WMI_SLP_CLK_CAL_ACT *pdata = (WMI_SLP_CLK_CAL_ACT *)&g_lowpower_wmi;
    if (Parameter_Count != 1 || !Parameter_List || !Parameter_List[0].Integer_Is_Valid) {
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
    if (Parameter_Count != 1 || !Parameter_List || !Parameter_List[0].Integer_Is_Valid) {
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }
    memset(pdata, 0, sizeof(*pdata));
    *pdata = Parameter_List[0].Integer_Value;
    wmi_cmd_send(WMI_SET_FORCE_DTIM, pdata, sizeof(*pdata));
    return QAPI_OK;
}

// uint16_t udp_list[10]={0};

uint64_t fourth_bc = 0;
extern uint64_t bc_delta_time, bc_after_bcn, bcmc_len;
extern uint8_t bcmc[1000];

uint64_t hres_timer_curr_time_us(void);

static bool parse_eth_frame_in_whitelist(const uint8_t *frame, uint16_t frame_len)
{
    if (frame_len < sizeof(struct eth_hdr) + sizeof(struct ip_hdr) + sizeof(struct udp_hdr)) {
        // printf("Frame too short\n");
        return TRUE;
    }

    struct eth_hdr *eth = (struct eth_hdr *)frame;
    if (ntohs(eth->type) != ETHTYPE_IP) {
        // printf("Not an IPv4 packet\n");
        return TRUE;
    }

    struct ip_hdr *ip = (struct ip_hdr *)(frame + sizeof(struct eth_hdr));
    if (ip->_proto != IP_PROTO_UDP) {
        // printf("Not a UDP packet\n");
        return TRUE;
    }

    struct udp_hdr *udp = (struct udp_hdr *)(frame + sizeof(struct eth_hdr) + (IPH_HL_BYTES(ip)));
    uint16_t src_port = ntohs(udp->src);
    uint16_t dst_port = ntohs(udp->dest);

    // whitelist for UDP
    for (uint16_t i = 0; i < UDP_WHITELIST_LEN; i++) {
        if (udp_whitelist_arr[i] && dst_port == udp_whitelist_arr[i]) {
            printf("type:0x%x\n", PP_HTONS(eth->type));
            printf("UDP Source Port: %d\n", src_port);
            printf("UDP Destination Port: %d\n", dst_port);
        }
    }

    return TRUE;
}

bool wakeup_cb_bcmc_filter_dtim(uint16_t type, bool bm_cast, void *wifi_frame, uint16_t len)
{
    // uint32_t ip_frame_len;
    uint8_t *ip_frame;
    if (bm_cast) {
        if (len < (WIFI_MAC_HEADER_LEN + LLC_SNAP_HEADER_LEN)) {
            return FALSE;
        }

        const uint8_t *llc_snap_header = wifi_frame + WIFI_MAC_HEADER_LEN;

        if (llc_snap_header[6] != 0x08 || llc_snap_header[7] != 0x00) {
            return TRUE;
        }

        // ip_frame_len = len - (WIFI_MAC_HEADER_LEN + LLC_SNAP_HEADER_LEN);
        ip_frame = wifi_frame + WIFI_MAC_HEADER_LEN + LLC_SNAP_HEADER_LEN;

        struct ip_hdr *ip = (struct ip_hdr *)(ip_frame);
        if (ip->_proto != IP_PROTO_UDP) {
            return TRUE;
        }

        struct udp_hdr *udp = (struct udp_hdr *)(ip_frame + (IPH_HL_BYTES(ip)));
        uint16_t src_port = ntohs(udp->src);
        uint16_t dst_port = ntohs(udp->dest);

        // whitelist for UDP dst port
        for (uint16_t i = 0; i < UDP_WHITELIST_LEN; i++) {
            if (udp_whitelist_arr[i] && dst_port == udp_whitelist_arr[i]) {
                return TRUE;
            }
        }

        return FALSE;
    }
    return TRUE;
}

bool wakeup_cb(uint16_t type, bool bm_cast, void *pbuf, uint16_t len)
{
    const struct eth_hdr *ethhdr;
    const struct ip_hdr *iphdr;
    const struct udp_hdr *udphdr;

    // printf("type:0x%x\n",type);
    // printf("bc_delta_time:%d, bc_after_bcn:%d, bcmc_len:%d \n",(uint32_t) bc_delta_time, (uint32_t) bc_after_bcn,
    // (uint32_t)bcmc_len );
    bc_after_bcn = 0;
    bcmc_len = 0;
    bc_delta_time = 0;

    // for(uint16_t i =0;i<1000;i++)
    // {
    //     if(bcmc[i])
    //         printf("%x ",bcmc[i]);
    // }
    memset(bcmc, 0, 1000);

    /*the pbuf as link-layer broadcast */
    if (bm_cast && (type == ETHTYPE_IP)) {
        fourth_bc = hres_timer_curr_time_us();
        return parse_eth_frame_in_whitelist((uint8_t *)pbuf, len);
    }

    // every unicast is in wake up whitelist
    return TRUE;
}

static qapi_Status_t bcmc_filter_enable(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    if (Parameter_Count != 2 || !Parameter_List || !Parameter_List[0].Integer_Is_Valid) {
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }

    qapi_bmps_rx_filter_enable(Parameter_List[0].Integer_Value ? 1 : 0);

    if (Parameter_List[0].Integer_Value) {
        qapi_bmps_bcmc_rx_filter_cb_register(wakeup_cb_bcmc_filter_dtim,
                                             Parameter_List[1].Integer_Value ? wakeup_cb : NULL);
    }
    return QAPI_OK;
}

void bmps_lowpower_cb_exit_task(void __attribute__((__unused__)) * pvParameters)
{
    WMI_BMPS_ENABLE *pdata = (WMI_BMPS_ENABLE *)&g_lowpower_wmi;
    while (1) {
        qurt_signal_wait(&bmps_lowpower_cb_exit_task_signal, LOWPOWER_CB_SIGNAL, QURT_SIGNAL_ATTR_CLEAR_MASK);

        uint32_t start_time = HAL_REG_RD(QWLAN_MTU_MTU_GLOBAL_TIMER_REG);
        uint32_t delta_time = 0;

        while (delta_time < 50000) {
            nt_socpm_nop_delay(1000);  // short wait before another check
            delta_time = HAL_REG_RD(QWLAN_MTU_MTU_GLOBAL_TIMER_REG) - start_time;
        }

        memset(pdata, 0, sizeof(*pdata));
        pdata->enable = 0;

        qapi_bmps_cfg(pdata->enable, 0);

        bmps_cb_uc_bc_wakeup = 0;
        // printf("test_task end\r\n");
    }
}

void bmps_sleep_wake_cb(uint8_t evt, void *p_args)
{
    (void)p_args;
    uint8_t reason = 0;

    if (evt == PWR_EVT_WMAC_PRE_SLEEP) {
        // printf("Enter PS mode!\r\n");
    }

    if (evt == PWR_EVT_WMAC_POST_AWAKE) {
        // printf("Exit PS mode!\r\n");

        /* Exit BMPS only when waken up by interrupt*/
        qapi_bmps_get_exit_reason(&reason);
        if (reason == EXIT_REASON_EXT_INT) {
            bmps_cb_exit_timer = nt_create_timer(bmps_callback_exit_timer_cb, NULL, 2, FALSE);  // ms
            if (!bmps_cb_exit_timer) {
                printf("BMPS timer create failed!\r\n");
                return;
            }
            if (nt_start_timer(bmps_cb_exit_timer) != NT_TIMER_SUCCESS) {
                printf("BMPS timer start failed!\r\n");
                return;
            }
            bmps_cb_exit_start = hres_timer_curr_time_us();
        } else if (bmps_cb_uc_bc_wakeup && (reason == EXIT_REASON_TIM_UC || reason == EXIT_REASON_TIM_BC)) {
            qurt_signal_set(&bmps_lowpower_cb_exit_task_signal, LOWPOWER_CB_SIGNAL);
        }
    }
}

static qapi_Status_t bmps_cb_register(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    static uint8_t cb_registered = 0;

    if ((Parameter_Count != 1 && Parameter_Count != 2) || !Parameter_List || !Parameter_List[0].Integer_Is_Valid) {
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }

    bmps_cb_uc_bc_wakeup = 0;

    if (Parameter_List[1].Integer_Is_Valid) {
        bmps_cb_uc_bc_wakeup = Parameter_List[1].Integer_Value;
    }

    if (cb_registered == 0) {
        qurt_signal_create(&bmps_lowpower_cb_exit_task_signal);

        nt_qurt_thread_create(bmps_lowpower_cb_exit_task, "bmps_lowpower_cb_exit_task", 300, NULL, 6, NULL);

        qapi_bmps_sleep_wakeup_cb((ps_evt_cb_t)&bmps_sleep_wake_cb, Parameter_List[0].Integer_Value);

        cb_registered = 1;
    }

    return QAPI_OK;
}
static qapi_Status_t bmps_log_enable(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    if ((Parameter_Count != 1) || !Parameter_List || !Parameter_List[0].Integer_Is_Valid) {
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }
    return qapi_bmps_log_enable(Parameter_List[0].Integer_Value ? 1 : 0);
}
static qapi_Status_t bcmc_filter_list(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    unsigned int index = 0;
    bool op = 0;
    bool add_success = false;
    unsigned int port = 0;
    if (Parameter_Count < 1) {
        printf("\nbcmc_filter_list -a [1|0] -u [dst udp port] -q\n");
        printf("  -a  = 1:add udp port to the whitelist, 0:delete udp port from the whitelist \n");
        printf("  -u  = the dst udp port like 7777\n");
        printf("  -q  = query the whitelist\n");

        return QAPI_ERR_INVALID_PARAM;
    }

    while (index < Parameter_Count) {
        if (0 == strcasecmp(Parameter_List[index].String_Value, "-a")) {
            index++;
            if (Parameter_List[index].Integer_Value != 0 && Parameter_List[index].Integer_Value != 1)
                return QAPI_ERR_INVALID_PARAM;

            op = Parameter_List[index].Integer_Value;
            index++;
        } else if (0 == strcasecmp(Parameter_List[index].String_Value, "-u")) {
            index++;
            port = Parameter_List[index].Integer_Value;
            index++;
        } else if (0 == strcasecmp(Parameter_List[index].String_Value, "-q")) {
            index++;
            for (uint16_t i = 0; i < UDP_WHITELIST_LEN; i++) {
                printf("%d ", udp_whitelist_arr[i]);
            }
            printf("\r\n");
            return QAPI_OK;
        } else {
            printf("Arguments not valid!\r\n");
            return QAPI_OK;
        }
    }

    if (op) {
        for (uint16_t i = 0; i < UDP_WHITELIST_LEN; i++) {
            if (udp_whitelist_arr[i] == 0) {
                udp_whitelist_arr[i] = port;
                add_success = true;
                break;
            }
        }
        if (!add_success) {
            printf("Add failed, the udp_whitelist_arr not empty");
        }
    } else {
        for (uint16_t i = 0; i < UDP_WHITELIST_LEN; i++) {
            if (udp_whitelist_arr[i] == port)
                udp_whitelist_arr[i] = 0;
        }
    }

    return QAPI_OK;
}

#ifdef CONFIG_CPR_ENABLE
static qapi_Status_t cpr_enable(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    extern void wifi_fw_cpr_reenable(void);
    extern void wifi_fw_cpr_disable(void);

    if (Parameter_Count != 1 || !Parameter_List || !Parameter_List[0].Integer_Is_Valid) {
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
#endif  // CONFIG_CPR_ENABLE

void bmps_lowpower_infinite_task(void __attribute__((__unused__)) * pvParameters)
{
    while (bmps_lowpower_infinite_task_created) {
        vTaskDelay(10);
    }
    printf("bmps_lowpower_infinite_task deleted\n");
    vTaskDelete(NULL);
}
static qapi_Status_t bmps_infinite_task(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    if ((Parameter_Count != 1) || !Parameter_List || !Parameter_List[0].Integer_Is_Valid) {
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }
    if(Parameter_List[0].Integer_Value && bmps_lowpower_infinite_task_created == 0){
        bmps_lowpower_infinite_task_created = true;
        printf("bmps_lowpower_infinite_task created\n");
        nt_qurt_thread_create(bmps_lowpower_infinite_task, "bmps_lowpower_infinite_task", 300, NULL, 5, NULL);
    }
    else if(Parameter_List[0].Integer_Value == 0){
        bmps_lowpower_infinite_task_created = false;
    }
    return QAPI_OK;
}

static qapi_Status_t bmps_power_optimization_enable(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    if ((Parameter_Count != 1) || !Parameter_List || !Parameter_List[0].Integer_Is_Valid) {
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }
    return qapi_bmps_power_optimization_enable(Parameter_List[0].Integer_Value ? 1 : 0);
}

static qapi_Status_t bmps_compress_qos_null_enable(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    if ((Parameter_Count != 1) || !Parameter_List || !Parameter_List[0].Integer_Is_Valid) {
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }
    return qapi_bmps_compress_qos_null_enable(Parameter_List[0].Integer_Value ? 1 : 0);
}

static qapi_Status_t bmps_stats(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    WMI_BMPS_GET_STATS *cmd_bmps_stats = NULL;
    WMI_GET_NOISE_STATUS *cmd_noise_status = NULL;
    uint32_t total_bwindow_sleep_time = 0;
    uint32_t total_bwindow_active_time = 0;
    uint32_t total_soc_sleep_time = 0;
    uint32_t total_soc_active_time = 0;
    uint8_t index = 0;

    if ((Parameter_Count < 1)) {
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }

    while (index < Parameter_Count) {
        if (0 == strcmp(Parameter_List[index].String_Value, "set")) {
            index++;
            if(Parameter_List[index].Integer_Is_Valid){
                qapi_bmps_set_period_to_record_for_stats(Parameter_List[index].Integer_Value);
            }  
            else{
                return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
            }
            return QAPI_OK;
        }
        else if(0 == strcmp(Parameter_List[index].String_Value, "get")){
            index++;

            cmd_noise_status = (WMI_GET_NOISE_STATUS *)malloc(sizeof(WMI_GET_NOISE_STATUS));
            if(cmd_noise_status == NULL){
                printf("malloc failed\n");
                return QAPI_ERROR;
            }

            cmd_bmps_stats = (WMI_BMPS_GET_STATS *)malloc(sizeof(WMI_BMPS_GET_STATS));
            if(cmd_bmps_stats == NULL){
                printf("malloc failed\n");
                free(cmd_noise_status);
                return QAPI_ERROR;
            }

            memset(cmd_bmps_stats, 0, sizeof(WMI_BMPS_GET_STATS));
            memset(cmd_noise_status, 0, sizeof(WMI_GET_NOISE_STATUS));

            cmd_bmps_stats->bwindow_wait_close_time = NULL;
            cmd_bmps_stats->soc_active_sleep_time = NULL;

            cmd_bmps_stats->bwindow_wait_close_time = (pm_stats_active_sleep_time_record_buffer_t *)malloc(sizeof(pm_stats_active_sleep_time_record_buffer_t));
            if(cmd_bmps_stats->bwindow_wait_close_time == NULL){
                printf("malloc failed\n");
                free(cmd_bmps_stats);
                return QAPI_ERROR;
            }

            cmd_bmps_stats->soc_active_sleep_time = (pm_stats_active_sleep_time_record_buffer_t *)malloc(sizeof(pm_stats_active_sleep_time_record_buffer_t));
            if(cmd_bmps_stats->soc_active_sleep_time == NULL){
                printf("malloc failed\n");
                free(cmd_bmps_stats->bwindow_wait_close_time);
                free(cmd_bmps_stats);
                return QAPI_ERROR;
            }

            cmd_bmps_stats->tx_rx_counts = (pm_stats_tx_rx_counts_record_buffer_t *)malloc(sizeof(pm_stats_tx_rx_counts_record_buffer_t));
            if(cmd_bmps_stats->tx_rx_counts == NULL){
                printf("malloc failed\n");
                free(cmd_bmps_stats->soc_active_sleep_time);
                free(cmd_bmps_stats->bwindow_wait_close_time);
                free(cmd_bmps_stats);
                return QAPI_ERROR;
            }

            memset(cmd_bmps_stats->bwindow_wait_close_time, 0, sizeof(pm_stats_active_sleep_time_record_buffer_t));
            memset(cmd_bmps_stats->soc_active_sleep_time, 0, sizeof(pm_stats_active_sleep_time_record_buffer_t));
            memset(cmd_bmps_stats->tx_rx_counts, 0, sizeof(pm_stats_tx_rx_counts_record_buffer_t));
            printf("\n-----------Bwindow Closed/Wait Time Stats-----------\n");
            if(!qapi_bmps_get_bwindow_stats(cmd_bmps_stats->bwindow_wait_close_time)){
                
                if(cmd_bmps_stats->bwindow_wait_close_time->get_failed == true){
                    printf("get bwindow closed/wait time failed\n");
                }
                else{
                    printf("Latest Bwindow Closed/Wait Time Stats:\n");
                    printf("*   recorded time: %dms\n", cmd_bmps_stats->bwindow_wait_close_time->time_recorded_latest/1000);
                    printf("*   wait time: %dms\n", cmd_bmps_stats->bwindow_wait_close_time->accumulated_active_time_latest/1000);
                    printf("*   avg wait time: %d.%dms (good env:0~3ms; medium: 3~6ms; noisy: 6ms~)\n", \
                        cmd_bmps_stats->bwindow_wait_close_time->accumulated_active_time_latest/(cmd_bmps_stats->bwindow_wait_close_time->record_counts_latest * 1000), \
                        cmd_bmps_stats->bwindow_wait_close_time->accumulated_active_time_latest%(cmd_bmps_stats->bwindow_wait_close_time->record_counts_latest * 1000));
                    printf("*   closed time: %dms\n", cmd_bmps_stats->bwindow_wait_close_time->accumulated_sleep_time_latest/1000);
                    printf("*   bwindow closed duty cycle(closed_time/recorded_time) is:%u.%02u%%\n",
                        (uint16_t)((uint64_t)cmd_bmps_stats->bwindow_wait_close_time->accumulated_sleep_time_latest * 100ULL /
                                        cmd_bmps_stats->bwindow_wait_close_time->time_recorded_latest),
                        (uint16_t)(((uint64_t)cmd_bmps_stats->bwindow_wait_close_time->accumulated_sleep_time_latest * 100ULL %
                                        cmd_bmps_stats->bwindow_wait_close_time->time_recorded_latest) * 100ULL /
                                        cmd_bmps_stats->bwindow_wait_close_time->time_recorded_latest));

                    printf("\nTotal Bwindow Closed/Wait Time During All Time In BMPS:\n");
                    printf("*   recorded time: %dms\n", cmd_bmps_stats->bwindow_wait_close_time->time_recorded_total/1000);
                    printf("*   wait time: %dms\n", cmd_bmps_stats->bwindow_wait_close_time->accumulated_active_time_total/1000);
                    printf("*   avg wait time: %d.%dms (good env:0~3ms; medium: 3~6ms; noisy: 6ms~)\n", \
                        cmd_bmps_stats->bwindow_wait_close_time->accumulated_active_time_total/(cmd_bmps_stats->bwindow_wait_close_time->record_counts_total * 1000), \
                        cmd_bmps_stats->bwindow_wait_close_time->accumulated_active_time_total%(cmd_bmps_stats->bwindow_wait_close_time->record_counts_total * 1000));
                    printf("*   closed time: %dms\n", cmd_bmps_stats->bwindow_wait_close_time->accumulated_sleep_time_total/1000);
                    printf("*   bwindow closed duty cycle(closed_time/recorded_time) is:%u.%02u%%\n",
                        (uint16_t)((uint64_t)cmd_bmps_stats->bwindow_wait_close_time->accumulated_sleep_time_total * 100ULL /
                                        cmd_bmps_stats->bwindow_wait_close_time->time_recorded_total),
                        (uint16_t)(((uint64_t)cmd_bmps_stats->bwindow_wait_close_time->accumulated_sleep_time_total * 100ULL %
                                        cmd_bmps_stats->bwindow_wait_close_time->time_recorded_total) * 100ULL /
                                        cmd_bmps_stats->bwindow_wait_close_time->time_recorded_total));
                }
            }
            printf("\n-----------SoC Active/Sleep Time Stats-----------\n");
            if(!qapi_bmps_get_soc_stats(cmd_bmps_stats->soc_active_sleep_time)){
                if(cmd_bmps_stats->soc_active_sleep_time->get_failed == true){
                    printf("get soc active/sleep time failed\n");
                }
                else{
                    if(cmd_bmps_stats->soc_active_sleep_time->time_recorded_latest == 0){
                        printf("*   No data available for latest SoC stats, this may be because latest SoC sleep time exceeds the set recording period.\n");
                    }else{
                        printf("Latest SoC Active/Sleep Time Stats:\n");
                        printf("*   recorded time: %dms\n", cmd_bmps_stats->soc_active_sleep_time->time_recorded_latest/1000);
                        printf("*   active time: %dms\n", cmd_bmps_stats->soc_active_sleep_time->accumulated_active_time_latest/1000);
                        printf("*   avg active time: %d.%dms\n", \
                            cmd_bmps_stats->soc_active_sleep_time->accumulated_active_time_latest/(cmd_bmps_stats->soc_active_sleep_time->record_counts_latest * 1000), \
                            cmd_bmps_stats->soc_active_sleep_time->accumulated_active_time_latest%(cmd_bmps_stats->soc_active_sleep_time->record_counts_latest * 1000));
                        printf("*   sleep time: %dms\n", cmd_bmps_stats->soc_active_sleep_time->accumulated_sleep_time_latest/1000);
                        
                        printf("*   sleep duty cycle(sleep_time/recorded_time) is:%u.%02u%%\n",
                        (uint16_t)((uint64_t)cmd_bmps_stats->soc_active_sleep_time->accumulated_sleep_time_latest * 100ULL /
                                        cmd_bmps_stats->soc_active_sleep_time->time_recorded_latest),
                        (uint16_t)(((uint64_t)cmd_bmps_stats->soc_active_sleep_time->accumulated_sleep_time_latest * 100ULL %
                                        cmd_bmps_stats->soc_active_sleep_time->time_recorded_latest) * 100ULL /
                                        cmd_bmps_stats->soc_active_sleep_time->time_recorded_latest));
                    }
                    
                    printf("\nTotal SoC Active/Sleep Time During All Time In BMPS:\n");
                    printf("*   recorded time: %dms\n", cmd_bmps_stats->soc_active_sleep_time->time_recorded_total/1000);
                    printf("*   active time: %dms\n", cmd_bmps_stats->soc_active_sleep_time->accumulated_active_time_total/1000);
                    printf("*   avg active time: %d.%dms\n", \
                        cmd_bmps_stats->soc_active_sleep_time->accumulated_active_time_total/(cmd_bmps_stats->soc_active_sleep_time->record_counts_total * 1000), \
                        cmd_bmps_stats->soc_active_sleep_time->accumulated_active_time_total%(cmd_bmps_stats->soc_active_sleep_time->record_counts_total * 1000));
                    printf("*   sleep time: %dms\n", cmd_bmps_stats->soc_active_sleep_time->accumulated_sleep_time_total/1000);
                    printf("*   sleep duty cycle(sleep_time/recorded_time) is:%u.%02u%%\n",
                        (uint16_t)((uint64_t)cmd_bmps_stats->soc_active_sleep_time->accumulated_sleep_time_total * 100ULL /
                                        cmd_bmps_stats->soc_active_sleep_time->time_recorded_total),
                        (uint16_t)(((uint64_t)cmd_bmps_stats->soc_active_sleep_time->accumulated_sleep_time_total * 100ULL %
                                        cmd_bmps_stats->soc_active_sleep_time->time_recorded_total) * 100ULL /
                                        cmd_bmps_stats->soc_active_sleep_time->time_recorded_total));
                }
            }
            printf("\n-----------------Latest Tx Rx Counts Stats-----------------\n");
            if(!qapi_bmps_get_tx_rx_counts(cmd_bmps_stats->tx_rx_counts)){
                if(cmd_bmps_stats->tx_rx_counts->get_failed == true){
                    printf("*   No Tx Rx count available, this would because of no waking up during the BMPS\n");
                }else{
                    printf("*   Tx:%d, Rx:%d, duration:%dms\n", cmd_bmps_stats->tx_rx_counts->tx_counts_accumulated, \
                                                      cmd_bmps_stats->tx_rx_counts->rx_counts_accumulated, \
                                                    cmd_bmps_stats->tx_rx_counts->time_recorded/1000); 
                }
            }
            printf("\n-----------------Noise floor/PD Thr-----------------\n");
            if(!qapi_bmps_get_noise_status(cmd_noise_status)){
                printf("*   Noise floor -%ddbm\n", 100-cmd_noise_status->noise_floor); 
                printf("*   PD threshold -%ddbm\n", 100-cmd_noise_status->pd_threshold); 
            }

            free(cmd_bmps_stats->bwindow_wait_close_time);
            free(cmd_bmps_stats->soc_active_sleep_time);
            free(cmd_bmps_stats->tx_rx_counts);
            free(cmd_bmps_stats);
            free(cmd_noise_status);
            return QAPI_OK;
        }
        else{
            index++;
        }
    }

    return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
}
const QAPI_Console_Command_t lowpower_shell_cmds[] = {
    // cmd_function    cmd_string               usage_string             description
    {pm_enable, "pm_enable", "<1/0>", "Enable/disable system power management\n"},
    //    {test_sleep, "test_sleep", "<1:mcu_sleep|2:lightsleep> <sleep duration in us>", "Cfg and enable sleep\n"},
    {test_sleep, "test_sleep", "", "Not support now\n"},
    {deepsleep, "deepsleep", "<1:AON timer wkup|2:Ext wkup> <sleep duration in ms>", "Cfg and enable deepsleep\n"},
    {slp_clk_cal_cfg, "slp_clk_cal_cfg", "<1/0>", "Enable/disable slp_clk_cal in sleep mode\n"},
    {bmps_enable, "bmps_enable", "<1/0> [timeout in ms to exit BMPS]", "Enable BMPS(DTIM) sleep for WLAN\n"},
    {bmps_ignore_bcmc, "bmps_ignore_bcmc", "<1/0>", "Ignore Bcast/Mcast wakeup during BMPS(DTIM)\n"},
    {bmps_idle_time, "bmps_idle_time", "<Idle time in ms>", "Cfg max idle time prior entering into BMPS(DTIM) sleep\n"},
    {bmps_timing_cfg, "bmps_timing_cfg", "<preBcn in us> <bcnWait in us> <telePreBcnInc in us> <teleBcnWaitInc in us>",
     "Cfg BMPS timing parameters\n"},
    {imps_cfg, "imps_cfg",
     "<1:Enable|0:Disable> <sleep time in ms> <recnx timeout in ms> <cmd proc in ms> <cnx timeout in ms> <sleep mode: "
     "2:qapi_mcu_sleep | 3:qapi_standby>",
     "Cfg BMPS timing parameters\n"},
    {imps_sleep, "imps_sleep", "<1:Enable|0:Disable> <wait time in ms> <sleep time in ms> ",
     "Enable IMPS Directly,default mode is qapi_mcu_sleep\n"},
    {slp_clk_cal_act, "slp_clk_cal_act", "<1/0>", "Enable/disable slp_clk_cal in active mode\n"},
    {bmps_force_dtim, "bmps_force_dtim", "<Forced DTIM count>", "Force DTIM count\n"},
#ifdef CONFIG_CPR_ENABLE
    {cpr_enable, "cpr_enable", "<1/0>",
     "Enable CPR for Power save (This qcli is only for debugging. CPR enabled for default)\n"},
#endif  // CONFIG_CPR_ENABLE
    {bcmc_filter_enable, "bcmc_filter_enable", "<1|0> <log enable:1|0>", "enable or disable the bcmc filter\n"},
    {bcmc_filter_list, "bcmc_filter_list", "\n\nUsage: bcmc_filter_list -a [1|0] -u [dst udp port] -q\n\n",
     "bcmc_filter_list"},
    {bmps_cb_register, "bmps_cb_regiser", "<1|0>", "register|deregister callback function when pre-sleep/post-awake\n"},
    {bmps_period_awake, "bmps_period_awake", "<1/0> [period in ms to awake]", "Enable BMPS(DTIM) period awake\n"},
    {bmps_log_enable, "bmps_log_enable", "<1/0>", "Enable BMPS(DTIM) Logs\n"},
    {bmps_infinite_task, "bmps_infinite_task", "<1/0>", "bmps_infinite_task\n"},
    {bmps_power_optimization_enable, "bmps_power_optimization_enable", "<1/0>", "bmps_power_optimization_enable\n"},
    {bmps_compress_qos_null_enable, "bmps_compress_qos_null_enable", "<1/0>", "bmps_compress_qos_null_enable\n"},
    {bmps_stats, "bmps_stats", "bmps_stats get: to get the stats\nbmps_stats set <period>: to set the period to record the stats", "bmps_stats\n"},

};

const QAPI_Console_Command_Group_t lowpower_shell_cmd_group = {
    "lowpower", sizeof(lowpower_shell_cmds) / sizeof(QAPI_Console_Command_t), lowpower_shell_cmds};

QAPI_Console_Group_Handle_t lowpower_shell_cmd_group_handle;

void lowpower_shell_init(void)
{
    lowpower_shell_cmd_group_handle = QAPI_Console_Register_Command_Group(NULL, &lowpower_shell_cmd_group);
}
