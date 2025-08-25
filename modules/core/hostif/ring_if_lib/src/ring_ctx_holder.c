/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*========================================================================
 * @brief Ring Context holder function definitions
 *=======================================================================*/

/*------------------------------------------------------------------------
 * Include Files
 * ----------------------------------------------------------------------*/
#include "fwconfig_cmn.h"
#include "nt_flags.h"

#if defined(SUPPORT_RING_IF) || defined(SUPPORT_RING_IF_ONLY)
#include "ring_ctx_holder.h"
#include "ring_svc_api.h"
#include "ctrl_ring_hdlr.h"
#include "data_ring_hdlr.h"
#include "wifi_fw_internal_api.h"
#ifdef FIRMWARE_APPS_INFORMED_WAKE
#include "wifi_fw_ext_intr.h"
#endif /* FIRMWARE_APPS_INFORMED_WAKE */
#include "wifi_evt_hndlr.h"

#ifdef SUPPORT_QCSPI_SLAVE
#include "qcspi_slave_api.h"
#endif
/*------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * ----------------------------------------------------------------------*/
/* None */

/*------------------------------------------------------------------------
 * Global Data Definitions
 * ----------------------------------------------------------------------*/
/* Tables of rings, write and read indices */
/* Dummy bytes in between to make sure extra byte write by Application */
/* for word alignment does not over write the next array */
static uint8_t g_a2f_ring0_outdex_array[MAX_NUM_A2F_RINGS + NUM_A2F_DUMMY_BYTES] = {0};
static uint8_t g_f2a_ring0_index_array[MAX_NUM_F2A_RINGS + NUM_F2A_DUMMY_BYTES] = {0};
static uint8_t g_a2f_ring0_index_array[MAX_NUM_A2F_RINGS + NUM_A2F_DUMMY_BYTES] = {0};
static uint8_t g_f2a_ring0_outdex_array[MAX_NUM_F2A_RINGS + NUM_F2A_DUMMY_BYTES] = {0};

static ring_ctx_t g_a2f_ring_ctx[MAX_NUM_A2F_RINGS];
static ring_ctx_t g_f2a_ring_ctx[MAX_NUM_F2A_RINGS];

#ifdef SUPPORT_RING_IF_DEBUG
static uint8_t g_ringif_dbg_log_lvl = 1;
#else  /* SUPPORT_RING_IF_DEBUG */
static uint8_t g_ringif_dbg_log_lvl = 0;
#endif /* SUPPORT_RING_IF_DEBUG */

#ifdef UNIT_TEST_SUPPORT
extern void apps_ringif_trigger_read();
#endif /* UNIT_TEST_SUPPORT */

/*------------------------------------------------------------------------
 * Static Function Declarations and Definitions
 * ----------------------------------------------------------------------*/
/*
 * @brief  Indicate to Host after a write
 * @param  ring_id        : Ring ID
 * @param  dir_a2f_f2a    : Direction of the ring A2F or F2A
 * @return FALSE if indication could not be sent, else TRUE
 *
 */
bool ringif_indicate_to_host(uint8_t ring_id, RING_DIR dir_a2f0_f2a1)
{
    WIFI_FW_UNUSED_ARG(ring_id);
    WIFI_FW_UNUSED_ARG(dir_a2f0_f2a1);

    RINGIF_PRINT_LOG_INFO("ringif_indicate_to_host (ring:%d, dir_a2f0_f2a1:%d)\r\n", ring_id, dir_a2f0_f2a1);

#ifdef FIRMWARE_APPS_INFORMED_WAKE
    wifi_fw_ext_f2a_signal_assert(F2A_SHORT_REASON_RING_TX_RX);
#else  /*  FIRMWARE_APPS_INFORMED_WAKE */
    wifi_fw_f2a_interrupt();
#endif /* FIRMWARE_APPS_INFORMED_WAKE */

#ifdef SUPPORT_UNIT_TEST_CMD
#ifndef SUPPORT_RING_IF_ONLY
    if (FALSE == wifi_fw_in_hosted_mode()) {
        /* Trigger Unit Test Application to read the rings */
        apps_ringif_trigger_read();
    }
#endif
#endif /* SUPPORT_UNIT_TEST_CMD */

    return TRUE;
}
/*
 * @brief  Posts ring update event to appropriate thread by calling registered
 *         post2thread function whose pointer is stored in the ring context
 * @param  p_ring_ctx          : Context pointer to the ring
 * @param  b_from_isr          : TRUE if caller is from ISR
 * @return None
 */
