/*========================================================================
Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
*
* @file coex_wlan.h
* @brief Coex wlan params and struct definitions
*========================================================================*/
#ifndef _COEX_TEST_H_
#define _COEX_TEST_H_
/* Standard includes. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "nt_osal.h"

#include "fwconfig_wlan.h"
#include "nt_flags.h"
#include "nt_common.h"
#include "coex_policy.h"

#if defined(SUPPORT_COEX)
/* A wrapper macro to create and send simulated GPM messages */
#define COEX_TEST_SIMULATE_SEND_GPM(name, args)        \
    do {                                               \
        if (TRUE == coex_sim_gpm_create(name, args)) { \
            coex_sim_mci_process_gpm(&g_sim.gpm);      \
        }                                              \
    } while (0);

#define POLICY_PRINT(policy)                                                  \
    do {                                                                      \
        switch (policy) {                                                     \
            case COEX_TRF_MGMT_FREERUN:                                       \
                NT_LOG_COEX_INFO(COEX_TEST, "Policy is: FREE RUN", 0, 0, 0);  \
                break;                                                        \
            case COEX_TRF_MGMT_SHAPE_STATIC_PM:                               \
                NT_LOG_COEX_INFO(COEX_TEST, "Policy is: STATIC PM", 0, 0, 0); \
                break;                                                        \
            default:                                                          \
                NT_LOG_COEX_INFO(COEX_TEST, "Policy is: NONE", 0, 0, 0);      \
                break;                                                        \
        }                                                                     \
    } while (0)

#ifdef PLATFORM_FERMION
enum coex_modules {
    BMH = 0x01,
    LMH = 0x02,
    PMH = 0x04,
    SMH = 0x08,
    LCMH = 0x10,
    MCIM = 0x20,
};

enum cxc_intr {
    BMH_INTR = 0,
    BMH_INTR1,
    BMH_RX_MSG,
    SMH_INTR,
};
#endif

#define COEX_SDM_WK_BEFORE_BCN     0x00000001
#define COEX_SDM_WK_AFTER_BCN      0x00000002
#define COEX_WEIGHT_UPDATE_DISABLE 0x00000004

#define COEX_TEST_PRINT_COEX_SDM_WK_BEFORE_BCN                                                                     \
    WLAN_DBG3_PRINT("Co-Ex B [ Pwrup Ctrl | MCIB Err Evt | Rx Msg Raw ]",                                          \
                    HAL_REG_RD(QWLAN_CXC_PMH_REG_R_CXC_PMH_R0_POWERUP_CTRL_REG),                                   \
                    HAL_REG_RD(QWLAN_CXC_MCIBASIC_REG_R_CXC_MCIBASIC_R1_ERROR_EVENT_REG),                          \
                    HAL_REG_RD(QWLAN_CXC_BMH_REG_R_CXC_BMH_R1_INTERRUPT_RX_MSG_RAW_REG));                          \
    WLAN_DBG2_PRINT("Co-Ex B [ BT Req | BT Req NACK ]", HAL_REG_RD(QWLAN_CXC_BMH_REG_R_CXC_BMH_R1_BT_REQ_CNT_REG), \
                    HAL_REG_RD(QWLAN_CXC_BMH_REG_R_CXC_BMH_R1_BT_REQ_NACK_CNT_REG));

#define COEX_TEST_PRINT_COEX_SDM_WK_AFTER_BCN                                             \
    WLAN_DBG3_PRINT("Co-Ex A [ Pwrup Ctrl | MCIB Err Evt | Rx Msg Raw ]",                 \
                    HAL_REG_RD(QWLAN_CXC_PMH_REG_R_CXC_PMH_R0_POWERUP_CTRL_REG),          \
                    HAL_REG_RD(QWLAN_CXC_MCIBASIC_REG_R_CXC_MCIBASIC_R1_ERROR_EVENT_REG), \
                    HAL_REG_RD(QWLAN_CXC_BMH_REG_R_CXC_BMH_R1_INTERRUPT_RX_MSG_RAW_REG)); \
    WLAN_DBG3_PRINT("Co-Ex A [ BT Req | BT Req NACK | TBTT ]",                            \
                    HAL_REG_RD(QWLAN_CXC_BMH_REG_R_CXC_BMH_R1_BT_REQ_CNT_REG),            \
                    HAL_REG_RD(QWLAN_CXC_BMH_REG_R_CXC_BMH_R1_BT_REQ_NACK_CNT_REG),       \
                    HAL_REG_RD(QWLAN_CXC_BMH_REG_R_CXC_BMH_R1_TBTT_CNT_ARB1_REG));        \
    WLAN_DBG2_PRINT("Co-Ex A [ Bcn Rx | Bcn Miss ]", gpBtCoexUtilDev->sdm_bcn_rx_cnt,     \
                    gpBtCoexUtilDev->sdm_bcn_miss_cnt);

