/*========================================================================
 *Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 * @file timer.h
 * @brief High Res Timer param and struct definitions
 *========================================================================*/
#ifndef TIMER_H
#define TIMER_H

/*-------------------------------------------------------------------------
 * Include Files
 * ----------------------------------------------------------------------*/
#include <stdbool.h>
#include "nt_osal.h"
#include "fwconfig_wlan.h"

#if defined(SUPPORT_HIGH_RES_TIMER)

/*-------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * ----------------------------------------------------------------------*/

#define HRES_TIMER_PROFILING
#define MAX_DUMP_LEN 128
/*-------------------------------------------------------------------------
 * Type Declarations
 * ----------------------------------------------------------------------*/
typedef uint64_t time_timetick_type;

/**
 * Various units supported by the timetick module
 */
typedef enum {
    T_TICK, /**< -- Return time in Ticks */
    T_USEC, /**< -- Return time in Microseconds */
    T_MSEC, /**< -- Return time in Milliseconds */
    T_SEC,  /**< -- Return time in Seconds */

    T_NONE = T_TICK /**< -- use if no paticular return type is needed */
} time_unit_type;

/** @brief Timer state structure type. */
typedef enum timer_state_struct {
    TIMER_DEFAULT_FLAG = 1,
    TIMER_DEF_FLAG,
    TIMER_SET_FLAG,
    TIMER_EXPIRED_FLAG,
    TIMER_CLEARED_FLAG,
    TIMER_UNDEF_FLAG,
} timer_state_struct_type;

/* Error Returns Enums of Timer APIs */
typedef enum {
    TE_SUCCESS = 0,
    TE_FAIL,
    /* Need to make "timer_client" as dependency if some module is calling
       into timer creation apis before timer module gets initialized */
    TE_TIMER_MODULE_NOT_INITIALIZED, /* timer creation apis are called before timer system is initialized */
    TE_HANDLE_IN_USE,                /* timer is being defined upon a valid timer */
    TE_INVALID_TIMER_HANDLE,         /* timer is invalid */
    TE_INVALID_PARAMETERS,           /* input parameters for an api are invalid */
    TE_INVALID_DURATION,             /* timer is invalid duration */
    TE_INVALID_STATE_FOR_OP,         /* timer is in invalid state for requested operation */
    TE_MALLOC_FAILED,                /* free client timers are over */
    TE_NO_FREE_INTERNAL_TIMER,       /* free internal timers are over */
    TE_TIMER_NOT_ACTIVE,             /* timer is not active but an operation expects it */
    TE_TIMER_ALREADY_IN_SAME_STATE,  /* timer is already in state where an operation is being tried */
    TE_INVALID_TASK,                 /* not a valid task to arm the timer from */
    TE_INVALID_UNIT,                 /* time unit not supported or invalid */
    TE_MAX = 0xFFFFFFFF
} timer_error_type;

typedef enum {
    TIMER_INFO_ABS_EXPIRY = 0,
    TIMER_INFO_TIMER_DURATION,
    TIMER_INFO_TIMER_REMAINING,
    TIMER_INFO_MAX,
} timer_info_type;

/* Timer msg identifier */
enum {
    TMR_CMD_ID_TIMEOUT,
    TMR_CMD_ID_UNTIMEOUT,
};

typedef uint32_t timer_type;

/** Pointer to timer structure */
typedef timer_type *timer_ptr_type;

/** Timer handle type.
 */
typedef uint32_t timer_handle_type;

/** Timer callback function.
 */
typedef void (*timer_cb_type)(timer_handle_type timer);

/** User task info type
 */
typedef struct {
    TaskHandle_t handle;
    uint32_t event;
    QueueHandle_t timer_queue;
} task_info_type;

typedef task_info_type *task_info_ptr;

typedef struct {
    bool reload;
    time_unit_type unit;
    time_timetick_type time;
} timer_set_attribute_type;

typedef struct {
    timer_handle_type handle;
    timer_cb_type timer_cb;
#if defined(HRES_TIMER_PROFILING)
    time_timetick_type set_time;
    time_timetick_type isr_start;
    time_timetick_type isr_end;
    time_timetick_type q_push_time;
    time_timetick_type q_pop_time;
    time_timetick_type delay;
#endif
} timer_msg_t;

typedef struct {
    char pcTaskName[configMAX_TASK_NAME_LEN];
    uint64_t ms_time;
    timer_cb_type q_timer_callback;
    TimerCallbackFunction_t nt_timer_callback;
} sleep_time_info_t;

