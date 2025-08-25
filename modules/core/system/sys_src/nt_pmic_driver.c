/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "nt_common.h"
#include "nt_logger_api.h"
#include "nt_devcfg.h"

#include "nt_hw.h"
#include "nt_hw_support.h"

#include "nt_pmic_driver.h"
#include "nt_socpm_sleep.h"

#ifdef NT_NEUTRINO_1_0
#include "halphy_api.h"
#endif  // NT_NEUTRINO_1_0

/*------------------------------MACROS----------------------------------------------- */

// local compile options
// sets XO settling time in AON
#define _PMIC_INC_XO_SETTLE_OPT

// set AON source for mem (default is smps2)
//#define _PMIC_INC_TST_MEMPWR_AON

//#define _TST_INC_RRAM_WR

// ------ test options start ---------------

// use tuned params for N12-758 (aon ldo, smps1 cal)
//#define _PMIC_INC_TST_N12   //490-debug H1D0131, also unset _PMIC_INC_TST_0P6_DFLT //configured for N12-758 debug
//board #define _PMIC_INC_TST_N18   //489-sleep H2D0026, also unset _PMIC_INC_TST_0P6_DFLT #define _PMIC_INC_TST_N18_1
////490-sleep H2D0233, set _PMIC_INC_TST_0P6_DFLT #define _PMIC_INC_TST_N18_2 //490-sleep H2D0214,  set
//_PMIC_INC_TST_0P6_DFLT #define _PMIC_INC_TST_SAMPLE_0142 // sampling board HD0142
// use hw defaults for smps2, do not touch the register at all
// #define _PMIC_INC_TST_0P6_DFLT

// force socpm standby entry just after pmic init - only for test purposes
//#define _PMIC_TST_MODE_CFG_SBY_INIT 1

// include AON PUPD configurations
#define _PMIC_INC_TST_PUPD_CFG

// disable pok-force - this helps resolve the need for double reset to restart the chip
//#define _PMIC_INC_TST_POK_FORCE_DISABLE

// force slp clk src selection; and set its value
//#define _PMIC_TST_INC_FORCE_SLP_CLK_SRC
//#define _PMIC_TST_INC_FORCE_SLP_CLK_SRC_VAL   NT_SOCPM_SLP_CLK_RC
//#define _PMIC_TST_INC_FORCE_SLP_CLK_SRC_VAL   NT_SOCPM_SLP_CLK_PMICXO

// do not enable brown out detect (use hw defaults instead)
#define _PMIC_TST_INC_NO_BROWN_OUT_DET

// ------ test options end ----------------

// set aon xo settle time (if enabled)
//#define _PMIC_PMU_AON_CFG_XO_SETTLE_TIME_CYCLES 35
#define _PMIC_PMU_AON_CFG_XO_SETTLE_TIME_CYCLES 35

#define NT_PMIC_VBAT_BROWN_OUT_EN_DATA \
    0x90  // bit(7) - vbat_brown_en, bits(4-0) - brown_detector_threshold_control(0x10)
#define NT_PMIC_PMIC_ULP_CONFIG_DATA_1  0x38        // ulp_bg_start_en and self_start_dis set to 1
#define NT_PMIC_PMIC_ULP_CONFIG_DATA_2  0x78        // ulp_bg_start_en, self_start_dis & ulp_bg_en set to 1
#define NT_PMIC_PMIC_ULP_CONFIG_DATA_3  0x68        // ulp_bg_en & self_start_dis set to 1
#define NT_PMIC_ULP_CONFIG_DATA_4       0x48        // self_start_dis set to 1
#define NT_PMIC_SMPS1_POK_FORCE_DATA    0x1         // smps1_pok_force set to 1
#define NT_PMIC_SMPS1_POK_GEN_OFF_DATA  0x3         // smps1_pok_force & smps1_pok_dis set to 1
#define NT_PMIC_SMPS2_POK_FORCE_DATA    0x1         // smps2_pok_force set to 1
#define NT_PMIC_SMPS2_POK_GEN_OFF_DATA  0x3         // smps2_pok_force & smps2_pok_dis set to 1
#define NT_PMIC_AONLDO_POK_FORCE        0x20808008  // AON_LDO_pok_force enabled
#define NT_PMIC_RFALDO_POK_FORCE        0x4040      // RFA_LDO_pok_force enabled
#define NT_PMIC_AONLDO_POK_GEN_OFF_DATA 0x20818008
#define NT_PMIC_RFALDO_POK_GEN_OFF_DATA 0x40C0
#define PMIC_32M_XO_ENABLE              0x50001
#define NT_PMIC_32M_RCOSC_DATA          0x40001

#define PMIC_start_addressCORE_11_DATA \
    0x4040  // rfaldo_pok_force enabled
            // smps_on_delay_from_sleep = 4 clock cycles
#define PMIC_start_addressCORE_10_DATA \
    0x40001                                    // bits(7-0) - 32k XO OSC trim(0x01)
                                               // bit(18)   - OSC LDO Bias selection (0 = MBG, 1= ULP)
#define NT_PMIC_SMPS1_ULPM_VREG_CAL 0x7208028  // bits(0-7)  - trim for ptat current tsense
                                               // bits(15-8) - neg tempco trim for ulpbg
                                               // bits(21-16)- vref select for smps2 ulp comp
                                               // bits(27-22)- vref select for smps1 ulp comp
#define NT_PMIC_SMPS1_ONE_SHOT_CODE 0X9        // bits(5-0)  - smps1 one shot trim
#define NT_PMIC_SMPS2_ONE_SHOT_CODE 0XB        // bits(5-0)  - smps2 one shot trim
#define NT_PMIC_AONLDO_VREG_CAL \
    0x1964B  // bits(7-0)  - ldoao low voltage set point(ldoao_vset_low)
             // bits(15-8) - ldoao high voltage set point(ldoao_vset_high)
             // bit(16)    - if set to 0, use trim value if trim came
             //              if set to 1, use ldoao_vset_high even after trim came
#define NT_PMIC_SMPS1_VREG_CAL_OVERRIDE 0X16160031  // SMPS1 Vreg cal override
#define NT_PMIC_SMPS1_VREG_CAL          0x80442304  // SMPS1 Vreg cal.
#define PMIC_start_addressCORE_7_DATA   0X40000000  // bit
#define PMIC_SMPS_PWM_PFM_MASK          0xFFE1FFFF  // Mask to disable the PWM/PFM bits
#define PMIC_SMPS_SET_PFM               0x00180000  // Mask to set the PFM mode
#define PMIC_SMPS_SET_PWM               0x00060000  // Mask to set the PWM mode

