/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*========================================================================

* @file timer_test.h
* @brief Timer Infrastructure Unit Test header
* ======================================================================*/
#ifndef TIMER_TEST_H
#define TIMER_TEST_H
/*------------------------------------------------------------------------
 * Include Files
 * ----------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>

#include "fwconfig_wlan.h"
#include "nt_flags.h"
#include "timer.h"

#if (defined(SUPPORT_HIGH_RES_TIMER) && defined(HRES_TIMER_UNIT_TEST))
/*-------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * ----------------------------------------------------------------------*/
#define TMR_TEST_CMD_QUEUE_LEN 10 /* unit test command queue */

/*-------------------------------------------------------------------------
 * Type Declarations
 * ----------------------------------------------------------------------*/
typedef enum {
    TMR_MSG_ID_TIMEOUT,
    TMR_MSG_ID_UNTIMEOUT,
} tmr_msg_id_t;

typedef struct {
    bool reload;
    uint8_t msg_id; /* tmr_msg_id_t */
    time_unit_type unit;
    timer_ptr_type p_handle;
    time_timetick_type time;
} tmr_test_msg_t;

/*-------------------------------------------------------------------------
 * Function Declarations and Documentation
 * ----------------------------------------------------------------------*/

/**
 * hres_timer_unit_test_timeout
 *
 * @brief Test timer def and timeout expiry
 *
 * @param num_args Number of arguments present in args
 * @param *args Arguments
 *          arg[1] timer handle
 *          arg[2] time
 *          arg[3] unit (0 = US/ 1 = MS)
 *          arg[4] reload 0/1
 *
 * @return Returns TRUE on success, else FALSE
 */
bool hres_timer_unit_test_timeout(uint16_t num_args, uint32_t *args);

/**
 * hres_timer_unit_test_untimeout
 *
 * @brief Test timer undef and stops the timer
 *
 * @param num_args Number of arguments present in args
 * @param *args Arguments
 *          arg[1] timer handle
 *
 * @return Returns TRUE on success, else FALSE
 */
bool hres_timer_unit_test_untimeout(uint16_t num_args, uint32_t *args);

/**
 * hres_timer_test_create_task
 *
 * @brief Creates timer test task
 *
 * @param none
 *
 * @return Returns pdPASS on success, else pdFAIL
 */
uint8_t hres_timer_test_create_task(void);

/**
 * hres_timer_unit_test_log_dump
 *
 * @brief Prints the timer test logs
 *
 * @param none
 *
 * @return none
 */
void hres_timer_unit_test_log_dump(void);

/**
 * timer_test_qtimer_instance
 *
 * @brief Test qtimer physical frame instances
 *
 * @param num_args Number of arguments present in args
 * @param *args Arguments
 *          arg[1] Frame number: 1-4
 *          arg[2] Time in microseconds
 *
 * @return Returns TRUE on success, else FALSE
 */
bool timer_test_qtimer_instance(uint16_t num_args, uint32_t *args);

/**
 * timer_test_print_timers
 *
 * @brief print all fermion timers (qtimer, mtu timer, wb timer, aon timer)
 *        sync data
 *
 * @param none
 *
 * @return none
 */
void timer_test_print_timers(void);
#endif /* SUPPORT_HIGH_RES_TIMER */
#endif /* TIMER_TEST_H */
