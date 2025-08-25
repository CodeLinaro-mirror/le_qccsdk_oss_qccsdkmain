/*
Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _HALPHY_DPD_CAL_H_
#define _HALPHY_DPD_CAL_H_

#include <stdint.h>
#include <stdbool.h>
#include "phyDevLib.h"
#include "phy_dev_wfm.h"
#include "phy_dev_DPD_pwl.h"
#include "phyCalUtils.h"

//#define NEUTRINO_LPBACK_TEST
/* below flag is due to DPD LUT HW bug. Read from LUT does not give correct values in order
 DPD Threshold value can not be read from HW after it is written. Also other values are shifed.
 Due to this we can not get the values from HW Read and hence can not use caldb functions related
 to DPD directly. Hence this is workaround flag and this should be disabled once HW Bug is fixed */
//#define NEUTRINO_DPD_LUT_HW_BUG
#define DPD_SCL_FACTOR (16384)
#define DPD_DIG_BO     (1)
#define DPD_nBin       (32)
#define DPD_nTrain     (64)
#define DPD_nCycle     (4)
#define DPD_nCordic    (7)
#define DPD_bScale     (2048)
#define DPD_nSample    (1)
//#define TX_PERIOD 384
#define TX_PERIOD    128
#define LEN_TXOFFSET 4

#define IQ_CORR_COEFF_MEM_COUNT (HWIO_CAL_REG_1RX_PRONTO_CAL_IQ_CORR_COEFF_MEMn_MAXn + 1)

#define YRAPP_P    3
#define YRAPP_VSAT 24000

#ifdef WFM_OPTIMIZE_STACK

typedef struct s_triangle_desc {
    iq_samples_t *iq;
    uint16_t sample_real;
    uint16_t sample_imag;
    uint16_t incr_real;
    uint16_t incr_imag;
    uint8_t i;
} triangle_desc_t;

void triangle_sample(wfm_desc_t *wfm_d, iq_samples_t *iq);
#endif /* WFM_OPTIMIZE_STACK */

struct phyrf_dpd_internal_for_restore {
    uint8_t tpcPathSel : 2;
};
#ifdef EMULATION_BUILD
// static struct phyrf_dpd_internal_for_restore gDpd4Restore;
#endif
/* DPD Correction each field is 12 bit number */
typedef struct dpd_correction_s {
    int16_t dpd_threshold; /* rA */
    int16_t dpd_aoffset;   /* bA */
    int16_t dpd_again;     /* mA in b_only mode this is 0 */
    int16_t dpd_poffset;   /* bP */
    int16_t dpd_pgain;     /* mP in b_only mode this is 0 */
} dpd_correction_t;

dpd_correction_t get_dpd_lut_coeff();
int run_dpd_cal(uint8_t bandCode, tx_power_mode_t tx_power_mode, PHYDEVLIB_DPDCAL_OUTPUT *dpdCalOutput);
void ReadDpdLut2CSR(dpd_correction_t *LUT);
void WriteDpdLut2CSR(dpd_correction_t *LUT);
#if defined(EMULATION_BUILD) && defined(PLATFORM_FERMION)
void inject_impairment_for_dpdcal();  // for FPGA emulation
void remove_impairment_for_dpdcal();  // for FPGA emulation
#endif
#endif /* _HALPHY_DPD_CAL_H_ */
