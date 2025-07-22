
/*
 * FreeRTOS Kernel V10.2.0
 * Copyright (C) 2019 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

/*-----------------------------------------------------------
 * Implementation of functions defined in portable.h for the ARM CM4F port.
 *----------------------------------------------------------*/

#ifndef _NT_SOCPM_RTOS_API_H_
#define _NT_SOCPM_RTOS_API_H_

#include <stdint.h>
#include <stdbool.h>
#include "nt_flags.h"
#include "nt_common.h"
#include "nt_osal.h"
#include "timer.h"

// Systick Register to load on system resume
#define configSYSTICK_CLOCK_HZ             (60000000u)
#define portNVIC_SYSTICK_CLK_BIT           (1UL << 2UL)
#define portNVIC_SYSTICK_CTRL_REG          (*((volatile uint32_t *)0xe000e010))
#define portNVIC_SYSTICK_LOAD_REG          (*((volatile uint32_t *)0xe000e014))
#define portNVIC_SYSTICK_CURRENT_VALUE_REG (*((volatile uint32_t *)0xe000e018))
#define portNVIC_SYSPRI2_REG               (*((volatile uint32_t *)0xe000ed20))
/* ...then bits in the registers. */
#define portNVIC_SYSTICK_INT_BIT        (1UL << 1UL)
#define portNVIC_SYSTICK_ENABLE_BIT     (1UL << 0UL)
#define portNVIC_SYSTICK_COUNT_FLAG_BIT (1UL << 16UL)
#define portNVIC_PENDSVCLEAR_BIT        (1UL << 27UL)
#define portNVIC_PEND_SYSTICK_CLEAR_BIT (1UL << 25UL)

// tick compensation for vPreSleepProcessing + vPostSleepProcessing
//  it is assumption at this stage need to be dynamic
//  reduced to 2ms (from 20), but can possibly be even smaller
#define _SOCPM_STOP_TMR_COMP 2

// see portNVIC_SYSTICK_CLK_BIT
#define _SOCPM_SYSTICK_CLK_BIT (1UL << 2UL)

void vPortSuppressTicksAndSleep(TickType_t xExpectedIdleTime);

void _socpm_systick_off(void);
void _socpm_systick_on(void);

#endif /* _NT_SOCPM_RTOS_API_H_ */
