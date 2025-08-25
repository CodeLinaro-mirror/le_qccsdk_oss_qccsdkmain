/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef CORE_SYSTEM_INC_NT_sysmon_H_
#define CORE_SYSTEM_INC_NT_sysmon_H_

#include <stdint.h>
/*apart from the wifi_app_task , sysmon_task is another app level task to provide the policy, to
 * handle system and safety of neutrino, to have all the policies that it will provide,
 *  this stack depth is chosen, based on approximation  */
#define NT_SYSMON_STACK_SIZE 400
#define NT_SYSMON_TASK_PRIO  7

/*
 * @Sensor selection
 * */
typedef enum { DHT11 = 0, IN_NEUT_T } temperature_sensor;

/*
 * @PMIC Sensor selection (Voltage sensor / Temperature sensor) as input to HKADC
 */

typedef enum { HKADC_VOLTAGE = 0, HKADC_TEMPERATURE = 1 } HKADC_sensor_sel_t;

/**
 * @Function: nt_sysmon_cc_hkadc_avg_voltage_get
 * @Description: API for getting average voltage number from hkadc, (raw register value)
 * @parm: none
 * @Return : battery voltage of neutrino in 32 bit integer format
 */
uint32_t nt_sysmon_cc_hkadc_avg_voltage_get(void);

/**
 * @Function: nt_sysmon_batt_voltage_get
 * @Description: API for getting battery voltage in Volts for neutrino
 * @parm: none
 * @Return : battery voltage of neutrino in 32 bit integer format
 */
uint32_t nt_sysmon_batt_voltage_get(void);

/**
 * @Function: nt_sysmon_temperature_data_get
 * @Description: API for getting internal temperature in celcius for neutrino
 * @parm: a variable of temperature_data(a enum) type
 * @Return : temperature inside neutrino in 32 bit integer format
 */
int32_t nt_sysmon_temperature_data_get(temperature_sensor temp_source);

/**
 * @Function: nt_sysmon_ms_delay
 * @Description: API for getting delay in milliseconds units
 * @parm: none
 * @Return :none
 */
void nt_sysmon_clk_cyc_delay(uint32_t clk_cyc);

/**
 * @Function: nt_sysmon_threshold_init
 * @Description: API for initiaizing the the threshold voltage warning and high temperature warning
 * @parm: none
 * @Return :none
 */
void nt_sysmon_threshold_init(void);

/**
 * @Function: nt_create_sysmon_task
 * @Description: API for creating a task for the system features
 * @parm: none
 * @Return :returns status of the task creation
 */
int32_t nt_sysmon_task_create(void);

/**
<<<<<<< HEAD
 * @Function: nt_sysmon_hkadc_input_snr_sel
 * @Description: API for selecting inut sensor for HKADC module and setting sampling rate
 * @parm: a variable of HKADC_sensor_sel_t(a enum) type
 * @Return :none
 */
void nt_sysmon_hkadc_input_snr_sel(HKADC_sensor_sel_t hkadc_input_snr);

/**
 * @Function: nt_sysmon_updated_batt_voltage_reg_val_get_from_isr
 * @Description: API for fetching average voltage number , (raw register value) to the isr
 * @parm: none
 * @Return : register value of battery voltage of neutrino  in 32 bit integer format
 */
uint32_t nt_sysmon_updated_batt_voltage_reg_val_get_from_isr(void);

/**
 * @Function: nt_system_sw_reset
 * @Description: API for providing system software reset
 * 				 it will reset the entire system
 * @parm: none
 * @Return :none
 */
void nt_system_sw_reset(void);

#endif /* CORE_SYSTEM_INC_NT_sysmon_H_ */
