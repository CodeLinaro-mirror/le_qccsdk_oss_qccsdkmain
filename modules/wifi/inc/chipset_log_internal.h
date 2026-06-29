/**
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once

#include "wifi_cmn.h"

#ifdef WLAN_CHIPSET_LOG_ENABLE

#include "chipset_log_api.h"
#include "wmi.h"

//#define CHIPSET_LOG_REG_DUMP_ENABLE
//#define CHIPSET_LOG_STATUS_PREV_DUMP_ENABLE
//#define CHIPSET_LOG_STATUS_STATS_DUMP_ENABLE

#define RATE_INDEX_MIN1 16
#define RATE_INDEX_MAX1 (RATE_INDEX_MIN1+CHIPSET_LOG_DP_MCS_MAX-1)
#define RATE_INDEX_MIN2 24
#define RATE_INDEX_MAX2 (RATE_INDEX_MIN2+CHIPSET_LOG_DP_MCS_MAX-1)

/** 11b rate indices: 0-3 long preamble, 4-7 short preamble (same 4 speed buckets) */
#define RATE_INDEX_11B_MAX   7u
/** 11a/g rate indices covering 6~36 Mbps */
#define RATE_INDEX_11AG_MIN  8u
#define RATE_INDEX_11AG_MAX  13u

/** Number of VO rate-index slots returned by hal_tpe_sta_vo_wifi_stats_get() */
#define NT_DP_CHIPSET_LOG_VO_RATE_SLOTS  3

typedef struct chipset_log_sleep_preserve_raw {
    uint32_t sleep_cnt;
    uint32_t rx_mpdu;
    uint32_t rx_ampdu;
    uint32_t rx_mpdu_in_ampdu;
	uint32_t dlm_err;
	uint32_t max_pktlen_fail;
} chipset_log_sleep_preserve_raw_t;

typedef struct hal_tpe_sta_wifi_iface_stats {
	uint32_t tx_rts_succ_cnt;   /**< sum of across all TIDs */
	uint32_t tx_rts_fail_cnt;   /**< sum of across all TIDs */
	uint32_t tx_ppdu_cnt;
	uint32_t tx_ppdu_ack_to;
#if CHIP_THEMISTO
	int32_t rssi_ack;
#endif
} hal_wifi_iface_stats_t;

typedef struct chipset_log_wmm_stats {
    uint32_t tx_success_frm_cnt; /**< sum of across all TIDs */
    uint32_t tx_fail_cnt;        /**< sum of across all TIDs */
    uint32_t tx_retry_cnt;       /**< sum of across all TIDs */
    uint32_t tx_mult_retry_cnt;  /**< sum of across all TIDs */
    uint32_t tx_ack_fail_cnt;    /**< sum of across all TIDs */
} chipset_log_wmm_stats_t;

typedef struct hal_vo_wifi_stats {
    uint32_t rx_mpdu;
    uint32_t rx_ampdu;
    uint32_t rx_mpdu_in_ampdu;
    uint32_t dlm_err; //rx
    uint32_t max_pktlen_fail; //rx
    uint32_t tx_mpdu_responded_20m[NT_DP_CHIPSET_LOG_VO_RATE_SLOTS];
#if CHIP_THEMISTO
    uint32_t tx_mpdu_responded_40m[NT_DP_CHIPSET_LOG_VO_RATE_SLOTS];
#endif
    uint32_t rate_index_20MHZ[NT_DP_CHIPSET_LOG_VO_RATE_SLOTS]; //tx
#if CHIP_THEMISTO
    uint32_t rate_index_40MHZ[NT_DP_CHIPSET_LOG_VO_RATE_SLOTS];
#endif
    NT_BOOL device_found;
} hal_vo_wifi_stats_t;

