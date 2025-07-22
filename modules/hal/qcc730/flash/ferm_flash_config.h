/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/** @file ferm_flash_config.h
   @brief Add all flash configurations here.
*/

/** @addtogroup peripherals_flash

  The QSPI Flash module provides all flash configurations.

*/

#ifndef __FERM_FLASH_CONFIG_H__
#define __FERM_FLASH_CONFIG_H__

/*-------------------------------------------------------------------------
 * Include Files
 *-----------------------------------------------------------------------*/
#include "FreeRTOS.h"

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

/* Flash clock depends on qspi clock which is defined by HW */
typedef enum {
    FLASH_CLOCK_30MHZ,
    FLASH_CLOCK_15MHZ,
    FLASH_CLOCK_10MHZ,
    FLASH_CLOCK_7MHZ,
    FLASH_CLOCK_6MHZ,
    FLASH_CLOCK_5MHZ,
    FLASH_CLOCK_4MHZ
} flash_clock_t;
/**
   Structure representing the flash configuration table.
   This must be configured based on the specific flash type.
*/
typedef struct flash_config_data_s {
    uint8_t addr_bytes;    /**< 3Bytes or 4Bytes addressing mode. */
    uint8_t read_cmd_mode; /**< Read operation command mode, QAPI_FLASH_RW_MODE_XXX. QUP access only supports single and
                              quad mode. */
    uint8_t read_addr_mode;   /**< Read operation address mode, QAPI_FLASH_RW_MODE_XXX. QUP access only supports single
                                 and quad mode. */
    uint8_t read_data_mode;   /**< Read operation data mode, QAPI_FLASH_RW_MODE_XXX. QUP access only supports single and
                                 quad mode. */
    uint8_t read_opcode;      /**< Read opcode. */
    uint8_t read_wait_state;  /**< Read wait state is the total dummy cycles to wait. */
    uint8_t write_cmd_mode;   /**< Write operation address mode, QAPI_FLASH_RW_MODE_XXX. */
    uint8_t write_addr_mode;  /**< Write operation address mode, QAPI_FLASH_RW_MODE_XXX. */
    uint8_t write_data_mode;  /**< Write operation data mode, QAPI_FLASH_RW_MODE_XXX. */
    uint8_t write_opcode;     /**< Opcode used for write. */
    uint8_t erase_4kb_opcode; /**< Opcode used for 4KB block erase. */
    uint8_t bulk_erase_size_4kb; /**< Bulk erase size. Unit is 4KB. */
    uint8_t bulk_erase_opcode;   /**< The opcode for the supported bulk erase size (larger than a 4KB block erase). If
                                    bulk erase is not supported, set to 0. */
    uint8_t chip_erase_opcode;   /**< Opcode used for chip erase. */
    uint8_t quad_enable_mode; /**< Quad Enable Requirements (QER) as defined in the JEDEC Standard No. 216A Document. */
    uint8_t suspend_erase_opcode;   /**< Instruction to suspend an in-progress erase. */
    uint8_t suspend_program_opcode; /**< Instruction to suspend an in-progress program. */
    uint8_t resume_erase_opcode;    /**< Instruction to resume an erase operation. */
    uint8_t resume_program_opcode;  /**< Instruction to resume a program operation. */
    uint8_t erase_err_bmsk; /**< Status BIT(s) in the EraseErrStatusReg register; indicating if there is an erase error
                               condition. */
    uint8_t erase_err_status_reg; /**< Register address used for polling the erase status. */
    uint8_t write_err_bmsk; /**< Status BIT(s) in the WriteErrStatusReg register; indicating if there is a write error
                               condition. */
    uint8_t write_err_status_reg;        /**< Register address used for polling the write status. */
    uint8_t high_performance_mode_bmask; /**< High Performance Bit(s). Non-Macronix parts set to 0. This field is used
                                           to enable High Performance mode supported on some Macronix parts. */
    uint8_t power_on_delay_in_us;        /**< Power On Reset delay in 100us units. */
    uint16_t suspend_erase_delay_in_us;  /**< Delay needed after suspending an in-progress erase; see JEDEC Standard No.
                                            216A for a definition. */
    uint16_t suspend_program_delay_in_us; /**< Delay needed after suspending an in-progress program; see JEDEC Standard
                                             No. 216A for a definition. */
    uint16_t resume_erase_delay_in_us; /**< Delay needed after sending a resume erase; see JEDEC Standard No. 216A for a
                                          definition. */
    uint16_t resume_program_delay_in_us; /**< Delay needed after sending a resume program; see JEDEC Standard No. 216A
                                            for a definition. */
    uint32_t density_in_blocks;          /**< Device density in unit of Blocks. */
    uint32_t device_id;                  /**< Device ID when querying with a Device Read ID command 0x9F. */
    uint32_t write_protect_bmask;        /**< Write Block Protection Mask. */
    flash_clock_t clk_freq;              /* decide qspi clock */
} flash_config_data_t;

#ifdef CONFIG_BOARD_QCC730_QSPI_ENABLE
/*-------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 *-----------------------------------------------------------------------*/

/*-------------------------------------------------------------------------
 * Function Definitions
 *-----------------------------------------------------------------------*/
/**
   @brief get all flash configuration.

   * Get the pointer to the supported NOR devices and their parameters
   *
   * @return void* [OUT]
   *   Pointer to the table that contains the Flash's NOR config parameters
   *   needed for flash operations
*/
void *flash_get_config_entries_struct(void);

/**
   @brief Get the number of entries in the table.

   Total number of entries in the table.

   @return
   Total number of entries in the table.
*/
uint32_t flash_get_config_entries_count();

#endif  // FERMION_QSPI_SUPPORT

/** @} */ /* end_addtogroup peripherals_flash */

#endif
