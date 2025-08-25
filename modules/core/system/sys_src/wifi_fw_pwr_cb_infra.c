/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier:
 * BSD-3-Clause-Clear
*/
/*********************************************************************************************
 * @file    wifi_fw_pwr_cb_infra.c
 * @brief   Definations of Fermion Callback Infra
 *
 *
 ********************************************************************************************/
#include "wifi_fw_pwr_cb_infra.h"
#include "sort.h"
#if FPCI_DEBUG == 1
#include "uart.h"
#endif /* FPCI_DEBUG */

/*************************
 * Defines
 *************************/
#ifdef FEATURE_FDI
#define USE_FEATURE_FDI (1)
#endif

#define FPCI_ASSERT_IF_FALSE FDI_ASSERT_IF_FALSE

#define FPCI_MIN_PRIOR     (0)
#define FPCI_UNUSED_ARG(x) (void)(x)

#define FPCI_CB_DUMMY1_PRIO (3)
#define FPCI_CB_DUMMY2_PRIO (5)
#define FPCI_CB_DUMMY3_PRIO (1)
/*************************
 * Private Struct
 *************************/
/*************************
 * Global Variables
 *************************/
#if FPCI_DEBUG == 1
uint8_t g_fpci_debug_toggle = FDI_RESET;
#endif
/*************************
 * Static Variables
 *************************/
#ifdef FEATURE_FPCI

static FDI_PS_DATA pwr_evt_reg_t g_event_bin[FPCI_MAX_REG];
static FDI_PS_DATA size_t g_event_bin_ctr = FDI_RESET;
SORT_INSTANCE_STRUCT(FPCI, pwr_evt_reg_t, g_event_bin, priority);

/*************************
 * Static Function Declare
 *************************/
#if FPCI_DEBUG == 1
static void ps_evt_cb_dummy1_t(uint8_t evt, const void *p_args);
static void ps_evt_cb_dummy2_t(uint8_t evt, const void *p_args);
static void ps_evt_cb_dummy3_t(uint8_t evt, const void *p_args);
#endif /* FPCI_DEBUG */
/*******************************
 * Extern Function Declaration
 *******************************/
extern uint8_t get_warmboot_status(void);
/*************************
 * Function Defination
 *************************/
/********************************************************************************************
 * @brief  To register a callback related to events and reorder/sort callback despatch list
 * @param cb                Pointer to callback registration
 * @param evt_reg_mask      Event mask flag
 * @param priority          Priority of the callback; Ranged 1-255
 * @return FPCI_SUCCESS, FPCI_ERR
 ********************************************************************************************/

fpci_err_t fpci_evt_cb_reg(ps_evt_cb_t cb, uint16_t evt_reg_mask, uint8_t priority, void *p_args)
{
    FPCI_ASSERT_IF_FALSE(cb != NULL, FPCI_ERR);
    FPCI_ASSERT_IF_FALSE(evt_reg_mask < PWR_EVT_WMAC_MAX, FPCI_ERR);
    FPCI_ASSERT_IF_FALSE(g_event_bin_ctr < FPCI_MAX_REG - 1, FPCI_ERR);
    FPCI_ASSERT_IF_FALSE(priority > FPCI_MIN_PRIOR, FPCI_ERR);

    size_t index = g_event_bin_ctr;

    /* Check if the cb is already registered */
    for (size_t itter = FDI_RESET; itter < g_event_bin_ctr; itter++) {
        if (g_event_bin[itter].evt_cb == cb) {
            index = itter;
            break;
        }
    }

    g_event_bin[index].evt_cb = cb;
    g_event_bin[index].evt_mask |= evt_reg_mask; /* Bitwise OR with the existing cb event mask */
    g_event_bin[index].priority = priority;
    g_event_bin[index].p_args = p_args;

    if (index == g_event_bin_ctr) {
        g_event_bin_ctr++;
    }

    /* Sort by priority */
    sort_bubble(&GET_SORT_INSTANCE(FPCI), SORT_DIRECTION_DESSENDING);

    return FPCI_SUCCESS;
}

/*********************************************************************************************
 * @brief To deregister a particular callback associated to the event mask
 * @param cb               Callback to be deregistered
 * @param evt_mask         Event masks to be de-registered
 * @return FPCI_SUCCESS, FPCI_ERR
 ********************************************************************************************/
fpci_err_t fpci_evt_cb_dereg(ps_evt_cb_t cb, uint16_t evt_reg_mask)
{
    FPCI_ASSERT_IF_FALSE(cb != NULL, FPCI_ERR);
    FPCI_ASSERT_IF_FALSE(evt_reg_mask < PWR_EVT_WMAC_MAX, FPCI_ERR);

    size_t itter_index;

    for (itter_index = FDI_RESET; itter_index < g_event_bin_ctr; itter_index++) {
        if ((g_event_bin[itter_index].evt_cb) == cb && (g_event_bin[itter_index].evt_mask & evt_reg_mask)) {
            g_event_bin[itter_index].evt_mask &= ~evt_reg_mask;
            break;
        }
    }
    if (g_event_bin[itter_index].evt_mask == FDI_RESET) {
        /* Remove the callback from reg list */
        memset(&(g_event_bin[itter_index]), FDI_RESET, sizeof(pwr_evt_reg_t));
        /* Sort by priority */
        sort_bubble(&GET_SORT_INSTANCE(FPCI), SORT_DIRECTION_DESSENDING);
        g_event_bin_ctr--;
    }

    return FPCI_SUCCESS;
}
/*********************************************************************************************
 * @brief To despactch the callbacks as per event
 * @note  Not to be used by the user other than power infra.
 * @param evt         Event macro as per pwr_evt_t
 * @return FPCI_SUCCESS, FPCI_ERR
 ********************************************************************************************/
