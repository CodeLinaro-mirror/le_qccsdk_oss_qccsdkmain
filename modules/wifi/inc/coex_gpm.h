/*========================================================================
Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
*
* @file coex_gpm.h
* @brief GPM params and struct definitions
*========================================================================*/
#ifndef COEX_GPM_H
#define COEX_GPM_H
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
#include "coex_wlan.h"

/*-------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * ----------------------------------------------------------------------*/

#define COEX_CAP_BITMAP_BYTES (9)

/*
Bit0: Connected (1)/Disconnected(0)
Bit1: Scanning (1)/Not Scanning(0)
Bit2: Connecting (1)/Any other state(0)
Bit3~7: Reserved
*/
#define COEX_GPM_WLAN_STATS_CONNECTED               (0x1)
#define COEX_GPM_WLAN_STATS_SCANNING                (0x2)
#define COEX_GPM_WLAN_STATS_CONNECTING              (0x4)
#define COEX_GPM_SET_WLAN_STATS_CH0(stats, op, chm) ((stats) |= ((((chm)&0x1)) ? (op) : 0))
#define COEX_GPM_SET_WLAN_STATS_CH1(stats, op, chm) ((stats) |= ((((chm)&0x2)) ? (op) : 0))

#define MCI_GPM_SET_TYPE_OPCODE(_p_gpm, _type, _opcode)                        \
    do {                                                                       \
        *(((uint8_t *)(_p_gpm)) + MCI_GPM_COEX_B_GPM_TYPE) = (_type)&0xff;     \
        *(((uint8_t *)(_p_gpm)) + MCI_GPM_COEX_B_GPM_OPCODE) = (_opcode)&0xff; \
    } while (0)

#define MCI_Q_AFH                   (0x00000001)
#define MCI_Q_UNHALT                (0x00000002)
#define MCI_Q_BT_STAT_QUERY         (0x00000004)
#define MCI_Q_BT_FLAGS_UPDATE       (0x00000008)
#define MCI_Q_BT_VER_QUERY          (0x00000010)
#define MCI_Q_BT_VER_RESPONSE       (0x00000020)
#define MCI_Q_LNA_TAKE              (0x00000040)
#define MCI_Q_LNA_TRANS             (0x00000080)
#define MCI_Q_RXSS_THRESHOLD        (0x00000100)
#define MCI_Q_RXSS_THRESHOLD_QUERY  (0x00000200)
#define MCI_Q_SCHED_INFO_TRIGGER    (0x00000400)
#define MCI_Q_SCAN_OP               (0x00000800)
#define MCI_Q_PRIO_CONFIG_OP        (0x00001000)
#define MCI_Q_CONNECTION_STATUS     (0x00002000)
#define MCI_Q_CONCURRENT_TX_CHAIN0  (0x00004000)  // Present in HK.
#define MCI_Q_BT_AIRTIME_REQ        (0x00008000)
#define MCI_Q_WLAN_STATUS           (0x00010000)
#define MCI_Q_LIMIT_BT_POWER        (0x00020000)
#define MCI_Q_START_BT_RSSI_REPORT  (0x00040000)
#define MCI_Q_THREE_WAY_COEX_CONFIG (0x00080000)
#define MCI_Q_ASD_STATUS_INDICATE   (0x00100000)
#define MCI_Q_BT_PRIORITY_CONFIG    (0x00200000)
#define MCI_Q_SET_BT_GAIN           (0x00400000)

#define COEX_GPM_CONN_STAT_BTVER_7_0_TX_RATE_SHIFT (6)
#define COEX_GPM_CONN_STAT_BTVER_7_0_RX_RATE_SHIFT (4)
#define COEX_GPM_CONN_STAT_BTVER_9_0_TX_RATE_SHIFT (5)
#define COEX_GPM_CONN_STAT_BTVER_9_0_RX_RATE_SHIFT (2)
#define COEX_GPM_CONN_STAT_BTVER_7_0_ROLE_MASK     (0x0F)
#define COEX_GPM_CONN_STAT_BTVER_9_0_ROLE_MASK     (0x03)
#define COEX_GPM_CONN_STAT_BTVER_7_0_RATE_TX_MASK  (0xC0)
#define COEX_GPM_CONN_STAT_BTVER_9_0_RATE_TX_MASK  (0xE0)
#define COEX_GPM_CONN_STAT_BTVER_7_0_RATE_RX_MASK  (0x30)
#define COEX_GPM_CONN_STAT_BTVER_9_0_RATE_RX_MASK  (0x1C)

#define ISO_SUBEVENT_MASK_GPM_CONFIG_SHIFT     (0x1)
#define ISO_SUBEVENT_MASK_GPM_TRANS_TYPE_SHIFT (0x6)

#define MCI_GPM_SET_CAL_TYPE(_p_gpm, _cal_type)                                \
    do {                                                                       \
        *(((uint8_t *)(_p_gpm)) + MCI_GPM_COEX_B_GPM_TYPE) = (_cal_type)&0xff; \
    } while (0)

/*
Bit0: Connected (1)/Disconnected(0)
Bit1: Scanning (1)/Not Scanning(0)
Bit2: Connecting (1)/Any other state(0)
Bit3~7: Reserved
*/
#define COEX_GPM_WLAN_STATS_CONNECTED  (0x1)
#define COEX_GPM_WLAN_STATS_SCANNING   (0x2)
#define COEX_GPM_WLAN_STATS_CONNECTING (0x4)

#define COEX_GPM_SET_WLAN_STATS ((stats) |= op)
/*-------------------------------------------------------------------------
 * Type Declarations
 * ----------------------------------------------------------------------*/
/* MCI GPM/Coex opcode/type definitions */

