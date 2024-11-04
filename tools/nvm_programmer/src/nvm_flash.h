/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/
#ifndef _NVM_FLASH_H_
#define _NVM_FLASH_H_
#ifdef CONFIG_BOARD_QCC730_QSPI_ENABLE


/*-------------------------------------------------------------------------
 * Preprocessor Definitions, Constants, and Type Declarations
 *-----------------------------------------------------------------------*/

#define NVM_FLASH_SPLIT_IN_SLICES

/*-------------------------------------------------------------------------
 * Function Declarations
 *-----------------------------------------------------------------------*/

int32_t flash_init(void);
int32_t flash_write(uint32_t destination, uint8_t* source, uint32_t length);
int32_t flash_read(uint32_t address, uint8_t *buffer, uint32_t length);
int32_t flash_partial_erase(uint32_t start_addr, uint32_t length);
int32_t flash_partial_check_erase(uint32_t start_addr, uint32_t length);
int32_t flash_chip_erase();

#endif /* CONFIG_BOARD_QCC730_QSPI_ENABLE */
#endif /* _NVM_FLASH_H_ */