static void post_ring_update(ring_ctx_t *p_ring_ctx, bool b_from_isr)
{
    if (p_ring_ctx->pfn_post2thread != NULL) {
        ((_pfn_post2thread)p_ring_ctx->pfn_post2thread)(p_ring_ctx->ring_id, b_from_isr, p_ring_ctx->p_usr_ctx);
    } else {
        RINGIF_PRINT_LOG_ERR("post_ring_update: post fn\r\n");
    }
}

/*
 * @brief  Delete Ring: Frees all attached buffers (used or not),
 *         frees the ring holder memory and marks the ring as invalid
 * @param  p_ring_ctx          : Context pointer to the ring
 * @param  pfn_clear_elem       : Function pointer to free the buffers
 * @return bool                : TRUE if successful and FALSE otherwise.
 */
static bool ringif_delete_ring(ring_ctx_t *p_ring_ctx, _pfn_clear_elem pfn_clear_elem)
{
    uint8_t elem_cnt;
    if (NULL == p_ring_ctx) {
        RINGIF_PRINT_LOG_ERR("ringif_delete_ring: NULL Context\r\n");
        return FALSE;
    }

    if (TRUE != ringif_ring_empty(p_ring_ctx)) {
        RINGIF_PRINT_LOG_ERR("ringif_delete_ring WARNING: Deleting Non-Empty Ring id %d state %d base %d\r\n",
                             (uint32_t)p_ring_ctx->ring_id, p_ring_ctx->ring_state, (uint32_t)p_ring_ctx->p_ring_base);
    }

    if (RING_STATE_INVALID == p_ring_ctx->ring_state) {
        RINGIF_PRINT_LOG_ERR("ringif_delete_ring ERR already deleted (id state base id %d state %d base %d\r\n)",
                             (uint32_t)p_ring_ctx->ring_id, p_ring_ctx->ring_state, (uint32_t)p_ring_ctx->p_ring_base);
        return FALSE;
    } else {
        RINGIF_PRINT_LOG_INFO("ringif_delete_ring (id state base) id %d state %d base %d\r\n",
                              (uint32_t)p_ring_ctx->ring_id, p_ring_ctx->ring_state, (uint32_t)p_ring_ctx->p_ring_base);
    }

    for (elem_cnt = 0; elem_cnt < p_ring_ctx->ring_num_elem; elem_cnt++) {
        ring_element_t *element = (p_ring_ctx->p_ring_base + elem_cnt * p_ring_ctx->ring_elem_size);

        if (NULL != element->p_buf) {
            pfn_clear_elem(element);
        }
    }
    /* NOTE: DO NOT RESET READ/WRITE INDICES HERE. CHECK CALLER FN */
    nt_osal_free_memory(p_ring_ctx->p_ring_base);
    memset(p_ring_ctx, 0x00, sizeof(ring_ctx_t));
    p_ring_ctx->ring_state = RING_STATE_INVALID;
    return TRUE;
}
/*
 * @brief  Function to stop ringif timer depending on caller from ISR or not
 * @param  b_from_isr   : flag to indicate if caller is from ISR
 * @return              : NONE
 *
 */
