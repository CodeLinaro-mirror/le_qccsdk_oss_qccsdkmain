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
 *   App -> 730: rtt_scan | rtt_ap_clear | rtt_locate [ftms] [bw]
 *               rtt_ap_add <bssid> <ch> <x> <y> [assoc_flag] [ssid] [password]  (legacy text)
 *               {"cmd":"rtt_ap_add","bssid":"..","channel":N,"bw":N,"ssid":"...",
 *                "mode":"unassoc"|"assoc","security":"open"|"wpa2","password":"..."}  (JSON, new App)
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

#define TCP_REPORT_TAG       "[TCP] "
#define RTT_WLAN_DEV_ID      1
#define RTT_TCP_MAX_APS      8
#define DEFAULT_AP_OFFSET_PS 23365

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
    char    password[64]; /* "open" = open network; else WPA2 passphrase */
    int      assoc;        /* 0 = unassoc FTM, 1 = connected FTM */
    uint32_t ap_offset_ps; /* per-AP cal offset, 0 = use g_ap_offset */
    uint8_t  forced_bw;    /* 0 = use App-provided bw; else force this value */
    float    x_m;
    float    y_m;
} tcp_ap_entry_t;

/* AP whitelist: BSSID → calibration params. Add new APs here. */
typedef struct {
    uint8_t  bssid[6];
    uint32_t ap_offset_ps;
    uint8_t  forced_bw;   /* 0 = no override */
    int      assoc;       /* 0 = unassoc FTM, 1 = connected FTM */
    char     ssid[33];
    char     password[64];
} ap_whitelist_entry_t;

static const ap_whitelist_entry_t g_ap_whitelist[] = {
    /* HK  IPQ5018:  unassoc FTM, bw=9,  offset=20333ps  (cal 2026-06-22, 105cm ref) */
    {{0x00,0x03,0x7f,0x08,0x54,0x61}, 20333, 9, 0, "", ""},
    /* WKK QCN9224:  connected FTM, open, offset=34463ps  (cal 2026-06-22, 1m ref) */
    {{0x00,0x03,0x7f,0x01,0x57,0x04}, 34463, 0, 1, "rtt-demo-5g", "open"},
    /* HK2 IPQ8074:  unassoc FTM, bw=9,  offset=21333ps  (cal 2026-06-23, 100cm ref) */
    {{0x00,0x03,0x7f,0x07,0x90,0x13}, 21333, 9, 0, "", ""},
    /* 2290 bengal: BSSID changes on reboot; ssid used as keyword fallback */
    {{0xb6,0x4b,0xe2,0xb9,0x48,0x70}, 73767, 8, 0, "rtt-demo-2290", "open"},
};
#define AP_WHITELIST_CNT ((int)(sizeof(g_ap_whitelist)/sizeof(g_ap_whitelist[0])))

static const ap_whitelist_entry_t *ap_whitelist_lookup(const uint8_t *bssid)
{
    for (int i = 0; i < AP_WHITELIST_CNT; i++)
        if (memcmp(g_ap_whitelist[i].bssid, bssid, 6) == 0)
            return &g_ap_whitelist[i];
    return NULL;
}

static const ap_whitelist_entry_t *ap_whitelist_lookup_ssid(const char *ssid)
{
    if (!ssid || !ssid[0]) return NULL;
    for (int i = 0; i < AP_WHITELIST_CNT; i++) {
        if (g_ap_whitelist[i].ssid[0] &&
            strstr(ssid, g_ap_whitelist[i].ssid) != NULL)
            return &g_ap_whitelist[i];
    }
    return NULL;
}

/* Scan cache: keep last scan results for SSID lookup (FORCE_AP mode) */
#define RTT_SCAN_CACHE_MAX 64
static qapi_WLAN_BSS_Scan_Info_t g_scan_cache[RTT_SCAN_CACHE_MAX];
static int                       g_scan_cache_cnt = 0;

static volatile int      g_tcp_stop         = 0;
static char              g_hotspot_ssid[33] = {0};
static char              g_hotspot_pass[64] = {0};
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
extern uint32_t g_ap_offset;

