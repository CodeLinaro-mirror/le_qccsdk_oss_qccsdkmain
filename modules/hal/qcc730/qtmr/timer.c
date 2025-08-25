/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*========================================================================
 *
 * @file  timer.c
 * @brief Timer Infrastructure.
 *========================================================================*/

/*-------------------------------------------------------------------------
 * Include Files
 * ----------------------------------------------------------------------*/

#include <stdlib.h>
#include <stdbool.h>
#include "timer.h"
#include "nt_osal.h"
#include "err.h"
#include "timer_internal.h"
#include "qtmr.h"
#include "nt_logger_api.h"

#if defined(SUPPORT_HIGH_RES_TIMER)

/*-------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * ----------------------------------------------------------------------*/

#define TIMER_CHECK_VALID_TASK 1
#define TIMER_TASK_QUEUE_LEN   5
#define TIMER_TASK_PRIORITY    (configMAX_PRIORITIES - 1)
#define TIMER_TASK_STACK_SIZE  400
#define MAX_QUEUE_WAIT         3000
#define TIMER_MAX_EXPIRY_TICKS 0xFFFFFFFFFFFFFFFF
#define MAX_TIMERS             8
#define NO_WAIT                0
#define TIMER_TICKS_LO_BITS(x) (uint32_t)((uint32_t)(x)&0xFFFFFFFFu)
#define TIMER_TICKS_HI_BITS(x) (uint32_t)(((uint32_t)((x) >> 32)) & 0x00FFFFFFu)

/*-------------------------------------------------------------------------
 * Global Data Definitions
 * ----------------------------------------------------------------------*/

/*-------------------------------------------------------------------------
 * Static Variable Definitions
 * ----------------------------------------------------------------------*/

static nt_osal_semaphore_handle_t timer_mutex = NULL;
static nt_osal_semaphore_handle_t log_mutex = NULL;
static time_timetick_type ticks_now = 0;
static bool b_timer_initialized = FALSE;
static timer_internal_type *ptr_timer_buffer = NULL;
static uint32_t timers_max = 0;
static bool timer_error_fatal_on_redefinition = FALSE;
nt_osal_task_handle_t timer_task_handle = NULL;
static nt_osal_queue_handle_t timer_task_queue = NULL;
/* Internal timer */
#if defined(HRES_TIMER_PROFILING)
static timers_type timers = {{NULL, NULL}, 0, FALSE, TRUE, 0, 0, 0, 0};
static timer_msg_t delay_dump[MAX_DUMP_LEN];
static uint8_t dump_idx = 0;
#else
static timers_type timers = {{NULL, NULL}, 0, FALSE, TRUE};
#endif

/*-------------------------------------------------------------------------
 * Function Definitions
 * ----------------------------------------------------------------------*/

/**
 * timer_cvt_to_tick64
 *
 * @brief Converts time to ticks
 *
 * @param time Timer to convert
 * @param unit Time unit type
 * @param pTimeRet Pointer to store the result
 *
 * @return Success or failure after conversion
 */
int8_t timer_cvt_to_tick64(uint64_t time, time_unit_type unit, uint64_t *pTimeRet)
{
    uint64_t cntr_freq_hz = TIMER_GET_FRQ();

    switch (unit) {
        case T_SEC:
            /* Seconds to ticks */
            time *= cntr_freq_hz;
            break;

        case T_MSEC:
            /* Milliseconds to ticks */
            time = (time * cntr_freq_hz) / 1000;
            break;

        case T_USEC:
            /* Microseconds to ticks */
            time = ((time * cntr_freq_hz) / 1000000);
            break;

        case T_TICK:
            /* Time already in tick */
            break;

        default:
            NT_LOG_PRINT(SYSTEM, CRIT,
                         "timer_cvt_to_tick64: Invalid timetick \
                            conversion %d",
                         unit);
            return -1;
    }

    *pTimeRet = time;

    return 0;

} /* timer_cvt_to_tick64 */

/**
 * timer_cvt_from_tick64
 *
 * @brief Converts ticks to time unit
 *
 * @param time Time in ticks to convert
 * @param unit Time unit type
 * @param pTimeRet Pointer to store the result
 *
 * @return Returns 0 on success, -1 on failure
 */
int8_t __attribute__((section(".__sect_ps_txt")))
timer_cvt_from_tick64(uint64_t time, time_unit_type unit, uint64_t *pTimeRet)
{
    uint64_t cntr_freq_hz = TIMER_GET_FRQ();

    switch (unit) {
        case T_SEC:
            /* Convert ticks to seconds */
            time /= cntr_freq_hz;
            break;

        case T_MSEC:
            /* Convert ticks to milliseconds */
            time = (time * 1000) / cntr_freq_hz;
            break;

        case T_USEC:
            /* Convert ticks to microseconds */
            time = (time * 1000000) / cntr_freq_hz;
            break;

        case T_TICK:
            /* Time already in tick */
            break;

        default:
            NT_LOG_PRINT(SYSTEM, CRIT,
                         "timer_cvt_from_tick64: Invalid timetick \
                            conversion %d",
                         unit);
            return -1;
    }

    *pTimeRet = time;

    return 0;

} /* timer_cvt_from_tick64 */

/**
 * timer_index_to_handle
 *
 * @brief Converts given index to handle
 *
 * @param index Index of the timer structure buffer
 *
 * @return Returns the handle value for a given index
 */
static uint32_t timer_index_to_handle(uint32_t index)
{
    uint32_t handle = 0;
    uint16_t t_index = index & TIMER_INDEX_MASK;
    uint16_t check = t_index ^ TIMER_MAGIC;
    handle = (uint32_t)(check << TIMER_INDEX_BITS | t_index);
    return handle;
}

/**
 * timer_handle_to_index
 *
 * @brief Converts given handle to index
 *
 * @param handle Timer handle
 * @param index Pointer to hold the result
 *
 * @return Returns whether the index is valid or not
 */
static bool timer_handle_to_index(uint32_t handle, uint32_t *index)
{
    uint16_t check = (handle >> TIMER_INDEX_BITS) & TIMER_INDEX_MASK;
    uint16_t t_index = (handle & TIMER_INDEX_MASK);
    uint16_t t_index_check = t_index ^ TIMER_MAGIC;

    if (t_index_check != check || t_index >= timers_max) {
        return FALSE;
    }

    *index = t_index;

    return TRUE;
}

/**
 * hres_timer_timetick_get
 *
 * @brief This function gets the timetick and stores the value in global variable
 * Timer module should be initialized before calling this function
 *
 * @return timetick value
 */
time_timetick_type __attribute__((section(".__sect_ps_txt"))) hres_timer_timetick_get(void)
{
    ticks_now = TIMER_GET_TIME64();
    return ticks_now;
}

/**
 * timer_set_next_interrupt
 *
 * @brief This function sets the next timer interrupt.
 * Timer module should be initialized before calling this function
 *
 * @param match_count Clock count at which next interrupt will occur
 * @param is_mv_intentional If TRUE, forces MATCH_VAL register to be updated
 * @param caller Event responsible for calling this function
 */
