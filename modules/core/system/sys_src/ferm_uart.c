/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <string.h>
#include <stdio.h>
#include "fwconfig_cmn.h"
#include "nt_flags.h"
#include "ferm_uart.h"
#include "nt_hw.h"
#include "timer.h"
#include "nt_common.h"
#include "ferm_prof.h"

#include "autoconf.h"
#include "qccx.h"

//#define UART_DRV
//#define UART_DRV_DBG

#ifdef UART_DRV
#define UART_DRV_DBG
#define UART_SEM_SYNC
//#define	UART_FIFO_MODE
#ifdef UART_DRV_DBG
#include "uart.h"
static char uart_buf[60];
#define UART_PRINTF(...)                               \
    snprintf(uart_buf, sizeof(uart_buf), __VA_ARGS__); \
    nt_dbg_print(uart_buf);
static void uart_dump(uart_dev *dev, uint8_t dump);
#define UART_DUMP(dev, dump) uart_dump(dev, dump);
#else
#define UART_PRINTF(...)
#define UART_DUMP(dev, config)
#endif  // UART_DRIVER_DEBUG

// Baudrate = (serial_clk_freq) / (16 * Divisor)
/* Hardware issue limited the peformace of UART, sugget to apply the baudrate under 115200 */
static const uart_baudrate baudrate_table[BAUDRATE_NUM_MAX] = {
    {921600, 0x04}, {460800, 0x08}, {230400, 0x10}, {115200, 0x20}, {57600, 0x40},
    {38400, 0x60},  {19200, 0xc0},  {9600, 0x180},  {4800, 0x300},
};

uint8_t tx_buf[UART_TX_BUFF_SIZE];
uint8_t rx_buf[UART_RX_BUFF_SIZE];

uart_dev fm_uart_dev[UART_INSTANCE_MAX];

/* Get UART instance device structure */
static inline uart_dev *uart_get_dev(uart_instance instance)
{
    if (instance >= UART_INSTANCE_MAX)
        return NULL;
    else
        return &fm_uart_dev[instance];
}

#ifdef UART_DRV_DBG
static void uart_dump(uart_dev *dev, uint8_t dump)
{
    uart_config *config;
    uart_hal *hal;
    uint32_t value;

    if (dev == NULL)
        return;

    if (dump & UART_DUMP_CONF) {
        config = &dev->config;
        UART_PRINTF("UART device config dump\r\n");
        UART_PRINTF("	baudrate:%u\r\n", (unsigned int)(config->baudrate));
        UART_PRINTF("	parity:%u\r\n", (unsigned int)(config->parity));
        UART_PRINTF("	data_bits:%u\r\n", (unsigned int)(config->data_bits));
        UART_PRINTF("	stop_bits:%u\r\n", (unsigned int)(config->stop_bits));
        UART_PRINTF("	loopback:%u\r\n", (unsigned int)(config->loopback));
    }

    if (dump & UART_DUMP_REG) {
#if CONFIG_SOC_QCC730V1
        hal = (uart_hal *)(QCC730V1_UART_BASE_BASE);
#elif CONFIG_SOC_QCC730V2
        hal = (uart_hal *)(QCC730V2_UART_BASE_BASE);
#endif
        UART_PRINTF("UART device regs dump\r\n");
        value = hal->UART_UART_DLH.reg;
        UART_PRINTF("UART DLH/IER(0x1233804):0x%x\r\n", (unsigned int)(value));
        value = hal->UART_UART_IIR.reg;
        UART_PRINTF("UART FCR/IIR(0x1233808):0x%x\r\n", (unsigned int)(value));
        value = hal->UART_UART_LCR.reg;
        UART_PRINTF("UART LCR(0x123380C):0x%x\r\n", (unsigned int)(value));
        value = hal->UART_UART_MCR.reg;
        UART_PRINTF("UART MCR(0x1233810):0x%x\r\n", (unsigned int)(value));
        value = hal->UART_UART_LSR.reg;
        UART_PRINTF("UART LSR(0x1233814):0x%x\r\n", (unsigned int)(value));
        value = hal->UART_UART_MSR.reg;
        UART_PRINTF("UART MSR(0x1233818):0x%x\r\n", (unsigned int)(value));
        value = hal->UART_UART_USR.reg;
        UART_PRINTF("UART USR(0x123387C):0x%x\r\n", (unsigned int)(value));
    }

    return;
}
#endif

