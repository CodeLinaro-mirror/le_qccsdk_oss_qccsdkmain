/*
Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
*/
#ifndef MLME_AL_H_
#define MLME_AL_H_
#include "wifi_cmn.h"

#include "fwconfig_wlan.h"
#include "nt_flags.h"
#include "data_path.h"
#include "dxe_api.h"

typedef struct sta_config {
    uint8_t bssid[NT_MAC_ADDR_SIZE];
    uint8_t IsAP;  // 0:The peer device is a STA; 1:The peer device is an AP
    uint8_t sta_mac_address[NT_MAC_ADDR_SIZE];
    uint8_t sta_sig;
    uint8_t dpu_sig;
    uint8_t qos_sta;   // 1: Qos STA, 0: Non-Qos STA
    uint8_t sec_mode;  // NONE - 0, WEP40 - 1, WEP104 - 2, AES - 3, TKIP - 4
    uint8_t rmf;       // NOPMF-0,PMF-1
    uint8_t ht;
} sta_config_t, *p_sta_config;

typedef struct dev_config {
    uint8_t OwnMAC[NT_MAC_ADDR_SIZE];
    uint8_t dev_mode;  // 0:AP; 1:STA; 2:Concurrent

} dev_config_t, *p_dev_config;

#ifdef FEATURE_TX_COMPLETE
typedef enum {
    DP_MSG_TX_SUCCESS,
    DP_MSG_TX_RETRY_LIMIT_REACHED,
    DP_MSG_TX_TIMEOUT,
    DP_MSG_TX_ERROR,
} e_tx_compl_status_dp_msg;

typedef void (*pfn_callback)(void *params, e_tx_compl_status_dp_msg status);
#endif

typedef struct dpm_callbacks {
    void (*frwd_recvd_mgmt_pkt)(void);
} dpm_callbacks_t;

typedef enum {
    ERR_NONE,
    ERR_NO_STA_ENTRY_FOUND,
    ERR_MAX_STA_ENTRY_REACHED,
    ERR_HW_TX_FAIL,
    ERR_INVALID_PARAMS,
    ERR_EXCEED_MAX_RESOURCES,
    ERR_INVALID_BSSID,
} e_err_t;

/********Function Declarations************/

/*  Add STA configuration to DPM.
 *	Parameters:
 *	sta_config : Pointer to STA Configuration structure
 *	staid : Return value of the staid allocated. Stored and used by MLME to call DPM functions.
 *	hal_sta_idx : hal sta descriptor index
 */
e_err_t nt_dpm_add_sta(p_sta_config sta_config, uint8_t *staid, uint8_t hal_sta_idx);

/*
 *   Updating RMF bit to DPM.
 *   @param1  staid : Reference STA Id.
 *   @param2  rmf   : rmf bit to be updated.
 */
e_err_t nt_dpm_set_rmf(uint8_t staid, uint8_t rmf);

/*  Delete STA configurations.
 *	Parameters:
 *	staid : Reference STA Id
 */
e_err_t nt_dpm_delete_sta(uint8_t staid);

nt_status_t nt_dpm_ba_add(uint8_t staidx, uint32_t seq_num, uint8_t badirection, uint8_t batid, uint8_t ba_window);
nt_status_t nt_dpm_ba_del(uint8_t staidx, uint32_t seq_num, uint8_t badirection, uint8_t batid, uint8_t ba_window);

/*  TX Mgmt Packets
 *	Parameters:
 *	buff: Packet Buffer pointer.
 *	length: Length of the Packet
 */
e_err_t nt_dpm_tx_mgmt_pkt(void *buff, uint32_t length);

/*  Device configuration. Called at Start to share device configurations with DPM
 *	Parameters:
 *	dev_config: Configurations of own device
 */
#ifdef SUPPORT_RING_IF
e_err_t nt_dpm_add_dev(p_dev_config dev_config, void *buffer);
#else
e_err_t nt_dpm_add_dev(p_dev_config dev_config);
#endif
e_err_t nt_dpm_delete_dev(device_t *dev);
e_err_t nt_dpm_initialise();
e_err_t nt_dpm_deinitialise();

uint32_t append_bd_to_frame(void *frame, uint32_t length, void **ret_frame);
uint32_t get_pdu_from_rcvd_packet(void *frame, void **ret_frame);

#ifdef FEATURE_TX_COMPLETE
eRet_t nt_dpm_send_mgmt_frame(void *bufPtr, uint32_t bufLen, pfn_callback src_callback, void *params);
void nt_dpm_send_data_frame(void *bufPtr, uint32_t bufLen, pfn_callback src_callback, void *params);
#else
eRet_t nt_dpm_send_mgmt_frame(void *bufPtr, uint32_t bufLen);
#endif

#ifdef NT_TST_FN_RMF
/*
 * Used to send mgmt frame for RMF STA Attack scenario
 * Parameters:
 * Param1:bufPtr:   Packet Buffer Pointer.
 * Param2:bufLen:   Length of the Packet.
 * Param3:testvar:  testvar used for frame manipulation
 * */
void nt_dpm_send_mgmt_frame_test(void *bufPtr, uint32_t bufLen, uint8_t testvar);
#endif
/*
 * Used to rx mgmt frame
 * Param1: pointer to packet buffer pointer
 * Param2: pointer to length of the packet
 * Param3: pointer to a flag to know frame is encrypted or not
 * */
uint8_t nt_dpm_rx_mgmt_frame(void **mgmt_pdu_pck_ptr, uint32_t *mgmt_pdu_pck_len, uint32_t *enc_flag, uint8_t *rssi);
void nt_dpm_stop_handler(void (*cbptr)(uint8_t), void (*data_available_cb)(uint8_t));
void nt_dpm_stop_rx_handler(void (*cbptr)(uint8_t));
void nt_dpm_start_handler(void (*cbptr)(uint8_t));
void nt_dpm_send_eap_frame(void *bufPtr, uint32_t bufLen);
void nt_dpm_process_eap_frame(void *rx_frame);
void nt_dpm_rev_eap_frame(void *eap_frame, uint32_t eap_length);
uint8_t nt_dpm_get_rssi_data_pckt();
uint32_t nt_dpm_get_last_tx_rx_time(uint8_t staid);
#ifdef NT_PKT_THLD_NOTIFICATION
e_err_t nt_dpm_set_pkt_threshold_notification(uint32_t pkt_count, void (*callback)(void));
e_err_t nt_dpm_disable_pkt_threshold_notification();
void nt_dpm_pkt_threshold_call_back();
#endif
uint16_t nt_dpm_get_last_rx_rate_index(uint8_t staid);
void nt_dpm_update_rate_index(uint8_t staid, uint8_t rate_index);
void nt_dpm_wifi_add_ba(uint8_t staid, uint8_t tid);
void nt_dpm_wifi_del_ba(uint8_t staid, uint8_t tid);
void nt_dpm_enable_auto_ba_traffic(uint8_t ac_type);
void nt_dpm_print_rx_rate_stats(device_t *dev);
#endif /* MLME_AL_H_ */
