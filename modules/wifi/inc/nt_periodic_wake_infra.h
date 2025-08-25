/*
Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
*/
#ifndef CORE_WIFI_SME_INC_NT_PERIODIC_WAKE_INFRA_H_
#define CORE_WIFI_SME_INC_NT_PERIODIC_WAKE_INFRA_H_

#include "fwconfig_wlan.h"
#include "nt_flags.h"
#include <stdio.h>
#include "nt_common.h"

#ifdef FEATURE_PERIODIC_WAKE_SLEEP
#include "wlan_dev.h"
#include "pm_api.h"
#include "nt_socpm_sleep.h"
#include "wifi_fw_mgmt_api.h"

#define VOICE_TRAFFIC_ITO MS_TO_US(5)

#define PERIODIC_TRAFFIC_WI_30MS  MS_TO_US(30)
#define PERIODIC_TRAFFIC_WI_70MS  MS_TO_US(70)
#define PERIODIC_TRAFFIC_WI_130MS MS_TO_US(130)

#define IS_SESSION_ID_VALID(idx) \
    ((idx == WLAN_PERIODIC_TRAFFIC_SESSION_ID) ? 1 : 0)  // Supporting one session with id = 0
#define IS_TRAFFIC_TYPE_VALID(type) ((type < WLAN_PERIODIC_TRAFFIC_INVALID) ? 1 : 0)
#define IS_WAKE_INTERVAL_VALID(wi)    \
    ((wi == PERIODIC_TRAFFIC_WI_30MS) \
         ? 1                          \
         : ((wi == PERIODIC_TRAFFIC_WI_70MS) ? 1 : ((wi == PERIODIC_TRAFFIC_WI_130MS) ? 1 : 0)))

#define LOWER32BIT(__val) ((uint32_t)(((__val)&0xFFFFFFFFull)))
#define UPPER32BIT(__val) ((uint32_t)((((__val) >> 32) & 0xFFFFFFFFull)))

/* Max sleep time that we use in ptsm */
#define PTSM_MAX_SLEEP_TIME_US MS_TO_US(200)

/* Wait time for a dtim bcn during active mode */
#define PTSM_DTIM_BCN_WAIT_TIME_US MS_TO_US(2)

/* Sleep to wake sw delay in mcu sleep */
#define MCU_SLEEP_S2W_SW_DELAY_IN_US                                                                           \
    (MCU_SLEEP_FIRST_RRI_RESTORE_US + MCU_SLEEP_SEC_RRI_RESTORE_US + PM_NULL_FRAME_TX_TIME_US +                \
     HALPHY_SET_CHA_AND_RATE_TBL_RESTORE_TIME_US + CTX_RESTORE_TO_SCHED_RESTART_US + DEBUG_LOG_PRINT_TIME_US + \
     SLEEP_EXIT_TO_DPM_START_TIME_US)
/* Sleep to wake sw delay in light sleep */
#define LIGHT_SLEEP_S2W_SW_DELAY_IN_US                                                                     \
    (LIGHT_SLEEP_RRI_RESTORE_US + PM_NULL_FRAME_TX_TIME_US + HALPHY_SET_CHA_AND_RATE_TBL_RESTORE_TIME_US + \
     CTX_RESTORE_TO_SCHED_RESTART_US + DEBUG_LOG_PRINT_TIME_US + SLEEP_EXIT_TO_DPM_START_TIME_US)
/* Sleep to wake sw delay in clk gated sleep */
#define CLK_GTK_SLEEP_S2W_SW_DELAY_IN_US                                                                  \
    (CLK_GTK_SLEEP_MIN_CB_TO_SLEEP_EXIT_US + PM_NULL_FRAME_TX_TIME_US + SLEEP_EXIT_TO_DPM_START_TIME_US + \
     HALPHY_DEBUG_LOG_TIME_US)

/* Time taken by dbg logs in mcu sleep */
#define MCU_SLEEP_DBG_LOG_TIME_US 3000

