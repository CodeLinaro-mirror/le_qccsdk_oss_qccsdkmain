/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include <string.h>
#include <limits.h>
#include "ferm_prof.h"
#include "ferm_qtmr.h"
#include "uart.h"
#include "nt_common.h"
#include "qurt_utils.h"
#include "safeAPI.h"

#ifdef PROF_DRV
#define PROF_DRV_DBG
#ifdef PROF_DRV_DBG
static char prof_buf[60];
#define PROF_PRINTF(...)                               \
    snprintf(prof_buf, sizeof(prof_buf), __VA_ARGS__); \
    nt_dbg_print(prof_buf);
#else
#define PROF_PRINTF(...)
#endif

static prof_dev fm_prof;
static prof_irq_stats irq_stats[PROF_IRQ_NUM];
static char prof_dump_buf[2000];

static char *irq_name[] = {
    "CTI_INTISR_0",
    "CTI_INTISR_1",
    "I2C",
    "UART",
    "QTMR_0",
    "QTMR_1",
    "QTMR_2",
    "QTMR_3",
    "QTMR_4",
    "NA",
    "NA",
    "NA",
    "NA",
    "NA",
    "NA",
    "NA",
    "NA",
    "NA",
    "CCU_APPS_WLAN_XFER_DONE",
    "CCU_APPS_WLAN_RX_AVAIL",
    "CMEM_INT",
    "DXE_0",
    "DXE_1",
    "DXE_2",
    "DXE_3",
    "DXE_4",
    "DXE_5",
    "DXE_6",
    "DXE_7",
    "DXE_8",
    "DXE_9",
    "DXE_10",
    "DXE_11",
    "WLAN_CCU",
    "WLAN_CCU_FIQ",
    "CCPU_CCU_AHB_ERR",
    "CCPU_CCU_CMEM_TIMEOUT",
    "NA",
    "PMU_CCPU_VBAT_LOW",
    "PMU_CCPU_TEMP_PANIC",
    "NA",
    "EXT",
    "NA",
    "RRI",
    "CCU_APPS_ASIC",
    "RRAM",
    "NA",
    "NA",
    "GPIO",
    "XPU2_NON_SECURE",
    "KDF_M4F",
    "ECC4_M4F",
    "CC",
    "WUR_CPU",
    "WDT",
    "AON_CMNSS_WLAN_SLP_TMR",
    "CCPU_CCU_FP_EXP_0",
    "CCPU_CCU_FP_EXP_1",
    "CCPU_CCU_FP_EXP_2",
    "CCPU_CCU_FP_EXP_3",
    "CCPU_CCU_FP_EXP_4",
    "CCPU_CCU_FP_EXP_5",
    "NA",
    "AON_A2F_ASSERT",
    "XPU2_SECURITY",
    "PMU_SLP_CAL_DONE",
    "CMEM_BANK_A_AUTO_PW",
    "CMEM_BANK_B_AUTO_PW",
    "CMEM_BANK_C_AUTO_PW",
    "CMEM_BANK_D_AUTO_PW",
    "CMEM_CMN_AUTO_PW",
    "CPR",
    "SPI_SLV",
    "AHB_PD_ERR",
    "NA",
    "NA",
    "NA",
    "NA",
    "NA",
    "NA",
    "NA",
    "NA",
    "NA",
    "COEX_BMH_ISR",
    "COEX_SMH_ISR",
    "COEX_PMH_ISR",
    "COEX_MCIM_ISR",
    "COEX_LMH_ISR",
    "NA",
    "NA",
    "NA",
    "NA",
    "NA",
    "NA",
    "NA",
    "NA",
    "NA",
    "NA",
    "NA",
    "NA",
    "NA",
    "NA",
    "NA",
    "NA",
    "NA",
    "NA",
    "NA",
    "NA",
    "A2F_DEASSERT",
};

static inline prof_dev *prof_get_dev()
{
    return &fm_prof;
}