static void timer_set_next_interrupt(time_timetick_type match_count, timer_match_interrupt_setter_type caller)
{
    /* Don't re-write the same value to the MATCH_VAL register */
    if (match_count != timers.match_value) {
        timers.match_value = match_count;
        TIMER_SET_TRIGGER64(match_count);
#if defined(HRES_TIMER_PROFILING)
        /* Record when this value was actually written */
        timers.set_time = ticks_now;
#endif
    }

    NT_LOG_PRINT(SYSTEM, INFO, "Next Timer Interrupt Set %d", caller);

/*Need to enable the timer ISR again irrespective of "match_count != timers.match_value"
  but enable it only after we program new match value(did above) otherwise the ISR may fire*/
#ifndef QTMR_DRV
    TIMER_ENABLE(TRUE);
#endif

} /* timer_set_next_interrupt */

/**
 * timer_update_timer_interrupt
 *
 * @brief This function updates the timer interrupt
 * This function should be called within an INTLOCK
 * Timer module should be initialized before calling this function
 *
 * @param caller Event responsible for calling this function
 */
static void timer_update_timer_interrupt(timer_match_interrupt_setter_type caller)
{
    /* Current clock count */
    time_timetick_type now;

    /* Time of first expiring timer */
    time_timetick_type first;

    /* NOTE: This function has many early returns. */

    /* FALSE == timers.do_reprogram_isr
       Skip updating the interrupts if not allowed.
       TRUE == Timers.processing
       If a timer is being altered inside "timer_process_active_timers"
       Don't bother to program the timer interrupt as it will be programmed
       when processing of timers is done */
    if (FALSE == timers.do_reprogram_isr || TRUE == timers.processing) {
        return;
    }

    /* Get the current time */
    now = hres_timer_timetick_get();

    /* Are there timers on the timer list? */
    if (timers.active.first != NULL) {
        /* Get the time of the first expiring timer */
        first = timers.active.first->expiry;

        /* If the first expiring timer matches the timer.match_value, ... */
        if (first == timers.match_value) {
            /* timer interrupt already properly programmed */
            return;
        }

        if (first < now) {
            /* Set the timer for "as soon as possible" (eg, "now") */
            first = now;
        }
        timer_set_next_interrupt(first, caller);
    }
#if defined(SUPPORT_ROOTCLK_DISABLE)
    else {
        TIMER_SET_ROOTCLK(FALSE);
    }
#endif

} /* timer_update_timer_interrupt */

/**
 * timer_remove
 *
 * @brief Removes the timer from the active list
 * This function needs to be called under INTLOCK
 *
 * @param timer_internal Timer to be removed from list of active timers
 *
 * @return timer_error_type
 */
static timer_error_type timer_remove(timer_internal_ptr_type timer_internal)
{
    if (timer_internal == NULL || timer_internal->list == NULL || timer_internal->list != &timers.active) {
        return TE_INVALID_TIMER_HANDLE;
    }

    /* Fix up links/list around this node */
    if (timer_internal->prev) {
        timer_internal->prev->next = timer_internal->next;
    } else {
        timer_internal->list->first = timer_internal->next;
    }

    if (timer_internal->next) {
        timer_internal->next->prev = timer_internal->prev;
    } else {
        timer_internal->list->last = timer_internal->prev;
    }

    timer_internal->next = NULL;
    timer_internal->prev = NULL;
    timer_internal->list = NULL;

    return TE_SUCCESS;

} /* timer_remove */

/**
 * timer_insert_between
 *
 * @brief Inserts the timer between two specified timers
 * This function needs to be called under INTLOCK
 *
 * @param list List to insert timer into
 * @param timer_internal Timer to insert into list
 * @param ptr1 Timer to insert after
 * @param ptr2 Timer to insert before
 */
static void timer_insert_between(timer_list_ptr list, timer_internal_ptr_type timer_internal,
                                 timer_internal_ptr_type ptr1, timer_internal_ptr_type ptr2)
{
    /* Update our timer's prev/next ptrs to point at correct timer structs */

    timer_internal->prev = ptr1;
    timer_internal->next = ptr2;

    /* Update surrounding prev/next ptrs (if necessary) to point to our
       newly inserted timer */

    if (ptr1 != NULL) {
        ptr1->next = timer_internal;
    } else {
        list->first = timer_internal; /* We have a new start of list, update first ptr */
    }

    if (ptr2 != NULL) {
        ptr2->prev = timer_internal;
    } else {
        list->last = timer_internal; /* We have a new end of list, update last ptr */
    }

} /* timer_insert_between */

/**
 * timer_insert
 *
 * @brief Inserts the timer into active list
 * This function needs to be called under INTLOCK
 *
 * @param list List timer is to be inserted into
 * @param timer_internal Timer to be inserted into list of active timers
 */
static void timer_insert(timer_list_ptr list, timer_internal_ptr_type timer_internal)
{
    /* Pointer to a timer. Used for walking list of timers */
    timer_internal_ptr_type ptr;

    /* First set the list field of the timer */
    timer_internal->list = list;

    /* Search for appropriate list location to insert timer */
    ptr = list->first;
    while (ptr != NULL && (ptr->expiry <= timer_internal->expiry)) {
        ptr = ptr->next;
    }

    /* Insert the timer into the list */
    timer_insert_between(list, timer_internal, ptr ? ptr->prev : list->last, ptr);

} /* timer_insert */

/**
 * timer_get_client_timer
 *
 * @brief This function returns pointer to client timer.
 * This function needs to be called under INTLOCK
 *
 * @param timer Time handle
 *
 * @return Returns relevant pointer to client timer structure.
 * If handle is invalid returns NULL.
 */
static timer_internal_ptr_type timer_get_client_timer(timer_ptr_type timer)
{
    timer_internal_ptr_type temp_clnt = NULL;
    uint32_t index = TIMER_INVALID_HANDLE;

    if (timer == NULL) {
        return NULL;
    }

    if (!timer_handle_to_index(*timer, &index)) {
        return NULL;
    }

    temp_clnt = &ptr_timer_buffer[index];

    if (temp_clnt->info.timer_state < TIMER_DEFAULT_FLAG || temp_clnt->info.timer_state > TIMER_UNDEF_FLAG) {
        NT_LOG_PRINT(SYSTEM, INFO, "Timer got corrupted. timer_state 0x%x, index = 0x%x", temp_clnt->info.timer_state,
                     index);
    }

    /* Only if the client timer is in use,
       otherwise it may be stale Handle */
    if (temp_clnt->info.node_state == NODE_IS_NOT_FREE) {
        return temp_clnt;
    } else {
        return NULL;
    }

} /* timer_get_client_timer */

/**
 * timer_get_new_client_timer
 *
 * @brief Get New client timer from timer_client_chunks.
 *
 * @param satus TE_SUCCESS on success, else valid error type
 * @param handle Timer handle
 *
 * @return Returns relevant pointer to client timer structure.
 */
