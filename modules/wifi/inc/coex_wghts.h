/*========================================================================
Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
*
* @file coex_weights.h
* @brief Coex weight table params and struct definitions
*========================================================================*/
#ifndef _COEX_WGHTS_H_
#define _COEX_WGHTS_H_
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
#include "stdbool.h"
#include "coex_bt.h"
/*-------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * ----------------------------------------------------------------------*/

#define COEX_FES_TX_ROW_START_INDEX (2)
#define COEX_FES_RX_ROW_START_INDEX (6)

#define COEX_WLAN_TX_PRIO_INDX_MAX (16)
#define COEX_WLAN_RX_PRIO_INDX_MAX (16)

#define COEX_NUM_802_11_FRM_TYPE    (3)
#define COEX_NUM_802_11_FRM_SUBTYPE (16)

/* coex_api.h */
#define COEX_WGHT_TABLE_NUM_ROWS (11)

#define COEX_WGHTS_LOWER_THRESH(prio, lower_lim) (((prio) < (lower_lim)) ? (lower_lim) : (prio))

/*
 * Low priority BT: 0 - 59(0x3b)
 * High priority BT: 60 - 125(0x7d)
 * Critical BT: 126 - 255

    BTCOEX_WL_WEIGHT0_VALUE0 ; // wl_idle + !tm_wl_wait_beacon
    BTCOEX_WL_WEIGHT0_VALUE1 ; // sw_ctrl[3] - all_stomp
    BTCOEX_WL_WEIGHT0_VALUE2 ; // sw_ctrl[2] - all_not_stomp
    BTCOEX_WL_WEIGHT0_VALUE3 ; // sw_ctrl[1] - pa_pre_distortion
    BTCOEX_WL_WEIGHT1_VALUE0 ; // sw_ctrl[0] - general purpose
    BTCOEX_WL_WEIGHT1_VALUE1 ; // tm_wl_wait_beacon
    BTCOEX_WL_WEIGHT1_VALUE2 ; // ts_state_wait_ack_cts
    BTCOEX_WL_WEIGHT1_VALUE3 ; // self_gen
    BTCOEX_WL_WEIGHT2_VALUE0 ; // WLAN TX FES Transmit part + (tx_priority == 0)
    BTCOEX_WL_WEIGHT2_VALUE1 ; // WLAN TX FES Transmit part + (tx_priority == 1)
    BTCOEX_WL_WEIGHT2_VALUE2 ; // WLAN TX FES Transmit part + (tx_priority == 2)
    BTCOEX_WL_WEIGHT2_VALUE3 ; // WLAN TX FES Transmit part + (tx_priority == 3)
    BTCOEX_WL_WEIGHT3_VALUE0 ; // WLAN TX FES Transmit part + (tx_priority == 4)
    BTCOEX_WL_WEIGHT3_VALUE1 ; // WLAN TX FES Transmit part + (tx_priority == 5)
    BTCOEX_WL_WEIGHT3_VALUE2 ; // WLAN TX FES Transmit part + (tx_priority == 6)
    BTCOEX_WL_WEIGHT3_VALUE3 ; // WLAN TX FES Transmit part + (tx_priority == 7)
    BTCOEX_WL_WEIGHT4_VALUE0 ; // WLAN TX FES Transmit part + (tx_priority == 8)
    BTCOEX_WL_WEIGHT4_VALUE1 ; // WLAN TX FES Transmit part + (tx_priority == 9)
    BTCOEX_WL_WEIGHT4_VALUE2 ; // WLAN TX FES Transmit part + (tx_priority == 10)
    BTCOEX_WL_WEIGHT4_VALUE3 ; // WLAN TX FES Transmit part + (tx_priority == 11)
    BTCOEX_WL_WEIGHT5_VALUE0 ; // WLAN TX FES Transmit part + (tx_priority == 12)
    BTCOEX_WL_WEIGHT5_VALUE1 ; // WLAN TX FES Transmit part + (tx_priority == 13)
    BTCOEX_WL_WEIGHT5_VALUE2 ; // WLAN TX FES Transmit part + (tx_priority == 14)
    BTCOEX_WL_WEIGHT5_VALUE3 ; // WLAN TX FES Transmit part + (tx_priority == 15)
    BTCOEX_WL_WEIGHT6_VALUE0 ; // WLAN TX FES Receive part + (tx_priority == 0)
    BTCOEX_WL_WEIGHT6_VALUE1 ; // WLAN TX FES Receive part + (tx_priority == 1)
    BTCOEX_WL_WEIGHT6_VALUE2 ; // WLAN TX FES Receive part + (tx_priority == 2)
    BTCOEX_WL_WEIGHT6_VALUE3 ; // WLAN TX FES Receive part + (tx_priority == 3)
    BTCOEX_WL_WEIGHT7_VALUE0 ; // WLAN TX FES Receive part + (tx_priority == 4)
    BTCOEX_WL_WEIGHT7_VALUE1 ; // WLAN TX FES Receive part + (tx_priority == 5)
    BTCOEX_WL_WEIGHT7_VALUE2 ; // WLAN TX FES Receive part + (tx_priority == 6)
    BTCOEX_WL_WEIGHT7_VALUE3 ; // WLAN TX FES Receive part + (tx_priority == 7)
    BTCOEX_WL_WEIGHT8_VALUE0 ; // WLAN TX FES Receive part + (tx_priority == 8)
    BTCOEX_WL_WEIGHT8_VALUE1 ; // WLAN TX FES Receive part + (tx_priority == 9)
    BTCOEX_WL_WEIGHT8_VALUE2 ; // WLAN TX FES Receive part + (tx_priority == 10)
    BTCOEX_WL_WEIGHT8_VALUE3 ; // WLAN TX FES Receive part + (tx_priority == 11)
    BTCOEX_WL_WEIGHT9_VALUE0 ; // WLAN TX FES Receive part + (tx_priority == 12)
    BTCOEX_WL_WEIGHT9_VALUE1 ; // WLAN TX FES Receive part + (tx_priority == 13)
    BTCOEX_WL_WEIGHT9_VALUE2 ; // WLAN TX FES Receive part + (tx_priority == 14)
    BTCOEX_WL_WEIGHT9_VALUE3 ; // WLAN TX FES Receive part + (tx_priority == 15)
    BTCOEX_WL_WEIGHT10_VALUE0 ; // level 3
    BTCOEX_WL_WEIGHT10_VALUE1 ; // level 2
    BTCOEX_WL_WEIGHT10_VALUE2 ; // level 1
    BTCOEX_WL_WEIGHT10_VALUE3 ; // 0
*/

