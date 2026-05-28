/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*========================================================================
 * @brief Ring Service api function definitions
 *=======================================================================*/

/*------------------------------------------------------------------------
 * Include Files
 * ----------------------------------------------------------------------*/
#include "fwconfig_cmn.h"
#include "nt_flags.h"

#if defined(SUPPORT_RING_IF) || defined(SUPPORT_RING_IF_ONLY)
#include "ring_svc_api.h"
#include "ring_ctx_holder.h"

#ifdef CONFIG_QAT_POWERSAVE_DEMO
#include "wlan_power.h"
extern uint8_t powersave_active;
#endif

/*------------------------------------------------------------------------
 * Static Function Declarations and Definitions
 * ----------------------------------------------------------------------*/
#ifdef SUPPORT_RING_IF_STATS
#define RINGIF_STATS_PRINT_TRIGGER 1000

static ring_stats_t g_a2f_ring_stats[MAX_NUM_A2F_RINGS];
static ring_stats_t g_f2a_ring_stats[MAX_NUM_F2A_RINGS];

/*
 * @brief  Increments relevant statistic of the ring
 * @param  ring_id (Index of the ring)
 * @param  num (number by which increment is needed)
 * @return None
 */
static void ringif_stats_a2f_attach_succ(uint8_t ring_id, uint8_t num)
{
    g_a2f_ring_stats[ring_id].num_pkts_attached += num;
}

/*
 * @brief  Increments relevant statistic of the ring
 * @param  ring_id (Index of the ring)
 * @param  num (number by which increment is needed)
 * @return None
 */
static void ringif_stats_a2f_detach_succ(uint8_t ring_id, uint8_t num)
{
    g_a2f_ring_stats[ring_id].num_pkts_detached += num;
}

/*
 * @brief  Increments relevant statistic of the ring
 * @param  ring_id (Index of the ring)
 * @param  num (number by which increment is needed)
 * @return None
 */
static void ringif_stats_a2f_attach_fail(uint8_t ring_id, uint8_t num)
{
    g_a2f_ring_stats[ring_id].num_attach_failures += num;

    if (!(g_a2f_ring_stats[ring_id].num_attach_failures % RINGIF_STATS_PRINT_TRIGGER)) {
        ringif_print_a2f_ring_stats(ring_id);
    }
}

/*
 * @brief  Increments relevant statistic of the ring
 * @param  ring_id (Index of the ring)
 * @param  num (number by which increment is needed)
 * @return None
 */
static void ringif_stats_a2f_detach_fail(uint8_t ring_id, uint8_t num)
{
    g_a2f_ring_stats[ring_id].num_detach_failures += num;

    if (!(g_a2f_ring_stats[ring_id].num_detach_failures % RINGIF_STATS_PRINT_TRIGGER)) {
        ringif_print_a2f_ring_stats(ring_id);
    }
}

/*
 * @brief  Increments relevant statistic of the ring
 * @param  ring_id (Index of the ring)
 * @param  num (number by which increment is needed)
 * @return None
 */
static void ringif_stats_f2a_attach_succ(uint8_t ring_id, uint8_t num)
{
    g_f2a_ring_stats[ring_id].num_pkts_attached += num;
}

/*
 * @brief  Increments relevant statistic of the ring
 * @param  ring_id (Index of the ring)
 * @param  num (number by which increment is needed)
 * @return None
 */
static void ringif_stats_f2a_detach_succ(uint8_t ring_id, uint8_t num)
{
    g_f2a_ring_stats[ring_id].num_pkts_detached += num;
}

/*
 * @brief  Increments relevant statistic of the ring
 * @param  ring_id (Index of the ring)
 * @param  num (number by which increment is needed)
 * @return None
 */
static void ringif_stats_f2a_attach_fail(uint8_t ring_id, uint8_t num)
{
    g_f2a_ring_stats[ring_id].num_attach_failures += num;

    if (!(g_f2a_ring_stats[ring_id].num_attach_failures % RINGIF_STATS_PRINT_TRIGGER)) {
        ringif_print_f2a_ring_stats(ring_id);
    }
}

/*
 * @brief  Increments relevant statistic of the ring
 * @param  ring_id (Index of the ring)
 * @param  num (number by which increment is needed)
 * @return None
 */
static void ringif_stats_f2a_detach_fail(uint8_t ring_id, uint8_t num)
{
    g_f2a_ring_stats[ring_id].num_detach_failures += num;

    if (!(g_f2a_ring_stats[ring_id].num_detach_failures % RINGIF_STATS_PRINT_TRIGGER)) {
        ringif_print_f2a_ring_stats(ring_id);
    }
}

