/**
 *Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include <string.h>
#include "fwconfig_cmn.h"
#include "nt_flags.h"
#include "ferm_i2c.h"
#include "nt_hw.h"
#include "timer.h"
#include "uart.h"
#include "nt_common.h"
#include "ferm_prof.h"

#include "autoconf.h"
#include "qccx.h"

#ifdef I2C_DRV
#ifdef I2C_DRV_DBG
static char i2c_buf[60];
#define I2C_PRINTF(...)                              \
    snprintf(i2c_buf, sizeof(i2c_buf), __VA_ARGS__); \
    nt_dbg_print(i2c_buf);
static void i2c_dump(i2c_dev *dev, uint8_t dump);
#define I2C_DUMP(dev, dump) i2c_dump(dev, dump);
#else
#define I2C_PRINTF(...)
#define I2C_DUMP(dev, config)
#endif  // I2C_DRIVER_DEBUG

static i2c_dev fm_i2c_dev[I2C_INSTANCE_MAX];

/*
 *SCL period Configuration of supported 4 speed:
 *						Speed(khz)	high(ns)	low(ns)
 *	Standard			100			4730		5270
 *	Fast				400			1340		1160
 *	Fast plus			1000		500			500
 *	High				2000		200			300
 **/
const i2c_scl_ht_lt scl_default[I2C_SPEED_NUM] = {
    {I2C_STD_SPEED_DEFAULT, I2C_STD_MODE, 4730, 5270},
    {I2C_FAST_SPEED_DEFAULT, I2C_FAST_MODE, 1340, 1160},
    {I2C_FAST_PLUS_SPEED_DEFAULT, I2C_FAST_MODE, 500, 500},
    {I2C_HIGH_SPEED_DEFAULT, I2C_HIGH_MODE, 200, 300},
};

/* Get I2C instance device structure */
static inline i2c_dev *i2c_get_dev(i2c_instance instance)
{
    if (instance >= I2C_INSTANCE_MAX)
        return NULL;
    else
        return &fm_i2c_dev[instance];
}

/* Get the capability of Fermion I2C module*/
static inline void i2c_get_cap(i2c_dev *dev)
{
    i2c_cap *cap = &dev->cap;

    // Get the default value on initialization: supported speed mode, spk lenth and tx/rx fifo depth in hardware.
    i2c_hal_max_speed_mode(dev->hal, &(cap->support_mode));
    i2c_hal_spklen(dev->hal, &(cap->fs_spklen), &(cap->hs_spklen));
    i2c_hal_tx_rx_fifo_depth(dev->hal, &(cap->tx_depth), &(cap->rx_depth));

    // Verified I2C clock frequency is 60Mhz
    cap->clk_khz = 60000;
    // RX fifo threshold as 0 to trigger RX interruption whenever data is arrived.
    cap->rx_tl = 0;
    // TX fifo threshold as 0 to trigger TX empty interruption only when tx fifo is truly empty
    cap->tx_tl = 0;

    return;
}

