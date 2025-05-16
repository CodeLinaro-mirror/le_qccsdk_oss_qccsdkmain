/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
 
#include <string.h>
#include <stdint.h>
#include "wifi_cmn.h"
#include "qapi_wlan.h"
#include "qapi_console.h"

#include "qurt_internal.h"
#include "qurt_mutex.h"

#include "qapi_net_status.h"
#include "lwip/pbuf.h"
#include "lwip/netif.h"
#include "lwip/tcpip.h"
#include "lwip/sockets.h"
#include "ip_addr.h"
#include "netifapi.h"
#include "data_path.h"
#if NT_FN_DHCPS_V4
#include "lwip/apps/nt_dhcps.h"
#include "dns.h"
#include "ip_addr.h"
#endif /* NT_FN_DHCPS_V4 */
#if NT_FN_DHCP6
#include "dhcp6.h"
#endif
#if LWIP_AUTOIP
#include "autoip.h"
#endif
#if LWIP_DNS
#include "dns.h"
#endif
#include "network_al.h"
#include "wifi_fw_pmu_ts_cfg.h"
#include "iperf.h"
#include "tcpip.h"
#include "nt_timer.h"
#include "wmi.h"

#define DEFAULT_NETIF_IDX   netif_get_index(netif_default) /* Default netif idx */

#ifdef FERMION_SILICON
extern uint32_t UART_Send_direct(char *txbuf,uint32_t buflen);
#define UART_SEND_DIRECT(str)   UART_Send_direct((str),strlen(str))
#else
#define UART_SEND_DIRECT(str)   
#endif

#define info_printf(msg,...)     printf("WLAN: " msg, ##__VA_ARGS__)

#ifndef DEV_STA_ID
#define DEV_STA_ID			1
#endif
#define min(a, b) (((a) < (b)) ? (a) : (b))

/**********************************************************************/
QAPI_Console_Group_Handle_t powertest_shell_cmd_group_handle;
TimerHandle_t  iperf_timer;
THROUGHPUT_CXT *dtim_iperf_tCxt = NULL;

extern qapi_Status_t qapi_pm_enable(uint8_t enable);
extern qapi_Status_t wmi_cmd_send (WMI_COMMAND_ID cmd_id, void *p_data, uint32_t data_len);
extern void qurt_thread_sleep (uint32 duration);

extern union {
    WMI_SLP_CLK_CAL_CFG slp_clk_cal;
    WMI_BMPS_ENABLE bmps_enable;
    WMI_BMPS_IGNORE_BCMC bmps_ignore_bcmc;
    WMI_BMPS_IDLE_TIME bmps_idle_time;
    WMI_BMPS_TIMING_CFG bmps_timing;
    WMI_IMPS_CFG imps_cfg;
} g_lowpower_wmi;

void pm_enable()
{

    info_printf("=====  sleep  =====\r\n"); 
    qapi_pm_enable(1);

    WMI_BMPS_IGNORE_BCMC *pdata = (WMI_BMPS_IGNORE_BCMC *)&g_lowpower_wmi;
    memset(pdata, 0, sizeof(*pdata));
    pdata->enable = 1;
    wmi_cmd_send(WMI_BMPS_IGNORE_BCMC_CMDID, pdata, sizeof(*pdata));

    WMI_BMPS_ENABLE *pdata2 = (WMI_BMPS_ENABLE *)&g_lowpower_wmi;
    memset(pdata2, 0, sizeof(*pdata2));
    pdata2->enable = 1;
    wmi_cmd_send(WMI_BMPS_ENABLE_CMDID, pdata2, sizeof(*pdata2));
}

int function_net_send_cb()
{

    int32_t send_bytes;
    
    info_printf("===== send data =====\r\n");
    
    if(dtim_iperf_tCxt->buffer == NULL)
    {
        while((dtim_iperf_tCxt->buffer = malloc(dtim_iperf_tCxt->params.tx_params.packet_size))==NULL)
        {
            qurt_thread_sleep(100);
        }
    }
    pattern(dtim_iperf_tCxt->buffer, dtim_iperf_tCxt->params.tx_params.packet_size);
    
    send_bytes = send(dtim_iperf_tCxt->sock_peer, dtim_iperf_tCxt->buffer, dtim_iperf_tCxt->params.tx_params.packet_size, 0);
    info_printf("===== sent %u bytes =====\r\n",send_bytes);
    
    return QAPI_OK;    
}