/** Raw TX statistics snapshot collected from HAL TPE for chipset log. */
typedef struct {
    uint32_t tx_rts_succ_cnt;   /**< across all TIDs */
    uint32_t tx_rts_fail_cnt;   /**< across all TIDs */
    uint32_t tx_ppdu_cnt;
    uint32_t tx_ppdu_ack_to;
    uint32_t tx_success_frm_cnt; /**< across all TIDs */
    uint32_t tx_fail_cnt;        /**< across all TIDs */
    uint32_t tx_ack_fail_cnt;    /**< across all TIDs */
    uint32_t tx_retry_cnt;       /**< across all TIDs */
    uint32_t tx_mult_retry_cnt;  /**< across all TIDs */
#ifdef CHIPSET_LOG_LINK_ENABLE
    uint32_t max_speed_kbps[CHIPSET_LOG_MAX_DEV_COUNT];
#endif
    /**
     * Per-MCS TX MPDU counts pre-computed inside nt_dp_chipset_log_tx_stats_get().
     *
     * Rate-index → MCS mapping (20 MHz HAL layout):
     *   rate_index  0-15 : legacy / non-HT  (skipped)
     *   rate_index 16-23 : HT MCS 0-7 (LGI)  → mcs_idx = rate_index - 16
     *   rate_index 24-31 : HT MCS 0-7 (SGI)  → mcs_idx = rate_index - 24
     *   => mcs_idx = (rate_index - 16) % 8
     *
     * Counts are aggregated across all connected STA-type vdevs so that
     * multi-device modes (STA + P2P GC) are handled correctly.
     */
    uint32_t tx_mcs_mpdu[CHIPSET_LOG_DP_MCS_MAX]; //tx_mpdu_responded_20m
    uint32_t tx_11b_mpdu[CHIPSET_LOG_DP_11B_RATES]; /**< per 11b speed bucket: [0]=1M [1]=2M [2]=5.5M [3]=11M */
    uint32_t tx_11ag_mpdu[CHIPSET_LOG_DP_11AG_RATES]; /**< per 11a/g speed bucket: [0]=6M [1]=9M [2]=12M [3]=18M [4]=24M [5]=36M */
    /**
     * Set to A_TRUE when at least one connected device was found in the
     * device loop.  Used by chipset_log_datapath_collect() to guard
     * dp_prev updates: if no device was connected, dp_prev must NOT be
     * updated to 0, otherwise the next connected-cycle delta would equal
     * the full cumulative total instead of the per-interval delta.
     */
    NT_BOOL device_found;
} nt_dp_chipset_log_tx_raw_t;

/**
 * @brief Previous datapath stats snapshot for per-interval delta calculation.
 *
 * Stores only the cumulative counter fields that require subtraction to
 * produce per-interval deltas.  This replaces the full
 * chipset_log_stats_t::prev_stats that was previously kept in
 * chipset_log_ctx_t; all other modules either use static variables
 * (STA CCA) or do not need a previous snapshot at all.
 *
 * TX fields mirror chipset_log_datapath_tx_t (scalar counters only;
 * tx_mcs_mpdu is limited to CHIPSET_LOG_DP_MCS_MAX = 8 HT MCS entries).
 * RX fields mirror the cumulative HAL RXP counters read by
 * chipset_log_rx_raw_stats_get() (rx_drop_mpdu is NOT stored here because
 * it comes from a non-cumulative DPM counter).
 * rx_mcs_mpdu IS stored here for per-interval delta calculation, consistent
 * with rx_ppdu / rx_mpdu / rx_dlm_err / rx_max_pktlen_fail.
 */
typedef struct chipset_log_dp_prev {
    /* TX cumulative counters (previous snapshot) */
    uint32_t tx_rts_succ_cnt;   /**< across all TIDs */
    uint32_t tx_rts_fail_cnt;   /**< across all TIDs */
    uint32_t tx_ppdu_cnt;
    uint32_t tx_ppdu_ack_to;
    uint32_t tx_success_frm_cnt; /**< across all TIDs */
    uint32_t tx_fail_cnt;        /**< across all TIDs */
    uint32_t tx_ack_fail_cnt;    /**< across all TIDs */
    uint32_t tx_retry_cnt;       /**< across all TIDs */
    uint32_t tx_mult_retry_cnt;  /**< across all TIDs */
    uint32_t tx_mcs_mpdu[CHIPSET_LOG_DP_MCS_MAX];   /**< Previous per-MCS TX MPDU counts (HT MCS 0-4) */
    uint32_t tx_11b_mpdu[CHIPSET_LOG_DP_11B_RATES]; /**< Previous 11b TX MPDU counts per speed bucket */
    uint32_t tx_11ag_mpdu[CHIPSET_LOG_DP_11AG_RATES]; /**< Previous 11a/g TX MPDU counts per speed bucket */
    /* RX cumulative counters (previous snapshot) */
    uint32_t rx_mpdu;
    uint32_t rx_ampdu;
    uint32_t rx_mpdu_in_ampdu;
    uint32_t dlm_err;
    uint32_t max_pktlen_fail;
} chipset_log_dp_prev_t;

