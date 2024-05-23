/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/

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
#include "sec_img_auth.h"
#include "boot_elf_loader.h"
#include "pbl_image_auth.h"

/******************************************************************************
                             PUBLIC MACROS/DEFINES
                         ADD NEW ONES UNDER THIS LINE
******************************************************************************/
#define PBL_AUTH_NO_ERR        (0x0)

typedef enum
{
   PBL_AUTH_ELF_ERR       = 0x1000000,
   PBL_AUTH_SEC_HW_ERR    = 0x2000000,
   PBL_AUTH_SEC_IMG_ERR   = 0x3000000,
   PBL_AUTH_SECBOOT_ERR   = 0x4000000,
   PBL_AUTH_FORCE32BITS    = 0x7FFFFFFF
}pbl_auth_block_type;

typedef enum
{
   PBL_AUTH_ELF_NO_ERR               = 0x0000,
   PBL_AUTH_ELF_NULL_PTR_ERR		 = 0x0100,   
   PBL_AUTH_ELF_RANGE_SIZE_ERR       = 0x0200,
   PBL_AUTH_ELF_INVALID_HDR_ERR      = 0x0300,
   PBL_AUTH_ELF_CLASS_FORMAT_ERR     = 0x0400,   
   PBL_AUTH_ELF_HASH_SEGMENT_ERR     = 0x0500,
   PBL_AUTH_ELF_ALIGNMENT_ERR        = 0x5000000,
   PBL_AUTH_ELF_FORCE32BITS              = 0x7FFFFFFF
}pbl_auth_elf_type;

typedef enum
{
   PBL_AUTH_SEC_HW_NO_ERR    = 0x0000,
   PBL_AUTH_SEC_HW_MISC_ERR	 = 0x0100,  
   PBL_AUTH_SEC_HW_FTBL_ERR	 = 0x0200,	    
   PBL_AUTH_SEC_HW_FORCE32BITS      = 0x7FFFFFFF
}pbl_auth_secboot_hw_type;

typedef enum
{
   PBL_AUTH_SEC_IMG_NO_ERR           = 0x0000,
   PBL_AUTH_SEC_IMG_NULL_PTR_ERR     = 0x0100,   
   PBL_AUTH_SEC_IMG_MISC_ERR         = 0x0200,
   PBL_AUTH_SEC_IMG_AUTH_ERR         = 0x0300,
   PBL_AUTH_SEC_IMG_FTBL_ERR         = 0x0400,
   PBL_AUTH_SEC_IMG_HASH_ERR		 = 0x0500,   
   PBL_AUTH_SEC_IMG_FORCE32BITS              = 0x7FFFFFFF
}pbl_auth_secboot_image_type;

typedef enum
{
   PBL_AUTH_SEBOOT_NO_ERR            = 0x0000,
   PBL_AUTH_SECBOOT_NULL_PTR_ERR     = 0x0100,   
   PBL_AUTH_SECBOOT_MISC_ERR         = 0x0200,
   PBL_AUTH_SECBOOT_FTBL_ERR		 = 0x0300,   
   PBL_AUTH_SECBOOT_FORCE32BITS              = 0x7FFFFFFF
}pbl_auth_secboot_type;

/******************************************************************************
                         PUBLIC TYPES and TYPE-DEFINES
                         ADD NEW ONES UNDER THIS LINE
******************************************************************************/
typedef struct pbl_auth_state
{
  boolean			auth_enabled;                     /* TRUE if authentication is enabled */ 
  boolean           hash_integrity_check_disabled;    /* TRUE if disable hash integrity check when secboot disabled*/
  boolean			debug_override;              /* M4F debug override */
  boolean           anti_rollback_unlock;        /*anti-rollback unlock*/
  uint64            sw_id;                       /*software ID*/  
} pbl_auth_state_t;

/* define the struct of PBL authentication */
typedef struct pbl_auth_handle
{
    uint8                         *  hash_segment_start_address; /* Pointer to the Hash segment */
    uint32                           hash_segment_size;          /* size of hash segment buffer */
	sec_img_auth_priv_t              sec_img_handle;             /* secboot image authentication handler*/
	sec_img_auth_verified_info_s	 auth_verified_info __attribute__((aligned(64))); 	     /* auth verified info */
	pbl_auth_state_t                 auth_state;                 /* PBL auth state */
}pbl_auth_handle_t;

typedef struct pbl_hash_handle
{
    uint8 hash_table[MI_PBT_MAX_SEGMENTS*CRYPTO_DIGEST_BYTE_SIZE_SHA256];
    uint8 crypto_imem[CRYPTO_HASH_IMEM_DATA_BYTE_SIZE];
	crypto_ftbl_type crypto_ftbl;
	pbl_auth_state_t auth_state;
}pbl_hash_handle_t;

/* Defines the main data structure that is shared
   with the APPs SBL image. */

typedef struct boot_apps_shared_data
{ 
  pbl_auth_state_t auth_state;
  uint32 (*pbl_get_image_hash_handle)(pbl_hash_handle_t *handle);
  uint32 (*secimg_auth)(secboot_auth_image_info_t * image_info);
  sec_img_auth_ftbl_type       sec_img_auth_ftbl;
} boot_apps_shared_data_t;

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
