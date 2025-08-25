/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*-------------------------------------------------------------------------
 * Include Files
 *-----------------------------------------------------------------------*/
#include <stdio.h>
#include <stdarg.h>
#include "string.h"
#include "qat.h"
#include "qat_api.h"
#include "qurt_internal.h"
#include "nt_osal.h"
#include "qurt_mutex.h"
#include <stdbool.h>
#include "qat_tcpip_demo.h"
#include "data_path.h"
#include "dhcp.h"
#include "ip_addr.h"
#include "stdint.h"
#include "icmp6.h"
#include "sockets.h"
#include "ip4.h"
#include "ip.h"
#include "dns.h"
#include "priv/nd6_priv.h"
#include "ip6_addr.h"
#include "netif.h"

/*-------------------------------------------------------------------------
 * Function Declarations
 *-----------------------------------------------------------------------*/
static QAT_Command_Status_t Extend_Command_EnableV6(uint32_t Op_Type, uint32_t Parameter_Count,
                                                    QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_Ping(uint32_t Op_Type, uint32_t Parameter_Count,
                                                QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_DHCPv4c(uint32_t Op_Type, uint32_t Parameter_Count,
                                                   QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_DHCPv4s(uint32_t Op_Type, uint32_t Parameter_Count,
                                                   QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_SetStation(uint32_t Op_Type, uint32_t Parameter_Count,
                                                      QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_Start(uint32_t Op_Type, uint32_t Parameter_Count,
                                                 QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_Close(uint32_t Op_Type, uint32_t Parameter_Count,
                                                 QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_Send(uint32_t Op_Type, uint32_t Parameter_Count,
                                                QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_SendData(uint32_t Op_Type, uint32_t Parameter_Count,
                                                    QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_RecvType(uint32_t Op_Type, uint32_t Parameter_Count,
                                                    QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_RecvData(uint32_t Op_Type, uint32_t Parameter_Count,
                                                    QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_Server(uint32_t Op_Type, uint32_t Parameter_Count,
                                                  QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_UdpServer(uint32_t Op_Type, uint32_t Parameter_Count,
                                                     QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_Mode(uint32_t Op_Type, uint32_t Parameter_Count,
                                                QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_IPV6Prefix(uint32_t Op_Type, uint32_t Parameter_Count,
                                                      QAT_Parameter_t *Parameter_List);

/* The following is the complete command list for the QAT common command demo. */
/** List of global commands that are supported when in a group. */
static QAT_Command_t QAT_TCPIP_Command_List[] = {
    {"+CIPV6", Extend_Command_EnableV6, QAT_OP_EXEC | QAT_OP_QUERY | QAT_OP_EXEC_W_PARAM},
    {"+CIPPING", Extend_Command_Ping, QAT_OP_EXEC | QAT_OP_EXEC_W_PARAM},
    {"+CIPDHCPV4C", Extend_Command_DHCPv4c, QAT_OP_EXEC | QAT_OP_EXEC_W_PARAM},
    {"+CIPDHCPV4S", Extend_Command_DHCPv4s, QAT_OP_EXEC | QAT_OP_EXEC_W_PARAM},
    {"+CIPSTA", Extend_Command_SetStation, QAT_OP_EXEC | QAT_OP_QUERY | QAT_OP_EXEC_W_PARAM},
    {"+CIPSTART", Extend_Command_Start, QAT_OP_EXEC | QAT_OP_QUERY | QAT_OP_EXEC_W_PARAM},
    {"+CIPCLOSE", Extend_Command_Close, QAT_OP_EXEC | QAT_OP_EXEC_W_PARAM},
    {"+CIPSENDDATA", Extend_Command_SendData, QAT_OP_EXEC | QAT_OP_EXEC_W_PARAM},
    {"+CIPSEND", Extend_Command_Send, QAT_OP_EXEC | QAT_OP_EXEC_W_PARAM},
    {"+CIPRECVTYPE", Extend_Command_RecvType, QAT_OP_EXEC | QAT_OP_QUERY | QAT_OP_EXEC_W_PARAM},
    {"+CIPRECVDATA", Extend_Command_RecvData, QAT_OP_EXEC | QAT_OP_EXEC_W_PARAM},
    {"+CIPSERVER", Extend_Command_Server, QAT_OP_EXEC | QAT_OP_QUERY | QAT_OP_EXEC_W_PARAM},
    {"+CIPUDPSERVER", Extend_Command_UdpServer, QAT_OP_EXEC | QAT_OP_QUERY | QAT_OP_EXEC_W_PARAM},
    {"+CIPMODE", Extend_Command_Mode, QAT_OP_EXEC | QAT_OP_QUERY | QAT_OP_EXEC_W_PARAM},
#if LWIP_IPV6
    {"+CIPV6PREFIX", Extend_Command_IPV6Prefix, QAT_OP_EXEC | QAT_OP_QUERY | QAT_OP_EXEC_W_PARAM},
#endif
};

/*-------------------------------------------------------------------------
 * Parameters define
 *-----------------------------------------------------------------------*/
#define TCPIP_COMMAND_LIST_SIZE (sizeof(QAT_TCPIP_Command_List) / sizeof(QAT_Command_t))

#define QAT_PING_DEFAULT_DELAY_MS    500
#define QAT_PING_DEFAULT_COUNT       4
#define QAT_PING_DEFAULT_PACKET_SIZE 32
#define QAT_PING_RCV_TIME            1000
#define QAT_PING_RECV_BUFFER_SIZE    1500
#define QAT_TIME_SEC_TO_MS           1000

#define QAT_DNS_SERVER_INDEX0 0
#define QAT_DNS_SERVER_INDEX1 1

#define QAT_CLIENT_MAX_CONNECTIONS 4
#define INVALID_FD                 -1
#define DATA_MAX_SEND_COUNT        5

#define QAT_CFG_PING_MAX_TX          1470
#define QAT_CFG_PING6_MAX_TX         1450
#define QAT_CMD_IP_BUFFER_LENGTH     512
#define QAT_INPUT_BUFFER_LENGTH      1400
#define QAT_DATA_INPUT_BUFFER_LENGTH 1371
#define TIMEOUT_TV_SEC               1
#define TIMEOUT_TV_USEC              0
#define INVALID_LINKID               -1
#define QAT_IP_PRINTF(...)           printf(__VA_ARGS__)
#define MY_MAX_PORT                  65535
#define MAX_WAIT_TIME                2000

/** ping identifier - must fit on a u16_t */
#ifndef QAT_PING_ID
#define QAT_PING_ID 0xACAB
#endif

#define QAT_OK    0
#define QAT_ERROR -1

#define QAT_IPV6PREFIX_LEN_MAX 19

/**********************************************************************************************************/
/* Globals											                                                      */
/**********************************************************************************************************/
static u16_t qat_ping_seq_num;
static u32_t qat_ping_time;
static u32_t qat_ping_sent_count;
static u32_t qat_ping_recv_count;
static uint32_t ping_count;
static uint32_t ping_delay;
static size_t ping_size;

static uint32_t data_mode_max_len = 0;
static uint32_t data_mode_total_send_len = 0;
static int data_mode_link_id = INVALID_LINKID;
static bool exitLengthValid = true;
uint8_t isPassThroughMode = 0;

static QueueHandle_t client_queue = NULL;
qurt_mutex_t client_mutex;
client_ctx_t g_client_conns_t[QAT_CLIENT_MAX_CONNECTIONS];

server_config tcp_config;
bool tcpServerThreadCreated = false;
server_config udp_config;
bool udpServerThreadCreated = false;

static QueueHandle_t server_queue = NULL;
qurt_mutex_t server_mutex;
server_ctx_t g_listen_clients[QAT_CLIENT_MAX_CONNECTIONS] = {0};
CircularBuffer *server_cb;

static QueueHandle_t udp_server_queue = NULL;
qurt_mutex_t udp_server_mutex;
udp_server_ctx_t g_listen_udp_clients[QAT_CLIENT_MAX_CONNECTIONS] = {0};
CircularBuffer *udp_server_cb;

bool ipd_message_print_flag = true;
bool server_ipd_message_print_flag = true;
bool udp_server_ipd_message_print_flag = true;

static int tcp_listen_fd = INVALID_FD;
static int udp_listen_fd = INVALID_FD;

static uint8_t v6_enable = 1;
extern struct nd6_router_list_entry default_router_list[];
char ipv6_prefix[40] = "2001:db8";  // set the default prefix
/*-------------------------------------------------------------------------
 * Function Definitions
 *-----------------------------------------------------------------------*/

static struct netif *get_netif_by_device(int devid)
{
    uint8_t netid = 0;
    struct netif *netif;

    if (devid <= AP_DEVICE) {
        NETIF_FOREACH(netif)
        {
            if (devid == ((device_t *)netif->state)->role) {
                netid = netif->num + 1; /* found! */
            }
        }
    }
    return netif_get_by_index(netid);
}

static int handle_parsed_data(char *ip, ip_addr_t *ip_addr, bool *is_ipv6)
{
    struct sockaddr_in foreign_addr;
    struct sockaddr_in6 foreign_addr6;

    if (inet_pton(AF_INET6, ip, (char *)&foreign_addr6.sin6_addr) == 1) {
        inet6_addr_to_ip6addr(ip_2_ip6(ip_addr), &foreign_addr6.sin6_addr);
        IP_SET_TYPE_VAL((*ip_addr), IPADDR_TYPE_V6);
        *is_ipv6 = true;
    } else if (inet_pton(AF_INET, ip, (char *)&foreign_addr.sin_addr) == 1) {
        inet_addr_to_ip4addr(ip_2_ip4(ip_addr), &foreign_addr.sin_addr);
        IP_SET_TYPE_VAL((*ip_addr), IPADDR_TYPE_V4);
        *is_ipv6 = false;
    } else {
        QAT_IP_PRINTF("The host is not a valid IP address!\r\n");
        return QAT_ERROR;
    }
    return QAT_OK;
}

void qat_ping_prepare_echo(icmpm_echo_hdr *icmp_hdr, size_t len, bool is_ipv6)
{
    if (is_ipv6) {
        ICMPH_TYPE_SET(icmpm_2_icmp6(icmp_hdr), ICMP6_TYPE_EREQ);
    } else {
        ICMPH_TYPE_SET(icmpm_2_icmp(icmp_hdr), ICMP_ECHO);
    }

    ICMPH_CODE_SET(icmpm_2_icmp(icmp_hdr), 0);
    icmpm_2_icmp(icmp_hdr)->chksum = 0;
    icmpm_2_icmp(icmp_hdr)->id = QAT_PING_ID;
    icmpm_2_icmp(icmp_hdr)->seqno = htons(++qat_ping_seq_num);

    for (int i = sizeof(icmpm_echo_hdr); i < len; i++) {
        ((char *)icmp_hdr)[i] = 0;
    }

    if (!is_ipv6) {
        icmpm_2_icmp(icmp_hdr)->chksum = inet_chksum(icmp_hdr, ping_size);
    }
}

static err_t qat_ping_send(int s, const ip_addr_t *addr)
{
    int err;
    struct sockaddr_storage to;
    icmpm_echo_hdr *icmp_hdr;
    size_t echo_size;
    struct netif *netif = NULL;

    if (get_netif_by_device(AP_DEVICE)) {
        netif = get_netif_by_device(AP_DEVICE);
    } else if (get_netif_by_device(STA_DEVICE)) {
        netif = get_netif_by_device(STA_DEVICE);
    } else {
        QAT_IP_PRINTF("+CIPSTA:network interface not initialized\r\n");
        return ERR_VAL;
    }

    echo_size = ping_size + sizeof(icmpm_echo_hdr);
    icmp_hdr = (icmpm_echo_hdr *)mem_malloc((mem_size_t)echo_size);
    if (!icmp_hdr) {
        return ERR_VAL;
    }

    qat_ping_prepare_echo(icmp_hdr, echo_size, IP_IS_V6(addr));
    if (IP_IS_V4(addr)) {
        struct sockaddr_in *to4 = (struct sockaddr_in *)&to;
        to4->sin_len = sizeof(to4);
        to4->sin_family = AF_INET;
        inet_addr_from_ip4addr(&to4->sin_addr, ip_2_ip4(addr));
    }

    if (IP_IS_V6(addr)) {
        struct sockaddr_in6 *to6 = (struct sockaddr_in6 *)&to;
        to6->sin6_len = sizeof(to6);
        to6->sin6_family = AF_INET6;
        if (netif) {
            to6->sin6_scope_id = netif_get_index(netif);
        }
        inet6_addr_from_ip6addr(&to6->sin6_addr, ip_2_ip6(addr));
    }

    err = sendto(s, icmp_hdr, ping_size, 0, (struct sockaddr *)&to, sizeof(to));
    mem_free(icmp_hdr);
    return (err < 0 ? ERR_VAL : ERR_OK);
}

static void qat_ping_recv(int s, char *buffer, char *buf)
{
    int ret;
    int recv_len;
    struct sockaddr_storage from;
    socklen_t fromlen = sizeof(from);
    ip_addr_t from_addr;
    struct timeval timeout;
    timeout.tv_sec = QAT_PING_RCV_TIME / 1000;
    timeout.tv_usec = 0;
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(s, &fds);
    icmpm_echo_hdr *icmp_header;

    do {
        ret = select(s + 1, &fds, NULL, NULL, &timeout);
        if (ret > 0) {
            recv_len = recvfrom(s, buf, QAT_PING_RECV_BUFFER_SIZE, 0, (struct sockaddr *)&from, (socklen_t *)&fromlen);
            if (recv_len >= (int)(sizeof(struct ip_hdr) + sizeof(struct icmp_echo_hdr))) {
                memset(&from_addr, 0, sizeof(from_addr));
#if LWIP_IPV4
                if (from.ss_family == AF_INET) {
                    struct sockaddr_in *from4 = (struct sockaddr_in *)&from;
                    inet_addr_to_ip4addr(ip_2_ip4(&from_addr), &from4->sin_addr);
                    IP_SET_TYPE_VAL(from_addr, IPADDR_TYPE_V4);
                }
#endif /* LWIP_IPV4 */
#if LWIP_IPV6
                if (from.ss_family == AF_INET6) {
                    struct sockaddr_in6 *from6 = (struct sockaddr_in6 *)&from;
                    inet6_addr_to_ip6addr(ip_2_ip6(&from_addr), &from6->sin6_addr);
                    IP_SET_TYPE_VAL(from_addr, IPADDR_TYPE_V6);
                }
#endif /* LWIP_IPV6 */

                if (IP_IS_V4_VAL(from_addr)) {
                    struct ip_hdr *ip_header = (struct ip_hdr *)buf;
                    icmp_header = (icmpm_echo_hdr *)(buf + (IPH_HL(ip_header) * 4));
                } else if (IP_IS_V6_VAL(from_addr)) {
                    icmp_header = (icmpm_echo_hdr *)(buf + sizeof(struct ip6_hdr));
                }

                if ((icmpm_2_icmp(icmp_header)->type != ICMP_ER) &&
                    (icmpm_2_icmp6(icmp_header)->type != ICMP6_TYPE_EREP)) {
                    continue;
                }

                if ((icmpm_2_icmp(icmp_header)->id == QAT_PING_ID) &&
                    (icmpm_2_icmp(icmp_header)->seqno == htons(qat_ping_seq_num))) {
                    qat_ping_recv_count++;
                    memset((void *)buffer, 0, QAT_CMD_IP_BUFFER_LENGTH);
                    snprintf(buffer, QAT_CMD_IP_BUFFER_LENGTH, "+CIPPING:%s,%u,%lu\r\n", ipaddr_ntoa(&from_addr),
                             ntohs(icmpm_2_icmp(icmp_header)->seqno), (sys_now() - qat_ping_time));
                    QAT_Response_Str(QAT_RC_QUIET, buffer);
                    return;
                }
            }
        }
        memset((void *)buffer, 0, QAT_CMD_IP_BUFFER_LENGTH);
        snprintf(buffer, QAT_CMD_IP_BUFFER_LENGTH, "+CIPPING:Request timed out!\r");
        QAT_Response_Str(QAT_RC_QUIET, buffer);
        return;
    } while (1);
}

/*-------------------------------------------------------------------------
 * Function Definitions
 *-----------------------------------------------------------------------*/

static int qat_ping_process(int s, const ip_addr_t *addr)
{
    int ret = QAT_ERROR;
    char buffer[QAT_CMD_IP_BUFFER_LENGTH] = {0};
    char recv_buf[QAT_PING_RECV_BUFFER_SIZE] = {0};

    for (int i = 0; i < ping_count; i++) {
        if (qat_ping_send(s, addr) == ERR_OK) {
            qat_ping_time = sys_now();
            qat_ping_sent_count++;
            qat_ping_recv(s, buffer, recv_buf);
            sys_msleep(ping_delay);
        } else {
            memset((void *)buffer, 0, QAT_CMD_IP_BUFFER_LENGTH);
            snprintf(buffer, QAT_CMD_IP_BUFFER_LENGTH, "+CIPPING:ping send %s - error \r", ipaddr_ntoa(addr));
            QAT_Response_Str(QAT_RC_QUIET, buffer);
            sys_msleep(500);
        }
    }

    memset((void *)buffer, 0, QAT_CMD_IP_BUFFER_LENGTH);
    snprintf(buffer, QAT_CMD_IP_BUFFER_LENGTH, "+CIPPING:%d,%d\r\n", qat_ping_sent_count, qat_ping_recv_count);
    QAT_Response_Str(QAT_RC_QUIET, buffer);
    if (qat_ping_recv_count > 0) {
        ret = QAT_OK;
    }
    return ret;
}

/**
   @brief Processes the Extend command from the QAT.

   This command will change the current group to its parent. No parameters are
   expected for this command.

   @param[in] Op_Type          The input command type.
   @param[in] Parameter_Count  Number of parameters that were entered into the
                               command line.
   @param[in] Parameter_List   List of parameters entered into the command line.
*/
static QAT_Command_Status_t Extend_Command_Ping(uint32_t Op_Type, uint32_t Parameter_Count,
                                                QAT_Parameter_t *Parameter_List)
{
    char *ptr = NULL;
    int s;
    bool is_ipv6 = false;
    ip_addr_t ip_addr;
    QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
    qat_ping_seq_num = 0;
    qat_ping_sent_count = 0;
    qat_ping_recv_count = 0;
    char buf[QAT_CMD_IP_BUFFER_LENGTH] = {0};
    char ip[INET6_ADDRSTRLEN];

    switch (Op_Type) {
        case QAT_OP_EXEC: {
            QAT_Response_Str(QAT_RC_OK, "+CIPPING=<host>[,<count>[,<delay>[,<package size>]]]\r\n");
            break;
        }
        case QAT_OP_EXEC_W_PARAM: {
            if (Parameter_Count > 4 || Parameter_Count < 1 || !Parameter_List) {
                QAT_Response_Str(QAT_RC_ERROR, "+CIPPING:Invalid input parameter!\r\n");
                return rc;
            }

            ping_count = QAT_PING_DEFAULT_COUNT;
            ping_delay = QAT_PING_DEFAULT_DELAY_MS;
            ping_size = QAT_PING_DEFAULT_PACKET_SIZE;

            if (Parameter_List[0].Integer_Is_Valid) {
                QAT_Response_Str(QAT_RC_ERROR, "+CIPPING:The host parameter is not valid!\r\n");
                return rc;
            }

            ptr = Parameter_List[0].String_Value;
            memset(&ip_addr, 0, sizeof(ip_addr));
            if (handle_parsed_data(ptr, &ip_addr, &is_ipv6) == QAT_ERROR) {
                QAT_Response_Str(QAT_RC_ERROR, "+CIPPING:IP address is not valid!\r\n");
                return rc;
            }

            if (Parameter_Count >= 2) {
                if (!Parameter_List[1].Integer_Is_Valid) {
                    QAT_Response_Str(QAT_RC_ERROR, "+CIPPING:the type of count must be a Integer!\r\n");
                    return rc;
                } else {
                    ping_count = Parameter_List[1].Integer_Value;
                }
            }

            if (Parameter_Count >= 3) {
                if (!Parameter_List[2].Integer_Is_Valid) {
                    QAT_Response_Str(QAT_RC_ERROR, "+CIPPING:the type of delay must be a Integer!\r\n");
                    return rc;
                } else {
                    ping_delay = Parameter_List[2].Integer_Value;
                }
            }

            if (Parameter_Count == 4) {
                if (!Parameter_List[3].Integer_Is_Valid) {
                    QAT_Response_Str(QAT_RC_ERROR, "+CIPPING:the type of package size must be a Integer!\r\n");
                    return rc;
                } else {
                    ping_size = Parameter_List[3].Integer_Value;
                }
            }

            if (is_ipv6 && (ping_size > QAT_CFG_PING6_MAX_TX)) {
                snprintf(buf, QAT_CMD_IP_BUFFER_LENGTH, "IPv6 Size should be <= %d\r\n", QAT_CFG_PING6_MAX_TX);
                QAT_Response_Str(QAT_RC_ERROR, buf);
                return rc;
            }
            if (!is_ipv6 && (ping_size > QAT_CFG_PING_MAX_TX)) {
                snprintf(buf, QAT_CMD_IP_BUFFER_LENGTH, "IPv4 Size should be <= %d\r\n", QAT_CFG_PING_MAX_TX);
                QAT_Response_Str(QAT_RC_ERROR, buf);
                return rc;
            }

            if (!is_ipv6) {
                s = socket(AF_INET, SOCK_RAW, IP_PROTO_ICMP);
            } else {
                s = socket(AF_INET6, SOCK_RAW, IP6_NEXTH_ICMP6);
            }

            if (s < 0) {
                QAT_Response_Str(QAT_RC_ERROR, NULL);
                return rc;
            }

            if (qat_ping_process(s, &ip_addr) == QAT_OK) {
                rc = QAT_Response_Str(QAT_RC_OK, NULL);
            } else {
                QAT_Response_Str(QAT_RC_ERROR, NULL);
            }
            closesocket(s);
            break;
        }
    }
    return rc;
}

static void qat_net_show_info(struct netif *netif, char *buffer, int *p_offset)
{
    ip_addr_t *ip_addr = (ip_addr_t *)netif_ip_addr4(netif);
    ip_addr_t *netmask = (ip_addr_t *)netif_ip_netmask4(netif);
    ip_addr_t *gw = (ip_addr_t *)netif_ip_gw4(netif);
    ip_addr_t *dns1 = (ip_addr_t *)dns_getserver(QAT_DNS_SERVER_INDEX0);
    ip_addr_t *dns2 = (ip_addr_t *)dns_getserver(QAT_DNS_SERVER_INDEX1);

    *p_offset += snprintf(buffer + *p_offset, QAT_CMD_IP_BUFFER_LENGTH, "%s,", ipaddr_ntoa(ip_addr));
    *p_offset += snprintf(buffer + *p_offset, QAT_CMD_IP_BUFFER_LENGTH, "%s,", ipaddr_ntoa(gw));
    *p_offset += snprintf(buffer + *p_offset, QAT_CMD_IP_BUFFER_LENGTH, "%s,", ipaddr_ntoa(netmask));

    if (IP_GET_TYPE(dns1) == IPADDR_TYPE_V6) {
        dns1 = IP4_ADDR_ANY;
    }
    if (IP_GET_TYPE(dns2) == IPADDR_TYPE_V6) {
        dns2 = IP4_ADDR_ANY;
    }

    *p_offset += snprintf(buffer + *p_offset, QAT_CMD_IP_BUFFER_LENGTH, "%s,", ipaddr_ntoa(dns1));
    *p_offset += snprintf(buffer + *p_offset, QAT_CMD_IP_BUFFER_LENGTH, "%s", ipaddr_ntoa(dns2));
}

static bool isDhcpSucceed(struct netif *netif)
{
    struct dhcp *dhcp = NULL;

    int dhcp_time_started = xTaskGetTickCount();
    bool is_bounded = FALSE;

    dhcp = netif_dhcp_data(netif);
    while (xTaskGetTickCount() - dhcp_time_started < MAX_WAIT_TIME) {
        if (dhcp->state == DHCP_STATE_BOUND) {
            is_bounded = TRUE;
            break;
        }
        qurt_thread_sleep(200);
    }

    return is_bounded;
}

static bool isDhcpReleased(struct netif *netif)
{
    struct dhcp *dhcp = NULL;

    dhcp = netif_dhcp_data(netif);
    while (dhcp->state != DHCP_STATE_OFF) {
        qurt_thread_sleep(200);
    }
    return TRUE;
}

/**
   @brief Processes the Extend command from the QAT.

   This command will change the current group to its parent. No parameters are
   expected for this command.

   @param[in] Op_Type          The input command type.
   @param[in] Parameter_Count  Number of parameters that were entered into the
                               command line.
   @param[in] Parameter_List   List of parameters entered into the command line.
*/
static QAT_Command_Status_t Extend_Command_DHCPv4c(uint32_t Op_Type, uint32_t Parameter_Count,
                                                   QAT_Parameter_t *Parameter_List)
{
    struct netif *netif = NULL;
    uint8_t netid;
    QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
    char *interface_name = NULL;
    char *action = NULL;
    uint8_t state = DHCP_TURN_OFF;
    bool status = FALSE;
    int offset = 0;
    char buffer[QAT_CMD_IP_BUFFER_LENGTH] = {0};
    ip_addr_t default_dns = IPADDR4_INIT_BYTES(8, 8, 8, 8);

    switch (Op_Type) {
        case QAT_OP_EXEC: {
            QAT_Response_Str(QAT_RC_OK, "+CIPDHCPV4C=<interface>,<new|release>\r\n");
            break;
        }
        case QAT_OP_EXEC_W_PARAM: {
            if ((Parameter_Count != 2) || !Parameter_List || Parameter_List[0].Integer_Is_Valid ||
                Parameter_List[1].Integer_Is_Valid) {
                QAT_Response_Str(QAT_RC_ERROR, "+CIPDHCPV4C:Invalid input parameter!\r\n");
                return rc;
            }

            netif = get_netif_by_device(STA_DEVICE);
            if (netif == NULL) {
                QAT_Response_Str(QAT_RC_ERROR, NULL);
                return rc;
            }

            interface_name = Parameter_List[0].String_Value;
            if (strcmp(interface_name, "wlan1") != 0) {
                QAT_Response_Str(QAT_RC_ERROR, "+CIPDHCPV4C:Just wlan1 support DHCP client mode currently\r\n");
                return rc;
            }

            action = Parameter_List[1].String_Value;
            if ((!memcmp(action, "new", 3))) {
                netif_set_addr(netif, IP4_ADDR_ANY4, IP4_ADDR_ANY4, IP4_ADDR_ANY4);
                etharp_cleanup_netif(netif);
                status = dhcp_start(netif);
                if (status != ERR_OK) {
                    QAT_Response_Str(QAT_RC_ERROR, "+CIPDHCPV4C:DHCP client start failed\n");
                    return rc;
                }
                if (isDhcpSucceed(netif)) {
                    offset += snprintf(buffer + offset, QAT_CMD_IP_BUFFER_LENGTH, "+CIPDHCPV4C:");
                    qat_net_show_info(netif, buffer, &offset);
                    rc = QAT_Response_Str(QAT_RC_OK, buffer);
                } else {
                    rc = QAT_Response_Str(QAT_RC_OK, "+CIPDHCPV4C:DHCP client start success\r\n");
                }
            } else if (!memcmp(action, "release", 7)) {
                status = dhcp_release(netif);
                if (status != ERR_OK) {
                    QAT_Response_Str(QAT_RC_ERROR, "+CIPDHCPV4C:DHCP client release failed\n");
                    return rc;
                }
                dhcp_stop(netif);
                netif_set_addr(netif, IP4_ADDR_ANY4, IP4_ADDR_ANY4, IP4_ADDR_ANY4);
                dns_setserver(QAT_DNS_SERVER_INDEX0, &default_dns);
                dns_setserver(QAT_DNS_SERVER_INDEX1, &default_dns);
                if (isDhcpReleased(netif)) {
                    offset += snprintf(buffer + offset, QAT_CMD_IP_BUFFER_LENGTH, "+CIPDHCPV4C:");
                    qat_net_show_info(netif, buffer, &offset);
                }
                rc = QAT_Response_Str(QAT_RC_OK, buffer);
            } else {
                QAT_Response_Str(QAT_RC_ERROR,
                                 "+CIPDHCPV4C:DHCP Command Failed due to invalid option i.e. start/release.\r\n\r\n");
                return rc;
            }
            break;
        }
    }
    return rc;
}

/*-------------------------------------------------------------------------
 * Function Definitions
 *-----------------------------------------------------------------------*/

/**
   @brief Processes the Extend command from the QAT.

   This command will change the current group to its parent. No parameters are
   expected for this command.

   @param[in] Op_Type          The input command type.
   @param[in] Parameter_Count  Number of parameters that were entered into the
                               command line.
   @param[in] Parameter_List   List of parameters entered into the command line.
*/
static QAT_Command_Status_t Extend_Command_SetStation(uint32_t Op_Type, uint32_t Parameter_Count,
                                                      QAT_Parameter_t *Parameter_List)
{
    ip_addr_t ip_addr, netmask, gw, dns1, dns2;
    struct netif *netif = NULL;
    QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
    char buffer[QAT_CMD_IP_BUFFER_LENGTH] = {0};
    int offset;
    switch (Op_Type) {
        case QAT_OP_EXEC: {
            QAT_Response_Str(QAT_RC_OK, "+CIPSTA=<ip address>,<gw>,<netmask>,<dns1>,<dns2>\r\n");
            break;
        }
        case QAT_OP_QUERY: {
            offset = 0;
            memset((void *)buffer, 0, QAT_CMD_IP_BUFFER_LENGTH);
            offset += snprintf(buffer + offset, QAT_CMD_IP_BUFFER_LENGTH, "+CIPSTA:");
            NETIF_FOREACH(netif)
            {
                qat_net_show_info(netif, buffer, &offset);
                if (netif->next != NULL) {
                    offset += snprintf(buffer + offset, QAT_CMD_IP_BUFFER_LENGTH, ",");
                }
            }

            NETIF_FOREACH(netif)
            {
                for (int i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++) {
                    if (!ip6_addr_isvalid(netif_ip6_addr_state(netif, i))) {
                        continue;
                    }
                    ip_addr_t *ip6_addr = (ip_addr_t *)(&netif->ip6_addr[i]);
                    if (ip6_addr_islinklocal(ip_2_ip6(ip6_addr))) {
                        offset += snprintf(
                            buffer + offset, QAT_CMD_IP_BUFFER_LENGTH, ",%s/64",
                            ipaddr_ntoa(ip6_addr));  // SLAAC only supports scenarios with a prefix length of 64
                    } else if (ip6_addr_isglobal(ip_2_ip6(ip6_addr))) {
                        offset += snprintf(buffer + offset, QAT_CMD_IP_BUFFER_LENGTH, ",%s/64", ipaddr_ntoa(ip6_addr));
                    }
                }
            }

            if (v6_enable) {
                ip_addr_t *dns1 = (ip_addr_t *)dns_getserver(QAT_DNS_SERVER_INDEX0);
                ip_addr_t *dns2 = (ip_addr_t *)dns_getserver(QAT_DNS_SERVER_INDEX1);
                if (IP_GET_TYPE(dns1) == IPADDR_TYPE_V4) {
                    dns1 = IP4_ADDR_ANY;
                }
                if (IP_GET_TYPE(dns2) == IPADDR_TYPE_V4) {
                    dns2 = IP4_ADDR_ANY;
                }
                if ((dns1 != IP4_ADDR_ANY) || (dns2 != IP4_ADDR_ANY)) {
                    offset += snprintf(buffer + offset, QAT_CMD_IP_BUFFER_LENGTH, ",%s/64", ipaddr_ntoa(dns1));
                    offset += snprintf(buffer + offset, QAT_CMD_IP_BUFFER_LENGTH, ",%s/64", ipaddr_ntoa(dns2));
                } else {
                    /* For now, use the IPv6 default router address as the DNS server address.*/
                    if (default_router_list[0].neighbor_entry != NULL) {
                        char addr_str[INET6_ADDRSTRLEN];
                        ip6addr_ntoa_r(&default_router_list[0].neighbor_entry->next_hop_address, addr_str,
                                       sizeof(addr_str));
                        offset += snprintf(buffer + offset, QAT_CMD_IP_BUFFER_LENGTH, ",%s/64", addr_str);
                    }
                    if (default_router_list[1].neighbor_entry != NULL) {
                        char addr_str[INET6_ADDRSTRLEN];
                        ip6addr_ntoa_r(&default_router_list[1].neighbor_entry->next_hop_address, addr_str,
                                       sizeof(addr_str));
                        offset += snprintf(buffer + offset, QAT_CMD_IP_BUFFER_LENGTH, ",%s/64", addr_str);
                    } else {
                        char addr_str[INET6_ADDRSTRLEN];
                        ip6addr_ntoa_r(&default_router_list[0].neighbor_entry->next_hop_address, addr_str,
                                       sizeof(addr_str));
                        offset += snprintf(buffer + offset, QAT_CMD_IP_BUFFER_LENGTH, ",%s/64", addr_str);
                    }
                }
            }

            rc = QAT_Response_Str(QAT_RC_OK, buffer);
            break;
        }

        case QAT_OP_EXEC_W_PARAM: {
            offset = 0;
            memset((void *)buffer, 0, QAT_CMD_IP_BUFFER_LENGTH);
            if ((Parameter_Count != 5) && (Parameter_Count != 3)) {
                QAT_IP_PRINTF("+CIPSTA:Invalid input parameter!\r\n");
                goto fail;
            }

            else if (Parameter_Count == 5) {
                if (!Parameter_List || Parameter_List[0].Integer_Is_Valid || Parameter_List[1].Integer_Is_Valid ||
                    Parameter_List[2].Integer_Is_Valid || Parameter_List[3].Integer_Is_Valid ||
                    Parameter_List[4].Integer_Is_Valid) {
                    QAT_IP_PRINTF("+CIPSTA:Invalid input parameter!\r\n");
                    goto fail;
                }

                if ((!ipaddr_aton(Parameter_List[0].String_Value, &ip_addr)) ||
                    (!ipaddr_aton(Parameter_List[1].String_Value, &gw) ||
                     (!ipaddr_aton(Parameter_List[2].String_Value, &netmask)) ||
                     (!ip_addr_netmask_valid(ip_2_ip4(&netmask)))) ||
                    (!ipaddr_aton(Parameter_List[3].String_Value, &dns1)) ||
                    (!ipaddr_aton(Parameter_List[4].String_Value, &dns2))) {
                    QAT_IP_PRINTF("+CIPSTA:Invalid input parameter!\r\n");
                    goto fail;
                }

                if (get_netif_by_device(AP_DEVICE)) {
                    netif = get_netif_by_device(AP_DEVICE);
                } else if (get_netif_by_device(STA_DEVICE)) {
                    netif = get_netif_by_device(STA_DEVICE);
                } else {
                    QAT_IP_PRINTF("+CIPSTA:network interface not initialized\r\n");
                    goto fail;
                }

                netif_set_ipaddr(netif, (const ip4_addr_t *)ip_2_ip4(&ip_addr));
                netif_set_netmask(netif, (const ip4_addr_t *)ip_2_ip4(&netmask));
                netif_set_gw(netif, (const ip4_addr_t *)ip_2_ip4(&gw));
                dns_setserver(QAT_DNS_SERVER_INDEX0, &dns1);
                dns_setserver(QAT_DNS_SERVER_INDEX1, &dns2);
            }

            else if (Parameter_Count == 3) {
                if (!Parameter_List || Parameter_List[0].Integer_Is_Valid || Parameter_List[1].Integer_Is_Valid ||
                    Parameter_List[2].Integer_Is_Valid) {
                    QAT_IP_PRINTF("+CIPSTA:Invalid input parameter!\r\n");
                    goto fail;
                }

                if ((!ipaddr_aton(Parameter_List[0].String_Value, &ip_addr)) ||
                    (!ipaddr_aton(Parameter_List[1].String_Value, &gw) ||
                     (!ipaddr_aton(Parameter_List[2].String_Value, &netmask)) ||
                     (!ip_addr_netmask_valid(ip_2_ip4(&netmask))))) {
                    QAT_IP_PRINTF("+CIPSTA:Invalid input parameter!\r\n");
                    goto fail;
                }

                if (get_netif_by_device(AP_DEVICE)) {
                    netif = get_netif_by_device(AP_DEVICE);
                } else if (get_netif_by_device(STA_DEVICE)) {
                    netif = get_netif_by_device(STA_DEVICE);
                } else {
                    QAT_IP_PRINTF("+CIPSTA:network interface not initialized\r\n");
                    goto fail;
                }

                netif_set_ipaddr(netif, (const ip4_addr_t *)ip_2_ip4(&ip_addr));
                netif_set_netmask(netif, (const ip4_addr_t *)ip_2_ip4(&netmask));
                netif_set_gw(netif, (const ip4_addr_t *)ip_2_ip4(&gw));
            }

            offset += snprintf(buffer + offset, QAT_CMD_IP_BUFFER_LENGTH, "+CIPSTA:");
            qat_net_show_info(netif, buffer, &offset);
            rc = QAT_Response_Str(QAT_RC_OK, buffer);
            break;
        }
    }
    return rc;
fail:
    QAT_Response_Str(QAT_RC_ERROR, NULL);
    return rc;
}

/*-------------------------------------------------------------------------
 * Function Definitions
 *-----------------------------------------------------------------------*/

CircularBuffer *CircularBuffer_Create()
{
    CircularBuffer *cb = malloc(sizeof(CircularBuffer));
    if (cb == NULL) {
        printf("Failed to allocate memory for CircularBuffer.\n");
        return NULL;
    }
    memset(cb, 0, sizeof(CircularBuffer));
    cb->head = 0;
    cb->tail = 0;
    cb->size = 0;
    qurt_mutex_create(&cb->mutex);
    return cb;
}

void CircularBuffer_Destroy(CircularBuffer *cb)
{
    if (cb != NULL) {
        qurt_mutex_delete(&cb->mutex);
        free(cb);
    }
}

void CircularBuffer_Write(CircularBuffer *cb, const char *data, size_t length)
{
    qurt_mutex_lock(&cb->mutex);
    for (size_t i = 0; i < length; i++) {
        cb->buffer[cb->head] = data[i];
        cb->head = (cb->head + 1) % QAT_CIRCULAR_BUFFER_SIZE;
    }
    cb->size += length;
    qurt_mutex_unlock(&cb->mutex);
    return;
}

int CircularBuffer_Read(CircularBuffer *cb, char *data, size_t length)
{
    qurt_mutex_lock(&cb->mutex);
    if (cb->size < length) {
        qurt_mutex_unlock(&cb->mutex);
        return QAT_ERROR;
    }

    for (size_t i = 0; i < length; i++) {
        data[i] = cb->buffer[cb->tail];
        cb->tail = (cb->tail + 1) % QAT_CIRCULAR_BUFFER_SIZE;
    }
    cb->size -= length;
    qurt_mutex_unlock(&cb->mutex);
    return QAT_OK;
}

int CircularBuffer_GetFreeSpace(CircularBuffer *cb)
{
    qurt_mutex_lock(&cb->mutex);
    int free_space = QAT_CIRCULAR_BUFFER_SIZE - cb->size;
    qurt_mutex_unlock(&cb->mutex);
    return free_space;
}

static void CleanupClientConnInfo(int link_id)
{
    g_client_conns_t[link_id].id = INVALID_LINKID;
    g_client_conns_t[link_id].sockfd = INVALID_FD;
    g_client_conns_t[link_id].protocol_type = PROTOCOL_INVALID;
    g_client_conns_t[link_id].active = INACTIVE;
    g_client_conns_t[link_id].recv_type = RECVTYPE_ACTIVE;
    g_client_conns_t[link_id].thread_quit = false;
    memset(&(g_client_conns_t[link_id].addr), 0, sizeof(struct sockaddr_in));
    memset(&(g_client_conns_t[link_id].addr6), 0, sizeof(struct sockaddr_in6));
    g_client_conns_t[link_id].cb = NULL;
}

/*-------------------------------------------------------------------------
 * Function Definitions
 *-----------------------------------------------------------------------*/

static int CreateConnection(int link_id, int protocol_type, ip_addr_t *ip_addr, int port)
{
    int sockfd;
    int tos_opt;
    int opt = 1;
    struct sockaddr_storage to;
    struct sockaddr_storage from;
    struct sockaddr_in6 *to6;
    struct sockaddr_in *to4;
    bool is_ipv6 = false;
    struct netif *netif = NULL;

    if (get_netif_by_device(STA_DEVICE))
        netif = get_netif_by_device(STA_DEVICE);
    else if (get_netif_by_device(AP_DEVICE))
        netif = get_netif_by_device(AP_DEVICE);
    else {
        QAT_IP_PRINTF("+CIPSTART:network interface not initialized\r\n");
        return QAT_ERROR;
    }

    if (IP_IS_V4(ip_addr)) {
        to4 = (struct sockaddr_in *)&to;
        to4->sin_len = sizeof(to4);
        to4->sin_family = AF_INET;
        to4->sin_port = htons(port);
        inet_addr_from_ip4addr(&to4->sin_addr, ip_2_ip4(ip_addr));

        ip_addr_t *ip_local_addr = (ip_addr_t *)netif_ip_addr4(netif);
        struct sockaddr_in *from4 = (struct sockaddr_in *)&from;
        from4->sin_len = sizeof(struct sockaddr_in);
        from4->sin_family = AF_INET;
        from4->sin_port = htons(0);
        inet_addr_from_ip4addr(&(from4->sin_addr), ip_2_ip4(ip_local_addr));
        tos_opt = IP_TOS;
    }

    if (IP_IS_V6(ip_addr)) {
        to6 = (struct sockaddr_in6 *)&to;
        to6->sin6_len = sizeof(struct sockaddr_in6);
        to6->sin6_family = AF_INET6;
        to6->sin6_port = htons(port);
        if (netif) {
            to6->sin6_scope_id = netif_get_index(netif);
        }
        inet6_addr_from_ip6addr(&(to6->sin6_addr), ip_2_ip6(ip_addr));

        ip_addr_t *ip6_local_addr = NULL;
        if (ip6_addr_islinklocal(ip_2_ip6(ip_addr))) {
            ip6_local_addr = (ip_addr_t *)netif_ip_addr6(netif, 0);  // link local address
        } else {
            ip6_local_addr = (ip_addr_t *)netif_ip_addr6(netif, 2);  // global address
        }

        struct sockaddr_in6 *from6 = (struct sockaddr_in6 *)&from;
        from6->sin6_len = sizeof(from6);
        from6->sin6_family = AF_INET6;
        from6->sin6_port = htons(0);
        inet6_addr_from_ip6addr(&(from6->sin6_addr), ip_2_ip6(ip6_local_addr));
        is_ipv6 = true;
    }

    switch (protocol_type) {
        case PROTOCOL_TCP:
        case PROTOCOL_TCPv6: {
            sockfd = socket(is_ipv6 ? AF_INET6 : AF_INET, SOCK_STREAM, 0);
            if (sockfd < 0) {
                QAT_IP_PRINTF("create socket failed\n");
                return QAT_ERROR;
            }
            break;
        }
        case PROTOCOL_UDP:
        case PROTOCOL_UDPv6: {
            sockfd = socket(is_ipv6 ? AF_INET6 : AF_INET, SOCK_DGRAM, 0);
            if (sockfd < 0) {
                QAT_IP_PRINTF("Failed to create UDP socket\n");
                return QAT_ERROR;
            }
            break;
        }
        default: {
        }
    }

    // if(is_ipv6){
    //     if (setsockopt(sockfd, IPPROTO_IPV6, IPV6_V6ONLY, &opt, sizeof(opt)) < 0) {
    //         printf("setsockopt failed: %s\n", strerror(errno));
    //         closesocket(sockfd);
    //         return QAT_ERROR;
    //     }
    // }

    if (!is_ipv6) {
        if (setsockopt(sockfd, IPPROTO_IP, tos_opt, &tos_opt, sizeof(int)) < 0) {
            printf("setsockopt failed: %s\n", strerror(errno));
            closesocket(sockfd);
            return QAT_ERROR;
        }
    }

    if (bind(sockfd, (struct sockaddr *)&from, sizeof(from)) < 0) {
        QAT_IP_PRINTF("Failed to bind local addr and port\n");
        closesocket(sockfd);
        return QAT_ERROR;
    }

    if (connect(sockfd, (struct sockaddr *)&to, sizeof(to)) < 0) {
        QAT_IP_PRINTF("Failed to connect TCP server\n");
        closesocket(sockfd);
        return QAT_ERROR;
    }

    g_client_conns_t[link_id].id = link_id;
    g_client_conns_t[link_id].sockfd = sockfd;
    g_client_conns_t[link_id].protocol_type = protocol_type;
    g_client_conns_t[link_id].active = ACTIVE;
    g_client_conns_t[link_id].cb = CircularBuffer_Create();
    if (is_ipv6) {
        g_client_conns_t[link_id].addr6.sin6_len = to6->sin6_len;
        g_client_conns_t[link_id].addr6.sin6_family = to6->sin6_family;
        g_client_conns_t[link_id].addr6.sin6_port = to6->sin6_port;
        g_client_conns_t[link_id].addr6.sin6_scope_id = to6->sin6_scope_id;
        memcpy(&g_client_conns_t[link_id].addr6.sin6_addr, &to6->sin6_addr, sizeof(struct in6_addr));
    } else {
        g_client_conns_t[link_id].addr.sin_len = to4->sin_len;
        g_client_conns_t[link_id].addr.sin_family = to4->sin_family;
        g_client_conns_t[link_id].addr.sin_port = to4->sin_port;
        g_client_conns_t[link_id].addr.sin_addr.s_addr = to4->sin_addr.s_addr;
    }
    return QAT_OK;
}

void cleanGlobalQueue(QueueHandle_t globalQueue, int idx)
{
    QueueHandle_t tempQueue = xQueueCreate(CONFIG_QAT_CB_QUEUE_MAX_LENGTH, sizeof(QueueElem));
    QueueElem elem;
    while (xQueueReceive(globalQueue, &elem, 0) == pdPASS) {
        if (elem.id != idx) {
            xQueueSend(tempQueue, &elem, 0);
        }
    }

    while (xQueueReceive(tempQueue, &elem, 0) == pdPASS) {
        xQueueSend(globalQueue, &elem, 0);
    }
    vQueueDelete(tempQueue);
}

/*-------------------------------------------------------------------------
 * Function Definitions
 *-----------------------------------------------------------------------*/

static void client_recv_thread(void *arg)
{
    int *p_id = (int *)arg;
    int max_fd;
    int client_fd;
    int recv_type, data_len;
    int ret;
    int protocol_type;
    char *protocol_name;
    fd_set readfds;
    char input_buf[QAT_DATA_INPUT_BUFFER_LENGTH] = {0};
    char buffer[QAT_INPUT_BUFFER_LENGTH] = {0};
    int recv_bytes, bytes_available;
    struct timeval timeout;
    int result;
    QueueElem elem;
    int offset;

    client_fd = g_client_conns_t[*p_id].sockfd;
    protocol_type = g_client_conns_t[*p_id].protocol_type;
    switch (protocol_type) {
        case PROTOCOL_TCP:
        case PROTOCOL_TCPv6: {
            protocol_name = "TCP";
            break;
        }
        case PROTOCOL_UDP:
        case PROTOCOL_UDPv6: {
            protocol_name = "UDP";
            break;
        }
    }

    do {
        if (g_client_conns_t[*p_id].thread_quit) {
            closesocket(client_fd);
            CircularBuffer_Destroy(g_client_conns_t[*p_id].cb);
            g_client_conns_t[*p_id].cb = NULL;
            cleanGlobalQueue(client_queue, *p_id);
            qurt_mutex_lock(&client_mutex);
            ipd_message_print_flag = true;
            qurt_mutex_unlock(&client_mutex);
            memset((void *)buffer, 0, QAT_INPUT_BUFFER_LENGTH);
            snprintf(buffer, QAT_INPUT_BUFFER_LENGTH, "+IPS:CLOSED:%d\r\n", *p_id);
            QAT_Response_Str(QAT_RC_OK, buffer);
            CleanupClientConnInfo(*p_id);
            break;
        }

        recv_type = g_client_conns_t[*p_id].recv_type;
        max_fd = 0;
        FD_ZERO(&readfds);
        FD_SET(client_fd, &readfds);
        if (client_fd > max_fd) {
            max_fd = client_fd;
        }

        timeout.tv_sec = TIMEOUT_TV_SEC;
        timeout.tv_usec = TIMEOUT_TV_USEC;

        if ((recv_type == RECVTYPE_PASSIVE) && (ipd_message_print_flag == true) &&
            (uxQueueMessagesWaiting(client_queue) > 0) && (xQueuePeek(client_queue, &elem, 0) == pdTRUE)) {
            memset((void *)buffer, 0, QAT_INPUT_BUFFER_LENGTH);
            if (*p_id == elem.id) {
                snprintf(buffer, QAT_INPUT_BUFFER_LENGTH, "+IPD:%c,%s,%d,%d\r\n", 'C', protocol_name, elem.id,
                         elem.len);
                QAT_Response_Str(QAT_RC_QUIET, buffer);
                qurt_mutex_lock(&client_mutex);
                ipd_message_print_flag = false;
                qurt_mutex_unlock(&client_mutex);
            }
        }

        ret = select(max_fd + 1, &readfds, NULL, NULL, &timeout);
        if (ret < 0) {
            QAT_IP_PRINTF("select error \r\n");
            goto client_recv_fail;
            ;
        } else if (ret == 0) {
            // QAT_IP_PRINTF("select timeout \r\n");
            continue;
        } else {
            if (FD_ISSET(client_fd, &readfds)) {
                memset((void *)buffer, 0, QAT_INPUT_BUFFER_LENGTH);
                memset((void *)input_buf, 0, QAT_INPUT_BUFFER_LENGTH);
                if (recv_type == RECVTYPE_ACTIVE) {
                    recv_bytes = recv(client_fd, input_buf, sizeof(input_buf) - 1, 0);
                    if (recv_bytes < 0) {
                        if (errno == ECONNRESET || errno == ENOTCONN) {
                            goto client_recv_fail;
                        }
                    } else if (recv_bytes == 0) {
                        goto client_recv_fail;
                    } else {
                        if (isPassThroughMode) {
                            offset = 0;
                            offset += snprintf(buffer, QAT_INPUT_BUFFER_LENGTH, "+IPDHEX:%c,%s,%d,%d,", 'C',
                                               protocol_name, *p_id, recv_bytes);
                            memcpy(buffer + offset, input_buf, recv_bytes);
                            QAT_Response_Buffer(QAT_RC_QUIET, buffer, offset + recv_bytes);
                        } else {
                            input_buf[recv_bytes] = '\0';
                            snprintf(buffer, QAT_INPUT_BUFFER_LENGTH, "+IPD:%c,%s,%d,%d,%s", 'C', protocol_name, *p_id,
                                     recv_bytes, input_buf);
                            QAT_Response_Str(QAT_RC_QUIET, buffer);
                        }
                    }
                } else {
                    if ((g_client_conns_t[*p_id].protocol_type == PROTOCOL_TCP) ||
                        (g_client_conns_t[*p_id].protocol_type == PROTOCOL_TCPv6)) {
                        if (uxQueueSpacesAvailable(client_queue) == 0) {
                            continue;
                        }
                        result = CircularBuffer_GetFreeSpace(g_client_conns_t[*p_id].cb);
                        if (lwip_ioctl(client_fd, FIONREAD, &bytes_available) == 0) {
                            if (result < bytes_available) {
                                continue;
                            }
                        } else {
                            goto client_recv_fail;
                            ;
                        }
                    }
                    recv_bytes = recv(client_fd, input_buf, sizeof(input_buf) - 1, 0);
                    if (recv_bytes < 0) {
                        if (errno == ECONNRESET || errno == ENOTCONN) {
                            goto client_recv_fail;
                        }
                        continue;
                    } else if (recv_bytes == 0) {
                        goto client_recv_fail;
                    } else {
                        if ((g_client_conns_t[*p_id].protocol_type == PROTOCOL_UDP) ||
                            (g_client_conns_t[*p_id].protocol_type == PROTOCOL_UDPv6)) {
                            if (uxQueueSpacesAvailable(client_queue) == 0) {
                                continue;
                            }
                            result = CircularBuffer_GetFreeSpace(g_client_conns_t[*p_id].cb);
                            if (result < recv_bytes) {
                                continue;
                            }
                        }
                        elem.id = *p_id;
                        elem.len = recv_bytes;
                        if (xQueueSend(client_queue, &elem, 0) == pdPASS) {
                            CircularBuffer_Write(g_client_conns_t[*p_id].cb, input_buf, recv_bytes);
                        }
                    }
                }
            }
        }
    } while (1);

    nt_osal_thread_delete(NULL);
    return;

client_recv_fail:
    closesocket(client_fd);
    CircularBuffer_Destroy(g_client_conns_t[*p_id].cb);
    g_client_conns_t[*p_id].cb = NULL;
    cleanGlobalQueue(client_queue, *p_id);
    qurt_mutex_lock(&client_mutex);
    ipd_message_print_flag = true;
    qurt_mutex_unlock(&client_mutex);
    snprintf(buffer, QAT_INPUT_BUFFER_LENGTH, "+IPS:CLOSED:%d\r\n", *p_id);
    QAT_Response_Str(QAT_RC_QUIET, buffer);
    CleanupClientConnInfo(*p_id);
    nt_osal_thread_delete(NULL);
    return;
}

/*-------------------------------------------------------------------------
 * Function Definitions
 *-----------------------------------------------------------------------*/

/**
   @brief Processes the Extend command from the QAT.

   This command will change the current group to its parent. No parameters are
   expected for this command.

   @param[in] Op_Type          The input command type.
   @param[in] Parameter_Count  Number of parameters that were entered into the
                               command line.
   @param[in] Parameter_List   List of parameters entered into the command line.
*/
static QAT_Command_Status_t Extend_Command_Start(uint32_t Op_Type, uint32_t Parameter_Count,
                                                 QAT_Parameter_t *Parameter_List)
{
    char *ptr;
    bool is_ipv6;
    int link_id, port, local_port;
    QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
    int protocol_type;
    char buffer[QAT_CMD_IP_BUFFER_LENGTH] = {0};
    int offset;
    char *protocol_name;
    int fd;
    ip_addr_t ip_addr;
    struct sockaddr_storage local_addr, peer_addr;
    socklen_t local_addr_len = sizeof(local_addr);
    socklen_t peer_addr_len = sizeof(peer_addr);
    char peer_ip[INET6_ADDRSTRLEN] = {0};
    char local_ip[INET6_ADDRSTRLEN] = {0};

    switch (Op_Type) {
        case QAT_OP_EXEC: {
            QAT_Response_Str(QAT_RC_OK, "+CIPSTART=<link id>,<type>,<remote ip>,<remote port>\r\n");
            break;
        }
        case QAT_OP_QUERY: {
            offset = 0;
            memset((void *)buffer, 0, QAT_CMD_IP_BUFFER_LENGTH);
            for (int i = 0; i < QAT_CLIENT_MAX_CONNECTIONS; i++) {
                if (g_client_conns_t[i].active == INACTIVE) {
                    continue;
                }

                protocol_type = g_client_conns_t[i].protocol_type;
                switch (protocol_type) {
                    case PROTOCOL_TCP: {
                        protocol_name = "TCP";
                        break;
                    }
                    case PROTOCOL_TCPv6: {
                        protocol_name = "TCPv6";
                        break;
                    }
                    case PROTOCOL_UDP: {
                        protocol_name = "UDP";
                        break;
                    }
                    case PROTOCOL_UDPv6: {
                        protocol_name = "UDPv6";
                        break;
                    }
                    // case PROTOCOL_SSL:{
                    //     protocol_name = "SSL";
                    //     break;
                    // }
                    // case PROTOCOL_SSLv6:{
                    //     protocol_name = "SSLv6";
                    //     break;
                    // }
                    default: {
                    }
                }

                fd = g_client_conns_t[i].sockfd;
                memset(&peer_addr, 0, sizeof(peer_addr));
                if (getpeername(fd, (struct sockaddr *)&peer_addr, &peer_addr_len) != 0) {
                    continue;
                }
#if LWIP_IPV4
                if (peer_addr.ss_family == AF_INET) {
                    struct sockaddr_in *addr4 = (struct sockaddr_in *)&peer_addr;
                    port = addr4->sin_port;
                    inet_ntop(AF_INET, &addr4->sin_addr, peer_ip, sizeof(peer_ip));
                }
#endif
#if LWIP_IPV6
                if (peer_addr.ss_family == AF_INET6) {
                    struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)&peer_addr;
                    port = addr6->sin6_port;
                    inet_ntop(AF_INET6, &addr6->sin6_addr, peer_ip, sizeof(peer_ip));
                }
#endif
                memset(&local_addr, 0, sizeof(local_addr));
                if (getsockname(fd, (struct sockaddr *)&local_addr, &local_addr_len) != 0) {
                    continue;
                }

#if LWIP_IPV4
                if (local_addr.ss_family == AF_INET) {
                    struct sockaddr_in *addr4 = (struct sockaddr_in *)&local_addr;
                    local_port = addr4->sin_port;
                    inet_ntop(AF_INET, &addr4->sin_addr, local_ip, sizeof(local_ip));
                }
#endif
#if LWIP_IPV6
                if (local_addr.ss_family == AF_INET6) {
                    struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)&local_addr;
                    local_port = addr6->sin6_port;
                    inet_ntop(AF_INET6, &addr6->sin6_addr, local_ip, sizeof(local_ip));
                }
#endif
                offset += snprintf(buffer + offset, QAT_CMD_IP_BUFFER_LENGTH, "+CIPSTART:");
                offset += snprintf(buffer + offset, QAT_CMD_IP_BUFFER_LENGTH, "%c,", 'C');
                offset += snprintf(buffer + offset, QAT_CMD_IP_BUFFER_LENGTH, "%d,%s,", i, protocol_name);
                offset += snprintf(buffer + offset, QAT_CMD_IP_BUFFER_LENGTH, "%s,%d,", peer_ip, ntohs(port));
                offset += snprintf(buffer + offset, QAT_CMD_IP_BUFFER_LENGTH, "%s,%d\r\n", local_ip, ntohs(local_port));
            }
            rc = QAT_Response_Str(QAT_RC_OK, buffer);
            break;
        }
        case QAT_OP_EXEC_W_PARAM: {
            if ((Parameter_Count != 4) || (!Parameter_List) || (!Parameter_List[0].Integer_Is_Valid) ||
                Parameter_List[1].Integer_Is_Valid || Parameter_List[2].Integer_Is_Valid ||
                (!Parameter_List[3].Integer_Is_Valid)) {
                QAT_IP_PRINTF("+CIPSTART:Invalid input parameter!\r\n");
                goto end;
            }

            memset((void *)buffer, 0, QAT_CMD_IP_BUFFER_LENGTH);
            link_id = Parameter_List[0].Integer_Value;
            // check if link_id is valid
            if (link_id < 0 || link_id >= QAT_CLIENT_MAX_CONNECTIONS) {
                snprintf(buffer, QAT_CMD_IP_BUFFER_LENGTH,
                         "+CIPSTART:The value of link id must be an integer between 0 and 3\r\n");
                goto end;
            }

            // check if link_id is active
            if (g_client_conns_t[link_id].active == ACTIVE) {
                snprintf(buffer, QAT_CMD_IP_BUFFER_LENGTH, "+CIPSTART:Link ID %d is already in use\r\n", link_id);
                goto end;
            }

            if (strcmp(Parameter_List[1].String_Value, "TCP") == 0) {
                protocol_type = PROTOCOL_TCP;
            } else if (strcmp(Parameter_List[1].String_Value, "UDP") == 0) {
                protocol_type = PROTOCOL_UDP;
            } else if (strcmp(Parameter_List[1].String_Value, "TCPv6") == 0) {
                protocol_type = PROTOCOL_TCPv6;
            } else if (strcmp(Parameter_List[1].String_Value, "UDPv6") == 0) {
                protocol_type = PROTOCOL_UDPv6;
            } else {  // todo  SSL
                protocol_type = PROTOCOL_INVALID;
                snprintf(buffer, QAT_CMD_IP_BUFFER_LENGTH, "+CIPSTART:protocol_type is invalid!\r\n");
                goto end;
            }

            ptr = Parameter_List[2].String_Value;
            memset(&ip_addr, 0, sizeof(ip_addr));
            if (handle_parsed_data(ptr, &ip_addr, &is_ipv6) == QAT_ERROR) {
                QAT_Response_Str(QAT_RC_ERROR,
                                 "+CIPSTART:IP is not valid or interface name not set for IPv6 link address!\r\n");
                return rc;
            }

            port = Parameter_List[3].Integer_Value;
            if (CreateConnection(link_id, protocol_type, &ip_addr, port) == QAT_ERROR) {
                snprintf(buffer, QAT_CMD_IP_BUFFER_LENGTH, "+IPS:FAILED:%d\r\n", link_id);
                goto end;
            }

            if (nt_qurt_thread_create(client_recv_thread, "client_recv_task", 1024, &(g_client_conns_t[link_id].id), 6,
                                      NULL) != pdPASS) {
                closesocket(g_client_conns_t[link_id].sockfd);
                CleanupClientConnInfo(link_id);
                snprintf(buffer, QAT_CMD_IP_BUFFER_LENGTH, "+IPS:FAILED:%d\r\n", link_id);
                goto end;
            }

            snprintf(buffer, QAT_CMD_IP_BUFFER_LENGTH, "+IPS:CONNECTED:%d\r\n", link_id);
            rc = QAT_Response_Str(QAT_RC_OK, buffer);
            break;
        }
    }
    return rc;
end:
    QAT_Response_Str(QAT_RC_ERROR, buffer);
    return rc;
}

/**
   @brief Processes the Extend command from the QAT.

   This command will change the current group to its parent. No parameters are
   expected for this command.

   @param[in] Op_Type          The input command type.
   @param[in] Parameter_Count  Number of parameters that were entered into the
                               command line.
   @param[in] Parameter_List   List of parameters entered into the command line.
*/
static QAT_Command_Status_t Extend_Command_Close(uint32_t Op_Type, uint32_t Parameter_Count,
                                                 QAT_Parameter_t *Parameter_List)
{
    int link_id;
    int sockfd;
    QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
    char buffer[QAT_CMD_IP_BUFFER_LENGTH] = {0};

    switch (Op_Type) {
        case QAT_OP_EXEC: {
            QAT_Response_Str(QAT_RC_OK, "+CIPCLOSE=<link id>\r\n");
            break;
        }
        case QAT_OP_EXEC_W_PARAM: {
            if ((Parameter_Count != 1) || (!Parameter_List) || (!Parameter_List[0].Integer_Is_Valid)) {
                QAT_Response_Str(QAT_RC_ERROR, "+CIPCLOSE:Invalid input parameter!\r\n");
                return rc;
            }
            memset((void *)buffer, 0, QAT_CMD_IP_BUFFER_LENGTH);

            link_id = Parameter_List[0].Integer_Value;
            // check if link_id is valid
            if (link_id < 0 || link_id >= QAT_CLIENT_MAX_CONNECTIONS) {
                snprintf(buffer, QAT_CMD_IP_BUFFER_LENGTH,
                         "+CIPCLOSE:The value of link id must be an integer between 0 and 3\r\n");
                QAT_Response_Str(QAT_RC_ERROR, buffer);
                return rc;
            }

            // check if link_id is active
            if (g_client_conns_t[link_id].active == INACTIVE) {
                snprintf(buffer, QAT_CMD_IP_BUFFER_LENGTH, "+CIPCLOSE:Link ID %d is not active\r\n", link_id);
                QAT_Response_Str(QAT_RC_ERROR, buffer);
                return rc;
            }
            g_client_conns_t[link_id].thread_quit = true;
            sys_msleep(TIMEOUT_TV_SEC * QAT_TIME_SEC_TO_MS +
                       2 * TIMEOUT_TV_SEC /
                           QAT_TIME_SEC_TO_MS);  // wait for g_client_conns_t[link_id].thread_quit be valid
            break;
        }
    }
    return rc;
}

static QAT_Command_Status_t Extend_Command_Send(uint32_t Op_Type, uint32_t Parameter_Count,
                                                QAT_Parameter_t *Parameter_List)
{
    int link_id, sockfd;
    int total_sent, sent_bytes;
    char *output_buf = NULL;
    int data_mode_one_send_len;
    QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
    char buffer[QAT_CMD_IP_BUFFER_LENGTH] = {0};
    switch (Op_Type) {
        case QAT_OP_EXEC: {
            QAT_Response_Str(QAT_RC_OK, "+CIPSEND=<link id>,<len>\r\n");
            break;
        }
        case QAT_OP_EXEC_W_PARAM: {
            if ((Parameter_Count != 2) || (!Parameter_List) || (!Parameter_List[0].Integer_Is_Valid) ||
                (!Parameter_List[1].Integer_Is_Valid)) {
                QAT_Response_Str(QAT_RC_ERROR, "+CIPSEND:Invalid input parameter!\r\n");
                return rc;
            }

            link_id = Parameter_List[0].Integer_Value;
            // check if link_id is valid
            if (link_id < 0 || link_id >= QAT_CLIENT_MAX_CONNECTIONS) {
                QAT_Response_Str(QAT_RC_ERROR, "+CIPSEND:The value of link id must be an integer between 0 and 3\r\n");
                return rc;
            }

            // check if link_id is active
            if (g_client_conns_t[link_id].active == INACTIVE) {
                snprintf(buffer, QAT_CMD_IP_BUFFER_LENGTH, "+CIPSEND:link id %d is not active\r\n", link_id);
                QAT_Response_Str(QAT_RC_ERROR, buffer);
                return rc;
            }

            data_mode_link_id = link_id;
            data_mode_max_len = Parameter_List[1].Integer_Value;
            if (data_mode_max_len < 0) {
                QAT_Response_Str(QAT_RC_ERROR, "+CIPSEND:The len parameter must be greater than or equal to 0.\r\n");
                return rc;
            } else if (data_mode_max_len == 0) {
                exitLengthValid = false;
            } else {
                exitLengthValid = true;
            }
            extern Cur_Data_Mode_Cmd_t Cur_Data_Mode_Cmd;
            memcpy(Cur_Data_Mode_Cmd.cur_data_mode_commnd, "+CIPSEND", strlen("+CIPSEND"));

            QAT_Transfer_Mode_set(QAT_Transfer_Mode_ONLINE_DATA_E, QAT_Data_Transfer_Mode_Handle);
            QAT_Response_Str(QAT_RC_OK, NULL);
            snprintf(buffer, QAT_CMD_IP_BUFFER_LENGTH, ">\r\n");
            QAT_Response_Str(QAT_RC_QUIET_NO_CR, buffer);
            break;
        }
        case QAT_OP_EXEC_IN_DATA_MODEL: {
            output_buf = (char *)Parameter_List;
            if (exitLengthValid) {
                data_mode_one_send_len = (Parameter_Count <= (data_mode_max_len - data_mode_total_send_len))
                                             ? Parameter_Count
                                             : (data_mode_max_len - data_mode_total_send_len);
            } else {
                data_mode_one_send_len = Parameter_Count;
            }
            total_sent = 0;
            sockfd = g_client_conns_t[data_mode_link_id].sockfd;
            while (total_sent < data_mode_one_send_len) {
                sent_bytes = send(sockfd, output_buf + total_sent, data_mode_one_send_len - total_sent, 0);
                if (sent_bytes < 0) {
                    snprintf(buffer, QAT_CMD_IP_BUFFER_LENGTH, "+IPS:SEND FAILED:%d\r\n", data_mode_link_id);
                    QAT_Response_Str(QAT_RC_ERROR, buffer);
                    return rc;
                }
                total_sent += sent_bytes;
            }

            if (exitLengthValid) {
                data_mode_total_send_len += data_mode_one_send_len;
                if (data_mode_total_send_len >= data_mode_max_len) {
                    snprintf(buffer, QAT_CMD_IP_BUFFER_LENGTH, "+IPS:SEND DONE:%d\r\n", data_mode_link_id);
                    rc = QAT_Response_Str(QAT_RC_QUIET_NO_CR, buffer);
                    data_mode_total_send_len = 0;
                    data_mode_max_len = 0;
                    data_mode_link_id = INVALID_LINKID;
                    QAT_Transfer_Mode_set(QAT_Transfer_Mode_AT_COMMAND_E, NULL);
                }
            }
            break;
        }
    }
    return rc;
}

/*-------------------------------------------------------------------------
 * Function Definitions
 *-----------------------------------------------------------------------*/

/**
   @brief Processes the Extend command from the QAT.

   This command will change the current group to its parent. No parameters are
   expected for this command.

   @param[in] Op_Type          The input command type.
   @param[in] Parameter_Count  Number of parameters that were entered into the
                               command line.
   @param[in] Parameter_List   List of parameters entered into the command line.
*/

static QAT_Command_Status_t Extend_Command_SendData(uint32_t Op_Type, uint32_t Parameter_Count,
                                                    QAT_Parameter_t *Parameter_List)
{
    int link_id, sockfd;
    int set_len, str_len;
    int sent_len, total_sent;
    int sent_bytes;
    int sent_count;
    char *output_buf = NULL;
    QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
    char buffer[QAT_CMD_IP_BUFFER_LENGTH] = {0};

    switch (Op_Type) {
        case QAT_OP_EXEC: {
            QAT_Response_Str(QAT_RC_OK, "+CIPSENDDATA=<link id>,<len>,\"data\"\r\n");
            break;
        }
        case QAT_OP_EXEC_W_PARAM: {
            if ((Parameter_Count != 3) || (!Parameter_List) || (!Parameter_List[0].Integer_Is_Valid) ||
                (!Parameter_List[1].Integer_Is_Valid)) {
                QAT_Response_Str(QAT_RC_ERROR, "+CIPSENDDATA:Invalid input parameter!\r\n");
                return rc;
            }

            link_id = Parameter_List[0].Integer_Value;

            // check if link_id is valid
            if (link_id < 0 || link_id >= QAT_CLIENT_MAX_CONNECTIONS) {
                QAT_Response_Str(QAT_RC_ERROR,
                                 "+CIPSENDDATA:The value of link id must be an integer between 0 and 3\r\n");
                return rc;
            }

            // check if link_id is active
            if (g_client_conns_t[link_id].active == INACTIVE) {
                snprintf(buffer, QAT_CMD_IP_BUFFER_LENGTH, "+CIPSENDDATA:link id %d is not active\r\n", link_id);
                QAT_Response_Str(QAT_RC_ERROR, buffer);
                return rc;
            }

            set_len = Parameter_List[1].Integer_Value;
            output_buf = Parameter_List[2].String_Value;
            sockfd = g_client_conns_t[link_id].sockfd;

            str_len = strlen(output_buf);
            sent_len = set_len <= str_len ? set_len : str_len;

            total_sent = 0;
            sent_count = 0;
            while (total_sent < sent_len) {
                sent_bytes = send(sockfd, output_buf + total_sent, sent_len - total_sent, 0);
                if (sent_bytes < 0) {
                    snprintf(buffer, QAT_CMD_IP_BUFFER_LENGTH, "+IPS:SEND FAILED:%d\r\n", link_id);
                    QAT_Response_Str(QAT_RC_ERROR, buffer);
                    return rc;
                }
                total_sent += sent_bytes;

                if (sent_count >= DATA_MAX_SEND_COUNT) {
                    snprintf(buffer, QAT_CMD_IP_BUFFER_LENGTH, "+IPS:SEND FAILED:%d\r\n", link_id);
                    QAT_Response_Str(QAT_RC_ERROR, buffer);
                    return rc;
                }
                sent_count++;
            }
            snprintf(buffer, QAT_CMD_IP_BUFFER_LENGTH, "+IPS:SEND DONE:%d\r\n", link_id);
            rc = QAT_Response_Str(QAT_RC_OK, buffer);
        }
    }
    return rc;
}

/**
   @brief Processes the Extend command from the QAT.

   This command will change the current group to its parent. No parameters are
   expected for this command.

   @param[in] Op_Type          The input command type.
   @param[in] Parameter_Count  Number of parameters that were entered into the
                               command line.
   @param[in] Parameter_List   List of parameters entered into the command line.
*/
static QAT_Command_Status_t Extend_Command_RecvType(uint32_t Op_Type, uint32_t Parameter_Count,
                                                    QAT_Parameter_t *Parameter_List)
{
    QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
    int link_id, recv_type;
    char buffer[QAT_CMD_IP_BUFFER_LENGTH] = {0};
    int offset;
    char *serverFlag = NULL;
    char *protocol_name = NULL;
    char *params;
    switch (Op_Type) {
        case QAT_OP_EXEC: {
            QAT_Response_Str(QAT_RC_OK, "+CIPRECVTYPE=<serverFlag>,<type>,<link_id>,<reception mode>\r\n");
            break;
        }
        case QAT_OP_QUERY: {
            offset = 0;
            for (int i = 0; i < QAT_CLIENT_MAX_CONNECTIONS; i++) {
                if ((g_client_conns_t[i].active == ACTIVE) && (g_client_conns_t[i].protocol_type != PROTOCOL_INVALID)) {
                    if ((g_client_conns_t[i].protocol_type == PROTOCOL_TCP) ||
                        (g_client_conns_t[i].protocol_type == PROTOCOL_TCPv6)) {
                        offset += snprintf(buffer + offset, QAT_CMD_IP_BUFFER_LENGTH, "+CIPRECVTYPE:%c,%s,%d,%d\r\n",
                                           'C', "TCP", i, g_client_conns_t[i].recv_type);
                    } else if ((g_client_conns_t[i].protocol_type == PROTOCOL_UDP) ||
                               (g_client_conns_t[i].protocol_type == PROTOCOL_UDPv6)) {
                        offset += snprintf(buffer + offset, QAT_CMD_IP_BUFFER_LENGTH, "+CIPRECVTYPE:%c,%s,%d,%d\r\n",
                                           'C', "UDP", i, g_client_conns_t[i].recv_type);
                    }
                } else {
                    continue;
                }
            }

            for (int i = 0; i < QAT_CLIENT_MAX_CONNECTIONS; i++) {
                if (g_listen_clients[i].active == ACTIVE) {
                    offset += snprintf(buffer + offset, QAT_CMD_IP_BUFFER_LENGTH, "+CIPRECVTYPE:%c,%s,%d,%d\r\n", 'S',
                                       "TCP", i, g_listen_clients[i].recv_type);
                } else {
                    continue;
                }
            }

            for (int i = 0; i < QAT_CLIENT_MAX_CONNECTIONS; i++) {
                if (g_listen_udp_clients[i].active == ACTIVE) {
                    offset += snprintf(buffer + offset, QAT_CMD_IP_BUFFER_LENGTH, "+CIPRECVTYPE:%c,%s,%d,%d\r\n", 'S',
                                       "UDP", i, g_listen_udp_clients[i].recv_type);
                } else {
                    continue;
                }
            }
            rc = QAT_Response_Str(QAT_RC_OK, buffer);
            break;
        }
        case QAT_OP_EXEC_W_PARAM: {
            if ((Parameter_Count != 4) || !Parameter_List || (Parameter_List[0].Integer_Is_Valid) ||
                (Parameter_List[1].Integer_Is_Valid) || (!Parameter_List[2].Integer_Is_Valid) ||
                (!Parameter_List[3].Integer_Is_Valid)) {
                QAT_Response_Str(QAT_RC_ERROR, "+CIPRECVTYPE:Invalid input parameter!\r\n");
                return rc;
            }

            serverFlag = Parameter_List[0].String_Value;
            if ((strcmp(serverFlag, "C") != 0) && (strcmp(serverFlag, "S") != 0)) {
                QAT_Response_Str(QAT_RC_ERROR,
                                 "+CIPRECVTYPE:serverFlag must be a string of C or S, C:Client, S:Server\r\n");
                return rc;
            }
            params = Parameter_List[1].String_Value;
            if ((strcmp(params, "TCP") != 0) && (strcmp(params, "UDP") != 0)) {
                snprintf(buffer, QAT_CMD_IP_BUFFER_LENGTH, "+CIPRECVTYPE:protocol_type is invalid!\r\n");
                QAT_Response_Str(QAT_RC_ERROR, buffer);
                return rc;
            }

            link_id = Parameter_List[2].Integer_Value;
            // check if link_id is valid
            if (link_id < 0 || link_id >= QAT_CLIENT_MAX_CONNECTIONS) {
                QAT_Response_Str(QAT_RC_ERROR,
                                 "+CIPRECVTYPE:The value of link id must be an integer between 0 and 3\r\n");
                return rc;
            }
            recv_type = Parameter_List[3].Integer_Value;
            if (recv_type < RECVTYPE_ACTIVE || recv_type > RECVTYPE_PASSIVE) {
                QAT_Response_Str(QAT_RC_ERROR,
                                 "+CIPRECVTYPE:recv_type must be an integer between 0 and 1, 0:ACTIVE, 1:PASSIVE\r\n");
                return rc;
            }

            if (strcmp(serverFlag, "C") == 0) {
                if (g_client_conns_t[link_id].active == INACTIVE) {
                    snprintf(buffer, QAT_CMD_IP_BUFFER_LENGTH, "+CIPRECVTYPE:Client link id %d is not active\r\n",
                             link_id);
                    QAT_Response_Str(QAT_RC_ERROR, buffer);
                    return rc;
                }
                switch (g_client_conns_t[link_id].protocol_type) {
                    case PROTOCOL_TCP:
                    case PROTOCOL_TCPv6: {
                        protocol_name = "TCP";
                        break;
                    }
                    case PROTOCOL_UDP:
                    case PROTOCOL_UDPv6: {
                        protocol_name = "UDP";
                        break;
                    }
                    default: {
                        ;
                    }
                }

                if (strcmp(params, protocol_name) != 0) {
                    snprintf(buffer, QAT_CMD_IP_BUFFER_LENGTH,
                             "+CIPRECVTYPE:Client link id %d protocol_type is not match\r\n", link_id);
                    QAT_Response_Str(QAT_RC_ERROR, buffer);
                    return rc;
                }
                g_client_conns_t[link_id].recv_type = recv_type;
            } else {
                if (strcmp(params, "TCP") == 0) {
                    if (g_listen_clients[link_id].active == INACTIVE) {
                        snprintf(buffer, QAT_CMD_IP_BUFFER_LENGTH,
                                 "+CIPRECVTYPE:TCP Server link id %d is not active\r\n", link_id);
                        QAT_Response_Str(QAT_RC_ERROR, buffer);
                        return rc;
                    }
                    g_listen_clients[link_id].recv_type = recv_type;
                } else {
                    if (g_listen_udp_clients[link_id].active == INACTIVE) {
                        snprintf(buffer, QAT_CMD_IP_BUFFER_LENGTH,
                                 "+CIPRECVTYPE:UDP Server link id %d is not active\r\n", link_id);
                        QAT_Response_Str(QAT_RC_ERROR, buffer);
                        return rc;
                    }
                    g_listen_udp_clients[link_id].recv_type = recv_type;
                }
            }
            rc = QAT_Response_Str(QAT_RC_OK, NULL);
            break;
        }
    }
    return rc;
}

/*-------------------------------------------------------------------------
 * Function Definitions
 *-----------------------------------------------------------------------*/

/**
   @brief Processes the Extend command from the QAT.

   This command will change the current group to its parent. No parameters are
   expected for this command.

   @param[in] Op_Type          The input command type.
   @param[in] Parameter_Count  Number of parameters that were entered into the
                               command line.
   @param[in] Parameter_List   List of parameters entered into the command line.
*/

static QAT_Command_Status_t Extend_Command_RecvData(uint32_t Op_Type, uint32_t Parameter_Count,
                                                    QAT_Parameter_t *Parameter_List)
{
    int link_id, data_len;
    QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
    char input_data[QAT_DATA_INPUT_BUFFER_LENGTH] = {0};
    char buffer[QAT_INPUT_BUFFER_LENGTH] = {0};
    QueueElem elem;
    char *serverFlag = NULL;
    char *protocol_name = NULL;
    char *params;
    switch (Op_Type) {
        case QAT_OP_EXEC: {
            QAT_Response_Str(QAT_RC_OK, "+CIPRECVDATA=<serverFlag>,<type>,<link_id>,<len>\r\n");
            break;
        }
        case QAT_OP_EXEC_W_PARAM: {
            if ((Parameter_Count != 4) || !Parameter_List || (Parameter_List[0].Integer_Is_Valid) ||
                (Parameter_List[1].Integer_Is_Valid) || (!Parameter_List[2].Integer_Is_Valid) ||
                (!Parameter_List[3].Integer_Is_Valid)) {
                QAT_Response_Str(QAT_RC_ERROR, "+CIPRECVDATA:Invalid input parameters\r\n");
                return rc;
            }

            serverFlag = Parameter_List[0].String_Value;
            if ((strcmp(serverFlag, "C") != 0) && (strcmp(serverFlag, "S") != 0)) {
                QAT_Response_Str(QAT_RC_ERROR,
                                 "+CIPRECVDATA:serverFlag must be a string of C or S, C:Client, S:Server\r\n");
                return rc;
            }

            params = Parameter_List[1].String_Value;
            if ((strcmp(params, "TCP") != 0) && (strcmp(params, "UDP") != 0)) {
                snprintf(buffer, QAT_INPUT_BUFFER_LENGTH, "+CIPRECVDATA:protocol_type is invalid!\r\n");
                QAT_Response_Str(QAT_RC_ERROR, buffer);
                return rc;
            }

            link_id = Parameter_List[2].Integer_Value;
            if (link_id < 0 || link_id >= QAT_CLIENT_MAX_CONNECTIONS) {
                QAT_Response_Str(QAT_RC_ERROR,
                                 "+CIPRECVDATA:The value of link id must be an integer between 0 and 3\r\n");
                return rc;
            }
            data_len = Parameter_List[3].Integer_Value;
            if (data_len <= 0) {
                QAT_Response_Str(QAT_RC_ERROR, "+CIPRECVDATA:The value of data_len must be greater than 0\r\n");
                return rc;
            }

            if (strcmp(serverFlag, "C") == 0) {
                if (g_client_conns_t[link_id].active == INACTIVE) {
                    snprintf(buffer, QAT_INPUT_BUFFER_LENGTH, "+CIPRECVDATA:Client link id %d is not active\r\n",
                             link_id);
                    QAT_Response_Str(QAT_RC_ERROR, buffer);
                    return rc;
                }
                switch (g_client_conns_t[link_id].protocol_type) {
                    case PROTOCOL_TCP:
                    case PROTOCOL_TCPv6: {
                        protocol_name = "TCP";
                        break;
                    }
                    case PROTOCOL_UDP:
                    case PROTOCOL_UDPv6: {
                        protocol_name = "UDP";
                        break;
                    }
                    default: {
                        ;
                    }
                }
                if (strcmp(params, protocol_name) != 0) {
                    snprintf(buffer, QAT_INPUT_BUFFER_LENGTH,
                             "+CIPRECVDATA:Client link id %d input protocol_type is not match\r\n", link_id);
                    QAT_Response_Str(QAT_RC_ERROR, buffer);
                    return rc;
                }

                if (g_client_conns_t[link_id].recv_type == RECVTYPE_ACTIVE) {
                    snprintf(buffer, QAT_INPUT_BUFFER_LENGTH,
                             "+CIPRECVDATA:Client link id %d is not passive receive type\r\n", link_id);
                    QAT_Response_Str(QAT_RC_ERROR, buffer);
                    return rc;
                }

                if (xQueuePeek(client_queue, &elem, 0) == pdFALSE) {
                    QAT_Response_Str(QAT_RC_ERROR, "+CIPRECVDATA:The news of the client queue is empty\r\n");
                    return rc;
                }

                if (data_len > elem.len) {
                    QAT_Response_Str(
                        QAT_RC_ERROR,
                        "+CIPRECVDATA:Reading length cannot be greater than the message length of the +IPD prompt\r\n");
                    return rc;
                } else if (data_len == elem.len) {
                    if (CircularBuffer_Read(g_client_conns_t[link_id].cb, input_data, data_len) < 0) {
                        QAT_Response_Str(
                            QAT_RC_ERROR,
                            "+CIPRECVDATA:Reading length cannot be greater than client ring buffer length\r\n");
                        return rc;
                    }
                    xQueueReceive(client_queue, &elem, 0);
                } else {
                    if (CircularBuffer_Read(g_client_conns_t[link_id].cb, input_data, data_len) < 0) {
                        QAT_Response_Str(QAT_RC_ERROR,
                                         "+CIPRECVDATA:Reading length cannot be greater than ring buffer length\r\n");
                        return rc;
                    }
                    size_t remaining_len = elem.len - data_len;
                    xQueueReceive(client_queue, &elem, 0);
                    elem.len = remaining_len;
                    xQueueSendToFront(client_queue, &elem, 0);
                }

                input_data[data_len] = '\0';
                snprintf(buffer, QAT_INPUT_BUFFER_LENGTH, "+CIPRECVDATA:%c,%s,%d,%s\r\n", 'C',
                         Parameter_List[1].String_Value, data_len, input_data);
                rc = QAT_Response_Str(QAT_RC_OK, buffer);
                qurt_mutex_lock(&client_mutex);
                ipd_message_print_flag = true;
                qurt_mutex_unlock(&client_mutex);
            } else {
                if (strcmp(params, "TCP") == 0) {
                    if (g_listen_clients[link_id].active == INACTIVE) {
                        snprintf(buffer, QAT_INPUT_BUFFER_LENGTH,
                                 "+CIPRECVDATA:TCP server link id %d is not active\r\n", link_id);
                        QAT_Response_Str(QAT_RC_ERROR, buffer);
                        return rc;
                    }
                    if (g_listen_clients[link_id].recv_type == RECVTYPE_ACTIVE) {
                        snprintf(buffer, QAT_INPUT_BUFFER_LENGTH,
                                 "+CIPRECVDATA:TCP server link id %d is not passive receive type\r\n", link_id);
                        QAT_Response_Str(QAT_RC_ERROR, buffer);
                        return rc;
                    }
                    if (xQueuePeek(server_queue, &elem, 0) == pdFALSE) {
                        QAT_Response_Str(QAT_RC_ERROR, "+CIPRECVDATA:The news of the tcp server queue is empty");
                        return rc;
                    }

                    if (data_len > elem.len) {
                        QAT_Response_Str(QAT_RC_ERROR,
                                         "+CIPRECVDATA:Reading length cannot be greater than the message length of the "
                                         "+IPD prompt\r\n");
                        return rc;
                    } else if (data_len == elem.len) {
                        if (CircularBuffer_Read(server_cb, input_data, data_len) < 0) {
                            QAT_Response_Str(
                                QAT_RC_ERROR,
                                "+CIPRECVDATA:Reading length cannot be greater than tcp server ring buffer length\r\n");
                            return rc;
                        }
                        xQueueReceive(server_queue, &elem, 0);
                    } else {
                        if (CircularBuffer_Read(server_cb, input_data, data_len) < 0) {
                            QAT_Response_Str(
                                QAT_RC_ERROR,
                                "+CIPRECVDATA:Reading length cannot be greater than tcp server ring buffer length\r\n");
                            return rc;
                        }
                        size_t remaining_len = elem.len - data_len;
                        xQueueReceive(server_queue, &elem, 0);
                        elem.len = remaining_len;
                        xQueueSendToFront(server_queue, &elem, 0);
                    }

                    input_data[data_len] = '\0';
                    snprintf(buffer, QAT_INPUT_BUFFER_LENGTH, "+CIPRECVDATA:%c,%s,%d,%s\r\n", 'S', "TCP", data_len,
                             input_data);
                    rc = QAT_Response_Str(QAT_RC_OK, buffer);
                    qurt_mutex_lock(&server_mutex);
                    server_ipd_message_print_flag = true;
                    qurt_mutex_unlock(&server_mutex);
                } else if (strcmp(params, "UDP") == 0) {
                    if (g_listen_udp_clients[link_id].active == INACTIVE) {
                        snprintf(buffer, QAT_INPUT_BUFFER_LENGTH,
                                 "+CIPRECVDATA:UDP server link id %d is not active\r\n", link_id);
                        QAT_Response_Str(QAT_RC_ERROR, buffer);
                        return rc;
                    }

                    if (g_listen_udp_clients[link_id].recv_type == RECVTYPE_ACTIVE) {
                        snprintf(buffer, QAT_INPUT_BUFFER_LENGTH,
                                 "+CIPRECVDATA:UDP server link id %d is not passive receive type\r\n", link_id);
                        QAT_Response_Str(QAT_RC_ERROR, buffer);
                        return rc;
                    }

                    if (xQueuePeek(udp_server_queue, &elem, 0) == pdFALSE) {
                        QAT_Response_Str(QAT_RC_ERROR, "+CIPRECVDATA:The news of the udp server queue is empty");
                        return rc;
                    }

                    if (data_len > elem.len) {
                        QAT_Response_Str(QAT_RC_ERROR,
                                         "+CIPRECVDATA:Reading length cannot be greater than the message length of the "
                                         "+IPD prompt\r\n");
                        return rc;
                    } else if (data_len == elem.len) {
                        if (CircularBuffer_Read(udp_server_cb, input_data, data_len) < 0) {
                            QAT_Response_Str(
                                QAT_RC_ERROR,
                                "+CIPRECVDATA:Reading length cannot be greater than udp server ring buffer length\r\n");
                            return rc;
                        }
                        xQueueReceive(udp_server_queue, &elem, 0);
                    } else {
                        if (CircularBuffer_Read(udp_server_cb, input_data, data_len) < 0) {
                            QAT_Response_Str(
                                QAT_RC_ERROR,
                                "+CIPRECVDATA:Reading length cannot be greater than udp server ring buffer length\r\n");
                            return rc;
                        }
                        size_t remaining_len = elem.len - data_len;
                        xQueueReceive(udp_server_queue, &elem, 0);
                        elem.len = remaining_len;
                        xQueueSendToFront(udp_server_queue, &elem, 0);
                    }

                    input_data[data_len] = '\0';
                    snprintf(buffer, QAT_INPUT_BUFFER_LENGTH, "+CIPRECVDATA:%c,%s,%d,%s\r\n", 'S', "UDP", data_len,
                             input_data);
                    rc = QAT_Response_Str(QAT_RC_OK, buffer);
                    qurt_mutex_lock(&udp_server_mutex);
                    udp_server_ipd_message_print_flag = true;
                    qurt_mutex_unlock(&udp_server_mutex);
                }
            }
            break;
        }
    }
    return rc;
}

static void CleanupListenClientConnInfo(int link_id)
{
    g_listen_clients[link_id].sockfd = INVALID_FD;
    g_listen_clients[link_id].active = false;
    g_listen_clients[link_id].recv_type = RECVTYPE_ACTIVE;
}
static void CleanupUdpListenClientConnInfo(int link_id)
{
    g_listen_udp_clients[link_id].sockfd = INVALID_FD;
    memset(&(g_listen_udp_clients[link_id].addr), 0, sizeof(sock_addr));
    g_listen_udp_clients[link_id].active = false;
    g_listen_udp_clients[link_id].recv_type = RECVTYPE_ACTIVE;
    g_listen_udp_clients[link_id].v6 = false;
}

/*-------------------------------------------------------------------------
 * Function Definitions
 *-----------------------------------------------------------------------*/
int find_invalid_socket()
{
    for (int i = 0; i < QAT_CLIENT_MAX_CONNECTIONS; i++) {
        if (g_listen_clients[i].active == false) {
            return i;
        }
    }
    return QAT_ERROR;
}

/*-------------------------------------------------------------------------
 * Function Definitions
 *-----------------------------------------------------------------------*/
static void tcp_server_thread(void *arg)
{
    server_config *config = (server_config *)arg;
    int data_fd, sd, maxfd;
    struct sockaddr_storage from_addr;
    socklen_t from_addr_len = sizeof(from_addr);
    fd_set readfds;
    int index, fd_index;
    char buffer[QAT_INPUT_BUFFER_LENGTH] = {0};
    char input_buf[QAT_DATA_INPUT_BUFFER_LENGTH];
    struct timeval timeout;
    int result;
    int recv_type, recv_bytes;
    int bytes_available;
    int offset;
    QueueElem elem;

    do {
        if (config->mode == 0) {
            if (tcp_listen_fd >= 0) {
                closesocket(tcp_listen_fd);
                tcp_listen_fd = INVALID_FD;
            }

            if (config->params.closeServer) {
                for (int index = 0; index < QAT_CLIENT_MAX_CONNECTIONS; index++) {
                    if (g_listen_clients[index].sockfd != INVALID_FD) {
                        closesocket(g_listen_clients[index].sockfd);
                        CleanupListenClientConnInfo(index);
                    }
                }
                CircularBuffer_Destroy(server_cb);
                server_cb = NULL;
                qurt_mutex_lock(&server_mutex);
                server_ipd_message_print_flag = true;
                qurt_mutex_unlock(&server_mutex);
                tcpServerThreadCreated = false;
                nt_osal_thread_delete(NULL);
                return;
            }
        }

        maxfd = 0;
        FD_ZERO(&readfds);
        if (tcp_listen_fd >= 0) {
            FD_SET(tcp_listen_fd, &readfds);
            maxfd = tcp_listen_fd;
        }

        for (index = 0; index < QAT_CLIENT_MAX_CONNECTIONS; index++) {
            sd = g_listen_clients[index].sockfd;
            if (sd >= 0) {
                FD_SET(sd, &readfds);
            }
            if (sd > maxfd) {
                maxfd = sd;
            }
        }

        timeout.tv_sec = TIMEOUT_TV_SEC;
        timeout.tv_usec = TIMEOUT_TV_USEC;

        if ((server_ipd_message_print_flag == true) && (uxQueueMessagesWaiting(server_queue) > 0) &&
            (xQueuePeek(server_queue, &elem, 0) == pdTRUE)) {
            memset((void *)buffer, 0, QAT_INPUT_BUFFER_LENGTH);
            if (g_listen_clients[elem.id].recv_type == RECVTYPE_PASSIVE) {
                snprintf(buffer, QAT_INPUT_BUFFER_LENGTH, "+IPD:%c,%s,%d,%d\r\n", 'S', "TCP", elem.id, elem.len);
                QAT_Response_Str(QAT_RC_QUIET, buffer);
                qurt_mutex_lock(&server_mutex);
                server_ipd_message_print_flag = false;
                qurt_mutex_unlock(&server_mutex);
            }
        }

        if (select(maxfd + 1, &readfds, NULL, NULL, &timeout) > 0) {
            if ((tcp_listen_fd >= 0) && FD_ISSET(tcp_listen_fd, &readfds)) {
                if ((data_fd = accept(tcp_listen_fd, (struct sockaddr *)&from_addr, &from_addr_len)) < 0) {
                    printf("accept failed");
                    continue;
                }
                fd_index = find_invalid_socket();
                if (fd_index == QAT_ERROR) {
                    QAT_Response_Str(QAT_RC_ERROR, "Conn_Exceeded\r\n");
                    continue;
                }
                g_listen_clients[fd_index].sockfd = data_fd;
                g_listen_clients[fd_index].active = ACTIVE;
            }

            for (index = 0; index < QAT_CLIENT_MAX_CONNECTIONS; index++) {
                sd = g_listen_clients[index].sockfd;
                recv_type = g_listen_clients[index].recv_type;
                if (sd < 0) {
                    continue;
                }
                if (FD_ISSET(sd, &readfds)) {
                    memset((void *)input_buf, 0, QAT_DATA_INPUT_BUFFER_LENGTH);
                    memset((void *)buffer, 0, QAT_INPUT_BUFFER_LENGTH);
                    if (recv_type == RECVTYPE_ACTIVE) {
                        recv_bytes = recv(sd, input_buf, sizeof(input_buf) - 1, 0);
                        if (recv_bytes < 0) {
                            if (errno == ECONNRESET || errno == ENOTCONN) {
                                closesocket(g_listen_clients[index].sockfd);
                                CleanupListenClientConnInfo(index);
                            }
                            continue;
                        } else if (recv_bytes == 0) {
                            closesocket(g_listen_clients[index].sockfd);
                            CleanupListenClientConnInfo(index);
                            continue;
                        } else {
                            if (isPassThroughMode) {
                                offset = 0;
                                offset += snprintf(buffer, QAT_INPUT_BUFFER_LENGTH, "+IPDHEX:%c,%s,%d,%d,", 'S', "TCP",
                                                   index, recv_bytes);
                                memcpy(buffer + offset, input_buf, recv_bytes);
                                QAT_Response_Buffer(QAT_RC_QUIET, buffer, offset + recv_bytes);
                            } else {
                                input_buf[recv_bytes] = '\0';
                                snprintf(buffer, QAT_INPUT_BUFFER_LENGTH, "+IPD:%c,%s,%d,%d,%s", 'S', "TCP", index,
                                         recv_bytes, input_buf);
                                QAT_Response_Str(QAT_RC_QUIET, buffer);
                            }
                        }
                    } else {
                        if (uxQueueSpacesAvailable(server_queue) == 0) {
                            continue;
                        }
                        result = CircularBuffer_GetFreeSpace(server_cb);
                        if (lwip_ioctl(sd, FIONREAD, &bytes_available) == 0) {
                            if (result < bytes_available) {
                                continue;
                            }
                        }
                        recv_bytes = recv(sd, input_buf, sizeof(input_buf) - 1, 0);
                        if (recv_bytes < 0) {
                            if (errno == ECONNRESET || errno == ENOTCONN) {
                                closesocket(g_listen_clients[index].sockfd);
                                CleanupListenClientConnInfo(index);
                            }
                            continue;
                        } else if (recv_bytes == 0) {
                            closesocket(g_listen_clients[index].sockfd);
                            CleanupListenClientConnInfo(index);
                            continue;
                        } else {
                            elem.id = index;
                            elem.len = recv_bytes;
                            if (xQueueSend(server_queue, &elem, 0) == pdPASS) {
                                CircularBuffer_Write(server_cb, input_buf, recv_bytes);
                            }
                        }
                    }
                }
            }
        }
        sys_msleep(200);
    } while (1);
}
/*-------------------------------------------------------------------------
 * Function Definitions
 *-----------------------------------------------------------------------*/

/**
   @brief Processes the Extend command from the QAT.

   This command will change the current group to its parent. No parameters are
   expected for this command.

   @param[in] Op_Type          The input command type.
   @param[in] Parameter_Count  Number of parameters that were entered into the
                               command line.
   @param[in] Parameter_List   List of parameters entered into the command line.
*/
static QAT_Command_Status_t Extend_Command_Server(uint32_t Op_Type, uint32_t Parameter_Count,
                                                  QAT_Parameter_t *Parameter_List)
{
    int mode, value;
    QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
    char buf[QAT_CMD_IP_BUFFER_LENGTH] = {0};
    int offset;
    int peer_port, local_port;
    int fd;
    struct sockaddr_storage local_addr, peer_addr;
    socklen_t local_addr_len = sizeof(local_addr);
    socklen_t peer_addr_len = sizeof(peer_addr);
    char peer_ip[INET6_ADDRSTRLEN] = {0};
    char local_ip[INET6_ADDRSTRLEN] = {0};
    int protocol_type = PROTOCOL_TCP;
    uint8_t ca_enable = 0;
    int keep_alive = 0;
    bool is_v6 = false;
    int opt = 1;
    switch (Op_Type) {
        case QAT_OP_EXEC: {
            QAT_Response_Str(QAT_RC_QUIET, "+CIPSERVER=<mode>,<param2>,[<\"type\">], [<CA enable>], [<keepalive>]");
            QAT_Response_Str(QAT_RC_QUIET, "mode:\r\n0: Shut down the server\r\n1: Build server");
            QAT_Response_Str(QAT_RC_QUIET,
                             "if mode = 0, param2 can be either 0 or 1:\r\n    param2=0:Maintain existing "
                             "server connections.\r\n    param2=1:Completely shut down the server.");
            QAT_Response_Str(QAT_RC_QUIET, "if mode = 1: param2 is listen port");
            QAT_Response_Str(QAT_RC_OK, NULL);
            break;
        }
        case QAT_OP_QUERY: {
            offset = 0;
            memset((void *)buf, 0, QAT_CMD_IP_BUFFER_LENGTH);
            for (int i = 0; i < QAT_CLIENT_MAX_CONNECTIONS; i++) {
                if (g_listen_clients[i].active == INACTIVE) {
                    continue;
                }
                fd = g_listen_clients[i].sockfd;

                memset(&peer_addr, 0, sizeof(peer_addr));
                if (getpeername(fd, (struct sockaddr *)&peer_addr, &peer_addr_len) != 0) {
                    QAT_Response_Str(QAT_RC_ERROR, NULL);
                    return rc;
                }
#if LWIP_IPV4
                if (peer_addr.ss_family == AF_INET) {
                    struct sockaddr_in *addr4 = (struct sockaddr_in *)&peer_addr;
                    peer_port = addr4->sin_port;
                    inet_ntop(AF_INET, &addr4->sin_addr, peer_ip, sizeof(peer_ip));
                }
#endif
#if LWIP_IPV6
                if (peer_addr.ss_family == AF_INET6) {
                    struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)&peer_addr;
                    peer_port = addr6->sin6_port;
                    inet_ntop(AF_INET6, &addr6->sin6_addr, peer_ip, sizeof(peer_ip));
                }
#endif
                memset(&local_addr, 0, sizeof(local_addr));
                if (getsockname(fd, (struct sockaddr *)&local_addr, &local_addr_len) != 0) {
                    QAT_Response_Str(QAT_RC_ERROR, NULL);
                    return rc;
                }

#if LWIP_IPV4
                if (local_addr.ss_family == AF_INET) {
                    struct sockaddr_in *addr4 = (struct sockaddr_in *)&local_addr;
                    local_port = addr4->sin_port;
                    inet_ntop(AF_INET, &addr4->sin_addr, local_ip, sizeof(local_ip));
                }
#endif
#if LWIP_IPV6
                if (local_addr.ss_family == AF_INET6) {
                    struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)&local_addr;
                    local_port = addr6->sin6_port;
                    inet_ntop(AF_INET6, &addr6->sin6_addr, local_ip, sizeof(local_ip));
                }
#endif
                offset += snprintf(buf + offset, QAT_CMD_IP_BUFFER_LENGTH, "+CIPSERVER:");
                offset += snprintf(buf + offset, QAT_CMD_IP_BUFFER_LENGTH, "%c,", 'S');
                if (peer_addr.ss_family == AF_INET6) {
                    offset += snprintf(buf + offset, QAT_CMD_IP_BUFFER_LENGTH, "%d,%s,", i, "TCPv6");
                } else {
                    offset += snprintf(buf + offset, QAT_CMD_IP_BUFFER_LENGTH, "%d,%s,", i, "TCP");
                }
                offset += snprintf(buf + offset, QAT_CMD_IP_BUFFER_LENGTH, "%s,%d,", peer_ip, ntohs(peer_port));
                offset += snprintf(buf + offset, QAT_CMD_IP_BUFFER_LENGTH, "%s,%d\r\n", local_ip, ntohs(local_port));
            }
            rc = QAT_Response_Str(QAT_RC_OK, buf);
            break;
        }
        case QAT_OP_EXEC_W_PARAM: {
            if (Parameter_Count < 2 || Parameter_Count > 5 || !Parameter_List || !Parameter_List[0].Integer_Is_Valid ||
                !Parameter_List[1].Integer_Is_Valid) {
                QAT_Response_Str(QAT_RC_ERROR, "+CIPSERVER:Invalid input parameter!\r\n");
                return rc;
            }

            mode = Parameter_List[0].Integer_Value;
            value = Parameter_List[1].Integer_Value;
            switch (mode) {
                case 0: {
                    if (value < 0 || value > 1) {
                        QAT_Response_Str(QAT_RC_ERROR, "+CIPSERVER:when mode = 0, param2 can only be 0 or 1\r\n");
                        return rc;
                    }
                    tcp_config.params.closeServer = value;
                    break;
                }
                case 1: {
                    if (value < 0 || value > 65535) {
                        QAT_Response_Str(QAT_RC_ERROR, "+CIPSERVER:The port number must not exceed 65535.\r\n");
                        return rc;
                    }
                    tcp_config.params.port = value;
                    break;
                }
                default: {
                    QAT_Response_Str(QAT_RC_ERROR, "+CIPSERVER:mode can only be 0 or 1\r\n");
                    return rc;
                }
            }
            tcp_config.mode = mode;

            if (Parameter_Count >= 3) {
                if (Parameter_List[2].Integer_Is_Valid) {
                    QAT_Response_Str(QAT_RC_ERROR, "+CIPSERVER:Protocol type parameter must be a string!\r\n");
                    return rc;
                } else {
                    if (strcmp(Parameter_List[2].String_Value, "TCP") == 0) {
                        protocol_type = PROTOCOL_TCP;
                    } else if (strcmp(Parameter_List[2].String_Value, "SSL") == 0) {
                        protocol_type = PROTOCOL_SSL;
                    } else if (strcmp(Parameter_List[2].String_Value, "TCPv6") == 0) {
                        if (v6_enable) {
                            protocol_type = PROTOCOL_TCPv6;
                        } else {
                            QAT_Response_Str(QAT_RC_ERROR, "+CIPSERVER:IPv6 is not enable\r\n");
                            return rc;
                        }
                    } else if (strcmp(Parameter_List[2].String_Value, "SSLv6") == 0) {
                        protocol_type = PROTOCOL_SSLv6;
                    } else {
                        protocol_type = PROTOCOL_INVALID;
                        QAT_Response_Str(QAT_RC_ERROR, "+CIPSERVER:protocol_type is invalid\r\n");
                        return rc;
                    }
                }
            }

            if (Parameter_Count >= 4) {
                if (!Parameter_List[3].Integer_Is_Valid || (Parameter_List[3].Integer_Value > 1)) {
                    QAT_Response_Str(QAT_RC_ERROR, "+CIPSERVER:ca enable parameter must be a integer!\r\n");
                    return rc;
                } else {
                    ca_enable = Parameter_List[3].Integer_Value;
                }
            }

            if (Parameter_Count == 5) {
                if (!Parameter_List[4].Integer_Is_Valid) {
                    QAT_Response_Str(QAT_RC_ERROR, "+CIPSERVER:keepalive parameter must be a integer!\r\n");
                    return rc;
                } else {
                    keep_alive = Parameter_List[4].Integer_Value;
                }
            }

            if ((tcpServerThreadCreated == false) && (mode == 1)) {
                if (protocol_type == PROTOCOL_TCP) {
                    tcp_listen_fd = socket(AF_INET, SOCK_STREAM, 0);
                } else if (protocol_type == PROTOCOL_TCPv6) {
                    tcp_listen_fd = socket(AF_INET6, SOCK_STREAM, 0);
                    is_v6 = true;
                } else {
                    ;  // todo SSL
                }

                if (tcp_listen_fd < 0) {
                    QAT_Response_Str(QAT_RC_ERROR, NULL);
                    return rc;
                }

                if (is_v6 && setsockopt(tcp_listen_fd, IPPROTO_IPV6, IPV6_V6ONLY, &opt, sizeof(opt)) < 0) {
                    goto tcp_server_fail;
                }
                if (setsockopt(tcp_listen_fd, SOL_SOCKET, SO_KEEPALIVE, &keep_alive, sizeof(keep_alive)) < 0) {
                    goto tcp_server_fail;
                }

                memset(&local_addr, 0, sizeof(local_addr));
                if (is_v6) {
                    struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)&local_addr;
                    addr6->sin6_len = sizeof(struct sockaddr_in);
                    addr6->sin6_family = AF_INET6;
                    addr6->sin6_addr = in6addr_any;
                    addr6->sin6_port = htons(tcp_config.params.port);
                } else {
                    struct sockaddr_in *addr4 = (struct sockaddr_in *)&local_addr;
                    addr4->sin_len = sizeof(struct sockaddr_in);
                    addr4->sin_family = AF_INET;
                    addr4->sin_addr.s_addr = INADDR_ANY;
                    addr4->sin_port = htons(tcp_config.params.port);
                }

                if (bind(tcp_listen_fd, (struct sockaddr *)&local_addr, sizeof(local_addr)) < 0) {
                    goto tcp_server_fail;
                }

                if (listen(tcp_listen_fd, QAT_CLIENT_MAX_CONNECTIONS) < 0) {
                    goto tcp_server_fail;
                }

                server_cb = CircularBuffer_Create();
                if (server_cb == NULL) {
                    goto tcp_server_fail;
                }

                if (nt_qurt_thread_create(tcp_server_thread, "tcp_server_thread", 1024, &tcp_config, 6, NULL) == 1) {
                    tcpServerThreadCreated = true;
                    rc = QAT_Response_Str(QAT_RC_OK, NULL);
                } else {
                    goto tcp_server_fail;
                }
            } else if (mode == 0) {
                rc = QAT_Response_Str(QAT_RC_OK, NULL);
            } else {
                QAT_Response_Str(QAT_RC_ERROR, "+CIPSERVER:Server has been established\r\n");
                return rc;
            }
            break;
        }
    }
    return rc;
tcp_server_fail:
    closesocket(tcp_listen_fd);
    tcp_listen_fd = INVALID_FD;
    QAT_Response_Str(QAT_RC_ERROR, NULL);
    return rc;
}

int find_invalid_addr(struct sockaddr_storage *from)
{
    for (int i = 0; i < QAT_CLIENT_MAX_CONNECTIONS; i++) {
        if (g_listen_udp_clients[i].active) {
            if (from->ss_family == AF_INET) {
                struct sockaddr_in *from4 = (struct sockaddr_in *)from;
                if ((g_listen_udp_clients[i].addr.v4_addr.sin_addr.s_addr == from4->sin_addr.s_addr) &&
                    (g_listen_udp_clients[i].addr.v4_addr.sin_port == from4->sin_port)) {
                    return i;
                }
            }
            if (from->ss_family == AF_INET6) {
                struct sockaddr_in6 *from6 = (struct sockaddr_in6 *)from;
                if ((memcmp(&g_listen_udp_clients[i].addr.v6_addr.sin6_addr, &from6->sin6_addr,
                            sizeof(struct in6_addr)) == 0) &&
                    (g_listen_udp_clients[i].addr.v6_addr.sin6_port == from6->sin6_port) &&
                    (g_listen_udp_clients[i].addr.v6_addr.sin6_scope_id == from6->sin6_scope_id)) {
                    return i;
                }
            }
        }
    }
    return QAT_ERROR;
}

int add_udp_client(struct sockaddr_storage *from)
{
    for (int i = 0; i < QAT_CLIENT_MAX_CONNECTIONS; i++) {
        if (!g_listen_udp_clients[i].active || (g_listen_udp_clients[i].recv_type == RECVTYPE_ACTIVE)) {
            if (from->ss_family == AF_INET) {
                memcpy(&g_listen_udp_clients[i].addr.v4_addr, (struct sockaddr_in *)from, sizeof(struct sockaddr_in));
            }
            if (from->ss_family == AF_INET6) {
                memcpy(&g_listen_udp_clients[i].addr.v6_addr, (struct sockaddr_in6 *)from, sizeof(struct sockaddr_in6));
                g_listen_udp_clients[i].v6 = true;
            }
            g_listen_udp_clients[i].active = true;
            return i;
        }
    }
    return QAT_ERROR;
}

/**
   @brief Processes the Extend command from the QAT.

   This command will change the current group to its parent. No parameters are
   expected for this command.

   @param[in] Op_Type          The input command type.
   @param[in] Parameter_Count  Number of parameters that were entered into the
                               command line.
   @param[in] Parameter_List   List of parameters entered into the command line.
*/

static void udp_server_thread(void *arg)
{
    server_config *udpConfig = (server_config *)arg;
    int maxfd = 0;
    char buffer[QAT_INPUT_BUFFER_LENGTH] = {0};
    struct sockaddr_storage from_addr;
    socklen_t from_addr_len = sizeof(from_addr);
    char input_buf[QAT_DATA_INPUT_BUFFER_LENGTH];
    int recv_len;
    int client_idx;
    struct timeval timeout;
    fd_set readfds;
    int recv_type;
    int result;
    int offset;
    QueueElem elem;

    do {
        if ((udpConfig->mode == 0) && udpConfig->params.closeServer) {
            if (udp_listen_fd >= 0) {
                closesocket(udp_listen_fd);
                udp_listen_fd = INVALID_FD;
            }
            for (uint8_t i = 0; i < QAT_CLIENT_MAX_CONNECTIONS; i++) {
                CleanupUdpListenClientConnInfo(i);
            }
            CircularBuffer_Destroy(udp_server_cb);
            udp_server_cb = NULL;
            qurt_mutex_lock(&udp_server_mutex);
            udp_server_ipd_message_print_flag = true;
            qurt_mutex_unlock(&udp_server_mutex);
            udpServerThreadCreated = false;
            nt_osal_thread_delete(NULL);
            return;
        } else {
            timeout.tv_sec = TIMEOUT_TV_SEC;
            timeout.tv_usec = TIMEOUT_TV_USEC;
            maxfd = 0;
            FD_ZERO(&readfds);
            if (udp_listen_fd >= 0) {
                FD_SET(udp_listen_fd, &readfds);
                maxfd = udp_listen_fd;
            }

            if ((udp_server_ipd_message_print_flag == true) && (uxQueueMessagesWaiting(udp_server_queue) > 0) &&
                (xQueuePeek(udp_server_queue, &elem, 0) == pdTRUE)) {
                memset((void *)buffer, 0, QAT_INPUT_BUFFER_LENGTH);
                if (g_listen_udp_clients[elem.id].recv_type == RECVTYPE_PASSIVE) {
                    snprintf(buffer, QAT_INPUT_BUFFER_LENGTH, "+IPD:%c,%s,%d,%d\r\n", 'S', "UDP", elem.id, elem.len);
                    QAT_Response_Str(QAT_RC_QUIET, buffer);
                    qurt_mutex_lock(&udp_server_mutex);
                    udp_server_ipd_message_print_flag = false;
                    qurt_mutex_unlock(&udp_server_mutex);
                }
            }

            if (select(maxfd + 1, &readfds, NULL, NULL, &timeout) > 0) {
                memset((void *)input_buf, 0, QAT_DATA_INPUT_BUFFER_LENGTH);
                recv_len = recvfrom(udp_listen_fd, input_buf, sizeof(input_buf) - 1, 0, (struct sockaddr *)&from_addr,
                                    &from_addr_len);
                if (recv_len < 0) {
                    if (errno == ECONNRESET || errno == ENOTCONN) {
                        CleanupUdpListenClientConnInfo(client_idx);
                    }
                    continue;
                } else if (recv_len == 0) {
                    CleanupUdpListenClientConnInfo(client_idx);
                } else {
                    client_idx = find_invalid_addr(&from_addr);
                    if (client_idx == QAT_ERROR) {
                        if ((udpConfig->mode == 0) && !udpConfig->params.closeServer) {
                            printf("Do not accept new UDP connections.\r\n");
                            continue;
                        }

                        client_idx = add_udp_client(&from_addr);
                        if (client_idx == QAT_ERROR) {
                            printf("Maximum connections reached. Cannot add client.\n");
                            continue;
                        }
                        g_listen_udp_clients[client_idx].active = ACTIVE;
                        g_listen_udp_clients[client_idx].sockfd = udp_listen_fd;
                    }

                    recv_type = g_listen_udp_clients[client_idx].recv_type;
                    if (recv_type == RECVTYPE_ACTIVE) {
                        if (isPassThroughMode) {
                            offset = 0;
                            offset += snprintf(buffer, QAT_INPUT_BUFFER_LENGTH, "+IPDHEX:%c,%s,%d,%d,", 'S', "UDP",
                                               client_idx, recv_len);
                            memcpy(buffer + offset, input_buf, recv_len);
                            QAT_Response_Buffer(QAT_RC_QUIET, buffer, offset + recv_len);
                        } else {
                            input_buf[recv_len] = '\0';
                            memset((void *)buffer, 0, QAT_INPUT_BUFFER_LENGTH);
                            snprintf(buffer, QAT_INPUT_BUFFER_LENGTH, "+IPD:%c,%s,%d,%d,%s", 'S', "UDP", client_idx,
                                     recv_len, input_buf);
                            QAT_Response_Str(QAT_RC_QUIET, buffer);
                        }
                    } else {
                        if (uxQueueSpacesAvailable(udp_server_queue) == 0) {
                            continue;
                        }
                        result = CircularBuffer_GetFreeSpace(udp_server_cb);
                        if (result < recv_len) {
                            continue;
                        }
                        elem.id = client_idx;
                        elem.len = recv_len;
                        if (xQueueSend(udp_server_queue, &elem, 0) == pdPASS) {
                            CircularBuffer_Write(udp_server_cb, input_buf, recv_len);
                        }
                    }
                }
            }
        }
        sys_msleep(200);
    } while (1);
}

static QAT_Command_Status_t Extend_Command_UdpServer(uint32_t Op_Type, uint32_t Parameter_Count,
                                                     QAT_Parameter_t *Parameter_List)
{
    int mode, value;
    QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
    char buf[QAT_CMD_IP_BUFFER_LENGTH] = {0};
    int offset;
    int fd;
    char peer_ip[INET6_ADDRSTRLEN] = {0};
    char local_ip[INET6_ADDRSTRLEN] = {0};
    int peer_port, local_port;
    struct sockaddr_storage local_addr;
    socklen_t local_addr_len = sizeof(local_addr);
    struct netif *netif = NULL;
    int protocol_type = PROTOCOL_UDP;
    bool is_v6 = false;
    switch (Op_Type) {
        case QAT_OP_EXEC: {
            QAT_Response_Str(QAT_RC_QUIET, "+CIPUDPSERVER=<mode>,<param2>,[<\"type\">]]");
            QAT_Response_Str(QAT_RC_QUIET, "mode:\r\n0: Shut down the server\r\n1: Build server");
            QAT_Response_Str(QAT_RC_QUIET,
                             "if mode = 0, param2 can be either 0 or 1:\r\n    param2=0:Maintain existing "
                             "server connections.\r\n    param2=1:Completely shut down the server.");
            QAT_Response_Str(QAT_RC_QUIET, "if mode = 1: param2 is listen port");
            QAT_Response_Str(QAT_RC_OK, NULL);
            break;
        }
        case QAT_OP_QUERY: {
            offset = 0;
            memset((void *)buf, 0, QAT_CMD_IP_BUFFER_LENGTH);
            for (int i = 0; i < QAT_CLIENT_MAX_CONNECTIONS; i++) {
                if (g_listen_udp_clients[i].active == INACTIVE) {
                    continue;
                }

                fd = g_listen_udp_clients[i].sockfd;
                memset(&local_addr, 0, sizeof(local_addr));
                if (getsockname(fd, (struct sockaddr *)&local_addr, &local_addr_len) != 0) {
                    QAT_Response_Str(QAT_RC_ERROR, NULL);
                    return rc;
                }

                if (g_listen_udp_clients[i].v6) {
                    struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)&local_addr;
                    local_port = addr6->sin6_port;
                    inet_ntop(AF_INET6, &addr6->sin6_addr, local_ip, sizeof(local_ip));
                    peer_port = g_listen_udp_clients[i].addr.v6_addr.sin6_port;
                    inet_ntop(AF_INET6, &g_listen_udp_clients[i].addr.v6_addr.sin6_addr, peer_ip, sizeof(peer_ip));
                } else {
                    struct sockaddr_in *addr4 = (struct sockaddr_in *)&local_addr;
                    local_port = addr4->sin_port;
                    inet_ntop(AF_INET, &addr4->sin_addr, local_ip, sizeof(local_ip));
                    peer_port = g_listen_udp_clients[i].addr.v4_addr.sin_port;
                    inet_ntop(AF_INET, &g_listen_udp_clients[i].addr.v4_addr.sin_addr, peer_ip, sizeof(peer_ip));
                }

                offset += snprintf(buf + offset, QAT_CMD_IP_BUFFER_LENGTH, "+CIPUDPSERVER:");
                offset += snprintf(buf + offset, QAT_CMD_IP_BUFFER_LENGTH, "%c,", 'S');
                if (g_listen_udp_clients[i].v6) {
                    offset += snprintf(buf + offset, QAT_CMD_IP_BUFFER_LENGTH, "%d,%s,", i, "UDPv6");
                } else {
                    offset += snprintf(buf + offset, QAT_CMD_IP_BUFFER_LENGTH, "%d,%s,", i, "UDP");
                }
                offset += snprintf(buf + offset, QAT_CMD_IP_BUFFER_LENGTH, "%s,%d,", peer_ip, ntohs(peer_port));
                offset += snprintf(buf + offset, QAT_CMD_IP_BUFFER_LENGTH, "%s,%d\r\n", local_ip, ntohs(local_port));
            }
            rc = QAT_Response_Str(QAT_RC_OK, buf);
            break;
        }
        case QAT_OP_EXEC_W_PARAM: {
            if (Parameter_Count < 2 || Parameter_Count > 3 || !Parameter_List || !Parameter_List[0].Integer_Is_Valid ||
                !Parameter_List[1].Integer_Is_Valid) {
                QAT_Response_Str(QAT_RC_ERROR, "+CIPUDPSERVER:Invalid input parameter!\r\n");
                return rc;
            }

            mode = Parameter_List[0].Integer_Value;
            value = Parameter_List[1].Integer_Value;
            switch (mode) {
                case 0: {
                    if (value < 0 || value > 1) {
                        QAT_Response_Str(QAT_RC_ERROR, "+CIPUDPSERVER:when mode = 0, param2 can only be 0 or 1\r\n");
                        return rc;
                    }
                    udp_config.params.closeServer = value;
                    break;
                }
                case 1: {
                    if (value < 0 || value > 65535) {
                        QAT_Response_Str(QAT_RC_ERROR, "+CIPUDPSERVER:The port number must not exceed 65535.\r\n");
                        return rc;
                    }
                    udp_config.params.port = value;
                    break;
                }
                default: {
                    QAT_Response_Str(QAT_RC_ERROR, "+CIPUDPSERVER:mode can only be 0 or 1\r\n");
                    return rc;
                }
            }
            udp_config.mode = mode;

            if (Parameter_Count >= 3) {
                if (Parameter_List[2].Integer_Is_Valid) {
                    QAT_Response_Str(QAT_RC_ERROR, "+CIPUDPSERVER:Protocol type parameter must be a string!\r\n");
                    return rc;
                } else {
                    if (strcmp(Parameter_List[2].String_Value, "UDP") == 0) {
                        protocol_type = PROTOCOL_UDP;
                    } else if (strcmp(Parameter_List[2].String_Value, "UDPv6") == 0) {
                        if (v6_enable) {
                            protocol_type = PROTOCOL_UDPv6;
                        } else {
                            QAT_Response_Str(QAT_RC_ERROR, "+CIPUDPSERVER:IPv6 is not enable\r\n");
                            return rc;
                        }
                    } else {
                        protocol_type = PROTOCOL_INVALID;
                        QAT_Response_Str(QAT_RC_ERROR, "+CIPUDPSERVER:protocol_type is invalid\r\n");
                        return rc;
                    }
                }
            }

            if ((udpServerThreadCreated == false) && (mode == 1)) {
                if (protocol_type == PROTOCOL_UDP) {
                    udp_listen_fd = socket(AF_INET, SOCK_DGRAM, 0);
                } else if (protocol_type == PROTOCOL_UDPv6) {
                    udp_listen_fd = socket(AF_INET6, SOCK_DGRAM, 0);
                    is_v6 = true;
                } else {
                    QAT_Response_Str(QAT_RC_ERROR, "+CIPUDPSERVER:protocol_type is invalid\r\n");
                }

                if (udp_listen_fd < 0) {
                    QAT_Response_Str(QAT_RC_ERROR, NULL);
                    return rc;
                }

                if (get_netif_by_device(AP_DEVICE)) {
                    netif = get_netif_by_device(AP_DEVICE);
                } else if (get_netif_by_device(STA_DEVICE)) {
                    netif = get_netif_by_device(STA_DEVICE);
                } else {
                    QAT_IP_PRINTF("+CIPUDPSERVER:network interface not initialized\r\n");
                    goto udp_server_fail;
                }

                memset(&local_addr, 0, sizeof(local_addr));
                if (is_v6) {
                    ip_addr_t *ip6_local_addr = (ip_addr_t *)netif_ip_addr6(netif, 2);
                    struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)&local_addr;
                    addr6->sin6_len = sizeof(struct sockaddr_in);
                    addr6->sin6_family = AF_INET6;
                    addr6->sin6_port = htons(udp_config.params.port);
                    inet6_addr_from_ip6addr(&addr6->sin6_addr, ip_2_ip6(ip6_local_addr));
                } else {
                    ip_addr_t *ip_local_addr = (ip_addr_t *)netif_ip_addr4(netif);
                    struct sockaddr_in *addr4 = (struct sockaddr_in *)&local_addr;
                    addr4->sin_len = sizeof(struct sockaddr_in);
                    addr4->sin_family = AF_INET;
                    addr4->sin_port = htons(udp_config.params.port);
                    inet_addr_from_ip4addr(&(addr4->sin_addr), ip_2_ip4(ip_local_addr));
                }

                if (bind(udp_listen_fd, (struct sockaddr *)&local_addr, sizeof(local_addr)) < 0) {
                    goto udp_server_fail;
                }

                udp_server_cb = CircularBuffer_Create();
                if (udp_server_cb == NULL) {
                    goto udp_server_fail;
                }

                if (nt_qurt_thread_create(udp_server_thread, "udp_server_thread", 1024, &udp_config, 6, NULL) == 1) {
                    udpServerThreadCreated = true;
                    rc = QAT_Response_Str(QAT_RC_OK, NULL);
                } else {
                    goto udp_server_fail;
                }
            } else if (mode == 0) {
                rc = QAT_Response_Str(QAT_RC_OK, NULL);
            } else {
                QAT_Response_Str(QAT_RC_ERROR, "+CIPUDPSERVER:UDP server has been established");
                return rc;
            }
            break;
        }
    }
    return rc;
udp_server_fail:
    closesocket(udp_listen_fd);
    udp_listen_fd = INVALID_FD;
    QAT_Response_Str(QAT_RC_ERROR, NULL);
    return rc;
}

static QAT_Command_Status_t Extend_Command_EnableV6(uint32_t Op_Type, uint32_t Parameter_Count,
                                                    QAT_Parameter_t *Parameter_List)
{
    QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
    char buf[QAT_CMD_IP_BUFFER_LENGTH] = {0};
    uint8_t value;
    struct netif *netif = NULL;
    switch (Op_Type) {
        case QAT_OP_EXEC: {
            QAT_Response_Str(QAT_RC_OK, "AT+CIPV6=<enable>\r\n");
            break;
        }
        case QAT_OP_QUERY: {
            snprintf(buf, QAT_CMD_IP_BUFFER_LENGTH, "+CIPV6:%d\r\n", v6_enable);
            QAT_Response_Str(QAT_RC_OK, buf);
            break;
        }
        case QAT_OP_EXEC_W_PARAM: {
            if (Parameter_Count != 1 || !Parameter_List || !Parameter_List[0].Integer_Is_Valid) {
                QAT_Response_Str(QAT_RC_ERROR, "+CIPV6:Invalid input parameter!\r\n");
                return rc;
            }
            value = Parameter_List[0].Integer_Value;
            if (value > 1) {
                QAT_Response_Str(QAT_RC_ERROR, "+CIPV6:enable parameter can only be 0 or 1!\r\n");
                return rc;
            }

            if (value) {
                NETIF_FOREACH(netif)
                {
                    if (strncmp(netif->name, "st", 2) == 0) {
                        netif->ip6_autoconfig_enabled = 1;
                        netif_create_ip6_linklocal_address(netif, 1);
                        nd6_restart_netif(netif);
                    } else if (strncmp(netif->name, "ap", 2) == 0) {
                        netif->ip6_autoconfig_enabled = 1;
                        netif_create_ip6_linklocal_address(netif, 1);
                        nd6_restart_netif(netif);
                    } else if (strncmp(netif->name, "lo", 2) == 0) {
                        netif->ip6_addr_state[0] = IP6_ADDR_VALID;
                    }
                }
                v6_enable = 1;
            } else {
                NETIF_FOREACH(netif)
                {
                    if ((strncmp(netif->name, "st", 2) == 0) || (strncmp(netif->name, "ap", 2) == 0)) {
                        netif->ip6_autoconfig_enabled = 0;
                        for (int i = 0; i < LWIP_IPV6_NUM_ADDRESSES; ++i) {
                            ip6_addr_set_zero(&netif->ip6_addr[i].u_addr.ip6);
                            netif_ip6_addr_set_state(netif, i, IP6_ADDR_INVALID);
                        }
                    } else if (strncmp(netif->name, "lo", 2) == 0) {
                        netif->ip6_addr_state[0] = IP6_ADDR_INVALID;
                    }
                }
                v6_enable = 0;
            }
            rc = QAT_Response_Str(QAT_RC_OK, NULL);
            break;
        }
        default: {
            ;
        }
    }
    return rc;
}

static QAT_Command_Status_t Extend_Command_DHCPv4s(uint32_t Op_Type, uint32_t Parameter_Count,
                                                   QAT_Parameter_t *Parameter_List)
{
    QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
    struct netif *netif = NULL;
    char *interface_name = NULL;
    char *action = NULL;
    char *start_ip_addr_string;
    char *end_ip_addr_string;
    struct dhcps_lease lease;
    char buf[QAT_CMD_IP_BUFFER_LENGTH];

    switch (Op_Type) {
        case QAT_OP_EXEC: {
            QAT_Response_Str(
                QAT_RC_OK,
                "AT+CIPDHCPV4S=<interface>,<start|stop|pool>,[<start_ip>],[<end_ip>],[<lease_time_minute>]\r\n");
            return rc;
        }
        case QAT_OP_EXEC_W_PARAM: {
            if ((Parameter_Count < 2) || (Parameter_Count > 5) || !Parameter_List) {
                QAT_Response_Str(QAT_RC_ERROR, "+CIPDHCPV4S:Invalid input parameter!\r\n");
                return rc;
            }

            netif = get_netif_by_device(AP_DEVICE);
            if (!netif) {
                QAT_Response_Str(QAT_RC_ERROR, "+CIPDHCPV4S:SoftAP is not started.\r\n");
                return rc;
            }

            interface_name = Parameter_List[0].String_Value;
            if (strcmp(interface_name, "wlan0") != 0) {
                QAT_Response_Str(QAT_RC_ERROR, "+CIPDHCPV4S:Just wlan0 support DHCP server mode currently!\r\n");
                return rc;
            }

            if (Parameter_Count >= 2) {
                action = Parameter_List[1].String_Value;
                if (strncmp(action, "start", 5) == 0) {
                    if (Parameter_Count > 2) {
                        QAT_Response_Str(QAT_RC_ERROR,
                                         "+CIPDHCPV4S:The start command does not take any parameters.\r\n");
                        return rc;
                    }
                    if (!nt_ap_dhcps_start(netif)) {
                        QAT_Response_Str(QAT_RC_ERROR, "+CIPDHCPV4S:DHCP server start failed!\r\n");
                        return rc;
                    }
                    rc = QAT_Response_Str(QAT_RC_OK, NULL);
                    return rc;
                }
                if (strncmp(action, "stop", 4) == 0) {
                    if (Parameter_Count > 2) {
                        QAT_Response_Str(QAT_RC_ERROR,
                                         "+CIPDHCPV4S:The stop command does not take any parameters.\r\n");
                        return rc;
                    }
                    if (nt_dhcps_netif_status(netif) == DHCP_STARTED) {
                        if (!nt_ap_dhcps_stop(netif)) {
                            QAT_Response_Str(QAT_RC_ERROR, "+CIPDHCPV4S:DHCP server stop failed!\r\n");
                            return rc;
                        }
                        QAT_IP_PRINTF("DHCP server stop success \r\n");
                        rc = QAT_Response_Str(QAT_RC_OK, NULL);
                        return rc;
                    } else {
                        QAT_Response_Str(QAT_RC_ERROR, "+CIPDHCPV4S:DHCP server not start!\r\n");
                        return rc;
                    }
                }
                if (strncmp(action, "pool", 4) == 0) {
                    if (Parameter_Count < 4) {
                        QAT_Response_Str(QAT_RC_ERROR,
                                         "+CIPDHCPV4S:The pool command must be followed by two parameters: "
                                         "<start_ip> and <end_ip>.\r\n");
                        return rc;
                    }
                    start_ip_addr_string = Parameter_List[2].String_Value;
                    end_ip_addr_string = Parameter_List[3].String_Value;

                    if (!ipaddr_aton(start_ip_addr_string, &(lease.start_ip))) {
                        QAT_Response_Str(QAT_RC_ERROR, "+CIPDHCPV4S:Please try again invalid IP Address.\r\n");
                        return rc;
                    }

                    if (!ipaddr_aton(end_ip_addr_string, &(lease.end_ip))) {
                        QAT_Response_Str(QAT_RC_ERROR, "+CIPDHCPV4S:Please try again invalid IP Address.\r\n");
                        return rc;
                    }

                    lease.lease_time = 0;
                    if (Parameter_Count == 5 && Parameter_List[4].Integer_Is_Valid) {
                        lease.lease_time = Parameter_List[4].Integer_Value;
                    }
                    lease.enable = TRUE;
                    if (!nt_set_dhcps_lease(&lease)) {
                        QAT_Response_Str(QAT_RC_ERROR, "+CIPDHCPV4S:Configure pool address fail.\r\n");
                        return rc;
                    }
                    rc = QAT_Response_Str(QAT_RC_OK, NULL);
                    return rc;
                } else {
                    snprintf(buf, QAT_CMD_IP_BUFFER_LENGTH, "+CIPDHCPV4S:Invalid command: %s\r\n", action);
                    QAT_Response_Str(QAT_RC_ERROR, buf);
                    return rc;
                }
            }
        }
    }
}

static QAT_Command_Status_t Extend_Command_Mode(uint32_t Op_Type, uint32_t Parameter_Count,
                                                QAT_Parameter_t *Parameter_List)
{
    QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
    char buf[QAT_CMD_IP_BUFFER_LENGTH] = {0};
    uint8_t value;
    switch (Op_Type) {
        case QAT_OP_EXEC: {
            QAT_Response_Str(QAT_RC_OK, "AT+CIPMODE=<mode>");
            break;
        }
        case QAT_OP_QUERY: {
            snprintf(buf, QAT_CMD_IP_BUFFER_LENGTH, "+CIPMODE:%d", isPassThroughMode);
            QAT_Response_Str(QAT_RC_OK, buf);
            break;
        }
        case QAT_OP_EXEC_W_PARAM: {
            if (Parameter_Count != 1 || !Parameter_List || !Parameter_List[0].Integer_Is_Valid) {
                QAT_Response_Str(QAT_RC_ERROR, "+CIPMODE:Invalid input parameter!\r\n");
                return rc;
            }
            value = Parameter_List[0].Integer_Value;
            if (value > 1) {
                QAT_Response_Str(QAT_RC_ERROR, "+CIPMODE:mode parameter can only be 0 or 1!\r\n");
                return rc;
            }

            if (value) {
                isPassThroughMode = 1;
            } else {
                isPassThroughMode = 0;
            }
            rc = QAT_Response_Str(QAT_RC_OK, NULL);
            break;
        }
        default: {
            ;
        }
    }
    return rc;
}

#if LWIP_IPV6
static int is_valid_prefix(const char *str);

static QAT_Command_Status_t Extend_Command_IPV6Prefix(uint32_t Op_Type, uint32_t Parameter_Count,
                                                      QAT_Parameter_t *Parameter_List)
{
    QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
    char buf[QAT_CMD_IP_BUFFER_LENGTH] = {0};
    char *ptr = NULL;
    int len = 0;
    char ip_str[40];
    ip6_addr_t addr;

    struct netif *netif = NULL;

    netif = get_netif_by_device(AP_DEVICE);
    if (NULL == netif) {
        QAT_IP_PRINTF("+CIPV6PREFIX: AP network interface not initialized\r\n");
        return rc;
    }

    switch (Op_Type) {
        case QAT_OP_EXEC: {
            QAT_Response_Str(QAT_RC_OK, "AT+CIPV6PREFIX=<prefix>");
            break;
        }
        case QAT_OP_QUERY: {
            snprintf(buf, QAT_CMD_IP_BUFFER_LENGTH, "+CIPV6PREFIX:%s", ipv6_prefix);
            QAT_Response_Str(QAT_RC_OK, buf);
            break;
        }
        case QAT_OP_EXEC_W_PARAM: {
            if (Parameter_Count != 1 || !Parameter_List) {
                QAT_Response_Str(QAT_RC_ERROR, "+CIPV6PREFIX:Invalid input parameter!\r\n");
                return rc;
            }
            ptr = Parameter_List[0].String_Value;
            if (!is_valid_prefix(ptr)) {
                QAT_Response_Str(QAT_RC_ERROR, "+CIPV6PREFIX:IPv6 Prefix format error\r\n");
                return rc;
            }
            len = strlen(ptr);
            if (len == 0 || len > QAT_IPV6PREFIX_LEN_MAX) {
                QAT_Response_Str(QAT_RC_ERROR, "+CIPV6PREFIX:IPv6 Prefix format error\r\n");
                return rc;
            } else {
                // try to check whether it is a valid prefix
                memset(ip_str, 0, sizeof(ip_str));
                strlcpy(ip_str, ptr, sizeof(ip_str));
                strlcat(ip_str, "::1", sizeof(ip_str));
                memset(&addr, 0, sizeof(ip6_addr_t));
                if (ip6addr_aton(ip_str, &addr) == 0) {
                    QAT_Response_Str(QAT_RC_ERROR, "+CIPV6PREFIX:IPv6 Prefix format error\r\n");
                    return rc;
                } else {
                    // check global or not
                    if ((addr.addr[0] & PP_HTONL(0xe0000000)) == PP_HTONL(0x20000000)) {
                        // config netif and save the prefix
                        netif_ip6_addr_set(netif, 1, &addr);
                        netif->ip6_autoconfig_enabled = 1;
                        netif_ip6_addr_set_state(netif, 1, IP6_ADDR_TENTATIVE);
                        strlcpy(ipv6_prefix, ptr, sizeof(ipv6_prefix));
                        rc = QAT_Response_Str(QAT_RC_OK, NULL);
                        break;
                    } else {
                        QAT_Response_Str(QAT_RC_ERROR, "+CIPV6PREFIX:IPv6 Prefix format error\r\n");
                        return rc;
                    }
                }
            }
        }
    }
    return rc;
}

static int is_valid_prefix(const char *str)
{
    if (strlen(str) > 19) {
        return 0;
    }

    int colon_count = 0;
    int is_valid = 1;
    int last_colon_pos = -1;

    if (str[0] == ':' || str[strlen(str) - 1] == ':') {
        return 0;
    }

    for (int i = 0; str[i] != '\0'; i++) {
        char c = str[i];
        if (c == ':') {
            colon_count++;
            if (last_colon_pos == i - 1) {
                is_valid = 0;
                break;
            }
            last_colon_pos = i;

        } else {
            if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) {
                is_valid = 0;
                break;
            }
        }
    }

    if (colon_count != 3) {
        return 0;
    }

    return is_valid;
}
#endif

void Initialize_QAT_TCPIP_Demo(void)
{
    qbool_t RetVal;
    for (int link_id = 0; link_id < QAT_CLIENT_MAX_CONNECTIONS; link_id++) {
        CleanupClientConnInfo(link_id);
        CleanupListenClientConnInfo(link_id);
        CleanupUdpListenClientConnInfo(link_id);
    }

    qurt_mutex_create(&client_mutex);
    qurt_mutex_create(&server_mutex);
    qurt_mutex_create(&udp_server_mutex);

    client_queue = xQueueCreate(CONFIG_QAT_CB_QUEUE_MAX_LENGTH, sizeof(QueueElem));
    server_queue = xQueueCreate(CONFIG_QAT_CB_QUEUE_MAX_LENGTH, sizeof(QueueElem));
    udp_server_queue = xQueueCreate(CONFIG_QAT_CB_QUEUE_MAX_LENGTH, sizeof(QueueElem));

    RetVal = QAT_Register_Command_Group(QAT_TCPIP_Command_List, TCPIP_COMMAND_LIST_SIZE);
    if (RetVal == false) {
        QAT_IP_PRINTF("Failed to register tcpip command group.\r\n");
    }
}