#ifdef I2C_DRV_DBG
/* I2C dump function for debug */
static void i2c_dump(i2c_dev *dev, uint8_t dump)
{
    i2c_cap *cap;
    i2c_config *config;
    i2c_hal *hal;
    i2c_xfr *xfr;
    i2c_stats *stats;
    uint32_t value, i;

    if (dev == NULL)
        return;

    if (dump & I2C_DUMP_CAP) {
        cap = &dev->cap;
        I2C_PRINTF("I2C device cap dump\r\n");
        I2C_PRINTF("	fs_spklen:%d\r\n", cap->fs_spklen);
        I2C_PRINTF("	hs_spklen:%d\r\n", cap->hs_spklen);
        I2C_PRINTF("	rx_depth:%d\r\n", cap->rx_depth);
        I2C_PRINTF("	tx_depth:%d\r\n", cap->tx_depth);
        I2C_PRINTF("	tx_tl:%d\r\n", cap->tx_tl);
        I2C_PRINTF("	rx_tl:%d\r\n", cap->rx_tl);
        I2C_PRINTF("	support_mode:%d\r\n", cap->support_mode);
        I2C_PRINTF("	clk_khz:%d\r\n", (int)cap->clk_khz);
    }

    if (dump & I2C_DUMP_CONF) {
        config = &dev->config;
        I2C_PRINTF("I2C device config dump\r\n");
        I2C_PRINTF("	freq:%d\r\n", (int)config->freq);
        I2C_PRINTF("	hcnt:%d\r\n", config->hcnt);
        I2C_PRINTF("	lcnt:%d\r\n", config->lcnt);
        I2C_PRINTF("	mode:%d\r\n", config->mode);
        I2C_PRINTF("	tar: 0x%x\r\n", config->tar_addr);
    }

    if (dump & I2C_DUMP_REG) {
#if CONFIG_SOC_QCC730V1
        hal = (i2c_hal *)(QCC730V1_I2C_BASE_BASE);
#elif CONFIG_SOC_QCC730V2
        hal = (i2c_hal *)(QCC730V2_I2C_BASE_BASE);
#endif
        I2C_PRINTF("I2C device regs dump\r\n");
        value = hal->I2C_I2C_IC_STATUS.reg;
        I2C_PRINTF("I2C_STATUS:0x%x\r\n", (unsigned int)(value));
        value = hal->I2C_I2C_IC_INTR_MASK.reg;
        I2C_PRINTF("I2C_INTR_MASK:0x%x\r\n", (unsigned int)(value));
        value = hal->I2C_I2C_IC_INTR_STAT.reg;
        I2C_PRINTF("I2C_INTR_STAT:0x%x\r\n", (unsigned int)(value));
        value = hal->I2C_I2C_IC_SS_SCL_HCNT.reg;
        I2C_PRINTF("I2C_SS_SCL_HCNT:0x%x\r\n", (unsigned int)(value));
        value = hal->I2C_I2C_IC_SS_SCL_LCNT.reg;
        I2C_PRINTF("I2C_SS_SCL_LCNT:0x%x\r\n", (unsigned int)(value));
        value = hal->I2C_I2C_IC_FS_SCL_HCNT.reg;
        I2C_PRINTF("I2C_FS_SCL_HCNT:0x%x\r\n", (unsigned int)(value));
        value = hal->I2C_I2C_IC_FS_SCL_LCNT.reg;
        I2C_PRINTF("I2C_FS_SCL_LCNT:0x%x\r\n", (unsigned int)(value));
        value = hal->I2C_I2C_IC_HS_SCL_HCNT.reg;
        I2C_PRINTF("I2C_HS_SCL_HCNT:0x%x\r\n", (unsigned int)(value));
        value = hal->I2C_I2C_IC_HS_SCL_LCNT.reg;
        I2C_PRINTF("I2C_HS_SCL_LCNT:0x%x\r\n", (unsigned int)(value));
        value = hal->I2C_I2C_IC_TX_TL.reg;
        I2C_PRINTF("I2C_TX_TL:0x%x\r\n", (unsigned int)(value));
        value = hal->I2C_I2C_IC_RX_TL.reg;
        I2C_PRINTF("I2C_RX_TL:0x%x\r\n", (unsigned int)(value));
    }

    if (dump & I2C_DUMP_XFR) {
        xfr = &dev->xfr;

        I2C_PRINTF("Dump XFR\r\n");
        I2C_PRINTF("	flag:0x%x\r\n", (uint16_t)(xfr->flag));
        I2C_PRINTF("	len:%d\r\n", (int)xfr->len);
        if (xfr->len) {
            I2C_PRINTF("	data:");
            for (i = 0; i < xfr->len; i++) {
                I2C_PRINTF("0x%x", xfr->buf[i]);
            }
            I2C_PRINTF("\r\n");
        }
    }

    if (dump & I2C_DUMP_ERR) {
        stats = &dev->stats;
        I2C_PRINTF("Dump dev error:\r\n");
        // I2C_PRINTF("	irq_err:0x%x, 0x%x\r\n", (int16_t)((stats->irq_err) >> 16), (int16_t)(stats->irq_err));
        I2C_PRINTF("	irq_err:0x%x\r\n", (unsigned int)stats->irq_err);
        I2C_PRINTF("	irq_cnt:%d\r\n", (int)stats->irq_cnt);
        I2C_PRINTF("\r\n");
    }

    return;
}
#endif

