/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/


/* -- Includes -- */
#include "nt_flags.h"

#ifdef NT_TU_HEAP_STATS
#include <stdio.h>
#include "nt_heap_stats.h"
#include "task.h"
#include "uart.h" /* nt_dbg_print*/
#include "nt_osal.h"
#include <string.h>

//extern unsigned int _task_name_start_addr; //for debugging purpose all the created task names stored in RAM
extern int CurrTotalUsagePerStep;
extern int MaxTotalUsagePerStep;
extern int max_total_usage;

static int ulArrTaskId[TASKID_ARR_NROWS] = {0};	//Array to store the Task IDs accordingly

static int NumMalloc[TASKID_ARR_NROWS] = {0};
static int NumFree[TASKID_ARR_NROWS] = {0};

static int CurrentUsage[TASKID_ARR_NROWS] = {0};		//global current usage per task
static int StepCurrentUsage[TASKID_ARR_NROWS] = {0};	//Records current usage for each step per task. Can be cleared to 0.
static int MaxMalloc[TASKID_ARR_NROWS] = {0};
static int StepMaxUsage[TASKID_ARR_NROWS] = {0};

static int MaxUsageByTasks[TASKID_ARR_NROWS] = {0};
heap_stats_t *ptr_name[TASKID_ARR_NROWS] = {0};

/**
 * @Function : nt_task_name_table_cfg(void)
 * @description : for task names we are storing in buffer.
 * And Assigned RAM address to buffer this is for debugging purpose.
 * @parm     : None.
 * @return : None.
 */
#if (NT_FN_QC_HEAP == 2)
int used_bin[16] = {0};
int free_bin[16] = {0};
void bin_stats(int binNo,int binType)
{
	if(binType == 0){
		used_bin[binNo] += 1;
	}
	if(binType == 1){
		free_bin[binNo] += 1;
	}
}
#endif
void nt_task_name_table_cfg(void)
{

    for(uint8_t row = 0; row < TASKID_ARR_NROWS ;row++)
    {
    	ptr_name[row] = (heap_stats_t *)nt_osal_calloc(1,sizeof(heap_stats_t));
    }
}
/*
 * @Function: nt_table_update_malloc
 * @description: calculate the max malloc allocations and how much max number allocated
 *   malloc and Current usage.
 * @parm    : nbytes : malloc size.
 *            SchedulerState : for stared or not
 *            ulTaskId       : Current task id
 *  @return : NULL;
 */

 void nt_table_update_malloc(int nbytes,UBaseType_t SchedulerState,UBaseType_t ulTaskId )
{
	char* TaskName = NULL;
	/*BaseType_t SchedulerState = NULL;*/
	uint8_t row;

	//SchedulerState = xTaskGetSchedulerState();

	/* **************************************************************************************************************************
	 * The task id is invalid before the start of the scheduler. The memory allocation before the vTaskStartScheduler will be
	 * stored inside the if condition. Once the scheduler is started, every task have their unique Task IDs which will be used to
	 * update the information i.e in the else condition.
	 * ****************************************************************************************************************************/
	if(SchedulerState == taskSCHEDULER_NOT_STARTED)
	{

		if(&ptr_name[0]->taskName[0] == 0)
			return;
		memcpy(&ptr_name[0]->taskName[0],"OS Alloc",NT_MAX_TASK_NAME_LENGTH);
		ulArrTaskId[0] = 0;
		NumMalloc[0] += 1;
		CurrentUsage[0] += nbytes;
		StepCurrentUsage[0] += nbytes;

		if(nbytes > MaxMalloc[ 0 ])
			MaxMalloc[ 0 ] = nbytes;

		if(StepMaxUsage[ 0 ] < CurrentUsage[ 0 ])
		{
			StepMaxUsage[ 0 ] = CurrentUsage[ 0 ];
			MaxUsageByTasks[0] = CurrentUsage[0];
		}
	}
	else
	{
		TaskName = qurt_thread_get_name(NULL);
		for(row = 0; row < TASKID_ARR_NROWS; row++)
		{
			if( ulArrTaskId[ row ] == (int)ulTaskId )
			{
				NumMalloc[ row ] += 1;
				CurrentUsage[ row ] += nbytes;
				StepCurrentUsage[row] += nbytes;

				if(nbytes > MaxMalloc[ row ])
					MaxMalloc[ row ] = nbytes;

				//global max usage per task
				if(MaxUsageByTasks[ row ] < CurrentUsage[ row ])
					MaxUsageByTasks[ row ] = CurrentUsage[ row ];

				//max usage per step per task
				if(StepMaxUsage[ row ] < StepCurrentUsage[ row ])
					StepMaxUsage[ row ] = StepCurrentUsage[ row ];
				break;
			}
		}
		if( row >= TASKID_ARR_NROWS)
		{
			for(row = 0; row < TASKID_ARR_NROWS; row++)
			{
				if(ptr_name[row]->taskName[0] == '\0')
				{
					memcpy(&ptr_name[row]->taskName[0],TaskName,NT_MAX_TASK_NAME_LENGTH);
					NumMalloc[ row ] = 1;
					NumFree[ row ] = 0;

					CurrentUsage[ row ] = nbytes;
					StepCurrentUsage[row] = nbytes;

					MaxMalloc[ row ] = nbytes;

					MaxUsageByTasks[ row ] = nbytes;
					StepMaxUsage[row] = nbytes;

					ulArrTaskId[ row ] = ulTaskId;	//Scheduler started. So the calling task will have its own id. Update the Task ID for the utility use.
					break;
				}
			}
		}
	}
}
 /*
  * @Function: nt_get_total
  * @description: It's adding the total values in array
  *
  * @parm    : input_to_get_total : for, getting values from all the arrays
  * like CurrentUsage and StepCurrentUsage etc..
  *
  *  @return : int
  */

