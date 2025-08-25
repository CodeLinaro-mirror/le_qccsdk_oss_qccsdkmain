/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "binding-handler.h"

#include "app/CommandSender.h"
#include "app/clusters/bindings/BindingManager.h"
#include "app/server/Server.h"
#include "controller/InvokeInteraction.h"
#include "platform/CHIPDeviceLayer.h"
#include "app/clusters/bindings/bindings.h"
#include <lib/support/CodeUtils.h>

using namespace chip;
using namespace chip::app;

namespace {

    void ProcessOnOffUnicastBindingCommand(CommandId commandId, const EmberBindingTableEntry &binding,
                                           DeviceProxy *peer_device)
    {
        auto onSuccess = [](const ConcreteCommandPath &commandPath, const StatusIB &status, const auto &dataResponse) {
            ChipLogProgress(NotSpecified, "OnOff command succeeds");
        };

        auto onFailure = [](CHIP_ERROR error) {
            ChipLogError(NotSpecified, "OnOff command failed: %" CHIP_ERROR_FORMAT, error.Format());
        };

        switch (commandId) {
            case Clusters::OnOff::Commands::Toggle::Id:
                Clusters::OnOff::Commands::Toggle::Type toggleCommand;
                Controller::InvokeCommandRequest(peer_device->GetExchangeManager(),
                                                 peer_device->GetSecureSession().Value(), binding.remote, toggleCommand,
                                                 onSuccess, onFailure);
                break;

            case Clusters::OnOff::Commands::On::Id:
                Clusters::OnOff::Commands::On::Type onCommand;
                Controller::InvokeCommandRequest(peer_device->GetExchangeManager(),
                                                 peer_device->GetSecureSession().Value(), binding.remote, onCommand,
                                                 onSuccess, onFailure);
                break;

            case Clusters::OnOff::Commands::Off::Id:
                Clusters::OnOff::Commands::Off::Type offCommand;
                Controller::InvokeCommandRequest(peer_device->GetExchangeManager(),
                                                 peer_device->GetSecureSession().Value(), binding.remote, offCommand,
                                                 onSuccess, onFailure);
                break;
        }
    }

    void ProcessOnOffGroupBindingCommand(CommandId commandId, const EmberBindingTableEntry &binding)
    {
        Messaging::ExchangeManager &exchangeMgr = Server::GetInstance().GetExchangeManager();

        switch (commandId) {
            case Clusters::OnOff::Commands::Toggle::Id:
                Clusters::OnOff::Commands::Toggle::Type toggleCommand;
                Controller::InvokeGroupCommandRequest(&exchangeMgr, binding.fabricIndex, binding.groupId,
                                                      toggleCommand);
                break;

            case Clusters::OnOff::Commands::On::Id:
                Clusters::OnOff::Commands::On::Type onCommand;
                Controller::InvokeGroupCommandRequest(&exchangeMgr, binding.fabricIndex, binding.groupId, onCommand);

                break;

            case Clusters::OnOff::Commands::Off::Id:
                Clusters::OnOff::Commands::Off::Type offCommand;
                Controller::InvokeGroupCommandRequest(&exchangeMgr, binding.fabricIndex, binding.groupId, offCommand);
                break;
        }
    }

    void LightSwitchChangedHandler(const EmberBindingTableEntry &binding, OperationalDeviceProxy *peer_device,
                                   void *context)
    {
        VerifyOrReturn(context != nullptr, ChipLogError(NotSpecified, "OnDeviceConnectedFn: context is null"));
        BindingCommandData *data = static_cast<BindingCommandData *>(context);

        if (binding.type == MATTER_MULTICAST_BINDING && data->isGroup) {
            switch (data->clusterId) {
                case Clusters::OnOff::Id:
                    ProcessOnOffGroupBindingCommand(data->commandId, binding);
                    break;
            }
        } else if (binding.type == MATTER_UNICAST_BINDING && !data->isGroup) {
            switch (data->clusterId) {
                case Clusters::OnOff::Id:
                    ProcessOnOffUnicastBindingCommand(data->commandId, binding, peer_device);
                    break;
            }
        }
    }

    void LightSwitchContextReleaseHandler(void *context)
    {
        VerifyOrReturn(context != nullptr,
                       ChipLogError(NotSpecified, "LightSwitchContextReleaseHandler: context is null"));

        Platform::Delete(static_cast<BindingCommandData *>(context));
    }

    void InitBindingHandlerInternal(intptr_t arg)
    {
        auto &server = chip::Server::GetInstance();
        chip::BindingManager::GetInstance().Init(
            {&server.GetFabricTable(), server.GetCASESessionManager(), &server.GetPersistentStorage()});
        chip::BindingManager::GetInstance().RegisterBoundDeviceChangedHandler(LightSwitchChangedHandler);
        chip::BindingManager::GetInstance().RegisterBoundDeviceContextReleaseHandler(LightSwitchContextReleaseHandler);
    }

}  // namespace

void SwitchWorkerFunction(intptr_t context)
{
    VerifyOrReturn(context != 0, ChipLogError(NotSpecified, "SwitchWorkerFunction - Invalid work data"));

    BindingCommandData *data = reinterpret_cast<BindingCommandData *>(context);
    BindingManager::GetInstance().NotifyBoundClusterChanged(data->localEndpointId, data->clusterId,
                                                            static_cast<void *>(data));
}

void BindingWorkerFunction(intptr_t context)
{
    VerifyOrReturn(context != 0, ChipLogError(NotSpecified, "BindingWorkerFunction - Invalid work data"));

    EmberBindingTableEntry *entry = reinterpret_cast<EmberBindingTableEntry *>(context);
    AddBindingEntry(*entry);

    Platform::Delete(entry);
}

CHIP_ERROR InitBindingHandler()
{
    // The initialization of binding manager will try establishing connection with unicast peers
    // so it requires the Server instance to be correctly initialized. Post the init function to
    // the event queue so that everything is ready when initialization is conducted.
    chip::DeviceLayer::PlatformMgr().ScheduleWork(InitBindingHandlerInternal);
    return CHIP_NO_ERROR;
}
