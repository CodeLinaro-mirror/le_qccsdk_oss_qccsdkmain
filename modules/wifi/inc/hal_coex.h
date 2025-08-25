/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#ifndef _HAL_COEX_H_
#define _HAL_COEX_H_

//#include "mac_hal_init.h"
#include "nt_common.h"

typedef unsigned char boolean;

//#include "ufw_wlan_ds.h"
/* HWIO Macro: R0 is common section, R1 is for interrupt, timer, status, debug */
#if defined(CONFIG_WHAL_MM)
#define HAL_COEX_GET_ADDR_LO(addr) (uint32_t)(((uint64_t)(addr)) & 0xffffffff)

#define HAL_COEX_GET_ADDR_HI(addr) (uint32_t)((((uint64_t)(addr)) >> 32) & 0xffffffff)
#else
#define WHAL_GET_ADDR_LO(addr) (uint32_t)(((uint64_t)(addr)) & 0xffffffff)

#define WHAL_GET_ADDR_HI(addr) (uint32_t)((((uint64_t)(addr)) >> 32) & 0xffffffff)
#endif
#define RAT_ID_LENGTH        4
#define RAT_ID_MASK          ((1 << RAT_ID_LENGTH) - 1)
#define RAT_ID_0             0
#define RAT_ID_1             1
#define NUM_WLAN_WEIGHT      11
#define NUM_MAX_WLAN_TX_PWR  11
#define NUM_SELFGEN_PKT_TYPE 4
/* Clock Gate */
#define COEX_CLKGATE_ENABLE  0x0 /* set 0 to enable module clock gate */
#define COEX_CLKGATE_DISABLE 0x1 /* set 1 to disable module clock gate */

#define CHKSUM_ENABLE  0x1
#define CHKSUM_DISABLE 0x0

#define TIMESTAMP_DISABLE 0x0 /* absolute time mode */
#define TIMESTAMP_ENABLE  0x1 /* delta time mode */

#define MCI_GPM_HDR  0x0
#define MCI_SCHD_HDR 0x1

#define MCI_REMOTE_RESET_ENABLE 0x1
#define MCI_CPU_INT_ENABLE      0x1
#define MCI_EXTRA_MSG_ENABLE    0x1
#define MCI_EXTRA_MSG_VALID     0x1

/* Timer Fix-value setting */
#define COEX_SMH_GPM_CHK_INTERVAL 30 /* ms */

/* SMH Fix-value setting */
#define COEX_SMH_GPM_RING_SIZE     0x64
#define COEX_SMH_GPM_RING_SIZE_MIN 0x2 /* Minimum GPM Ring size is 2 */

/* PMH Fix-value setting */
#define COEX_PMH_BT_RESP_TOUT 0xff

/* BMH Fix-value setting */
#define COEX_BMH_RX_ANT_MASK                       0x3
#define COEX_BMH_WLAN_TX_PWR_THRESH                0x14
#define COEX_BMH_WLAN_MAX_TX_PWR_THRESH_BT_LOW_PRI 0xf
#define COEX_BMH_WLAN_MAX_TX_PWR_THRESH_BT_HI_PRI  0xf
#define COEX_BMH_CONC_TX_PWR_THRESH                0xb
#define COEX_BMH_BT_GUARD_PERIOD                   0x8
#define COEX_BMH_LTE_GUARD_PERIOD                  0x8
#define COEX_BMH_WLAN_FREQ_RANGE_20M_LOWER         0x99c
#define COEX_BMH_WLAN_FREQ_RANGE_20M_UPPER         0x9b0
#define COEX_BMH_WLAN_FREQ_RANGE_40M_LOWER         0x988
#define COEX_BMH_WLAN_FREQ_RANGE_40M_UPPER         0x994
#define COEX_BMH_WLAN_FREQ_RANGE_80M_LOWER         0x984
#define COEX_BMH_WLAN_FREQ_RANGE_80M_UPPER         0x996
#define COEX_BMH_WLAN_USAGE_TRACKING_MASK                                                                   \
    (HWIO_CXC_BMH_REG_CXC_BMH_REG_R_CXC_BMH_R1_BT_WLAN_USAGE_TRACKING_CTRL_COUNT_BT_TX_TIME_BMSK |          \
     HWIO_CXC_BMH_REG_CXC_BMH_REG_R_CXC_BMH_R1_BT_WLAN_USAGE_TRACKING_CTRL_COUNT_BT_RX_TIME_BMSK |          \
     HWIO_CXC_BMH_REG_CXC_BMH_REG_R_CXC_BMH_R1_BT_WLAN_USAGE_TRACKING_CTRL_COUNT_WLAN1_NAV_TIME_BMSK |      \
     HWIO_CXC_BMH_REG_CXC_BMH_REG_R_CXC_BMH_R1_BT_WLAN_USAGE_TRACKING_CTRL_COUNT_WLAN2_NAV_TIME_BMSK |      \
     HWIO_CXC_BMH_REG_CXC_BMH_REG_R_CXC_BMH_R1_BT_WLAN_USAGE_TRACKING_CTRL_COUNT_WLAN1_CCA_BUSY_TIME_BMSK | \
     HWIO_CXC_BMH_REG_CXC_BMH_REG_R_CXC_BMH_R1_BT_WLAN_USAGE_TRACKING_CTRL_COUNT_WLAN2_CCA_BUSY_TIME_BMSK | \
     HWIO_CXC_BMH_REG_CXC_BMH_REG_R_CXC_BMH_R1_BT_WLAN_USAGE_TRACKING_CTRL_CLEAR_BMSK)
/*
#define CXC_R0_ISR_P_BMSK HWIO_WCMN_CORE_R0_ISR_P_CXC_BMSK
#define CXC_R0_ISR_S1_BMSK (HWIO_WCMN_CORE_R0_ISR_S1_BMH_BMSK | \
                   HWIO_WCMN_CORE_R0_ISR_S1_LMH_BMSK | \
                   HWIO_WCMN_CORE_R0_ISR_S1_PMH_BMSK | \
                   HWIO_WCMN_CORE_R0_ISR_S1_SMH_BMSK | \
                   HWIO_WCMN_CORE_R0_ISR_S1_MCIM_BMSK | \
//                   HWIO_WCMN_CORE_R0_ISR_S1_LCMH_STROBE_BMSK | \
//                   HWIO_WCMN_CORE_R0_ISR_S1_LCMH_WCI2_BMSK)
*/
/* Extend MCI queue */
#define MCI_NUM_OF_GPM_EXT   (32)
#define MCI_GPM_BUF_SIZE_EXT (MCI_NUM_OF_GPM_EXT * 16)

#define MAX_LOCAL_BT_LINK       (10)
#define MAX_REMOTE_BT_LINK      (72)
#define MAX_BT_NONLINK_CRT_LINK (4)
#define INVALID_BT_LINKID       (0xFF)

/*LCMH message definition*/
#define TYPE_0         0x0
#define TYPE_1         0x1
#define TYPE_2         0x2
#define TYPE_3         0x3
#define TYPE_4         0x4
#define TYPE_5         0x5
#define TYPE_6         0x6
#define TYPE_7         0x7
#define TYPE_0_RAT_ID0 0x8
#define TYPE_0_RAT_ID1 0x9
#define TYPE_6_RAT_ID0 0xa
#define TYPE_6_RAT_ID1 0xb

#define FIFO_POP_ON_READ 0x0
#define FIFO_POP_ONCE    0x1
#define FIFO_POP_CLEAR   0x2

/* GPM Message Operation */
#define MCI_GPM_SET_TYPE_OPCODE(_p_gpm, _type, _opcode)                        \
    do {                                                                       \
        *(((uint8_t *)(_p_gpm)) + MCI_GPM_COEX_B_GPM_TYPE) = (_type)&0xff;     \
        *(((uint8_t *)(_p_gpm)) + MCI_GPM_COEX_B_GPM_OPCODE) = (_opcode)&0xff; \
    } while (0)

#define MCI_GPM_SET_CAL_TYPE(_p_gpm, _cal_type)                                \
    do {                                                                       \
        *(((uint8_t *)(_p_gpm)) + MCI_GPM_COEX_B_GPM_TYPE) = (_cal_type)&0xff; \
    } while (0)

#define PTA_2WIRE_MODE 0
#define PTA_3WIRE_MODE 1
#define PTA_FULL_MODE  2

/* Common macros for register offsets*/
//#define CXC_R0_ISR_S1_BMH_BMSK                     HWIO_WCMN_CORE_R0_ISR_S1_BMH_BMSK
//#define CXC_R0_ISR_S1_LMH_BMSK             HWIO_WCMN_CORE_R0_ISR_S1_LMH_BMSK
//#define CXC_R0_ISR_S1_PMH_BMSK             HWIO_WCMN_CORE_R0_ISR_S1_PMH_BMSK
//#define CXC_R0_ISR_S1_SMH_BMSK             HWIO_WCMN_CORE_R0_ISR_S1_SMH_BMSK
//#define CXC_R0_ISR_S1_MCIM_BMSK            HWIO_WCMN_CORE_R0_ISR_S1_MCIM_BMSK
//#define CXC_R0_ISR_S1_LCMH_STROBE_BMSK         HWIO_WCMN_CORE_R0_ISR_S1_LCMH_STROBE_BMSK
//#define CXC_R0_ISR_S1_LCMH_WCI2_BMSK           HWIO_WCMN_CORE_R0_ISR_S1_LCMH_WCI2_BMSK

typedef enum {
    COEX_WEIGHT_BASE = 0,
    COEX_WEIGHT_LOW = 1,
    COEX_WEIGHT_MID = 2,
    COEX_WEIGHT_MID_NONSYNC = 3,
    COEX_WEIGHT_HI_NONVOICE = 4,
    COEX_WEIGHT_HI = 5,
    COEX_WEIGHT_CRITICAL_NONSYNC = 6,
    COEX_WEIGHT_CRITICAL = 7,
#ifdef SUPPORT_PTA_COEX
    COEX_WEIGHT_PTA_MIDDLE = 8,
#endif
} COEX_WEIGHT_LEVEL;

typedef enum { MCI_CCA_DEFAULT, MCI_CCA_WAN_TX, MCI_CCA_BT_TX, MCI_CCA_BT_WAN_TX } MCI_CCA_TYPE;

typedef enum {
    COEX_WHAL_WLAN_BW_20M = 0,
    COEX_WHAL_WLAN_BW_40M,
    COEX_WHAL_WLAN_BW_80M,
    COEX_WHAL_WLAN_BW_MAX = COEX_WHAL_WLAN_BW_80M,
} WHAL_BANDWITH_MHZ;

typedef enum {
    COEX_BMH_CTS_OFFSET = 0,
    COEX_BMH_BA_OFFSET,
    COEX_BMH_ACK_OFFSET,
    COEX_BMH_OTHERS_OFFSET,
} WHAL_COEX_SELFGEN_OFFSET_WEIGHT;

enum {
    MCI_QUERY_ALL_INFO = 0x1,
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
    MCI_GPM_XPAN_CONNECTION_UPDATE = 87,      // 0x57
    MCI_GPM_TWM_RELAY_CONN_UPDATE = 88,       // 0x58
    MCI_GPM_COEX_NUM_OPCODES
} MCI_GPM_COEX_OPCODE_T;