/* Caculate the hcnt and lcnt of SCL according to the speed */
static i2c_status i2c_get_scl_hcnt_lcnt(i2c_cap *cap, i2c_config *config, uint32_t freq)
{
    uint8_t i, spklen, find = 0;
    uint32_t low_time, high_time;

    for (i = 0; i < I2C_SPEED_NUM; i++) {
        if (freq == scl_default[i].freq) {
            config->freq = freq;
            config->mode = scl_default[i].mode;
            low_time = scl_default[i].low_time;
            high_time = scl_default[i].high_time;

            find = 1;
            break;
        }
    }

    if (find == 1) {
        if (config->mode == I2C_HIGH_MODE)
            spklen = cap->hs_spklen;
        else
            spklen = cap->fs_spklen;

        config->hcnt = HCNT_CAL(high_time, cap->clk_khz, spklen);
        config->lcnt = LCNT_CAL(low_time, cap->clk_khz);
    } else {
        return I2C_ERROR_INVALID_PARAM;
    }

    return I2C_SUCCESS;
}

/* Setting I2CM platform related configuration:
 * 1) I2C clock configuration
 * 2) I2C bootstrap configuration
 * 3) I2C interruption enable/disable in SOC level
 * */
static i2c_status i2c_platform(uint8_t enable)
{
#if CONFIG_SOC_QCC730V1
    PMU_BASE_pmu_Type *pmu = (PMU_BASE_pmu_Type *)(QCC730V1_PMU_BASE_BASE);
#elif CONFIG_SOC_QCC730V2
    PMU_BASE_pmu_Type *pmu = (PMU_BASE_pmu_Type *)(QCC730V2_PMU_BASE_BASE);
#endif

    if (enable) {
        pmu->PMU_ROOT_CLK_ENABLE.bit.I2C_ROOT_CLK_ENABLE = enable;
        pmu->PMU_BOOT_STRAP_CONFIG_SECURE.reg = FERM_BOOT_STRAP_VALUE;
        pmu->PMU_BOOT_STRAP_CONFIGURATION_STATUS.bit.CFG_I2C_ENABLE = enable;

        NT_REG_WR(NT_NVIC_ISER0, ENABLE_I2C_IRQ);
    } else {
        NT_REG_WR(NT_NVIC_ICER0, ENABLE_I2C_IRQ);

        pmu->PMU_BOOT_STRAP_CONFIG_SECURE.reg = FERM_BOOT_STRAP_VALUE;
        pmu->PMU_BOOT_STRAP_CONFIGURATION_STATUS.bit.CFG_I2C_ENABLE = enable;
        pmu->PMU_ROOT_CLK_ENABLE.bit.I2C_ROOT_CLK_ENABLE = enable;
    }

    if (pmu->PMU_BOOT_STRAP_CONFIGURATION_STATUS.bit.CFG_I2C_ENABLE != enable)
        return I2C_ERROR_BOOTSTRAP_CFG_FAIL;

    if (pmu->PMU_ROOT_CLK_ENABLE.bit.I2C_ROOT_CLK_ENABLE != enable)
        return I2C_ERROR_BUS_CLK_CFG_FAIL;

    return I2C_SUCCESS;
}

/* Open I2C instance:
 * 1) Enable platform configuration
 * 2) Initialize I2C instance device
 * 3) Get the capability of I2C module
 * */
i2c_status i2c_open(i2c_instance instance)
{
    i2c_dev *dev;
    uint32_t comp_type;
    uint8_t enable = 1;
    i2c_status status;

    dev = i2c_get_dev(instance);
    if (dev == NULL) {
        I2C_PRINTF("i2c_open get device failed\n");
        return I2C_ERROR_INVALID_PARAM;
    }

    if ((dev->state & I2C_INIT) != 0) {
        I2C_PRINTF("i2c_open device already opened\n");
        return I2C_ERROR_DEVICE_STATE;
    }

#if CONFIG_BOARD_QCC730_I2C_ENABLE
    status = i2c_platform(enable);
#else
#warning "CONFIG_BOARD_QCC730_I2C_ENABLE should be defined. See board_defconfig"
    status = I2C_ERROR;
#endif

    if (status != I2C_SUCCESS) {
        I2C_PRINTF("i2c_open platform enable failed %d\n", status);
        return status;
    }

    memset(dev, 0, sizeof(i2c_dev));

#if CONFIG_SOC_QCC730V1
    dev->hal = (i2c_hal *)(QCC730V1_I2C_BASE_BASE);
#elif CONFIG_SOC_QCC730V2
    dev->hal = (i2c_hal *)(QCC730V2_I2C_BASE_BASE);
#endif

    i2c_hal_comp_type(dev->hal, &comp_type);

    /* verify that we have a valid DesignWare register first */
    if (comp_type != QWLAN_I2C_I2C_IC_COMP_TYPE_DEFAULT) {
        I2C_PRINTF("i2c_open I2C comparsion register type failure\n");
        memset(dev, 0, sizeof(i2c_dev));
        return I2C_ERROR_IC_COMP;
    }

    i2c_get_cap(dev);

    dev->sync_sem = nt_osal_xsemaphore_create_binary();
    dev->state = I2C_INIT;

    I2C_DUMP(dev, I2C_DUMP_CAP);

    return I2C_SUCCESS;
}