static void ringif_stop_timer(bool b_from_isr)
{
    BaseType_t b_stop_status;

    if (TRUE == b_from_isr) {
        b_stop_status = nt_osal_stop_timer_from_isr(ringif_timer, NULL);
    } else {
        b_stop_status = qurt_timer_stop(ringif_timer, 5);
    }

    if (pdFAIL == b_stop_status) {
        // RINGIF_PRINT_LOG_INFO("ringif_tickle_all_rings timer stop FAIL\r\n", (uint32_t) ringif_timer,(uint32_t)
        // b_from_isr, 0);
    }
}

/*------------------------------------------------------------------------
 * Externalized Function Definitions
 * ----------------------------------------------------------------------*/

void ringif_set_dbg_log_lvl(uint8_t dbg_log_lvl)
{
    RINGIF_PRINT_LOG_ERR("RingIF: debug level changed to %d from %d\r\n", dbg_log_lvl, g_ringif_dbg_log_lvl);
    g_ringif_dbg_log_lvl = dbg_log_lvl;
}

uint8_t ringif_dbg_log_lvl(void)
{
    return g_ringif_dbg_log_lvl;
}
/*
 * @brief  Ring infrastructure initialization to be called from Boot/Init path
 * @param          : NONE
 * @return         : NONE
 *
 */
void ringif_init(void)
{
    /* create the default config command and event rings */
    create_config_rings();

    /* create the default Host Fermion Communication Data rings */
    create_data_rings();
#ifndef SUPPORT_RING_IF_ONLY
    /* Initialize data service specific structures */
    data_svc_init();

    /* Initialize wifi apis structures */
    wifi_svc_init();
#endif
}

/*
 * @brief  Tickle an A2F ring so that an update is posted to its thread
 *         if the ring has new unprocessed packets
 * @param  ring_id             : Index of the ring to be tickled
 * @param  b_from_isr          : TRUE if caller is from ISR
 * @return None
 */
void ringif_tickle_a2f_ring(uint8_t ring_id, bool b_from_isr)
{
    ring_ctx_t *p_ring_ctx = ringif_a2f_ring_ctx(ring_id);

    if (NULL == p_ring_ctx) {
        RINGIF_PRINT_LOG_ERR("ringif_tickle_a2f_ring: NULL Context id %d, cxt %d\r\n", ring_id, b_from_isr);
        return;
    }

    /* Post ring update event to corresponding thread for non-empty A2F ring */
    if (FALSE == ringif_ring_empty(p_ring_ctx)) {
        post_ring_update(p_ring_ctx, b_from_isr);
        RINGIF_PRINT_LOG_INFO("RingInt: A2F-Cmd id:%d w:%d r:%d ", ring_id, *p_ring_ctx->p_write_idx,
                              *p_ring_ctx->p_read_idx);
    } else {
        RINGIF_PRINT_LOG_INFO("RingInt: A2F-Empty id:%d w:%d r:%d ", ring_id, *p_ring_ctx->p_write_idx,
                              *p_ring_ctx->p_read_idx);
    }
}

/*
 * @brief  Tickle an F2A ring so that an update is posted to its thread
 *         if the ring has new unprocessed packets
 * @param  ring_id             : Index of the ring to be tickled
 * @param  b_from_isr          : TRUE if caller is from ISR
 * @return None
 */
void ringif_tickle_f2a_ring(uint8_t ring_id, bool b_from_isr)
{
    ring_ctx_t *p_ring_ctx = ringif_f2a_ring_ctx(ring_id);
    uint8_t num_elems_to_clear = 0;

    if (NULL == p_ring_ctx) {
        RINGIF_PRINT_LOG_ERR("ringif_tickle_f2a_ring: NULL Context id %d cxt %d\r\n", ring_id, b_from_isr);
        return;
    }

    /* Post ring update event to corresponding thread if F2A ring has elements to be cleated*/
    num_elems_to_clear = ringif_f2a_num_elems_to_clear(p_ring_ctx);
    if (num_elems_to_clear > 0) {
        post_ring_update(p_ring_ctx, b_from_isr);
        RINGIF_PRINT_LOG_INFO("RingInt: F2A-Clean id:%d r:%d c:%d\r\n", ring_id, *p_ring_ctx->p_read_idx,
                              p_ring_ctx->ring_idx_to_clear);
    } else {
        RINGIF_PRINT_LOG_INFO("RingInt: F2A-Empty id:%d r:%d c:%d\r\n", ring_id, *p_ring_ctx->p_read_idx,
                              p_ring_ctx->ring_idx_to_clear);
    }
}