typedef enum mci_message_header {
    MCI_LNA_BT_LOCK = 0x10, /* len = 0 */
    MCI_CONT_NACK = 0x20,   /* len = 0 */
    MCI_CONT_INFO = 0x30,   /* len = 4 */
    MCI_CONT_RST = 0x40,    /* len = 0 */
    MCI_SCHD_INFO = 0x50,   /* len = 16 */
    MCI_CPU_INT = 0x60,     /* len = 4 */
    MCI_SYS_WAKING = 0x70,  /* len = 0 */
    MCI_GPM = 0x80,         /* len = 16 */
    MCI_LNA_INFO = 0x90,    /* len = 1 */
    MCI_LNA_STATE = 0x94,
    MCI_LNA_TAKE = 0x98,
    MCI_LNA_TRANS = 0x9c,
    MCI_SYS_SLEEPING = 0xa0, /* len = 0 */
    MCI_REQ_WAKE = 0xc0,     /* len = 0 */
    MCI_DEBUG_16 = 0xfe,     /* len = 2 */
    MCI_REMOTE_RESET = 0xff  /* len = 16 */
} MCI_MESSAGE_HEADER;

/* MCI GPM/Coex opcode/type definitions */
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

enum {
    MCI_REMOTE_SLEEPING = 0,
    MCI_REMOTE_AWAKE = 1,
};

typedef struct mci_gpm_msg {
    uint32_t WBTimer;
    uint32_t GPMBody[3];
} MCI_GPM_MSG, *pMCI_GPM_MSG;

typedef struct gpm_bt_profile_info {
    uint8_t ProfileType;
    uint8_t LinkID;
    uint8_t State;
    uint8_t Role;
    uint8_t Rate;
    uint8_t VoiceType;
    uint16_t TInterval;
    uint8_t WRetrx;
    uint8_t Attempts;
    uint8_t BTPerformanceState;
    uint8_t BTNonLinkOpType;
} GPM_BT_PROFILE_INFO, *pGPM_BT_PROFILE_INFO;

typedef enum mci_gpm_subtype {
    MCI_GPM_BT_CAL_REQ = 0,
    MCI_GPM_BT_CAL_GRANT = 1,
    MCI_GPM_BT_CAL_DONE = 2,
    MCI_GPM_WLAN_CAL_REQ = 3,
    MCI_GPM_WLAN_CAL_GRANT = 4,
    MCI_GPM_WLAN_CAL_DONE = 5,
    MCI_GPM_COEX = 0x0C,
    MCI_GPM_ANT = 0x0D,
    MCI_GPM_INVALID8 = 0xFE,
    MCI_GPM_INVALID = (int)0xFEFEFEFE,
    MCI_GPM_BT_DEBUG = 0xFF
} MCI_GPM_SUBTYPE_T;

typedef struct intr_stat {
    uint32_t intr_count;    // no of times interrupt occur for that event.
    uint32_t lastevent_ts;  // time stamp of the last event occur.
} INTR_STAT;

///////////////// LCMH ///////////////
#define LCMH_MSG_Q_MAX_SIZE (4 * 1024)  // 4k size

typedef struct coex_lte_whalinfo {
    uint32_t config;
    uint32_t status;
    uint32_t enmaskintr;  // 32 interrupt enable mask
    uint16_t errcnt;
    uint16_t errthres;
    uint8_t tx_aept_thr;
    uint8_t rx_aful_thr;
    uint16_t tpc_limit;
} COEX_LTE_WHALINFO;

typedef struct lcmh_wci2_intr_info {
    INTR_STAT LCMH_WCI2_INTR_STATUS[32];
} LCMH_WCI2_INTR_INFO;

typedef struct lcmh_strobe_intr_info {
    INTR_STAT strobe_stat[2];
} LCMH_STROBE_INTR_INFO;

typedef struct lcmh_type7_intr_info {
    INTR_STAT type7_stat[6];
} LCMH_TYPE7_INTR_INFO;

typedef struct {
    LCMH_WCI2_INTR_INFO wci2info;
    LCMH_STROBE_INTR_INFO strobeinfo;
    LCMH_TYPE7_INTR_INFO type7info;
} LCMH_INTR_STAT;

typedef struct {
    uint32_t data[LCMH_MSG_Q_MAX_SIZE];
    uint32_t wbtimer[LCMH_MSG_Q_MAX_SIZE];
    uint8_t rd_pos;
    uint8_t wr_pos;
} LCMH_MSG_Q;

///////////////// LMH ///////////////
typedef struct LMH_WHAL_INTR_INFO {
    INTR_STAT lmh_intr_stat[10];
} LMH_WHAL_INTR_INFO;

///////////////// MCI ///////////////
typedef struct mci_intr_info {
    INTR_STAT mci_intr_stat[20];
} MCI_INTR_INFO;

typedef struct mci_mode_ctrl {
    boolean chksum_enable;
    uint8_t max_retries;
} MCI_MODE_CTRL;

typedef struct mci_wsim_mode {
    uint8_t edge_config;
    uint8_t bus_sync_timeout;
} MCI_WSIM_MODE;

typedef struct mci_time_interval {
    /* CXC_MCIBASIC_R0_TIME_INTERVAL* register */
    uint32_t schd_info_1st_schd_info_2nd_seq0;
    uint32_t schd_info_2nd_cont_info_seq0;
    uint32_t cont_info_or_lna_lock_to_cont_rst_seq0;
    uint32_t schd_info_lna_lock_seq0;
    uint32_t cont_nack_cont_rst_seq0;
    uint32_t nxt_seq_seq0;
    uint32_t schd_info_1st_schd_info_2nd_seq1;
    uint32_t schd_info_2nd_cont_info_seq1;
    uint32_t cont_info_or_lna_lock_to_cont_rst_seq1;
    uint32_t schd_info_lna_lock_seq1;
    uint32_t cont_nack_cont_rst_seq1;
    uint32_t nxt_seq_seq1;
} MCI_TIME_INTERVAL, *pMCI_TIME_INTERVAL;

typedef struct mci_btsim_msg_cntrl {
    /* CXC_MCIBASIC_R0_BT_SIMULATOR_MSG_CNTRL_SEQ* register */
    uint32_t seq0_modified_cont_info_en;
    uint32_t seq0_schd_info2_en;
    uint32_t seq0_schd_info1_en;
    uint32_t seq0_tx;
    uint32_t seq0_repeat_cnt;
    uint32_t seq0_seq_gen_en;
    uint32_t seq1_modified_cont_info_en;
    uint32_t seq1_schd_info2_en;
    uint32_t seq1_schd_info1_en;
    uint32_t seq1_tx;
    uint32_t seq1_repeat_cnt;
    uint32_t seq1_seq_gen_en;
} MCI_BTSIM_MSG_CNTRL, *pMCI_BTSIM_MSG_CNTRL;

typedef struct mci_btsim_config {
    uint32_t cont_info_seq0;
    uint32_t cont_info_seq1;
    uint32_t modified_cont_info_seq0;
    uint32_t modified_cont_info_seq1;
    uint32_t cont_info_v2_seq0;
    uint32_t cont_info_v2_seq1;
    uint32_t schd_info_1_seq0_reg1;
    uint32_t schd_info_1_seq0_reg2;
    uint32_t schd_info_1_seq0_reg3;
    uint32_t schd_info_1_seq0_reg4;
    uint32_t schd_info_2_seq0_reg1;
    uint32_t schd_info_2_seq0_reg2;
    uint32_t schd_info_2_seq0_reg3;
    uint32_t schd_info_2_seq0_reg4;
    uint32_t schd_info_1_seq1_reg1;
    uint32_t schd_info_1_seq1_reg2;
    uint32_t schd_info_1_seq1_reg3;
    uint32_t schd_info_1_seq1_reg4;
    uint32_t schd_info_2_seq1_reg1;
    uint32_t schd_info_2_seq1_reg2;
    uint32_t schd_info_2_seq1_reg3;
    uint32_t schd_info_2_seq1_reg4;
    uint32_t wbt_schd_info_0;
    uint32_t wbt_schd_info_1;
} MCI_BTSIM_CONFIG, *pMCI_BTSIM_CONFIG;

