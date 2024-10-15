/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/



#ifndef _NT_HEAP_STATS_H
#define _NT_HEAP_STATS_H

#include "nt_flags.h"

#ifdef NT_TU_HEAP_STATS
#include "FreeRTOS.h"
void table_update_malloc(int nbytes,UBaseType_t SchedulerState,UBaseType_t ulTaskId);
void table_update_free(int mbytes,UBaseType_t SchedulerState,UBaseType_t ulTaskId);
int get_total(int *input_to_get_total);
void getclearTableMemory(void);
#if (NT_FN_QC_HEAP == 2)
void bin_stats(int binNo,int binType);
#endif
/* -- Includes -- */
#include "FreeRTOS.h"

/* for, max number of task names stored in buffer*/
#define TASKID_ARR_NROWS  20


/* for max size of name*/
#define NT_MAX_TASK_NAME_LENGTH 16

/* Starting address of heap provided by linker */
extern unsigned char _ln_RAM_addr_heap_start__;

#ifdef PLATFORM_FERMION
#define HEAP_END_ADDR   ( 0x9FFFF )
#else
#define HEAP_END_ADDR	( 0x7FFFF )
#endif //PLATFORM_FERMION
/**
 * for taskName stored in buffer
 */
typedef struct heap_stats_s
{
	char taskName[NT_MAX_TASK_NAME_LENGTH];
}heap_stats_t;
/**
 * @Function : nt_task_name_table_cfg(void)
 * @description : for task names we are storing in buffer.
 * And Assigned RAM address to buffer this is for debugging purpose.
 * @parm     : None.
 * @return : None.
 */
void nt_task_name_table_cfg(void);
/*
 * @Function: nt_table_update_malloc
 * @description: calculate the max malloc allocations and how much max number allocated
 *   malloc and Current usage.
 * @parm    : nbytes : malloc size.
 *            SchedulerState : for stared or not
 *            ulTaskId       : Current task id
 *  @return : NULL;
 */
void nt_table_update_malloc(int nbytes,UBaseType_t SchedulerState,UBaseType_t ulTaskId);
/*
 * @Function: nt_table_update_free
 * @description: calculate the Current freed and how many freed.
 *
 * @parm    : mbytes : free size.
 *            SchedulerState : for stared or not
 *            ulTaskId       : Current task id
 *  @return : NULL;
 */
void nt_table_update_free(int mbytes,UBaseType_t SchedulerState,UBaseType_t ulTaskId);
/*
 * @Function: nt_get_total
 * @description: It's adding the total values in array
 *
 * @parm    : input_to_get_total : for, getting values from all the arrays
 * like CurrentUsage and StepCurrentUsage etc..
 *
 *  @return : int
 */
int nt_get_total(int *input_to_get_total);
/*
 * @Function: nt_getClearTableMemory
 * @description: clear the stepMaxUsage and stepCurrentUsage and NumMalloc and NumFrees.
 *
 * @parm    : NULL;
 * @return : NULL;
 */
void nt_getclearTableMemory(void);
/*
 * @Function : nt_getupdatedTableMemory
 * @description: for printing updated table
 * @parm     : Null.
 * @return   : NULL
 */
void nt_getupdatedTableMemory(void);

#endif /** NT_TU_HEAP_STATS */
#endif //_NT_HEAP_STATS_H
