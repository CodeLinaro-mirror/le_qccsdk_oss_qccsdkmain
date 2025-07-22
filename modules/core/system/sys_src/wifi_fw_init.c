/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*========================================================================
 *
 * @brief WiFi FW (qca5321/7321 family) specific initializations and APIs
 *========================================================================*/
/*-------------------------------------------------------------------------
 * Include Files
 * ----------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "fwconfig_cmn.h"
#include "nt_flags.h"
#include "nt_common.h"
#include "nt_hw.h"
#include "nt_gpio_api.h"
#include "nt_mem.h"
#ifdef IMAGE_FERMION
#include "timer_test.h"
#if defined(SUPPORT_RING_IF) || defined(SUPPORT_RING_IF_ONLY)
#include "ring_svc_api.h"
#include "wifi_fw_table_api.h"
#endif
#include "wifi_fw_internal_api.h"
#include "fdi.h"
#include "wifi_fw_logger.h"
#include "wifi_fw_ext_intr.h"
#ifdef CONFIG_WIFI_FW_COREDUMP_SUPPORT
#include "err.h"
#endif
/*-------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * ----------------------------------------------------------------------*/
#ifdef PLATFORM_FERMION
#define MULTIUSE_SYNC_GPIO GPIO_PIN_10
#define NT2APPS_GPIO       GPIO_PIN_8
#else
#define NT2APPS_GPIO GPIO_PIN_9
#endif
// For 60MHz clock, 7 NOPs + 2 Mem IO takes 1us
#ifdef FERMION_SILICON
#define NOP_CALIB                            7
#define FERM_F2A_DEFAULT_PULSE_WIDTH_US      2
#define FERM_MULTIUSE_DEFAULT_PULSE_WIDTH_US 20
#if defined(SUPPORT_RING_IF) || defined(SUPPORT_RING_IF_ONLY)
static uint32_t f2a_delay = FERM_F2A_DEFAULT_PULSE_WIDTH_US;
#endif
static uint32_t multiuse_delay = FERM_MULTIUSE_DEFAULT_PULSE_WIDTH_US;
#define US_DELAY(INPUT)                                                \
    do {                                                               \
        for (uint32_t cycle = 0; cycle < NOP_CALIB * INPUT; cycle++) { \
            __asm volatile(" nop \n");                                 \
        }                                                              \
    } while (0)
#endif  // FERMION_SILICON
/*------------------------------------------------------------------------
 * Global Data Definitions
 * ----------------------------------------------------------------------*/
/* Table holding the ring configurations exposed to apps */
#if defined(SUPPORT_RING_IF) || defined(SUPPORT_RING_IF_ONLY)
static wifi_fw_defaults_t g_fw_defaults_table;
#endif
static bool g_fw_in_hosted_mode = FALSE;
extern unsigned int _ln_RAM_ferm_table_addr;
uint32_t *g_fw_table_addr_holder = (uint32_t *)&_ln_RAM_ferm_table_addr;
#if defined(UNIT_TEST_SUPPORT) && defined(SUPPORT_RING_IF)
extern void apps_ringif_init();
#endif
#ifdef SUPPORT_FERMION_LOGGER
extern volatile debug_log_elem debugBuffer;
extern volatile size_t debugBufferPos;
extern volatile size_t g_SPI_host_read_pos;
#endif  // SUPPORT_FERMION_LOGGER
#ifdef CONFIG_QCSPI_HFC_ETH_ENABLE
extern uint8_t get_active_device();
extern int32_t wlan_get_mac_address(uint8_t, uint8_t *);
#endif

/*-------------------------------------------------------------------------
 * Static Function Definitions
 * ----------------------------------------------------------------------*/
/*
 * @brief  Initializes the Table that is to be shared with Host/Apps system
 * @param          : NONE
 * @return         : NONE
 *
 */
