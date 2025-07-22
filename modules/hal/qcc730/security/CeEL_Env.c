/**
@file CeEL_Env.c
@brief Crypto Engine Environment Library source file
*/

/*===========================================================================

                     Crypto Engine Environment Library

DESCRIPTION

INITIALIZATION AND SEQUENCING REQUIREMENTS
  None

Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
============================================================================*/

/*===========================================================================

                           EDIT HISTORY FOR FILE

  This section contains comments describing changes made to this file.
  Notice that changes are listed in reverse chronological order.

 $Header:
 $DateTime: 2017/01/05 14:29:56 $
 $Author: pwbldsvc $

when         who     what, where, why
--------     ---     ----------------------------------------------------------
2015-10-29   yk      Initial version
============================================================================*/
#include "CeEL_Env.h"
#include "CeML.h"
#include "qurt_mutex.h"

/* Mutex to prevent concurrent accesses to the hardware */
static qurt_mutex_t CeEL_mutex;
static uint32 ceel_mutex_init = 0;

/**********************************************************
 * Initialize a mutex using Qurt. Mutex memory is allocated
 *
 **********************************************************/
void CeEL_mutex_init(void)
{
    if (ceel_mutex_init == 0) {
        qurt_mutex_create(&CeEL_mutex);
        ceel_mutex_init = 1;
    }
}
/**********************************************************
 * Lock the CeEL mutex
 *
 **********************************************************/
void CeEL_mutex_lock(void)
{
    if (ceel_mutex_init == 0)
        qurt_mutex_lock(&CeEL_mutex);
}

/**********************************************************
 * Unlock the CeEL mutex
 *
 **********************************************************/
void CeEL_mutex_unlock(void)
{
    if (ceel_mutex_init == 0)
        qurt_mutex_unlock(&CeEL_mutex);
}

void CeEL_mutex_deinit(void)
{
    if (1 == ceel_mutex_init) {
        // printf("will call qurt_mutex_delete\r\n");
        qurt_mutex_delete(&CeEL_mutex);
        ceel_mutex_init = 0;
    }
}
