/**
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file chipset_WMI.h
 * @brief WLAN Chipset Debug Log Collection - WMI Header
 *
 * This file defines data structures, constants, and public API declarations
 * for the chipset debug log collection feature.  The feature collects WLAN
 * chipset debug statistics and sends them to the host via WMI events
 * on-demand when the host issues WMI_GET_CHIPSET_LOGGING_STATS_CMDID.
 *
 */

#ifndef _CHIPSET_WMI_H_
#define _CHIPSET_WMI_H_

#include "wifi_cmn.h"

#ifdef WLAN_CHIPSET_LOG_ENABLE

#ifndef __ATTRIB_PACK
#ifdef __GNUC__
#define __ATTRIB_PACK __attribute__((packed))
#else
#define __ATTRIB_PACK
#endif
#endif /* __ATTRIB_PACK */

#ifndef POSTPACK
#define POSTPACK				__ATTRIB_PACK
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** Chipset log version */
#define CHIPSET_LOG_VERSION                     1
//#define CHIPSET_LOG_TAG_ENABLE
//#define CHIPSET_LOG_TODO
//#define CHIPSET_LOG_LINK_ENABLE

/** Maximum number of devices (vdevs) supported for per-dev stats */
#define CHIPSET_LOG_MAX_DEV_COUNT               2

/**
 * Number of MCS indices (HT MCS 0-4) tracked in the datapath previous-snapshot
 * buffer for per-interval delta calculation.
 */
#define CHIPSET_LOG_DP_MCS_MAX                  5

/** 11b rate count: 1/2/5.5/11 Mbps */
#define CHIPSET_LOG_DP_11B_RATES                4
/** 11a/g rate count: 6/9/12/18/24/36 Mbps */
#define CHIPSET_LOG_DP_11AG_RATES               6

/* =========================================================================
 * Tag IDs for each module
 * ========================================================================= */
typedef enum {
    CHIPSET_LOG_TAG_LINK      = 1,  /**< Link quality stats tag */
    CHIPSET_LOG_TAG_DATAPATH  = 2,  /**< Datapath stats tag */
    CHIPSET_LOG_TAG_MAX
} chipset_log_tag_id_t;

/* =========================================================================
 * Core data structures
 * ========================================================================= */

/**
 * @brief Chipset log packet header
 *
 * Prepended to each chipset log stats payload sent to host.
 */
typedef struct chipset_log_header {
    uint32_t version;       /**< Log format version (CHIPSET_LOG_VERSION) */
    uint32_t total_length;  /**< Total length of the log payload in bytes */    
    uint32_t timestamp_lo;     /**< Timestamp in microseconds when log was collected */
    uint32_t timestamp_hi;
} POSTPACK chipset_log_header_t;

#if CHIPSET_LOG_TAG_ENABLE
/**
 * @brief Per-module tag header
 *
 * Prepended to each module's stats block within the log payload.
 * used_bit is set to 1 when the module has updated stats since the
 * last report. Only modules with used_bit == 1 are included in the
 * WMI event payload. After sending, used_bit is cleared to 0.
 */
typedef struct chipset_log_tag_header {
    uint32_t tag_id   : 8;   /**< Module tag ID (chipset_log_tag_id_t) */
    uint32_t used_bit : 8;   /**< 1 = module has new data; 0 = no update */
    uint32_t length   : 16;  /**< Length of this module's stats block in bytes */
} chipset_log_tag_header_t;
#endif

#ifdef CHIPSET_LOG_LINK_ENABLE

/* =========================================================================
 * Link module statistics
 * ========================================================================= */

/**
 * @brief Link module information
 *
 * Contains interface type, PHY mode, band, channel, and link speeds.
 */
typedef struct chipset_log_link_info {
#ifdef CHIPSET_LOG_TAG_ENABLE
    chipset_log_tag_header_t link_hdr;          /**< Tag header (tag=CHIPSET_LOG_TAG_LINK) */
#endif
#ifdef CHIPSET_LOG_TODO
    uint32_t ifc_type;                          /**< Interface type (e.g., STA, AP) */
    uint32_t phy_mode;                          /**< PHY mode currently in use (e.g., 11a/b/g/n/ac/ax) */
    uint32_t band;                              /**< Operating frequency band (2.4GHz, 5GHz, or 6GHz) */
    uint32_t channel;                           /**< Current operating channel */
#endif
    uint32_t rx_speed;                          /**< Current receive link speed */
    uint32_t tx_speed;                          /**< Current transmit link speed */
} POSTPACK chipset_log_link_info_t;
#endif

/* =========================================================================
 * Datapath module statistics
 * ========================================================================= */

/**
 * @brief Datapath TX statistics
 *
 * Transmit MPDU statistics collected from the data path.
 */
