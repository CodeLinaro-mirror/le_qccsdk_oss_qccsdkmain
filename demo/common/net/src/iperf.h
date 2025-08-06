/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _IPERF_H_

#define _IPERF_H_

#include <stdbool.h>
#include "qcli_api.h"
#include "qapi_console.h"

#undef A_OK
#define A_OK QAPI_OK

#undef A_ERROR
#define A_ERROR -1

#define IPERF_SERVER                0
#define IPERF_CLIENT                1
#define IPERF_DEFAULT_PORT          5001
#define IPERF3_DEFAULT_PORT         5201
#define IPERF_DEFAULT_RUNTIME       10
#define IPERF_MAX_PACKET_SIZE_TCP   1452 /* Max performance without splitting packets */
#define IPERF_MAX_PACKET_SIZE_UDP   1462 /* Max UDP */
#define IPERF_MAX_PACKET_SIZE_TCPV6 1424 /* Max performance without splitting packets */
#define IPERF_MAX_PACKET_SIZE_UDPV6 1452 /* Max UDP */

/* Note for this macro:
 * Some implementations represent 1KB as 1024 bytes,
 * while others represent 1KB as 1000 bytes.
 * The latter may produce slightly higher stats,
 * although the raw performance is still the same.
 *
 * Modify this macro based on your needs.
 */
#define BYTES_PER_KILO_BYTE 1024

#define IPERF_DEFAULT_UDP_RATE 1  //(BYTES_PER_KILO_BYTE * BYTES_PER_KILO_BYTE); // Default UDP Rate, 1 Mbit/sec
#define IPERF_DEFAULT_TCP_RATE 0  // Default tcp rate, 0 means not fixed.

/* iperf3 control states */
#define TEST_START       1
#define TEST_RUNNING     2
#define TEST_END         4
#define PARAM_EXCHANGE   9
#define CREATE_STREAMS   10
#define SERVER_TERMINATE 11
#define CLIENT_TERMINATE 12
#define EXCHANGE_RESULTS 13
#define DISPLAY_RESULTS  14
#define IPERF_START      15
#define IPERF_DONE       16
#define ACCESS_DENIED    -1
#define SERVER_ERROR     -2

#define COOKIE_SIZE        37
#define COOKIE_CLIENT_NAME "QCC730 client"

#define BENCH_TEST_COMPLETED   "**** Throughput Test Completed ****\r\n"
#define CFG_PACKET_SIZE_MAX_TX (1576)
//#define CFG_PACKET_SIZE_MAX_RX  (1556)
#define CFG_PACKET_SIZE_MAX_RX (2422)
#define DUMP_DIRECTION_TX      (0)
#define DUMP_DIRECTION_RX      (1)

#define OFFSETOF(type, field) ((size_t)(&((type *)0)->field))

typedef struct timeval_s {
    long tv_sec;  /* seconds */
    long tv_usec; /* and microseconds */
} timeval_t;

typedef struct stat_packet {
    uint32_t bytes;
    uint32_t kbytes;
    uint32_t msec;
    uint32_t numPackets;
} stat_packet_t;

typedef struct ip_params {
    uint32_t ipv4Addr;
    uint32_t local_ipv4Addr;
    uint8_t ipv6Addr[16];
    uint8_t local_ipv6Addr[16];
    uint8_t ip_tos;
    uint32_t source_ipv4_addr; /* To fill the 'source address' field in IPv4 header (in net order)
                                * This is for IP_RAW_TX_HDR.
                                */
    int32_t scope_id;
} IP_PARAMS;

typedef struct multicast_params {
    uint32_t ipv4Addr;
    uint32_t rcvIf;
    uint8_t ipv6Addr[16];
    uint8_t enabled;
} MULTICAST_PARAMS;

/**************************************************************************/ /*!
                                                                              * TX/RX Test parameters
                                                                              ******************************************************************************/
typedef struct transmit_params {
    uint32_t ip_address; /* peer's IPv4 address */
    uint8_t v6addr[16];  /* peer's IPv6 address */
    int32_t scope_id;
    uint32_t source_ipv4_addr; /* To fill the 'source address' field in IPv4 header (in net order)
                                * This is for IP_RAW_TX_HDR.
                                */
    uint32_t packet_size;
    uint32_t tx_time;
    uint32_t packet_number;
    uint32_t interval_us;
    uint16_t port;         /* port for UDP/TCP, protocol for IP_RAW */
    uint8_t zerocopy_send; /* 1 = this is zero-copy TX */
    uint8_t test_mode;     /* TIME_TEST or PACKET_TEST */
    uint8_t v6;            /* 1 = this is to TX IPv6 packets */
    uint8_t ip_tos;        /* TOS value in IPv4 header */
    uint8_t is_select;     /* if select before TX send packet */
    uint8_t is_so_unblock; /* if set unblock on TX socket */
} TX_PARAMS;

typedef struct receive_params {
    uint16_t port;
    uint16_t local_if;
    uint32_t local_address;
    uint8_t local_v6addr[16];
    uint32_t mcIpaddr;
    uint32_t mcRcvIf;
    uint8_t mcIpv6addr[16];
    int32_t scope_id;
    uint8_t mcEnabled;
    uint8_t v6;
    float flow_wht;
    uint32_t flow_high;
    uint32_t flow_low;
    uint8_t flow_wht_flag;
    uint8_t flow_high_flag;
    uint8_t flow_low_flag;
} RX_PARAMS;

