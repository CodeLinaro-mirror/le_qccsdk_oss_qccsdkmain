

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
#include "sec_auth_errors.h"
#include "sbl_auth.h"
#include "sec_img_auth.h"
#include "boot_rollback_version_impl.h"
#include "boot_print.h"

/******************************************************************************
                             MODULE MACROS/DEFINES
                         ADD NEW ONES UNDER THIS LINE
******************************************************************************/


/******************************************************************************
                         MODULE TYPES and TYPE-DEFINES
                         ADD NEW ONES UNDER THIS LINE
******************************************************************************/

typedef struct _pbl_elf_image_info_type
{
  Elf32_Ehdr     * ehdr;                        /* Data-structure
                                                    maintaining
                                                    ELF Header */

  Elf32_Phdr     * phdr;                        /* Data-structure
                                                    maintaining
                                                    Program Header */


  uint8          *  hash_segment_start_address; /* Pointer to the Hash segment */

  uint32            hash_segment_size;
} pbl_elf_image_info_type;


/******************************************************************************
                             MODULE DATA DEFITIONS
                         ADD NEW ONES UNDER THIS LINE
******************************************************************************/


/******************************************************************************
                            PUBLIC DATA DECLARATION
                         ADD NEW ONES UNDER THIS LINE
******************************************************************************/
/* M4 ELF info */
pbl_elf_image_info_type image_elf_info;

sec_img_auth_priv_t s_img_handle;

sec_img_auth_verified_info_s	sbl_auth_verified_info
                                           __attribute__((aligned(64)));
/* M4F hash segment holder */
static uint8 image_hash_segment[MAX_HASH_SEGMENT_SIZE];


/******************************************************************************
                         MODULE FUNCTION DECLARATIONS
                         ADD NEW ONES UNDER THIS LINE
******************************************************************************/
boolean sbl_check_valid_hash_address_range(Elf32_Phdr *hash_segment_phdr, uint32 hash_segment_buf_size);

uint32 sbl_secboot_is_auth_enabled(uint32 *auth_enabled_ptr);

boolean sbl_check_anti_rollback_ver( sec_img_auth_id_type app_img_id, boot_apps_shared_data_type *sbl_shared, secboot_verified_info_type *sbl_verified);

/******************************************************************************
                         MODULE FUNCTION DEFINITIONS
                         ADD NEW ONES UNDER THIS LINE
******************************************************************************/


/**===========================================================================
**
** FUNCTION DESCRIPTION
**     This function return the actual size of the hash segment of ELF image
**
** DEPENDENCIES
**     None
**
** PARAMETERS
**     Type     : [IN]
**     DataType : pbl_elf_handle_type *
**     Param    : elf_handle
**                Pointer to ELF handler
**
**     Type     : [IN]
**     DataType : uint32 *
**     Param    : Pointer to hash_segment_size destination
**
** RETURN VALUE
**     DataType : uint32
**     Value    :
**                BL_ERR_NONE - success
**
** SIDE EFFECTS
**     None.
**
**==========================================================================*/
uint32 sbl_get_hash_segment_phdr(boot_elf_loader * elf_loader_ptr, Elf32_Phdr **hash_segment_phdr, uint32 *hash_segment_idx)
{
	uint32 ret_val              = PBL_DEPENDENCY_FAIL;
	uint32 segment_index;
	uint32 pflags;
	uint16 phnum;
	boot_elf32_ehdr *  ehdr = NULL;

	/* Verify input parameters */
	if(NULL == elf_loader_ptr)
	{
		return BL_ERR_IMG_AUTH_NULL_PTR;
	}

	ehdr = &elf_loader_ptr->elf_hdr;
	if (NULL == ehdr)
	{
		return BL_ERR_IMG_AUTH_NULL_PTR;
	}

	phnum = ehdr->e_phnum;

	for(segment_index = 0; segment_index < phnum; segment_index++)
	{
		pflags = elf_loader_ptr->prog_hdrs[segment_index].p_flags;

		/* Check for the hash segment program header to parse QC header */
		if( MI_PBT_HASH_SEGMENT == MI_PBT_SEGMENT_TYPE_VALUE(pflags))
		{
			*hash_segment_phdr = (Elf32_Phdr*)&(elf_loader_ptr->prog_hdrs[segment_index]);
			*hash_segment_idx = segment_index;
			ret_val = BL_ERR_NONE;
			break;
		}
	}

	return ret_val;
}


