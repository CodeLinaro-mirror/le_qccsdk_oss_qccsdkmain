/******************************************************************************
 *
*Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
*SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 ******************************************************************************
 */
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <windows.h>
#include <process.h>
#include <Tlhelp32.h>
#include <io.h>
#include "tchar.h"


#include <stddef.h>
#include <winbase.h>
#include "serialport.h"
#include "Qcmbr.h"

DCB uartDCB;
COMMTIMEOUTS   CommTimeouts;
extern int char_interval;
int uartCreate(HANDLE *hPort, char *comPort)
{
    HANDLE fileHandler;
    unsigned char comStr[32] = {0};

    if ((NULL == hPort) || (NULL == comPort))
    {
        printf("COM Port or hPort is NULL\r\n");
        return -1;
    }
    snprintf(comStr, sizeof(comStr), "\\\\.\\com%s", comPort);

    printf("Open COM port %s len:%d\r\n", comStr, strlen(comStr));
	/* Create COM Port */
    fileHandler = CreateFileA(comStr,	     /* COM Port \\\\.\\COM12*/
				  GENERIC_READ | GENERIC_WRITE,  /* Read and Write */
			      0,
				  NULL,
				  OPEN_EXISTING,
				  FILE_ATTRIBUTE_NORMAL,
			      NULL);

	/* Open Failed will return*/
	if (fileHandler == INVALID_HANDLE_VALUE)
	{
		printf("COM Port Open Failed, fileHandler is INVALID_HANDLE_VALUE %d\r\n", GetLastError());
		return -1;
	}
    printf("Open COM port %s success!!!\r\n", comStr);


    *hPort = fileHandler;

    return 0;
}

int uartDcbSet(HANDLE hPort, int BaudRate, int ByteSize)
{
    if (NULL == hPort)
    {
        printf("Handle hPort is NULL\r\n");
		return -1;
    }

    /* Get COM Port Status and Confiure*/
    uartDCB.DCBlength = sizeof (DCB);
    GetCommState(hPort, &uartDCB);


    #if 1
    /* Configure COM Port Parameters*/
	//uartDCB.fBinary = TRUE;                         // Binary mode; no EOF check
	uartDCB.fParity = FALSE;                        // Enable parity checking
	uartDCB.fDsrSensitivity = FALSE;                // DSR sensitivity
	uartDCB.fErrorChar = FALSE;                     // Disable error replacement
	uartDCB.fOutxDsrFlow = FALSE;                   // No DSR output flow control
	uartDCB.fAbortOnError = FALSE;                  // Do not abort reads/writes on error
	uartDCB.fNull = FALSE;                          // Disable null stripping
	//uartDCB.fTXContinueOnXoff = TRUE;               // XOFF continues Tx
    uartDCB.fRtsControl = TRUE;
    uartDCB.fDtrControl = TRUE;
	#endif

    /* BaudRate|ByteSize|Parity|StopBits */
    uartDCB.BaudRate = BaudRate;
    uartDCB.ByteSize = 8;
    uartDCB.Parity   = NOPARITY;
    uartDCB.StopBits = ONESTOPBIT;
    uartDCB.fOutX = FALSE;
    uartDCB.fInX = FALSE;
    uartDCB.XonChar = 0x0;
    uartDCB.XoffChar = 0x0;
    uartDCB.XonLim = 0;
    uartDCB.XoffLim = 0;

    return 0;
}

int uartConfig(HANDLE hPort)
{
    if (NULL == hPort)
    {
        printf("Handle hPort is NULL\r\n");
		return -1;
    }

    /* Set COM Port in Driver */
    if (!SetCommState(hPort, &uartDCB))
    {
        printf("SetCommState Failed\r\n");
        CloseHandle(hPort);
        return -1;
    }

	return 0;
}

int uartSend(unsigned char *buf, int len, HANDLE hPort)
{
	DWORD sndBytes;
    int i;

    if (char_interval != 0)
    {
        for (i = 0; i< len; i++)
        {
            Sleep(char_interval);
            WriteFile(hPort, &buf[i], 1, &sndBytes, NULL);
            if(sndBytes<=0)
            {
                DbgPrintf("          single byte uartSend failed\r\n");
                return -1;
            }
        }
        DbgPrintf("          single byte uartSend success length:%d\r\n", len);
    }
    else
    {
        WriteFile(hPort, buf, len, &sndBytes, NULL);
	    if (sndBytes > 0)
	    {
		    DbgPrintf("          bulk uartSend success length:%d\r\n", sndBytes);
	    }
	    else
	    {
		    DbgPrintf("          bulk uartSend failed\r\n");
		    return -1;
	    }
    }
	return 0;
}

