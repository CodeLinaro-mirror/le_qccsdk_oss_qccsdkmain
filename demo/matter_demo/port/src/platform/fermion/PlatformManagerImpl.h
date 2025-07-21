/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 *    @file
 *          Stub platform manager for Fermion platform.
 */

#pragma once

#include <platform/internal/GenericPlatformManagerImpl_FreeRTOS.h>

#include <queue>

namespace chip {
    namespace DeviceLayer {

        /**
         * Concrete implementation of the PlatformManager singleton object for qca platforms.
         */
        class PlatformManagerImpl final : public PlatformManager,
                                          public Internal::GenericPlatformManagerImpl_FreeRTOS<PlatformManagerImpl> {
            // Allow the PlatformManager interface class to delegate method calls to
            // the implementation methods provided by this class.
            friend PlatformManager;

            // Allow the generic implementation base class to call helper methods on
            // this class.
#ifndef DOXYGEN_SHOULD_SKIP_THIS
            friend Internal::GenericPlatformManagerImpl_FreeRTOS<PlatformManagerImpl>;
#endif

        public:
            // ===== Platform-specific members that may be accessed directly by the application.

            System::Clock::Timestamp GetStartTime()
            {
                return mStartTime;
            }

        private:
            // ===== Methods that implement the PlatformManager abstract interface.

            CHIP_ERROR _InitChipStack(void);
            void _Shutdown(void);

            // ===== Members for internal use by the following friends.

            friend PlatformManager &PlatformMgr();
            friend PlatformManagerImpl &PlatformMgrImpl();
            // friend class Internal::BLEManagerImpl;

            System::Clock::Timestamp mStartTime = System::Clock::kZero;

            static PlatformManagerImpl sInstance;
        };

        /**
         * Returns the public interface of the PlatformManager singleton object.
         *
         * chip applications should use this to access features of the PlatformManager object
         * that are common to all platforms.
         */
        inline PlatformManager &PlatformMgr()
        {
            return PlatformManagerImpl::sInstance;
        }

        /**
         * Returns the platform-specific implementation of the PlatformManager singleton object.
         *
         * chip applications can use this to gain access to features of the PlatformManager
         * that are specific to the ESP32 platform.
         */
        inline PlatformManagerImpl &PlatformMgrImpl()
        {
            return PlatformManagerImpl::sInstance;
        }

    }  // namespace DeviceLayer
}  // namespace chip
