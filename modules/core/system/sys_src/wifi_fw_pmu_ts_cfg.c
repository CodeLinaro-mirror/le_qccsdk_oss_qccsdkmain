/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/**********************************************************************************************
 * @file wifi_fw_pmu_ts_cfg.c
 * @brief PMU Temperature Sensor Configurations and Trims
 *
 *
 *********************************************************************************************/

/*----------------------------------------------------------------------------
 * Include Files
 * --------------------------------------------------------------------------*/

#include "fwconfig_cmn.h"
#ifdef PMU_TS_CONFIGURATION

#include <stdio.h>
#include <ctype.h>
#include "wifi_fw_pmu_ts_cfg.h"
#include "nt_common.h"
#include "HALhwio.h"
#include "Fermion_seq_hwioreg.h"
#include "fermion_hw_reg.h"
#include "nt_logger_api.h"
#include "hal_int_sys.h"
#include "wifi_fw_pwr_cb_infra.h"
#include "phyCalUtils.h"
#include "ferm_hkadc_drv.h"
#include "printfext.h"

/*-----------------------------------------------------------------------------
 * Global Data Definitions
 *----------------------------------------------------------------------------*/

pmu_ts_param_t g_pmu_ts_struct;

/*-----------------------------------------------------------------------------
 * Externalized Function Definitions
 *----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
 * @function  : pmu_ccpu_temp_mon_done_intr
 * @brief     : Interuppt handler for pmu_ccpu_temp_mon_done_intr
 * @param     : None
 * @return    : None
 *-----------------------------------------------------------------------------
 */
void __attribute__((section(".after_ram_vectors"))) pmu_ccpu_temp_mon_done_intr(void)
{
    uint32_t rdata;
    pmu_ts_param_t *ps_pmu_ts = &g_pmu_ts_struct;

    if ((ps_pmu_ts->pmu_ts_configured) && (ps_pmu_ts->pmu_ts_meas_mode == PERIODIC)) {
        ps_pmu_ts->pmu_ts_data_valid = hkadc_get_temp_raw_data(&rdata);
        if (ps_pmu_ts->pmu_ts_data_valid) {
            ps_pmu_ts->pmu_ts_prev_valid_raw_data = rdata;
            ps_pmu_ts->pmu_ts_data_update_time = hres_timer_curr_time_ms();
        }
        hkadc_auto_vbat_monitor_enable();
        // printf("Tirq valid=%d Traw=%d %dms\n", ps_pmu_ts->pmu_ts_data_valid, ps_pmu_ts->pmu_ts_prev_valid_raw_data,
        // ps_pmu_ts->pmu_ts_data_update_time);
    }
}

void __attribute__((section(".after_ram_vectors"))) pmu_ccpu_vbat_mon_done_intr(void)
{
    uint32_t rdata;
    pmu_ts_param_t *ps_pmu_ts = &g_pmu_ts_struct;

    if ((ps_pmu_ts->pmu_ts_configured) && (ps_pmu_ts->pmu_ts_meas_mode == PERIODIC)) {
        ps_pmu_ts->pmu_vbat_data_valid = hkadc_get_vbat_raw_data(&rdata);
        if (ps_pmu_ts->pmu_vbat_data_valid) {
            ps_pmu_ts->pmu_vbat_prev_valid_raw_data = rdata;
            ps_pmu_ts->pmu_vbat_data_update_time = hres_timer_curr_time_ms();
        }
        hkadc_auto_temp_monitor_enable();
        // printf("Virq valid=%d Traw=%d %dms\n", ps_pmu_ts->pmu_vbat_data_valid,
        // ps_pmu_ts->pmu_vbat_prev_valid_raw_data, ps_pmu_ts->pmu_vbat_data_update_time);
    }
}

/*-----------------------------------------------------------------------------
 * @function  : pmu_ts_configure
 * @brief     : Configure PMU TS for periodic temperature monitoring and enable HKADC
 * @param     : None
 * @return    : None
 *-----------------------------------------------------------------------------
 */
