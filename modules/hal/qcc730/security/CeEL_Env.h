/*===========================================================================

                    Crypto Engine Environment API

GENERAL DESCRIPTION


EXTERNALIZED FUNCTIONS


INITIALIZATION AND SEQUENCING REQUIREMENTS

Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
===========================================================================*/

/*===========================================================================
                      EDIT HISTORY FOR FILE

  $Header: //components/rel/core.ioe/1.0/v2/rom/drivers/security/crypto/src/CeEL_Env.h#5 $

when       who     what, where, why
--------   ---     ----------------------------------------------------------
10/29/15   yk      initial version
===========================================================================*/

/*===========================================================================
                     INCLUDE FILES FOR MODULE
===========================================================================*/
#include "Fermion_seq_hwioreg.h"
#include "Fermion_hwiobase.h"
#include "com_dtypes.h"
#include "qurt_mutex.h"

/*===========================================================================
                 DEFINITIONS AND TYPE DECLARATIONS
===========================================================================*/
typedef enum {
    CEEL_ENV_ERROR_SUCCESS = 0x0,
    CEEL_ENV_ERROR_FAILURE = 0x1,
    CEEL_ENV_ERROR_INVALID_PARAM = 0x2
} CeEL_EnvErrorType;

#define CEEL_ASSERT()

#define CeElClkEnable() CeClClockEnable()

#define CeElClkDisable() CeClClockDisable()

#define CeElKDFClkEnable() CeClKDFClockEnable()

#define CeElKDFClkDisable() CeClKDFClockDisable()

#define CEEL_MUTEX_INIT() CeEL_mutex_init()

#define CEEL_MUTEX_ENTER() CeEL_mutex_lock()

#define CEEL_MUTEX_EXIT() CeEL_mutex_unlock()

#define CEEL_DM_IS_SUPPORTED() 1

#define CEEL_MEMORY_BARRIER()

/*===========================================================================

                      PUBLIC DATA DECLARATIONS

===========================================================================*/
