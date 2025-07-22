/*========================================================================
Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
* @file mib.h
* @brief Function definitions
*========================================================================*/

#ifndef _MIB_H_
#define _MIB_H_

/*-------------------------------------------------------------------------
 * Include files
 * ----------------------------------------------------------------------*/

#include "fwconfig_wlan.h"
#include "nt_flags.h"
#ifdef FERMION_CONFIG_HCF

#define LEN(Var) (Var[3] | (Var[2] << 8))
#define MIN_LEN  0x12
#define MAX_LEN  0x2000

/*-------------------------------------------------------------------------
 * Function declaration
 * ----------------------------------------------------------------------*/
extern uint8_t mibtypeint(uint8_t *data);

extern uint8_t mibsetint(uint16_t key, uint8_t *data);

extern uint8_t mibsetos(uint16_t key, uint8_t *octet_array, uint16_t vl);

#endif  // FERMION_CONFIG_HCF
#endif  //_MIB_H_
