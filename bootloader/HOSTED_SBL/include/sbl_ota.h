/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#ifndef __BOOT_SBL_OTA_H__
#define __BOOT_SBL_OTA_H__
/*===========================================================================

                SBL Over The Air Update Definitions

DESCRIPTION
  This header file gives the definition of OTA.
Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
===========================================================================*/


/*===========================================================================

                      EDIT HISTORY FOR FILE

    This section contains comments describing changes made to this file.
    Notice that changes are listed in reverse chronological order.


   
when       who     what, where, why
--------   ---     ---------------------------------------------------------- 
07/28/23   Bingzhe Initial revision
===========================================================================*/


/*
 *
                RRAM Firmware Descriptor Table Definitions
 *                       ------------------------
 *
 * This section contains definitions necessary to parse an FDT in RRAM
 *
 */

#include <stdint.h>
#include "boot_error_if.h"
#include "rram_fdt.h"

#define SBL_OTA_DUMP_FDE	0x01

#define SBL_OTA_RANK_PICK_START		OTA_IMG_RANK_GOLDEN
#define SBL_OTA_RANK_PICK_END		OTA_IMG_RANK_TRIAL

enum sbl_ota_fde_state {
	SBL_OTA_FDE_NONE,
	SBL_OTA_FDE_PICK,
	SBL_OTA_FDE_UPDATE,
	SBL_OTA_FDE_RETIRE,
	SBL_OTA_FDE_SUCCESS,
};

typedef struct {
	fdt_entry	content;
	uint32_t	idx;
	uint32_t	state;
	bl_error_type err_type;
} sbl_ota_fde;

typedef struct {
	fdt_entry	content;
	uint32_t	idx;
	uint32_t	state;
} sbl_ota_fde_es;

typedef struct {
	fdt_entry	context;
	uint32_t	idx;
} pbl_patch_fde;

typedef struct {
	fdt_entry	content;
	bl_error_type err_type;
} sbl_prev_fde;

typedef struct {
	sbl_prev_fde	prev_fde;
	uint32_t	next_fde_idx;
} sbl_ota_context;

typedef struct {
	fdt_s			*fdt;
	sbl_ota_fde		sbl_fde;
	pbl_patch_fde	patch_fde;
	sbl_ota_context	sbl_context;
} sbl_ota_s;

extern sbl_ota_s sbl_ota;

bl_error_type sbl_ota_get_fdt(sbl_ota_s *sbl_ota);
bl_error_type sbl_ota_get_patch(sbl_ota_s *sbl_ota);
bl_error_type sbl_ota_update_patch(sbl_ota_s *sbl_ota);
bl_error_type sbl_ota_pick_fde(sbl_ota_s *sbl_ota);
bl_error_type sbl_ota_handle_trial_fde(sbl_ota_fde *trial_fde);
bl_error_type sbl_ota_handle_curr_fde(sbl_ota_fde *curr_fde);
bl_error_type sbl_ota_handle_fde(sbl_ota_s *sbl_ota);
bl_error_type sbl_ota_age_curr_fde(sbl_ota_s *sbl_ota);
bl_error_type sbl_ota_update_fde(sbl_ota_s *sbl_ota);
bl_error_type sbl_ota_exit_context(sbl_ota_s *sbl_ota);
bl_error_type sbl_ota_enter_context(sbl_ota_s *sbl_ota);
bl_error_type sbl_ota_process(sbl_ota_s *sbl_ota);

typedef bl_error_type (*sbl_ota_get_fdt_t)(sbl_ota_s *sbl_ota);
typedef bl_error_type (*sbl_ota_get_patch_t)(sbl_ota_s *sbl_ota);
typedef bl_error_type (*sbl_ota_update_patch_t)(sbl_ota_s *sbl_ota);
typedef bl_error_type (*sbl_ota_pick_fde_t)(sbl_ota_s *sbl_ota);
typedef bl_error_type (*sbl_ota_handle_trial_fde_t)(sbl_ota_fde *trial_fde);
typedef bl_error_type (*sbl_ota_handle_curr_fde_t)(sbl_ota_fde *curr_fde);
typedef bl_error_type (*sbl_ota_handle_fde_t)(sbl_ota_s *sbl_ota);
typedef bl_error_type (*sbl_ota_age_curr_fde_t)(sbl_ota_s *sbl_ota);
typedef bl_error_type (*sbl_ota_update_fde_t)(sbl_ota_s *sbl_ota);
typedef bl_error_type (*sbl_ota_exit_context_t)(sbl_ota_s *sbl_ota);
typedef bl_error_type (*sbl_ota_enter_context_t)(sbl_ota_s *sbl_ota);
typedef bl_error_type (*sbl_ota_process_t)(sbl_ota_s *sbl_ota);

typedef struct {
	sbl_ota_get_fdt_t			sbl_ota_get_fdt_pfn;
	sbl_ota_get_patch_t			sbl_ota_get_patch_pfn;
	sbl_ota_update_patch_t		sbl_ota_update_patch_pfn;
	sbl_ota_pick_fde_t			sbl_ota_pick_fde_pfn;
	sbl_ota_handle_trial_fde_t	sbl_ota_handle_trial_fde_pfn;
	sbl_ota_handle_curr_fde_t	sbl_ota_handle_curr_fde_pfn;
	sbl_ota_handle_fde_t		sbl_ota_handle_fde_pfn;
	sbl_ota_age_curr_fde_t		sbl_ota_age_curr_fde_pfn;
	sbl_ota_update_fde_t		sbl_ota_update_fde_pfn;
	sbl_ota_exit_context_t		sbl_ota_exit_context_pfn;
	sbl_ota_enter_context_t		sbl_ota_enter_context_pfn;
	sbl_ota_process_t			sbl_ota_process_pfn;
} sbl_ota_ind_t;
#endif
