/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 *    @file
 *          Provides an implementation of the DiagnosticDataProvider object.
 */

#pragma once

#include <memory>

#include <platform/DiagnosticDataProvider.h>

namespace chip {
    namespace DeviceLayer {

        /**
         * Concrete implementation of the PlatformManager singleton object for Linux platforms.
         */
        class DiagnosticDataProviderImpl : public DiagnosticDataProvider {
        public:
            static DiagnosticDataProviderImpl &GetDefaultInstance();

            // ===== Methods that implement the PlatformManager abstract interface.

            /*
            CHIP_ERROR GetCurrentHeapFree(uint64_t & currentHeapFree) override;
            CHIP_ERROR GetCurrentHeapUsed(uint64_t & currentHeapUsed) override;
            CHIP_ERROR GetCurrentHeapHighWatermark(uint64_t & currentHeapHighWatermark) override;
            CHIP_ERROR GetThreadMetrics(ThreadMetrics ** threadMetricsOut) override;
            void ReleaseThreadMetrics(ThreadMetrics * threadMetrics) override;

            CHIP_ERROR GetRebootCount(uint16_t & rebootCount) override;
            CHIP_ERROR GetUpTime(uint64_t & upTime) override;
            CHIP_ERROR GetTotalOperationalHours(uint32_t & totalOperationalHours) override;
            CHIP_ERROR GetBootReason(uint8_t & bootReason) override;

            CHIP_ERROR GetActiveHardwareFaults(GeneralFaults<kMaxHardwareFaults> & hardwareFaults) override;
            CHIP_ERROR GetActiveRadioFaults(GeneralFaults<kMaxRadioFaults> & radioFaults) override;
            CHIP_ERROR GetActiveNetworkFaults(GeneralFaults<kMaxNetworkFaults> & networkFaults) override;
            */
        };
        DiagnosticDataProvider &GetDiagnosticDataProviderImpl();

    }  // namespace DeviceLayer
}  // namespace chip