#define BTCOEX_IDLE_OVERRIDE_ID0               (0)
#define BTCOEX_BEACON_OVERRIDE_ID              (5)
#define BTCOEX_SELFGEN_OVERRIDE_ID             (7)
#define BTCOEX_ACK_WAIT_OVERRIDE_ID            (6)
#define BTCOEX_RX_LVL3_OVERRIDE_ID             (40)
#define BTCOEX_RX_LVL2_OVERRIDE_ID             (41)
#define BTCOEX_RX_LVL1_OVERRIDE_ID             (42)
#define BTCOEX_CTS2S_PAUSE_UNPAUSE_OVERRIDE_ID (14)
#define BTCOEX_BEACON_TX_OVERRIDE_ID           (8)

#define BTCOEX_IDLE_OVERRIDE(pPointer, Priority) *(pPointer + BTCOEX_IDLE_OVERRIDE_ID0) = Priority;

#define BTCOEX_BEACON_OVERRIDE(pPointer, Priority)          \
    do {                                                    \
        *(pPointer + BTCOEX_BEACON_OVERRIDE_ID) = Priority; \
    } while (0)

#define BTCOEX_SELFGEN_OVERRIDE(pPointer, Priority)          \
    do {                                                     \
        *(pPointer + BTCOEX_SELFGEN_OVERRIDE_ID) = Priority; \
    } while (0)

#define BTCOEX_ACK_WAIT_OVERRIDE(pPointer, Priority)          \
    do {                                                      \
        *(pPointer + BTCOEX_ACK_WAIT_OVERRIDE_ID) = Priority; \
    } while (0)

#define BTCOEX_RX_OVERRIDE(pPointer, lvl3_prio, lvl2_prio, lvl1_prio) \
    do {                                                              \
        *(pPointer + BTCOEX_RX_LVL3_OVERRIDE_ID) = lvl3_prio;         \
        *(pPointer + BTCOEX_RX_LVL2_OVERRIDE_ID) = lvl2_prio;         \
        *(pPointer + BTCOEX_RX_LVL1_OVERRIDE_ID) = lvl1_prio;         \
    } while (0)

