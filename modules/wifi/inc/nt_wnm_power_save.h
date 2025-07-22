/*
Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#ifndef CORE_WIFI_SME_INC_NT_WNM_POWER_SAVE_H_
#define CORE_WIFI_SME_INC_NT_WNM_POWER_SAVE_H_
#include <stdio.h>
#include "wifi_cmn.h"

#include "fwconfig_wlan.h"
#include "nt_flags.h"

#ifdef NT_FN_WNM_POWERSAVE_MODE
#include "nt_common.h"
#include "wlan_dev.h"
#include "wlan_framegen.h"

/**
 * In Extended capability, 17th bit is used for WNM_SLEEP_MODE feature.
 * If 17th bit is set as 1 then WNM_SLEEP_MODE feature will be enabled.
 */
#define WNM_SLEEP_MODE_CAPABILITY (1 << 17)

/* If wnm sleep time is not set by command then, it will set by default value i.e 10 sec */
#define WNM_SLEEP_MODE_INTERVAL 10000

/* If wnm sleep time is not set by command then, it will set by default value i.e 180 sec */
#define WNM_BSS_MAX_IDLE_TIME 180

/* dialog taken for wnm action frame */
#define WNM_SLEEP_MODE_DIALOG_TOKEN 1

#define WNM_SLEEP_MODE_ENTER 0 /* Action type to enter into wnm sleep mode */
#define WNM_SLEEP_MODE_EXIT  1 /* Action type to exit from wnm sleep mode */
#define WNM_STA_TFS_ID       1

#define DEFAULT_SEND_SUSPEND_REQ_FRAME_AT_DATA_AVAILABILITY  1
#define DEFAULT_SEND_SUSPEND_RESP_FRAME_AT_DATA_AVAILABILITY 1

/* @struct	: wnm_config_t
 * @brief	: parameter for wnm power-save configuration
 * */
typedef struct {
    // NT_BOOL dot11WirelessManagementImplemented;
    // NT_BOOL dot11TFSActivated;
    NT_BOOL wnm_sleep_mode_enable;                    ///< To check sleep mode is feature is enable/disable
    uint32_t wnm_sleep_interval;                      ///< sleep time interval for STA in wnm sleep mode
    NT_BOOL is_requested_ap_have_wnm_capabilities;    ///< check for wnm capability at ap side
    NT_BOOL is_requesting_sta_have_wnm_capabilities;  ///< check for wnm capability at sta side
    NT_BOOL
        allow_wake_up_when_sta_data_availablity;  ///< Permission for Wakeup enable due to data availability at sta side
    NT_BOOL allow_wake_up_if_received_wake_up_frame;  ///< Permission for Wakeup enable due to sleepmode suspend req at
                                                      ///< AP side
    NT_BOOL
        allow_wake_up_when_ap_data_availablity;  ///< Permission for Wakeup enable due to data availability at AP side
    uint8_t wnm_dtim_enable_policy;              ///
    uint32_t wnm_bss_max_idle_time;              ///< bss max idle time
    NT_BOOL is_connected_ap_have_proxy_arp_support;  /// check connected AP have proxy arp support or not.
} wnm_config_t;

/* @struct	: wnm_ps_struct_t
 * @brief	: parameter for wnm power-save parameter
 * */
typedef struct {
    uint8_t wnm_mode_set_initiated;                    ///< Flag to check the sta is currently in Enter sleep mode stat
    wnm_config_t wnm_config;                           ///< configuration for wnm sleep mode feature
    NT_BOOL dpm_data_availability_interrupt_received;  ///< Flag for data availability interrupt
    uint8_t wnm_bss_idle_timout_exit_flag;             ///< Flag to check sta exit due to bss idle timer
} wnm_ps_struct_t;

/* @struct	: wnm_ps_sleep_mode_element
 * @brief	: parameter for sleep mode element used into action frame
 * */
typedef struct {
    uint8_t element_id;         ///< element id
    uint8_t length;             ///< length of the sleep mode element data
    uint8_t action_type;        ///< action type (Enter/Exit) for wnm sleep mode
    uint8_t sleep_resp_status;  ///< response status for sleep mode frames
    uint16_t sleep_interval;    ///< sleep time for sta when enter into wnm sleep mode
} __ATTRIB_PACK wnm_ps_sleep_mode_element_t;

