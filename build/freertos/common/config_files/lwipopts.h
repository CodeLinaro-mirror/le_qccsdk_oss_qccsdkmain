/*
 * Copyright (c) 2001-2003 Swedish Institute of Computer Science.
 * All rights reserved.
 *
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
 * Author: Simon Goldschmidt
 *
 */
#ifndef LWIP_HDR_LWIPOPTS_H__
#define LWIP_HDR_LWIPOPTS_H__
#include "wifi_cmn.h"
#include "fwconfig_cmn.h"
#include "nt_flags.h"
#ifdef MEM_CPY_VIA_DXE
#include <stdint.h>
#ifdef NT_FN_RRAM_PERF_BUILD
__attribute__ ((section(".perf_cm_txt"))) void *nt_dpm_memcpy(void *dst, const void *src, uint32_t length);
#else
void *nt_dpm_memcpy(void *dst, const void *src, uint32_t length);
#endif
#endif /* MEM_CPY_VIA_DXE */

#ifdef NT_FN_DPM_DEBUG
#define LWIP_STATS_DISPLAY              1
#endif
#define LWIP_SINGLE_NETIF               0

#define LWIP_NETIF_STATUS_CALLBACK      1

//#define MEM_SIZE                        3000
#define LWIP_BROADCAST_PING             1
#define LWIP_MULTICAST_PING             1
#define LWIP_RAW                        1
#define LWIP_NETIF_API                  1
#define TCPIP_THREAD_STACKSIZE          600	/** TCPIP thread stack size. */
#define TCPIP_THREAD_PRIO               6		/** TCPIP thread priority. */
#define TCPIP_MBOX_SIZE                 64		/** mailbox size for the tcpip thread messages. */
#define DEFAULT_THREAD_STACKSIZE        3000
#define DEFAULT_THREAD_PRIO             3
#define LWIP_NETCONN                    0
#define MEMP_NUM_UDP_PCB                5		/** Maximum number of PCB allocation for UDP allowed at any instance in system */
#define DEFAULT_TCP_RECVMBOX_SIZE		64		/** mailbox size for the incoming packets on a NETCONN_TCP. */
#define DEFAULT_UDP_RECVMBOX_SIZE		64		/** mailbox size for the incoming packets on a NETCONN_UDP. */
#define DEFAULT_RAW_RECVMBOX_SIZE		64		/** mailbox size for the incoming packets on a NETCONN_RAW. */
#define DEFAULT_ACCEPTMBOX_SIZE			64		/** mailbox size for the incoming connections in case of TCP listen. */

#define MEMP_NUM_TCPIP_MSG_INPKT		75		/** Number of struct tcpip_msg, which are used for incoming packets. */

#define MEMP_NUM_NETBUF                 8       /** Number of struct netbufs. */

#define IP_REASS_MAXAGE                 3

#define CHECKSUM_GEN_IP                 1
#define CHECKSUM_GEN_UDP                1
#define CHECKSUM_GEN_TCP                1
#define CHECKSUM_GEN_ICMP               1
#define CHECKSUM_GEN_ICMP6              1

/* Prevent having to link sys_arch.c (we don't test the API layers in unit tests) */
#define NO_SYS                          0
#define LWIP_SOCKET                     1
#define SYS_LIGHTWEIGHT_PROT            1

#ifdef NT_FN_IPV6
#define LWIP_IPV6                       1
#define LWIP_IPV6_FRAG                  0
#define LWIP_IPV6_REASS                 1
#if NT_FN_DHCP6
#define LWIP_IPV6_DHCP6                 1
#ifdef SUPPORT_RING_IF
#define LWIP_IPV6_DHCP6_STATELESS       1
#endif
#endif

#ifdef NT_TST_MULTICAST_IPV6_EN
#define LWIP_IPV6_MLD                   1
#else
#define LWIP_IPV6_MLD                   0
#endif /* NT_TST_MULTICAST_IPV6_EN */

#define IPV6_FRAG_COPYHEADER            0
#define LWIP_IPV6_DUP_DETECT_ATTEMPTS   0
#define MEMP_NUM_ND6_QUEUE              3

/*Enable RA for AP mode*/
#define LWIP_IPV6_SEND_ROUTER_ADVERTISE 1

#define LWIP_ND6_MAX_INITIAL_RA         3
#define LWIP_ND6_INITIAL_RA_INTERVAL    2
#define LWIP_ND6_NORMAL_RA_INTERVAL     9 /* @todo: 10 for quick connect, 600 for low affect */

#endif /* NT_FN_IPV6 */

