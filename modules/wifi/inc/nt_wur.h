/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * nt_wur.h
 *
 *  Created on: May 28, 2020
 *      Author: c_suresi
 */

#ifndef CORE_WIFI_SME_INC_NT_WUR_H_
#define CORE_WIFI_SME_INC_NT_WUR_H_

#include <stdio.h>

// CORE_WIFI_SME_INC_NT_WUR_H_
#include "nt_common.h"
#include "wlan_dev.h"
#include "wlan_framegen.h"
#include "mlme_api.h"
#include "hal_int_template.h"

#if (defined NT_FN_WUR_AP) || (defined NT_FN_WUR_STA)

#define IEEE80211BA_WUR_CATAGORY  32
#define WUR_DUTY_CYCLE_START_TIME 0
#define WUR_GROUP_ID_LIST         0
#define WUR_INT_ENABLE            21

/*!	\def WUR_CRC16_POLYNOMIAL
 * 	\brief	Polynomial used for crc calculation is  X^16 + X^12 + X^5 + 1
 *	Polynomial value (in binary) X^16 + X^12 + X^5 + 1 : 10001000000100001
 *	Polynomial value (in hex) is 0x11021 but for 16 bit crc, x^16 bit is not used
 *	so the value for after rejecting 16th bit is 0x1021.
 *	Now for the CRC16_x25 algorithm, reverse pattern is used so polynomial
 *	value will be 0x8408 i.e reverse bit pattern of 0x1021
 */
#define WUR_CRC16_POLYNOMIAL 0x8408

/*!	\def WUR_CRC32_POLYNOMIAL
 * 	\brief	Polynomial used for crc calculation is
 * 	x32 + x26 + x23 + x22 + x16 + x12 + x11 + x10 + x8 + x7 + x5 + x4 + x2 + x + 1
 *	Polynomial value (in binary) : 1 0000 0100 1100 0001 0001 1101 1011 0111
 *	Polynomial value (in hex) is 0x104c11db7
 *	For CRC32 algorithm, reverse pattern is used so polynomial
 *	value is set as 0xEDB88320
 */
#define WUR_CRC32_POLYNOMIAL 0xEDB88320

/*!	@def WUR_CTS2S_DURATION
 *  @brief	it is used for duration in us. for cts to self frame 32 ms time is required.
 * */
#define WUR_CTS2S_DURATION 32000

/*!	@def WUR_CFE_DURATION
 *  @brief	it is used for duration in us. 32 ms time is set for duration.
 * */
#define WUR_CFE_DURATION 32000

/*!	\def WUR_MAX_FRM_QUEUE
 * 	\brief Max count for wur frame which can be saved in the queue list
 */
#define WUR_MAX_FRM_QUEUE 4

/*!	\def WUR_MAX_STA_COUNT
 * 	\brief Max count for support wur sta associate with AP
 */
#define WUR_MAX_STA_COUNT 4

/*Dialog token value begin*/
#define WUR_MODE_REQUEST_DIALOG_TOKEN          1
#define WUR_MODE_SUSPEND_REQUEST_DIALOG_TOKEN  2
#define WUR_MODE_RESPONSE_DIALOG_TOKEN         WUR_MODE_REQUEST_DIALOG_TOKENs
#define WUR_MODE_SUSPEND_RESPONSE_DIALOG_TOKEN WUR_MODE_SUSPEND_REQUEST_DIALOG_TOKEN
/*Dialog token value end*/

/*wur default config begin*/
#define DEFAULT_WUR_ENABLE_DISCOVERY                          0
#define DEFAULT_WUR_SERVICE_PERIOD                            2   /*default service period time in mili sec */
#define DEFAULT_WUR_SERVICE_INTERVAL                          200 /*default service interval time in mili sec */
#define DEFAULT_WUR_BEACON_PERIOD                             200 /*default beacon period time in mili sec */
#define DEFAULT_WUR_BEACON_TIME_OFFSET                        60  /*default beacon time offset*/
#define DEFAULT_PRE_BEACON_INTERVAL                           50  /*default pre beacon interval in mili sec */
#define DEFAULT_MIN_SLEEP_TIME                                500 /*default minimum sleep timer*/
#define DEFAULT_WUR_FRAME_SIZE                                6
#define DEFAULT_SEND_WAKEUP_FRAME_AT_DATA_AVAILABILITY        1
#define DEFAULT_SEND_SUSSPEND_FRAME_AT_DATA_AVAILABILITY      1
#define DEFAULT_SEND_SUSSPEND_FRAME_IF_WAKE_UP_FRAME_RECEIVED 1;
/*wur default config end*/