/**===========================================================================
 **
 ** FUNCTION DESCRIPTION
 **     This helper function returns if there is a valid ROM patching info in FWD.
 **
 ** DEPENDENCIES
 **     None
 **
 ** PARAMETERS
 **     Type     : [IN]
 **     DataType : void *
 **     Param    : ehdr_ptr
 **                Pointer to ELF header
 **     Type     : [IN]
 **     DataType : void *
 **     Param    : phdr_ptr
 **                Pointer to Program header
 **     Type     : [IN]
 **     DataType : uint8 *
 **     Param    : hash_seg_ptr
 **                Pointer to Hash segment
 **     Type     : [IN]
 **     DataType : uint32
 **     Param    : hash_seg_size
 **                Hash segment size
 **     Type     : [IN]
 **     DataType : sec_img_auth_id_type
 **     Param    : img_id
 **                Image ID
 **     Type     : [IN]
 **     DataType : boot_apps_shared_data_type
 **     Param    : sbl_shared
 **                PBL shared data with SBL
 **
 **
 ** RETURN VALUE
 **     DataType : uint32
 **     Value    :
 **                BL_ERR_NONE - success
 **
 **
 ** SIDE EFFECTS
 **     None.
 **
 **==========================================================================*/
uint32 sbl_secboot_image_auth
(
 void       *ehdr_ptr,
 void       *phdr_ptr,
 uint8	   *hash_seg_ptr,
 uint32      hash_seg_size,
 sec_img_auth_id_type img_id,
 boot_apps_shared_data_type *sbl_shared
 )
{
	sec_img_auth_error_type sec_img_auth_status = SEC_IMG_AUTH_FAILURE;
	static sec_img_auth_elf_info_type sbl_elf_info;
	static secboot_hw_ftbl_type secboot_hw_ftbl;
	static crypto_ftbl_type crypto_ftbl = {0};
	static uint8 crypto_imem[CRYPTO_HASH_IMEM_DATA_BYTE_SIZE];
	sec_img_auth_verified_info_s* sbl_auth_verified_info_p = &sbl_auth_verified_info;
	sec_img_auth_ftbl_type * sec_img_auth_ftbl = &(sbl_shared->sec_img_auth_ftbl);

    if ((NULL == ehdr_ptr) || (NULL == phdr_ptr) || (NULL == hash_seg_ptr) || (0 == hash_seg_size))
    {
        return BL_ERR_IMG_AUTH_NULL_PTR;
    }

	/* Initialize Secimg/Crypto driver */
	s_img_handle.crypto_ftbl  = &crypto_ftbl;
	sec_img_auth_status = sec_img_auth_ftbl->sec_img_auth_init(&s_img_handle);
	if (SEC_IMG_AUTH_SUCCESS != sec_img_auth_status)
		return BL_ERR_IMG_SECURITY_FAIL;

	memset(&sbl_elf_info, 0, sizeof(sbl_elf_info));
	memset(sbl_auth_verified_info_p, 0, sizeof(sec_img_auth_verified_info_s));

	sbl_elf_info.elf_hdr      = ehdr_ptr;
	sbl_elf_info.prog_hdr     = phdr_ptr;
	sbl_elf_info.hash_seg_hdr = hash_seg_ptr;

	s_img_handle.img_id = img_id;
	s_img_handle.img_data = &sbl_elf_info;
	s_img_handle.crypto_ftbl = &crypto_ftbl;
	s_img_handle.ftbl_ptr = &secboot_hw_ftbl;

	crypto_ftbl.crypto_ctx.ctx_imem = crypto_imem;
	crypto_ftbl.crypto_ctx.ctx_imem_size = CRYPTO_HASH_IMEM_DATA_BYTE_SIZE;

	sec_img_auth_status = sec_img_auth_ftbl->sec_img_auth_verify_metadata(&s_img_handle, sbl_auth_verified_info_p);

	if (SEC_IMG_AUTH_SUCCESS != sec_img_auth_status)
	{
		return BL_ERR_IMG_META_DATA_AUTH_FAIL;
	}

	if (!sbl_check_anti_rollback_ver(img_id, sbl_shared, &(sbl_auth_verified_info_p->v_info)))
	{
	    SECBOOT_PRINT("Not same rollback ver\r\n");
	    return BL_ERR_ROLLBACK_VERSION_VERIFY_FAIL;
	}

	return BL_ERR_NONE;
}

/**===========================================================================
 **
 ** FUNCTION DESCRIPTION
 **     This function is called to initialized M4F ELF handler data
 **
 ** DEPENDENCIES
 **     None
 **
 ** PARAMETERS
 **     None
 **
 **
 ** RETURN VALUE
 **     DataType : uint32
 **     Value    :
 **                BL_ERR_NONE - success
 **                != BL_ERR_NONE - error
 **
 ** SIDE EFFECTS
 **     None.
 **
 **==========================================================================*/
uint32 sbl_image_auth_handler_init(void)
{
   uint32 res = BL_ERR_NONE;

   // Reinit will clean up the memories of ehdr and phdr.
   // So, it is not needed to be done here in init function.
   image_elf_info.hash_segment_start_address = (uint8*)image_hash_segment;
   image_elf_info.hash_segment_size = sizeof(image_hash_segment);

   return res;
}

