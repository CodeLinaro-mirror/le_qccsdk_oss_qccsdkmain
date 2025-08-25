/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "nt_cc_battery_driver.h"

#include <stdio.h>
#include "nt_hw.h"
#include "nt_common.h"
#include "nt_osal.h"
#include "nt_logger_api.h"
#include "ExceptionHandlers.h"
#include "nt_devcfg.h"

#include "nt_cc_batt_mng.h"
#include "nt_sys_monitoring.h"

#if ((defined NT_FN_CC_MGMT) && (defined NT_HOSTLESS_SDK))
#ifdef FR_HWIO_WAR
#define QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_PD_WLAN_PHY_RXTA_CNTL_BIT_MASK \
    QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_PD_WLAN_PHY_RXA_CNTL_BIT_MASK
#endif

uint32_t *ret_batt_level;
extern uint8_t rint_status_flag;
nt_cc_mgmt_t *head = NULL;
uint32_t cc_battery_level;
/**
 * <!-- nt_lut_map -->
 *
 * @brief              : Convert recovery time into battery level based on the look up table.
 * @param recovery_time: recovery time in micro seconds
 * @return             : void
 */
uint16_t nt_lut_map(uint32_t recovery_time)
{
    uint32_t lut_map[11][2] = {{100, 100}, {110, 85}, {120, 80}, {130, 70}, {140, 60}, {150, 50},
                               {160, 40},  {170, 30}, {180, 20}, {190, 15}, {200, 10}};
    uint8_t i, j = 0;
    for (i = 0; i < 11; i = (uint8_t)(i + 2)) {
        if (lut_map[i][j] == recovery_time) {
            return (uint16_t)lut_map[i][1];
        }
    }
    return 0;
}

/**
 * <!-- nt_cc_battery_mgmt_init -->
 *
 * @brief Initialize the configurations for vbatt and temperature measurements and interrupt registers
 * @return: void
 */
void nt_cc_battery_mgmt_init(void)
{
    uint32_t value;
    // enum measure type;

    // Active state sleep clock calibration is completed and it will be cleared when read on this register
    do {
        value = NT_REG_RD(QWLAN_PMU_CFG_ACAL_VBAT_MON_EN_REG);
        value &= QWLAN_PMU_CFG_ACAL_VBAT_MON_EN_SLP_CAL_DONE_STATUS_MASK;
    } while (value);
    /*   Disable the temperature measurements and
     *   Battery voltage measurements and
     *   Sleep clock calibration and
     *  Temperature measure event trigger intervel count value wrt to xo clock (32MHz)
     */
    value &= (uint32_t)((~(1 << QWLAN_PMU_CFG_ACAL_VBAT_MON_EN_TEMP_MON_EN_OFFSET)) |
                        (~(1 << QWLAN_PMU_CFG_ACAL_VBAT_MON_EN_AUTO_TEMP_MON_EN_OFFSET)) |
                        (~(1 << QWLAN_PMU_CFG_ACAL_VBAT_MON_EN_VBAT_MON_EN_OFFSET)) |
                        (~(1 << QWLAN_PMU_CFG_ACAL_VBAT_MON_EN_SLP_CAL_ENABLE_ACTIVE_OFFSET)));
    NT_REG_WR(QWLAN_PMU_CFG_ACAL_VBAT_MON_EN_REG, value);
    //(or)
    // NT_REG_WR(NT_CFG_ACAL_VBAT_MON_EN_REG,NT_CFG_ACAL_VBAT_MON_DISABLE)
    // Disable the interrupts
    NT_REG_WR(QWLAN_TPE_INTERRUPT_ENABLE_REG, QWLAN_TPE_INTERRUPT_ENABLE_RINT_VBATT_TIMEOUT_INT_EN_DEFAULT);
    /* Enable the rint vbatt time_out interrupts,
     * Rint vbatt interrupt enable and
     * Txop vbatt intrrupt enable
     */
    value = NT_REG_RD(QWLAN_TPE_INTERRUPT_ENABLE_REG);
    value |= (1 << QWLAN_TPE_INTERRUPT_ENABLE_RINT_VBATT_TIMEOUT_INT_EN_OFFSET) |
             (1 << QWLAN_TPE_INTERRUPT_ENABLE_RINT_VBATT_INIT_EN_OFFSET) |
             (1 << QWLAN_TPE_INTERRUPT_ENABLE_TXOP_VBATT_INT_EN_OFFSET);
    NT_REG_WR(QWLAN_TPE_INTERRUPT_ENABLE_REG, value);

    // nt_pmu_vbatt_temp_get(type);
    nt_tpe_rint_init();
    nt_tpe_txop_init();
}

