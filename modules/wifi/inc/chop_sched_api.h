/* 
Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 * @file chop_sched_api.h
 * @brief Channel manager Declarations.
 *
 * A module desiring to perform an operation on a specific channel
 * (off the home channel) submits this request to the channel manager.
 * The channel manager will call the start callback when it has changed
 * to the requested channel so the module can perform the desired function.
 *
 * The channel manager will call the finish callback when the operation is
 * required to finish. The finish callback is normally called at the timeout
 * specified on the requested op, but it might be called earlier if the
 * channel manager needs to return to the home channel.
 */

#ifndef _CHOP_SCHED_API_H_
#define _CHOP_SCHED_API_H_

#include "fwconfig_wlan.h"
#include "nt_flags.h"
#if defined(CONFIG_CHANNEL_SCHEDULER)
/*-------------------------------------------------------------------------
 * Include Files
 * ------------------------------------------------------------------------
 */

#include <pm_api.h>

/*-------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * ------------------------------------------------------------------------
 */
#define MCC_TXTX_ACTIVITY_THRESHOLD 1000
#define MCC_TXTX_DROP_THOLD 20
#define ROUTER_MAX_DEV 2

#define CHMGR_INFINITE_DURATION 0        /* infinite duration for new_op */
#define CO_PAUSETX_TIMEOUT 50            /* milliseconds */
#define CO_PAUSETX_BT_TIMEOUT 5          /* milliseconds, drain time before BT device */
#define CO_PM_TIMEOUT 5                  /* milliseconds, drain time before BT device */
#define CO_MULTI_HOME_PAUSETX_TIMEOUT 1  /* milliseconds */
#define CO_UPDATE_TIME_THRESHOLD 1024000 /*microseconds, the threshold to update start_time of co*/

//#define DURATION_DEV_START      1000    /* 1 second */
#define DURATION_DEV_START 1000000 /* 1 second */

#define DURATION_BEACON_RESYNC 1000000 /* 1 second */
#define DURATION_LOW_PRIO 102400       /* 20 TU */
#define DURATION_HIGH_PRIO 20480       /* 20 TU */
#define DURATION_NORMAL 15360          /* 15 TU */

#define DELAY_BEFORE_TBTT                                                                                              \
    (1024 * 17) /*why 17 TU: tx_drain_ahead (2) + wait tx drain (5)                                                    \
                 * + fake sleep(3) + wait ap drain(2) + change                                                         \
                 * channel(1) + swba ahead(4)                                                                          \
                 */

#define CTS_MAX_DURATION 32000
typedef enum {
    CO_SHARE_ALL_LOW = 1,
    CO_LONG_WLAN_OPERATION = 2,
} CO_BTC_POLICY;

/* Forward declarations */
struct channel;

typedef enum {
    CO_PAUSETX_TIMEOUT_ID = 0,
} CO_CONFIG_ID;

typedef enum {
    CO_GET_NULL_FRAME_ERROR = -1,
    CO_DONOT_IGNORE_NULL_FRAME_ERROR = 0,
    CO_IGNORE_NULL_FRAME_ERROR = 1,
} CO_CHECK_NULL_FRAME_ERROR_TYPE;

/*-------------------------------------------------------------------------
 * Type Declarations
 * ------------------------------------------------------------------------
 */

/*
 * Device channel operation types. Each device can schedule
 * 1. One offchannel operation like scan, RM, etc.
 * 2. One high priority home channel operation
 * 3. One low priority home channel operation
 */

typedef enum co_dev_op_type {
    CO_DEV_OP_TYPE_BT = 0,
    CO_DEV_OP_TYPE_HOME_CHANNEL_HIGH = 1,
    CO_DEV_OP_TYPE_OFF_CHANNEL = 2,
    CO_DEV_OP_TYPE_HOME_CHANNEL_LOW = 3,
    CO_DEV_OP_TYPE_MAX = 4,
} CO_DEV_OP_TYPE;

typedef enum co_dev_op_flags {
    CO_DEV_OP_FLAG_PREEMPT_HOME_CHANNEL = 0x1, /* off channel can preempt home channel */
    CO_DEV_OP_FLAG_DEV_START = 0x2,            /* device start operation */
    CO_DEV_OP_FLAG_RELAX_START_TIME = 0x4,     /* relax start time requirement for this periodic req */
    CO_DEV_OP_FLAG_FORCE_SCHEDULER_ON = 0x8,   /* force the scheduler to be always running */
    CO_DEV_OP_FLAG_DONOT_FORCE_DRAIN = 0x10,   /* donot force drain txq, wait until all frames in hw txq sent */
} CO_DEV_OP_FLAGS;
/*
 * Device channel operation basic configuration
 */
