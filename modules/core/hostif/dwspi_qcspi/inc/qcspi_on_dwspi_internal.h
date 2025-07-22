/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*========================================================================
 * @file qcspi_on_dwspi.h
 * @brief Macros, Enums related to SPI slave SW
 *========================================================================*/
#ifndef _QCSPI_ON_DWSPI_H_
#define _QCSPI_ON_DWSPI_H_

/*-------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * ----------------------------------------------------------------------*/
/* The following register mappings, offsets and
   masks are not defined under NEUT_2.
   These registers, masks, offsets are needed
   for Neutrino SPI Slave interface.

   So as a temporary work around, I have added
   these in the header file.

   Need to be removed at a later point
 */

#ifndef QWLAN_SPI_TXOICR_REG
#define QWLAN_SPI_TXOICR_REG (QWLAN_SPI_BASE + 0x38)
#endif

#ifndef QWLAN_SPI_ISR_TXOIS_MASK
#define QWLAN_SPI_ISR_TXOIS_MASK 0x2
#endif

#ifndef QWLAN_SPI_IMR_TXEIM_OFFSET
#define QWLAN_SPI_IMR_TXEIM_OFFSET 0x0
#endif

#ifndef QWLAN_SPI_ISR_TXEIS_MASK
#define QWLAN_SPI_ISR_TXEIS_MASK 0x1
#endif

#ifndef QWLAN_SPI_TXFLR_REG
#define QWLAN_SPI_TXFLR_REG (QWLAN_SPI_BASE + 0x20)
#endif

#ifndef QWLAN_SPI_RXFLR_REG
#define QWLAN_SPI_RXFLR_REG (QWLAN_SPI_BASE + 0x24)
#endif

/* Macros */

#define START_DATA     0xA5
#define READ_RSP_CMDID 0xC7

#define MOD_SUB(X, Y, MOD) ((X) < (Y) ? (MOD + X - Y) : (X - Y))

#define RING_EMPTY() (g_spi_shared.read_offset == g_spi_shared.write_offset)
#define RING_LEVEL() (MOD_SUB(g_spi_shared.write_offset, g_spi_shared.read_offset, IR_MAX_SIZE))

#define read_from_rx_fifo()     (NT_REG_RD(QWLAN_SPI_DR0_REG))
#define write_to_tx_fifo(value) (NT_REG_WR(QWLAN_SPI_DR0_REG, (value)))
#define write_header_to_tx_fifo() \
    write_to_tx_fifo(START_DATA); \
    write_to_tx_fifo(READ_RSP_CMDID);

#define set_SR_field(field_mask)   (g_status_register |= field_mask)
#define clear_SR_field(field_mask) (g_status_register &= ~field_mask)

#define SR_CRP_MASK     0x00010000
#define SR_DMAACT_MASK  0x00020000
#define SR_WIP_MASK     0x00040000
#define SR_CMD_ERR_MASK 0x00800000
#define SR_NRDY_MASK    0x01000000
#define SR_RXOERR_MASK  0x08000000
#define SR_TXUERR_MASK  0x10000000

#define FIFO_DEPTH            0xFF
#define MAX_TXF_REFILL_IN_ISR 32

#define TX_FIFO_LEVEL (NT_REG_RD(QWLAN_SPI_TXFLR_REG))
#define RX_FIFO_LEVEL (NT_REG_RD(QWLAN_SPI_RXFLR_REG))

#define IR_THRESH_DIFF 600

#define C_RDSR  0x05
#define C_FREAD 0x0B
#define C_WRITE 0x02
#define C_NOP   0x00
#define C_IRR   0x81
#define C_IRW   0x82
#define C_MIR   0x83

#define CMD_TASK_PRIORITY 7
#define CMD_FRAME_SIZE    7

#define IRW_DATA_SIZE 4

#define SR_SEND_SIZE          6
#define READ_START_FRAME_SIZE 2

#define MAX_READ_ISR 16

#define CMD_LEN_BYTES 3

#define IS_TASK_SUSPENDED(x)                         \
    do {                                             \
        value = NT_REG_RD(QWLAN_SPI_IMR_REG);        \
        value &= ~(1 << QWLAN_SPI_IMR_RXFIM_OFFSET); \
        NT_REG_WR(QWLAN_SPI_IMR_REG, value);         \
                                                     \
        g_spi_shared.is_task_suspended = (x);        \
                                                     \
        if (false == is_task_busy)                   \
            clear_SR_field(SR_NRDY_MASK);            \
                                                     \
        value = NT_REG_RD(QWLAN_SPI_IMR_REG);        \
        value |= (1 << QWLAN_SPI_IMR_RXFIM_OFFSET);  \
        NT_REG_WR(QWLAN_SPI_IMR_REG, value);         \
    } while (0)

/* END --- Macros */

enum {
    SM_INIT = 0x10,
    SM_CMD,
    SM_LEN1,
    SM_LEN2,
    SM_ADDR1,
    SM_ADDR2,
    SM_ADDR3,
    SM_ADDR4,
    SM_DATA_WRITE,
    SM_IRW,
    SM_CHECK_NEXT_CMD,
    SM_READ_ISR,
    SM_IRW_TRACK_ISR,
    SM_WRITE_TRACK_ISR
};

#endif /* #ifndef _QCSPI_ON_DWSPI_H_ */
