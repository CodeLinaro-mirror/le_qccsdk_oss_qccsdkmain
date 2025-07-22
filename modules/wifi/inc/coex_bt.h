/*========================================================================
Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
*
* @file coex_bt.h
* @brief Coex bt params and struct definitions
*========================================================================*/
#ifndef _COEX_BT_H_
#define _COEX_BT_H_
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
#include "coex_api.h"
#include "coex_gpm.h"
#include "coex_mci.h"
#include "coex_sched.h"

/*-------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * ----------------------------------------------------------------------*/

#define COEX_GET_ACTIVE_SCAN(optype) \
    (gpBtCoexBtInfoDev->bt_scan_bitmap & ((0x1 << (optype)) << COEX_BT_ACT_SCAN_BMAP_SHIFT))

#define MAX_REMOTE_BT_LINK (72)
#define MAX_LOCAL_BT_LINK  (10)
#define MAX_NSE            (32)
#define MAX_BT_NONLINKS    (4)
#ifdef PLATFORM_NT
#define MAX_BT_NONLINK_CRT_LINK (MAX_BT_NONLINKS)
#endif /* PLATFORM_NT */
#define INVALID_BT_LINKID   (0xFF)
#define MAX_BT_NUM_PROFILES (2)

#define COEX_BT_LINK(RemoteId)            (gpBtCoexBtInfoDev->BtLink[gpBtCoexBtInfoDev->LocalBtLinkID[(RemoteId)]])
#define COEX_BT_LINK_PTR(RemoteId)        (&COEX_BT_LINK(RemoteId))
#define COEX_BT_LINK_IS_INVALID(RemoteId) (gpBtCoexBtInfoDev->LocalBtLinkID[(RemoteId)] == INVALID_BT_LINKID)
#define COEX_BT_LINK_INFO(RemoteId)       (gpBtCoexBtInfoDev->BtLinkInfo[gpBtCoexBtInfoDev->LocalBtLinkID[(RemoteId)]])
#define COEX_BT_LINK_INFO_PTR(RemoteId)   (&COEX_BT_LINK_INFO((RemoteId)))

#define BT_NON_LINK_CRITICAL_BUDGET (30)
#define BT_NON_LINK_MAX_BUDGET      (40)

#define NUM_BT_PROF_RFCOMM         (gpBtCoexBtInfoDev->NumOfBtRFCOMMProfiles - gpBtCoexBtInfoDev->NumOfPwrSaveBtRFCOMMProfiles)
#define NUM_BT_PROF_PWRSAVE_RFCOMM (gpBtCoexBtInfoDev->NumOfPwrSaveBtRFCOMMProfiles)
#define NUM_BT_PROF_A2DP           (gpBtCoexBtInfoDev->NumOfBtA2DPProfiles - gpBtCoexBtInfoDev->NumOfPwrSaveBtA2DPProfiles)
#define NUM_BT_PROF_BNEP           (gpBtCoexBtInfoDev->NumOfBtBNEPProfiles - gpBtCoexBtInfoDev->NumOfPwrSaveBtBNEPProfiles)
#define NUM_BT_PROF_PWRSAVE_BNEP   (gpBtCoexBtInfoDev->NumOfPwrSaveBtBNEPProfiles)
#define NUM_BT_PROF_VOICE          (gpBtCoexBtInfoDev->NumOfBtVoiceProfiles)
#define NUM_BT_BTLE_LINK           (gpBtCoexBtInfoDev->NumOfBTLEProfiles)
#define NUM_BT_PROF_BA             (gpBtCoexBtInfoDev->NumOfBAProfiles)
#define NUM_BT_PROF_APTX_LL        (gpBtCoexBtInfoDev->NumOfAPTXLowLatencyProfile)
#define NUM_BT_PROF_XPAN           (gpBtCoexBtInfoDev->NumOfA2DPoXPANProfiles)

#define BT_LINK_STATE_IDLE              (0x00)
#define BT_LINK_STATE_ACTIVE            (0x01)
#define BT_LINK_STATE_CRITICAL          (0x02)
#define BT_LINK_STATE_POWER_SAVE_BITMAP (0x10)
#define BT_LINK_STATE_ACTIVE_BITMAP     (0x0F)

#define COEX_SET_BT_IDLE(state)          ((state) = BT_LINK_STATE_IDLE)
#define COEX_SET_BT_ACTIVE(state)        ((state) = (((state)&BT_LINK_STATE_POWER_SAVE_BITMAP) | BT_LINK_STATE_ACTIVE))
#define COEX_SET_BT_CRITICAL(state)      ((state) = (((state)&BT_LINK_STATE_POWER_SAVE_BITMAP) | BT_LINK_STATE_CRITICAL))
#define COEX_CLR_BT_LINK_PWR_SAVE(state) (CLR_FLAG_BIT(BT_LINK_STATE_POWER_SAVE_BITMAP, (state)))
#define COEX_SET_BT_LINK_PWR_SAVE(state) (SET_FLAG_BIT(BT_LINK_STATE_POWER_SAVE_BITMAP, (state)))

