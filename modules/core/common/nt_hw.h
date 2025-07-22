/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _NT_HW_H_
#define _NT_HW_H_

#if defined(PLATFORM_NT)
#include "nt_hw_reg.h"
#elif defined(PLATFORM_FERMION)
#include "fermion_hw_reg.h"
#else
#error "PLATFORM is not defined"
#endif

/*---------------------------------------------------------------------------
 * Timer
 *-------------------------------------------------------------------------*/

#define QWLAN_MTU_TIMER_REG_N(index) (QWLAN_MTU_TIMER_0_REG + (index)*4)

#define QWLAN_MTU_SW_MATCH_REGISTER_REG(index) (QWLAN_MTU_SW_MATCH_REGISTER_0_REG + (index)*4)

/* Timer control register */
#define QWLAN_MTU_TIMER_CONTROL_DIRECTION_DOWN 0
#define QWLAN_MTU_TIMER_CONTROL_DIRECTION_UP   1

#define QWLAN_MTU_TIMER_CONTROL_UNIT_STOP 0
#define QWLAN_MTU_TIMER_CONTROL_UNIT_CLK  1
#define QWLAN_MTU_TIMER_CONTROL_UNIT_USEC 2
#define QWLAN_MTU_TIMER_CONTROL_UNIT_USER 3

#define QWLAN_MTU_TIMER_CONTROL_UNIT_N(index, value) \
    ((uint32)(value) << (QWLAN_MTU_TIMER_CONTROL_SW_MTU_BASIC_UNIT_SELECT_OFFSET + (index)*2))
#define QWLAN_MTU_TIMER_CONTROL_DIRECTION_N(index, value) \
    ((uint32)(value) << (QWLAN_MTU_TIMER_CONTROL_SW_MTU_TIMER_UP_DOWN_CNTRL_OFFSET + (index)*1))
#define QWLAN_MTU_TIMER_CONTROL_MASK_N(index) \
    (QWLAN_MTU_TIMER_CONTROL_UNIT_N(index, 3) | QWLAN_MTU_TIMER_CONTROL_DIRECTION_N(index, 1))

#define QWLAN_MTU_TIMER_CONTINUOUS_CONTROL_REG QWLAN_MTU_TIMER_CONTROL11TO8_REG
#define QWLAN_MTU_TIMER_CONTINUOUS_CONTROL_ENABLE_N(index) \
    (1 << ((index) + QWLAN_MTU_TIMER_CONTROL11TO8_SW_MTU_CONTINUOUS_VALID_OFFSET))

#endif /* _NT_HW_H_ */
