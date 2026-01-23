/*
Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
*/
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "nt_socpm_sleep.h"
#include "uart.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "nt_common.h"
#include "ExceptionHandlers.h"

#if (NT_CHIP_VERSION == 2)
#include "nt_logger_api.h"
#endif  //(NT_CHIP_VERSION==2)

#include "nt_devcfg.h"
#include "ferm_prof.h"

#include "autoconf.h"
#include "qccx.h"

extern int SEGGER_RTT_printf(unsigned BufferIndex, const char *sFormat, ...);
/**uncomment below flag
 **to disable UART in Application image
 */
//#define _UART_DISABLE_ALL_FLAG

#define LOG_TAKE_MUTEX_SEMAPHORE_NONBLOCKING(mutex, result) (result = 1U)
#define LOG_GIVE_MUTEX_SEMAPHORE(mutex)
#define LOG_CREATE_MUTEX_SEMAPHORE(mutex)

#ifdef FERMION_SILICON
void nt_nop_delay(uint32_t n)
{
    uint32_t nop_count = 0;
    for (nop_count = 0; nop_count < n; nop_count++) {
        __asm volatile(" nop \n");
    }
}

// Register Read and Write
#define HW_REG_WR(_reg, _val)                  \
    (*((volatile uint32_t *)(_reg))) = (_val); \
    __asm volatile("dsb" ::: "memory")
#define HW_REG_RD(_reg) (*((volatile uint32_t *)(_reg)))
//#define UART_ENABLE
#define R_SCS_NVIC_ISER0  0xE000E100  // Irq 0 to 31 Set Enable Register
#define NVIC_ISER0_ENABLE 0x8

#define SYS_UART_DLL (QWLAN_UART_UART_RBR_REG)
#define SYS_UART_THR (QWLAN_UART_UART_RBR_REG)
#define SYS_UART_RBR (QWLAN_UART_UART_RBR_REG)
#define SYS_UART_DLH (QWLAN_UART_UART_DLH_REG)
#define SYS_UART_IER (QWLAN_UART_UART_DLH_REG)
#define SYS_UART_FCR (QWLAN_UART_UART_IIR_REG)
#define SYS_UART_LSR (QWLAN_UART_UART_LSR_REG)
#define SYS_UART_MSR (QWLAN_UART_UART_MSR_REG)
#define SYS_UART_LCR (QWLAN_UART_UART_LCR_REG)
#define SYS_UART_FAR (QWLAN_UART_UART_FAR_REG)
#define SYS_UART_IIR (QWLAN_UART_UART_IIR_REG)
#define SYS_UART_MCR (QWLAN_UART_UART_MCR_REG)
#define SYS_UART_USR (QWLAN_UART_UART_USR_REG)

#define UART_ERDA_INTTERUPT_DISABLE 0x00
#define UART_ERDA_INTTERUPT_ENABLE  0x01
#define FCR_DISABLE                 0x00

#define UART_DLL_BAUD_PBL (0x20)  // 115200

#define EMU_UART_SYS_CLK_SHIFT 0
#else
#define EMU_UART_SYS_CLK_SHIFT 4
#endif /* FERMION_SILICON */

#if (NT_CHIP_VERSION == 1) || defined(PLATFORM_FERMION)
// chip runs at 60MHz

#define NT_SYSTEM_CORE_CLOCK 60000000u /* this should be chip specific clock frequency value */

#endif

#if (NT_CHIP_VERSION == 2)

// emulation platform at 15MHz
#define NT_SYSTEM_CORE_CLOCK 15000000u /* this should be chip specific clock frequency value */
#endif

#define NT_HW_RD(_reg) *((volatile unsigned long *)(_reg))

extern uint32_t SystemCoreClock;

static int _uart_rx_buf_idx;
;
static uint32_t _uart_baud_rate = UART_BAUD_RATE_230400;
static char _uart_rx_buf[512];

#if (defined CONFIG_NT_RCLI)
volatile char cUartInputString[cmdMAX_INPUT_SIZE];
volatile uint16_t cInputIndex = 0;
#endif

