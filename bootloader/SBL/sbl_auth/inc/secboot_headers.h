/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/
#ifndef BOOT_HEADERS_H
#define BOOT_HEADERS_H

/*===========================================================================

  boot_headers.h

DESCRIPTION
  This header file contains declarations and type definitions for the
  configuration data stored in external flash and used by the boot ROM
  code.

============================================================================*/

/*===========================================================================

                           EDIT HISTORY FOR FILE

This section contains comments describing changes made to this file.
Notice that changes are listed in reverse chronological order.

when       who     what, where, why
--------   ---     ----------------------------------------------------------
2/2/15      da       Changes to expose sec_img_auth functions in M0 shared data
============================================================================*/

/*===========================================================================
                           INCLUDE FILES
===========================================================================*/
#ifndef _ARM_ASM_
//#ifndef M0_BOOT_ROM
#include "secboot.h"
#include "secboot_hw.h"
#include "sec_img_auth.h"
//#endif
#endif

/*===========================================================================
                      PUBLIC DATA DECLARATIONS
===========================================================================*/

/*---------------------------------------------------------------------------
  Magic number definition for identifying start of boot configuration data.
---------------------------------------------------------------------------*/
#define MAGIC_NUM       0x73D71034     /* Used for NOR, NAND, etc. flash. */

/*---------------------------------------------------------------------------
  Magic number definition for identifying PBL shared data.
---------------------------------------------------------------------------*/
#define BOOT_PBL_SHARED_DATA_MAGIC  0x78577020

#ifndef _ARM_ASM_

//#ifdef M4F_BOOT_ROM
/* Image type definition */
typedef enum
{
  NONE_IMG = 0,
  OEM_SBL_IMG,
  AMSS_IMG,
  QCSBL_IMG,
  HASH_IMG,
  APPSBL_IMG,
  APPS_IMG,
  HOSTDL_IMG,
  DSP1_IMG,
  FSBL_IMG,
  DBL_IMG,
  OSBL_IMG,
  DSP2_IMG,
  EHOSTDL_IMG,
  NANDPRG_IMG,
  NORPRG_IMG,
  RAMFS1_IMG,
  RAMFS2_IMG,
  ADSP_Q5_IMG,
  APPS_KERNEL_IMG,
  BACKUP_RAMFS_IMG,
  SBL1_IMG,
  SBL2_IMG,
  RPM_IMG,
  SBL3_IMG,
  TZ_IMG,
  SSD_KEYS_IMG,
  GEN_IMG,

 /******************************************************/
 /* Always add enums at the end of the list. there are */
 /*  hard dependencies on this enum in apps builds     */
 /*  which DONOT SHARE this definition file            */
 /******************************************************/

 /* add above */
  MAX_IMG = 0x7FFFFFFF
}image_type;



/*****************************************************************************
 * This structure is used as the header for the hash data segment in
 * signed ELF images.  The sec_img_auth component of secure boot
 * makes use of the image_size, code_size, signature_size and
 * cert_chain_size elements only.  The reserved structure elements are
 * from an earlier implementation that does not support ELF images.
 * The reserved elements are preserved to minimize the impact on code
 * signing and image creation tools.
******************************************************************************/

typedef struct
{
  uint32 res1;            /* Reserved for compatibility: was image_id */
  uint32 res2;            /* Reserved for compatibility: was header_vsn_num */
  uint32 res3;            /* Reserved for compatibility: was image_src */
  uint32 res4;            /* Reserved for compatibility: was image_dest_ptr */
  uint32 image_size;      /* Size of complete hash segment in bytes */
  uint32 code_size;       /* Size of hash table in bytes */
  uint32 res5;            /* Reserved for compatibility: was signature_ptr */
  uint32 signature_size;  /* Size of the attestation signature in bytes */
  uint32 res6;            /* Reserved for compatibility: was cert_chain_ptr */
  uint32 cert_chain_size; /* Size of the attestation chain in bytes */
} mi_boot_image_header_type;



//#endif /* #ifndef M4F_BOOT_ROM */

typedef struct pbl_secboot_shared_info_type
{
  secboot_verified_info_type pbl_verified_info;    /**<  */
  secboot_ftbl_type          pbl_secboot_ftbl;     /**< Contains pointers to PBL secboot routines */
  secboot_hw_ftbl_type       pbl_secboot_hw_ftbl;  /**< Contains pointers to PBL secboot hw routines */
} pbl_secboot_shared_info_type;

#endif /* _ARM_ASM_ */
#endif  /* BOOT_HEADERS_H */

