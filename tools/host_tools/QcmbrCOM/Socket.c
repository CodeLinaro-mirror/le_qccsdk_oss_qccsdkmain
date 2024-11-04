/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/


/* socket.c - socket interface */

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>


#include <winsock2.h>
#include <windows.h>
#include <process.h>
#include <Tlhelp32.h>
#include <io.h>


#include <stddef.h>
#include "Socket.h"
#include "safeAPI.h"


#define SEND_BUF_SIZE 1024

char terminationChar = '\n';

int socketWriteEnable = 0;
PARSEDLLSPEC void SocketWriteEnableMode( int writeEnable )
{

    socketWriteEnable = writeEnable;
    //printf( "SocketWriteEnableMode( %d )\n", socketWriteEnable );
}


/**************************************************************************
* osSockRead - read len bytes into *buf from the socket in pSockInfo
*
* This routine calls recv for socket reading
*
* RETURNS: length read
*/

PARSEDLLSPEC int SocketRead(struct _Socket *pSockInfo, unsigned char *buf, int len)
{
    unsigned long noblock;
    int nread;
    int ncopy;
    int error;
    int il;

    ncopy=0;

    noblock=1;
    error=ioctlsocket(pSockInfo->sockfd,FIONBIO,&noblock);
    if(error!=0)
    {
        return -1;
    }
    //
    // try to get some more data
    //
    nread=MSOCKETBUFFER-pSockInfo->nbuffer;
        nread=recv(pSockInfo->sockfd, &(pSockInfo->buffer[pSockInfo->nbuffer]), nread, 0);

    if (nread == 0) {
        return(-1);
    } else
    if(nread<0)
    {
        if(WSAGetLastError()==WSAEWOULDBLOCK)
        {
            nread=0;
        }
        else
        {
            return -1;
        }
    }

    pSockInfo->nbuffer+=nread;
    //
    // check to see if we have enough stored data to return a message
    //
    for(il=0; il<pSockInfo->nbuffer; il++)
    {
        if ( pSockInfo->buffer[il] == terminationChar )
        {
            il++;
            //
            // copy data to user buffer
            //
            if(len<il)
            {
                ncopy=len-1;
            }
            else
            {
                ncopy=il;
            }
            memcpy(buf,pSockInfo->buffer,ncopy);
            buf[ncopy]=0;
            //
            // compress the internal data
            //
            memcpy(pSockInfo->buffer,&pSockInfo->buffer[il],pSockInfo->nbuffer-il);
            pSockInfo->nbuffer -= il;

            return ncopy;
        }
    }
    // nart doesn't send terminationChar
    if (pSockInfo->nbuffer > 0 && il==pSockInfo->nbuffer) {
        ncopy = pSockInfo->nbuffer;
        memcpy(buf,pSockInfo->buffer,ncopy);
        buf[ncopy]=0;
    }
    //
    // no message, but buffer is full. This is a problem.
    // delete allof the existing data.
    //
    if(il>=MSOCKETBUFFER)
    {
        pSockInfo->nbuffer=0;
    }

    return ncopy;
}

/**************************************************************************
* osSockWrite - write len bytes into the socket, pSockInfo, from *buf
*
* This routine calls a OS specific routine for socket writing
*
* RETURNS: length read
*/
PARSEDLLSPEC int SocketWrite(struct _Socket *pSockInfo, unsigned char *buf, int len)
{
    int dwWritten;
    int bytes,cnt;
    unsigned char* bufpos;
    int tmp_len;
    int error;

    if ( socketWriteEnable == 0 )
    {
        //printf( "SocketWrite(): socketWrite is not enabled.\n" );
        return 0;
    }

    error=0;

    tmp_len = len;
    bufpos = buf;
    dwWritten = 0;

    while (len)
    {
        if (len < SEND_BUF_SIZE) bytes = len;
        else bytes = SEND_BUF_SIZE;

        cnt = send(pSockInfo->sockfd, (char *)bufpos, bytes, 0);
        if(cnt<=0)
        {
            if(WSAGetLastError()==WSAEWOULDBLOCK)
            {
                        error++;
                if(error>100000)
                {
                    break;
                }
                cnt=0;
                    Sleep(10);
                    continue;
            }
            else
            {
                return -1;
            }
        }
        error=0;
        dwWritten += cnt;
        len  -= cnt;
        bufpos += cnt;
    }

    len = tmp_len;

    if (dwWritten != len) {
        dwWritten = 0;
    }

    return dwWritten;
}

