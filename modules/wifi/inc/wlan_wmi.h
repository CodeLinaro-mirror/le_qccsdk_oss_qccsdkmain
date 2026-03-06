/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#ifndef _WIFI_WMI_
#define _WIFI_WMI_

#include <stdbool.h>
#include "wifi_cmn.h"
#include "wmi.h"
#include "wlan_dev.h"

#define wmi_timer_disp_hnd_t TimDispatchTab
#define NT_WLAN_RA_ON        1
#define NT_WLAN_RA_HT_ONLY_ENABLE		2
#define NT_WLAN_RA_HT_ONLY_DISABLE		3
typedef struct _WLAN_CONN_INFO_ {
    uint8_t no_conn;  //@Connection status
    uint8_t ssid[wificonfigMAX_SSID_LEN + 1];
    uint8_t *mac;
    uint8_t *sta_mac_conn;
    uint8_t numsta_con;
    uint32_t pwr_save;
    uint32_t net_mode;
    uint8_t phymode;
    uint32_t inactive_period;
    uint8_t prof_commit;
    uint8_t apsd_en;
} WlanConnInfo_t;

#ifdef SUPPORT_COEX
#define DPM_TO_COEX_QUEUE_TIMEOUT 0
#define LWIP_TO_COEX_TIMEOUT      10
#endif

typedef struct {
    uint16_t rsn_cap;
} WMI_RSN_CAP_CMD;

NT_BOOL dispatch_wlan_cserv_cmds(void **pContext, uint16_t Command, void *buffer, void *msg);
#ifdef NT_FN_RMF
void wlan_wmi_set_rsn_cap(devh_t *dev, uint16_t buffer);
#endif
void send_bc_frm(devh_t *dev, int8_t frm_type);
void wlan_wmi_set_wur_config(devh_t *dev, WMI_SET_CONFIG_CMD *config_cmd);
#ifdef SUPPORT_RING_IF
void wlan_wmi_event_notification(WIFIReturnCode_t return_type, event_t id, void *data);
#endif
#if defined(SUPPORT_RING_IF) || defined(CONFIG_WMI_EVENT)
NT_BOOL nt_set_sta_channel(devh_t *dev, uint16_t freq);
#endif /* SUPPORT_RING_IF */
void _wmi_event_notification(WIFIReturnCode_t return_type, event_t id, void *data);
void wlan_wmi_set_wur_enable(devh_t *dev, uint8_t wur_enable);
void wmi_set_passphrase_cmd(devh_t *dev, uint8_t *buffer);
#ifdef NT_FN_ROAMING
void wmi_set_bg_scan_params_cmd(devh_t *dev, WMI_BACKGROUND_SCAN_CMD *cmd);
void wlan_get_roam_tbl(devh_t *dev, WMI_TARGET_ROAM_TBL *buffer);
#endif  // NT_FN_ROAMING
void wmi_set_probed_ssid_cmd(devh_t *dev, WMI_PROBED_SSID_CMD *cmd);
NT_BOOL nt_set_ap_channel(devh_t *dev, uint16_t freq);

/**
 *	@Func 	:	wlan_wmi_set_wnm_enable
 *	@Brief 	: 	This api enable/disable wnm flag
 *	@Param	:	dev - device structure
 *	@Param	:	wnm_enable - 0 for disable and 1 for enable
 *	@Return	:	none
 */
void wlan_wmi_set_wnm_enable(devh_t *dev, uint8_t wnm_enable);
/**
 *	@Func 	:	wlan_wmi_set_max_bss_idle_time
 *	@Brief 	: 	This api set bss max idle time
 *	@Param	:	dev - device structure
 *	@Param	:	bss_idle_time
 *	@Return	:	none
 */
void wlan_wmi_set_max_bss_idle_time(devh_t *dev, uint16_t bss_idle_time);

/**
 *	@Func 	:	wlan_wmi_set_sleep_time
 *	@Brief 	: 	This api set sleep interval at sta
 *	@Param	:	dev - device structure
 *	@Param	:	bss_idle_time
 *	@Return	:	none
 */