void pmu_ts_configure(void)
{
    pmu_ts_configure_periodic_meas();
}
void pmu_ts_configure_periodic_meas(void)
{
    if (g_pmu_ts_struct.pmu_ts_configured) {
        return;
    }

    hkadc_auto_temp_monitor_enable();
    hkadc_temp_monitor_done_sys_intr_enable(true);
    hkadc_vbat_monitor_done_sys_intr_enable(true);

    g_pmu_ts_struct.pmu_ts_configured = true;
    g_pmu_ts_struct.pmu_ts_meas_mode = PERIODIC;

    return;
}

/*-----------------------------------------------------------------------------
 * @function  : invalidate_pmu_ts_configuration
 * @brief     : Set g_pmu_ts_struct.pmu_ts_configured to false
 * @param     : None
 * @return    : None
 *-----------------------------------------------------------------------------
 */
void invalidate_pmu_ts_configuration(void)
{
    g_pmu_ts_struct.pmu_ts_configured = false;
    g_pmu_ts_struct.pmu_ts_meas_mode = ONE_TIME;
    hkadc_stop();
    hkadc_temp_monitor_done_sys_intr_enable(false);
    hkadc_vbat_monitor_done_sys_intr_enable(false);

    return;
}

/*-----------------------------------------------------------------------------
 * @function  : pmu_ts_get_raw_data
 * @brief     : Return the current value of PMU Temp Sensor reading
 * @param     : None
 * @return    : PMU TS data
 *-----------------------------------------------------------------------------
 */
uint32_t pmu_ts_get_raw_data(void)
{
    return g_pmu_ts_struct.pmu_ts_prev_valid_raw_data;
}

/*-----------------------------------------------------------------------------
 * @function  : pmu_ts_get_current_temperature
 * @brief     : Return current temperature in degrees from PMU TS raw data
 * @param     : None
 * @return    : Current temperature in degree celsius
 *-----------------------------------------------------------------------------
 */
int32_t pmu_ts_get_current_temperature(void)
{
    uint32_t pmu_reg_data;
    pmu_reg_data = pmu_ts_get_raw_data();
    return pmu_ts_convert_to_deg_cel(pmu_reg_data);
}

inline int32_t _sign_extend_dword(uint32_t data, uint8_t n_bits)
{
    const int32_t shift_bits = 32 - n_bits;
    return (((int32_t)data << shift_bits) >> shift_bits);
}

/*-----------------------------------------------------------------------------
 * @function  : pmu_ts_get_current_temperature
 * @brief     : Calculate current temperature from PMU TS raw data
 * @param     : pmu_reg_data -Raw TS data from PMU register
 * @return    : Current temperature in degree celsius
 *-----------------------------------------------------------------------------
 */