typedef struct co_dev_op {
    channel_t *co_channel;
    uint32_t co_duration; /* usec */
    void *co_arg;
    CSERV_COMPLETION_CB co_start_cb;
    CSERV_COMPLETION_CB co_finish_cb;
    uint8_t co_extmode;
    uint8_t co_dev_op_type;
    uint16_t co_flags;
    uint32_t co_start_time;
} CO_DEV_OP;

/*
 * Device channel operation priority levels
 */
typedef enum co_priority_level {
    CO_PRIORITY_LEVEL_0 = 0,
    CO_PRIORITY_LEVEL_1 = 1,
    CO_PRIORITY_LEVEL_2 = 2,
    CO_PRIORITY_LEVEL_3 = 3,
    CO_PRIORITY_LEVEL_4 = 4,
    CO_PRIORITY_LEVEL_5 = 5,
    CO_PRIORITY_LEVEL_6 = 6,
    CO_PRIORITY_LEVEL_MAX = 7
} CO_PRIORITY_LEVEL;

typedef enum co_priority {
    CO_HOME_CHANNEL_LOW_PRIORITY = CO_PRIORITY_LEVEL_1,
    CO_BACKGROUND_SEARCH_PRIORITY = CO_PRIORITY_LEVEL_2,
    CO_CONTINUE_SCAN_PRIORITY = CO_PRIORITY_LEVEL_2,
    CO_WREG_SEARCH_PRIORITY = CO_PRIORITY_LEVEL_3,
    CO_P2P_SCAN_LO_PRIORITY = CO_PRIORITY_LEVEL_3,
    CO_HOST_INITIATED_SEARCH_LO_PRIORITY = CO_PRIORITY_LEVEL_3,
#define CO_HOST_INITIATED_SEARCH_PRIORITY CO_HOST_INITIATED_SEARCH_LO_PRIORITY
    CO_NETWORK_LIST_OFFLOAD_SCAN_PRIORITY = CO_PRIORITY_LEVEL_3,
    CO_HOST_INITIATED_SEARCH_HI_PRIORITY = CO_PRIORITY_LEVEL_4,
    CO_FOREGROUND_SEARCH_PRIORITY = CO_PRIORITY_LEVEL_4,
    CO_INITIATE_HANDOFF_PRIORITY = CO_PRIORITY_LEVEL_4,
    CO_ACS_SEARCH_PRIORITY = CO_PRIORITY_LEVEL_4,
    CO_RM_PRIORITY = CO_PRIORITY_LEVEL_6,
    CO_P2P_GROUP_FRM_PRIORITY = CO_PRIORITY_LEVEL_4,
    CO_P2P_SCAN_HI_PRIORITY = CO_PRIORITY_LEVEL_4,
    CO_RO_LOW_RSSI_SCAN_PRIORITY = CO_PRIORITY_LEVEL_4,
    CO_HOME_CHANNEL_HIGH_L2_PRIORITY = CO_PRIORITY_LEVEL_4,
    //#define CO_HOME_CHANNEL_HIGH_PRIORITY       CO_HOME_CHANNEL_HIGH_L2_PRIORITY
    CO_HOME_CHANNEL_HIGH_L1_PRIORITY = CO_PRIORITY_LEVEL_5,
#define CO_HOME_CHANNEL_HIGH_PRIORITY CO_HOME_CHANNEL_HIGH_L1_PRIORITY
    CO_CM_REFRESH_ROAM_TABLE_PRIORITY = CO_PRIORITY_LEVEL_6,
    CO_HOME_CHANNEL_CONNECT_REQUEST_PRIORITY = CO_PRIORITY_LEVEL_6,
    CO_HOME_CHANNEL_START_BSS_PRIORITY = CO_PRIORITY_LEVEL_6,
    CO_HOME_CHANNEL_BEACON_RESYNC_PRIORITY = CO_PRIORITY_LEVEL_6,
    CO_BT_CHANNEL_PRIORITY = CO_PRIORITY_LEVEL_6,
    CO_SET_CHANNEL_CMD_PRIORITY = CO_PRIORITY_LEVEL_3,
} CO_PRIORITY;

