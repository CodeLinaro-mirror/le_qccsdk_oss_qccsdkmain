/*========================================================================
 *Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 * @brief QcSPI Slave Driver
 *========================================================================*/
/*-------------------------------------------------------------------------
 * Include Files
 * ----------------------------------------------------------------------*/
#include "qcspi_slave_api.h"
#include "HALhwio.h"
#include "assert.h"
#include "nt_gpio_api.h"
#include "fwconfig_cmn.h"
#include "nt_flags.h"
#include <stdint.h>
#include <stdbool.h>
#ifdef SUPPORT_QCSPI_SLAVE
#if defined(SUPPORT_RING_IF) || defined(SUPPORT_RING_IF_ONLY)
#include "ring_svc_api.h"
#endif
#include "Fermion_hwiobase.h"
#include "Fermion_seq_hwioreg.h"
#include "wifi_fw_cmn_api.h"
/*-------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * ----------------------------------------------------------------------*/
#define QCSPI_SLAVE_ENABLE              0x1
#define QCSPI_SLAVE_DISABLE             0x0
#define QCSPI_SLAVE_FR_BOOT_STRAP_VALE  0x63887466
#define QCSPI_SLAVE_NVIC_ISER2          0xE000E108
#define QCSPI_SLAVE_NVIC_MASK           (0x1 << 8)
#define QCSPI_SLAVE_HOST_INT0_MASK      0x1000000
#define QCSPI_SLAVE_HOST_INT1_MASK      0x2000000
#define QCSPI_SLAVE_HOST_INT2_MASK      0x4000000
#define QCSPI_SLAVE_DW_SPI_GPIO_PIN     0x20
#define QCSPI_SLAVE_CLEAR_INT_MAX_RETRY 3
// Following is the priority of QcSPI slave peripheral in AHB arbitration to highest, to reduce access latencies
#define QCSPI_AHB_PRI_CONFIG 0xF

#define AHB_LOWEST_PRI_CONFIG 0x1

/*-------------------------------------------------------------------------
 * Externalized Function Definitions
 * ----------------------------------------------------------------------*/
/**
 *@func.    qcspi_slv_init
 *@brief
 * Slave initialization sequence:
 * 1.    Disable core
 * 2.    Set HOST_CTRL
 * 3.    Configure parameters
 * 4.    Enable core
 *@return NULL
 *@Param NULL
 */