/*
 * @brief  Prints A2F Ring stats
 * @param  ring_id (Index of the ring)
 * @return None
 */
void ringif_print_a2f_ring_stats(uint8_t ring_id)
{
    ring_stats_t *p_ring_stats = NULL;

    if (ring_id < MAX_NUM_A2F_RINGS) {
        p_ring_stats = &g_a2f_ring_stats[ring_id];

        RINGIF_PRINT_LOG_ERR("A2F Ring:%d Attach Suc:%d Fail:%d Detach Suc:%d Fail:%d\r\n", ring_id,
                             p_ring_stats->num_pkts_attached, p_ring_stats->num_attach_failures,
                             p_ring_stats->num_pkts_detached, p_ring_stats->num_detach_failures);
    } else {
        RINGIF_PRINT_LOG_ERR("RingIFStats Err: A2F invalid index %d", ring_id);
        return;
    }
}

/*
 * @brief  Prints F2A Ring stats
 * @param  ring_id (Index of the ring)
 * @return None
 */
void ringif_print_f2a_ring_stats(uint8_t ring_id)
{
    ring_stats_t *p_ring_stats = NULL;

    if (ring_id < MAX_NUM_F2A_RINGS) {
        p_ring_stats = &g_f2a_ring_stats[ring_id];

        RINGIF_PRINT_LOG_ERR("F2A Ring:%d Attach Suc:%d Fail:%d Detach Suc:%d Fail:%d\r\n", ring_id,
                             p_ring_stats->num_pkts_attached, p_ring_stats->num_attach_failures,
                             p_ring_stats->num_pkts_detached, p_ring_stats->num_detach_failures);
    } else {
        RINGIF_PRINT_LOG_ERR("RingIFStats Err: F2A invalid index %d", ring_id);
        return;
    }
}

/*
 * @brief  Clears Ring stats
 * @param  p_ring_stats Pointer to Stats struct
 * @return None
 */
static void ringif_clear_stats(ring_stats_t *p_ring_stats)
{
    memset(p_ring_stats, 0, sizeof(ring_stats_t));
}

/*
 * @brief  Clears A2F Ring stats
 * @param  ring_id (Index of the ring)
 * @return None
 */
void ringif_clear_a2f_ring_stats(uint8_t ring_id)
{
    ringif_clear_stats(&g_a2f_ring_stats[ring_id]);
}

/*
 * @brief  Clears F2A Ring stats
 * @param  ring_id (Index of the ring)
 * @return None
 */
void ringif_clear_f2a_ring_stats(uint8_t ring_id)
{
    ringif_clear_stats(&g_f2a_ring_stats[ring_id]);
}
#else /* SUPPORT_RING_IF_STATS */
#define ringif_stats_a2f_attach_succ(a, b)
#define ringif_stats_a2f_detach_succ(a, b)
#define ringif_stats_a2f_attach_fail(a, b)
#define ringif_stats_a2f_detach_fail(a, b)
#define ringif_stats_f2a_attach_succ(a, b)
#define ringif_stats_f2a_detach_succ(a, b)
#define ringif_stats_f2a_attach_fail(a, b)
#define ringif_stats_f2a_detach_fail(a, b)
#endif /* SUPPORT_RING_IF_STATS */

/*------------------------------------------------------------------------
 * Externalized Function Definitions
 * ----------------------------------------------------------------------*/
/*
 * @brief  Checks if Ring is FULL based on index/outdex values of the ring
 * @param  ring_ctx_t            : Context pointer of the ring
 * @return TRUE if Ring is full and FALSE otherwise.
 *
 */
