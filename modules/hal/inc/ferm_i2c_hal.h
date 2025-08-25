/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#ifndef _FERM_I2C_HAL_
#define __FERM_I2C_HAL_
#include "qccx.h"

#ifdef I2C_HAL
typedef volatile I2C_BASE_i2c_Type i2c_hal;

static inline void i2c_hal_comp_type(i2c_hal *hal, uint32_t *type)
{
    *type = hal->I2C_I2C_IC_COMP_TYPE.reg;
}

static inline void i2c_hal_max_speed_mode(i2c_hal *hal, uint16_t *speed_mode)
{
    *speed_mode = hal->I2C_I2C_IC_CON.bit.SPEED;
}

static inline void i2c_hal_ss_scl_config(i2c_hal *hal, uint16_t lcnt, uint16_t hcnt)
{
    hal->I2C_I2C_IC_SS_SCL_LCNT.bit.IC_SS_SCL_LCNT = lcnt;
    hal->I2C_I2C_IC_SS_SCL_HCNT.bit.IC_SS_SCL_HCNT = hcnt;
}

static inline void i2c_hal_fs_scl_config(i2c_hal *hal, uint16_t lcnt, uint16_t hcnt)
{
    hal->I2C_I2C_IC_FS_SCL_LCNT.bit.IC_FS_SCL_LCNT = lcnt;
    hal->I2C_I2C_IC_FS_SCL_HCNT.bit.IC_FS_SCL_HCNT = hcnt;
}

static inline void i2c_hal_hs_scl_config(i2c_hal *hal, uint16_t lcnt, uint16_t hcnt)
{
    hal->I2C_I2C_IC_HS_SCL_HCNT.bit.IC_HS_SCL_HCNT = hcnt;
    hal->I2C_I2C_IC_HS_SCL_LCNT.bit.IC_HS_SCL_LCNT = lcnt;
}

static inline void i2c_hal_tl_rl_config(i2c_hal *hal, uint8_t tl, uint8_t rl)
{
    hal->I2C_I2C_IC_TX_TL.bit.TX_TL = tl;
    hal->I2C_I2C_IC_RX_TL.bit.RX_TL = rl;
}

static inline void i2c_hal_tar_write(i2c_hal *hal, uint16_t addr)
{
    hal->I2C_I2C_IC_TAR.bit.IC_TAR = addr;
}

static inline void i2c_hal_tar_read(i2c_hal *hal, uint16_t *addr)
{
    *addr = hal->I2C_I2C_IC_TAR.bit.IC_TAR;
}

static inline void i2c_hal_con_read(i2c_hal *hal, uint32_t *value)
{
    *value = hal->I2C_I2C_IC_CON.reg;
}

static inline void i2c_hal_con_write(i2c_hal *hal, uint32_t value)
{
    hal->I2C_I2C_IC_CON.reg = value;
}

static inline void i2c_hal_enable(i2c_hal *hal)
{
    hal->I2C_I2C_IC_ENABLE.bit.ENABLE = 1;
}

static inline void i2c_hal_disable(i2c_hal *hal)
{
    hal->I2C_I2C_IC_ENABLE.bit.ENABLE = 0;
}

static inline void i2c_hal_slave_disable(i2c_hal *hal, uint8_t disable)
{
    hal->I2C_I2C_IC_CON.bit.IC_SLAVE_DISABLE = disable;
}

static inline void i2c_hal_restart_enable(i2c_hal *hal, uint8_t enable)
{
    hal->I2C_I2C_IC_CON.bit.IC_RESTART_EN = enable;
}

static inline void i2c_hal_master_enable(i2c_hal *hal, uint8_t enable)
{
    hal->I2C_I2C_IC_CON.bit.MASTER_MODE = enable;
}

static inline void i2c_hal_master_10bit_addr_enable(i2c_hal *hal, uint8_t enable)
{
    hal->I2C_I2C_IC_CON.bit.IC_10BITADDR_MASTER = enable;
}

static inline void i2c_hal_slave_10bit_addr_enable(i2c_hal *hal, uint8_t enable)
{
    hal->I2C_I2C_IC_CON.bit.IC_10BITADDR_SLAVE = enable;
}