#define BTCOEX_CTS2S_PAUSE_UNPAUSE_OVERRIDE(pPointer, Priority)          \
    do {                                                                 \
        *(pPointer + BTCOEX_CTS2S_PAUSE_UNPAUSE_OVERRIDE_ID) = Priority; \
    } while (0)

#define BTCOEX_BEACON_TX_OVERRIDE(pPointer, Priority)          \
    do {                                                       \
        *(pPointer + BTCOEX_BEACON_TX_OVERRIDE_ID) = Priority; \
    } while (0)

#define BTCOEX_MIN_PRIO (0)
#define BTCOEX_MAX_PRIO (255)

#define BTCOEX_RFCOMM_PRIO (50)
#define BTCOEX_BNEP_PRIO   (52)

#define BTCOEX_CONCURRENT_TX_PRIO (1)
#define BTCOEX_ACL_PROF_HI_PRIO   (90)
#define BTCOEX_SELFGEN_ESCO_PRIO  (115)  // higher than esco normal transmittion but low than esco retansmittion
#define BTCOEX_SELFGEN_ISO_PRIO   (115)

#define BTCOEX_INVALID_PRIO BTCOEX_MIN_PRIO

#define BTCOEX_BASE_PRIO          (2)
#define BTCOEX_BASE_PRIO_CCRNT_TX (BT_DEFAULT_PRIO_CHAN_ASSESS + 1)

#define BTCOEX_LOW_PRIO          (54)
#define BTCOEX_LOW_PRIO_CCRNT_TX (BTCOEX_LOW_PRIO + 1)

#define BTCOEX_MID_PRIO          (66)
#define BTCOEX_MID_PRIO_CCRNT_TX (BTCOEX_MID_PRIO + 1)

#define BTCOEX_MID_PRIO_NONSYNC          (92)
#define BTCOEX_MID_PRIO_NONSYNC_CCRNT_TX (BTCOEX_MID_PRIO_NONSYNC + 1)

#define BTCOEX_HI_PRIO_NONVOICE          (108)
#define BTCOEX_HI_PRIO_NONVOICE_CCRNT_TX (BTCOEX_HI_PRIO_NONVOICE + 1)

#define BTCOEX_HI_PRIO          (125)
#define BTCOEX_HI_PRIO_CCRNT_TX (BTCOEX_HI_PRIO + 1)

#define BTCOEX_CRITICAL_PRIO_NONSYNC               (220U)  // BTCOEX_MID_PRIO_NONSYNC(92) + 128
#define BTCOEX_CRITICAL_PRIO_NONSYNC_CCRNT_TX      (BTCOEX_CRITICAL_PRIO_NONSYNC + 1)
#define BTCOEX_CRITICAL_PRIO_NONSYNC_INCR_CCRNT_TX (BTCOEX_CRITICAL_PRIO_NONSYNC_CCRNT_TX + 1)

#define BT_CRITICAL_PRIO_LE_DATA      (236U)
#define BT_CRITICAL_PRIO_LE_CONTROL   (238U)
#define BT_CRITICAL_PRIO_LE_INITIATOR (240U)
#define BT_CRITICAL_PRIO_LMP          (242U)
#define BT_CRITICAL_PRIO_SNIFF        (244U)
#define BT_CRITICAL_PRIO_CHAN_ASSESS  (246U)

#define BT_CRITICAL_PRIO_PAGE_SCAN  (222U)
#define BT_CRITICAL_PRIO_SYNC_TRAIN (224U)

#define BTCOEX_CRITICAL_PRIO (255U)

#define BTCOEX_A2DP_DEFAULT               (90)
#define BTCOEX_LOW_LATENCY_MODE_A2DP_LOW  (18)
#define BTCOEX_LOW_LATENCY_MODE_A2DP_HIGH (218)
#define BTCOEX_LOW_LATENCY_MODE_NONLINK   (20)

#define BTCOEX_IS_VALID_WEIGHT_GRP(wg) (wg < BT_COEX_INVALID)
#define BTCOEX_IS_VALID_WG_PRIO(prio)  ((prio > BTCOEX_MIN_PRIO) && (prio <= ((BTCOEX_MAX_PRIO))))

#define XPAN_QOS_PRIO    (83)
#define XPAN_NONQOS_PRIO (81)
/*-------------------------------------------------------------------------
 * Type Declarations
 * ----------------------------------------------------------------------*/