static void prof_timer_callback(void *param)
{
    BaseType_t wakeup_task = pdFALSE;
    prof_dev *dev = (prof_dev *)param;

    if (dev->state == PROF_START && dev->cfg.prof_timer_task) {
        /* Signal the timer task of the timer interrupt event */
        xTaskNotifyFromISR((nt_osal_task_handle_t)dev->cfg.prof_timer_task, PROF_STOP_EVENT_FROM_ISR, eSetBits,
                           &wakeup_task);
        /* Wake the priority task if required */
        portYIELD_FROM_ISR(wakeup_task);
    }
}

void prof_timer_os_snapshot(task_snap *snap)
{
    unsigned long task_num;
    uint32_t ulTotalTime;
    task_num = uxTaskGetNumberOfTasks();

    if (snap == NULL)
        return;

    if (snap->status_snap != NULL) {
        snap->snap_num = uxTaskGetSystemState(snap->status_snap, task_num, &ulTotalTime);
    }

    return;
}

static void prof_timer_task(void __attribute__((__unused__)) * pvParameters)
{
    BaseType_t xResult;
    uint32_t notified_value = 0;

    for (;;) {
        xResult = xTaskNotifyWait(pdFALSE, ULONG_MAX, &notified_value, portMAX_DELAY);
        if (xResult == pdPASS) {
            /* Process expiring timer(s) */
            if (notified_value == PROF_STOP_EVENT_FROM_ISR) {
                prof_timer_stop();
            }
        }
    }
}

prof_status prof_timer_init()
{
    prof_dev *dev;
    qtmr_status status;

    qtmr_plat_init();

    dev = prof_get_dev();
    dev->cfg.instance = QTMR_FRAME_1;
    dev->irq.stats = irq_stats;
    dev->dump.buf = prof_dump_buf;

    dev->os.prof_start.max_num = PROF_OS_MAX_SNAP;
    dev->os.prof_start.status_snap = pvPortMalloc(dev->os.prof_start.max_num * sizeof(TaskStatus_t));
    dev->os.prof_stop.max_num = PROF_OS_MAX_SNAP;
    dev->os.prof_stop.status_snap = pvPortMalloc(dev->os.prof_stop.max_num * sizeof(TaskStatus_t));

    if (pdPASS != nt_qurt_thread_create(prof_timer_task, "prof_timer", PROF_TIMER_TASK_STACK_SIZE, NULL,
                                        PROF_TIMER_TASK_PRIORITY, (TaskHandle_t *)&dev->cfg.prof_timer_task))
        return PROF_ERROR_TIMER;

    status = qtmr_frame_comp_init(dev->cfg.instance, prof_timer_callback, dev);

    if (status) {
        PROF_PRINTF("PROF DRV: prof_timer_init init frame comp failed with status %d\r\n", (int)status);
        return PROF_ERROR_TIMER;
    }

    dev->state = PROF_INIT;

    return PROF_SUCCESS;
}

prof_status prof_timer_deinit()
{
    prof_dev *dev;
    qtmr_status status;

    dev = prof_get_dev();

    status = qtmr_frame_comp_stop(dev->cfg.instance);
    if (status) {
        PROF_PRINTF("PROF DRV: prof_timer_deinit stop frame comp failed with status %d\r\n", (int)status);
        return PROF_ERROR_TIMER;
    }

    status = qtmr_frame_comp_deinit(dev->cfg.instance);
    if (status) {
        PROF_PRINTF("PROF DRV: prof_timer_deinit deinit frame comp failed with status %d\r\n", (int)status);
        return PROF_ERROR_TIMER;
    }
    if (dev->os.prof_start.status_snap != NULL)
        vPortFree(dev->os.prof_start.status_snap);

    if (dev->os.prof_stop.status_snap != NULL)
        vPortFree(dev->os.prof_stop.status_snap);

    nt_osal_thread_delete(dev->cfg.prof_timer_task);

    dev->state = PROF_DEINIT;

    return PROF_SUCCESS;
}

uint64_t prof_timer_get_count()
{
    prof_dev *dev;
    prof_cfg *cfg;
    uint64_t prof_tick;

    dev = prof_get_dev();
    cfg = &dev->cfg;

    if (dev->cfg.start == 1) {
        prof_tick = qtmr_get_frame_count_no_check(cfg->instance);
        prof_tick = prof_tick - dev->cfg.start_tick;
    } else {
        prof_tick = 0;
    }

    return prof_tick;
}

