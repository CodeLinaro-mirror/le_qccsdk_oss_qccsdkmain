/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef CORE_SYSTEM_INC_NT_CPR_DRIVER_H_
#define CORE_SYSTEM_INC_NT_CPR_DRIVER_H_

#include "nt_flags.h"

#ifdef NT_FN_CPR

#include <stdint.h>
/*-----------------------------------Enums/Typedefs------------------------------------------------*/

typedef enum nt_cpr_vdd_change_e { VDD_UP = 0, VDD_DOWN } nt_cpr_vdd_change_t;

/*-------------------------------------------------------------------------------------------------*/

/*---------------------------------------Macros----------------------------------------------------*/
#define NT_NVIC_ISER2        0xE000E108  // Irq 64 to 73 set Enable register
#define ENABLE_CPR_INTERRUPT (0x1 << 7)
/*-------------------------------------------------------------------------------------------------*/

/**
 * <!-- nt_cpr_init -->
 *
 * @brief Initializing CPR configurations such as writing TARGET_QUOT values, step_quot_init, GCNT, timer interval, etc
 * into respective registers.
 * @return void
 */
void nt_cpr_init(void);

/**
 * <!-- nt_cpr_init_poll -->
 *
 * @brief: Polling for the sensor enumeration check and get the up/down recommendation from CPR master
 * .
 * @return void
 */
void nt_cpr_init_poll(void);

/**
 * <!-- nt_cpr_pmic_vdd_adjustment -->
 *
 * @brief: ISR handler to make adjustment to the CX voltage rail based on the triggered interrupt
 * .from CPR master
 * @return void
 */
void nt_cpr_pmic_vdd_adjustment(uint8_t val);
void nt_cpr_pre_sleep_config(void);
void nt_cpr_post_sleep_config(void);

void nt_cpr_isr_handler(void);
#endif  // NT_FN_CPR

#endif /* CORE_SYSTEM_INC_NT_CPR_DRIVER_H_ */
