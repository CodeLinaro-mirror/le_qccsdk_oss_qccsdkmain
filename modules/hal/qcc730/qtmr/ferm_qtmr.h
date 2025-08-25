/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef CORE_SYSTEM_INC_FERM_QTMR_H_
#define CORE_SYSTEM_INC_FERM_QTMR_H_
#include <stdint.h>
//#include "nt_common.h"
//#include "nt_osal.h"

typedef enum {
    QTMR_FRAME_0,
    QTMR_FRAME_1,
    QTMR_FRMAE_2,
    QTMR_FRAME_3,
    QTMR_FRAME_4,
    QTMR_FRAME_NUM,
} qtmr_frame_instance;

typedef void (*qtmr_callback)(void *param);

typedef enum {
    QTMR_SUCCESS,
    QTMR_ERROR_INVALID_PARAM,
    QTMR_ERROR_INVALID_STATE,
    QTMR_ERROR_FRAME_COMP_STATE,
} qtmr_status;

#define QTMR_FRAME_COMP_FLAG_ONCE   1
#define QTMR_FRAME_COMP_FLAG_REPEAT 2
#define QTMR_FRAME_COMP_FLAG_MASK   QTMR_FRAME_COMP_FLAG_ONCE | QTMR_FRAME_COMP_FLAG_REPEAT

#define QTMR_TICK64_LO_BITS(x)       (unsigned int)((uint32_t)(x)&0xFFFFFFFFu)
#define QTMR_TICK64_HI_BITS(x)       (unsigned int)(((uint32_t)((x) >> 32)) & 0x00FFFFFFu)
#define HILO_BITS_QTMR_TICKS(hi, lo) (uint64_t)(((uint64_t)(hi) << 32) | lo)

#define QTMR_TIME64_LO_BITS(x) (unsigned int)((uint32_t)(x)&0xFFFFFFFFu)
#define QTMR_TIME64_HI_BITS(x) (unsigned int)((uint32_t)((x) >> 32) & 0xFFFFFFFFu)

void qtmr_enable_clock(uint8_t enable);
void qtmr_plat_init();
uint32_t qtmr_get_counter_freq();
uint64_t qtmr_usec_to_tick(uint64_t usec);
uint64_t qtmr_tick_to_usec(uint64_t tick);
uint64_t qtmr_get_frame_count_no_check(qtmr_frame_instance instance);

qtmr_status qtmr_get_frame_count(qtmr_frame_instance instance, uint64_t *cnt64);
qtmr_status qtmr_frame_comp_start(qtmr_frame_instance instance, uint64_t cval, uint64_t tval, uint32_t flag);
qtmr_status qtmr_frame_comp_stop(qtmr_frame_instance instance);
qtmr_status qtmr_frame_comp_init(qtmr_frame_instance instance, qtmr_callback callback, void *param);
qtmr_status qtmr_frame_comp_deinit(qtmr_frame_instance instance);

#endif  // CORE_SYSTEM_INC_FERM_QTMR_H
