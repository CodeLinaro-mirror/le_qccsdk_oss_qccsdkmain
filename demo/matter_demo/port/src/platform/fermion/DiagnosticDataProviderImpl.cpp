/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 *    @file
 *          Provides an implementation of the DiagnosticDataProvider object
 *          for Fermion platform.
 */

#include <platform/internal/CHIPDeviceLayerInternal.h>

#include <lib/support/logging/CHIPLogging.h>
#include <platform/DiagnosticDataProvider.h>
#include <platform/fermion/DiagnosticDataProviderImpl.h>

namespace chip {
    namespace DeviceLayer {

        DiagnosticDataProviderImpl &DiagnosticDataProviderImpl::GetDefaultInstance()
        {
            static DiagnosticDataProviderImpl sInstance;
            return sInstance;
        }

        DiagnosticDataProvider &GetDiagnosticDataProviderImpl()
        {
            return DiagnosticDataProviderImpl::GetDefaultInstance();
        }

    }  // namespace DeviceLayer
}  // namespace chip
