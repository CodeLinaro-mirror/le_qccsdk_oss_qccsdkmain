/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/******************************************************************************
 * @file    fdi_rmc.c
 * @brief   Fermion Debug Infra on RAM MIN CODE implementation
 *
 *
 *****************************************************************************/
#include "fdi_rmc.h"

#ifdef FEATURE_FDI

/*----------------------------------------------------------------------------
 * Global Variables
 *----------------------------------------------------------------------------*/
FDI_PS_DATA fdi_node_ins_t g_fdi_rmc_node_ins_list[FDI_RMC_MAX_NODES];
FDI_PS_DATA uint32_t g_ins_index = FDI_RESET;

/*----------------------------------------------------------------------------
 * Extern Variables
 *----------------------------------------------------------------------------*/
extern fdi_interface_t g_ferm_dbg_nodes[];
extern fdi_t g_fdi;

/*----------------------------------------------------------------------------
 * Function Defination
 *----------------------------------------------------------------------------*/
/*****************************************************************
 * @brief FDI node capture from RMC
 *
 * @param evt           Event Enum
 * @param tick_type     Tick Type
 * @param identifier    Node Identifier
 * @return fdi_ret_t
 ****************************************************************/
FDI_PS_TXT fdi_ret_t fdi_rmc_inst(fdi_dbg_node_t evt, tick_type_t tick_type, uint32_t identifier)
{
    FDI_ASSERT_IF_FALSE((g_fdi.module_bmap_en & g_ferm_dbg_nodes[evt]->module_bmap) != FDI_RESET,
                        FDI_RET_FAILED_ASSERT_MOD_NOT_ENABLE);
    FDI_ASSERT_IF_FALSE((g_ferm_dbg_nodes[evt]->attr & FDI_NODE_ATTR_EN_BIT_MASK) != FDI_RESET,
                        FDI_RET_FAILED_ASSERT_NODE_NOT_ENABLE);

    g_fdi_rmc_node_ins_list[g_ins_index].ticks = fdi_get_time_stamp();
    g_fdi_rmc_node_ins_list[g_ins_index].attr =
        (uint16_t)(tick_type << FDI_INS_ATTR_TICK_TYPE_OFFSET) | (evt & FDI_NODE_ATTR_ID_MASK);
    g_fdi_rmc_node_ins_list[g_ins_index].sq_no = g_ins_index;
    g_fdi_rmc_node_ins_list[g_ins_index].identifier = identifier;

    g_ins_index++;
    g_ins_index %= FDI_RMC_MAX_NODES;
    return FDI_RET_SUCCESS;
}

/******************************************************************
 * @brief Reset the g_ins_index
 *
 *****************************************************************/
void fdi_rmc_reset_index(void)
{
    g_ins_index = FDI_RESET;
}

/******************************************************************
 * @brief Get current g_ins_index
 *
 * @return uint32_t
 *****************************************************************/
uint32_t fdi_rmc_get_index(void)
{
    return g_ins_index;
}

#endif /* FEATURE_FDI */
