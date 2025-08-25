/*
Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
*/
#ifndef NT_EVENT_DEF
#define NT_EVENT_DEF
#include "wifi_cmn.h"
#include "wmi.h"
#include "nt_wlan.h"
#include "wlan_wmi.h"
#include "iot_wifi.h"

typedef struct {
    uint16_t type;
    uint16_t id;
    uint16_t sec_id;
    void *data;
#ifdef NT_HOSTLESS_SDK
    event evt_cb;
#endif
#ifdef NT_HOSTED_SDK
    event evt_cb1;
#endif
    uint8_t sta_mac[IEEE80211_ADDR_LEN];
#if (defined NT_FN_WUR_STA) || (defined NT_FN_WUR_AP)
    int64_t wur_data;
    int16_t wur_flag;
#endif
} WurInfoEvnt_t;
#ifdef NT_HOSTLESS_SDK
extern WurInfoEvnt_t evnt_struct_inst;
#endif
#ifdef NT_HOSTED_SDK
extern WurInfoEvnt_t hosted_evnt_struct_inst;
#endif
NT_BOOL nt_event_register(const WurInfoEvnt_t *const evt_to_register);
NT_BOOL nt_evnt_process(WIFIReturnCode_t status, event_t evnt_id, void *payload);

#endif