// SMPS1 SETTING
#define SMPS1_V_SET_SEL_FORCE_HIGH 0x3   // force high set point SMPS1_SET_VAL
#define SMPS1_VSET_HIGH_DATA       0X45  // high for 0.86v
#define SMPS1_VSET_HIGH_OFFSET     24    // offset for smps1_vset_high config bits
#define SMPS1_VSET_HIGH_MASK       0xFFFFFFCF
#define SMPS1_VREF_TRIM_MASK       0xFFFFFF07  ////TRIM data is user input
#define SMPS1_SET_VAL_MASK         0x00FFFFFF
#define SMPS1_VREF_TRIM_OFFSET     0x3  // offset for vref bits of smps1
#define SMPS1_PWM_PFM_MASK         0xFF7FFFFF
#define SMPS1_FORCE_ULPM           0x1  // Force smps1 into ULPM mode.

// SMPS2 SETTING
#define SMPS2_VOLTAGE_SET_POINT_DATA 0x3   // force high set point
#define SMPS2_VSET_SEL_OFFSET        4     // offset for smps2_set_val bits
#define SMPS2_VSET_HIGH_DATA         0x45  // high for 0.86v
#define SMPS2_VSET_HIGH_OFFSET       24    // offset for smps2_vset_high config bits
#define SMPS2_VSET_HIGH_MASK         0xFFFFFFCF
#define SMPS2_VREF_TRIM_MASK         0xFFFFFF07  // TRIM data is user input
#define SMPS2_SET_VAL_MASK           0x00FFFFFF
#define SMPS2_VREF_TRIM_OFFSET       0x3  // offset for vref bits of smps2
#define SMPS1_SET_PFM_FOR_SLEEP      0x1
#define PMIC_SMPS1_VSET_CAL_VAL      0x16150031

// AONLDO SETTING
#define PMIC_AONLDO_CTRL_MASK            0XDFFFFFFF
#define PMIC_AONLDO_CAL_MASK             0xFFFF00FF
#define PMIC_AONLDO_VREF_TRIM_BIT_OFFSET 0x100  // Mask to check if the ldo_aon_vset_high bit is enabled or not
#define PMIC_AONLDO_CAL_OFFSET           8      // offset to access calibration bits of aonldo
#define PMIC_AON_LDO_TO_SMPS1            0x20000000

#ifdef FR_HWIO_WAR
#define QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_PD_WLAN_PHY_RXTA_CNTL_BIT_MASK \
    QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_PD_WLAN_PHY_RXA_CNTL_BIT_MASK
#endif

// pmic cold boot

// pmic pmu address space
#define PMIC_PMU_START_OFFSET (SEQ_WCSS_RPMU_OFFSET)
#define PMIC_PMU_END_OFFSET   (SEQ_WCSS_RPMU_OFFSET + 0x100)  // 30bc is teh last address currently

// expected number of registers to be trimmed from rram
// used to determine if aon ldo, smps1, smps2 are trimmed
#define _PMIC_N_EXP_REG_TRIM 3

/*------------------------------------------------------------------------------------------------------------------*/

#ifdef NT_NEUTRINO_1_0
extern uint16_t halphy_program_trim_values(uint32_t start_base, uint32_t stop_base, bool debug);
#endif /* NT_NEUTRINO_1_0 */

/*---------------------------------------FUNCTION DEFINITIONS-----------------------------------------------------*/

/**
 * <!-- nt_pmic_init -->
 *
 * @brief Initializing PMIC configurations
 * @return void
 */
