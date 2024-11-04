/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/


#include <stdio.h>
#include <string.h>
#include "pbl_patch_table.h"
#include "pbl_version_build.h"

/****************************************************************************/
/** external variables */
/****************************************************************************/
/* LMA of .data from pbl_patch*/
extern uint32_t _ln_RF_start_addr_patch_data__;
/* VMA of .data from pbl_patch*/
extern uint32_t _ln_RAM_start_addr_patch_data__;
/* Size of .dat from pbl_patch*/
extern uint32_t _ln_patch_data_size__;

void pbl_patch_functions(void *fn_ind_table_addr_ram);
void pbl_patch_data(void *global_data_addr_ram);
//#define PBL_PATCH_TEST
/****************************************************************************/
/** global definitions */
/****************************************************************************/

/* Patcher functions to update the indirection table and global data table. They
 * are placed at the start of PBL_PATCH Memory region.
 */
__attribute__ ((section(".pbl_patch_fn"))) pbl_patch_func_t patch_functions = {
    pbl_patch_data,
    pbl_patch_functions
};

/* Pointer to indirection table in RAM*/
func_ind_t *ind_table_ram_ptr = NULL;
/* Pointer to global data table in RAM*/
glb_data_t *data_table_ram_ptr = NULL;

const uint32_t pbl_version_num  __attribute__ ((section(".pbl_version_num"))) = ((PBL_VER_MAJOR << 24) | (PBL_VER_MINOR << 16) | (PBL_VER_COUNT));

//#define PBL_PATCH_TEST_DATA
//#define PBL_PATCH_TEST_FUNC
#ifdef PBL_PATCH_TEST_DATA
/*dummy patch Data*/
uint32_t patch_fdt_addr = 0x216400;
#endif

/****************************************************************************/
/** Function Definitions */
/****************************************************************************/

#ifdef PBL_PATCH_TEST_FUNC
/*dummy patch function*/
bl_error_type sbl_ota_process_patch(sbl_ota_s *sbl_ota)
{
	(void)sbl_ota;
	return 1;
}
#endif

/* Patches the function indirection table
 * @param fn_ind_table_addr_ram Start address of the function indirection table
 * in RAM
 * @note This function must be called to patch the functions
 */
void pbl_patch_functions(void *fn_ind_table_addr_ram)
{
    ind_table_ram_ptr = (func_ind_t *)fn_ind_table_addr_ram;
#ifdef PBL_PATCH_TEST_FUNC
	ind_table_ram_ptr->sbl_ota.sbl_ota_process_pfn = sbl_ota_process_patch;
#endif
}

/* Patches the global data pointers table
 * @param global_data_addr_ram Start address of the structure of global data
 * indirection in RAM
 * @note This function must be called to patch the data
 */
void pbl_patch_data(void *global_data_addr_ram)
{
    /*Copy .data table from RRAM to RAM */
    //memcpy( &_ln_RAM_start_addr_patch_data__, &_ln_RF_start_addr_patch_data__, (uint32_t)&_ln_patch_data_size__);

    data_table_ram_ptr = (glb_data_t*)global_data_addr_ram;
#ifdef PBL_PATCH_TEST_DATA
    data_table_ram_ptr->fdt_addr = (fdt_s *)patch_fdt_addr;
#endif
}
