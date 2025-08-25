/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*========================================================================
 *
 * @brief This file contains the implementation of the Qtimer driver.
 *========================================================================*/

/*-------------------------------------------------------------------------
 * Include Files
 * ----------------------------------------------------------------------*/

#include "qtmr.h"
#if defined(PLATFORM_NT)
#include "neutrino_reg.h"
#elif defined(PLATFORM_FERMION)
#include "fermion_reg.h"
#include "hal_coex.h"
#endif /* PLATFORM_NT */
#include "nt_common.h"
#include "nt_hw.h"
#include "nt_osal.h"
#include "neutrino.h"
#if defined(HRES_TIMER_UNIT_TEST)
#include "uart.h"
#include "hal_int_sys.h"
#include "nt_socpm_sleep.h"
#include "timer.h"
#include "timer_internal.h"
#include "nt_logger_api.h"
#endif

/*-------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * ----------------------------------------------------------------------*/

#if defined(PLATFORM_NT)
#define QTMR_CNTR_FREQ_HZ (32000000u)
#else
#define QTMR_CNTR_FREQ_HZ (38400000u)
#endif /* PLATFORM_NT */
#define QTMR_FRAME_n (0u)

#define PMU_BASE_REG    SEQ_WCSS_PMU_OFFSET
#define QTMR_AC_BASE    SEQ_WCSS_QTMR_AC_OFFSET
#define QTMR_V1_T0_BASE SEQ_WCSS_QTMR_V1_T0_OFFSET
#define QTMR_V1_T1_BASE SEQ_WCSS_QTMR_V1_T1_OFFSET
#define QTMR_V1_T2_BASE SEQ_WCSS_QTMR_V1_T2_OFFSET
#define QTMR_V1_T3_BASE SEQ_WCSS_QTMR_V1_T3_OFFSET
#define QTMR_V1_T4_BASE SEQ_WCSS_QTMR_V1_T4_OFFSET

/**
 * Qtimer counter helper macros
 */
#define QTMR_TICKS_LO_BITS(x) (uint32_t)((uint32_t)(x)&0xFFFFFFFFu)
#define QTMR_TICKS_HI_BITS(x) (uint32_t)(((uint32_t)((x) >> 32)) & 0x00FFFFFFu)

#define MAX_DUMP 20
/*-------------------------------------------------------------------------
 * Type Declarations
 * ----------------------------------------------------------------------*/

/*-------------------------------------------------------------------------
 * Global Data Definitions
 * ----------------------------------------------------------------------*/
#if defined(HRES_TIMER_UNIT_TEST)
uint64_t g_qtimer_ticks[MAX_DUMP];
uint64_t g_aon_ticks[MAX_DUMP];
uint32_t g_mac_ticks[MAX_DUMP];
uint32_t g_sys_ticks[MAX_DUMP];
#if defined(PLATFORM_FERMION)
uint32_t g_wb_ticks[MAX_DUMP];
#endif
uint8_t g_idx_qtmr = 0;
/*periodicity of timer sync test*/
extern uint32_t g_test_period;
extern nt_osal_task_handle_t timer_task_handle;
#endif

/*-------------------------------------------------------------------------
 * Static Variable Definitions
 * ----------------------------------------------------------------------*/

/*-------------------------------------------------------------------------
 * Static Function Declarations and Definitions
 * ----------------------------------------------------------------------*/

/*-------------------------------------------------------------------------
 * Externalized Function Definitions
 * ----------------------------------------------------------------------*/

/**
 * @brief   Get frequency of QTMR counter
 * @return  qtmr counter frequency
 */
__attribute__((section(".__sect_ps_txt"))) uint32_t qtmr_get_freq(void)
{
    return QTMR_CNTR_FREQ_HZ;
}

/**
 * @brief   Enable / Disable Qtimer
 * @enable  enable  Flag (TRUE: enable, FALSE: disable)
 */