typedef enum {
    COEX_BT_CRASH_INFO_REASON_UNKNOWN = 0x81,
    COEX_BT_CRASH_INFO_REASON_SW_REQUESTED = 0x82,
    COEX_BT_CRASH_INFO_REASON_STACK_OVERFLOW = 0x83,
    COEX_BT_CRASH_INFO_REASON_EXCEPTION = 0x84,
    COEX_BT_CRASH_INFO_REASON_ASSERT = 0x85,
    COEX_BT_CRASH_INFO_REASON_TRAP = 0x86,
    COEX_BT_CRASH_INFO_REASON_OS_FATAL = 0x87,
    COEX_BT_CRASH_INFO_REASON_HCI_RESET = 0x88,
    COEX_BT_CRASH_INFO_REASON_PATCH_RESET = 0x89,
    COEX_BT_CRASH_INFO_REASON_ABT = 0x8A,
    COEX_BT_CRASH_INFO_REASON_RAMMASK = 0x8B,
    COEX_BT_CRASH_INFO_REASON_WDOG_BARK = 0x8C,
    COEX_BT_CRASH_INFO_REASON_BUSERROR = 0x8D,
    COEX_BT_CRASH_INFO_REASON_IOP_FATAL = 0x8E,
    COEX_BT_CRASH_INFO_REASON_SSR_CMD = 0x8F,
    COEX_BT_CRASH_INFO_REASON_POWERON = 0x90,
    COEX_BT_CRASH_INFO_REASON_WDOG_BITE = 0x91,
    COEX_BT_CRASH_INFO_REASON_RAMMASK_RGN1 = 0x92,
    COEX_BT_CRASH_INFO_REASON_RAMMASK_RGN0 = 0x93,
    COEX_BT_CRASH_INFO_REASON_Q6_WATCHDOG = 0x94,
    COEX_BT_CRASH_INFO_REASON_INVALID_STACK = 0xF0
} COEX_BT_CRASH_INFO_REASON_CODE;

#ifdef PLATFORM_NT
enum {
    MCI_GPM_COEX_B_GPM_BODY = 6,
    MCI_GPM_COEX_W_GPM_PAYLOAD = 1,
    MCI_GPM_COEX_B_GPM_TYPE = 4,
    MCI_GPM_COEX_B_GPM_OPCODE = 5,
    /* MCI_GPM_WLAN_CAL_REQ, MCI_GPM_WLAN_CAL_DONE */
    MCI_GPM_WLAN_CAL_W_SEQUENCE = 2,
    /* MCI_GPM_COEX_VERSION_QUERY */
    /* MCI_GPM_COEX_VERSION_RESPONSE */
    MCI_GPM_COEX_B_MAJOR_VERSION = MCI_GPM_COEX_B_GPM_BODY,
    MCI_GPM_COEX_B_MINOR_VERSION = 7,
    MCI_GPM_COEX_B_CAP_CONFIG = 8,
    /* MCI_GPM_COEX_STATUS_QUERY */
    MCI_GPM_COEX_B_BT_BITMAP = MCI_GPM_COEX_B_GPM_BODY,
    MCI_GPM_COEX_B_WLAN_BITMAP = 7,
    /* MCI_GPM_COEX_HALT_BT_GPM */
    MCI_GPM_COEX_B_HALT_STATE = MCI_GPM_COEX_B_GPM_BODY,
    /* MCI_GPM_COEX_WLAN_CHANNELS */
    MCI_GPM_COEX_B_CHANNEL_MAP = MCI_GPM_COEX_B_GPM_BODY,
    /* MCI_GPM_COEX_BT_PROFILE_INFO */
    MCI_GPM_COEX_B_PROFILE_TYPE = MCI_GPM_COEX_B_GPM_BODY,
    MCI_GPM_COEX_B_PROFILE_LINKID = 7,
    MCI_GPM_COEX_B_PROFILE_STATE = 8,
    MCI_GPM_COEX_B_PROFILE_ROLE = 9,
    MCI_GPM_COEX_B_PROFILE_RATE = 10,
    MCI_GPM_COEX_B_PROFILE_VOTYPE = 11,
    MCI_GPM_COEX_H_PROFILE_T = 12,
    MCI_GPM_COEX_B_PROFILE_W = 14,
    MCI_GPM_COEX_B_PROFILE_A = 15,
    /* MCI_GPM_COEX_BT_STATUS_UPDATE */
    MCI_GPM_COEX_B_STATUS_TYPE = MCI_GPM_COEX_B_GPM_BODY,
    MCI_GPM_COEX_B_STATUS_LINKID = 7,
    MCI_GPM_COEX_B_STATUS_STATE = 8,
    /* MCI_GPM_COEX_BT_UPDATE_FLAGS */
    MCI_GPM_COEX_B_BT_FLAGS_OP = 10,
    MCI_GPM_COEX_W_BT_FLAGS = MCI_GPM_COEX_B_GPM_BODY,
    MCI_GPM_COEX_B_PROFILE_PAUSE_MASK = 8,
    MCI_GPM_COEX_B_PAUSE_SLOTS = MCI_GPM_COEX_B_GPM_BODY,
    MCI_GPM_COEX_B_INACTIVITY_THRESH = MCI_GPM_COEX_B_GPM_BODY,
    MCI_GPM_COEX_B_INACTIVITY_ENA = 8,
    MCI_GPM_COEX_B_RXSS_THRESHOLD = MCI_GPM_COEX_B_GPM_BODY,
    MCI_GPM_COEX_B_SCHED_INFO_TRIGGER = MCI_GPM_COEX_B_GPM_BODY,
    MCI_GPM_COEX_B_SCHED_INFO_TRIGGER_BOUND = 8,
    MCI_GPM_COEX_B_SCAN_OP = MCI_GPM_COEX_B_GPM_BODY,
    MCI_GPM_COEX_B_CONFIG_PRIO_OP = MCI_GPM_COEX_B_GPM_BODY,
    MCI_GPM_COEX_B_CONFIG_PRIO_PRIORITY = 7,
    MCI_GPM_COEX_B_CONFIG_PRIO_TYPE = 8,
    MCI_GPM_COEX_B_CONFIG_CONN_TYPE = MCI_GPM_COEX_B_GPM_BODY,
    MCI_GPM_COEX_B_CONFIG_CONN_LINKID = 7,
    MCI_GPM_COEX_B_CONFIG_CONN_OP = 8,
    MCI_GPM_COEX_B_CONFIG_CONN_ROLE = 9,
    MCI_GPM_COEX_B_CONFIG_CONN_INTRVL = 10,
    MCI_GPM_COEX_B_CONFIG_CONN_TO = 12,
    MCI_GPM_COEX_B_CONFIG_CONN_LATNCY = 14,
    MCI_GPM_COEX_W_BT_REG_ADDR = MCI_GPM_COEX_B_GPM_BODY,
    MCI_GPM_COEX_W_BT_REG_DATA = 10,
    MCI_GPM_COEX_B_BT_REG_OP = 14,
    MCI_GPM_COEX_W_BT_AAID_FREQ = MCI_GPM_COEX_B_GPM_BODY,
    MCI_GPM_COEX_B_BT_AAID_OFFSET = 8,
    MCI_GPM_COEX_B_BT_AAID_PWR = 9,
    MCI_GPM_COEX_W_BT_AAID_DURATION = 10,
    MCI_GPM_COEX_B_BT_STATUS_CH0_5G = MCI_GPM_COEX_B_GPM_BODY,
    MCI_GPM_COEX_B_BT_STATUS_CH0_2G = 7,
    MCI_GPM_COEX_B_BT_STATUS_CH1_5G = 8,
    MCI_GPM_COEX_B_BT_STATUS_CH1_2G = 9,
    MCI_GPM_COEX_B_BT_STATUS_OPERATING_MODE = 10,
    MCI_GPM_COEX_B_BT_STATUS_WLM_LEVEL = 11,
    MCI_GPM_COEX_B_BT_STATUS_COEX_LATENCY_MODE = 12,
    MCI_GPM_COEX_B_CONCURRENT_TX_CH0 = MCI_GPM_COEX_B_GPM_BODY,
    // BT Airtime GPM:
    MCI_GPM_COEX_B_BT_AIRTIME_STATS_REQUEST_NUM_OF_DELIVERY = MCI_GPM_COEX_B_GPM_BODY,
    MCI_GPM_COEX_B_BT_AIRTIME_STATS_REQUEST_TEST_INTERVAL = 7,

