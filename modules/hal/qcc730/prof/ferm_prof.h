/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef CORE_SYSTEM_INC_FERM_PROF_H_
#define CORE_SYSTEM_INC_FERM_PROF_H_
#include "ferm_qtmr.h"
#include "nt_osal.h"

// QTMR FRAME
#define PROF_TIMER                 QTMR_FRAME_1
#define PROF_ICSR                  0xE000ED04
#define PROF_EX_MASK               0x1FF
#define PROF_IRQ_NUM               109
#define PROF_IRQ_BASE              16
#define PROF_IRQ_ACTIVE_NUM(value) ((value & PROF_EX_MASK) - PROF_IRQ_BASE)

#define PROF_RUN_TIME(enter, exit) ((exit > enter) ? (exit - enter) : 0)

#define PROF_BIT(n)   (1 << n)
#define PROF_DUMP_IRQ PROF_BIT(0)
#define PROF_DUMP_OS  PROF_BIT(1)

#define PROF_OS_MAX_SNAP 25

#define PROF_TIMER_TASK_PRIORITY   (configMAX_PRIORITIES - 1)
#define PROF_TIMER_TASK_STACK_SIZE 400

typedef enum {
    PROF_STOP_EVENT_FROM_ISR,
} prof_timer_event_type;

typedef enum {
    PROF_NO_SPEC_MODE,
    PROF_SPEC_TIME_MODE,
} PROF_TIMER_MODE;

typedef enum {
    PROF_DEINIT,
    PROF_INIT,
    PROF_START,
    PROF_STOP,
} prof_state;

typedef enum {
    PROF_SUCCESS,
    PROF_ERROR_INVALID_PARAM,
    PROF_ERROR_TIMER,
    PROF_ERROR_STATE,
} prof_status;

typedef struct {
    uint64_t irq_enter;
    uint64_t irq_exit;
    uint64_t irq_run;
    uint32_t irq_cnt;
    uint32_t irq_num;
} prof_irq_stats;

typedef struct {
    uint32_t irqs_0;
    uint32_t irqs_1;
    uint32_t irqs_2;
    uint32_t irqs_3;
    uint32_t irqs_num;
} prof_irq_cfg;

typedef struct {
    prof_irq_stats *stats;
    prof_irq_cfg cfg;
} prof_irq;

typedef struct {
    uint32_t max_num;
    uint32_t snap_num;
    TaskStatus_t *status_snap;
} task_snap;

typedef struct {
    task_snap prof_start;
    task_snap prof_stop;
    TaskHandle_t curr_tcb;
    uint64_t *total_run;
    uint64_t *switch_in;
} prof_os;

typedef struct {
    prof_irq_stats *irq_stats;
    TaskStatus_t *task_status;
} prof_stats;

typedef volatile struct {
    qtmr_frame_instance instance;
    uint32_t start;
    uint32_t mode;
    uint64_t duration;
    uint64_t run_tick;
    uint64_t cval;
    uint64_t start_tick;
    uint64_t end_tick;
    nt_osal_task_handle_t prof_timer_task;
} prof_cfg;

typedef struct {
    char *buf;
    uint32_t cfg;
} prof_dump;

typedef struct {
    prof_cfg cfg;
    prof_irq irq;
    prof_os os;
    prof_state state;
    prof_dump dump;
} prof_dev;

prof_status prof_timer_init();
prof_status prof_timer_deinit();
uint64_t prof_timer_get_count();
prof_status prof_timer_config(PROF_TIMER_MODE mode, uint64_t duration);
prof_status prof_timer_start();
prof_status prof_timer_stop();
void prof_timer_dump(uint32_t dump_cfg);
prof_irq_stats *prof_timer_irq_stats();

#ifdef PROF_DRV_OS_REMOVE_IRQ
void TaskRunCounterUpdate(uint64_t RunTime);
void ulTaskSwitchedInTimeUpdate(uint64_t SwitchInTime);
#endif

#ifdef PROF_DRV
void prof_irq_enter();
void prof_irq_exit();
void prof_irq_enter_num();
void prof_irq_exit_num();

#define PROF_IRQ_ENTER() prof_irq_enter();
#define PROF_IRQ_EXIT()  prof_irq_exit();
#else
#define PROF_IRQ_ENTER()
#define PROF_IRQ_EXIT()
#endif

#endif
