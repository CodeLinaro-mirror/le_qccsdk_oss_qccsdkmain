/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/** @file hal_qspi.h
   @brief qspi HW interface definition.

   This module provide qspi HW interface definitions.
*/

#ifndef __FERM_QSPI_HAL_H__
#define __FERM_QSPI_HAL_H__

/*-------------------------------------------------------------------------
 * Include Files
 *-----------------------------------------------------------------------*/

#include "qccx.h"

typedef __IOM QSPI_BASE_qspi_Type qspi_hal;

#define QSPI_R_QSPI_RD_FIFOn(n) *(uint32_t *)((uint32_t *)&hal->QSPI_R_QSPI_RD_FIFO0 + (n))

#define FERM_PMU_BOOT_STRAP_UNLOCK 0x63887466
static inline void hal_qspi_enable_qspi(uint8_t enable, uint8_t pads_option)
{
#if CONFIG_SOC_QCC730V1
    QCC730V1_PMU_BASE_Type *pmu = QCC730V1_PMU_BASE;
#elif CONFIG_SOC_QCC730V2
    QCC730V2_PMU_BASE_Type *pmu = QCC730V2_PMU_BASE;
#endif

    /* unlock the configure register */
    pmu->pmu.PMU_BOOT_STRAP_CONFIG_SECURE.reg = FERM_PMU_BOOT_STRAP_UNLOCK;
    if (enable) {
#if CONFIG_SOC_QCC730V1
        pmu->pmu.PMU_BOOT_STRAP_CONFIGURATION_STATUS.bit.CFG_QSPI_ENABLE0 = enable;
        pmu->pmu.PMU_BOOT_STRAP_CONFIG_SECURE.reg = FERM_PMU_BOOT_STRAP_UNLOCK;
        pmu->pmu.PMU_BOOT_STRAP_CONFIGURATION_STATUS.bit.CFG_QSPI_ENABLE1 = pads_option;
#elif CONFIG_SOC_QCC730V2
        pmu->pmu.PMU_BOOT_STRAP_CONFIGURATION_STATUS.bit.CFG_QSPI_ENABLE = enable;
        pmu->pmu.PMU_BOOT_STRAP_CONFIG_SECURE.reg = FERM_PMU_BOOT_STRAP_UNLOCK;
        pmu->pmu.PMU_BOOT_STRAP_CONFIGURATION_STATUS.bit.CFG_QSPI_QUAD = pads_option;
#endif
    } else {
#if CONFIG_SOC_QCC730V1
        pmu->pmu.PMU_BOOT_STRAP_CONFIGURATION_STATUS.bit.CFG_QSPI_ENABLE0 = enable;
#elif CONFIG_SOC_QCC730V2
        pmu->pmu.PMU_BOOT_STRAP_CONFIGURATION_STATUS.bit.CFG_QSPI_ENABLE = enable;
#endif
    }
}

static inline void hal_qspi_mcu_enable_qspi()
{
    uint8_t value;
#if CONFIG_SOC_QCC730V1
    QCC730V1_PMU_BASE_Type *pmu = QCC730V1_PMU_BASE;
#elif CONFIG_SOC_QCC730V2
    QCC730V2_PMU_BASE_Type *pmu = QCC730V2_PMU_BASE;
#endif

    /* Power up QSPI domain in NON_OS mode when MCU is in active mode. */
    value = pmu->pmu.PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ.bit.PD_QSPI_CNTL_BIT;
    if (!value) {
        pmu->pmu.PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ.bit.PD_QSPI_CNTL_BIT = 1;
    }
}

static inline void hal_qspi_enable_clock_gating(uint8_t enable)
{
#if CONFIG_SOC_QCC730V1
    QCC730V1_CCU_BASE_Type *ccu = QCC730V1_CCU_BASE;
#elif CONFIG_SOC_QCC730V2
    QCC730V2_CCU_BASE_Type *ccu = QCC730V2_CCU_BASE;
#endif

    if (enable) {
        /* enable clock gating, qspi stop work */
        ccu->ccu.CCU_R_CCU_DISABLE_CLK_GATING.bit.QSPI_HCLK_CLKGATE_DISABLE = 0;
        ccu->ccu.CCU_R_CCU_DISABLE_CLK_GATING.bit.QSPI_FB_CLKGATE_DISABLE = 0;
        ccu->ccu.CCU_R_CCU_DISABLE_CLK_GATING.bit.QSPI_REF_CLKGATE_DISABLE = 0;
    } else {
        /* disable clock gating */
        ccu->ccu.CCU_R_CCU_DISABLE_CLK_GATING.bit.QSPI_HCLK_CLKGATE_DISABLE = 1;
        ccu->ccu.CCU_R_CCU_DISABLE_CLK_GATING.bit.QSPI_FB_CLKGATE_DISABLE = 1;
        ccu->ccu.CCU_R_CCU_DISABLE_CLK_GATING.bit.QSPI_REF_CLKGATE_DISABLE = 1;
    }
}

