/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

// for QCLI demo

/*-------------------------------------------------------------------------
 * Include Files
 *-----------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "AppConfig.h"
#include "AppTask.h"

// CHIP includes
#include "binding-handler.h"

#include <lib/support/CHIPMem.h>
#include <lib/support/CHIPPlatformMemory.h>
#include <lib/support/logging/CHIPLogging.h>
#include <platform/CHIPDeviceLayer.h>
#include <platform/CommissionableDataProvider.h>
#include <platform/fermion/FermionUtils.h>
#include <app/server/Server.h>
#include <app/server/OnboardingCodesUtil.h>
#include <app/clusters/bindings/BindingManager.h>
#include <credentials/examples/DeviceAttestationCredsExample.h>

#include <inet/UDPEndPointImpl.h>
#include <system/SystemPacketBuffer.h>

// QR Code
#include <bitset>
#include <setup_payload/Base38Decode.h>
#include <setup_payload/Base38Encode.h>
#include <setup_payload/QRCodeSetupPayloadGenerator.h>
#include <setup_payload/QRCodeSetupPayloadParser.h>
#include <setup_payload/SetupPayload.h>
#include <string>

using namespace ::chip;
using namespace ::chip::app;
using namespace ::chip::Credentials;
using namespace ::chip::Inet;
using namespace ::chip::System;
using namespace ::chip::DeviceLayer;
using namespace ::chip::DeviceLayer::Internal;

/*-------------------------------------------------------------------------
 * Function Definitions
 *-----------------------------------------------------------------------*/

/**
   @brief This function represents the main thread of execution.
*/

/**
   @brief This function is used to pre initialize resource
          required before starting the demo this is called
          from app_init function in pal.c
 */

typedef void *QAPI_Console_Group_Handle_t;
extern "C" {
void QCLI_Printf(QAPI_Console_Group_Handle_t Group_Handle, const char *format, ...);
}
extern QAPI_Console_Group_Handle_t qcli_matter_group;

uint8_t matterRunning = 0;

CHIP_ERROR Application_Init(void)
{
    return GetAppTask().StartAppTask();
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

    // ChipLogProgress(NotSpecified, "Init CHIP Stack");
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

exit:
    return ret;
}

void MatterPostAttributeChangeCallback(const chip::app::ConcreteAttributePath &attributePath, uint8_t type,
                                       uint16_t size, uint8_t *value)
{
    QCLI_Printf(qcli_matter_group, "PostAttributeChangeCallback:\n");
    QCLI_Printf(qcli_matter_group, "Cluster ID: '0x%04x', EndPoint ID: '0x%02x', Attribute ID: '0x%04x' \n",
                attributePath.mClusterId, attributePath.mEndpointId, attributePath.mAttributeId);
    QCLI_Printf(qcli_matter_group, "size:%d\n", size);
    QCLI_Printf(qcli_matter_group, "value:");
    for (int i = 0; i < size; i++) {
        QCLI_Printf(qcli_matter_group, "'0x%02x',", *value);
        value++;
    }
    QCLI_Printf(qcli_matter_group, "\n", size);
}

extern "C" {

void OnSwitchCommandHandler()
{
    BindingCommandData *data = Platform::New<BindingCommandData>();
    data->commandId = Clusters::OnOff::Commands::On::Id;
    data->clusterId = Clusters::OnOff::Id;

    DeviceLayer::PlatformMgr().ScheduleWork(SwitchWorkerFunction, reinterpret_cast<intptr_t>(data));
}

void OffSwitchCommandHandler()
{
    BindingCommandData *data = Platform::New<BindingCommandData>();
    data->commandId = Clusters::OnOff::Commands::Off::Id;
    data->clusterId = Clusters::OnOff::Id;

    DeviceLayer::PlatformMgr().ScheduleWork(SwitchWorkerFunction, reinterpret_cast<intptr_t>(data));
}

void ToggleSwitchCommandHandler()
{
    BindingCommandData *data = Platform::New<BindingCommandData>();
    data->commandId = Clusters::OnOff::Commands::Toggle::Id;
    data->clusterId = Clusters::OnOff::Id;

    DeviceLayer::PlatformMgr().ScheduleWork(SwitchWorkerFunction, reinterpret_cast<intptr_t>(data));
}

void GroupOnSwitchCommandHandler()
{
    BindingCommandData *data = Platform::New<BindingCommandData>();
    data->commandId = Clusters::OnOff::Commands::On::Id;
    data->clusterId = Clusters::OnOff::Id;
    data->isGroup = true;

    DeviceLayer::PlatformMgr().ScheduleWork(SwitchWorkerFunction, reinterpret_cast<intptr_t>(data));
}

void GroupOffSwitchCommandHandler()
{
    BindingCommandData *data = Platform::New<BindingCommandData>();
    data->commandId = Clusters::OnOff::Commands::Off::Id;
    data->clusterId = Clusters::OnOff::Id;
    data->isGroup = true;

    DeviceLayer::PlatformMgr().ScheduleWork(SwitchWorkerFunction, reinterpret_cast<intptr_t>(data));
}

void GroupToggleSwitchCommandHandler()
{
    BindingCommandData *data = Platform::New<BindingCommandData>();
    data->commandId = Clusters::OnOff::Commands::Toggle::Id;
    data->clusterId = Clusters::OnOff::Id;
    data->isGroup = true;

    DeviceLayer::PlatformMgr().ScheduleWork(SwitchWorkerFunction, reinterpret_cast<intptr_t>(data));
}

void BindingGroupBindCommandHandler(uint8_t fabrixIndex, uint16_t groupId)
{
    EmberBindingTableEntry *entry = Platform::New<EmberBindingTableEntry>();
    entry->type = MATTER_MULTICAST_BINDING;
    entry->fabricIndex = fabrixIndex;
    entry->groupId = groupId;
    entry->local = 1;              // Hardcoded to endpoint 1 for now
    entry->clusterId.SetValue(6);  // Hardcoded to OnOff cluster for now

    DeviceLayer::PlatformMgr().ScheduleWork(BindingWorkerFunction, reinterpret_cast<intptr_t>(entry));
}

void BindingUnicastBindCommandHandler(uint8_t fabrixIndex, uint16_t groupId, uint16_t remote)
{
    EmberBindingTableEntry *entry = Platform::New<EmberBindingTableEntry>();
    entry->type = MATTER_UNICAST_BINDING;
    entry->fabricIndex = fabrixIndex;
    entry->nodeId = groupId;
    entry->local = 1;  // Hardcoded to endpoint 1 for now
    entry->remote = remote;
    entry->clusterId.SetValue(6);  // Hardcode to OnOff cluster for now

    DeviceLayer::PlatformMgr().ScheduleWork(BindingWorkerFunction, reinterpret_cast<intptr_t>(entry));
}

void GetOnboardingCodes()
{
    PrintOnboardingCodes(chip::RendezvousInformationFlags(chip::RendezvousInformationFlag::kBLE,
                                                          chip::RendezvousInformationFlag::kOnNetwork));
}

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

    /* Initialize CHIP stack */
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