/**
 * <!-- nt_pmu_vbatt_temp_get -->
 *
 * @brief      : To get the current temperature or voltage of coin cell battery
 * @param type : 0 for temperature measurement
                 1 for voltage measurement
 * @return: Temperature in celcius or voltage in volts
 */
float nt_pmu_vbatt_temp_get(type_t type)
{
    uint32_t value;
    if (type == TEMP_MEASURE) {
        //  Temperature monitoring
        value = NT_REG_RD(QWLAN_PMU_CFG_HKADC_DATA_AVG_CNT_REG);
        value &= (uint32_t)(~(QWLAN_PMU_CFG_HKADC_DATA_AVG_CNT_CFG_TEMP_VBATT_MON_SEL_MASK));
        NT_REG_WR(QWLAN_PMU_CFG_HKADC_DATA_AVG_CNT_REG, value);
        // Active state sleep clock calibration is completed and it will be cleared when read on this register
        do {
            value = NT_REG_RD(QWLAN_PMU_CFG_ACAL_VBAT_MON_EN_REG);
            value &= QWLAN_PMU_CFG_ACAL_VBAT_MON_EN_SLP_CAL_DONE_STATUS_MASK;
        } while (value);
        value |= (1 << QWLAN_PMU_CFG_ACAL_VBAT_MON_EN_TEMP_MON_EN_OFFSET);  // Enable the temparature monitoring bit.
        NT_REG_WR(QWLAN_PMU_CFG_ACAL_VBAT_MON_EN_REG, value);
        do {
            value = NT_REG_RD(QWLAN_PMU_TEMP_SNR_RD_DATA_REG);
            value &= (QWLAN_PMU_TEMP_SNR_RD_DATA_TC_MEASURED_DATA_VALID_MASK);
        } while (value);
        value = NT_REG_RD(QWLAN_PMU_TEMP_SNR_RD_DATA_REG);
        value =
            (uint32_t)(((value - LOWEST_TEMPERATURE_REG_READ) * TEMPERATURE_INCREMENT) + LOWEST_TEMPERATURE_IN_CELSIUS);
    } else {
        // Battery voltage monitoring
        value = NT_REG_RD(QWLAN_PMU_CFG_ACAL_VBAT_MON_EN_REG);
        value |= QWLAN_PMU_CFG_HKADC_DATA_AVG_CNT_CFG_TEMP_VBATT_MON_SEL_MASK;
        NT_REG_WR(QWLAN_PMU_CFG_HKADC_DATA_AVG_CNT_REG, value);
        // Active state sleep clock calibration is completed and it will be cleared when read on this register
        do {
            value = NT_REG_RD(QWLAN_PMU_CFG_ACAL_VBAT_MON_EN_REG);
            value &= QWLAN_PMU_CFG_ACAL_VBAT_MON_EN_SLP_CAL_DONE_STATUS_MASK;
        } while (value);
        value |= (1 << QWLAN_PMU_CFG_ACAL_VBAT_MON_EN_VBAT_MON_EN_OFFSET);  // Enable the battery voltage monitor bit.
        NT_REG_WR(QWLAN_PMU_CFG_ACAL_VBAT_MON_EN_REG, value);

        // Read data is valid and it will be cleared when read on this register
        do {
            value = NT_REG_RD(QWLAN_PMU_VBAT_MON_RD_DATA_REG);
            value &= (QWLAN_PMU_VBAT_MON_RD_DATA_VBAT_MON_DATA_VALID_MASK);
        } while (value);
        value = NT_REG_RD(QWLAN_PMU_VBAT_MON_RD_DATA_REG);  // Battery Voltage measured through HKADC
        value = (uint32_t)(((value - LOWEST_VOLTAGE_REG_READ) * VOLTAGE_INCREMENT) + LOWEST_VOLTAGE_IN_VOLTS);
    }

    return (float)value;
}

