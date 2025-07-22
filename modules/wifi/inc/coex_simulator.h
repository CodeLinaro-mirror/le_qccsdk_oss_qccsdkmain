/*========================================================================
Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
*
* @file coex_bt.h
* @brief Coex bt params and struct definitions
*========================================================================*/
#ifndef _COEX_SIMULATOR_H_
#define _COEX_SIMULATOR_H_
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
#if defined(SUPPORT_COEX_SIMULATOR)
#include "coex_gpm.h"
/*
 * This structure hold the general data structures that holds critical
 * state information. It also holds different management entities required for the
 * operation of the Coex Simulator.
 */
typedef struct simulator {
    tMciGpm gpm;
    uint8_t is_bton;
} COEX_SIM_SIMULATOR;

extern COEX_SIM_SIMULATOR g_sim;

uint8_t coex_sim_gpm_create(uint32_t *args, uint8_t num_args);
uint8_t coex_sim_get_bt_state();
void coex_sim_set_bt_state(uint8_t bt_on);

#endif /*SUPPORT_COEX_SIMULATOR */
#endif /* SUPPORT_COEX */
#endif /* #ifndef _COEX_SIMULATOR_H_*/