int nt_get_total(int *input_to_get_total)
{
	int total = 0;
	uint8_t row = 0;
	for(row=0; row<TASKID_ARR_NROWS; row++)
	{
		total += input_to_get_total[row];
	}
	return total;
}

/*
 * @Function : nt_getupdatedTableMemory
 * @description: for printing updated table
 * @parm     : Null.
 * @return   : NULL
 */

void nt_getupdatedTableMemory(void)
{
	char buffer[ 200 ];
	int row;
	char summary[15]="Total Summary";
/* place holders to calculate maximum and total values */
	int total_num_malloc = 0;
	int total_num_free = 0;
	int total_current_usage = 0;
	int total_max_usage = 0;
	int num_malloc_free_diff = 0;
	int total_curr_step_usage = 0;
	uint32_t TotalHeapSize = (HEAP_END_ADDR) - (uint32_t)&_ln_RAM_addr_heap_start__ ;

	total_current_usage = nt_get_total(CurrentUsage);
	total_curr_step_usage = nt_get_total(StepCurrentUsage);
	total_num_malloc = nt_get_total(NumMalloc);
	total_num_free = nt_get_total(NumFree);
	total_max_usage = nt_get_total(StepMaxUsage);
	num_malloc_free_diff = total_num_malloc-total_num_free;

	for( row = 0; ptr_name[row]->taskName[0]; row++)
	{
		snprintf(buffer,sizeof(buffer),"%s  \t%d\t\t%d\t\t%d\t\t%d\t\t%d\t \t%d\t%d\t%d\r\n",
		&ptr_name[row]->taskName[0],CurrentUsage[ row ],MaxUsageByTasks[ row ],StepCurrentUsage[row],StepMaxUsage[row],
		MaxMalloc[ row ],NumMalloc[ row ],NumFree[ row ], ulArrTaskId[row]);
		nt_dbg_print(buffer);
	}

	nt_dbg_print("----------------------------------------------------------------------------------------------------------------------\r\n");
	memset(buffer, 0, 200);
	snprintf(buffer,sizeof(buffer),"%s\t%d\t\tNA\t\t%d\t\t%d\t\tNA\t\t%d\t%d\r\n", summary, total_current_usage,  total_curr_step_usage, total_max_usage, total_num_malloc,  total_num_free);
	nt_dbg_print(buffer);

	memset(buffer, 0, 200);
	snprintf(buffer,sizeof(buffer), "\r\nTotal Heap Size : %d\r\n", (int)TotalHeapSize);
	nt_dbg_print(buffer);
#if (NT_FN_QC_HEAP == 0)
	memset(buffer, 0, 200);
	snprintf(buffer,sizeof(buffer), "\r\nRemaining bytes : %d\r\n", xPortGetFreeHeapSize());
	nt_dbg_print(buffer);
#endif
#if (NT_FN_QC_HEAP == 1 || NT_FN_QC_HEAP == 2)
	memset(buffer, 0, 200);
	snprintf(buffer,sizeof(buffer), "\r\nRemaining bytes : %d\r\n", (int)(TotalHeapSize - total_current_usage));
	nt_dbg_print(buffer);
#endif
	memset(buffer, 0, 200);
	snprintf(buffer,sizeof(buffer), "\r\nNumber of Malloc/Free difference : %d\r\n", num_malloc_free_diff);
	nt_dbg_print(buffer);

	/* Printing Max Heap Usage Ever */
	memset(buffer, 0, 200);
	snprintf(buffer,sizeof(buffer), "\r\nMax Heap Usage : %d\r\n", max_total_usage);
	nt_dbg_print(buffer);
#if (NT_FN_QC_HEAP == 2)
	char pcWriteBuffer[200];
	snprintf((char *) pcWriteBuffer,sizeof(pcWriteBuffer), "%s","BIN no\tmalloc\tfreed\r\n");
	nt_dbg_print(pcWriteBuffer);
	int i = 1 ;
	for(i = 0 ; i <16;i++)
	{
	  snprintf((char *) pcWriteBuffer,sizeof(pcWriteBuffer), "%d\t%d\t%d\r\n",i,used_bin[i],free_bin[i]);
	  nt_dbg_print(pcWriteBuffer);
	}
#endif
#ifdef CONFIG_NT_RCLI
	nt_dbg_print("<eWiFiSuccess>");
#endif

}