    MCI_GPM_COEX_B_LE_DATA_LEN_REQ_OP = MCI_GPM_COEX_B_GPM_BODY,
    MCI_GPM_COEX_B_LE_DATA_LEN_REQ_ID = 7,
    MCI_GPM_COEX_B_LE_DATA_LEN_REQ_MAX_TX = 8,
    MCI_GPM_COEX_B_LE_DATA_LEN_REQ_MAX_RX = 10,
    /* MCI_GPM_WLAN_STATUS_UPDATE */
    MCI_GPM_COEX_B_WLAN_STATUS = MCI_GPM_COEX_B_GPM_BODY,
    MCI_GPM_COEX_B_READ_BT_POWER_LINKID = MCI_GPM_COEX_B_GPM_BODY,
    MCI_GPM_COEX_B_LIMIT_BT_POWER_MAX_POWER = MCI_GPM_COEX_B_GPM_BODY,
    MCI_GPM_COEX_B_START_BT_RSSI_REPORT_PERIOD = MCI_GPM_COEX_B_GPM_BODY,
    MCI_GPM_COEX_B_THREE_WAY_COEX_CONFIG_REQ = MCI_GPM_COEX_B_GPM_BODY,
    MCI_GPM_COEX_B_SET_BT_GAIN = MCI_GPM_COEX_B_GPM_BODY,
    /*For coex versioning*/
    MCI_GPM_COEX_B_CAPABILITY_CONTROL = MCI_GPM_COEX_B_GPM_BODY,
    MCI_GPM_COEX_B_CAPABILITY_BITMAP = 7,
    MCI_GPM_COEX_B_ASD_STATUS_IND_GRANT = MCI_GPM_COEX_B_GPM_BODY,
    MCI_GPM_COEX_B_ASD_STATUS_IND_REASON = 7,
    /*For group priority*/
    MCI_GPM_COEX_B_WRITE_GROUP_BTPRIO_OP = MCI_GPM_COEX_B_GPM_BODY,
    /*For coex policy indication*/
    MCI_GPM_COEX_B_POLICY_MODE = MCI_GPM_COEX_B_GPM_BODY,
    MCI_GPM_COEX_B_GRANT_BT = 7,
    MCI_GPM_COEX_B_WLAN_DURATION = 8,
    MCI_GPM_COEX_B_BT_DURATION = 9,
    /*For ISO Priority Update GPM*/
    MCI_GPM_COEX_B_ISO_PRIO_UPDATE_GRPID = MCI_GPM_COEX_B_GPM_BODY,
    MCI_GPM_COEX_B_ISO_PRIO_UPDATE_CONFIG = 7,
    MCI_GPM_COEX_B_ISO_PRIO_UPDATE_MCI_PRIORITY = 2, /* As we use 32 bit offset for this */
    MCI_GPM_COEX_B_ISO_PRIO_UPDATE_PRIORITY_BMAP = 12,

    /*For ISO Subevent Mask GPM*/
    MCI_GPM_COEX_B_ISO_SUBEVENT_MASK_LINKID = MCI_GPM_COEX_B_GPM_BODY,
    MCI_GPM_COEX_B_ISO_SUBEVENT_MASK = 1, /* As we use 64 bit offset for this */
};

