/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*========================================================================
 *
 * @file wlan_regulatory.h
 * @brief Function header and defines for FW Regulatory
 * ======================================================================*/

#ifndef _wlan_regulatory_H_
#define _wlan_regulatory_H_

/*------------------------------------------------------------------------
 * Include Files
 * ----------------------------------------------------------------------*/

#include "fwconfig_wlan.h"
#include "nt_flags.h"
#ifdef SUPPORT_REGULATORY
#include <stdint.h>
#include <stdbool.h>
#include "halphy_regulatory_db_struct.h"
#include "wlan_dev.h"
/*------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * ----------------------------------------------------------------------*/
#define CH_BW_5MHz                       5
#define EDGE_BAND_10MHz                  10
#define CH_BW_20MHz                      20
#define WIFI_MAX_CH_LIST                 102
#define MAX_REGULATORY_RULES_PER_COUNTRY 17
#define MAX_6G_POWER_TYPE_STA_SAP        6
#define REGULATORY_DEFAULT_POWER         10  // dbm when freq is not reg or reg is not initiated default power will be used

// Edge Frequencies for reg/CTL power

#define ADDITIONAL_BAND_5G_1 5940
#define ADDITIONAL_BAND_5G_2 5960  // Freq falls in 6G band
#define ADDITIONAL_BAND_5G_3 5980  // Freq falls in 6G band

#define REGULATORY_5G_LOWER_BOUND 5180  // Start of 5GHZ band
#define REGULATORY_5G_UPPER_BOUND 5900  // End of 5.9GHz band, 5915 is still in proposed state
#define REGULATORY_6G_LOWER_BOUND 5955  // Freq falls in 6G band
#define REGULATORY_6G_UPPER_BOUND 6415  // Freq falls in 6G band

typedef enum {
    FREQ_BAND_2G = 0x1,
    FREQ_BAND_5G = 0x2,
    FREQ_BAND_6G = 0x4,
    FREQ_BAND_2G_5G = FREQ_BAND_2G | FREQ_BAND_5G,
    FREQ_BAND_2G_5G_6G = FREQ_BAND_2G | FREQ_BAND_5G | FREQ_BAND_6G,
} CURRENT_FREQ_BAND;

typedef enum {
    STA_SP,
    SAP_SP,
    STA_LPI,
    SAP_LPI,
    STA_VLP,
    SAP_VLP,
    INVALID_POWER_TYPE,
} POWER_TYPE_6G;

/*------------------------------------------------------------------------
 * Function Declarations and Documentation
 * ----------------------------------------------------------------------*/
#ifdef REGULATORY_TEST_FRAMEWORK
#ifdef CONFIG_6G_BAND
void wlan_reg_chlist_unit_test(POWER_TYPE_6G power_type_6g);
#else
void wlan_reg_chlist_unit_test(void);
#endif
void wlan_print_ch_list(void);

void wlan_reg_packed_reg_unit_test();
#endif /* REGULATORY_TEST_FRAMEWORK */

#ifdef CONFIG_6G_BAND
int16_t wlan_regulatory_get_reg_power(uint16_t freq, uint8_t *regdmn, uint8_t *power_type_6g);
uint8_t wlan_regulatory_unpack(dev_common_t *pDevCmn, uint8_t freq_band, POWER_TYPE_6G power_type_6g);
#else
int16_t wlan_regulatory_get_reg_power(uint16_t freq, uint8_t *regdmn);
uint8_t wlan_regulatory_unpack(dev_common_t *pDevCmn, uint8_t freq_band);
#endif

NT_BOOL wlan_regulatory_set_country_code(dev_common_t *pDevCmn, uint32_t country_code);
uint8_t wlan_regulatory_find_num_ch(dev_common_t *pDevCmn, uint8_t num_reg_rules, uint8_t reg_rule_offset,
                                    uint8_t ch_min_bw);
int8_t wlan_regulatory_set_tx_power(uint8_t dbm, uint8_t policy);

#endif /* SUPPORT_REGULATORY */
#endif /* _wlan_regulatory_H_ */
