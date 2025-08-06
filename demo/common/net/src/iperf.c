/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#include "net_shell.h"
#include "qapi_status.h"

#include "qurt_internal.h"
#include "qurt_mutex.h"
#include "iperf.h"
#include <stdbool.h>
#include "lwip/sockets.h"
#include "lwip/def.h"
#include "lwip/ip6_addr.h"
#include "timer.h"
#include "safeAPI.h"
#include "data_path.h"

#ifdef CONFIG_NET_IPERF

#define TO_CHECK 0

#define IPERF_RX_THREAD 1
uint8_t iperf_tx_quit = 0;
uint8_t iperf_rx_quit = 0;
#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b))

extern QAPI_Console_Group_Handle_t net_shell_cmd_group_handle; /* Handle for our QCLI Command Group. */

#define IPERF_PRINTF(...) QCLI_Printf(net_shell_cmd_group_handle, __VA_ARGS__)

// #define UDP_TX_DEBUG

#define IPV6_TCLASS                     16 /* int; set IPV6 traffic class */
#define IS_IPV6_MULTICAST(ipv6_Address) (((uint8_t *)ipv6_Address)[0] == 0xff)

/* loopback behavior (disabled or enabled) for multicast packets */
#define IPV6_MC_LPBK_DIS         0 /**< Disable loopback behavior for multicast packets. */
#define IPV6_MC_LPBK_EN          1 /**< Enable loopback behavior for multicast packets. */
#define IPERF_TX_THREAD_PRIO     5
#define IPERF_RX_THREAD_PRIO     6
#define IPERF_RESULT_THREAD_PRIO 3

#define MAX_IPERF_RX_THREAD_COUNTE 4
UBaseType_t iperf_rx_thread_priority = IPERF_RX_THREAD_PRIO;
TaskHandle_t iperf_rx_thread_handle[MAX_IPERF_RX_THREAD_COUNTE] = {0};
uint8_t iperf_rx_thread_handle_sum = 0;

#if TO_CHECK
void iperf3_make_cookie(char *str, int len);
void iperf3_tx_reverse(THROUGHPUT_CXT *p_tCxt);
#endif

void iperf_udp_tx(THROUGHPUT_CXT *p_tCxt);
#if IPERF_RX_THREAD
void iperf_udp_rx(void *arg);
#else
void iperf_udp_rx(THROUGHPUT_CXT *p_tCxt);
#endif
void iperf_tcp_tx(THROUGHPUT_CXT *p_tCxt);
#if IPERF_RX_THREAD
void iperf_tcp_rx(void *arg);
#else
void iperf_tcp_rx(THROUGHPUT_CXT *p_tCxt);
#endif
void iperf_result_print(STATS *pCxtPara, uint32_t prev, uint32_t cur, bool is_iperf_done);
void iperf_rx_show_result(void *arg);

extern qbool_t get_device_connect_state(void);
extern void qurt_thread_sleep(uint32_t duration);
#define HEADER_VERSION1 0x80000000

#define MAX_STREAM 10
uint8_t iperf_stream_id[MAX_STREAM] = {0};
uint16_t bench_udp_rx_port_in_use[MAX_STREAM] = {0}; /* Used to prevent two udp rx streams from using the same port */

#define BENCH_TCP_RX_MAX_SESSIONS 2 /* Max number of TCP RX sessions */
#define BENCH_TCP_MAX_SERVERS     2 /* Max number of TCP servers that can execute in parallel */
bench_tcp_session_t g_tcpSessions[BENCH_TCP_RX_MAX_SESSIONS]; /* Array of TCP Session objects */
#define BENCH_TCP_PKTS_PER_DOT 1000                           /* Produce a progress dot each X packets */

qurt_mutex_t sessionLock;                               /* Lock to protect the global session object array */
qurt_mutex_t serverLock;                                /* Lock to protect the global TCP Server object array */
int sessionRefCount = 0;                                /* Total number of active TCP/SSL RX sessions */
int tcpRefCount = 0;                                    /* Number of active TCP RX sessions */
bench_tcp_server_t g_tcpServers[BENCH_TCP_MAX_SERVERS]; /* Array of TCP Server objects */
int serverRefCount = 0;

/***************************************************************************************
 *
 * iperf function
 *
 **************************************************************************************/
#define TCP_QUEUE_PBUF_THRESHOLD_DEFAULT 21
#define TCP_QUEUE_PBUF_THRESHOLD_LOW     12
#define TCP_QUEUE_PBUF_THRESHOLD_STEP    3

#define Mbps (1000 * 1000)
#define Kbps 1000

extern uint8_t tcp_queue_pbuf_threshold;
void iperf_incrs_tcp_queue_pbuf_thrsh(void)
{
    if (tcp_queue_pbuf_threshold < TCP_QUEUE_PBUF_THRESHOLD_DEFAULT) {
        tcp_queue_pbuf_threshold += TCP_QUEUE_PBUF_THRESHOLD_STEP;
    }

    return;
}

void iperf_decrs_tcp_queue_pbuf_thrsh(void)
{
    if (tcp_queue_pbuf_threshold > TCP_QUEUE_PBUF_THRESHOLD_LOW) {
        tcp_queue_pbuf_threshold -= TCP_QUEUE_PBUF_THRESHOLD_STEP;
    }

    return;
}

static void app_get_time(uint32_t *time_ms)
{
    *time_ms = hres_timer_curr_time_ms();
}

static uint8_t iperf_get_unused_id()
{
    uint8_t i;
    for (i = 0; i < MAX_STREAM; i++) {
        if (iperf_stream_id[i] == 0) {
            iperf_stream_id[i] = 1;
            break;
        }
    }
    return i;
}

static uint8_t iperf_stream_full(void)
{
    uint8_t i;
    for (i = 0; i < MAX_STREAM; i++) {
        if (iperf_stream_id[i] == 0) {
            break;
        }
    }

    if (i >= MAX_STREAM) {
        IPERF_PRINTF("ERROR: iperf_stream_full: Reach the limited streams\r\n");
        return 1;
    }

    return 0;
}

uint8_t iperf_stream_count(void)
{
    uint8_t count = 0;
    for (int i = 0; i < MAX_STREAM; i++) {
        if (iperf_stream_id[i] != 0)
            count++;
    }
    IPERF_PRINTF("INFO: iperf_stream_count:%d\r\n", count);
    return count;
}

static void iperf_common_clear_stats(THROUGHPUT_CXT *p_tCxt)
{
    p_tCxt->pktStats.bytes = 0;
    p_tCxt->pktStats.total_bytes = 0;
    p_tCxt->pktStats.kbytes = 0;
    p_tCxt->pktStats.sent_bytes = 0;
    p_tCxt->pktStats.pkts_recvd = 0;
}

static int iperf_get_unused_rx_thread_index(void)
{
    uint8_t thread_index;
    for (thread_index = 0; thread_index < MAX_IPERF_RX_THREAD_COUNTE; thread_index++) {
        if (iperf_rx_thread_handle[thread_index] == NULL) {
            return thread_index;
        }
    }
    if (thread_index >= MAX_IPERF_RX_THREAD_COUNTE) {
        return -1;
    }

    return -1;
}

static void iperf_upgrade_rx_thread_priority(void)
{
    uint8_t thread_index;
    for (thread_index = 0; thread_index < MAX_IPERF_RX_THREAD_COUNTE; thread_index++) {
        if (iperf_rx_thread_handle[thread_index] != NULL) {
            nt_qurt_thread_set_priority(
                iperf_rx_thread_handle[thread_index],
                iperf_rx_thread_priority + 1); /*increase udp rx thread priority in multi-thread context*/
        }
    }

    return;
}

static void iperf_resume_rx_thread_priority(void)
{
    uint8_t thread_index;
    for (thread_index = 0; thread_index < MAX_IPERF_RX_THREAD_COUNTE; thread_index++) {
        if (iperf_rx_thread_handle[thread_index] != NULL) {
            nt_qurt_thread_set_priority(
                iperf_rx_thread_handle[thread_index],
                iperf_rx_thread_priority); /*increase udp rx thread priority in multi-thread context*/
        }
    }

    return;
}

static int iperf_remove_rx_thread_based_on_id(TaskHandle_t rx_thread_id)
{
    uint8_t thread_index;

    for (thread_index = 0; thread_index < MAX_IPERF_RX_THREAD_COUNTE; thread_index++) {
        if (iperf_rx_thread_handle[thread_index] == rx_thread_id) {
            iperf_rx_thread_handle[thread_index] = NULL;
            iperf_rx_thread_handle_sum--;
            break;
        }
    }
    if (thread_index >= MAX_IPERF_RX_THREAD_COUNTE) {
        IPERF_PRINTF("remove_rx_thread error!\r\n", thread_index, rx_thread_id);
        return -1;
    }

    if (iperf_rx_thread_handle_sum == 1) {
        iperf_resume_rx_thread_priority();
    }

    return 0;
}

/************************************************************************
 * NAME: iperf_common_check_test_time
 *
 * DESCRIPTION: If test mode is time, check if current time has exceeded
 * test time limit
 * Parameters: pointer to throughput context
 ************************************************************************/
static uint32_t iperf_common_check_test_time(THROUGHPUT_CXT *p_tCxt)
{
    uint32_t duration; /* in ms */
    uint32_t last_time = p_tCxt->pktStats.last_time;
    uint32_t first_time = p_tCxt->pktStats.first_time;

    if (last_time < first_time) {
        /* Assume the systick wraps around once */
        duration = ~first_time + 1 + last_time;
    } else {
        duration = last_time - first_time;
    }

    if (duration >= p_tCxt->params.tx_params.tx_time * 1000)
        return 1;
    else
        return 0;
}

static void iperf_common_SetProtocol(THROUGHPUT_CXT *p_rxtCxt, const char *protocol)
{
    if (strcasecmp("udp", protocol) == 0) {
        p_rxtCxt->protocol = UDP;
    } else if (strcasecmp("tcp", protocol) == 0) {
        p_rxtCxt->protocol = TCP;
    } else {
        p_rxtCxt->protocol = ~0; /* Invalid protocol */
    }
}

static uint32_t iperf_common_IsPortInUse(THROUGHPUT_CXT *p_rxtCxt, uint16_t port)
{
    int i;

    if (p_rxtCxt->protocol == UDP) {
        int j = -1;

        for (i = 0; i < MAX_STREAM; i++) {
            if (port == bench_udp_rx_port_in_use[i]) {
                break;
            }

            if (0 == bench_udp_rx_port_in_use[i]) {
                j = i;
            }
        }

        if ((i < MAX_STREAM) || (j == -1)) {
            return 1;
        } else {
            bench_udp_rx_port_in_use[j] = port;
        }
    } else if (p_rxtCxt->protocol == TCP) {
        bench_tcp_session_t *session;
        bench_tcp_server_t *server;

        for (i = 0; i < BENCH_TCP_MAX_SERVERS; i++) {
            server = &g_tcpServers[i];
            if (server->busySlot && (server->port == port)) {
                return 1;
            }
        }

        for (i = 0; i < BENCH_TCP_RX_MAX_SESSIONS; i++) {
            session = &g_tcpSessions[i];
            if (session->busySlot && (session->port == port)) {
                return 1;
            }
        }
    }

    return 0;
}

static uint32_t iperf_common_SetParams(THROUGHPUT_CXT *p_rxtCxt, uint32_t v6, const char *protocol, uint16_t port,
                                       enum test_type type)
{
    iperf_common_SetProtocol(p_rxtCxt, protocol);

    if (type == RX) {
        if (iperf_common_IsPortInUse(p_rxtCxt, port)) {
            IPERF_PRINTF("port %d is in use; use another port.\n", port);
            return QAPI_NET_ERR_OPERATION_FAILED;
        }
    }

    switch (type) {
        case RX:
            p_rxtCxt->params.rx_params.v6 = v6;
            p_rxtCxt->params.rx_params.port = port;
            break;
        case TX:
            p_rxtCxt->params.tx_params.v6 = v6;
            p_rxtCxt->params.tx_params.port = port;
            break;
    }
    p_rxtCxt->test_type = type;
    return 0;
}

/************************************************************************
 * The pattern:
 * < ---          len                       -->
 * 00 01 02 03 .. FE FF 00 01 02 .. FF 00 01 ..
 * A
 * |
 * p
 ************************************************************************/
#define INCREMENTAL_PATTERN_SIZE 256
char g_incremental_pattern[INCREMENTAL_PATTERN_SIZE];

void iperf_common_add_pattern(char *p, int len)
{
    int n, ml;

    while (len) {
        ml = min(INCREMENTAL_PATTERN_SIZE, len);
        for (n = 0; n < ml; ++n, ++p) {
            *p = (char)n;
        }
        len -= ml;
    }
}

/**************************************************************
 * FUNCTION: iperf_tcp_CreateSession(THROUGHPUT_CXT *)
 * @brief: Create TCP Session object.
 * @return: sessionId if success, -1 on failure.
 *************************************************************/
static int iperf_tcp_CreateSession(THROUGHPUT_CXT *p_tCxt)
{
    bench_tcp_session_t *session;
    int sessionId = -1;
    int index = 0;

    if (!p_tCxt) {
        IPERF_PRINTF("%s:Context is NULL", __func__);
        return -1;
    }
    /*If this is the first session, initialize the lock and clear out the session array.
      For subsequent sessions, sessionRefCount will be > 0, so the mutex will not be re-init*/
    if (!sessionRefCount) {
        qurt_mutex_create(&sessionLock);
        memset(g_tcpSessions, 0, sizeof(g_tcpSessions));
    }

    qurt_mutex_lock(&sessionLock);
    for (index = 0; index < BENCH_TCP_RX_MAX_SESSIONS; index++) {
        if (!g_tcpSessions[index].busySlot) {
            sessionId = index;
            break;
        }
    }

    if (index >= BENCH_TCP_RX_MAX_SESSIONS) {
        qurt_mutex_unlock(&sessionLock);

        if (!sessionRefCount) {
            /*Failure. No sessions are active, destroy the lock*/
            qurt_mutex_delete(&sessionLock);
        }
        return -2;
    }

    g_tcpSessions[sessionId].busySlot = 1;

    session = &g_tcpSessions[sessionId];
    session->ctxt = p_tCxt;
    tcpRefCount++;
    sessionRefCount++;
    qurt_mutex_unlock(&sessionLock);

    return sessionId;
}

