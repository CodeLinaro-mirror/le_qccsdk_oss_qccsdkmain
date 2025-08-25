/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once

#include "platform/internal/DeviceNetworkInfo.h"
#include <platform/internal/CHIPDeviceLayerInternal.h>

namespace chip {
    namespace DeviceLayer {
        namespace Internal {

            class FermionUtils {
            public:
                static CHIP_ERROR EnableWiFi();
                static CHIP_ERROR SetPersistentStationProvision(char *ssid, char *password);
                static CHIP_ERROR GetPersistentStationProvision();
                static bool IsStationProvisioned();
                static void ClearStationProvision();
                static CHIP_ERROR IsStationConnected(bool &connected);
                static CHIP_ERROR EnableStationMode();
                static CHIP_ERROR DisconnectFromNetwork();
                static CHIP_ERROR ConnectNetwork();
                static CHIP_ERROR ConnectNetwork(const char *ssid, uint8_t ssidLen, const char *passphrase,
                                                 uint8_t passphraseLen);
            };

        }  // namespace Internal
    }      // namespace DeviceLayer
}  // namespace chip
