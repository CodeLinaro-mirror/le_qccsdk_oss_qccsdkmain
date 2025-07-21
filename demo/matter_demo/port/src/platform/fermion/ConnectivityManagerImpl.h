/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once

#include <platform/ConnectivityManager.h>
#include <platform/internal/GenericConnectivityManagerImpl.h>
#include <platform/internal/GenericConnectivityManagerImpl_NoBLE.h>
#include <platform/internal/GenericConnectivityManagerImpl_NoThread.h>

#if CHIP_DEVICE_CONFIG_ENABLE_WIFI
#include <platform/internal/GenericConnectivityManagerImpl_WiFi.h>
#else
#include <platform/internal/GenericConnectivityManagerImpl_NoWiFi.h>
#endif
#include <platform/internal/GenericConnectivityManagerImpl_UDP.h>
#if INET_CONFIG_ENABLE_TCP_ENDPOINT
#include <platform/internal/GenericConnectivityManagerImpl_TCP.h>
#endif

namespace chip {
    namespace Inet {
        class IPAddress;
    }  // namespace Inet
}  // namespace chip

namespace chip {
    namespace DeviceLayer {

        class ConnectivityManagerImpl final
            : public ConnectivityManager,
              public Internal::GenericConnectivityManagerImpl<ConnectivityManagerImpl>,
              public Internal::GenericConnectivityManagerImpl_UDP<ConnectivityManagerImpl>,
#if INET_CONFIG_ENABLE_TCP_ENDPOINT
              public Internal::GenericConnectivityManagerImpl_TCP<ConnectivityManagerImpl>,
#endif
#if CHIP_DEVICE_CONFIG_ENABLE_WIFI
              public Internal::GenericConnectivityManagerImpl_WiFi<ConnectivityManagerImpl>,
#else
              public Internal::GenericConnectivityManagerImpl_NoWiFi<ConnectivityManagerImpl>,
#endif
              public Internal::GenericConnectivityManagerImpl_NoBLE<ConnectivityManagerImpl>,
              public Internal::GenericConnectivityManagerImpl_NoThread<ConnectivityManagerImpl> {
            // Allow the ConnectivityManager interface class to delegate method calls to
            // the implementation methods provided by this class.
            friend class ConnectivityManager;

        private:
            // ===== Members that implement the ConnectivityManager abstract interface.
            CHIP_ERROR _Init(void);
            void _OnPlatformEvent(const ChipDeviceEvent *event);

#if CHIP_DEVICE_CONFIG_ENABLE_WIFI
            using Flags = GenericConnectivityManagerImpl_WiFi::ConnectivityFlags;
            // ===== Members that implement the ConnectivityManager abstract interface.

            WiFiStationMode _GetWiFiStationMode(void);
            CHIP_ERROR _SetWiFiStationMode(WiFiStationMode val);
            bool _IsWiFiStationEnabled(void);
            bool _IsWiFiStationApplicationControlled(void);
            bool _IsWiFiStationConnected(void);
            System::Clock::Timeout _GetWiFiStationReconnectInterval(void);
            CHIP_ERROR _SetWiFiStationReconnectInterval(System::Clock::Timeout val);
            bool _IsWiFiStationProvisioned(void);
            void _ClearWiFiStationProvision(void);
            bool _CanStartWiFiScan();
            void _OnWiFiScanDone();
            void _OnWiFiStationProvisionChange();

            // ===== Private members reserved for use by this class only.

            System::Clock::Timestamp mLastStationConnectFailTime;
            WiFiStationMode mWiFiStationMode;
            WiFiStationState mWiFiStationState;
            System::Clock::Timeout mWiFiStationReconnectInterval;
            BitFlags<Flags> mFlags;

            CHIP_ERROR InitWiFi(void);
            void OnWiFiPlatformEvent(const ChipDeviceEvent *event);

            void DriveStationState(void);
            void OnStationConnected(void);
            void OnStationDisconnected(void);
            void ChangeWiFiStationState(WiFiStationState newState);
            static void DriveStationState(::chip::System::Layer *aLayer, void *aAppState);

            void UpdateInternetConnectivityState(void);
            void OnIPv6AddressAvailable();
#endif  // CHIP_DEVICE_CONFIG_ENABLE_WIFI

            // ===== Members for internal use by the following friends.
            friend ConnectivityManager &ConnectivityMgr(void);
            friend ConnectivityManagerImpl &ConnectivityMgrImpl(void);

            static ConnectivityManagerImpl sInstance;
        };

#if CHIP_DEVICE_CONFIG_ENABLE_WIFI
        inline bool ConnectivityManagerImpl::_IsWiFiStationApplicationControlled(void)
        {
            return mWiFiStationMode == kWiFiStationMode_ApplicationControlled;
        }

        inline bool ConnectivityManagerImpl::_IsWiFiStationConnected(void)
        {
            return mWiFiStationState == kWiFiStationState_Connected;
        }

        inline System::Clock::Timeout ConnectivityManagerImpl::_GetWiFiStationReconnectInterval(void)
        {
            return mWiFiStationReconnectInterval;
        }

        inline bool ConnectivityManagerImpl::_CanStartWiFiScan()
        {
            return mWiFiStationState != kWiFiStationState_Connecting;
        }

#endif  // CHIP_DEVICE_CONFIG_ENABLE_WIFI

        /**
         * Returns the public interface of the ConnectivityManager singleton object.
         *
         * chip applications should use this to access features of the ConnectivityManager object
         * that are common to all platforms.
         */
        inline ConnectivityManager &ConnectivityMgr(void)
        {
            return ConnectivityManagerImpl::sInstance;
        }

        /**
         * Returns the platform-specific implementation of the ConnectivityManager singleton object.
         *
         * chip applications can use this to gain access to features of the ConnectivityManager
         * that are specific to the ESP32 platform.
         */
        inline ConnectivityManagerImpl &ConnectivityMgrImpl(void)
        {
            return ConnectivityManagerImpl::sInstance;
        }

    }  // namespace DeviceLayer
}  // namespace chip
