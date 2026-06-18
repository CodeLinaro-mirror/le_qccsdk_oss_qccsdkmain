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
#include "com_api.h"
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

/* FTM measurement counters (defined in nt_ftm.c) — used by cmd_RTT_Start */
extern uint8_t g_rtt_index;
extern uint8_t g_ftm_number;

#ifdef NT_FN_RTT_DEMO

/* ==========================================================================
 * TCP report task — connects to Android App and exchanges JSON messages
 *
 * Usage:  rtt_tcp_start <server_ip> <port>
 *   e.g.  rtt_tcp_start 192.168.43.1 8080
 *
 * Protocol (newline-delimited text/JSON):
 *   App -> 730: rtt_scan | rtt_ap_clear | rtt_ap_add <bssid> <ch> <x> <y> | rtt_locate [ftms] [bw]
 *   730 -> App: {"type":"scan_result","aps":[...]}
 *               {"type":"ftm_result","status":N,"mac":"..","distance_mm":N,...}
 * ========================================================================== */
#include "sockets.h"
#include <errno.h>
#include "lwip/netif.h"
#include "lwip/ip4_addr.h"
#include "lwip/dhcp.h"
#include "qapi_wlan.h"
#include "nt_common.h"

#define TCP_REPORT_TAG   "[TCP] "
#define RTT_WLAN_DEV_ID  1          /* STA device ID for scan/connect */
#define RTT_TCP_MAX_APS  8

/* Result storage filled by _tcp_range_ap_done_cb, read by tcp_session */
static int32_t           g_tcp_ap_dist_mm  = 0;
static int               g_tcp_ap_status   = 1;
static uint8_t           g_tcp_ap_bssid[6] = {0};
static SemaphoreHandle_t g_tcp_ap_sem      = NULL;

/* TCP session globals — accessed by nt_rtt_demo_notify */
static int     g_tcp_sock             = -1;
static uint8_t g_tcp_current_bssid[6] = {0};
static char    g_tcp_current_ssid[33] = {0};

/* RTT sample arrays (defined in nt_ftm.c) */
#define RTT_MAX_LENGTH 10
extern uint32_t RTT_TOD[RTT_MAX_LENGTH];
extern uint32_t RTT_TOA[RTT_MAX_LENGTH];
extern uint32_t FAC[RTT_MAX_LENGTH];
extern uint8_t  RTT_VALID[RTT_MAX_LENGTH];

typedef struct {
    uint8_t bssid[6];
    uint8_t channel;
    char    ssid[33];
    float   x_m;
    float   y_m;
} tcp_ap_entry_t;

static volatile int      g_tcp_stop         = 0;
static SemaphoreHandle_t g_tcp_range_sem     = NULL;
static tcp_ap_entry_t    g_tcp_aps[RTT_TCP_MAX_APS];
static int               g_tcp_ap_cnt        = 0;
static TaskHandle_t      g_tcp_task_handle   = NULL;
static char              g_tcp_server_ip[40] = {0};
static uint16_t          g_tcp_server_port   = 0;

/* Pending result: saved when FTM completes while TCP socket is broken
 * (ch36 channel switch during unassoc FTM causes TCP on ch52 to disconnect).
 * tcp_session() delivers it on the next reconnect before waiting for new commands. */
static char g_pending_result[512]  = {0};
static int  g_pending_result_valid = 0;

static void tcp_send_line(int sock, const char *line);

static int32_t rtt_calc_std_dev_mm(void)
{
    extern uint64_t nt_rtt_calculation(uint32_t t1, uint32_t t2,
                                        uint32_t t3, uint32_t t4,
                                        uint32_t fac);
    extern uint64_t nt_distance_cal(uint64_t rtt);

    int n = (int)g_rtt_index;
    if (n < 2) return 0;

    int64_t samples[RTT_MAX_LENGTH];
    int cnt = 0;
    int64_t sum = 0;
    for (int i = 0; i < n && i < RTT_MAX_LENGTH; i++) {
        if (!(RTT_VALID[i] & 0x03)) continue;
        uint64_t rtt = nt_rtt_calculation(0, RTT_TOA[i], RTT_TOD[i], 0, FAC[i]);
        if (rtt > (uint64_t)0x7FFFFFFFFFFFFFFF) continue;
        uint64_t dist_cm = nt_distance_cal(rtt);
        if (dist_cm > 100000ULL) continue;
        samples[cnt] = (int64_t)dist_cm * 10;
        sum += samples[cnt];
        cnt++;
    }
    if (cnt < 2) return 0;

    int64_t mean = sum / cnt;
    int64_t var = 0;
    for (int i = 0; i < cnt; i++) {
        int64_t d = samples[i] - mean;
        var += d * d;
    }
    var /= cnt;

    if (var <= 0) return 0;
    int64_t x = var;
    int64_t y = (x + 1) / 2;
    while (y < x) { x = y; y = (x + var / x) / 2; }
    return (int32_t)x;
}
#endif /* NT_FN_RTT_DEMO */

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
static int64_t g_last_rtt_distance_mm = 0;
static int g_rtt_status = -1; /* 0 = Success, -1 = Fail */
QCLI_Group_Handle_t qcli_rtt_group;