void qtmr_enable(qtmr_frame_t frame, bool enable)
{
    uint32_t mask, value;

    mask = HWIO_QTMR_V1_QTMR_V1_CNTP_CTL_IMSK_BMSK | HWIO_QTMR_V1_QTMR_V1_CNTP_CTL_EN_BMSK;

    if (enable) {
        /* Timer Control Register settings - unmask intr and enable timer */
        value = 0 << HWIO_QTMR_V1_QTMR_V1_CNTP_CTL_IMSK_SHFT | 1 << HWIO_QTMR_V1_QTMR_V1_CNTP_CTL_EN_SHFT;

        switch (frame) {
            case QTMR_FRAME_PHYSICAL_0:
                /* Unmask Interrupt and Enable Timer */
                HWIO_QTMR_V1_QTMR_V1_CNTP_CTL_OUTM(QTMR_V1_T0_BASE, mask, value);
                break;
            case QTMR_FRAME_PHYSICAL_1:
                HWIO_QTMR_V1_QTMR_V1_CNTP_CTL_OUTM(QTMR_V1_T1_BASE, mask, value);
                break;
            case QTMR_FRAME_PHYSICAL_2:
                HWIO_QTMR_V1_QTMR_V1_CNTP_CTL_OUTM(QTMR_V1_T2_BASE, mask, value);
                break;
            case QTMR_FRAME_PHYSICAL_3:
                HWIO_QTMR_V1_QTMR_V1_CNTP_CTL_OUTM(QTMR_V1_T3_BASE, mask, value);
                break;
            case QTMR_FRAME_PHYSICAL_4:
                HWIO_QTMR_V1_QTMR_V1_CNTP_CTL_OUTM(QTMR_V1_T4_BASE, mask, value);
                break;
            default:
                ASSERT(FALSE);
        }

    } else {
        /* Timer Control Register settings - mask intr and disable timer */
        value = 1 << HWIO_QTMR_V1_QTMR_V1_CNTP_CTL_IMSK_SHFT | 0 << HWIO_QTMR_V1_QTMR_V1_CNTP_CTL_EN_SHFT;

        switch (frame) {
            case QTMR_FRAME_PHYSICAL_0:
                /* Mask Interrupt and Disable Timer */
                HWIO_QTMR_V1_QTMR_V1_CNTP_CTL_OUTM(QTMR_V1_T0_BASE, mask, value);
                break;
            case QTMR_FRAME_PHYSICAL_1:
                HWIO_QTMR_V1_QTMR_V1_CNTP_CTL_OUTM(QTMR_V1_T1_BASE, mask, value);
                break;
            case QTMR_FRAME_PHYSICAL_2:
                HWIO_QTMR_V1_QTMR_V1_CNTP_CTL_OUTM(QTMR_V1_T2_BASE, mask, value);
                break;
            case QTMR_FRAME_PHYSICAL_3:
                HWIO_QTMR_V1_QTMR_V1_CNTP_CTL_OUTM(QTMR_V1_T3_BASE, mask, value);
                break;
            case QTMR_FRAME_PHYSICAL_4:
                HWIO_QTMR_V1_QTMR_V1_CNTP_CTL_OUTM(QTMR_V1_T4_BASE, mask, value);
                break;
            default:
                ASSERT(FALSE);
        }
    }
}

/**
 * @brief   Get the current timer counter value.
 * @return  Current time in ticks
 */
