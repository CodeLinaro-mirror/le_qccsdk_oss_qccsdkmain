/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 * Copyright (c) 2019 Alfredo Cavallara
 */

/*
* Redistribution and use in source and binary forms, with or without modification, 
* are permitted provided that the following conditions are met:
*
* 1. Redistributions of source code must retain the above copyright notice,
*    this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright notice,
*    this list of conditions and the following disclaimer in the documentation
*    and/or other materials provided with the distribution.
* 3. The name of the author may not be used to endorse or promote products
*    derived from this software without specific prior written permission. 
*
* THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED 
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
* SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
* OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
* IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
* OF SUCH DAMAGE.
*
* This file is part of the lwIP TCP/IP stack.
* 
*/
/*-------------------------------------------------------------------------
 * Include Files
 *-----------------------------------------------------------------------*/
#include "lwip/opt.h"

#if LWIP_RAW /* don't build if not configured for use in lwipopts.h */
#include "pmtud.h"

#include <stdio.h>
#include <sys/time.h>
#include "lwip/sockets.h"
#include "lwip/def.h"
#include "lwip/raw.h"
#include "lwip/icmp.h"
#include "lwip/netif.h"
#include "lwip/sys.h"
#include "lwip/timeouts.h"
#include "lwip/inet_chksum.h"
#include "lwip/prot/ip4.h"
#include "lwip/prot/ip.h"
#include "lwip/init.h"
#include "qapi_status.h"

/*-------------------------------------------------------------------------
 * Preprocessor Definitions, Constants, and Type Declarations
 *-----------------------------------------------------------------------*/

static ip_addr_t* pmtud_target;
static u16_t icmp_seq_num = 0;
static u16_t ip_identification;

#define MIN_MTU           1000
#define LOCAL_MTU         1500
#define TIMEOUT           900000
#define MTU_MUX_RETRY     10

/**
 * PMTU_DEBUG: Enable debugging for PMTUD.
 */
#if !PMTUD_DEBUG_ENABLE
#define PMTUD_PRINTF(...) printf(__VA_ARGS__)
#else
#define PMTUD_PRINTF(...)
#endif
/*-------------------------------------------------------------------------
 * Function Definitions
 *-----------------------------------------------------------------------*/
 /**
 * @brief Prepare a echo ICMP request
 *
 * @param pktbuff Pointer to the data buffer.
 * @param packet_size Length of the data packet.
 * @return 
 */
static void pmtud_prepare_echo(icmp_echo_header *icmp_header, u16_t packet_size)
{
    size_t i;

    size_t icmp_len = packet_size - sizeof(icmp_echo_header);

    /*ICMP header*/
    // LWIP_IPV4
    if(IP_IS_V4(pmtud_target)) {
        ICMPH_TYPE_SET(icmpm_2_icmp(icmp_header), ICMP_ECHO);
    }

    ICMPH_CODE_SET(icmpm_2_icmp(icmp_header), 0);

    icmpm_2_icmpg(icmp_header)->chksum = 0;

    icmpm_2_icmpg(icmp_header)->id     = lwip_htons(0x1234);

    icmpm_2_icmpg(icmp_header)->seqno  = lwip_htons(++icmp_seq_num);
  
    // /* fill the additional data buffer with some data */
    for(i = 0; i < icmp_len; i++) {
        ((char*)icmp_header)[sizeof(struct icmp_echo_header) + i] = (char)i;
    }

    icmpm_2_icmpg(icmp_header)->chksum = inet_chksum(icmpm_2_icmpg(icmp_header), packet_size);
}

 /**
 * @brief Pmtud send packet
 * 
 * This function sends the ICMP packet by raw socket.
 *
 * @param s socket process.
 * @param addr the point to dest ip address.
 * @param packet_size Length of the data packet.
 * @return the send status error
 */
static err_t
pmtud_send(int32_t s, const ip_addr_t *addr, int32_t packet_size)
{
    int32_t err;
    struct sockaddr_storage to;
    struct icmp_echo_header *icmp_header;
    
    size_t pmtud_size = sizeof(struct icmp_echo_header) + packet_size;

    icmp_header = (struct icmp_echo_header *)mem_malloc((mem_size_t)pmtud_size);
    if (!icmp_header) {
        return ERR_MEM;
    }

    pmtud_prepare_echo(icmp_header, (u16_t)pmtud_size);

    if(IP_IS_V4(addr)) {
        struct sockaddr_in *to4 = (struct sockaddr_in*)&to;
        to4->sin_len    = sizeof(to4);
        to4->sin_family = AF_INET;
        inet_addr_from_ip4addr(&to4->sin_addr, ip_2_ip4(addr));
    }

    PMTUD_PRINTF("send mtud_packe:%d\n", pmtud_size);

    err = lwip_sendto(s, icmp_header, pmtud_size, 0, (struct sockaddr*)&to, sizeof(to));
    if (err < 0) {
        mem_free(icmp_header);
        return ERR_VAL;
    }

    mem_free(icmp_header);
    return ERR_OK;
}

 /**
 * @brief Handle ICMP echo Response
 * 
 * This function handles ICMP echo Response.
 *
 * @param buffer receive buffer.
 * @return handle the icmp echo response.
 */
