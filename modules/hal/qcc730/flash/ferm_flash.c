/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*-------------------------------------------------------------------------
 * Include Files
 *-----------------------------------------------------------------------*/

#include "fwconfig_cmn.h"
#include "nt_flags.h"
#include "nt_logger_api.h"
#ifndef CONFIG_NON_OS
#include "wifi_fw_pwr_cb_infra.h"
#endif
#include "timer.h"
#ifndef CONFIG_NON_OS
#include "qurt_mutex.h"
#endif

#include "ferm_qspi.h"
#include "ferm_flash.h"

#ifdef CONFIG_BOARD_QCC730_QSPI_ENABLE
/*-------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 *-----------------------------------------------------------------------*/
/* Flash XiP support if needed */
//#define FLASH_XIP_SUPPORT

/* If need, do power up operation before flash init for some flash */
#define FLASH_POWER_UP_SUPPORT

/* Enable flash high performance if neeed */
#define FLASH_HP_MODE_SUPPORT

/* Read/Write flash registers */
#define FLASH_ACCESS_REG_SUPPORT

#define QSPI_TRANS_MODE QSPI_PIO_MODE_E

#define WRITE_STATUS_POLLING_USEC      100
#define WRITE_TIMEOUT                  5000000 /**< Time unit is usec. */
#define ERASE_STATUS_POLLING_MSEC      5
#define ERASE_TIMEOUT                  5000000 /**< Time unit is usec. */
#define READ_STATUS_POLLING_USEC       20
#define READ_STATUS_TIMEOUT            5000000 /**< Time unit is usec. */
#define CHIP_ERASE_STATUS_POLLING_MSEC 1000
#define CHIP_ERASE_TIMEOUT             500000000 /**< Time unit is usec. */

#define WRITE_ENABLE_CMD        0x06
#define READ_STATUS_CMD         0x05
#define READ_STATUS_2_CMD       0x3F
#define READ_CFG_REG_CMD        0x15
#define READ_CFG1_CMD           0x35
#define WRITE_STATUS_CMD        0x01
#define ENTER_4B_ADDR_CMD       0xB7
#define WRITE_STATUS_2_CMD      0x3E
#define READ_IDENTIFICATION_CMD 0x9F

#define STATUS_WR_EN_MASK          0x02
#define PROG_ERASE_WRITE_BUSY_BMSK 0x01
#define READ_STATUS_BUSY_MASK      0x01

#define FLASH_PID2VID(__pid__) ((uint8_t)(__pid__))

/* Flash Manufacturer ID, from the lowest byte of device id */
#define MANUFACTURER_ID_MACRONIX 0xC2 /**< Macronix. */
#define MANUFACTURER_ID_WINBOND  0xEF /**< Winbond. */
#define MANUFACTURER_ID_ISSI     0x9D /**< ISSI. */
#define MANUFACTURER_ID_GD       0xC8 /**< GD. */
#define MANUFACTURER_ID_GT       0xC4 /**< GT. */

/* Winbond SPI Command */
#define WINBOND_READ_STATUS_2_CMD  0x35
#define WINBOND_READ_STATUS_3_CMD  0x15
#define WINBOND_WRITE_STATUS_2_CMD 0x31
#define WINBOND_WRITE_STATUS_3_CMD 0x11

#if CONFIG_BOARD_QCC730_QSPI_V2_QUAD_MODE
/* Quad enable mode. */
#define ENABLE_QUAD_MODE_0 0x0
#define ENABLE_QUAD_MODE_1 0x1
#define ENABLE_QUAD_MODE_2 0x2
#define ENABLE_QUAD_MODE_3 0x3
#define ENABLE_QUAD_MODE_4 0x4
#define ENABLE_QUAD_MODE_5 0x5
#endif

#define BIT_1 0x2
#define BIT_6 0x40
#define BIT_7 0x80

#define WRITE_OPERATION 0x0
#define ERASE_OPERATION 0x1
#define OTHER_OPERATION 0x2

/* Flash state. */
#define FLASH_STATE_WRITE        0x1
#define FLASH_STATE_READ         0x2
#define FLASH_STATE_ERASE        0x4
#define FLASH_STATE_WRITE_REG    0x8
#define FLASH_STATE_READ_REG     0x10
#define FLASH_STATE_NON_BLOCKING 0x80

/* Flash operation task event. */
#define FLASH_TSK_EVENT_TRANSFER_DONE ((uint32_t)(1 << 0))
#define FLASH_TSK_EVENT_OPERATE_DONE  ((uint32_t)(1 << 1))
#define FLASH_TSK_EVENT_OPERATE_ERROR ((uint32_t)(1 << 2))

#define FLASH_TSK_SHORT_WAIT 1 /**< Short wait of 1ms incase of polling timer started fail. */
#define NO_TIMER_COMPARATOR  0xFF

#define DIFF_TIME(__x__, __y__) ((__x__) - (__y__))

#define VALID_RW_MODE(__mode__)                                                          \
    (((__mode__) >= FLASH_RW_MODE_SDR_SINGLE && (__mode__) <= FLASH_RW_MODE_SDR_QUAD) || \
     ((__mode__) == FLASH_RW_MODE_DDR_QUAD))

#define IS_QUAD_MODE(__mode__) (((__mode__)&0x3) == 0x3)

/*-------------------------------------------------------------------------
 * Type Declarations
 *-----------------------------------------------------------------------*/

/**
   Structure representing flash non-blocking operation parameters.
*/
typedef struct flash_operation_param_s {
    flash_operation_cb_t operation_cb; /**< The callback function for non-blocking flash operation. */
    void *user_param;                  /**< The user specified parameter for the callback function. */
    qspi_cmd_t qspi_cmd;               /**< The qspi cmd for read/writ/erase. */
    uint32_t address;                  /**< The start address for read/writ/erase. */
    uint32_t byte_cnt;                 /**< The total number of data for read/write/erase. */
    uint32_t tried_cnt;                /**< The number of data tried to read/write/erase in last operation. */
    uint8_t *buffer;                   /**< The buffer pointer for read/write. */
} flash_operation_param_t;

/**
   Structure representing context for flash module.
*/
typedef struct flash_context_s {
    uint8_t state;                            /**< Flash state. */
    flash_config_data_t *config;              /**< Flash specific configurations. */
    TaskHandle_t operation_task;              /**< Flash operation task. */
    flash_operation_param_t *operation_param; /**< Flash operation parameters. */
    uint8_t timer_comparator;                 /**< Flash timer comparator. */
    uint64_t polling_start;                   /**< Record the start time of polling flash operation status. */
} flash_context_t;

/*-------------------------------------------------------------------------
 * Variables
 *-----------------------------------------------------------------------*/

static flash_context_t flash_context;
static bool flash_init_done = false;
/* Mutex to prevent concurrent accesses to the hardware */
#ifndef CONFIG_NON_OS
qurt_mutex_t flash_mutex;
#endif

/* default flash type. with default clock 30Mhz */
static uint8_t default_qspi_clock = FLASH_CLOCK_30MHZ;

/*-------------------------------------------------------------------------
 * Private Function Declarations
 *-----------------------------------------------------------------------*/

/*-------------------------------------------------------------------------
 * Function Definitions
 *-----------------------------------------------------------------------*/