/* Close I2C instance:
 * 1) Disable I2C module
 * 2) Disable I2C platform configuration
 * 3) De-init I2C instance device
 * */
i2c_status i2c_close(i2c_instance instance)
{
    i2c_dev *dev;
    i2c_status status;
    uint8_t enable = 0;

    dev = i2c_get_dev(instance);
    if (dev == NULL) {
        I2C_PRINTF("i2c_open get device failed\n");
        return I2C_ERROR_INVALID_PARAM;
    }

    if ((dev->state & I2C_INIT) == 0) {
        I2C_PRINTF("i2c_open device didn't opened\n");
        return I2C_ERROR_DEVICE_STATE;
    }

    i2c_hal_disable(dev->hal);

    if (dev->sync_sem)
        nt_osal_semaphore_delete(dev->sync_sem);

    status = i2c_platform(enable);

    if (status != I2C_SUCCESS) {
        I2C_PRINTF("i2c_open platform disable failed %d\n", status);
        return status;
    }

    memset(dev, 0, sizeof(i2c_dev));

    return I2C_SUCCESS;
}

/* Cancel the transfer if I2C instance working on it */
i2c_status i2c_cancel_transfer(i2c_instance instance)
{
    i2c_dev *dev;
    uint8_t max_cnt = 10;

    dev = i2c_get_dev(instance);
    if (dev == NULL)
        return I2C_ERROR_INVALID_PARAM;

    if ((dev->state & I2C_BUSY) == 0)
        return I2C_CANCEL_TRANSFER_INVALID;

    while ((dev->state & I2C_BUSY) && (max_cnt--)) {
        i2c_hal_abort_tx(dev->hal);
        hres_timer_us_delay(1000);
    }

    if ((dev->state & I2C_BUSY) != 0)
        return I2C_ERROR_CANCEL_TRANSFER_FAIL;

    return I2C_SUCCESS;
}

/* I2C module configure based on the capability and configuration from user
 *	1) Caculate SCL HCNT and LCNT
 *	2) Programing I2 module according to synopsis programming guide as below:
 *		1, Disable I2C
 *		2, Disable interruption
 *		3, Set CON registers
 *		4, Set slave address
 *		5, Set speed mode
 *		6, Config SCL LCNT and HCNT
 *		7, Config TX/RX FIFO TL
 *		8, Enable I2C
 *	3) Set I2C instance state
 * */
static i2c_status i2c_configure(i2c_dev *dev, uint16_t addr, uint32_t freq)
{
    i2c_hal *hal = dev->hal;
    i2c_config *config = &dev->config;
    i2c_cap *cap = &dev->cap;
    uint32_t value;
    i2c_status status;

    if ((dev->state & I2C_INIT) == 0)
        return I2C_ERROR_DEVICE_STATE;

    status = i2c_get_scl_hcnt_lcnt(cap, config, freq);

    if (status != I2C_SUCCESS) {
        dev->state &= ~I2C_SETUP;
        return status;
    }

    i2c_hal_disable(hal);

    i2c_hal_intr_mask(hal, 0);
    i2c_hal_intr_clear_all(hal, &value);

    i2c_hal_slave_disable(hal, 1);
    i2c_hal_restart_enable(hal, 1);
    i2c_hal_slave_10bit_addr_enable(hal, 0);
    i2c_hal_master_10bit_addr_enable(hal, 0);
    i2c_hal_master_enable(hal, 1);

    i2c_hal_speed_mode(hal, config->mode);

    i2c_hal_tar_write(hal, addr);
    config->tar_addr = addr;

    if (config->mode == I2C_STD_MODE)
        i2c_hal_ss_scl_config(hal, config->lcnt, config->hcnt);
    else if (config->mode == I2C_FAST_MODE)
        i2c_hal_fs_scl_config(hal, config->lcnt, config->hcnt);
    else
        i2c_hal_hs_scl_config(hal, config->lcnt, config->hcnt);

    i2c_hal_tl_rl_config(hal, cap->tx_tl, cap->rx_tl);

    i2c_hal_enable(hal);

    dev->state |= I2C_SETUP;

    I2C_DUMP(dev, I2C_DUMP_CAP | I2C_DUMP_CONF | I2C_DUMP_REG);

    return I2C_SUCCESS;
}

