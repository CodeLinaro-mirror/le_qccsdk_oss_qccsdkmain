/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "nt_sys_monitoring.h"

#include "nt_common.h"
#include "nt_hw.h"
#include "nt_mem.h"
#include "uart.h"
#include "ExceptionHandlers.h"
#include "nt_devcfg.h"

static TaskHandle_t nt_sysmon_hnd = NULL;

static volatile uint32_t cc_batt_voltage = 0;

#if CONFIG_PBL_PREES_RESET_FOR_DTIM
uint32_t reset_dbg = 0x9;
#endif

/**
 * @Function: nt_sysmon_hkadc_input_snr_sel
 * @Description: API for selecting inut sensor for HKADC module and setting sampling rate
 * @parm: a variable of HKADC_sensor_sel_t(a enum) type
 * @Return :none
 */
void nt_sysmon_hkadc_input_snr_sel(HKADC_sensor_sel_t hkadc_input_snr)
{
    uint32_t rd_value = 0;

    if (hkadc_input_snr == HKADC_VOLTAGE) {
        rd_value = NT_REG_RD(QWLAN_PMU_CFG_HKADC_DATA_AVG_CNT_REG);
        rd_value |= QWLAN_PMU_CFG_HKADC_DATA_AVG_CNT_CFG_TEMP_VBATT_MON_SEL_MASK;
        rd_value |= QWLAN_PMU_CFG_HKADC_DATA_AVG_CNT_CFG_HKADC_DATA_AVG_CNT_DEFAULT;
        NT_REG_WR(QWLAN_PMU_CFG_HKADC_DATA_AVG_CNT_REG, rd_value);
    }

    else if (hkadc_input_snr == HKADC_TEMPERATURE) {
        rd_value = 0;
        rd_value = NT_REG_RD(QWLAN_PMU_CFG_HKADC_DATA_AVG_CNT_REG);
        rd_value |= QWLAN_PMU_CFG_HKADC_DATA_AVG_CNT_CFG_HKADC_DATA_AVG_CNT_DEFAULT;
        NT_REG_WR(QWLAN_PMU_CFG_HKADC_DATA_AVG_CNT_REG, rd_value);
    }

    else {
    }
}

/**
 * @Function: nt_sysmon_temperature_data_get
 * @Description: API for getting internal temperature for neutrino
 * @parm: a variable of temperature_data(a enum) type
 * @Return : temperature inside neutrino in 32 bit integer format
 */
int32_t nt_sysmon_temperature_data_get(temperature_sensor temp_source)
{
    int32_t temp_celc = 0;
#ifdef NT_DEBUG
    // char str[50];
#endif
#if 0
	if(temp_source==DHT11){
		//enabling the root clock and set the prset address
		nt_gpio_init();

		//to check thre in which moe a pin is now
		uint32_t data1= nt_gpio_pin_read_mode(gpio_register_t* GPIOx);

		//to change mode for a particular gpio pin
		nt_gpio_pin_mode (gpio_register_t* GPIOx,uint32_t Pin, uint32_t Mode);

		//to read vlue if its set to input mode
		uint32_t rd-high-low=nt_gpio_pin_read_level(gpio_register_t* GPIOx);

		//to change mode for a particular gpio pin
		nt_gpio_pin_mode (gpio_register_t* GPIOx,uint32_t Pin, uint32_t Mode);

		//to write to a particular gpio pin
		nt_gpio_pin_write(gpio_register_t* GPIOx,uint32_t Pin,GPIO_PinState val);
	}
#endif

    if (temp_source == IN_NEUT_T) {
        uint32_t hx_temp = 0;
        uint32_t rd_value = 0;

        // selecting the temperature sensor from PMIC as input to HKADC
        nt_sysmon_hkadc_input_snr_sel(HKADC_TEMPERATURE);

        rd_value = NT_REG_RD(QWLAN_PMU_CFG_ACAL_VBAT_MON_EN_REG);
        rd_value |= QWLAN_PMU_CFG_ACAL_VBAT_MON_EN_TEMP_MON_EN_MASK;
        NT_REG_WR(QWLAN_PMU_CFG_ACAL_VBAT_MON_EN_REG, rd_value);

        // delay for averaging the samples, 900uSec
        // providing 54216 clock cycles of delay for averaging the samples
        nt_sysmon_clk_cyc_delay(54216);

        hx_temp = NT_REG_RD(QWLAN_PMU_TEMP_SNR_RD_DATA_REG);
        hx_temp = hx_temp & 0x000001FF;
        // ;

#ifdef NT_DEBUG
        // snprintf((char *)str,sizeof(str),"Neut-T %u \r\n",(int16_t)hx_temp);
        // nt_dbg_print(str);
#endif

        // in neutrino1 FPU is not enabled.
        // the actual equation is temperature_in_celcious= (((temp_decimal-50)*(165/410))-40)
        //(165/410)=0.4024
        // to get that 0.4024*10000=4024
        // to balance the equation  40*10000 and the result is divided by 10000

        temp_celc = (((hx_temp - 50) * 4024) - (40 * 10000));
        temp_celc = temp_celc / 10000;

#ifdef NT_DEBUG
//		snprintf ((char *)str,sizeof(str),"temp aftr conv %ld\r\n",(int32_t)temp_celc);
//		nt_dbg_print (str);
#endif
    }

    nt_sysmon_hkadc_input_snr_sel(HKADC_VOLTAGE);
    return temp_celc;
}

