/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/



#ifndef OS_INC_NT_OSAL_H_
#define OS_INC_NT_OSAL_H_

#include "FreeRTOS.h"
#include <string.h>
#include "autoconf.h"
#include "projdefs.h"
#include "task.h"
#include "timers.h"
#include "queue.h"
#include "semphr.h"
#include "time.h"
#include "FreeRTOSConfig.h"
#include "portmacro.h"
#include "qurt_internal.h"
/******************************MACRO DEFINITION***************************/
#define NT_OSAL_VERSION         0x00001     /* OSAL version definition 					*/
#define NT_OSAL_NAME			"freeRTOS"	/* Internally which OS we are using right now 	*/

#ifndef CONFIG_HEAP_STATISTIC
extern void * pvPortCalloc(size_t xNum, size_t xSize);
#ifdef DEBUG_MEM_LEAK
extern void * pvPortCallocWrapper(size_t xNum, size_t xSize, const char *caller);
#endif
#endif


/*Thread ID identifies the thread*/
typedef TaskHandle_t nt_osal_task_handle_t;

/*Message Queue ID identifies the message queue*/
typedef QueueHandle_t nt_osal_queue_handle_t;

/*Semaphore ID identifies the semaphore*/
typedef SemaphoreHandle_t nt_osal_semaphore_handle_t;

/*Timer ID identifies the Timer*/
typedef TimerHandle_t nt_osal_timer_handle_t;

typedef TickType_t nt_osal_tick_type_t;


/************************Timer return value defs**************************/
#define NT_TIMER_SUCCESS    0
#define NT_TIMER_FAILURE    1

/******************************FREERTOS FUNCTION***************************/
#define nt_pass (pdPASS)
#define nt_fail (pdFAIL)

#if (NT_FN_QURT == 1)
#define NT_QUEUE_SUCCESS   (QURT_EOK)
#define NT_QUEUE_FAIL      (QURT_EFAILED_TIMEOUT)
#else
#define NT_QUEUE_SUCCESS   (pdPASS)
#define NT_QUEUE_FAIL      (pdFALSE)
#endif  /* (NT_FN_QURT == 1) */
#define NT_MS_TO_TICKS(ms) pdMS_TO_TICKS(ms)
/*Create New Task/Thread and send the handler to caller*/
#if(NT_FN_QURT == 1)
BaseType_t nt_osal_thread_create(	TaskFunction_t pxTaskCode,
		const char * const pcName,		/*lint !e971 Unqualified char types are allowed for strings and single characters only. */
		const configSTACK_DEPTH_TYPE usStackDepth,
		void * const pvParameters,
		UBaseType_t uxPriority,
		TaskHandle_t * const pxCreatedTask );
#else
#define nt_osal_thread_create(ptr_function, name, stack_size, argument, priority, handler)\
		xTaskCreate(ptr_function, name, stack_size, argument, priority, handler)
#endif //NT_FN_QURT

/*Delete the task*/
#define nt_osal_thread_delete(handler) \
		vTaskDelete(handler);

/*Create and Initialize a Message Queue*/
#if(NT_FN_QURT == 1)
QueueHandle_t nt_osal_queue_create( const UBaseType_t uxQueueLength, const UBaseType_t uxItemSize );
#else
#define nt_osal_queue_create(queue_size,item_size)\
         xQueueCreate(queue_size,item_size)
#endif //NT_FN_QURT

/*Put a Message to a Queue*/

#if(NT_FN_QURT == 1)
#define nt_osal_queue_send(queue_id, message, tick_to_wait)\
		qurt_pipe_send_timed(queue_id,message,tick_to_wait)
#else
#define nt_osal_queue_send(queue_id, message, tick_to_wait)\
		xQueueSend(queue_id,message,tick_to_wait)
#endif //NT_FN_QURT

/*Put a Message to a Queue From ISR*/
#if(NT_FN_QURT == 1)
#define nt_osal_queue_send_from_isr(queue_id, message, tasktoken) \
		qurt_pipe_try_send(queue_id, message, tasktoken)
#else
#define nt_osal_queue_send_from_isr(queue_id, message, tasktoken) \
		xQueueSendFromISR(queue_id, message, tasktoken)
#endif

