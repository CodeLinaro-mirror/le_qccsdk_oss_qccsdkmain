/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "fwconfig_cmn.h"
#include "nt_flags.h"

#ifdef NT_FN_CPR
/*---------------------------------------------------Header
 * files------------------------------------------------------------*/
#include "nt_hw.h"
#include "nt_cpr_driver.h"
#include "nt_hw_support.h"
#include "wlan_dev.h"
#include "nt_logger_api.h"
#include "stdio.h"
#include "prof_ferm.h"
/*---------------------------------------------------------------------------------------------------------------------------*/

/*------------------------------------------------------Macros---------------------------------------------------------------*/
#define NT_CPR_HW_STEP_QUOT_MIN             0x14  // Expected initial Step quotient min value for one step PMIC VDD change
#define NT_CPR_HW_STEP_QUOT_MAX             0x14  // Expected initial Step quotient max value for one step PMIC VDD change
#define NT_CPR_HW_STEP_QUOT_MIN_CLEAR_MASK  0x3F  // Mask to clear the step quot min value from the register
#define NT_CPR_HW_STEP_QUOT_MAX_CLEAR_MASK  0x3F  // Mask to clear the step quot max value from the register
#define NT_CPR_GATE_COUNT_BASE_ADDRESS      QWLAN_CPR_WRAPPER_R_CPR_GCNT0_REG
#define NT_CPR_TARGET_QUOTIENT_BASE_ADDRESS QWLAN_CPR_WRAPPER_R_CPR_TARGET0_0_0_REG
#define NT_CPR_RO_GATE_CONT                 0x5F  // Gate count for RO's
#define NT_CPR_FSM_IDLE_CLOCK \
    (0x1F << QWLAN_CPR_WRAPPER_R_CPR_FSM_CTL_IDLE_CLOCKS_OFFSET)  // Setting FSM idle clock for CPR
#define NT_CPR_SENSOR_MASK          ~(0xF)                        // N1.0 CPR uses only four sensors
#define NT_CPR_SCLK_EXPECTED_CONFIG 0x18  // Expected clock count for the RO (Taken from VI code)
#define NT_CPR_CX_CEILING_VOLTAGE \
    0xB0304305  // Ceiling voltage of CX(0.671v as observed on Kratos) thread: TODO yet to get the data from hw team
#define NT_CPR_CX_FLOOR_VOLTAGE \
    0x68304305  // Floor voltage of CX(0.491v as observed on Kratos) thread: TODO yet to get the data from hw team
#define NT_PMIC_STEP_SIZE_VOLTAGE \
    0x0  // Step voltage which needs to be added or subtracted from the CX rail based on CPR results
#define NT_PMIC_CX_VDD_ADJUSTMENT_OFFSET \
    0x01000000                              // Offset ask to increment or decrement the CX rail viltage by 2.5mV
#define NT_PMIC_DEFAULT_VOLTAGE_CX 0.55481  // as seen from kratos for the value 0x81 for SMPS2
/*---------------------------------------------------------------------------------------------------------------------------*/

/*---------------------------------------------------Globals-----------------------------------------------------------------*/
// uint32_t target_values[16] = {119,109,115,47,433,257,227,66,93,297,177,146,40,15,75,63};//Data from PTE(Min) for
// gate_cnt 1F
uint32_t target_values[16] = {357, 327, 345, 141, 1299, 771, 681, 198,
                              279, 891, 531, 438, 120,  45,  225, 189};  // PTE(Min) for gate_cnt 5F
// uint32_t target_values[16] = {595,545,575,235,2165,1285,1135,330,465,1485,885,730,200,75,375,315};//Data from
// PTE(Min) for gate_cnt scaled target quot 0x9F(5uS)
/*---------------------------------------------------------------------------------------------------------------------------*/

/**
 * <!-- nt_cpr_init -->
 *
 * @brief Initializing CPR configurations such as writing TARGET_QUOT values, step_quot_init, GCNT, timer interval, etc
 * into respective registers.
 * @return void
 */