static timer_internal_ptr_type timer_get_new_client_timer(timer_error_type *status, uint32_t *handle)
{
    timer_internal_ptr_type clnt_timer = NULL;
    uint32_t idx;
    uint32_t temp_index = TIMER_INVALID_HANDLE;
    timer_internal_ptr_type buffer_ptr = NULL;

    if (status == NULL || handle == NULL) {
        return NULL;
    }

    *status = TE_FAIL;
    *handle = 0;

    buffer_ptr = &ptr_timer_buffer[0];

    for (idx = 0; idx < timers_max; idx++) {
        if (buffer_ptr[idx].info.node_state == NODE_IS_FREE) {
            break;
        }
    }

    if (idx < timers_max) {
        /* Get Client Timer */
        clnt_timer = &buffer_ptr[idx];

        /*store the temp info that we cannot lose*/
        temp_index = clnt_timer->index;

        /* memset to zero before giving for new client */
        memset(clnt_timer, 0, sizeof(timer_internal_type));

        /*restore the info*/
        clnt_timer->index = temp_index;
        clnt_timer->info.node_state = NODE_IS_NOT_FREE;
        *handle = timer_index_to_handle(idx);
        *status = TE_SUCCESS;
        return clnt_timer;
    } else {
        *status = TE_NO_FREE_INTERNAL_TIMER;
        ASSERT(0);
    }

} /* timer_get_new_client_timer */

/**
 * timer_determine_timer_expiry
 *
 * @brief Determines whether the timer callback can be called depending on the
 * state of the timer.
 * hres_timer_init should be called before calling this function.
 *
 * @param timer_internal Timer to to checked for expiry
 *
 * @return Returns TRUE if expiry is required, else FALSE
 */
static bool timer_determine_timer_expiry(timer_internal_ptr_type timer_internal)
{
    bool b_expire_timer = FALSE;

    /*Check for the integrity of the external timer state*/
    if (timer_internal->info.timer_state < TIMER_DEF_FLAG || timer_internal->info.timer_state > TIMER_UNDEF_FLAG) {
        NT_LOG_SYSTEM_ERR("External timer structure corrupted", 0, 0, 0);
    }

    /*Expire the timer only if the timer state is
      resumed or set*/
    if (TIMER_SET_FLAG == timer_internal->info.timer_state) {
        timer_internal->info.timer_state = TIMER_EXPIRED_FLAG;
        b_expire_timer = TRUE;
    }

    return b_expire_timer;
}

/**
 * timer_expire
 *
 * @brief  Calls the timer callback or sets the signal
 * hres_timer_init should be called before calling this function
 *
 * @param timer_internal Timer to be expired
 */
static void timer_expire(timer_internal_ptr_type timer_internal)
{
    task_info_ptr task_info;
    timer_msg_t timer_msg;

    task_info = timer_internal->task_info;
    timer_msg.handle = *timer_internal->external_timer;
    timer_msg.timer_cb = timer_internal->timer_callback;
#if defined(HRES_TIMER_PROFILING)
    timer_msg.set_time = timers.set_time;
    timer_msg.isr_start = timers.isr_start;
    timer_msg.isr_end = timers.isr_end;
    timer_msg.q_push_time = ticks_now;
#endif

    /*release mutex after backing up the expiry info*/
    if (nt_fail == nt_osal_semaphore_give(timer_mutex)) {
        NT_LOG_SYSTEM_ERR("Timer failed to give semaphore", 0, 0, 0);
    }

    if (task_info != NULL) {
        /*Insert into msg queue*/
        if (pdTRUE != xQueueSend(task_info->timer_queue, &timer_msg, MAX_QUEUE_WAIT)) {
            NT_LOG_SYSTEM_ERR("Posting timer msg timeout", 0, 0, 0);
        }

        /*Notify the caller task*/
        xTaskNotify(task_info->handle, (1 << task_info->event), eSetBits);
    }

    /*acquire mutex back after expiring  the timer*/
    if (nt_fail == nt_osal_semaphore_take(timer_mutex, portMAX_DELAY)) {
        NT_LOG_SYSTEM_ERR("Timer failed to take semaphore", 0, 0, 0);
    }
} /* timer_expire */

/**
 * timer_remove_head
 *
 * @brief Removes the first timer in the active list
 * This function should be called from intlock
 * hres_timer_init should be called before calling this function
 *
 * @param list List to chop head off of
 */
static void timer_remove_head(timer_list_ptr list)
{
    /* Head of the timer list */
    timer_internal_ptr_type head = list->first;

    /* New front of list */
    list->first = head->next;

    /* Fix up links/list around this node */
    if (list->first != NULL) {
        /* Update next node's prev ptr */
        list->first->prev = NULL;
    } else {
        /* Empty list */
        list->last = NULL;
    }

    /* NULL out deleted node's link fields */
    head->next = NULL;
    head->list = NULL;

} /* timer_remove_head */

/**
 * timer_free_internal_timer
 *
 * @brief Puts the timer back in free q
 * Must be called from inside INTLOCK.
 *
 * @param timer_internal Timer to be freed
 */
static void timer_free_internal_timer(timer_internal_ptr_type timer_internal)
{
    uint32_t temp_index;

    if (NULL == timer_internal || timer_internal->info.node_state == NODE_IS_FREE) {
        NT_LOG_SYSTEM_ERR("Removing internal timer which is not active.", 0, 0, 0);
        return;
    }

    /* Save index value in a temp var */
    temp_index = timer_internal->index;

    /* Clear the internal timer */
    memset(timer_internal, 0, sizeof(timer_internal_type));

    /* Restore timer index*/
    timer_internal->index = temp_index;
    timer_internal->info.node_state = NODE_IS_FREE;

} /* timer_free_internal_timer */

/**
 * timer_prep_for_set
 *
 * @brief Prepare the timer for setting
 * Must be called from inside INTLOCK
 *
 * @param timer Timer handle
 * @timer_internal_ptr_ptr Pointer to internal timer
 */
static void timer_prep_for_set(timer_ptr_type timer, timer_internal_ptr_type *timer_internal_ptr_ptr)
{
    timer_internal_ptr_type timer_internal = *timer_internal_ptr_ptr;
    timer_internal = timer_get_client_timer(timer);

    if (NULL == timer_internal || timer_internal->external_timer != timer) {
        NT_LOG_PRINT(SYSTEM, INFO, "Corruption in external timer handle. %imer = 0x%x", *timer);
        return;
    }

    /* Remove timer from timer list, if any */
    timer_remove(timer_internal);

    if (timer_internal != NULL) {
        timer_internal->info.timer_state = TIMER_SET_FLAG;
    }

    *timer_internal_ptr_ptr = timer_internal;
}

/**
 * timer_process_active_timers
 *
 * @brief Iterates through timer linked list and removes all expired timers
 * Then programs the next timer in the linked list.
 * hres_timer_init should be called before calling this function
 */