/*enum declaration begin*/
typedef enum {
    IEEE80211_WUR_CAPABILITIES_ELEM_EXTEN_ID = 48,
    IEEE80211_WUR_OPERATION_ELEM_EXTEN_ID = 49,
    IEEE80211_WUR_MODE_ELEM_EXTEN_ID = 50,
    IEEE80211_WUR_DISCOVERY_ELEM_EXTEN_ID = 51,
    IEEE80211_WUR_PROTECTION_ELEM_EXTEN_ID = 52
} wur_element_id;

typedef enum {
    WUR_RESPONSE_STATUS_ACCEPT = 0,
    WUR_RESPONSE_STATUS_DENIED = 1,
    WUR_RESPONSE_STATUS_DENIED_DUTY_CYCLE_TIME = 2,
    WUR_RESPONSE_STATUS_DENIED_DUTY_CYCLE_SERVICE_TIME = 3
} wur_mode_response_status;

typedef enum {
    WUR_ACTION_ENTER_WUR_MODE_REQUEST = 0,
    WUR_ACTION_ENTER_WUR_MODE_RESPONSE = 1,
    WUR_ACTION_ENTER_WUR_MODE_SUSPEND_REQUEST = 2,
    WUR_ACTION_ENTER_WUR_MODE_SUSPEND_RESPONSE = 3,
    WUR_ACTION_ENTER_WUR_MODE_SUSPEND = 4,
    WUR_ACTION_ENTER_WUR_MODE = 5
} wur_mode_ie_action_type;

typedef enum { WUR_MODE_SETUP = 0, WUR_MODE_TEARDOWN = 1, WUR_WAKEUP_INDICATION = 2 } wur_mode_actions;

/**
 * wur_frame_type : This enum defines the control type of wur frame and
 * 					it is used to create wur frame.
 */
typedef enum {
    WUR_BEACON = 0,           //<type = 0  for wur beacon frame
    WUR_WAKEUP = 1,           //<type = 1  for wur wake frame
    WUR_VENDOR_SPECIFIC = 2,  //<type = 2  for wur vendor frame
    WUR_DISCOVERY = 3,        //<type = 3  for wur discovery frame
    WUR_SHORT_WAKE_UP = 4     //<type = 4  for wur short wakeup frame
} wur_frame_type;

typedef enum { WAKE_UP_FRAME_RECEIVED, VENDOR_FRAME_RECEIVED } HAL_NOTIFICATION_TYPE_E;
/*enum declaration end*/

/*Typedef Declaration begin*/

typedef struct {
    uint8_t enable_discovery;
    uint8_t wur_service_period;
    uint32_t wur_service_interval;
    uint8_t wur_beacon_offset;
    uint8_t CTS2Senable;
    uint8_t CFEenable;
    uint8_t wurMcEnable;
    volatile int8_t wurEnable;
    uint8_t dot11WUROptionImplemented;
    uint8_t dot11MultiBSSIDImplemented;
    uint32_t dot11WURBeaconPeriod;
    uint8_t dot11WURFDMAChannelSwitchImplemented;
    uint8_t dot11WURDiscoveryImplemented;
    uint8_t dot11WURNeighborDiscoveryImplemented;
    uint8_t dot11RSNAWURFrameProtectionActivated;
    uint8_t dot11RSNAStatsCMACWURReplays;
    uint8_t dot11CurrentChannelWidth;
    uint8_t is_requested_ap_have_wur_capabilities;
    uint8_t add_mode_setup_element_in_association_frames;
    uint8_t is_requesting_sta_have_wur_capabilities;  // CHECK
#if (defined NT_FN_WUR_AP) || (defined NT_FN_WUR_STA)
    uint8_t allow_wake_up_when_sta_data_availablity;
    uint8_t allow_wake_up_if_received_wake_up_frame;
    uint16_t wur_vendor_id1;
    uint16_t wur_vendor_id2;
#endif
#ifdef NT_FN_WUR_AP
    uint8_t allow_wake_up_when_ap_data_availablity;
#endif
} WUR_CONFIG_T;

