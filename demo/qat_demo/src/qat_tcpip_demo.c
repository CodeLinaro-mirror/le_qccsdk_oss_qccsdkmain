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

/* The following is the complete command list for the QAT common command demo. */
/** List of global commands that are supported when in a group. */
static QAT_Command_t QAT_TCPIP_Command_List[] =
{
	{"+CIPPING",        Extend_Command_Ping,            QAT_OP_EXEC_W_PARAM},
    {"+CIPDHCPV4C",     Extend_Command_DHCPv4c,         QAT_OP_EXEC_W_PARAM},
    {"+CIPSTA",         Extend_Command_SetStation,      QAT_OP_QUERY | QAT_OP_EXEC_W_PARAM},
};

/*-------------------------------------------------------------------------
 * Parameters define
 *-----------------------------------------------------------------------*/
#define TCPIP_COMMAND_LIST_SIZE                     (sizeof(QAT_TCPIP_Command_List) / sizeof(QAT_Command_t))

#define QAT_PING_DEFAULT_DELAY_SEC                  1
#define QAT_PING_DEFAULT_COUNT                      4
#define QAT_PING_DEFAULT_PACKET_SIZE                32
#define QAT_PING_RCV_TIME                           1000
#define QAT_PING_RECV_BUFFER_SIZE                   128
#define QAT_TIME_SEC_TO_MS                          1000

#define QAT_DNS_SERVER_INDEX0                       0
#define QAT_DNS_SERVER_INDEX1                       1
#define QAT_IP_STRING_BUFFER_LENGTH                 128

#define QAT_CLIENT_MAX_CONNECTIONS                  4
#define QAT_SERVER_MAX_CLIENTS                      4
#define INVALID_FD                                  -1
#define DATA_MAX_SEND_COUNT                         5

#define QAT_CMD_IP_BUFFER_LENGTH					256
#define QAT_INPUT_BUFFER_LENGTH					    512

#define TIMEOUT_TV_SEC					            1
#define TIMEOUT_TV_USEC					            0
#define QAT_IP_PRINTF(...) printf(__VA_ARGS__)


#define SIGNAL_MASK 0x01
#define EXIT_SIGNAL 0x02

/** ping identifier - must fit on a u16_t */
#ifndef QAT_PING_ID
#define QAT_PING_ID        0xACAB
#endif

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

/*-------------------------------------------------------------------------
 * Function Definitions
 *-----------------------------------------------------------------------*/
void qat_ping_prepare_echo(icmp_echo_hdr *icmp_hdr)
{
    ICMPH_TYPE_SET(icmp_hdr, ICMP_ECHO);
    ICMPH_CODE_SET(icmp_hdr, 0);
    icmp_hdr->chksum = 0;
    icmp_hdr->id     = QAT_PING_ID;
    icmp_hdr->seqno  = lwip_htons(++qat_ping_seq_num);

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

    err = lwip_sendto(s, icmp_hdr, ping_size, 0, to, sizeof(struct sockaddr));
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
    ret = select(s + 1, &fds, NULL, NULL, &timeout);
    if (ret > 0) {
        recv_len = lwip_recvfrom(s, buf, QAT_PING_RECV_BUFFER_SIZE, 0, (struct sockaddr*)&src_addr, &src_addr_len);
        if(recv_len >= (int)(sizeof(struct ip_hdr)+sizeof(struct icmp_echo_hdr))) {
            inet_addr_to_ip4addr(ip_2_ip4(&from_addr), &src_addr.sin_addr);
            IP_SET_TYPE_VAL(from_addr, IPADDR_TYPE_V4);
            struct ip_hdr * ip_header = (struct ip_hdr *)buf;
            struct icmp_echo_hdr *icmp_header = (struct icmp_echo_hdr *)(buf + (IPH_HL(ip_header) * 4));
            
            if ((icmp_header->id == QAT_PING_ID) && (icmp_header->seqno == lwip_htons(qat_ping_seq_num)) && (ICMPH_TYPE(icmp_header) == ICMP_ER)) 
            {
                qat_ping_recv_count++;
                memset((void*)buffer, 0, QAT_CMD_IP_BUFFER_LENGTH);
                snprintf(buffer, QAT_CMD_IP_BUFFER_LENGTH, "Ping reply from %s : seq = %u, time = %lu ms \r", ipaddr_ntoa(&from_addr), lwip_ntohs(icmp_header->seqno), (sys_now()-qat_ping_time));
                QAT_Response_Str(QAT_RC_QUIET, buffer);
                return;
            }
        }
    }

    memset((void*)buffer, 0, QAT_CMD_IP_BUFFER_LENGTH);
    snprintf(buffer, QAT_CMD_IP_BUFFER_LENGTH, "Request timed out!\r");
    QAT_Response_Str(QAT_RC_QUIET, buffer);
    return;
}