/*
 * @brief  Function to tickle all the valid rings so that
 *         updates (if any) get posted to respective threads
 * @param  b_from_isr  : TRUE if caller is from ISR
 * @return None
 */
void ringif_tickle_all_rings(bool b_from_isr)
{
    uint8_t ring_id;

    if (TRUE == b_from_isr) {
        wifi_fw_set_hosted_mode(TRUE);
    }

    // RINGIF_PRINT_LOG_INFO("From ISR: ringif_tickle_all_rings\r\n", (uint32_t) b_from_isr, 0,0);
    ringif_stop_timer(b_from_isr);

    /* Handle A2F Ring updates */
    for (ring_id = 0; ring_id < MAX_NUM_A2F_RINGS; ring_id++) {
        /* Skip inactive rings */
        if (RING_STATE_INVALID == g_a2f_ring_ctx[ring_id].ring_state) {
            continue;
        }

        /*Tickle the A2F ring so that update gets posted if there is any */
        ringif_tickle_a2f_ring(ring_id, b_from_isr);
    }

    /* Handle F2A Ring updates */
    for (ring_id = 0; ring_id < MAX_NUM_F2A_RINGS; ring_id++) {
        /* Skip inactive rings */
        if (RING_STATE_INVALID == g_f2a_ring_ctx[ring_id].ring_state) {
            continue;
        }

        /*Tickle the F2A ring so that update gets posted if there is any */
        ringif_tickle_f2a_ring(ring_id, b_from_isr);
    }
}
/*
 * @brief  Function to handle the ring update interrupt from Apps/Host sub system
 * @param          : NONE
 * @return         : NONE
 *
 */
void ringif_apps_ring_update_isr(void)
{
    ringif_tickle_all_rings(TRUE);
}

/*
 * @brief  Function to pause all rings
 * @param  b_from_isr  : TRUE if caller is from ISR
 * @param  b_process_b4_pause  : TRUE if processing pending packets needed before pausing
 * @return         : NONE
 *
 */
void ringif_pause_all_rings(bool b_from_isr, bool b_process_pending_entries)
{
    /* Mask any Further A2F interrupts */
#ifdef SUPPORT_QCSPI_SLAVE
    qcspi_slv_disable_host_int();
#endif

    /* If processing pending packets is required */
    if (TRUE == b_process_pending_entries) {
        ringif_tickle_all_rings(b_from_isr);
    }
}

/*
 * @brief  Function to resume all rings
 * @param  b_from_isr  : TRUE if caller is from ISR
 * @return         : NONE
 *
 */
void ringif_resume_all_rings(bool b_from_isr)
{
    /* Process any pending packets in the rings */
    ringif_tickle_all_rings(b_from_isr);

    /* UnMask Further A2F interrupts */
#ifdef SUPPORT_QCSPI_SLAVE
    qcspi_slv_enable_host_int();
#endif
}

/*
 * @brief  Get A2F Ring's context pointer from ring ID
 * @param  ring_id        : Ring ID
 * @return void*          : void* pointer to A2F ring
 *
 */
void *ringif_a2f_ring_ctx(uint8_t ring_id)
{
    if (ring_id < MAX_NUM_A2F_RINGS) {
        return &(g_a2f_ring_ctx[ring_id]);
    }

    return NULL;
}

/*
 * @brief  Get F2A Ring's context pointer from ring ID
 * @param  ring_id        : Ring ID
 * @return void*          : void* pointer to F2A ring
 *
 */
