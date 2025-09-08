/*
Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
*/
#include "wifi_cmn.h"
#include "fwconfig_cmn.h"
#include "nt_flags.h"
#include <stdint.h>

#include "nt_common.h"
//#ifdef NT_FN_WATCHDOG
#include "nt_hw.h"
#include "nt_logger_api.h"
#include "nt_wdt_api.h"
#include "ferm_prof.h"
#include "nt_timer.h"
#include "wifi_fw_pwr_cb_infra.h"
#if (NT_CHIP_VERSION == 2) || defined(PLATFORM_FERMION)
#include "uart.h"
#endif  //(NT_CHIP_VERSION==2) || defined (PLATFORM_FERMION)
#if CONFIG_WATCH_DOG_ENABLE
#define _WDT_LOAD_SECURE_VAL 0xA1A602E7
#if (NT_CHIP_VERSION == 2) || defined(PLATFORM_FERMION)
#define RST_SUCCESS 1
#define RST_FAIL    -1
#endif  //(NT_CHIP_VERSION==2) || defined (PLATFORM_FERMION)
#ifdef PLATFORM_FERMION
#define QWLAN_PMU_WDOG_CTL_WDOG_UNMASKED_INT_ENABLE_MASK     0x0
#define QWLAN_PMU_AON_WDOG_CTL_WDOG_UNMASKED_INT_ENABLE_MASK 0x0
#endif
uint32_t bark_time;
uint32_t bite_time;
static void (*_wdt_callback_fnc_ptr)(void);
#define WDOG_TIMER_NAME "wdog_timer"
TimerHandle_t wdt_timer_handle;

#define NT_WATCH_DOG_DEBUG 0
#if NT_WATCH_DOG_DEBUG
char *nt_wdog_bark_str = "Bark\n";
char *nt_wdog_sw_pet_str = "Pet\n";
#endif
extern void PAL_Console_Write(uint32_t Length, const char *Buffer);
extern void vPortEnterCritical(void);
extern void vPortExitCritical(void);

/*-------------------------------------------------------------------------------
 * FUNCTION:    nt_watchdog_init(uint32_t _wdog_bite_timout,uint32_t _wdog_bark_timeout)
 *
 * NOTE:
 *    Initialize watchdog timer
 * -------------------------------------------------------------------------------
 */

void nt_watchdog_init(uint32_t _wdog_bite_timout, uint32_t _wdog_bark_timeout)
{
    uint32_t value;

    _wdt_callback_fnc_ptr = NULL;

    value = NT_REG_RD(QWLAN_PMU_ROOT_CLK_ENABLE_REG);
    value |= QWLAN_PMU_ROOT_CLK_ENABLE_WDOG_XO_ROOT_CLK_ENABLE_MASK;
    NT_REG_WR(QWLAN_PMU_ROOT_CLK_ENABLE_REG, value);  // Enable the Nps Root clock.

    value = NT_REG_RD(QWLAN_PMU_AON_TOP_CFG_REG);  // Read the AON top control reg
    value |= QWLAN_PMU_AON_TOP_CFG_AON_WDOG_SLP_ROOT_CLK_ENABLE_MASK;
    NT_REG_WR(QWLAN_PMU_AON_TOP_CFG_REG, value);  // Enable the AON wdog slp root clk

    do {
        // Nps - when set wdog bark time is synchronizing to sleep_clk. Data value is not guaranteed until this bit is
        // clear.
        value = NT_REG_RD(QWLAN_PMU_WDOG_BARK_TIME_REG);
    } while (!((value & QWLAN_PMU_WDOG_BARK_TIME_SYNC_STATUS_MASK) == 0));
    NT_REG_WR(QWLAN_PMU_WDOG_BARK_TIME_REG, _wdog_bark_timeout);  // load the Nps bark time

    do {
        // AON - when set wdog bark time is synchronizing to sleep_clk. Data value is not guaranteed until this bit is
        // clear.
        value = NT_REG_RD(QWLAN_PMU_AON_WDOG_BARK_TIME_REG);
    } while (!((value & QWLAN_PMU_AON_WDOG_BARK_TIME_SYNC_STATUS_MASK) == 0));
    NT_REG_WR(QWLAN_PMU_AON_WDOG_BARK_TIME_REG, _wdog_bark_timeout);  // load the AON bark time

    NT_REG_WR(QWLAN_PMU_WDOG_BITE_SECURE_REG, _WDT_LOAD_SECURE_VAL);  // lock the Nps bite secure reg

    do {
        value = NT_REG_RD(QWLAN_PMU_WDOG_BITE_TIME_REG);  // when set wdog bark time is synchronizing to sleep_clk. Data
                                                          // value is not guaranteed until this bit is clear.
    } while (!((value & QWLAN_PMU_WDOG_BITE_TIME_SYNC_STATUS_MASK) == 0));
    NT_REG_WR(QWLAN_PMU_WDOG_BITE_TIME_REG, _wdog_bite_timout);  // load the Nps bite time

    NT_REG_WR(QWLAN_PMU_AON_WDOG_BITE_SECURE_REG, _WDT_LOAD_SECURE_VAL);  // lock the AON bite secure reg

    do {
        value = NT_REG_RD(QWLAN_PMU_AON_WDOG_BITE_TIME_REG);  // when set wdog bark time is synchronizing to sleep_clk.
                                                              // Data value is not guaranteed until this bit is clear.
    } while (!((value & QWLAN_PMU_AON_WDOG_BITE_TIME_SYNC_STATUS_MASK) == 0));
    NT_REG_WR(QWLAN_PMU_AON_WDOG_BITE_TIME_REG, _wdog_bite_timout);  // load the AON bite time

    nt_enable_watchdog_timer();
}
/*------------------------------------------------------------------------------------------
 * FUNCTION :     nt_enable_watchdog_timer
 *
 * NOTE:      This function start the watchdog timer
 * ------------------------------------------------------------------------------------------
 */

