#ifndef DATA_SVC_HFC_PRIV_H
#define DATA_SVC_HFC_PRIV_H

/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*------------------------------------------------------------------------
 * Include Files
 * ----------------------------------------------------------------------*/
#include <stdint.h>

/*------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * ----------------------------------------------------------------------*/
/* None */

/*------------------------------------------------------------------------
 * Type Declarations
 * ----------------------------------------------------------------------*/

/** Function prototype for functions passed to hfc_callback() */
typedef void (*hfc_callback_fn)(void *ctx);
typedef struct hfc_cb {
    hfc_callback_fn fun;
    void *ctx;
} hfc_cb_t;

/*------------------------------------------------------------------------
 * Function Declarations
 * ----------------------------------------------------------------------*/
extern bool data_svc_get_hfc_data_buff(void *p_element, uint16_t len);
extern bool data_svc_free_hfc_data_buff(void *p_element);
extern int32_t data_svc_hfc_send_data_pkt(void *p_buff, uint8_t *payload, uint16_t len, uint16_t info);
extern int32_t data_svc_hfc_recv_data_pkt(void *p_buff, uint16_t *buf_len, uint32_t timeout, uint16_t *data_len,
                                          uint16_t *info);
extern bool process_hfc_data_pkt(void *p_element);
extern bool process_hfc_config_pkt(void *p_element);
extern err_t hfc_data_try_callback(hfc_callback_fn fun, void *ctx);
extern void qcspi_hfc_init(void);
#endif /* DATA_SVC_HFC_PRIV_H */