/* Demo-side config cache (default) */
static usr_ftm g_cfg = {
    .ftm_mode = FTM_INITIATOR,
    .location = 0,
    .location_type = 0,
    .asap = 1,
    .ftms_per_burst = 8,
    .no_of_bur_exp = 0,
    .min_delta_ftm = 20,
    .burst_duration = 15,
    .burst_period = 0,
    .cal_val = 0,
    .format_and_bw = 0,
};
#ifdef NT_FN_RTT_DEMO
extern uint32_t g_phy_delay;
extern uint32_t g_ap_offset;
#define g_dynamic_base_delay g_ap_offset
#else
extern uint32_t g_dynamic_base_delay;
#endif
void nt_rtt_demo_notify(uint64_t dist_cm)
{
    if (dist_cm > 100000ULL)
        g_last_rtt_distance_mm = -1;
    else
        g_last_rtt_distance_mm = (int64_t)dist_cm * 10;

    g_rtt_status = 0;

#ifdef NT_FN_RTT_DEMO
    /* Send result via TCP only when connected and NOT in unassoc FTM mode */
    if (g_tcp_sock >= 0 && g_tcp_ap_sem == NULL) {
        char line[256];
        int32_t dist_send = (g_last_rtt_distance_mm < 0 || g_last_rtt_distance_mm > 0x7FFFFFFF)
                            ? -1 : (int32_t)g_last_rtt_distance_mm;
        int status       = (dist_send > 0) ? 0 : 1;
        int32_t std_dev  = rtt_calc_std_dev_mm();
        uint32_t ts_ms   = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);

        snprintf(line, sizeof(line),
                 "{\"type\":\"ftm_result\","
                 "\"status\":%d,"
                 "\"mac\":\"%02x:%02x:%02x:%02x:%02x:%02x\","
                 "\"ssid\":\"%s\","
                 "\"distance_mm\":%d,"
                 "\"distance_std_dev_mm\":%d,"
                 "\"rssi\":0,"
                 "\"num_attempted_measurements\":%d,"
                 "\"num_successful_measurements\":%d,"
                 "\"ranging_timestamp_ms\":%u}",
                 status,
                 g_tcp_current_bssid[0], g_tcp_current_bssid[1], g_tcp_current_bssid[2],
                 g_tcp_current_bssid[3], g_tcp_current_bssid[4], g_tcp_current_bssid[5],
                 g_tcp_current_ssid,
                 dist_send,
                 std_dev,
                 (int)g_ftm_number,
                 (int)g_rtt_index,
                 ts_ms);
        tcp_send_line(g_tcp_sock, line);
        printf(TCP_REPORT_TAG "notify sent: %s\r\n", line);
    }
#endif /* NT_FN_RTT_DEMO */

    if (g_rtt_sync_sem != NULL)
        xSemaphoreGive(g_rtt_sync_sem);
    else
        printf("RTT Async: %d mm\r\n", (int)g_last_rtt_distance_mm);
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

    printf("RTT STATUS: last=%lld mm, status=%d ",
           (long long)g_last_rtt_distance_mm, g_rtt_status);
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
    printf("RTT: ap_offset updated to %d ps\n", g_dynamic_base_delay);
    return QCLI_STATUS_SUCCESS_E;
}

#ifdef NT_FN_RTT_DEMO
/*
 * Unassociated multi-AP FTM commands
 * Usage:
 *   rtt_ap_add  <bssid> <channel> <x_m> <y_m>
 *   rtt_ap_clear
 *   rtt_range  <bssid> <channel> <x_m> <y_m> //for single AP test
 *   rtt_locate  [ftms_per_burst] [format_bw]
 */
static void _locate_done_cb(const unassoc_ftm_position_t *pos, void *ctx)
{
    (void)ctx;
    if (pos && pos->valid) {
        int xi = (int)pos->x_m;
        int xd = (int)((pos->x_m - (float)xi) * 100.0f);
        if (xd < 0) xd = -xd;
        int yi = (int)pos->y_m;
        int yd = (int)((pos->y_m - (float)yi) * 100.0f);
        if (yd < 0) yd = -yd;
        int ei = (int)pos->error_m;
        int ed = (int)((pos->error_m - (float)ei) * 100.0f);
        if (ed < 0) ed = -ed;
        printf("\r\n[LOCATE] Position: X=%d.%02d m  Y=%d.%02d m  err=%d.%02d m\r\n",
               xi, xd, yi, yd, ei, ed);
    } else {
        printf("\r\n[LOCATE] Positioning failed (not enough valid measurements)\r\n");
    }
}

/**
 * rtt_ap_add <bssid_hex> <channel> <x_m> <y_m>
 * Example: rtt_ap_add AA:BB:CC:DD:EE:FF 6 0.0 5.0
 */
