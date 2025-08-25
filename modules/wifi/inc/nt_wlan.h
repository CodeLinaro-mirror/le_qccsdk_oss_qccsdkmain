/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef INC_NT_WLAN_H_
#define INC_NT_WLAN_H_

#include <stdio.h>
#include "nt_osal.h"

#include "iot_wifi.h"
//#include "nt_sme_mlme.h"

#define INVALID_OPCODE      0x4
#define MAX_CMD_IND         12
#define COMMAND_MATCHED     1
#define COMMAND_NOT_MATCHED 0
#define INVALID_MAP         10
#define PREPACK

typedef void (*res_function)(void *, void *);
typedef void (*resp_function)(void *);
typedef void (*event)(WIFIReturnCode_t, event_t, void *);

/*Message ID's*/

typedef struct test {
    uint8_t ress;
} test;

/*Neutrino Code*/

#endif /* INC_NT_WLAN_H_ */