#define COEX_TEST_PRINT(testcase)                      \
    extern BTCOEX_UTIL_STRUCT *gpBtCoexUtilDev;        \
    if (testcase && gpBtCoexUtilDev->coex_test_code) { \
        COEX_TEST_PRINT_##testcase;                    \
    }

enum {
    /* Add enums, for different
    Co-Ex Unit Test here */
    COEX_RX_RATE_MONITOR_SET_THRESHOLD = 0,
    COEX_SET_BMISS_THRESHOLD = 1,
    COEX_GPM_RX = 2,
    COEX_GPM_TX = 3,
    COEX_GET_PROF_INFO = 4,
    COEX_DUMP_BT_LINK_INFO = 5,
    COEX_DUMP_BT_SCAN_INFO = 6,
    COEX_DUMP_BT_LINK = 7,
    COEX_DUMP_ISO_INFO = 8,
    COEX_DUMP_ACL_INTERACTIVE_CFG = 9,
    COEX_DUMP_SCAN_ACTIVITY = 10,
    COEX_GET_BEST_POLICY = 11,
    COEX_POLICY_CHANGE_TEST = 12,
    COEX_SET_BT_WLAN_INTERVALS = 13,
    COEX_FREERUN_POLICY_TEST = 14,
    COEX_SET_WEIGHT = 15,
#ifdef PLATFORM_FERMION
    COEX_USE_BT = 16,
    COEX_SET_CONT_INFO_TIMEOUT = 17,

#ifdef SUPPORT_COEX_SIMULATOR
    COEX_INJECT_MCI_MESSAGE = 18,
#endif

    COEX_WRITE_WEIGHT_REG = 19,
    COEX_RESET_WEIGHT_REG = 20,
    COEX_MAXOUT_WEIGHT_REG = 21,
    COEX_RESET_INTR_STAT_COUNTER = 22,
    COEX_DUMP_INTR_STAT_COUNTER = 23,
    COEX_ENABLE_TX_SOFT_ABORT = 24,
    COEX_DUMP_ARB_DEBUG_COUNTERS = 25,
    COEX_SET_RX_TO_TX_OVERRIDE = 26,

#ifdef SUPPORT_COEX_SIMULATOR
    /* BTSIM Related */
    COEX_ENABLE_DISABLE_BTSIM = 27,
    COEX_SET_BTSIM_TIMERS_SEQ0 = 28,
    COEX_SET_BTSIM_TIMERS_SEQ1 = 29,
    COEX_SET_BTSIM_MSG_CNTRL_SEQ0 = 30,
    COEX_SET_BTSIM_MSG_CNTRL_SEQ1 = 31,
    COEX_CONFIG_AND_ENABLE_BTSIM = 32,
    COEX_ENABLE_DISABLE_BTSIM_SEQ = 33,
    COEX_STRESS_BTSIM_TIMER_START = 34,
    COEX_STRESS_BTSIM_TIMER_STOP = 35,
/* BTSIM Related */
#endif /* SUPPORT_COEX_SIMULATOR */
#endif
    COEX_SCHEDULE = 36,
    COEX_SET_AGGREGATION_LIMIT = 37,
    COEX_PRINT_AGGREGATION_LIMIT = 38,
    COEX_SET_RX_REORDER_TIMEOUT = 39,
    COEX_SET_RX_AGGR_SIZE = 40,
    COEX_PRINT_RX_AGGR_SIZE = 41,
    COEX_DERESTRICT_CCK_RATE = 42,
    COEX_CCK_MIN_RATE = 43,
    COEX_LIMIT_MCS_RATE = 44,
    COEX_SET_AMSDU_SUPPORT_BA = 45,

#ifdef PLATFORM_FERMION
    COEX_GET_TX_QUEUE_PRIO = 46,
    COEX_EN_DIS_CXC_INTRS = 47,
    COEX_EN_DIS_CRX = 48,
    COEX_EN_DIS_FORCE_CRX = 49,
    COEX_SET_BT_RSSI_THRESHOLD = 50,
    COEX_SET_TPE_STADS_LEGACY_MODE = 51,
    COEX_DO_FORCE_WSI_BUS_SYNC = 52,
#endif
    COEX_SIM_SET_BT_STATE = 53,
    COEX_EN_DIS_MOD_LOG = 54,
    COEX_SET_LOG_BITMAP = 55,
    COEX_LOG_CONT_INFO = 56,
    COEX_PRINT_CONT_LOGS = 57,
    COEX_DEVOP_TEST = 58,
    COEX_ENABLE_TEST_CODES = 59,
#if defined(PLATFORM_FERMION) && !defined(EMULATION_BUILD)
    COEX_MCI_RESET = 60,
#endif
};
void coex_unit_test_cmd(uint32_t *args, uint8_t num_args);

#else
#define COEX_TEST_PRINT(testcase)
#endif /* SUPPORT_COEX */
#endif /* #ifndef _COEX_TEST_H_ */