/**
 * <!-- nt_tpe_txop_init -->
 *
 * @brief      : To initialize txop configurations to control the transmission rate
 * @return     : void
 */
void nt_tpe_txop_init(void)
{
#ifdef NT_FN_SYSMON
    uint32_t value = 0, temp_config = 0;

    /*Plugging in PMIC Voltage sensor signals to HKADC*/
    nt_sysmon_hkadc_input_snr_sel(HKADC_VOLTAGE);

    /*Enabling Vbatt monitoring for each TXOP */
    value = NT_REG_RD(QWLAN_TPE_SW_TXOP_VBATT_REG);
    value |= QWLAN_TPE_SW_TXOP_VBATT_TXOP_VBATT_EN_MASK;
    NT_REG_WR(QWLAN_TPE_SW_TXOP_VBATT_REG, value);

    /* Configuring rint_vbatt_threshold*/
    value = NT_REG_RD(QWLAN_TPE_SW_TXOP_VBATT_REG);

#ifndef EMULATION_BUILD
    /* For Emulation setting the Threshold value to 0 */
    temp_config = *((uint32_t *)(nt_devcfg_get_config(NT_DEVCFG_TRANSMIT_INTERRUPT_THRESHOLD_CONFIG)));
#endif

    value &= ~(QWLAN_TPE_SW_TXOP_VBATT_TXOP_VBATT_THRESHOLD_MASK);

    value |= temp_config << QWLAN_TPE_SW_TXOP_VBATT_TXOP_VBATT_THRESHOLD_OFFSET;

    NT_REG_WR(QWLAN_TPE_SW_TXOP_VBATT_REG, value);

    /* Configuring txop_vbatt_average_samples*/
    value = NT_REG_RD(QWLAN_TPE_SW_TXOP_VBATT_REG);
    temp_config = *((uint32_t *)(nt_devcfg_get_config(NT_DEVCFG_TXOP_VBATT_AVERAGE_SAMPLES_CONFIG)));
    value &= ~(QWLAN_TPE_SW_TXOP_VBATT_TXOP_VBATT_AVERAGE_SAMPLES_MASK);
    value |= temp_config << QWLAN_TPE_SW_TXOP_VBATT_TXOP_VBATT_AVERAGE_SAMPLES_OFFSET;
    NT_REG_WR(QWLAN_TPE_SW_TXOP_VBATT_REG, value);

    /* Configuring txop_vbatt_min_samples*/
    value = NT_REG_RD(QWLAN_TPE_SW_TXOP_VBATT_REG);
    value &= ~(QWLAN_TPE_SW_TXOP_VBATT_TXOP_VBATT_MIN_SAMPLES_MASK);
    value |= VBATT_TXOP_MIN_SAMPLES_CONFIG << QWLAN_TPE_SW_TXOP_VBATT_TXOP_VBATT_MIN_SAMPLES_OFFSET;
    NT_REG_WR(QWLAN_TPE_SW_TXOP_VBATT_REG, value);

    /*Enable TxOP adjustment Interrupts in TPE*/
    value = NT_REG_RD(QWLAN_TPE_INTERRUPT_ENABLE_REG);
    value |= QWLAN_TPE_INTERRUPT_ENABLE_TXOP_VBATT_INT_EN_MASK;
    NT_REG_WR(QWLAN_TPE_INTERRUPT_ENABLE_REG, value);

    /*Enabling Txop adjustment Interrupts in MCU*/
    value = NT_REG_RD(QWLAN_MCU_IRQ_EN_REG);
    value |= QWLAN_MCU_IRQ_EN_TPE_IRQ_EN_MASK;
    NT_REG_WR(QWLAN_MCU_IRQ_EN_REG, value);

    /*enabling TXOP iRQ in NVIC*/
    nt_enable_device_irq(WLAN_ccu_irq);
#endif
}

/**
 * <!-- nt_tpe_rint_init -->
 *
 * @brief      : To initialize rint configurations for battery level
 * @return     : void
 */
