/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/**********************************************************************************************
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 * @file wifi_fw_pmic_driver.c
 * @brief WiFi FW PMIC related definitions
 *
 *
 *********************************************************************************************/
#include "wifi_fw_pmic_driver.h"
#include "fwconfig_cmn.h"
#include "nt_flags.h"
#include "nt_socpm_sleep.h"

#ifdef PLATFORM_FERMION

#include "HALhwio.h"
#include "Fermion_seq_hwioreg.h"
#include "nt_logger_api.h"
#include "nt_devcfg.h"
#include "hal_int_modules.h"

#define OTP_TRIM_TAG_LOW  1
#define OTP_TRIM_TAG_HIGH 2

uint32_t g_ferm_neutrino_mode = 0;  // default to be Fermion mode

extern SOCPM_STRUCT g_socpm_struct;
extern void phyrf_temperature_data_otp_trim(void);

/*
 * @brief: Programmed RFA/AON/CX trim values
 * @param : none
 * @return : none
 */

void wifi_fw_program_pmic_and_aon_otp_trim(void)
{
#if (FERMION_CHIP_VERSION == 1)
    uint32_t otp_tag_low = 0;
    uint32_t otp_tag_high = 0;

    otp_tag_low = HWIO_INXF(SEQ_WCSS_OTP_OFFSET,
                            FERMION_V1_0_QFPROM_RAW_FUSE_MAP_SECURITY_CONTROL_CORE_RAW_R_QFPROM_RAW_PTE_REGION_1_W3,
                            TRIM_TAG_LOW);
    otp_tag_high = HWIO_INXF(SEQ_WCSS_OTP_OFFSET,
                             FERMION_V1_0_QFPROM_RAW_FUSE_MAP_SECURITY_CONTROL_CORE_RAW_R_QFPROM_RAW_PTE_REGION_1_W3,
                             TRIM_TAG_HIGH);

    // TODO:consider RRAM_BYPASS=1?? support OTP v1 or V2 only
    if ((otp_tag_low >= OTP_TRIM_TAG_LOW) || (otp_tag_high >= OTP_TRIM_TAG_HIGH))  // from OTP
    {
        // MB
        uint32_t tOTP16 =
            HWIO_INXF(SEQ_WCSS_OTP_OFFSET,
                      FERMION_V1_0_QFPROM_RAW_FUSE_MAP_SECURITY_CONTROL_CORE_RAW_R_QFPROM_RAW_RF_CALIBRATION_ROW4_W2,
                      LDO_1P6_VREF);
        HWIO_OUTXF(SEQ_WCSS_MBIAS_OFFSET, MBIAS_MBIAS_R_LDO_CTRL_0, D_LDO16_EN_OVS, 0x3);
        HWIO_OUTXF(SEQ_WCSS_MBIAS_OFFSET, MBIAS_MBIAS_R_LDO_CTRL_0, D_LDO16_VREF, tOTP16);

        // refer phyBootSeq.c#2
        tOTP16 = HWIO_INXF(SEQ_WCSS_OTP_OFFSET,
                           FERMION_V1_0_QFPROM_RAW_FUSE_MAP_SECURITY_CONTROL_CORE_RAW_R_QFPROM_RAW_NPS_CONFIG_ROW1_W1,
                           CORE_V2IC_TRIM);
        HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_CORE_2, CORE_V2IC_TRIM_SEL, 0x1);  // Auto Fetch
        HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_CORE_2, CORE_V2IC_TRIM, tOTP16);

        // AON trim
        tOTP16 =
            HWIO_INXF(SEQ_WCSS_OTP_OFFSET,
                      FERMION_V1_0_QFPROM_RAW_FUSE_MAP_SECURITY_CONTROL_CORE_RAW_R_QFPROM_RAW_RF_CALIBRATION_ROW4_W3,
                      LDO_RESERVD_0);
        HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_CORE_5, LDO_AON_VSET_HIGH_OV_TRIM, 0x1);
        HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_CORE_5, LDO_AON_VSET_HIGH,
                   (tOTP16 & 0xff));  // bit 19:26 (19:31) //tOTP14

        // RC trim 27K from 32K
        tOTP16 =
            HWIO_INXF(SEQ_WCSS_OTP_OFFSET,
                      FERMION_V1_0_QFPROM_RAW_FUSE_MAP_SECURITY_CONTROL_CORE_RAW_R_QFPROM_RAW_RF_CALIBRATION_ROW5_W0,
                      LDO_RESERVD_1);
        HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_CORE_2, CORE_RCOSC_32K_TRIM_SEL, 0x1);
        HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_CORE_2, CORE_RCOSC_32K_TRIM,
                   ((tOTP16 >> 9) & 0x1f));  // bit 9:13 (0:20)

        // CX trim only for OTPv3 and above
        if ((g_socpm_struct.cpr_cfg.ini_enabled == 1) && (otp_tag_high > OTP_TRIM_TAG_HIGH)) {
            tOTP16 =
                HWIO_INXF(SEQ_WCSS_OTP_OFFSET,
                          FERMION_V1_0_QFPROM_RAW_FUSE_MAP_SECURITY_CONTROL_CORE_RAW_R_QFPROM_RAW_NPS_CONFIG_ROW1_W1,
                          LDOCX_RDAC_CFG);
            HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_CORE_12, LDOCX_VSET_TRIM_SEL,
                       0x1);  // Auto Fetch //PMU Related
            HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_CORE_12, LDOCX_VREF_TRIM,
                       (tOTP16 & 0x1f));  // bit 23:27 (23:30)
        }
    }
