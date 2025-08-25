/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

//
// This file contains the internal structures and definitions used by
// the beacon module.
//
//

#ifndef __WLAN_BEACON_H_
#define __WLAN_BEACON_H_

#include "fwconfig_wlan.h"
#include "nt_flags.h"
#include "wlan_beacon_api.h"

typedef struct tbtt_estimate_s {
#ifdef WLAN_BMPS_TBTT_DEBUG
    uint64_t last_tbtt_ret;            /* last TBTT computed */
    uint64_t last_dtim_tbtt_ret;       /* last DTIM TBTT computed */
    uint64_t last_tsf;                 /* TSF when last TBTT was computed */
    uint64_t last_tsf_dtim;            /* TSF when last DTIM TBTT was computed */
#endif                                 /* WLAN_BMPS_TBTT_DEBUG */
    uint64_t tbtt_estimate_us;         /* computed tbtt estimate from beacon */
    uint64_t tbtt_estimate_us_history; /* computed tbtt estimate from beacon */
    uint16_t dtim_count;               /* dtim count at tbtt sync from beacon */
    uint16_t dtim_period;              /* dtim period at tbtt sync from beacon */
    uint16_t bcn_intval_tu;            /* beacon interval at tbtt sync from beacon */
    bool tbtt_sync_is_needed;          /* whether tbtt is to be synced */
#ifdef WLAN_BMPS_TBTT_DEBUG
    uint8_t last_status_ret; /* return for last TBTT computation */
#endif                       /* WLAN_BMPS_TBTT_DEBUG */
} tbtt_estimate_t;

typedef struct wlan_beacon_struct {
    uint8_t *bcnBuf;                /* beacon buffer pointer */
    uint16_t userListenIntervalT;   /* User configured listen interval in TU */
    uint16_t userListenIntervalB;   /* User configured listen interval in beacons */
    uint16_t curListenIntervalB;    /* current listen interval in beacons */
    uint16_t curBmissB;             /* Current BMISS in beacon count */
    uint8_t xbeacon_miss_threshold; /**< global variable for holding beacon miss count */
#ifdef SUPPORT_COEX
    uint8_t pre_bmiss_threshold; /* 75% of the xbeacon_miss_threshold */
    uint8_t pre_bmiss_detected;
#endif
    TimerHandle_t tbttTmrHndlr;

    uint16_t modified_bcn_interval;        /*	tbbt timer ,value read from dev config*/
    uint8_t bcn_rx_tbtt_tmr_align_pending; /* Align TBTT timer with fixed offset from Bcn rx */
    tbtt_estimate_t tbtt_estimate;         /* TBTT estimate data */
} WLAN_BEACON_STRUCT;

#ifdef SUPPORT_COEX
typedef enum beacon_event {
    COEX_WLAN_BEACON_INSYNC = 0,
    COEX_WLAN_BEACON_OUTOFSYNC = 1,
} BEACON_EVENT;
#endif

/* Beacon Rx to TBTT timer margin */
#define NT_WLAN_BMISS_MARGIN_MS (2)

/* Default beacon interval is 100 TUs */
#define DEFAULT_BEACON_INTERVAL 100
//#define MODIFIED_BEACON_INTERVAL 500

/* Maximum expected delay of beacon transmission from AP, without change
 * in TBTT at AP */
#define BEACON_MAX_DELAY_US 20000

#define CONSECUTIVE_BMISS_THRESHOLD 3

#define TBTT_INTEGRAL_DIFF_LIMIT 10
#define TBTT_ROUNDDOWN_THRESHOLD 5
#ifdef SUPPORT_SAP_POWERSAVE
#define CALC_BI_FROM_BM(interval, bm) ((interval / pPmStruct->beacon_multiplier) * bm)
#endif
#define BCN_MULTIPLIER_CNT 10

/* Convert TUs to # of Beacons */
#define TU_NUM_BEACON(t, bi) ((((t)-1) / (bi)) + 1)