uint64_t qtmr_get_time64(qtmr_frame_t frame)
{
    uint64_t current_ticks;
    uint32_t count_lo, count_hi;

    switch (frame) {
        case QTMR_FRAME_PHYSICAL_0:
            count_hi = HWIO_QTMR_V1_QTMR_V1_CNTPCT_HI_IN(QTMR_V1_T0_BASE);
            count_lo = HWIO_QTMR_V1_QTMR_V1_CNTPCT_LO_IN(QTMR_V1_T0_BASE);
            break;
        case QTMR_FRAME_PHYSICAL_1:
            count_hi = HWIO_QTMR_V1_QTMR_V1_CNTPCT_HI_IN(QTMR_V1_T1_BASE);
            count_lo = HWIO_QTMR_V1_QTMR_V1_CNTPCT_LO_IN(QTMR_V1_T1_BASE);
            break;
        case QTMR_FRAME_PHYSICAL_2:
            count_hi = HWIO_QTMR_V1_QTMR_V1_CNTPCT_HI_IN(QTMR_V1_T2_BASE);
            count_lo = HWIO_QTMR_V1_QTMR_V1_CNTPCT_LO_IN(QTMR_V1_T2_BASE);
            break;
        case QTMR_FRAME_PHYSICAL_3:
            count_hi = HWIO_QTMR_V1_QTMR_V1_CNTPCT_HI_IN(QTMR_V1_T3_BASE);
            count_lo = HWIO_QTMR_V1_QTMR_V1_CNTPCT_LO_IN(QTMR_V1_T3_BASE);
            break;
        case QTMR_FRAME_PHYSICAL_4:
            count_hi = HWIO_QTMR_V1_QTMR_V1_CNTPCT_HI_IN(QTMR_V1_T4_BASE);
            count_lo = HWIO_QTMR_V1_QTMR_V1_CNTPCT_LO_IN(QTMR_V1_T4_BASE);
            break;
        default:
            ASSERT(FALSE);
    }

    current_ticks = (((uint64_t)count_hi << 32) | count_lo);

    return current_ticks;
}

/**
 * @brief   Set timer trigger value.
 * @match_count clock count at which next interrupt will occur
 */
void qtmr_set_trigger64(qtmr_frame_t frame, uint64_t match_count)
{
    uint32_t count_lo = 0, count_hi = 0;

    count_lo = QTMR_TICKS_LO_BITS(match_count);
    count_hi = QTMR_TICKS_HI_BITS(match_count);

    taskENTER_CRITICAL();

    switch (frame) {
        case QTMR_FRAME_PHYSICAL_0:
            HWIO_QTMR_V1_QTMR_V1_CNTP_CVAL_LO_OUT(QTMR_V1_T0_BASE, count_lo);
            HWIO_QTMR_V1_QTMR_V1_CNTP_CVAL_HI_OUT(QTMR_V1_T0_BASE, count_hi);
            break;
        case QTMR_FRAME_PHYSICAL_1:
            HWIO_QTMR_V1_QTMR_V1_CNTP_CVAL_LO_OUT(QTMR_V1_T1_BASE, count_lo);
            HWIO_QTMR_V1_QTMR_V1_CNTP_CVAL_HI_OUT(QTMR_V1_T1_BASE, count_hi);
            break;
        case QTMR_FRAME_PHYSICAL_2:
            HWIO_QTMR_V1_QTMR_V1_CNTP_CVAL_LO_OUT(QTMR_V1_T2_BASE, count_lo);
            HWIO_QTMR_V1_QTMR_V1_CNTP_CVAL_HI_OUT(QTMR_V1_T2_BASE, count_hi);
            break;
        case QTMR_FRAME_PHYSICAL_3:
            HWIO_QTMR_V1_QTMR_V1_CNTP_CVAL_LO_OUT(QTMR_V1_T3_BASE, count_lo);
            HWIO_QTMR_V1_QTMR_V1_CNTP_CVAL_HI_OUT(QTMR_V1_T3_BASE, count_hi);
            break;
        case QTMR_FRAME_PHYSICAL_4:
            HWIO_QTMR_V1_QTMR_V1_CNTP_CVAL_LO_OUT(QTMR_V1_T4_BASE, count_lo);
            HWIO_QTMR_V1_QTMR_V1_CNTP_CVAL_HI_OUT(QTMR_V1_T4_BASE, count_hi);
            break;
        default:
            ASSERT(FALSE);
    }

    taskEXIT_CRITICAL();
}

/**
 * @brief Set Root Clock
 *
 * @param[in] set_val (TRUE: enable rootclk, FALSE: disable rootclk)
 *
 * @return  None.
 */
