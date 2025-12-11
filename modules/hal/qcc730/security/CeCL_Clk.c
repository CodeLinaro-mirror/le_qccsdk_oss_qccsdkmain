/**
@file CeCL_Clk.c
@brief Crypto Engine Core Library source file
*/

/*===========================================================================

                     Crypto Engine Core Library

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
 $DateTime: 2017/03/21 07:30:11 $
 $Author: pwbldsvc $

when         who     what, where, why
--------     ---     -----------------------------------------------------
2016-09-15   yash     initial version
============================================================================*/
#include "CeCL.h"
#include "CeCL_Target.h"
#include "qccx.h"

/*
 * NOTE: As per understanding the high level tree is voting for crypto and KDF clocks
 *       hence the clock request using DAL API's is not required.
 */

CeCLErrorType CeClClockEnable(void)
{
    QCC730V2_CCU_BASE_Type *ccu = QCC730V2_CCU_BASE;
    ccu->ccu.CCU_R_CCU_ENABLE_CLK.bit.QCC_ENABLE_CLK = 1;

    /* Everything went well, returning success! */
    return CECL_ERROR_SUCCESS;
}

CeCLErrorType CeClClockDisable(void)
{
    QCC730V2_CCU_BASE_Type *ccu = QCC730V2_CCU_BASE;
    ccu->ccu.CCU_R_CCU_ENABLE_CLK.bit.QCC_ENABLE_CLK = 0;

    /* Everything went well, returning success! */
    return CECL_ERROR_SUCCESS;
}

CeCLErrorType CeClKDFClockEnable(void)
{
    QCC730V2_CCU_BASE_Type *ccu = QCC730V2_CCU_BASE;
    ccu->ccu.CCU_R_CCU_ENABLE_CLK.bit.KDF_ENABLE_CLK = 1;

    /* Everything went well, returning success! */
    return CECL_ERROR_SUCCESS;
}

CeCLErrorType CeClKDFClockDisable(void)
{
    QCC730V2_CCU_BASE_Type *ccu = QCC730V2_CCU_BASE;
    ccu->ccu.CCU_R_CCU_ENABLE_CLK.bit.KDF_ENABLE_CLK = 0;

    /* Everything went well, returning success! */
    return CECL_ERROR_SUCCESS;

}
