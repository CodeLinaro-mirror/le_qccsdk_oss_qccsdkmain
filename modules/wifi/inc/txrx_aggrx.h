/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _TXRX_AGGRX_H_
#define _TXRX_AGGRX_H_

#include "osapi.h"
#include "txrx_api.h"
#include "tx_aggr_internal_api.h"

#define TXRX_AGGRX_REORDER_ENABLE  1
#define TXRX_AGGRX_REORDER_DISABLE 0

#define TXRX_AGGRX_SESSION_TIMEOUT_ENABLE  1
#define TXRX_AGGRX_SESSION_TIMEOUT_DISABLE 0

#define TXRX_AGGRX_REORDER_CFG         TXRX_AGGRX_REORDER_ENABLE
#define TXRX_AGGRX_SESSION_TIMEOUT_CFG TXRX_AGGRX_SESSION_TIMEOUT_ENABLE

#define AGGRX_BUFFER_SIZE_CFG      4
#define AGGRX_SUPPORT_AMSDU_CFG    0
#define AGGRX_SUPPORT_DELAYED_BA   0
#define AGGRX_SUPPORT_IMMEDIATE_BA 1
/* watch the block ack session. If 0, indefinite */
#define AGGRX_SESSION_TIMEOUT_VAL 0

/* BACK (block-ack) parties */
enum ieee80211_back_parties {
    WLAN_BACK_RECIPIENT = 0,
    WLAN_BACK_INITIATOR = 1,
};

/* Status codes */
enum ieee80211_statuscode {
    WLAN_STATUS_UNSPECIFIED_QOS = 32,
    WLAN_REASON_QSTA_LEAVE_QBSS = 36,
};

#define IEEE80211_QOS_CTL_ACK_POLICY_NORMAL   0x0
#define IEEE80211_QOS_CTL_ACK_POLICY_NOACK    0x1
#define IEEE80211_QOS_CTL_ACK_POLICY_NO_EXPL  0x2
#define IEEE80211_QOS_CTL_ACK_POLICY_BLOCKACK 0x3

#define IEEE80211_SN_MASK   ((IEEE80211_SEQ_SEQ_MASK) >> IEEE80211_SEQ_SEQ_SHIFT)
#define IEEE80211_MAX_SN    IEEE80211_SN_MASK
#define IEEE80211_SN_MODULO (IEEE80211_MAX_SN + 1)

#define IEEE80211_SN_LESS(sn1, sn2) ((((sn1) - (sn2)) & IEEE80211_SN_MASK) > (IEEE80211_SN_MODULO >> 1))

#define IEEE80211_SN_ADD(sn1, sn2) (((sn1) + (sn2)) & IEEE80211_SN_MASK)

#define IEEE80211_SN_INC(sn) IEEE80211_SN_ADD((sn), 1)

#define IEEE80211_SN_SUB(sn1, sn2) (((sn1) - (sn2)) & IEEE80211_SN_MASK)

#define IEEE80211_SEQ_TO_SN(seq) (((seq)&IEEE80211_SEQ_SEQ_MASK) >> IEEE80211_SEQ_SEQ_SHIFT)
#define IEEE80211_SN_TO_SEQ(ssn) (((ssn) << IEEE80211_SEQ_SEQ_SHIFT) & IEEE80211_SEQ_SEQ_MASK)

struct aggrx_tid *TXRX_aggrx_get_tid(conn_t *conn, uint8_t tid);
struct aggrx_tid *TXRX_aggrx_tid_alloc(conn_t *conn, uint8_t tid);
void TXRX_aggrx_tid_free(conn_t *conn, uint8_t tid);

void TXRX_aggrx_init(devh_t *dev);
void TXRX_aggrx_start_rx_ba_session(devh_t *dev, struct ieee80211_action_ba_addbarequest *req, conn_t *co);
void TXRX_aggrx_stop_rx_ba_session(devh_t *dev, conn_t *conn, uint8_t tid, uint8_t initiator, uint16_t reason,
                                   NT_BOOL tx);
void TXRX_aggrx_amsdu_enable_update();

#endif /*_TXRX_AGGRX_H_*/
