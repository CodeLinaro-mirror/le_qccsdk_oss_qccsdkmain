/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

//
// File: roaming_internal.h
//
// Abstract: Internal header file used by the roaming module
//
// Notes:
//
//

#ifndef __ROAMING_INTERNAL_H__
#define __ROAMING_INTERNAL_H__

#ifdef NT_FN_ROAMING

#include "osapi.h"
//#include "wmi_api.h"
#include "mlme_api.h"
#include "pm_api.h"
#include "chop_api.h"
#include "com_api.h"
#include "discovery_api.h"
#include "keymgmt_api.h"
#include "roaming_api.h"
#ifdef WLAN_CONFIG_RRM
#include "rrm_api.h"
#endif /* WLAN_CONFIG_RRM */

/*
 * RSSI threshold parameters
 */
#define RO_BRSSI_LOWER_THRESHOLD_DISABLE 0
#define RO_BRSSI_UPPER_THRESHOLD_DISABLE 0x80

#define RO_RSSI_AVE(avg, rssi) ((!(avg)) ? (rssi) : (((avg) + (rssi)) / 2))

#define RO_FOREGND_SEARCH_INTERVAL_DEFAULT 60 /* seconds */
#define RO_BACKGND_SEARCH_INTERVAL_DEFAULT 60 /* seconds */

#define RO_DEV_FOREGND_SCAN 1
#define RO_DEV_BACKGND_SCAN 2

#define RO_RSSI_MIN_THRESH_DEFAULT 60
#define RO_RSSI_MAX_THRESH_DEFAULT 120

#define RO_RATE_MIN_THRESH_DEFAULT HAL_RT_IDX_MCS_1NSS_MM_6_5_MBPS
#define RO_RATE_MAX_THRESH_DEFAULT HAL_RT_IDX_MCS_1NSS_MM_19_5_MBPS

typedef struct periodic_search_params_s {
    uint32_t search_interval;
    uint32_t scan_type;
} PERIODIC_SEARCH_PARAMS;

typedef struct periodic_bg_search_params_s {
    uint8_t full_scan_freq;  /* how often full scan is done */
    uint8_t curr_scan_cnt;   /* current active scan cnt before a full scan */
    uint8_t rssi_min_thresh; /* minimum rssi threshold range 0-255 */
    uint8_t rssi_max_thresh; /* maximum rssi threshold range 0-255 */
    uint8_t rate_min_thresh; /* min rate index */
    uint8_t rate_max_thresh; /* max rate index */
} PERIODIC_BG_SEARCH_PARAMS;

#define GET_DEVICE(pro_struct) pro_struct->dev

typedef struct nt_roam_bg_scan {
    uint8_t statusBgScan;
    uint8_t bgPeriod;
    uint32_t probeType;         // active= '1' , passive = '2';can be made as enums
    uint8_t bg_full_scan_freq;  //'n' every nth scan is a full scan, 0 is invalid
    roam_trigger_types triggerType;
} nt_roam_bg_scan_t;

typedef struct nt_periodic_bg_search_params {
    uint8_t rssi_min_thresh;
    uint8_t rssi_max_thresh;
    uint16_t rate_min_thresh;
    uint16_t rate_max_thresh;
} nt_periodic_bg_search_params_t;

typedef struct {
    devh_t *dev;

    /* Parameters governing the handoff behavior */
    uint32_t roam_metric_weight;

    struct bss_s *rc_list[WLAN_NUM_ROAMING_CANDIDATES];
    uint8_t rc_list_size;
    uint8_t rc_index;

    TimerHandle_t periodic_search_timer;
    struct bss_s *bss_to_roam; /*bss used for sending probe request*/

#ifdef CONFIG_WLAN_LOWRSSI_RO_SCAN
    uint16_t old_ro_scan_period;
    uint8_t sq_lowrssi_scan_flags; /* To know whether roaming is in progress, required etc */
    NT_BOOL lrssi_scan_requested;
    NT_BOOL lrssi_event_processed;
    uint32_t curr_cfg_scan_type;
    TimerHandle_t lrssi_scan_timer;
    WMI_LOWRSSI_SCAN_PARAMS lrscan_params;
#endif

    /* Network topology  */
    PERIODIC_SEARCH_PARAMS psearch_params[RO_SEARCH_PATTERN_NUM_MAX];
    PERIODIC_SEARCH_PARAMS *periodic_search_type;

    PERIODIC_BG_SEARCH_PARAMS bg_search_params; /* background scan specific params */

    uint8_t dev_scan_type;   /* scan type - foreground or background */
    NT_BOOL bg_scan_enable;  /* bg scan enable/disable flag */
    NT_BOOL rssi_bg_scan_on; /* rssi based bg scan on/off flag */
    NT_BOOL rate_bg_scan_on; /* rate based bg scan on/off flag */
} RO_STRUCT;

#define RO_RESET_ROAM_TABLE_INDEX(pro_struct) ((pro_struct)->rc_index = 0)

#ifdef CONFIG_WLAN_LOWRSSI_RO_SCAN

/*
 *  * Following macros are used to enable/disable low rssi SCAN
 *   * and roaming functionality *
 *    *
 *     */

#define RO_IS_LOWRSSI_SCAN_REQ(pro_struct)        ((pro_struct)->sq_lowrssi_scan_flags & RO_LOWRSSI_SCAN_REQ)
#define RO_IS_LOWRSSI_ROAM_REQ(pro_struct)        ((pro_struct)->sq_lowrssi_scan_flags & RO_LOWRSSI_ROAM_REQ)
#define RO_IS_LOWRSSI_SCAN_INPROGRESS(pro_struct) ((pro_struct)->sq_lowrssi_scan_flags & RO_LOWRSSI_SCAN_INPROGRESS)

#define RO_LOWRSSI_SET_SCAN_REQ(pro_struct)        ((pro_struct)->sq_lowrssi_scan_flags |= RO_LOWRSSI_SCAN_REQ)
#define RO_LOWRSSI_SET_ROAM_REQ(pro_struct)        ((pro_struct)->sq_lowrssi_scan_flags |= RO_LOWRSSI_ROAM_REQ)
#define RO_LOWRSSI_SET_SCAN_INPROGRESS(pro_struct) ((pro_struct)->sq_lowrssi_scan_flags |= RO_LOWRSSI_SCAN_INPROGRESS)

#define RO_LOWRSSI_CLR_SCAN_REQ(pro_struct)        ((pro_struct)->sq_lowrssi_scan_flags &= ~RO_LOWRSSI_SCAN_REQ)
#define RO_LOWRSSI_CLR_ROAM_REQ(pro_struct)        ((pro_struct)->sq_lowrssi_scan_flags &= ~RO_LOWRSSI_ROAM_REQ)
#define RO_LOWRSSI_CLR_SCAN_INPROGRESS(pro_struct) ((pro_struct)->sq_lowrssi_scan_flags &= ~RO_LOWRSSI_SCAN_INPROGRESS)
#define RO_DISABLE_LOWRSSI_SEARCH()                ro_set_lowrssi_search(0)

#define RO_ENABLE_LOWRSSI_SEARCH() ro_set_lowrssi_search(1)
#endif /* LOWRSSI */

#define IS_BSS_FREE(bss) (IEEE80211_ADDR_NULL(bss->ni_bssid))

#endif  // NT_FN_ROAMING

#endif /* __ROAMING_INTERNAL_H__ */
