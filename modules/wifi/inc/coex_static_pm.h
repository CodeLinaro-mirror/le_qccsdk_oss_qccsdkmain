/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*========================================================================
 *
 * @file coex_static_pm.h
 * @brief Coex Static PM Policy Header File
 *========================================================================*/

#ifndef COEX_STATIC_PM_H
#define COEX_STATIC_PM_H

/*-------------------------------------------------------------------------
 * Include Files
 * ----------------------------------------------------------------------*/

#include <stdint.h>

#ifdef SUPPORT_COEX

/*-------------------------------------------------------------------------
 * Type Declarations
 * ----------------------------------------------------------------------*/

typedef struct coex_pm_ctx_s {
    uint32_t event_bitmap;
    uint32_t chan_req_duration;
    uint32_t chan_req_interval;
} coex_pm_ctx_t;

/*-------------------------------------------------------------------------
 * Function Declarations
 * ----------------------------------------------------------------------*/

void coex_init_pm_ctx(coex_pm_ctx_t *context);
void coex_pm_mgr(uint32_t event);
void coex_pm_prepare(coex_mac *mac, uint8_t sched_req, uint8_t algo, uint8_t *next_interval, uint32_t *wlan_weight,
                     uint32_t *bt_weight);

#endif /* SUPPORT_COEX */
#endif /* COEX_STATIC_PM_H */
