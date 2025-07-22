/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*----------------------------------------------------------------------------*
 * @file wifi_fw_dbg_infra.c
 * @brief Implementation of Fermion Debug infra
 *
 *
 *----------------------------------------------------------------------------*/

#include "fdi.h"

#ifdef FEATURE_FDI
#include "nt_osal.h"
#include "nt_devcfg.h"
#include "nt_devcfg_structure.h"
#include "nt_devcfg.h"
#include "nt_hw_support.h"

#define FDI_MOD_DEF_BMAP ((uint32_t)-1)
#define NT_IS_ISR        ((HW_REG_RD(NT_ICSR_REG)) & NT_SCB_ICSR_VECTACTIVE_Msk) != 0

/*----------------------------------------------------------------------------
 * Extern Functions
 *----------------------------------------------------------------------------*/
extern uint8_t get_warmboot_status(void);

/*----------------------------------------------------------------------------
 * Static Function Declare
 *----------------------------------------------------------------------------*/
#if FDI_PRINT_ON_UT == FDI_SET
static void _post_wm_cb(fifo_t *p_fifo);
static fdi_ret_t _fdi_enqueue(fifo_t *q, char *node);
#endif /* FDI_PRINT_ON_UT */

#if FDI_EN_POST_PROCESS == FDI_SET
static void _fdi_thread(void *p_queue);
static void _fdi_post_process_complete_cb(fifo_t *p_fifo);
static void _swap_log_buffer(void);
#else
#if FDI_PRINT_ON_UT == FDI_SET
static fdi_ret_t _fdi_dequeue(fifo_t *q, char *node);
static fdi_ret_t _fdi_trav_q(fifo_t *q, size_t *next_el, char *node);
#endif
#endif /* FDI_EN_POST_PROCESS */

fdi_ret_t _fdi_push_ins_node(const fdi_node_ins_t *ins_node);
/*----------------------------------------------------------------------------
 * Static Variables
 *----------------------------------------------------------------------------*/
#if FDI_EN_POST_PROCESS == FDI_SET
FIFO_INIT_BUFFER_CIRCULAR(fdi_log_ping, FDI_FIFO_DEPTH, fdi_node_ins_t, FDI_FIFO_ABS_WATERMARK(FDI_FIFO_DEF_WATERMARK),
                          _post_wm_cb);
FIFO_INIT_BUFFER_CIRCULAR(fdi_log_pong, FDI_FIFO_DEPTH, fdi_node_ins_t, FDI_FIFO_ABS_WATERMARK(FDI_FIFO_DEF_WATERMARK),
                          _post_wm_cb);
/* Queue to push Non-processed starts with identifiers */
FIFO_INIT_BUFFER_CIRCULAR(fdi_log_idfs, FDI_FIFO_DEPTH, fdi_node_ins_t, FDI_FIFO_ABS_WATERMARK(FDI_FIFO_DEF_WATERMARK),
                          NULL);
#else
#if FDI_PRINT_ON_UT == FDI_SET
FIFO_INIT_BUFFER_CIRCULAR(fdi_log_full, FDI_FIFO_DEPTH, fdi_node_ins_t, FDI_FIFO_ABS_WATERMARK(FDI_FIFO_DEF_WATERMARK),
                          _post_wm_cb);
#endif
#endif

#if FDI_EN_POST_PROCESS == FDI_SET
static uint32_t g_processed_sq_no = FDI_RESET;
static uint32_t g_missed_sq_no = FDI_RESET;

static TaskHandle_t xFDIHandle = NULL;
static EventGroupHandle_t xFDIEventGroup;
static uint8_t g_fdi_signal_flag = FDI_RESET;
#endif

static SemaphoreHandle_t xFDI_SEM = NULL;
static FDI_PS_DATA uint32_t g_sequence_number = FDI_RESET;
/*----------------------------------------------------------------------------
 * Global Variables
 *----------------------------------------------------------------------------*/