/* SW delay needed during S2W and W2S transition */
#define PTSM_MCU_OVERALL_SW_DELAY_US \
    (MCU_SLEEP_S2W_SW_DELAY_IN_US + MCU_SLEEP_SW_DELAY_SS_TO_SLEEP_REG_US - MCU_SLEEP_DBG_LOG_TIME_US)
#define PTSM_LIGHT_OVERALL_SW_DELAY_US   (LIGHT_SLEEP_S2W_SW_DELAY_IN_US + LIGHT_SLEEP_SW_DELAY_SS_TO_SLEEP_REG_US)
#define PTSM_CLK_GTK_OVERALL_SW_DELAY_US (CLK_GTK_SLEEP_S2W_SW_DELAY_IN_US + CLK_GTK_SLEEP_SW_DELAY_SS_TO_SLEEP_REG_US)

#define SYS_MIN_SLEEP_TIME_IN_US (PTSM_CLK_GTK_OVERALL_SW_DELAY_US)

#define ITO_WAIT_CNT_IN_SP_EXTENSION 3

#define AVERAGE_AITO_MS(p_struct) ((p_struct)->pw_ito_steps * US_TO_MS((p_struct)->pw_idle_timer_timeout_in_us))
#define AVERAGE_SITO_MS(p_struct) (((p_struct)->pw_ito_steps * US_TO_MS((p_struct)->pw_idle_timer_timeout_in_us)) / 2)
#define AVERAGE_CHAN_CONG         25

/* @struct      : nt_periodic_wake_struct_t
 * @brief       : parameters require for periodic wake and sleep infra
 */
typedef struct {
    uint64_t sleep_duration_us;           /*periodic wake session sleep duration in us*/
    uint64_t next_dtim_tbtt_us;           /*next dtim tsf which is near to 500ms*/
    uint64_t next_sp_tsf_us;              /*periodic wake session next sp start tsf in us*/
    uint32_t wake_interval_us;            /*periodic wake session wake interval in us*/
    uint32_t pw_idle_timer_timeout_in_us; /*periodic wake session idle timer timeout in us*/
    uint16_t traffic_start_delta_in_ms;   /*periodic wake session first SP will start after this many ms*/
    uint16_t dtim_wake_time_in_ms;        /*DTIM wake time in ms*/
    uint16_t dtim_bmiss_cnt_in_sp_ext;    /*no. of dtim bcn missed cnt during sp extension*/
    uint16_t dtim_rxd_cnt_in_sp_ext;      /*no. of dtim bcn receive cnt during sp extension*/
    uint16_t dtim_bmiss_cnt_in_ram_min;   /*no. of dtim bcn missed cnt in ram min code*/
    uint16_t dtim_rxd_cnt_in_ram_min;     /*no. of dtim bcn receive cnt in ram min code*/
    uint16_t swake_time_ms;               /* speculative wake up time in ms*/
    sleep_mode slp_mode;                  /*periodic traffic sleep mode */
    PM_TIMER ptsm_idle_timer;             /*periodic traffic idle timer */
    uint8_t traffic_type;                 /*periodic wake session Traffic type:Audio, Voice*/
    uint8_t session_id;                   /*periodic wake session id: valid =0, rest all = invalid*/
    uint8_t is_session_active;            /*To check whether a session is active or not */
    uint8_t pw_ito_steps;    /*periodic wake session ITO steps, step=5, idle timer timeout = 5ms, ITo = 5*5 = 25ms*/
    uint8_t is_aito_enable;  /* Check if adaptive ito feature is enabled through ini*/
    uint8_t is_sito_enable;  /* Check if speculative ito feature is enabled through ini*/
    uint8_t sleep_aborted;   /*To check if sleep is aborted or not */
    uint8_t dtim_in_curr_wi; /*To check if dtim bcn is coming in this wake interval */
    uint8_t is_dtim_rx;      /*To check if dtim bcn is received in current wake interval */
    uint8_t is_dtim_coming_during_slp;   /*To check if dtim bcn is falling in ptsm sleep*/
    uint8_t is_fake_timer_active;        /*To check if fake timer is active in sp extention case*/
    uint8_t fake_timer_timeout;          /*fake timer timeout in sp extention case*/
    uint8_t ptsm_enable;                 /*To check if ptsm feature is enabled through ini*/
    uint8_t is_ptsm_enable_in_idle_mode; /*To check if ptsm feature is enabled through ini*/
    uint8_t print_stat_at_sp;            /*When this flag is set we will print stat in each sp */
    uint8_t aito_ms;                     /* adaptive ito in ms*/
    uint8_t sito_ms;                     /* speculative ito in ms*/
    uint8_t swake_max_cnt;               /* speculative wake up max counts*/
    uint8_t swake_curr_cnt;              /* speculative wake up current counts*/
    uint8_t is_mid_point_wake_needed;    /* Check if mid point wake is needed in current SI*/
    uint8_t is_tx_rx_activity_in_sp;     /* Check if there is some tx/rx activity in current SP*/
#ifdef FEATURE_CONTENT_AFTER_BEACON
    uint8_t is_dtim_rx_mc_bc_set;      /*To check recvd bcn have mc bc set */
    uint8_t is_pm_set_null_frame_sent; /*To check null frame sent during sp extension */
    uint8_t is_mc_bc_ito_enabled;      /*To check ito enabled for mc_bc ito after sp extension */
#endif                                 /* FEATURE_CONTENT_AFTER_BEACON */
} nt_periodic_wake_struct_t;