/**
 * @Function: nt_sysmon_cc_hkadc_avg_voltage_get
 * @Description: API for getting average voltage number from hkadc, (raw register value)
 * @parm: none
 * @Return : register value of battery voltage of neutrino  in 32 bit integer format
 */
uint32_t nt_sysmon_cc_hkadc_avg_voltage_get(void)
{
    uint32_t hx_voltage = 0;
    uint32_t rd_value = 0;
#ifdef NT_DEBUG
    // char str[50];
#endif
    // selecting the voltage sensor from PMIC as input to HKADC
    nt_sysmon_hkadc_input_snr_sel(HKADC_VOLTAGE);

    rd_value = NT_REG_RD(QWLAN_PMU_CFG_ACAL_VBAT_MON_EN_REG);
    rd_value |= QWLAN_PMU_CFG_ACAL_VBAT_MON_EN_VBAT_MON_EN_MASK;
    NT_REG_WR(QWLAN_PMU_CFG_ACAL_VBAT_MON_EN_REG, rd_value);

    // delay ~25 clock cycle requires
    // providing 900 uSec of delay for averaging the samples
    // equivalent to 54216 clock cycles
    nt_sysmon_clk_cyc_delay(54216);

    hx_voltage = NT_REG_RD(QWLAN_PMU_VBAT_MON_RD_DATA_REG);
    hx_voltage = hx_voltage & 0x000001FF;
    cc_batt_voltage = hx_voltage;

#ifdef NT_DEBUG
    // snprintf((char *)str,sizeof(str),"Neut-V %u \r\n",(int16_t)hx_voltage);
    // nt_dbg_print(str);
#endif

    return hx_voltage;
}

/**
 * @Function: nt_get_battery_voltage
 * @Description: API for getting battery voltage for neutrino in (volts*100) units
 * @parm: none
 * @Return : battery voltage of neutrino in 32 bit integer format
 */
uint32_t nt_sysmon_batt_voltage_get(void)
{
    double Voltage = 0;
    uint32_t hx_voltage = nt_sysmon_cc_hkadc_avg_voltage_get();
    Voltage = (hx_voltage - 16) * (1.8 / 410) + 1.6;

#ifdef NT_DEBUG
    // uint32_t itemp_volt = Voltage;
    // double temp_frac = Voltage - itemp_volt;
    // uint32_t itemp_volt0 = (uint32_t)(temp_frac * 100);
    // snprintf((char *)str,sizeof(str),"%s" "%u" "%s" "%u" "%s","Neutrino
    // Voltage",(int16_t)itemp_volt,".",(int16_t)itemp_volt0,"\r\n"); nt_dbg_print(str);
#endif
    return Voltage * 100;
}

/**
 * @Function: sysmon_clk_cyc_delay
 * @Description: API for getting delay in terms of clock cycles
 * @parm: no of clock cycle , that will be delayed
 * @Return :none
 */
void nt_sysmon_clk_cyc_delay(uint32_t clk_cyc)
{
    for (uint32_t clk_cyc_cnt = 0; clk_cyc_cnt < clk_cyc; clk_cyc_cnt++) {
        __asm volatile(" nop \n");
    }
}

/**
 * @Function: sysmon_threshold_init
 * @Description: API for initiaizing the the threshold voltage warning and high temperature warning
 * @parm: none
 * @Return :none
 */
