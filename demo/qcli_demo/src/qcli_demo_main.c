/*
#Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
#SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <string.h>
#include <stdint.h>
#ifdef CONFIG_FWUP_DEMO
#include "ota_demo.h"
#endif
#if CONFIG_MPU_DEMO
#include "mpu_demo.h"
#endif
#ifdef CONFIG_MGMT_FILTER_DEMO
#include "mgmt_filter_demo.h"
#endif

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
#ifdef CONFIG_FWUP_DEMO
    Initialize_FwUpgrade_Demo();
#endif 
    Initialize_Crypto_Demo();
#ifdef CONFIG_QCSPI_HFC_TEST
	extern void Initialize_qcspi_hfc_Demo(void);
	Initialize_qcspi_hfc_Demo();
#endif
#if CONFIG_MPU_DEMO
	Initialize_MPU_Demo();
#endif
#ifdef CONFIG_MGMT_FILTER_DEMO
	Initialize_Mgmt_Filter_Demo();
#endif
#if defined(CONFIG_MBEDTLS_AES_ALT) || defined(CONFIG_MBEDTLS_CCM_ALT) || defined(CONFIG_MBEDTLS_SHA_ALT)
    Initialize_Qcc_Demo();
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

