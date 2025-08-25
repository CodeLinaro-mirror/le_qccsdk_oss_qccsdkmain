/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*========================================================================
 * @file wifi_fw_cmn_api.h
 * @brief Wi-Fi Firmware default defs used in other API files
 *========================================================================*/
#ifndef WIFI_FW_CMN_API_H
#define WIFI_FW_CMN_API_H

/*------------------------------------------------------------------------
 * Include Files
 * ----------------------------------------------------------------------*/
/* None */

/*------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * ----------------------------------------------------------------------*/
#if defined(__GNUC__)
#define PACKSTRUCT __attribute__((packed))
#else
#define PACKSTRUCT
#endif

#define Reserve_24_BIT 3

#ifndef WIFI_FW_COMPILE_ASSERT
#define WIFI_FW_COMPILE_ASSERT(predicate) extern uint8_t wifi_fw_dummy_array[(predicate) ? 1 : -1];
#endif /* WIFI_FW_COMPILE_ASSERT */

#ifndef WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK
#define WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(struct) WIFI_FW_COMPILE_ASSERT(!(sizeof(struct) % 4))
#endif /* WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK */

#ifndef WIFI_FW_STRUCT_SIZE_SYNC
#define WIFI_FW_STRUCT_SIZE_SYNC(struct, val) WIFI_FW_COMPILE_ASSERT(((sizeof(struct)) == val))
#endif /* WIFI_FW_STRUCT_SIZE_SYNC */

#define IEEE80211_ADDR_LENGTH 6

/*
Agreed config for the QcSPI:
Please look at register bitfield definitons for more details, for the register QCSPI_SLAVE_CONFIG
QCSPI_CONFIG_ACC_SIZE_WORD is 0 (4 bytes)
QCSPI_CONFIG_N_DUMMY is 10 bytes
QCSPI_CONFIG_HOST_CTRL is 0
QCSPI_CONFIG_ADDR_BYTE_LEN is 4 bytes
QCSPI_CONFIG_RDBREN is unset
QCSPI_CONFIG_WRBREN is unset
QCSPI_CONFIG_WPDIS is set
QCSPI_CONFIG_SEQMOD is 0, little edian
QCSPI_CONFIG_CPOL is 0
QCSPI_CONFIG_CPHA(0) is 0
QCSPI_CONFIG_CORE_DIS is unset
QCSPI_CONFIG_EXT_BASE_ADDR_LOCK(1) is set
QCSPI_CONFIG_SPI_ACC_CTRL is unset
*/
#define QCSPI_HOST_CONFIG 0x0a050800

#define QCSPI_EXPECTED_SANITY 0xDEADBEEF
/** Expected STATUS reg value based on QCSPI_HOST_CONFIG.
 * Needs to be updated if host config is modified */
#define QCSPI_EXPECTED_STATUS 0x82000000

#ifdef HOST_APP_CONFIG_WAR
#define QCSPI_EXPECTED_SANITY_FA 0xDEADBEEF
/** Expected STATUS reg value based on QCSPI_HOST_CONFIG.
 * Needs to be updated if host config is modified */
#define QCSPI_EXPECTED_STATUS_FA 0x82400000
#endif /* HOST_APP_CONFIG_WAR */

/* Mask to remove bits which can change based on transactions on the bus */
#define QCSPI_STATUS_CHECK_MASK 0x82700000

enum _ringif_ipaddr_type {
    RINGIF_IP_VER_V4 = 0U,
    RINGIF_IP_VER_V6 = 1U,
};

enum _ringif_dhcp_type {
    RINGIF_IP_TYPE_STATIC = 0U,
    RINGIF_IP_TYPE_DYNAMIC = 1U,
    RINGIF_IP_TYPE_LOCAL = 2U,
};

/*------------------------------------------------------------------------
 * Type Declarations
 * ----------------------------------------------------------------------*/
/* None */

#endif /* WIFI_FW_CMN_API_H */