/**
   @brief Read flash registers.

   @param[in]  reg_opcode  operation code.
   @param[in]  len        The length of register value to be read.
   @param[out] reg_value   The read out value.

   @return FLASH_DEVICE_DONE on success or an error code on failure.
*/
static FLASH_STATUS drv_flash_read_reg_internal(uint8_t reg_opcode, uint8_t len, uint8_t *reg_value)
{
    qspi_cmd_t qspi_read_reg;

    if (len > 0 && reg_value == NULL) {
        return FLASH_DEVICE_INVALID_PARAMETER;
    }
#ifdef FLASH_XIP_SUPPORT
    if (QSPI_TRANS_MODE == QSPI_PIO_MODE_E)
        drv_qspi_disable_xip_mode();
#endif

    (void)drv_qspi_prepare_cmd(&qspi_read_reg, reg_opcode, 0, 0, QSPI_SDR_1BIT_E, QSPI_SDR_1BIT_E, QSPI_SDR_1BIT_E,
                               false);

    if (drv_qspi_run_cmd(&qspi_read_reg, 0, reg_value, len, QSPI_TRANS_MODE)) {
#ifdef FLASH_XIP_SUPPORT
        if (QSPI_TRANS_MODE == QSPI_PIO_MODE_E)
            drv_qspi_restore_xip_mode();
#endif
        return FLASH_DEVICE_DONE;
    } else {
#ifdef FLASH_XIP_SUPPORT
        if (QSPI_TRANS_MODE == QSPI_PIO_MODE_E)
            drv_qspi_restore_xip_mode();
#endif

        return FLASH_DEVICE_FAIL;
    }
}

/**
   @brief Check flash operation error

   @param[in] operation_type  Flash operation type, WRITE_OPERATION,
                             ERASE_OPERATION, or OTHER_OPERATION.

   @return FLASH_DEVICE_DONE on success operation or an error code on failure.
*/
static FLASH_STATUS drv_flash_check_error(uint8_t operation_type)
{
    uint8_t status_mask = 0;
    uint8_t err_status_register = 0;
    uint8_t qspi_status = 0;
    FLASH_STATUS status = FLASH_DEVICE_DONE;

    switch (operation_type) {
        case WRITE_OPERATION:
            status_mask = flash_context.config->write_err_bmsk;
            err_status_register = flash_context.config->write_err_status_reg;
            break;

        case ERASE_OPERATION:
            status_mask = flash_context.config->erase_err_bmsk;
            err_status_register = flash_context.config->erase_err_status_reg;
            break;

        default:
            break;
    }

    if (err_status_register == 0) {
        return status;
    }

    status = drv_flash_read_reg_internal(err_status_register, 1, &qspi_status);
    if ((status == FLASH_DEVICE_DONE) && (qspi_status & status_mask)) {
        status = FLASH_DEVICE_FAIL;
    }

    return status;
}

#ifdef CONFIG_NON_OS
/**
 * @brief delay function in usec
 * @param time: Delay time in usec
 * @return: void
 */
void nop_us_delay(uint32_t time)
{
    volatile uint32_t idx;
    volatile uint32_t sum = time * 4;
    for (idx = 0; idx < sum; idx++) {
        __asm volatile("nop");
    }
}
#endif
/**
   @brief Check flash operation status.

   @param[in] timeout            The total time can be used for status check.
   @param[in] status_polling_usec  The status polling period.
   @param[in] operation          Flash operation, WRITE_OPERATION, or
                                 ERASE_OPERATION, or OTHER_OPERATION.
   @param[in] bmask              The bits of the status register to be checked.
   @param[in] status_value        The expected value for the checked status bits.

   @return FLASH_DEVICE_DONE on success or an error code on failure.
*/
static FLASH_STATUS drv_flash_wait_operation_done(uint32_t timeout, uint32_t status_polling_usec, uint8_t operation,
                                                  uint8_t bmask, uint8_t status_value)
{
    FLASH_STATUS status = FLASH_DEVICE_FAIL;
    uint8_t result = 0;

    while (timeout) {
        (void)drv_flash_read_reg_internal(READ_STATUS_CMD, 1, &result);

        if ((result & bmask) == status_value) {
            status = drv_flash_check_error(operation);
            break;
        }
#ifdef CONFIG_NON_OS
        nop_us_delay(status_polling_usec);
#else
        hres_timer_us_delay(status_polling_usec);
#endif
        timeout -= status_polling_usec;
    }
    return status;
}

/**
   @brief Flash write enable.

   @return FLASH_DEVICE_DONE on success or an error code on failure.
*/
static FLASH_STATUS drv_flash_write_enable()
{
    FLASH_STATUS status;
    qspi_cmd_t qspi_write_enable;

    (void)drv_qspi_prepare_cmd(&qspi_write_enable, WRITE_ENABLE_CMD, 0, 0, QSPI_SDR_1BIT_E, QSPI_SDR_1BIT_E,
                               QSPI_SDR_1BIT_E, false);

    drv_qspi_run_cmd(&qspi_write_enable, 0, NULL, 0, QSPI_TRANS_MODE);

    status = drv_flash_wait_operation_done(READ_STATUS_TIMEOUT, READ_STATUS_POLLING_USEC, OTHER_OPERATION,
                                           STATUS_WR_EN_MASK, STATUS_WR_EN_MASK);
    return status;
}

/**
   @brief Write flash registers.

   @param[in]  reg_opcode  operation code.
   @param[in]  len        The length of register value to be written.
   @param[out] reg_value   The written value.

   @return FLASH_DEVICE_DONE on success or an error code on failure.
*/
static FLASH_STATUS drv_flash_write_reg_internal(uint8_t reg_opcode, uint8_t len, uint8_t *reg_value)
{
    FLASH_STATUS status;
    qspi_cmd_t qspi_write_reg;

    if (len > 0 && reg_value == NULL) {
        return FLASH_DEVICE_INVALID_PARAMETER;
    }

    (void)drv_qspi_prepare_cmd(&qspi_write_reg, reg_opcode, 0, 0, QSPI_SDR_1BIT_E, QSPI_SDR_1BIT_E, QSPI_SDR_1BIT_E,
                               true);

    status = drv_flash_write_enable();
    if (status != FLASH_DEVICE_DONE) {
        return status;
    }

    if (drv_qspi_run_cmd(&qspi_write_reg, 0, reg_value, len, QSPI_TRANS_MODE)) {
        return FLASH_DEVICE_DONE;
    } else {
        return FLASH_DEVICE_FAIL;
    }
}

#ifdef FLASH_POWER_UP_SUPPORT
/**
   @brief Flash power up.

   Some flash support "Deep power down" mode, this function release flash from
   power down if it has entered into such mode.
*/
#define POWERUP_OPCODE   0xAB
#define DEEPSLEEP_OPCODE 0xB9

#define POWERUP_DELAY_100US 0x8
static void drv_flash_power_up()
{
    qspi_cmd_t qspi_power_up;

    (void)drv_qspi_prepare_cmd(&qspi_power_up, POWERUP_OPCODE, 0, 0, QSPI_SDR_1BIT_E, QSPI_SDR_1BIT_E, QSPI_SDR_1BIT_E,
                               false);

    drv_qspi_run_cmd(&qspi_power_up, 0, NULL, 0, QSPI_TRANS_MODE);
}

