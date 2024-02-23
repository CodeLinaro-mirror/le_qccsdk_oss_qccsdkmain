/******************************************************************************
 *
 *
 ******************************************************************************
 */

#include <windows.h>
#include <conio.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <string.h>
#include "Qcmbr.h"
#include "Socket.h"
#include "SerialPort.h"
#include "synchapi.h"
#include "safeAPI.h"

const char *QCMBR_VERSION = "QcmbrCOM.exe v1.0, 2023/06/09";

#define SformatOutput(buffer, size, format, ...) _snprintf_s(buffer, size, _TRUNCATE, format, __VA_ARGS__)
#define SformatInput sscanf

HANDLE ghMutex=NULL;
HANDLE hBcEvent = INVALID_HANDLE_VALUE;

int    gFromQ = 0;
// When verbose mode is enabled only then display all the details on packets.
// could be utilized to disable all prints. For now, starting with the place
// which has the most impact.
int    verbose = 0;
int    char_interval = 0;
#define MAX_HS_WIDTH    16

#define MCOMMAND 50

#define QCMBR_LOG DbgPrintfWithStamp

static struct _Socket *_ListenSocket;   // this is the socket on which we listen for client connections
static struct _Socket *_ClientSocket[MCLIENT];  // these are the client sockets
static int _CommandNext=0;      // index of next command to perform
static int _CommandRead=0;      // index of slot for next command read from socket
static char *_Command[MCOMMAND];
static char _CommandClient[MCOMMAND];

//
static int logTimeInit = 0;
static LARGE_INTEGER Freq;
static LARGE_INTEGER Start;
static LARGE_INTEGER End;

HANDLE hPort;

void DbgPrintf(const char *fmt, ...)
{
    va_list args;
    if (verbose > 0)
    {
        va_start(args, fmt);
        vfprintf(stderr, fmt, args);
        va_end(args);
    }
}

void DbgPrintfWithStamp(const char *fmt, ...)
{
    double delta_milliseconds = 0;
    va_list args;

    if (verbose <= 0)
    {
        return;
    }

    if (logTimeInit == 0)
    {
        logTimeInit = 1;
        QueryPerformanceFrequency(&Freq);
        QueryPerformanceCounter(&Start);
        QueryPerformanceCounter(&End);

        delta_milliseconds = ((double)(End.QuadPart - Start.QuadPart) * 1000) / ((double)Freq.QuadPart);
    }
    else
    {
        QueryPerformanceCounter(&End);
        delta_milliseconds = ((double)(End.QuadPart - Start.QuadPart) * 1000) / ((double)Freq.QuadPart);
        QueryPerformanceCounter(&Start);
    }

    printf("%7.2f | ", delta_milliseconds);

    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
}

void DispHexString(unsigned char *pkt_buffer, int recvsize)
{
  int i, j, k;

  if (verbose == 1)
  {
      for (i=0; i<recvsize; i+=MAX_HS_WIDTH) {
        printf("          "); printf("[%4.4d] ",i);
        for (j=i, k=0; (k<MAX_HS_WIDTH) && ((j+k)<recvsize); k++)
          printf("0x%2.2X ",(pkt_buffer[j+k])&0xFF);
        printf("\n");
      }
  }
}


extern int gClientSocket;

void CloseAll(BOOLEAN bQspr)
{
  extern void ClientClose(int client);
  int it;

  if (_ListenSocket)
  {
    SocketClose(_ListenSocket);
    _ListenSocket = NULL;
  }

  for (it=0; it<MCLIENT; it++)
    ClientClose(it);

  if (hBcEvent != INVALID_HANDLE_VALUE)
  {
      CloseHandle(hBcEvent);
      hBcEvent = INVALID_HANDLE_VALUE;
  }

  #if 0 //rwu
  if (IsDeviceOpened()) {
    //printf("Closing Device driver handles.\n");
    CloseDevice();
  }
  #endif

  if (ghMutex)
      CloseHandle(ghMutex);

  WSACleanup();
}


