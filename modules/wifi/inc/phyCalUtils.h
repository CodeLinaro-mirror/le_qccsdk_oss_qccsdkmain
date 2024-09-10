/*
Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**===========================================================================

 FILE:
 phyCalUtils.h

 BRIEF DESCRIPTION:
 This File Contains All Calibration Utilities Functions Header

 DESCRIPTION:
 ===========================================================================**/

#ifndef _HALPHY_CAL_UTILS_H_
#define _HALPHY_CAL_UTILS_H_

#include <stdint.h>
#include <stdbool.h>
#include "phyDevLib.h"
#include "fermion_reg.h"


#ifdef _WIN32
#include <stdio.h>

#define NT_LOG_PRINT(MODNAME,LVL,msg,...) {\
                        printf(msg, __VA_ARGS__); \
                        printf("\n");\
                        }
#include <Windows.h>
#define nt_osal_calloc(count, size) calloc(count, size)
#define nt_osal_free_memory(ptr) free(ptr)
#define memscpy memcpy_s
#elif defined(__linux__) && defined(PHYDEVLIB_IMAGE_STANDALONE)
#define NT_LOG_PRINT(MODNAME,LVL,msg,...) {\
                        printf(msg, ##__VA_ARGS__); \
                        printf("\n");\
                        }
#define nt_osal_calloc(count, size) calloc(count, size)
#define nt_osal_free_memory(ptr) free(ptr)
#else
#include <nt_logger_api.h>
#include <nt_osal.h>
#include <timer.h>
#include "hal_int_sys.h"
#endif

#ifdef _WIN32
// for Q5 Windows build
#define phyrf_hal_delay_us(ms)  Sleep(ms)
#define phyrf_os_delay(ms)  Sleep(ms)
#else
// HAL Delay. Can be called after Timer Initialization
// Minimum Delay 10us
#if defined(HALMAC_UFW)
#define phyrf_hal_delay_us(ms)  nt_osal_delay(ms/1000)
#else
#define phyrf_hal_delay_us(us)  hres_timer_us_delay(us)
#endif
 // Thread safe delay. It should not be called before OS Initialization
#define phyrf_os_delay(ms)  nt_osal_delay(ms)
#endif

#define FFT_TOGGLE_DELAY_US  10
#define MAX_TIMEOUT_COUNT_FFT_TOGGLE  100

#define TX_CAL_MEASURE_DELAY_US 10
#define MAX_TIMEOUT_COUNT_TX_CAL_MEASURE  100

#define RXDCO_CAL_MEASURE_DELAY_US  10
#define MAX_TIMEOUT_COUNT_RXDCO_CAL_MEASURE  100

#define TONE_FREQ 12 // Tone Frequency (k) for TXLO, TXIQ, RXIQ Cal

#define RXGAINNUM 1
#define CXCRXGAINNUM 2
#define OTP_SP50_M15_LIMIT_SS 150
#define OTP_SP50_M15_LIMIT_FF 175
#define OTP_SP50_M15_LIMIT_TT (OTP_SP50_M15_LIMIT_SS+OTP_SP50_M15_LIMIT_FF)/2
#define OTP_SP50_M15_TURNPOINT_5G 167
#define DACBO_5G_SUBBAND_RANGE1 4900
#define DACBO_5G_SUBBAND_RANGE2 5295
#define DACBO_5G_SUBBAND_RANGE3 5745
#define DACBO_5G_SUBBAND_RANGE4 5900
#define DACBO_6G_SUBBAND_RANGE1 6045
#define DACBO_6G_SUBBAND_RANGE2 6195
/* Tx Power Mode */
typedef enum e_tx_power_mode {
    tx_power_mode_high, tx_power_mode_medium, tx_power_mode_low, tx_power_mode_very_low, num_tx_power_modes,
} tx_power_mode_t;

/* Calibration Mode */
typedef enum e_cal_mode {
// Updated as per Neutrino_RFA_RegMap_WLAN.xlsx 11/08/20
    cal_mode_dco = 0,       // Normal Tx/Rx, DTIM Rx
    cal_mode_tx_iq = 1,     // lb_iqcaltx
    cal_mode_rx_iq = 2,     // lb_iqcalrx
    cal_mode_dpd = 3,       // lp_dpd_por
    cal_mode_dpd_lna = 4    // lb_dpd_lna
} cal_mode_t;

