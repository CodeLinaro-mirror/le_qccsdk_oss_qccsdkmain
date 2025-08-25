/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/* this file behaves like a config.h, comes first */
#include <platform/internal/CHIPDeviceLayerInternal.h>

#include <platform/CommissionableDataProvider.h>
#include <platform/ConnectivityManager.h>

#include <lib/support/CodeUtils.h>
#include <lib/support/logging/CHIPLogging.h>
#include <platform/DiagnosticDataProvider.h>
#include <platform/fermion/NetworkCommissioningDriver.h>
#include <platform/fermion/FermionUtils.h>
#include <app/server/Dnssd.h>

extern "C" {
#include "qapi/qapi_wlan.h"
}

#if CHIP_DEVICE_CONFIG_ENABLE_WIFI

using namespace ::chip;
using namespace ::chip::Inet;
using namespace ::chip::System;
using namespace ::chip::TLV;
using namespace ::chip::DeviceLayer::DeviceEventType;

namespace chip {
    namespace DeviceLayer {

        ConnectivityManager::WiFiStationMode ConnectivityManagerImpl::_GetWiFiStationMode(void)
        {
            if (mWiFiStationMode != kWiFiStationMode_ApplicationControlled) {
                qapi_WLAN_DEV_Mode_e mode = DEV_MODE_STATION_E;
                uint32_t deviceId = 1, dataLen = sizeof(qapi_WLAN_DEV_Mode_e);
                if (0 == qapi_WLAN_Get_Param(deviceId, __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                                             __QAPI_WLAN_PARAM_GROUP_WIRELESS_OPERATION_MODE, &mode, &dataLen)) {
                    if (mode == DEV_MODE_STATION_E)
                        mWiFiStationMode = kWiFiStationMode_Enabled;
                    else
                        mWiFiStationMode = kWiFiStationMode_Disabled;
                }

                else
                    mWiFiStationMode = kWiFiStationMode_Disabled;
            }
            return mWiFiStationMode;
        }

        bool ConnectivityManagerImpl::_IsWiFiStationEnabled(void)
        {
            return GetWiFiStationMode() == kWiFiStationMode_Enabled;
        }

        CHIP_ERROR ConnectivityManagerImpl::_SetWiFiStationMode(WiFiStationMode val)
        {
            DeviceLayer::SystemLayer().ScheduleWork(DriveStationState, NULL);

            if (mWiFiStationMode != val) {
                ChipLogProgress(DeviceLayer, "WiFi station mode change: %s -> %s",
                                WiFiStationModeToStr(mWiFiStationMode), WiFiStationModeToStr(val));
            }
            mWiFiStationMode = val;

            return CHIP_NO_ERROR;
        }

        bool ConnectivityManagerImpl::_IsWiFiStationProvisioned(void)
        {
            return Internal::FermionUtils::IsStationProvisioned();
        }

        void ConnectivityManagerImpl::_ClearWiFiStationProvision(void)
        {
            if (mWiFiStationMode != kWiFiStationMode_ApplicationControlled) {
                Internal::FermionUtils::ClearStationProvision();

                DeviceLayer::SystemLayer().ScheduleWork(DriveStationState, NULL);
            }
        }

        CHIP_ERROR ConnectivityManagerImpl::InitWiFi()
        {
            mLastStationConnectFailTime = System::Clock::kZero;
            mWiFiStationMode = kWiFiStationMode_Disabled;
            mWiFiStationState = kWiFiStationState_NotConnected;
            mWiFiStationReconnectInterval =
                System::Clock::Milliseconds32(CHIP_DEVICE_CONFIG_WIFI_STATION_RECONNECT_INTERVAL);
            mFlags.SetRaw(0);
            bool connected;

            ReturnErrorOnFailure(Internal::FermionUtils::EnableWiFi());

            if (IsWiFiStationProvisioned()) {
                NetworkCommissioning::FermionWiFiDriver::GetInstance().AddOrUpdateNetwork();
            } else {
                // Get persistent station provision
                ReturnErrorOnFailure(Internal::FermionUtils::GetPersistentStationProvision());
            }

            // If there is no persistent station provision...
            if (!IsWiFiStationProvisioned()) {
                return CHIP_NO_ERROR;
            } else {
                ReturnErrorOnFailure(Internal::FermionUtils::IsStationConnected(connected));
                if (connected) {
                    mWiFiStationMode = kWiFiStationMode_Enabled;
                    return CHIP_NO_ERROR;
                } else {
                    if (CHIP_NO_ERROR != Internal::FermionUtils::ConnectNetwork()) {
                        ChipLogError(DeviceLayer, "Connect to network failed");
                        ReturnErrorOnFailure(SetWiFiStationMode(kWiFiStationMode_Disabled));
                    } else
                        ReturnErrorOnFailure(SetWiFiStationMode(kWiFiStationMode_Enabled));
                }
            }

            // Queue work items to bootstrap the AP and station state machines once the Chip event loop is running.
            ReturnErrorOnFailure(DeviceLayer::SystemLayer().ScheduleWork(DriveStationState, NULL));

            return CHIP_NO_ERROR;
        }

