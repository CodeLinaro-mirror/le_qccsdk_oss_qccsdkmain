/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "AppConfig.h"
#include "AppTask.h"
#include "binding-handler.h"

#if CHIP_DEVICE_CONFIG_ENABLE_WIFI
#include <platform/fermion/NetworkCommissioningDriver.h>
#endif

#include <app/clusters/bindings/BindingManager.h>
#include <app/clusters/network-commissioning/network-commissioning.h>
#include <app/server/OnboardingCodesUtil.h>
#include <credentials/examples/DeviceAttestationCredsExample.h>
#include <DeviceInfoProviderImpl.h>
#include <setup_payload/SetupPayload.h>

#define APP_TASK_STACK_SIZE  (4096)
#define APP_TASK_PRIORITY    2
#define APP_EVENT_QUEUE_SIZE 10

#define BUTTON_ACTION_ON  1
#define BUTTON_ACTION_OFF 0

using namespace chip;
using namespace ::chip::DeviceLayer;
using namespace ::chip::Credentials;

namespace {
    TaskHandle_t sAppTaskHandle;
    QueueHandle_t sAppEventQueue;
}  // namespace

AppTask AppTask::sAppTask;
typedef void *QAPI_Console_Group_Handle_t;
extern "C" {
void QCLI_Printf(QAPI_Console_Group_Handle_t Group_Handle, const char *format, ...);
}
extern QAPI_Console_Group_Handle_t qcli_matter_group;
#if CHIP_DEVICE_CONFIG_ENABLE_WIFI
chip::app::Clusters::NetworkCommissioning::Instance sWiFiNetworkCommissioningInstance(
    0, &(chip::DeviceLayer::NetworkCommissioning::FermionWiFiDriver ::GetInstance()));
#endif
static chip::DeviceLayer::DeviceInfoProviderImpl gExampleDeviceInfoProvider;

CHIP_ERROR AppTask::StartAppTask()
{
    CHIP_ERROR err = sAppTask.Init();

    if (err != CHIP_NO_ERROR) {
        QCLI_Printf(qcli_matter_group, "AppTask.Init() failed.\n");
        QCLI_Printf(qcli_matter_group, "Critical Error.\n");
        return CHIP_ERROR_INTERNAL;
    }

    sAppEventQueue = xQueueCreate(APP_EVENT_QUEUE_SIZE, sizeof(AppEvent));
    if (sAppEventQueue == NULL) {
        QCLI_Printf(qcli_matter_group, "Failed to allocate app event queue.\n");
        return CHIP_ERROR_INTERNAL;
    }

    // Start App task.
    xTaskCreate(AppTaskMain, APP_TASK_NAME, APP_TASK_STACK_SIZE / sizeof(StackType_t), NULL, 1, &sAppTaskHandle);
    return (sAppTaskHandle == nullptr) ? CHIP_ERROR_INTERNAL : CHIP_NO_ERROR;
}

CHIP_ERROR AppTask::Init()
{
    static chip::CommonCaseDeviceServerInitParams initParams;
    (void)initParams.InitializeStaticResourcesBeforeServerInit();

    chip::DeviceLayer::PlatformMgr().LockChipStack();

    // Initialize device attestation config
    SetDeviceAttestationCredentialsProvider(Examples::GetExampleDACProvider());
    gExampleDeviceInfoProvider.SetStorageDelegate(initParams.persistentStorageDelegate);
    chip::DeviceLayer::SetDeviceInfoProvider(&gExampleDeviceInfoProvider);

    chip::Server::GetInstance().Init(initParams);

    chip::DeviceLayer::PlatformMgr().UnlockChipStack();

    ConfigurationMgr().LogDeviceConfig();
#if CHIP_DEVICE_CONFIG_ENABLE_WIFI
    sWiFiNetworkCommissioningInstance.Init();
#endif

    PrintOnboardingCodes(chip::RendezvousInformationFlags(chip::RendezvousInformationFlag::kOnNetwork));

    if (CHIP_NO_ERROR != InitBindingHandler()) {
        QCLI_Printf(qcli_matter_group, "InitBindingHandler() failed.\n");
        return CHIP_ERROR_INTERNAL;
    }

    QCLI_Printf(qcli_matter_group, "Current Software Version: %s\n", CHIP_DEVICE_CONFIG_DEVICE_SOFTWARE_VERSION_STRING);

    return CHIP_NO_ERROR;
}

void AppTask::AppTaskMain(void *pvParameter)
{
    AppEvent event;

    QCLI_Printf(qcli_matter_group, "App Task started.\n");

    while (true) {
        BaseType_t eventReceived = xQueueReceive(sAppEventQueue, &event, pdMS_TO_TICKS(10));
        while (eventReceived == pdTRUE) {
            sAppTask.DispatchEvent(&event);
            eventReceived = xQueueReceive(sAppEventQueue, &event, 0);
        }
    }
}

void AppTask::DispatchEvent(AppEvent *aEvent)
{
    if (aEvent->Handler) {
        aEvent->Handler(aEvent);
    } else {
        QCLI_Printf(qcli_matter_group, "Event received with no handler. Dropping event.\n");
    }
}
