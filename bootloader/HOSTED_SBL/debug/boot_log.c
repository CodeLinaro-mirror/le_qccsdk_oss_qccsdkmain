/*
* * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
* * SPDX-License-Identifier: BSD-3-Clause-Clear
*/
#include <string.h>
#include "boot_print.h"
#include "pbl_patch_table.h"

boot_log *pbl_boot_log = (boot_log *)BOOT_LOG_REGION_START;
const binary_desriptor_t nt_pbl_descriptor_t={0};

void boot_log_init()
{
	uint32_t log_curr = 0;
	pbl_boot_log = PBL_GLB_PTR(pbl_boot_log);

	while (BOOT_LOG_SIGN_VALID(pbl_boot_log->signature)) {
		log_curr ++;

		if (((uint32_t)pbl_boot_log + sizeof(boot_log))
								< (BOOT_LOG_REGION_LAST))
			pbl_boot_log ++;
		else 
			break;
	}

	memset(pbl_boot_log, 0x00, sizeof(boot_log));

	pbl_boot_log->signature = BOOT_LOG_SIGNATURE;
	pbl_boot_log->pbl_version = PBL_GLB_DATA_PTR(nt_pbl_descriptor_t)->version; 

	return;
}

void boot_log_err(bl_error_type type, const char *file, uint32_t line)
{
	boot_error_log *err_log;

	err_log = &pbl_boot_log->error;

	err_log->err_type = type;
	err_log->file_name = file;
	err_log->line_num = line;
}

#ifdef P_DEBUG_PRINT_LOG
#ifdef BOOT_EXCEPTION_LOG
void boot_log_show_exception(boot_log *log_ptr)
{
	uint32_t i = 0;
	uint32_t *regs;

	if (log_ptr == NULL)
		return;

	if (!BOOT_EX_NONE_IRQ((log_ptr->exception.scb_vals.ICSR & BOOT_EX_MASK)))
		return;
		
	static const char *core_names[] = {"R0", "R1", "R2", "R3", "R4", "R5", "R6", "R7",
									 "R8", "R9", "R10", "R11", "R12", "SP", "LR", "PC"};

	static const char *scb_names[] = {"CPUID", "ICSR", "VTOR", "AIRCR", "SCR",
									"CCR", "SHPR1", "SHPR2", "SHPR3", "SHCSR",
									"CFSR", "HFSR", "DFSR", "MMFAR", "BFAR",
									"AFSR","ID_PFR0", "ID_PFR1", "ID_DFR0", "ID_AFR0",
									"ID_MMFR0", "ID_MMFR1", "ID_MMFR2", "ID_MMFR3", "ID_ISAR0",
									"ID_ISAR1", "ID_ISAR2","ID_ISAR3", "ID_ISAR4", "ID_ISAR5",
									"CLIDR", "CTR", "CCSIDR", "CSSELR", "CPACR"};

	LOG_PRINTF("Boot exception:\r\n");

	regs = (uint32_t *)&log_ptr->exception.core_vals;

	for (i = 0; i < NUM_CORE_REGS; i++) {
		LOG_PRINTF("	%s:0x%08lX\r\n", (unsigned int)core_names[i], regs[i]);
	}

	LOG_PRINTF("	xPSR:0x%08lX\r\n", log_ptr->exception.stack.xpsr);

	regs = (uint32_t *)&log_ptr->exception.scb_vals;

	for (i = 0; i < NUM_SCB_REGS; i++)
	{
		LOG_PRINTF("	%s:0x%08lX\r\n", (unsigned int)scb_names[i], regs[i]);
	}
	
	return;
}
#endif

void boot_log_show_err(boot_log *log_ptr) {
	if (log_ptr == NULL)
		return;

	LOG_PRINTF("Boot error:\r\n");
	LOG_PRINTF("	type 0x%08x\r\n", log_ptr->error.err_type);
	LOG_PRINTF("	file %s\r\n", (unsigned int)log_ptr->error.file_name);
	LOG_PRINTF("	line %lu\r\n", log_ptr->error.line_num);
	
	return;
}

void boot_log_show_system(boot_log *log_ptr) {
	if (log_ptr == NULL)
		return;

	LOG_PRINTF("Boot system:\r\n");
	LOG_PRINTF("	start_tick	%lu\r\n", log_ptr->system.boot_start_tick);
	LOG_PRINTF("	end_tick	%lu\r\n", log_ptr->system.boot_end_tick);
	LOG_PRINTF("	pwr_req 0x%08x\r\n", log_ptr->system.boot_pwr_req);
	
	return;
}

void boot_log_show_handler(boot_log *log_ptr) {
	if (log_ptr == NULL)
		return;

	LOG_PRINTF("Boot handler:\r\n");
	LOG_PRINTF("	Type %u\r\n",	log_ptr->boot_handler.reason.type);
	LOG_PRINTF("	Subtype %u\r\n", log_ptr->boot_handler.reason.subtype);
	LOG_PRINTF("	sys_status 0x%08x\r\n", log_ptr->boot_handler.sys_status);
	
	return;
}

void boot_log_show_elf_load(boot_log *log_ptr) {
	if (log_ptr == NULL)
		return;

	LOG_PRINTF("Boot elf load:\r\n");
	LOG_PRINTF("	load type %u\r\n",	log_ptr->elf_load.load_type);
	
	return;
}

void boot_log_show()
{
	uint32_t log_curr = 0;
	boot_log *log_ptr = PBL_GLB_PTR(pbl_boot_log);

	while (BOOT_LOG_SIGN_VALID(log_ptr->signature)) {
		LOG_PRINTF("BOOT LOG[%lu]:\r\n", log_curr);
		LOG_PRINTF("	PBL version 0x%08x\r\n", log_ptr->pbl_version);

		boot_log_show_err(log_ptr);
		boot_log_show_system(log_ptr);
		boot_log_show_handler(log_ptr);
		boot_log_show_elf_load(log_ptr);
#ifdef BOOT_EXCEPTION_LOG
		boot_log_show_exception(log_ptr);
#endif
		log_ptr ++;
		log_curr ++;
	}
	
	return;
}

#endif

