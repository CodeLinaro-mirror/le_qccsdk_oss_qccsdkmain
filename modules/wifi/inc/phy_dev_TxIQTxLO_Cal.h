/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**===========================================================================

 FILE:
 phyrf_txiq_txlo.h

 BRIEF DESCRIPTION:
 This File Contains TXLO and TXIQ Calibration routines

 DESCRIPTION:
 ===========================================================================**/

#ifndef _HALPHY_TXIQ_TXLO_CAL_H_
#define _HALPHY_TXIQ_TXLO_CAL_H_
#include <stdint.h>
#include <stdbool.h>

#include "phyCalUtils.h"

typedef enum cal_type_s {
    TXIQ = 0,
    TXLO = 1,
    RXCAL = 2,
    TXIQ_TEST = 3,
    TXLO_DIG = 4,
    TXLO_TEST = 5,
    TX_NONE
} cal_type_t;

typedef struct txiq_txlo_result_s {
    int32_t phaimb;
    int32_t ampimb;
} txiq_txlo_result_t;

struct phyrf_txlo_txiq_internal_for_restore {
    uint8_t tpcPathSel : 2;
    uint8_t tpcTIAIC : 3;
    uint8_t clpc_en;
    uint8_t txowr_override;
    uint8_t fixed_gain;
};

void run_txiq_txlo_cal(uint8_t band_code, tx_power_mode_t tx_power_mode, int32_t k, uint8_t odac_range,
                       cal_type_t cal_mode, bool do_envdet_cal, uint8_t max_n_iter, int32_t ss_cpm, int32_t ss_ca,
                       int32_t test_cpm, int32_t test_ca, bool override, dac_rate_t dacrate,
                       txiq_txlo_result_t *txiq_txlo_result);
#if defined(EMULATION_BUILD) && defined(PLATFORM_FERMION)
void inject_impairment_for_txlocal();  // for FPGA emulation, Tx LO
void remove_impairment_for_txlocal();  // for FPGA emulation, Tx LO
void inject_impairment_for_txiqcal();  // for FPGA emulation, Tx IQ
void remove_impairment_for_txiqcal();  // for FPGA emulation, Tx IQ
#endif
#endif
