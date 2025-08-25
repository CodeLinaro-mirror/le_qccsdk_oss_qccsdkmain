/*
Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
*/
/*
 * nt_imps.h
 *
 *  Created on: Feb 7, 2020
 *      Author: surendar
 */
#ifndef CORE_WIFI_SME_INC_NT_IMPS_H_
#define CORE_WIFI_SME_INC_NT_IMPS_H_

#include "nt_osal.h"
#include "pm_api.h"  // PM_TIMER structure is used
//#include "autoconf.h"

#define DEFAULT_IMPS_SLEEP_TIME 120000

/*
 * 1 - Norrmal sleep mode(WIFI and MCU will be off);
 * 2 - standby mode(WIFI,MCU and RAM will be shut off);
 */
//#define DEFAULT_IMPS_SLEEP_MODE 1

typedef enum { TURN_OFF = 0, TURN_ON = 1 } IMPS_ENABLE_DISABLE;

typedef struct {
    uint32_t imps_sleep_time;     /*IMPS time period value*/
    uint8_t imps_enabled;         /* IMPS Enabled or disabled */
    NT_BOOL host_mode_configured; /* Host mode enabled or disabled for IMPS */
#ifdef SUPPORT_IMPS_IMPROVEMENTS
#ifdef ENABLE_IMPS_TIMER_ON_BOOTUP
    uint32_t cnx_wait_time_ms;                  /* Wait time for wifi connection */
#endif                                          /* ENABLE_IMPS_TIMER_ON_BOOTUP */
    uint32_t recnx_wait_time_ms;                /* Wait time for wifi reconnection after disconnection*/
    uint32_t cmd_proc_wait_time_ms;             /* Max time taken by the WMI cmd to process*/
    uint64_t cmd_recv_timestamp;                /* to store last received WMI cmd timestamp */
    nt_osal_timer_handle_t imps_cnx_wait_timer; /* Timer to wait for connection/reconnection */
    bool imps_cnx_timer_started;
#endif                    /* SUPPORT_IMPS_IMPROVEMENTS */
    bool imps_registered; /* Register for IMPS at imps_cnx_wait_timer timeout cb and handled it in idle task */
    uint8_t policy;       /*  use mcu sleep or deep sleep */
} IMPS_STRUCT_CTX_t;

/*timer*/
void nt_wpm_init_imps(devh_t *);
void nt_wpm_imps_enable_disable(devh_t *dev, uint8_t state);

/*
 * @brief: for registering imps standby
 *@parameters: none
 *@returns: none
 */
void nt_wpm_register_imps_standby();

/*
 * @brief  checks for IMPS enable and host to register IMPS in host mode
 * @param  none
 * @return : TRUE-> if IMPS is enabled and STA configured through host else FALSE
 *
 */
NT_BOOL nt_imps_is_enabled_in_hosted_mode(void);

/*
 * @brief  enable the host mode param for IMPS
 * @param  none
 * @return : NONE
 *
 */
void nt_configure_host_mode_in_imps(void);
void nt_set_reset_delayed_imps(NT_BOOL is_set);

NT_BOOL nt_is_imps_registered(void);

void nt_send_imps_enter_cmd(NT_BOOL is_isr_context);
void nt_send_pm_mode_cmd(NT_BOOL enable);

#if (defined CONFIG_NT_RCLI)
void *nt_wpm_imps_stats(void);
#endif

#ifdef SUPPORT_IMPS_IMPROVEMENTS
void nt_imps_cnx_timeout_cb(void);
void _pm_post_pmImps_timeout_msg(TimerHandle_t thandle);
void start_imps_cnx_wait_timer(uint32_t timeout_value);
void stop_imps_cnx_wait_timer();
#endif /* SUPPORT_IMPS_IMPROVEMENTS */

#endif /* CORE_WIFI_SME_INC_NT_IMPS_H_ */
