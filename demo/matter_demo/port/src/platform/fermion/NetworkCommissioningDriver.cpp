/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <lib/support/CodeUtils.h>
#include <lib/support/SafeInt.h>
#include <platform/CHIPDeviceLayer.h>
#include <platform/fermion/NetworkCommissioningDriver.h>
#include <platform/fermion/FermionUtils.h>
#include <app/server/Dnssd.h>

extern "C" {
#include <qapi/qapi_wlan.h>
}

using namespace ::chip;
using namespace ::chip::DeviceLayer::Internal;

extern const char kWiFiSSIDKeyName[];
extern const char kWiFiCredentialsKeyName[];
extern char wifi_ssid[];
extern char wifi_credentials[];

namespace chip {
    namespace DeviceLayer {
        namespace NetworkCommissioning {

#define MAX_WIFI_CONNECT_SECONDS 10

            CHIP_ERROR FermionWiFiDriver::Init(NetworkStatusChangeCallback *networkStatusChangeCallback)
            {
                CHIP_ERROR err;
                size_t ssidLen = 0;
                size_t credentialsLen = 0;

                err = PersistedStorage::KeyValueStoreMgr().Get(kWiFiCredentialsKeyName, mSavedNetwork.credentials,
                                                               sizeof(mSavedNetwork.credentials), &credentialsLen);
                if (err == CHIP_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND) {
                    return CHIP_NO_ERROR;
                }

                err = PersistedStorage::KeyValueStoreMgr().Get(kWiFiSSIDKeyName, mSavedNetwork.ssid,
                                                               sizeof(mSavedNetwork.ssid), &ssidLen);
                if (err == CHIP_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND) {
                    return CHIP_NO_ERROR;
                }
                mSavedNetwork.credentialsLen = credentialsLen;
                mSavedNetwork.ssidLen = ssidLen;

                mStagingNetwork = mSavedNetwork;
                mpScanCallback = nullptr;
                mpConnectCallback = nullptr;
                mpStatusChangeCallback = networkStatusChangeCallback;
                return err;
            }

            void FermionWiFiDriver::Shutdown()
            {
                mpStatusChangeCallback = nullptr;
            }

            CHIP_ERROR FermionWiFiDriver::CommitConfiguration()
            {
                ReturnErrorOnFailure(PersistedStorage::KeyValueStoreMgr().Put(kWiFiSSIDKeyName, mStagingNetwork.ssid,
                                                                              mStagingNetwork.ssidLen));
                ReturnErrorOnFailure(PersistedStorage::KeyValueStoreMgr().Put(
                    kWiFiCredentialsKeyName, mStagingNetwork.credentials, mStagingNetwork.credentialsLen));
                mSavedNetwork = mStagingNetwork;
                return CHIP_NO_ERROR;
            }

            CHIP_ERROR FermionWiFiDriver::RevertConfiguration()
            {
                mStagingNetwork = mSavedNetwork;
                return CHIP_NO_ERROR;
            }

            bool FermionWiFiDriver::NetworkMatch(const WiFiNetwork &network, ByteSpan networkId)
            {
                return networkId.size() == network.ssidLen &&
                       memcmp(networkId.data(), network.ssid, network.ssidLen) == 0;
            }

            Status FermionWiFiDriver::AddOrUpdateNetwork()
            {
                char debugTextBuffer[1];
                MutableCharSpan outDebugText(debugTextBuffer);
                uint8_t networkIndex = 0;

                return AddOrUpdateNetwork(ByteSpan((uint8_t *)wifi_ssid, strlen(wifi_ssid) + 1),
                                          ByteSpan((uint8_t *)wifi_credentials, strlen(wifi_credentials) + 1),
                                          outDebugText, networkIndex);
            }