typedef struct chipset_log_datapath_tx {
    uint32_t tx_mcs_mpdu[CHIPSET_LOG_DP_MCS_MAX];           /**< TX MPDUs per HT-MCS index (MCS 0-4) */
    uint32_t tx_11b_mpdu[CHIPSET_LOG_DP_11B_RATES];         /**< TX MPDUs per 11b rate: [0]=1M [1]=2M [2]=5.5M [3]=11M */
    uint32_t tx_11ag_mpdu[CHIPSET_LOG_DP_11AG_RATES];       /**< TX MPDUs per 11a/g rate: [0]=6M [1]=9M [2]=12M [3]=18M [4]=24M [5]=36M */
    uint32_t tx_rts_succ_cnt;                               /**< across all TIDs */
    uint32_t tx_rts_fail_cnt;                               /**< across all TIDs */
    uint32_t tx_ppdu_cnt;
    uint32_t tx_ppdu_ack_to;
    uint32_t tx_success_frm_cnt;                            /**< across all TIDs */
    uint32_t tx_fail_cnt;                                   /**< across all TIDs */
    uint32_t tx_ack_fail_cnt;                               /**< across all TIDs */
    uint32_t tx_retry_cnt;                                  /**< across all TIDs */
    uint32_t tx_mult_retry_cnt;                             /**< across all TIDs */
} POSTPACK chipset_log_datapath_tx_t;

/**
 * @brief Datapath RX statistics
 *
 * Receive MPDU statistics collected from the data path.
 */
typedef struct chipset_log_datapath_rx {
    uint32_t rx_mcs_mpdu[CHIPSET_LOG_DP_MCS_MAX];           /**< RX MPDUs per HT-MCS index (MCS 0-4) */
    uint32_t rx_11ag_mpdu[CHIPSET_LOG_DP_11AG_RATES];       /**< RX MPDUs per 11a/g rate: [0]=6M [1]=9M [2]=12M [3]=18M [4]=24M [5]=36M */
    uint32_t rx_11b_mpdu[CHIPSET_LOG_DP_11B_RATES];         /**< RX MPDUs per 11b rate: [0]=1M [1]=2M [2]=5.5M [3]=11M */
    uint32_t rx_mpdu;
    uint32_t rx_ampdu;
    uint32_t rx_mpdu_in_ampdu;
    uint32_t dlm_err;
    uint32_t max_pktlen_fail;
#ifdef CHIPSET_LOG_TODO
    uint32_t rx_drop_mpdu;                      /**< Number of received MPDUs dropped by the data path */
#endif
} POSTPACK chipset_log_datapath_rx_t;

/**
 * @brief Datapath statistics
 *
 * TX and RX MPDU statistics collected from the data path.
 */
typedef struct chipset_log_datapath_info {
#ifdef CHIPSET_LOG_TAG_ENABLE
    chipset_log_tag_header_t dp_hdr;            /**< Tag header (tag=CHIPSET_LOG_TAG_DATAPATH) */
#endif
    chipset_log_datapath_tx_t tx;               /**< TX statistics */
    chipset_log_datapath_rx_t rx;               /**< RX statistics */
} POSTPACK chipset_log_datapath_info_t;

#ifdef CHIPSET_LOG_TODO
typedef struct chipset_log_sta_info {
    chipset_log_tag_header_t sta_hdr;           /**< Tag header (tag=CHIPSET_LOG_TAG_STA) */
    A_UINT32 rx_beacon_cnt;                     /**< Number of beacons received from the associated AP */
    A_INT32  rssi[CHIPSET_LOG_STA_RSSI_BUF_SIZE]; /**< Per-beacon RSSI samples (one per beacon, up to CHIPSET_LOG_STA_RSSI_BUF_SIZE) */
    A_UINT32 cca_busy_ratio;                    /**< CCA busy ratio */
    A_INT32  cu;                                /**< Channel utilization from QBSS IE; -1 means IE absent */
} chipset_log_sta_info_t;
#endif

/* =========================================================================
 * Combined chipset log stats structure
 * ========================================================================= */

/**
 * @brief Combined chipset log statistics
 *
 * Contains stats for all modules. Per-device modules (link, datapath, sta, sap, p2p)
 * are arrays indexed by device ID to support multiple concurrent devices.
 */
typedef struct chipset_log_stats {
#ifdef CHIPSET_LOG_LINK_ENABLE
    chipset_log_link_info_t      link_stats[CHIPSET_LOG_MAX_DEV_COUNT];  /**< Per-dev link stats (multi-entry) */
#endif
    chipset_log_datapath_info_t  datapath_stats;                         /**< Datapath stats (single entry) */
#ifdef CHIPSET_LOG_TODO
    chipset_log_sta_info_t       sta_stats;                              /**< STA stats (single entry) */
#endif
} POSTPACK chipset_log_stats_t;

#ifdef __cplusplus
}
#endif

#endif

#endif /* _CHIPSET_LOG_WMI_H_ */