static inline uint8_t hal_qspi_is_qspi_active()
{
#if CONFIG_SOC_QCC730V1
    QCC730V1_PMU_BASE_Type *pmu = QCC730V1_PMU_BASE;
#elif CONFIG_SOC_QCC730V2
    QCC730V2_PMU_BASE_Type *pmu = QCC730V2_PMU_BASE;
#endif
#if CONFIG_SOC_QCC730V1
    return pmu->pmu.PMU_BOOT_STRAP_CONFIGURATION_STATUS.bit.CFG_QSPI_ENABLE0;
#elif CONFIG_SOC_QCC730V2
    return pmu->pmu.PMU_BOOT_STRAP_CONFIGURATION_STATUS.bit.CFG_QSPI_ENABLE;
#endif
}

static inline void hal_qspi_set_clock(uint8_t clock)
{
#if CONFIG_SOC_QCC730V1
    QCC730V1_PMU_BASE_Type *pmu = QCC730V1_PMU_BASE;
#elif CONFIG_SOC_QCC730V2
    QCC730V2_PMU_BASE_Type *pmu = QCC730V2_PMU_BASE;
#endif

    pmu->pmu.PMU_COMMON_QSPI_REF_CLK_DIV_RCGR.reg = clock;
}

static inline void hal_qspi_qspi_gdscr_config()
{
    uint32_t value;
#if CONFIG_SOC_QCC730V1
    QCC730V1_PMU_BASE_Type *pmu = QCC730V1_PMU_BASE;
#elif CONFIG_SOC_QCC730V2
    QCC730V2_PMU_BASE_Type *pmu = QCC730V2_PMU_BASE;
#endif
    value = pmu->pmu.PMU_QSPI_GDSCR.reg;
    value |= (2 << QWLAN_PMU_QSPI_GDSCR_EN_REST_WAIT_OFFSET | 2 << QWLAN_PMU_QSPI_GDSCR_CLK_DIS_WAIT_OFFSET |
              2 << QWLAN_PMU_QSPI_GDSCR_EN_FEW_WAIT_OFFSET);

    pmu->pmu.PMU_QSPI_GDSCR.reg = value;
}

static inline uint32_t hal_qspi_gdscr_pwr_ready()
{
#if CONFIG_SOC_QCC730V1
    QCC730V1_PMU_BASE_Type *pmu = QCC730V1_PMU_BASE;
#elif CONFIG_SOC_QCC730V2
    QCC730V2_PMU_BASE_Type *pmu = QCC730V2_PMU_BASE;
#endif
    return pmu->pmu.PMU_QSPI_GDSCR.bit.GDS_CTL_PWR_STATUS;
}

/* set qspi master configure */
static inline void hal_qspi_set_master_config_sbl_en(qspi_hal *hal, uint8_t enable)
{
    hal->QSPI_R_QSPI_MSTR_CONFIG.bit.SBL_EN = enable;
}

static inline void hal_qspi_set_master_config_spi_mode(qspi_hal *hal, uint8_t mode)
{
    hal->QSPI_R_QSPI_MSTR_CONFIG.bit.SPI_MODE = mode;
}

static inline void hal_qspi_set_master_config_wpn(qspi_hal *hal, uint8_t enable)
{
    hal->QSPI_R_QSPI_MSTR_CONFIG.bit.PIN_WPN = enable;
}

static inline void hal_qspi_set_master_config_holdn(qspi_hal *hal, uint8_t enable)
{
    hal->QSPI_R_QSPI_MSTR_CONFIG.bit.PIN_HOLDN = enable;
}