void nt_pmic_init(void)
{
    uint32_t regval;
    uint32_t reg_boot_cmpl;
    uint8_t sleep_clk_sel;

#ifndef _PMIC_TST_INC_FORCE_SLP_CLK_SRC
    uint8_t *devcfg_val;

    devcfg_val = ((uint8_t *)(nt_devcfg_get_config(NT_DEVCFG_SLEEP_CLOCK_SELECTION_FOR_AON)));
    if (devcfg_val == NULL) {
        //@TODO error handling
        sleep_clk_sel = NT_SOCPM_SLP_CLK_RC;  // use RC by default
    } else {
        sleep_clk_sel = *devcfg_val;
    }
#else
    sleep_clk_sel = _PMIC_TST_INC_FORCE_SLP_CLK_SRC_VAL;
#endif

    // turn on all wlan mac + phy : needed in order to access pmic reg space
    reg_boot_cmpl = NT_REG_RD(QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_REG);
    regval = reg_boot_cmpl;
    regval &= (~(QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_PD_WLAN_PHY_RXTOP_CNTL_BIT_MASK |
                 QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_PD_WLAN_PHY_RXTA_CNTL_BIT_MASK |
                 QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_PD_WLAN_PHY_TX_CNTL_BIT_MASK |
                 QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_PD_WLAN_MAC_CNTL_BIT_MASK));
    NT_REG_WR(
        QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_REG,
        (regval | (QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_PD_WLAN_PHY_RXTOP_CNTL_BIT_MASK |
                   QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_PD_WLAN_PHY_RXTA_CNTL_BIT_MASK |
                   QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_PD_WLAN_PHY_TX_CNTL_BIT_MASK |
                   QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_PD_WLAN_MAC_CNTL_BIT_MASK)));
    // delay(20000);
    for (int i = 0; i < 20000; i++) {
        asm volatile("nop");
    }

    NT_REG_WR(0x204305c, 0xed1fe000);  // Enabling SMPS2 for standby sleep exit

    NT_REG_WR(0x2043020, 0x16160031);
#if defined(NT_FN_CPR) && defined(NT_FN_SMPS2_RRAM_ATE_CFG)
    /*Reading the RRAM OTP region for SMPS2 configurations done from ATE and using it to configure
      SMPS2 before CPR gets enabled. Read data can be interpreted as follows
      Bits (31-24) : smps2_vset_high
           (23-16) : smps2_vset_low
           (15-12) : smps2_stepper_delay
           (11)    : smps2_stepper_step
           (10)    : smps2_force_target
           (9)     : smps2_vs_en
           (8)     : smps2_ss_en
           (7-3)   : smps2_vref_trim
           (2-1)   : smmps2_ramp_pk
           (0)     : smps2_vfb_sel
      */
    regval = NT_REG_RD(NT_PMIC_SMPS2_RRAM_ADDR);
    NT_REG_WR(NT_PMU_PMIC_CFG_SMPS2_0_REG, regval);
#else
    // SMPS-2 CX to 0.58 from 0.67
    NT_REG_WR(0x2043014, 0x81304305);
#endif

#ifndef _PMIC_TST_INC_NO_BROWN_OUT_DET
    /*  1 brown_det 0x204307C 0x90 Brown-out detector enable*/
    NT_REG_WR(NT_PMU_PMIC_CFG_BROWN_DET_REG, NT_PMIC_VBAT_BROWN_OUT_EN_DATA);
#endif

    /*  2 pmu_ulpbg_6 0x2043068 0x38 ULP bias enable */
    NT_REG_WR(NT_PMU_PMIC_CFG_ULPBG_6_REG, NT_PMIC_PMIC_ULP_CONFIG_DATA_1);

    /*  3 pmu_ulpbg_6 0x2043068 0x78 ULP bias enable */
    NT_REG_WR(NT_PMU_PMIC_CFG_ULPBG_6_REG, NT_PMIC_PMIC_ULP_CONFIG_DATA_2);

    for (int i = 0; i < 20000; i++) {
        asm volatile("nop");
    }

    /*  4 pmu_ulpbg_6 0x2043068 0x68 ULP bias enable */
    NT_REG_WR(NT_PMU_PMIC_CFG_ULPBG_6_REG, NT_PMIC_PMIC_ULP_CONFIG_DATA_3);

    for (int i = 0; i < 90000; i++) {
        asm volatile("nop");
    }

    /*  5 pmu_ulpbg_6 0x2043068 0x48 ULP bias enable */
    NT_REG_WR(NT_PMU_PMIC_CFG_ULPBG_6_REG, NT_PMIC_ULP_CONFIG_DATA_4);

    for (int i = 0; i < 180000; i++) {
        asm volatile("nop");
    }

    /*  6 pmu_smps1_6 0x204304C 0x1 POK force (SMPS1)*/
    NT_REG_WR(NT_PMU_PMIC_CFG_SMPS1_6_REG, NT_PMIC_SMPS1_POK_FORCE_DATA);

    /*  7 pmu_smps2_6 0x2043058 0x1 POK force (SMPS2)*/
    NT_REG_WR(NT_PMU_PMIC_CFG_SMPS2_6_REG, NT_PMIC_SMPS2_POK_FORCE_DATA);

    /*  8 PMU start addressCORE_9 0x2043064 0x2080 8008 POK force (AON LDO)*/
#ifdef _PMIC_INC_TST_POK_FORCE_DISABLE
    NT_REG_WR(NT_PMU_PMIC_CFG_CORE_9_REG, NT_PMIC_AONLDO_POK_FORCE & (~0x00018000));
#else
    NT_REG_WR(NT_PMU_PMIC_CFG_CORE_9_REG, NT_PMIC_AONLDO_POK_FORCE);
#endif

    /*  9 PMU start addressCORE_11 0x2043070 0xE040 POK force (RFA LDO) */
    NT_REG_WR(NT_PMU_PMIC_CFG_CORE_11_REG, NT_PMIC_RFALDO_POK_FORCE);

    /* 10 pmu_smps1_6 0x204304C 0x3 POK-gen off (SMPS1)*/
    NT_REG_WR(NT_PMU_PMIC_CFG_SMPS1_6_REG, NT_PMIC_SMPS1_POK_GEN_OFF_DATA);

    /* 11 PMU start addressCORE_9 0x2043058 0x3 POK-gen off (SMPS2)*/
    NT_REG_WR(NT_PMU_PMIC_CFG_SMPS2_6_REG, NT_PMIC_SMPS2_POK_GEN_OFF_DATA);

    /* 12 PMU start addressCORE_9 0x2043064 0x2081 8008 POK-gen off (AON LDO)*/

#ifdef _PMIC_INC_TST_POK_FORCE_DISABLE
    NT_REG_WR(NT_PMU_PMIC_CFG_CORE_9_REG, NT_PMIC_AONLDO_POK_GEN_OFF_DATA & (~0x00018000));
#else
    NT_REG_WR(NT_PMU_PMIC_CFG_CORE_9_REG, NT_PMIC_AONLDO_POK_GEN_OFF_DATA);
#endif

    /* 13 PMU start addressCORE_11 0x2043070 0xE0C0 POK-gen off (RFA LDO)*/
    NT_REG_WR(NT_PMU_PMIC_CFG_CORE_11_REG, NT_PMIC_RFALDO_POK_GEN_OFF_DATA);

    // [1] Prior to step '14', please switch AON clock source from 32K RC oscillator to 32M/1000 clock.
    // This is calibrating the sleep clock, so while calibrating the clock source shouldn't used
    regval = NT_REG_RD(QWLAN_PMU_AON_TOP_CFG_REG);
    regval |= QWLAN_PMU_AON_TOP_CFG_CFG_XO_SLP_CLK_SEL_EN_MASK;
    NT_REG_WR(QWLAN_PMU_AON_TOP_CFG_REG, regval);

    // delay(20000);
    for (int i = 0; i < 20000; i++) {
        asm volatile("nop");
    }

    /* 14 PMU start addressCORE_10 0x204306C 0x40001 32K RC osc bias source change*/
    NT_REG_WR(NT_PMU_PMIC_CFG_CORE_10_REG, NT_PMIC_32M_RCOSC_DATA);

    /* 15 pmu_ulpbg_1 0x2043038 0x720 8028 SMPS1/2 ULPM Vreg cal*/
    NT_REG_WR(NT_PMU_PMIC_CFG_ULPBG_1_REG, NT_PMIC_SMPS1_ULPM_VREG_CAL);

    /* 16 pmu_smps1_18 0x2043040 0x9 SMPS1 ULPM one-shot code Correct*/
    NT_REG_WR(NT_PMU_PMIC_CFG_SMPS1_18_REG, NT_PMIC_SMPS1_ONE_SHOT_CODE);

    /* 17 pmu_smps2_18 0x2043044 0xB SMPS2 ULPM one-shot code*/
    NT_REG_WR(NT_PMU_PMIC_CFG_SMPS2_18_REG, NT_PMIC_SMPS2_ONE_SHOT_CODE);

    /* 18 PMU start addressCORE_5 0x2043028 0x1964B AON LDO to ~0.80v Vreg cal*/
    NT_REG_WR(NT_PMU_PMIC_CFG_CORE_5_REG, NT_PMIC_AONLDO_VREG_CAL);

    /*19 PMU start addresssmps1_3 0x204300C 0x1616 0031 SMPS1 Vreg cal override*/
    NT_REG_WR(NT_PMU_PMIC_CFG_SMPS1_3_REG, NT_PMIC_SMPS1_VREG_CAL_OVERRIDE);

    /* 20 PMU start addresssmps1_0 0x2043000 0x80442304 SMPS1 trim value to ~0.86v*/
    NT_REG_WR(NT_PMU_PMIC_CFG_SMPS1_0_REG, NT_PMIC_SMPS1_VREG_CAL);

    if (sleep_clk_sel == NT_SOCPM_SLP_CLK_RFAXO)  // using XO from RFA as sleep clock for AON
    {
        // do nothing, already selected RFA XO
        NT_REG_WR(QWLAN_PMU_CFG_SLP_CAL_CAL_EN_REG, 0);
    } else if (sleep_clk_sel == NT_SOCPM_SLP_CLK_PMICXO)  // using XO from PMIC as sleep clock for AON
    {
#ifdef PLATFORM_NT
        NT_REG_WR(QWLAN_PMU_AON_TOP_CFG_REG,
                  QWLAN_PMU_AON_TOP_CFG_DEFAULT | QWLAN_PMU_AON_TOP_CFG_CFG_EXT_SLP_CLK_SEL_EN_MASK);
#else
        NT_REG_WR(QWLAN_PMU_AON_TOP_CFG_REG, QWLAN_PMU_AON_TOP_CFG_DEFAULT);
#endif
        for (int i = 0; i < 20000; i++) {
            asm volatile("nop");
        }
        NT_REG_WR(QWLAN_PMU_CFG_SLP_CAL_CAL_EN_REG, 0);
    } else  // Use RC from PMIC as sleep clock for AON
    {
        // For any invalid entries of sleep clock selection from dev_cfg, choose RC as sleep clock
        // switch xo-based sleepclk to rc based sleep clk
        regval = NT_REG_RD(QWLAN_PMU_AON_TOP_CFG_REG);
#ifdef PLATFORM_NT
        regval &=
            ~(QWLAN_PMU_AON_TOP_CFG_CFG_XO_SLP_CLK_SEL_EN_MASK | QWLAN_PMU_AON_TOP_CFG_CFG_EXT_SLP_CLK_SEL_EN_MASK);
#else
        regval &= ~(QWLAN_PMU_AON_TOP_CFG_CFG_XO_SLP_CLK_SEL_EN_MASK);
#endif
        // regval = regval | QWLAN_PMU_AON_TOP_CFG_CFG_EXT_SLP_CLK_SEL_EN_MASK;
        NT_REG_WR(QWLAN_PMU_AON_TOP_CFG_REG, regval);

        // delay(20000);
        for (int i = 0; i < 20000; i++) {
            asm volatile("nop");
        }

        NT_REG_WR(NT_PMU_PMIC_CFG_CORE_10_REG, 0x50001);  // Disabling XO sleep clock (bit 16) *it was 0x50001*.
    }

#ifdef _PMIC_INC_XO_SETTLE_OPT
    NT_REG_WR(QWLAN_PMU_CFG_XO_SETTLE_TIME_REG, _PMIC_PMU_AON_CFG_XO_SETTLE_TIME_CYCLES);
#endif

#ifdef _SOCPM_TST_SBY_SLP_CAL_DIS
    /* Sleep calibration is disabled in active/sleep mode */
    NT_REG_WR(QWLAN_PMU_CFG_SLP_CAL_CAL_EN_REG, ~QWLAN_PMU_CFG_SLP_CAL_CAL_EN_CFG_SLP_CLK_CAL_EN_MASK);
    /* SLP_CLK_CNT_REG (VI team code data) */
    NT_REG_WR(QWLAN_PMU_CFG_REF_SLP_CLK_CNT_REG, 0x20);
    /* CFG_CAL_DATA_REF VI team data copied */
    NT_REG_WR(QWLAN_PMU_CFG_CAL_DATA_REF_REG, 0x7a12);  // ref_data_cnt
#endif                                                  // _SOCPM_TST_SBY_SLP_CAL_DIS

#ifdef NT_CC_DEBUG_FLAG
    nt_socpm_footsw_state_set(1);
#endif

#ifdef NT_NEUTRINO_1_0
    // try to program smps1, smps2, aonldo trim values from rram - if available
    if (halphy_program_trim_values(PMIC_PMU_START_OFFSET, PMIC_PMU_END_OFFSET, 0) == _PMIC_N_EXP_REG_TRIM) {
        NT_REG_WR(0x2043060, 0x20008000);  // switch aon ldo i/p to smps1
        NT_REG_WR(0x2043048, 0x01000000);  // pwm-on-tx
    } else                                 // some/none of the trims available...
    {
#ifdef _PMIC_INC_TST_SAMPLE_0142
        {
            NT_REG_WR(0x2043000, 0x90442304);  // smps1 to 0.89+
            NT_REG_WR(0x2043028, 0x00017700);  // aonldo to 0.81+
            NT_REG_WR(0x2043060, 0x20008000);  // switch aon ldo i/p to smps1
            NT_REG_WR(0x2043048, 0x01000000);  // pwm-on-tx
#ifndef _PMIC_INC_TST_0P6_DFLT
            NT_REG_WR(0x2043014, 0x82304305);  // 0p6 to 0.57+
#endif
        }
#endif

#ifdef _PMIC_INC_TST_N12
        {
            NT_REG_WR(0x2043000, 0x90442304);  // smps1 to 0.86+
            NT_REG_WR(0x2043028, 0x00018800);  // aonldo to 0.8+
            NT_REG_WR(0x2043060, 0x20008000);  // switch aon ldo i/p to smps1
            NT_REG_WR(0x2043048, 0x01000000);  // pwm-on-tx
#ifndef _PMIC_INC_TST_0P6_DFLT
            NT_REG_WR(0x2043014, 0x70304305);  // 0p6 to 0.51+
#endif
        }
#else  //_PMIC_INC_TST_N12
        {
#ifdef _PMIC_INC_TST_N18
            NT_REG_WR(0x2043000, 0x72002304);
            NT_REG_WR(0x2043028, 0x00018820);
            NT_REG_WR(0x2043060, 0x20008000);  // switch aon ldo i/p to smps1
            NT_REG_WR(0x2043048, 0x01000000);  // pwm-on-tx

#ifndef _PMIC_INC_TST_0P6_DFLT
            NT_REG_WR(0x2043014, 0x63304305);  // 0p6 to 0.52+
#endif
#endif  //_PMIC_INC_TST_N18

#ifdef _PMIC_INC_TST_N18_1
            NT_REG_WR(0x204300C, 0x16160031);  // smps1 vset-sel
            NT_REG_WR(0x2043020, 0x16160031);  // smps2 vset-sel
            // NT_REG_WR(0x2043030, 0x16160031); //aon vset-sel
            NT_REG_WR(0x2043000, 0x95002304);  // 0.869
            NT_REG_WR(0x2043028, 0x0001864b);  // 0.80+
            NT_REG_WR(0x2043060, 0x20008000);  // switch aon ldo i/p to smps1
            NT_REG_WR(0x2043048, 0x01000000);  // pwm-on-tx
#endif  //_PMIC_INC_TST_N18_N

#ifdef _PMIC_INC_TST_N18_2
            NT_REG_WR(0x204300C, 0x16160031);  // smps1 vset-sel
            NT_REG_WR(0x2043020, 0x16160031);  // smps2 vset-sel
            // use defaults
            // NT_REG_WR(0x2043000, 0x95002304); //0.869
            NT_REG_WR(0x2043028, 0x0001764b);  // aon ldo 0.80+
            NT_REG_WR(0x2043060, 0x20008000);  // switch aon ldo i/p to smps1
            NT_REG_WR(0x2043048, 0x01000000);  // pwm-on-tx
#endif  //_PMIC_INC_TST_N18_N

            // NT_REG_WR(0x2043060, 0x20008000); // aon ldo i/p switch to smps //configured for N12-758 debug board
            // NT_REG_WR(0x2043048, 0x01000000); // pwm mode
        }
#endif  //_PMIC_INC_TST_N12
    }
    // NT_REG_WR(NT_PMU_PMIC_CFG_SMPS2_5_REG, 0x03000000); // smps2 lpm_high always

#endif  // NT_NEUTRINO_1_0

#ifdef _PMIC_INC_TST_MEMPWR_AON
    {   // set aon ldo as source for mem
        // Set (bit 2) vddmx_ctrl_sel = 1, and
        //    (bit 0) vddmx_ovr_tristate = 0.
        // If (bit 1) vddmx_ovr_p8v = 0/ 1, output vddmx is based on SMPS2/ AON LDO.
        uint32_t regval = NT_REG_RD(0x204308C);
        NT_REG_WR(0x204308C, (regval & (0xFFFFFFF8)) | 5);
    }
#endif  //_PMIC_INC_TST_MEMPWR_AON

    // restore the boot complete register to original state
    NT_REG_WR(QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_REG, reg_boot_cmpl);

#ifdef _PMIC_INC_TST_PUPD_CFG
    {
/*
// pupd configurations
// TDI/TMS/TRST_N pupd controls are flipped between registers (bits 14/15/17)

//NT_REG_WR(0x011af90c, 0);
//NT_REG_WR(0x011af910, 0);

//NT_REG_WR(0x011af90c, 0x00d80030);
//NT_REG_WR(0x011af910, 0x0603C000);

regval = NT_REG_RD(0x011af910); // pu
NT_REG_WR(0x011af910, regval & 0xF9F80000); //0-18,25,26 : no pu
regval = NT_REG_RD(0x011af90c);
NT_REG_WR(0x011af90c, regval | 0x0607FFFF); //0-18,25,26 : pd

regval = NT_REG_RD(0x011af90C); // pu
NT_REG_WR(0x011af90C, regval & 0xF9F80000); //0-18,25,26 : no pu
regval = NT_REG_RD(0x011af910);
NT_REG_WR(0x011af910, regval | 0x0607FFFF); //0-18,25,26 : pd

NT_REG_WR(0x011af90C,0); //no pu
NT_REG_WR(0x011af910,0); //no pd
NT_REG_WR(0x01233A04, 0x3FC0); //GPIO6-13: o/p
NT_REG_WR(0x01233A00, 0x400);

NT_REG_WR(0x011af90C, 0);      //no pu
NT_REG_WR(0x011af910, 0x3F);   //0-5 pd
NT_REG_WR(0x01233A04, 0x3FC0); //GPIO6-13: o/p
NT_REG_WR(0x01233A00, 0x400);

NT_REG_WR(0x011af90C, 0);      //no pu
NT_REG_WR(0x011af910, 0x0FFFFFFF);   //0-5 pd

Iio = 22uA*/
#if 0
		{
			regval = NT_REG_RD(0x011af90C); // pu
			NT_REG_WR(0x011af90C, regval & 0xF9F80000); //0-18,25,26 : no pu
			regval = NT_REG_RD(0x011af910);
			NT_REG_WR(0x011af910, (regval & (~ 0x0002C000)) | 0x06053FFF); //0-18,25,26 : pd, 14/15/17: no pu
			regval = NT_REG_RD(0x011af90C); // pu
			NT_REG_WR(0x011af90C, regval | 0x0002C000); //0-18,25,26 : no pu, 14/15/17: pd
		}
#else
        {
            /*
            // 16.9uA
            regval = NT_REG_RD(0x011af90C); // pu
            NT_REG_WR(0x011af90C, regval & 0xFFFFFD00); //0-18,25,26 : no pu
            regval = NT_REG_RD(0x011af910);
            NT_REG_WR(0x011af910, (regval | 0x000002FF)); //0-18,25,26 : pd, 14/15/17: no pu

            // 16.7uA
            regval = NT_REG_RD(0x011af90C); // pu
            NT_REG_WR(0x011af90C, regval & 0xFFFFFD00); //0-18,25,26 : no pu
            regval = NT_REG_RD(0x011af910);
            NT_REG_WR(0x011af910, (regval | 0x080002FF)); //0-18,25,26 : pd, 14/15/17: no pu
            */
            //
            // regval = NT_REG_RD(0x011af90C); // pu
            // NT_REG_WR(0x011af90C, regval & 0xFFFFFD00); //0-18,25,26 : no pu
            // regval = NT_REG_RD(0x011af910);
            // 100uA NT_REG_WR(0x011af910, (regval | 0x08E002FF)); //0-18,25,26 : pd, 14/15/17: no pu
            // 60uA NT_REG_WR(0x011af910, (regval | 0x088002FF)); //0-18,25,26 : pd, 14/15/17: no pu
            // 58uA NT_REG_WR(0x011af910, (regval | 0x084002FF)); //0-18,25,26 : pd, 14/15/17: no pu
            // 16.8uA NT_REG_WR(0x011af910, (regval | 0x082002FF)); //0-18,25,26 : pd, 14/15/17: no pu
            /*
             * 35uA
            regval = NT_REG_RD(0x011af90C); // pu
            NT_REG_WR(0x011af90C, regval & 0xFFFEFD00); //0-18,25,26 : no pu
            regval = NT_REG_RD(0x011af910);
            NT_REG_WR(0x011af910, (regval | 0x082802FF)); //0-18,25,26 : pd, 14/15/17: no pu
            */

            /* 17uA KS, 21uA KR
            regval = NT_REG_RD(0x011af90C); // pu
            NT_REG_WR(0x011af90C, (regval & 0xFFFEFD00) | 0x2C000); //0-18,25,26 : no pu
            regval = NT_REG_RD(0x011af910);
            NT_REG_WR(0x011af910, (regval | 0x082002FF) & 0xFFFD3FFF); //0-18,25,26 : pd, 14/15/17: no pu
            */
            /*
            // 20uA KR
            regval = NT_REG_RD(0x011af90C); // pu
            NT_REG_WR(0x011af90C, (regval & 0xFFFEFD00) | 0x2C000); //0-18,25,26 : no pu
            regval = NT_REG_RD(0x011af910);
            NT_REG_WR(0x011af910, (regval | 0x002002FF) & 0xFFFD3FFF); //0-18,25,26 : pd, 14/15/17: no pu
            */
            /*
            // 17uA KeySight
            regval = NT_REG_RD(0x011af90C); // pu
            NT_REG_WR(0x011af90C, (regval & 0xFFFEF900) | 0x2C000); //0-18,25,26 : no pu
            regval = NT_REG_RD(0x011af910);
            NT_REG_WR(0x011af910, (regval | 0x002002FF) & 0xFFFD3BFF); //0-18,25,26 : pd, 14/15/17: no pu
            */
            // 30uA KeySight
            // NT_REG_WR(0x011af90C, 0x001012FF);
            // NT_REG_WR(0x011af910, 0x0803CC00);
            // 30uA KeySight
            // NT_REG_WR(0x011af90C, 0x001002FF);
            // NT_REG_WR(0x011af910, 0x0803DC00);

            // NT_REG_WR(0x011af90C, 0x001002ED); //29uA
            // NT_REG_WR(0x011af910, 0x0803DC00);
            // NT_REG_WR(0x011af90C, 0x001002E1); //23uA
            // NT_REG_WR(0x011af910, 0x0803D800);
            // NT_REG_WR(0x011af90C, 0x001002E1); //11uA
            // NT_REG_WR(0x011af910, 0x0803C800);
            // NT_REG_WR(0x011af90C, 0x00100221); //11uA
            // NT_REG_WR(0x011af910, 0x0803C8C0);
            // NT_REG_WR(0x011af90C, 0x00100020); //11uA
            // NT_REG_WR(0x011af910, 0x0803CAC1);
            // NT_REG_WR(0x011af90C, 0x00100000); //11uA
            // NT_REG_WR(0x011af910, 0x0803CAC1);
            // NT_REG_WR(0x011af90C, 0x00100000); //11uA
            // NT_REG_WR(0x011af910, 0x0801CAC1);// bit 17, trst_n = 0
            // NT_REG_WR(0x011af90C, 0x00100000); //11uA
            // NT_REG_WR(0x011af910, 0x0801C2C1);// bit 11, clk-32-bypass-en no-pull
            // NT_REG_WR(0x011af90C, 0x00000000); //7uA
            // NT_REG_WR(0x011af910, 0x0801C2C1);// bit 11, clk-32-bypass-en no-pull
            // NT_REG_WR(0x011af90C, 0x00000000); //41uA
            // NT_REG_WR(0x011af910, 0x080102C1);// bit 14/15 tms/tck no-pull
            // NT_REG_WR(0x011af90C, 0x0000C000); // 1.72uA
            // NT_REG_WR(0x011af910, 0x080102C1);// bit 14/15 tms/tck pull-down

            NT_REG_WR(0x011af90C, 0x0000C000);  //
            NT_REG_WR(0x011af910, 0x080302C1);  // trst_n internal pullup (bit 17)

            // NT_REG_WR(0x011af90C, 0x001002C1);
            // NT_REG_WR(0x011af910, 0x0803C00C);
            // NT_REG_WR(0x011af900, 1);
            // NT_REG_WR(0x011af904, 1);

            for (uint32_t i = 0; i < 20000000; i++) {
                asm volatile("nop");
            }
            // while (1);

            /* 105uA
            regval = NT_REG_RD(0x011af90C); // pu
            NT_REG_WR(0x011af90C, (regval & 0xFFFEFD00) | 0x2C000); //0-18,25,26 : no pu
            regval = NT_REG_RD(0x011af910);
            NT_REG_WR(0x011af910, (regval | 0x08E002FF) & 0xFFFD3FFF); //0-18,25,26 : pd, 14/15/17: no pu
            */
            /* 100uA
            //regval = NT_REG_RD(0x011af90C); // pu
            NT_REG_WR(0x011af90C, 0x00180000); //0-18,25,26 : no pu
            //regval = NT_REG_RD(0x011af910);
            NT_REG_WR(0x011af910, ~0x00180000); //0-18,25,26 : pd, 14/15/17: no pu
            */
        }
#endif

        // NT_REG_WR(0x011af900, 1); // AON retain GPIO DD

        /*Iio = 22uA
        regval = NT_REG_RD(0x011af90C); // pu
        NT_REG_WR(0x011af90C, regval & 0xF9F80000); //0-18,25,26 : no pu
        regval = NT_REG_RD(0x011af910);
        NT_REG_WR(0x011af910, (regval & (~ 0x0002C000)) | 0x06003FFF); //0-18,25,26 : pd, 14/15/17: no pu, 16,18:float
        regval = NT_REG_RD(0x011af90C); // pu
        NT_REG_WR(0x011af90C, regval | 0x0002C000); //0-18,25,26 : no pu, 14/15/17: pd
        */

        /* Iio = 130uA
        NT_REG_WR(0x011af90C,0); //no pu
        NT_REG_WR(0x011af910,0); //no pd
        NT_REG_WR(0x01233A04, 0x3FC0); //GPIO6-13: o/p
        NT_REG_WR(0x01233A00, 0x400);
        */
    }
#endif

    // delay(20000);
    for (int i = 0; i < 20000; i++) {
        asm volatile("nop");
    }

//
#ifdef _TST_INC_RRAM_WR
    for (int i = 0; i < 200000; i++) {
        asm volatile("nop");
    }
    // volatile uint32_t *test_addr = (uint32_t *) 0x2fc000;
    volatile uint32_t *test_addr = (uint32_t *)0x2532b0;
    *test_addr = 0x55555555;
    *test_addr = 0xAAAAAAAA;
    while (1)
        ;
#endif

#if _PMIC_TST_MODE_CFG_SBY_INIT == 1
    nt_socpm_tst_standby(5000);
#endif
}

