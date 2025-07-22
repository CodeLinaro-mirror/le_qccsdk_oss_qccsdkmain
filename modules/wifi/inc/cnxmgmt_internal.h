/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __CNXMGMT_INTERNAL_H__
#define __CNXMGMT_INTERNAL_H__

#include "osapi.h"
#include "mlme_api.h"
#include "com_api.h"
#include "pm_api.h"

#define VALID_HANDOFF_TRIGGER_SOURCES                                                                       \
    (CONNECTION_STATE_METRIC | BMISS_EVENT | DISCONNECT_EVENT | LOW_RSSI_EVENT | PERIODIC_SEARCH_COMPLETE | \
     BSS_CHANNEL_CHANGE | HOST_REQUEST | RECONNECT_EVENT | CONNECT_REQUEST)

#define GET_DEVICE(pcm_struct) pcm_struct->dev
typedef struct cm_struct {
    devh_t *dev;
    uint8_t reassocMode; /* do/dont disassoc when reassoc */

    uint32_t connect_timeout;
    TimerHandle_t connect_timer;

    uint32_t connect_type;
    uint32_t ho_trigger_source;

    uint8_t prev_ap_bssid[IEEE80211_ADDR_LEN];
#ifdef NT_FN_DEBUG_STATS
    COM_STATS com_stats;
#endif  // NT_FN_DEBUG_STATS
    uint8_t conn_state;
    bss_t *current_bss;
    uint32_t disconn_reason;

    uint16_t protoReasonStatus;

    uint8_t conn_retry_count;
    uint8_t max_conn_retry_count; /*max connection retry, value read from dev config*/
#ifdef CONFIG_CHANNEL_SCHEDULER
    NT_BOOL high_pri_ch_req_adjusted; /* if the tbbt req has been readjusted for this connection */
#endif
} CM_STRUCT;

#define CM_DISABLE_CONNECT_TIMEOUT(pcm_struct)      \
    do {                                            \
        nt_stop_timer((pcm_struct)->connect_timer); \
    } while (0)

#define CM_ENABLE_CONNECT_TIMEOUT(pcm_struct)            \
    do {                                                 \
        if ((pcm_struct)->connect_timeout) {             \
            nt_start_timer((pcm_struct)->connect_timer); \
        }                                                \
    } while (0)

#endif /* __CNXMGMT_INTERNAL_H__ */