#elif (FERMION_CHIP_VERSION == 2)
    // sync from HWS autoseq: void phyrf_otp_trim_pmu_iot()
    uint32_t otp_tag_high = HWIO_INXF(
        SEQ_WCSS_OTP_OFFSET, FERMION_V2_0_QFPROM_RAW_FUSE_MAP_SECURITY_CONTROL_CORE_RAW_R_QFPROM_RAW_PTE_REGION_1_W3,
        TRIM_TAG_HIGH);
    uint32_t otp_tag_low = HWIO_INXF(
        SEQ_WCSS_OTP_OFFSET, FERMION_V2_0_QFPROM_RAW_FUSE_MAP_SECURITY_CONTROL_CORE_RAW_R_QFPROM_RAW_PTE_REGION_1_W3,
        TRIM_TAG_LOW);
    uint32_t value;

    if ((otp_tag_low == 0) && (otp_tag_high == 0))  // for blind chip
    {
        NT_LOG_PRINT(SOCPM, INFO, "Blind Chip - set ov to 1 (use default ovd value to avoid auto fecth 0) - PMU");
        HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_CORE_2, CORE_V2IC_TRIM_SEL, 0x1);  // Auto Fetch //PMU Related
        HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_CORE_12, LDOCX_VSET_TRIM_SEL,
                   0x1);  // Auto Fetch //PMU Related
    }

    if (otp_tag_high >= 1)  // for OTP >= v1
    {
        NT_LOG_PRINT(SOCPM, INFO, "Program Fermion2 OTP v1 registers - PMU");
    }
    if (otp_tag_high >= 2)  // for OTP >= v2
    {
        NT_LOG_PRINT(SOCPM, INFO, "Program Fermion2 OTP v2 registers - PMU");

        // 32K TRIM
        /*
         * Quoted from Senpeng Sheng : Change the default value of ULP_BG_IPT62P5N_TRIM for register RPMU_R_PMU_ULPBG_6
         * from 4 to 0 The bias current to the oscillator of 32kHz clock is halved with this change
         */
        if (otp_tag_high >= 6)  // for OTP >= v6
        {
            HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_ULPBG_6, ULP_BG_IPT62P5N_TRIM, 0x4);
        } else {
            HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_ULPBG_6, ULP_BG_IPT62P5N_TRIM, 0x0);
        }

        if (otp_tag_high >= 3)  // for OTP >= v3
        {
            // VIFERMION-435
            uint32_t value_CORE_RCOSC_32K_TRIM = HWIO_INXF(
                SEQ_WCSS_OTP_OFFSET,
                FERMION_V2_0_QFPROM_RAW_FUSE_MAP_SECURITY_CONTROL_CORE_RAW_R_QFPROM_RAW_RF_CALIBRATION_ROW5_W0,
                CORE_RCOSC_32K_TRIM);  // bit 9:16
            uint32_t value_XO_OSC_32K_TRIM_MSB =
                HWIO_INXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_CORE_10, XO_OSC_32K_TRIM_MSB);  // bit 15:8
            value_XO_OSC_32K_TRIM_MSB &= 0xE3;  // clear bit 12:10 of RPMU_RPMU_R_PMU_CORE_10
            value_XO_OSC_32K_TRIM_MSB |=
                ((value_CORE_RCOSC_32K_TRIM & 0x3)
                 << 2);  // bit 11:10 of RPMU_RPMU_R_PMU_CORE_10 filled with bit 1:0 of value_CORE_RCOSC_32K_TRIM
            value_XO_OSC_32K_TRIM_MSB |=
                ((value_CORE_RCOSC_32K_TRIM & 0x80) >>
                 3);  // bit 12 of RPMU_RPMU_R_PMU_CORE_10 filled with bit 7 of value_CORE_RCOSC_32K_TRIM
            HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_CORE_10, XO_OSC_32K_TRIM_MSB,
                       value_XO_OSC_32K_TRIM_MSB);  // bit 12:10 of RPMU_RPMU_R_PMU_CORE_10 filled with bit 7, 1, 0 of
                                                    // value_CORE_RCOSC_32K_TRIM
            HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_CORE_2, CORE_RCOSC_32K_TRIM,
                       (value_CORE_RCOSC_32K_TRIM & 0x7c) >>
                           2);  // bit 25:21 of RPMU_R_PMU_CORE_2 filled with bit 6:2 of value_CORE_RCOSC_32K_TRIM
            HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_CORE_2, CORE_RCOSC_32K_TRIM_SEL, 0x1);
        } else {
            // 32K TRIM
            uint32_t SleepClock = HWIO_INXF(
                SEQ_WCSS_OTP_OFFSET,
                FERMION_V2_0_QFPROM_RAW_FUSE_MAP_SECURITY_CONTROL_CORE_RAW_R_QFPROM_RAW_RF_CALIBRATION_ROW5_W0,
                CORE_RCOSC_32K_TRIM);  // bit 9:16
            HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_CORE_2, CORE_RCOSC_32K_TRIM_SEL, 0x1);
            HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_CORE_2, CORE_RCOSC_32K_TRIM, SleepClock);  // bit 21:25
        }

        NT_LOG_PRINT(SOCPM, INFO, "OTPv2: remove LDOCX_ULP_COMP_VREF_TRIM and D_LDOCX_MX_TRIM due to V&M only");
