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

#include "qapi_types.h"
#include "qapi_status.h"
#include "qapi_uart.h"

#include <qat_api.h>
#include <qat.h>
#include "nt_osal.h"


/*-------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 *-----------------------------------------------------------------------*/
//#define UART_HTC_INSTANCE QAPI_UART_INSTANCE_SE2_E
#define UART_HTC_INSTANCE QAPI_UART_INST_0	
extern void* QCLI_Context;

//#define QAT_LOG_ENABLE
#ifdef QAT_LOG_ENABLE
#define UART_DBG_PRINTF(Fmt, ...) QCLI_Printf(&QCLI_Context, Fmt , ##__VA_ARGS__)

#else
#define UART_DBG_PRINTF(x, ...)
#endif
#define UART_PRINTF(Fmt, ...)     QCLI_Printf(&QCLI_Context, Fmt , ##__VA_ARGS__)


#define UART_TSK_EVENT_INIT        (1) /**< Task event init */
#define UART_TSK_EVENT_RX          (2) /**< Task event indicating the UART rx event */

#define QAPI_UART_INSTANCE_MAX     QAPI_UART_MAX_INST_E

#define UART_INSTANCE_VAVLID(instance) (instance >=0 && instance < QAPI_UART_INSTANCE_MAX)

#define INSTANCE_2_UART(instance) (UART_INSTANCE_VAVLID(instance)? & GUartCtrl[instance] : NULL)

#define IN_RANGE(val, min, max) (val >=min && val <= max)
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

#define UART_INPUT_9BIT_IS_1_CHARACTER                     ("1\1")
#define UART_INPUT_9BIT_IS_0_CHARACTER                     ("0\0")

#define UART_CMD_CONSTDATA(Num)      Num, Num, Num, Num, Num, Num, Num, Num, Num, Num
#define UART_CMD_BUFLEN_MIN          1
#define UART_CMD_BUFLEN_MAX          1000
#define UART_CMD_PATTERN             0x76
#define UART_TX_MAX_LEN 			 128
#define UART_RECV_MAX_LEN		     UART_TX_MAX_LEN
#define UART_RECV_TASK_TIMEOUT	     500 //ms
//#define UART_HTC_BUFFER_HEADER_SIZE sizeof(UART_HTC_BUFFER_HEADRT_t)  // ???? this need be adajust.

#define INPUT_BUFFER_SIZE        1024
unsigned char Uart_Rcv_Buff[INPUT_BUFFER_SIZE];

#ifdef PAL_USE_RTT_CONSOLE
#define QAT_TSK_WAIT_TIME       (50)
#else
#define QAT_TSK_WAIT_TIME       (QAPI_TSK_INFINITE_WAIT)
#endif
nt_osal_task_handle_t  qat_uart_rx_task_hdl = NULL;


/*-------------------------------------------------------------------------
 * Type Declarations
 *-----------------------------------------------------------------------*/

typedef enum{
    WORK_MODE_SILENT,
    WORK_MODE_ECHO,
    WORK_MODE_9BITS_HW_M,
    WORK_MODE_9BITS_HW_S,
    WORK_MODE_9BITS_SW_M,
    WORK_MODE_9BITS_SW_S,
    WORK_MODE_9BITS_SW_DUAL,
    WORK_MODE_9BITS_HW_DUAL,
    WORK_MODE_TX_REVERT,
    WORK_MODE_IDLE
}WORK_MODE_E;

typedef struct UART_s
{
    qapi_UART_Instance_t      Instance;
    qbool_t                   Enabled;
    WORK_MODE_E               WorkMode;

    uint32_t                  BaudRate;
    qapi_UART_Parity_Mode_e   ParityMode;
    qapi_UART_Num_Stop_Bits_e NumStopBits;
    qapi_UART_Bits_Per_Char_e BitsPerChar;
    qbool_t                   EnableLoopback;
    qbool_t                   EnableFlowCtrl;
    qbool_t                   DmaEnable;
    qbool_t                   TxInvert;
    uint32_t                  BufferSize;
    uint32_t                  RxBufferSize;
    uint8_t                   TransmitAddr;

    char*                     UartBuf;          /*< app buffer size keep same with  driver buffer size */
    uint32_t                  UartBufContentLen;           /*< used for general work mode */
    uint32_t                  StrLen;           /*< used for 9 bit work mode */
    uint64_t                  TxBytes;
    uint64_t                  RxBytes;
} UART_t;

/*-------------------------------------------------------------------------
 * Static & global Variable Declarations
 *-----------------------------------------------------------------------*/