uint32 sbl_image_auth
(
	secboot_auth_image_info_s *image_info,
	boot_apps_shared_data_type *sbl_shared
)
{
	sec_img_auth_error_type sbl_sec_img_status = SEC_IMG_AUTH_FAILURE;
    Elf32_Phdr	   *hash_segment_phdr = NULL;
	boot_elf_loader * elf_loader_ptr = image_info->elf_loader_ptr;
	sec_img_auth_ftbl_type * sec_img_auth_ftbl = &(sbl_shared->sec_img_auth_ftbl);
	secboot_hw_ftbl_type *secboot_hw_ftbl = &(sbl_shared->secboot_shared_data->pbl_secboot_hw_ftbl);
	uint32 sbl_ret   = BL_ERR_NONE;
	uint32 hash_segment_idx   = 0;

	if(NULL == elf_loader_ptr)
	{
		return BL_ERR_IMG_AUTH_NULL_PTR;
	}

	sbl_ret = secboot_hw_ftbl->secboot_hw_is_auth_enabled(SECBOOT_HW_M4_CODE_SEGMENT,
																	&(s_img_handle.auth_info.auth_enabled));
	if (sbl_ret != E_SECBOOT_HW_SUCCESS)
	{
	    return BL_ERR_IMG_SECURITY_FAIL;
	}

	if (FALSE == s_img_handle.auth_info.auth_enabled)
	{
	    SECBOOT_PRINT("secboot disable\r\n");
	    return BL_ERR_NONE;
	}

	sbl_image_auth_handler_init();

	image_elf_info.ehdr = (Elf32_Ehdr*)(&elf_loader_ptr->elf_hdr);
	image_elf_info.phdr = (Elf32_Phdr*)(&elf_loader_ptr->prog_hdrs[0]);

	sbl_sec_img_status = sec_img_auth_ftbl->sec_img_auth_validate_elf(&elf_loader_ptr->elf_hdr);
    if(SEC_IMG_AUTH_SUCCESS != sbl_sec_img_status)
	{
		return (uint32)sbl_sec_img_status;
	}
	if (sbl_get_hash_segment_phdr(elf_loader_ptr, &hash_segment_phdr, &hash_segment_idx) != BL_ERR_NONE)
	{
        SECBOOT_PRINT("No hash segment\r\n");
	    return BL_ERR_IMG_AUTH_INVAL_PARAM;
	}

    if (!sbl_check_valid_hash_address_range(hash_segment_phdr, MAX_HASH_SEGMENT_SIZE))
    {
	    return BL_ERR_IMG_AUTH_INVAL_PARAM;
    }

	/* Load full hash segment into memory */
	sbl_ret = boot_elf_load_generic_segment(elf_loader_ptr, hash_segment_idx, 0, (uint32)image_elf_info.hash_segment_start_address, hash_segment_phdr->p_filesz);

	if (BL_ERR_NONE != sbl_ret)
	{
        SECBOOT_PRINT("load hash segment failed\r\n");
		return sbl_ret;
	}
    sbl_ret = sbl_secboot_image_auth((void*)(&elf_loader_ptr->elf_hdr), (void*)(&elf_loader_ptr->prog_hdrs), (uint8 *)(image_elf_info.hash_segment_start_address), image_elf_info.hash_segment_size, image_info->image_id, sbl_shared);

	if (sbl_ret == BL_ERR_NONE)
	{
	    SECBOOT_PRINT("sbl_secboot_image_auth SUC\r\n");
	}
	else
	{
	    SECBOOT_PRINT("sbl_secboot_image_auth FAIL\r\n");
	}

	return sbl_ret;
}

/**===========================================================================
 **
 ** FUNCTION DESCRIPTION
 **     This function compute/verify the hashes for ELF image
 **
 ** DEPENDENCIES
 **     None
 **
 ** PARAMETERS
 **     Type     : [IN]
 **     DataType : boot_apps_shared_data_type *
 **     Param    : pbl_shared
 **                Pointer to PBL shared data
 **     Type     : [IN]
 **     DataType : uint32
 **     Param    : img_rank
 **                Image rank

 **
 **
 ** RETURN VALUE
 **     DataType : uint32
 **     Value    :
 **                BL_ERR_NONE - success
 **
 ** SIDE EFFECTS
 **     None.
 **
 **==========================================================================*/