static QCLI_Command_Status_t cmd_rtt_ap_add(uint32_t Parameter_Count,
                                             QCLI_Parameter_t *Parameter_List)
{
    if (Parameter_Count < 4) {
        printf("Usage: rtt_ap_add <bssid> <channel> <x_m> <y_m>\r\n");
        return QCLI_STATUS_USAGE_E;
    }

    /* Parse BSSID: accept "AA:BB:CC:DD:EE:FF" format */
    uint8_t bssid[6];
    const char *s = (const char *)Parameter_List[0].String_Value;
    unsigned int v[6] = {0, 0, 0, 0, 0, 0};
    int parsed = 0;

    if (strlen(s) == 17) {
        parsed = sscanf(s, "%x:%x:%x:%x:%x:%x",
                        &v[0], &v[1], &v[2], &v[3], &v[4], &v[5]);
    } else if (strlen(s) == 12) {
        /* AABBCCDDEEFF format: parse each byte manually */
        parsed = 0;
        for (int i = 0; i < 6; i++) {
            unsigned int byte_val = 0;
            if (sscanf(s + i * 2, "%2x", &byte_val) == 1) {
                v[i] = byte_val;
                parsed++;
            }
        }
    }

    if (parsed != 6) {
        printf("rtt_ap_add: invalid BSSID format (expected AA:BB:CC:DD:EE:FF)\r\n");
        return QCLI_STATUS_ERROR_E;
    }
    for (int i = 0; i < 6; i++) {
        bssid[i] = (uint8_t)(v[i] & 0xFF);
    }

    uint8_t channel = (uint8_t)atoi((char *)Parameter_List[1].String_Value);
    float   x_m     = (float)atof((char *)Parameter_List[2].String_Value);
    float   y_m     = (float)atof((char *)Parameter_List[3].String_Value);

    if (nt_unassoc_ftm_add_ap(bssid, channel, x_m, y_m) != 0) {
        printf("rtt_ap_add: AP table full\r\n");
        return QCLI_STATUS_ERROR_E;
    }

    int x_int = (int)x_m;
    int x_dec = (int)((x_m - (float)x_int) * 10.0f);
    if (x_dec < 0) x_dec = -x_dec;
    int y_int = (int)y_m;
    int y_dec = (int)((y_m - (float)y_int) * 10.0f);
    if (y_dec < 0) y_dec = -y_dec;
    printf("rtt_ap_add: AP added (ch=%u x=%d.%d y=%d.%d)\r\n",
           channel, x_int, x_dec, y_int, y_dec);
    return QCLI_STATUS_SUCCESS_E;
}

/**
 * rtt_ap_clear
 * Remove all registered anchor APs.
 */
static QCLI_Command_Status_t cmd_rtt_ap_clear(uint32_t Parameter_Count,
                                               QCLI_Parameter_t *Parameter_List)
{
    (void)Parameter_Count;
    (void)Parameter_List;
    nt_unassoc_ftm_clear_aps();
    printf("rtt_ap_clear: AP list cleared\r\n");
    return QCLI_STATUS_SUCCESS_E;
}

/**
 * rtt_range <bssid> <channel> [ftms] [bw]
 * Single-AP unassociated FTM ranging (no trilateration needed).
 * Useful for debugging: tests whether the AP responds to FTM at all.
 * Result is printed via the locate done callback (valid=0 on timeout).
 */
static void _range_done_cb(const unassoc_ftm_position_t *pos, void *ctx)
{
    (void)pos;
    (void)ctx;
    const unassoc_ftm_ap_entry_t *ap = nt_unassoc_ftm_get_ap(0);
    if (ap && ap->measured && ap->distance_mm > 0) {
        uint32_t dist_cm  = (uint32_t)(ap->distance_mm / 10);
        uint32_t dist_int = dist_cm / 100;
        uint32_t dist_dec = dist_cm % 100;
        printf("\r\n[RANGE] Distance: %u.%02u m  (%u mm)\r\n",
               dist_int, dist_dec, (uint32_t)ap->distance_mm);
    } else {
        printf("\r\n[RANGE] Measurement failed (AP did not respond or timed out)\r\n");
        printf("[RANGE] Possible causes:\r\n");
        printf("  1. AP does not support FTM (check AP firmware/config)\r\n");
        printf("  2. AP requires association before FTM (not all APs support unassoc FTM)\r\n");
        printf("  3. Wrong channel number (check with rtt_ap_add ch=<N>)\r\n");
        printf("  4. AP MAC address mismatch\r\n");
    }
}

