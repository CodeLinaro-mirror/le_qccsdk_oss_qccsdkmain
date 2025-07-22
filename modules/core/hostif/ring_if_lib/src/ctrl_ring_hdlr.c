/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*========================================================================
 * @brief Control ring Handler function definitions
 *=======================================================================*/

/*------------------------------------------------------------------------
 * Include Files
 * ----------------------------------------------------------------------*/
#include "fwconfig_cmn.h"
#include "nt_flags.h"

#if defined(SUPPORT_RING_IF) || defined(SUPPORT_RING_IF_ONLY)
#include "nt_osal.h"
#include "limits.h"
#include "portmacro.h"
#include "ring_svc_api.h"
#include "ring_ctx_holder.h"
#include "ctrl_ring_hdlr.h"
#include "data_svc_internal_api.h"
#include "data_svc_hfc_priv.h"

/*-------------------------------------------------------------------------
 * Externalized Function Declarations
 * ----------------------------------------------------------------------*/
extern app_mode_id_t nt_get_app_mode(void);

/*-------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * ----------------------------------------------------------------------*/
/* None */

/*------------------------------------------------------------------------
 * Type Declarations
 * ----------------------------------------------------------------------*/
typedef bool (*_process_config_fn)(uint32_t *p_buf, uint16_t len);

/* Enumeration for ringif thread signals */
typedef enum {
    RINGIF_SIGNAL_A2F_RING_UPDATE,
    RINGIF_SIGNAL_F2A_RING_UPDATE,
    RINGIF_SIGNAL_MESSAGE_QUEUE,
} RINGIF_SIGNAL_ENUM;
/*------------------------------------------------------------------------
 * Global Data Definitions
 * ----------------------------------------------------------------------*/
nt_osal_task_handle_t ringif_task_hdl;
nt_osal_queue_handle_t ringif_queue_hdl;
nt_osal_timer_handle_t ringif_timer = NULL;
/*------------------------------------------------------------------------
 * Static Function Declarations and Definitions
 * ----------------------------------------------------------------------*/
/*
 * @brief  Prints the element contents and frees it
 * @param  p_elem     : Pointer to the read ring element to be processed.
 * @return None
 *
 */
static void ringif_print_and_free_elem(void *ptr)
{
    ring_element_t *p_elem = ptr;
    RINGIF_PRINT_LOG_INFO("RingIfErr: Unknown config: buf[0]:%x len:%d info:%d\r\n", p_elem->p_buf[0], p_elem->len,
                          p_elem->info);
    ringif_free_ctrl_buf(p_elem->p_buf_start, p_elem->len);
}

#ifndef SUPPORT_RING_IF_ONLY
/*
 * @brief  post the packet to thread intended for WI-Fi packets
 * @param  p_elem     : Pointer to the read ring element to be processed.
 * @return bool       : TRUE if posting to thread successful
 *
 */
static bool post2thread_wifi_config_pkt(void *p_elem)
{
    ring_element_t *p_ring_elem = p_elem;
    return process_wifi_config_pkt(p_ring_elem->p_buf, p_ring_elem->len);
}
#endif
static bool data_svc_post2thread_hfc_pkt(void *p_elem)
{
    return process_hfc_config_pkt(p_elem);
}

/*
 * @brief  Depending on the packet type, posts to appropriate thread
 * @param  p_elem     : Pointer to the read ring element to be processed.
 * @return bool       : TRUE if posting to thread successful
 *
 */
