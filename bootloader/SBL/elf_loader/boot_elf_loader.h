/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/
#ifndef __BOOT_ELF_LOADER_H__
#define __BOOT_ELF_LOADER_H__

#include "elf_header.h"
#include "miprogressive.h"
#include "boot_error_if.h"
#include "boot_elf_mem.h"

#define BOOT_ELF_DUMP_ELF_HDR	0x01
#define BOOT_ELF_DUMP_PRG_HDR	0x02
#define BOOT_ELF_DUMP_ALL	BOOT_ELF_DUMP_ELF_HDR | BOOT_ELF_DUMP_PRG_HDR

typedef enum {
	BOOT_ELF_LOAD_FULL,
	BOOT_ELF_LOAD_RAM,
	BOOT_ELF_LOAD_COPY,

	BOOT_ELF_LOAD_MAX,
} boot_elf_load_type;

typedef struct {
	uint32_t *elf_addr;
	boot_elf32_ehdr elf_hdr;
	boot_elf32_phdr prog_hdrs[MI_PBT_MAX_SEGMENTS];
	uint32_t phr_num;
	bl_elf_mem_ops mem_ops;
} boot_elf_loader;

typedef void (*sbl_elf_entry)(void* arg);
typedef void (*app_elf_entry)(void);

extern boot_elf_loader elf_loader;

//bl_error_type boot_elf_load_init(boot_elf_loader * elf_loader, uint32_t * elf_addr);
bl_error_type boot_elf_load_init(boot_elf_loader * elf_loader, uint32_t * elf_addr, bl_elf_mem_op *op_cfg, uint32_t op_cfg_num);
bl_error_type boot_elf_validate_elf_header(const boot_elf32_ehdr * elf_hdr);
bl_error_type boot_elf_load_image(boot_elf_loader *elf_loader, boot_elf_load_type type);
bl_error_type boot_elf_load_elf_header(boot_elf_loader *elf_loader);
bl_error_type boot_elf_load_prog_hdrs(boot_elf_loader *elf_loader);
bl_error_type boot_elf_load_generic_segment(boot_elf_loader *elf_loader, uint32_t prog_idx,
								uint32_t offset, uint32_t dest_addr, uint32_t bytes_to_load);

//typedef bl_error_type (*boot_elf_load_init_t)(boot_elf_loader * elf_loader, uint32_t * elf_addr);
typedef bl_error_type (*boot_elf_load_init_t)(boot_elf_loader * elf_loader, uint32_t * elf_addr, bl_elf_mem_op *op_cfg, uint32_t op_cfg_num);
typedef bl_error_type (*boot_elf_validate_elf_header_t)(const boot_elf32_ehdr * elf_hdr);
typedef bl_error_type (*boot_elf_load_image_t)(boot_elf_loader *elf_loader, boot_elf_load_type type);
typedef bl_error_type (*boot_elf_load_elf_header_t)(boot_elf_loader *elf_loader);
typedef bl_error_type (*boot_elf_load_prog_hdrs_t)(boot_elf_loader *elf_loader);
typedef bl_error_type (*boot_elf_load_generic_segment_t)(boot_elf_loader *elf_loader, uint32_t prog_idx,
								uint32_t offset, uint32_t dest_addr, uint32_t bytes_to_load);

typedef struct {
	boot_elf_load_init_t			boot_elf_load_init_pfn;
	boot_elf_load_image_t			boot_elf_load_image_pfn;
	boot_elf_load_elf_header_t		boot_elf_load_elf_header_pfn;
	boot_elf_load_prog_hdrs_t		boot_elf_load_prog_hdrs_pfn;
	boot_elf_validate_elf_header_t	boot_elf_validate_elf_header_pfn;
	boot_elf_load_generic_segment_t boot_elf_load_generic_segment_pfn;
} boot_elf_loader_ind_t;

#endif