#if defined(SUPPORT_RING_IF) || defined(SUPPORT_RING_IF_ONLY)
void wifi_fw_defaults_table_init(void)
{
#ifdef CONFIG_QCSPI_HFC_ETH_ENABLE
    uint32_t length = 6;
    uint8_t deviceId = 0;
#endif
    /* Table that is to be exposed to Apps systems to be initialized here */
    memset(&g_fw_defaults_table, 0, sizeof(wifi_fw_defaults_t));

    g_fw_defaults_table.table_hdr = WIFI_FW_TABLE_HDR_PATTERN;
    g_fw_defaults_table.table_len = sizeof(wifi_fw_defaults_t);
    g_fw_defaults_table.reserved0 = 0;

    g_fw_defaults_table.wifi_fw_maj_ver = WIFI_FW_MAJOR_VER;
    g_fw_defaults_table.wifi_fw_min_ver = WIFI_FW_MINOR_VER;

#ifdef CONFIG_QCSPI_HFC_ETH_ENABLE
    deviceId = get_active_device();
    wlan_get_mac_address(deviceId, &g_fw_defaults_table.mac_addr[0]);
#else
    nt_get_macid(&g_fw_defaults_table.mac_addr[0]);
    /*get the mac address of wlan st1 device*/
    g_fw_defaults_table.mac_addr[IEEE80211_ADDR_LENGTH - 1] += 1;
#endif

    g_fw_defaults_table.reserved1 = 0;

#if defined(SUPPORT_RING_IF) || defined(SUPPORT_RING_IF_ONLY)
    g_fw_defaults_table.num_a2f_rings = ringif_max_num_a2f_rings();
    g_fw_defaults_table.num_f2a_rings = ringif_max_num_f2a_rings();
    g_fw_defaults_table.reserved2 = 0;

    g_fw_defaults_table.a2f_ring0_elem_size = ringif_a2f_elem_size(A2F_RING_ID_CONFIG);
    g_fw_defaults_table.f2a_ring0_elem_size = ringif_f2a_elem_size(A2F_RING_ID_CONFIG);
    g_fw_defaults_table.a2f_ring0_num_elems = ringif_a2f_num_ring_elems(A2F_RING_ID_CONFIG);
    g_fw_defaults_table.f2a_ring0_num_elems = ringif_f2a_num_ring_elems(F2A_RING_ID_CONFIG);

    g_fw_defaults_table.p_a2f_ring0_read_idx = ringif_a2f_rd_idx_array();
    g_fw_defaults_table.p_f2a_ring0_write_idx = ringif_f2a_wr_idx_array();
    g_fw_defaults_table.p_f2a_ring0_read_idx = ringif_f2a_rd_idx_array();
    g_fw_defaults_table.p_a2f_ring0_write_idx = ringif_a2f_wr_idx_array();

    g_fw_defaults_table.p_a2f_ring0_base = ringif_a2f_ring_addr(A2F_RING_ID_CONFIG);
    g_fw_defaults_table.p_f2a_ring0_base = ringif_f2a_ring_addr(F2A_RING_ID_CONFIG);

    /* Data Ring */
    g_fw_defaults_table.a2f_ring1_elem_size = ringif_a2f_elem_size(A2F_RING_ID_DATA);
    g_fw_defaults_table.f2a_ring1_elem_size = ringif_f2a_elem_size(F2A_RING_ID_DATA);
    g_fw_defaults_table.a2f_ring1_num_elems = ringif_a2f_num_ring_elems(A2F_RING_ID_DATA);
    g_fw_defaults_table.f2a_ring1_num_elems = ringif_f2a_num_ring_elems(F2A_RING_ID_DATA);

    g_fw_defaults_table.p_a2f_ring1_read_idx = ringif_a2f_rd_idx_array() + 1;
    g_fw_defaults_table.p_f2a_ring1_write_idx = ringif_f2a_wr_idx_array() + 1;
    g_fw_defaults_table.p_f2a_ring1_read_idx = ringif_f2a_rd_idx_array() + 1;
    g_fw_defaults_table.p_a2f_ring1_write_idx = ringif_a2f_wr_idx_array() + 1;

    g_fw_defaults_table.p_a2f_ring1_base = ringif_a2f_ring_addr(A2F_RING_ID_DATA);
    g_fw_defaults_table.p_f2a_ring1_base = ringif_f2a_ring_addr(F2A_RING_ID_DATA);
#endif

#ifdef SUPPORT_FERMION_LOGGER
    g_fw_defaults_table.p_wifi_fw_log_buf = (void *)&debugBuffer;
    g_fw_defaults_table.p_wifi_fw_log_read_idx = (uint32_t *)&g_SPI_host_read_pos;
    g_fw_defaults_table.p_wifi_fw_log_write_idx = (uint32_t *)&debugBufferPos;
    g_fw_defaults_table.wifi_fw_log_entry_size = DEBUG_LOG_BUF_SIZE * sizeof(uint32_t);
#else
    g_fw_defaults_table.p_wifi_fw_log_buf = NULL;
    g_fw_defaults_table.p_wifi_fw_log_read_idx = NULL;
    g_fw_defaults_table.p_wifi_fw_log_write_idx = NULL;
    g_fw_defaults_table.wifi_fw_log_entry_size = 0;
#endif  // SUPPORT_FERMION_LOGGER

#ifdef PLATFORM_FERMION
    /* Address of the table to be updated in holder location */
    if ((uint32 *)WIFI_FW_TABLE_ADDR_HOLDER == g_fw_table_addr_holder) {
        *g_fw_table_addr_holder = (uint32_t)&g_fw_defaults_table;
    } else {
        FERM_INIT_LOG_ERR("************ Wlan fw Table address mismatch (%x %x %x) **************\r\n",
                          (uint32_t)WIFI_FW_TABLE_ADDR_HOLDER, (uint32_t)g_fw_table_addr_holder,
                          (uint32_t)&g_fw_defaults_table);
        configASSERT(0);
        return;
    }
#else
    /* Address of the table to be updated in holder location */
    if ((uint32 *)WIFI_FW_TABLE_ADDR_HOLDER_LEGACY == g_fw_table_addr_holder) {
        *g_fw_table_addr_holder = (uint32_t)&g_fw_defaults_table;
    } else {
        FERM_INIT_LOG_ERR("************ Wlan fw Table address mismatch for NT platform (%x %x %x) **************\r\n",
                          (uint32_t)WIFI_FW_TABLE_ADDR_HOLDER_LEGACY, (uint32_t)g_fw_table_addr_holder,
                          (uint32_t)&g_fw_defaults_table);
        configASSERT(0);
        return;
    }
#endif

    FERM_INIT_LOG_ERR("***** Table (Tbl_Hldr: %x, Tbl_Adr:%x, Tbl_Len:%d Mj.ver:%d Mn.ver:%d) **** ",
                      (uint32_t)g_fw_table_addr_holder, (uint32_t)&g_fw_defaults_table,
                      (uint32_t)g_fw_defaults_table.table_len, (uint32_t)g_fw_defaults_table.wifi_fw_maj_ver,
                      (uint32_t)g_fw_defaults_table.wifi_fw_min_ver);

#if defined(SUPPORT_RING_IF) || defined(SUPPORT_RING_IF_ONLY)
    FERM_INIT_LOG_INFO("FermTbl A2F_Params: num_rings:%d, r0_num_elems:%d r0_elem_size:%d  ",
                       (uint32_t)g_fw_defaults_table.num_a2f_rings, (uint32_t)g_fw_defaults_table.a2f_ring0_num_elems,
                       (uint32_t)g_fw_defaults_table.a2f_ring0_elem_size);

    FERM_INIT_LOG_INFO(
        "FermTbl A2F_Params: r0_base:%x r0_rd:%x,r0_wt:%x ", (uint32_t)g_fw_defaults_table.p_a2f_ring0_base,
        (uint32_t)g_fw_defaults_table.p_a2f_ring0_read_idx, (uint32_t)g_fw_defaults_table.p_a2f_ring0_write_idx);

    FERM_INIT_LOG_INFO("FermTbl F2A_Params: num_rings:%d, r0_num_elems:%d r0_elem_size:%d ",
                       (uint32_t)g_fw_defaults_table.num_f2a_rings, (uint32_t)g_fw_defaults_table.f2a_ring0_num_elems,
                       (uint32_t)g_fw_defaults_table.f2a_ring0_elem_size);

    FERM_INIT_LOG_INFO(
        "FermTbl F2A_Params: r0_base:%x r0_rd:%x,r0_wt:%x \n\r", (uint32_t)g_fw_defaults_table.p_f2a_ring0_base,
        (uint32_t)g_fw_defaults_table.p_f2a_ring0_read_idx, (uint32_t)g_fw_defaults_table.p_f2a_ring0_write_idx);

    FERM_INIT_LOG_INFO("FermTbl A2F_Params: num_rings:%d, r1_num_elems:%d r1_elem_size:%d  ",
                       (uint32_t)g_fw_defaults_table.num_a2f_rings, (uint32_t)g_fw_defaults_table.a2f_ring1_num_elems,
                       (uint32_t)g_fw_defaults_table.a2f_ring1_elem_size);

    FERM_INIT_LOG_INFO(
        "FermTbl A2F_Params: r1_base:%x r1_rd:%x,r1_wt:%x ", (uint32_t)g_fw_defaults_table.p_a2f_ring1_base,
        (uint32_t)g_fw_defaults_table.p_a2f_ring1_read_idx, (uint32_t)g_fw_defaults_table.p_a2f_ring1_write_idx);

    FERM_INIT_LOG_INFO("FermTbl F2A_Params: num_rings:%d, r1_num_elems:%d r1_elem_size:%d ",
                       (uint32_t)g_fw_defaults_table.num_f2a_rings, (uint32_t)g_fw_defaults_table.f2a_ring1_num_elems,
                       (uint32_t)g_fw_defaults_table.f2a_ring1_elem_size);

    FERM_INIT_LOG_INFO(
        "FermTbl F2A_Params: r1_base:%x r1_rd:%x,r1_wt:%x \n\r", (uint32_t)g_fw_defaults_table.p_f2a_ring1_base,
        (uint32_t)g_fw_defaults_table.p_f2a_ring0_read_idx, (uint32_t)g_fw_defaults_table.p_f2a_ring1_write_idx);

    FERM_INIT_LOG_INFO(
        "FermTbl F2A_Params: Debug_Buffer_addr:%x Write_ptr:%x, Read_ptr:%x, Size:%d Bytes",
        (uint32_t)g_fw_defaults_table.p_wifi_fw_log_buf, (uint32_t)g_fw_defaults_table.p_wifi_fw_log_write_idx,
        (uint32_t)g_fw_defaults_table.p_wifi_fw_log_read_idx, (uint32_t)g_fw_defaults_table.wifi_fw_log_entry_size);
#endif
}
#endif

