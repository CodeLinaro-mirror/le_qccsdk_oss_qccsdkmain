/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __ECC_LOCK_HW_H__
#define __ECC_LOCK_HW_H__


// REGISTER OFFSETS
enum {
    ECC_LOCK_REQUEST,
    ECC_LOCK_RELEASE,
    ECC_LOCK_STATUS,
    ECC_LOCK_TIMTOUT,
    ECC_LOCK_INTR_STATUS,
    ECC_LOCK_INTR_ENABLE,
};


// LOCK_REQUEST
#define LOCK_REQUEST_LOCK_REQUEST_LSB                   0
#define LOCK_REQUEST_LOCK_REQUEST_MSB                   0
#define LOCK_REQUEST_LOCK_REQUEST_MASK                  0x00000001
#define LOCK_REQUEST_LOCK_REQUEST_GET(x)                (((x) & LOCK_REQUEST_LOCK_REQUEST_MASK) >> LOCK_REQUEST_LOCK_REQUEST_LSB)
#define LOCK_REQUEST_LOCK_REQUEST_SET(x)                (((0 | (x)) << LOCK_REQUEST_LOCK_REQUEST_LSB) & LOCK_REQUEST_LOCK_REQUEST_MASK)
#define LOCK_REQUEST_LOCK_REQUEST_RESET                 0x0

// LOCK_RELEASE
#define LOCK_RELEASE_LOCK_RELEASE_LSB                   0
#define LOCK_RELEASE_LOCK_RELEASE_MSB                   0
#define LOCK_RELEASE_LOCK_RELEASE_MASK                  0x00000001
#define LOCK_RELEASE_LOCK_RELEASE_GET(x)                (((x) & LOCK_RELEASE_LOCK_RELEASE_MASK) >> LOCK_RELEASE_LOCK_RELEASE_LSB)
#define LOCK_RELEASE_LOCK_RELEASE_SET(x)                (((0 | (x)) << LOCK_RELEASE_LOCK_RELEASE_LSB) & LOCK_RELEASE_LOCK_RELEASE_MASK)
#define LOCK_RELEASE_LOCK_RELEASE_RESET                 0x0

// LOCK_STATUS
#define LOCK_STATUS_STATUS_LSB                          31
#define LOCK_STATUS_STATUS_MSB                          31
#define LOCK_STATUS_STATUS_MASK                         0x80000000
#define LOCK_STATUS_STATUS_GET(x)                       (((x) & LOCK_STATUS_STATUS_MASK) >> LOCK_STATUS_STATUS_LSB)
#define LOCK_STATUS_STATUS_SET(x)                       (((0 | (x)) << LOCK_STATUS_STATUS_LSB) & LOCK_STATUS_STATUS_MASK)
#define LOCK_STATUS_STATUS_RESET                        0x0
#define LOCK_STATUS_VMID_LSB                            0
#define LOCK_STATUS_VMID_MSB                            4
#define LOCK_STATUS_VMID_MASK                           0x0000001F
#define LOCK_STATUS_VMID_GET(x)                         (((x) & LOCK_STATUS_VMID_MASK) >> LOCK_STATUS_VMID_LSB)
#define LOCK_STATUS_VMID_SET(x)                         (((0 | (x)) << LOCK_STATUS_VMID_LSB) & LOCK_STATUS_VMID_MASK)
#define LOCK_STATUS_VMID_RESET                          0x0

// LOCK_TIMEOUT
#define LOCK_TIMEOUT_TIMEOUT_LOADVAL_LSB                0
#define LOCK_TIMEOUT_TIMEOUT_LOADVAL_MSB                11
#define LOCK_TIMEOUT_TIMEOUT_LOADVAL_MASK               0x00000FFF
#define LOCK_TIMEOUT_TIMEOUT_LOADVAL_GET(x)             (((x) & LOCK_TIMEOUT_TIMEOUT_LOADVAL_MASK) >> LOCK_TIMEOUT_TIMEOUT_LOADVAL_LSB)
#define LOCK_TIMEOUT_TIMEOUT_LOADVAL_SET(x)             (((0 | (x)) << LOCK_TIMEOUT_TIMEOUT_LOADVAL_LSB) & LOCK_TIMEOUT_TIMEOUT_LOADVAL_MASK)
#define LOCK_TIMEOUT_TIMEOUT_LOADVAL_RESET              0x0

// LOCK_INTR_STATUS
#define LOCK_INTR_STATUS_INTRSTATUS_LSB                 0
#define LOCK_INTR_STATUS_INTRSTATUS_MSB                 0
#define LOCK_INTR_STATUS_INTRSTATUS_MASK                0x00000001
#define LOCK_INTR_STATUS_INTRSTATUS_GET(x)              (((x) & LOCK_INTR_STATUS_INTRSTATUS_MASK) >> LOCK_INTR_STATUS_INTRSTATUS_LSB)
#define LOCK_INTR_STATUS_INTRSTATUS_SET(x)              (((0 | (x)) << LOCK_INTR_STATUS_INTRSTATUS_LSB) & LOCK_INTR_STATUS_INTRSTATUS_MASK)
#define LOCK_INTR_STATUS_INTRSTATUS_RESET               0x0

// LOCK_INTR_ENABLE
#define LOCK_INTR_ENABLE_INTRENABLE_LSB                 0
#define LOCK_INTR_ENABLE_INTRENABLE_MSB                 0
#define LOCK_INTR_ENABLE_INTRENABLE_MASK                0x00000001
#define LOCK_INTR_ENABLE_INTRENABLE_GET(x)              (((x) & LOCK_INTR_ENABLE_INTRENABLE_MASK) >> LOCK_INTR_ENABLE_INTRENABLE_LSB)
#define LOCK_INTR_ENABLE_INTRENABLE_SET(x)              (((0 | (x)) << LOCK_INTR_ENABLE_INTRENABLE_LSB) & LOCK_INTR_ENABLE_INTRENABLE_MASK)
#define LOCK_INTR_ENABLE_INTRENABLE_RESET               0x0

#endif // __ECC_LOCK_HW_H__

