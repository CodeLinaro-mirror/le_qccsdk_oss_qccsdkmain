/*========================================================================
Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
*
* @brief Coex Power function definitions
*========================================================================*/

#ifndef COEX_POWER_H
#define COEX_POWER_H

#include "nt_osal.h"

#include "fwconfig_wlan.h"
#include "nt_flags.h"

#if defined(SUPPORT_COEX)
#include "coex_main.h"
#include "coex_bt.h"
#include "coex_wlan.h"
#include "coex_sched.h"
#include "coex_utils.h"
#include "coex_version.h"
#include "coex_iso.h"
#include "nt_logger_api.h"
#include "assert.h"
#include "coex_policy.h"
#include "pm_api.h"

#define PS_CALLBACK_COEX_PRIORITY 10

typedef enum {
    COEX_PS_VOTE_SLEEP = 0,
    COEX_PS_VOTE_WAKE = 1,
    COEX_PS_VOTE_UNKOWN = 255,
} COEX_PS_VOTE;

typedef enum coex_pwr_vote_src_t {
    COEX_PWR_VOTE_SRC_RX_UNDER_THRESH = 1,
    COEX_PWR_VOTE_SRC_PSP = 2,
    COEX_PWR_VOTE_SRC_ALGO = 3,
    COEX_PWR_VOTE_SRC_FORCEWAKE_CONN = 6,
    COEX_PWR_VOTE_SRC_WSI_HANDLER = 8,
    COEX_PWR_VOTE_SRC_CHREQ = 9,
    COEX_PWR_VOTE_SRC_FORCE_SLEEP = 10,  // SLEEP requests only
    COEX_PWR_VOTE_SRC_MAX,               /* LAST ENTRY */
} E_COEX_PWR_VOTE_SRC;

typedef enum coex_sleep_reason_t {
    COEX_SLEEP_REASON_NONE = 0,
    COEX_SLEEP_REASON_FORCEWAKE_TMR = 1,
    COEX_SLEEP_REASON_BTOFF = 2,
    COEX_SLEEP_REASON_FORCEWAKE_RX_UNDER_THRESH = 4,
    COEX_SLEEP_REASON_SCAN = 5,
    COEX_SLEEP_REASON_DEVOP_BT_REQ = 8,
    COEX_SLEEP_REASON_DEVOP_BT_REQ_STOP = 9,
    COEX_SLEEP_REASON_BT_COEX_IDLE_NOT_REQD = 16,
    COEX_SLEEP_REASON_FORCE_WAKE_FOR_CONN = 17,
    COEX_SLEEP_REASON_WSI_PWR_UP = 21,
    COEX_SLEEP_REASON_CHAN_REQ_GRANT = 22,
    COEX_SLEEP_REASON_CHAN_REQ_STOPPED = 23,
} E_COEX_SLEEP_REASON;

typedef enum coex_pwr_state {
    COEX_PWR_STATE_SLEEP = 0,
    COEX_PWR_STATE_AWAKE = 1,
    COEX_PWR_STATE_UNKNOWN = 0xFF,
} E_COEX_PWR_STATE;

typedef enum wal_coex_pwr_event_type {
    COEX_PWR_EVENT_TYPE_PRE = 0,
    COEX_PWR_EVENT_TYPE_POST = 1,
    COEX_PWR_EVENT_TYPE_MAX
} E_COEX_PWR_EVENT_TYPE;

typedef struct _coex_power_state {
    E_COEX_PWR_STATE state;                            /* Latest power state per mac. */
    E_COEX_PWR_VOTE_SRC src;                           /* The src that voted for the latest power state. */
    E_COEX_SLEEP_REASON reason[COEX_PWR_VOTE_SRC_MAX]; /* The reason code associated with the latest power state. */
    E_COEX_PWR_VOTE_SRC lastwake_src;                  /* The src that voted for the latest power state. */
    E_COEX_SLEEP_REASON lastwake_reason;               /*Last wake reason */
    uint64_t ts;                                       /* Time at which the last request is sent to the power module. */
    uint32_t vote_bitmap;                              /* Store the votes in the bitmap. */
    uint32_t sleep_vote_bt_int;       /* Counter to count the sleep votes done during BT interval- Debug use only. */
    uint16_t bitmap_reset_fail_count; /* Number of times bitmap reset fail due to CAL */
    uint16_t ps_req_fail_count;       /* PS request failure count */
} coex_power_state;

typedef struct _coex_power_debug_entry {
    uint64_t ts;
    uint32_t event;
    PM_POWER_MODE state;
    E_COEX_PWR_STATE pre_post_state;
} coex_power_debug_entry;

#define COEX_PWR_DBG_NUM_ENTRY 16
typedef struct _coex_power_debug {
    uint8_t event_indx;
    coex_power_debug_entry info[COEX_PWR_DBG_NUM_ENTRY];
} coex_power_debug;

typedef struct _coex_ps_struct {
    coex_power_state *cur_pwr_state;
    coex_power_debug dbg;
} COEX_PS_STRUCT;

#ifndef FEATURE_FPCI
void coex_power_change_cb(uint8_t wlan_id, uint8_t EventType, uint8_t NewPowerState);
#else
void coex_power_change_cb(uint8_t evt, void *p_args);
#endif
uint8_t coex_wlan_sleep_enable(COEX_PS_VOTE vote, uint8_t mac_id, E_COEX_PWR_VOTE_SRC src, E_COEX_SLEEP_REASON reason);
#endif /*ifndef COEX_POWER_H */
#endif /*SUPPORT_COEX*/
