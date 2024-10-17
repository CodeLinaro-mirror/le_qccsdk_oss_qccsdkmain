/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <string.h>
#include <stdint.h>
#include "qat_demo.h"
#ifdef FERMION_SILICON
extern uint32_t UART_Send_direct(char *txbuf,uint32_t buflen);
#define UART_SEND_DIRECT(str)   UART_Send_direct((str),strlen(str))
#else
#define UART_SEND_DIRECT(str)
#endif

static volatile int dead_loop = 0;

void app_init(void)
{
    UART_SEND_DIRECT("app_init entry\r\n");
    //register app console commands here if have
#ifdef CONFIG_QAT_COMMON_DEMO
    Initialize_QAT_Common_Demo();
#endif 

    UART_SEND_DIRECT("app_init over\r\n");
}

void app_main(void)
{
    UART_SEND_DIRECT("app_main entry\r\n");
    UART_SEND_DIRECT("qcli demo!\r\n");
    if (dead_loop) {
        UART_SEND_DIRECT("Dead loop...\r\n");
        while(dead_loop);
    }
    UART_SEND_DIRECT("app_main over\r\n");
}
