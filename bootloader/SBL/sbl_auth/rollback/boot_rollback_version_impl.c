/*===========================================================================

                    Boot Rollback Version Image Set Impl

GENERAL DESCRIPTION
  This header file contains the definition of
  the rollback version image set table.

============================================================================*/

/*===========================================================================

                           EDIT HISTORY FOR FILE

This section contains comments describing changes made to this file.
Notice that changes are listed in reverse chronological order.


when       who     what, where, why
--------   ---     ----------------------------------------------------------
05/19/16   ck      Corrected fuse blowing issue in boot_rollback_update_fuse_version
04/08/16   ck      Added PBL loaded image rank logic
03/31/16   ck      Adjusted rollback fuse bit rows
03/21/16   ck      Initial revision

============================================================================*/
/*===========================================================================

                           INCLUDE FILES

===========================================================================*/
#include "boot_rollback_version_impl.h"
#include <string.h>
#include "HALhwio.h"
#include "Fermion_seq_hwioreg.h"
#include "boot_print.h"
#include "nt_bl_rram_dxe.h"

/*===========================================================================

                      MODULE MACROS/DEFINES

===========================================================================*/

#define RRAM_WRITE_ADR_BYTE_ALIGN 16
#define ANTI_ROLLBACK_OTP_OFFSET 0x1A00B0

/*===========================================================================

                      PUBLIC DATA DECLARATIONS

===========================================================================*/
static struct boot_version_rollback_img_set boot_rollback_version_img_set_table[] =
{
  /* Image set 0  QCOM Images */
  {
    /* images in this set*/
    {
      SEC_IMG_AUTH_SBL_IMG,
      SEC_IMG_AUTH_APP_IMG,
      SEC_IMG_AUTH_NUM_SUPPORTED_IMG
    },

    /* Antirollback feature enable row base address.
       If null then fuse enablement doesn't exist.
       Secboot enabled will be used only. */
    NULL,

    /* Antirollback feature enable mask */
    (uint32)NULL,

    /* Version lsb row base address*/
    (uint32 *)HWIO_FERMION_V2_0_QFPROM_RAW_FUSE_MAP_SECURITY_CONTROL_CORE_RAW_R_QFPROM_RAW_ANTI_ROLLBACK_W0_ADDR(SEQ_WCSS_OTP_OFFSET),

    /* Version lsb mask */
    HWIO_FERMION_V2_0_QFPROM_RAW_FUSE_MAP_SECURITY_CONTROL_CORE_RAW_R_QFPROM_RAW_ANTI_ROLLBACK_W0_M4_ANTI_ROLLBACK_31_0_BMSK,

    /* Version lsb shift */
    HWIO_FERMION_V2_0_QFPROM_RAW_FUSE_MAP_SECURITY_CONTROL_CORE_RAW_R_QFPROM_RAW_ANTI_ROLLBACK_W0_M4_ANTI_ROLLBACK_31_0_SHFT,

    /* Version msb base addr */
    (uint32 *)HWIO_FERMION_V2_0_QFPROM_RAW_FUSE_MAP_SECURITY_CONTROL_CORE_RAW_R_QFPROM_RAW_ANTI_ROLLBACK_W1_ADDR(SEQ_WCSS_OTP_OFFSET),

    /* Version msb mask */
    HWIO_FERMION_V2_0_QFPROM_RAW_FUSE_MAP_SECURITY_CONTROL_CORE_RAW_R_QFPROM_RAW_ANTI_ROLLBACK_W1_M4_ANTI_ROLLBACK_63_32_BMSK,

    /* Version msb shift */
    HWIO_FERMION_V2_0_QFPROM_RAW_FUSE_MAP_SECURITY_CONTROL_CORE_RAW_R_QFPROM_RAW_ANTI_ROLLBACK_W1_M4_ANTI_ROLLBACK_63_32_SHFT,
  },

};


uint32
boot_rollback_version_img_set_num = sizeof(boot_rollback_version_img_set_table) /
                                    sizeof(struct boot_version_rollback_img_set);