#if 0
        // LDOCX_ULP_COMP_VREF_TRIM, for V&M only
        uint32_t CXULP_COMP_VREF = HWIO_INXF(SEQ_WCSS_OTP_OFFSET, FERMION_V2_0_QFPROM_RAW_FUSE_MAP_SECURITY_CONTROL_CORE_RAW_R_QFPROM_RAW_RF_CALIBRATION_ROW5_W0, LDOCX_ULP_COMP_VREF_TRIM);
        HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_ULPBG_7, LDOCX_ULP_COMP_VREF_TRIM, CXULP_COMP_VREF);
#endif

#if 0
        // D_LDOCX_MX_TRIM, for V&M only
        uint32_t CXMX_TRIM = HWIO_INXF(SEQ_WCSS_OTP_OFFSET, FERMION_V2_0_QFPROM_RAW_FUSE_MAP_SECURITY_CONTROL_CORE_RAW_R_QFPROM_RAW_RF_CALIBRATION_ROW8_W2, D_LDOCX_MX_TRIM); // bit 4:5
        uint32_t SMPS2SPAREE = HWIO_INXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_SMPS2_3, SMPS2_SPAREE);                                                                        // bit 11:14
        SMPS2SPAREE = (SMPS2SPAREE | CXMX_TRIM);                                                                                                                              // Keep bit 13:14, only change bit 11:12 only
        HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_SMPS2_3, SMPS2_SPAREE, SMPS2SPAREE);