/* Setting UART platform related configuration:
 * 1) UART clock configuration
 * 2) UART bootstrap configuration
 * 3) UART interruption enable/disable in SOC level
 * */
static uart_status uart_platform(uint8_t enable)
{
#if CONFIG_SOC_QCC730V1
    PMU_BASE_pmu_Type *pmu = (PMU_BASE_pmu_Type *)(QCC730V1_PMU_BASE_BASE);
#elif CONFIG_SOC_QCC730V2
    PMU_BASE_pmu_Type *pmu = (PMU_BASE_pmu_Type *)(QCC730V2_PMU_BASE_BASE);
#endif

    if (enable) {
        pmu->PMU_ROOT_CLK_ENABLE.bit.UART_ROOT_CLK_ENABLE = enable;
        pmu->PMU_BOOT_STRAP_CONFIG_SECURE.reg = FERM_BOOT_STRAP_VALUE;
        pmu->PMU_BOOT_STRAP_CONFIGURATION_STATUS.bit.CFG_UART_ENABLE = enable;

#if CONFIG_SOC_QCC730V2
        pmu->PMU_BOOT_STRAP_CONFIG_SECURE.reg = FERM_BOOT_STRAP_VALUE;
        pmu->PMU_BOOT_STRAP_CONFIGURATION_STATUS.bit.CFG_UART_OPTION = CONFIG_BOARD_QCC730_UART_GPIO_OPTION;
#endif

        NT_REG_WR(NT_NVIC_ISER0, ENABLE_UART_IRQ);
    } else {
        NT_REG_WR(NT_NVIC_ICER0, ENABLE_UART_IRQ);

        pmu->PMU_BOOT_STRAP_CONFIG_SECURE.reg = FERM_BOOT_STRAP_VALUE;
        pmu->PMU_BOOT_STRAP_CONFIGURATION_STATUS.bit.CFG_UART_ENABLE = enable;
        pmu->PMU_ROOT_CLK_ENABLE.bit.UART_ROOT_CLK_ENABLE = enable;
    }

    if (pmu->PMU_BOOT_STRAP_CONFIGURATION_STATUS.bit.CFG_UART_ENABLE != enable)
        return UART_ERROR_BOOTSTRAP_CFG_FAIL;

    if (pmu->PMU_ROOT_CLK_ENABLE.bit.UART_ROOT_CLK_ENABLE != enable)
        return UART_ERROR_BUS_CLK_CFG_FAIL;

    return UART_SUCCESS;
}

static uart_status uart_config_param_check(uart_config *cfg)
{
    if (cfg == NULL) {
        UART_PRINTF("uart_config_param_check NULL pointer\n");
        return UART_ERROR_NULL_PTR;
    }

    if ((cfg->parity != UART_PARITY_NONE) && (cfg->parity != UART_PARITY_ODD) && (cfg->parity != UART_PARITY_EVEN)) {
        UART_PRINTF("uart_config_param_check invalid parity\n");
        return UART_ERROR_CFG_PARAM;
    }

    if ((cfg->stop_bits != UART_STOP_BITS_1) && (cfg->stop_bits != UART_STOP_BITS_1_5_OR_2)) {
        UART_PRINTF("uart_config_param_check invalid stop bits\n");
        return UART_ERROR_CFG_PARAM;
    }

    if ((cfg->data_bits != UART_DATA_BITS_5) && (cfg->data_bits != UART_DATA_BITS_6) &&
        (cfg->data_bits != UART_DATA_BITS_7) && (cfg->data_bits != UART_DATA_BITS_8)) {
        UART_PRINTF("uart_config_param_check invalid data bits\n");
        return UART_ERROR_CFG_PARAM;
    }

    return UART_SUCCESS;
}

static uart_status uart_baudrate_divisor_get(uint32_t rate, uint32_t *divisor)
{
    uint32_t i = 0;

    if (divisor == NULL) {
        UART_PRINTF("uart_baudrate_divisor_get NULL pointer\n");
        return UART_ERROR_NULL_PTR;
    }

    while (i < (sizeof(baudrate_table) / sizeof(uart_baudrate))) {
        if (baudrate_table[i].rate == rate) {
            *divisor = baudrate_table[i].divisor;
            return UART_SUCCESS;
        }

        i++;
    }

    return UART_ERROR_BAUDRATE_CFG;
}

