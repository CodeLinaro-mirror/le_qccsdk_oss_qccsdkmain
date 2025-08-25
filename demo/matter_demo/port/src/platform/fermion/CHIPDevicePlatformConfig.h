/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 *    @file
 *          Platform-specific configuration overrides for the Chip Device Layer
 *          on the QCA.
 */

#pragma once

// ==================== Platform Adaptations ====================

#define CHIP_DEVICE_CONFIG_ENABLE_CHIPOBLE 0

#define CHIP_DEVICE_CONFIG_CHIPOBLE_DISABLE_ADVERTISING_WHEN_PROVISIONED 0

// ========== Platform-specific Configuration =========

// These are configuration options that are unique to the platform.
// These can be overridden by the application as needed.

// ...

// ========== Platform-specific Configuration Overrides =========

#define CHIP_DEVICE_CONFIG_CHIP_TASK_STACK_SIZE (8 * 1024)

#define CHIP_DEVICE_CONFIG_MAX_EVENT_QUEUE_SIZE 50