void nt_tpe_rint_init(void)
{
#ifdef NT_FN_SYSMON
    uint32_t value = 0, temp_config = 0;

    value = NT_REG_RD(QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_REG);

    /*enabling WLAN_PHY_RX_TOP_CNTL_BIT*/
    value |= QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_PD_WLAN_PHY_RXTOP_CNTL_BIT_MASK;
    NT_REG_WR(QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_REG, value);

    /*enabling WLAN_PHY_RXTA_CNTL_BIT*/
    value |= QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_PD_WLAN_PHY_RXTA_CNTL_BIT_MASK;
    NT_REG_WR(QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_REG, value);

    /*enabling WLAN_PHY_TX_CNTL_BIT*/
    value |= QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_PD_WLAN_PHY_TX_CNTL_BIT_MASK;
    NT_REG_WR(QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_REG, value);

    /*enabling WLAN_MAC_CNTL_BIT*/
    value |= QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_PD_WLAN_PHY_TX_CNTL_BIT_MASK;
    NT_REG_WR(QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_REG, value);
    //-------------------------------------------------------------------------------------------------------------------
    /*Enable monitoring window for rint measurement*/
    value = NT_REG_RD(QWLAN_TPE_SW_RINT_VBATT_REG);
    value |= QWLAN_TPE_SW_RINT_VBATT_RINT_VBATT_EN_MASK;
    NT_REG_WR(QWLAN_TPE_SW_RINT_VBATT_REG, value);

    /*Enable monitoring window for rint measurement*/
    value = NT_REG_RD(QWLAN_TPE_SW_RINT_VBATT_REG);
    value |= QWLAN_TPE_SW_RINT_VBATT_RINT_VBATT_WINDOW_MON_EN_MASK;
    NT_REG_WR(QWLAN_TPE_SW_RINT_VBATT_REG, value);

    /* Configuring rint_vbatt_timeout*/
    value = NT_REG_RD(QWLAN_TPE_SW_RINT_VBATT_REG);
    temp_config = *((uint32_t *)(nt_devcfg_get_config(NT_DEVCFG_RINT_MEASUREMENT_TIME_OUT_CONFIG)));
    value &= ~(QWLAN_TPE_SW_RINT_VBATT_RINT_VBATT_TIMEOUT_MASK);
    value |= (temp_config << QWLAN_TPE_SW_RINT_VBATT_RINT_VBATT_TIMEOUT_OFFSET);
    NT_REG_WR(QWLAN_TPE_SW_RINT_VBATT_REG, value);

    /*Plugging in PMIC Voltage sensor signals to HKADC*/
    nt_sysmon_hkadc_input_snr_sel(HKADC_VOLTAGE);

    /*configuring reg to monitor Vbatt at the beginning of Tx, to absorb latency of HKADC*/
    value = NT_REG_RD(QWLAN_TPE_SW_RINT_VBATT_REG);
    value |= QWLAN_TPE_SW_RINT_VBATT_RINT_VBATT_EARLY_MON_EN_MASK;
    NT_REG_WR(QWLAN_TPE_SW_RINT_VBATT_REG, value);

    /* Configuring rint_vbatt_threshold*/
    value = NT_REG_RD(QWLAN_TPE_SW_RINT_VBATT_REG);
    temp_config = *((uint32_t *)(nt_devcfg_get_config(NT_DEVCFG_RINT_MEASUREMENT_STOP_THRESHOLD_CONFIG)));
    value &= ~(TPE_SW_RINT_VBATT_RINT_VBATT_THRESHOLD_MASK << QWLAN_TPE_SW_RINT_VBATT_RINT_VBATT_THRESHOLD_OFFSET);
    value |= (temp_config << QWLAN_TPE_SW_RINT_VBATT_RINT_VBATT_THRESHOLD_OFFSET);
    NT_REG_WR(QWLAN_TPE_SW_RINT_VBATT_REG, value);

    /* Configuring rint_vbatt_average_samples*/
    value = NT_REG_RD(QWLAN_TPE_SW_RINT_VBATT_REG);
    temp_config = *((uint32_t *)(nt_devcfg_get_config(NT_DEVCFG_RINT_AVERAGE_SAMPLES_CONFIG)));
    value &= ~(QWLAN_TPE_SW_RINT_VBATT_RINT_VBATT_AVERAGE_SAMPLES_MASK);
    value |= (temp_config << QWLAN_TPE_SW_RINT_VBATT_RINT_VBATT_AVERAGE_SAMPLES_OFFSET);
    NT_REG_WR(QWLAN_TPE_SW_RINT_VBATT_REG, value);

    /* Configuring rint_vbatt_min_samples*/
    value = NT_REG_RD(QWLAN_TPE_SW_RINT_VBATT_REG);
    value &= ~(QWLAN_TPE_SW_RINT_VBATT_RINT_VBATT_MIN_SAMPLES_MASK);
    value |= (VBATT_RINT_MIN_SAMPLES_CONFIG << QWLAN_TPE_SW_RINT_VBATT_RINT_VBATT_MIN_SAMPLES_OFFSET);
    NT_REG_WR(QWLAN_TPE_SW_RINT_VBATT_REG, value);

    /* Enabling the interrupts*/
    // rint_vatt_timeout_int_en , rint_vbatt_int_en , txop_vbatt_int_en
    value = NT_REG_RD(QWLAN_TPE_INTERRUPT_ENABLE_REG);
    value |=
        (QWLAN_TPE_INTERRUPT_ENABLE_RINT_VBATT_TIMEOUT_INT_EN_MASK | QWLAN_TPE_INTERRUPT_ENABLE_RINT_VBATT_INT_EN_MASK);
    NT_REG_WR(QWLAN_TPE_INTERRUPT_ENABLE_REG, value);

    /* Window configuration for rint measurement in micro seconds*/
    value = NT_REG_RD(QWLAN_TPE_SW_RINT_VBATT1_REG);
    temp_config = *((uint32_t *)(nt_devcfg_get_config(NT_DEVCFG_RINT_MEASUREMENT_WINDOW_TIME_CONFIG)));
    value &= ~(QWLAN_TPE_SW_RINT_VBATT1_RINT_VBATT_WINDOW_MASK);
    value |= (temp_config << QWLAN_TPE_SW_RINT_VBATT1_RINT_VBATT_WINDOW_OFFSET);
    NT_REG_WR(QWLAN_TPE_SW_RINT_VBATT1_REG, value);

    /* Vbatt start_threshold configuration*/
    value = NT_REG_RD(QWLAN_TPE_SW_RINT_VBATT1_REG);
    temp_config = *((uint32_t *)(nt_devcfg_get_config(NT_DEVCFG_RINT_MEASUREMENT_START_THRESHOLD_CONFIG)));
    value &= ~(QWLAN_TPE_SW_RINT_VBATT1_RINT_VBATT_START_THRESHOLD_MASK);
    value |= temp_config;
    NT_REG_WR(QWLAN_TPE_SW_RINT_VBATT1_REG, value);
    /*Enabling rint measurement Interrupts in MCU*/
    value = NT_REG_RD(QWLAN_MCU_IRQ_EN_REG);
    value |= QWLAN_MCU_IRQ_EN_TPE_IRQ_EN_MASK;
    NT_REG_WR(QWLAN_MCU_IRQ_EN_REG, value);

    /*enabling TXOP iRQ in NVIC*/
    nt_enable_device_irq(WLAN_ccu_irq);
#endif
}

