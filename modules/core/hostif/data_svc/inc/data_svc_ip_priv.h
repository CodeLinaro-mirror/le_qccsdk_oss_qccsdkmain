/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#ifndef IP_DATA_SVC_PRIV_H
#define IP_DATA_SVC_PRIV_H

/*========================================================================
 * @file	lwip_svc.h
 * @brief Lwip Handler param, struct, function declarations internal to data_svc
 *========================================================================*/

/*------------------------------------------------------------------------
 * Include Files
 * ----------------------------------------------------------------------*/
#include <stdint.h>
#include <data_svc_priv.h>

/*------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * ----------------------------------------------------------------------*/
#define IS_BIT_SET(bit, num) (num >> (bit)) & 0x01
#define SET_BIT(bit, num)    (num | (1 << (bit)))

/*------------------------------------------------------------------------
 * Type Declarations
 * ----------------------------------------------------------------------*/

typedef enum {
    IP_DATA_SVC_CLIENT = 0,
    IP_DATA_SVC_SERVER = 1,
    IP_DATA_SVC_LISTEN = 2,
    IP_DATA_SVC_INVALID_ROLE = 3,
} lwip_svc_role_t;

typedef enum {
    IP_DATA_SVC_NOT_CONNECTED = 0,
    IP_DATA_SVC_CONNECTED = 1,
} lwip_svc_state_t;

#endif /* IP_DATA_SVC_PRIV_H */
