/*
Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear*/

 /*-------------------------------------------------------------------------
* Include Files
*-----------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include "nt_hw.h"
#include "uart_print.h"
#include "nvm_common.h"
#include "nt_flags.h"

/*-------------------------------------------------------------------------
 * Global Variables
 *-----------------------------------------------------------------------*/

char uart_print_buff[UART_PRINT_BUFF_LENGTH];

/*-------------------------------------------------------------------------
 * Function Declarations
 *-----------------------------------------------------------------------*/
void nop_delay( uint32_t n );

/*-------------------------------------------------------------------------
 * Private Function Declarations
 *-----------------------------------------------------------------------*/

/*-------------------------------------------------------------------------
 * Function Definitions
 *-----------------------------------------------------------------------*/

void frn_uart_init(void)
{
    uint32_t uart_config = 0;

    HW_REG_WR(SYS_UART_IER, UART_ERDA_INTTERUPT_DISABLE);

    // Enable root clock of UART
    uart_config = HW_REG_RD(QWLAN_PMU_ROOT_CLK_ENABLE_REG);
    uart_config |= QWLAN_PMU_ROOT_CLK_ENABLE_UART_ROOT_CLK_ENABLE_MASK;
    HW_REG_WR(QWLAN_PMU_ROOT_CLK_ENABLE_REG, uart_config);

    HW_REG_WR(QWLAN_UART_UART_MCR_REG,QWLAN_UART_UART_MCR_DEFAULT);
    //DLAB bit enable and Set to 8 bits data
    HW_REG_WR(QWLAN_UART_UART_LCR_REG,QWLAN_UART_UART_LCR_DLAB_MASK | QWLAN_UART_UART_LCR_DLS_MASK );
    // Configure the DLL will configure the baud rate
    // Value will be determined using formula Baud rate = (system_clock)/(16 * divisor)
    HW_REG_WR(SYS_UART_DLL,UART_DLL_BAUD);
    HW_REG_WR(QWLAN_UART_UART_DLH_REG,QWLAN_UART_UART_DLH_DEFAULT);
    // Disable the DLAB in LCR register
    HW_REG_WR(QWLAN_UART_UART_LCR_REG,QWLAN_UART_UART_LCR_DLS_MASK);
    nop_delay(10);
    HW_REG_WR(SYS_UART_FCR,FCR_DISABLE);
    HW_REG_WR(SYS_UART_IER,UART_ERDA_INTTERUPT_ENABLE);
}

void frn_myputchar(uint32_t ch) {
    uint16_t timeout = 0;
    // Check the transmit holding register empty bit
    do{
        timeout++;
    }while(((HW_REG_RD(QWLAN_UART_UART_LSR_REG) & QWLAN_UART_UART_LSR_TEMPT_MASK ) == 0x00) && (timeout < 1000));
    //Write to the transmit holding register
    HW_REG_WR(SYS_UART_THR,ch);
}

void frn_uart_sent( const char *data, uint32_t length )
{
    while(length--)
    {
        frn_myputchar( *data++);
    }
}

void frn_printf( const char *print )
{
    frn_uart_sent(print, strlen(print));
}

uint8_t nt_log_printf(
		uint8_t mod_id,
		uint8_t loglvl,
#if( NT_FN_FUNCTION_LINE_NUM_FLAG == 1)
		char *func_name,
		/*@ for line number*/
		uint16_t ln,
#endif
		const char *fmt,
		uint8_t num,
		...
		)
{
#ifndef UART_PRINT_ENABLE
    UNUSED(loglvl);
    UNUSED(mod_id); 
#if( NT_FN_FUNCTION_LINE_NUM_FLAG == 1)    
    UNUSED(func_name);
    UNUSED(ln);
#endif
    UNUSED(fmt);
    UNUSED(num);
#else
    if(loglvl < LOG_LEVEL)
        return 0;
    
    va_list argp;

    snprintf(uart_print_buff,sizeof(uart_print_buff), "%ld %ld"
                                ": "
#if( NT_FN_FUNCTION_LINE_NUM_FLAG == 1)
                                "(%s:%lu) "
#endif
                                ,(uint32_t)mod_id,
                                (uint32_t)loglvl
#if( NT_FN_FUNCTION_LINE_NUM_FLAG == 1)
                                ,func_name,
                                (uint32_t)ln
#endif
                            );

    frn_printf(uart_print_buff);

    va_start(argp, num);

    vsnprintf(uart_print_buff, sizeof(uart_print_buff), fmt, argp);
    frn_printf(uart_print_buff);

    va_end(argp);
    
    snprintf(uart_print_buff,sizeof(uart_print_buff),"\r\n");
    frn_printf(uart_print_buff);
#endif
    return 0;
}