typedef struct bmh_dbg_cnt_arb1 {
    uint32_t mci_wlan_cont_rst_rx_cnt;
    uint32_t mci_wlan_cont_rst_tx_cnt;
    uint32_t mci_wlan_cont_info_rx_cnt;
    uint32_t mci_wlan_cont_info_tx_cnt;
    uint32_t bt_req_nack_cnt;
    uint32_t bt_tx_req_cnt;
    uint32_t bt_rx_req_cnt;
    uint32_t concurrent_bt_rx_wl_rx_cnt;
    uint32_t concurrent_bt_tx_wl_rx_cnt;
    uint32_t concurrent_bt_rx_wl_tx_cnt;
    uint32_t concurrent_bt_tx_wl_tx_cnt;
    uint32_t wl_tx_req_nack_schd_bt_reason_cnt;
    uint32_t wl_tx_req_nack_current_bt_reason_cnt;
    uint32_t wl_tx_req_nack_other_wlan_tx_reason_cnt;
    uint32_t wl_tx_req_nack_lcmh_reason_cnt;
    uint32_t coex_tx_resp_concurrent_wlan_tx_cnt;
    uint32_t coex_tx_resp_alt_based_cnt;
    uint32_t coex_tx_resp_default_based_cnt;
    uint32_t tx_status_resp_tx_end_cnt;
    uint32_t tx_status_resp_tx_start_cnt;
    uint32_t tx_status_fes_end_cnt;
    uint32_t tx_status_fes_tx_end_cnt;
    uint32_t tx_status_fes_tx_start_cnt;
    uint32_t tx_flush_cnt;
    uint32_t tbtt_cnt;
    uint32_t wl_in_tx_abort_cnt;
    uint32_t wl_tx_auto_resp_req_cnt;
    uint32_t wl_tx_req_ack_cnt;
    uint32_t wl_tx_req_cnt;
    uint32_t crx_dbg_signal_sts;                    // CRX_FEATURE_DEBUG_SIGNAL_STATUS
    uint32_t dbg_stat_linkid_value_ix0;             // CRX_FEATURE_DEBUG_STAT_LINKID_VALUE_IX0
    uint32_t dbg_stat_linkid_value_ix1;             // CRX_FEATURE_DEBUG_STAT_LINKID_VALUE_IX1
    uint32_t dbg_stat_rssi_value_capture_ix0;       // CRX_FEATURE_DEBUG_STAT_RSSI_VALUE_CAPTURE_IX0
    uint32_t dbg_stat_rssi_value_capture_ix1;       // CRX_FEATURE_DEBUG_STAT_RSSI_VALUE_CAPTURE_IX1
    uint32_t dbg_stat_rssi_value_capture_ix2;       // CRX_FEATURE_DEBUG_STAT_RSSI_VALUE_CAPTURE_IX2
    uint32_t dbg_stat_wbtimer_capture_ix0;          // CRX_FEATURE_DEBUG_STAT_WBTIMER_CAPTURE_IX0
    uint32_t dbg_stat_wbtimer_capture_ix1;          // CRX_FEATURE_DEBUG_STAT_WBTIMER_CAPTURE_IX1
    uint32_t dbg_stat_wbtimer_capture_ix2;          // CRX_FEATURE_DEBUG_STAT_WBTIMER_CAPTURE_IX2
    uint32_t dbg_stat_wbtimer_capture_ix3;          // CRX_FEATURE_DEBUG_STAT_WBTIMER_CAPTURE_IX3
    uint32_t dbg_stat_wbtimer_capture_ix4;          // CRX_FEATURE_DEBUG_STAT_WBTIMER_CAPTURE_IX4
    uint32_t dbg_stat_wbtimer_capture_ix5;          // CRX_FEATURE_DEBUG_STAT_WBTIMER_CAPTURE_IX5
    uint32_t dbg_stat_wbtimer_capture_ix6;          // CRX_FEATURE_DEBUG_STAT_WBTIMER_CAPTURE_IX6
    uint32_t dbg_stat_wbtimer_capture_ix7;          // CRX_FEATURE_DEBUG_STAT_WBTIMER_CAPTURE_IX7
    uint32_t dbg_stat_wbtimer_capture_ix8;          // CRX_FEATURE_DEBUG_STAT_WBTIMER_CAPTURE_IX8
    uint32_t con_bt_req_in_rssi_thres_cnt_ix0;      // CXC_BMH_R1_CONSECUTIVE_BT_REQ_WITHIN_RSSI_THRESH_CNT_IX0
    uint32_t con_bt_req_in_rssi_thres_cnt_ix1;      // CXC_BMH_R1_CONSECUTIVE_BT_REQ_WITHIN_RSSI_THRESH_CNT_IX1
    uint32_t con_bt_req_in_rssi_thres_cnt_ix2;      // CXC_BMH_R1_CONSECUTIVE_BT_REQ_WITHIN_RSSI_THRESH_CNT_IX2
    uint32_t con_bt_req_in_rssi_thres_cnt_ix3;      // CXC_BMH_R1_CONSECUTIVE_BT_REQ_WITHIN_RSSI_THRESH_CNT_IX3
    uint32_t con_bt_req_in_rssi_thres_cnt_ix4;      // CXC_BMH_R1_CONSECUTIVE_BT_REQ_WITHIN_RSSI_THRESH_CNT_IX4
    uint32_t con_bt_req_above_rssi_thres_cnt_ix0;   // CXC_BMH_R1_CONSECUTIVE_BT_REQ_ABOVE_RSSI_UPPER_THRESH_CNT_IX0
    uint32_t con_bt_req_above_rssi_thres_cnt_ix1;   // CXC_BMH_R1_CONSECUTIVE_BT_REQ_ABOVE_RSSI_UPPER_THRESH_CNT_IX1
    uint32_t con_bt_req_above_rssi_thres_cnt_ix2;   // CXC_BMH_R1_CONSECUTIVE_BT_REQ_ABOVE_RSSI_UPPER_THRESH_CNT_IX2
    uint32_t con_bt_req_above_rssi_thres_cnt_ix3;   // CXC_BMH_R1_CONSECUTIVE_BT_REQ_ABOVE_RSSI_UPPER_THRESH_CNT_IX3
    uint32_t con_bt_req_above_rssi_thres_cnt_ix4;   // CXC_BMH_R1_CONSECUTIVE_BT_REQ_ABOVE_RSSI_UPPER_THRESH_CNT_IX4
    uint32_t con_bt_req_below_rssi_thres_cnt_ix0;   // CXC_BMH_R1_CONSECUTIVE_BT_REQ_BELOW_RSSI_LOWER_THRESH_CNT_IX0
    uint32_t con_bt_req_below_rssi_thres_cnt_ix1;   // CXC_BMH_R1_CONSECUTIVE_BT_REQ_BELOW_RSSI_LOWER_THRESH_CNT_IX1
    uint32_t con_bt_req_below_rssi_thres_cnt_ix2;   // CXC_BMH_R1_CONSECUTIVE_BT_REQ_BELOW_RSSI_LOWER_THRESH_CNT_IX2
    uint32_t con_bt_req_below_rssi_thres_cnt_ix3;   // CXC_BMH_R1_CONSECUTIVE_BT_REQ_BELOW_RSSI_LOWER_THRESH_CNT_IX3
    uint32_t con_bt_req_below_rssi_thres_cnt_ix4;   // CXC_BMH_R1_CONSECUTIVE_BT_REQ_BELOW_RSSI_LOWER_THRESH_CNT_IX4
    uint32_t con_bt_req_crx_allow_set_cnt_ix0;      // CXC_BMH_R1_CONSECUTIVE_BT_REQ_CRX_ALLOW_SET_CNT_IX0
    uint32_t con_bt_req_crx_allow_set_cnt_ix1;      // CXC_BMH_R1_CONSECUTIVE_BT_REQ_CRX_ALLOW_SET_CNT_IX1
    uint32_t con_bt_req_crx_allow_set_cnt_ix2;      // CXC_BMH_R1_CONSECUTIVE_BT_REQ_CRX_ALLOW_SET_CNT_IX2
    uint32_t con_bt_req_crx_allow_set_cnt_ix3;      // CXC_BMH_R1_CONSECUTIVE_BT_REQ_CRX_ALLOW_SET_CNT_IX3
    uint32_t con_bt_req_crx_allow_set_cnt_ix4;      // CXC_BMH_R1_CONSECUTIVE_BT_REQ_CRX_ALLOW_SET_CNT_IX4
    uint32_t con_bt_req_crx_allow_not_set_cnt_ix0;  // CXC_BMH_R1_CONSECUTIVE_BT_REQ_CRX_ALLOW_NOT_SET_CNT_IX0
    uint32_t con_bt_req_crx_allow_not_set_cnt_ix1;  // CXC_BMH_R1_CONSECUTIVE_BT_REQ_CRX_ALLOW_NOT_SET_CNT_IX1
    uint32_t con_bt_req_crx_allow_not_set_cnt_ix2;  // CXC_BMH_R1_CONSECUTIVE_BT_REQ_CRX_ALLOW_NOT_SET_CNT_IX2
    uint32_t con_bt_req_crx_allow_not_set_cnt_ix3;  // CXC_BMH_R1_CONSECUTIVE_BT_REQ_CRX_ALLOW_NOT_SET_CNT_IX3
    uint32_t con_bt_req_crx_allow_not_set_cnt_ix4;  // CXC_BMH_R1_CONSECUTIVE_BT_REQ_CRX_ALLOW_NOT_SET_CNT_IX4
    uint32_t mci_crx_en_sts;                        // CXC_BMH_R1_MCI_CRX_ENABLE_STATUS_D0

    uint32_t con_bt_req_in_rssi_thres_cnt_cmp_int;         // CONSECUTIVE_BT_REQ_WITHIN_RSSI_THRESH_CNTR_CMP_THRESH
    uint32_t con_bt_req_above_rssi_thres_cnt_cmp_int;      // CONSECUTIVE_BT_REQ_ABOVE_RSSI_UPPER_THRESH_CNTR_CMP_THRESH
    uint32_t con_bt_req_below_rssi_thres_cnt_cmp_int;      // CONSECUTIVE_BT_REQ_BELOW_RSSI_LOWER_THRESH_CNTR_CMP_THRESH
    uint32_t con_bt_req_crx_allow_set_cnt_cmp_int;         // CONSECUTIVE_BT_REQ_CRX_ALLOW_SET_CNTR_CMP_THRESH
    uint32_t con_bt_req_crx_allow_not_set_cnt_cmp_int;     // CONSECUTIVE_BT_REQ_CRX_ALLOW_NOT_SET_CNTR_CMP_THRESH
    uint32_t con_bt_req_in_rssi_thres_cnt_cmp_int_sts;     // CONSECUTIVE_BT_REQ_WITHIN_RSSI_THRESH_CNTR_INTR_STATUS
    uint32_t con_bt_req_above_rssi_thres_cnt_cmp_int_sts;  // CONSECUTIVE_BT_REQ_ABOVE_RSSI_UPPER_THRESH_CNTR_INTR_STATUS
    uint32_t con_bt_req_below_rssi_thres_cnt_cmp_int_sts;  // CONSECUTIVE_BT_REQ_BELOW_RSSI_LOWER_THRESH_CNTR_INTR_STATUS
    uint32_t con_bt_req_crx_allow_set_cnt_cmp_int_sts;     // CONSECUTIVE_BT_REQ_CRX_ALLOW_SET_CNTR_INTR_STATUS
    uint32_t con_bt_req_crx_allow_not_set_cnt_cmp_int_sts;  // CONSECUTIVE_BT_REQ_CRX_ALLOW_NOT_SET_CNTR_INTR_STATUS
} BMH_DBG_CNT_ARB1, *pBMH_DBG_CNT_ARB1;

typedef struct bmh_dbg_cnt_arb2 {
    uint32_t wl_tx_req_nack_other_wlan_tx_reason_cnt;
    uint32_t wl_tx_req_nack_lcmh_reason_cnt;
    uint32_t coex_tx_resp_concurrent_wlan_tx_cnt;
    uint32_t coex_tx_resp_alt_based_cnt;
    uint32_t coex_tx_resp_default_based_cnt;
    uint32_t tx_status_resp_tx_end_cnt;
    uint32_t tx_status_resp_tx_start_cnt;
    uint32_t tx_status_fes_end_cnt;
    uint32_t tx_status_fes_tx_end_cnt;
    uint32_t tx_status_fes_tx_start_cnt;
    uint32_t tx_flush_cnt;
    uint32_t tbtt_cnt;
    uint32_t wl_in_tx_abort_cnt;
    uint32_t wl_tx_auto_resp_req_cnt;
    uint32_t wl_tx_req_ack_cnt;
    uint32_t wl_tx_req_cnt;
} BMH_DBG_CNT_ARB2, *pBMH_DBG_CNT_ARB2;

typedef struct bmh_dbg_cnt_common {
    uint32_t wmac1_bt_wlan_medium_usage_cnt;
    uint32_t wmac2_bt_wlan_medium_usage_cnt;
    uint32_t dbg_sync_cnt;
    uint32_t dbg_out_of_sync_cnt;
    uint32_t channel_busy_cnt;
    uint32_t bt_tx_cnt;
    uint32_t bt_rx_cnt;
} BMH_DBG_CNT_COMMON, *pBMH_DBG_CNT_COMMON;

