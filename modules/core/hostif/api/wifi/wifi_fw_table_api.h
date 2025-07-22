/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*========================================================================
 * @file wifi_fw_table_api.h
 * @brief WiFi Firmware initialization table APIs/Defs to be shared with Apps
 *========================================================================*/
#ifndef WIFI_FW_TABLE_API_H
#define WIFI_FW_TABLE_API_H

/*------------------------------------------------------------------------
 * Include Files
 * ----------------------------------------------------------------------*/
#include "wifi_fw_cmn_api.h"

/*------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * ----------------------------------------------------------------------*/
/* WLAN Firmware Table Address holder */
#define WIFI_FW_TABLE_ADDR_HOLDER (0x000001f4)

/* WLAN Firmware Table Address holder when used with legacy platform*/
#define WIFI_FW_TABLE_ADDR_HOLDER_LEGACY (0x00000168)

/* This is the place where the reason gets reflected for the multiuse gpio(formerly named as TSF sync GPIO) */
#define MULTIUSE_GPIO_ASSERT_REASON_HOLDER (WIFI_FW_TABLE_ADDR_HOLDER + 4)

/*-------------------------------------------------*/
/* A2F direction => Application to (wifi) Firmware */
/* F2A direction => (wifi) Firmware to Application */
/*-------------------------------------------------*/

#define WIFI_FW_SBL_CORRUPTED_PATTERN (0xF2A0DEAD)
#define A2F_SBL_ELF_UPDATED           (0xA2F00001)
#define F2A_SBL_ELF_UPDATE_SUCCESS    (0xF2A00001)
#define F2A_SBL_ELF_UPDATE_FAILED     (0xF2A00002)
#define WIFI_FW_SBL_ENTERED_PATTERN   (0xF2AFACE1)

#define WIFI_FW_TABLE_HDR_PATTERN (0xFE0DEFA)
#define WIFI_FW_MAJOR_VER         (0)
#define WIFI_FW_MINOR_VER         (2)

/* Reason codes for multiuse_gpio_assert_reason */
typedef enum MULTIUSE_GPIO_ASSERT_REASON {
    MULTIUSE_GPIO_POST_DEEPSLEEP_RESET =
        0x5A00,                       /* Reason code for deepsleep wakeup, The fermion_defaults will be reinitialized */
    MULTIUSE_GPIO_TSF_SYNC = 0x5A01,  /* Reason code for TSF sync request */
    MULTIUSE_GPIO_CORE_DUMP = 0x5A02, /* Reason code for coredump */
    MULTIUSE_GPIO_INVALID = 0x5A7F,   /* Invalid reason code */
} MULTIUSE_GPIO_ASSERT_REASON_ENUM;

/*------------------------------------------------------------------------
 * Type Declarations
 * ----------------------------------------------------------------------*/

/* The DWORD structure which reflects the info for multiuse gpio assertion */
typedef struct wifi_fw_multiuse_gpio_assert_info {
    uint16_t multiuse_gpio_assert_reason;
    uint16_t reserved;
} wifi_fw_multiuse_gpio_assert_info_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(wifi_fw_multiuse_gpio_assert_info_t);
WIFI_FW_STRUCT_SIZE_SYNC(wifi_fw_multiuse_gpio_assert_info_t, (4));

/* The table shared to host when there is an invalid SBL image */
typedef struct wifi_fw_boot_defaults {
    uint32_t table_hdr; /* Table Header pattern for host verification */
    uint16_t table_len; /* Table length for host verification */
    uint16_t reserved;

    /* DFU parameters for sbl download */
    uint32_t *p_a2f_status;    /* the address to which the dfu status for WLAN Firmware sbl gets updated */
    void *p_sbl_elf_push_addr; /* the address to which ermion sbl gets updated */
    uint32_t max_sbl_elf_size; /* The max size allocated for SBL elf in WLAN Firmware memory*/
} wifi_fw_boot_defaults_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(wifi_fw_boot_defaults_t);
WIFI_FW_STRUCT_SIZE_SYNC(wifi_fw_boot_defaults_t, (20));

/* The table shared to host when SBL image boots up */
typedef struct wifi_fw_dfu_defaults {
    uint32_t table_hdr; /* Table Header pattern for host verification */
    uint16_t table_len; /* Table length for host verification */
    uint16_t reserved1;

    /* DFU parameters for WLAN, REG_DB, BDF, INI download */
    uint32_t wifi_fw_pbl_ver;    /* WLAN Firmware pbl version for Host verification */
    uint32_t wifi_fw_wlan_ver;   /* WLAN Firmware wlan version for Host verification */
    uint32_t wifi_fw_sbl_ver;    /* WLAN Firmware sbl version for Host verification */
    uint32_t wifi_fw_bdf_ver;    /* WLAN Firmware bdf version for Host verification */
    uint32_t wifi_fw_reg_db_ver; /* WLAN Firmware reg_db version for Host verification */
    uint32_t wifi_fw_ini_ver;    /* WLAN Firmware ini version for Host verification */

    uint8_t a2f_dfu_ring_elem_size; /* size of each elememnt of A2F DFU ring */
    uint8_t f2a_dfu_ring_elem_size; /* size of each element of F2A DFU ring */
    uint8_t a2f_dfu_ring_num_elems; /* Maximum Num elements in A2F DFU ring */
    uint8_t f2a_dfu_ring_num_elems; /* Maximum Num elements in A2F DFU ring */

    uint32_t *p_a2f_dfu_read_idx;  /* a2f DFU read index */
    uint32_t *p_f2a_dfu_write_idx; /* f2a DFU write index */

    uint32_t *p_f2a_dfu_read_idx;  /* f2a DFU read idx */
    uint32_t *p_a2f_dfu_write_idx; /* a2f DFU write idx */

    /* Base address of A2F DFU Ring */
    void *p_a2f_dfu_ring_base;

    /* Base address of F2A DFU Ring */
    void *p_f2a_dfu_ring_base;
} wifi_fw_dfu_defaults_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(wifi_fw_dfu_defaults_t);
WIFI_FW_STRUCT_SIZE_SYNC(wifi_fw_dfu_defaults_t, (60));

