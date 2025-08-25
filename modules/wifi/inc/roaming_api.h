/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _RO_API_H_
#define _RO_API_H_

#ifdef NT_FN_ROAMING

/* Forward declarations */
struct bss_s;
struct devh_s;
struct periodic_search_params_s;

typedef enum {
    RO_FOREGROUND_SEARCH = 0,
    RO_BACKGROUND_SEARCH,
    RO_SEARCH_PATTERN_NUM_MAX,
} RO_SEARCH_PATTERN;

typedef enum {
    RO_ROAM_METRIC_WEIGHT_ID = 0,
    RO_FG_SEARCH_INTERVAL_ID,
    RO_ROAM_MODE_ID,
    RO_BG_SCAN_FLAG_ID,
} RO_CONFIG_ID;

typedef enum {
    RO_RESET = 0x01,
    RO_ENABLE = 0x02,
    RO_CONNECT = 0x04,
    RO_DISABLE_ALL = 0x08,
} RO_CONFIG_MODE;

typedef enum {
    RO_BG_RSSI_TRIGGER = 0x01,
    RO_BG_RATE_TRIGGER = 0x02,
    RO_BG_RATE_MIXED = (RO_BG_RSSI_TRIGGER | RO_BG_RATE_TRIGGER),
    RO_BG_PERIODIC = 0x04,
} RO_BG_TRIGGER;

/* Foreground search */
#define RO_RESET_FOREGROUND_SEARCH(dev)        ro_set_periodic_search(dev, RO_FOREGROUND_SEARCH, RO_RESET)
#define RO_ENABLE_FOREGROUND_SEARCH(dev, mode) ro_set_periodic_search(dev, RO_FOREGROUND_SEARCH, RO_ENABLE | mode)

/* Background search */
#define RO_RESET_BACKGROUND_SEARCH(dev) ro_set_periodic_search(dev, RO_BACKGROUND_SEARCH, RO_RESET)

#define RO_ENABLE_BACKGROUND_SEARCH(dev)                              \
    do {                                                              \
        ro_set_periodic_search(dev, RO_BACKGROUND_SEARCH, RO_ENABLE); \
    } while (0);

#define RO_DISABLE_BACKGROUND_SEARCH(dev)                                  \
    do {                                                                   \
        ro_set_periodic_search(dev, RO_BACKGROUND_SEARCH, RO_DISABLE_ALL); \
    } while (0);

#define RO_DISABLE_PERIODIC_SEARCH(dev) ro_set_periodic_search(dev, 0, 0)

#define RO_DISABLE_ALL_PERIODIC_SEARCH(dev) ro_set_periodic_search(dev, 0, RO_DISABLE_ALL)

#define RO_SET_ROAM_METRIC_WEIGHT(dev, weight)                 \
    do {                                                       \
        uint32_t config = (weight);                            \
        ro_set_config(dev, RO_ROAM_METRIC_WEIGHT_ID, &config); \
    } while (0);

#define RO_SET_ROAM_MODE(dev, mode)                   \
    do {                                              \
        uint8_t config = (mode);                      \
        ro_set_config(dev, RO_ROAM_MODE_ID, &config); \
    } while (0);

#define RO_SET_ROAM_CONTROL_FLAGS(dev, flags)                  \
    do {                                                       \
        uint32_t config = (flags);                             \
        ro_set_config(dev, RO_ROAM_CONTROL_FLAGS_ID, &config); \
    } while (0);

#define RO_SET_BG_SCAN_FLAG(dev, flag)                   \
    do {                                                 \
        NT_BOOL config = (flag);                         \
        ro_set_config(dev, RO_BG_SCAN_FLAG_ID, &config); \
    } while (0);

#define RO_GET_FG_SEARCH_INTERVAL(dev, interval)                         \
    do {                                                                 \
        ro_get_config(dev, RO_FG_SEARCH_INTERVAL_ID, (void *)&interval); \
    } while (0);

