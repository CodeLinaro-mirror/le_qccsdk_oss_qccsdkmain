/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once

#include "app-common/zap-generated/ids/Clusters.h"
#include "app-common/zap-generated/ids/Commands.h"
#include "lib/core/CHIPError.h"

CHIP_ERROR InitBindingHandler();
void SwitchWorkerFunction(intptr_t context);
void BindingWorkerFunction(intptr_t context);

struct BindingCommandData {
    chip::EndpointId localEndpointId = 1;
    chip::CommandId commandId;
    chip::ClusterId clusterId;
    bool isGroup = false;
};
