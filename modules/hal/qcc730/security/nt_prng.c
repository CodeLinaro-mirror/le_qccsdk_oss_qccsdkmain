/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "nt_hw.h"
#include "hal_int_sys.h"
#include "nt_osal.h"
#include "safeAPI.h"
#include <stdio.h>
#include "qccx.h"
#include "wifi_fw_pwr_cb_infra.h"

void prng_enable_clock()
{
#if CONFIG_SOC_QCC730V1
    QCC730V1_CCU_BASE_Type *ccu = QCC730V1_CCU_BASE;
#elif CONFIG_SOC_QCC730V2
    QCC730V2_CCU_BASE_Type *ccu = QCC730V2_CCU_BASE;
#endif
    ccu->ccu.CCU_R_CCU_ENABLE_CLK.bit.PRNG_ENABLE_CLK = 1;
}

int8_t nt_prng_init(void);
extern void pka_power_switch_to_config();
void rng_power_state_change_cb(uint8_t evt, void *p_args)
{
    (void)p_args;

    if ((evt == PWR_EVT_WMAC_POST_AWAKE) || (evt == PWR_EVT_WMAC_SLEEP_ABORT)) {
        nt_prng_init();
    }
}

int8_t nt_prng_init(void)
{
    uint32_t val = 0;
    uint32_t qcc_reset_delay = 0xFF;

    pka_power_switch_to_config();
    prng_enable_clock();
    // Issue a PRNG SW reset
    val = HAL_REG_RD(QWLAN_PRNG_R_PRNG_CONFIG_REG);
    val |= (1 << QWLAN_PRNG_R_PRNG_CONFIG_SW_RESET_OFFSET);
    HAL_REG_WR(QWLAN_PRNG_R_PRNG_CONFIG_REG, val);

    while (--qcc_reset_delay)
        ;

    /* Enable RNG clock source || By default peripheral clock is enabled in ccu register */
    /* RNG Peripheral enable | Enable PRNG only if it is not already enabled */
    // setting the ring oscillator clock
    val = (QWLAN_PRNG_R_PRNG_LFSR_CFG_RING_OSC0_CFG_EFEEDBACK_POINT_0
           << QWLAN_PRNG_R_PRNG_LFSR_CFG_RING_OSC0_CFG_OFFSET) |
          QWLAN_PRNG_R_PRNG_LFSR_CFG_LFSR0_EN_MASK |
          (QWLAN_PRNG_R_PRNG_LFSR_CFG_RING_OSC1_CFG_EFEEDBACK_POINT_0
           << QWLAN_PRNG_R_PRNG_LFSR_CFG_RING_OSC1_CFG_OFFSET) |
          QWLAN_PRNG_R_PRNG_LFSR_CFG_LFSR1_EN_MASK |
          (QWLAN_PRNG_R_PRNG_LFSR_CFG_RING_OSC2_CFG_EFEEDBACK_POINT_0
           << QWLAN_PRNG_R_PRNG_LFSR_CFG_RING_OSC2_CFG_OFFSET) |
          QWLAN_PRNG_R_PRNG_LFSR_CFG_LFSR2_EN_MASK |
          (QWLAN_PRNG_R_PRNG_LFSR_CFG_RING_OSC3_CFG_EFEEDBACK_POINT_0
           << QWLAN_PRNG_R_PRNG_LFSR_CFG_RING_OSC3_CFG_OFFSET) |
          QWLAN_PRNG_R_PRNG_LFSR_CFG_LFSR3_EN_MASK;

    HAL_REG_WR(QWLAN_PRNG_R_PRNG_LFSR_CFG_REG, val);

    // enable the PRNG
    qcc_reset_delay = 0xFF;
    while (--qcc_reset_delay)
        ;

    val = HAL_REG_RD(QWLAN_PRNG_R_PRNG_CONFIG_REG);
    val |= (1 << QWLAN_PRNG_R_PRNG_CONFIG_PRNG_EN_OFFSET);
    HAL_REG_WR(QWLAN_PRNG_R_PRNG_CONFIG_REG, val);

    fpci_evt_cb_reg((ps_evt_cb_t)&rng_power_state_change_cb,
                    PWR_EVT_WMAC_PRE_SLEEP | PWR_EVT_WMAC_POST_AWAKE | PWR_EVT_WMAC_SLEEP_ABORT, 10, NULL);

    return 0;
}

uint32_t nt_pget_rng(void)
{
    uint32_t data = 0;
    uint32_t val = 0;

    val = HAL_REG_RD(QWLAN_PRNG_R_PRNG_STATUS_REG);
    val &= (1 << QWLAN_PRNG_R_PRNG_STATUS_DATA_AVAIL_OFFSET);
    if (val) {
        data = HAL_REG_RD(QWLAN_PRNG_R_PRNG_DATA_OUT_REG);
    }

    return data;
}

NT_BOOL nt_wlan_hw_prng_get(uint8_t *ptr, uint16_t len)
{
    volatile uint32 tmp_iv;
    uint32_t i;
    const uint32_t unit_random_len = 4;

    if (!ptr || (0 == len)) {
        return FALSE;
    }

    for (i = 0; i < (len / unit_random_len); i++) {
        tmp_iv = nt_pget_rng();
        memscpy((void *)ptr, 4, (void *)&tmp_iv, 4);
        ptr += 4;
    }
    if (len % 4) {
        tmp_iv = nt_pget_rng();
        memscpy((void *)ptr, len % 4, (void *)&tmp_iv, len % 4);
    }
    return TRUE;
}
