/*
 *  * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "qapi_status.h"
#include "net_shell.h"
#include "safeAPI.h"
#include "mbedtls/ssl.h"
#include <unistd.h>
#include "at_web_server.h"
#include "qapi_httpsvr.h"

#ifdef CONFIG_HTTP_SERVER

/**
   @brief get wifi config.

   @param[in] ssid  device id

   @return
   - QAPI_OK     --  get wifi config successfully.
*/
qapi_Status_t qapi_get_wifi_cfg(char *ssid, char *password)
{
    if ((NULL == ssid) 
		|| (NULL == password))
    {
        return QAPI_ERROR;
    }

    at_get_wifi_cfg(ssid, password);
	
    return QAPI_OK;
}

/**
   @brief start http server.

   @param[in] server_port: the port of http server;

   @return
   - QAPI_OK        --  start http server successfully.
*/
qapi_Status_t qapi_web_start(uint16_t server_port)
{
    qapi_Status_t err;

    err = at_web_start(server_port);
    if (err != QAPI_OK) {
        return QAPI_ERROR;
    }

    return QAPI_OK;
}

/**
   @brief start http server.

   @param[in] ;

   @return
   - QAPI_OK        --  start http server successfully.
*/
qapi_Status_t qapi_web_stop(void)
{
    qapi_Status_t err;

    if ((err = at_web_stop()) != QAPI_OK) {
        return QAPI_ERROR;
    }

    return QAPI_OK;
}

#endif