/*
 * @brief  : Setup periodic traffic params, start periodic wake and sleep session
 * @param  : cmd_param - pointer which contains information related to wlan_periodic_traffic_setup_cmd_t command
 * @param  : dev - Pointer to device structure
 * @return : nt_status_t
 */
nt_status_t nt_periodic_wake_infra_traffic_setup_cmd_hdl(devh_t *dev, wlan_periodic_traffic_setup_cmd_t *cmd_param);

/*
 * @brief  : Get periodic traffic params e.g wake interval, next_sp_tsf
 * @param  : cmd_param - pointer which contains information related to wlan_periodic_traffic_status_cmd_t command
 * @param  : dev - Pointer to device structure
 * @return : nt_status_t
 */
nt_status_t nt_periodic_wake_infra_traffic_status_cmd_hdl(devh_t *dev, wlan_periodic_traffic_status_cmd_t *cmd_param);

/*
 * @brief  : Teardown periodic traffic session
 * @param  : cmd_param - pointer which contains information related to wlan_periodic_traffic_teardown_cmd_t command
 * @param  : dev - Pointer to device structure
 * @return : nt_status_t
 */
nt_status_t nt_periodic_wake_infra_traffic_teardown_cmd_hdl(devh_t *dev,
                                                            wlan_periodic_traffic_teardown_cmd_t *cmd_param);

/*
 * @brief  : Print useful power stats in periodic session
 * @param  : traffic_type - traffic type 0-Audio, 1-Voice
 * @param  : session_id   - session id
 * @param  : print_at_sp  - 1- print at each sp start, 0-print once
 * @param  : dev - Pointer to device structure
 * @return : nt_status_t
 */
nt_status_t nt_periodic_wake_infra_traffic_pm_stats_cmd_hdl(devh_t *dev, uint8_t traffic_type, uint8_t session_id,
                                                            uint8_t print_at_sp);

/*
 * @brief  : send periodic traffic setup(WI, session_id, traffic_type, first_SP_tsf, status) in the event
 * @param  : status - traffic setup status
 * @param  : dev - Pointer to device structure
 * @return : none
 */
void nt_periodic_wake_infra_traffic_setup_event(devh_t *dev, wlan_periodic_traffic_evt_status_t status);

/*
 * @brief  : send periodic traffic status(WI, session_id, traffic_type, next_SP_tsf) params in the event
 * @param  : status - traffic status
 * @param  : dev - Pointer to device structure
 * @return : none
 */
void nt_periodic_wake_infra_traffic_status_event(devh_t *dev, wlan_periodic_traffic_evt_status_t status);

/*
 * @brief  : send periodic traffic setup(session_id, traffic_type, teardown_reason) in the event
 * @param  : status - traffic teardown status
 * @param  : dev - Pointer to device structure
 * @return : none
 */
