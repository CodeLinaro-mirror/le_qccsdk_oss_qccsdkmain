/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/



#ifdef CONFIG_SIGMA_TRAFFIC

#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#include "qapi_status.h"
#include "qapi_console.h"
#include "qurt_internal.h"
#include "qurt_mutex.h"
#include "sigma.h"
#include "lwip/sockets.h"
#include "lwip/def.h"
#include "lwip/ip6_addr.h"
#include "timer.h"

/*-------------------------------------------------------------------------
 * Preprocessor Definitions, Constants, and Type Declarations
 *-----------------------------------------------------------------------*/

/*-------------------------------------------------------------------------
 * Variables
 *-----------------------------------------------------------------------*/

uint8_t sigma_tx_quit;
uint8_t sigma_rx_quit;


/*-------------------------------------------------------------------------
 * Private Function Declarations
 *-----------------------------------------------------------------------*/

/* -------------------------------------------------------------------
 * usage
 * ------------------------------------------------------------------- */

static void sigma_help()
{
    SIGMA_PRINTF( "Usage: udp [-s|-c host] [options]\n");
    SIGMA_PRINTF("Server or Client:\n");
    SIGMA_PRINTF("  -p, --port      #         server port to listen on/connect to (default %d)\n",SIGMA_TRAFFIC_DEFAULT_PORT);
    SIGMA_PRINTF("  -B, --bind <host>         bind to the interface associated with the address <host>\n");
    SIGMA_PRINTF("  -h, --help                show this message and quit\n");
    SIGMA_PRINTF("  -s, --server              run in server mode\n");
    SIGMA_PRINTF("  -e, --echo                transmit back the receiving packet\n");
    SIGMA_PRINTF("  -c, --client <host>       run in client mode, connecting to <host>\n");
    SIGMA_PRINTF("  -t, --time      #         time in seconds to transmit for (default %d seconds)\n",SIGMA_TRAFFIC_DEFAULT_RUNTIME);
    SIGMA_PRINTF("  -n, --packets   #         number of packets to transmit (instead of -t)\n");
    SIGMA_PRINTF("  -l, --length    #         message length to transmit (default %d bytes)\n", SIGMA_TRAFFIC_DEFAULT_PACKET_SIZE);
    SIGMA_PRINTF("  -i, --interval  #         delay in milliseconds between packets (default 1 millisecond)\n");
    SIGMA_PRINTF("  -S, --tos       #         set the IP type of service, 0-255. (default 0)\n");
    SIGMA_PRINTF("                            The usual prefixes hex can be used,\n");
    SIGMA_PRINTF("                            i.e. 52 and 0x34 specify the same value.\n");
    SIGMA_PRINTF("Examples:\n");
    SIGMA_PRINTF("udp -s -B 224.2.2.5 -e -p 7001\n");
    SIGMA_PRINTF("udp -c 224.2.2.5 -l 1500 -i 1 -t 20 -B 192.168.1.101\n");                     
}


/*-------------------------------------------------------------------------
 * Function Definitions
 *-----------------------------------------------------------------------*/


/************************************************************************
 ************************************************************************/
void sigma_clear_stats(SIGMA_CXT *p_tCxt)
{
    p_tCxt->pktStats.bytes = 0;
    p_tCxt->pktStats.kbytes = 0;
    p_tCxt->pktStats.sent_bytes = 0;
    p_tCxt->pktStats.pkts_recvd = 0;
}

/************************************************************************
* NAME: sigma_check_test_time
*
* DESCRIPTION: If test mode is time, check if current time has exceeded
* test time limit
* Parameters: pointer to sigma context
************************************************************************/
uint32_t sigma_check_test_time(SIGMA_CXT *p_tCxt)
{
    uint32_t duration; /* in ms */
    uint32_t last_time = p_tCxt->pktStats.last_time;
    uint32_t first_time = p_tCxt->pktStats.first_time;

    if (last_time < first_time)
    {
        /* Assume the systick wraps around once */
        duration = ~first_time + 1 + last_time;
    }
    else
    {
        duration = last_time - first_time;
    }

    if (duration >= p_tCxt->params.tx_params.tx_time * 1000)
        return 1;
    else
        return 0;
}


