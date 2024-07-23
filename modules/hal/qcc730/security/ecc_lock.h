/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __ECC_LOCK_H__
#define __ECC_LOCK_H__

#include <stdint.h>

typedef struct ecc_lock_state_s {
    uint32_t *regbase;
} ecc_lock_state_t;

void ecc_lock_init(ecc_lock_state_t * ctxt, uint32_t * regbase);
void ecc_lock_request(ecc_lock_state_t * ctxt);
void ecc_lock_release(ecc_lock_state_t * ctxt);
uint32_t ecc_lock_is_locked_by_us(ecc_lock_state_t * ctxt);


#endif // __ECC_LOCK_H__