#endif

        // BROWN_OUT_THRES_TRIM
        int32_t brown_out_thresh =
            HWIO_INXF(SEQ_WCSS_OTP_OFFSET,
                      FERMION_V2_0_QFPROM_RAW_FUSE_MAP_SECURITY_CONTROL_CORE_RAW_R_QFPROM_RAW_RF_CALIBRATION_ROW8_W2,
                      BROWN_OUT_THRES_TRIM);
        /* original sweep from 13 to 27 and write OTP with -7 to 7 by offset of -20 */
        if (brown_out_thresh & 0x8)
            brown_out_thresh |= 0xFFFFFFF0;  // sign extension.
        brown_out_thresh += 20;              /* add back the offset during ATE trim */
        HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_BROWN_DET, VBAT_BROWN_POK_TRIM, brown_out_thresh);
        HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_BROWN_DET, VBAT_BROWN_EN, 1);
        HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_BROWN_DET, VBAT_BROWN_SEL_ULP_BIAS, 1);

        // AON VSET HIGH trim
        // VIFERMION-414 ,LIBR-5470
        HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_CORE_12, LDOAO_VIN_MODE, 0x2);
        nt_socpm_nop_delay(1000);

        value =
            HWIO_INXF(SEQ_WCSS_OTP_OFFSET,
                      FERMION_V2_0_QFPROM_RAW_FUSE_MAP_SECURITY_CONTROL_CORE_RAW_R_QFPROM_RAW_RF_CALIBRATION_ROW8_W1,
                      LDO_AON_VSET_HIGH_VMX);
        HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_CORE_5, LDO_AON_VSET_HIGH_OV_TRIM, 0x1);
        HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_CORE_5, LDO_AON_VSET_HIGH, value);  // bit 8:15

        // FERMTWO-406, Set LDOAO_LOOPGAIN_UP to 1 give better performance after cold-boot
        HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_CORE_7, LDOAO_LOOPGAIN_UP, 0x1);

        // SMPS2_VREF_TRIM
        NT_LOG_PRINT(SOCPM, INFO, "OTPv2: added SMPS2_VREF_TRIM");
        uint32_t OTP_SMPS2_VREF_TRIM =
            HWIO_INXF(SEQ_WCSS_OTP_OFFSET,
                      FERMION_V2_0_QFPROM_RAW_FUSE_MAP_SECURITY_CONTROL_CORE_RAW_R_QFPROM_RAW_NPS_CONFIG_ROW1_W0,
                      SMPS2_VREF_TRIM);
        HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_SMPS2_0, SMPS2_VREF_TRIM, OTP_SMPS2_VREF_TRIM);
    }

    if ((otp_tag_high >= 2) && (otp_tag_high < 4)) {
        // SMPS1_VREF_TRIM
        NT_LOG_PRINT(SOCPM, INFO, "OTPv2-3.2: added SMPS1_VREF_TRIM");
        uint32_t OTP_SMPS1_VREF_TRIM =
            HWIO_INXF(SEQ_WCSS_OTP_OFFSET,
                      FERMION_V2_0_QFPROM_RAW_FUSE_MAP_SECURITY_CONTROL_CORE_RAW_R_QFPROM_RAW_NPS_CONFIG_ROW1_W0,
                      SMPS1_VREF_TRIM);
        HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_SMPS1_0, SMPS1_VREF_TRIM, OTP_SMPS1_VREF_TRIM);
    }

    if (otp_tag_high >= 3)  // for OTP >= v3
    {
        NT_LOG_PRINT(HALPHY, INFO, "Program Fermion2 OTP v3 registers - PMU (IOT)");

        // ULP_BG_LDOAO_VREF1/2_TRIM
        // Will keep same always according to ATE
        uint32_t ULPBG_VREF1 =
            HWIO_INXF(SEQ_WCSS_OTP_OFFSET,
                      FERMION_V2_0_QFPROM_RAW_FUSE_MAP_SECURITY_CONTROL_CORE_RAW_R_QFPROM_RAW_RF_CALIBRATION_ROW5_W0,
                      ULP_BG_LDOAO_VREF1_TRIM);
        uint32_t ULPBG_VREF2 =
            HWIO_INXF(SEQ_WCSS_OTP_OFFSET,
                      FERMION_V2_0_QFPROM_RAW_FUSE_MAP_SECURITY_CONTROL_CORE_RAW_R_QFPROM_RAW_RF_CALIBRATION_ROW4_W3,
                      ULP_BG_LDOAO_VREF2_TRIM);
        HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_ULPBG_6, ULP_BG_LDOAO_VREF1_TRIM, ULPBG_VREF1);
        HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_ULPBG_6, ULP_BG_LDOAO_VREF2_TRIM, ULPBG_VREF2);

        NT_LOG_PRINT(SOCPM, INFO, "OTPv3: added SMPS1_ULP_COMP_VREF_TRIM");
        // SMPS1_ULP_COMP_VREF_TRIM
        uint32_t OTP_SMPS1_ULP_COMP_VREF_TRIM =
            HWIO_INXF(SEQ_WCSS_OTP_OFFSET,
                      FERMION_V2_0_QFPROM_RAW_FUSE_MAP_SECURITY_CONTROL_CORE_RAW_R_QFPROM_RAW_RF_CALIBRATION_ROW8_W2,
                      SMPS1_ULP_COMP_VREF_TRIM);
        HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_SMPS1_18, SMPS1_ULP_COMP_VREF_TRIM,
                   OTP_SMPS1_ULP_COMP_VREF_TRIM);
    }

    if (otp_tag_high >= 4)  // SMPS1/2_ONESHOT_TRIM
    {
        if ((otp_tag_high == 4) && (otp_tag_low == 0)) {
            NT_LOG_PRINT(HALPHY, INFO, "In OTPv4.0, program fixed value 6/4 to SMPS1/2 oneshot trim");
            HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_SMPS1_18, SMPS1_ONESHOT_TRIM, 0x6);  // Default is 0x6
            HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_SMPS2_18, SMPS2_ONESHOT_TRIM,
                       0x4);  // Default is 0x6, CS requires to set 4
        } else {
            uint32_t OTP_SMPS1_OS_TRM = HWIO_INXF(
                SEQ_WCSS_OTP_OFFSET,
                FERMION_V2_0_QFPROM_RAW_FUSE_MAP_SECURITY_CONTROL_CORE_RAW_R_QFPROM_RAW_RF_CALIBRATION_ROW9_W0,
                SMPS1_ONESHOT_TRIM);
            uint32_t OTP_SMPS2_OS_TRM = HWIO_INXF(
                SEQ_WCSS_OTP_OFFSET,
                FERMION_V2_0_QFPROM_RAW_FUSE_MAP_SECURITY_CONTROL_CORE_RAW_R_QFPROM_RAW_RF_CALIBRATION_ROW9_W0,
                SMPS2_ONESHOT_TRIM);
            HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_SMPS1_18, SMPS1_ONESHOT_TRIM, OTP_SMPS1_OS_TRM);
            HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_SMPS2_18, SMPS2_ONESHOT_TRIM, OTP_SMPS2_OS_TRM);
            NT_LOG_PRINT(HALPHY, INFO, "In OTPv4.1 or newer, program value %d/%d from OTP to SMPS1/2 oneshot trim",
                         OTP_SMPS1_OS_TRM, OTP_SMPS2_OS_TRM);
        }
    }

    if (otp_tag_high >= 4)  // for OTP >= v4
    {
        NT_LOG_PRINT(HALPHY, INFO, "Program Fermion2 OTP v4 registers - PMU (IOT)");

        // SMPS1_VSET_HIGH
        uint32_t OTP_SMPS1_VSET_HIGH =
            HWIO_INXF(SEQ_WCSS_OTP_OFFSET,
                      FERMION_V2_0_QFPROM_RAW_FUSE_MAP_SECURITY_CONTROL_CORE_RAW_R_QFPROM_RAW_RF_CALIBRATION_ROW8_W3,
                      SMPS1_VSET_HIGH);
        HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_SMPS1_0, SMPS1_VSET_HIGH, OTP_SMPS1_VSET_HIGH);

        // SMPS2_ULP_COMP_VREF_TRIM
        uint32_t OTP_SMPS2_ULP_COMP_VREF_TRIM =
            HWIO_INXF(SEQ_WCSS_OTP_OFFSET,
                      FERMION_V2_0_QFPROM_RAW_FUSE_MAP_SECURITY_CONTROL_CORE_RAW_R_QFPROM_RAW_RF_CALIBRATION_ROW8_W2,
                      SMPS2_ULP_COMP_VREF_TRIM);
        HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_SMPS2_18, SMPS2_ULP_COMP_VREF_TRIM,
                   OTP_SMPS2_ULP_COMP_VREF_TRIM);
        NT_LOG_PRINT(SOCPM, INFO, "OTPv4: added SMPS2_ULP_COMP_VREF_TRIM %d", OTP_SMPS2_ULP_COMP_VREF_TRIM);
    }
