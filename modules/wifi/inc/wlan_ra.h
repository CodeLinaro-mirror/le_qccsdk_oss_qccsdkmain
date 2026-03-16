/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*
 *
 * * wlan_ra.h
 *
 *  Created on: Jun 9, 2020
 *      Author: vijis
 */

#ifndef _WLAN_RA_H_
#define _WLAN_RA_H_

#ifdef NT_FN_RA

#include "hal_int_rates.h"

//#define NT_RA_SAMPLING_PERIOD      10000       /* RA Sampling Period */
#define NT_RA_ON 1 /* Rate Adapt Algorithm is enabled */
#define NT_RA_OFF 0 /* Rate Adapt Algorithm is disabled */
//#define NT_RA_MIN_PKT_TX_THRES     10          /* Min Pkt Txmitted to consider for RA */
//#define NT_RA_FAST_JUMP_THRES      100         /* Tertiary rate failures to consider fast jump */
#define NT_RA_MIN_UPRATE_CNT 1000 /* Min Pkt Txmitter to consider uprate sampling */
//#define NT_RA_GOODPUT_COEFF_MIN    30          /* Good put minimum threshold */
//#define NT_RA_GOODPUT_COEFF_MAX    90          /* Good put maximum threshold */

#define NT_RA_RESET_RATE_ON_WAKEUP  1
#define NT_RA_RETAIN_RATE_ON_WAKEUP 2

typedef struct nt_rate_table_s {
    uint8_t phyMode;
    NT_BOOL valid;
    NT_BOOL basicRate;
    int8_t rateCode;

    uint32_t rateKbps;
    uint16_t txPower;
} nt_rate_table_t;

typedef struct nt_rate_context_s {
    int8_t ra_ON;            /* RA Enabled if ra_ON is 1 */
    int8_t ra_pri_FixedRate; /* it specifies pri fixed rate index */
    int8_t ra_sec_FixedRate; /* it specifies sec fixed rate index */
    int8_t ra_tri_FixedRate; /* it specifies tri fixed rate index */

    uint32_t raSamplingPeriod; /* RA periodic timer or sampling period */

    uint8_t lowPowerMode;          /* Low power mode enabled/disbaled */
    uint16_t fastJumpThresh;       /* no. of failures at tertiary rate before fast jump is enabled */
    uint16_t minPktTxThresh;       /* min pkts tx required to consider in next rate computation */
    uint16_t minGoodPutFilterCoef; /* Good put minimum threshold */
    uint16_t maxGoodPutFilterCoef; /* Good put maximum threshold */
    uint8_t perUpperThresh;        /* PER upper threshold before dropping rate */
    uint8_t upRateSamplingCnt;     /* up rate sampling count */
    uint8_t retainRate;            /* retain rate on wakeup from power save */
    uint32_t validModeMask;        /* Bit mask for valid rates */

    TimerHandle_t raPeriodicTimer; /* Periodic Timer handler */
    NT_BOOL htOnly;                /* HT rate only enabled/disabled */
} nt_rate_context_t;

#ifdef SUPPORT_COEX
/* Note: The rate defines must align with Rate Table definitions */
typedef enum {
    DERESTRICT_MIN_RATE = 0,
    RATE_9_MBPS = 9,  // 9   Mbps
    MCS1 = 16,        // 6.5 Mbps MCS 1
    INVALID_MIN_RATE = 0xFF,
} E_MIN_RATE_CODE;
#endif

void ra_inspect_rates(TimerHandle_t timer_handle);
int8_t ra_rc_to_idx(uint8_t rc);

void nt_wlan_ra_on(devh_t *dev);
void nt_wlan_ra_set_rate(devh_t *dev, uint8_t sta_idx, nt_hal_sta_tx_rate_t *rate);
#ifdef SUPPORT_COEX
uint8_t nt_ra_update_crl(devh_t *dev, conn_t *conn, uint32_t min_rate);
#endif

#endif  // NT_FN_RA

#endif /* _WLAN_RA_H_ */
