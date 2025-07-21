/*
 *  * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _HTTPD_H_

#define _HTTPD_H_

#include "qapi_status.h"
#include "qat_api.h"

#ifndef min
#define min(a, b) (((a) <= (b)) ? (a) : (b))
#endif

qapi_Status_t httpd_command_handler(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List);
qapi_Status_t syscfg_command(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List);

#endif