/*****************************************************************************
 * FUNCTION: iperf_tcp_CloseSession(bench_tcp_session_t *)
 * @brief: Close an existing TCP RX session and destroy the TCP session object.
 * @return: void
 *****************************************************************************/
static void iperf_tcp_CloseSession(bench_tcp_session_t *session, fd_set *rd_set)
{
    if ((session == NULL) || (rd_set == NULL)) {
        return;
    }

    qurt_mutex_lock(&sessionLock);
    session->ready = 0;
    session->busySlot = 0;

    iperf_result_print(&session->pktStats, 0, 0, true);

    if (session->buffer) {
        free(session->buffer);
        session->buffer = 0;
    }

    if (A_ERROR != session->sock_peer) {
        FD_CLR(session->sock_peer, rd_set);
        closesocket(session->sock_peer);
        IPERF_PRINTF("INFO: %s: IPERF socket: close:%d\r\n", __func__, session->sock_peer);
        session->sock_peer = A_ERROR;
    }

    tcpRefCount = (tcpRefCount) ? (tcpRefCount - 1) : 0;
    sessionRefCount = (sessionRefCount) ? (sessionRefCount - 1) : 0;

    memset(session, 0, sizeof(bench_tcp_session_t));
    if (!sessionRefCount)
        FD_ZERO(rd_set);

    qurt_mutex_unlock(&sessionLock);

    if (!sessionRefCount)
        qurt_mutex_delete(&sessionLock);

    return;
}

#if TO_CHECK
static void iperf_tcp_rx_quit(int serverId)
{
    bench_tcp_server_t *server = NULL;

    if (serverId < 0 || serverId >= BENCH_TCP_MAX_SERVERS) {
        IPERF_PRINTF("Invalid session id %d\n", serverId);
        return;
    }

    qurt_mutex_lock(&serverLock);
    server = &g_tcpServers[serverId];
    if (server->busySlot) {
        server->exit = 1;
    }
    qurt_mutex_unlock(&serverLock);
}
#endif

static int iperf_tcp_getServerId()
{
    int serverId = -1;
    int i = 0;

    qurt_mutex_lock(&serverLock);
    for (i = 0; i < BENCH_TCP_MAX_SERVERS; i++) {
        if (!g_tcpServers[i].busySlot) {
            serverId = i;
            break;
        }
    }

    if (serverId == -1 || serverId >= BENCH_TCP_MAX_SERVERS) {
        qurt_mutex_unlock(&serverLock);
        return -1;
    }

    g_tcpServers[serverId].busySlot = 1;
    serverRefCount++;
    qurt_mutex_unlock(&serverLock);

    return serverId;
}

static void iperf_tcp_stopServer(bench_tcp_server_t *server)
{
    qurt_mutex_lock(&serverLock);

    if (A_ERROR != server->sockfd) {
        closesocket(server->sockfd);
        IPERF_PRINTF("INFO: %s: IPERF socket: close:%d, tcp_server->sockfd.\r\n", __func__, server->sockfd);
        server->sockfd = A_ERROR;
    }
    memset(server, 0, sizeof(bench_tcp_server_t));
    serverRefCount = (serverRefCount) ? (serverRefCount - 1) : 0;

    qurt_mutex_unlock(&serverLock);

    if (!serverRefCount) {
        qurt_mutex_delete(&serverLock);
    }
}

#if TO_CHECK
void iperf_tcp_rx_dump_servers()
{
    int i = 0;

    if (!serverRefCount) {
        IPERF_PRINTF("No TCP servers found\n");
        return;
    }

    qurt_mutex_lock(&serverLock);
    for (i = 0; i < BENCH_TCP_MAX_SERVERS; i++) {
        if (g_tcpServers[i].busySlot) {
            IPERF_PRINTF("****** TCP SERVER ******\n");
            IPERF_PRINTF("ServerId: %d\n", i);
            IPERF_PRINTF("Port: %d\n", g_tcpServers[i].port);
            IPERF_PRINTF("***********************\n");
        }
    }
    qurt_mutex_unlock(&serverLock);
}

/********************************************************************************
 * FUNCTION: iperf_tcp_CloseAllSessions()
 * @brief: Close all existing TCP RX sessions with the given ctxt
 *         and destroy the corresponding TCP session objects.
 * @return: void
 ********************************************************************************/

static void iperf_tcp_CloseAllSessions(THROUGHPUT_CXT *p_tCxt, fd_set *rd_set)
{
    int i = 0;
    bench_tcp_session_t *session;

    if (sessionRefCount > 0) {
        for (i = 0; i < BENCH_TCP_RX_MAX_SESSIONS; i++) {
            session = &g_tcpSessions[i];

            if (session->busySlot && (session->ctxt == p_tCxt)) {
                IPERF_PRINTF("Closing SessionId:%d\n", i);
                iperf_tcp_CloseSession(session, rd_set);
            }
        }
    }
}
#endif

#if 0
static void iperf_print_buffer(const char *buf, uint32_t len, struct sockaddr *sock_addr, uint8_t direction)
{
    (void)buf;
    (void)len;
    (void)sock_addr;
    (void)direction;

    IPERF_PRINTF("to do...");
}
#endif

#if TO_CHECK
static void rxreorder_udp_payload_statistics(stat_udp_pattern_t *stat_udp, char *buffer, uint32_t len)
{
    UDP_PATTERN_PACKET udp_pattern;

    if (len < sizeof(UDP_PATTERN_PACKET)) {
        return;
    }

    memscpy(&udp_pattern, sizeof(UDP_PATTERN_PACKET), buffer, sizeof(UDP_PATTERN_PACKET));
    if (udp_pattern.code != CODE_UDP) {
        return;
    }

    if (!(stat_udp->stat_valid)) {
        stat_udp->stat_valid = 1;
        stat_udp->seq_last = udp_pattern.seq;
    }
    stat_udp->pkts_seq_recvd++;
    if (IEEE80211_SN_LESS(udp_pattern.seq, stat_udp->seq_last)) {
        stat_udp->pkts_seq_less++;
    } else {
        stat_udp->seq_last = udp_pattern.seq;
    }
}
#endif

#define RATIO_BASE (10000)
#define UINT32MAX  (0xffffffff)

static unsigned short ratio(uint32_t numerator, uint32_t denominator, unsigned short base)
{
    unsigned short ret = 0;
    if (base) {
        if (denominator) {
            if (numerator) {
                unsigned short multiplier = min(UINT32MAX / numerator, base);
                ret = ((numerator * multiplier) / denominator) * (base / multiplier);
            } else {
                ret = 0;
            }
        } else {
            IPERF_PRINTF("warning, denominator=%d\n", denominator);
        }
    } else {
        IPERF_PRINTF("warning, base=%d not supported\n", base);
    }
    return ret;
}

static void rxreorder_udp_payload_report(stat_udp_pattern_t *stat_udp)
{
    if (!(stat_udp->stat_valid)) {
        return;
    }

    if (stat_udp->pkts_plan) {
        stat_udp->ratio_of_drop = ratio((stat_udp->pkts_plan - stat_udp->pkts_recvd), stat_udp->pkts_plan, RATIO_BASE);
    }
    stat_udp->ratio_of_seq_less = ratio(stat_udp->pkts_seq_less, stat_udp->pkts_seq_recvd, RATIO_BASE);
    IPERF_PRINTF("udp pkts: plan=%d recvd=%d drop_ratio=%d/%d\n", stat_udp->pkts_plan, stat_udp->pkts_recvd,
                 stat_udp->ratio_of_drop, RATIO_BASE);
    IPERF_PRINTF("udp pkts of seq: recvd=%d less=%d less_ratio=%d/%d\n", stat_udp->pkts_seq_recvd,
                 stat_udp->pkts_seq_less, stat_udp->ratio_of_seq_less, RATIO_BASE);
}

void iperf_print_data(char *buffer, int received)
{
    IPERF_PRINTF("Len %d, data:: ", received);
    for (int i = 0; i < received && i < 64; i++) {
        IPERF_PRINTF("%02x ", buffer[i]);
        if (i + 1 % 16 == 0)
            IPERF_PRINTF("\n");
    }
    IPERF_PRINTF("\n");
}
/**********************************************************************************************/
qapi_Status_t iperf_quit(uint32_t __attribute__((__unused__)) Parameter_Count,
                         QAPI_Console_Parameter_t __attribute__((__unused__)) * Parameter_List)
{
    iperf_tx_quit = 1;
    iperf_rx_quit = 1;

    return QAPI_OK;
}