typedef enum mci_gpm_coex_opcode {
    MCI_GPM_VERSION_QUERY = 0,
    MCI_GPM_VERSION_RESPONSE = 1,
    MCI_GPM_STATUS_QUERY = 2,
    MCI_GPM_HALT_BT_GPM = 3,
    MCI_GPM_WLAN_CHANNELS = 4,
    MCI_GPM_BT_PROFILE_INFO = 5,
    MCI_GPM_BT_STATUS_UPDATE = 6,
    MCI_GPM_BT_UPDATE_FLAGS = 7,
    MCI_GPM_BT_SCAN_UPDATE = 8,
    MCI_GPM_WLAN_PRIO = 9,
    MCI_GPM_BT_PAUSE_PROFILE = 10,
    MCI_GPM_BT_SCAN_ACTIVITY = 11,
    MCI_GPM_BT_ACL_INACTIVITY_REPORT = 12,
    MCI_GPM_WLAN_SET_ACL_INACTIVITY = 13,
    MCI_GPM_WLAN_READ_BT_TX_POWER = 14,
    MCI_GPM_BT_REPORT_TX_POWER = 15,
    MCI_GPM_WLAN_SET_BT_RXSS_THRES = 16,
    MCI_GPM_BT_RXSS_THRES_QUERY = 17,
    MCI_GPM_BT_RXSS_LOWER_THRESHOLD_RESPONSE = 18,
    MCI_GPM_WLAN_SCHED_INFO_TRIGGER = 19,
    MCI_GPM_WLAN_SCHED_INFO_TRIGGER_RESPONSE = 20,
    MCI_GPM_SCAN_OP = 21,
    MCI_GPM_PAUSE_GPM_TX = 22,
    MCI_GPM_BT_CONNECT_STATUS = 23,
    MCI_GPM_BT_SCAN_ACTIVITY_BITMAP = 24,
    MCI_GPM_BT_SYSTEM_RESET = 25,
    MCI_GPM_BT_QUEUE_OVERFLOW = 26,
    MCI_GPM_BT_REG_OP = 27,
    MCI_GPM_BT_REPORT_REG = 28,
    MCI_GPM_BT_PAUSE_BT_CAL = 29,
    MCI_GPM_BT_REQUEST_WLAN_FLAG = 30,
    MCI_GPM_BT_ADDITIONAL_PROF_INFO = 31,
#if defined(SUPPORT_COEX_SIMULATOR)
    MCI_GPM_BT_BLE_ANCHOR_NOTIFY = 34,  // 0x22
#endif                                  // SUPPORT_COEX_SIMULATOR
    MCI_GPM_CONNECTION_STATUS = 38,     // 0x26
    MCI_GPM_BT_LE_DATA_LEN_REQ = 39,    // 0x27
    MCI_GPM_BT_LE_DATA_LEN_RES = 40,    // 0x28
    MCI_CONCURRENT_TX_CHAIN0_REQ = 41,  // 0x29
    MCI_CONCURRENT_TX_CHAIN0_RES = 42,  // 0x2A
    // BT Airtime GPM
    MCI_GPM_BT_AIRTIME_STATS_REQUEST = 43,    // 0x2B
    MCI_GPM_BT_AIRTIME_STATS_RESPONSE = 44,   // 0x2C
    MCI_GPM_BT_AIRTIME_STATS_STATUS = 45,     // 0x2D
    MCI_GPM_BT_ACL_INTERACTIVE = 46,          // 0x2E
    MCI_GPM_BT_ADV_PROFILE_INFO = 47,         // 0x2F
    MCI_GPM_BT_ADV_CONNECTION_STATUS = 48,    // 0x30
    MCI_GPM_BT_ADV_STATUS_UPDATE = 49,        // 0x31
    MCI_GPM_WLAN_STATUS_UPDATE = 50,          // 0x32
    MCI_GPM_BT_QOS_UPDATE = 51,               // 0x33
    MCI_GPM_WLAN_LIMIT_BT_MAX_TX_POWER = 55,  // 0x37
    MCI_GPM_WLAN_START_BT_RSSI_REPORT = 56,   // 0x38
    MCI_GPM_BT_REPORT_RSSI = 57,              // 0x39
    MCI_GPM_BT_DELAY_INDICATION = 58,         // 0x3A
    MCI_GPM_BT_MPTA_INFO = 59,                // 0x3B
    MCI_GPM_CAPABILITIES = 60,                // 0x3C
    MCI_GPM_WRITE_GROUP_BTPRIO = 61,          // 0x3D
    MCI_GPM_READ_GROUP_BTPRIO = 62,           // 0x3E
    MCI_GPM_THREE_WAY_COEX_CONFIG_REQ = 63,   // 0x3F
    MCI_GPM_THREE_WAY_COEX_CONFIG_RSP = 64,   // 0x40
    MCI_GPM_BT_ASD_STATUS_SYNC = 65,          // 0x41
    MCI_GPM_WLAN_ASD_STATUS_INDICATE = 66,    // 0x42
    MCI_GPM_LE_ADV_RX_INDICATION = 67,        // 0X43
    MCI_GPM_BT_ACL_RX_STATE_INDICATE = 68,    // 0X44
    MCI_GPM_BT_RX_GAIN_LVL_INDICATE = 69,     // 0X45
    MCI_GPM_ISO_PRIORITY_UPDATE = 70,         // 0X46
    MCI_GPM_BT_SLEEP_INFO = 71,               // 0x47
    MCI_GPM_BT_HCI_INFO = 72,                 // 0x48
    MCI_GPM_BT_SLC_INFO = 73,                 // 0x49
    MCI_GPM_BT_CRASH_INFO = 74,               // 0x4A
    MCI_GPM_QOS_LE_ISO_INFO = 75,             // 0X4B
    MCI_GPM_ISO_PRIORITY_MASK = 76,           // 0X4C
    MCI_GPM_BT_ISO_GRP_INFO = 77,             // 0X4D
    MCI_GPM_BT_ISO_LINK_INFO = 78,            // 0X4E
    MCI_GPM_WLAN_POLICY_INFO = 79,            // 0X4F
    MCI_GPM_RESERVED = 80,                    // 0X50
    MCI_GPM_BT_CAL_STATUS = 81,               // 0X51
    MCI_GPM_ISO_SUB_EVENT_MASK = 83,          // 0x53
#ifdef SUPPORT_XPAN_COEX
    MCI_GPM_XPAN_CONNECTION_UPDATE = 87,  // 0x57
#endif
    MCI_GPM_TWM_RELAY_CONN_UPDATE = 88,  // 0x58
    MCI_GPM_COEX_NUM_OPCODES
} MCI_GPM_COEX_OPCODE_T;

