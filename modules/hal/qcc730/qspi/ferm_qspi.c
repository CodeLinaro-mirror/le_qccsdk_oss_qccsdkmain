/*
 *Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*-------------------------------------------------------------------------
 * Include Files
 *-----------------------------------------------------------------------*/

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "fwconfig_cmn.h"
#include "nt_flags.h"
#include "nt_hw.h"
#include "nt_osal.h"
#include "nt_logger_api.h"
#include "nt_common.h"

#include "ferm_qspi_hal.h"
#include "ferm_qspi.h"

#include "autoconf.h"

#ifdef CONFIG_BOARD_QCC730_QSPI_ENABLE

/*-------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 *-----------------------------------------------------------------------*/
#define BLOCK_SIZE_IN_BYTES 4096
#define MAX_READ_WAIT       0x20000
#define MAX_WRITE_WAIT      0x2000
#define INT_TO_PTR(__x__)   ((void *)(uint32_t)(__x__))

/*-------------------------------------------------------------------------
 * Type Declarations
 *-----------------------------------------------------------------------*/

/**
   Enumeration of QSPI transfer direction.
*/
typedef enum {
    QSPI_MASTER_READ_E, /**< QSPI read. */
    QSPI_MASTER_WRITE_E /**< QSPI write. */
} qspi_transfer_dir_t;

/**
   Structur representing the QSPI descriptors.
*/
typedef struct qspi_descriptor_s {
    uint32_t data_address;    /**< Descriptor offset  0 = data address[ 7: 0],
                                  descriptor offset  1 = data address[15: 8],
                                  descriptor offset  2 = data address[23:16],
                                  descriptor offset  3 = data address[31:24]. */
    uint32_t next_descriptor; /**< Descriptor offset  4 = next descriptor[ 7: 0],
                                  descriptor offset  5 = next descriptor[15: 8],
                                  descriptor offset  6 = next descriptor[23:16],
                                  descriptor offset  7 = next descriptor[31:24]. */
    uint32_t direction : 1;   /**< Descriptor offset  8 = mode/dir. */
    uint32_t multi_io_mode : 3;
    uint32_t reserved1 : 4;
    uint32_t fragment : 1; /**< Descriptor offset  9 = fragment. */
    uint32_t reserved2 : 7;
    uint32_t length : 16; /**< Descriptor offset 10 = transfer length[ 7:0],
                               descriptor offset 11 = transfer length[15:8]. */
    uint32_t bounce_src;
    uint32_t bounce_dst;
    uint32_t bounce_length;
    uint32_t padding[2];
} qspi_descriptor_t;

/**
   Structur representing context for QSPI module.
*/
typedef struct qspi_context_s {
    qspi_hal *hal;                      /**< qspi hal layer handler */
    qspi_transfer_mode_t transfer_mode; /**< Record transfer mode. */
    uint8_t *free_ptr;                  /**< Dma free buffer pointer. */
    qspi_isr_cb_t isr_cb;               /**< User registered isr callback. */
    void *user_param;                   /**< User specified parameter for the callback function. */
} qspi_context_t;

/*-------------------------------------------------------------------------
 * Variables
 *-----------------------------------------------------------------------*/

static qspi_descriptor_t qspi_descriptors[8] __attribute__((aligned(32)));
static uint8_t qspi_transfer_buffer[32 * 4] __attribute__((aligned(32)));
static qspi_descriptor_t *qspi_transfer_chain = NULL;
static qspi_context_t qspi_context;

/*-------------------------------------------------------------------------
 * Private Function Declarations
 *-----------------------------------------------------------------------*/

/*-------------------------------------------------------------------------
 * Function Definitions
 *-----------------------------------------------------------------------*/