typedef enum coex_wghts_change_reason_t {
    COEX_WGHTS_CHANGE_REASON_TRANS_INIT = 0,
    COEX_WGHTS_CHANGE_REASON_HID_CRIT = 1,
    COEX_WGHTS_CHANGE_REASON_SM_IDLE = 2,
    COEX_WGHTS_CHANGE_REASON_START_BT_INTRVL = 3,
    COEX_WGHTS_CHANGE_REASON_START_WLAN_INTRVL = 4,
    COEX_WGHTS_CHANGE_REASON_START_CTS2S_INTRVL = 5,
    COEX_WGHTS_CHANGE_REASON_BT_GRANTED = 6,
    COEX_WGHTS_CHANGE_REASON_PRIO_IBSS = 7,
    COEX_WGHTS_CHANGE_REASON_INC_BCN_RX_PRIO = 8,
    COEX_WGHTS_CHANGE_REASON_RST_BCN_RX_PRIO = 9,
    COEX_WGHTS_CHANGE_REASON_BCN_TX_PRIO = 10,
    COEX_WGHTS_CHANGE_REASON_PSP_WLAN_CRIT = 11,
    COEX_WGHTS_CHANGE_REASON_PRIO_FC = 13,
    COEX_WGHTS_CHANGE_REASON_WLAN_COLLISION_CRIT = 14,
    COEX_WGHTS_CHANGE_REASON_WLAN_COLLISION_NORM = 15,
    COEX_WGHTS_CHANGE_REASON_BT_A2DP_CRITICAL = 16,
    COEX_WGHTS_CHANGE_REASON_WLAN_PAUSED = 17,
    COEX_WGHTS_CHANGE_REASON_ENTER_OCS = 18,
    COEX_WGHTS_CHANGE_REASON_PRIO_MGMT_TXRX = 19,
    COEX_WGHTS_CHANGE_REASON_WLAN_PEER_UNPAUSE = 20,
    COEX_WGHTS_CHANGE_REASON_WLAN_PRIO_CTS2SEXT = 21,
    COEX_WGHTS_CHANGE_REASON_RSTWLAN_PRIO_VDEV_PAUSE = 22,
    COEX_WGHTS_CHANGE_REASON_RSTWLAN_PRIO_VDEV_UNPAUSE = 23,
    COEX_WGHTS_CHANGE_REASON_WEIGHT_GROUP_6_TIMEOUT_REACHED = 24,
    COEX_WGHTS_CHANGE_REASON_DELAYED_ENTER_OCS = 25,
    COEX_WGHTS_CHANGE_REASON_BT_A2DP_NORMAL = 26,
    COEX_WGHTS_CHANGE_REASON_END_WLAN_DURATION = 27,
    COEX_WGHTS_CHANGE_REASON_END_BT_DURATION = 28,
    COEX_WGHTS_CHANGE_REASON_WLAN_OVER_ZBLOW = 29,
    COEX_WGHTS_CHANGE_REASON_LE_OVER_WLAN_TRAFFIC = 30,
    COEX_WGHTS_CHANGE_REASON_VBC_UPDATE = 31,
} E_COEX_WGHTS_CHANGE_REASON;

typedef struct _coex_whal_intf {
    uint32_t WlanCoexWeight[COEX_WGHT_TABLE_NUM_ROWS];
} coex_whal_intf;

/*
 *This structure stores the static weigths configurations for each weight group.
 */
typedef struct coex_wghts_static_wghts_t {
    uint8_t w0_b0; /* BTCOEX_WL_WEIGHT0_VALUE0 ; // wl_idle + !tm_wl_wait_beacon */
    uint8_t w0_b1; /* BTCOEX_WL_WEIGHT0_VALUE1 ; // sw_ctrl[3] - all_stomp */
    uint8_t w0_b2; /* BTCOEX_WL_WEIGHT0_VALUE2 ; // sw_ctrl[2] - all_not_stomp */
    uint8_t w0_b3; /* BTCOEX_WL_WEIGHT0_VALUE3 ; // sw_ctrl[1] - pa_pre_distortion */

    uint8_t w1_b0; /* BTCOEX_WL_WEIGHT1_VALUE0 ; // sw_ctrl[0] - general purpose */
    uint8_t w1_b1; /* BTCOEX_WL_WEIGHT1_VALUE1 ; // tm_wl_wait_beacon */
    uint8_t w1_b2; /* BTCOEX_WL_WEIGHT1_VALUE2 ; // ts_state_wait_ack_cts */
    uint8_t w1_b3; /* BTCOEX_WL_WEIGHT1_VALUE3 ; // self_gen */

    uint8_t w10_b0; /* BTCOEX_WL_WEIGHT10_VALUE0 ; // level 3 */
    uint8_t w10_b1; /* BTCOEX_WL_WEIGHT10_VALUE1 ; // level 2 */
    uint8_t w10_b2; /* BTCOEX_WL_WEIGHT10_VALUE2 ; // level 1 */
    uint8_t w10_b3; /* BTCOEX_WL_WEIGHT10_VALUE3 ; // 0 */
} coex_wghts_static_wghts;