int uartRecv(unsigned char *buf, int len, HANDLE hPort)
{
	DWORD rcvBytes;
    BOOL fWaitingOnRead = FALSE;
    OVERLAPPED osReader = {0};
    unsigned long retlen=0;
    DWORD dwEvtMask;
    unsigned char uart_read_tmp_buf[MBUFFER];
    unsigned int index = 0;
    unsigned int total_length;

    memset(&uart_read_tmp_buf, 0, sizeof(uart_read_tmp_buf));

    osReader.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (osReader.hEvent == NULL)
       DbgPrintf("          Error in creating Overlapped event\r\n");

    DbgPrintf("          Into uartRecv\n");
    if (WaitCommEvent(hPort, &dwEvtMask, NULL))
    {
        DbgPrintf("          Catch Event, Mask:0x%x\n", dwEvtMask);
        if (dwEvtMask & EV_RXCHAR)
        {
            while (1)
            {
                memset(uart_read_tmp_buf, 0, sizeof(uart_read_tmp_buf));
                if (!ReadFile(hPort, uart_read_tmp_buf, len, &rcvBytes, NULL))
                {
                    DbgPrintf("          uartRecv failed err:%d\r\n", GetLastError());
                    return -1;
                }
                if (rcvBytes > 0)
                {
                    /* 0x7E is terminationChar */
                    if (uart_read_tmp_buf[rcvBytes-1] == 0x7E)
                    {
                        if (0 != index)
                        {
                            memcpy(&buf[index], uart_read_tmp_buf, rcvBytes);
                            index += rcvBytes;
                            total_length = index;
                            index = 0;
                            DbgPrintf("          uartRecv success super long length:%d\r\n", rcvBytes);
                            return (int)total_length;
                        }
                        else
                        {
                            memcpy(buf, uart_read_tmp_buf, rcvBytes);
                            DbgPrintf("          uartRecv success length:%d\r\n", rcvBytes);
                            return (int)rcvBytes;
                        }
                    }
                    else
                    {
                        memcpy(&buf[index], uart_read_tmp_buf, rcvBytes);
                        index += rcvBytes;
                    }
                }
                else
                {
                    DbgPrintf("          uartRecv failed length:%d  err:%d\r\n", rcvBytes, GetLastError());
                    return 0;
                }
            }
        }

        if (dwEvtMask & EV_CTS)
        {
            // To do.
        }
    }
    else
    {
        DWORD dwRet = GetLastError();
        if (ERROR_IO_PENDING == dwRet)
        {
            printf("I/O is pending...\n");

            // To do.
        }
        else
            printf("Wait failed with error %d.\n", GetLastError());
    }

#if 0
    if (!fWaitingOnRead)
    {
       DbgPrintf("Read File Start\r\n");
   	   /*Read Data from UART*/
       if (!ReadFile(hPort, buf, len, &rcvBytes, NULL))
       {
           printf("Read DATA failed %d\r\n", GetLastError());
       }
    }
 //   printf("Read DATA Success length:%d\r\n", rcvBytes);
	if (rcvBytes > 0)
	{
        DbgPrintf("555555Read DATA Success length:%d\r\n", rcvBytes);
		return (int)rcvBytes;
	}
    else
    {
        DbgPrintf("Read DATA failed length:%d  err:%d\r\n", rcvBytes, GetLastError());
        return 0;

    }
#endif
	return 0;
 }

int uartClearBuffer(HANDLE hPort)
{
    if (PurgeComm(hPort, PURGE_TXCLEAR | PURGE_RXCLEAR | PURGE_TXABORT | PURGE_RXABORT) == 0)
    {
        printf("Clearing The Port Failed\r\n");
        CloseHandle(hPort);
        return -1;
    }
	return 0;
}

int uartClose(HANDLE hPort)
{
	CloseHandle(hPort);

	return 0;
}

int uartTest(char* comPort)
{
    int ret;
	HANDLE hUartTest;
	char*msg = "Hello This is a uart test!\n";
	char buf[100];
	memset(buf,0,100);

    if (NULL == comPort)
    {
        printf("comPort is NULL!\n");
    }

	printf("Test begin ...! %s\n", comPort);

	ret = uartCreate(&hUartTest, comPort);
    if (-1 == ret)
    {
        printf("COM Create failed\n");
        return -1;
    }
    printf("COM create successfully\n");
    uartDcbSet(hUartTest, 115200, 8);
    uartConfig(hUartTest);
    uartClearBuffer(hUartTest);
	ret = uartSend((unsigned char*)msg, strlen(msg), hUartTest);
    if (ret < 0)
    {
        printf("COM send successfully\n");
    }
	uartClose(hUartTest);

	return 0;
}

int uartTimeoutParaSet(int ReadIntervalTimeout,
    int ReadTotalTimeoutConstant,
    int ReadTotalTimeoutMultiplier,
    int WriteTotalTimeoutMultiplier,
    int WriteTotalTimeoutConstant)
{
    /* Timeout between byteA and byteB millseconds */
    CommTimeouts.ReadIntervalTimeout = ReadIntervalTimeout;

    /* Once read serial port Total timeout ReadTotalTimeoutMultiplier * readbytes + ReadTotalTimeoutConstant millseconds*/
    CommTimeouts.ReadTotalTimeoutConstant = ReadTotalTimeoutConstant;

    /* Used for ReadTotalTimeoutConstant Caculate */
    CommTimeouts.ReadTotalTimeoutMultiplier = ReadTotalTimeoutMultiplier;
    CommTimeouts.WriteTotalTimeoutMultiplier = WriteTotalTimeoutMultiplier;
    CommTimeouts.WriteTotalTimeoutConstant = WriteTotalTimeoutConstant;
    return 0;
}


int uartTimeoutConfig(HANDLE hPort)
{
    int ret = 0;

    if (!SetCommTimeouts(hPort, &CommTimeouts))
    {
        printf("SetCommTimeouts Failed\r\n");
        CloseHandle(hPort);
        return -1;
    }

    return 0;
}

int uartGetCommTimeout(HANDLE hPort)
{
    if (!GetCommTimeouts(hPort, &CommTimeouts))
    {
        printf("GetCommTimeouts Failed\r\n");
        CloseHandle(hPort);
        return -1;
    }

    return 0;
}