/**
   @brief Read data from the RX FIFO.

   @param[in] ptr       The pointer stored read data.
   @param[in] num_bytes  Total number of data to read.

   @return The number of data actually read.
*/
static uint32_t drv_qspi_service_rxfifo(uint8_t *ptr, uint32_t num_bytes)
{
    uint32_t full_words = num_bytes >> 2;
    uint32_t prtial_bytes = num_bytes & 0x03;
    uint8_t bytes_in_fifo;
    uint8_t words_in_fifo;
    uint32_t words_to_read;
    uint32_t bytes_to_read;
    uint32_t word_value;
    uint32_t i = 0;

    bytes_in_fifo = hal_qspi_get_pio_rd_fifo_wrcnts(qspi_context.hal);
    words_in_fifo = bytes_in_fifo >> 2;

    words_to_read = (words_in_fifo < full_words) ? (words_in_fifo) : (full_words);
    bytes_to_read = (bytes_in_fifo < num_bytes) ? (0) : (prtial_bytes);

    for (i = 0; i < words_to_read; i++) {
        word_value = hal_qspi_get_pio_rd_fifo(qspi_context.hal, i);
        *(uint32_t *)ptr = word_value;  // ptr is word aligned
        ptr += 4;
    }

    if (bytes_to_read) {
        word_value = hal_qspi_get_pio_rd_fifo(qspi_context.hal, i);
        for (i = 0; i < bytes_to_read; i++) {
            ptr[i] = (uint8_t)(word_value >> i * 8);
        }
    }

    return (words_to_read * 4 + bytes_to_read);
}

/**
   @brief Write data to the TX FIFO.

   @param[in] ptr       The pointer stored data to be written.
   @param[in] num_bytes  Total number of data to be written.

   @return The number of data actually been written.
*/
static uint32_t drv_qspi_service_txfifo(uint8_t *ptr, uint32_t num_bytes)
{
    uint32_t full_words = num_bytes >> 2;
    uint32_t partial_bytes = num_bytes & 0x03;
    // uint32_t pio_wrfifo_status;
    uint32_t room_in_bytes;
    uint32_t room_in_words;
    uint32_t words_to_send;
    uint32_t bytes_to_send;
    uint32_t word_value;
    uint32_t i;

    room_in_bytes = hal_qspi_get_pio_transfer_wr_fifo_bytes(qspi_context.hal);
    room_in_words = room_in_bytes >> 2;

    words_to_send = (full_words > room_in_words) ? (room_in_words) : (full_words);
    bytes_to_send = (num_bytes > room_in_bytes) ? (0) : (partial_bytes);

    for (i = 0; i < words_to_send; i++) {
        word_value = *(uint32_t *)ptr;  // ptr is word aligned
        hal_qspi_pio_write_4bytes(qspi_context.hal, word_value);
        ptr += 4;
    }

    for (i = 0; i < bytes_to_send; i++) {
        hal_qspi_pio_write_1bytes(qspi_context.hal, *ptr);
        ptr++;
    }

    return (words_to_send * 4 + bytes_to_send);
}

/**
   @brief Perform a PIO read transfer using a single transaction.

   @param[in] buffer             The pointer stored read data.
   @param[in] num_bytes           Total number of data to read.
   @param[in] pio_transfer_config  PIO transfer configurations.
*/
static void drv_qspi_pio_read(uint8_t *buffer, uint32_t num_bytes, uint8_t write, uint8_t io_mode, uint8_t fragment)
{
    uint8_t *ptr = buffer;
    uint32_t bytes_left = num_bytes;
    uint32_t bytes_read;
    uint32_t retry = 0;

    hal_qspi_set_pio_config(qspi_context.hal, write, io_mode, fragment);
    hal_qspi_set_pio_transfer_control_request_count(qspi_context.hal, num_bytes);

    /* Poll until all data has been read from the RX FIFO. */
    while (bytes_left && retry++ < MAX_READ_WAIT) {
        bytes_read = drv_qspi_service_rxfifo(ptr, bytes_left);
        bytes_left -= bytes_read;
        ptr += bytes_read;
    }
    if (retry == MAX_READ_WAIT)
        NT_LOG_PRINT(SYSTEM, ERR, "rx timeout");
}