            Status FermionWiFiDriver::AddOrUpdateNetwork(ByteSpan ssid, ByteSpan credentials,
                                                         MutableCharSpan &outDebugText, uint8_t &outNetworkIndex)
            {
                outDebugText.reduce_size(0);
                outNetworkIndex = 0;
                VerifyOrReturnError(mStagingNetwork.ssidLen == 0 || NetworkMatch(mStagingNetwork, ssid),
                                    Status::kBoundsExceeded);
                VerifyOrReturnError(credentials.size() <= sizeof(mStagingNetwork.credentials), Status::kOutOfRange);
                VerifyOrReturnError(ssid.size() <= sizeof(mStagingNetwork.ssid), Status::kOutOfRange);

                memcpy(mStagingNetwork.credentials, credentials.data(), credentials.size());
                mStagingNetwork.credentialsLen =
                    static_cast<decltype(mStagingNetwork.credentialsLen)>(credentials.size());

                memcpy(mStagingNetwork.ssid, ssid.data(), ssid.size());
                mStagingNetwork.ssidLen = static_cast<decltype(mStagingNetwork.ssidLen)>(ssid.size());

                return Status::kSuccess;
            }

            Status FermionWiFiDriver::RemoveNetwork(ByteSpan networkId, MutableCharSpan &outDebugText,
                                                    uint8_t &outNetworkIndex)
            {
                outDebugText.reduce_size(0);
                outNetworkIndex = 0;
                VerifyOrReturnError(NetworkMatch(mStagingNetwork, networkId), Status::kNetworkIDNotFound);

                // Use empty ssid for representing invalid network
                mStagingNetwork.ssidLen = 0;
                return Status::kSuccess;
            }

            Status FermionWiFiDriver::ReorderNetwork(ByteSpan networkId, uint8_t index, MutableCharSpan &outDebugText)
            {
                outDebugText.reduce_size(0);

                // Only one network is supported now
                VerifyOrReturnError(index == 0, Status::kOutOfRange);
                VerifyOrReturnError(NetworkMatch(mStagingNetwork, networkId), Status::kNetworkIDNotFound);
                return Status::kSuccess;
            }

            CHIP_ERROR FermionWiFiDriver::ConnectWiFiNetwork(const char *ssid, uint8_t ssidLen, const char *key,
                                                             uint8_t keyLen)
            {
                // If device is already connected to WiFi, disconnect the WiFi,
                // clear the WiFi configurations and add the newly provided WiFi configurations.
                bool isConnected;
                VerifyOrReturnError(FermionUtils::IsStationConnected(isConnected) == CHIP_NO_ERROR,
                                    CHIP_ERROR_INTERNAL);
                if (isConnected) {
                    ChipLogProgress(DeviceLayer, "Disconnecting WiFi station interface");
                    if (CHIP_NO_ERROR != FermionUtils::DisconnectFromNetwork()) {
                        ChipLogError(DeviceLayer, "disconnect from network failed");
                        return CHIP_ERROR_INTERNAL;
                    }
                }

                ReturnErrorOnFailure(
                    ConnectivityMgr().SetWiFiStationMode(ConnectivityManager::kWiFiStationMode_Disabled));

                // Configure the Fermion WiFi interface.
                memset(wifi_ssid, 0, kMaxWiFiSSIDLength);
                memset(wifi_credentials, 0, kMaxWiFiKeyLength);
                VerifyOrReturnError(kMaxWiFiSSIDLength >= ssidLen, CHIP_ERROR_INTERNAL);
                VerifyOrReturnError(kMaxWiFiKeyLength >= keyLen, CHIP_ERROR_INTERNAL);
                memcpy(wifi_ssid, ssid, ssidLen);
                memcpy(wifi_credentials, key, keyLen);

                return ConnectivityMgr().SetWiFiStationMode(ConnectivityManager::kWiFiStationMode_Enabled);
            }

            void FermionWiFiDriver::OnConnectWiFiNetwork()
            {
                if (mpConnectCallback) {
                    mpConnectCallback->OnResult(Status::kSuccess, CharSpan(), 0);
                    mpConnectCallback = nullptr;
                }
            }