void timer_process_active_timers(void)
{
    /* Timer being processed */
    timer_internal_ptr_type timer_internal;

    /* Minimum advance required for reloading timer */
    time_timetick_type min_advance;

    /* Temporary value to compute the new expiry point */
    time_timetick_type new_expiry;

    /*to determine whether timer is to be expired or not*/
    bool b_expire_timer = FALSE;

    /* Lock interrupts while testing & manipulating the active timer list */
    if (nt_fail == nt_osal_semaphore_take(timer_mutex, portMAX_DELAY)) {
        NT_LOG_SYSTEM_ERR("Timer failed to take semaphore", 0, 0, 0);
    }

    /* Record the fact that timers are being processed, to prevent re-entry
       into this function, and unnecessary modification of TIMETICK_MATCH. */
    timers.processing = TRUE;

    /* Get current clock count */
    hres_timer_timetick_get();

    /* Check to determine if the timer at head of the active timer list has
       past its expiry point.  If it has, remove it, expire the timer, and
       repeat with the new timer at the head of active timer list.*/
    while (timer_internal = timers.active.first,
           timer_internal != NULL && timer_internal->expiry <= ticks_now + TIMER_EARLY_TOL) {
        /* Remove expiring timer from active timer list */
        timer_remove_head(&timers.active);

        /* Reactivate timer, if required */
        if (TRUE == timer_internal->info.reload) {
            /* Determine how late the timer expired; this is the minimum
               amount the timer must be advanced by for the next expiry. */
            min_advance = ticks_now - timer_internal->expiry;

            new_expiry = timer_internal->expiry + timer_internal->duration_sclk;

            if (new_expiry <= ticks_now) {
                /* Temporary value to compute the new expiry point */
                new_expiry = timer_internal->expiry;

                /* Timer expired 1 or more reload period ago.  This can happen if
                   the timer belongs to a timer group which gets disabled, such as
                   the default timer groups during sleep. */

                /* Round min_advance up to the next multiple of reload periods. */
                min_advance += timer_internal->duration_sclk - min_advance % timer_internal->duration_sclk;

                /* Add the rounded-up minimum advance to the timer expiry */
                new_expiry += min_advance;
            }

            /* Check to make sure that the new expiry point is further in the future
               than the old one.  This prevents the cases where overflow in the
               calculation could occur or wrap around past the active timer list
               zero. */

            if (new_expiry > timer_internal->expiry) {
                /* New expiry point is further in the future than the old one, use it */
                timer_internal->expiry = new_expiry;
            } else {
                /* Move the expiry point as far out as possible */
                timer_internal->expiry = ticks_now - 1;
            }

            /* Record the new start time for the next expiry */
            timer_internal->start = ticks_now;

            /* Insert timer back in active list */
            timer_insert(&timers.active, timer_internal);

            b_expire_timer = timer_determine_timer_expiry(timer_internal);

            if (timer_internal) {
                timer_internal->info.timer_state = TIMER_SET_FLAG;
            }

            if (b_expire_timer) {
                /* Expire timer */
                timer_expire(timer_internal);
            }

        } else {
            b_expire_timer = timer_determine_timer_expiry(timer_internal);

            if (b_expire_timer) {
                /* Expire timer */
                timer_expire(timer_internal);
            }
        }

        /* Get current slow clock count */
        hres_timer_timetick_get();

    } /* while timers on timer.active.list are expiring */

    /* Timers that expire at and before "ticks_now" have been processed.
       Set interrupt for when next timer expires. */
    if (timers.active.first != NULL) {
        /* The first timer on the active list is the timer next to expire */
        timer_set_next_interrupt(timers.active.first->expiry, TIMER_MVS_TIMER_PROCESSED);
    }
#if defined(SUPPORT_ROOTCLK_DISABLE)
    else {
        TIMER_SET_ROOTCLK(FALSE);
    }
#endif

    /* Timer processing has completed */
    timers.processing = FALSE;

    /* We've finished manipulating the active timer list.  */
    if (nt_fail == nt_osal_semaphore_give(timer_mutex)) {
        NT_LOG_SYSTEM_ERR("Timer failed to give semaphore", 0, 0, 0);
    }

} /* timer_process_active_timers */

/**
 * timer_adjust_active_list
 *
 * @brief Adjust the active timer list
 *
 * @param adjust_tick Time to be adjusted in ticks
 */
static void timer_adjust_active_list(time_timetick_type adjust_tick)
{
    timer_internal_ptr_type ptimer = NULL;
    time_timetick_type new_time = 0;

    hres_timer_timetick_get();

    if (nt_fail == nt_osal_semaphore_take(timer_mutex, portMAX_DELAY)) {
        NT_LOG_SYSTEM_ERR("Timer failed to take semaphore", 0, 0, 0);
    }

    for (ptimer = timers.active.first; ptimer != NULL; ptimer = ptimer->next) {
        /* update the list here */
        if (ptimer->duration_sclk > adjust_tick) {
            new_time = ticks_now + (ptimer->duration_sclk - adjust_tick);
        } else {
            new_time = ticks_now;
        }
        ptimer->expiry = new_time;

    } /* for all timers on timer.active.list are expiring */
#if defined(HRES_TIMER_PROFILING)
    /* Since timer started from 0 since wakeup, now is the time taken for
       adjusting the timer list */
    timers.sleep_adjust_time = ticks_now;
#endif

    timers.do_reprogram_isr = TRUE;

    if (nt_fail == nt_osal_semaphore_give(timer_mutex)) {
        NT_LOG_SYSTEM_ERR("Timer failed to give semaphore", 0, 0, 0);
    }

} /* timer_adjust_active_list */

/**
 * timer_task
 *
 * @brief Timer task entry point
 * Handles timer expiry events and sleep time adjustments
 */
static void timer_task(void __attribute__((__unused__)) * pvParameters)
{
    /* Signal the task in order to prime the timer processing mechanism */
    BaseType_t xResult;
    time_timetick_type timer_msg = 0;
    time_timetick_type sleep_duration = 0;
    uint32_t notified_value = 0;

    for (;;) {
        xResult = xTaskNotifyWait(pdFALSE, ULONG_MAX, &notified_value, portMAX_DELAY);
        if (xResult == pdPASS) {
            /* Process sleep adjustment */
            if ((notified_value & (1 << TIMER_EVENT_PM_WAKEUP)) != 0) {
                while (pdTRUE == xQueueReceive(timer_task_queue, &timer_msg, NO_WAIT)) {
                    /* add the total sleep ticks*/
                    sleep_duration += timer_msg;
                }
                if (sleep_duration > 0) {
                    timer_adjust_active_list(sleep_duration);
                    notified_value |= 1 << TIMER_EVENT_EXPIRY_FROM_ISR;
                }
            }
            /* Process expiring timer(s) */
            if ((notified_value & (1 << TIMER_EVENT_EXPIRY_FROM_ISR)) != 0) {
                timer_process_active_timers();
            }
#if defined(HRES_TIMER_UNIT_TEST)
            /*
             * Important Note: This code is only for testing purpose. IRQ for frame 1-4 should not
             * be handled here!
             */
            if ((notified_value & (1 << TIMER_EVENT_QTMR_FRAME_2)) != 0) {
                NT_LOG_PRINT(SYSTEM, CRIT, "qtimer frame #2 irq");
            }
            if ((notified_value & (1 << TIMER_EVENT_QTMR_FRAME_3)) != 0) {
                NT_LOG_PRINT(SYSTEM, CRIT, "qtimer frame #3 irq");
            }
            if ((notified_value & (1 << TIMER_EVENT_QTMR_FRAME_4)) != 0) {
                NT_LOG_PRINT(SYSTEM, CRIT, "qtimer frame #4 irq");
            }
#endif
        }
    }

} /* timer_task */

