/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef CORE_SYSTEM_INC_FERM_I2C_H_
#define CORE_SYSTEM_INC_FERM_I2C_H_
#include <stdint.h>
#include "nt_common.h"
#include "nt_osal.h"
#include "ferm_i2c_hal.h"

typedef struct {
    uint8_t *buf;
    uint32_t len;
    uint32_t flag;
} i2c_msg;

typedef struct {
    uint8_t *buf;
    uint32_t len;
    uint32_t request_bytes;
    uint32_t rx_pending;
    uint32_t flag;
    uint32_t last_dir;
} i2c_xfr;

typedef struct {
    uint8_t fs_spklen;
    uint8_t hs_spklen;
    uint8_t rx_tl;
    uint8_t tx_tl;
    uint8_t rx_depth;
    uint8_t tx_depth;
    uint16_t support_mode;
    uint32_t clk_khz;
} i2c_cap;

typedef enum {
    I2C_STD_MODE = 1,
    I2C_FAST_MODE = 2,
    I2C_HIGH_MODE = 3,
} i2c_mode;

typedef struct {
    uint16_t hcnt;
    uint16_t lcnt;
    uint16_t tar_addr;
    i2c_mode mode;
    uint32_t freq;
} i2c_config;

typedef enum {
    I2C_STD_SPEED,
    I2C_FAST_SPEED,
    I2C_FAST_SPEED_PLUS,
    I2C_HIGH_SPEED_MODE,
    I2C_INVALID_SPEED,
    I2C_SPEED_NUM = I2C_INVALID_SPEED,
} i2c_speed;

typedef enum {
    I2C_INSTANCE_0,
    I2C_INSTANCE_MAX,
} i2c_instance;

typedef struct {
    uint32_t irq_err;
    uint32_t irq_cnt;
} i2c_stats;

typedef struct {
    i2c_hal *hal;
    i2c_cap cap;
    i2c_config config;
    i2c_xfr xfr;
    uint32_t state;
    i2c_stats stats;
    nt_osal_semaphore_handle_t sync_sem;
} i2c_dev;

typedef struct {
    uint32_t freq;
    i2c_mode mode;
    uint32_t low_time;   // high period of SCL period, in ns unit
    uint32_t high_time;  // Low period of SCL period, in ns uint
} i2c_scl_ht_lt;

typedef enum {
    I2C_SUCCESS,
    I2C_ERROR,
    I2C_ERROR_INVALID_PARAM,
    I2C_EEROR_MEM_ALLOC,
    I2C_EEROR_TRANSFER_BUSY,
    I2C_EEROR_TRANSFER_TIMEOUT,
    I2C_ERROR_INPUT_FIFO_UNDER_RUN,
    I2C_ERROR_INPUT_FIFO_OVER_RUN,
    I2C_ERROR_OUTPUT_FIFO_UNDER_RUN,
    I2C_ERROR_OUTPUT_FIFO_OVER_RUN,
    I2C_ERROR_COMMAND_OVER_RUN,
    I2C_ERROR_TRANSFER_FORCE_TERMINATED,
    I2C_ERROR_COMMAND_ILLEGAL,
    I2C_ERROR_COMMAND_FAIL,
    I2C_ERROR_BUS_CLK_CFG_FAIL,
    I2C_ERROR_BUS_GPIO_ENABLE_FAIL,
    I2C_ERROR_DMA_TX_BUS_ERROR,
    I2C_ERROR_DMA_RX_BUS_ERROR,
    I2C_DMA_TX_REST_DONE,
    I2C_DMA_RX_REST_DONE,
    I2C_CANCEL_TRANSFER_COMPLETED,
    I2C_CANCEL_TRANSFER_INVALID,

    // Above status are keep align with the previous project, some may not be used in the Fermion
    I2C_ERROR_CANCEL_TRANSFER_FAIL,
    I2C_ERROR_BOOTSTRAP_CFG_FAIL,
    I2C_ERROR_DEVICE_STATE,
    I2C_ERROR_INIT_XFR,
    I2C_ERROR_IC_COMP,
    I2C_ERROR_TX_ABORT_INTR,

    // Below status only using in i2c driver, which wouldn't be exposed to QAPI
    I2C_RX_FIFO_EXPECT_FULL,
} i2c_status;

