/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "stdint.h"

#ifndef CORE_SYSTEM_INC_NT_CC_BATT_MNG_H_
#define CORE_SYSTEM_INC_NT_CC_BATT_MNG_H_

typedef struct {
    // char* action;
    float voltage[42];
    uint32_t value[42];
} vol_table_t;

typedef struct {
    // char* action;
    float voltage[42];
    uint32_t value[42];
} lookuptable_t;

// void nt_vbatt_callback_reg(void (*ptr),uint32_t threshold);
// void nt_temp_callback_reg(void (*ptr), uint32_t threshold);
void nt_vbatt_callback_reg(uint32_t threshold);
void nt_temp_callback_reg(uint32_t threshold);
// int nt_battery_voltage_table(void);

// int battery_lookup_table(void);

#endif /* CORE_SYSTEM_INC_NT_CC_BATT_MNG_H_ */
