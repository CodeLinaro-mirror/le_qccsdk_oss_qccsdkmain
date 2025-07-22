/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*------------------------------------------------------------------------
 * @file    wifi_fw_dbg_nodes.c
 * @brief   Power Debug Infrastructure related code
 *
 *
 *------------------------------------------------------------------------*/
#include "fdi.h"

#ifdef FEATURE_FDI
/*--------------------------------------------------------------------------
 * Callback Declare
 --------------------------------------------------------------------------*/
static void pow_post_ins_cb(fdi_node_ins_t *p_fdi_ins);

/*--------------------------------------------------------------------------
 * Global Variables
 --------------------------------------------------------------------------*/
fdi_interface_t g_ferm_dbg_nodes[FDI_DBG_MAX];

/****************************************************************
 * @brief Node registration insertion list
 *
 * @note: To insert node NODE_LIST_INS(Node_id_enum, node_enable,
 *                                     DEBUG/PROD, module_bitmap,
 *                                     post_insertion callback)
 *****************************************************************/
static FDI_PS_DATA const fdi_nodes_t node_list[] = {
    /* Power Nodes */
    NODE_LIST_INS(FDI_DBG_PWR_UNIT_TEST_NODE, FDI_SET, LOG_LEVEL_DEBUG, FDI_MOD_BMAP_PWR, pow_post_ins_cb),
    NODE_LIST_INS(FDI_DBG_PWR_S2W_WARM_BOOT_CB, FDI_RESET, LOG_LEVEL_DEBUG, FDI_MOD_BMAP_PWR, NULL),
    NODE_LIST_INS(FDI_DBG_PWR_S2W_WARM_BOOT_CB_WAKE, FDI_SET, LOG_LEVEL_DEBUG, FDI_MOD_BMAP_PWR, NULL),
    NODE_LIST_INS(FDI_DBG_PWR_S2W_WARM_BOOT_CB_SLEEP, FDI_RESET, LOG_LEVEL_DEBUG, FDI_MOD_BMAP_PWR, NULL),
    NODE_LIST_INS(FDI_DBG_PWR_S2W_IDLE_RESTORE_CONTEXT, FDI_RESET, LOG_LEVEL_DEBUG, FDI_MOD_BMAP_PWR, pow_post_ins_cb),
    NODE_LIST_INS(FDI_DBG_PWR_S2W_SLEEP_TIMER_EXP_ISR, FDI_RESET, LOG_LEVEL_DEBUG, FDI_MOD_BMAP_PWR, pow_post_ins_cb),
    NODE_LIST_INS(FDI_DBG_PWR_S2W_TWT_WAKEUP_PROCESS, FDI_RESET, LOG_LEVEL_DEBUG, FDI_MOD_BMAP_PWR, pow_post_ins_cb),
    NODE_LIST_INS(FDI_DBG_PWR_S2W_WAKE_TRANSITION, FDI_SET, LOG_LEVEL_DEBUG, FDI_MOD_BMAP_PWR, pow_post_ins_cb),
    NODE_LIST_INS(FDI_DBG_PWR_S2W_BMPS_ANN_WAKE_TO_AP, FDI_RESET, LOG_LEVEL_DEBUG, FDI_MOD_BMAP_PWR, pow_post_ins_cb),
    NODE_LIST_INS(FDI_DBG_PWR_S2W_TWT_ANN_WAKE_TO_AP, FDI_RESET, LOG_LEVEL_DEBUG, FDI_MOD_BMAP_PWR, pow_post_ins_cb),
    NODE_LIST_INS(FDI_DBG_PWR_S2W_WTAP_DPM_START, FDI_SET, LOG_LEVEL_DEBUG, FDI_MOD_BMAP_PWR, pow_post_ins_cb),
    NODE_LIST_INS(FDI_DBG_PWR_S2W_WTAP_PM_INFRA_STATE_TRN, FDI_SET, LOG_LEVEL_DEBUG, FDI_MOD_BMAP_PWR, pow_post_ins_cb),
    NODE_LIST_INS(FDI_DBG_PWR_S2W_WTAP_PM_SEND_PSPOLL, FDI_SET, LOG_LEVEL_DEBUG, FDI_MOD_BMAP_PWR, pow_post_ins_cb),
    NODE_LIST_INS(FDI_DBG_PWR_AUD_PACK_RXTX, FDI_SET, LOG_LEVEL_DEBUG, FDI_MOD_BMAP_PWR, pow_post_ins_cb),
    NODE_LIST_INS(FDI_DBG_PWR_W2S_SP_END_TIMER_EXP, FDI_SET, LOG_LEVEL_DEBUG, FDI_MOD_BMAP_PWR, pow_post_ins_cb),
    NODE_LIST_INS(FDI_DBG_PWR_W2S_NT_WLAN_SLEEP, FDI_SET, LOG_LEVEL_DEBUG, FDI_MOD_BMAP_PWR, pow_post_ins_cb),
    NODE_LIST_INS(FDI_DBG_PWR_W2S_TRANSIT_TO_SLEEP, FDI_SET, LOG_LEVEL_DEBUG, FDI_MOD_BMAP_PWR, pow_post_ins_cb),
    NODE_LIST_INS(FDI_DBG_PWR_W2S_TTS_STOP_DPM, FDI_SET, LOG_LEVEL_DEBUG, FDI_MOD_BMAP_PWR, pow_post_ins_cb),
    NODE_LIST_INS(FDI_DBG_PWR_W2S_TTS_HAL_TX_PEND, FDI_SET, LOG_LEVEL_DEBUG, FDI_MOD_BMAP_PWR, pow_post_ins_cb),
    NODE_LIST_INS(FDI_DBG_PWR_W2S_TTS_STOP_DPM_RX, FDI_SET, LOG_LEVEL_DEBUG, FDI_MOD_BMAP_PWR, pow_post_ins_cb),
    NODE_LIST_INS(FDI_DBG_PWR_W2S_TTS_DATA_Q_EMP_WAIT, FDI_SET, LOG_LEVEL_DEBUG, FDI_MOD_BMAP_PWR, pow_post_ins_cb),
    NODE_LIST_INS(FDI_DBG_PWR_W2S_END_STATE_TRANSITION, FDI_SET, LOG_LEVEL_DEBUG, FDI_MOD_BMAP_PWR, pow_post_ins_cb),
    NODE_LIST_INS(FDI_DBG_PWR_W2S_EST_SET_PM_SLEEP, FDI_SET, LOG_LEVEL_DEBUG, FDI_MOD_BMAP_PWR, pow_post_ins_cb),
    NODE_LIST_INS(FDI_DBG_PWR_W2S_EST_MLME_GO_TO_SLEEP, FDI_SET, LOG_LEVEL_DEBUG, FDI_MOD_BMAP_PWR, pow_post_ins_cb),
    NODE_LIST_INS(FDI_DBG_PWR_S2W_EST_MLME_SEND_NULL, FDI_SET, LOG_LEVEL_DEBUG, FDI_MOD_BMAP_PWR, pow_post_ins_cb),
    NODE_LIST_INS(FDI_DBG_PWR_W2S_EST_MLME_WLAN_SLEEP, FDI_SET, LOG_LEVEL_DEBUG, FDI_MOD_BMAP_PWR, pow_post_ins_cb),
    NODE_LIST_INS(FDI_DBG_PWR_W2S_EST_MLME_RRI_LFB, FDI_SET, LOG_LEVEL_DEBUG, FDI_MOD_BMAP_PWR, pow_post_ins_cb),
    NODE_LIST_INS(FDI_DBG_PWR_W2S_EST_MLME_RRI_TTB, FDI_SET, LOG_LEVEL_DEBUG, FDI_MOD_BMAP_PWR, pow_post_ins_cb),
    NODE_LIST_INS(FDI_DBG_PWR_W2S_EST_MLME_SLP_REG, FDI_SET, LOG_LEVEL_DEBUG, FDI_MOD_BMAP_PWR, pow_post_ins_cb),
    NODE_LIST_INS(FDI_DBG_PWR_W2S_IDLE_TASK_KICK_IN, FDI_RESET, LOG_LEVEL_DEBUG, FDI_MOD_BMAP_PWR, pow_post_ins_cb),
    NODE_LIST_INS(FDI_DBG_PWR_BMPS_WAKEUP, FDI_SET, LOG_LEVEL_DEBUG, FDI_MOD_BMAP_PWR, pow_post_ins_cb),
    NODE_LIST_INS(FDI_DBG_PWR_BMPS_EXIT_CMDID, FDI_SET, LOG_LEVEL_DEBUG, FDI_MOD_BMAP_PWR, pow_post_ins_cb),
    /* Power Callback Nodes */
    NODE_LIST_INS(FDI_DBG_PWR_EVT_WMAC_PRE_SLEEP, FDI_SET, LOG_LEVEL_DEBUG, FDI_MOD_BMAP_PWR, pow_post_ins_cb),
    NODE_LIST_INS(FDI_DBG_PWR_EVT_WMAC_POST_SLEEP, FDI_SET, LOG_LEVEL_DEBUG, FDI_MOD_BMAP_PWR, pow_post_ins_cb),
    NODE_LIST_INS(FDI_DBG_PWR_EVT_WMAC_PRE_AWAKE, FDI_SET, LOG_LEVEL_DEBUG, FDI_MOD_BMAP_PWR, pow_post_ins_cb),
    NODE_LIST_INS(FDI_DBG_PWR_EVT_WMAC_POST_AWAKE, FDI_SET, LOG_LEVEL_DEBUG, FDI_MOD_BMAP_PWR, pow_post_ins_cb),
    NODE_LIST_INS(FDI_DBG_PWR_EVT_WMAC_SLEEP_ABORT, FDI_SET, LOG_LEVEL_DEBUG, FDI_MOD_BMAP_PWR, pow_post_ins_cb),
};