static void rtt_demo_print_connected_bssid(void)
{
#ifdef NT_DEV_STA_ID
    extern dev_common_t *gpDevCommon;
    if (gpDevCommon && gpDevCommon->devp[NT_DEV_STA_ID] && gpDevCommon->devp[NT_DEV_STA_ID]->bss) {
        devh_t *dev = gpDevCommon->devp[NT_DEV_STA_ID];
        uint8_t *bssid = dev->bss->ni_bssid;
        printf("RTT: Connected BSSID=%02x:%02x:%02x:%02x:%02x:%02x\n",
               bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
        const ap_whitelist_entry_t *wl = ap_whitelist_lookup(bssid);
        if (wl) {
            g_ap_offset = wl->ap_offset_ps;
            if (wl->forced_bw) g_cfg.format_and_bw = wl->forced_bw;
            printf("rtt_start: whitelist hit offset=%u bw=%u\r\n", g_ap_offset, g_cfg.format_and_bw);
        } else {
            printf("rtt_start: no whitelist entry, using offset=%u\r\n", g_ap_offset);
        }
        return;
    }
#endif
    printf("RTT: Connected BSSID=unknown");
}

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
    uint32_t burst_cnt = g_cfg.ftms_per_burst;
    g_ftm_number = g_cfg.ftms_per_burst;
    g_rtt_index = 0;

    if (Parameter_Count > 1 && Parameter_List[1].Integer_Is_Valid) {
        burst_cnt = (uint32_t)Parameter_List[1].Integer_Value;
    }

#ifdef NT_FN_RTT_DEMO
    rtt_demo_print_connected_bssid();
#endif

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
#ifdef NT_FN_RTT_DEMO
    g_ap_offset = (uint32_t)atoi((char*)Parameter_List[0].String_Value);
    printf("RTT: ap_offset updated to %u ps\n", g_ap_offset);
#else
    g_dynamic_base_delay = (uint32_t)atoi((char*)Parameter_List[0].String_Value);
    printf("RTT: ap_offset updated to %d ps\n", g_dynamic_base_delay);
#endif
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

    /* Auto-apply whitelist: override ap_offset + bw if not explicitly provided */
    const ap_whitelist_entry_t *wl = ap_whitelist_lookup(bssid);
    if (wl) {
        if (wl->ap_offset_ps) g_ap_offset = wl->ap_offset_ps;
        if (wl->forced_bw && Parameter_Count < 4) bw = wl->forced_bw;
        printf("rtt_range: whitelist hit offset=%u bw=%u\r\n", g_ap_offset, bw);
    }

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


QCLI_Command_Status_t cmd_set_phy_delay(uint32_t Parameter_Count, QCLI_Parameter_t *Parameter_List) {
    if (Parameter_Count < 1) {
        printf("RTT: offset=%u ps (default=%u)\n", g_ap_offset, DEFAULT_AP_OFFSET_PS);
        return QCLI_STATUS_SUCCESS_E;
    }
    uint32_t val = (uint32_t)atoi((char*)Parameter_List[0].String_Value);
    g_ap_offset = (val == 0) ? DEFAULT_AP_OFFSET_PS : val;
    printf("RTT: offset updated to %u ps%s\n", g_ap_offset, (val == 0) ? " (default)" : "");
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

    int cache_cnt = bss_cnt < RTT_SCAN_CACHE_MAX ? bss_cnt : RTT_SCAN_CACHE_MAX;
    memscpy(g_scan_cache, sizeof(g_scan_cache),
            evt->scan_bss_info, cache_cnt * sizeof(qapi_WLAN_BSS_Scan_Info_t));
    g_scan_cache_cnt = cache_cnt;

    static const uint8_t rtt_ch[] = {36, 40, 44, 48, 1, 6, 11};
    int sent = 0;

    int json_sz = 64 + bss_cnt * 120;
    char *line = malloc(json_sz);
    if (!line) { free(evt); return; }

    int pos = 0;
    pos += snprintf(line + pos, json_sz - pos,
                    "{\"type\":\"scan_result\",\"aps\":[");

    for (int i = 0; i < bss_cnt && pos < json_sz - 120; i++) {
        qapi_WLAN_BSS_Scan_Info_t *bss = &evt->scan_bss_info[i];
        int ch_ok = 0;
        for (int c = 0; c < (int)(sizeof(rtt_ch)/sizeof(rtt_ch[0])); c++) {
            if (bss->channel == rtt_ch[c]) { ch_ok = 1; break; }
        }
        if (!ch_ok) continue;
        char ssid[33] = {0};
        int slen = bss->ssid_Length < 32 ? bss->ssid_Length : 32;
        memscpy(ssid, sizeof(ssid), bss->ssid, slen);
        if (strncmp(ssid, "rtt-demo", 8) != 0) continue;
        pos += snprintf(line + pos, json_sz - pos,
                        "%s{\"ssid\":\"%s\","
                        "\"bssid\":\"%02x:%02x:%02x:%02x:%02x:%02x\","
                        "\"channel\":%d,"
                        "\"rssi\":-%d}",
                        sent > 0 ? "," : "",
                        ssid,
                        bss->bssid[0], bss->bssid[1], bss->bssid[2],
                        bss->bssid[3], bss->bssid[4], bss->bssid[5],
                        bss->channel, bss->rssi);
        sent++;
    }

    for (int i = 0; i < bss_cnt && pos < json_sz - 120; i++) {
        qapi_WLAN_BSS_Scan_Info_t *bss = &evt->scan_bss_info[i];
        char ssid[33] = {0};
        int slen = bss->ssid_Length < 32 ? bss->ssid_Length : 32;
        memscpy(ssid, sizeof(ssid), bss->ssid, slen);
        int ch_ok = 0;
        for (int c = 0; c < (int)(sizeof(rtt_ch)/sizeof(rtt_ch[0])); c++) {
            if (bss->channel == rtt_ch[c]) { ch_ok = 1; break; }
        }
        if (ch_ok && strncmp(ssid, "rtt-demo", 8) == 0) continue;
        pos += snprintf(line + pos, json_sz - pos,
                        "%s{\"ssid\":\"%s\","
                        "\"bssid\":\"%02x:%02x:%02x:%02x:%02x:%02x\","
                        "\"channel\":%d,"
                        "\"rssi\":-%d}",
                        sent > 0 ? "," : "",
                        ssid,
                        bss->bssid[0], bss->bssid[1], bss->bssid[2],
                        bss->bssid[3], bss->bssid[4], bss->bssid[5],
                        bss->channel, bss->rssi);
        sent++;
    }

    snprintf(line + pos, json_sz - pos, "]}");
    tcp_send_line(sock, line);
    printf(TCP_REPORT_TAG "scan sent: %d APs total (%d bss_cnt)\r\n", sent, bss_cnt);
    free(line);
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
        char *p = g_pending_result;
        while (*p) {
            char *nl = strchr(p, '\n');
            if (nl) *nl = '\0';
            tcp_send_line(sock, p);
            printf(TCP_REPORT_TAG "pending result delivered: %s\r\n", p);
            if (!nl) break;
            *nl = '\n';
            p = nl + 1;
        }
        g_pending_result_valid = 0;
    }

    qurt_thread_sleep(500);  /* let TCP session stabilize before scan */
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

        printf(TCP_REPORT_TAG "recv: %s\r\n", rx_buf[0] ? rx_buf : "(empty/timeout)");

        if (strcmp(rx_buf, "rtt_scan") == 0) {
            tcp_do_scan(sock);

        } else if (strncmp(rx_buf, "rtt_ap_add ", 11) == 0 || rx_buf[0] == '{') {
            char     bssid_str[18] = {0};
            uint8_t  bssid[6]      = {0};
            int      ch            = 0;
            int      assoc_flag    = 0;
            char     ssid_arg[33]  = {0};
            char     pass_arg[64]  = {0};
            uint32_t wkk_offset    = 0;
            uint8_t  json_bw       = 0;
            int      parse_ok      = 0;

            if (rx_buf[0] != '{') {
                char x_str[16] = {0}, y_str[16] = {0};
                unsigned int v[6] = {0};
                int n_base = sscanf(rx_buf + 11, "%17s %d %15s %15s",
                                    bssid_str, &ch, x_str, y_str);
                if (n_base == 4) {
                    const char *p = rx_buf + 11;
                    int tok = 0;
                    while (*p && tok < 4) {
                        while (*p && *p != ' ') p++;
                        while (*p == ' ') p++;
                        tok++;
                    }
                    if (*p)
                        sscanf(p, "%d %32s %63s", &assoc_flag, ssid_arg, pass_arg);
                }
                if (n_base == 4
                    && sscanf(bssid_str, "%x:%x:%x:%x:%x:%x",
                              &v[0],&v[1],&v[2],&v[3],&v[4],&v[5]) == 6) {
                    for (int i = 0; i < 6; i++) bssid[i] = (uint8_t)v[i];
                    parse_ok = 1;
                }
            } else {
                #define JSON_STR(key, dst, dsz) do { \
                    const char *_k = "\"" key "\":\""; \
                    const char *_p = strstr(rx_buf, _k); \
                    if (_p) { \
                        _p += strlen(_k); \
                        int _i = 0; \
                        while (*_p && *_p != '"' && _i < (dsz)-1) (dst)[_i++] = *_p++; \
                        (dst)[_i] = '\0'; \
                    } \
                } while(0)

                #define JSON_INT(key, dst) do { \
                    const char *_k = "\"" key "\":"; \
                    const char *_p = strstr(rx_buf, _k); \
                    if (_p) { _p += strlen(_k); (dst) = atoi(_p); } \
                } while(0)

                char mode_str[16] = {0}, sec_str[16] = {0};
                int bw_int = 0;

                JSON_STR("bssid",    bssid_str, sizeof(bssid_str));
                JSON_INT("channel",  ch);
                JSON_INT("bw",       bw_int);
                JSON_STR("ssid",     ssid_arg,  sizeof(ssid_arg));
                JSON_STR("mode",     mode_str,  sizeof(mode_str));
                JSON_STR("security", sec_str,   sizeof(sec_str));
                JSON_STR("password", pass_arg,  sizeof(pass_arg));

                #undef JSON_STR
                #undef JSON_INT

                if (strcmp(mode_str, "assoc") == 0) assoc_flag = 1;

                if (assoc_flag && sec_str[0] && strcmp(sec_str, "wpa2") != 0)
                    strlcpy(pass_arg, "open", sizeof(pass_arg));
                if (assoc_flag && !sec_str[0] && !pass_arg[0])
                    strlcpy(pass_arg, "open", sizeof(pass_arg));

                json_bw = (bw_int > 0 && bw_int < 256) ? (uint8_t)bw_int : 0;

                unsigned int v[6] = {0};
                if (bssid_str[0]
                    && sscanf(bssid_str, "%x:%x:%x:%x:%x:%x",
                              &v[0],&v[1],&v[2],&v[3],&v[4],&v[5]) == 6) {
                    for (int i = 0; i < 6; i++) bssid[i] = (uint8_t)v[i];
                    parse_ok = 1;
                }
                printf(TCP_REPORT_TAG "ap_add(JSON): bssid=%s ch=%d bw=%d ssid=%s mode=%s sec=%s\r\n",
                       bssid_str, ch, json_bw, ssid_arg, mode_str, sec_str);
            }

            if (parse_ok) {
                const ap_whitelist_entry_t *wl = ap_whitelist_lookup(bssid);
                if (!wl && ssid_arg[0])
                    wl = ap_whitelist_lookup_ssid(ssid_arg);
                if (!wl) {
                    char cached_ssid[33] = {0};
                    for (int si = 0; si < g_scan_cache_cnt; si++) {
                        if (memcmp(g_scan_cache[si].bssid, bssid, 6) == 0) {
                            int slen = g_scan_cache[si].ssid_Length < 32
                                       ? g_scan_cache[si].ssid_Length : 32;
                            memscpy(cached_ssid, sizeof(cached_ssid), g_scan_cache[si].ssid, slen);
                            cached_ssid[slen] = '\0';
                            break;
                        }
                    }
                    if (cached_ssid[0])
                        wl = ap_whitelist_lookup_ssid(cached_ssid);
                    if (wl)
                        printf(TCP_REPORT_TAG "ap_add: ssid-keyword fallback matched \"%s\"\r\n", cached_ssid);
                }
                uint8_t final_bw = json_bw;
                if (wl) {
                    assoc_flag = wl->assoc;
                    wkk_offset = wl->ap_offset_ps;
                    if (wl->forced_bw) final_bw = wl->forced_bw;
                    if (wl->ssid[0])
                        strlcpy(ssid_arg, wl->ssid, sizeof(ssid_arg));
                    else if (!ssid_arg[0]) {
                        for (int si = 0; si < g_scan_cache_cnt; si++) {
                            if (memcmp(g_scan_cache[si].bssid, bssid, 6) == 0) {
                                int slen = g_scan_cache[si].ssid_Length < 32
                                           ? g_scan_cache[si].ssid_Length : 32;
                                memscpy(ssid_arg, sizeof(ssid_arg), g_scan_cache[si].ssid, slen);
                                ssid_arg[slen] = '\0';
                                break;
                            }
                        }
                    }
                    if (wl->password[0])
                        strlcpy(pass_arg, wl->password, sizeof(pass_arg));
                    printf(TCP_REPORT_TAG "ap_add: whitelist hit bssid=%s offset=%u assoc=%d forced_bw=%d(final=%d)\r\n",
                           bssid_str, wkk_offset, assoc_flag, wl->forced_bw, final_bw);
                }

                if (nt_unassoc_ftm_add_ap(bssid, (uint8_t)ch, 0.0f, 0.0f) == 0) {
                    printf(TCP_REPORT_TAG "ap_add ok: %s ch=%d assoc=%d bw=%d ssid=%s\r\n",
                           bssid_str, ch, assoc_flag, final_bw, ssid_arg);
                    for (int j = 0; j < g_tcp_ap_cnt; j++) {
                        if (memcmp(g_tcp_aps[j].bssid, bssid, 6) == 0) {
                            g_tcp_aps[j].channel      = (uint8_t)ch;
                            g_tcp_aps[j].assoc        = assoc_flag;
                            g_tcp_aps[j].ap_offset_ps = wkk_offset;
                            g_tcp_aps[j].forced_bw    = final_bw;
                            strlcpy(g_tcp_aps[j].ssid,     ssid_arg, sizeof(g_tcp_aps[j].ssid));
                            strlcpy(g_tcp_aps[j].password, pass_arg, sizeof(g_tcp_aps[j].password));
                            g_tcp_aps[j].x_m = 0.0f;
                            g_tcp_aps[j].y_m = 0.0f;
                            goto next_cmd;
                        }
                    }
                    if (g_tcp_ap_cnt < RTT_TCP_MAX_APS) {
                        memscpy(g_tcp_aps[g_tcp_ap_cnt].bssid, 6, bssid, 6);
                        g_tcp_aps[g_tcp_ap_cnt].channel      = (uint8_t)ch;
                        g_tcp_aps[g_tcp_ap_cnt].assoc        = assoc_flag;
                        g_tcp_aps[g_tcp_ap_cnt].ap_offset_ps = wkk_offset;
                        g_tcp_aps[g_tcp_ap_cnt].forced_bw    = final_bw;
                        strlcpy(g_tcp_aps[g_tcp_ap_cnt].ssid,     ssid_arg, sizeof(g_tcp_aps[g_tcp_ap_cnt].ssid));
                        strlcpy(g_tcp_aps[g_tcp_ap_cnt].password, pass_arg, sizeof(g_tcp_aps[g_tcp_ap_cnt].password));
                        g_tcp_aps[g_tcp_ap_cnt].x_m = 0.0f;
                        g_tcp_aps[g_tcp_ap_cnt].y_m = 0.0f;
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
            printf(TCP_REPORT_TAG "rtt_locate ftms=%d bw=%d ap_cnt=%d\r\n",
                   ftms, bw, g_tcp_ap_cnt);

            if (!g_tcp_ap_sem)
                g_tcp_ap_sem = xSemaphoreCreateBinary();

#ifdef NT_DEV_STA_ID
            extern dev_common_t *gpDevCommon;
            #define _DEV_ ((gpDevCommon && gpDevCommon->devp[NT_DEV_STA_ID]) \
                            ? gpDevCommon->devp[NT_DEV_STA_ID] : NULL)
#else
            extern devh_t *gdevp;
            #define _DEV_ gdevp
#endif
            extern qapi_Status_t wmi_disconnect(void);
            extern qapi_Status_t wmi_connect(void);

            #define LOC_MAX_RESULTS 8
            char loc_results[LOC_MAX_RESULTS][256];
            int  loc_cnt = 0;

            char saved_hotspot_ssid[33]   = {0};
            char saved_hotspot_pass[64]   = {0};
            {
                uint32_t ssid_len = sizeof(saved_hotspot_ssid) - 1;
                qapi_WLAN_Get_Param(RTT_WLAN_DEV_ID,
                                    __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                                    __QAPI_WLAN_PARAM_GROUP_WIRELESS_SSID,
                                    saved_hotspot_ssid, &ssid_len);
                uint32_t pass_len = sizeof(saved_hotspot_pass) - 1;
                qapi_WLAN_Get_Param(RTT_WLAN_DEV_ID,
                                    __QAPI_WLAN_PARAM_GROUP_WIRELESS_SECURITY,
                                    __QAPI_WLAN_PARAM_GROUP_SECURITY_PASSPHRASE,
                                    saved_hotspot_pass, &pass_len);
            }

            int was_connected = CM_DEVICE_CONNECTED(_DEV_);
            ip4_addr_t saved_ip = {0}, saved_nm = {0}, saved_gw = {0};
            if (was_connected) {
                struct netif *nif_save = NULL;
                NETIF_FOREACH(nif_save) { break; }
                if (nif_save) {
                    saved_ip = *netif_ip4_addr(nif_save);
                    saved_nm = *netif_ip4_netmask(nif_save);
                    saved_gw = *netif_ip4_gw(nif_save);
                }
                printf(TCP_REPORT_TAG "disconnecting from hotspot before FTM\r\n");
                wmi_disconnect();
                for (int w = 0; w < 50; w++) {
                    if (!CM_DEVICE_CONNECTED(_DEV_)) break;
                    qurt_thread_sleep(10);
                }
                printf(TCP_REPORT_TAG "hotspot disconnected\r\n");
            }

            for (int ai = 0; ai < g_tcp_ap_cnt && loc_cnt < LOC_MAX_RESULTS; ai++) {
                tcp_ap_entry_t *ap = &g_tcp_aps[ai];
                if (!ap->assoc) continue;

                printf(TCP_REPORT_TAG "locate[%d]: connected FTM to %s ssid=%s\r\n",
                       ai, ap->ssid[0] ? ap->ssid : "?", ap->ssid);

                if (strcmp(ap->password, "open") == 0) {
                    qapi_WLAN_Auth_Mode_e auth_none = QAPI_WLAN_AUTH_NONE_E;
                    qapi_WLAN_Crypt_Type_e crypt_none = QAPI_WLAN_CRYPT_NONE_E;
                    qapi_WLAN_Set_Param(RTT_WLAN_DEV_ID,
                                        __QAPI_WLAN_PARAM_GROUP_WIRELESS_SECURITY,
                                        __QAPI_WLAN_PARAM_GROUP_SECURITY_AUTH_MODE,
                                        &auth_none, sizeof(auth_none), FALSE);
                    qapi_WLAN_Set_Param(RTT_WLAN_DEV_ID,
                                        __QAPI_WLAN_PARAM_GROUP_WIRELESS_SECURITY,
                                        __QAPI_WLAN_PARAM_GROUP_SECURITY_ENCRYPTION_TYPE,
                                        &crypt_none, sizeof(crypt_none), FALSE);
                } else {
                    qapi_WLAN_Auth_Mode_e wpa_ver = QAPI_WLAN_AUTH_WPA2_PSK_E;
                    qapi_WLAN_Crypt_Type_e cipher  = QAPI_WLAN_CRYPT_AES_CRYPT_E;
                    qapi_WLAN_Set_Param(RTT_WLAN_DEV_ID,
                                        __QAPI_WLAN_PARAM_GROUP_WIRELESS_SECURITY,
                                        __QAPI_WLAN_PARAM_GROUP_SECURITY_AUTH_MODE,
                                        &wpa_ver, sizeof(wpa_ver), FALSE);
                    qapi_WLAN_Set_Param(RTT_WLAN_DEV_ID,
                                        __QAPI_WLAN_PARAM_GROUP_WIRELESS_SECURITY,
                                        __QAPI_WLAN_PARAM_GROUP_SECURITY_ENCRYPTION_TYPE,
                                        &cipher, sizeof(cipher), FALSE);
                    qapi_WLAN_Set_Param(RTT_WLAN_DEV_ID,
                                        __QAPI_WLAN_PARAM_GROUP_WIRELESS_SECURITY,
                                        __QAPI_WLAN_PARAM_GROUP_SECURITY_PASSPHRASE,
                                        ap->password, strlen(ap->password) + 1, FALSE);
                }
                memscpy(g_tcp_current_bssid, 6, ap->bssid, 6);
                strlcpy(g_tcp_current_ssid, ap->ssid, sizeof(g_tcp_current_ssid));
                qapi_WLAN_Set_Param(RTT_WLAN_DEV_ID,
                                    __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                                    __QAPI_WLAN_PARAM_GROUP_WIRELESS_SSID,
                                    ap->ssid, strlen(ap->ssid), FALSE);
                {
                    uint8_t zero_bssid[6] = {0};
                    qapi_WLAN_Set_Param(RTT_WLAN_DEV_ID,
                                        __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                                        __QAPI_WLAN_PARAM_GROUP_WIRELESS_BSSID,
                                        zero_bssid, 6, FALSE);
                }
                qapi_WLAN_Commit(RTT_WLAN_DEV_ID);

                int conn_ok = 0;
                for (int w = 0; w < 100; w++) {
                    if (CM_DEVICE_CONNECTED(_DEV_)) { conn_ok = 1; break; }
                    qurt_thread_sleep(100);
                }
                if (!conn_ok) {
                    printf(TCP_REPORT_TAG "locate[%d]: connect failed, skipping\r\n", ai);
                    snprintf(loc_results[loc_cnt++], 256,
                             "{\"type\":\"ftm_result\",\"status\":1,"
                             "\"mac\":\"%02x:%02x:%02x:%02x:%02x:%02x\","
                             "\"ssid\":\"%s\",\"distance_mm\":-1,"
                             "\"distance_std_dev_mm\":0,\"rssi\":0,"
                             "\"num_attempted_measurements\":0,"
                             "\"num_successful_measurements\":0,"
                             "\"ranging_timestamp_ms\":%u}",
                             ap->bssid[0], ap->bssid[1], ap->bssid[2],
                             ap->bssid[3], ap->bssid[4], ap->bssid[5],
                             ap->ssid,
                             (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS));
                    qapi_WLAN_Disconnect(RTT_WLAN_DEV_ID);
                    qurt_thread_sleep(500);
                    continue;
                }
                printf(TCP_REPORT_TAG "locate[%d]: connected, starting rtt_start\r\n", ai);

                g_rtt_index = 0;
                g_ftm_number = ftms;
                if (!g_rtt_sync_sem) g_rtt_sync_sem = xSemaphoreCreateBinary();
                xSemaphoreTake(g_rtt_sync_sem, 0);
                g_rtt_status = -1;
                usr_ftm ftm_cfg = g_cfg;
                ftm_cfg.ftm_mode = FTM_INITIATOR;
                ftm_cfg.ftms_per_burst = ftms;
                uint32_t saved_ap_offset = g_ap_offset;
                if (ap->ap_offset_ps) {
                    g_ap_offset = ap->ap_offset_ps;
                    printf(TCP_REPORT_TAG "locate[%d]: using per-AP offset=%u ps\r\n", ai, g_ap_offset);
                }
                wmi_send_cmd(WMI_SET_RTT_CFG, &ftm_cfg, sizeof(ftm_cfg));
                wmi_send_cmd(WMI_SEND_FTM_FRAME, NULL, 0);
                xSemaphoreTake(g_rtt_sync_sem, pdMS_TO_TICKS(5000));
                g_ap_offset = saved_ap_offset;

                int32_t dist_mm = (g_last_rtt_distance_mm > 0 &&
                                   g_last_rtt_distance_mm <= 0x7FFFFFFF)
                                  ? (int32_t)g_last_rtt_distance_mm : -1;
                printf(TCP_REPORT_TAG "locate[%d]: rtt_start done dist=%d mm\r\n", ai, dist_mm);

                snprintf(loc_results[loc_cnt++], 256,
                         "{\"type\":\"ftm_result\",\"status\":%d,"
                         "\"mac\":\"%02x:%02x:%02x:%02x:%02x:%02x\","
                         "\"ssid\":\"%s\",\"distance_mm\":%d,"
                         "\"distance_std_dev_mm\":%d,\"rssi\":0,"
                         "\"num_attempted_measurements\":%d,"
                         "\"num_successful_measurements\":%d,"
                         "\"ranging_timestamp_ms\":%u}",
                         dist_mm > 0 ? 0 : 1,
                         ap->bssid[0], ap->bssid[1], ap->bssid[2],
                         ap->bssid[3], ap->bssid[4], ap->bssid[5],
                         ap->ssid, dist_mm,
                         rtt_calc_std_dev_mm(),
                         (int)g_ftm_number, (int)g_rtt_index,
                         (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS));

                qapi_WLAN_Disconnect(RTT_WLAN_DEV_ID);
                for (int w = 0; w < 30; w++) {
                    if (!CM_DEVICE_CONNECTED(_DEV_)) break;
                    qurt_thread_sleep(100);
                }
            }

            int has_unassoc = 0;
            for (int ai = 0; ai < g_tcp_ap_cnt; ai++) {
                if (!g_tcp_aps[ai].assoc) { has_unassoc = 1; break; }
            }
            int unassoc_ap_cnt = 0;
            for (int ai = 0; ai < g_tcp_ap_cnt; ai++)
                if (!g_tcp_aps[ai].assoc) unassoc_ap_cnt++;

            if (has_unassoc) {
                nt_unassoc_ftm_clear_aps();
                for (int ai = 0; ai < g_tcp_ap_cnt; ai++) {
                    if (g_tcp_aps[ai].assoc) continue;
                    nt_unassoc_ftm_add_ap(g_tcp_aps[ai].bssid,
                                         g_tcp_aps[ai].channel,
                                         g_tcp_aps[ai].x_m,
                                         g_tcp_aps[ai].y_m);
                }

                uint8_t use_bw = bw;
                for (int ai = 0; ai < g_tcp_ap_cnt; ai++) {
                    if (!g_tcp_aps[ai].assoc && g_tcp_aps[ai].forced_bw) {
                        use_bw = g_tcp_aps[ai].forced_bw;
                        printf(TCP_REPORT_TAG "unassoc forced_bw=%d from AP[%d]\r\n", use_bw, ai);
                        break;
                    }
                }

                xSemaphoreTake(g_tcp_ap_sem, 0);
                g_tcp_ap_dist_mm = -1;
                g_tcp_ap_status  = 1;

                printf(TCP_REPORT_TAG "calling nt_unassoc_ftm_start ftms=%d bw=%d\r\n", ftms, use_bw);
                int rc = nt_unassoc_ftm_start(_DEV_, ftms, use_bw, _tcp_range_ap_done_cb, NULL);
                if (rc != 0) {
                    printf(TCP_REPORT_TAG "unassoc ftm_start failed rc=%d\r\n", rc);
                } else {
                    xSemaphoreTake(g_tcp_ap_sem,
                                   pdMS_TO_TICKS((UNASSOC_FTM_SESSION_TIMEOUT_MS + 500) * unassoc_ap_cnt));
                    printf(TCP_REPORT_TAG "unassoc FTM done: dist=%d mm status=%d\r\n",
                           g_tcp_ap_dist_mm, g_tcp_ap_status);
                }

                {
                    int unassoc_idx = 0;
                    for (int ai = 0; ai < g_tcp_ap_cnt; ai++) {
                        if (g_tcp_aps[ai].assoc) continue;
                        const unassoc_ftm_ap_entry_t *uap = nt_unassoc_ftm_get_ap((uint8_t)unassoc_idx);
                        unassoc_idx++;
                        if (!uap) break;
                        int32_t dist = uap->measured ? (int32_t)uap->distance_mm : -1;
                        int     st   = uap->measured ? 0 : 1;
                        if (loc_cnt < LOC_MAX_RESULTS) {
                            snprintf(loc_results[loc_cnt++], 256,
                                     "{\"type\":\"ftm_result\",\"status\":%d,"
                                     "\"mac\":\"%02x:%02x:%02x:%02x:%02x:%02x\","
                                     "\"ssid\":\"%s\",\"distance_mm\":%d,"
                                     "\"distance_std_dev_mm\":0,\"rssi\":0,"
                                     "\"num_attempted_measurements\":%d,"
                                     "\"num_successful_measurements\":%d,"
                                     "\"ranging_timestamp_ms\":%u}",
                                     st,
                                     uap->bssid[0], uap->bssid[1], uap->bssid[2],
                                     uap->bssid[3], uap->bssid[4], uap->bssid[5],
                                     g_tcp_aps[ai].ssid, dist,
                                     (int)g_ftm_number, (int)g_rtt_index,
                                     (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS));
                        }
                    }
                }
            }

            if (was_connected) {
                printf(TCP_REPORT_TAG "reconnecting to hotspot...\r\n");
                {
                    qapi_WLAN_Auth_Mode_e wpa2 = QAPI_WLAN_AUTH_WPA2_PSK_E;
                    qapi_WLAN_Crypt_Type_e aes  = QAPI_WLAN_CRYPT_AES_CRYPT_E;
                    qapi_WLAN_Set_Param(RTT_WLAN_DEV_ID,
                                        __QAPI_WLAN_PARAM_GROUP_WIRELESS_SECURITY,
                                        __QAPI_WLAN_PARAM_GROUP_SECURITY_AUTH_MODE,
                                        &wpa2, sizeof(wpa2), FALSE);
                    qapi_WLAN_Set_Param(RTT_WLAN_DEV_ID,
                                        __QAPI_WLAN_PARAM_GROUP_WIRELESS_SECURITY,
                                        __QAPI_WLAN_PARAM_GROUP_SECURITY_ENCRYPTION_TYPE,
                                        &aes, sizeof(aes), FALSE);
                    if (saved_hotspot_pass[0]) {
                        qapi_WLAN_Set_Param(RTT_WLAN_DEV_ID,
                                            __QAPI_WLAN_PARAM_GROUP_WIRELESS_SECURITY,
                                            __QAPI_WLAN_PARAM_GROUP_SECURITY_PASSPHRASE,
                                            saved_hotspot_pass,
                                            strlen(saved_hotspot_pass) + 1, FALSE);
                    } else if (g_hotspot_pass[0]) {
                        qapi_WLAN_Set_Param(RTT_WLAN_DEV_ID,
                                            __QAPI_WLAN_PARAM_GROUP_WIRELESS_SECURITY,
                                            __QAPI_WLAN_PARAM_GROUP_SECURITY_PASSPHRASE,
                                            g_hotspot_pass,
                                            strlen(g_hotspot_pass) + 1, FALSE);
                    }
                    uint8_t zero_bssid[6] = {0};
                    qapi_WLAN_Set_Param(RTT_WLAN_DEV_ID,
                                        __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                                        __QAPI_WLAN_PARAM_GROUP_WIRELESS_BSSID,
                                        zero_bssid, 6, FALSE);
                    const char *use_ssid = saved_hotspot_ssid[0] ? saved_hotspot_ssid : g_hotspot_ssid;
                    if (use_ssid[0]) {
                        qapi_WLAN_Set_Param(RTT_WLAN_DEV_ID,
                                            __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                                            __QAPI_WLAN_PARAM_GROUP_WIRELESS_SSID,
                                            use_ssid,
                                            strlen(use_ssid), FALSE);
                        printf(TCP_REPORT_TAG "reconnecting to ssid=%s\r\n", use_ssid);
                    }
                    qapi_WLAN_Commit(RTT_WLAN_DEV_ID);
                    for (int w = 0; w < 100; w++) {
                        if (CM_DEVICE_CONNECTED(_DEV_)) break;
                        qurt_thread_sleep(100);
                    }
                }
                int dhcp_ok = 0;
                qurt_thread_sleep(2500);
                struct netif *nif = NULL;
                NETIF_FOREACH(nif) { break; }
                if (nif) {
                    if (!ip4_addr_isany_val(saved_ip)) {
                        netif_set_addr(nif, &saved_ip, &saved_nm, &saved_gw);
                        char ipbuf[16];
                        ip4addr_ntoa_r(&saved_ip, ipbuf, sizeof(ipbuf));
                        printf(TCP_REPORT_TAG "DHCP done, IP=%s\r\n", ipbuf);
                        dhcp_ok = 1;
                    } else {
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
                        if (!dhcp_ok)
                            printf(TCP_REPORT_TAG "DHCP timeout after 10s\r\n");
                    }
                }
            }

            {
                char *pbuf = g_pending_result;
                int   rem  = (int)sizeof(g_pending_result);
                int   ppos = 0;
                for (int ri = 0; ri < loc_cnt && ppos < rem - 2; ri++) {
                    int n = snprintf(pbuf + ppos, rem - ppos,
                                     "%s%s", ri > 0 ? "\n" : "", loc_results[ri]);
                    if (n > 0) ppos += n;
                }
                pbuf[ppos] = '\0';
                g_pending_result_valid = (loc_cnt > 0) ? 1 : 0;
            }
            printf(TCP_REPORT_TAG "locate done: %d results pending\r\n", loc_cnt);

            if (loc_cnt > 0 && g_tcp_server_ip[0] && g_tcp_server_port) {
                {
                    struct netif *nif_push = NULL;
                    NETIF_FOREACH(nif_push) { break; }
                    if (nif_push && !ip4_addr_isany_val(*netif_ip4_gw(nif_push))) {
                        ip4addr_ntoa_r(netif_ip4_gw(nif_push), g_tcp_server_ip, sizeof(g_tcp_server_ip));
                        printf(TCP_REPORT_TAG "push: updated server IP to %s\r\n", g_tcp_server_ip);
                    }
                }
                int push_sock = socket(AF_INET, SOCK_STREAM, 0);
                if (push_sock >= 0) {
                    struct sockaddr_in push_addr;
                    memset(&push_addr, 0, sizeof(push_addr));
                    push_addr.sin_family = AF_INET;
                    push_addr.sin_port   = htons(g_tcp_server_port);
                    inet_pton(AF_INET, g_tcp_server_ip, &push_addr.sin_addr);
                    struct timeval push_tv = {.tv_sec = 5, .tv_usec = 0};
                    setsockopt(push_sock, SOL_SOCKET, SO_RCVTIMEO, &push_tv, sizeof(push_tv));
                    if (connect(push_sock, (struct sockaddr *)&push_addr, sizeof(push_addr)) == 0) {
                        printf(TCP_REPORT_TAG "push socket connected, delivering %d results\r\n", loc_cnt);
                        for (int ri = 0; ri < loc_cnt; ri++) {
                            tcp_send_line(push_sock, loc_results[ri]);
                            printf(TCP_REPORT_TAG "pending result delivered: %s\r\n", loc_results[ri]);
                        }
                        g_pending_result_valid = 0;
                    } else {
                        printf(TCP_REPORT_TAG "push connect failed (errno=%d), result kept pending\r\n", errno);
                    }
                    closesocket(push_sock);
                }
            }

            nt_unassoc_ftm_clear_aps();
            g_tcp_ap_cnt = 0;
            #undef _DEV_
            #undef LOC_MAX_RESULTS
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
            printf(TCP_REPORT_TAG "reconnect to %s:%d failed (errno=%d)\r\n",
                   g_tcp_server_ip, g_tcp_server_port, errno);
            closesocket(sock);

#ifdef NT_DEV_STA_ID
            extern dev_common_t *gpDevCommon;
            devh_t *_dev = (gpDevCommon && gpDevCommon->devp[NT_DEV_STA_ID])
                           ? gpDevCommon->devp[NT_DEV_STA_ID] : NULL;
#else
            extern devh_t *gdevp;
            devh_t *_dev = gdevp;
#endif
            if (_dev && !CM_DEVICE_CONNECTED(_dev)) {
                printf(TCP_REPORT_TAG "WiFi down, reconnecting hotspot...\r\n");
                if (g_hotspot_ssid[0]) {
                    qapi_WLAN_Auth_Mode_e wpa2 = QAPI_WLAN_AUTH_WPA2_PSK_E;
                    qapi_WLAN_Crypt_Type_e aes  = QAPI_WLAN_CRYPT_AES_CRYPT_E;
                    qapi_WLAN_Set_Param(RTT_WLAN_DEV_ID,
                                        __QAPI_WLAN_PARAM_GROUP_WIRELESS_SECURITY,
                                        __QAPI_WLAN_PARAM_GROUP_SECURITY_AUTH_MODE,
                                        &wpa2, sizeof(wpa2), FALSE);
                    qapi_WLAN_Set_Param(RTT_WLAN_DEV_ID,
                                        __QAPI_WLAN_PARAM_GROUP_WIRELESS_SECURITY,
                                        __QAPI_WLAN_PARAM_GROUP_SECURITY_ENCRYPTION_TYPE,
                                        &aes, sizeof(aes), FALSE);
                    if (g_hotspot_pass[0])
                        qapi_WLAN_Set_Param(RTT_WLAN_DEV_ID,
                                            __QAPI_WLAN_PARAM_GROUP_WIRELESS_SECURITY,
                                            __QAPI_WLAN_PARAM_GROUP_SECURITY_PASSPHRASE,
                                            g_hotspot_pass, strlen(g_hotspot_pass) + 1, FALSE);
                    uint8_t zero_bssid[6] = {0};
                    qapi_WLAN_Set_Param(RTT_WLAN_DEV_ID,
                                        __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                                        __QAPI_WLAN_PARAM_GROUP_WIRELESS_BSSID,
                                        zero_bssid, 6, FALSE);
                    qapi_WLAN_Set_Param(RTT_WLAN_DEV_ID,
                                        __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                                        __QAPI_WLAN_PARAM_GROUP_WIRELESS_SSID,
                                        g_hotspot_ssid, strlen(g_hotspot_ssid), FALSE);
                    qapi_WLAN_Commit(RTT_WLAN_DEV_ID);
                } else {
                    extern qapi_Status_t wmi_connect(void);
                    wmi_connect();
                }
                for (int w = 0; w < 100 && !g_tcp_stop; w++) {
                    if (CM_DEVICE_CONNECTED(_dev)) break;
                    qurt_thread_sleep(100);
                }
                if (CM_DEVICE_CONNECTED(_dev)) {
                    /* Re-acquire IP via DHCP and update server IP to new GW */
                    qurt_thread_sleep(2500);
                    struct netif *nif = NULL;
                    NETIF_FOREACH(nif) { break; }
                    if (nif) {
                        netif_set_addr(nif, IP4_ADDR_ANY4, IP4_ADDR_ANY4, IP4_ADDR_ANY4);
                        dhcp_start(nif);
                        for (int w = 0; w < 75 && !g_tcp_stop; w++) {
                            qurt_thread_sleep(100);
                            if (dhcp_supplied_address(nif)) {
                                ip4_addr_t gw = *netif_ip4_gw(nif);
                                ip4addr_ntoa_r(&gw, g_tcp_server_ip, sizeof(g_tcp_server_ip));
                                printf(TCP_REPORT_TAG "WiFi reconnected, new GW=%s\r\n",
                                       g_tcp_server_ip);
                                break;
                            }
                        }
                    }
                } else {
                    printf(TCP_REPORT_TAG "WiFi reconnect failed, will retry\r\n");
                }
            }

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
        printf(TCP_REPORT_TAG "session ended, will reconnect\r\n");

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
        printf("Usage: rtt_tcp_start <server_ip> <port> [ssid] [password]\r\n");
        printf("  e.g. rtt_tcp_start 192.168.43.1 8080 rttphone <password>\r\n");
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

    /* Optional hotspot SSID + password for reconnect after connected FTM */
    if (Parameter_Count >= 4) {
        strlcpy(g_hotspot_ssid, (const char *)Parameter_List[2].String_Value, sizeof(g_hotspot_ssid));
        strlcpy(g_hotspot_pass, (const char *)Parameter_List[3].String_Value, sizeof(g_hotspot_pass));
    } else if (Parameter_Count >= 3) {
        strlcpy(g_hotspot_ssid, (const char *)Parameter_List[2].String_Value, sizeof(g_hotspot_ssid));
        g_hotspot_pass[0] = '\0';
    } else {
        /* Auto-detect: read current connected SSID */
        uint32_t ssid_len = sizeof(g_hotspot_ssid) - 1;
        g_hotspot_ssid[0] = '\0';
        qapi_WLAN_Get_Param(RTT_WLAN_DEV_ID,
                            __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                            __QAPI_WLAN_PARAM_GROUP_WIRELESS_SSID,
                            g_hotspot_ssid, &ssid_len);
        /* Also save password */
        uint32_t pass_len = sizeof(g_hotspot_pass) - 1;
        g_hotspot_pass[0] = '\0';
        qapi_WLAN_Get_Param(RTT_WLAN_DEV_ID,
                            __QAPI_WLAN_PARAM_GROUP_WIRELESS_SECURITY,
                            __QAPI_WLAN_PARAM_GROUP_SECURITY_PASSPHRASE,
                            g_hotspot_pass, &pass_len);
        printf(TCP_REPORT_TAG "auto hotspot ssid=%s\r\n", g_hotspot_ssid);
    }

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
    {cmd_set_phy_delay, "set_phy_delay", "[value_ps]",                      "Get/set total offset (ps); 0=use default 23365ps"},
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
