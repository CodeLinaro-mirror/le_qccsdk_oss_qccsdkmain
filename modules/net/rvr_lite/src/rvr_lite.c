/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*========================================================================
 * @brief Function definitions for RVR Lite.
 *=======================================================================*/

/*------------------------------------------------------------------------
 * Include Files
 * ----------------------------------------------------------------------*/
#include "qapi_status.h"
#include "net_shell.h"
#include "err.h"
#include "hal_int_powersave.h"
#include "netif.h"
#include "network_al.h"
#include "nt_flags.h"
#include "nt_logger_api.h"
#include "nt_timer.h"
#include "rvr_lite.h"
#include "sockets.h"
#include "tcpip.h"
#include "udp.h"
#include "network_al.h"

// int t

#ifdef CONFIG_SUPPOR_RVR_LITE
/*------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * ----------------------------------------------------------------------*/
#define RVR_LITE_THREAD_STACK_SIZE 300
#define RVR_LITE_THREAD_PRIORITY   TCPIP_THREAD_PRIO
#define RVR_LITE_THREAD_QUEUE_LEN  21 /* timer event msg queue */
#define RVR_LITE_CMD_QUEUE_LEN     7  /* timer cmd msg queue */
#define MAX_BLOCK_WAIT             10
/*-------------------------------------------------------------------------
 * Global Data Definitions
 * ----------------------------------------------------------------------*/

/*-------------------------------------------------------------------------
 * Static Data Definitions
 * ----------------------------------------------------------------------*/
static rvr_lite_server_ctx_t g_rvr_lite_server;
static rvr_lite_client_ctx_t g_rvr_lite_client;

static bool b_rvr_lite_tx_thread_created = FALSE;
static bool b_rvr_lite_started = FALSE;
static bool b_offset_added = FALSE;
static nt_osal_task_handle_t rvr_lite_thread_hdl;
static nt_osal_queue_handle_t rvr_lite_thread_queue;
static nt_osal_queue_handle_t rvr_lite_tmr_cmd_queue;
static nt_osal_semaphore_handle_t rvr_lite_mutex = NULL;
static task_info_type rvr_lite_thread_info;
static uint64_t g_dl_latency;

static NT_BOOL wlan_unit_test_rvr_lite(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List);

#endif /* CONFIG_SUPPOR_RVR_LITE */

qapi_Status_t rvr(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    if (wlan_unit_test_rvr_lite(Parameter_Count, Parameter_List))
        return QAPI_OK;
    else
        return QAPI_ERROR;
}

qapi_Status_t rvr_stop(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    if (wlan_unit_test_rvr_lite(Parameter_Count, Parameter_List))
        return QAPI_OK;
    else
        return QAPI_ERROR;
}

#ifdef CONFIG_SUPPOR_RVR_LITE

/*-----------------------------------------------------------------------------
 * @function   wlan_unit_test_rvr_lite
 *
 * @brief      wlan unit test RVR Lite handler function.
 *             args[0-3] => IPv4 Address in bytes
 *             args[4] => Remote port
 *             args[5] => Local port
 * @param  cmd: Pointer to the command from user layer containing arguments
 * @return None
 *------------------------------------------------------------------------------
 */
