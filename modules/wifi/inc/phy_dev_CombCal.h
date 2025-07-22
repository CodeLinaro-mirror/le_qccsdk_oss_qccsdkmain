/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#if !defined(_PHY_DEV_FDMT_COMB_CAL_H)
#define _PHY_DEV_FDMT_COMB_CAL_H

/*!
@file phy_dev_fdmtCombCal.h
@brief
PHY VI combined CAL user header file
*/
#include "phyDevLib.h"
#include "phyDevLibCommon.h"

//#include "phyDevLibHastingsPrime.h"
#include "phyDevLibApiParm.h"
#include "phyDevLibApiDef.h"
//#include "q5_math.h"
#include "phyCalUtils.h"
#include "phyReset.h"
//#define _DBG_REPROCESS 1
//#define _DBG_REPROCESS_ 1
//#define _DBG_REPROCESS_API 1
//#define _DBG_REPROCESS_CALTIME 1
//#define Combined_DEBUG_2
#define TX_DCOCBB_RANGE 2
#if defined(Combined_DEBUG_2)
#define RXIQ_SEARCH_N_ITER 6  // Number of iteraration for RXIQ Cal search
#else
#define RXIQ_SEARCH_N_ITER 20  // Number of iteraration for RXIQ Cal search
#endif
#if defined(PHYDEVLIB_IMAGE_STANDALONE) && defined(_DBG_REPROCESS)
#include <sys/types.h>
#include <sys/stat.h>

#ifdef _WINDOWS
#define MKDIR(d, m) _mkdir(d)
#else
#define MKDIR(d, m) mkdir(d, m)
#endif

#endif

#define PI      3.14159265358979323846
#define INV_2PI 162

#define MAX_TONE_LIST 48

//#define MAX_RXCAL_GAINS 25  // Defined below, which one to use?
#define MAX_CALIB_WAY 3
#define MAX_TONES     16
#define MAC_CORR      48

//////////////////////////////////fdmt/////////////////////
#define COPY_HP_TO_LP             0
#define LOWER_UPPER_WORD          2
#define MAX_RX_CORRECTION_ENTRIES 16
#define LB_CALRTX                 1
#define LB_TRSW                   3
#define DCO_IQ_CAL_GAINS          2
#define RXGAIN_CAL                3
#define MAX_CAL_ENGINES           2
#define PARAMETERS                5
#define DIRECTION_TX              1
#define DIRECTION_RX              2
#define INVALID                   3
#define MAX_ITERATIONS            16  // justin: for iter
#define MAX_CAL_GAINS             16
#define NUM_RTX_TAPS_PHYA         11
#define NUM_TX_TAPS_PHYB          7
#define NUM_RX_TAPS_PHYB          9
//#define MAX_NUM_TAPS NUM_RTX_TAPS_PHYA
#define MAX_NUM_TAPS               NUM_RX_TAPS_PHYB
#define NUM_TAPS_CPS               3
#define FDMT_TX_IQ_CAL             1
#define FDMT_BA                    5  // fdmt cal type =5 - check???
#define FDMT_AGC                   4
#define MEM_START_ADDR_POINTER     0
#define PHYDBG_BANK2_START_ADDRESS 4096
#define ENABLE_PHYDBG_CLOCK        1
#define TRIGGER_PLAYBACK           1
#define BW20_NUM_TONES             11
#define BW40_NUM_TONES             21
#define BW80_NUM_TONES             21
#define BW160_NUM_TONES            21

#define MAX_NUM_TONES BW160_NUM_TONES

#define MAX_FFT_SIZE   512
#define MAX_BA_SAMPLES 1920

#define MAX_CAL_RUNS    3
#define MAX_CAL_RUNS_CC 6

#define MAX_PILOT_SIZE     10
#define MAX_DC_SIZE        1
#define MAX_ALL_PILOT_SIZE MAX_PILOT_SIZE * 2 + 1
#define MAX_PHASE_SHIFT    2

#define MAX_RX_GAINS               64
#define MAX_TX_GAIN_TBL_SIZE       64
#define MAX_RX_GAIN_TBL_SIZE       256
#define MAX_RXIQ_LOOPBACK_GAINS    34
#define MAX_NUM_STAGES             2
#define MAX_DAC_OSR                24
#define MAX_TPC_GLUT               16
#define TONE_IDX_START_ADDR_PHYA   2022
#define TONE_IDX_START_ADDR_PHYB   1010
#define DFT_RESULT_START_ADDR_PHYA 1920
#define DFT_RESULT_START_ADDR_PHYB 960