typedef struct cxc_pmh_pwrup_ctrl {
    /* CXC_PMH_R0_POWERUP_CTRL register */
    boolean int_seq_wlan1;         // CXC_PMH_R0_POWERUP_CTRL [0:0]
    boolean pwrup_with_wsi_wlan1;  // CXC_PMH_R0_POWERUP_CTRL [1:1]
    boolean pwrup_with_bt_wlan1;   // CXC_PMH_R0_POWERUP_CTRL [2:2]
    boolean pwrup_with_lna_wlan1;  // CXC_PMH_R0_POWERUP_CTRL [3:3]
    boolean pwrup_with_lte_wlan1;  // CXC_PMH_R0_POWERUP_CTRL [4:4]
    boolean pwrup_with_smh_wlan1;  // CXC_PMH_R0_POWERUP_CTRL [5:5]
    boolean int_seq_wlan2;         // CXC_PMH_R0_POWERUP_CTRL [6:6]
    boolean pwrup_with_wsi_wlan2;  // CXC_PMH_R0_POWERUP_CTRL [7:7]
    boolean pwrup_with_bt_wlan2;   // CXC_PMH_R0_POWERUP_CTRL [8:8]
    boolean pwrup_with_lna_wlan2;  // CXC_PMH_R0_POWERUP_CTRL [9:9]
    boolean pwrup_with_lte_wlan2;  // CXC_PMH_R0_POWERUP_CTRL [10:10]
    boolean pwrup_with_smh_wlan2;  // CXC_PMH_R0_POWERUP_CTRL [11:11]
    boolean sw_active_mcib;        // CXC_PMH_R0_POWERUP_CTRL [12:12]
    boolean bt_resp_tmout_wlan1;   // CXC_PMH_R0_POWERUP_CTRL [22:22]
    boolean bt_resp_tmout_wlan2;   // CXC_PMH_R0_POWERUP_CTRL [23:23]
    // boolean init_status_wlan1; //CXC_PMH_R0_POWERUP_CTRL [24:24]
    // uint8_t init_result_wlan1; //CXC_PMH_R0_POWERUP_CTRL [27:25]
    // boolean init_status_wlan2; //CXC_PMH_R0_POWERUP_CTRL [28:28]
    // uint8_t init_result_wlan2; //CXC_PMH_R0_POWERUP_CTRL [31:29]
} CXC_PMH_PWRUP_CTRL, *pCXC_PMH_PWRUP_CTRL;

typedef struct cxc_pmh_pwrdown_ctrl {
    /* CXC_PMH_R0_POWERUP_CTRL register */
    boolean int_seq_wlan1;               // CXC_PMH_R0_POWERDOWN_CTRL [0:0]
    boolean int_seq_wlan2;               // CXC_PMH_R0_POWERDOWN_CTRL [1:1]
    boolean pwrdown_pmu_to_wlan1_allow;  // CXC_PMH_R0_POWERDOWN_CTRL [2:2]
    boolean pwrdown_pmu_to_wlan2_allow;  // CXC_PMH_R0_POWERDOWN_CTRL [3:3]
    boolean smh_gpm_block;               // CXC_PMH_R0_POWERDOWN_CTRL [4:4]
    boolean lmh_lna_take_block;          // CXC_PMH_R0_POWERDOWN_CTRL [5:5]
    boolean lcmh_tdm_block;              // CXC_PMH_R0_POWERDOWN_CTRL [6:6]
    boolean lcmh_rtsm_block;             // CXC_PMH_R0_POWERDOWN_CTRL [7:7]
    uint8_t mcim_shutdown_delay;         // CXC_PMH_R0_POWERUP_CTRL [13:8]
    boolean sys_sleep_disable;           // CXC_PMH_R0_POWERDOWN_CTRL [26:26]
} CXC_PMH_PWRDOWN_CTRL, *pCXC_PMH_PWRDOWN_CTRL;

typedef struct coex_pmh_context {
    CXC_PMH_PWRUP_CTRL pwrup_ctrl;
    CXC_PMH_PWRDOWN_CTRL pwrdown_ctrl;
    uint32_t pwrdown_tout;
    uint32_t intr_enable;

} COEX_PMH_CONTEXT, *pCOEX_PMH_CONTEXT;

typedef struct coex_bmh_context {
    boolean mac_dis;  // CXC_BMH_R0_WMAC1/2/3_CTRL [0:0]
    uint32_t intr_enable;
    uint32_t intr_1_enable;
    // boolean lte_coex_output_en[HAL_NOF_PMAC_IN_DEVICE]; // CXC_BMH_R0_WMAC1/2/3_CTRL [3:3]
    uint32_t use_2x2_mode;      // Usage 0: WMAC1/2/3 NOT use 2x2 mode, 1: WMAC1 use 2x2 mode, 2: WMAC2 use 2x2 mode, 3:
                                // WMAC3 use 2x2 mode.
    uint8_t rx_ant_mask;        // CXC_BMH_R0_WMAC1/2/3_CTRL [9:8]
    boolean cca_thresh_en_bt;   // CXC_BMH_R0_WMAC1/2/3_CTRL [10:10]
    boolean cca_thresh_en_lte;  // CXC_BMH_R0_WMAC1/2/3_CTRL [11:11]
    boolean lna_signal_en;      // CXC_BMH_R0_WMAC1/2/3_CTRL [12:12]
    uint32_t coex_wlan_weight[NUM_WLAN_WEIGHT];
    uint8_t max_wlan_txpower[NUM_MAX_WLAN_TX_PWR][COEX_WHAL_WLAN_BW_MAX + 1]; /* 11*3 dimension */
    uint8_t selfgen_offset_weight[NUM_SELFGEN_PKT_TYPE];
    // BTCOEX_CTRL
    boolean mci_rx_disable_ts;                    // CXC_BMH_R0_CTRL [0:0]
    boolean conc_tx_en;                           // CXC_BMH_R0_CTRL [1:1]
    boolean osla_en;                              // CXC_BMH_R0_CTRL [2:2]
    boolean wl_tx_pri_allows_bt_tx_shared_ant;    // CXC_BMH_R0_CTRL [3:3]
    boolean wl_tx_pri_allows_bt_rx_shared_ant;    // CXC_BMH_R0_CTRL [4:4]
    boolean bt_tx_pri_allows_wl_tx_shared_ant;    // CXC_BMH_R0_CTRL [5:5]
    boolean bt_rx_pri_allows_wl_tx_unshared_ant;  // CXC_BMH_R0_CTRL [6:6]
    boolean cont_eval_en;                         // CXC_BMH_R0_CTRL [7:7]
    boolean swap_en;                              // CXC_BMH_R0_CTRL [8:8]
    boolean bt_aoa_filter_mode;                   // CXC_BMH_R0_CTRL [9:9]
    uint8_t wl_pri_sw_ctrl;                       // CXC_BMH_R0_CTRL [15:12]
    uint8_t wl_tx_power_thresh_bt_rx;             // CXC_BMH_R0_CTRL [22:16]
    uint8_t arb1_wlan_idx;                        // CXC_BMH_R0_CTRL [24:23]
    uint8_t arb2_wlan_idx;                        // CXC_BMH_R0_CTRL [26:25]
    uint8_t arb3_wlan_idx;                        // CXC_BMH_R0_CTRL [28:27]
    uint32_t mci_dis;                             // CXC_BMH_R0_CTRL [29:29]
    // BTCOEX_CTRL2
    boolean conc_wl_wl_tx_en;                           // CXC_BMH_R0_CTRL2 [0:0]
    boolean wl_wl_pwr_backoff_en;                       // CXC_BMH_R0_CTRL2 [1:1]
    boolean wl1_tx_status_blank_en;                     // CXC_BMH_R0_CTRL2 [2:2]
    boolean wl2_tx_status_blank_en;                     // CXC_BMH_R0_CTRL2 [3:3]
    boolean bt_tx_status_blank_en;                      // CXC_BMH_R0_CTRL2 [4:4]
    boolean deweight_rx_en;                             // CXC_BMH_R0_CTRL2 [9:9]
    boolean wl_1s_rx_allow_bt_tx;                       // CXC_BMH_R0_CTRL2 [10:10]
    boolean bt_rx_allow_wl_unshared_chain_tx;           // CXC_BMH_R0_CTRL2 [11:11]
    uint8_t bt_shared_ant_mask;                         // CXC_BMH_R0_CTRL2 [15:12]
    uint8_t wl_max_tx_pwr_thresh_during_bt_rx_low_pri;  // CXC_BMH_R0_CTRL2 [22:16]
    boolean wl3_tx_status_blank_enable;                 // CXC_BMH_R0_CTRL2 [23:23]
    uint8_t wl_max_tx_pwr_thresh_during_bt_rx_hi_pri;   // CXC_BMH_R0_CTRL2 [30:24]
    // BTCOEX_CTRL3
    uint16_t cont_info_timeout;  // CXC_BMH_R0_CTRL3 [11:0]
    uint8_t jumping_offset;      // CXC_BMH_R0_CTRL3 [19:12]
    uint8_t default_bt_pri;      // CXC_BMH_R0_CTRL3 [31:24]
    // BTCOEX_CTRL4
    boolean wmac1_force_wait_ba;         // CXC_BMH_R0_CTRL4 [0:0]
    boolean wmac1_sch_block_backoff;     // CXC_BMH_R0_CTRL4 [1:1]
    boolean wmac1_txpcu_block_response;  // CXC_BMH_R0_CTRL4 [2:2]
    boolean wmac1_lte_block_sch_en;      // CXC_BMH_R0_CTRL4 [3:3]
    boolean wmac2_force_wait_ba;         // CXC_BMH_R0_CTRL4 [8:8]
    boolean wmac2_sch_block_backoff;     // CXC_BMH_R0_CTRL4 [9:9]
    boolean wmac2_txpcu_block_response;  // CXC_BMH_R0_CTRL4 [10:10]
    boolean wmac2_lte_block_sch_en;      // CXC_BMH_R0_CTRL4 [11:11]
    boolean wmac3_force_wait_ba;         // CXC_BMH_R0_CTRL4 [12:12]
    boolean wmac3_sch_block_backoff;     // CXC_BMH_R0_CTRL4 [13:13]
    boolean wmac3_txpcu_block_response;  // CXC_BMH_R0_CTRL4 [14:14]
    boolean wmac3_lte_block_sch_en;      // CXC_BMH_R0_CTRL4 [15:15]
    // BTCOEX_CTRL5
    uint8_t max_sch_iteration;       // CXC_BMH_R0_CTRL5 [5:0]
    uint16_t cont_info_bt_end_time;  // CXC_BMH_R0_CTRL5 [15:6]
    uint16_t tlv_resend_delay;       // CXC_BMH_R0_CTRL5 [27:16]
    uint8_t max_bt_offset;           // CXC_BMH_R0_CTRL5 [31:28]
    // BTCOEX_CTRL6
    uint16_t bt_pri_select_bt_end;    // CXC_BMH_R0_CTRL6 [15:0]
    uint16_t jumping_offset_timeout;  // CXC_BMH_R0_CTRL6 [27:16]

    // BTCOEX_CTRL7
    boolean wlan_post_packet_priority_en;           // CXC_BMH_R0_CTRL7 [0:0]
    boolean lte_rx_wlan_tx_allowed_with_pwr_bkoff;  // CXC_BMH_R0_CTRL7 [1:1]
    boolean modified_mci_cont_nack_en;              // CXC_BMH_R0_CTRL7 [2:2]
    boolean coex_mac_nap_tlv_en;                    // CXC_BMH_R0_CTRL7 [3:3]
    boolean pwr_res_incr_en;                        // CXC_BMH_R0_CTRL7 [5:5]PWR_RES_INCR_EN
    boolean coex_status_broadcast_new_fields_en;    // CXC_BMH_R0_CTRL7 [6:6]COEX_STATUS_BROADCAST_NEW_FIELDS_EN

    // CXC_BMH_R0_LCMH_TX_GRANT_ENABLE_MASK
    uint8_t wmac1_lcmh_tx_grant_enable_mask;  // CXC_BMH_R0_LCMH_TX_GRANT_ENABLE_MASK[7:0]
    uint8_t wmac1_lcmh_txrx_on_enable_mask;   // CXC_BMH_R0_LCMH_TX_GRANT_ENABLE_MASK[15:8]
    uint8_t wmac2_lcmh_tx_grant_enable_mask;  // CXC_BMH_R0_LCMH_TX_GRANT_ENABLE_MASK[23:16]
    uint8_t wmac2_lcmh_txrx_on_enable_mask;   // CXC_BMH_R0_LCMH_TX_GRANT_ENABLE_MASK[31:24]
    // wmac3_* are hard code to 0 in HW

    // CXC_BMH_R0_LCMH_SCHD_RATID_ENABLE_MASK
    uint16_t wmac1_lcmh_schd_ratid_enable_mask;
    uint16_t wmac2_lcmh_schd_ratid_enable_mask;

    // Frame Slicing
    uint32_t slice_thresh1;
    uint32_t slice_thresh2;
    uint32_t slice_thresh3;
    uint32_t slice_thresh4;

    pBMH_DBG_CNT_ARB1 pBmhDbgCntArb1;
    pBMH_DBG_CNT_ARB2 pBmhDbgCntArb2;
    pBMH_DBG_CNT_COMMON pBmhDbgCntCommon;

    /* TBD */
} COEX_BMH_CONTEXT, *pCOEX_BMH_CONTEXT;