/**********************************************************************************************/
qapi_Status_t iperf(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    unsigned int protocol = TCP;
    unsigned int port = IPERF_DEFAULT_PORT;
    unsigned int port_tag = 0;
    unsigned int seconds = IPERF_DEFAULT_RUNTIME;
    unsigned int pktSize = 0;
    int operation_mode = -1;
    int reverse_mode = 0;
    unsigned int udpRate = IPERF_DEFAULT_UDP_RATE;
    unsigned int tcpRate = IPERF_DEFAULT_TCP_RATE;
    unsigned int bandwidth_unit = 0;
    char *rateString = NULL;
    unsigned short mcastEnabled = 0;
    int ip_tos = 0;
    unsigned int sndbuf_size = 0;

    unsigned int ipAddress = 0;
    unsigned int numOfPkts = 0;
    unsigned int interval = 0;
    unsigned int index = 0;

    THROUGHPUT_CXT *tCxt = NULL;
    // THROUGHPUT_CXT *rCxt = NULL;
    uint32_t v6 = 0;
    char *receiver_ip;
    uint8_t v6addr[16] = {0};

    // memset(&tCxt, 0, sizeof(THROUGHPUT_CXT));
    // memset(&rCxt, 0, sizeof(THROUGHPUT_CXT));

    index = 0;

    if (Parameter_Count < 1) {
        IPERF_PRINTF("\nUsage: iperf [-s|-c host] [options]\n");
        IPERF_PRINTF(
            "  -p  = The server port for the server to listen on and the client to connect to. This should be "
            "the same in both client and server. Default is 5001.\n");
        IPERF_PRINTF("  -i  = Sets the interval time in seconds between periodic bandwidth\n");
        IPERF_PRINTF("  -u  = Use UDP rather than TCP\n");
        IPERF_PRINTF("  -l = The length of buffers to read or write\n");
        IPERF_PRINTF("  -t = The time in seconds to transmit for\n");
        IPERF_PRINTF("  -n = The number of buffers to transmit\n");
        IPERF_PRINTF("  -b = Set target bandwidth to Mbits/sec\n");
        IPERF_PRINTF("  -S = Set TOS for TCP\n");
        IPERF_PRINTF("  -V = IPV6\n");

        return QAPI_ERR_INVALID_PARAM;
    }

    if (iperf_stream_full()) {
        return QAPI_ERR_INVALID_PARAM;
    }

    while (index < Parameter_Count) {
        if (0 == strcmp(Parameter_List[index].String_Value, "-u")) {
            index++;
            protocol = UDP;
        } else if (0 == strcmp(Parameter_List[index].String_Value, "-s")) {
            index++;
            operation_mode = IPERF_SERVER;
        } else if (0 == strcmp(Parameter_List[index].String_Value, "-c")) {
            index++;
            operation_mode = IPERF_CLIENT;
            receiver_ip = Parameter_List[index].String_Value;
            if (inet_pton(AF_INET, receiver_ip, &ipAddress) == 1) {
                /* is valid IPV4 */
                if ((ipAddress & 0xf0000000) == 0xE0000000)  // 224.xxx.xxx.xxx - 239.xxx.xxx.xxx
                {
                    mcastEnabled = 1;
                }
            } else if (inet_pton(AF_INET6, receiver_ip, &v6addr) == 1) {
                /* is valid IPV6*/
                if (IS_IPV6_MULTICAST(v6addr)) {
                    mcastEnabled = 1;
                }
            } else {
                IPERF_PRINTF("Incorrect IP address %s\n", receiver_ip);
                return QAPI_ERR_INVALID_PARAM;
            }
            index++;
        } else if (0 == strcmp(Parameter_List[index].String_Value, "-p")) {
            index++;
            port = Parameter_List[index].Integer_Value;
            port_tag = 1;
            index++;

            if (port > 64 * 1024) {
                IPERF_PRINTF("error: invalid port\n");
                return QAPI_ERR_INVALID_PARAM;
            }
        } else if (0 == strcmp(Parameter_List[index].String_Value, "-i")) {
            index++;
            interval = Parameter_List[index].Integer_Value;
            index++;

            if (interval <= 0) {
                interval = 1;
            }
        } else if (0 == strcmp(Parameter_List[index].String_Value, "-l")) {
            index++;
            pktSize = Parameter_List[index].Integer_Value;
            index++;
            pktSize = pktSize < 12 ? 12 : pktSize;
        } else if (0 == strcmp(Parameter_List[index].String_Value, "-t")) {
            index++;
            seconds = Parameter_List[index].Integer_Value;
            index++;

            if (seconds <= 0) {
                seconds = IPERF_DEFAULT_RUNTIME;
            }
        } else if (0 == strcmp(Parameter_List[index].String_Value, "-n")) {
            index++;
            numOfPkts = Parameter_List[index].Integer_Value;
            index++;
        } else if (0 == strcmp(Parameter_List[index].String_Value, "-b")) {
            index++;
            rateString = Parameter_List[index].String_Value;
            index++;

            if (rateString[strlen(rateString) - 1] == 'M') {
                rateString[strlen(rateString) - 1] = '\0';
                udpRate = atoi(rateString);
                tcpRate = udpRate;
                bandwidth_unit = 0;  // Mbps
                if (udpRate == 0 || udpRate > 100) {
                    IPERF_PRINTF("error: invalid bandwidth value, unit is Mbps, should less 100\n");
                    return QAPI_ERR_INVALID_PARAM;
                }
            } else if (rateString[strlen(rateString) - 1] == 'K') {
                rateString[strlen(rateString) - 1] = '\0';
                udpRate = atoi(rateString);
                tcpRate = udpRate;
                bandwidth_unit = 1;  // Kbps
                if (udpRate == 0 || udpRate > 100000) {
                    IPERF_PRINTF("error: invalid bandwidth value, unit is Kbps, should less 100000\n");
                    return QAPI_ERR_INVALID_PARAM;
                }
            } else {
                IPERF_PRINTF("error: invalid bandwidth format, valid input example: \"500K\" or \"5M\"\n");
            }

        } else if (0 == strcmp(Parameter_List[index].String_Value, "-V")) {
            index++;
            v6 = 1;
        } else if (0 == strcmp(Parameter_List[index].String_Value, "-S")) {
            index++;
            ip_tos = Parameter_List[index].Integer_Value;
            if (ip_tos > 255 || ip_tos < 0) {
                IPERF_PRINTF("error: invalid TOS value\n");
                return QAPI_ERR_INVALID_PARAM;
            }
        } else if (0 == strcmp(Parameter_List[index].String_Value, "-h")) {
            index++;
            IPERF_PRINTF("Usage: iperf [-s|-c host] [options]\n");
            IPERF_PRINTF("       iperf [-h]\n");
        } else if (0 == strcmp(Parameter_List[index].String_Value, "-w")) {
            index++;
            sndbuf_size = Parameter_List[index].Integer_Value;
            if (sndbuf_size <= 0 || sndbuf_size > 24) {
                IPERF_PRINTF("error: invalid sndbuf size value\n");
                return QAPI_ERR_INVALID_PARAM;
            }
        } else {
            /*silent ignore*/
            index++;
        }
    }
    tCxt = malloc(sizeof(THROUGHPUT_CXT));
    if (tCxt == NULL) {
        IPERF_PRINTF("Memory alloc failed\n");
        return QAPI_ERR_NO_MEMORY;
    }
    memset(tCxt, 0, sizeof(THROUGHPUT_CXT));
    tCxt->pktStats.iperf_display_interval = interval;
    tCxt->pktStats.iperf_udp_rate = udpRate;
    tCxt->pktStats.iperf_tcp_rate = tcpRate;
    tCxt->bandwidth_unit = bandwidth_unit;

    tCxt->tcp_snd_buf = sndbuf_size * TCP_MSS;

    if (operation_mode == IPERF_CLIENT) {
        iperf_tx_quit = 0;
        tCxt->params.tx_params.v6 = v6;
        tCxt->params.tx_params.ip_address = ipAddress;
        memscpy(tCxt->params.tx_params.v6addr, sizeof(tCxt->params.tx_params.v6addr), v6addr, sizeof(v6addr));
        tCxt->params.tx_params.scope_id = nt_get_netifidx_by_devmode(STA_DEVICE);
        tCxt->params.tx_params.ip_tos = ip_tos;
        if (pktSize > 0) {
            if (v6) {
                if (protocol == TCP) {
                    pktSize = min(pktSize, IPERF_MAX_PACKET_SIZE_TCPV6);
                } else {
                    pktSize = min(pktSize, IPERF_MAX_PACKET_SIZE_UDPV6);
                }
            } else {
                if (protocol == TCP) {
                    pktSize = min(pktSize, IPERF_MAX_PACKET_SIZE_TCP);
                } else {
                    pktSize = min(pktSize, IPERF_MAX_PACKET_SIZE_UDP);
                }
            }
        } else {
            if (v6) {
                if (protocol == TCP) {
                    pktSize = IPERF_MAX_PACKET_SIZE_TCPV6;
                } else {
                    pktSize = IPERF_MAX_PACKET_SIZE_UDPV6;
                }
            } else {
                if (protocol == TCP) {
                    pktSize = IPERF_MAX_PACKET_SIZE_TCP;
                } else {
                    pktSize = IPERF_MAX_PACKET_SIZE_UDP;
                }
            }
        }
        tCxt->params.tx_params.packet_size = pktSize;
        tCxt->params.tx_params.test_mode = TIME_TEST;
        tCxt->params.tx_params.tx_time = seconds;
        if (numOfPkts > 0) {
            tCxt->params.tx_params.test_mode = PACKET_TEST;
            tCxt->params.tx_params.packet_number = numOfPkts;
        }

        /* set default iperf3 port if reverse_mode */
        if ((reverse_mode != 0) && (port_tag == 0)) {
            port = IPERF3_DEFAULT_PORT;
        }

        if ((protocol == UDP) && mcastEnabled) {
            IPERF_PRINTF("Multicast transmit not yet implemented\n");
            goto RET_ERR;
        } else if (protocol == UDP) {
            uint32_t ret;
            ret = iperf_common_SetParams(tCxt, tCxt->params.tx_params.v6, "udp", port, TX);
            if (ret == 0) {
                tCxt->protocol = UDP;
                tCxt->test_type = TX;

                iperf_udp_tx(tCxt);
            } else {
                goto RET_ERR;
            }
        } else if (protocol == TCP) {
            uint32_t ret;
            ret = iperf_common_SetParams(tCxt, tCxt->params.tx_params.v6, "tcp", port, TX);
            if (ret == 0) {
                tCxt->protocol = TCP;
                tCxt->test_type = TX;

                iperf_tcp_tx(tCxt);
            } else {
                goto RET_ERR;
            }
        }
    } else if (operation_mode == IPERF_SERVER) {
        iperf_rx_quit = 0;
        tCxt->params.rx_params.v6 = v6;
        if ((protocol == UDP) && mcastEnabled) {
            IPERF_PRINTF("Multicast receive not yet implemented\n");
            goto RET_ERR;
        } else if (protocol == UDP) {
            if (iperf_common_SetParams(tCxt, tCxt->params.rx_params.v6, "udp", port, RX) != 0) {
                IPERF_PRINTF("error: invalid port\n");

                goto RET_ERR;
            }
            memset(&tCxt->pktStats, 0, sizeof(STATS));
            tCxt->protocol = UDP;
            tCxt->test_type = RX;
            tCxt->pktStats.iperf_display_interval = interval;

#if IPERF_RX_THREAD
            err_t ret;
            int unused_thread_index = -1;

            unused_thread_index = iperf_get_unused_rx_thread_index();
            if (unused_thread_index == -1) {
                IPERF_PRINTF("ERROR: %s: UDP server get unused thread index failed!\r\n", __func__);
                if (tCxt) {
                    free(tCxt);
                    tCxt = NULL;
                }
                goto RET_OK;
            }
            ret = nt_qurt_thread_create(iperf_udp_rx, "udp_rx", 3072, tCxt, iperf_rx_thread_priority,
                                        &(iperf_rx_thread_handle[unused_thread_index]));
            if (ret == -1) {
                IPERF_PRINTF("ERROR: %s: UDP server task creation failed\r\n", __func__);
                if (tCxt) {
                    free(tCxt);
                    tCxt = NULL;
                }

                goto RET_OK;
            } else {
                iperf_rx_thread_handle_sum++;
            }
            if (iperf_rx_thread_handle_sum > 1) {
                iperf_upgrade_rx_thread_priority();
            }
#else
            iperf_udp_rx(tCxt);
#endif
            if (tCxt->pktStats.iperf_display_interval) {
                ret = nt_qurt_thread_create(iperf_rx_show_result, "udp_rx_rslt", 1024, tCxt, IPERF_RESULT_THREAD_PRIO,
                                            &(tCxt->rx_task_handler));
                if (ret == -1) {
                    IPERF_PRINTF("UDP result task creation failed\r\n");
                    goto RET_OK;
                }
                tCxt->result_create = true;
            }
        } else if (protocol == TCP) {
            if (iperf_common_SetParams(tCxt, tCxt->params.rx_params.v6, "tcp", port, RX) != 0) {
                IPERF_PRINTF("error: invalid port\n");
                goto RET_ERR;
            }
            tCxt->protocol = TCP;
            tCxt->test_type = RX;
            tCxt->pktStats.iperf_display_interval = interval;
#if IPERF_RX_THREAD
            err_t ret;
            int unused_thread_index = -1;

            unused_thread_index = iperf_get_unused_rx_thread_index();
            if (unused_thread_index == -1) {
                IPERF_PRINTF("TCP server get unused thread index failed!\r\n");
                if (tCxt) {
                    free(tCxt);
                    tCxt = NULL;
                }
                goto RET_OK;
            }

            if (serverRefCount == BENCH_TCP_MAX_SERVERS) {
                IPERF_PRINTF("%s: Max num of servers supported is %d, stop here.\n", __func__, BENCH_TCP_MAX_SERVERS);
                if (tCxt) {
                    free(tCxt);
                    tCxt = NULL;
                }
                goto RET_OK;
            }

            ret = nt_qurt_thread_create(iperf_tcp_rx, "tcp_rx", 3072, tCxt, iperf_rx_thread_priority,
                                        &(iperf_rx_thread_handle[unused_thread_index]));
            if (ret == -1) {
                IPERF_PRINTF("TCP server task creation failed\r\n");
                if (tCxt) {
                    free(tCxt);
                    tCxt = NULL;
                }

                goto RET_OK;
            } else {
                iperf_rx_thread_handle_sum++;
            }

            if (iperf_rx_thread_handle_sum > 1) {
                iperf_upgrade_rx_thread_priority();
            }

            // return;
#else
            iperf_tcp_rx(tCxt);
#endif
            if (tCxt->pktStats.iperf_display_interval) {
                ret = nt_qurt_thread_create(iperf_rx_show_result, "tcp_rx_rslt", 1024, tCxt, IPERF_RESULT_THREAD_PRIO,
                                            &(tCxt->rx_task_handler));
                if (ret == -1) {
                    IPERF_PRINTF("TCP result task creation failed\r\n");
                    goto RET_OK;
                }
                tCxt->result_create = true;
            }
        }
    } else {
        IPERF_PRINTF("Usage: iperf [-s|-c host] [options]\n");
        IPERF_PRINTF("Try `iperf -h` for more information.\n");
        goto RET_ERR;
    }

RET_OK:
    return QAPI_OK;

RET_ERR:
    if (tCxt) {
        free(tCxt);
        tCxt = NULL;
    }

    return QAPI_ERR_INVALID_PARAM;
}

#define RATE_KBYTES 1000
void iperf_result_print(STATS *pCxtPara, uint32_t prev, uint32_t cur, bool is_iperf_done)
{
    uint32_t throughput_Kbps = 0;
    uint32_t msInterval = 0;
    uint32_t rem_bytes = 0;
    char *transfer_unit = " ";
    char *bandwidth_unit = " ";
    uint32_t sec_val1, sec_val2;
    uint32_t bytes = 0;

    if (pCxtPara == NULL) {
        IPERF_PRINTF("error: illegal pCxtPara.\n");
        return;
    }
    msInterval = (cur - prev);
    // pCxtPara->total_bytes += pCxtPara->bytes;

    if (msInterval > 0) {
        throughput_Kbps = (pCxtPara->bytes / (msInterval / 8));

        bytes = pCxtPara->bytes;
        sec_val1 = (prev - pCxtPara->first_time) / 1000;
        sec_val2 = (cur - pCxtPara->first_time) / 1000;

        if (bytes > BYTES_PER_KILO_BYTE * BYTES_PER_KILO_BYTE) {
            transfer_unit = "M";
            rem_bytes = ((bytes % (BYTES_PER_KILO_BYTE * BYTES_PER_KILO_BYTE)) * 100) /
                        (BYTES_PER_KILO_BYTE * BYTES_PER_KILO_BYTE);
            bytes /= BYTES_PER_KILO_BYTE * BYTES_PER_KILO_BYTE;
        } else if (bytes > BYTES_PER_KILO_BYTE) {
            transfer_unit = "K";
            rem_bytes = ((bytes % (BYTES_PER_KILO_BYTE)) * 100) / BYTES_PER_KILO_BYTE;
            bytes /= BYTES_PER_KILO_BYTE;
        }
    } else {
        msInterval = (pCxtPara->last_time - pCxtPara->first_time);

        if (msInterval == 0) {
            return; /* error */
        }

        pCxtPara->iperf_time_sec -= pCxtPara->iperf_display_interval;

        /* Final stats */
        throughput_Kbps = (pCxtPara->total_bytes / (msInterval / 8));
        //(msInterval/1000)) * 8;

        sec_val1 = 0;
        sec_val2 = msInterval / 1000;

        if (pCxtPara->total_bytes > BYTES_PER_KILO_BYTE * BYTES_PER_KILO_BYTE * BYTES_PER_KILO_BYTE) {
            transfer_unit = "G";
            rem_bytes =
                ((pCxtPara->total_bytes % (BYTES_PER_KILO_BYTE * BYTES_PER_KILO_BYTE * BYTES_PER_KILO_BYTE)) * 100) /
                (BYTES_PER_KILO_BYTE * BYTES_PER_KILO_BYTE * BYTES_PER_KILO_BYTE);
            bytes = pCxtPara->total_bytes / (BYTES_PER_KILO_BYTE * BYTES_PER_KILO_BYTE * BYTES_PER_KILO_BYTE);
        } else if (pCxtPara->total_bytes > BYTES_PER_KILO_BYTE * BYTES_PER_KILO_BYTE) {
            transfer_unit = "M";
            rem_bytes = ((pCxtPara->total_bytes % (BYTES_PER_KILO_BYTE * BYTES_PER_KILO_BYTE)) * 100) /
                        (BYTES_PER_KILO_BYTE * BYTES_PER_KILO_BYTE);
            bytes = pCxtPara->total_bytes / (BYTES_PER_KILO_BYTE * BYTES_PER_KILO_BYTE);
        } else if (pCxtPara->total_bytes > BYTES_PER_KILO_BYTE) {
            transfer_unit = "K";
            rem_bytes = ((pCxtPara->total_bytes % (BYTES_PER_KILO_BYTE)) * 100) / BYTES_PER_KILO_BYTE;
            bytes = pCxtPara->total_bytes / BYTES_PER_KILO_BYTE;
        }
    }

    if (throughput_Kbps >= RATE_KBYTES) {
        bandwidth_unit = "M";
    } else if (pCxtPara->bytes > 0 || throughput_Kbps) {
        bandwidth_unit = "K";
    }
    if (throughput_Kbps >= RATE_KBYTES) {
        if (is_iperf_done) {
            IPERF_PRINTF(
                "### [%3d] %2d.0-%2d.0 sec %3d.%02d %sBytes %d.%02d %sbits/sec ###\n", pCxtPara->iperf_stream_id,
                sec_val1, sec_val2, bytes, rem_bytes, transfer_unit, throughput_Kbps / RATE_KBYTES,
                ((throughput_Kbps % RATE_KBYTES) / 10) + ((throughput_Kbps % 10 >= 5) ? 1 : 0), bandwidth_unit);
        } else {
            IPERF_PRINTF("[%3d] %2d.0-%2d.0 sec %3d.%02d %sBytes %d.%02d %sbits/sec\n", pCxtPara->iperf_stream_id,
                         sec_val1, sec_val2, bytes, rem_bytes, transfer_unit, throughput_Kbps / RATE_KBYTES,
                         ((throughput_Kbps % RATE_KBYTES) / 10) + ((throughput_Kbps % 10 >= 5) ? 1 : 0),
                         bandwidth_unit);
        }
    } else if (pCxtPara->bytes > 0 || throughput_Kbps > 0) {
        if (is_iperf_done) {
            IPERF_PRINTF("### [%3d] %2d.0-%2d.0 sec %3d.%02d %sBytes %d.%02d %sbits/sec ###\n",
                         pCxtPara->iperf_stream_id, sec_val1, sec_val2, bytes, rem_bytes, transfer_unit,
                         throughput_Kbps, 0, bandwidth_unit);
        } else {
            IPERF_PRINTF("[%3d] %2d.0-%2d.0 sec %3d.%02d %sBytes %d.%02d %sbits/sec\n", pCxtPara->iperf_stream_id,
                         sec_val1, sec_val2, bytes, rem_bytes, transfer_unit, throughput_Kbps, 0, bandwidth_unit);
        }
    }

    /* Clear for next time */
    pCxtPara->bytes = 0;
    pCxtPara->iperf_time_sec += pCxtPara->iperf_display_interval;
}

