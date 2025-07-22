/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*========================================================================
 * @brief Data ring Handler function definitions
 *=======================================================================*/

/*------------------------------------------------------------------------
 * Include Files
 * ----------------------------------------------------------------------*/
#include "fwconfig_cmn.h"
#include "nt_flags.h"
#include "unistd.h"
#if defined(SUPPORT_RING_IF) || defined(SUPPORT_RING_IF_ONLY)
#include "ring_ctx_holder.h"
#include "data_ring_hdlr.h"
#include "nt_osal.h"
#include "tcpip.h"
#include "data_svc_internal_api.h"
#include "nt_logger_api.h"
#include "data_svc_hfc_priv.h"

/*------------------------------------------------------------------------
 * Global type definitions
 * ----------------------------------------------------------------------*/
/* None */

/*------------------------------------------------------------------------
 * Global Data Definitions
 * ----------------------------------------------------------------------*/
#ifdef SUPPORT_RING_IF
static uint8_t g_a2f_udp_ring_id = MAX_NUM_A2F_RINGS;
static uint8_t g_f2a_udp_ring_id = MAX_NUM_F2A_RINGS;
static uint8_t g_a2f_raweth_ring_id = MAX_NUM_A2F_RINGS;
static uint8_t g_f2a_raweth_ring_id = MAX_NUM_F2A_RINGS;
static uint8_t g_num_a2f_udp_rings = 0;
static uint8_t g_num_f2a_udp_rings = 0;
static uint8_t g_num_a2f_raweth_rings = 0;
static uint8_t g_num_f2a_raweth_rings = 0;
#endif

extern portBASE_TYPE xInsideISR;
/*------------------------------------------------------------------------
 * Static Function Declarations and Definitions
 * ----------------------------------------------------------------------*/

#ifdef SUPPORT_RING_IF
static bool ringif_refill_raweth_elem(void *p_elem)
{
    return data_svc_get_rawEth_data_buff(p_elem, DATA_RING_BUFF_SIZE);
}

static bool ringif_refill_udp_elem(void *p_elem)
{
    return data_svc_get_udp_data_buff(p_elem, DATA_RING_BUFF_SIZE);
}
static bool ringif_refill_tcp_elem(void *p_elem)
{
    return data_svc_get_tcp_data_buff(p_elem, DATA_RING_BUFF_SIZE);
}

static bool ringif_clear_raweth_elem(void *p_elem)
{
    return data_svc_free_rawEth_data_buff(p_elem);
}

static bool ringif_clear_udp_elem(void *p_elem)
{
    return data_svc_free_udp_data_buff(p_elem);
}
static bool ringif_clear_tcp_elem(void *p_elem)
{
    return data_svc_free_tcp_data_buff(p_elem);
}

static _pfn_refill_elem ringif_get_data_refill_fn(uint8_t ring_id)
{
    if (ring_id == g_a2f_raweth_ring_id) {
        return ringif_refill_raweth_elem;
    } else if (ring_id == g_a2f_udp_ring_id) {
        return ringif_refill_udp_elem;
    } else {
        return ringif_refill_tcp_elem;
    }
}

static _pfn_clear_elem ringif_get_data_clear_fn(uint8_t ring_id)
{
    if (ring_id == g_f2a_raweth_ring_id) {
        /* TBD: Change fn to the one exposed from raw eth */
        return ringif_clear_raweth_elem;
    } else if (ring_id == g_f2a_udp_ring_id) {
        /* TBD: Change fn to the one exposed from TCP/IP */
        return ringif_clear_udp_elem;
    } else {
        /* TBD: Change fn to the one exposed from TCP/IP */
        return ringif_clear_tcp_elem;
    }
}
/*
 * @brief  Get the corresponding process function of the data type
 * @param  data_ring_type : Type of data ring (udp/tcp/raweth)
 * @return _pfn_process   : Address of the process function
 *
 */
static _pfn_process get_process_fn(uint8_t data_ring_type)
{
    switch (data_ring_type) {
        case DATA_RING_TCP:
            return (void *)data_svc_process_tcp_pkt;
        case DATA_RING_RAWETH:
            return (void *)data_svc_process_raweth_pkt;
        default:
            return (void *)data_svc_process_udp_pkt;
    }
}

