/*
Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
*/
#ifndef INC_NT_SME_MLME_H_
#define INC_NT_SME_MLME_H_
#include "wifi_cmn.h"

#include "fwconfig_wlan.h"
#include "nt_flags.h"
#include "wlan_dev.h"
#include "nt_wlan.h"

#define NT_WLAN_MAX_SSID_LEN            32  // use existing one
#define NT_WLAN_MAX_PASSPHRASE_LEN      32  // use exis on
#define NT_WLAN_MAX_BSSID_LEN           6   // uaw exist
#define NT_AUTH_RETRY                   3   // wlan prefix
#define NT_ASSO_RETRY                   3   // wlan prefix
#define NT_MAX_POWER_SAVE_STATION_COUNT 16

/*Return status*/
typedef enum { NT_STATUS_OK = 0, NT_STATUS_ERROR = 1, NT_STATUS_TIMEOUT = 2, NT_STATUS_NOT_SUPPORT = 3 } NT_ReturnCode;

typedef enum {
    NT_Security_Open = 0,
    NT_Security_WEP,
    NT_Security_WPA,
    NT_Security_WPA2,
    NT_Security_Not_Supported
} NT_Security;

typedef enum { NT_Mode_Station = 0, NT_ModeAP, NT_ModeP2P, NT_Mode_Not_Supported } NT_Device_Mode;

typedef struct {
    const char *pcSSID;
    uint8_t ucSSIDLength;
    const char *pcPassword;
    uint8_t ucPasswordLength;
    NT_Security xSecurity;
    uint8_t num_channels;
    uint8_t cChannelList[11];
} NT_NetworkParams;

typedef struct {
    char cSSID[NT_WLAN_MAX_SSID_LEN + 1];
    uint8_t ucBSSID[NT_WLAN_MAX_BSSID_LEN];
    NT_Security xSecurity;
    int8_t cRSSI;
    int8_t cChannel;
    uint8_t ucHidden;
} NT_Scan_Result;

typedef struct {
    char cSSID[NT_WLAN_MAX_SSID_LEN + 1];
    uint8_t ucSSIDLength;
    uint8_t ucBSSID[NT_WLAN_MAX_BSSID_LEN];
    char cPassword[NT_WLAN_MAX_PASSPHRASE_LEN + 1];
    uint8_t ucPasswordLength;
    NT_Security xSecurity;
} NT_Network_Profile;

typedef struct nt_wep_key_cfg_s {
    uint8_t wep_type;
    uint8_t wep_key_len;
    uint8_t def_key_idx;
    uint8_t key_string[256];
} nt_wep_key_cfg_t;

typedef struct nt_wep_def_key_idx_cfg_s {
    uint8_t sta_id;
    uint8_t def_key_idx;
} nt_wep_def_key_idx_cfg_t;

/*-----------------Function Declaratin begin---------------------*/
void nt_process_wlan_receive_management_frame_message(void);
void nt_process_pre_beacon_notify();
void nt_set_dtim_value(uint8_t dtim_value);
void nt_set_idle_timer_value(uint32_t idle_time_value);

#ifdef FEATURE_TX_COMPLETE
void tx_complete_src_callback_msg(void *msg);
#endif

#ifdef NT_FN_AMPDU
NT_BOOL nt_start_addba(devh_t *dev, void *msg);
NT_BOOL nt_start_delba(devh_t *dev, void *msg);
#endif
#ifdef NT_FN_WMM
/**
 * @brief  Sets/Resets the WMM capability for device
 * @params  msg: void pointer type for wmi structure wmi_msg_struct_t
 * @retval  TRUE
 */
NT_BOOL nt_wmm_control(devh_t *dev, void *msg);
#endif  // NT_FN_WMM
#ifdef NT_FN_HT
/**
 * @brief  Sets/Resets the HT capability for device
 * @params  msg: void pointer type for wmi structure wmi_msg_struct_t
 * @retval  TRUE
 */
NT_BOOL nt_ht_cap_control(devh_t *dev, void *msg);
#endif  // NT_FN_HT
NT_BOOL nt_set_ip_precedence(void *msg);