bool ringif_ring_full(ring_ctx_t *p_ring_ctx)
{
    if (NULL == p_ring_ctx) {
        RINGIF_PRINT_LOG_ERR("RingIF_Err: p_ring_ctx NULL\r\n");
        return TRUE;
    }

    if (NULL == p_ring_ctx->p_ring_base) {
        RINGIF_PRINT_LOG_ERR("RingIF_Err: Ring base NULL for ring_id: %d\r\n", p_ring_ctx->ring_id);
        return TRUE;
    }

    if (NULL == p_ring_ctx->p_write_idx) {
        RINGIF_PRINT_LOG_ERR("RingIF_Err: Ring write ptr NULL for ring_id: %d\r\n", p_ring_ctx->ring_id);
        return TRUE;
    }

    if (*p_ring_ctx->p_write_idx >= p_ring_ctx->ring_num_elem) {
        RINGIF_PRINT_LOG_ERR("RingIF_Err: Corrupted Write index:%d >= num_elem:%d for ring_id:%d\r\n",
                             *p_ring_ctx->p_write_idx, p_ring_ctx->ring_num_elem, p_ring_ctx->ring_id);
        return TRUE;
    }

    if (NULL == p_ring_ctx->p_read_idx) {
        RINGIF_PRINT_LOG_ERR("RingIF_Err: Ring read ptr NULL for ring_id: %d\r\n", p_ring_ctx->ring_id);
        return TRUE;
    }

    if (*p_ring_ctx->p_read_idx >= p_ring_ctx->ring_num_elem) {
        RINGIF_PRINT_LOG_ERR("RingIF_Err: Corrupted Read index:%d >= num_elem:%d for ring_id:%d\r\n",
                             *p_ring_ctx->p_read_idx, p_ring_ctx->ring_num_elem, p_ring_ctx->ring_id);
        return TRUE;
    }

    if (*p_ring_ctx->p_read_idx == (*p_ring_ctx->p_write_idx + 1) % p_ring_ctx->ring_num_elem) {
        RINGIF_PRINT_LOG_INFO("RingIF_Err: Ring FULL ring_id:%d RdIdx:%d WrIdx:%d\r\n", p_ring_ctx->ring_id,
                              *p_ring_ctx->p_read_idx, *p_ring_ctx->p_write_idx);
        return TRUE;
    } else {
        return FALSE;
    }
}

/*
 * @brief  Checks if Ring is EMPTY based on index/outdex values of the ring
 * @param  ring_ctx_t            : Context pointer of the ring
 * @return TRUE if Ring is empty and FALSE otherwise.
 *
 */
bool ringif_ring_empty(ring_ctx_t *p_ring_ctx)
{
    if (NULL == p_ring_ctx) {
        RINGIF_PRINT_LOG_ERR("RingIF_Err: p_ring_ctx NULL\r\n");
        return TRUE;
    }

    if (NULL == p_ring_ctx->p_ring_base) {
        RINGIF_PRINT_LOG_ERR("RingIF_Err: Ring base NULL for ring_id: %d\r\n", p_ring_ctx->ring_id);
        return TRUE;
    }

    if (NULL == p_ring_ctx->p_write_idx) {
        RINGIF_PRINT_LOG_ERR("RingIF_Err: Ring write ptr NULL for ring_id: %d\r\n", p_ring_ctx->ring_id);
        return TRUE;
    }

    if (*p_ring_ctx->p_write_idx >= p_ring_ctx->ring_num_elem) {
        RINGIF_PRINT_LOG_ERR("RingIF_Err: Corrupted Write index:%d >= num_elem:%d for ring_id:%d\r\n",
                             *p_ring_ctx->p_write_idx, p_ring_ctx->ring_num_elem, p_ring_ctx->ring_id);
        return TRUE;
    }

    if (NULL == p_ring_ctx->p_read_idx) {
        RINGIF_PRINT_LOG_ERR("RingIF_Err: Ring read ptr NULL for ring_id: %d\r\n", p_ring_ctx->ring_id);
        return TRUE;
    }

    if (*p_ring_ctx->p_read_idx >= p_ring_ctx->ring_num_elem) {
        RINGIF_PRINT_LOG_ERR("RingIF_Err: Corrupted Read index:%d >= num_elem:%d for ring_id:%d\r\n",
                             *p_ring_ctx->p_read_idx, p_ring_ctx->ring_num_elem, p_ring_ctx->ring_id);
        return TRUE;
    }

    if (*p_ring_ctx->p_read_idx == *p_ring_ctx->p_write_idx)
        return TRUE;
    else
        return FALSE;
}

/*
 * @brief  Checks if the element buffer is NULl or not.
 * @param  ring_ctx_t : Context pointer of the ring.
 * @param  idx        : Index of ring element.
 * @return TRUE if the element buffer is empty and FALSE otherwise.
 *
 */
