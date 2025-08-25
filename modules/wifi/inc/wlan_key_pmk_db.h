/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

//
// PMK DB APIs
//
//

#ifndef __WLAN_KEY_PMK_DB_H__
#define __WLAN_KEY_PMK_DB_H__

typedef struct {
    uint8_t bssid[IEEE80211_ADDR_LEN];
    uint8_t pmkid[WMI_PMKID_LEN];
} pmkid_bss_info_t;

typedef struct {
    uint8_t numPMKSACached;
    pmkid_bss_info_t pmkid_bss_list[WMI_MAX_PMKID_CACHE];
} pmksa_cache_t;

void keymgmt_pmksa_init_exe(pmksa_cache_t *);
void keymgmt_get_pmkid_list_build_resp(WMI_PMKID_LIST_REPLY *, pmksa_cache_t *, NT_BOOL);
void keymgmt_set_pmkid_list_cmd_exe(WMI_SET_PMKID_LIST_CMD *, pmksa_cache_t *);
uint8_t *keymgmt_pmksa_lookup_pmkid_exe(pmksa_cache_t *, uint8_t *);
uint32_t keymgmt_get_num_cached_pmkid_exe(pmksa_cache_t *, NT_BOOL, NT_BOOL, NT_BOOL);
uint16_t keymgmt_pmksa_copy_pmkid_exe(pmksa_cache_t *, NT_BOOL, NT_BOOL, uint8_t *, uint8_t *);

#endif /* __WLAN_KEY_PMK_DB_H__ */
