/* 
Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#ifndef WLAN_QAPI_HELPER_H
#define WLAN_QAPI_HELPER_H

#include "qapi_wlan_base.h"
#include "wmi.h"
#ifdef CONFIG_ENABLE_P2P_MODE
#include "qapi_wlan_p2p.h"
#endif

/* Should be called under protection of p_cxt->wlan_qapi_cxt_mutex */
extern void wlan_clear_privacy(void);
extern void wlan_set_connect_ssid(const unsigned char *ssid, uint8_t ssidLength);
extern void wlan_set_connect_bssid(const uint8_t *bssid, uint8_t bssid_length);
extern void wlan_set_passphrase(const uint8_t *passphrase, uint8_t passphrase_len);
extern void wlan_set_scan_param(WMI_START_SCAN_CMD *p_cmd, const qapi_WLAN_Start_Scan_Params_t *scan_Params);
extern void wlan_preset_specific_param(void);
extern qapi_Status_t wlan_set_channel(uint8_t device_id, uint16_t channel, qbool_t is_6g_index);
extern qapi_Status_t wlan_set_country_code(uint8_t device_id, uint8_t *country_code);
extern qapi_Status_t wlan_set_phy_mode(uint8_t device_id, uint32_t phy_mode);
extern int32_t wlan_set_11n_ht(uint8_t __attribute__((__unused__)) device_id, uint8_t htconfig, uint8_t is_sgi, uint8_t mpdu_density);
extern int32_t wlan_set_op_mode(uint8_t mode);
extern qapi_Status_t wlan_get_mac_address(uint8_t __attribute__((__unused__)) device_ID,
                                          uint8_t mac_addr[__QAPI_WLAN_MAC_LEN]);
extern qapi_Status_t wlan_get_power_mode(uint8_t __attribute__((__unused__)) device_ID, uint8_t *powermode);
extern qapi_Status_t wlan_get_phy_mode(uint8_t *phymode);
extern qapi_Status_t wlan_sta_get_rssi(uint8_t device_ID, uint8_t *rssi);
extern qapi_Status_t wlan_sta_get_reg_info(qapi_WLAN_Reg_Evt_t *regulatory);
extern qapi_Status_t wlan_set_ap_beacon_inteval(uint8_t device_ID, uint32_t beacon_interval);
extern qapi_Status_t wlan_set_ap_dtim_period(uint8_t device_ID, uint32_t dtim_period);
extern qapi_Status_t wlan_set_ap_inactivity(uint8_t device_ID, uint32_t inactivity_time);
extern qapi_Status_t wlan_set_ap_hidden(uint8_t device_ID, uint8_t hidden);
extern qapi_Status_t wlan_set_agg_cfg(uint8_t device_ID, uint16_t tx_tid_mask, uint16_t rx_tid_mask);
extern qapi_Status_t wlan_set_amsdu_rx(uint8_t device_ID, uint8_t enable);
extern qapi_Status_t wlan_set_sta_slptime(uint8_t device_ID, uint16_t time, uint16_t round_type);
extern qapi_Status_t wlan_get_sta_slptime(uint32_t *listen_interval);
extern qapi_Status_t wlan_clear_mgmt_frame_queue(void);
extern qapi_Status_t wlan_recv_mgmt_frame(uint8_t *buffer, uint32_t buffer_len, uint32_t *frame_len, uint32_t timeout);
extern qapi_Status_t wlan_set_appie(qapi_WLAN_App_Ie_Params_t *ie_params);
extern qapi_Status_t wlan_set_rts_cts(uint8_t device_ID, uint32_t enable);
extern qapi_Status_t wlan_get_rts_cts(uint32_t *enable);
extern qapi_Status_t wlan_set_rts_rate(uint8_t device_ID, uint32_t rate);
extern qapi_Status_t wlan_get_rts_rate(uint32_t *rate);
extern qapi_Status_t wlan_set_edca_param(uint8_t device_ID, uint8_t qid, uint8_t aifsn, uint16_t cw_min,
                                         uint16_t cw_max, uint16_t txop_limit);
extern qapi_Status_t wlan_get_edca_param(uint8_t qid, uint8_t *aifs, uint16_t *cw_min, uint16_t *cw_max,
                                         uint16_t *txop_limit);
extern qapi_Status_t wlan_set_per_upper_threshold(uint8_t device_ID, uint32_t threshold);
extern qapi_Status_t wlan_get_per_upper_threshold(uint32_t *threshold);
extern qapi_Status_t wlan_set_ba_win_size(uint8_t device_ID, uint16_t ack_timeout, uint16_t delay);
extern qapi_Status_t wlan_get_ba_win_size(uint16_t *ack_timeout, uint16_t *delay);
extern qapi_Status_t wlan_set_slot_time(uint8_t device_ID, uint32_t slot_time);
extern qapi_Status_t wlan_get_slot_time(uint32_t *slot_time);
extern qapi_Status_t wlan_set_edcca_threshold(uint8_t device_ID, uint8_t edcca_threshold);
extern qapi_Status_t wlan_get_edcca_threshold(uint8_t *edcca_threshold);
extern qapi_Status_t wlan_set_tx_power(qapi_WLAN_Set_Txpower_Params_t txpower_params);
extern qapi_Status_t wlan_get_tx_power(qapi_WLAN_Get_Power_Evt_t *txpower_params);
extern qapi_Status_t wlan_set_bmiss_threshold(uint8_t device_ID, uint8_t bmiss_threshold);
extern qapi_Status_t wlan_get_bmiss_threshold(uint8_t *bmiss_threshold);
extern qapi_Status_t wlan_set_rsp_rate(uint8_t device_id, uint8_t rate_idx);
extern qapi_Status_t wlan_set_ba_window_size(uint8_t device_ID, uint16_t tx_size, uint16_t rx_size);

#ifdef CONFIG_ENABLE_P2P_MODE
/*Structure definition for passing Atheros specific data from App*/
typedef struct WLAN_P2P_PARAM_STRUCT {
    uint16_t cmd_id;
    void *data;
    uint16_t length;
} WLAN_P2P_PARAM_STRUCT;

typedef struct WLAN_P2P_JOIN_STRUCT {
    WMI_P2P_FW_CONNECT_CMD peer_profile;
    WPS_PIN wps_pin;
} WLAN_P2P_JOIN_STRUCT;

/**
@ingroup qapi_wlan
Identifies the enable/disable options for WLAN.
*/
typedef enum {
    QAPI_WLAN_P2P_DISABLE_E = 0, /**< Disable the Wi-Fi module. */
    QAPI_WLAN_P2P_ENABLE_E = 1   /**< Enable the Wi-Fi module. */
} qapi_WLAN_P2p_Enable_e;

#endif // CONFIG_ENABLE_P2P_MODE
#endif // WLAN_QAPI_HELPER_H