static NT_BOOL wlan_unit_test_rvr_lite(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    uint32_t args[WMI_UNIT_TEST_ARGS_MAX] = {0};
    uint16_t num_args = Parameter_Count;
    uint8_t netif_id = DEFAULT_NETIF_IDX;
    uint8_t argc_index = 0;

    if (num_args < 1) {
        NT_LOG_WMI_ERR("UNIT_TEST_RVR_LITE: Insufficient arguments ", num_args, 0, 0);
        NT_LOG_WMI_ERR(
            "rvr  <no_args> <start-1/stop-0> <ip addr in bytes> <r_port> <l_port> <bytes> <duration US> \
<ip_tos>r\n",
            0, 0, 0);
        return FALSE;
    }

    for (argc_index = 0; argc_index < num_args; argc_index++) {
        args[argc_index] = Parameter_List[1 + argc_index].Integer_Value;
    }

    NT_LOG_WMI_ERR("UNIT_TEST_RVR_LITE", num_args, 0, 0);

    switch (args[0]) {
        case 0:
            /* stop rvr lite */
            rvr_lite_stop_req(args[1]);
            break;
        case 1:
            if (num_args < 13) {
                NT_LOG_WMI_ERR("UNIT_TEST_RVR_LITE: very few arguments for rvr lite start- \r\n", num_args, 0, 0);
                return FALSE;
            }
            /* construct IPv4 address from bytes */
            ip_addr_t remote_ip =
                IPADDR4_INIT_BYTES((uint8_t)args[1], (uint8_t)args[2], (uint8_t)args[3], (uint8_t)args[4]);
            if (IP_GET_TYPE(&remote_ip) == IPADDR_TYPE_V4) {
                NT_LOG_PRINT(WIFI_APP, ERR, "Remote IPv4: %d.%d.%d.%d", ip4_addr1(ip_2_ip4(&remote_ip)),
                             ip4_addr2(ip_2_ip4(&remote_ip)), ip4_addr3(ip_2_ip4(&remote_ip)),
                             ip4_addr4(ip_2_ip4(&remote_ip)));
            }
            /* start rvr lite */
            rvr_lite_start_req(&remote_ip, args[5], args[6], netif_id, args[7], args[8], args[9], args[10], args[11]);
            break;
        default:
            NT_LOG_WMI_ERR("UNIT_TEST_RVR_LITE: Invalid Command ", num_args, args[0], 0);
    }
    return TRUE;
}
#else
static NT_BOOL wlan_unit_test_rvr_lite(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    NT_LOG_WMI_ERR("UNIT_TEST_RVR_LITE_NOT_SUPPORTED", Parameter_Count, 0, 0);

    return 0;
}
#endif /* CONFIG_SUPPOR_RVR_LITE */

#ifdef CONFIG_SUPPOR_RVR_LITE
/*------------------------------------------------------------------------
 * Internal Function Definitions
 * ----------------------------------------------------------------------*/
/**
 * @brief Get function for RVR Lite server context
 *
 * @return Pointer to RVR Lite server context
 */
static rvr_lite_server_ctx_t *rvr_lite_get_server_ctx(void)
{
    return &g_rvr_lite_server;
}

/**
 * @brief Get function for RVR Lite client context
 *
 * @return Pointer to RVR Lite client context
 */
static rvr_lite_client_ctx_t *rvr_lite_get_client_ctx(void)
{
    return &g_rvr_lite_client;
}

/**
 * @brief Timer callback function that is executed upon timer expiry
 *
 * @param timer_handle Timer handle for which the timer expired
 * @return none
 */
static void rvr_lite_tx_timer_cb(uint32_t timer_handle)
{
    (void)timer_handle;
    int err = 0;
    uint8_t *send_buf = NULL;
    rvr_lite_client_ctx_t *client_ctx = NULL;
    uint64_t gen_ts = 0;
    uint64_t tx_ts = 0;
    uint64_t chunk_seqno = 0;
    uint64_t dl_letancy = 0;

    /* Get tsf timestamp */
    gen_ts = nt_hal_tsf_get();

    client_ctx = rvr_lite_get_client_ctx();
    if (client_ctx->conn_state == 0) {
        NT_LOG_PRINT(COMMON, ERR, "rvr_lite_tx: connection error\r\n");
        return;
    }
    /* Increment sequence no. for next tx packet */
    client_ctx->seq_no++;
    chunk_seqno = client_ctx->seq_no;

    send_buf = (uint8_t *)nt_osal_allocate_memory(client_ctx->nbytes);
    if (send_buf == NULL) {
        NT_LOG_PRINT(COMMON, ERR, "rvr_lite_tx: mem alloc failed\r\n");
        return;
    }
    memset(send_buf, 0, client_ctx->nbytes);

    if (nt_fail == nt_osal_semaphore_take(rvr_lite_mutex, MAX_BLOCK_WAIT)) {
        NT_LOG_PRINT(COMMON, ERR, "RVR Lite failed to take semaphore");
    }

    dl_letancy = g_dl_latency;

    if (nt_fail == nt_osal_semaphore_give(rvr_lite_mutex)) {
        NT_LOG_PRINT(COMMON, ERR, "RVR Lite failed to give semaphore");
    }

    rvr_header_t *rvr_hdr = (rvr_header_t *)send_buf;
    /* flow_id to identify the stream*/
    rvr_hdr->flow_id = htonll(client_ctx->flow_id);
    /* generation tsf timestamp */
    rvr_hdr->gen_ts = htonll(gen_ts);
    /* sequence no. of the super chunk. default to max since current
       implementation expects only one chunk*/
    rvr_hdr->superchunk_sn = htonl(0xFFFFFFFF);
    /* sequence number of the chunk */
    rvr_hdr->chunk_seqno = htonll(chunk_seqno);
    /* fragment sequence no. Default to 1 since no fragmentation*/
    rvr_hdr->num_frags = htons(1);
    /* one-way dl latency */
    rvr_hdr->rev_ts = htonll(dl_letancy);
    tx_ts = nt_hal_tsf_get();
    /* received packet tsf timestamp */
    rvr_hdr->tx_ts = htonll(tx_ts);

    err = sendto(client_ctx->s_fd, send_buf, client_ctx->nbytes, 0, (const struct sockaddr *)&client_ctx->rem_addr,
                 sizeof(client_ctx->rem_addr));
    if (err < 0) {
        NT_LOG_PRINT(COMMON, ERR, "rvr_lite_tx: send failed\r\n");
    }
    nt_osal_free_memory(send_buf);
    NT_LOG_PRINT(COMMON, INFO, "seq_no: %d, gen_ts: %u %u, tx_ts: %u %u", client_ctx->seq_no, HI_BITS(gen_ts),
                 LO_BITS(gen_ts), HI_BITS(tx_ts), LO_BITS(tx_ts));
}

