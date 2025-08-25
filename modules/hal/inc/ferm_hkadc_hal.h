/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __FERM_HKADC_HAL__
#define __FERM_HKADC_HAL__
#include "qccx.h"
#include <stdbool.h>

#define TEMPERATUREC_MIN    (-40)
#define TEMPERATUREC_MAX    (125)
#define TEMPERATUREC_ERR_N  (-5)
#define TEMPERATUREC_ERR_P  (5)
#define TEMPERATUREC_GOLDEN (40)

#define VBATMV_MIN   (1600)
#define VBATMV_MAX   (3600)
#define VBATMV_ERR_N (-50)
#define VBATMV_ERR_P (10)

#define CX_ONESHOT_GOLDEN (3)  // for temperature<=40C
#define CX_ONESHOT_MIN    (0)
#define CX_ONESHOT_MAX    (31)

#define PMU_TS_RAW_25C 212  // 25C
#define PMU_TS_RAW_30C 224  // 30C
#define PMU_TS_RAW_32C 229  // 32C
#define PMU_TS_RAW_33C 231  // 33C
#define PMU_TS_RAW_35C 236  // 35C
#define PMU_TS_RAW_40C 249  // 40C

#define PMU_VBAT_TYPICAL_DEFAULT 0x18a  // 3.3V

#define HKADC_READ_DATA(val, p_reg)  ((val) = *(p_reg))
#define HKADC_WRITE_DATA(val, p_reg) (*(p_reg) = (val))

/* XO tick is computed in terms of 38.4 Mhz.
 * Each tick is 1/38400000 s = 1000000/38400000 us = 10/384= 5/192 us
 * So number of ticks given the time in us is (time *192) /5 */
#define us2xocnt(slp_time_us) (((slp_time_us)*192) / 5)
#define xocnt2us(xocnt)       (((xocnt)*5) / 192)

extern PMU_BASE_pmu_Type *const p_hkadc_reg;
extern RPMU_BASE_rpmu_Type *const p_rpmu;

bool hkadc_get_temp_raw_data(uint32_t *temp_data);
bool hkadc_get_vbat_raw_data(uint32_t *vbat_data);
void hkadc_stop(void);
void hkadc_single_temp_monitor_enable(void);
void hkadc_auto_temp_monitor_enable(void);
void hkadc_single_vbat_monitor_enable(void);
void hkadc_auto_vbat_monitor_enable(void);
void hkadc_temp_monitor_done_sys_intr_enable(bool enable);
void hkadc_vbat_monitor_done_sys_intr_enable(bool enable);

int32_t hkadc_temp_raw2C(uint32_t raw);
int32_t hkadc_temp_raw2C_trimmed(uint32_t raw);
uint32_t hkadc_temp_C2raw(int32_t tempC);
uint32_t hkadc_vbat_raw2mV(uint32_t raw);
uint32_t hkadc_vbat_raw2mV_trimmed(uint32_t raw);
uint32_t hkadc_vbat_mV2raw(uint32_t mV);

inline void hkadc_set_auto_monitor_interval(uint32_t xo_clk_cnt)
{
    p_hkadc_reg->PMU_CFG_TEMP_MON_INTERVAL.reg = xo_clk_cnt;
}

inline uint32_t hkadc_get_auto_monitor_interval(void)
{
    return p_hkadc_reg->PMU_CFG_TEMP_MON_INTERVAL.reg;
}

uint32_t ulpsmps2_get_OTP_oneshot(void);
uint32_t ulpsmps2_get_oneshot(void);
void ulpsmps2_set_oneshot(uint32_t oneshot);
uint32_t ulpsmps2_get_optimized_oneshot(uint32_t cc_vbat_milivolt, int32_t temp_deg, uint32_t *p_OTP_oneshot,
                                        uint32_t *p_t_one_shot_ns);

#endif  //__FERM_HKADC_HAL__
