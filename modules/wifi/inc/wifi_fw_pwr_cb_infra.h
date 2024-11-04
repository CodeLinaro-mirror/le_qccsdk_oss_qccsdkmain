/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/
/*********************************************************************************************
 * @file    wifi_fw_pwr_cb_infra.h
 * @brief   Declarations of Fermion Callback Infra
 *
 *
 ********************************************************************************************/
#ifndef _WIFI_FW_PWR_CB_INFRA_H_
#define _WIFI_FW_PWR_CB_INFRA_H_

#include "fwconfig_cmn.h"
#include "nt_flags.h"

#ifdef FEATURE_FPCI

#define FPCI_MAX_REG                (16)

#include "fdi.h"
#include "fdi_rmc.h"
#include "stdio.h"
#include "stdlib.h"

typedef enum fpci_err
{
    FPCI_SUCCESS,
    FPCI_ERR
}fpci_err_t;

 /* @brief Register Power Event Mask as Enum */
typedef enum pwr_evt
{
    PWR_EVT_WMAC_PRE_SLEEP  = 1 << 1U,
    PWR_EVT_WMAC_POST_SLEEP = 1 << 2U,
    PWR_EVT_WMAC_PRE_AWAKE  = 1 << 3U,
    PWR_EVT_WMAC_POST_AWAKE = 1 << 4U,
    PWR_EVT_WMAC_SLEEP_ABORT= 1 << 5U,
    PWR_EVT_PRE_IMPS_TRIGGER= 1 << 6U,
    PWR_EVT_WMAC_MAX        = 1 << 7U,
} pwr_evt_t;

/* @brief Event Callback Typedef */
typedef void (*ps_evt_cb_t)(uint8_t evt, const void* p_args);

/* @brief Power Event Registration */
typedef struct pwr_evt_reg
{
    uint16_t evt_mask;                  /* OR Masked Event Registration flag */
    uint8_t priority;                   /* Priority level of an evvent reg */
    ps_evt_cb_t evt_cb;                 /* Pointer to event callback */
    void* p_args;                       /* Pointer to data that shall be referenced during the callback */
} pwr_evt_reg_t;

fpci_err_t fpci_evt_cb_reg  (ps_evt_cb_t cb, uint16_t evt_reg_mask, uint8_t priority, void* p_args);
fpci_err_t fpci_evt_cb_dereg(ps_evt_cb_t cb, uint16_t evt_mask);
fpci_err_t fpci_evt_dispatch(pwr_evt_t evt);

#if FPCI_DEBUG == 1
void fpci_register_test_cb(void);
#endif

#endif /* FEATURE_FPCI */
#endif /* _WIFI_FW_PWR_CB_INFRA_H_ */