#if(NT_FN_QURT == 1)
#define nt_osal_queue_msg_receive(queue_handle, msg_buffer, block_time) \
		qurt_pipe_receive_timed(queue_handle, msg_buffer, block_time)
#else
#define nt_osal_queue_msg_receive(queue_handle, msg_buffer, block_time) \
		xQueueReceive(queue_handle, msg_buffer, block_time)
#endif

#if(NT_FN_QURT == 1)
#define nt_osal_queue_msg_receive_from_isr(queue_id, message, tasktoken) \
	qurt_pipe_try_receive(queue_id, message,tasktoken)
#else
#define nt_osal_queue_msg_receive_from_isr(queue_handle, msg_buffer, block_time) \
	xQueueReceiveFromISR(queue_handle, msg_buffer,block_time)
#endif

/* Delete the queue */
#define nt_osal_queue_delete(queue_id) \
               vQueueDelete(queue_id);

/*Start the RTOS Kernel*/
#define nt_osal_kernel_start() \
		vTaskStartScheduler()

/*Wait for Timeout (Time Delay)*/
#if(NT_FN_QURT == 1)
#define nt_osal_delay(mil_sec) \
		qurt_thread_sleep(mil_sec)
#else
#define nt_osal_delay(mil_sec) \
		vTaskDelay(mil_sec)
#endif

/*Create and Initialize a Semaphore object*/
#define nt_osal_semaphore_create_binary(semaphore_id) \
		vSemaphoreCreateBinary(semaphore_id)

/*Wait until a Semaphore token becomes available*/
#define nt_osal_semaphore_take(semaphore_id, block_time) \
		xSemaphoreTake(semaphore_id, block_time)

/*Release a Semaphore token*/
#define nt_osal_semaphore_give(semaphore_id) \
		xSemaphoreGive(semaphore_id)\

/* Delete the semaphore */
#define nt_osal_semaphore_delete(semaphore_id) \
               vSemaphoreDelete(semaphore_id)

/* Create Mutex */
#define nt_osal_create_mutex() \
		xSemaphoreCreateMutex()

/*Create Timer*/
#if(NT_FN_QURT ==1)
TimerHandle_t nt_osal_timer_create(	char * pcTimerName,
		const TickType_t xTimerPeriodInTicks,
		const UBaseType_t uxAutoReload,
		void *  pvTimerID,//const
		TimerCallbackFunction_t pxCallbackFunction );
#else
#define nt_osal_timer_create(timer_name,timer_period,auto_reload,timer_id,target_task) \
		xTimerCreate(timer_name,timer_period,auto_reload,timer_id,target_task)
#endif
/*Create Timer*/
#if(NT_FN_QURT ==1)
int nt_osal_timer_start(nt_osal_timer_handle_t timer_handle , nt_osal_tick_type_t block_time);
int nt_osal_timer_start_frm_isr(nt_osal_timer_handle_t timer_handle , long *const port_yield);
int nt_osal_get_expiry_time(nt_osal_timer_handle_t timer_handle);
int nt_osal_timer_stop(nt_osal_timer_handle_t timer_handle , nt_osal_tick_type_t block_time);
int nt_osal_delete_timer(nt_osal_timer_handle_t timer_handle , nt_osal_tick_type_t block_time);
int nt_osal_timer_period_change(nt_osal_timer_handle_t timer_handle , nt_osal_tick_type_t timer_period, nt_osal_tick_type_t block_time);
#define nt_osal_get_ticks() \
		xTaskGetTickCount()
size_t  strnscat(char  *dst, size_t  dst_size, const char  *src, size_t src_size);
size_t  memscpy(void *dst,size_t dst_size,const void *src,size_t src_size);
#else

/*Start Timer*/
#define nt_osal_timer_start(timer_handle, block_time) \
        xTimerStart(timer_handle, block_time)

/*Stop Timer*/
#define nt_osal_timer_stop(timer_handle, block_time) \
        xTimerStop(timer_handle, block_time)

/*delete Timer*/
#define nt_osal_delete_timer(timer_handle, block_time) \
        xTimerDelete(timer_handle, block_time)

/*Get Tick Counts*/
#define nt_osal_get_ticks() \
		xTaskGetTickCount()
/*Start Timer from isr*/
#define nt_osal_timer_start_frm_isr(timer_handle, task_priority) \
		xTimerStartFromISR(timer_handle, task_priority)