/**
 * @brief RVR_Lite Tx Thread
 *
 * @param pvParameters Thread context parameter
 */
static void rvr_lite_tx_thread(void *pvParameters)
{
    BaseType_t xResult;
    uint32_t notified_value = 0;
    timer_cmd_t cmd_msg;
    rvr_lite_client_ctx_t *client_ctx = (rvr_lite_client_ctx_t *)pvParameters;

    for (;;) {
        xResult = xTaskNotifyWait(pdFALSE, ULONG_MAX, &notified_value, portMAX_DELAY);
        if (xResult == pdPASS) {
            if ((notified_value & (1 << RVR_LITE_EVENT_TMR_EXPIRY)) != 0) {
                /* call wrapper function to handle timer callback */
                hres_timer_handler(rvr_lite_thread_queue);
            }
            if ((notified_value & (1 << RVR_LITE_EVENT_TMR_CMD)) != 0) {
                /* To set timer*/
                while (pdPASS == xQueueReceive(rvr_lite_tmr_cmd_queue, &cmd_msg, 0)) {
                    if (cmd_msg.cmd_id == TMR_CMD_ID_TIMEOUT) {
                        /* Handle required offset with multiple devices */
                        if (!b_offset_added) {
                            b_offset_added = TRUE;
                            while ((LO_BITS(nt_hal_tsf_get()) % client_ctx->duration) != ((uint32_t)client_ctx->offset))
                                ;
                        }
                        /* Start hrest timer from same thread context */
                        hres_timer_set_64(cmd_msg.p_handle, cmd_msg.time, cmd_msg.reload, cmd_msg.unit);
                    }
                }
            }
        }
    } /* for(;;) */
}

/**
 * @brief Creates RVR Lite thread
 *
 * @param client_ctx Client context
 * @return Returns pdPASS on success, else pdFAIL
 */