void nt_periodic_wake_infra_traffic_teardown_event(devh_t *dev, wlan_periodic_traffic_evt_status_t status);

/*
 * @brief 	: Check if any periodic traffic session is active or not.
 * @param 	: dev pointer
 * @return 	: Ture/False
 */
NT_BOOL nt_periodic_wake_infra_is_session_active(devh_t *dev);

/*
 * @brief 	: process the periodic wake sp
 * @param 	: dev pointer
 * @return 	: None
 */
void nt_periodic_wake_infra_process_sp(devh_t *dev);

/*
 * @brief	: process the periodic wake sleep
 * @param	: dev pointer
 * @return 	: None
 */
void nt_periodic_wake_infra_goto_sleep(devh_t *dev);

/*
 * @brief	: post periodic wake timeout message to wlan task
 * @param  	: Timer handler
 * @return 	: None
 */
void nt_periodic_wake_infra_Idle_timeout_msg(TimerHandle_t thandle);

/*
 * @brief	: periodic wake timeout function which takes decision to enter into sleep or continue to be awake
 * @param	: Timer handler
 * @return 	: None
 */
void nt_periodic_wake_infra_ptsm_idle_timer_timeout(TimerHandle_t timer_handle);

/**
 * @brief This function is call back function wakeup from periodic sleep.
 * @Return none
 * @Param wkup_reason    : SOC reason for wakeup
 */
void nt_periodic_wake_infra_wakeup_callback(soc_wkup_reason wkup_reason);

/**
 * @brief This function is min call back function which will process beacons.
 * @Return uint64_t
 * @Param wkup_reason    : wkup_delay_us
 */
uint64_t nt_periodic_wake_infra_min_callback(uint32_t wkup_delay_us);

/**
 * @brief  : This function is call back function when enter into periodic sleep.
 * @Return : none
 * @Param  : none
 */
void nt_periodic_wake_infra_enter_sleep_callback(void);

/*
 * @brief	: periodic traffic wakeup processing function
 * @param	: dev poniter
 * @return 	: None
 */
void nt_periodic_wake_infra_wakeup_processing(void);

/*
 * @brief 	: periodic traffic transitioning to awake from sleep.
 * @param 	: dev poniter
 * @return 	: None
 */
void nt_periodic_wake_infra_transition_to_awake(devh_t *dev);

/**
 * @brief This function is sleep solver function which will calculate sleep time and sleep mode.
 * @Return : none
 * @Param  : dev pointer
 */
void nt_periodic_wake_infra_sleep_solver(devh_t *dev);

/* Name : nt_periodic_wake_infra_process_pm_dph_data_available_interrupt
 * process data path tx data available interrupt
 * Arguments            : None
 * Return value         : None
 */
void nt_periodic_wake_infra_process_pm_dph_data_available_interrupt(void);

/*****************************************************************************
 * @brief Calculate the sleep time for periodic traffic and enters into sleep.
 * @param dev pointer
 * @return none
 ******************************************************************************/
void nt_periodic_wake_infra_ptsm_go_to_sleep(devh_t *dev);

/* @Brief : Tops down wakeup when we receive interface cmds during soc sleep.
 *  Arguments            : dev pointer
 *  Return value         : None
 */
void nt_periodic_wake_infra_ptsm_wake_up_notification_sta(devh_t *dev);

/*****************************************************************************
 * @brief Calculate the next sp tsf.
 * @param dev pointer
 * @return next_sp_tsf
 ******************************************************************************/
uint64_t nt_periodic_wake_infra_get_next_sp_tsf(devh_t *dev);

/*****************************************************************************
 * @brief Get the last sp tsf.
 * @param dev pointer
 * @return curr_sp_tsf
 ******************************************************************************/
uint64_t nt_periodic_wake_infra_get_curr_sp_tsf(devh_t *dev);

/*****************************************************************************
 * @brief Get the PTSM wake interval
 * @param dev pointer
 * @return wake interval
 ******************************************************************************/
uint32_t nt_periodic_wake_infra_get_ptsm_wake_interval(devh_t *dev);

