/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*======================================================================
 * @file wifi_evt_hndlr.h
 * @brief contains event handler defininations and structure for events
 *=======================================================================*/

#ifndef _WIFI_EVT_HNDL_
#define _WIFI_EVT_HNDL_

/*-------------------------------------------------------------------------

* Type Declarations

* ----------------------------------------------------------------------*/

typedef void (*evt_fn_table)(void *);

typedef struct {
    void *command_buffer;
    NT_BOOL in_use;
} command_struct;

typedef struct {
    uint32_t assoc_id; /* association id */
    uint8_t reason;    /* status code */
    NT_BOOL is_host_initiated;
} wifi_disconnect_event;

typedef struct {
    uint8_t net_id; /* network interface id */
    uint8_t status; /* status code */
} wifi_if_add_event;

typedef struct {
    uint8_t status;
} wifi_unit_test_event;

typedef struct {
    uint8_t bssid[IEEE80211_ADDR_LEN]; /* bssid of the ap joined */
    ssid_t ssid;                       /*ssid of joind AP */
    uint32_t assoc_id;                 /* association id */
    uint8_t status;                    /* status code for success or faliure */
    uint8_t reason_code;               /* reason code for status */
    uint8_t host_initiated;            /* whether host initiated or not */
    uint16_t channel_frequency;        /*frequency of current channel*/
} wifi_join_event;

typedef struct {
    uint8_t status;
    uint8_t scan_id;
} wifi_stop_event;

typedef struct {
    uint8_t status;
    uint32_t updated_tsf_lo;
    uint32_t updated_tsf_hi;
} tsf_periodic_sync;

typedef struct {
    uint8_t status;
} tsf_sync_start_event;
/*-------------------------------------------------------------------------

* Function Declarations and Documentation

* ----------------------------------------------------------------------*/

/*
 * @brief Initializes the wifi api specific data structure
 * @param : This function does not need any Params
 * @return This function does not return anything
 *
 */
void wifi_svc_init(void);

/*
 * @brief Sends wifi enable event to APPS.
 * @param msg: buffer containing event data to be sent to APPs
 * @return This function does not return anything
 *
 */
void wlan_enable_event(void *msg);

/*
 * @brief Sends disable event to APPS once wifi is disabled
 * @param msg: buffer containing event data to be sent to APPs
 * @return This function does not return anything
 *
 */
void wlan_disable_event(__unused void *msg);

/*
 * @brief  Sends interface add event to APPs
 * @param msg: buffer containing event data to be sent to APPs
 * @return This function does not return anything
 *
 */
void wlan_if_add_event(void *msg);

/*
 * @brief Sends the scan start event once scan starts.
 * @param msg: buffer containing event data to be sent to APPs
 * @return This function does not return anything
 *
 */
void wlan_scan_start_event(void *msg);

/*
 * @brief Sends the scan stop event once scan stop cmd is processed.
 * @param msg: buffer containing event data to be sent to APPs
 * @return This function does not return anything
 *
 */
void wlan_scan_stop_event(void *msg);

/*
 * @brief Sends the scan complete event with scan results to APPs
 * @param msg: buffer containing event data to be sent to APPs
 * @return This function does not return anything
 *
 */
void wlan_scan_comp_event(void *msg);

/*
 * @brief Sends the scan complete event on faliure to APPs
 * @param msg: buffer containing event data to be sent to APPs
 * @return This function does not return anything
 *
 */
void wlan_scan_fail_event(void *msg);

/*
 * @brief  Sends the join complete event to apps once sta joins a AP
 * @param msg: buffer containing event data to be sent to APPs
 * @return This function does not return anything
 *
 */
void wlan_join_comp_event(void *msg);

/*
 * @brief  Sends disconnect event to APPs in case of disconnection from peer
 * @param msg: buffer containing event data to be sent to APPs
 * @return This function does not return anything
 *
 */
void wlan_disconnect_event(void *msg);

/*
* @brief  Sends set param event in response to set param cmd indicating sucess
      or faliure
* @param msg: buffer containing event data to be sent to APPs
* @return This function does not return anything
*
*/
void wlan_set_param_event(void *msg);

/*
* @brief This function is a wrapper for wlan system to trigger disconnection
         events.
* @param assoc-id: Association ID of the station
* @param reason: Reason code for disconnection
* @return This function does not return anything
*
*/
void send_wifi_disc_event(uint16_t associd, WMI_DISCONNECT_REASON reason);

