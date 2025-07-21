/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 *    @file
 *          General utility methods for the QCA platform.
 */
/* this file behaves like a config.h, comes first */
#include <platform/internal/CHIPDeviceLayerInternal.h>

#include <lib/support/CodeUtils.h>
#include <lib/core/ErrorStr.h>
#include <lib/support/logging/CHIPLogging.h>
#include <platform/fermion/ConnectivityManagerImpl.h>
#include <platform/fermion/FermionUtils.h>

extern "C" {
#include "qapi/qapi_wlan.h"
qbool_t get_device_connect_state(void);
}

// using namespace ::chip::DeviceLayer::NetworkCommissioning;
using namespace ::chip::DeviceLayer::Internal;
using namespace ::chip::DeviceLayer;
using namespace ::chip::DeviceLayer::DeviceEventType;
using chip::DeviceLayer::Internal::DeviceNetworkInfo;

extern const char kWiFiSSIDKeyName[];
extern const char kWiFiCredentialsKeyName[];
const char kWiFiSSIDKeyName[] = "wifi-ssid";
const char kWiFiCredentialsKeyName[] = "wifi-pass";

char wifi_ssid[kMaxWiFiSSIDLength];
char wifi_credentials[kMaxWiFiKeyLength];

typedef struct matter_wifi_cxt_s {
    qapi_WLAN_Auth_Mode_e auth;
    qbool_t connected;
    uint8_t active_device;
    uint8_t wlan_enabled;
} matter_wifi_cxt_t;

#define MATTER_DEVICE 1
int active_device = MATTER_DEVICE;
matter_wifi_cxt_t g_matter_wifi_cxt = {
    .auth = QAPI_WLAN_AUTH_NONE_E, .connected = false, .active_device = 0, .wlan_enabled = 0};

static void WlanCallback(uint8_t deviceId, uint32_t cbId, void *pApplicationContext, void *payload,
                         uint32_t payload_Length)
{
    ChipDeviceEvent event;
    matter_wifi_cxt_t *p_cxt = &g_matter_wifi_cxt;

    memset(&event, 0, sizeof(event));
    event.Type = DeviceEventType::kFermionSystemEvent;
    event.Platform.FermionSystemEvent.Id = QAPI_WLAN_UNKNOWN_ID;

    switch (cbId) {
        case QAPI_WLAN_CONNECT_CB_E: {
            qapi_WLAN_Join_Comp_Evt_t *cxnInfo = (qapi_WLAN_Join_Comp_Evt_t *)(payload);
            if (cxnInfo->evt_hdr.status == QAPI_OK) {
                p_cxt->connected = true;
                event.Platform.FermionSystemEvent.Id = QAPI_WLAN_CONNECT;
            } else {
                p_cxt->connected = false;
                event.Platform.FermionSystemEvent.Id = QAPI_WLAN_DISCONNECT;
            }
        } break;
        case QAPI_WLAN_SCAN_COMPLETE_CB_E:
            // TODO
            break;
        default:
            break;
    }

    PlatformMgr().PostEventOrDie(&event);
    return;
}

CHIP_ERROR FermionUtils::EnableWiFi()
{
    qapi_Status_t ret;
    qapi_WLAN_DEV_Mode_e devMode = DEV_MODE_STATION_E;
    matter_wifi_cxt_t *p_cxt = &g_matter_wifi_cxt;

    if (p_cxt->wlan_enabled) {
        return CHIP_NO_ERROR;
    }

    qapi_WLAN_Set_Callback(WlanCallback, p_cxt);

    ret = qapi_WLAN_Enable(QAPI_WLAN_ENABLE_E);
    if (QAPI_OK != ret) {
        return CHIP_ERROR_INTERNAL;
    }

    p_cxt->wlan_enabled = 1;

    ret = qapi_WLAN_Set_Param(0, __QAPI_WLAN_PARAM_GROUP_WIRELESS, __QAPI_WLAN_PARAM_GROUP_WIRELESS_OPERATION_MODE,
                              &devMode, sizeof(devMode), QAPI_WLAN_NO_WAIT_E);

    if (ret == QAPI_OK) {
        p_cxt->active_device = MATTER_DEVICE;
        return CHIP_NO_ERROR;
    } else
        return CHIP_ERROR_INTERNAL;
}

CHIP_ERROR FermionUtils::SetPersistentStationProvision(char *ssid, char *password)
{
    if ((strlen(ssid) >= kMaxWiFiSSIDLength) || (strlen(password) >= kMaxWiFiKeyLength))
        return CHIP_ERROR_MESSAGE_TOO_LONG;

    strlcpy(wifi_ssid, ssid, kMaxWiFiSSIDLength);
    if (strlen(password) > 0)
        strlcpy(wifi_credentials, password, kMaxWiFiKeyLength);

    return CHIP_NO_ERROR;
}

