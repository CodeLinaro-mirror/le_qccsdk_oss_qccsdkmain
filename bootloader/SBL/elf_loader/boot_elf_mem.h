/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/
#ifndef __BOOT_ELF_MEM_H__
#define __BOOT_ELF_MEM_H__

#include <stdint.h>
#include "boot_error_if.h"

#define FRN_RAM_START	0x0
#define	FRN_RAM_END		0x9FFFF

#define FRN_RRAM_START	0x200000
#define FRN_RRAM_END	0x37FFFF
#ifdef SBL_BUILD
//Virtual add for Flash. TODO: read from partition table
#define APP_IMAGE_FLASH_ADDRESS	0xA00000
#define FRN_FLASH_START	(0xA00000-0x1)
#define	FRN_FLASH_END	(0xA00000+0x400000)
#endif

#define BOOT_ELF_MEM_DUMP_OP	0x01
#define BOOT_ELF_MEM_DUMP_ALL	BOOT_ELF_MEM_DUMP_OP

#define RRAM_READ_BUF_SIZE  2048

typedef enum {
	BL_RRAM_RAM = 0, /*RRAM -> RAM*/
	BL_RRAM_RRAM = 1, /*RRAM-> RRAM*/
	BL_FLASH_RAM = 2, /*Flash -> RAM*/
	BL_FLASH_RRAM = 3, /*Flash -> RRAM*/
	BL_MEM_MAX_NUM = 4,
} bl_elf_mem_type;

typedef bl_error_type (*bl_elf_mem_read)(void *address, void *data, uint32_t byte);
typedef bl_error_type (*bl_elf_mem_set)(void *address, int32_t value, uint32_t byte);

typedef struct {
	uint32_t v_start;
	uint32_t v_end;
	uint32_t p_start;
	uint32_t p_end;
} bl_elf_mem_region;

typedef struct {
	bl_elf_mem_type type;
	bl_elf_mem_region region;
	bl_elf_mem_read read;
	bl_elf_mem_set	set;	
} bl_elf_mem_op;

typedef struct {
	bl_elf_mem_op op[BL_MEM_MAX_NUM];
	uint32_t num;
} bl_elf_mem_ops;

#define BL_ELF_MEM_READ_REGION_CHECK(src, dst, bytes, p_start, p_end, v_start, v_end)  \
							(((src >= p_start) && ((src + bytes)<= p_end)) &&	\
								((dst >= v_start) && ((dst + bytes) <= v_end)))
#define BL_ELF_MEM_SET_REGION_CHECK(dst, bytes, v_start, v_end)	\
							((dst >= v_start) && ((dst + bytes) <= v_end))

extern bl_elf_mem_op elf_mem_op[];
extern uint32_t elf_mem_op_num;

bl_error_type bl_elf_flash_read_to_ram(void *address, void *data, uint32_t bytes);
bl_error_type bl_elf_flash_read_to_rram_dxe(void *address, void *data, uint32_t bytes);

bl_error_type bl_elf_rram_set_rram_dxe(void *address, int32_t value, uint32_t bytes);
bl_error_type bl_elf_rram_read_to_rram_dxe(void *address, void *data, uint32_t bytes);
bl_error_type bl_elf_rram_set_ram(void *address, int32_t value , uint32_t bytes);
bl_error_type bl_elf_rram_read_to_ram(void *address, void *data, uint32_t bytes);

bl_error_type boot_elf_mem_read(void *address, void *data, uint32_t bytes, bl_elf_mem_ops *mem_ops);
bl_error_type boot_elf_mem_set(void *address, int32_t value, uint32_t bytes, bl_elf_mem_ops *mem_ops);
bl_error_type boot_elf_mem_op_register(bl_elf_mem_ops *mem_ops, bl_elf_mem_op mem_op);
bl_error_type boot_elf_mem_op_init(bl_elf_mem_ops *mem_ops, bl_elf_mem_op *op_cfg, uint32_t op_cfg_num);

typedef bl_error_type (*bl_elf_rram_set_rram_dxe_t)(void *address, int32_t value, uint32_t bytes);
typedef bl_error_type (*bl_elf_rram_read_to_rram_dxe_t)(void *address, void *data, uint32_t bytes);
typedef bl_error_type (*bl_elf_rram_set_ram_t)(void *address, int32_t value , uint32_t bytes);
typedef bl_error_type (*bl_elf_rram_read_to_ram_t)(void *address, void *data, uint32_t bytes);

typedef bl_error_type (*boot_elf_mem_read_t)(void *address, void *data, uint32_t bytes, bl_elf_mem_ops *mem_ops);
typedef bl_error_type (*boot_elf_mem_set_t)(void *address, int32_t value, uint32_t bytes, bl_elf_mem_ops *mem_ops);
typedef bl_error_type (*boot_elf_mem_op_register_t)(bl_elf_mem_ops *mem_ops, bl_elf_mem_op mem_op);
typedef bl_error_type (*boot_elf_mem_op_init_t)(bl_elf_mem_ops *mem_ops, bl_elf_mem_op *op_cfg, uint32_t op_cfg_num);

typedef struct {
	bl_elf_rram_set_rram_dxe_t		bl_elf_rram_set_rram_dxe_pfn;
	bl_elf_rram_read_to_rram_dxe_t	bl_elf_rram_read_to_rram_dxe_pfn;
	bl_elf_rram_set_ram_t			bl_elf_rram_set_ram_pfn;
	bl_elf_rram_read_to_ram_t		bl_elf_rram_read_to_ram_pfn;
	boot_elf_mem_read_t				boot_elf_mem_read_pfn;
	boot_elf_mem_set_t				boot_elf_mem_set_pfn;
	boot_elf_mem_op_register_t		boot_elf_mem_op_register_pfn;
	boot_elf_mem_op_init_t			boot_elf_mem_op_init_pfn;
} boot_elf_mem_ind_t;

#endif