qapi_Status_t iperf_for_powertest(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    BaseType_t xResult=pdFAIL;
    uint32_t notified_value = 0;
    unsigned int index = 0;
    TickType_t dtim_time = 0;
    char *receiver_ip;
    unsigned int ipAddress = 0;
    unsigned int pktSize = 0;

    dtim_iperf_tCxt = malloc(sizeof(THROUGHPUT_CXT));
    if (dtim_iperf_tCxt == NULL)
    {
        info_printf("Memory alloc failed\n");
        return QAPI_ERR_NO_MEMORY;
    }
    memset(dtim_iperf_tCxt, 0, sizeof(THROUGHPUT_CXT));

    dtim_iperf_tCxt->protocol = TCP;
    dtim_iperf_tCxt->params.tx_params.port = IPERF_DEFAULT_PORT;
    while (index < Parameter_Count)
    {
        if (0 == strcmp(Parameter_List[index].String_Value, "-u"))
        {
            index++;
            dtim_iperf_tCxt->protocol = UDP;
        }
        else if(0 == strcmp(Parameter_List[index].String_Value, "-d"))
        {
            index++;
            dtim_time = Parameter_List[index].Integer_Value;
            index++;
        }
        else if(0 == strcmp(Parameter_List[index].String_Value, "-p"))
        {
            index++;
            dtim_iperf_tCxt->params.tx_params.port = Parameter_List[index].Integer_Value;
            index++;
        }
        else if (0 == strcmp(Parameter_List[index].String_Value, "-c"))
        {
            index++;
            receiver_ip = Parameter_List[index].String_Value;
            if (inet_pton(AF_INET, receiver_ip, &ipAddress) != 1)
            {
                info_printf("Incorrect IP address %s\n", receiver_ip);
                return QAPI_ERR_INVALID_PARAM;
            }
            index++;
        }
        else if (0 == strcmp(Parameter_List[index].String_Value, "-l"))
        {
            index++;
            pktSize = Parameter_List[index].Integer_Value;
            index++;
            pktSize = pktSize < 12 ? 12 : pktSize;
        }
        else
        {
            index++;
        }
    }

    if(pktSize > 0)
    {
        if (dtim_iperf_tCxt->protocol == TCP)
        {
            dtim_iperf_tCxt->params.tx_params.packet_size = min(pktSize, IPERF_MAX_PACKET_SIZE_TCP);
        }
        else
        {
            dtim_iperf_tCxt->params.tx_params.packet_size = min(pktSize, IPERF_MAX_PACKET_SIZE_UDP);
        }
    }
    else 
    {
        if (dtim_iperf_tCxt->protocol == TCP)
        {
            dtim_iperf_tCxt->params.tx_params.packet_size = IPERF_MAX_PACKET_SIZE_TCP;
        }
        else
        {
            dtim_iperf_tCxt->params.tx_params.packet_size = IPERF_MAX_PACKET_SIZE_UDP;
        }
    }

    iperf_timer = nt_create_timer(function_net_send_cb, NULL, 
                                    NT_MS_TO_TICKS(dtim_time), TRUE);

    if(dtim_iperf_tCxt->protocol == TCP){
        if ((dtim_iperf_tCxt->sock_peer = socket(AF_INET, SOCK_STREAM, 0)) == A_ERROR)
        {
            info_printf("Socket creation failed\n");
            goto ERROR_1;
        }
    }
    else{
        if ((dtim_iperf_tCxt->sock_peer = socket(AF_INET, SOCK_DGRAM, 0)) == A_ERROR)
        {
            info_printf("Socket creation failed\n");
            goto ERROR_1;
        }
    }

    struct sockaddr_in si_other;
    memset((char *) &si_other, 0, sizeof(si_other));
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(dtim_iperf_tCxt->params.tx_params.port);
    dtim_iperf_tCxt->params.tx_params.ip_address = ipAddress;
    si_other.sin_addr.s_addr = dtim_iperf_tCxt->params.tx_params.ip_address;

    if (connect(dtim_iperf_tCxt->sock_peer, (struct sockaddr *)&si_other, sizeof(si_other)) == -1)
    {
        info_printf("Connection failed\n");
        goto ERROR_1;
    }
    
    pm_enable();
    nt_start_timer(iperf_timer);

ERROR_1:

    return QAPI_OK; 
}

const QAPI_Console_Command_t Powertest_Command_List[] =
{
    /* cmd_function                     cmd_string      usage_string              description */
    {iperf_for_powertest,              "iperf_for_powertest",     "\n\niperf_for_powertest",             "iperf in dtim sleep"},
};

const QAPI_Console_Command_Group_t Powertest_Command_Group =
{
    "Powertest",  /* Power Test */
    sizeof(Powertest_Command_List) / sizeof(QAPI_Console_Command_t),
    Powertest_Command_List,
};

void Initialize_powertest_Demo (void)
{
	powertest_shell_cmd_group_handle = QAPI_Console_Register_Command_Group(NULL, &Powertest_Command_Group);
    if (powertest_shell_cmd_group_handle) {
        info_printf("Powertest Registered \n");
    }
}