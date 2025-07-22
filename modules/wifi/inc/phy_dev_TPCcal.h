/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#if !defined(_PHY_DEV_TPCCAL_H)
#define _PHY_DEV_TPCCAL_H
#include "phyDevLib.h"
//#include "phyrf_api.h"
//#define HALMAC_UFW 1
uint8_t tpcGetDemandedPowerCode(int8_t targetPower);  // convert desired target power to 'demand_power_code' used in TPE
#define NUM_CHAINS_IN_PHY 2

#define NUM_GLUT_ENTRIES 16
#define NUM_PLUT_ENTRIES 32
#define NUM_ALUT_ENTRIES 8
#define MAX_CHAINS_BITS  2  // No. of bits which are used to indicate the number of chains.
#define NUM_GLUT_IDX_IN_CAL_TABLE \
    5  // No. of GLUT entries present per channel per chain in the CAL table - either cal data or BDF file.

#define CCK_START_GLUT_IDX 1
#define CCK_END_GLUT_IDX   5  // Anything below 15

#define OFDM_LOWMCS_START_GLUT_IDX 1
#define OFDM_LOWMCS_END_GLUT_IDX   5  // Anything below 15

#define OFDM_HIGHMCS_START_GLUT_IDX 1
#define OFDM_HIGHMCS_END_GLUT_IDX   5  // Anything below 15

#define SINGLE_RU_START_GLUT_IDX 1
#define SINGLE_RU_END_GLUT_IDX   5  // Anything below 15

#define BTCOEX_START_GLUT_IDX   1
#define BTCOEX_END_GLUT_IDX     5  // Anything below 15
#define DPDTRAIN_START_GLUT_IDX 1
#define DPDTRAIN_END_GLUT_IDX   5  // Anything below 15
#ifndef SM_HWIO
#define SM_HWIO(io, field, val) \
    (((uint32)(val) << HWIO_SHFT(io, field)) & HWIO_FMSK(io, field))  // is this already defined somewhere else ???
#endif
#ifndef MS_HWIO
#define MS_HWIO(io, field, val) \
    (((uint32)(val) & (HWIO_FMSK(io, field))) >> (HWIO_SHFT(io, field)))  // is this already defined somewhere else ???
#endif

#define CLPC_MODE_FORCED_GAIN         0
#define CLPC_MODE_XCOUPLER_IPDET      1
#define CLPC_MODE_VDET                2
#define CLPC_MODE_OLPC                3
#define CLPC_MODE_ICOUPLER_INTERSTAGE 4
#define CLPC_MODE_ICOUPLER_OUTSTAGE   5

#define GET_CHAIN_NUM(x)                  (x) == 0x1 ? 0 : (x) == 0x2 ? 1 : (x) == 0x4 ? 2 : (x) == 0x8 ? 3 : GET_CHAIN_NUM_EXT(x)
#define GET_CHAIN_NUM_EXT(x)              (x) == 0x10 ? 0 : (x) == 0x20 ? 1 : (x) == 0x40 ? 2 : (x) == 0x80 ? 3 : 4
#define TX_CONVERGENCE_TXGAIN_COUNT_DELTA 10
#define TX_CONVERGENCE_PWR_DELTA          3
#define TX_CONVERGENCE_REGREAD_SUCCESS    1
#define TX_CONVERGENCE_REGREAD_FAILURE    0
#define TX_CONVERGENCE_VALIDATION_SUCCESS 1
#define TX_CONVERGENCE_VALIDATION_FAILURE 0
#define TX_CONVERGENCE_RETRY_COUNT        30
#define TX_CONVERGENCE_RETRY_TIME_US      2000

typedef struct {
    uint8_t PdadcLowPwr2G[PHY_CHAIN_MAX];
    uint8_t PdadcLowPwr5G[PHY_CHAIN_MAX];
    uint8_t PdadcHiPwr2G[PHY_CHAIN_MAX];
    uint8_t PdadcHiPwr5G[PHY_CHAIN_MAX];
    int8_t AvgPwrPdadc702G[PHY_CHAIN_MAX];
    int8_t AvgPwrPdadc705G[PHY_CHAIN_MAX];
} __ATTRIB_PACK OTP_PDADC_VALS;

RECIPE_RC phyrf_tpc_scpc_setting(void *phy_Input, void *tpc_scpc_setting_Input, void *tpc_scpc_setting_Output);
RECIPE_RC phyrf_tpc_clpc_setting(void *phy_Input, void *tpc_clpc_setting_Input, void *tpc_clpc_setting_Output);
void phphyrf_tpc_temp_comp_set_FineGainOffset(uint8_t fineGainOffsetOFDM, uint8_t fineGainOffset11b);
void phphyrf_tpc_temp_comp_get_FineGainOffset(uint8_t *pFineGainOffsetOFDM, uint8_t *pFineGainOffset11b);

typedef struct dummy_glut_data {
    uint16_t pwr_meas_cal;
    uint16_t txgain_idx_cal;
    uint16_t clpc_err;
    uint16_t glut_idx;
    uint16_t tpc_cl_pkt_cnt;
    uint8_t min_dac_gain_cal;
    uint8_t read_reg_31_0;
    uint8_t read_reg_63_32;
} __ATTRIB_PACK DUMMY_GLUT_DATA;

typedef struct dummy_gain_settings {
    uint16_t max_dac_gain_cal;
    uint16_t dac_gain_cal;
    uint16_t target_power_boundaries[3];
} __ATTRIB_PACK DUMMY_GAIN_SETTINGS;

typedef struct tx_convergence {
    int32_t txgain_count[2];
    int32_t stat_latest_fwd;
    int32_t stat_latest_lb;
    uint8_t tx_converged;
} __ATTRIB_PACK TX_CONVERGENCE;

typedef enum TX_CONVERGENCE_ENUM {
    TX_CONVERGENCE_INIT = 0,
    TX_CONVERGENCE_UPDATE,
    TX_CONVERGENCE_CLEAR,
} TX_CONVERGENCE_ENUM;
#if defined(HALMAC_UFW)
#include "hal_int_phy.h"
//#include "hal_int_rates.h"
void halphy_set_tx_pwr_for_rate(hal_phy_rates_t phyRate, int16_t desired_tx_power);
int16_t halphy_get_tx_pwr_for_rate(hal_phy_rates_t phyRate, uint16_t rfChannel);
#elif defined(HALMAC_UFW) && defined(SUPPORT_BDF)
#include "hal_int_phy.h"
//#include "hal_int_rates.h"
void halphy_set_tx_pwr_for_rate(hal_phy_rates_t phyRate, int16_t desired_tx_power);
int16_t halphy_get_tx_pwr_for_rate(hal_phy_rates_t phyRate, uint16_t rfChannel);
#endif
#endif
