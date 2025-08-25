/*
Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#ifndef _UART_H_
#define _UART_H_

// #include "hal_int_cfg.h"

#include <stddef.h>
#include <stdlib.h>
#include "autoconf.h"
#include <stdint.h>
#include "nt_hw.h"
#include "wifi_cmn.h"
#include "fwconfig_cmn.h"
#include "nt_flags.h"
#include "nt_osal.h"
#include "nt_logger_api.h"

#define NATIVE_CTL0_ADDR   0x000FFF00
#define PASS_FLAG          0x07770000
#define FAIL_FLAG          0x09110000
#define CDAHB_VERSION_ADDR 0x011a0090

//#define NVIC_BASE            	0xE000E100
#define NVIC_ICTR        0xE000E004  // Interrupt Controller type reg
#define R_SCS_NVIC_ISER0 0xE000E100  // Irq 0 to 31 Set Enable Register
#define R_SCS_NVIC_ICER0 0xE000E180  // Irq 0 to 31 Clear Enable Register
#define R_SCS_NVIC_IABR0 0xE000E300  // Irq 0 to 31 Active Bit Register
#define R_SCS_ICSR       0xE000E300  // Interrupt Control State Register
#define R_SCS_IPSR       0xE000E400  // Interrupt Control State Register

#define UART_ERDA_INTTERUPT_DESABLE 0x00
#define UART_ERDA_INTTERUPT_ENABLE  0x01
#define CLOCK_ENABLE                0x03EE11
#define UART_MODEM_DISABLE          0x00
#define UART_MODEM_ENABLE           0x80
#define UART_DLAB_ENABLE            0x83
#ifdef NT_NEUTRINO_1_0_SYS_MAC
//#define UART_DLL_BAUD          	 	0x20  //115200 baud rate for chip(60Mhz)
#define UART_DLL_BAUD 0x10  // 230400 baud rate for chip(60Mhz)
#else
//#define UART_DLL_BAUD          	 	0x08 //115200 baud rate for FPGA(15Mhz)
#define UART_DLL_BAUD 0x04  // 230400 baud rate for FPGA(15Mhz)
#endif
#define UART_DLH_BAUD      0x00
#define UART_DLAB_DISABLE  0x03
#define INTERRUPT_ENABLE   0x1
#define NVIC_ICER0_DISABLE 0x00
#define NVIC_ISER0_ENABLE  0x8
#define FCR_DISABLE        0x00

#define UART_INTCFG_THRE_OFFSET  2
#define UART_INTCFG__ERDA_OFFSET 1
#define UART_FIFO_INT_BIT        4

#define UART_TRANS_TIME_OUT      1000
#define UART_UART_LSR_TEMPT_BUSY 0x00
// ALL MAC FOR BAUD_RATE_CHANGE

#define UART_BAUD_RATE_110    110
#define UART_BAUD_RATE_300    300
#define UART_BAUD_RATE_600    600
#define UART_BAUD_RATE_1200   1200
#define UART_BAUD_RATE_2400   2400
#define UART_BAUD_RATE_4800   4800
#define UART_BAUD_RATE_9600   9600
#define UART_BAUD_RATE_14400  14400
#define UART_BAUD_RATE_19200  19200
#define UART_BAUD_RATE_38400  38400
#define UART_BAUD_RATE_57600  57600
#define UART_BAUD_RATE_115200 115200
#define UART_BAUD_RATE_230400 230400
#define UART_BAUD_RATE_460800 460800
#define UART_BAUD_RATE_921600 921600

//#define UART_FIFO_ENABLE           0x07    vinod disabled

#define UART_IIR_INTID_RLS  ((uint32_t)(3 << 1))
#define UART_IIR_INTID_RDA  ((uint32_t)(2 << 1))
#define UART_IIR_INTID_CTI  ((uint32_t)(6 << 1))
#define UART_IIR_INTID_THRE ((uint32_t)(1 << 1))
#define UART_IIR_INTID_MASK ((uint32_t)(7 << 1))
#define UART_TX_FIFO_SIZE   (16)

#define UART_FIFO_EMPTY ((uint8_t)(1 << 1))

#define UART_FCR_DMA_DISABLE ((uint8_t)(1 << 3))  // vinod

#define UART_MCR_ENABLE ((uint32_t)(0x43 << 0))
#define UART_IER_ENABLE ((uint32_t)(0xF << 0))

#define NUM_OF_INTRS        1  // 32 to 63 interrupts
#define UART_INTR_ENABLE    0xFF
#define LINE_STA_BIT_ENABLE 0x04

#define CMEM_START_ADDR 0x00001000

#define UART_LSR_RDR ((uint8_t)(1 << 0)) /*!<Line status register: Receive data ready*/

#define SYS_UART_TFR   0x01233874  // To Transmitt Data From Transmit Buffer If FIFO is Enable
#define SYS_CLK_ENABLE 0x011af41c

#define UART_RFW 0x01233878  // To Read Data From RX Buffer If FIFO is Enable

#define CMEM_END_ADDR 0x0007F000

#define cmdMAX_INPUT_SIZE 500

typedef enum {
    RCLI_SIGNAL_UART_MSG,
    RCLI_SIGNAL_SPI_MSG,
    RCLI_SIGNAL_LOGGER_MSG,
} LOGGER_MESSAGE_SOURCE_ENUM;

#ifndef SUPPORT_FERMION_LOGGER
#define UART_Send(x, n) UART_Send_direct(x, n)
#else
#define UART_Send(x, n) DBG_STR_PRINT("", x, n)
#define DBG_STR_PRINT(msg, ptr, len)                                      \
    do {                                                                  \
        DEBUG_LOG_STRING(log_fmt, msg                                     \
                         ""                                               \
                         " %s");                                          \
        log_dynamic_string_to_buffer_with_len(log_fmt, (char *)ptr, len); \
    } while (0)
#endif  // SUPPORT_FERMION_LOGGER

#if (NT_CHIP_VERSION == 2)
#define UART_DLS_MASK 0x3
#endif  //(NT_CHIP_VERSION==2)
/*-----------------------------------------------------------*/

void myputchar(uint8_t ch);
void uart_init(void);
void uart_set_baud_rate(uint32_t uart_baud_rate);
void myputs(const char *s);
int transmit(const char *str);
void uart_irq_handler(void);
void UART_Recieve_buff(void);
void FreeRTOS_UART_open(void);
void FreeRTOS_UART_write(const void *pvBuffer, const size_t xBytes);
size_t FreeRTOS_UART_read(uint8_t *const pvBuffer, const size_t xBytes);
uint32_t UART_Send_direct(char *txbuf, uint32_t buflen);
uint32_t UART_Send_u32(char *txbuf, uint32_t buflen);
// void xIOUtilsClearRxCharQueue( Peripheral_Control_t * const pxPeripheralControl );
uint32_t UART_Receive(uint8_t *rxbuf, uint32_t buflen);
void nt_dbg_print(char *Printstring);
void nt_systick_ms_delay(uint8_t dly_t);
#if (NT_CHIP_VERSION == 2)
void uart_data_status(void);
void uart_stop_bit_status(void);
void uart_parity_status(void);
#endif  //(NT_CHIP_VERSION==2)
#endif  /* _UART_H_ */