typedef struct {
    uint8_t element_id : 8;
    uint8_t length : 8;
    uint8_t element_id_extension : 8;
    uint8_t supported_bands : 8;
    /*WUR Capability informations begin*/
    uint8_t transition_delay : 8;
    uint8_t vl_wur_frame_support : 1;
    uint8_t wur_group_id_support : 2;
    uint8_t protected_wur_frame_support : 1;
    uint8_t wur_basic_ppdu_with_hdr_support_20_mhz : 1;
    uint8_t wur_fdma_support : 1;
    uint8_t wur_short_wake_up_frame_support : 1;
    uint8_t reserved : 1;
    /*WUR Capability informations end*/
} wur_capabilities_element_t;

typedef struct {
    uint8_t element_id : 8;
    uint8_t length : 8;
    uint8_t element_id_extension : 8;

    /*WUR Operation Parameteres begin*/
    uint8_t minimum_wake_up_duration : 8;
    uint16_t duty_cycle_period_units : 16;
    uint8_t wur_operating_class;
    uint8_t wur_channel;
    uint16_t wur_beacon_period;
    uint16_t offset_of_twbtt;
    uint8_t counter : 4;
    uint8_t common_pn : 1;
    uint8_t reserved : 3;
    /*WUR Operation Parameteres end*/

} wur_operation_element_t;

typedef struct {
    uint8_t element_id : 8;
    uint8_t length : 8;
    uint8_t element_id_extension : 8;

    uint8_t action_type : 8;
    uint8_t wur_mode_response_status : 8;

    /*WUR parameter control field begin*/
    uint8_t is_wur_duty_cycle_start_time_present : 1;
    uint8_t is_wur_group_id_list_present : 1;
    uint8_t is_recommended_wur_parameters_present : 1;
    uint8_t reserved_cf : 5;
    /*WUR parameter control field end*/

    /*WUR parameter field begin*/
    uint16_t wur_id : 12;
    uint8_t wur_channel_offset : 3;
    uint8_t reserved_wur_parameter : 1;
    uint64_t wur_duty_cycle_start_time;
    uint8_t wur_group_id_list;  // TODO : have to fix. find what we need to fill in this fields
    /*WUR parameter field end*/
} wur_mode_element_t;

typedef struct {
    uint8_t element_id : 8;
    uint8_t length : 8;
    uint8_t element_id_extension : 8;

    uint8_t action_type : 8;
    uint8_t wur_mode_response_status : 8;

    /*WUR parameter control field begin*/
    uint8_t is_wur_duty_cycle_start_time_present : 1;
    uint8_t is_wur_group_id_list_present : 1;
    uint8_t is_recommended_wur_parameters_present : 1;
    uint8_t reserved_cf : 5;
    /*WUR parameter control field end*/

    /*WUR parameter field for sta begin*/
    uint32_t wur_duty_cycle_service_time;
    uint16_t wur_duty_cycle_time;
    uint8_t wur_wakeup_frame_rate : 2;
    uint8_t wur_channel_offset : 3;
    /*WUR parameter field end*/
} wur_mode_element_sta_t;

typedef struct {
    uint8_t element_id : 8;
    uint8_t length : 8;
    uint8_t element_id_extension : 8;

    /*WUR AP Information set begin*/
    uint8_t wur_discovery_operating_classs;
    uint8_t wur_discovery_channel;
    uint8_t wur_ap_count;
    uint8_t wur_ap_list;  // TODO :  have to fix. find what we need to fill in this field
    /*WUR AP information set end*/
} wur_discoery_element_t;

typedef struct {
    uint8_t cycle_start_time_present : 1;
    uint8_t group_id_list_present : 1;
    uint8_t recommended_wur_parameters_present : 1;
    uint8_t reserved_cf : 5;
} wur_parameter_control_t;

typedef struct {
    uint16_t wur_id : 12;
    uint8_t wur_channel_offset : 3;
    uint8_t reserved_wur_parameter : 1;
    uint64_t wur_duty_cycle_start_time;
    uint8_t wur_group_id_list;
} wur_parameter_ap_t, wur_mode_parameter_t;

typedef struct {
    uint32_t wur_duty_cycle_service_time;
    uint16_t wur_duty_cycle_time;
    uint8_t wur_wakeup_frame_rate : 2;
    uint8_t wur_channel_offset : 3;
    uint8_t keep_alive_wur_frame : 1;
    uint8_t reserved_cf : 8;
} wur_parameter_sta_t;

