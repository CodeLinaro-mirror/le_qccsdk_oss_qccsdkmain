/*
* * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
* * SPDX-License-Identifier: BSD-3-Clause-Clear
*/
#include <stdio.h>
#include "boot_debug.h"
#include "boot_print.h"
#include "nt_bl_uart.h"
#include "nt_bl_wdt.h"
#include "boot_elf_loader.h"
#include "sbl_ota.h"
#include "pbl_auth.h"
#include "nt_bl_system_init.h"
#include "nt_bl_rng.h"
#include "boot_log.h"
#include "nt_bl_lock.h"
#include "pbl_share.h"
#include "nt_bl_eventhandler.h"

#ifdef PBL_DEBUG_MODE

static void boot_debug_reset(char cmd);
static void boot_debug_wdg_bite(char cmd);
static void boot_debug_rng(char cmd);
static void boot_debug_load_sbl(char cmd);
static void boot_debug_load_auth_sbl(char cmd);
static void boot_debug_load_auth_app(char cmd);
static void boot_debug_ota(char cmd);
static void boot_debug_log(char cmd);

extern uint32_t _ln_SBL_stack__;

#define DEBUG_WAIT_TIME	4000
#define SBL_ELF_RRAM_ADDR	(uint32_t)(&_SBLA_IMAGE_Start_Addr)

const  boot_debug_handle dbg_handles[] =
{
	{.cmd = 'r',
	 .handler = boot_debug_reset,
	},
	{.cmd = 'w',
	 .handler = boot_debug_wdg_bite,
	},
	{.cmd = 'n',
	 .handler = boot_debug_rng,
	},
	{.cmd = 'l',
	 .handler = boot_debug_load_sbl,
	},
	{.cmd = 'f',
	 .handler = boot_debug_load_sbl,
	},
	{.cmd = 'c',
	 .handler = boot_debug_load_sbl,
	},
	{.cmd = 'j',
	 .handler = boot_debug_load_sbl,
	},
	{.cmd = 'x',
	 .handler = boot_debug_load_sbl,
	},
	{.cmd = 'a',
	 .handler = boot_debug_load_auth_sbl,
	},
    {.cmd = 's',
	 .handler = boot_debug_load_auth_app,
	},
	{.cmd = 't',
	 .handler = boot_debug_ota,
	},
	{.cmd = 'g',
	 .handler = boot_debug_log,
	}
};

static void boot_debug_reset(char cmd) {

	DEBUG_MODE_PRINTF("CMD[%c]:SW Reset\r\n", cmd);
	
	UNUSED(cmd);

	NT_SOFTWARE_RESET();
	
	return;
}

static void boot_debug_wdg_bite(char cmd)
{
	DEBUG_MODE_PRINTF("CMD[%c]:CPU hang\r\n", cmd);

	UNUSED(cmd);

	nt_wdt_int();

	// Create a CPU hang condition to trigger watchdog
	__asm volatile("cpsid i" : : : "memory");
	__asm volatile ("dmb 0xF":::"memory");

	while(1);

	return;
}

static void boot_debug_rng(char cmd)
{
	uint32_t count = 0;

	DEBUG_MODE_PRINTF("CMD[%c]: Get RNG start\r\n", cmd);

	UNUSED(cmd);

	nt_rng_init();

	nt_msdelay(10);

	for(count = 0; count < 1000; count++)
	{
		nt_get_rng();
	}

	return;
}

static void boot_debug_log(char cmd)
{
	DEBUG_MODE_PRINTF("CMD[%c]: Show boot log\r\n", cmd);

	UNUSED(cmd);

	boot_log_show();

	return;
}

