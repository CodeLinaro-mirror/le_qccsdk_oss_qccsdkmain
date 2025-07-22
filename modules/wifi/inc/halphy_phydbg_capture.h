/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*========================================================================

 * @file chalphy_phydbg_capture.h
 * @brief PHYBDG Capture related parameters and function declarations
 * ======================================================================*/

/*------------------------------------------------------------------------
 * Include Files
 * ----------------------------------------------------------------------*/

#include <stdint.h>
#include <stdbool.h>

/*------------------------------------------------------------------------
 * Type Declarations
 * ----------------------------------------------------------------------*/

typedef enum phydbg_capt_type_e { capt_type_xbar = 0, capt_type_agc = 1 } phydbg_capt_type_t;

typedef enum phydbg_capt_trigger_e {
    capt_trigger_none = 0,
    capt_trigger_crcfail = 1,
    capt_trigger_agc = 2
} phydbg_capt_trigger_t;

typedef enum phydbg_capt_data_format_e {
    capt_data_format_unsigned = 0,
    capt_data_format_twos_complement = 1,
    capt_data_format_offset_binary = 2
} phydbg_capt_data_format_t;

typedef struct phydbg_adc_sample_s {
    int16_t i;
    int16_t q;
} __ATTRIB_PACK phydbg_adc_sample_t;

void phydbg_capture_adc(uint16_t n_samples, phydbg_capt_trigger_t trigger, bool post_iq,
                        phydbg_adc_sample_t *p_adc_sample);