void nt_cpr_init(void)
{
    uint32_t reg_val = 0;
    uint32_t step_quotient_val, ro_count;

    /*
     * co-power reduction version number,
     * Indicates what version of the CPR hardware is in palce on this chip.
     *
     */
    reg_val = NT_REG_RD(QWLAN_CPR_WRAPPER_R_CPR_VERSION_REG);
#ifdef NT_DEBUG
    // NT_LOG_SYSTEM_INFO("CPR version number \r\n",reg_val,0,0);
#endif
    // Checking if the RAM bank A is powered up
    reg_val = NT_REG_RD(QWLAN_PMU_CMEM_BANK_A_GDSCR_REG);
    while (!((NT_REG_RD(QWLAN_PMU_CMEM_BANK_A_GDSCR_REG)) & QWLAN_PMU_CMEM_BANK_A_GDSCR_GDS_CTL_PWR_STATUS_MASK))
        ;

    // Clear the loop_en bit
    NT_REG_WR(QWLAN_CPR_WRAPPER_R_CPR_FSM_CTL_REG, QWLAN_CPR_WRAPPER_R_CPR_FSM_CTL_LOOP_EN_DEFAULT);

    for (int i = 0; i < 20000; i++) {
        asm volatile("nop");
    }
    // Soft reset of CPR
    // NT_REG_WR(QWLAN_PMU_SOFT_RESET_REG, QWLAN_PMU_SOFT_RESET_CPR_SOFT_RESET_MASK);

    // Initializing root clocks for CPR module
    reg_val = NT_REG_RD(QWLAN_PMU_ROOT_CLK_ENABLE_REG);
    reg_val &= ~QWLAN_PMU_ROOT_CLK_ENABLE_UART_ROOT_CLK_ENABLE_MASK;
    NT_REG_WR(QWLAN_PMU_ROOT_CLK_ENABLE_REG, reg_val);

    // Initializing FSM clock
    NT_REG_WR(QWLAN_CPR_WRAPPER_R_CPR_FSM_CTL_REG, NT_CPR_FSM_IDLE_CLOCK);

    /*Set timer clamp interval: If clamp signals,sensor mask, or sensor bypass changes,
     *loop_en will be turned off for this refclk signals TODO Set to default value for now*/
    NT_REG_WR(QWLAN_CPR_WRAPPER_R_CPR_TIMER_CLAMP_REG, QWLAN_CPR_WRAPPER_R_CPR_TIMER_CLAMP_DEFAULT);
    /* Masking RO's: Enabling only 0,1,5,6,10 and 11 only*/
    NT_REG_WR(QWLAN_CPR_WRAPPER_R_CPR_MASK_THREAD__MREG, 0xF39C);
    // Initializing step quotient
    NT_REG_WR(
        QWLAN_CPR_WRAPPER_R_CPR_STEP_QUOT_INIT_REG,
        ~(NT_CPR_HW_STEP_QUOT_MIN_CLEAR_MASK << QWLAN_CPR_WRAPPER_R_CPR_STEP_QUOT_INIT_STEP_QUOT_INIT_MIN_OFFSET) |
            ~(NT_CPR_HW_STEP_QUOT_MAX_CLEAR_MASK << QWLAN_CPR_WRAPPER_R_CPR_STEP_QUOT_INIT_STEP_QUOT_INIT_MAX_OFFSET));
    step_quotient_val = (NT_CPR_HW_STEP_QUOT_MIN << QWLAN_CPR_WRAPPER_R_CPR_STEP_QUOT_INIT_STEP_QUOT_INIT_MIN_OFFSET) |
                        (NT_CPR_HW_STEP_QUOT_MAX << QWLAN_CPR_WRAPPER_R_CPR_STEP_QUOT_INIT_STEP_QUOT_INIT_MAX_OFFSET);
    NT_REG_WR(QWLAN_CPR_WRAPPER_R_CPR_STEP_QUOT_INIT_REG, step_quotient_val);

    // Initializing gate count for 16 Ring oscillators and updating the target quotient values
    for (ro_count = 0; ro_count < 16; ro_count++) {
        NT_REG_WR((NT_CPR_GATE_COUNT_BASE_ADDRESS + ro_count * 4), NT_CPR_RO_GATE_CONT);
        NT_REG_WR((NT_CPR_TARGET_QUOTIENT_BASE_ADDRESS + ro_count * 4), target_values[ro_count]);
    }
    reg_val = 0;
    reg_val = NT_REG_RD(QWLAN_CPR_WRAPPER_R_CPR_THRESHOLD__MREG);
    reg_val |= (0x2 << QWLAN_CPR_WRAPPER_R_CPR_THRESHOLD__UP_THRESHOLD_OFFSET) |
               (0x2 << QWLAN_CPR_WRAPPER_R_CPR_THRESHOLD__DN_THRESHOLD_OFFSET);
    reg_val &= (~(0xF << QWLAN_CPR_WRAPPER_R_CPR_THRESHOLD__CONSECUTIVE_UP_OFFSET));
    reg_val &= (~(0xF << QWLAN_CPR_WRAPPER_R_CPR_THRESHOLD__CONSECUTIVE_DN_OFFSET));
    NT_REG_WR(QWLAN_CPR_WRAPPER_R_CPR_THRESHOLD__MREG, reg_val);

    // Mask the unwanted sensors: We're using only 4 sensors for N1_0 CPR implementation
    NT_REG_WR(QWLAN_CPR_WRAPPER_R_CPR_SENSOR_MASK_WRITE_MREG, NT_CPR_SENSOR_MASK);
    // nt_cpr_init_poll();
    // Setting the timer interval for CPR meausurements (giving time for PMIC VDD change to settle down)
    reg_val = NT_REG_RD(NT_NVIC_ISER2);
    reg_val |= ENABLE_CPR_INTERRUPT;
    NT_REG_WR(NT_NVIC_ISER2, reg_val);
    // Enable the interrupts(only up and down flags since it's mission mode)
    NT_REG_WR(QWLAN_CPR_WRAPPER_R_CPR_IRQ_EN__MREG,
              QWLAN_CPR_WRAPPER_R_CPR_IRQ_EN__UP_FLAG_EN_MASK | QWLAN_CPR_WRAPPER_R_CPR_IRQ_EN__DOWN_FLAG_EN_MASK);
    // Set the loop_en bit
    reg_val = NT_REG_RD(QWLAN_CPR_WRAPPER_R_CPR_FSM_CTL_REG);
    NT_REG_WR(QWLAN_CPR_WRAPPER_R_CPR_FSM_CTL_REG, reg_val | QWLAN_CPR_WRAPPER_R_CPR_FSM_CTL_LOOP_EN_MASK);

    for (int i = 0; i < 20000; i++) {
        asm volatile("nop");
    }
}

