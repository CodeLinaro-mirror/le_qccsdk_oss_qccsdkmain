/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/


#ifndef NEUTRINO_PBL_SYSTEM_NT_BL_RNG_H_
#define NEUTRINO_PBL_SYSTEM_NT_BL_RNG_H_

#include "nt_bl_common.h"

#ifdef NT_PRNG
int8_t nt_rng_init(void);
uint32_t nt_get_rng( void );

#if defined(SUPPORT_PBL_PATCH)
typedef int8_t (*nt_rng_init_t)(void);
typedef uint32_t (*nt_get_rng_t)( void );

typedef struct nt_bl_rng_s{
    nt_rng_init_t  nt_rng_init_pfn;
    nt_get_rng_t nt_get_rng_pfn;
}nt_bl_rng_t;
#endif
#endif /*SUPPORT_PBL_PATCH*/

#endif /* NEUTRINO_PBL_SYSTEM_NT_BL_RNG_H_ */
