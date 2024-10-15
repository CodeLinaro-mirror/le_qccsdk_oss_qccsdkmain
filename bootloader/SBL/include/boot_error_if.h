/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/



#ifndef BOOT_ERROR_IF_H
#define BOOT_ERROR_IF_H

/*===========================================================================

                 Boot Loader Error Handler Header File

GENERAL DESCRIPTION
  This header file contains declarations and definitions for the boot
  error handler interface.

Copyright 2023 by QUALCOMM Technologies Incorporated.  All Rights Reserved.
============================================================================*/

/*===========================================================================

                           EDIT HISTORY FOR FILE

This section contains comments describing changes made to this file.
Notice that changes are listed in reverse chronological order.


when       who     what, where, why
--------   ---     ----------------------------------------------------------
10/07/23   BingHan      Initial revision

============================================================================*/

/*===========================================================================

                           INCLUDE FILES

===========================================================================*/
#include <stdint.h>
#include "boot_print.h"

/*===========================================================================

                      PUBLIC DATA DECLARATIONS

===========================================================================*/
typedef void (*boot_error_handler_ptr_type)
(
  const char* filename_ptr,     /* File this function was called from -
                                   defined by __FILE__ */
  uint32_t      line,             /* Line number this function was called
                                   from - defined by  __LINE__ */
  uint32_t      error             /* Enum that describes error type */
);

/*---------------------------------------------------------------------------
  Define the SBL ERROR types. This classifies different types of errors
  encountered in the SBL.
 ---------------------------------------------------------------------------*/
typedef enum
{
  BL_ERR_NONE = 0,                  /* No BL ERR, SUCCESS, PASS */
  BL_ERR_NULL_PTR,                  /* Null pointer encountered */
  BL_ERR_INVALID_PARAM,

  BL_ERR_ELF_MEM_NULL_PTR,              /* Null parameter to ELF MEM layer */
  BL_ERR_ELF_MEM_INIT,					/* ELF mem init err */
  BL_ERR_ELF_MEM_READ,					/* ELF mem read err */
  BL_ERR_ELF_MEM_REGION,				/* ELF mem invalid region */
  BL_ERR_ELF_MEM_INVAL_PARAM,			/* ELF mem invalid param */

  BL_ERR_ELF_NULL_PTR,              /* Null parameter to ELF loader */
  BL_ERR_ELF_INVAL_PARAM,           /* Otherwise invalid param to ELF loader */
  BL_ERR_ELF_MEM,                   /* MEM-related error in ELF loader */
  BL_ERR_ELF_PARSE,                 /* Parse error in ELF loader */
  BL_ERR_ELF_FORMAT,                /* ELF file format error in ELF loader */
  BL_ERR_ELF_HASH_TABLE_MISSING,    /* Missing ELF hash segment */
  BL_ERR_ELF_CLASS_INVALID,         /* ELF class not valid (32 or 64 bit) */
  BL_ERR_ELF_NO_HASH_SEG_FOUND,     /* No hash segment found in ELF image */
  BL_ERR_ELF_SEGMENT_COUNT,         /* Eclipsed supported ELF segment count */
  BL_ERR_ELF_HASH_MISMATCH,			/* ELF HASH miss match */
  BL_ERR_ELF_MULTI_HASH_SEG_FOUND,  /* Multiple hash segment found in */
  BL_ERR_ELF_WHITELIST_CHECK,       /* Whitelist has segments that overlap */

  BL_ERR_RRAM_DXE_INVALID_ADDR,        /* RRAM DXE invalid address */
  BL_ERR_RRAM_DXE_INVALID_BLOCK_ADDR,  /* RRAM DXE invalid block address */

  BL_ERR_FDT_NULL_PTR,
  BL_ERR_FDT_INVALID_ADDR,
  BL_ERR_FDT_INVALID_SIGNATURE,
  BL_ERR_FDT_INVALID_ENTRY_NUM,
  BL_ERR_FDT_INVALID_PARAM,
  BL_ERR_FDE_INVALID_IMAGE_ID,
  BL_ERR_FDE_INVALID_IMAGE_ADDR,
  BL_ERR_FDE_INVALID_IDX,
  BL_ERR_FDE_UPDATE_ERR,

  BL_ERR_IMG_AUTH_NULL_PTR,              /* Null parameter to image authentication layer */
  BL_ERR_IMG_AUTH_INVAL_PARAM, 			 /* Invalid param to image authentication */
  BL_ERR_IMG_SECURITY_FAIL, 		    /* Image authentication  or Decryption failed */
  BL_ERR_IMG_META_DATA_AUTH_FAIL,       /* Failed in authentication of image meta data */
  BL_ERR_AUTH_ELF_HDR_FAIL,             /* Failed in authentication of elf header */
  BL_ERR_ROLLBACK_IMAGE_SET_NOT_FOUND,  /* Image sw type not found in image table */
  BL_ERR_ROLLBACK_VERSION_VERIFY_FAIL,  /* Error during version rollback check */
  BL_ERR_ROLLBACK_VERSION_BLOW_FAIL,    /* Error blowing version numbers*/
  BL_ERR_ROLLBACK_OPEN_VERSION_FUSE_NOT_FOUND,	/* Open fuse for updating version could not be found */

  BL_ERR_SBL_OTA_NULL_PTR,                   /* SBL OTA NULL ptr error, start from 38*/
  BL_ERR_SBL_OTA_INVAL_PARAM,                /* SBL OTA invalid param*/
  BL_ERR_SBL_OTA_GET_FDT_ERR,
  BL_ERR_SBL_OTA_PICK_FDE_ERR,                /* SBL OTA pick error*/
  BL_ERR_SBL_OTA_UPDATE_FDE_ERR,
  BL_ERR_SBL_OTA_AGE_FDE_ERR,
  BL_ERR_SBL_OTA_HANDLE_TRIAL_FDE_ERR,
  BL_ERR_SBL_OTA_HANDLE_CURR_FDE_ERR,
  BL_ERR_SBL_OTA_HANDLE_FDE_ERR,
  BL_ERR_SBL_OTA_PROCESS_ERR,
  BL_ERR_SBL_OTA_UPDATE_PATCH_ERR,
  BL_ERR_SBL_OTA_GET_PATCH_ERR,



  BL_ERR_BOOT_HDR_NULL_PTR,
  BL_ERR_BOOT_HDR_SUBTYPE_HDR_NONE,

    /* Cortex M4 Additions */
  BL_ERR_FAULT_NMI,                   /* NMI exception occurred */
  BL_ERR_FAULT_HARD,                  /* Hard fault exception occurred */
  BL_ERR_FAULT_MEMORY_MANAGEMENT,     /* Memory Management fault exception occurred */
  BL_ERR_FAULT_BUS,                   /* Bus fault exception occurred */
  BL_ERR_FAULT_USAGE,                 /* Usage fault exception occurred */

  BL_ERR_EXEP_UNDEF_INSTR,               /* Undefined Instruction exception occurred */
  BL_ERR_EXEP_SWI,                       /* Software Interrupt (SWI) exception occurred */
  BL_ERR_EXEP_PREFETCH_ABORT,            /* Prefetch abort exception occurred */
  BL_ERR_EXEP_DATA_ABORT,                /* Data abort exception occurred */
  BL_ERR_EXEP_RESERVED_HANDLER,          /* Reserved exception occurred */
  BL_ERR_EXEP_IRQ,                       /* IRQ exception occurred */

  /* add above this enum */
  BL_ERR_MAX = 0x7FFFFFFF
} bl_error_type;