/*-------------------------------------------------------------------------
 * Externalized Function Definitions
 * ----------------------------------------------------------------------*/
/*
 * @brief  Fermion module specific initialization
 * @param          : NONE
 * @return         : NONE
 *
 */
void wifi_fw_module_init(void)
{
#if defined(SUPPORT_HIGH_RES_TIMER)
    hres_timer_init_setup();
#if defined(HRES_TIMER_UNIT_TEST)
    hres_timer_test_create_task();
#endif
#endif

#if defined(SUPPORT_RING_IF) || defined(SUPPORT_RING_IF_ONLY)
    /* Initialize the ring interface */
    // ringif_init();

    /* Update Fermion defaults Table to be used by Apps */
    // wifi_fw_defaults_table_init();
#endif

#if defined(UNIT_TEST_SUPPORT) && defined(SUPPORT_RING_IF)
    apps_ringif_init();
#endif
#ifdef CONFIG_WIFI_FW_COREDUMP_SUPPORT
    err_init();
#endif
}

#if defined(SUPPORT_RING_IF) || defined(SUPPORT_RING_IF_ONLY)
/* @brief This function tells the caller, of table init state
 * @param          : NONE
 * @return         : True, if Table is initialized
 *                 : False otherwise
 *
 */
bool wifi_fw_is_table_initialized(void)
{
    if (g_fw_defaults_table.table_hdr == WIFI_FW_TABLE_HDR_PATTERN) {
        return TRUE;
    } else {
        return FALSE;
    }
}
#endif