typedef enum mci_gpm_subtype {
    MCI_GPM_BT_CAL_REQ = 0,
    MCI_GPM_BT_CAL_GRANT = 1,
    MCI_GPM_BT_CAL_DONE = 2,
    MCI_GPM_WLAN_CAL_REQ = 3,
    MCI_GPM_WLAN_CAL_GRANT = 4,
    MCI_GPM_WLAN_CAL_DONE = 5,
    MCI_GPM_COEX = 0x0C,
    MCI_GPM_BT_5GTX_CHAIN1_REQ = 0x0E,
    MCI_GPM_BT_DEBUG = 0xFF
} MCI_GPM_SUBTYPE_T;

#endif /* #ifdef PLATFORM_NT */

/* This is the priority index from the BT, or the priority IDs. Coex uses this Ids to set the
 * coex priority via the "WLAN Read/Write BT priority" GPM. Please note that this will set only
 * the priority advertised to coex and the prioroty associated with BT internal scheuduler is
 * still un-altered.
 */
enum {
    BT_COEX_MCI_PRIO_ID_IDLE = 0,
    BT_COEX_MCI_PRIO_ID_POLL = 1,
    BT_COEX_MCI_PRIO_ID_INQUIRE_SCAN = 2,
    BT_COEX_MCI_PRIO_ID_INQUIRE_SCAN_RESP = 3,
    BT_COEX_MCI_PRIO_ID_PAGE_SCAN = 4,
    BT_COEX_MCI_PRIO_ID_LE_ADVERTISER = 5,
    BT_COEX_MCI_PRIO_ID_LE_SCANNER = 6,
    BT_COEX_MCI_PRIO_ID_LE_INITIATOR = 7,
    BT_COEX_MCI_PRIO_ID_LLR_TRIGGER_SCAN = 8,
    BT_COEX_MCI_PRIO_ID_CHANNEL_ASSESSMENT = 9,
    BT_COEX_MCI_PRIO_ID_SYNC_TRAIN_SCAN = 10,
    BT_COEX_MCI_PRIO_ID_ACL_1MPS = 11,
    BT_COEX_MCI_PRIO_ID_ACL_2MPS = 12,
    BT_COEX_MCI_PRIO_ID_ACL_3MPS = 13,
    BT_COEX_MCI_PRIO_ID_LLR_BEACON = 14,
    BT_COEX_MCI_PRIO_ID_TEST_MODE = 15,
    BT_COEX_MCI_PRIO_ID_RFCOMM = 17,
    BT_COEX_MCI_PRIO_ID_BNEP = 18,
    BT_COEX_MCI_PRIO_ID_LMP = 19,
    BT_COEX_MCI_PRIO_ID_INQUIRY = 20,
    BT_COEX_MCI_PRIO_ID_INQUIRY_RESP = 21,
    BT_COEX_MCI_PRIO_ID_SYNC_TRAIN = 22,
    BT_COEX_MCI_PRIO_ID_PAGE = 23,
    BT_COEX_MCI_PRIO_ID_ROLE_SWITCH = 24,
    BT_COEX_MCI_PRIO_ID_LLR_TRIGGER = 25,
    BT_COEX_MCI_PRIO_ID_LINKLESS_BC = 26,
    BT_COEX_MCI_PRIO_ID_SNIFF = 27,
    BT_COEX_MCI_PRIO_ID_LE_CONNECTION_DATA = 28,
    BT_COEX_MCI_PRIO_ID_LE_CONNECTION_CONTROL = 29,
    BT_COEX_MCI_PRIO_ID_QOS = 30,
    BT_COEX_MCI_PRIO_ID_A2DP = 31,
    BT_COEX_MCI_PRIO_ID_HID = 32,
    BT_COEX_MCI_PRIO_ID_VOICE_SCO = 34,
    BT_COEX_MCI_PRIO_ID_VOICE_EV3 = 35,
    BT_COEX_MCI_PRIO_ID_VOICE_2EV3 = 36,
    BT_COEX_MCI_PRIO_ID_VOICE_2EV5 = 37,
    BT_COEX_MCI_PRIO_ID_VOICE_3EV3 = 38,
    BT_COEX_MCI_PRIO_ID_VOICE_3EV5 = 39,
    BT_COEX_MCI_PRIO_ID_VOICE_RETRX = 40,
    BT_COEX_MCI_PRIO_ID_RESERVED_42 = 42,
    BT_COEX_MCI_PRIO_ID_RESERVED_43 = 43,
    BT_COEX_MCI_PRIO_ID_HID_ACTIVITY_REPORTED = 44,
    BT_COEX_MCI_PRIO_ID_LE_AUX_ADVERTISER = 45,
    BT_COEX_MCI_PRIO_ID_LE_AUX_SCANNER = 46,
    BT_COEX_MCI_PRIO_ID_LE_AUX_INITIATOR = 47,
    BT_COEX_MCI_PRIO_ID_BCAUD_SYNC_TRAIN_NORMAL = 48,
    BT_COEX_MCI_PRIO_ID_BCAUD_SYNC_TRAIN_CRITICAL = 49,
    BT_COEX_MCI_PRIO_ID_BCAUD = 50,
    BT_COEX_MCI_PRIO_ID_APTX_LL = 51,
    BT_COEX_MCI_PRIO_ID_ISO_DATA = 52,
    BT_COEX_MCI_PRIO_ID_ISO_RETRX = 53,
    BT_COEX_MCI_PRIO_ID_ISO_BIG_CONTROL = 54,
    BT_COEX_MCI_PRIO_ID_RESERVED_55 = 55,
    BT_COEX_MCI_PRIO_ID_AOA = 56,
    BT_COEX_MCI_PRIO_ID_ASHA_DATA = 57,
    BT_COEX_MCI_PRIO_ID_ASHA_CONTROL = 58,
    BT_COEX_MCI_PRIO_ID_RETRX_RVP_H1 = 59,
    BT_COEX_MCI_PRIO_ID_RETRX_RVP_H2 = 60,
    BT_COEX_MCI_PRIO_ID_RETRX_RVP_H3 = 61,
    BT_COEX_MCI_PRIO_ID_RETRX_RVP_L = 62,
    BT_COEX_MCI_PRIO_ID_A2DP_LL_LOW = 63,
    BT_COEX_MCI_PRIO_ID_A2DP_LL_HIGH = 64,
    BT_COEX_MCI_PRIO_ID_MAX_IDX
};