/************************************************************************
************************************************************************/
void sigma_print_test_results(SIGMA_CXT *p_tCxt, SIGMA_STATS *pktStats)
{
    /* Print throughput results.*/
    unsigned long long total_bytes = 0;
    uint64_t total_interval;    /* in msec */
    uint32_t duration;          /* in msec */
    uint32_t sec_interval;
    uint32_t throughput;
    uint32_t last_time = pktStats->last_time;
    uint32_t first_time = pktStats->first_time;

    if (last_time < first_time)
    {
        /* Assume the systick wraps around once */
        duration = ~first_time + 1 + last_time;
#ifdef BENCH_TEST_RESULT_DEBUG
        SIGMA_PRINTF("last: 0x%x first: 0x%x duration: 0x%x\n",
                        last_time, first_time, duration);
#endif
    }
    else
    {
        duration = last_time - first_time; 
    }

    total_interval = duration; 
    sec_interval = duration / (1000);

    if (total_interval > 0)
    {
        /*No test was run, or test is terminated, print results of previous test*/
        if (pktStats->bytes == 0)
        {
            total_bytes     = pktStats->last_bytes;
            total_interval  = pktStats->last_interval;
            throughput      = pktStats->last_throughput;
        }
        else
        {
            total_bytes = pktStats->bytes;
            /* Take care of wrap around cases. If number of bytes exceeds
               0x3FFFFFFFFFFFFFLL, it exceeds 64 bits and wraps around
               resulting in wrong throughput number */
            if (total_bytes <= 0x3FFFFFFFFFFFFFLL)
            {
                /* (N/T) bytes/ms = (1000/128)*(N/T) Kbits/s */
                throughput = (total_bytes*125/(duration*16)) ; /* in Kb/sec */
            }
            else
            {
                unsigned long long bytes;
                unsigned long long kbytes;

                /* Convert bytes to kb and divide by seconds for this case */
                kbytes  = total_bytes / 1024;
                bytes   = total_bytes % 1024;
                throughput = ((kbytes*8) / sec_interval) + ((bytes*8/1024) / sec_interval); /* Kb/sec */
            }
            pktStats->last_interval   = total_interval;
            pktStats->last_bytes      = total_bytes;
            pktStats->last_throughput = throughput;
            sec_interval = pktStats->last_interval / (1000);
        }
    }
    else
    {
        total_bytes = pktStats->bytes + 1024*pktStats->kbytes;
        throughput = 0;
    }

    SIGMA_PRINTF("\nResults for %s %s test:\n\n", (p_tCxt->protocol == PROT_TCP)? "TCP":"UDP",
	    							(p_tCxt->test_type == SIGMA_RX)?"Receive":"Transmit");
    SIGMA_PRINTF("\t%u KBytes %u bytes (%u bytes) in %u seconds %u ms (%u miliseconds)\n",
            (uint32_t)(total_bytes/1024), (uint32_t)(total_bytes%1024), (uint32_t)total_bytes, sec_interval, (uint32_t)(total_interval%1000), total_interval);

    SIGMA_PRINTF("\n\tThroughput: %d Kbits/sec\n", throughput);
}

void sigma_SetProtocol(SIGMA_CXT *p_rxtCxt, const char* protocol)
{
	if (strcasecmp("udp", protocol) == 0 ) {
		p_rxtCxt->protocol = PROT_UDP;
	}
	else {
	    p_rxtCxt->protocol = ~0; /* Invalid protocol */
	}
}