void *ringif_f2a_ring_ctx(uint8_t ring_id)
{
    if (ring_id < MAX_NUM_F2A_RINGS) {
        return &(g_f2a_ring_ctx[ring_id]);
    }

    return NULL;
}

/*
 * @brief  Get A2F Ring base address
 * @param  ring_id        : Ring ID
 * @return void*          : Address of 0th element of the A2F ring
 *
 */
void *ringif_a2f_ring_addr(uint8_t ring_id)
{
    if (ring_id < MAX_NUM_A2F_RINGS) {
        return g_a2f_ring_ctx[ring_id].p_ring_base;
    }

    return NULL;
}

/*
 * @brief  Get F2A Ring base address
 * @param  ring_id        : Ring ID
 * @return void*          : Address of 0th element of the F2A ring
 *
 */
void *ringif_f2a_ring_addr(uint8_t ring_id)
{
    if (ring_id < MAX_NUM_F2A_RINGS) {
        return g_f2a_ring_ctx[ring_id].p_ring_base;
    }

    return NULL;
}

/*
 * @brief  Get number of elements/slots of A2F Ring
 * @param  ring_id        : Ring ID
 * @return uint16         : Number of elements of the ring
 *
 */
uint16_t ringif_a2f_num_ring_elems(uint8_t ring_id)
{
    if (ring_id < MAX_NUM_A2F_RINGS) {
        return g_a2f_ring_ctx[ring_id].ring_num_elem;
    }
    return 0;
}

/*
 * @brief  Get number of elements/slots of F2A Ring
 * @param  ring_id        : Ring ID
 * @return uint16         : Number of elements of the ring
 *
 */
uint16_t ringif_f2a_num_ring_elems(uint8_t ring_id)
{
    if (ring_id < MAX_NUM_F2A_RINGS) {
        return g_f2a_ring_ctx[ring_id].ring_num_elem;
    }

    return 0;
}

/*
 * @brief  Get the size of each element of A2F Ring
 * @param  ring_id        : Ring ID
 * @return uint8_t        : actual size of each element
 *
 */
uint8_t ringif_a2f_elem_size(uint8_t ring_id)
{
    if (ring_id < MAX_NUM_A2F_RINGS) {
        return g_a2f_ring_ctx[ring_id].ring_elem_size;
    }
    return 0;
}

/*
 * @brief  Get the size of each element of F2a Ring
 * @param  ring_id        : Ring ID
 * @return uint8_t        : actual size of each element
 *
 */
uint8_t ringif_f2a_elem_size(uint8_t ring_id)
{
    if (ring_id < MAX_NUM_F2A_RINGS) {
        return g_f2a_ring_ctx[ring_id].ring_elem_size;
    }

    return 0;
}

/*
 * @brief  Creates a new A2F ring context, initializes the function pointers and other params
 * @param  p_usr_ctx         : Ring-user's context that can be given back when posting an update.
 * @param  pfn_post2thread   : Pointer to Function that posts a ring update event to corresponding thread
 * @param  elem_size         : Size of each element of the ring
 * @param  max_num_elem      : Maximum number of elements in the ring
 * @return ring_id           : Index to the new created ring if successful, Invalid number if not.
 */
