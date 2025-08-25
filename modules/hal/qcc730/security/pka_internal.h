/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __PKA_INTERNAL_H__
#define __PKA_INTERNAL_H__

#include "ecc_lock.h"
#include "elppka.h"
#include "qurt_mutex.h"
#include "unpa.h"

struct pka_state_s {
    ecc_lock_state_t lock_ctxt;
    struct pka_state elppka_ctxt;
    qurt_mutex_t mutex;  // used so that only a single SW thread can lock PKA at a time
    unpa_client *unpa_client_ctxt;
};

#endif  // __PKA_INTERNAL_H__