/* This structure stores the weight group Vs priority map. This is used to create a static lookup map
 * from which the default priorities of the frames are picked up.
 */
typedef struct coex_wghtgrp_prio_map {
    uint32_t wght_grp;
    uint8_t tx_prio[COEX_WLAN_TX_PRIO_INDX_MAX];
    uint8_t rx_prio[COEX_WLAN_RX_PRIO_INDX_MAX];
} coex_wghtgrp_prio_map_t;

typedef struct btcoex_weight_t {
    uint32_t prev_wghts[32];
    uint8_t CurWeightGroup;
    uint8_t BtWeightGroup;    // Wlan priority used during BT allocated bandwidth
    uint8_t WlanWeightGroup;  // Wlan priority used after BT allocated bandwidth
    uint8_t WlanIsIdleOverride;
    uint8_t WeightOption;
    uint8_t WlanBeaconRxProtectOverride;
    uint8_t HidConcurrentTxOverride;
    uint8_t HidConcurrentTxOverrideCritical;
    uint8_t PrioritizeWlanFCFrames;
    uint8_t PrioritizeWlanDuringCollisions;
    uint8_t WlanBeaconTxProtectOverride;            /*beacon tx protect */
    uint8_t WlanBeaconTxWeight;                     /*beacon tx weight */
    uint8_t wlan_state_based_elevated_connect_prio; /* Connection pacekt weight override */
    uint8_t wlan_state_based_elevated_scan_prio;    /* Probe req/resonse packet weight override */
    uint8_t WlanMgmtFramesWeight;
    uint8_t WlanCritRxOverride;
    uint8_t prioritizeSelfgenTxOverA2dpNormal;
    bool resetWtVdevPause;
    uint8_t PrioritizeCTS2SForPauseUnpause;
    uint8_t prioritizeSelfgenTxOverpage;
    uint8_t XPANOverride;
    uint8_t XPANVBCOverride;
    uint8_t XPANVBCQos;
    uint8_t TxSelfgen5GOverride;
} BTCOEX_WEIGHT_STRUCT;

void coex_utils_print_wghts(const uint32_t *wght_array);
void btcoex_setweight(uint8_t WlanWeightGroup, __attribute__((__unused__)) uint32_t reason);
void btcoex_weights_override(uint8_t wght_grp, uint32_t reason);
uint8_t coex_recalc_frm_prio_mgmt(uint32_t wght_grp, E_COEX_PRIO_FRM_TYPE type, bool is_tx);
uint8_t coex_recalc_frm_prio_ctrl(uint32_t wght_grp, E_COEX_PRIO_FRM_TYPE type, bool is_tx);
uint8_t coex_recalc_frm_prio_data(uint32_t wght_grp, E_COEX_PRIO_FRM_TYPE type, bool is_tx);
uint8_t coex_recalc_frm_prio_others(uint32_t wght_grp, E_COEX_PRIO_FRM_TYPE type, bool is_tx);
uint8_t coex_recalc_wlan_frame_priority(uint32_t wght_grp, E_COEX_PRIO_FRM_TYPE type, bool is_tx);
uint8_t coex_wghts_grp2prio(uint8_t group);
void coex_update_vbc_prio(uint8_t override_enable, uint8_t qos, uint8_t netif_id);
void coex_prioritize_flow_control_frames(uint8_t Enable, uint8_t FlowCtrlID, uint8_t update_weights);

#endif /* SUPPORT_COEX */
#endif /* #ifndef _COEX_WGHTS_H_ */
