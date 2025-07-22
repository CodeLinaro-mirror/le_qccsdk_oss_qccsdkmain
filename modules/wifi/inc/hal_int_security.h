/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _HAL_INT_SECURITY_H_

#define _HAL_INT_SECURITY_H_

#include <stdint.h>

#include "nt_common.h"
#include "hal_api_sys.h"

// Length of the security keys

#define HAL_DPU_WEP40_LENGTH       5
#define HAL_DPU_WEP104_LENGTH      13
#define HAL_DPU_AES_LENGTH         16
#define HAL_DPU_TKIP_KEY_LENGTH    16
#define HAL_DPU_WAPI_KEY_LENGTH    16
#define HAL_DPU_TKIP_MICKEY_LENGTH 16
#define HAL_DPU_TKIP_MICKEY_SIZE   8

#define nt_hal_rplyidx(x, y)                                                     \
    ((((x) + (y)) << (32 - 8)) | ((((x) + (y + 1)) << (32 - 16)) & 0x00ff0000) | \
     ((((x) + (y + 2)) << (32 - 24)) & 0x0000ff00) | (((x) + (y + 3)) & 0x000000ff))

// DPU desc idx for BC/MC data frames
#define HAL_DPUDESC_IDX_BCMC_DATA 5

// DPU desc idx for BC/MC data frames
#define HAL_DPUDESC_IDX_BCMC_MGMT 6

// DPU Key Descriptor GTK Start index
#define HAL_DPU_KEYDESC_GTK_IDX_START 4

// Encryption type enum
typedef enum hal_key_t {
    HAL_DPU_ENC_NONE = 0,
    HAL_DPU_ENC_WEP40,
    HAL_DPU_ENC_WEP104,
    HAL_DPU_ENC_TKIP,
    HAL_DPU_ENC_AES,
    HAL_DPU_ENC_WAPI,  // new
    HAL_DPU_ENC_MAX
} hal_key_s;
typedef hal_key_s nt_hal_key_s;

/*
 * @brief: To sets the wepkey in bss or staion. Sets WEP-40, WEP-104, AES and TKIP keys. Call this function both AP and
 * station side to set the key
 * @param parameter1: bss pointer to the bss structure
 * @param parameter2: sta pointer to the sta structure
 * @param parameter3: enc_type - it is the enum related to which key type WEP40 - 1, WEP104 - 2, TKIP - 3, AES - 4
 * @param parameter4: pkey - it's pointer to the actual key. Key length is derived from the key type
 * @param parameter5: it's pointer pointing to the mic key
 * @param parameter6: wepdefkeyid - default WEP key id to use(should always < 4)
 * @returns: Invalid key length or Invalid key id return NT_EPARAM otherwise return  NT_OK
 */
nt_status_t hal_key_set(nt_hal_bss_t *bss, nt_hal_sta_t *sta, hal_key_s enc_type, uint8_t *pkey, uint8_t *mickey,
                        uint8_t defkeyidx);
nt_status_t nt_key_set(nt_hal_bss_t *bss, nt_hal_sta_t *sta, hal_key_s enc_type, uint8_t *pkey, uint8_t *mickey,
                       uint8_t defkeyidx);

/*
 * @brief: Set the GTK to BSS descriptor. Both AP and station side.
 * @param parameter1: bss pointer to the bss structure
 * @param parameter2: pkey - it's pointer to the actual key
 * @param parameter3: pkey_len - length of the key
 * @param parameter4: key_idx  - gtk key index
 * @param parameter5: enc_type - it is the enum related to which key type  TKIP - 3, AES - 4
 * @returns: Invalid enc_type return NT_EPARAM otherwise return  NT_OK
 */
nt_status_t hal_set_gtk(nt_hal_bss_t *bss, uint8_t *pkey, uint8_t pkey_len, uint8_t key_idx, hal_key_s enc_type,
                        uint8_t *mickey);
nt_status_t nt_hal_set_gtk(nt_hal_bss_t *bss, uint8_t *pkey, uint8_t pkey_len, uint8_t key_idx, nt_hal_key_s enc_type,
                           uint8_t *mickey);

nt_status_t hal_dpu_keydesc_set(uint8_t keydescidx, hal_key_s enc_type, uint8_t *pkey);
nt_status_t hal_dup_desc_set(uint8_t dpuidx, hal_key_s enc_type, uint8_t defwepid, uint8_t *pkey);
uint8_t hal_dpu_descidx_get(uint8_t mode, uint8_t staid);
uint8_t nt_dpu_descidx_get(uint8_t mode, uint8_t staid);

/*
 *@brief:  To invalidate the keys
 *@param parameter1: bss pointer to the bss structure
 *@param parameter2: sta pointer to the sta structure
 */
nt_status_t hal_invalidate_key(nt_hal_bss_t *bss, nt_hal_sta_t *sta);

/*
 *@brief:  To invalidate gtk keys
 *@param parameter1: bss pointer to the bss structure
 */
nt_status_t hal_invalidate_gtk(nt_hal_bss_t *bss);

#endif  //_HAL_INT_SECURITY_H_