            void FermionWiFiDriver::ConnectNetwork(ByteSpan networkId, ConnectCallback *callback)
            {
                CHIP_ERROR err = CHIP_NO_ERROR;
                Status networkingStatus = Status::kSuccess;

                VerifyOrExit(NetworkMatch(mStagingNetwork, networkId), networkingStatus = Status::kNetworkIDNotFound);
                VerifyOrExit(mpConnectCallback == nullptr, networkingStatus = Status::kUnknownError);
                ChipLogProgress(NetworkProvisioning, "Fermion NetworkCommissioningDelegate: SSID: %.*s",
                                static_cast<int>(networkId.size()), networkId.data());

                mpConnectCallback = callback;
                err = ConnectWiFiNetwork(reinterpret_cast<const char *>(mStagingNetwork.ssid), mStagingNetwork.ssidLen,
                                         reinterpret_cast<const char *>(mStagingNetwork.credentials),
                                         mStagingNetwork.credentialsLen);
            exit:
                if (err != CHIP_NO_ERROR) {
                    networkingStatus = Status::kUnknownError;
                }
                if (networkingStatus != Status::kSuccess) {
                    ChipLogError(NetworkProvisioning, "Failed to connect to WiFi network:%s", chip::ErrorStr(err));
                    mpConnectCallback = nullptr;
                    callback->OnResult(networkingStatus, CharSpan(), 0);
                }
            }

            CHIP_ERROR FermionWiFiDriver::StartScanWiFiNetworks(ByteSpan ssid)
            {
                return CHIP_ERROR_NOT_IMPLEMENTED;
            }

            void FermionWiFiDriver::OnScanWiFiNetworkDone()
            {
                // TODO: Do WiFi scan and reponse with scan results

                if (mpScanCallback) {
                    mpScanCallback->OnFinished(Status::kSuccess, CharSpan(), nullptr);
                    mpScanCallback = nullptr;
                }
            }

            CHIP_ERROR GetConfiguredNetwork(Network &network)
            {
                char data[32 + 1] = {'\0'};
                uint32_t dataLen = sizeof(data);

                if (0 != qapi_WLAN_Get_Param(1, __QAPI_WLAN_PARAM_GROUP_WIRELESS, __QAPI_WLAN_PARAM_GROUP_WIRELESS_SSID,
                                             (void *)data, &dataLen)) {
                    return CHIP_ERROR_INTERNAL;
                }

                if (dataLen > sizeof(network.networkID)) {
                    return CHIP_ERROR_INTERNAL;
                }
                memcpy(network.networkID, data, dataLen);
                network.networkIDLen = dataLen;
                return CHIP_NO_ERROR;
            }

            void FermionWiFiDriver::OnNetworkStatusChange() {}

            void FermionWiFiDriver::ScanNetworks(ByteSpan ssid, WiFiDriver::ScanCallback *callback)
            {
                mpScanCallback = callback;

                // TODO: Do WiFi scan and reponse with scan results
                OnScanWiFiNetworkDone();
            }

            CHIP_ERROR FermionWiFiDriver::SetLastDisconnectReason(const ChipDeviceEvent *event)
            {
                return CHIP_ERROR_NOT_IMPLEMENTED;
            }

            int32_t FermionWiFiDriver::GetLastDisconnectReason()
            {
                return -1;
            }

            size_t FermionWiFiDriver::WiFiNetworkIterator::Count()
            {
                return mDriver->mStagingNetwork.ssidLen == 0 ? 0 : 1;
            }

            bool FermionWiFiDriver::WiFiNetworkIterator::Next(Network &item)
            {
                if (mExhausted || mDriver->mStagingNetwork.ssidLen == 0) {
                    return false;
                }
                memcpy(item.networkID, mDriver->mStagingNetwork.ssid, mDriver->mStagingNetwork.ssidLen);
                item.networkIDLen = mDriver->mStagingNetwork.ssidLen;
                item.connected = false;
                mExhausted = true;

                Network configuredNetwork;
                CHIP_ERROR err = GetConfiguredNetwork(configuredNetwork);
                if (err == CHIP_NO_ERROR) {
                    bool isConnected = false;
                    err = FermionUtils::IsStationConnected(isConnected);
                    if (err == CHIP_NO_ERROR && isConnected && configuredNetwork.networkIDLen == item.networkIDLen &&
                        memcmp(configuredNetwork.networkID, item.networkID, item.networkIDLen) == 0) {
                        item.connected = true;
                    }
                }
                return true;
            }

        }  // namespace NetworkCommissioning
    }      // namespace DeviceLayer
}  // namespace chip
