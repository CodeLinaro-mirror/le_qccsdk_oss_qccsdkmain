/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "securefs.h"
#include "CeCL.h"
#include "CeEL.h"
#include "CeML.h"
#include <string.h>

/*===========================================================================

                 DEFINITIONS AND TYPE DECLARATIONS

===========================================================================*/
#define BLOCK_SIZE 16
#define META_DATA_SIZE 64
#define ZERO_ARRAY_KEY_SIZE 16

#define FREE_IF(x)                                                             \
  if ((x)) {                                                                   \
    CeElfree((x));                                                             \
    (x) = NULL;                                                                \
  }
#define IS_32_MEM_RANGE(ptr, len) (((uint64)ptr + (uint64)len) <= 0xFFFFFFFF)

static const char hmac_key_label[] = {
    "integrity protection:Quartz Mission Rom"};
static const char aes_derived_key_label[] = {
    "confidentiality:Fermion Mission Rom"};
uint8 zero_array_kdf[ZERO_ARRAY_KEY_SIZE] = {0};

typedef struct {
  CeCLCipherCntxType ctx;
} CeMLUtilCipherAlgoCntxType;
/**
  Encrypts the user data and generates the authentication/integrity protection
  data (referred to as meta data in the API). Encryption is done with an
  AES-128 CBC-bit key with meta data that includes HMAC SHA-256. The user
  stores the meta data along with the encrypted data.

  @param[in] p_user_input_password    Pointer to the user input password
  @param[in] user_input_password_size user input password size in bytes
  @param[in] pt_ptr_in                Pointer to the plaintext data.
  @param[in] input_data_len           Byte length of the plaintext data, in
                                      multiples of 16 bytes.
  @param[out] pt_ptr_out              Pointer to the ciphertext data. The output
                                      data length is equal to the input data
  length because the function expects the input data length to be an AES block
  multiple of 16 bytes with no padding.
  @param[out] output_data_len_ptr     Pointer to hold the byte length of the
                                      ciphertext data. output_data_len_ptr is
  the same value as input_data_len.
  @param[out] meta_data_out_ptr       Pointer to the meta data, which includes
  HMAC SHA-256.
  @param[out] meta_data_out_len       Byte length of meta_data_out; 64 bytes.

  @return
  CeMLErrorType

  @dependencies
  None.
*/
CeMLErrorType secure_storage_encrypt_authenticate(
    uint8 *p_user_input_password, uint32 user_input_password_size,
    void *pt_ptr_in, uint32 input_data_len, void *pt_ptr_out,
    uint32 *output_data_len_ptr, void *meta_data_out_ptr,
    uint32 meta_data_out_len) {
  uint8 *input_data = (uint8 *)pt_ptr_in;
  uint8 *output_data = (uint8 *)pt_ptr_out;
  uint8 *meta_data_out = (uint8 *)meta_data_out_ptr;
  uint8 iv_ptr[CEML_AES_IV_SIZE] = {0x0};
  uint8 hash_out[CEML_HASH_DIGEST_SIZE_SHA256] = {0x0};
  // uint32      hash_out_len=32;
  uint32 aes_key[4] = {0x0};
  uint32 hmac_key[4] = {0x0};
  CeMLIovecListType ioVecIn;
  CeMLIovecListType ioVecOut;
  // CeMLIovecListType   ioVecIn_tmp;
  // CeMLIovecListType   ioVecOut_tmp;

  CeMLIovecType IovecIn;
  CeMLIovecType IovecOut;
  // CeMLCntxHandle* cipher_cntx = NULL;
  CeMLCntxHandle *cipher_cntx = NULL;
  CeMLCntxHandle ceMlHandle;
  CeMLUtilCipherAlgoCntxType ClientCtxt;
  CeMLCipherDir cipher_direction;
  CeMLCipherModeType cipher_mode;
  CeMLErrorType err_code = CEML_ERROR_SUCCESS;

  /* Only support multiples of the block size for now.*/
  if ((NULL == input_data) || (NULL == output_data) ||
      (NULL == output_data_len_ptr) || (input_data_len % BLOCK_SIZE != 0)) {
    return CEML_ERROR_INVALID_PARAM;
  }

  if ((*output_data_len_ptr != input_data_len) ||
      (!IS_32_MEM_RANGE((uint32)output_data, *output_data_len_ptr))) {
    return CEML_ERROR_INVALID_PARAM;
  }

  /* Check to see meta data pointer is valid and meta data size is 64 bytes */
  if ((NULL == meta_data_out) || (meta_data_out_len != META_DATA_SIZE)) {
    return CEML_ERROR_INVALID_PARAM;
  }

  if ((uint8 *)meta_data_out >= (uint8 *)output_data &&
      (uint8 *)meta_data_out < ((uint8 *)output_data + *output_data_len_ptr)) {
    return CEML_ERROR_BAD_ADDRESS;
  }

  /* get IV from prng */
#ifdef NT_FN_HW_CRYPTO
  nt_wlan_hw_prng_get(iv_ptr, BLOCK_SIZE);
#else
  Rng_getRNG(iv_ptr, BLOCK_SIZE);
#endif // NT_FN_HW_CRYPTO

  cipher_cntx = &ceMlHandle;
  cipher_cntx->pClientCtxt = &ClientCtxt;

  ioVecIn.size = 1;
  ioVecOut.size = 1;
  ioVecIn.iov = &IovecIn;
  ioVecOut.iov = &IovecOut;
  ioVecIn.iov->dwLen = input_data_len;
  ioVecIn.iov->pvBase = (void *)input_data;
  ioVecOut.iov->dwLen = input_data_len;
  ioVecOut.iov->pvBase = (void *)output_data;

  do {
    err_code = CeMLInit();
    if (CEML_ERROR_SUCCESS != err_code) {
      break;
    }

    err_code = CeMLCipherInit(&cipher_cntx, CEML_CIPHER_ALG_AES128);
    if (CEML_ERROR_SUCCESS != err_code) {
      break;
    }

    cipher_mode = CEML_CIPHER_MODE_CBC;
    err_code = CeMLCipherSetParam(cipher_cntx, CEML_CIPHER_PARAM_MODE,
                                  &cipher_mode, sizeof(CeMLCipherModeType));
    if (CEML_ERROR_SUCCESS != err_code) {
      break;
    }

    /*deriving a cipher key from HW key*/
    err_code =
        CeML_hw_kdf(cipher_cntx, CEML_KDF_SECURE_STORAGE,
                    (void *)(p_user_input_password), user_input_password_size,
                    0, aes_key, sizeof(aes_key) / sizeof(aes_key[0]));
    if (CEML_ERROR_SUCCESS != err_code) {
      break;
    }

    // err_code = CeMLCipherSetParam(cipher_cntx, CEML_CIPHER_PARAM_KEY,
    // aes_key, CEML_AES128_KEY_SIZE);
    err_code = CeMLCipherSetParam(cipher_cntx, CEML_CIPHER_PARAM_KEY, NULL,
                                  CEML_AES128_KEY_SIZE);
    if (CEML_ERROR_SUCCESS != err_code) {
      break;
    }

    err_code = CeMLCipherSetParam(cipher_cntx, CEML_CIPHER_PARAM_IV, iv_ptr,
                                  BLOCK_SIZE);
    if (CEML_ERROR_SUCCESS != err_code) {
      break;
    }

    /* Encrypt the data */
    cipher_direction = CEML_CIPHER_ENCRYPT;
    err_code = CeMLCipherSetParam(cipher_cntx, CEML_CIPHER_PARAM_DIRECTION,
                                  &cipher_direction, sizeof(CeMLCipherDir));
    if (CEML_ERROR_SUCCESS != err_code) {
      break;
    }

    err_code = CeMLCipherData(cipher_cntx, ioVecIn, &ioVecOut);
    if (err_code != CEML_ERROR_SUCCESS) {
      break;
    }

    err_code = CeMLCipherDeInit(&cipher_cntx);
    if (err_code != CEML_ERROR_SUCCESS) {
      break;
    }

    ioVecIn.iov->dwLen = input_data_len;
    ioVecIn.iov->pvBase = (void *)output_data;
    ioVecOut.iov->dwLen = CEML_HASH_DIGEST_SIZE_SHA256;
    ioVecOut.iov->pvBase = (void *)hash_out;

    if (CEML_ERROR_SUCCESS != err_code) {
      break;
    }

    err_code = CeMLHmac(p_user_input_password, user_input_password_size,
                        ioVecIn, &ioVecOut, CEML_HASH_ALGO_SHA256);

    if (CEML_ERROR_SUCCESS != err_code) {
      break;
    }

    /* meta data contains 32 byte HMAC and 1 byte version, 16byte IV, 1byte key1
     * index, 1byte key2 index */
    /* clear the hmac filed in input data and then compute HMAC */
    CeElMemset((uint8 *)meta_data_out, 0, CEML_HASH_DIGEST_SIZE_SHA256);
    CeElMemset((uint8 *)meta_data_out + CEML_HASH_DIGEST_SIZE_SHA256, 1, 1);
    CeElMemScpy(
        (uint8 *)((uint8 *)meta_data_out + CEML_HASH_DIGEST_SIZE_SHA256 + 1),
        BLOCK_SIZE, (uint8 *)iv_ptr, BLOCK_SIZE);
    CeElMemScpy((uint8 *)meta_data_out, CEML_HASH_DIGEST_SIZE_SHA256,
                (uint8 *)hash_out, CEML_HASH_DIGEST_SIZE_SHA256);
  } while (0);

  /* erase the kdf key */
  CeML_hw_kdf(cipher_cntx, CEML_KDF_SECURE_STORAGE, zero_array_kdf,
              ZERO_ARRAY_KEY_SIZE, 0, hmac_key,
              sizeof(hmac_key) / sizeof(hmac_key[0]));

  if (cipher_cntx != NULL) {
    CeMLCipherDeInit(&cipher_cntx);
  }

  CeMLDeInit();

  *output_data_len_ptr = input_data_len;

  return err_code;
}

