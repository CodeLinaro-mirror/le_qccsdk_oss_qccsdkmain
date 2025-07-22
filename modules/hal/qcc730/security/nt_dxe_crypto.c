/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/** ------------------------------------------------------------------------- *

    ------------------------------------------------------------------------- *

    \file asicDXE.c

    \brief DXE implementation

    $Id$

    Copyright (C) 2006 Airgo Networks, Incorporated
   ========================================================================== */

#include "nt_hw.h"
#include "hal_int_sys.h"
#include "nt_osal.h"
#include "dxe.h"
#include "nt_logger_api.h"

#ifdef NT_FN_HW_CRYPTO

NT_BOOL nt_qcc_dxe_transfer(e_dxe_channel channelIn, e_dxe_channel channelOut, uint32_t DinAddr, uint32_t DoutAddr,
                            uint32_t inDwords, uint32_t outDwords)
{
    uint32_t regVal = 0;
    uint32_t dxe_timeout = 100000;
    uint32_t chInDXEBaseAddr, chOutDXEBaseAddr;

    if ((channelIn >= DXE_CHANNEL_MAX) || (channelOut >= DXE_CHANNEL_MAX)) {
        NT_LOG_PRINT(DPM, ERR, "asicDXEConfigChannel failed - channelin %d channelout %d greater than max chan# %d.\n",
                     channelIn, channelOut, DXE_CHANNEL_MAX);
        return 0;
    }

    chInDXEBaseAddr = QWLAN_DXE_0_CH0_CTRL_REG + NT_DXE_CH_REG_SIZE * channelIn;

    chOutDXEBaseAddr = QWLAN_DXE_0_CH0_CTRL_REG + NT_DXE_CH_REG_SIZE * channelOut;

    // Set up destination address for channelIn
    regVal = ((QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_DATA_IN_REG >> 2) << QWLAN_DXE_0_CH0_DADRL_BASE_OFFSET) | 0x20;
    HAL_REG_WR(chInDXEBaseAddr + NT_DXE_CH_DADRL_REG, regVal);
    HAL_REG_WR(chInDXEBaseAddr + NT_DXE_CH_DADRH_REG, 0);

    // Set up source address for channelIn
    regVal = DinAddr;
    HAL_REG_WR(chInDXEBaseAddr + NT_DXE_CH_SADRL_REG, regVal);
    HAL_REG_WR(chInDXEBaseAddr + NT_DXE_CH_SADRH_REG, 0);

    // Set transfer size and chunk size
    regVal = (1 << QWLAN_DXE_0_CH0_SZ_CHK_SZ_OFFSET) | ((inDwords << 2) & QWLAN_DXE_0_CH0_SZ_TOT_SZ_MASK);
    HAL_REG_WR(chInDXEBaseAddr + NT_DXE_CH_SZ_REG, regVal);

    // Set up destination address for channelOut
    regVal = DoutAddr;
    HAL_REG_WR(chOutDXEBaseAddr + NT_DXE_CH_DADRL_REG, regVal);
    HAL_REG_WR(chOutDXEBaseAddr + NT_DXE_CH_DADRH_REG, 0);

    // Set up source address for channelOut
    regVal = ((QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_DATA_OUT_REG >> 2) << QWLAN_DXE_0_CH0_SADRL_BASE_OFFSET) | 0x20;
    HAL_REG_WR(chOutDXEBaseAddr + NT_DXE_CH_SADRL_REG, regVal);
    HAL_REG_WR(chOutDXEBaseAddr + NT_DXE_CH_SADRH_REG, 0);

    // Set transfer size and chunk size
    regVal = (1 << QWLAN_DXE_0_CH0_SZ_CHK_SZ_OFFSET) | ((outDwords << 2) & QWLAN_DXE_0_CH0_SZ_TOT_SZ_MASK);
    HAL_REG_WR(chOutDXEBaseAddr + NT_DXE_CH_SZ_REG, regVal);

    // H2H(0), Ch Enable
    regVal = QWLAN_DXE_0_CH0_CTRL_EN_MASK | QWLAN_DXE_0_CH0_CTRL_SIQ_MASK |
             (channelOut << QWLAN_DXE_0_CH0_CTRL_CTR_SEL_OFFSET) |
             (QWLAN_DXE_0_CH0_CTRL_ENDIANNESS_ELTLEND << QWLAN_DXE_0_CH0_CTRL_ENDIANNESS_OFFSET);

    // This should start the DMA on output channel
    HAL_REG_WR(chOutDXEBaseAddr + NT_DXE_CH_CTRL_REG, regVal);

    // H2H(0), Ch Enable
    regVal = QWLAN_DXE_0_CH0_CTRL_EN_MASK | QWLAN_DXE_0_CH0_CTRL_DIQ_MASK |
             (channelIn << QWLAN_DXE_0_CH0_CTRL_CTR_SEL_OFFSET) |
             (QWLAN_DXE_0_CH0_CTRL_ENDIANNESS_ELTLEND << QWLAN_DXE_0_CH0_CTRL_ENDIANNESS_OFFSET);

    // This shoould start the DMA on input channel
    HAL_REG_WR(chInDXEBaseAddr + NT_DXE_CH_CTRL_REG, regVal);

    // Check if done
    while (!(((regVal = HAL_REG_RD(chOutDXEBaseAddr + NT_DXE_CH_STATUS_REG)) & QWLAN_DXE_0_CH0_STATUS_DONE_MASK)) &&
           (--dxe_timeout > 0)) {
        if (regVal & QWLAN_DXE_0_CH0_STATUS_ERR_MASK) {
            return 0;
        }
    }

    if (dxe_timeout == 0) {
        NT_LOG_DPM_ERR("########dxe_timeout#######", 0, 0, 0);
        return 0;
    }

    return 1;
}

#endif