uint32 sbl_compute_verify_hash(boot_apps_shared_data_type *sbl_shared, uint32 img_rank)
{
  sec_img_auth_verified_info_s* sbl_auth_verified_info_p = &sbl_auth_verified_info;
  sec_img_auth_ftbl_type * sec_img_auth_ftbl = &(sbl_shared->sec_img_auth_ftbl);
  secboot_hw_ftbl_type *secboot_hw_ftbl = &(sbl_shared->secboot_shared_data->pbl_secboot_hw_ftbl);
  sec_img_auth_error_type sec_img_auth_status = SEC_IMG_AUTH_FAILURE;
  uint32 ret = BL_ERR_NONE;

  if (FALSE == s_img_handle.auth_info.auth_enabled)
  {
	  return BL_ERR_NONE;
  }

  sec_img_auth_status = sec_img_auth_ftbl->sec_img_auth_hash_elf_segments(&s_img_handle, sbl_auth_verified_info_p);
  if(SEC_IMG_AUTH_SUCCESS != sec_img_auth_status)
  {
      ret = BL_ERR_IMG_SECURITY_FAIL;
  }

  if ((ret == BL_ERR_NONE)
  	   && (OTA_IMG_RANK_CURRENT == img_rank)
  	   && (SECBOOT_ANTI_ROLLBACK_UNLOCKED == sbl_auth_verified_info_p->v_info.anti_rollback_lock))
  {
   	  ret = boot_rollback_update_fuse_version(SECBOOT_IMG_AUTH_APP_IMG,
   												 secboot_hw_ftbl,
   												 &sbl_auth_verified_info.v_info);
  }

  if (ret == BL_ERR_NONE)
  {
	SECBOOT_PRINT("sbl_compute_verify_hash SUC\r\n");
  }
  else
  {
    SECBOOT_PRINT("sbl_compute_verify_hash FAIL\r\n");
  }

  return ret;
}


/**===========================================================================
 **
 ** FUNCTION DESCRIPTION
 **     This function returns if M4F hash adress range is valid
 **
 ** DEPENDENCIES
 **     None
 **
 ** PARAMETERS
 **     Type     : [IN]
 **     DataType : uint32
 **     Param    : hash_segment_size
 **                Hash segment size
 **
 **
 ** RETURN VALUE
 **     DataType : boolean
 **     Value    :
 **                TRUE - valid
 **
 **
 ** SIDE EFFECTS
 **     None.
 **
 **==========================================================================*/
boolean sbl_check_valid_hash_address_range(Elf32_Phdr *hash_segment_phdr, uint32 hash_segment_buf_size)
{
   if (hash_segment_phdr == NULL)
      return FALSE;

   if ((hash_segment_phdr->p_memsz > hash_segment_buf_size)
   	    || (hash_segment_phdr->p_filesz> hash_segment_buf_size))
      return FALSE;

   return TRUE;
}


/**===========================================================================
 **
 ** FUNCTION DESCRIPTION
 **     This function returns if M4F hash adress range is valid
 **
 ** DEPENDENCIES
 **     None
 **
 ** PARAMETERS
 **     Type     : [IN]
 **     DataType : sec_img_auth_id_type
 **     Param    : img_id
 **                Image ID
 **     Type     : [IN]
 **     DataType : boot_apps_shared_data_type
 **     Param    : sbl_shared
 **                PBL shared data with SBL
 **     Type     : [IN]
 **     DataType : secboot_verified_info_type
 **     Param    : sbl_verified
 **                Verified infomation after SBL authenticated application image
 **
 **
 ** RETURN VALUE
 **     DataType : boolean
 **     Value    :
 **                TRUE - valid
 **
 **
 ** SIDE EFFECTS
 **     None.
 **
 **==========================================================================*/
boolean sbl_check_anti_rollback_ver( sec_img_auth_id_type app_img_id, boot_apps_shared_data_type *sbl_shared, secboot_verified_info_type *sbl_verified)
{
   sec_img_auth_id_type sbl_img_id = SEC_IMG_AUTH_SBL_IMG;
   secboot_verified_info_type *pbl_verified = NULL;
   uint32 app_version = 0;
   uint32 sbl_version = 0;

   if ((sbl_shared == NULL) || (sbl_shared->secboot_shared_data == NULL))
   {
       return FALSE;
   }

   pbl_verified = &(sbl_shared->secboot_shared_data->pbl_verified_info);

   sbl_img_id = (uint32)((pbl_verified->sw_id) & 0xFFFFFFFF);
   if ((SECBOOT_ANTI_ROLLBACK_UNLOCKED == pbl_verified->anti_rollback_lock)
   	    &&(SEC_IMG_AUTH_SBL_IMG == sbl_img_id)
   	    && (SEC_IMG_AUTH_APP_IMG == app_img_id))
   {
       app_version = (pbl_verified->sw_id>>32) & 0xFFFFFFFF;
	   sbl_version = (sbl_verified->sw_id>>32) & 0xFFFFFFFF;
	   if (app_version != sbl_version)
	   {
	       pbl_verified->anti_rollback_lock = SECBOOT_ANTI_ROLLBACK_LOCKED;
		   return FALSE;
	   }
   }

   return TRUE;
}

