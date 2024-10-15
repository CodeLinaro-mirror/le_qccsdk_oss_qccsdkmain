/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/
/*=============================================================================

                   Boot Rollback Version Check

GENERAL DESCRIPTION
  This module provides functionality for version roll back protection

=============================================================================*/


/*=============================================================================

              EDIT HISTORY FOR MODULE

  This section contains comments describing changes made to the module.
  Notice that changes are listed in reverse chronological order.


when       who   what, where, why
--------   ---   ----------------------------------------------------------
05/19/16   ck    Corrected boot_rollback_calculate_fuse_version result issue
03/31/16   ck    Adjusted fuse row valid bits for OTP hw
03/24/16   ck    Added boot_rollback_find_open_version_bit
02/12/16   ck    Updated to new security definitions
08/03/15   ck    Initial creation

===========================================================================*/

/*===========================================================================

           INCLUDE FILES FOR MODULE

===========================================================================*/
#include "boot_rollback_version.h"


/*===========================================================================

                LOCAL FUNCTION DEFINITIONS

===========================================================================*/


/*===========================================================================

**  Function : boot_rollback_count_bits_set

** ==========================================================================
*/
/*!
*
* @brief
*    This function Counts the number of bits set in an uint32.
*
* @param[in] input_val:  uint32 Input value whose set bits need to be
*                        counted.
*
* @par Dependencies
*    None
*
* @retval
*    count: uint32 Number of bits set to 1.
*
* @par Side Effects
*    None
*
*/
uint32 boot_rollback_count_bits_set(uint32 input_val)
{
  uint32 count = 0;
  for(count = 0; input_val; count++)
  {
    input_val &= input_val - 1 ;
  }
  return count;
}


/*===========================================================================

**  Function : boot_rollback_find_open_bit

** ==========================================================================
*/
/*!
*
* @brief
*    This function looks for the first bit not set in value passed.  Mask is used
*    to specify what bits to evaluate.
*
* @param[in]
*    uint32 value:  Input value whose set bits need to be evaluated.
*    uint32 mask :  Mask to apply to value to specify what bits to evaluate.
*
* @par Dependencies
*    None
*
* @retval
*    uint32:  Bit mask with first open bit available.  0 if none were found.
*
* @par Side Effects
*    None
*
*/
uint32 boot_rollback_find_open_bit(uint32 value,
                                   uint32 mask)
{
  uint32 count = 0;


  for(count = 0;
      count < 32;
      count++)
  {
    /* If mask bit is set, and value bit is 0, this is an open fuse.  Return this
       bitmask */
    if (mask & (1 << count))
    {
      if (!(value & (1 << count)))
      {
        return (1 << count);
      }
    }
  }


  /* If execution reached here then no open version fuses were found. */
  return 0;
}


/*===========================================================================

**  Function : boot_rollback_get_set_by_img_type

** ==========================================================================
*/
/*!
*
* @brief
*    This function returns a pointer to the rollback image set given an image type
*
* @param[in/out]
*    boot_version_rollback_img_set *  - Pointer to entire image set table
*    uint32                           - Number of entries in image set table
*    sec_img_auth_id_type             - Security image id
*    boot_version_rollback_img_set *  - Pointer to matching image set if found
*
* @par Dependencies
*    None
*
* @retval
*    bl_error_type
*
*/
bl_error_type
boot_rollback_get_set_by_img_type(const boot_version_rollback_img_set * const image_set_table,
                                  uint32 image_set_table_entries,
                                  sec_img_auth_id_type image_sw_type,
                                  boot_version_rollback_img_set ** image_set)
{
  uint32 image_set_count = 0;
  uint32 current_image_index = 0;


  /* Validate pointers */
  if ((image_set_table == NULL) ||
      (image_set == NULL))
  {
    return BL_ERR_NULL_PTR;
  }

  /* Assign pointer after validation. */
  *image_set = (boot_version_rollback_img_set *)image_set_table;


  while (image_set_count < image_set_table_entries)
  {
    /* Image set can contain n number of images.  Check them all. */
    for(current_image_index = 0;
        current_image_index < BOOT_VERSION_ROLLBACK_MAX_IMG_NUM_PER_SET;
        current_image_index++)
    {
      if ((*image_set)->sec_sw_img_list[current_image_index] == image_sw_type)
      {
        return BL_ERR_NONE;
      }
    }
  }

  return BL_ERR_ROLLBACK_IMAGE_SET_NOT_FOUND;
}


/*===========================================================================

**  Function : boot_rollback_is_feature_enabled_on_set

** ==========================================================================
*/
/*!
*
* @brief
*    This function determins if rollback protection feature is enabled given an image set
*
* @param[in]
*    boot_version_rollback_img_set *  - Pointer to image set to evaluate
*    boolean *                        - Pointer to location where result should be stored
*
* @par Dependencies
*    None
*
* @retval
*    bl_error_type                    - Indicates if an error occured
*
*
*/
bl_error_type
boot_rollback_is_feature_enabled_on_set(const boot_version_rollback_img_set * const image_set,
                                        boolean * feature_enabled)
{
  /* Validate pointers */
  if ((image_set == NULL) ||
      (feature_enabled == NULL))
  {
    return BL_ERR_NULL_PTR;
  }


  /* Feature is disabled until evaluated true */
  *feature_enabled = FALSE;


  /* If feature enable base address is NULL then feature is disabled. */
  if (image_set->feature_enable_base_address != NULL)
  {
    /* Apply the bit mask on the fuse content, if result is nonzero then
       this feature is enabled */
    if ((*image_set->feature_enable_base_address) &
        image_set->feature_enable_mask)
    {
      *feature_enabled = TRUE;
    }
  }


  return BL_ERR_NONE;
}


