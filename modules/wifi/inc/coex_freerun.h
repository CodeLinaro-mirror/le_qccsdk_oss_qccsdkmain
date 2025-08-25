/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*========================================================================
 *
 * @file coex_freerun.h
 * @brief Coex Freerun Policy Header File
 *========================================================================*/

#ifndef COEX_FREERUN_H
#define COEX_FREERUN_H

/*-------------------------------------------------------------------------
 * Include Files
 * ----------------------------------------------------------------------*/

#include <stdint.h>

#ifdef SUPPORT_COEX
/*-------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * ----------------------------------------------------------------------*/
#define COEX_UPDATE_NI_MAP(next_int, shift) (next_interval_map |= (next_int << shift))

#define COEX_SAVE_NI_MAP(policy, map) (((coex_freerun_ctx_t *)COEX_GET_CONTEXT(policy))->next_interval_map = map)

#define COEX_GET_NI_MAP(policy) (((coex_freerun_ctx_t *)COEX_GET_CONTEXT(policy))->next_interval_map)

#define IS_XPAN_CONN_IN_PROGRESS(coex_mac) \
    ((coex_mac->state & COEX_WLAN_SYS_CONNECTING) | (coex_mac->state & COEX_WLAN_SYS_CRIT_PROTO_HINT))

/*-------------------------------------------------------------------------
 * Type Declarations
 * ----------------------------------------------------------------------*/

typedef struct coex_freerun_ctx_s {
    uint32_t event_bitmap;

    /* Used only for debug and
       unit testing purposes */
    uint32_t next_interval_map;
} coex_freerun_ctx_t;

/*-------------------------------------------------------------------------
 * Function Declarations
 * ----------------------------------------------------------------------*/

void coex_init_freerun_ctx(coex_freerun_ctx_t *context);
void coex_freerun_mgr(uint32_t event);
void coex_freerun_prepare(coex_mac *mac, uint8_t sched_req, uint8_t algo, uint8_t *next_interval, uint32_t *wlan_weight,
                          uint32_t *bt_weight);

#endif /* SUPPORT_COEX */
#endif /* COEX_FREERUN_H */
