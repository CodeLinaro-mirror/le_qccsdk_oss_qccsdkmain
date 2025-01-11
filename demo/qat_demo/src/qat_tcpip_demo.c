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
/*-------------------------------------------------------------------------
 * Function Declarations
 *-----------------------------------------------------------------------*/
static QAT_Command_Status_t Extend_Command_Ping(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_DHCPv4c(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_SetStation(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_Start(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_Close(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_Send(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_SendData(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_RecvType(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_RecvData(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_Server(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List);
static QAT_Command_Status_t Extend_Command_UdpServer(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List);


/* The following is the complete command list for the QAT common command demo. */
/** List of global commands that are supported when in a group. */
static QAT_Command_t QAT_TCPIP_Command_List[] =
{
	{"+CIPPING",        Extend_Command_Ping,            QAT_OP_EXEC | QAT_OP_EXEC_W_PARAM},
    {"+CIPDHCPV4C",     Extend_Command_DHCPv4c,         QAT_OP_EXEC | QAT_OP_EXEC_W_PARAM},
    {"+CIPSTA",         Extend_Command_SetStation,      QAT_OP_EXEC | QAT_OP_QUERY | QAT_OP_EXEC_W_PARAM},
    {"+CIPSTART",       Extend_Command_Start,           QAT_OP_EXEC | QAT_OP_QUERY | QAT_OP_EXEC_W_PARAM},
    {"+CIPCLOSE",       Extend_Command_Close,           QAT_OP_EXEC | QAT_OP_EXEC_W_PARAM},
    {"+CIPSENDDATA",    Extend_Command_SendData,        QAT_OP_EXEC | QAT_OP_EXEC_W_PARAM},
    {"+CIPSEND",        Extend_Command_Send,            QAT_OP_EXEC | QAT_OP_EXEC_W_PARAM},
    {"+CIPRECVTYPE",    Extend_Command_RecvType,        QAT_OP_EXEC | QAT_OP_QUERY | QAT_OP_EXEC_W_PARAM},
    {"+CIPRECVDATA",    Extend_Command_RecvData,        QAT_OP_EXEC | QAT_OP_EXEC_W_PARAM},
    {"+CIPSERVER",      Extend_Command_Server,          QAT_OP_EXEC | QAT_OP_QUERY | QAT_OP_EXEC_W_PARAM},
    {"+CIPUDPSERVER",   Extend_Command_UdpServer,       QAT_OP_EXEC | QAT_OP_QUERY | QAT_OP_EXEC_W_PARAM},
};

/*-------------------------------------------------------------------------
 * Parameters define
 *-----------------------------------------------------------------------*/
#define TCPIP_COMMAND_LIST_SIZE                     (sizeof(QAT_TCPIP_Command_List) / sizeof(QAT_Command_t))

#define QAT_PING_DEFAULT_DELAY_MS                   500
#define QAT_PING_DEFAULT_COUNT                      4
#define QAT_PING_DEFAULT_PACKET_SIZE                32
#define QAT_PING_RCV_TIME                           1000
#define QAT_PING_RECV_BUFFER_SIZE                   1500
#define QAT_TIME_SEC_TO_MS                          1000

#define QAT_DNS_SERVER_INDEX0                       0
#define QAT_DNS_SERVER_INDEX1                       1

#define QAT_CLIENT_MAX_CONNECTIONS                  4
#define INVALID_FD                                  -1
#define DATA_MAX_SEND_COUNT                         5

#define QAT_CFG_PING_MAX_TX                         1470
#define QAT_CMD_IP_BUFFER_LENGTH					512
#define QAT_INPUT_BUFFER_LENGTH                     1400
#define QAT_DATA_INPUT_BUFFER_LENGTH                1370
#define TIMEOUT_TV_SEC					            1
#define TIMEOUT_TV_USEC					            0
#define INVALID_LINKID					            -1
#define QAT_IP_PRINTF(...) printf(__VA_ARGS__)
#define MY_MAX_PORT                                 65535

/** ping identifier - must fit on a u16_t */
#ifndef QAT_PING_ID
#define QAT_PING_ID        0xACAB
#endif

#define QAT_OK                                      0
#define QAT_ERROR                                   -1

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
static QueueHandle_t client_queue = NULL;
qurt_mutex_t client_mutex;
client_ctx_t g_client_conns_t[QAT_CLIENT_MAX_CONNECTIONS];

server_config tcp_config;
bool tcpServerThreadCreated  = false;
server_config udp_config;
bool udpServerThreadCreated  = false;

static QueueHandle_t server_queue = NULL;
qurt_mutex_t server_mutex;
server_ctx_t g_listen_clients[QAT_CLIENT_MAX_CONNECTIONS] = {0};
CircularBuffer *server_cb;

static QueueHandle_t udp_server_queue = NULL;
qurt_mutex_t udp_server_mutex;
server_ctx_t g_listen_udp_clients[QAT_CLIENT_MAX_CONNECTIONS] = {0};
CircularBuffer *udp_server_cb;

bool ipd_message_print_flag = true;
bool server_ipd_message_print_flag = true;
bool udp_server_ipd_message_print_flag = true;

static int tcp_listen_fd = INVALID_FD;
static int udp_listen_fd = INVALID_FD;
/*-------------------------------------------------------------------------
 * Function Definitions
 *-----------------------------------------------------------------------*/
void qat_ping_prepare_echo(icmp_echo_hdr *icmp_hdr)
{
    ICMPH_TYPE_SET(icmp_hdr, ICMP_ECHO);
    ICMPH_CODE_SET(icmp_hdr, 0);
    icmp_hdr->chksum = 0;
    icmp_hdr->id     = QAT_PING_ID;
    icmp_hdr->seqno  = htons(++qat_ping_seq_num);

    for(int i = sizeof(icmp_echo_hdr); i < ping_size; i++)
    {
        ((char *)icmp_hdr)[i] = 0;
    }

    icmp_hdr->chksum = inet_chksum(icmp_hdr, ping_size);
}

static err_t qat_ping_send(int s, const ip_addr_t *addr)
{
    int err;
    struct sockaddr_in foreign_addr;
    struct sockaddr *to;
    icmp_echo_hdr *icmp_hdr;

    icmp_hdr = (struct icmp_echo_hdr *)mem_malloc((mem_size_t)ping_size);
    if (!icmp_hdr) {
        return ERR_VAL;
    }
    qat_ping_prepare_echo(icmp_hdr);

    if(IP_IS_V4(addr)) {
        foreign_addr.sin_len    = sizeof(foreign_addr);
        foreign_addr.sin_family = AF_INET;
        inet_addr_from_ip4addr(&foreign_addr.sin_addr, ip_2_ip4(addr));
        to = (struct sockaddr *)&foreign_addr;
    }

    err = sendto(s, icmp_hdr, ping_size, 0, to, sizeof(struct sockaddr));
    mem_free(icmp_hdr);
    return (err < 0 ? ERR_VAL : ERR_OK);
}

static void qat_ping_recv(int s, char *buffer, char *buf)
{
    int ret;
    int recv_len;
    struct sockaddr_in src_addr;
    socklen_t src_addr_len = sizeof(src_addr);
    ip_addr_t from_addr;
    struct timeval timeout;

    timeout.tv_sec = QAT_PING_RCV_TIME / 1000;
    timeout.tv_usec = 0;
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(s, &fds);
    do{
        ret = select(s + 1, &fds, NULL, NULL, &timeout);
        if (ret > 0) {
            recv_len = recvfrom(s, buf, QAT_PING_RECV_BUFFER_SIZE, 0, (struct sockaddr*)&src_addr, &src_addr_len);
            if(recv_len >= (int)(sizeof(struct ip_hdr)+sizeof(struct icmp_echo_hdr))) {
                inet_addr_to_ip4addr(ip_2_ip4(&from_addr), &src_addr.sin_addr);
                IP_SET_TYPE_VAL(from_addr, IPADDR_TYPE_V4);
                struct ip_hdr * ip_header = (struct ip_hdr *)buf;
                struct icmp_echo_hdr *icmp_header = (struct icmp_echo_hdr *)(buf + (IPH_HL(ip_header) * 4));
                
                if ((icmp_header->id == QAT_PING_ID) && (icmp_header->seqno == htons(qat_ping_seq_num))) 
                {
                    if(ICMPH_TYPE(icmp_header) != ICMP_ER){
                        continue;
                    }
                    qat_ping_recv_count++;
                    memset((void*)buffer, 0, QAT_CMD_IP_BUFFER_LENGTH);
                    snprintf(buffer, QAT_CMD_IP_BUFFER_LENGTH, "+CIPPING:%s,%u,%lu\r\n", ipaddr_ntoa(&from_addr), ntohs(icmp_header->seqno), (sys_now()-qat_ping_time));
                    QAT_Response_Str(QAT_RC_QUIET, buffer);
                    return;
                }
            }
        }
        memset((void*)buffer, 0, QAT_CMD_IP_BUFFER_LENGTH);
        snprintf(buffer, QAT_CMD_IP_BUFFER_LENGTH, "+CIPPING:Request timed out!\r");
        QAT_Response_Str(QAT_RC_QUIET, buffer);
        return;
    }while(1);
}

/*-------------------------------------------------------------------------
 * Function Definitions
 *-----------------------------------------------------------------------*/

static int qat_ping_process(int s, const ip_addr_t *addr)
{
    int ret = QAT_ERROR;
    char buffer[QAT_CMD_IP_BUFFER_LENGTH] = {0};
    char recv_buf[QAT_PING_RECV_BUFFER_SIZE] = {0};

    // snprintf(buffer, QAT_CMD_IP_BUFFER_LENGTH, "Pinging %s with %d bytes of data:\r", ipaddr_ntoa(addr), ping_size);
    // QAT_Response_Str(QAT_RC_QUIET, buffer);
    for(int i = 0; i < ping_count; i++)
    {
        if (qat_ping_send(s, addr) == ERR_OK) {
            qat_ping_time = sys_now();
            qat_ping_sent_count++;
            qat_ping_recv(s, buffer, recv_buf);
            sys_msleep(ping_delay);
        } 
        else {
            memset((void*)buffer, 0, QAT_CMD_IP_BUFFER_LENGTH);
            snprintf(buffer, QAT_CMD_IP_BUFFER_LENGTH, "+CIPPING:ping send %s - error \r", ipaddr_ntoa(addr));
            QAT_Response_Str(QAT_RC_QUIET, buffer);
        }
    }

    memset((void*)buffer, 0, QAT_CMD_IP_BUFFER_LENGTH);
    snprintf(buffer, QAT_CMD_IP_BUFFER_LENGTH, "+CIPPING:%d,%d\r\n", qat_ping_sent_count, qat_ping_recv_count);
    QAT_Response_Str(QAT_RC_QUIET, buffer);
    if(qat_ping_recv_count > 0){
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
static QAT_Command_Status_t Extend_Command_Ping(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List)
{
    char *ptr = NULL;
    int s;
    ip_addr_t ip_addr;
    QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
    qat_ping_seq_num = 0;
    qat_ping_sent_count = 0;
    qat_ping_recv_count = 0;
    char buf[QAT_CMD_IP_BUFFER_LENGTH] = {0};
    switch (Op_Type)
    {
        case QAT_OP_EXEC:
        {
            QAT_Response_Str(QAT_RC_OK, "+CIPPING=<host>[,<count>[,<delay>[,<package size>]]]\r\n");
            break;
        }
        case QAT_OP_EXEC_W_PARAM:
        {
            if( Parameter_Count > 4 || Parameter_Count < 1 || !Parameter_List) {
                QAT_Response_Str(QAT_RC_ERROR, "+CIPPING:Invalid input parameter!\r\n");
                return rc;
            }

            ping_count = QAT_PING_DEFAULT_COUNT;
            ping_delay = QAT_PING_DEFAULT_DELAY_MS;
            ping_size = QAT_PING_DEFAULT_PACKET_SIZE;

            if(Parameter_List[0].Integer_Is_Valid){
                QAT_Response_Str(QAT_RC_ERROR, "+CIPPING:The host is not a valid IP address!\r\n");
                return rc;
            }
            ptr = Parameter_List[0].String_Value;
            if(!ipaddr_aton(ptr, &ip_addr)){
                QAT_Response_Str(QAT_RC_ERROR, "+CIPPING:Invalid ip address!\r\n");
                return rc;
            }

            if(Parameter_Count >= 2){
                if(!Parameter_List[1].Integer_Is_Valid){
                    QAT_Response_Str(QAT_RC_ERROR, "+CIPPING:the type of count must be a Integer!\r\n");
                    return rc;
                }else{
                    ping_count = Parameter_List[1].Integer_Value;
                }
            }

            if(Parameter_Count >= 3){
                if(!Parameter_List[2].Integer_Is_Valid){
                    QAT_Response_Str(QAT_RC_ERROR, "+CIPPING:the type of delay must be a Integer!\r\n");
                    return rc;
                }else{
                    ping_delay = Parameter_List[2].Integer_Value;
                }
            }

            if(Parameter_Count == 4){
                if(!Parameter_List[3].Integer_Is_Valid){
                    QAT_Response_Str(QAT_RC_ERROR, "+CIPPING:the type of package size must be a Integer!\r\n");
                    return rc;
                }else{
                    ping_size = Parameter_List[3].Integer_Value;
                }
            }

            if(ping_size > QAT_CFG_PING_MAX_TX){
                snprintf(buf, QAT_CMD_IP_BUFFER_LENGTH, "Size should be <= %d\r\n", QAT_CFG_PING_MAX_TX);
                QAT_Response_Str(QAT_RC_ERROR, buf);
                return rc;
            }
            
            s = socket(AF_INET, SOCK_RAW, IP_PROTO_ICMP);
            if (s < 0) {
                QAT_Response_Str(QAT_RC_ERROR, NULL);
                return rc;
            }
            
            if(qat_ping_process(s, &ip_addr) == QAT_OK){
                rc = QAT_Response_Str(QAT_RC_OK, NULL);
            }
            else{
                QAT_Response_Str(QAT_RC_ERROR, NULL);
            }
            closesocket(s);
            break;
        }
    }
    return rc;
}

static struct netif *get_netif_by_device(int devid)
{
    uint8_t netid;
    struct netif *netif;

    if (devid <= AP_DEVICE) {
        NETIF_FOREACH(netif) {
            if (devid == ((device_t *)netif->state)->role) {
                netid = netif->num+1;   /* found! */
            }
        }
    }
    return netif_get_by_index(netid);
}

static void qat_net_show_info(struct netif *netif, char *buffer , int *p_offset)
{
    ip_addr_t *ip_addr = (ip_addr_t *)netif_ip_addr4(netif);
    ip_addr_t *netmask = (ip_addr_t *)netif_ip_netmask4(netif);
    ip_addr_t *gw = (ip_addr_t *)netif_ip_gw4(netif);
    ip_addr_t *dns1 = (ip_addr_t *)dns_getserver(QAT_DNS_SERVER_INDEX0);
    ip_addr_t *dns2 = (ip_addr_t *)dns_getserver(QAT_DNS_SERVER_INDEX1);

    *p_offset += snprintf(buffer + *p_offset, QAT_CMD_IP_BUFFER_LENGTH, "%s,", ipaddr_ntoa(ip_addr));
    *p_offset += snprintf(buffer + *p_offset, QAT_CMD_IP_BUFFER_LENGTH, "%s,", ipaddr_ntoa(gw));
    *p_offset += snprintf(buffer + *p_offset, QAT_CMD_IP_BUFFER_LENGTH, "%s,", ipaddr_ntoa(netmask));
    *p_offset += snprintf(buffer + *p_offset, QAT_CMD_IP_BUFFER_LENGTH, "%s,", ipaddr_ntoa(dns1));
    *p_offset += snprintf(buffer + *p_offset, QAT_CMD_IP_BUFFER_LENGTH, "%s", ipaddr_ntoa(dns2));
}

static bool isDhcpSucceed(struct netif *netif)
{
    struct dhcp *dhcp = NULL;

    dhcp = netif_dhcp_data(netif);
    while(dhcp->state != DHCP_STATE_BOUND)
    {
        qurt_thread_sleep(200);
    }
    return TRUE;
}

static bool isDhcpReleased(struct netif *netif)
{
    struct dhcp *dhcp = NULL;

    dhcp = netif_dhcp_data(netif);
    while(dhcp->state != DHCP_STATE_OFF)
    {
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
static QAT_Command_Status_t Extend_Command_DHCPv4c(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List)
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
    ip_addr_t default_dns = IPADDR4_INIT_BYTES(8,8,8,8);

    switch (Op_Type)
    {
        case QAT_OP_EXEC:
        {
            QAT_Response_Str(QAT_RC_OK, "+CIPDHCPV4C=<interface>,<new|release>\r\n");
            break;
        }
        case QAT_OP_EXEC_W_PARAM:
        {
            if( (Parameter_Count != 2) || !Parameter_List || Parameter_List[0].Integer_Is_Valid || Parameter_List[1].Integer_Is_Valid) 
            {
                QAT_Response_Str(QAT_RC_ERROR, "+CIPDHCPV4C:Invalid input parameter!\r\n");
                return rc;
            }

            netif = get_netif_by_device(STA_DEVICE);
            if(netif == NULL){
                QAT_IP_PRINTF("netif is NULL!\r\n");
                QAT_Response_Str(QAT_RC_ERROR, NULL);
                return rc;
            }

            interface_name = Parameter_List[0].String_Value;
            if(strcmp(interface_name, "wlan1") != 0 ) {
                QAT_Response_Str(QAT_RC_ERROR, "+CIPDHCPV4C:Just wlan1 support DHCP client mode currently\r\n");
                return rc;
            }

            action = Parameter_List[1].String_Value;
            if((!memcmp(action,"new",3))) {
                netif_set_addr(netif, IP4_ADDR_ANY4, IP4_ADDR_ANY4, IP4_ADDR_ANY4);
                etharp_cleanup_netif(netif);
                status = dhcp_start(netif);
                if( status != ERR_OK){
                    QAT_Response_Str(QAT_RC_ERROR, "+CIPDHCPV4C:DHCP client start failed\n");
                    return rc;
                }

                if(isDhcpSucceed(netif)){
                    offset += snprintf(buffer + offset, QAT_CMD_IP_BUFFER_LENGTH, "+CIPDHCPV4C:");
                    qat_net_show_info(netif, buffer, &offset);
                }
       
                rc = QAT_Response_Str(QAT_RC_OK, buffer);           
            }
            else if(!memcmp(action,"release",7)) {
                status = dhcp_release(netif);
                if(status != ERR_OK){
                    QAT_Response_Str(QAT_RC_ERROR, "+CIPDHCPV4C:DHCP client release failed\n");
                    return rc;
                }
                dhcp_stop(netif);
                netif_set_addr(netif, IP4_ADDR_ANY4, IP4_ADDR_ANY4, IP4_ADDR_ANY4);
                dns_setserver(QAT_DNS_SERVER_INDEX0, &default_dns);
                dns_setserver(QAT_DNS_SERVER_INDEX1, &default_dns);
                if(isDhcpReleased(netif)){
                    offset += snprintf(buffer + offset, QAT_CMD_IP_BUFFER_LENGTH, "+CIPDHCPV4C:");
                    qat_net_show_info(netif, buffer, &offset);
                }
                rc = QAT_Response_Str(QAT_RC_OK, buffer);
            }
            else {
                QAT_Response_Str(QAT_RC_ERROR, "+CIPDHCPV4C:DHCP Command Failed due to invalid option i.e. start/release.\r\n\r\n");
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
static QAT_Command_Status_t Extend_Command_SetStation(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List)
{
    ip_addr_t ip_addr, netmask, gw, dns1, dns2;
    struct netif* netif = NULL;
    QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
    char buffer[QAT_CMD_IP_BUFFER_LENGTH]= {0};
    int offset;

    switch (Op_Type)
    {
        case QAT_OP_EXEC:
        {
            QAT_Response_Str(QAT_RC_OK, "+CIPSTA=<ip address>,<gw>,<netmask>,<dns1>,<dns2>\r\n");
            break;
        }
        case QAT_OP_QUERY:
        {
            offset = 0;
            memset((void*)buffer, 0, QAT_CMD_IP_BUFFER_LENGTH);
            offset += snprintf(buffer + offset, QAT_CMD_IP_BUFFER_LENGTH, "+CIPSTA:");
            NETIF_FOREACH(netif) {
                qat_net_show_info(netif, buffer, &offset);
                if(netif->next != NULL){
                    offset += snprintf(buffer + offset, QAT_CMD_IP_BUFFER_LENGTH, ",");
                }
            }
    
            rc = QAT_Response_Str(QAT_RC_OK, buffer);
            break;
        }

        case QAT_OP_EXEC_W_PARAM: 
        {
            offset = 0;
            memset((void*)buffer, 0, QAT_CMD_IP_BUFFER_LENGTH);
            if( Parameter_Count != 5 || !Parameter_List || Parameter_List[0].Integer_Is_Valid ||
                    Parameter_List[1].Integer_Is_Valid || Parameter_List[2].Integer_Is_Valid ||
                    Parameter_List[3].Integer_Is_Valid || Parameter_List[4].Integer_Is_Valid) 
            {
                QAT_IP_PRINTF("+CIPSTA:Invalid input parameter!\r\n");
                goto fail;
            }

            if ((!ipaddr_aton(Parameter_List[0].String_Value, &ip_addr))|| (!ipaddr_aton(Parameter_List[1].String_Value, &gw)
                || (!ipaddr_aton(Parameter_List[2].String_Value, &netmask)) || (!ip_addr_netmask_valid(ip_2_ip4(&netmask))) )
                || (!ipaddr_aton(Parameter_List[3].String_Value, &dns1)) || (!ipaddr_aton(Parameter_List[4].String_Value, &dns2)))
            {
                QAT_IP_PRINTF("+CIPSTA:Invalid input parameter!\r\n");
                goto fail;
            }

            netif = get_netif_by_device(STA_DEVICE);
            if(netif == NULL) {
                QAT_IP_PRINTF("+CIPSTA:network interface not initialized\r\n");
                goto fail;
            }

            netif_set_ipaddr(netif, (const ip4_addr_t*)ip_2_ip4(&ip_addr));
            netif_set_netmask(netif, (const ip4_addr_t*)ip_2_ip4(&netmask));
            netif_set_gw(netif, (const ip4_addr_t*)ip_2_ip4(&gw));
            dns_setserver(QAT_DNS_SERVER_INDEX0, &dns1);
            dns_setserver(QAT_DNS_SERVER_INDEX1, &dns2);

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

CircularBuffer *CircularBuffer_Create() {
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

void CircularBuffer_Destroy(CircularBuffer *cb) {
   if (cb != NULL) {
       qurt_mutex_delete(&cb->mutex);
       free(cb);
   }
}

void CircularBuffer_Write(CircularBuffer *cb, const char *data, size_t length) {
   qurt_mutex_lock(&cb->mutex);
   for (size_t i = 0; i < length; i++) {
       cb->buffer[cb->head] = data[i];
       cb->head = (cb->head + 1) % QAT_CIRCULAR_BUFFER_SIZE;
   }
   cb->size += length;
   qurt_mutex_unlock(&cb->mutex);
   return;
}

int CircularBuffer_Read(CircularBuffer *cb, char *data, size_t length) {
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

int CircularBuffer_GetFreeSpace(CircularBuffer *cb) {
   qurt_mutex_lock(&cb->mutex);
   int free_space = QAT_CIRCULAR_BUFFER_SIZE - cb->size;
   qurt_mutex_unlock(&cb->mutex);
   return free_space;
}

static void CleanupClientConnInfo(int link_id) {
    g_client_conns_t[link_id].id = INVALID_LINKID;
    g_client_conns_t[link_id].sockfd = INVALID_FD;
    g_client_conns_t[link_id].protocol_type = PROTOCOL_INVALID;
    g_client_conns_t[link_id].active = INACTIVE;
    g_client_conns_t[link_id].recv_type = RECVTYPE_ACTIVE;
    g_client_conns_t[link_id].thread_quit = false;
    memset(&(g_client_conns_t[link_id].addr), 0, sizeof(struct sockaddr_in));
    g_client_conns_t[link_id].cb = NULL;
}

/*-------------------------------------------------------------------------
 * Function Definitions
 *-----------------------------------------------------------------------*/

static int CreateConnection(int link_id, int protocol_type, ip_addr_t *ip_addr, int port)
{
    int sockfd;
    struct sockaddr_in remote_addr;
    int tos_opt = IP_TOS;
    struct sockaddr_in local_addr;
    struct netif* netif = NULL;

    netif = get_netif_by_device(STA_DEVICE);
    ip_addr_t *ip_local_addr = (ip_addr_t *)netif_ip_addr4(netif);

    remote_addr.sin_len = sizeof(struct sockaddr_in);
    remote_addr.sin_family = AF_INET;
    remote_addr.sin_port = htons(port);
    inet_addr_from_ip4addr(&(remote_addr.sin_addr), ip_2_ip4(ip_addr));

    local_addr.sin_len = sizeof(struct sockaddr_in);
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(0);
    inet_addr_from_ip4addr(&(local_addr.sin_addr), ip_2_ip4(ip_local_addr));

    switch(protocol_type){
        case PROTOCOL_TCP:
        {
            sockfd = socket(AF_INET, SOCK_STREAM, 0);
            if (sockfd < 0) {
                QAT_IP_PRINTF("create socket failed\n");
                return QAT_ERROR;
            }
            break;
        }
        case PROTOCOL_UDP:
        {
            sockfd = socket(AF_INET, SOCK_DGRAM, 0);
            if (sockfd < 0) {
                QAT_IP_PRINTF("Failed to create UDP socket\n");
                return QAT_ERROR;
            }
            break;
        }
    }

    if(bind(sockfd, (struct sockaddr *)&local_addr, sizeof(struct sockaddr_in)) < 0){
        QAT_IP_PRINTF("Failed to bind local addr and port\n");
        closesocket(sockfd);
        return QAT_ERROR;
    }
    if (setsockopt(sockfd, IPPROTO_IP, tos_opt, &tos_opt, sizeof(int)) < 0){
        closesocket(sockfd);
        return QAT_ERROR;
    }
    if (connect(sockfd, (struct sockaddr *)&remote_addr, sizeof(struct sockaddr_in)) < 0) {
        QAT_IP_PRINTF("Failed to connect TCP server\n");
        closesocket(sockfd);
        return QAT_ERROR;
    }

    g_client_conns_t[link_id].id = link_id;
    g_client_conns_t[link_id].sockfd = sockfd;
    g_client_conns_t[link_id].protocol_type = protocol_type;
    g_client_conns_t[link_id].active = ACTIVE;
    g_client_conns_t[link_id].cb = CircularBuffer_Create();

    if(IP_IS_V4(ip_addr)) {
        g_client_conns_t[link_id].addr.sin_len = remote_addr.sin_len;
        g_client_conns_t[link_id].addr.sin_family = remote_addr.sin_family;
        g_client_conns_t[link_id].addr.sin_port = remote_addr.sin_port;
        g_client_conns_t[link_id].addr.sin_addr.s_addr = remote_addr.sin_addr.s_addr;
    }
    return QAT_OK;
}


void cleanGlobalQueue(QueueHandle_t globalQueue, int idx) {
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

    client_fd = g_client_conns_t[*p_id].sockfd;
    protocol_type = g_client_conns_t[*p_id].protocol_type;
    switch (protocol_type){
        case PROTOCOL_TCP:{
            protocol_name = "TCP";
            break;
        }
        case PROTOCOL_UDP:{
            protocol_name = "UDP";
            break;
        }
    }

    do{
        if(g_client_conns_t[*p_id].thread_quit){
            closesocket(client_fd);
            CircularBuffer_Destroy(g_client_conns_t[*p_id].cb);
            cleanGlobalQueue(client_queue, *p_id);
            qurt_mutex_lock(&client_mutex);
            ipd_message_print_flag = true;
            qurt_mutex_unlock(&client_mutex);
            memset((void*)buffer, 0, QAT_INPUT_BUFFER_LENGTH);
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

        if((recv_type == RECVTYPE_PASSIVE)&& (ipd_message_print_flag == true) && 
            (uxQueueMessagesWaiting(client_queue) > 0) && (xQueuePeek(client_queue, &elem, 0) == pdTRUE))
        {
            memset((void*)buffer, 0, QAT_INPUT_BUFFER_LENGTH);
            if(*p_id == elem.id){
                snprintf(buffer, QAT_INPUT_BUFFER_LENGTH, "+IPD:%c,%s,%d,%d\r\n", 'C', protocol_name, elem.id, elem.len);
                QAT_Response_Str(QAT_RC_QUIET, buffer);
                qurt_mutex_lock(&client_mutex);
                ipd_message_print_flag = false;
                qurt_mutex_unlock(&client_mutex);
            }
        }

        ret = select(max_fd + 1, &readfds, NULL, NULL, &timeout);
        if (ret < 0) {
            QAT_IP_PRINTF("select error \r\n");
            goto client_recv_fail;;
        }
        else if (ret == 0) {
            // QAT_IP_PRINTF("select timeout \r\n");
            continue;
        }
        else
        {
            if (FD_ISSET(client_fd, &readfds)) 
            {
                memset((void*)buffer, 0, QAT_INPUT_BUFFER_LENGTH);
                memset((void*)input_buf, 0, QAT_INPUT_BUFFER_LENGTH);
                if(recv_type == RECVTYPE_ACTIVE){
                    recv_bytes = recv(client_fd, input_buf, sizeof(input_buf) - 1, 0);
                    if (recv_bytes < 0) {
                        if (errno == ECONNRESET || errno == ENOTCONN) {
                            goto client_recv_fail;
                        }
                    }
                    else if (recv_bytes == 0) {
                        goto client_recv_fail;
                    }
                    else{
                        input_buf[recv_bytes] = '\0';
                        snprintf(buffer, QAT_INPUT_BUFFER_LENGTH, "+IPD:%c,%s,%d,%d,%s\r\n", 'C', protocol_name, *p_id, recv_bytes, input_buf);
                        QAT_Response_Str(QAT_RC_QUIET, buffer);
                    }
                }else{
                    if(g_client_conns_t[*p_id].protocol_type == PROTOCOL_TCP){
                        if (uxQueueSpacesAvailable(client_queue) == 0) {
                            continue;
                        }
                        result = CircularBuffer_GetFreeSpace(g_client_conns_t[*p_id].cb);
                        if(lwip_ioctl(client_fd, FIONREAD, &bytes_available) == 0){
                            if(result < bytes_available){
                                continue;
                            }
                        }else{
                            goto client_recv_fail;;
                        }
                    }
                    recv_bytes = recv(client_fd, input_buf, sizeof(input_buf) - 1, 0);
                    if (recv_bytes < 0) {
                        if (errno == ECONNRESET || errno == ENOTCONN) {
                            goto client_recv_fail;
                        }
                        continue;
                    }
                    else if (recv_bytes == 0) {
                        goto client_recv_fail;
                    }
                    else{
                        if(g_client_conns_t[*p_id].protocol_type == PROTOCOL_UDP){
                            if (uxQueueSpacesAvailable(client_queue) == 0) {
                                continue;
                            }
                            result = CircularBuffer_GetFreeSpace(g_client_conns_t[*p_id].cb);
                            if(result < recv_bytes){
                                continue;
                            }
                        }
                        elem.id = *p_id;
                        elem.len = recv_bytes;
                        if(xQueueSend(client_queue, &elem, 0) == pdPASS ){
                            CircularBuffer_Write(g_client_conns_t[*p_id].cb, input_buf, recv_bytes);
                        }
                    }
                }
            }
        }
    } while(1);

    nt_osal_thread_delete(NULL);
    return;

client_recv_fail:
    closesocket(client_fd);
    CircularBuffer_Destroy(g_client_conns_t[*p_id].cb);
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
static QAT_Command_Status_t Extend_Command_Start(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List)
{
    ip_addr_t ip_addr, local_ip_addr;
    int link_id, port, local_port;
    QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
    int protocol_type;
    char buffer[QAT_CMD_IP_BUFFER_LENGTH] = {0};
    int offset;
    char *protocol_name;
    int fd;
    struct sockaddr_in local_addr;
    socklen_t local_addr_len = sizeof(local_addr);
    
    switch (Op_Type){        
        case QAT_OP_EXEC:
        {
            QAT_Response_Str(QAT_RC_OK, "+CIPSTART=<link id>,<type>,<remote ip>,<remote port>\r\n");
            break;
        }
        case QAT_OP_QUERY:
        {
            offset = 0;
            memset((void*)buffer, 0, QAT_CMD_IP_BUFFER_LENGTH);
            for(int i = 0; i < QAT_CLIENT_MAX_CONNECTIONS; i++)
            {
                if(g_client_conns_t[i].active == INACTIVE){
                    continue;
                }

                protocol_type = g_client_conns_t[i].protocol_type;
                switch (protocol_type){
                    case PROTOCOL_TCP:{
                        protocol_name = "TCP";
                        break;
                    }
                    case PROTOCOL_UDP:{
                        protocol_name = "UDP";
                        break;
                    }
                    default:
                    {}
                }

                port = g_client_conns_t[i].addr.sin_port;
                inet_addr_to_ip4addr(ip_2_ip4(&ip_addr), &g_client_conns_t[i].addr.sin_addr);
                
                fd = g_client_conns_t[i].sockfd;
                if (getsockname(fd, (struct sockaddr *)&local_addr, &local_addr_len) != 0){
                    continue;
                }

                local_port = local_addr.sin_port;
                inet_addr_to_ip4addr(ip_2_ip4(&local_ip_addr), &local_addr.sin_addr);
                offset += snprintf(buffer + offset, QAT_CMD_IP_BUFFER_LENGTH, "+CIPSTART:");
                offset += snprintf(buffer + offset, QAT_CMD_IP_BUFFER_LENGTH, "%c,", 'C');
                offset += snprintf(buffer + offset, QAT_CMD_IP_BUFFER_LENGTH, "%d,%s,", i, protocol_name);
                offset += snprintf(buffer + offset, QAT_CMD_IP_BUFFER_LENGTH, "%s,%d,", ipaddr_ntoa(&ip_addr), ntohs(port));
                offset += snprintf(buffer + offset, QAT_CMD_IP_BUFFER_LENGTH, "%s,%d\r\n", ipaddr_ntoa(&local_ip_addr), ntohs(local_port));
                }
            rc = QAT_Response_Str(QAT_RC_OK, buffer);
            break;
        }
        case QAT_OP_EXEC_W_PARAM: 
        {
            if( (Parameter_Count != 4) || (!Parameter_List) || (!Parameter_List[0].Integer_Is_Valid) ||
                    Parameter_List[1].Integer_Is_Valid || Parameter_List[2].Integer_Is_Valid ||
                    (!Parameter_List[3].Integer_Is_Valid)) {
                QAT_IP_PRINTF("+CIPSTART:Invalid input parameter!\r\n");
                goto end;
            }

            memset((void*)buffer, 0, QAT_CMD_IP_BUFFER_LENGTH);
            link_id = Parameter_List[0].Integer_Value;
            // check if link_id is valid
            if (link_id < 0 || link_id >= QAT_CLIENT_MAX_CONNECTIONS){
                snprintf(buffer, QAT_CMD_IP_BUFFER_LENGTH, "+CIPSTART:The value of link id must be an integer between 0 and 3\r\n");
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
            } else {
                protocol_type = PROTOCOL_INVALID;
                snprintf(buffer, QAT_CMD_IP_BUFFER_LENGTH, "+CIPSTART:protocol_type is invalid!\r\n");
                goto end;
            }

            if(!ipaddr_aton(Parameter_List[2].String_Value, &ip_addr)){
                snprintf(buffer, QAT_CMD_IP_BUFFER_LENGTH, "+CIPSTART:ipv4 address is invalid!\r\n");
                goto end;
            }

            port = Parameter_List[3].Integer_Value;
            if(CreateConnection(link_id, protocol_type, &ip_addr, port) == QAT_ERROR){
                snprintf(buffer, QAT_CMD_IP_BUFFER_LENGTH, "+IPS:FAILED:%d\r\n", link_id);
                goto end;
            }

            if (nt_qurt_thread_create(client_recv_thread, "client_recv_task", 1024, &(g_client_conns_t[link_id].id) , 6, NULL) != pdPASS)
            {
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
static QAT_Command_Status_t Extend_Command_Close(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List)
{
    int link_id;
    int sockfd;
    QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
    char buffer[QAT_CMD_IP_BUFFER_LENGTH] = {0};

    switch (Op_Type)
    {
        case QAT_OP_EXEC:
        {
            QAT_Response_Str(QAT_RC_OK, "+CIPCLOSE=<link id>\r\n");
            break;
        }
        case QAT_OP_EXEC_W_PARAM:
        {
            if( (Parameter_Count != 1) || (!Parameter_List) || (!Parameter_List[0].Integer_Is_Valid)) 
            {
                QAT_Response_Str(QAT_RC_ERROR, "+CIPCLOSE:Invalid input parameter!\r\n");
                return rc;
            }
            memset((void*)buffer, 0, QAT_CMD_IP_BUFFER_LENGTH);

            link_id = Parameter_List[0].Integer_Value;
            // check if link_id is valid
            if (link_id < 0 || link_id >= QAT_CLIENT_MAX_CONNECTIONS) 
            {
                snprintf(buffer, QAT_CMD_IP_BUFFER_LENGTH, "+CIPCLOSE:The value of link id must be an integer between 0 and 3\r\n");
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
            sys_msleep(TIMEOUT_TV_SEC * QAT_TIME_SEC_TO_MS + 2* TIMEOUT_TV_SEC/QAT_TIME_SEC_TO_MS);         // wait for g_client_conns_t[link_id].thread_quit be valid
            break;
        }
    }
    return rc;
}

static QAT_Command_Status_t Extend_Command_Send(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List)
{
    int link_id, sockfd;
    int total_sent, sent_bytes;
    char *output_buf = NULL;
    int data_mode_one_send_len;
    QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
    char buffer[QAT_CMD_IP_BUFFER_LENGTH] = {0};
    switch (Op_Type)
    {        
        case QAT_OP_EXEC:
        {
            QAT_Response_Str(QAT_RC_OK, "+CIPSEND=<link id>,<len>\r\n");
            break;
        }
        case QAT_OP_EXEC_W_PARAM:
        {
            if( (Parameter_Count != 2) || (!Parameter_List) || (!Parameter_List[0].Integer_Is_Valid) 
                    || (!Parameter_List[1].Integer_Is_Valid))
            {
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
            if(data_mode_max_len < 0){
                QAT_Response_Str(QAT_RC_ERROR,"+CIPSEND:The len parameter must be greater than or equal to 0.\r\n");
                return rc;
            }
            extern Cur_Data_Mode_Cmd_t Cur_Data_Mode_Cmd;
            memcpy(Cur_Data_Mode_Cmd.cur_data_mode_commnd,"+CIPSEND",strlen("+CIPSEND"));

            QAT_Transfer_Mode_set(QAT_Transfer_Mode_ONLINE_DATA_E,QAT_Data_Transfer_Mode_Handle);
            QAT_Response_Str(QAT_RC_OK, NULL);
            snprintf(buffer, QAT_CMD_IP_BUFFER_LENGTH, ">\r\n");
            QAT_Response_Str(QAT_RC_QUIET_NO_CR, buffer);
            break;
        }
        case QAT_OP_EXEC_IN_DATA_MODEL:
        {
            output_buf = (char*)Parameter_List;
            data_mode_one_send_len = (Parameter_Count <= (data_mode_max_len - data_mode_total_send_len))? 
                                    Parameter_Count : (data_mode_max_len - data_mode_total_send_len);

            total_sent = 0;
            sockfd = g_client_conns_t[data_mode_link_id].sockfd;
            while(total_sent < data_mode_one_send_len){
                sent_bytes = send(sockfd, output_buf + total_sent, data_mode_one_send_len - total_sent, 0);
                if(sent_bytes < 0){
                    snprintf(buffer, QAT_CMD_IP_BUFFER_LENGTH, "+IPS:SEND FAILED:%d\r\n", data_mode_link_id);
                    QAT_Response_Str(QAT_RC_ERROR, buffer);
                    return rc;
                }
                total_sent += sent_bytes;
            }

            data_mode_total_send_len += data_mode_one_send_len;
            if(data_mode_total_send_len >= data_mode_max_len)
            {
                snprintf(buffer, QAT_CMD_IP_BUFFER_LENGTH, "+IPS:SEND DONE:%d\r\n", data_mode_link_id);
                rc = QAT_Response_Str(QAT_RC_QUIET_NO_CR, buffer);
                data_mode_total_send_len = 0;
                data_mode_max_len = 0;
                data_mode_link_id = INVALID_LINKID;
                QAT_Transfer_Mode_set(QAT_Transfer_Mode_AT_COMMAND_E,NULL);
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

static QAT_Command_Status_t Extend_Command_SendData(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List)
{
    int link_id, sockfd;
    int set_len, str_len;
    int sent_len, total_sent;
    int sent_bytes;
    int sent_count;
    char *output_buf = NULL;
    QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
    char buffer[QAT_CMD_IP_BUFFER_LENGTH] = {0};

    switch (Op_Type)
    {        
        case QAT_OP_EXEC:
        {
            QAT_Response_Str(QAT_RC_OK, "+CIPSENDDATA=<link id>,<len>,\"data\"\r\n");
            break;
        }
        case QAT_OP_EXEC_W_PARAM:
        {
            if( (Parameter_Count != 3) || (!Parameter_List) || (!Parameter_List[0].Integer_Is_Valid) 
                    || (!Parameter_List[1].Integer_Is_Valid))
            {
                QAT_Response_Str(QAT_RC_ERROR, "+CIPSENDDATA:Invalid input parameter!\r\n");
                return rc;
            }

            link_id = Parameter_List[0].Integer_Value;

            // check if link_id is valid
            if (link_id < 0 || link_id >= QAT_CLIENT_MAX_CONNECTIONS) {
                QAT_Response_Str(QAT_RC_ERROR, "+CIPSENDDATA:The value of link id must be an integer between 0 and 3\r\n");
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
            while(total_sent < sent_len){
                sent_bytes = send(sockfd, output_buf + total_sent, sent_len - total_sent, 0);
                if(sent_bytes < 0){
                    snprintf(buffer, QAT_CMD_IP_BUFFER_LENGTH, "+IPS:SEND FAILED:%d\r\n", link_id);
                    QAT_Response_Str(QAT_RC_ERROR, buffer);
                    return rc;
                }
                total_sent += sent_bytes;
                
                if(sent_count >= DATA_MAX_SEND_COUNT)
                {
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
static QAT_Command_Status_t Extend_Command_RecvType(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List)
{
    QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
    int link_id, recv_type;
    char buffer[QAT_CMD_IP_BUFFER_LENGTH] = {0};
    int offset;
    char *serverFlag = NULL;
    int protocol_type;
    switch (Op_Type)
    {
        case QAT_OP_EXEC:
        {
            QAT_Response_Str(QAT_RC_OK, "+CIPRECVTYPE=<serverFlag>,<type>,<link_id>,<reception mode>\r\n");
            break;
        }
        case QAT_OP_QUERY:
        {
            offset = 0;
            for(int i = 0; i < QAT_CLIENT_MAX_CONNECTIONS; i++){
                if((g_client_conns_t[i].active == ACTIVE) && (g_client_conns_t[i].protocol_type != PROTOCOL_INVALID)){
                    if(g_client_conns_t[i].protocol_type == PROTOCOL_TCP){
                        offset += snprintf(buffer + offset, QAT_CMD_IP_BUFFER_LENGTH, "+CIPRECVTYPE:%c,%s,%d,%d\r\n", 'C', "TCP", i, g_client_conns_t[i].recv_type);
                    }
                    else if(g_client_conns_t[i].protocol_type == PROTOCOL_UDP){
                        offset += snprintf(buffer + offset, QAT_CMD_IP_BUFFER_LENGTH, "+CIPRECVTYPE:%c,%s,%d,%d\r\n", 'C', "UDP", i, g_client_conns_t[i].recv_type);
                    }
                }else{
                    continue;
                }
            }
  
            for(int i = 0; i < QAT_CLIENT_MAX_CONNECTIONS; i++){
               if(g_listen_clients[i].active == ACTIVE){
                    offset += snprintf(buffer + offset, QAT_CMD_IP_BUFFER_LENGTH, "+CIPRECVTYPE:%c,%s,%d,%d\r\n",'S', "TCP", i, g_listen_clients[i].recv_type);
                }else{
                    continue;
                }
            }

            for(int i = 0; i < QAT_CLIENT_MAX_CONNECTIONS; i++){
               if(g_listen_udp_clients[i].active == ACTIVE){
                    offset += snprintf(buffer + offset, QAT_CMD_IP_BUFFER_LENGTH, "+CIPRECVTYPE:%c,%s,%d,%d\r\n", 'S', "UDP", i, g_listen_udp_clients[i].recv_type);
                }else{
                    continue;
                }
            }
            rc = QAT_Response_Str(QAT_RC_OK, buffer);
            break;
        }
        case QAT_OP_EXEC_W_PARAM:
        {
            if( (Parameter_Count != 4) || !Parameter_List || (Parameter_List[0].Integer_Is_Valid)
                    || (Parameter_List[1].Integer_Is_Valid) || (!Parameter_List[2].Integer_Is_Valid)
                    || (!Parameter_List[3].Integer_Is_Valid)) 
            {
                QAT_Response_Str(QAT_RC_ERROR, "+CIPRECVTYPE:Invalid input parameter!\r\n");
                return rc;
            }

            serverFlag = Parameter_List[0].String_Value;
            if ((strcmp(serverFlag, "C") != 0) && (strcmp(serverFlag, "S") != 0)) {
                QAT_Response_Str(QAT_RC_ERROR, "+CIPRECVTYPE:serverFlag must be a string of C or S, C:Client, S:Server\r\n");
                return rc;
            }

            if (strcmp(Parameter_List[1].String_Value, "TCP") == 0) {
                protocol_type = PROTOCOL_TCP;
            } else if (strcmp(Parameter_List[1].String_Value, "UDP") == 0) {
                protocol_type = PROTOCOL_UDP;                              
            } else {
                snprintf(buffer, QAT_CMD_IP_BUFFER_LENGTH, "+CIPRECVTYPE:protocol_type is invalid!\r\n");
                QAT_Response_Str(QAT_RC_ERROR, buffer);
                return rc;
            }

            link_id = Parameter_List[2].Integer_Value;
            // check if link_id is valid
            if (link_id < 0 || link_id >= QAT_CLIENT_MAX_CONNECTIONS) {
                QAT_Response_Str(QAT_RC_ERROR, "+CIPRECVTYPE:The value of link id must be an integer between 0 and 3\r\n");
                return rc;
            }
            recv_type = Parameter_List[3].Integer_Value;
            if (recv_type < RECVTYPE_ACTIVE || recv_type > RECVTYPE_PASSIVE) {
                QAT_Response_Str(QAT_RC_ERROR, "+CIPRECVTYPE:recv_type must be an integer between 0 and 1, 0:ACTIVE, 1:PASSIVE\r\n");
                return rc;
            }

            if(strcmp(serverFlag, "C") == 0){
                if(g_client_conns_t[link_id].active == INACTIVE){
                    snprintf(buffer, QAT_CMD_IP_BUFFER_LENGTH, "+CIPRECVTYPE:Client link id %d is not active\r\n", link_id);
                    QAT_Response_Str(QAT_RC_ERROR, buffer);
                    return rc;
                }
                if(g_client_conns_t[link_id].protocol_type != protocol_type){
                    snprintf(buffer, QAT_CMD_IP_BUFFER_LENGTH, "+CIPRECVTYPE:Client link id %d protocol_type is not match\r\n", link_id);
                    QAT_Response_Str(QAT_RC_ERROR, buffer);
                    return rc;
                }
                g_client_conns_t[link_id].recv_type = recv_type;
            }else{
                if(protocol_type == PROTOCOL_TCP){
                    if(g_listen_clients[link_id].active == INACTIVE){
                        snprintf(buffer, QAT_CMD_IP_BUFFER_LENGTH, "+CIPRECVTYPE:TCP Server link id %d is not active\r\n", link_id);
                        QAT_Response_Str(QAT_RC_ERROR, buffer);
                        return rc;
                    }
                    g_listen_clients[link_id].recv_type = recv_type;
                }
                else{
                    if(g_listen_udp_clients[link_id].active == INACTIVE){
                        snprintf(buffer, QAT_CMD_IP_BUFFER_LENGTH, "+CIPRECVTYPE:UDP Server link id %d is not active\r\n", link_id);
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

static QAT_Command_Status_t Extend_Command_RecvData(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List)
{
    int link_id, data_len;
    QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
    char input_data[QAT_DATA_INPUT_BUFFER_LENGTH] = {0};
    char buffer[QAT_INPUT_BUFFER_LENGTH] = {0};
    QueueElem elem;
    char *serverFlag = NULL;
    int protocol_type;
    switch (Op_Type)
    {
        case QAT_OP_EXEC:
        {
            QAT_Response_Str(QAT_RC_OK, "+CIPRECVDATA=<serverFlag>,<type>,<link_id>,<len>\r\n");
            break;
        }
        case QAT_OP_EXEC_W_PARAM:
        {
            if( (Parameter_Count != 4) || !Parameter_List || (Parameter_List[0].Integer_Is_Valid) || (Parameter_List[1].Integer_Is_Valid)
            || (!Parameter_List[2].Integer_Is_Valid) || (!Parameter_List[3].Integer_Is_Valid)) 
            {
                QAT_Response_Str(QAT_RC_ERROR, "+CIPRECVDATA:Invalid input parameters\r\n");
                return rc;
            }

            serverFlag = Parameter_List[0].String_Value;
            if ((strcmp(serverFlag, "C") != 0) && (strcmp(serverFlag, "S") != 0)) {
                QAT_Response_Str(QAT_RC_ERROR, "+CIPRECVDATA:serverFlag must be a string of C or S, C:Client, S:Server\r\n");
                return rc;
            }

            if (strcmp(Parameter_List[1].String_Value, "TCP") == 0) {
                protocol_type = PROTOCOL_TCP;
            } else if (strcmp(Parameter_List[1].String_Value, "UDP") == 0) {
                protocol_type = PROTOCOL_UDP;                              
            } else {
                snprintf(buffer, QAT_INPUT_BUFFER_LENGTH, "+CIPRECVDATA:protocol_type is invalid!\r\n");
                QAT_Response_Str(QAT_RC_ERROR, buffer);
                return rc;
            }

            link_id = Parameter_List[2].Integer_Value;
            if (link_id < 0 || link_id >= QAT_CLIENT_MAX_CONNECTIONS) {
                QAT_Response_Str(QAT_RC_ERROR, "+CIPRECVDATA:The value of link id must be an integer between 0 and 3\r\n");
                return rc;
            }
            data_len = Parameter_List[3].Integer_Value;
            if(data_len <= 0){
                QAT_Response_Str(QAT_RC_ERROR, "+CIPRECVDATA:The value of data_len must be greater than 0\r\n");
                return rc;                
            }

            if(strcmp(serverFlag, "C") == 0){
                if (g_client_conns_t[link_id].active == INACTIVE) {
                    snprintf(buffer, QAT_INPUT_BUFFER_LENGTH, "+CIPRECVDATA:Client link id %d is not active\r\n", link_id);
                    QAT_Response_Str(QAT_RC_ERROR, buffer);
                    return rc;
                }
                if(g_client_conns_t[link_id].protocol_type != protocol_type){
                    snprintf(buffer, QAT_INPUT_BUFFER_LENGTH, "+CIPRECVDATA:Client link id %d input protocol_type is not match\r\n", link_id);
                    QAT_Response_Str(QAT_RC_ERROR, buffer);
                    return rc;
                }
                if (g_client_conns_t[link_id].recv_type == RECVTYPE_ACTIVE) {
                    snprintf(buffer, QAT_INPUT_BUFFER_LENGTH, "+CIPRECVDATA:Client link id %d is not passive receive type\r\n", link_id);
                    QAT_Response_Str(QAT_RC_ERROR, buffer);
                    return rc;
                }

                if(xQueuePeek(client_queue, &elem, 0) == pdFALSE){
                    QAT_Response_Str(QAT_RC_ERROR, "+CIPRECVDATA:The news of the client queue is empty\r\n");
                    return rc;
                }

                if (data_len > elem.len) {
                    QAT_Response_Str(QAT_RC_ERROR, "+CIPRECVDATA:Reading length cannot be greater than the message length of the +IPD prompt\r\n");
                    return rc;
                }else if(data_len == elem.len){
                    if(CircularBuffer_Read(g_client_conns_t[link_id].cb, input_data, data_len) < 0){
                        QAT_Response_Str(QAT_RC_ERROR, "+CIPRECVDATA:Reading length cannot be greater than client ring buffer length\r\n");
                        return rc;
                    }
                    xQueueReceive(client_queue, &elem, 0);
                }else{
                    if(CircularBuffer_Read(g_client_conns_t[link_id].cb, input_data, data_len) < 0){
                        QAT_Response_Str(QAT_RC_ERROR, "+CIPRECVDATA:Reading length cannot be greater than ring buffer length\r\n");
                        return rc;
                    }
                    size_t remaining_len = elem.len - data_len;
                    xQueueReceive(client_queue, &elem, 0);
                    elem.len = remaining_len;
                    xQueueSendToFront(client_queue, &elem, 0);
                }

                input_data[data_len] = '\0';
                snprintf(buffer, QAT_INPUT_BUFFER_LENGTH, "+CIPRECVDATA:%c,%s,%d,%s\r\n", 'C', Parameter_List[1].String_Value, data_len, input_data);
                rc = QAT_Response_Str(QAT_RC_OK, buffer);
                qurt_mutex_lock(&client_mutex);
                ipd_message_print_flag = true;
                qurt_mutex_unlock(&client_mutex);
            }
            else{
                if(protocol_type == PROTOCOL_TCP){
                    if (g_listen_clients[link_id].active == INACTIVE) {
                        snprintf(buffer, QAT_INPUT_BUFFER_LENGTH, "+CIPRECVDATA:TCP server link id %d is not active\r\n", link_id);
                        QAT_Response_Str(QAT_RC_ERROR, buffer);
                        return rc;
                    }
                    if (g_listen_clients[link_id].recv_type == RECVTYPE_ACTIVE) {
                        snprintf(buffer, QAT_INPUT_BUFFER_LENGTH, "+CIPRECVDATA:TCP server link id %d is not passive receive type\r\n", link_id);
                        QAT_Response_Str(QAT_RC_ERROR, buffer);
                        return rc;
                    }
                    if(xQueuePeek(server_queue, &elem, 0) == pdFALSE){
                        QAT_Response_Str(QAT_RC_ERROR, "+CIPRECVDATA:The news of the tcp server queue is empty");
                        return rc;
                    }

                    if (data_len > elem.len) {
                        QAT_Response_Str(QAT_RC_ERROR, "+CIPRECVDATA:Reading length cannot be greater than the message length of the +IPD prompt\r\n");
                        return rc;
                    }else if(data_len == elem.len){
                        if(CircularBuffer_Read(server_cb, input_data, data_len) < 0){
                            QAT_Response_Str(QAT_RC_ERROR, "+CIPRECVDATA:Reading length cannot be greater than tcp server ring buffer length\r\n");
                            return rc;
                        }
                        xQueueReceive(server_queue, &elem, 0);
                    }else{
                        if(CircularBuffer_Read(server_cb, input_data, data_len) < 0){
                            QAT_Response_Str(QAT_RC_ERROR, "+CIPRECVDATA:Reading length cannot be greater than tcp server ring buffer length\r\n");
                            return rc;
                        }
                        size_t remaining_len = elem.len - data_len;
                        xQueueReceive(server_queue, &elem, 0);
                        elem.len = remaining_len;
                        xQueueSendToFront(server_queue, &elem, 0);
                    }
                    
                    input_data[data_len] = '\0';
                    snprintf(buffer, QAT_INPUT_BUFFER_LENGTH, "+CIPRECVDATA:%c,%s,%d,%s\r\n", 'S' ,"TCP", data_len, input_data);
                    rc = QAT_Response_Str(QAT_RC_OK, buffer);
                    qurt_mutex_lock(&server_mutex);
                    server_ipd_message_print_flag = true;
                    qurt_mutex_unlock(&server_mutex);
                }
                else if(protocol_type == PROTOCOL_UDP){
                    if (g_listen_udp_clients[link_id].active == INACTIVE) {
                        snprintf(buffer, QAT_INPUT_BUFFER_LENGTH, "+CIPRECVDATA:UDP server link id %d is not active\r\n", link_id);
                        QAT_Response_Str(QAT_RC_ERROR, buffer);
                        return rc;
                    }

                    if (g_listen_udp_clients[link_id].recv_type == RECVTYPE_ACTIVE) {
                        snprintf(buffer, QAT_INPUT_BUFFER_LENGTH, "+CIPRECVDATA:UDP server link id %d is not passive receive type\r\n", link_id);
                        QAT_Response_Str(QAT_RC_ERROR, buffer);
                        return rc;
                    }

                    if(xQueuePeek(udp_server_queue, &elem, 0) == pdFALSE){
                        QAT_Response_Str(QAT_RC_ERROR, "+CIPRECVDATA:The news of the udp server queue is empty");
                        return rc;
                    }

                    if (data_len > elem.len) {
                        QAT_Response_Str(QAT_RC_ERROR, "+CIPRECVDATA:Reading length cannot be greater than the message length of the +IPD prompt\r\n");
                        return rc;
                    }else if(data_len == elem.len){
                        if(CircularBuffer_Read(udp_server_cb, input_data, data_len) < 0){
                            QAT_Response_Str(QAT_RC_ERROR, "+CIPRECVDATA:Reading length cannot be greater than udp server ring buffer length\r\n");
                            return rc;
                        }
                        xQueueReceive(udp_server_queue, &elem, 0);
                    }else{
                        if(CircularBuffer_Read(udp_server_cb, input_data, data_len) < 0){
                            QAT_Response_Str(QAT_RC_ERROR, "+CIPRECVDATA:Reading length cannot be greater than udp server ring buffer length\r\n");
                            return rc;
                        }
                        size_t remaining_len = elem.len - data_len;
                        xQueueReceive(udp_server_queue, &elem, 0);
                        elem.len = remaining_len;
                        xQueueSendToFront(udp_server_queue, &elem, 0);
                    }
                    
                    input_data[data_len] = '\0';
                    snprintf(buffer, QAT_INPUT_BUFFER_LENGTH, "+CIPRECVDATA:%c,%s,%d,%s\r\n",'S',"UDP", data_len, input_data);
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

static void CleanupListenClientConnInfo(int link_id) {
    g_listen_clients[link_id].sockfd = INVALID_FD;
    memset(&(g_listen_clients[link_id].client_addr), 0, sizeof(struct sockaddr_in));
    g_listen_clients[link_id].active = false;
    g_listen_clients[link_id].recv_type = RECVTYPE_ACTIVE;
}
static void CleanupUdpListenClientConnInfo(int link_id) {
    g_listen_udp_clients[link_id].sockfd = INVALID_FD;
    memset(&(g_listen_udp_clients[link_id].client_addr), 0, sizeof(struct sockaddr_in));
    g_listen_udp_clients[link_id].active = false;
    g_listen_udp_clients[link_id].recv_type = RECVTYPE_ACTIVE;
}

/*-------------------------------------------------------------------------
 * Function Definitions
 *-----------------------------------------------------------------------*/
int find_invalid_socket() {
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
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    fd_set readfds;
    int index, fd_index;
    char buffer[QAT_INPUT_BUFFER_LENGTH] = {0};
    char input_buf[QAT_DATA_INPUT_BUFFER_LENGTH];
    struct timeval timeout;
    int result;
    int recv_type, recv_bytes;
    int bytes_available;
    QueueElem elem;

    do{
        if (config->mode == 0){
            if(tcp_listen_fd >= 0){
                closesocket(tcp_listen_fd);
                tcp_listen_fd = INVALID_FD;
            }

            if(config->params.closeServer){
                for(int index = 0; index < QAT_CLIENT_MAX_CONNECTIONS; index++){
                    if(g_listen_clients[index].sockfd != INVALID_FD){
                        closesocket(g_listen_clients[index].sockfd);
                        CleanupListenClientConnInfo(index);
                    }
                }
                CircularBuffer_Destroy(server_cb);
                server_cb = NULL;
                qurt_mutex_lock(&server_mutex);
                server_ipd_message_print_flag = true;
                qurt_mutex_unlock(&server_mutex);
                tcpServerThreadCreated =false;
                nt_osal_thread_delete(NULL);
                return;
            }
        }

        maxfd = 0;
        FD_ZERO(&readfds);
        if(tcp_listen_fd >= 0){
            FD_SET(tcp_listen_fd, &readfds);
            maxfd = tcp_listen_fd;
        }

        for(index = 0; index < QAT_CLIENT_MAX_CONNECTIONS; index++){
            sd = g_listen_clients[index].sockfd;
            if(sd >= 0){
                FD_SET(sd, &readfds);
            }
            if(sd > maxfd){
                maxfd = sd;
            }
        }

        timeout.tv_sec = TIMEOUT_TV_SEC;
        timeout.tv_usec = TIMEOUT_TV_USEC;
        
        if((server_ipd_message_print_flag == true) && (uxQueueMessagesWaiting(server_queue) > 0) && 
            (xQueuePeek(server_queue, &elem, 0) == pdTRUE))
        {
            memset((void*)buffer, 0, QAT_INPUT_BUFFER_LENGTH);
            if(g_listen_clients[elem.id].recv_type == RECVTYPE_PASSIVE){
                snprintf(buffer, QAT_INPUT_BUFFER_LENGTH, "+IPD:%c,%s,%d,%d\r\n", 'S', "TCP", elem.id, elem.len);
                QAT_Response_Str(QAT_RC_QUIET, buffer);
                qurt_mutex_lock(&server_mutex);
                server_ipd_message_print_flag = false;
                qurt_mutex_unlock(&server_mutex);
            }
        }

        if(select(maxfd + 1, &readfds, NULL, NULL, &timeout) > 0){
            if ((tcp_listen_fd >= 0) && FD_ISSET(tcp_listen_fd, &readfds)){
                if ((data_fd = accept(tcp_listen_fd, (struct sockaddr *)&client_addr, &client_addr_len)) < 0) {
                    printf("accept failed");
                    continue;
                }
                fd_index = find_invalid_socket();
                if(fd_index == QAT_ERROR)
                {
                    QAT_Response_Str(QAT_RC_ERROR, "Conn_Exceeded\r\n");
                    continue;
                }
                g_listen_clients[fd_index].sockfd = data_fd;
                g_listen_clients[fd_index].active = ACTIVE;
            }

            for (index = 0; index < QAT_CLIENT_MAX_CONNECTIONS; index++) {
                sd = g_listen_clients[index].sockfd;
                recv_type = g_listen_clients[index].recv_type;
                if(sd < 0){
                    continue;
                }
                if (FD_ISSET(sd, &readfds)) {
                    memset((void*)input_buf, 0, QAT_DATA_INPUT_BUFFER_LENGTH);
                    memset((void*)buffer, 0, QAT_INPUT_BUFFER_LENGTH);
                    if(recv_type == RECVTYPE_ACTIVE){
                        recv_bytes = recv(sd, input_buf, sizeof(input_buf) - 1, 0);
                        if (recv_bytes < 0) {
                            if (errno == ECONNRESET || errno == ENOTCONN) {
                                closesocket(g_listen_clients[index].sockfd);
                                CleanupListenClientConnInfo(index);
                            }
                            continue;
                        }else if(recv_bytes == 0) {
                            closesocket(g_listen_clients[index].sockfd);
                            CleanupListenClientConnInfo(index);
                            continue;
                        }else{
                            input_buf[recv_bytes] = '\0';
                            snprintf(buffer, QAT_INPUT_BUFFER_LENGTH, "+IPD:%c,%s,%d,%d,%s\r\n", 'S', "TCP", index, recv_bytes, input_buf);
                            QAT_Response_Str(QAT_RC_QUIET, buffer);
                        }
                    }else{
                        if (uxQueueSpacesAvailable(server_queue) == 0) {
                            continue;
                        }
                        result = CircularBuffer_GetFreeSpace(server_cb);
                        if(lwip_ioctl(sd, FIONREAD, &bytes_available) == 0){
                            if(result < bytes_available){
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
                        }
                        else if (recv_bytes == 0) {
                            closesocket(g_listen_clients[index].sockfd);
                            CleanupListenClientConnInfo(index);
                            continue;
                        }
                        else{
                            elem.id = index;
                            elem.len = recv_bytes;
                            if(xQueueSend(server_queue, &elem, 0) == pdPASS ){
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
static QAT_Command_Status_t Extend_Command_Server(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List)
{
    int mode, value;
    QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
    char buf[QAT_CMD_IP_BUFFER_LENGTH] = {0};
    int offset;
    ip_addr_t peer_ip_addr, local_ip_addr;
    int peer_port, local_port;
    int fd;
    struct sockaddr_in local_addr, peer_addr;
    socklen_t local_addr_len = sizeof(local_addr);
    socklen_t peer_addr_len = sizeof(peer_addr);

    struct sockaddr_in server_addr;
    int opt = 1;
    switch (Op_Type)
    {
        case QAT_OP_EXEC:
        {
            QAT_Response_Str(QAT_RC_QUIET, "+CIPSERVER=<mode>,<param2>");
            QAT_Response_Str(QAT_RC_QUIET, "mode:\r\n0: Shut down the server\r\n1: Build server");
            QAT_Response_Str(QAT_RC_QUIET, "if mode = 0, param2 can be either 0 or 1:\r\n    param2=0:Maintain existing server connections.\r\n    param2=1:Completely shut down the server.");
            QAT_Response_Str(QAT_RC_QUIET, "if mode = 1: param2 is listen port");
            QAT_Response_Str(QAT_RC_OK, NULL);
            break;
        }
        case QAT_OP_QUERY:
        {
            offset = 0;
            memset((void*)buf, 0, QAT_CMD_IP_BUFFER_LENGTH);
            for(int i = 0; i < QAT_CLIENT_MAX_CONNECTIONS; i++)
            {
                if(g_listen_clients[i].active == INACTIVE){
                    continue;
                }
                fd = g_listen_clients[i].sockfd;

                if (getpeername(fd, (struct sockaddr *)&peer_addr, &peer_addr_len) != 0) {
                    QAT_Response_Str(QAT_RC_ERROR, NULL);
                    return rc;
                }
                peer_port = peer_addr.sin_port;
                inet_addr_to_ip4addr(ip_2_ip4(&peer_ip_addr), &peer_addr.sin_addr);

                if (getsockname(fd, (struct sockaddr *)&local_addr, &local_addr_len) != 0){
                    QAT_Response_Str(QAT_RC_ERROR, NULL);
                    return rc;
                }
                local_port = local_addr.sin_port;
                inet_addr_to_ip4addr(ip_2_ip4(&local_ip_addr), &local_addr.sin_addr);

                offset += snprintf(buf + offset, QAT_CMD_IP_BUFFER_LENGTH, "+CIPSERVER:");
                offset += snprintf(buf + offset, QAT_CMD_IP_BUFFER_LENGTH, "%c,", 'S');
                offset += snprintf(buf + offset, QAT_CMD_IP_BUFFER_LENGTH, "%d,%s,", i, "TCP");
                offset += snprintf(buf + offset, QAT_CMD_IP_BUFFER_LENGTH, "%s,%d,", ipaddr_ntoa(&peer_ip_addr), ntohs(peer_port));
                offset += snprintf(buf + offset, QAT_CMD_IP_BUFFER_LENGTH, "%s,%d\r\n", ipaddr_ntoa(&local_ip_addr), ntohs(local_port));
                }
            rc = QAT_Response_Str(QAT_RC_OK, buf);
            break;
        }
        case QAT_OP_EXEC_W_PARAM:
        {
            if( Parameter_Count != 2 || !Parameter_List || !Parameter_List[0].Integer_Is_Valid || !Parameter_List[1].Integer_Is_Valid) 
            {
                QAT_Response_Str(QAT_RC_ERROR, "+CIPSERVER:Invalid input parameter!\r\n");
                return rc;
            }

            mode = Parameter_List[0].Integer_Value;
            value = Parameter_List[1].Integer_Value;
            switch(mode)
            {
                case 0:
                {
                    if(value < 0 || value >1)
                    {
                        QAT_Response_Str(QAT_RC_ERROR, "+CIPSERVER:when mode = 0, param2 can only be 0 or 1\r\n");
                        return rc;
                    }
                    tcp_config.params.closeServer = value;
                    break;
                }
                case 1:
                {
                    if(value < 0 || value > 65535){
                        QAT_Response_Str(QAT_RC_ERROR, "+CIPSERVER:The port number must not exceed 65535.\r\n");
                        return rc;
                    }
                    tcp_config.params.port = value;
                    break;
                }
                default:
                {
                    QAT_Response_Str(QAT_RC_ERROR, "+CIPSERVER:mode can only be 0 or 1\r\n");
                    return rc;
                }
            }
            tcp_config.mode = mode;
            
            if((tcpServerThreadCreated == false) && (mode == 1))
            {
                tcp_listen_fd = socket(AF_INET, SOCK_STREAM, 0);
                if (tcp_listen_fd < 0) {
                    QAT_Response_Str(QAT_RC_ERROR, NULL);
                    return rc;
                }

                if (setsockopt(tcp_listen_fd, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt)) < 0) {
                    goto tcp_server_fail;
                }

                memset(&server_addr, 0, sizeof(server_addr));
                server_addr.sin_len = sizeof(struct sockaddr_in);
                server_addr.sin_family = AF_INET;
                server_addr.sin_addr.s_addr = INADDR_ANY;
                server_addr.sin_port = htons(tcp_config.params.port);

                if (bind(tcp_listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
                    goto tcp_server_fail;
                }

                if (listen(tcp_listen_fd, QAT_CLIENT_MAX_CONNECTIONS) < 0) {
                    goto tcp_server_fail;
                }

                server_cb = CircularBuffer_Create();
                if(server_cb == NULL){
                    goto tcp_server_fail;
                }

                if (nt_qurt_thread_create(tcp_server_thread, "tcp_server_thread", 1024, &tcp_config, 6, NULL) == 1){
                    tcpServerThreadCreated = true;
                    rc = QAT_Response_Str(QAT_RC_OK, NULL);
                }
                else{
                    QAT_Response_Str(QAT_RC_ERROR, NULL);
                    return rc;
                }
            }else if(mode == 0){
                rc = QAT_Response_Str(QAT_RC_OK, NULL);
            }else{
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

int find_invalid_addr(struct sockaddr_in *client_addr) {
   for (int i = 0; i < QAT_CLIENT_MAX_CONNECTIONS; i++) {
       if (g_listen_udp_clients[i].active &&
           (g_listen_udp_clients[i].client_addr.sin_addr.s_addr == client_addr->sin_addr.s_addr) &&
           (g_listen_udp_clients[i].client_addr.sin_port == client_addr->sin_port)) {
           return i;
       }
   }
   return QAT_ERROR;
}

int add_udp_client(struct sockaddr_in *client_addr) {
   for (int i = 0; i < QAT_CLIENT_MAX_CONNECTIONS; i++) {
       if (!g_listen_udp_clients[i].active) {
           memcpy(&g_listen_udp_clients[i].client_addr, client_addr, sizeof(struct sockaddr_in));
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
    struct sockaddr_in local_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    char input_buf[QAT_DATA_INPUT_BUFFER_LENGTH];
    int recv_len;
    int client_idx;
    struct timeval timeout;
    fd_set readfds;
    int recv_type;
    int result;
    QueueElem elem;

    do{
        if ((udpConfig->mode == 0) && udpConfig->params.closeServer){
            if(udp_listen_fd >= 0){
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
            udpServerThreadCreated =false;
            nt_osal_thread_delete(NULL);
            return;
        }
        else{
            timeout.tv_sec = TIMEOUT_TV_SEC;
            timeout.tv_usec = TIMEOUT_TV_USEC;
            maxfd = 0;
            FD_ZERO(&readfds);
            if(udp_listen_fd >= 0){
                FD_SET(udp_listen_fd, &readfds);
                maxfd = udp_listen_fd;
            }

            if((udp_server_ipd_message_print_flag == true) && (uxQueueMessagesWaiting(udp_server_queue) > 0) && 
                (xQueuePeek(udp_server_queue, &elem, 0) == pdTRUE))
            {
                memset((void*)buffer, 0, QAT_INPUT_BUFFER_LENGTH);
                if(g_listen_udp_clients[elem.id].recv_type == RECVTYPE_PASSIVE){
                    snprintf(buffer, QAT_INPUT_BUFFER_LENGTH, "+IPD:%c,%s,%d,%d\r\n", 'S', "UDP", elem.id, elem.len);
                    QAT_Response_Str(QAT_RC_QUIET, buffer);
                    qurt_mutex_lock(&udp_server_mutex);
                    udp_server_ipd_message_print_flag = false;
                    qurt_mutex_unlock(&udp_server_mutex);
                }
            }

            if(select(maxfd + 1, &readfds, NULL, NULL, &timeout) > 0)
            {
                memset((void*)input_buf, 0, QAT_DATA_INPUT_BUFFER_LENGTH);
                recv_len = recvfrom(udp_listen_fd, input_buf, sizeof(input_buf) - 1, 0, (struct sockaddr*)&client_addr, &client_addr_len);
                if (recv_len < 0) {
                    if (errno == ECONNRESET || errno == ENOTCONN) {
                        CleanupUdpListenClientConnInfo(client_idx);
                    }
                    continue;
                }else if (recv_len == 0) {
                    CleanupUdpListenClientConnInfo(client_idx);
                }else{
                    client_idx = find_invalid_addr(&client_addr);
                    if (client_idx == QAT_ERROR) {
                        if ((udpConfig->mode == 0) && !udpConfig->params.closeServer){
                            printf("Do not accept new UDP connections.\r\n");
                            continue;
                        }

                        client_idx = add_udp_client(&client_addr);
                        if (client_idx == QAT_ERROR) {
                            printf("Maximum connections reached. Cannot add client %s:%d.\n",
                                    inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                            continue;
                        }
                        g_listen_udp_clients[client_idx].active = ACTIVE;
                        g_listen_udp_clients[client_idx].sockfd = udp_listen_fd;
                    }

                    recv_type = g_listen_udp_clients[client_idx].recv_type;
                    if(recv_type == RECVTYPE_ACTIVE){
                        input_buf[recv_len] = '\0';
                        memset((void*)buffer, 0, QAT_INPUT_BUFFER_LENGTH);
                        snprintf(buffer, QAT_INPUT_BUFFER_LENGTH, "+IPD:%c,%s,%d,%d,%s\r\n", 'S', "UDP", client_idx, recv_len, input_buf);
                        QAT_Response_Str(QAT_RC_QUIET, buffer);
                    }else{
                        if (uxQueueSpacesAvailable(udp_server_queue) == 0) {
                            continue;
                        }
                        result = CircularBuffer_GetFreeSpace(udp_server_cb);
                        if(result < recv_len){
                            continue;
                        }
                        elem.id = client_idx;
                        elem.len = recv_len;
                        if(xQueueSend(udp_server_queue, &elem, 0) == pdPASS ){
                            CircularBuffer_Write(udp_server_cb, input_buf, recv_len);
                        }
                    }
                }
            }
        }
        sys_msleep(200);
    }while(1);
}

static QAT_Command_Status_t Extend_Command_UdpServer(uint32_t Op_Type, uint32_t Parameter_Count, QAT_Parameter_t *Parameter_List)
{
    int mode, value;
    QAT_Command_Status_t rc = QAT_STATUS_ERROR_E;
    char buf[QAT_CMD_IP_BUFFER_LENGTH] = {0};
    int offset;
    int fd;
    ip_addr_t peer_ip_addr, local_ip_addr;
    int peer_port, local_port;
    struct sockaddr_in local_addr;
    socklen_t local_addr_len = sizeof(local_addr);
    struct netif* netif = NULL;

    switch (Op_Type)
    {
        case QAT_OP_EXEC:
        {
            QAT_Response_Str(QAT_RC_QUIET, "+CIPUDPSERVER=<mode>,<param2>");
            QAT_Response_Str(QAT_RC_QUIET, "mode:\r\n0: Shut down the server\r\n1: Build server");
            QAT_Response_Str(QAT_RC_QUIET, "if mode = 0, param2 can be either 0 or 1:\r\n    param2=0:Maintain existing server connections.\r\n    param2=1:Completely shut down the server.");
            QAT_Response_Str(QAT_RC_QUIET, "if mode = 1: param2 is listen port");
            QAT_Response_Str(QAT_RC_OK, NULL);
            break;
        }
        case QAT_OP_QUERY:
        {
            offset = 0;
            memset((void*)buf, 0, QAT_CMD_IP_BUFFER_LENGTH);
            for(int i = 0; i < QAT_CLIENT_MAX_CONNECTIONS; i++)
            {
                if(g_listen_udp_clients[i].active == INACTIVE){
                    continue;
                }

                peer_port = g_listen_udp_clients[i].client_addr.sin_port;
                inet_addr_to_ip4addr(ip_2_ip4(&peer_ip_addr), &g_listen_udp_clients[i].client_addr.sin_addr);

                fd = g_listen_udp_clients[i].sockfd;
                if (getsockname(fd, (struct sockaddr *)&local_addr, &local_addr_len) != 0){
                    QAT_Response_Str(QAT_RC_ERROR, NULL);
                    return rc;
                }
                local_port = local_addr.sin_port;
                inet_addr_to_ip4addr(ip_2_ip4(&local_ip_addr), &local_addr.sin_addr);

                offset += snprintf(buf + offset, QAT_CMD_IP_BUFFER_LENGTH, "+CIPUDPSERVER:");
                offset += snprintf(buf + offset, QAT_CMD_IP_BUFFER_LENGTH, "%c,",'S');
                offset += snprintf(buf + offset, QAT_CMD_IP_BUFFER_LENGTH, "%d,%s,", i, "UDP");
                offset += snprintf(buf + offset, QAT_CMD_IP_BUFFER_LENGTH, "%s,%d,", ipaddr_ntoa(&peer_ip_addr), ntohs(peer_port));
                offset += snprintf(buf + offset, QAT_CMD_IP_BUFFER_LENGTH, "%s,%d\r\n", ipaddr_ntoa(&local_ip_addr), ntohs(local_port));
                }
            rc = QAT_Response_Str(QAT_RC_OK, buf);
            break;
        }
        case QAT_OP_EXEC_W_PARAM:
        {
            if( Parameter_Count != 2 || !Parameter_List || !Parameter_List[0].Integer_Is_Valid || !Parameter_List[1].Integer_Is_Valid) 
            {
                QAT_Response_Str(QAT_RC_ERROR, "+CIPUDPSERVER:Invalid input parameter!\r\n");
                return rc;
            }

            mode = Parameter_List[0].Integer_Value;
            value = Parameter_List[1].Integer_Value;
            switch(mode)
            {
                case 0:
                {
                    if(value < 0 || value >1){
                        QAT_Response_Str(QAT_RC_ERROR, "+CIPUDPSERVER:when mode = 0, param2 can only be 0 or 1\r\n");
                        return rc;
                    }
                    udp_config.params.closeServer = value;
                    break;
                }
                case 1:
                {
                    if(value < 0 || value > 65535){
                        QAT_Response_Str(QAT_RC_ERROR, "+CIPUDPSERVER:The port number must not exceed 65535.\r\n");
                        return rc;
                    }
                    udp_config.params.port = value;
                    break;
                }
                default:
                {
                    QAT_Response_Str(QAT_RC_ERROR, "+CIPUDPSERVER:mode can only be 0 or 1\r\n");
                    return rc;
                }
            }
            udp_config.mode = mode;
            if((udpServerThreadCreated == false) && (mode == 1))
            {
                udp_listen_fd = socket(AF_INET, SOCK_DGRAM, 0);
                if (udp_listen_fd  < 0) {
                    QAT_Response_Str(QAT_RC_ERROR, NULL);
                    return rc;
                }

                netif = get_netif_by_device(STA_DEVICE);
                ip_addr_t *ip_local_addr = (ip_addr_t *)netif_ip_addr4(netif);

                memset(&local_addr, 0, sizeof(local_addr));
                local_addr.sin_len = sizeof(struct sockaddr_in);
                local_addr.sin_family = AF_INET;
                local_addr.sin_port = htons(udp_config.params.port);
                inet_addr_from_ip4addr(&(local_addr.sin_addr), ip_2_ip4(ip_local_addr));

                if (bind(udp_listen_fd, (struct sockaddr *)&local_addr, sizeof(local_addr)) < 0) {
                    goto udp_server_fail;
                }

                udp_server_cb = CircularBuffer_Create();
                if(udp_server_cb == NULL){
                    goto udp_server_fail;
                }

                if (nt_qurt_thread_create(udp_server_thread, "udp_server_thread", 1024, &udp_config, 6, NULL) == 1){
                    udpServerThreadCreated = true;
                    rc = QAT_Response_Str(QAT_RC_OK, NULL);
                }
                else{
                    QAT_Response_Str(QAT_RC_ERROR, NULL);
                    return rc;
                }
            }else if(mode == 0){
                rc = QAT_Response_Str(QAT_RC_OK, NULL);
            }
            else{
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

void Initialize_QAT_TCPIP_Demo (void)
{
	qbool_t RetVal;
    for(int link_id = 0; link_id < QAT_CLIENT_MAX_CONNECTIONS; link_id++)
    {
        CleanupClientConnInfo(link_id);
        CleanupListenClientConnInfo(link_id);
        CleanupUdpListenClientConnInfo(link_id);
    }
    
    client_queue = xQueueCreate(CONFIG_QAT_CB_QUEUE_MAX_LENGTH, sizeof(QueueElem));
    server_queue = xQueueCreate(CONFIG_QAT_CB_QUEUE_MAX_LENGTH, sizeof(QueueElem));
    udp_server_queue = xQueueCreate(CONFIG_QAT_CB_QUEUE_MAX_LENGTH, sizeof(QueueElem));

	RetVal = QAT_Register_Command_Group(QAT_TCPIP_Command_List, TCPIP_COMMAND_LIST_SIZE);
	if(RetVal == false)
   {
      QAT_IP_PRINTF("Failed to register tcpip command group.\r\n");
   }
}