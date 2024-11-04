/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/
#ifndef __BOOT_LOG_H__
#define __BOOT_LOG_H__
#include "neutrino.h"
#include "core_cm4.h"
#include "nt_bl_common.h"
#include "boot_module_id.h"
#include "boot_handler.h"
#include "boot_elf_loader.h"
#include "boot_error_if.h"

#define BOOT_EXCEPTION_LOG

#ifdef BOOT_EXCEPTION_LOG
typedef struct {
	exception_stack_t stack;                                                                                                
	arm_core_regs_t core_vals;
	SCB_Type	scb_vals;
} except_log;
#endif

typedef struct {
	boot_reason reason;
	uint32_t sys_status;
} boot_handler_log;

typedef struct {
	boot_elf_load_type	load_type;
} boot_elf_load_log;

typedef struct {
	uint32_t boot_start_tick;
	uint32_t boot_end_tick;
	uint32_t boot_pwr_req;
} boot_system_log;

typedef struct {
	bl_error_type err_type;
	const char * file_name;
	uint32_t	line_num;
} boot_error_log;

typedef struct {
	uint32_t			signature;
	uint32_t			pbl_version;
	boot_error_log		error;
	boot_system_log		system;
	boot_handler_log	boot_handler;
	boot_elf_load_log	elf_load;
#ifdef BOOT_EXCEPTION_LOG
	except_log			exception;
#endif
} boot_log;

typedef struct {
	boot_mod_id id;
	const char * name;
} boot_module_log;

void boot_log_init();
void boot_log_show();
void boot_log_err();

typedef void (*boot_log_init_t)(void);
typedef void (*boot_log_err_t)(bl_error_type type, const char *file, uint32_t line);

typedef struct {
	boot_log_init_t boot_log_init_pfn;
	boot_log_err_t	boot_log_err_pfn;
} boot_log_ind_t;

extern uint32_t __pbl_boot_log_start__;
extern uint32_t __pbl_boot_log_end__;
extern uint32_t _ln_pbl_boot_log_size__;
extern boot_log *pbl_boot_log;

#define BOOT_LOG_SIGNATURE		0x474F4C50
#define BOOT_LOG_SIZE			(uint32_t)(sizeof(boot_log))
#define BOOT_LOG_REGION_START	(uint32_t)(&__pbl_boot_log_start__)
#define BOOT_LOG_REGION_END		(uint32_t)(&__pbl_boot_log_end__)
#define BOOT_LOG_REGION_LAST		(BOOT_LOG_REGION_END - BOOT_LOG_SIZE)
#define BOOT_LOG_REGION_SIZE	(uint32_t)(_ln_pbl_boot_log_size__)
#define BOOT_LOG_REGION_OVERUN(addr)	(BOOT_lOG_REGION_)	

#define BOOT_LOG_SIGN_VALID(signature)	((signature == BOOT_LOG_SIGNATURE) ? 1 : 0)
#define BOOT_LOG_MOD_PTR(MOD)	(&(pbl_boot_log->MOD))

#define	BOOT_EX_MASK			0x1FF
#define	BOOT_IRQ_BASE			16
#define BOOT_EX_RESET			0
#define BOOT_EX_NONE_IRQ(num)	((num > BOOT_EX_RESET) && (num < BOOT_IRQ_BASE))
#define NUM_CORE_REGS (sizeof(arm_core_regs_t)/sizeof(uint32_t))
#define NUM_SCB_REGS (sizeof(SCB_Type)/sizeof(uint32_t))


#ifdef P_DEBUG_PRINT_LOG
#define	BOOT_LOG_SHOW()		boot_log_show();
#else
#define	BOOT_LOG_SHOW()
#endif

#endif
