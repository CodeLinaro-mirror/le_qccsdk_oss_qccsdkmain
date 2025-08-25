/*========================================================================
Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
*
* @file coex_main.h
* @brief Coex main params and struct definitions
*========================================================================*/
#ifndef COEX_MAIN_H
#define COEX_MAIN_H
/* Standard includes. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "nt_osal.h"

#include "fwconfig_wlan.h"
#include "nt_flags.h"
#include "nt_common.h"

#if defined(SUPPORT_COEX)
#include "wlan_dev.h"
#include "discovery.h"
#include "coex_wlan.h"
#include "wlan_wmi.h"
/*-------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * ----------------------------------------------------------------------*/

#define COEX_MIN(x, y) (((x) <= (y)) ? (x) : (y))
#define COEX_MAX(x, y) (((x) >= (y)) ? (x) : (y))

#define MIN_RSSI 0

#define DEFAULT_COEX_INTERVAL 30000  // in us
#ifndef EMULATION_BUILD
#define WAL_COEX_WSI_STATE_CHECK_MS   5000   // in ms
#define CXC_RESET_BT_EVENT_TIMEOUT_MS 10000  // in ms
#endif

#define COEX_MAX_AGGREGATION_TIME    4000
#define COEX_BT_MULTI_PROF_AGGR_TIME 1000

#define COEX_SET_MIN_RATE gpBtCoexWlanInfoDev->ra_min_rate_enable = 1
#define COEX_CLR_MIN_RATE gpBtCoexWlanInfoDev->ra_min_rate_enable = 0
#define COEX_CHK_MIN_RATE gpBtCoexWlanInfoDev->ra_min_rate_enable ? 1 : 0

#define CXC_RESET_MAX_RETRIES 3

/*-------------------------------------------------------------------------
 * Type Declarations
 * ----------------------------------------------------------------------*/

/* Coex Schedule Types */
enum {
    COEX_SCHED_NONE = 0,
    COEX_SCHED_WLAN = 1,
    COEX_SCHED_BT = 2,
    COEX_SCHED_WLAN_CTS2S = 3,
    COEX_SCHED_INVALID = 0xFF,
};

/* Coex Scheduler Request Types */
enum {
    COEX_SCHED_REQ_NEXT = 0,
    COEX_SCHED_REQ_BT = 1,
    COEX_SCHED_REQ_WLAN = 2,
    COEX_SCHED_REQ_CTS2S = 3,
};

/* Coex SM stimulus */
enum {
    COEX_SM_INVALID = 0,
    COEX_BT_STATE_CHANGE = 1,
    COEX_WLAN_STATE_CHANGE = 2,
    COEX_BT_LIMIT_REACHED = 3,
    COEX_TIMER_EXPIRED = 4,
    COEX_BT_INACTIVITY_REPORT = 5,
    COEX_WLAN_RX_BELOW_THRESHOLD = 6,
    COEX_WLAN_RX_ABOVE_THRESHOLD = 7,
    COEX_INVOKE_SCHEDULER = 8,
    COEX_BT_GRANTED_START = 9,
    COEX_BT_GRANTED_END = 10,
};

enum {
    COEX_ALL_OFF = 0,
    COEX_BTCOEX_NOT_REQD = 1,
    COEX_WLAN_IS_IDLE = 2,
    COEX_EXECUTE_SCHEME = 3,
    COEX_WLAN_IS_PAUSED = 6,
    COEX_WAIT_FOR_NEXT_ACTION = 7,
    COEX_SOC_WAKE = 8,
    COEX_WLAN_SLEEPING = 9,
};

enum rate_monitor_events {
    RX_RATE_MONITOR_NO_EVENT = 0,
    RX_RATE_MONITOR_LO_THRESH_EVENT,
    RX_RATE_MONITOR_HI_THRESH_EVENT
};
enum {
    BTCOEX_IDLE_CONTROL = 0,
    BTCOEX_ACTIVE_ASYNCHRONOUS_CONTROL = 1,
    BTCOEX_PASSIVE_SYNCHRONOUS_CONTROL = 2,
    BTCOEX_ACTIVE_SYNCHRONOUS_CONTROL = 3,
    BTCOEX_DEFAULT_CONTROL = 4,
    BTCOEX_CONCURRENCY_CONTROL = 5,
    BTCOEX_LOW_LATENCY_CONTROL = 6,
    BTCOEX_SCHEME_INVALID = 0xFF,
};

