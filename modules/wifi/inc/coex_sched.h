/*========================================================================
Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
*
* @file coex_sched.h
* @brief Coex scheduling params and struct definitions
*========================================================================*/
#ifndef COEX_SCHED_H
#define COEX_SCHED_H
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
#include "coex_utils.h"
/*-------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * ----------------------------------------------------------------------*/
#define COEX_SCHED_DETERMINE_DURATION (0xFFFFFFFF)
#define COEX_SCHED_INVALID_TIME       (0xFFFFFFFF)

#define LOW_PRIO_WLAN_KPI_REQ_PCT (65)

#define COEX_WLAN_DHCP_DETECTION_TIMEOUT 5000  // ms
#define BT_BW_LIMIT                      (gpBtCoexBtInfoDev->CoexMgrPolicy & COEX_POLICY_BT_BW_LIMIT_ENABLE)

/* BtCoexSchedulerFlags */
#define COEX_SCHED_SCAN_INACTIVE_CB (0x0000001)
#define IS_COEX_SCHED_SCAN_INACTIVE_CB \
    IS_FLAG_BIT_SET(COEX_SCHED_SCAN_INACTIVE_CB, gpBtCoexSchedDev->BtCoexSchedulerFlags)
#define SET_COEX_SCHED_SCAN_INACTIVE_CB \
    SET_FLAG_BIT(COEX_SCHED_SCAN_INACTIVE_CB, gpBtCoexSchedDev->BtCoexSchedulerFlags)
#define CLR_COEX_SCHED_SCAN_INACTIVE_CB \
    CLR_FLAG_BIT(COEX_SCHED_SCAN_INACTIVE_CB, gpBtCoexSchedDev->BtCoexSchedulerFlags)

#define COEX_SCHED_SCAN_ACTIVE_CB     (0x0000002)
#define IS_COEX_SCHED_SCAN_ACTIVE_CB  IS_FLAG_BIT_SET(COEX_SCHED_SCAN_ACTIVE_CB, gpBtCoexSchedDev->BtCoexSchedulerFlags)
#define SET_COEX_SCHED_SCAN_ACTIVE_CB SET_FLAG_BIT(COEX_SCHED_SCAN_ACTIVE_CB, gpBtCoexSchedDev->BtCoexSchedulerFlags)
#define CLR_COEX_SCHED_SCAN_ACTIVE_CB CLR_FLAG_BIT(COEX_SCHED_SCAN_ACTIVE_CB, gpBtCoexSchedDev->BtCoexSchedulerFlags)

#define COEX_SCHED_BTSCAN_PROT_ACTIVE_CB (0x0000004)
#define IS_COEX_SCHED_BTSCAN_PROT_ACTIVE_CB \
    IS_FLAG_BIT_SET(COEX_SCHED_BTSCAN_PROT_ACTIVE_CB, gpBtCoexSchedDev->BtCoexSchedulerFlags)
#define SET_COEX_SCHED_BTSCAN_PROT_ACTIVE_CB \
    SET_FLAG_BIT(COEX_SCHED_BTSCAN_PROT_ACTIVE_CB, gpBtCoexSchedDev->BtCoexSchedulerFlags)
#define CLR_COEX_SCHED_BTSCAN_PROT_ACTIVE_CB \
    CLR_FLAG_BIT(COEX_SCHED_BTSCAN_PROT_ACTIVE_CB, gpBtCoexSchedDev->BtCoexSchedulerFlags)

#define COEX_SCHED_SELFGEN_OVERRIDE (0x0000008)
#define IS_COEX_SCHED_SELFGEN_OVERRIDE \
    IS_FLAG_BIT_SET(COEX_SCHED_SELFGEN_OVERRIDE, gpBtCoexSchedDev->BtCoexSchedulerFlags)
#define SET_COEX_SCHED_SELFGEN_OVERRIDE \
    SET_FLAG_BIT(COEX_SCHED_SELFGEN_OVERRIDE, gpBtCoexSchedDev->BtCoexSchedulerFlags)