#endif /* (FERMION_CHIP_VERSION) */
}

/********************************************************************************************
 * Function Defination
 ********************************************************************************************/

/**********************************************************************************************
 * @brief Initialise PMIC module for fermion
 *
 *********************************************************************************************/

void wifi_fw_pmic_init(cpr_mode_e cpr_mode)
{
#ifdef PLATFORM_INIT_PMIC
    hal_mac_sw_powerup();
    g_ferm_neutrino_mode =
        NT_REG_RD(QWLAN_PMU_IO_RAW_STRAP_VALUE_REG) & QWLAN_PMU_IO_RAW_STRAP_VALUE_NEUTRINO_MODE_MASK ? 1 : 0;
    (void)g_ferm_neutrino_mode;
    // g_ferm_neutrino_mode = 0;
    (void)g_ferm_neutrino_mode;
    printf("PMIC init. neutrino mode: %d\n", g_ferm_neutrino_mode);

#if (FERMION_CHIP_VERSION == 2)
    {
        uint32_t TAG_HIGH = HWIO_INXF(
            SEQ_WCSS_OTP_OFFSET,
            FERMION_V2_0_QFPROM_RAW_FUSE_MAP_SECURITY_CONTROL_CORE_RAW_R_QFPROM_RAW_PTE_REGION_1_W3, TRIM_TAG_HIGH);
        uint32_t TAG_LOW = HWIO_INXF(
            SEQ_WCSS_OTP_OFFSET,
            FERMION_V2_0_QFPROM_RAW_FUSE_MAP_SECURITY_CONTROL_CORE_RAW_R_QFPROM_RAW_PTE_REGION_1_W3, TRIM_TAG_LOW);
        printf("Chip v2, OTP Tag high: %d, low: %d\n", TAG_HIGH, TAG_LOW);
    }
#endif

    wifi_fw_select_xo_sleep_clock();
    //  HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_BROWN_DET, VBAT_BROWN_EN, 1);
    HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_ULPBG_6, ULP_BG_SELFSTART_DIS, 1);
    nt_socpm_nop_delay(100);
    HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_ULPBG_6, ULP_BG_START_EN, 1);
    nt_socpm_nop_delay(100);
    HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_ULPBG_6, ULP_BG_EN, 1);

    // delay 4ms
    nt_socpm_nop_delay(4000);
    HWIO_OUTX3F(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_ULPBG_7, ULP_BG_OTA_START_OV, ULP_BG_SELFSTART_DIS_OV,
                ULP_BG_START_EN_OV, 0x2, 0x2, 0x2);

    nt_socpm_nop_delay(4000);
    HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_SMPS1_6, SMPS1_POK_FORCE, 1);
    HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_SMPS2_6, SMPS2_POK_FORCE, 1);
    HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_CORE_9, LDOAO_POK_FORCE, 1);
    HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_CORE_11, LDORFA_POK_FORCE, 1);
    HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_SMPS1_6, SMPS1_POK_DIS, 1);
    HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_SMPS2_6, SMPS2_POK_DIS, 1);

    // AON VSET HIGH trim
    HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_CORE_9, LDOAO_POK_DIS, 1);
    HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_CORE_11, LDORFA_POK_DIS, 1);

    HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_CORE_10, LDOOSC_BIAS_SEL, 1);

    // delay 8ms
    nt_socpm_nop_delay(100);
    wifi_fw_program_pmic_and_aon_otp_trim();

    if (g_ferm_neutrino_mode == 1) {
        // HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET,RPMU_RPMU_R_PMU_CORE_2,LDOOSC_TRIM,5);
        // HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET,RPMU_RPMU_R_PMU_CORE_2,LDOOSC_TRIM,5);
        // TODO:not required since SMPS_S1 in PFM mode
        // HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET,RPMU_RPMU_R_FOOTSW,FOOTSW_EN,1);
        // TODO: cannot find FOOTSW_CODE ?????
        // HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET,RPMU_RPMU_R_PMU_CORE_12,D_LDOAO_VIN_MODE_OVS,3);

        HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_SMPS1_0, SMPS1_SS_EN, 1);
        HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_SMPS1_5, SMPS1_LPM_OVR,
                   3);  // follows TX enable signal from PHY
        HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_SMPS1_3, SMPS1_VSET_SEL, 0);
        HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_SMPS1_3, SMPS1_VSET_TRIM_SEL, 1);
        HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_FOOTSW, FOOTSW_CHRG_RATE, 0x2);