static void drv_flash_deep_sleep()
{
    qspi_cmd_t qspi_power_sleep;

    (void)drv_qspi_prepare_cmd(&qspi_power_sleep, DEEPSLEEP_OPCODE, 0, 0, QSPI_SDR_1BIT_E, QSPI_SDR_1BIT_E,
                               QSPI_SDR_1BIT_E, false);

    drv_qspi_run_cmd(&qspi_power_sleep, 0, NULL, 0, QSPI_TRANS_MODE);
}

#endif

#ifdef FLASH_HP_MODE_SUPPORT
/**
   @brief Set flash high performance mode.

   @return Returns FLASH_DEVICE_DONE on success or an error code on failure.
*/
static FLASH_STATUS drv_flash_set_high_performance()
{
    FLASH_STATUS status;
    uint8_t temp[3];

    memset(&temp[0], 0, sizeof(temp));
    drv_flash_read_reg_internal(READ_STATUS_CMD, 1, &temp[0]);
    drv_flash_read_reg_internal(READ_CFG_REG_CMD, 2, &temp[1]);

    temp[2] |= flash_context.config->high_performance_mode_bmask;

    status = drv_flash_write_reg_internal(WRITE_STATUS_CMD, 3, &temp[0]);
    if (status != FLASH_DEVICE_DONE) {
        return status;
    }

    status = drv_flash_wait_operation_done(READ_STATUS_TIMEOUT, READ_STATUS_POLLING_USEC, OTHER_OPERATION,
                                           READ_STATUS_BUSY_MASK, 0);

    return status;
}
#else
/**
   @brief Set flash high performance mode.

   @return Returns FLASH_DEVICE_DONE on success or an error code on failure.
*/
static FLASH_STATUS drv_flash_set_high_performance()
{
    return FLASH_DEVICE_DONE;
}
#endif

#if CONFIG_BOARD_QCC730_QSPI_V2_QUAD_MODE
/**
   @brief Quad enable mode 1, 4, 5.

   Mode 1: QE is bit 1 of status register 2. It is set via Write status  with
   two data bytes where bit 1 of the second byte is one. It is cleared via
   Write status with two data bytes where bit 1 of the second byte is zero.
   Writing only one byte to the status register has the side-effect of clearing
   status register 2, including the QE bit. The 100b code is used if writing
   one byte to the status register does not modify status register 2.
   Mode 4: QE is bit 1 of status register 2. It is set via Write status  with
   two data byte where bit 1 of the second byte is one. It is cleared via
   Write status  with two data bytes where bit 1 of the second byte is zeor.
   In constrast to the ENABLE_QUAD_MODE_1, writing one byte to the status
   register does not modify status register 2.
   Mode 5: QE is bit 1 of the status register 2. status  register 1 is read
   using instruction 05h. status  register 2 is read using instruction 35h.
   QE is set via Write status  instruction 01h with two data bytes where bit 1
   of the second byte is one. It is cleared via Write status  with two data
   bytes where bit 1 of the second byte is zero.

   @return FLASH_DEVICE_DONE on success or an error code on failure.
*/
static FLASH_STATUS drv_flash_enable_quad_mode145()
{
    FLASH_STATUS status = FLASH_DEVICE_DONE;
    uint8_t status_reg1 = 0;
    uint8_t status_reg2 = 0;
    uint8_t temp[2];

    status = drv_flash_read_reg_internal(READ_STATUS_CMD, 1, &status_reg1);
    if (status != FLASH_DEVICE_DONE) {
        return status;
    }

    status = drv_flash_read_reg_internal(READ_CFG1_CMD, 1, &status_reg2);
    if (status != FLASH_DEVICE_DONE) {
        return status;
    }

    if (status_reg2 & BIT_1) {
        return status;
    }

    status_reg2 |= BIT_1;
    temp[0] = status_reg1;
    temp[1] = status_reg2;

    status = drv_flash_write_reg_internal(WRITE_STATUS_CMD, 2, &temp[0]);
    if (status != FLASH_DEVICE_DONE) {
        return status;
    }

    status = drv_flash_wait_operation_done(READ_STATUS_TIMEOUT, READ_STATUS_POLLING_USEC, OTHER_OPERATION,
                                           READ_STATUS_BUSY_MASK, 0);

    return status;
}

/**
   @brief Quad enable mode 2.

   QE is bit 6 of status register 1. It is set via Write status  with one data
   byte where bit 6 is one. It is cleared via Write status  with one data byte
   where bit 6 is zero.

   @return FLASH_DEVICE_DONE on success or an error code on failure.
*/
static FLASH_STATUS drv_flash_enable_quad_mode2()
{
    FLASH_STATUS status = FLASH_DEVICE_DONE;
    uint8_t status_reg = 0;

    status = drv_flash_read_reg_internal(READ_STATUS_CMD, 1, &status_reg);
    if (status != FLASH_DEVICE_DONE) {
        return status;
    }

    /* In mode 2, Bit 6 of status register is used to enable Quad mode */
    if (status_reg & BIT_6) {
        return status;
    }

    status_reg |= BIT_6;
    status = drv_flash_write_reg_internal(WRITE_STATUS_CMD, 1, &status_reg);
    if (status != FLASH_DEVICE_DONE) {
        return status;
    }

    status = drv_flash_wait_operation_done(READ_STATUS_TIMEOUT, READ_STATUS_POLLING_USEC, OTHER_OPERATION,
                                           READ_STATUS_BUSY_MASK, 0);

    return status;
}

/**
   @brief Quad enable mode 3.

   QE is bit 7 of status register 2. It is set via Write status register 2
   instruction 3Eh with one data byte where bit 7 is one. It is cleared via
   Write status register 2 instruction 3Eh with one data byte where bit 7 is
   zero. The status register 2 is read using instruction 3Fh.

   @return FLASH_DEVICE_DONE on success or an error code on failure.
*/
static FLASH_STATUS drv_flash_enable_quad_mode3()
{
    FLASH_STATUS status = FLASH_DEVICE_DONE;
    uint8_t status2_reg = 0;

    /* Read 1 byte status  2 register with instruction 3Fh */
    status = drv_flash_read_reg_internal(READ_STATUS_2_CMD, 1, &status2_reg);
    if (status != FLASH_DEVICE_DONE) {
        return status;
    }

    /* In mode 3, Bit 7 of status 2 register is used to enable Quad mode. */
    if (status2_reg & BIT_7) {
        return status;
    }

    status2_reg |= BIT_7;
    status = drv_flash_write_reg_internal(WRITE_STATUS_2_CMD, 1, &status2_reg);
    if (status != FLASH_DEVICE_DONE) {
        return status;
    }

    status = drv_flash_wait_operation_done(READ_STATUS_TIMEOUT, READ_STATUS_POLLING_USEC, OTHER_OPERATION,
                                           READ_STATUS_BUSY_MASK, 0);

    return status;
}