typedef struct {
    uint8_t minimum_wake_up_duration : 8;
    uint16_t duty_cycle_period_units : 16;
    uint8_t wur_operating_class;
    uint8_t wur_channel;
    uint16_t wur_beacon_period;
    uint16_t offset_of_twbtt;
    uint8_t counter : 4;
    uint8_t common_pn : 1;
    uint8_t reserved : 3;
} wur_operation_parameter_t;

typedef struct {
    // frame control
    uint32_t wur_frame_control_type : 3;
    uint32_t wur_frame_protected : 1;
    uint32_t is_wur_frame_body_present : 1;
    uint32_t wur_frame_body_length : 3;
    // id
    uint32_t wur_id : 12;
    // dependent control
    uint32_t wur_dependent_ctrl_bits : 12;
    uint32_t fcs : 16;
    uint32_t res : 16;  //< reserved

} wur_mac_header_t;

typedef struct {
    uint32_t comp_bssid;
    uint16_t tx_id : 12;
    uint16_t wur_id : 12;
    uint16_t vendor_id[2];
} wur_id_t;

typedef struct {
    uint16_t vendor_id;
    uint16_t vendor_data;
} WUR_STRUCT_VENDOR_CMD;

typedef struct {
    uint16_t id;
} WUR_STRUCT_WAKE_UP_CMD;

struct WUR_FRAME_LIST_S {
    wur_frame_type type;
    uint16_t id;
    uint16_t payload;
    struct WUR_FRAME_LIST_S *next_frame;
};

/**
 * @struct	:	wur_frm_inf_t
 * @Brief	:	It store the wur frame info for wur list
 */
typedef struct {
    uint8_t frame_type;  ///< wur frame type
    uint16_t id;         ///< id used in wur frame
    uint16_t data;       ///< data used in wur frame
} wur_frm_inf_t;

// wur struct
typedef struct {
    uint8_t wur_mode_set_initiated;
    wur_id_t wur_id;
    WUR_CONFIG_T wur_config;
    NT_BOOL dpm_data_availability_interrupt_received;
    WUR_STRUCT_VENDOR_CMD vendor_cmd;
    WUR_STRUCT_WAKE_UP_CMD wakeup_cmd;

    uint8_t vendor_frame_available_count;  ///< vendor frm count in the list
    uint8_t wakeup_frame_available_count;  ///< wake frm count in the list

    NT_BOOL is_dpm_stopped;
    wur_mode_parameter_t wur_mode_parameter;
    wur_operation_parameter_t wur_operation_parameter;

    struct WUR_FRAME_LIST_S *wur_frame_list;
    NT_BOOL is_enter_wur_mode;

    int8_t frm_queue_start_ctr;                  ///< start counter for wur list
    int8_t frm_queue_end_ctr;                    ///< end counter for wur list
    uint8_t frm_queue_count;                     ///< no of frm in the list
    wur_frm_inf_t cmd_queue[WUR_MAX_FRM_QUEUE];  ///< wur frm info
    uint16_t wakeup_counter_ap : 12;             ///< counter for wakeup frame at ap side
    uint16_t wakeup_counter_sta : 12;            ///< wakeup counter at sta side
    uint8_t *wur_bcn;
} WUR_STRUCT_T;

/* @struct	: wur_struct_cts2s_t
 * @brief	: set parameter for CTS to Self frame
 * */
typedef struct {
    uint8_t frm_ctl[2];  ///< frame control
    uint16_t duration;   ///< cts2s duration
    uint8_t address[6];  ///< ap address
    uint32_t crc;        ///< 32 bit crc
} wur_struct_cts2s_t;

/* @struct	: wur_struct_cfe_t
 * @brief	: set parameter for CFE frame
 * */
typedef struct {
    uint8_t frm_ctl[2];  ///< frame control
    uint16_t duration;   ///< duration
    uint8_t address[6];  ///< ap address
    uint8_t bssid[6];    ///< bssid address
    uint32_t crc;        ///< 32 bit crc
} wur_struct_cfe_t;

/* @struct	: wur_struct_sta_list_t
 * @brief	: it store wur sta info at ap
 * */
typedef struct {
    uint8_t sta_mac_add[6]; /*! sta mac add */
    uint16_t sta_wur_id;    /*! sta wur id */
} wur_struct_sta_list_t;

/*Typedef Declaration end*/