/**
 * <!-- nt_tpe_rint_get -->
 *
 * @brief            : for debugging purpose we will take parameters vbatt_avg,
 * sample_index,sample_ptr,recovery_time,vbatt_endtime,vbatt_starttime.
 * @param statustype :
 * @return           : void
 */
uint32_t nt_tpe_rint_get(statustype_t statustype)
{
    uint32_t value;
    value = NT_REG_RD(QWLAN_TPE_SW_RINT_VBATT_REG);                    // read the sw_rint_vbatt_reg.
    value |= (1 << QWLAN_TPE_SW_RINT_VBATT_RINT_VBATT_ENABLE_OFFSET);  // Enable the rint_vbatt_en bit.
    NT_REG_WR(QWLAN_TPE_SW_RINT_VBATT_REG, value);

    if (statustype == VBATT_AVG) {
        value = NT_REG_RD(QWLAN_TPE_SW_VBATT_STATUS_REG &
                          QWLAN_TPE_SW_VBATT_STATUS_VBATT_AVERAGE_MASK);  // read the Current vbatt average.
    }

    else if (statustype == SAMPLE) {
        value |=
            (1 << QWLAN_TPE_SW_VBATT_STATUS_VBATT_SAMPLE_INDEX_OFFSET);  // array of index use to display the samples.
        NT_REG_WR(QWLAN_TPE_SW_VBATT_STATUS_REG, value);
        value = NT_REG_RD(
            QWLAN_TPE_SW_VBATT_STATUS_REG &
            QWLAN_TPE_SW_VBATT_STATUS_VBATT_SAMPLE_MASK);  // VBATT sample stored in array at index vbatt_sample_index.
    } else if (statustype == SAMPLE_PTR) {
        value = NT_REG_RD(QWLAN_TPE_SW_VBATT_STATUS_REG &
                          QWLAN_TPE_SW_VBATT_STATUS_VBATT_HEAD_PTR_MASK);  // Current head ptr for vbatt sample array.
    } else if (statustype == RECOVERY_TIME) {
        value = NT_REG_RD(QWLAN_TPE_SW_RINT_VBATT_STATUS_REG &
                          QWLAN_TPE_SW_RINT_VBATT_STATUS_RINT_RECOVERY_TIME_MASK);  // Read recovery time.
    } else if (statustype == VBATT_END_TIME) {
        value = NT_REG_RD(QWLAN_TPE_SW_RINT_VBATT_STATUS_REG &
                          QWLAN_TPE_SW_RINT_VBATT_STATUS_VBATT_END_MASK);  // read battery end voltage.
    } else if (statustype == VBATT_START_TIME) {
        value = NT_REG_RD(QWLAN_TPE_SW_RINT_VBATT_STATUS_REG &
                          QWLAN_TPE_SW_RINT_VBATT_STATUS_VBATT_START_MASK);  // read battery start voltage.
    }
    return value;
}