/*
 * @brief  Processes the new data reception in data ring
 * @param  ctx            : Context of the ring
 * @param  data_ring_type : Type of data ring (udp/tcp/raweth)
 * @return None
 */
static void handle_a2f_data_ring_update(void *ctx, uint8_t data_ring_type)
{
    uint8_t ring_id = ((ring_ctx_t *)ctx)->ring_id;
    _pfn_process pfn_process = get_process_fn(data_ring_type);
    _pfn_refill_elem pfn_refill = ringif_get_data_refill_fn(ring_id);
    uint8_t num_pkts_processed = ringif_a2f_process_pkts(ring_id, pfn_process, pfn_refill);

    RINGIF_PRINT_LOG_INFO("A2F DataPkts(num: %d) detached from Ring:%d (type:%d)", num_pkts_processed, ring_id,
                          data_ring_type);
}

/*
 * @brief  Processes the new data reception in TCP ring
 * @param  ctx            : Context of the ring
 * @return None
 */
static void handle_a2f_tcp_ring_update(void *ctx)
{
    handle_a2f_data_ring_update(ctx, DATA_RING_TCP);
}

/*
 * @brief  Processes the new data reception in UDP ring
 * @param  ctx            : Context of the ring
 * @return None
 */
static void handle_a2f_udp_ring_update(void *ctx)
{
    handle_a2f_data_ring_update(ctx, DATA_RING_UDP);
}

/*
 * @brief  Processes the new data reception in RawEth ring
 * @param  ctx            : Context of the ring
 * @return None
 */
static void handle_a2f_raweth_ring_update(void *ctx)
{
    handle_a2f_data_ring_update(ctx, DATA_RING_RAWETH);
}

/*
 * @brief  Processes update indicating a packet read event in F2A ring
 * @param  ctx            : Context of the ring
 * @param  data_ring_type : Type of data ring (udp/tcp/raweth)
 * @return None
 */
static void handle_f2a_data_ring_update(void *ctx, uint8_t data_ring_type)
{
    uint8_t ring_id = ((ring_ctx_t *)ctx)->ring_id;

    ringif_f2a_clear_used_bufs(ring_id, ringif_get_data_clear_fn(ring_id));
    WIFI_FW_UNUSED_ARG(data_ring_type);
}

/*
 * @brief  Processes update indicating a packet read event in F2A TCP ring
 * @param  ctx            : Context of the ring
 * @return None
 */
static void handle_f2a_tcp_ring_update(void *ctx)
{
    handle_f2a_data_ring_update(ctx, DATA_RING_TCP);
}

/*
 * @brief  Processes update indicating a packet read event in F2A UDP ring
 * @param  ctx            : Context of the ring
 * @return None
 */
static void handle_f2a_udp_ring_update(void *ctx)
{
    handle_f2a_data_ring_update(ctx, DATA_RING_UDP);
}

/*
 * @brief  Processes update indicating a packet read event in F2A RawEth ring
 * @param  ctx            : Context of the ring
 * @return None
 */
static void handle_f2a_raweth_ring_update(void *ctx)
{
    handle_f2a_data_ring_update(ctx, DATA_RING_RAWETH);
}

/*
 * @brief Posts the given ring update event to appropriate thread
 * @param ring_id         : index of the ring
 * @param b_from_isr      : Flag to indicate if caller is from ISR
 * @param p_usr_ctx       : Context of the ring user
 * @return bool           : TRUE if posting is successful, FALSE otherwise.
 */
static bool post_a2f_tcp_ring_update(uint8_t ring_id, bool b_from_isr, void *p_usr_ctx)
{
    WIFI_FW_UNUSED_ARG(p_usr_ctx);

    if (TRUE == b_from_isr) {
        xInsideISR = pdTRUE;
    }
    if (ERR_OK == tcpip_try_callback((tcpip_callback_fn)handle_a2f_tcp_ring_update, ringif_a2f_ring_ctx(ring_id))) {
        xInsideISR = pdFALSE;
        return TRUE;
    } else {
        xInsideISR = pdFALSE;
        return FALSE;
    }
}

