/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

//
// This file contains the api for the Key Management  on the target.
//
// $Id:
//
//

#ifndef _KEYMGMT_API_H_
#define _KEYMGMT_API_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "nt_common.h"
#include "nt_sme_mlme.h"

#define MAX_WEP_KEY_CMDS 4

void *keymgmt_init(devh_t *dev);
void keymgmt_deinit(void *key_mgmt);

void keymgmt_connection_init(devh_t *dev, conn_t *conn, NT_BOOL bssConn);
void keymgmt_connection_complete(devh_t *dev, conn_t *conn, NT_BOOL bssConn);
void keymgmt_connection_close(devh_t *dev, conn_t *conn, NT_BOOL bssConn);

NT_BOOL keymgmt_is_wep_key_set(devh_t *dev);
int keymgmt_get_wep_key_len(devh_t *dev);
int keymgmt_get_sec_type(devh_t *dev, conn_t *conn);
nt_status_t keymgmt_add_wep_key_cmd(devh_t *dev, nt_wep_key_cfg_t *wep_key_cfg_ptr);
void keymgmt_add_key_cmd(devh_t *dev, WMI_ADD_CIPHER_KEY_CMD *buffer);
nt_status_t keymgmt_set_wep_def_key_idx_cmd(devh_t *dev, uint8_t *wep_def_key_idx);

void keymgmt_set_tkip_countermeasures_cmd(devh_t *dev, WMI_SET_TKIP_COUNTERMEASURES_CMD *buffer);
uint16_t keymgmt_pmksa_copy_pmkid(devh_t *dev, bss_t *bss, uint8_t *dst);
uint32_t keymgmt_get_num_cached_pmkid(devh_t *dev, bss_t *bss);
uint8_t *keymgmt_pmksa_lookup_pmkid(devh_t *dev, uint8_t *bssid);
void keymgmt_pmksa_set_pmkid(devh_t *dev, uint8_t *bssid, uint8_t *pmkid);
nt_status_t keymgmt_indicate_uplink_data(devh_t *dev, uint8_t *abf);

#ifdef ATH_KF
void keymgmt_set_pmkid_list_cmd(devh_t *dev, WMI_SET_PMKID_LIST_CMD *buffer);
void keymgmt_get_pmkid_list_cmd(devh_t *dev, WMI_PMKID_LIST_REPLY *buffer);
CRYPTO_TYPE keymgmt_get_cipher_type(devh_t *dev, struct rxbfChain *rxChain);
#endif  // ATH_KF

void keymgmt_km_adaptate(devh_t *dev, uint8_t *pAddr);
void keymgmt_km_clear(devh_t *dev);

#ifdef __cplusplus
}
#endif

#endif /* _KEYMGMT_API_H_ */
