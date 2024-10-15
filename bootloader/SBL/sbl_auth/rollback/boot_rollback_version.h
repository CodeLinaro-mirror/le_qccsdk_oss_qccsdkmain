/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/
#ifndef BOOT_VERSION_ROLLBACK_H
#define BOOT_VERSION_ROLLBACK_H

/*===========================================================================

                        Boot Module Interface Header File

GENERAL DESCRIPTION
  This header file contains the definition of the public interface to
  the rollback version prevention feature.

============================================================================*/

/*===========================================================================

                           EDIT HISTORY FOR FILE

This section contains comments describing changes made to this file.
Notice that changes are listed in reverse chronological order.


when       who     what, where, why
--------   ---     ----------------------------------------------------------
03/24/16   ck      Added boot_rollback_find_open_version_bit
02/12/16   ck      Updated to new security definitions
08/03/15   ck      Initial creation

============================================================================*/
/*===========================================================================

                           INCLUDE FILES

===========================================================================*/
#include "boot_error_if.h"

#include "seccommon.h"
#include "sec_img_auth.h"

/*===========================================================================
                      PUBLIC DATA DECLARATIONS
===========================================================================*/

#define BOOT_VERSION_ROLLBACK_MAX_IMG_NUM_PER_SET 3

/*
  - Data structure that represent a set of images that share the same anti-
    rollback version fuses.
  - Each image set can contain a max of
    BOOT_VERSION_ROLLBACK_MAX_IMG_NUM_PER_SET images.
  - Each image set has one fuse which indicates if rollback protection is
    enabled for current image set or not.
  - Each image set also has one variable that store the current version
    (obtained from secboot cert after authentication).**
    (**: ToDo confirm if this legacy behavior is currently supported or not)
  - Anti-rollback supported for the one current image set.
  - Currently the current image set consists of only M4 and M0 images.
  - Both M4 and M0 share the 64 fuse bits ranging across the LSB and MSB
    addresses and with only 8 fuse bits available at each of these 32-bit
    locations.
  - Since these fields are shared across M4 and M0,
    a) The corresponding versions of M4 and M0 may be interleaved across these
       64 fuse bits and based on the respectively versions set/configured via
       devcfg xml.
    b) An anti-rollback version > 32 bitmap value, can be configured for either
       M4 or M0 (*This is a legacy behavior and it continues to be supported.)

*/
typedef struct boot_version_rollback_img_set
{
  /* List of images in current image set*/
  const sec_img_auth_id_type sec_sw_img_list[BOOT_VERSION_ROLLBACK_MAX_IMG_NUM_PER_SET];

  /* Base address of the fuse that defines anti rollback feature for current image set*/
  const uint32 * const feature_enable_base_address;

  /* Bit Mask that should be applies to the fuse that defines anti rollback feature for current image set*/
  const uint32 feature_enable_mask;

  /* Base address of the fuse that defines the LSB of version number of current image set*/
  const uint32 * const version_lsb_base_address;

  /* Bit Mask that should be applies to the LSB fuse to extract LSB of version number*/
  const uint32 version_lsb_mask;

  /* Bit Shift that should be applies to the LSB fuse to extract LSB of version number*/
  const uint32 version_lsb_shift;

  /* Base address of the fuse that defines the MSB of version number of current image set*/
  const uint32 * const version_msb_base_address;

  /* Bit Mask that should be applies to the MSB fuse to extract MSB of version number*/
  const uint32 version_msb_mask;

  /* Bit Shift that should be applies to the MSB fuse to extract MSB of version number*/
  const uint32 version_msb_shift;

} boot_version_rollback_img_set;


/*===========================================================================

                FUNCTION DEFINITIONS

===========================================================================*/
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
*    boot_version_rollback_img_set *   - Pointer to entire image set table
*    uint32                            - Number of entries in image set table
*    sec_img_auth_id_type              - Security image id
*    boot_version_rollback_img_set **  - Pointer to matching image set if found
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
                                  boot_version_rollback_img_set ** image_set);


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
                                        boolean * feature_enabled);


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
                                     uint32 * calculated_version);


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
                                    uint64 * fuse_bit_mask);


#endif /*BOOT_VERSION_ROLLBACK_H*/