#ifdef CHIPSET_LOG_TODO
/**
 * @brief STA module per-cycle state for RSSI collection and CCA delta.
 *
 * Replaces the file-scope static variables that were previously declared
 * inside chipset_log_stats.c:
 *   static A_UINT32 s_sta_rssi_count
 *   static A_UINT32 s_sta_prev_cca_busy_usec
 *   static A_UINT32 s_sta_prev_tx_frm_usec
 *   static A_UINT32 s_sta_prev_rx_frm_usec
 *   static A_UINT32 s_sta_prev_usec_cnt
 *
 * Embedding them in chipset_log_ctx_t makes the state visible to the
 * whole chipset log subsystem and avoids hidden static state.
 *
 * rssi_count is reset to 0 by chipset_log_sta_stats_reset() after each
 * WMI event is sent.  The prev_* CCA snapshots are NOT reset there; they
 * are preserved so that the next chipset_log_sta_collect() call can
 * compute a correct per-interval delta.
 */
typedef struct chipset_log_sta_prev {
    uint32_t rssi_count;          /**< RSSI samples collected this cycle (0..CHIPSET_LOG_STA_RSSI_BUF_SIZE) */
    uint32_t cca_busy_usec;       /**< Previous cca_busy_usec snapshot for CCA delta */
    uint32_t tx_frm_usec;         /**< Previous tx_frm_usec snapshot for CU delta */
    uint32_t rx_frm_usec;         /**< Previous rx_frm_usec snapshot for CU delta */
    uint32_t usec_cnt;            /**< Previous usec_cnt snapshot for CCA/CU delta */
    uint32_t cca_busy_usec_cache; /**< Cached cca_busy_usec total preserved across sleep */
    uint32_t usec_cnt_cache;      /**< Cached usec_cnt total preserved across sleep */
} chipset_log_sta_prev_t;

/* =========================================================================
 * Interface type enumeration
 * ========================================================================= */

typedef enum {
    IFACE_TYPE_STA      = 0,  /**< Station (STA) */
    IFACE_TYPE_P2P_GO   = 1,  /**< P2P Group Owner */
    IFACE_TYPE_P2P_GC   = 2,  /**< P2P Group Client */
    IFACE_TYPE_AWARE    = 3,  /**< NAN/Aware */
    IFACE_TYPE_MHS      = 4,  /**< Mobile Hotspot (SoftAP) */
    IFACE_TYPE_TDLS     = 5,  /**< TDLS */
} iface_type_t;

#endif

/* =========================================================================
 * Chipset log context (internal state)
 * ========================================================================= */

/**
 * Overhead (bytes) reserved at the front of wmi_buf for WMI/HTC transport
 * headers and the fixed_param TLV:
 *   WMI_CMD_HDR (4) + HTC_FRAME_HDR (8) + fixed_param TLV (~32) + alignment.
 * 64 bytes is a generous upper bound that avoids including WMI headers here.
 */
#define CHIPSET_LOG_WMI_HDR_OVERHEAD    64u

/**
 * Maximum size of the pre-allocated WMI event buffer stored in
 * chipset_log_ctx_t::wmi_buf.  Sized to hold the worst-case payload
 * (all modules reporting simultaneously):
 *   transport overhead + chipset_log_header_t + full chipset_log_stats_t
 */
#define CHIPSET_LOG_WMI_BUF_SIZE \
    (CHIPSET_LOG_WMI_HDR_OVERHEAD + \
     sizeof(chipset_log_header_t) + \
     sizeof(chipset_log_stats_t))

/**
 * @brief Chipset log module context
 *
 * Holds the current stats, a compact previous-snapshot for datapath delta
 * calculation, module state, and a pre-allocated WMI event buffer.
 */
typedef struct chipset_log_ctx {
    WMI_GET_CHIPSET_LOGGING_STATS_EVT wmi_stats; /**< WMI event payload (request_id, payload_len, header, stats) */
    chipset_log_dp_prev_t  dp_prev;     /**< Previous datapath snapshot for delta calculation */
#ifdef CHIPSET_LOG_TODO
    chipset_log_sta_prev_t sta_prev;    /**< STA per-cycle RSSI count and CCA previous snapshots */
#endif
    NT_BOOL                 enabled;     /**< Feature enabled flag */

#ifdef CHIPSET_LOG_TODO
    /**
     * Pre-allocated WMI event buffer used by chipset_log_wmi_send_event().
     * Avoids a per-event A_MALLOC / A_FREE.  Layout when in use:
     *   [WMI_CMD_HDR][HTC_FRAME_HDR][fixed_param TLV][chipset_log_header_t][payload]
     * The buffer is allocated once (as part of chipset_log_ctx_t) and reused
     * on every on-demand WMI stats request.  The WMI_EVT_CLASS_DIRECT_BUFFER
     * completion callback is a no-op because the buffer lifetime is tied to
     * the context.
     */
    uint8_t              wmi_buf[CHIPSET_LOG_WMI_BUF_SIZE]; /**< Pre-allocated WMI event buffer */
#endif
} chipset_log_ctx_t;

#endif
