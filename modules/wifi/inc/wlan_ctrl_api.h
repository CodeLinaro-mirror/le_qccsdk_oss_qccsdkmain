/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _WLAN_CTRL_API_H
#define _WLAN_CTRL_API_H

#include "wlan_dev.h"
#include "nt_common.h"

void wlan_ctrl_attach(void);

/*
 * We can register callback handler for particular interrupt cause
 * only allow one module to do registration for one interrupt source
 */

#if defined(AR6002_REV66) || defined(AR6002_REV67) || defined(AR6002_REV68) || defined(AR6002_REV7)
nt_status_t _wlan_send_pspoll_tbd(devh_t *devh, uint8_t wmmAC, uint32_t cb_flags, uint16_t sendCompId,
                                  uint32_t discardTime);

nt_status_t wlan_send_cts_to_self_tbd(devh_t *devh, uint32_t cb_flags, uint16_t sendCompId, uint16_t dur,
                                      uint32_t discardTime);
#endif
nt_status_t wlan_send_pspoll(devh_t *devh, uint8_t wmmAC, uint32_t cb_flags, uint16_t sendCompId);

nt_status_t wlan_send_cts_to_self(devh_t *devh, uint32_t cb_flags, uint16_t sendCompId, uint16_t dur);

nt_status_t wlan_send_bar(devh_t *devh, conn_t *conn, uint32_t cb_flags, uint16_t sendCompId, uint8_t tid,
                          uint16_t seq_num);

nt_status_t wlan_send_cf_end(devh_t *devh, uint32_t cb_flags, uint16_t sendCompId);

#endif /* _WLAN_CTRL_API_H */
