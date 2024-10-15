/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#ifndef INC_SERIAL_PORT_H
#define INC_SERIAL_PORT_H

extern DCB uartDCB;
extern int uartCreate(HANDLE *hPort, char *comPort);
extern int uartDcbSet(HANDLE hPort, int BaudRate, int ByteSize);
extern int uartConfig(HANDLE hPort);
extern int uartSend(unsigned char *buf, int len, HANDLE hPort);
extern int uartRecv(unsigned char *buf, int len, HANDLE hPort);
extern int uartClearBuffer(HANDLE hPort);
extern int uartClose(HANDLE hPort);
extern int uartTest(char* comPort);
extern int uartTimeoutParaSet(int ReadIntervalTimeout,
int ReadTotalTimeoutConstant,
int ReadTotalTimeoutMultiplier,
int WriteTotalTimeoutMultiplier,
int WriteTotalTimeoutConstant);
extern int uartTimeoutConfig(HANDLE hPort);
extern int uartGetCommTimeout(HANDLE hPort);

#endif