/* Calibration Results */
typedef enum e_cal_result {
    cal_fail = -1, /* Calibration Failed */
    cal_ok = 0, /* Calibration OK. No Error  */
    cal_error_param = 1, /* Ivalid Paramaters */
    cal_error_range = 2, /* Out of range, Total number exceeds the set maximum */
    cal_error_no_memory = 3 /* Not enough memory, Memory allocation failed */
} cal_result_t;

/* Calibration Status */
typedef enum e_cal_status {
    cal_status_idle, cal_status_done, cal_status_busy, cal_status_err
} cal_status_t;

/* DAC Sampling Rate */
typedef enum e_dac_rate {
    dac_rate_60, dac_rate_120
} dac_rate_t;

typedef struct s_iq_data_16b {
    int16_t real_data;
    int16_t imag_data;
} iq_data_16b_t;

typedef struct s_iq_data_32b {
    int32_t real_data;
    int32_t imag_data;
} iq_data_32b_t;

typedef struct s_iq_data_u32b {
    uint32_t real_data;
    uint32_t imag_data;
} iq_data_u32b_t;

typedef struct cal_txiq_ps_s {
    uint8_t tx_gain_dict[num_tx_power_modes];
    uint8_t rx_gain_dict[num_tx_power_modes];
    uint8_t edet_tia;    // Transimpedance Amplifier Gain Control
    uint8_t edet_atten_dict[num_tx_power_modes];  // PDET Attenuation
    uint8_t edet_sq;     // Squarer Gain Control
} cal_txiq_ps_t;

typedef struct cal_dpd_ps_s {
    uint8_t tx_gain_dict[num_tx_power_modes];
    uint8_t rx_gain_dict[num_tx_power_modes];
    int8_t tx_dig_gain_dict[num_tx_power_modes];
    uint8_t loop_atten_dict[num_tx_power_modes];
} cal_dpd_ps_t;

typedef struct cal_rxiq_ps_s {
    uint8_t tx_gain_dict[num_tx_power_modes];
    uint8_t rx_gain_list[1];
    uint8_t rx_gain_list_hp[1];
    uint8_t loop_atten_dict[num_tx_power_modes];
} cal_rxiq_ps_t;

typedef struct cal_rxdco_ps_s {
    uint8_t tx_gain_dict[num_tx_power_modes];
    uint8_t rx_gain_list[RXGAINNUM];
    uint8_t rx_gain_list_hp[RXGAINNUM];
    uint8_t rx_gain_list_sharedlna[6][CXCRXGAINNUM];
} cal_rxdco_ps_t;

typedef union power_settings_u {
    cal_txiq_ps_t cal_txiq_ps;
    cal_dpd_ps_t cal_dpd_ps;
    cal_rxiq_ps_t cal_rxiq_ps;
    cal_rxdco_ps_t cal_rxdco_ps;
} power_settings_t;

/* TXLO Correction*/
typedef struct txlo_correction_s {
    int16_t i_lo_corr;
    int16_t q_lo_corr;
} txlo_correction_t;

typedef struct tx_dcoc_range_s
{
	uint8_t dcoc_range_i;
	uint8_t dcoc_range_q;
} tx_dcoc_range_t;