static bool post2thread_a2f_ctrl_pkt(void *p_elem)
{
    switch (((ring_element_t *)p_elem)->info) {
#ifndef SUPPORT_RING_IF_ONLY
        case CONFIG_INFO_WIFI:
            return post2thread_wifi_config_pkt(p_elem);
        case CONFIG_INFO_RAW_ETH:
        case CONFIG_INFO_IP_CONNECT:
        case CONFIG_INFO_IP_SECURITY:
            return data_svc_post2thread_config_pkt(p_elem);
#endif
        case CONFIG_INFO_HFC:
            return data_svc_post2thread_hfc_pkt(p_elem);
        default:
            RINGIF_PRINT_LOG_ERR("RingIfErr: Unknown Config Cmd (buf: %x len:%d info:%d)\r\n",
                                 (uint32_t)((ring_element_t *)p_elem)->p_buf, ((ring_element_t *)p_elem)->len,
                                 ((ring_element_t *)p_elem)->info);
            if (((ring_element_t *)p_elem)->p_buf != NULL) {
                RINGIF_PRINT_LOG_ERR("RingIfErr: Unknown Config Cmd (buf[0]:%x buf[1]:%x buf[2]:%x)\r\n",
                                     (uint32_t)((ring_element_t *)p_elem)->p_buf[0],
                                     ((ring_element_t *)p_elem)->p_buf[1], ((ring_element_t *)p_elem)->p_buf[2]);
            }
            ringif_print_and_free_elem(p_elem);
            return TRUE;
    }
}
/*
 * @brief  Process the A2F Ring update in Ring IF Thread
 *         Takes the next ring read element and posts to subsystem thread for processing.
 *         If posting successful, detaches from the ring, updates read index, refills another buffer.
 * @param  ring_id    : Ring ID for which update is being posted
 * @return bool       : TRUE if processing is successful
 */
static bool process_ctrl_a2f_ring_update(uint8_t ring_id)
{
    ring_element_t *read_elem = ringif_a2f_next_read_elem(ring_id);
    bool b_process_result = FALSE;
    ring_ctx_t *p_ring_ctx = ringif_a2f_ring_ctx(ring_id);
    if (p_ring_ctx == NULL) {
        RINGIF_PRINT_LOG_ERR("failed to update ring context");
        return b_process_result;
    }

    if (NULL != read_elem) {
        RINGIF_PRINT_LOG_INFO("RingIF: A2F Ctrl Cmd w_ix:%d r_ix:%d info:%d\r\n", *(p_ring_ctx->p_write_idx),
                              *(p_ring_ctx->p_read_idx), read_elem->info);
        /* Freeing of the buf expected to be taken care after processing is complete */
        b_process_result = post2thread_a2f_ctrl_pkt(read_elem);
    } else {
        RINGIF_PRINT_LOG_INFO("RingIFErr: A2F Ctrl Cmd NULL w_ix:%d r_ix:%d Elem:%x\r\n", *(p_ring_ctx->p_write_idx),
                              *(p_ring_ctx->p_read_idx), (uint32_t)read_elem);
    }

    if (TRUE == b_process_result) {
        ringif_a2f_mark_as_read(ring_id, 1);
        ringif_a2f_refill_next_empty_elem(ring_id, ringif_refill_ctrl_elem);
    }

    return b_process_result;
}

/*
 * @brief  From ISR, posts the ring update event to ring if thread.
 * @param  ring_id    : Ring ID for which update is being posted
 * @param  b_from_isr : Flag to indicate if caller is from ISR
 * @param  p_usr_ctx  : Ring-user's context structure
 * @return bool       : TRUE if posting to thread successful
 */
static bool post2thread_ctrl_a2f_ring_update(uint8_t ring_id, bool b_from_isr, void *p_usr_ctx)
{
    BaseType_t xVal = pdFALSE;
    WIFI_FW_UNUSED_ARG(ring_id);
    WIFI_FW_UNUSED_ARG(p_usr_ctx);

    if (TRUE == b_from_isr) {
        xTaskNotifyFromISR(ringif_task_hdl, (1 << RINGIF_SIGNAL_A2F_RING_UPDATE), eSetBits, &xVal);
    } else {
        xVal = xTaskNotify(ringif_task_hdl, (1 << RINGIF_SIGNAL_A2F_RING_UPDATE), eSetBits);
    }

    if (pdFALSE == xVal) {
        return FALSE;
    } else {
        return TRUE;
    }
}

/*
 * @brief  Process the F2A Ring update in Ring IF Thread
 * @param  ring_id    : Ring ID for which update is being posted
 * @return bool       : TRUE if processing is successful
 */
static bool process_ctrl_f2a_ring_update(uint8_t ring_id)
{
    ring_ctx_t *p_ring_ctx = ringif_f2a_ring_ctx(ring_id);
    if (p_ring_ctx == NULL) {
        return FALSE;
    }
    RINGIF_PRINT_LOG_INFO("RingIF: F2A Ctrl Cleanup w_ix:%d r_ix:%d clr_ix:%d\r\n", *(p_ring_ctx->p_write_idx),
                          *(p_ring_ctx->p_read_idx), p_ring_ctx->ring_idx_to_clear);

    return ringif_f2a_clear_used_bufs(ring_id, ringif_clear_ctrl_elem);
}

