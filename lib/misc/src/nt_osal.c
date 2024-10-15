/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/
#include "nt_osal.h"
#include "FreeRTOS.h"
#include "projdefs.h"
#include "task.h"
#include "timers.h"
#include "queue.h"
#include "semphr.h"
#include "time.h"
#include "FreeRTOSConfig.h"
#include "portmacro.h"
#include "qurt_internal.h"

#define TASK_FAILED           -1


/**
 * <!-- nt_delay -->
 *
 * @brief Normal delay function
 * @param time: Delay time needed in milli seconds
 * @return: void
 */
void nt_normal_delay(uint32_t time)
{
    volatile int32_t i,j, value;
    value = (volatile int32_t)(time * (10^6)*3);
    for(i = value ; i>=0 ; i--)
    {
       for(j = 100;j>=0;j--)
       {
         __asm volatile("nop");
       }
    }
}

size_t  strnscat(
          char        *dst,
          size_t      dst_size,
          const char  *src,
          size_t      src_size
          )

{
  size_t  copy_size = (dst_size <= src_size)? dst_size : src_size;
  strlcat(dst, src, copy_size);
  return copy_size;
}
