/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*-------------------------------------------------------------------------
 * Include Files
 *-----------------------------------------------------------------------*/
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "qat.h"
#include "qat_spi.h"
#include "qapi_types.h"
#include "qapi_status.h"
#include "qurt_internal.h"
#include "nt_osal.h"

/*-------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 *-----------------------------------------------------------------------*/
#define SPI_TX_MAX_LEN 			 128
#define SPI_RECV_MAX_LEN		 SPI_TX_MAX_LEN
#define INPUT_BUFFER_SIZE        1024

/*-------------------------------------------------------------------------
 * Type Declarations
 *-----------------------------------------------------------------------*/

/*-------------------------------------------------------------------------
 * Static & global Variable Declarations
 *-----------------------------------------------------------------------*/

 /*-------------------------------------------------------------------------
 * External Function declaration
 *-----------------------------------------------------------------------*/
 extern qbool_t (*Process_Input_Data_Handle)(uint32_t Length, char *Buffer);


 /*-------------------------------------------------------------------------
 * Function declaration
 *-----------------------------------------------------------------------*/
void SPIRxTasks();


/*-------------------------------------------------------------------------
 * Function defination
 *-----------------------------------------------------------------------*/

void SPIRxTasks()
{
    uint32_t Len = INPUT_BUFFER_SIZE, RemainLen = 0;
    qapi_Status_t Status = QAPI_OK;
	uint32_t DataLen, Recved = 0, i;
	uint32_t Total = 0;
	uint8_t end_char_found = 0;
	char *SPI_Rcv_Buff = NULL;
    
	while(1)
	{
		end_char_found = 0;
		
		SPI_Rcv_Buff = (char *)nt_osal_allocate_memory(INPUT_BUFFER_SIZE);
		if (SPI_Rcv_Buff == NULL)
			return ;

		memset(SPI_Rcv_Buff, 0, INPUT_BUFFER_SIZE);
		
		//Status = qapi_UART_Receive(Uart->Instance, SPI_Rcv_Buff, INPUT_BUFFER_SIZE, &Recved);
		if (Recved > 0) {
			Total += Recved;
		
	        for(i = 0; i < Recved; i ++)
	        {
				if(SPI_Rcv_Buff[i] == PAL_INPUT_END_OF_LINE_CHARACTER)
					end_char_found = 1;
	        }
			if ((i % SPI_RECV_MAX_LEN) == 0) {
				printf("\r\n");
			}

            printf("%c", SPI_Rcv_Buff[i]);
			//sprintf("\r\n");
			if(Process_Input_Data_Handle && end_char_found == 1)
				    Process_Input_Data_Handle(Len, SPI_Rcv_Buff);
	    }
		
		nt_osal_free_memory((char*)SPI_Rcv_Buff);
		
	}
}

qbool_t SPI_Initialize()
{
    qbool_t Ret_Val = true;
	
    //leave for SPI initialization
	
    return Ret_Val;
}

void QAT_SPI_Output(uint32_t Length, const char *Buffer)
{
	char *Buffer_bk = NULL;
		
	Buffer_bk = (char *)nt_osal_allocate_memory(Length);
	
	if(!Buffer_bk)
	{	
	   return ;
	}
	
	memset((void*)Buffer_bk, 0, Length);
	
	if((Length != 0) && (Buffer != NULL))
    {
		memcpy(Buffer_bk, Buffer, Length);
		/*leave for send data to spi*/
	}

	nt_osal_free_memory((char *)Buffer_bk);
}

