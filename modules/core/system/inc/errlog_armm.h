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

typedef enum {
    ARM_R0 = 0,
    ARM_R1,
    ARM_R2,
    ARM_R3,
    ARM_R4,
    ARM_R5,
    ARM_R6,
    ARM_R7,
    ARM_R8,
    ARM_R9,
    ARM_R10,
    ARM_R11,
    ARM_R12,
    ARM_SP,
    ARM_LR,
    ARM_PC,
    ARM_PSP,
    ARM_MSP,
    ARM_PSR,
#ifndef ERR_CORTEXM0
    ARM_APSR,
    ARM_IPSR,
    ARM_EPSR,
#endif
    ARM_PRIMASK,
#ifndef ERR_CORTEXM0
    ARM_FAULTMASK,
    ARM_BASEPRI,
#endif
    ARM_CONTROL,
    ARM_EXCEPTION_R0,
    ARM_EXCEPTION_R1,
    ARM_EXCEPTION_R2,
    ARM_EXCEPTION_R3,
    ARM_EXCEPTION_R12,
    ARM_EXCEPTION_LR,
    ARM_EXCEPTION_PC,
    ARM_EXCEPTION_XPSR,
    SIZEOF_ARCH_COREDUMP_REGISTERS
} arch_coredump_register_type;

#define SIZEOF_SVC_REGS

typedef struct {
    unsigned int regs[SIZEOF_ARCH_COREDUMP_REGISTERS];
} arch_coredump_array_type;

typedef struct {
    unsigned int regs[13]; /* r0-r12 */
    unsigned int sp;
    unsigned int lr;
    unsigned int pc;
    unsigned int psp;
    unsigned int msp;
    unsigned int psr;
#ifndef ERR_CORTEXM0
    unsigned int aspr;
    unsigned int ipsr;
    unsigned int epsr;
#endif
    unsigned int primask;
#ifndef ERR_CORTEXM0
    unsigned int faultmask;
    unsigned int basepri;
#endif
    unsigned int control;
    unsigned int exception_r0;
    unsigned int exception_r1;
    unsigned int exception_r2;
    unsigned int exception_r3;
    unsigned int exception_r12;
    unsigned int exception_lr;
    unsigned int exception_pc;
    unsigned int exception_xpsr;
} arch_coredump_field_type;

union arch_coredump_union {
    unsigned int array[SIZEOF_ARCH_COREDUMP_REGISTERS];
    arch_coredump_field_type name;
};

typedef struct err_coredump_config_reg {
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
} err_coredump_config_reg;

#endif /* ERRLOG_ARMM_H */