FDI_PS_DATA fdi_reg_t g_fdi_node_table[FDI_MAX_NODE];
fdi_t g_fdi = {
#if FDI_EN_POST_PROCESS == FDI_SET
    .watermark = FDI_FIFO_ABS_WATERMARK(FDI_FIFO_DEF_WATERMARK),
    .buffer_ptr_idfs = &FIFO_INSTANCE(fdi_log_idfs),
    .buffer_ptr_active = &FIFO_INSTANCE(fdi_log_ping),
    .buffer_ptr_inactive = &FIFO_INSTANCE(fdi_log_pong),
#else
#if FDI_PRINT_ON_UT == FDI_SET
    .buffer_ptr_active = &FIFO_INSTANCE(fdi_log_full),
#else
    .buffer_ptr_active = NULL,
#endif /* FDI_PRINT_ON_UT */
#endif /* FDI_EN_POST_PROCESS */
    .p_reg_table = g_fdi_node_table,
    .module_bmap_en = FDI_MOD_DEF_BMAP};

/*----------------------------------------------------------------------------
 * Function Define
 *----------------------------------------------------------------------------*/
/**********************************************************************************************
 * @brief Register an Event Node
 *
 * @param id        ID of the Node
 * @param attr      attr_t
 * @param p_cb      pointer for Post log insertion Callback
 * @param p_ret     pointer to fdi_reg_t type pointer var for return the Address of registration
 * @return fdi_ret_t
 ***********************************************************************************************/
fdi_ret_t fdi_reg_node(uint16_t id, attr_t attr, post_ins_cb p_cb, fdi_reg_t **p_ret)
{
    FDI_ASSERT_IF_FALSE(id < FDI_MAX_NODE, FDI_RET_FAILED_NODE_MAX);
    FDI_ASSERT_IF_FALSE(p_ret != NULL, FDI_RET_FAILED_ASSERT_PARAM);

    g_fdi_node_table[id].attr = (uint16_t)((attr.Enable << FDI_NODE_ATTR_ENABLE_OFFSET) |
                                           (attr.log_level << FDI_NODE_ATTR_LOG_LVL_OFFSET) | id);
    g_fdi_node_table[id].module_bmap = attr.module_bmap;
    g_fdi_node_table[id].p_cb = p_cb;
    *p_ret = &(g_fdi_node_table[id]);

    return FDI_RET_SUCCESS;
}

/******************************************************************************************************
 * @brief Insert an Event Log to the Log Buffer
 * @param p_reg             Pointer to Log reg, returned during fdi_reg_node as p_ret
 * @param tick_type         TICK_TYPE_START_TICK / TICK_TYPE_STOP_TICK
 * @param identifier        A param to indentify corresponding start and stop. Can be defaulted to NULL
 * @return fdi_ret_t
 *******************************************************************************************************/
FDI_PS_TXT fdi_ret_t fdi_insert_log(const fdi_reg_t *p_reg, tick_type_t tick_type, uint32_t identifier)
{
    /* If BMPS context call fdi_rmc_inst*/
    if (get_warmboot_status() == FDI_SET) {
#if defined(FEATURE_FDI_RMC)
        return fdi_rmc_inst((p_reg->attr & FDI_NODE_ATTR_ID_MASK), tick_type, identifier);
#else
        return FDI_RET_FAILED_RMC_CONTEXT;
#endif
    }

    fdi_node_ins_t ins_node;
    /* Get Current Time-stamp */
    ins_node.ticks = fdi_get_time_stamp();
    FDI_ASSERT_IF_FALSE(p_reg != NULL, FDI_RET_FAILED_ASSERT_PARAM);
    FDI_ASSERT_IF_FALSE((g_fdi.module_bmap_en & p_reg->module_bmap) != FDI_RESET, FDI_RET_FAILED_ASSERT_MOD_NOT_ENABLE);
    FDI_ASSERT_IF_FALSE((FDI_NODE_ATTR_EN_BIT_MASK & p_reg->attr) != FDI_RESET, FDI_RET_FAILED_ASSERT_NODE_NOT_ENABLE);
#ifndef NT_DEBUG
    FDI_ASSERT_IF_FALSE((p_reg->attr & LOG_LEVEL_PROD) != FDI_RESET, FDI_RET_FAILED_ASSERT_PROD_NOT_ENABLE);
#endif /* NT_DEBUG */

    fdi_ret_t ret = FDI_RET_SUCCESS;
    /* Fill up node insertion details */
    ins_node.attr = (uint16_t)((tick_type << FDI_INS_ATTR_TICK_TYPE_OFFSET) | (p_reg->attr & FDI_NODE_ATTR_ID_MASK));
    ins_node.sq_no = g_sequence_number++;
    ins_node.identifier = identifier;

    ret = _fdi_push_ins_node(&ins_node);

    FDI_ASSERT_IF_FALSE(ret == FDI_RET_SUCCESS, ret);

    /* Call for post insertion callback if registered */
    p_reg->p_cb != NULL ? p_reg->p_cb(&ins_node) : NULL;

    return FDI_RET_SUCCESS;
}