static uart_status uart_baudrate_divisor_set(uart_hal *hal, uint32_t divisor)
{
    if (hal == NULL) {
        UART_PRINTF("uart_baudrate_divisor_set NULL pointer\n");
        return UART_ERROR_NULL_PTR;
    }

    uart_hal_divisor_access(hal, 1);
    uart_hal_divisor_low_cfg(hal, DIVISOR_DLL(divisor));
    uart_hal_divisor_high_cfg(hal, DIVISOR_DLH(divisor));
    uart_hal_divisor_access(hal, 0);

    return UART_SUCCESS;
}

static uart_status uart_parity_set(uart_hal *hal, uint32_t parity)
{
    if (hal == NULL) {
        UART_PRINTF("uart_parity_set NULL pointer\n");
        return UART_ERROR_NULL_PTR;
    }

    /* Configure parity */
    if (parity == UART_PARITY_EVEN) {
        uart_hal_parity_enable(hal, 1);
        uart_hal_event_parity_select(hal, 1);
    } else if (parity == UART_PARITY_ODD) {
        uart_hal_parity_enable(hal, 1);
        uart_hal_event_parity_select(hal, 0);
    } else {
        uart_hal_parity_enable(hal, 0);
    }

    return UART_SUCCESS;
}

/* UART module configure based on the configuration from user
 *	1)	Check the config paremeters
 *	2) Programing UART module according to synopsis programming guide as below:
 *		1, Configure Modem Control Registers if loopback enabled
 *		2, Configure divisor to DLH/DLL
 *		3, Configure parity
 *		4, Configure data bits
 *		5, Configure stop bits
 *		6, Enable FIFO mode
 *	3) Set UART instance state
 * */
static uart_status uart_configure(uart_dev *dev, uart_config *cfg)
{
    uart_hal *hal = dev->hal;
    uint32_t divisor;
    uart_status status;

    if (dev == NULL || cfg == NULL) {
        UART_PRINTF("uart_configure NULL pointer\n");
        return UART_ERROR_NULL_PTR;
    }

    if ((dev->state & UART_INIT) == 0)
        return UART_ERROR_DEVICE_STATE;

    status = uart_config_param_check(cfg);
    if (status)
        return status;

    status = uart_baudrate_divisor_get(cfg->baudrate, &divisor);
    if (status) {
        UART_PRINTF("uart_configure baudrate divisor get failure\n");
        return status;
    }

    if (cfg->loopback)
        uart_hal_loopback_enable(hal, cfg->loopback);

    status = uart_baudrate_divisor_set(dev->hal, divisor);
    if (status) {
        UART_PRINTF("uart_configure baudrate divisor get failure\n");
        return status;
    }

    status = uart_parity_set(dev->hal, cfg->parity);
    if (status) {
        UART_PRINTF("uart_configure parity set failure\n");
        return status;
    }

    /* Configure data bits and stop bits */
    uart_hal_data_bits_cfg(hal, cfg->data_bits);
    uart_hal_stop_bits_cfg(hal, cfg->stop_bits);

#ifdef UART_FIFO_MODE
    /* Enable hardware FIFO */
    uart_hal_fifo_enable(hal, 1);
    // uart_hal_fifo_tet_cfg(hal, TX_FIFO_TRIG_1_2_FULL);
    // uart_hal_fifo_rt_cfg(hal, RX_FIFO_TRIG_1_CHAR);
#endif

    uart_hal_disable_intr_tx(hal);
    uart_hal_disable_intr_rx(hal);

    dev->config = *cfg;

    dev->state |= UART_SETUP;

    return UART_SUCCESS;
}

#define UART_WAIT_DELAY (100UL / portTICK_RATE_MS)
uart_status uart_xfr_init(uart_xfr *xfr, uint8_t *buf, uint32_t size, uint32_t timeout, uart_dir dir, uint32_t thres)
{
    if (xfr == NULL) {
        UART_PRINTF("uart_xfer_init param invalid\n");
        return UART_ERROR_NULL_PTR;
    }

    xfr->ring = buf;
    xfr->size = size;
    xfr->rd_idx = 0;
    xfr->wr_idx = 0;
    xfr->thres = thres;

#ifdef UART_SEM_SYNC
    xfr->timeout = timeout;

    if (dir == UART_TX_DIR)
        xfr->sync_sem = xSemaphoreCreateCounting(xfr->size - 1, xfr->size - 1);
    else if (dir == UART_RX_DIR)
        xfr->sync_sem = xSemaphoreCreateCounting(xfr->size - 1, 0);
#endif

    return UART_SUCCESS;
}