/* @Brief : TO check if DTIM beacon near to 500ms is coming in current wake interval.
 *  Arguments            : pointer to dev
 *  Return value         : TRUE/FALSE
 */
bool nt_periodic_wake_infra_is_dtim_bcn_coming_in_curr_wi(devh_t *dev);

/* @Brief : Update the dtim bcn missed status.
 *  Arguments            : pointer to dev
 *  Return value         : TRUE/FALSE
 */
bool nt_periodic_wake_update_dtim_bmiss(devh_t *dev);

/* @Brief : TO check if STA is connected to SAP or third party AP
 *  Arguments            : pointer to dev
 *  Return value         : TRUE/FALSE
 */
bool nt_periodic_wake_infra_is_connected_to_sap(devh_t *dev);

/* @Brief : Calculate the next dtim bcn tsf which is near to 500ms.
 *  Arguments            : Next dtim beacon tbtt, dev pointer
 *  Return value         : Next dtim beacon tbtt which is near to 500ms
 */
uint64_t nt_periodic_wake_infra_get_dtim_bcn_tbtt(devh_t *dev, uint64_t next_dtim_tbtt);

/* @Brief : Check if tsf sync is needed in current sp.
 *  Arguments            : expected sleep time, dev pointer
 *  Return value         : TRUE/FALSE
 */
bool nt_periodic_wake_infra_is_time_sync_required(devh_t *dev, uint64_t next_slp_time_exp);

/* @Brief : calculate the sleep back time in ram minimal code during dtim rx.
 *  Arguments            : dev pointer
 *  Return value         : sleep back time in us
 */
uint64_t nt_periodic_wake_infra_dtim_sleep_solver(devh_t *dev);

/* @Brief : calculate the beacon wait time in ram minimal code during dtim rx.
 *  Arguments            : dev pointer
 *  Return value         : beacon wait time in us
 */
uint64_t nt_periodic_wake_infra_bcn_wait_time(devh_t *dev);

/* @Brief : Print the ptsm stats.
 *  Arguments            : dev pointer
 *  Return value         : none
 */
void nt_periodic_wake_infra_print_ptsm_stats(devh_t *dev);

/* @Brief : set dtim policy to handle multicast services during periodic wake
*  Arguments            : dev pointer
                          default_dtim_policy TRUE - default DTIM intervals will be set
                          else PTSM DTIM intervals will be followed
*  Return value         : True on successful dtim policy update else False will be returned.
*/
bool nt_periodic_wake_infra_set_dtim_policy_to_default(devh_t *dev, bool default_dtim_policy);

/* @Brief : Check if speculative wakeup needed.
 *  Arguments            : dev pointer
 *  Return value         : bool
 */
bool nt_periodic_wake_infra_is_mid_point_wake_needed(devh_t *dev);

/* @Brief : Update qpower stats.
 *  Arguments            : dev pointer
 *  Return value         : none
 */
void nt_periodic_wake_infra_update_qpower_stats(devh_t *dev);

/* @Brief : adjust sp tsf after vdev transition
 *  Arguments            : dev pointer
 *  Return value         : none
 */
void nt_periodic_wake_infra_adjust_sp_after_vdev_transition(devh_t *dev);

#ifdef FEATURE_CONTENT_AFTER_BEACON

/* @Brief : Handle MC/BC after receiving the DTIM during SP extension.
 *  Arguments            : pointer to dev
 *  Return value         : Bool; True if MC/BC handled else false
 */
NT_BOOL nt_periodic_wake_handle_mc_bc_in_sp_ext(devh_t *dev);

/*
 * Restart PTSM idle timer after CAB expiry
 * Arguments            :
 *          pointer to dev
 * Return value         :
 *          None
 */
void nt_periodic_restart_ptsm_idle_timer_after_cab(devh_t *dev);

#endif /* FEATURE_CONTENT_AFTER_BEACON */

#endif /* FEATURE_PERIODIC_WAKE_SLEEP */

#endif /* CORE_WIFI_SME_INC_NT_PERIODIC_WAKE_INFRA_H_ */
