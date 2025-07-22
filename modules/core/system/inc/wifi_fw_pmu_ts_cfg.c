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

#include "wifi_fw_pmu_ts_cfg.h"
#include "nt_common.h"
#include "HALhwio.h"
#include "Fermion_seq_hwioreg.h"
#include "fermion_hw_reg.h"
#include "nt_logger_api.h"
#include "hal_int_sys.h"

/*-----------------------------------------------------------------------------
 * Global Data Definitions
 *----------------------------------------------------------------------------*/
uint32_t g_pmu_ts_prev_valid_raw_data = PMU_TS_ROOM_TEMP_DEFAULT;
bool g_pmu_ts_data_valid = false;
bool g_pmu_ts_configured = true;

/*-----------------------------------------------------------------------------
 * Externalized Function Definitions
 *----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
 * @function  : pmu_ts_enable_vbatt_temp_mon_done_int
 * @brief     : Enable pmu_ccpu_temp_mon_done_intr Interuupt //Device Specific 79
 * @param     : None
 * @return    : None
 *-----------------------------------------------------------------------------
 */
void pmu_ts_enable_vbatt_temp_mon_done_int(void)
{
    uint32_t temp1;
    temp1 = HAL_REG_RD(NT_SOCPM_NVIC_ISER2);
    temp1 = temp1 | (0x1 << 15);
    HAL_REG_WR(NT_SOCPM_NVIC_ISER2, temp1);
    return;
}

/*-----------------------------------------------------------------------------
 * @function  : pmu_ccpu_temp_mon_done_intr
 * @brief     : Interuppt handler for pmu_ccpu_temp_mon_done_intr
 * @param     : None
 * @return    : None
 *-----------------------------------------------------------------------------
 */
void __attribute__((section(".after_ram_vectors"))) pmu_ccpu_temp_mon_done_intr(void)
{
    uint32_t RAW_TS;
    if (((RAW_TS = HAL_REG_RD(QWLAN_PMU_TEMP_SNR_RD_DATA_REG)) & 0x80000000) != 0) {
        g_pmu_ts_data_valid = true;
        g_pmu_ts_prev_valid_raw_data = RAW_TS & QWLAN_PMU_TEMP_SNR_RD_DATA_TC_MEASURED_DATA_MASK;
    } else {
        g_pmu_ts_data_valid = false;
    }
    return;
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
    uint32_t rdata;

    // Enable HKADC for temperature monitoring
    rdata = HAL_REG_RD(QWLAN_PMU_CFG_HKADC_DATA_AVG_CNT_REG);
    rdata &= ~QWLAN_PMU_CFG_HKADC_DATA_AVG_CNT_CFG_TEMP_VBATT_MON_SEL_MASK;
    HAL_REG_WR(QWLAN_PMU_CFG_HKADC_DATA_AVG_CNT_REG, rdata);

    // Set periodicity of temperature monitoring in XO ticks
    HAL_REG_WR(QWLAN_PMU_CFG_TEMP_MON_INTERVAL_REG, _SOCPM_US_TO_XO_TICK(PMU_TS_MON_PERIOD_US));

    // Enable periodic temperature monitoring
    rdata = HAL_REG_RD(QWLAN_PMU_CFG_ACAL_VBAT_MON_EN_REG);
    rdata = rdata | QWLAN_PMU_CFG_ACAL_VBAT_MON_EN_AUTO_TEMP_MON_EN_MASK |
            QWLAN_PMU_CFG_ACAL_VBAT_MON_EN_CFG_TEMP_MON_DONE_INTR_EN_MASK;
    HAL_REG_WR(QWLAN_PMU_CFG_ACAL_VBAT_MON_EN_REG, rdata);

    pmu_ts_enable_vbatt_temp_mon_done_int();
    g_pmu_ts_configured = true;
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
    return g_pmu_ts_prev_valid_raw_data;
}

/*-----------------------------------------------------------------------------
 * @function  : pmu_ts_get_current_temperature
 * @brief     : Calculate current temperature from PMU TS raw data
 * @param     : None
 * @return    : Current temperature in degree celsius
 *-----------------------------------------------------------------------------
 */
int32_t pmu_ts_get_current_temperature(void)
{
    int32_t gain_err, ts_raw_data, temp_deg;
    double ts_slope_meas;
#if (FERMION_CHIP_VERSION == 1)
    if (HWIO_INXF(SEQ_WCSS_OTP_OFFSET,
                  FERMION_V1_0_QFPROM_RAW_FUSE_MAP_SECURITY_CONTROL_CORE_RAW_R_QFPROM_RAW_PTE_REGION_1_W3,
                  TRIM_TAG_HIGH) >= OTP_TRIM_TAG_HIGH)  // for OTP >= v2
#else
    if (HWIO_INXF(SEQ_WCSS_OTP_OFFSET,
                  FERMION_V2_0_QFPROM_RAW_FUSE_MAP_SECURITY_CONTROL_CORE_RAW_R_QFPROM_RAW_PTE_REGION_1_W3,
                  TRIM_TAG_HIGH) >= OTP_TRIM_TAG_HIGH)  // for OTP >= v2
#endif
    {
#if (FERMION_CHIP_VERSION == 1)
        gain_err =
            HWIO_INX(SEQ_WCSS_OTP_OFFSET,
                     FERMION_V1_0_QFPROM_RAW_FUSE_MAP_SECURITY_CONTROL_CORE_RAW_R_QFPROM_RAW_RF_CALIBRATION_ROW7_W3);
#else
        gain_err =
            HWIO_INX(SEQ_WCSS_OTP_OFFSET,
                     FERMION_V2_0_QFPROM_RAW_FUSE_MAP_SECURITY_CONTROL_CORE_RAW_R_QFPROM_RAW_RF_CALIBRATION_ROW7_W3);
#endif
        gain_err = (gain_err & R_QFPROM_RAW_RF_CALIBRATION_ROW7_W3_GAIN_ERR_BMSK) >>
                   R_QFPROM_RAW_RF_CALIBRATION_ROW7_W3_GAIN_ERR_SHFT;

        if (gain_err > 127) {
            gain_err = gain_err - 256;
        }
        gain_err = gain_err / 2;
    } else {
        gain_err = -14;
    }
    ts_raw_data = (int32_t)(pmu_ts_get_raw_data() / 2);
    ts_slope_meas = ((double)gain_err / 100.0) * 1.242 + 1.242;
    temp_deg = (int32_t)(((double)(ts_raw_data - (PMU_TS_ROOM_TEMP_DEFAULT / 2)) / ts_slope_meas) + 25.0);
    return temp_deg;
}