static uint8_t rvr_lite_thread_create(rvr_lite_client_ctx_t *client_ctx)
{
    uint8_t ret_val = pdPASS;

    if (!b_rvr_lite_tx_thread_created) {
        ret_val = (uint8_t)nt_qurt_thread_create(rvr_lite_tx_thread, "rvr_lite", RVR_LITE_THREAD_STACK_SIZE, client_ctx,
                                                 RVR_LITE_THREAD_PRIORITY, &rvr_lite_thread_hdl);
        if (ret_val != pdPASS) {
            NT_LOG_PRINT(COMMON, ERR, "rvr_lite thread creation failed");
            return ret_val;
        }
        /* Msg queue to handle timer events from HRES timer thread */
        rvr_lite_thread_queue = nt_qurt_pipe_create(RVR_LITE_THREAD_QUEUE_LEN, sizeof(timer_msg_t));
        /* Msg queue to handle timer commands */
        rvr_lite_tmr_cmd_queue = nt_qurt_pipe_create(RVR_LITE_CMD_QUEUE_LEN, sizeof(timer_cmd_t));
        /* Tx thread mutex */
        rvr_lite_mutex = nt_osal_create_mutex();
        if (rvr_lite_thread_queue == NULL || rvr_lite_tmr_cmd_queue == NULL || rvr_lite_mutex == NULL) {
            qurt_pipe_delete(rvr_lite_thread_queue);
            qurt_pipe_delete(rvr_lite_tmr_cmd_queue);
            nt_osal_thread_delete(rvr_lite_thread_hdl);
            if (rvr_lite_mutex != NULL) {
                nt_osal_semaphore_delete(rvr_lite_mutex);
                rvr_lite_mutex = NULL;
            }
            ret_val = pdFAIL;
            NT_LOG_PRINT(COMMON, ERR, "rvr_lite_thread queue creation failed");
        } else {
            /* Thread creation success. Fill up task info required for hres
               timer */
            rvr_lite_thread_info.handle = rvr_lite_thread_hdl;
            rvr_lite_thread_info.event = RVR_LITE_EVENT_TMR_EXPIRY,
            rvr_lite_thread_info.timer_queue = rvr_lite_thread_queue;
            client_ctx->task_info = &rvr_lite_thread_info;
            b_rvr_lite_tx_thread_created = TRUE;
            NT_LOG_PRINT(COMMON, ERR, "rvr_lite thread created");
        }
    }
    return ret_val;
}

/**
 * @brief Delete RVR Lite thread
 *
 * @param context Context parameter
 */
static void rvr_lite_thread_delete(void *context)
{
    rvr_lite_client_ctx_t *client_ctx = (rvr_lite_client_ctx_t *)context;
    if (b_rvr_lite_tx_thread_created) {
        memset(client_ctx->task_info, 0, sizeof(task_info_type));
        if (rvr_lite_mutex != NULL) {
            nt_osal_semaphore_delete(rvr_lite_mutex);
            rvr_lite_mutex = NULL;
        } else {
            NT_LOG_PRINT(COMMON, ERR, "attempt to delete semaphore rvr_lite_mutex which is NULL");
        }
        qurt_pipe_delete(rvr_lite_thread_queue);
        qurt_pipe_delete(rvr_lite_tmr_cmd_queue);
        nt_osal_thread_delete(rvr_lite_thread_hdl);
        rvr_lite_thread_queue = NULL;
        rvr_lite_tmr_cmd_queue = NULL;
        rvr_lite_thread_hdl = NULL;

        b_rvr_lite_tx_thread_created = FALSE;
        NT_LOG_PRINT(COMMON, ERR, "rvr_lite thread deleted");
    }
}

/**
 * @brief Notify RVR_Lite thread to start hres timer
 *
 * @param timer_hdl Timer handle
 * @param duration Timer duration in microseconds
 */
static void tx_timer_start(timer_type *timer_hdl, time_timetick_type duration)
{
    timer_cmd_t tmr_cmd;

    tmr_cmd.reload = TRUE; /* periodic */
    tmr_cmd.cmd_id = TMR_CMD_ID_TIMEOUT;
    tmr_cmd.unit = T_USEC;
    tmr_cmd.p_handle = timer_hdl;
    tmr_cmd.time = duration;
    /*Insert into msg queue*/
    if (pdTRUE != xQueueSend(rvr_lite_tmr_cmd_queue, &tmr_cmd, MAX_BLOCK_WAIT)) {
        NT_LOG_PRINT(COMMON, ERR, "tx_timer_start: posting msg timeout");
        return;
    }
    /*Notify the task*/
    if (pdTRUE != xTaskNotify(rvr_lite_thread_hdl, 1 << RVR_LITE_EVENT_TMR_CMD, eSetBits)) {
        NT_LOG_PRINT(COMMON, ERR, "tx_timer_start: posting notification failed");
    }
}

/**
 * @brief Callback triggered when some data is received on rvr_lite
 * @param arg user supplied argument (udp_pcb.recv_arg)
 * @param pcb the udp_pcb which received data
 * @param pb the packet buffer that was received
 * @param addr the remote IP address from which the packet was received
 * @param port the remote port from which the packet was received
 */