/**
 * <!-- nt_cc_add_client_callback -->
 *
 * @brief              : Add a new linked list node for user info (user defined function & recovery time)
 * @param driver_fcn   : function pointer for the callback if the measured battery level is less than the required
 * threshold
 * @param recovery_time: recovery time in micro seconds read
 * @return             : void
 */
void nt_cc_add_client_callback(txop_driver_fcn driver_fcn, uint32_t recovery_time)
{
    nt_cc_mgmt_t *new = (nt_cc_mgmt_t *)nt_osal_allocate_memory(sizeof(nt_cc_mgmt_t));
    if (new == NULL) {
        NT_LOG_SYSTEM_CRIT("Couldn't allocate memory", 0, 0, 0);
        return;
    }
    new->cc_mgmt_driver_fn = driver_fcn;
    new->recovery_time = recovery_time;
    new->next = head;
    head = new;
}

/**
 * <!-- nt_cc_call_user_callback_from_isr -->
 *
 * @brief               : To calculate vbatt level and call the user defined function to adjust txop after interrupt has
 * been triggered.
 * @param battery_level : Battery level calculated
 * @return              : void
 */
void nt_cc_call_user_callback_from_isr(uint32_t recovery_time)
{
    nt_cc_mgmt_t *new = head;
    uint32_t battery_level;
    if (rint_status_flag == 1 || rint_status_flag == 3) {
    }
    while (new->recovery_time != recovery_time) {
        new = new->next;
    }
    battery_level = nt_lut_map(recovery_time);
    if (rint_status_flag == 1 || rint_status_flag == 3) {
        new->cc_mgmt_driver_fn(0);
    }
    new->cc_mgmt_driver_fn(battery_level);
}

/**
 * <!-- nt_issue_batt_level_mon_request -->
 *
 * @brief            : Battery level monitoring before TX operation issued by user
 * @param rint_user_config_t *user_config :  Rint measurement window in micro seconds
                                            uint32_t    rint_window;
                                             TPE will ignore this rint measurement if the first sample is greater than
 this threshold uint32_t    rint_vbatt1_threshold; TPE will continue to measure vbatt samples until average is greater
 than this threshold uint32_t    vbatt_threshold; configuration RINT will only be measured if we have received more than
 this many samples (max 8) uint32_t    min_samples; configuration How to average the vbatt samples Decoded as: 0 :
 Average taken over last 1 sample
                                            * 1 : Average taken over last 2 samples
                                            * 2 : Average taken over last 4 samples
                                            * 3 : Average taken over last 8 samples
                                            Note, if rint_vbatt_min_samples is less than the average samples, rint will
 only be measured if we receive at least the number of average uint32_t    average_samples; TPE will stop vbatt
 meaurement this many usecs after last TX if vbatt does not reach rint_vbatt_threshold uint32_t vbatt_timeout_threshold;
 * @return           : void
 */
