/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/* this file behaves like a config.h, comes first */
#include <platform/internal/CHIPDeviceLayerInternal.h>

#include <platform/ConnectivityManager.h>
#include <platform/CHIPDeviceLayer.h>
#include <platform/fermion/FermionUtils.h>
#include <platform/internal/GenericConnectivityManagerImpl_UDP.ipp>
#if INET_CONFIG_ENABLE_TCP_ENDPOINT
#include <platform/internal/GenericConnectivityManagerImpl_TCP.ipp>
#endif
#if CHIP_DEVICE_CONFIG_ENABLE_WIFI
#include <platform/internal/GenericConnectivityManagerImpl_WiFi.ipp>
#endif

extern "C" {
//#include <qapi/qurt_thread.h>
//#include <qapi/qurt_timer.h>
}

using namespace ::chip;
using namespace ::chip::TLV;
using namespace ::chip::DeviceLayer::Internal;
namespace chip {
    namespace DeviceLayer {

        /** Singleton instance of the ConnectivityManager implementation object.
         */
        ConnectivityManagerImpl ConnectivityManagerImpl::sInstance;

        CHIP_ERROR ConnectivityManagerImpl::_Init()
        {
            // Initialize the generic base classes that require it.
#if CHIP_DEVICE_CONFIG_ENABLE_WIFI
            InitWiFi();
#endif
            return CHIP_NO_ERROR;
        }

        void ConnectivityManagerImpl::_OnPlatformEvent(const ChipDeviceEvent *event)
        {
            // Forward the event to the generic base classes as needed.
#if CHIP_DEVICE_CONFIG_ENABLE_WIFI
            OnWiFiPlatformEvent(event);
#endif
        }

    }  // namespace DeviceLayer
}  // namespace chip