/**
 * <!-- nt_pmic_get -->
 *
 * @brief To read and print the calibration values of smps1,smps2 and aonldo based on the request from CLI
 * @return void
 */
void nt_pmic_get(uint8_t option, uint8_t type)
{
    uint32_t reg_value;
    if (option == NT_PMIC_OPT_CAL) {
        if (type == NT_PMIC_CAL_SMPS1) {
            reg_value = NT_REG_RD(NT_PMU_PMIC_CFG_SMPS1_0_REG);
            reg_value &= (NT_PMU_PMIC_SMPS1_VSET_HIGH_MASK << NT_PMU_PMIC_SMPS1_VSET_HIGH_OFFSET);
            NT_LOG_SYSTEM_INFO("SMPS1 cal value =", (reg_value >> NT_PMU_PMIC_SMPS1_VSET_HIGH_OFFSET), 0, 0);
        } else if (type == NT_PMIC_CAL_SMPS2) {
            reg_value = NT_REG_RD(NT_PMU_PMIC_CFG_SMPS1_0_REG);
            reg_value &= (NT_PMU_PMIC_SMPS2_VSET_HIGH_MASK << NT_PMU_PMIC_SMPS2_VSET_HIGH_OFFSET);
            NT_LOG_SYSTEM_INFO("SMPS2 cal value =", (reg_value >> NT_PMU_PMIC_SMPS2_VSET_HIGH_OFFSET), 0, 0);
        } else if (type == NT_PMIC_CAL_AONLDO) {
            uint32_t reg_value;
            reg_value = NT_REG_RD(NT_PMU_PMIC_CFG_CORE_5_REG);
            reg_value &= (NT_PMU_PMIC_LDO_AON_VSET_HIGH_MASK << NT_PMU_PMIC_LDO_AON_VSET_HIGH_OFFSET);
            NT_LOG_SYSTEM_INFO("AONLDO cal value =", (reg_value >> NT_PMU_PMIC_LDO_AON_VSET_HIGH_OFFSET), 0, 0);
        }
    }
}

