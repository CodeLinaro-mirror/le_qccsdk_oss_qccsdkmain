/*
 *Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>
#include "stdio.h"
#include "qapi_prng.h"
#include "qapi_status.h"
#include "qapi_console.h"
#include "qcli_api.h"
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"

#define RNG_SHELL_GROUP_NAME "RNG"

static qapi_Status_t Init(uint32_t __attribute__((__unused__)) Parameter_Count,
                          QAPI_Console_Parameter_t __attribute__((__unused__)) * Parameter_List)
{
    nt_prng_init();
    return QAPI_OK;
}

static qapi_Status_t Get(uint32_t __attribute__((__unused__)) Parameter_Count,
                         QAPI_Console_Parameter_t __attribute__((__unused__)) * Parameter_List)
{
    uint32_t rng_data = 0;
    char buffer[100];
    rng_data = nt_pget_rng();
    snprintf(buffer, sizeof(buffer), "rng data is 0x%08x", rng_data);
    printf("%s\r\n", buffer);
    return QAPI_OK;
}

static qapi_Status_t Getn(uint32_t __attribute__((__unused__)) Parameter_Count,
                          QAPI_Console_Parameter_t __attribute__((__unused__)) * Parameter_List)
{
    if (Parameter_Count < 1 || !Parameter_List) {
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }

    uint32_t len = Parameter_List[0].Integer_Value;
    if (len <= 0 || len > 1024) {
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }

    uint8_t *ptr = NULL;
    ptr = (uint8_t *)malloc(len);

    if (ptr == NULL) {
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }
    qapi_prng_get((uint8_t *)ptr, (uint16_t)len);

    uint32_t i = 0;
    printf("0x");
    for (i = 0; i < len; i++) {
        printf("%02x", *(ptr + i));
    }
    printf("\r\n");
    free(ptr);
    ptr = NULL;

    return QAPI_OK;
}

static const QAPI_Console_Command_t rng_shell_cmds[] = {
    // cmd_function      cmd_string      usage_string                            description
    {Init, "init", " ", "init rng driver"},
    {Get, "get", " ", "get random number"},
    {Getn, "getn", " <len>", "get n-Bytes(0,1024] random number"},
};

static const QAPI_Console_Command_Group_t rng_shell_cmd_group = {
    RNG_SHELL_GROUP_NAME, sizeof(rng_shell_cmds) / sizeof(QAPI_Console_Command_t), rng_shell_cmds};

QAPI_Console_Group_Handle_t rng_shell_cmd_group_handle;

void rng_shell_init(void)
{
    rng_shell_cmd_group_handle = QAPI_Console_Register_Command_Group(NULL, &rng_shell_cmd_group);
}
