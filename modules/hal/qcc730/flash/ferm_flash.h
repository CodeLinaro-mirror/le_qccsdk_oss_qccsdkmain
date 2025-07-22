/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/** @file ferm_flash.h
   @brief Flash Services Interface definition.
   This module provides flash operation APIs.
*/

/** @addtogroup peripherals_flash

  The QSPI Flash module provides flash operation APIs.

  drv_flash_init() initializes the flash module and must be called
  before any other function in this module. The input parameter - Config -
  must be set based on the specific flash used.

  Flash read/write/erase/writereg operations cannot be performed simultaneously.
  There can be only one operation ongoing at a time. A mutex is used to protect it.

  Flash read/write/erase/writereg support blocking operations on Fermion.

  For a blocking operation, the related function returns until the
  required number of data/status are read/written/erased successfully, or
  there is an error. If the required number is large, the related function
  may take some time to return. It should be taken into consideration if
  the product is power and time sensitive.
*/

#ifndef __FERM_FLASH_H__
#define __FERM_FLASH_H__

/*-------------------------------------------------------------------------
 * Include Files
 *-----------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "ferm_flash_config.h"

#define FLASH_DEVICE_DONE              0   /**< Operation passed */
#define FLASH_DEVICE_FAIL              (1) /**< Operation failed */
#define FLASH_DEVICE_NOT_SUPPORTED     (2) /**< Device/operation not supported */
#define FLASH_DEVICE_INVALID_PARAMETER (3) /**< API parameters invalid */
#define FLASH_DEVICE_IMAGE_NOT_FOUND   (4) /**< FW Image ID not found */
#define FLASH_DEVICE_NOT_FOUND         (5) /**< Device not found on supported device list */
#define FLASH_DEVICE_PENDING           (6) /** Flash non-blocking operation is ongoing. */
#define FLASH_DEVICE_NO_MEMORY         (7) /**alloc memory failed */
#define FLASH_DEVICE_BUSY              (8) /** flash device busy */

#define PAGE_SIZE_IN_BYTES  256
#define BLOCK_SIZE_IN_BYTES 4096
#define FLASH_16MB_IN_BYTES 0x1000000

typedef int FLASH_STATUS; /**< Error status values used in FLASH driver */

/** @addtogroup qapi_peripherals_flash
@{ */

/*-------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 *-----------------------------------------------------------------------*/
/** Flash read/write mode: SDR, single mode. */
#define FLASH_RW_MODE_SDR_SINGLE 1

/** Flash read/write mode: SDR, dual mode. */
#define FLASH_RW_MODE_SDR_DUAL 2

/** Flash read/write mode: SDR, quad mode. */
#define FLASH_RW_MODE_SDR_QUAD 3

/** Flash read/write mode: DDR, quad mode. */
#define FLASH_RW_MODE_DDR_QUAD 7

/*-------------------------------------------------------------------------
 * Type Declarations
 *-----------------------------------------------------------------------*/

/**
   Enumeration of flash erase types.
*/
typedef enum {
    FLASH_BLOCK_ERASE_E, /**< Block erase. Block size is 4KB. */
    FLASH_BULK_ERASE_E,  /**< Bulk erase. A bulk size is 64KB.
                                   It depends on the specific flash type. */
    FLASH_CHIP_ERASE_E   /**< Chip erase. */
} flash_erase_type_t;

/**
   @brief Prototype for a function called after a non-blocking flash
          read/write/erase/writereg operation is complete.

   This function is called from a flash operation task.

   @param[in] Status     Flash operation result.
   @param[in] UserParam  User-specified parameter provided when the callback
                         is registered.
*/
typedef void (*flash_operation_cb_t)(FLASH_STATUS status, void *user_param);

/*-------------------------------------------------------------------------
 * Function Declarations
 *-----------------------------------------------------------------------*/

/**
   @brief Initialize the flash module.

   This function must be called before any other flash functions.

   @return
   FLASH_DEVICE_DONE -- On success. \n
   Error code -- On failure.
*/
FLASH_STATUS drv_flash_init();

/**
   @brief deInitialize the flash module.

   @param[in]  dereg    Deregister the sleep callback function if set
 * to 1.

   @return
   FLASH_DEVICE_DONE -- On success, or an error code on failure.
*/
FLASH_STATUS drv_flash_deinit(uint32_t dereg);

/**
   @brief Read data from the flash.

   This function, if:
   - Blocking -- returns when the read is done
   - Non-blocking -- returns with #QAPI_FLASH_STATUS_PENDING,
   indicating that the read is performed in the background

   A callback function indicates when the non-blocking operation
   is complete. Because read is ongoing after this function returns,
   the input buffer for storing read data should not be freed. All
   other operations that must run after flash read, should not be performed
   before the callback function is called.

   @param[in]  address    The flash address to start to read from.
   @param[in]  byte_cnt    Number of bytes to read.
   @param[out] buffer     Data buffer for a flash read operation; it should be
                          in heap, or a static buffer for a non-blocking read.
   @param[in]  read_cb     Flash read callback function for a non-blocking
                          read. After a read is done, the ReadCB is
                          called to inform about the completion; it should
                          be NULL for a blocking read.
   @param[in]  user_param  The user-specified parameter for the callback
                          function.

   @return
   FLASH_DEVICE_DONE -- If a blocking read completed successfully. \n
   FLASH_STATUS_PENDING -- Indicating a non-blocking read is ongoing. \n
   Negative value -- If there is an error.
*/
FLASH_STATUS drv_flash_read(uint32_t address, uint32_t byte_cnt, uint8_t *buffer, flash_operation_cb_t read_cb,
                            void *user_param);