#define CLR_COEX_SCHED_SELFGEN_OVERRIDE \
    CLR_FLAG_BIT(COEX_SCHED_SELFGEN_OVERRIDE, gpBtCoexSchedDev->BtCoexSchedulerFlags)

#define COEX_SCHED_NONLINK_EXISTED (0x0000010)
#define IS_COEX_SCHED_NONLINK_EXISTED \
    IS_FLAG_BIT_SET(COEX_SCHED_NONLINK_EXISTED, gpBtCoexSchedDev->BtCoexSchedulerFlags)
#define SET_COEX_SCHED_NONLINK_EXISTED SET_FLAG_BIT(COEX_SCHED_NONLINK_EXISTED, gpBtCoexSchedDev->BtCoexSchedulerFlags)
#define CLR_COEX_SCHED_NONLINK_EXISTED CLR_FLAG_BIT(COEX_SCHED_NONLINK_EXISTED, gpBtCoexSchedDev->BtCoexSchedulerFlags)

#define BT_INTERVAL_COUNT   (gpBtCoexSchedDev->BtIntervalCnt)
#define WLAN_INTERVAL_COUNT (gpBtCoexSchedDev->WlanIntervalCnt)

#define BT_QUOTA_OVER \
    ((gpBtCoexSchedDev->BtIntervalCnt != 0) && (gpBtCoexSchedDev->BtIntervalCnt >= AllocatedBtIntervals))
#define BT_QUOTA_NOT_OVER \
    ((gpBtCoexSchedDev->BtIntervalCnt != 0) && (gpBtCoexSchedDev->BtIntervalCnt < AllocatedBtIntervals))

#define WLAN_QUOTA_OVER \
    ((gpBtCoexSchedDev->WlanIntervalCnt != 0) && (gpBtCoexSchedDev->WlanIntervalCnt >= AllocatedWlanIntervals))
#define WLAN_QUOTA_NOT_OVER \
    ((gpBtCoexSchedDev->WlanIntervalCnt != 0) && (gpBtCoexSchedDev->WlanIntervalCnt < AllocatedWlanIntervals))

#define RC_PPDU_MAX_TIME_LIMIT 4000
#define RC_PPDU_MIN_TIME_LIMIT 128

// Coex SM States
enum coex_states {
    COEX_SM_IDLE = 0,
    COEX_SM_BT_INTERVAL = 1,
    COEX_SM_WLAN_INTERVAL = 2,
    COEX_SM_WLAN_POSTPAUSE_INTERVAL = 3,
    COEX_SM_WLAN_CTS2S_INTERVAL = 4,
};

// CoexVdevSchedulerFlags
#define COEX_VDEV_CRIT_PROTO_HINT (0x0000001)
#define IS_COEX_VDEV_CRIT_PROTO_HINT(id) \
    ((id != INVALID_WLAN_VDEVID) &&      \
     IS_FLAG_BIT_SET(COEX_VDEV_CRIT_PROTO_HINT, gpBtCoexWlanInfoDev->pCoexVdev[id].VdevSchedulerFlags))
#define SET_COEX_VDEV_CRIT_PROTO_HINT(id)                                                               \
    if (id != INVALID_WLAN_VDEVID) {                                                                    \
        SET_FLAG_BIT(COEX_VDEV_CRIT_PROTO_HINT, gpBtCoexWlanInfoDev->pCoexVdev[id].VdevSchedulerFlags); \
    }
#define CLR_COEX_VDEV_CRIT_PROTO_HINT(id)                                                               \
    if (id != INVALID_WLAN_VDEVID) {                                                                    \
        CLR_FLAG_BIT(COEX_VDEV_CRIT_PROTO_HINT, gpBtCoexWlanInfoDev->pCoexVdev[id].VdevSchedulerFlags); \
    }

#define COEX_VDEV_WLAN_CONNECTING (0x0000002)
#define IS_COEX_VDEV_WLAN_CONNECTING(id) \
    ((id != INVALID_WLAN_VDEVID) &&      \
     IS_FLAG_BIT_SET(COEX_VDEV_WLAN_CONNECTING, gpBtCoexWlanInfoDev->pCoexVdev[id].VdevSchedulerFlags))