static void boot_debug_load_sbl(char cmd)
{
	DEBUG_MODE_PRINTF("CMD[%c]: Load SBL ELF\r\n", cmd);

	boot_elf_loader sbl_elf_loader;
	uint32_t sbl_entry;

	pbl_force_unlock_jtag();

	boot_elf_load_init(&sbl_elf_loader,
		(uint32_t *)SBL_ELF_RRAM_ADDR, elf_mem_op, elf_mem_op_num);
	
	boot_elf_load_image(&sbl_elf_loader, BOOT_ELF_LOAD_FULL);

	DEBUG_MODE_PRINTF("Load SBL ELF Done\r\n");

	NT_DISABLE_SYSTICK_INT();

	boot_pbl_share_set();
	pbl_share.data.cmd = &cmd;

	sbl_entry = (sbl_elf_loader.elf_hdr.e_entry);

	jump_to_sbl(sbl_entry, &pbl_share);

	return;
}

#if (ENABLE_SECBOOT)
extern boot_apps_shared_data_type pbl_shared_data;
#endif

static void boot_debug_load_auth_app(char cmd)
{
	DEBUG_MODE_PRINTF("CMD[%c]: Load Auth SBL ELF\r\n", cmd);

	boot_elf_loader sbl_elf_loader;
	uint32_t sbl_entry;
#if (ENABLE_SECBOOT)	
        bl_error_type status;
	sbl_ota_s sbl_ota;
	sbl_ota_fde *sbl_fde;

	secboot_auth_image_info_s image_info;
	
	image_info.elf_loader_ptr = &sbl_elf_loader;
	status = sbl_ota_process(&sbl_ota);
	if (status) {
		DEBUG_MODE_PRINTF("OTA process err %u\r\n", (unsigned int)status);
	} else { 
		sbl_fde = &sbl_ota.sbl_fde;
	}
	if (OTA_IMG_RANK_GOLDEN == sbl_fde->content.rank){
	    image_info.image_id= SECBOOT_IMG_AUTH_SBL_GOLD_IMG;
	} else {
	    image_info.image_id= SECBOOT_IMG_AUTH_SBL_IMG;        
	}
#endif

	boot_elf_load_init(&sbl_elf_loader,
		(uint32_t *)SBL_ELF_RRAM_ADDR, elf_mem_op, elf_mem_op_num);
#if (ENABLE_SECBOOT)
	pbl_image_auth(&image_info);
#endif	
	boot_elf_load_image(&sbl_elf_loader, BOOT_ELF_LOAD_FULL);
#if (ENABLE_SECBOOT)
    pbl_compute_verify_hash();  
    pbl_populate_share_data();
#endif	
	DEBUG_MODE_PRINTF("Load Auth SBL ELF Done\r\n");

	NT_DISABLE_SYSTICK_INT();

	boot_pbl_share_set();
	pbl_share.data.cmd = &cmd;

	sbl_entry = sbl_elf_loader.elf_hdr.e_entry;

	jump_to_sbl(sbl_entry, &pbl_share);

	return;
}

static void boot_debug_load_auth_sbl(char cmd)
{
	DEBUG_MODE_PRINTF("CMD[%c]: Load Auth SBL ELF\r\n", cmd);

	boot_elf_loader sbl_elf_loader;
	uint32_t sbl_entry;
#if (ENABLE_SECBOOT)	
        bl_error_type status;
	sbl_ota_s sbl_ota;
	sbl_ota_fde *sbl_fde;

	secboot_auth_image_info_s image_info;
	
	image_info.elf_loader_ptr = &sbl_elf_loader;
	status = sbl_ota_process(&sbl_ota);
	if (status) {
		DEBUG_MODE_PRINTF("OTA process err %u\r\n", (unsigned int)status);
	} else { 
		sbl_fde = &sbl_ota.sbl_fde;
	}
	if (OTA_IMG_RANK_GOLDEN == sbl_fde->content.rank){
	    image_info.image_id= SECBOOT_IMG_AUTH_SBL_GOLD_IMG;
	} else {
	    image_info.image_id= SECBOOT_IMG_AUTH_SBL_IMG;        
	}
#endif

	boot_elf_load_init(&sbl_elf_loader,
		(uint32_t *)SBL_ELF_RRAM_ADDR, elf_mem_op, elf_mem_op_num);
#if (ENABLE_SECBOOT)	
    pbl_image_auth(&image_info);
#endif	
	boot_elf_load_image(&sbl_elf_loader, BOOT_ELF_LOAD_FULL);
#if (ENABLE_SECBOOT)	
    pbl_compute_verify_hash();                    
#endif	
	DEBUG_MODE_PRINTF("Load Auth SBL ELF Done\r\n");
	pbl_exit_configuration();

	NT_DISABLE_SYSTICK_INT();

	/*sbl_entry = (sbl_elf_entry)(sbl_elf_loader.elf_hdr.e_entry);

	__asm volatile ("MSR msp, %0" : : "r" (&_ln_SBL_stack__) : "sp");

	pbl_share.data.cmd = &cmd;

	sbl_entry(&pbl_share);*/

	boot_pbl_share_set();
	pbl_share.data.cmd = &cmd;

	sbl_entry = sbl_elf_loader.elf_hdr.e_entry;

	jump_to_sbl(sbl_entry, &pbl_share);

	return;
}