#define COEX_IS_BT_LINK_PWR_SAVE(state) (state & BT_LINK_STATE_POWER_SAVE_BITMAP)
#define COEX_IS_BT_LINK_ACTIVE(state)   ((state & BT_LINK_STATE_ACTIVE_BITMAP) == BT_LINK_STATE_ACTIVE)
#define COEX_IS_BT_LINK_CRITICAL(state) ((state & BT_LINK_STATE_ACTIVE_BITMAP) == BT_LINK_STATE_CRITICAL)
#define COEX_IS_BT_LINK_IDLE(state)     (state == BT_LINK_STATE_IDLE)

#define IS_CSB_ACTIVE() (NUM_BT_PROF_BA && (FALSE == gpBtCoexBtInfoDev->csb_cfg->paused))

// BTCoex Link Type
#define COEX_BT_TYPE_LE  (0x10)
#define COEX_BT_TYPE_BDR (0x40)

#define LOW_PRIO_ACL_WEIGHT_PERCENTAGE (40)

#define A2DP_EDR_WEIGHT_PERCENTAGE (25)
#define A2DP_BDR_DELTA             (10)
#define BT_SLAVE_DELTA             (10)
#define BT_SINK_DELTA              (12)

// more profiles
#define NUM_BT_PROF_3DD                  (NUM_BT_3DD_LINK)
#define NUM_BT_PROF_ACL                  (NUM_BT_PROF_RFCOMM + NUM_BT_PROF_BNEP + NUM_BT_PROF_A2DP)
#define NUM_BT_PROF_HI_PRIO_ACL          NUM_BT_PROF_A2DP
#define NUM_BT_PROF_LOW_PRIO_ACL         (NUM_BT_PROF_RFCOMM + NUM_BT_PROF_BNEP)
#define NUM_BT_PROF_PWRSAVE_LOW_PRIO_ACL (NUM_BT_PROF_PWRSAVE_RFCOMM + NUM_BT_PROF_PWRSAVE_BNEP)
#define NUM_BT_PROF_BTLE_BASIC           (NUM_BT_BTLE_LINK)

#define NUM_BT_PROF coex_bt_num_profiles()

/* Following are the default priorities advertised by BT over the MCI.
 * This will be the priority seen by CXC in the Contention INFO/NACK/RST etc.
 * Critical priorities are derived by adding 128 to the default priority.
 */
#define BT_DEFAULT_PRIO_IDLE                       (0)
#define BT_DEFAULT_PRIO_POLL                       (4)
#define BT_DEFAULT_PRIO_INQ_SCAN                   (8)
#define BT_DEFAULT_PRIO_INQ_SCAN_RESP              (10)
#define BT_DEFAULT_PRIO_PAGE_SCAN                  (12)
#define BT_DEFAULT_PRIO_LE_ADVERTISER              (14)
#define BT_DEFAULT_PRIO_LE_SCANNER                 (16)
#define BT_DEFAULT_PRIO_LLR_TRIGGER_SCAN           (18)
#define BT_DEFAULT_PRIO_CHAN_ASSESS                (20)
#define BT_DEFAULT_PRIO_ACL_1MBPS_PRIO             (30)
#define BT_DEFAULT_PRIO_ACL_2MBPS_PRIO             (32)
#define BT_DEFAULT_PRIO_ACL_3MBPS_PRIO             (34)
#define BT_DEFAULT_PRIO_LLR_BEACON                 (36)
#define BT_DEFAULT_PRIO_BT_TEST_MODE               (40)
#define BT_DEFAULT_PRIO_RFCOMM                     (50)
#define BT_DEFAULT_PRIO_BNEP                       (52)
#define BT_DEFAULT_PRIO_LMP                        (56)
#define BT_DEFAULT_PRIO_INQ                        (58)
#define BT_DEFAULT_PRIO_INQ_RESP                   (60)
#define BT_DEFAULT_PRIO_SYNC_TRAIN                 (62)
#define BT_DEFAULT_PRIO_PAGE                       (64)
#define BT_DEFAULT_PRIO_ROLE_SWITCH                (68)
#define BT_DEFAULT_PRIO_LLR_TRIGGER                (70)
#define BT_DEFAULT_PRIO_CONNECTIONLESS_SLAVE_BCAST (72)
#define BT_DEFAULT_PRIO_SNIFF                      (74)
#define BT_DEFAULT_PRIO_LE_DATA                    (76)
#define BT_DEFAULT_PRIO_LE_CONTROL                 (78)
#define BT_DEFAULT_PRIO_QOS                        (80)
#define BT_DEFAULT_PRIO_A2DP                       (90)
#define BT_DEFAULT_PRIO_HID_ACTIVITY_REPORTED      (94)
#define BT_DEFAULT_PRIO_HID                        (96)
#define BT_DEFAULT_PRIO_LE_INITIATOR               (64)
#define BT_DEFAULT_PRIO_APTX_LL                    (109)
#define BT_DEFAULT_PRIO_VOICE_SCO                  (110)
#define BT_DEFAULT_PRIO_VOICE_EV3                  (112)
#define BT_DEFAULT_PRIO_VOICE_2EV3                 (114)
#define BT_DEFAULT_PRIO_VOICE_2EV5                 (116)
#define BT_DEFAULT_PRIO_VOICE_3EV3                 (118)
#define BT_DEFAULT_PRIO_VOICE_3EV5                 (120)
#define BT_DEFAULT_PRIO_VOICE_RETRAX               (122)