typedef struct trim_input
{
	uint32_t     PMU_MBias_IC_Code;
	uint32_t     Calibrations_LDO_DT_0p8_Code;
	uint32_t     Calibrations_LDO_MBIAS_1p6_Code;
	uint32_t     Calibrations_LDO_XO_0p7_Code;
	uint32_t     Calibrations_LDO_LO_0p8_Code;
	uint32_t     Calibrations_LDO_Syn0p7_Code;
	uint32_t     Calibrations_LDO_VCO0p7_Code;
	uint32_t     Calibrations_LDO_FE0p8_0_Code;
	uint32_t     Calibrations_LDO_FE0p8_4_1_Code;
	uint32_t     Calibrations_LDO_FE0p8_Code;
	uint32_t     Calibrations_LDO_BB0p8_Code;
	uint32_t     Calibrations_LDO_MS0p8_ADDA_Code;
	uint32_t     Top_MBIAS_i_ic_with_ldo_1p6_sel_TTR_CD;
	uint32_t     Top_MBIAS_v_ir_with_ldo_1p6_sel_TTR_CD;
	uint32_t     Top_MBIAS_i_ip_with_ldo_1p6_sel_TTR_CD;
	uint32_t     CheckMBias_R_int1_Err_Code;
	uint32_t     PMU_AON_LDO;
	uint32_t     PMU_CX_LDO;
	uint32_t     PMU_LF_RC_OSC_32K;
	uint32_t     SYNTH_dcc_vrefop;
	uint32_t     tsens_mon_vcm_trim;
	uint32_t     PMU_ts_Offset;
	uint32_t     LO_POLYPHASE_CBANK_00;
	uint32_t     LO_POLYPHASE_CBANK_01_2_0;
	uint32_t     LO_POLYPHASE_CBANK_01_6_3;
	uint32_t     LO_POLYPHASE_CBANK_01;
	uint32_t     LO_POLYPHASE_CBANK_02;
	uint32_t     LO_POLYPHASE_CBANK_03;
	uint32_t     LO_POLYPHASE_CBANK_04;
	uint32_t     LO_POLYPHASE_CBANK_05;
	uint32_t     LO_POLYPHASE_CBANK_06;
	uint32_t     LO_POLYPHASE_CBANK_07;
	uint32_t     LO_POLYPHASE_CBANK_08;
	uint32_t     LO_POLYPHASE_CBANK_09;
	uint32_t     LO_POLYPHASE_CBANK_10_3_0;
	uint32_t     LO_POLYPHASE_CBANK_10_6_4;
	uint32_t     LO_POLYPHASE_CBANK_10;
	uint32_t     LO_POLYPHASE_CBANK_11;
	uint32_t     LO_POLYPHASE_CBANK_12;
	uint32_t     LO_POLYPHASE_CBANK_13;
	uint32_t     LO_POLYPHASE_CBANK_14;
	uint32_t     LO_POLYPHASE_CBANK_15_0;
	uint32_t     LO_POLYPHASE_CBANK_15_6_1;
	uint32_t     LO_POLYPHASE_CBANK_15;
	uint32_t     TX_VCM_TXBBF_TIA_VICMADJ;
	uint32_t     TX_VCM_TXBBF_TIA_VICMADJQ;
	uint32_t     TX_VCM_TXBBF_VSEL_VCM;
	uint32_t     DPD_VCM_TX_TIA_VICMADJ_0;
	uint32_t     DPD_VCM_TX_TIA_VICMADJ_2_1;
	uint32_t     DPD_VCM_TX_TIA_VICMADJ;
	uint32_t     DPD_VCM_TX_TIA_Q_VICMADJ;
	uint32_t     RX_BQ_VCM_SEL;
	uint32_t     DAC_ICM_BP_I_0;
	uint32_t     DAC_ICM_BP_I_5_1;
	uint32_t     DAC_ICM_BP_I;
	uint32_t     DAC_ICM_BP_Q;
	uint32_t     DAC_IOP_IOM_I;
	uint32_t     DAC_IOP_IOM_Q;
	uint32_t     DAC_STATIC_CONFIG0_Rest;
	uint32_t     DAC_STATIC_CONFIG0_All;
} __ATTRIB_PACK TRIM_INPUT;


#define FFT_RAM0_Count (HWIO_FFT_FFT_FFT_RAM_0n_MAXn + 1)
#define CAL_BARAM_Count (HWIO_CAL_REG_1RX_PRONTO_CAL_BARAMn_MAXn +1)

#define RFIF_RX_GAINn_Count (HWIO_RFIF_RFIF_RX_GAINn_MAXn + 1)
#define SHARED_LNA_Count 90
#define NON_SHARED_LNA_Count 80
#define BLOCKER_GAIN_TABLE_OFFSET 0xA0
#define SHARED_LNA_RX_GAIN_TABLE_OFFSET 0x140
#define RX_PRONTO_CAL_IQ_CORR_COEFF_MEM_Count (HWIO_CAL_REG_1RX_PRONTO_CAL_IQ_CORR_COEFF_MEMn_MAXn + 1)

