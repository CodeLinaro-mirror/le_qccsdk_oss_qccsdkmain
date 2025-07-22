/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef CORE_SYSTEM_INC_FERM_INTERNAL_QTMR_H_
#define CORE_SYSTEM_INC_FERM_INTERNALQTMR_H_
#include <stdint.h>
#include "nt_common.h"
#include "nt_osal.h"
#include "ferm_qtmr.h"
#include "ferm_qtmr_hal.h"

typedef struct {
    qtmr_ac_hal *hal;
    uint32_t freq;
    uint32_t cnt_tid;
} qtmr_ac;

typedef struct {
    uint32_t state;
    uint64_t cval;
    uint64_t tval;
    uint32_t flag;
    qtmr_callback cb;
    void *param;
} qtmr_frame_comp;

typedef struct {
    qtmr_tmr_hal *hal;
    uint32_t ac;
    qtmr_frame_comp comp;
} qtmr_frame;

typedef struct {
    qtmr_ac ac;
    qtmr_frame frame[QTMR_FRAME_NUM];
    uint32_t state;
} qtmr_dev;

#define QTMR_BIT(n)      (1 << (n))
#define QTMR_BIT_MASK(n) (QTMR_BIT(n) - 1UL)

#define QTMR_V1_BASE_OFFSET 0x1000UL
#define QTMR_CNTR_FREQ_HZ   (38400000u)

#define QTMR_STATE_DEINIT 0
#define QTMR_STATE_INIT   1

#define QTMR_FRAME_INT(FRAME_NUM) (FRAME_NUM + 4) /* IRQn for Qtmr framen */
#define QTMR_FRAME_AC_RPCT        QTMR_BIT(0)     // Read access to CNTPCT register
#define QTMR_FRAME_AC_RPVCT       QTMR_BIT(1)     // Read access to CNTVCT register
#define QTMR_FRAME_AC_RFRQ        QTMR_BIT(2)     // Read access to CNTFRQ	register
#define QTMR_FRAME_AC_RVOFF       QTMR_BIT(3)     // Read access to CNTVOFF register
#define QTMR_FRAME_AC_RWVT        QTMR_BIT(4)     // Read/Write access to CNTV_* registers
#define QTMR_FRAME_AC_RWPT        QTMR_BIT(5)     // Read/Write access to CNTP_* registers
#define QTMR_FRAME_AC_DEFAULT                                                                                  \
    QTMR_FRAME_AC_RPCT | QTMR_FRAME_AC_RPVCT | QTMR_FRAME_AC_RFRQ | QTMR_FRAME_AC_RVOFF | QTMR_FRAME_AC_RWVT | \
        QTMR_FRAME_AC_RWPT

#define QTMR_FRAME_COMP_DEINIT 0
#define QTMR_FRAME_COMP_INIT   1

#define QTMR_FRAME_CNTP_CTL_UMSK 0
#define QTMR_FRAME_CNTP_CTL_MSK  QTMR_BIT(1)
#define QTMR_FRAME_CNTP_CTL_DIS  0
#define QTMR_FRAME_CNTP_CTL_EN   QTMR_BIT(0)

#endif  // CORE_SYSTEM_INC_FERM_QTMR_H