prof_status prof_timer_config(PROF_TIMER_MODE mode, uint64_t duration)
{
    prof_dev *dev;
    prof_cfg *cfg;

    dev = prof_get_dev();
    cfg = &dev->cfg;

    if (dev->state != PROF_INIT)
        return PROF_ERROR_STATE;

    cfg->duration = duration;
    cfg->mode = mode;

    return PROF_SUCCESS;
}

prof_status prof_timer_start()
{
    prof_dev *dev;
    prof_cfg *cfg;
    qtmr_status status;

    dev = prof_get_dev();
    cfg = &dev->cfg;

    if (dev->state != PROF_INIT)
        return PROF_ERROR_STATE;

    memset(dev->irq.stats, 0, PROF_IRQ_NUM * sizeof(prof_irq_stats));

    prof_timer_os_snapshot(&(dev->os.prof_start));

    cfg->start_tick = qtmr_get_frame_count_no_check(dev->cfg.instance);
    cfg->start = 1;

    if (cfg->mode == PROF_SPEC_TIME_MODE) {
        // tval = qtmr_usec_to_tick(cfg->duration);
        cfg->cval = cfg->start_tick + cfg->duration;
        status = qtmr_frame_comp_start(cfg->instance, cfg->cval, 0, 0);

        PROF_PRINTF("PROF DRV: prof_timer_start start with cval %u:%u\r\n", QTMR_TICK64_HI_BITS(cfg->cval),
                    QTMR_TICK64_LO_BITS(cfg->cval));

        if (status) {
            PROF_PRINTF("PROF DRV: prof_timer_start start frame comp failed with status %d\r\n", (int)status);
            return PROF_ERROR_TIMER;
        }
    }

    dev->state = PROF_START;

    PROF_PRINTF("PROF DRV: Profing started with mode %d\r\n", (int)dev->cfg.mode);

    return PROF_SUCCESS;
}

prof_status prof_timer_stop()
{
    prof_dev *dev;
    qtmr_status status;

    dev = prof_get_dev();

    if (dev->state == PROF_START) {
        dev->cfg.start = 0;
        dev->cfg.end_tick = qtmr_get_frame_count_no_check(dev->cfg.instance);
        dev->cfg.duration = dev->cfg.end_tick - dev->cfg.start_tick;

        prof_timer_os_snapshot(&(dev->os.prof_stop));
        status = qtmr_frame_comp_stop(dev->cfg.instance);

        PROF_PRINTF("PROF DRV: Profing stopped\r\n");

        if (status) {
            PROF_PRINTF("PROF DRV: prof_timer_stop stop frame comp failed with status %d\r\n", (int)status);
            return PROF_ERROR_TIMER;
        }

        dev->state = PROF_INIT;
    }

    return PROF_SUCCESS;
}

static char *prof_write_name_to_buf(char *buf, const char *name)
{
    size_t x;

    /* Start by copying the entire string. */
    strncpy_s(buf, configMAX_TASK_NAME_LEN, name, configMAX_TASK_NAME_LEN);

    /* Pad the end of the string with spaces to ensure columns line up when
    printed out. */
    for (x = strlen(buf); x < (size_t)(configMAX_TASK_NAME_LEN - 1); x++) {
        buf[x] = ' ';
    }

    /* Terminate. */
    buf[x] = (char)0x00;

    /* Return the new end of string. */
    return &(buf[x]);
}

void prof_timer_os_update_task_status(TaskStatus_t *task_status, task_snap *snap)
{
    TaskStatus_t *snap_task;
    uint32_t x;

    if (snap != NULL && snap->status_snap != NULL) {
        for (x = 0; x < snap->snap_num; x++) {
            snap_task = &(snap->status_snap[x]);
            if (snap_task->xTaskNumber == task_status->xTaskNumber) {
                task_status->ulRunTimeCounter -= snap_task->ulRunTimeCounter;
                break;
            }
        }
    }

    return;
}

