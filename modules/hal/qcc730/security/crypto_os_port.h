/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __CRYPTO_OS_PORT_H__
#define __CRYPTO_OS_PORT_H__

/* Crypto mutex defines */
#include "qurt_mutex.h"
#include "qurt_internal.h"

typedef qurt_mutex_t crypto_mutex_t;
#define CRYPTO_INIT_LOCK(__lock__)   do { qurt_mutex_create(&(__lock__)); } while(0)
#define CRYPTO_TAKE_LOCK(__lock__)      ((qurt_mutex_lock_timed(&(__lock__), QURT_TIME_WAIT_FOREVER)) == QURT_EOK) 
#define CRYPTO_RELEASE_LOCK(__lock__)   do { qurt_mutex_unlock(&(__lock__)); } while(0)

typedef qurt_time_t crypto_time_t;
#define CRYPTO_GET_TIME() qurt_timer_get_ticks()
#define CRYPTO_GET_ELAPSED_TIME_IN_MS(start_ticks, end_ticks) qurt_timer_convert_ticks_to_time(end_ticks - start_ticks, QURT_TIME_MSEC)

#endif

