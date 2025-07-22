/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*========================================================================
 *
 * @brief Function definitions for hfc connections
 *=======================================================================*/

/*------------------------------------------------------------------------
 * Include Files
 * ----------------------------------------------------------------------*/
#include <stdlib.h>
#include "unistd.h"
#include "netif.h"
#include "pbuf.h"
#include "data_svc_priv.h"
#include "data_svc_hfc_priv.h"
#include "data_svc_hfc.h"
#include "nt_logger_api.h"

#if defined(SUPPORT_RING_IF) || defined(SUPPORT_RING_IF_ONLY)
#define QCSPI_HFC_THREAD_STACKSIZE 2048
#define QCSPI_HFC_THREAD_PRIO      6
#define HFC_HEADER_SIZE            sizeof(hfc_msg_t)

extern unsigned int _ln_RAM_ferm_multiuse_gpio_assert_info_addr;
uint32_t *f2a_gpio_assert_info = (uint32_t *)&_ln_RAM_ferm_multiuse_gpio_assert_info_addr;

/*The queue used for receive data from Host*/
static qurt_pipe_t qcspi_hfc_data_queue;

/*The queue used for QCC730 Application receiving data from Ring buffer*/
static qurt_pipe_t qcspi_hfc_recv_queue;

/*The queue used for hfc receiving msg from host*/
static qurt_pipe_t qcspi_hfc_msg_queue;

#if defined(CONFIG_QCSPI_HFC_ETH_ENABLE)
/*
 *Get pbuf used for hfc receive
 *@param p_element :   pointer to ring element
 *@param len       :   length of payload
 *@return          :   TRUE if buffer allocated
 *                 :   FALSE if buffer not allocated
 */
bool data_svc_get_hfc_data_buff(void *p_element, uint16_t len)
{
    uint8_t *buf = NULL;
    struct pbuf *p_pbuf = NULL;

    configASSERT(NULL != p_element);
    ring_element_t *p_elem = (ring_element_t *)p_element;

    buf = nt_osal_allocate_memory(len);
    if (buf == NULL) {
        return FALSE;
    }

    p_pbuf = pbuf_alloc(PBUF_RAW, len, PBUF_REF);
    if (p_pbuf != NULL) {
        p_pbuf->payload = buf;
        p_elem->p_buf = p_pbuf->payload;
        p_elem->p_buf_start = p_pbuf;
        p_elem->len = len;
        return TRUE;
    } else {
        RINGIF_PRINT_LOG_ERR("data_svc_get_hfc_data_buff fail");
    }

    return FALSE;
}
#else
/*
 *Get buffer used for hfc receive
 *@param p_element :   pointer to ring element
 *@param len       :   length of payload
 *@return          :   TRUE if buffer allocated
 *                 :   FALSE if buffer not allocated
 */
bool data_svc_get_hfc_data_buff(void *p_element, uint16_t len)
{
    configASSERT(NULL != p_element);
    ring_element_t *p_elem = (ring_element_t *)p_element;
    memset(p_elem, 0, sizeof(ring_element_t));
    uint32_t *buf = nt_osal_allocate_memory(len);
    if (buf != NULL) {
        memset(buf, 0, len);
        p_elem->p_buf = buf;
        p_elem->p_buf_start = buf;
        p_elem->len = len;
        return TRUE;
    }

    return FALSE;
}

#endif
/*
 *Free buffer used for hfc receive
 *@param p_element :   pointer to ring element
 *@return          :   TRUE if buffer freed
 *                 :   FALSE if buffer not freed
 */
bool data_svc_free_hfc_data_buff(void *p_element)
{
    configASSERT(NULL != p_element);

    void *p_buf = NULL;
    ring_element_t *p_elem = (ring_element_t *)p_element;

    RINGIF_PRINT_LOG_INFO("free hfc buf %x\r\n", (uint32_t)p_elem->p_buf_start);

    p_buf = p_elem->p_buf_start;
    nt_osal_free_memory(p_buf);

    p_elem->p_buf = NULL;
    p_elem->p_buf_start = NULL;

    return TRUE;
}