/* Open UART instance:
 * 1) Enable platform configuration
 * 2) Initialize UART instance device
 * */
#define UART_WAIT_DELAY (100UL / portTICK_RATE_MS)
uart_status uart_open(uart_instance instance, uart_config *config)
{
    uart_dev *dev;
    uint8_t enable = 1;
    uart_status status;

    dev = uart_get_dev(instance);

    if (dev == NULL || config == NULL) {
        UART_PRINTF("uart_open param invalid\n");
        return UART_ERROR_NULL_PTR;
    }

    if ((dev->state & UART_INIT) != 0) {
        UART_PRINTF("uart_open device already opened\n");
        return UART_ERROR_DEVICE_STATE;
    }

#if CONFIG_BOARD_QCC730_UART_ENABLE
    status = uart_platform(enable);
#else
#warning "CONFIG_BOARD_QCC730_UART_ENABLE should be defined. See board_defconfig"
    status = UART_ERROR;
#endif

    if (status != UART_SUCCESS) {
        UART_PRINTF("uart_open platform enable failed %d\n", status);
        return status;
    }

    memset(dev, 0, sizeof(uart_dev));

#if CONFIG_SOC_QCC730V1
    dev->hal = (uart_hal *)(QCC730V1_UART_BASE_BASE);
#elif CONFIG_SOC_QCC730V2
    dev->hal = (uart_hal *)(QCC730V2_UART_BASE_BASE);
#endif
    uart_xfr_init(&dev->tx, tx_buf, sizeof(tx_buf), portMAX_DELAY, UART_TX_DIR, (UART_HW_FIFO_SIZE / 2));
    uart_xfr_init(&dev->rx, rx_buf, sizeof(rx_buf), portMAX_DELAY, UART_RX_DIR, (UART_HW_FIFO_SIZE / 2));

    dev->state = UART_INIT;

    status = uart_configure(dev, config);

    if (status != UART_SUCCESS) {
        UART_PRINTF("uart_open configure failed %d\n", status);
        dev->state = UART_UNINIT;
        return status;
    }

    UART_DUMP(dev, UART_DUMP_CONF | UART_DUMP_REG);

    return UART_SUCCESS;
}

uart_status uart_open_with_rx_timeout(uart_instance instance, uart_config *config, uint32_t timeout)
{
    uart_dev *dev;
    uint8_t enable = 1;
    uart_status status;

    dev = uart_get_dev(instance);

    if (dev == NULL || config == NULL) {
        UART_PRINTF("uart_open param invalid\n");
        return UART_ERROR_NULL_PTR;
    }

    if ((dev->state & UART_INIT) != 0) {
        UART_PRINTF("uart_open device already opened\n");
        return UART_ERROR_DEVICE_STATE;
    }

#if CONFIG_BOARD_QCC730_UART_ENABLE
    status = uart_platform(enable);
#else
#warning "CONFIG_BOARD_QCC730_UART_ENABLE should be defined. See board_defconfig"
    status = UART_ERROR;
#endif

    if (status != UART_SUCCESS) {
        UART_PRINTF("uart_open platform enable failed %d\n", status);
        return status;
    }

    memset(dev, 0, sizeof(uart_dev));

#if CONFIG_SOC_QCC730V1
    dev->hal = (uart_hal *)(QCC730V1_UART_BASE_BASE);
#elif CONFIG_SOC_QCC730V2
    dev->hal = (uart_hal *)(QCC730V2_UART_BASE_BASE);
#endif
    uart_xfr_init(&dev->tx, tx_buf, sizeof(tx_buf), portMAX_DELAY, UART_TX_DIR, (UART_HW_FIFO_SIZE / 2));
    uart_xfr_init(&dev->rx, rx_buf, sizeof(rx_buf), timeout, UART_RX_DIR, (UART_HW_FIFO_SIZE / 2));

    dev->state = UART_INIT;

    status = uart_configure(dev, config);

    if (status != UART_SUCCESS) {
        UART_PRINTF("uart_open configure failed %d\n", status);
        dev->state = UART_UNINIT;
        return status;
    }

    UART_DUMP(dev, UART_DUMP_CONF | UART_DUMP_REG);

    return UART_SUCCESS;
}