/*
 * @brief  From ISR, posts the ring update event to ring if thread.
 * @param  ring_id    : Ring ID for which update is being posted
 * @param  b_from_isr : Flag to indicate if caller is from ISR
 * @param  p_usr_ctx  : Ring-user's context structure
 * @return bool       : TRUE if posting to thread successful
 */
static bool post2thread_ctrl_f2a_ring_update(uint8_t ring_id, bool b_from_isr, void *p_usr_ctx)
{
    BaseType_t xVal = pdFALSE;
    WIFI_FW_UNUSED_ARG(ring_id);
    WIFI_FW_UNUSED_ARG(p_usr_ctx);

    if (TRUE == b_from_isr) {
        xTaskNotifyFromISR(ringif_task_hdl, (1 << RINGIF_SIGNAL_F2A_RING_UPDATE), eSetBits, &xVal);
    } else {
        xVal = xTaskNotify(ringif_task_hdl, (1 << RINGIF_SIGNAL_F2A_RING_UPDATE), eSetBits);
    }
    if (pdFALSE == xVal) {
        return FALSE;
    } else {
        return TRUE;
    }
}

/*
 * @brief  Attaches the config packet sent by subsystems to F2A ring
 * @param  p_buf_start    : Pointer to Starting of the buffer used for freeing
 * @param  p_buf          : Pointer to buffer holding the config packet from subsystem
 * @param  len            : Length of config packet
 * @param  info           : Info related to which config type this packet belongs to
 * @return bool           : TRUE if packet is attached to the ring
 *
 */
static bool ringif_attach_config_pkt(uint32_t *p_buf_start, uint32_t *p_buf, uint16_t len, uint16_t info)
{
    return ringif_f2a_pkt_attach(F2A_RING_ID_CONFIG, p_buf_start, p_buf, len, info);
}

/*
 * @brief  control ring buffer size depending on application mode
 * @param  none
 * @return control ring size
 *
 */
uint16_t ringif_get_ctrl_buf_size(void)
{
    /* get the mode and based on that allocate the size */
    uint16_t ctrl_ring_size;
    if (nt_get_app_mode() == APP_MODE_FTM) {
        ctrl_ring_size = FTM_CTRL_RING_BUFF_SIZE;
    } else {
        ctrl_ring_size = CTRL_RING_BUFF_SIZE;
    }
    return (ctrl_ring_size);
}
/*
 * @brief  Post the config packet from subsystem thread to ring if thread (NOT to be used from ISR)
 * @param  p_buf          : Pointer to buffer holding the config packet from subsystem
 * @param  len            : Length of config packet
 * @param  info           : Info related to which config type this packet belongs to
 * @return bool           : TRUE if packet is attached to the ring
 *
 */
static bool ringif_send_config_pkt(uint32_t *p_buf, uint16_t len, uint16_t info)
{
    ring_element_t element;
    bool b_ret_val;
    if ((CONFIG_INFO_INVALID == info) || (info >= CONFIG_INFO_MAX)) {
        RINGIF_PRINT_LOG_ERR("RingIfErr: Invalid config packet p_buf:%x len:%d info:%d\r\n", (uint32_t)p_buf, len,
                             info);
        return ringif_free_ctrl_buf(p_buf, len);
    }

    if (len > ringif_get_ctrl_buf_size()) {
        RINGIF_PRINT_LOG_ERR("RingIfErr: Wrong len packet p_buf:%x len:%d info:%d\r\n", (uint32_t)p_buf, len, info);
        return ringif_free_ctrl_buf(p_buf, len);
    }
    element.p_buf_start = p_buf;
    element.p_buf = p_buf;
    element.len = len;
    element.info = info;

    if (NT_QUEUE_FAIL != nt_osal_queue_send(ringif_queue_hdl, (void *)(&element), portMAX_DELAY)) {
        BaseType_t xReturn = xTaskNotify(ringif_task_hdl, (1 << RINGIF_SIGNAL_MESSAGE_QUEUE), eSetBits);
        if (pdFALSE == xReturn) {
            RINGIF_PRINT_LOG_ERR("RingIfErr: Notification failed p_buf:%x len:%d info:%d\r\n", (uint32_t)element.p_buf,
                                 element.len, element.info);
            b_ret_val = FALSE;
        } else {
            b_ret_val = TRUE;
        }
    } else {
        RINGIF_PRINT_LOG_ERR("RingIfErr: posting to q failed p_buf:%x len:%d info:%d\r\n", (uint32_t)element.p_buf,
                             element.len, element.info);
        b_ret_val = FALSE;
    }

    if (FALSE == b_ret_val) {
        ringif_free_ctrl_buf(p_buf, len);
    }

    RINGIF_PRINT_LOG_INFO("RingIf: config event (p_buf: %x len:%d info:%d) ", (uint32_t)element.p_buf, element.len,
                          element.info);
    RINGIF_PRINT_LOG_INFO("RingIf: config event (p_buf[0]:%x p_buf[1]:%x p_buf[2]:%x) ", (uint32_t)element.p_buf[0],
                          element.p_buf[1], element.p_buf[2]);
    return b_ret_val;
}

