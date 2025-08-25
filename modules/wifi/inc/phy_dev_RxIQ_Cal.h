/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**===========================================================================

 FILE:
 phyrf_rxiq_cal.h

 BRIEF DESCRIPTION:
 This File Contains RxIQ Calibration Functions Header

 DESCRIPTION:
 ===========================================================================**/

#ifndef _HALPHY_RXIQ_CAL_H_
#define _HALPHY_RXIQ_CAL_H_

#include <stdint.h>
#include <stdbool.h>

#include "phyCalUtils.h"

#define RX_PHSQ_LAMBDA_SH     1
#define RX_PHSI_LAMBDA_SH     1
#define RXIQ_MAX_RX_GAINS_CNT 4
#define TH_SSB                1000000  // Single Sideband Threshold | 60 dB

/* RXIQ Correction */
typedef struct rxiq_correction_s {
    int16_t i_coeff;
    int16_t q_coeff;
} rxiq_correction_t;

struct phyrf_rxiq_internal_for_restore {
    uint8_t tpcPathSel : 2;
};
// static struct phyrf_rxiq_internal_for_restore grxiq4Restore;

void rxiq_cal_setup(uint8_t band_code, tx_power_mode_t tx_power_mode, bool init_mem, bool store_cals,
                    power_settings_t *ps, bool ifTest);

void rxiq_cal_setup_rf(uint8_t band_code, power_settings_t *ps, tx_power_mode_t tx_power_mode);
void rxiq_cal_restore(uint8_t tx_gain, bool restore_dcoc);

void rxiq_cal_write_rxiq_corr(uint8_t idx, int16_t coeff_i, int16_t coeff_q);
void rxiq_cal_read_rxiq_corr(uint8_t idx, int16_t *coeff_i, int16_t *coeff_q);

void rxiq_cal_measure_rximb(int8_t tone_idx, int16_t *aie, int16_t *pie, int32_t *SSB);
void run_rxiq_cal(uint8_t band_code, tx_power_mode_t tx_power_mode, int8_t k, uint8_t n_iters, bool init_mem,
                  bool restore_dcoc, int16_t start_phsi, int16_t start_phsq, int32_t tx_phsi, int32_t tx_phsq,
                  bool ifTest, dac_rate_t dacrate);

void rxiq_cal_copy_lut(uint8_t *gain_idx_list, uint8_t gain_idx_count, uint8_t memory_size);

void rxiq_cal_store_lut(uint8_t band_code, rxiq_correction_t *p_rxiq_corr);
void rxiq_cal_restore_lut(uint8_t band_code, rxiq_correction_t *p_rxiq_corr);

#if defined(EMULATION_BUILD) && defined(PLATFORM_FERMION)
void inject_impairment_for_rxiqcal();  // for FPGA emulation
void remove_impairment_for_rxiqcal();  // for FPGA emulation
#endif
#endif /* _HALPHY_RXIQ_CAL_H_ */