/* Initilize the xfr structure for I2C tx/rx handler */
static i2c_status i2c_transfer_init(i2c_msg *msg, i2c_xfr *xfr, uint8_t last)
{
    if (msg == NULL)
        return I2C_ERROR_INIT_XFR;

    if ((msg->flag & I2C_MSG_RW_MASK) == 0 || (msg->flag & I2C_MSG_RW_MASK) == I2C_MSG_RW_MASK || msg->len == 0)
        return I2C_ERROR_INIT_XFR;

    // Setting stop flag for the last msg.
    if (last)
        msg->flag |= I2C_MSG_STOP;

    xfr->buf = msg->buf;
    xfr->len = msg->len;
    xfr->flag = msg->flag;

    // Setting restart flag if the msg direction changed.
    if ((xfr->last_dir != 0) && (xfr->last_dir != (xfr->flag & I2C_MSG_RW_MASK)))
        xfr->flag |= I2C_MSG_RESTART;

    if ((xfr->flag & I2C_MSG_RW_MASK) == I2C_MSG_WRITE) {
        xfr->last_dir = I2C_MSG_WRITE;
    } else {
        xfr->request_bytes = msg->len;
        xfr->rx_pending = 0;
        xfr->last_dir = I2C_MSG_READ;
    }

    return I2C_SUCCESS;
}

/* I2C MSG transfer function:
 *	1) Configure I2C according to user's setting
 *	2) Initialize the xfr according to user's MSGs
 *	3) Waitting for tx/rx handle done in IRQ
 * */
i2c_status i2c_transfer(i2c_instance instance, i2c_msg *msgs, uint8_t num_msgs, uint16_t tar_addr, uint32_t freq)
{
    i2c_dev *dev;
    i2c_msg *cur_msg = msgs;
    i2c_xfr *xfr;
    uint8_t left = 0, last = 0;
    i2c_status status = I2C_SUCCESS;

    // Valid check for msgs
    if (num_msgs == 0 || msgs == NULL)
        return I2C_ERROR_INVALID_PARAM;

    dev = i2c_get_dev(instance);
    if (dev == NULL)
        return I2C_ERROR_INVALID_PARAM;

    if ((dev->state & I2C_INIT) == 0) {
        I2C_PRINTF("i2c_open device didn't opened\r\n");
        return I2C_ERROR_DEVICE_STATE;
    }

    if ((dev->state & I2C_BUSY) != 0) {
        I2C_PRINTF("i2c_open device is busy\r\n");
        return I2C_EEROR_TRANSFER_BUSY;
    }

    status = i2c_configure(dev, tar_addr, freq);

    if (dev->state != I2C_READY) {
        I2C_PRINTF("i2c_open configuration failed %d\r\n", status);
        I2C_DUMP(dev, I2C_DUMP_CAP | I2C_DUMP_CONF | I2C_DUMP_REG);
        return status;
    }

    dev->state |= I2C_BUSY;

    xfr = &(dev->xfr);

    // MSGs handle
    for (left = num_msgs; left > 0; left--, cur_msg++) {
        if ((cur_msg->flag & I2C_MSG_RW_MASK) == I2C_MSG_WRITE)
            dev->state |= I2C_SEND;
        else if ((cur_msg->flag & I2C_MSG_RW_MASK) == I2C_MSG_READ)
            dev->state |= I2C_RECV;

        if (left == 1)
            last = 1;
        // Initilize the xfer for transfer handle in IRQ
        status = i2c_transfer_init(cur_msg, xfr, last);

        I2C_DUMP(dev, I2C_DUMP_XFR);

        if (status != I2C_SUCCESS)
            break;

        // Enable TX/RX interruptions
        i2c_hal_intr_mask(dev->hal, I2C_INTR_TX | I2C_INTR_RX);

        // Take the sync sem which would be released after msg handle done in the ISR
        if (nt_pass != nt_osal_semaphore_take(dev->sync_sem, I2C_WAIT_DELAY)) {
            status = I2C_EEROR_TRANSFER_TIMEOUT;
            break;
        }

        // I2C transfer aborted, check the irq err status then return
        if (dev->state & I2C_ABORT) {
            if (dev->stats.irq_err & I2C_INTR_TX_ABRT)
                status = I2C_ERROR_TRANSFER_FORCE_TERMINATED;
            else if (dev->stats.irq_err & I2C_INTR_TX_OV)
                status = I2C_ERROR_OUTPUT_FIFO_OVER_RUN;
            else if (dev->stats.irq_err & I2C_INTR_RX_OV)
                status = I2C_ERROR_INPUT_FIFO_OVER_RUN;
            else if (dev->stats.irq_err & I2C_INTR_RX_UN)
                status = I2C_ERROR_INPUT_FIFO_UNDER_RUN;

            I2C_DUMP(dev, I2C_DUMP_ERR);
            break;
        }

        // Waite for 5ms in case of some I2C devices can't ack immediately sometimes after last transaction is done.
        if (xfr->flag & I2C_MSG_STOP)
            hres_timer_us_delay(I2C_TRANS_INTERVAL);
    }

    dev->state = I2C_INIT;

    return status;
}

