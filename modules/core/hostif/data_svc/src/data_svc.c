/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*========================================================================
 *
 * @brief Function definitions common to TCP/UDP/RawEth connections
 *=======================================================================*/

/*------------------------------------------------------------------------
 * Include Files
 * ----------------------------------------------------------------------*/

#include "data_svc_priv.h"
#include "nt_osal.h"
#include "nt_logger_api.h"

#ifdef SUPPORT_RING_IF
#ifdef SUPPORT_DATA_LOOPBACK
#include "udp.h"
#endif
/*------------------------------------------------------------------------
 * Global Data Definitions
 * ----------------------------------------------------------------------*/

static conn_handler_t *g_conn_list[DATA_SVC_MAX_CONN];
static listen_handler_t *g_listen_list[DATA_SVC_MAX_LISTEN];

#ifdef SUPPORT_DATA_LOOPBACK
static bool gb_loopback_state = FALSE;
#endif

// move all udp pcb related data loopback intialization to the ip_data_svc
/*------------------------------------------------------------------------
 * Externalized Function Defnitions
 * ----------------------------------------------------------------------*/

#ifdef SUPPORT_DATA_LOOPBACK

/*
 *disable the loopback functinality by setting state
 *to FALSE
 */
bool disable_data_loopback()
{
    gb_loopback_state = FALSE;
    RINGIF_PRINT_LOG_ERR("Loopback disabled");
    return TRUE;
}

/*
 *Enable the loopback functinality by setting state
 *to TRUE and initializing udp for sending data back
 */
bool enable_data_loopback()
{
    gb_loopback_state = TRUE;
    RINGIF_PRINT_LOG_ERR("Loopback enabled");
    return TRUE;
}

/*
*Enable or disbale loopback based on input
*@param enable      :   1 - enable loopback
                        2 - disable loopback
*@return            :   TRUE/FALSE
*/
bool data_svc_enable_disable_data_loopback(uint8_t enable)
{
    bool b_err = FALSE;
    if (DATA_SVC_LOOPBACK_ENABLE == enable) {
        b_err = enable_data_loopback();
    } else if (DATA_SVC_LOOPBACK_DISABLE == enable) {
        b_err = disable_data_loopback();
    }

    return b_err;
}

/*
 *Returns state of loopback
 */
bool data_svc_get_loopback_state()
{
    return gb_loopback_state;
}

#endif /* SUPPORT_DATA_LOOPBACK */

/*
 *create a new connection in the connection table
 *@return    :   Ptr to a conn_handler strcuture
 */
conn_handler_t *data_svc_get_new_conn_handler()
{
    conn_handler_t *p_conn_handle;
    int idx;

    for (idx = 0; idx < DATA_SVC_MAX_CONN; idx++) {
        if (g_conn_list[idx] == NULL)
            break;
    }

    if (idx < DATA_SVC_MAX_CONN) {
        p_conn_handle = (conn_handler_t *)nt_osal_allocate_memory(sizeof(conn_handler_t));
        configASSERT(NULL != p_conn_handle);

        if (p_conn_handle != NULL) {
            memset(p_conn_handle, 0, sizeof(conn_handler_t));
            p_conn_handle->idx = idx;
        }

        g_conn_list[idx] = p_conn_handle;
    } else {
        p_conn_handle = NULL;
    }

    return p_conn_handle;
}

/*
 *Return the connection handler pointer for the conn_id.
 *@param conn_id :   id of the connection for which to
 *                   provide the conn_hadnler pointer for.
 *@return    :       Ptr to required conn_handler strcuture
 */
conn_handler_t *data_svc_get_conn_handler_from_id(uint8_t conn_id)
{
    conn_handler_t *conn_handler = NULL;

    if (conn_id < DATA_SVC_MAX_CONN) {
        conn_handler = g_conn_list[conn_id];
    }

    return conn_handler;
}

/*
 *Delete the specified connection from connection table
 *@param conn_id :   id of the connection to delete.
 */