void nt_cpr_init_poll(void)
{
    uint32_t reg_read;
    // Clear the interrupts
    NT_REG_WR(QWLAN_CPR_WRAPPER_R_CPR_IRQ_CLEAR__MREG,
              QWLAN_CPR_WRAPPER_R_CPR_IRQ_CLEAR__CLAMP_CHANGE_WHILE_BUSY_CLEAR_MASK |
                  QWLAN_CPR_WRAPPER_R_CPR_IRQ_CLEAR__STARVATION_ERROR_CLEAR_MASK |
                  QWLAN_CPR_WRAPPER_R_CPR_IRQ_CLEAR__UP_FLAG_CLEAR_MASK |
                  QWLAN_CPR_WRAPPER_R_CPR_IRQ_CLEAR__MID_FLAG_CLEAR_MASK |
                  QWLAN_CPR_WRAPPER_R_CPR_IRQ_CLEAR__DOWN_FLAG_CLEAR_MASK |
                  QWLAN_CPR_WRAPPER_R_CPR_IRQ_CLEAR__CPR_DONE_CLEAR_MASK);
    // Configuring the expected clock count for the sensor enumeration check and enabling the en_check for sensors
    NT_REG_WR(QWLAN_CPR_WRAPPER_R_CPR_BIST_CHAIN_CHECK0_REG,
              QWLAN_CPR_WRAPPER_R_CPR_BIST_CHAIN_CHECK0_EN_CHAIN_CHECK_MASK | NT_CPR_SCLK_EXPECTED_CONFIG);
    // Set the loop_en bit
    reg_read = NT_REG_RD(QWLAN_CPR_WRAPPER_R_CPR_FSM_CTL_REG);
    NT_REG_WR(QWLAN_CPR_WRAPPER_R_CPR_FSM_CTL_REG, reg_read | QWLAN_CPR_WRAPPER_R_CPR_FSM_CTL_LOOP_EN_MASK);
    for (int i = 0; i < 20000; i++) {
        asm volatile("nop");
    }
    // Polling for the sensor enumeration check
    while (!(NT_REG_RD(QWLAN_CPR_WRAPPER_R_CPR_BIST_CHAIN_CHECK1_REG) &
             QWLAN_CPR_WRAPPER_R_CPR_BIST_CHAIN_CHECK1_SCLK_CNT_DONE_MASK))
        ;
    reg_read = NT_REG_RD(QWLAN_CPR_WRAPPER_R_CPR_BIST_CHAIN_CHECK1_REG);
    if (((reg_read & QWLAN_CPR_WRAPPER_R_CPR_BIST_CHAIN_CHECK1_SCLK_CNT0_MASK) == NT_CPR_SCLK_EXPECTED_CONFIG) ||
        (reg_read & QWLAN_CPR_WRAPPER_R_CPR_BIST_CHAIN_CHECK1_SCLK_CNT1_MASK) ==
            (NT_CPR_SCLK_EXPECTED_CONFIG << QWLAN_CPR_WRAPPER_R_CPR_BIST_CHAIN_CHECK1_SCLK_CNT1_OFFSET)) {
#ifdef NT_DEBUG
        NT_LOG_SYSTEM_INFO("SCLK count is as expected\r\n", 0, 0, 0);
#endif
    } else {
#ifdef NT_DEBUG
        NT_LOG_SYSTEM_INFO("SCLK count is not as expected\r\n", 0, 0, 0);
#endif
    }
    // Disabling the loop_en after the polling has been done
    reg_read = NT_REG_RD(QWLAN_CPR_WRAPPER_R_CPR_FSM_CTL_REG);
    NT_REG_WR(QWLAN_CPR_WRAPPER_R_CPR_FSM_CTL_REG, reg_read & ~(QWLAN_CPR_WRAPPER_R_CPR_FSM_CTL_LOOP_EN_MASK));
    for (int i = 0; i < 20000; i++) {
        asm volatile("nop");
    }
}