static QCLI_Command_Status_t cmd_rtt_range(uint32_t Parameter_Count,
                                            QCLI_Parameter_t *Parameter_List)
{
    if (Parameter_Count < 2) {
        printf("Usage: rtt_range <bssid> <channel> [ftms=3] [bw=4]\r\n");
        printf("  bssid:   AP MAC, e.g. 3e:a1:27:10:10:81\r\n");
        printf("  channel: 802.11 channel, e.g. 48 (5GHz) or 6 (2.4GHz)\r\n");
        printf("  ftms:    FTM frames per burst (default 3)\r\n");
        printf("  bw:      format_and_bw code (default 4 = HT20)\r\n");
        return QCLI_STATUS_USAGE_E;
    }

#ifdef NT_DEV_STA_ID
    extern dev_common_t *gpDevCommon;
    if (!gpDevCommon || !gpDevCommon->devp[NT_DEV_STA_ID]) {
        printf("rtt_range: device not available\r\n");
        return QCLI_STATUS_ERROR_E;
    }
    devh_t *dev = gpDevCommon->devp[NT_DEV_STA_ID];
#else
    extern devh_t *gdevp;
    devh_t *dev = gdevp;
#endif

    uint8_t bssid[6];
    const char *s = (const char *)Parameter_List[0].String_Value;
    unsigned int v[6] = {0};
    int parsed = 0;
    if (strlen(s) == 17) {
        parsed = sscanf(s, "%x:%x:%x:%x:%x:%x",
                        &v[0], &v[1], &v[2], &v[3], &v[4], &v[5]);
    } else if (strlen(s) == 12) {
        parsed = 0;
        for (int i = 0; i < 6; i++) {
            unsigned int bv = 0;
            if (sscanf(s + i * 2, "%2x", &bv) == 1) { v[i] = bv; parsed++; }
        }
    }
    if (parsed != 6) {
        printf("rtt_range: invalid BSSID format\r\n");
        return QCLI_STATUS_ERROR_E;
    }
    for (int i = 0; i < 6; i++) bssid[i] = (uint8_t)(v[i] & 0xFF);

    uint8_t channel = (uint8_t)atoi((char *)Parameter_List[1].String_Value);
    uint8_t ftms    = (Parameter_Count >= 3)
                      ? (uint8_t)atoi((char *)Parameter_List[2].String_Value) : 3;
    uint8_t bw      = (Parameter_Count >= 4)
                      ? (uint8_t)atoi((char *)Parameter_List[3].String_Value) : 4;

    nt_unassoc_ftm_clear_aps();
    if (nt_unassoc_ftm_add_ap(bssid, channel, 0.0f, 0.0f) != 0) {
        printf("rtt_range: failed to add AP\r\n");
        return QCLI_STATUS_ERROR_E;
    }

    printf("rtt_range: starting unassoc FTM to %02x:%02x:%02x:%02x:%02x:%02x ch=%u ftms=%u bw=%u\r\n",
           bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5],
           channel, ftms, bw);

    int rc = nt_unassoc_ftm_start(dev, ftms, bw, _range_done_cb, NULL);
    if (rc != 0) {
        printf("rtt_range: failed to start (rc=%d)\r\n", rc);
        return QCLI_STATUS_ERROR_E;
    }
    printf("rtt_range: waiting for result (timeout=%d ms)...\r\n",
           UNASSOC_FTM_SESSION_TIMEOUT_MS);
    return QCLI_STATUS_SUCCESS_E;
}

/**
 * rtt_locate [ftms_per_burst] [format_bw]
 * Start sequential unassociated FTM ranging and compute 2-D position.
 */
static QCLI_Command_Status_t cmd_rtt_locate(uint32_t Parameter_Count,
                                             QCLI_Parameter_t *Parameter_List)
{
#ifdef NT_DEV_STA_ID
    extern dev_common_t *gpDevCommon;
    if (!gpDevCommon || !gpDevCommon->devp[NT_DEV_STA_ID]) {
        printf("rtt_locate: device not available\r\n");
        return QCLI_STATUS_ERROR_E;
    }
    devh_t *dev = gpDevCommon->devp[NT_DEV_STA_ID];
#else
    extern devh_t *gdevp;
    devh_t *dev = gdevp;
#endif

    uint8_t ftms = (Parameter_Count >= 1)
                   ? (uint8_t)atoi((char *)Parameter_List[0].String_Value) : 3;
    uint8_t bw   = (Parameter_Count >= 2)
                   ? (uint8_t)atoi((char *)Parameter_List[1].String_Value) : 4;

    int rc = nt_unassoc_ftm_start(dev, ftms, bw, _locate_done_cb, NULL);
    if (rc != 0) {
        printf("rtt_locate: failed to start (rc=%d)\r\n", rc);
        return QCLI_STATUS_ERROR_E;
    }
    printf("rtt_locate: ranging started (ftms=%u bw=%u)\r\n", ftms, bw);
    return QCLI_STATUS_SUCCESS_E;
}

QCLI_Command_Status_t cmd_set_ap_type(uint32_t Parameter_Count, QCLI_Parameter_t *Parameter_List) {
    if (Parameter_Count < 1) {
        printf("Usage: set_ap_type <hk|wkk|default> [2g|5g]\n");
        printf("  hk      HK 10-YE079-300: 2G=35085ps 5G=12267ps\n");
        printf("  wkk     WKK QCN9224:     ch36=36196ps ch48=44917ps\n");
        printf("  default no offset (0)\n");
        return QCLI_STATUS_USAGE_E;
    }
    const char *ap = (const char*)Parameter_List[0].String_Value;
    int is_5g = (Parameter_Count >= 2 &&
                 strcasecmp((const char*)Parameter_List[1].String_Value, "5g") == 0);

    if (strcasecmp(ap, "hk") == 0) {
        g_ap_offset = is_5g ? 12267 : 35085;
    } else if (strcasecmp(ap, "wkk") == 0) {
        g_ap_offset = is_5g ? 36196 : 37514;
    } else {
        g_ap_offset = 0;
    }
    printf("RTT: ap_type=%s band=%s ap_offset=%d ps  phy_delay=%d ps\n",
           ap, is_5g ? "5G" : "2G", g_ap_offset, g_phy_delay);
    return QCLI_STATUS_SUCCESS_E;
}

