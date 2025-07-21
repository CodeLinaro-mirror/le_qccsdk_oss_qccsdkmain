/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <stdio.h>
#include <string.h>

void app_init(void) {}

extern void mqtt_demo_main();

void app_main(void)
{
    mqtt_demo_main();
}