/**
 * timer_task_init
 *
 * @brief This is the init function for timer task
 *
 * @return TE_SUCCESS on successfull init, else TE_FAIL
 */
uint32_t timer_task_init(void)
{
    uint32_t status = TE_SUCCESS;

    if (pdPASS != nt_qurt_thread_create(timer_task, "hres_timer", TIMER_TASK_STACK_SIZE, NULL, TIMER_TASK_PRIORITY,
                                        &timer_task_handle)) {
        status = TE_FAIL;
    }

    timer_task_queue = nt_qurt_pipe_create(TIMER_TASK_QUEUE_LEN, sizeof(time_timetick_type));

    if (timer_task_queue == NULL) {
        status = TE_FAIL;
    }

    return status;
}

#ifdef QTMR_DRV
void __attribute__((section(".after_ram_vectors"))) hres_qtmr_callback(void *param)
{
    BaseType_t wakeup_task = pdFALSE;

#if defined(HRES_TIMER_PROFILING)
    /* Record when timer isr actually was handled */
    timers.isr_start = hres_timer_timetick_get();
#endif

    if (param) {
        /* Signal the timer task of the timer interrupt event */
        xTaskNotifyFromISR((nt_osal_task_handle_t)param, (1 << TIMER_EVENT_EXPIRY_FROM_ISR), eSetBits, &wakeup_task);
        /* Wake the priority task if required */
        portYIELD_FROM_ISR(wakeup_task);
    }

#if defined(HRES_TIMER_PROFILING)
    timers.isr_end = hres_timer_timetick_get();
#endif
}
#endif  // QTMR_DRV
/**
 * hres_timer_init
 *
 * @brief Initializes timer module
 *
 * @param max_timers Maximum allowed internal timer structure
 * @param buffer_start_address Timer structure buffer
 * @param start_timer_task TRUE if timer task should be started
 *
 * @return TE_SUCCESS on success, else valid error type
 */
timer_error_type hres_timer_init(uint32_t max_timers, void *buffer_start_address, bool start_timer_task)
{
    uint32_t counter = 0;
#ifdef QTMR_DRV
    qtmr_status status;
#endif

    if (NULL == buffer_start_address || 0 == max_timers) {
        return TE_FAIL;
    }

    /* Init timer HW*/
    TIMER_INIT_HW();

    /* Init timer task */
    if (start_timer_task == TRUE) {
        if (TE_SUCCESS != timer_task_init()) {
            NT_LOG_SYSTEM_ERR("Timer task creation failed", 0, 0, 0);
            return TE_FAIL;
        }
    }

    /*Initialize the mutex*/
    timer_mutex = nt_osal_create_mutex();
    log_mutex = nt_osal_create_mutex();
    if (timer_mutex == NULL || log_mutex == NULL) {
        return TE_FAIL;
    }

    timers_max = max_timers;
    ptr_timer_buffer = (timer_internal_ptr_type)buffer_start_address;

    /*initialize the timer buffer elements*/
    for (; counter < timers_max; counter++) {
        memset(&ptr_timer_buffer[counter], 0, sizeof(timer_internal_type));
        ptr_timer_buffer[counter].index = counter;
        ptr_timer_buffer[counter].info.node_state = NODE_IS_FREE;
    }

#ifdef QTMR_DRV
    status = TIMER_INIT(hres_qtmr_callback, timer_task_handle);
    if (status != QTMR_SUCCESS)
        return TE_FAIL;
#endif
    b_timer_initialized = TRUE;
    return TE_SUCCESS;
}

/**
 * hres_timer_init_setup
 *
 * @brief Initializes timer module
 */
void hres_timer_init_setup(void)
{
    timer_internal_ptr_type *timer_buff =
        (timer_internal_ptr_type *)nt_osal_allocate_memory(sizeof(timer_internal_type) * MAX_TIMERS);
    if (NULL == timer_buff) {
        NT_LOG_PRINT(SYSTEM, CRIT, "Failed to allocate memory for timer");
    }
    if (TE_SUCCESS == hres_timer_init(MAX_TIMERS, timer_buff, TRUE)) {
        NT_LOG_PRINT(SYSTEM, CRIT, "Timer Init Done");
    } else {
        NT_LOG_PRINT(SYSTEM, CRIT, "Timer Init Failed");
    }
}

/**
 * hres_timer_def
 *
 * @brief Defines the timer. The supplied timer memory is formatted to store
 * callback functions, signaling information and deferrable/non-deferrable
 * information
 *
 * hres_timer_init should be called before calling this function
 *
 * @param timer Pointer to timer handle
 * @timer_attr Attributes of the timer to be defined
 *
 * @return Returns TE_SUCCESS on success, else a valid error type
 */
timer_error_type hres_timer_def(timer_type *timer, timer_cb_type timer_cb, task_info_ptr task_info)
{
    timer_internal_ptr_type timer_internal;
    timer_error_type status = TE_FAIL;

    if (FALSE == b_timer_initialized) {
        NT_LOG_SYSTEM_CRIT("Timer not initialized", 0, 0, 0);
        return TE_TIMER_MODULE_NOT_INITIALIZED;
    }

    if (NULL == timer || timer_cb == NULL) {
        return TE_INVALID_PARAMETERS;
    }

    if (nt_fail == nt_osal_semaphore_take(timer_mutex, portMAX_DELAY)) {
        NT_LOG_SYSTEM_ERR("Timer failed to take semaphore", 0, 0, 0);
    }

    /* To find the internal timer corresponding to external timer */
    if (*timer != TIMER_INVALID_HANDLE && NULL != (timer_internal = timer_get_client_timer(timer))) {
        /*if we found an internal timer allocation for an external timer
          means that we should not go ahead with timer def since we do not
          want to allocate multiple internal timers for the same external timer
          because that would lead to memory leak*/
        if (timer_internal->external_timer == timer) {
            if (TRUE == timer_error_fatal_on_redefinition)

            {
                NT_LOG_PRINT(SYSTEM, INFO, "Active timer trying to be redefined timer = 0x%x", *timer);
            } else {
                memset(timer, TIMER_INVALID_HANDLE, sizeof(timer_type));
            }
        }
    }

    timer_internal = timer_get_new_client_timer(&status, timer);

    if (NULL != timer_internal) {
        timer_internal->external_timer = timer;

        timer_internal->timer_callback = timer_cb;
        timer_internal->task_info = task_info;

        timer_internal->info.timer_state = TIMER_DEF_FLAG;
        status = TE_SUCCESS;
    }

    if (nt_fail == nt_osal_semaphore_give(timer_mutex)) {
        NT_LOG_SYSTEM_ERR("Timer failed to give semaphore", 0, 0, 0);
    }

    NT_LOG_SYSTEM_INFO("Timer defined successfully", 0, 0, 0);
    return status;
}

