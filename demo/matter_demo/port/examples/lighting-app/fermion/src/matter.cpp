/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*-------------------------------------------------------------------------
 * Include Files
 *-----------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* CHIP includes */
#include <lib/support/CHIPMem.h>
#include <lib/support/CHIPPlatformMemory.h>
#include <lib/support/logging/CHIPLogging.h>
#include <platform/CHIPDeviceLayer.h>
#include <platform/CommissionableDataProvider.h>
#include <app/server/Server.h>
#include <app/server/OnboardingCodesUtil.h>
//#include <app/clusters/bindings/BindingManager.h>
#include <app/clusters/network-commissioning/network-commissioning.h>
#include <credentials/examples/DeviceAttestationCredsExample.h>
#include <platform/fermion/FermionUtils.h>
#include <platform/fermion/NetworkCommissioningDriver.h>
#include <inet/UDPEndPointImpl.h>
#include <system/SystemPacketBuffer.h>
#include <DeviceInfoProviderImpl.h>

/* QR Code */
#include <bitset>
#include <setup_payload/Base38Decode.h>
#include <setup_payload/Base38Encode.h>
#include <setup_payload/QRCodeSetupPayloadGenerator.h>
#include <setup_payload/QRCodeSetupPayloadParser.h>
#include <setup_payload/SetupPayload.h>
#include <string>

using namespace ::chip;
using namespace ::chip::Credentials;
using namespace ::chip::Inet;
using namespace ::chip::System;
using namespace ::chip::DeviceLayer;
using namespace ::chip::DeviceLayer::Internal;

typedef void *QAPI_Console_Group_Handle_t;
extern "C" {
void QCLI_Printf(QAPI_Console_Group_Handle_t Group_Handle, const char *format, ...);
}
extern QAPI_Console_Group_Handle_t qcli_matter_group;
static chip::DeviceLayer::DeviceInfoProviderImpl gExampleDeviceInfoProvider;

/*-------------------------------------------------------------------------
 * Function Definitions
 *-----------------------------------------------------------------------*/
uint8_t matterRunning = 0;

#if CHIP_DEVICE_CONFIG_ENABLE_WIFI
chip::app::Clusters::NetworkCommissioning::Instance sWiFiNetworkCommissioningInstance(
    0, &(chip::DeviceLayer::NetworkCommissioning::FermionWiFiDriver ::GetInstance()));
#endif
CHIP_ERROR Application_Init(void)
{
    static chip::CommonCaseDeviceServerInitParams initParams;
    (void)initParams.InitializeStaticResourcesBeforeServerInit();

    SetDeviceAttestationCredentialsProvider(Examples::GetExampleDACProvider());
    gExampleDeviceInfoProvider.SetStorageDelegate(initParams.persistentStorageDelegate);
    chip::DeviceLayer::SetDeviceInfoProvider(&gExampleDeviceInfoProvider);

    chip::Server::GetInstance().Init(initParams);

    // ConfigurationMgr().LogDeviceConfig();

#if CHIP_DEVICE_CONFIG_ENABLE_WIFI
    sWiFiNetworkCommissioningInstance.Init();
#endif

    PrintOnboardingCodes(chip::RendezvousInformationFlags(chip::RendezvousInformationFlag::kOnNetwork));

    return CHIP_NO_ERROR;
}

CHIP_ERROR CHIP_Init(void)
{
    CHIP_ERROR ret = chip::Platform::MemoryInit();
    if (ret != CHIP_NO_ERROR) {
        QCLI_Printf(qcli_matter_group, "Memory Init failed.\n");
        goto exit;
    } else {
        QCLI_Printf(qcli_matter_group, "Memory Init succeed.\n");
    }

    ret = PlatformMgr().InitChipStack();
    if (ret != CHIP_NO_ERROR) {
        QCLI_Printf(qcli_matter_group, "CHIP Stack Init failed.\n");
        goto exit;
    } else {
        QCLI_Printf(qcli_matter_group, "CHIP Stack Inited.\n");
    }

    QCLI_Printf(qcli_matter_group, "Starting Platform Manager Event Loop.\n");
    ret = PlatformMgr().StartEventLoopTask();
    if (ret != CHIP_NO_ERROR) {
        QCLI_Printf(qcli_matter_group, "Event Loop start failed.\n");
        goto exit;
    } else {
        QCLI_Printf(qcli_matter_group, "Event Loop started.\n");
    }

    ConfigurationMgr().LogDeviceConfig();

exit:
    return ret;
}

extern "C" {

void Matter_FactoryReset()
{
    FermionConfig::FactoryResetConfig();

    return;
}

void Matter_Onboarding(char *ssid, char *password)
{
    FermionUtils::SetPersistentStationProvision(ssid, password);

    return;
}

void Matter_Enable()
{
    CHIP_ERROR error;

    if (matterRunning) {
        QCLI_Printf(qcli_matter_group, "CHIP already running.\n");
        return;
    }

    error = CHIP_Init();
    if (error != CHIP_NO_ERROR) {
        QCLI_Printf(qcli_matter_group, "CHIP init failed.\n");
        return;
    } else {
        QCLI_Printf(qcli_matter_group, "CHIP init succeed.\n");
    }

    /* Application task */

    error = Application_Init();
    if (error != CHIP_NO_ERROR) {
        QCLI_Printf(qcli_matter_group, "Application init failed.\n");
        return;
    } else {
        QCLI_Printf(qcli_matter_group, "Application init succeed.\n");
    }

    QCLI_Printf(qcli_matter_group, "CHIP stack init succeed.\n");
    matterRunning = 1;
    return;
}
}