/**
   @brief Perform a PIO write transfer using a single transaction.

   @param[in] buffer             The pointer stored data to be written.
   @param[in] numBytes           Total number of data to be written.
   @param[in] pio_transfer_config  PIO transfer configurations.
*/
static void drv_qspi_pio_write(uint8_t *buffer, uint32_t num_bytes, uint8_t write, uint8_t io_mode, uint8_t fragment)
{
    uint8_t *ptr = buffer;
    uint32_t bytes_left = num_bytes;
    uint32_t bytes_written;
    //    uint32_t value;
    uint32_t retry = 0;

    /* Clear latched status bits. */
    hal_qspi_clear_isr_status(qspi_context.hal);
    hal_qspi_set_pio_config(qspi_context.hal, write, io_mode, fragment);
    hal_qspi_set_pio_transfer_control_request_count(qspi_context.hal, num_bytes);

    /* Poll until all data has been written to the TX FIFO. */
    while (bytes_left) {
        bytes_written = drv_qspi_service_txfifo(ptr, bytes_left);
        bytes_left -= bytes_written;
        ptr += bytes_written;
    }

    while (1) {
        if (hal_qspi_check_pio_transaction_done(qspi_context.hal) || retry++ >= MAX_WRITE_WAIT)
            break;
    }

    if (retry == MAX_WRITE_WAIT)
        NT_LOG_PRINT(SYSTEM, ERR, "tx timeout");
}

/**
   @brief Transfer a chain of descriptors over the bus using PIO mode.

   @param[in] chain  QSPI descriptor chain to be transfered.
*/
static void drv_qspi_pio_transfer_chain(qspi_descriptor_t *pchain)
{
    qspi_descriptor_t *pdesc = pchain;
    bool write;
    uint8_t *buffer;
    uint32_t num_bytes;

    while (pdesc) {
        write = (pdesc->direction == QSPI_MASTER_WRITE_E);
        buffer = INT_TO_PTR(pdesc->data_address);
        num_bytes = pdesc->length;

        if (write) {
            drv_qspi_pio_write(buffer, num_bytes, write, pdesc->multi_io_mode, pdesc->fragment);
        } else {
            drv_qspi_pio_read(buffer, num_bytes, write, pdesc->multi_io_mode, pdesc->fragment);
        }

        pdesc = INT_TO_PTR(pdesc->next_descriptor);
    }
}

/**
   @brief Flush the existing descriptor chain to hardware.

   @param[in] transfer_mode QSPI transfer mode.
*/
static void drv_qspi_flush_chain(qspi_transfer_mode_t transfer_mode)
{
    qspi_descriptor_t *pdesc = qspi_transfer_chain;
    uint8_t *psrc;
    uint8_t *pdst;
    uint32_t i;

    if (transfer_mode == QSPI_PIO_MODE_E) {
        drv_qspi_pio_transfer_chain(qspi_transfer_chain);
    }

    if (transfer_mode != QSPI_DMA_INT_MODE_E) {
        /* Copy any bounce data from the descriptors to the client buffer (for reads only). */
        while (pdesc) {
            psrc = INT_TO_PTR(pdesc->bounce_src);
            pdst = INT_TO_PTR(pdesc->bounce_dst);

            for (i = 0; i < pdesc->bounce_length; i++) {
                pdst[i] = psrc[i];
            }

            pdesc = INT_TO_PTR(pdesc->next_descriptor);
        }

        qspi_context.free_ptr = qspi_transfer_buffer;
        qspi_transfer_chain = NULL;
    }
}

/**
   @brief Get the last descriptor in the chain.

   @return Returns a pointer to the last descriptor in the chain or Null if no
           chain exists.
*/
static qspi_descriptor_t *drv_qspi_get_last_descriptor()
{
    qspi_descriptor_t *pdesc = qspi_transfer_chain;

    if (pdesc) {
        while (pdesc->next_descriptor) {
            pdesc = INT_TO_PTR(pdesc->next_descriptor);
        }
    }

    return pdesc;
}