/*
 * @brief Posts the given ring update event to appropriate thread
 * @param ring_id         : index of the ring
 * @param b_from_isr      : Flag to indicate if caller is from ISR
 * @param p_usr_ctx       : Context of the ring user
 * @return bool           : TRUE if posting is successful, FALSE otherwise.
 */
static bool post_f2a_tcp_ring_update(uint8_t ring_id, bool b_from_isr, void *p_usr_ctx)
{
    WIFI_FW_UNUSED_ARG(p_usr_ctx);
    if (TRUE == b_from_isr) {
        xInsideISR = pdTRUE;
    }

    if (ERR_OK == tcpip_try_callback((tcpip_callback_fn)handle_f2a_tcp_ring_update, ringif_f2a_ring_ctx(ring_id))) {
        xInsideISR = pdFALSE;
        return TRUE;
    } else {
        xInsideISR = pdFALSE;
        return FALSE;
    }
}

/*
 * @brief Posts the given ring update event to appropriate thread
 * @param ring_id         : index of the ring
 * @param b_from_isr      : Flag to indicate if caller is from ISR
 * @param p_usr_ctx       : Context of the ring user
 * @return bool           : TRUE if posting is successful, FALSE otherwise.
 */
static bool post_a2f_udp_ring_update(uint8_t ring_id, bool b_from_isr, void *p_usr_ctx)
{
    WIFI_FW_UNUSED_ARG(p_usr_ctx);
    if (TRUE == b_from_isr) {
        xInsideISR = pdTRUE;
    }

    if (ERR_OK == tcpip_try_callback((tcpip_callback_fn)handle_a2f_udp_ring_update, ringif_a2f_ring_ctx(ring_id))) {
        xInsideISR = pdFALSE;
        return TRUE;
    } else {
        xInsideISR = pdFALSE;
        return FALSE;
    }
}

/*
 * @brief Posts the given ring update event to appropriate thread
 * @param ring_id         : index of the ring
 * @param b_from_isr      : Flag to indicate if caller is from ISR
 * @param p_usr_ctx       : Context of the ring user
 * @return bool           : TRUE if posting is successful, FALSE otherwise.
 */
static bool post_f2a_udp_ring_update(uint8_t ring_id, bool b_from_isr, void *p_usr_ctx)
{
    WIFI_FW_UNUSED_ARG(p_usr_ctx);
    if (TRUE == b_from_isr) {
        xInsideISR = pdTRUE;
    }

    if (ERR_OK == tcpip_try_callback((tcpip_callback_fn)handle_f2a_udp_ring_update, ringif_f2a_ring_ctx(ring_id))) {
        xInsideISR = pdFALSE;
        return TRUE;
    } else {
        xInsideISR = pdFALSE;
        return FALSE;
    }
}

/*
 * @brief Posts the given ring update event to appropriate thread
 * @param ring_id         : index of the ring
 * @param b_from_isr      : Flag to indicate if caller is from ISR
 * @param p_usr_ctx       : Context of the ring user
 * @return bool           : TRUE if posting is successful, FALSE otherwise.
 */
static bool post_a2f_raweth_ring_update(uint8_t ring_id, bool b_from_isr, void *p_usr_ctx)
{
    WIFI_FW_UNUSED_ARG(p_usr_ctx);
    if (TRUE == b_from_isr) {
        xInsideISR = pdTRUE;
    }

    if (ERR_OK == tcpip_try_callback((tcpip_callback_fn)handle_a2f_raweth_ring_update, ringif_a2f_ring_ctx(ring_id))) {
        xInsideISR = pdFALSE;
        return TRUE;
    } else {
        xInsideISR = pdFALSE;
        return FALSE;
    }
}

/*
 * @brief Posts the given ring update event to appropriate thread
 * @param ring_id         : index of the ring
 * @param b_from_isr      : Flag to indicate if caller is from ISR
 * @param p_usr_ctx       : Context of the ring user
 * @return bool           : TRUE if posting is successful, FALSE otherwise.
 */
