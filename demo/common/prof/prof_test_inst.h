/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include "prof_demo_internal.h"
#include "nt_osal.h"
#include "uart.h"
#include <string.h>
#include "timer.h"

typedef struct {
    uint32_t busy_ratio;
    nt_osal_task_handle_t prof_test_task;
} busy_test_inst;

prof_cmd_status prof_test_cfg(uint32_t param_cnt, prof_param *param_list);
prof_cmd_status prof_test_stop();
prof_cmd_status prof_test_start();
prof_cmd_status prof_test_dump();