/*-------------------------------------------------------------------------
 * Function Definitions
 *-----------------------------------------------------------------------*/

static int qat_ping_process(int s, const ip_addr_t *addr)
{
    int ret = QAT_ERROR;
    char *buffer = NULL;
    char *recv_buf = NULL;

    buffer = malloc(QAT_CMD_IP_BUFFER_LENGTH);
    if(!buffer)
    {
        QAT_IP_PRINTF("Failed to malloc cmd memory.\r\n");
        return ret;
    }
    memset((void*)buffer, 0, QAT_CMD_IP_BUFFER_LENGTH);

    recv_buf = malloc(QAT_PING_RECV_BUFFER_SIZE);
    if(!recv_buf)
    {
        QAT_IP_PRINTF("Failed to malloc recv_buf memory.\r\n");
        free(buffer);
        return ret;
    }
    memset((void*)recv_buf, 0, QAT_PING_RECV_BUFFER_SIZE);

    snprintf(buffer, QAT_CMD_IP_BUFFER_LENGTH, "Pinging %s with %d bytes of data:\r", ipaddr_ntoa(addr), ping_size);
    QAT_Response_Str(QAT_RC_QUIET, buffer);
    for(int i = 0; i < ping_count; i++)
    {
        if (qat_ping_send(s, addr) == ERR_OK) {
            qat_ping_time = sys_now();
            qat_ping_sent_count++;
            qat_ping_recv(s, buffer, recv_buf);
            sys_msleep(ping_delay * QAT_TIME_SEC_TO_MS);
        } 
        else {
            memset((void*)buffer, 0, QAT_CMD_IP_BUFFER_LENGTH);
            snprintf(buffer, QAT_CMD_IP_BUFFER_LENGTH, "ping: send %s - error \r", ipaddr_ntoa(addr));
            QAT_Response_Str(QAT_RC_QUIET, buffer);
        }
    }

    memset((void*)buffer, 0, QAT_CMD_IP_BUFFER_LENGTH);
    snprintf(buffer, QAT_CMD_IP_BUFFER_LENGTH, "Ping statistics: Sent packets = %d, Received packets = %d \r\n", qat_ping_sent_count, qat_ping_recv_count);
    QAT_Response_Str(QAT_RC_QUIET, buffer);
    if(qat_ping_recv_count > 0){
        ret = QAT_OK;
    }

    free(recv_buf);
    free(buffer);
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

    switch (Op_Type)
    {
        case QAT_OP_EXEC_W_PARAM:
        {
            if( Parameter_Count != 4 || !Parameter_List || Parameter_List[0].Integer_Is_Valid
            || !Parameter_List[1].Integer_Is_Valid || !Parameter_List[2].Integer_Is_Valid 
            || !Parameter_List[3].Integer_Is_Valid) {
                QAT_Response_Str(QAT_RC_ERROR, "Invalid input parameter!\r\n");
                return rc;
            }
            
            ptr = Parameter_List[0].String_Value;
            if(!ipaddr_aton(ptr, &ip_addr)){
                QAT_Response_Str(QAT_RC_ERROR, NULL);
                return rc;
            }

            ping_count = (Parameter_List[1].Integer_Value == 0) ? QAT_PING_DEFAULT_COUNT : Parameter_List[1].Integer_Value;
            ping_delay = (Parameter_List[2].Integer_Value == 0) ? QAT_PING_DEFAULT_DELAY_SEC : Parameter_List[2].Integer_Value;
            ping_size = (Parameter_List[3].Integer_Value == 0) ? QAT_PING_DEFAULT_PACKET_SIZE : Parameter_List[3].Integer_Value;
            
            s = lwip_socket(AF_INET, SOCK_RAW, IP_PROTO_ICMP);
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

static struct netif * get_netif_by_device(int devid)
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

    *p_offset += snprintf(buffer + *p_offset, QAT_CMD_IP_BUFFER_LENGTH, "%s, ", ipaddr_ntoa(ip_addr));
    *p_offset += snprintf(buffer + *p_offset, QAT_CMD_IP_BUFFER_LENGTH, "%s, ", ipaddr_ntoa(gw));
    *p_offset += snprintf(buffer + *p_offset, QAT_CMD_IP_BUFFER_LENGTH, "%s, ", ipaddr_ntoa(netmask));
    *p_offset += snprintf(buffer + *p_offset, QAT_CMD_IP_BUFFER_LENGTH, "%s, ", ipaddr_ntoa(dns1));
    *p_offset += snprintf(buffer + *p_offset, QAT_CMD_IP_BUFFER_LENGTH, "%s\r\n", ipaddr_ntoa(dns2));
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
    char *buffer = NULL;
    ip_addr_t default_dns = IPADDR4_INIT_BYTES(8,8,8,8);

    buffer = malloc(QAT_CMD_IP_BUFFER_LENGTH);
    if(!buffer)
    {
        QAT_IP_PRINTF("Failed to malloc cmd memory.\r\n");
        return rc;
    }
    memset((void*)buffer, 0, QAT_CMD_IP_BUFFER_LENGTH);

    switch (Op_Type)
    {
        case QAT_OP_EXEC_W_PARAM:
        {
            if( (Parameter_Count != 2) || !Parameter_List || Parameter_List[0].Integer_Is_Valid || Parameter_List[1].Integer_Is_Valid) 
            {
                QAT_Response_Str(QAT_RC_ERROR, "Invalid input parameter!\r\n");
                return rc;
            }

            netif = get_netif_by_device(STA_DEVICE);
            if(netif == NULL){
                QAT_IP_PRINTF("netif is NULL!\n");
                QAT_Response_Str(QAT_RC_ERROR, NULL);
                return rc;
            }

            interface_name = Parameter_List[0].String_Value;
            if(strcmp(interface_name, "wlan1") != 0 ) {
                QAT_Response_Str(QAT_RC_ERROR, "Just wlan1 support DHCP client mode currently\r\n");
                return rc;
            }

            action = Parameter_List[1].String_Value;
            if((!memcmp(action,"new",3))) {
                netif_set_addr(netif, IP4_ADDR_ANY4, IP4_ADDR_ANY4, IP4_ADDR_ANY4);
                etharp_cleanup_netif(netif);
                status = dhcp_start(netif);
                if( status != ERR_OK){
                    QAT_Response_Str(QAT_RC_ERROR, "DHCP client start failed\n");
                    return rc;
                }

                if(isDhcpSucceed(netif)){
                    offset += snprintf(buffer + offset, QAT_CMD_IP_BUFFER_LENGTH, "+CIPDHCPV4C=");
                    qat_net_show_info(netif, buffer, &offset);
                }
       
                rc = QAT_Response_Str(QAT_RC_OK, buffer);           
            }
            else if(!memcmp(action,"release",7)) {
                status = dhcp_release(netif);
                if(status != ERR_OK){
                    QAT_Response_Str(QAT_RC_ERROR, "DHCP client release failed\n");
                    return rc;
                }
                dhcp_stop(netif);
                netif_set_addr(netif, IP4_ADDR_ANY4, IP4_ADDR_ANY4, IP4_ADDR_ANY4);
                dns_setserver(QAT_DNS_SERVER_INDEX0, &default_dns);
                dns_setserver(QAT_DNS_SERVER_INDEX1, &default_dns);
                if(isDhcpReleased(netif)){
                    offset += snprintf(buffer + offset, QAT_CMD_IP_BUFFER_LENGTH, "+CIPDHCPV4C=");
                    qat_net_show_info(netif, buffer, &offset);
                }
                rc = QAT_Response_Str(QAT_RC_OK, buffer);
            }
            else {
                QAT_Response_Str(QAT_RC_ERROR, "DHCP Command Failed due to invalid option i.e. start/release.\r\n\r\n");
                return rc;
            }
            break;
        }
    }
    free(buffer);
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
    char *buffer = NULL;
    int offset;

    buffer = malloc(QAT_CMD_IP_BUFFER_LENGTH);
    if(!buffer)
    {
        QAT_Response_Str(QAT_RC_ERROR, NULL);
        return rc;
    }

    switch (Op_Type)
    {
        case QAT_OP_QUERY:
        {
            offset = 0;
            memset((void*)buffer, 0, QAT_CMD_IP_BUFFER_LENGTH);
            NETIF_FOREACH(netif) {
                offset += snprintf(buffer + offset, QAT_CMD_IP_BUFFER_LENGTH, "+CIPSTA=");
                qat_net_show_info(netif, buffer, &offset);
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
                QAT_IP_PRINTF("Invalid input parameter!\r\n");
                goto fail;
            }

            if ((!ipaddr_aton(Parameter_List[0].String_Value, &ip_addr)) || (!ipaddr_aton(Parameter_List[1].String_Value, &netmask))
                || (!ip_addr_netmask_valid(ip_2_ip4(&netmask))) || (!ipaddr_aton(Parameter_List[2].String_Value, &gw))
                || (!ipaddr_aton(Parameter_List[3].String_Value, &dns1)) || (!ipaddr_aton(Parameter_List[4].String_Value, &dns2)))
            {
                QAT_IP_PRINTF("Invalid input params!\r\n");
                goto fail;
            }

            netif = get_netif_by_device(STA_DEVICE);
            if(netif == NULL) {
                QAT_IP_PRINTF("network interface not initialized\r\n");
                goto fail;
            }

            netif_set_ipaddr(netif, (const ip4_addr_t*)ip_2_ip4(&ip_addr));
            netif_set_netmask(netif, (const ip4_addr_t*)ip_2_ip4(&netmask));
            netif_set_gw(netif, (const ip4_addr_t*)ip_2_ip4(&gw));
            dns_setserver(QAT_DNS_SERVER_INDEX0, &dns1);
            dns_setserver(QAT_DNS_SERVER_INDEX1, &dns2);

            offset += snprintf(buffer + offset, QAT_CMD_IP_BUFFER_LENGTH, "+CIPSTA=");
            qat_net_show_info(netif, buffer, &offset);
            rc = QAT_Response_Str(QAT_RC_OK, buffer);
            break;
        }
    }
    free(buffer);
    return rc;
fail:
    free(buffer);
    QAT_Response_Str(QAT_RC_ERROR, NULL);
    return rc;
}

void Initialize_QAT_TCPIP_Demo (void)
{
	qbool_t RetVal;
	RetVal = QAT_Register_Command_Group(QAT_TCPIP_Command_List, TCPIP_COMMAND_LIST_SIZE);
	if(RetVal == false)
   {
      QAT_IP_PRINTF("Failed to register tcpip command group.\r\n");
   }
}