void boot_log_err(bl_error_type type, const char *file, uint32_t line);
void pbl_exit_configuration(void);

#ifdef P_DEBUG_PRINT_LOG
void boot_log_show();
#define	BOOT_LOG_SHOW()		boot_log_show();
#else
#define	BOOT_LOG_SHOW()
#endif

/*===========================================================================

                      PUBLIC FUNCTION DECLARATIONS

===========================================================================*/

/*===========================================================================

**  Macro :  bl_verify

** ==========================================================================
*/
/*!
*
* @brief
*   Given a boolean expression, this macro will verify the condition is TRUE
*   and do nothing if the condition is TRUE, otherwise it will call the
*   SBL error handler.
*
* @par Dependencies
*   None
*
* @retval
*   None
*
* @par Side Effects
*   This macro never returns if the condition is FALSE.
*
*/
/*The forever loop after error_handler will never be executed, it is added to fix klockwork warning*/
#define BL_VERIFY( xx_exp, error_type ) \
            do { \
               if( !(xx_exp) ) \
               { \
				 boot_log_err(error_type, __FILE__, __LINE__);\
				 BOOT_LOG_SHOW() \
				 pbl_exit_configuration(); \
                 while(1) \
                 { \
                 } \
               } \
            } while(0)

/*===========================================================================

**  Macro :  bl_err_fatal

** ==========================================================================
*/
/*!
*
* @brief
*   This macro calls the error handler.
*
* @par Dependencies
*   None
*
* @retval
*   None
*
* @par Side Effects
*   This macro never returns.
*
*/
/*The forever loop after error_handler will never be executed, it is added to fix klockwork warning*/
#define BL_ERR_FATAL( error_type )  \
            do{ \
				boot_log_err(error_type, __FILE__, __LINE__);\
				BOOT_LOG_SHOW() \
                while(1) \
                { \
                } \
              } while (0)

#endif  /* BOOT_ERROR_IF_H */
