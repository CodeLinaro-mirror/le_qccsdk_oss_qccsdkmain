/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "ecc_lock.h"
#include "ecc_lock_hw.h"

#define io_read32(addr)         (*(volatile uint32_t *)addr)
#define io_write32(addr, value) (*(volatile uint32_t *)addr = value)

#define ECC_LOCK_VMID_OURS 4

void ecc_lock_init(ecc_lock_state_t *ctxt, uint32_t *regbase)
{
    ctxt->regbase = regbase;
}

void ecc_lock_request(ecc_lock_state_t *ctxt)
{
    io_write32(&ctxt->regbase[ECC_LOCK_REQUEST], LOCK_REQUEST_LOCK_REQUEST_SET(1));
}

void ecc_lock_release(ecc_lock_state_t *ctxt)
{
    io_write32(&ctxt->regbase[ECC_LOCK_RELEASE], LOCK_RELEASE_LOCK_RELEASE_SET(1));
}

uint32_t ecc_lock_is_locked_by_us(ecc_lock_state_t *ctxt)
{
    uint32_t val = io_read32(&ctxt->regbase[ECC_LOCK_STATUS]);
    if (1 && (LOCK_STATUS_STATUS_GET(val) != 0) && (LOCK_STATUS_VMID_GET(val) == ECC_LOCK_VMID_OURS)) {
        return 1;
    }
    return 0;
}