#endif //NT_FN_QURT
/*Start Timer from isr*/
#define nt_osal_timer_start_from_isr(timer_handle, task_priority) \
		xTimerStartFromISR(timer_handle, task_priority)

/*Stop Timer from isr*/
#define nt_osal_stop_timer_from_isr(timer_handle, task_priority) \
	xTimerStopFromISR(timer_handle, task_priority)

#define nt_osal_higher_priority_task_woken xHigherPriorityTaskWoken

#define nt_osal_yield_from_isr(nt_osal_higher_priority_task_woken)\
portYIELD_FROM_ISR(nt_osal_higher_priority_task_woken)


/*Change Time Period*/
#define nt_osal_timer_change_period(timer_handle, period) \
        xTimerChangePeriod(timer_handle, period, portMAX_DELAY)

#ifdef CONFIG_HEAP_STATISTIC
#include "qc_heap.h"

#ifndef pvPortCalloc
extern void *__pvPortCalloc( size_t xNum, size_t xSize );

#define pvPortCalloc(n, size) (({heap_statistics[heap_statistics_index%NT_HEAP_RCD_CNT].function=__FUNCTION__; \
                                            heap_statistics[heap_statistics_index%NT_HEAP_RCD_CNT].req_size=n*size; \
                                            heap_statistics_index++;}) , \
                                         (__pvPortCalloc(n, size)))
#endif

#endif

/*allocating heap memory*/
#ifndef DEBUG_MEM_LEAK
#define nt_osal_allocate_memory(size) \
		pvPortMalloc(size)
#else
#define nt_osal_allocate_memory(size) \
		pvPortMallocWrapper(size, __FUNCTION__)
#endif

/*Release allocated memory*/
#ifndef DEBUG_MEM_LEAK
#define nt_osal_free_memory(ptr) \
		vPortFree(ptr)
#else
#define nt_osal_free_memory(ptr) \
		pvPortFreeWrapper(ptr,__FUNCTION__)
#endif

/* Calloc */
#ifndef DEBUG_MEM_LEAK
#define nt_osal_calloc(count, size) \
		pvPortCalloc(count, size)
#else
#define nt_osal_calloc(count, size) \
		pvPortCallocWrapper(count, size,__FUNCTION__)
#endif



/*get timer id*/
#define nt_osal_get_timer_id(timer_handle) \
		pvTimerGetTimerID(timer_handle)

#define nt_osal_get_expiry_time(timer_handle) \
		xTimerGetExpiryTime(timer_handle)


/*to check is timer active or not*/
#define nt_osal_is_timer_active(timer_handle) \
 xTimerIsTimerActive(timer_handle)

/*to get timer period*/
#define nt_osal_get_time_period(timer_handle) \
		xTimerGetPeriod(timer_handle)

#define nt_osal_get_current_task_handle() \
		pcTaskGetName(NULL)
/**
 *  Updates the mode of a software timer to be either an auto reload timer or a one-shot timer.
 *  @param  timer_handle The handle of the timer to update.
 *  @param auto_reload pdTRUE to set the timer into auto reload mode, or pdFALSE to set the timer into one shot mode.
 */
#define nt_osal_timer_set_reload_mode(timer_handle, auto_reload) \
		vTimerSetReloadMode( timer_handle, auto_reload )

/** convert ms to ticks */
#define nt_ms_to_ticks(ms)  pdMS_TO_TICKS(ms)

/*create semaphore*/
#define nt_osal_xsemaphore_create_binary() \
	xSemaphoreCreateBinary()

/*Release a Semaphore token from isr*/
#define nt_osal_semaphore_give_from_isr(semaphore_id, target_task) \
 xSemaphoreGiveFromISR(semaphore_id, target_task)

#define nt_osal_get_param(pcCommandString, uxWantedParameter, pxParameterStringLength) \
	FreeRTOS_CLIGetParameter(pcCommandString, uxWantedParameter, pxParameterStringLength)

/**
 * <!-- nt_normal_delay -->
 *
 * @brief Normal delay function
 * @param time: Delay time needed in milli seconds
 * @return: void
 */
void nt_normal_delay(uint32_t time);

#endif /* OS_INC_NT_OSAL_H_ */
