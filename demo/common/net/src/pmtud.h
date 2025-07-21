/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef PMTUD_H
#define PMTUD_H

/*-------------------------------------------------------------------------
 * Include Files
 *-----------------------------------------------------------------------*/
#include "lwip/ip_addr.h"
#include "stdint.h"
#include "lwip/icmp6.h"
#include "lwip/icmp.h"

/*-------------------------------------------------------------------------
 * Preprocessor Definitions, Constants, and Type Declarations
 *-----------------------------------------------------------------------*/

/**
 * PMTUD_DEBUG_ENABLE: Set to 1 to open the print for mtud
 */
#ifndef PMTUD_DEBUG_ENABLE
#define PMTUD_DEBUG_ENABLE 1  // DEBUG_ENABLE
#endif

#define MTU_ERR_STATUS  -1  // receive error
#define MTU_ERR_TIMEOUT -2  // timeout
#define MTU_ERR_SOCK    -3  // socket creation error
#define MTU_ERR_BUFFER  -4  // invalid buffer

typedef struct icmp_echo_hdr icmp4_echo;

typedef struct icmp_echo_header {
    union {
        icmp4_echo icmp_4;
    } u_icmp;
} icmp_echo_header;

/** @ingroup icmp6
 * Convert generic icmp to specific protocol version
 */
#define icmpm_2_icmp6(ipaddr) (&((ipaddr)->u_icmp.icmp_6))
#define icmpm_2_icmp(ipaddr)  (&((ipaddr)->u_icmp.icmp_4))
#define icmpm_2_icmpg(ipaddr) (&((ipaddr)->u_icmp.icmp_4))

/*-------------------------------------------------------------------------
 * Function Definitions
 *-----------------------------------------------------------------------*/
int32_t qapi_Path_MTU_Discover(ip_addr_t *ip_addr);

#endif  // PMTUD_H
