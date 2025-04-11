/*
 *  * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "qapi_status.h"
#include "safeAPI.h"
#include <unistd.h>
#include "httpd_demo.h"

#include "qat_api.h"

#ifdef CONFIG_HTTP_SERVER_DEMO

#define HTTPS_PRINTF(...)                   printf( __VA_ARGS__)

void httpd_command_help(void)
{
    HTTPS_PRINTF("httpc {start | stop}\n");
    HTTPS_PRINTF(" httpc start {PORT}\n");
}

qapi_Status_t httpd_command_handler(uint32_t __attribute__((__unused__)) Parameter_Count, QAPI_Console_Parameter_t __attribute__((__unused__)) *Parameter_List)
{
    int error = QAPI_OK;
    struct http_client_demo_s* arg = NULL;

    uint8_t enable = 0;
	uint32_t port = 0;
    uint32_t num = 0;

    if(Parameter_Count < 1)
    {
usage:
        httpd_command_help();
        return QAPI_ERROR;
    }

    enable = Parameter_List[0].Integer_Value;
	port = Parameter_List[1].Integer_Value;
    if (enable)
    {
        HTTPS_PRINTF("HTTP server start \r\n");
        error = qapi_web_start(port);
        if (error)
        {
            HTTPS_PRINTF("HTTP Server start failed %d\r\n", error);
            return QAPI_ERROR;
        }
        return QAPI_OK;
    }
    else {
        error = qapi_web_stop();	
        if (error)
        {
            HTTPS_PRINTF("HTTP Client stop failed %d\r\n",error);
            return QAPI_ERROR;
        }
        return QAPI_OK;
    }
    
}   

#endif
