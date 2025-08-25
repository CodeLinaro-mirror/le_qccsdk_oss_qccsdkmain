/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include <limits.h>
#include <string.h>
#include "ferm_qtmr.h"
#include "nt_osal.h"
#include "uart.h"
#include "nt_common.h"
#include "prof_demo_internal.h"
#include "prof_demo.h"
#ifdef PROF_TEST_INST
#include "prof_test_inst.h"
#endif

#ifdef PROF_DEMO
#define PROF_DEMO_DBG
#ifdef PROF_DEMO_DBG
static char PROFOutputBuffer[100];
#define PROF_PRINTF(...)                                               \
    snprintf(PROFOutputBuffer, sizeof(PROFOutputBuffer), __VA_ARGS__); \
    nt_dbg_print(PROFOutputBuffer);
#else
#define PROF_PRINTF(...)
#endif

#define PROF_DEMO_TASK_PRIORITY   (configMAX_PRIORITIES - 1)
#define PROF_DEMO_TASK_STACK_SIZE 400

#define PROF_STATE_DEINIT 0
#define PROF_STATE_INIT   1
#define PROF_STATE_START  2

#define PROF_CFG_PARAM_NUM_MIN 3

static prof_demo p_dm;

static prof_demo *prof_demo_get()
{
    return &p_dm;
}

static prof_cmd_status prof_def_cfg(uint32_t param_cnt, prof_param *param_list)
{
    if (param_cnt != 0) {
        PROF_PRINTF("prof_def_cfg param[0] = %u\r\n", (unsigned int)param_list[0].Integer_Value);
    }

    return PROF_CMD_SUCCESS;
}

static prof_cmd_status prof_def_stop()
{
    return PROF_CMD_SUCCESS;
}

static prof_cmd_status prof_def_start()
{
    return PROF_CMD_SUCCESS;
}

static prof_cmd_status prof_def_dump()
{
    return PROF_CMD_SUCCESS;
}

const prof_inst default_inst = {prof_def_cfg, prof_def_start, prof_def_stop, prof_def_dump};

#ifdef PROF_TEST_INST
const prof_inst test_inst = {prof_test_cfg, prof_test_start, prof_test_stop, prof_test_dump};
#endif

prof_inst p_inst[] = {
    default_inst,
#ifdef PROF_TEST_INST
    test_inst,
#endif
};

static void prof_event_handler(uint32_t value)
{
    prof_demo *dm;
    dm = prof_demo_get();
    prof_status status = PROF_SUCCESS;

    switch (value) {
        case PROF_EVENT_START:
            status = prof_timer_start();
            break;
        case PROF_EVENT_STOP:
            status = prof_timer_stop();
            if (status == PROF_SUCCESS)
                dm->state = PROF_STATE_INIT;
            break;
        default:
            break;
    }

    PROF_PRINTF("prof_event_handler event:%d, status %d\r\n", (int)value, (int)status);
    return;
}

static void prof_demo_task(void __attribute__((__unused__)) * pvParameters)
{
    BaseType_t xResult;
    uint32_t notified_value = 0;

    for (;;) {
        xResult = xTaskNotifyWait(pdFALSE, ULONG_MAX, &notified_value, portMAX_DELAY);
        if (xResult == pdPASS) {
            prof_event_handler(notified_value);
        }
    }
}

qapi_Status_t prof_demo_init()
{
    prof_demo *dm;
    dm = prof_demo_get();
    prof_status status;

    if (dm->state != PROF_STATE_DEINIT) {
        PROF_PRINTF("prof_demo_init already initilized\r\n");
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_ERROR;
    }

    dm->pinst = p_inst;
    dm->pinst_num = sizeof(p_inst) / sizeof(prof_inst);
    dm->prof = &dm->pinst[0];

    status = prof_timer_init();
    if (status) {
        PROF_PRINTF("prof_demo_init prof_timer_init failure\r\n");
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_ERROR;
    }

    if (pdPASS != nt_qurt_thread_create(prof_demo_task, "prof_demo", PROF_DEMO_TASK_STACK_SIZE, NULL,
                                        PROF_DEMO_TASK_PRIORITY, &dm->prof_task)) {
        PROF_PRINTF("prof_demo_init prof_demo task create failure\r\n");
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_ERROR;
    }

    dm->state = PROF_STATE_INIT;

    return QAPI_OK;
}
qapi_Status_t prof_demo_deinit()
{
    prof_demo *dm;
    dm = prof_demo_get();
    prof_status status;

    if (dm->state != PROF_STATE_INIT) {
        PROF_PRINTF("prof_demo_deinit already de-initilized\r\n");
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_ERROR;
    }

    status = prof_timer_deinit();
    if (status) {
        PROF_PRINTF("prof_demo_init prof timer deinit failure\r\n");
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_ERROR;
    }

    nt_osal_thread_delete(dm->prof_task);

    dm->state = PROF_STATE_DEINIT;

    return QAPI_OK;
}