BOOL CtrlHandler( DWORD fdwCtrlType )
{
  switch (fdwCtrlType)
  {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_CLOSE_EVENT:
      printf("\nUser abort detected! Qcmbr is exiting now!\n");
      CloseAll(FALSE);
      return(FALSE);
    default:
      return FALSE;
  }
}


int main(int narg, char *arg[])
{
    int iarg;
    int console;
    int broadcast;
    int port = 2500;
    char comport[8] = {0};
    int ret;
    BOOL fSuccess;
    /* Bind Ctrl Event */
    SetConsoleCtrlHandler((PHANDLER_ROUTINE) CtrlHandler, TRUE);

    broadcast = 0;
    console=0;
    port = -1;
    //comport = "com4";

    printf("QCMBR_VERSION:%s\n", QCMBR_VERSION);

    for(iarg=1; iarg<narg; iarg++)
    {
        if (strcmp(arg[iarg],"-q") == 0)
        {
           gFromQ = 1;
           printf("Called from QDart.\n");
        }
        else if ( strncmp(arg[iarg], "-port", sizeof( "-port" ) ) == 0)
        {
            if(iarg+1<narg)
            {
                sscanf( arg[iarg+1], " %d ",(int *) &port);
                iarg++;
            }
        }
        else if (strncmp(arg[iarg], "-i", sizeof("-i")) == 0) // char interval
        {
            if (iarg + 1 < narg)
            {
                sscanf(arg[iarg + 1], " %d ", (int*)&char_interval);
                iarg++;
            }
        }
        else if ( strncmp(arg[iarg], "-com", sizeof( "-com" ) ) == 0)
        {
            printf("iarg = %d, narg = %d\n", iarg, narg);
            if(iarg+1<narg)
            {
                sscanf( arg[iarg+1], " %s ",comport);
                iarg++;
            }
            //printf("Prepare into unit test %s\r\n", comport);
            //uartTest(comport);

            ret = uartCreate(&hPort, comport);
            if (-1 == ret)
            {
                printf("COM Create failed\n");
                return -1;
            }
            printf("COM create successfully\n");

            /* Queue Size */
			SetupComm(hPort, 8192, 8192);

            /* Time Out */
            uartGetCommTimeout(hPort);
            uartTimeoutParaSet(50, 50, 10, 10, 50);
            uartTimeoutConfig(hPort);

            /* DCB */
            uartDcbSet(hPort, 115200, 8);
            uartConfig(hPort);

            fSuccess = SetCommMask(hPort, 0);

            fSuccess = SetCommMask(hPort, EV_RXCHAR);
            if (!fSuccess)
            {
                // Handle the error.
                printf("SetCommMask failed with error %d.\n", GetLastError());
                return;
            }

            uartClearBuffer(hPort);
        }

        else if ( strncmp(arg[iarg],"-help", sizeof( "-help" ) ) == 0)
        {

            printf( "-port: Quts port \r\n" );
            printf( "-com: uart port \r\n" );
            printf( "-v :  Verbose mode\r\n");
            printf( "-i :  byte send interval\r\n");
            exit(0);
        }
        else if ( strncmp(arg[iarg], "-v", sizeof( "-v" ) ) == 0)
        {
            printf( "Enabling verbose mode\n" );
            verbose = 1;
        }
        else
        {
            printf( "Error - Unknown parameter\n" );
        }
    }

    if ((ghMutex = CreateMutex(NULL,TRUE,"Qcmbr-Mutex") ) == NULL) {
        if (gFromQ)
          exit(-2);
    } else
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        if (gFromQ)
          exit(-3);
    }

    Qcmbr_Run(port);
    exit(0);
}

