/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "CeML.h"
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
    uint32 meta_data_out_len);

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
  @param[in] meta_data_in_len         Byte length of the meta_data_out; 64
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
    uint32 meta_data_in_len);