/******************************************************************************************************
 * @brief:  Insert an Event Log to the Log Buffer from already filled insert node
 * @note:   The sequence number is going to be re-adjusted irrespective of the value in ins_node->sq_no
 *
 * @param p_reg             Pointer to Log reg, returned during fdi_reg_node as p_ret
 * @param ins_node          Pointer to the pre-filled insertion node
 *
 * @return fdi_ret_t
 *******************************************************************************************************/
fdi_ret_t fdi_node2log(const fdi_reg_t *p_reg, fdi_node_ins_t *ins_node)
{
    FDI_ASSERT_IF_FALSE(p_reg != NULL, FDI_RET_FAILED_ASSERT_PARAM);
#ifndef NT_DEBUG
    FDI_ASSERT_IF_FALSE((p_reg->attr & LOG_LEVEL_PROD) != FDI_RESET, FDI_RET_FAILED_ASSERT_PROD_NOT_ENABLE);
#endif /* NT_DEBUG */

    fdi_ret_t ret = FDI_RET_SUCCESS;
    /* Fill up node insertion details */
    ins_node->sq_no = g_sequence_number++;

    ret = _fdi_push_ins_node(ins_node);
    FDI_ASSERT_IF_FALSE(ret == FDI_RET_SUCCESS, ret);

    /* Call for post insertion callback if registered */
    p_reg->p_cb != NULL ? p_reg->p_cb(ins_node) : NULL;

    return FDI_RET_SUCCESS;
}
/********************************************************************
 * @brief Enable/Disable a node
 * @param node_id           Node_ID Enum to disable the node
 * @param en_flag           1/0 for enable or disable
 * @return fdi_ret_t
 *********************************************************************/
fdi_ret_t fdi_node_en(uint16_t node_id, uint8_t en_flag)
{
    FDI_ASSERT_IF_FALSE(node_id < FDI_MAX_NODE, FDI_RET_FAILED_ASSERT_PARAM);

    if (en_flag) {
        g_fdi.p_reg_table[node_id].attr |= (FDI_SET << FDI_NODE_ATTR_ENABLE_OFFSET);
    } else {
        g_fdi.p_reg_table[node_id].attr &= ~(FDI_SET << FDI_NODE_ATTR_ENABLE_OFFSET);
    }

    return FDI_RET_SUCCESS;
}
/******************************************************************************************************
 * @brief Enable/Disable a module
 * @param bmap              bitmap value to enable/disable the respective module Eg: BMAP_A | BMAP_B ...
 * @param en_flag           1/0 for enable or disable
 * @return fdi_ret_t
 *******************************************************************************************************/
fdi_ret_t fdi_mod_en(uint32_t bmap, uint8_t en_flag)
{
    FDI_ASSERT_IF_FALSE(bmap != FDI_MOD_DEF_BMAP, FDI_RET_FAILED_ASSERT_PARAM);

    if (en_flag) {
        g_fdi.module_bmap_en |= bmap;
    } else {
        g_fdi.module_bmap_en &= ~bmap;
    }
    NT_LOG_PRINT(COMMON, ERR, "Modules Enabled:");
    for (fdi_mod_t mod = FDI_MOD_PWR; mod < FDI_MOD_MAX; mod++) {
        if ((g_fdi.module_bmap_en & (FDI_SET << mod)) != FDI_RESET) {
            NT_LOG_PRINT(COMMON, ERR, "[%02u] : [1]", mod);
        }
    }

    return FDI_RET_SUCCESS;
}

