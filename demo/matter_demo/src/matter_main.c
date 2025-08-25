/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <string.h>
#include <stdint.h>
#if CONFIG_MATTER_ENABLE
#include "matter_demo.h"
#endif

#ifdef FERMION_SILICON
extern uint32_t UART_Send_direct(char *txbuf, uint32_t buflen);
#define UART_SEND_DIRECT(str) UART_Send_direct((str), strlen(str))
#else
#define UART_SEND_DIRECT(str)
#endif

void __cpp_initialize__aeabi_(void);
void app_init(void)
{
    __cpp_initialize__aeabi_();
#if CONFIG_MATTER_ENABLE
    Initialize_Matter_Demo();
#endif
}

void app_main(void)
{
    UART_SEND_DIRECT("Matter demo running...\r\n");
}
