/*
#Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
#SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <string.h>
#include <stdint.h>

#include <stdio.h>
#include <stdlib.h>

#include "qcli.h"
#include "qcli_api.h"

#include "wmi.h"
#include "FreeRTOS.h"
#include "semphr.h"

#include "nt_wfm_wmi_interface.h"
#include <nt_ftm.h>

#include "wlan_dev.h"
#ifdef CONFIG_FWUP_DEMO
#include "ota_demo.h"
#endif
#if CONFIG_MPU_DEMO
#include "mpu_demo.h"
#endif
#ifdef CONFIG_MGMT_FILTER_DEMO
#include "mgmt_filter_demo.h"
#endif

#ifdef FERMION_SILICON
extern uint32_t UART_Send_direct(char *txbuf, uint32_t buflen);
#define UART_SEND_DIRECT(str) UART_Send_direct((str), strlen(str))
#else
#define UART_SEND_DIRECT(str)
#endif

static volatile int dead_loop = 0;

/*
 * RTT/FTM QCLI demo
 */
extern qurt_pipe_t msg_wfm_wmi_id;

typedef struct {
    uint64_t t1;
    uint64_t t2;
    uint64_t t3;
    uint64_t t4;
    uint16_t fac;
    uint32_t base_delay;    /* from PHY/BDF (phyRTT_enable) */
    int32_t  rtt;           /* corrected RTT (time unit follows nt_rtt_calculation) */
    int32_t  tof;           /* rtt/2 */
    uint32_t distance_mm;   /* derived (approx) */
    uint8_t  valid;
    uint8_t  seq;
} rtt_sample_t;

static SemaphoreHandle_t g_rtt_sync_sem = NULL;
static uint32_t g_last_rtt_distance_cm = 0;
static int g_rtt_status = -1; /* 0 = Success, -1 = Fail */
QCLI_Group_Handle_t qcli_rtt_group;

/* Demo-side config cache (default) */
static usr_ftm g_cfg = {
    .ftm_mode = FTM_INITIATOR,
    .location = 0,
    .location_type = 0,
    .asap = 1,
    .ftms_per_burst = 4,
    .no_of_bur_exp = 0,
    .min_delta_ftm = 20,
    .burst_duration = 15,
    .burst_period = 0,
    .cal_val = 0,
    .format_and_bw = 0,
};
extern uint32_t g_dynamic_base_delay;
void nt_rtt_demo_notify(uint64_t dist_cm)
{
    g_last_rtt_distance_cm = (uint32_t)dist_cm;
    g_rtt_status = 0;

    if (g_rtt_sync_sem != NULL) {
        xSemaphoreGive(g_rtt_sync_sem);
    } else {
        printf("RTT Async: %u cm", (uint32_t)dist_cm);
    }
}