#if FDI_EN_POST_PROCESS == FDI_SET

/*******************************
 * @brief Trigger Post process
 * @return fdi_ret_t
 *******************************/
fdi_ret_t fdi_trigger_pp()
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    g_fdi_signal_flag = FDI_SET;

    if (eTaskGetState(xFDIHandle) < eSuspended) {
        /* If FDI Task is not suspended */
        if (NT_IS_ISR) {
            xEventGroupSetBitsFromISR(xFDIEventGroup, FDI_WM_EVT_BIT_MASK, &xHigherPriorityTaskWoken);
        } else {
            xEventGroupSetBits(xFDIEventGroup, FDI_WM_EVT_BIT_MASK);
        }
    }

    return FDI_RET_SUCCESS;
}

/*********************************************
 * @brief Set the watermark object
 *
 * @param wm_percent
 * @return fdi_ret_t
 **********************************************/
fdi_ret_t fdi_set_wm(uint8_t wm_percent)
{
    FDI_ASSERT_IF_FALSE(wm_percent > FDI_FIFO_MAX_WATERMARK, FDI_RET_FAILED_ASSERT_PARAM);

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    BaseType_t sem_ret = pdFALSE;

    sem_ret = (NT_IS_ISR) ? xSemaphoreTakeFromISR(xFDI_SEM, &xHigherPriorityTaskWoken)
                          : xSemaphoreTake(xFDI_SEM, portMAX_DELAY);
    FDI_ASSERT_IF_FALSE(sem_ret != pdFALSE, FDI_RET_FAILED_ASSERT_SEMAPHORE_GET);

    g_fdi.watermark = FDI_FIFO_ABS_WATERMARK(wm_percent);
    fifo_set_wm(FDI_LOG_FIFO, g_fdi.watermark);

    (NT_IS_ISR) ? xSemaphoreGiveFromISR(xFDI_SEM, &xHigherPriorityTaskWoken) : xSemaphoreGive(xFDI_SEM);

    return FDI_RET_SUCCESS;
}
#endif /* FDI_EN_POST_PROCESS */

/**********************************************
 * @brief Initialise FDI
 * @param void
 * @return None
 *********************************************/
void fdi_init(void)
{
    if (g_fdi.module_bmap_en == FDI_MOD_DEF_BMAP) {
        g_fdi.module_bmap_en = *((uint32_t *)(nt_devcfg_get_config(NT_DEVCFG_DEFAULT_LOG_BMAP)));
    }

    if (xFDI_SEM == NULL) {
        xFDI_SEM = xSemaphoreCreateBinary();
        xSemaphoreGive(xFDI_SEM);
    }
#if FDI_EN_POST_PROCESS
    if (xFDIHandle == NULL || eTaskGetState(xFDIHandle) < eDeleted) {
        /* Init FDI Thread */
        nt_qurt_thread_create(_fdi_thread, FDI_THREAD_NAME, FDI_THREAD_STACK_SIZE, &g_fdi, FDI_THREAD_PRIOR,
                              &xFDIHandle);
        if (xFDIHandle != NULL)
            xFDIEventGroup = xEventGroupCreate();
    } else {
        return;
    }

#endif /* FDI_EN_POST_PROCESS */
}

/**************************************************
 * @brief Print Output Buffer to specified channel
 * @param clear             Reset Logs or stats
 * @return fdi_ret_t
 **************************************************/