#ifdef HALMAC_UFW

        // need to be tuned per chip for RFA voltage is 0.87V,POR is 128

        HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_SMPS1_0, SMPS1_VSET_HIGH, 121);

        HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_SMPS1_0, SMPS1_VREF_TRIM,
                   17);  // need to change per chip to be 0.87V

        // HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET,RPMU_RPMU_R_PMU_ULPBG_1,SMPS1_ULP_COMP_VREF,0x20);// sleep 0.80V

#endif

        HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_SMPS2_0, SMPS2_SS_EN, 1);
        HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_SMPS2_2, SMPS2_CL_SEL_PFM,
                   0xf);  // Default is 0xb (increase the value, increase the loading capability)
        HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_SMPS2_4, SMPS2_CL_ILIM_MIN,
                   0x7);  // Default is 0xb (lower the value, increase current limit)
        HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_SMPS2_5, SMPS2_LPM_OVR, 3);  // PFM
        HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_SMPS2_3, SMPS2_VSET_TRIM_SEL, 1);
        HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_SMPS2_3, SMPS2_VSET_SEL,
                   cpr_mode);  // 0: open loop, 1: close loop
        if (cpr_mode == cpr_openloop) {
            HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_SMPS2_0, SMPS2_VSET_HIGH, 150);
        }