#define MAX_REDOAGC_CNT 6
#define SW_AGC_CHK      1
#define useFdSync       1
#define USE_HW_DFT
#define justin_dbg3 1

//#define single_tap1
//#define single_tap2
#define single_tap 0  // 3: txAmp only, 2: amp only, 1: both amp and pha, 0: none

#define FABS(x) ((x) > 0 ? (x) : -(x))
#define MYDIV(a, b) \
    ((((a) > 0 && (b) > 0) || ((a) < 0 && (b) < 0)) ? (((a) + ((b) >> 1)) / (b)) : (((a) - ((b) >> 1)) / (b)));
//#define MYDIV(a, b) ((a>0) ? ((a + (b >> 1)) / b) : ((a - (b >> 1)) / b));
#define MYDIV2(a, b) \
    (((a) > 0) ? (((a) + ((int64_t)1 << ((b)-1))) >> (b)) : (((a) + (((int64_t)1 << ((b)-1)) - 1)) >> (b)));
//#define MOD_FUNC(a, b)  (((int32_t)(a) < 0) ? (uint32_t) ((int32_t) (a)%(int32_t)(b) + (int32_t)(b)) :
//(uint32_t)((int32_t)(a) % (int32_t)(b)))

#define MOD_FUNC(a, b) (uint32_t)(((int32_t)(a) % (int32_t)(b) + (int32_t)(b)) % (int32_t)(b))

typedef enum {
    NOT_USE_IDX = 0,
    NOT_CAL_IDX = 1,
    CALED_IDX = 2,
} FDMT_CORR_IDX;

typedef enum {
    DAC_BW_60 = 0,
    ADC_BW_30 = 0,
    DAC_BW_120 = 1,
    ADC_BW_60 = 1,
    DAC_BW_240 = 2,
    ADC_BW_120 = 2,
    DAC_BW_480 = 3,
    ADC_BW_240 = 3,
    ADC_BW_480 = 4,
} DAC_ADC_RATE;

typedef struct {
    int8_t dac_scale_db;
    uint8_t txgain_idx;
    uint8_t rxgain_idx;
    uint8_t rxcorr_idx;
    uint8_t calRTxGc;
    uint8_t pads[3];

} __ATTRIB_PACK SAVED_GAINS;

typedef struct {
    uint32_t gain_idx;
    PHY_COMPLEX coeffs[MAX_NUM_TAPS];
    int32_t cps;
} __ATTRIB_PACK TX_IQEntry;

typedef struct {
    uint32_t gain_idx;
    PHY_COMPLEX coeffs[MAX_NUM_TAPS];
    int32_t cps;
} __ATTRIB_PACK RX_IQEntry;

typedef struct PHY16_COMPLEX {
    int16_t real;
    int16_t imag;
} PHY16_COMPLEX;

typedef struct PHY64_COMPLEX {
    int64_t real;
    int64_t imag;
} PHY64_COMPLEX;

#if 0
typedef struct txiq_txlo_result_s
{
	int32_t phaimb;
	int32_t ampimb;
} txiq_txlo_result_t;
#endif

// typedef enum e_dac_rate
//{
//	dac_rate_60, dac_rate_120
//} dac_rate_t;

// typedef enum e_tx_power_mode {
//	tx_power_mode_high, tx_power_mode_medium, tx_power_mode_low, tx_power_mode_very_low, num_tx_power_modes,
//} tx_power_mode_t;

#if 0
typedef enum phyrf_cal_type_s
{
	TXIQ = 0,
	TXLO = 1,
	RXCAL = 2,
	TXIQ_TEST = 3,
	TXLO_DIG = 4,
	TXLO_TEST = 5,
	TX_NONE
} phyrf_cal_type_s;
#endif

RECIPE_RC run_combined_cal(void *phy_input, void *fdmt_iqcal_input, void *fdmt_iqcal_output);
// RECIPE_RC combinedcal_enable(void *phy_input, void *fdmt_iqcal_input, void *fdmt_iqcal_output);
// RECIPE_RC combinedcal_disable(void *phy_input, void *fdmt_iqcal_input, void *fdmt_iqcal_output);
RECIPE_RC combinedcal_save(void *phy_input, void *fdmt_iqcal_input, void *fdmt_iqcal_output);
RECIPE_RC combinedcal_restore(void *phy_input, void *fdmt_iqcal_input, void *fdmt_iqcal_output);

#endif