/**
 * timer_set
 *
 * @brief Starts the timer with the specified duration.
 * If reload == TRUE, then the timer will be reset/restarted when it expires
 * hres_timer_init should be called before calling this function
 *
 * @param timer Timer handle
 * @param timer_set_attr Timer attributes to set
 *
 * @return Returns TE_SUCCESS on success, else a valid error type
 */
timer_error_type timer_set(timer_ptr_type timer, timer_set_attribute_type *timer_set_attr)
{
#if defined(TIMER_CHECK_VALID_TASK)
    TaskHandle_t current_task_handle;
#endif
    timer_internal_ptr_type timer_internal;

    if (timer == NULL) {
        return TE_INVALID_PARAMETERS;
    }

    if (nt_fail == nt_osal_semaphore_take(timer_mutex, portMAX_DELAY)) {
        NT_LOG_SYSTEM_ERR("Timer failed to take semaphore", 0, 0, 0);
    }

#if defined(SUPPORT_ROOTCLK_DISABLE)
    /* If its the first timer in the list, enable the root clk */
    if (timers.active.first == NULL) {
        TIMER_SET_ROOTCLK(TRUE);
    }
#endif

    timer_prep_for_set(timer, &timer_internal);

#if defined(TIMER_CHECK_VALID_TASK)
    current_task_handle = xTaskGetCurrentTaskHandle();
    if (timer_internal->task_info->handle != current_task_handle) {
        NT_LOG_SYSTEM_ERR("Arming timer from another task not allowed", 0, 0, 0);
        if (nt_fail == nt_osal_semaphore_give(timer_mutex)) {
            NT_LOG_SYSTEM_ERR("Timer failed to give semaphore", 0, 0, 0);
        }
        return TE_INVALID_TASK;
    }
#endif

    /* ... convert given duration into ticks, and save in cache */
    timer_cvt_to_tick64(timer_set_attr->time, timer_set_attr->unit, &timer_internal->duration_sclk);
    timer_internal->info.unit = timer_set_attr->unit;

    /* Determine when timer should expire, and set reload */
    timer_internal->start = hres_timer_timetick_get();
    timer_internal->expiry = timer_internal->start + timer_internal->duration_sclk;
    timer_internal->info.reload = timer_set_attr->reload;

    /* add to active list */
    timer_insert(&timers.active, timer_internal);

    /* Active timer list has changed - ensure next timer event is correct */
    timer_update_timer_interrupt(TIMER_MVS_TIMER_SET);

    if (nt_fail == nt_osal_semaphore_give(timer_mutex)) {
        NT_LOG_SYSTEM_ERR("Timer failed to give semaphore", 0, 0, 0);
    }

    NT_LOG_SYSTEM_INFO("Timer Set Success", 0, 0, 0);
    return TE_SUCCESS;

} /* timer_set */

/**
 * hres_timer_undef
 *
 * @brief Stops an active timer, Frees the internal timer memory for the same
 *
 * @param Timer to stop
 *
 * @return Returns TE_SUCCESS on success, else a valid error type
 */
timer_error_type hres_timer_undef(timer_ptr_type timer)
{
    timer_error_type status = TE_SUCCESS;
    timer_internal_ptr_type timer_internal = NULL;

    if (nt_fail == nt_osal_semaphore_take(timer_mutex, portMAX_DELAY)) {
        NT_LOG_SYSTEM_ERR("Timer failed to take semaphore", 0, 0, 0);
        status = TE_FAIL;
    }

    if (NULL == timer || NULL == (timer_internal = timer_get_client_timer(timer))) {
        if (nt_fail == nt_osal_semaphore_give(timer_mutex)) {
            NT_LOG_SYSTEM_ERR("Timer failed to give semaphore", 0, 0, 0);
        }
        return TE_TIMER_ALREADY_IN_SAME_STATE;
    }

    /* Remove from active list if active, Free the internal timer */
    if (timer_internal->list != NULL) {
        /* Timer is active - remove timer */
        timer_remove(timer_internal);

        /* Active timer list has changed - ensure next timer event is correct */
        timer_update_timer_interrupt(TIMER_MVS_TIMER_UNDEFINED);
    }

    timer_free_internal_timer(timer_internal);
    *timer = TIMER_INVALID_HANDLE;

    if (nt_fail == nt_osal_semaphore_give(timer_mutex)) {
        NT_LOG_SYSTEM_ERR("Timer failed to give semaphore", 0, 0, 0);
        status = TE_FAIL;
    }
    return status;
} /* hres_timer_undef */

/**
 * timer_get_time_info
 *
 * @brief Gets the remaining time and the total duration of the timer in timeticks
 * hres_timer_init should be called before calling this function
 *
 * @param timer Timer handle
 * @param timer_info Requested timer information
 * @param data Time of the requested info in ticks
 *
 * @return Returns TE_SUCCESS on success, else a valid error type
 */
timer_error_type timer_get_time_info(timer_ptr_type timer, timer_info_type timer_info, time_timetick_type *data)
{
    timer_internal_ptr_type timer_internal;

    if (NULL == timer || NULL == data || timer_info >= TIMER_INFO_MAX ||
        NULL == (timer_internal = timer_get_client_timer(timer))) {
        return TE_FAIL;
    }

    hres_timer_timetick_get();
    if (timer_internal->info.timer_state == TIMER_SET_FLAG && timer_internal->expiry >= ticks_now) {
        switch (timer_info) {
            case TIMER_INFO_ABS_EXPIRY:
                *data = timer_internal->expiry;
                break;
            case TIMER_INFO_TIMER_DURATION:
                *data = timer_internal->duration_sclk;
                break;
            case TIMER_INFO_TIMER_REMAINING:
                *data = timer_internal->expiry - ticks_now;
                break;
            default:
                break;
        }
        return TE_SUCCESS;
    }
    return TE_TIMER_NOT_ACTIVE;
}

/**
 * hres_timer_stop
 *
 * @brief Clears the timer
 * hres_timer_init should be called before calling this function
 *
 * @param timer Timer to stop
 *
 * @return Returns TE_SUCCESS on success, else a valid error type
 */
timer_error_type hres_timer_stop(timer_ptr_type timer)
{
    timer_error_type status = TE_FAIL;
    timer_internal_ptr_type timer_internal;

    if (nt_fail == nt_osal_semaphore_take(timer_mutex, portMAX_DELAY)) {
        NT_LOG_SYSTEM_ERR("Timer failed to take semaphore", 0, 0, 0);
    }

    if (NULL == (timer_internal = timer_get_client_timer(timer))) {
        if (nt_fail == nt_osal_semaphore_give(timer_mutex)) {
            NT_LOG_SYSTEM_ERR("Timer failed to give semaphore", 0, 0, 0);
        }
        return TE_TIMER_ALREADY_IN_SAME_STATE;
    }

    /* Try to remove the timer. Should fail in timer_remove
       if the timer is NULL or not in the active list*/
    if (TE_SUCCESS == (status = timer_remove(timer_internal))) {
        timer_internal->info.timer_state = TIMER_CLEARED_FLAG;
        /* Active timer list has changed - ensure next timer event is correct */
        timer_update_timer_interrupt(TIMER_MVS_TIMER_CLEARED);
    }

    if (nt_fail == nt_osal_semaphore_give(timer_mutex)) {
        NT_LOG_SYSTEM_ERR("Timer failed to give semaphore", 0, 0, 0);
    }

    return status;
}

