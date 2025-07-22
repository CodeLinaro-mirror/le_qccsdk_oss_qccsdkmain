/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*========================================================================
 *
 * @file wifi_fw_ext_int_api.h
 * @brief Declarations related to external interrupts like f2a, a2f
 *========================================================================*/

#ifndef WIFI_FW_EXT_INTR_H
#define WIFI_FW_EXT_INTR_H

/*-------------------------------------------------------------------------
 * Include Files
 * ----------------------------------------------------------------------*/

#include "fwconfig_cmn.h"
#include "nt_flags.h"

#include <stdint.h>
#include <stdbool.h>

/*------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * ----------------------------------------------------------------------*/
#ifdef PLATFORM_FERMION

#define FIRMWARE_2_HOST_ASSERT    !FIRMWARE_2_HOST_GPIO_DEF_POL
#define FIRMWARE_2_HOST_DE_ASSERT FIRMWARE_2_HOST_GPIO_DEF_POL

/*
 *  FIRMWARE_2_HOST GPIO default config
 */

#define FIRMWARE_2_HOST_GPIO_DEF_POL TRUE        //>> Default polarity of the FIRMWARE_2_HOST GPIO
#define FIRMWARE_2_HOST_GPIO_PORT    NT_GPIOA    //>> Default GPIO PORT being used
#define FIRMWARE_2_HOST_GPIO         GPIO_PIN_8  //>> Default GPIO PIN being used
#define MAX_F2A_PULSE_WIDTH_US       25          //>> Max F2A pulse Width
#define MAX_INTER_F2A_INTERVAL_US    10          //>> Max interval between F2A pulses

#define A2F_ASSERT_INTR_NVIC1_MASK   (0x1 << 31)
#define A2F_DEASSERT_INTR_NVIC3_MASK (0x1 << 12)

#ifdef FIRMWARE_APPS_INFORMED_WAKE
typedef enum f2a_short_reason {
    F2A_SHORT_REASON_RING_TX_RX,    //>> F2A short due to Ring Tx, Rx request
    F2A_SHORT_REASON_A2F_RESP,      //>> F2A Short in resp of A2F assert
    F2A_SHORT_REASON_TWT_SP_START,  //>> F2A Short because of TWT SP START
} f2a_short_reason_t;

/*-------------------------------------------------------------------------
 * Function Declarations and Documentation
 * ----------------------------------------------------------------------*/

/* Function to send pulse signal from Fw to Apps */
void wifi_fw_ext_f2a_pulse(f2a_short_reason_t reason);

/* Function to assert Fw to Apps signal */
void wifi_fw_ext_f2a_signal_assert(f2a_short_reason_t reason);

/* Function to de-assert Fw to Apps signal */
void wifi_fw_ext_f2a_signal_deassert(void);

/* Timer handler for F2A timer */
void wifi_fw_ext_f2a_timeout_cb(void);

/* Disables A2F interrupt service and changes A2F to level trigger */
void aon_ext_wakeup_set_lvl_trigger(void);

/* Function to initialize configuration for A2F interrupt */
void init_aon_ext_wakeup_int(void);

/* Function to send F2A on cold boot */
void wifi_fw_ext_cold_boot_f2a_signal(void);

void configure_twt_wake_send_f2a(uint8_t enable_f2a);

bool get_twt_wake_send_f2a_configuration(void);

/* Function to enable/disable F2A assert for testing with FermionApp */
bool f2a_enable_disable_assert(uint8_t enable_assert);

#else

/* Function to disable A2F interrupt */
void disable_aon_ext_wakeup_int(void);

#endif /* FIRMWARE_APPS_INFORMED_WAKE */
#endif /* PLATFORM_FERMION */
#endif /* WIFI_FW_EXT_INTR_H */
