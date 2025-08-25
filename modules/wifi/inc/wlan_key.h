/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

//
// Key Table Manipulation Routines
//

#ifndef __WLAN_KEY__
#define __WLAN_KEY__

#include "nt_common.h"
#include "wlan_key_pmk_db.h"
#include "wlan_dev.h"
#include "wlan_conn.h"
//#include "mfp_crypto.h"

#define WHAL_KEY_CACHE_SIZE 64

typedef enum {
    WHAL_CIPHER_CLR = 0, /* no encryption */
    WHAL_CIPHER_WEP40 = 0,
    WHAL_CIPHER_WEP104 = 1,
    WHAL_CIPHER_TKIP = 2,
    WHAL_CIPHER_CCMP = 3,
} WHAL_CIPHER;

#define WEP_INV_KEY_INDEX 0xFF
#define MAX_TID           8

/* This matches with key type definition in dpm mlme_al.h file */
typedef enum {
    WLAN_NONE_CRYPT = 0,
    WLAN_WEP40_CRYPT,
    WLAN_WEP104_CRYPT,
    WLAN_AES_CRYPT,
    WLAN_TKIP_CRYPT
} WLAN_CRYPTO_TYPE;

/*
 * With group key rekeying, some APs takes some time to transition to the
 * new group key. During this transition, AP will be sending multicast frames
 * using the old key. If the supplicant, plumbs the new key during this period,
 * we cannot overwrite the key RSC on the WMI_ADD_KEY cmd, as the AP may still
 * transmit frames with the old key. So save the new keyRSC and use it only
 * when the AP switches to the new key index. This is determined by using the
 * keyidx in the IV of the frame
 */
typedef struct {
    uint8_t updatePending;
    uint8_t curKeyId;
    uint8_t newGrpKeyId;
} GRP_KEY_UPDATE_STRUCT;

typedef struct wlan_keymgmt_struct {
    /* Device specific information */
    pmksa_cache_t pmksa_cache;
    uint8_t assoc_bssid[IEEE80211_ADDR_LEN];
    NT_BOOL multiPMKIDEn;
    GRP_KEY_UPDATE_STRUCT grpKeyUpdateStruct;

    /* Static Wep Key Related information */
    uint8_t wep_type;  // WEP40 or WEP104
    uint8_t wep_cmds;
    uint8_t wep_key_len;
    uint8_t wep_def_key_idx;
    uint8_t wep_cache[WEP_CACHE_SIZE];
} WLAN_KEYMGMT_DEV_STRUCT;

NT_BOOL keymgmt_is_def_txkey_set(devh_t *dev);
void keymgmt_pmksa_init(devh_t *dev);
nt_status_t keymgmt_hold_wpav1_m4_frame(devh_t *dev, uint8_t *bufPtr);
nt_status_t keymgmt_delete_pairwise_key(conn_t *conn);

#endif /* __WLAN_KEY__ */
