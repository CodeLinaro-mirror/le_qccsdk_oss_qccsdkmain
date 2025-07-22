/*========================================================================
Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
*
* @file coex_devop.h
* @brief Coex devop params and struct definitions
*========================================================================*/
#ifndef _COEX_DEV_OP_H_
#define _COEX_DEV_OP_H_
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
#include "coex_api.h"
#include "coex_gpm.h"
#include "coex_mci.h"
#include "coex_sched.h"
typedef struct COEX_DEVOP_CTXT {
    uint8_t coex_devop_flags;

} COEX_DEVOP_CTXT;

typedef enum {
    COEX_DEVOP_START = 0,
    COEX_DEVOP_STOP = 1,
    COEX_DEVOP_UPDATE = 2,
    COEX_DEVOP_RESERVE = 3,
    COEX_DEVOP_POSTPONE = 4,
    COEX_DEVOP_EXTEND = 5,
} COEX_DEVOP_REQ;

void coex_channel_sched_cb(uint8_t channel_sched_event);
void coex_devop_mgr(uint32_t stimulus, uint32_t duration, uint32_t period, uint32_t start_time);
#endif /*SUPPORT_COEX */
#endif /*_COEX_DEV_OP_H_*/