/* Enable some protocols to test them */
#define LWIP_IPV4						1
#if NT_FN_DHCPS_V4
#define LWIP_DHCP                       1
#define LWIP_AUTOIP                     1
#endif

#define LWIP_IGMP                       1

#if NT_FN_DNS
#define LWIP_DNS                        1
#endif

#ifdef NT_FN_MBEDTLS_APP
#define LWIP_ALTCP                      1
#define LWIP_ALTCP_TLS                  1
#endif

#if defined(NT_TST_LWIP_STATS)
#define LWIP_STATS_DISPLAY              1
#endif

/* Turn off checksum verification of fuzzed data */
#define CHECKSUM_CHECK_IP               1
#define CHECKSUM_CHECK_UDP              1
#define CHECKSUM_CHECK_TCP              1
#define CHECKSUM_CHECK_ICMP             1
#define CHECKSUM_CHECK_ICMP6            1

/* Minimal changes to opt.h required for tcp unit tests: */
#define MEM_ALIGNMENT                   4U
#define TCP_MSS                         1460

#if (CONFIG_LWIP_MULTIPLE_STREAMS)
#define MEMP_NUM_PBUF                   128
#else
#define MEMP_NUM_PBUF                   64
#endif

#if defined LWIPERF_PERF_BUILD
#define MEM_SIZE                        120000
#else
#define MEM_SIZE                        60000
#endif

#if !CONFIG_MATTER_ENABLE
#define MEM_STATS                       1
#define MEMP_STATS                      1
#else
#define MEMP_STATS                      0
#define LWIP_DISABLE_TCP_SANITY_CHECKS  1
#endif

#ifdef HTTPS_SERVER
#define PBUF_POOL_SIZE                  16
#else
#define PBUF_POOL_SIZE                  24
#define PBUF_POOL_BUFSIZE               2500
#endif
#define MEMP_NUM_TCP_SEG                TCP_SND_QUEUELEN
#define TCP_OVERSIZE                    TCP_MSS
#define TCP_WND                         (24 * TCP_MSS)
#define TCP_SND_BUF                     TCP_WND
#define LWIP_WND_SCALE                  0
#define TCP_RCV_SCALE                   0
#define LWIP_NETIF_TX_SINGLE_PBUF       0
#define LWIP_TCPIP_CORE_LOCKING         1
#define LWIP_TCPIP_CORE_LOCKING_INPUT   0
#define MEMP_NUM_SYS_TIMEOUT            (1 + 1 + 1 + (2*LWIP_DHCP) + LWIP_AUTOIP + LWIP_IGMP + LWIP_DNS + 0 + (LWIP_IPV6 * (1 + LWIP_IPV6_REASS + LWIP_IPV6_MLD)) + 1/*lwiperf*/ + 1/*ping*/ + 6/*udp tool*/)

#define IP_REASS_MAX_PBUFS              15

/* Minimal changes to opt.h required for etharp unit tests: */
#define ETHARP_SUPPORT_STATIC_ENTRIES   0

#define LWIP_NUM_NETIF_CLIENT_DATA      0
#define LWIP_SNMP                       0
#define MIB2_STATS                      0
#define LWIP_MDNS_RESPONDER             0

/*
 * Memory from BSS region will be used for pbufs and PCBs when LWIP_MEM_PRE_ALLOC_FROM_HEAP set to 0.
 * Memory will be pre-allocated from the heap for pbufs and PCBs when LWIP_MEM_PRE_ALLOC_FROM_HEAP set to 1
 *  */
#define LWIP_MEM_PRE_ALLOC_FROM_HEAP	1
/** MBEDTLS Specific */
#ifdef NT_FN_MBEDTLS_APP
#define LWIP_COMPAT_SOCKETS             2
#define SO_REUSE                        1
#define LWIP_ALTCP_TLS_MBEDTLS          1
#endif

/** HTTPS Support */
#ifdef NT_FN_HTTPS_FLAG
#define HTTPD_ENABLE_HTTPS              1
#define LWIP_HTTPD_SUPPORT_POST   		1
#define LWIP_HTTPD_SUPPORT_PUT			1
#define LWIP_HTTPD_SUPPORT_DELETE		1
#endif

/* Enable socket send timeout */
#define LWIP_SO_SNDTIMEO 				1

#define LWIP_SO_RCVBUF                  1
#define LWIP_SO_SNDBUF                  1

/* the number of struct netconns. */
#define MEMP_NUM_NETCONN                10
#endif /* LWIP_HDR_LWIPOPTS_H__ */