int32_t __attribute__((section(".__sect_ps_txt"))) pmu_ts_convert_to_deg_cel(uint32_t pmu_reg_data)
{
    int32_t ts_gain_otp_value = 0, ts_offset_residue = 0, temp_deg = 25;
    float ts_slope_meas, ts_raw_data;
    float ts_gain_err = -14.0f;
    uint8_t otp_version_high = 1;
    uint8_t otp_version_low = 0;

#if (FERMION_CHIP_VERSION == 1)
    otp_version_high = HWIO_INXF(
        SEQ_WCSS_OTP_OFFSET, FERMION_V1_0_QFPROM_RAW_FUSE_MAP_SECURITY_CONTROL_CORE_RAW_R_QFPROM_RAW_PTE_REGION_1_W3,
        TRIM_TAG_HIGH);
    otp_version_low = HWIO_INXF(SEQ_WCSS_OTP_OFFSET,
                                FERMION_V1_0_QFPROM_RAW_FUSE_MAP_SECURITY_CONTROL_CORE_RAW_R_QFPROM_RAW_PTE_REGION_1_W3,
                                TRIM_TAG_LOW);
#else  /* FERMION_CHIP_VERSION == 1 */
    otp_version_high = HWIO_INXF(
        SEQ_WCSS_OTP_OFFSET, FERMION_V2_0_QFPROM_RAW_FUSE_MAP_SECURITY_CONTROL_CORE_RAW_R_QFPROM_RAW_PTE_REGION_1_W3,
        TRIM_TAG_HIGH);
    otp_version_low = HWIO_INXF(SEQ_WCSS_OTP_OFFSET,
                                FERMION_V2_0_QFPROM_RAW_FUSE_MAP_SECURITY_CONTROL_CORE_RAW_R_QFPROM_RAW_PTE_REGION_1_W3,
                                TRIM_TAG_LOW);
#endif /* FERMION_CHIP_VERSION == 1 */

    if (otp_version_high >= OTP_V2)  // for OTP >= v2
    {
#if (FERMION_CHIP_VERSION == 1)
        ts_gain_otp_value =
            HWIO_INX(SEQ_WCSS_OTP_OFFSET,
                     FERMION_V1_0_QFPROM_RAW_FUSE_MAP_SECURITY_CONTROL_CORE_RAW_R_QFPROM_RAW_RF_CALIBRATION_ROW7_W3);
        ts_gain_otp_value =
            (ts_gain_otp_value &
             QWLAN_SECURITY_CONTROL_CORE_RAW_R_QFPROM_RAW_RF_CALIBRATION_ROW7_W3_TSENSOR_GAIN_ERROR_MASK) >>
            QWLAN_SECURITY_CONTROL_CORE_RAW_R_QFPROM_RAW_RF_CALIBRATION_ROW7_W3_TSENSOR_GAIN_ERROR_OFFSET;
        ts_gain_otp_value = _sign_extend_dword((uint32_t)ts_gain_otp_value, 8);
#else
        /* TSENDOR_OFFSET_RESIDUE field is added to only 2.0 version of chip */
        /* convert two's complement to signed */
        ts_gain_otp_value = _sign_extend_dword(
            (uint32_t)(HWIO_INXF(
                SEQ_WCSS_OTP_OFFSET,
                FERMION_V2_0_QFPROM_RAW_FUSE_MAP_SECURITY_CONTROL_CORE_RAW_R_QFPROM_RAW_RF_CALIBRATION_ROW7_W3,
                TSENSOR_GAIN_ERROR)),
            8);
        ts_offset_residue = _sign_extend_dword(
            (uint32_t)(HWIO_INXF(
                SEQ_WCSS_OTP_OFFSET,
                FERMION_V2_0_QFPROM_RAW_FUSE_MAP_SECURITY_CONTROL_CORE_RAW_R_QFPROM_RAW_RF_CALIBRATION_ROW8_W0,
                TSENSOR_OFFSET_RESIDUE)),
            8);
#endif
        ts_gain_err = (float)ts_gain_otp_value * TS_GAIN_RESOLUTION;
    }

    ts_raw_data = ((float)pmu_reg_data / 2.0f);  // 9 bit adc output convereted to 8 bit resolution
    ts_slope_meas = ((ts_gain_err * TS_SLOPE_IDEAL) / 100.0f) + TS_SLOPE_IDEAL;
    if (!ts_slope_meas) {
        NT_LOG_SOCPM_CRIT("TS slope is 0 - DIV by zero error", ts_gain_err, ts_gain_otp_value, 0);
        ts_slope_meas = TS_SLOPE_IDEAL;
    }
    temp_deg = (int32_t)(((ts_raw_data - (float)TS_IDEAL_DOUT_30C - (float)ts_offset_residue) / ts_slope_meas) +
                         TS_DOUT_REF_DEG);
    if (((otp_version_high == OTP_V3) && ((otp_version_low == 2) || (otp_version_low == 3))) ||
        (otp_version_high == OTP_V4) || ((otp_version_high == OTP_V5) && (otp_version_low == 0))) {
        temp_deg = temp_deg - TEMP_OUTPUT_OFFSET;  // Only needed for debug boards (otpv3p2, otpv3p3, otpv4, otpv5p0)
    }
    return temp_deg;
}
/*-----------------------------------------------------------------------------
 * @function  : pmu_ts_init
 * @brief     : Initializes the SW params and update the bootup temperature
 * @param     : None
 * @return    : None
 *-----------------------------------------------------------------------------
 */