void nt_enable_watchdog_timer(void)
{
    uint32_t value;

    value = NT_REG_RD(QWLAN_PMU_AON_WDOG_CTL_REG);  // Read the AON watchdog timer reg
    value |= QWLAN_PMU_AON_WDOG_CTL_WDOG_ENABLE_MASK | QWLAN_PMU_AON_WDOG_CTL_WDOG_UNMASKED_INT_ENABLE_MASK;
    NT_REG_WR(QWLAN_PMU_AON_WDOG_CTL_REG, value);  // Enable the AON watchdog timer.
}
/*---------------------------------------------------------------------------------
 * FUNCTION :   nt_disable_watchdog_timer(void)
 *
 * NOTE : This function stop the watchdog timer
 * --------------------------------------------------------------------------------
 */
void nt_disable_watchdog_timer(void)
{
    uint32_t value;

    value = NT_REG_RD(QWLAN_PMU_WDOG_CTL_REG);  // Nps - Read the watchdog timer reg
    value &=
        (long unsigned int)(~(QWLAN_PMU_WDOG_CTL_WDOG_ENABLE_MASK | QWLAN_PMU_WDOG_CTL_WDOG_UNMASKED_INT_ENABLE_MASK));
    NT_REG_WR(QWLAN_PMU_WDOG_CTL_REG, value);  // Nps- Disable the watchdog timer

    value = NT_REG_RD(QWLAN_PMU_AON_WDOG_CTL_REG);  // AON - Read the watchdog timer reg
    value &= (long unsigned int)(~(QWLAN_PMU_AON_WDOG_CTL_WDOG_ENABLE_MASK |
                                   QWLAN_PMU_AON_WDOG_CTL_WDOG_UNMASKED_INT_ENABLE_MASK));
    NT_REG_WR(QWLAN_PMU_AON_WDOG_CTL_REG, value);  // AON - Disable the watchdog timer
}

/*-----------------------------------------------------------
 * FUNCTION : nt_wdog_int_wcss_wdog_bark(void)
 *
 * NOTE : ISR routine
 * -----------------------------------------------------------
 */
void __attribute__((section(".after_ram_vectors"))) nt_wdt_int_wcss_wdog_bark(void)
{
    PROF_IRQ_ENTER();

    if (_wdt_callback_fnc_ptr)
        (*_wdt_callback_fnc_ptr)();  // when we are call the pointer variable. it is calling to user registered
                                     // function.
    else
        nt_disable_watchdog_timer();

    PROF_IRQ_EXIT();
}
/*--------------------------------------------------------------
 * FUNCTION :  nt_wdog_callback_reg(void (*ptr)(void))
 *
 * NOTE : This API provided for user to register a pointer to a function.
 * ------------------------------------------------------------------
 */