/*
*Callback triggered when some data is receievd on Fermion
* and need to send to Host.
*
@param p_buff   :   pointer of buffer
@param payload  :   pointer of payload
@param len      :   length of payload
@param info     :   extra info
*/
int32_t data_svc_hfc_send_data_pkt(void *p_buff, uint8_t *payload, uint16_t len, uint16_t info)
{
    bool b_result = FALSE;

    RINGIF_PRINT_LOG_INFO("data_svc_hfc_send_data_pkt p_buff %x len %d\r\n", (uint32_t)p_buff, len);
    if (NULL == p_buff || NULL == payload || 0 == len) {
        return -1;
    }

    b_result = ringif_f2a_pkt_attach(F2A_RING_ID_DATA, (uint32_t *)p_buff, (uint32_t *)payload, len, info);
    if (FALSE == b_result) {
        RINGIF_PRINT_LOG_INFO("dropping hfc f2a pkt as ring full");
        return -2;
    }

    RINGIF_PRINT_LOG_INFO("Rx Pkt Sent to ring(%d) len:%d", F2A_RING_ID_DATA, len);
    return 0;
}

/*
*
@param p_buff   :   pointer of buffer
@param buf_len  :   pointer of buffer length
@param data_len :   pointer of data length
@param info     :   pointer of extra info
*/
int32_t data_svc_hfc_recv_data_pkt(void *p_buff, uint16_t *buf_len, uint32_t timeout, uint16_t *data_len,
                                   uint16_t *info)
{
    bool b_result = FALSE;
    uint16_t len = 0;
    hfc_msg_t msg;
    int ret = QAPI_ERR_TIMEOUT;

    RINGIF_PRINT_LOG_INFO("data_svc_hfc_recv_data_pkt p_buff %x buf_len %d\r\n", (uint32_t)p_buff, buf_len);

    if ((NULL == p_buff) || (NULL == buf_len) || (0 == *buf_len) || (NULL == data_len)) {
        return QAPI_ERR_INVALID_PARAM;
    }

    if (nt_osal_queue_msg_receive(qcspi_hfc_recv_queue, &msg, timeout) == NT_QUEUE_SUCCESS) {
        len = msg.len;
        if (msg.len > *buf_len) {
            len = *buf_len;
            ret = -2;
        }
        *data_len = len;
        if (NULL != info) {
            *info = msg.id;
        }
#ifdef MEM_CPY_VIA_DXE
        nt_dpm_memcpy(p_buff, msg.data, len);
#else
        memcpy(p_buff, msg.data, len);
#endif
        ret = QAPI_OK;
        free(msg.buf);
    }

    return ret;
}

/*
*
@param p_buff   :   pointer of buffer
@param buf_len  :   pointer of buffer length
@param data_len :   pointer of data length
@param info     :   pointer of extra info
*/
int32_t data_svc_hfc_recv_msg(hfc_msg_t *msg, uint32_t timeout)
{
    bool b_result = FALSE;
    uint16_t len = 0;
    int ret = QAPI_ERR_TIMEOUT;

    if (NULL == msg) {
        return QAPI_ERR_INVALID_PARAM;
    }

    if (nt_osal_queue_msg_receive(qcspi_hfc_msg_queue, msg, portMAX_DELAY) == NT_QUEUE_SUCCESS) {
        ret = QAPI_OK;
    }

    return ret;
}

int32_t data_svc_hfc_queue_send(ring_element_t *p_elem, hfc_msg_type_t type)
{
    hfc_msg_t msg;
    hfc_msg_hdr *header = NULL;

    if (NULL == p_elem->p_buf) {
        return -1;
    }

    if (p_elem->len > 1400) {
        RINGIF_PRINT_LOG_ERR("msg len = %d > 1400\r\n", msg.len);
        A_ASSERT(0);
    }

    msg.type = type;
    if (HFC_DATA_MSG == type) {
        msg.id = p_elem->info;
    } else if (HFC_CTRL_MSG == type) {
        header = (hfc_msg_hdr *)p_elem->p_buf;
        msg.id = header->msg_id;
    }
    msg.data = (uint8_t *)p_elem->p_buf;
    msg.len = p_elem->len;
    msg.buf = p_elem->p_buf_start;
#ifdef CONFIG_QCSPI_HFC_TEST
    if (NT_QUEUE_FAIL == nt_osal_queue_send(qcspi_hfc_msg_queue, (void *)&msg, portMAX_DELAY)) {
        RINGIF_PRINT_LOG_ERR("hfc queue send fail", 0);
    }
#elif defined(CONFIG_QCSPI_HFC_ATCMD_ENABLE)
    if (NT_QUEUE_FAIL == nt_osal_queue_send(qcspi_hfc_recv_queue, (void *)&msg, portMAX_DELAY)) {
        RINGIF_PRINT_LOG_ERR("hfc queue send fail", 0);
    }
    // printf("data_svc_hfc_queue_send len %d\r\n", msg.len);
#else
    (void)msg;
#endif
    return 0;
}