uint8_t ringif_create_a2f_ring(void *p_usr_ctx, _pfn_post2thread pfn_post2thread, uint8_t elem_size,
                               uint8_t max_num_elem)
{
    uint8_t ring_id;

    /* Travel through global array to find the first available ring*/
    for (ring_id = 0; ring_id < MAX_NUM_A2F_RINGS; ring_id++) {
        if (RING_STATE_INVALID != g_a2f_ring_ctx[ring_id].ring_state) {
            continue;
        } else {
            break;
        }
    }

    if (ring_id < MAX_NUM_A2F_RINGS) {
        /* TBD: Make sure to do this allocation from an area accessible to Apps */
        memset(&(g_a2f_ring_ctx[ring_id]), 0x00, sizeof(ring_ctx_t));
        g_a2f_ring_ctx[ring_id].p_ring_base = nt_osal_allocate_memory(elem_size * max_num_elem);
        if (NULL == g_a2f_ring_ctx[ring_id].p_ring_base) {
            return INVALID_RING_ID;
        }
        memset(g_a2f_ring_ctx[ring_id].p_ring_base, 0x00, elem_size * max_num_elem);

        g_a2f_ring_ctx[ring_id].ring_state = RING_STATE_IN_USE;
        g_a2f_ring_ctx[ring_id].ring_id = ring_id;
        g_a2f_ring_ctx[ring_id].ring_elem_size = elem_size;
        g_a2f_ring_ctx[ring_id].ring_num_elem = max_num_elem;
        g_a2f_ring_ctx[ring_id].pfn_post2thread = pfn_post2thread;
        g_a2f_ring_ctx[ring_id].p_usr_ctx = p_usr_ctx;

        g_a2f_ring_ctx[ring_id].p_write_idx = &(g_a2f_ring0_index_array[ring_id]);
        g_a2f_ring_ctx[ring_id].p_read_idx = &(g_a2f_ring0_outdex_array[ring_id]);

        /* Note: Do NOT RESET the read and write indexes to avoid any sync issues */
        /* Instead only set the firmware owned index to Host owned one  */
        if (*(g_a2f_ring_ctx[ring_id].p_write_idx) < max_num_elem) {
            *(g_a2f_ring_ctx[ring_id].p_read_idx) = *(g_a2f_ring_ctx[ring_id].p_write_idx);
        } else {
            *(g_a2f_ring_ctx[ring_id].p_write_idx) = 0;
            *(g_a2f_ring_ctx[ring_id].p_read_idx) = 0;
        }

        if (ring_id > 0) {
            ringif_clear_a2f_ring_stats(ring_id);
            RINGIF_PRINT_LOG_INFO("A2F Ring creation success (id:%d, elem_size:%d, num_elem:%d) ", (uint32_t)ring_id,
                                  (uint32_t)elem_size, (uint32_t)max_num_elem);

            RINGIF_PRINT_LOG_INFO("A2F Ring creation params (base:%x, pfn:%x, ctx:%x) ",
                                  (uint32_t)g_a2f_ring_ctx[ring_id].p_ring_base, (uint32_t)pfn_post2thread,
                                  (uint32_t)p_usr_ctx);
        }
    } else {
        RINGIF_PRINT_LOG_ERR("A2F Ring creation failure  id %d elem_size %d max_num_elem %d\r\n", (uint32_t)ring_id,
                             (uint32_t)elem_size, (uint32_t)max_num_elem);

        ring_id = INVALID_RING_ID;
    }

    return ring_id;
}

/*
 * @brief  Creates a new F2A ring context, initializes the function pointers and other params
 * @param  p_usr_ctx         : Ring-user's context that can be given back when posting an update.
 * @param  pfn_post2thread   : Pointer to Function that posts a ring update event to corresponding thread
 * @param  elem_size         : Size of each element of the ring
 * @param  max_num_elem      : Maximum number of elements in the ring
 * @return ring_id           : Index to the new created ring if successful, Invalid number if not.
 */
