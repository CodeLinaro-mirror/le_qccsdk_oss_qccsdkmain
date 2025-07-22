/*========================================================================
Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear

* @file apps_ringif.h
* @brief Application Ring Interface header
* ======================================================================*/
#ifndef APPS_RINGIF_H
#define APPS_RINGIF_H
/*------------------------------------------------------------------------
 * Include Files
 * ----------------------------------------------------------------------*/
#include <stdbool.h>
#include <com_dtypes.h>

#include "fwconfig_wlan.h"
#include "nt_flags.h"

#ifdef SUPPORT_RING_IF
#include "nt_common.h"
#include "nt_osal.h"
#include "nt_logger_api.h"
#include "wlan_dev.h"
#include "wifi_fw_table_api.h"
#include "wifi_fw_ring_api.h"
#include "wifi_fw_mgmt_api.h"
#include "wifi_fw_ip_conn_api.h"

/*------------------------------------------------------------------------
 * Definitions
 * ----------------------------------------------------------------------*/
#define A2F_RING_ID_CONFIG 0
#define F2A_RING_ID_CONFIG 0

#define APPS_RINGIF_AVOID_RX_PKT_COPY
#define APPS_RINGIF_AVOID_TX_PKT_COPY

#define APPS_RINGIF_LOG_ERR(...) NT_LOG_PRINT(COMMON, ERR, __VA_ARGS__)

#if 0
extern uint8_t ringif_dbg_log_lvl(void);
#define APPS_RINGIF_LOG_INFO(...)                \
    if (ringif_dbg_log_lvl() > 0) {              \
        NT_LOG_PRINT(COMMON, ERR, __VA_ARGS__);  \
    } else {                                     \
        NT_LOG_PRINT(COMMON, INFO, __VA_ARGS__); \
    }
#else
#define APPS_RINGIF_LOG_INFO(...) NT_LOG_PRINT(COMMON, INFO, __VA_ARGS__)
#endif
/*------------------------------------------------------------------------
 * Type Declarations
 * ----------------------------------------------------------------------*/
typedef struct apps_ring_ctx {
    uint8_t ring_state;     /* UN_INITIALIZED(0)/IN_USE(1) */
    uint8_t ring_id;        /* Ring ID */
    uint8_t ring_elem_size; /* Actual size of each ring elem */
    uint8_t ring_num_elem;  /* Maximum number of Ring Elements */

    void *p_ring_base; /* Ptr to 0th element of the ring */
} apps_ring_ctx_t;

typedef void (*_pfn_tx_callback)(uint16_t len);

extern uint8_t apps_ringif_a2f_write_n_pkts(uint8_t ring_id, void *p_input_buf, uint16_t len, uint16_t info,
                                            uint8_t num_pkts, _pfn_tx_callback pfn_callback);
extern bool apps_ringif_a2f_write_pkt(uint8_t ring_id, void *p_input_buf, uint16_t len, uint16_t info);
extern bool apps_ringif_wifi_send_cmd(uint16_t num_args, uint32_t *args);
extern bool apps_ringif_wifi_print_rsp(void *p_buf, uint16_t len);
extern bool apps_ringif_ip_connect_print_rsp(void *p_buf, uint16_t len);
extern bool apps_ringif_ip_connect_cmd(uint16_t num_args, uint32_t *args);
extern bool apps_ringif_ip_send_data_pkt(uint16_t num_args, uint32_t *args);
extern bool apps_ringif_raweth_print_rsp(void *p_buf, uint16_t len);
extern bool apps_ringif_raweth_cmd(uint16_t num_args, uint32_t *args);
extern bool apps_ringif_chk_update_a2f_ctx(uint8_t ring_id);
extern bool apps_ringif_chk_update_f2a_ctx(uint8_t ring_id);
extern bool apps_ringif_read_all_f2a_rings();
extern uint8_t apps_ringif_send_n_pkts(uint8_t ring_id, uint16_t info, uint8_t num_pkts);

extern void apps_ringif_dp_test_init(void);
extern bool apps_ringif_dp_rx_pkt(uint8_t ring_id, uint16_t info, void *p_apps_buf, uint16_t len_bytes);
extern bool apps_ringif_dp_test_tasks(void);

extern void apps_ringif_update_all_indexes(void);
extern bool apps_ringif_update_apps_a2f_ctx(uint8_t ring_id);
extern bool apps_ringif_update_apps_f2a_ctx(uint8_t ring_id);
extern void *apps_ringif_allocate_mem(uint16_t len);
extern void apps_ringif_free_mem(void *buf);
extern void apps_ringif_prepare_tx_buff(void *p_tx_buf, uint16_t len_bytes);
#endif /* SUPPORT_RING_IF */
#endif /* APPS_RINGIF_H */