qbool_t data_svc_hfc_send_config(uint32_t *p_buf, uint16_t len)
{
    return ringif_send_hfc_config(p_buf, len);
}

uint32_t data_svc_set_gpio_assert_info(uint32_t info)
{
    if (*f2a_gpio_assert_info != info) {
        *f2a_gpio_assert_info = info;
        ringif_indicate_to_host(0, RING_DIR_F2A);
    }
}

int hfc_rx_raw_ether(struct pbuf *p, struct netif *netif)
{
    uint8_t *buf = NULL;

    if (p->next != NULL || p->len > 1600) {
        // Packets chain and payload is too big
        printf("[%s][%d]: buff size %d %d next 0x%x\n", __func__, __LINE__, PBUF_POOL_BUFSIZE, p->len, p->next);
        return 0;
    }

    buf = (uint8_t *)nt_osal_allocate_memory(p->len);

    if (buf == NULL)
        return 0;

    memcpy(buf, p->payload, p->len);

    if (data_svc_hfc_send_data_pkt(buf, buf, p->len, 0) != QAPI_OK)
        nt_osal_free_memory(buf);

    return 0;
}

static int hfc_tx_raw_ethernet(uint8_t *buff, int len)
{
    int ret = 0;
    struct netif *netif = netif_find("st1");
    if (netif == NULL)
        netif = netif_find("ap2");
    struct pbuf *pb = (struct pbuf *)buff;

    if (netif) {
        if (pb) {
            pb->len = pb->tot_len = len;
            ret = netif->linkoutput(netif, pb);
            if (ret != 0) {
                RINGIF_PRINT_LOG_INFO("linkoutput err %d\n", ret);
            }
        }
    } else {
        RINGIF_PRINT_LOG_ERR("[%s][%d]: netif not found\n", __func__, __LINE__);
    }

    if (pb) {
        if (pb->payload) {
            nt_osal_free_memory(pb->payload);
        }
        pbuf_free(pb);
    }

    return 0;
}

bool process_hfc_data_pkt(void *p_element)
{
    RINGIF_PRINT_LOG_INFO("hfc: Processing hfc packet\r\n");
    ring_element_t *p_elem = (ring_element_t *)p_element;

    if (NULL == p_elem || NULL == p_elem->p_buf) {
        return FALSE;
    }

#if (!defined(CONFIG_QCSPI_HFC_TEST) && !defined(CONFIG_QCSPI_HFC_ETH_ENABLE) && \
     !defined(CONFIG_QCSPI_HFC_ATCMD_ENABLE))
    uint16_t i;
    uint8_t *p = (uint8_t *)(p_elem->p_buf);

    printf("data %x len %d:", (uint32_t)p_elem->p_buf_start, p_elem->len);

    for (i = 0; i < p_elem->len; i++) {
        printf("%02x", *(p + i));
    }
    printf("\r\n");
    nt_osal_free_memory(p_elem->p_buf_start);
#elif defined(CONFIG_QCSPI_HFC_ETH_ENABLE)
    hfc_tx_raw_ethernet(p_elem->p_buf_start, p_elem->len);
#else
    // RINGIF_PRINT_LOG_INFO("recv atcmd buf %x len %d:  \r\n", (uint32_t)p_elem->p_buf_start, p_elem->len);
    data_svc_hfc_queue_send(p_elem, HFC_DATA_MSG);
