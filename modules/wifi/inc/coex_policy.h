/*========================================================================
Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
*
* @file coex_policy.h
* @brief Coex policy related definitions
*========================================================================*/
#ifndef COEX_POLICY_H
#define COEX_POLICY_H

/*------------------------------------------------------------------------
 * Include Files
 *----------------------------------------------------------------------*/

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "nt_osal.h"

#include "fwconfig_wlan.h"
#include "nt_flags.h"
#include "nt_common.h"

#if defined(SUPPORT_COEX)
#include "coex_wlan.h"
/*-------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * ----------------------------------------------------------------------*/

#define COEX_CURRENT_POLICY (gp_coex_algo->current_policy)

#define COEX_GET_CONTEXT(policy) (gp_coex_algo->policy_table[policy].context)

#define COEX_UPDATE_PREVIOUS_POLICY(policy)     \
    do {                                        \
        gp_coex_algo->previous_policy = policy; \
    } while (0)

#define COEX_UPDATE_CURRENT_POLICY(policy)     \
    do {                                       \
        gp_coex_algo->current_policy = policy; \
    } while (0)

#define COEX_INIT_POLICY                                    \
    do {                                                    \
        gp_coex_algo->previous_policy = COEX_TRF_MGMT_NONE; \
        gp_coex_algo->current_policy = COEX_TRF_MGMT_NONE;  \
    } while (0)

#define COEX_ADD_POLICY(policy_tbl, policy, evnt_mgr, prepare_api, ctx) \
    do {                                                                \
        if (policy >= COEX_TRF_MGMT_NUM_POLICIES) {                     \
            NT_LOG_COEX_ERR("Co-Ex Policy Add Error !", 0, 0, 0);       \
            break;                                                      \
        }                                                               \
        policy_tbl[policy].event_mgr = evnt_mgr;                        \
        policy_tbl[policy].prepare = prepare_api;                       \
        policy_tbl[policy].context = ctx;                               \
    } while (0)

#define COEX_CALL_PREPARE_API(mac, sched_req, policy, interval, wlan_wt, bt_wt) \
    (gp_coex_algo->policy_table[policy].prepare(mac, sched_req, policy, interval, wlan_wt, bt_wt))

#define COEX_ALGO_MANAGER(event)                                             \
    gp_coex_algo && (COEX_CURRENT_POLICY < COEX_TRF_MGMT_NUM_POLICIES)       \
        ? (gp_coex_algo->policy_table[COEX_CURRENT_POLICY].event_mgr(event)) \
        : (void)event

#define COEX_GET_POLICY_TABLE (gp_coex_algo->policy_table)

#define COEX_UPDATE_POLICY_MAP(policy, shift) (gp_coex_algo->policy_map |= (policy << shift))

#define COEX_GET_POLICY_MAP (gp_coex_algo->policy_map)

enum coex_policy {
    COEX_TRF_MGMT_FREERUN = 0,
    COEX_TRF_MGMT_SHAPE_STATIC_PM,
    COEX_TRF_MGMT_NUM_POLICIES,
    COEX_TRF_MGMT_NONE = 0xFF
};

enum coex_traffic_shaping_type {
    COEX_TRAFFIC_SHAPING_NONE = 0,
    COEX_TRAFFIC_SHAPING_STATIC = 1,
    COEX_TRAFFIC_SHAPING_DYNAMIC = 2,
    COEX_TRAFFIC_SHAPING_DYNAMIC_SYNC = 3,
    COEX_TRAFFIC_SHAPING_LOW_LATENCY = 4,
};

enum event_bitmap {
    COEX_POLICY_EVENT_ENTRY = 1,
    COEX_POLICY_EVENT_EXIT = 2,
    COEX_POLICY_EVENT_BT_START = 3,
    COEX_POLICY_EVENT_BT_END = 4,
    COEX_POLICY_EVENT_RECOMPUTE_SCHEDULE = 5,
};

/*-------------------------------------------------------------------------
 * Type Declarations
 * ----------------------------------------------------------------------*/

typedef void (*coex_event_mgr)(uint32_t event);

typedef void (*coex_prepare)(coex_mac *mac, uint8_t sched_req, uint8_t algo, uint8_t *next_interval,
                             uint32_t *wlan_weight, uint32_t *bt_weight);

typedef struct coex_policy_table_s {
    coex_event_mgr event_mgr;
    coex_prepare prepare;
    void *context;
} coex_policy_table_t;

typedef struct coex_algo_s {
    uint8_t current_policy;
    uint8_t previous_policy;

    /* Used only for debug and
       unit testing purposes */
    uint32_t policy_map;

    coex_policy_table_t policy_table[COEX_TRF_MGMT_NUM_POLICIES];

} BTCOEX_ALGO_STRUCT;

/*-------------------------------------------------------------------------
 * Function Declarations
 * ----------------------------------------------------------------------*/

void coex_algo_module_init();
uint8_t coex_get_trafficmgmt_algorithm(coex_mac *mac);
void coex_policy_change(uint8_t policy);

#endif /* SUPPORT_COEX */
#endif /* #ifndef  COEX_POLICY_H */