/**
 * <!-- nt_cpr_pmic_vdd_adjustment -->
 *
 * @brief: to read the CX voltage rail based and increment or decrement the rail
 * voltage based on the triggered interrupt
 * @return void
 */
void nt_cpr_pmic_vdd_adjustment(uint8_t val)
{
    uint32_t reg_read;
    // Commented out to avoid the hard fault which occurs during UDP uplink traffic (Line no: 177-183,249)
    // turn on all wlan mac + phy : needed in order to access pmic reg space
    //	reg_boot_cmpl = NT_REG_RD(QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_REG);
    //	reg_read = reg_boot_cmpl;
    //	NT_REG_WR(QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_REG, ( reg_read |
    //			(QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_PD_WLAN_PHY_RXTOP_CNTL_BIT_MASK
    //					|
    //QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_PD_WLAN_PHY_RXTA_CNTL_BIT_MASK 					|
    //QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_PD_WLAN_PHY_TX_CNTL_BIT_MASK
    //					|QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_PD_WLAN_MAC_CNTL_BIT_MASK))
    //);
    if (val == VDD_DOWN) {
        /* TODO Read the current CX rail voltage from the PMIC and check if it is possible
         * to decrease by one pmic step size, by comparing it with the floor voltage. If after the decrease
         * in cx rail voltage reaches the floor voltage, disable the down interrupt for the
         * further cpr measurements*/
        reg_read = NT_REG_RD(NT_PMU_PMIC_CFG_SMPS2_0_REG);

        if ((reg_read)-NT_PMIC_CX_VDD_ADJUSTMENT_OFFSET >= NT_CPR_CX_FLOOR_VOLTAGE) {
            reg_read = reg_read -
                       NT_PMIC_CX_VDD_ADJUSTMENT_OFFSET;  //@TODO Equation to change the CX voltage is yet to be added
            NT_REG_WR(NT_PMU_PMIC_CFG_SMPS2_0_REG, reg_read);

            //#ifdef NT_FN_CPR_DEBUG
            //			WLAN_DBG1_PRINT("VDD down, SMPS2(0x2043014) reg value = r\n",reg_read);
            //#endif//NT_FN_CPR_DEBUG

            NT_REG_WR(QWLAN_CPR_WRAPPER_R_CPR_CONT_CMD__MREG,
                      QWLAN_CPR_WRAPPER_R_CPR_CONT_CMD__VDD_CHANGED_ONE_STEP_MASK);
            /* If after decrementing the CX rail voltage reaches the floor value, disable the down interrupt
             * for further cpr measurements.*/
            if (reg_read <= NT_CPR_CX_FLOOR_VOLTAGE)  // TODO Condition check has to be modified.
            {
                reg_read = NT_REG_RD(QWLAN_CPR_WRAPPER_R_CPR_IRQ_EN__MREG);
                reg_read &= ~QWLAN_CPR_WRAPPER_R_CPR_IRQ_EN__DOWN_FLAG_EN_MASK;
                NT_REG_WR(QWLAN_CPR_WRAPPER_R_CPR_IRQ_EN__MREG, reg_read);
            }

        } else {
            NT_REG_WR(QWLAN_CPR_WRAPPER_R_CPR_CONT_CMD__MREG,
                      ~QWLAN_CPR_WRAPPER_R_CPR_CONT_CMD__VDD_CHANGED_ONE_STEP_MASK);
        }

    } else if (val == VDD_UP) {
        /* TODO Read the current CX rail voltage from the PMIC and check if it is possible
         * to increase by one pmic step size, by comparing it with the ceiling voltage. If after the increment
         * in cx rail voltage reaches the ceiling voltage, disable the up interrupt for the
         * further cpr measurements*/
        reg_read = NT_REG_RD(NT_PMU_PMIC_CFG_SMPS2_0_REG);

        if ((reg_read + NT_PMIC_CX_VDD_ADJUSTMENT_OFFSET) <= NT_CPR_CX_CEILING_VOLTAGE) {
            reg_read = reg_read +
                       NT_PMIC_CX_VDD_ADJUSTMENT_OFFSET;  //@TODO Equation to change the CX voltage is yet to be added
            NT_REG_WR(NT_PMU_PMIC_CFG_SMPS2_0_REG, reg_read);

            //#ifdef NT_FN_CPR_DEBUG
            //			WLAN_DBG1_PRINT("VDD UP, SMPS2(0x2043014) reg value = \r\n",reg_read);
            //#endif//NT_FN_CPR_DEBUG

            NT_REG_WR(QWLAN_CPR_WRAPPER_R_CPR_CONT_CMD__MREG,
                      QWLAN_CPR_WRAPPER_R_CPR_CONT_CMD__VDD_CHANGED_ONE_STEP_MASK);
            if (reg_read >= NT_CPR_CX_CEILING_VOLTAGE)  // TODO Condition check has to be modified.
            {
                reg_read = NT_REG_RD(QWLAN_CPR_WRAPPER_R_CPR_IRQ_EN__MREG);
                reg_read &= (~(QWLAN_CPR_WRAPPER_R_CPR_IRQ_EN__UP_FLAG_EN_MASK));
                NT_REG_WR(QWLAN_CPR_WRAPPER_R_CPR_IRQ_EN__MREG, reg_read);
            }
        } else {
            NT_REG_WR(QWLAN_CPR_WRAPPER_R_CPR_CONT_CMD__MREG,
                      ~QWLAN_CPR_WRAPPER_R_CPR_CONT_CMD__VDD_CHANGED_ONE_STEP_MASK);
        }
    }
    // restore the boot complete register to original state
    // NT_REG_WR(QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_REG, reg_boot_cmpl);
}

