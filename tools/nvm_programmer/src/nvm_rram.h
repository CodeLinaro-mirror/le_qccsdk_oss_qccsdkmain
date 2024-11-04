/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/
#ifndef _NVM_RRAM_H_
#define _NVM_RRAM_H_

/*-------------------------------------------------------------------------
 * Preprocessor Definitions, Constants, and Type Declarations
 *-----------------------------------------------------------------------*/
#define RRAM_START                                           (0x00200000)
#define RRAM_END                                             (0x0037FFFF)

#define OTP_START                                            (0x001A0000)
#define OTP_END                                              (0x001A0FFF)
#define READ_PERMISSION_READ_WRITE_PERMISIONS_ADDRESS        (0x001A0030)
#define READ_PERMISSION_READ_WRITE_PERMISIONS_OFFSET         (0x2)

/*PBL should not be erased*/
#define RRAM_ERASE_START                                     (0x00208000)


#define RRAM_ERASE_BUFFER_SIZE                               (4096)


/*-------------------------------------------------------------------------
 * Type Declarations
 *-----------------------------------------------------------------------*/

/*-------------------------------------------------------------------------
 * Function Declarations
 *-----------------------------------------------------------------------*/
 void rram_dxe_init(void);

int32_t rram_block_write(uint32_t destination, const uint8_t* source, uint32_t length);

int32_t rram_write(uint32_t destination, uint8_t* source, uint32_t length);

int32_t rram_read(uint32_t address, uint8_t *buffer, uint32_t length);

int32_t nvm_rram_read(uint32_t address, uint8_t *buffer, uint32_t length);
int32_t nvm_rram_write(uint32_t destination, uint8_t* source, uint32_t length);

int32_t otp_rram_read(uint32_t address, uint8_t *buffer, uint32_t length);
int32_t otp_rram_write(uint32_t destination, uint8_t* source, uint32_t length);

int32_t rram_erase(uint32_t address, uint32_t length);

void nop_delay( uint32_t n );

#endif
