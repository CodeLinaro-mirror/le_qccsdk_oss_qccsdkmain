/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/


/**===========================================================================
 **
 **                        PRIMARY BOOT LOADER
 **                        ------------------- 
 ** FILE 
 **     sbl_auth.c
 ** 
 ** GENERAL DESCRIPTION 
 **     This file contains the PBL interface to the software that authenticates
 **     the code image from external source (ie: Flash).
 **
 **==========================================================================*/


/*=============================================================================

                            EDIT HISTORY FOR MODULE

  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.

  when       who            what, where, why
  --------   ----------     ---------------------------------------------------
  09/10/15   bmuthusa             Initial Revision for qca402x
=============================================================================*/


/******************************************************************************
                                MODULE INCLUDES
                         ADD NEW ONES UNDER THIS LINE
******************************************************************************/
#include <string.h>
#include "rram_fdt.h"
#include "boot_elf_loader.h"
#include "sbl_auth.h"
#include "sec_img_auth.h"
#include "boot_rollback_version_impl.h"
#include "sha256.h"
#include "boot_print.h"

/******************************************************************************
                             MODULE MACROS/DEFINES
                         ADD NEW ONES UNDER THIS LINE  
******************************************************************************/


/******************************************************************************
                         MODULE TYPES and TYPE-DEFINES
                         ADD NEW ONES UNDER THIS LINE  
******************************************************************************/


/******************************************************************************
                             MODULE DATA DEFITIONS
                         ADD NEW ONES UNDER THIS LINE  
******************************************************************************/


/******************************************************************************
                            PUBLIC DATA DECLARATION
                         ADD NEW ONES UNDER THIS LINE  
******************************************************************************/
pbl_hash_handle_t hash_handle;

/******************************************************************************
                         MODULE FUNCTION DECLARATIONS
                         ADD NEW ONES UNDER THIS LINE
******************************************************************************/


/******************************************************************************
                         MODULE FUNCTION DEFINITIONS
                         ADD NEW ONES UNDER THIS LINE  
******************************************************************************/
uint32 sbl_image_auth
(
	secboot_auth_image_info_t *image_info,
	boot_apps_shared_data_t *sbl_shared
)
{    
	uint32 sbl_ret = BL_ERR_NONE;

    if ((FALSE == sbl_shared->auth_state.auth_enabled) && (TRUE == sbl_shared->auth_state.hash_integrity_check_disabled))
    {
        return BL_ERR_NONE;
    }
	
    if ((NULL == sbl_shared->secimg_auth) || (NULL == sbl_shared->pbl_get_image_hash_handle))
    {
        return BL_ERR_IMG_AUTH_NULL_PTR;
    }
	
    sbl_ret = sbl_shared->secimg_auth(image_info);
	if (BL_ERR_NONE == sbl_ret)
	{
	    sbl_ret = sbl_shared->pbl_get_image_hash_handle(&hash_handle);	
	}

	if ((BL_ERR_NONE == sbl_ret) && (TRUE == sbl_shared->auth_state.auth_enabled))
	{
	    if (FALSE == boot_rollback_validate_image_version(image_info->image_id, &(sbl_shared->auth_state), &(hash_handle.auth_state)))
	    {
	        sbl_ret = BL_ERR_ROLLBACK_VERSION_VERIFY_FAIL;    
	    }
	}
	
	if (BL_ERR_NONE != sbl_ret)
	{	    
	    if (FALSE == sbl_shared->auth_state.auth_enabled)
	    {
	        SECBOOT_PRINT("app image header hash verification FAILED\r\n");
	     
	    }
		else
		{
	        SECBOOT_PRINT("app image authentication FAILED\r\n");   
		}
	}
	return sbl_ret;
}

uint32 sbl_compute_verify_hash(secboot_auth_image_info_t *image_info, boot_apps_shared_data_t *sbl_shared)
{    
	uint i = 0;	
	uint32 sbl_ret = BL_ERR_NONE;
	secboot_error_type	 sec_err = E_SECBOOT_FAILURE;  
	uint32_t phr_num;
	boot_elf32_phdr *p_prog_hdrs;
	uint8 *p_hash = &(hash_handle.hash_table[0]);
	uint8 sha_digest[CRYPTO_DIGEST_BYTE_SIZE_SHA256];
	
	sec_img_auth_ftbl_type * sec_img_auth_ftbl = NULL;	
	pbl_auth_state_t *p_sbl_img_auth_state = NULL;	 
	pbl_auth_state_t *p_app_img_auth_state = &(hash_handle.auth_state);

	if ((NULL == image_info) 
		|| (NULL == image_info->elf_loader_ptr) 
		|| (NULL == sbl_shared))
	{
		return BL_ERR_IMG_AUTH_NULL_PTR;
	}

    if ((FALSE == sbl_shared->auth_state.auth_enabled) && (TRUE == sbl_shared->auth_state.hash_integrity_check_disabled))
    {
        return BL_ERR_NONE;
    }

    /*Initialize the program header of app image*/
	p_prog_hdrs = &(image_info->elf_loader_ptr->prog_hdrs[0]);
	phr_num = image_info->elf_loader_ptr->phr_num;

	/*Get the shared info from PBL*/
	p_sbl_img_auth_state = &(sbl_shared->auth_state);
	sec_img_auth_ftbl = &(sbl_shared->sec_img_auth_ftbl);	
	
	hash_handle.crypto_ftbl.crypto_ctx.ctx_imem = hash_handle.crypto_imem;
	hash_handle.crypto_ftbl.crypto_ctx.ctx_imem_size = CRYPTO_HASH_IMEM_DATA_BYTE_SIZE;

    for (i=1; i<phr_num; i++)
    {        
		p_hash += CRYPTO_DIGEST_BYTE_SIZE_SHA256;
        if (sec_img_auth_ftbl->secimg_auth_is_valid_segment(ELF_CLASS_32, (void*)(&p_prog_hdrs[i])) 
			 && (p_prog_hdrs[i].p_filesz != 0))
        {
	        sec_err = mbedtls_sha256_ret((uint8 *)(p_prog_hdrs[i].p_vaddr), p_prog_hdrs[i].p_filesz, sha_digest, 0);
			if (sec_err != E_SECBOOT_SUCCESS)
            {
                sbl_ret = BL_ERR_IMG_SECURITY_FAIL;
                break;
            }

			if (0 != memcmp(p_hash, sha_digest, CRYPTO_DIGEST_BYTE_SIZE_SHA256))
			{
                sbl_ret = BL_ERR_ELF_HASH_MISMATCH;
                break;
			}
        }
    }

	if ((sbl_ret == BL_ERR_NONE) 
		   && (TRUE == sbl_shared->auth_state.auth_enabled)
		   && (OTA_IMG_RANK_CURRENT == image_info->image_rank) 
		   && (TRUE == p_sbl_img_auth_state->anti_rollback_unlock)) 
	{
		  sbl_ret = boot_rollback_update_fuse_version(SECBOOT_IMG_AUTH_APP_IMG,
													 p_app_img_auth_state);
	}

    if (sbl_ret != BL_ERR_NONE)
    {
        SECBOOT_PRINT("app image hash verification FAILED\r\n");
    }
    return sbl_ret;
}