#if (defined FTM_OVER_UART)
#ifdef cmdMAX_INPUT_SIZE
#undef cmdMAX_INPUT_SIZE
#endif
#define cmdMAX_INPUT_SIZE 1024 * 2 /* receive DIAG packet from  QDART */
volatile unsigned char cInputString[cmdMAX_INPUT_SIZE];
volatile uint16_t cInputIndex = 0;
uint16_t cInputLength = 0;
#endif
int uart_flag = 0;

void myputs(const char *s)
{
    while (*s) {
        myputchar((unsigned int)*s++);
    }
}

//#ifdef FTM_OVER_UART
#if 0
void ftm_diag_msg_tx(void *buf, int len)
{
    uint8_t *ch = (uint8_t *)buf;
    int i;
    for (i = 0; i < len; i++) {
        uint16_t cntr = 0;
        do {
            cntr++;
        }
        while (((NT_REG_RD(QWLAN_UART_UART_LSR_REG) & QWLAN_UART_UART_LSR_TEMPT_MASK )== UART_UART_LSR_TEMPT_BUSY) && (cntr < UART_TRANS_TIME_OUT));
        NT_REG_WR(QWLAN_UART_UART_RBR_REG, ch[i]);
    }
}
#endif

void myputchar(uint8_t ch)
{
#if CONFIG_AMBIENT_POWER_ENABLE
    return;
#endif

#ifdef FTM_OVER_UART
    char txbuf[2];
    txbuf[0] = (char)ch;
    txbuf[1] = 0;
    SEGGER_RTT_printf(0, txbuf);
#else

#ifdef CONFIG_RTT_VIEW_CLI
    char txbuf[2];
    txbuf[0] = (char)ch;
    txbuf[1] = 0;
    SEGGER_RTT_printf(0, txbuf);
#else
    if (nt_socpm_uart_flag_state_get(APP_MODE_SEL)) {
        __asm volatile(" nop	\n");
        (void)ch;
    } else {
        uint16_t cntr = 0;
        do {
            cntr++;
        } while (((NT_REG_RD(QWLAN_UART_UART_LSR_REG) & QWLAN_UART_UART_LSR_TEMPT_MASK) == UART_UART_LSR_TEMPT_BUSY) &&
                 (cntr < UART_TRANS_TIME_OUT));
        NT_REG_WR(QWLAN_UART_UART_RBR_REG, ch);
    }  //_UART_DISABLE_ALL
#endif
#endif
}

#define FERM_PMU_BOOT_STRAP_UNLOCK 0x63887466

static inline void uart_gpio_enable(uint8_t enable)
{
#if CONFIG_SOC_QCC730V1
    QCC730V1_PMU_BASE_Type *pmu = QCC730V1_PMU_BASE;
#elif CONFIG_SOC_QCC730V2
    QCC730V2_PMU_BASE_Type *pmu = QCC730V2_PMU_BASE;
#endif

    /* unlock the configure register */
    pmu->pmu.PMU_BOOT_STRAP_CONFIG_SECURE.reg = FERM_PMU_BOOT_STRAP_UNLOCK;
    if (enable) {
        pmu->pmu.PMU_BOOT_STRAP_CONFIGURATION_STATUS.bit.CFG_UART_ENABLE = enable;
#if CONFIG_SOC_QCC730V2
        pmu->pmu.PMU_BOOT_STRAP_CONFIG_SECURE.reg = FERM_PMU_BOOT_STRAP_UNLOCK;
        pmu->pmu.PMU_BOOT_STRAP_CONFIGURATION_STATUS.bit.CFG_UART_OPTION = CONFIG_BOARD_QCC730_UART_GPIO_OPTION;
#endif
    } else {
        pmu->pmu.PMU_BOOT_STRAP_CONFIGURATION_STATUS.bit.CFG_UART_ENABLE = enable;
    }
}

