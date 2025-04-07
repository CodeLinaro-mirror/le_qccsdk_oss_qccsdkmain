/*
 *  * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "qapi_status.h"

#define QAPI_WEB_MAX_SSID_SIZE                       32
#define QAPI_WEB_MAX_PWD_SIZE                        64

#ifndef MIN
#define  MIN(a,b)    (((a) <= (b)) ? (a) : (b))
#endif

qapi_Status_t qapi_get_wifi_cfg(char *ssid, char *password);

qapi_Status_t qapi_web_start(uint16_t server_port);

qapi_Status_t qapi_web_stop(void);