#ifdef FERMION_SILICON

#ifdef SUPPORT_RING_IF
/* @brief Sets F2A pulse width (in us)
 * @param          : NONE
 */
void wifi_fw_set_ext_f2a_pulse_width(uint32_t input)
{
    f2a_delay = input;
}
#endif

/* @brief Sets MULTIUSE GPIO pulse width (in us)
 * @param          : NONE
 *
 */
void wifi_fw_set_ext_multiuse_pulse_width(uint32_t input)
{
    multiuse_delay = input;
}
#endif  // FERMION_SILICON
#ifndef FIRMWARE_APPS_INFORMED_WAKE
// For PLATFORM_FERMION, this functionality is supported by FIRMWARE_APPS_INFORMED_WAKE
#if defined(SUPPORT_RING_IF) || defined(SUPPORT_RING_IF_ONLY)
/*
 * @brief  Fermion specific GPIO initialization
 * @param          : NONE
 * @return         : NONE
 *
 */
void wifi_fw_f2a_interrupt(void)
{
#ifndef FERMION_SILICON
#ifdef NT_GPIO_FLAG
    nt_gpio_pin_write(NT_GPIOA, NT2APPS_GPIO, NT_GPIO_HIGH);
    hres_timer_us_delay(2);
    nt_gpio_pin_write(NT_GPIOA, NT2APPS_GPIO, NT_GPIO_LOW);
#endif /* NT_GPIO_FLAG */
#else
    /* Wait till Fermion Table is initialized */
    if (!wifi_fw_is_table_initialized()) {
        return;
    }
#ifdef NT_GPIO_FLAG
    nt_gpio_pin_write(NT_GPIOA, NT2APPS_GPIO, NT_GPIO_LOW);
    US_DELAY(f2a_delay);
    nt_gpio_pin_write(NT_GPIOA, NT2APPS_GPIO, NT_GPIO_HIGH);
#endif  /* NT_GPIO_FLAG */
#endif  //!(FERMION_SILICON)
}
#endif
#endif /* FIRMWARE_APPS_INFORMED_WAKE */

