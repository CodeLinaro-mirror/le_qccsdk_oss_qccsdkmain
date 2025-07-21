/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once
#include <platform/NetworkCommissioning.h>

namespace chip {
    namespace DeviceLayer {
        namespace NetworkCommissioning {
            namespace {
                constexpr uint8_t kMaxWiFiNetworks = 1;
                constexpr uint8_t kWiFiScanNetworksTimeOutSeconds = 10;
                constexpr uint8_t kWiFiConnectNetworkTimeoutSeconds = 30;
            }  // namespace

            class FermionWiFiDriver final : public WiFiDriver {
            public:
                class WiFiNetworkIterator final : public NetworkIterator {
                public:
                    WiFiNetworkIterator(FermionWiFiDriver *aDriver) : mDriver(aDriver) {}
                    size_t Count() override;
                    bool Next(Network &item) override;
                    void Release() override
                    {
                        delete this;
                    }
                    ~WiFiNetworkIterator() = default;

                private:
                    FermionWiFiDriver *mDriver;
                    bool mExhausted = false;
                };

                struct WiFiNetwork {
                    char ssid[DeviceLayer::Internal::kMaxWiFiSSIDLength];
                    uint8_t ssidLen = 0;
                    char credentials[DeviceLayer::Internal::kMaxWiFiKeyLength];
                    uint8_t credentialsLen = 0;
                };

                // BaseDriver
                NetworkIterator *GetNetworks() override
                {
                    return new WiFiNetworkIterator(this);
                }
                CHIP_ERROR Init(NetworkStatusChangeCallback *networkStatusChangeCallback) override;
                void Shutdown() override;

                // WirelessDriver
                uint8_t GetMaxNetworks() override
                {
                    return kMaxWiFiNetworks;
                }
                uint8_t GetScanNetworkTimeoutSeconds() override
                {
                    return kWiFiScanNetworksTimeOutSeconds;
                }
                uint8_t GetConnectNetworkTimeoutSeconds() override
                {
                    return kWiFiConnectNetworkTimeoutSeconds;
                }

                CHIP_ERROR CommitConfiguration() override;
                CHIP_ERROR RevertConfiguration() override;

                Status RemoveNetwork(ByteSpan networkId, MutableCharSpan &outDebugText,
                                     uint8_t &outNetworkIndex) override;
                Status ReorderNetwork(ByteSpan networkId, uint8_t index, MutableCharSpan &outDebugText) override;
                void ConnectNetwork(ByteSpan networkId, ConnectCallback *callback) override;

                // WiFiDriver
                Status AddOrUpdateNetwork();
                Status AddOrUpdateNetwork(ByteSpan ssid, ByteSpan credentials, MutableCharSpan &outDebugText,
                                          uint8_t &outNetworkIndex) override;
                void ScanNetworks(ByteSpan ssid, ScanCallback *callback) override;

                CHIP_ERROR ConnectWiFiNetwork(const char *ssid, uint8_t ssidLen, const char *key, uint8_t keyLen);
                void OnConnectWiFiNetwork();
                void OnScanWiFiNetworkDone();
                void OnNetworkStatusChange();

                CHIP_ERROR SetLastDisconnectReason(const ChipDeviceEvent *event);
                int32_t GetLastDisconnectReason();

                static FermionWiFiDriver &GetInstance()
                {
                    static FermionWiFiDriver instance;
                    return instance;
                }

            private:
                bool NetworkMatch(const WiFiNetwork &network, ByteSpan networkId);
                CHIP_ERROR StartScanWiFiNetworks(ByteSpan ssid);

                WiFiNetworkIterator mWiFiIterator = WiFiNetworkIterator(this);
                WiFiNetwork mSavedNetwork;
                WiFiNetwork mStagingNetwork;
                ScanCallback *mpScanCallback;
                ConnectCallback *mpConnectCallback;
                NetworkStatusChangeCallback *mpStatusChangeCallback = nullptr;
                int32_t mLastDisconnectedReason;
            };

        }  // namespace NetworkCommissioning
    }      // namespace DeviceLayer
}  // namespace chip
