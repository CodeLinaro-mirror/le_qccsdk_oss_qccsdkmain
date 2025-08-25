/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __TX_AGGR_INTERNAL_API_H__
#define __TX_AGGR_INTERNAL_API_H__

#include "ieee80211.h"

#define AGGTX_BUFFER_SIZE_CFG      32
#define AGGTX_SUPPORT_AMSDU_CFG    1
#define AGGTX_SUPPORT_DELAYED_BA   0
#define AGGTX_SUPPORT_IMMEDIATE_BA 1
/* watch the block ack session. If 0, indefinite */
#define AGGRTX_SESSION_TIMEOUT_VAL 0
#define AGGRRX_SESSION_TIMEOUT_VAL 0

//#define BA_SESSION_TIMER_TIMEOUT_VAL 1000 //addba timeout increased due to bmps wakeup

#define MAX_BAR_LIMIT 5

#define HT_AGG_STATE_START             (1 << 0)
#define HT_AGG_STATE_RESPONSE_RECEIVED (1 << 1)
#define HT_AGG_STATE_OPERATIONAL       (1 << 2)
#define HT_AGG_STATE_WANT_STOP         (1 << 3)
#define HT_AGG_STATE_STOPPING          (1 << 4)

enum ieee80211_reasoncode {
    WLAN_REASON_QSTA_NOT_USE = 37,
};

enum aggtx_stop_reason {
    AGGTX_STOP_DECLINED,      /* req is declined by peer */
    AGGTX_STOP_LOCAL_REQUEST, /* timeout to receive resp */
    AGGTX_STOP_PEER_REQUEST,  /* receive delba */
    AGGTX_STOP_DESTROY_STA,   /* disconnection */
};

#define SORT_TYPE_ASCENDING_SEQ 1

#ifdef NT_FN_AMPDU
void tx_aggr_start_tx_ba_session(devh_t *dev, void *conn, uint8_t tid);
void tx_aggr_process_addba_resp(devh_t *dev, struct ieee80211_action_ba_addbaresponse *resp, conn_t *co);
void tx_aggr_stop_tx_ba_session(devh_t *dev, conn_t *conn, uint8_t tid, uint16_t stop_reason);
struct aggtx_tid *tx_aggr_get_tid(conn_t *conn, uint8_t tid);
struct aggtx_tid *tx_aggr_tid_alloc(conn_t *conn, uint8_t tid);
void tx_aggr_tid_free(conn_t *conn, uint8_t tid);
uint16_t tx_aggr_select_ssn(conn_t *co, uint8_t tid);
void tx_aggr_window_size_update(devh_t *dev, uint16_t window_size);
void tx_aggr_ampdu_bit_update(devh_t *dev, uint8_t status, uint8_t sta_id);

/**
 * @brief      function reads the configuration done for HT_cap in devconfig structures
 *             fills the necessary fields in devh_h type device structure
 * @parameter  dev: devh_s type device structure pointer
 * @return     none
 */
void nt_tx_aggr_devconfig_ht_cap(devh_t *dev);
#endif  // NT_FN_AMPDU
#ifdef NT_FN_WMM
/**
 * @brief      function reads the configuration done for wmm in devconfig structures
 *             fills the necessary fields in devh_h type device structure
 * @parameter  dev: devh_s type device structure pointer
 * @return     none
 */
void nt_tx_aggr_devconfig_wmm(struct devh_s *dev);
#endif  // NT_FN_WMM

#endif /* __TX_AGGR_INTERNAL_API_H__ */
