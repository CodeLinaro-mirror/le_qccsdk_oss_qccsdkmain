/*========================================================================
Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
*
* @file coex_api.h
* @brief Coex API definitions
*========================================================================*/
#ifndef COEX_API_H
#define COEX_API_H

/*-------------------------------------------------------------------------
 * Include Files
 * ----------------------------------------------------------------------*/
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "nt_osal.h"

#include "fwconfig_wlan.h"
#include "nt_flags.h"
#include "nt_common.h"
#ifdef SUPPORT_COEX
#include "wlan_dev.h"

/*-------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * ----------------------------------------------------------------------*/

#define COEX_BT_PASS_SCAN_BMAP_SHIFT (0)
#define COEX_BT_ACT_SCAN_BMAP_SHIFT  (16)

// Wlan system state fields
#define COEX_WLAN_SYS_IDLE            (0x00000000)
#define COEX_WLAN_SYS_CONNECTING      (0x00000001)
#define COEX_WLAN_SYS_CONNECTED       (0x00000002)
#define COEX_WLAN_SYS_SCANNING        (0x00000004)
#define COEX_WLAN_SYS_UP              (0x00000008)
#define COEX_WLAN_SYS_PAUSED          (0x00000010)
#define COEX_WLAN_SYS_SECURITY        (0x00000020)
#define COEX_WLAN_SYS_ALL_PAUSED      (0x00000040)
#define COEX_WLAN_SYS_CRIT_PROTO_HINT (0x00000080)
#define COEX_WLAN_SYS_NAN_DISCOVERING (0x00000100)
// FlowCtrlIDs: flow control frames
#define FLOW_CTRL_NULL_FRM (0x0000001)
#define FLOW_CTRL_PSP      (0x0000002)
#define FLOW_CTRL_CTS2S    (0x0000004)

#define COEX_SCHED_POSTPAUSE_EXPIRED             (0x0000001)
#define COEX_SCHED_CTS2S_PENDING                 (0x0000002)
#define COEX_SCHED_PSPOLL_VDEV_SWITCHING         (0x0000008)
#define COEX_SCHED_LOAD_BALANCING_REQUIRED       (0x0000010)
#define COEX_SCHED_OCS_EXIT_PENDING              (0x0000020)
#define COEX_SCHED_CRIT_SCAN_MGMT_FRM            (0x0000040)
#define COEX_SCHED_CRIT_BCN_RX                   (0x0000080)
#define COEX_SCHED_AP_PEER_MAX_INACTIVITY        (0x0000100)
#define COEX_SCHED_IBSS_IN_ATIM_WIN              (0x0000200)
#define COEX_SCHED_CRIT_ASSOC                    (0x0000400)
#define COEX_SCHED_DISABLE_AGGR                  (0x0000800)
#define COEX_SCHED_PRIO_WLAN_CONN                (0x0001000)
#define COEX_SCHED_PAUSE_TDLS                    (0x0002000)
#define COEX_SCHED_SUSPEND_TDLS_OFFCHAN          (0x0004000)
#define COEX_SCHED_INCREASE_HID_PRIORITY_FREERUN (0x0008000)
#define COEX_SCHED_PSPOLL_EXIT_IN_PROGRESS       (0x0010000)
#define COEX_SCHED_INCREASE_RX_LVL1              (0x0020000)
#define COEX_SCHED_MODIFY_AGGRLIMIT_FREERUN      (0x0040000)

/*-------------------------------------------------------------------------
 * Type Declarations
 * ----------------------------------------------------------------------*/
#if defined(PLATFORM_NT)
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
#endif

typedef enum {
    COEX_STATS_QRY_SRC_BTCOEX_FULL_SLEEP_WAKING = 0,
    COEX_STATS_QRY_SRC_BTCOEX_MCI_TIMER_HANDLER = 1,
    COEX_STATS_QRY_SRC_BTCOEX_MCI_RECOVERY = 2,
    COEX_STATS_QRY_SRC_BTCOEX_PROCESS_BT_QUEUE_OVERFLOW = 3,
    COEX_STATS_QRY_SRC_PROCESS_BAND_CHANGE = 4,
    COEX_STATS_QRY_SRC_POWER_CHANGE_CB = 5,
    COEX_STATS_QRY_SRC_INIT_WLAN_BANDS = 6,
    COEX_STATS_QRY_SRC_MCI_RESET_SEQUENCE_COMPLETE = 7,
    COEX_STATS_QRY_SRC_MCI_SEND_PENDING_MSGS = 8,
    COEX_STATS_QRY_SRC_MCI_PMH_POWERUP_WLAN = 9,
    COEX_STATS_QRY_SRC_BTCOEX_CHANGE_ALGO = 10,
    COEX_STATS_QRY_SRC_PROCESS_GPM_FULL_INT = 11,
} E_STATUS_QUERY_SOURCE;