/**
 * @brief  passes the beacon miss count value received from wmi to device structure
 * @params  msg: void pointer type for wmi structure wmi_msg_struct_t
 * @retval  TRUE
 */
NT_BOOL nt_set_beacon_miss_threshold(devh_t *dev, void *msg);
#ifdef NT_FN_WMM_PS_AP
/**
 * @brief  enables or disables UAPSD on AP side
 * @params  msg: void pointer type for wmi structure wmi_msg_struct_t
 * @retval  TRUE
 */
NT_BOOL nt_uapsd_control_ap(devh_t *dev, void *msg);
#endif  // NT_FN_WMM_PS_AP
#ifdef NT_FN_WMM_PS_STA
/**
 * @brief  configures UAPSD AC flags in qos info field of wmm ie on STA side
 * @params  msg: void pointer type for wmi structure wmi_msg_struct_t
 * @retval  TRUE
 */
NT_BOOL nt_uapsd_config_sta(devh_t *dev, void *msg);
/**
 * @brief  controls qos null trigger timer
 * @params  msg: void pointer type for wmi structure wmi_msg_struct_t
 * @retval  TRUE
 */
NT_BOOL nt_control_uapsd_trigger_timer(devh_t *dev, void *msg);

/**
 * @brief  function is used to check if the given tid supports uapsd or not.
 * @params  tid : To which uapsd status needs to be checked.
 *          ac_ptr : pointer to write back the AC information of given tid if it supports uapsd.
 * @retval  TRUE if tid supports uapsd, otherwise FALSE.
 */
NT_BOOL nt_check_tid_supports_uapsd(devh_t *dev, uint8_t tid, uint8_t *ac_ptr);
/**
 * @brief  sets pm timers timeout values as per configuration.
 * @params  msg: void pointer type for wmi structure wmi_msg_struct_t
 * @retval  TRUE.
 */
NT_BOOL nt_pm_timers_policy_set(devh_t *dev, void *msg);
/**
 * @brief  sets pspoll threshold value as per configuration.
 * @params  msg: void pointer type for wmi structure wmi_msg_struct_t
 * @retval  TRUE.
 */
NT_BOOL nt_pspoll_threshold_set(devh_t *dev, void *msg);
#endif  // NT_FN_WMM_PS_STA

#if (defined NT_FN_WMM_PS_STA) || (defined NT_FN_WMM_PS_AP)
/**
 * @brief  function returns the address of queue tid map array.
 * @params  none
 * @retval  address of queue tid map array.
 */
const uint8_t *nt_que_tid_map_array(void);
#endif

#ifdef NT_FN_DEBUG_STATS
/**
 * @brief  function prints debug stats.
 * @params  dev : address for device structure
 * @retval  TRUE
 */
NT_BOOL nt_get_wlan_debug_stats(devh_t *dev);
#endif  // NT_FN_DEBUG_STATS

#ifdef NT_FN_PRODUCTION_STATS
/**
 * @brief  function returns production stats.
 * @params  dev : address for device structure
 * @params  buf ptr: buffer pointer for returning stats
 * @retval  TRUE
 */
NT_BOOL nt_get_wlan_production_stats(devh_t *dev, void *buffer);
#endif  // NT_FN_PRODUCTION_STATS

/*AP Functions*/

/*Call back function*/
void scan_result_call_back_fun(uint8_t *res, SemaphoreHandle_t *semaphore_handle);
void nt_pre_beacon_notify();
void nt_bad_decrypt_error_notify();
void nt_mic_error_notify();
/*Dxe config functions*/
void nt_dxe_cfg_rx_mgmt_chan(void);
void nt_dxe_cfg_tx_mgmt_chan(void);
void nt_process_rtt_available_interrupt(void __attribute__((__unused__)) * dev);
void nt_rtt_t1_t4_avail_notification();
void nt_rtt_t2_capture_cb();
void nt_rtt_t2_capture_process();

/*-----------------Function Declaratin end---------------------*/

#endif /* INC_NT_SME_MLME_H_ */