uint8_t ringif_create_f2a_ring(void *p_usr_ctx, _pfn_post2thread pfn_post2thread, uint8_t elem_size,
                               uint8_t max_num_elem)
{
    uint8_t ring_id;

    /* Travel through global array to find the first available ring*/
    for (ring_id = 0; ring_id < MAX_NUM_F2A_RINGS; ring_id++) {
        if (RING_STATE_INVALID != g_f2a_ring_ctx[ring_id].ring_state) {
            continue;
        } else {
            break;
        }
    }

    if (ring_id < MAX_NUM_F2A_RINGS) {
        /* TBD: Make sure to do this allocation from an area accessible to Apps */
        memset(&(g_f2a_ring_ctx[ring_id]), 0x00, sizeof(ring_ctx_t));
        g_f2a_ring_ctx[ring_id].p_ring_base = nt_osal_allocate_memory(elem_size * max_num_elem);
        if (NULL == g_f2a_ring_ctx[ring_id].p_ring_base) {
            return INVALID_RING_ID;
        }
        memset(g_f2a_ring_ctx[ring_id].p_ring_base, 0x00, elem_size * max_num_elem);

        g_f2a_ring_ctx[ring_id].ring_state = RING_STATE_IN_USE;
        g_f2a_ring_ctx[ring_id].ring_id = ring_id;
        g_f2a_ring_ctx[ring_id].ring_elem_size = elem_size;
        g_f2a_ring_ctx[ring_id].ring_num_elem = max_num_elem;
        g_f2a_ring_ctx[ring_id].pfn_post2thread = pfn_post2thread;
        g_f2a_ring_ctx[ring_id].p_usr_ctx = p_usr_ctx;

        g_f2a_ring_ctx[ring_id].p_write_idx = &(g_f2a_ring0_index_array[ring_id]);
        g_f2a_ring_ctx[ring_id].p_read_idx = &(g_f2a_ring0_outdex_array[ring_id]);

        /* Note: Do NOT RESET the read and write indexes to avoid any sync issues */
        /* Instead only set the firmware owned index to Host owned one  */
        if (*(g_f2a_ring_ctx[ring_id].p_read_idx) < max_num_elem) {
            *(g_f2a_ring_ctx[ring_id].p_write_idx) = *(g_f2a_ring_ctx[ring_id].p_read_idx);
        } else {
            *(g_f2a_ring_ctx[ring_id].p_write_idx) = 0;
            *(g_f2a_ring_ctx[ring_id].p_read_idx) = 0;
        }

        /* Re initialize the clear index as well */
        g_f2a_ring_ctx[ring_id].ring_idx_to_clear = *(g_f2a_ring_ctx[ring_id].p_read_idx);

        if (ring_id > 0) {
            ringif_clear_f2a_ring_stats(ring_id);
            RINGIF_PRINT_LOG_INFO("F2A Ring creation success (id:%d, elem_size:%d, num_elem:%d) ", (uint32_t)ring_id,
                                  (uint32_t)elem_size, (uint32_t)max_num_elem);

            RINGIF_PRINT_LOG_INFO("F2A Ring creation params (base:%x, pfn:%x, ctx:%x) ",
                                  (uint32_t)g_f2a_ring_ctx[ring_id].p_ring_base, (uint32_t)pfn_post2thread,
                                  (uint32_t)p_usr_ctx);
        }
    } else {
        RINGIF_PRINT_LOG_ERR("F2A Ring creation failure id %d elem_size %d max_num_elem %d\r\n", (uint32_t)ring_id,
                             (uint32_t)elem_size, (uint32_t)max_num_elem);

        ring_id = INVALID_RING_ID;
    }

    return ring_id;
}

/*
 * @brief  Frees the ring element memory and marks the ring as invalid
 * @param  ring_id         : Index to the ring to be deleted
 * @param  _pfn_clear_elem  : Function pointer used for freeing buffers
 * @return bool            : TRUE if ring deletion successful, else FALSE.
 */
bool ringif_a2f_delete_ring(uint8_t ring_id, _pfn_clear_elem pfn_clear_elem)
{
    ring_ctx_t *p_ring_ctx = ringif_a2f_ring_ctx(ring_id);
    ringif_print_a2f_ring_stats(ring_id);
    if (TRUE == ringif_delete_ring(p_ring_ctx, pfn_clear_elem)) {
        /* Modifying Apps owned index is Risky/Prone to issues */
        /* Hence Fermion owned index is made same as that of Host owned */
        /* This will make sure ring is marked as empty after deletion */
        *(p_ring_ctx->p_read_idx) = *(p_ring_ctx->p_write_idx);
        return TRUE;
    }
    return FALSE;
}

