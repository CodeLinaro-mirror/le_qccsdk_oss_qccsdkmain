/**
@file CeEL_Dm.h
@brief Crypto Engine DMOV source file
*/

/*===========================================================================



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

  $Header: //components/rel/core.ioe/1.0/v2/rom/drivers/security/crypto/src/CeEL_Dm.h#5 $
  $DateTime: 2017/04/07 16:13:09 $
  $Author: pwbldsvc $

when         who     what, where, why
--------     ---     ----------------------------------------------------------
2015-10-29   yk      Initial Version
============================================================================*/

/*===========================================================================

                           INCLUDE FILES

===========================================================================*/
#include "com_dtypes.h"

/*===========================================================================

            LOCAL DEFINITIONS AND DECLARATIONS FOR MODULE

===========================================================================*/
/*===========================================================================

                      PUBLIC FUNCTION DECLARATIONS

===========================================================================*/
/**
 * @brief Check if Polling mode or Interrupt mode.
 *
 * @return CeELErrorType
 *
 * @see
 *
 */
uint8 CeElDmIsPollingMode(void);

/**
 * @brief Set up CRYPTO CONFIG register.
 *
 * @return CeELErrorType
 *
 * @see
 *
 */
CeELErrorType CeElSetupConfig(void);

/**
 * @brief Initialize the Data mover
 *
 * @return none
 *
 * @see
 *
 */

CeELErrorType CeElDmInit(void);

/**
 * @brief
 *
 * @param
 *
 * @return
 *
 * @see
 *
 */

CeELErrorType CeElHashDmXfer(uint8 *buff_ptr, uint32 buff_len, uint8 *digest_ptr, uint32 digest_len,
                             uint8 non_blocking_flag);

/**
 * @brief
 *
 * @param
 *
 * @return
 *
 * @see
 *
 */

CeELErrorType CeElCipherDmXfer(uint8 *buff_in, uint32 buff_in_len, uint8 *buff_out, uint32 buff_out_len);