static int ClientAccept()
{
    int it;
    int noblock;
    struct _Socket *TryClientSocket;
    static int OldestClient = 0;

    if(_ListenSocket!=0)
    {
        //
        // If we have no clients, we will block waiting for a client.
        // Otherwise, just check and go on.
        //
        noblock=0;
        for(it=0; it<MCLIENT; it++)
        {
            if(_ClientSocket[it]!=0)
            {
                noblock=1;
                break;
            }
        }
        if(noblock==0)
        {
            printf("\nReady for Client(QDart) Connection!\n" );
        }
        //
        // Look for new client
        //
        for(it=0; it<MCLIENT; it++)
        {
            if(_ClientSocket[it]==0)
            {
                _ClientSocket[it]=SocketAccept(_ListenSocket,noblock);// don't block
                if(_ClientSocket[it]!=0)
                {
                    printf("Client connection is established at [%d]!\n",it);
                    DbgPrintf( "_ClientSocket[%d]->sockfd = %d, port = %d\n",
                        it, _ClientSocket[it]->sockfd, _ClientSocket[it]->port_num);
                    return it;
                }
            }
        }
        // In case all clients have been used up, but there is another client want to connect, give away the oldest one
        if (it == MCLIENT)
        {
            TryClientSocket = SocketAccept(_ListenSocket,noblock);
            if (TryClientSocket)
            {
                SocketClose(_ClientSocket[OldestClient]);
                _ClientSocket[OldestClient] = TryClientSocket;

                return it;
            }
        }
    }
    return -1;
}


static void ClientClose(int client)
{
    if(client>=0 && client<MCLIENT && _ClientSocket[client]!=0)
    {
        SocketClose(_ClientSocket[client]);
        _ClientSocket[client]=0;
    }
}
unsigned char uart_read_buf[MBUFFER];
int CommandRead()
{
    unsigned char buffer[MBUFFER];
    int nread;
    //int ntotal;
    int it;
    int diagPacketReceived = 0;
    unsigned int cmdLen = 0;
    int ret;
    int readLen;
    //
    // look for new clients
    //
    ClientAccept();
    //
    // try to read everything on the client socket
    //
    //ntotal=0;
    while(_CommandNext!=_CommandRead || _Command[_CommandRead]==0)
    {
        if(_ListenSocket==0)
        {
            DbgPrintf( "ListenSocket is NULL\n");
            return -1;
        }
        else
        {
            //
            // read commands from each client in turn
            //
            for(it=0; it<MCLIENT; it++)
            {
                if(_ClientSocket[it]!=0)
                {
                    nread = (int)SocketRead(_ClientSocket[it],buffer,MBUFFER-1);
                    if(nread>0)
                    {
                        DbgPrintf( "\n\nStep 1 - SocketRead() from QUTS %d bytes\n", nread );
                        buffer[nread]=0;
                        DispHexString(buffer,nread);

                        if(nread>1 && buffer[nread-1]==DIAG_TERM_CHAR)
                        {
                            diagPacketReceived = 1;
                            //buffer[nread-1]=0;//terminate char need to tx to uart.
                            cmdLen = nread-0;
                        }
                        //Needed for linux path
                        if(nread>2 && buffer[nread-2]==DIAG_TERM_CHAR)
                        {
                            diagPacketReceived = 1;
                            buffer[nread-2]=0;
                            cmdLen = nread - 1;
                        }
                        //
                        // check to see if we received a diag packet
                        //
                        if (diagPacketReceived) {

                            DbgPrintf("Step 2 - uartSend() to Fermion\n");
                            uartSend(buffer, cmdLen, hPort);

                            /* Need to sleep 100 to give frimware handle the packet */
                            //Sleep(100);

                            DbgPrintf("Step 3 - uartRecv() from Fermion\n");
                            readLen = uartRecv(uart_read_buf,sizeof(uart_read_buf), hPort);
                            if (readLen > 0)
                            {
                                DispHexString(uart_read_buf, readLen);

                                SocketWriteEnableMode(1);
                                ret = SocketWrite(_ClientSocket[it], uart_read_buf, readLen);
                                DbgPrintf("Step 4 - SocketWrite to QUTS: len=%d\n\n", ret);
                                SocketWriteEnableMode(0);
                                if (ret <= 0)
                                {
                                    DbgPrintf("SocketWrite error ret=%d\n", ret);
                                }

                            }
                            else
                            {
                                DbgPrintf("uartRecv uartRecv=%d\n", readLen);
                            }
#if 0
                             {
                                }
                            if (processDiagPacket(it, (unsigned char*)buffer, cmdLen)) {
                                DbgPrintf("\n--processDiagPacket-succeed------ Wait For Next Diag Packet ----------------\n\n");
                                continue;
                            }
                            else
                            {
                                printf("\n--processDiagPacket-failed------- Wait For Next Diag Packet ----------------\n\n");
                            }
                        }
#endif
                        }
                    }
                    else if(nread<0)
                    {
                        printf("Closing connection <-- Remote connection closed.[%d]\n",gFromQ);
                        ClientClose(it);
                        CloseAll(TRUE);
                        exit(1);
                    }
                }
            }
        }
    }
    return 0; //ntotal;
}