void nt_wdog_callback_reg(void (*ptr)(void))
{
    _wdt_callback_fnc_ptr = ptr; /*This API provided for the user to register a pointer to a function.
                                 which will be called when watchdog bark interrupt occurs.*/
}
/*---------------------------------------------------------------------
 * FUNCTION :  nt_watchdog_bark_timer_reset(void)
 *
 * NOTE : reset the watchdog timer
 * ---------------------------------------------------------------------
 */
void nt_watchdog_bark_timer_reset(void)
{
    uint32_t value;
    volatile int count;

    value = NT_REG_RD(QWLAN_PMU_AON_WDOG_CTL_REG);  // AON - read the watchdog control reg.

    value |= QWLAN_PMU_AON_WDOG_CTL_WDOG_RESET_MASK;
    NT_REG_WR(QWLAN_PMU_AON_WDOG_CTL_REG,
              value);  // AON - wirte 1 to AON dog ctl reg by using AON wdog reset mask field. the microprocessor should
                       // periodically write the register to reset watch dog.

    value &= (long unsigned int)(~(QWLAN_PMU_AON_WDOG_CTL_WDOG_RESET_MASK));
    NT_REG_WR(QWLAN_PMU_AON_WDOG_CTL_REG, value);  // write 0 to AON wdog ctl reg by using wdog reset mask field
}
/*---------------------------------------------------------------------
 * FUNCTION :  nt_wdog_bark_bite_time_status(void)
 *
 * NOTE :  bark and bite time status.
 * ---------------------------------------------------------------------
 */
void nt_wdog_bark_bite_time_status(void)
{
    NT_LOG_PRINT(COMMON, INFO, "bark time = %d, bite time = %d", bark_time, bite_time);
    NT_LOG_SME_CRIT("nps wdog counter, aon wdog counter: ", NT_REG_RD(0x11af47c), NT_REG_RD(0x11af8e8),
                    NT_REG_RD(0x11af8e4));
}

void nt_watchdog_freeze_timer(void)
{
    uint32_t value;
    value = NT_REG_RD(QWLAN_PMU_WDOG_CTL_REG);
    value |= (1 << QWLAN_PMU_WDOG_CTL_WDOG_FREEZE_OFFSET);  // Watchdog freeze enable
    NT_REG_WR(QWLAN_PMU_WDOG_CTL_REG, value);

    value = NT_REG_RD(QWLAN_PMU_AON_WDOG_CTL_REG);              // AON - read the watchdog control reg.
    value |= (1 << QWLAN_PMU_AON_WDOG_CTL_WDOG_FREEZE_OFFSET);  // Watchdog freeze enable
    NT_REG_WR(QWLAN_PMU_AON_WDOG_CTL_REG, value);
}
void nt_watchdog_unfreeze_timer(void)
{
    uint32_t value;

    value = NT_REG_RD(QWLAN_PMU_AON_WDOG_CTL_REG);  // AON - read the watchdog control reg.

    value |= QWLAN_PMU_AON_WDOG_CTL_WDOG_RESET_MASK;
    NT_REG_WR(QWLAN_PMU_AON_WDOG_CTL_REG,
              value);  // AON - wirte 1 to AON dog ctl reg by using AON wdog reset mask field. the microprocessor should
                       // periodically write the register to reset watch dog.

    value &= (long unsigned int)(~(QWLAN_PMU_AON_WDOG_CTL_WDOG_RESET_MASK));
    NT_REG_WR(QWLAN_PMU_AON_WDOG_CTL_REG, value);  // write 0 to AON wdog ctl reg by using wdog reset mask field

    value = NT_REG_RD(QWLAN_PMU_WDOG_CTL_REG);  // Nps - read the watchdog control reg.

    value |= QWLAN_PMU_WDOG_CTL_WDOG_RESET_MASK;
    NT_REG_WR(QWLAN_PMU_WDOG_CTL_REG,
              value);  // wirte 1 to Nps wdog ctl reg by using Nps wdog reset mask field. the microprocessor should
                       // periodically write the register to reset watch dog.

    value &= (long unsigned int)(~(QWLAN_PMU_WDOG_CTL_WDOG_RESET_MASK));
    NT_REG_WR(QWLAN_PMU_WDOG_CTL_REG, value);

    value = NT_REG_RD(QWLAN_PMU_WDOG_CTL_REG);
    value &= (~(QWLAN_PMU_WDOG_CTL_WDOG_FREEZE_MASK));  // Watchdog freeze disable
    NT_REG_WR(QWLAN_PMU_WDOG_CTL_REG, value);

    value = NT_REG_RD(QWLAN_PMU_AON_WDOG_CTL_REG);  // AON - read the watchdog control reg.

    value &= (~(1 << QWLAN_PMU_AON_WDOG_CTL_WDOG_FREEZE_OFFSET));
    NT_REG_WR(QWLAN_PMU_AON_WDOG_CTL_REG, value);
}

