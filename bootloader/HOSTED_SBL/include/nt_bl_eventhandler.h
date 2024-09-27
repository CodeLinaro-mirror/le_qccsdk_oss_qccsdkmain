/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#ifndef NEUTRINO_PBL_SYSTEM_NT_BL_EVENTHANDLER_H_
#define NEUTRINO_PBL_SYSTEM_NT_BL_EVENTHANDLER_H_

#include <stdint.h>
#include "nt_bl_common.h"

void jump_to_sbl(uint32_t e_entry, void* arg);
uint32_t pbl_running_check();


typedef void (*jump_to_sbl_t) (uint32_t e_entry, void* arg);

typedef struct {
	jump_to_sbl_t jump_to_sbl_pfn;
}evt_hdlr_ind_t;

extern uint32_t _ln_PBL_stack__;

#ifndef SBL_BUILD
#define PBL_STACK_ADDR ((uint32_t)&_ln_PBL_stack__)
#else
#define PBL_STACK_ADDR 0xa000
#endif

#endif /* NEUTRINO_PBL_SYSTEM_NT_BL_EVENTHANDLER_H_ */