void pmu_ts_init(void)

{
    uint32_t rdata, RAW_TS, start_time, stop_time, new_time;
    int32_t bootup_temp_deg;
    int32_t raw_vbat = 0;
    uint32_t vbatmV = 0;

    invalidate_pmu_ts_configuration();

    hkadc_set_auto_monitor_interval(us2xocnt(CONFIG_PMU_TS_MON_PERIOD_US));
    // g_pmu_ts_struct.pmu_SMPS2_ONESHOT_TRIM = ulpsmps2_get_oneshot();

    g_pmu_ts_struct.pmu_ts_data_valid = false;
    g_pmu_ts_struct.pmu_ts_prev_valid_raw_data = PMU_TS_RAW_25C;
    start_time = (uint32_t)hres_timer_curr_time_us();
    g_pmu_ts_struct.pmu_ts_data_update_time = 0;

    // Enable HKADC for temperature monitoring
    rdata = HAL_REG_RD(QWLAN_PMU_CFG_HKADC_DATA_AVG_CNT_REG);
    rdata &= ~QWLAN_PMU_CFG_HKADC_DATA_AVG_CNT_CFG_TEMP_VBATT_MON_SEL_MASK;
    HAL_REG_WR(QWLAN_PMU_CFG_HKADC_DATA_AVG_CNT_REG, rdata);

    rdata = HAL_REG_RD(QWLAN_PMU_CFG_ACAL_VBAT_MON_EN_REG);
    HAL_REG_WR(QWLAN_PMU_CFG_ACAL_VBAT_MON_EN_REG, (rdata | QWLAN_PMU_CFG_ACAL_VBAT_MON_EN_TEMP_MON_EN_MASK));
#if (FERMION_CHIP_VERSION == 1)
    if (HWIO_INXF(SEQ_WCSS_OTP_OFFSET,
                  FERMION_V1_0_QFPROM_RAW_FUSE_MAP_SECURITY_CONTROL_CORE_RAW_R_QFPROM_RAW_PTE_REGION_1_W3,
                  TRIM_TAG_HIGH) >= OTP_V1)
#else
    if (HWIO_INXF(SEQ_WCSS_OTP_OFFSET,
                  FERMION_V2_0_QFPROM_RAW_FUSE_MAP_SECURITY_CONTROL_CORE_RAW_R_QFPROM_RAW_PTE_REGION_1_W3,
                  TRIM_TAG_HIGH) >= OTP_V2)
#endif
    {
        do {
            RAW_TS = HAL_REG_RD(QWLAN_PMU_TEMP_SNR_RD_DATA_REG);
            new_time = (uint32_t)hres_timer_curr_time_us();
        } while (((RAW_TS & QWLAN_PMU_TEMP_SNR_RD_DATA_TC_MEASURED_DATA_VALID_MASK) == 0) &&
                 ((new_time - start_time) < TEMP_MEAS_TIMEOUT_US));
        if ((RAW_TS & QWLAN_PMU_TEMP_SNR_RD_DATA_TC_MEASURED_DATA_VALID_MASK) != 0) {
            g_pmu_ts_struct.pmu_ts_data_valid = true;
            g_pmu_ts_struct.pmu_ts_prev_valid_raw_data = RAW_TS & QWLAN_PMU_TEMP_SNR_RD_DATA_TC_MEASURED_DATA_MASK;
            g_pmu_ts_struct.pmu_ts_data_update_time = start_time / 1000;
        }
    }
    stop_time = (uint32_t)hres_timer_curr_time_us();
    bootup_temp_deg = pmu_ts_get_current_temperature();
    NT_LOG_PRINT(SOCPM, WARN,
                 "Bootup temperature %dC, PMU_TS raw data %d, time taken to measure temperature %dus, is Valid %d",
                 bootup_temp_deg, g_pmu_ts_struct.pmu_ts_prev_valid_raw_data, (stop_time - start_time),
                 g_pmu_ts_struct.pmu_ts_data_valid);

    g_pmu_ts_struct.pmu_vbat_data_valid = false;
    g_pmu_ts_struct.pmu_vbat_data_update_time = 0;
    g_pmu_ts_struct.pmu_vbat_prev_valid_raw_data = PMU_VBAT_TYPICAL_DEFAULT;
    start_time = (uint32_t)hres_timer_curr_time_us();
    raw_vbat = hkadc_single_vbat_monitor_get_raw();
    if (raw_vbat >= 0) {
        g_pmu_ts_struct.pmu_vbat_data_valid = true;
        g_pmu_ts_struct.pmu_vbat_prev_valid_raw_data = (uint32_t)raw_vbat;
        g_pmu_ts_struct.pmu_vbat_data_update_time = start_time / 1000;
        vbatmV = hkadc_vbat_raw2mV((uint32_t)raw_vbat);
    }
    stop_time = (uint32_t)hres_timer_curr_time_us();
    printf("Bootup vbat=%dmV raw=%d time=%dus valid=%d\n", vbatmV, g_pmu_ts_struct.pmu_vbat_prev_valid_raw_data,
           (stop_time - start_time), g_pmu_ts_struct.pmu_vbat_data_valid);

    hkadc_stop();

    fpci_evt_cb_reg((ps_evt_cb_t)&pmu_ts_power_state_change_cb,
                    PWR_EVT_WMAC_PRE_SLEEP | PWR_EVT_WMAC_POST_AWAKE | PWR_EVT_WMAC_SLEEP_ABORT,
                    PS_CALLBACK_PMU_TS_PRIORITY, NULL);
    return;
}