/**
   @brief Allocates a new descriptor, adds it to the chain, and returns a pointer to
   the new descriptor.

   If no descriptors are available or there is not enough free space in the DMA buffer
   then the existing chain will be flushed to HW and a new chain will be created.

   @param[in] bytes_needed  The number of bytes that needed.
   @param[in] transfer_mode  The transfer mode.

   @return Returns a pointer to the new descriptor.
*/
static qspi_descriptor_t *drv_qspi_alloc_descriptor(uint32_t bytes_needed, qspi_transfer_mode_t transfer_mode)
{
    uint32_t alignment;
    uint8_t *free_ptr;
    uint32_t room;
    qspi_descriptor_t *current;
    qspi_descriptor_t *next;

    alignment = (transfer_mode != QSPI_PIO_MODE_E) ? 32 : 4;
    free_ptr = qspi_context.free_ptr;
    free_ptr = (uint8_t *)(((uint32_t)free_ptr + alignment - 1) & (~(alignment - 1)));
    room = qspi_transfer_buffer + sizeof(qspi_transfer_buffer) - free_ptr;

    current = drv_qspi_get_last_descriptor();
    if (current) {
        next = current + 1;
    } else {
        next = &qspi_descriptors[0];
    }

    if ((uint8_t *)next >= (uint8_t *)qspi_descriptors + sizeof(qspi_descriptors)) {
        next = NULL;
    }

    /* Flushing existing chain if necessary */
    if (room < bytes_needed || next == NULL) {
        /* Can not use DMA interrupt here. */
        drv_qspi_flush_chain((transfer_mode != QSPI_DMA_INT_MODE_E) ? transfer_mode : QSPI_DMA_POLL_MODE_E);
        free_ptr = qspi_transfer_buffer;
        current = NULL;
        next = &qspi_descriptors[0];
    }

    /* Populate descriptor */
    memset(next, 0, sizeof(qspi_descriptor_t));
    next->data_address = bytes_needed ? (uint32_t)free_ptr : 0;
    next->direction = QSPI_MASTER_READ_E;
    next->fragment = 1;

    if (current) {
        current->next_descriptor = (uint32_t)next;
    } else {
        qspi_transfer_chain = next;
    }

    return next;
}

/**
   @brief Calculate the room available in the last descriptor in the chain (the
   caller must pass in a pointer to the *last* descriptor).

   @param[in] last  The last descriptor in the chain.

   @return Returns avilable room in the last descriptor.
*/
static uint32_t drv_qspi_get_available_room(qspi_descriptor_t *last)
{
    uint8_t *end_of_buffer = qspi_transfer_buffer + sizeof(qspi_transfer_buffer);

    if (INT_TO_PTR(last->data_address) < (void *)qspi_transfer_buffer ||
        INT_TO_PTR(last->data_address) >= (void *)end_of_buffer) {
        return 0;
    }

    return (uint32_t)(end_of_buffer - (last->data_address + last->length));
}

/**
   @brief Setting descriptors.

   @param[in] data       The data buffer containing data to write, or as buffer for reading data.
   @param[in] data_bytes  The number of data.
   @param[in] mode       The multi IO and DDR mode.
   @param[in] direction  Transfer direction.
   @param[in] transfer_mode    QSPI transfer mode.
*/
static void drv_qspi_set_descriptor(uint8_t *data, uint32_t data_bytes, qspi_mode_t mode, qspi_transfer_dir_t direction,
                                    qspi_transfer_mode_t transfer_mode)
{
    qspi_descriptor_t *pdesc;
    uint8_t *ptr;
    uint32_t i;

    if (data_bytes == 0 || data == NULL) {
        return;
    }

    pdesc = drv_qspi_get_last_descriptor();

    /* Check if we can add to the last descriptor else allocate a new one */
    if ((pdesc == NULL) || (pdesc->direction != direction) || (pdesc->multi_io_mode != mode) ||
        (drv_qspi_get_available_room(pdesc) < data_bytes)) {
        pdesc = drv_qspi_alloc_descriptor(data_bytes, transfer_mode);
        pdesc->direction = direction;
        pdesc->multi_io_mode = mode;
    }

    ptr = INT_TO_PTR(pdesc->data_address + pdesc->length);

    if (direction == QSPI_MASTER_WRITE_E) {
        for (i = 0; i < data_bytes; i++) {
            ptr[i] = data[i];
        }
    } else {
        pdesc->bounce_src = (uint32_t)ptr;
        pdesc->bounce_dst = (uint32_t)data;
        pdesc->bounce_length = data_bytes;
    }

    pdesc->length += data_bytes;
    qspi_context.free_ptr = INT_TO_PTR(pdesc->data_address + pdesc->length);
}