/*===========================================================================

**  Function : boot_rollback_validate_image_version

** ==========================================================================
*/
/*!
*
* @brief
*    This function indicates to the caller if the image version is higher or
*    equal to the version blown in fuses.  This is compared if secboot is
*    enabled or the image set has the antirollback feature enabled.  If neither
*    is true then evaluation is not done and success is returned.
*
* @param[in]
*    sec_img_auth_id_type          - Security image id
     secboot_verified_info_type *  - Verified info from secboot auth,
     secboot_hw_ftbl_type *        - Pointer to secboot hw function pointer table
*
* @par Dependencies
*    None
*
* @retval
*    bl_error_type             - Result of validation
*
*
*/
bl_error_type
boot_rollback_validate_image_version(sec_img_auth_id_type image_id,
                                     const secboot_hw_ftbl_type * const secboot_hw_ftbl,
                                     const secboot_verified_info_type * const secboot_verified_info)
{
  bl_error_type result = BL_ERR_NONE;
  secboot_hw_etype secboot_result = E_SECBOOT_HW_SUCCESS;
  uint32 secboot_enabled = 0;
  boot_version_rollback_img_set * image_set;
  boolean feature_enabled = FALSE;
  uint32 fuse_version_value = 0;


  /* Validate pointers */
  if ((secboot_hw_ftbl == NULL) ||
      (secboot_verified_info == NULL))
  {
    return BL_ERR_NULL_PTR;
  }

#if 0 //TODO
  /* Rollback is only valid for "Current" ranked images.
     PBL stored the rank in AON during cold boot.  Evaluate it. */
  if (*(uint8 *)LOADED_IMAGE_RANK_ADDRESS != LOADED_IMAGE_RANK_CURRENT)
  {
    return BL_ERR_NONE;
  }
#endif

  /* Find image set for image id passed from larger image table */
  result =
    boot_rollback_get_set_by_img_type(boot_rollback_version_img_set_table,
                                      boot_rollback_version_img_set_num,
                                      image_id,
                                      &image_set);
  if (result != BL_ERR_NONE)
  {
    return result;
  }


  /* Check if antirollback is enabled in the feature set. */
  result =
    boot_rollback_is_feature_enabled_on_set(image_set,
                                            &feature_enabled);
  if (result != BL_ERR_NONE)
  {
    return result;
  }


  /* Check if secboot is enabled. */
  switch (image_id)
  {
  case SEC_IMG_AUTH_SBL_IMG:
    secboot_result = secboot_hw_ftbl->secboot_hw_is_auth_enabled(SECBOOT_HW_M4_CODE_SEGMENT,
                                                                 &secboot_enabled);
    break;

  case SEC_IMG_AUTH_APP_IMG:
    secboot_result = secboot_hw_ftbl->secboot_hw_is_auth_enabled(SECBOOT_HW_M0_CODE_SEGMENT,
                                                                 &secboot_enabled);
    break;

  default:
    secboot_result = E_SECBOOT_HW_SUCCESS;
    secboot_enabled = FALSE;
    break;
  }

  if (secboot_result != E_SECBOOT_HW_SUCCESS)
  {
    return BL_ERR_IMG_SECURITY_FAIL;
  }


  /* If the feature is disabled in the image set and secboot is disabled then
     there is nothing to validate, return success. */
  if ((!feature_enabled) &&
      (!secboot_enabled))
  {
    return BL_ERR_NONE;
  }


  /* Obtain version value from fuses */
  result =
    boot_rollback_calculate_fuse_version(image_set,
                                         &fuse_version_value);
  if (result != BL_ERR_NONE)
  {
    return result;
  }


  /* Compare image version and fuse version values */
  /* Image version is the upper 32 bits of sw_id */
  if (((secboot_verified_info->sw_id >> 32) & 0xFFFFFFFF) < fuse_version_value)
  {
    return BL_ERR_ROLLBACK_VERSION_VERIFY_FAIL;
  }


  return BL_ERR_NONE;
}


