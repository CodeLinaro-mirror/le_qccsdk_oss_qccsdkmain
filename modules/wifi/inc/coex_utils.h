/*========================================================================
Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
*
* @file coex_utils.h
* @brief Coex utilities definitions
*========================================================================*/
#ifndef _COEX_UTILS_H_
#define _COEX_UTILS_H_
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
#include "coex_api.h"

#define IS_FLAG_BIT_SET(flag_bit, flag_word) ((flag_word) & (flag_bit))
#define SET_FLAG_BIT(flag_bit, flag_word)    ((flag_word) |= (flag_bit))
#define CLR_FLAG_BIT(flag_bit, flag_word)    ((flag_word) &= ~(flag_bit))
// upper frequency boundary is  10M apart from center frequency
#define COEX_WLAN_BAND_HT40_UPPER_RANGE (10)
// lower frequency boundary is  30M apart from center frequency
#define COEX_WLAN_BAND_HT40_LOWER_RANGE (30)

#define COEX_UNSTREAM_UINT32(pStream)                                              \
    ((uint32_t)((*(uint8_t *)(pStream)) + ((*(((uint8_t *)(pStream)) + 1)) << 8) + \
                ((*(((uint8_t *)(pStream)) + 2)) << 16) + ((*(((uint8_t *)(pStream)) + 3)) << 24)))

#define COEX_UNSTREAM_SINT32(pStream)                                             \
    ((int32_t)((*(uint8_t *)(pStream)) + ((*(((uint8_t *)(pStream)) + 1)) << 8) + \
               ((*(((uint8_t *)(pStream)) + 2)) << 16) + ((*(((uint8_t *)(pStream)) + 3)) << 24)))

/* If the error codes of any module exceeds the max limit of 32, we must increment this index.*/
#define COEX_UTILS_DBG_INFO_INDX 1

#define COEX_MAX_TIME (0xFFFFFFFF)
/* These set of macros and E_COEX_BT_ERR_CODE are part of the debug framework to set error codes at
 * error scenarios and print it at one place, rather than adding print statements on each error location.
 */
#define COEX_BT_SET_ERR(code) \
    gpBtCoexUtilDev->coex_dbg_info[COEX_UTILS_DBG_INFO_SRC_BT]->error[(code) / 32] |= (0x1 << ((code) % 32))
#define COEX_BT_SET_WARN(code) \
    gpBtCoexUtilDev->coex_dbg_info[COEX_UTILS_DBG_INFO_SRC_BT]->warning[(code) / 32] |= (0x1 << ((code) % 32))
#define COEX_BT_GET_ERR_MAP(indx, flush) \
    coex_get_dbg_map(COEX_UTILS_DBG_INFO_SRC_BT, COEX_UTILS_DBG_INFO_TYPE_ERR, indx, flush)
#define COEX_BT_GET_WARN_MAP(indx, flush) \
    coex_get_dbg_map(COEX_UTILS_DBG_INFO_SRC_BT, COEX_UTILS_DBG_INFO_TYPE_WARN, indx, flush)

#define COEX_BYTE0(Value) (Value)
#define COEX_BYTE1(Value) ((Value) << 8)
#define COEX_BYTE2(Value) ((Value) << 16)
#define COEX_BYTE3(Value) ((Value) << 24)