#define BTCOEX_BTPRIO_WRITE  (0)
#define BTCOEX_BTPRIO_READ   (1)
#define BTCOEX_BTPRIO_RETURN (2)

#define APTX_LL_WEIGHT_PERCENTAGE (20) /*20 Value calculated by 2.5ms(4BT Slots)/13ms(T for aptX LL) x 100 */
#define BT_PERCENTAGE             (50)
#define BT_MISC_LOW_PRIO_PERCT    (15)

#define COEX_NUM_BT_CHANNELS (78)
#define BT_SLOT_TIME         (625)

#define COEX_BT_SLOTS2US(slots) ((slots)*BT_SLOT_TIME)

// get role value according to coex version
#define COEX_BT_ROLE(role) coex_get_bt_role((role))

#define GPM_SET_CHANNEL_BIT(_p_gpm, _bt_chan)                                 \
    do {                                                                      \
        if (_bt_chan < COEX_NUM_BT_CHANNELS) {                                \
            *(((uint8_t *)(_p_gpm)) + (_bt_chan / 8)) |= 1 << (_bt_chan & 7); \
        }                                                                     \
    } while (0)

#define GPM_CLR_CHANNEL_BIT(_p_gpm, _bt_chan)                                    \
    do {                                                                         \
        if (_bt_chan < COEX_NUM_BT_CHANNELS) {                                   \
            *(((uint8_t *)(_p_gpm)) + (_bt_chan / 8)) &= ~(1 << (_bt_chan & 7)); \
        }                                                                        \
    } while (0)

#define COEX_INACTIVE_SCAN_CB_REQUIRED \
    (IS_COEX_SCHED_SCAN_INACTIVE_CB && (pScanActivityInfo->IsStart == 0) && (gpBtCoexBtInfoDev->BtScanActivity == 0))
#define COEX_ACTIVE_SCAN_CB_REQUIRED \
    (IS_COEX_SCHED_SCAN_ACTIVE_CB && (pScanActivityInfo->IsStart) && (gpBtCoexBtInfoDev->BtScanActivity))
#define COEX_SCAN_ACTIVITY_BITMAP                                                                                    \
    (1 << BT_SCAN_TYPE_INQSCAN | 1 << BT_SCAN_TYPE_PSCAN | 1 << BT_SCAN_TYPE_LESCAN | 1 << BT_SCAN_TYPE_LE_CONNECT | \
     1 << BT_SCAN_TYPE_LE_ADV | 1 << BT_SCAN_TYPE_LESCAN_EXT | 1 << BT_SCAN_TYPE_LE_INIT_EXT |                       \
     1 << BT_SCAN_TYPE_LE_ADV_EXT)

#define COEX_BLE_SCAN_MAX_DUTY_CYCLE (45)

#define COEX_BTLE_SCAN_TRF_THRSHLD         (0xB0)
#define COEX_BTLE_SCAN_TRF_THRSHLD_NO_A2DP (0xFFFF)

#define COEX_BLE_MAX_INTERVAL                  (0xFFFF)
#define COEX_BLE_SCAN_NONLINK_THRESHOLD        (48)   // Interval threshold
#define COEX_BLE_SCAN_NONLINK_WINDOW_THRESHOLD (160)  // 100msecs. 160(slots) = 160 x 0.625msecs = 100msecs
#define COEX_BLE_HI_FREQ_BLE_LINK_THRESHOLD    (32)
#define COEX_BLE_NUM_HI_FREQ_LOWER_THRESHOLD   (1)
#define COEX_BLE_NUM_HI_FREQ_UPPER_THRESHOLD   (2)
#define COEX_BLE_MIN_SCAN_WINDOW               (2)

#define BTLE_SCAN_CRITICAL_THRESHOLD_SLOT   (60)
#define BTLE_SCAN_INTRVL_CRITICAL_THRESHOLD (12)  // Use eSCO interval as reference

#define BTLE_CONNECT_SCAN_CRITICAL_INTERVAL (1024)
#define BTLE_CONNECT_SCAN_CRITICAL_RATIO    (25)  // Window:Interval -> 1:25

#define BT_CONNECTLESS_CRITICAL_THRESHOLD     (12)  // Use eSCO interval as reference
#define BT_CONNECTLESS_NON_CRITICAL_THRESHOLD (92)  // > 60msec

