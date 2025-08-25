/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include <limits.h>
#include <string.h>
#include "ferm_qtmr.h"
#include "qtmr_demo.h"
#include "nt_osal.h"
#include "uart.h"
#include "nt_common.h"
#include "qurt_utils.h"

#define QTMR_DEMO_TASK_PRIORITY   (configMAX_PRIORITIES - 1)
#define QTMR_DEMO_TASK_STACK_SIZE 400

#define QTMR_DEMO_DEINIT 0
#define QTMR_DEMO_INIT   1

/** Timer event flags */
typedef enum {
    QTMR_DEMO_EXPIRY_FROM_ISR,
} qtmr_demo_event_type;

typedef struct {
    qtmr_frame_instance qtmr_instance;
    nt_osal_task_handle_t qtmr_task;
    uint32_t config;
    uint32_t state;
    uint64_t qtmr_sval;
    uint64_t qtmr_cval;
    uint64_t qtmr_tval;
    uint64_t qtmr_cbval;
    uint64_t qtmr_dpval;
    uint32_t mode;
    uint32_t icsr;
} qtmr_demo;

#ifdef QTMR_DEMO
#define QTMR_DEMO_DBG
#ifdef QTMR_DEMO_DBG
static char QTMROutputBuffer[100];
#define QTMR_PRINTF(...)                                               \
    snprintf(QTMROutputBuffer, sizeof(QTMROutputBuffer), __VA_ARGS__); \
    nt_dbg_print(QTMROutputBuffer);
#else
#define QTMR_PRINTF(x, ...)
#endif

#define ICSR_REG 0xE000ED04
static qtmr_demo demo;

static qtmr_demo *qtmr_demo_get()
{
    return &demo;
}

static void qtmr_callback_func(void *param)
{
    BaseType_t wakeup_task = pdFALSE;

    qtmr_demo *demo = (qtmr_demo *)param;

    demo->qtmr_cbval = qtmr_get_frame_count_no_check(demo->qtmr_instance);

    demo->icsr = NT_REG_RD(ICSR_REG);

    if (demo->qtmr_task) {
        /* Signal the timer task of the timer interrupt event */
        xTaskNotifyFromISR((nt_osal_task_handle_t)demo->qtmr_task, (1 << QTMR_DEMO_EXPIRY_FROM_ISR), eSetBits,
                           &wakeup_task);
        /* Wake the priority task if required */
        portYIELD_FROM_ISR(wakeup_task);
    }
}

void qtmr_demo_dump()
{
    qtmr_demo *demo;

    demo = qtmr_demo_get();
    demo->qtmr_dpval = qtmr_get_frame_count_no_check(demo->qtmr_instance);

    QTMR_PRINTF("QTMR DEMO dump:\r\n");
    QTMR_PRINTF("		instance %u\r\n", (unsigned int)demo->qtmr_instance);
    QTMR_PRINTF("		state %u\r\n", (unsigned int)demo->state);
    QTMR_PRINTF("		mode 0x%08x\r\n", (unsigned int)demo->mode);
    QTMR_PRINTF("		icsr 0x%x\r\n", (unsigned int)demo->icsr);
    QTMR_PRINTF("		sval %u:%u\r\n", (unsigned int)QTMR_TICK64_HI_BITS(demo->qtmr_sval),
                (unsigned int)QTMR_TICK64_LO_BITS(demo->qtmr_sval));
    QTMR_PRINTF("		cval %u:%u\r\n", (unsigned int)QTMR_TICK64_HI_BITS(demo->qtmr_cval),
                (unsigned int)QTMR_TICK64_LO_BITS(demo->qtmr_cval));
    QTMR_PRINTF("		tval %u:%u\r\n", (unsigned int)QTMR_TICK64_HI_BITS(demo->qtmr_tval),
                (unsigned int)QTMR_TICK64_LO_BITS(demo->qtmr_tval));
    QTMR_PRINTF("		cbval %u:%u\r\n", (unsigned int)QTMR_TICK64_HI_BITS(demo->qtmr_cbval),
                (unsigned int)QTMR_TICK64_LO_BITS(demo->qtmr_cbval));
    QTMR_PRINTF("		dpval %u:%u\r\n", (unsigned int)QTMR_TICK64_HI_BITS(demo->qtmr_dpval),
                (unsigned int)QTMR_TICK64_LO_BITS(demo->qtmr_dpval));
}

static void qtmr_demo_task(void __attribute__((__unused__)) * pvParameters)
{
    BaseType_t xResult;
    uint32_t notified_value = 0;

    for (;;) {
        xResult = xTaskNotifyWait(pdFALSE, ULONG_MAX, &notified_value, portMAX_DELAY);
        if (xResult == pdPASS) {
            /* Process expiring timer(s) */
            if ((notified_value & (1 << QTMR_DEMO_EXPIRY_FROM_ISR)) != 0) {
                qtmr_demo_dump();
            }
        }
    }
}

static void cmd_qtmr_init_help(void)
{
    QTMR_PRINTF("Usage:\tQTMR Init <Instance>\r\n");
    QTMR_PRINTF("Options:\r\n");
    QTMR_PRINTF("\tInstance: 1 ~ 4, qtmr instance 1 ~ 4 can be used for customer (0 used for hres timer)\r\n");
    QTMR_PRINTF("\tExample:\r\n");
    QTMR_PRINTF("\tQTMR Init 2\r\n");
}