static void cb_recv_rvr_lite_udp(void *arg, struct udp_pcb *pcb, struct pbuf *pb, const ip_addr_t *addr, uint16_t port)
{
    LWIP_UNUSED_ARG(pcb);

    configASSERT(arg != NULL);
    configASSERT(pb != NULL);
    configASSERT(addr != NULL);
    rvr_header_t *msg_in = NULL;
    uint64_t gen_ts = 0;
    uint64_t rev_ts = 0;
    uint64_t rx_tsf = 0;
    uint32_t superchunk_sn = 0;

    /* Get tsf timestamp */
    rx_tsf = nt_hal_tsf_get();
    if (IP_GET_TYPE(addr) == IPADDR_TYPE_V4) {
        NT_LOG_PRINT(COMMON, INFO, "Remote IPv4: %d.%d.%d.%d", ip4_addr1(ip_2_ip4(addr)), ip4_addr2(ip_2_ip4(addr)),
                     ip4_addr3(ip_2_ip4(addr)), ip4_addr4(ip_2_ip4(addr)));
        NT_LOG_PRINT(COMMON, INFO, "r_port: %d", port);
    }

    rvr_lite_server_ctx_t *server_ctx = (rvr_lite_server_ctx_t *)arg;
    /* Check if the incoming packet is from valid remote IP */
    if (ip_addr_cmp(&server_ctx->remote_ip, addr)) {
        NT_LOG_PRINT(COMMON, INFO, "cb_recv_rvr_lite_udp: recvd packet, len=%d", pb->len);

        /* Decode RVR Lite Header */
        msg_in = (rvr_header_t *)pb->payload;
        gen_ts = ntohll(msg_in->gen_ts);
        rev_ts = ntohll(msg_in->rev_ts);
        superchunk_sn = ntohl(msg_in->superchunk_sn);

        if (nt_fail == nt_osal_semaphore_take(rvr_lite_mutex, MAX_BLOCK_WAIT)) {
            NT_LOG_PRINT(COMMON, ERR, "RVR Lite failed to take semaphore");
        }

        /* One-way DL latency. Required for TX header. */
        g_dl_latency = TIME_DIFF_WITH_WRAP_U64(rx_tsf, rev_ts);

        if (nt_fail == nt_osal_semaphore_give(rvr_lite_mutex)) {
            NT_LOG_PRINT(COMMON, ERR, "RVR Lite failed to give semaphore");
        }

        NT_LOG_PRINT(COMMON, INFO, "gen_ts: %u %u, rev_ts: %u %u, superchunk_sn: %u, rx_tsf: %u %u", HI_BITS(gen_ts),
                     LO_BITS(gen_ts), HI_BITS(rev_ts), LO_BITS(rev_ts), superchunk_sn, HI_BITS(rx_tsf),
                     LO_BITS(rx_tsf));
    } else {
        NT_LOG_PRINT(COMMON, ERR, "cb_recv_rvr_lite_udp: Invalid remote host");
    }

    if (pb != NULL) {
        /* Free RX Pbuf */
        pbuf_free(pb);
    }
}

/**
 * @brief Start RVR Lite Server
 *
 * @param netif_id Network interface Id
 * @param remote_ip Remote IP address
 * @param local_port Local port
 * @param server_ctx RVR Lite server context
 */