void __attribute__((section(".__sect_ps_txt"))) qcspi_slv_init(void)
{
    uint32_t temp;

    // gpio disable dw_spi_slave
    nt_gpio_pin_mode(NT_GPIOA, QCSPI_SLAVE_DW_SPI_GPIO_PIN, GPIO_INPUT);
    nt_gpio_pin_write(NT_GPIOA, QCSPI_SLAVE_DW_SPI_GPIO_PIN, NT_GPIO_LOW);

    // PMU Root clock enable to SPI Slave
    HWIO_OUTXF(SEQ_WCSS_PMU_OFFSET, NEUTRINO_PMU_PRONTO_LP_FRODO_PMU_ROOT_CLK_ENABLE, SPI_ROOT_CLK_ENABLE,
               QCSPI_SLAVE_ENABLE);
    // Enable corresponding NVIC bit
    temp = in_dword(QCSPI_SLAVE_NVIC_ISER2);
    temp |= QCSPI_SLAVE_NVIC_MASK;
    out_dword(QCSPI_SLAVE_NVIC_ISER2, temp);
    // This register needs to be written first to unlock write access to BOOT_STRAP_CONFIGURATION_STATUS
    HWIO_OUTXF(SEQ_WCSS_PMU_OFFSET, NEUTRINO_PMU_PRONTO_LP_FRODO_PMU_BOOT_STRAP_CONFIG_SECURE, BOOT_STRAP_CONFIG_SECURE,
               QCSPI_SLAVE_FR_BOOT_STRAP_VALE);
    // Disabling DWSPI, Enabling QcSPI
    HWIO_OUTXF(SEQ_WCSS_PMU_OFFSET, NEUTRINO_PMU_PRONTO_LP_FRODO_PMU_BOOT_STRAP_CONFIGURATION_STATUS,
               CFG_SPISLAVE_SELECT, QCSPI_SLAVE_ENABLE);

    // This register needs to be written first to unlock write access to BOOT_STRAP_CONFIGURATION_STATUS
    HWIO_OUTXF(SEQ_WCSS_PMU_OFFSET, NEUTRINO_PMU_PRONTO_LP_FRODO_PMU_BOOT_STRAP_CONFIG_SECURE, BOOT_STRAP_CONFIG_SECURE,
               QCSPI_SLAVE_FR_BOOT_STRAP_VALE);

    // enable spi slave
    HWIO_OUTXF(SEQ_WCSS_PMU_OFFSET, NEUTRINO_PMU_PRONTO_LP_FRODO_PMU_BOOT_STRAP_CONFIGURATION_STATUS, CFG_SPI_ENABLE,
               QCSPI_SLAVE_ENABLE);

    // Disabling Serial Synchronous Interface
    HWIO_OUTXF(SEQ_WCSS_DWSPI_SLAVE_OFFSET, DWSPI_SLAVE_DWSPI_SLAVE_SSIENR, SSI_EN, QCSPI_SLAVE_DISABLE);
    // 1.  Disable QcSPI Slave Core
    HWIO_OUTXF(SEQ_WCSS_QCSPI_SLAVE_OFFSET, QCSPI_SLAVE_QCSPI_SLAVE_R_SPI_SLAVE_CONFIG, CORE_DIS, QCSPI_SLAVE_ENABLE);
    /**
     * 2.  Configuring QcSpi
     * Keeping default values except N_DUMMY and Address_byte_length
     * Enabling WP_DIS and SEQMOD
     * (5 dummy bytes, 0x05<<24 and 4 address bytes, ~(2<<15))
     */
    HWIO_OUTX(SEQ_WCSS_QCSPI_SLAVE_OFFSET, QCSPI_SLAVE_QCSPI_SLAVE_R_SPI_SLAVE_CONFIG, QCSPI_HOST_CONFIG);

    /**
     * Enabling HOST_INT0, HOST_INT1, HOST_INT2 and SW RESET
     * HOST_INT - to be able to call control interface
     * SW RESET - Software reset here refers to QcSPI slave core's reset
     */
    HWIO_OUTX4F(SEQ_WCSS_QCSPI_SLAVE_OFFSET, QCSPI_SLAVE_QCSPI_SLAVE_R_SPI_SLAVE_IRQ_EN, HOST_INT0_IRQ_EN,
                HOST_INT1_IRQ_EN, HOST_INT2_IRQ_EN, SW_RESET_IRQ_EN, QCSPI_SLAVE_ENABLE, QCSPI_SLAVE_ENABLE,
                QCSPI_SLAVE_ENABLE, QCSPI_SLAVE_ENABLE);

    // 3.   Re-Enabling QcSPI Slave Core
    HWIO_OUTXF(SEQ_WCSS_QCSPI_SLAVE_OFFSET, QCSPI_SLAVE_QCSPI_SLAVE_R_SPI_SLAVE_CONFIG, CORE_DIS, QCSPI_SLAVE_DISABLE);

    // Setting QcSPI priority over AHB, to reduce memory access latencies
    HWIO_OUTXF(SEQ_WCSS_CDAHB_OFFSET, CDAHB_PRONTO_CDAHB_CDAHB_SPI_S_PL, PRIORITY, QCSPI_AHB_PRI_CONFIG);
}

/**
 *@func.    qcspi_slv_deinit
 *@brief
 * Slave initialization sequence:
 * 1.    Disable QcSPI Slave core
 * 2.    Set HOST_CTRL
 * 3.    Configure parameters
 * 4.    Disable SPI Slave core
 *@return NULL
 *@Param NULL
 */
