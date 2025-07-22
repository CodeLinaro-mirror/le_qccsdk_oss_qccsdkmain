/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*******************************************************************************
 * @file wifi_fw_cpr_driver.c
 * @brief WiFi FW CPR related definitions
 *
 *
 ******************************************************************************/
#include "wifi_fw_pmic_driver.h"
#include "fwconfig_cmn.h"
#include "nt_flags.h"
#include "nt_socpm_sleep.h"

#ifdef PLATFORM_FERMION

#include "HALhwio.h"
#include "Fermion_seq_hwioreg.h"
#include "nt_logger_api.h"

extern SOCPM_STRUCT g_socpm_struct;

/*******************************************************************************
 *  Note:
 *  The CPR functionality can be enabled/disabled by the macro CONFIG_CPR_ENABLE.
 *******************************************************************************/

/*******************************************************************************
 * Function Defination
 *******************************************************************************/

/*
 * @brief : Reads CPR OTP code and caculates initial voltage / open loop voltage.
 * @param : none
 * @return: initial_mV: The initial voltage / open loop voltage in milli volt.
 */
static int32_t cpr_get_initial_mV(void)
{
    uint32 otp_target, initial_mV;
    otp_target =
        (NT_REG_RD(QWLAN_SECURITY_CONTROL_CORE_RAW_R_QFPROM_RAW_PTE_REGION_1_W0_REG) >> 8);  // CPR0_TARGET_VOL_MODE0
    otp_target &= 0x0000007f;

    if (otp_target == 0) {
        initial_mV = CPR_CX_VOLTAGE_FOR_NOT_TRIMMED_CHIP;
    } else if (otp_target > CPR_OTP_TARGET_MAX) {
        initial_mV = CPR_CX_VOLTAGE_FOR_MAX_OTP_TARGET;
    } else if (otp_target < CPR_OTP_TARGET_MIN) {
        initial_mV = CPR_CX_VOLTAGE_FOR_MIN_OTP_TARGET;
    } else {
        initial_mV = (CPR_CX_VOLTAGE_FOR_MAX_OTP_TARGET - ((CPR_OTP_TARGET_MAX - otp_target) * 3));
    }
    /* Calculating initial voltage / open loop voltage */
#if (FERMION_CHIP_VERSION == 1)
    initial_mV = ((initial_mV + 35) * 105) / 100;
#else
    initial_mV = ((initial_mV + 40) * 105) / 100;
#endif
    NT_LOG_PRINT(SOCPM, ERR, "CPR OTP Target %d, Initial Voltage: %d mV, ", otp_target, initial_mV);

    return initial_mV;
}

/*
 * @brief  : Converts milli volt to verf to set in PMU register.
 * @param  : mv: milli volt to convert to vref.
 * @return : vref: Converted vref from milli volt.
 */
static uint32_t cpr_get_vref_from_mv(uint32_t mv)
{
    /*
     * This CPR formula is only validated for EVB boards with OTP >= 5.0
     */
    uint32_t vref;
    vref = ((mv - 249) * 1000) / 2700;
    return vref;
}

/*
 * @brief  : Initializes CPR module.
 *           Should be called from main after calling PMIC init.
 * @param  : none
 * @return : none
 */