#define COEX_BLE_SCAN_NONLINK_DC_THRESHOLD (48)  // Duty cycle threshold

/* NONLINK EXTRAS */
#define NONLINK_EXTRA_PAGE   3
#define NONLINK_EXTRA_INQ    2
#define NONLINK_EXTRA_BTLE   0
#define NONLINK_EXTRA_LESCAN 1

#define BT_LOW_PRIO_ACL_PROF_SCENARIO ((NUM_BT_PROF_LOW_PRIO_ACL) && (NUM_BT_PROF_LOW_PRIO_ACL == NUM_BT_PROF))
#define IS_COEX_BT_SCO_PRESENT        (gpBtCoexBtInfoDev->BtIdleTime && gpBtCoexBtInfoDev->BtIdleTime <= 2500)

#define IS_COEX_BT_XSCO_PRESENT \
    (NUM_BT_PROF_VOICE && (gpBtCoexBtInfoDev->BtIdleTime && gpBtCoexBtInfoDev->BtIdleTime > 2500))

#define IS_BT_SCAN_CRITICAL(Window, Interval) \
    (((Interval <= BTLE_SCAN_INTRVL_CRITICAL_THRESHOLD) || ((Window << 1) >= Interval)) && (Window) && (Interval))

#define BT_LE_INITIATOR_PRIO_HIGH (80)

#define COEX_LOW_PRIO_ACL_N_NONLINK_TRAFFIC                                                   \
    ((NUM_BT_PROF_LOW_PRIO_ACL &&                                                             \
      (gpBtCoexSchedDev->IsHeavyTraffic || coex_prof_info(BT_PROF_RFCOMM, NUM_SLAVE, NULL) || \
       coex_prof_info(BT_PROF_BNEP, NUM_SLAVE, NULL))) ||                                     \
     coex_is_nonlink_active() || coex_acl_interact_is_active(COEX_ACL_INT_MODE_ANY))

#define BTCOEX_BT_TRAFFIC_TRACKING_REQUIRED(wlan_2g)                                                       \
    (!(gpBtCoexWlanInfoDev->mac[(wlan_2g)].state & (COEX_WLAN_SYS_CONNECTING | COEX_WLAN_SYS_SECURITY)) && \
     /*BT_LOW_PRIO_ACL_PROF_SCENARIO*/ NUM_BT_PROF_LOW_PRIO_ACL)
#define BTCOEX_BT_SLAVE_INACTIVITY_TRACKING_REQUIRED(wlan_2g)                                              \
    (!(gpBtCoexWlanInfoDev->mac[(wlan_2g)].state & (COEX_WLAN_SYS_CONNECTING | COEX_WLAN_SYS_SECURITY)) && \
     (coex_prof_info(BT_PROF_RFCOMM, NUM_SLAVE, NULL) || coex_prof_info(BT_PROF_BNEP, NUM_SLAVE, NULL)))

#define COEX_POLICY_BT_BW_LIMIT_ENABLE (0x00000001)

#define COEX_MIN_INTERVAL(i)                                                                \
    do {                                                                                    \
        if (gpBtCoexBtInfoDev->MinIntervalLinkID == INVALID_BT_LINKID) {                    \
            gpBtCoexBtInfoDev->MinIntervalLinkID = gpBtCoexBtInfoDev->BtLink[i].LinkID;     \
            gpBtCoexBtInfoDev->MinInterval = gpBtCoexBtInfoDev->BtLink[i].TInterval;        \
        } else {                                                                            \
            if (gpBtCoexBtInfoDev->BtLink[i].TInterval < gpBtCoexBtInfoDev->MinInterval) {  \
                gpBtCoexBtInfoDev->MinIntervalLinkID = gpBtCoexBtInfoDev->BtLink[i].LinkID; \
                gpBtCoexBtInfoDev->MinInterval = gpBtCoexBtInfoDev->BtLink[i].TInterval;    \
            }                                                                               \
        }                                                                                   \
    } while (0)

#define BT_BA_BW_DEFAULT       (22)
#define BT_MOUSE_BW_DEFAULT    (22)
#define BT_PWR_SAVE_BW_DEFAULT (22)
#define BT_SCO_BW_DEFAULT      (33)
#define BT_LE_BW_DEFAULT       (22)
#define BT_3DD_BW_DEFAULT      (22)

/* Percentage threshold for light/heavy BT Tx traffic:
 * 6% of BT Interval for Normal Mode
 * 10% of BT Interval for WLM Mode (Not supported by Fermion)
 */
#define BTCOEX_RFCOMM_TRAFFIC_PERCENTAGE_THRESH 6
#define COEX_BT_PWR_SAVE_THRESHOLD              (100)  // BT slots

