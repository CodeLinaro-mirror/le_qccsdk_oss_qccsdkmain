/*
Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
*/
#ifndef _UART_PRINT_H_
#define _UART_PRINT_H_

/*-------------------------------------------------------------------------
 * Include header files from standard path
 *-----------------------------------------------------------------------*/
#include <stdint.h>
#include "nt_hw.h"

/*-------------------------------------------------------------------------
 * Preprocessor Definitions, Constants, and Type Declarations
 *-----------------------------------------------------------------------*/

#define UART_PRINT_ENABLE

#define UNUSED(x) (void)(x) 

#define LOG_LEVEL  2

#define UART_PRINT_BUFF_LENGTH            256

extern char uart_print_buff[UART_PRINT_BUFF_LENGTH];

#ifdef UART_PRINT_ENABLE
#define UART_PRINT(...) \
    do { \
        snprintf(uart_print_buff, UART_PRINT_BUFF_LENGTH, __VA_ARGS__); \
        frn_printf(uart_print_buff); \
	} while (0);

#define UART_PRINT_INIT()     frn_uart_init()
#else
#define UART_PRINT(...)

#define UART_PRINT_INIT(...)
#endif

#define R_SCS_NVIC_ISER0   0xE000E100    //Irq 0 to 31 Set Enable Register

#define SYS_UART_DLL    ( QWLAN_UART_UART_RBR_REG )
#define SYS_UART_THR    ( QWLAN_UART_UART_RBR_REG )
#define SYS_UART_RBR    ( QWLAN_UART_UART_RBR_REG )
#define SYS_UART_DLH    ( QWLAN_UART_UART_DLH_REG )
#define SYS_UART_IER    ( QWLAN_UART_UART_DLH_REG )
#define SYS_UART_FCR    ( QWLAN_UART_UART_IIR_REG )
#define SYS_UART_LSR    ( QWLAN_UART_UART_LSR_REG )
#define SYS_UART_MSR    ( QWLAN_UART_UART_MSR_REG )
#define SYS_UART_LCR    ( QWLAN_UART_UART_LCR_REG )
#define SYS_UART_FAR    ( QWLAN_UART_UART_FAR_REG )
#define SYS_UART_IIR    ( QWLAN_UART_UART_IIR_REG )
#define SYS_UART_MCR    ( QWLAN_UART_UART_MCR_REG )

#define UART_ERDA_INTTERUPT_DISABLE     0x00
#define UART_ERDA_INTTERUPT_ENABLE 		0x01
#define FCR_DISABLE                 0x00

#define UART_DLL_BAUD          (0x20) // 115200

/*-------------------------------------------------------------------------
 * Type Declarations
 *-----------------------------------------------------------------------*/

/*-------------------------------------------------------------------------
 * Function Declarations
 *-----------------------------------------------------------------------*/

void frn_uart_init(void);
void frn_uart_sent( const char *data, uint32_t length );
void frn_printf( const char *print );

#endif