static inline void hal_qspi_master_status_reset(qspi_hal *hal)
{
    hal->QSPI_R_QSPI_AHB_MASTER_CFG.reg = 0x00001A02;
    hal->QSPI_R_QSPI_MSTR_INT_ENABLE.reg = 0;
    hal->QSPI_R_QSPI_MSTR_INT_STATUS.reg = 0xFFFFFFFF;
    hal->QSPI_R_QSPI_RD_FIFO_CONFIG.reg = 0;
    hal->QSPI_R_QSPI_RD_FIFO_RESET.bit.RESET_FIFO = 1;
    hal->QSPI_R_QSPI_XIP_MASTER_CFG.reg = QWLAN_QSPI_R_QSPI_XIP_MASTER_CFG_DEFAULT;
}

static inline void hal_qspi_pmu_qspi_bcr_reset()
{
#if CONFIG_SOC_QCC730V1
    QCC730V1_PMU_BASE_Type *pmu = QCC730V1_PMU_BASE;
#elif CONFIG_SOC_QCC730V2
    QCC730V2_PMU_BASE_Type *pmu = QCC730V2_PMU_BASE;
#endif

    pmu->pmu.PMU_QSPI_BCR.reg = 1;
    pmu->pmu.PMU_QSPI_BCR.reg = 0;
}

static inline void hal_qspi_clear_isr_status(qspi_hal *hal)
{
    hal->QSPI_R_QSPI_MSTR_INT_STATUS.reg = 0xFFFFFFFF;
}

static inline void hal_qspi_set_pio_config(qspi_hal *hal, uint8_t write, uint8_t io_mode, uint8_t fragment)
{
    uint32_t reg = hal->QSPI_R_QSPI_PIO_TRANSFER_CONFIG.reg;
    reg &= ~(QWLAN_QSPI_R_QSPI_PIO_TRANSFER_CONFIG_MULTI_IO_MODE_MASK |
             QWLAN_QSPI_R_QSPI_PIO_TRANSFER_CONFIG_TRANSFER_FRAGMENT_MASK |
             QWLAN_QSPI_R_QSPI_PIO_TRANSFER_CONFIG_TRANSFER_DIRECTION_MASK);
    reg |= (write << QWLAN_QSPI_R_QSPI_PIO_TRANSFER_CONFIG_TRANSFER_DIRECTION_OFFSET |
            io_mode << QWLAN_QSPI_R_QSPI_PIO_TRANSFER_CONFIG_MULTI_IO_MODE_OFFSET |
            fragment << QWLAN_QSPI_R_QSPI_PIO_TRANSFER_CONFIG_TRANSFER_FRAGMENT_OFFSET);
    hal->QSPI_R_QSPI_PIO_TRANSFER_CONFIG.reg = reg;
}

static inline void hal_qspi_set_pio_transfer_control_request_count(qspi_hal *hal, uint16_t request_count)
{
    hal->QSPI_R_QSPI_PIO_TRANSFER_CONTROL.bit.REQUEST_COUNT = request_count;
}
uint32_t hal_qspi_check_pio_request_count();

static inline uint32_t hal_qspi_get_pio_transfer_wr_fifo_bytes(qspi_hal *hal)
{
    return hal->QSPI_R_QSPI_PIO_TRANSFER_STATUS.bit.WR_FIFO_BYTES;
}

static inline uint8_t hal_qspi_check_pio_transaction_done(qspi_hal *hal)
{
    return hal->QSPI_R_QSPI_MSTR_INT_STATUS.bit.TRANSACTION_DONE;
}

static inline void hal_qspi_pio_write_4bytes(qspi_hal *hal, uint32_t word_value)
{
    hal->QSPI_R_QSPI_PIO_DATAOUT_4BYTE.reg = word_value;
}

static inline void hal_qspi_pio_write_1bytes(qspi_hal *hal, uint8_t byte_value)
{
    hal->QSPI_R_QSPI_PIO_DATAOUT_1BYTE.bit.DATAIN = byte_value;
}

static inline uint8_t hal_qspi_get_pio_rd_fifo_wrcnts(qspi_hal *hal)
{
    return hal->QSPI_R_QSPI_RD_FIFO_STATUS.bit.WR_CNTS;
}

static inline uint32_t hal_qspi_get_pio_rd_fifo(qspi_hal *hal, uint32_t index)
{
    return (uint32_t)QSPI_R_QSPI_RD_FIFOn(index);
}

static inline uint8_t hal_qspi_xip_is_enabled(qspi_hal *hal)
{
    return hal->QSPI_R_QSPI_XIP_MASTER_CFG.bit.XIP_ENABLE;
}