#ifdef SUPPORT_PERIODIC_TSF_SYNC
/*
 * @brief  Fermion TSF GPIO initialization
 * @param          : NONE
 * @return         : NONE
 *
 */
void wifi_fw_ext_pulse_multiuse_gpio(MULTIUSE_GPIO_ASSERT_REASON_ENUM multiuse_reason)
{
    (void)multiuse_reason;
#ifdef FERMION_SILICON
#ifdef NT_GPIO_FLAG
    wifi_fw_multiuse_gpio_assert_info_t *assert_info =
        (wifi_fw_multiuse_gpio_assert_info_t *)MULTIUSE_GPIO_ASSERT_REASON_HOLDER;
    assert_info->multiuse_gpio_assert_reason = multiuse_reason;

    nt_gpio_pin_write(NT_GPIOA, MULTIUSE_SYNC_GPIO, NT_GPIO_HIGH);
    US_DELAY(multiuse_delay);
    nt_gpio_pin_write(NT_GPIOA, MULTIUSE_SYNC_GPIO, NT_GPIO_LOW);
#endif
#endif
}
#endif

/*
 * @brief  Fermion specific GPIO initialization
 * @param          : NONE
 * @return         : NONE
 *
 */
void wifi_fw_gpio_init(bool __attribute__((__unused__)) is_init_after_sleep)
{
#ifdef NT_GPIO_FLAG
#ifdef PLATFORM_FERMION
    nt_gpio_pin_mode(NT_GPIOA, MULTIUSE_SYNC_GPIO, GPIO_OUTPUT);

#ifdef CONFIG_RING_IF_ONLY
    /* Feature FIRMWARE_APPS_INFORMED_WAKE defines FIRMWARE_2_HOST_GPIO for ring_update interrupt */
    nt_gpio_pin_mode(FIRMWARE_2_HOST_GPIO_PORT, FIRMWARE_2_HOST_GPIO, GPIO_OUTPUT);
    /* Pull Up if active low */
    nt_gpio_pin_write(FIRMWARE_2_HOST_GPIO_PORT, FIRMWARE_2_HOST_GPIO, FIRMWARE_2_HOST_DE_ASSERT);
#endif
#endif /* PLATFORM_FERMION */
    if (!is_init_after_sleep) {
        FERM_INIT_LOG_ERR("Fermion Ring update interrupt Reg: %x\n",
                          (uint32_t)(NT_REG_RD(QWLAN_GPIO_GPIO_SWPORTA_DDR_REG)));
#ifndef FIRMWARE_APPS_INFORMED_WAKE
#ifdef SUPPORT_RING_IF
        wifi_fw_f2a_interrupt();
#endif
#endif /* FIRMWARE_APPS_INFORMED_WAKE */
    }

#else  /* NT_GPIO_FLAG */
    if (!is_init_after_sleep) {
        FERM_INIT_LOG_ERR("***Fermion Warning: GPIO Module not enabled ***\r\n");
    }
#endif /* NT_GPIO_FLAG */

#if defined(PLATFORM_FERMION) && !defined(FIRMWARE_APPS_INFORMED_WAKE)
    if (!is_init_after_sleep) {
#ifdef SUPPORT_RING_IF
        FERM_INIT_LOG_ERR("***Fermion Warning: Using NT mode of F2A interrupt ***\r\n");
#endif
    }
#else
    SOCPM_UNUSED(is_init_after_sleep);
#endif /* defined(PLATFORM_FERMION) && !defined(FIRMWARE_APPS_INFORMED_WAKE) */
}

/*
 * @brief  Returns if Fermion is being used in Hosted Mode
 * @param          : NONE
 * @return         : TRUE if Fermion in Hosted Mode
 */
bool wifi_fw_in_hosted_mode(void)
{
    return g_fw_in_hosted_mode;
}

/*
 * @brief  Store Fermion hosted mode
 * @param  bool    : Fermion_Hosted_Mode TRUE/FALSE
 * @return         : NONE
 */
void wifi_fw_set_hosted_mode(bool hosted_mode)
{
    if (g_fw_in_hosted_mode != hosted_mode) {
        g_fw_in_hosted_mode = hosted_mode;
        FERM_INIT_LOG_ERR("******** Changing Fermion Hosted Mode to:%d *******\r\n", hosted_mode);
    }
}
#endif /* IMAGE_FERMION */
