
/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */


#ifndef _PBL_SYSTEM_NT_BL_VECTOR_TABLE_H_
#define _PBL_SYSTEM_NT_BL_VECTOR_TABLE_H_
#include "nt_bl_common.h"

extern arm_core_regs_t core_regs;

void NMI_Handler_Ind(void);
void init_fault_handlers(void);

/* Function pointers for IRQ Handler extention function */
typedef void (*NMI_Handler_Ind_t)(void);
typedef void (*Hard_FaultHandler_Ind_t)(void);
typedef void (*MemManage_Handler_Ind_t)(void);
typedef void (*BusFault_Handler_Ind_t)(void);
typedef void (*UsageFault_Handler_Ind_t)(void);
typedef void (*init_fault_handlers_t)(void);

/* Strucutre to hold address of IRQ Handler extention functions */
typedef struct irq_handler_ext_s{
    NMI_Handler_Ind_t			NMI_Handler_Ind_pfn;
    Hard_FaultHandler_Ind_t		Hard_FaultHandler_Ind_pfn;
    MemManage_Handler_Ind_t		MemManage_Handler_Ind_pfn;
    BusFault_Handler_Ind_t		BusFault_Handler_Ind_pfn;
    UsageFault_Handler_Ind_t	UsageFault_Handler_Ind_pfn;
	init_fault_handlers_t		init_fault_handlers_pfn;
}irq_handler_ext_ind_t;

#endif