void wifi_fw_cpr_init(void)
{
    uint32_t reg_val;

    if (g_socpm_struct.cpr_cfg.ini_enabled == 1) {
#if (FERMION_CHIP_VERSION == 1)
        g_socpm_struct.cpr_cfg.otp_tag_high = HWIO_INXF(
            SEQ_WCSS_OTP_OFFSET,
            FERMION_V1_0_QFPROM_RAW_FUSE_MAP_SECURITY_CONTROL_CORE_RAW_R_QFPROM_RAW_PTE_REGION_1_W3, TRIM_TAG_HIGH);
#else
        g_socpm_struct.cpr_cfg.otp_tag_high = HWIO_INXF(
            SEQ_WCSS_OTP_OFFSET,
            FERMION_V2_0_QFPROM_RAW_FUSE_MAP_SECURITY_CONTROL_CORE_RAW_R_QFPROM_RAW_PTE_REGION_1_W3, TRIM_TAG_HIGH);
#endif
        if (g_socpm_struct.cpr_cfg.otp_tag_high > CPR_OTP_TRIM_TAG_HIGH) {
            uint32_t cx_open_loop_mv = cpr_get_initial_mV();
            /* Referring to the experience of the V&M department, the cx
             * voltage may goes too low on a slow chip at low temp.
             * So, here we set sleep mV to the maximum of CPR open loop
             * voltage and 630mV.
             */
            uint32_t cx_sleep_mv = (cx_open_loop_mv > CPR_CX_MIN_SLEEP_MV) ? cx_open_loop_mv : CPR_CX_MIN_SLEEP_MV;
            g_socpm_struct.cpr_cfg.cx_initial_mV_vref = cpr_get_vref_from_mv(cx_open_loop_mv);
            g_socpm_struct.cpr_cfg.cx_sleep_mV_vref = cpr_get_vref_from_mv(cx_sleep_mv);

            reg_val = NT_REG_RD(QWLAN_PMU_ROOT_CLK_ENABLE_REG);
            reg_val |= (QWLAN_PMU_ROOT_CLK_ENABLE_CPR_XO_ROOT_CLK_ENABLE_MASK |
                        QWLAN_PMU_ROOT_CLK_ENABLE_CPR_AHB_ROOT_CLK_ENABLE_MASK);
            NT_REG_WR(QWLAN_PMU_ROOT_CLK_ENABLE_REG, reg_val);

            NT_REG_WR(QWLAN_PMU_CFG_PWFM_TRAGET_REG, g_socpm_struct.cpr_cfg.cx_initial_mV_vref);

            reg_val = NT_REG_RD(QWLAN_RPMU_R_PMU_SMPS2_0_REG);
            reg_val &= ~QWLAN_RPMU_R_PMU_SMPS2_0_SMPS2_VSET_HIGH_MASK;
            reg_val |= (g_socpm_struct.cpr_cfg.cx_initial_mV_vref << QWLAN_RPMU_R_PMU_SMPS2_0_SMPS2_VSET_HIGH_OFFSET) &
                       QWLAN_RPMU_R_PMU_SMPS2_0_SMPS2_VSET_HIGH_MASK;
            NT_REG_WR(QWLAN_RPMU_R_PMU_SMPS2_0_REG, reg_val);

            // Only 0/2/3/12/14/15 are enabled
            // GCNT is 9
#if (FERMION_CHIP_VERSION == 1)
            NT_REG_WR(QWLAN_CPR_WRAPPER_R_CPR_GCNT0_REG, CPR_RO_GCNT);
            NT_REG_WR(QWLAN_CPR_WRAPPER_R_CPR_GCNT2_REG, CPR_RO_GCNT);
            NT_REG_WR(QWLAN_CPR_WRAPPER_R_CPR_GCNT3_REG, CPR_RO_GCNT);
            NT_REG_WR(QWLAN_CPR_WRAPPER_R_CPR_GCNT12_REG, CPR_RO_GCNT);
            NT_REG_WR(QWLAN_CPR_WRAPPER_R_CPR_GCNT14_REG, CPR_RO_GCNT);
            NT_REG_WR(QWLAN_CPR_WRAPPER_R_CPR_GCNT15_REG, CPR_RO_GCNT);
            NT_REG_WR(QWLAN_CPR_WRAPPER_R_CPR_TARGET0_0_0_REG, CPR_RO0_TARGET);
            NT_REG_WR(QWLAN_CPR_WRAPPER_R_CPR_TARGET2_0_0_REG, CPR_RO2_TARGET);
            NT_REG_WR(QWLAN_CPR_WRAPPER_R_CPR_TARGET3_0_0_REG, CPR_RO3_TARGET);
            NT_REG_WR(QWLAN_CPR_WRAPPER_R_CPR_TARGET12_0_0_REG, CPR_RO12_TARGET);
            NT_REG_WR(QWLAN_CPR_WRAPPER_R_CPR_TARGET14_0_0_REG, CPR_RO14_TARGET);
            NT_REG_WR(QWLAN_CPR_WRAPPER_R_CPR_TARGET15_0_0_REG, CPR_RO15_TARGET);
#else
            NT_REG_WR(QWLAN_CPR_WRAPPER_R_CPR_GCNT0_REG, CPR_RO_GCNT);
            NT_REG_WR(QWLAN_CPR_WRAPPER_R_CPR_GCNT1_REG, CPR_RO_GCNT);
            NT_REG_WR(QWLAN_CPR_WRAPPER_R_CPR_GCNT7_REG, CPR_RO_GCNT);
            NT_REG_WR(QWLAN_CPR_WRAPPER_R_CPR_GCNT12_REG, CPR_RO_GCNT);
            NT_REG_WR(QWLAN_CPR_WRAPPER_R_CPR_GCNT13_REG, CPR_RO_GCNT);
            NT_REG_WR(QWLAN_CPR_WRAPPER_R_CPR_GCNT14_REG, CPR_RO_GCNT);
            NT_REG_WR(QWLAN_CPR_WRAPPER_R_CPR_GCNT15_REG, CPR_RO_GCNT);
            NT_REG_WR(QWLAN_CPR_WRAPPER_R_CPR_TARGET0_0_0_REG, CPR_RO0_TARGET);
            NT_REG_WR(QWLAN_CPR_WRAPPER_R_CPR_TARGET1_0_0_REG, CPR_RO1_TARGET);
            NT_REG_WR(QWLAN_CPR_WRAPPER_R_CPR_TARGET7_0_0_REG, CPR_RO7_TARGET);
            NT_REG_WR(QWLAN_CPR_WRAPPER_R_CPR_TARGET12_0_0_REG, CPR_RO12_TARGET);
            NT_REG_WR(QWLAN_CPR_WRAPPER_R_CPR_TARGET13_0_0_REG, CPR_RO13_TARGET);
            NT_REG_WR(QWLAN_CPR_WRAPPER_R_CPR_TARGET14_0_0_REG, CPR_RO14_TARGET);
            NT_REG_WR(QWLAN_CPR_WRAPPER_R_CPR_TARGET15_0_0_REG, CPR_RO15_TARGET);
#endif  //(FERMION_CHIP_VERSION == 1)

            NT_REG_WR(QWLAN_CPR_WRAPPER_R_CPR_SENSOR_MASK_WRITE_MREG, 0);
            reg_val = 0;
            reg_val |= (0x5 << QWLAN_CPR_WRAPPER_R_CPR_STEP_QUOT_INIT_STEP_QUOT_INIT_MAX_OFFSET) &
                       QWLAN_CPR_WRAPPER_R_CPR_STEP_QUOT_INIT_STEP_QUOT_INIT_MAX_MASK;
            reg_val |= (0x1 << QWLAN_CPR_WRAPPER_R_CPR_STEP_QUOT_INIT_STEP_QUOT_INIT_MIN_OFFSET) &
                       QWLAN_CPR_WRAPPER_R_CPR_STEP_QUOT_INIT_STEP_QUOT_INIT_MIN_MASK;
            NT_REG_WR(QWLAN_CPR_WRAPPER_R_CPR_STEP_QUOT_INIT_REG, reg_val);

            // Some delays - based on 9.6MHz clock
            NT_REG_WR(QWLAN_CPR_WRAPPER_R_CPR_TIMER_AUTO_CONT_REG, CPR_DONE_MEASUREMENT_PERIOD_USECS * 96 / 10);

            reg_val = QWLAN_CPR_WRAPPER_R_CPR_MARGIN_TEMP_CORE_TIMERS_DEFAULT;
            reg_val &= ~QWLAN_CPR_WRAPPER_R_CPR_MARGIN_TEMP_CORE_TIMERS_TIMER_SETTLE_VOLTAGE_COUNT_MASK;
            reg_val |= ((CPR_STEP_MEASUREMENT_PERIOD_USECS * 96 / 10)
                        << QWLAN_CPR_WRAPPER_R_CPR_MARGIN_TEMP_CORE_TIMERS_TIMER_SETTLE_VOLTAGE_COUNT_OFFSET) &
                       QWLAN_CPR_WRAPPER_R_CPR_MARGIN_TEMP_CORE_TIMERS_TIMER_SETTLE_VOLTAGE_COUNT_MASK;
            NT_REG_WR(QWLAN_CPR_WRAPPER_R_CPR_MARGIN_TEMP_CORE_TIMERS_REG, reg_val);

            // Enable HW closed loop
            reg_val = QWLAN_CPR_WRAPPER_R_CPR_TIMER_CLAMP_DEFAULT;
            reg_val |= QWLAN_CPR_WRAPPER_R_CPR_TIMER_CLAMP_CPR_DISABLE_VALID_MASK;
            NT_REG_WR(QWLAN_CPR_WRAPPER_R_CPR_TIMER_CLAMP_REG, reg_val);

            reg_val = QWLAN_CPR_WRAPPER_R_CPR_MARGIN_ADJ_CTL_DEFAULT;
            reg_val |= QWLAN_CPR_WRAPPER_R_CPR_MARGIN_ADJ_CTL_CLOSED_LOOP_EN_MASK;
            reg_val |= QWLAN_CPR_WRAPPER_R_CPR_MARGIN_ADJ_CTL_TIMER_SETTLE_VOLTAGE_EN_MASK;
            NT_REG_WR(QWLAN_CPR_WRAPPER_R_CPR_MARGIN_ADJ_CTL_REG, reg_val);

            reg_val = QWLAN_CPR_WRAPPER_R_CPR_MISC_REGISTER_DEFAULT;
            reg_val &= ~QWLAN_CPR_WRAPPER_R_CPR_MISC_REGISTER_CLOSED_LOOP_UP_DN_SUPPRESS_EN_MASK;
            NT_REG_WR(QWLAN_CPR_WRAPPER_R_CPR_MISC_REGISTER_REG, reg_val);

            reg_val = QWLAN_CPR_WRAPPER_R_CPR_FSM_CTL_DEFAULT;
            reg_val &= ~QWLAN_CPR_WRAPPER_R_CPR_FSM_CTL_COUNT_REPEAT_MASK;
            reg_val |= (0x1 << QWLAN_CPR_WRAPPER_R_CPR_FSM_CTL_COUNT_REPEAT_OFFSET) &
                       QWLAN_CPR_WRAPPER_R_CPR_FSM_CTL_COUNT_REPEAT_MASK;
            reg_val |= QWLAN_CPR_WRAPPER_R_CPR_FSM_CTL_LOOP_EN_MASK;
            NT_REG_WR(QWLAN_CPR_WRAPPER_R_CPR_FSM_CTL_REG, reg_val);

            NT_REG_WR(QWLAN_PMU_CFG_SMPS2_DVS_CNTL_REG, QWLAN_PMU_CFG_SMPS2_DVS_CNTL_CFG_ALLOW_PWFM_CPR_SENSOR_MASK);

            reg_val = 0;
            reg_val |= (cpr_get_vref_from_mv(CPR_MAX_CX_VOLTAGE) << QWLAN_PMU_CPR_CONFIG1_MAX_VREF_VALUE_OFFSET) &
                       QWLAN_PMU_CPR_CONFIG1_MAX_VREF_VALUE_MASK;
            reg_val |= (cpr_get_vref_from_mv(CPR_MIN_CX_VOLTAGE) << QWLAN_PMU_CPR_CONFIG1_MIN_VREF_VALUE_OFFSET) &
                       QWLAN_PMU_CPR_CONFIG1_MIN_VREF_VALUE_MASK;
            reg_val |= (g_socpm_struct.cpr_cfg.cx_initial_mV_vref << QWLAN_PMU_CPR_CONFIG1_INTIAL_VREF_VALUE_OFFSET) &
                       QWLAN_PMU_CPR_CONFIG1_INTIAL_VREF_VALUE_MASK;
            NT_REG_WR(QWLAN_PMU_CPR_CONFIG1_REG, reg_val);

            reg_val = 0;
            reg_val |=
                (CPR_STEP_SIZE << QWLAN_PMU_CPR_CONFIG0_STEP_VALUE_OFFSET) & QWLAN_PMU_CPR_CONFIG0_STEP_VALUE_MASK;
            reg_val |= (0xa << QWLAN_PMU_CPR_CONFIG0_DELAY_VALUE_OFFSET) & QWLAN_PMU_CPR_CONFIG0_DELAY_VALUE_MASK;
            reg_val |= QWLAN_PMU_CPR_CONFIG0_HW_CL_ENABLE_MASK;
            reg_val |= QWLAN_PMU_CPR_CONFIG0_CPR_ENABLE_MASK;
            NT_REG_WR(QWLAN_PMU_CPR_CONFIG0_REG, reg_val);

#if (FERMION_CHIP_VERSION == 1)
            NT_REG_WR(QWLAN_CPR_WRAPPER_R_CPR_MASK_THREAD__MREG, 0x2ff2);  // 5 RO sensors check when runtime
#else
            NT_REG_WR(QWLAN_CPR_WRAPPER_R_CPR_MASK_THREAD__MREG, 0x0f7c);  // 6 RO sensors check when runtime
#endif

            NT_LOG_PRINT(SOCPM, ERR, "CPR Initialized with Step size: %d, Interval: %d", CPR_STEP_SIZE,
                         CPR_STEP_MEASUREMENT_PERIOD_USECS);
        }
    }
}