fpci_err_t FDI_PS_TXT fpci_evt_dispatch(pwr_evt_t evt)
{
    FPCI_ASSERT_IF_FALSE(evt < PWR_EVT_WMAC_MAX, FPCI_ERR);
#if USE_FEATURE_FDI
    fdi_dbg_node_t dbg_node;
    if (evt == PWR_EVT_WMAC_PRE_SLEEP)
        dbg_node = FDI_DBG_PWR_EVT_WMAC_PRE_SLEEP;
    else if (evt == PWR_EVT_WMAC_POST_AWAKE)
        dbg_node = FDI_DBG_PWR_EVT_WMAC_POST_AWAKE;
    else if (evt == PWR_EVT_WMAC_POST_SLEEP)
        dbg_node = FDI_DBG_PWR_EVT_WMAC_POST_SLEEP;
    else if (evt == PWR_EVT_WMAC_PRE_AWAKE)
        dbg_node = FDI_DBG_PWR_EVT_WMAC_PRE_AWAKE;
    else if (evt == PWR_EVT_WMAC_SLEEP_ABORT)
        dbg_node = FDI_DBG_PWR_EVT_WMAC_SLEEP_ABORT;
    else
        dbg_node = FDI_DBG_PWR_EVT_PRE_IMPS_TRIGGER;
#endif /* USE_FEATURE_FDI */

    for (size_t itter_index = FDI_RESET; itter_index < g_event_bin_ctr; itter_index++) {
        if ((g_event_bin[itter_index].evt_mask & evt) && (g_event_bin[itter_index].evt_cb != NULL)) {
#if FPCI_DEBUG
            if (((g_event_bin[itter_index].evt_cb == &ps_evt_cb_dummy1_t) ||
                 (g_event_bin[itter_index].evt_cb == &ps_evt_cb_dummy2_t) ||
                 (g_event_bin[itter_index].evt_cb == &ps_evt_cb_dummy3_t)) &&
                g_fpci_debug_toggle) {
                g_event_bin[itter_index].evt_cb((uint8_t)evt, g_event_bin[itter_index].p_args);
            } else
#endif
            {
#if USE_FEATURE_FDI
                FDI_NODE_START_ID(dbg_node, (uint32_t)g_event_bin[itter_index].evt_cb);
#endif /* USE_FEATURE_FDI */
                g_event_bin[itter_index].evt_cb((uint8_t)evt, g_event_bin[itter_index].p_args);
#if USE_FEATURE_FDI
                FDI_NODE_STOP_ID(dbg_node, (uint32_t)g_event_bin[itter_index].evt_cb);
#endif /* USE_FEATURE_FDI */
            }
        } else {
            continue;
        }
    }
    return FPCI_SUCCESS;
}

#if FPCI_DEBUG
/******************************************************
 * @brief Unit Test CB registration
 *
 *****************************************************/
void fpci_register_test_cb(void)
{
    fpci_evt_cb_reg(&ps_evt_cb_dummy1_t,
                    PWR_EVT_WMAC_PRE_SLEEP | PWR_EVT_WMAC_POST_SLEEP | PWR_EVT_WMAC_PRE_AWAKE |
                        PWR_EVT_WMAC_POST_AWAKE | PWR_EVT_WMAC_SLEEP_ABORT,
                    FPCI_CB_DUMMY1_PRIO, NULL);
    fpci_evt_cb_reg(&ps_evt_cb_dummy2_t,
                    PWR_EVT_WMAC_PRE_SLEEP | PWR_EVT_WMAC_POST_SLEEP | PWR_EVT_WMAC_PRE_AWAKE |
                        PWR_EVT_WMAC_POST_AWAKE | PWR_EVT_WMAC_SLEEP_ABORT,
                    FPCI_CB_DUMMY2_PRIO, NULL);
    fpci_evt_cb_reg(&ps_evt_cb_dummy3_t,
                    PWR_EVT_WMAC_PRE_SLEEP | PWR_EVT_WMAC_POST_SLEEP | PWR_EVT_WMAC_PRE_AWAKE |
                        PWR_EVT_WMAC_POST_AWAKE | PWR_EVT_WMAC_SLEEP_ABORT,
                    FPCI_CB_DUMMY3_PRIO, NULL);
}
/*This is for testing FPCI only*/
FDI_PS_TXT static void ps_evt_cb_dummy1_t(uint8_t evt, const void *p_args)
{
    FPCI_UNUSED_ARG(evt);
    FPCI_UNUSED_ARG(p_args);
    UART_Send("A", 1);
}
FDI_PS_TXT static void ps_evt_cb_dummy2_t(uint8_t evt, const void *p_args)
{
    FPCI_UNUSED_ARG(evt);
    FPCI_UNUSED_ARG(p_args);
    UART_Send("B", 1);
}
FDI_PS_TXT static void ps_evt_cb_dummy3_t(uint8_t evt, const void *p_args)
{
    FPCI_UNUSED_ARG(evt);
    FPCI_UNUSED_ARG(p_args);
    UART_Send("C", 1);
}
#endif /* FPCI_DEBUG */
#endif /* FEATURE_FPCI */