void nt_sysmon_threshold_init(void)
{
    uint32_t threshold_int_status = 0;  //,panic_tempr_limit = 0 ;
#ifdef NT_CC_DEBUG_FLAG
    uint32_t ret_val = 0;
#endif
    uint16_t threshold_voltage = 0;
#ifdef NT_DEBUG
    char str[50];
#endif
    // parsing the threshold voltage value from the devcfg
    threshold_voltage = *((uint16_t *)(nt_devcfg_get_config(NT_DEVGFG_THRESHOLD_WARNING_VOLTAGE_CONFIG)));
    // parsing the panic high temperature limit from devcfg
    // panic_tempr_limit = *((uint32_t*)(nt_devcfg_get_config(NT_DEVCFG_WARNING_HIGH_TEMPERATURE_CONFIG)));
    // enabling the interrupt event in NVIC
    nt_enable_device_irq(PMU_ccpu_vbat_low_hit_int_p);
    // nt_enable_device_irq(PMU_ccpu_temp_panic_hit_int);

    threshold_int_status = NT_REG_RD(QWLAN_PMU_CFG_VABT_TEMP_MON_INT_EN_REG);
    // enabling  Vbat low interrupt
    threshold_int_status |= QWLAN_PMU_CFG_VABT_TEMP_MON_INT_EN_CFG_VBAT_LOW_HIT_INT_EN_MASK;
    // enabling Panic high temperature interrupt
    threshold_int_status |= QWLAN_PMU_CFG_VABT_TEMP_MON_INT_EN_CFG_TEMP_PANIC_HIGH_HIT_INT_EN_MASK;
    NT_REG_WR(QWLAN_PMU_CFG_VABT_TEMP_MON_INT_EN_REG, threshold_int_status);
    // setting warning threshold voltage
    // step1: clearing the register
    NT_REG_WR(QWLAN_PMU_CFG_VABT_MON_TH_REG, 0x00);
    // step2: setting the desired br-out voltage
    NT_REG_WR(QWLAN_PMU_CFG_VABT_MON_TH_REG, threshold_voltage);

#ifdef NT_DEBUG
    // check the settled value of the voltage which is being set after boot-up
    threshold_int_status = NT_REG_RD(QWLAN_PMU_CFG_VABT_MON_TH_REG);
    snprintf((char *)str, sizeof(str), "threshold voltage %u \r\n", (int16_t)threshold_int_status);
    nt_dbg_print(str);
#endif
    // setting high temperature warning
    // step1: clearing the register
    // NT_REG_WR (QWLAN_PMU_CFG_TEMP_MON_TH_REG , 0x0);
    // step2: setting desired panic temperature limit
    // NT_REG_WR (QWLAN_PMU_CFG_TEMP_MON_TH_REG , panic_tempr_limit);

#ifdef NT_DEBUG
    // check the settled value of the highest temperature limit which is being set after boot-up
    threshold_int_status = NT_REG_RD(QWLAN_PMU_CFG_TEMP_MON_TH_REG);
    snprintf((char *)str, sizeof(str), "panic high temperature limit %lu \r\n", (uint32_t)threshold_int_status);
    // nt_dbg_print(str);
#endif
    // creating system task
#ifdef NT_CC_DEBUG_FLAG
    ret_val = (uint32_t)nt_sysmon_task_create();  // creation of a system task is done  here
    if (ret_val != NT_OK) {
        nt_dbg_print("system task init failed");
    }
#endif
}

/**
 * @Function: nt_sysmon_updated_batt_voltage_reg_val_get_from_isr
 * @Description: API for fetching average voltage number , (raw register value) to the isr
 * @parm: none
 * @Return : register value of battery voltage of neutrino  in 32 bit integer format
 */
uint32_t nt_sysmon_updated_batt_voltage_reg_val_get_from_isr(void)
{
    uint32_t vbatt = cc_batt_voltage;

    return vbatt;
}

static void _sysmon_battery_int_res_meas_enable(void)
{
    uint32_t value = 0;
    /*Enable rint measurement*/
    value = NT_REG_RD(QWLAN_TPE_SW_RINT_VBATT_REG);
    value |= QWLAN_TPE_SW_RINT_VBATT_RINT_VBATT_EN_MASK;
    NT_REG_WR(QWLAN_TPE_SW_RINT_VBATT_REG, value);
}

static void _sysmon_battery_txop_vbatt_mon_enable(void)
{
    uint32_t value = 0;
    value = NT_REG_RD(QWLAN_TPE_SW_TXOP_VBATT_REG);
    value |= QWLAN_TPE_SW_TXOP_VBATT_TXOP_VBATT_EN_MASK;
    NT_REG_WR(QWLAN_TPE_SW_TXOP_VBATT_REG, value);
}

