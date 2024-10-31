/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#ifndef SEC_IMG_COMMON_H
#define SEC_IMG_COMMON_H

/**
@file seccommon.h
@brief Secure Image Authentication 
*/

/*===========================================================================

                            EDIT HISTORY FOR FILE

when       who      what, where, why
--------   ---      ------------------------------------
10/26/15   SM       Adapted for IOT
15/08/14   mm       Adapted for 64 bit 
10/26/13   mm       Adapted for Boot ROM 
02/20/12   vg       Ported from TZ PIL.

===========================================================================*/

/*----------------------------------------------------------------------------
 * Include Files
 * -------------------------------------------------------------------------*/
#include <sec_comdef.h>
#include "miprogressive.h"
/*----------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * -------------------------------------------------------------------------*/

#define SEC_IMG_AUTH_ERROR_TYPE_UIE_BASE  (0x150)

/* The first 0x100 values are reserved for secure boot failures. Please refer secboot.h */
typedef enum sec_img_auth_error_type
{
  SEC_IMG_AUTH_SUCCESS = 0x0,
  SEC_IMG_AUTH_FAILURE = 0x101,
  SEC_IMG_AUTH_INVALID_ARG = 0x102,
  SEC_IMG_AUTH_OUT_OF_RANGE = 0x103,
  SEC_IMG_ELF_HEADERS_INVALID_DIGEST = 0x104,
  SEC_IMG_AUTH_SEGMENT_INVALID_DIGEST = 0x105,
  SEC_IMG_AUTH_NO_MEMORY = 0x106,
  SEC_IMG_AUTH_INVALID_DIGEST_LEN = 0x107,
  SEC_IMG_AUTH_GET_SW_VERSION_FAILED = 0x108,
  SEC_IMG_AUTH_GET_SW_ID_FAILED = 0x109,
  SEC_IMG_AUTH_GET_CODE_SEGMENT_FAILED = 0x10A,
  SEC_IMG_AUTH_INCORRECT_HASH_LEN = 0x10B,
  SEC_IMG_AUTH_INCORRECT_HASH_TOTAL_LEN = 0x10C,
  SEC_IMG_AUTH_CORRUPTED_IMAGE = 0x10D,
  SEC_IMG_AUTH_MI_BOOT_INVALID = 0x10E,
  SEC_IMG_AUTH_INVALID_ELF = 0x10F,
  SEC_IMG_AUTH_STATE_INVALID = 0x110,
  SEC_IMG_AUTH_MI_BOOT_INVALID_SIZE = 0x111,
  SEC_IMG_AUTH_ELF_INCORRECT_MAGIC = 0x112,
  SEC_IMG_AUTH_ELF_INVALID_CLASS = 0x113,
  SEC_IMG_AUTH_INVALID_VERSION = 0x114,
  SEC_IMG_AUTH_ELF_INVALID_EHSIZE = 0x115,
  SEC_IMG_AUTH_ELF_INVALID_PHENTSIZE = 0x116,
  SEC_IMG_AUTH_PROG_TBL_INVALID_SIZE = 0x117,
  SEC_IMG_AUTH_HASH_SEG_INVALID_SIZE = 0x118,
  SEC_IMG_AUTH_ELF_HDR_INVALID_SIZE = 0x119,
  SEC_IMG_AUTH_NOT_INITIALIZED = 0x11A,
  SEC_IMG_AUTH_ELF_INVALID_NUM_SEGMENTS = 0x11B,
  SEC_IMG_AUTH_INVALID_IMG_ID = 0x11C,
  SEC_IMG_AUTH_VALIDATE_IMAGE_AREA_INVALID_SEGMENT = 0x11D,
  SEC_IMG_AUTH_VALIDATE_IMAGE_AREA_INTEGER_OVERFLOW = 0x11E,
  SEC_IMG_AUTH_MI_BOOT_INVALID_CERT_CHAIN_SIZE = 0x11F,
  SEC_IMG_AUTH_HASH_SEG_TOTAL_INVALID_SIZE = 0x120,
  SEC_IMG_AUTH_SIG_INVALID_SIZE = 0x121,
  SEC_IMG_AUTH_MIHDR_NOT_ALIGNED = 0x122,
  SEC_IMG_AUTH_VALIDATE_IMAGE_AREA_INVALID_ENTRY= 0x123,
  SEC_IMG_AUTH_CERT_INVALID_SIZE = 0x124,
  SEC_IMG_AUTH_HASH_SEG_ARRAY_INVALID_SIZE = 0x125,
  SEC_IMG_AUTH_INCORRECT_HASH_TABLE_SEGMENTS_COUNT = 0x126,
  SEC_IMG_AUTH_ENCRYPTION_PARAMS_INVALID_SIZE =0x127,
  SEC_IMG_AUTH_ENCRYPTION_PARAMS_INVALID_MAGIC = 0x128,
  SEC_IMG_AUTH_ENCRYPTION_PARAMS_INVALID_VERSION = 0x129,
  SEC_IMG_AUTH_ENCRYPTION_INVALID_SIZE = 0x12A,
  SEC_IMG_AUTH_ENCRYPTION_PARAMS_INVALID_SOURCE_ID = 0x12B,
  SEC_IMG_AUTH_ENCRYPTION_PARAMS_INVALID_KEY_LADDER_LEN = 0x12C,
  SEC_IMG_AUTH_INVALID_ENCRYPTION_KEY_ID = 0x12D,
  SEC_IMG_AUTH_INVALID_IMAGE_ID = 0x12E,
  SEC_IMG_AUTH_INVALID_ENC_PRARM_SIZE = 0x12F,
  SEC_IMG_AUTH_INTEGER_OVERFLOW = 0x12E,
  SEC_IMG_AUTH_CE_BIST_FAILURE = 0x130,
  SEC_IMG_AUTH_CE_TIMEOUT = 0x131, 
  SEC_IMG_AUTH_CE_INIT_FAILURE = 0x132,

  /* Error codes reserved for UIE, from SEC_IMG_AUTH_ERROR_TYPE_UIE_BASE (0x150) to 0x1FF */

  SEC_IMG_AUTH_MAX                    = 0x7FFFFFFF /**< Force to 32 bits */
}sec_img_auth_error_type;


