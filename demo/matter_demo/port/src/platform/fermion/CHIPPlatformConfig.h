/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 *    @file
 *          Platform-specific configuration overrides for CHIP on
 *          Fermion platform.
 */

#pragma once

#include <stdint.h>

// ==================== General Platform Adaptations ====================

#define CHIP_LOG_FILTERING 0

// ==================== Security Adaptations ====================

// ==================== General Configuration Overrides ====================
/**
 *  @def CHIP_CONFIG_MAX_FABRICS
 *
 *  @brief
 *    Maximum number of fabrics the device can participate in.  Each fabric can
 *    provision the device with its unique operational credentials and manage
 *    its own access control lists.
 */
#define CHIP_CONFIG_MAX_FABRICS 4

#define CHIP_CONFIG_MAX_EXCHANGE_CONTEXTS 8

//#define CHIP_CONFIG_ENABLE_SERVER_IM_EVENT 0

// ==================== Security Configuration Overrides ====================

// ==================== FreeRTOS Configuration Overrides ====================
