/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 *    @file
 *          Provides an implementation of the ConfigurationManager object
 *          for the Fermion platform.
 */

#pragma once

#include <platform/CHIPDeviceConfig.h>
#include <platform/CHIPDeviceError.h>

#include <platform/internal/GenericConfigurationManagerImpl.h>
#include <platform/fermion/FermionConfig.h>

namespace chip {
    namespace DeviceLayer {

        /**
         * Concrete implementation of the ConfigurationManager singleton object for the fake platform.
         */
        class ConfigurationManagerImpl : public Internal::GenericConfigurationManagerImpl<Internal::FermionConfig> {
        public:
            static ConfigurationManagerImpl &GetDefaultInstance();

        private:
            CHIP_ERROR Init() override;
            CHIP_ERROR GetPrimaryWiFiMACAddress(uint8_t *buf) override;
#if CHIP_CONFIG_TEST
            CHIP_ERROR RunUnitTests(void) override
            {
                return CHIP_ERROR_NOT_IMPLEMENTED;
            }
#endif
            bool CanFactoryReset() override
            {
                return true;
            }
            void InitiateFactoryReset() override {}
            CHIP_ERROR ReadPersistedStorageValue(::chip::Platform::PersistedStorage::Key key, uint32_t &value) override
            {
                return CHIP_DEVICE_ERROR_CONFIG_NOT_FOUND;
            }
            CHIP_ERROR WritePersistedStorageValue(::chip::Platform::PersistedStorage::Key key, uint32_t value) override
            {
                return CHIP_ERROR_NOT_IMPLEMENTED;
            }

            CHIP_ERROR ReadConfigValue(Key key, bool &val) override
            {
                return CHIP_DEVICE_ERROR_CONFIG_NOT_FOUND;
            }
            CHIP_ERROR ReadConfigValue(Key key, uint32_t &val) override;

            CHIP_ERROR ReadConfigValue(Key key, uint64_t &val) override
            {
                return CHIP_DEVICE_ERROR_CONFIG_NOT_FOUND;
            }
            CHIP_ERROR ReadConfigValueStr(Key key, char *buf, size_t bufSize, size_t &outLen) override;

            CHIP_ERROR ReadConfigValueBin(Key key, uint8_t *buf, size_t bufSize, size_t &outLen) override;

            CHIP_ERROR WriteConfigValue(Key key, bool val) override
            {
                return CHIP_NO_ERROR;
            }

            CHIP_ERROR WriteConfigValue(Key key, uint32_t val) override;

            CHIP_ERROR WriteConfigValue(Key key, uint64_t val) override
            {
                return CHIP_NO_ERROR;
            }
            CHIP_ERROR WriteConfigValueStr(Key key, const char *str) override;

            CHIP_ERROR WriteConfigValueStr(Key key, const char *str, size_t strLen) override;

            CHIP_ERROR WriteConfigValueBin(Key key, const uint8_t *data, size_t dataLen) override;

            void RunConfigUnitTest(void) override {}
        };
        ConfigurationManager &ConfigurationMgrImpl();
    }  // namespace DeviceLayer
}  // namespace chip
