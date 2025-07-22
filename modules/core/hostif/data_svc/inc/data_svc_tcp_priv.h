/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#ifndef DATA_SVC_TCP_PRIV_H
#define DATA_SVC_TCP_PRIV_H

/*========================================================================
 * @file	lwip_svc.h
 * @brief Lwip Handler param, struct, function declarations internal to data_svc
 *========================================================================*/

/*------------------------------------------------------------------------
 * Include Files
 * ----------------------------------------------------------------------*/
#include <stdint.h>
#include <data_svc_ip_priv.h>

/*------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * ----------------------------------------------------------------------*/
/* None */

/*------------------------------------------------------------------------
 * Type Declarations
 * ----------------------------------------------------------------------*/
/* None */

/*------------------------------------------------------------------------
 * Function Declarations
 * ----------------------------------------------------------------------*/
extern bool data_svc_tcp_listen_start_req(ip_tcp_listen_start_req_t *p_req_msg);
extern bool data_svc_tcp_listen_close_req(ip_tcp_listen_close_req_t *p_req_msg);
extern bool data_svc_tcp_client_connect_req(ip_tcp_client_connect_req_t *p_req_msg);
extern bool data_svc_tcp_connection_close_req(ip_tcp_connection_close_req_t *p_req_msg);
extern bool data_svc_tcp_closed_rsp(ip_tcp_closed_rsp_t *p_req_msg);

#endif /* DATA_SVC_TCP_PRIV_H */
