/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include <string.h>
#include "com_dtypes.h"
#include "ferm_qtmr_internal.h"
#include "nt_logger_api.h"
#include "uart.h"

#ifdef QTMR_DRV
#define QTMR_DRV_DBG
#ifdef QTMR_DRV_DBG
static char qtmr_buf[60];
#define QTMR_PRINTF(...)                               \
    snprintf(qtmr_buf, sizeof(qtmr_buf), __VA_ARGS__); \
    nt_dbg_print(qtmr_buf);
#else
#define QTMR_PRINTF(...)
#endif

static qtmr_dev fm_qtmr;

static qtmr_dev *qtmr_get_dev()
{
    return &fm_qtmr;
}

__attribute__((section(".__sect_ps_txt"))) void qtmr_enable_clock(uint8_t enable)
{
#if CONFIG_SOC_QCC730V1
    PMU_BASE_pmu_Type *pmu = (PMU_BASE_pmu_Type *)(QCC730V1_PMU_BASE_BASE);
#elif CONFIG_SOC_QCC730V2
    PMU_BASE_pmu_Type *pmu = (PMU_BASE_pmu_Type *)(QCC730V2_PMU_BASE_BASE);
#endif
    uint8_t value;

    value = enable > 0 ? 1 : 0;

    pmu->PMU_ROOT_CLK_ENABLE.bit.QTIMER_XO_ROOT_CLK_ENABLE = value;
    pmu->PMU_ROOT_CLK_ENABLE.bit.QTIMER_AHB_ROOT_CLK_ENABLE = value;

    return;
}

__attribute__((section(".__sect_ps_txt"))) uint32_t qtmr_get_counter_freq()
{
    return QTMR_CNTR_FREQ_HZ;
}

uint64_t qtmr_usec_to_tick(uint64_t usec)
{
    uint32_t freq_hz;
    uint64_t tick;

    freq_hz = qtmr_get_counter_freq();
    tick = ((usec * freq_hz) / 1000000);

    return tick;
}

uint64_t qtmr_tick_to_usec(uint64_t tick)
{
    uint32_t freq_hz;
    uint64_t usec;

    freq_hz = qtmr_get_counter_freq();
    usec = (tick * 1000000) / freq_hz;

    return usec;
}

__attribute__((section(".__sect_ps_txt"))) void qtmr_plat_init()
{
    qtmr_dev *dev;
    qtmr_ac_hal *ac_hal;
    uint8_t i = 0;

    dev = qtmr_get_dev();

#if CONFIG_SOC_QCC730V1
    dev->ac.hal = (qtmr_ac_hal *)QCC730V1_QTMR_AC_BASE_BASE;
#elif CONFIG_SOC_QCC730V2
    dev->ac.hal = (qtmr_ac_hal *)QCC730V2_QTMR_AC_BASE_BASE;
#endif

    for (i = 0; i < QTMR_FRAME_NUM; i++) {
#if CONFIG_SOC_QCC730V1
        dev->frame[i].hal = (qtmr_tmr_hal *)(QCC730V1_QTMR_V1_T0_BASE_BASE + QTMR_V1_BASE_OFFSET * i);
#elif CONFIG_SOC_QCC730V2
        dev->frame[i].hal = (qtmr_tmr_hal *)(QCC730V2_QTMR_V1_T0_BASE_BASE + QTMR_V1_BASE_OFFSET * i);
#endif
        dev->frame[i].ac = QTMR_FRAME_AC_DEFAULT;
    }

    qtmr_enable_clock(1);

    /* Set counter frequency */
    ac_hal = dev->ac.hal;

    qtmr_hal_ac_set_cnt_freq(ac_hal, QTMR_CNTR_FREQ_HZ);

    for (i = 0; i < QTMR_FRAME_NUM; i++) {
        NVIC_EnableIRQ(QTMR_FRAME_INT(i));
        qtmr_hal_ac_set_cnt_acr(ac_hal, i, dev->frame[i].ac);
    }

    dev->state = QTMR_STATE_INIT;

    return;
}

__attribute__((section(".__sect_ps_txt"))) static void qtmr_enable_frame_comp(qtmr_frame *frame, uint8_t enable)
{
    qtmr_tmr_hal *hal;
    uint32_t value;

    hal = frame->hal;

    if (enable)
        value = QTMR_FRAME_CNTP_CTL_UMSK | QTMR_FRAME_CNTP_CTL_EN;
    else
        value = QTMR_FRAME_CNTP_CTL_MSK | QTMR_FRAME_CNTP_CTL_DIS;

    qtmr_hal_tmr_phy_ctr(hal, value);

    return;
}

__attribute__((section(".__sect_ps_txt"))) uint64_t qtmr_get_frame_count_no_check(qtmr_frame_instance instance)
{
    qtmr_dev *dev;
    uint32_t cnt_hi, cnt_lo;

    dev = qtmr_get_dev();

    qtmr_hal_tmr_get_phy_cnt(dev->frame[instance].hal, &cnt_hi, &cnt_lo);

    return HILO_BITS_QTMR_TICKS(cnt_hi, cnt_lo);
}

qtmr_status qtmr_get_frame_count(qtmr_frame_instance instance, uint64_t *cnt64)
{
    qtmr_dev *dev;
    uint32_t cnt_hi, cnt_lo;

    dev = qtmr_get_dev();

    if (instance >= QTMR_FRAME_NUM)
        return QTMR_ERROR_INVALID_PARAM;

    if (dev->state != QTMR_STATE_INIT)
        return QTMR_ERROR_INVALID_STATE;

    taskENTER_CRITICAL();

    qtmr_hal_tmr_get_phy_cnt(dev->frame[instance].hal, &cnt_hi, &cnt_lo);

    taskEXIT_CRITICAL();

    *cnt64 = HILO_BITS_QTMR_TICKS(cnt_hi, cnt_lo);

    return QTMR_SUCCESS;
}

