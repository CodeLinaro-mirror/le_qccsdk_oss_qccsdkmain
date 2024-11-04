/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/


/* socket.h - header file for socket.c */

#ifndef INC_SOCKET_H
#define INC_SOCKET_H


#ifdef _USE_SOCKET_FUNCTIONS_IN_EXE_
#define PARSEDLLSPEC
#else
#ifdef _WINDOWS
//For devlib-qdart
	#ifdef PART_STATIC
		#define PARSEDLLSPEC
	#elif PARSEDLL
		#define PARSEDLLSPEC __declspec(dllexport)
	#else
		#define PARSEDLLSPEC __declspec(dllimport)
	#endif
#else
	#define PARSEDLLSPEC
#endif
#endif





#define MSOCKETBUFFER 4096



struct _Socket
{
	char hostname[128];
	unsigned int port_num;
	unsigned int ip_addr;
	int inHandle;
	int outHandle;
	int  sockfd;
	unsigned int sockDisconnect;
	unsigned int sockClose;
	int nbuffer;
	char buffer[MSOCKETBUFFER];
};



extern PARSEDLLSPEC int SocketRead(struct _Socket *pSockInfo, unsigned char *buf, int len);
extern PARSEDLLSPEC int SocketWrite(struct _Socket *pSockInfo, unsigned char *buf, int len);
extern PARSEDLLSPEC void SocketClose(struct _Socket *pSockInfo);
extern PARSEDLLSPEC struct _Socket *SocketConnect(char *pname, unsigned int port);
extern PARSEDLLSPEC struct _Socket *SocketAccept(struct _Socket *pSockInfo, unsigned long noblock);
extern PARSEDLLSPEC struct _Socket *SocketListen(unsigned int port);
extern PARSEDLLSPEC void SocketWriteEnableMode( int writeEnable );


#endif