// types of BT scans
typedef enum bt_scan_type_t {
    BT_SCAN_TYPE_ASSESSMENT = 0,
    BT_SCAN_TYPE_PSCAN = 1,
    BT_SCAN_TYPE_INQSCAN = 2,
    BT_SCAN_TYPE_LESCAN = 3,
    BT_SCAN_TYPE_LE_CONNECT = 4,
    BT_SCAN_TYPE_LE_ADV = 5,
    BT_SCAN_TYPE_ACT_SYNC_TRAIN = 8,
    BT_SCAN_TYPE_ACT_SYNC_TRAIN_SCAN = 9,
    BT_SCAN_TYPE_ACT_CBS_TX = 10,
    BT_SCAN_TYPE_ACT_CBS_RX = 11,
    BT_SCAN_TYPE_LESCAN_EXT = 12,
    BT_SCAN_TYPE_LE_INIT_EXT = 13,
    BT_SCAN_TYPE_LE_ADV_EXT = 14,
} E_BT_SCAN_TYPE;

/*-------------------------------------------------------------------------
 * Type Declarations
 * ----------------------------------------------------------------------*/

enum {
    BT_SCAN_PAGE_STANDARD = 0,
    BT_SCAN_PAGE_INTERLACED = 1,
    BT_SCAN_INQ_STANDARD = 2,
    BT_SCAN_INQ_INTERLACED = 3,
    BT_SCAN_LE_FOREGRND_SCAN = 4,
    BT_SCAN_LE_BKGRND_SCAN = 5,
    BT_SCAN_LE_INITIATOR_SCAN = 6,
    BT_SCAN_LE_ADV = 7,
    BT_SYNC_TRAIN_TX = 10,
    BT_SCAN_SYN_TRAIN = 11,
    BT_CSB_TX = 12,
    BT_CSB_RX = 13,
    BT_SCAN_LE_EXT_SCAN = 14,
    BT_SCAN_LE_EXT_INITIATOR = 15,
    BT_SCAN_LE_EXT_ADVERTISOR = 16,
};

enum {
    BT_SCAN_TYPE_PAGE = 0,
    BT_SCAN_TYPE_INQ = 1,
    BT_SCAN_TYPE_LE_SCAN = 2,
    BT_SCAN_TYPE_LE_INIT_SCAN = 3,
    BT_SCAN_TYPE_LE_ADV_SCAN = 4,
    BT_SCAN_TYPE_SYNC_TRAIN = 8,
    BT_SCAN_TYPE_MAX = 9,
};

enum {
    BT_COEX_BASE = 0,
    BT_COEX_LOW = 1,
    BT_COEX_MID = 2,
    BT_COEX_MID_NONSYNC = 3,
    BT_COEX_HI_NONVOICE = 4,
    BT_COEX_HI = 5,
    BT_COEX_CRITICAL_NONSYNC = 6,
    BT_COEX_CRITICAL = 7,
    BT_COEX_INVALID = 8,
};

typedef enum {
    E_COEX_BT_BW_BELOW_THRESH = 1, /* Default level */
    E_COEX_BT_BW_ABOVE_THRESH = 2, /* Considers equal to threshold case also here. */
} E_COEX_BT_BW_LEVEL;

typedef enum {
    E_COEX_BT_STATUS_QUERY_ALL_INFO = 0,
    E_COEX_BT_STATUS_QUERY_ACK_STATUS = 1,
} E_COEX_BT_STATUS_WL_BITMAP;

typedef enum _wal_coex_bt_link_type {
    COEX_BT_LINK_TYPE_UNKOWN = 0,
    COEX_BT_LINK_TYPE_BR_EDR = 1, /* Not used in Coex code for now. Use the link rate to identify. */
    COEX_BT_LINK_TYPE_LE = 2,
} E_COEX_BT_LINK_TYPE;

typedef enum _wal_coex_bt_link_rate {
    COEX_BT_LINK_RATE_BR = 0,
    COEX_BT_LINK_RATE_EDR = 1,
    COEX_BT_LINK_RATE_QHS = 2,
    COEX_BT_LINK_RATE_UNKNOWN = 0xFF
} E_COEX_BT_LINK_RATE;

enum {
    BT_MASTER = 0,
    BT_SLAVE = 1,
    BT_ROLE_MAX = 2,
};

enum {
    COEX_BT_PRIO_NONE = 0,
    COEX_BT_HI_PRIO_ONLY = 1,
    COEX_BT_LOW_PRIO_ONLY = 2,
    COEX_BT_HI_AND_LOW_PRIO = 3,
};

enum {
    BT_VPktType_HV1 = 0,
    BT_VPktType_HV2 = 1,
    BT_VPktType_HV3 = 2,
    BT_VPktType_EV3 = 3,
    BT_VPktType_EV4 = 4,
    BT_VPktType_EV5 = 5,
    BT_VPktType_2EV3 = 6,
    BT_VPktType_2EV5 = 7,
    BT_VPktType_3EV3 = 8,
    BT_VPktType_3EV5 = 9,
};

