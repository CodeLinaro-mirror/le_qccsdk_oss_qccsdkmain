/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*
 * nt_kdf.h
 */

#ifndef CORE_SYSTEM_INC_NT_CRYPTO_H_
#define CORE_SYSTEM_INC_NT_CRYPTO_H_

#include "nt_flags.h"
#include "nt_osal.h"
#include "nt_common.h"
#include "dxe_api.h"

#ifdef NT_FN_HW_CRYPTO

#define DATA_IN_BASE_ADDRESS  0x10020000  // need to change thebase address
#define DATA_OUT_BASE_ADDRESS 0x10021000  // need to change thebase address

#define DATA_IN(i)  *((uint32_t *)(DATA_IN_BASE_ADDRESS) + i)   // data in buffer
#define DATA_OUT(i) *((uint32_t *)(DATA_OUT_BASE_ADDRESS) + i)  // data out buffer

#define encrypt 1
#define decrypt 0

enum mode_of_transfer { HCAL_DXE_MODE = 0, HCAL_REGISTER_MODE = 1 };

int8_t nt_secure_ip_pwr_status(void);

int8_t nt_prng_init(void);

uint8_t nt_mode_of_transfer(uint8_t mode);

int32_t nt_aes128_ecb(uint8_t mode, const unsigned char din[16], unsigned char key[4], unsigned char dout[16]);

uint32_t crypto_arm(uint32_t ce_data_in[], uint32_t ce_data_out[], uint32_t din_limit, uint32_t dout_limit,
                    uint32_t timeout, uint32_t ceId);

NT_BOOL nt_qcc_dxe_transfer(e_dxe_channel channelIn, e_dxe_channel channelOut, uint32_t DinAddr, uint32_t DoutAddr,
                            uint32_t inDwords, uint32_t outDwords);

uint32_t crypto_arm_1(uint32_t ce_data_in[], uint32_t ce_data_out[], uint32_t din_limit, uint32_t dout_limit,
                      uint32_t timeout, uint32_t ceId);

uint32_t nt_qcc_register_transfer(const uint32_t ce_data_in[], uint32_t ce_data_out[], uint32_t din_limit,
                                  uint32_t dout_limit, uint32_t timeout, uint32_t ceId);

uint32_t crypto_arm2_sha(uint32_t ce_data_in[], uint32_t __attribute__((__unused__)) ce_data_out[], uint32_t din_limit,
                         uint32_t ceId);

int32_t nt_qcc_aes128_ecb(uint8_t mode, const unsigned char din[16], const uint32_t key[8], unsigned char dout[16]);

int32_t nt_qcc_aes128_cbc(int mode, const unsigned char iv[16], const unsigned char *input, const uint32_t key[8],
                          unsigned char *output);
#endif  // NT_FN_HW_CRYPTO
#endif  /* CORE_SYSTEM_INC_NT_CRYPTO_H_ */
