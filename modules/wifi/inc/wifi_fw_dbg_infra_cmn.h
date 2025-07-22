/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/******************************************************************************
 * @file    wifi_fw_dbg_infra_common.h
 * @brief   Placeholder for debug nodes enum register
 *
 *
 *****************************************************************************/
#ifndef _WIFI_FW_DEBUG_INFRA_COMMON_H_
#define _WIFI_FW_DEBUG_INFRA_COMMON_H_

#define NODE_LIST_INS(_id, _node_en, _log_lvl, _mod_bmap, _cb)                                   \
    {                                                                                            \
        .id = _id, .p_cb = _cb, .attributes.Enable = _node_en, .attributes.log_level = _log_lvl, \
        .attributes.module_bmap = _mod_bmap                                                      \
    }

/*************************
 * Configs
 **************************/
#define FDI_MAX_NODE (FDI_DBG_MAX)

/** @brief Length of Log FIFO */
#define FDI_MAX_LOG (128)

/** @brief Enable On Target post processing */
#define FDI_EN_POST_PROCESS (0)

/**
 * @brief: Print only on Unit Test Command.
 * @note: 0 For the logs to be printed directly
 */
#define FDI_PRINT_ON_UT (0)

/**
 * @brief Add FDI Modules bitmaps here
 */
#define FDI_MOD_BMAP_PWR    (1 << FDI_MOD_PWR)
#define FDI_MOD_BMAP_COEX   (1 << FDI_MOD_COEX)
#define FDI_MOD_BMAP_HALPHY (1 << FDI_MOD_HALPHY)

/**
 * @brief Add FDI Modules here
 */
typedef enum fdi_mod { FDI_MOD_PWR = 1, FDI_MOD_COEX, FDI_MOD_HALPHY, FDI_MOD_MAX } fdi_mod_t;

/* Debug Enum Define
 * @note This enum is being used to parse in post process script. Please do not change the format of the same.
 */
typedef enum dbg_node {
    /* Power Nodes */
    FDI_DBG_PWR_UNIT_TEST_NODE,
    FDI_DBG_PWR_S2W_WARM_BOOT_CB,
    FDI_DBG_PWR_S2W_WARM_BOOT_CB_WAKE,
    FDI_DBG_PWR_S2W_WARM_BOOT_CB_SLEEP,
    FDI_DBG_PWR_W2S_IDLE_TASK_KICK_IN,
    FDI_DBG_PWR_S2W_IDLE_RESTORE_CONTEXT,
    FDI_DBG_PWR_S2W_SLEEP_TIMER_EXP_ISR,
    FDI_DBG_PWR_S2W_TWT_WAKEUP_PROCESS,
    FDI_DBG_PWR_S2W_WAKE_TRANSITION,
    FDI_DBG_PWR_S2W_TWT_ANN_WAKE_TO_AP,
    FDI_DBG_PWR_S2W_BMPS_ANN_WAKE_TO_AP,
    FDI_DBG_PWR_S2W_WTAP_DPM_START,
    FDI_DBG_PWR_S2W_WTAP_PM_INFRA_STATE_TRN,
    FDI_DBG_PWR_S2W_WTAP_PM_SEND_PSPOLL,
    FDI_DBG_PWR_S2W_EST_MLME_SEND_NULL,
    FDI_DBG_PWR_AUD_PACK_RXTX,
    FDI_DBG_PWR_W2S_SP_END_TIMER_EXP,
    FDI_DBG_PWR_W2S_NT_WLAN_SLEEP,
    FDI_DBG_PWR_W2S_TRANSIT_TO_SLEEP,
    FDI_DBG_PWR_W2S_TTS_STOP_DPM,
    FDI_DBG_PWR_W2S_TTS_HAL_TX_PEND,
    FDI_DBG_PWR_W2S_TTS_STOP_DPM_RX,
    FDI_DBG_PWR_W2S_TTS_DATA_Q_EMP_WAIT,
    FDI_DBG_PWR_W2S_END_STATE_TRANSITION,
    FDI_DBG_PWR_W2S_EST_SET_PM_SLEEP,
    FDI_DBG_PWR_W2S_EST_MLME_GO_TO_SLEEP,
    FDI_DBG_PWR_W2S_EST_MLME_WLAN_SLEEP,
    FDI_DBG_PWR_W2S_EST_MLME_RRI_LFB,
    FDI_DBG_PWR_W2S_EST_MLME_RRI_TTB,
    FDI_DBG_PWR_W2S_EST_MLME_SLP_REG,
    FDI_DBG_PWR_BMPS_WAKEUP,
    FDI_DBG_PWR_BMPS_EXIT_CMDID,
    /* Power Callback Nodes */
    FDI_DBG_PWR_EVT_WMAC_PRE_SLEEP,
    FDI_DBG_PWR_EVT_WMAC_POST_SLEEP,
    FDI_DBG_PWR_EVT_WMAC_PRE_AWAKE,
    FDI_DBG_PWR_EVT_WMAC_POST_AWAKE,
    FDI_DBG_PWR_EVT_WMAC_SLEEP_ABORT,
    FDI_DBG_PWR_EVT_PRE_IMPS_TRIGGER,
    /* Node END */
    FDI_DBG_MAX
} fdi_dbg_node_t;

#endif /* _WIFI_FW_DEBUG_INFRA_COMMON_H_ */