static bool post_f2a_raweth_ring_update(uint8_t ring_id, bool b_from_isr, void *p_usr_ctx)
{
    WIFI_FW_UNUSED_ARG(p_usr_ctx);
    if (TRUE == b_from_isr) {
        xInsideISR = pdTRUE;
    }

    if (ERR_OK == tcpip_try_callback((tcpip_callback_fn)handle_f2a_raweth_ring_update, ringif_f2a_ring_ctx(ring_id))) {
        xInsideISR = pdFALSE;
        return TRUE;
    } else {
        xInsideISR = pdFALSE;
        return FALSE;
    }
}
/*
 * @brief  Creates the default A2F udp ring for first time
 * @param                 : None
 * @return                : None
 */
static void create_a2f_default_udp_ring(void)
{
    g_a2f_udp_ring_id = ringif_create_a2f_ring(NULL, post_a2f_udp_ring_update, sizeof(a2f_data_ring_elem_t),
                                               MAX_NUM_A2F_DATA_RING_ELEMS);
    if (g_a2f_udp_ring_id >= MAX_NUM_A2F_RINGS) {
        A_ASSERT(0);
    }
    ringif_a2f_refill_bufs(g_a2f_udp_ring_id, ringif_get_data_refill_fn(g_a2f_udp_ring_id));
}

/*
 * @brief  Creates the default F2A udp ring for first time
 * @param                 : None
 * @return                : None
 */
static void create_f2a_default_udp_ring(void)
{
    g_f2a_udp_ring_id = ringif_create_f2a_ring(NULL, post_f2a_udp_ring_update, sizeof(f2a_data_ring_elem_t),
                                               MAX_NUM_F2A_DATA_RING_ELEMS);
    if (g_f2a_udp_ring_id >= MAX_NUM_F2A_RINGS) {
        A_ASSERT(0);
    }
}

/*
 * @brief  Creates the default A2F raweth ring for first time
 * @param                 : None
 * @return                : None
 */
static void create_a2f_default_raweth_ring(void)
{
    g_a2f_raweth_ring_id = ringif_create_a2f_ring(NULL, post_a2f_raweth_ring_update, sizeof(a2f_data_ring_elem_t),
                                                  MAX_NUM_A2F_DATA_RING_ELEMS);
    if (g_a2f_raweth_ring_id >= MAX_NUM_A2F_RINGS) {
        A_ASSERT(0);
    }
    ringif_a2f_refill_bufs(g_a2f_raweth_ring_id, ringif_get_data_refill_fn(g_a2f_raweth_ring_id));
}

/*
 * @brief  Creates the default F2A raweth ring for first time
 * @param                 : None
 * @return                : None
 */
static void create_f2a_default_raweth_ring(void)
{
    g_f2a_raweth_ring_id = ringif_create_f2a_ring(NULL, post_f2a_raweth_ring_update, sizeof(f2a_data_ring_elem_t),
                                                  MAX_NUM_F2A_DATA_RING_ELEMS);
    if (g_f2a_raweth_ring_id >= MAX_NUM_F2A_RINGS) {
        A_ASSERT(0);
    }
}

/*
 * @brief  Call appropriate ring API function to delete the data ring
 * @param  ring_id        : Ring ID to be deleted
 * @param  data_ring_type : ring type, might need to identify the free fn
 * @return bool           : TRUE if ring deletion successful, FALSE if not.
 */
static bool ringif_delete_a2f_data_ring(uint8_t ring_id, uint8_t data_ring_type)
{
    WIFI_FW_UNUSED_ARG(data_ring_type);
    return ringif_a2f_delete_ring(ring_id, ringif_get_data_clear_fn(ring_id));
}

/*
 * @brief  Call appropriate ring API function to delete the data ring
 * @param  ring_id        : Ring ID to be deleted
 * @param  data_ring_type : ring type, might need to identify the free fn
 * @return bool           : TRUE if ring deletion successful, FALSE if not.
 */
static bool ringif_delete_f2a_data_ring(uint8_t ring_id, uint8_t data_ring_type)
{
    WIFI_FW_UNUSED_ARG(data_ring_type);
    return ringif_f2a_delete_ring(ring_id, ringif_get_data_clear_fn(ring_id));
}
/*------------------------------------------------------------------------
 * Externalized Function Definitions
 * ----------------------------------------------------------------------*/
