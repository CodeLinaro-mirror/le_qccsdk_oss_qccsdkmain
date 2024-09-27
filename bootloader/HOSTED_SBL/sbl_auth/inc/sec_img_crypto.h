/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#ifndef SECBOOT_CRYPTO_H
#define SECBOOT_CRYPTO_H
/*****************************************************************************
*
* @file  secboot_crypto.h (Secboot Crypto API)
*
* @brief API to read Security Control Fuses containing authentication
*        information
* Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*
*****************************************************************************/

/*===========================================================================

                           EDIT HISTORY FOR FILE

This section contains comments describing changes made to this file.
Notice that changes are listed in reverse chronological order.

when       who     what, where, why
--------   ---     ----------------------------------------------------------
10/26/15   SM      Adapted for IOT
07/02/15   hw      Init Version
============================================================================*/
/** @ingroup SecureMSM
 *  @{
 */

/** @defgroup SecbootHardware Secboot Hardware Library */

/**
 * @addtogroup SecbootHardware
 *
 * @{
 */

#include "sec_comdef.h"
#include "seccommon.h"

#define MEMCPY  memcpy
#define MEMSET  memset
#define MEMCMP  memcmp
#define CRYPTO_HASH_IMEM_DATA_BYTE_SIZE (2048)
#define CRYPTO_DIGEST_BYTE_SIZE_SHA1    (20)
#define CRYPTO_DIGEST_BYTE_SIZE_SHA256  (32)

#define CRYPTO_HASH_ALGO_SHA1 (0x1)
#define CRYPTO_HASH_ALGO_SHA256 (0x2)

/**
 Identifies the error type returned by the API.
 */
typedef enum
{
  E_SECBOOT_CRYPTO_SUCCESS                = 0,   /**< Operation was successful. */
  E_SECBOOT_CRYPTO_FAILURE                = 1,   /**< General failure. */
  E_SECBOOT_CRYPTO_INVALID_PARAM          = 2,   /**< Invalid parameter passed into function. */
  E_SECBOOT_CRYPTO_HASH_INIT_FAIL         = 3,   /**< crypto hash init fail. */
  E_SECBOOT_CRYPTO_HASH_UPDATE_FAIL       = 4,   /**< crypto hash update fail. */
  E_SECBOOT_CRYPTO_HASH_FINAL_FAIL        = 5, /**< crypto hash final fail */
  E_SECBOOT_CRYPTO_HASH_DEINIT_FAIL       = 6,   /**< crypto hash deinit fail. */
  E_SECBOOT_CRYPTO_INVALID_CTX_SIZE_FAIL  = 7,   /**< ctx size is not sufficient. */
  E_SECBOOT_CRYPTO_DECRYPT_CCM_FAIL       = 8,   /**< crypto decrypt key fail. */
  E_SECBOOT_CRYPTO_INIT_TIME_OUT          = 9,   /**< crypto init timeout fail. */
  E_SECBOOT_CRYPTO_DECRYPT_LOAD_KEY_FAIL  = 10,   /**< fail to load decryption key. */
  E_SECBOOT_CRYPTO_CTX_INIT_FAIL          = 11,   /**< crypto context init fail. */
  E_SECBOOT_CRYPTO_UNSUPPORTED_KEY_LEVEL  = 12,   /**< unsupported key level type. */
  E_SECBOOT_CRYPTO_DECRYPT_CBC_FAIL       = 13,   /**< crypto decrypt cbc fail. */
  E_SECBOOT_CRYPTO_DECRYPT_UNLOAD_KEY_FAIL= 14,   /**< fail to unload decryption key. */
  E_SECBOOT_CRYPTO_OVERFLOW_FAIL          = 15,   /**< overflow error. */
  E_SECBOOT_CRYPTO_RSA_MODEXP_FAIL        = 16,   /**< rsa modexp operation error. */
  E_SECBOOT_CRYPTO_HASH_MAX               = 0x7FFFFFFF /**< Force to 32 bits */
} SECBOOT_CRYPTO_ERR_TYPE;

/**
 * @brief This function initializes the CE. Context memory must be assigned
 *        and passed in outside the call.
 *
 * @param[in] ctx   The context
 * @param[in] hash_algo The hash algorithm type (CRYPTO_HASH_ALGO_SHA1/CRYPTO_HASH_ALGO_SHA256)
 * @param[in] ctx_byte_size The context byte size
 *
 * @return - 0 on success, error code SECBOOT_CRYPTO_ERR_TYPE on failure.
 *
 */
uint32 hashInit(crypto_ctx_type* ctx, uint32 hash_algo);

/**
 * @brief This function will hash data into the hash context
 *        structure, which must have been initialized by
 *        HashInit.
 *
 * @param[in] ctx   The context
 * @param[in] data  Pointer to input message to be hashed
 * @param[in] data_len The message byte size
 *
 * @return - 0 on success, error code SECBOOT_CRYPTO_ERR_TYPE on failure.
 *
 */
uint32 hashUpdate(crypto_ctx_type* ctx, const void *data, uint32 data_len);

/**
 * @brief Compute the final digest hash value.
 *
 * @param[in] ctx   The context
 * @param[in] digest_ptr  Pointer to digest data
 * @param[in] digest_len The digest byte size
 *
 * @return - 0 on success, error code SECBOOT_CRYPTO_ERR_TYPE on failure.
 *
 */
uint32 hashFinal(crypto_ctx_type* ctx, void* digest_ptr, uint32 digest_len);

/**
 * @brief Deintialize a cipher context
 *
 * @param[in] ctx   The context
 *
 * @return - 0 on success, error code SECBOOT_CRYPTO_ERR_TYPE on failure.
 *
 */
uint32 hashDeInit(crypto_ctx_type* ctx);

/**
 * @brief Initialize Crypto
 *
 * @return - 0 on success, error code SECBOOT_CRYPTO_ERR_TYPE on failure.
 *
 */
uint32 cryptoInit(crypto_ctx_type* crypto_ctx);


/**
 * @brief decrypt the message in RSA with given mod and exp.
 *        mod and exp are in big endian for sp, as sp use hw based rsa,
 *        which operates in big endian. apps and modem use sw based rsa,
 *        which works in little endian.
 *
 * @param[in]  crypto_ctx crypto context
 * @param[in]  mod_size the modsize (byte size), the same as the return value size
 * @param[in]  exp_size the exp size (byte size)
 * @param[in]  mod the ptr to mod
 * @param[in]  exp the ptr to exp
 * @param[in]  message the ptr to message
 * @param[out]  r the ptr to return value
 *
 * @return - 0 on success, error code SECBOOT_CRYPTO_ERR_TYPE on failure.
 *
 */
uint32 ModExp
(
  crypto_ctx_type* ctx,
  uint32 mod_size,
  uint32 exp_size,
  uint8* mod,
  uint8* exp,
  uint8* message,
  uint8* r);



/// @}
//
/// @}
#endif //SECBOOT_CRYPTO_H
