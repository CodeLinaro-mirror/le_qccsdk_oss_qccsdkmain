/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier:
 * BSD-3-Clause-Clear
*/
#ifndef _RNG_h_
#define _RNG_h_ #include <stdint.h>
#include "nt_common.h"
#define RNG_CACHE_SIZE 64  // 64 words or 256 bytes.
#define RNG_CACHE_THRESHOLD \
    16  // max request size from FW modules today is 192B, hence (256 - 192 = 64 bytes or 16 words)
#define NT_UINT32_MAX 0xFFFFFFFF
#if !defined(AR6002_REV77)
#define CACHE_INDEX_2_CYCLIC_INDEX(index) ((index) % ((RNG_CACHE_SIZE) * sizeof(uint32_t)))
#define INDEX_IS_OVERFLOW(index)          (((index) >= ((RNG_CACHE_SIZE) * sizeof(uint32_t))) ? TRUE : FALSE)
#endif

typedef enum {
    RNG_DISABLED = 0,
    RNG_ENABLED,
    RNG_PENDING,
} RNG_STATE;

/* Data structure for DRNG module */
typedef struct _RngCtx {
    uint32_t rng_cache[RNG_CACHE_SIZE];
    uint32_t rng_rd_index;
    uint32_t rng_write_index;
    uint32_t rng_num_avail;
    RNG_STATE state;
} RngCtx;
NT_BOOL Rng_Init(void);
RngCtx *Rng_getGlobalRngCtx(void);
NT_BOOL Rng_getRNG(uint8_t *ptr, uint16_t len);
NT_BOOL Rng_refillRndCache(uint8_t *ptr, uint16_t len);
NT_BOOL Rng_Instantiate(RngCtx *rngCtx, uint16_t lenInWords);
#endif /* _RNG_h_ */
