/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/**********************************************************************************************
 * @file wifi_fw_pmu_ts_cfg.h
 * @brief PMU Temperature Sensor Configurations and Trims
 *
 *
 *********************************************************************************************/

#ifndef _WIFI_FW_PMU_TS_CFG_H_
#define _WIFI_FW_PMU_TS_CFG_H_

/*-----------------------------------------------------------------------------
 * Include Files
 * ---------------------------------------------------------------------------*/
#include "fwconfig_cmn.h"
#ifdef PMU_TS_CONFIGURATION
#include <stdint.h>
#include <stdbool.h>

/*-----------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * ---------------------------------------------------------------------------*/
// moved to hkadc_hal
//#define PMU_TS_ROOM_TEMP_DEFAULT 212

// moved to Kconfig
//#define CONFIG_PMU_TS_MON_PERIOD_US 1000000 // 1 seconds

// OTP versions
#define OTP_V1 1
#define OTP_V2 2
#define OTP_V3 3
#define OTP_V4 4
#define OTP_V5 5

#if (FERMION_CHIP_VERSION == 1)
#define QWLAN_SECURITY_CONTROL_CORE_RAW_R_QFPROM_RAW_RF_CALIBRATION_ROW7_W3_TSENSOR_GAIN_ERROR_MASK   0xFF0000
#define QWLAN_SECURITY_CONTROL_CORE_RAW_R_QFPROM_RAW_RF_CALIBRATION_ROW7_W3_TSENSOR_GAIN_ERROR_OFFSET 0x10
#endif

// Moved to hkadc_hal
/* XO tick is computed in terms of 38.4 Mhz.
 * Each tick is 1/38400000 s = 1000000/38400000 us = 10/384= 5/192 us
 * So number of ticks given the time in us is (time *192) /5 */
//#define _SOCPM_US_TO_XO_TICK(slp_time_us) (((slp_time_us) * 192) / 5)

#define PS_CALLBACK_PMU_TS_PRIORITY 2
#define TEMP_MEAS_TIMEOUT_US        (10 * 1000)  // 10 ms

#define TS_GAIN_RESOLUTION 0.5f
#define TS_SLOPE_IDEAL     1.2424f  // 205/165 from PMU HPG
#define TS_DOUT_REF_DEG    30.0f    // Temperature at which chips are fused
#define TS_IDEAL_DOUT_30C  112      // Ideal PMU TS ADC output at 30 C
/* As part of VIFERMION-470, this offset has to be considered for debug chips
 * otpv3p2, otpv3p3, otp v4 and otpv5p0) and not for production chips(otpv5p1) */
#define TEMP_OUTPUT_OFFSET 5

/*-----------------------------------------------------------------------------
 * Function Declarations
 *----------------------------------------------------------------------------*/

uint32_t pmu_ts_get_raw_data(void);
int32_t pmu_ts_get_current_temperature(void);
void pmu_ts_configure_periodic_meas(void);
void pmu_ts_init(void);
bool is_pmu_ts_data_valid(void);
bool is_pmu_ts_configured(void);
void invalidate_pmu_ts_configuration(void);
void pmu_ts_power_state_change_cb(uint8_t evt, void *p_args);
int32_t pmu_ts_convert_to_deg_cel(uint32_t pmu_reg_data);
void pmu_ts_configure(void);

uint32_t tv_monitor_get_vbat_mV(void);
uint32_t tv_monitor_get_vbat_raw_data(void);
bool is_tv_monitor_vbat_data_valid(void);
void tv_monitor_dump(const char *title);

void dtim_tv_monitor_trigger(void);
void dtim_tv_monitor_poll(void);
void dtim_tv_set_ulpsmps2_oneshot(uint32_t oneshot);
void dtim_tv_monitor_dump(const char *title);
void presleep_update_ulpsmps2_oneshot(void);

/*-----------------------------------------------------------------------------
 * Type Declarations
 * ---------------------------------------------------------------------------*/

typedef enum pmu_ts_meas_mode_type {
    ONE_TIME = 0,
    PERIODIC = 1,
} pmu_ts_meas_mode_type_t;

typedef struct pmu_ts_param {
    uint32_t pmu_ts_data_update_time;     // Time in ms when temperature was last updated in SW
    uint32_t pmu_ts_prev_valid_raw_data;  // PMU TS RAW Data value in SW
    uint32_t pmu_ts_meas_mode : 6;        /* Indicates whether PMU TS is in ONE_TIME
                                            or PERIODIC temperature measurement modes*/
    uint32_t pmu_ts_data_valid : 1;       /* Indicates whether pmu_ts_prev_valid_raw_data
                                             is valid */
    uint32_t pmu_ts_configured : 1;       /* Indicates whether pmu_ts_prev_valid_raw_data
                                             is configured in PERIODIC Meas mode or not */
    uint32_t pmu_vbat_data_valid : 1;     /* Indicates whether pmu_vbat_prev_valid_raw_data is valid */
    uint32_t pmu_dtim_next_update_ts : 1; /* Indicates if ts will be updated in DTIM, else vbat */
    uint32_t pmu_dtim_ts_data_valid : 1;
    uint32_t pmu_dtim_vbat_data_valid : 1;
    // uint32_t    pmu_SMPS2_ONESHOT_TRIM : 6; /* save pmu_SMPS2_ONESHOT_TRIM when set */
    uint32_t pmu_vbat_data_update_time;     // Time in ms when VBAT was last updated in SW
    uint32_t pmu_vbat_prev_valid_raw_data;  // PMU VBAT RAW Data value in SW
} pmu_ts_param_t;

#endif /* PMU_TS_CONFIGURATION */
#endif /* _WIFI_FW_PMU_TS_CFG_H_ */