/**
 * <!-- nt_pmic_set -->
 *
 * @brief Set PMIC registers(cal value(smps1,smps2,aonldo), switching between vbatt and smps and switching between PFM
 * and PWM mode) from CLI
 * @return void
 */
void nt_pmic_set(uint8_t option, uint8_t type, uint32_t value)
{
    if (option == NT_PMIC_OPT_CAL) {
        if (type == (NT_PMIC_CAL_SMPS1 || NT_PMIC_CAL_SMPS2)) {
            nt_pmic_set_smps_cal(type, value);
        } else if (type == NT_PMIC_CAL_AONLDO) {
            nt_pmic_set_aonldo_cal(value);
        }
    } else if (option == NT_PMIC_OPT_AONSRC) {
        nt_pmic_aonsrc_ctrl(type);
    } else if (option == NT_PMIC_OPT_AONMODE) {
        nt_pmic_pfm_pwm_ctrl(type, value);
    } else if (option == NT_PMIC_OPT_STATE) {
        nt_pmic_get(option, type);
    }
}

/**
 * <!-- nt_pmic_set_smps_cal -->
 *
 * @brief Set   : smps calibration values from the CLI input. By default it will set smps1_vset_sel = 3
 * @param type : 0 - smps1
 *               1 - smps2
 * @param value: calibration value
 * @return void
 */
