/*
Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _COEX_VERSION_H_
#define _COEX_VERSION_H_
/* Standard includes. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "nt_osal.h"

#include "fwconfig_wlan.h"
#include "nt_flags.h"
#include "nt_common.h"

#if defined(SUPPORT_COEX)
#include "coex_api.h"
#include "coex_gpm.h"

typedef enum _coex_ver_caps {
    COEX_VER_CAPS_BT_LE_LONG_RANGE = 0,
    COEX_VER_CAPS_BT_CONCUR_TX_CH0 = 1,
    COEX_VER_CAPS_BT_QHS = 2,
    COEX_VER_CAPS_BT_ASD = 3,
    COEX_VER_CAPS_PER_CHAIN_WLAN_STATUS = 4,
    COEX_VER_CAPS_GPM_ACK_FEATURE = 5,
    COEX_VER_CAPS_TWM_SUPPORT = 7,
    COEX_VER_CAPS_XPAN_SUPPORT = 8,
    COEX_VER_CAPS_CRX_CONTROL = 9,
    COEX_VER_CAPS_NUM
} E_COEX_VER_CAPS;

#define COEX_VERSION_BT_MAJ_7_0 (7)
#define COEX_VERSION_BT_MAJ_9_0 (9)

#define COEX_CXC_MINOR_VER_0 0
#define COEX_CXC_MAJOR_VER_5 5

#define COEX_CAP_REQ_BMASK           (0x80)
#define COEX_CAP_REQ_BIT_POS         (7)
#define COEX_VERSION_PAGE_NUM        (0XC0)
#define COEX_CAPABILITY_REQUEST_BIT  (0X80)
#define COEX_CAPABILITY_GPM_MORE_BIT (0X01)
#define COEX_VER_BITMAP_BYTES        (9)
#define COEX_VER_NUM_BITS_IN_BYTE    (8)
#define COEX_VER_NUM_PAGES_SUPPORTED \
    (((COEX_VER_CAPS_NUM - 1) / (COEX_VER_BITMAP_BYTES * COEX_VER_NUM_BITS_IN_BYTE)) + 1)

typedef struct _coex_capability_cfg {
    uint8_t bt_maj_ver;
    uint8_t bt_min_ver;
    uint8_t cap_info; /* This fielf will be copied to the GPM. */
    uint8_t wlan_cap_pages;
    uint8_t wlan_cap_pages_sent;
    uint8_t bt_cap_pages;
    uint8_t bt_cap_pages_received;
    uint8_t bt_cap[COEX_VER_NUM_PAGES_SUPPORTED][COEX_VER_BITMAP_BYTES];
    uint8_t wlan_cap[COEX_VER_NUM_PAGES_SUPPORTED][COEX_VER_BITMAP_BYTES];
} coex_capability_cfg;

uint8_t coex_version_process_bt_capablilities_gpm(coex_gpm_capabilities *caps);
void coex_version_init(void);
void coex_version_init_supported_caps(void);
void coex_version_send_all_wlan_capabilities(void);
void coex_set_bt_versions(uint8_t major, uint8_t minor);
uint8_t coex_version_get_cap_info(void);
uint8_t coex_version_get_bt_major(void);
uint8_t coex_get_wlan_version(uint8_t *major, uint8_t *minor);
uint8_t coex_version_is_feature_supported(E_COEX_VER_CAPS capability);

#endif  // #if defined(SUPPORT_COEX)
#endif  // _COEX_VERSION_H_