uint32_t sigma_SetParams(SIGMA_CXT *p_rxtCxt,  const char *protocol, uint16_t port, enum sigma_test_type type)
{
	sigma_SetProtocol(p_rxtCxt, protocol);

	if (type == SIGMA_RX) {
		if (sigma_udp_IsPortInUse(port)) {
			SIGMA_PRINTF("port %d is in use; use another port.\n", port);
			return QAPI_ERR_INVALID_PARAM;
		}
	}

	switch(type) {
		case SIGMA_RX:
    	p_rxtCxt->params.rx_params.port = port;
		break;
		case SIGMA_TX:
		p_rxtCxt->params.tx_params.port = port;
		break;
	}
	p_rxtCxt->test_type = type;
	return 0;
}

qapi_Status_t sigma_udp_cmd(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    int32_t err = 0;
    uint32_t port = SIGMA_TRAFFIC_DEFAULT_PORT;
    uint32_t pktSize = SIGMA_TRAFFIC_DEFAULT_PACKET_SIZE;
    int32_t operation_mode = -1;
    uint8_t mcastEnabled = 0;
    uint8_t echo = 0;
    uint8_t test_mode = SIGMA_TIME_TEST;
    int32_t ip_tos = 0;

    uint32_t dest_ip_address = 0;
    uint32_t bind_ip_address = 0;
    uint32_t numOfPkts = 0;
    uint32_t packet_interval_ms = 1;
    uint32_t tx_time = SIGMA_TRAFFIC_DEFAULT_RUNTIME;
    uint32_t index = 0;

    SIGMA_CXT *p_tCxt = NULL;

    index = 0;

    if (Parameter_Count < 1)
    {
        sigma_help();
        return QAPI_ERR_INVALID_PARAM;
    }

    while (index < Parameter_Count)
    {
        if (0 == strcmp(Parameter_List[index].String_Value, "-s")
            || 0 == strcmp(Parameter_List[index].String_Value, "--server"))
        {
            index++;
            operation_mode = SIGMA_TRAFFIC_SERVER;
        }
        else if (0 == strcmp(Parameter_List[index].String_Value, "-e")
            ||0 == strcmp(Parameter_List[index].String_Value, "--echo"))
        {
            index++;
            echo = 1;
     
        }
        else if (0 == strcmp(Parameter_List[index].String_Value, "-c")
            || 0 == strcmp(Parameter_List[index].String_Value, "--client"))
        {
            index++;
            operation_mode = SIGMA_TRAFFIC_CLIENT;
            
            /*Get IPv4 address of Peer (network order)*/
              /* is valid IPV4 */
            if (inet_pton(AF_INET, Parameter_List[index].String_Value, &dest_ip_address) == 1)
            {
                /* is valid IPV4 */
                if (IP_MULTICAST(ntohl(dest_ip_address))) // 224.xxx.xxx.xxx - 239.xxx.xxx.xxx
                {
                    mcastEnabled = 1;
                }
            }
            index++;
        }
        else if (0 == strcmp(Parameter_List[index].String_Value, "-B")
            || 0 == strcmp(Parameter_List[index].String_Value, "--bind"))
        {
            index++;           
            /*Get IPv4 address of Bind IP (network order)*/
            if (inet_pton(AF_INET, Parameter_List[index].String_Value, &bind_ip_address) == 1)
            {
                /* is valid IPV4 */
                if (IP_MULTICAST(ntohl(bind_ip_address))) // 224.xxx.xxx.xxx - 239.xxx.xxx.xxx
                {
                    mcastEnabled = 1;
                }
            }
            index++;
        }
        else if (0 == strcmp(Parameter_List[index].String_Value, "-p")
            || 0 == strcmp(Parameter_List[index].String_Value, "--port"))
        {
            index++;
            port = Parameter_List[index].Integer_Value;
            index++;

            if (port > 65535)
            {
                SIGMA_PRINTF("error: invalid port\n");
                return QAPI_ERR_INVALID_PARAM;
            }
        }
        else if (0 == strcmp(Parameter_List[index].String_Value, "-i")
            || 0 == strcmp(Parameter_List[index].String_Value, "--interval"))
        {
            index++;
            packet_interval_ms = Parameter_List[index].Integer_Value;
            index++;
        }
        else if (0 == strcmp(Parameter_List[index].String_Value, "-l")
            || 0 == strcmp(Parameter_List[index].String_Value, "--length"))
        {
            index++;
            pktSize = Parameter_List[index].Integer_Value;
            index++;
            pktSize = pktSize < 12 ? 12 : pktSize;
        }
        else if (0 == strcmp(Parameter_List[index].String_Value, "-t")
            || 0 == strcmp(Parameter_List[index].String_Value, "--time"))
        {
            index++;
            tx_time = Parameter_List[index].Integer_Value;
            index++;
            test_mode = SIGMA_TIME_TEST;
        }
        else if (0 == strcmp(Parameter_List[index].String_Value, "-n")
            || 0 == strcmp(Parameter_List[index].String_Value, "--packets"))
        {
            index++;
            numOfPkts = Parameter_List[index].Integer_Value;
            index++;
            test_mode = SIGMA_PACKET_TEST;
        }
        else if (0 == strcmp(Parameter_List[index].String_Value, "-S")
            || 0 == strcmp(Parameter_List[index].String_Value, "--tos"))
        {
            index++;
            ip_tos = Parameter_List[index].Integer_Value;
            index++;
            if(ip_tos > 255 || ip_tos < 0)
            {
                SIGMA_PRINTF("error: invalid TOS value\n");
                return QAPI_ERR_INVALID_PARAM;
            }
        }
        else if (0 == strcmp(Parameter_List[index].String_Value, "-h")
            || 0 == strcmp(Parameter_List[index].String_Value, "--help"))
        {
            index++;
            sigma_help();
            return QAPI_OK;
        }
        else
        {
            SIGMA_PRINTF("Sigma UDP don't support comand parameter %s\n",Parameter_List[index].String_Value);
            index++;
            return QAPI_ERR_INVALID_PARAM;
        }
    }
    
    p_tCxt = malloc(sizeof(SIGMA_CXT));
    if (p_tCxt == NULL)
    {
        SIGMA_PRINTF("Sigma UDP context memory alloc failed\n");
        return QAPI_ERR_NO_MEMORY;
    }
    memset(p_tCxt, 0, sizeof(SIGMA_CXT));

    if (operation_mode == SIGMA_TRAFFIC_CLIENT)
    {
        sigma_tx_quit = 0;
        p_tCxt->params.tx_params.ip_address = dest_ip_address;
        p_tCxt->params.tx_params.ip_tos = ip_tos;
        p_tCxt->params.tx_params.packet_size = pktSize;
        p_tCxt->params.tx_params.test_mode = test_mode;
        p_tCxt->params.tx_params.tx_time = tx_time;
        p_tCxt->params.tx_params.packet_number = numOfPkts;
        p_tCxt->params.tx_params.interval_ms = packet_interval_ms;
        p_tCxt->params.tx_params.source_ipv4_addr = bind_ip_address;

        if (mcastEnabled)
        {
            p_tCxt->params.rx_params.mcEnabled = 1;
        }

        sigma_SetParams(p_tCxt, "udp", port, SIGMA_TX);
        p_tCxt->protocol = PROT_UDP;
        p_tCxt->test_type = SIGMA_TX;
        /* create thread for sigma udp tx*/
        if (nt_qurt_thread_create(sigma_udp_tx, "sigma_udp_tx", 3072, p_tCxt, TCPIP_THREAD_PRIO, NULL) != pdPASS)
        {
            err = -1;
            SIGMA_PRINTF("Sigma UDP client task creation failed\r\n");
            goto end;
        }

        
    }
    else if (operation_mode == SIGMA_TRAFFIC_SERVER)
    {
        sigma_rx_quit = 0;

        if (mcastEnabled)
        {
            p_tCxt->params.rx_params.mcEnabled = 1;
            p_tCxt->params.rx_params.mcIpaddr = bind_ip_address;
          //p_tCxt->params.rx_params.local_address = local_address;
        }
        else
        {
            p_tCxt->params.rx_params.local_address = bind_ip_address;

        }

        if (sigma_SetParams(p_tCxt, "udp", port, SIGMA_RX) != 0)
        {
            return QAPI_ERR_INVALID_PARAM;
        }
        memset(&p_tCxt->pktStats, 0, sizeof(SIGMA_STATS));
      
        p_tCxt->protocol = PROT_UDP;
        p_tCxt->test_type = SIGMA_RX;
        p_tCxt->echo = echo;
        if (nt_qurt_thread_create(sigma_udp_rx, "sigma_udp_rx", 3072, p_tCxt, TCPIP_THREAD_PRIO, NULL) != pdPASS)
        {
            err = -1;
            SIGMA_PRINTF("Sigma UDP server task creation failed\r\n");
            goto end;
        }
        
    }
    else
    {
        err = -1;
        sigma_help();
        goto end;
    }
end:

  	if (err) {
        if (p_tCxt) {
            free(p_tCxt);
            p_tCxt = NULL;
        }
		return QAPI_ERR_INVALID_PARAM;
	}
    return QAPI_OK;
}