struct coex_phymode_stat {
    uint16_t numOf11bPeers;
    uint16_t numOf11gPeers;
};

typedef struct coex_vdev_notif_t {
    channel_t *pChan;
    uint8_t OpMode;
} COEX_VDEV_NOTIF;

enum {
    COEX_WLAN_DEV_START = 1,
    COEX_WLAN_DEV_SCAN_STARTED = 2,
    COEX_WLAN_DEV_SCAN_END = 3,
    COEX_WLAN_DEV_STOP = 4,
    COEX_WLAN_DEV_SCAN_FOREIGN_CHANNEL = 5,
    COEX_WLAN_DEV_SCAN_FOREIGN_CHANNEL_EXIT = 6,
    COEX_WLAN_DEV_DOWN = 7,
    COEX_WLAN_DEV_PEER_CONNECTED = 8,
    COEX_WLAN_DEV_PEER_CRIT_PROTO_HINT = 9,
    COEX_WLAN_DEV_PAUSE = 10,
    COEX_WLAN_DEV_UNPAUSE = 11,
    COEX_WLAN_DEV_CONNECTED = 12,
    COEX_WLAN_DEV_UP = 13,
    COEX_WLAN_DEV_PEER_DELETE = 14,
    COEX_WLAN_DEV_TWT_SETUP = 15,
    COEX_WLAN_DEV_TWT_TEARDOWN = 16,
};

/* This enum designates the meaning of the tx priorty indices. eg: priority index of 4 is PS-POLL */
typedef enum COEX_PRIO_FRM_TYPE {
    COEX_PRIO_FRM_TYPE_MGMT_BCN = 0,   // Beacons
    COEX_PRIO_FRM_TYPE_MGMT_CONN = 1,  // Connection frames (Req/Resp)-  Assoc, Reassoc, Dissassoc, Auth, Deauth
    COEX_PRIO_FRM_TYPE_MGMT_SCAN = 2,  // Probe Req/Resp.
    COEX_PRIO_FRM_TYPE_MGMT_IBSS = 3,  // ATIM, Action
    COEX_PRIO_FRM_TYPE_CTRL_PSP = 4,   // PS-POLL
    COEX_PRIO_FRM_TYPE_CTRL_BAR = 5,   // Block ACK request.
    COEX_PRIO_FRM_TYPE_CTRL_CTS = 6,   // CTS2S frames.
    COEX_PRIO_FRM_TYPE_CTRL_RSVD = 7,  // Reserved.
    COEX_PRIO_FRM_TYPE_DATA_NULL = 8,  // Data NULL packets (including QoS NULL).
    COEX_PRIO_FRM_TYPE_DATA_DATA = 9,  // Data packets.
    COEX_PRIO_FRM_TYPE_DATA_QOS = 10,  // Data QoS frames - VO and Vi frames only.
    COEX_PRIO_FRM_TYPE_DATA_VBC = 11,  // XPAN VBC
    COEX_PRIO_FRM_TYPE_OTH_MGMT = 12,  // Other management frames.
    COEX_PRIO_FRM_TYPE_OTH_CTRL = 13,  // Other control frames.
    COEX_PRIO_FRM_TYPE_OTH_DATA = 14,  // Other data frames.
    COEX_PRIO_FRM_TYPE_OTH_OTH = 15,   // Any other frames that doesnt fit in the above conditions.
    COEX_PRIO_FRM_TYPE_INVALID = 16,   // Invalid type.
} E_COEX_PRIO_FRM_TYPE;

// Earbud role
typedef enum {
    COEX_BT_EARBUD_PRIMARY = 0,
    COEX_BT_EARBUD_SECONDARY = 1,
} E_COEX_BT_EARBUD_ROLE;

// XPAN Connection state
typedef enum {
    XPAN_CONNECTION_PENDING = 0,
    XPAN_CONNECTION_PRESENT = 1,
    XPAN_CONNECTION_ESTABLISHED = 2,
    XPAN_CONNECTION_DISCONNECTED = 3,
    /* XPAN_LE_TRANSITION is sent by BT when CIS connection is established with HS*/
    XPAN_LE_TRANSITION = 4,
} E_COEX_XPAN_CONN_STATE;

// BT link focus
typedef enum {
    COEX_BT_LINK_NOT_IN_FOCUS = 0,
    COEX_BT_LINK_IN_FOCUS = 1,
} E_COEX_BT_LINK_FOCUS;
/*-------------------------------------------------------------------------
 * Function Declarations
 * ----------------------------------------------------------------------*/
void coex_init(void);
void coex_update_wlanstate(devh_t *dev, uint32_t Stimulus, void *pInput1, void *pInput2);

#endif /* SUPPORT_COEX */
#endif /* define COEX_API_H */