#define RO_GET_ROAM_MODE(dev, mode)                         \
    do {                                                    \
        ro_get_config(dev, RO_ROAM_MODE_ID, (void *)&mode); \
    } while (0);

#define RO_GET_BG_SCAN_ENABLE_FLAG(dev, mode)                    \
    do {                                                         \
        ro_get_config(dev, RO_BG_SCAN_ENABLE_ID, (void *)&mode); \
    } while (0);

typedef struct roam_metrics_s {
    uint8_t rssi_avg;           /* Average as in hardware */
    uint8_t rssi_last;          /* Only used for reporting purposes */
    uint8_t cs_sec_mode_metric; /* security mode based metric */
    uint8_t cs_ps_mode_metric;  /* power save metric */
    uint8_t cs_prot_metric;     /* protection related metric */
    uint8_t bss_age;            /* bss ageing counter before its removed from roaming table */
    uint8_t roam_util;          /* roaming util used for bss selection */
} ROAM_METRICS;

#define BSS_HAS_VALID_PMKID(bss)     (bss)->ni_roam_metrics.pmkid_valid
#define CONNECTION_STATE_METRIC_MASK 0x01
#define RSSI_METRIC_MASK             0x02
#define PMKID_METRIC_MASK            0x04
#define HOST_REQUEST_METRIC_MASK     0x08

#define METRIC_MASK_DEFAULT \
    (CONNECTION_STATE_METRIC_MASK | RSSI_METRIC_MASK | PMKID_METRIC_MASK | HOST_REQUEST_METRIC_MASK)

#ifdef CONFIG_WLAN_LOWRSSI_RO_SCAN
#define BSS_UTIL_LOWRSSI 11
#endif

// Security Utility - in increasing order of preference
// Pure mode is preferred over mixed mode
#define BSS_UTIL_WPA           1
#define BSS_UTIL_MIX_WPA2_WPA  2
#define BSS_UTIL_WPA2          3
#define BSS_UTIL_MIX_WPA3_WPA2 4
#define BSS_UTIL_WPA3          5

// Power Save Capability preference
#define BSS_UTIL_UAPSD 1
#define BSS_UTIL_WNM   2
#define BSS_UTIL_WUR   3
#define BSS_UTIL_FTM   4
#define BSS_UTIL_TWT   5

// Protection preference
#define BSS_UTIL_PROT_ON  1
#define BSS_UTIL_PROT_OFF 2

/*
 * Although we expose the ROAM_METRICS structure to the bss structure, we
 * would not want other modules to directly operate on that so we are
 * providing this macro if not API to get the value
 */
#define RO_GET_RSSI(_bss) (_bss)->ni_roam_metrics.rssi_avg
#ifdef CONFIG_WLAN_LOWRSSI_RO_SCAN

#define RO_DEFAULT_LRSSI_SCAN_PERIOD           (20 * 1000) /* secs   */
#define RO_DEFAULT_LRSSI_SCAN_THRESHOLD        22          /* rssi */
#define RO_DEFAULT_LRSSI_ROAM_THRESHOLD        20          /* rssi */
#define RO_DEFAULT_LRSSI_ROAM_FLOOR            80          /* rssi */
#define RO_DEFAULT_LRSSI_SCAN_POLLING_INTERVAL 5           /* beacon interval */
#define RO_DEFAULT_SCAN_THRESHOLD_PERCENT      50          /* low rssi scan threshold */
/*
 *  * Following macros are used to enable/disable low rssi SCAN
 *   * and roaming functionality *
 *    *
 *     */
#define RO_LOWRSSI_SCAN_REQ        0x01
#define RO_LOWRSSI_ROAM_REQ        0x02
#define RO_LOWRSSI_SCAN_INPROGRESS 0x04

#endif