#ifndef QTMR_DRV
/**
 * qtmr_0_irq_handler
 *
 * @brief This is the interrupt handler for qtimer frame 0
 */
void __attribute__((section(".after_ram_vectors"))) qtmr_0_irq_handler(void)
{
    BaseType_t wakeup_task = pdFALSE;

#if defined(HRES_TIMER_PROFILING)
    /* Record when timer isr actually was handled */
    timers.isr_start = hres_timer_timetick_get();
#endif

    /* Disable qtmr */
    TIMER_ENABLE(FALSE);

    /* Signal the timer task of the timer interrupt event */
    xTaskNotifyFromISR(timer_task_handle, (1 << TIMER_EVENT_EXPIRY_FROM_ISR), eSetBits, &wakeup_task);
    /* Wake the priority task if required */
    portYIELD_FROM_ISR(wakeup_task);

#if defined(HRES_TIMER_PROFILING)
    timers.isr_end = hres_timer_timetick_get();
#endif
}
#endif  // QTMR_DRV

/**
 * hres_timer_pre_sleep
 *
 * @brief This function will return the next expiry time before going into
 * power collapse. Sleep module will call this function when going into
 * power collpase.
 *
 * This function is NOT REENTRANT and should not be called by any module
 * except sleep.
 *
 * hres_timer_init should be called before calling this function
 *
 * @return Returns the nearest expiry in microseconds. 0 if no active timers.
 */
uint64_t hres_timer_pre_sleep(sleep_time_info_t *info, bool is_task_scheduler_suspended)
{
    uint64_t ms_time = 0;
    time_timetick_type future_expiry;

    if (is_task_scheduler_suspended && (nt_fail == nt_osal_semaphore_take(timer_mutex, portMAX_DELAY))) {
        NT_LOG_SYSTEM_ERR("Timer failed to take semaphore", 0, 0, 0);
    }

    /* Get the current time */
    hres_timer_timetick_get();

    future_expiry = TIMER_MAX_EXPIRY_TICKS;

    if (timers.active.first != NULL) {
        /* The first timer on the active list is the timer next to expire */
        future_expiry = timers.active.first->expiry;
        if (info != NULL)
            info->q_timer_callback = timers.active.first->timer_callback;
    }

    /* If future_expiry is in past */
    if (future_expiry < ticks_now) {
        if (is_task_scheduler_suspended && (nt_fail == nt_osal_semaphore_give(timer_mutex))) {
            NT_LOG_SYSTEM_ERR("Timer failed to give semaphore", 0, 0, 0);
        }
        if (info != NULL)
            info->ms_time = ms_time;
        return ms_time;
    }

    /* From this point on timer interrupt won't be adjusted */
    timers.do_reprogram_isr = FALSE;

    if (is_task_scheduler_suspended && (nt_fail == nt_osal_semaphore_give(timer_mutex))) {
        NT_LOG_SYSTEM_ERR("Timer failed to give semaphore", 0, 0, 0);
    }
    /* Return the difference between now and the above future expiry */
    timer_cvt_from_tick64(future_expiry - ticks_now, T_USEC, &ms_time);

    if (info != NULL)
        info->ms_time = ms_time;
    return ms_time;

} /* hres_timer_pre_sleep */

/**
 * hres_timer_post_sleep
 *
 * @brief Notifies the timer task about wakeup
 * Sleep module will call this function after waking up from power collpase.
 * hres_timer_init should be called before calling this function
 *
 */
void hres_timer_post_sleep(void)
{
    BaseType_t wakeup_task = pdFALSE;
    /* Signal the timer task of the wake up event */
    NT_LOG_SYSTEM_INFO("Timer post socpm_sleep", 0, 0, 0);
    xTaskNotifyFromISR(timer_task_handle, (1 << TIMER_EVENT_EXPIRY_FROM_ISR), eSetBits, &wakeup_task);
    /* Wake the priority task if required */
    portYIELD_FROM_ISR(wakeup_task);
} /* timer_undefer_match_interrupt */

/**
 * hres_timer_sleep_adjust
 *
 * @brief Notifies the timer task about requirment for sleep time adjustment
 * Sleep module will call this function after waking up from power collpase.
 * hres_timer_init should be called before calling this function
 *
 * @param sleep_duration Sleep duration in microseconds
 *
 */
void hres_timer_sleep_adjust(uint64_t sleep_duration)
{
    BaseType_t wakeup_task = pdFALSE;
    time_timetick_type time = 0;

    /* Convert ms to ticks */
    timer_cvt_to_tick64(sleep_duration, T_USEC, &time);
    /* Insert into queue */
    xQueueSendFromISR(timer_task_queue, &time, NULL);
    /* Signal the timer task of the wake up event */
    xTaskNotifyFromISR(timer_task_handle, (1 << TIMER_EVENT_PM_WAKEUP), eSetBits, &wakeup_task);
    /* Wake the priority task if required */
    portYIELD_FROM_ISR(wakeup_task);
} /* timer_undefer_match_interrupt */

/**
 * hres_timer_deinit
 *
 * @brief Deinitializes all timers
 */
uint32_t hres_timer_deinit(void)
{
    /*set the timer init to FALSE*/
    b_timer_initialized = FALSE;
#ifdef QTMR_DRV
    qtmr_status status;
#endif

    if (timer_mutex != NULL) {
        nt_osal_semaphore_delete(timer_mutex);
        timer_mutex = NULL;
    } else {
        NT_LOG_SYSTEM_ERR("attempt to delete timer_mutex which is NULL", 0, 0, 0);
    }

#ifndef QTMR_DRV
    /*disable the timer interrupt */
    TIMER_ENABLE(FALSE);
#else
    status = TIMER_DEINIT();
#endif  // QMTR_DRV

#if defined(SUPPORT_ROOTCLK_DISABLE)
    TIMER_SET_ROOTCLK(FALSE);
#endif

    /*set the internal timer memory to zero*/
    memset(&ptr_timer_buffer[0], 0, sizeof(timer_internal_type) * timers_max);

    /*set the max number of timers to zero*/
    timers_max = 0;

#ifdef QTMR_DRV
    NT_LOG_SYSTEM_INFO("Timer Deinitialized %d", status, 0, 0);
#else
    NT_LOG_SYSTEM_INFO("Timer Deinitialized", 0, 0, 0);
#endif

    return 0;
}

/**
 * hres_timer_set_64
 *
 * @brief This function is wrapper to timer set function
 *
 * @param timer Timer handle
 * @param time Time duration for expiry
 * @param reload True if timer is periodic
 * @param unit Unit fo time
 *              [T_USEC = microsecond, T_MSEC = millisecond]
 *
 * @return Returns TE_SUCCESS on success, else a valid error type
 */

