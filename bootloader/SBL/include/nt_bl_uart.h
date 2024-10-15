/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/


#ifndef _NT_BL_UART_H_
#define _NT_BL_UART_H_

#include <stdint.h>
#include "nt_hw.h"

//UART driver
//#define UART_ENABLE
#define R_SCS_NVIC_ISER0   0xE000E100    //Irq 0 to 31 Set Enable Register
#define NVIC_ISER0_ENABLE           0x8

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

#ifdef FERMION_SILICON
// chip platform at 60MHz,baudrate 60M/(16*230400)
#define SYS_CLOCK     (60000000u)
#define UART_DLL_BAUD          (0x20) // 115200
#else
// emu runs at 15MHZ,baudrate 15M/(16*230400)
#define SYS_CLOCK     (15000000u>>2)
#define UART_DLL_BAUD           (0x1)
#endif

void nt_uartInit(void);

void nt_uart_sent( const char *data, uint32_t length );
void nt_pbl_printf( const char *print );

void pbl_uart_irq_handler_ind(void);
void pbl_uart_irq_handler(void);
void setuartFlag( uint8_t flag );
uint8_t getuartFlag( void );
void pbl_uart_receive_buff(char uart_ch);

#endif