extern int32_t PAL_Uart_Instance_Get(void);
extern qbool_t QAT_Process_Input_Data(uint32_t Length, char *Buffer);
extern qbool_t QAT_Initialize(void);
static UART_t GUartCtrl[QAPI_UART_INSTANCE_MAX];
//static int32_t ConsoleUartInstance = -1;

 /*-------------------------------------------------------------------------
 * External Function declaration
 *-----------------------------------------------------------------------*/


 /*-------------------------------------------------------------------------
 * Function declaration
 *-----------------------------------------------------------------------*/
//qbool_t Initialize_Uart_Htc(qapi_UART_Instance_t Instance);
qapi_Status_t QAT_Output(uint32_t length, const char *buffer);

/* Internal help functions. */
static qbool_t UartRxTaskStart(qapi_UART_Instance_t Instance);
static void UartRxTasks(void *Task_Uart);
static qbool_t UartEnabled(qapi_UART_Instance_t Instance);

static qapi_Status_t UartInit(qapi_UART_Instance_t Instance, uint32_t BaudRate, qapi_UART_Parity_Mode_e ParityMode,
    qapi_UART_Num_Stop_Bits_e NumStopBits, qapi_UART_Bits_Per_Char_e BitsPerChar, qbool_t EnableLoopback, qbool_t EnableFlowCtrl,
    qbool_t DmaEnable, uint32_t BufferSize, uint32_t RxStaleCnt, qbool_t TxInvert);
//static qapi_Status_t UartDeinit(qapi_UART_Instance_t Instance);

/* Callback functions. */
//static void UartRxCbCommon(unsigned int Instance, uint32_t Status, unsigned int Length, void *CbCtxt);

qbool_t (*Process_Input_Data_Handle)(uint32_t Length, char *Buffer);


/*-------------------------------------------------------------------------
 * Function defination
 *-----------------------------------------------------------------------*/
qbool_t Uart_Initialize(qapi_UART_Instance_t Instance)
{
    qbool_t Ret_Val = true;
    
    printf("Open uart instance %d\n", Instance);
    if (QAPI_OK != UartInit(Instance, 57600, QAPI_UART_NO_PARITY_E, QAPI_UART_1_0_STOP_BITS_E, QAPI_UART_8_BITS_PER_CHAR_E, 
                0, 0, 1, INPUT_BUFFER_SIZE, 10, 0))
    {
        Ret_Val = false;
		return Ret_Val;
    }
    
    printf("Start uart %d task...\n", Instance);
    GUartCtrl[Instance].WorkMode = WORK_MODE_SILENT;//WORK_MODE_ECHO; //
    Ret_Val = UartRxTaskStart(Instance);
    
    return Ret_Val;
}
/*
qbool_t Deinitialize_Uart_Htc(qapi_UART_Instance_t Instance)
{
    if (!UartEnabled(Instance))
    {
        UART_PRINTF("ERROR: UART %d is not enabled.\n", Instance);
        return false;
    }

    if (GUartCtrl[Instance].WorkMode == WORK_MODE_IDLE)
    {
        UART_PRINTF("ERROR: Uart %d is in idle status.\n", Instance);
        return false;
    }

    UART_DBG_PRINTF("Stop task and disable UART %d.\n", Instance);
    GUartCtrl[Instance].WorkMode = WORK_MODE_IDLE;
    qapi_TSK_Delete_Task(GUartCtrl[Instance].UartTask);
    GUartCtrl[Instance].UartTask = NULL;
    UartDeinit(Instance);

    ConsoleUartInstance = PAL_Uart_Instance_Get();
    return true;
}
*/
static void UartRxTasks(void *Task_Uart)
{
    uint32_t Len = INPUT_BUFFER_SIZE, RemainLen = 0;
    UART_t *Uart;
    qapi_Status_t Status = QAPI_OK;
	uint32_t DataLen, Recved = 0, i;
	uint32_t Total = 0;
	uint8_t end_char_found = 0;
	char *Uart_Rcv_Buff = NULL;

	Uart = (UART_t *)Task_Uart;
	if (NULL == Uart)
    {
        /*assert*/
        printf("UART Instance %d does not exist.\n", Uart->Instance);
        return ;
    }
    
	while(1)
	{
		end_char_found = 0;
		
		Uart_Rcv_Buff = (char *)nt_osal_allocate_memory(INPUT_BUFFER_SIZE);
		if (Uart_Rcv_Buff == NULL)
			return ;
		
		Status = qapi_UART_Receive(Uart->Instance, Uart_Rcv_Buff, INPUT_BUFFER_SIZE, &Recved);
		if (Recved > 0) {
			Total += Recved;
		
	        for(i = 0; i < Recved; i ++)
	        {
				if(Uart_Rcv_Buff[i] == PAL_INPUT_END_OF_LINE_CHARACTER)
					end_char_found = 1;
	        }
			if ((i % UART_RECV_MAX_LEN) == 0) {
				printf("\r\n");
			}

            printf("%c", Uart_Rcv_Buff[i]);
			//sprintf("\r\n");
			if(Process_Input_Data_Handle && end_char_found == 1)
				    Process_Input_Data_Handle(Len, Uart_Rcv_Buff);
	    }
		
		nt_osal_free_memory((char*)Uart_Rcv_Buff);
	}
}