/**
   @brief Queue the 1-byte command opcode (cmd-ADDR-dummy-data).

   @param[in] opcode   The command opcode
   @param[in] cmd_mode  The command mode.
   @param[in] transfer_mode  QSPI transfer mode.
*/
static void drv_qspi_queue_opcode(uint8_t opcode, qspi_mode_t cmd_mode, qspi_transfer_mode_t transfer_mode)
{
    drv_qspi_set_descriptor(&opcode, 1, cmd_mode, QSPI_MASTER_WRITE_E, transfer_mode);
}

/**
   @brief Queue the 3-byte or 4-byte address (cmd-ADDR-dummy-data).

   @param[in] addr       The address.
   @param[in] addr_bytes  The address bytes.
   @param[in] addr_mode   The address mode.
   @param[in] transfer_mode    QSPI transfer mode.
*/
static void drv_qspi_queue_addr(uint32_t addr, uint8_t addr_bytes, qspi_mode_t addr_mode,
                                qspi_transfer_mode_t transfer_mode)
{
    uint8_t write_addr[4];
    uint8_t i;

    if (addr_bytes == 0) {
        return;
    }

    for (i = addr_bytes; i > 0; i--) {
        write_addr[addr_bytes - i] = (uint8_t)(addr >> (i - 1) * 8);
    }

    drv_qspi_set_descriptor(write_addr, (uint32_t)addr_bytes, addr_mode, QSPI_MASTER_WRITE_E, transfer_mode);
}

/**
   @brief Queue the dummy bytes to generate dummy clock cycles (cmd-addr-DUMMY-data).

   @param[in] DummyClocks  The dummy clock cycles.
   @param[in] DmaMode      QSPI transfer mode.
*/
static void drv_qspi_queue_dummy(uint8_t dummy_clocks, qspi_transfer_mode_t transfer_mode)
{
    qspi_descriptor_t *pdesc;
    uint8_t *ptr = NULL;
    uint32_t i;
    uint32_t dummy_bytes;
    uint32_t clocks_per_bytes;
    uint32_t remainder;

    if (dummy_clocks == 0) {
        return;
    }

    pdesc = drv_qspi_get_last_descriptor();

    /* Check if we can add to the last descriptor else allocate a new one */
    if (pdesc && pdesc->direction == QSPI_MASTER_WRITE_E) {
        switch (pdesc->multi_io_mode) {
            case QSPI_SDR_1BIT_E:
                clocks_per_bytes = 8;
                break;

            case QSPI_SDR_2BIT_E:
                clocks_per_bytes = 4;
                break;

            case QSPI_SDR_4BIT_E:
                clocks_per_bytes = 2;
                break;

            case QSPI_DDR_1BIT_E:
                clocks_per_bytes = 4;
                break;

            case QSPI_DDR_2BIT_E:
                clocks_per_bytes = 2;
                break;

            case QSPI_DDR_4BIT_E:
                clocks_per_bytes = 1;
                break;

            default:
                return;
        }

        dummy_bytes = dummy_clocks / clocks_per_bytes;
        remainder = dummy_clocks % clocks_per_bytes;

        if (remainder == 0 && drv_qspi_get_available_room(pdesc) >= dummy_bytes) {
            ptr = INT_TO_PTR(pdesc->data_address + pdesc->length);
        }
    }
    if (ptr == NULL) {
        NT_LOG_PRINT(SYSTEM, ERR, " dummy:%x", dummy_clocks);
        dummy_bytes = dummy_clocks / 2;
        pdesc = drv_qspi_alloc_descriptor(dummy_bytes, transfer_mode);
        pdesc->direction = QSPI_MASTER_WRITE_E;
        pdesc->multi_io_mode = QSPI_SDR_4BIT_E;
        ptr = INT_TO_PTR(pdesc->data_address);
    }

    /* Write dummy bytes to descriptor */
    for (i = 0; i < dummy_bytes; i++) {
        *ptr++ = 0xFF;
    }

    pdesc->length += dummy_bytes;
    qspi_context.free_ptr = INT_TO_PTR(pdesc->data_address + pdesc->length);
}