/*-----------------------------------------------------------------------------
 * @function  : is_pmu_ts_data_valid
 * @brief     : Return true if pmu ts data valid bit is set else return false
 * @param     : None
 * @return    : g_pmu_ts_struct.pmu_ts_data_valid
 *-----------------------------------------------------------------------------
 */
bool is_pmu_ts_data_valid(void)
{
    return (bool)g_pmu_ts_struct.pmu_ts_data_valid;
}

/*-----------------------------------------------------------------------------
 * @function  : is_pmu_ts_configured
 * @brief     : Return true if pmu ts is configured else return false
 * @param     : None
 * @return    : g_pmu_ts_struct.pmu_ts_configured
 *-----------------------------------------------------------------------------
 */
bool is_pmu_ts_configured(void)
{
    return (bool)g_pmu_ts_struct.pmu_ts_configured;
}

uint32_t tv_monitor_get_vbat_raw_data(void)
{
    return g_pmu_ts_struct.pmu_vbat_prev_valid_raw_data;
}

bool is_tv_monitor_vbat_data_valid(void)
{
    return (bool)g_pmu_ts_struct.pmu_vbat_data_valid;
}

uint32_t tv_monitor_get_vbat_mV(void)
{
    uint32_t pmu_reg_data = tv_monitor_get_vbat_raw_data();
    return hkadc_vbat_raw2mV(pmu_reg_data);
}

void tv_monitor_dump(const char *title)
{
    pmu_ts_param_t *ps_pmu_ts = &g_pmu_ts_struct;

    if (title) {
        printf("%s\n", title);
    }

    printf("pmu_ts_configured=%d\n", ps_pmu_ts->pmu_ts_configured);
    printf("pmu_ts_meas_mode=%d\n", ps_pmu_ts->pmu_ts_meas_mode);
    printf("pmu_ts_data_valid=%d pmu_ts_data_update_time=%dms pmu_ts_prev_valid_raw_data=%d %dC\n",
           ps_pmu_ts->pmu_ts_data_valid, ps_pmu_ts->pmu_ts_data_update_time, ps_pmu_ts->pmu_ts_prev_valid_raw_data,
           pmu_ts_convert_to_deg_cel(ps_pmu_ts->pmu_ts_prev_valid_raw_data));
    printf("pmu_vbat_data_valid=%d pmu_vbat_data_update_time=%dms pmu_vbat_prev_valid_raw_data=%d %dmV\n",
           ps_pmu_ts->pmu_vbat_data_valid, ps_pmu_ts->pmu_vbat_data_update_time,
           ps_pmu_ts->pmu_vbat_prev_valid_raw_data, hkadc_vbat_raw2mV(ps_pmu_ts->pmu_vbat_prev_valid_raw_data));
}

