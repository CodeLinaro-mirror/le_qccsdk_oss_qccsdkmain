/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <string.h>

void app_init(void)
{
}

extern void ambient_power_demo_main();

void app_main(void)
{
    ambient_power_demo_main();
}