typedef struct _coex_utils_gpm_sub_hdr {
    uint8_t gpm_type;
    uint8_t gpm_subtype;
} coex_utils_gpm_sub_hdr;

typedef struct _coex_gpm_version_query {
    coex_utils_gpm_sub_hdr hdr;
    uint8_t major_version;
    uint8_t minor_version;
    uint8_t total_pages : 4, reserved : 3, cap_req : 1;
} coex_gpm_version_query;

typedef struct _coex_gpm_version_resp {
    coex_utils_gpm_sub_hdr hdr;
    uint8_t major;
    uint8_t minor;
    uint8_t total_pages : 4, reserved : 3, cap_req : 1;
} coex_gpm_version_resp;

typedef struct t_BtAdditionalProfInfo {
    coex_utils_gpm_sub_hdr hdr;
    uint8_t LinkID;
    uint8_t IsA2dpSink;
    uint32_t reserved1;
    uint32_t reserved2;
} tBtAdditionalProfInfo;

typedef struct t_WlanFlagReq {
    coex_utils_gpm_sub_hdr hdr;
    uint8_t reason;
} tWlanFlagReq;

typedef struct t_MCISchedInfoTriggerResp {
    coex_utils_gpm_sub_hdr hdr;
    uint8_t TriggerType;
    uint8_t LinkID;
    uint8_t Prio;
    uint8_t TriggerStartTime[4];
} tMCISchedInfoTriggerResp;

typedef struct t_BtConnectGpm {
    coex_utils_gpm_sub_hdr hdr;
    uint8_t LinkOp;
    uint8_t LinkID;
    uint8_t LinkRole;
    uint8_t WSize;
    uint16_t SupTo_WinOffset;
    uint16_t Interval;
    uint16_t ConnLatency;
} tBtConnectGpm;

typedef struct t_ScanUpdate {
    coex_utils_gpm_sub_hdr hdr;
    uint8_t IsPageScan;
    uint8_t ScanType;
    uint16_t Interval;
    uint16_t Window;
    uint8_t LinkID;
    uint8_t Enabled;
} tScanUpdate;

typedef struct t_BtProfileGpm {
    coex_utils_gpm_sub_hdr hdr;
    uint8_t ProfileType;
    uint8_t LinkID;
    uint8_t LinkState;
    uint8_t LinkRole;
    uint8_t LinkRate;
    uint8_t VoiceType;
    uint16_t TInterval;
    uint8_t WRetrx;
    uint8_t Attempts;
    uint8_t BTPerformanceState;
    uint8_t BTNonLinkOpType;
} tBtProfileGpm;

typedef struct t_MCILinkStatus {
    coex_utils_gpm_sub_hdr hdr;
    uint8_t LinkType;
    uint8_t LinkID;
    uint8_t PerformanceState;
    uint8_t OpType;
} tMCILinkStatus;

typedef struct t_MCIACLInactivity {
    coex_utils_gpm_sub_hdr hdr;
    uint8_t LinkID;
    uint8_t Reserved;
    int32_t ExpiredSlots;
} tMCIACLInactivity;

typedef struct t_MCIScanActivity {
    coex_utils_gpm_sub_hdr hdr;
    uint8_t ScanType;
    uint8_t IsStart;
    uint8_t LinkID;
} tMCIScanActivity;

typedef struct _coex_gpm_bt_delay_indication {
    coex_utils_gpm_sub_hdr hdr;
    uint8_t LinkID;
    uint8_t delay_ms;
    uint32_t clock;
    uint32_t rsvd;
} coex_gpm_bt_delay_indication;

typedef struct _coex_gpm_bt_qos_update {
    coex_utils_gpm_sub_hdr hdr;
    uint8_t LinkID;
    uint8_t BtLinkBwPct;
    uint8_t BtNonLinkBwPct;
    uint8_t BtQosMaxLatency;
    uint32_t reserved1;
    uint16_t reserved2;
} coex_gpm_bt_qos_update;