void __attribute__((section(".__sect_ps_txt"))) qcspi_slv_deinit(void)
{
    uint32_t temp;

    /*QCSPI Deinit*/
    // Setting QcSPI priority over AHB, to reduce memory access latencies
    HWIO_OUTXF(SEQ_WCSS_CDAHB_OFFSET, CDAHB_PRONTO_CDAHB_CDAHB_SPI_S_PL, PRIORITY, AHB_LOWEST_PRI_CONFIG);

    // 1.  Disable QcSPI Slave Core
    HWIO_OUTXF(SEQ_WCSS_QCSPI_SLAVE_OFFSET, QCSPI_SLAVE_QCSPI_SLAVE_R_SPI_SLAVE_CONFIG, CORE_DIS, QCSPI_SLAVE_ENABLE);

    /**
     * 2.  Configuring QcSpi
     * Keeping default values except N_DUMMY and Address_byte_length
     * Enabling WP_DIS and SEQMOD
     * (5 dummy bytes, 0x05<<24 and 4 address bytes, ~(2<<15))
     */
    HWIO_OUTX(SEQ_WCSS_QCSPI_SLAVE_OFFSET, QCSPI_SLAVE_QCSPI_SLAVE_R_SPI_SLAVE_CONFIG, 0);

    /**
     * Disable HOST_INT0, HOST_INT1, HOST_INT2 and SW RESET
     * HOST_INT - to be able to call control interface
     * SW RESET - Software reset here refers to QcSPI slave core's reset
     */
    HWIO_OUTX(SEQ_WCSS_QCSPI_SLAVE_OFFSET, QCSPI_SLAVE_QCSPI_SLAVE_R_SPI_SLAVE_IRQ_EN, 0);

#if 0
	//Disabling Serial Synchronous Interface
    HWIO_OUTXF(SEQ_WCSS_DWSPI_SLAVE_OFFSET,
    DWSPI_SLAVE_DWSPI_SLAVE_SSIENR,
    SSI_EN, QCSPI_SLAVE_DISABLE);
#endif

    // This register needs to be written first to unlock write access to BOOT_STRAP_CONFIGURATION_STATUS
    HWIO_OUTXF(SEQ_WCSS_PMU_OFFSET, NEUTRINO_PMU_PRONTO_LP_FRODO_PMU_BOOT_STRAP_CONFIG_SECURE, BOOT_STRAP_CONFIG_SECURE,
               QCSPI_SLAVE_FR_BOOT_STRAP_VALE);

    // disable spi slave
    HWIO_OUTXF(SEQ_WCSS_PMU_OFFSET, NEUTRINO_PMU_PRONTO_LP_FRODO_PMU_BOOT_STRAP_CONFIGURATION_STATUS, CFG_SPI_ENABLE,
               QCSPI_SLAVE_DISABLE);

    // Disabling DWSPI, Enabling QcSPI
    HWIO_OUTXF(SEQ_WCSS_PMU_OFFSET, NEUTRINO_PMU_PRONTO_LP_FRODO_PMU_BOOT_STRAP_CONFIGURATION_STATUS,
               CFG_SPISLAVE_SELECT, QCSPI_SLAVE_DISABLE);

    // Enable corresponding NVIC bit
    temp = in_dword(QCSPI_SLAVE_NVIC_ISER2);
    temp &= ~QCSPI_SLAVE_NVIC_MASK;
    out_dword(QCSPI_SLAVE_NVIC_ISER2, temp);

    // PMU Root clock enable to SPI Slave
    HWIO_OUTXF(SEQ_WCSS_PMU_OFFSET, NEUTRINO_PMU_PRONTO_LP_FRODO_PMU_ROOT_CLK_ENABLE, SPI_ROOT_CLK_ENABLE,
               QCSPI_SLAVE_DISABLE);
}

#if CONFIG_AMBIENT_POWER_ENABLE
extern void spi_rx_data_indication();
#endif
/**
 *@func.    nt_spi_slv_interrupt
 *@brief
 * Routine for interrupts triggered from QcSPI
 *@return NULL
 *@Param NULL
 */

