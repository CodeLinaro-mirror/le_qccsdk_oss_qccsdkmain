/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef CORE_SYSTEM_INC_FERM_QTMR_DEMO_H_
#define CORE_SYSTEM_INC_FERM_QTMR_DEMO_H_
#include "ferm_qtmr.h"
#include "qapi_types.h"
#include "qapi_status.h"
#include "qapi_console.h"
qapi_Status_t qtmr_demo_init(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List);
qapi_Status_t qtmr_demo_start(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List);
qapi_Status_t qtmr_demo_stop();
qapi_Status_t qtmr_demo_deinit();
void qtmr_demo_dump();
#endif
