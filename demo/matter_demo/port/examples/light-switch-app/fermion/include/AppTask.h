/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "AppEvent.h"

#include "FreeRTOS.h"
#include "queue.h"
#include "timers.h"

//#include <ble/BLEEndPoint.h>
//#include <platform/CHIPDeviceLayer.h>
#include <lib/core/CHIPError.h>

class AppTask {
public:
    CHIP_ERROR StartAppTask();
    static void AppTaskMain(void *pvParameter);

    void PostEvent(const AppEvent *event);

private:
    friend AppTask &GetAppTask(void);

    CHIP_ERROR Init();

    void DispatchEvent(AppEvent *event);

    static AppTask sAppTask;
};

inline AppTask &GetAppTask(void)
{
    return AppTask::sAppTask;
}