QCLI_Command_Status_t cmd_set_phy_delay(uint32_t Parameter_Count, QCLI_Parameter_t *Parameter_List) {
    if (Parameter_Count < 1) {
        printf("RTT: phy_delay=%d ps  ap_offset=%d ps\n", g_phy_delay, g_ap_offset);
        return QCLI_STATUS_SUCCESS_E;
    }
    g_phy_delay = (uint32_t)atoi((char*)Parameter_List[0].String_Value);
    printf("RTT: phy_delay updated to %d ps\n", g_phy_delay);
    return QCLI_STATUS_SUCCESS_E;
}

/* Send one null-terminated line over TCP (append '\n') */
static void tcp_send_line(int sock, const char *line)
{
    int len = strlen(line);
    char *buf = malloc(len + 2);
    if (!buf) return;
    memscpy(buf, len + 2, line, len);
    buf[len]     = '\n';
    buf[len + 1] = '\0';
    send(sock, buf, len + 1, 0);
    free(buf);
}

static void _tcp_range_ap_done_cb(const unassoc_ftm_position_t *pos, void *ctx)
{
    (void)pos; (void)ctx;
    printf(TCP_REPORT_TAG "done_cb called\r\n");
    const unassoc_ftm_ap_entry_t *ap = nt_unassoc_ftm_get_ap(0);
    if (ap) {
        printf(TCP_REPORT_TAG "done_cb: ap found measured=%d raw_dist_mm=%u\r\n",
               (int)ap->measured, (unsigned)ap->distance_mm);
        g_tcp_ap_status  = ap->measured ? 0 : 1;
        g_tcp_ap_dist_mm = ap->measured ? (int32_t)ap->distance_mm : -1;
        memscpy(g_tcp_ap_bssid, 6, ap->bssid, 6);
    } else {
        printf(TCP_REPORT_TAG "done_cb: ap entry NULL\r\n");
    }
    printf(TCP_REPORT_TAG "done_cb: result dist=%d mm status=%d\r\n",
           g_tcp_ap_dist_mm, g_tcp_ap_status);
    if (g_tcp_ap_sem)
        xSemaphoreGive(g_tcp_ap_sem);
}

static void tcp_do_scan(int sock)
{
    qapi_WLAN_Scan_Comp_Evt_t hdr = {0};
    int16_t bss_cnt = 0;

    printf(TCP_REPORT_TAG "scanning...\r\n");
    qapi_WLAN_Start_Scan(RTT_WLAN_DEV_ID, NULL);
    qapi_WLAN_Get_Scan_Results(RTT_WLAN_DEV_ID, &hdr, &bss_cnt);
    bss_cnt = hdr.num_bss_cur;

    if (bss_cnt <= 0) {
        printf(TCP_REPORT_TAG "scan: 0 APs found\r\n");
        return;
    }

    qapi_WLAN_Scan_Comp_Evt_t *evt = malloc(
        sizeof(qapi_WLAN_Scan_Comp_Evt_t) +
        bss_cnt * sizeof(qapi_WLAN_BSS_Scan_Info_t));
    if (!evt) return;

    qapi_WLAN_Get_Scan_Results(RTT_WLAN_DEV_ID, evt, &bss_cnt);

    int json_sz = 64 + bss_cnt * 100;
    char *line = malloc(json_sz);
    if (line) {
        int pos = 0;
        pos += snprintf(line + pos, json_sz - pos,
                        "{\"type\":\"scan_result\",\"aps\":[");
        for (int i = 0; i < bss_cnt && pos < json_sz - 100; i++) {
            qapi_WLAN_BSS_Scan_Info_t *bss = &evt->scan_bss_info[i];
            char ssid[33] = {0};
            int slen = bss->ssid_Length < 32 ? bss->ssid_Length : 32;
            memscpy(ssid, sizeof(ssid), bss->ssid, slen);
            pos += snprintf(line + pos, json_sz - pos,
                            "%s{\"ssid\":\"%s\","
                            "\"bssid\":\"%02x:%02x:%02x:%02x:%02x:%02x\","
                            "\"channel\":%d,"
                            "\"rssi\":-%d}",
                            i > 0 ? "," : "",
                            ssid,
                            bss->bssid[0], bss->bssid[1], bss->bssid[2],
                            bss->bssid[3], bss->bssid[4], bss->bssid[5],
                            bss->channel,
                            bss->rssi);
        }
        snprintf(line + pos, json_sz - pos, "]}");
        tcp_send_line(sock, line);
        printf(TCP_REPORT_TAG "scan sent: %d APs\r\n", bss_cnt);
        free(line);
    }
    free(evt);
}