void __attribute__((section(".after_ram_vectors"))) nt_spi_slv_interrupt(void)
{
    uint32_t qcspi_status;
    uint32_t qcspi_status_1;
    uint8_t max_retry = 0;

    // Reading IRQ Status register
    qcspi_status = HWIO_INX(SEQ_WCSS_QCSPI_SLAVE_OFFSET, QCSPI_SLAVE_QCSPI_SLAVE_R_SPI_SLAVE_IRQ_STATUS);

    // clearing interrupts
    HWIO_OUTX(SEQ_WCSS_QCSPI_SLAVE_OFFSET, QCSPI_SLAVE_QCSPI_SLAVE_R_SPI_SLAVE_IRQ_CLR, qcspi_status);

    qcspi_status_1 = HWIO_INX(SEQ_WCSS_QCSPI_SLAVE_OFFSET, QCSPI_SLAVE_QCSPI_SLAVE_R_SPI_SLAVE_IRQ_STATUS);

    while (QCSPI_SLAVE_HOST_INT0_MASK & qcspi_status_1) {
        if (max_retry > QCSPI_SLAVE_CLEAR_INT_MAX_RETRY)
            break;

        // clearing interrupts
        HWIO_OUTX(SEQ_WCSS_QCSPI_SLAVE_OFFSET, QCSPI_SLAVE_QCSPI_SLAVE_R_SPI_SLAVE_IRQ_CLR, qcspi_status);

        qcspi_status_1 = HWIO_INX(SEQ_WCSS_QCSPI_SLAVE_OFFSET, QCSPI_SLAVE_QCSPI_SLAVE_R_SPI_SLAVE_IRQ_STATUS);

        max_retry++;
    }

    // If the source of interrupt is HOST_INT0, control interface is called
    if (qcspi_status & QCSPI_SLAVE_HOST_INT0_MASK) {
#if defined(SUPPORT_RING_IF) || defined(SUPPORT_RING_IF_ONLY)
        ringif_apps_ring_update_isr();
#endif

#if CONFIG_AMBIENT_POWER_ENABLE
        spi_rx_data_indication();
#endif
    }
    // If the source of interrupt is SW RESET command from master, the SW_RESET register is set
    if (HWIO_INXF(SEQ_WCSS_QCSPI_SLAVE_OFFSET, QCSPI_SLAVE_QCSPI_SLAVE_R_SPI_SLAVE_SW_RESET, SW_RESET)) {
        HWIO_OUTX(SEQ_WCSS_QCSPI_SLAVE_OFFSET, QCSPI_SLAVE_QCSPI_SLAVE_R_SPI_SLAVE_SW_RESET, QCSPI_SLAVE_ENABLE);
    }
    // If the source of interrupt is HOST_INT1
    if (qcspi_status & QCSPI_SLAVE_HOST_INT1_MASK) {
        assert(false);
    }
    // If the source of interrupt is HOST_INT2
    if (qcspi_status & QCSPI_SLAVE_HOST_INT2_MASK) {
        assert(false);
    }
}

/**
 *@func.    qcspi_slv_disable_host_int
 *@brief
 * Disables interrupt from Host
 *@return NULL
 *@Param NULL
 */
void __attribute__((section(".after_ram_vectors"))) qcspi_slv_disable_host_int(void)
{
    HWIO_OUTXF(SEQ_WCSS_QCSPI_SLAVE_OFFSET, QCSPI_SLAVE_QCSPI_SLAVE_R_SPI_SLAVE_IRQ_EN, HOST_INT0_IRQ_EN,
               QCSPI_SLAVE_DISABLE);
}

/**
 *@func.    qcspi_slv_enable_host_int
 *@brief
 * Enables interrupt from Host
 *@return NULL
 *@Param NULL
 */
void __attribute__((section(".after_ram_vectors"))) qcspi_slv_enable_host_int(void)
{
    HWIO_OUTXF(SEQ_WCSS_QCSPI_SLAVE_OFFSET, QCSPI_SLAVE_QCSPI_SLAVE_R_SPI_SLAVE_IRQ_EN, HOST_INT0_IRQ_EN,
               QCSPI_SLAVE_ENABLE);
}
#endif  // SUPPORT_QCSPI_SLAVE
