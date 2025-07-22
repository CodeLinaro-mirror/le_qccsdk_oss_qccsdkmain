/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __ELPPKA_INTR_H__
#define __ELPPKA_INTR_H__

#define PKA_USE_INTERRUPT 1
#define ECC_INTERRUPT_ID  51

#define NT_NVIC_ISER1   0xE000E104  // Irq 32 to 63 Set Enable Register
#define NVIC_PKA_ENABLE 0x1 << (ECC_INTERRUPT_ID % 32)

#endif /* __ELPPKA_INTR_H__ */