typedef struct _coex_gpm_bt_tx_power_update {
    coex_utils_gpm_sub_hdr hdr;
    uint8_t LinkID;
    int8_t TxPower;
    uint32_t reserved1;
    uint32_t reserved2;
} coex_gpm_bt_tx_power_update;

typedef struct _coex_gpm_bt_rssi_update {
    coex_utils_gpm_sub_hdr hdr;
    uint8_t LinkID;
    int8_t BtRssi;
    uint32_t reserved1;
    uint32_t reserved2;
} coex_gpm_bt_rssi_update;

typedef struct coex_gpm_le_rx_indication {
    coex_utils_gpm_sub_hdr hdr;
    uint16_t threshold;
} coex_gpm_le_rx_indication;

typedef struct _coex_gpm_capabilities {
    coex_utils_gpm_sub_hdr hdr;
    uint8_t curr_page : 4, reserved : 3, more : 1;
    uint8_t cap_bm[COEX_CAP_BITMAP_BYTES];
} coex_gpm_capabilities;

typedef struct _coex_gpm_wlan_perchain_stats {
    uint8_t chain0_5g_status;
    uint8_t chain0_2g_status;
    uint8_t chain1_5g_status;
    uint8_t chain1_2g_status;
    uint8_t operating_mode;
    uint8_t config;
    uint8_t coex_latency_mode_enabled;
} coex_gpm_wlan_perchain_stats;

typedef struct _coex_gpm_wlan_coex_policy_info_t {
    coex_utils_gpm_sub_hdr hdr; /* Generic GPM header. */
    uint8_t policy;             /* coex algo. */
    uint8_t grant_bt;           /* grant BT. */
    uint8_t wlan_dur;           /* WLAN duration. */
    uint8_t bt_dur;             /* BT duration. */

} coex_gpm_wlan_coex_policy_info;

typedef struct _coex_bt_status_query_gpm {
    coex_utils_gpm_sub_hdr hdr;
    uint8_t bt_bitmap;
    uint8_t wlan_bitmap;
    uint32_t piconet_clk;
    uint16_t seq_num;
    uint16_t rsvd;

} coex_bt_status_query_gpm;

typedef struct coex_gpm_bt_sleep_info {
    coex_utils_gpm_sub_hdr hdr;
    uint16_t reserved;
    uint32_t current_ce_timestamp;
    uint32_t last_sleep_timestamp;
} coex_gpm_bt_sleep_info;

typedef struct coex_gpm_bt_hci_info {
    coex_utils_gpm_sub_hdr hdr;
    uint8_t hci_state;
    uint8_t reserved;
    uint32_t hci_tx_timestamp;
    uint32_t hci_rx_timestamp;
} coex_gpm_bt_hci_info;

typedef struct coex_gpm_bt_slc_info {
    coex_utils_gpm_sub_hdr hdr;
    uint16_t reserved;
    uint32_t slc_wakeup_timestamp;
    uint32_t slc_last_sched_timestamp;
} coex_gpm_bt_slc_info;

typedef struct coex_gpm_bt_crash_info {
    coex_utils_gpm_sub_hdr hdr;
    uint8_t reset_reason;
    uint8_t reserved;
    uint32_t crash_timestamp;
    uint32_t pc_address;
} coex_gpm_bt_crash_info;

typedef struct coex_gpm_bt_delay_ind {
    coex_utils_gpm_sub_hdr hdr;
    uint8_t LinkID;
    uint8_t DelayObserved;
    uint8_t DelayType;
    uint8_t Reserved;
    uint16_t ContNackCount;
    uint32_t PiconetClk;
} coex_gpm_bt_delay_ind;

typedef struct coex_gpm_le_data_len_resp_parms_t {
    uint8_t cmd_status;  /* bit 0 => 0 = success, 1 = failure. bit 1-7 => reason codes for failure.*/
    uint8_t linkid;      /* Link Id of the BT link. */
    uint16_t max_tx_dur; /* The max tx duration value that BT shall start using after this gpm is sent.
                             valid only if cmd_status is success.*/
    uint16_t max_rx_dur; /* The max rx duration value that BT shall start using after this gpm is sent.
                             valid only if cmd_status is success.*/
    uint32_t reserved;
} coex_gpm_le_data_len_resp_params;

typedef struct coex_gpm_le_data_len_req_parms_t {
    uint8_t opcode;      /* 0 = read, 1 = write. */
    uint8_t linkid;      /* Link Id of the BT link. */
    uint16_t max_tx_dur; /* The max tx duration value that BT shall start using after this gpm is sent.
                             valid only if cmd_status is success.*/
    uint16_t max_rx_dur; /* The max rx duration value that BT shall start using after this gpm is sent.
                             valid only if cmd_status is success.*/
    uint32_t reserved;
} coex_gpm_le_data_len_req_params;

/* BT sends this message to indicate that there is an ACL interactive task created
 * to handle a profile level connection.
 */
typedef struct _coex_gpm_acl_interactive {
    coex_utils_gpm_sub_hdr hdr;
    uint8_t link_id;     /* The link id of the BT link. */
    uint8_t reason;      /* This should be typically tells about profile level connection or a Role switch procedure.*/
    uint32_t time_slice; /* This may not be required and is applicable only for fixed time operations like Role switch .
                          * Ideally Role switch is instant based and BT should not miss the instant frame.
                          * So we can use this TimeSlice filed, where the medium is given to BT till the time slice
                          * expires. Max value will be 20 msec.
                          */
    uint8_t start;       /* The start/stop of an interactive task for particular link Id. */
    uint32_t reserved;
} coex_gpm_acl_interactive;

