/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <string.h>
#include <stdint.h>
#include "qat_demo.h"
#ifdef FERMION_SILICON
extern uint32_t UART_Send_direct(char *txbuf, uint32_t buflen);
#define UART_SEND_DIRECT(str) UART_Send_direct((str), strlen(str))
#else
#define UART_SEND_DIRECT(str)
#endif

static volatile int dead_loop = 0;

void app_init(void)
{
    UART_SEND_DIRECT("app_init entry\r\n");
    // register app console commands here if have
#ifdef CONFIG_QAT_COMMON_DEMO
    Initialize_QAT_Common_Demo();
#endif

#ifdef CONFIG_QAT_TCPIP_DEMO
    Initialize_QAT_TCPIP_Demo();
#endif

#ifdef CONFIG_QAT_MQTT_DEMO
    Initialize_QAT_Mqtt_Demo();
#endif

#ifdef CONFIG_QAT_HTTP_SERVER_DEMO
    Initialize_QAT_Http_Server_Demo();
#endif

#ifdef CONFIG_QAT_WLAN_DEMO
    Initialize_QAT_Wlan_Demo();
#endif

#ifdef CONFIG_QAT_HTTPC_DEMO
    Initialize_QAT_HttpC_Demo();
#endif

#ifdef CONFIG_QAT_OTA_DEMO
    Initialize_QAT_OTA_Demo();

#ifdef CONFIG_FWUP_DEMO
    Initialize_FwUpgrade_Demo();
#endif

#ifdef CONFIG_QAT_POWERSAVE_DEMO
    Initialize_QAT_POWERSAVE_Demo();
#endif

#endif

#ifdef CONFIG_QAT_FSTORE_DEMO
    Initialize_QAT_Fstore_Demo();
#endif

    UART_SEND_DIRECT("app_init over\r\n");
}

void app_main(void)
{
    UART_SEND_DIRECT("app_main entry\r\n");
    UART_SEND_DIRECT("qcli demo!\r\n");
    if (dead_loop) {
        UART_SEND_DIRECT("Dead loop...\r\n");
        while (dead_loop)
            ;
    }
    UART_SEND_DIRECT("app_main over\r\n");
}