void iperf_result_print2(STATS *pCxtPara, uint32_t msInterval, uint64_t totalbyts, bool is_iperf_done)
{
    uint32_t throughput_Kbps = 0;

    uint32_t rem_bytes = 0;
    char *transfer_unit = " ";
    char *bandwidth_unit = " ";
    uint32_t sec_val1, sec_val2;
    uint32_t bytes = 0;

    if (msInterval == 0) {
        return; /* error */
    }

    /* Final stats */
    throughput_Kbps = (totalbyts / (msInterval / 8));
    //(msInterval/1000)) * 8;

    sec_val1 = 0;
    sec_val2 = msInterval / 1000;
    if ((msInterval % 1000) > 900) {
        sec_val2++;
    }

    if (totalbyts > BYTES_PER_KILO_BYTE * BYTES_PER_KILO_BYTE * BYTES_PER_KILO_BYTE) {
        transfer_unit = "G";
        rem_bytes = ((totalbyts % (BYTES_PER_KILO_BYTE * BYTES_PER_KILO_BYTE * BYTES_PER_KILO_BYTE)) * 100) /
                    (BYTES_PER_KILO_BYTE * BYTES_PER_KILO_BYTE * BYTES_PER_KILO_BYTE);
        bytes = totalbyts / (BYTES_PER_KILO_BYTE * BYTES_PER_KILO_BYTE * BYTES_PER_KILO_BYTE);
    } else if (totalbyts > BYTES_PER_KILO_BYTE * BYTES_PER_KILO_BYTE) {
        transfer_unit = "M";
        rem_bytes = ((totalbyts % (BYTES_PER_KILO_BYTE * BYTES_PER_KILO_BYTE)) * 100) /
                    (BYTES_PER_KILO_BYTE * BYTES_PER_KILO_BYTE);
        bytes = (uint32_t)(totalbyts / (BYTES_PER_KILO_BYTE * BYTES_PER_KILO_BYTE));
    } else if (totalbyts > BYTES_PER_KILO_BYTE) {
        transfer_unit = "K";
        rem_bytes = ((totalbyts % (BYTES_PER_KILO_BYTE)) * 100) / BYTES_PER_KILO_BYTE;
        bytes = totalbyts / BYTES_PER_KILO_BYTE;
    }

    if (throughput_Kbps >= RATE_KBYTES) {
        bandwidth_unit = "M";
    } else if (throughput_Kbps > 0) {
        bandwidth_unit = "K";
    }
    if (throughput_Kbps >= RATE_KBYTES) {
        if (is_iperf_done) {
            IPERF_PRINTF(
                "### [%3d] %2d.0-%2d.0 sec %3d.%02d %sBytes %d.%02d %sbits/sec ###\n", pCxtPara->iperf_stream_id,
                sec_val1, sec_val2, bytes, rem_bytes, transfer_unit, throughput_Kbps / RATE_KBYTES,
                ((throughput_Kbps % RATE_KBYTES) / 10) + ((throughput_Kbps % 10 >= 5) ? 1 : 0), bandwidth_unit);
        } else {
            IPERF_PRINTF("[%3d] %2d.0-%2d.0 sec %3d.%02d %sBytes %d.%02d %sbits/sec\n", pCxtPara->iperf_stream_id,
                         sec_val1, sec_val2, bytes, rem_bytes, transfer_unit, throughput_Kbps / RATE_KBYTES,
                         ((throughput_Kbps % RATE_KBYTES) / 10) + ((throughput_Kbps % 10 >= 5) ? 1 : 0),
                         bandwidth_unit);
        }
    } else if (throughput_Kbps > 0) {
        if (is_iperf_done) {
            IPERF_PRINTF("### [%3d] %2d.0-%2d.0 sec %3d.%02d %sBytes %d.%02d %sbits/sec ###\n",
                         pCxtPara->iperf_stream_id, sec_val1, sec_val2, bytes, rem_bytes, transfer_unit,
                         throughput_Kbps, 0, bandwidth_unit);
        } else {
            IPERF_PRINTF("[%3d] %2d.0-%2d.0 sec %3d.%02d %sBytes %d.%02d %sbits/sec\n", pCxtPara->iperf_stream_id,
                         sec_val1, sec_val2, bytes, rem_bytes, transfer_unit, throughput_Kbps, 0, bandwidth_unit);
        }
    }

    /* Clear for next time */
    pCxtPara->bytes = 0;
    pCxtPara->iperf_time_sec += pCxtPara->iperf_display_interval;
}

void pattern(char *outBuf, int inBytes)
{
    while (inBytes-- > 0) {
        outBuf[inBytes] = (inBytes % 10) + '0';
    }
}

int iperf_udp_tx_finish(THROUGHPUT_CXT *p_tCxt, uint32_t cur_packet_number)  //,struct sockaddr *to, uint32_t tolen)
{
    int32_t received;
    int error = A_ERROR;
    uint32_t retry_counter = 0;
    struct udp_datagram *mbuf_udp;
    int packet_number_fin = cur_packet_number * (-1);
    uint32_t now;
    char *ack_buf;
    uint32_t ack_buf_len = 0;
    server_hdr *received_server_hdr;
    struct timeval tv;
    fd_set rset;
    server_hdr *hdr;

    if (NULL == p_tCxt->buffer)
        return error;

    ack_buf_len = sizeof(udp_datagram) + sizeof(server_hdr);
    if (ack_buf_len > p_tCxt->params.tx_params.packet_size) {
        if ((ack_buf = malloc(ack_buf_len)) == NULL) {
            IPERF_PRINTF("%s: received_buf malloc error\r\n", __func__);
            return error;
        }
    } else {
        ack_buf = p_tCxt->buffer;  // reuse
        ack_buf_len = p_tCxt->params.tx_params.packet_size;
    }
    memset(ack_buf, 0, ack_buf_len);
    mbuf_udp = (struct udp_datagram *)ack_buf;
    now = hres_timer_curr_time_ms();
    mbuf_udp->id = htonl(packet_number_fin);
    mbuf_udp->tv_sec = htonl(now / 1000);
    mbuf_udp->tv_usec = htonl(now % 1000);

    hdr = (server_hdr *)(mbuf_udp + 1);
    hdr->flags = htonl(HEADER_VERSION1);
    hdr->total_len1 = htonl((long)(p_tCxt->pktStats.total_bytes >> 32));
    hdr->total_len2 = htonl((long)(p_tCxt->pktStats.total_bytes & 0xFFFFFFFF));
    hdr->stop_sec = htonl((p_tCxt->pktStats.last_time - p_tCxt->pktStats.first_time) / 1000);
    hdr->stop_usec = htonl(((p_tCxt->pktStats.last_time - p_tCxt->pktStats.first_time) % 1000) * 1000);

    memset(&rset, 0, sizeof(fd_set));
    FD_SET(p_tCxt->sock_peer, &rset);

    tv.tv_sec = 1;
    tv.tv_usec = 1000;

    while (retry_counter < 10) {
        int sent_bytes = 0;

        sent_bytes = send(p_tCxt->sock_peer, ack_buf, ack_buf_len, 0);

        if (sent_bytes < 0) {
            IPERF_PRINTF("ERROR: iperf_udp_tx_finish: UDP send terminate packet error %d , retry %d \r\n", sent_bytes,
                         retry_counter);

            retry_counter++;
            qurt_thread_sleep(1);
            continue;
        }
        FD_SET(p_tCxt->sock_peer, &rset);
        if ((select(p_tCxt->sock_peer + 1, &rset, NULL, NULL, &tv)) > 0) {
            /* Receive data from server*/
            received = recv(p_tCxt->sock_peer, ack_buf, ack_buf_len, 0);
            if (received <= 0) {
                IPERF_PRINTF("INFO: iperf_udp_tx_finish: received none\r\n");
            } else if (received >= (int)(sizeof(udp_datagram) + sizeof(server_hdr))) {
                uint32_t interval, interval2, len2, len1;
                uint64_t total;
                received_server_hdr = (server_hdr *)(ack_buf + sizeof(udp_datagram));
                interval = ntohl(received_server_hdr->stop_sec);
                interval2 = ntohl(received_server_hdr->stop_usec) / 1000;
                len1 = ntohl(received_server_hdr->total_len1);
                len2 = ntohl(received_server_hdr->total_len2);
                total = len1;
                total = (uint64_t)(total << 32) + len2;
                // IPERF_PRINTF("got from server: total:0x%xbytes, \n", total);
                // IPERF_PRINTF("got from server:  time %d.%d s\n", interval, interval2);
                // IPERF_PRINTF("got from server: interval:%d, len2:%d, interval2:%d\n",interval,len2, interval2);
                // iperf_result_print2(&p_tCxt->pktStats, interval * 1000 + interval2, total, true);
            }
            error = QAPI_OK;
            break;
        } else {
            retry_counter++;
        }
    }
    if (retry_counter == 10)
        IPERF_PRINTF("WARNING: iperf_udp_tx_finish:wait server ack timeout\r\n");

    if (ack_buf != p_tCxt->buffer)
        free(ack_buf);

    return error;
}

/* -------------------------------------------------------------------
 * Send an AckFIN (a datagram acknowledging a FIN) on the socket,
 * then select on the socket for some time. If additional datagrams
 * come in, probably our AckFIN was lost and they are re-transmitted
 * termination datagrams, so re-transmit our AckFIN.
 * ------------------------------------------------------------------- */

void iperf_udp_ack_finish(THROUGHPUT_CXT *p_tCxt, struct sockaddr *faddr, uint32_t addrlen)
{
    int conn;
    uint8_t ack_buf[sizeof(udp_datagram) + sizeof(server_hdr)];
    udp_datagram *udp_hdr;
    server_hdr *hdr;
    struct timeval timeout;
    int count = 0;
    int bytes;
    int sent_bytes = 0;

    fd_set readSet;
    memset(&readSet, 0, sizeof(fd_set));
    FD_SET(p_tCxt->sock_local, &readSet);
    memset(ack_buf, 0, sizeof(ack_buf));
    while (count < 10) {
        udp_hdr = (udp_datagram *)ack_buf;
        memscpy(udp_hdr, sizeof(udp_datagram), p_tCxt->buffer, sizeof(udp_datagram));

        hdr = (server_hdr *)(udp_hdr + 1);
        hdr->flags = htonl(HEADER_VERSION1);
        hdr->total_len1 = htonl((long)(p_tCxt->pktStats.total_bytes >> 32));
        hdr->total_len2 = htonl((long)(p_tCxt->pktStats.total_bytes & 0xFFFFFFFF));
        hdr->stop_sec = htonl((p_tCxt->pktStats.last_time - p_tCxt->pktStats.first_time) / 1000);
        hdr->stop_usec = htonl(((p_tCxt->pktStats.last_time - p_tCxt->pktStats.first_time) % 1000) * 1000);

        // write data
        sent_bytes = sendto(p_tCxt->sock_local, ack_buf, sizeof(ack_buf), 0, faddr, addrlen);
        if (sent_bytes < 0) {
            IPERF_PRINTF("UDP send terminate packet error %d , retry %d \r\n", sent_bytes, count);

            count++;
            qurt_thread_sleep(1);
            continue;
        }

        if ((iperf_rx_quit) || (get_device_connect_state() == false)) {
            IPERF_PRINTF("4\n");
            break;
        }

        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        FD_SET(p_tCxt->sock_local, &readSet);
        conn = select(p_tCxt->sock_local + 1, &readSet, NULL, NULL, &timeout);

        if (conn == 0) {
            return;
        } else {
            // socket ready to read
            bytes = recvfrom(p_tCxt->sock_local, ack_buf, sizeof(ack_buf), 0, faddr, &addrlen);

            if (bytes <= 0) {
                // Connection closed or errored
                // Stop using it.
                return;
            }
        }
    }
}

