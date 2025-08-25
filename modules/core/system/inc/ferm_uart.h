
/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef CORE_SYSTEM_INC_FERM_UART_H_
#define CORE_SYSTEM_INC_FERM_UART_H_
#include <stdint.h>
#include "nt_common.h"
#include "nt_osal.h"
#include "ferm_uart_hal.h"

typedef enum {
    UART_STOP_BITS_1,
    UART_STOP_BITS_1_5_OR_2,
} uart_stop_bits;

typedef enum {
    UART_DATA_BITS_5,
    UART_DATA_BITS_6,
    UART_DATA_BITS_7,
    UART_DATA_BITS_8,
} uart_data_bits;

typedef enum {
    UART_PARITY_NONE,
    UART_PARITY_ODD,
    UART_PARITY_EVEN,
} uart_parity;

typedef struct {
    uint32_t baudrate;
    uart_parity parity;
    uart_data_bits data_bits;
    uart_stop_bits stop_bits;
    uint32_t loopback;
} uart_config;

typedef struct {
    uint8_t *ring;
    uint32_t size;
    uint32_t wr_idx;
    uint32_t rd_idx;
    uint32_t timeout;
    uint32_t thres;
    uint32_t full_cnt;
    nt_osal_semaphore_handle_t sync_sem;
} uart_xfr;

typedef enum {
    UART_INSTANCE_0,
    UART_INSTANCE_MAX,
} uart_instance;

typedef struct {
    uint8_t line_status;
    uint8_t rx_avail;
    uint8_t char_timeout;
    uint8_t tx_empty;
    uint8_t modem_status;
    uint8_t busy_detect;
    uint8_t no_pending;
} uart_irqs;

typedef struct {
    uint32_t rx_irq_num;
    uint32_t tx_irq_num;
} uart_stats;

typedef struct {
    uart_hal *hal;
    uart_config config;
    uint32_t state;
    uart_xfr tx;
    uart_xfr rx;
    uart_irqs irqs;
    uart_stats stats;
} uart_dev;

typedef struct {
    uint32_t rate;
    uint32_t divisor;
} uart_baudrate;

typedef enum {
    UART_TX_DIR,
    UART_RX_DIR,
} uart_dir;

typedef enum {
    UART_SUCCESS,
    UART_ERROR,
    UART_ERROR_NULL_PTR,
    UART_ERROR_INVALID_PARAM,
    UART_ERROR_CFG_PARAM,
    UART_ERROR_BAUDRATE_CFG,
    UART_ERROR_SEND_BUSY,
    UART_ERROR_RECV_BUSY,
    UART_ERROR_TX_ENQUEUE_SEM_SYNC,
    UART_ERROR_TX_ENQUEUE_FULL,
    UART_ERROR_TX_DEQUEUE_EMPTY,
    UART_ERROR_RX_ENQUEUE_FULL,
    UART_ERROR_RX_DEQUEUE_SEM_SYNC,
    UART_ERROR_RX_DEQUEUE_EMPTY,
    UART_ERROR_TRANSFER_TIMEOUT,
    UART_ERROR_INPUT_FIFO_UNDER_RUN,
    UART_ERROR_INPUT_FIFO_OVER_RUN,
    UART_ERROR_OUTPUT_FIFO_UNDER_RUN,
    UART_ERROR_OUTPUT_FIFO_OVER_RUN,
    UART_ERROR_TRANSFER_FORCE_TERMINATED,
    UART_ERROR_BUS_CLK_CFG_FAIL,
    UART_ERROR_BUS_GPIO_ENABLE_FAIL,
    UART_ERROR_CANCEL_TRANSFER_FAIL,
    UART_ERROR_BOOTSTRAP_CFG_FAIL,
    UART_ERROR_DEVICE_STATE,
} uart_status;

#define DIVISOR_DLL(divisor) (divisor & 0xff)
#define DIVISOR_DLH(divisor) ((divisor >> 8) & 0xff)
#define BAUDRATE_NUM_MAX     9

#define RX_FIFO_TRIG_1_CHAR       0
#define RX_FIFO_TRIG_1_4_FULL     1
#define RX_FIFO_TRIG_1_2_FULL     2
#define RX_FIFO_TRIG_LESS_2_CHARS 3

#define TX_FIFO_TRIG_EMPTY    0
#define TX_FIFO_TRIG_2_CHARS  1
#define TX_FIFO_TRIG_1_4_FULL 2
#define TX_FIFO_TRIG_1_2_FULL 3

#define UART_HW_FIFO_SIZE 16

#define UART_TX_BUFF_SIZE 32
#define UART_RX_BUFF_SIZE 32

#define BIT(n) (1 << (n))
// UARET instance state
#define UART_UNINIT 0
#define UART_INIT   BIT(0)
#define UART_SETUP  BIT(1)
#define UART_READY  (UART_INIT | UART_SETUP)
#define UART_SEND   BIT(2)
#define UART_RECV   BIT(3)
#define UART_ABORT  BIT(4)

#define FERM_BOOT_STRAP_VALUE 0x63887466

#define NT_NVIC_ISER0   0xE000E100  // Irq 0 to 31 Set Enable Register
#define NT_NVIC_ICER0   0xE000E180  // Irq 0 to 31 Clear Enable Register
#define ENABLE_UART_IRQ BIT(3)

#define IIR_IID_MASK        0xF
#define IIR_MODEM_STATUS    0x0
#define IIR_NO_INTR_PENDING 0x1
#define IIR_THR_EMPTY       0x2
#define IIR_RX_DATA_AVAIL   0x4
#define IIR_LINE_STATUS     0x6
#define IIR_BUSY_DETECT     0x7
#define IIR_CHAR_TIMEOUT    0xC

#define UART_DUMP_CONF BIT(0)
#define UART_DUMP_REG  BIT(1)

uart_status uart_close(uart_instance instance);
uart_status uart_open(uart_instance instance, uart_config *config);
uart_status uart_transmit(uart_instance instance, uint8_t *buf, uint32_t size, uint32_t *sent);
uart_status uart_receive(uart_instance instance, uint8_t *buf, uint32_t size, uint32_t *recv);
uart_status uart_open_with_rx_timeout(uart_instance instance, uart_config *config, uint32_t timeout);

#endif
