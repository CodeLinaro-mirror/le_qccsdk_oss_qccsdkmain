/**
@file CeEL.c
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
 $DateTime: 2017/04/07 16:13:09 $
 $Author: pwbldsvc $

when         who     what, where, why
--------     ---     ----------------------------------------------------------
2015-10-29   yk      Initial version
============================================================================*/

#include "CeEL.h"
#include "CeEL_Dm.h"
#include "CeEL_Env.h"
#include "CeCL.h"
#include "HALhwio.h"
#include <stdlib.h>
#include "safeAPI.h"
#include <string.h>
/**
 * @brief Function performs register transfer
 *        Selects appropriate function based on
 *        dataType
 *
 *
 * @return CeELErrorType
 *
 * @see
 *
 */
CeELErrorType CeElRegXferFunction(CEELIovecListType *inData, CEELIovecListType *outData, boolean lastBlock, uint8 *cntx,
                                  CEELDataType dataType);

/**
 * @brief memory barrier
 *
 *
 * @return None
 *
 * @see
 *
 */
CeELErrorType CeElMemoryBarrier(void)
{
    CEEL_MEMORY_BARRIER();
    return CEEL_ERROR_SUCCESS;
}

/**
 * @brief Initialize environment layer
 *
 *
 * @return None
 *
 * @see
 *
 */
CeELErrorType CeElInit(void)
{
    if (CEEL_ERROR_SUCCESS != CeElDmInit()) {
        return CEEL_ERROR_FAILURE;
    }

    return CEEL_ERROR_SUCCESS;
}

/**
 * @brief Deinitialize environment layer
 *
 * @return None
 *
 * @see
 *
 */
CeELErrorType CeElDeInit(void)
{
    return CEEL_ERROR_SUCCESS;
}

/**
 * @brief Enable crypto related clock
 *
 *
 * @return None
 *
 * @see
 *
 */
CeELErrorType CeElEnableClock(void)
{
    CeELErrorType ret_val = CEEL_ERROR_FAILURE;

    ret_val = (CeELErrorType)CeElClkEnable();  // refer pka_enable_clock
    return ret_val;
}

/**
 * @brief Disalbe crypto related clock
 *
 *
 * @return None
 *
 * @see
 *
 */
CeELErrorType CeElDisableClock(void)
{
    CeELErrorType ret_val = CEEL_ERROR_FAILURE;
    // ret_val = (CeELErrorType)CeElClkDisable();
    return ret_val;
}

/**
 * @brief Enable KDF related clock
 *
 *
 * @return None
 *
 * @see
 *
 */
CeELErrorType CeElEnableKDFClock(void)
{
    CeELErrorType ret_val = CEEL_ERROR_FAILURE;
    ret_val = (CeELErrorType)CeElKDFClkEnable();
    return ret_val;
}

/**
 * @brief Disalbe crypto related clock
 *
 *
 * @return None
 *
 * @see
 *
 */
CeELErrorType CeElDisableKDFClock(void)
{
    CeELErrorType ret_val = CEEL_ERROR_FAILURE;
    ret_val = (CeELErrorType)CeElKDFClkDisable();
    return ret_val;
}

/**
 * @brief  allocate heap memory (or global memory in case heap is not available)
 *
 * @param buff_ptr [in] pointer to a pointer to allocate heap memory
 *        buff_len [in] length of the heap memory to be allcoated
 *
 * @return CeELErrorType
 *
 * @see
 *
 */
CeELErrorType CeElmalloc(void **ptr, uint32 ptrLen)
{
    return CeElmalloc_Env(ptr, ptrLen);
}

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
CeELErrorType CeElfree(void *ptr)
{
    free(ptr);
    return CEEL_ERROR_SUCCESS;
}

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
CeELErrorType CeElMemScpy(void *dst, uint32 dstLen, void *src, uint32 srcLen)
{
    memscpy(dst, dstLen, src, srcLen);

    return CEEL_ERROR_SUCCESS;
}

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
CeELErrorType CeElMemset(void *src, uint32 val, uint32 len)
{
    memset(src, val, len);

    return CEEL_ERROR_SUCCESS;
}

