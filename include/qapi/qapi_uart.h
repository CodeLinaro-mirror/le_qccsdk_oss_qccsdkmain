/*
* Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#ifndef __QAPI_UART_H__
#define __QAPI_UART_H__

#include "qapi_types.h"
#include "qapi_status.h"

#define QAPI_I2CM_ERROR                                          __QAPI_ERROR(QAPI_MOD_I2C, 1)   /**< Common error */

/**UART Error code Mapping*/
#define UART_SUCCESS(x)  (x == QAPI_OK)
#define UART_ERROR(x)    (x != QAPI_OK)

#define UART_ErrorMap(Status)           (Status == QAPI_OK) ? QAPI_OK : __QAPI_ERROR(QAPI_MOD_UART, Status)

#define QAPI_UART_ERROR													__QAPI_ERROR(QAPI_MOD_UART, 1)   /**< Common error */
#define QAPI_UART_ERROR_NULL_PTR_ERROR                                  __QAPI_ERROR(QAPI_MOD_UART, 2)   /**< NULL pointer error */
#define QAPI_UART_ERROR_INVALID_PARAM									__QAPI_ERROR(QAPI_MOD_UART, 3)   /**< Invalid parameter error */
#define	QAPI_UART_ERROR_CFG_PARAM										__QAPI_ERROR(QAPI_MOD_UART, 4)   /**< Configuration error */	
#define QAPI_UART_ERROR_BAUDRATE_CFG									__QAPI_ERROR(QAPI_MOD_UART, 5)   /**< Baudrate confiuration err */
#define QAPI_UART_ERROR_SEND_BUSY										__QAPI_ERROR(QAPI_MOD_UART, 6)   /**< Send busy err */
#define QAPI_UART_ERROR_RECV_BUSY										__QAPI_ERROR(QAPI_MOD_UART, 7)   /**< Receive busy err */
#define QAPI_UART_ERROR_TX_ENQUEUE_SEM_SYNC								__QAPI_ERROR(QAPI_MOD_UART, 8)   /**< Tx enqueue sem sync err */
#define QAPI_UART_ERROR_TX_ENQUEUE_FULL									__QAPI_ERROR(QAPI_MOD_UART, 9)   /**< Tx enqueue full err */
#define QAPI_UART_ERROR_TX_DEQUEUE_EMPTY								__QAPI_ERROR(QAPI_MOD_UART, 10)	 /**< Tx dequeue empty err */
#define QAPI_UART_ERROR_RX_ENQUEUE_FULL									__QAPI_ERROR(QAPI_MOD_UART, 11)  /**< Rx enqueue full err */
#define QAPI_UART_ERROR_RX_DEQUEUE_SEM_SYNC								__QAPI_ERROR(QAPI_MOD_UART, 12)	 /**< Rx dequeue sem err */
#define QAPI_UART_ERROR_RX_DEQUEUE_EMPTY								__QAPI_ERROR(QAPI_MOD_UART, 13)	 /**< Rx dequeue empty err */
#define QAPI_UART_ERROR_TRANSFER_TIMEOUT								__QAPI_ERROR(QAPI_MOD_UART, 14)	 /**< Transfer timeout err */
#define QAPI_UART_ERROR_INPUT_FIFO_UNDER_RUN							__QAPI_ERROR(QAPI_MOD_UART, 15)	 /**< Input fifo under run err */
#define QAPI_UART_ERROR_INPUT_FIFO_OVER_RUN								__QAPI_ERROR(QAPI_MOD_UART, 16)	 /**< Input fifo over run err */
#define QAPI_UART_ERROR_OUTPUT_FIFO_UNDER_RUN							__QAPI_ERROR(QAPI_MOD_UART, 17)	 /**< Output fifo under run err */
#define QAPI_UART_ERROR_OUTPUT_FIFO_OVER_RUN							__QAPI_ERROR(QAPI_MOD_UART, 18)	 /**< Output fifo over run err */
#define QAPI_UART_ERROR_TRANSFER_FORCE_TERMINATED						__QAPI_ERROR(QAPI_MOD_UART, 19)	 /**< Transfer force terminated */
#define QAPI_UART_ERROR_BUS_CLK_CFG_FAIL								__QAPI_ERROR(QAPI_MOD_UART, 20)	 /**< Bus clock configuration failure */
#define QAPI_UART_ERROR_BUS_GPIO_ENABLE_FAIL							__QAPI_ERROR(QAPI_MOD_UART, 21)	 /**< Bus GPIO Enable failire */
#define QAPI_UART_ERROR_CANCEL_TRANSFER_FAIL							__QAPI_ERROR(QAPI_MOD_UART, 22)	 /**< Cancel transfer failure */
#define QAPI_UART_ERROR_BOOTSTRAP_CFG_FAIL								__QAPI_ERROR(QAPI_MOD_UART, 23)  /**< Bootstrap configuration failure */
#define QAPI_UART_ERROR_DEVICE_STATE									__QAPI_ERROR(QAPI_MOD_UART, 23)  /**< Device state invalid for the operation */
/*
 */
