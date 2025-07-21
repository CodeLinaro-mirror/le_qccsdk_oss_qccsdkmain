/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef CORE_SYSTEM_INC_FERM_PROF_DEMO_INTERNAL_H_
#define CORE_SYSTEM_INC_FERM_PROF_DEMO_INTERNAL_H_
#include "ferm_qtmr.h"
#include "ferm_prof.h"

#define PROF_INST_NUM 1
typedef struct {
    char *String_Value;
    int32_t Integer_Value;
    uint32_t Integer_Is_Valid;
} prof_param;

typedef enum {
    PROF_CMD_SUCCESS,
    PROF_CMD_FAILURE,
    PROF_CMD_INVALID,
} prof_cmd_status;

typedef enum {
    PROF_EVENT_START = 1,
    PROF_EVENT_STOP = 2,
} prof_event;

typedef prof_cmd_status (*prof_cfg_handler)(uint32_t param_cnt, prof_param *param_list);
typedef prof_cmd_status (*prof_start_handler)();
typedef prof_cmd_status (*prof_stop_handler)();
typedef prof_cmd_status (*prof_dump_handler)();

typedef struct {
    prof_cfg_handler cfg;
    prof_start_handler start;
    prof_stop_handler stop;
    prof_dump_handler dump;
} prof_inst;

typedef struct {
    uint32_t pinst_num;
    prof_inst *pinst;
    prof_inst *prof;
    uint32_t prof_num;
    uint32_t state;
    nt_osal_task_handle_t prof_task;
} prof_demo;

#endif