static void iperf_client_send(void *arg)
{
    THROUGHPUT_CXT *p_tCxt = (THROUGHPUT_CXT *)arg;
    if (p_tCxt == NULL) {
        IPERF_PRINTF("ERROR: p_tCxt in iperf_client_send() is NULL\r\n");
        goto QUIT;
    }
    uint32_t cur_packet_number, n_send_ok, n_send_fail;
    int32_t send_bytes;
    uint32_t packet_size = p_tCxt->params.tx_params.packet_size;
    uint32_t now;
    struct udp_datagram *mbuf_udp;
    struct client_hdr *mbuf_udp_client_hdr;
    /* iperf display related */
    uint32_t iperf_display_interval = 0;
    uint32_t iperf_display_last = 0;
    uint32_t iperf_display_next = 0;

    /* iperf bandwidth limitation */
    uint32_t iperf_udp_packets_per_second = 0;
    uint32_t iperf_udp_packets_counter = 0;
    uint32_t iperf_udp_start_time = 0;
    uint32_t iperf_curr_time;
    uint32_t iperf_tcp_packets_per_second = 0;
    uint32_t iperf_tcp_packets_per_divided_second = 0;
    uint32_t iperf_tcp_packets_per_res_second = 0;
    uint32_t iperf_tcp_packets_counter = 0;
    uint32_t iperf_tcp_start_time = 0;
    uint32_t qurt_sleep_one_second_to_be_divided = 10;
    uint32_t qurt_sleep_counter = 0;

    /* Sending.*/
    IPERF_PRINTF("Sending\r\n");
    /*Reset all counters*/
    cur_packet_number = 0;
    n_send_ok = 0;
    n_send_fail = 0;

    if (p_tCxt->protocol == TCP) {
        iperf_decrs_tcp_queue_pbuf_thrsh();
    }

    app_get_time(&p_tCxt->pktStats.first_time);

    /* Convert bps to B/s, and then to packets/sec */
    if (p_tCxt->protocol == UDP) {
        if (p_tCxt->bandwidth_unit == 0) /* Mbps */
        {
            iperf_udp_packets_per_second =
                1 + ((p_tCxt->pktStats.iperf_udp_rate * Mbps / 8) / p_tCxt->params.tx_params.packet_size);
        } else /* Kbps */
        {
            iperf_udp_packets_per_second =
                1 + ((p_tCxt->pktStats.iperf_udp_rate * Kbps / 8) / p_tCxt->params.tx_params.packet_size);
        }
        app_get_time(&iperf_udp_start_time);
    } else if (p_tCxt->protocol == TCP && p_tCxt->pktStats.iperf_tcp_rate != 0) {
        /* calculate the packets according to the rate */
        if (p_tCxt->bandwidth_unit == 0) {
            iperf_tcp_packets_per_second =
                1 + ((p_tCxt->pktStats.iperf_tcp_rate * Mbps / 8) / p_tCxt->params.tx_params.packet_size);
        } else {
            iperf_tcp_packets_per_second =
                1 + ((p_tCxt->pktStats.iperf_tcp_rate * Kbps / 8) / p_tCxt->params.tx_params.packet_size);
        }

        /* divide the time to different slices according to the packets send per second */
        if (iperf_tcp_packets_per_second > 20) {
            qurt_sleep_one_second_to_be_divided = 10;
            iperf_tcp_packets_per_divided_second = iperf_tcp_packets_per_second / qurt_sleep_one_second_to_be_divided;
            iperf_tcp_packets_per_res_second = iperf_tcp_packets_per_second - iperf_tcp_packets_per_divided_second *
                                                                                  qurt_sleep_one_second_to_be_divided;
        } else if (iperf_tcp_packets_per_second > 5 && iperf_tcp_packets_per_second <= 20) {
            qurt_sleep_one_second_to_be_divided = 5;
            iperf_tcp_packets_per_divided_second = iperf_tcp_packets_per_second / qurt_sleep_one_second_to_be_divided;
            iperf_tcp_packets_per_res_second = iperf_tcp_packets_per_second - iperf_tcp_packets_per_divided_second *
                                                                                  qurt_sleep_one_second_to_be_divided;
        } else {
            qurt_sleep_one_second_to_be_divided = 5;
            iperf_tcp_packets_per_divided_second = 1;
            iperf_tcp_packets_per_res_second = 0;
        }

        app_get_time(&iperf_tcp_start_time);
    }
    iperf_display_interval = p_tCxt->pktStats.iperf_display_interval;  // second
    iperf_display_last = p_tCxt->pktStats.first_time;
    iperf_display_next = iperf_display_last + iperf_display_interval * 1000;

    uint32_t is_test_done = 0;
    p_tCxt->iperf_stream_id = iperf_get_unused_id();
    if (p_tCxt->iperf_stream_id == MAX_STREAM) {
        goto QUIT;
    }
    p_tCxt->pktStats.iperf_stream_id = p_tCxt->iperf_stream_id;
    while (!is_test_done) {
        if ((iperf_tx_quit) || (get_device_connect_state() == false)) {
            app_get_time(&p_tCxt->pktStats.last_time);
            break;
        }

        /* allocate the buffer, if needed */
        if (p_tCxt->buffer == NULL) {
            while ((p_tCxt->buffer = malloc(packet_size)) == NULL) {
                /*Wait till we get a buffer*/
                if ((iperf_tx_quit) || (get_device_connect_state() == false)) {
                    app_get_time(&p_tCxt->pktStats.last_time);
                    goto QUIT;
                }
                /*Allow small delay to allow other thread to run*/
                qurt_thread_sleep(100);
            }

            pattern(p_tCxt->buffer, packet_size);
            now = hres_timer_curr_time_ms();
            mbuf_udp = (struct udp_datagram *)p_tCxt->buffer;
            mbuf_udp->id = htonl(cur_packet_number);
            mbuf_udp->tv_sec = htonl(now / 1000);
            mbuf_udp->tv_usec = htonl((now % 1000) * 1000);

            mbuf_udp_client_hdr = (struct client_hdr *)(p_tCxt->buffer + sizeof(struct udp_datagram));
            int buflen = (packet_size < sizeof(struct udp_datagram) + sizeof(struct client_hdr))
                             ? packet_size - sizeof(struct udp_datagram)
                             : sizeof(struct client_hdr);

            memset(mbuf_udp_client_hdr, 0, buflen);

        } else {
            pattern(p_tCxt->buffer, packet_size);
        }

        do {
            if ((iperf_tx_quit) || (get_device_connect_state() == false)) {
                app_get_time(&p_tCxt->pktStats.last_time);
                is_test_done = 1;
                break;
            }
            {
                // send_bytes = sendto(p_tCxt->sock_peer, p_tCxt->buffer, packet_size, 0, &gto, gtolen);
                send_bytes = send(p_tCxt->sock_peer, p_tCxt->buffer, packet_size, 0);
            }

            if (send_bytes != (int)packet_size) {
                n_send_fail++;

                if (ENOMEM == errno || ERR_INPROGRESS == errno) {
                    qurt_thread_sleep(5);
                } else {
                    IPERF_PRINTF("ERROR: iperf_client_send: TX err:%d\r\n", errno);
                    if (EAGAIN == errno || ENOMEM == errno)
                        break;
                    app_get_time(&p_tCxt->pktStats.last_time);
                    if (ERR_TIMEOUT == errno || ERR_OK == errno) {
                        if (!iperf_common_check_test_time(p_tCxt))
                            continue;
                    }

                    is_test_done = 1;

                    break;
                }
            } else {
                cur_packet_number++;
            }

#ifdef UDP_TX_DEBUG
            IPERF_PRINTF("%d send_bytes = %d\n", cur_packet_number, send_bytes);
#endif
            app_get_time(&now);
            if (send_bytes > 0) {
                p_tCxt->pktStats.bytes += send_bytes;
                p_tCxt->pktStats.total_bytes += send_bytes;
                ++n_send_ok;

                mbuf_udp = (struct udp_datagram *)p_tCxt->buffer;
                mbuf_udp->id = htonl(cur_packet_number);
                mbuf_udp->tv_sec = htonl(now / 1000);
                mbuf_udp->tv_usec = htonl((now % 1000) * 1000);

                // if (p_tCxt->print_buf)
                //     iperf_print_buffer(p_tCxt->buffer, send_bytes, to, DUMP_DIRECTION_TX);
            }

            if (p_tCxt->pktStats.iperf_display_interval) {
                iperf_curr_time = now;
                if (iperf_curr_time >= iperf_display_next && iperf_display_interval) {
                    iperf_result_print(&p_tCxt->pktStats, iperf_display_last, iperf_curr_time, false);
                    iperf_display_last = iperf_curr_time;
                    iperf_display_next = iperf_curr_time + iperf_display_interval * 1000;
                    iperf_udp_packets_counter = 0;
                }
            }

            /* iperf bandwidth */
            if (p_tCxt->protocol == UDP) {
                iperf_udp_packets_counter++;

                if (iperf_udp_packets_counter == iperf_udp_packets_per_second) {
                    uint32_t iperf_diff_time = 0;

                    /* Get the current time and calculate the sleep needed till the end of the second */
                    // app_get_time(&iperf_curr_time);
                    iperf_curr_time = now;
                    iperf_diff_time = iperf_curr_time - iperf_udp_start_time;

                    // IPERF_PRINTF("@%d,iperf_udp_packets_counter:%d,iperf_udp_packets_per_second:%d",
                    //              iperf_diff_time, iperf_udp_packets_counter, iperf_udp_packets_per_second);
                    /* Check that the diff is less than a second. If it's more than a second,
                     * it means that we were asked to limit the bandwidth to a value we cannot
                     * reach, so we are behind. In this case, no sleep is required, just push as much as
                     * we can...
                     */
                    if (iperf_diff_time < 1000) {
                        qurt_thread_sleep(1000 - iperf_diff_time);
                    }

                    /* Restart the timer and clear the counter */
                    app_get_time(&iperf_udp_start_time);
                    iperf_udp_packets_counter = 0;
                }
            } else if (p_tCxt->protocol == TCP && p_tCxt->pktStats.iperf_tcp_rate != 0) {
                iperf_tcp_packets_counter++;

                if ((qurt_sleep_counter < qurt_sleep_one_second_to_be_divided - 1) &&
                        (iperf_tcp_packets_counter == iperf_tcp_packets_per_divided_second) ||
                    (qurt_sleep_counter == qurt_sleep_one_second_to_be_divided - 1) &&
                        (iperf_tcp_packets_counter ==
                         iperf_tcp_packets_per_divided_second + iperf_tcp_packets_per_res_second)) {
                    uint32_t iperf_diff_time = 0;

                    /* Get the current time and calculate the sleep needed till the end of the second */
                    // app_get_time(&iperf_curr_time);
                    iperf_curr_time = now;
                    iperf_diff_time = iperf_curr_time - iperf_tcp_start_time;

                    /* Check that the diff is less than a second. If it's more than
                     * 1/QURT_SLEEP_ONE_SECOND_TO_BE_DIVIDED second, it means that we were asked to limit the bandwidth
                     * to a value we cannot reach, so we are behind. In this case, no sleep is required, just push as
                     * much as we can...
                     */
                    if (qurt_sleep_counter == qurt_sleep_one_second_to_be_divided - 1)
                        qurt_sleep_counter = 0;

                    if (iperf_diff_time < (1000 / qurt_sleep_one_second_to_be_divided)) {
                        qurt_thread_sleep((1000 / qurt_sleep_one_second_to_be_divided) - iperf_diff_time);
                        qurt_sleep_counter++;
                    }

                    /* Restart the timer and clear the counter */
                    app_get_time(&iperf_tcp_start_time);
                    iperf_tcp_packets_counter = 0;
                }
            }
            /*Test mode can be "number of packets" or "fixed time duration"*/
            if (p_tCxt->params.tx_params.test_mode == PACKET_TEST) {
                if ((cur_packet_number >= p_tCxt->params.tx_params.packet_number)) {
                    app_get_time(&p_tCxt->pktStats.last_time);
                    is_test_done = 1;
                    if (p_tCxt->pktStats.iperf_display_interval) {
                        app_get_time(&iperf_curr_time);
                        if (iperf_curr_time >= iperf_display_next && iperf_display_interval) {
                            iperf_result_print(&p_tCxt->pktStats, iperf_display_last, iperf_curr_time, false);
                            iperf_display_last = iperf_curr_time;
                            iperf_display_next = iperf_curr_time + iperf_display_interval * 1000;
                            iperf_udp_packets_counter = 0;
                        }
                    }
                    break;
                }
            } else if (p_tCxt->params.tx_params.test_mode == TIME_TEST) {
                // app_get_time(&p_tCxt->pktStats.last_time);
                p_tCxt->pktStats.last_time = now;
                if (iperf_common_check_test_time(p_tCxt)) {
                    is_test_done = 1;
                    if (p_tCxt->pktStats.iperf_display_interval) {
                        app_get_time(&iperf_curr_time);
                        /*
                            When ending the test, but the time is less than 1s, will not print the result.
                            here -100ms to print the last result.
                        */
                        if (iperf_curr_time >= iperf_display_next - 100 && iperf_display_interval) {
                            iperf_result_print(&p_tCxt->pktStats, iperf_display_last, iperf_curr_time, false);
                            iperf_display_last = iperf_curr_time;
                            iperf_display_next = iperf_curr_time + iperf_display_interval * 1000;
                            iperf_udp_packets_counter = 0;
                        }
                    }
                    break;
                }
            }
        } while (!((is_test_done) || /*(send_bytes == (int)packet_size) ||*/ (NULL == p_tCxt->buffer))); /* send loop */
    } /* while ( !is_test_done ) */

    iperf_result_print(&p_tCxt->pktStats, 0, 0, true);

QUIT:
    /* Send endmark packet and wait for stats from server */
    if (p_tCxt->buffer) {
        if ((p_tCxt->protocol == UDP) && (get_device_connect_state() == true))
            iperf_udp_tx_finish(p_tCxt, cur_packet_number);
    }

    if (p_tCxt) {
        if (A_ERROR != p_tCxt->sock_peer) {
            IPERF_PRINTF("INFO: %s: %s : IPERF socket: close:%d\r\n", __func__,
                         (p_tCxt->protocol == UDP) ? "UDP" : "TCP", p_tCxt->sock_peer);
            closesocket(p_tCxt->sock_peer);
        }

        if (errno == EBADF)
            errno = 0;

        qurt_thread_sleep(10 * p_tCxt->iperf_stream_id);

        if (p_tCxt->iperf_stream_id < MAX_STREAM) {
            iperf_stream_id[p_tCxt->iperf_stream_id] = 0;
        }

        if (p_tCxt->buffer) {
            free(p_tCxt->buffer);
            p_tCxt->buffer = NULL;
        }

        if (p_tCxt->protocol == TCP) {
            iperf_incrs_tcp_queue_pbuf_thrsh();
        }

        free(p_tCxt);
        p_tCxt = NULL;
    }
    IPERF_PRINTF(BENCH_TEST_COMPLETED);
    nt_osal_thread_delete(NULL);
}
/************************************************************************
 * NAME: iperf_udp_tx
 *
 * DESCRIPTION: Start TX UDP throughput test.
 ************************************************************************/

