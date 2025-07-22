/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*========================================================================
 *
 * @file wifi_fw_internal_api.h
 * @brief Declarations related to Fermion specific features
 *========================================================================*/

#ifndef WIFI_FW_INTERNAL_API_H
#define WIFI_FW_INTERNAL_API_H
#include "fwconfig_cmn.h"
#include "nt_flags.h"

#ifdef IMAGE_FERMION
#include "nt_logger_api.h"
#ifdef SUPPORT_RING_IF
#include "wifi_fw_table_api.h"
#endif
/*------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * ----------------------------------------------------------------------*/
#define FERM_INIT_LOG_ERR(...) NT_LOG_PRINT(COMMON, ERR, __VA_ARGS__)
#define FERM_INIT_LOG_INFO(...)

/*-------------------------------------------------------------------------
 * Function Declarations and Documentation
 * ----------------------------------------------------------------------*/

/* Function to initialize Fermion specific modules */
void wifi_fw_module_init(void);

/* Function to set specific GPIOs for Fermion */
void wifi_fw_gpio_init(bool);

/* Function to send update interrupt from Fermion to Apps */
#ifndef FIRMWARE_APPS_INFORMED_WAKE
void wifi_fw_f2a_interrupt(void);
#endif /* FIRMWARE_APPS_INFORMED_WAKE */

#ifdef SUPPORT_RING_IF
void wifi_fw_ext_pulse_multiuse_gpio(MULTIUSE_GPIO_ASSERT_REASON_ENUM multiuse_reason);
#endif
/* Function to change the F2A pulse width */
void wifi_fw_set_ext_f2a_pulse_width(uint32_t input);
/* Function to change the TSF pulse width */
void wifi_fw_set_ext_multiuse_pulse_width(uint32_t input);

/* Function to check if table is initialized */
bool wifi_fw_is_table_initialized(void);

/* APIs for fermion hosted mode check */
bool wifi_fw_in_hosted_mode(void);
void wifi_fw_set_hosted_mode(bool);
#endif /* IMAGE_FERMION */
#endif /* WIFI_FW_INTERNAL_API_H */