bool ringif_ring_buf_full(ring_ctx_t *p_ring_ctx, uint8_t idx)
{
    ring_element_t *element = NULL;
    uint8_t this_elem_null = 0;
    uint8_t next_elem_null = 0;

    if (NULL == p_ring_ctx) {
        RINGIF_PRINT_LOG_ERR("RingIF_Err: p_ring_ctx NULL\r\n");
        return TRUE;
    }

    if (NULL == p_ring_ctx->p_ring_base) {
        RINGIF_PRINT_LOG_ERR("RingIF_Err: Ring base NULL for ring_id: %d\r\n", p_ring_ctx->ring_id);
        return TRUE;
    }

    element = (p_ring_ctx->p_ring_base + idx * p_ring_ctx->ring_elem_size);

    if ((element->p_buf != NULL) || (element->p_buf_start != NULL)) {
        this_elem_null = 1;
    }

    if (idx == 0) {
        idx = p_ring_ctx->ring_num_elem - 1;
    } else {
        idx = idx - 1;
    }

    element = (p_ring_ctx->p_ring_base + idx * p_ring_ctx->ring_elem_size);

    if ((element->p_buf != NULL) || (element->p_buf_start != NULL)) {
        next_elem_null = 1;
    }

    if (this_elem_null && next_elem_null) {
        return TRUE;
    } else {
        return FALSE;
    }
}

/*
 * @brief  Checks if the F2A Ring has any used elements to be freed
 * @param  ring_ctx_t            : Context pointer of the ring
 * @return uint8_t               : Number of used elements pending to be cleared
 */
uint8_t ringif_f2a_num_elems_to_clear(ring_ctx_t *p_ring_ctx)
{
    uint8_t ring_rd_idx = 0;

    if (NULL == p_ring_ctx) {
        RINGIF_PRINT_LOG_ERR("RingIF_Err: p_ring_ctx NULL\r\n");
        return 0;
    }

    if (NULL == p_ring_ctx->p_read_idx) {
        RINGIF_PRINT_LOG_ERR("RingIF_Err: Ring read ptr NULL for ring_id: %d\r\n", p_ring_ctx->ring_id);
        return 0;
    }

    if (*p_ring_ctx->p_read_idx >= p_ring_ctx->ring_num_elem) {
        RINGIF_PRINT_LOG_ERR("RingIF_Err: Corrupted Read index:%d >= num_elem:%d for ring_id:%d\r\n",
                             *p_ring_ctx->p_read_idx, p_ring_ctx->ring_num_elem, p_ring_ctx->ring_id);
        return 0;
    }

    ring_rd_idx = *p_ring_ctx->p_read_idx;
    if (ring_rd_idx > p_ring_ctx->ring_idx_to_clear) {
        return (ring_rd_idx - p_ring_ctx->ring_idx_to_clear);
    } else if (ring_rd_idx == p_ring_ctx->ring_idx_to_clear) {
        if ((*p_ring_ctx->p_write_idx == ring_rd_idx) && (p_ring_ctx->ring_idx_clear_pending == 1)) {
            return p_ring_ctx->ring_num_elem;
        }
        return 0;
    } else {
        return (p_ring_ctx->ring_num_elem + ring_rd_idx - p_ring_ctx->ring_idx_to_clear);
    }
}

/*
 * @brief  Attach a packet pointer to an F2A ring and update index accordingly (F2A Ring Write Operation)
 * @param  ring_id        : Ring ID
 * @param  p_buf_start    : Pointer to the start of Packet buffer (used while freeing)
 * @param  p_buf          : Pointer to the Packet buffer
 * @param  len            : Valid packet length in the buffer
 * @param  info           : Info related to packet such as connection ID or Config type
 * @return FALSE if ring is already full, TRUE if packet attach successful
 *
 */
bool ringif_f2a_pkt_attach(uint8_t ring_id, uint32_t *p_buf_start, uint32_t *p_buf, uint16_t len, uint16_t info)
{
    ring_element_t *p_write_element;
    ring_ctx_t *p_ring_ctx = ringif_f2a_ring_ctx(ring_id);
    uint8_t curr_write_idx;

    if (TRUE == ringif_ring_full(p_ring_ctx)) {
        RINGIF_PRINT_LOG_INFO("ringif_f2a_pkt_attach RING FULL for ring_id:%d len:%d info:%x\r\n", (uint32_t)ring_id,
                              len, info);
        ringif_stats_f2a_attach_fail(ring_id, 1);
        return FALSE;
    }

    curr_write_idx = *p_ring_ctx->p_write_idx;
    p_write_element = p_ring_ctx->p_ring_base + curr_write_idx * p_ring_ctx->ring_elem_size;

    if ((p_write_element->p_buf != NULL) || (p_write_element->p_buf_start != NULL)) {
        // RINGIF_PRINT_LOG_ERR("ringif_f2a_pkt_attach(ring:%d) FAIL as old pkt not cleared yet at idx: %d\r\n",
        // (uint32_t) ring_id, curr_write_idx);
        ringif_stats_f2a_attach_fail(ring_id, 1);
        return FALSE;
    }

    p_write_element->p_buf = p_buf;
    p_write_element->p_buf_start = p_buf_start;
    p_write_element->len = len;
    p_write_element->info = info;

    *p_ring_ctx->p_write_idx = (curr_write_idx + 1) % p_ring_ctx->ring_num_elem;
    p_ring_ctx->ring_idx_clear_pending = 1;
    ringif_stats_f2a_attach_succ(ring_id, 1);

    RINGIF_PRINT_LOG_INFO("ringif_f2a_pkt_attach (old_w_idx:%d new_w_idx:%d p_write_element:%x) ", curr_write_idx,
                          *p_ring_ctx->p_write_idx, (uint32_t)p_write_element);
    RINGIF_PRINT_LOG_INFO("ringif_f2a_pkt_attach success ( buf[0]:%x len:%d info:%d) ",
                          (uint32_t)p_write_element->p_buf[0], p_write_element->len, p_write_element->info);

#ifdef CONFIG_QAT_HTTPC_DEMO
    sys_msleep(5);
#endif

#ifdef CONFIG_QAT_POWERSAVE_DEMO
    if (powersave_active) {
        pm_set_powersave_policy(gdevp, PS_POLICY_NOT_ALLOWED_SLEEP);
    }
#endif

    /* Indicate to Host that ring has been updated */
    ringif_indicate_to_host(ring_id, RING_DIR_F2A);
    return TRUE;
}

