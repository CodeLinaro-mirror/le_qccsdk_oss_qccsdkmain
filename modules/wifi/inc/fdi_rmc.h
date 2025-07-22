/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/******************************************************************************
 * @file    fdi_rmc.h
 * @brief   Interface for using Fermion Debug Infra on RAM MIN CODE
 *
 *
 *****************************************************************************/
#ifndef _FDI_RMC_H_
#define _FDI_RMC_H_

#include "nt_flags.h"
#include "fdi.h"

#define FDI_RMC_MAX_NODES (512)

#ifdef FEATURE_FDI
#define FEATURE_FDI_RMC
#endif

#if defined(FEATURE_FDI_RMC)
#define FDI_RMC_INS_START_NULL(_evt)            fdi_rmc_inst(_evt, TICK_TYPE_START_TICK, (uint32_t)NULL)
#define FDI_RMC_INS_STOP_NULL(_evt)             fdi_rmc_inst(_evt, TICK_TYPE_STOP_TICK, (uint32_t)NULL)
#define FDI_RMC_INS_START_ID(_evt, _identifier) fdi_rmc_inst(_evt, TICK_TYPE_START_TICK, (uint32_t)_identifier)
#define FDI_RMC_INS_STOP_ID(_evt, _identifier)  fdi_rmc_inst(_evt, TICK_TYPE_STOP_TICK, (uint32_t)_identifier)

extern fdi_node_ins_t g_fdi_rmc_node_ins_list[FDI_RMC_MAX_NODES];
extern uint32_t g_ins_index;

fdi_ret_t fdi_rmc_inst(fdi_dbg_node_t evt, tick_type_t tick_type, uint32_t identifier);
void fdi_rmc_reset_index(void);
uint32_t fdi_rmc_get_index(void);

#else

#define FDI_RMC_INS_START_NULL(_evt)
#define FDI_RMC_INS_STOP_NULL(_evt)
#define FDI_RMC_INS_START_ID(_evt, _identifier)
#define FDI_RMC_INS_STOP_ID(_evt, _identifier)

#endif /* FEATURE_FDI */

#endif /*_FDI_RMC_H_*/
