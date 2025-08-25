/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _SIGMA_H_
#define _SIGMA_H_

#include "qcli_api.h"
#include "lwip/sockets.h"

/*-------------------------------------------------------------------------
 * Preprocessor Definitions, Constants, and Type Declarations
 *-----------------------------------------------------------------------*/

#define WiFiCERT_SHELL_GROUP_NAME "wificert"

#define SIGMA_TEST_RESULT_DEBUG
#undef A_OK
#define A_OK QAPI_OK

#undef A_ERROR
#define A_ERROR -1

#define SIGMA_TRAFFIC_SERVER              0
#define SIGMA_TRAFFIC_CLIENT              1
#define SIGMA_TRAFFIC_DEFAULT_PORT        6000
#define SIGMA_TRAFFIC_DEFAULT_PACKET_SIZE 1260

#define SIGMA_TRAFFIC_DEFAULT_RUNTIME 10

#define SEND_ACK_DEBUG
#undef HOST_TO_LE_LONG
#define HOST_TO_LE_LONG(n) (n)

#define SIGMA_TEST_COMPLETED "**** IOT Throughput Test Completed ****\r\n"

#define CFG_PACKET_SIZE_MAX_RX (1556)

#define END_OF_TEST_CODE (0xAABBCCDD)

#define SIGMA_PRINTF(...) printf(__VA_ARGS__)

//#define SIGMA_DEBUG_PRINT_ENABLE
#ifdef SIGMA_DEBUG_PRINT_ENABLE
#define SIGMA_DEBUG_PRINTF(...) \
    do {                        \
        printf(__VA_ARGS__);    \
    } while (0);

#else
#define SIGMA_DEBUG_PRINTF(...)
#endif

/*-------------------------------------------------------------------------
 * Type Declarations
 *-----------------------------------------------------------------------*/

/*structure to manage stats received from peer during UDP traffic test*/
typedef struct stat_packet {
    uint32_t bytes;
    uint32_t kbytes;
    uint32_t msec;
    uint32_t numPackets;
} stat_packet_t;

/**************************************************************************/ /*!
                                                                              * TX/RX Test parameters
                                                                              ******************************************************************************/
typedef struct sigma_transmit_params {
    uint32_t ip_address;       /* peer's IPv4 address */
    uint32_t source_ipv4_addr; /* To fill the 'source address' field in IPv4 header (in net order)
                                * This is for IP_RAW_TX_HDR.
                                */
    int32_t packet_size;
    uint32_t tx_time;
    uint32_t packet_number;
    uint32_t interval_ms;
    uint16_t port;         /* port for UDP/TCP, protocol for IP_RAW */
    uint8_t test_mode;     /* TIME_TEST or PACKET_TEST */
    uint8_t ip_tos;        /* TOS value in IPv4 header */
    uint8_t is_so_unblock; /* if set unblock on TX socket */
} SIGMA_TX_PARAMS;

typedef struct sigma_receive_params {
    uint16_t port;
    uint16_t local_if;
    uint32_t local_address;
    uint32_t mcIpaddr;
    uint32_t mcRcvIf;
    uint8_t mcEnabled;
} SIGMA_RX_PARAMS;

typedef struct sigma_stats {
    uint32_t first_time; /* Test start time */
    uint32_t last_time;
    unsigned long long bytes;      /* Number of bytes received in current test */
    unsigned long long kbytes;     /* Number of kilo bytes received in current test */
    unsigned long long last_bytes; /* Number of bytes received in the previous test */
    unsigned long long last_kbytes;
    unsigned long long sent_bytes;
    uint32_t pkts_recvd;
    uint32_t last_interval;
    uint32_t last_throughput;
} SIGMA_STATS;

/************************************************************************
 *    Benchmark server control structure.
 *************************************************************************/
typedef struct sigma_cxt {
    uint32_t protocol; /* 1:TCP 2:UDP 4:SSL*/
    uint16_t port;
    int32_t sock_local;   /* Listening socket.*/
    int32_t sock_peer;    /* Foreign socket.*/
    int32_t sock_control; /* Control socket.*/
    char *buffer;
    SIGMA_STATS pktStats;
    union sigma_params_u {
        SIGMA_TX_PARAMS tx_params;
        SIGMA_RX_PARAMS rx_params;
    } params;
    uint8_t test_type;
    uint8_t echo : 1;
} SIGMA_CXT;

typedef struct end_of_test {
    int code;
    int packet_count;
} EOT_PACKET;

enum sigma_test_type {
    SIGMA_TX,
    SIGMA_RX,
};

enum net_protocol {
    PROT_UDP,
    PROT_TCP,
};

enum Sigma_Test_Mode { SIGMA_TIME_TEST, SIGMA_PACKET_TEST };

/*-------------------------------------------------------------------------
 * Function Declarations
 *-----------------------------------------------------------------------*/

void send_ack(SIGMA_CXT *p_tCxt, struct sockaddr *faddr, int addrlen);
void sigma_clear_stats(SIGMA_CXT *p_tCxt);

void sigma_udp_rx(void *sigma_context);
void sigma_udp_tx(void *sigma_context);
void sigma_print_test_results(SIGMA_CXT *p_tCxt, SIGMA_STATS *pktStats);
uint32_t sigma_check_test_time(SIGMA_CXT *p_tCxt);
int sigma_wait_for_response(SIGMA_CXT *p_tCxt, struct sockaddr *to, uint32_t tolen, uint32_t cur_packet_number);
uint32_t sigma_udp_IsPortInUse(uint16_t port);
char *sigma_GetInterfaceNameFromStr(char *ipstr);
uint32_t sigma_SetParams(SIGMA_CXT *p_rxtCxt, const char *protocol, uint16_t port, enum sigma_test_type type);

#endif /* _SIGMA_H_ */