static qbool_t UartRxTaskStart(qapi_UART_Instance_t Instance)
{
    UART_t *Uart = NULL;
	uint32_t ret_val;
	
	
    Uart = INSTANCE_2_UART(Instance);
    if (NULL == Uart)
    {
        return false;
    }
    ret_val =  (uint32_t)nt_qurt_thread_create(UartRxTasks, "UartRxTasks", 1024, Uart, 6, &qat_uart_rx_task_hdl);
    if(ret_val != pdPASS)
    {
   	 printf("QAT: task creation failed out of memory\r\n");
   	 ASSERT(0);
    } 
}

static qapi_Status_t UartInit(qapi_UART_Instance_t Instance, uint32_t BaudRate, qapi_UART_Parity_Mode_e ParityMode,
    qapi_UART_Num_Stop_Bits_e NumStopBits, qapi_UART_Bits_Per_Char_e BitsPerChar, qbool_t EnableLoopback, qbool_t EnableFlowCtrl,
    qbool_t DmaEnable, uint32_t BufferSize, uint32_t RxStaleCnt, qbool_t TxInvert)
{
    UART_t *Uart = NULL;
    qapi_UART_Open_Config_t Config;
    qapi_Status_t Result;

    Uart = INSTANCE_2_UART(Instance);
    if (NULL == Uart)
    {
        return QAPI_ERR_INVALID_PARAM;
    }

    if (Uart->Enabled)
    {
        return QAPI_ERR_INVALID_PARAM;
    }

    memset(Uart, 0, sizeof(UART_t));

    Uart->Instance= Instance;

    if(DmaEnable)
    {
        Uart->RxBufferSize = BufferSize>>1;
    }
    else
    {
        Uart->RxBufferSize = BufferSize;
    }

    if((Uart->UartBuf = malloc(Uart->RxBufferSize)) == NULL)
    {
        return QAPI_ERR_NO_MEMORY;
    }

    memset(&Config, 0, sizeof(qapi_UART_Open_Config_t));

	Config.baud_Rate = Uart->BaudRate = BaudRate;
	Config.parity_Mode = Uart->ParityMode = ParityMode;
	Config.bits_Per_Char = Uart->BitsPerChar = BitsPerChar;
	Config.num_Stop_Bits = Uart->NumStopBits = NumStopBits;
	Config.enable_Loopback = 0;

    Result = qapi_UART_Open_With_Rx_Timeout(Uart->Instance, &Config, UART_RECV_TASK_TIMEOUT);
    if (Result != QAPI_OK)
    {
        goto fail;
    }

    Uart->Enabled = true;
    Uart->WorkMode = WORK_MODE_IDLE;
    return QAPI_OK;

fail:
    return Result;
}

static qapi_Status_t UartDeinit(qapi_UART_Instance_t Instance)
{
    UART_t *Uart = NULL;

    Uart = INSTANCE_2_UART(Instance);
    if (NULL == Uart)
    {
        return QAPI_ERR_INVALID_PARAM;
    }

    if (!Uart->Enabled)
    {
        return QAPI_OK;
    }

    if(Uart->UartBuf)
    {
        qapi_Free(Uart->UartBuf);
    }
    memset(Uart, 0, sizeof(UART_t));
    return(qapi_UART_Close(Instance));
}

static qbool_t UartEnabled(qapi_UART_Instance_t Instance)
{
    UART_t *Uart = NULL;

    Uart = INSTANCE_2_UART(Instance);
    if (NULL == Uart)
    {
        return false;
    }

    return Uart->Enabled;
}

qapi_Status_t QAT_Output(uint32_t Length, const char *Buffer)
{
    uint32_t Remain = 0, Sent = 0, Offset = 0;

    if((Length != 0) && (Buffer != NULL))
    {
        Remain = Length;
        do
        {
            /* Transmit the data. */
            if(qapi_UART_Transmit(UART_HTC_INSTANCE, (char *)(Buffer + Offset), Remain, &Sent) == QAPI_OK)
            {
                Remain = Remain - Sent;
                Offset += Sent;
                GUartCtrl[UART_HTC_INSTANCE].TxBytes += Sent;
            }
            else
            {
                //uart transmit error
                ;
            }
        }while(Remain);
    }

    return QAPI_OK;
}

void Initialize_QAT_Main(void)
{
    Process_Input_Data_Handle = QAT_Process_Input_Data;

	QAT_Initialize();

    Uart_Initialize(UART_HTC_INSTANCE);

}