/*
 * @brief  Returns the A2F UDP Ring ID
 * @param  p_ring_id      : Pointer to Ring ID thats created
 * @return bool           : TRUE if ring creation successful, FALSE if not.
 */
bool ringif_create_a2f_udp_ring(uint8_t *p_ring_id)
{
    if (MAX_NUM_A2F_RINGS == g_a2f_udp_ring_id) {
        create_a2f_default_udp_ring();
    }

    if (g_a2f_udp_ring_id < MAX_NUM_A2F_RINGS) {
        g_num_a2f_udp_rings++;
        *p_ring_id = g_a2f_udp_ring_id;
        return TRUE;
    } else {
        return FALSE;
    }
}

/*
 * @brief  Returns the F2A UDP Ring ID
 * @param  p_ring_id      : Pointer to Ring ID thats created
 * @return bool           : TRUE if ring creation successful, FALSE if not.
 */
bool ringif_create_f2a_udp_ring(uint8_t *p_ring_id)
{
    if (MAX_NUM_F2A_RINGS == g_f2a_udp_ring_id) {
        create_f2a_default_udp_ring();
    }

    if (g_f2a_udp_ring_id < MAX_NUM_F2A_RINGS) {
        g_num_f2a_udp_rings++;
        *p_ring_id = g_f2a_udp_ring_id;
        return TRUE;
    } else {
        return FALSE;
    }
}

/*
 * @brief  Returns the A2F TCP Ring ID
 * @param  p_ring_id      : Pointer to Ring ID thats created
 * @return bool           : TRUE if ring creation successful, FALSE if not
 */
bool ringif_create_a2f_tcp_ring(uint8_t *p_ring_id)
{
    uint8_t ring_id = ringif_create_a2f_ring(NULL, post_a2f_tcp_ring_update, sizeof(a2f_data_ring_elem_t),
                                             MAX_NUM_A2F_DATA_RING_ELEMS);
    if (ring_id < MAX_NUM_A2F_RINGS) {
        ringif_a2f_refill_bufs(ring_id, ringif_get_data_refill_fn(ring_id));
        *p_ring_id = ring_id;
        return TRUE;
    } else {
        return FALSE;
    }
}

/*
 * @brief  Returns the F2A TCP Ring ID
 * @param  p_ring_id      : Pointer to Ring ID thats created
 * @return bool           : TRUE if ring creation successful, FALSE if not
 */
bool ringif_create_f2a_tcp_ring(uint8_t *p_ring_id)
{
    *p_ring_id = ringif_create_f2a_ring(NULL, post_f2a_tcp_ring_update, sizeof(f2a_data_ring_elem_t),
                                        MAX_NUM_F2A_DATA_RING_ELEMS);
    if (*p_ring_id < MAX_NUM_F2A_RINGS) {
        return TRUE;
    } else {
        return FALSE;
    }
}
/*
 * @brief  Returns the A2F raweth Ring ID
 * @param  p_ring_id      : Pointer to Ring ID thats created
 * @return bool           : TRUE if ring creation successful, FALSE if not.
 */
bool ringif_create_a2f_raweth_ring(uint8_t *p_ring_id)
{
    if (MAX_NUM_A2F_RINGS == g_a2f_raweth_ring_id) {
        create_a2f_default_raweth_ring();
    }

    if (g_a2f_raweth_ring_id < MAX_NUM_A2F_RINGS) {
        g_num_a2f_raweth_rings++;
        *p_ring_id = g_a2f_raweth_ring_id;
        return TRUE;
    } else {
        return FALSE;
    }
}
/*
 * @brief  Returns the F2A raweth Ring ID
 * @param  p_ring_id      : Pointer to Ring ID thats created
 * @return bool           : TRUE if ring creation successful, FALSE if not.
 */