void nt_pmic_set_smps_cal(uint8_t type, uint8_t value)
{
    uint32_t reg_value;
    if (type == NT_PMIC_CAL_SMPS1) {
        /*Setting the smps1_vset_high to take the high value without disturbing any other bits*/
        reg_value = NT_REG_RD(NT_PMU_PMIC_CFG_SMPS1_3_REG);
        reg_value &= ~(NT_PMU_PMIC_SMPS1_VSET_SEL_MASK << NT_PMU_PMIC_SMPS1_VSET_SEL_OFFSET);
        NT_REG_WR(NT_PMU_PMIC_CFG_SMPS1_3_REG,
                  SMPS1_V_SET_SEL_FORCE_HIGH << NT_PMU_PMIC_SMPS1_VSET_SEL_OFFSET | reg_value);
        /*Setting the smps1_vset_val (calibration value from CLI) without disturbing any other bits */
        reg_value = NT_REG_RD(NT_PMU_PMIC_CFG_SMPS1_0_REG);
        reg_value &= ~(NT_PMU_PMIC_SMPS1_VSET_HIGH_MASK << NT_PMU_PMIC_SMPS1_VSET_HIGH_OFFSET);
        NT_REG_WR(NT_PMU_PMIC_CFG_SMPS1_0_REG, (value << NT_PMU_PMIC_SMPS1_VSET_HIGH_OFFSET) | reg_value);
    } else if (type == NT_PMIC_CAL_SMPS2) {
        /*Setting the smps2_vset_high to take the high value without disturbing any other bits*/
        reg_value = NT_REG_RD(NT_PMU_PMIC_CFG_SMPS2_3_REG);
        reg_value &= ~(NT_PMU_PMIC_SMPS2_VSET_SEL_MASK << NT_PMU_PMIC_SMPS2_VSET_SEL_OFFSET);
        NT_REG_WR(NT_PMU_PMIC_CFG_SMPS2_3_REG,
                  (NT_PMU_PMIC_SMPS2_VSET_SEL_MASK << NT_PMU_PMIC_SMPS2_VSET_SEL_OFFSET) | reg_value);
        /*Setting the smps2_vset_val (calibration value from CLI) without disturbing any other bits */
        reg_value = NT_REG_RD(NT_PMU_PMIC_CFG_SMPS2_0_REG);
        reg_value &= ~(NT_PMU_PMIC_SMPS2_VSET_HIGH_MASK << NT_PMU_PMIC_SMPS2_VSET_HIGH_OFFSET);
        NT_REG_WR(NT_PMU_PMIC_CFG_SMPS2_0_REG, (value << NT_PMU_PMIC_SMPS2_VSET_HIGH_OFFSET) | reg_value);
    }
}