/*
 * @brief  Frees the ring element memory and marks the ring as invalid
 * @param  ring_id         : Index to the ring to be deleted
 * @param  _pfn_clear_elem  : Function pointer used for freeing buffers
 * @return bool            : TRUE if ring deletion successful, else FALSE.
 */
bool ringif_f2a_delete_ring(uint8_t ring_id, _pfn_clear_elem pfn_clear_elem)
{
    ring_ctx_t *p_ring_ctx = ringif_f2a_ring_ctx(ring_id);
    ringif_print_f2a_ring_stats(ring_id);
    if (TRUE == ringif_delete_ring(p_ring_ctx, pfn_clear_elem)) {
        /* Modifying Apps owned index is Risky/Prone to issues */
        /* Hence Fermion owned index is made same as that of Host owned */
        /* This will make sure ring is marked as empty after deletion */
        *(p_ring_ctx->p_write_idx) = *(p_ring_ctx->p_read_idx);
        return TRUE;
    }
    return FALSE;
}

/*
 * @brief  Get the base of the array holding Read indexes of all A2F rings
 * @param  None
 * @return void*        : Pointer to the Ring0 index
 */
void *ringif_a2f_rd_idx_array(void)
{
    return g_a2f_ring0_outdex_array;
}

/*
 * @brief  Get the base of the array holding Write indexes of all F2A rings
 * @param  None
 * @return void*        : Pointer to the Ring0 index
 */
void *ringif_f2a_wr_idx_array(void)
{
    return g_f2a_ring0_index_array;
}

/*
 * @brief  Get the base of the array holding Write indexes of all A2F rings
 * @param  None
 * @return void*        : Pointer to the Ring0 index
 */
void *ringif_a2f_wr_idx_array(void)
{
    return g_a2f_ring0_index_array;
}

/*
 * @brief  Get the base of the array holding Read indexes of all F2A rings
 * @param  None
 * @return void*        : Pointer to the Ring0 index
 */
void *ringif_f2a_rd_idx_array(void)
{
    return g_f2a_ring0_outdex_array;
}

/*
 * @brief  Get maximum supported num of a2f rings
 * @param  None
 * @return uint16: Max num rings
 */
uint16_t ringif_max_num_a2f_rings(void)
{
    return MAX_NUM_A2F_RINGS;
}

/*
 * @brief  Get maximum supported num of f2a rings
 * @param  None
 * @return uint16: Max num rings
 */
uint16_t ringif_max_num_f2a_rings(void)
{
    return MAX_NUM_F2A_RINGS;
}

#ifdef SUPPORT_RING_IF_STATS
/*
 * @brief  prints stats of all valid rings
 * @param  None
 * @return None
 */
void ringif_print_all_ring_stats(void)
{
    uint8_t ring_id;

    /* Handle A2F Ring stats */
    for (ring_id = 0; ring_id < MAX_NUM_A2F_RINGS; ring_id++) {
        /* Skip inactive rings */
        if (RING_STATE_INVALID == g_a2f_ring_ctx[ring_id].ring_state) {
            continue;
        }

        /*Print the stats for the valid ring */
        ringif_print_a2f_ring_stats(ring_id);
    }

    /* Handle F2A Ring stats */
    for (ring_id = 0; ring_id < MAX_NUM_F2A_RINGS; ring_id++) {
        /* Skip inactive rings */
        if (RING_STATE_INVALID == g_f2a_ring_ctx[ring_id].ring_state) {
            continue;
        }

        /*Print the stats for the valid ring */
        ringif_print_f2a_ring_stats(ring_id);
    }
}
#endif /* SUPPORT_RING_IF_STATS */
#endif /* SUPPORT_RING_IF */
