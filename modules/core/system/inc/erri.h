/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef ERRI_H
#define ERRI_H

/*===========================================================================

                    Error Handling Service Internal Header File

===========================================================================*/

#include "com_dtypes.h"
#include "err.h"

#define ERR_LOOP_DELAY_USEC 10000
#define ERR_CLK_PAUSE_SMALL 500
#define ERR_CLK_PAUSE_KICK  100
/* The following values are based on ERR_LOOP_DELAY_USEC */
/* Actual frequency is loop_freq_value*ERR_LOOP_DELAY_USEC microseconds */
#define ERR_LOOP_DOG_FREQ 5 /* 50 ms */

#endif /* ERRI_H */