#ifdef HALMAC_UFW

        HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_SMPS2_0, SMPS2_VSET_HIGH,
                   101);  // need to change per chip to be 0.55V

        // HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET,RPMU_RPMU_R_PMU_ULPBG_1,SMPS2_ULP_COMP_VREF,0x2B);// chip to be 0.63V

#endif
    }

    HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_CORE_10, D_OSPARE_REG, 0x0);
    HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_CORE_9, MBG_DELAY_TO_SLEEP, 0x1);
    HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_CORE_10, LDOOSC_DIS, 0x1);
#if (FERMION_CHIP_VERSION == 1)
    HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_BROWN_DET, VBAT_BROWN_EN, 1);
    HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_BROWN_DET, VBAT_BROWN_SEL_ULP_BIAS, 1);
#endif
    HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_CORE_12, LDOAO_VIN_MODE,
               0x2);  // use SMPS_S1 output always except Indefinite deepsleep
    // VIFERMION-190
    HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_CORE_5, LDO_AON_INDEF_SLP_ONESHOT, 0x3F);

#if (FERMION_CHIP_VERSION == 1)
    uint32_t otp_tag_high = HWIO_INXF(
        SEQ_WCSS_OTP_OFFSET, FERMION_V1_0_QFPROM_RAW_FUSE_MAP_SECURITY_CONTROL_CORE_RAW_R_QFPROM_RAW_PTE_REGION_1_W3,
        TRIM_TAG_HIGH);
#else
    uint32_t otp_tag_high = HWIO_INXF(
        SEQ_WCSS_OTP_OFFSET, FERMION_V2_0_QFPROM_RAW_FUSE_MAP_SECURITY_CONTROL_CORE_RAW_R_QFPROM_RAW_PTE_REGION_1_W3,
        TRIM_TAG_HIGH);
#endif /* (FERMION_CHIP_VERSION == 1) */
    if (g_ferm_neutrino_mode == 0) {
        HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_CORE_10, D_OSPARE_REG, 0x8);  // bit3
        HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_FOOTSW, FOOTSW_CHRG_RATE, 0x20);

        HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_CORE_12, LDOCX_POK_FORCE, 1);
        HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_CORE_12, LDOCX_VSET_SEL, 1);
        if ((g_socpm_struct.cpr_cfg.ini_enabled == 0) || (otp_tag_high <= OTP_TRIM_TAG_HIGH)) {
            /* Setting LDOCX_VSET_TRIM_SEL to 0 if target is less than OTP v3 */
            HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_CORE_12, LDOCX_VSET_TRIM_SEL, 0);
        }

#if 0
        HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_CORE_5,LDO_AON_INDEF_SLP_COMP_OV,0);
        HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_CORE_5,LDO_AON_INDEF_SLP_LOOP_OV,0);
        HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_CORE_5,LDO_AON_INDEF_SLP_PG_LOW,4);
#endif
        HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_CORE_5, LDO_AON_INDEF_SLP_ONESHOT, 0x3F);
    }
    // use SLEEP default setting. Override with wifi_fw_pmic_pre_sleep_config(Standby) for deepsleep

    phyrf_temperature_data_otp_trim();
    wifi_fw_pmic_pre_sleep_config(mcu_sleep);

    hal_mac_hw_ctrl();
    wifi_fw_select_rc_sleep_clock();
#endif /* PLATFORM_INIT_PMIC */
}

