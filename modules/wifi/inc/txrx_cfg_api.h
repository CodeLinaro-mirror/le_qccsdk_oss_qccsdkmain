/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

//
// This file contains the txrx config data definitions
//

#ifndef __TXRX_CFG_API_H__
#define __TXRX_CFG_API_H__

#include <stdio.h>
#include "nt_common.h"  //NT_BOOL

typedef struct {
    uint16_t userPriority : 3;
    uint16_t ps : 1;
    uint16_t reserved_bit_9 : 1;
    uint16_t accessPolicy : 2;
    uint16_t direction : 2;
    uint16_t tsid : 4;
    uint16_t trafficType : 1;
} TS_INFO;

typedef struct {
    NT_BOOL valid;
    uint8_t statusCode;
    TS_INFO tsInfo;
    uint16_t nominalMSDU;
    uint16_t maxMSDU;
    uint32_t minServiceInt;
    uint32_t maxServiceInt;
    uint32_t inactivityInt;
    uint32_t suspensionInt;
    uint32_t serviceStartTime;
    uint32_t minDataRate;
    uint32_t meanDataRate;
    uint32_t peakDataRate;
    uint32_t maxBurstSize;
    uint32_t delayBound;
    uint32_t minPhyRate;
    uint32_t sba;
    uint32_t mediumTime;
} WMM_TSPEC_INFO;

#endif /* __TXRX_CFG_API_H__ */