/*
 * @brief  Ring if Task handler of control ring thread
 * @param  void*     : Pointer holding any arguments
 * @return None
 */
static void ringif_handle_tasks(void *arg)
{
    BaseType_t xResult;
    uint32_t notified_value = 0;
    WIFI_FW_UNUSED_ARG(arg);

    for (;;) {
        xResult = xTaskNotifyWait(pdFALSE, ULONG_MAX, &notified_value, portMAX_DELAY);
        if (xResult == pdPASS && notified_value) {
            RINGIF_PRINT_LOG_INFO("RingIf: Ring Thread/Task (bit_map: %x) ", (uint32_t)notified_value);
            if ((notified_value & (1 << RINGIF_SIGNAL_A2F_RING_UPDATE)) != 0) {
                process_ctrl_a2f_ring_update(A2F_RING_ID_CONFIG);
            }

            if ((notified_value & (1 << RINGIF_SIGNAL_F2A_RING_UPDATE)) != 0) {
                process_ctrl_f2a_ring_update(F2A_RING_ID_CONFIG);
            }

            if ((notified_value & (1 << RINGIF_SIGNAL_MESSAGE_QUEUE)) != 0) {
                ring_element_t elem;
                uint16_t count, num_msgs = uxQueueMessagesWaiting(ringif_queue_hdl);
                for (count = 0; count < num_msgs; count++) {
                    if (nt_osal_queue_msg_receive(ringif_queue_hdl, &elem, portMAX_DELAY) == NT_QUEUE_SUCCESS) {
                        RINGIF_PRINT_LOG_INFO("RingIf: config event in Ring thread (p_buf: %x len:%d info:%d) ",
                                              (uint32_t)elem.p_buf, elem.len, elem.info);
                        if (FALSE == ringif_attach_config_pkt(elem.p_buf_start, elem.p_buf, elem.len, elem.info)) {
                            ringif_free_ctrl_buf(elem.p_buf_start, 0);
                        }
                    }
                }
            }
            notified_value = 0;
        }
    }
}

/*
 * @brief  Ring if Timer handler of control ring thread
 * @param  timer     : Pointer to the timer
 * @return None
 */
static void ringif_handle_timer(nt_osal_timer_handle_t timer)
{
    WIFI_FW_UNUSED_ARG(timer);
    ringif_tickle_all_rings(FALSE);
}
/*------------------------------------------------------------------------
 * Externalized Function Definitions
 * ----------------------------------------------------------------------*/
/*
 * @brief  Creates a new control buffer, used by all sub modules
 * @param  flags          : 8bit flag that can be used to indicate any info
 * @param  p_len          : Pointer holding the length of buffer allocated
 * @return Buffer ptr     : Pointer to buffer allocated
 *
 */
void *ringif_get_ctrl_buf(uint8_t flags, uint16_t *p_len)
{
    void *alloc_mem_ptr = NULL;
    WIFI_FW_UNUSED_ARG(flags);
    /* get the mode and based on that allocate the size */
    uint16_t ctrl_ring_size;
    ctrl_ring_size = ringif_get_ctrl_buf_size();
    if (NULL != p_len) {
        *p_len = ctrl_ring_size;
    }
    alloc_mem_ptr = nt_osal_allocate_memory(ctrl_ring_size);
    if (alloc_mem_ptr == NULL) {
        RINGIF_PRINT_LOG_ERR("RingIfErr: mem alloc fail for ctrl_buf size: %d\r\n", ctrl_ring_size);
        return NULL;
    }
    memset(alloc_mem_ptr, 0, ctrl_ring_size);
    return alloc_mem_ptr;
}