/**************************************************************************
* osSockClose - close socket
*
* Close the handle to the pipe
*
* RETURNS: 0 if error, non 0 if no error
*/
void SocketClose(struct _Socket* pOSSock)
{
    closesocket(pOSSock->sockfd);
    free(pOSSock);
    return;
}

#define CONNECT_TIMEOUT 20 /* secs cart will wait for a nart connect */

static int socketConnect(char *target_hostname, unsigned int target_port_num, unsigned int *ip_addr)
{
    int    bcfd;
    struct protoent *   proto;
    int    res;
    struct sockaddr_in  sin;
    int    i;
    struct hostent *hostent;

    WORD   wVersionRequested;
    WSADATA wsaData;

    wVersionRequested = MAKEWORD( 2, 2 );

    res = WSAStartup( wVersionRequested, &wsaData );
    if ( res != 0 ) {
    return -1;
    }

    if ( LOBYTE( wsaData.wVersion ) != 2 ||
        HIBYTE( wsaData.wVersion ) != 2 ) {
    WSACleanup( );
    return -1;
    }

    if((proto = getprotobyname("tcp")) == NULL) {
        WSACleanup( );
        return -1;
    }

    bcfd = (int)WSASocket(PF_INET, SOCK_STREAM, proto->p_proto, NULL, (GROUP)NULL,0);
    if (bcfd == INVALID_SOCKET) {
        WSACleanup( );
        return -1;
    }

    /* Allow immediate reuse of port */
    i = 1;
    res = setsockopt(bcfd, SOL_SOCKET, SO_REUSEADDR, (char *) &i, sizeof(i));
    if (res != 0) {
        WSACleanup( );
        return -1;
    }

// Linux disabled Nagle by default
    /* Set TCP Nodelay */
    i = 1;
    res = setsockopt(bcfd, IPPROTO_TCP, TCP_NODELAY, (char *) &i, sizeof(i));
    if (res != 0) {
        WSACleanup( );
        return -1;
    }

    hostent = gethostbyname(target_hostname);
    if (!hostent) {
        WSACleanup( );
        return -1;
    }

    memcpy(ip_addr, hostent->h_addr_list[0], hostent->h_length);
    *ip_addr = ntohl(*ip_addr);

    sin.sin_family = AF_INET;
    memcpy(&sin.sin_addr.s_addr, hostent->h_addr_list[0], hostent->h_length);
    sin.sin_port = htons((short)target_port_num);

    res = connect(bcfd, (struct sockaddr *) &sin, sizeof(sin));
    if (res!=0)
    {
        return -1;
    }

    return bcfd;
}


PARSEDLLSPEC struct _Socket *SocketConnect(char *pname, unsigned int port)
{
    char        pname_lcl[256];
    char *      mach_name;
    char *      cp;
    struct _Socket *pOSSock;
    int         res;

    strncpy_s(pname_lcl, sizeof(pname_lcl), pname, sizeof(pname_lcl));
    pname_lcl[sizeof(pname_lcl) - 1] = '\0';
    mach_name = pname_lcl;
    while (*mach_name == '\\') {
        mach_name++;
    }
    for (cp = mach_name; (*cp != '\0') && (*cp != '\\'); cp++) {
    }
    *cp = '\0';


    if (!strcmp(mach_name, ".")) {
        /* A windows convention meaning "local machine" */
        mach_name = "localhost";
    }

    pOSSock = (struct _Socket *) malloc(sizeof(struct _Socket));
    if(!pOSSock) {
        return NULL;
    }

    strncpy_s(pOSSock->hostname, sizeof(pOSSock->hostname), mach_name, sizeof(pOSSock->hostname));
    pOSSock->hostname[sizeof(pOSSock->hostname) - 1] = '\0';

    pOSSock->port_num = port;


    res = socketConnect(pOSSock->hostname, pOSSock->port_num,
        &pOSSock->ip_addr);;
    if (res < 0) {
        free(pOSSock);
        return NULL;
    }

    pOSSock->sockfd = res;

    pOSSock->nbuffer=0;

    return pOSSock;
}