/************************************************************************
 *            [0]          [1]
 * udpquit [rx | tx] [session id]
 ************************************************************************/
qapi_Status_t udpquit(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    if (Parameter_Count == 0)
    {
        sigma_tx_quit = 1;
        sigma_rx_quit = 1;
        return QAPI_OK;
    }

    if (Parameter_Count > 3)
    {
        return QAPI_ERR_INVALID_PARAM;
    }

    if (strcasecmp("-s", Parameter_List[0].String_Value) == 0)
    {
        if (Parameter_Count < 2)
        {
            return QAPI_ERR_INVALID_PARAM;
        }
    }
    else if (strcasecmp("-c", Parameter_List[0].String_Value) == 0)
    {
        sigma_tx_quit = 1;
    }
    else
    {
        return QAPI_ERR_INVALID_PARAM;
    }

    return QAPI_OK;
}

/************************************************************************
* NAME: send_ack
*
* DESCRIPTION:
* In the UDP receive test, the test is terminated on receiving
* an 8-byte endMark UDP packet with the 1st word in the payload = 0xaabbccdd.
* We have implemented a feedback mechanism where we (the server) will ack
* the endMark with a packet containing RX stats allowing client to display
* correct test results. The Ack packet will contain time duration and number
* of bytes received.
* Implementation details-
* 1. Client sends endMark packet, then waits for 500 ms for a response.
* 2. We, on receiving endMark, send ACK (containing RX stats), and waits
*    1000 ms to check for more incoming packets.
* 3. If the client receives this ACK, it will stop sending endMark packets.
* 4. If we do not see the endMark packet for 1000 ms, we will assume SUCCESS
*    and exit gracefully.
* 5. Each side makes 40 attempts.
************************************************************************/
#define MAX_ACK_RETRY   40
void send_ack(SIGMA_CXT *p_tCxt, struct sockaddr *faddr, int addrlen)
{
    int send_result;
    int received;
    uint32_t retry = MAX_ACK_RETRY;

    stat_packet_t *stat;
    char pktbuf[ sizeof(stat_packet_t) ];
    int pktlen;

    uint32_t last_time_ms   = p_tCxt->pktStats.last_time;
    uint32_t first_time_ms  = p_tCxt->pktStats.first_time;
    uint32_t total_interval = last_time_ms - first_time_ms;
    uint16_t port_save = 0;
   	struct timeval tv;
    
    stat = (stat_packet_t *)pktbuf;
    pktlen = sizeof(stat_packet_t);

    /* Change client's rx port to be the same as our rx port
     * because client will receive the ACK from us on this port.
     */
    port_save = ((struct sockaddr_in *)faddr)->sin_port;
    ((struct sockaddr_in *)faddr)->sin_port = htons(p_tCxt->params.rx_params.port);


    stat->kbytes      = p_tCxt->pktStats.bytes/1024;
    stat->bytes       = p_tCxt->pktStats.bytes;
    stat->msec        = total_interval;
    stat->numPackets  = p_tCxt->pktStats.pkts_recvd;
    tv.tv_sec = 0;
    tv.tv_usec = 1000000;
    while (retry)
    {
        {
            send_result = sendto(p_tCxt->sock_local, pktbuf, pktlen, 0, faddr, addrlen);
        }

        if (send_result < 0)
        {
        	uint32_t counter = 0;

        	do
        	{
				received = recvfrom(p_tCxt->sock_local, (char*)(&p_tCxt->buffer[0]), CFG_PACKET_SIZE_MAX_RX, MSG_DONTWAIT, NULL, NULL);
				counter++;
        	}
			while (received > 0 && counter < 10);

        	if (received > 0)
        	{
        		/* Peer continues to send data even after EOT, need to abort */
        		retry = 0;
        		break;
        	}
        }

        if ( send_result != pktlen )
        {
            SIGMA_PRINTF("Error while sending stat packet, e=%d\n", send_result);
            qurt_thread_sleep(1);
        }
        else /* Sending ACK is successful.  Now waiting 1000 ms for more endmark packets from client. */
        {
            int32_t conn_sock;
            fd_set rset;
#ifdef SEND_ACK_DEBUG
            SIGMA_PRINTF("%d sent ACK\n", retry);
#endif
            FD_ZERO(&rset);
            FD_SET(p_tCxt->sock_local, &rset);

            conn_sock = select(p_tCxt->sock_local + 1, &rset, NULL, NULL, &tv);
            if (conn_sock > 0)
            {
                {
                    received = recvfrom(p_tCxt->sock_local, (char*)(&p_tCxt->buffer[0]), CFG_PACKET_SIZE_MAX_RX, 0, NULL, NULL);
                }

                if (received > 0)
                {
                    SIGMA_PRINTF("ACK success (%u)\n", MAX_ACK_RETRY - retry);
                    break;
                }
#ifdef SEND_ACK_DEBUG
                SIGMA_PRINTF(" received %d\n", received);
#endif
            }
            else
            {
            	/* ACK dropped or lost, peer does not retry */
            	break;
            }
        }
        retry--;
    } /* while */

    if (retry == 0)
    {
        SIGMA_PRINTF("Did not receive ACK.\n");
    }


    ((struct sockaddr_in *)faddr)->sin_port = port_save;

    return;
}

const QAPI_Console_Command_t wificert_shell_cmds[] =
{
    // cmd_function    cmd_string               usage_string             description 
    {sigma_udp_cmd, "udp", "\n\nType \"udp -h\" to get more info on usage\n",
        "\nPerform IPv4 UDP TX/RX benchmarking test"}, 
    {udpquit, "udpquit", "\n\nudpquit [rx|tx]\n",
        "\nTerminate some or all ongoing benchmarking tests or force to close a special socket"},

};


const QAPI_Console_Command_Group_t wificert_shell_cmd_group = {WiFiCERT_SHELL_GROUP_NAME, sizeof(wificert_shell_cmds) / sizeof(QAPI_Console_Command_t), wificert_shell_cmds};

QAPI_Console_Group_Handle_t wificert_shell_cmd_group_handle;

void wificert_shell_init (void)
{

    wificert_shell_cmd_group_handle = QAPI_Console_Register_Command_Group(NULL, &wificert_shell_cmd_group);
    if (wificert_shell_cmd_group_handle)
    {
        SIGMA_PRINTF("WiFiCert Registered\n");
    }
}

#endif