enum eBTOperationType {
    /* BT op type */
    BT_OP_NONE = 0,
    BT_OP_PAGE = 1,
    BT_OP_INQ = 2,
    BT_OP_SLAVE_PAGE = 3,
    /* WLAN internal use only */
    BT_OP_LE_SCAN = 4,
    BT_OP_LE_CONNECT_SCAN = 5,
    BT_OP_LE_ADV = 6,
    BT_OP_SYNC_TRAIN = 10,
    BT_OP_MAX = 11,
};

typedef enum bt_profile {
    BT_PROF_UNKNOWN = 0,
    BT_PROF_RFCOMM = 1,
    BT_PROF_A2DP = 2,
    BT_PROF_HID = 3,
    BT_PROF_BNEP = 4,
    BT_PROF_VOICE = 5,
    BT_PROF_A2DPVOICE = 6,
    BT_PROF_BA = 7, /* Broadcast Audio (Connectionless Slave Broadcast. */
    BT_PROF_APTX_LOW_LATENCY = 8,
    BT_PROF_RVP = 9,
    BT_PROF_BA_DUP = 10,
    BT_PROF_A2DPoXPAN = 11,
    BT_PROF_VOICEoXPAN = 12,

    /* Following profile defenitions are internal to Coex. Starting this from 16 as BT internally uses a 16 bit bimap
     * for the profiles. Any new profile defenitions from BT should be added above this.
     */
    BT_BTLE_PROF_BASIC = 16,
    BT_3DD_PROF = 18,
    BT_PROF_MAX = 19,
    BT_PROF_ALL = 0xFF,
} E_BT_PROFILE;

// remove/add BT profiles
enum {
    BTCOEX_REMOVE_PROFILE = FALSE,
    BTCOEX_ADD_PROFILE = TRUE,
};

#ifdef PLATFORM_NT
enum {
    MCI_QUERY_ALL_INFO = 0x1,
};
#endif /*#ifdef PLATFORM_NT */

// types of information for BT profiles
enum {
    NUM_CRITICAL = 0,
    NUM_EDR = 1,
    NUM_BDR = 2,
    NUM_PERIOD_NO_MORE_THAN = 3,
    NUM_PERIOD_NO_LESS_THAN = 4,
    NUM_SLAVE = 5,
    NUM_SINK = 6,
    NUM_LE = 7,
    NUM_PWR_SAVE = 9,
    NUM_ACTIVE = 10,
    NUM_A2DP_CODEC_LOW_DEF = 11,
    NUM_A2DP_CODEC_HI_DEF = 12,
    BT_QOS_PCT = 13,
    NUM_QHS = 14,
};

enum { BT_NONLINK_EVENT = 0, BT_LINKBASED_EVENT };

enum { BT_PERFORMANCE_NORMAL = 0, BT_PERFORMANCE_CRITICAL = 1, BT_PERFORMANCE_CRITICAL_ELEVATE = 2 };

typedef enum {
    BT_LINK_STATUS_ACTIVE = 0x00,
    BT_LINK_STATUS_PWR_SAVE = 0x01,
    BTLE_LINK_STATUS_CONNECT = 0x10,
    BTLE_LINK_STATUS_LINK_UP = 0x11,
    BTLE_LINK_STATUS_DISCONNECT = 0x12,
    BTLE_LINK_STATUS_ISO_CONNECTING = 0x13,   /* Connection establishing. */
    BTLE_LINK_STATUS_ISO_CONNECTED = 0x14,    /* Connected. */
    BTLE_LINK_STATUS_ISO_DISCONNECTED = 0x15, /* Disconnected. */
    BTLE_LINK_STATUS_ASHA_CONNECTING = 0x16,
    BTLE_LINK_STATUS_ASHA_CONNECTED = 0x17,
    BTLE_LINK_STATUS_ASHA_DISCONNECTED = 0x18,
    BT_LINK_STATUS_INVALID = 0xFF,
} E_COEX_BT_LINK_OP;

enum {
    A2DP_CODEC_LOW_DEF = 0,
    A2DP_CODEC_HI_DEF = 1,
};

// Coex SM stimulus
enum {
    COEX_BT_STATE_CH_BT_ON = 0,
    COEX_BT_STATE_CH_PRF_GPM = 1,
    COEX_BT_STATE_CH_STAT_UPDATE = 2,
    COEX_BT_STATE_CH_RESET_TPLY = 3,
    COEX_BT_STATE_CH_BT_OFF = 4,
};

enum {
    COEX_ACL_INT_MODE_NONE = 0,
    COEX_ACL_INT_MODE_ROLE_SWITCH = 1,
    COEX_ACL_INT_MODE_PROF_CONN = 2,
    COEX_ACL_INT_MODE_ANY = 3,
};

/* Configurations for Connectionless Slave Broadcast (Broadcast Audio). */
typedef struct _coex_csb_cfg {
    uint8_t enabled;      /* CSB is active or not. */
    uint8_t critical;     /* CSB link is critical. */
    uint8_t paused;       /* Link is paused. Interval will be long.
                           * Happens when operating concurrently with HFP. */
    uint8_t interval;     /* in BT slot units. */
    uint8_t window;       /* in BT slot units. */
    uint32_t packet_type; /* Bluetooth packet type used. */
} coex_csb_cfg;

