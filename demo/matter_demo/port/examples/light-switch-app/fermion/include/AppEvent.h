/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once

struct AppEvent;
typedef void (*EventHandler)(AppEvent *);

struct AppEvent {
    enum AppEventTypes {
        kEventType_Button = 0,
        kEventType_Timer,
    };

    uint16_t Type;

    union {
        struct {
            uint8_t Action;
        } ButtonEvent;
    };

    EventHandler Handler;
};