void __attribute__((section(".__sect_ps_txt"))) uart_init(void)
{
#ifdef FERMION_SILICON

    uint32_t uart_config = 0;

#if CONFIG_BOARD_QCC730_UART_ENABLE
    uart_gpio_enable(CONFIG_BOARD_QCC730_UART_ENABLE);
#else
#warning "CONFIG_BOARD_QCC730_UART_ENABLE should be defined. See boad_defconfig"
#endif

    HW_REG_WR(SYS_UART_IER, UART_ERDA_INTTERUPT_DISABLE);

    // Enable root clock of UART
    uart_config = HW_REG_RD(QWLAN_PMU_ROOT_CLK_ENABLE_REG);
    uart_config |= QWLAN_PMU_ROOT_CLK_ENABLE_UART_ROOT_CLK_ENABLE_MASK;
    HW_REG_WR(QWLAN_PMU_ROOT_CLK_ENABLE_REG, uart_config);

    // Enable interrupt for UART
    HW_REG_WR(R_SCS_NVIC_ISER0, NVIC_ISER0_ENABLE);

    HW_REG_WR(QWLAN_UART_UART_MCR_REG, QWLAN_UART_UART_MCR_DEFAULT);
    // DLAB bit enable and Set to 8 bits data
    HW_REG_WR(QWLAN_UART_UART_LCR_REG, QWLAN_UART_UART_LCR_DLAB_MASK | QWLAN_UART_UART_LCR_DLS_MASK);
    // Configure the DLL will configure the baud rate
    // Value will be determined using formula Baud rate = (system_clock)/(16 * divisor)
    HW_REG_WR(SYS_UART_DLL, UART_DLL_BAUD_PBL);
    HW_REG_WR(QWLAN_UART_UART_DLH_REG, QWLAN_UART_UART_DLH_DEFAULT);
    // Disable the DLAB in LCR register
    HW_REG_WR(QWLAN_UART_UART_LCR_REG, QWLAN_UART_UART_LCR_DLS_MASK);
    nt_nop_delay(10);
    HW_REG_WR(SYS_UART_FCR, FCR_DISABLE);
    HW_REG_WR(SYS_UART_IER, UART_ERDA_INTTERUPT_ENABLE);

#else
    if (nt_socpm_uart_flag_state_get(APP_MODE_SEL)) {
        __asm volatile(" nop	\n");
    } else {
        uint32_t baud;
        // uint8_t buffer[10];
        uint32_t value;
        // int exitflag;
        _uart_rx_buf_idx = 0;
        // TEMP TODO: restore devcfg read to a separate function
        // uint32_t *devcfg_baud_rate = ((uint32_t*)(nt_devcfg_get_config(NT_DEVCFG_UART_BAUD_RATE)));
        // uint32_t devcfg_baud_rate = 230400;

        // reading the line control register
        value = NT_REG_RD(QWLAN_UART_UART_LCR_REG);
        value &= (long unsigned int)(~(1 << QWLAN_UART_UART_LCR_DLAB_OFFSET));
        NT_REG_WR(QWLAN_UART_UART_LCR_REG, value);
        // LCR[7] = 0 like IER register is accessible
        value = NT_REG_RD(QWLAN_UART_UART_DLH_REG);
        // disable the Enable received data available interrupt(ERDA)
        value &= (long unsigned int)(~(1 << QWLAN_UART_UART_DLH_VALUE_OFFSET));
        NT_REG_WR(QWLAN_UART_UART_DLH_REG, value);
        // To Enable system clock of UART
        value = NT_REG_RD(QWLAN_PMU_ROOT_CLK_ENABLE_REG);
        value |= (1 << QWLAN_PMU_ROOT_CLK_ENABLE_UART_ROOT_CLK_ENABLE_OFFSET);
        NT_REG_WR(QWLAN_PMU_ROOT_CLK_ENABLE_REG, value);
        //__asm volatile(" nop					\n");
        // WriteRegister(NVIC_ICTR, INTERRUPT_ENABLE);//Configure the NVIC to support 64 interrupts
        // WriteRegister(R_SCS_NVIC_ICER0,NVIC_ICER0_DISABLE);//Enable interrupt for UART
        NT_REG_WR(R_SCS_NVIC_ISER0, NVIC_ISER0_ENABLE);  // Enable interrupt for UART
        //__asm volatile(" nop					\n");
        // Disable the modem control register
        NT_REG_WR(QWLAN_UART_UART_MCR_REG, QWLAN_UART_UART_MCR_DEFAULT);
        // LCR[7] = 1 Enable to set the baud rate in DLL.
        // reading the line control register
        value = NT_REG_RD(QWLAN_UART_UART_LCR_REG);
        value |= (0x1 << QWLAN_UART_UART_LCR_DLAB_OFFSET) | (0x3 << QWLAN_UART_UART_LCR_DLS_OFFSET);
        NT_REG_WR(QWLAN_UART_UART_LCR_REG, value);
        /* Baud = (frq / ( 16 * Divisables)); */
#ifdef EMULATION_BUILD
        baud = ((SystemCoreClock >> EMU_UART_SYS_CLK_SHIFT) / ((16) * (_uart_baud_rate)));
#else
        baud = ((SystemCoreClock) / ((16) * (_uart_baud_rate)));
#endif
        //__asm volatile(" nop					\n");
        // DLL register used to set baud rate
        NT_REG_WR(QWLAN_UART_UART_RBR_REG, baud);
        NT_REG_WR(QWLAN_UART_UART_DLH_REG,
                  UART_DLH_BAUD);  // Setting baudrate 230400 for both side Rx/Tx . assuming DLH is DLM not DLL
        // reading the line control register and disable the DLAB bit
        //__asm volatile(" nop					\n");
        value = NT_REG_RD(QWLAN_UART_UART_LCR_REG);
        value &= (long unsigned int)(~(1 << QWLAN_UART_UART_LCR_DLAB_OFFSET));
        NT_REG_WR(QWLAN_UART_UART_LCR_REG, value);
        //__asm volatile(" nop					\n");
        // LCR[7] = 0 like IER register is accessible
        value = NT_REG_RD(QWLAN_UART_UART_DLH_REG);
        //__asm volatile(" nop					\n");
        // Enable the Enable received data available interrupt(ERDA)
        value |= (1 << QWLAN_UART_UART_DLH_VALUE_OFFSET);
        NT_REG_WR(QWLAN_UART_UART_DLH_REG, value);
        __asm volatile(" nop					\n");
        __asm volatile(" nop					\n");
        __asm volatile(" nop					\n");
        //	nt_systick_ms_delay(2);
        NT_REG_WR(QWLAN_UART_UART_IIR_REG, FCR_DISABLE);
        //__asm volatile(" nop					\n");
    }
#endif
}

