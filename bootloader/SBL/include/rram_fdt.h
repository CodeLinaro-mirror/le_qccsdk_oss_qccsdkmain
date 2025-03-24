/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#ifndef __BOOT_RRAM_FDT_H__
#define __BOOT_RRAM_FDT_H__
/*===========================================================================

                RRAM Firmware Descriptor Table  Definitions

DESCRIPTION
  This header file gives the definition of the structures in RRAM FDT.

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

extern uint32_t _FDT_Start_Addr;
extern uint32_t _SBLA_IMAGE_Start_Addr;
#define FDT_RRAM_ADDR	(uint32_t)(&_FDT_Start_Addr)
#define FDT_SIGNATURE	0x54445746
#define FDT_SIZE	1024
#define FDE_SIZE	32
#define FDE_START_IDX	0
#define FDT_MAX_ENTRY ((FDT_SIZE/FDE_SIZE) - 1)
#define FDE_INVAL_IDX (FDT_SIZE/FDE_SIZE)

#define FDT_DUMP_HDR 0x1
#define FDT_DUMP_FDE 0x2
#define FDT_DUMP_ALL (FDT_DUMP_HDR | FDT_DUMP_FDE)

#define SBL_IMAGE_REGION_START (uint32_t)(&_SBLA_IMAGE_Start_Addr)

#define SBL_IMAGE_REGION_SIZE 0x10000
#define SBL_IMAGE_REGION_END  (SBL_IMAGE_REGION_START + SBL_IMAGE_REGION_SIZE)

enum ota_image_state {
	OTA_IMG_STATE_INVALID,
	OTA_IMG_STATE_VALID,
	OTA_IMG_STATE_ABORTED,
	OTA_IMG_STATE_NEW,
	OTA_IMG_STATE_VERIFY_PENDING,
};

enum ota_image_rank {
	OTA_IMG_RANK_AGED,
	OTA_IMG_RANK_GOLDEN,
	OTA_IMG_RANK_CURRENT,
	OTA_IMG_RANK_TRIAL,
};

enum ota_image_format {
	OTA_IMG_FORMAT_ELF,
	OTA_IMG_FORMAT_BIN,
};

enum ota_image_id {
	OTA_IMG_ID_SBL,
	OTA_IMG_ID_PBL_PATCH,
	OTA_IMG_ID_APP,
	OTA_IMG_ID_BDF,
	OTA_IMG_ID_UD,
	OTA_IMG_ID_RAMDUMP,
};

enum ota_patch_state {
	OTA_PATCH_STATE_NONE,
	OTA_PATCH_STATE_PATCHED,
};

typedef struct {
	uint32_t signature;
	uint32_t num_fde;
	uint8_t reserve[24];
} fdt_hdr;

typedef struct {
	uint32_t id;
	uint32_t rank;
	uint32_t format;
	uint32_t addr;
	uint32_t state;
	uint32_t version;
	uint8_t reserve[8];
} fdt_entry;

typedef struct {
	fdt_hdr header;
	fdt_entry fde[0];
} fdt_s;

bl_error_type fdt_get(fdt_s **fdt);
bl_error_type fdt_verify(fdt_s *fdt);
bl_error_type fde_verify(fdt_entry *fde);
bl_error_type fde_update(fdt_s *fdt, fdt_entry *fde, uint32_t idx);
bl_error_type fde_get_by_idx(fdt_s *fdt, fdt_entry *fde, uint32_t idx);
bl_error_type fde_get_idx_by_id(fdt_s *fdt, uint32_t id, uint32_t *idx);
bl_error_type fde_get_idx_by_id_rank(fdt_s *fdt, uint32_t id, uint32_t rank, uint32_t *idx);

typedef bl_error_type (*fdt_get_t)(fdt_s **fdt);
typedef bl_error_type (*fdt_verify_t)(fdt_s *fdt);
typedef bl_error_type (*fde_verify_t)(fdt_entry *fde);
typedef bl_error_type (*fde_update_t)(fdt_s *fdt, fdt_entry *fde, uint32_t idx);
typedef bl_error_type (*fde_get_by_idx_t)(fdt_s *fdt, fdt_entry *fde, uint32_t idx);
typedef bl_error_type (*fde_get_idx_by_id_t)(fdt_s *fdt, uint32_t id, uint32_t *idx);
typedef bl_error_type (*fde_get_idx_by_id_rank_t)(fdt_s *fdt, uint32_t id, uint32_t rank, uint32_t *idx);

typedef struct {
	fdt_get_t					fdt_get_pfn;
	fdt_verify_t				fdt_verify_pfn;
	fde_verify_t				fde_verify_pfn;
	fde_update_t				fde_update_pfn;
	fde_get_by_idx_t			fde_get_by_idx_pfn;
	fde_get_idx_by_id_t			fde_get_idx_by_id_pfn;
	fde_get_idx_by_id_rank_t	fde_get_idx_by_id_rank_pfn;
} rram_fdt_ind_t;
#endif
