/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/
/*===========================================================================

                              RRAM FDT

GENERAL DESCRIPTION
  This module performs RRAM FDT parse and update.

===========================================================================*/

/*===========================================================================
                           EDIT HISTORY FOR FILE
  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.

when       who        what, where, why
--------   ---        -------------------------------------------------------
07/28/23   bingzhe         Initial revision
===========================================================================*/

/*===========================================================================
                     INCLUDE FILES FOR MODULE
===========================================================================*/

#include <stdio.h>
#include <string.h>
#include "rram_fdt.h"
#include "boot_print.h"
#include "pbl_patch_table.h"

#ifdef P_DEBUG
#define FDT_DUMP_DBG
#endif

#ifdef FDT_DUMP_DBG
void fdt_dump(fdt_s *fdt, uint32_t dump_cfg);
#define FDT_DUMP(fdt, cfg)	fdt_dump(fdt, cfg);
#else
#define FDT_DUMP(fdt, cfg)
#endif

fdt_s *fdt_addr = (fdt_s *)(&_FDT_Start_Addr);

bl_error_type fdt_verify(fdt_s *fdt)
{	
	if (fdt == NULL) {
		FDT_PRINTF("FDT verify NULL point of fdt\r\n");
		return BL_ERR_FDT_NULL_PTR;
	}

	if (fdt->header.signature != FDT_SIGNATURE) {
		FDT_PRINTF("FDT verify SIGNATURE failure\r\n");
		return BL_ERR_FDT_INVALID_SIGNATURE;
	}

	if (fdt->header.num_fde == 0 || fdt->header.num_fde > FDT_MAX_ENTRY) {
		FDT_PRINTF("FDT verify SIGNATURE failure\r\n");
		return BL_ERR_FDT_INVALID_ENTRY_NUM;
	}

	return BL_ERR_NONE;
}

bl_error_type fde_verify(fdt_entry *fde)
{
	if (fde == NULL) {
		FDT_PRINTF("FDE verify NULL point of FDE\r\n");
		return BL_ERR_FDT_NULL_PTR;
	}

	if (fde->id > OTA_IMG_ID_APP) {
		FDT_PRINTF("FDE verify invalid image ID of FDE\r\n");
		return BL_ERR_FDE_INVALID_IMAGE_ID;
	}
	
	if ((fde->id == OTA_IMG_ID_SBL) &&
			(fde->addr < SBL_IMAGE_REGION_START
		 || fde->addr > SBL_IMAGE_REGION_END)) {
		FDT_PRINTF("FDE verify invalid image ADDR of FDE\r\n");
		return BL_ERR_FDE_INVALID_IMAGE_ADDR;
	}

	return BL_ERR_NONE;
}

bl_error_type fdt_get(fdt_s **fdt)
{
	bl_error_type status = BL_ERR_NONE;
	
	*fdt = PBL_GLB_PTR(fdt_addr);
	status = PBL_IND_MOD_PFN(rram_fdt, fdt_verify)(*fdt);

	if (status) {
	   FDT_DUMP(*fdt, FDT_DUMP_HDR)
	} else {
	   FDT_DUMP(*fdt, FDT_DUMP_ALL)
	}

	return status; 
}

bl_error_type fde_get_idx_by_id(fdt_s *fdt, uint32_t id, uint32_t *idx)
{
	bl_error_type status = BL_ERR_NONE;
	uint32_t i;
	fdt_entry * fde;

	if (fdt == NULL) {
		FDT_PRINTF("FDE get idx with invalid parameters\r\n");
		return  BL_ERR_FDT_INVALID_PARAM;
	}

	fde = fdt->fde;
	*idx = FDE_INVAL_IDX;

	for (i = FDE_START_IDX; i < fdt->header.num_fde; i ++) {
		if ((fde[i].id == id)) {
			*idx = i;
			break;
		}
	}

	return status;
}