static inline void i2c_hal_speed_mode(i2c_hal *hal, uint8_t mode)
{
    hal->I2C_I2C_IC_CON.bit.SPEED = mode;
}

static inline void i2c_hal_spklen(i2c_hal *hal, uint8_t *fs, uint8_t *hs)
{
    *fs = hal->I2C_I2C_IC_FS_SPKLEN.bit.IC_FS_SPKLEN;
    *hs = hal->I2C_I2C_IC_HS_SPKLEN.bit.IC_HS_SPKLEN;
}

static inline void i2c_hal_status(i2c_hal *hal, uint32_t *status)
{
    *status = hal->I2C_I2C_IC_STATUS.reg;
}

static inline void i2c_hal_data_cmd_write(i2c_hal *hal, uint32_t value)
{
    hal->I2C_I2C_IC_DATA_CMD.reg = value;
}

static inline uint32_t i2c_hal_data_cmd_read(i2c_hal *hal)
{
    return hal->I2C_I2C_IC_DATA_CMD.reg;
}

static inline int i2c_hal_tx_fifo_not_full(i2c_hal *hal)
{
    return hal->I2C_I2C_IC_STATUS.bit.TFNF;
}

static inline int i2c_tx_fifo_empty(i2c_hal *hal)
{
    return hal->I2C_I2C_IC_STATUS.bit.TFE;
}

static inline int i2c_hal_rx_fifo_not_empty(i2c_hal *hal)
{
    return hal->I2C_I2C_IC_STATUS.bit.RFNE;
}

static inline int i2c_rx_fifo_full(i2c_hal *hal)
{
    return hal->I2C_I2C_IC_STATUS.bit.RFF;
}

static inline void i2c_hal_tx_fifo_level(i2c_hal *hal, uint32_t *level)
{
    *level = hal->I2C_I2C_IC_TXFLR.bit.TXFLR;
}

static inline void i2c_hal_rx_fifo_level(i2c_hal *hal, uint32_t *level)
{
    *level = hal->I2C_I2C_IC_RXFLR.bit.RXFLR;
}

static inline void i2c_hal_tx_rx_fifo_depth(i2c_hal *hal, uint8_t *tx, uint8_t *rx)
{
    *tx = hal->I2C_I2C_IC_COMP_PARAM_1.bit.TX_BUFFER_DEPTH;
    *rx = hal->I2C_I2C_IC_COMP_PARAM_1.bit.RX_BUFFER_DEPTH;
}

static inline void i2c_hal_intr_stat(i2c_hal *hal, uint32_t *value)
{
    *value = hal->I2C_I2C_IC_INTR_STAT.reg;
}

static inline void i2c_hal_intr_mask(i2c_hal *hal, uint32_t mask)
{
    hal->I2C_I2C_IC_INTR_MASK.reg = mask;
}

static inline void i2c_hal_intr_tx_empty_set(i2c_hal *hal)
{
    hal->I2C_I2C_IC_INTR_MASK.bit.M_TX_EMPTY = 1;
}

static inline void i2c_hal_intr_tx_empty_clear(i2c_hal *hal)
{
    hal->I2C_I2C_IC_INTR_MASK.bit.M_TX_EMPTY = 0;
}

static inline void i2c_hal_intr_clear_all(i2c_hal *hal, uint32_t *value)
{
    *value = hal->I2C_I2C_IC_CLR_INTR.reg;
}

static inline uint32_t i2c_hal_intr_clear_stop_det(i2c_hal *hal)
{
    return hal->I2C_I2C_IC_CLR_STOP_DET.reg;
}

static inline int i2c_hal_tx_abort_intr(i2c_hal *hal)
{
    return hal->I2C_I2C_IC_INTR_STAT.bit.R_TX_ABRT;
}

static inline void i2c_hal_abort_tx(i2c_hal *hal)
{
    hal->I2C_I2C_IC_ENABLE.bit.ABORT = 1;
}
#endif  // I2C_HAL
#endif  //__FERM_I2C_HAL_
