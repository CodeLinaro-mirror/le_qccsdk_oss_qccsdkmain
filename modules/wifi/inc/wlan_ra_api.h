/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*
 *
 * * wlan_ra_api.h
 *
 *  Created on: Jun 9, 2020
 *  Author: Viji Alagarsamy
 */
#ifndef _WLAN_RA_API_H_
#define _WLAN_RA_API_H_

#ifdef NT_FN_RA
#include "wlan_dev.h"
#include "wlan_conn.h"

#ifdef SUPPORT_5GHZ
#define DEFAULT_5G_RATE_INDEX HAL_DEFAULT_5G_RATE_INDEX  // 6 mbps rate for 5ghz
#define DEFAULT_2G_RATE_INDEX HAL_DEFAULT_2G_RATE_INDEX  // 1 mbps rate for 2ghz
#endif

#define DEFAULT_CONNECTED_HAL_STA_IDX     1
#define DEFAULT_NON_CONNECTED_HAL_STA_IDX 2

typedef enum {
    NT_RA_NO_CHANGE = 0,
    NT_RA_RESET_RATE,
    NT_RA_UP_RATE,
    NT_RA_ONE_DOWN_RATE,
    NT_RA_TWO_DOWN_RATE,
    NT_RA_RESET_TO_MIDDLE_RATE
} nt_ra_op_e;

typedef struct nt_ra_crl_vector_s {
    uint8_t p_crl_idx;
    uint8_t s_crl_idx;
    uint8_t t_crl_idx;
} nt_ra_crl_vector_t;

#define HAL_RT_CRL_IDX_MAX_RATES HAL_RT_IDX_MAX_RATES

typedef struct nt_ra_crl_s {
    uint8_t cnt;
    uint8_t crl[HAL_RT_CRL_IDX_MAX_RATES];
} nt_ra_crl_t;

typedef struct nt_ra_cfg_s {
    int8_t ra_ON;            /* RA Enabled if ra_ON is 1 */
    int8_t ra_pri_FixedRate; /* it specifies pri fixed rate index */
    int8_t ra_sec_FixedRate; /* it specifies sec fixed rate index */
    int8_t ra_tri_FixedRate; /* it specifies tri fixed rate index */

    uint32_t raSamplingPeriod; /* RA periodic timer or sampling period */

    uint16_t fastJumpThresh;       /* no. of failures at tertiary rate before fast jump is enabled */
    uint16_t minPktTxThresh;       /* min pkts tx required to consider in next rate computation */
    uint16_t minGoodPutFilterCoef; /* Good put filter minimum threshold */
    uint16_t maxGoodPutFilterCoef; /* Good put filter maximum threshold */
    uint8_t perUpperThresh;        /* PER upper threshold before dropping rate */
    uint8_t upRateSamplingCnt;     /* up rate sampling count */
    uint8_t retainRate;            /* retain rate on wakeup from power save */
} nt_ra_cfg_t;
#endif  // NT_FN_RA

void *nt_wlan_ra_init(struct devh_s *dev);
nt_status_t nt_wlan_ra_set_supp_rates(WLAN_PHY_MODE phy_mode, struct ieee80211_rateset *supp_rates);

#ifdef NT_FN_RA
void nt_wlan_ra_deinit(void *);
void nt_wlan_ra_set_cfg(struct devh_s *dev, nt_ra_cfg_t *ra_cfg);
#ifdef NT_FN_SNIFFER
void nt_wlan_sniffer_log(NT_BOOL flag);
#endif  // NT_FN_SNIFFER
void nt_wlan_ra_get_cfg(struct devh_s *dev);
void nt_ra_adj_crl(struct devh_s *dev, struct conn_s *conn, NT_BOOL htrates_only);
void nt_ra_set_crl(struct conn_s *conn);
void nt_ra_set_ht_crl(struct conn_s *conn);
NT_BOOL nt_ra_set_tx_rates(struct devh_s *dev, struct conn_s *conn, nt_ra_op_e ra_op);
void nt_set_overwrite_fixed_rate(struct devh_s *dev, int8_t rateindex);
void nt_wlan_start_ra(struct devh_s *dev);
void nt_wlan_ra_update_rate();
void nt_wlan_stop_ra(struct devh_s *dev);
void nt_wlan_resume_ra(struct devh_s *dev);
void nt_wlan_pause_ra(struct devh_s *dev);
void nt_wlan_ra_on(struct devh_s *dev);
NT_BOOL nt_ra_set_htrates_only(struct devh_s *dev, NT_BOOL enable);

#endif  // NT_FN_RA

#endif /* _WLAN_RA_API_H_ */
