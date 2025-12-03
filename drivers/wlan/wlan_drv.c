/* 
Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include "wlan_drv.h"
#include "wlan_qapi_helper.h"
#include "wmi_api.h"

#define WLAN_ROAMING_TIMER_PERIOD_DEFAULT 5000
#define WLAN_ROAMING_TIMER_PERIOD_NICREASE1 5000
#define WLAN_ROAMING_TIMER_PERIOD_NICREASE2 15000
#define WLAN_ROAMING_TIMER_PERIOD_NICREASE3 30000
#define WLAN_ROAMING_TIMER_PERIOD_MAX 600000
#define WLAN_ROAMING_CNT_FOR_NXT_TIMER_PERIOD 3

typedef void (*wmi_evt_cb_t)(uint32_t if_id, event_t evt_id, void *data, uint32_t data_length, void *cxt);
extern void wmi_register_event_handler(wmi_evt_cb_t cb, void *cxt);

wlan_qapi_cxt_t gs_wlan_qapi_cxt;
wlan_qapi_cxt_t *gp_wlan_qapi_cxt;

qapi_Status_t wlan_drv_set_cb(qapi_WLAN_Callback_t callback, void *application_Context)
{
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
    qapi_Status_t ret = QAPI_WLAN_ERROR;

    PRINT_LOG_FUNC_LINE;
    if (!callback) {
        warn_printf("clear call back\n");
    }
    qurt_mutex_lock(&p_cxt->wlan_qapi_cxt_mutex);
    if (callback && p_cxt->qapi_event_handler) {
        PRINT_ERR_ALREADY_EXIST;
        ret = QAPI_WLAN_ERR_EEXIST;
    } else {
        p_cxt->qapi_event_handler = callback;
        p_cxt->event_application_Context = application_Context;
        ret = QAPI_OK;
    }
    qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);
    return ret;
}

void wlan_drv_roaming_timer_handler(TimerHandle_t thandle)
{
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;
    static uint8_t cnt_for_current_time_period = 1;

    qurt_mutex_lock(&p_cxt->wlan_qapi_cxt_mutex);

    if (cnt_for_current_time_period >= WLAN_ROAMING_CNT_FOR_NXT_TIMER_PERIOD) {
        cnt_for_current_time_period = 0;

        if (p_cxt->roaming_time_out < WLAN_ROAMING_TIMER_PERIOD_NICREASE2) {
            p_cxt->roaming_time_out += WLAN_ROAMING_TIMER_PERIOD_NICREASE1;
        } else if (p_cxt->roaming_time_out < WLAN_ROAMING_TIMER_PERIOD_NICREASE3) {
            p_cxt->roaming_time_out += WLAN_ROAMING_TIMER_PERIOD_NICREASE2;
        } else {
            p_cxt->roaming_time_out += WLAN_ROAMING_TIMER_PERIOD_NICREASE3;
        }

        nt_timer_change_time_period(thandle, NT_MS_TO_TICKS(p_cxt->roaming_time_out));
    }

    cnt_for_current_time_period += 1;

    if (p_cxt->roaming_time_out <= WLAN_ROAMING_TIMER_PERIOD_MAX) {
        uint8_t authMode = p_cxt->connect_cmd.authMode;
        if ((authMode == WMI_WPA_PSK_AUTH) || (authMode == WMI_WPA2_PSK_AUTH) || (authMode == WMI_WPA3_SHA256_AUTH) ||
            (authMode == (WMI_WPA2_PSK_AUTH | WMI_WPA3_SHA256_AUTH))) {
            wmi_set_passphrase();
        }
        wmi_connect();

        nt_start_timer(p_cxt->roaming_timer);
    } else {
        p_cxt->wlan_roaming_started = 0;
        nt_stop_timer(p_cxt->roaming_timer);
    }

    qurt_mutex_unlock(&p_cxt->wlan_qapi_cxt_mutex);

    return;
}

qapi_Status_t wlan_drv_roaming_start(void)
{
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;

    if ((p_cxt) && (p_cxt->roaming_timer) && (p_cxt->wlan_roaming_started == 0) &&
        (p_cxt->connect_cmd.ssidLength != 0)) {
        p_cxt->wlan_roaming_started = 1;
        p_cxt->roaming_time_out = WLAN_ROAMING_TIMER_PERIOD_DEFAULT;
        nt_timer_change_time_period(p_cxt->roaming_timer, NT_MS_TO_TICKS(p_cxt->roaming_time_out));

        nt_start_timer(p_cxt->roaming_timer);
    }

    return QAPI_OK;
}

qapi_Status_t wlan_drv_roaming_stop(void)
{
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;

    if ((p_cxt) && (p_cxt->roaming_timer) && (p_cxt->wlan_roaming_started)) {
        p_cxt->wlan_roaming_started = 0;
        nt_stop_timer(p_cxt->roaming_timer);
    }

    return QAPI_OK;
}

int wlan_qapi_init(void)
{
    gp_wlan_qapi_cxt = &gs_wlan_qapi_cxt;
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;

    PRINT_LOG_FUNC_LINE;
    memset(p_cxt, 0, sizeof(wlan_qapi_cxt_t));
    if (qurt_mutex_create(&p_cxt->wlan_qapi_cxt_mutex) != QURT_EOK) {
        PRINT_ERR_NO_RESOURCE;
        set_wlan_qapi_error(QAPI_WLAN_ERR_NO_RESOURCE);
        return (int)QAPI_WLAN_ERR_NO_RESOURCE;
    }
    qurt_signal_create(&p_cxt->wlan_cmd_done);
    qurt_mutex_create(&p_cxt->wlan_qapi_block_mutex);
    p_cxt->network_id = __QAPI_NETWORK_ID_UNSPECIFIED;
    p_cxt->wlan_enable_block_mode = true;
    p_cxt->wlan_disable_block_mode = true;
    p_cxt->wlan_if_add_block_mode = true;
    p_cxt->wlan_scan_start_block_mode = true;
#ifdef CONFIG_WPS
    p_cxt->wlan_scan_stop_block_mode = true;
    p_cxt->wlan_start_wps_block_mode = false;
#endif
#ifdef CONFIG_ENABLE_P2P_MODE
    p_cxt->wlan_p2p_block_mode = true;
    p_cxt->get_p2p_nodelist.buffer_Length = __QAPI_WLAN_P2P_EVT_BUF_SIZE;
    p_cxt->get_p2p_nodelist.node_List_Buffer = malloc(__QAPI_WLAN_P2P_EVT_BUF_SIZE);
#endif
    p_cxt->wlan_connect_block_mode = false;
    p_cxt->wlan_disconnect_block_mode = true;
    p_cxt->wlan_get_stat_block_mode = true;
    p_cxt->wlan_set_param_block_mode = true;
    p_cxt->wlan_get_regulatory_block_mode = true;
    p_cxt->wlan_set_rate_block_mode = true;
    p_cxt->wlan_send_raw_block_mode = true;
    p_cxt->wlan_set_mgmt_filter_block_mode = true;
    p_cxt->wlan_get_tx_power_block_mode = true;
    p_cxt->wlan_get_bmps_stats_block_mode = true;

    wmi_register_event_handler(wmi_event_relay, (void *)p_cxt);
    p_cxt->event_payload_buf[EVT_LARGE_PAYLOAD].buf_length = QAPI_EVENT_LARGE_PAYLOAD_LENGTH_MAX;
    p_cxt->event_payload_buf[EVT_LARGE_PAYLOAD].buf_num = QAPI_EVENT_LARGE_PAYLOAD_BUF_NUM;
    p_cxt->event_payload_buf[EVT_LARGE_PAYLOAD].buf = (uint8_t *)malloc(
        p_cxt->event_payload_buf[EVT_LARGE_PAYLOAD].buf_length * p_cxt->event_payload_buf[EVT_LARGE_PAYLOAD].buf_num);
    p_cxt->event_payload_buf[EVT_SMALL_PAYLOAD].buf_length = QAPI_EVENT_SMALL_PAYLOAD_LENGTH_MAX;
    p_cxt->event_payload_buf[EVT_SMALL_PAYLOAD].buf_num = QAPI_EVENT_SMALL_PAYLOAD_BUF_NUM;
    p_cxt->event_payload_buf[EVT_SMALL_PAYLOAD].buf = (uint8_t *)malloc(
        p_cxt->event_payload_buf[EVT_SMALL_PAYLOAD].buf_length * p_cxt->event_payload_buf[EVT_SMALL_PAYLOAD].buf_num);
    p_cxt->scanBssMaxCount = __QAPI_MAX_SCAN_RESULT_ENTRY;
    p_cxt->pScanOutSize =
        sizeof(qapi_WLAN_Scan_Comp_Evt_t) + sizeof(qapi_WLAN_BSS_Scan_Info_t) * p_cxt->scanBssMaxCount;
    p_cxt->pScanOut = malloc(p_cxt->pScanOutSize);
    p_cxt->cmd_bmps_stats.bwindow_wait_close_time = (pm_stats_active_sleep_time_record_buffer_t *)malloc(sizeof(pm_stats_active_sleep_time_record_buffer_t));
    p_cxt->cmd_bmps_stats.soc_active_sleep_time = (pm_stats_active_sleep_time_record_buffer_t *)malloc(sizeof(pm_stats_active_sleep_time_record_buffer_t));
    p_cxt->cmd_bmps_stats.tx_rx_counts = (pm_stats_tx_rx_counts_record_buffer_t *)malloc(sizeof(pm_stats_tx_rx_counts_record_buffer_t));

    p_cxt->opmode = WHAL_M_AP;
    p_cxt->conc_mode = WHAL_M_NO_CONC;
    wlan_clear_privacy();
    wlan_preset_specific_param();

    p_cxt->wlan_roaming_started = 0;
    p_cxt->roaming_time_out = WLAN_ROAMING_TIMER_PERIOD_DEFAULT;
    p_cxt->roaming_timer =
        nt_create_timer(wlan_drv_roaming_timer_handler, NULL, NT_MS_TO_TICKS(p_cxt->roaming_time_out), FALSE);
    memscpy(p_cxt->country_code, 3, DEF_AP_COUNTRY_CODE, 3);
    p_cxt->mgmt_filter.recv_queue = nt_qurt_pipe_create(100, sizeof(WMI_MGMT_FRAME_RECV_MSG));
    return (int)QAPI_OK;
}

void wlan_qapi_exit(void)
{
    wlan_qapi_cxt_t *p_cxt = gp_wlan_qapi_cxt;

    PRINT_LOG_FUNC_LINE;
    qurt_mutex_delete(&p_cxt->wlan_qapi_cxt_mutex);
    qurt_signal_delete(&p_cxt->wlan_cmd_done);
    qurt_mutex_delete(&p_cxt->wlan_qapi_block_mutex);
    free(p_cxt->event_payload_buf[EVT_LARGE_PAYLOAD].buf);
    free(p_cxt->event_payload_buf[EVT_SMALL_PAYLOAD].buf);
    free(p_cxt->pScanOut);
    p_cxt->event_payload_buf[EVT_LARGE_PAYLOAD].buf = NULL;
    p_cxt->event_payload_buf[EVT_SMALL_PAYLOAD].buf = NULL;
    p_cxt->pScanOut = NULL;
}