        void ConnectivityManagerImpl::OnWiFiPlatformEvent(const ChipDeviceEvent *event)
        {
            // Handle Fermion system events...
            if (event->Type == DeviceEventType::kFermionSystemEvent) {
                switch (event->Platform.FermionSystemEvent.Id) {
                    case QAPI_WLAN_SCAN_COMPLETE:
                        ChipLogProgress(DeviceLayer, "WIFI_EVENT_SCAN_DONE");
                        NetworkCommissioning::FermionWiFiDriver::GetInstance().OnScanWiFiNetworkDone();
                        break;
                    case QAPI_WLAN_CONNECT:
                        ChipLogProgress(DeviceLayer, "WIFI_EVENT_STA_CONNECTED");
                        if (mWiFiStationState == kWiFiStationState_Connecting) {
                            ChangeWiFiStationState(kWiFiStationState_Connecting_Succeeded);
                        }
                        DriveStationState();
                        break;
                    case QAPI_WLAN_DISCONNECT:
                        ChipLogProgress(DeviceLayer, "WIFI_EVENT_STA_DISCONNECTED");
                        if (mWiFiStationState == kWiFiStationState_Connecting) {
                            ChangeWiFiStationState(kWiFiStationState_Connecting_Failed);
                        }
                        DriveStationState();
                        break;
                    default:
                        break;
                }
            }
        }

        void ConnectivityManagerImpl::_OnWiFiScanDone()
        {
            // Schedule a call to DriveStationState method in case a station connect attempt was
            // deferred because the scan was in progress.
            DeviceLayer::SystemLayer().ScheduleWork(DriveStationState, NULL);
        }

        void ConnectivityManagerImpl::_OnWiFiStationProvisionChange()
        {
            // Schedule a call to the DriveStationState method to adjust the station state as needed.
            DeviceLayer::SystemLayer().ScheduleWork(DriveStationState, NULL);
        }

        void ConnectivityManagerImpl::DriveStationState()
        {
            bool stationConnected;

            // Refresh the current station mode.
            GetWiFiStationMode();

            // If the station interface is NOT under application control...
            if (mWiFiStationMode != kWiFiStationMode_ApplicationControlled) {
                // Ensure that station mode is enabled in the Fermion WiFi layer.
                ReturnOnFailure(Internal::FermionUtils::EnableStationMode());
            }

            // Determine if the Fermion WiFi layer thinks the station interface is currently connected.
            ReturnOnFailure(Internal::FermionUtils::IsStationConnected(stationConnected));

            // If the station interface is currently connected ...
            if (stationConnected) {
                // Advance the station state to Connected if it was previously NotConnected or
                // a previously initiated connect attempt succeeded.
                if (mWiFiStationState == kWiFiStationState_NotConnected ||
                    mWiFiStationState == kWiFiStationState_Connecting_Succeeded) {
                    ChangeWiFiStationState(kWiFiStationState_Connected);
                    ChipLogProgress(DeviceLayer, "WiFi station interface connected");
                    mLastStationConnectFailTime = System::Clock::kZero;
                    OnStationConnected();
                }

                // If the WiFi station interface is no longer enabled, or no longer provisioned,
                // disconnect the station from the AP, unless the WiFi station mode is currently
                // under application control.
                if (mWiFiStationMode != kWiFiStationMode_ApplicationControlled &&
                    (mWiFiStationMode != kWiFiStationMode_Enabled)) {
                    ChipLogProgress(DeviceLayer, "Disconnecting WiFi station interface");
                    if (CHIP_NO_ERROR != Internal::FermionUtils::DisconnectFromNetwork()) {
                        ChipLogError(DeviceLayer, "Disconnect from network failed");
                        return;
                    }

                    ChangeWiFiStationState(kWiFiStationState_Disconnecting);
                }
            }

            // Otherwise the station interface is NOT connected to an AP, so...
            else {
                System::Clock::Timestamp now = System::SystemClock().GetMonotonicTimestamp();

                // Advance the station state to NotConnected if it was previously Connected or Disconnecting,
                // or if a previous initiated connect attempt failed.
                if (mWiFiStationState == kWiFiStationState_Connected ||
                    mWiFiStationState == kWiFiStationState_Disconnecting ||
                    mWiFiStationState == kWiFiStationState_Connecting_Failed) {
                    WiFiStationState prevState = mWiFiStationState;
                    ChangeWiFiStationState(kWiFiStationState_NotConnected);
                    if (prevState != kWiFiStationState_Connecting_Failed) {
                        ChipLogProgress(DeviceLayer, "WiFi station interface disconnected");
                        mLastStationConnectFailTime = System::Clock::kZero;
                        OnStationDisconnected();
                    } else {
                        mLastStationConnectFailTime = now;
                    }
                }

                // If the WiFi station interface is now enabled and provisioned (and by implication,
                // not presently under application control), AND the system is not in the process of
                // scanning, then...
                if (mWiFiStationMode == kWiFiStationMode_Enabled && IsWiFiStationProvisioned() &&
                    mWiFiStationState != kWiFiStationState_Connecting) {
                    // Initiate a connection to the AP if we haven't done so before, or if enough
                    // time has passed since the last attempt.
                    if (mLastStationConnectFailTime == System::Clock::kZero ||
                        now >= mLastStationConnectFailTime + mWiFiStationReconnectInterval) {
                        ChipLogProgress(DeviceLayer, "Attempting to connect WiFi station interface");
                        if (CHIP_NO_ERROR != Internal::FermionUtils::ConnectNetwork()) {
                            ChipLogError(DeviceLayer, "Connect to network failed");
                            return;
                        }
                        ChangeWiFiStationState(kWiFiStationState_Connecting);
                    }

                    // Otherwise arrange another connection attempt at a suitable point in the future.
                    else {
                        System::Clock::Timeout timeToNextConnect =
                            (mLastStationConnectFailTime + mWiFiStationReconnectInterval) - now;

                        ChipLogProgress(DeviceLayer, "Next WiFi station reconnect in %" PRIu32 " ms",
                                        System::Clock::Milliseconds32(timeToNextConnect).count());

                        ReturnOnFailure(
                            DeviceLayer::SystemLayer().StartTimer(timeToNextConnect, DriveStationState, NULL));
                    }
                }
            }

            ChipLogProgress(DeviceLayer, "Done driving station state, nothing else to do...");
            // Kick-off any pending network scan that might have been deferred due to the activity
            // of the WiFi station.
        }