#define BIT_REVERSE_SHIFT_RANGE 6u
#ifndef PHYDEVLIB_IMAGE_STANDALONE
#define abs(x)  (((x)<0 )? (-x) : (x))
#endif
#define sign(x) (((x)<0 )? -1 : (((x) > 0) ? 1 : 0))

#ifndef _WIN32
#define min(a,b) (((a)<(b))?(a):(b))
#define max(a,b) (((a)>(b))?(a):(b))
#endif

void get_power_settings(uint8_t band_code, cal_mode_t mode, power_settings_t *ps_p);

void set_rx_gain(uint8_t rx_gain);
#if 0
void set_tx_gain(uint32_t rf_gain, uint8_t dig_atten);
#endif

void strobe_tx_gain(void);
void strobe_rx_gain(void);
void reset_rx_gain(void);

void set_cal_mode(cal_mode_t cal_mode);

uint8_t un_bit_reverse_idx(int8_t idx);

uint32_t demote_to_nbits(int32_t data, uint8_t n_bits);
int32_t sign_extend_dword(uint32_t data, uint8_t n_bits);

void measure_rxdco_and_rxpwr(int16_t *dc_offset_i, int16_t *dc_offset_q, int16_t *power_i,
        int16_t *power_q);

void write_edet_dco(int16_t hg_ovd, int16_t lg_ovd);
void read_edet_dco(int16_t *hg_ovd, int16_t *lg_ovd);

void read_dco_lut(uint8_t dco_lut_idx, int16_t *dc_i, int16_t *dc_q, bool override, bool ifTest,
        bool ifEdet);
void write_dco_lut(uint8_t dco_lut_idx, int16_t dc_i, int16_t dc_q, bool override, bool ifTest,
        bool ifEdet);
void write_dco_lut_from_caldb(uint8_t dco_lut_idx, uint32_t value_dcoc_res, bool crx_enable);

void read_dco_lut_to_caldb(uint8_t dco_lut_idx, int16_t *dc_i, int16_t *dc_q);

void write_iq_coef(int32_t ampimb, int32_t phsi, int32_t phsq, dac_rate_t dacrate);
void read_iq_coef(int32_t *ampimb, int32_t *phsi, int32_t *phsq, dac_rate_t dacrate);

void bb_tx_gain_override(uint32_t rf_gain_word, int8_t dig_gain_idx);

void output_baram_content(iq_data_16b_t *tone_all);

void cal_utils_toggle_fft(void);
int phyrf_get_param(int param_id, void* p_in, void** p_out);
int phyrf_set_param(int param_id, int value, void* p_in, void* p_out);



cal_status_t cal_utils_wait_for_measure(void);

void cal_utils_get_tone(int8_t tone_idx, iq_data_16b_t *tona_val, int32_t *tone_pwr);

void test_read_dco(bool uselo, int16_t *icorr, int16_t *qcorr);
void test_write_dco(bool uselo, int16_t icorr, int16_t qcorr);
void read_lo_coef(int16_t *corr_i, int16_t *corr_q, tx_dcoc_range_t *dcoc_range, bool ifAnalog);
void write_lo_coef(int32_t corr_i, int32_t corr_q, tx_dcoc_range_t dcoc_range, bool ifAnalog, bool override);

uint32_t form_tx_gain_word(uint16_t rf_gain_word, int32_t txlo_i, int32_t txlo_q,
        uint8_t odac_range);