typedef struct timer_cmd_s {
    bool reload;
    uint8_t cmd_id;
    time_unit_type unit;
    timer_ptr_type p_handle;
    time_timetick_type time;
} timer_cmd_t;
/*-------------------------------------------------------------------------
 * Function Declarations and Documentation
 * ----------------------------------------------------------------------*/

/**
 * hres_timer_init_setup
 *
 * @brief Initializes timer module
 */
void hres_timer_init_setup(void);

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
timer_error_type hres_timer_init(uint32_t max_timers, void *buffer_start_address, bool start_timer_task);

/**
 * hres_timer_def
 *
 * @brief This function is used to define a timer
 *
 * @param timer Timer handle
 * @param func_addr Timer callback
 * @param task_info Task information of the calling task
 *
 * @return Returns TE_SUCCESS on success, else a valid error type
 */
timer_error_type hres_timer_def(timer_ptr_type timer, timer_cb_type func_addr, task_info_ptr task_info);

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
timer_error_type hres_timer_set_64(timer_ptr_type timer, time_timetick_type time, bool reload, time_unit_type unit);

/**
 * hres_timer_undef
 *
 * @brief Stops an active timer, Frees the internal timer memory for the same
 *
 * @param Timer to stop
 *
 * @return Returns TE_SUCCESS on success, else a valid error type
 */
timer_error_type hres_timer_undef(timer_ptr_type timer);

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
 * @param sleep time info for logging
 * @param is task scheduler suspended to provide shceduler status
 *
 * @return Returns the nearest expiry in microseconds.
 */
uint64_t hres_timer_pre_sleep(sleep_time_info_t *info, bool is_task_scheduler_suspended);

/**
 * hres_timer_post_sleep
 *
 * @brief Notifies the timer task about wakeup
 * Sleep module will call this function after waking up from power collpase.
 * hres_timer_init should be called before calling this function
 *
 */
void hres_timer_post_sleep(void);

/**
 * hres_timer_sleep_adjust
 *
 * @brief Notifies the timer task about requirment for sleep time adjustment
 * Sleep module will call this function after waking up from power collpase.
 * hres_timer_init should be called before calling this function
 *
 * @param sleep_duration Sleep duration in microseconds.
 *
 */
void hres_timer_sleep_adjust(uint64_t sleep_duration);

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
timer_error_type hres_timer_stop(timer_ptr_type timer_handle);

/**
 * hres_timer_timetick_get
 *
 * @brief This function gets the timetick and stores the value in global variable
 * Timer module should be initialized before calling this function
 *
 * @return timetick value
 */
time_timetick_type hres_timer_timetick_get(void);

/**
 * hres_timer_deinit
 *
 * @brief Deinitializes all timers
 */
uint32_t hres_timer_deinit(void);

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
int8_t timer_cvt_to_tick64(uint64_t time, time_unit_type unit, uint64_t *pTimeRet);

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
int8_t timer_cvt_from_tick64(uint64_t time, time_unit_type unit, uint64_t *pTimeRet);

/**
 * hres_timer_handler
 *
 * @brief Wrapper function for processing client timer queue
 *
 * @param timer_queue client timer queue handle
 *
 * @return Returns TE_SUCCESS on success, else a valid error type
 */
uint32_t hres_timer_handler(QueueHandle_t timer_queue);

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
void hres_timer_log_dump(timer_msg_t *timer_msg);

/**
 * hres_timer_print_dump
 *
 * @brief Prints the dumpped timestamps recorded using hres_timer_log_dump()
 *
 * @param None
 *
 * @return None
 */
void hres_timer_print_dump(void);
#endif

#endif /*SUPPORT_HIGH_RES_TIMER*/

#ifdef IMAGE_FERMION

/**
 * hres_curr_time_us
 *
 * @brief Returns current time in us in u64_t format
 *
 * @param None
 *
 * @return u64_t current time in us
 */
uint64_t hres_timer_curr_time_us(void);

/**
 * hres_curr_time_ms
 *
 * @brief Returns current time in ms in u32_t format
 *
 * @param None
 *
 * @return u32_t current time in ms
 */
uint32_t hres_timer_curr_time_ms(void);

/**
 * hres_us_delay
 *
 * @brief Waits for us delay
 *
 * @param delay in us
 *
 * @return None
 */
void hres_timer_us_delay(uint32_t time_us);

/**
 * hres_ms_delay
 *
 * @brief Waits for ms delay
 *
 * @param delay in ms
 *
 * @return None
 */
void hres_timer_ms_delay(uint32_t time_ms);

#endif /* IMAGE_FERMION */
#endif /*TIMER_H*/