#ifdef FEATURE_FPCI
/**
 * @brief  Presleep/ Postawake Callbacks for PMU TS
 * @param  evt - Denotes the sleep event
 * @return None
 */
void pmu_ts_power_state_change_cb(uint8_t evt, void *p_args)
{
    (void)p_args;
    if (evt == PWR_EVT_WMAC_PRE_SLEEP) {
        invalidate_pmu_ts_configuration();
        g_pmu_ts_struct.pmu_dtim_next_update_ts = true;  // sleep will update temp first
        presleep_update_ulpsmps2_oneshot();
    } else if ((evt == PWR_EVT_WMAC_POST_AWAKE) || (evt == PWR_EVT_WMAC_SLEEP_ABORT)) {
        hkadc_set_auto_monitor_interval(us2xocnt(CONFIG_PMU_TS_MON_PERIOD_US));
        pmu_ts_configure_periodic_meas();
    }
    return;
}
#endif /* FEATURE_FPCI */

#define DEBUG_ONESHOT_WRITE_READ_IN_SLEEP 0

bool g_presleep_update_ulpsmps2_oneshot_enable = true;

uint32_t dbg_oneshot_update_cnt = 0;
uint32_t dbg_sleep_vbat_ready_cnt = 0;
uint32_t dbg_sleep_temp_ready_cnt = 0;
#if DEBUG_ONESHOT_WRITE_READ_IN_SLEEP
uint32_t dbg_oneshot_update_temp1_raw = 0;
uint32_t dbg_oneshot_update_temp2_raw = 0;
int32_t dbg_oneshot_update_temp1 = 0;
int32_t dbg_oneshot_update_temp2 = 0;
uint32_t dbg_oneshot_update_vbat1 = 0;
uint32_t dbg_oneshot_update_vbat2 = 0;
uint32_t dbg_oneshot_update_cur_oneshot1 = 0;
uint32_t dbg_oneshot_update_cur_oneshot2 = 0;
uint32_t dbg_oneshot_update_cur_oneshot3 = 0;
uint32_t dbg_oneshot_update_cur_oneshot4 = 0;
uint32_t dbg_oneshot_update_new_oneshot1 = 0;
uint32_t dbg_oneshot_update_new_oneshot2 = 0;
uint32_t dbg_oneshot_update_new_oneshot3 = 0;
uint32_t dbg_oneshot_update_new_oneshot4 = 0;
uint32_t dbg_oneshot_update_final_oneshot1 = 0;
uint32_t dbg_oneshot_update_final_oneshot2 = 0;
#endif

void __attribute__((section(".__sect_ps_txt"))) dtim_tv_monitor_trigger(void)
{
    if (g_pmu_ts_struct.pmu_dtim_next_update_ts) {
        hkadc_single_temp_monitor_enable();
    } else {
        hkadc_single_vbat_monitor_enable();
    }
}

void __attribute__((section(".__sect_ps_txt"))) dtim_tv_monitor_poll(void)
{
    bool data_ready = false;
    uint32_t data;
    pmu_ts_param_t *ps_pmu_ts = &g_pmu_ts_struct;

    if (ps_pmu_ts->pmu_dtim_next_update_ts) {
        data_ready = hkadc_get_temp_raw_data(&data);
        if (data_ready) {
            ps_pmu_ts->pmu_ts_prev_valid_raw_data = data;
            ps_pmu_ts->pmu_dtim_ts_data_valid = true;
            dbg_sleep_temp_ready_cnt++;
        }
    } else {
        data_ready = hkadc_get_vbat_raw_data(&data);
        if (data_ready) {
            ps_pmu_ts->pmu_vbat_prev_valid_raw_data = data;
            ps_pmu_ts->pmu_dtim_vbat_data_valid = true;
            dbg_sleep_vbat_ready_cnt++;
        }
    }
    ps_pmu_ts->pmu_dtim_next_update_ts = !ps_pmu_ts->pmu_dtim_next_update_ts;
}

