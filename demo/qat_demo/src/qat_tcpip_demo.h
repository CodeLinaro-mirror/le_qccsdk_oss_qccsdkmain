/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*-------------------------------------------------------------------------
 * Include Files
 *-----------------------------------------------------------------------*/
#include "icmp.h"

typedef struct icmp_echo_hdr icmp_echo_hdr;

typedef enum {
	QAT_OK,
	QAT_ERROR
} qat_tcpip_status;

typedef enum {
	INACTIVE,
	ACTIVE
} qat_connect_status;

typedef enum{
	DHCP_TURN_ON = 0,
	DHCP_TURN_OFF= 1
} dhcp_action;