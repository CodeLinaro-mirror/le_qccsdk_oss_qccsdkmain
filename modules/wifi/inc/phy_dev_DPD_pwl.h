/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**===========================================================================

 FILE:
 phy_dev_dpd_pwl.h

 BRIEF DESCRIPTION:
 This File Contains DPD Post processing function headers and defines

 DESCRIPTION:
 ===========================================================================**/

#ifndef _PHY_DEV_DPD_PWL_H_
#define _PHY_DEV_DPD_PWL_H_

#include <stdint.h>
#include <stdbool.h>
#include "phy_dev_DPD_Cal.h"

typedef struct  // calibration parameters used in dpd_pwl_*.c
{
    // int8_t dpd_bOffset[4];
    uint8_t dpd_nBin;
    uint8_t dpd_nTrain;
    uint8_t dpd_nCycle;
    // uint8_t dpd_nSample;
    // uint32_t dpd_bScale;
} calpar_t;

typedef struct  // dpd calibration data
{
    float xi;
    float xq;
    float yi;
    float yq;
    float zi;
    float zq;
} iqpair_t;

typedef struct  // dpd calibration lut
{
    float rA;
    float mA;
    float bA;
    float mP;
    float bP;
} dpdlut_t;

enum { ENUM_DPD_AM, ENUM_DPD_PM };  // dpd calibration modes

int16_t dpd_pwl_align(const calpar_t cfg, iqpair_t xyz[]);
float dpd_pwl_apply(const calpar_t cfg, iqpair_t xyz[]);
void dpd_pwl_train(const calpar_t cfg, iqpair_t xyz[], dpdlut_t lut[]);
void dpd_pwl_train_est(const calpar_t cfg, iqpair_t xyz[], dpdlut_t lut[], int mode);
void dpd_pwl_train_sort(const calpar_t cfg, iqpair_t xyz[]);

float dpd_sat(float x, int nbit, int frac, int sign);
#ifdef NEUTRINO_LPBACK_TEST
float dpd_pwl_apply(const calpar_t cfg, iqpair_t xyz[]);  // not for halphy
#endif

#endif /* _HALPHY_DPD_PWL_H_ */