typedef struct _coex_acl_interactive_link {
    uint8_t link_id;     /* Link id of the acl interactive link. */
    uint8_t reason;      /* Role switch or profile connection. */
    uint32_t time_slice; /* For role switch only. */
    uint8_t state;       /* active or inactive. */
} coex_acl_interactive_link;

#define COEX_ACL_INT_MAX_LINKS 8
typedef struct _coex_acl_interactive_cfg {
    coex_acl_interactive_link
        links[COEX_ACL_INT_MAX_LINKS]; /* Stores the information about the ACL interactive links. */
    uint8_t acl_interactive_bitmap;    /* Bit map to plot the state (active/inactive) of all ACL interactive links. */
} coex_acl_interactive_cfg;

typedef struct t_ScanInfo {
    uint8_t Enabled;
    uint8_t LinkID;
    uint16_t Interval;
    uint16_t Window;
} tScanInfo;

typedef struct t_BtLinkInfo {
    uint32_t LastBtInactivityReport;
    uint32_t nextStarttime;
#ifdef COEX_BT_STATS
    uint32_t NumOfIntervals;
    uint32_t NumOfCriticalIntervals;
    uint32_t NumOfCriticalInstances;
    uint8_t NumOfCriticalInstancesInOneInterval;
#endif
    uint16_t LinkTO;
    uint16_t ConnLatency;
    uint16_t WSize;
    uint16_t WOffset;
    uint8_t IsBtInactReportPending;
    uint8_t BTLinkOp;
    uint8_t A2dpSink;
    uint8_t BtLinkBwPct;  // BT Link Bandwidth Requirement Percentage
    int8_t BtRssi;        // dbm
    int8_t BtTxPower;     // dbm
    E_COEX_BT_LINK_TYPE LinkType;
    uint8_t DataRateTx;
    uint8_t DataRateRx;
} tBtLinkInfo;

/* This structure is to track the non-links.
 * This is updated from the Status Update GPM.
 */
typedef struct wal_btcoex_nonlink_link_t {
    uint8_t link_id; /* Link Identifier of the BT link. */
    uint8_t type;    /* The type of non-link. eg: Page Scan, Inq Scan etc. */
    uint8_t active;  /* If the state is active (critical) or not.*/
} wal_btcoex_nonlink_link;

typedef struct btcoex_info_t {
    /* Bt Link Info */
    tBtProfileGpm BtLink[MAX_LOCAL_BT_LINK];
    tBtLinkInfo BtLinkInfo[MAX_LOCAL_BT_LINK];

    uint32_t BtIdleTime;
    uint32_t MinInterval;
    uint8_t MinIntervalLinkID;

    uint32_t btRfcommIntervalThresh;

    /* Scan Info */
    tScanInfo BtScanInfo[BT_SCAN_TYPE_MAX];
    uint32_t BtScanActivity;

    coex_acl_interactive_cfg *acl_int_cfg;
    /* Profile Info */
    uint8_t LocalBtLinkID[MAX_REMOTE_BT_LINK];       /* Remote BT Link ID to Coex BT Link Index Mapping */
    uint8_t RFCOMMProfiles[MAX_BT_NUM_PROFILES];     /* List of RFCOMM Profiles */
    uint8_t BNEPProfiles[MAX_BT_NUM_PROFILES];       /* List of RFCOMM Profiles */
    uint8_t A2DPProfiles[MAX_BT_NUM_PROFILES];       /* List of A2DP Profiles */
    uint8_t VoiceProfiles[MAX_BT_NUM_PROFILES];      /* List of Voice Profiles */
    uint8_t A2DPoXPANProfiles[MAX_BT_NUM_PROFILES];  /* List of Voice Profiles */
    uint8_t VoiceoXPANProfiles[MAX_BT_NUM_PROFILES]; /* List of A2DPVoice Profiles */
    uint8_t BTLEProfiles[MAX_BT_NUM_PROFILES];       /* List of BTLE Profiles */
    uint8_t BAProfiles[MAX_BT_NUM_PROFILES];
    uint8_t AptXLowLatencyProfiles[MAX_BT_NUM_PROFILES];
    uint8_t BtNonLinkCriticalID[MAX_BT_NONLINK_CRT_LINK];
    uint8_t BtNonLinkCriticalNum;
    wal_btcoex_nonlink_link nonlink_links[MAX_BT_NONLINKS]; /* tracks the not connected links. */

    uint8_t BtNonLinkNum;
    uint8_t MinFreeLocalBtLinkID;
    uint8_t NumOfBtProfiles;
    uint8_t NumOfBtRFCOMMProfiles;
    uint8_t NumOfBtBNEPProfiles;

    uint8_t NumOfBtVoiceProfiles;
    uint8_t NumOfBtA2DPProfiles;
    uint8_t NumOfBTLEProfiles;

    uint8_t NumOfBAProfiles; /* Should not be more than 1. */
    uint8_t NumOfAPTXLowLatencyProfile;
    uint8_t NumOfA2DPoXPANProfiles;
    uint8_t NumOfBtSlaves;

    uint8_t NumOfPwrSaveBtProfiles;
    uint8_t NumOfPwrSaveBtRFCOMMProfiles;
    uint8_t NumOfPwrSaveBtBNEPProfiles;
    uint8_t NumOfPwrSaveBtA2DPProfiles;

    uint32_t bt_prof_bitmap;
    uint32_t bt_scan_bitmap;

    uint32_t NumBtGpmOverflow;
    uint16_t PrevGPMIndex;

    uint8_t AclActiveScoTimerState;
    uint8_t le_rx_on_and_high_scan_ratio;
    uint8_t prev_bw_level;
    uint8_t isACLInteractive;

    uint8_t btPowerSaveThresh;
    uint8_t ProfileInfoLinkState;
    uint8_t BtLowPrioBudget; /* BT duty cycle in percentage */
    uint8_t BtHiPrioBudget;  /* BT duty cycle in percentage */
    uint8_t BtStateChangeReason;

    uint8_t BtTriggerRecieved;
    uint32_t last_btinfo_recv_timestamp;
    uint32_t current_ce_timestamp;
    uint32_t last_sleep_timestamp;
    uint32_t slc_wakeup_timestamp;
    uint32_t slc_last_sched_timestamp;
    uint32_t crash_timestamp;
    uint32_t pc_address;
    uint32_t hci_tx_timestamp;
    uint32_t hci_rx_timestamp;
    uint8_t hci_state;
    uint8_t reset_reason;
    uint32_t last_bt_delay_timestamp;
    uint8_t delay_type;
    uint8_t delay_observed;
    uint8_t ContNackCount;
    uint8_t PiconetClk;
    coex_bt_cal_status_gpm cal_status_gpm;
    coex_gpm_xpan_conn_update xpan_update_gpm;
    coex_csb_cfg *csb_cfg;
    uint32_t CoexMgrPolicy;
    uint8_t btBnepPrio;
    uint8_t btRfcommPrio;
    uint8_t btAclInactSlotThresh;
    uint32_t btBaActiveSlots;
    uint32_t TimeLastBtUpdate;

} BTCOEX_BT_INFO_STRUCT;