/*
 * @brief  Get index to the next packet to read from A2F ring
 * @param  ring_id        : Ring ID
 * @param  p_read_idx     : Pointer to hold the read index to be returned
 * @return num_elems      : Indicates the number of elements pending to be read
 */
uint8_t ringif_a2f_get_read_idx(uint8_t ring_id, uint8_t *p_read_idx)
{
    uint8_t curr_read_idx, curr_write_idx, num_elems_to_read;
    ring_ctx_t *p_ring_ctx = ringif_a2f_ring_ctx(ring_id);

    if (TRUE == ringif_ring_empty(p_ring_ctx)) {
        RINGIF_PRINT_LOG_ERR("ringif_a2f_get_read_idx RING EMPTY ring_id:%d\r\n", (uint32_t)ring_id);
        return 0;
    }

    curr_read_idx = *p_ring_ctx->p_read_idx;
    curr_write_idx = *p_ring_ctx->p_write_idx;

    if (curr_write_idx > curr_read_idx) {
        num_elems_to_read = curr_write_idx - curr_read_idx;
    } else {
        num_elems_to_read = p_ring_ctx->ring_num_elem + curr_write_idx - curr_read_idx;
    }

    *p_read_idx = curr_read_idx;
    return num_elems_to_read;
}

/*
 * @brief  Get pointer to an element of the given index
 * @param  ring_id        : Ring ID
 * @param  idx            : Index to the element whose pointer is needed
 * @return Pointer to the ring element
 */
void *ringif_a2f_element_ptr(uint8_t ring_id, uint8_t idx)
{
    ring_ctx_t *p_ring_ctx = ringif_a2f_ring_ctx(ring_id);
    if (p_ring_ctx == NULL) {
        RINGIF_PRINT_LOG_ERR("RingIF_Err: NULL context %d\r\n", ring_id);
        return NULL;
    }
    return (p_ring_ctx->p_ring_base + idx * p_ring_ctx->ring_elem_size);
}

/*
 * @brief  Get pointer to next element to be read from A2F ring
 * @param  ring_id        : Ring ID
 * @return Pointer to next read ring element
 */
void *ringif_a2f_next_read_elem(uint8_t ring_id)
{
    uint8_t read_idx;
    if (0 != ringif_a2f_get_read_idx(ring_id, &read_idx)) {
        return ringif_a2f_element_ptr(ring_id, read_idx);
    } else {
        return NULL;
    }
}

/*
 * @brief  Fn to Detach process multiple packets at one go.
 * @param  ring_id          : Ring ID
 * @param  _pfn_process     : Process Function pointer
 * @param  _pfn_refill_elem : Refill function pointer
 * @return uint8_t          : number packets detached and processed
 */