#define SET_COEX_VDEV_WLAN_CONNECTING(id)                                                               \
    if (id != INVALID_WLAN_VDEVID) {                                                                    \
        SET_FLAG_BIT(COEX_VDEV_WLAN_CONNECTING, gpBtCoexWlanInfoDev->pCoexVdev[id].VdevSchedulerFlags); \
    }
#define CLR_COEX_VDEV_WLAN_CONNECTING(id)                                                               \
    if (id != INVALID_WLAN_VDEVID) {                                                                    \
        CLR_FLAG_BIT(COEX_VDEV_WLAN_CONNECTING, gpBtCoexWlanInfoDev->pCoexVdev[id].VdevSchedulerFlags); \
    }

#define COEX_VDEV_WLAN_SCANNING (0x0000004)
#define IS_COEX_VDEV_WLAN_SCANNING(id) \
    ((id != INVALID_WLAN_VDEVID) &&    \
     IS_FLAG_BIT_SET(COEX_VDEV_WLAN_SCANNING, gpBtCoexWlanInfoDev->pCoexVdev[id].VdevSchedulerFlags))
#define SET_COEX_VDEV_WLAN_SCANNING(id)                                                               \
    if (id != INVALID_WLAN_VDEVID) {                                                                  \
        SET_FLAG_BIT(COEX_VDEV_WLAN_SCANNING, gpBtCoexWlanInfoDev->pCoexVdev[id].VdevSchedulerFlags); \
    }
#define CLR_COEX_VDEV_WLAN_SCANNING(id)                                                               \
    if (id != INVALID_WLAN_VDEVID) {                                                                  \
        CLR_FLAG_BIT(COEX_VDEV_WLAN_SCANNING, gpBtCoexWlanInfoDev->pCoexVdev[id].VdevSchedulerFlags); \
    }

#define COEX_VDEV_TWT_TEARDOWN (0x0000008)
#define IS_COEX_VDEV_TWT_TEARDOWN(id) \
    ((id != INVALID_WLAN_VDEVID) &&   \
     IS_FLAG_BIT_SET(COEX_VDEV_TWT_TEARDOWN, gpBtCoexWlanInfoDev->pCoexVdev[id].VdevSchedulerFlags))
#define SET_COEX_VDEV_TWT_TEARDOWN(id)                                                               \
    if (id != INVALID_WLAN_VDEVID) {                                                                 \
        SET_FLAG_BIT(COEX_VDEV_TWT_TEARDOWN, gpBtCoexWlanInfoDev->pCoexVdev[id].VdevSchedulerFlags); \
    }
#define CLR_COEX_VDEV_TWT_TEARDOWN(id)                                                               \
    if (id != INVALID_WLAN_VDEVID) {                                                                 \
        CLR_FLAG_BIT(COEX_VDEV_TWT_TEARDOWN, gpBtCoexWlanInfoDev->pCoexVdev[id].VdevSchedulerFlags); \
    }

#define IS_COEX_SCHED_CRIT_SCAN_MGMT_FRM \
    IS_FLAG_BIT_SET(COEX_SCHED_CRIT_SCAN_MGMT_FRM, gpBtCoexWlanInfoDev->CoexSchedulerFlags)
#define SET_COEX_SCHED_CRIT_SCAN_MGMT_FRM \
    SET_FLAG_BIT(COEX_SCHED_CRIT_SCAN_MGMT_FRM, gpBtCoexWlanInfoDev->CoexSchedulerFlags)
#define CLR_COEX_SCHED_CRIT_SCAN_MGMT_FRM \
    (gpBtCoexWlanInfoDev && CLR_FLAG_BIT(COEX_SCHED_CRIT_SCAN_MGMT_FRM, gpBtCoexWlanInfoDev->CoexSchedulerFlags))

#define IS_COEX_SCHED_CRIT_ASSOC \
    (gpBtCoexWlanInfoDev && IS_FLAG_BIT_SET(COEX_SCHED_CRIT_ASSOC, gpBtCoexWlanInfoDev->CoexSchedulerFlags))