/**
 * <!-- nt_pmic_set_aonldo_cal -->
 *
 * @brief Set   : aonldo calibration values from the CLI input
 * @param value : calibration value
 * @return void
 */
void nt_pmic_set_aonldo_cal(uint8_t value)
{
    uint32_t reg_value;
    // setting the calibration value for AON LDO
    reg_value = NT_REG_RD(NT_PMU_PMIC_CFG_CORE_5_REG);
    reg_value &= ~(NT_PMU_PMIC_LDO_AON_VSET_HIGH_MASK << NT_PMU_PMIC_LDO_AON_VSET_HIGH_OFFSET);
    NT_REG_WR(NT_PMU_PMIC_CFG_CORE_5_REG, reg_value | value);
}

/**
 * <!-- nt_pmic_aonsrc_ctrl -->
 *
 * @brief Set   : To switch the aonsrc control between vbatt and smps1 based on the value from CLI input
 * @param type  : 0 = Use vbatt
 *                1 = Use smps1
 * @return void
 */

void nt_pmic_aonsrc_ctrl(uint8_t type)
{
    uint32_t reg_value;
    if (type == 0) {
        // clearing the 29th bit to set VBATT3 as LDO input
        reg_value = NT_REG_RD(NT_PMU_PMIC_CFG_CORE_7_REG);
        reg_value &= PMIC_AONLDO_CTRL_MASK;
        NT_REG_WR(NT_PMU_PMIC_CFG_CORE_7_REG, reg_value);
    } else {
        // setting the 29th bit to set SMPS1 as LDO input
        reg_value = NT_REG_RD(NT_PMU_PMIC_CFG_CORE_7_REG);
        reg_value &= PMIC_AONLDO_CTRL_MASK;
        NT_REG_WR(NT_PMU_PMIC_CFG_CORE_7_REG,
                  reg_value | NT_PMU_PMIC_LDOAO_VIN_MODE_MASK << NT_PMU_PMIC_LDOAO_VIN_MODE_OFFSET);
    }
}