/*
 * @brief  Free the control buffer
 * @param  p_buf          : Pointer to buffer to be freed
 * @param  len            : Length of the buffer to be freed
 * @return bool           : TRUE if Freeing is successful, else FALSE
 *
 */
bool ringif_free_ctrl_buf(void *p_buf, uint16_t len)
{
    WIFI_FW_UNUSED_ARG(len);
    nt_osal_free_memory(p_buf);
    return TRUE;
}

bool ringif_refill_ctrl_elem(void *p_elem)
{
    ring_element_t *p_ctrl_elem = p_elem;
    void *p_alloc_buf = NULL;

    if ((NULL != p_ctrl_elem->p_buf_start) || (NULL != p_ctrl_elem->p_buf) || (0 != p_ctrl_elem->len)) {
        RINGIF_PRINT_LOG_ERR("RingIfErr: trying to refill a non-empty slot %x %x %d\r\n",
                             (uint32_t)p_ctrl_elem->p_buf_start, (uint32_t)p_ctrl_elem->p_buf, p_ctrl_elem->len);
        return FALSE;
    }

    p_alloc_buf = ringif_get_ctrl_buf(0, &((ring_element_t *)p_ctrl_elem)->len);
    if (p_alloc_buf) {
        p_ctrl_elem->p_buf_start = p_alloc_buf;
        p_ctrl_elem->p_buf = p_alloc_buf;
        return TRUE;
    } else {
        return FALSE;
    }
}

bool ringif_clear_ctrl_elem(void *p_elem)
{
    ring_element_t *p_ctrl_elem = p_elem;

    if ((NULL == p_ctrl_elem->p_buf_start) || (NULL == p_ctrl_elem->p_buf) || (0 == p_ctrl_elem->len)) {
        RINGIF_PRINT_LOG_ERR("RingIfErr: trying to free a empty slot %x %x %d\r\n", (uint32_t)p_ctrl_elem->p_buf_start,
                             (uint32_t)p_ctrl_elem->p_buf, p_ctrl_elem->len);
        return FALSE;
    }

    if (TRUE == ringif_free_ctrl_buf(p_ctrl_elem->p_buf_start, p_ctrl_elem->len)) {
        p_ctrl_elem->p_buf = NULL;
        p_ctrl_elem->p_buf_start = NULL;
        p_ctrl_elem->len = 0;
        return TRUE;
    }

    return FALSE;
}

/*
 * @brief  Creates the default control rings for A2F and F2A directions
 * @param                 : None
 * @return                : None
 *
 */