typedef struct coex_lte_rat_id {
    boolean rat_id_enable;
    uint8_t lte_rat_id1;
    uint8_t lte_rat_id0;
    uint8_t wlan1_rat_id;
    uint8_t wlan2_rat_id;
    uint8_t wlan3_rat_id;
} COEX_LTE_RAT_ID, *pCOEX_LTE_RAT_ID;

typedef struct coex_lte_wlan_rx_pri_control {
    uint8_t coex_wlan_iface;   // Sw mode
    boolean select_mode;       /* CXC_LCMH_R0_WLAN_RX_PRI_CONTROL[11:11] 0: refer to select, 1: refer to select2 */
    uint8_t select;            /* CXC_LCMH_R0_WLAN_RX_PRI_CONTROL[10:9][8:7] */
    boolean overwrite_value;   // CXC_LCMH_R0_WLAN1_RX_PRI_CONTROL_RESULT [1:1]
    boolean overwrite_enable;  // CXC_LCMH_R0_WLAN1_RX_PRI_CONTROL_RESULT [2:2]
    boolean value2;            // CXC_LCMH_R0_WLAN1_RX_PRI_CONTROL_RESULT [3:3]
    boolean enable2;           // CXC_LCMH_R0_WLAN1_RX_PRI_CONTROL_RESULT [4:4]
    boolean value1;            // CXC_LCMH_R0_WLAN1_RX_PRI_CONTROL_RESULT [5:5]
    boolean enable1;           // CXC_LCMH_R0_WLAN1_RX_PRI_CONTROL_RESULT [6:6]
    boolean rx_pri_select;     // CXC_LCMH_R0_WLAN1_RX_PRI_CONTROL_RESULT [8:8] 1: Select SW generated RX_PRI 0:
                            // WLAN1_RX_ON
    boolean rx_pri_sw;  // CXC_LCMH_R0_WLAN1_RX_PRI_CONTROL_RESULT [9:9]

    uint32_t threshold1;
    uint32_t threshold2;
} COEX_LTE_WLAN_RX_PRI_CONTROL, *pCOEX_LTE_WLAN_RX_PRI_CONTROL;

typedef struct coex_lte_tx_grant_control {
    boolean overwrite_value;   // CXC_LCMH_R0_WLAN1/2/3_TX_GRANT_CONTROL [1:1]
    boolean overwrite_enable;  // CXC_LCMH_R0_WLAN1/2/3_TX_GRANT_CONTROL [2:2]
    boolean value2;            // CXC_LCMH_R0_WLAN1/2/3_TX_GRANT_CONTROL [3:3]
    boolean enable2;           // CXC_LCMH_R0_WLAN1/2/3_TX_GRANT_CONTROL [4:4]
    boolean value1;            // CXC_LCMH_R0_WLAN1/2/3_TX_GRANT_CONTROL [5:5]
    boolean enable1;           // CXC_LCMH_R0_WLAN1/2/3_TX_GRANT_CONTROL [6:6]
    uint32_t threshold1;
    uint32_t threshold2;
} COEX_LTE_TX_GRANT_CONTROL, *pCOEX_LTE_TX_GRANT_CONTROL;

typedef struct coex_lte_lcxm_config {
    uint16_t num_clocks_per_micro_sec;      // CXC_LCMH_R0_CONFIGURATION [9:0]
    boolean en_tx_fifo_flush_before_sleep;  // CXC_LCMH_R0_CONFIGURATION [10:10]
    boolean type0_clear_upon_wakeup;        // CXC_LCMH_R0_CONFIGURATION [11:11]
    boolean wr_en_timer_counter;            // CXC_LCMH_R0_CONFIGURATION [12:12]
    uint8_t wlan_tx_on_select;              // CXC_LCMH_R0_CONFIGURATION [14:13]
    boolean send_rtsm_after_pwrup_sel;      // CXC_LCMH_R0_CONFIGURATION [15:15]
    uint8_t scheduling_feature_mode;        // CXC_LCMH_R0_CONFIGURATION [18:17]
    boolean scheduling_mode_select;         // CXC_LCMH_R0_CONFIGURATION [19:19]
    uint8_t lcmh_sw_clkgate_disable;        // CXC_LCMH_R0_CONFIGURATION [23:20]
} COEX_LTE_LCXM_CONFIG, *pCOEX_LTE_LCXM_CONFIG;

typedef struct coex_lcmh_context {
    COEX_LTE_WHALINFO whalinfo;
    LCMH_INTR_STAT intr_stat;
    LCMH_MSG_Q msg_q;
    uint32_t type0_response_duration;
    uint32_t txfifo_sendtime_delay;
    uint32_t intr_enable;
    uint32_t strobe_intr_enable;
    boolean type3_message_enable;
    COEX_LTE_RAT_ID rat_id;
    COEX_LTE_WLAN_RX_PRI_CONTROL wlan_rx_pri;
    COEX_LTE_TX_GRANT_CONTROL wlan_tx_grant;
    COEX_LTE_LCXM_CONFIG lcxm_config;
    uint8_t schedule_info_priority;
    uint8_t schedule_info_power;
    uint32_t tx_schedule_pattern_ratid0;
    uint32_t rx_schedule_pattern_ratid0;
    uint32_t tx_schedule_pattern_ratid1;
    uint32_t rx_schedule_pattern_ratid1;
} COEX_LCMH_CONTEXT, *pCOEX_LCMH_CONTEXT;

typedef struct coex_lmh_context {
    LMH_WHAL_INTR_INFO intr_info;
    uint32_t lna_state_timeout;
    uint32_t lna_in_use_timeout;
    uint32_t lna_locked_timeout;

    /* TBD */

} COEX_LMH_CONTEXT, *pCOEX_LMH_CONTEXT;

typedef struct coex_smh_context {
    /* Programming Guide for gpm_base_addr:
     * Value have to be right shifted 2 bits before writing into the register and
     * last 2 bits [1:0] should be 0 all the time.
     */
    uint32_t *gpm_base_addr; /* allocate as 64 bit address*/
    uint32_t ring_enteries;
    uint32_t ring_head_idx;
    uint32_t ring_tail_idx;
    uint32_t intr_enable;
    boolean mci_rx_ctrl_disable_timestamp; /*0 - absolute time mode; 1 - delta time mode */
    boolean clkgate_disable;
    uint32_t t1_field;
    uint32_t body0;
    uint32_t body1;
    uint32_t body2;
    uint32_t prev_gpm_timestamp;
    uint8_t major_version;
    uint8_t minor_version;
    uint8_t msgType;
    uint8_t msgLen;
    uint8_t msgBody[16];
} COEX_SMH_CONTEXT, *pCOEX_SMH_CONTEXT;

typedef struct coex_mci_context {
    MCI_INTR_INFO intr_info;
    MCI_MODE_CTRL mode_ctrl;
    MCI_WSIM_MODE wsim_mode;
    uint8_t x_header;  // WCSS_UMAC_CXC_MCIBASIC_REG_CXC_MCIBASIC_R0_EXTRA_TYPE_MSG [7:0]
    uint8_t x_body;    // WCSS_UMAC_CXC_MCIBASIC_REG_CXC_MCIBASIC_R0_EXTRA_TYPE_MSG[15:8]
    MCI_TIME_INTERVAL mci_time_interval;
    MCI_BTSIM_MSG_CNTRL btsim_msg_cntrl;
    MCI_BTSIM_CONFIG btsim_config;
    /* TBD */

} COEX_MCI_CONTEXT, *pCOEX_MCI_CONTEXT;

typedef struct coex_pta_bluetooth_mode {
    uint8_t first_slot_time;
    uint8_t priority_time;
    boolean rx_clear_polarity;
    uint8_t wbcnt_bt_priority_ctrl;
    uint8_t wbcnt_bt_active_ctrl;
    uint8_t bt_priority_extend_threshold;
    boolean bt_tx_on_en;
    boolean dynamic_toggle_wla_disable;
    uint16_t bt_priority_extend;
    uint16_t bt_active_extend;
    boolean tx_on_src;
} COEX_PTA_BLUETOOTH_MODE, *pCOEX_PTA_BLUETOOTH_MODE;

typedef struct coex_pta_context {
    uint32_t pta_en;
    uint32_t pta_mode;
    uint8_t wlan_active_sel;
    uint8_t wlan_in_rx_sel;
    uint8_t wlan_in_tx_sel;
    boolean cont_info_txrx;
    uint8_t delay_time;
    uint8_t wait_time;
    uint8_t nack_time;
    uint8_t tx_channel;
    uint8_t tx_txpwr;
    uint8_t tx_link;
    uint8_t rx_channel;
    uint8_t rx_rssi;
    uint8_t rx_link;
    COEX_PTA_BLUETOOTH_MODE btMode;
    uint32_t weight;
} COEX_PTA_CONTEXT, *pCOEX_PTA_CONTEXT;

typedef struct coex_pdg_reg_config {
    uint16_t cts_offset;        // PDG_R0_COEX_CTS_OFFSET [15:0]
    uint16_t cts_min_duration;  // PDG_R0_COEX_CTS_MIN_FES_DURATION [15:0]
    uint32_t hw_mode_coex_disable;
    uint32_t resp_mode_coex_disable;
    uint32_t sw_mode_coex_disable;
} COEX_PDG_REG_CONFIG, *pCOEX_PDG_REG_CONFIG;

typedef struct coex_tlv_config {
    uint32_t low_pri_slicing_allowed;
    uint32_t high_pri_slicing_allowed;
    uint32_t tx_chain_mask;
} COEX_TLV_CONFIG, *pCOEX_TLV_CONFIG;

typedef struct coex_hwsch_reg_config {
    boolean auto_flush_disable;        // HWSCH_R0_COEX_CTRL [3:3]
    boolean soft_2_hard_abort_enable;  // HWSCH_R0_COEX_CTRL [4:4]
    boolean filter_soft_abort_enable;  // HWSCH_R0_COEX_CTRL [5:5]
    uint16_t cca_sw_en_coex;           // HWSCH_R0_CCA_SW_EN_COEX
} COEX_HWSCH_REG_CONFIG, *pCOEX_HWSCH_REG_CONFIG;

