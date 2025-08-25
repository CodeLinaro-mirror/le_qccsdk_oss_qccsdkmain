/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <string.h>
#include <stdint.h>
#include "stdio.h"
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"
#include "posix_demo.h"

#ifdef FERMION_SILICON
extern uint32_t UART_Send_direct(char *txbuf, uint32_t buflen);
#define UART_SEND_DIRECT(str) UART_Send_direct((str), strlen(str))
#else
#define UART_SEND_DIRECT(str)
#endif

#define mainPOSIX_DEMO_PRIORITY (tskIDLE_PRIORITY + 4)

static volatile int dead_loop = 0;

void app_init(void)
{
    UART_SEND_DIRECT("app_init entry\r\n");
    // register app console commands here if have
    UART_SEND_DIRECT("app_init over\r\n");
}

void app_main(void)
{
    // const uint32_t ulLongTime_ms = pdMS_TO_TICKS( 1000UL );

    printf("FreeRTOS POSIX demo\n");

    /* Start the task to run POSIX demo */
    xTaskCreate(vStartPOSIXDemo, "posix", configMINIMAL_STACK_SIZE, NULL, mainPOSIX_DEMO_PRIORITY, NULL);

    UART_SEND_DIRECT("app_main over\r\n");
}
