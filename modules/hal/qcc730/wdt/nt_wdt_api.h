/*
Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
*/
#ifndef _WATCHDOG_H
#define _WATCHDOG_H

#include "fwconfig_cmn.h"
#include "nt_flags.h"

#include <stdint.h>

/*
 * user configurations for watch-dog timer
 */

#define _WATCHDOG_BARK_TIMEOUT 10240  // 10sec
//#define NT_WDT_DEFAULT_BITE_S 13312  //13sec
#define _WATCHDOG_BITE_TIMEOUT 61440  // 1min

#define NT_NVIC_ISER1    0xE000E104   // Irq 32 to 60 Set Enable Register
#define _WDT_INTR_ENABLE (0x1 << 22)  // Enable wdt interrupt
// watchdog timer.c functions....
#if (NT_CHIP_VERSION == 2) || defined(PLATFORM_FERMION)
#define NT_WDT_CMEM_BANK_A       1
#define NT_WDT_CMEM_BANK_B       2
#define NT_WDT_CMEM_BANK_C       3
#define NT_WDT_CMEM_BANK_D       4
#define NT_WDT_CMEM_BANK_B_C_D   5
#define NT_WDT_CMEM_BANK_A_B_C_D 6
#endif  //(NT_CHIP_VERSION==2) || defined (PLATFORM_FERMION)

void nt_watchdog_bark_timer_reset(void);
void nt_disable_watchdog_timer(void);
void nt_enable_watchdog_timer(void);
void nt_watchdog_init(uint32_t _wdog_bite_timout, uint32_t _wdog_bark_timeout);
void nt_wdog_int_wcss_wdog_bark(void);
void nt_wdog_callback_reg(void (*ptr)(void));
void nt_wdog_bark_bite_time_status(void);
#if (NT_CHIP_VERSION == 2) || defined(PLATFORM_FERMION)
uint8_t warm_boot_by_aon_wdog_retention_cMem_banks(uint8_t bank);
WIFIReturnCode_t _nt_wdt_init(void *);
#endif  //(NT_CHIP_VERSION==2) || defined (PLATFORM_FERMION)

void nt_watchdog_freeze_timer(void);
void nt_watchdog_unfreeze_timer(void);

void nt_watchdog_timer_init(void);
void nt_watchdog_timer_freeze(void);
void nt_watchdog_timer_restart(void);

void nt_watchdog_swtimer_stop(void);

//#if (defined __NT_2_H)

//#endif	// __NT_2_H

#endif /* _WATCHDOG_H */
