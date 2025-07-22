/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*========================================================================
 * @file ring_ctx_holder.h
 * @brief Ring Context Holder param and struct definitions
 * ======================================================================*/
#ifndef RING_CTX_HOLDER_H
#define RING_CTX_HOLDER_H
/*------------------------------------------------------------------------
 * Include Files
 * ----------------------------------------------------------------------*/
#include "fwconfig_cmn.h"
#include "nt_flags.h"

#if defined(SUPPORT_RING_IF) || defined(SUPPORT_RING_IF_ONLY)
#include "stdbool.h"
#include "com_dtypes.h"
#include "nt_common.h"
#include "nt_osal.h"
#include "nt_logger_api.h"
#include "ring_svc_api.h"
#include "wifi_fw_ring_api.h"

/*------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * ----------------------------------------------------------------------*/
#define RINGIF_PRINT_LOG_ERR(...) NT_LOG_PRINT(COMMON, ERR, __VA_ARGS__)

#define RINGIF_PRINT_LOG_INFO(...)               \
    if (ringif_dbg_log_lvl() > 0) {              \
        NT_LOG_PRINT(COMMON, ERR, __VA_ARGS__);  \
    } else {                                     \
        NT_LOG_PRINT(COMMON, INFO, __VA_ARGS__); \
    }

#define WIFI_FW_UNUSED_ARG(x)            (void)x
#define A_COMPILE_TIME_ASSERT(predicate) typedef char ring_if_lib[(predicate) ? 1 : -1];
#define A_ASSERT(x)                      configASSERT(x)

#define MAX_UDP_RINGS      1
#define MAX_TCP_RINGS      6
#define MAX_NUM_DATA_RINGS (MAX_UDP_RINGS + MAX_TCP_RINGS)

#define MAX_NUM_CTRL_RINGS 1

#if defined(SUPPORT_RING_IF_ONLY)
#define MAX_NUM_F2A_RINGS (MAX_NUM_DATA_RINGS + MAX_NUM_CTRL_RINGS)
#define MAX_NUM_A2F_RINGS (MAX_NUM_DATA_RINGS + MAX_NUM_CTRL_RINGS)
#else
#define MAX_NUM_F2A_RINGS 2
#define MAX_NUM_A2F_RINGS 2
#endif

#define NUM_F2A_DUMMY_BYTES (8 - (MAX_NUM_F2A_RINGS % 4))
#define NUM_A2F_DUMMY_BYTES (8 - (MAX_NUM_A2F_RINGS % 4))

/* Ring IF Timer timeout value in milliseconds */
#define RING_IF_TIMEOUT 5
/*------------------------------------------------------------------------
 * Type Declarations
 * ----------------------------------------------------------------------*/
typedef bool (*_pfn_post2thread)(uint8_t ring_id, bool b_from_ISR, void *p_usr_ctx);

/* Enumeration for Config info */
typedef enum { RING_STATE_INVALID = 0, RING_STATE_IN_USE } RING_STATE_ENUM;

/* Enumeration for Config info */
typedef enum { RING_DIR_A2F = 0, RING_DIR_F2A } RING_DIR;

typedef struct ring_ctx {
    void *p_ring_base;             /* Ptr to 0th element of the ring */
    volatile uint8_t *p_write_idx; /* Ptr to Index/Write offset */
    volatile uint8_t *p_read_idx;  /* Ptr to Outdex/Read offset */

    uint8_t ring_state;     /* UN_INITIALIZED(0)/IN_USE(1) */
    uint8_t ring_id;        /* Ring ID */
    uint8_t ring_elem_size; /* Actual size of each ring elem */
    uint8_t ring_num_elem;  /* Maximum number of Ring Elements */
    uint8_t ring_idx_to_clear;
    uint8_t ring_idx_clear_pending;

    void *pfn_post2thread;
    void *p_usr_ctx;
} ring_ctx_t;

#ifdef SUPPORT_RING_IF_STATS
typedef struct ring_stats {
    uint32_t num_pkts_attached;
    uint32_t num_pkts_detached;
    uint32_t num_attach_failures;
    uint32_t num_detach_failures;
} ring_stats_t;
#endif /* SUPPORT_RING_IF_STATS */

extern nt_osal_timer_handle_t ringif_timer;
/*------------------------------------------------------------------------
 * Function Declarations and Documentation
 * ----------------------------------------------------------------------*/
/* Ring creation functions to be called by data path module */
uint8_t ringif_create_a2f_ring(void *p_usr_ctx, _pfn_post2thread pfn_post2thread, uint8_t elem_size,
                               uint8_t max_num_elem);
uint8_t ringif_create_f2a_ring(void *p_usr_ctx, _pfn_post2thread pfn_post2thread, uint8_t elem_size,
                               uint8_t max_num_elem);

uint8_t ringif_dbg_log_lvl(void);

/* Indication to Host*/
bool ringif_indicate_to_host(uint8_t ring_id, RING_DIR dir_a2f0_f2a1);

/* Function to check ring full/empty */
bool ringif_ring_full(ring_ctx_t *p_ring_ctx);
bool ringif_ring_empty(ring_ctx_t *p_ring_ctx);

/* API to be used by other modules to get ring context pointer */
void *ringif_a2f_ring_ctx(uint8_t ring_id);
void *ringif_f2a_ring_ctx(uint8_t ring_id);

/* APIs for manipulating the A2F ring */
void *ringif_a2f_element_ptr(uint8_t ring_id, uint8_t idx);
void *ringif_a2f_next_read_elem(uint8_t ring_id);
uint8_t ringif_a2f_num_pending_pkts(uint8_t ring_id);
bool ringif_a2f_refill_next_empty_elem(uint8_t ring_id, _pfn_refill_elem pfn_refill_elem);
bool ringif_a2f_mark_as_read(uint8_t ring_id, uint8_t num_read_elem);
bool ringif_a2f_refill_bufs(uint8_t ring_id, _pfn_refill_elem pfn_refill_elem);
uint8_t ringif_a2f_get_read_idx(uint8_t ring_id, uint8_t *p_read_idx);

/* API to clear used buffers of F2A rings */
uint8_t ringif_f2a_num_elems_to_clear(ring_ctx_t *p_ring_ctx);
bool ringif_f2a_clear_used_bufs(uint8_t ring_id, _pfn_clear_elem pfn_clear_elem);
#endif /* SUPPORT_RING_IF */
#endif /* RING_CTX_HOLDER_H */
