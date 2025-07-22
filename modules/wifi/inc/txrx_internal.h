/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _TXRX_INTERNAL_H_
#define _TXRX_INTERNAL_H_

#include "osapi.h"
#include "wlan_dev.h"
#include "wlan_conn.h"
#include "txrx_api.h"
#include "tx_aggr_internal_api.h"
#include "txrx_aggrx.h"

#define ADDBA_ATTEMPTS_MAX 10

typedef struct {
    uint16_t min;
    uint16_t max;
} INTF_PARAM;

struct tspecNorespTimerArgs {
    devh_t *dev;
    TimerHandle_t tspecTimer;
    uint8_t ac;
};

struct mboxInactivityTimerArgs {
    devh_t *dev;
    uint8_t trafficClass;
};

typedef struct TXRX_CONTEXT {
    struct apsdConfig sc_apsdConfig; /* apsd config */
    struct chanAccParams sc_chanParams;
    struct chanAccParams sc_defaultChanParams;
    uint8_t apsdConfigInWmmIe;
        /* per device config */  // current status of SP and AC's trigger enable and delivery enable details.

    TimerHandle_t addBaRespTimer; /* per device */
    uint16_t txopEnabledAcs;
    uint8_t addba_resp_timeout; /*add ba resp timer, value read from dev config*/
    uint8_t auto_ba_enable;     /*enable ba after connection */
    uint8_t auto_ba_ac_type;    /*gives type of AC*/
    uint8_t mic_err_timer;      /*mic error timer, value read from dev config*/
    uint8_t tkip_wait_timer;    /*tkip countermeasure timer, value read from dev config*/
} TXRX_CONTEXT;

extern TXRX_CONTEXT *g_pTXRX;

// QOS realted internal API
#ifdef NT_FN_WMM
void txrx_qos_init_queueparams(devh_t *dev);
void nt_txrx_devcfg_qos_params(devh_t *dev);
#endif  // NT_FN_WMM
void txrx_qos_update_tx_param(devh_t *dev, struct wmmParams *pPhyParam, uint32_t wmm_ac);
void TXRXAddBaRespTimeout(TimerHandle_t data);
void _nt_wlan_post_TxrxAddBaResp(TimerHandle_t thandle);

#endif /*_TXRX_INTERNAL_H_*/