/* I2C data recv function, called from I2C IRQ to get received data when RX FULL irq occured */
static i2c_status i2c_data_recv(i2c_dev *dev)
{
    i2c_xfr *xfr = &dev->xfr;
    i2c_hal *hal = dev->hal;

    while (xfr->len > 0 && i2c_hal_rx_fifo_not_empty(hal)) {
        if (i2c_hal_tx_abort_intr(hal)) {
            dev->stats.irq_err = I2C_INTR_TX_ABRT;
            return I2C_ERROR_TX_ABORT_INTR;
        }

        xfr->buf[0] = (uint8_t)i2c_hal_data_cmd_read(hal);
        xfr->buf++;
        xfr->len--;
        xfr->rx_pending--;

        if (xfr->len == 0UL)
            break;
    }

    if (xfr->len == 0)
        dev->state &= ~I2C_RECV;

    return I2C_SUCCESS;
}

/* I2C data read function, called from I2C IRQ to ask for the data
 *	when we have data to ask and TX FIFO empty IRQ happened.
 **/
static i2c_status i2c_data_read(i2c_dev *dev)
{
    i2c_xfr *xfr = &dev->xfr;
    i2c_hal *hal = dev->hal;
    uint32_t tx_fl, rx_fl;
    int32_t rx_avail;
    uint32_t tx_avail, final_avail;
    uint32_t read_cmd = I2C_DATA_CMD_READ;

    if (xfr->request_bytes == 0)
        return I2C_SUCCESS;

    // Get TX/RX FIFO level for calucating avaliable transfer slot.
    i2c_hal_tx_fifo_level(hal, &tx_fl);
    i2c_hal_rx_fifo_level(hal, &rx_fl);

    rx_avail = dev->cap.rx_depth - rx_fl - xfr->rx_pending;

    if (rx_avail <= 0) {
        // Wait to read data in next IRQ
        return I2C_RX_FIFO_EXPECT_FULL;
    }

    tx_avail = dev->cap.tx_depth - tx_fl;

    final_avail = MIN(MIN((uint32_t)rx_avail, tx_avail), xfr->request_bytes);

    while (final_avail > 0) {
        if (i2c_hal_tx_abort_intr(hal)) {
            dev->stats.irq_err = I2C_INTR_TX_ABRT;
            return I2C_ERROR_TX_ABORT_INTR;
        }

        /* Send RESTART if needed */
        if (xfr->flag & I2C_MSG_RESTART) {
            read_cmd |= I2C_DATA_CMD_RESTART;
            xfr->flag &= ~(I2C_MSG_RESTART);
        } 
        else if ((read_cmd & I2C_DATA_CMD_RESTART)) {
            read_cmd &= ~ (I2C_DATA_CMD_RESTART);
        }

        /* Send STOP if needed */
        if ((xfr->len == 1U || xfr->request_bytes == 1U) && (xfr->flag & I2C_MSG_STOP)) {
            read_cmd |= I2C_DATA_CMD_STOP;
        }

        i2c_hal_data_cmd_write(hal, read_cmd);

        xfr->request_bytes--;
        xfr->rx_pending++;
        final_avail--;
    }

    return I2C_SUCCESS;
}

