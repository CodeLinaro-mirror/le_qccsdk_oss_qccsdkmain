/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "qapi_flash.h"
#include "ferm_flash.h"

#define Flash_ErrorMap(Status)   (Status == QAPI_OK) ? QAPI_OK : __QAPI_ERROR(QAPI_MOD_FLASH, Status)

/**
   @brief Initialize the flash module.

   This function must be called before any other flash functions.

   @return
   QAPI_OK -- On success. \n
   Error code -- On failure.
*/
qapi_Status_t qapi_Flash_Init()
{
    FLASH_STATUS status;

    status = drv_flash_init();

    return Flash_ErrorMap(status);
}

/**
   @brief Read data from the flash.

   @param[in]  Address    The flash address to start to read from.
   @param[in]  ByteCnt    Number of bytes to read.
   @param[out] Buffer     Data buffer for a flash read operation.

   @return
   QAPI_OK -- If a read completed successfully. \n
   Error code -- If there is an error.
*/
qapi_Status_t qapi_Flash_Read(uint32_t Address, uint32_t ByteCnt, uint8_t *Buffer)
{
    FLASH_STATUS status;

    status = drv_flash_read(Address, ByteCnt, Buffer, NULL, NULL);

    return Flash_ErrorMap(status);
}

/**
   @brief Write data to the flash.

   @param[in] Address    The flash address to start to write to.
   @param[in] ByteCnt    Number of bytes to write.
   @param[in] Buffer     Data buffer containing data to be written.

   @return
   QAPI_OK -- If blocking write completed successfully. \n
   Error code -- If there is an error.
*/
qapi_Status_t qapi_Flash_Write(uint32_t Address, uint32_t ByteCnt, uint8_t *Buffer)
{
    FLASH_STATUS status;

    status = drv_flash_write(Address, ByteCnt, Buffer, NULL, NULL);

    return Flash_ErrorMap(status);
}

/**
   @brief Erase the given flash blocks or bulks, or the whole chip.

   @param[in] EraseType  Specify the erase type.
   @param[in] Start      For block erase - the starting block of a
                         number of blocks to erase.
                         For bulk erase - the starting bulk of a number
                         of bulks to erase.
                         For chip erase, it should be 0.
   @param[in] Cnt        For block erase - the number of blocks to erase.
                         For bulk erase - the number of bulks to erase.
                         For chip erase, it should be 1.
   @return
   QAPI_OK -- If blocking erase completed successfully. \n
   Error code -- If there was an error.
*/
qapi_Status_t qapi_Flash_Erase(qapi_FLASH_Erase_Type_t EraseType, uint32_t Start, uint32_t Cnt)
{
    FLASH_STATUS status;

    status = drv_flash_erase(EraseType, Start, Cnt, NULL, NULL);

    return Flash_ErrorMap(status);
}

/**
   @brief Read flash registers.

   @param[in]  RegOpcode  Operation code.
   @param[in]  Len        The length of register value to be read.
   @param[out] RegValue   The read out value.

   @return
   QAPI_OK -- On success. \n
   Error code -- On failure.
*/
qapi_Status_t qapi_Flash_Get_Info(flash_info_t *flash_info)
{
    flash_config_data_t* flash_cfg = NULL;
    FLASH_STATUS status = QAPI_OK;

    if(!flash_info) {
       return QAPI_ERR_INVALID_PARAM;
    }

    flash_cfg = drv_flash_get_config();
    if(!flash_cfg) {
        status = FLASH_DEVICE_NOT_FOUND;
    } else {
        flash_info->devicd_id = flash_cfg->device_id;
        flash_info->block_count = flash_cfg->density_in_blocks;
        flash_info->block_size_bytes = BLOCK_SIZE_IN_BYTES;
        flash_info->page_size_bytes = PAGE_SIZE_IN_BYTES;
    }

    return Flash_ErrorMap(status);
}

