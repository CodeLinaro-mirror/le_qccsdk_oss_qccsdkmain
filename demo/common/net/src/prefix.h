/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef LWIP_PREFIX_H
#define LWIP_PREFIX_H

#include "lwip/ip_addr.h"

void prefix_send(ip_addr_t *ip_addr, uint32_t prefixlen, uint32_t prefix_lifetime, uint32_t valid_lifetime,
                 uint8_t netid);
#endif /* LWIP_PREFIX_H */