/* Close UART instance:
 * 1) Disable platform configuration
 * 2) Un-init UART instance device
 * */
uart_status uart_close(uart_instance instance)
{
    uart_dev *dev;
    uint8_t enable = 0;
    uart_status status;

    dev = uart_get_dev(instance);

    if (dev == NULL) {
        UART_PRINTF("uart_close get device NULL\n");
        return UART_ERROR_NULL_PTR;
    }

    if ((dev->state & UART_INIT) == 0) {
        UART_PRINTF("uart_close device didn't be opened\n");
        return UART_ERROR_DEVICE_STATE;
    }

    status = uart_platform(enable);

    if (status != UART_SUCCESS) {
        UART_PRINTF("uart_close platform disable failed %d\n", status);
        return status;
    }

    memset(dev, 0, sizeof(uart_dev));

    return UART_SUCCESS;
}

#define CORTEX_M4_ICSR            0xE000ED04
#define CORTEX_M4_ISR_ACTIVE_MASK 0x1FF
static inline int32_t uart_in_isr()
{
    uint32_t icsr;
    uint32_t in_isr;

    icsr = NT_REG_RD(CORTEX_M4_ICSR);
    in_isr = (icsr & CORTEX_M4_ISR_ACTIVE_MASK) > 0 ? 1 : 0;

    return in_isr;
}

#define UART_WAIT_DELAY (100UL / portTICK_RATE_MS)
uart_status uart_tx_ring_enqueue(uart_xfr *xfr, uint8_t data)
{
#ifdef UART_SEM_SYNC
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    BaseType_t sem_ret = pdFALSE;
#endif
    uint32_t is_isr;
    uint32_t wr_next;

    if (xfr == NULL)
        return UART_ERROR_NULL_PTR;

        // UART_PRINTF("uart_tx_ring_enqueue enter\r\n");
#ifdef UART_SEM_SYNC
    is_isr = uart_in_isr();

    if (is_isr)
        sem_ret = xSemaphoreTakeFromISR(xfr->sync_sem, &xHigherPriorityTaskWoken);
    else
        sem_ret = xSemaphoreTake(xfr->sync_sem, xfr->timeout);

    if (sem_ret == pdFALSE)
        return UART_ERROR_TX_ENQUEUE_SEM_SYNC;
#endif

    taskENTER_CRITICAL();
    wr_next = xfr->wr_idx + 1;

    if (wr_next >= xfr->size)
        wr_next = 0;

    if (wr_next == xfr->rd_idx) {
        xfr->full_cnt++;
        taskEXIT_CRITICAL();
        return UART_ERROR_TX_ENQUEUE_FULL;
    }

    xfr->ring[xfr->wr_idx] = data;
    xfr->wr_idx = wr_next;

    taskEXIT_CRITICAL();

    return UART_SUCCESS;
}

// This function only called from ISR
uart_status uart_tx_ring_dequeue(uart_xfr *xfr, uint8_t *data)
{
#ifdef UART_SEM_SYNC
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
#endif

    if (xfr == NULL || data == NULL)
        return UART_ERROR_NULL_PTR;

    if (xfr->rd_idx == xfr->wr_idx)
        return UART_ERROR_TX_DEQUEUE_EMPTY;

    *data = xfr->ring[xfr->rd_idx++];

    if (xfr->rd_idx >= xfr->size)
        xfr->rd_idx = 0;

#ifdef UART_SEM_SYNC
    xSemaphoreGiveFromISR(xfr->sync_sem, &xHigherPriorityTaskWoken);
#endif

    return UART_SUCCESS;
}

// This function only called from ISR
uart_status uart_rx_ring_enqueue(uart_xfr *xfr, uint8_t data)
{
#ifdef UART_SEM_SYNC
    BaseType_t pxHigherPriorityTaskWoken = pdFALSE;
#endif
    uint32_t wr_next;

    if (xfr == NULL)
        return UART_ERROR_NULL_PTR;

    wr_next = xfr->wr_idx + 1;

    if (wr_next >= xfr->size)
        wr_next = 0;

    if (wr_next == xfr->rd_idx) {
        xfr->full_cnt++;
        return UART_ERROR_RX_ENQUEUE_FULL;
    }

    xfr->ring[xfr->wr_idx] = data;
    xfr->wr_idx = wr_next;

#ifdef UART_SEM_SYNC
    xSemaphoreGiveFromISR(xfr->sync_sem, &pxHigherPriorityTaskWoken);
#endif

    return UART_SUCCESS;
}