void qtmr_set_rootclk(bool set_val)
{
    if (set_val) {
        HWIO_OUTX2F(PMU_BASE_REG, NEUTRINO_PMU_PRONTO_LP_FRODO_PMU_ROOT_CLK_ENABLE, QTIMER_AHB_ROOT_CLK_ENABLE,
                    QTIMER_XO_ROOT_CLK_ENABLE, 1, 1);
        HWIO_OUTXF(PMU_BASE_REG, NEUTRINO_PMU_PRONTO_LP_FRODO_PMU_SON_GDSCR, RETAIN_FF_ENABLE, 1);
    } else {
        HWIO_OUTX2F(PMU_BASE_REG, NEUTRINO_PMU_PRONTO_LP_FRODO_PMU_ROOT_CLK_ENABLE, QTIMER_AHB_ROOT_CLK_ENABLE,
                    QTIMER_XO_ROOT_CLK_ENABLE, 0, 0);
        HWIO_OUTXF(PMU_BASE_REG, NEUTRINO_PMU_PRONTO_LP_FRODO_PMU_SON_GDSCR, RETAIN_FF_ENABLE, 0);
    }
}

/**
 * @brief   Initialize the QTMR HW block
 */
__attribute__((section(".__sect_ps_txt"))) void qtmr_init()
{
    uint8_t cnt;

    qtmr_set_rootclk(TRUE);

    /* Enable QTMR Interrupt */
    NVIC_EnableIRQ(QTMR_0_INT);
    NVIC_EnableIRQ(QTMR_1_INT);
    NVIC_EnableIRQ(QTMR_2_INT);
    NVIC_EnableIRQ(QTMR_3_INT);
    NVIC_EnableIRQ(QTMR_4_INT);

    /* Set counter frequency */
    HWIO_QTMR_AC_QTMR_AC_CNTFRQ_OUT(QTMR_AC_BASE, QTMR_CNTR_FREQ_HZ);

    for (cnt = 0; cnt <= HWIO_QTMR_AC_QTMR_AC_CNTACR_n_MAXn; cnt++) {
        /* Enable Read and Write Access to CNTP Register */
        HWIO_QTMR_AC_QTMR_AC_CNTACR_n_OUTI(QTMR_AC_BASE, cnt, HWIO_QTMR_AC_QTMR_AC_CNTACR_n_RMSK);
    }
}

/**
 * qtmr_1_irq_handler
 *
 * @brief This is the interrupt handler for qtimer frame 1
 */
void __attribute__((section(".after_ram_vectors"))) qtmr_1_irq_handler(void)
{
#if defined(HRES_TIMER_UNIT_TEST)
    uint64_t now, next;
    uint32_t count_lo, count_hi;
    UBaseType_t int_status;

    qtmr_enable(QTMR_FRAME_PHYSICAL_1, FALSE);

    if (g_idx_qtmr < MAX_DUMP) {
        now = qtmr_get_time64(QTMR_FRAME_PHYSICAL_1);

        g_mac_ticks[g_idx_qtmr] = nt_hal_get_curr_time();
        g_sys_ticks[g_idx_qtmr] = xTaskGetTickCount();
        g_qtimer_ticks[g_idx_qtmr] = now;
        g_aon_ticks[g_idx_qtmr] = nt_socpm_get_slp_tmr_us();
#if defined(PLATFORM_FERMION)
        g_wb_ticks[g_idx_qtmr] = MacHalCoexLcmhGetWbtimer();
#endif

        g_idx_qtmr++;

        timer_cvt_to_tick64(g_test_period, T_USEC, &next);
        next += now;

        count_lo = QTMR_TICKS_LO_BITS(next);
        count_hi = QTMR_TICKS_HI_BITS(next);

        int_status = taskENTER_CRITICAL_FROM_ISR();
        HWIO_QTMR_V1_QTMR_V1_CNTP_CVAL_LO_OUT(QTMR_V1_T1_BASE, count_lo);
        HWIO_QTMR_V1_QTMR_V1_CNTP_CVAL_HI_OUT(QTMR_V1_T1_BASE, count_hi);
        qtmr_enable(QTMR_FRAME_PHYSICAL_1, TRUE);
        taskEXIT_CRITICAL_FROM_ISR(int_status);
    } else {
        g_idx_qtmr = 0;
    }
#endif
}

