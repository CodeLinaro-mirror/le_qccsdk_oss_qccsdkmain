/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 *    @file
 *          Provides an implementation of the PlatformManager object
 *          for the Fermion platform.
 */
/* this file behaves like a config.h, comes first */
#include <platform/internal/CHIPDeviceLayerInternal.h>

#include <platform/fermion/DiagnosticDataProviderImpl.h>
#include <platform/FreeRTOS/SystemTimeSupport.h>
#include <platform/PlatformManager.h>
#include <platform/internal/GenericPlatformManagerImpl_FreeRTOS.ipp>

namespace chip {
    namespace DeviceLayer {

        /** Singleton instance of the KeyValueStoreManager implementation object.
         */
        PlatformManagerImpl PlatformManagerImpl::sInstance;

        CHIP_ERROR PlatformManagerImpl::_InitChipStack(void)
        {
            CHIP_ERROR err = CHIP_NO_ERROR;

            // Initialize the configuration system.
            SetDiagnosticDataProvider(&DiagnosticDataProviderImpl::GetDefaultInstance());

            ReturnErrorOnFailure(System::Clock::InitClock_RealTime());

            err = Internal::FermionConfig::Init();
            SuccessOrExit(err);

            // Call _InitChipStack() on the generic implementation base class
            // to finish the initialization process.
            err = Internal::GenericPlatformManagerImpl_FreeRTOS<PlatformManagerImpl>::_InitChipStack();
            SuccessOrExit(err);

        exit:
            return err;
        }

        void PlatformManagerImpl::_Shutdown()
        {
            Internal::GenericPlatformManagerImpl_FreeRTOS<PlatformManagerImpl>::_Shutdown();
            return;
        }

    }  // namespacr DeviceLayer
}  // namespace chip