fdi_ret_t fdi_print_log(
#if FDI_PRINT_ON_UT == FDI_RESET
    __attribute__((__unused__))
#endif
    uint8_t clear)
{
#if FDI_EN_POST_PROCESS
    uint16_t node_id = FDI_RESET;

    while (node_id < FDI_MAX_NODE) {
        if ((((g_fdi.p_reg_table[node_id].module_bmap) & g_fdi.module_bmap_en) != FDI_RESET) &&
            (g_fdi.p_reg_table[node_id].event_ctr != FDI_RESET)) {
            NT_LOG_PRINT(
                COMMON, ERR, "FDI_2:%04u:%04u:%010u:%010u:%010u:%010u;",
                (unsigned int)(g_fdi.p_reg_table[node_id].attr & FDI_NODE_ATTR_ID_MASK),
                (unsigned int)g_fdi.p_reg_table[node_id].event_ctr, (unsigned int)g_fdi.p_reg_table[node_id].tick_last,
                (unsigned int)g_fdi.p_reg_table[node_id].tick_peak, (unsigned int)g_fdi.p_reg_table[node_id].tick_avrg,
                (unsigned int)g_fdi.p_reg_table[node_id].tick_min);
            if (clear) {
                g_fdi.p_reg_table[node_id].tick_avrg = FDI_RESET;
                g_fdi.p_reg_table[node_id].tick_last = FDI_RESET;
                g_fdi.p_reg_table[node_id].tick_peak = FDI_RESET;
                g_fdi.p_reg_table[node_id].tick_min = FDI_RESET;
                g_fdi.p_reg_table[node_id].event_ctr = FDI_RESET;
            }
        }
        node_id++;
    }

#else /* !FDI_EN_POST_PROCESS */
#if FDI_PRINT_ON_UT == FDI_SET
    fdi_node_ins_t node_ret;
    size_t el_next = FDI_RESET;
    uint8_t cond = FDI_RET_SUCCESS;
    volatile uint8_t is_clear = clear;

    do {
        el_next = is_clear ? FDI_SET : el_next; /* Set el_nect if want to traverse */
        cond = is_clear != FDI_RESET ? _fdi_dequeue(FDI_LOG_FIFO, (char *)&node_ret) : /* Dequeue if want to clear */
                   _fdi_trav_q(FDI_LOG_FIFO, &el_next, (char *)&node_ret); /* Traverse only as we dont want to clear */
        if (cond == FDI_RET_SUCCESS) {
            if (((node_ret.attr >> FDI_INS_ATTR_TICK_TYPE_OFFSET) & 0x01) == TICK_TYPE_STOP_TICK) {
                NT_LOG_PRINT(COMMON, ERR, "FDI_1:%04u:%02u:SP:%010u:%u;", node_ret.sq_no,
                             node_ret.attr & FDI_NODE_ATTR_ID_MASK, node_ret.ticks, node_ret.identifier);
            } else {
                NT_LOG_PRINT(COMMON, ERR, "FDI_1:%04u:%02u:ST:%010u:%u;", node_ret.sq_no,
                             node_ret.attr & FDI_NODE_ATTR_ID_MASK, node_ret.ticks, node_ret.identifier);
            }
        }
    } while (cond == FDI_RET_SUCCESS && (el_next != FIFO_RESET));
#endif /* FDI_PRINT_ON_UT == FDI_SET */
#endif /* FDI_EN_POST_PROCESS */
    return FDI_RET_SUCCESS;
}

/**********************************************
 * @brief Get current Timestamp
 *
 * @return uint32_t
 ***********************************************/
FDI_PS_TXT uint32_t fdi_get_time_stamp(void)
{
#ifdef SUPPORT_HIGH_RES_TIMER
    return (uint32_t)hres_timer_timetick_get();
#else
    return FDI_RESET;
#endif /* SUPPORT_HIGH_RES_TIMER */
}
/*----------------------------------------------------------------------------
 * Static Function Define
 *----------------------------------------------------------------------------*/

/***************************************************************
 * @brief Push data to output buffer
 *
 * @param ins_node
 * @return fdi_ret_t
 ***************************************************************/
fdi_ret_t _fdi_push_ins_node(const fdi_node_ins_t *ins_node)
{
#if FDI_PRINT_ON_UT == FDI_SET
    /* Enqueue the node */
    return _fdi_enqueue(FDI_LOG_FIFO, (char *)ins_node);
#else
    /* Directly print on console/diag buffer */
    if (((ins_node->attr >> FDI_INS_ATTR_TICK_TYPE_OFFSET) & 0x01) == TICK_TYPE_STOP_TICK) {
        NT_LOG_PRINT(COMMON, ERR, "FDI_1:%04u:%02u:SP:%010u:%u;", ins_node->sq_no,
                     ins_node->attr & FDI_NODE_ATTR_ID_MASK, ins_node->ticks, ins_node->identifier);
    } else {
        NT_LOG_PRINT(COMMON, ERR, "FDI_1:%04u:%02u:ST:%010u:%u;", ins_node->sq_no,
                     ins_node->attr & FDI_NODE_ATTR_ID_MASK, ins_node->ticks, ins_node->identifier);
    }
    return FDI_RET_SUCCESS;
#endif
}

