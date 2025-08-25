/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include "nt_osal.h"
#include "uart.h"
#include <string.h>
#include "timer.h"
#include "prof_test_inst.h"

#ifdef PROF_TEST_INST

static char PROFTestOutputBuffer[100];
#define PROF_TEST_PRINTF(...)                                                  \
    snprintf(PROFTestOutputBuffer, sizeof(PROFTestOutputBuffer), __VA_ARGS__); \
    nt_dbg_print(PROFTestOutputBuffer);

#define PROF_TEST_TASK_PRIORITY   (configMAX_PRIORITIES - 1)
#define PROF_TEST_TASK_STACK_SIZE 400
static busy_test_inst prof_busy_inst;

static void prof_test_task(void __attribute__((__unused__)) * pvParameters)
{
    uint32_t busy_ratio, delay_ratio;

    busy_ratio = prof_busy_inst.busy_ratio;
    delay_ratio = 100 - busy_ratio;

    for (;;) {
        hres_timer_ms_delay(busy_ratio);
        vTaskDelay(delay_ratio);
    }
}

prof_cmd_status prof_test_cfg(uint32_t param_cnt, prof_param *param_list)
{
    uint32_t cfg_ratio;

    if (param_list[0].Integer_Value <= 1)
        cfg_ratio = 1;
    else if (param_list[0].Integer_Value >= 99)
        cfg_ratio = 99;
    else
        cfg_ratio = param_list[0].Integer_Value;

    prof_busy_inst.busy_ratio = cfg_ratio;

    PROF_TEST_PRINTF("prof_test_cfg param_cnt = %u, busy_ratio = %u\r\n", (unsigned int)param_cnt,
                     (unsigned int)cfg_ratio);

    return PROF_CMD_SUCCESS;
}

prof_cmd_status prof_test_stop()
{
    nt_osal_thread_delete(prof_busy_inst.prof_test_task);
    return PROF_CMD_SUCCESS;
}

prof_cmd_status prof_test_start()
{
    if (pdPASS != nt_qurt_thread_create(prof_test_task, "prof_test_task", PROF_TEST_TASK_STACK_SIZE, NULL,
                                        PROF_TEST_TASK_PRIORITY, &prof_busy_inst.prof_test_task))
        return PROF_CMD_FAILURE;

    return PROF_CMD_SUCCESS;
}

prof_cmd_status prof_test_dump()
{
    return PROF_CMD_SUCCESS;
}

#endif
