/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/
#ifndef _BOOT_PBL_SHARE_H_
#define _BOOT_PBL_SHARE_H_
#include "boot_handler.h"
#include "sbl_ota.h"
#include "boot_log.h"
#include "boot_elf_loader.h"

typedef struct {
	char			*cmd;
	uint32_t		pbl_version;
	boot_reason		bt_reason;
	sbl_ota_fde		sbl_fde;
	sbl_prev_fde	prev_fde;
	boot_log		*pbl_log;
	void *secboot_data;
} boot_pbl_share_data;

typedef struct {
	char			*cmd;
	uint32_t		pbl_version;
	boot_reason		bt_reason;
	sbl_ota_fde_es	sbl_fde;
	boot_log		*pbl_log;
	void *secboot_data;
} boot_pbl_share_data_es;

typedef struct {
	/* ELF load share function */
	boot_elf_load_init_t			boot_elf_load_init;
	boot_elf_load_image_t			boot_elf_load_image;
	/* FDT share function */  
	fdt_get_t						fdt_get;
	fde_update_t					fde_update;
	fde_get_idx_by_id_rank_t		fde_get_idx_by_id_rank;
	fde_get_by_idx_t				fde_get_by_idx;
} boot_pbl_share_func;

typedef struct {
	boot_pbl_share_data		data;
	boot_pbl_share_func		func;
} boot_pbl_share;

typedef struct {
	boot_pbl_share_data_es	data;
	boot_pbl_share_func		func;
} boot_pbl_share_es;

extern boot_pbl_share	pbl_share;

extern void boot_pbl_share_set();

typedef void (*boot_pbl_share_set_t)(void);

typedef struct {
	boot_pbl_share_set_t	boot_pbl_share_set_pfn;
} pbl_share_ind_t;

#endif