#if FDI_EN_POST_PROCESS == FDI_SET
/**********************************************
 * @brief FDI Post Process Thread
 *
 * @param p_queue
 **********************************************/
static void _fdi_thread(void *p_queue)
{
    fdi_t *p_node = (fdi_t *)p_queue;
    EventBits_t evt = FDI_RESET;
    fdi_node_ins_t node_ret;
    fdi_node_ins_t get_start_node;
    static size_t start_idfs_ctr = FDI_RESET;
    static size_t start_idfs_idx = FDI_RESET;

    while (FDI_SET) {
        evt = xEventGroupWaitBits(xFDIEventGroup, FDI_WM_EVT_BIT_MASK, pdTRUE, /* Clear on Exit */
                                  pdFALSE,                                     /* Wait for All Bits */
                                  portMAX_DELAY);
        if (evt == FDI_WM_EVT_BIT_MASK) {
            _swap_log_buffer();
            /* No need to lock the queue as the operation is over inactive buffer */
            while (fifo_dequeue(p_node->buffer_ptr_inactive, (char *)&node_ret) != FIFO_FAILURE) {
                /* Manage missed sequence number */
                if ((node_ret.sq_no - g_processed_sq_no) > FDI_SET) {
                    /* The sequence missed */
                    g_missed_sq_no += (node_ret.sq_no - g_processed_sq_no);
                    NT_LOG_PRINT(COMMON, ERR, "FDI_3: Missed sequence: %04u;", (unsigned int)g_missed_sq_no);
                }
                g_processed_sq_no = node_ret.sq_no;
                /* node_ret post process */
                if ((node_ret.attr & (FDI_SET << FDI_INS_ATTR_TICK_TYPE_OFFSET)) ==
                    (FDI_SET << FDI_INS_ATTR_TICK_TYPE_OFFSET)) {
                    /* STOP Node Process */
                    if ((node_ret.identifier != FDI_RESET) && (start_idfs_ctr != FDI_RESET)) {
                        /* STOP Node Process having identifier */
                        start_idfs_idx = start_idfs_ctr;
                        while (start_idfs_idx > FDI_RESET) {
                            /* Search for correspondinf START node with same identifier */
                            fifo_dequeue(p_node->buffer_ptr_idfs, (char *)&get_start_node);
                            if (node_ret.identifier == get_start_node.identifier) {
                                p_node->p_reg_table[FDI_NODE_ID(node_ret.attr)].evt_start = get_start_node.ticks;
                                start_idfs_ctr--;
                                break;
                            } else {
                                /* Enqueue it again if the identifier does not match */
                                fifo_enqueue(p_node->buffer_ptr_idfs, (char *)&get_start_node);
                            }
                            start_idfs_idx--;
                        }
                    }
                    /* Condition for Event Start Occured */
                    if (p_node->p_reg_table[FDI_NODE_ID(node_ret.attr)].evt_start != FDI_RESET) {
                        /* Increment event counter */
                        p_node->p_reg_table[FDI_NODE_ID(node_ret.attr)].event_ctr++;

                        /* Update last tick captured */
                        p_node->p_reg_table[FDI_NODE_ID(node_ret.attr)].tick_last =
                            node_ret.ticks - p_node->p_reg_table[FDI_NODE_ID(node_ret.attr)].evt_start;

                        /* Enter tick_min */
                        p_node->p_reg_table[FDI_NODE_ID(node_ret.attr)].tick_min =
                            p_node->p_reg_table[FDI_NODE_ID(node_ret.attr)].tick_min > FDI_RESET
                                ? p_node->p_reg_table[FDI_NODE_ID(node_ret.attr)].tick_min <
                                          p_node->p_reg_table[FDI_NODE_ID(node_ret.attr)].tick_last
                                      ? p_node->p_reg_table[FDI_NODE_ID(node_ret.attr)].tick_min
                                      : p_node->p_reg_table[FDI_NODE_ID(node_ret.attr)].tick_last
                                : p_node->p_reg_table[FDI_NODE_ID(node_ret.attr)].tick_last;

                        /* Enter tick_peak */
                        p_node->p_reg_table[FDI_NODE_ID(node_ret.attr)].tick_peak =
                            p_node->p_reg_table[FDI_NODE_ID(node_ret.attr)].tick_last >
                                    p_node->p_reg_table[FDI_NODE_ID(node_ret.attr)].tick_peak
                                ? p_node->p_reg_table[FDI_NODE_ID(node_ret.attr)].tick_last
                                : p_node->p_reg_table[FDI_NODE_ID(node_ret.attr)].tick_peak;

                        /* Enter tick_avg*/
                        p_node->p_reg_table[FDI_NODE_ID(node_ret.attr)].tick_avrg =
                            (((p_node->p_reg_table[FDI_NODE_ID(node_ret.attr)].tick_avrg *
                               (p_node->p_reg_table[FDI_NODE_ID(node_ret.attr)].event_ctr - FDI_SET)) +
                              p_node->p_reg_table[FDI_NODE_ID(node_ret.attr)].tick_last) /
                             p_node->p_reg_table[FDI_NODE_ID(node_ret.attr)].event_ctr);

                        /* Reset evt_start indecating the node process completed */
                        p_node->p_reg_table[FDI_NODE_ID(node_ret.attr)].evt_start = FDI_RESET;  // Reset
                    }
                } else if ((node_ret.attr & (FDI_SET << FDI_INS_ATTR_TICK_TYPE_OFFSET)) == FDI_RESET) {
                    /* START Node Process */
                    if (node_ret.identifier != FDI_RESET) {
                        /* STOP Node Process having identifier. Push it to idfs buffer */
                        fifo_enqueue(p_node->buffer_ptr_idfs, (char *)&node_ret);
                        start_idfs_ctr++;
                    } else {
                        p_node->p_reg_table[FDI_NODE_ID(node_ret.attr)].evt_start = node_ret.ticks;
                    }
                }
                taskYIELD(); /* Chaeck for any other task required to be scheduled */
            }
            g_fdi_signal_flag = FDI_RESET;
            _fdi_post_process_complete_cb(p_node->buffer_ptr_inactive);
        } else {
            taskYIELD(); /* UNKNOW STATE Context Switch*/
        }
    }
}

