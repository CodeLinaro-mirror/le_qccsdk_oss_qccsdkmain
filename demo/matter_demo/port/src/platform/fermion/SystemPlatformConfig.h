/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 *    @file
 *          Platform-specific configuration overrides for the CHIP System
 *          Layer on QCA Platform.
 *
 */

#pragma once

#include <stdint.h>

namespace chip {
    namespace DeviceLayer {
        struct ChipDeviceEvent;
    }  // namespace DeviceLayer
}  // namespace chip

// ==================== Platform Adaptations ====================
#define CHIP_SYSTEM_CONFIG_PLATFORM_PROVIDES_TIME 1
#define CHIP_SYSTEM_CONFIG_EVENT_OBJECT_TYPE      const struct ::chip::DeviceLayer::ChipDeviceEvent *

// ========== Platform-specific Configuration Overrides =========

#ifndef CHIP_SYSTEM_CONFIG_NUM_TIMERS
#define CHIP_SYSTEM_CONFIG_NUM_TIMERS 16
#endif  // CHIP_SYSTEM_CONFIG_NUM_TIMERS

#define CHIP_CONFIG_MDNS_CACHE_SIZE 4
