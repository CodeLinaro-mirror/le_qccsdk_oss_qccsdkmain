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
  unsigned int regs[SIZEOF_ARCH_COREDUMP_REGISTERS];
} arch_coredump_array_type;

typedef struct
{
  unsigned int r0;
  unsigned int r1;
  unsigned int r2;
  unsigned int r3;
  unsigned int r12;
  unsigned int lr;
  unsigned int pc;
  unsigned int psr;
  unsigned int icsr;
  unsigned int vtor;
  unsigned int aircr;
  unsigned int scr;
  unsigned int ccr;
  unsigned int shpr1;
  unsigned int shpr2;
  unsigned int shpr3;
  unsigned int shcsr;
  unsigned int cfsr;
  unsigned int hfsr;
  unsigned int dfsr;
  unsigned int mmfar;
  unsigned int bfar;
  unsigned int afsr;
  unsigned int pfr0;
  unsigned int pfr1;
  unsigned int dfr;
  unsigned int adr;
  unsigned int mmfr[4];
  unsigned int isar[5];
  unsigned int cpacr;
  unsigned int ispr[3];
  unsigned int iser[3];
} arch_coredump_field_type;

union arch_coredump_union
{
  unsigned int                   array[SIZEOF_ARCH_COREDUMP_REGISTERS];
  arch_coredump_field_type name;
};


#endif /* ERRLOG_ARMM_H */