/**
   @brief Enable flash quad mode.

   @param[in] quad_mode  The quad enable mode for flash, as defined in the
                        JEDEC Standard No. 216A Document, Quad Enable
                        Requirements in 15th word.

   @return FLASH_DEVICE_DONE on success or an error code on failure.
*/
static FLASH_STATUS drv_flash_enable_quad_mode(uint8_t quad_mode)
{
    FLASH_STATUS status = FLASH_DEVICE_DONE;

    switch (quad_mode) {
        case ENABLE_QUAD_MODE_0:
            /* For mode 0, Device does not have a QE bit. Device detects
               1-1-4 and 1-4-4 reads based on instruction. */
            break;

        case ENABLE_QUAD_MODE_1:
        case ENABLE_QUAD_MODE_4:
        case ENABLE_QUAD_MODE_5:
            status = drv_flash_enable_quad_mode145();
            break;

        case ENABLE_QUAD_MODE_2:
            status = drv_flash_enable_quad_mode2();
            break;

        case ENABLE_QUAD_MODE_3:
            status = drv_flash_enable_quad_mode3();
            break;

        default:
            status = FLASH_DEVICE_FAIL;
            break;
    }

    return status;
}
#endif
/**
   @brief Clear Specific bits of the flash registers.

   @param[in] reg_read_opcode   Register read operation code.
   @param[in] reg_write_opcode  Register write operation code.
   @param[in] bits_mask        The bits mask of the register that need to be cleared.

   @return FLASH_DEVICE_DONE on success or an error code on failure.
*/
static FLASH_STATUS drv_flash_clear_spi_bits(uint8_t reg_read_opcode, uint8_t reg_write_opcode, uint8_t bits_mask)
{
    uint8_t read_ret = 0;
    uint8_t read_back = 0;
    FLASH_STATUS status = FLASH_DEVICE_DONE;

    if (!bits_mask) {
        return status; /* Not set bits_mask, skip */
    }

    /* Get value and check */
    status = drv_flash_read_reg_internal(reg_read_opcode, 1, &read_ret);
    if ((!(read_ret & bits_mask)) || status != FLASH_DEVICE_DONE) {
        return status; /* Bits are cleared already, skip */
    }

    /* Clear bits_mask of value */
    read_ret &= ~bits_mask;
    status = drv_flash_write_reg_internal(reg_write_opcode, 1, &read_ret);
    if (status != FLASH_DEVICE_DONE) {
        return status;
    }

    /* Delay till complete */
    status = drv_flash_wait_operation_done(READ_STATUS_TIMEOUT, READ_STATUS_POLLING_USEC, OTHER_OPERATION,
                                           READ_STATUS_BUSY_MASK, 0);
    if (status != FLASH_DEVICE_DONE) {
        return status;
    }

    /* Verify the result */
    status = drv_flash_read_reg_internal(reg_read_opcode, 1, &read_back);
    if ((read_back & bits_mask) || (status != FLASH_DEVICE_DONE)) {
        return FLASH_DEVICE_FAIL; /* Bits are not cleared yet, return fail */
    }

    return status;
}

/**
   @brief Clear a list of Specific bits of the flash registers.

   @param[in] reg_read_opcode   Register read operation code list.
   @param[in] reg_write_opcode  Fegister write operation code list.
   @param[in] bits_mask        The list of register bits that need to be cleared.
   @param[in] cnt             The count of the list.

   @return FLASH_DEVICE_DONE on success or an error code on failure.
*/
static FLASH_STATUS drv_flash_clear_spi_bits_list(const uint8_t *reg_read_opcodes, const uint8_t *reg_write_opcode,
                                                  const uint8_t *bits_masks, uint8_t cnt)
{
    uint8_t i = 0;
    FLASH_STATUS status = FLASH_DEVICE_DONE;

    for (i = 0; i < cnt; i++) {
        status = drv_flash_clear_spi_bits(reg_read_opcodes[i], reg_write_opcode[i], bits_masks[i]);
        if (status != FLASH_DEVICE_DONE) {
            return status;
        }
    }

    return status;
}

/**
   @brief Clear flash write protection, so that flash can be written and erased.

   For different flash types, the registers and protect bits may be different.
   They should be handled differently.

   @return FLASH_DEVICE_DONE on success or an error code on failure.
*/
static FLASH_STATUS drv_flash_clear_write_protection()
{
    uint8_t flash_vid = FLASH_PID2VID(flash_context.config->device_id);
    uint32_t wp_mask = flash_context.config->write_protect_bmask;
    FLASH_STATUS status = FLASH_DEVICE_DONE;

    if (MANUFACTURER_ID_WINBOND == flash_vid || MANUFACTURER_ID_GD == flash_vid || MANUFACTURER_ID_GT == flash_vid) {
        /* status  Register Format is as described below for Winbond Flash part.
           Winbond or GD
           status -1 Register Format:
           =======================
           bit7     bit6    bit5      bit4    bit3    bit2  bit1    bit0
           SRP      SEC      TB       BP2     BP1     BP0   WEL     BUSY
           (status  (Sector  (TOP/    (Block protect Bits)  (Write  (Erase/write
           register protect) bottom                         enable  in progress)
           protect)          protect)                       latch)

           status -2 Register Format:
           =======================
           bit15    bit14   bit13     bit12   bit11   bit10 bit9    bit8
           SUS      CMP     LB3       LB2     LB1     (R)   QE      SRL
                    (Coplement                                      (status  register)
                    protect)                                        lock)

           Expect WriteProtectBmask has {0xFC, 0x41, 0x04} for W25Q32JVZPIQ */
        uint8_t wb_wp_bits[] = {(uint8_t)(wp_mask), (uint8_t)(wp_mask >> 8), (uint8_t)(wp_mask >> 16)};
        uint8_t wb_read_cmd_list[] = {READ_STATUS_CMD, WINBOND_READ_STATUS_2_CMD, WINBOND_READ_STATUS_3_CMD};
        uint8_t wb_write_cmd_list[] = {WRITE_STATUS_CMD, WINBOND_WRITE_STATUS_2_CMD, WINBOND_WRITE_STATUS_3_CMD};
        status = drv_flash_clear_spi_bits_list((const uint8_t *)wb_read_cmd_list, (const uint8_t *)wb_write_cmd_list,
                                               (const uint8_t *)wb_wp_bits, sizeof(wb_wp_bits));
    } else if (MANUFACTURER_ID_MACRONIX == flash_vid || MANUFACTURER_ID_ISSI == flash_vid) {
        /* WORKAROUND: On some Macronix parts, the block write protection,
           non-volatile bits of status register i.e. BP3-BP0 (bit5-bit2) are
           spuriously set thereby causing erase/write operation on Flash to fail.

           This is a temporary WORKAROUND to recover from this situation until
           the actual reason of how these bits are set is uncovered.
           TODO: Need to revisit once the actual problem is root caused.

           status  Register Format is as described below for Macronix Flash part.
           Macronix status  Register Format:
           =======================
           bit7     bit6    bit5    bit4    bit3    bit2    bit1    bit0
           SRWD     QE      BP3     BP2     BP1     BP0     WEL     WIP
           (status  (Quad   (---"---Level of                (Write  (Write
           register Enable)         Protected Block--"---)  enable  in
           write                                            latch)  progress)
           protect)

           1=status 1=Quad                                  1=write 1=write
           register Enable                                  enable  operation
           write    0=not                                   0=not   0=not in
           disable  Quad                                    write   write
                    Enable                                  enable  operation

           (--------"-------Non-volatile bits----"-------)  (-"-Volatile bits-"-)

           reserved_3 for Macronix parts is defaulted to 0xBC in OEM modifiable
           flash_config stored in AON region and considered as a mask to detect
           whether these bits are set. If set then auto-clear these bits.

           Expect reserved_3 has {0xBC, 0x00, 0x00} for MX25 */
        status = drv_flash_clear_spi_bits(READ_STATUS_CMD, WRITE_STATUS_CMD, (uint8_t)(wp_mask));
    }
    return status;
}