typedef enum sec_img_auth_id_type
{
  SEC_IMG_AUTH_SBL_IMG = 0x0,             /**<-- SBL Image */
  SEC_IMG_AUTH_APP_IMG = 0x1,			  /**<-- APP Image */	
  SEC_IMG_AUTH_SBL_GOLD_IMG = 0x2,        /**<-- Golden SBL Image */
  SEC_IMG_AUTH_APP_GOLD_IMG = 0x3,        /**<-- Golden APP Image */
  //SEC_IMG_AUTH_M4_PATCH_IMG = 0x6,       /**<-- ROM Patching Image */ 
  SEC_IMG_AUTH_UNSUPPORTED_IMG = 0x7FFFFFFF,          /**<-- Processor not Supported */
  /**<-- No of Processors supported */
  SEC_IMG_AUTH_NUM_SUPPORTED_IMG = SEC_IMG_AUTH_UNSUPPORTED_IMG 
}sec_img_auth_id_type;

/**
 * @brief type definition for the crypto driver APIs
 */
typedef struct crypto_ctx_type {
  uint8    *ctx_imem;  // pointer to crypto internal memory, assigned by caller for crypto operation
  uint32   ctx_imem_size;   // size of the crypto internal memory
  boolean  selftest; // enabling this flag slowers the crypto perf (required by pbl cmu)
} crypto_ctx_type;

/**
 * @brief Function table containing pointers to the crypto driver APIs
 */
typedef struct crypto_ftbl_type {
    crypto_ctx_type crypto_ctx;
    uint32 (*HashInit) (crypto_ctx_type* ctx, uint32 hash_algo);  // initialize hash crypto setup
    uint32 (*HashUpdate) (crypto_ctx_type* ctx, const void *data, uint32 data_len); // update hash data
    uint32 (*HashFinal) (crypto_ctx_type* ctx, void* digest_ptr, uint32 digest_len); // return hash digest
    uint32 (*HashDeInit) (crypto_ctx_type* ctx);                  // deinitialize hash crypto setup
    uint32 (*CryptoInit)(crypto_ctx_type* crypto_ctx);   // initialize crypto enviromnent setup at the start of 
                                                         // secimgauth lib if needed
    // Brief: crypto function for RSA mod exp operation
    uint32 (*ModExp) (
                    crypto_ctx_type* ctx,                // the crypto context
                    uint32 mod_size,                     // the mod size
                    uint32 exp_size,                     // the exp size
                    uint8* mod,                          // the ptr to mod
                    uint8* exp,                          // the ptr to exp
                    uint8* message,                      // the ptr to message
                    uint8* r);                           // the ptr to return value
}crypto_ftbl_type;


#endif