#if (NT_CHIP_VERSION == 2) || defined(PLATFORM_FERMION)
uint8_t warm_boot_by_aon_wdog_retention_cMem_banks(uint8_t bank)
{
    uint32_t top_cfg_val;
    uint32_t boot_cfg_val;
    uint8_t reslt;
    top_cfg_val = NT_REG_RD(QWLAN_PMU_DIG_TOP_CFG_REG);
    boot_cfg_val = NT_REG_RD(QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_REG);
    if (bank == NT_WDT_CMEM_BANK_B) {
        top_cfg_val |= (1 << QWLAN_PMU_DIG_TOP_CFG_FORCE_BANK_B_CORE_ON_OFFSET);
        boot_cfg_val |=
            (1 << QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_PD_CMEM_BANK_B_CNTL_BIT_OFFSET);
        reslt = RST_SUCCESS;
    } else if (bank == NT_WDT_CMEM_BANK_C) {
        top_cfg_val |= (1 << QWLAN_PMU_DIG_TOP_CFG_FORCE_BANK_C_CORE_ON_OFFSET);
        boot_cfg_val |=
            (1 << QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_PD_CMEM_BANK_C_CNTL_BIT_OFFSET);
        reslt = RST_SUCCESS;
    } else if (bank == NT_WDT_CMEM_BANK_D) {
        top_cfg_val |= (1 << QWLAN_PMU_DIG_TOP_CFG_FORCE_BANK_D_CORE_ON_OFFSET);
        boot_cfg_val |=
            (1 << QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_PD_CMEM_BANK_D_CNTL_BIT_OFFSET);
        reslt = RST_SUCCESS;
    } else {
        NT_LOG_PRINT(COMMON, ERR, "Error for enabling retention of bank ");
        reslt = RST_FAIL;
    }
    NT_REG_WR(QWLAN_PMU_DIG_TOP_CFG_REG, top_cfg_val);
    NT_REG_WR(QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_REG, boot_cfg_val);

    return reslt;
}
#endif  //(NT_CHIP_VERSION==2) || defined (PLATFORM_FERMION)
#ifndef DEBUG

/*-------------------------------------------------------------------------------
 * FUNCTION :      nt_wdt_reload(uint32_t *Reloadvalue)
 *
 * NOTE : This function used reload the value. This function debugging purpose.
 * @param info about the reload value.
 * -------------------------------------------------------------------------------
 */
void nt_wdt_reload(uint32_t *Reloadvalue)
{
    int value;
    do {
        value = NT_REG_RD(QWLAN_PMU_WDOG_TEST_REG);
    } while (!((value & (1 << QWLAN_PMU_WDOG_TEST_SYNC_STATUS_OFFSET)) == 0));  // Reload the value;
    NT_REG_WR(QWLAN_PMU_WDOG_TEST_REG, *Reloadvalue);
}
/*------------------------------------------------------------------------------
 * FUNCTION :       nt_wdt_freeze_start(void),nt_wdt_freeze_stop(void)
 *
 * NOTE : nt_wdt_freeze_start, enable freeze the watchdog count and
 *        nt_wdt_freeze_stop, disable freeze the watchdog count.
 * -----------------------------------------------------------------------------
 */

