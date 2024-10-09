/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/
#ifndef _BOOT_PBL_PATCH_TABLE_H_
#define _BOOT_PBL_PATCH_TABLE_H_
#include "nt_bl_eventhandler.h"
#include "nt_bl_system_init.h"
#include "nt_bl_mem.h"
#include "nt_bl_rram_dxe.h"
#include "nt_bl_lock.h"
//#include "boot_elf_mem.h"
#include "boot_elf_loader.h"
#include "boot_handler.h"
#include "boot_log.h"
#include "rram_fdt.h"
#include "sbl_ota.h"
#include "nt_bl_vector_table.h"
#include "pbl_share.h"
//#include "pbl_main.h"
#include "binary_descriptor.h"
#include "pbl_image_auth.h"
#include "pbl_auth_indt.h"
#include "pbl_mpu.h"

#ifndef PBL_PATCH_ENABLE
#ifndef SBL_BUILD 
#define PBL_PATCH_ENABLE
#endif
#endif

bl_error_type pbl_patch_invalid(void);
typedef bl_error_type (*pbl_patch_invalid_t)(void);

typedef struct {
	pbl_patch_invalid_t	pbl_patch_invalid_pfn;
} pbl_patch_ind_t;

typedef void (*boot_processing_t)(void);

typedef struct {
    boot_processing_t boot_processing_pfn;
} pbl_main_ind_t;

typedef struct {
	binary_desriptor_t			*nt_pbl_descriptor_t_ptr;
	boot_elf_loader				*elf_loader_ptr;
	boot_pbl_share				*pbl_share_ptr;
	boot_handler				*pbl_boot_handler_ptr; 
	sbl_ota_s					*sbl_ota_ptr;
	uint32_t					*elf_mem_op_num_ptr;
	uint32_t					*subtype_handlers_num_ptr;
	fdt_s						*fdt_addr;
	bl_elf_mem_op				*elf_mem_op;
	boot_subtype_handler		*subtype_handlers; 
	boot_log					*pbl_boot_log;
} glb_data_t;

/* Function indirection table strucutre. This holds indirection table from
 * each module.
 */
typedef struct {
	boot_log_ind_t				boot_log;
    evt_hdlr_ind_t				evt_hdlr;
	pbl_share_ind_t				pbl_share;
	pbl_main_ind_t				pbl_main;
	nt_bl_system_ind_t			nt_bl_system;
	nt_bl_mem_ind_t				nt_bl_mem;
	nt_bl_lock_ind_t			nt_bl_lock;
	nt_bl_rram_dxe_ind_t		nt_bl_rram_dxe;
	boot_elf_mem_ind_t			boot_elf_mem;
	rram_fdt_ind_t				rram_fdt;
	sbl_ota_ind_t				sbl_ota;
	boot_elf_loader_ind_t		boot_elf_loader;
	boot_handler_ind_t			boot_handler;
    irq_handler_ext_ind_t		irq_handler_ext;
	pbl_patch_ind_t				pbl_patch;
	pbl_image_auth_ind_t        pbl_image_auth;
	pbl_secboot_ind_p           pbl_secboot;	
	pbl_mpu_ind_t           	pbl_mpu;
} func_ind_t;

typedef void (*pbl_patch_glb_data_t)(void*);
typedef void (*pbl_patch_func_ind_t)(void*);

/* Structure to hold address of patcher functions */
typedef struct {
    pbl_patch_glb_data_t pbl_patch_glb_data_pfn;
    pbl_patch_func_ind_t pbl_patch_func_ind_pfn;
}pbl_patch_func_t;

extern func_ind_t *ind_table_ptr;
extern glb_data_t *g_data_ptr;

extern uint32_t _ln_RF_start_addr_ind__;    //Indirection table load address RRAM
extern uint32_t _ln_RAM_start_addr_ind__;   //Indirection table start address RAM
extern uint32_t _ln_pbl_ind_size__;         //Indirection table size

extern uint32_t _ln_RF_start_addr_glb_data__;   //Global data table load address RRAM
extern uint32_t _ln_RAM_start_addr_glb_data__;  //Global data table start address RAM
extern uint32_t _ln_pbl_glb_data_size__;        //Global data table size

extern uint32_t __sec_patch_start;  //pbl patcher functions start address

/* Start address of global data pointers table */
extern uint32_t __glb_data_table_start;
/* Start address of PBL logs */
extern uint32_t __PBL_log_start_addr__;
/* Start address of indirection table in RAM */
extern uint32_t __ind_table_start;

#ifdef PBL_PATCH_ENABLE
#define PBL_IND_MOD_T(mod)					(&(ind_table_ptr->mod))		
#define PBL_IND_MOD_PFN(mod, func)			((mod##_ind_t *)(PBL_IND_MOD_T(mod)))->func##_pfn
#define PBL_GLB_DATA_PTR(data)				(g_data_ptr->data##_ptr)		
#define PBL_GLB_DATA_PTR_DEREF(data)		(*(g_data_ptr->data##_ptr))		
#define PBL_GLB_PTR(ptr)					(g_data_ptr->ptr)
#else
#define PBL_IND_MOD_PFN(mod, func)			(func)
#define PBL_GLB_DATA_PTR(data)				(&data)	
#define PBL_GLB_DATA_PTR_DEREF(data)		(data)		
#define PBL_GLB_PTR(ptr)					(ptr)
#endif

#define PBL_PATCH_MAGIC_NUM 0xC0C0CAFE
#define PBL_PATCH_START_ADDR 0x208400
#define PBL_PATCH_CHECK_SIZE 16

void pbl_patch_init();
bl_error_type pbl_patch_invalid();

#endif
