/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#ifndef DATA_SVC_HFC_H
#define DATA_SVC_HFC_H

/*------------------------------------------------------------------------
 * Include Files
 * ----------------------------------------------------------------------*/
#include <stdint.h>
#include "qapi_status.h"
/*------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * ----------------------------------------------------------------------*/
/* None */

/*------------------------------------------------------------------------
 * Type Declarations
 * ----------------------------------------------------------------------*/
typedef enum {
    WLAN_DISCONNECT_EVENT,
    WLAN_CONNECT_EVENT,
} f2a_event_type;

typedef enum { HFC_CTRL_MSG, HFC_DATA_MSG } hfc_msg_type_t;

typedef struct {
    uint16_t msg_id;
    uint8_t reserved[2];
} hfc_msg_hdr;

typedef struct hfc_msg {
    hfc_msg_type_t type;
    uint32_t id;
    void *buf;
    uint8_t *data;
    uint16_t len;
    uint8_t reserved[2];
} hfc_msg_t;

/*------------------------------------------------------------------------
 * Function Declarations
 * ----------------------------------------------------------------------*/
extern int32_t data_svc_hfc_send_data_pkt(void *p_buff, uint8_t *payload, uint16_t len, uint16_t info);
extern int32_t data_svc_hfc_recv_data_pkt(void *p_buff, uint16_t *buf_len, uint32_t timeout, uint16_t *data_len,
                                          uint16_t *info);
extern int32_t data_svc_hfc_recv_msg(hfc_msg_t *msg, uint32_t timeout);
extern qbool_t data_svc_hfc_send_config(uint32_t *p_buf, uint16_t len);
extern uint32_t data_svc_set_gpio_assert_info(uint32_t info);
#endif /* DATA_SVC_HFC_H */