/*-----------------------------------------------------------------------------
 * @function  : pmu_ts_update_boot_temperature
 * @brief     : Update the bootup temperature
 * @param     : None
 * @return    : None
 *-----------------------------------------------------------------------------
 */
void pmu_ts_update_boot_temperature(void)

{
    uint32_t rdata, RAW_TS, start_time, stop_time;
    int32_t bootup_temp_deg;
    start_time = nt_hal_get_curr_time();

    // Enable HKADC for temperature monitoring
    rdata = HAL_REG_RD(QWLAN_PMU_CFG_HKADC_DATA_AVG_CNT_REG);
    rdata &= ~QWLAN_PMU_CFG_HKADC_DATA_AVG_CNT_CFG_TEMP_VBATT_MON_SEL_MASK;
    HAL_REG_WR(QWLAN_PMU_CFG_HKADC_DATA_AVG_CNT_REG, rdata);

    rdata = HAL_REG_RD(QWLAN_PMU_CFG_ACAL_VBAT_MON_EN_REG);
    HAL_REG_WR(QWLAN_PMU_CFG_ACAL_VBAT_MON_EN_REG, (rdata | QWLAN_PMU_CFG_ACAL_VBAT_MON_EN_TEMP_MON_EN_MASK));
#if (FERMION_CHIP_VERSION == 1)
    if ((HWIO_INXF(SEQ_WCSS_OTP_OFFSET,
                   FERMION_V1_0_QFPROM_RAW_FUSE_MAP_SECURITY_CONTROL_CORE_RAW_R_QFPROM_RAW_PTE_REGION_1_W3,
                   TRIM_TAG_LOW) >= OTP_TRIM_TAG_LOW) ||
        (HWIO_INXF(SEQ_WCSS_OTP_OFFSET,
                   FERMION_V1_0_QFPROM_RAW_FUSE_MAP_SECURITY_CONTROL_CORE_RAW_R_QFPROM_RAW_PTE_REGION_1_W3,
                   TRIM_TAG_HIGH) >= OTP_TRIM_TAG_HIGH))
#else
    if ((HWIO_INXF(SEQ_WCSS_OTP_OFFSET,
                   FERMION_V2_0_QFPROM_RAW_FUSE_MAP_SECURITY_CONTROL_CORE_RAW_R_QFPROM_RAW_PTE_REGION_1_W3,
                   TRIM_TAG_LOW) >= OTP_TRIM_TAG_LOW) ||
        (HWIO_INXF(SEQ_WCSS_OTP_OFFSET,
                   FERMION_V2_0_QFPROM_RAW_FUSE_MAP_SECURITY_CONTROL_CORE_RAW_R_QFPROM_RAW_PTE_REGION_1_W3,
                   TRIM_TAG_HIGH) >= OTP_TRIM_TAG_HIGH))
#endif
    {
        while ((((RAW_TS = HAL_REG_RD(QWLAN_PMU_TEMP_SNR_RD_DATA_REG)) & 0x80000000) == 0) &&
               ((nt_hal_get_curr_time() - start_time) < 1000)) {
            continue;
        }
        g_pmu_ts_data_valid = true;
        g_pmu_ts_prev_valid_raw_data = RAW_TS & QWLAN_PMU_TEMP_SNR_RD_DATA_TC_MEASURED_DATA_MASK;
    } else {
        g_pmu_ts_data_valid = false;
    }
    bootup_temp_deg = pmu_ts_get_current_temperature();
    stop_time = nt_hal_get_curr_time();
    NT_LOG_PRINT(SOCPM, WARN, "Bootup temperature %d, PMU_TS raw data %d, time taken to measure temperature %d",
                 bootup_temp_deg, g_pmu_ts_prev_valid_raw_data, (stop_time - start_time));
    return;
}

/*-----------------------------------------------------------------------------
 * @function  : is_pmu_ts_data_valid
 * @brief     : Return true if pmu ts data valid bit is set else return false
 * @param     : None
 * @return    : g_pmu_ts_data_valid
 *-----------------------------------------------------------------------------
 */
bool is_pmu_ts_data_valid(void)
{
    return g_pmu_ts_data_valid;
}

/*-----------------------------------------------------------------------------
 * @function  : is_pmu_ts_configured
 * @brief     : Return true if pmu ts is configured else return false
 * @param     : None
 * @return    : g_pmu_ts_configured
 *-----------------------------------------------------------------------------
 */
bool is_pmu_ts_configured(void)
{
    return g_pmu_ts_configured;
}

/*-----------------------------------------------------------------------------
 * @function  : invalidate_pmu_ts_configuration
 * @brief     : Set g_pmu_ts_configured to false
 * @param     : None
 * @return    : None
 *-----------------------------------------------------------------------------
 */
void invalidate_pmu_ts_configuration(void)
{
    g_pmu_ts_configured = false;
    return;
}

#endif /* PMU_TS_CONFIGURATION */
