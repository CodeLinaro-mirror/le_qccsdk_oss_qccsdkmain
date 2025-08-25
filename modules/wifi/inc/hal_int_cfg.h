/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _HAL_INT_CFG_DEFS_H_
#define _HAL_INT_CFG_DEFS_H_

// Compile time HAL internal configuration controls -------------------------------------

// uncomment this if big byte endian
#define HAL_CFG_BIGBYTE_ENDIAN
#define HAL_CFG_INC_DBG_PRINT  // include debug prints
//#define HAL_CFG_INC_TEST_THREAD   // build the HAL thread in
#define HAL_CFG_INC_PARAM_CHECKS  // keep parameter checks in functions
//#define HAL_CFG_INC_BCN_DISABLE   // disable beacon tx

#define HAL_CFG_INC_AP_STA1_ENABLE  // TODO: enable STAID 1 for AP mode auto; temporary for AP_STA link only

#ifdef HAL_CFG_INC_DBG_PRINT
#include "uart.h"
#include <string.h>  // strlen
#define HAL_DEBUG(x) UART_Send(x, strlen(x))
#else
#define HAL_DEBUG(x)
#endif

#endif  // _HAL_INT_CFG_DEFS_H_