/* I2C data write function, called from IRQ when there has data to write and TX FIFO empty IRQ occured. */
static i2c_status i2c_data_write(i2c_dev *dev)
{
    i2c_xfr *xfr = &dev->xfr;
    i2c_hal *hal = dev->hal;
    uint32_t data;

    while (xfr->len > 0 && i2c_hal_tx_fifo_not_full(hal)) {
        if (i2c_hal_tx_abort_intr(hal)) {
            dev->stats.irq_err = I2C_INTR_TX_ABRT;
            return I2C_ERROR_TX_ABORT_INTR;
        }

        data = (uint32_t)xfr->buf[0];

        /* Send RESTART if needed */
        if (xfr->flag & I2C_MSG_RESTART) {
            data |= I2C_DATA_CMD_RESTART;
            xfr->flag &= ~(I2C_MSG_RESTART);
        }

        /* Send STOP if needed */
        if ((xfr->len == 1U) && (xfr->flag & I2C_MSG_STOP)) {
            data |= I2C_DATA_CMD_STOP;
        }

        i2c_hal_data_cmd_write(hal, data);

        xfr->buf++;
        xfr->len--;
    }

    if (xfr->len == 0)
        dev->state &= ~I2C_SEND;

    return I2C_SUCCESS;
}

/* When transfer done, disable IRQ and give the semaphore to transfering thread */
static void i2c_transfer_done(i2c_dev *dev)
{
    uint32_t value;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    // Disable all the intr
    i2c_hal_intr_mask(dev->hal, 0);
    i2c_hal_intr_clear_all(dev->hal, &value);

    nt_osal_semaphore_give_from_isr(dev->sync_sem, &xHigherPriorityTaskWoken);
}

void __attribute__((section(".after_ram_vectors"))) I2C_irq_handler(void)
{
    PROF_IRQ_ENTER();
    // Fermion only have one I2C instance, so irq only need to handle instance 0;
    i2c_instance instance = I2C_INSTANCE_0;
    i2c_dev *dev;
    i2c_hal *hal;
    i2c_xfr *xfr;
    uint32_t isr_raw;
    i2c_status status = 0;

    dev = i2c_get_dev(instance);

    if ((dev->state & I2C_READY) == 0)
        return;

    dev->stats.irq_cnt++;

    hal = dev->hal;
    xfr = &dev->xfr;

    i2c_hal_intr_stat(hal, &isr_raw);

    // Check if there error happends
    if (isr_raw & I2C_INTR_ERROR) {
        dev->stats.irq_err = isr_raw & I2C_INTR_ERROR;
        dev->state |= I2C_ABORT;

        i2c_transfer_done(dev);
        return;
    }

    // Check if RX FIFO FULL
    if ((isr_raw & I2C_INTR_RX_FULL) && ((xfr->flag & I2C_MSG_READ) != 0)) {
        status = i2c_data_recv(dev);

        if (status == I2C_ERROR_TX_ABORT_INTR) {
            dev->state |= I2C_ABORT;
            i2c_transfer_done(dev);
            return;
        }
    }

    if (isr_raw & I2C_INTR_TX_EMPTY) {
        if ((xfr->flag & I2C_MSG_RW_MASK) == I2C_MSG_WRITE) {
            status = i2c_data_write(dev);
        } else {
            status = i2c_data_read(dev);
        }

        if (status) {
            if (status == I2C_ERROR_TX_ABORT_INTR) {
                dev->state |= I2C_ABORT;
                i2c_transfer_done(dev);
                return;
            }

            if (status == I2C_RX_FIFO_EXPECT_FULL)
                return;
        }

        if (xfr->len == 0 && !(xfr->flag & I2C_MSG_STOP)) {
            i2c_transfer_done(dev);
        }

        if (isr_raw & I2C_INTR_STOP_DET) {
            i2c_hal_intr_clear_stop_det(hal);
            i2c_transfer_done(dev);
        }
    }

    PROF_IRQ_EXIT();
    return;
}
#endif  // I2C_DRV