#define BIT(n)      (1 << (n))
#define BIT_MASK(n) (BIT(n) - 1UL)

#define I2C_MSG_RESTART BIT(0)
#define I2C_MSG_STOP    BIT(1)
#define I2C_MSG_WRITE   BIT(2)
#define I2C_MSG_READ    BIT(3)
#define I2C_MSG_RW_MASK (I2C_MSG_WRITE | I2C_MSG_READ)

#define I2C_ADDRESS_10BIT_MODE BIT(12)
#define I2C_ADDRESS_MASK       BIT_MASK(10)

// I2C instance state
#define I2C_INIT  BIT(0)
#define I2C_SETUP BIT(1)
#define I2C_READY (I2C_INIT | I2C_SETUP)
#define I2C_BUSY  BIT(2)
#define I2C_SEND  BIT(3)
#define I2C_RECV  BIT(4)
#define I2C_ABORT BIT(5)

#define I2C_STD_SPEED_DEFAULT       100
#define I2C_FAST_SPEED_DEFAULT      400
#define I2C_FAST_PLUS_SPEED_DEFAULT 1000
#define I2C_HIGH_SPEED_DEFAULT      2000

#define I2C_DATA_CMD_DAT_MASK 0xFF
#define I2C_DATA_CMD_READ     BIT(8)
#define I2C_DATA_CMD_WRITE    0
#define I2C_DATA_CMD_STOP     BIT(9)
#define I2C_DATA_CMD_RESTART  BIT(10)

#define I2C_INTR_RX_UN    BIT(0)
#define I2C_INTR_RX_OV    BIT(1)
#define I2C_INTR_RX_FULL  BIT(2)
#define I2C_INTR_TX_OV    BIT(3)
#define I2C_INTR_TX_EMPTY BIT(4)
#define I2C_INTR_TX_ABRT  BIT(6)
#define I2C_INTR_STOP_DET BIT(9)

#define I2C_INTR_ERROR (I2C_INTR_TX_ABRT | I2C_INTR_TX_OV | I2C_INTR_RX_OV | I2C_INTR_RX_UN)

#define I2C_INTR_TX (I2C_INTR_TX_OV | I2C_INTR_TX_EMPTY | I2C_INTR_TX_ABRT | I2C_INTR_STOP_DET)

#define I2C_INTR_RX (I2C_INTR_RX_UN | I2C_INTR_RX_OV | I2C_INTR_RX_FULL | I2C_INTR_STOP_DET)

#define MIN(x, y) ((x) < (y) ? (x) : (y))

#define SCL_FALL_TIME 70   // SCL bus fall time in ns unit
#define SCL_RISE_TIME 100  // SCL bus rise time in ns unit

#define HCNT_CAL(period, freq, spklen) (((period - SCL_FALL_TIME) * freq + 500000) / 1000000 - spklen - 7)

#define LCNT_CAL(period, freq) (((period + SCL_FALL_TIME - SCL_RISE_TIME) * freq + 500000) / 1000000 - 1)

#define FERM_BOOT_STRAP_VALUE 0x63887466

#define NT_NVIC_ISER0  0xE000E100  // Irq 0 to 31 Set Enable Register
#define NT_NVIC_ICER0  0xE000E180  // Irq 0 to 31 Clear Enable Register
#define ENABLE_I2C_IRQ BIT(2)

#define I2C_WAIT_DELAY     (100UL / portTICK_RATE_MS)
#define I2C_TRANS_INTERVAL 5000  // 5ms for transaction interval

#define I2C_DUMP_CAP  BIT(0)
#define I2C_DUMP_CONF BIT(1)
#define I2C_DUMP_REG  BIT(2)
#define I2C_DUMP_XFR  BIT(3)
#define I2C_DUMP_ERR  BIT(4)

i2c_status i2c_open(i2c_instance instance);
i2c_status i2c_close(i2c_instance instance);
i2c_status i2c_transfer(i2c_instance instance, i2c_msg *msgs, uint8_t num_msgs, uint16_t tar_addr, uint32_t freq);
i2c_status i2c_cancel(i2c_instance instance);
i2c_status i2c_cancel_transfer(i2c_instance instance);
#endif