static inline void hal_qspi_xip_enable(qspi_hal *hal, uint8_t enable)
{
    hal->QSPI_R_QSPI_XIP_MASTER_CFG.bit.XIP_ENABLE = enable;
}

static inline uint8_t hal_qspi_get_xip_is_active(qspi_hal *hal)
{
    return hal->QSPI_R_QSPI_XIP_MASTER_STATUS.bit.XIP_ACTIVE;
}

static inline void hal_qspi_xip_enable_program_erase_ongoing(qspi_hal *hal, uint8_t enable)
{
    hal->QSPI_R_QSPI_XIP_MASTER_CFG.bit.PROGRAM_ERASE_ONGOING = enable;
}

static inline void hal_qspi_xip_set_suspend_opcode(qspi_hal *hal, uint8_t opcode)
{
    hal->QSPI_R_QSPI_XIP_SUSPEND_PH_CONFIG.bit.SUSPEND_OP_CODE = opcode;
}

static inline void hal_qspi_xip_set_suspend_delay(qspi_hal *hal, uint16_t delay)
{
    hal->QSPI_R_QSPI_XIP_SUSPEND_PH_CONFIG.bit.T_SUSPEND_PROG_DELAY = delay;
}

static inline void hal_qspi_xip_enable_suspend(qspi_hal *hal, uint8_t enable)
{
    hal->QSPI_R_QSPI_XIP_SUSPEND_PH_CONFIG.bit.SUSPEND_ENABLE = enable;
}

static inline void hal_qspi_xip_set_resume_opcode(qspi_hal *hal, uint8_t opcode)
{
    hal->QSPI_R_QSPI_XIP_RESUME_PH_CONFIG.bit.RESUME_OP_CODE = opcode;
}

static inline void hal_qspi_xip_set_resume_delay(qspi_hal *hal, uint16_t delay)
{
    hal->QSPI_R_QSPI_XIP_RESUME_PH_CONFIG.bit.T_RESUME_PROG_DELAY = delay;
}

static inline void hal_qspi_xip_resume_suspend(qspi_hal *hal, uint8_t enable)
{
    hal->QSPI_R_QSPI_XIP_RESUME_PH_CONFIG.bit.RESUME_ENABLE = enable;
}

static inline void hal_qspi_xip_ph_config(qspi_hal *hal, uint32_t ph_inst, uint32_t ph_dummy)
{
    hal->QSPI_R_QSPI_XIP_INST_PH_CONFIG.bit.LINEAR_BURST_INST_OPCODE = ph_inst;
    hal->QSPI_R_QSPI_XIP_DUMMY_PH_CONFIG.reg = ph_dummy;
}

static inline void hal_qspi_xip_region_config(qspi_hal *hal, uint32_t region_id, uint32_t region_size,
                                              uint32_t regigon_addr)
{
    hal->QSPI_R_QSPI_XIP_REGION_n_SIZE[region_id].bit.REGION_SIZE = region_size;
    hal->QSPI_R_QSPI_XIP_REGION_n_SPI_BASE[region_id].bit.REGION_SPI_BASE = regigon_addr;
}

static inline void hal_qspi_xip_master_config_set(qspi_hal *hal, uint32_t val)
{
    hal->QSPI_R_QSPI_XIP_MASTER_CFG.reg = val;
}

static inline uint32_t hal_qspi_xip_master_config_get(qspi_hal *hal)
{
    return hal->QSPI_R_QSPI_XIP_MASTER_CFG.reg;
}

static inline void hal_qspi_xip_master_config(qspi_hal *hal)
{
    uint32_t reg_val = hal->QSPI_R_QSPI_XIP_MASTER_CFG.reg;

    reg_val &= ~QWLAN_QSPI_R_QSPI_XIP_MASTER_CFG_AHBCLK_FREQ_MASK;
    reg_val |= QWLAN_QSPI_R_QSPI_XIP_MASTER_CFG_AHBCLK_FREQ_DEFAULT |  // 64MHz
               QWLAN_QSPI_R_QSPI_XIP_MASTER_CFG_ADDRESS_TRANSLATION_EN_REGION_3_MASK |
               QWLAN_QSPI_R_QSPI_XIP_MASTER_CFG_XIP_ENABLE_MASK;

    hal->QSPI_R_QSPI_XIP_MASTER_CFG.reg = reg_val;
}
#endif  //__FERM_QSPI_HAL_H__