int CommandNext(unsigned char *command, int max, int *client)
{
    int length;
    //
    // try to read new commands
    //
    if(CommandRead()<0)
    {
        return -1;
    }
    //
    // if we have a command, return it
    //
    if(_Command[_CommandNext]!=0)
    {
        length = sizeof( _Command[_CommandNext]); //length=Slength(_Command[_CommandNext]);
        if(length>max)
        {
            _Command[_CommandNext][max]=0;
            length=max;
        }

        if ( command != NULL )
        {
            if ( _Command[_CommandNext] == NULL )
            {
                strncpy_s( (char*) command, max, _Command[_CommandNext], MCOMMAND);
            }
        }
        *client=_CommandClient[_CommandNext];
        if ( _Command[_CommandNext] != NULL ) { free( _Command[_CommandNext] ); }
        _Command[_CommandNext]=0;
        _CommandNext=(_CommandNext+1)%MCOMMAND;

        printf("> %s\n",command);

        return length;
    }
    return 0;
}

int SendItDiag(int client, unsigned char *buffer, int length)
{
    int nwrite;

    if(_ListenSocket==0|| (client>=0 && client<MCLIENT && _ClientSocket[client]!=0))
    {
        if ( _ListenSocket == 0 )
        {
            //printf("%s",response);
        }
        else
        {
            DbgPrintf( "SendItDiag() return data to QDART, length=%d\n", length );
            DispHexString(buffer, length);

            SocketWriteEnableMode( 1 );
            nwrite=SocketWrite(_ClientSocket[client],buffer,length);
            SocketWriteEnableMode( 0 );

            if(nwrite<0)
            {
                printf( "Error - Call to SocketWrite() failed.\n" );
                ClientClose(client);
                return -1;
            }
        }
        return 0;
    }
    else
    {
        return -1;
    }

}


void Qcmbr_Run(int port)
{
    unsigned char buffer[MBUFFER];
    int nread;
    int client;
    int it;

    SetStrTerminationChar( DIAG_TERM_CHAR );
    SocketWriteEnableMode( 0 );

    //
    // open listen socket
    //
    if(port>0)
    {
        _ListenSocket=SocketListen(port);
        if(_ListenSocket==0)
        {
            printf( "Can't open control process listen port %d.", port );
            exit(-1);
        }
        DbgPrintf( "_ListenSocket->sockfd = %d, port = %d\n", _ListenSocket->sockfd, _ListenSocket->port_num);
    }
    else
    {
        //
        // this means accept commands from the keyboard
        // and display result by typing in the console window
        //
        _ListenSocket= 0;
    }

    // Clear the client socket records
    for ( it=0; it < MCLIENT; it++ )
    {
        _ClientSocket[it] = 0;
    }

    //
    // wait for commands or new clients
    //
    //while(1)
    //{
        if (ClientAccept() != -1)
        {
            if (hBcEvent != INVALID_HANDLE_VALUE)
              SetEvent(hBcEvent);
        }

        while(1)
        {
            nread=CommandNext(buffer,MBUFFER-1,&client);

            //
            // Got data. Process it.
            //
            if(nread>0)
            {
                // PASS TO DIAG PACKET HANDLER
                printf(" we received packet");
                //processDiagPacket( 0, buffer, nread);
            }
            //
            // Got error. Probably lost command module. Redo socket accept.
            //
            else if(nread<0)
            {
                DbgPrintf( "nread =%d < 0\n", nread);
                return;
            }
            //
            // slow down
            //
            else
            {
                //Sleep(0);
            }
        }
    //}
}