int32_t handle_icmp_response(char *buffer)
{
    struct ip_hdr *ip_header = (struct ip_hdr *)buffer;
    struct icmp_echo_header *icmp_header = (struct icmp_echo_header *)(buffer + (IPH_HL(ip_header) * 4));
	// returns 1 if the packet comes from the specified host and it's valid
	if (ICMPH_TYPE(icmpm_2_icmp(icmp_header)) == ICMP_ER) // valid if the source addr is server's
	{
        PMTUD_PRINTF("receive from server\n");
	}
	else if (ICMPH_TYPE(icmpm_2_icmp(icmp_header)) == ICMP_DUR) // some kind of error occurred
	{
        PMTUD_PRINTF("dest unreadhcale\n");
		return -(ICMPH_CODE(icmpm_2_icmp(icmp_header))); // return error type
	}
	else // unhandled ICMP packet
    {
        PMTUD_PRINTF("discard\n");
        return -256;
    }
		
	return 1; // success
}

 /**
 * @brief Implements Path MTU Discovery.
 *
 * This function performs Path MTU Discovery to determine the MTU size on the network path between the source and destination.
 *
 * @param ip_addr Destination IP address.
 * @param mtu the return data to take the mtu from handle.
 * @return The discovered Path MTU size.
 */

int32_t Path_MTU_Discover(ip_addr_t *ip_addr)
{
    int32_t s;
    int32_t current_mtu;
    int32_t new_mtu;
    new_mtu = MTU_ERR_TIMEOUT;  // we do not know if the server is up and reachable
    int32_t low = MIN_MTU;
    int32_t high = LOCAL_MTU - IP_HLEN;
    int32_t status = 0;

    pmtud_target = (ip_addr_t *)ip_addr;
    
    // struct sockaddr_storage from;
    struct sockaddr_in from;
    socklen_t fromlen = sizeof(from);
    int32_t sel, res;

    uint32_t retry_counter = MTU_MUX_RETRY;
    int32_t recv_len;
    fd_set fds;
    FD_ZERO(&fds);

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = TIMEOUT;

    s = lwip_socket(AF_INET, SOCK_RAW, IP_PROTO_ICMP);

    if (s < 0) {
        PMTUD_PRINTF("Socket creation failed");
        return MTU_ERR_SOCK;
    }
    
    char *buffer = malloc(LOCAL_MTU);

    while (low <= high) {
        current_mtu = (low + high) / 2;
        if (pmtud_send(s, pmtud_target, current_mtu) == ERR_OK) {
            PMTUD_PRINTF("pmtud: send ");
            ip_addr_debug_print(PMTUD_DEBUG_ENABLE, pmtud_target);
            PMTUD_PRINTF("\n");
            PMTUD_PRINTF("send successful\n");
        } 
        else
        {
            PMTUD_PRINTF("packet too big for local interface\n");
            low = current_mtu - 1;
            status = MTU_ERR_SOCK;
            goto fail;
        } 
        FD_SET(s, &fds);
        sel = select(s + 1, &fds, NULL, NULL, &timeout);
        if (sel < 0) {
            PMTUD_PRINTF("lwip_select error");
            retry_counter++;
        } 
        else if (sel == 0) {
            PMTUD_PRINTF("Timeout occurred! No data available.\n");
            retry_counter++;
            high = current_mtu - 1;
        } else {
            recv_len = lwip_recvfrom(s, buffer, LOCAL_MTU, 0, (struct sockaddr*)&from, &fromlen);
            if (recv_len < 0) {
                // timeout: packet got lost or server is down
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    if (--retry_counter == 0) {
                        printf("no response, invalid MTU size\n");
                        retry_counter = MTU_MUX_RETRY;
                        // update the mtu
                        low = current_mtu + 1;
                    }
                    continue;
                }
                PMTUD_PRINTF("Socket recv failed");
                status = MTU_ERR_SOCK;
            }
            // a packet has been received, check if it's valid
            res = handle_icmp_response(buffer);
            if (res > 0) // success, the packet comes from the server and it's valid
            {
                PMTUD_PRINTF("valid\n");
                retry_counter = MTU_MUX_RETRY; // server is up, mtu_current is valid: reset retry counter
                low = current_mtu + 1; // update the mtu
                if (current_mtu > new_mtu)
                    new_mtu = current_mtu;
		    }
		    else if (res == 0) // this packet comes from another source, discard it and retry
		    {
			    continue;
		    }
		    else // ICMP error message or unknown packet: either way, lower the range and go on
		    {
			    switch(res)
			    {
                    case -1:    PMTUD_PRINTF("error:host unreachable\n"); break;
                    case -3:    PMTUD_PRINTF("error:port unreachable\n"); break;
                    case -4:    PMTUD_PRINTF("error:fragmentation needed\n"); break;
                    case -256:  PMTUD_PRINTF("unknown error\n"); break;
                    default:    PMTUD_PRINTF("unknown ICMP error\n"); break;
			    }

			    retry_counter = MTU_MUX_RETRY;
			    high = current_mtu - 1; // update the mtu
		    }
        }
	}
    closesocket(s);
    free(buffer);
    return new_mtu + IP_HLEN;
fail:
    free(buffer);
    return status;
    
}

 /**
 * @brief Implements Path MTU Discovery.
 *
 * This function performs Path MTU Discovery to determine the MTU size on the network path between the source and destination.
 *
 * @param ip_addr Destination IP address.
 * @param mtu the return data to take the mtu from handle.
 * @return The discovered Path MTU size.
 */
int32_t
qapi_Path_MTU_Discover(ip_addr_t *ip_addr)
{
    int32_t mtu;
    
    if (ip_addr == NULL)
    {
        return QAPI_ERR_INVALID_PARAM;
    }
    mtu = Path_MTU_Discover(ip_addr);
    if (mtu < 0) {
        return QAPI_ERROR;
    }

    return mtu;
}

#endif /* LWIP_RAW */
