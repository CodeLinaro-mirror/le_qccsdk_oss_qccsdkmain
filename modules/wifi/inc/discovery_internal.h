/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __DISCOVERY_INTERNAL_H__
#define __DISCOVERY_INTERNAL_H__

#include "osapi.h"
//#include "wmi_api.h"
#include "mlme_api.h"
#include "discovery_api.h"
#include "ieee80211_var.h"
//#include "wlan_80211h.h"
//#include "ieee80211_proto.h"
#include <pm_api.h>
#include <com_api.h>

#define MAX_REGCODES_LIST 4
typedef struct reg_node_s {
    NT_REG_CODE regCode;
    int32_t regUtility;
} REG_NODE;

typedef struct ssid_probe_list_s {
    ssid_t ssid[MAX_PROBED_SSIDS];
    uint8_t flag[MAX_PROBED_SSIDS];
    uint8_t index;
    uint8_t cur_probe_resp_flag;
    uint8_t match_probe_resp_flag;
} SSID_PROBE_LIST;

typedef struct dc_channel_s {
    uint8_t priority;
} DC_CHANNEL_ATTRIBUTES;

typedef struct dc_scan_list_s {
    uint8_t chindex[DEV_CHANNEL_NUM_MAX];
    uint8_t size;
    uint8_t index;
} DC_SCAN_LIST;

typedef enum {
    SEARCH_IN_PROGRESS = 0x0001,
    CHANNEL_SCAN_IN_PROGRESS = 0x0002,
    PROFILE_MATCHED = 0x0004,
    CHANNEL_OP_SCHEDULING_IN_PROGRESS = 0x0008,
    SCAN_STOP_PENDING = 0x0010,
} SEARCH_STATE;

#ifdef SUPPORT_EVENT_HANDLERS
typedef struct wlan_dc_ev_handler_info dc_ev_handler_info;
#endif /* SUPPORT_EVENT_HANDLERS */

typedef struct dc_struct {
    devh_t *dev; /* dev pointer */
    CSERV_COMPLETION_CB cb_fn;
    void *cb_arg;
    TimerHandle_t min_chdwell_timer;
    TimerHandle_t deterministic_scan_timer;

    uint32_t scan_list_index;

    uint32_t chdwell_minact_duration;
#ifdef NT_FN_WPS
    uint32_t chdwell_maxact_duration;
#endif  // NT_FN_WPS
    uint32_t chdwell_pas_duration;
    uint32_t deterministic_scan_interval;
    NT_BOOL channel_op_requested;
    SEARCH_STATE search_state;

    NT_BOOL reg_11d_enable;
    REG_NODE reg_list[MAX_REGCODES_LIST];
    REG_NODE bestreg;
    NT_REG_CODE cur_regcode; /* current reg code */
    struct ieee80211_country_ie countryIe;
    struct ieee80211_suppchan_ie suppchanIe;

    DC_SCAN_TYPE scan_type;
    DC_CHANNEL_ATTRIBUTES ch_attr[DEV_CHANNEL_NUM_MAX];
    DC_SCAN_LIST scan_list;
    SSID_PROBE_LIST ssid_probe_list;
    uint8_t channel_hint;         /* index to channel hint */
    uint32_t beacon_update_count; /* per sta beacon update cnt*/
    uint32_t cfg_scan_type;
    uint8_t profile_filter;
    SCAN_RESULT *scan_results;
    uint8_t scan_id;
#ifdef SUPPORT_EVENT_HANDLERS
    dc_ev_handler_info *ev_handler_info;
#endif /* SUPPORT_EVENT_HANDLERS */
} DC_STRUCT;

#ifdef SUPPORT_EVENT_HANDLERS
/* Enter module name to register event handler */
enum discovery_event_cb_mods {
    DISCOVERY_EVENT_CB_BT_COEX,
    DISCOVERY_EVENT_CB_MAX_EVENT_HANDLER,
};

struct wlan_dc_ev_handler_info {
    uint16_t num_ev_handlers;
    void (*evhandlers[DISCOVERY_EVENT_CB_MAX_EVENT_HANDLER])(devh_t *dev, DC_EVENT *event);
};
#endif /* SUPPORT_EVENT_HANDLERS */

#ifdef NT_FN_WPS
/* Virutal dev support:

* Device specific DC context

*/

typedef struct dc_dev_ctx {
    DC_SCAN_TYPE scan_type;                             /* per dev */
    DC_CHANNEL_ATTRIBUTES ch_attr[DEV_CHANNEL_NUM_MAX]; /* per dev */
    DC_SCAN_LIST scan_list;                             /* per dev */
    SSID_PROBE_LIST ssid_probe_list;                    /* per dev */
    uint8_t channel_hint;                               /* index to channel hint */
    uint32_t bss_filter;                                /* per dev */
    uint32_t ie_mask;                                   /* per dev */
    uint32_t beacon_update_count;                       /* per sta beacon update cnt*/
    uint32_t cfg_scan_type;
    NT_BOOL bss_reporting_policy;
    uint8_t profile_filter;
    NT_BOOL resync_beacon;
    WMI_PHY_MODE phyMode; /* current phy mode */
    DC_STRUCT *pdc_cmn_ctx;

#ifdef CONFIG_NLO
    // Network Offload Variables Used
    nt_timer network_offload_timer;
    uint32_t network_offload_timeout;
    uint32_t nFastScans;       /*Number of Scan with FastScanPeriod Interval. Once the counter becomes '0' scan uses
                                  SlowScanInterval*/
    uint32_t FastScanInterval; /*in MilliSeconds*/
    uint32_t SlowScanInterval; /*in MilliSeconds Always SlowScanInterval > FastScanInterval*/
    NT_BOOL bIsNetworkListOffloaded; /*Host Driver offloads the list */
    NT_BOOL bIsNLOProfileMatch;
#ifndef AR6002_REV74
    uint8_t maxNLOProfiles;

#endif  // AR6002_REV74

#endif  // CONFIG_NLO

#if defined(CONFIG_CHANNEL_SCHEDULER_1)

        SEARCH_STATE                   search_state;

        CSERV_COMPLETION_CB            cb_fn;

        void                          *cb_arg;

        CSERV_COMPLETION_CB            beacon_update_comp_cb;

        A_TIMER                        beacon_update_timer;

        A_TIMER                        min_chdwell_timer;

        A_TIMER                        deterministic_scan_timer;

        A_UINT32                       deterministic_scan_interval;

        A_UINT32                       chdwell_minact_duration;

        A_UINT32                       chdwell_maxact_duration;

        A_UINT32                       chdwell_pas_duration;

        A_UINT8                        chdwell_maxact_conf_by_user;

        A_UINT8                        channel_op_requested;

#endif

} DC_DEV_CTX;
#endif  // NT_FN_WPS