/*===========================================================================

**  Function : boot_rollback_calculate_fuse_version

** ==========================================================================
*/
/*!
*
* @brief
*    This function returns the fuse version value given an image set
*
* @param[in]
*    boot_version_rollback_img_set *   - Pointer to image set to evaluate
*    uint32 *                          - Pointer to location to store result
*
* @par Dependencies
*    None
*
* @retval
*    bl_error_type                     - Indicates if an error occured
*
*/
bl_error_type
boot_rollback_calculate_fuse_version(const boot_version_rollback_img_set * const image_set,
                                     uint32 * calculated_version)
{
  uint32 * current_field = NULL;


  /* Validate pointers */
  if ((image_set == NULL) ||
      (calculated_version == NULL) ||
      (image_set->version_lsb_base_address == NULL) ||
      (image_set->version_msb_base_address == NULL))
  {
    return BL_ERR_NULL_PTR;
  }


  /* Zero out the calculated_version data */
  *calculated_version = 0;


  /* Get the raw value of LSB fuse by applying the bit mask */
  /* Add LSB to fuse version */
  *calculated_version +=
    boot_rollback_count_bits_set((*image_set->version_lsb_base_address) &
                                 image_set->version_lsb_mask);


  /* Check for fuse rows (4 byte aligned) in between the LSB & MSB addresses. */
  if ((image_set->version_msb_base_address -
       image_set->version_lsb_base_address) > (int)(sizeof(uint32)))
  {
    for (current_field = (uint32 *)(image_set->version_lsb_base_address + 1);
         current_field < image_set->version_msb_base_address;
         current_field++)
    {
      /* Calculate the corresponding fuse bits set and add to version */
      *calculated_version +=
        boot_rollback_count_bits_set(*current_field & 0xFF);
    }
  }


  /* Get the raw value of MSB fuse by applying the bit mask */
  /* Add MSB to fuse version */
  *calculated_version +=
    boot_rollback_count_bits_set((*image_set->version_msb_base_address) &
                                 image_set->version_msb_mask);


  return BL_ERR_NONE;
}


/*===========================================================================

**  Function : boot_rollback_find_open_version_bit

** ==========================================================================
*/
/*!
*
* @brief
*    This function returns the base address and the bit mask of the next
*    open version fuse for an image.
*
* @param[in]
*    boot_version_rollback_img_set *   - Pointer to image set to evaluate
*    uint32                            - Pointer to location to store fuse row with open version
*    uint64                            - Pointer to location to mark open version bitmask
*
* @par Dependencies
*    None
*
* @retval
*    bl_error_type                     - Indicates if an error occured
*
*/
bl_error_type
boot_rollback_find_open_version_bit(const boot_version_rollback_img_set * const image_set,
                                    uint32 * fuse_base_address,
                                    uint64 * fuse_bit_mask)
{
  uint32 * current_field = NULL;


  /* Validate pointers */
  if ((image_set == NULL) ||
      (fuse_base_address == NULL) ||
      (fuse_bit_mask == NULL) ||
      (image_set->version_lsb_base_address == NULL) ||
      (image_set->version_msb_base_address == NULL))
  {
    return BL_ERR_NULL_PTR;
  }


  /* Check for open fuse bit in LSB */
  *fuse_base_address = (uint32)image_set->version_lsb_base_address;
  *fuse_bit_mask = boot_rollback_find_open_bit(*image_set->version_lsb_base_address,
                                               image_set->version_lsb_mask);
  if (*fuse_bit_mask)
  {
    /* Open fuse was found, exit. */
    return BL_ERR_NONE;
  }


  /* Check for fuse rows (4 byte aligned) in between the LSB & MSB addresses. */
  if ((image_set->version_msb_base_address -
       image_set->version_lsb_base_address) > (int)(sizeof(uint32)))
  {
    for (current_field = (uint32 *)(image_set->version_lsb_base_address + 1);
         current_field < image_set->version_msb_base_address;
         current_field++)
    {
      /* Check for open fuse bit in between the LSB & MSB addresses. */
      /* Provision only the fuse row base address (that is 4-byte aligned). */
      *fuse_base_address = (uint32)current_field & 0xFFFFFFFF;
      *fuse_bit_mask = boot_rollback_find_open_bit(*current_field,
                                                   0xFF);
      if (*fuse_bit_mask)
      {
        /* Open fuse was found, exit. */
        return BL_ERR_NONE;
      }
    }
  }


  /* Check for open fuse bit in MSB */
  *fuse_base_address = (uint32)image_set->version_msb_base_address;
  *fuse_bit_mask = boot_rollback_find_open_bit(*image_set->version_msb_base_address,
                                               image_set->version_msb_mask);
  if (*fuse_bit_mask)
  {
    /* Open fuse was found, exit. */
    return BL_ERR_NONE;
  }


  /* If execution reaches here then no open version fuse was found.  Return error. */
  return BL_ERR_ROLLBACK_OPEN_VERSION_FUSE_NOT_FOUND;
}