void nt_cpr_isr_handler(void)
{
    PROF_IRQ_ENTER();

    uint32_t irq_status;  // hold on let me add something here0 we are checking up/down interrupt mainly
    irq_status = NT_REG_RD(QWLAN_CPR_WRAPPER_R_CPR_IRQ_STATUS__MREG);
    // Checking the IRQ status of CPR for down interrupt
    if (irq_status & QWLAN_CPR_WRAPPER_R_CPR_IRQ_STATUS__CPR_DONE_MASK) {
        /* TODO clear the cpr done*/
        NT_REG_WR(QWLAN_CPR_WRAPPER_R_CPR_IRQ_CLEAR__MREG, QWLAN_CPR_WRAPPER_R_CPR_IRQ_CLEAR__CPR_DONE_CLEAR_MASK);
    }
    if (irq_status & QWLAN_CPR_WRAPPER_R_CPR_IRQ_STATUS__DOWN_FLAG_MASK) {
        /* TODO We have to read the current CX rail voltage and add a condition here to check if the down operation
         * is possible or not? Something like (current voltage - 1 pmic step size <= floor voltage )*/
        nt_cpr_pmic_vdd_adjustment(VDD_DOWN);
        NT_REG_WR(QWLAN_CPR_WRAPPER_R_CPR_IRQ_CLEAR__MREG, QWLAN_CPR_WRAPPER_R_CPR_IRQ_CLEAR__DOWN_FLAG_CLEAR_MASK |
                                                               QWLAN_CPR_WRAPPER_R_CPR_IRQ_CLEAR__CPR_DONE_CLEAR_MASK);
    }
    if (irq_status & QWLAN_CPR_WRAPPER_R_CPR_IRQ_STATUS__UP_FLAG_MASK) {
        /* TODO We have to read the current CX rail voltage and add a condition here to check if the down operation
         * is possible or not? Something like (current voltage + 1 pmic step size >= ceiling voltage )*/
        nt_cpr_pmic_vdd_adjustment(VDD_UP);
        NT_REG_WR(QWLAN_CPR_WRAPPER_R_CPR_IRQ_CLEAR__MREG, QWLAN_CPR_WRAPPER_R_CPR_IRQ_CLEAR__UP_FLAG_CLEAR_MASK |
                                                               QWLAN_CPR_WRAPPER_R_CPR_IRQ_CLEAR__CPR_DONE_CLEAR_MASK);
    }
    if (irq_status & QWLAN_CPR_WRAPPER_R_CPR_IRQ_STATUS__MID_FLAG_MASK) {
        /*Here we need to add only that acknowledgement */
        NT_REG_WR(QWLAN_CPR_WRAPPER_R_CPR_IRQ_CLEAR__MREG, QWLAN_CPR_WRAPPER_R_CPR_IRQ_CLEAR__MID_FLAG_CLEAR_MASK |
                                                               QWLAN_CPR_WRAPPER_R_CPR_IRQ_CLEAR__CPR_DONE_CLEAR_MASK);
    }

    PROF_IRQ_EXIT();
}

