/*
Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
*/
#include "wifi_cmn.h"
#include "fwconfig_cmn.h"
#include "nt_flags.h"
#ifdef NT_FN_WATCHDOG
#include <stdint.h>

#include "nt_common.h"
#include "nt_hw.h"
#include "nt_logger_api.h"
#include "nt_wdt_api.h"
#include "ferm_prof.h"
#if (NT_CHIP_VERSION==2) || defined (PLATFORM_FERMION)
#include "uart.h"
#endif //(NT_CHIP_VERSION==2) || defined (PLATFORM_FERMION)
#define _WDT_LOAD_SECURE_VAL      0xA1A602E7
#if (NT_CHIP_VERSION==2) || defined (PLATFORM_FERMION)
#define RST_SUCCESS 1
#define RST_FAIL   -1
#endif //(NT_CHIP_VERSION==2) || defined (PLATFORM_FERMION)
#ifdef PLATFORM_FERMION
# define QWLAN_PMU_WDOG_CTL_WDOG_UNMASKED_INT_ENABLE_MASK			0x0
#endif
uint32_t bark_time;
uint32_t bite_time;
static void (*_wdt_callback_fnc_ptr)(void);

/*-------------------------------------------------------------------------------
 * FUNCTION:    nt_watchdog_init(uint32_t _wdog_bite_timout,uint32_t _wdog_bark_timeout)
 *
 * NOTE:
 *    Initialize watchdog timer
 * -------------------------------------------------------------------------------
 */

void
nt_watchdog_init
	(uint32_t _wdog_bite_timout,uint32_t _wdog_bark_timeout)
{
   	uint32_t value;

   	_wdt_callback_fnc_ptr = NULL;


   	value = NT_REG_RD(QWLAN_PMU_ROOT_CLK_ENABLE_REG);
   	value |= QWLAN_PMU_ROOT_CLK_ENABLE_WDOG_XO_ROOT_CLK_ENABLE_MASK;
   	NT_REG_WR(QWLAN_PMU_ROOT_CLK_ENABLE_REG, value);              // Enable the Nps Root clock.


   	value = NT_REG_RD(QWLAN_PMU_AON_TOP_CFG_REG);                  //Read the AON top control reg
   	value |= QWLAN_PMU_AON_TOP_CFG_AON_WDOG_SLP_ROOT_CLK_ENABLE_MASK;
   	NT_REG_WR(QWLAN_PMU_AON_TOP_CFG_REG,value);                   //Enable the AON wdog slp root clk

	value = NT_REG_RD(NT_NVIC_ISER1);                            //Read the watchdog interrupt
	value |=  _WDT_INTR_ENABLE;
	NT_REG_WR(NT_NVIC_ISER1,value);                              //Enable the watchdog interrupt
	nt_disable_watchdog_timer ();



   do {
   		//Nps - when set wdog bark time is synchronizing to sleep_clk. Data value is not guaranteed until this bit is clear.
   		value = NT_REG_RD(QWLAN_PMU_WDOG_BARK_TIME_REG);
   	} while(!((value & QWLAN_PMU_WDOG_BARK_TIME_SYNC_STATUS_MASK) == 0));
   	NT_REG_WR(QWLAN_PMU_WDOG_BARK_TIME_REG, _wdog_bark_timeout);              //load the Nps bark time


   	do {
   		//AON - when set wdog bark time is synchronizing to sleep_clk. Data value is not guaranteed until this bit is clear.
   		value = NT_REG_RD(QWLAN_PMU_AON_WDOG_BARK_TIME_REG);
   	} while(!((value & QWLAN_PMU_AON_WDOG_BARK_TIME_SYNC_STATUS_MASK) == 0));
   	NT_REG_WR(QWLAN_PMU_AON_WDOG_BARK_TIME_REG, _wdog_bark_timeout);              //load the AON bark time


   	NT_REG_WR(QWLAN_PMU_WDOG_BITE_SECURE_REG, _WDT_LOAD_SECURE_VAL);//lock the Nps bite secure reg

   	do {
   		value = NT_REG_RD(QWLAN_PMU_WDOG_BITE_TIME_REG);             //when set wdog bark time is synchronizing to sleep_clk. Data value is not guaranteed until this bit is clear.
   	} while(!((value & QWLAN_PMU_WDOG_BITE_TIME_SYNC_STATUS_MASK) == 0));
   	NT_REG_WR(QWLAN_PMU_WDOG_BITE_TIME_REG, _wdog_bite_timout);               //load the Nps bite time

   	NT_REG_WR(QWLAN_PMU_AON_WDOG_BITE_SECURE_REG,_WDT_LOAD_SECURE_VAL);//lock the AON bite secure reg

   	do {
   		value = NT_REG_RD(QWLAN_PMU_AON_WDOG_BITE_TIME_REG);             //when set wdog bark time is synchronizing to sleep_clk. Data value is not guaranteed until this bit is clear.
   	} while(!((value & QWLAN_PMU_AON_WDOG_BITE_TIME_SYNC_STATUS_MASK) == 0));
   	NT_REG_WR(QWLAN_PMU_AON_WDOG_BITE_TIME_REG, _wdog_bite_timout);          //load the AON bite time

   	nt_enable_watchdog_timer ();

}
/*------------------------------------------------------------------------------------------
 * FUNCTION :     nt_enable_watchdog_timer
 *
 * NOTE:      This function start the watchdog timer
 * ------------------------------------------------------------------------------------------
 */