/* TSF round up */
#define WAL_BEACON_TSF_ROUND_UP(from, step, until)                \
    do {                                                          \
        if (((step) != 0) && ((from) < (until))) {                \
            (from) += (((until) - (from)) / (step) + 1) * (step); \
        }                                                         \
    } while (0)

/*
 * Internal function declarations
 */
#ifdef FEATURE_TX_COMPLETE
void test_src_callback_handler(void *params, e_tx_compl_status_dp_msg msg);
nt_status_t test_wlan_send_probe_req(devh_t *dev, uint8_t *dstAddr, uint8_t *bssid, ssid_t *ssidInfo);
nt_status_t test_wlan_send_data_frame(devh_t *dev, uint8_t *dstAddr, uint8_t *bssid);
#endif

nt_status_t nt_wlan_beacon_recv(devh_t *dev, uint8_t *bufPtr, uint16_t bufLen, NT_BOOL isProbe, uint8_t rssi);
nt_status_t wlan_recv_probe_req(devh_t *dev, uint8_t *bufPtr, uint16_t bufLen);
nt_status_t wlan_beacon_bmiss_handler(TimerHandle_t timer_param);

nt_status_t wlan_send_probe_req(devh_t *dev, uint8_t *dstAddr, uint8_t *bssid, ssid_t *ssidInfo);

/**
 * @brief function for modifying beacon miss threshold
 * @params  count : Beacon miss count value intending to set. Passing  zero to this will
 *                  disables reconnect procedure.
 * @retval None
 */
void wlan_set_beacon_threshold(devh_t *dev, uint8_t count);

/**
 * @brief function for getting beacon miss threshold
 * @params  count : Beacon miss count value intending to get.
 * @retval None
 */
void wlan_get_beacon_threshold(devh_t *dev, uint8_t *count);

/**
 * @brief Invalidate TBTT estimate for device
 * @Param  : dev : device structure pointer
 * @Return : none
 */
void wlan_beacon_invalidate_tbtt_estimate(devh_t *dev);

/**
 * @brief Check if TBTT estimate is available form beacon
 * @Param  : dev : device structure pointer
 * @Return : bool -> TRUE: if TBTT estimate available; FALSE: Otherwise
 */
bool wlan_beacon_tbtt_estimate_is_available(devh_t *dev);

/**
 * @brief Check if existing TBTT estimate is inaccurate, based on beacon TSF
 *      stored in dev->bss
 * @Param  : dev : device structure pointer
 * @Return : bool -> TRUE: current TBTT estimate inaccurate; FALSE: Otherwise
 */
bool wlan_beacon_check_tbtt_estimate_is_inaccurate(devh_t *dev);

/**
 * @brief Update TBTT estimate from beacon info stored in dev->bss
 * @Param  : dev : device structure pointer
 * @Return : none
 */
void wlan_beacon_sync_tbtt_from_timestamp(devh_t *dev);

/**
 * @brief  : Get TBTT estimate for device
 * @Param  : dev : device structure pointer
 * @Param  : tbtt_estimate : pointer to memory for copying TBTT estimate into
 * @Return : NT_OK: TBTT estimate available and updated successfully
 *           NT_FAIL: Failed to get TBTT estimate
 */
uint8_t wlan_beacon_get_tbtt(devh_t *dev, uint64_t *tbtt_estimate);

/**
 * @brief  : Get DTIM beacon TBTT estimate for device
 * @Param  : dev : device structure pointer
 * @Param  : tbtt_estimate : pointer to memory for copying TBTT estimate into
 * @Return : NT_OK: DTIM TBTT estimate available and updated successfully
 *           NT_FAIL: Failed to get TBTT estimate
 */
uint8_t wlan_beacon_get_dtim_tbtt(devh_t *dev, uint64_t *tbtt_estimate);

/**
 * @brief  : Update RTC padding time according to TSF
 * @Param  : bss
 * @Return : none
 */
void wlan_beacon_update_rtc_padding_time(bss_t *bss);
#endif /* __WLAN_BEACON_H_ */
