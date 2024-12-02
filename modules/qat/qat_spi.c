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
#include "data_svc_hfc.h"
#include "qapi_hfc.h"

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

void SPIRxTasks(void *arg)
{
    uint32_t Len = INPUT_BUFFER_SIZE, RemainLen = 0;
    qapi_Status_t Status = QAPI_OK;
	uint32_t DataLen, Recved = 0, i;
	uint32_t Total = 0;
	uint8_t end_char_found = 0;
	char *SPI_Rcv_Buff = NULL;
	uint16_t buf_len = INPUT_BUFFER_SIZE;
	uint16_t data_len = 0;
	
    (void)(arg);

 	SPI_Rcv_Buff = (char *)nt_osal_allocate_memory(INPUT_BUFFER_SIZE);
	if (SPI_Rcv_Buff == NULL)
		return ;
		
    while (1) 
    {
	    if (qapi_hfc_recvfrom_host_data_pkt(SPI_Rcv_Buff, &buf_len, &data_len, NULL) == QAPI_OK)
		{		    
			*(SPI_Rcv_Buff + data_len) = '\r';
			printf("\r\nSPIRx cmd %d:", data_len);
			for (i=0; i<data_len; i++)
			{
			    printf("%c", SPI_Rcv_Buff[i]);
			}
			printf("\r\n");
			if(Process_Input_Data_Handle)
		        Process_Input_Data_Handle(data_len+1, SPI_Rcv_Buff);
			
		}		
    }
}

qbool_t SPI_Initialize()
{
    qbool_t Ret_Val = true;
	
    //leave for SPI initialization
	
    return Ret_Val;
}

#define HFC_SEND_MAX_RETRY_COUNT                 200

int QAT_SPI_Output(uint32_t Length, const char *Buffer)
{
	uint8_t *payload = NULL;
	int retry_cnt = 0;
	qapi_Status_t ret = QAPI_OK;
    uint32_t i;
	
	payload = (uint8_t *)nt_osal_allocate_memory(Length);
	
	if(!payload)
	{	
	   return ;
	}
	
	memset((void*)payload, 0, Length);
	
	while (retry_cnt < HFC_SEND_MAX_RETRY_COUNT)
	{
    	if((Length != 0) && (Buffer != NULL))
        {
    		memcpy(payload, Buffer, Length);
    		ret = qapi_hfc_sendto_host_data_pkt(payload, payload, Length, 0);
            if (0 == ret)
        	{
    			printf("\r\nSPITx resp %d: %s\r\n", Length, (char*)payload);
    			break;
    		} 
    
    		if (QAPI_ERR_NO_RESOURCE == ret)
    		{
    			/* Temp way to wait for the free elment, which
    			 * could be optimized to improve the speed. */
        		nt_osal_delay(10); 
    			retry_cnt++;
    			if (retry_cnt >= HFC_SEND_MAX_RETRY_COUNT)
    			{
    			    nt_osal_free_memory(payload);
    			}
        	}
    		else
    		{
    			 nt_osal_free_memory(payload);
    		     return -1;
    		}		
    	}
	}
}