void nt_issue_batt_level_mon_request(nt_cc_user_config_t *user_config, txop_driver_fcn driver_fcn)
{
    NT_REG_WR(QWLAN_TPE_SW_RINT_VBATT1_REG,
              user_config->rint_window << QWLAN_TPE_SW_RINT_VBATT1_RINT_VBATT_WINDOW_OFFSET |
                  user_config->rint_vbatt1_threshold);
    NT_REG_WR(
        QWLAN_TPE_SW_RINT_VBATT_REG,
        (QWLAN_TPE_SW_RINT_VBATT_RINT_VBATT_WINDOW_MON_EN_MASK |
         (uint32_t)(user_config->vbatt_timeout_threshold) << QWLAN_TPE_SW_RINT_VBATT_RINT_VBATT_TIMEOUT_OFFSET |
         (uint32_t)(user_config->vbatt_threshold) << QWLAN_TPE_SW_RINT_VBATT_RINT_VBATT_THRESHOLD_OFFSET |
         user_config->average_samples << QWLAN_TPE_SW_RINT_VBATT_RINT_VBATT_AVERAGE_SAMPLES_OFFSET |
         (QWLAN_TPE_SW_RINT_VBATT_RINT_VBATT_MIN_SAMPLES << QWLAN_TPE_SW_RINT_VBATT_RINT_VBATT_MIN_SAMPLES_OFFSET) |
         QWLAN_TPE_SW_RINT_VBATT_RINT_VBATT_EN_MASK));
    // ret_batt_level = &(user_config->vbatt_level);
    nt_cc_add_client_callback(driver_fcn, user_config->vbatt_threshold);  // TODO :convert vbatt_threshold  to micro
                                                                          // seconds before passing to the linked list
}

/**
 * <!-- nt_cc_battery_level -->
 *
 * @brief            : ISR to read the recovery time for coin cell management
 * @return           : void
 */
void nt_cc_battery_level(void)
{
    uint32_t rint_recovery_time = 0;
    rint_recovery_time = NT_REG_RD(QWLAN_TPE_SW_RINT_VBATT_STATUS_REG);
    rint_recovery_time = (rint_recovery_time & QWLAN_TPE_SW_RINT_VBATT_STATUS_RINT_RECOVERY_TIME_MASK) >>
                         QWLAN_TPE_SW_RINT_VBATT_STATUS_RINT_RECOVERY_TIME_OFFSET;
    nt_cc_call_user_callback_from_isr(
        rint_recovery_time);  // TODO convert it into micro seconds before passing it to the linked list
}

void nt_update_battery_level(void)
{
    uint32_t reg_read;
    reg_read = NT_REG_RD(QWLAN_TPE_SW_RINT_VBATT_STATUS_REG);
    reg_read = reg_read >> QWLAN_TPE_SW_RINT_VBATT_STATUS_RINT_RECOVERY_TIME_OFFSET;
    /* Mapping the recovery time with respective battery level */
    cc_battery_level = nt_lut_map(reg_read);
    /* Re- Enabling the interrupts back*/
    NT_REG_WR(QWLAN_TPE_INTERRUPT_ENABLE_REG, QWLAN_TPE_INTERRUPT_ENABLE_RINT_VBATT_TIMEOUT_INT_EN_MASK |
                                                  QWLAN_TPE_INTERRUPT_ENABLE_RINT_VBATT_INT_EN_MASK);
}
/**
 * <!-- nt_cc_txop_adjust -->
 *
 * @brief      : To abandon the txop if the battery level drops below a certain threshold. And, re configure the
 * threshold value.
 * @return     : void
 */
void nt_cc_txop_adjust(void)
{
    // QWLAN_TPE_SW_TXOP_VBATT_TXOP_VBATT_THRESHOLD_MASK QWLAN_TPE_SW_TXOP_VBATT_REG
}

#endif  // NT_FN_CC_MGMT && NT_HOSTLESS_SDK
