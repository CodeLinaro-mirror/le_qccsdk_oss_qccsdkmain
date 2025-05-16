/*
* Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/


#include <string.h>
#include <stdint.h>
#include <stdbool.h>

/****************************************************************************
  * UART Write functions
  ***************************************************************************/
 #ifdef FERMION_SILICON
 extern uint32_t UART_Send_direct(char *txbuf,uint32_t buflen);
 #define UART_SEND_DIRECT(str)   UART_Send_direct((str),strlen(str))
 #else
 #define UART_SEND_DIRECT(str)
 #endif

 // UART LOG meesages
#define MAX_LOG_STR_LEN     255

#define LOG_INF(args...)                                                \
{                                                                       \
    char    log_txt[MAX_LOG_STR_LEN] = "Demo: ";                         \
    const int8_t  prefix_len = 5;                                       \
                                                                        \
    snprintf(log_txt + prefix_len, MAX_LOG_STR_LEN - prefix_len, args); \
    log_txt[MAX_LOG_STR_LEN - 1] = 0;                                   \
    UART_SEND_DIRECT(log_txt);                                          \
    UART_SEND_DIRECT("\n\r");                                           \
}

void demo_app(void);

void demo_init(void);

/**
 * IP Ready event notifier
 */
void demo_wmi_ip_addr_ready_event();
 