typedef struct coex_wmac_context {
    COEX_PDG_REG_CONFIG pdg_reg_config;
    COEX_HWSCH_REG_CONFIG hwsch_reg_config;
    COEX_TLV_CONFIG tlv_config;
} COEX_WMAC_CONTEXT, *pCOEX_WMAC_CONTEXT;

typedef struct pta_interrupt {
    uint32_t bt_active_rising;
    uint32_t bt_active_falling;
    uint32_t bt_low_pri_rising;
    uint32_t bt_low_pri_falling;
    uint32_t bt_hi_pri_rising;
    uint32_t bt_hi_pri_falling;
    uint32_t wlan_stomped;
    uint32_t bt_stomped;
} PTA_INTERRUPT, *pPTA_INTERRUPT;

typedef struct smh_interrupt {
    uint32_t rx_mci_cpu_intr;
    uint32_t rx_mci_reset_intr;
    uint32_t rx_mci_gpm_intr;
    uint32_t rx_mci_gpm_full_err_intr;
    uint32_t tx_mci_gpm_done_intr;
    uint32_t tx_mci_cpu_int_done_intr;
    uint32_t tx_remote_reset_done_intr;
    uint32_t ahb_err_intr;
    uint32_t ccu_access_err_intr;
    uint32_t gpm_skip_err_intr;
    uint32_t tx_mci_cpu_int_pwrdown_err;
    uint32_t tx_mci_remote_rst_pwrdown_err;
} SMH_INTERRUPT, *pSMH_INTERRUPT;

typedef struct gpm_msg_check {
    uint32_t gpm_rx_msg_mismatch;
    uint32_t gpm_rx_msg_match;
    uint32_t gpm_rx_timestamp_err;
    uint32_t gpm_rx_timestamp_normal;
    uint32_t gpm_rx_invallid;
    uint32_t gpm_tx_pwrdown_err;
    uint32_t gpm_tx_success;
} GPM_MSG_CHECK, *pGPM_MSG_CHECK;

typedef struct mcib_interrupt {
    uint32_t sw_type1_err_evt;
    uint32_t sw_type2_err_evt;
    uint32_t btc_type1_err_evt;
    uint32_t btc_type2_err_evt;
    uint32_t lna_type1_err_evt;
    uint32_t lna_type2_err_evt;
    uint32_t lte_type1_err_evt;
    uint32_t lte_type2_err_evt;
    uint32_t pmu_type1_err_evt;
    uint32_t pmu_type2_err_evt;
    uint32_t chksum_err_evt;
    uint32_t bussync_err_evt;
    uint32_t wsi_stat_err_evt;
    uint32_t tx_nak_err_evt;
    uint32_t rx_nak_err_evt;
    uint32_t dest_err_evt;
    uint32_t fifo_overwrite_err_evt;
    uint32_t ccu_access_err_evt;
    uint32_t setup_time_check_err_evt;
    uint32_t injector_err_evt;
} MCIB_INTERRUPT, *pMCIB_INTERRUPT;

typedef struct pmh_interrupt {
    uint32_t bt_sys_waking_intr;         /* When set an MCI_SYS_WAKING message has been received */
    uint32_t bt_sys_sleeping_intr;       /* When set an MCI_SYS_SLEEPING message has been received */
    uint32_t sw_wlan1_pwr_dwn_done_intr; /* When set the SW initiated power down sequence has finished (result status is
                                            available) */
    uint32_t sw_wlan2_pwr_dwn_done_intr; /* When set the SW initiated power down sequence has finished (result status is
                                            available) */
    uint32_t pmu_wlan1_pwr_dwn_done_intr; /* When set the PMU initiated power down sequence has finished */
    uint32_t pmu_wlan2_pwr_dwn_done_intr; /* When set the PMU initiated power down sequence has finished */
    uint32_t pwr_down_timeout_intr; /* When set the power down sequence was not yet finished by the time the timeout
                                       timer expired. */
    uint32_t bt_resp_timeout_intr;  /* When set no BT CONT_INFO or CONT_RST was received within timout after sending
                                       MCU_SYS_WAKING */
    uint32_t wsi_bus_sync_intr;     /* When set a WSI bus sync error was detected. */
    uint32_t ccu_access_err_intr;   /* When set a CCU read/write unsuccessful*/
    uint32_t sw_wlan3_pwr_dwn_done_intr; /* When set the SW initiated power down sequence has finished (result status is
                                            available) */
    uint32_t pmu_wlan3_pwr_dwn_done_intr; /* When set the PMU initiated power down sequence has finished */
} PMH_INTERRUPT, *pPMH_INTERRUPT;

typedef struct lcmh_interrupt {
    uint32_t strobe_0;
    uint32_t strobe_1;
    uint32_t wci2_type0_msg_tx;
    uint32_t invalid_sub_frame_value_in_type6_msg_intr;
    uint32_t type0_resp_before_tout_intr;
    uint32_t type0_resp_after_tout_intr;
    uint32_t tx_fifo_underflow_intr;
    uint32_t tx_fifo_overflow_intr;
    uint32_t tx_fifo_almost_empty_intr;
    uint32_t tx_fifo_almost_full_intr;
    uint32_t rx_fifo_underflow_intr;
    uint32_t rx_fifo_overflow_intr;
    uint32_t rx_fifo_almost_empty_intr;
    uint32_t rx_fifo_almost_full_intr;
    uint32_t type0_msg_with_frame_sync_1_intr;
    uint32_t mws_rx_rising_edge_intr;
    uint32_t mws_rx_falling_edge_intr;
    uint32_t mws_tx_rising_edge_intr;
    uint32_t mws_tx_falling_edge_intr;
    uint32_t rx_802_rx_pri_rising_edge_intr;
    uint32_t rx_802_rx_pri_falling_edge_intr;
    uint32_t tx_802_tx_on_rising_edge_intr;
    uint32_t tx_802_tx_on_falling_edge_intr;
    uint32_t wci2_type0_msg_rx_intr;
    uint32_t wci2_type1_msg_rx_intr;
    uint32_t wci2_type2_msg_rx_intr;
    uint32_t wci2_type3_msg_rx_intr;
    uint32_t wci2_type4_msg_rx_intr;
    uint32_t wci2_type5_msg_rx_intr;
    uint32_t wci2_type6_msg_rx_intr;
    uint32_t wci2_type7_msg_rx_intr;
    uint32_t direct_wci2_trig_intr;
    uint32_t rx_fifo_lsb_nibble_order_err_intr;
    uint32_t rx_fifo_msb_nibble_order_err_intr;
    uint32_t type7_msg_err_intr;
    uint32_t type6_msg_err_intr;
    uint32_t type0_msg_err_intr;
    uint32_t no_ratid_match_err_intr;
} LCMH_INTERRUPT, *pLCMH_INTERRUPT;

typedef struct lmh_interrupt {
    uint32_t rx_mci_lna_state_intr;
    uint32_t rx_mci_lna_bt_lock_intr;
    uint32_t bt_lock_intr;
    uint32_t bt_unlock_intr;
    uint32_t bt_lna_in_use_intr;
    uint32_t bt_lna_in_use_clear_intr;
    uint32_t lna_state_tout_intr;
    uint32_t lna_in_use_tout_intr;
    uint32_t lna_lock_tout_intr;
    uint32_t ccu_access_err_intr;
} LMH_INTERRUPT, *pLMH_INTERRUPT;

typedef struct RAW_1_INTERRUPT {
    uint32_t invalid_bt_pwr;
    uint32_t bt_txrx_cnt_limit;
    uint32_t bt_schd_pri;
    uint32_t bmh_register_access_err;
    uint32_t bmh1_wl_intx_wdg_timeout;
    uint32_t bmh1_wl_inrx_wdg_timeout;
    uint32_t bmh2_wl_intx_wdg_timeout;
    uint32_t bmh2_wl_inrx_wdg_timeout;
    uint32_t bmh1_wl_txrx_err;
    uint32_t bmh2_wl_txrx_err;
    uint32_t bmh1_tlvout_timeout_err;
    uint32_t bmh2_tlvout_timeout_err;
    uint32_t bmh3_wl_intx_wdg_timeout;
    uint32_t bmh3_wl_inrx_wdg_timeout;
    uint32_t bmh3_wl_txrx_err;
    uint32_t bmh3_tlvout_timeout_err;
} RAW_1_INTERRUPT, *pRAW_1_INTERRUPT;

typedef struct RAW_INTERRUPT {
    uint32_t apb_err_wmac1;
    uint32_t apb_err_wmac2;
    uint32_t rx_invalid_hdr;
    uint32_t schd_btrx_intr;
    uint32_t schd_bttx_intr;
    uint32_t wltxsm_invld_seq_wmac1;
    uint32_t wltxsm_invld_seq_wmac2;
    uint32_t rx_msg;
    uint32_t bt_pri;
    uint32_t bt_pri_thr;
    uint32_t bt_freq;
    uint32_t bt_stomp;
    uint32_t cont_info_timeout;
} RAW_INTERRUPT, *pRAW_INTERRUPT;

typedef struct RX_MSG_INTERRUPT {
    uint32_t cont_nack;
    uint32_t cont_info;
    uint32_t cont_rst;
    uint32_t schd_info;
    uint32_t cont_linkid_int;
    uint32_t schd_linkid_int;
    uint32_t crx_enable_tx_int;
} RX_MSG_INTERRUPT, *pRX_MSG_INTERRUPT;

typedef struct BMH_INTERRUPT {
    RAW_1_INTERRUPT int1raw_intr;
    RAW_INTERRUPT intraw_intr;
    RX_MSG_INTERRUPT rxmsg_intr;
    PTA_INTERRUPT pta_intr;
    PTA_INTERRUPT pta2_intr;
} BMH_INTERRUPT, *pBMH_INTERRUPT;

typedef struct COEX_STATS {
    /* COEX interrupt related */
    uint32_t bmh_intr_bmsk;
    uint32_t lmh_intr_bmsk;
    uint32_t pmh_intr_bmsk;
    uint32_t smh_intr_bmsk;
    uint32_t mcim_intr_bmsk;
    uint32_t lcmh_strobe_intr_bmsk;
    uint32_t lcmh_wci2_intr_bmsk;
    BMH_INTERRUPT bmh_intr;
    SMH_INTERRUPT smh_intr;
    MCIB_INTERRUPT mcib_intr;
    PMH_INTERRUPT pmh_intr;
    LCMH_INTERRUPT lcmh_intr;
    LMH_INTERRUPT lmh_intr;
    GPM_MSG_CHECK gpm_check;

    /* some other stats rather than intr related */
} COEX_STATS, *pCOEX_STATS;

#ifdef SUPPORT_PTA_COEX
#define UFW_COEX_MAX_PTA_INTERFACE 1
#endif