static void _sysmon_battery_rint_meas_upper_threhold_update(void)
{
    uint32_t last_config = 0, rint_vbatt_renewed_up_threshold = 0;
    last_config = NT_REG_RD(QWLAN_TPE_SW_RINT_VBATT1_REG);
    rint_vbatt_renewed_up_threshold = nt_sysmon_cc_hkadc_avg_voltage_get();
    last_config &= ~(QWLAN_TPE_SW_RINT_VBATT1_RINT_VBATT_START_THRESHOLD_MASK);
    last_config |= rint_vbatt_renewed_up_threshold;
    NT_REG_WR(QWLAN_TPE_SW_RINT_VBATT1_REG, last_config);
}

static void nt_sysmon_main_task()
{
    for (;;) {
        _sysmon_battery_rint_meas_upper_threhold_update();
        _sysmon_battery_int_res_meas_enable();
        _sysmon_battery_txop_vbatt_mon_enable();

#ifdef NT_DEBUG
        // nt_dbg_print("sysmon task running");
#endif

        qurt_thread_sleep(*((uint32_t *)(nt_devcfg_get_config(NT_DEVCFG_SYSMON_TASK_PERIODICITY_CONFIG))));
    }
}

/**
 * @Function: nt_create_sys_manager_task
 * @Description: API for creating a task for the system features
 * @parm: none
 * @Return :returns status of the task creation
 */
int32_t nt_sysmon_task_create(void)
{
    BaseType_t ret_val;

    ret_val = nt_qurt_thread_create(nt_sysmon_main_task, "sysmon_task", NT_SYSMON_STACK_SIZE, NULL, NT_SYSMON_TASK_PRIO,
                                    &nt_sysmon_hnd);
    return (ret_val);
}

/**
 * @Function: nt_system_sw_reset
 * @Description: API for providing system software reset
 * 				 it will reset the entire system
 * @parm: none
 * @Return :none
 */
void nt_system_sw_reset(void)
{
    // System Reset is not working RFA XO, independent of other clock if SW reset is
    // performed then clock will be switched to RC then reset will performed
    NT_REG_WR(QWLAN_PMU_AON_TOP_CFG_REG,
              QWLAN_PMU_AON_TOP_CFG_DEFAULT);  // Configuring with a value of 0 to choose RC from PMIC

    __asm volatile("nop \n");
    __asm volatile("nop \n");
    __asm volatile("nop \n");

    nt_dbg_print("Software Reset\r\n");

#if CONFIG_PBL_PREES_RESET_FOR_DTIM
    if (reset_dbg) {
        NT_REG_WR(QWLAN_PMU_BOOT_STRAP_CONFIG_SECURE_REG, 0x63887466);

        uint32_t value = 0;
        value |=
            (reset_dbg & 0x1) ? (3 << QWLAN_PMU_CFG_IO_BASED_STRAP_OVERRIDE_VALUE_CFG_AON_TESTBUS_ENABLE_OFFSET) : 0;
        value |= (reset_dbg & 0x2) ? (1 << QWLAN_PMU_CFG_IO_BASED_STRAP_OVERRIDE_VALUE_CFG_TEST_STRAP_OFFSET) : 0;
        value |=
            (reset_dbg & 0x4) ? (1 << QWLAN_PMU_CFG_IO_BASED_STRAP_OVERRIDE_VALUE_CFG_RRAM_BYPASS_ENABLE_OFFSET) : 0;
        value |= (reset_dbg & 0x8) ? (1 << QWLAN_PMU_CFG_IO_BASED_STRAP_OVERRIDE_VALUE_CFG_FORCE_NPS_PD_ON_OFFSET) : 0;
        value |= (reset_dbg & 0x10) ? (1 << QWLAN_PMU_CFG_IO_BASED_STRAP_OVERRIDE_VALUE_CFG_JTAG_MODE_OFFSET) : 0;
        NT_REG_WR(QWLAN_PMU_CFG_IO_BASED_STRAP_OVERRIDE_VALUE_REG, value);
    }
#endif

    NT_REG_WR(QWLAN_PMU_SYS_SOFT_RESET_REQ_REG, QWLAN_PMU_SYS_SOFT_RESET_REQ_SYS_SOFT_RESET_REQ_MASK);

    __asm volatile("nop \n");
    __asm volatile("nop \n");
    __asm volatile("nop \n");
}
