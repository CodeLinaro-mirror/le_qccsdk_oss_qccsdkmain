/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/** 
    @file  qapi_uart.c
    @brief UART QAPI implementation
 */



/*==================================================================================================
                                           INCLUDE FILES
==================================================================================================*/
#include <string.h>
#include <stdlib.h>
#include "qapi_uart.h"
#include "ferm_uart.h"
#include "nt_osal.h"

#ifdef UART_QAPI

/*==================================================================================================
                                          GLOBAL FUNCTIONS
==================================================================================================*/
/*==================================================================================================

FUNCTION: qapi_UART_Close

DESCRIPTION:

==================================================================================================*/
qapi_Status_t qapi_UART_Close
(  
	qapi_UART_Instance_t	instance
)
{
	uart_status u_status;
   
	u_status = uart_close((uart_instance)instance);

	return UART_ErrorMap(u_status);
}


/*==================================================================================================

FUNCTION: qapi_UART_Open

DESCRIPTION:

==================================================================================================*/
qapi_Status_t qapi_UART_Open
(
	qapi_UART_Instance_t		instance,
	qapi_UART_Open_Config_t		*config
)
{
	uart_status u_status;

    u_status = uart_open((uart_instance)instance, (uart_config *)config);

	return UART_ErrorMap(u_status);
}

/*==================================================================================================

FUNCTION: qapi_UART_Receive

DESCRIPTION:

==================================================================================================*/
qapi_Status_t qapi_UART_Receive
(
	qapi_UART_Instance_t		instance,
	char						*buf, 
	uint32_t					buf_Size, 
	uint32_t					*recved
)
{
	uart_status u_status;
	
	u_status = uart_receive((uart_instance)instance, (uint8_t *)buf, buf_Size, recved);

	return UART_ErrorMap(u_status);
}

/*==================================================================================================

FUNCTION: qapi_UART_Open_With_Rx_Timeout

DESCRIPTION:

==================================================================================================*/
qapi_Status_t qapi_UART_Open_With_Rx_Timeout
(
	qapi_UART_Instance_t		instance,
	qapi_UART_Open_Config_t		*config,
	uint32_t					timeout
)
{
	uart_status u_status;

    u_status = uart_open_with_rx_timeout((uart_instance)instance, (uart_config *)config, timeout);

	return UART_ErrorMap(u_status);
}


/*==================================================================================================

FUNCTION: qapi_UART_Transmit

DESCRIPTION:

==================================================================================================*/
qapi_Status_t qapi_UART_Transmit
(
	qapi_UART_Instance_t		instance,
	char						*buf, 
	uint32_t					bytes_To_Tx, 
	uint32_t					*sent
)
{
	uart_status u_status;
	
	u_status = uart_transmit((uart_instance)instance, (uint8_t *)buf, bytes_To_Tx, sent);
	
	return UART_ErrorMap(u_status);
}
#endif