/*Function declaration begin*/
uint8_t nt_wur_add_capabilities_element(uint8_t *frm);
uint8_t nt_wur_add_operation_element(uint8_t *frm);
uint8_t nt_wur_add_mode_element(uint8_t *frm, wur_mode_ie_action_type action_type,
                                wur_mode_response_status response_status);
uint8_t nt_wur_add_discovery_element(uint8_t *frm);
uint8_t nt_wur_add_mode_setup_element_ap(uint8_t *frm, uint8_t action_t, uint8_t resp_status,
                                         wur_parameter_control_t *wur_ctrl_prm, wur_parameter_ap_t *prm_ap);
uint8_t nt_wur_add_mode_setup_element(uint8_t *frm, uint8_t action_t, uint8_t resp_status,
                                      wur_parameter_control_t *wur_ctrl_prm, wur_parameter_sta_t *prm_sta);
void nt_wur_init(devh_t *);
nt_status_t nt_wur_send_wur_mode_enter_frame(devh_t *dev);
nt_status_t nt_wur_recv_wur_action_frame(devh_t *dev, uint8_t *frm, uint8_t __attribute__((__unused__)) * frmend,
                                         conn_t *conn);
nt_status_t nt_wur_enter_wur_mode_setup_request(devh_t *dev);
nt_status_t nt_wur_process_wur_mode_frame(devh_t *dev, conn_t *conn);
uint8_t nt_wur_add_wur_beacon_mac_header(uint8_t *frm);

nt_status_t nt_wur_enter_wur_mode();
uint8_t nt_wur_create_wur_frame(devh_t *dev, uint8_t *frm, uint8_t type, uint16_t id, uint16_t td);
uint16_t nt_wur_generate_crc16(uint8_t *data_p, uint16_t length);
wur_id_t nt_wur_save_id(uint8_t *bssid);
uint32_t nt_wur_generate_crc32(uint8_t *buf, uint16_t len);
nt_status_t nt_wur_send_vendor_cmd(devh_t *dev, uint16_t id, uint16_t data);
void nt_wur_stopped_ap_dpm_cb(uint8_t status);
// void nt_wur_data_availability_notification();
void nt_wur_tsf_match_notification_cb();
NT_BOOL nt_wur_process_tsf_match_command(devh_t *dev);
uint64_t nt_wur_calculate_next_twbtt();
void nt_wur_start_dpm_response_handler(uint8_t status);
NT_BOOL nt_wur_process_wur_wakeup_frame_notification(devh_t *dev);
NT_BOOL nt_wur_process_wur_vendor_frame_notification(devh_t *dev, uint16_t id, uint32_t data);
NT_BOOL nt_wur_send_susspend_request(devh_t *dev);
void nt_wur_transition_to_awake(devh_t *dev);
nt_status_t nt_wur_process_data_availability_command(devh_t *dev);
void nt_wur_assoc_notification_ap(NT_BOOL assoc, uint8_t *conn_id);

/**
 *	@Func 	:	nt_wur_process_received_wur_frame
 *	@Brief 	: 	This api process the received wur data and send notification to application
 *	@Param	:	dev - device structure
 *	@Param	:	wur_inf_data - 8 byte received wur frame data
 *	@Return	:	none
 */
void nt_wur_process_received_wur_frame(devh_t *dev, uint64_t wur_inf_data);
NT_BOOL nt_wur_process_wake_up_command_from_sta(devh_t *dev);

nt_status_t nt_wur_config_drivers(devh_t *dev, uint8_t pm_state);
uint64_t nt_wur_minimal_code();
void nt_wur_wakeup_callback(soc_wkup_reason wkup_reason);
void nt_wur_pre_sleep_callback();

///< This function will be triggered when any wur frame received at station.
void nt_wur_frame_received_notify_cb(uint64_t payload);
void nt_wur_set_vendor_id_filter(devh_t *dev, uint16_t vendor_id_1, uint16_t vendor_id_2);

///< This function send the CTS2SELF frame
nt_status_t nt_wur_send_cts2s_frame(devh_t *dev);

///< This function send the CFE frame
nt_status_t nt_wur_send_cfe_frame(devh_t *dev);

/**
 * 	@fruntion	:	nt_wur_dequeue_wur_frm
 * 	@Brief		:	it dequeue wur frame from the queue list based on FIFO
 *	@Param		:	none
 *	@Return		:	wur_frm_inf_t * -> dequeue wur frame info
 */