static void prof_write_data_to_buf(char *write_buf, char *dump_head, uint32_t percentage, uint32_t decimal,
                                   uint64_t time64)
{
    if (percentage == 100) {
        if (dump_head)
            snprintf(write_buf, sizeof(prof_dump_buf), "%s\t100%%\t\t%u:%u\r\n", dump_head, QTMR_TIME64_HI_BITS(time64),
                     QTMR_TIME64_LO_BITS(time64));

        else
            snprintf(write_buf, sizeof(prof_dump_buf), "\t100%%\t\t%u:%u\r\n", QTMR_TIME64_HI_BITS(time64),
                     QTMR_TIME64_LO_BITS(time64));

        return;
    }

    if (decimal >= 10) {
        if (dump_head)
            snprintf(write_buf, sizeof(prof_dump_buf), "%s\t%u.%u%%\t\t%u:%u\r\n", dump_head, (unsigned int)percentage,
                     (unsigned int)decimal, QTMR_TIME64_HI_BITS(time64), QTMR_TIME64_LO_BITS(time64));
        else
            snprintf(write_buf, sizeof(prof_dump_buf), "\t%u.%u%%\t\t%u:%u\r\n", (unsigned int)percentage,
                     (unsigned int)decimal, QTMR_TIME64_HI_BITS(time64), QTMR_TIME64_LO_BITS(time64));
    } else if (decimal > 0 || percentage > 0) {
        if (dump_head)
            snprintf(write_buf, sizeof(prof_dump_buf), "%s\t%u.0%u%%\t\t%u:%u\r\n", dump_head, (unsigned int)percentage,
                     (unsigned int)decimal, QTMR_TIME64_HI_BITS(time64), QTMR_TIME64_LO_BITS(time64));
        else
            snprintf(write_buf, sizeof(prof_dump_buf), "\t%u.0%u%%\t\t%u:%u\r\n", (unsigned int)percentage,
                     (unsigned int)decimal, QTMR_TIME64_HI_BITS(time64), QTMR_TIME64_LO_BITS(time64));
    } else {
        if (dump_head)
            snprintf(write_buf, sizeof(prof_dump_buf), "%s\t<0.01%%\t\t%u:%u\r\n", dump_head,
                     QTMR_TIME64_HI_BITS(time64), QTMR_TIME64_LO_BITS(time64));
        else
            snprintf(write_buf, sizeof(prof_dump_buf), "\t<0.01%%\t\t%u:%u\r\n", QTMR_TIME64_HI_BITS(time64),
                     QTMR_TIME64_LO_BITS(time64));
    }

    return;
}

void prof_timer_os_stats_dump()
{
    prof_dev *dev;
    prof_cfg *cfg;
    TaskStatus_t *pxTaskStatusArray;
    TaskStatus_t task_status;
    unsigned long x, task_num;
    uint32_t ulTotalTime;
    uint32_t stats_percentage = 0, stats_decimal = 0;
    uint64_t total_tick64, task_time64_usec, total_time64_usec;
    char *pcWriteBuffer;
#ifdef PROF_DRV_OS_REMOVE_IRQ
    uint64_t tasks_time64_usec = 0;
#endif

    dev = prof_get_dev();
    cfg = &dev->cfg;

    pcWriteBuffer = dev->dump.buf;

    if (cfg->start == 0) {
        pxTaskStatusArray = dev->os.prof_stop.status_snap;
        task_num = dev->os.prof_stop.snap_num;
    } else {
        task_num = uxTaskGetNumberOfTasks();
        pxTaskStatusArray = pvPortMalloc(task_num * sizeof(TaskStatus_t));

        if (pxTaskStatusArray != NULL) {
            task_num = uxTaskGetSystemState(pxTaskStatusArray, task_num, &ulTotalTime);
        }
    }

    // Should update the run ticks firstly before calling this dump function
    total_tick64 = cfg->run_tick;

    if (pxTaskStatusArray != NULL) {
        /* For percentage calculations. */
        total_time64_usec = qtmr_tick_to_usec(total_tick64);
        total_time64_usec /= 100UL;

        /* Avoid divide by zero errors. */
        if (total_time64_usec > 0UL) {
            /* Create a human readable table from the binary data. */
            for (x = 0; x < task_num; x++) {
                /* What percentage of the total run time has the task used?
                This will always be rounded down to the nearest integer.
                ulTotalRunTimeDiv100 has already been divided by 100. */

                task_status = pxTaskStatusArray[x];
                prof_timer_os_update_task_status(&task_status, &(dev->os.prof_start));
                task_time64_usec = qtmr_tick_to_usec(task_status.ulRunTimeCounter);
                stats_percentage = task_time64_usec / total_time64_usec;
                stats_decimal = task_time64_usec % total_time64_usec;
                stats_decimal = (uint32_t)((double)stats_decimal / (double)total_time64_usec * 100);

#ifdef PROF_DRV_OS_REMOVE_IRQ
                tasks_time64_usec += task_time64_usec;
#endif
                /* Write the task name to the string, padding with
                spaces so it can be printed in tabular form more
                easily. */
                pcWriteBuffer = prof_write_name_to_buf(pcWriteBuffer, task_status.pcTaskName);

                prof_write_data_to_buf(pcWriteBuffer, NULL, stats_percentage, stats_decimal, task_time64_usec);

                pcWriteBuffer +=
                    strlen(pcWriteBuffer); /*lint !e9016 Pointer arithmetic ok on char pointers especially as in this
                                              case where it best denotes the intent of the code. */
            }

#ifdef PROF_DRV_OS_REMOVE_IRQ
            stats_percentage = tasks_time64_usec / total_time64_usec;
            stats_decimal = tasks_time64_usec % total_time64_usec;
            stats_decimal = (uint32_t)((double)stats_decimal / (double)total_time64_usec * 100);

            pcWriteBuffer = prof_write_name_to_buf(pcWriteBuffer, "Total tasks");
            prof_write_data_to_buf(pcWriteBuffer, NULL, stats_percentage, stats_decimal, tasks_time64_usec);
#endif
            nt_dbg_print(dev->dump.buf);
        }
    }

    if (cfg->start == 1 && pxTaskStatusArray != NULL)
        vPortFree(pxTaskStatusArray);
}