typedef enum coex_utils_dbg_info_src_t {
    COEX_UTILS_DBG_INFO_SRC_MAIN = 0,
    COEX_UTILS_DBG_INFO_SRC_WLAN = 1,
    COEX_UTILS_DBG_INFO_SRC_BT = 2,
    COEX_UTILS_DBG_INFO_SRC_SCHED = 3,
    COEX_UTILS_DBG_INFO_SRC_TWS = 4,
    COEX_UTILS_DBG_INFO_SRC_AAID = 5,
    COEX_UTILS_DBG_INFO_SRC_PROTOCOL = 6,
    COEX_UTILS_DBG_INFO_SRC_ISO = 7,
    COEX_UTILS_DBG_INFO_SRC_NUM
} E_COEX_UTILS_DBG_INFO_SRC;
typedef enum {
    COEX_BT_ERR_PHY_DEV_REGISTER_FAIL = 0,
    COEX_BT_ERR_ZERO_BA_INTRVL = 1,
    COEX_BT_ERR_SAVE_PROF_MAX_LINK = 2,
    COEX_BT_ERR_ENABLE_CCK_FAIL = 3,
    COEX_BT_ERR_STATUS_UPDATE_MAX_LINK = 4,
    COEX_BT_ERR_RSSI_LOWER_THRESH_OOR = 5,
    COEX_BT_ERR_INACTIVITY_INVALID_LINKID = 6,
    COEX_BT_ERR_INACTIVITY_MAX_LINK = 7,
    COEX_BT_ERR_CONN_STAT_MAX_LINK = 8,
    COEX_BT_ERR_UPDATE_CONLESS_MAX_LINK = 9,
    COEX_BT_ERR_SCAN_UPDATE_INVALID_GPM = 10,
    COEX_BT_ERR_CRIT_NONLINK_DUPLICATE = 11,
    COEX_BT_ERR_CRIT_NONLINK_OVERFLOW = 12,
    COEX_BT_ERR_PROC_ADDL_BT_PROF = 13,
    COEX_BT_ERR_PROC_ADV_PROF_INFO_GPM = 14,
    COEX_BT_ERR_PROC_ADV_CONN_STAT_GPM = 15,
    COEX_BT_ERR_PROC_ADV_STAT_UPDATE_GPM = 16,
    COEX_BT_ERR_PROC_BT_QOS_UPDATE = 17,
    COEX_BT_ERR_PHY_ZERO_BA_INTRVL = 18,
    COEX_BT_ERR_GPM_BUF_NULL = 19,
    COEX_BT_ERR_ACLINT_NO_INDX_AVAILABLE = 20,
    COEX_BT_ERR_NO_2G_MAC_UP = 21,
    COEX_BT_ERR_THREE_WAY_COEX_CONFIG = 22,
    COEX_BT_ERR_CAP_GPM = 23,
    COEX_BT_ERR_GPM_ASD_STATUS_SYNC = 24,
    COEX_BT_ERR_GPM_QOS_LE_ISO_INFO = 25,
    COEX_BT_ERR_GPM_ISO_PRIORITY_MASK = 26,
    COEX_BT_ERR_GPM_ISO_GRP = 27,
    COEX_BT_ERR_GPM_ISO_LINK = 28,
    COEX_BT_ERR_NUM
} E_COEX_BT_ERR_CODE;

typedef enum { COEX_BT_WARN_PRIO_TMR_ALREADY_RUNNNING = 0, COEX_BT_WARN_NUM } E_COEX_BT_WARN_CODE;

typedef enum {
    COEX_BT = 0,
    COEX_GPM_MCI = 1,
    COEX_SCHEDULER = 2,
    COEX_DEV_OP = 3,
    COEX_POLICY = 4,
    COEX_WLAN = 5,
    COEX_TEST = 6,
    COEX_WEIGHT = 7,
    COEX_MAIN = 8,
    COEX_POWER = 9,
    COEX_ISO = 10,
} COEX_MOD_ID;

typedef struct coex_utils_dbg_info_t {
    uint32_t error[COEX_UTILS_DBG_INFO_INDX];   /* Stores the error codes as a binary bitmap.*/
    uint32_t warning[COEX_UTILS_DBG_INFO_INDX]; /* Stores the warning codes as a binary bitmap.*/
} coex_utils_dbg_info;

typedef struct btcoex_util_t {
    coex_utils_dbg_info *coex_dbg_info[COEX_UTILS_DBG_INFO_SRC_NUM];
    uint8_t coex_log_en;
    uint16_t dbg_cntrl;
    uint8_t use_real_bt;
    uint8_t cont_logging;
    uint32_t coex_test_code;
    uint16_t sdm_bcn_rx_cnt;
    uint16_t sdm_bcn_miss_cnt;
} BTCOEX_UTIL_STRUCT;

extern BTCOEX_UTIL_STRUCT *gpBtCoexUtilDev;

bool coex_utils_is_coex_needed(uint8_t band);
bool coex_find_wlanstate(uint8_t pdev_id, uint32_t WantedState);
bool coex_find_wlanstate_all_vdev(uint32_t WantedState);
bool coex_is_peer_security_inprogress(uint8_t pdev_id);
bool coex_is_vdev_scanning(uint8_t pdev_id);
bool coex_is_vdev_connecting(uint8_t pdev_id);
bool coex_is_peer_crit_proto_hint(uint8_t pdev_id);
uint32_t coex_utils_packto_uint32(uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3);
void coex_set_aggr_size(uint8_t buffer_size);
void coex_set_amsdu_support_ba(uint8_t enable);
void coex_print_aggr_size();
void coex_get_11bg_peers_num(devh_t *dev, struct coex_phymode_stat *stat);
uint8_t coex_num_of_connected_2g_vdev(void);

#endif /* SUPPORT_COEX */
#endif /* ifndef _COEX_UTILS_H_ */