static size_t len_node_list = sizeof(node_list) / sizeof(fdi_nodes_t);
/*--------------------------------------------------------------------------
 * Function Definations
 --------------------------------------------------------------------------*/
/******************************************************************
 * @brief Register all Debug Nodes in node list
 *
 *****************************************************************/
void fdi_reg_all_nodes(void)
{
    fdi_ret_t fdi_ret;
    for (size_t id = FDI_RESET; id < len_node_list; id++) {
        fdi_ret = fdi_reg_node(node_list[id].id, node_list[id].attributes, node_list[id].p_cb,
                               &(g_ferm_dbg_nodes[node_list[id].id]));
        if (fdi_ret != FDI_RET_SUCCESS) {
            NT_LOG_PRINT(COMMON, ERR, "ERR: %u", (unsigned int)fdi_ret);
        }
    }
}

/*--------------------------------------------------------------------------
 * Function Definations
 --------------------------------------------------------------------------*/
/******************************************************************
 * @brief Power post insertion callback
 *
 * @param p_fdi_ins
 *****************************************************************/
static void pow_post_ins_cb(fdi_node_ins_t *p_fdi_ins)
{
    if (((p_fdi_ins->attr >> FDI_INS_ATTR_TICK_TYPE_OFFSET) & 0x01) == TICK_TYPE_STOP_TICK) {
        switch (p_fdi_ins->attr & FDI_NODE_ATTR_ID_MASK) {
            case FDI_DBG_PWR_UNIT_TEST_NODE:
#if FDI_DBG == FDI_SET
                NT_LOG_PRINT(COMMON, ERR, "FDI_DBG_PWR_UNIT_TEST_NODE inserted!");
#endif
                break;
            case FDI_DBG_PWR_S2W_IDLE_RESTORE_CONTEXT:
                fdi_node_en(FDI_DBG_PWR_S2W_IDLE_RESTORE_CONTEXT, FDI_RESET);
                break;
            case FDI_DBG_PWR_W2S_END_STATE_TRANSITION:
                fdi_node_en(FDI_DBG_PWR_W2S_IDLE_TASK_KICK_IN, FDI_SET);
                break;
            case FDI_DBG_PWR_W2S_IDLE_TASK_KICK_IN:
                fdi_node_en(FDI_DBG_PWR_W2S_IDLE_TASK_KICK_IN, FDI_RESET);
                fdi_node_en(FDI_DBG_PWR_S2W_IDLE_RESTORE_CONTEXT, FDI_SET);
                break;
            case FDI_DBG_PWR_W2S_EST_MLME_WLAN_SLEEP:
                fdi_node_en(FDI_DBG_PWR_W2S_EST_MLME_SLP_REG, FDI_SET);
                break;
            case FDI_DBG_PWR_W2S_EST_MLME_SLP_REG:
                fdi_node_en(FDI_DBG_PWR_W2S_EST_MLME_SLP_REG, FDI_RESET);
                break;
            default:
                break;
        }
    }
}

#endif /* FEATURE_FDI */
