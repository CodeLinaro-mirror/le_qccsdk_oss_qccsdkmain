/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#pragma once

#define APP_TASK_NAME "Switch"

typedef void *QCLI_Group_Handle_t;
extern "C" {
void QCLI_Printf(QCLI_Group_Handle_t Group_Handle, const char *format, ...);
}
extern QCLI_Group_Handle_t qcli_matter_handle;