/*
* @brief This function is a wrapper for wlan system to trigger the connection
         events.
* @param conn: structure containing connection related info
* @param reason: Reason code for disconnection
* @param b_cnx_sucess: True is connection is sucessful, Flase if connection is failed
* @return This function does not return anything
*
*/
void send_wifi_cnx_event(void *conn, WMI_DISCONNECT_REASON reason, NT_BOOL b_cnx_sucess);

/*
 * @brief this function sends the mode event FTM or MM.
 * @param msg: not used, just to keep format of event handler
 * @return This function does not return anything
 *
 */
void wlan_mode_event(void *msg);

/*
 * @brief this function gets the wifi authmode for corrosponding wmi auth mode.
 * @param authmode: wmi authmode
 * @return Returns the wifi authmode for the given wmi authmode
 *
 */
uint8_t get_wifi_authmode_from_wmi_authmode(AUTH_MODE authmode);

/**
 *@brief This functions is called from wlan tasks to dispatch events to its respective handler
 *
 *@param msg: The wmi msg struct that contains the data related to result/event
 *        and, event type.

 *@return: This function does not returns anything
 */
void wmi_response_handler(void *msg);

/*
 * @brief this function gets the wifi disconnect/connect reason for corrosponding wmi reson codes.
 * @param authmode: wmi authmode
 * @return Returns the wifi authmode for the given wmi authmode
 *
 */
uint8_t get_wifi_reason_from_wmi_reason(WMI_DISCONNECT_REASON reason);

void wlan_unit_test_event(__unused void *msg);

/*
 * @brief this function sends the TWT setup event.
 * @param msg: It contains TWT session info
 * @return This function does not return anything
 *
 */
void wlan_twt_setup_event(__unused void *msg);

/*
 * @brief this function sends the TWT teardown event.
 * @param msg: It contains TWT teardown success/failure
 * @return This function does not return anything
 *
 */
void wlan_twt_teardown_event(__unused void *msg);

/*
 * @brief this function sends the TWT status event.
 * @param msg: It contains TWT status info
 * @return This function does not return anything
 *
 */
void wlan_twt_status_event(__unused void *msg);

/*
 * @brief Sends Address and Length of the PHYDBG Captured Buffer
 * @param msg: None
 * @return None
 *
 */
void wlan_phydbgdump_event(void *msg);

/*
 * @brief this function sends the beacon update event.
 * @param msg: It contains next tbtt and new BI
 * @return This function does not return anything
 *
 */
void wlan_update_bi_event(__unused void *msg);

/*
 * @brief this function sends the beacon miss threshold time event.
 * @param msg: It contains new beacon miss threshold time
 * @return This function does not return anything
 *
 */
void wlan_update_bmtt_event(__unused void *msg);

/*
 * @brief this function sends the set or reset wakelock event.
 * @param msg: It take the event type as a parameter
 * @return This function does not return anything
 *
 */

void wlan_set_reset_wakelock_event(void *msg);

/*
 * @brief this function sends the TSF syn start event.
 * @param msg: Data to be sent to apps
 * @return This function does not return anything
 */
void wlan_periodic_tsf_sync_start_event(__unused void *msg);

/*
 * @brief this function sends the TSF sync event.
 * @param msg: It contains TSF sync time
 * @return This function does not return anything
 */
void wlan_periodic_tsf_sync_event(__unused void *msg);

/*
 * @brief this function sends the f2a pulse on twt wakeup event.
 * @param msg: It contains the current configuration of F2A pulse on TWT wakeup
 * @return This function does not return anything
 *
 */

void wlan_f2a_on_wakeup_config_event(__unused void *msg);

#ifdef SUPPORT_COEX
/*
 * @brief this function sends the coex event.
 * @param msg: It contains coex event type
 * @return This function does not return anything
 */
void wlan_coex_event(__unused void *msg);
#endif

/*
 * @brief this function sends periodic traffic setup event.
 * @param msg: It contains wake interval, first_sp_start_tsf, traffic_type and session_id.
 * @return This function does not return anything.
 */
void wlan_periodic_traffic_setup_event(__unused void *msg);

/*
 * @brief this function sends periodic traffic status event.
 * @param msg: It contains wake interval, next_sp_start_tsf, traffic_type and session_id.
 * @return This function does not return anything.
 */
void wlan_periodic_traffic_status_event(__unused void *msg);

/*
 * @brief this function sends periodic traffic teardown event.
 * @param msg: It contains teardown_reason, traffic_type and session_id.
 * @return This function does not return anything.
 */
void wlan_periodic_traffic_teardown_event(__unused void *msg);

/*
 * @brief this function sends clk latency event.
 * @param msg: It contains status in event header.
 * @return This function does not return anything.
 */
void wlan_clk_latency_event(__unused void *msg);

#endif /* _WIFI_EVT_HNDL_ */