/**
   @brief Initialize the flash controller HW.
    Fermion just support one flash connection. SPI clocking mode is 0 only.

   @return FLASH_DEVICE_DONE on success or an error code on failure.
*/
static FLASH_STATUS drv_flash_controller_init()
{
    qspi_master_config_t qspi_cfg;

    memset(&qspi_cfg, 0, sizeof(qspi_master_config_t));

    /* Set clock frequency. */
    qspi_cfg.clk_freq = default_qspi_clock;

    if (drv_qspi_init(&qspi_cfg)) {
        return FLASH_DEVICE_DONE;
    }

    return FLASH_DEVICE_FAIL;
}

/**
   @brief Initialize flash context info.

   @param[in] config  Configuration data for the flash device.

   @return FLASH_DEVICE_DONE on success or an error code on failure.
*/
static FLASH_STATUS drv_flash_info_init(uint32_t device_id)
{
    FLASH_STATUS status = FLASH_DEVICE_DONE;
    uint32_t i;
    uint32_t total_flash_products = flash_get_config_entries_count();
    flash_config_data_t *pflash_device_config = flash_get_config_entries_struct();

    for (i = 0; i < total_flash_products; i++) {
        if (device_id == pflash_device_config[i].device_id) {
            NT_LOG_PRINT(SYSTEM, INFO, "found device %x\n", device_id);
            break;
        }
    }
    if (i == total_flash_products) {
        NT_LOG_PRINT(SYSTEM, ERR, "Didn't found any configured device:%x\n", device_id);
        return FLASH_DEVICE_NOT_FOUND;
    }

    memset(&flash_context, 0, sizeof(flash_context_t));
    flash_context.config = pflash_device_config + i;
    if (flash_context.config == NULL) {
        return FLASH_DEVICE_NOT_FOUND;
    }

    /* check the clock in the configure */
    if (flash_context.config->clk_freq != default_qspi_clock) {
        default_qspi_clock = flash_context.config->clk_freq;
        drv_flash_controller_init();
    }

#ifdef CONFIG_NON_OS
    static flash_operation_param_t flash_param;
    flash_context.operation_param = &flash_param;
#else
    flash_context.operation_param = nt_osal_allocate_memory(sizeof(flash_operation_param_t));

    if (flash_context.operation_param == NULL) {
        return FLASH_DEVICE_NO_MEMORY;
    }
#endif
    memset(flash_context.operation_param, 0, sizeof(flash_operation_param_t));
// TODO...
#ifdef FLASH_NONEBLOCKING
    /* Create operation task. */

#endif
    flash_context.timer_comparator = NO_TIMER_COMPARATOR;

    return status;
}

#if defined FEATURE_FPCI && !defined CONFIG_NON_OS
/**
 * @brief  Presleep/ Postawake Callbacks
 * @param  evt - Denotes the sleep event
 * @return None
 */
void flash_power_state_change_cb(uint8_t evt, void *p_args)
{
    (void)p_args;

    if (evt == PWR_EVT_WMAC_PRE_SLEEP) {
        drv_flash_deinit(0);
    }

    if ((evt == PWR_EVT_WMAC_POST_AWAKE) || (evt == PWR_EVT_WMAC_SLEEP_ABORT)) {
        drv_flash_init();
    }
}
#endif /* FEATURE_FPCI */

/**
   @brief Initialize the flash module.

   This function must be called before any other flash functions.

   @param[in] config  Configuration data for the flash device.

   @return
   FLASH_DEVICE_DONE -- On success, or an error code on failure.
*/
FLASH_STATUS drv_flash_init()
{
    FLASH_STATUS status = FLASH_DEVICE_DONE;
    uint32_t device_id = 0;

    if (flash_init_done) {
        return FLASH_DEVICE_DONE;
    }

#ifndef CONFIG_NON_OS
    qurt_mutex_create(&flash_mutex);
    qurt_mutex_lock(&flash_mutex);
#endif

    /* Initialize QSPI driver. */
    status = drv_flash_controller_init();
    if (status != FLASH_DEVICE_DONE) {
        goto FLASH_INIT_END;
    }

#ifdef FLASH_POWER_UP_SUPPORT
    /* power up flash */
    drv_flash_power_up();
#ifdef CONFIG_NON_OS
    nop_us_delay(100 * POWERUP_DELAY_100US);
#else
    hres_timer_us_delay(100 * POWERUP_DELAY_100US);
#endif
#endif
    /* Check device id. */
    status = drv_flash_read_reg_internal(READ_IDENTIFICATION_CMD, 3, (uint8_t *)&device_id);
    if (status != FLASH_DEVICE_DONE) {
        goto FLASH_INIT_END;
    }

    status = drv_flash_info_init(device_id);
    if (status != FLASH_DEVICE_DONE) {
        goto FLASH_INIT_END;
    }

    /* Clear write protection */
    status = drv_flash_clear_write_protection();
    if (status != FLASH_DEVICE_DONE) {
        goto FLASH_INIT_END;
    }

    /* high performance mode */
    if (flash_context.config->high_performance_mode_bmask) {
        status = drv_flash_set_high_performance();
        if (status != FLASH_DEVICE_DONE) {
            goto FLASH_INIT_END;
        }
    }

    /* Check rw mode. */
    if (!VALID_RW_MODE(flash_context.config->read_cmd_mode) || !VALID_RW_MODE(flash_context.config->read_addr_mode) ||
        !VALID_RW_MODE(flash_context.config->read_data_mode) || !VALID_RW_MODE(flash_context.config->write_cmd_mode) ||
        !VALID_RW_MODE(flash_context.config->write_addr_mode) ||
        !VALID_RW_MODE(flash_context.config->write_data_mode)) {
        status = FLASH_DEVICE_INVALID_PARAMETER;
        goto FLASH_INIT_END;
    }

#if CONFIG_BOARD_QCC730_QSPI_V2_QUAD_MODE
    /* Set quard mode */
    if (IS_QUAD_MODE(flash_context.config->read_cmd_mode) || IS_QUAD_MODE(flash_context.config->read_addr_mode) ||
        IS_QUAD_MODE(flash_context.config->read_data_mode) || IS_QUAD_MODE(flash_context.config->write_cmd_mode) ||
        IS_QUAD_MODE(flash_context.config->write_addr_mode) || IS_QUAD_MODE(flash_context.config->write_data_mode)) {
        status = drv_flash_enable_quad_mode(flash_context.config->quad_enable_mode);
        if (status != FLASH_DEVICE_DONE) {
            goto FLASH_INIT_END;
        }
    }
#endif
    /* Set address mode */
    if ((flash_context.config->addr_bytes == 4) &&
        (flash_context.config->density_in_blocks * BLOCK_SIZE_IN_BYTES > FLASH_16MB_IN_BYTES)) {
        status = drv_flash_write_reg_internal(ENTER_4B_ADDR_CMD, 0, NULL);
        if (status != FLASH_DEVICE_DONE) {
            goto FLASH_INIT_END;
        }
    }
#if defined FEATURE_FPCI && !defined CONFIG_NON_OS
    fpci_evt_cb_reg((ps_evt_cb_t)&flash_power_state_change_cb,
                    PWR_EVT_WMAC_PRE_SLEEP | PWR_EVT_WMAC_POST_AWAKE | PWR_EVT_WMAC_SLEEP_ABORT, 10, NULL);
#endif

    flash_init_done = true;

FLASH_INIT_END:
#ifndef CONFIG_NON_OS
    /* If there is an error, free the memory. */
    if (status != FLASH_DEVICE_DONE) {
        if (flash_context.operation_param != NULL) {
            nt_osal_free_memory(flash_context.operation_param);
        }
    }

    qurt_mutex_unlock(&flash_mutex);
#endif

    return status;
}