/*
 * @Function: nt_table_update_free
 * @description: calculate the Current freed and how many freed.
 *
 * @parm    : mbytes : free size.
 *            SchedulerState : for stared or not
 *            ulTaskId       : Current task id
 *  @return : NULL;
 */
 void nt_table_update_free(int mbytes,UBaseType_t SchedulerState,UBaseType_t ulTaskId )
{
	char* TaskName = NULL;
	uint8_t row;
	//BaseType_t SchedulerState = NULL;

	//SchedulerState = xTaskGetSchedulerState();

	/* The task id is invalid before the start of the scheduler. The memory free before the vTaskStartScheduler will be
	 * stored inside the if condition. Once the scheduler is started, every task have their unique Task IDs which will be used to
	 * update the information i.e in the else condition. */

	if(SchedulerState == taskSCHEDULER_NOT_STARTED)
	{
		NumFree[0] += 1;
		CurrentUsage[0] -= mbytes;
	}
	else
	{
		TaskName = qurt_thread_get_name(NULL);
		for(row = 0; row < TASKID_ARR_NROWS; row++)
		{
			if(ulArrTaskId[ row ] == (int)ulTaskId)
			{
				NumFree[ row ] += 1;
				CurrentUsage[ row ] -= mbytes;
				StepCurrentUsage[row] -= mbytes;
				break;
			}
		}
		if(row >=TASKID_ARR_NROWS )
		{
			for(row = 0; row < TASKID_ARR_NROWS; row++)
			{
				if(ptr_name[ row ]->taskName[0] == '\0')
				{
					memcpy(&ptr_name[row]->taskName[0],TaskName,NT_MAX_TASK_NAME_LENGTH);
					NumFree[ row ] = 1;
					CurrentUsage[ row ] -= mbytes;
					StepCurrentUsage[row] -= mbytes;
					NumMalloc[ row ] = 0;
					MaxMalloc[ row ] = 0;
					break;
				}
			}
		}
	}
}

 /*
  * @Function: nt_getClearTableMemory
  * @description: clear the stepMaxUsage and stepCurrentUsage and NumMalloc and NumFrees.
  *
  * @parm    : NULL;
  * @return : NULL;
  */

void nt_getclearTableMemory(void)
{
	int row;
	for(row = 0; row < TASKID_ARR_NROWS; row++)
	{
//		CurrentUsage[ row ] = 0;
		StepMaxUsage[ row ] = 0;
		StepCurrentUsage[row] = 0;
//		MaxMalloc[ row ] = 0;
		NumMalloc[ row ] = 0;
		NumFree[ row ] = 0;
	}
	MaxTotalUsagePerStep = 0;
	CurrTotalUsagePerStep = 0;

}
#endif /** NT_TU_HEAP_STATS */