static void boot_debug_ota(char cmd)
{
	bl_error_type status;
	boot_elf_loader sbl_elf_loader;
	uint32_t sbl_entry;
	sbl_ota_s sbl_ota;
	sbl_ota_fde *sbl_fde;

	status = sbl_ota_process(&sbl_ota);

	if (status) {
		DEBUG_MODE_PRINTF("OTA process err %u\r\n", (unsigned int)status);
	} else { 
		sbl_fde = &sbl_ota.sbl_fde;
		
		if (sbl_fde->content.format == OTA_IMG_FORMAT_ELF) {
			boot_elf_load_init(&sbl_elf_loader,
				(uint32_t *)sbl_fde->content.addr, elf_mem_op, elf_mem_op_num);

			boot_elf_load_image(&sbl_elf_loader, BOOT_ELF_LOAD_FULL);

			DEBUG_MODE_PRINTF("Load SBL ELF Done\r\n");

			boot_pbl_share_set();
			pbl_share.data.cmd = &cmd;
			pbl_share.data.sbl_fde = *sbl_fde;

			sbl_entry = sbl_elf_loader.elf_hdr.e_entry;

			jump_to_sbl(sbl_entry, &pbl_share);
			
		} else {
			DEBUG_MODE_PRINTF("Invalid SBL format %u\r\n",
							(unsigned int)sbl_fde->content.format);
		}
	}

	return;
}

static void boot_debug_mode(void)
{
	char uart_cmd = 0;
	uint32_t i = 0;
	const boot_debug_handle *dbg_hdle;

	DEBUG_MODE_PRINTF("Enter Debug Mode\r\n");

	while (1) {
		if (getuartFlag()) {
			setuartFlag(0);
	
			uart_cmd = HW_REG_RD(SYS_UART_THR);
			for (i = 0; i < sizeof(dbg_handles)/sizeof(boot_debug_handle); i ++){
				dbg_hdle = &dbg_handles[i];

				if (dbg_hdle->cmd == uart_cmd && dbg_hdle->handler != NULL) {
					dbg_hdle->handler(uart_cmd);
					
					break;
				}
			}
		}
	}
}

void boot_enter_debug_mode(void)
{
	uint32_t pre_tick = 0;
	uint32_t curr_tick = 0;

   DEBUG_MODE_PRINTF("Waiting for Timeout or Press any key to enter debug\r\n");

   setuartFlag(0);

   while (1) {
		if (getuartFlag()) {
			setuartFlag(0);
			
			boot_debug_mode();
		}

		curr_tick = nt_getsystick();

		if((curr_tick - pre_tick) >= DEBUG_WAIT_TIME) {

			DEBUG_MODE_PRINTF("\tTimeout Exit....\r\n");
			
			break;
		}
   }
}

#endif