static void tcp_session(int sock)
{
    if (!g_tcp_range_sem)
        g_tcp_range_sem = xSemaphoreCreateBinary();
    if (!g_rtt_sync_sem)
        g_rtt_sync_sem = xSemaphoreCreateBinary();

    nt_unassoc_ftm_clear_aps();
    g_tcp_ap_cnt = 0;

    if (g_pending_result_valid) {
        tcp_send_line(sock, g_pending_result);
        printf(TCP_REPORT_TAG "pending result delivered: %s\r\n", g_pending_result);
        g_pending_result_valid = 0;
    }

    tcp_do_scan(sock);

    static char rx_buf[512];

    while (!g_tcp_stop) {
        memset(rx_buf, 0, sizeof(rx_buf));
        int total = 0;
        while (total < (int)sizeof(rx_buf) - 1 && !g_tcp_stop) {
            int n = recv(sock, rx_buf + total, 1, 0);
            if (n == 0) { printf(TCP_REPORT_TAG "connection closed\r\n"); return; }
            if (n < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) continue;
                printf(TCP_REPORT_TAG "recv error %d\r\n", errno);
                return;
            }
            if (rx_buf[total] == '\n') { rx_buf[total] = 0; break; }
            total++;
        }

        printf(TCP_REPORT_TAG "recv: %s\r\n", rx_buf);

        if (strcmp(rx_buf, "rtt_scan") == 0) {
            tcp_do_scan(sock);

        } else if (strncmp(rx_buf, "rtt_ap_add ", 11) == 0) {
            char bssid_str[18] = {0};
            int ch = 0;
            float x = 0.0f, y = 0.0f;
            unsigned int v[6] = {0};
            char x_str[16] = {0}, y_str[16] = {0};

            if (sscanf(rx_buf + 11,
                       "%17s %d %15s %15s",
                       bssid_str, &ch, x_str, y_str) == 4
                && sscanf(bssid_str, "%x:%x:%x:%x:%x:%x",
                          &v[0], &v[1], &v[2], &v[3], &v[4], &v[5]) == 6) {
                x = (float)atof(x_str);
                y = (float)atof(y_str);
                uint8_t bssid[6];
                for (int i = 0; i < 6; i++) bssid[i] = (uint8_t)v[i];
                if (nt_unassoc_ftm_add_ap(bssid, (uint8_t)ch, x, y) == 0) {
                    printf(TCP_REPORT_TAG "ap_add ok: %s ch=%d\r\n", bssid_str, ch);
                    for (int j = 0; j < g_tcp_ap_cnt; j++) {
                        if (memcmp(g_tcp_aps[j].bssid, bssid, 6) == 0) {
                            g_tcp_aps[j].channel = (uint8_t)ch;
                            g_tcp_aps[j].x_m = x;
                            g_tcp_aps[j].y_m = y;
                            goto next_cmd;
                        }
                    }
                    if (g_tcp_ap_cnt < RTT_TCP_MAX_APS) {
                        memscpy(g_tcp_aps[g_tcp_ap_cnt].bssid, 6, bssid, 6);
                        g_tcp_aps[g_tcp_ap_cnt].channel = (uint8_t)ch;
                        g_tcp_aps[g_tcp_ap_cnt].ssid[0] = 0;
                        g_tcp_aps[g_tcp_ap_cnt].x_m = x;
                        g_tcp_aps[g_tcp_ap_cnt].y_m = y;
                        g_tcp_ap_cnt++;
                    }
                } else {
                    printf(TCP_REPORT_TAG "ap_add failed (table full?)\r\n");
                }
            } else {
                printf(TCP_REPORT_TAG "ap_add: bad format\r\n");
            }

        } else if (strncmp(rx_buf, "rtt_locate", 10) == 0) {
            uint8_t ftms = 8, bw = 9;
            sscanf(rx_buf + 10, " %hhu %hhu", &ftms, &bw);
            printf(TCP_REPORT_TAG "rtt_locate ftms=%d bw=%d\r\n", ftms, bw);

            if (!g_tcp_ap_sem)
                g_tcp_ap_sem = xSemaphoreCreateBinary();

            static const uint8_t HK_BSSID[6] = {0x00, 0x03, 0x7f, 0x08, 0x54, 0x61};
            static const uint8_t HK_CH = 36;
            static const char    HK_SSID[] = "rtt-demo-HK-5g";

            nt_unassoc_ftm_clear_aps();
            nt_unassoc_ftm_add_ap((uint8_t *)HK_BSSID, HK_CH, 0.0f, 0.0f);
            printf(TCP_REPORT_TAG "rtt_locate: forced HK bssid=%02x:%02x:%02x:%02x:%02x:%02x ch=%d\r\n",
                   HK_BSSID[0], HK_BSSID[1], HK_BSSID[2],
                   HK_BSSID[3], HK_BSSID[4], HK_BSSID[5], HK_CH);

#ifdef NT_DEV_STA_ID
            extern dev_common_t *gpDevCommon;
#else
            extern devh_t *gdevp;
#endif
            extern qapi_Status_t wmi_disconnect(void);
            extern qapi_Status_t wmi_connect(void);
            int was_connected = CM_DEVICE_CONNECTED(
#ifdef NT_DEV_STA_ID
                (gpDevCommon && gpDevCommon->devp[NT_DEV_STA_ID])
                ? gpDevCommon->devp[NT_DEV_STA_ID] : NULL
#else
                gdevp
#endif
            );
            if (was_connected) {
                printf(TCP_REPORT_TAG "disconnecting from hotspot before FTM\r\n");
                wmi_disconnect();
                for (int w = 0; w < 50; w++) {
                    int still_connected = CM_DEVICE_CONNECTED(
#ifdef NT_DEV_STA_ID
                        (gpDevCommon && gpDevCommon->devp[NT_DEV_STA_ID])
                        ? gpDevCommon->devp[NT_DEV_STA_ID] : NULL
#else
                        gdevp
#endif
                    );
                    if (!still_connected) break;
                    qurt_thread_sleep(10);
                }
                printf(TCP_REPORT_TAG "hotspot disconnected, starting FTM\r\n");
            }

            xSemaphoreTake(g_tcp_ap_sem, 0);
            g_tcp_ap_dist_mm = -1;
            g_tcp_ap_status  = 1;

            printf(TCP_REPORT_TAG "calling nt_unassoc_ftm_start ftms=%d bw=%d\r\n", ftms, bw);
            int rc = nt_unassoc_ftm_start(
#ifdef NT_DEV_STA_ID
                    (gpDevCommon && gpDevCommon->devp[NT_DEV_STA_ID])
                    ? gpDevCommon->devp[NT_DEV_STA_ID] : NULL,
#else
                    gdevp,
#endif
                    ftms, bw, _tcp_range_ap_done_cb, NULL);

            if (rc != 0) {
                printf(TCP_REPORT_TAG "ftm_start failed rc=%d\r\n", rc);
            } else {
                printf(TCP_REPORT_TAG "waiting for FTM done (timeout=%dms)...\r\n",
                       UNASSOC_FTM_SESSION_TIMEOUT_MS + 500);
                xSemaphoreTake(g_tcp_ap_sem,
                               pdMS_TO_TICKS(UNASSOC_FTM_SESSION_TIMEOUT_MS + 500));
                printf(TCP_REPORT_TAG "FTM done: dist=%d mm status=%d ftm_num=%d success=%d\r\n",
                       g_tcp_ap_dist_mm, g_tcp_ap_status,
                       (int)g_ftm_number, (int)g_rtt_index);
            }

            if (was_connected) {
                printf(TCP_REPORT_TAG "reconnecting to hotspot...\r\n");
                wmi_connect();
                {
                    int dhcp_ok = 0;
                    qurt_thread_sleep(2500);
                    struct netif *nif = NULL;
                    NETIF_FOREACH(nif) { break; }
                    if (nif) {
                        netif_set_addr(nif, IP4_ADDR_ANY4, IP4_ADDR_ANY4, IP4_ADDR_ANY4);
                        dhcp_start(nif);
                        printf(TCP_REPORT_TAG "DHCP restarted on netif\r\n");
                        for (int w = 0; w < 75 && !dhcp_ok; w++) {
                            qurt_thread_sleep(100);
                            if (dhcp_supplied_address(nif)) {
                                char ipbuf[16];
                                ip4addr_ntoa_r(netif_ip4_addr(nif), ipbuf, sizeof(ipbuf));
                                printf(TCP_REPORT_TAG "DHCP done, IP=%s\r\n", ipbuf);
                                dhcp_ok = 1;
                            }
                        }
                    }
                    if (!dhcp_ok)
                        printf(TCP_REPORT_TAG "DHCP timeout after 10s\r\n");
                }
            }

            char result_line[256];
            snprintf(result_line, sizeof(result_line),
                     "{\"type\":\"ftm_result\","
                     "\"status\":%d,"
                     "\"mac\":\"%02x:%02x:%02x:%02x:%02x:%02x\","
                     "\"ssid\":\"%s\","
                     "\"distance_mm\":%d,"
                     "\"distance_std_dev_mm\":%d,"
                     "\"rssi\":0,"
                     "\"num_attempted_measurements\":%d,"
                     "\"num_successful_measurements\":%d,"
                     "\"ranging_timestamp_ms\":%u}",
                     g_tcp_ap_status,
                     HK_BSSID[0], HK_BSSID[1], HK_BSSID[2],
                     HK_BSSID[3], HK_BSSID[4], HK_BSSID[5],
                     HK_SSID,
                     g_tcp_ap_dist_mm,
                     rtt_calc_std_dev_mm(),
                     (int)g_ftm_number,
                     (int)g_rtt_index,
                     (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS));
            memscpy(g_pending_result, sizeof(g_pending_result),
                    result_line, strlen(result_line) + 1);
            g_pending_result_valid = 1;
            printf(TCP_REPORT_TAG "locate done (dist=%d mm), returning for reconnect\r\n",
                   g_tcp_ap_dist_mm);

            nt_unassoc_ftm_clear_aps();
            g_tcp_ap_cnt = 0;
            return;

        } else if (strcmp(rx_buf, "rtt_ap_clear") == 0) {
            nt_unassoc_ftm_clear_aps();
            g_tcp_ap_cnt = 0;
            printf(TCP_REPORT_TAG "AP list cleared\r\n");
        }

        next_cmd:;
    }
}