void iperf_udp_tx(THROUGHPUT_CXT *p_tCxt)
{
    if (p_tCxt == NULL) {
        IPERF_PRINTF("ERROR: p_tCxt in iperf_udp_tx() is NULL\n");
        goto QUIT;
    }

#if LWIP_IPV4
    struct sockaddr_in foreign_addr;
#endif
#if LWIP_IPV6
    struct sockaddr_in6 foreign_addr6;
#endif
    struct sockaddr *to;
    uint32_t tolen;
    char ip_str[48];
    int family;
    int tos_opt;
    struct sockaddr_in src_sin;
    err_t ret;
    p_tCxt->sock_peer = A_ERROR;

    if (p_tCxt->params.tx_params.v6) {
#if LWIP_IPV6
        family = AF_INET6;
        inet_ntop(family, p_tCxt->params.tx_params.v6addr, ip_str, sizeof(ip_str));

        memset(&foreign_addr6, 0, sizeof(foreign_addr6));
        memscpy(&foreign_addr6.sin6_addr, sizeof(foreign_addr6.sin6_addr), p_tCxt->params.tx_params.v6addr,
                sizeof(foreign_addr6.sin6_addr));
        foreign_addr6.sin6_port = htons(p_tCxt->params.tx_params.port);
        foreign_addr6.sin6_family = family;
        foreign_addr6.sin6_scope_id = p_tCxt->params.tx_params.scope_id;
        foreign_addr6.sin6_flowinfo = 0;

        to = (struct sockaddr *)&foreign_addr6;
        tolen = sizeof(foreign_addr6);
        tos_opt = IPV6_TCLASS;
#else
        goto QUIT;
#endif
    } else {
#if LWIP_IPV4
        family = AF_INET;
        inet_ntop(family, &p_tCxt->params.tx_params.ip_address, ip_str, sizeof(ip_str));

        memset(&foreign_addr, 0, sizeof(foreign_addr));
        foreign_addr.sin_addr.s_addr = p_tCxt->params.tx_params.ip_address;
        foreign_addr.sin_port = htons(p_tCxt->params.tx_params.port);
        foreign_addr.sin_family = family;

        src_sin.sin_family = family;
        src_sin.sin_addr.s_addr = p_tCxt->params.tx_params.source_ipv4_addr;
        src_sin.sin_port = htons(0);

        to = (struct sockaddr *)&foreign_addr;
        tolen = sizeof(foreign_addr);
        tos_opt = IP_TOS;
#else
        goto QUIT;
#endif
    }

    IPERF_PRINTF("------------------------------------------------------------\n");
    if (p_tCxt->bandwidth_unit == 0)
        IPERF_PRINTF("Client connecting to %s, UDP port %d, bandwidth:%dMbps\n", ip_str, p_tCxt->params.tx_params.port,
                     p_tCxt->pktStats.iperf_udp_rate);
    else
        IPERF_PRINTF("Client connecting to %s, UDP port %d, bandwidth:%dKbps\n", ip_str, p_tCxt->params.tx_params.port,
                     p_tCxt->pktStats.iperf_udp_rate);
    IPERF_PRINTF("------------------------------------------------------------\n");

    /* Create UDP socket */
    if ((p_tCxt->sock_peer = socket(family, SOCK_DGRAM, 0)) == A_ERROR)  // IPPROTO_UDP
    {
        IPERF_PRINTF("ERROR: iperf_udp_tx: Socket creation failed\n");
        goto QUIT;
    }

    IPERF_PRINTF("INFO: %s: IPERF socket: open:%d\r\n", __func__, p_tCxt->sock_peer);

    if (p_tCxt->params.tx_params.source_ipv4_addr != 0) {
        if (bind(p_tCxt->sock_peer, (struct sockaddr *)&src_sin, sizeof(src_sin)) == A_ERROR) {
            IPERF_PRINTF("ERROR: iperf_udp_tx: Socket bind failed\n");
            goto QUIT;
        }
    }

    if (p_tCxt->params.tx_params.ip_tos > 0) {
        if (setsockopt(p_tCxt->sock_peer, IPPROTO_IP, tos_opt, &p_tCxt->params.tx_params.ip_tos, sizeof(int)) < 0) {
            goto QUIT;
        }
    }
    if (p_tCxt->params.tx_params.v6 && IS_IPV6_MULTICAST(p_tCxt->params.tx_params.v6addr)) {
#if LWIP_IPV6
        uint32_t val;

        /* Configure value to be used in the Hop Limit field in IPv6 header of
         * outgoing multicast datagrams
         */
        val = 16;
        if (setsockopt(p_tCxt->sock_peer, IPPROTO_IP, IPV6_MULTICAST_HOPS, &val, sizeof(int)) < 0) {
            goto QUIT;
        }

        /* disable local loopback of outgoing multicast datagrams */
        val = IPV6_MC_LPBK_DIS;
        if (setsockopt(p_tCxt->sock_peer, IPPROTO_IP, IPV6_MULTICAST_LOOP, &val, sizeof(unsigned int)) < 0) {
            goto QUIT;
        }

        if (setsockopt(p_tCxt->sock_peer, IPPROTO_IP, IPV6_MULTICAST_IF, &foreign_addr6.sin6_scope_id,
                       sizeof(unsigned int)) < 0) {
            goto QUIT;
        }
#else
        goto QUIT;
#endif
    }

    /* Connect to the server.*/

    if (connect(p_tCxt->sock_peer, to, tolen) == A_ERROR) {
        IPERF_PRINTF("Connection failed\n");
        goto QUIT;
    }

    ret = nt_qurt_thread_create(iperf_client_send, "udp_client", 1024, p_tCxt, IPERF_TX_THREAD_PRIO, NULL);

    if (ret == -1) {
        IPERF_PRINTF("ERROR: iperf_udp_tx: UDP client task creation failed out of memory\r\n");
        goto QUIT;
    }
    return;

//////////////////////////////////////////////////////////////////////////
QUIT:
    if (A_ERROR != p_tCxt->sock_peer) {
        closesocket(p_tCxt->sock_peer);
        IPERF_PRINTF("INFO: %s: IPERF socket: close:%d\r\n", __func__, p_tCxt->sock_peer);
    }

    IPERF_PRINTF(BENCH_TEST_COMPLETED);

    if (p_tCxt) {
        free(p_tCxt);
        p_tCxt = NULL;
    }

    return;
}

void iperf_rx_show_result(void *arg)
{
    STATS *ppktStats = NULL;
    uint32_t iperf_curr_time = 0;
    uint32_t iperf_last_time = 0;
    uint32_t iperf_display_interval;
    THROUGHPUT_CXT *p_tCxt = (THROUGHPUT_CXT *)arg;

    if (p_tCxt == NULL) {
        return;
    }

    iperf_display_interval = p_tCxt->pktStats.iperf_display_interval * 1000;  // ms

    while (1) {
        qurt_thread_sleep(iperf_display_interval);

        if (iperf_rx_quit == 1 || p_tCxt->quit)
            goto QUIT;

        app_get_time(&iperf_curr_time);

        if (p_tCxt->protocol == UDP) {
            if (p_tCxt->pktStats.first_time == 0) {
                continue;
            }

            if (p_tCxt->pktStats.last_time) {
                iperf_curr_time = p_tCxt->pktStats.last_time;
                iperf_last_time = p_tCxt->pktStats.prev_time;
                ppktStats = &p_tCxt->pktStats;
            } else {
                iperf_last_time = p_tCxt->pktStats.prev_time;
                p_tCxt->pktStats.prev_time = iperf_curr_time;
                ppktStats = &p_tCxt->pktStats;
            }
        } else if (p_tCxt->protocol == TCP) {
            bench_tcp_session_t *session = (bench_tcp_session_t *)(p_tCxt->session);
            if (session) {
                if (session->pktStats.first_time == 0) {
                    continue;
                }

                iperf_last_time = session->iperf_display_last;
                session->iperf_display_last = iperf_curr_time;
                ppktStats = &session->pktStats;
            } else
                continue;
        } else {
            break;
        }

        if (ppktStats) {
            iperf_result_print(ppktStats, iperf_last_time, iperf_curr_time, false);
        } else {
            IPERF_PRINTF("ERROR: ppktStats of p_tCxt is NULL\n");
        }
    }

QUIT:
    p_tCxt->result_create = false;
    nt_osal_thread_delete(NULL);
    return;
}

/************************************************************************
 * NAME: iperf_udp_rx
 *
 * DESCRIPTION: Start throughput UDP server.
 ************************************************************************/