/**
   @brief Write data to the flash.

   This function, if:
   - Blocking -- returns when the write is done
   - Non-blocking -- returns with #QAPI_FLASH_STATUS_PENDING,
   indicating that the write is performed in the background

   A callback function indicates when the non-blocking operation
   is complete. Because write is ongoing after this function returns,
   the input buffer for storing write data should not be freed. All
   other operations that must run after flash write, should not be performed
   before the callback function is called.

   @param[in] address    The flash address to start to write to.
   @param[in] byte_cnt    Number of bytes to write.
   @param[in] buffer     Data buffer containing data to be written; it should be
                         in heap, or a static buffer for a non-blocking write.
   @param[in] write_cb    Flash write callback function for non-blocking
                         write. After write is done, the WriteCB is
                         called to inform about the completion; it should
                         be NULL for a blocking write.
   @param[in] user_param  The user-specified parameter for the callback function.

   @return
   FLASH_DEVICE_DONE-- If blocking write completed successfully. \n
   FLASH_STATUS_PENDING -- Indicating a non-blocking write is ongoing. \n
   Negative value -- If there is an error.
*/
FLASH_STATUS drv_flash_write(uint32_t address, uint32_t byte_cnt, uint8_t *buffer, flash_operation_cb_t write_cb,
                             void *user_param);

/**
   @brief Erase the given flash blocks or bulks, or the whole chip.

   This function, if:
   - Blocking -- returns when the erase is done
   - Non-blocking -- returns with #QAPI_FLASH_STATUS_PENDING,
   indicating that the erase is performed in the background

   A callback function indicates when the non-blocking operation
   is complete. Because erase is ongoing after this function returns,
   other operations that must run after flash erase, should not be performed
   before the callback function is called.

   @param[in] erase_type  Specify the erase type.
   @param[in] start      For block erase - the starting block of a
                         number of blocks to erase.
                         For bulk erase - the starting bulk of a number
                         of bulks to erase.
                         For chip erase, it should be 0.
   @param[in] cnt        For block erase - the number of blocks to erase.
                         For bulk erase - the number of bulks to erase.
                         For chip erase, it should be 1.
   @param[in] erase_cb    Flash erase callback function for non-blocking
                         erase. After erase is done, the EraseCB is
                         called to inform about the completion; it should be
                         NULL for a blocking erase.
   @param[in] user_param  The user-specified parameter for the callback function.

   @return
   FLASH_DEVICE_DONE -- If blocking erase completed successfully. \n
   FLASH_STATUS_PENDING -- Indicating a non-blocking erase is ongoing. \n
   Negative value -- If there was an error.
*/
FLASH_STATUS drv_flash_erase(flash_erase_type_t erase_type, uint32_t start, uint32_t cnt, flash_operation_cb_t erase_cb,
                             void *user_param);

/**
   @brief Get current flash configuration.

   @return
   Pointer to current flash configuration.
*/
flash_config_data_t *drv_flash_get_config(void);

/**
   @brief Read flash registers.

   @param[in]  reg_opcode  Operation code.
   @param[in]  len        The length of register value to be read.
   @param[out] reg_value   The read out value.

   @return
   FLASH_DEVICE_DONE -- On success. \n
   Error code -- On failure.
*/
FLASH_STATUS drv_flash_read_reg(uint8_t reg_opcode, uint8_t len, uint8_t *reg_value);

/**
   @brief Write flash registers.

   This function, if:
   - Blocking -- returns when the writereg is done
   - Non-blocking -- it returns with #QAPI_FLASH_STATUS_PENDING,
   indicating that the writereg is performed in the background

   A callback function indicates when the non-blocking operation
   is complete. Because writereg is ongoing after this function returns,
   other operations that must run after flash writereg, should not be performed
   before the callback function is called.

   @param[in] reg_opcode   Operation code.
   @param[in] len         The length of register value to be written.
   @param[in] reg_value    The written value.
   @param[in] write_cb  Flash writereg callback function for a non-blocking
                          writereg. After writereg is done, the WriteRegCB
                          is called to inform about the completion; it
                          should be NULL for a blocking operation.
   @param[in] user_param   The user-specified parameter for the callback
                          function.

   @return
   FLASH_DEVICE_DONE -- If blocking writereg completed successfully. \n
   FLASH_STATUS_PENDING -- Indicating non-blocking writereg is ongoing. \n
   Negative value -- If there was an error.
*/
FLASH_STATUS drv_flash_write_reg(uint8_t reg_opcode, uint8_t len, uint8_t *reg_value, flash_operation_cb_t write_cb,
                                 void *user_param);

/** @} */ /* end_addtogroup peripherals_flash */

#endif