        void ConnectivityManagerImpl::OnStationConnected()
        {
            NetworkCommissioning::FermionWiFiDriver::GetInstance().OnConnectWiFiNetwork();
            // TODO Invoke WARM to perform actions that occur when the WiFi station interface comes up.

            // Alert other components of the new state.
            ChipDeviceEvent event;
            event.Type = DeviceEventType::kWiFiConnectivityChange;
            event.WiFiConnectivityChange.Result = kConnectivity_Established;
            PlatformMgr().PostEventOrDie(&event);

            UpdateInternetConnectivityState();
        }

        void ConnectivityManagerImpl::OnStationDisconnected()
        {
            // TODO Invoke WARM to perform actions that occur when the WiFi station interface goes down.

            // Alert other components of the new state.
            ChipDeviceEvent event;
            event.Type = DeviceEventType::kWiFiConnectivityChange;
            event.WiFiConnectivityChange.Result = kConnectivity_Lost;
            PlatformMgr().PostEventOrDie(&event);

            UpdateInternetConnectivityState();
        }

        void ConnectivityManagerImpl::ChangeWiFiStationState(WiFiStationState newState)
        {
            if (mWiFiStationState != newState) {
                ChipLogProgress(DeviceLayer, "WiFi station state change: %s -> %s",
                                WiFiStationStateToStr(mWiFiStationState), WiFiStationStateToStr(newState));
                mWiFiStationState = newState;
                SystemLayer().ScheduleLambda(
                    []() { NetworkCommissioning::FermionWiFiDriver::GetInstance().OnNetworkStatusChange(); });
            }
        }

        void ConnectivityManagerImpl::DriveStationState(::chip::System::Layer *aLayer, void *aAppState)
        {
            sInstance.DriveStationState();
        }

        void ConnectivityManagerImpl::UpdateInternetConnectivityState(void)
        {
            ChipDeviceEvent event;

            if (mWiFiStationState == kWiFiStationState_Connected) {
                mFlags.Set(ConnectivityFlags::kHaveIPv6InternetConnectivity, true);
                event.Type = DeviceEventType::kInternetConnectivityChange;
                event.InternetConnectivityChange.IPv6 = kConnectivity_Established;
                // TODO:Set data to new address
                PlatformMgr().PostEventOrDie(&event);

                chip::app::DnssdServer::Instance().StartServer();
            }

            if (mWiFiStationState == kWiFiStationState_NotConnected) {
                mFlags.Set(ConnectivityFlags::kHaveIPv6InternetConnectivity, false);
                event.Type = DeviceEventType::kInternetConnectivityChange;
                event.InternetConnectivityChange.IPv6 = kConnectivity_Lost;
                PlatformMgr().PostEventOrDie(&event);
            }
        }

        void ConnectivityManagerImpl::OnIPv6AddressAvailable()
        {
            ChipDeviceEvent event;
            event.Type = DeviceEventType::kInterfaceIpAddressChanged;
            event.InterfaceIpAddressChanged.Type = InterfaceIpChangeType::kIpV6_Assigned;
            PlatformMgr().PostEventOrDie(&event);
        }

    }  // namespace DeviceLayer
}  // namespace chip

#endif  // CHIP_DEVICE_CONFIG_ENABLE_WIFI
