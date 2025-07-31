/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*========================================================================
 *
 *
 * @file wlan_sleep_clk_cal.h
 * @brief Function definitions for Sleep Clock Calibration Feature
 *
 *========================================================================*/

#ifndef _WLAN_SLEEP_CLK_CAL_H_
#define _WLAN_SLEEP_CLK_CAL_H_

/*-----------------------------------------------------------------------------
 * Include Files
 * ---------------------------------------------------------------------------*/

#include "fwconfig_cmn.h"

#ifdef SLEEP_CLK_CAL_IN_ACTIVE_MODE

#include <stdint.h>
#include <stdbool.h>

/*-----------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * ---------------------------------------------------------------------------*/

#define NUM_HBIN_RANGE              17
#define MAX_NUM_HBIN                15
#define PS_CB_SLEEP_CAL_PRIORITY    5
#define INIT_SLP_CAL_POLL_PERIOD_MS 1000
#define HBIN_RANGE1_START           0
#define HBIN_RANGE2_START           6
#define HBIN_RANGE3_START           12
#define HBIN_TEMP_RANGE_START_OFFSET                                          \
    0 /* Sleep Clock Calibration to be supported for PMU TS value of 0 to 496 \
        (i.e) which covers the temperature range -20 to 85 degrees celsius */
#define HBIN_0_TO_5_RANGE                                                                      \
    0x3FFFFFFF /* max range of 31 for all hbins (i.e) PMU TS value from 0 to 31 falls in HBIN0 \
                      ,32 to 63 falls in HBIN1 and so on */
#define HBIN_6_TO_11_RANGE  0x3FFFFFFF
#define HBIN_12_TO_15_RANGE 0xFFFFF

#define HBIN_RANGE_BIT_OFFSET            5      /* number of bits to specify a hbin range */
#define REF_SLEEP_CLK_CNT                0x200  // sleep clock count for calibration
#define RS_VALUE                         0x6    // sleep clk count * RS Value = calibration interval
#define MIN_SLP_DURATION_FOR_SLP_CLK_CAL 20000  // 20 ms
#define TS_HYS_THRESH \
    2  // Temp sensor hysterisis threshold, to avoid frequent cal re-trigger when TS change only a little bit

/*-----------------------------------------------------------------------------
 * Type Declarations
 * ---------------------------------------------------------------------------*/

typedef enum slp_clk_cal_mode_e {
    DISABLE = 0,
    ACTIVE_MODE = 1,
    SLEEP_MODE = 2,
    MAX_SLEEP_CAL_MODE = 3,
} slp_clk_cal_mode_t;

typedef struct socpm_sleep_clk_cal_s {
#ifdef APPLY_SLEEP_CLK_CORRECTION
    uint32_t xocnt;                                 // xocnt taken from hbin after sleep clk calibration
    uint32_t refxocnt;                              // refxocnt calculated
    uint32_t hbin_range[NUM_HBIN_RANGE];            /* temperature range of HBINs,if TS raw data is between
                                                       hbin_range[0] & hbin_range[1], cal data is available in HBIN0*/
    uint32_t pmu_temp_sensor_data;                  // data from PMU TS
#endif                                              /* APPLY_SLEEP_CLK_CORRECTION */
    uint32_t sleep_clk_cal_timer_pending_ticks;     // pending sleep clock poll timer duration for expiry before pausing
                                                    // the timer at sleep entry
    nt_osal_timer_handle_t slp_clk_cal_poll_timer;  // nt timer on whose expiry sleep clk calibration is triggered
    uint32_t slp_clk_cal_poll_period;               // periodicity of sleep clock cal poll timer
#ifdef APPLY_SLEEP_CLK_CORRECTION
    uint8_t prev_hbin;  // Holds the prev hbin. If current temperature falls in same hbin do not trigger cal
#endif                  /* APPLY_SLEEP_CLK_CORRECTION */
    bool sleep_clk_cal_initialized;  // bit to indicate if sleep clock calibration is initialized
#ifdef SLEEP_CLK_CAL_IN_SLEEP_MODE
    bool sleep_mode_cal_enabled;                  // bit to indicate whether sleep mode calibration enabled/not
#endif                                            /* SLEEP_CLK_CAL_IN_SLEEP_MODE */
    slp_clk_cal_mode_t slp_clk_cal_enabled_mode;  // this field indicates the Sleep Clock Calibration mode
} socpm_sleep_clk_cal_t;

/*-----------------------------------------------------------------------------
 * Function Declarations and Documentation
 * ---------------------------------------------------------------------------*/

void socpm_enable_slp_clk_cal_int(void);

nt_status_t socpm_slp_clk_cal_trigger(void);
nt_status_t socpm_slp_clk_cal_hw_init(void);
nt_status_t socpm_slp_clk_cal_init(void);
#ifdef APPLY_SLEEP_CLK_CORRECTION
nt_status_t socpm_slp_clk_cal_get_hbin(void);
#endif /* APPLY_SLEEP_CLK_CORRECTION */
nt_status_t socpm_slp_clk_cal_enable(slp_clk_cal_mode_t mode);

void socpm_sleep_clk_cal_timer_cb(void);
void socpm_actv_slp_clk_cal_slp_cb(uint8_t evt, void *p_args);
void socpm_actv_slp_clk_cal_monitor_pause(void);
void socpm_actv_slp_clk_cal_monitor_resume(void);

#ifdef SLEEP_CLK_CAL_IN_SLEEP_MODE
void socpm_slp_clk_cal_presleep_activites(uint64_t remaining_slp_time);
void socpm_slp_clk_cal_postawake_activities(void);
void socpm_slp_clk_cal_dynamic_slp_mode_cal_enable(bool en);
#endif /* SLEEP_CLK_CAL_IN_SLEEP_MODE */

#endif /* SLEEP_CLK_CAL_IN_ACTIVE_MODE */
#endif /* _WLAN_SLEEP_CLK_CAL_H_ */
