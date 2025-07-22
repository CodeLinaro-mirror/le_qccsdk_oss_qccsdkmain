/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier:
 * BSD-3-Clause-Clear
*/

#ifndef _HALPHY_WFM_H_
#define _HALPHY_WFM_H_

#include <stdint.h>
#include <stdbool.h>
#include "phyCalUtils.h"

typedef struct s_iq_samples {
    int16_t i;  // ADC sample of PHY_I_RAIL
    int16_t q;  // ADC sample of PHY_Q_RAIL
} iq_samples_t;

#ifdef WFM_OPTIMIZE_STACK
typedef struct s_wfm_desc {
    uint16_t num_samples;
    void *info;
} wfm_desc_t;

typedef void(wfm_sample_func)(wfm_desc_t *, iq_samples_t *);

typedef struct s_tone_desc {
    iq_samples_t *p_dc_offset;
    uint8_t tone;
    int8_t index_i, index_q;
    int8_t dir_i, dir_q;
    int8_t sign_i, sign_q;
} tone_desc_t;

void tone_sample(wfm_desc_t *wfm_d, iq_samples_t *iq);
void phy_load_waveform(wfm_desc_t *wfm_d, wfm_sample_func *sample_fn, dac_rate_t dac_rate);
#else
void phy_load_waveform(const iq_samples_t *p_wave, uint16_t num_samples, iq_samples_t *p_dc_offset,
                       dac_rate_t dac_rate);
#endif /* WFM_OPTIMIZE_STACK */

int phy_wfm_start_tone(uint8_t tone, uint16_t amp, iq_samples_t *p_dc_offset, dac_rate_t dac_rate);
void phy_wfm_stop(void);

#endif
