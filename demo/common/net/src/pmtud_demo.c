/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*-------------------------------------------------------------------------
 * Include Files
 *-----------------------------------------------------------------------*/
#include <stdio.h>
#include "pmtud.h"
#include "pmtud_demo.h"

/*-------------------------------------------------------------------------
 * Function Definitions
 *-----------------------------------------------------------------------*/

/**
 * Display path mtu for the network
 */
qapi_Status_t pmtud_demo(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    ip_addr_t ip_addr;
    char *ptr = NULL;
    int32_t mtu;

    if (Parameter_Count < 1 || Parameter_List == NULL) {
        printf("\nUsage: mtud [--dst host]\n");
        printf("  --dst = find the mtu to dst host\n");
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }

    for (uint8_t i = 0; i < Parameter_Count; i++) {
        if (0 == strcmp(Parameter_List[i].String_Value, "--dst")) {
            i++;
            ptr = Parameter_List[i].String_Value;
            if (!ipaddr_aton(ptr, &ip_addr)) {
                printf("error: invalid address\n");
                return QAPI_ERR_INVALID_PARAM;
            }
        } else {
            printf("Default network interface not initialized");
            return QAPI_ERR_INVALID_PARAM;
        }
    }

    mtu = qapi_Path_MTU_Discover(&ip_addr);
    if (mtu < 0) {
        printf("timeout: the path mtu discover failed.");
        return QAPI_ERROR;
    }

    printf("Minimum MTU found: %d\n", mtu);
    return QAPI_OK;
}