// $QTI_LICENSE_QDN_C$

/**
 * @file qapi_uart.h
 */

/*==================================================================================================
                                            DESCRIPTION
====================================================================================================

GLOBAL FUNCTIONS:
   qapi_UART_Close
   qapi_UART_Open
   qapi_UART_Receive
   qapi_UART_Transmit
   qapi_UART_Open_With_Rx_Timeout

==================================================================================================*/

/*==================================================================================================
                                           INCLUDE FILES
										   utput/qcc730v2_evb11_hostless/FERMION_IOE_QCLI_DEMO/DEBUG/autoconf.
==================================================================================================*/


/*==================================================================================================
                                             ENUMERATIONS
==================================================================================================*/

/** @addtogroup qapi_uart
@{ */

/**
 * UART port ID enumeration. 
 *  
 * Enumeration to specify which port is to be opened during the uart_open 
 * call.
 */
typedef enum
{
   QAPI_UART_INST_0 = 0, /**< port0. */
   QAPI_UART_MAX_INST_E,
}qapi_UART_Instance_t;


/**
 * Enumeration to specify how many UART bits are to be used per character configuration.
 *  
 */
typedef enum
{
  QAPI_UART_5_BITS_PER_CHAR_E  = 0,  /**< 5 bits per character. Due to hardware limitation, currently it is not support */
  QAPI_UART_6_BITS_PER_CHAR_E  = 1,  /**< 6 bits per character. Due to hardware limitation, currently it is not support */
  QAPI_UART_7_BITS_PER_CHAR_E  = 2,  /**< 7 bits per character. Due to hardware limitation, currently it is not support */
  QAPI_UART_8_BITS_PER_CHAR_E  = 3,  /**< 8 bits per character. */
} qapi_UART_Bits_Per_Char_e;

/**
 * Enumeration for the UART number of stop bits configuration.  
 *   
 */
typedef enum
{
  QAPI_UART_1_0_STOP_BITS_E    = 0,  /**< 1.0 stop bit. */
  QAPI_UART_1_5_OR_2_0_STOP_BITS_E    = 1,  /**< 1.5 stop bits for 5 bits data, otherwise 2.0 stop bits. */
} qapi_UART_Num_Stop_Bits_e;

/**
 * Enumeration for the UART parity mode configuration.  
 *   
 */
typedef enum
{
  QAPI_UART_NO_PARITY_E        = 0,  /**< No parity. */ 
  QAPI_UART_ODD_PARITY_E       = 1,  /**< Odd parity. */
  QAPI_UART_EVEN_PARITY_E     = 2,   /**< Even parity. */
} qapi_UART_Parity_Mode_e;

/** @} */ /* end_addtogroup qapi_uart */

/** @addtogroup qapi_uart
@{ */

/** Structure for UART configuration. */
typedef struct
{
   uint32_t                     baud_Rate; /**< Baud rates */
   qapi_UART_Parity_Mode_e      parity_Mode; /**< Parity mode. */
   qapi_UART_Bits_Per_Char_e    bits_Per_Char; /**< Bits per character. */
   qapi_UART_Num_Stop_Bits_e    num_Stop_Bits; /**< Number of stop bits. */
   qbool_t                      enable_Loopback; /**< Enable loopback. */
}qapi_UART_Open_Config_t;

/** @} */ /* end_addtogroup qapi_uart */