/* COEX configuration for each module */
typedef struct WLAN_CXC_CTXT {
    boolean coex_enable;
    uint32_t gpm_pkt_cnt;
    pCOEX_BMH_CONTEXT pBmhCtxt;
    pCOEX_PMH_CONTEXT pPmhCtxt;
    pCOEX_SMH_CONTEXT pSmhCtxt;
    pCOEX_LMH_CONTEXT pLmhCtxt;
    pCOEX_LCMH_CONTEXT pLcmhCtxt;
    pCOEX_MCI_CONTEXT pMciCtxt;
#ifdef SUPPORT_PTA_COEX
    pCOEX_PTA_CONTEXT pPtaCtxt[UFW_COEX_MAX_PTA_INTERFACE];
#endif
    pCOEX_WMAC_CONTEXT pWmacCtxt;
    COEX_STATS stats;
    uint32_t coexCounter;
    uint32_t coexDelayCInfoRst;
    uint32_t contRstCountr;
    uint32_t continfoCountr;
    uint32_t coexMode;
    uint32_t newMsg;
    uint32_t coex_param1;
    uint32_t coex_param2;
} WLAN_CXC_CTXT, *pWLAN_CXC_CTXT;
typedef struct smh_intr_stats {
    uint32_t rx_mci_cpu_cnt;
    uint32_t rx_mci_reset_cnt;
    uint32_t rx_mci_gpm_cnt;
    uint32_t rx_mci_gpm_full_err_cnt;
    uint32_t tx_mci_gpm_done_cnt;
    uint32_t tx_mci_cpu_int_done_cnt;
    uint32_t tx_remote_reset_done_cnt;
    uint32_t ahb_err_cnt;
    uint32_t ccu_access_err_cnt;
    uint32_t gpm_skip_err_cnt;
} SMH_INTR_STATS, *pSMH_INTR_STATS;

typedef struct mcib_intr_stats {
    uint32_t sw_type1_err_cnt;
    uint32_t sw_type2_err_cnt;
    uint32_t btc_type1_err_cnt;
    uint32_t btc_type2_err_cnt;
    uint32_t lna_type1_err_cnt;
    uint32_t lna_type2_err_cnt;
    uint32_t lte_type1_err_cnt;
    uint32_t lte_type2_err_cnt;
    uint32_t pmu_type1_err_cnt;
    uint32_t pmu_type2_err_cnt;
    uint32_t chksum_err_cnt;
    uint32_t bussync_err_cnt;
    uint32_t wsi_stat_err_cnt;
    uint32_t tx_nak_err_cnt;
    uint32_t rx_nak_err_cnt;
    uint32_t dest_err_cnt;
    uint32_t fifo_overwrite_err_cnt;
    uint32_t ccu_access_err_cnt;
    uint32_t setup_time_check_err_cnt;
    uint32_t injector_err_cnt;
} MCIB_INTR_STATS, *pMCIB_INTR_STATS;

typedef struct gpm_msg_stats {
    uint32_t gpm_rx_msg_mismatch_cnt;
    uint32_t gpm_rx_msg_match_cnt;
    uint32_t gpm_rx_timestamp_err_cnt;
    uint32_t gpm_rx_timestamp_normal_cnt;
    uint32_t gpm_tx_pwrdown_err_cnt;
    uint32_t gpm_tx_success_cnt;
} GPM_MSG_STATS, *pGPM_MSG_STATS;
#define MAC_HAL_NOK   0
#define MAC_HAL_OK    1
#define MAC_HAL_ERROR -1

#ifndef HAL_NOF_PMAC_IN_DEVICE
#define HAL_NOF_PMAC_IN_DEVICE 1
typedef enum {
#if (HAL_NOF_PMAC_IN_DEVICE > 0)
    MAC0_ID = 0,
#endif
#if (HAL_NOF_PMAC_IN_DEVICE > 1)
    MAC1_ID = 1,
#endif
#if (HAL_NOF_PMAC_IN_DEVICE > 2)
    MAC2_ID = 2,
#endif
} UFW_MAC_ID_E;

#endif

void MacHalCoexRootClkgenEn(uint32_t enable);
int32_t MacHalCoexPmhPwrupCtrl(UFW_MAC_ID_E macId, pCXC_PMH_PWRUP_CTRL pPwrUpCtrl);
uint32_t MacHalCoexPmhPwrupCtrlStatus(UFW_MAC_ID_E macId);
uint32_t MacHalCoexPmhPwrupCtrlResult(UFW_MAC_ID_E macId);
uint32_t MacHalCoexPmhPwrupCtrlBtRespTout(UFW_MAC_ID_E macId);
int32_t MacHalCoexPmhPwrDownCtrl(UFW_MAC_ID_E macId, pCXC_PMH_PWRDOWN_CTRL pPwrDownCtrl);
uint32_t MacHalCoexPmhPwrdownCtrlStatus(UFW_MAC_ID_E macId);
int32_t MacHalCoexPmhPwrdownCtrlResult(UFW_MAC_ID_E macId);
int32_t MacHalCoexPmhPwrdownTout(uint32_t timeout);
void MacHalCoexPmhIntrEnable(uint32_t mask);
void MacHalCoexPmhClockGateDisable(uint32_t disable);
void MacHalCoexPmhSetBtRespTout(uint32_t timeout);
uint32_t MacHalCoexPmhGetBtPwrState(void);
int32_t MacHalCoexPmhInit(pCOEX_PMH_CONTEXT pPmhCtxt);
void MacHalCoexSmhGpmRingInit(pCOEX_SMH_CONTEXT pSmhCtxt);
void MacHalCoexSmhIntrEnable(uint32_t mask);
uint32_t MacHalCoexSmhGetIntrSt(void);
void MacHalCoexSmhCpuIntrEnable(uint32_t mask);
int32_t MacHalCoexSmhGetCpuIntBody(void);
uint32_t MacHalCoexSmhGetMciGpmInt(void);
int32_t MacHalCoexSmhGpmMsgWrite(uint32_t *pMsg);
char *MacHalCoexSmhGetGpmBaseAddr(pCOEX_SMH_CONTEXT pSmhCtxt);
int32_t MacHalCoexSmhGetGpmHeadIndex(void);
void MacHalCoexSmhGpmHwSend(void);
int32_t MacHalCoexSmhGpmTxStCheck(void);
void MacHalCoexSmhGpmMciHeaderEncode(uint32_t mode);
int32_t MacHalCoexSmhGetGpmTailIndex(void);
void MacHalCoexSmhSetGpmTailIndex(uint32_t index);
void MacHalGetSmhIntrStats(pSMH_INTR_STATS dest, pSMH_INTERRUPT source);
void MacHalGetMcibIntrStats(pMCIB_INTR_STATS dest, pMCIB_INTERRUPT source);
void MacHalGetGpmStats(pGPM_MSG_STATS dest, pGPM_MSG_CHECK source);
void MacHalCoexSmhTxTsDisable(uint32_t disable);
void MacHalCoexSmhClockGateDisable(uint32_t disable);
void MacHalCoexSmhRxTsDisable(uint32_t disable);
uint32_t MacHalCoexSmhGetRxTs(void);
void MacHalCoexSmhRemoteResetEnable(uint32_t enable);
void MacHalCoexSmhTxMciCpuIntEnable(uint32_t enable);
void MacHalCoexSmhTxMciCpuIntBody(uint32_t pMciCpuIntBody);
void MacHalCoexSmhTxTriggerMsgBody(uint32_t pMciCpuIntBody);
void MacHalCoexMciTxExtraTypeMsg(uint32_t x_header, uint32_t x_body, uint32_t x_valid);
void MacHalCoexMciModifiedContNackEn(uint32_t enable);
void MacHalCoexMacNapTlvEn(uint32_t enable);
void MacHalCoexLtePwrBkoffEn(uint32_t enable);
void MacHalCoexHwAbortEn(uint32_t enable);
void MacHalCoexPostPktPriEn(uint32_t enable);
int32_t MacHalCoexSmhInit(pCOEX_SMH_CONTEXT pSmhCtxt);
int32_t MacHalCoexBmhSetCcaThres(UFW_MAC_ID_E macId, MCI_CCA_TYPE mode, uint32_t cca_1, uint32_t cca_20,
                                 uint32_t cca_40, uint32_t cca_80, uint32_t cca_160);
void MacHalCoexBmhGivenBtPwrMaxWlanTxPower(uint32_t bt_power_index, uint32_t wlan_power_20, uint32_t wlan_power_40,
                                           uint32_t wlan_power_80);
void MacHalCoexBmhSetWlanFreqRange(WHAL_BANDWITH_MHZ bw, uint32_t freq_lower, uint32_t freq_upper);
void MacHalCoexBmhSetWlanSliceThres(uint32_t slice_thres1, uint32_t slice_thres2, uint32_t slice_thres3,
                                    uint32_t slice_thres4);
void MacHalCoexBmhSetSameAntPowerLut(uint32_t power_lut_ix_0, uint32_t power_lut_ix_1, uint32_t power_lut_ix_2,
                                     uint32_t power_lut_ix_3);
void MacHalCoexBmhSetOtherAntPowerLut(uint32_t power_lut_ix_0, uint32_t power_lut_ix_1, uint32_t power_lut_ix_2,
                                      uint32_t power_lut_ix_3);
