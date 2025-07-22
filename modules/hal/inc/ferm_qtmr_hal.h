/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#ifndef _FERM_I2C_HAL_
#define _FERM_I2C_HAL_
#include "qccx.h"

typedef volatile QTMR_AC_BASE_qtmr_ac_Type qtmr_ac_hal;
typedef volatile QTMR_V1_T0_BASE_qtmr_v1_t0_Type qtmr_tmr_hal;

#ifdef QTMR_HAL
static inline void qtmr_hal_tmr_phy_ctr(qtmr_tmr_hal *hal, uint32_t value)
{
    hal->QTMR_V1_CNTP_CTL.reg = value;
}

static inline void qtmr_hal_tmr_get_phy_cnt(qtmr_tmr_hal *hal, uint32_t *cnt_hi, uint32_t *cnt_lo)
{
    *cnt_hi = hal->QTMR_V1_CNTPCT_HI.reg;
    *cnt_lo = hal->QTMR_V1_CNTPCT_LO.reg;
}

static inline void qtmr_hal_tmr_set_phy_cval(qtmr_tmr_hal *hal, uint32_t cval_hi, uint32_t cval_lo)
{
    hal->QTMR_V1_CNTP_CVAL_HI.reg = cval_hi;
    hal->QTMR_V1_CNTP_CVAL_LO.reg = cval_lo;
}

static inline void qtmr_hal_ac_set_cnt_freq(qtmr_ac_hal *hal, uint32_t freq)
{
    hal->QTMR_AC_CNTFRQ.reg = freq;
}

static inline void qtmr_hal_ac_set_cnt_acr(qtmr_ac_hal *hal, uint32_t frame, uint32_t value)
{
    hal->QTMR_AC_CNTACR[frame].reg = value;
}

#endif  // FERM_QTIMER_HAL
#endif  //_FERM_I2C_HAL