/* The table shared to host when wlan image boots up */
typedef struct wifi_fw_defaults {
    uint32_t table_hdr; /* Table Header pattern for host verification */
    uint16_t table_len; /* Table length for host verification */
    uint16_t reserved0;

    uint16_t wifi_fw_maj_ver; /* WLAN Firmware Major version for Host verification */
    uint16_t wifi_fw_min_ver; /* WLAN Firmware Minor version for Host verification */

    uint8_t mac_addr[IEEE80211_ADDR_LENGTH]; /*WLAN Device Mac Address*/
    uint16_t reserved1;

    uint8_t num_a2f_rings; /* Number of rings supported in A2F direction  */
    uint8_t num_f2a_rings; /* Number of rings supported in F2A direction  */
    uint16_t reserved2;

    uint8_t a2f_ring0_elem_size; /* Size of each element of A2F ring0 */
    uint8_t f2a_ring0_elem_size; /* Size of each element of F2A ring0 */
    uint8_t a2f_ring0_num_elems; /* Maximum Num elements in A2F ring0 */
    uint8_t f2a_ring0_num_elems; /* Maximum Num elements in A2F ring0 */

    /* A2F_Outdex_Ring_0, A2F_Outdex_Ring_1..........A2F_Outdex_Ring_n       */
    uint8_t *p_a2f_ring0_read_idx;
    /* F2A_Index_Ring_0, F2A_Index_Ring_1...... F2A_Index_Ring_n             */
    uint8_t *p_f2a_ring0_write_idx;
    /* F2A_Outdex_Ring_0,  F2A_Outdex_Ring_1........ F2A_Outdex_Ring_n       */
    uint8_t *p_f2a_ring0_read_idx;
    /* A2F_Index_Ring_0, A2F_Index_Ring_1...... A2F_Index_Ring_n             */
    uint8_t *p_a2f_ring0_write_idx;

    /* Base address of A2F Control command Ring */
    void *p_a2f_ring0_base;

    /* Base address of F2A Control event Ring */
    void *p_f2a_ring0_base;

    /*Host Fermion Communication Data Ring*/
    uint8_t a2f_ring1_elem_size; /* Size of each element of A2F ring0 */
    uint8_t f2a_ring1_elem_size; /* Size of each element of F2A ring0 */
    uint8_t a2f_ring1_num_elems; /* Maximum Num elements in A2F ring0 */
    uint8_t f2a_ring1_num_elems; /* Maximum Num elements in A2F ring0 */

    /* A2F_Outdex_Ring_0, A2F_Outdex_Ring_1..........A2F_Outdex_Ring_n       */
    uint8_t *p_a2f_ring1_read_idx;
    /* F2A_Index_Ring_0, F2A_Index_Ring_1...... F2A_Index_Ring_n             */
    uint8_t *p_f2a_ring1_write_idx;
    /* F2A_Outdex_Ring_0,  F2A_Outdex_Ring_1........ F2A_Outdex_Ring_n       */
    uint8_t *p_f2a_ring1_read_idx;
    /* A2F_Index_Ring_0, A2F_Index_Ring_1...... A2F_Index_Ring_n             */
    uint8_t *p_a2f_ring1_write_idx;

    /* Base address of A2F Control command Ring */
    void *p_a2f_ring1_base;

    /* Base address of F2A Control event Ring */
    void *p_f2a_ring1_base;

    /* INI parameters */
    void *p_ini_file;
    uint16_t max_ini_len;
    uint16_t reserved3;

    /* Core dump parameters */
    void *p_core_dump;
    uint16_t core_dump_len;
    uint16_t reserved4;

    /* WLAN Firmware logging parameters */
    /*Pointer to the logging buffer*/
    void *p_wifi_fw_log_buf;
    /* Index till which Host has read*/
    uint32_t *p_wifi_fw_log_read_idx;
    /* Index till which FW has written */
    uint32_t *p_wifi_fw_log_write_idx;
    /* Maximum size of logger buffer */
    uint16_t wifi_fw_log_entry_size;
    uint16_t reserved5;
} wifi_fw_defaults_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(wifi_fw_defaults_t)
WIFI_FW_STRUCT_SIZE_SYNC(wifi_fw_defaults_t, (112))

#endif /* WIFI_FW_TABLE_API_H */