void
nt_enable_watchdog_timer(
	void)
{
	uint32_t value;

	value = NT_REG_RD(QWLAN_PMU_WDOG_CTL_REG);                       //Read the Nps watchdog timer reg
	value |= QWLAN_PMU_WDOG_CTL_WDOG_ENABLE_MASK | QWLAN_PMU_WDOG_CTL_WDOG_UNMASKED_INT_ENABLE_MASK;
    NT_REG_WR(QWLAN_PMU_WDOG_CTL_REG, value);                       //Enable the Nps watchdog timer and Nps unmasked IRQ or FIQ will enable wdog timer.

	value = NT_REG_RD(QWLAN_PMU_AON_WDOG_CTL_REG);                   //Read the AON watchdog timer reg
	value |= QWLAN_PMU_AON_WDOG_CTL_WDOG_ENABLE_MASK|QWLAN_PMU_AON_WDOG_CTL_WDOG_UNMASKED_INT_ENABLE_MASK;
    NT_REG_WR(QWLAN_PMU_AON_WDOG_CTL_REG, value);                   //Enable the AON watchdog timer.

}
/*---------------------------------------------------------------------------------
 * FUNCTION :   nt_disable_watchdog_timer(void)
 *
 * NOTE : This function stop the watchdog timer
 * --------------------------------------------------------------------------------
 */
void
nt_disable_watchdog_timer(
	void)
{
	uint32_t value;

	value = NT_REG_RD(QWLAN_PMU_WDOG_CTL_REG);                     //Nps - Read the watchdog timer reg
	value &=  (long unsigned int)(~( QWLAN_PMU_WDOG_CTL_WDOG_ENABLE_MASK | QWLAN_PMU_WDOG_CTL_WDOG_UNMASKED_INT_ENABLE_MASK));
    NT_REG_WR(QWLAN_PMU_WDOG_CTL_REG, value);                     //Nps- Disable the watchdog timer

	value = NT_REG_RD(QWLAN_PMU_AON_WDOG_CTL_REG);                     //AON - Read the watchdog timer reg
	value &=  (long unsigned int)(~( QWLAN_PMU_AON_WDOG_CTL_WDOG_ENABLE_MASK | QWLAN_PMU_AON_WDOG_CTL_WDOG_UNMASKED_INT_ENABLE_MASK));
    NT_REG_WR(QWLAN_PMU_AON_WDOG_CTL_REG, value);                     //AON - Disable the watchdog timer
}

/*-----------------------------------------------------------
 * FUNCTION : nt_wdog_int_wcss_wdog_bark(void)
 *
 * NOTE : ISR routine
 * -----------------------------------------------------------
 */
