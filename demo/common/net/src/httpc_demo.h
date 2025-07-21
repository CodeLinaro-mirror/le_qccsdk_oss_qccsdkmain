/*
 *  * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*
 *  Copyright (C) 2006-2015, ARM Limited, All Rights Reserved
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may
 *  not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  This file is part of mbed TLS (https://tls.mbed.org)
 */

#ifndef _HTTPC_H_

#define _HTTPC_H_

#include "qapi_status.h"

#ifndef min
#define min(a, b) (((a) <= (b)) ? (a) : (b))
#endif

#ifdef CONFIG_QAT_HTTPC_DEMO
extern uint16_t at_httpc_method;
extern int at_rec_state;
extern uint16_t at_rec_error_code;

typedef enum { QAT_HTTP_HEAD = 1, QAT_HTTP_GET, QAT_HTTP_GETSIZE, QAT_HTTP_POST, QAT_HTTP_PUT } HTTPC_Method;
#endif

qapi_Status_t httpc_command_handler(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List);

#endif
