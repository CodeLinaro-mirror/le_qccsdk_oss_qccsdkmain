/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*========================================================================
 * @file wifi_fw_ring_api.h
 * @brief Ring I/F configuration APIs/Defs to be shared with Apps
 * ======================================================================*/
#ifndef WIFI_FW_RING_API_H
#define WIFI_FW_RING_API_H

/*------------------------------------------------------------------------
 * Include Files
 * ----------------------------------------------------------------------*/
#include "wifi_fw_cmn_api.h"

/*------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * ----------------------------------------------------------------------*/
/*-------------------------------------------------*/
/* A2F direction => Application to (wifi) Firmware */
/* F2A direction => (wifi) Firmware to Application */
/*-------------------------------------------------*/

#define A2F_RING_ID_CONFIG 0 /* Control command ring id */
#define F2A_RING_ID_CONFIG 0 /* Control event ring id */

#define A2F_RING_ID_DATA 1 /* A2F data ring id */
#define F2A_RING_ID_DATA 1 /* F2A data ring id */

/*------------------------------------------------------------------------
 * Type Declarations
 * ----------------------------------------------------------------------*/
typedef struct _ring_element_apps {
    uint32_t *p_buf; /* Pointer to wlan fw memory buffer*/
    uint16_t len;    /* Length of the packet */
    uint16_t info;   /* Info describing the packet type */

    /* uint32_t scratch_buf[]; NOTE: Extra variable len scratch buffer is only for wlan fw internal use */
} ring_element_apps_t;
WIFI_FW_STRUCT_4BYTE_ALLIGN_CHECK(ring_element_apps_t)
WIFI_FW_STRUCT_SIZE_SYNC(ring_element_apps_t, 8)

/* Enumeration for Config info */
typedef enum {
    CONFIG_INFO_INVALID, /* Not to be used */
    CONFIG_INFO_WIFI,    /* Wi-Fi control */
    CONFIG_INFO_RAW_ETH,
    CONFIG_INFO_IP_CONNECT,
    CONFIG_INFO_IP_SECURITY,
    CONFIG_INFO_HFC,
    CONFIG_INFO_DFU,
    CONFIG_INFO_MAX,
} CONFIG_INFO_ENUM;

#endif /* WIFI_FW_RING_API_H */