uint8_t ringif_a2f_process_pkts(uint8_t ring_id, _pfn_process pfn_process, _pfn_refill_elem pfn_refill)
{
    bool b_process_done = TRUE;
    ring_element_t *p_elem = NULL;
    ring_ctx_t *p_ring_ctx = ringif_a2f_ring_ctx(ring_id);
    uint8_t idx, num_processed = 0, num_refilled = 0;

    if (NULL == p_ring_ctx) {
        RINGIF_PRINT_LOG_ERR("Invalid Ring context %x %x %x", (uint32_t)p_ring_ctx, (uint32_t)pfn_process,
                             (uint32_t)pfn_refill);
        return 0;
    }

    uint8_t read_idx = *p_ring_ctx->p_read_idx;
    uint8_t wr_idx = *p_ring_ctx->p_write_idx;

    /* Check for empty ring */
    if (read_idx == wr_idx) {
        return 0;
    }

    if ((NULL == pfn_process) || (NULL == pfn_refill)) {
        RINGIF_PRINT_LOG_ERR("Fn ptrs NULL %x %x", (uint32_t)pfn_process, (uint32_t)pfn_refill);
        return 0;
    } else {
        RINGIF_PRINT_LOG_INFO("Read:%d Wr:%d ElemSize:%d numElem:%d", read_idx, wr_idx, p_ring_ctx->ring_elem_size,
                              p_ring_ctx->ring_num_elem);
    }

    qurt_timer_stop(ringif_timer, 0);

    idx = read_idx;
    while (1) {
        wr_idx = *p_ring_ctx->p_write_idx;

        if (idx == wr_idx) {
            break;
        }

        p_elem = (p_ring_ctx->p_ring_base + idx * p_ring_ctx->ring_elem_size);
        if ((p_elem->p_buf != NULL) && (p_elem->p_buf_start != NULL)) {
            b_process_done = pfn_process(p_elem);

            if (FALSE == b_process_done) {
                break;
            }
        }
        num_processed++;

        if (TRUE == pfn_refill(p_elem)) {
            num_refilled++;
        } else {
            memset(p_elem, 0, sizeof(ring_element_t));
            RINGIF_PRINT_LOG_ERR("elem malloc fail");
            break;
        }
        idx = (idx + 1) % p_ring_ctx->ring_num_elem;
        /* Update read index to current value */
        *p_ring_ctx->p_read_idx = idx;
    }

    /* Start the timer if job is not complete */
    if ((FALSE == b_process_done) || (num_refilled < num_processed)) {
        if (FALSE == b_process_done) {
            ringif_stats_a2f_detach_fail(ring_id, 1);
        }

        if (num_processed > num_refilled) {
            ringif_stats_a2f_attach_fail(ring_id, (num_processed - num_refilled));
        }

        if (qurt_timer_start(ringif_timer, RING_IF_TIMEOUT) != NT_TIMER_SUCCESS) {
            RINGIF_PRINT_LOG_INFO("restart timer FAIL");
        } else {
            RINGIF_PRINT_LOG_INFO("RingIF Timer Started rd:%d wr:%d mx:%d num: %d %d", idx, wr_idx,
                                  p_ring_ctx->ring_num_elem, num_refilled, num_processed);
        }
    } else {
        RINGIF_PRINT_LOG_INFO("Processed %d %d %d", idx, num_processed, num_refilled);
    }

    ringif_stats_a2f_detach_succ(ring_id, num_processed);
    ringif_stats_a2f_attach_succ(ring_id, num_refilled);

    /* Indicate to Host that ring has been updated */
    ringif_indicate_to_host(ring_id, RING_DIR_A2F);

    return num_processed;
}
/*
 * @brief  Get pointer to next element to be read from A2F ring
 * @param  ring_id        : Ring ID
 * @return Pointer to next read ring element
 */
uint8_t ringif_a2f_num_pending_pkts(uint8_t ring_id)
{
    ring_ctx_t *p_ring_ctx = ringif_a2f_ring_ctx(ring_id);

    if (p_ring_ctx == NULL) {
        RINGIF_PRINT_LOG_ERR("RingIF_Err: NULL context %d\r\n", ring_id);
        return 0;
    }
    uint8_t curr_read_idx = *p_ring_ctx->p_read_idx;
    uint8_t curr_wr_idx = *p_ring_ctx->p_write_idx;

    if (curr_wr_idx >= curr_read_idx) {
        return (curr_wr_idx - curr_read_idx);
    } else {
        return (p_ring_ctx->ring_num_elem + curr_wr_idx - curr_read_idx);
    }
}

/*
 * @brief  Marks the number of read elements as Read
 * @param  ring_id        : Ring ID
 * @param  num_read_elem  : Number of elements to be marked as read.
 * @return bool           : TRUE if marking and indicating to host is successful
 */
