/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 *    @file
 *          Platform-specific configuration overrides for the CHIP Inet
 *          Layer on the Fermion platform.
 *
 */

#pragma once

// ==================== Platform Adaptations ====================

// ==================== General Configuration Overrides ====================

#define INET_CONFIG_NUM_TCP_ENDPOINTS 4

#define IPV6_MULTICAST_IMPLEMENTED

#define INET_CONFIG_NUM_UDP_ENDPOINTS 4