bl_error_type fde_get_idx_by_id_rank(fdt_s *fdt, uint32_t id, uint32_t rank, uint32_t *idx)
{
	bl_error_type status = BL_ERR_NONE;
	uint32_t i;
	fdt_entry * fde;

	if (fdt == NULL) {
		FDT_PRINTF("FDE get idx with invalid parameters\r\n");
		return  BL_ERR_FDT_INVALID_PARAM;
	}

	fde = fdt->fde;
	*idx = FDE_INVAL_IDX;

	for (i = FDE_START_IDX; i < fdt->header.num_fde; i ++) {
		if ((fde[i].id == id) && (fde[i].rank == rank)) {
			*idx = i;
			break;
		}
	}

	return status;
}

bl_error_type fde_get_by_idx(fdt_s *fdt, fdt_entry *fde, uint32_t idx)
{
	bl_error_type status = BL_ERR_NONE;

	if (fde == NULL || fdt == NULL || idx > FDT_MAX_ENTRY) {
		FDT_PRINTF("FDE get by idx with invalid parameters\r\n");
		return  BL_ERR_FDT_INVALID_PARAM;
	}

	if (idx >= fdt->header.num_fde) {
		FDT_PRINTF("FDE get by idx with invalid idx %u\r\n", (unsigned int)(idx));
		return BL_ERR_FDE_INVALID_IDX;
	}

	*fde = fdt->fde[idx];

	status = PBL_IND_MOD_PFN(rram_fdt, fde_verify)(fde);

	return status;
}

bl_error_type fde_update(fdt_s *fdt, fdt_entry *fde, uint32_t idx)
{
	bl_error_type status = BL_ERR_NONE;

	if (fde == NULL || fdt == NULL || idx > FDT_MAX_ENTRY) {
		FDT_PRINTF("FDE update with invalid parameters\r\n");
		return  BL_ERR_FDT_INVALID_PARAM;
	}

	if (idx >= fdt->header.num_fde) {
		FDT_PRINTF("FDE update with invalid idx %u\r\n", (unsigned int)(idx));
		return BL_ERR_FDE_INVALID_IDX;
	}

	status = PBL_IND_MOD_PFN(nt_bl_rram_dxe, rram_write_dxe)((uint32_t)(&fdt->fde[idx]), (uint8_t *)fde, sizeof(fdt_entry));

	if (status) {
		FDT_PRINTF("FDE update failed %d\r\n", status);
		return BL_ERR_FDE_UPDATE_ERR;
	}

	return status;
}

#ifdef FDT_DUMP_DBG
void fdt_dump(fdt_s *fdt, uint32_t dump_cfg)
{
	uint32_t i;

	if (fdt == NULL) {
		FDT_PRINTF("FDT DUMP with NULL point of fdt\r\n");
		return;
	}

	if (dump_cfg & FDT_DUMP_HDR) {
		FDT_PRINTF("Dump FDT header:\r\n");
		FDT_PRINTF("Signature 0x%lx\r\n", fdt->header.signature);
		FDT_PRINTF("Num_fde %lu\r\n", fdt->header.num_fde);
	}

	if (dump_cfg & FDT_DUMP_FDE) {
		FDT_PRINTF("Dump FDT entry:\r\n");
		for (i = 0; i < fdt->header.num_fde; i ++) {
			FDT_PRINTF("FDE[%lu]:\r\n", i);
			FDT_PRINTF("       id %lu\r\n", fdt->fde[i].id);
			FDT_PRINTF("       rank %lu\r\n", fdt->fde[i].rank);
			FDT_PRINTF("       format %lu\r\n", fdt->fde[i].format);
			FDT_PRINTF("       state %lu\r\n", fdt->fde[i].state);
			FDT_PRINTF("       version %lu\r\n", fdt->fde[i].version);
			FDT_PRINTF("       addr 0x%lx\r\n", fdt->fde[i].addr);
		}
	}
	
	return;
}
#endif