bool ringif_a2f_mark_as_read(uint8_t ring_id, uint8_t num_read_elem)
{
    uint8_t curr_read_idx;
    ring_element_t *p_curr_read_element;
    ring_ctx_t *p_ring_ctx = ringif_a2f_ring_ctx(ring_id);
    if (p_ring_ctx == NULL) {
        RINGIF_PRINT_LOG_ERR("Failed to fetch ring ctx");
        return FALSE;
    }
    uint8_t num_elems = num_read_elem;
    curr_read_idx = *p_ring_ctx->p_read_idx;

    p_curr_read_element = (p_ring_ctx->p_ring_base + curr_read_idx * p_ring_ctx->ring_elem_size);

    while (num_elems--) {
        p_curr_read_element->p_buf = NULL;
        p_curr_read_element->p_buf_start = NULL;
        p_curr_read_element->len = 0;
        p_curr_read_element->info = 0;
        p_curr_read_element = (void *)p_curr_read_element + p_ring_ctx->ring_elem_size;
    }

    *p_ring_ctx->p_read_idx = (curr_read_idx + num_read_elem) % p_ring_ctx->ring_num_elem;

    /* Indicate to Host that ring has been updated */
    return TRUE;
}

/*
 * @brief  Get next element that is empty ready to take a new buf
 * @param  ring_id        : Ring ID
 * @return FALSE if ring is empty or process function not ready to take a new packet.
 *         TRUE if packet is detached successfully and sent for processing.
 */
void *ringif_a2f_get_next_empty_elem(uint8_t ring_id)
{
    uint8_t idx;
    ring_element_t *p_element;
    ring_ctx_t *p_ring_ctx = ringif_a2f_ring_ctx(ring_id);

    if (TRUE == ringif_ring_full(p_ring_ctx)) {
        RINGIF_PRINT_LOG_ERR("ringif_a2f_get_next_empty_elem RING FULL ring_id:%d\r\n", (uint32_t)ring_id);
        return NULL;
    }

    idx = *p_ring_ctx->p_write_idx;

    while (1) {
        p_element = (p_ring_ctx->p_ring_base + idx * p_ring_ctx->ring_elem_size);
        if (NULL == p_element->p_buf) {
#ifdef SUPPORT_RING_IF_DEBUG
            if (0 != p_element->len) {
                A_ASSERT(0);
            }
#endif /* SUPPORT_RING_IF_DEBUG */
            return p_element;
        }
        idx = (idx + 1) % p_ring_ctx->ring_num_elem;

        if (idx == *p_ring_ctx->p_read_idx)
            break;
    }

    return NULL;
}

/*
 * @brief  Call registered ringif_buf fn to refill A2F Ring's empty slots
 * @param  ring_id        : Ring ID
 * @param  _pfn_get_buf   : Function pointer to fetch a new free buffer
 * @return FALSE if ring is already full, TRUE otherwise
 *
 */
bool ringif_a2f_refill_next_empty_elem(uint8_t ring_id, _pfn_refill_elem pfn_refill_elem)
{
    ring_element_t *element = ringif_a2f_get_next_empty_elem(ring_id);
    if (NULL == element) {
        RINGIF_PRINT_LOG_ERR("ringif_a2f_refill_next_empty_elem element FULL ring_id:%d\r\n", ring_id);
        return TRUE;
    }

    if (TRUE == pfn_refill_elem(element)) {
        ringif_stats_a2f_attach_succ(ring_id, 1);
        /* Indicate to Host that ring has been updated */
        return ringif_indicate_to_host(ring_id, RING_DIR_A2F);
    } else {
        ringif_stats_a2f_attach_fail(ring_id, 1);
        return FALSE;
    }
}
/*
 * @brief  Call registered ringif_buf fn to refill A2F Ring's empty slots
 * @param  ring_id        : Ring ID
 * @param  _pfn_get_buf   : Function pointer to fetch a new free buffer
 * @return FALSE if ring is already full, TRUE otherwise
 *
 */
