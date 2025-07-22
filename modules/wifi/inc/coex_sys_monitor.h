/*========================================================================
Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
*
* @file coex_sys_monitor.h
* @brief Coex sys monitor functions and struct definitions
*========================================================================*/
#ifndef _COEX_SYS_MONITOR_H_
#define _COEX_SYS_MONITOR_H_
/* Standard includes. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "nt_osal.h"

#include "fwconfig_wlan.h"
#include "nt_flags.h"
#include "nt_common.h"

#if defined(SUPPORT_COEX)
#include "coex_wlan.h"
#include "wlan_dev.h"

#define COEX_WLAN_DATA_INACTIVITY_THRESHOLD (200)
#define COEX_TXRX_IDLE_THRESHOLD_FREERUN    COEX_WLAN_DATA_INACTIVITY_THRESHOLD /* In milliseconds. */
#define COEX_TXRX_IDLE_THRESHOLD_STATIC_PM  COEX_WLAN_DATA_INACTIVITY_THRESHOLD /* In milliseconds. */

void coex_init_rssi_rate_threshold(uint8_t rssi_type, uint32_t min_rx_rate, uint32_t max_rx_rate,
                                   uint8_t num_mpdus_below_min_rx_rate, uint8_t num_mpdus_above_max_rx_rate,
                                   uint32_t frame_length_threshold);

void coex_init_phy_rate_threshold(uint8_t phy_rate, uint32_t min_rx_rate, uint32_t max_rx_rate,
                                  uint8_t num_mpdus_below_min_rx_rate, uint8_t num_mpdus_above_max_rx_rate,
                                  uint32_t frame_length_threshold);

void coex_rrm_system_update(void);
void coex_check_and_update_rx_rate(bss_t *peer, uint32_t rx_rate, uint32_t frame_length);
void coex_rrm_cb(WMI_COMMAND_ID event, uint32_t last_rxrate);
void coex_set_rrm_threshold(uint8_t rssi_type);
void coex_reset_rrm_threshold(void);
void coex_update_rrm_threshold(tCoexPeerRxThresh *rrm_threshold);
void coex_wlan_update_num_2g_vdev_with_bmiss(void);
void coex_bmiss_monitor(devh_t *dev, uint8_t beacon_event);
void coex_set_bmiss_threshold(uint32_t value);

#endif
#endif