static void rtt_demo_print_connected_bssid(void)
{
#ifdef NT_DEV_STA_ID
    extern dev_common_t *gpDevCommon;
    if (gpDevCommon && gpDevCommon->devp[NT_DEV_STA_ID] && gpDevCommon->devp[NT_DEV_STA_ID]->bss) {
        devh_t *dev = gpDevCommon->devp[NT_DEV_STA_ID];
        uint8_t *bssid = dev->bss->ni_bssid;
        printf("RTT: Connected BSSID=%02x:%02x:%02x:%02x:%02x:%02x\n",
               bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
        return;
    }
#endif
    printf("RTT: Connected BSSID=unknown");
}

static int util_parse_mac(const char *mac_str, uint8_t *mac_out)
{
    int values[6];
    if (sscanf(mac_str, "%x:%x:%x:%x:%x:%x",
               &values[0], &values[1], &values[2],
               &values[3], &values[4], &values[5]) == 6)
    {
        for (int i = 0; i < 6; ++i)
            mac_out[i] = (uint8_t)values[i];
        return 0;
    }
    return -1;
}

static int wmi_send_cmd(uint16_t cmd_id, void *data, uint16_t len)
{
    wmi_msg_struct_t msg;
    memset(&msg, 0x0, sizeof(msg));
    msg.msg_struct.return_status = eWiFiNotSupported;
    msg.trans_wmi_message_id = cmd_id;
    msg.msg_struct.vo_data = data;
    msg.msg_struct.vo_data_len = len;

    if (QURT_EOK != qurt_pipe_send_timed(msg_wfm_wmi_id, &msg, pdMS_TO_TICKS(100)))
    {
        printf("Error: Failed to send WMI command 0x%x", cmd_id);
        return -1;
    }
    return 0;
}

/* =========================================================================
 * CLI Commands
 * ========================================================================= */
static QCLI_Command_Status_t cmd_RTT_Cfg(uint32_t Parameter_Count, QCLI_Parameter_t *Parameter_List)
{
    /* Example: rtt_cfg ftms=8 delta=20 asap=1 bw=0 cal=0x0 */
    for (uint32_t i = 0; i < Parameter_Count; i++) {
        const char *s = Parameter_List[i].String_Value;
        if (s == NULL || s[0] == 0) {
            continue;
        }

        if (strncmp(s, "ftms=", 5) == 0) g_cfg.ftms_per_burst = (uint8_t)atoi(s + 5);
        else if (strncmp(s, "delta=", 6) == 0) g_cfg.min_delta_ftm = (uint8_t)atoi(s + 6);
        else if (strncmp(s, "asap=", 5) == 0) g_cfg.asap = (uint8_t)atoi(s + 5);
        else if (strncmp(s, "bw=", 3) == 0) g_cfg.format_and_bw = (uint8_t)atoi(s + 3);
        else if (strncmp(s, "cal=", 4) == 0) g_cfg.cal_val = (uint32_t)strtoul(s + 4, NULL, 0);
        else if (strncmp(s, "dur=", 4) == 0) g_cfg.burst_duration = (uint8_t)atoi(s + 4);
        else if (strncmp(s, "period=", 7) == 0) g_cfg.burst_period = (uint16_t)atoi(s + 7);
    }

    printf("RTT CFG: ftms=%d delta=%d asap=%d bw=%d dur=%d period=%d cal=0x%x",
           g_cfg.ftms_per_burst, g_cfg.min_delta_ftm, g_cfg.asap, g_cfg.format_and_bw,
           g_cfg.burst_duration, g_cfg.burst_period, g_cfg.cal_val);
    return QCLI_STATUS_SUCCESS_E;
}
extern uint8_t g_rtt_index;
extern uint8_t g_ftm_number;
static QCLI_Command_Status_t cmd_RTT_Start(uint32_t Parameter_Count, QCLI_Parameter_t *Parameter_List)
{
    uint8_t target_mac[6];
    uint32_t burst_cnt = g_cfg.ftms_per_burst;
    g_ftm_number = g_cfg.ftms_per_burst;
    g_rtt_index = 0;

    if (Parameter_Count > 1 && Parameter_List[1].Integer_Is_Valid) {
        burst_cnt = (uint32_t)Parameter_List[1].Integer_Value;
    }

    rtt_demo_print_connected_bssid();

    if (g_rtt_sync_sem == NULL) {
        g_rtt_sync_sem = xSemaphoreCreateBinary();
    }

    xSemaphoreTake(g_rtt_sync_sem, 0);
    g_rtt_status = -1;

    usr_ftm ftm_config = g_cfg;
    ftm_config.ftm_mode = FTM_INITIATOR;
    ftm_config.ftms_per_burst = (uint8_t)burst_cnt;

    if (wmi_send_cmd(WMI_SET_RTT_CFG, &ftm_config, sizeof(ftm_config)) != 0) {
        return QCLI_STATUS_ERROR_E;
    }

    if (wmi_send_cmd(WMI_SEND_FTM_FRAME, NULL, 0) != 0) {
        return QCLI_STATUS_ERROR_E;
    }

    // if (xSemaphoreTake(g_rtt_sync_sem, pdMS_TO_TICKS(5000)) == pdTRUE) {
    //     if (g_rtt_status == 0) {
    //         printf("RTT SUCCESS: Distance = %u cm (%u mm)\n",
    //             (unsigned)(g_last_rtt_distance_mm / 10),
    //             (unsigned)g_last_rtt_distance_mm);
    //         return QCLI_STATUS_SUCCESS_E;
    //     }
    //     printf("RTT FAILED: no sample produced. Try rtt_dump.");
    //     return QCLI_STATUS_ERROR_E;
    // }
    printf("g_rtt_index=%d, g_ftm_number=%d\r\n", g_rtt_index, g_ftm_number);
    return QCLI_STATUS_SUCCESS_E;
}

static QCLI_Command_Status_t cmd_RTT_Status(uint32_t Parameter_Count, QCLI_Parameter_t *Parameter_List)
{
    (void)Parameter_Count;
    (void)Parameter_List;

    printf("RTT STATUS: last=%u cm, status=%d ", g_last_rtt_distance_cm, g_rtt_status);
    printf("RTT CFG: ftms=%d delta=%d asap=%d bw=%d dur=%d period=%d cal=0x%x",
           g_cfg.ftms_per_burst, g_cfg.min_delta_ftm, g_cfg.asap, g_cfg.format_and_bw,
           g_cfg.burst_duration, g_cfg.burst_period, g_cfg.cal_val);

    return QCLI_STATUS_SUCCESS_E;
}

QCLI_Command_Status_t cmd_set_rtt_base_delay(uint32_t Parameter_Count, QCLI_Parameter_t *Parameter_List) {
    if (Parameter_Count < 1) {
        return QCLI_STATUS_USAGE_E;
    }
    
    g_dynamic_base_delay = (uint32_t)atoi((char*)Parameter_List[0].String_Value);
    
    printf("Success: Dynamic base_delay updated to %d\n", g_dynamic_base_delay);
    
    return QCLI_STATUS_SUCCESS_E;
}

const QCLI_Command_t rtt_cmd_list[] =
{
    {cmd_RTT_Cfg,   "rtt_cfg",   "[ftms=N] [delta=N] [asap=0|1] [bw=N] [dur=N] [period=N] [cal=HEX]", "Configure RTT/FTM"},
    {cmd_RTT_Start, "rtt_start", "<mac_addr> [count]", "Start RTT/FTM measurement"},
    {cmd_RTT_Status,"rtt_status","", "Show RTT status"},
    { cmd_set_rtt_base_delay, "set_base_delay", "set_base_delay <value>", "Set RTT base delay dynamically"},
};

const QCLI_Command_Group_t rtt_cmd_group =
{
    "RTT",
    sizeof(rtt_cmd_list) / sizeof(QCLI_Command_t),
    rtt_cmd_list
};

void Initialize_RTT_Demo(void)
{
    extern void nt_rtt_register_notify_callback(void (*callback)(uint64_t));
    nt_rtt_register_notify_callback(nt_rtt_demo_notify);
    
    qcli_rtt_group = QCLI_Register_Command_Group(NULL, &rtt_cmd_group);
    if (qcli_rtt_group) {
        printf("RTT Demo Initialized.");
    } else {
        printf("RTT Demo Register FAIL!");
    }
}

void app_init(void)
{
    UART_SEND_DIRECT("app_init entry\r\n");
    // register app console commands here if have
#ifdef CONFIG_FWUP_DEMO
    Initialize_FwUpgrade_Demo();
#endif
    Initialize_Crypto_Demo();
#ifdef CONFIG_QCSPI_HFC_TEST
    extern void Initialize_qcspi_hfc_Demo(void);
    Initialize_qcspi_hfc_Demo();
#endif
#if CONFIG_MPU_DEMO
    Initialize_MPU_Demo();
#endif
#ifdef CONFIG_MGMT_FILTER_DEMO
    Initialize_Mgmt_Filter_Demo();
#endif
#if defined(CONFIG_MBEDTLS_AES_ALT) || defined(CONFIG_MBEDTLS_CCM_ALT) || defined(CONFIG_MBEDTLS_SHA_ALT)
    Initialize_Qcc_Demo();
#endif
#ifdef CONFIG_SECUREFS_DEMO
    Initialize_SecureFs_Demo();
#endif
    UART_SEND_DIRECT("app_init over\r\n");
}

void app_main(void)
{
    UART_SEND_DIRECT("app_main entry\r\n");
    UART_SEND_DIRECT("qcli demo!\r\n");
    Initialize_RTT_Demo();
    if (dead_loop) {
        UART_SEND_DIRECT("Dead loop...\r\n");
        while (dead_loop)
            ;
    }
    UART_SEND_DIRECT("app_main over\r\n");
}
