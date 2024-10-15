/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#ifndef SBL_AUTH_H
#define SBL_AUTH_H
 /**===========================================================================
 **
 **                        PRIMARY BOOT LOADER
 **                        -------------------
 ** FILE
 **     sbl_auth.h
 **
 ** GENERAL DESCRIPTION
 **     This header file contains declarations and definitions for the SBL
 **     interface to the software that authenticates the code image from external
 **     source (ie: Flash).
 **
 **==========================================================================*/




/******************************************************************************
                                MODULE INCLUDES
                         ADD NEW ONES UNDER THIS LINE
******************************************************************************/
#include "pbl_auth.h"

#undef memcpy
#undef memset
#undef memcmp

/******************************************************************************
                         PUBLIC TYPES and TYPE-DEFINES
                         ADD NEW ONES UNDER THIS LINE
******************************************************************************/


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
 **     DataType : secboot_auth_image_info_s *
 **     Param    : image_info
 **                Pointer to image_info
 **     Type     : [IN]
 **     DataType : boot_apps_shared_data_type *
 **     Param    : pbl_shared
 **                Pointer to PBL shared data

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
uint32 sbl_image_auth
(
  secboot_auth_image_info_t *image_info,
  boot_apps_shared_data_t *pbl_shared
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
 **                PBL_NO_ERR - success
 **
 ** SIDE EFFECTS
 **     None.
 **
 **==========================================================================*/
uint32 sbl_compute_verify_hash(secboot_auth_image_info_t *image_info, boot_apps_shared_data_t *pbl_shared);

#endif  /* SBL_AUTH_H */
/*=============================================================================
                                  END OF FILE
=============================================================================*/