void MacHalCoexBmhSetWlWeight(uint32_t index, uint32_t weight);
void MacHalCoexBmhSetJumpOffset(uint32_t offset, uint32_t timeout);
void MacHalCoexBmhSharedAntMask(uint32_t mask);
void MacHalCoexBmhSetCtrlConfig(pCOEX_BMH_CONTEXT pBmhCtxt);
void MacHalCoexBmhMciDisable(uint32_t mci_dis);
void MacHalCoexBmhSetCtrl2Config(pCOEX_BMH_CONTEXT pBmhCtxt);
void MacHalCoexBmhSetCtrl3Config(pCOEX_BMH_CONTEXT pBmhCtxt);
void MacHalCoexBmhSetCtrl4Config(pCOEX_BMH_CONTEXT pBmhCtxt);
void MacHalCoexBmhSetCtrl5Config(pCOEX_BMH_CONTEXT pBmhCtxt);
void MacHalCoexBmhSetCtrl6Config(pCOEX_BMH_CONTEXT pBmhCtxt);
void MacHalCoexBmhSetCtrl7Config(pCOEX_BMH_CONTEXT pBmhCtxt);
void MacHalCoexBmhSet2x2Mode(uint32_t mode);
void MacHalCoexBmhConcTxPowerThreshold(uint32_t conc_bt_power_thresh);
void MacHalCoexBmhSelfGenConcTxEnable(boolean enable);
void MacHalCoexBmhForceConcEnable(boolean enable);
void MacHalCoexBmhConcTxArbEnable(boolean enable);
void MacHalCoexBmhIntrEnable(uint32_t mask);
void MacHalCoexBmhIntr1Enable(uint32_t mask);
void MacHalCoexBmhIntr2Enable(uint32_t mask);
void MacHalCoexBmhIntrRxMsgEnable(uint32_t mask);
void MacHalCoexBmhSetRxAntMask(UFW_MAC_ID_E macId, uint32_t mask);
void MacHalCoexBmhMacDisable(UFW_MAC_ID_E macId, uint32_t disable);
void MacHalCoexBmhClockGateDisable(uint32_t disable);
void MacHalCoexBmhBtCCAThresEnable(UFW_MAC_ID_E macId, uint32_t enable);
void MacHalCoexBmhLnaSigEnable(UFW_MAC_ID_E macId, uint32_t enable);
uint32_t MacHalCoexBmhReadLastMsgHdr(void);
int32_t MacHalCoexBmhInit(pCOEX_BMH_CONTEXT pBmhCtxt);
void MacHalCoexBmhSetEventMask(uint32_t ix, uint32_t mask);
void MacHalCoexBmhTrcSelect(uint32_t sel);
void MacHalCoexBmhSwTxSoftAbortEn(uint32_t val);
void MacHalCoexBmhSwTxHardAbortEn(uint32_t val);
uint32_t MacHalCoexLcmhGetWbtimer();
void MacHalCoexBmhSetLcmhSchdRatIdEnableMask(pCOEX_BMH_CONTEXT pBmhCtxt);
void MacHalCoexBmhSetLcmhTxGrantEnableMask(pCOEX_BMH_CONTEXT pBmhCtxt);
void MacHalCoexLcmhClockGateDisable(uint32_t disable);
void MacHalCoexLcmhSetConfig(pCOEX_LTE_LCXM_CONFIG pLcxmCfg);
int32_t MacHalCoexLcmhInit(pCOEX_LCMH_CONTEXT pLcmhCtxt);
void MacHalCoexLmhIntrEnable(uint32_t mask);
void MacHalCoexLmhMciLnaStateTimeout(uint32_t timeout);
void MacHalCoexLmhMciLnaInUseTimeout(uint32_t timeout);
void MacHalCoexLmhMciLnaLockedTimeout(uint32_t timeout);
void MacHalCoexLmhClockGateDisable(uint32_t disable);
int32_t MacHalCoexLmhInit(pCOEX_LMH_CONTEXT pLmhCtxt);
void MacHalCoexMciIntrEnable(uint32_t mask);
void MacHalCoexMciModeCtrl(uint32_t chksum_enable, uint32_t max_retries);
void MacHalCoexMciChksumEn(uint32_t enable);
uint32_t MacHalCoexMciGetChksum(void);
void MacHalCoexMciArbitraryHeader(uint32_t enable);
void MacHalCoexMciWsimMode(uint32_t edge, uint32_t timeout);
void MacHalCoexMciFroceWsiBusSync(uint32_t enable);
int32_t MacHalCoexMciFroceWsiBusSyncStatus(void);
void MacHalCoexMciClockGateDisable(uint32_t disable);
int32_t MacHalCoexMciInit(pCOEX_MCI_CONTEXT pMciCtxt);
uint32_t MacHalGetPendingIntr1RawIntr(void);
uint32_t MacHalGetPendingIntrRawIntr(void);
uint32_t MacHalGetPendingIntrRxMsgIntr(void);
void MacHalClearPendingIntr1RawIntr(uint32_t mask);
void MacHalClearPendingIntrRawIntr(uint32_t mask);
void MacHalClearPendingIntrRxMsgIntr(uint32_t mask);
void MacHalCoexClearSmhRxMciCpuInt(void);
void MacHalCoexClearSmhRxMciResetInt(void);
void MacHalCoexClearSmhRxMciGpmInt(void);
void MacHalCoexClearSmhRxMciGpmFullErrInt(void);
void MacHalCoexClearSmhTxMciGpmDoneInt(void);
void MacHalCoexClearSmhTxMciCpuIntDone(void);
void MacHalCoexClearSmhTxRemoveResetDoneInt(void);
void MacHalCoexClearSmhAhbErrInt(void);
void MacHalCoexClearSmhCcuAccessErInt(void);
void MacHalCoexClearSmhGpmSkipErInt(void);
uint32_t MacHalGetPendingSmhIntr(void);
void MacHalClearPendingSmhIntr(uint32_t mask);
uint32_t MacHalGetPendingMcibIntr(void);
void MacHalClearPendingMcibIntr(uint32_t mask);
void MacHalMcibErrSeverity(uint32_t value);
uint32_t MacHalGetPendingPmhIntr(void);
void MacHalClearPendingPmhIntr(uint32_t mask);
uint32_t MacHalGetPendingLmhIntr(void);
void MacHalClearPendingLmhIntr(uint32_t mask);
uint32_t MacHalGetPendingCRxAllowSetCntr(void);
uint32_t MacHalGetPendingCRxAllowNotSetCntr(void);
uint32_t MacHalGetPendingCRxInRssiThesCntr(void);
uint32_t MacHalGetPendingCRxUpperRssiThesCntr(void);
uint32_t MacHalGetPendingCRxLowerRssiThesCntr(void);
void MacHalClearPendingCRxAllowSetCntr(uint32_t mask);
void MacHalClearPendingCRxAllowNotSetCntr(uint32_t mask);
void MacHalClearPendingCRxInRssiThesCntr(uint32_t mask);
void MacHalClearPendingCRxUpperRssiThesCntr(uint32_t mask);
void MacHalClearPendingCRxLowerRssiThesCntr(uint32_t mask);
uint32_t MacHalGetPendingCRxDebugCntr(void);
void MacHalClearPendingCRxDebugCntr(uint32_t mask);
void MacHalCoexMciBtSimEn(uint32_t enable);
void MacHalCoexMciBtSimV2MsgEn(uint32_t enable);
void MacHalCoexMciBtSimSeq0GenEn(uint32_t enable);
void MacHalCoexMciBtSimSeq1GenEn(uint32_t enable);
void MacHalCoexMciTimeIntervalCfg(pMCI_TIME_INTERVAL pMciTimeInterval);
void MacHalCoexMciBtSimTimers(pCOEX_MCI_CONTEXT pMciCtxt);
void MacHalCoexMciBtSimMsgCtrlConfig(pMCI_BTSIM_MSG_CNTRL pBTSimMsgCtrl);
void MacHalCoexMciBtSimContInfoSeq0Config(pMCI_BTSIM_CONFIG pBTSimCfg);
void MacHalCoexMciBtSimContInfoSeq1Config(pMCI_BTSIM_CONFIG pBTSimCfg);
void MacHalCoexMciBtSimWbtSchdInfoSeq0Config(uint32_t value);
void MacHalCoexMciBtSimWbtSchdInfoSeq1Config(uint32_t value);
void MacHalCoexMciSchdInfoContent1Reg0Cfg(pMCI_BTSIM_CONFIG pBTSimCfg);
void MacHalCoexMciSchdInfoContent2Reg0Cfg(pMCI_BTSIM_CONFIG pBTSimCfg);
void MacHalCoexMciSchdInfoContent1Reg1Cfg(pMCI_BTSIM_CONFIG pBTSimCfg);
void MacHalCoexMciSchdInfoContent2Reg1Cfg(pMCI_BTSIM_CONFIG pBTSimCfg);
void MacHalCoexMciBtSimConfig(pCOEX_MCI_CONTEXT pMciCtxt);
void MacHalCoexArbFreqConfig(void);
uint32_t MacHalCoexGetEachIntrSt(uint32_t case_select);
void MacHalCoexClrEachIntrSt(uint32_t case_select);
void MacHalCoexMciInjectEnableNoBt(void);
void MacHalCoexMciInjectEnable(boolean enable);
void MacHalCoexMciInjectValue(uint32_t value);
uint32_t MacHalCoexMciInjectStatus(void);
void MacHalCoexBmhLinkIdIntrEnable(uint32_t mask);
void MacHalCoexBmhCRxDebugCntrEnable(uint32_t mask);
void MacHalCoexSubModuleIntrEnable(uint32_t enable);
void MacHalCoexCRxEnable(uint32_t enable);
void MacHalCoexForceCRxEnable(uint32_t enable);
void MacHalCoexGainIndexConfig(uint32_t idx);
void MacHalCoexRssiThresConfig(uint32_t upthred, uint32_t lowthred);
void MacHalCoexCRxWaitForLatchDoneFromTimeoutConfig(uint32_t enable, uint32_t value);
void MacHalCoexContInfoRxtoTxOverrideConfig(uint32_t val);
void MacHalCoexSchdInfoRxtoTxOverrideConfig(uint32_t val);
void MacHalCoexCRxArbOverrideConfig(uint32_t val);
void MacHalCoexBmhWlanContMsgEn(uint32_t enable);
void MacHalCoexBmhBtSchdLen(uint32_t value);
void MacHalCoexBmhSetHwSchdTblCtl(uint32_t value);
void MacHalCoexBmhSetHwSchdTblD0(uint32_t value);
void MacHalCoexBmhSetHwSchdTblD1(uint32_t value);
void MacHalCoexBmhSetHwSchdTblD2(uint32_t value);
void MacHalCoexBmhSetHwSchdTblD3(uint32_t value);
uint32_t MacHalCoexBmhGetHwSchdTblCtl(void);
uint32_t MacHalCoexBmhGetHwSchdTblD0(void);
uint32_t MacHalCoexBmhGetHwSchdTblD1(void);
uint32_t MacHalCoexBmhGetHwSchdTblD2(void);
uint32_t MacHalCoexBmhGetHwSchdTblD3(void);
void MacHalCoexDebugConfig(void);
void MacHalCoexBmhCounterEnable(void);
void MacHalCoexBmhModifiedContInfoEn(uint32_t enable);
void MacHalCoexBmhTxRxCntEn(uint32_t value);
void MacHalCoexBmhClearTxRxCnt(uint32_t value);
void MacHalCoexBmhSetTxRxCntLmt(uint32_t value);
void MacHalCoexBmhSetLowPriLmt(uint32_t value);
void MacHalCoexBmhSetHiPriLmt(uint32_t value);
uint32_t MacHalCoexBmhGetBtTxCnt(void);
uint32_t MacHalCoexBmhGetBtRxCnt(void);
void MacHalCoexBmhGetDbgCounter(pCOEX_BMH_CONTEXT pBmhCtxt);
int32_t MacHalCoexWmacInit(uint32_t enable);
int32_t MacHalCoexInit(pWLAN_CXC_CTXT pCoexCtxt);

/* PTA */
#ifdef SUPPORT_PTA_COEX
void MacHalCoexPtaIntrEnable(uint32_t mask);
void MacHalClearPendingPtaIntr(uint32_t mask);
void MacHalCoexPtaEnable(uint32_t pta_en);
void MacHalCoexPtaMode(uint32_t pta_mode);
void MacHalCoexPtaMiscCtrl(pCOEX_PTA_CONTEXT pPtaCtxt);
void MacHalCoexPtaSetTimers(pCOEX_PTA_CONTEXT pPtaCtxt);
void MacHalCoexPtaContInfoTx(pCOEX_PTA_CONTEXT pPtaCtxt);
void MacHalCoexPtaContInfoRx(pCOEX_PTA_CONTEXT pPtaCtxt);
void MacHalCoexPtaBluetoothModeConfig(pCOEX_PTA_BLUETOOTH_MODE pBtMode);
void MacHalCoexPtaBtWeight(uint32_t weight);
uint32_t MacHalGetPendingPtaIntr(void);

void MacHalCoexClearPtaBtActiveRising(void);
void MacHalCoexClearPtaBtActiveFalling(void);
void MacHalCoexClearPtaBtHiPriRising(void);
void MacHalCoexClearPtaBtHiPriFalling(void);
void MacHalCoexClearPtaBtLowPriRising(void);
void MacHalCoexClearPtaBtLowPriFalling(void);
void MacHalCoexClearPtaWlanStomped(void);
void MacHalCoexClearPtaBtStomped(void);
#endif  // SUPPORT_PTA_COEX
#endif  // _HAL_COEX_H_