static void tcp_task_fn(void *param)
{
    (void)param;

    while (!g_tcp_stop) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) { qurt_thread_sleep(2000); continue; }

        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port   = htons(g_tcp_server_port);
        inet_pton(AF_INET, g_tcp_server_ip, &addr.sin_addr);

        if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
            printf(TCP_REPORT_TAG "reconnect to %s:%d failed (errno=%d), retry in 5s\r\n",
                   g_tcp_server_ip, g_tcp_server_port, errno);
            closesocket(sock);
            for (int w = 0; w < 50 && !g_tcp_stop; w++)
                qurt_thread_sleep(100);
            continue;
        }
        printf(TCP_REPORT_TAG "connected to %s:%d\r\n",
               g_tcp_server_ip, g_tcp_server_port);
        g_tcp_sock = sock;

        struct timeval tv = {.tv_sec = 3, .tv_usec = 0};
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        tcp_session(sock);
        closesocket(sock);
        g_tcp_sock = -1;
        printf(TCP_REPORT_TAG "disconnected, reconnecting...\r\n");

        for (int w = 0; w < 50 && !g_tcp_stop; w++)
            qurt_thread_sleep(100);
    }

    g_tcp_task_handle = NULL;
    vTaskDelete(NULL);
}

static QCLI_Command_Status_t cmd_rtt_tcp_start(uint32_t Parameter_Count,
                                                QCLI_Parameter_t *Parameter_List)
{
    if (Parameter_Count < 2) {
        printf("Usage: rtt_tcp_start <server_ip> <port>\r\n");
        printf("  e.g. rtt_tcp_start 192.168.43.1 8080\r\n");
        return QCLI_STATUS_USAGE_E;
    }

    if (g_tcp_task_handle != NULL) {
        printf(TCP_REPORT_TAG "already running, use rtt_tcp_stop first\r\n");
        return QCLI_STATUS_ERROR_E;
    }

    const char *ip   = (const char *)Parameter_List[0].String_Value;
    uint16_t    port = (uint16_t)atoi((const char *)Parameter_List[1].String_Value);

    memscpy(g_tcp_server_ip, sizeof(g_tcp_server_ip), ip, strlen(ip) + 1);
    g_tcp_server_port = port;
    g_tcp_stop = 0;

    BaseType_t ret = nt_qurt_thread_create(tcp_task_fn, "rtt_tcp",
                                           WIFI_MNGR_TASK_STACK_SIZE * 4,
                                           NULL,
                                           7,
                                           &g_tcp_task_handle);
    if (ret != pdPASS) {
        printf(TCP_REPORT_TAG "failed to create task\r\n");
        return QCLI_STATUS_ERROR_E;
    }
    printf(TCP_REPORT_TAG "TCP task started, connecting to %s:%d\r\n", ip, port);
    return QCLI_STATUS_SUCCESS_E;
}