wur_frm_inf_t *nt_wur_dequeue_wur_frm(devh_t *dev);

/**
 * 	@fruntion	:	nt_wur_add_frm_into_queue
 * 	@Brief		:	add wur frame in the queue list
 *	@Param		:	wur_frm_inf_t -> contains wur frame info
 *	@Return		:	NT_Ok if wur frm added successfully otherwise Error
 */
uint8_t nt_wur_add_frm_into_queue(wur_frm_inf_t *frm_ptr);

/**
 * 	@fruntion	:	nt_wur_display_sta_wur_statistics
 * 	@Brief		:	This function diplay wur stats for sta
 *	@Param		:	none
 *	@Return		:	none
 */
void nt_wur_display_sta_wur_statistics(devh_t *dev);

/**
 * 	@fruntion	:	nt_wur_display_ap_wur_statistics
 * 	@Brief		:	This function diplay wur stats for ap
 *	@Param		:	none
 *	@Return		:	none
 */
void nt_wur_display_ap_wur_statistics(devh_t *dev);

/**
 *	@func	nt_wur_error_frame_recv_cb
 *	@brief	This function will be triggered	when any error frame received at station.
 * 	@Return none
 * 	@Param	none
 */
void nt_wur_error_frame_recv_cb(void);

/**
 *	@func	nt_wur_process_error_frame
 *	@brief	This function is triggered when error frame is received
 *			so wake up the sta and	update wur stats.
 * 	@Param	dev : device stucture
 * 	@Return none
 */
void nt_wur_process_error_frame(devh_t *dev);

/**
 *	@func	nt_wur_send_wakeup_frm_with_updated_counter
 *	@brief	This function is used to update the wakeup_counter and
 *			add wakeup frame into queue to send from AP.
 * 	@Param	none
 * 	@Return none
 */
void nt_wur_send_wakeup_frm_with_updated_counter(devh_t *dev);

/**
 * 	Func	:	nt_wur_data_availability_notification_ap
 *	Brief	:	This function will be called when data is available at AP side.
 *  Return	: 	none
 *  Param	:	status of data availability notification
 */
void nt_wur_data_availability_notification_ap(uint8_t status);

/**
 *	@func	nt_wur_beacon_miss_recv_cb
 *	@brief	This function is triggered when beacon miss interrupt is received.
 * 	@Return none
 * 	@Param	none
 */
void nt_wur_beacon_miss_recv_cb(void);

/**
 *	@func	nt_wur_process_error_frame
 *	@brief	This function is triggered from beacon miss call back is received.
 *			so wake up the sta and	update wur stats.
 * 	@Param	dev : device stucture
 * 	@Return none
 */
void nt_wur_process_beacon_miss(devh_t *dev);

/**
 *	@func	nt_wur_wake_up_received_cb
 *	@brief	This function is triggered from wake main radio interrupt
 * 	@Param	payload : payload of the frame
 * 	@Return none
 */
void nt_wur_wake_up_received_cb(uint64_t payload);

/**
 *	@func	nt_wur_calculate_next_twbtt_sta
 *	@brief	This function is used to calculate next tbtt at sta side
 * 	@Param	none
 * 	@Return next twbtt
 */
uint64_t nt_wur_calculate_next_twbtt_sta(void);

/**
 *	@func	nt_wur_calculate_current_twbtt
 *	@brief	This function is used to calculate current tbtt to send wur beacon
 * 	@Param	none
 * 	@Return next twbtt
 */
uint64_t nt_wur_calculate_current_twbtt(void);

/**
 *	@func	nt_wur_process_tsf_match_for_beacon_command
 *	@brief	This function is used to send wur frame at beacon tsf match
 * 	@Param	dev - device structure
 * 	@Return status
 */
NT_BOOL nt_wur_process_tsf_match_for_beacon_command(devh_t *dev);

/**
 *	@func	nt_wur_tsf_match_for_beacon_cb
 *	@brief	This function will be triggered	when wur frame is to be sent.
 * 	@Return none
 * 	@Param	none
 */
void nt_wur_tsf_match_for_beacon_cb();

/*Function declaration end*/

#endif /* (defined NT_FN_WUR_AP) || (defined NT_FN_WUR_STA) */
#endif /* CORE_WIFI_SME_INC_NT_WUR_H_ */
