/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*-------------------------------------------------------------------------
 * Include Files
 *-----------------------------------------------------------------------*/
#include "icmp.h"
#include "sockets.h"
#include "queue.h"
#include "autoconf.h"

#ifndef QAT_MAX_MTU_PACKET_SIZE
#define QAT_MAX_MTU_PACKET_SIZE         1462
#endif	

#ifndef QAT_CIRCULAR_BUFFER_SIZE
#define QAT_CIRCULAR_BUFFER_SIZE        (QAT_MAX_MTU_PACKET_SIZE * CONFIG_QAT_CB_SIZE_MTU_MULTIPLIER)
#endif			    

typedef struct icmp_echo_hdr icmp_echo_hdr;

typedef enum {
	PROTOCOL_INVALID,
	PROTOCOL_TCP,
	PROTOCOL_UDP,
	PROTOCOL_SSL
} Protocol;

typedef enum {
	INACTIVE,
	ACTIVE
} qat_connect_status;

typedef enum{
	DHCP_TURN_ON = 0,
	DHCP_TURN_OFF= 1
} dhcp_action;

typedef enum {
    RECVTYPE_ACTIVE,
    RECVTYPE_PASSIVE
} receiveType_mode;

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

typedef struct {
	int sockfd;
	struct sockaddr_in client_addr;
	bool active;
	int recv_type;
} server_ctx_t;


