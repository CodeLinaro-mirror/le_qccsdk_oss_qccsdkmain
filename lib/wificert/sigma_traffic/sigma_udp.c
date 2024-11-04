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

#define SIGMA_UDP_PKTS_PER_DOT	1000 /* Produce a progress dot each X packets */


/*-------------------------------------------------------------------------
 * Variables
 *-----------------------------------------------------------------------*/

extern uint8_t sigma_tx_quit;
extern uint8_t sigma_rx_quit;
uint16_t sigma_udp_rx_port_in_use = 0;    /* Used to prevent two udp rx streams from using the same port */


/************************************************************************
 ************************************************************************/
static void app_get_time(uint32_t *time_ms)
{
    *time_ms = hres_timer_curr_time_ms();
}

uint32_t sigma_udp_IsPortInUse(uint16_t port)
{
    int ret = 0;
    if (sigma_udp_rx_port_in_use == port) {
        SIGMA_PRINTF("port %d is in use; use another port.\n", sigma_udp_rx_port_in_use);
        ret = 1;
    }
    else {
        sigma_udp_rx_port_in_use = port;
    }

    return ret;
}

/************************************************************************
* NAME: sigma_udp_rx
*
* DESCRIPTION: Start sigma UDP server.
************************************************************************/
void sigma_udp_rx(void *sigma_context)
{
    SIGMA_CXT *p_tCxt = (SIGMA_CXT *)sigma_context;
    if (p_tCxt == NULL) {
        SIGMA_PRINTF("sigma_context is NULL\n");
        goto ERROR_1;
    }

    int32_t  received;
    int32_t  conn_sock;
    struct sockaddr *addr;
    int addrlen;
    uint32_t fromlen;
    struct sockaddr *from;
    struct sockaddr_in  local_addr;

    struct sockaddr_in  foreign_addr;

    void *local_sin_addr;
    char ip_str[48];
    int family;
    uint16_t port;
    uint32_t cur_packet_number = 0;
    uint64_t send_bytes = 0; /* for UDP echo */
    SIGMA_STATS echo_stats;

    struct timeval tv;

    if ((p_tCxt->buffer = malloc(CFG_PACKET_SIZE_MAX_RX)) == NULL)
    {
        SIGMA_PRINTF("Out of memory error\n");
        goto ERROR_1;
    }

    port = p_tCxt->params.rx_params.port;

    family = AF_INET;
    from = (struct sockaddr *)&foreign_addr;
    addr = (struct sockaddr *)&local_addr;
    local_sin_addr = &local_addr.sin_addr.s_addr;
    fromlen = addrlen = sizeof(struct sockaddr_in);
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_port = htons(port);
    local_addr.sin_family = family;
    if (p_tCxt->params.rx_params.mcEnabled)
        local_addr.sin_addr.s_addr = htonl(IPADDR_ANY);
    else
        local_addr.sin_addr.s_addr = p_tCxt->params.rx_params.local_address;

    /* Open socket */
    if ((p_tCxt->sock_local = socket(family, SOCK_DGRAM, IPPROTO_IP)) == A_ERROR)
    {
        SIGMA_PRINTF("ERROR: Socket creation error.\n");
        goto ERROR_1;
    }
    SIGMA_PRINTF("UDP local socket handle:%d\n",p_tCxt->sock_local);

    /* Bind */
    if (bind( p_tCxt->sock_local, addr, addrlen) != QAPI_OK)
    {
        SIGMA_PRINTF("ERROR: Socket bind error.\n");
        goto ERROR_2;
    }

    if (p_tCxt->params.rx_params.mcEnabled)
    {
        struct ip_mreq group;
        memset(&group, 0, sizeof(struct ip_mreq));
        group.imr_multiaddr.s_addr = p_tCxt->params.rx_params.mcIpaddr;

        group.imr_interface.s_addr = p_tCxt->params.rx_params.local_address;


        if (setsockopt(p_tCxt->sock_local, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void *)&group, sizeof(group)) != QAPI_OK)
        {
            SIGMA_PRINTF("ERROR: Socket set option failure.\n");
            goto ERROR_2;
        }
    }

    memset(ip_str,0,sizeof(ip_str));

    /* ------ Start test.----------- */
    SIGMA_PRINTF("****************************************************\n");
    SIGMA_PRINTF("IOT UDP RX Test\n");
    SIGMA_PRINTF("****************************************************\n");
    
    const char* address_str = p_tCxt->params.rx_params.mcEnabled ? \
                            inet_ntop(family, &(p_tCxt->params.rx_params.local_address), ip_str, sizeof(ip_str)) : \
                            inet_ntop(family, local_sin_addr, ip_str, sizeof(ip_str));
    
    if (address_str) {
        SIGMA_PRINTF("Bind address: %s\n", ip_str);
    } else {
        SIGMA_PRINTF("Bind address: 0x0\n"); 
    }

    SIGMA_PRINTF("Local port: %d\n", port);
    SIGMA_PRINTF("Type udpquit to terminate test\n");
    SIGMA_PRINTF("****************************************************\n");

    memset(&echo_stats, 0, sizeof(SIGMA_STATS));

    tv.tv_sec = 1;
    tv.tv_usec = 1000;

    while (!sigma_rx_quit) /* Main loop */
    {
        int32_t is_first = 1;

        SIGMA_PRINTF("Waiting\n");

        sigma_clear_stats(p_tCxt);
        memset(ip_str,0,sizeof(ip_str));

        cur_packet_number = 0;
        send_bytes = 0; /* for UDP echo */

        while (!sigma_rx_quit)   /* Receive loop */
        {
            int32_t i = 0;  /* for debugging use */
            do
            {
                fd_set rset;

                if (sigma_rx_quit)
                {
					app_get_time(&p_tCxt->pktStats.last_time);
                    goto ERROR_3;
                }

                /* block for 500msec or until a packet is received */
                memset(&rset, 0, sizeof(fd_set));
                FD_SET(p_tCxt->sock_local, &rset);

                conn_sock = select(p_tCxt->sock_local + 1, &rset, NULL, NULL, &tv);
                if (conn_sock == A_ERROR)
                    goto ERROR_3;       // socket no longer valid

                if(family == AF_INET && errno == ENOTSOCK)
                    goto ERROR_2;
            } while (conn_sock == 0);

            /* Receive data */
            received = recvfrom( p_tCxt->sock_local,
                    (char*)(&p_tCxt->buffer[0]),
                    CFG_PACKET_SIZE_MAX_RX, 0,
                    from, &fromlen);
            ++i;

            if (received >= 0)
            {
                if (received != sizeof(EOT_PACKET) ||
                    ((EOT_PACKET *)p_tCxt->buffer)->code != (int)END_OF_TEST_CODE)
                {
#ifdef UDP_RX_STATUS_DEBUG
                    uint32_t ts;

                    app_get_time(&ts);
                    SIGMA_PRINTF("ms=%d rcvd=%d\n", i, ts, received);
#endif
                    p_tCxt->pktStats.bytes += received;
                    ++p_tCxt->pktStats.pkts_recvd;

                    if (is_first)
                    {
                        SIGMA_PRINTF("Receiving\n");

                        app_get_time(&p_tCxt->pktStats.first_time);

#ifdef UDP_RX_TS_DEBUG
                        SIGMA_PRINTF("first_time: ms=%d\n",
                                p_tCxt->pktStats.first_time);
#endif

                        is_first = 0;
                    }

                    if (p_tCxt->echo) {
                        int ret;
                        /* Echo the buffer back to the sender (best effort, no retransmission). */
                        ret = sendto(p_tCxt->sock_local, p_tCxt->buffer, received, MSG_DONTWAIT, from, fromlen);

                        if (ret > 0) {
                            send_bytes += ret;
                            cur_packet_number++;
                        }
                    }

                }
                else if (!is_first) /* End of transfer. */
                {
                    app_get_time(&p_tCxt->pktStats.last_time);

#ifdef UDP_RX_TS_DEBUG
                    SIGMA_PRINTF("send acklast_time: ms=%d rcvd=%d\n",
                            p_tCxt->pktStats.last_time, received);
#endif
#ifdef UDP_ENDMARK_ENABLE

                    /* Send throughput results to Peer so that it can display correct results*/
                    send_ack(p_tCxt, from, fromlen);
#endif
                    break;
                }
            }
            else
            {
                SIGMA_PRINTF("%d received= %d\n", i, received);
                break;
            }
        } /* receive_loop */

ERROR_3:
        app_get_time(&p_tCxt->pktStats.last_time);
        
        const char* address_str = inet_ntop(family, (void *)&((struct sockaddr_in *)from)->sin_addr.s_addr, ip_str, sizeof(ip_str));
        if (address_str){
           SIGMA_PRINTF("Received %u bytes, Packets %u from %s:%u\n",
                (uint32_t)p_tCxt->pktStats.bytes, 
                p_tCxt->pktStats.pkts_recvd,
                *address_str,
                ntohs(((struct sockaddr_in *)from)->sin_port));
        } else {
            SIGMA_PRINTF("Received %u bytes, Packets %u from %s:%u\n",
                (uint32_t)p_tCxt->pktStats.bytes, 
                p_tCxt->pktStats.pkts_recvd,
                "0x0",
                ntohs(((struct sockaddr_in *)from)->sin_port));            
        }

        sigma_print_test_results(p_tCxt, &p_tCxt->pktStats);

        if (p_tCxt->echo)
        {
            if (send_bytes > 0)
            {
                echo_stats = p_tCxt->pktStats;
                echo_stats.bytes = send_bytes;
            }

            SIGMA_PRINTF("\nSent %u packets, %u bytes\n",
                    cur_packet_number, send_bytes);

            p_tCxt->test_type = SIGMA_TX;
            sigma_print_test_results(p_tCxt, &echo_stats);
            p_tCxt->test_type = SIGMA_RX;
        }

    } /* main loop */

