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

  $Header: //components/rel/core.ioe/1.0/v2/rom/drivers/security/crypto/src/CeEL.h#8 $

when       who     what, where, why
--------   ---     ----------------------------------------------------------
10/29/15   yk      initial version
===========================================================================*/

/*===========================================================================
                     INCLUDE FILES FOR MODULE
===========================================================================*/
#include "com_dtypes.h"

/*===========================================================================
                 DEFINITIONS AND TYPE DECLARATIONS
===========================================================================*/
typedef enum {
    CEEL_XFER_MODE_REG_INTF = 0x0,
    CEEL_XFER_MODE_DMA_INTF = 0x1,
    CEEL_XFER_MODE_MAX = 0x2
} CeElXferModeType;

typedef struct {
    void *pvBase;
    uint32 dwLen;
} CEELIovecType;

typedef struct {
    CEELIovecType *iov;
    uint32 size;
} CEELIovecListType;

typedef enum { CEEL_DATA_HASH = 0x1, CEEL_DATA_CIPHER = 0x2 } CEELDataType;

typedef enum {
    CEEL_ERROR_SUCCESS = 0x0,
    CEEL_ERROR_FAILURE = 0x1,
    CEEL_ERROR_INVALID_PARAM = 0x2,
    CEEL_ERROR_NOT_SUPPORTED = 0x3,
    CEEL_ERROR_TIMEOUT = 0x4,
    CEEL_ERROR_NO_MEMORY = 0x5,
    CEEL_ERROR_BAD_ADDRESS = 0x6,
    CEEL_ERROR_BAD_DATA = 0x7,
    CEEL_ERROR_HW_BUSY = 0x8,
    CEEL_ERROR_NOT_ALLOWED = 0x9
} CeELErrorType;

/* the function pointer representing the data transfer function available in the given
 * environment. "direction" determines if the transfer is into the crypto core or out of
 * the crypto core.
 */
typedef CeELErrorType (*CeElXferFunctionType)(CEELIovecListType *, CEELIovecListType *, boolean, uint8 *, CEELDataType);

/*===========================================================================
                      FUNCTION DECLARATIONS
===========================================================================*/

/**
 * @brief  memory barrier
 *
 * @param void
 *
 * @return CeELErrorType
 *
 * @see
 *
 */

CeELErrorType CeElMemoryBarrier(void);

/**
 * @brief  Performs initialization of environment layer
 *
 * @param  void
 *
 * @return CeELErrorType
 *
 * @see
 *
 */
CeELErrorType CeElInit(void);

/**
 * @brief  Deinitialization of environment layer
 *
 * @param
 *
 * @return CeELErrorType
 *
 * @see
 *
 */
CeELErrorType CeElDeInit(void);

/**
 * @brief  Enables crypto related clock
 *
 * @param  void
 *
 * @return CeELErrorType
 *
 * @see
 *
 */
CeELErrorType CeElEnableClock(void);

/**
 * @brief  Disable crypto related clock
 *
 * @param  void
 *
 * @return CeELErrorType
 *
 * @see
 *
 */
CeELErrorType CeElDisableClock(void);
/**
 * @brief  Enables KDF related clock
 *
 * @param  void
 *
 * @return CeELErrorType
 *
 * @see
 *
 */
CeELErrorType CeElEnableKDFClock(void);

/**
 * @brief  Disable KDF related clock
 *
 * @param  void
 *
 * @return CeELErrorType
 *
 * @see
 *
 */
CeELErrorType CeElDisableKDFClock(void);

/**
 * @brief  free heam memory allocated
 *
 * @param  pointer to a heap memory
 *
 * @return CeELErrorType
 *
 * @see
 *
 */
CeELErrorType CeElfree(void *ptr);

/**
 * @brief  memory secure copy
 *
 * @param dst    [in] output pointer to copy data to
 *        dstLen [in] max size of dst buffer
 *        src    [in] input pointer to copy data from
 *        srcLen [in] size of data to copy to src.
 *
 * @return CeELErrorType
 *
 * @see
 *
 */
CeELErrorType CeElMemScpy(void *dst, uint32 dstLen, void *src, uint32 srcLen);

/**
 * @brief  set memory
 *
 * @param src [in] memory pointer to set data
 *        val [in]  value to set memory
 *        len [in] size of data to set
 *
 * @return CeELErrorType
 *
 * @see
 *
 */
CeELErrorType CeElMemset(void *src, uint32 val, uint32 len);

/**
 * @brief Gets the transfer mode supported
 *
 *
 * @return CeELErrorType
 *
 * @see
 *
 */
CeELErrorType CeElGetXferModeList(CeElXferModeType *xferMode);

/**
 * @brief Transfer Function for performing DM mode
 *        crypto operations
 *
 * @param inData[in] : input data, pointer given by caller
 * @param outData[in] : output data, pointer given by caller
 * @param lastBlock[in] : if last block of operation
 * @param cntx[in] : context of the operation to be performed
 * @param dataType[in] : Operation type
 *
 *
 * @return CeELErrorType
 *
 * @see
 *
 */

CeELErrorType CeElDmXferFunction(CEELIovecListType *inData, CEELIovecListType *outData, boolean lastBlock, uint8 *cntx,
                                 CEELDataType dataType);

/**
 * @brief Gets the transfer function based on the transfer mode
 *
 * @param xferMode[in] : xfer mode
 * @param xferFunc[out] : pointer to transfer function
 *
 * @return CeELErrorType
 *
 * @see
 *
 */
CeELErrorType CeElGetXferfunction(CeElXferModeType, CeElXferFunctionType *);

/**
 * @brief  enter mutex
 *
 * @param  void
 *
 * @return CeELErrorType
 *
 * @see
 *
 */
CeELErrorType CeElMutexEnter(void);

/**
 * @brief  exit mutex
 *
 * @param void
 *
 * @return CeELErrorType
 *
 * @see
 *
 */
CeELErrorType CeElMutexExit(void);

/**
 * @brief  internal ceiling function to get multiple of 16

 *
 * @param
 *
 * @return CeELErrorType
 *
 * @see
 *
 */
CeELErrorType CeElCeil(uint32 total, uint32 power, uint32 *ceil);

/**
 * @brief  wrapper API to allocate heap memory
 *
 * @param  ptr    [in] pointer to a pointer to allocate heap memory
 *         ptrLen [in] length of heap memory to be allocated
 *
 * @return CeELErrorType
 *
 * @see
 *
 */
CeELErrorType CeElmalloc_Env(void **ptr, uint32 ptrLen);

/**
 * @brief  wrapper API to free heap memory
 *
 * @param  ptr [in] pointer to the heap memory
 *
 * @return CeELErrorType
 *
 * @see
 *
 */
CeELErrorType CeElfree_Env(void *ptr);

/**
 * @brief  Initialize a mutex using Qurt. Mutex memory is allocated
 *
 * @param  none
 *
 * @return void
 *
 * @see
 *
 */
void CeEL_mutex_init(void);
void CeEL_mutex_deinit(void);

/**
 * @brief  Lock the CeEL mutex
 *
 * @param  none
 *
 * @return void
 *
 * @see
 *
 */
void CeEL_mutex_lock(void);

/**
 * @brief  Unlock the CeEL mutex
 *
 * @param  none
 *
 * @return void
 *
 * @see
 *
 */
void CeEL_mutex_unlock(void);

/**
 * @brief Check if Polling mode or Interrupt mode.
 *
 * @return CeELErrorType
 *
 * @see
 *
 */
uint8 CeElIsPollingMode(void);