/* @struct	: wnm_ps_tfs_resp_sub_element_t
 * @brief	: parameter for tfs sub element
 * */
typedef struct {
    uint8_t id;
    uint8_t len;
    uint8_t resp_data;
} __ATTRIB_PACK wnm_ps_tfs_resp_sub_element_t;

/* @struct	: wnm_tclas_element_t
 * @brief	: Parameter traffic classifier element
 * */
typedef struct {
    uint8_t id;                ///< tclas element id
    uint8_t len;               ///< tclas element length
    uint8_t user_priority;     ///< user priority for tclas
    uint8_t frame_classifier;  ///< tclas frame classifier
} __ATTRIB_PACK wnm_ps_tclas_element_t;

/* @struct	: wnm_tsf_req_sub_element_t
 * @brief	: parameter for tfs sub element
 * */
typedef struct {
    uint8_t sub_element_id;                ///< element id
    uint8_t length;                        ///< length of sub element data
    wnm_ps_tclas_element_t tclas_element;  ///< traffic classifier element
    uint8_t tclas_processing_element;      ///< tclas processing element
} __ATTRIB_PACK wnm_ps_tsf_req_sub_element_t;

/* @struct	: wnm_ps_tfs_req_element
 * @brief	: parameter for sleep mode element used into action frame
 * */
typedef struct {
    uint8_t element_id;                                ///< element id
    uint8_t length;                                    ///< length of the sleep mode element data
    uint8_t tfs_id;                                    ///< action type (Enter/Exit) for wnm sleep mode
    uint8_t tfs_action;                                ///< response status for sleep mode frames
    wnm_ps_tsf_req_sub_element_t tsf_req_sub_element;  ///< sleep time for sta when enter into wnm sleep mode
} __ATTRIB_PACK wnm_ps_tfs_req_element_t;

/* @struct	: wnm_ps_tfs_resp_element
 * @brief	: parameter for sleep mode element used into action frame
 * */
typedef struct {
    uint8_t element_id;  ///< element id
    uint8_t length;      ///< length ofwnm_ps_sleep_mode_req_t the sleep mode element data
    uint8_t tfs_id;      ///< tfs id
    wnm_ps_tfs_resp_sub_element_t tfs_resp_sub_element;  ///< tfs resp sub element
} __ATTRIB_PACK wnm_ps_tfs_resp_element_t;

/* @struct	: wnm_ps_sleep_mode_req_t
 * @brief	: Parameter for sleep mode request
 * */
typedef struct {
    uint8_t catagory;                               ///< frame catagory
    uint8_t wnm_action;                             ///< tclas element length
    uint8_t dialog_token;                           ///< dialog token
    wnm_ps_sleep_mode_element_t wnm_sleep_element;  ///< sleep mode element
    wnm_ps_tfs_req_element_t tfs_req_element;       ///< tfs request element
} __ATTRIB_PACK wnm_ps_sleep_mode_req_t;

/* @struct	: wnm_ps_sleep_mode_resp_t
 * @brief	: Parameter for sleep mode response frame
 * */
typedef struct {
    uint8_t catagory;                               ///< frame catagory
    uint8_t wnm_action;                             ///< tclas element length
    uint8_t dialog_token;                           ///< dialog token
    uint16_t key_data_len;                          ///< key data length
                                                    //	uint16_t key_data;								///< key data if protection is on
    wnm_ps_sleep_mode_element_t wnm_sleep_element;  ///< sleep mode element
    wnm_ps_tfs_resp_element_t tfs_resp_element;     ///< tfs request element
} __ATTRIB_PACK wnm_ps_sleep_mode_resp_t;

/**
 *	@func	nt_wnm_ps_init
 *	@brief	This function is used to initialize parameter related with wnm feature.
 * 	@Return none
 * 	@Param	none
 */
void nt_wnm_ps_init(devh_t *dev);
/**
 *	@func	nt_wnm_ps_sleep_mode_enter_frame
 *	@brief	This function is used to create and send wnm sleep mode enter frame.
 * 	@Return NT_OK -> send successfully , NT_ETXFAIL -> Transmission failed
 * 	@Param	none
 */