#define SET_COEX_SCHED_CRIT_ASSOC                                                     \
    if (gpBtCoexWlanInfoDev) {                                                        \
        SET_FLAG_BIT(COEX_SCHED_CRIT_ASSOC, gpBtCoexWlanInfoDev->CoexSchedulerFlags); \
    }
#define CLR_COEX_SCHED_CRIT_ASSOC                                                     \
    if (gpBtCoexWlanInfoDev) {                                                        \
        CLR_FLAG_BIT(COEX_SCHED_CRIT_ASSOC, gpBtCoexWlanInfoDev->CoexSchedulerFlags); \
    }

#define IS_COEX_SCHED_PRIO_WLAN_CONN \
    (gpBtCoexWlanInfoDev && IS_FLAG_BIT_SET(COEX_SCHED_PRIO_WLAN_CONN, gpBtCoexWlanInfoDev->CoexSchedulerFlags))

#define IS_COEX_SCHED_INCREASE_RX_LVL1 \
    IS_FLAG_BIT_SET(COEX_SCHED_INCREASE_RX_LVL1, gpBtCoexWlanInfoDev->CoexSchedulerFlags)

/*-------------------------------------------------------------------------
 * Type Declarations
 * ----------------------------------------------------------------------*/
typedef struct btcoex_sched_t {
    /* MCI, GPM */
    uint32_t MCIConfig;
    uint32_t NumBtGpmOverflow;
    uint16_t PrevGPMIndex;
    uint8_t MCIQueryAtAwake;
    uint8_t PauseState;

    /* Scheduling: time */
    uint32_t BtIntervalAllocation; /* BT traffic pattern period in us */
    uint32_t CoexInterval;
    uint16_t BtIntervalCnt;
    uint16_t WlanIntervalCnt;
    uint32_t wlan_duration;
    uint32_t bt_duration;
    uint32_t BtPmLatency;

    /* Scheduling: Flags, States, Params */
    uint32_t BtCoexSchedulerFlags; /* BT sched flags */
    uint8_t BtCoexSMState;         /* State machine */
    uint8_t BtLowPrioBudget;       /* BT duty cycle in percentage */
    uint8_t BtHiPrioBudget;        /* BT duty cycle in percentage */
    uint8_t BtStateChangeReason;
    uint8_t wlan_conn_bt_high_bw;
    uint8_t wlan_conn_mgmt_bt_high_bw;
    uint8_t IsHeavyTraffic; /* Flag to indicate heavy/light traffic */
    uint32_t BtCoexLastIntervalDuration;
    uint32_t BtCoexWlanAggrLimit;

    /* For flexible FTP/PAN sche */
    uint32_t PreviousBtACLTxBW;  /* Reserve, BT TX for possible bi-direction traffic */
    uint32_t PreviousBtACLRxBW;  /* Reserve, BT RX for possible bi-direction traffic */
    uint32_t CumulativeInterval; /* Total past BT interval length */
} BTCOEX_SCHED_STRUCT;

/*-------------------------------------------------------------------------
 * Function Declarations and Documentation
 * ----------------------------------------------------------------------*/
void coex_schedule(uint8_t scheduler_req, uint32_t duration);
void coex_stop_prio_timer(void);
void coex_start_prio_timer(uint32_t Duration);
void coex_sched_reset();
void coex_pause_interval(void);
void coex_start_bt_interval(uint32_t Duration, uint32_t Weight, uint8_t EnableBtBwLimit);
void coex_start_wlan_cts2s_interval(uint32_t Duration, uint32_t Weight);
void coex_start_wlan_interval(uint32_t Duration, uint32_t Weight);
void coex_set_rc_time_limit(uint32_t value);
void coex_print_rc_time_limit();
void coex_get_bt_traffic_status(uint32_t Percentage, uint32_t IntervalDuration, uint8_t *TrafficStatus);
uint8_t coex_nonlink_extra_intervals(void);
#endif /* SUPPORT_COEX */
#endif /* #ifndef  COEX_SCHED_H */
