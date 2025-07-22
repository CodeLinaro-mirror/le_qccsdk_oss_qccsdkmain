/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/******************************************************************************
 * @file    fdi.h
 * @brief   Interface for using Fermion Debug Infra
 *
 *
 *****************************************************************************/
#ifndef _FDI_H_
#define _FDI_H_

#include "nt_flags.h"

#include "wifi_fw_dbg_infra.h"
#include "wifi_fw_dbg_infra_cmn.h"
#include "fdi_rmc.h"

#ifdef FEATURE_FDI
#include "nt_common.h"

/* To Enable FDI DBG Codes*/
#define FDI_DBG (FDI_RESET)

/****************************************
 * @brief Register all debug Nodes
 *
 ***************************************/
void fdi_reg_all_nodes(void);

#endif /* FEATURE_FDI */

#define FDI_PS_TXT  __attribute__((section(".__sect_ps_txt")))
#define FDI_PS_DATA __attribute__((section(".__sect_ps_data")))

#endif /* _FDI_H_ */