/**
 * qtmr_2_irq_handler
 *
 * @brief This is the interrupt handler for qtimer frame 2
 */
void __attribute__((section(".after_ram_vectors"))) qtmr_2_irq_handler(void)
{
#if defined(HRES_TIMER_UNIT_TEST)
    BaseType_t wakeup_task = pdFALSE;
    qtmr_enable(2, FALSE); /* disable qtimer interrupt */
    // nt_dbg_print("qtimer frame #2 irq\n\r");
    xTaskNotifyFromISR(timer_task_handle, (1 << TIMER_EVENT_QTMR_FRAME_2), eSetBits, &wakeup_task);
#endif
}

/**
 * qtmr_3_irq_handler
 *
 * @brief This is the interrupt handler for qtimer frame 3
 */
void __attribute__((section(".after_ram_vectors"))) qtmr_3_irq_handler(void)
{
#if defined(HRES_TIMER_UNIT_TEST)
    BaseType_t wakeup_task = pdFALSE;
    qtmr_enable(3, FALSE); /* disable qtimer interrupt */
    // nt_dbg_print("qtimer frame #3 irq\n\r");
    xTaskNotifyFromISR(timer_task_handle, (1 << TIMER_EVENT_QTMR_FRAME_3), eSetBits, &wakeup_task);
#endif
}

/**
 * qtmr_4_irq_handler
 *
 * @brief This is the interrupt handler for qtimer frame 4
 */
void __attribute__((section(".after_ram_vectors"))) qtmr_4_irq_handler(void)
{
#if defined(HRES_TIMER_UNIT_TEST)
    BaseType_t wakeup_task = pdFALSE;
    qtmr_enable(4, FALSE); /* disable qtimer interrupt */
    // nt_dbg_print("qtimer frame #4 irq\n\r");
    xTaskNotifyFromISR(timer_task_handle, (1 << TIMER_EVENT_QTMR_FRAME_4), eSetBits, &wakeup_task);
#endif
}

/**
 * @brief print synchoronization values
 *
 * @param None.
 *
 * @return  None.
 */
void qtmr_print_sync_data(void)
{
#if defined(HRES_TIMER_UNIT_TEST)
    uint8_t idx = 0;
    NT_LOG_PRINT(SYSTEM, CRIT, "Qtimer");
    for (idx = 0; idx < MAX_DUMP; idx++) {
        NT_LOG_PRINT(SYSTEM, CRIT, "%u%u", QTMR_TICKS_HI_BITS(g_qtimer_ticks[idx]),
                     QTMR_TICKS_LO_BITS(g_qtimer_ticks[idx]));
        vTaskDelay(10);
    }
    NT_LOG_PRINT(SYSTEM, CRIT, "MTU Timer");
    for (idx = 0; idx < MAX_DUMP; idx++) {
        NT_LOG_PRINT(SYSTEM, CRIT, "%d", (uint32_t)g_mac_ticks[idx]);
        vTaskDelay(10);
    }
#if defined(PLATFORM_FERMION)
    NT_LOG_PRINT(SYSTEM, CRIT, "WB Timer");
    for (idx = 0; idx < MAX_DUMP; idx++) {
        NT_LOG_PRINT(SYSTEM, CRIT, "%d", (uint32_t)g_wb_ticks[idx]);
        vTaskDelay(10);
    }
#endif
    NT_LOG_PRINT(SYSTEM, CRIT, "AON Timer");
    for (idx = 0; idx < MAX_DUMP; idx++) {
        NT_LOG_PRINT(SYSTEM, CRIT, "%u%u", QTMR_TICKS_HI_BITS(g_aon_ticks[idx]), QTMR_TICKS_LO_BITS(g_aon_ticks[idx]));
        vTaskDelay(20);
    }
#endif
}