typedef struct stats {
    uint32_t first_time; /* Test start time */
    uint32_t prev_time;
    uint32_t last_time;
    uint64_t bytes;       /* Number of bytes in one second */
    uint64_t total_bytes; /* total bytes in current test */
    uint64_t kbytes;      /* Number of kilo bytes received in current test */
    uint64_t last_bytes;  /* Number of bytes received in the previous test */
    uint64_t last_kbytes;
    uint64_t sent_bytes;
    uint32_t pkts_recvd;
    uint32_t last_interval;
    uint32_t last_throughput;
    /* iperf stats */
    uint32_t iperf_display_interval;
    uint32_t iperf_time_sec;
    uint32_t iperf_stream_id;
    uint32_t iperf_udp_rate;
    uint32_t iperf_tcp_rate;
} STATS;

typedef struct throughput_cxt {
    uint32_t protocol; /* 1:TCP 2:UDP 4:SSL*/
    uint32_t zc;       /* zero-copy */
    uint16_t port;
    int32_t sock_local;   /* Listening socket.*/
    int32_t sock_peer;    /* Foreign socket.*/
    int32_t sock_control; /* Control socket.*/
    int32_t rxcode;       /* event code from rx_upcall */
    char *buffer;
    // uint8_t *buffer;
    STATS pktStats;
    union params_u {
        TX_PARAMS tx_params;
        RX_PARAMS rx_params;
    } params;
    uint8_t test_type;
    uint32_t iperf_stream_id;
    uint8_t is_iperf : 1;
    uint8_t print_buf : 1;
    uint8_t echo : 1;
    uint8_t bandwidth_unit : 1; /* 0:Mbps 1:Kbps*/
    void *session;
    TaskHandle_t rx_task_handler;
    uint16_t tcp_snd_buf;
    bool result_create;
    bool quit;
} THROUGHPUT_CXT;

typedef struct {
    int32_t sockfd; /* Listening Socket */
    uint16_t port;
    IP_PARAMS ip_params;
    int busySlot;
    int exit;
} bench_tcp_server_t;

typedef struct {
    THROUGHPUT_CXT *ctxt;
    uint16_t port;
    int32_t rxcode; /* event code from rx_upcall */
    int busySlot;
    int exit;
    int ready;
    uint32_t netbuf_id;
    uint32_t buffer_offset;
    uint32_t cur_packet_number;
    int send_flag;
    int isFirst;
    int sock_peer;
    STATS pktStats;
    char *buffer;
    uint32_t iperf_display_last;
    uint32_t iperf_display_next;
} bench_tcp_session_t;

enum test_type {
    TX,
    RX,
};

enum protocol {
    UDP,  // UDP Transmit (Uplink Test)
    TCP,  // TCP Receive (Downlink Test)
};

enum Test_Mode { TIME_TEST, PACKET_TEST };

typedef struct udp_pattern_of_test {
    unsigned int code;
    unsigned short seq;
} UDP_PATTERN_PACKET;
#define CODE_UDP                    ('U' | ('D' << 8) | 'P' << 16)
#define IEEE80211_SN_LESS(sn1, sn2) ((((sn1) - (sn2)) & IEEE80211_SN_MASK) > (IEEE80211_SN_MODULO >> 1))

typedef struct stat_udp_pattern {
    uint32_t pkts_plan;
    uint32_t pkts_recvd;
    uint32_t pkts_seq_recvd;
    uint32_t pkts_seq_less;
    unsigned short seq_last;
    unsigned short ratio_of_drop;
    unsigned short ratio_of_seq_less;
    char stat_valid;
} stat_udp_pattern_t;

// use int32_t if possible, otherwise a 32 bit bitfield (e.g. on J90)
typedef struct udp_datagram {
#ifdef HAVE_INT32_T
    int32_t id;
    u_int32_t tv_sec;
    u_int32_t tv_usec;
#else
    signed int id : 32;
    unsigned int tv_sec : 32;
    unsigned int tv_usec : 32;
#endif
} udp_datagram;

/*
 * The client_hdr structure is sent from clients
 * to servers to alert them of things that need
 * to happen. Order must be perserved in all
 * future releases for backward compatibility.
 * 1.7 has flags, numThreads, mPort, and bufferlen
 */
typedef struct client_hdr {
    int32_t flags;
    int32_t numThreads;
    int32_t mPort;
    int32_t bufferlen;
    int32_t mWindowSize;
    int32_t mAmount;
    int32_t mRate;
    int32_t mUDPRateUnits;
    int32_t mRealtime;
} client_hdr;

/*
 * The server_hdr structure facilitates the server
 * report of jitter and loss on the client side.
 * It piggy_backs on the existing clear to close
 * packet.
 */
typedef struct server_hdr {
    int32_t flags;
    int32_t total_len1;
    int32_t total_len2;
    int32_t stop_sec;
    int32_t stop_usec;
    int32_t error_cnt;
    int32_t outorder_cnt;
    int32_t datagrams;
    int32_t jitter1;
    int32_t jitter2;
    int32_t minTransit1;
    int32_t minTransit2;
    int32_t maxTransit1;
    int32_t maxTransit2;
    int32_t sumTransit1;
    int32_t sumTransit2;
    int32_t meanTransit1;
    int32_t meanTransit2;
    int32_t m2Transit1;
    int32_t m2Transit2;
    int32_t vdTransit1;
    int32_t vdTransit2;
    int32_t cntTransit;
    int32_t IPGcnt;
    int32_t IPGsum;
} server_hdr;

qapi_Status_t iperf(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List);
qapi_Status_t iperf_quit(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List);
#endif /* _IPERF_H_ */