void nt_cpr_pre_sleep_config(void)
{
    uint32_t reg_read;

    // Disable CPR interrupt
    reg_read = NT_REG_RD(NT_NVIC_ISER2);
    reg_read &= ~(ENABLE_CPR_INTERRUPT);
    NT_REG_WR(NT_NVIC_ISER2, reg_read);

    // disable loop_en
    reg_read = NT_REG_RD(QWLAN_CPR_WRAPPER_R_CPR_FSM_CTL_REG);
    reg_read = reg_read & ~(QWLAN_CPR_WRAPPER_R_CPR_FSM_CTL_LOOP_EN_MASK);
    NT_REG_WR(QWLAN_CPR_WRAPPER_R_CPR_FSM_CTL_REG, reg_read);
    for (int i = 0; i < 20000; i++) {
        asm volatile("nop");
    }
    // root clock disable for CPR
    reg_read = NT_REG_RD(QWLAN_PMU_ROOT_CLK_ENABLE_REG);
    reg_read = reg_read & ~(QWLAN_PMU_ROOT_CLK_ENABLE_CPR_XO_ROOT_CLK_ENABLE_MASK |
                            QWLAN_PMU_ROOT_CLK_ENABLE_CPR_AHB_ROOT_CLK_ENABLE_MASK);
    NT_REG_WR(QWLAN_PMU_ROOT_CLK_ENABLE_REG, reg_read);

    // Setting the CX rail to ~0.6v before entering DTIM sleep to ensure safe voltage for memory retention
    NT_REG_WR(0x2043014, 0x90304305);
}
void nt_cpr_post_sleep_config(void)
{
    uint32_t reg_val;
    // Initializing root clocks for CPR module
    NT_REG_WR(QWLAN_PMU_ROOT_CLK_ENABLE_REG, QWLAN_PMU_ROOT_CLK_ENABLE_CPR_XO_ROOT_CLK_ENABLE_MASK |
                                                 QWLAN_PMU_ROOT_CLK_ENABLE_CPR_AHB_ROOT_CLK_ENABLE_MASK);
    reg_val = NT_REG_RD(NT_NVIC_ISER2);
    reg_val |= ENABLE_CPR_INTERRUPT;
    NT_REG_WR(NT_NVIC_ISER2, reg_val);
    // Enable the interrupts(only up and down flags since it's mission mode)
    NT_REG_WR(QWLAN_CPR_WRAPPER_R_CPR_IRQ_EN__MREG,
              QWLAN_CPR_WRAPPER_R_CPR_IRQ_EN__UP_FLAG_EN_MASK | QWLAN_CPR_WRAPPER_R_CPR_IRQ_EN__DOWN_FLAG_EN_MASK);
    // Set the loop_en bit
    reg_val = NT_REG_RD(QWLAN_CPR_WRAPPER_R_CPR_FSM_CTL_REG);
    NT_REG_WR(QWLAN_CPR_WRAPPER_R_CPR_FSM_CTL_REG, reg_val | QWLAN_CPR_WRAPPER_R_CPR_FSM_CTL_LOOP_EN_MASK);

    for (int i = 0; i < 20000; i++) {
        asm volatile("nop");
    }
}
#endif  // NT_FN_CPR