/** @addtogroup qapi_uart
@{ */

/*==================================================================================================
                                        FUNCTION PROTOTYPES
==================================================================================================*/

/**
 * Closes the UART port. Not to be called from ISR context.
 *
 * Releases clock, interrupt, and GPIO handles related to this UART and
 *          cancels any pending transfers.
 * 
 * @note1hang Do not call this API from ISR context.
 * 
 * @param[in] handle      UART handle provided by qapi_UART_Open().
 * 
 * @return
 * QAPI_OK      Port close successful. \n
 * QAPI_ERR_XX   Port close failed, for err code refer the api_Status_t.
 * 
 */

qapi_Status_t qapi_UART_Close
(  
   qapi_UART_Instance_t		instance
);

/**
 * Initializes the UART port. Not to be called from ISR context.
 *
 * Opens the UART port and configures the corresponding clocks, interrupts, and GPIO.
 * 
 * @note1hang Do not call this API from ISR context.
 * 
 * @param[in] id ID of the port to be opened.
 * @param[in] config Structure that holds all configuration data.
 *  
 * @return 
 * QAPI_OK      Port open successful. \n
 * QAPI_ERR_XX   Port open failed, for err code refer the api_Status_t.
 *  
 *  
 */

qapi_Status_t qapi_UART_Open
(
   qapi_UART_Instance_t		instance,
   qapi_UART_Open_Config_t*   config
);

/**
 * Queues the buffer provided for receiving the data. Not to be called from ISR context.
 *
 * This is an synchronous call. Return when the transaction is done.
 * 
 * @newpage           
 * Call uart_receive immediately after uart_open to queue a buffer.
 * 
 * @note1hang Do not call this API from ISR context.
 * 
 * @param[in] handle       UART handle provided by qapi_UART_Open().
 * @param[in] buf          Buffer to be filled with data.
 * @param[in] buf_Size     Buffer size. Must be @ge 4 and a multiple of 4. 
 * @param[in] recved       Callback data to be passed when rx_cb_isr is called 
 *                         during Rx completion.
 *  
 * @return 
 * QAPI_OK		 Recieve transcation was successful. \n
 * QAPI_ERR_XX   Receive transcation has error, for err code refer the api_Status_t.
 *  
 */

qapi_Status_t qapi_UART_Receive
(
   qapi_UART_Instance_t		instance,
   char*					buf, 
   uint32_t					buf_Size, 
   uint32_t*				recved
);

/**
 * Transmits data from a specified buffer. Not to be called from ISR context.
 *
 * This is an synchronous call. Return when transmit is completed.
 * 
 * The buffer is owned by the UART driver until the call return.
 * 
 * @note1hang Do not call this API from ISR context.
 *
 * @param[in] handle         UART handle provided by qapi_UART_Open().
 * @param[in] buf            Buffer with data for transmit. 
 * @param[in] bytes_To_Tx    Bytes of data to transmit
 * @param[in] sent			 Bytes of data sent
 *  
 * @return 
 * QAPI_OK      Transmit was successful. \n
 * QAPI_ERROR   Transmit buffer failed, for err code refer the api_Status_t. 
 *
 */

qapi_Status_t qapi_UART_Transmit
(
   qapi_UART_Instance_t		instance,
   char*					buf, 
   uint32_t					bytes_To_Tx, 
   uint32_t*				sent
);

/**
 * Initializes the UART port. Not to be called from ISR context.
 *
 * Opens the UART port and configures the corresponding clocks, interrupts, and GPIO with Rx timeout.
 * 
 * @note1hang Do not call this API from ISR context.
 * 
 * @param[in] id ID of the port to be opened.
 * @param[in] config Structure that holds all configuration data.
 * @param[in] config timeout of Rx thread
 
 * @return 
 * QAPI_OK      Port open successful. \n
 * QAPI_ERR_XX   Port open failed, for err code refer the api_Status_t.
 *  
 *  
 */

qapi_Status_t qapi_UART_Open_With_Rx_Timeout
(
   qapi_UART_Instance_t		instance,
   qapi_UART_Open_Config_t*   config,
   uint32_t 				timeout
);

/** @} */ /* end_addtogroup qapi_uart */

#endif