/**
 * @brief Gets the transfer mode supported
 *
 *
 * @return CeELErrorType
 *
 * @see
 *
 */
CeELErrorType CeElGetXferModeList(CeElXferModeType *xferMode)  // not need?
{
    if (!xferMode) {
        return CEEL_ERROR_INVALID_PARAM;
    }

    if (CEEL_DM_IS_SUPPORTED())
        *xferMode = CEEL_XFER_MODE_DMA_INTF;
    else
        *xferMode = CEEL_XFER_MODE_REG_INTF;

    return CEEL_ERROR_SUCCESS;
}

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
CeELErrorType CeElGetXferfunction(CeElXferModeType xferMode, CeElXferFunctionType *xferFunc)
{
    if (!xferFunc) {
        return CEEL_ERROR_INVALID_PARAM;
    }

    if (xferMode == CEEL_XFER_MODE_REG_INTF) {
        *xferFunc = CeElRegXferFunction;
        return CEEL_ERROR_SUCCESS;
    } else {
        *xferFunc = CeElDmXferFunction;
        return CEEL_ERROR_SUCCESS;
    }
}

/**
 * @brief Enter mutex
 *
 *
 * @return None
 *
 * @see
 *
 */
CeELErrorType CeElMutexEnter(void)
{
    CEEL_MUTEX_INIT();
    CEEL_MUTEX_ENTER();
    return CEEL_ERROR_SUCCESS;
}

/**
 * @brief Exit mutex
 *
 *
 * @return None
 *
 * @see
 *
 */
CeELErrorType CeElMutexExit(void)
{
    CEEL_MUTEX_EXIT();
    return CEEL_ERROR_SUCCESS;
}

/**
 * @brief Transfer Function for performing register mode
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
CeELErrorType CeElRegXferFunction(CEELIovecListType *inData, CEELIovecListType *outData, boolean lastBlock, uint8 *cntx,
                                  CEELDataType dataType)
{
    CeELErrorType ret_val = CEEL_ERROR_FAILURE;
    if ((!inData) || (!outData) || (!inData->iov) || (!outData->iov) || (!cntx)) {
        return CEEL_ERROR_INVALID_PARAM;
    }

    if (dataType == CEEL_DATA_HASH) {
        ret_val = CeELHashRegXfer(inData->iov->pvBase, inData->iov->dwLen, outData->iov->pvBase, outData->iov->dwLen);
        return ret_val;
    } else if (dataType == CEEL_DATA_CIPHER) {
        ret_val = CeELCipherRegXfer(inData->iov->pvBase, inData->iov->dwLen, outData->iov->pvBase, outData->iov->dwLen);
        return ret_val;
    } else {
        ret_val = CEEL_ERROR_INVALID_PARAM;
        return ret_val;
    }
}

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
                                 CEELDataType dataType)
{
    CeELErrorType ret_val = CEEL_ERROR_FAILURE;
    if ((!inData) || (!outData) || (!inData->iov) || (!outData->iov) || (!cntx)) {
        return CEEL_ERROR_INVALID_PARAM;
    }

    if (dataType == CEEL_DATA_HASH) {
        ret_val =
            CeElHashDmXfer(inData->iov->pvBase, inData->iov->dwLen, outData->iov->pvBase, outData->iov->dwLen, *cntx);
        return ret_val;
    } else if (dataType == CEEL_DATA_CIPHER) {
        ret_val = CeElCipherDmXfer(inData->iov->pvBase, inData->iov->dwLen, outData->iov->pvBase, outData->iov->dwLen);
        return ret_val;
    } else {
        ret_val = CEEL_ERROR_INVALID_PARAM;
        return ret_val;
    }
}

CeELErrorType CeElCeil(uint32 total, uint32 power, uint32 *ceil)
{
    *ceil = total >> power;
    if ((*ceil << power) != total)
        (*ceil)++;

    return CEEL_ERROR_SUCCESS;
}

/**
 * @brief Check if Polling mode or Interrupt mode.
 *
 * @return CeELErrorType
 *
 * @see
 *
 */
uint8 CeElIsPollingMode(void)
{
    return (CeElDmIsPollingMode());
}