typedef enum COEX_MGR_reason {
    COEX_MGR_REASON_NONE = 0,
    COEX_MGR_REASON_BT_ON = 1,
    COEX_MGR_REASON_BT_OFF = 2,
    COEX_MGR_REASON_TOPOLOGY_RESET = 3,
    COEX_MGR_REASON_PROF_INFO = 4,
    COEX_MGR_REASON_NONLINK_UPDATE = 5,
    COEX_MGR_REASON_BT_SCAN_ACTIVITY = 6,
    COEX_MGR_REASON_BT_SCAN_ACTIVITY_BMAP = 7,
    COEX_MGR_REASON_ADDL_PROF_INFO = 8,
    COEX_MGR_REASON_BT_INACTIVITY_REP = 9,
    COEX_MGR_REASON_BT_TXRX_CNT_LIM = 10,
    COEX_MGR_REASON_ACL_INTERACTIVE = 11,
    COEX_MGR_REASON_WLAN_CAL_END = 12,
    COEX_MGR_REASON_CTS2S_INT = 13,
    COEX_MGR_REASON_CFG_MGR = 14,
    COEX_MGR_REASON_PRIO_TMR_EXP = 15,
    COEX_MGR_REASON_WLAN_LAYER_READY = 16,
    COEX_MGR_REASON_STATE_IND_TMR = 17,
    COEX_MGR_REASON_WBTMR_SYNC = 18,
    COEX_MGR_REASON_WLAN_PWR_SLEEP = 20,
    COEX_MGR_REASON_WLAN_PWR_WAKE = 21,
    COEX_MGR_REASON_BT_GRANTED_START = 22,
    COEX_MGR_REASON_BT_GRANTED_END = 23,
    COEX_MGR_REASON_PEER_SECURITY_TO = 25,
    COEX_MGR_REASON_PEER_RX_RATE_CHNGE = 26,
    COEX_MGR_REASON_VDEV_UPDATE = 27,
    COEX_MGR_REASON_BT_QOS_UPDATE = 28,
    COEX_MGR_REASON_BT_TX_POWER_UPDATE = 29,
    COEX_MGR_REASON_BT_RSSI_UPDATE = 30,
    COEX_MGR_REASON_WLAN_TX_STATE_CHNGE = 31,
    COEX_MGR_REASON_ACL_ACTIVE_SCO_TMR = 32,
    COEX_MGR_REASON_ISO_CONN = 33,
    COEX_MGR_REASON_ISO_PRIO_MASK = 34,
    COEX_MGR_BAND_CHANGE_STIMULUS = 35,

} E_COEX_MGR_REASON;

#define COEX_EXIT_CRIT_ASSOC coex_exit_crit_assoc()

enum {
    COEX_WLAN_CONNECTION_COMPLETE = 1,
    COEX_WLAN_SECURITY_INPROGRESS = 2,
};

/*-------------------------------------------------------------------------
 * Function Declarations and Documentation
 * ----------------------------------------------------------------------*/
void coex_exit_crit_assoc();
void coex_manager(uint32_t Stimulus, __attribute__((__unused__)) E_COEX_MGR_REASON Reason);
void coex_dev_mgr_cb(devh_t *dev, wlan_dev_event *event);
void coex_scan_event_cb(devh_t *dev, DC_EVENT *event);
void coex_rssi_monitor(uint8_t devId);
void coex_prioritize_beacon_rx(bool PrioritizeBeaconRx, uint8_t BeaconID);
void coex_inc_beacon_priority();
void coex_reset_beacon_priority();
void _nt_post_coex_prio_timer_timeout_msg(nt_osal_timer_handle_t thandle);
void coex_prio_timer_handler();
bool coex_config_min_rate(uint8_t min_rate);
void coex_main_set_wlan_aggr_limit(uint8_t TrafficAlgo);
#if defined(PLATFORM_FERMION) && !defined(EMULATION_BUILD)
void _nt_post_coex_wsi_state_timer_timeout_msg(nt_osal_timer_handle_t thandle);
void coex_wsi_state_handler();
void _nt_post_cxc_reset_bt_event_timeout_msg(nt_osal_timer_handle_t thandle);
void cxc_reset_bt_event_timeout();
#endif

#endif /* SUPPORT_COEX */
#endif /* #ifndef  COEX_MAIN_H */
