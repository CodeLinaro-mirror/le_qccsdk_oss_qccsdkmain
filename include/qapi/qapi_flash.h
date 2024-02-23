/** @file qapi_flash.h
   @brief Flash Services Interface definition.

      This module provides flash operation APIs.

*/

/** @addtogroup qapi_peripherals_flash

  The QSPI Flash module provides flash operation APIs.

  qapi_Flash_Init() initializes the flash module and must be called
  before any other function in this module. The system automatically searches for the connected flash.

  If need to change some flash configuration, refer to flash_config_data_t flash_device_config[].

  Flash read/write/erase/readreg/writereg operations cannot be performed simultaneously.
  There can be only one operation ongoing at a time.

  Flash read/write/erase/readreg/writereg support only blocking operations.

  For a blocking operation, the related function returns until the
  required number of data/status are read/written/erased successfully, or
  there is an error. If the required number is large, the related function
  may take some time to return. It should be taken into consideration if
  the product is power and time sensitive.
*/

#ifndef __QAPI_FLASH_H__
#define __QAPI_FLASH_H__

/*-------------------------------------------------------------------------
 * Include Files
 *-----------------------------------------------------------------------*/

#include "qapi_types.h"
#include "qapi_status.h"

/** @addtogroup qapi_peripherals_flash
@{ */

#define QAPI_FLASH_DEVICE_FAIL                __QAPI_ERROR(QAPI_MOD_FLASH, 1) /**< Operation failed */
#define QAPI_FLASH_DEVICE_NOT_SUPPORTED       __QAPI_ERROR(QAPI_MOD_FLASH, 2) /**< Device/operation not supported */
#define QAPI_FLASH_DEVICE_INVALID_PARAMETER   __QAPI_ERROR(QAPI_MOD_FLASH, 3) /**< API parameters invalid */
#define QAPI_FLASH_DEVICE_IMAGE_NOT_FOUND     __QAPI_ERROR(QAPI_MOD_FLASH, 4) /**< FW Image ID not found */
#define QAPI_FLASH_DEVICE_NOT_FOUND           __QAPI_ERROR(QAPI_MOD_FLASH, 5) /**< Device not found on supported device list */
#define QAPI_FLASH_DEVICE_PENDING             __QAPI_ERROR(QAPI_MOD_FLASH, 6) /** Flash non-blocking operation is ongoing. */
#define QAPI_FLASH_DEVICE_NO_MEMORY           __QAPI_ERROR(QAPI_MOD_FLASH, 7) /**alloc memory failed */
#define QAPI_FLASH_DEVICE_BUSY                __QAPI_ERROR(QAPI_MOD_FLASH, 8) /** flash device busy */

/*-------------------------------------------------------------------------
 * Type Declarations
 *-----------------------------------------------------------------------*/

/**
   Enumeration of flash erase types.
*/
typedef enum
{
    QAPI_FLASH_BLOCK_ERASE_E, /**< Block erase. Block size is 4KB. */
    QAPI_FLASH_BULK_ERASE_E,  /**< Bulk erase. Bulk size is n*4Kb, for
                                   example 32Kb, 64Kb, and so on.
                                   It depends on the specific flash type. Here is 64Kb by default*/
    QAPI_FLASH_CHIP_ERASE_E   /**< Chip erase. */
} qapi_FLASH_Erase_Type_t;

/*-------------------------------------------------------------------------
 * Function Declarations
 *-----------------------------------------------------------------------*/

/**
   @brief Initialize the flash module.

   This function must be called before any other flash functions.

   @return
   QAPI_OK -- On success. \n
   Error code -- On failure.
*/
qapi_Status_t qapi_Flash_Init();

/**
   @brief Read data from the flash.

   @param[in]  Address    The flash address to start to read from.
   @param[in]  ByteCnt    Number of bytes to read.
   @param[out] Buffer     Data buffer for a flash read operation.
   @return
   QAPI_OK -- If a read completed successfully. \n
   Error code -- If there is an error.
*/
qapi_Status_t qapi_Flash_Read(uint32_t Address, uint32_t ByteCnt, uint8_t *Buffer);

/**
   @brief Write data to the flash.

   @param[in] Address    The flash address to start to write to.
   @param[in] ByteCnt    Number of bytes to write.
   @param[in] Buffer     Data buffer containing data to be written.

   @return
   QAPI_OK -- If blocking write completed successfully. \n
   Error code -- If there is an error.
*/
qapi_Status_t qapi_Flash_Write(uint32_t Address, uint32_t ByteCnt, uint8_t *Buffer);

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
qapi_Status_t qapi_Flash_Erase(qapi_FLASH_Erase_Type_t EraseType, uint32_t Start, uint32_t Cnt);

/**
   @brief Read flash registers.

   @param[in]  RegOpcode  Operation code.
   @param[in]  Len        The length of register value to be read.
   @param[out] RegValue   The read out value.

   @return
   QAPI_OK -- On success. \n
   Error code -- On failure.
*/
qapi_Status_t qapi_Flash_Read_Reg(uint8_t RegOpcode, uint8_t Len, uint8_t *RegValue);

/**
   @brief Write flash registers.

   @param[in] RegOpcode   Operation code.
   @param[in] Len         The length of register value to be written.
   @param[in] RegValue    The written value.

   @return
   QAPI_OK -- If blocking writereg completed successfully. \n
   Error code -- If there was an error.
*/
qapi_Status_t qapi_Flash_Write_Reg(uint8_t RegOpcode, uint8_t Len, uint8_t *RegValue);

/** @} */ /* end_addtogroup qapi_peripherals_flash */

#endif