void FreeRTOS_UART_open(void)
{
    /* Setup the pins for the UART being used. */
    taskENTER_CRITICAL();
    {
        uart_init();
    }
    taskEXIT_CRITICAL();
}

void FreeRTOS_UART_write(const void *pvBuffer, const size_t xBytes)
{
    // size_t xReturn;
    UART_Send((char *)pvBuffer, (size_t)xBytes);
    // return xReturn;
}

void nt_dbg_print(char *Printstring)
{
    UART_Send(Printstring, strlen(Printstring));
}

uint32_t __attribute__((section(".__sect_ps_txt"))) UART_Send_direct(char *txbuf, uint32_t buflen)
{
    if (nt_socpm_uart_flag_state_get(APP_MODE_SEL)) {
        __asm volatile(" nop	\n");
        (void)txbuf;
        buflen = 0;
        return buflen;
    } else {  //_UART_DISABLE_ALL_FLAG
        uint32_t /*length=buflen,*/ sent_items = 0;
        while (/*length*/ buflen--) {
            myputchar((uint8_t) * /*data*/ txbuf++);
            sent_items++;
        }
        return sent_items;
    }  //_UART_DISABLE_ALL_FLAG
}

/// @brief Threadsafe uart send for sending 32 bits
uint32_t __attribute__((section(".__sect_ps_txt"))) UART_Send_u32(char *txbuf, uint32_t buflen)
{
    uint8_t byte_index = 4;
    if (nt_socpm_uart_flag_state_get(APP_MODE_SEL)) {
        __asm volatile(" nop	\n");
        (void)txbuf;
        buflen = 0;
        return buflen;
    } else {  //_UART_DISABLE_ALL_FLAG
        uint32_t /*length=buflen,*/ sent_items = 0;
        while (/*length*/ buflen) {
            buflen -= 4;
            taskENTER_CRITICAL();
            while (byte_index--) {
                myputchar((uint8_t) * /*data*/ txbuf++);
                sent_items++;
            }
            taskEXIT_CRITICAL();
            byte_index = 4;
        }
        return sent_items;
    }  //_UART_DISABLE_ALL_FLAG
}