void create_config_rings(void)
{
    uint32_t ret_val;
    uint16_t ring_id;
    ret_val = (uint32_t)nt_qurt_thread_create(ringif_handle_tasks, "ringif_task", RING_IF_TASK_STACK_SIZE, NULL,
                                              RING_IF_TASK_PRIORITY, &ringif_task_hdl);
    if (ret_val != pdPASS) {
        RINGIF_PRINT_LOG_ERR("RingIfErr: task creation failed out of memory\r\n");
        A_ASSERT(0);
    } else {
        ringif_queue_hdl = nt_qurt_pipe_create(RING_IF_QUEUE_SIZE, sizeof(ring_element_t));
        if (ringif_queue_hdl == NULL) {
            RINGIF_PRINT_LOG_ERR("RingIfErr: queue creation failed out of memory, task_hdl:%x\r\n",
                                 (uint32_t)ringif_task_hdl);
            nt_osal_thread_delete(ringif_task_hdl);
            A_ASSERT(0);
        }

        ringif_timer = nt_qurt_timer_create("ringif_timer", RING_IF_TIMEOUT, pdFALSE, NULL, ringif_handle_timer);
        if (!ringif_timer) {
            RINGIF_PRINT_LOG_ERR("RingIfErr: timer creation FAIL, task_hdl:%x ", (uint32_t)ringif_task_hdl, 0, 0);
        } else {
            RINGIF_PRINT_LOG_INFO("RingIf: Task:%x, Q:%x, timer:%x create SUCCESS ", (uint32_t)ringif_task_hdl,
                                  (uint32_t)ringif_queue_hdl, (uint32_t)ringif_timer);

            if (qurt_timer_start(ringif_timer, RING_IF_TIMEOUT) != NT_TIMER_SUCCESS) {
                RINGIF_PRINT_LOG_ERR("RingIfErr: Timer start FAIL");
            }
        }
    }

    /* create buffers based on mode */
    if (nt_get_app_mode() == APP_MODE_FTM) {
        ring_id = ringif_create_f2a_ring(NULL, post2thread_ctrl_f2a_ring_update, sizeof(f2a_ctrl_ring_elem_t),
                                         MAX_NUM_F2A_FTM_CTRL_RING_ELEMS);
    } else {
        ring_id = ringif_create_f2a_ring(NULL, post2thread_ctrl_f2a_ring_update, sizeof(f2a_ctrl_ring_elem_t),
                                         MAX_NUM_F2A_CTRL_RING_ELEMS);
    }

    A_ASSERT(ring_id == A2F_RING_ID_CONFIG);

    /* create buffers based on mode */
    if (nt_get_app_mode() == APP_MODE_FTM) {
        ring_id = ringif_create_a2f_ring(NULL, post2thread_ctrl_a2f_ring_update, sizeof(a2f_ctrl_ring_elem_t),
                                         MAX_NUM_A2F_FTM_CTRL_RING_ELEMS);
    } else {
        ring_id = ringif_create_a2f_ring(NULL, post2thread_ctrl_a2f_ring_update, sizeof(a2f_ctrl_ring_elem_t),
                                         MAX_NUM_A2F_CTRL_RING_ELEMS);
    }

    A_ASSERT(ring_id == A2F_RING_ID_CONFIG);
    ringif_a2f_refill_bufs(ring_id, ringif_refill_ctrl_elem);
}

/*
 * @brief  API to be used by Wi-Fi Config layer to send control packet
 * @param  p_buf          : Pointer to the config packet that needs to be sent
 * @param  len            : Length of the config packet
 * @return bool           : TRUE if attaching to ring is successful
 *
 */
bool ringif_send_wifi_config(uint32_t *p_buf, uint16_t len)
{
    return ringif_send_config_pkt(p_buf, len, CONFIG_INFO_WIFI);
}

/*
 * @brief  API to be used by HFC Config layer to send control packet
 * @param  p_buf          : Pointer to the config packet that needs to be sent
 * @param  len            : Length of the config packet
 * @return bool           : TRUE if attaching to ring is successful
 *
 */
bool ringif_send_hfc_config(uint32_t *p_buf, uint16_t len)
{
    return ringif_send_config_pkt(p_buf, len, CONFIG_INFO_HFC);
}

/*
 * @brief  API to be used by IP Config layer to send control packet
 * @param  p_buf          : Pointer to the config packet that needs to be sent
 * @param  len            : Length of the config packet
 * @return bool           : TRUE if attaching to ring is successful
 *
 */
bool ringif_send_ip_conn_config(uint32_t *p_buf, uint16_t len)
{
    return ringif_send_config_pkt(p_buf, len, CONFIG_INFO_IP_CONNECT);
}

/*
 * @brief  API to be used by IP Security layer to send control packet
 * @param  p_buf          : Pointer to the config packet that needs to be sent
 * @param  len            : Length of the config packet
 * @return bool           : TRUE if attaching to ring is successful
 *
 */
bool ringif_send_ip_secu_config(uint32_t *p_buf, uint16_t len)
{
    return ringif_send_config_pkt(p_buf, len, CONFIG_INFO_IP_SECURITY);
}

/*
 * @brief  API to be used by RAW Ethernet layer to send control packet
 * @param  p_buf          : Pointer to the config packet that needs to be sent
 * @param  len            : Length of the config packet
 * @return bool           : TRUE if attaching to ring is successful
 *
 */
bool ringif_send_raweth_config(uint32_t *p_buf, uint16_t len)
{
    return ringif_send_config_pkt(p_buf, len, CONFIG_INFO_RAW_ETH);
}
#endif /* SUPPORT_RING_IF */