nt_status_t nt_wnm_ps_sleep_mode_enter_frame(devh_t *dev);

/**
 *	@func	nt_wnm_ps_sleep_mode_suspend_frame
 *	@brief	This function is used to create and send wnm sleep mode suspend frame.
 * 	@Return NT_OK -> send successfully , NT_ETXFAIL -> Transmission failed
 * 	@Param	none
 */
nt_status_t nt_wnm_ps_sleep_mode_suspend_frame(devh_t *dev);

/**
 *	@func	nt_wnm_recv_wnm_action_frame
 *	@brief	This function is used to handle received wnm action frame.
 * 	@Return NT_OK -> send successfully , NT_ERXFAIL -> Error in Rx frame
 * 	@Param	dev -> Pointer to device structure
 * 			frm -> received frame pointer
 * 			frmend -> pointer to end of frame
 * 			conn -> pointer to connection structure
 */
nt_status_t nt_wnm_recv_wnm_action_frame(devh_t *dev, uint8_t *frm, uint8_t __attribute__((__unused__)) * frmend,
                                         conn_t *conn);

/**
 *	@func	nt_wnm_process_data_availability_command
 *	@brief	This function is used to wakeup the system due to data availability.
 * 	@Return NT_OK -> send successfully , NT_EFAIL -> Operation failed
 * 	@Param	dev -> Pointer to device structure
 */
nt_status_t nt_wnm_process_data_availability_command(devh_t *dev);

/**
 *	@func	nt_wnm_transition_to_awake
 *	@brief	This function is used to notify connection manager for becon receive and set
 *			the tramsition state to AWAKE.
 * 	@Return NT_OK -> send successfully , NT_EFAIL -> Operation failed
 * 	@Param	dev -> Pointer to device structure
 */
void nt_wnm_transition_to_awake(devh_t *dev);

/**
 *	@func	nt_wnm_wakeup_callback
 *	@brief	This function is call back function wakeup from WNM sleep mode.
 * 	@Return none
 * 	@Param	wkup_reason  : SOC reason for wakeup
 */
void nt_wnm_wakeup_callback(soc_wkup_reason wkup_reason);

/**
 *	@func	nt_wnm_enter_sleep_callback
 *	@brief	This function is call back function to enter into sleep mode.
 * 	@Return none
 * 	@Param	none
 */
void nt_wnm_enter_sleep_callback(void);

/* Name : nt_wnm_process_data_available_interrupt
 * Routine description  : process data path tx data available interrupt
 * Arguments            : None
 * Return value         : None
 *
 */
void nt_wnm_process_data_available_interrupt(void);

/* Name : nt_wnm_process_data_available_interrupt_from_ap
 * Routine description  : process data path tx data available interrupt
 * Arguments            : None
 * Return value         : None
 *
 */
void nt_wnm_process_data_available_interrupt_from_ap(void);

/**
 *	@Func 	:	nt_wnm_ps_display_wnm_stats_ap
 *	@Brief 	: 	This api will display wur stats at sp
 *	@Param	:	none
 *	@Return	:	none
 */
void nt_wnm_ps_display_wnm_stats_ap(devh_t *dev);
/**
 *	@Func 	:	nt_wnm_ps_display_wnm_stats_sta
 *	@Brief 	: 	This api will display wnm stats at sta
 *	@Param	:	none
 *	@Return	:	none
 */
void nt_wnm_ps_display_wnm_stats_sta(devh_t *dev);

/**
 *	@Func 	:	nt_wnm_post_bss_idle_timeout_msg
 *	@Brief 	: 	This API Posts bss idle timeout message to wmi
 *	@Param	:	timer handler
 *	@Return	:	none
 */
void nt_wnm_post_bss_idle_timeout_msg(TimerHandle_t thandle);
/**
 *	@Func 	:	nt_wnm_bss_idle_timeout_cb
 *	@Brief 	: 	functin for bss idle timer expire
 *	@Param	:	timer handler
 *	@Return	:	none
 */
void nt_wnm_bss_idle_timeout_cb(TimerHandle_t __attribute__((__unused__)) timer_handle);

#endif /* NT_FN_WNM_POWERSAVE_MODE */

#endif /* CORE_WIFI_SME_INC_NT_WNM_POWER_SAVE_H_ */
