
#ifndef PBL_AUTH_H
#define PBL_AUTH_H
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
#include "secboot_headers.h"
#include "sec_img_auth.h"
#include "boot_elf_loader.h"
#include "pbl_image_auth.h"

/******************************************************************************
                             PUBLIC MACROS/DEFINES
                         ADD NEW ONES UNDER THIS LINE
******************************************************************************/

/******************************************************************************
                         PUBLIC TYPES and TYPE-DEFINES
                         ADD NEW ONES UNDER THIS LINE
******************************************************************************/
/* Defines the main data structure that is shared
   with the APPs SBL image. */

/* define the struct of PBL authentication */
typedef struct boot_apps_shared_data_type
{
  /* Contains pointers to PBL secboot hw routines */
  sec_img_auth_ftbl_type       sec_img_auth_ftbl;

  /* Fields below are filled in by auth module */
  pbl_secboot_shared_info_type    *secboot_shared_data;
} boot_apps_shared_data_type;

/******************************************************************************
                            PUBLIC DATA DECLARATION
                         ADD NEW ONES UNDER THIS LINE
******************************************************************************/


/******************************************************************************
                         PUBLIC FUNCTION DECLARATIONS
                         ADD NEW ONES UNDER THIS LINE
******************************************************************************/

#endif  /* PBL_AUTH_H */
/*=============================================================================
                                  END OF FILE
=============================================================================*/
