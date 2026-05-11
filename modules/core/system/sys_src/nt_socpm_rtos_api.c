/*
 */
// -------------------------------------------------------------------

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
#include "FreeRTOS.h"
#include "task.h"
#include "uart.h"
#include "ExceptionHandlers.h"
#include "fwconfig_cmn.h"
#include "nt_flags.h"

#include "nt_common.h"
#include "nt_logger_api.h"
#include "nt_devcfg.h"
#include "nt_socpm_rtos_api.h"
#include "nt_socpm_sleep.h"

#include "nt_hw.h"
#include "nt_hw_support.h"
#include "qurt_internal.h"

#include "wlan_power.h"
#include "hal_int_powersave.h"
#include "nt_wfm_wmi_interface.h"
//#include "wifi_app.h"

#include "timer.h"
#include "wifi_fw_ext_intr.h"

#ifdef NT_GPIO_FLAG
#include "nt_gpio_api.h"
#endif

#include "wps_def.h"

#ifdef NT_FN_CPR
#include "nt_cpr_driver.h"
#endif  // NT_FN_CPR

#include "qtmr.h"
#ifdef IMAGE_FERMION
#include "wifi_fw_internal_api.h"
#include "mlme_api.h"
#endif /*IMAGE_FERMION*/

#ifdef PLATFORM_FERMION
#include "HALhwio.h"
#include "Fermion_seq_hwioreg.h"
#endif /* PLATFORM_FERMION */

#include "fdi_rmc.h"
#include "wifi_fw_pwr_cb_infra.h"
#include "wifi_fw_pmic_driver.h"

#ifdef SUPPORT_COEX
#include "coex_utils.h"
#include "coex_test.h"
#endif
#include "wlan_power.h"
#include "nt_imps.h"

extern TickType_t xMaximumPossibleSuppressedTicks;
extern SOCPM_STRUCT g_socpm_struct;
extern volatile int nt_socpm_resume_f;
extern uint64_t hres_time_pre_sleep;

/* Flag set from the tick interrupt to allow the sleep processing to know if
 sleep mode was exited because of an Sleep timer interrupt or a different interrupt. */
static volatile uint32_t ulTickFlag = pdFALSE;

// To save systick current count before going to sleep
static uint32_t _socpm_systick_save;

void _socpm_systick_off(void)
{
    // disable the systick
    _socpm_systick_save = portNVIC_SYSTICK_CURRENT_VALUE_REG;
    portNVIC_SYSTICK_CTRL_REG &= ~portNVIC_SYSTICK_ENABLE_BIT;
}

/*-----------------------------------------------------------*/
void _socpm_systick_on(void)
{
    if (nt_socpm_resume_f == 0) {
        nt_socpm_resume_f = 2;
        /* Restart from whatever is left in the count register to complete
         this tick period. */
        portNVIC_SYSTICK_LOAD_REG = (configCPU_CLOCK_HZ / configTICK_RATE_HZ) - 1UL;
        portNVIC_SYSTICK_CURRENT_VALUE_REG = 0;
        /* Restart SysTick. */
        portNVIC_SYSTICK_CTRL_REG = (_SOCPM_SYSTICK_CLK_BIT | portNVIC_SYSTICK_INT_BIT | portNVIC_SYSTICK_ENABLE_BIT);
    } else
        portNVIC_SYSTICK_CTRL_REG |= portNVIC_SYSTICK_ENABLE_BIT;

    /* The CPU woke because of a tick. */
    ulTickFlag = pdTRUE;
}

/* Override the default definition of vPortSuppressTicksAndSleep() that is weakly
 defined in the FreeRTOS Cortex-M3 port layer with a version that manages the
 asynchronous timer (Sleep timer), as the tick is generated from the low power Sleep timer and
 not the SysTick as would normally be the case on a Cortex-M. */
extern bool nt_tcp_has_pending_acks(void);
extern void nt_tcp_flush_acks_cb(void *ctx);

void vPortSuppressTicksAndSleep(TickType_t xExpectedIdleTime)
{
    if (nt_tcp_has_pending_acks()) { 
        tcpip_try_callback(nt_tcp_flush_acks_cb, NULL); 
        return; 
    }
    eSleepModeStatus eSleepAction;
    uint64_t slp_val = 0;

    FDI_NODE_START_NULL(FDI_DBG_PWR_W2S_IDLE_TASK_KICK_IN);
    /* THIS FUNCTION IS CALLED WITH THE SCHEDULER SUSPENDED. */
    /* Mget ake sure the Sleep timer reload value does not overflow the counter. */
    if (xExpectedIdleTime > xMaximumPossibleSuppressedTicks) {
        xExpectedIdleTime = xMaximumPossibleSuppressedTicks;
    }
    /* Stop the SysTick momentarily  */
    _socpm_systick_off();
    hres_time_pre_sleep = hres_timer_curr_time_us();
#ifdef COMPENSATE_AON_PROG_DELAY
    g_socpm_struct.systick_off_time_us = (uint32_t)hres_timer_curr_time_us();
#endif /* COMPENSATE_AON_PROG_DELAY */
    /* Calculate the reload value required to wait xExpectedIdleTime tick
     periods. */
    slp_val = xExpectedIdleTime;
    if (slp_val > _SOCPM_STOP_TMR_COMP) {
        /* Compensate for the fact that the Sleep timer is going to be stopped
         momentarily. */
        slp_val -= _SOCPM_STOP_TMR_COMP;
    }
#ifdef NT_DEBUG
    // store the pending interrupts before sleep
    g_socpm_struct.pre_sleep_nvic_icpr_status[0] = NT_REG_RD(NT_CM4_NVIC_ISER0_CLEAR_PENDING_REG);
    g_socpm_struct.pre_sleep_nvic_icpr_status[1] = NT_REG_RD(NT_CM4_NVIC_ISER1_CLEAR_PENDING_REG);
    g_socpm_struct.pre_sleep_nvic_icpr_status[2] = NT_REG_RD(NT_CM4__NVIC_ISER2_CLEAR_PENDING_REG);
    g_socpm_struct.pre_sleep_nvic_icpr_status[3] = NT_REG_RD(NT_CM4_NVIC_ISER3_CLEAR_PENDING_REG);
#endif /*NT_DEBUG*/
    __asm volatile("dsb" ::: "memory");
    __asm volatile("isb");
    /* Enter a critical section but don't use the taskENTER_CRITICAL() method as
     that will mask interrupts that should exit sleep mode. */
    NT_SOCPM_IRQ_DISABLE();

    /* The tick flag is set to false before sleeping.  If it is true when sleep
     mode is exited then sleep mode was probably exited because the tick was
     suppressed for the entire xExpectedIdleTime period. */
    ulTickFlag = pdFALSE;

    /* If a context switch is pending then abandon the low power entry as
     the context switch might have been pended by an external interrupt that
     requires processing. */
    eSleepAction = eTaskConfirmSleepModeStatus();
    if (eSleepAction == eAbortSleep) {
        /* Restart tick. */
        _socpm_systick_on();
        /* Re-enable interrupts */
        NT_SOCPM_IRQ_ENABLE();
    } else {
        nt_socpm_soc_sleep_processing(slp_val);
    }
}
