/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef SME_MLME_INC_NT_TIMER_H_
#define SME_MLME_INC_NT_TIMER_H_

/*Include Files*/
#include "nt_osal.h"

#define TIMER_NAME "sme_timer"

/*Timer Configuration*/                  /*Move this configuration to sme header file*/
#define NT_SCAN_BEACON_WAIT_TIME   1000  // need to mention unit like milli sec or mili sec
#define NT_AUTH_RESPONSE_WAIT_TIME 1000
#define NT_ASSO_RESPONSE_WAIT_TIME 1000

#define NT_BEACON_TIMER_ID         0x01
#define NT_AUTHENTICATION_TIMER_ID 0x02
#define NT_ASSOCIATION_TIMER_ID    0x03

#define TIME_IS_GREATER_U64(a, b)    ((int64_t)((uint64_t)a - (uint64_t)b) > 0)
#define TIME_IS_GREATER_EQ_U64(a, b) ((int64_t)((uint64_t)a - (uint64_t)b) >= 0)
#define TIME_IS_SMALLER_U64(a, b)    ((int64_t)((uint64_t)a - (uint64_t)b) < 0)
#define TIME_IS_SMALLER_EQ_U64(a, b) ((int64_t)((uint64_t)a - (uint64_t)b) <= 0)
#define TIME_DIFF_U64(a, b)          ((uint64_t)((int64_t)(a) - (int64_t)(b)))
#define TIME_DIFF_WITH_WRAP_U64(a, b) \
    (TIME_IS_GREATER_U64(a, b) ? TIME_DIFF_U64(a, b) : (0xFFFFFFFFFFFFFFFF - (b) + (a) + 1))

#define TIME_IS_GREATER(a, b)         ((int32_t)((uint32_t)a - (uint32_t)b) > 0)
#define TIME_IS_GREATER_EQ(a, b)      ((int32_t)((uint32_t)a - (uint32_t)b) >= 0)
#define TIME_IS_SMALLER(a, b)         ((int32_t)((uint32_t)a - (uint32_t)b) < 0)
#define TIME_IS_SMALLER_EQ(a, b)      ((int32_t)((uint32_t)a - (uint32_t)b) <= 0)
#define TIME_DIFF(a, b)               ((uint32_t)((int32_t)(a) - (int32_t)(b)))
#define TIME_DIFF_WITH_WRAP_U32(a, b) (TIME_IS_GREATER(a, b) ? TIME_DIFF(a, b) : (0xFFFFFFFF - (b) + (a) + 1))

#define MS_TO_US(x)    ((x)*1000)   /* millisecond to microsecond */
#define US_TO_MS(x)    ((x) / 1000) /* microsecond to millisecond */
#define MS_TO_TU(x)    (((x)*1000) >> 10)
#define TU_TO_MS(x)    ((x)*1024 / 1000)
#define TU_TO_US(x)    ((x) << 10)
#define SEC_TO_MSEC(x) (x * 1000)

/*Timer Function Declaration*/
TimerHandle_t nt_create_timer(void *call_back_function, void *timer_id, uint32_t time_countdown,
                              UBaseType_t auto_reload);
int nt_start_timer(TimerHandle_t timer_handle);
int nt_stop_timer(TimerHandle_t timer_handle);
void *nt_get_timeout_arg(TimerHandle_t timer_handle);
int nt_delete_timer(TimerHandle_t timer_handle);
TickType_t nt_timer_get_tick_count();
int nt_timer_change_time_period(TimerHandle_t timer_handle, TickType_t new_period);

//#if (defined NT_RCLI)
TickType_t nt_timer_get_expiry_time(TimerHandle_t timer_handle);
//#endif	// NT_RCLI

#endif /* SME_MLME_INC_NT_TIMER_H_ */