/*
 * @brief  : Re-enable CPR module.
 *           Should be called in case of warm boot.
 * @param  : none
 * @return : none
 */
void wifi_fw_cpr_reenable(void)
{
    if ((g_socpm_struct.cpr_cfg.ini_enabled == 1) && (g_socpm_struct.cpr_cfg.otp_tag_high > CPR_OTP_TRIM_TAG_HIGH)) {
        uint32_t reg_val;
        NT_REG_WR(QWLAN_PMU_CFG_PWFM_TRAGET_REG, g_socpm_struct.cpr_cfg.cx_initial_mV_vref);

        reg_val = NT_REG_RD(QWLAN_PMU_CPR_CONFIG0_REG);
        reg_val |= QWLAN_PMU_CPR_CONFIG0_CPR_ENABLE_MASK;
        NT_REG_WR(QWLAN_PMU_CPR_CONFIG0_REG, reg_val);
    }
}

/*
 * @brief  : Disable CPR module.
 *           Should be called before going to light sleep, MCU sleep and
 *           deep sleep.
 * @param  : none
 * @return : none
 * @note   : The caller must ensure PMU flush by reading PMU register after
 *           calling this API.
 */
void wifi_fw_cpr_disable(void)
{
    if ((g_socpm_struct.cpr_cfg.ini_enabled == 1) && (g_socpm_struct.cpr_cfg.otp_tag_high > CPR_OTP_TRIM_TAG_HIGH)) {
        uint32_t reg_val;
        // Set SMPS2 voltage to sleep voltage for safety, and then controlled by CPR ULP mode
        NT_REG_WR(QWLAN_PMU_CFG_PWFM_TRAGET_REG, g_socpm_struct.cpr_cfg.cx_sleep_mV_vref);

        reg_val = NT_REG_RD(QWLAN_PMU_CPR_CONFIG0_REG);
        reg_val &= ~QWLAN_PMU_CPR_CONFIG0_CPR_ENABLE_MASK;
        NT_REG_WR(QWLAN_PMU_CPR_CONFIG0_REG, reg_val);
    }
}

#endif /* PLATFORM_FERMION */