/* Off channel lock id for tracking purpose */
typedef enum {
    OC_LOCK_ID_CO_CMD = 3,
    OC_LOCK_ID_DC_SCAN = 4,
    OC_LOCK_ID_P2P_LISTEN = 5,
    OC_LOCK_ID_ROAM_SCAN = 6,
} OC_LOCK_ID;

typedef enum { CHANNEL_SCHED_RECOMPUTE_SCHEDULE = 0 } CHANNEL_SCHED_EVENT;
/*-------------------------------------------------------------------------
 * Function Declarations and Documentation
 * ------------------------------------------------------------------------
 */

LOCAL void *co_init();
void co_deinit(devh_t *dev);
LOCAL nt_status_t co_acquire_offchannel_lock(devh_t *dev, uint32_t priority, void (*cb)(void *arg, nt_status_t status),
                                             void *arg, OC_LOCK_ID lock_id);
LOCAL void co_release_offchannel_lock(devh_t *dev, OC_LOCK_ID lock_id);
LOCAL nt_status_t co_start_offchannel_op(devh_t *dev, CO_DEV_OP *dev_op);
LOCAL nt_status_t co_start_homechannel_op(devh_t *dev, CO_DEV_OP_TYPE dev_op_type, CO_DEV_OP *dev_op, uint32_t priority,
                                          uint32_t periodicity, uint32_t start_time);
LOCAL nt_status_t co_start_bt_op(devh_t *dev, CO_DEV_OP *dev_op, uint32_t periodicity, uint32_t start_time);
LOCAL void co_finish_offchannel_op(devh_t *dev);
LOCAL void co_finish_homechannel_op(devh_t *dev, CO_DEV_OP_TYPE dev_op_type);
LOCAL void co_finish_bt_op(devh_t *dev);
LOCAL void co_cancel_offchannel_op_with_status(devh_t *dev, nt_status_t status);
LOCAL void co_cancel_offchannel_op(devh_t *dev);
LOCAL void co_cancel_homechannel_op(devh_t *dev, CO_DEV_OP_TYPE dev_op_type);
LOCAL void co_cancel_bt_op(devh_t *dev);
LOCAL struct channel *co_get_current_channel(devh_t *dev);
LOCAL NT_BOOL co_check_null_frame_error(devh_t *dev, CO_CHECK_NULL_FRAME_ERROR_TYPE check);
LOCAL void co_abort_scan(devh_t *dev);
LOCAL void co_set_channel_cmd(devh_t *dev, WMI_SET_CHANNEL_CMD *pCmd);
LOCAL void co_dev_down(devh_t *dev);
LOCAL void co_pause_homechannel_op(devh_t *dev, CO_DEV_OP_TYPE dev_op_type, NT_BOOL pause);
LOCAL uint32_t co_active_dev_no(void);
LOCAL NT_BOOL co_is_fake_sleep_in_progress(devh_t *dev);
LOCAL uint32_t co_is_fake_sleep_in_progress_any_dev(void);
LOCAL void co_keep_scheduler_awake(NT_BOOL enable);
LOCAL void co_dev_bmiss_set(devh_t *dev);
LOCAL NT_BOOL co_dev_bmiss_get(devh_t *dev);
NT_BOOL co_tx_force_drain(uint32_t devid);
void co_op_dwell_expiry_cb_fn(nt_osal_timer_handle_t thandle);
LOCAL void co_finish_op_timeout_msg(nt_osal_timer_handle_t thandle);
void co_scheduler_expiry_cb_fn(nt_osal_timer_handle_t thandle);
LOCAL void co_scheduler_timeout_msg(nt_osal_timer_handle_t thandle);
LOCAL void co_pm_event_timeout_msg(nt_osal_timer_handle_t thandle);
void co_pm_event_expiry_cb_fn(nt_osal_timer_handle_t thandle);
void co_fake_sleep_timeout_msg(nt_osal_timer_handle_t thandle);
void co_fake_sleep_expiry_cb_fn(nt_osal_timer_handle_t thandle);
void co_wait_for_cts_timeout_msg(nt_osal_timer_handle_t thandle);
void co_wait_for_cts_expir_cb_fn(nt_osal_timer_handle_t thandle);
LOCAL nt_status_t co_update_bt_op(devh_t *dev, uint32_t periodicity, uint32_t start_time);
#endif /* CONFIG_CHANNEL_SCHEDULER */

#endif /* _CHOP_SCHED_API_H_ */
