/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**===========================================================================

 FILE:
 phyrf_rxdco_cal.h

 BRIEF DESCRIPTION:
 This File Contains RxDCO Calibration Functions Headers

 DESCRIPTION:
 ===========================================================================**/

#ifndef _HALPHY_RXDCO_CAL_H_
#define _HALPHY_RXDCO_CAL_H_

#include <stdint.h>
#include <stdbool.h>
#include "phyDevLib.h"
#include "phyCalUtils.h"

typedef enum e_search {
    SEARCH_BINARY = 0,       // Binary Search
    SEARCH_BRUTE_FORCE = 1,  // Brute Force Search
    SWEEP_CORR_FORCE = 2,    // Corr Sweep
} search_t;

#ifdef EMULATION_BUILD
#define RX_DCOC_RANGE 1
#else
#define RX_DCOC_RANGE 0
#endif

#define RXDCO_TH                2  // Target DC Offset Threshold
#define RXDCO_MAX_RX_GAINS_CNT  4
#define RXDCO_CRX_SUB_GAINS_CNT 6  /* number of gain tables for a LNA settings */
#define NUM_RXDCO_CRX_BLOCK     15 /* number of values with in one sub block of CRX */
#define RXDCO_CRX_BLOCK_OFFSET  80 /* start of CRX block RXDCO gain offset */

// For 1.0 chip: #define RXDCO_DCOCBB_FORMAT    3 /*  3: 2's complement */
// For 2.0 chip : #define RXDCO_DCOCBB_FORMAT    1 /*  1: signed magnitude*/
#if (FERMION_CHIP_VERSION == 1)
#define RXDCO_DCOCBB_FORMAT 3
#elif (FERMION_CHIP_VERSION == 2)
#define RXDCO_DCOCBB_FORMAT 1
#endif

typedef struct rxdco_lut_s {
    int16_t lut_i;
    int16_t lut_q;
} rxdco_lut_t;

typedef struct rx_dcoc_range_s {
    uint8_t dcoc_range_i;
    uint8_t dcoc_range_q;
} rx_dcoc_range_t;

void rxdco_cal_env_restore(uint8_t bandCode);

void rxdco_cal_env_setup(uint8_t bandCode, uint8_t dcoc_range, power_settings_t *ps_p, bool ifTest);

void rxdco_cal_setup_rf(uint8_t bandCode, uint8_t dcoc_range, power_settings_t *ps_p);

void rxdco_cal_measure_noshunt_dco(uint8_t bandCode, int16_t *dco_real, int16_t *dco_imag);

void rxdco_cal_write_dcoc_and_measure(uint8_t bandCode, uint8_t dco_lut_idx, int16_t dc_real, int16_t dc_imag,
                                      int16_t *meas_dc_i, int16_t *meas_dc_q, bool override, bool ifTest, bool ifEdet);

void rxdco_cal_search_brute_force(uint8_t bandCode, uint8_t dco_lut_idx, uint8_t ss, int16_t *corr_i, int16_t *corr_q,
                                  int16_t *min_dc_i, int16_t *min_dc_q, bool override, bool ifTest, bool ifEdet);

void rxdco_cal_search_binary(uint8_t bandCode, uint8_t dco_lut_idx, int8_t polarity, uint16_t target_th,
                             int16_t *corr_i, int16_t *corr_q, int16_t *meas_dc_i, int16_t *meas_dc_q, bool override,
                             bool ifTest, bool ifEdet);

void rxdco_cal_sweep_corr_force(uint8_t bandCode, uint8_t dco_lut_idx, uint8_t ss, int16_t *corr_i, int16_t *corr_q,
                                int16_t *min_dc_i, int16_t *min_dc_q, bool override, bool ifTest, bool ifEdet);

RECIPE_RC run_rxdco_cal(uint8_t bandCode, uint8_t *rx_gain_list, uint8_t rx_gain_count, search_t search,
                        uint16_t dc_thr, int16_t *corr_i, int16_t *corr_q, int16_t *res_i, int16_t *res_q,
                        uint8_t dcoc_range, bool ifTest, bool ifEdet, bool override);

void rxdco_cal_copy_lut(uint8_t bandCode, uint8_t *gain_idx_list, uint8_t gain_idx_count, uint8_t memory_size);

void rxdco_cal_store_lut(uint8_t bandCode, int8_t crx_mode, rxdco_lut_t *p_rxdco_lut, rx_dcoc_range_t *p_rx_dcoc_range,
                         rxdco_lut_t p_rxdco_lut_crx[][RXDCO_MAX_RX_GAINS_CNT], rx_dcoc_range_t *p_rx_dcoc_range_crx);
void rxdco_cal_restore_lut(uint8_t bandCode, int8_t crx_mode, rxdco_lut_t *p_rxdco_lut,
                           rx_dcoc_range_t *p_rx_dcoc_range, rxdco_lut_t p_rxdco_lut_crx[][RXDCO_MAX_RX_GAINS_CNT],
                           rx_dcoc_range_t *p_rx_dcoc_range_crx);

#if defined(EMULATION_BUILD) && defined(PLATFORM_FERMION)
void inject_impairment_for_rxdcocal();
void remove_impairment_for_rxdcocal();
#endif
#endif /* _HALPHY_RXDCO_CAL_H_ */
void store_dcoc_range_CRx();
void restore_dcoc_range_CRx();
void store_dcoc_range_nonCRx();
void restore_dcoc_range_nonCRx();
void rfpkdetDcoCal(uint8_t bandcode);
