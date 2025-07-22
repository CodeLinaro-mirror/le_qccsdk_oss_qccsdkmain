/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef CORE_SYSTEM_INC_NT_PMIC_DRIVER_H_
#define CORE_SYSTEM_INC_NT_PMIC_DRIVER_H_

#include <stdint.h>

#define NT_PMIC_SMPS2_RRAM_ADDR (0x80218)  // RRAM OTP address to store SMPS2 configurations
/* This address in RRAM OTP will contain the configuration data needed to be done for SMPS2,
 * which should be written after cold-boot.
 * */
/*-----------------------------------------------TYPEDEFS-----------------------------------------------------------*/

typedef enum nt_pmic_cal_type_e { NT_PMIC_CAL_SMPS1 = 0, NT_PMIC_CAL_SMPS2, NT_PMIC_CAL_AONLDO } nt_pmic_cal_type_t;

typedef enum nt_pmic_options_e {
    NT_PMIC_OPT_CAL = 0,
    NT_PMIC_OPT_AONSRC,
    NT_PMIC_OPT_AONMODE,
    NT_PMIC_OPT_STATE
} nt_pmic_options_t;

typedef enum nt_pmic_voltage_control_e { NT_PMIC_VCTL_PWM = 0, NT_PMIC_VCTL_PFM } nt_pmic_voltage_control_t;

/*----------------------------------------------------------------------------------------------------------------*/

/**
 * <!-- nt_pmic_init -->
 *
 * @brief Initializing PMIC configurations
 * @return void
 */
void nt_pmic_init(void);

/**
 * <!-- nt_pmic_set -->
 *
 * @brief Set PMIC registers(cal value(smps1,smps2,aonldo), switching between vbatt and smps and switching between PFM
 * and PWM mode) from CLI
 * @return void
 */
void nt_pmic_set(uint8_t option, uint8_t type, uint32_t value);

/**
 * <!-- nt_pmic_get -->
 *
 * @brief To read and print the calibration values of smps1,smps2 and aonldo based on the request from CLI
 * @return void
 */
void nt_pmic_get(uint8_t option, uint8_t type);

/**
 * <!-- nt_pmic_set_smps_cal -->
 *
 * @brief Set   : smps calibration values from the CLI input
 * @param type : 0 - smps1
 *               1 - smps2
 * @param value: calibration value
 * @return void
 */
void nt_pmic_set_smps_cal(uint8_t type, uint8_t value);

/**
 * <!-- nt_pmic_set_aonldo_cal -->
 *
 * @brief Set   : aonldo calibration values from the CLI input
 * @param value : calibration value
 * @return void
 */
void nt_pmic_set_aonldo_cal(uint8_t value);

/**
 * <!-- nt_pmic_aonsrc_ctrl -->
 *
 * @brief Set   : To switch the aonsrc control between vbatt and smps1 based on the value from CLI input
 * @param type  : 0 = Use vbatt
 *                1 = Use smps1
 * @return void
 */
void nt_pmic_aonsrc_ctrl(uint8_t type);

/**
 * <!-- nt_pmic_pfm_pwm_ctrl -->
 *
 * @brief Set   : To switch the voltage controlling mode(either PWM or PFM) based on the value from CLI input
 * @param type  : 0 = Use PWM
 *                1 = Use PFM
 * @return void
 */
void nt_pmic_pfm_pwm_ctrl(uint8_t type, uint32_t value);

/**
 * <!-- nt_pmic_pre_sleep_config -->
 *
 * @brief Set   : Configurations for PMIC before going to sleep
 * @return void
 */
void nt_pmic_post_sleep_config(void);

/**
 * <!-- nt_pmic_smps2_rram_cfg -->
 *
 * @brief: Write the initial smps2 voltage into RRAM OTP region from CLI
 * @param data:  32 bit data from the CLI which should be written in RRAM
 * @return void
 */
void nt_pmic_smps2_rram_cfg(uint32_t data);

#endif /* CORE_SYSTEM_INC_NT_PMIC_DRIVER_H_ */
