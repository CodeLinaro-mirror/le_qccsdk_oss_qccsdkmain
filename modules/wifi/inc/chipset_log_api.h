/**
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file chipset_log_api.h
 * @brief WLAN Chipset Debug Log Collection - API Header
 *
 * This file defines data structures, constants, and public API declarations
 * for the chipset debug log collection feature.  The feature collects WLAN
 * chipset debug statistics and sends them to the host via WMI events
 * on-demand when the host issues WMI_GET_CHIPSET_LOGGING_STATS_CMDID.
 *
 */

#ifndef _CHIPSET_LOG_API_H_
#define _CHIPSET_LOG_API_H_

#include "wifi_cmn.h"

#ifdef WLAN_CHIPSET_LOG_ENABLE

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * Public API - core framework (chipset_log.c)
 * ========================================================================= */
void chipset_log_enable(NT_BOOL enable);
void chipset_log_init(void);
void chipset_log_deinit(void);

/**
 * @brief Check whether chipset logging is currently enabled
 *
 * Returns A_TRUE if the global chipset log context has been initialised
 * and the feature has been enabled via chipset_log_enable().
 *
 * Intended as a lightweight guard for modules (e.g. coex) that update
 * chipset log stats event-driven and want to skip work when the feature
 * is disabled.
 *
 * @return A_TRUE if chipset logging is enabled, A_FALSE otherwise
 */
NT_BOOL chipset_log_is_enabled(void);

/* =========================================================================
 * Public API - stats collection (chipset_log_stats.c)
 * ========================================================================= */
void chipset_log_collect_and_send_with_request_id(uint32_t request_id);

/* Forward declarations to avoid circular includes */
struct devh_s;

/* Link module */
/**
 * @brief Clear link info for a vdev when it stops or disconnects (set used_bit = 0)
 *
 * Must be called in two situations:
 *
 *   1. Device down (wlan_unified_vdev_stop or equivalent)
 *      The vdev is being torn down.  Zeros the entire link_stats slot and
 *      clears used_bit so the slot is excluded from subsequent WMI reports.
 *
 *   2. STA disconnect (wlan_sta_disconnect or equivalent)
 *      The STA has lost its association.  The channel info is no longer
 *      meaningful for a connected link, so the slot is cleared here too.
 *      chipset_log_link_info_update_from_wmi_chan() will repopulate the
 *      slot when the next vdev_start fires.
 *
 * Slot assignment: regular STA → slot 0, SAP / P2P GC / P2P GO → slot 1.
 *
 * @param dev  Pointer to the vdev (devh_t) that is stopping or disconnecting
 */
void chipset_log_link_info_clear(struct devh_s *dev);

/**
 * @brief Update max TX rate for a device (called per data packet TX)
 *
 * @param dev        Pointer to the vdev (devh_t)
 * @param rate_kbps  TX rate of this packet in kbps
 */
void chipset_log_link_tx_rate_update(struct devh_s *dev, uint32_t rate_kbps);

#ifdef CHIPSET_LOG_LINK_ENABLE
/**
 * @brief Update max RX rate for a device (called per data packet RX)
 *
 * @param dev_role   device role
 * @param rate_kbps  RX rate of this packet in kbps
 */
void chipset_log_link_rx_rate_update(uint8_t dev_role, uint32_t rate_kbps);
#endif

/**
 * @brief Directly update rx_mcs_mpdu counter in chipset log datapath stats.
 *
 * Called from nt_dpm_rx_sanity_check() on every received data frame.
 * Bypasses the dev->rx_rate_index_counter[] path which is only updated
 * for STA_DEVICE and STA_GC_DEVICE, ensuring all device types (including
 * AP_DEVICE) contribute to the per-MCS RX MPDU statistics.
 *
 * Rate-index to MCS mapping (HAL layout, 20 MHz):
 *   rate_index 16-23 : HT MCS 0-7 (LGI)
 *   rate_index 24-31 : HT MCS 0-7 (SGI)
 *   => mcs_idx = (rate_index - 16) % 8
 * Indices outside [16, 31] are silently ignored.
 *
 * @param rate_index  HAL rate index from bd->rateIndex
 */
void chipset_log_dp_rx_mcs_update(uint32_t rate_index);

/**
 * @brief Accumulate a received MPDU into the 11a/g per-rate RX counter.
 *
 * Maps rate_index 8-13 (6/9/12/18/24/36 Mbps) to rx_11ag_mpdu[0-5].
 * Indices outside [8, 13] are silently ignored.
 *
 * @param rate_index  HAL rate index from bd->rateIndex
 */
void chipset_log_dp_rx_11ag_update(uint32_t rate_index);

/**
 * @brief Accumulate a received MPDU into the 11b per-rate RX counter.
 *
 * Maps rate_index 0-7 (long/short preamble, same 4 speed buckets) to
 * rx_11b_mpdu[0-3] via rate_index % 4.
 * Indices outside [0, 7] are silently ignored.
 *
 * @param rate_index  HAL rate index from bd->rateIndex
 */
void chipset_log_dp_rx_11b_update(uint32_t rate_index);

void chipset_log_dump_stats (void);
void chipset_log_dump_prev (void);

#ifdef __cplusplus
}
#endif

#endif

#endif /* _CHIPSET_LOG_H_ */