#if 0
void nt_wdt_warm_cold_boot_status(void)
{
	char buffer[100];
	uint32_t sys_stus = NT_REG_RD(QWLAN_PMU_SYSTEM_STATUS_REG);
	if( ( (sys_stus & QWLAN_PMU_SYSTEM_STATUS_COLD_WARM_BOOT_MASK) == QWLAN_PMU_SYSTEM_STATUS_COLD_WARM_BOOT_MASK ) )
	{
		//NT_LOG_PRINT(COMMON,INFO,"Neutrino-2: Warm boot \r\n");
		snprintf(buffer,sizeof(buffer),"Neutrino-2: Warm boot");
		nt_dbg_print(buffer);
	}
	else
	{
		//NT_LOG_PRINT(COMMON,INFO,"Neutrino-2: cold boot \r\n");
		snprintf(buffer,sizeof(buffer),"Neutrino-2: cold boot");
		nt_dbg_print(buffer);
	}
}
#endif

#endif  // NT_FN_WATCHDOG

void nt_watchdog_timer_call_back()
{
#if NT_WATCH_DOG_DEBUG
    PAL_Console_Write(strlen(nt_wdog_sw_pet_str), nt_wdog_sw_pet_str);
#endif
    vPortEnterCritical();
    nt_watchdog_bark_timer_reset();
    vPortExitCritical();
}

void nt_watchdog_timer_bark_call_back()
{
#if NT_WATCH_DOG_DEBUG
    PAL_Console_Write(strlen(nt_wdog_bark_str), nt_wdog_bark_str);
#endif
}

void nt_watchdog_timer_power_state_change_cb(uint8_t evt, void *p_args)
{
    (void)p_args;

    if (evt == PWR_EVT_WMAC_PRE_SLEEP) {
        nt_watchdog_bark_timer_reset();
        nt_watchdog_swtimer_stop();
    }

    if ((evt == PWR_EVT_WMAC_POST_AWAKE) || (evt == PWR_EVT_WMAC_SLEEP_ABORT)) {
        qurt_timer_start(wdt_timer_handle, (TickType_t)100);
        nt_watchdog_bark_timer_reset();
    }
}

void nt_watchdog_timer_init(void)
{

    bark_time = CONFIG_WATCH_DOG_BITE_TIME * 1000;
    bite_time = CONFIG_WATCH_DOG_BITE_TIME * 1000;

    wdt_timer_handle = nt_qurt_timer_create(WDOG_TIMER_NAME, NT_MS_TO_TICKS((bark_time - 1000)), TRUE, NULL,
                                            nt_watchdog_timer_call_back);

    if (!wdt_timer_handle)
        return;

    /*write the AON bark time same as bite time, since we feed dog using software timer*/
    nt_watchdog_init(bite_time, bark_time);
    fpci_evt_cb_reg((ps_evt_cb_t)&nt_watchdog_timer_power_state_change_cb,
                    PWR_EVT_WMAC_PRE_SLEEP | PWR_EVT_WMAC_POST_AWAKE | PWR_EVT_WMAC_SLEEP_ABORT, 10, NULL);
    nt_wdog_callback_reg(&nt_watchdog_timer_bark_call_back);
    nt_watchdog_unfreeze_timer();

    qurt_timer_start(wdt_timer_handle, (TickType_t)100);
}

void nt_watchdog_timer_freeze(void)
{
    nt_watchdog_freeze_timer();
}
void nt_watchdog_swtimer_stop(void)
{
    qurt_timer_stop(wdt_timer_handle, (TickType_t)0);
}

void nt_watchdog_timer_restart(void)
{
    PAL_Console_Write(3, "R\r\n");

    nt_watchdog_freeze_timer();
    nt_watchdog_init(bite_time, bark_time);
    nt_wdog_callback_reg(&nt_watchdog_timer_bark_call_back);
    nt_watchdog_unfreeze_timer();
    qurt_timer_start(wdt_timer_handle, (TickType_t)100);
}

#else
void nt_watchdog_timer_init(void) {}

void nt_watchdog_timer_freeze(void) {}

void nt_watchdog_swtimer_stop(void) {}

void nt_watchdog_timer_restart(void) {}

void nt_watchdog_bark_timer_reset(void) {}

#endif