/**
  Decrypts the user data and checks the authenticity/integrity of the data.
  The metadata contains the authenticity/integrity protection data. Encrypted
  user data is decrypted by this function after passing the
  authenticity/integrity check.

  @param[in] p_user_input_password    Pointer to the user input password
  @param[in] user_input_password_size user input password size in bytes
  @param[in] pt_ptr_in                Pointer to the cipherext data.
  @param[in] input_data_len           Byte length of the ciphertext data, in
                                      multiples of 16 bytes.
  @param[out] pt_ptr_out              Pointer to the plaintext data.
  @param[out] output_data_len_ptr     Pointer to hold the byte length of the
                                      plaintext data. output_data_len_ptr is the
                                      same value as input_data_len.
  @param[in] meta_data_in_ptr         Pointer to the meta data, which includes
  HMAC SHA-256.
  @param[in] meta_data_in_len         Byte length of the meta_data_in; 64
  bytes.

  @return
  CeMLErrorType

  @dependencies
  None.
*/
CeMLErrorType secure_storage_decrypt_authenticate(
    uint8 *p_user_input_password, uint32 user_input_password_size,
    void *pt_ptr_in, uint32 input_data_len, void *pt_ptr_out,
    uint32 *output_data_len_ptr, void *meta_data_in_ptr,
    uint32 meta_data_in_len) {
  uint8 *input_data = (uint8 *)pt_ptr_in;
  uint8 *output_data = (uint8 *)pt_ptr_out;
  uint8 *meta_data_in = (uint8 *)meta_data_in_ptr;
  uint8 iv_ptr[CEML_AES_IV_SIZE] = {0x0};
  uint8 hash_out[CEML_HASH_DIGEST_SIZE_SHA256] = {0x0};
  uint32 aes_key[4] = {0x0};
  uint32 hmac_key[4] = {0x0};
  CeMLIovecListType ioVecIn;
  CeMLIovecListType ioVecOut;
  CeMLIovecType IovecIn;
  CeMLIovecType IovecOut;
  CeMLCntxHandle *cipher_cntx = NULL;
  CeMLCntxHandle ceMlHandle;
  CeMLUtilCipherAlgoCntxType ClientCtxt;
  CeMLCipherDir cipher_direction;
  CeMLCipherModeType cipher_mode;
  CeMLErrorType err_code = CEML_ERROR_SUCCESS;

  /* Only support multiples of the block size for now.*/
  if ((NULL == input_data) || (NULL == output_data) ||
      (NULL == output_data_len_ptr) || (input_data_len % BLOCK_SIZE != 0)) {
    return CEML_ERROR_INVALID_PARAM;
  }

  // output length needs to be equal to input length and checking for wraparound
  if ((*output_data_len_ptr != input_data_len) ||
      (!IS_32_MEM_RANGE((uint32)output_data, *output_data_len_ptr))) {
    return CEML_ERROR_INVALID_PARAM;
  }

  /* Check to see meta data pointer is valid and meta data size is 64 bytes */
  if ((NULL == meta_data_in) || (meta_data_in_len != META_DATA_SIZE)) {
    return CEML_ERROR_INVALID_PARAM;
  }

  if ((uint8 *)meta_data_in >= (uint8 *)output_data &&
      (uint8 *)meta_data_in < ((uint8 *)output_data + *output_data_len_ptr)) {
    return CEML_ERROR_BAD_ADDRESS;
  }

  CeElMemScpy(
      (uint8 *)iv_ptr, BLOCK_SIZE,
      (uint8 *)((uint8 *)meta_data_in + CEML_HASH_DIGEST_SIZE_SHA256 + 1),
      BLOCK_SIZE);

  cipher_cntx = &ceMlHandle;
  cipher_cntx->pClientCtxt = &ClientCtxt;

  ioVecIn.size = 1;
  ioVecOut.size = 1;
  ioVecIn.iov = &IovecIn;
  ioVecOut.iov = &IovecOut;
  ioVecIn.iov->dwLen = input_data_len;
  ioVecIn.iov->pvBase = (void *)input_data;

  do {
    err_code = CeMLInit();
    if (CEML_ERROR_SUCCESS != err_code) {
      break;
    }

    ioVecOut.iov->dwLen = CEML_HASH_DIGEST_SIZE_SHA256;
    ioVecOut.iov->pvBase = (void *)hash_out;

    err_code = (CeMLErrorType)CeML_hw_kdf(
        cipher_cntx, CEML_KDF_SECURE_STORAGE, (void *)(p_user_input_password),
        user_input_password_size, 0, hmac_key,
        sizeof(hmac_key) / sizeof(hmac_key[0]));

    if (CEML_ERROR_SUCCESS != err_code) {
      break;
    }

    // err_code = CeMLHmac(NULL, CEML_AES128_KEY_SIZE, ioVecIn, &ioVecOut,
    // CEML_HASH_ALGO_SHA256);
    err_code = CeMLHmac(p_user_input_password, user_input_password_size,
                        ioVecIn, &ioVecOut, CEML_HASH_ALGO_SHA256);

    if (err_code != CEML_ERROR_SUCCESS) {
      break;
    }

    /* meta data contains 32 byte HMAC and 1 byte version, 16byte IV, 1byte key1
     * index, 1byte key2 index */
    /* compare the stored hmac with that calculated of encrypted input passed to
     * authenticate integrity of data */
    if (memcmp((uint8 *)meta_data_in, (uint8 *)hash_out,
               CEML_HASH_ALGO_SHA256) != 0) {
      err_code = CEML_ERROR_BAD_DATA;
      printf("Invalid user password or encrypted data is corrupted\n");
      break;
    }

    ioVecOut.iov->dwLen = input_data_len;
    ioVecOut.iov->pvBase = (void *)output_data;

    err_code = CeMLCipherInit(&cipher_cntx, CEML_CIPHER_ALG_AES128);
    if (CEML_ERROR_SUCCESS != err_code) {
      break;
    }

    cipher_mode = CEML_CIPHER_MODE_CBC;
    err_code = CeMLCipherSetParam(cipher_cntx, CEML_CIPHER_PARAM_MODE,
                                  &cipher_mode, sizeof(CeMLCipherModeType));
    if (CEML_ERROR_SUCCESS != err_code) {
      break;
    }

    /*deriving a cipher key from HW key*/
    err_code = (CeMLErrorType)CeML_hw_kdf(cipher_cntx, CEML_KDF_SECURE_STORAGE,
                                          (void *)(p_user_input_password),
                                          user_input_password_size, 0, aes_key,
                                          sizeof(aes_key) / sizeof(aes_key[0]));
    if (CEML_ERROR_SUCCESS != err_code) {
      break;
    }

    err_code = CeMLCipherSetParam(cipher_cntx, CEML_CIPHER_PARAM_KEY, NULL,
                                  CEML_AES128_KEY_SIZE);
    // err_code = CeMLCipherSetParam(cipher_cntx, CEML_CIPHER_PARAM_KEY,
    // aes_key, CEML_AES128_KEY_SIZE);
    if (CEML_ERROR_SUCCESS != err_code) {
      break;
    }

    err_code = CeMLCipherSetParam(cipher_cntx, CEML_CIPHER_PARAM_IV, iv_ptr,
                                  BLOCK_SIZE);
    if (CEML_ERROR_SUCCESS != err_code) {
      break;
    }

    /* Decrypt the data */
    cipher_direction = CEML_CIPHER_DECRYPT;
    err_code = CeMLCipherSetParam(cipher_cntx, CEML_CIPHER_PARAM_DIRECTION,
                                  &cipher_direction, sizeof(CeMLCipherDir));

    if (CEML_ERROR_SUCCESS != err_code) {
      break;
    }

    err_code = CeMLCipherData(cipher_cntx, ioVecIn, &ioVecOut);
    if (err_code != CEML_ERROR_SUCCESS) {
      break;
    }

    err_code = CeMLCipherDeInit(&cipher_cntx);
    if (err_code != CEML_ERROR_SUCCESS) {
      break;
    }
  } while (0);

  /* erase the kdf key */
  CeML_hw_kdf(cipher_cntx, CEML_KDF_SECURE_STORAGE, zero_array_kdf,
              ZERO_ARRAY_KEY_SIZE, 0, hmac_key,
              sizeof(hmac_key) / sizeof(hmac_key[0]));

  if (cipher_cntx != NULL) {
    CeMLCipherDeInit(&cipher_cntx);
  }

  CeMLDeInit();

  *output_data_len_ptr = input_data_len;

  return err_code;
}
