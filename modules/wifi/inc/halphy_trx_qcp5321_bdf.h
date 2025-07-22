/*========================================================================
Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 * @file  halphy_trx_qcp5321_bdf.h
 *
 *========================================================================*/

#ifndef HALPHY_TRX_FERMION_BDF_H
#define HALPHY_TRX_FERMION_BDF_H
#include <nt_flags.h>
/*------------------------------------------------------------------------
 * Include Files
 * ----------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>

#include <halphy_common.h>
#include <bdf_struct.h>
#include <component_bdf_struct.h>

/*------------------------------------------------------------------------
 * Type Declarations
 * ----------------------------------------------------------------------*/

// Definitions are synced to bdwlan

// CTL Regulatory domains
typedef enum ctl_domain {
    NEW_FCC = 0x1,
    NEW_ETSI = 0x3,
    NEW_MKK = 0x4,
    NEW_KOR = 0x5,
    NEW_CHN = 0x6,
    NEW_USER_DEFINE = 0x7,
    NEW_NO_CTL = 0xFF,
} ctl_domain_t;

typedef enum ctl_11b_offset_index {
    FCC_11B_OFFSET,
    ETSI_11B_OFFSET,
    MKK_11B_OFFSET,
    KOR_11B_OFFSET,
    CHN_11B_OFFSET,
    USER_DEFINE_11B_OFFSET
} ctl_11b_offset_index_t;

// CTL MODE - WLAN packet types
typedef enum ctl_mode {
    CTL_MODE_LEGACY = 0,
    CTL_MODE_11B = 1,
    CTL_MODE_HT20_VHT20 = 2,
} ctl_mode_t;

typedef enum power_types_6g /* 6G power mode, Not used if not 6G */
{ POWER_TYPE_LPI = 0,
  POWER_TYPE_SP = 1,
  POWER_TYPE_VLP = 2,
} power_types_6g_t;

typedef enum rate_idx_bdf_e {
    RATE_1_L = 0,
    RATE_2_L,
    RATE_2_S,
    RATE_5_L,
    RATE_5_S,
    RATE_11_L,
    RATE_11_S,
    RATE_6,
    RATE_9,
    RATE_12,
    RATE_18,
    RATE_24,
    RATE_36,
    RATE_48,
    RATE_54,
    RATE_HT_0,
    RATE_HT_1,
    RATE_HT_2,
    RATE_HT_3,
    RATE_HT_4,
    RATE_HT_5,
    RATE_HT_6,
    RATE_HT_7,
    RATE_MAX_BDF = 23,
} rate_idx_bdf_t;

/*------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * ----------------------------------------------------------------------*/

// CTL fail safe power limit                TODO:: All power values need to be get approved by  regulatory team and HWS
#define CTL_FAIL_SAFE_PWR            20  // 10 dBm - 0.50 dBm accuracy. Based on CTL BDF data. Multiplied x2 by FW
#define WHAL_MAX_RATE_POWER          63  // 31.5 dbm
#define CTL_FAIL_SAFE_FS             1
#define WHAL_MAX_RATE_PWR_MULTIPLIER 2

#define MAX_PWR_FOR_ANY_RATE 60  // 15dbm, This value should be given by HWS
#define DEFAULT_SUBBAND      0

/*------------------------------------------------------------------------
 * Function Declarations and Documentation
 * ----------------------------------------------------------------------*/
ctl_mode_t halphy_get_ctl_mode(rate_idx_bdf_t rate_idx);
int8_t get_target_power(uint16_t channel, rate_idx_bdf_t rate_idx);
#ifdef CONFIG_6G_BAND
int16_t get_max_edge_power(const uint16_t chan, ctl_domain_t reg_dmn, ctl_mode_t mode, power_types_6g_t power_mode_6G);
#else
int16_t get_max_edge_power(const uint16_t chan, ctl_domain_t reg_dmn, ctl_mode_t mode);
#endif
void phyrf_bdf_get_scpc_fine_gain_offset(uint16_t freq, SCPC_POWER_OFFSET *p_scpc_offset);
void phyrf_bdf_get_rxgain_cal_data(uint16_t freq, RXGAIN_CAL_DATA *p_rxgain_cal_data);
int8_t halphy_get_tx_power_offset(void);

void phyrf_bdf_clear_rxgain_cal_data_by_band(uint8_t band);
int8_t phyrf_bdf_get_rxgain_cal_cfg_refISS(uint8_t band);
uint8_t phyrf_bdf_get_rxgain_cal_cfg_rate(uint8_t band);
uint8_t phyrf_bdf_get_rxgain_cal_cfg_num_chan(uint8_t band);
uint16_t phyrf_bdf_get_rxgain_cal_cfg_num_pkts(uint8_t band);
uint16_t phyrf_bdf_get_rxgain_cal_cfg_chan(uint8_t band, uint8_t chan_idx);
void phyrf_bdf_set_rxgain_cal_result(uint8_t band, uint8_t chan_idx, uint8_t rxNFCalPowerDBr, uint8_t rxNFCalPowerDBm,
                                     uint8_t minCCAThreshold);

void phyrf_bdf_clear_scpc_cal_data_by_band(uint8_t band);
void phyrf_bdf_get_scpc_cal_cfg_by_band(uint8_t band, SCPC_CAL_CFG *p_scpc_cal_cfg);
uint16_t phyrf_bdf_get_tpc_cal_chan(uint8_t band, uint8_t chan_idx);
void phyrf_bdf_set_scpc_cal_result(uint8_t band, uint8_t chan_idx, bool is_ofdm, int8_t scpc_power_offset);

void phyrf_bdf_get_ed_threshold(uint16_t freq, uint8_t *p_agc_ed_threshold, power_types_6g_t power_mode_6g);
#if (FERMION_CHIP_VERSION == 2)
int8_t phyrf_bdf_get_fine_gain_offset(int8_t offset, mod_type_t mod_type, bool isTpcCal);
#endif
int8_t phyrf_bdf_get_fine_gain_process_corner_offset(uint8_t chip_type, uint8_t band);

bool phyrf_bdf_is_ceb_enabled(freq_band_t band);
void phyrf_bdf_get_temp_based_update_list(PHYRF_REG_TEMP_BASED_UPDATE *p_phyrf_reg_update_list);

#if (FERMION_CHIP_VERSION == 2)
bool phyrf_bdf_is_dac_bo_enable(uint16_t chan);
uint16_t phyrf_bdf_get_dac_bo_nom(uint16_t chan);
#endif /* #if (FERMION_CHIP_VERSION == 2) */
#endif /* HALPHY_TRX_FERMION_BDF_H */