void wlan_wmi_set_sleep_time(devh_t *dev, uint16_t sleep_time);

/**
 *	@Func 	:	wmi_ap_set_dtim
 *	@Brief 	: 	This api set DTIM at ap
 *	@Param	:	dev - device structure
 *	@Param	:	dtim time
 *	@Return	:	none
 */
void wmi_sta_set_itim(devh_t *dev, uint32_t idle_time_value);
void wmi_ap_set_dtim(devh_t *dev, uint8_t dtim);
void wmi_set_pre_bcn_wkup(devh_t *dev, uint32_t pre_wakeup);
void wmi_set_bcn_wait_time(devh_t *dev, uint32_t beacon_wait_time_ms);
void wmi_set_tele_pre_bcn_inc(devh_t *dev, uint32_t tele_pre_wake_inc);
void wmi_set_tele_bcn_wait_inc(devh_t *dev, uint32_t tele_beacon_wait_inc);
void wmi_ignore_bcmc_in_bmps(devh_t *dev, uint8_t data);
void wmi_slp_clk_cal_cfg(devh_t *dev, uint8_t data);
void wmi_imps_cfg(devh_t *dev, WMI_IMPS_CFG *cfg);
void wmi_bmps_enable(devh_t *dev, uint8_t data);
void wmi_bmps_log_enable(devh_t *dev, uint8_t data);
void wmi_bmps_rx_filter_enable(devh_t *dev, uint8_t data);
void wmi_slp_cal_clk_act(devh_t *dev, uint8_t data);
void wmi_bmps_pwr_opt_enable(devh_t *dev, uint8_t data);
void wmi_bmps_compress_qos_null_enable(devh_t *dev, uint8_t data);
bool wmi_bmps_get_active_sleep_time(devh_t *dev, pm_stats_active_sleep_time_record_buffer_t *record, BMPS_STATS_TYPE type);
bool wmi_bmps_get_tx_rx_counts(devh_t *dev, pm_stats_tx_rx_counts_record_buffer_t *record);
void wmi_bmps_set_period_for_recording_stats(devh_t *dev, uint32_t period_to_record);

NT_BOOL wmi_wlan_receive_management_frame_message(devh_t *dev);

void wmi_ap_set_dtim(devh_t *dev, uint8_t dtim);

#ifdef SUPPORT_COEX
bool post_message_to_nt_wlan(uint8_t wmi_cmd_id, void *vo_data);
#endif

#if defined(SUPPORT_RING_IF) || defined(CONFIG_WMI_EVENT)
nt_status_t wmi_update_bi_cmd(devh_t *dev, void *buffer);
void wmi_update_bmtt_cmd(devh_t *dev, void *buffer);
void wifi_get_scan_results(devh_t *dev, SCAN_RESULT *scan_res);
#endif

#ifdef ENABLE_TSF_SYNC_STATS
void print_tsf_min_stats(tsf_periodic_sync_stats_t *p_tsf_sync_stats);
#endif

#ifdef CONFIG_WMI_EVENT
NT_BOOL dispatch_wlan_wmi_evts(void **pContext, void *msg);
void wlan_wmi_local_evt_notification(WMI_EVENTT_ID id, void *data, uint32_t data_len);
void wlan_wmi_cnx_event(void *conn, int32_t reason, NT_BOOL cnx_sucess);
void wlan_wmi_disc_event(void *conn, int32_t reason);
void wlan_wmi_wps_fail_event(int32_t reason);
#endif

void wlan_wmi_addba(void *msg);
void wlan_wmi_delba(void *msg);

void wmi_scan_reset_parameter(devh_t *dev, uint32_t act_dur, uint32_t pas_dur, uint32_t intv);
void wmi_scan_restore_parameter(devh_t *dev);

void send_mgmt_frame_to_app(devh_t *dev, uint32_t subtype, uint8_t *bufPtr, uint32_t bufLen);

#endif  //_WIFI_WMI_
