/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "nt_cc_batt_mng.h"

#include <stdlib.h>
#include "string.h"
#include "nt_hw.h"
#include "nt_common.h"
#include "stdint.h"

#include "nt_cc_battery_driver.h"

#if ((defined NT_FN_CC_MGMT) && (defined NT_HOSTLESS_SDK))

/**
 * <!-- nt_lookup_table_init -->
 *
 * @brief Allocate the memory to load the look up table
 * @return: void
 */
void nt_lookup_table_init()
{
    uint32_t value;
    value = (uint32_t)malloc(sizeof(battery_measurements_t));
    (void)memset((uint32_t *)value, 0x0, sizeof(value));
}

/**
 * <!-- nt_vbatt_callback_reg -->
 *
 * @brief callback function to set the threshold for voltage
 * @param threshold : voltage
 * @return: void
 */
void nt_vbatt_callback_reg(uint32_t volt_threshold)
{
    uint32_t value;
    NT_REG_WR(QWLAN_PMU_CFG_TEMP_MON_TH_REG, volt_threshold);
    value = (1 << QWLAN_PMU_CFG_VABT_TEMP_MON_INT_EN_CFG_TEMP_PANIC_HIGH_HIT_INT_EN_OFFSET);
    NT_REG_WR(QWLAN_PMU_CFG_VABT_TEMP_MON_INT_EN_REG, value);
}

/**
 * <!-- nt_vbatt_callback_reg -->
 *
 * @brief callback function to set the threshold for temperature
 * @param threshold : temperature
 * @return: void
 */
void nt_temp_callback_reg(uint32_t temp_threshold)
{
    uint32_t value;
    NT_REG_WR(QWLAN_PMU_CFG_VABT_MON_TH_REG, temp_threshold);
    value = ((1 << QWLAN_PMU_CFG_VABT_TEMP_MON_INT_EN_CFG_VBAT_LOW_HIT_INT_EN_OFFSET));
    NT_REG_WR(QWLAN_PMU_CFG_VABT_TEMP_MON_INT_EN_REG, value);
}

#endif  // NT_FN_CC_MGMT && NT_HOSTLESS_SDK