/**
   @brief Queue direct data. This is data that will be copied directly between the client
          buffer and the hardware (and must be properly aligned).

   @param[in] data       The data buffer containing data to write, or as buffer for reading data.
   @param[in] data_bytes  The number of data.
   @param[in] data_mode   The data mode.
   @param[in] write      Write or read, true: write, false: read.
   @param[in] transfer_mode    QSPI transfer mode.
*/
static void drv_qspi_queue_data_direct(uint8_t *data, uint32_t data_bytes, qspi_mode_t data_mode, bool write,
                                       qspi_transfer_mode_t transfer_mode)
{
    qspi_descriptor_t *pdesc;
    uint32_t chunk_size;

    while (data_bytes) {
        /* Maximum transfer size is 0xFFFF bytes but 0xFFE0 is the largest size that
           maintains 32-byte alignment between chunks. */
        chunk_size = (data_bytes > 0xFFE0) ? (0xFFE0) : (data_bytes);

        /* Allocate a new descriptor and point it to the client buffer. */
        pdesc = drv_qspi_alloc_descriptor(0, transfer_mode);
        pdesc->direction = write;
        pdesc->multi_io_mode = data_mode;
        pdesc->data_address = (uint32_t)data;
        pdesc->length = chunk_size;

        data += chunk_size;
        data_bytes -= chunk_size;
    }
}

/**
   @brief Queue the data from the client buffer.

   The data may be split into multiple parts to ensure the following:
   1) the buffer starts on a 32-byte boundary (for DMA) or a 4-byte boundary (for PIO)
   2) the buffer ends on a 4-byte boundary (only required for DMA reads)
   3) the buffer is no larger than 65535 (0xFFFF) bytes.
   It is straightforward to handle (3) by splitting the buffer across multiple descriptors.
   Bounce buffers are used to handle (1) and (2).

   @param[in] data       The data buffer containing data to write, or as buffer for reading data.
   @param[in] data_bytes  The number of data.
   @param[in] data_mode   The data mode.
   @param[in] write      Write or read, true: write, false: read.
   @param[in] transfer_mode    QSPI transfer mode.
*/
static void drv_qspi_queue_data(uint8_t *data, uint32_t data_bytes, qspi_mode_t data_mode, bool write,
                                qspi_transfer_mode_t transfer_mode)
{
    uint32_t alignment;
    uint8_t *aligned_ptr;
    uint8_t *epilog_ptr;
    uint32_t prolog_bytes;
    uint32_t aligned_bytes;
    uint32_t epilog_bytes;

    if (data_bytes == 0) {
        return;
    }

    alignment = (transfer_mode != QSPI_PIO_MODE_E) ? 32 : 4;
    aligned_ptr = (uint8_t *)(((uint32_t)data + alignment - 1) & (~(alignment - 1)));
    prolog_bytes = ((uint32_t)(aligned_ptr - data) > data_bytes) ? (data_bytes) : (uint32_t)(aligned_ptr - data);
    epilog_bytes = ((transfer_mode != QSPI_PIO_MODE_E) && !write) ? ((data_bytes - prolog_bytes) & 0x3) : 0;
    aligned_bytes = data_bytes - prolog_bytes - epilog_bytes;
    epilog_ptr = data + prolog_bytes + aligned_bytes;

    if (aligned_bytes == 0) {
        /* Combine prolog and epilog if there is nothing between them.  This avoids
           the need to have two bounce buffers in the same descriptor. */
        prolog_bytes += epilog_bytes;
        epilog_bytes = 0;
    }

    if (prolog_bytes) {
        drv_qspi_set_descriptor(data, prolog_bytes, data_mode, (qspi_transfer_dir_t)write, transfer_mode);
    }
    if (aligned_bytes) {
        drv_qspi_queue_data_direct(aligned_ptr, aligned_bytes, data_mode, write, transfer_mode);
    }
    if (epilog_bytes) {
        drv_qspi_set_descriptor(epilog_ptr, epilog_bytes, data_mode, (qspi_transfer_dir_t)write, transfer_mode);
    }
}

