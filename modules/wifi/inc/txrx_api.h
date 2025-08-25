/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
//
// This file contains the api for the WLAN TxRx module.
//
// $Id:
//
//

#ifndef _TXRX_API_H_
#define _TXRX_API_H_

#include "txrx_cfg_api.h"
#include "wmi.h"
#include "nt_osal.h"

struct devh_s;
struct bss_s;
struct conn_s;

//#define BA_WINDOW_SIZE		8

#define NT_SET_DELIVERY_APSD_FOR_AC(_config, _ac, _val) \
    do {                                                \
        (_config)->deliveryEnabled &= ~((1 << (_ac)));  \
        (_config)->deliveryEnabled |= (_val) << (_ac);  \
    } while (0)

#define NT_SET_TRIGGERED_APSD_FOR_AC(_config, _ac, _val) \
    do {                                                 \
        (_config)->triggeredEnabled &= ~((1 << (_ac)));  \
        (_config)->triggeredEnabled |= (_val) << (_ac);  \
    } while (0)

#define NT_GET_DELIVERY_APSD_FOR_AC(_config, _ac) ((_config)->deliveryEnabled & (1 << (_ac)))

#define NT_GET_TRIGGERED_APSD_FOR_AC(_config, _ac) ((_config)->triggeredEnabled & (1 << (_ac)))

#define NT_SET_UAPSD_CAPABILITY(_config, _val) \
    do {                                       \
        (_config)->uapsd_cap = _val;           \
    } while (0)
#define NT_GET_UAPSD_CAPABILITY(_config) (_config)->uapsd_cap
#define NT_RESET_UPSD_COUNT(_config)     \
    do {                                 \
        (_config)->triggeredEnabled = 0; \
        (_config)->deliveryEnabled = 0;  \
    } while (0)
#define NT_GET_MAX_SP_LEN(_config) ((_config)->maxSP)
#define NT_SET_MAX_SP_LEN(_config, _len)                                      \
    do {                                                                      \
        (_config)->maxSP = ((_len) > DELIVER_6_PKT) ? DELIVER_6_PKT : (_len); \
    } while (0)

struct apsdConfig {
    NT_BOOL uapsd_cap;
    uint8_t deliveryEnabled;
    uint8_t triggeredEnabled;
    APSD_SP_LEN_TYPE maxSP;
};

/* enum used by TXRX_ChangeChannel to allow for proper channel config
 * when using HT rates */
typedef enum {
    TXRX_MODE_NONE = 0,
    TXRX_MODE_HT20, /* use HT20 */
    TXRX_MODE_MAX
} TXRX_MODE_EXT;

void TXRX_Init(struct devh_s *dev);
/*@brief:TXRX deinit for deleting the timers used in txrx_api.c
 * This function called at wlan_deinit()
 * @param: global device structure
 * @returns:nothing
 */
void TXRX_Deinit(struct devh_s *dev);
void TXRX_ConnectionNotify(struct devh_s *dev, struct conn_s *conn, NT_BOOL bConnected);
void TXRX_perform_ba(struct devh_s *dev, WMI_ADDBA_REQ_CMD *cmd, void *conn);
void TXRX_process_addba_req(struct devh_s *dev, struct ieee80211_action_ba_addbarequest *req, struct conn_s *conn);
void TXRX_process_addba_resp(struct devh_s *, struct ieee80211_action_ba_addbaresponse *, struct conn_s *conn);
void TXRX_process_delba(struct devh_s *, uint8_t, uint8_t, uint16_t, NT_BOOL, void *conn);
void TXRX_teardown_ba(struct devh_s *dev, WMI_DELBA_REQ_CMD *cmd);
nt_status_t nt_eapol_rx_input(struct devh_s *dev, uint8_t *bufPtr, uint16_t bufLen);

/*@brief: tkip countermeasure ,called when mic-error is received by STA/AP. Disconnects AP from all STA's or STA from AP
 * as is the case when 2 mic-errors are received by the device in less than a minute
 * @param: global device structure
 * @returns: nothing
 */
void TXRX_countermeasures_tkip(struct devh_s *dev);
/*@brief: call-back function for mic error counting timer , re-sets the mic-error counter
 * called when only a single mic-error occurs in a span of 60 seconds
 *@param: timer handle
 *@returns: nothing
 */
void TXRX_reset_mic_err_cnt_timeout_handler(TimerHandle_t timer_handle);
/*@brief: enables association for STA/AP after tkip countermeasure
 *@param: timer handle
 *@returns: nothing
 */
void TXRX_enable_assoc_timeout_handler(TimerHandle_t timer_handle);
void _nt_wlan_post_txrx_enable_assoc(TimerHandle_t thandle);

#endif /* _TXRX_API_H_ */