static err_t rvr_lite_server_start(uint8_t netif_id, ip_addr_t *remote_ip, uint16_t local_port,
                                   rvr_lite_server_ctx_t *server_ctx)
{
    uint8_t ip_ver = 0;
    ip_addr_t *local_ip;
    struct netif *p_udp_netif = NULL;
    struct udp_pcb *p_new_pcb = NULL;
    err_t error_code = ERR_OK;

    if (server_ctx == NULL) {
        NT_LOG_PRINT(COMMON, ERR, "rvr_lite_server_start: server context NULL");
        return ERR_ARG;
    }

    /* get netif structure from netif_id */
    p_udp_netif = netif_get_by_index(netif_id);
    if (TRUE == nt_dpm_is_netif_ready(p_udp_netif)) {
        /* find out ip_ver */
        ip_ver = IP_GET_TYPE(remote_ip);

        /* get local ip address based on remote's ip version */
        if (ip_ver == IPADDR_TYPE_V4) {
            local_ip = (ip_addr_t *)nt_dpm_get_ip(p_udp_netif, IPADDR_TYPE_V4, IPv4_IP_IDX);
            if (ip_addr_isany(local_ip)) {
                NT_LOG_PRINT(COMMON, ERR, "IPv4 address pointer is NULL or IPv4 address is NULL");
                return ERR_ARG;
            }
        } else {
            /* ipv6 is not tested */
            local_ip = (ip_addr_t *)nt_dpm_get_ip(p_udp_netif, IPADDR_TYPE_V6, IPv6_LINK_LOCAL_IDX);
            if (ip_addr_isany(local_ip)) {
                NT_LOG_PRINT(COMMON, ERR, "IPv6 address pointer is NULL or IPv6 address is NULL");
                return ERR_ARG;
            }
        }

        p_new_pcb = udp_new_ip_type(ip_ver);

        if (p_new_pcb != NULL) {
            /* bind ip address and port for incoming udp packets */
            error_code = udp_bind(p_new_pcb, local_ip, local_port);
            if (error_code == ERR_OK) {
                /* bind to specific network interface id*/
                udp_bind_netif(p_new_pcb, p_udp_netif);

                server_ctx->netif_id = netif_id;
                server_ctx->conn_state = 1;  // 0 - not-connected, 1 - connected
                server_ctx->local_port = local_port;
                server_ctx->pcb = p_new_pcb;
                memscpy(&server_ctx->remote_ip, sizeof(ip_addr_t), remote_ip, sizeof(ip_addr_t));

                /* listen for udp packets */
                udp_recv(p_new_pcb, cb_recv_rvr_lite_udp, server_ctx);
            } else {
                udp_remove(p_new_pcb);
            }
        } else {
            error_code = ERR_MEM;
        }
    } else {
        NT_LOG_PRINT(COMMON, ERR, "rvr_lite_server_start: Netif not ready, err=%d", error_code);
        error_code = ERR_CONN;
    }
    return error_code;
}

/**
 * @brief Start RVR Lite Client
 *
 * @param remote_ip Remote IP address
 * @param remote_port Remote port
 * @param client_ctx RVR Lite client context
 */
static err_t rvr_lite_client_start(ip_addr_t *remote_ip, uint16_t remote_port, rvr_lite_client_ctx_t *client_ctx)
{
    (void)remote_ip;
    (void)remote_port;
    (void)client_ctx;
    err_t error_code = ERR_OK;
    int sockfd;
    struct sockaddr_in *rem_addr;

    /* Create socket for RVR Lite Tx*/
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        NT_LOG_PRINT(COMMON, ERR, "socket creation failed");
        return ERR_CONN;
    }
    /* Set IP TOS */
    if (setsockopt(sockfd, IPPROTO_IP, IP_TOS, &client_ctx->tos, sizeof(client_ctx->tos)) != 0) {
        NT_LOG_PRINT(COMMON, ERR, "socketopt failed");
        closesocket(sockfd);
        return ERR_CONN;
    }

    rem_addr = &client_ctx->rem_addr;
    rem_addr->sin_family = AF_INET;
    rem_addr->sin_port = htons(client_ctx->remote_port);
    rem_addr->sin_len = sizeof(rem_addr);
    inet_addr_from_ip4addr(&rem_addr->sin_addr, ip_2_ip4(&client_ctx->remote_ip));
    client_ctx->conn_state = 1;
    client_ctx->s_fd = sockfd;

    /* Start tx thread */
    if (pdPASS == rvr_lite_thread_create(client_ctx)) {
        /* define a high resolution timer for periodic packet generation */
        hres_timer_def(&client_ctx->timer, &rvr_lite_tx_timer_cb, client_ctx->task_info);
    } else
        error_code = ERR_CONN;

    return error_code;
}

/*------------------------------------------------------------------------
 * External Function Definitions
 * ----------------------------------------------------------------------*/
/**
 * @brief Process request to start RVR Lite
 *
 * @param remote_ip Remote IP address
 * @param remote_port Remote Port
 * @param local_port Local port
 * @param netif_id Network Interface ID
 * @param n_bytes Number of bytes in TX packets
 * @param duration Duration of timer
 * @param tos Types of service
 * @param flow_id Flow id of the stream
 * @param offset Offset for TX generation
 *
 * @return pdTRUE on success else pdFALSE
 */