typedef struct coex_gpm_iso_grp_info_t {
    coex_utils_gpm_sub_hdr hdr; /* Generic GPM header. */
    uint8_t groupid;            /* Identifier that uniquely identifies this group.*/
    uint8_t stream_type : 1,    /* 0 = sequential 1 = interleaved */
        transp_type : 2,        /* 0 = CIS, 1 = BIS */
        reserved : 5;
    uint16_t duration; /* in slot units */
    uint16_t interval; /* in slot units */
    uint8_t num_links;
    uint8_t usecase_id;
} coex_gpm_iso_grp_info;

typedef struct coex_gpm_iso_link_info_t {
    coex_utils_gpm_sub_hdr hdr; /* Generic GPM header. */
    uint8_t type : 4,           /* Type of multi-link profile (Eg: TWS) */
        profile : 4;            /* Supported profile, designated by E_BT_PROFILE */
    uint8_t linkid;
    uint8_t groupid; /* Identifier that uniquely identifies this group.*/
    uint8_t tx_flush_to : 4, rx_flush_to : 4;
    uint8_t role : 2, rx_data_rate : 3, tx_data_rate : 3;
    uint8_t nse;
    uint8_t linkop;
    uint8_t tx_burst_no : 4, rx_burst_no : 4;
    uint16_t sub_intrvl; /* in solt units */
} coex_gpm_iso_link_info;

typedef struct coex_gpm_qos_le_iso_info_t {
    coex_utils_gpm_sub_hdr hdr; /* Generic GPM header. */
    uint8_t groupid;            /* Identifier that uniquely identifies this group.*/
    uint8_t codec_info;
    uint8_t phy_rate;
    uint8_t total_bw; /*Currently not used */
    uint16_t M2S_SDU_size;
    uint16_t S2M_SDU_size;
    uint8_t tx_flush_to : 4, rx_flush_to : 4;
    uint8_t latency_ms;
} coex_gpm_qos_le_iso_info;

typedef struct coex_gpm_le_iso_priority_mask_t {
    coex_utils_gpm_sub_hdr hdr; /* Generic GPM header. */
    uint8_t linkid;             /* Identifier that uniquely identifies this link.*/
    uint8_t reason_code;
    uint16_t piconet_clk; /*last 2 bytes of clk */
    uint8_t stream_idx;
    uint8_t dbg_info; /*not used now */
    uint32_t priority_mask;

} coex_gpm_le_iso_priority_mask;

typedef struct _coex_bt_cal_status_gpm {
    coex_utils_gpm_sub_hdr hdr;
    uint8_t cal_result;
    uint8_t cal_status;
    uint32_t cal_start_clk;
    uint32_t cal_end_clk;
} coex_bt_cal_status_gpm;

typedef struct coex_gpm_xpan_conn_update {
    coex_utils_gpm_sub_hdr hdr;
    E_COEX_XPAN_CONN_STATE xpan_state;
    E_COEX_BT_LINK_FOCUS focus;
    uint16_t bitrate;
    uint8_t parent_linkid; /* Link ID of corresponding ACL link to handset */
    uint8_t usecase;
    uint8_t latency;
    uint8_t reserved;
    uint16_t reserved_1;
} coex_gpm_xpan_conn_update;

typedef struct coex_gpm_twm_conn_update {
    coex_utils_gpm_sub_hdr hdr;
    uint8_t link_id;
    uint8_t profile_type;
    uint8_t parent_linkid;
    uint8_t connection_state;
    E_COEX_BT_EARBUD_ROLE earbud_role;
    uint8_t reserved1;
    uint32_t reserved2;
} coex_gpm_twm_conn_update;

typedef struct _coex_gpm_cfg {
    uint32_t pending_msg_bm;
    uint32_t pending_msg_ack_bm;
    uint32_t PendingBtConfigFlags;
    uint8_t PendingBtConfigOpcode;
    uint8_t PendingStatQuery;
    int8_t PendingRXSSThreshold;
    uint8_t IsBtGpmHalted;
    uint8_t PeriodOfBtReportRSSI;
    int8_t BtMaxTxPower; /*WLAN limit Max BT tx power*/
    coex_gpm_wlan_perchain_stats *chain_stats;
    uint8_t BtPrioIdMax;
    uint8_t PendingBtPriority[BT_COEX_MCI_PRIO_ID_MAX_IDX];
} coex_gpm_cfg;

/*GPM Tx*/
void coex_send_version_query(void);
void coex_send_halt_bt(uint8_t halt);
void coex_send_wlan_capabilities(uint8_t page, uint8_t *cap_bitmap, uint8_t more);
void coex_send_version_response(uint8_t MajorVersion, uint8_t MinorVersion);
void coex_gpm_wlan_conn_status_report(coex_gpm_wlan_perchain_stats *wlan_stats);
void coex_send_afh(uint32_t *payload);
void coex_send_priority_config(uint8_t CoexOp, uint8_t CoexPriority, uint8_t IsRead);
void coex_send_bt_status_query(uint8_t query_type, E_STATUS_QUERY_SOURCE source);
void coex_send_acl_inactivity_trig(uint16_t InactivityThreshold, uint8_t LinkID);
void coex_send_iso_priority_update(uint8_t grpid, uint8_t iso_config, uint8_t trans_type, uint8_t priority_bmap,
                                   uint32_t mci_priority);
void coex_send_iso_subevent_mask(uint8_t linkid, uint64_t mask);

/* GPM Rx */
void coex_process_status_update(tMCILinkStatus *pBtLinkStatus);

/*GPM Utils*/
uint8_t coex_gpm_get_role_from_conn_stat(uint8_t role_field);
uint8_t coex_gpm_get_tx_rate_from_conn_stat(uint8_t role_field);
uint8_t coex_gpm_get_rx_rate_from_conn_stat(uint8_t role_field);
#endif /* defined(SUPPORT_COEX) */
#endif /* #ifndef _COEX_GPM_H_ */