#endif

    return TRUE;
}

bool process_hfc_config_pkt(void *p_element)
{
    ring_element_t *p_elem = (ring_element_t *)p_element;

    if (NULL == p_elem) {
        return FALSE;
    }
#ifndef CONFIG_QCSPI_HFC_TEST
    hfc_msg_hdr *header = (hfc_msg_hdr *)p_elem->p_buf;
    printf("hfc config msg_id %d\r\n", header->msg_id);
    nt_osal_free_memory(p_elem->p_buf_start);
#else
    data_svc_hfc_queue_send(p_elem, HFC_CTRL_MSG);
#endif
    return TRUE;
}

err_t hfc_data_try_callback(hfc_callback_fn fun, void *ctx)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    hfc_cb_t msg;

    msg.fun = fun;
    msg.ctx = ctx;

    if (NT_QUEUE_FAIL == nt_osal_queue_send_from_isr(qcspi_hfc_data_queue, (void *)&msg, &xHigherPriorityTaskWoken)) {
        RINGIF_PRINT_LOG_ERR("hfc queue send fail", 0);
    }

    return ERR_OK;
}

/**
 * The main qcspi hfc thread.
 *
 * @param arg : unused argument
 */
static void qcspi_hfc_thread(void *arg)
{
    (void)(arg);
    hfc_cb_t msg;

    while (1) {
        if (nt_osal_queue_msg_receive(qcspi_hfc_data_queue, &msg, portMAX_DELAY) == NT_QUEUE_SUCCESS) {
            if (NULL != msg.fun) {
                msg.fun(msg.ctx);
            }
        }
    }
}

/**
 * Initialize this qcspi hfc module:
 * - start the qcspi_hfc_thread
 */
void qcspi_hfc_init(void)
{
    uint32_t ret_val;
    nt_osal_task_handle_t hfc_task_hdl;
    RINGIF_PRINT_LOG_INFO("qcspi_hfc_init");

    extern void ringif_init(void);
    /* Initialize the ring interface */
    ringif_init();

    extern void wifi_fw_defaults_table_init(void);
    /* Update Fermion defaults Table to be used by Apps */
    wifi_fw_defaults_table_init();

    ret_val = (uint32_t)nt_qurt_thread_create(qcspi_hfc_thread, "qcspi_hfc_thread", QCSPI_HFC_THREAD_STACKSIZE, NULL,
                                              QCSPI_HFC_THREAD_PRIO, &hfc_task_hdl);
    if (ret_val != pdPASS) {
        RINGIF_PRINT_LOG_ERR("RingIfErr: task creation failed out of memory\r\n");
        A_ASSERT(0);
    }

    qcspi_hfc_data_queue = nt_qurt_pipe_create(TOTAL_NUM_DATA_RING_ELEMS, sizeof(hfc_cb_t));
    if (qcspi_hfc_data_queue == NULL) {
        RINGIF_PRINT_LOG_ERR("failed to create qcspi_hfc_data_queue", 0);
        nt_osal_thread_delete(hfc_task_hdl);
        A_ASSERT(0);
    }

    qcspi_hfc_msg_queue = nt_qurt_pipe_create(TOTAL_NUM_DATA_RING_ELEMS, sizeof(hfc_msg_t));
    if (qcspi_hfc_msg_queue == NULL) {
        RINGIF_PRINT_LOG_ERR("failed to create qcspi_hfc_msg_queue", 0);
        nt_osal_thread_delete(hfc_task_hdl);
        A_ASSERT(0);
    }

#ifdef CONFIG_QCSPI_HFC_ATCMD_ENABLE
    qcspi_hfc_recv_queue = nt_qurt_pipe_create(TOTAL_NUM_DATA_RING_ELEMS_AT, sizeof(hfc_msg_t));
    if (qcspi_hfc_recv_queue == NULL) {
        RINGIF_PRINT_LOG_ERR("failed to create qcspi_hfc_recv_queue", 0);
        nt_osal_thread_delete(hfc_task_hdl);
        A_ASSERT(0);
    }
#endif
}
#endif  // SUPPORT_RING_IF