size_t FreeRTOS_UART_read(uint8_t *const pvBuffer, const size_t xBytes)
{
    if ((uart_flag == 1) && xBytes) {
        *pvBuffer = (uint8_t)_uart_rx_buf[_uart_rx_buf_idx++];
        uart_flag = 0;

        return 1;
    } else {
        *pvBuffer = 0;
    }
    return 0;
}

/*uint32_t UART_Receive(uint8_t *rxbuf, uint32_t buflen)
{
    uint32_t bToRecv, bRecv, timeOut;
    uint8_t *pChar = rxbuf;

    bToRecv = buflen;
    bRecv = 0;
            while (bToRecv) {
                if (!(ReadRegister(SYS_UART_LSR)&1)) {
                    __asm volatile(" nop					\n");
                    break;
                } else {
                    (*pChar++) = (ReadRegister(SYS_UART_THR));
                    bRecv++;
                    bToRecv--;
                }
            }
    return bRecv;
}*/
/*
size_t xIOUtilsReceiveCharsFromRxQueue( Peripheral_Control_t * const pxPeripheralControl, uint8_t * const pucBuffer,
const size_t xTotalBytes )
{
    size_t xBytesReceived = 0U;
    portTickType xTicksToWait;
    xTimeOutType xTimeOut;
    Character_Queue_State_t *pxTransferState = prvRX_CHAR_QUEUE_STATE( pxPeripheralControl );
    xTicksToWait = pxTransferState->xBlockTime;
        vTaskSetTimeOutState( &xTimeOut );
        //Are there any more bytes to be received?
        while( xBytesReceived < xTotalBytes )
        {
            // Receive the next character.
            if( xQueueReceive( pxTransferState->xQueue, &( pucBuffer[ xBytesReceived ] ), xTicksToWait ) == pdPASS )
            {
                xBytesReceived++;
            }

            if( xTaskCheckForTimeOut( &xTimeOut, &xTicksToWait ) != pdFALSE )
            {
                // Time out has expired.
                break;
            }
        }

        return xBytesReceived;



}
void xIOUtilsClearRxCharQueue( Peripheral_Control_t * const pxPeripheralControl )
{
Character_Queue_State_t *pxCharQueueState = prvTX_CHAR_QUEUE_STATE( pxPeripheralControl );

    configASSERT( pxCharQueueState );
    xQueueReset( pxCharQueueState->xQueue );
}

*/