timer_error_type hres_timer_set_64(timer_ptr_type timer, time_timetick_type time, bool reload, time_unit_type unit)
{
    timer_error_type status = TE_SUCCESS;
    timer_set_attribute_type timer_set_attr;

    timer_set_attr.time = time;

    if (unit > T_MSEC) {
        NT_LOG_PRINT(SYSTEM, INFO, "Time unit not supported %d\r\n", unit);
        return TE_INVALID_UNIT;
    }
    timer_set_attr.unit = unit;

    if (TRUE == reload) {
        timer_set_attr.reload = TRUE;
    } else {
        timer_set_attr.reload = FALSE;
    }

    if (TE_FAIL == timer_set(timer, &timer_set_attr)) {
        status = TE_FAIL;
    }

    return status;
}

/**
 * hres_timer_handler
 *
 * @brief Wrapper function for processing client timer queue
 *
 * @param timer_queue client timer queue handle
 *
 * @return Returns TE_SUCCESS on success, else a valid error type
 */
uint32_t hres_timer_handler(QueueHandle_t timer_queue)
{
    uint32_t status = TE_SUCCESS;
    timer_msg_t timer_msg;

    while (pdPASS == xQueueReceive(timer_queue, &timer_msg, NO_WAIT)) {
#if defined(HRES_TIMER_PROFILING)
        timer_msg.q_pop_time = hres_timer_timetick_get();
        timer_cvt_from_tick64(timer_msg.q_pop_time - timer_msg.isr_start, T_USEC, &timer_msg.delay);
        hres_timer_log_dump(&timer_msg);
#endif

        timer_msg.timer_cb(timer_msg.handle);
    }

    return status;
}

#if defined(HRES_TIMER_PROFILING)
/**
 * hres_timer_log_dump
 *
 * @brief Dupms timer timestamps to buffer
 *
 * @param timer_msg pointer to timer message containing timestamps
 *
 * @return None
 */
void hres_timer_log_dump(timer_msg_t *timer_msg)
{
    if (timer_msg == NULL) {
        return;
    }
    if (nt_fail == nt_osal_semaphore_take(log_mutex, portMAX_DELAY)) {
        NT_LOG_SYSTEM_ERR("Timer failed to take semaphore", 0, 0, 0);
    }
    if (dump_idx >= MAX_DUMP_LEN) {
        dump_idx = 0;
    }
    delay_dump[dump_idx].handle = timer_msg->handle;
    delay_dump[dump_idx].set_time = timer_msg->set_time;
    delay_dump[dump_idx].isr_start = timer_msg->isr_start;
    delay_dump[dump_idx].isr_end = timer_msg->isr_end;
    delay_dump[dump_idx].q_push_time = timer_msg->q_push_time;
    delay_dump[dump_idx].q_pop_time = timer_msg->q_pop_time;
    delay_dump[dump_idx].delay = timer_msg->delay;
    dump_idx++;
    if (nt_fail == nt_osal_semaphore_give(log_mutex)) {
        NT_LOG_SYSTEM_ERR("Timer failed to give semaphore", 0, 0, 0);
    }
}

/**
 * hres_timer_print_dump
 *
 * @brief Prints the dumpped timestamps recorded using hres_timer_log_dump()
 *
 * @param None
 *
 * @return None
 */
void hres_timer_print_dump(void)
{
    uint8_t idx = 0;
    for (idx = 0; idx < MAX_DUMP_LEN; idx++) {
        if (delay_dump[idx].handle != 0) {
            NT_LOG_PRINT(SYSTEM, CRIT, "handle \t%x", (uint32_t)delay_dump[idx].handle, 0, 0);
            NT_LOG_PRINT(SYSTEM, CRIT, "set_time \t%u %u", TIMER_TICKS_HI_BITS(delay_dump[idx].set_time),
                         TIMER_TICKS_LO_BITS(delay_dump[idx].set_time), 0);
            NT_LOG_PRINT(SYSTEM, CRIT, "isr_start \t%u %u", TIMER_TICKS_HI_BITS(delay_dump[idx].isr_start),
                         TIMER_TICKS_LO_BITS(delay_dump[idx].isr_start), 0);
            NT_LOG_PRINT(SYSTEM, CRIT, "isr_end \t%u %u", TIMER_TICKS_HI_BITS(delay_dump[idx].isr_end),
                         TIMER_TICKS_LO_BITS(delay_dump[idx].isr_end), 0);
            NT_LOG_PRINT(SYSTEM, CRIT, "queue push \t%u %u", TIMER_TICKS_HI_BITS(delay_dump[idx].q_push_time),
                         TIMER_TICKS_LO_BITS(delay_dump[idx].q_push_time), 0);
            NT_LOG_PRINT(SYSTEM, CRIT, "queue pop \t%u %u", TIMER_TICKS_HI_BITS(delay_dump[idx].q_pop_time),
                         TIMER_TICKS_LO_BITS(delay_dump[idx].q_pop_time), 0);
            NT_LOG_PRINT(SYSTEM, CRIT, "delay \t%u us", (uint32_t)delay_dump[idx].delay, 0, 0);
        }
    }
}

#endif /*HRES_TIMER_PROFILING*/
#endif /*SUPPORT_HIGH_RES_TIMER*/

#ifdef IMAGE_FERMION

/**
 * @brief Returns current time in milli second
 * @param None
 * @return: u32_t time value in milli second
 */
uint32_t hres_timer_curr_time_ms(void)
{
#ifdef SUPPORT_HIGH_RES_TIMER
    uint64_t curr_time_ms;
    timer_cvt_from_tick64(hres_timer_timetick_get(), T_MSEC, &curr_time_ms);
    return curr_time_ms;
#else  /* SUPPORT_HIGH_RES_TIMER */
    return (uint32_t)(nt_hal_get_curr_time() / 1000);
#endif /* SUPPORT_HIGH_RES_TIMER */
}

/**
 * @brief Returns current time in us
 * @param None
 * @return: u64 time value
 */
uint64_t hres_timer_curr_time_us(void)
{
#ifdef SUPPORT_HIGH_RES_TIMER
    uint64_t curr_time_us;
    timer_cvt_from_tick64(hres_timer_timetick_get(), T_USEC, &curr_time_us);
    return curr_time_us;
#else  /* SUPPORT_HIGH_RES_TIMER */
    return nt_hal_get_curr_time();
#endif /* SUPPORT_HIGH_RES_TIMER */
}

/**
 * <!-- delay in ms -->
 *
 * @brief Normal delay function in milli sec
 * @param time: Delay time needed in milli sec
 * @return: void
 */
void hres_timer_ms_delay(uint32_t time_ms)
{
    uint64_t curr_time = hres_timer_curr_time_us();
    uint64_t target_time = (curr_time + 1000 * time_ms);

    while (curr_time < target_time) {
        curr_time = hres_timer_curr_time_us();
    }
}

/**
 * <!-- delay in us -->
 *
 * @brief Normal delay function in micro sec
 * @param time: Delay time needed in micro sec
 * @return: void
 */
void __attribute__((section(".__sect_ps_txt"))) hres_timer_us_delay(uint32_t time_us)
{
    uint64_t curr_time = hres_timer_curr_time_us();
    uint64_t target_time = (curr_time + time_us);

    while (curr_time < target_time) {
        curr_time = hres_timer_curr_time_us();
    }
}
#endif /* IMAGE_FERMION */