/**
   @brief deInitialize the flash and QSPI modules.

   @return
   FLASH_DEVICE_DONE -- On success, or an error code on failure.
*/
FLASH_STATUS drv_flash_deinit(uint32_t dereg)
{
    (void)dereg;
#ifdef CONFIG_NON_OS
    return FLASH_DEVICE_DONE;
#endif

    if (!flash_init_done)
        return FLASH_DEVICE_DONE;

#ifdef FLASH_POWER_UP_SUPPORT
    /*
      Send deep sleep commands to flash, put the device in the lowest consumption mode (the
      Deep Power-Down Mode).
    */
    drv_flash_deep_sleep();
#endif

    if (flash_context.operation_param) {
        nt_osal_free_memory(flash_context.operation_param);
        flash_context.operation_param = NULL;
    }

#ifndef CONFIG_NON_OS
    qurt_mutex_delete(&flash_mutex);
#endif

#if defined FEATURE_FPCI && !defined CONFIG_NON_OS
    if (dereg)
        fpci_evt_cb_dereg((ps_evt_cb_t)&flash_power_state_change_cb,
                          PWR_EVT_WMAC_PRE_SLEEP | PWR_EVT_WMAC_POST_AWAKE | PWR_EVT_WMAC_SLEEP_ABORT);
#endif

    flash_init_done = 0;

    /* Disalbe QSPI */
    drv_qspi_deinit();

    return FLASH_DEVICE_DONE;
}

/**
   @brief Read data from the flash.

   This function returns when the read is done, if it is blocking. If it is non-blocking,
   it returns with #FLASH_DEVICE_PENDING, indicating that the read is
   performed in the background.

   A callback function indicates when the non-blocking operation
   is complete. Because read is ongoing after this function returns,
   the input buffer for storing read data should not be freed. All
   other operations that must run after flash read and should not be performed
   before the callback function is called.

   @param[in]  address    The flash address to start to read from.
   @param[in]  byte_cnt    Number of bytes to read.
   @param[out] buffer     Data buffer for a flash read operation; it should
                          be in heap, or a static buffer for a non-blocking read.
   @param[in]  read_cb     Flash read callback function for a non-blocking
                          read. After a read is done, the read_cb is
                          called to inform about the completion; it should
                          be NULL for a blocking read.
   @param[in]  user_param  The user specified parameter for the callback function.

   @return
   FLASH_DEVICE_DONE -- If a blocking read completed successfully, or a negative value if there was an error.
   FLASH_DEVICE_PENDING -- Indicating a non-blocking read is ongoing.
*/
FLASH_STATUS drv_flash_read(uint32_t address, uint32_t byte_cnt, uint8_t *buffer, flash_operation_cb_t read_cb,
                            void *user_param)
{
    FLASH_STATUS status = FLASH_DEVICE_DONE;
    qspi_cmd_t qspi_read_cmd;
    (void)user_param;
    if (!flash_init_done) {
        return FLASH_DEVICE_FAIL;
    }

    if (flash_context.state != 0) {
        return FLASH_DEVICE_BUSY;
    }

#ifndef CONFIG_NON_OS
    qurt_mutex_lock(&flash_mutex);
#endif

#ifdef FLASH_XIP_SUPPORT
    if (QSPI_TRANS_MODE == QSPI_PIO_MODE_E)
        drv_qspi_disable_xip_mode();
#endif
    flash_context.state = FLASH_STATE_READ;

    if (buffer == NULL || byte_cnt == 0 || ((address + byte_cnt) < address) ||
        ((address + byte_cnt) > flash_context.config->density_in_blocks * BLOCK_SIZE_IN_BYTES)) {
        status = FLASH_DEVICE_INVALID_PARAMETER;
        goto FLASH_READ_END;
    }

    (void)drv_qspi_prepare_cmd(&qspi_read_cmd, flash_context.config->read_opcode, flash_context.config->addr_bytes,
                               flash_context.config->read_wait_state, (qspi_mode_t)flash_context.config->read_cmd_mode,
                               (qspi_mode_t)flash_context.config->read_addr_mode,
                               (qspi_mode_t)flash_context.config->read_data_mode, false);

    if (read_cb == NULL) {
        if (!drv_qspi_run_cmd(&qspi_read_cmd, address, buffer, byte_cnt, QSPI_TRANS_MODE)) {
            status = FLASH_DEVICE_FAIL;
        }
    }
#ifdef FLASH_NONEBLOCKING
    else {
        // TODO
    }
#endif

FLASH_READ_END:
#ifdef FLASH_XIP_SUPPORT
    if (QSPI_TRANS_MODE == QSPI_PIO_MODE_E)
        drv_qspi_restore_xip_mode();
#endif

    flash_context.state = 0;

#ifndef CONFIG_NON_OS
    qurt_mutex_unlock(&flash_mutex);
#endif

    return status;
}