/**
 * <!-- nt_pmic_pfm_pwm_ctrl -->
 *
 * @brief Set   : To switch the voltage controlling mode(either PWM or PFM) based on the value from CLI input
 * @param type  : 0 = Use PWM
 *                1 = Use PFM
 * @return void
 */
void nt_pmic_pfm_pwm_ctrl(uint8_t type, uint32_t value)
{
    uint32_t reg_value;
    if (type == NT_PMIC_CAL_SMPS1) {
        if (value == NT_PMIC_VCTL_PWM) {
            // clearing the smps1_cl_sel_pfm bit to set PWM mode
            reg_value = NT_REG_RD(NT_PMU_PMIC_CFG_SMPS1_2_REG);
            reg_value &= ~(NT_PMU_PMIC_SMPS1_CL_SEL_PFM_MASK << NT_PMU_PMIC_SMPS1_CL_SEL_PFM_OFFSET);
            NT_REG_WR(NT_PMU_PMIC_CFG_SMPS1_2_REG, reg_value);
        } else if (value == NT_PMIC_VCTL_PFM) {
            // setting the smps1_cl_sel_pfm bit to set PFM mode
            reg_value = NT_REG_RD(NT_PMU_PMIC_CFG_SMPS1_2_REG);
            reg_value &= ~(NT_PMU_PMIC_SMPS1_CL_SEL_PFM_MASK << NT_PMU_PMIC_SMPS1_CL_SEL_PFM_OFFSET);
            NT_REG_WR(NT_PMU_PMIC_CFG_SMPS1_2_REG,
                      reg_value | NT_PMU_PMIC_SMPS1_CL_SEL_PFM_MASK << NT_PMU_PMIC_SMPS1_CL_SEL_PFM_OFFSET);
        }
    } else if (type == NT_PMIC_CAL_SMPS2) {
        if (value == NT_PMIC_VCTL_PWM) {
            // clearing the smps2_cl_sel_pfm bit to set PWM mode
            reg_value = NT_REG_RD(NT_PMU_PMIC_CFG_SMPS2_2_REG);
            reg_value &= ~(NT_PMU_PMIC_SMPS2_CL_SEL_PFM_SEL_MASK << NT_PMU_PMIC_SMPS2_CL_SEL_PFM_SEL_OFFSET);
            NT_REG_WR(NT_PMU_PMIC_CFG_SMPS2_2_REG, reg_value);
        } else if (value == NT_PMIC_VCTL_PFM) {
            // setting the smps2_cl_sel_pfm bit to set PFM mode
            reg_value = NT_REG_RD(NT_PMU_PMIC_CFG_SMPS2_2_REG);
            reg_value &= ~(NT_PMU_PMIC_SMPS2_CL_SEL_PFM_SEL_MASK << NT_PMU_PMIC_SMPS2_CL_SEL_PFM_SEL_OFFSET);
            NT_REG_WR(NT_PMU_PMIC_CFG_SMPS2_2_REG,
                      reg_value | NT_PMU_PMIC_SMPS2_CL_SEL_PFM_SEL_MASK << NT_PMU_PMIC_SMPS2_CL_SEL_PFM_SEL_OFFSET);
        }
    }
}

/**
 * <!-- nt_pmic_pre_sleep_config -->
 *
 * @brief Set   : Configurations for PMIC before going to sleep
 * @return void
 */
void nt_pmic_pre_sleep_config(void)
{
    uint32_t reg_read;
    /* Disable 32K RC Oscillator*/
    reg_read = NT_REG_RD(NT_PMU_PMIC_CFG_CORE_10_REG);
    reg_read &= ~(NT_PMU_PMIC_ROSC_32K_DIS_MASK << NT_PMU_PMIC_ROSC_32K_DIS_OFFSET);
    NT_REG_WR(NT_PMU_PMIC_CFG_CORE_10_REG,
              reg_read | (NT_PMU_PMIC_ROSC_32K_DIS_MASK << NT_PMU_PMIC_ROSC_32K_DIS_OFFSET));
    /* Enable 32K XO Oscillator*/
    reg_read = NT_REG_RD(NT_PMU_PMIC_CFG_CORE_10_REG);
    reg_read &= ~(NT_PMU_PMIC_XO_OSC_32K_DIS_MASK << NT_PMU_PMIC_XO_OSC_32K_DIS_OFFSET);
    NT_REG_WR(NT_PMU_PMIC_CFG_CORE_10_REG, reg_read);
    /*Turning off smps2( SMPS2 would move from either PWM or PFM (whichever setting we are using
       in functional mode) to sleep mode condition (either ULPM or off-state, depending on ulpm_smps2_seg_en))*/
    reg_read = NT_REG_RD(NT_PMU_PMIC_CFG_SMPS2_7_REG);
    reg_read &= ~(NT_PMU_PMIC_ULPM_SMPS2_SEG_EN_MASK);
    NT_REG_WR(NT_PMU_PMIC_CFG_SMPS2_7_REG, reg_read);
    /* Disabling smps2_pok_force (Sun's reply to Mohan on Friday, March 12, 2021 7:21 AM)*/
    reg_read = NT_REG_RD(NT_PMU_PMIC_CFG_SMPS2_6_REG);
    reg_read &= ~(NT_PMU_PMIC_SMPS2_POK_FORCE_MASK);
    NT_REG_WR(NT_PMU_PMIC_CFG_SMPS2_6_REG, reg_read);
}

/**
 * <!-- nt_pmic_smps2_rram_cfg -->
 *
 * @brief: Write the initial smps2 voltage into RRAM OTP region from CLI
 * @param data:  32 bit data from the CLI which should be written in RRAM
 * @return void
 */
void nt_pmic_smps2_rram_cfg(uint32_t data)
{
    NT_REG_WR(NT_PMIC_SMPS2_RRAM_ADDR, data);
}
