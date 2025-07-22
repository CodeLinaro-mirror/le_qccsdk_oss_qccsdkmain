/*========================================================================
Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
*
* @file coex_wlan.h
* @brief Coex wlan params and struct definitions
*========================================================================*/
#ifndef _COEX_WLAN_H_
#define _COEX_WLAN_H_
/* Standard includes. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "nt_osal.h"

#include "fwconfig_wlan.h"
#include "nt_flags.h"
#include "nt_common.h"
#if defined(SUPPORT_COEX)
#include "wlan_dev.h"
#include "coex_wlan_event_handler.h"

/*-------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * ----------------------------------------------------------------------*/

#define COEX_MIN_PKT_FOR_RSSI            (20)      // 20 pkts
#define COEX_TIME_THRESHOLD_FOR_RSSI_MON (500000)  // 500 ms
#define COEX_THRESHOLD_FOR_LOW_RSSI      (-66)     //(30)

#define RX_MONITOR_TIMEOUT           (3300)  // ms
#define WLAN_RX_XPUT_RECOVERY_THRESH (350)

#define COEX_AFH_MAP_SIZE   (4)
#define INVALID_WLAN_VDEVID (0xFF)

#define MAX_NUM_OF_WLAN_CHAN (4)
#define NUM_MAC              1

#define COEX_IS_WLAN_CONNECTED(coex_mac) \
    ((coex_mac)->state & (COEX_WLAN_SYS_CONNECTED | COEX_WLAN_SYS_PAUSED | COEX_WLAN_SYS_ALL_PAUSED))
#define COEX_IS_PROTO_HINT(coex_mac) (coex_mac->state & COEX_WLAN_SYS_CRIT_PROTO_HINT)
#define COEX_IS_WLAN_DISCONNECTED(coex_mac) \
    (!((coex_mac)->state & (COEX_WLAN_SYS_CONNECTED | COEX_WLAN_SYS_CONNECTING)))

#define COEX_WLAN_BAND_HALF_RANGE_20M (10)
#define COEX_WLAN_BAND_HALF_RANGE_80M (40)

#define BEACON_MISS_ID (0x1)

#define COEX_PDEV(wlan_id)         gpBtCoexWlanInfoDev->psoc->p_wal_pdev_array[wlan_id]
#define COEX_IS_2GHZ_WLAN(wlan_id) (gpBtCoexWlanInfoDev->mac[wlan_id].band == COEX_WLAN_2GHZ) ? TRUE : FALSE

// taking into consideration the worst case scenarios of lateny of data path in transmitting CTS2S frame
// if the frame goes OTA much sooner CTS2S interval gets preempted normally.
#define CTS2S_TX_TIMEOUT (15)  // ms

#define COEX_DONTCARE_CHM (0x3)

/*-------------------------------------------------------------------------
 * Type Declarations
 * ----------------------------------------------------------------------*/
enum {
    COEX_LOW_RSSI_TYPE = 0,
    COEX_MID_RSSI_TYPE = 1,
    COEX_HI_RSSI_TYPE = 2,
    COEX_INVALID_RSSI_TYPE = 3,
};

enum {
    COEX_11B_PHY_RATE = 0,
    COEX_11G_PHY_RATE = 1,
};

enum {
    COEX_WLAN_XPUT_NORMAL = 0,
    COEX_WLAN_XPUT_RX_UNDER_THRESH = 1,
    COEX_WLAN_XPUT_RX_CRITICAL = 2,
    COEX_WLAN_XPUT_RX_RECOVERY_TIMEOUT = 3,
};

typedef struct {
    uint32_t min_rx_rate;                /* in Kbps */
    uint32_t max_rx_rate;                /* in Kbps */
    uint8_t num_mpdus_below_min_rx_rate; /* number of consecutive PPDUs whose rxrate should be below
                                             lower_threshold in order to trigger a event */
    uint8_t num_mpdus_above_max_rx_rate; /* number of consecutive PPDUs whose rxrate should be above
                                             upper_threshold in order to trigger a event */
    uint8_t ReserveByte0;
    uint8_t ReserveByte1;
    uint32_t frame_length_threshold;
} tCoexPeerRxThresh;

enum {
    COEX_WLAN_1 = 0,
    COEX_WLAN_INVALID = 0xFF,
};

typedef struct _coex_mac {
    uint8_t id;
    uint8_t band;
    uint8_t cxc_settings;
    uint16_t freq;
    uint32_t state;
    uint8_t pwr_state;
} coex_mac;

typedef enum {
    PDEV_EVENT_CHANNEL_CHANGE = 0x100, /* event generated immediately before channel change */
    PDEV_EVENT_MAX = 0x80000000,
} phy_dev_event_type;