void data_svc_free_conn_handler(uint8_t conn_id)
{
    conn_handler_t *conn_handler = data_svc_get_conn_handler_from_id(conn_id);

    configASSERT(conn_handler != NULL);
    nt_osal_free_memory(conn_handler);
    g_conn_list[conn_id] = NULL;
}

/*
 *create a new listen in the listen table
 *@return    :   Ptr to a listen_handler strcuture
 */
listen_handler_t *data_svc_get_new_listen_handler()
{
    listen_handler_t *p_listen_handle;
    int idx;

    for (idx = 0; idx < DATA_SVC_MAX_LISTEN; idx++) {
        if (g_listen_list[idx] == NULL)
            break;
    }

    if (idx < DATA_SVC_MAX_LISTEN) {
        p_listen_handle = (listen_handler_t *)nt_osal_allocate_memory(sizeof(listen_handler_t));

        configASSERT(p_listen_handle != NULL) if (p_listen_handle != NULL)
        {
            p_listen_handle->idx = idx;
        }

        g_listen_list[idx] = p_listen_handle;
    } else {
        p_listen_handle = NULL;
    }

    return p_listen_handle;
}

/*
 *Return the connection handler pointer for the conn_id.
 *@param conn_id :   id of the connection for which to
 *                   provide the conn_hadnler pointer for.
 *@return    :       Ptr to required conn_handler strcuture
 */
listen_handler_t *data_svc_get_listen_handler_from_id(uint8_t listen_id)
{
    listen_handler_t *listen_handler = NULL;

    if (listen_id < DATA_SVC_MAX_CONN) {
        listen_handler = g_listen_list[listen_id];
    }

    return listen_handler;
}

/*
 *Delete the specified connection from connection table
 *@param conn_id :   id of the connection to delete.
 */

void data_svc_free_listen_handler(uint8_t listen_id)
{
    listen_handler_t *listen_handler = data_svc_get_listen_handler_from_id(listen_id);
    nt_osal_free_memory(listen_handler);
    g_listen_list[listen_id] = NULL;
}

/*
 *Call appropriate sub function to process the config message from ring
 *@param p_elem  :   Pointer to the element containing buf, len, info
 *@return none
 */
static void data_svc_process_config_pkt(void *p_elem)
{
    ring_element_t *p_local_elem = p_elem;
    configASSERT(p_local_elem != NULL);
    switch (p_local_elem->info) {
        case CONFIG_INFO_IP_CONNECT:
            data_svc_process_ip_config_pkt(p_local_elem->p_buf, p_local_elem->len);
            break;
        case CONFIG_INFO_RAW_ETH:
            data_svc_process_raweth_config_pkt(p_local_elem->p_buf, p_local_elem->len);
            break;
        default:
            NT_LOG_PRINT(COMMON, ERR, "invalid info");
            break;
    }

    ringif_free_ctrl_buf(p_local_elem->p_buf_start, 0);
    nt_osal_free_memory(p_local_elem);
}

/*
 * Copies the ring element to local buf and posts an event on to TCP/IP thread
 *@param p_elem  : Pointer to the element containing buf, len, info
 *@return        : TRUE if posting to thread successful
 */
bool data_svc_post2thread_config_pkt(void *p_elem)
{
    ring_element_t *p_local_elem = nt_osal_allocate_memory(sizeof(ring_element_t));

    if (NULL == p_local_elem) {
        return FALSE;
    }

    memcpy(p_local_elem, p_elem, sizeof(ring_element_t));

    if (ERR_OK == tcpip_try_callback((tcpip_callback_fn)data_svc_process_config_pkt, p_local_elem)) {
        return TRUE;
    } else {
        nt_osal_free_memory(p_local_elem);
        return FALSE;
    }
}

/*
 *Init function. Initialises the global variables.
 */
void data_svc_init()
{
    memset(g_conn_list, 0, sizeof(g_conn_list));
    memset(g_listen_list, 0, sizeof(g_listen_list));
}

#endif  // SUPPORT_RING_IF
