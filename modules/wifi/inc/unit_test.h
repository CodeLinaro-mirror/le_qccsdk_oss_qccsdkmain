/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*
 *
 * @file unit_test.h
 * @brief Unit Test command params and definitions
 *========================================================================*/

#ifndef UNIT_TEST_H
#define UNIT_TEST_H

//#ifdef SUPPORT_UNIT_TEST_CMD
typedef enum UNIT_TEST_MODULE_ID {
    WLAN_MODULE_CMN = 0,
    WLAN_MODULE_RINGIF = 1,
    WLAN_MODULE_OTP = 2,
    WLAN_MODULE_TIMER = 3,
    WLAN_MODULE_ANI = 4,
    WLAN_MODULE_COEX = 5,
    WLAN_MODULE_TX_COMPLETE = 6,
    WLAN_MODULE_BDF = 7,
    WLAN_MODULE_TWT = 8,
    WLAN_MODULE_CBC = 9,
    WLAN_MODULE_TEMPCOMP = 10,
    WLAN_MODULE_FTM = 11,
    WLAN_MODULE_REGULATORY = 12,
    WLAN_MODULE_FDI = 13,
    WLAN_MODULE_ECSA = 14,
    WLAN_MODULE_SAP_PS = 15,
    WLAN_MODULE_LOGGER = 16,
    WLAN_MODULE_SOCPM = 17,
    WLAN_MODULE_RCLI = 18,
    WLAN_MODULE_RRAM = 19,
    WLAN_MODULE_CONFIG_INI = 20,
    WLAN_MODULE_TPC = 21,
    WLAN_MODULE_DPM = 22,
    WLAN_MODULE_SLP_DEBUG = 23,
    WLAN_MODULE_SW_MD = 24,
    WLAN_MODULE_PERIODIC_WAKE = 25,
    WLAN_MODULE_PHY_RATE_CHNG = 26,
    WLAN_MODULE_EDCA_CONFIG = 27,
    WLAN_MODULE_SLP_CLK_CAL = 28,
    WLAN_MODULE_WPM = 29,
    /* add new module ID here */
} WLAN_MODULE_ID;

typedef enum ut_fdi {
    UT_FDI_START_NODE_NULL = 0,
    UT_FDI_STOP_NODE_NULL = 1,
    UT_FDI_START_NODE_ID = 2,
    UT_FDI_STOP_NODE_ID = 3,
    UT_FDI_EN_MODULE_BMAP = 4,
    UT_FDI_EN_NODE = 5,
    UT_FDI_PRINT_LOGS = 6,
    UT_FDI_POST_PROC_TRIG = 7,
    UT_FDI_SET_WM = 8,
    UT_FPCI_DEBUG_TOGGLE = 9,
} ut_fdi_t;

/*-----------------------------------------------------------------------------
  * test_handler_t:
  *    Function pointer type for functions that handle unit test cases.
  *    These functions are implemented by the respective modules
-----------------------------------------------------------------------------*/
typedef NT_BOOL (*test_handler_t)(WMI_UNIT_TEST_CMD *cmd);

/*-----------------------------------------------------------------------------
  * unit_test_func_tbl_t:
  *    Function pointer table type.
  *    WLAN_MODULE_ID and the corresponding handler function
-----------------------------------------------------------------------------*/
typedef struct unit_test_func_tbl {
    WLAN_MODULE_ID module_id;
    test_handler_t test_handler;
} unit_test_func_tbl_t;

void wmi_unit_test_cmd_handler(WMI_UNIT_TEST_CMD *cmd);
//#endif /* SUPPORT_UNIT_TEST_CMD */
#endif /* UNIT_TEST_H */
