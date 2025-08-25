/*
 *Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/** @file qspi.h
   @brief qspi Services Interface definition.

   This module provide quad serial peripheral interface APIs, types, and
   definitions.
*/

#ifndef __FERM_QSPI_H__
#define __FERM_QSPI_H__

/*-------------------------------------------------------------------------
 * Include Files
 *-----------------------------------------------------------------------*/

#include "fwconfig_cmn.h"
#include "nt_flags.h"
#include <stdbool.h>

/** @addtogroup peripherals_qspi
@{
*/

/*-------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 *-----------------------------------------------------------------------*/

/*-------------------------------------------------------------------------
 * Type Declarations
 *-----------------------------------------------------------------------*/

/**
   Enumeration of QSPI DDR mode and multi IO mode.
*/
typedef enum {
    QSPI_SDR_1BIT_E = 1, /**< Single SPI, SDR mode. */
    QSPI_SDR_2BIT_E,     /**< Dual SPI, SDR mode. */
    QSPI_SDR_4BIT_E,     /**< Quad SPI, SDR mode. */
    QSPI_DDR_1BIT_E = 5, /**< Single SPI, DDR mode. */
    QSPI_DDR_2BIT_E,     /**< Dual SPI, DDR mode. */
    QSPI_DDR_4BIT_E      /**< Quad SPI, DDR mode. */
} qspi_mode_t;

/**
   Enumeration of QSPI transfer mode.
*/
typedef enum {
    QSPI_PIO_MODE_E,      /**< PIO mode. */
    QSPI_DMA_POLL_MODE_E, /**< DMA polling mode. */
    QSPI_DMA_INT_MODE_E,  /**< DMA interrupt mode. */
    QSPI_XIP_MODE_E,      /* XiP mode */
} qspi_transfer_mode_t;

/**
   Enumeration of QSPI xip region.
*/
typedef enum _qspi_xip_flash_region {
    QSPI_XIP_FLASH_REGION_0 = -1,  // do not support region #0
    QSPI_XIP_FLASH_REGION_1 = 0,
    QSPI_XIP_FLASH_REGION_2 = 1,
    QSPI_XIP_FLASH_REGION_3 = 2
} qspi_xip_flash_region;

/**
   Structur representing the QSPI cmd configurations.
*/
typedef struct qspi_cmd_s {
    uint8_t opcode;        /**< The instruction for the command. */
    uint8_t addr_bytes;    /**< The address bytes for the command, 3 bytes or 4 bytes,
                                 depend on the connected flash type. */
    uint8_t dummy_clocks;  /**< The dummy clock cycles for the command. */
    qspi_mode_t cmd_mode;  /**< The instruction mode. */
    qspi_mode_t addr_mode; /**< The address mode. */
    qspi_mode_t data_mode; /**< The data mode. */
    bool write;            /**< Read or write operation, TRUE: write, FALSE: read. */
} qspi_cmd_t;

/**
   @brief Prototype for a function called when interrupt is triggered.

   @param[in] status     The interrupt status.
   @param[in] user_param  User specified parameter provided when the callback is registered.
*/
typedef void (*qspi_isr_cb_t)(uint32_t status, void *user_param);

/**
   Structur representing the QSPI master configurations.
*/
typedef struct qspi_master_config_s {
    uint8_t chip_select;  /**< Chip select to assert during transaction (0 or 1). */
    bool clk_polarity;    /**< Clock polarity. */
    bool clk_phase;       /**< Clock phase. */
    uint32_t clk_freq;    /**< Bus clock frequency. */
    qspi_isr_cb_t isr_cb; /**< User registered isr callback. It should do minimal processing. */
    void *user_param;     /**< User specified parameter for the callback function. */
} qspi_master_config_t;

/*-------------------------------------------------------------------------
 * Function Declarations
 *-----------------------------------------------------------------------*/

/**
   @brief Initialize the QSPI and the HW it depends on (clocks, GPIOs).

   @param[in] config  The QSPI master configurations.

   @return true on success or false on failure.
*/
bool drv_qspi_init(qspi_master_config_t *config);

/**
   @brief De-initialize the QSPI.

   @return true on success or false on failure.
*/

bool drv_qspi_deinit();

/**
   @brief Set QSPI command parameters.

   @param[in] cmd          Pointer to the command.
   @param[in] opcode       The instruction code.
   @param[in] addr_bytes    The address bytes.
   @param[in] dummy_clocks  The dummy clock cycles.
   @param[in] cmd_mode      The instruction mode.
   @param[in] addr_mode     The address mode.
   @param[in] data_mode     The data mode.
   @param[in] write        Read or Write, true: write, false: read.

   @return true on success or false on failure.
*/
bool drv_qspi_prepare_cmd(qspi_cmd_t *cmd, uint8_t opcode, uint8_t addr_bytes, uint8_t dummy_clocks,
                          qspi_mode_t cmd_mode, qspi_mode_t addr_mode, qspi_mode_t data_mode, bool write);

/**
   @brief Run QSPI command
   When calling this API in multi-thread env, need to be protected by a lock.

   @param[in]    cmd        The command configuration.
   @param[in]    addr       The address for the command.
   @param[inout] data       The data buffer containing data to write,
                            or as buffer for reading data.
   @param[in]    data_bytes  The number of data to write or read.
   @param[in]    enable_dma  The command transfer mode.

   @return true on success or false on failure.
*/
bool drv_qspi_run_cmd(qspi_cmd_t *cmd, uint32_t addr, uint8_t *data, uint32_t data_bytes,
                      qspi_transfer_mode_t enable_dma);

/**
   Default is XiP mode, so when do PIO operations, need to do switch.

   @return true on success or false on failure.
 */
bool drv_qspi_disable_xip_mode();

/**
   After PIO operations, restore to original mode.

   @return true on success or false on failure.
 */
bool drv_qspi_restore_xip_mode();

/**
   SW signal to indicate Program/Erase operation is on going
   XIP HW uses this flag to send Suspend/Resume commands to
   stop the Program/Erase operations for XIP instruction fetch

   @param state [in]
    Indicates Program/Erase operation is active or not

   @return true on success or false on failure.
 */

bool drv_qspi_xip_set_pe_state(uint8_t state);

/**
    Configure the controller so that when XIP instruction fetch happens, the
    controller would send suspend/resume and delays between suspend/resume oprations.

    @param suspend_delay [in]
    @param suspend_opcode [in]
    @param resume_delay [in]
    @param resume_opcode [in]

    @return true on success or false on failure.
 */
bool drv_qspi_xip_config_suspend_resume(uint16_t suspend_delay, uint8_t suspend_opcode, uint16_t resume_delay,
                                        uint8_t resume_opcode);

int32_t drv_qspi_xip_config(qspi_xip_flash_region region_id, uint32_t region_size, uint32_t regigon_addr);

/** @} */

#endif
