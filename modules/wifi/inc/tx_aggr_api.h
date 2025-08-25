/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __TX_AGGR_API_H__
#define __TX_AGGR_API_H__

#include "wlan_conn.h"

struct devh_s;
struct conn_s;

#define RX_AGGR_GET_STATE(_x, _t)   ((_x)->state & (0x1 << (_t)))
#define RX_AGGR_SET_STATE(_x, _t)   ((_x)->state |= (0x1 << (_t)))
#define RX_AGGR_RESET_STATE(_x, _t) ((_x)->state &= ~(0x1 << (_t)))
#define RX_AGGR_CLEAR_STATE(_x)     ((_x)->state = 0)

#define TX_AGGR_GET_STATE(_x, _t)   RX_AGGR_GET_STATE((_x), (_t))
#define TX_AGGR_SET_STATE(_x, _t)   RX_AGGR_SET_STATE((_x), (_t))
#define TX_AGGR_RESET_STATE(_x, _t) RX_AGGR_RESET_STATE((_x), (_t))
#define TX_AGGR_CLEAR_STATE(_x)     RX_AGGR_CLEAR_STATE(_x)

typedef struct {
    uint32_t state;
} RX_AGGR, TX_AGGR;

#define AGGR_CFG_GET_TX_BUF_SZ(_x) ((_x)->aggr_cfg.tx_win)
#define AGGR_CFG_GET_RX_BUF_SZ(_x) ((_x)->aggr_cfg.rx_win)

#define AGGR_CFG_GET_SUPPORT_AMSDU(_x) ((_x)->aggr_cfg.amsdu_support)

#define AGGR_CFG_IS_TX_AGGR_ALLOWED(_x, _t) ((_x)->aggr_cfg.tx_allow_aggr & (0x1 << (_t)))
#define AGGR_CFG_IS_RX_AGGR_ALLOWED(_x, _t) ((_x)->aggr_cfg.rx_allow_aggr & (0x1 << (_t)))

typedef struct {
    uint8_t rx_win; /* It will be 8 or as dictated by buf req */
    uint8_t tx_win; /* We will ask for 32, or some config value */
    uint8_t amsdu_support;
    uint16_t tx_allow_aggr; /* bit maps indicating if tx/rx aggr is allowed */
    uint16_t rx_allow_aggr;
} AGGR_CFG;

#ifdef NT_FN_AMPDU
struct aggtx_tid {
    struct conn_s *conn;
    nt_osal_timer_handle_t addba_resp_timer;
    uint16_t ssn;
    uint16_t buf_size;
    uint16_t timeout;
    uint8_t dialog_token;
    uint8_t tid;
    uint8_t state;
};
#endif

/* struct aggrx_tid - TID aggregation information (Rx).
 * session_timer: check if peer keeps Tx-ing on the TID (by timeout value)
 * ssn: Starting Sequence Number expected to be aggregated.
 * buf_size: buffer size for incoming A-MPDUs
 * timeout: reset timer value (in TUs).
 * dialog_token: dialog token for aggregation session
 * removed: this session is removed
 */
struct aggrx_tid {
    uint16_t ssn;
    uint16_t buf_size;
    uint8_t dialog_token;
    uint8_t tid;
};

#define IEEE80211_MAX_SEQ_NO     0xFFF
#define IEEE80211_NEXT_SEQ_NO(x) (((x) + 1) & IEEE80211_MAX_SEQ_NO)
#define AGGR_FRAME_TYPE_MASK     (TXRX_QOS_FRAME)

#define MAX_FW_RETRY_LIMIT 2

/*
 * Routine: tx_aggr_init
 *      Initialze data structures for tid q's
 * Arguments:
 *      None
 */
void tx_aggr_init(void *dev_p);

/*
 * Routine: tx_aggr_set_params
 *      Sets aggregation params, after ADDBA negotiations.
 * Arguments:
 *      tid: tid for setting params
 *      seq_st: starting seq num
 *      win_sz: negotiated window size with addba
 *      state: on/off
 */
void tx_aggr_set_params(struct conn_s *conn, uint8_t tid, uint16_t seq_st, uint8_t win_sz, NT_BOOL state);

/*
 * Routine: tx_aggr_get_state
 *      Return the aggregate state in hold q for a tid.
 * Arguments:
 *      conn -> pointer to conn object <currently not used>
 *      tid -> tid q to test
 */
NT_BOOL
tx_aggr_get_state(struct conn_s *conn, uint8_t tid);

#endif /*__TX_AGGR_API_H__ */