/* osSockAccept - Wait for a connection
*
*/
PARSEDLLSPEC struct _Socket *SocketAccept(struct _Socket *pOSSock, unsigned long noblock)
{
    struct _Socket *pOSNewSock;
    int     i;
    int     bcfd;
    struct sockaddr_in  sin;
    int error;
    unsigned long varbk = 1;

    //printf("SocketAccept: pOSSock->sockfd =%d, noblock=%d.\n", pOSSock->sockfd, noblock);
    // In order not to cause deadlock when terminate socket from user, change to use nonblocking mode to check socket.
    error=ioctlsocket(pOSSock->sockfd,FIONBIO,&varbk);
    if(error!=0)
    {
    }

    if (noblock == 0) // blocking wait connection.
    {
        while(1) // block to wait socket with sleep. This will be terminated from user by "Ctrl + C" or Kill process.
        {
            i = sizeof(sin);
            bcfd = (int)accept(pOSSock->sockfd, (struct sockaddr *) &sin, (int *)&i);
            if (bcfd != INVALID_SOCKET)
            {
                break; // connection is on.
            }

            Sleep(100);
        }
    }
    else
    {
        i = sizeof(sin);
        bcfd = (int)accept(pOSSock->sockfd, (struct sockaddr *) &sin, (int *)&i);

        if (bcfd == INVALID_SOCKET)
        {
            if(WSAGetLastError()==WSAEWOULDBLOCK)
            {
                return 0;
            }
            else
            {
                return 0;
            }
        }
    }

    pOSNewSock = (struct _Socket *) malloc(sizeof(*pOSNewSock));
    if (!pOSNewSock)
    {
        return NULL;
    }

    strncpy_s(pOSNewSock->hostname, 128, inet_ntoa(sin.sin_addr), 128);
    pOSNewSock->port_num = pOSSock->port_num;

    pOSNewSock->sockClose = 0;
    pOSNewSock->sockDisconnect = 0;
    pOSNewSock->sockfd = bcfd;

    pOSNewSock->nbuffer=0;

    return pOSNewSock;
}



static int socketListen(struct _Socket *pOSSock)
{
    int    sockfd;
    struct protoent *   proto;
    int    res;
    struct sockaddr_in sin;
    int    i;

    WORD   wVersionRequested;
    WSADATA wsaData;

    wVersionRequested = MAKEWORD( 2, 2 );

    res = WSAStartup( wVersionRequested, &wsaData );

    if ( res != 0 ) {
        return -1;
    }

    if ( LOBYTE( wsaData.wVersion ) != 2 ||
        HIBYTE( wsaData.wVersion ) != 2 ) {
        WSACleanup( );
        return -1;
    }

    if((proto = getprotobyname("tcp")) == NULL) {
        WSACleanup( );
        return -1;
    }

    sockfd = (int)WSASocket(PF_INET, SOCK_STREAM, proto->p_proto, NULL, (GROUP)NULL, 0);

    if (sockfd == INVALID_SOCKET) {
        WSACleanup( );
        return -1;
    }

    i = 1;

// Linux disabled Nagle by default
    res = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char *) &i, sizeof(i));
    if (res != 0) {
        WSACleanup( );
        return -1;
    }

    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr =  INADDR_ANY;
    sin.sin_port = htons((unsigned short)pOSSock->port_num);

    res = bind(sockfd, (struct sockaddr *) &sin, sizeof(sin));
    if (res != 0) {
        WSACleanup( );
        return -1;
    }

    //printf("listen to port = %d\n", pOSSock->port_num);
    res = listen(sockfd, 4);
    if (res != 0) {
        WSACleanup( );
        return -1;
    }

    return sockfd;
}



PARSEDLLSPEC struct _Socket *SocketListen(unsigned int port)
{
    struct _Socket *pOSSock;

    pOSSock = (struct _Socket *) malloc(sizeof(struct _Socket));
    if(!pOSSock) {
        return NULL;
    }

    pOSSock->port_num = port;

    pOSSock->sockDisconnect = 0;
    pOSSock->sockClose = 0;

    pOSSock->sockfd = socketListen(pOSSock);
    if(pOSSock->sockfd < 0) {
       free(pOSSock);
       return NULL;
    }

    pOSSock->nbuffer=0;

    return pOSSock;
}

PARSEDLLSPEC void SetStrTerminationChar( char tc )
{
    terminationChar = tc;
}

PARSEDLLSPEC char GetStrterminationChar( void )
{
    return terminationChar;
}
