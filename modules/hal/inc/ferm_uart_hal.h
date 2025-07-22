/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#ifndef _FERM_UART_HAL_
#define __FERM_UART_HAL_
#include "qccx.h"

/* ======= UART_RBR_THR_DLL_REG ======== */
#define UART_RBR_THR_DLL_REG_RBR_POS 0
#define UART_RBR_THR_DLL_REG_RBR_MSK 0xFF
#define UART_RBR_THR_DLL_REG_THR_POS 0
#define UART_RBR_THR_DLL_REG_THR_MSK 0xFF
#define UART_RBR_THR_DLL_REG_DLL_POS 0
#define UART_RBR_THR_DLL_REG_DLL_MSK 0xFF

/* ========= UART_IER_DLH_REG ========== */
#define UART_IER_DLH_REG_PTIME_POS  7
#define UART_IER_DLH_REG_PTIME_MSK  0x80
#define UART_IER_DLH_REG_DLH_POS    0
#define UART_IER_DLH_REG_DLH_MSK    0x7F
#define UART_IER_DLH_REG_ELCOLR_POS 4
#define UART_IER_DLH_REG_ELCOLR_MSK 0x10
#define UART_IER_DLH_REG_EDSSI_POS  3
#define UART_IER_DLH_REG_EDSSI_MSK  0x8
#define UART_IER_DLH_REG_ELSI_POS   2
#define UART_IER_DLH_REG_ELSI_MSK   0x4
#define UART_IER_DLH_REG_ETBEI_POS  1
#define UART_IER_DLH_REG_ETBEI_MSK  0x2
#define UART_IER_DLH_REG_ERBFI_POS  0
#define UART_IER_DLH_REG_ERBFI_MSK  0x1

/* ========== UART_IIR_FCR_REG ========== */
#define UART_IIR_FCR_REG_FIFOE_POS 0
#define UART_IIR_FCR_REG_FIFOE_MSK 0x1
#define UART_IIR_FCR_REG_TET_POS   4
#define UART_IIR_FCR_REG_TET_MSK   0x30
#define UART_IIR_FCR_REG_RT_POS    6
#define UART_IIR_FCR_REG_RT_MSK    0xC0

/* =========== UART_MCR_REG =========== */
#define UART_MCR_REG_LB_POS 4
#define UART_MCR_REG_LB_MSK 0x10

/* =========== UART_USR_REG ============ */
#define UART_USR_REG_BUSY_MSK 0x1
#define UART_USR_REG_BUSY_POS 0
#define UART_USR_REG_TFNF_MSK 0x2
#define UART_USR_REG_TFNF_POS 1
#define UART_USR_REG_TFE_MSK  0x4
#define UART_USR_REG_TFE_POS  2
#define UART_USR_REG_RFNE_MSK 0x8
#define UART_USR_REG_RFNE_POS 3
#define UART_USR_REG_RFF_MSK  0x10
#define UART_USR_REG_RFF_POS  4
/* =========== UART_USR_REG ============ */
#define UART_LSR_REG_THRE_MSK 0x20
#define UART_LSR_REG_THRE_POS 5
#define UART_LSR_REG_DR_MSK   0x1
#define UART_LSR_REG_DR_POS   0

typedef volatile UART_BASE_uart_Type uart_hal;

static inline void uart_hal_enable_intr_tx(uart_hal *hal)
{
    hal->UART_UART_DLH.reg |= UART_IER_DLH_REG_PTIME_MSK | UART_IER_DLH_REG_ETBEI_MSK;
}

static inline void uart_hal_disable_intr_tx(uart_hal *hal)
{
    hal->UART_UART_DLH.reg &= ~(UART_IER_DLH_REG_PTIME_MSK | UART_IER_DLH_REG_ETBEI_MSK);
}

static inline void uart_hal_enable_intr_rx(uart_hal *hal)
{
    hal->UART_UART_DLH.reg |= UART_IER_DLH_REG_ERBFI_MSK;
}

static inline void uart_hal_disable_intr_rx(uart_hal *hal)
{
    hal->UART_UART_DLH.reg &= ~UART_IER_DLH_REG_ERBFI_MSK;
}

