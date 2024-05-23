/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#ifndef PBL_IMAGE_AUTH_H
#define PBL_IMAGE_AUTH_H
 /**===========================================================================
 **
 **                        PRIMARY BOOT LOADER
 **                        -------------------
 ** FILE
 **     pbl_auth.h
 **
 ** GENERAL DESCRIPTION
 **     This header file contains declarations and definitions for the PBL
 **     interface to the software that authenticates the code image from external
 **     source (ie: Flash).
 **
 **==========================================================================*/


/*===========================================================================

                           EDIT HISTORY FOR FILE

  This section contains comments describing changes made to this module.
  Notice that changes are listed in reverse chronological order.

  when       who            what, where, why
  --------   ----------     ---------------------------------------------------

============================================================================*/


/******************************************************************************
                                MODULE INCLUDES
                         ADD NEW ONES UNDER THIS LINE
******************************************************************************/


/******************************************************************************
                             PUBLIC MACROS/DEFINES
                         ADD NEW ONES UNDER THIS LINE
******************************************************************************/
#define SECBOOT_IMG_AUTH_SBL_IMG       0x0 			/**<-- SBL Image */
#define SECBOOT_IMG_AUTH_APP_IMG       0x1			/**<-- APP Image */
#define SECBOOT_IMG_AUTH_SBL_GOLD_IMG  0x2		/**<-- Golden SBL Image */
#define SECBOOT_IMG_AUTH_APP_GOLD_IMG  0x3		/**<-- Golden APP Image */

/******************************************************************************
                         PUBLIC TYPES and TYPE-DEFINES
                         ADD NEW ONES UNDER THIS LINE
******************************************************************************/
/* Defines the main data structure that is shared
   with the APPs SBL image. */
typedef struct secboot_auth_image_info
{
	boot_elf_loader * elf_loader_ptr;
	uint32 image_id;
	uint32 image_rank;
} secboot_auth_image_info_t;


/******************************************************************************
                            PUBLIC DATA DECLARATION
                         ADD NEW ONES UNDER THIS LINE
******************************************************************************/


/******************************************************************************
                         PUBLIC FUNCTION DECLARATIONS
                         ADD NEW ONES UNDER THIS LINE
******************************************************************************/


/**===========================================================================
 **
 ** FUNCTION DESCRIPTION
 **     This helper function returns if there is a valid secure image in FWD.
 **
 ** DEPENDENCIES
 **     None
 **
 ** PARAMETERS
 **     Type     : [IN]
 **     DataType : boot_elf_loader *
 **     Param    : elf_loader_ptr
 **                Pointer to ELF header

 **
 **
 ** RETURN VALUE
 **     DataType : uint32
 **     Value    :
 **                PBL_NO_ERR - success
 **
 **
 ** SIDE EFFECTS
 **     None.
 **
 **==========================================================================*/
uint32 pbl_image_auth
(
  secboot_auth_image_info_t * image_info
);


/**===========================================================================
 **
 ** FUNCTION DESCRIPTION
 **     This function compute/verify the hashes for ELF image
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
 **                PBL_NO_ERR - success
 **
 ** SIDE EFFECTS
 **     None.
 **
 **==========================================================================*/
uint32 pbl_compute_verify_hash(void);

/**===========================================================================
**
** FUNCTION DESCRIPTION
**     This function write to debug overrite sticky bit
**
** DEPENDENCIES
**     None
**
** PARAMETERS
**     Type     : [IN]
**     DataType : uint32
**     Param    : debug_override
**
**
**
** RETURN VALUE
**     None
**
**
** SIDE EFFECTS
**     None.
**
**==========================================================================*/
void pbl_write_debug_override_sticky_bits(void);


/**===========================================================================
**
** FUNCTION DESCRIPTION
**     This function write to anti-rollback lock sticky bit
**
** DEPENDENCIES
**     None
**
** PARAMETERS
**     Type     : [IN]
**     DataType : uint32
**     Param    : debug_override
**
**
**
** RETURN VALUE
**     None
**
**
** SIDE EFFECTS
**     None.
**
**==========================================================================*/
void pbl_write_anti_rollback_lock_sticky_bits(void);

/**===========================================================================
 **
 ** FUNCTION DESCRIPTION
 **     This function will save relevant data to be passed to SBL
 **     Other functions which have data to be shared in this structure must
 **     not be called prior to this initialization process.
 **
 ** DEPENDENCIES
 **     None
 **
 ** PARAMETERS
 **
 ** RETURN VALUE
 **     DataType : uint32
 **     Value    :
 **                PBL_NO_ERR if successful.
 **
 ** SIDE EFFECTS
 **     shared_data of the PBL state is initialized.
 **
 **==========================================================================*/
uint32 pbl_populate_share_data(void);


typedef struct {
	uint32 (*pbl_image_auth_pfn)(secboot_auth_image_info_t *image_info);
	uint32 (*pbl_compute_verify_hash_pfn)(void);
	void (*pbl_write_debug_override_sticky_bits_pfn)(void);
	void (*pbl_write_anti_rollback_lock_sticky_bits_pfn)(void);
	uint32 (*pbl_populate_share_data_pfn)(void);
} pbl_image_auth_ind_t;

#endif  /* PBL_IMAGE_AUTH_H */
/*=============================================================================
                                  END OF FILE
=============================================================================*/