/*===========================================================================

**  Function : boot_rollback_update_fuse_version

** ==========================================================================
*/
/*!
*
* @brief
*    This function evaluates the image version number against the version burnt
*    into the fuses and updates the fuses if the image version is greater.
*
* @param[in]
*    sec_img_auth_id_type          - Security image id
     secboot_verified_info_type *  - Verified info from secboot auth,
     secboot_hw_ftbl_type *        - Pointer to secboot hw function pointer table
*
* @par Dependencies
*    None
*
* @retval
*    bl_error_type
*
*
*/
bl_error_type
boot_rollback_update_fuse_version(sec_img_auth_id_type image_id,
                                  const secboot_hw_ftbl_type * const secboot_hw_ftbl,
                                  const secboot_verified_info_type * const secboot_verified_info)
{
  bl_error_type result = BL_ERR_NONE;
  secboot_hw_etype secboot_result = E_SECBOOT_HW_SUCCESS;
  uint32 secboot_enabled = 0;
  boot_version_rollback_img_set * image_set;
  boolean feature_enabled = FALSE;
  uint32 fuse_version_value = 0;
  uint32 sw_version = 0;
  //uint32 fuse_base_address;
  uint64 fuse_bit_mask = 0;
  uint32 lsb_mask = 0;
  uint32 msb_mask = 0;
  uint32 i = 0;

  /* Validate pointers */
  if ((secboot_hw_ftbl == NULL) ||
      (secboot_verified_info == NULL))
  {
    return BL_ERR_NULL_PTR;
  }

#if 0 // TODO
  /* Rollback is only valid for "Current" ranked images.
     PBL stored the rank in AON during cold boot.  Evaluate it. */
  if (*(uint8 *)LOADED_IMAGE_RANK_ADDRESS != LOADED_IMAGE_RANK_CURRENT)
  {
    return BL_ERR_NONE;
  }
#endif

  /* Find image set for image id passed from larger image table */
  result =
    boot_rollback_get_set_by_img_type(boot_rollback_version_img_set_table,
                                      boot_rollback_version_img_set_num,
                                      image_id,
                                      &image_set);
  if (result != BL_ERR_NONE)
  {
    return result;
  }


  /* Check if antirollback is enabled in the feature set. */
  result =
    boot_rollback_is_feature_enabled_on_set(image_set,
                                            &feature_enabled);
  if (result != BL_ERR_NONE)
  {
    return result;
  }


  /* Check if secboot is enabled. */
  switch (image_id)
  {
  case SEC_IMG_AUTH_SBL_IMG:
    secboot_result = secboot_hw_ftbl->secboot_hw_is_auth_enabled(SECBOOT_HW_M4_CODE_SEGMENT,
                                                                 &secboot_enabled);
    break;

  case SEC_IMG_AUTH_APP_IMG:
    secboot_result = secboot_hw_ftbl->secboot_hw_is_auth_enabled(SECBOOT_HW_M4_CODE_SEGMENT,
                                                                 &secboot_enabled);
    break;

  default:
    secboot_result = E_SECBOOT_HW_SUCCESS;
    secboot_enabled = FALSE;
    break;
  }

  if (secboot_result != E_SECBOOT_HW_SUCCESS)
  {
    return BL_ERR_IMG_SECURITY_FAIL;
  }


  /* If the feature is disabled in the image set and secboot is disabled then
     there is nothing to validate, return success. */
  if ((!feature_enabled) &&
      (!secboot_enabled))
  {
    return BL_ERR_NONE;
  }


  /* Keep blowing fuses until fuse version and image version match */
  {
    /* Obtain version value from fuses */
    result =
      boot_rollback_calculate_fuse_version(image_set,
                                           &fuse_version_value);
    if (result != BL_ERR_NONE)
    {
      return result;
    }


    /* Compare image version and fuse version values.  If equal then return. */
    /* Image version is the upper 32 bits of sw_id */
    if (((secboot_verified_info->sw_id >> 32) & 0xFFFFFFFF) <= fuse_version_value)
    {
      return BL_ERR_NONE;
    }

    sw_version = (secboot_verified_info->sw_id >> 32) & 0xFFFFFFFF;

    if (sw_version > 64)
    {
		return BL_ERR_NONE;
    }

	for (i=0; i< sw_version; i++)
	{
	    if (i < 32)
	    {
	       lsb_mask|= (0x1<<i);
	    }
		else
		{
		   msb_mask|= (0x1<<(i-32));
		}
	}
	fuse_bit_mask = msb_mask;
	fuse_bit_mask = (fuse_bit_mask<<32);
	fuse_bit_mask |= lsb_mask;

	//SECBOOT_PRINT("anti-rollback sw_ver %d otp value %x bit_mask %x:%x fuse_bit_mask:%016lx\r\n, ",  sw_version, *(uint32*)(0x1A00B0), msb_mask, lsb_mask, fuse_bit_mask);
	result = boot_rollback_fuse_version_save(fuse_bit_mask);
  }

  return BL_ERR_NONE;
}

bl_error_type
boot_rollback_fuse_version_save(uint64 fuse_value)
{
	bl_error_type result = BL_ERR_NONE;
	uint8 buf[RRAM_WRITE_ADR_BYTE_ALIGN] = {0};

	memcpy(buf, &fuse_value, sizeof(uint64));
	result = rram_block_write_dxe(ANTI_ROLLBACK_OTP_OFFSET, &(buf[0]), RRAM_WRITE_ADR_BYTE_ALIGN);

    return result;
}