#if IPERF_RX_THREAD
void iperf_udp_rx(void *arg)
#else
void iperf_udp_rx(THROUGHPUT_CXT *p_tCxt)
#endif
{
#if IPERF_RX_THREAD
    THROUGHPUT_CXT *p_tCxt = (THROUGHPUT_CXT *)arg;
#endif
    if (p_tCxt == NULL) {
        IPERF_PRINTF("ERROR: p_tCxt in iperf_udp_rx() is NULL\r\n");
        goto QUIT;
    }

    int32_t received;
    struct sockaddr *addr;
    int addrlen;
    uint32_t fromlen;
    struct sockaddr *from;
#if LWIP_IPV4
    struct sockaddr_in local_addr;
    struct sockaddr_in foreign_addr;
#endif
#if LWIP_IPV6
    struct sockaddr_in6 local_addr6;
    struct sockaddr_in6 foreign_addr6;
#endif
    char ip_str[48];
    int family;
    uint16_t port;
    struct timeval tv;
    int32_t conn_sock;
    int is_test_done = 0;
    int32_t udp_datagram_size = (int32_t)sizeof(udp_datagram);
    int ret = 0;
    p_tCxt->sock_local = A_ERROR;

    p_tCxt->iperf_stream_id = iperf_get_unused_id();
    if (p_tCxt->iperf_stream_id == MAX_STREAM) {
        goto QUIT;
    }
    p_tCxt->pktStats.iperf_stream_id = p_tCxt->iperf_stream_id;

    if ((p_tCxt->buffer = malloc(CFG_PACKET_SIZE_MAX_RX)) == NULL) {
        IPERF_PRINTF("ERROR: %s: Out of memory error\r\n", __func__);
        goto QUIT;
    }

    port = p_tCxt->params.rx_params.port;

    if (p_tCxt->params.rx_params.v6) {
#if LWIP_IPV6
        family = AF_INET6;
        from = (struct sockaddr *)&foreign_addr6;
        addr = (struct sockaddr *)&local_addr6;
        // local_sin_addr = &local_addr6.sin6_addr;
        fromlen = addrlen = sizeof(struct sockaddr_in6);

        memset(&local_addr6, 0, sizeof(local_addr6));
        local_addr6.sin6_port = htons(port);
        local_addr6.sin6_family = family;
        memscpy(&local_addr6.sin6_addr, sizeof(struct ip6_addr), p_tCxt->params.rx_params.local_v6addr,
                sizeof(struct ip6_addr));
#else
        goto QUIT;
#endif
    } else {
#if LWIP_IPV4
        family = AF_INET;
        from = (struct sockaddr *)&foreign_addr;
        addr = (struct sockaddr *)&local_addr;
        // local_sin_addr = &local_addr.sin_addr;
        fromlen = addrlen = sizeof(struct sockaddr_in);

        memset(&local_addr, 0, sizeof(local_addr));
        local_addr.sin_port = htons(port);
        local_addr.sin_family = family;
        local_addr.sin_addr.s_addr = p_tCxt->params.rx_params.local_address;
#else
        goto QUIT;
#endif
    }

    /* Open socket */
    if ((p_tCxt->sock_local = socket(family, SOCK_DGRAM, 0)) == A_ERROR) {
        IPERF_PRINTF("ERROR: %s: Socket creation error.\r\n", __func__);
        goto QUIT;
    }
    IPERF_PRINTF("INFO: %s: IPERF socket: open:%d\r\n", __func__, p_tCxt->sock_local);

    /* Bind */
    if (bind(p_tCxt->sock_local, addr, addrlen) != QAPI_OK) {
        IPERF_PRINTF("ERROR: %s: Socket bind error.\r\n", __func__);
        goto QUIT;
    }

    if (p_tCxt->params.rx_params.mcEnabled) {
        if (p_tCxt->params.rx_params.v6) {
#if LWIP_IPV6
            struct ipv6_mreq group6;
            memscpy(&group6.ipv6mr_multiaddr, sizeof(struct ip6_addr), p_tCxt->params.rx_params.mcIpv6addr,
                    sizeof(struct ip6_addr));
            group6.ipv6mr_interface = p_tCxt->params.rx_params.scope_id;
            if (setsockopt(p_tCxt->sock_local, IPPROTO_IP, IPV6_JOIN_GROUP, (void *)&group6, sizeof(group6)) !=
                QAPI_OK) {
                IPERF_PRINTF("ERROR: %s: Socket set option failure.\r\n", __func__);
                goto QUIT;
            }
#else
            goto QUIT;
#endif
        } else {
            /*
            struct ip_mreq group;
            group.imr_multiaddr = p_tCxt->params.rx_params.mcIpaddr;
            if(p_tCxt->params.rx_params.local_address)
                group.imr_interface = p_tCxt->params.rx_params.local_address;
            else
                group.imr_interface = p_tCxt->params.rx_params.mcRcvIf;
            if (setsockopt(p_tCxt->sock_local, IPPROTO_IP, IP_ADD_MEMBERSHIP, (void *)&group, sizeof(group)) != QAPI_OK)
            {
                IPERF_PRINTF( "ERROR: Socket set option failure.\n");
                goto ERROR_2;
            }
            */
        }
    }

    memset(ip_str, 0, sizeof(ip_str));

    IPERF_PRINTF("------------------------------------------------------------\n");
    IPERF_PRINTF("Server listening on UDP port %d\n", port);
    IPERF_PRINTF("------------------------------------------------------------\n");

    while ((!iperf_rx_quit) && (get_device_connect_state() == true)) /* Main loop */
    {
        int id = 0;
        fd_set read_fds;
        int32_t is_first = 1;
        stat_udp_pattern_t stat_udp;
        struct udp_datagram *mbuf_udp = (struct udp_datagram *)p_tCxt->buffer;

        IPERF_PRINTF("Waiting\r\n");
        is_test_done = 0;

        iperf_common_clear_stats(p_tCxt);
        memset(ip_str, 0, sizeof(ip_str));
        memset(&stat_udp, 0, sizeof(stat_udp_pattern_t));
        p_tCxt->pktStats.prev_time = p_tCxt->pktStats.last_time = p_tCxt->pktStats.first_time = 0;

        /* block for 2s or until a packet is received */
        tv.tv_sec = 2;
        tv.tv_usec = 0;

        while (!is_test_done) /* Receive loop */
        {
            if ((iperf_rx_quit) || (get_device_connect_state() == false)) {
                app_get_time(&p_tCxt->pktStats.last_time);
                goto QUIT;
            }
            do {
                if ((iperf_rx_quit) || (get_device_connect_state() == false)) {
                    app_get_time(&p_tCxt->pktStats.last_time);
                    goto QUIT;
                }

                /* block for 2s or until a packet is received */
                memset(&read_fds, 0, sizeof(fd_set));
                FD_SET(p_tCxt->sock_local, &read_fds);

                conn_sock = select(p_tCxt->sock_local + 1, &read_fds, NULL, NULL, &tv);
                if (conn_sock == A_ERROR) {
                    app_get_time(&p_tCxt->pktStats.last_time);
                    goto QUIT;  // socket no longer valid
                } else if (conn_sock == 0) {
                    /* if udp tx has started and no activity for continues 2s, early quit udp rx */
                    uint32_t curtime;
                    app_get_time(&curtime);
                    if ((is_first == 0) && (curtime - p_tCxt->pktStats.first_time) / 1000 > 20) {
                        app_get_time(&p_tCxt->pktStats.last_time);
                        goto QUIT;
                    }
                }

            } while (conn_sock == 0);

            // check recv data
            received =
                recvfrom(p_tCxt->sock_local, (char *)(&p_tCxt->buffer[0]), CFG_PACKET_SIZE_MAX_RX, 0, from, &fromlen);

            if (received >= 0) {
                if (received > udp_datagram_size) {
                    p_tCxt->pktStats.bytes += received;
                    p_tCxt->pktStats.total_bytes += received;
                    ++p_tCxt->pktStats.pkts_recvd;
                    id = ntohl(mbuf_udp->id);

                    if (is_first) {
                        app_get_time(&p_tCxt->pktStats.first_time);
                        p_tCxt->pktStats.prev_time = p_tCxt->pktStats.first_time;
                        is_first = 0;
                    }

                    if (id < 0) {
                        is_test_done = 1;
                        app_get_time(&p_tCxt->pktStats.last_time);
                    }
                }
            } else {
                IPERF_PRINTF("ERROR: %s received= %d\r\n", __func__, received);
                break;
            }
        } /* receive_loop */

#if 0
#ifdef UDP_TX_DEBUG

        IPERF_PRINTF("Received 0x%x bytes, Packets %d \n",
                     p_tCxt->pktStats.total_bytes,
                     p_tCxt->pktStats.pkts_recvd);
#endif
        if (stat_udp.stat_valid)
        {
            stat_udp.pkts_recvd = p_tCxt->pktStats.pkts_recvd;
            rxreorder_udp_payload_report(&stat_udp);
        }
#endif
        break;

    } /* main loop */

QUIT:
    if (is_test_done)
        iperf_udp_ack_finish(p_tCxt, from, fromlen);

    iperf_result_print(&p_tCxt->pktStats, 0, 0, true);

    if (A_ERROR != p_tCxt->sock_local) {
        closesocket(p_tCxt->sock_local);
        IPERF_PRINTF("INFO: %s: IPERF socket: close:%d\r\n", __func__, p_tCxt->sock_local);
    }

    if (p_tCxt) {
        qurt_thread_sleep(10 * p_tCxt->iperf_stream_id);

        for (int i = 0; i < MAX_STREAM; i++) {
            if (p_tCxt->params.rx_params.port == bench_udp_rx_port_in_use[i]) {
                bench_udp_rx_port_in_use[i] = 0;
                break;
            }
        }

        if (p_tCxt->iperf_stream_id < MAX_STREAM) {
            iperf_stream_id[p_tCxt->iperf_stream_id] = 0;
        }
        p_tCxt->quit = true;
        while (p_tCxt->result_create) {
            qurt_thread_sleep(10);
        }

        if (p_tCxt->buffer) {
            free(p_tCxt->buffer);
            p_tCxt->buffer = NULL;
        }

        free(p_tCxt);
        p_tCxt = NULL;
    }

    IPERF_PRINTF(BENCH_TEST_COMPLETED);
#if IPERF_RX_THREAD
    ret = iperf_remove_rx_thread_based_on_id(nt_qurt_thread_get_id());
    if (ret == -1) {
        IPERF_PRINTF("ERROR: %s: remove_rx_thread error!\r\n", __func__);
    }
    nt_osal_thread_delete(NULL);
#endif

    return;
}

/************************************************************************
 * NAME: iperf_tcp_tx
 *
 * DESCRIPTION: Start TCP Transmit test.
 ************************************************************************/

#define IPERF_SOCKET_RX_TIMEOUT 10
#define IPERF_SOCKET_TX_TIMEOUT 4

void iperf_tcp_tx(THROUGHPUT_CXT *p_tCxt)
{
    if (p_tCxt == NULL) {
        IPERF_PRINTF("ERROR: p_tCxt in iperf_tcp_tx() is NULL\n");
        goto QUIT;
    }

#if LWIP_IPV4
    struct sockaddr_in foreign_addr;
#endif
#if LWIP_IPV6
    struct sockaddr_in6 foreign_addr6;
#endif
    struct sockaddr *to;
    uint32_t tolen;
    char ip_str[48];
    uint32_t i;

    int family;
    int tos_opt;
    int opt = 1;
    err_t ret;
    struct timeval send_timeout = {0};

    memset(ip_str, 0, sizeof(ip_str));
    p_tCxt->sock_peer = A_ERROR;

    for (i = 0; i < INCREMENTAL_PATTERN_SIZE; i++) {
        g_incremental_pattern[i] = i;
    }

    p_tCxt->pktStats.iperf_time_sec = 0;
    p_tCxt->iperf_stream_id += 1;

    if (p_tCxt->params.tx_params.v6) {
#if LWIP_IPV6
        family = AF_INET6;
        inet_ntop(family, &p_tCxt->params.tx_params.v6addr[0], ip_str, sizeof(ip_str));

        memset(&foreign_addr6, 0, sizeof(foreign_addr6));
        memscpy(&foreign_addr6.sin6_addr, sizeof(foreign_addr6.sin6_addr), p_tCxt->params.tx_params.v6addr,
                sizeof(foreign_addr6.sin6_addr));
        foreign_addr6.sin6_port = htons(p_tCxt->params.tx_params.port);
        foreign_addr6.sin6_family = family;
        foreign_addr6.sin6_scope_id = p_tCxt->params.tx_params.scope_id;
        foreign_addr6.sin6_flowinfo = 0;

        to = (struct sockaddr *)&foreign_addr6;
        tolen = sizeof(foreign_addr6);
        tos_opt = IPV6_TCLASS;
#else
        goto QUIT;
#endif
    } else {
#if LWIP_IPV4
        family = AF_INET;
        inet_ntop(family, &p_tCxt->params.tx_params.ip_address, ip_str, sizeof(ip_str));

        memset(&foreign_addr, 0, sizeof(foreign_addr));
        foreign_addr.sin_addr.s_addr = p_tCxt->params.tx_params.ip_address;
        foreign_addr.sin_port = htons(p_tCxt->params.tx_params.port);
        foreign_addr.sin_family = family;

        to = (struct sockaddr *)&foreign_addr;
        tolen = sizeof(foreign_addr);
        tos_opt = IP_TOS;
#else
        goto QUIT;
#endif
    }

    /* Create socket */
    if ((p_tCxt->sock_peer = socket(family, SOCK_STREAM, 0)) == A_ERROR) {
        IPERF_PRINTF("ERROR: iperf_tcp_tx: Unable to create socket\r\n");
        goto QUIT;
    }
    IPERF_PRINTF("INFO: %s: IPERF socket: open:%d\r\n", __func__, p_tCxt->sock_peer);

    if (p_tCxt->tcp_snd_buf > 0) {
        setsockopt(p_tCxt->sock_peer, SOL_SOCKET, SO_SNDBUF, &p_tCxt->tcp_snd_buf, sizeof(int));
    }

    if (p_tCxt->params.tx_params.ip_tos > 0) {
        if (setsockopt(p_tCxt->sock_peer, IPPROTO_IP, tos_opt, &p_tCxt->params.tx_params.ip_tos, sizeof(int)) < 0) {
            goto QUIT;
        }
    }
    /* enable TCP TX socket non-blocking*/
    // if (p_tCxt->params.tx_params.is_so_unblock)
    // {
    //     setsockopt(p_tCxt->sock_peer, SOL_SOCKET, O_NONBLOCK, NULL, 0);
    //     IPERF_PRINTF("non-blocking mode\n");
    // }

    IPERF_PRINTF("------------------------------------------------------------\n");
    IPERF_PRINTF("Client connecting to %s, TCP port %d\n", ip_str, p_tCxt->params.tx_params.port);
    IPERF_PRINTF("------------------------------------------------------------\n");

    /* enable TCP keepalive */
    setsockopt(p_tCxt->sock_peer, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt));

    /* set sending timeout */
    send_timeout.tv_sec = IPERF_SOCKET_TX_TIMEOUT;
    setsockopt(p_tCxt->sock_peer, SOL_SOCKET, SO_SNDTIMEO, &send_timeout, sizeof(send_timeout));

    /* Connect to the server.*/
    IPERF_PRINTF("INFO: iperf_tcp_tx: TCP Connecting\r\n");
    if (connect(p_tCxt->sock_peer, to, tolen) == A_ERROR) {
        IPERF_PRINTF("ERROR: iperf_tcp_tx: Connection failed.\r\n");
        goto QUIT;
    }
    IPERF_PRINTF("INFO: iperf_tcp_tx: TCP Connected\r\n");

    ret = nt_qurt_thread_create(iperf_client_send, "tcp_client", 1024, p_tCxt, IPERF_TX_THREAD_PRIO, NULL);

    if (ret == -1) {
        IPERF_PRINTF("ERROR: iperf_tcp_tx: TCP client task creation failed out of memory\r\n");
        goto QUIT;
    }
    return;

QUIT:
    if (A_ERROR != p_tCxt->sock_peer) {
        closesocket(p_tCxt->sock_peer);
        IPERF_PRINTF("INFO: %s: IPERF socket: close:%d\r\n", __func__, p_tCxt->sock_peer);
    }
    IPERF_PRINTF(BENCH_TEST_COMPLETED);

    if (p_tCxt) {
        free(p_tCxt);
        p_tCxt = NULL;
    }
}

/************************************************************************
 * NAME: iperf_tcp_rx
 *
 * DESCRIPTION: Start throughput TCP server.
 ************************************************************************/