static QCLI_Command_Status_t cmd_rtt_tcp_stop(uint32_t Parameter_Count,
                                               QCLI_Parameter_t *Parameter_List)
{
    (void)Parameter_Count; (void)Parameter_List;
    g_tcp_stop = 1;
    if (g_tcp_sock >= 0) closesocket(g_tcp_sock);
    nt_unassoc_ftm_clear_aps();
    g_tcp_task_handle = NULL;
    return QCLI_STATUS_SUCCESS_E;
}
#endif

const QCLI_Command_t rtt_cmd_list[] =
{
    {cmd_RTT_Cfg,   "rtt_cfg",   "[ftms=N] [delta=N] [asap=0|1] [bw=N] [dur=N] [period=N] [cal=HEX]", "Configure RTT/FTM"},
    {cmd_RTT_Start, "rtt_start", "<mac_addr> [count]", "Start RTT/FTM measurement"},
    {cmd_RTT_Status,"rtt_status","", "Show RTT status"},
    { cmd_set_rtt_base_delay, "set_base_delay", "set_base_delay <value>", "Set RTT base delay dynamically"},
#ifdef NT_FN_RTT_DEMO
    {cmd_rtt_ap_add,    "rtt_ap_add",    "<bssid> <channel> <x_m> <y_m>",  "Add anchor AP for positioning"},
    {cmd_rtt_ap_clear,  "rtt_ap_clear",  "",                                "Clear anchor AP list"},
    {cmd_rtt_range,     "rtt_range",     "<bssid> <channel> [ftms] [bw]",   "Single-AP unassoc FTM ranging (debug)"},
    {cmd_rtt_locate,    "rtt_locate",    "[ftms_per_burst] [format_bw]",    "Start unassociated multi-AP FTM positioning"},
    {cmd_set_ap_type,   "set_ap_type",   "<hk|wkk|default> [2g|5g]",       "Set AP preset and band"},
    {cmd_set_phy_delay, "set_phy_delay", "[value_ps]",                      "Get/set board-side phy_delay (ps)"},
    {cmd_rtt_tcp_start, "rtt_tcp_start", "<server_ip> <port>",              "Connect to Android App and report FTM results via TCP"},
    {cmd_rtt_tcp_stop,  "rtt_tcp_stop",  "",                                "Disconnect TCP session"},
#endif
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
#ifdef NT_FN_RTT_DEMO
    nt_unassoc_ftm_init();
#endif
    
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
