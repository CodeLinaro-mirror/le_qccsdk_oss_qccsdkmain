#ifndef BOOT_VERSION_ROLLBACK_IMPL_H
#define BOOT_VERSION_ROLLBACK_IMPL_H

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
04/08/16   ck      Added PBL loaded image rank logic
03/21/16   ck      Initial creation

============================================================================*/
/*===========================================================================

                           INCLUDE FILES

===========================================================================*/
#include "boot_rollback_version.h"

#include "seccommon.h"
#include "sec_img_auth.h"

/*===========================================================================
                FUNCTION DEFINITIONS
===========================================================================*/
#undef memcpy
#undef memset
#undef memcmp

#define LOADED_IMAGE_RANK_TRIAL      0x36
#define LOADED_IMAGE_RANK_CURRENT    0x37
#define LOADED_IMAGE_RANK_GOLDEN     0x38


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
*    sec_img_auth_id               - Security image id
*    secboot_hw_ftbl_type *        - Pointer to secboot hw function pointer table
*    secboot_verified_info_type *  - Verified info from secboot auth (if available),
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
                                     const secboot_verified_info_type * const secboot_verified_info);


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
*    sec_img_auth_id               - Security image id
*    secboot_hw_ftbl_type *        - Pointer to secboot hw function pointer table
*    secboot_verified_info_type *  - Verified info from secboot auth (if available),
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
                                  const secboot_verified_info_type * const secboot_verified_info);

/*===========================================================================

**  Function : boot_rollback_fuse_version_save

** ==========================================================================
*/
/*!
*
* @brief
*    This function save image version number in fuse
*
* @param[in]
*    fuse_value      - new image security version
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
boot_rollback_fuse_version_save(uint64 fuse_value);

#endif /*BOOT_VERSION_ROLLBACK_IMPL_H*/