bool ringif_create_f2a_raweth_ring(uint8_t *p_ring_id)
{
    if (MAX_NUM_F2A_RINGS == g_f2a_raweth_ring_id) {
        create_f2a_default_raweth_ring();
    }

    if (g_f2a_raweth_ring_id < MAX_NUM_F2A_RINGS) {
        g_num_f2a_raweth_rings++;
        *p_ring_id = g_f2a_raweth_ring_id;
        return TRUE;
    } else {
        return FALSE;
    }
}

/*
 * @brief  Takes care of deleting the said data ring
 * @param  ring_id        : Ring ID to be deleted
 * @return bool           : TRUE if ring deletion successful, FALSE if not.
 */
bool ringif_delete_a2f_udp_ring(uint8_t ring_id)
{
    if (g_num_a2f_udp_rings) {
        g_num_a2f_udp_rings--;
        if (0 == g_num_a2f_udp_rings) {
            bool b_result = FALSE;
            b_result = ringif_delete_a2f_data_ring(ring_id, DATA_RING_UDP);
            g_a2f_udp_ring_id = MAX_NUM_A2F_RINGS;
            return b_result;
        }
    }
    return TRUE;
}

/*
 * @brief  Takes care of deleting the said data ring
 * @param  ring_id        : Ring ID to be deleted
 * @return bool           : TRUE if ring deletion successful, FALSE if not.
 */
bool ringif_delete_f2a_udp_ring(uint8_t ring_id)
{
    if (g_num_f2a_udp_rings) {
        g_num_f2a_udp_rings--;
        if (0 == g_num_f2a_udp_rings) {
            bool b_result = FALSE;
            b_result = ringif_delete_f2a_data_ring(ring_id, DATA_RING_UDP);
            g_f2a_udp_ring_id = MAX_NUM_A2F_RINGS;
            return b_result;
        }
    }
    return TRUE;
}

/*
 * @brief  Takes care of deleting the said data ring
 * @param  ring_id        : Ring ID to be deleted
 * @return bool           : TRUE if ring deletion successful, FALSE if not.
 */
bool ringif_delete_a2f_raweth_ring(uint8_t ring_id)
{
    if (g_num_a2f_raweth_rings) {
        g_num_a2f_raweth_rings--;
        if (0 == g_num_a2f_raweth_rings) {
            bool b_result = FALSE;
            b_result = ringif_delete_a2f_data_ring(ring_id, DATA_RING_RAWETH);
            g_a2f_raweth_ring_id = MAX_NUM_A2F_RINGS;
            return b_result;
        }
    }
    return TRUE;
}

/*
 * @brief  Takes care of deleting the said data ring
 * @param  ring_id        : Ring ID to be deleted
 * @return bool           : TRUE if ring deletion successful, FALSE if not.
 */
bool ringif_delete_f2a_raweth_ring(uint8_t ring_id)
{
    if (g_num_f2a_raweth_rings) {
        g_num_f2a_raweth_rings--;
        if (0 == g_num_f2a_raweth_rings) {
            bool b_result = FALSE;
            b_result = ringif_delete_f2a_data_ring(ring_id, DATA_RING_RAWETH);
            g_f2a_raweth_ring_id = MAX_NUM_F2A_RINGS;
            return b_result;
        }
    }
    return TRUE;
}

/*
 * @brief  Takes care of deleting the said data ring
 * @param  ring_id        : Ring ID to be deleted
 * @return bool           : TRUE if ring deletion successful, FALSE if not.
 */
bool ringif_delete_a2f_tcp_ring(uint8_t ring_id)
{
    return ringif_delete_a2f_data_ring(ring_id, DATA_RING_TCP);
}

/*
 * @brief  Takes care of deleting the said data ring
 * @param  ring_id        : Ring ID to be deleted
 * @return bool           : TRUE if ring deletion successful, FALSE if not.
 */
bool ringif_delete_f2a_tcp_ring(uint8_t ring_id)
{
    return ringif_delete_f2a_data_ring(ring_id, DATA_RING_TCP);
}
#endif

static bool ringif_refill_hfc_elem(void *p_elem)
{
    return data_svc_get_hfc_data_buff(p_elem, DATA_RING_BUFF_SIZE);
}

static bool ringif_clear_hfc_elem(void *p_elem)
{
    return data_svc_free_hfc_data_buff(p_elem);
}

