
 /*-------------------------------------------------------------------------
* Include Files
*-----------------------------------------------------------------------*/

#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include "nt_hw.h"
#include "uart_print.h"
#include "nvm_common.h"

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