void prof_timer_irq_stats_dump()
{
    prof_dev *dev;
    prof_irq_stats *stats;
    uint32_t i = 0;
    uint64_t irq_usec;
    char *pcWriteBuffer;
#ifdef PROF_DRV_OS_REMOVE_IRQ
    uint64_t irqs_usec = 0;
#endif
    uint32_t stats_percentage = 0, stats_decimal = 0;
    uint64_t total_tick64, total_time64_usec = 0;

    dev = prof_get_dev();
    stats = dev->irq.stats;
    pcWriteBuffer = dev->dump.buf;

    // Should update the run tick firstly before calling this dump function
    total_tick64 = dev->cfg.run_tick;

    total_time64_usec = qtmr_tick_to_usec(total_tick64);
    total_time64_usec /= 100UL;

    while (i < PROF_IRQ_NUM) {
        if (stats[i].irq_cnt) {
            irq_usec = qtmr_tick_to_usec(stats[i].irq_run);

            if (total_time64_usec > 0) {
                stats_percentage = irq_usec / total_time64_usec;
                stats_decimal = irq_usec % total_time64_usec;
                stats_decimal = (uint32_t)((double)stats_decimal / (double)total_time64_usec * 100);
            }

            if (stats_decimal >= 10)
                snprintf(pcWriteBuffer, sizeof(prof_dump_buf), "IRQ[%u] %s \t%u.%u%%\t\t%u:%u\t\tcnt:%u\r\n",
                         (unsigned int)i, irq_name[i], (unsigned int)stats_percentage, (unsigned int)stats_decimal,
                         QTMR_TIME64_HI_BITS(irq_usec), QTMR_TIME64_LO_BITS(irq_usec), (unsigned int)stats[i].irq_cnt);
            else if (stats_decimal > 0)
                snprintf(pcWriteBuffer, sizeof(prof_dump_buf), "IRQ[%u] %s \t%u.0%u%%\t\t%u:%u\t\tcnt:%u\r\n",
                         (unsigned int)i, irq_name[i], (unsigned int)stats_percentage, (unsigned int)stats_decimal,
                         QTMR_TIME64_HI_BITS(irq_usec), QTMR_TIME64_LO_BITS(irq_usec), (unsigned int)stats[i].irq_cnt);
            else
                snprintf(pcWriteBuffer, sizeof(prof_dump_buf), "IRQ[%u] %s \t<0.01%%\t\t%u:%u\t\tcnt:%u\r\n",
                         (unsigned int)i, irq_name[i], QTMR_TIME64_HI_BITS(irq_usec), QTMR_TIME64_LO_BITS(irq_usec),
                         (unsigned int)stats[i].irq_cnt);

            pcWriteBuffer += strlen(pcWriteBuffer);

#ifdef PROF_DRV_OS_REMOVE_IRQ
            irqs_usec += irq_usec;
#endif
        }
        i++;
    }

#ifdef PROF_DRV_OS_REMOVE_IRQ
    if (total_time64_usec > 0) {
        stats_percentage = irqs_usec / total_time64_usec;
        stats_decimal = irqs_usec % total_time64_usec;
        stats_decimal = (uint32_t)((double)stats_decimal / (double)total_time64_usec * 100);

        prof_write_data_to_buf(pcWriteBuffer, "Total IRQs", stats_percentage, stats_decimal, irqs_usec);
    }
#endif

    nt_dbg_print(dev->dump.buf);

    return;
}

