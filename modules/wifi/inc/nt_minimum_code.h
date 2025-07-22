/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef CORE_SYSTEM_INC_NT_MINIMUM_CODE_H_
#define CORE_SYSTEM_INC_NT_MINIMUM_CODE_H_

#include "nt_socpm_sleep.h"
//#include "core_cm4.h"
#include "ExceptionHandlers.h"

// M4: if used during PS, CPU timer starts downcounts from this value
#define NT_SYSTCK_PS_DFLT 0xFFFFFF
// M4: current timer value reg addr
#define NT_SYSTCK_CVR_REG 0xE000E018
/* Number of cycles to delay accessing other CMEM banks/sub-banks for avoiding
 * power inrush issues. Each iteration of the nop loop takes three instructions
 * and 7 iterations would provide a delay of 21 instructions.
 */
#define MIN_CMEM_INRUSH_DELAY 7

#endif /* CORE_SYSTEM_INC_NT_MINIMUM_CODE_H_ */