/*
 * @brief  Processes the new data reception in hfc data ring
 * @param  ctx            : Context of the ring
 * @return None
 */
static void handle_a2f_hfc_data_ring_update(void *ctx)
{
    uint8_t ring_id = ((ring_ctx_t *)ctx)->ring_id;
    _pfn_process pfn_process = (void *)process_hfc_data_pkt;
    _pfn_refill_elem pfn_refill = ringif_refill_hfc_elem;
    uint8_t num_pkts_processed = ringif_a2f_process_pkts(ring_id, pfn_process, pfn_refill);

    RINGIF_PRINT_LOG_INFO("A2F DataPkts(num: %d) detached from Ring:%d\r\n", num_pkts_processed, ring_id);
}

/*
 * @brief  Processes update indicating a packet read event in F2A ring
 * @param  ctx            : Context of the ring
 * @return None
 */
static void handle_f2a_hfc_data_ring_update(void *ctx)
{
    uint8_t ring_id = ((ring_ctx_t *)ctx)->ring_id;

    ringif_f2a_clear_used_bufs(ring_id, ringif_clear_hfc_elem);
}

/*
 * @brief Posts the given ring update event to appropriate thread
 * @param ring_id         : index of the ring
 * @param b_from_isr      : Flag to indicate if caller is from ISR
 * @param p_usr_ctx       : Context of the ring user
 * @return bool           : TRUE if posting is successful, FALSE otherwise.
 */
static bool post_a2f_hfc_ring_update(uint8_t ring_id, bool b_from_isr, void *p_usr_ctx)
{
    WIFI_FW_UNUSED_ARG(p_usr_ctx);
    if (TRUE == b_from_isr) {
        xInsideISR = pdTRUE;
    }
    RINGIF_PRINT_LOG_INFO("post_a2f_hfc_ring_update\r\n");

    if (ERR_OK ==
        hfc_data_try_callback((hfc_callback_fn)handle_a2f_hfc_data_ring_update, ringif_a2f_ring_ctx(ring_id))) {
        xInsideISR = pdFALSE;
        return TRUE;
    } else {
        xInsideISR = pdFALSE;
        return FALSE;
    }
}

/*
 * @brief Posts the given ring update event to appropriate thread
 * @param ring_id         : index of the ring
 * @param b_from_isr      : Flag to indicate if caller is from ISR
 * @param p_usr_ctx       : Context of the ring user
 * @return bool           : TRUE if posting is successful, FALSE otherwise.
 */
static bool post_f2a_hfc_ring_update(uint8_t ring_id, bool b_from_isr, void *p_usr_ctx)
{
    WIFI_FW_UNUSED_ARG(p_usr_ctx);
    if (TRUE == b_from_isr) {
        xInsideISR = pdTRUE;
    }
    RINGIF_PRINT_LOG_INFO("post_f2a_hfc_ring_update\r\n");

    if (ERR_OK ==
        hfc_data_try_callback((hfc_callback_fn)handle_f2a_hfc_data_ring_update, ringif_f2a_ring_ctx(ring_id))) {
        xInsideISR = pdFALSE;
        return TRUE;
    } else {
        xInsideISR = pdFALSE;
        return FALSE;
    }
}

void create_data_rings(void)
{
    uint16_t ring_id;

    /* create buffers based on mode */

    ring_id = ringif_create_f2a_ring(NULL, post_f2a_hfc_ring_update, sizeof(f2a_ctrl_ring_elem_t),
                                     MAX_NUM_F2A_DATA_RING_ELEMS);

    A_ASSERT(ring_id == A2F_RING_ID_DATA);

    ring_id = ringif_create_a2f_ring(NULL, post_a2f_hfc_ring_update, sizeof(a2f_ctrl_ring_elem_t),
                                     MAX_NUM_A2F_DATA_RING_ELEMS);

    A_ASSERT(ring_id == F2A_RING_ID_DATA);
    ringif_a2f_refill_bufs(ring_id, ringif_refill_hfc_elem);
}
#endif /* SUPPORT_RING_IF || SUPPORT_RING_IF_ONLY*/