bool ringif_a2f_refill_bufs(uint8_t ring_id, _pfn_refill_elem pfn_refill_elem)
{
    uint8_t idx, read_idx, total_cnt = 0;
    ring_ctx_t *p_ring_ctx = ringif_a2f_ring_ctx(ring_id);

    if (TRUE == ringif_ring_full(p_ring_ctx)) {
        RINGIF_PRINT_LOG_ERR("ringif_a2f_refill_bufs already FULL ring_id:%d\r\n", ring_id);
        return TRUE;
    }

    idx = *p_ring_ctx->p_write_idx;
    read_idx = *p_ring_ctx->p_read_idx;

    /* Travel through ring and refill the buffers from p_write_idx onwards */
    while (1) {
        ring_element_t *element = (p_ring_ctx->p_ring_base + idx * p_ring_ctx->ring_elem_size);

        if (NULL == element->p_buf) {
            if (FALSE == pfn_refill_elem(element)) {
                ringif_stats_a2f_attach_fail(ring_id, 1);
                RINGIF_PRINT_LOG_ERR("ringif_a2f_refill_bufs Refill Fail  (idx: %d, elem_ptr:%d, len:%d)\r\n", idx,
                                     (uint32_t)element, element->len);
                return FALSE;
            }
        }
        idx = (idx + 1) % p_ring_ctx->ring_num_elem;
        total_cnt++;
        if (idx == read_idx)
            break;
    }

    ringif_stats_a2f_attach_succ(ring_id, total_cnt);

    RINGIF_PRINT_LOG_INFO("ringif_a2f_refill_bufs Refill Success (ring_id:%d, from_write_idx:%d, total_cnt:%d) ",
                          p_ring_ctx->ring_id, idx, total_cnt);

    /* Indicate to Host that ring has been updated */
    ringif_indicate_to_host(ring_id, RING_DIR_A2F);

    return TRUE;
}
/*
 * @brief  Call registered free function for all read-packets of an F2A ring
 * @param  ring_id         : Ring ID
 * @param  _pfn_free_buf   : Function pointer to free a used buffer
 * @return FALSE if ring is already empty, TRUE otherwise
 *
 */
bool ringif_f2a_clear_used_bufs(uint8_t ring_id, _pfn_clear_elem pfn_clear_elem)
{
    uint8_t idx;
    uint8_t num_cleared = 0;
    uint8_t read_idx;
    uint8_t write_idx;
    uint8_t idx_clear_pending;

    ring_ctx_t *p_ring_ctx = ringif_f2a_ring_ctx(ring_id);
    if (p_ring_ctx == NULL) {
        RINGIF_PRINT_LOG_ERR("RingIF_Err: NULL context %d\r\n", ring_id);
        return FALSE;
    }
    read_idx = *p_ring_ctx->p_read_idx;
    write_idx = *p_ring_ctx->p_write_idx;
    idx_clear_pending = p_ring_ctx->ring_idx_clear_pending;

    if (p_ring_ctx->ring_idx_to_clear == read_idx) {
        RINGIF_PRINT_LOG_INFO("ringif_f2a_clear_used_bufs already cleared (id%d), p_ring_ctx->ring_idx_to_clear:%d\r\n",
                              ring_id, p_ring_ctx->ring_idx_to_clear);
        if ((write_idx != read_idx) || !ringif_ring_buf_full(p_ring_ctx, p_ring_ctx->ring_idx_to_clear)) {
            return TRUE;
        }
    }

    idx = p_ring_ctx->ring_idx_to_clear;

    /* Travel through ring and free the buffers from p_read_idx back wards */
    while (1) {
        ring_element_t *element = (p_ring_ctx->p_ring_base + idx * p_ring_ctx->ring_elem_size);

        if (element->p_buf && element->p_buf_start) {
            if (FALSE == pfn_clear_elem(element)) {
                ringif_stats_f2a_detach_fail(ring_id, 1);
                RINGIF_PRINT_LOG_ERR("ringif_f2a_clear_used_bufs: Clear failed ring_id:%d, idx:%d, read_idx:%d",
                                     ring_id, idx, read_idx);
                break;
            }
        } else {
            RINGIF_PRINT_LOG_ERR("ringif_f2a_clear_used_bufs: Invalid element ring_id:%d, idx:%d, read_idx:%d", ring_id,
                                 idx, read_idx);
            break;
        }

        num_cleared++;
        idx = (idx + 1) % p_ring_ctx->ring_num_elem;

        read_idx = *p_ring_ctx->p_read_idx;
        if (idx == read_idx)
            break;
    }

    ringif_stats_f2a_detach_succ(ring_id, num_cleared);
    p_ring_ctx->ring_idx_to_clear = idx;

    /* If still some reading is left by Apps, Remind again to clear the buffers later */
    if (FALSE == ringif_ring_empty(p_ring_ctx)) {
        if (qurt_timer_start(ringif_timer, RING_IF_TIMEOUT) != NT_TIMER_SUCCESS) {
            RINGIF_PRINT_LOG_ERR("ringif_f2a_clear_used_bufs restart timer FAIL");
        }
    } else {
        p_ring_ctx->ring_idx_clear_pending = 0;
#ifdef CONFIG_QAT_POWERSAVE_DEMO
        if (powersave_active && !spi_is_ext_wakeup()) {
            pm_set_powersave_policy(gdevp, PS_POLICY_ALLOWED_SLEEP);
        }
#endif
    }

    return TRUE;
}
#endif /* SUPPORT_RING_IF */