// sometimes oneshot is not accessable on DTIM, so add some management potential
void __attribute__((section(".__sect_ps_txt"))) dtim_tv_set_ulpsmps2_oneshot(uint32_t oneshot)
{
    ulpsmps2_set_oneshot(oneshot);
    // g_pmu_ts_struct.pmu_SMPS2_ONESHOT_TRIM = oneshot;
}

void dtim_tv_monitor_dump(const char *title)
{
    if (title) {
        info_printf("%s\n", title);
    }

    info_printf("g_presleep_update_ulpsmps2_oneshot_enable=%d\n", g_presleep_update_ulpsmps2_oneshot_enable);
    // info_printf("pmu_SMPS2_ONESHOT_TRIM=%d\n", ps_pmu_ts->pmu_SMPS2_ONESHOT_TRIM);
    info_printf("dbg_oneshot_update_cnt=%d\n", dbg_oneshot_update_cnt);
    info_printf("dbg_sleep_vbat_ready_cnt=%d\n", dbg_sleep_vbat_ready_cnt);
    info_printf("dbg_sleep_temp_ready_cnt=%d\n", dbg_sleep_temp_ready_cnt);
#if DEBUG_ONESHOT_WRITE_READ_IN_SLEEP
    info_printf("dbg_oneshot_update_temp1_raw=%d\n", dbg_oneshot_update_temp1_raw);
    info_printf("dbg_oneshot_update_temp2_raw=%d\n", dbg_oneshot_update_temp2_raw);
    info_printf("dbg_oneshot_update_temp1=%d\n", dbg_oneshot_update_temp1);
    info_printf("dbg_oneshot_update_temp2=%d\n", dbg_oneshot_update_temp2);
    info_printf("dbg_oneshot_update_vbat1=%d\n", dbg_oneshot_update_vbat1);
    info_printf("dbg_oneshot_update_vbat2=%d\n", dbg_oneshot_update_vbat2);
    info_printf("dbg_oneshot_update_cur_oneshot1=%d\n", dbg_oneshot_update_cur_oneshot1);
    info_printf("dbg_oneshot_update_cur_oneshot2=%d\n", dbg_oneshot_update_cur_oneshot2);
    info_printf("dbg_oneshot_update_cur_oneshot3=%d\n", dbg_oneshot_update_cur_oneshot3);
    info_printf("dbg_oneshot_update_cur_oneshot4=%d\n", dbg_oneshot_update_cur_oneshot4);
    info_printf("dbg_oneshot_update_new_oneshot1=%d\n", dbg_oneshot_update_new_oneshot1);
    info_printf("dbg_oneshot_update_new_oneshot2=%d\n", dbg_oneshot_update_new_oneshot2);
    info_printf("dbg_oneshot_update_new_oneshot3=%d\n", dbg_oneshot_update_new_oneshot3);
    info_printf("dbg_oneshot_update_new_oneshot4=%d\n", dbg_oneshot_update_new_oneshot4);
    info_printf("dbg_oneshot_update_final_oneshot1=%d\n", dbg_oneshot_update_final_oneshot1);
    info_printf("dbg_oneshot_update_final_oneshot2=%d\n", dbg_oneshot_update_final_oneshot2);
#endif
}

// CONFIG_ULP_SMPS2_ONTSHOT_OPTIMIZE is defined in Kconfig

