/*
Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
*/
 /*-------------------------------------------------------------------------
 * Include Files
 *-----------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "qapi_status.h"
#include "ferm_flash.h"
#include "uart_print.h"
#include "nvm_flash.h"

#ifdef CONFIG_BOARD_QCC730_QSPI_ENABLE

/*-------------------------------------------------------------------------
 * External Function Declarations
 *-----------------------------------------------------------------------*/


/*-------------------------------------------------------------------------
 * Variables
 *-----------------------------------------------------------------------*/
static uint32_t density_in_blocks = 0;

/* Buffer all bytes are 0xFF, used as comparative objects */
uint8_t comparative_buffer[BLOCK_SIZE_IN_BYTES];

/* Buffer used to read data and check all bytes are 0xFF. */
uint8_t erase_read_buffer[BLOCK_SIZE_IN_BYTES];

/*-------------------------------------------------------------------------
 * Private Function Declarations
 *-----------------------------------------------------------------------*/

/*-------------------------------------------------------------------------
 * Function Definitions
 *-----------------------------------------------------------------------*/

/**
   @brief Initialize the flash module.

   This function must be called before any other flash functions.

   @param[in] void

   @return QAPI_OK if flash initialise successfully or QAPI_ERROR if it is failed.
*/

int32_t flash_init(void)
{
    qapi_Status_t status;
    flash_config_data_t * flash_config;
    static uint32_t flash_inited = 0;
    if( flash_inited )
    {
        return QAPI_OK;
    }

    status = drv_flash_init();
    if (status != QAPI_OK)
    {
        UART_PRINT("drv_flash_init fail :%ld\r\n",status);
        return QAPI_ERROR;
    }
    else
    {
        flash_config = drv_flash_get_config();
        if(flash_config)
        {
            density_in_blocks = flash_config->density_in_blocks;
        }
        else
        {
            UART_PRINT("drv_flash_get_config fail\r\n");
            status = QAPI_ERROR;
        }
        memset(comparative_buffer, 0xFF, BLOCK_SIZE_IN_BYTES);
        flash_inited = 1;
        return status;
    }

    return QAPI_ERROR;
}

/**
   @brief Verifies if the given Flash address range is valid for an Flash write
          operation.

   @param[in] address  Start address for the Flash write operation.
   @param[in] length   Number of bytes to be written.

   @return true if the address range is valid or false if it is not.
*/
static qbool_t flash_verify_address(uint32_t address, uint32_t length)
{
    qbool_t ret_val;

    if ( (address <= density_in_blocks * BLOCK_SIZE_IN_BYTES) &&
        (address % BLOCK_SIZE_IN_BYTES == 0) &&
        (((density_in_blocks * BLOCK_SIZE_IN_BYTES - address) + 1) >= length))
    {
        /* Main flash region. */
        ret_val = true;
    }
    else
    {
        /* Invalid address and/or length. */
        ret_val = false;
    }

    return (ret_val);
}


/* Check if a block need to erase */
int32_t flash_partial_check_erase(uint32_t start_addr, uint32_t length)
{
    uint32_t result = FLASH_DEVICE_DONE;
    uint32_t offset;
    uint32_t end_offset;
    uint32_t start_block;
    uint32_t end_block;
    uint32_t block_num;

    /* Verify the address and size are valid. */
    if (flash_verify_address(start_addr, length) == false)
    {
        UART_PRINT("Invalid address: %ld Size:%ld\r\n", start_addr, length);
        return QAPI_ERR_BOUNDS;
    }

    start_block = start_addr /BLOCK_SIZE_IN_BYTES;
    end_block = (start_addr + length -1) /BLOCK_SIZE_IN_BYTES;

    block_num = end_block -start_block + 1;

    if (block_num == 0)
        return result;

    offset = start_block * BLOCK_SIZE_IN_BYTES;
    end_offset = offset + block_num * BLOCK_SIZE_IN_BYTES;

    while(offset < end_offset)
    {
        result = flash_read(offset, erase_read_buffer, BLOCK_SIZE_IN_BYTES);
        if(result != FLASH_DEVICE_DONE)
        {
            break;
        }

        /* Check if the buffer is filled with 0xFF, if it is not, needs erase. */
        if(memcmp(erase_read_buffer, comparative_buffer, BLOCK_SIZE_IN_BYTES) != 0)
        {
            result = drv_flash_erase(FLASH_BLOCK_ERASE_E, start_block, block_num, NULL, NULL);
            if (result != FLASH_DEVICE_DONE){
                break;
            }
        }
        offset += BLOCK_SIZE_IN_BYTES;
    }

    return result;
}


__attribute__((unused)) int32_t flash_partial_erase(uint32_t start_addr, uint32_t length)
{
    qapi_Status_t status = QAPI_OK;

    uint32_t start_block;
    uint32_t end_block;
    uint32_t block_num;

    /* Verify the address and size are valid. */
    if (flash_verify_address(start_addr, length) == false)
    {
        UART_PRINT("Invalid address: %ld Size:%ld\r\n", start_addr, length);
        return QAPI_ERR_BOUNDS;
    }

    start_block = start_addr /BLOCK_SIZE_IN_BYTES;
    end_block = (start_addr + length -1) /BLOCK_SIZE_IN_BYTES;

    block_num = end_block -start_block + 1;
    status = drv_flash_erase(FLASH_BLOCK_ERASE_E, start_block, block_num, NULL, NULL);

    return status;
}

int32_t flash_chip_erase()
{
    qapi_Status_t status = QAPI_OK;

    status = drv_flash_erase(FLASH_CHIP_ERASE_E, 0, 1, NULL, NULL);

    return status;
}

int32_t flash_write(uint32_t destination, uint8_t* source, uint32_t length)
{
    qapi_Status_t status;

    status = flash_partial_check_erase(destination, length);
    if(status != QAPI_OK) {
        return status;
    }

    status = drv_flash_write(destination, length, source, NULL, NULL);

    return status;
}

int32_t flash_read(uint32_t address, uint8_t *buffer, uint32_t length)
{
    qapi_Status_t status = QAPI_OK;
#ifdef NVM_FLASH_SPLIT_IN_SLICES
    uint32_t byte_cnt = length;
    uint32_t      read_size = 0;

    while (byte_cnt)
    {
        if (address % BLOCK_SIZE_IN_BYTES)
        {
            read_size = BLOCK_SIZE_IN_BYTES - (address % BLOCK_SIZE_IN_BYTES);
            if (read_size > byte_cnt)
            {
                read_size = byte_cnt;
            }
        }
        else
        {
            read_size = (byte_cnt > BLOCK_SIZE_IN_BYTES) ? (BLOCK_SIZE_IN_BYTES) : (byte_cnt);
        }

        status = drv_flash_read(address, read_size, buffer, NULL, NULL);
        if(status != QAPI_OK)
            break;

        address += read_size;
        buffer += read_size;
        byte_cnt -= read_size;
    }
#else
    status = drv_flash_read(address, length, buffer, NULL, NULL);
#endif
    return status;
}

#endif /* CONFIG_BOARD_QCC730_QSPI_ENABLE */