/**
   @brief Write data to the flash.

   This function returns when the write is done, if it is blocking. If it is non-blocking,
   it returns with #FLASH_DEVICE_PENDING, indicating that the write is
   performed in the background.

   A callback function indicates when the non-blocking operation
   is complete. Because write is ongoing after this function returns,
   the input buffer for storing write data should not be freed. All
   other operations that must run after flash write, should not be performed
   before the callback function is called.

   @param[in] address    The flash address to start to write to.
   @param[in] byte_cnt    Number of bytes to write.
   @param[in] buffer     Data buffer containing data to be written; it should
                         be in heap, or a static buffer for a non-blocking write.
   @param[in] write_cb    Flash write callback function for non-blocking
                         write. After write is done, the write_cb is
                         called to inform about the completion; it should
                         be NULL for a blocking write.
   @param[in] user_param  The user specified parameter for the callback function.

   @return
   FLASH_DEVICE_DONE -- If blocking write completed successfully, or a negative value if there was an error.
   FLASH_DEVICE_PENDING -- Indicating a non-blocking write is ongoing.
*/
FLASH_STATUS drv_flash_write(uint32_t address, uint32_t byte_cnt, uint8_t *buffer, flash_operation_cb_t write_cb,
                             void *user_param)
{
    FLASH_STATUS status = FLASH_DEVICE_DONE;
    qspi_cmd_t qspi_page_write_cmd;
    uint32_t transfer_size = 0;
    uint32_t limit = PAGE_SIZE_IN_BYTES;
    (void)user_param;

    if (!flash_init_done) {
        return FLASH_DEVICE_FAIL;
    }

    if (flash_context.state != 0) {
        return FLASH_DEVICE_BUSY;
    }

#ifndef CONFIG_NON_OS
    qurt_mutex_lock(&flash_mutex);
#endif

#ifdef FLASH_XIP_SUPPORT
    if (QSPI_TRANS_MODE == QSPI_PIO_MODE_E)
        drv_qspi_disable_xip_mode();
#endif

    flash_context.state = FLASH_STATE_WRITE;

    if (buffer == NULL || byte_cnt == 0 || ((address + byte_cnt) < address) ||
        ((address + byte_cnt) > flash_context.config->density_in_blocks * BLOCK_SIZE_IN_BYTES)) {
        status = FLASH_DEVICE_INVALID_PARAMETER;
        goto FLASH_WRITE_END;
    }

    (void)drv_qspi_prepare_cmd(&qspi_page_write_cmd, flash_context.config->write_opcode,
                               flash_context.config->addr_bytes, 0, (qspi_mode_t)flash_context.config->write_cmd_mode,
                               (qspi_mode_t)flash_context.config->write_addr_mode,
                               (qspi_mode_t)flash_context.config->write_data_mode, true);

#ifdef FLASH_NONEBLOCKING
    if (write_cb != NULL) {
        // TODO
    }
#endif
#ifdef FLASH_XIP_SUPPORT
    /* Signle the HW write operation is ongoing. Needed for XIP. */
    if (flash_context.config->suspend_program_opcode > 0 && flash_context.config->resume_program_opcode > 0) {
        (void)drv_qspi_xip_set_pe_state(true);
        (void)drv_qspi_xip_config_suspend_resume(
            flash_context.config->suspend_program_delay_in_us, flash_context.config->suspend_program_opcode,
            flash_context.config->resume_program_delay_in_us, flash_context.config->resume_program_opcode);
    }
#endif

    while (byte_cnt) {
        if (address % PAGE_SIZE_IN_BYTES) {
            transfer_size = PAGE_SIZE_IN_BYTES - (address % PAGE_SIZE_IN_BYTES);
            if (transfer_size > byte_cnt) {
                transfer_size = byte_cnt;
            }
        } else {
            transfer_size = (byte_cnt > limit) ? (limit) : (byte_cnt);
        }

        status = drv_flash_write_enable();
        if (status != FLASH_DEVICE_DONE) {
            goto FLASH_WRITE_END;
        }

        if (write_cb == NULL) {
            drv_qspi_run_cmd(&qspi_page_write_cmd, address, buffer, transfer_size, QSPI_TRANS_MODE);
        }
#ifdef FLASH_NONEBLOCKING
        else {
            // TODO
        }
#endif
        status = drv_flash_wait_operation_done(WRITE_TIMEOUT, WRITE_STATUS_POLLING_USEC, WRITE_OPERATION,
                                               PROG_ERASE_WRITE_BUSY_BMSK, 0);
        if (status != FLASH_DEVICE_DONE) {
            goto FLASH_WRITE_END;
        }

        address += transfer_size;
        buffer += transfer_size;
        byte_cnt -= transfer_size;
    }

FLASH_WRITE_END:
#ifdef FLASH_XIP_SUPPORT
    (void)drv_qspi_xip_set_pe_state(false);
    if (QSPI_TRANS_MODE == QSPI_PIO_MODE_E)
        drv_qspi_restore_xip_mode();
#endif
    flash_context.state = 0;

#ifndef CONFIG_NON_OS
    qurt_mutex_unlock(&flash_mutex);
#endif

    return status;
}

/**
   @brief Erase the given flash blocks or bulks, or the whole chip.

   This function returns when the erase is done, if it is blocking. If it is non-blocking, it
   returns with #FLASH_DEVICE_PENDING, indicating that the erase is
   performed in the background.

   A callback function indicates when the non-blocking operation
   is complete. Because erase is ongoing after this function returns,
   other operations that must run after flash erase, should not be performed
   before the callback function is called.

   @param[in] erase_type  Specify the erase type.
   @param[in] start      For block erase - the starting block of a
                         number of blocks to erase.
                         For bulk erase - the starting bulk of a number
                         of bulks to erase.
                         For chip erase, it should be 0.
   @param[in] cnt        For block erase - the number of blocks to erase.
                         For bulk erase - the number of bulks to erase.
                         For chip erase, it should be 1.
   @param[in] erase_cb    Flash erase callback function for non-blocking
                         erase. After erase is done, the erase_cb is
                         called to inform about the completion; it should be
                         NULL for a blocking erase.
   @param[in] user_param  The user specified parameter for the callback function.

   @return
   FLASH_DEVICE_DONE -- If blocking erase completed successfully, or a negative value if there was an error.
   FLASH_DEVICE_PENDING -- Indicating a non-blocking erase is ongoing.
*/
FLASH_STATUS drv_flash_erase(flash_erase_type_t erase_type, uint32_t start, uint32_t cnt, flash_operation_cb_t erase_cb,
                             void *user_param)
{
    FLASH_STATUS status = FLASH_DEVICE_DONE;
    qspi_cmd_t qspi_erase_cmd;
    uint32_t address;
    uint32_t size = 0;
    uint32_t erase_timeout = ERASE_TIMEOUT;
    uint32_t erase_polling = ERASE_STATUS_POLLING_MSEC;
    uint8_t opcode;
    uint8_t addr_bytes = flash_context.config->addr_bytes;
    (void)user_param;
    (void)erase_cb;

    if (!flash_init_done) {
        return FLASH_DEVICE_FAIL;
    }

    if (flash_context.state != 0) {
        return FLASH_DEVICE_BUSY;
    }

#ifndef CONFIG_NON_OS
    qurt_mutex_lock(&flash_mutex);
#endif

#ifdef FLASH_XIP_SUPPORT
    if (QSPI_TRANS_MODE == QSPI_PIO_MODE_E)
        drv_qspi_disable_xip_mode();
#endif

    flash_context.state = FLASH_STATE_ERASE;

    if (erase_type == FLASH_BLOCK_ERASE_E) {
        if (flash_context.config->erase_4kb_opcode == 0) {
            status = FLASH_DEVICE_INVALID_PARAMETER;
            goto FLASH_ERASE_END;
        }
        size = BLOCK_SIZE_IN_BYTES;
        opcode = flash_context.config->erase_4kb_opcode;
    } else if (erase_type == FLASH_BULK_ERASE_E) {
        if (flash_context.config->bulk_erase_size_4kb == 0 || flash_context.config->bulk_erase_opcode == 0) {
            status = FLASH_DEVICE_INVALID_PARAMETER;
            goto FLASH_ERASE_END;
        }
        size = flash_context.config->bulk_erase_size_4kb * BLOCK_SIZE_IN_BYTES;
        opcode = flash_context.config->bulk_erase_opcode;
    } else if (erase_type == FLASH_CHIP_ERASE_E) {
        if (flash_context.config->chip_erase_opcode == 0) {
            status = FLASH_DEVICE_INVALID_PARAMETER;
            goto FLASH_ERASE_END;
        }
        size = flash_context.config->density_in_blocks * BLOCK_SIZE_IN_BYTES;
        opcode = flash_context.config->chip_erase_opcode;
        addr_bytes = 0;
        erase_timeout = CHIP_ERASE_TIMEOUT;
        erase_polling = CHIP_ERASE_STATUS_POLLING_MSEC;
    } else {
        status = FLASH_DEVICE_INVALID_PARAMETER;
        goto FLASH_ERASE_END;
    }

    address = start * size;
    if (cnt == 0 || ((start + cnt) * size < address) ||
        ((start + cnt) * size > flash_context.config->density_in_blocks * BLOCK_SIZE_IN_BYTES)) {
        status = FLASH_DEVICE_INVALID_PARAMETER;
        goto FLASH_ERASE_END;
    }

    (void)drv_qspi_prepare_cmd(&qspi_erase_cmd, opcode, addr_bytes, 0, QSPI_SDR_1BIT_E, QSPI_SDR_1BIT_E,
                               QSPI_SDR_1BIT_E, false);
#ifdef FLASH_XIP_SUPPORT
    /* Signle the HW write operation is ongoing. Needed for XIP. */
    if (flash_context.config->suspend_erase_opcode > 0 && flash_context.config->resume_erase_opcode > 0) {
        (void)drv_qspi_xip_set_pe_state(true);
        (void)drv_qspi_xip_config_suspend_resume(
            flash_context.config->suspend_erase_delay_in_us, flash_context.config->suspend_erase_opcode,
            flash_context.config->resume_erase_delay_in_us, flash_context.config->resume_erase_opcode);
    }
#endif

    while (cnt) {
        status = drv_flash_write_enable();
        if (status != FLASH_DEVICE_DONE) {
            goto FLASH_ERASE_END;
        }

        drv_qspi_run_cmd(&qspi_erase_cmd, address, NULL, 0, QSPI_TRANS_MODE);
#ifdef FLASH_NONEBLOCKING
        if (erase_cb != NULL) {
            // TODO
        }
#endif
        status = drv_flash_wait_operation_done(erase_timeout, erase_polling * 1000, ERASE_OPERATION,
                                               PROG_ERASE_WRITE_BUSY_BMSK, 0);
        if (status != FLASH_DEVICE_DONE) {
            goto FLASH_ERASE_END;
        }

        address += size;
        cnt--;
    }

FLASH_ERASE_END:
#ifdef FLASH_XIP_SUPPORT
    (void)drv_qspi_xip_set_pe_state(false);
    if (QSPI_TRANS_MODE == QSPI_PIO_MODE_E)
        drv_qspi_restore_xip_mode();
#endif

    flash_context.state = 0;

#ifndef CONFIG_NON_OS
    qurt_mutex_unlock(&flash_mutex);
#endif

    return status;
}