void __attribute__((section(".__sect_ps_txt"))) UART_Recieve_buff(void)
{
    if (nt_socpm_uart_flag_state_get(APP_MODE_SEL)) {
        __asm volatile(" nop	\n");
    } else {  //_UART_DISABLE_ALL_FLAG
#if defined(NT_NOR_CLI) || defined(CONFIG_NT_RCLI) || CONFIG_QCCSDK_CONSOLE || defined(FTM_OVER_UART)
        char data = 0;
        BaseType_t wakeup_task = pdFALSE;
#ifdef FTM_OVER_UART
        extern TaskHandle_t FTM_Task_handle;
#else
#if defined NT_NOR_CLI
        extern xTaskHandle xCommandConsoleTask;
#endif

#if (defined CONFIG_NT_RCLI)
        extern xTaskHandle xRCLICommandConsoleTask;
#endif

#if CONFIG_QCCSDK_CONSOLE
        extern TaskHandle_t QCLI_Task_handle;
#endif
#endif
        /* WR: Case for external wakup from deepsleep */
        HW_REG_RD(QWLAN_UART_UART_USR_REG);
        // read THR register
        data = (char)NT_HW_RD(QWLAN_UART_UART_RBR_REG);
#ifdef FTM_OVER_UART
        if (cInputIndex < cmdMAX_INPUT_SIZE) {
            cInputString[cInputIndex] = data;
            cInputIndex++;
        }
        if (data == 0x7E)  // terminate char
        {
            cInputLength = cInputIndex;
            cInputIndex = 0;
            xTaskNotifyFromISR(FTM_Task_handle, data, eSetBits, &wakeup_task);
        }
#else
#if defined NT_NOR_CLI
        if (xCommandConsoleTask)
            xTaskNotifyFromISR(xCommandConsoleTask, data, eSetBits, &wakeup_task);
#endif

#if (defined CONFIG_NT_RCLI)
        if (data != '\n' && cInputIndex < cmdMAX_INPUT_SIZE) {
            cUartInputString[cInputIndex] = data;
            cInputIndex++;
        } else {
            cUartInputString[cInputIndex] = '\0';
            cInputIndex = 0;
            xTaskNotifyFromISR(xRCLICommandConsoleTask, (1 << RCLI_SIGNAL_UART_MSG), eSetBits, &wakeup_task);
        }
#endif

#if CONFIG_QCCSDK_CONSOLE
        if (QCLI_Task_handle)
            xTaskNotifyFromISR(QCLI_Task_handle, data, eSetBits, &wakeup_task);
#endif
#endif
        portYIELD_FROM_ISR(wakeup_task);
#endif
    }  //_UART_DISABLE_ALL_FLAG
}

/** INTERRUPT SERVICE ROUTINES **/
void __attribute__((section(".__sect_ps_txt"))) uart_irq_handler(void)
{
    PROF_IRQ_ENTER();
    extern int process_routine;
    extern int process_uart_rx_irq;
    if (process_routine == 0 && process_uart_rx_irq) {
        UART_Recieve_buff();
        uart_flag = 1;
    }
    PROF_IRQ_EXIT();
}

void uart_set_baud_rate(uint32_t baud_rate)
{
    _uart_baud_rate = baud_rate;
    uart_init();
}

void nt_systick_ms_delay(uint8_t dly_t)
{
    uint32_t cur_ticks;
    cur_ticks = (qurt_timer_get_ticks() + dly_t);
    while (qurt_timer_get_ticks() > cur_ticks)
        ;
}
#if (NT_CHIP_VERSION == 2)
void uart_data_status(void)
{
    uint32_t value;
    value = NT_REG_RD(QWLAN_UART_UART_LCR_REG);
    if ((value & UART_DLS_MASK) == 0x0) {
        NT_LOG_PRINT(COMMON, INFO, "5 bit data");
    } else if ((value & UART_DLS_MASK) == 0x1) {
        NT_LOG_PRINT(COMMON, INFO, "6 bit data");
    } else if ((value & UART_DLS_MASK) == 0x2) {
        NT_LOG_PRINT(COMMON, INFO, "7 bit data");
    } else if ((value & UART_DLS_MASK) == 0x3) {
        NT_LOG_PRINT(COMMON, INFO, "8 bit data");
    }
}

void uart_stop_bit_status(void)
{
    uint32_t value;
    value = NT_REG_RD(QWLAN_UART_UART_LCR_REG);
    if ((value & QWLAN_UART_UART_LCR_STOP_MASK) == 0x0) {
        NT_LOG_PRINT(COMMON, INFO, "1 stop bit ");
    } else if ((value & QWLAN_UART_UART_LCR_STOP_MASK) == 0x4) {
        NT_LOG_PRINT(COMMON, INFO, "1.5 stop bits");
    } else if ((value & UART_DLS_MASK) == 0x0) {
        NT_LOG_PRINT(COMMON, INFO, "2 stop bits ");
    }
}

void uart_parity_status(void)
{
    uint32_t value;
    value = NT_REG_RD(QWLAN_UART_UART_LCR_REG);
    if ((value & QWLAN_UART_UART_LCR_PEN_MASK) != 0x8) {
        NT_LOG_PRINT(COMMON, INFO, "NONE parity (parity disabled) ");
    } else {
        NT_LOG_PRINT(COMMON, INFO, "parity enabled ");
    }
}
#endif  //(NT_CHIP_VERSION==2)