qapi_Status_t prof_demo_config(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    prof_demo *dm;
    uint32_t prof_num;
    prof_cfg_handler cfg_handler;
    prof_cmd_status cmd_status;
    prof_status status;
    uint32_t prof_usec;
    uint32_t prof_mode;
    uint64_t prof_tick64;

    dm = prof_demo_get();
    if (dm->state != PROF_STATE_INIT) {
        PROF_PRINTF("prof_demo_config profile not be initilized or stoped\r\n");
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_ERROR;
    }

    if (Parameter_Count < PROF_CFG_PARAM_NUM_MIN) {
        PROF_PRINTF("prof_demo_config config param less than minimum number\r\n");
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_ERROR;
    }

    prof_num = Parameter_List[0].Integer_Value;

    if (prof_num >= dm->pinst_num) {
        PROF_PRINTF("prof_demo_config config profiling instance not exit\r\n");
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_ERROR;
    }

    dm->prof = &dm->pinst[prof_num];
    dm->prof_num = prof_num;

    prof_mode = Parameter_List[1].Integer_Value;
    prof_usec = Parameter_List[2].Integer_Value;
    prof_tick64 = qtmr_usec_to_tick((uint64_t)prof_usec);

    cfg_handler = dm->prof->cfg;

    Parameter_Count -= PROF_CFG_PARAM_NUM_MIN;
    cmd_status = cfg_handler(Parameter_Count, (prof_param *)&Parameter_List[3]);

    if (cmd_status)
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_ERROR;

    status = prof_timer_config(prof_mode, prof_tick64);

    if (status)
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_ERROR;

    PROF_PRINTF("Prof demo config inst %d, mode %d, usec %u\r\n", (int)prof_num, (int)prof_mode,
                (unsigned int)prof_usec);

    return QAPI_OK;
}

qapi_Status_t prof_demo_start()
{
    prof_demo *dm;
    prof_start_handler start_handler;
    prof_cmd_status cmd_status;
    prof_status status;

    dm = prof_demo_get();
    if (dm->state != PROF_STATE_INIT) {
        PROF_PRINTF("prof_demo_start profile not be initilized or stopped\r\n");
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_ERROR;
    }

    start_handler = dm->prof->start;

    cmd_status = start_handler();

    if (cmd_status)
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_ERROR;

    status = prof_timer_start();

    if (status) {
        PROF_PRINTF("prof_demo_start prof timer start failed\r\n");
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_ERROR;
    }

    dm->state = PROF_STATE_START;

    return QAPI_OK;
}

qapi_Status_t prof_demo_stop()
{
    prof_demo *dm;
    prof_stop_handler stop_handler;
    prof_cmd_status cmd_status;
    prof_status status;

    dm = prof_demo_get();
    if (dm->state != PROF_STATE_START) {
        PROF_PRINTF("prof_demo_stop profile not be initilized or started\r\n");
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_ERROR;
    }

    stop_handler = dm->prof->stop;

    status = prof_timer_stop();
    if (status) {
        PROF_PRINTF("prof_demo_stop prof timer start failed\r\n");
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_ERROR;
    }

    cmd_status = stop_handler();
    if (cmd_status)
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_ERROR;

    dm->state = PROF_STATE_INIT;

    return QAPI_OK;
}

qapi_Status_t prof_demo_dump()
{
    prof_demo *dm;
    prof_dump_handler dump_handler;
    prof_cmd_status cmd_status;

    dm = prof_demo_get();

    if (dm->state == PROF_STATE_DEINIT) {
        PROF_PRINTF("prof_demo_dump prof not be initialized\r\n");
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_ERROR;
    }

    dump_handler = dm->prof->dump;

    cmd_status = dump_handler();
    if (cmd_status)
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_ERROR;

    prof_timer_dump(PROF_DUMP_IRQ | PROF_DUMP_OS);

    return QAPI_OK;
}

#if (CONFIG_PROF_SHELL)
const QAPI_Console_Command_t prof_shell_cmds[] = {
    // cmd_function			cmd_string       usage_string									description
    {prof_demo_init, "Init", "", "Initialize Profiling"},
    {prof_demo_config, "Config", "<Instance> <Mode> <Usec> <param0>", "Configure profiling instance"},
    {prof_demo_start, "Start", "", "Start profiling"},
    {prof_demo_stop, "Stop", "", "Stop profiling"},
    {prof_demo_dump, "Dump", "", "Dump profiling result"},
    {prof_demo_deinit, "Deinit", "", "Deinitialize profiling"},
};

const QAPI_Console_Command_Group_t prof_shell_cmd_group = {
    "PROF", sizeof(prof_shell_cmds) / sizeof(QAPI_Console_Command_t), prof_shell_cmds};

QAPI_Console_Group_Handle_t prof_shell_cmd_group_handle;

void prof_shell_init(void)
{
    prof_shell_cmd_group_handle = QAPI_Console_Register_Command_Group(NULL, &prof_shell_cmd_group);
}
#endif
#endif  // PROF_DEMO