/*-------------------------------------------------------------------------
 * Function Declarations and Documentation
 * ----------------------------------------------------------------------*/
void coex_reset_topology_info(void);
void coex_set_bt_prio_limit(uint8_t LowPrioLimit, uint8_t HiPrioLimit);
uint8_t coex_alloc_local_link_id(uint8_t LinkID);
void coex_dealloc_local_link_id(uint8_t LinkID);
void coex_track_profile(uint8_t IsAddProfile, uint8_t LinkID, uint8_t ProfileType);
void coex_reset_bt_link_info(uint8_t LinkID);
uint8_t coex_bt_num_profiles(void);
void coex_mci_process_gpm(tMciGpm *pGPM);
void coex_reset_bt_link_info(uint8_t LinkID);
uint8_t coex_save_prof_info(tBtProfileGpm *pBtProfileInfo);
void coex_handle_critical_links(tBtProfileGpm *pCoexBtlink);
uint8_t coex_prof_info(uint8_t BtProfile, uint8_t BtProfileInfoReq, void *pParam);
uint8_t coex_nonlink_get_type(uint8_t opType);
void coex_process_long_le_scan(uint8_t ScanType, uint8_t IsStart);
void coex_process_btle_scan_activity(uint32_t old_bitmap, uint32_t new_bitmap);
uint8_t coex_acl_interact_update_linkinfo(coex_gpm_acl_interactive *acl_interactive);
void coex_acl_interact_reset(void);
uint8_t coex_is_nonlink_active(void);
uint8_t coex_acl_interact_is_active();
void coex_enable_bt_traffic_tracking(uint32_t BtIntervalDuration);
void coex_enable_bt_slave_inactivity_tracking(void);
uint8_t coex_is_le_scan_heavy(void);
uint8_t coex_get_le_scan_duty_cycle(void);
uint8_t coex_get_nonlink_id_of_type(uint8_t opType);
uint16_t coex_min_active_nonlink_op_interval(void);
uint8_t coex_max_active_nonlink_duty_cycle(void);
uint8_t coex_get_le_scan_duty_cycle(void);
void coex_update_critical_nonlink(uint8_t LinkID, uint8_t active);
void coex_update_nonlink_status(uint8_t id, uint8_t active, uint8_t type);
nt_status_t coex_bt_calculate_budget(void);
uint8_t coex_get_bt_role(uint8_t role);
#if defined(PLATFORM_FERMION)
void coex_bt_power_on(bool bt_on);
#endif
#endif /* SUPPORT_COEX */
#endif /*#ifndef _COEX_BT_H_ */