ERROR_2:
	closesocket(p_tCxt->sock_local);

ERROR_1:

	sigma_udp_rx_port_in_use = 0;

    SIGMA_PRINTF(SIGMA_TEST_COMPLETED);
    
    if (p_tCxt)
    {
        if (p_tCxt->buffer)
        {
            free(p_tCxt->buffer);
            p_tCxt->buffer = NULL;
        }
        free(p_tCxt);
        p_tCxt = NULL;
    }

    nt_osal_thread_delete(NULL);
    return;
}

/************************************************************************
* NAME: sigma_wait_for_response
*
* DESCRIPTION: In UDP uplink test, the test is terminated by transmitting
* end-mark (single byte packet). We have implemented a feedback mechanism
* where the Peer will reply with receive stats allowing us to display correct
* test results.
* Parameters: pointer to sigma context
************************************************************************/
#ifdef UDP_ENDMARK_ENABLE
int sigma_wait_for_response(SIGMA_CXT *p_tCxt, struct sockaddr *to, uint32_t tolen, uint32_t cur_packet_number)
{
    uint32_t received;
    int error = A_ERROR;
    struct sockaddr_in local_addr;

    struct sockaddr *addr = NULL;
    uint32_t addrlen = 0;
    stat_packet_t *stat_packet, stats;
    EOT_PACKET eot_packet, *endmark;
    uint32_t retry_counter = 0;
    int family = (int)to->sa_family;
    struct timeval tv;

    stat_packet = &stats;

    if (p_tCxt->test_type == SIGMA_TX && p_tCxt->protocol == PROT_UDP)
    {
        if (family == AF_INET)
        {
            memset(&local_addr, 0, sizeof(local_addr));
            /* Receive peer's ACK at original dest port */
            local_addr.sin_port = ((struct sockaddr_in *)to)->sin_port;
            local_addr.sin_family = family;
            addr = (struct sockaddr *)&local_addr;
            addrlen = sizeof(struct sockaddr_in);
        }

        /* Create socket */
        if ((p_tCxt->sock_local = socket(family, SOCK_DGRAM, 0)) == A_ERROR)
        {
            SIGMA_PRINTF("%s: Socket creation error\n", __func__);
            goto ERROR_1;
        }

        /* Bind */
        if (addr == NULL || bind(p_tCxt->sock_local, addr, addrlen) != QAPI_OK)
        {
            SIGMA_PRINTF("%s: Socket bind error\n", __func__);
            goto ERROR_2;
        }
    }
    tv.tv_sec = 0;
    tv.tv_usec = 2000000;

    while (retry_counter < 10)
    {
        int conn_sock, sent_bytes;
        fd_set rset;

        endmark = &eot_packet;

        /* Send endmark packet */
        ((EOT_PACKET*)endmark)->code            = HOST_TO_LE_LONG(END_OF_TEST_CODE);
        ((EOT_PACKET*)endmark)->packet_count    = htonl(cur_packet_number);

        {
            sent_bytes = sendto( p_tCxt->sock_peer, (char *)endmark, sizeof(EOT_PACKET), 0, to, tolen) ;
        }

        if (sent_bytes != sizeof(EOT_PACKET))
        	break;

        SIGMA_DEBUG_PRINTF("%d sent EOT_PACKET %d bytes\n", retry_counter, sent_bytes);

        FD_ZERO(&rset);

        FD_SET(p_tCxt->sock_local, &rset);


        if ((conn_sock = select(p_tCxt->sock_local + 1, &rset, NULL, NULL, &tv)) > 0)
        {
            /* Receive data */
            {
                received = recvfrom( p_tCxt->sock_local, (char*)(stat_packet), sizeof(stat_packet_t), 0, NULL, NULL);
            }

            if (received == sizeof(stat_packet_t))
            {
                SIGMA_DEBUG_PRINTF("%d received %u-byte statistics\n", retry_counter, received);
                error = QAPI_OK;

                break;
            }
            else
            {
                SIGMA_DEBUG_PRINTF("%d Did not receive response: %d\n", retry_counter, received);
                retry_counter++;
            }
        }
        else
        {
            uint32_t time_stamp = 0;
            app_get_time(&time_stamp);
            SIGMA_DEBUG_PRINTF("%d conn_sock=%d ms=%u\n", retry_counter, conn_sock, time_stamp);
            retry_counter++;
        }
    } /* while(retry_counter) */

ERROR_2:
    if (p_tCxt->protocol == PROT_UDP && p_tCxt->test_type == SIGMA_TX)
    {
        closesocket(p_tCxt->sock_local);
    }

ERROR_1:
    return error;
}
#endif
/************************************************************************
* NAME: sigma_udp_tx
*
* DESCRIPTION: Start sigma UDP transmit  test.
************************************************************************/
void sigma_udp_tx(void *sigma_context)
{
    SIGMA_CXT *p_tCxt = (SIGMA_CXT *)sigma_context;
    if (p_tCxt == NULL) {
        SIGMA_PRINTF("sigma_context is NULL\n");
        goto ERROR_1;
    }

    struct sockaddr_in foreign_addr;

    struct sockaddr *to;
    uint32_t tolen;
    char ip_str [48];
    int32_t send_bytes, result = QAPI_OK;
    uint32_t packet_size = p_tCxt->params.tx_params.packet_size;
    uint32_t cur_packet_number, i, n_send_ok;

    int family;
    int tos_opt;
    struct sockaddr_in src_sin;
    {
        family = AF_INET;
        inet_ntop(family, &p_tCxt->params.tx_params.ip_address, ip_str, sizeof(ip_str));

        memset(&foreign_addr, 0, sizeof(foreign_addr));
        foreign_addr.sin_addr.s_addr    = p_tCxt->params.tx_params.ip_address;
        foreign_addr.sin_port           = htons(p_tCxt->params.tx_params.port);
        foreign_addr.sin_family         = family;

        src_sin.sin_family              = family;
        src_sin.sin_addr.s_addr         = p_tCxt->params.tx_params.source_ipv4_addr;
        src_sin.sin_port                = htons(0);

        to = (struct sockaddr *)&foreign_addr;
        tolen = sizeof(foreign_addr);
        tos_opt = IP_TOS;
    }

    /* ------ Start test.----------- */
    SIGMA_PRINTF("****************************************************************\n");
    SIGMA_PRINTF("IOT UDP TX Test\n");
    SIGMA_PRINTF("****************************************************************\n");
    SIGMA_PRINTF("Remote IP addr: %s\n", ip_str);
    SIGMA_PRINTF("Remote port: %d\n", p_tCxt->params.tx_params.port);
    SIGMA_PRINTF("Message size: %d\n", p_tCxt->params.tx_params.packet_size);
    SIGMA_PRINTF("Number of messages: %d\n", p_tCxt->params.tx_params.packet_number);
    SIGMA_PRINTF("Delay in milliseconds: %u\n", p_tCxt->params.tx_params.interval_ms);
    SIGMA_PRINTF("Type udpquit to terminate test\n");
    SIGMA_PRINTF("****************************************************************\n");


    /* Create UDP socket */
    if ((p_tCxt->sock_peer = socket(family, SOCK_DGRAM, 0)) == A_ERROR)
    {
        SIGMA_PRINTF("Socket creation failed\n");
        goto ERROR_1;
    }
    SIGMA_PRINTF("handle: %d\n", p_tCxt->sock_peer);

	if (p_tCxt->params.tx_params.source_ipv4_addr != 0) {
        SIGMA_PRINTF("Socket bind %x\n",p_tCxt->params.tx_params.source_ipv4_addr);
		if (bind(p_tCxt->sock_peer, (struct sockaddr*)&src_sin, sizeof(src_sin)) != QAPI_OK) {
			SIGMA_PRINTF("Socket bind failed\n");
			goto ERROR_2;
		}
	}

    if (p_tCxt->params.tx_params.ip_tos > 0)
    {
	    setsockopt(p_tCxt->sock_peer, IPPROTO_IP, tos_opt, &p_tCxt->params.tx_params.ip_tos, sizeof(int));
    }
    // if (p_tCxt->params.tx_params.is_so_unblock)
    // {
    //     setsockopt(p_tCxt->sock_peer, SOL_SOCKET, O_NONBLOCK, NULL, 0);
	// 	SIGMA_PRINTF("non-blocking mode\n");
    // }    

    /* Connect to the server.*/
    SIGMA_PRINTF("Connecting\n");
    if (connect( p_tCxt->sock_peer, to, tolen) == A_ERROR)
    {
        SIGMA_PRINTF("Connection failed\n");
        goto ERROR_2;
    }

    /* Sending.*/
    SIGMA_PRINTF("Sending\n");

    /*Reset all counters*/
    cur_packet_number = 0;
    i = SIGMA_UDP_PKTS_PER_DOT;
    n_send_ok = 0;

    app_get_time(&p_tCxt->pktStats.first_time);

    uint32_t is_test_done = 0;
    while ( !is_test_done )
    {
        if (sigma_tx_quit)
        {
            app_get_time(&p_tCxt->pktStats.last_time);
            break;
        }

        /* allocate the buffer, if needed */
        if ( p_tCxt->buffer == NULL )
        {
            while ((p_tCxt->buffer = malloc(packet_size)) == NULL)
            {
                /*Wait till we get a buffer*/
                if (sigma_tx_quit)
                {
                    app_get_time(&p_tCxt->pktStats.last_time);
                    goto ERROR_2;
                }
                /*Allow small delay to allow other thread to run*/
                qurt_thread_sleep(100);
            }
            memset(p_tCxt->buffer, 0, packet_size);

        }

        do
        {
            if (sigma_tx_quit)
            {
                app_get_time(&p_tCxt->pktStats.last_time);
                is_test_done = 1;
                break;
            }
            {

            if (packet_size >= 4)
            {
                /* Packet index */
                uint32_t *index_ptr = (uint32_t *)p_tCxt->buffer;
                *index_ptr = htonl(cur_packet_number);
            }
            if (packet_size >= 12)
            {
                uint32_t *time_ptr = ((uint32_t *)p_tCxt->buffer) + 2;
                app_get_time(time_ptr);
            }
                send_bytes = send(p_tCxt->sock_peer, p_tCxt->buffer, packet_size, 0);
            }

            if ( send_bytes != (int)packet_size )
            {

                if (ENOMEM == errno || ERR_INPROGRESS == errno)
                {
                    qurt_thread_sleep(5);
                }
                else
                {
                    SIGMA_PRINTF("TX timeout\n");
                    app_get_time(&p_tCxt->pktStats.last_time);
                    if(ERR_TIMEOUT == errno || ERR_OK == errno)
                    {
                        if(!sigma_check_test_time(p_tCxt))
                            continue;
                    }

                    is_test_done = 1;

                    break;
                }

            }
            else
            {
                cur_packet_number ++;
            }

            if (++i >= SIGMA_UDP_PKTS_PER_DOT)
            {
                SIGMA_PRINTF(".");
                i = 0;
            }

#ifdef UDP_TX_DEBUG
            SIGMA_PRINTF("%d send_bytes = %d\n", cur_packet_number, send_bytes);
#endif

            if (send_bytes > 0)
            {
                p_tCxt->pktStats.bytes += send_bytes;
                ++n_send_ok;
            }

            /*Test mode can be "number of packets" or "fixed time duration"*/
            if (p_tCxt->params.tx_params.test_mode == SIGMA_PACKET_TEST)
            {
                if ((cur_packet_number >= p_tCxt->params.tx_params.packet_number))
                {
                    app_get_time(&p_tCxt->pktStats.last_time);
                    is_test_done = 1;
                    break;
                }
            }
            else if (p_tCxt->params.tx_params.test_mode == SIGMA_TIME_TEST)
            {
                app_get_time(&p_tCxt->pktStats.last_time);
                if (sigma_check_test_time(p_tCxt))
                {
                    is_test_done = 1;
                    break;
                }
            }

            if (p_tCxt->params.tx_params.interval_ms)
            {
                qurt_thread_sleep(p_tCxt->params.tx_params.interval_ms);
            }

        } while ( !((is_test_done) || (send_bytes == (int)packet_size) || (NULL == p_tCxt->buffer)) );   /* send loop */

    } /* while ( !is_test_done ) */

    if ( p_tCxt->buffer )
    {
        free(p_tCxt->buffer);
        p_tCxt->buffer = NULL;
    }

    SIGMA_PRINTF("\nSent %u packets, %u bytes to %s %u (%u)\n",
        cur_packet_number, (uint32_t)p_tCxt->pktStats.bytes, ip_str, p_tCxt->params.tx_params.port, cur_packet_number - n_send_ok);

#ifdef UDP_ENDMARK_ENABLE
    /* Send endmark packet and wait for stats from server */
    result = sigma_wait_for_response(p_tCxt, to, tolen, cur_packet_number);
#endif

    if (result != QAPI_OK)
    {
        SIGMA_DEBUG_PRINTF("UDP Transmit test failed, did not receive Ack from Peer\n");
    }

ERROR_2:

    sigma_print_test_results(p_tCxt, &p_tCxt->pktStats);

    closesocket(p_tCxt->sock_peer);

ERROR_1:
    SIGMA_PRINTF(SIGMA_TEST_COMPLETED);

    if (p_tCxt)
    {
        free(p_tCxt);
        p_tCxt = NULL;
    }

    nt_osal_thread_delete(NULL);
    return;
}

#endif