bool drv_qspi_init(qspi_master_config_t *config)
{
    uint8_t enable = 1;      /* enable qspi function if needed */
    uint8_t pads_option = 0; /* qspi pads mode*/
    memset(&qspi_context, 0, sizeof(qspi_context_t));

#if CONFIG_SOC_QCC730V1
    qspi_context.hal = (qspi_hal *)QCC730V1_QSPI_BASE_BASE;
#elif CONFIG_SOC_QCC730V2
    qspi_context.hal = (qspi_hal *)QCC730V2_QSPI_BASE_BASE;
#endif

    qspi_context.transfer_mode = QSPI_PIO_MODE_E;

    /* Add init code below */

#if CONFIG_SOC_QCC730V1 && CONFIG_BOARD_QCC730_QSPI_ENABLE
    pads_option = CONFIG_BOARD_QCC730_QSPI_V1_OPTION;
#elif CONFIG_SOC_QCC730V2 && CONFIG_BOARD_QCC730_QSPI_ENABLE
    pads_option = CONFIG_BOARD_QCC730_QSPI_V2_QUAD_MODE;
#else
    //"CONFIG_SOC_QCC730Vx and CONFIG_BOARD_QCC730_QSPI_ENABLE should be defined. See boad_defconfig"
    return false;
#endif
    hal_qspi_mcu_enable_qspi();

#ifndef CONFIG_NON_OS
    if (!hal_qspi_is_qspi_active()) {
#endif
        NT_LOG_PRINT(SYSTEM, INFO, "do qspi init...\n");
        hal_qspi_enable_qspi(enable, pads_option);
        hal_qspi_set_clock((uint8_t)config->clk_freq);

        /* wait for QSPI to be powered up */
        hal_qspi_qspi_gdscr_config();
        while (!hal_qspi_gdscr_pwr_ready())
            ;

        /* disable clock gating */
        hal_qspi_enable_clock_gating(0);

        hal_qspi_set_master_config_wpn(qspi_context.hal, 1);
        hal_qspi_set_master_config_holdn(qspi_context.hal, 1);

        if (!hal_qspi_xip_is_enabled(qspi_context.hal)) {
            /* reset registers */
            /* TODO: if XIP is enabled in bootloader, we need a careful reset, now use default value.*/
            hal_qspi_master_status_reset(qspi_context.hal);
        }
#ifndef CONFIG_NON_OS
    }
#endif
    /* Regiter ISR callback. */
    qspi_context.isr_cb = config->isr_cb;
    qspi_context.user_param = config->user_param;

    /* init the free pointer */
    qspi_context.free_ptr = qspi_transfer_buffer;

    return true;
}

/**
   @brief Deinit QSPI module
   Deinit QSPI module when needed.

   @return TRUE on success or FALSE on failure.
*/
bool drv_qspi_deinit()
{
    /* reset registers */
    hal_qspi_master_status_reset(qspi_context.hal);

    /* enable clock gating */
    hal_qspi_enable_clock_gating(1);

    /* disbale QSPI */
    hal_qspi_enable_qspi(0, 0);

    return true;
}

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

   @return TRUE on success or FALSE on failure.
*/
bool drv_qspi_prepare_cmd(qspi_cmd_t *cmd, uint8_t opcode, uint8_t addr_bytes, uint8_t dummy_clocks,
                          qspi_mode_t cmd_mode, qspi_mode_t addr_mode, qspi_mode_t data_mode, bool write)
{
    if (cmd == NULL) {
        return false;
    }

    memset(cmd, 0, sizeof(qspi_cmd_t));
    cmd->opcode = opcode;
    cmd->addr_bytes = addr_bytes;
    cmd->dummy_clocks = dummy_clocks;
    cmd->cmd_mode = cmd_mode;
    cmd->addr_mode = addr_mode;
    cmd->data_mode = data_mode;
    cmd->write = write;

    return true;
}