/*
 * METRIC_AVERAGE: BSS RSSI Averaging Control.
 * Currently there are two types.
 * (a) The RSSI of the BSS should be averaged before use (METRIC_AVERAGE)
 * (b) Do not average the RSSI before use (METRIC_RESET).
 *
 * Most of the times (a) above would be used. (b)  is used when there is
 * a beacon miss and the directed probe request didnot elicit an ACK.
 */
typedef enum {
    METRIC_RESET,
    METRIC_UPDATE,
    METRIC_AVERAGE,
} METRIC_MODIFY_FLAG;

void *ro_init(struct devh_s *dev);
void ro_deinit(void *pro_struct);
void ro_set_scan_param(struct devh_s *dev);
void nt_ro_init_bg_scan(struct devh_s *dev);
struct bss_s *ro_get_next_roam_candidate(struct devh_s *dev);
void ro_periodic_search_callback(void *arg, nt_status_t status);
void ro_periodic_search_timeout(TimerHandle_t thandle);
void ro_configure_foreground_search_params(struct devh_s *dev, uint32_t scan_type, uint16_t search_interval);
void ro_configure_background_search_params(struct devh_s *dev, uint32_t scan_type, uint8_t full_scan_freq,
                                           uint16_t search_interval, uint8_t rssi_min, uint8_t rssi_max,
                                           uint8_t rate_min, uint8_t rate_max);
void ro_set_periodic_search(struct devh_s *dev, uint32_t search_type, uint32_t mode);
void ro_connect_event_notify(struct devh_s *dev, NT_BOOL connected);

#ifdef CONFIG_WLAN_LOWRSSI_RO_SCAN
void ro_set_lowrssi_search(struct devh_s *, uint32_t);
uint8_t ro_get_lowrssi_scan_status(struct devh_s *);
void ro_lowrssi_scan_clr_flags(struct devh_s *);
void ro_update_low_rssi_scan_params(struct devh_s *, WMI_LOWRSSI_SCAN_PARAMS);
NT_BOOL ro_get_lowrssi_scan_allowed(struct devh_s *);
void ro_lowrssi_scan_timeout(TimerHandle_t thandle, void *arg);
#endif

void ro_update_age_metric(struct devh_s *dev, struct bss_s *bss, int32_t value, METRIC_MODIFY_FLAG flag);
void ro_update_rssi_metric(struct devh_s *dev, struct bss_s *bss, uint8_t rssi, METRIC_MODIFY_FLAG flag);
void ro_trigger_bg_scan(struct devh_s *dev, struct bss_s *bss, RO_BG_TRIGGER trigger, uint8_t cur_val);
uint32_t ro_calculate_roam_utility(struct bss_s *bss);
struct bss_s *ro_get_best_roaming_candidate(struct devh_s *dev);
void ro_update_bss_roaming_metric(struct bss_s *bss);
struct bss_s *ro_bss_lookup(struct devh_s *dev, const uint8_t *macaddr);
struct bss_s *ro_bss_alloc(struct devh_s *dev);
void ro_add_bss_to_roam_table(struct devh_s *dev, struct bss_s *bss);
nt_status_t ro_remove_bss(struct devh_s *dev, struct bss_s *bss);
void ro_flush_roam_table(struct devh_s *dev);
uint32_t ro_get_roam_table(struct devh_s *dev, struct bss_s *bss_list[]);
void ro_print_roam_table(struct devh_s *dev);
void ro_set_neighbor_info(struct devh_s *dev, uint8_t version, uint8_t num_ap, void *ap_info);
void *ro_get_addr(struct devh_s *dev, RO_CONFIG_ID id, uint32_t *length);

void ro_set_config(struct devh_s *dev, RO_CONFIG_ID id, void *config);
uint32_t ro_get_config(struct devh_s *dev, RO_CONFIG_ID id, void *config);

#ifdef WLAN_CONFIG_RRM
void ro_neighbor_report_request_callback(void *arg, nt_status_t status);
#endif /* WLAN_CONFIG_RRM */

#endif  // NT_FN_ROAMING

#endif /* _RO_API_H_ */