void load_tx_gainlut(uint8_t band_code,uint8_t tx_gain_id, int8_t tx_dig_gain);
void phyrf_load_rx_gain_lut(uint8_t band_code, uint16_t mhz);
int32_t signmag2twoscomp(uint32_t data, uint8_t n_bits);
int32_t twoscomp2signmag(int32_t data, uint8_t n_bits);
void set_finegain_offset(uint8_t fgoffset11a, uint8_t fgoffset11b, uint8_t fgoffset11n);
uint8_t phyrf_get_process_monitor_chiptype(void);
uint16_t chip_version_otp_read();
uint16_t otp_version_otp_read();
void PHYRF_REG_WR_SCRIPTS(const uint32_t TBL[][2], int count);
uint32_t PHYRF_REGFLD_RD(uint32_t addr, uint32_t lsb, uint32_t mask);
void PHYRF_REGFLD_WR(uint32_t addr, uint32_t data, uint32_t lsb, uint32_t mask);
void phyrf_temperature_data_otp_trim(void);
void phyrf_temperature_data_config(void);
int32_t phyrf_temperature_data_get(void);
uint32_t phyrf_pmu_ts_data_get(void);
int8_t phyrf_rssi_correction(int8_t input_rssi, uint8_t band);
int8_t phyrf_tpc_apply_dac_bo(uint16_t dac_bo_nom, uint16_t chan, uint8_t band, bool dac_bo_enabled);
void phyrf_rf_bandedge_disable();
void phyrf_rf_bandedge_enable();
void phyrf_rf_bandedge_adjust();
void rctune_compose_regfld(uint32_t *data, uint32_t value, uint32_t SHIFT, uint32_t MASK);
uint32_t rctune_extract_regfld(uint32_t data, uint32_t SHIFT, uint32_t MASK);
#if defined(HALMAC_UFW)
void phyrf_rx_set_rssi_offset(int8_t rssiOffset, int8_t rssiOffset_lgt);
#endif
typedef enum phyrf_param_id_t
{
    phyrf_param_id_xLNA5G,
    phyrf_param_id_xLNA6G,
    phyrf_param_id_COEX,
    phyrf_param_id_tpcMode,
    phyrf_param_id_pwrOffset,
    phyrf_param_id_txPowerMode,
    phyrf_param_id_bdfPointer,
    phyrf_param_id_lnaPwrMode,
    phyrf_param_id_neutrino_xpa,
    phyrf_param_id_BOOTSEQ_EXECUTED,
    phyrf_param_id_current_bandcode,
    phyrf_param_id_current_freq,
    phyrf_param_id_bandedge_enable,
} phyrf_param_id_t;

typedef enum phyrf_param_onoff_t
{
    phyrf_param_off = 0,
    phyrf_param_on = 1,
    phyrf_param_disable = 0,
    phyrf_param_enable = 1,
} phyrf_param_onoff_t;

typedef enum phyrf_param_tpcmode_t
{
    phyrf_param_fixedgain = 0,
    phyrf_param_scpc = 1,
    phyrf_param_clpc = 2,

} phyrf_param_tpcmode_t;

typedef enum phyrf_param_lnamode_t
{
    phyrf_param_lnamode_low_power = 0,
    phyrf_param_lnamode_high_perf = 1,
    phyrf_param_lnamode_xlna = 2,
    phyrf_param_lnamode_max
} phyrf_param_lnamode_t;

typedef struct phyrf_cal_globol_flag_t
{
    uint32_t phyrf_current_freq : 16;
    uint32_t phyrf_current_bandcode : 2;
    uint32_t phyrf_xLNA5G : 1;
    uint32_t phyrf_xLNA6G : 1;
    uint32_t phyrf_coexEnable2G : 1;
    uint32_t phyrf_tpcMode : 2;
    uint32_t phyrf_txPowerMode : 2;
    uint32_t phyrf_pwrOffset : 5;
    uint32_t phyrf_lnaPwrMode : 2; // 0:LP, 1:HP
    uint32_t phyrf_neutrino_xpa : 2;
    uint32_t phyrf_bootseq_executed : 1;  // 0: False, 1: True, default is 0
    uint32_t phyrf_bandedge_enable : 1;
} phyrf_cal_globol_flag_t;

typedef enum phyrf_chiptype_t 
{
	phyrf_chiptype_ffl=0,
	phyrf_chiptype_tt=1,
	phyrf_chiptype_ssh=2,
  phyrf_chiptype_unknown=3
} phyrf_chiptype_t;

#endif /* _HALPHY_CAL_UTILS_H_ */