typedef struct coex_chan_change_async {
    devh_t *chan_dev_handle;       /* pdev handle. */
    phy_dev_event_type event_type; /* Type of event - Channel change. */
    channel_t cur_channel;         /* current channel */
    channel_t new_channel;         /* new channel being set */
    void *chan_event_arg;          /* Event arguments to cache before go async. */
} coex_chan_change_async_t;

typedef struct t_CoexVdevStruct {
    uint8_t pdev_id;
    uint8_t WlanRxState;
    int32_t MinRSSI;
    uint32_t coex_rssi_time;
    uint32_t coex_rssi_seq;
    uint32_t PrevPeerSecurityState;
    uint16_t VdevSchedulerFlags;
    uint16_t OperatingChannel;
    uint16_t CurVdevState;
    uint8_t OpMode;
    uint8_t NumOfConnectedPeers;
    uint8_t OperatingBand;
    uint8_t isUp;
    uint8_t isPaused;
    uint8_t beacon_sync_state;
    channel_t Chan;
    uint8_t PeerSecurityState;
    uint16_t NumOf11gPeers;
    uint16_t NumOf11bPeers;
} tCoexVdevStruct;

typedef struct coex_wlan_info_t {
    /*Device State */
    tCoexVdevStruct *pCoexVdev;
    uint8_t *pWlanSTAList;

    uint8_t NumOfVdev;
    uint8_t NumOfWlanSTA;

    uint8_t num_of_vdev_with_bmiss;

    uint8_t SchemeOp;

    /* RSSI Monitor */
    int32_t MinRSSI;
    int32_t BRSSI;
    int32_t MaxRSSI;
    uint8_t RssiType;

    /*Throughput Monitor */
    uint32_t MinDirectRxDeltaTime;
    uint32_t MinDirectRxRate;
    uint32_t NumOfRxUnderThreshPeers;
    uint8_t isRxUnderThreshWakelockOn;
    uint32_t XputMonitorActiveNum;
    uint32_t XputMonitorTriggeredRxRate;
    uint8_t IsWlanRxCritical;
    uint8_t WlanRxState;
    tCoexPeerRxThresh LowRssiWlanRxThreshPerBand;
    tCoexPeerRxThresh MidRssiWlanRxThreshPerBand;
    tCoexPeerRxThresh HiRssiWlanRxThreshPerBand;
    tCoexPeerRxThresh Phy11BModeWlanRxThreshPerBand;
    tCoexPeerRxThresh Phy11GModeWlanRxThreshPerBand;

    uint32_t WlanBtAfhMap[COEX_AFH_MAP_SIZE];
    coex_mac mac[NUM_MAC];
    uint32_t WmacPowerUpFlag;

    uint8_t isCoexPrioTimerRunning;
    uint32_t LastScanTime;
    uint8_t NumOfConnectedVdev;
    uint8_t dhcp_vdev_id;
    uint32_t CoexSchedulerFlags;
    uint32_t TotalCoexIntervals;
    coex_chan_change_async_t chan_change_async[NUM_MAC];
    uint32_t BeaconID;
    uint8_t FlowCtrlIDs;
    uint8_t PrevCoexAlgorithm;
    bool bSTAexistWithA2dpHiDef;
    nt_osal_timer_handle_t DhcpDetectionTimer;
    nt_osal_timer_handle_t CoexPrioTimer;
#ifndef EMULATION_BUILD
    nt_osal_timer_handle_t BtRespCXCResetTimer;
#endif
    bool ra_min_rate_enable;
    uint8_t ra_min_rate;
    uint8_t crl_count;
    uint8_t Is2GpDevIdle;
} BTCOEX_WLAN_INFO_STRUCT;

/*-------------------------------------------------------------------------
 * Function Declarations and Documentation
 * ----------------------------------------------------------------------*/
void coex_set_rx_rate_thresh(uint8_t RSSIType);
void coex_update_afh(channel_t *pWHALCh, uint8_t IsAdd);
void coex_reassess_wlanstate(uint8_t pdev_id);
void _nt_wlan_post_dhcp_timeout_msg(nt_osal_timer_handle_t thandle);
void coex_dhcp_detection_timer_expired();

void coex_channel_change_cb(devh_t *dev, channel_t *chan);
void coex_channel_change_proc_async(coex_chan_change_async_t *chan_change_param);
nt_status_t coex_process_band_change(uint8_t mac, channel_t *new_chan);
void coex_get_wlan_perchain_status(void *perchain_stats_);
void coex_send_wlan_perchain_status(void *new_stats_, uint32_t reason);
#endif /* SUPPORT_COEX */
#endif /*#ifndef _COEX_WLAN_H_*/