/**********************************************
 * @brief Post Process complete callback
 *
 *********************************************/
static void _fdi_post_process_complete_cb(fifo_t *p_fifo)
{
    NT_LOG_PRINT(COMMON, INFO, "FDI Post process complete, %u", p_fifo->n_el_curr);
}

/**********************************************
 * @brief Post Process swap log buffers
 *
 *********************************************/
static void _swap_log_buffer(void)
{
    fifo_t *temp = NULL;

    /* Swap ptrs */
    xSemaphoreTake(xFDI_SEM, portMAX_DELAY);
    temp = g_fdi.buffer_ptr_active;
    g_fdi.buffer_ptr_active = g_fdi.buffer_ptr_inactive;
    g_fdi.buffer_ptr_inactive = temp;
    xSemaphoreGive(xFDI_SEM);
}
#else /* FDI_EN_POST_PROCESS != FDI_SET */
#if FDI_PRINT_ON_UT == FDI_SET
/**********************************************
 * @brief FDI Dequeue from a queue ponter. Thread Safe
 *
 * @param q
 * @param node
 * @return fdi_ret_t
 **********************************************/
static fdi_ret_t _fdi_dequeue(fifo_t *q, char *node)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    BaseType_t sem_ret = pdFALSE;
    TickType_t wait_ticks = portMAX_DELAY;
    fifo_ret_t ret;

    /* If Scheduler is suspended then wait_ticks = 0 */
    wait_ticks = (xTaskGetSchedulerState() == taskSCHEDULER_SUSPENDED) ? FDI_RESET : portMAX_DELAY;

    sem_ret =
        (NT_IS_ISR) ? xSemaphoreTakeFromISR(xFDI_SEM, &xHigherPriorityTaskWoken) : xSemaphoreTake(xFDI_SEM, wait_ticks);
    FDI_ASSERT_IF_FALSE(sem_ret != pdFALSE, FDI_RET_FAILED_ASSERT_SEMAPHORE_GET);
    ret = fifo_dequeue(q, node);
    (NT_IS_ISR) ? xSemaphoreGiveFromISR(xFDI_SEM, &xHigherPriorityTaskWoken) : xSemaphoreGive(xFDI_SEM);

    return (fdi_ret_t)ret;
}
/**********************************************
 * @brief FDI Queue Traversal. Thread Safe
 *
 * @param q
 * @param next_el
 * @param node
 * @return fdi_ret_t
 *********************************************/
