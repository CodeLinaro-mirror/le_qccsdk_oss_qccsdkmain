/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __NET_SHELL_H__  // [
#define __NET_SHELL_H__

#include "qapi_net_status.h"
#include "qapi_console.h"

#define NET_SHELL_INFO 1
#define NET_SHELL_LOG  0

#define NET_SHELL_GROUP_NAME          "NET"
#define NET_SHELL_GROUP_PRINTF_SUFFIX "NET: "

#define LOG_PREFIX "[LOG] "

/* enable Iperf fucnion */
#define CONFIG_NET_IPERF

#if NET_SHELL_INFO
#define info_printf(msg, ...) printf(NET_SHELL_GROUP_PRINTF_SUFFIX msg, ##__VA_ARGS__)
#else
#define info_printf(args...) \
    do {                     \
    } while (0)
#endif

#if NET_SHELL_LOG
#define log_printf(msg, ...) printf(NET_SHELL_GROUP_PRINTF_SUFFIX LOG_PREFIX msg, ##__VA_ARGS__)
#else
#define log_printf(args...) \
    do {                    \
    } while (0)
#endif

typedef enum { FALSE = 0, TRUE = 1 } Boolean;

#define PRINT_ERR_NOT_SUPPORTED info_printf("Not supported yet\n")
#define PRINT_ERR_CMD_FAILED    info_printf("Cmd failed\n")

#define IPv4_IP_IDX      0
#define IPv4_NETMASK_IDX 1
#define IPv4_GATEWAY_IDX 2

#define IPv6_LINK_LOCAL_IDX 0

#define DEFAULT_NETIF_IDX netif_get_index(netif_default) /* Default netif idx */

void net_shell_init(void);

#endif  //]NET_SHELL_H