/**
   @brief Get current flash configuration.

   @return
   Pointer to current flash configuration.
*/
flash_config_data_t *drv_flash_get_config(void)
{
    if (!flash_init_done) {
        return NULL;
    }
    return flash_context.config;
}

#ifdef FLASH_ACCESS_REG_SUPPORT
/**
   @brief Read flash registers.

   @param[in]  reg_opcode  operation code.
   @param[in]  len        The length of register value to be read.
   @param[out] reg_value   The read out value.

   @return
   FLASH_DEVICE_DONE -- On success, or an error code on failure.
*/
FLASH_STATUS drv_flash_read_reg(uint8_t reg_opcode, uint8_t len, uint8_t *reg_value)
{
    FLASH_STATUS status;

    if (!flash_init_done) {
        return FLASH_DEVICE_FAIL;
    }

    if (flash_context.state != 0) {
        return FLASH_DEVICE_BUSY;
    }

#ifndef CONFIG_NON_OS
    qurt_mutex_lock(&flash_mutex);
#endif

#ifdef FLASH_XIP_SUPPORT
    if (QSPI_TRANS_MODE == QSPI_PIO_MODE_E)
        drv_qspi_disable_xip_mode();
#endif

    flash_context.state = FLASH_STATE_READ_REG;

    status = drv_flash_read_reg_internal(reg_opcode, len, reg_value);

    flash_context.state = 0;
#ifdef FLASH_XIP_SUPPORT
    if (QSPI_TRANS_MODE == QSPI_PIO_MODE_E)
        drv_qspi_restore_xip_mode();
#endif

#ifndef CONFIG_NON_OS
    qurt_mutex_unlock(&flash_mutex);
#endif

    return status;
}

/**
   @brief Write flash registers.

   This function returns when the writereg is done, if it is blocking. If it is non-blocking, it
   returns with #FLASH_DEVICE_PENDING, indicating that the writereg is
   performed in the background.

   A callback function indicates when the non-blocking operation
   is complete. Because writereg is ongoing after this function returns,
   other operations that must run after flash writereg, should not be performed
   before the callback function is called.

   @param[in] reg_opcode   operation code.
   @param[in] len         The length of register value to be written.
   @param[in] reg_value    The written value.
   @param[in] write_cb  Flash writereg callback function for a non-blocking
                          writereg. After writereg is done, the write_cb
                          is called to inform about the completion; it
                          should be NULL for a blocking operation.
   @param[in] user_param   The user specified parameter for the callback function.

   @return
   FLASH_DEVICE_DONE -- If blocking writereg completed successfully, or a negative value if there was an error.
   FLASH_DEVICE_PENDING -- Indicating non-blocking writereg is ongoing.
*/
FLASH_STATUS drv_flash_write_reg(uint8_t reg_opcode, uint8_t len, uint8_t *reg_value, flash_operation_cb_t write_cb,
                                 void *user_param)
{
    FLASH_STATUS status;
    (void)user_param;

    if (!flash_init_done) {
        return FLASH_DEVICE_FAIL;
    }

    if (flash_context.state != 0) {
        return FLASH_DEVICE_BUSY;
    }

#ifndef CONFIG_NON_OS
    qurt_mutex_lock(&flash_mutex);
#endif

#ifdef FLASH_XIP_SUPPORT
    if (QSPI_TRANS_MODE == QSPI_PIO_MODE_E)
        drv_qspi_disable_xip_mode();
#endif

    flash_context.state = FLASH_STATE_WRITE_REG;

    status = drv_flash_write_reg_internal(reg_opcode, len, reg_value);
    if (status != FLASH_DEVICE_DONE) {
        goto FLASH_WRITEREG_END;
    }

    if (write_cb == NULL) {
        status = drv_flash_wait_operation_done(READ_STATUS_TIMEOUT, READ_STATUS_POLLING_USEC, OTHER_OPERATION,
                                               READ_STATUS_BUSY_MASK, 0);
    }
#ifdef FLASH_NONEBLOCKING
    else {
        // TODO
    }
#endif
FLASH_WRITEREG_END:
    flash_context.state = 0;
#ifdef FLASH_XIP_SUPPORT
    if (QSPI_TRANS_MODE == QSPI_PIO_MODE_E)
        drv_qspi_restore_xip_mode();
#endif

#ifndef CONFIG_NON_OS
    qurt_mutex_unlock(&flash_mutex);
#endif

    return status;
}
#else
/**
   @brief Stub Function for Read flash registers.

*/
FLASH_STATUS drv_flash_read_reg(uint8_t reg_opcode, uint8_t len, uint8_t *reg_value)
{
    return FLASH_DEVICE_FAIL;
}

/**
   @brief Stub function for Write flash registers.
*/
FLASH_STATUS drv_flash_write_reg(uint8_t reg_opcode, uint8_t len, uint8_t *reg_value, flash_operation_cb_t write_cb,
                                 void *user_param)
{
    return FLASH_DEVICE_FAIL;
}
#endif

#endif  // FERMION_QSPIM_SUPPORT
