/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef LWIP_PING_H
#define LWIP_PING_H

#include "fwconfig_cmn.h"
#include "nt_flags.h" /* Resolve the NT_TST_PING_TOOL & NT_TST_PERF_TOOL. */

//#if NT_TST_PING_TOOL
#if 1
#include "lwip/ip_addr.h"
#include "stdint.h"
#include "lwip/icmp6.h"
#include "lwip/icmp.h"
/**
 * PING_USE_SOCKETS: Set to 1 to use sockets, otherwise the raw api is used
 */
#ifndef PING_USE_SOCKETS
#define PING_USE_SOCKETS 0  // LWIP_SOCKET
#endif
typedef enum { ping_fail, ping_success } ping_status;

#if LWIP_IPV4 && LWIP_IPV6

typedef struct icmp6_echo_hdr icmp6_echo;
typedef struct icmp_echo_hdr icmp4_echo;

typedef struct icmp_echo {
    union {
        icmp6_echo icmp_6;
        icmp4_echo icmp_4;
    } u_icmp;
} icmpm_echo_hdr;

/** @ingroup icmp6
 * Convert generic icmp to specific protocol version
 */
#define icmpm_2_icmp6(ipaddr) (&((ipaddr)->u_icmp.icmp_6))
#define icmpm_2_icmp(ipaddr)  (&((ipaddr)->u_icmp.icmp_4))
#define icmpm_2_icmpg(ipaddr) \
    (&((ipaddr)->u_icmp.icmp_4))  // both union hold same variable so we can access any it will give same result
                                  // instead we can use (&((ipaddr)->u_icmp.icmp_6) both will have same effect.

#else /* LWIP_IPV4 && LWIP_IPV6 */

#if LWIP_IPV4

typedef struct icmp_echo_hdr icmpm_echo_hdr;

#define icmpm_2_icmp(ipaddr)  (ipaddr)
#define icmpm_2_icmp6(ipaddr) (ipaddr)
#define icmpm_2_icmpg(ipaddr) (ipaddr)
#else /* LWIP_IPV4 */

typedef struct icmp6_echo_hdr icmpm_echo_hdr;

#define icmpm_2_icmp(ipaddr)  (ipaddr)
#define icmpm_2_icmp6(ipaddr) (ipaddr)
#define icmpm_2_icmpg(ipaddr) (ipaddr)
#endif /* LWIP_IPV4 */
#endif /* LWIP_IPV4 && LWIP_IPV6 */

void ping_init();

#if !PING_USE_SOCKETS
void ping(ip_addr_t *ip_addr, uint32_t no_of_bytes, uint32_t no_of_pkts, uint32_t delay);
#endif /* !PING_USE_SOCKETS */

#endif /* NT_TST_PING_TOOL */
#endif /* LWIP_PING_H */