void prof_timer_dump(uint32_t dump_cfg)
{
    prof_dev *dev;
    uint64_t duration;

    dev = prof_get_dev();

    if (dev->cfg.start == 0)
        duration = dev->cfg.duration;
    else
        duration = prof_timer_get_count();

    dev->dump.cfg = dump_cfg;
    dev->cfg.run_tick = duration;
    duration = qtmr_tick_to_usec(duration);
    PROF_PRINTF("Profiling Duation: %u:%u  Done: %d\r\n", QTMR_TIME64_HI_BITS(duration), QTMR_TIME64_LO_BITS(duration),
                (int)(!dev->cfg.start));

    if (dev->dump.cfg & PROF_DUMP_OS) {
        PROF_PRINTF("\r\n==========================================================================\r\n");
        PROF_PRINTF("\r\nOS stats:\r\n");
        prof_timer_os_stats_dump();
    }

    if (dev->dump.cfg & PROF_DUMP_IRQ) {
        PROF_PRINTF("\r\n==========================================================================\r\n");
        PROF_PRINTF("\r\nIRQ stats:\r\n");
        prof_timer_irq_stats_dump();
    }

    return;
}

prof_irq_stats *prof_timer_irq_stats()
{
    prof_irq_stats *stats;
    prof_dev *dev;

    dev = prof_get_dev();

    stats = dev->irq.stats;

    return stats;
}

static inline uint32_t prof_irq_num()
{
    uint32_t icsr;
    uint32_t irq_num;

    icsr = NT_REG_RD(PROF_ICSR);
    irq_num = PROF_IRQ_ACTIVE_NUM(icsr);

    return irq_num;
}

void __attribute__((section(".after_ram_vectors"))) prof_irq_enter()
{
    uint32_t irq_num;
    prof_dev *dev;
    prof_irq_stats *stats;

    dev = prof_get_dev();

    if (dev->cfg.start == 0)
        return;

    irq_num = prof_irq_num();

    if (irq_num < PROF_IRQ_NUM) {
        stats = &(dev->irq.stats[irq_num]);

        stats->irq_num = irq_num;
        stats->irq_enter = prof_timer_get_count();
        stats->irq_cnt++;

#ifdef PROF_DRV_OS_REMOVE_IRQ
        TaskRunCounterUpdate(stats->irq_enter);
#endif
    }

    return;
}

void __attribute__((section(".after_ram_vectors"))) prof_irq_exit()
{
    uint32_t irq_num;
    prof_dev *dev;
    prof_irq_stats *stats;

    dev = prof_get_dev();

    if (dev->cfg.start == 0)
        return;

    irq_num = prof_irq_num();

    if (irq_num < PROF_IRQ_NUM) {
        stats = &(dev->irq.stats[irq_num]);

        stats->irq_exit = prof_timer_get_count();
        stats->irq_num = irq_num;
        stats->irq_run += PROF_RUN_TIME(stats->irq_enter, stats->irq_exit);

#ifdef PROF_DRV_OS_REMOVE_IRQ
        ulTaskSwitchedInTimeUpdate(stats->irq_exit);
#endif
    }

    return;
}

#endif  // PROF_DRV