#if IPERF_RX_THREAD
void iperf_tcp_rx(void *arg)
#else
void iperf_tcp_rx(THROUGHPUT_CXT *p_tCxt)
#endif
{
#if IPERF_RX_THREAD
    THROUGHPUT_CXT *p_tCxt = (THROUGHPUT_CXT *)arg;
#endif

    int32_t received = 0;
    int32_t conn_sock = 0, printit = 1;
#if LWIP_IPV4
    struct sockaddr_in local_addr;
    struct sockaddr_in foreign_addr;
#endif
#if LWIP_IPV6
    struct sockaddr_in6 local_addr6;
    struct sockaddr_in6 foreign_addr6;
#endif
    struct sockaddr *addr;
    uint32_t addrlen;
    struct sockaddr *from;
    uint32_t fromlen;
    void *sin_addr;
    void *local_sin_addr = NULL;
    char ip_str[48];
    int ret = 0;

#ifdef TCP_RX_RETRY_AFTER_FIN
    uint32_t retry = 20;
#endif
    int sessionId = 0;
    int family;
    bench_tcp_session_t *session = NULL;
    bench_tcp_session_t *sess = NULL;
    int sock_peer = A_ERROR;
    int maxfd = 0;
    int newSession = 0;
    bench_tcp_server_t *tcp_server = NULL;
    int serverId = -1;
    fd_set rd_set, rset;
    uint32_t iperf_select_start_time = 0;
    struct timeval tv = {0};

    if (p_tCxt == NULL) {
        IPERF_PRINTF("ERROR: p_tCxt in iperf_tcp_tx() is NULL\n");
        goto QUIT;
    }

    (void)local_sin_addr;
    if (serverRefCount == BENCH_TCP_MAX_SERVERS) {
        IPERF_PRINTF("%s: Max num of servers supported is %d\n", __func__, BENCH_TCP_MAX_SERVERS);
#if IPERF_RX_THREAD
        free(p_tCxt);
        p_tCxt = NULL;
        nt_osal_thread_delete(NULL);
#endif
        return;
    }

    if (!serverRefCount) {
        qurt_mutex_create(&serverLock);
        memset(g_tcpServers, 0, sizeof(g_tcpServers));
    }

    serverId = iperf_tcp_getServerId();
    if (serverId == -1) {
        IPERF_PRINTF("ERROR: %s: Invalid ServerId %d\r\n", __func__, serverId);
#if IPERF_RX_THREAD
        free(p_tCxt);
        p_tCxt = NULL;
        nt_osal_thread_delete(NULL);
#endif
        return;
    }

    tcp_server = &g_tcpServers[serverId];
    tcp_server->sockfd = A_ERROR;

    IPERF_PRINTF("TCP Server Id: %d\n", serverId);

    tcp_server->port = p_tCxt->params.rx_params.port;

    memset(&rd_set, 0, sizeof(fd_set));
    p_tCxt->iperf_stream_id = iperf_get_unused_id();
    if (p_tCxt->iperf_stream_id == MAX_STREAM) {
        goto QUIT;
    }
    p_tCxt->pktStats.iperf_stream_id = p_tCxt->iperf_stream_id;

    if (p_tCxt->params.rx_params.v6) {
#if LWIP_IPV6
        family = AF_INET6;

        memset(&local_addr6, 0, sizeof(local_addr6));
        local_addr6.sin6_port = htons(tcp_server->port);
        local_addr6.sin6_family = family;
        memscpy(&local_addr6.sin6_addr, sizeof(local_addr6.sin6_addr), p_tCxt->params.rx_params.local_v6addr,
                sizeof(local_addr6.sin6_addr));
        addr = (struct sockaddr *)&local_addr6;
        local_sin_addr = &local_addr6.sin6_addr;
        addrlen = sizeof(struct sockaddr_in6);

        from = (struct sockaddr *)&foreign_addr6;
        fromlen = sizeof(struct sockaddr_in6);
        sin_addr = &foreign_addr6.sin6_addr;
#else
        goto QUIT;
#endif
    } else {
#if LWIP_IPV4
        family = AF_INET;

        memset(&local_addr, 0, sizeof(local_addr));
        local_addr.sin_port = htons(tcp_server->port);
        local_addr.sin_family = family;
        local_addr.sin_addr.s_addr = p_tCxt->params.rx_params.local_address;
        addr = (struct sockaddr *)&local_addr;
        local_sin_addr = &local_addr.sin_addr;
        addrlen = sizeof(struct sockaddr_in);

        from = (struct sockaddr *)&foreign_addr;
        fromlen = sizeof(struct sockaddr_in);
        sin_addr = &foreign_addr.sin_addr;
#else
        goto QUIT;
#endif
    }

    /* Create listen socket */
    if ((tcp_server->sockfd = socket(family, SOCK_STREAM, 0)) == A_ERROR) {
        IPERF_PRINTF("ERROR: %s: Socket creation error.\r\n", __func__);
        goto QUIT;
    }
    IPERF_PRINTF("INFO: %s: IPERF socket: open:%d, tcp_server->sockfd.\r\n", __func__, tcp_server->sockfd);

    /* Bind socket */
    if (bind(tcp_server->sockfd, addr, addrlen) == A_ERROR) {
        IPERF_PRINTF("ERROR: Socket bind error.\n");
        goto QUIT;
    }

    /* set to non-blocking mode */
    // setsockopt(tcp_server->sockfd, SOL_SOCKET, O_NONBLOCK, NULL, 0);

    /* Configure queue sizes */
    // bench_config_queue_size(tcp_server->sockfd);

    /* Listen */
    if (listen(tcp_server->sockfd, 5) == A_ERROR) {
        IPERF_PRINTF("ERROR: Socket listen error.\n");
        goto QUIT;
    }

    if (printit) {
        memset(ip_str, 0, sizeof(ip_str));

        IPERF_PRINTF("------------------------------------------------------------\n");
        IPERF_PRINTF("Server listening on TCP port %d,interval:%d\n", tcp_server->port,
                     p_tCxt->pktStats.iperf_display_interval);
        IPERF_PRINTF("------------------------------------------------------------\n");

        printit = 0;
    }
    // Restart_tcp_server:

    IPERF_PRINTF("Waiting\n");
#if IPERF_RX_THREAD
    IPERF_PRINTF("TCP RX thread...\n");
#endif
    // memset(&rset, 0, sizeof(fd_set));
    FD_ZERO(&rset);

    do {
        if (iperf_rx_quit || tcp_server->exit || (get_device_connect_state() == false)) {
            goto QUIT;
        }


        FD_SET(tcp_server->sockfd, &rset);
        tv.tv_sec = 10;
        if (select(tcp_server->sockfd + 1, &rset, NULL, NULL, &tv) > 0) {
            /* Accept incoming connection */
            if ((sock_peer = accept(tcp_server->sockfd, from, &fromlen)) == A_ERROR) {
                if (!sessionRefCount) {
                    qurt_thread_sleep(1);
                    continue;
                } else if (!tcpRefCount) {
                    qurt_thread_sleep(1000);
                    continue;
                }
            } else {
                newSession = 1;
                IPERF_PRINTF("Get a new session, socket handle:%d\n", sock_peer);
                IPERF_PRINTF("INFO: %s: IPERF socket: open:%d\r\n", __func__, sock_peer);
                break;
            }
        }
    } while (1);

    /* Create a new TCP session object */
    if (newSession) {
        int opt = 1;
        // struct timeval timeout = { 0 };

        /* set to non-blocking mode */
        // setsockopt(sock_peer, SOL_SOCKET, O_NONBLOCK, NULL, 0);

        /* enable TCP keepalive */
        setsockopt(sock_peer, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt));

        /* set RX timeout */
        // tv.tv_sec = IPERF_SOCKET_RX_TIMEOUT;
        // setsockopt(sock_peer, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
#if 0
            if(p_tCxt->params.rx_params.flow_low_flag){
				/*Update flow control low threshold*/
				opt = p_tCxt->params.rx_params.flow_low;
				setsockopt(sock_peer, SOL_SOCKET, SO_FLOW_LOW_THRESHOLD, &opt, sizeof(opt));
			}

			if(p_tCxt->params.rx_params.flow_high_flag){
				/*Update flow control high threshold*/
				opt = p_tCxt->params.rx_params.flow_high;
				setsockopt(sock_peer, SOL_SOCKET, SO_FLOW_HIGH_THRESHOLD, &opt, sizeof(opt));
			}

			if(p_tCxt->params.rx_params.flow_wht_flag){
				/*Update flow control weight */
				float f_opt = p_tCxt->params.rx_params.flow_wht;
				qapi_setsockopt(sock_peer, SOL_SOCKET, SO_FLOW_WEIGHT, &f_opt, sizeof(f_opt));
			}
#endif

        if ((sessionId = iperf_tcp_CreateSession(p_tCxt)) < 0) {
            IPERF_PRINTF("ERROR: %s: Failed to create TCP RX session object\r\n", __func__);
            closesocket(sock_peer);
            IPERF_PRINTF("INFO: %s: IPERF socket: close:%d\r\n", __func__, sock_peer);
            newSession = 0;
            // continue;
            goto QUIT;
        }

        session = &g_tcpSessions[sessionId];
        session->port = tcp_server->port;
        session->sock_peer = sock_peer;

        /*Allocate buffer*/
        if ((session->buffer = malloc(CFG_PACKET_SIZE_MAX_RX)) == NULL) {
            IPERF_PRINTF("Out of memory error\n");
            iperf_tcp_CloseSession(session, &rd_set);
            newSession = 0;
            // continue;
            goto QUIT;
        }

        // p_tCxt->iperf_stream_id++;
        session->pktStats.iperf_stream_id = p_tCxt->iperf_stream_id;
        session->pktStats.iperf_time_sec = 0;
        session->pktStats.iperf_display_interval = p_tCxt->pktStats.iperf_display_interval;

        session->isFirst = 1;
        FD_SET(session->sock_peer, &rd_set);
        if ((sock_peer + 1) > maxfd)
            maxfd = sock_peer + 1;

        session->ready = 1;
        const char *address_str = inet_ntop(family, sin_addr, ip_str, sizeof(ip_str));
        if (address_str) {
            IPERF_PRINTF("Accepted conn from %s:%d\n", *address_str, ntohs(((struct sockaddr_in *)from)->sin_port));
        } else {
            IPERF_PRINTF("ERR: conn is invalid\n");
        }

        newSession = 0;
        // app_get_time(&session->pktStats.first_time);

        session->pktStats.last_time = session->pktStats.first_time = 0;

        app_get_time(&iperf_select_start_time);

        p_tCxt->session = (void *)session;
        sess = session;
    } else {
        goto QUIT;
    }

    FD_ZERO(&rset);

    do {
        if (iperf_rx_quit || (get_device_connect_state() == false)) {
            app_get_time(&sess->pktStats.last_time);
            goto QUIT;
        }

        tv.tv_sec = 10;
        tv.tv_usec = 0;
        rset = rd_set;
        app_get_time(&iperf_select_start_time);
        conn_sock = select(maxfd, &rset, NULL, NULL, &tv);

        if (conn_sock < 0) {
            IPERF_PRINTF("select error\n");
            app_get_time(&sess->pktStats.last_time);
            goto QUIT;
        } else if (conn_sock == 0) {
            /* No activity. Continue with the next */
            uint32_t curtime;
            app_get_time(&curtime);
            if ((curtime - iperf_select_start_time) / 1000 > 100) {
                IPERF_PRINTF("select timeout %d,%s\n", errno, strerror(errno));
                app_get_time(&sess->pktStats.last_time);
                goto QUIT;
            }
            continue;
        } else if (FD_ISSET(session->sock_peer, &rset)) {
            /*Packet is available, receive it*/
            received = recvfrom(sess->sock_peer, sess->buffer, CFG_PACKET_SIZE_MAX_RX, 0, from, &fromlen);

            /*Valid packet received*/
            if (received > 0) {
                sess->pktStats.bytes += received;
                sess->pktStats.total_bytes += received;

                if (sess->isFirst) {
                    /*This is the first packet, set initial time used to calculate throughput*/
                    app_get_time(&sess->pktStats.first_time);
                    sess->pktStats.last_time = sess->pktStats.first_time;
                    sess->isFirst = 0;

                    sess->iperf_display_last = sess->pktStats.first_time;
                }

#ifdef TCP_RX_DEBUG
                IPERF_PRINTF("%d received= %d total=%ld , %d\n", index, received, sess->pktStats.bytes,
                             p_tCxt->pktStats.total_bytes);
#endif
            } else /* received <= 0 */
            {
#ifdef TCP_RX_DEBUG
                IPERF_PRINTF("@ received= %d total=0x%x bytes\n", received, sess->pktStats.bytes);
#endif
#ifdef TCP_RX_RETRY_AFTER_FIN
                if (retry > 0) {
                    qurt_thread_sleep(1000);
                    --retry;
                    continue;
                } else
#endif
                {
                    app_get_time(&sess->pktStats.last_time);

                    goto QUIT;
                }
            }
            conn_sock--;
        }
    } while (1);

QUIT:
    if (session) {
        iperf_tcp_CloseSession(session, &rd_set);
    }

    if (tcp_server) {
        iperf_tcp_stopServer(tcp_server);
    }

    if (p_tCxt) {
        p_tCxt->quit = true;
        while (p_tCxt->result_create) {
            qurt_thread_sleep(10);
        }
        qurt_thread_sleep(10 * p_tCxt->iperf_stream_id);

        if (p_tCxt->iperf_stream_id < MAX_STREAM) {
            iperf_stream_id[p_tCxt->iperf_stream_id] = 0;
        }

        free(p_tCxt);
        p_tCxt = NULL;
    }

    IPERF_PRINTF(BENCH_TEST_COMPLETED);
#if IPERF_RX_THREAD
    ret = iperf_remove_rx_thread_based_on_id(nt_qurt_thread_get_id());
    if (ret == -1) {
        IPERF_PRINTF("remove_rx_thread error!\r\n");
    }

    nt_osal_thread_delete(NULL);
#endif
    return;
}
#else
qapi_Status_t iperf(uint32_t __attribute__((__unused__)) Parameter_Count,
                    QAPI_Console_Parameter_t __attribute__((__unused__)) * Parameter_List)
{
    printf("This function has been disabled\n");
    return QAPI_OK;
}
qapi_Status_t iperf_quit(uint32_t __attribute__((__unused__)) Parameter_Count,
                         QAPI_Console_Parameter_t __attribute__((__unused__)) * Parameter_List)
{
    printf("This function has been disabled\n");
    return QAPI_OK;
}
#endif