static inline uint32_t uart_hal_intr_tx_busy(uart_hal *hal)
{
    return (hal->UART_UART_USR.reg & UART_USR_REG_BUSY_MSK);
}

static inline uint32_t uart_hal_intr_tx_ready(uart_hal *hal)
{
    /*  The USR STAT not implemented by hardware, so that we can only rely on LSR */
    // return (hal->UART_UART_USR.reg & UART_USR_REG_TFNF_MSK);
    return (hal->UART_UART_LSR.reg & UART_LSR_REG_THRE_MSK);
}

static inline uint32_t uart_hal_intr_rx_ready(uart_hal *hal)
{
    // return (hal->UART_UART_USR.reg & UART_USR_REG_RFNE_MSK);
    return (hal->UART_UART_LSR.reg & UART_LSR_REG_DR_MSK);
}

static inline uint32_t uart_hal_intr_tx_done(uart_hal *hal)
{
    return (hal->UART_UART_USR.reg & UART_USR_REG_TFE_MSK);
}

static inline uint32_t uart_hal_intr_rx_full(uart_hal *hal)
{
    return (hal->UART_UART_USR.reg & UART_USR_REG_RFF_MSK);
}

static inline uint32_t uart_hal_intr_get(uart_hal *hal)
{
    return (hal->UART_UART_IIR.reg);
}

static inline uint32_t uart_hal_line_status_get(uart_hal *hal)
{
    return hal->UART_UART_LSR.reg;
}

static inline void uart_hal_tx_write(uart_hal *hal, uint8_t data)
{
    hal->UART_UART_RBR.reg = data;
    return;
}

static inline uint8_t uart_hal_rx_read(uart_hal *hal)
{
    return hal->UART_UART_RBR.reg;
}

static inline void uart_hal_loopback_enable(uart_hal *hal, uint32_t enable)
{
    hal->UART_UART_MCR.bit.LOOPBACK = enable;
    return;
}

static inline void uart_hal_divisor_access(uart_hal *hal, uint32_t enable)
{
    hal->UART_UART_LCR.bit.DLAB = enable;
    return;
}

static inline void uart_hal_divisor_low_cfg(uart_hal *hal, uint32_t dll)
{
    hal->UART_UART_RBR.reg = ((dll << UART_RBR_THR_DLL_REG_RBR_POS) & UART_RBR_THR_DLL_REG_DLL_MSK);
    return;
}

static inline void uart_hal_divisor_high_cfg(uart_hal *hal, uint32_t dlh)
{
    hal->UART_UART_DLH.reg = ((dlh << UART_IER_DLH_REG_DLH_POS) & UART_IER_DLH_REG_DLH_MSK);
    return;
}

static inline void uart_hal_data_bits_cfg(uart_hal *hal, uint32_t bits)
{
    hal->UART_UART_LCR.bit.DLS = bits;

    return;
}

static inline void uart_hal_stop_bits_cfg(uart_hal *hal, uint32_t bits)
{
    hal->UART_UART_LCR.bit.STOP = bits;

    return;
}

static inline void uart_hal_parity_enable(uart_hal *hal, uint32_t enable)
{
    hal->UART_UART_LCR.bit.PEN = enable;

    return;
}

static inline void uart_hal_event_parity_select(uart_hal *hal, uint32_t event)
{
    hal->UART_UART_LCR.bit.EPS = event;

    return;
}

static inline void uart_hal_fifo_enable(uart_hal *hal, uint32_t enable)
{
    hal->UART_UART_IIR.reg |= ((enable << UART_IIR_FCR_REG_FIFOE_POS) & UART_IIR_FCR_REG_FIFOE_MSK);
    return;
}

static inline void uart_hal_fifo_tet_cfg(uart_hal *hal, uint32_t tet)
{
    hal->UART_UART_IIR.reg |= ((tet << UART_IIR_FCR_REG_TET_POS) & UART_IIR_FCR_REG_TET_MSK);
    return;
}

static inline void uart_hal_fifo_rt_cfg(uart_hal *hal, uint32_t rt)
{
    hal->UART_UART_IIR.reg |= ((rt << UART_IIR_FCR_REG_RT_POS) & UART_IIR_FCR_REG_RT_MSK);
    return;
}

#endif