/*
 * @brief: PMIC configurations to be done before entering sleep
 * @param : mode: sleep mode to be configured for
 * @return : none
 */
void wifi_fw_pmic_pre_sleep_config(sleep_mode mode)
{
#ifdef PLATFORM_INIT_PMIC
    uint32_t value;

    hal_mac_sw_powerup();
    switch (mode) {
        case Standby:
        case InfDeepsleep:
            if (g_ferm_neutrino_mode == 1) {
                HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_SMPS2_7, ULPM_SMPS2_SEG_EN, 0);
                HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_SMPS2_6, SMPS2_POK_FORCE, 0);
                HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_SMPS2_6, SMPS2_POK_DIS, 0);
                HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_SMPS2_0, SMPS2_FORCE_TARGET, 0);

                if (InfDeepsleep == mode) {
                    HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_SMPS1_7, ULPM_SMPS1_SEG_EN, 0);
                    HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_SMPS1_6, SMPS1_POK_FORCE, 0);
                    HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_SMPS1_6, SMPS1_POK_DIS, 0);
                    HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_SMPS1_0, SMPS1_FORCE_TARGET, 0);
                    HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_SMPS1_6, ULPM_SMPS1_DIS_OVR, 2);
                    HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_SMPS2_6, ULPM_SMPS2_DIS_OVR, 2);
                } else {
                    HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_SMPS2_6, ULPM_SMPS2_DIS_OVR, 1);
                    HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_SMPS1_0, SMPS1_FORCE_TARGET, 1);
                }
            } else {
                // VIFERMION-199
                HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_CORE_6, LDOCX_SS_EN, 1);
                HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_CORE_12, LDOCX_POK_FORCE, 0);
            }
            break;
        case mcu_sleep:
        case Lightsleep:
            if (g_ferm_neutrino_mode == 1) {
                HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_SMPS2_7, ULPM_SMPS2_SEG_EN, 0xFF);
                HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_SMPS1_7, ULPM_SMPS1_SEG_EN, 0x3);
                HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_SMPS1_0, SMPS1_FORCE_TARGET, 1);
                HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_SMPS1_2, SMPS1_CL_SEL_PFM, 0xF);
                HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_SMPS1_0, SMPS1_RAMP_PK, 1);
                HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_SMPS2_0, SMPS2_FORCE_TARGET, 1);
                // To keep backward compatibility on PBL w and w/o CONFIG_DISABLE_CURRENT_LIMIT
                value = HWIO_INXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_SMPS2_2, SMPS2_CL_OUT_GATE);
                if (value != 2) {
                    NT_LOG_PRINT(SOCPM, WARN, "APP disable current limit: %d => 2", value);
                    HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_SMPS2_2, SMPS2_CL_OUT_GATE, 2);
                } else {
                    NT_LOG_PRINT(SOCPM, WARN, "PBL disable current limit already");
                }
                HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_SMPS2_6, ULPM_SMPS2_DIS_OVR, 0);
            } else {
                HWIO_OUTXF(SEQ_WCSS_RPMU_OFFSET, RPMU_RPMU_R_PMU_CORE_6, LDOCX_SS_EN, 0);
            }
            break;
        default:
            NT_LOG_PRINT(SOCPM, WARN, "PMIC pre sleep config invoked for invalid sleep mode");
            break;
    }
    hal_mac_hw_ctrl();

#else  /* PLATFORM_INIT_PMIC */
    (void)mode;
#endif /* PLATFORM_INIT_PMIC */
}

/*
 * @brief: Select XO/1200 as sleep clock
 * @param : none
 * @return : none
 */
void wifi_fw_select_xo_sleep_clock(void)
{
    HWIO_OUTXF(SEQ_WCSS_PMU_OFFSET, NEUTRINO_PMU_PRONTO_LP_FRODO_PMU_AON_TOP_CFG, CFG_XO_SLP_CLK_SEL_EN, 1);
}

/*
 * @brief: Select RC OSC as sleep clock source
 * @param : none
 * @return : none
 */
void wifi_fw_select_rc_sleep_clock(void)
{
    HWIO_OUTXF(SEQ_WCSS_PMU_OFFSET, NEUTRINO_PMU_PRONTO_LP_FRODO_PMU_AON_TOP_CFG, CFG_XO_SLP_CLK_SEL_EN, 0);
}

#endif /* PLATFORM_FERMION */