uart_status uart_rx_ring_dequeue(uart_xfr *xfr, uint8_t *data)
{
    BaseType_t sem_ret = pdFALSE;

    if (xfr == NULL || data == NULL)
        return UART_ERROR_NULL_PTR;

#ifdef UART_SEM_SYNC
    sem_ret = xSemaphoreTake(xfr->sync_sem, xfr->timeout);

    if (sem_ret == pdFALSE)
        return UART_ERROR_RX_DEQUEUE_SEM_SYNC;
#endif

    taskENTER_CRITICAL();

    if (xfr->rd_idx == xfr->wr_idx)
        return UART_ERROR_RX_DEQUEUE_EMPTY;

    *data = xfr->ring[xfr->rd_idx++];

    if (xfr->rd_idx >= xfr->size)
        xfr->rd_idx = 0;

    taskEXIT_CRITICAL();

    return UART_SUCCESS;
}

uint32_t uart_ring_thres_reach_check(uart_xfr *xfr)
{
    if (xfr == NULL)
        return 0;

#ifdef UART_FIFO_MODE
    if (xfr->wr_idx >= xfr->rd_idx) {
        return ((xfr->wr_idx - xfr->rd_idx) >= xfr->thres) ? 1 : 0;
    } else {
        return (((xfr->wr_idx + xfr->size) - xfr->rd_idx) >= xfr->thres) ? 1 : 0;
    }
#else
    return 1;
#endif
}

uart_status uart_transmit(uart_instance instance, uint8_t *buf, uint32_t size, uint32_t *sent)
{
    uart_dev *dev;
    uart_status status;
    uint32_t enqueued = 0;

    dev = uart_get_dev(instance);

    if (dev == NULL) {
        UART_PRINTF("uart_transmit get device NULL\n");
        return UART_ERROR_NULL_PTR;
    }

    if (buf == NULL || size == 0) {
        UART_PRINTF("uart_transmit params error\n");
        return UART_ERROR_INVALID_PARAM;
    }

    if (dev->state != UART_READY) {
        UART_PRINTF("uart_transmit device didn't be opened\r\n");
        return UART_ERROR_DEVICE_STATE;
    }

    *sent = 0;

    UART_PRINTF("uart_transmit start to enqueue\r\n");
    while (size > 0) {
        status = uart_tx_ring_enqueue(&dev->tx, *buf);

        if (status) {
            UART_PRINTF("uart_transmit tx ring enqueued status %d\r\n", status);

            break;
        } else {
            size--;
            buf++;
            enqueued++;

            if (uart_ring_thres_reach_check(&dev->tx)) {
                uart_hal_enable_intr_tx(dev->hal);
            }
        }
    }

    UART_PRINTF("uart_transmit end to enqueue\r\n");

    if (enqueued != 0) {
        *sent = enqueued;
        uart_hal_enable_intr_tx(dev->hal);
    }

    return status;
}

uart_status uart_receive(uart_instance instance, uint8_t *buf, uint32_t size, uint32_t *recv)
{
    uart_dev *dev;
    uart_status status;
    uint32_t dequeued = 0;

    dev = uart_get_dev(instance);

    if (dev == NULL) {
        UART_PRINTF("uart_receive get device NULL\n");
        return UART_ERROR_NULL_PTR;
    }

    if (buf == NULL || recv == NULL || size == 0) {
        UART_PRINTF("uart_receive params error\n");
        return UART_ERROR_INVALID_PARAM;
    }

    if (dev->state != UART_READY) {
        UART_PRINTF("uart_receive device didn't be opened\r\n");
        return UART_ERROR_DEVICE_STATE;
    }

    uart_hal_enable_intr_rx(dev->hal);

    while (size--) {
        status = uart_rx_ring_dequeue(&dev->rx, buf++);

        if (status) {
            // UART_PRINTF("uart_receive rx ring dequeued err %d\r\n", status);
            break;
        } else {
            dequeued++;
        }
    }

    uart_hal_disable_intr_rx(dev->hal);

    if (dequeued == 0) {
        return status;
    } else {
        *recv = dequeued;
        return UART_SUCCESS;
    }
}