static fdi_ret_t _fdi_trav_q(fifo_t *q, size_t *next_el, char *node)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    BaseType_t sem_ret = pdFALSE;
    TickType_t wait_ticks = portMAX_DELAY;
    fifo_ret_t ret;

    /* If Scheduler is suspended then wait_ticks = 0 */
    wait_ticks = (xTaskGetSchedulerState() == taskSCHEDULER_SUSPENDED) ? FDI_RESET : portMAX_DELAY;

    sem_ret =
        (NT_IS_ISR) ? xSemaphoreTakeFromISR(xFDI_SEM, &xHigherPriorityTaskWoken) : xSemaphoreTake(xFDI_SEM, wait_ticks);
    FDI_ASSERT_IF_FALSE(sem_ret != pdFALSE, FDI_RET_FAILED_ASSERT_SEMAPHORE_GET);
    ret = fifo_trav_renterant(q, next_el, node);
    (NT_IS_ISR) ? xSemaphoreGiveFromISR(xFDI_SEM, &xHigherPriorityTaskWoken) : xSemaphoreGive(xFDI_SEM);

    return (fdi_ret_t)ret;
}
#endif /* FDI_PRINT_ON_UT */
#endif /* FDI_EN_POST_PROCESS */

#if FDI_PRINT_ON_UT == FDI_SET
/**********************************************
 * @brief FDI Enqueue. Thread safe
 *
 * @param q
 * @param node
 * @return fdi_ret_t
 *********************************************/
static fdi_ret_t _fdi_enqueue(fifo_t *q, char *node)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    BaseType_t sem_ret = pdFALSE;
    TickType_t wait_ticks = portMAX_DELAY;
    fifo_ret_t ret;

    /* If Scheduler is suspended then wait_ticks = 0 */
    wait_ticks = (xTaskGetSchedulerState() == taskSCHEDULER_SUSPENDED) ? FDI_RESET : portMAX_DELAY;

    sem_ret =
        (NT_IS_ISR) ? xSemaphoreTakeFromISR(xFDI_SEM, &xHigherPriorityTaskWoken) : xSemaphoreTake(xFDI_SEM, wait_ticks);
    FDI_ASSERT_IF_FALSE(sem_ret != pdFALSE, FDI_RET_FAILED_ASSERT_SEMAPHORE_GET);
    ret = fifo_enqueue(q, node);
    (NT_IS_ISR) ? xSemaphoreGiveFromISR(xFDI_SEM, &xHigherPriorityTaskWoken) : xSemaphoreGive(xFDI_SEM);

    return (fdi_ret_t)ret;
}

/******************************************************
 * @brief Post watermark callback for the FIFOs defined
 *
 *****************************************************/
static void _post_wm_cb(fifo_t *p_fifo)
{
#if FDI_EN_POST_PROCESS == FDI_SET
    if (g_fdi_signal_flag == FDI_RESET) {
        NT_LOG_PRINT(COMMON, INFO, "FDI Post process trigger, %u", p_fifo->n_el_curr);
        fdi_trigger_pp();
    }
#else
    NT_LOG_PRINT(COMMON, ERR, "FDI Watermark reached, %u", p_fifo->n_el_curr);
#endif /* FDI_EN_POST_PROCESS */
}

#endif

#endif /* FEATURE_FDI */