void __attribute__((section(".__sect_ps_txt"))) presleep_update_ulpsmps2_oneshot(void)
{
#if CONFIG_ULP_SMPS2_ONTSHOT_OPTIMIZE
    pmu_ts_param_t *ps_pmu_ts = &g_pmu_ts_struct;
    uint32_t temp_raw = ps_pmu_ts->pmu_ts_prev_valid_raw_data;
    uint32_t final_oneshot = CX_ONESHOT_GOLDEN;
    uint32_t cur_oneshot = ulpsmps2_get_oneshot();
    // uint32_t cur_oneshot = ps_pmu_ts->pmu_SMPS2_ONESHOT_TRIM;
    int32_t tempC = 0;
    uint32_t vbatmV = 0;

    if (!g_presleep_update_ulpsmps2_oneshot_enable || !cur_oneshot) {
        // if cur_oneshot==0, oneshot cannot be write or read
        return;
    }

    if (temp_raw > PMU_TS_RAW_33C) {
        tempC = pmu_ts_convert_to_deg_cel(temp_raw);
        // tempC = hkadc_temp_raw2C(temp_raw);
        if (tempC > TEMPERATUREC_GOLDEN) {
            vbatmV = hkadc_vbat_raw2mV(g_pmu_ts_struct.pmu_vbat_prev_valid_raw_data);
            final_oneshot = ulpsmps2_get_optimized_oneshot(vbatmV, tempC, NULL, NULL);
        }
    }

    if (final_oneshot != cur_oneshot) {
        dbg_oneshot_update_cnt++;
        dtim_tv_set_ulpsmps2_oneshot(final_oneshot);
#if DEBUG_ONESHOT_WRITE_READ_IN_SLEEP
        uint32_t new_oneshot = 5;
        new_oneshot = ulpsmps2_get_oneshot();
        if (!dbg_oneshot_update_temp1_raw) {
            dbg_oneshot_update_temp1_raw = temp_raw;
        } else if (!dbg_oneshot_update_temp2_raw) {
            dbg_oneshot_update_temp2_raw = temp_raw;
        }
        if (!dbg_oneshot_update_temp1) {
            dbg_oneshot_update_temp1 = tempC;
        } else if (!dbg_oneshot_update_temp2) {
            dbg_oneshot_update_temp2 = tempC;
        }
        if (!dbg_oneshot_update_vbat1) {
            dbg_oneshot_update_vbat1 = vbatmV;
        } else if (!dbg_oneshot_update_vbat2) {
            dbg_oneshot_update_vbat2 = vbatmV;
        }
        if (!dbg_oneshot_update_cur_oneshot1) {
            dbg_oneshot_update_cur_oneshot1 = cur_oneshot;
        } else if (!dbg_oneshot_update_cur_oneshot2) {
            dbg_oneshot_update_cur_oneshot2 = cur_oneshot;
        } else if (!dbg_oneshot_update_cur_oneshot3) {
            dbg_oneshot_update_cur_oneshot3 = cur_oneshot;
        } else if (!dbg_oneshot_update_cur_oneshot4) {
            dbg_oneshot_update_cur_oneshot4 = cur_oneshot;
        }
        if (!dbg_oneshot_update_new_oneshot1) {
            dbg_oneshot_update_new_oneshot1 = new_oneshot;
        } else if (!dbg_oneshot_update_new_oneshot2) {
            dbg_oneshot_update_new_oneshot2 = new_oneshot;
        } else if (!dbg_oneshot_update_new_oneshot3) {
            dbg_oneshot_update_new_oneshot3 = new_oneshot;
        } else if (!dbg_oneshot_update_new_oneshot4) {
            dbg_oneshot_update_new_oneshot4 = new_oneshot;
        }
        if (!dbg_oneshot_update_final_oneshot1) {
            dbg_oneshot_update_final_oneshot1 = final_oneshot;
        } else if (!dbg_oneshot_update_final_oneshot2) {
            dbg_oneshot_update_final_oneshot2 = final_oneshot;
        }
#endif
    }
    return;
#else /* !CONFIG_ULP_SMPS2_ONTSHOT_OPTIMIZE */
    return;
#endif
}

#endif /* PMU_TS_CONFIGURATION */
