/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/


#ifndef  NT_S_RECORD_H_
#define  NT_S_RECORD_H_

#include <stdint.h>

//Max size for storing the downloaded data
#define NT_MAX_B_SZ 1024
//Max size of buffer for storing the downloaded data
#define NT_REC_MAX_IMAGES 6
#define NT_REC_TYPE_CNT_ADDR_CHECKSUME_SIZE 7
#define NT_REC_SHIFT_8_BITS 8
#define NT_FDT_START_OFFSET  0x54
#define NT_REC_START_OFFSET  0x53
#define NT_REC_COUNT_OFFSET 2
#define NT_REC_NIBBLE_OFFSET 4
#define NT_REC_TYPE_OFFSET  1
#define NT_REC_DATA_OFFSET 28
#define NT_REC_REDUCE_LEN 3


#endif //NT_S_RECORD_H_
