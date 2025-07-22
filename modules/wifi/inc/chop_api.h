/*
Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
*/
#ifndef _CHOP_API_H_
#define _CHOP_API_H_
#include "wifi_cmn.h"
#include "wlan_dev.h"

#include "fwconfig_wlan.h"
#include "nt_flags.h"

typedef enum {
    CO_OP_START_BSS = 1,   /* Starting a BSS */
    CO_OP_JOIN_BSS = 2,    /* Connecting to a BSS */
    CO_OP_START_SCAN = 3,  /* Start a scan */
    CO_OP_RETURN_HOME = 4, /* Returning to home channel */
    CO_OP_HOST = 5,        /* Host request */
} CO_OPTYPE;

void co_cmn_init(dev_common_t *pdevCmn);
void co_cmn_deinit(dev_common_t *pDevCmn);
#ifndef CONFIG_CHANNEL_SCHEDULER
void *co_init(devh_t *dev);
void co_deinit(devh_t *dev);
#endif
nt_status_t co_change_channel(devh_t *dev, channel_t *ch, CO_OPTYPE optype);
channel_t *co_get_current_channel(devh_t *dev);
void co_set_channel_cmd(devh_t *dev, WMI_SET_CHANNEL_CMD *pCmd);
#ifdef SUPPORT_5GHZ
nt_status_t co_update_channel_list(dev_common_t *pDevCmn, uint8_t *chanlist_size);
#endif

#endif /* _CHOP_API_H_ */
