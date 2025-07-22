/*
 *  * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _AT_WEB_SERVER_H_
#define _AT_WEB_SERVER_H_ 1

#define AT_WEB_MAX_SSID_SIZE 32
#define AT_WEB_MAX_PWD_SIZE  64

int at_get_wifi_cfg(char *ssid, char *password);

int at_web_start(uint16_t server_port);

int at_web_stop(void);

#endif /* _AT_WEB_SERVER_H_ */
