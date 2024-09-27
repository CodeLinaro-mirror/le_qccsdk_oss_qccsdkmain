/*
*Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
*SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include "nt_hw.h"
#include "nt_bl_common.h"
#include "nt_bl_uart.h"
#include "nt_bl_system_init.h"
#include "Fermion_seq_hwioreg.h"


void nt_nop_delay( uint32_t n )
{
   uint32_t nop_count = 0;
   for( nop_count = 0; nop_count < n; nop_count++)
   {
       __asm volatile(" nop \n");
   }
}

void nt_uartInit(void)
{
	uint32_t uart_config = 0;
	uint32_t pwr_req = 0;

	//Power up PSS domain for UART
    pwr_req = HW_REG_RD(QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_REG);
	pwr_req |= QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_PD_PSS_CNTL_BIT_MASK;
    HW_REG_WR(QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_REG, pwr_req);

#if CONFIG_BOARD_QCC730_UART_ENABLE
#define FRN_BOOT_STRAP_VALUE  0x63887466
	NT_REG_WR(QWLAN_PMU_BOOT_STRAP_CONFIG_SECURE_REG, FRN_BOOT_STRAP_VALUE);

	HWIO_OUTXF(SEQ_WCSS_PMU_OFFSET, NEUTRINO_PMU_PRONTO_LP_FRODO_PMU_BOOT_STRAP_CONFIGURATION_STATUS,
				CFG_UART_ENABLE,CONFIG_BOARD_QCC730_UART_ENABLE);

#if defined(CONFIG_BOARD_QCC730_UART_GPIO_OPTION)
	NT_REG_WR(QWLAN_PMU_BOOT_STRAP_CONFIG_SECURE_REG, FRN_BOOT_STRAP_VALUE);

	HWIO_OUTXF(SEQ_WCSS_PMU_OFFSET, NEUTRINO_PMU_PRONTO_LP_FRODO_PMU_BOOT_STRAP_CONFIGURATION_STATUS,
				CFG_UART_OPTION, CONFIG_BOARD_QCC730_UART_GPIO_OPTION);
#endif
#else
    #warning "CONFIG_BOARD_QCC730_UART_ENABLE should be defined. See boad_defconfig"
#endif

	HW_REG_WR(SYS_UART_IER, UART_ERDA_INTTERUPT_DISABLE);

	// Enable root clock of UART
	uart_config = HW_REG_RD(QWLAN_PMU_ROOT_CLK_ENABLE_REG);
	uart_config |= QWLAN_PMU_ROOT_CLK_ENABLE_UART_ROOT_CLK_ENABLE_MASK;
	HW_REG_WR(QWLAN_PMU_ROOT_CLK_ENABLE_REG, uart_config);

	HW_REG_WR(R_SCS_NVIC_ISER0,NVIC_ISER0_ENABLE);

	HW_REG_WR(QWLAN_UART_UART_MCR_REG,QWLAN_UART_UART_MCR_DEFAULT);
	//DLAB bit enable and Set to 8 bits data
	HW_REG_WR(QWLAN_UART_UART_LCR_REG,QWLAN_UART_UART_LCR_DLAB_MASK | QWLAN_UART_UART_LCR_DLS_MASK );

	// Configure the DLL will configure the baud rate
	// Value will be determined using formula Baud rate = (system_clock)/(16 * divisor)
	HW_REG_WR(SYS_UART_DLL,UART_DLL_BAUD);
	HW_REG_WR(QWLAN_UART_UART_DLH_REG,QWLAN_UART_UART_DLH_DEFAULT);
	// Disable the DLAB in LCR register
	HW_REG_WR(QWLAN_UART_UART_LCR_REG,QWLAN_UART_UART_LCR_DLS_MASK);

	nt_nop_delay(10);

	HW_REG_WR(SYS_UART_FCR,FCR_DISABLE);
//HW_REG_WR(SYS_UART_IER,UART_ERDA_INTTERUPT_ENABLE);

	//setuartFlag(0);
}


#ifdef P_DEBUG
void nt_myputchar(uint32_t ch) {
#ifdef NT_SILENT_BOOT
	(void)ch;
#else
   uint16_t timeout = 0;
   // Check the transmit holding register empty bit
   do{
     timeout++;
   }while(((HW_REG_RD(QWLAN_UART_UART_LSR_REG) & QWLAN_UART_UART_LSR_TEMPT_MASK ) == 0x00) && (timeout < 1000));
   //Write to the transmit holding register
   HW_REG_WR(SYS_UART_THR,ch);
#endif
}

void nt_uart_sent( const char *data, uint32_t length )
{
   while(length--)
   {
     nt_myputchar( *data++);
   }
}

void nt_pbl_printf( const char *print )
{
   nt_uart_sent(print, strlen(print));
}
#endif





