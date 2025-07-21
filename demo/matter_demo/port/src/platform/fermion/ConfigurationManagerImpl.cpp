/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 *    @file
 *          Provides the implementation of the Device Layer ConfigurationManager object
 *          for the Fermion.
 */

#include <platform/ConfigurationManager.h>
#include <platform/fermion/ConfigurationManagerImpl.h>
#include <platform/internal/GenericConfigurationManagerImpl.ipp>
extern "C" {
#include <qapi/wlan/qapi_wlan_param_group.h>
}

namespace chip {
    namespace DeviceLayer {

        using namespace ::chip::DeviceLayer::Internal;

        CHIP_ERROR ConfigurationManagerImpl::Init()
        {
            CHIP_ERROR err;
            // Initialize the generic implementation base class.
            err = Internal::GenericConfigurationManagerImpl<FermionConfig>::Init();
            SuccessOrExit(err);

        exit:
            return err;
        }

        CHIP_ERROR ConfigurationManagerImpl::ReadConfigValue(Key key, uint32_t &val)
        {
            return FermionConfig::ReadConfigValue(key, val);
        }

        CHIP_ERROR ConfigurationManagerImpl::ReadConfigValueStr(Key key, char *buf, size_t bufSize, size_t &outLen)
        {
            return FermionConfig::ReadConfigValueStr(key, buf, bufSize, outLen);
        }

        CHIP_ERROR ConfigurationManagerImpl::ReadConfigValueBin(Key key, uint8_t *buf, size_t bufSize, size_t &outLen)
        {
            return FermionConfig::ReadConfigValueBin(key, buf, bufSize, outLen);
        }

        CHIP_ERROR ConfigurationManagerImpl::WriteConfigValue(Key key, uint32_t val)
        {
            return FermionConfig::WriteConfigValue(key, val);
        }

        CHIP_ERROR ConfigurationManagerImpl::WriteConfigValueStr(Key key, const char *str)
        {
            return FermionConfig::WriteConfigValueStr(key, str);
        }

        CHIP_ERROR ConfigurationManagerImpl::WriteConfigValueStr(Key key, const char *str, size_t strLen)
        {
            return FermionConfig::WriteConfigValueStr(key, str, strLen);
        }

        CHIP_ERROR ConfigurationManagerImpl::WriteConfigValueBin(Key key, const uint8_t *data, size_t dataLen)
        {
            return FermionConfig::WriteConfigValueBin(key, data, dataLen);
        }

#if CHIP_DEVICE_CONFIG_ENABLE_WIFI
        CHIP_ERROR ConfigurationManagerImpl::GetPrimaryWiFiMACAddress(uint8_t *buf)
        {
            uint32_t len = 6;
            if (0 != qapi_WLAN_Get_Param(1, __QAPI_WLAN_PARAM_GROUP_WIRELESS,
                                         __QAPI_WLAN_PARAM_GROUP_WIRELESS_MAC_ADDRESS, buf, &len)) {
                return CHIP_ERROR_UNSUPPORTED_CHIP_FEATURE;
            } else {
                return CHIP_NO_ERROR;
            }
        }

#else
        /* Function GetPrimaryMACAddress() in GenericConfigurationManagerImpl.ipp calls
         * GetPrimaryWiFiMACAddress() even in OpenThread mode. Simply let it return for
         * Openthread mode.*/
        CHIP_ERROR ConfigurationManagerImpl::GetPrimaryWiFiMACAddress(uint8_t *buf)
        {
            return CHIP_DEVICE_ERROR_CONFIG_NOT_FOUND;
        }
#endif  // CHIP_DEVICE_CONFIG_ENABLE_WIFI

        ConfigurationManagerImpl &ConfigurationManagerImpl::GetDefaultInstance()
        {
            static ConfigurationManagerImpl sInstance;
            return sInstance;
        }

        ConfigurationManager &ConfigurationMgrImpl()
        {
            return ConfigurationManagerImpl::GetDefaultInstance();
        }

    }  // namespace DeviceLayer
}  // namespace chip
