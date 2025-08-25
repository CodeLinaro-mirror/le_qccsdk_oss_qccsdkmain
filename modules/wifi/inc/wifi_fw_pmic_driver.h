/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/**********************************************************************************************
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 * @file wifi_fw_pmic_driver.h
 * @brief WiFi FW PMIC related declarations
 *
 *
 *********************************************************************************************/

#ifndef _WIFI_FW_PMIC_DRIVER_H_

#define _WIFI_FW_PMIC_DRIVER_H_

#ifdef PLATFORM_FERMION

#include "fermion_hw_reg.h"

#include "nt_common.h"
#include "nt_socpm_sleep.h"

/********************************************************************************************

* Function Declaration

********************************************************************************************/

void wifi_fw_pmic_init(cpr_mode_e cpr_mode);

void wifi_fw_pmic_pre_sleep_config(sleep_mode mode);

void wifi_fw_select_xo_sleep_clock(void);

void wifi_fw_select_rc_sleep_clock(void);

void wifi_fw_program_pmic_and_aon_otp_trim(void);

#endif /* PLATFORM_FERMION */

#endif /* _WIFI_FW_PMIC_DRIVER_H_ */
