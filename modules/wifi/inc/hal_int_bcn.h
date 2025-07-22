/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _HAL_INT_BCN_H_
#define _HAL_INT_BCN_H_

#include "hal_int_cfg.h"
#include "hal_int_mmap.h"
#include "hal_api_sys.h"

#define HAL_BCN_INTVL_DEFAULT 100

#define HAL_BCN_TMPL_CRC_SZ  0x04
#define HAL_BCN_TMPL_BODY_SZ 0x20
#define HAL_MAC_MGMT_FRAME   0x0
#define HAL_MAC_MGMT_BEACON  0x8

#define HAL_ONE_TU 0x400

#define HAL_BEACON_TEMPLATE_HEADER 0x18
#define HAL_BEACON_INDEX           1
#define HAL_TEMPLATE_HDR_SIZE      0x60

// Pre beacon interval time
#define PRE_BEACON_INTERVAL 0X14

// Irq 32 to 60 Set Enable Register
#define NT_NVIC_ISER1 0xE000E104

#define hal_htonl(x)                                                                  \
    ((((x) & (uint32_t)0x000000ffUL) << 24) | (((x) & (uint32_t)0x0000ff00UL) << 8) | \
     (((x) & (uint32_t)0x00ff0000UL) >> 8) | (((x) & (uint32_t)0xff000000UL) >> 24))

typedef struct hal_bcn_tmpl_body_s {
#ifndef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t protocol : 2;
    uint32_t type : 2;
    uint32_t subtype : 4;
    uint32_t tods : 1;
    uint32_t fromds : 1;
    uint32_t morefrag : 1;
    uint32_t retry : 1;
    uint32_t pm : 1;
    uint32_t moredata : 1;
    uint32_t wep : 1;
    uint32_t order : 1;
    uint32_t duration : 16;
#else

    uint32_t duration : 16;

    uint32_t tods : 1;
    uint32_t fromds : 1;
    uint32_t morefrag : 1;
    uint32_t retry : 1;
    uint32_t pm : 1;
    uint32_t moredata : 1;
    uint32_t wep : 1;
    uint32_t order : 1;

    uint32_t protocol : 2;
    uint32_t type : 2;
    uint32_t subtype : 4;  //  suraj & vijay: Added hack to make beacon type/subtype match
#endif

    uint32_t daLo;

#ifndef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t daHi : 16;
    uint32_t saLo : 16;
#else
    uint32_t saLo : 16;
    uint32_t daHi : 16;
#endif

#ifndef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t saHi1 : 16;
    uint32_t saHi2 : 16;
#else
    uint32_t saHi2 : 16;
    uint32_t saHi1 : 16;
#endif

#ifndef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t bssidLo1 : 16;
    uint32_t bssidLo2 : 16;
#else
    uint32_t bssidLo2 : 16;
    uint32_t bssidLo1 : 16;
#endif

#ifndef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t bssidHi : 16;
    uint32_t seqlo : 4;
    uint32_t frag : 4;
    uint32_t seqhi : 8;
#else
    uint32_t seqhi : 8;
    uint32_t frag : 4;
    uint32_t seqlo : 4;
    uint32_t bssidHi : 16;
#endif

    uint32_t timeStampLo;
    uint32_t timeStampHi;

} hal_bcn_tmpl_body_t;

typedef struct hal_bcn_tmpl_hdr_s {
#ifndef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t reserved1 : 7;
    uint32_t template_len : 12;
    uint32_t expected_resp_sub_type : 4;
    uint32_t expected_resp_type : 2;
    uint32_t resp_is_expected : 1;
    uint32_t template_sub_type : 4;
    uint32_t template_type : 2;
#else
    uint32_t template_type : 2;
    uint32_t template_sub_type : 4;
    uint32_t resp_is_expected : 1;
    uint32_t expected_resp_type : 2;
    uint32_t expected_resp_sub_type : 4;
    uint32_t template_len : 12;
    uint32_t reserved1 : 7;
#endif

#ifndef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t reserved2 : 12;
    uint32_t tx_power : 5;
    uint32_t tx_antenna_enable : 3;
    uint32_t reserved3 : 1;
    uint32_t stbc : 2;
    uint32_t primary_data_rate_index : 9;
#else
    uint32_t primary_data_rate_index : 9;
    uint32_t stbc : 2;
    uint32_t reserved3 : 1;
    uint32_t tx_antenna_enable : 3;
    uint32_t tx_power : 5;
    uint32_t reserved2 : 12;
#endif

#ifndef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t reserved4 : 16;
    uint32_t tsf_offset : 16;
#else
    uint32_t tsf_offset : 16;
    uint32_t reserved4 : 16;
#endif

    uint32_t reserved5;
    uint32_t tsf_lower;
    uint32_t tsf_upper;
} hal_bcn_tmpl_hdr_t;

#define HAL_BCN_TMPL_DATA (HAL_MMAP_BCNTP_DESC_SZ - sizeof(hal_bcn_tmpl_hdr_t) - sizeof(hal_bcn_tmpl_body_t))

typedef struct hal_bcn_tmpl_s {
    hal_bcn_tmpl_hdr_t hdr;
    hal_bcn_tmpl_body_t body;
    uint8_t data[HAL_BCN_TMPL_DATA];
} hal_bcn_tmpl_t;

void hal_bcn_setup(nt_hal_bss_t *bss);
void hal_bcn_enable(uint16_t bcn_intv);
void hal_bcn_disable();
void hal_get_tsf(uint32_t mode, uint32_t *tsf_lo, uint32_t *tsf_hi);
uint32_t hal_get_bcn_tsfoff(uint8_t rate_idx);
void hal_bcn_update(uint32_t *bi_frame, uint16_t bcn_len);
void nt_hal_bcn_update(nt_hal_bss_t *bss);

#endif  // _HAL_INT_BCN_H_