__attribute__((section(".__sect_ps_txt"))) qtmr_status qtmr_frame_comp_start(qtmr_frame_instance instance,
                                                                             uint64_t cval, uint64_t tval,
                                                                             uint32_t flag)
{
    qtmr_dev *dev;
    qtmr_frame_comp *comp;
    qtmr_frame *frame;
    uint32_t cval_hi, cval_lo;

    dev = qtmr_get_dev();

    if (instance >= QTMR_FRAME_NUM)
        return QTMR_ERROR_INVALID_PARAM;

    if (dev->state != QTMR_STATE_INIT)
        return QTMR_ERROR_INVALID_STATE;

    frame = &dev->frame[instance];
    comp = &frame->comp;

    if (comp->state != QTMR_FRAME_COMP_INIT)
        return QTMR_ERROR_FRAME_COMP_STATE;

    cval_lo = QTMR_TICK64_LO_BITS(cval);
    cval_hi = QTMR_TICK64_HI_BITS(cval);

    taskENTER_CRITICAL();

    qtmr_hal_tmr_set_phy_cval(frame->hal, cval_hi, cval_lo);

    qtmr_enable_frame_comp(frame, 1);

    taskEXIT_CRITICAL();

    comp->flag = flag;
    comp->cval = cval;
    comp->tval = tval;

    return QTMR_SUCCESS;
}

qtmr_status qtmr_frame_comp_stop(qtmr_frame_instance instance)
{
    qtmr_dev *dev;
    qtmr_frame_comp *comp;
    qtmr_frame *frame;

    dev = qtmr_get_dev();

    if (instance >= QTMR_FRAME_NUM)
        return QTMR_ERROR_INVALID_PARAM;

    if (dev->state != QTMR_STATE_INIT)
        return QTMR_ERROR_INVALID_STATE;

    frame = &dev->frame[instance];
    comp = &frame->comp;

    qtmr_enable_frame_comp(frame, 0);

    comp->flag = 0;
    comp->cval = 0;
    comp->tval = 0;

    return QTMR_SUCCESS;
}

qtmr_status qtmr_frame_comp_init(qtmr_frame_instance instance, qtmr_callback callback, void *param)
{
    qtmr_dev *dev;
    qtmr_frame_comp *comp;

    dev = qtmr_get_dev();

    if (instance >= QTMR_FRAME_NUM)
        return QTMR_ERROR_INVALID_PARAM;

    if (dev->state != QTMR_STATE_INIT)
        return QTMR_ERROR_INVALID_STATE;

    comp = &dev->frame[instance].comp;
    if (comp->state != QTMR_FRAME_COMP_DEINIT)
        return QTMR_ERROR_FRAME_COMP_STATE;

    comp->cb = callback;
    comp->param = param;
    comp->state = QTMR_FRAME_COMP_INIT;

    return QTMR_SUCCESS;
}

qtmr_status qtmr_frame_comp_deinit(qtmr_frame_instance instance)
{
    qtmr_dev *dev;
    qtmr_frame_comp *comp;

    dev = qtmr_get_dev();

    if (instance >= QTMR_FRAME_NUM)
        return QTMR_ERROR_INVALID_PARAM;

    if (dev->state != QTMR_STATE_INIT)
        return QTMR_ERROR_INVALID_STATE;

    comp = &dev->frame[instance].comp;
    if (comp->state == QTMR_FRAME_COMP_DEINIT)
        return QTMR_ERROR_FRAME_COMP_STATE;

    memset(comp, 0, sizeof(qtmr_frame_comp));

    comp->state = QTMR_FRAME_COMP_DEINIT;

    QTMR_PRINTF("qtmr frame %d deinit\r\rn", instance);

    return QTMR_SUCCESS;
}

static void __attribute__((section(".after_ram_vectors"))) qtmr_frame_irq(qtmr_frame_instance instance)
{
    qtmr_dev *dev;
    qtmr_frame *frame;
    qtmr_frame_comp *comp;
    uint32_t cnt_hi, cnt_lo, cval_lo, cval_hi;
    uint64_t cur_cnt64, next_cnt64;

    dev = qtmr_get_dev();
    frame = &dev->frame[instance];
    comp = &frame->comp;

    qtmr_enable_frame_comp(frame, 0);

    if (comp->cb)
        comp->cb(comp->param);

    if ((comp->flag & QTMR_FRAME_COMP_FLAG_REPEAT) != 0 && comp->tval != 0) {
        qtmr_hal_tmr_get_phy_cnt(frame->hal, &cnt_hi, &cnt_lo);
        cur_cnt64 = HILO_BITS_QTMR_TICKS(cnt_hi, cnt_lo);

        next_cnt64 = cur_cnt64 + comp->tval;

        cval_lo = QTMR_TICK64_LO_BITS(next_cnt64);
        cval_hi = QTMR_TICK64_HI_BITS(next_cnt64);

        qtmr_hal_tmr_set_phy_cval(frame->hal, cval_hi, cval_lo);
        qtmr_enable_frame_comp(frame, 1);
    }

    return;
}

#define QTMR_IRQ_HANDLER(num)                                                    \
    void __attribute__((section(".after_ram_vectors"))) qtmr_irq_handler_##num() \
    {                                                                            \
        qtmr_frame_irq(num);                                                     \
    }

QTMR_IRQ_HANDLER(0);
QTMR_IRQ_HANDLER(1);
QTMR_IRQ_HANDLER(2);
QTMR_IRQ_HANDLER(3);
QTMR_IRQ_HANDLER(4);
#endif  // QTMR_DRV