/**
   @brief Send QSPI command
   When calling this API in multi-thread env, need to be protected by a lock.

   @param[in]    cmd        The command configuration.
   @param[in]    addr       The address for the command.
   @param[inout] data       The data buffer containing data to write,
                            or as buffer for reading data.
   @param[in]    data_bytes  The number of data to write or read.
   @param[in]    transfer_mode  The command transfer mode.

   @return TRUE on success or FALSE on failure.
*/
bool drv_qspi_run_cmd(qspi_cmd_t *cmd, uint32_t addr, uint8_t *data, uint32_t data_bytes,
                      qspi_transfer_mode_t transfer_mode)
{
    qspi_descriptor_t *last;

    if (cmd == NULL || cmd->addr_bytes > 4 || cmd->dummy_clocks > 16) {
        return false;
    }

    drv_qspi_queue_opcode(cmd->opcode, cmd->cmd_mode, transfer_mode);
    drv_qspi_queue_addr(addr, cmd->addr_bytes, cmd->addr_mode, transfer_mode);
    drv_qspi_queue_dummy(cmd->dummy_clocks, transfer_mode);
    drv_qspi_queue_data(data, data_bytes, cmd->data_mode, cmd->write, transfer_mode);

    last = drv_qspi_get_last_descriptor();
    last->fragment = 0;
    drv_qspi_flush_chain(transfer_mode);

    return true;
}

/**
   Default is XiP mode, so when do PIO operations, need to do switch.

   @return true on success or false on failure.
 */
bool drv_qspi_disable_xip_mode()
{
    if (!hal_qspi_xip_is_enabled(qspi_context.hal))
        return true;

    hal_qspi_xip_enable(qspi_context.hal, 0);
    while (1) {
        if (!hal_qspi_get_xip_is_active(qspi_context.hal))
            break;
    }
    return true;
}

/**
   After PIO operations, restore to original mode.

   @param mode [IN]
    which mode to restore, PIO or XiP.

   @return true on success or false on failure.
 */
bool drv_qspi_restore_xip_mode()
{
    if (!hal_qspi_xip_is_enabled(qspi_context.hal)) {
        while (1) {
            if (hal_qspi_check_pio_transaction_done(qspi_context.hal))
                break;
        }
        hal_qspi_xip_enable(qspi_context.hal, 1);
    }
    return true;
}

/**
   SW signal to indicate Program/Erase operation is on going
   XIP HW uses this flag to send Suspend/Resume commands to
   stop the Program/Erase operations for XIP instruction fetch

   @param state [in]
    Indicates Program/Erase operation is active or not

   @return true on success or false on failure.
 */
bool drv_qspi_xip_set_pe_state(uint8_t enable)
{
    hal_qspi_xip_enable_program_erase_ongoing(qspi_context.hal, enable);
    return true;
}

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
                                        uint8_t resume_opcode)
{
    /*
        Because Fermion uses PIO mode, and disable XiP when do PIO operations.
        So don't need this function here.
        Keep it for building pass.
    */

    hal_qspi_xip_set_suspend_delay(qspi_context.hal, suspend_delay);
    hal_qspi_xip_set_resume_opcode(qspi_context.hal, suspend_opcode);
    /*QWLAN_QSPI_R_QSPI_XIP_SUSPEND_PH_CONFIG_SUSPEND_ENABLE_MASK*/
    hal_qspi_xip_enable_suspend(qspi_context.hal, 1);

    hal_qspi_xip_set_resume_opcode(qspi_context.hal, resume_opcode);
    hal_qspi_xip_set_resume_delay(qspi_context.hal, resume_delay);
    hal_qspi_xip_resume_suspend(qspi_context.hal, 1);
    return true;
}

int32_t drv_qspi_xip_config(qspi_xip_flash_region region_id, uint32_t region_size, uint32_t regigon_addr)
{
    static bool xip_init_done = FALSE;

    if (xip_init_done == TRUE) {
        return 0;
    }

    /* only region #3 is supported */
    if (QSPI_XIP_FLASH_REGION_3 != region_id) {
        return -1;
    }

    xip_init_done = TRUE;

    /* Configure INST_PH
     * linear burst opcode: 0x03=normal read 0x0B=fast read 0xEB=Quad I/O read
     * wrap burst opcode 0x0C : burst read
     * only support SDR and single mode
     * dummy is 0 for CMD 0x03
     */
    hal_qspi_xip_ph_config(qspi_context.hal, 0x03, 0);

    /*
     * 4K bytes/block
     */
    hal_qspi_xip_region_config(qspi_context.hal, region_id, region_size, (regigon_addr / BLOCK_SIZE_IN_BYTES));

    hal_qspi_xip_master_config(qspi_context.hal);

    return 0;
}

#endif