static void cmd_qtmr_start_help(void)
{
    QTMR_PRINTF("Usage:\tQTMR start <usec> <mode>\r\n");
    QTMR_PRINTF("Options:\r\n");
    QTMR_PRINTF("\tusec: duration for the qtmr timer\r\n");
    QTMR_PRINTF("\tmode: mode the qtmr timer, 0:Only once, 1:Repeat\r\n");
    QTMR_PRINTF("\tExample:\r\n");
    QTMR_PRINTF("\tQTMR Start 10000000 0\r\n");
}

qapi_Status_t qtmr_demo_init(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    qtmr_status status;
    qtmr_demo *demo;

    if ((Parameter_Count != 1) || (Parameter_List[0].Integer_Value <= QTMR_FRAME_0) ||
        (Parameter_List[0].Integer_Value >= QTMR_FRAME_NUM)) {
        QTMR_PRINTF("Invalid Parameter.\r\n");
        cmd_qtmr_init_help();
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_ERROR;
    }

    demo = qtmr_demo_get();
    demo->qtmr_instance = (qtmr_frame_instance)Parameter_List[0].Integer_Value;

    if (demo->state) {
        QTMR_PRINTF("QTMR DEMO: qtmr_demo_init already inited\r\n");
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_ERROR;
    }

    if (pdPASS != nt_qurt_thread_create(qtmr_demo_task, "qtmr_demo", QTMR_DEMO_TASK_STACK_SIZE, NULL,
                                        QTMR_DEMO_TASK_PRIORITY, &demo->qtmr_task)) {
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_ERROR;
    }

    status = qtmr_frame_comp_init(demo->qtmr_instance, qtmr_callback_func, demo);

    if (status) {
        QTMR_PRINTF("QTMR DEMO: qtmr_demo_init init frame comp failed with status %d\r\n", (int)status);
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_ERROR;
    }

    demo->state = QTMR_DEMO_INIT;

    return QAPI_OK;
}

qapi_Status_t qtmr_demo_start(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    qtmr_status status;
    qtmr_demo *demo;
    uint64_t now, tval, cval;
    uint32_t usec, mode;

    if ((Parameter_Count != 2)) {
        QTMR_PRINTF("Invalid Parameter.\r\n");
        cmd_qtmr_start_help();
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_ERROR;
    }

    usec = Parameter_List[0].Integer_Value;
    mode = Parameter_List[1].Integer_Value;

    demo = qtmr_demo_get();

    if (demo->state != QTMR_DEMO_INIT) {
        QTMR_PRINTF("QTMR DEMO: qtmr_demo_start not in init state\r\n");
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_ERROR;
    }

    tval = qtmr_usec_to_tick((uint64_t)usec);

    demo->qtmr_tval = tval;
    demo->mode = mode;

    status = qtmr_get_frame_count(demo->qtmr_instance, &now);

    if (status) {
        QTMR_PRINTF("QTMR DEMO: qtmr_demo_start get count failed with status %d\r\n", (int)status);
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_ERROR;
    }

    demo->qtmr_sval = now;
    cval = now + tval;
    demo->qtmr_cval = cval;

    if (mode)
        status = qtmr_frame_comp_start(demo->qtmr_instance, cval, tval, QTMR_FRAME_COMP_FLAG_REPEAT);
    else
        status = qtmr_frame_comp_start(demo->qtmr_instance, cval, 0, 0);

    if (status) {
        QTMR_PRINTF("QTMR DEMO: qtmr_demo_start start frame failed with status %d\r\n", (int)status);
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_ERROR;
    }

    return QAPI_OK;
}

qapi_Status_t qtmr_demo_stop()
{
    qtmr_status status;
    qtmr_demo *demo;

    demo = qtmr_demo_get();

    if (demo->state != QTMR_DEMO_INIT) {
        QTMR_PRINTF("QTMR DEMO: qtmr_demo_stop not in init state\r\n");
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_ERROR;
    }

    status = qtmr_frame_comp_stop(demo->qtmr_instance);

    if (status) {
        QTMR_PRINTF("qtmr demo: qtmr_demo_stop stop the frame comp failed with status %d\r\n", (int)status);
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_ERROR;
    }

    return QAPI_OK;
}

qapi_Status_t qtmr_demo_deinit()
{
    qtmr_status status;
    qtmr_demo *demo;

    demo = qtmr_demo_get();

    if (demo->state != QTMR_DEMO_INIT) {
        QTMR_PRINTF("QTMR DEMO: qtmr_demo_deinit not in init state\r\n");
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_ERROR;
    }

    status = qtmr_frame_comp_stop(demo->qtmr_instance);
    if (status) {
        QTMR_PRINTF("qtmr demo: qtmr_demo_deinit stop the frame comp failed with status %d\r\n", (int)status);
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_ERROR;
    }

    status = qtmr_frame_comp_deinit(demo->qtmr_instance);
    if (status) {
        QTMR_PRINTF("qtmr demo: qtmr_demo_deinit deinit the frame comp failed with status %d\r\n", (int)status);
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_ERROR;
    }

    nt_osal_thread_delete(demo->qtmr_task);

    demo->state = QTMR_DEMO_DEINIT;
    return QAPI_OK;
}
#endif  // QTMR_DEMO
