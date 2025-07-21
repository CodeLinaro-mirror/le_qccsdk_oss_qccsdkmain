/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 *    @file
 *          Utilities for interacting with the the Fermion "NVS" key-value store.
 */

#pragma once

#include <platform/CHIPDeviceError.h>

namespace chip {
    namespace DeviceLayer {
        namespace Internal {

            class FermionConfig {
            public:
                using Key = const char *;

                // Key definitions for well-known keys.
                static Key kConfigKey_SerialNum;
                static Key kConfigKey_UniqueId;
                static Key kConfigKey_MfrDeviceId;
                static Key kConfigKey_MfrDeviceCert;
                static Key kConfigKey_MfrDeviceICACerts;
                static Key kConfigKey_MfrDevicePrivateKey;
                static Key kConfigKey_HardwareVersion;
                static Key kConfigKey_ManufacturingDate;
                static Key kConfigKey_SetupPinCode;
                static Key kConfigKey_ServiceConfig;
                static Key kConfigKey_PairedAccountId;
                static Key kConfigKey_ServiceId;
                static Key kConfigKey_LastUsedEpochKeyId;
                static Key kConfigKey_FailSafeArmed;
                static Key kConfigKey_WiFiStationSecType;
                static Key kConfigKey_SetupDiscriminator;
                static Key kConfigKey_RegulatoryLocation;
                static Key kConfigKey_CountryCode;
                static Key kConfigKey_Spake2pIterationCount;
                static Key kConfigKey_Spake2pSalt;
                static Key kConfigKey_Spake2pVerifier;
                static Key kConfigKey_DACCert;
                static Key kConfigKey_DACPrivateKey;
                static Key kConfigKey_DACPublicKey;
                static Key kConfigKey_PAICert;
                static Key kConfigKey_CertDeclaration;
                static Key kConfigKey_VendorId;
                static Key kConfigKey_VendorName;
                static Key kConfigKey_ProductId;
                static Key kConfigKey_ProductName;

                // CHIP Counter keys
                static Key kCounterKey_RebootCount;
                static Key kCounterKey_UpTime;
                static Key kCounterKey_TotalOperationalHours;

                static CHIP_ERROR Init();

                static CHIP_ERROR ReadConfigValue(Key key, uint32_t &val);
                static CHIP_ERROR ReadConfigValueBin(Key key, uint8_t *buf, size_t bufSize, size_t &outLen);
                static CHIP_ERROR ReadConfigValueStr(Key key, char *buf, size_t bufSize, size_t &outLen);

                static CHIP_ERROR WriteConfigValue(Key key, uint32_t val);
                static CHIP_ERROR WriteConfigValueBin(Key key, const uint8_t *buf, size_t bufSize);
                static CHIP_ERROR WriteConfigValueStr(Key key, const char *str);
                static CHIP_ERROR WriteConfigValueStr(Key key, const char *str, size_t strLen);

                static CHIP_ERROR FactoryResetConfig(void);
            };

        }  // namespace Internal
    }      // namespace DeviceLayer
}  // namespace chip