BaseType_t rvr_lite_start_req(ip_addr_t *remote_ip, uint16_t remote_port, uint16_t local_port, uint8_t netif_id,
                              uint16_t n_bytes, uint32_t duration, uint8_t tos, uint32_t flow_id, uint32_t offset)
{
    err_t error_code = ERR_OK;
    rvr_lite_server_ctx_t *server_ctx = NULL;
    rvr_lite_client_ctx_t *client_ctx = NULL;

    if (b_rvr_lite_started) {
        NT_LOG_PRINT(COMMON, ERR, "rvr_lite_start_req: error, rvr_lite instance already running");
        return pdFALSE;
    }

    server_ctx = rvr_lite_get_server_ctx();
    client_ctx = rvr_lite_get_client_ctx();

    /* start rvr lite server */
    error_code = rvr_lite_server_start(netif_id, remote_ip, local_port, server_ctx);
    if (error_code != ERR_OK) {
        NT_LOG_PRINT(COMMON, ERR, "rvr_lite_start_req: failed to start server, err=%d", error_code);
        return pdFALSE;
    }

    /* prepare client context */
    client_ctx->remote_port = remote_port;
    memscpy(&client_ctx->remote_ip, sizeof(ip_addr_t), remote_ip, sizeof(ip_addr_t));
    client_ctx->nbytes = n_bytes;  // number to bytes in tx packet
    client_ctx->seq_no = 0;
    client_ctx->tos = tos;
    client_ctx->flow_id = flow_id;
    client_ctx->offset = offset;
    client_ctx->duration = duration;

    /* start rvr lite client */
    error_code = rvr_lite_client_start(remote_ip, remote_port, client_ctx);
    if (error_code != ERR_OK) {
        NT_LOG_PRINT(COMMON, ERR, "rvr_lite_start_req: failed to start client, err=%d", error_code);
        return pdFALSE;
    }

    g_dl_latency = 0;
    b_rvr_lite_started = TRUE;
    /* start tx timer */
    tx_timer_start(&client_ctx->timer, client_ctx->duration);
    NT_LOG_PRINT(
        COMMON, ERR,
        "rvr_lite started: err=%d, netif_id=%d, rport=%d, lport=%d, nbytes=%d, duration=%d, flow_id=%d, offset=%d",
        error_code, netif_id, remote_port, local_port, n_bytes, duration, flow_id, offset);

    return pdTRUE;
}

/**
 * @brief Process request to stop RVR Lite UDP
 *
 * @param netif_id Network interface id
 */
void rvr_lite_stop_req(uint8_t netif_id)
{
    rvr_lite_server_ctx_t *server_ctx = rvr_lite_get_server_ctx();
    rvr_lite_client_ctx_t *client_ctx = rvr_lite_get_client_ctx();
    if (netif_id != server_ctx->netif_id) {
        NT_LOG_PRINT(COMMON, ERR, "rvr_lite_stop_req: Invalid netif id\r\n");
        return;
    }
    if (b_rvr_lite_started) {
        /* stop server*/
        udp_disconnect(server_ctx->pcb);
        udp_remove(server_ctx->pcb);
        server_ctx->conn_state = 0;  // 0 - not-connected, 1 - connected
        memset(server_ctx, 0, sizeof(rvr_lite_server_ctx_t));

        /* stop client*/
        hres_timer_undef(&client_ctx->timer);   // delete hres timer
        shutdown(client_ctx->s_fd, SHUT_RDWR);  // disable socket
        closesocket(client_ctx->s_fd);          // close socket
        b_offset_added = FALSE;
        rvr_lite_thread_delete(client_ctx);
        client_ctx->conn_state = 0;  // 0 - not-connected, 1 - connected
        memset(client_ctx, 0, sizeof(rvr_lite_client_ctx_t));

        b_rvr_lite_started = FALSE;
        NT_LOG_PRINT(COMMON, ERR, "rvr_lite_stop_req: RVR Lite stopped, netif=%d\r\n", netif_id);
    } else {
        NT_LOG_PRINT(COMMON, ERR, "rvr_lite_stop_req: RVR Lite not started\r\n");
    }
}
#endif  // CONFIG_SUPPOR_RVR_LITE