#ifndef CONFIG_CHANNEL_SCHEDULER
#define DC_START_MIN_CHDWELL_TIMER(pdc_struct, timeout_value)                        \
    do {                                                                             \
        nt_stop_timer((pdc_struct)->min_chdwell_timer);                              \
        nt_timer_change_time_period((pdc_struct)->min_chdwell_timer, timeout_value); \
    } while (0);

#define DC_CANCEL_MIN_CHDWELL_TIMER(pdc_struct)         \
    do {                                                \
        nt_stop_timer((pdc_struct)->min_chdwell_timer); \
    } while (0);
#endif /* CONFIG_CHANNEL_SCHEDULER */
#define DC_RESET_SSID_PROBE_INDEX(pdc_struct)    \
    do {                                         \
        (pdc_struct)->ssid_probe_list.index = 0; \
    } while (0);

#define DC_SET_CURRENT_SCAN_TYPE(pdc_struct, type) ((pdc_struct)->scan_type = (type))
#define DC_GET_CURRENT_SCAN_TYPE(pdc_struct)       ((pdc_struct)->scan_type)

#define DC_SCAN_IN_PROGRESS(pdc_struct)                   ((pdc_struct)->search_state & SEARCH_IN_PROGRESS)
#define DC_SCAN_TYPE_MULTI_CHANNEL(scan_type)             ((scan_type)&SCAN_MULTI_CHANNEL)
#define DC_SCAN_TYPE_DETERMINISTIC(scan_type)             ((scan_type)&SCAN_DETERMINISTIC)
#define DC_SCAN_PROFILE_MATCH_TERMINATED(scan_type)       ((scan_type)&SCAN_PROFILE_MATCH_TERMINATED)
#define DC_SCAN_HOME_CHANNEL_SKIP(scan_type)              ((scan_type)&SCAN_HOME_CHANNEL_SKIP)
#define DC_SCAN_CURRENT_SSID_SKIP(scan_type)              ((scan_type)&SCAN_CURRENT_SSID_SKIP)
#define DC_SCAN_ACTIVE_PROBE_DISABLED(scan_type)          ((scan_type)&SCAN_ACTIVE_PROBE_DISABLE)
#define DC_SCAN_CHANNEL_HINT_ONLY(scan_type)              ((scan_type)&SCAN_CHANNEL_HINT_ONLY)
#define DC_SCAN_ACTIVE_CHANNELS_ONLY(scan_type)           ((scan_type)&SCAN_ACTIVE_CHANNELS_ONLY)
#define DC_SCAN_TYPE_PERIODIC(scan_type)                  ((scan_type)&SCAN_PERIODIC)
#define DC_SCAN_AP_ASSISTED(scan_type)                    ((scan_type)&SCAN_AP_ASSISTED)
#define DC_SCAN_ANY_PROFILE(scan_type)                    ((scan_type)&SCAN_ANY_PROFILE)
#define DC_SCAN_DONOT_RETURN_TO_HOME_AFTERSCAN(scan_type) ((scan_type)&SCAN_DONOT_RETURN_TO_HOME_AFTERSCAN)

#define DC_PROBED_SSID_MATCH(probe_ssid, str, length) \
    (((probe_ssid)->ssid_len == (length)) && (memcmp((probe_ssid)->ssid, (str), (length)) == 0))

#define DC_PROFILE_SSID_MATCH(profile, str, length) \
    (((profile)->ssid.ssid_len == (length)) && (memcmp((profile)->ssid.ssid, (str), (length)) == 0))

#define DC_PROFILE_BSSID_MATCH(profile, macaddr) \
    (((profile)->bssid_set) && (IEEE80211_ADDR_EQ((macaddr), (profile)->bssid)))

#define DC_CHANNEL_OPPORTUNITY_REQUESTED(pdc_struct) ((pdc_struct)->channel_op_requested)

// NEUTRINO FIX-ME: dc_scan_opportunity may have to be replaced with pmChanOpRequest TBD
#define DC_SET_CHANNEL_OPPORTUNITY_REQUESTED(pdc_struct, enable) \
    do {                                                         \
        if (enable) {                                            \
            assert((pdc_struct)->channel_op_requested == FALSE); \
            (pdc_struct)->channel_op_requested = TRUE;           \
            dc_scan_opportunity((pdc_struct)->dev);              \
        } else {                                                 \
            (pdc_struct)->channel_op_requested = FALSE;          \
        }                                                        \
    } while (0)

#endif /* __DISCOVERY_INTERNAL_H__ */
