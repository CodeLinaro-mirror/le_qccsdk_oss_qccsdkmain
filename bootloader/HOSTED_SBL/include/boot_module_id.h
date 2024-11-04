/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#ifndef __BOOT_MODULE_ID_H__
#define __BOOT_MODULE_ID_H__

typedef enum {
	PBL_OTA_SBL,
	PBL_FDT,
	PBL_BOOT_HANDLER,
	PBL_ELF_LOADER,
	PBL_ELF_MEM,
	PBL_SECBOOT,
	PBL_SYSTEM,
	PBL_RRAM_DXE,
	PBL_DBG_MODE,
	PBL_LOG,
	PBL_VECTOR,
	PBL_MOD_MAX,
} boot_mod_id;

#endif
