/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _DISCOVERY_H_
#define _DISCOVERY_H_

/*
 * DC_SCAN_PRIORITY is an 8-bit bitmap of the scan priority of a channel
 */
typedef enum {
    DEFAULT_SCPRI = 0x01,
    POPULAR_SCPRI = 0x02,
    SSIDS_SCPRI = 0x04,
    PROF_SCPRI = 0x08,
    DISABLE_SCPRI = 0x10,
} DC_SCAN_PRIORITY;

/* The following search type construct can be used to manipulate the behavior of the search module based on different
 * bits set */
typedef enum {
    SCAN_RESET = 0,
    SCAN_ALL = (DEFAULT_SCPRI | POPULAR_SCPRI | SSIDS_SCPRI | PROF_SCPRI),
    SCAN_SSIDS = (SSIDS_SCPRI | PROF_SCPRI),
    SCAN_MULTI_CHANNEL = 0x000100,
    SCAN_DETERMINISTIC = 0x000200,
    SCAN_PROFILE_MATCH_TERMINATED = 0x000400,
    SCAN_HOME_CHANNEL_SKIP = 0x000800,
    SCAN_CHANNEL_LIST_CONTINUE = 0x001000,
    SCAN_CURRENT_SSID_SKIP = 0x002000,
    SCAN_ACTIVE_PROBE_DISABLE = 0x004000,
    SCAN_CHANNEL_HINT_ONLY = 0x008000,
    SCAN_ACTIVE_CHANNELS_ONLY = 0x010000,
    SCAN_PERIODIC = 0x020000,
    SCAN_TRIGGER_RSSI = 0x040000,
    SCAN_TRIGGER_RATE = 0x080000,
    SCAN_AP_ASSISTED = 0x100000,
    SCAN_ANY_PROFILE = 0x200000,
    SCAN_DONOT_RETURN_TO_HOME_AFTERSCAN = 0x400000,
    SCAN_SPECIFIC_SSID = 0x800000,
    SCAN_LONG_DURATION = 0x1000000
} DC_SCAN_TYPE;

typedef enum {
    BSS_REPORTING_DEFAULT = 0x0,
    EXCLUDE_NON_SCAN_RESULTS = 0x1, /* Exclude results outside of scan */
} DC_BSS_REPORTING_POLICY;

typedef enum {
    DC_IGNORE_WPAx_GROUP_CIPHER = 0x01,
    DC_IGNORE_AAC_BEACON = 0x02,
    DC_CSA_FOLLOW_BSS = 0x04,
} DC_PROFILE_FILTER;

#ifdef SUPPORT_EVENT_HANDLERS
enum dc_event_type {
    DC_EVENT_STARTED,
    DC_EVENT_COMPLETED,
    DC_EVENT_FOREIGN_CHANNEL,
    DC_EVENT_FOREIGN_CHANNEL_EXIT,
};

typedef struct _dc_event {
    uint8_t type;
    uint32_t chan_freq;
    uint32_t dev_id;
    DC_SCAN_TYPE scan_type;
    uint32_t dwell_time;
} DC_EVENT;
#endif /* SUPPORT_EVENT_HANDLERS */

#define DEFAULT_DC_PROFILE_FILTER (DC_CSA_FOLLOW_BSS)

#endif /* _DISCOVERY_H_ */
