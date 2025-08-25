/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*========================================================================
 *
 * @file timer_internal.h
 * @brief Timer internal param and struct definitions
 *========================================================================*/
#ifndef TIMER_INTERNAL_H
#define TIMER_INTERNAL_H

/*-------------------------------------------------------------------------
 * Include Files
 * ----------------------------------------------------------------------*/
#ifdef QTMR_DRV
#include "ferm_qtmr.h"
#else
#include "qtmr.h"
#endif
#include "timer.h"
#include "nt_osal.h"

#if defined(SUPPORT_HIGH_RES_TIMER)
/*-------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * ----------------------------------------------------------------------*/

#define TIMER_INVALID_HANDLE 0xFFFFFFFF
#define TIMER_EARLY_TOL      0 /*Can be adjusted based on requirement*/
#define TIMER_INDEX_MASK     0xFFFF
#define TIMER_MAGIC          0xC3C3
#define TIMER_INDEX_BITS     16

#ifndef QTMR_DRV
/* Hardware Abstraction */
#define FRAME_n QTMR_FRAME_PHYSICAL_0

/* Get counter frequency */
#define TIMER_GET_FRQ() qtmr_get_freq()
/* Enable timer */
#define TIMER_ENABLE(x) qtmr_enable(FRAME_n, x)
/* Get current time */
#define TIMER_GET_TIME64() qtmr_get_time64(FRAME_n)
/* Set next interrupt */
#define TIMER_SET_TRIGGER64(x) qtmr_set_trigger64(FRAME_n, x)
/* Init the HW timer */
#define TIMER_INIT_HW() qtmr_init()

#if defined(SUPPORT_ROOTCLK_DISABLE)
#define TIMER_SET_ROOTCLK(x) qtmr_set_rootclk(x)
#endif /* SUPPORT_ROOTCLK_DISABLE */

#else
/* Hardware Abstraction */
#define FRAME_n                   QTMR_FRAME_PHYSICAL_0

/* Get counter frequency */
#define TIMER_GET_FRQ()           qtmr_get_counter_freq()
/* Enable timer */
#define TIMER_INIT(cb, param)     qtmr_frame_comp_init(FRAME_n, cb, param)
/* Get current time */
#define TIMER_GET_TIME64(cnt64)   qtmr_get_frame_count_no_check(FRAME_n)
/* Set next interrupt */
#define TIMER_SET_TRIGGER64(cval) qtmr_frame_comp_start(FRAME_n, cval, 0, 0)
/* Stop the timer */
#define TIMER_DEINIT()            qtmr_frame_comp_deinit(FRAME_n)
/* Init the HW timer */
#define TIMER_INIT_HW()           qtmr_plat_init()

#if defined(SUPPORT_ROOTCLK_DISABLE)
#define TIMER_SET_ROOTCLK(x) qtmr_enable_clock(x)
#endif /* SUPPORT_ROOTCLK_DISABLE */

#endif  // QTMR_DRV

/*-------------------------------------------------------------------------
 * Type Declarations
 * ----------------------------------------------------------------------*/

/*This enum will be used to specify the event
 responsible for setting the match value*/
typedef enum {
    TIMER_MVS_TIMER_PROCESSED = 0x1,
    TIMER_MVS_TIMER_SET,
    //    TIMER_MVS_TIMER_SET_ABS,
    TIMER_MVS_DEFER_MATCH_INT,
    TIMER_MVS_UNDEFER_MATCH_INT,
    TIMER_MVS_TIMER_CLEARED,
    TIMER_MVS_TIMER_UNDEFINED,
    TIMER_MVS_INT_DISABLED,
} timer_match_interrupt_setter_type;

/** Timer Node status flags */
typedef enum timer_internal_node { NODE_IS_FREE = 0x1, NODE_IS_NOT_FREE = 0x2 } timer_node_status_t;

typedef struct timer_properties_struct {
    timer_state_struct_type timer_state : 8; /* 8 bits for timer_state: 0-255 */
    bool reload : 1;                         /* 1 bit for reload */
    time_unit_type unit : 3;                 /* 3 bits for unit */
    timer_node_status_t node_state : 4;      /* 3 bits for node state*/
} timer_properties_type;

/** Timer event flags */
typedef enum {
    TIMER_EVENT_EXPIRY_FROM_ISR,
    TIMER_EVENT_PM_WAKEUP,
    TIMER_EVENT_QTMR_FRAME_2,
    TIMER_EVENT_QTMR_FRAME_3,
    TIMER_EVENT_QTMR_FRAME_4
} timer_event_type;

/** @brief Timer list structure type. Values in this structure are for private
use by timer.c only. */
typedef struct timer_list_struct {
    struct timer_struct *first; /**< List of timers. */

    struct timer_struct *last; /**< End of the timer list. */
} timer_list_type;

typedef timer_list_type *timer_list_ptr;

/*timer type*/
typedef struct timer_struct {
    /* Index of timer_buffer array */
    uint32_t index;

    timer_ptr_type external_timer;

    task_info_ptr task_info;

    /**< Callback function to call when the timer
      expires. */
    timer_cb_type timer_callback;

    /* Pointer to active timer list */
    timer_list_ptr list;

    /* Actual duation in ticks*/
    time_timetick_type duration_sclk;

    /** Clock tick count timer expiry*/
    time_timetick_type expiry;

    /** Clock tick count value when timer was set (started) */
    time_timetick_type start;

    struct timer_struct *next;

    struct timer_struct *prev;

    timer_properties_type info;

} timer_internal_type;

typedef timer_internal_type *timer_internal_ptr_type;

typedef struct {
    /* Linked list of timer groups which have been disabled at some time. */
    timer_list_type active;

    /* Last value written to match count*/
    time_timetick_type match_value;

    /* Flag to indicate if timers_process_active_timers() is executing */
    bool processing;

    /* Flag to indicate if timer_isr can be updated */
    bool do_reprogram_isr;

#if defined(HRES_TIMER_PROFILING)
    /* Timestamp when call to _set_next_interrupt updates match_value */
    time_timetick_type set_time;

    /* Timestamp when timer isr occurred at */
    time_timetick_type isr_start;

    /* Timestamp when timer isr ended */
    time_timetick_type isr_end;

    /* Timestmaps for sleep adjust */
    time_timetick_type sleep_adjust_time;
#endif
} timers_type;

#endif /*SUPPORT_HIGH_RES_TIMER*/
#endif /*TIMER_INTERNAL_H */