static uint32_t uart_fifo_write(uart_hal *hal, const uint8_t *data, uint32_t len)
{
    uint32_t num = 0;

    if (hal == NULL)
        return 0;

    while ((len - num) > 0) {
        uart_hal_tx_write(hal, data[num++]);
    }

    return num;
}

static uint32_t uart_fifo_read(uart_hal *hal, uint8_t *data, uint32_t len)
{
    uint32_t num = 0;

    if (hal == NULL)
        return 0;

    while ((len - num) > 0) {
        data[num++] = uart_hal_rx_read(hal);
    }

    return num;
}

static void uart_irqs_get(uart_irqs *irqs, uart_hal *hal)
{
    uint32_t irq_raw, status_raw;

    if (irqs == NULL || hal == NULL)
        return;

    irq_raw = uart_hal_intr_get(hal);

    switch (irq_raw & IIR_IID_MASK) {
        case IIR_NO_INTR_PENDING:
            irqs->no_pending = 1;
#ifndef UART_FIFO_MODE
            /* Hardware issue, IID is no_intr_pending when data arrival */
            irqs->rx_avail = 1;
#endif
            break;
        case IIR_THR_EMPTY:
            irqs->tx_empty = 1;
            break;
        case IIR_LINE_STATUS:
            irqs->line_status = 1;
            // Reading line status register to reset the irq;
            status_raw = uart_hal_line_status_get(hal);
            break;
        case IIR_RX_DATA_AVAIL:
            irqs->rx_avail = 1;
            break;
        case IIR_CHAR_TIMEOUT:
            irqs->char_timeout = 1;
            break;
        case IIR_MODEM_STATUS:
            irqs->modem_status = 1;
            break;
        case IIR_BUSY_DETECT:
            irqs->busy_detect = 1;
            break;
        default:
            break;
    }

    (void)status_raw;

    return;
}

void __attribute__((section(".after_ram_vectors"))) UART_irq_handler(void)
{
    PROF_IRQ_ENTER();
    // Fermion only have one UART instance, so irq only need to handle instance 0;
    uart_instance instance = UART_INSTANCE_0;
    uart_dev *dev;
    uart_hal *hal;
    uart_xfr *xfr_tx, *xfr_rx;
    uart_irqs *irqs;
    uart_stats *stats;
    uint8_t data;

    dev = uart_get_dev(instance);

    if ((dev->state & UART_READY) == 0)
        return;

    hal = dev->hal;
    xfr_tx = &dev->tx;
    xfr_rx = &dev->rx;
    irqs = &dev->irqs;
    stats = &dev->stats;

    // memset(irqs, 0, sizeof(uart_irqs));

    uart_irqs_get(irqs, hal);

    if (irqs->rx_avail) {
        stats->rx_irq_num++;

#ifdef UART_FIFO_MODE
        if (uart_hal_intr_rx_ready(hal)) {
            while (uart_fifo_read(hal, &data, 1) != 0) {
                if (uart_rx_ring_enqueue(xfr_rx, data) != UART_SUCCESS)
                    break;
            }
        }
#else
        /* Hardware issue that LSR data ready bit not work */
        /* while (uart_hal_intr_rx_ready(hal)) {
            if (uart_fifo_read(hal, &data, 1) == 0)
                break;

            if (uart_rx_ring_enqueue(xfr_rx, data) != UART_SUCCESS)
                break;
        } */

        if (uart_fifo_read(hal, &data, 1) != 0) {
            uart_rx_ring_enqueue(xfr_rx, data);
        }

#endif
    }

    if (irqs->tx_empty) {
        stats->tx_irq_num++;

#ifdef UART_FIFO_MODE
        if (uart_hal_intr_tx_ready(hal)) {
            while (uart_tx_ring_dequeue(xfr_tx, &data) == UART_SUCCESS) {
                if (uart_fifo_write(hal, &data, 1) == 0)
                    break;
            }

            uart_hal_disable_intr_tx(hal);
        }
#else
        while (uart_hal_intr_tx_ready(hal)) {
            if (uart_tx_ring_dequeue(xfr_tx, &data) == UART_SUCCESS) {
                if (uart_fifo_write(hal, &data, 1) == 0)
                    break;
            } else {
                uart_hal_disable_intr_tx(hal);
                break;
            }
        }
#endif
    }

    PROF_IRQ_EXIT();

    return;
}
#endif  // UART_DRV
