/*
* Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#ifndef ERRLOG_ARMM_H
#define ERRLOG_ARMM_H
/*===========================================================================

                   L O G  P A C K E T S  F O R  E R R

DESCRIPTION
  This header file contains the definitions of log structure for core dump
 
===========================================================================*/


/************************************************************************
 *                        ARCH_COREDUMP_TYPES
 ************************************************************************/

/****************
 *    ARM
 ****************/

typedef enum
{
  ARM_R0 = 0,
  ARM_R1,
  ARM_R2,
  ARM_R3,
  ARM_R12,
  ARM_LR,
  ARM_PC,
  ARM_PSR,
  ARM_ICSR,
  ARM_VTOR,
  ARM_AIRCR,
  ARM_SCR,
  ARM_CCR,
  ARM_SHPR1,
  ARM_SHPR2,
  ARM_SHPR3,
  ARM_SHCSR,
  ARM_CFSR,
  ARM_HFSR,
  ARM_DFSR,
  ARM_MMFAR,
  ARM_BFAR,
  ARM_AFSR,
  ARM_PFR0,
  ARM_PFR1,
  ARM_DFR,
  ARM_ADR,
  ARM_MMFR0,
  ARM_MMFR1,
  ARM_MMFR2,
  ARM_MMFR3,
  ARM_ISAR0,
  ARM_ISAR1,
  ARM_ISAR2,
  ARM_ISAR3,
  ARM_ISAR4,
  ARM_CPACR,
  ARM_NVIC_ISPR0,
  ARM_NVIC_ISPR1,
  ARM_NVIC_ISPR2,
  ARM_NVIC_ISER0,
  ARM_NVIC_ICER1,
  ARM_NVIC_ICER2,
  SIZEOF_ARCH_COREDUMP_REGISTERS
} arch_coredump_register_type;

#define SIZEOF_SVC_REGS 

typedef struct
{
  uint32 regs[SIZEOF_ARCH_COREDUMP_REGISTERS];
} arch_coredump_array_type;

typedef struct
{
  uint32 r0;
  uint32 r1;
  uint32 r2;
  uint32 r3;
  uint32 r12;
  uint32 lr;
  uint32 pc;
  uint32 psr;
  uint32 icsr;
  uint32 vtor;
  uint32 aircr;
  uint32 scr;
  uint32 ccr;
  uint32 shpr1;
  uint32 shpr2;
  uint32 shpr3;
  uint32 shcsr;
  uint32 cfsr;
  uint32 hfsr;
  uint32 dfsr;
  uint32 mmfar;
  uint32 bfar;
  uint32 afsr;
  uint32 pfr0;
  uint32 pfr1;
  uint32 dfr;
  uint32 adr;
  uint32 mmfr[4];
  uint32 isar[5];
  uint32 cpacr;
  uint32 ispr[3];
  uint32 iser[3];
} arch_coredump_field_type;

union arch_coredump_union
{
  uint32                   array[SIZEOF_ARCH_COREDUMP_REGISTERS];
  arch_coredump_field_type name;
};


#endif /* ERRLOG_ARMM_H */
