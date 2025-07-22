/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _WLAN_QPOWER_H_
#define _WLAN_QPOWER_H_

#include "fwconfig_wlan.h"
#include "nt_flags.h"
#include <stdio.h>
#include "nt_common.h"
#include "wlan_dev.h"

#define QPOWER_ITO_ROW 6
#define QPOWER_ITO_COL 9

#define QPOWER_CHAN_CONG_COL_NO 0

#define QPOWER_AITO_2G_70MS_WI_COL_NO 1
#define QPOWER_SITO_2G_70MS_WI_COL_NO 2
#define QPOWER_AITO_5G_70MS_WI_COL_NO 3
#define QPOWER_SITO_5G_70MS_WI_COL_NO 4

#define QPOWER_AITO_2G_130MS_WI_COL_NO 5
#define QPOWER_SITO_2G_130MS_WI_COL_NO 6
#define QPOWER_AITO_5G_130MS_WI_COL_NO 7
#define QPOWER_SITO_5G_130MS_WI_COL_NO 8

typedef struct {
    uint32_t start_cca_busy_cnt;           /* cca busy cnt in us during sampling start*/
    uint32_t end_cca_busy_cnt;             /* cca busy cnt in us during sampling end*/
    uint32_t start_sampling_time_us;       /* Qtime in us during sampling start*/
    uint32_t total_sample_time_us;         /* Total sampling duration*/
    uint32_t avg_busy_chan_activity;       /* Average channel busy count in us*/
    uint8_t chan_congestion_perc;          /* Channel congestion percentage*/
    uint8_t is_sampling_started;           /* Check if sampling is started on not*/
    uint8_t chan_activity_coeff;           /* Low pass filter co-efficient */
    uint8_t chan_activity_round_off_const; /* Round off constant */
} chan_activity_t;

/*
 * @brief  Start channel stats sampling.
 * @param  : dev pointer
 * @return : none
 */
void wlan_qpower_chan_stats_start_sampling(devh_t *dev);

/*
 * @brief  Stop channel stats sampling and calculate channel congestion.
 * @param  : dev pointer
 * @return : none
 */
void wlan_qpower_chan_stats_end_sampling(devh_t *dev);

/*
 * @brief  : To get adaptive and speculative ITO from channel congestion %
 * @param  : dev pointer
 * @return : none
 */
void wlan_qpower_get_adaptive_and_speculative_ito(devh_t *dev);

/*
 * @brief  : To check if qpower feature is enabled for PTSM/BMPS/TWT
 * @param  : dev pointer
 * @return : TRUE/FALSE
 */
bool wlan_qpower_is_qpower_feature_enabled(devh_t *dev);

#endif  // _WLAN_QPOWER_H_