CHIP_ERROR FermionUtils::GetPersistentStationProvision()
{
    CHIP_ERROR err;

    err = PersistedStorage::KeyValueStoreMgr().Get(kWiFiCredentialsKeyName, wifi_credentials, kMaxWiFiKeyLength, 0);
    if (err == CHIP_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND) {
        return CHIP_NO_ERROR;
    }
    if (err != CHIP_NO_ERROR) {
        ClearStationProvision();
        return CHIP_ERROR_INTERNAL;
    }

    err = PersistedStorage::KeyValueStoreMgr().Get(kWiFiSSIDKeyName, wifi_ssid, kMaxWiFiSSIDLength, 0);
    if (err == CHIP_ERROR_PERSISTED_STORAGE_VALUE_NOT_FOUND) {
        return CHIP_NO_ERROR;
    }
    if (err != CHIP_NO_ERROR) {
        ClearStationProvision();
        return CHIP_ERROR_INTERNAL;
    }

    return CHIP_NO_ERROR;
}

bool FermionUtils::IsStationProvisioned()
{
    return (wifi_ssid[0] != 0);
}

void FermionUtils::ClearStationProvision()
{
    memset(wifi_ssid, 0, kMaxWiFiSSIDLength);
    memset(wifi_credentials, 0, kMaxWiFiKeyLength);
}

CHIP_ERROR FermionUtils::IsStationConnected(bool &connected)
{
    matter_wifi_cxt_t *p_cxt = &g_matter_wifi_cxt;
    connected = p_cxt->connected;
    return CHIP_NO_ERROR;
}

CHIP_ERROR FermionUtils::EnableStationMode()
{
    return CHIP_NO_ERROR;
}

CHIP_ERROR FermionUtils::DisconnectFromNetwork()
{
    qapi_Status_t ret = QAPI_OK;
    matter_wifi_cxt_t *p_cxt = &g_matter_wifi_cxt;
    uint8_t deviceId = p_cxt->active_device;

    if (!p_cxt->wlan_enabled) {
        return CHIP_ERROR_INTERNAL;
    }
    p_cxt->auth = QAPI_WLAN_AUTH_NONE_E;
    ret = qapi_WLAN_Disconnect(deviceId);
    if (ret == QAPI_OK)
        return CHIP_NO_ERROR;
    else
        return CHIP_ERROR_INTERNAL;
}

CHIP_ERROR FermionUtils::ConnectNetwork()
{
    return ConnectNetwork(wifi_ssid, strlen(wifi_ssid), wifi_credentials, strlen(wifi_credentials));
}

CHIP_ERROR FermionUtils::ConnectNetwork(const char *ssid, uint8_t ssidLen, const char *passphrase,
                                        uint8_t passphraseLen)
{
    qapi_Status_t ret = QAPI_OK;
    matter_wifi_cxt_t *p_cxt = &g_matter_wifi_cxt;
    qapi_WLAN_Auth_Mode_e wpa_ver;
    qapi_WLAN_Crypt_Type_e e_cipher;
    uint8_t deviceId = p_cxt->active_device;

    if (!p_cxt->wlan_enabled) {
        return CHIP_ERROR_INTERNAL;
    }

    if (passphraseLen > 63)
        return CHIP_ERROR_INTERNAL;

    if ((passphraseLen < 8) && (passphraseLen != 0))
        return CHIP_ERROR_INTERNAL;

    if (passphraseLen == 0) {
        wpa_ver = QAPI_WLAN_AUTH_NONE_E;
        e_cipher = QAPI_WLAN_CRYPT_NONE_E;
    } else {
        wpa_ver = QAPI_WLAN_AUTH_WPA2_PSK_E;
        e_cipher = QAPI_WLAN_CRYPT_AES_CRYPT_E;
    }
    qapi_WLAN_Set_Param(0, __QAPI_WLAN_PARAM_GROUP_WIRELESS, __QAPI_WLAN_PARAM_GROUP_WIRELESS_SSID, (void *)ssid,
                        ssidLen, QAPI_WLAN_NO_WAIT_E);
    qapi_WLAN_Set_Param(deviceId, __QAPI_WLAN_PARAM_GROUP_WIRELESS_SECURITY, __QAPI_WLAN_PARAM_GROUP_SECURITY_AUTH_MODE,
                        (void *)&wpa_ver, sizeof(qapi_WLAN_Auth_Mode_e), QAPI_WLAN_NO_WAIT_E);
    qapi_WLAN_Set_Param(deviceId, __QAPI_WLAN_PARAM_GROUP_WIRELESS_SECURITY,
                        __QAPI_WLAN_PARAM_GROUP_SECURITY_ENCRYPTION_TYPE, (void *)&e_cipher,
                        sizeof(qapi_WLAN_Crypt_Type_e), QAPI_WLAN_NO_WAIT_E);
    qapi_WLAN_Set_Param(deviceId, __QAPI_WLAN_PARAM_GROUP_WIRELESS_SECURITY,
                        __QAPI_WLAN_PARAM_GROUP_SECURITY_PASSPHRASE, (void *)passphrase, passphraseLen,
                        QAPI_WLAN_NO_WAIT_E);

    p_cxt->auth = wpa_ver;
    ret = qapi_WLAN_Commit(deviceId);
    if (ret == QAPI_OK)
        return CHIP_NO_ERROR;
    else
        return CHIP_ERROR_INTERNAL;
}

qbool_t get_device_connect_state(void)
{
    return g_matter_wifi_cxt.connected;
}