void __attribute__ ((section(".after_ram_vectors")))
nt_wdt_int_wcss_wdog_bark(
	void)
{
	PROF_IRQ_ENTER();

	if(_wdt_callback_fnc_ptr)
		(*_wdt_callback_fnc_ptr)(); // when we are call the pointer variable. it is calling to user registered function.
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
void
nt_wdog_callback_reg(
		void (*ptr)(void))
{
	_wdt_callback_fnc_ptr = ptr;      /*This API provided for the user to register a pointer to a function.
                                   	  which will be called when watchdog bark interrupt occurs.*/

}
/*---------------------------------------------------------------------
 * FUNCTION :  nt_watchdog_bark_timer_reset(void)
 *
 * NOTE : reset the watchdog timer
 * ---------------------------------------------------------------------
 */
void
nt_watchdog_bark_timer_reset(
	void)
{
	uint32_t value;

	value = NT_REG_RD(QWLAN_PMU_WDOG_CTL_REG);                         //Nps - read the watchdog control reg.

	value |= QWLAN_PMU_WDOG_CTL_WDOG_RESET_MASK;
	NT_REG_WR(QWLAN_PMU_WDOG_CTL_REG,value);                          //wirte 1 to Nps wdog ctl reg by using Nps wdog reset mask field. the microprocessor should periodically write the register to reset watch dog.

	value &= (long unsigned int)(~(QWLAN_PMU_WDOG_CTL_WDOG_RESET_MASK));
	NT_REG_WR(QWLAN_PMU_WDOG_CTL_REG,value);                          // write 0 to Nps wdog ctl reg by using wdog reset mask field


	value = NT_REG_RD(QWLAN_PMU_AON_WDOG_CTL_REG);                     //AON - read the watchdog control reg.

	value |= QWLAN_PMU_AON_WDOG_CTL_WDOG_RESET_MASK;
	NT_REG_WR(QWLAN_PMU_AON_WDOG_CTL_REG,value);                      //AON - wirte 1 to AON dog ctl reg by using AON wdog reset mask field. the microprocessor should periodically write the register to reset watch dog.

	value &= (long unsigned int)(~(QWLAN_PMU_AON_WDOG_CTL_WDOG_RESET_MASK));
	NT_REG_WR(QWLAN_PMU_AON_WDOG_CTL_REG,value);                      // write 0 to AON wdog ctl reg by using wdog reset mask field



}
/*---------------------------------------------------------------------
 * FUNCTION :  nt_wdog_bark_bite_time_status(void)
 *
 * NOTE :  bark and bite time status.
 * ---------------------------------------------------------------------
 */
void nt_wdog_bark_bite_time_status(void)
{
	NT_LOG_PRINT(COMMON,INFO,"bark time = %d, bite time = %d", bark_time,bite_time);
	NT_LOG_SME_CRIT("nps wdog counter, aon wdog counter: ", NT_REG_RD(0x11af47c),NT_REG_RD(0x11af8e8), NT_REG_RD(0x11af8e4));
}



void
nt_watchdog_freeze_timer(
		void)
{
	uint32_t value;
	value = NT_REG_RD(QWLAN_PMU_WDOG_CTL_REG);
	value |=(1 << QWLAN_PMU_WDOG_CTL_WDOG_FREEZE_OFFSET); //Watchdog freeze enable
	NT_REG_WR(QWLAN_PMU_WDOG_CTL_REG,value);

	value = NT_REG_RD(QWLAN_PMU_AON_WDOG_CTL_REG);                     //AON - read the watchdog control reg.
	value |= (1 << QWLAN_PMU_AON_WDOG_CTL_WDOG_FREEZE_OFFSET);        //Watchdog freeze enable
	NT_REG_WR(QWLAN_PMU_AON_WDOG_CTL_REG,value);
}
void
nt_watchdog_unfreeze_timer(
		void)
{
	uint32_t value;


	value = NT_REG_RD(QWLAN_PMU_AON_WDOG_CTL_REG);                     //AON - read the watchdog control reg.

	value |= QWLAN_PMU_AON_WDOG_CTL_WDOG_RESET_MASK;
	NT_REG_WR(QWLAN_PMU_AON_WDOG_CTL_REG,value);                      //AON - wirte 1 to AON dog ctl reg by using AON wdog reset mask field. the microprocessor should periodically write the register to reset watch dog.

	value &= (long unsigned int)(~(QWLAN_PMU_AON_WDOG_CTL_WDOG_RESET_MASK));
	NT_REG_WR(QWLAN_PMU_AON_WDOG_CTL_REG,value);                      // write 0 to AON wdog ctl reg by using wdog reset mask field

	value = NT_REG_RD(QWLAN_PMU_WDOG_CTL_REG);                         //Nps - read the watchdog control reg.

	value |= QWLAN_PMU_WDOG_CTL_WDOG_RESET_MASK;
	NT_REG_WR(QWLAN_PMU_WDOG_CTL_REG,value);                          //wirte 1 to Nps wdog ctl reg by using Nps wdog reset mask field. the microprocessor should periodically write the register to reset watch dog.

	value &= (long unsigned int)(~(QWLAN_PMU_WDOG_CTL_WDOG_RESET_MASK));
	NT_REG_WR(QWLAN_PMU_WDOG_CTL_REG,value);

	value = NT_REG_RD(QWLAN_PMU_WDOG_CTL_REG);
	value &= (~(QWLAN_PMU_WDOG_CTL_WDOG_FREEZE_MASK));//Watchdog freeze disable
	NT_REG_WR(QWLAN_PMU_WDOG_CTL_REG,value);

	value = NT_REG_RD(QWLAN_PMU_AON_WDOG_CTL_REG);                     //AON - read the watchdog control reg.

	value &= (~(1 << QWLAN_PMU_AON_WDOG_CTL_WDOG_FREEZE_OFFSET));
	NT_REG_WR(QWLAN_PMU_AON_WDOG_CTL_REG,value);
}

#if (NT_CHIP_VERSION==2) || defined (PLATFORM_FERMION)
uint8_t warm_boot_by_aon_wdog_retention_cMem_banks(uint8_t bank)
{
	uint32_t top_cfg_val;
	uint32_t boot_cfg_val;
	uint8_t reslt;
	top_cfg_val = NT_REG_RD(QWLAN_PMU_DIG_TOP_CFG_REG);
	boot_cfg_val = NT_REG_RD(QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_REG);
	if(bank == NT_WDT_CMEM_BANK_B)
	{
		top_cfg_val |= ( 1 << QWLAN_PMU_DIG_TOP_CFG_FORCE_BANK_B_CORE_ON_OFFSET);
	    boot_cfg_val |= ( 1 << QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_PD_CMEM_BANK_B_CNTL_BIT_OFFSET);
	    reslt = RST_SUCCESS;
	}
	else if(bank == NT_WDT_CMEM_BANK_C)
	{
		top_cfg_val |= ( 1 << QWLAN_PMU_DIG_TOP_CFG_FORCE_BANK_C_CORE_ON_OFFSET);
	    boot_cfg_val |= ( 1 << QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_PD_CMEM_BANK_C_CNTL_BIT_OFFSET);
	    reslt = RST_SUCCESS;
	}
	else if(bank == NT_WDT_CMEM_BANK_D)
	{
		top_cfg_val |= ( 1 << QWLAN_PMU_DIG_TOP_CFG_FORCE_BANK_D_CORE_ON_OFFSET);
	    boot_cfg_val |= ( 1 << QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_PD_CMEM_BANK_D_CNTL_BIT_OFFSET);
	    reslt = RST_SUCCESS;
	}
	else
	{
		NT_LOG_PRINT(COMMON,ERR,"Error for enabling retention of bank ");
		reslt = RST_FAIL;
	}
	NT_REG_WR(QWLAN_PMU_DIG_TOP_CFG_REG,top_cfg_val);
	NT_REG_WR(QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_REG,boot_cfg_val);

	return reslt;
}
#endif //(NT_CHIP_VERSION==2) || defined (PLATFORM_FERMION)
#ifndef DEBUG

/*-------------------------------------------------------------------------------
 * FUNCTION :      nt_wdt_reload(uint32_t *Reloadvalue)
 *
 * NOTE : This function used reload the value. This function debugging purpose.
 * @param info about the reload value.
 * -------------------------------------------------------------------------------
 */
void
nt_wdt_reload(
		uint32_t *Reloadvalue)
{
	int value;
	do {
		value = NT_REG_RD(QWLAN_PMU_WDOG_TEST_REG);
	}while(!((value & (1 << QWLAN_PMU_WDOG_TEST_SYNC_STATUS_OFFSET)) == 0)); //Reload the value;
	NT_REG_WR(QWLAN_PMU_WDOG_TEST_REG,*Reloadvalue);

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

#endif //NT_FN_WATCHDOG

#endif
