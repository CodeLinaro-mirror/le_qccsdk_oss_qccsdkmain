/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*-------------------------------------------------------------------------
 * Include Files
 *-----------------------------------------------------------------------*/
#include "icmp.h"
#include "icmp6.h"
#include "sockets.h"
#include "queue.h"
#include "autoconf.h"

#ifndef QAT_MAX_MTU_PACKET_SIZE
#define QAT_MAX_MTU_PACKET_SIZE 1370
#endif

#ifndef QAT_CIRCULAR_BUFFER_SIZE
#define QAT_CIRCULAR_BUFFER_SIZE (QAT_MAX_MTU_PACKET_SIZE * CONFIG_QAT_CB_SIZE_MTU_MULTIPLIER)
#endif

#if LWIP_IPV4 && LWIP_IPV6
typedef struct icmp6_echo_hdr icmp6_echo_hdr;
typedef struct icmp_echo_hdr icmp4_echo_hdr;

typedef struct icmp_echo {
    union {
        icmp6_echo_hdr icmp_6;
        icmp4_echo_hdr icmp_4;
    } u_icmp;
} icmpm_echo_hdr;

/** @ingroup icmp6
 * Convert generic icmp to specific protocol version
 */
#define icmpm_2_icmp(ipaddr)  (&((ipaddr)->u_icmp.icmp_4))
#define icmpm_2_icmp6(ipaddr) (&((ipaddr)->u_icmp.icmp_6))

#else /* LWIP_IPV4 && LWIP_IPV6 */

#if LWIP_IPV4
typedef struct icmp_echo_hdr icmpm_echo_hdr;
#define icmpm_2_icmp(ipaddr)  (ipaddr)
#define icmpm_2_icmp6(ipaddr) (ipaddr)
#else /* LWIP_IPV4 */

typedef struct icmp6_echo_hdr icmpm_echo_hdr;
#define icmpm_2_icmp(ipaddr)  (ipaddr)
#define icmpm_2_icmp6(ipaddr) (ipaddr)
#endif /* LWIP_IPV4 */
#endif /* LWIP_IPV4 && LWIP_IPV6 */

typedef enum {
    PROTOCOL_INVALID,
    PROTOCOL_TCP,
    PROTOCOL_TCPv6,
    PROTOCOL_UDP,
    PROTOCOL_UDPv6,
    PROTOCOL_SSL,
    PROTOCOL_SSLv6
} Protocol;

typedef enum { INACTIVE, ACTIVE } qat_connect_status;

typedef enum { DHCP_TURN_ON = 0, DHCP_TURN_OFF = 1 } dhcp_action;

typedef enum { RECVTYPE_ACTIVE, RECVTYPE_PASSIVE } receiveType_mode;

typedef struct {
    char buffer[QAT_CIRCULAR_BUFFER_SIZE];
    size_t head;
    size_t tail;
    size_t size;
    qurt_mutex_t mutex;
} CircularBuffer;

typedef struct {
    int id;
    int len;
} QueueElem;

typedef struct {
    int id;
    int sockfd;
    int protocol_type;
    int active;
    int recv_type;
    bool thread_quit;
    struct sockaddr_in addr;
    struct sockaddr_in6 addr6;
    CircularBuffer *cb;
} client_ctx_t;

union server_params {
    int port;
    int closeServer;
};

typedef struct {
    int mode;
    union server_params params;
} server_config;

typedef union {
    struct sockaddr_in v4_addr;
    struct sockaddr_in6 v6_addr;
} sock_addr;

typedef struct {
    int sockfd;
    bool active;
    int recv_type;
} server_ctx_t;

typedef struct {
    int sockfd;
    sock_addr addr;
    bool active;
    bool v6;
    int recv_type;
} udp_server_ctx_t;
