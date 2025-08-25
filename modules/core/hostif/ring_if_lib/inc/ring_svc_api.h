/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*========================================================================
 * @file ring_svc_api.h
 * @brief Ring Service API param and function declarations
 * ======================================================================*/
#ifndef RING_SVC_API_H
#define RING_SVC_API_H
/*------------------------------------------------------------------------
 * Include Files
 * ----------------------------------------------------------------------*/
#include <stdbool.h>
#include "fwconfig_cmn.h"
#include "nt_flags.h"

#if defined(SUPPORT_RING_IF) || defined(SUPPORT_RING_IF_ONLY)
#include "com_dtypes.h"
#include "nt_common.h"
#include "wifi_fw_ring_api.h"

/*------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * ----------------------------------------------------------------------*/
#define INVALID_RING_ID 0xFF

/*------------------------------------------------------------------------
 * Type Declarations
 * ----------------------------------------------------------------------*/
typedef struct _ring_element {
    uint32_t *p_buf; /* Pointer to Fermion memory buffer*/
    uint16_t len;    /* Length of the packet */
    uint16_t info;   /* Info describing the packet type */

    /* Scratch buff */
    void *p_buf_start; /* Start of the buf used for freeing */
} ring_element_t;

typedef bool (*_pfn_clear_elem)(void *p_element);
typedef bool (*_pfn_refill_elem)(void *p_element);
typedef bool (*_pfn_process)(ring_element_t *p_elem);
/*------------------------------------------------------------------------
 * Function Declarations and Documentation
 * ----------------------------------------------------------------------*/
/* API exposed to Init module */
void ringif_init(void);
void ringif_set_dbg_log_lvl(uint8_t dbg_log_lvl);

/* API to be registered with Interrupt service */
void ringif_apps_ring_update_isr(void);

/* APIs used to tickle a ring so that updates get handled */
void ringif_tickle_a2f_ring(uint8_t ring_id, bool b_from_isr);
void ringif_tickle_f2a_ring(uint8_t ring_id, bool b_from_isr);
void ringif_tickle_all_rings(bool b_from_isr);

/* APIs to pause/resume rings from power save module */
void ringif_pause_all_rings(bool b_from_isr, bool b_process_pending_entries);
void ringif_resume_all_rings(bool b_from_isr);

/* APIs used for filling config responses for ring creation */
void *ringif_a2f_ring_addr(uint8_t ring_id);
void *ringif_f2a_ring_addr(uint8_t ring_id);

uint16_t ringif_a2f_num_ring_elems(uint8_t ring_id);
uint16_t ringif_f2a_num_ring_elems(uint8_t ring_id);

uint8_t ringif_a2f_elem_size(uint8_t ring_id);
uint8_t ringif_f2a_elem_size(uint8_t ring_id);

void *ringif_a2f_rd_idx_array(void);
void *ringif_f2a_wr_idx_array(void);
void *ringif_a2f_wr_idx_array(void);
void *ringif_f2a_rd_idx_array(void);

uint16_t ringif_max_num_a2f_rings(void);
uint16_t ringif_max_num_f2a_rings(void);

/* APIs exposed for control ring access */

/* buffer based allocate/free APIs */
void *ringif_get_ctrl_buf(uint8_t flags, uint16_t *p_len_ptr);
bool ringif_free_ctrl_buf(void *p_buf_start, uint16_t len);

/* element based allocate/free APIs */
bool ringif_refill_ctrl_elem(void *p_ctrl_elem);
bool ringif_clear_ctrl_elem(void *p_ctrl_elem);

bool ringif_send_wifi_config(uint32_t *p_buf, uint16_t len);
bool ringif_send_hfc_config(uint32_t *p_buf, uint16_t len);
bool ringif_send_ip_conn_config(uint32_t *p_buf, uint16_t len);
bool ringif_send_ip_secu_config(uint32_t *p_buf, uint16_t len);
bool ringif_send_raweth_config(uint32_t *p_buf, uint16_t len);

/* APIs exposed for data ring creation */
bool ringif_create_a2f_udp_ring(uint8_t *p_ring_id);
bool ringif_create_f2a_udp_ring(uint8_t *p_ring_id);

bool ringif_create_a2f_raweth_ring(uint8_t *p_ring_id);
bool ringif_create_f2a_raweth_ring(uint8_t *p_ring_id);

bool ringif_create_a2f_tcp_ring(uint8_t *p_ring_id);
bool ringif_create_f2a_tcp_ring(uint8_t *p_ring_id);

/* APIs exposed for data ring deletion */
bool ringif_delete_a2f_udp_ring(uint8_t ring_id);
bool ringif_delete_f2a_udp_ring(uint8_t ring_id);

bool ringif_delete_a2f_raweth_ring(uint8_t ring_id);
bool ringif_delete_f2a_raweth_ring(uint8_t ring_id);

bool ringif_delete_a2f_tcp_ring(uint8_t ring_id);
bool ringif_delete_f2a_tcp_ring(uint8_t ring_id);

/* APIs exposed for ring deletion  */
bool ringif_a2f_delete_ring(uint8_t ring_id, _pfn_clear_elem pfn_clear_elem);
bool ringif_f2a_delete_ring(uint8_t ring_id, _pfn_clear_elem pfn_clear_elem);

/* API for attaching a packet to the ring */
bool ringif_f2a_pkt_attach(uint8_t ring_id, uint32_t *p_buf_start, uint32_t *p_buf, uint16_t len, uint16_t info);

/* API for detaching multiple packets from the ring */
uint8_t ringif_a2f_process_pkts(uint8_t ring_id, _pfn_process pfn_process, _pfn_refill_elem pfn_refill);

/* API function to get the control buffer size */
uint16_t ringif_get_ctrl_buf_size(void);

#ifdef SUPPORT_RING_IF_STATS
void ringif_print_a2f_ring_stats(uint8_t ring_id);
void ringif_clear_a2f_ring_stats(uint8_t ring_id);
void ringif_print_f2a_ring_stats(uint8_t ring_id);
void ringif_clear_f2a_ring_stats(uint8_t ring_id);
void ringif_print_all_ring_stats();
#else /* SUPPORT_RING_IF_STATS */
#define ringif_print_a2f_ring_stats(a)
#define ringif_clear_a2f_ring_stats(a)
#define ringif_print_f2a_ring_stats(a)
#define ringif_clear_f2a_ring_stats(a)
#define ringif_print_all_ring_stats()
#endif /* SUPPORT_RING_IF_STATS */

#endif /* SUPPORT_RING_IF */
#endif /* RING_SVC_API_H */
