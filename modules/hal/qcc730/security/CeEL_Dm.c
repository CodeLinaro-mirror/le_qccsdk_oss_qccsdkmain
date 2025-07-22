/**
@file CeEL_Dm.c
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

  $Header: //components/rel/core.ioe/1.0/v2/rom/drivers/security/crypto/src/CeEL_Dm.c#7 $
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
#include "CeEL.h"
#include "CeEL_Env.h"
#include "CeCL.h"
#include "CeCL_Target.h"
#include "qurt_signal.h"
#include "nt_common.h"
#include <unistd.h>

/*===========================================================================

            LOCAL DEFINITIONS AND DECLARATIONS FOR MODULE

===========================================================================*/

#define CRYPTO_SIGNAL_MASK 0x00000001

static uint32 ceel_dm_isr_init = 0;
static qurt_signal_t crypto_signal;

/* This shall be set to 1 to force the polling mode in MissionROM. This is "dont care" for boot image */
#if defined(CECL_FORCE_POLLING_MODE)
volatile uint8 ceel_force_dm_polling = 1;
#else
volatile uint8 ceel_force_dm_polling = 0;
#endif

/* Polling mode only in Boot images and Interrupt/Polling mode in OS based images based on global variable
 * ceel_force_dm_polling */
//#define CEEL_DM_MODE_IS_POLLING() (platform_isOSMode()?ceel_force_dm_polling:1)
#define CEEL_DM_MODE_IS_POLLING() 1

// Register Read and Write
#define HW_REG_WR(_reg, _val)                  \
    (*((volatile uint32_t *)(_reg))) = (_val); \
    __asm volatile("dsb" ::: "memory")
#define HW_REG_RD(_reg) (*((volatile uint32_t *)(_reg)))

/*===========================================================================

                      PUBLIC FUNCTION DECLARATIONS

===========================================================================*/

uint8 CeElDmIsPollingMode(void)
{
    if (CEEL_DM_MODE_IS_POLLING()) {
        /* polling mode */
        return 1;
    } else {
        /* interrupt mode. */
        return 0;
    }
}

/**
 * @brief Set up CRYPTO CONFIG register.
 *
 * @return CeELErrorType
 *
 * @see
 *
 */
CeELErrorType CeElSetupConfig(void)
{
    CeELErrorType result = CEEL_ERROR_SUCCESS;
    volatile uint32_t regVal;
    uint32_t val;

    if (CEEL_DM_MODE_IS_POLLING()) {
        /* Disable all interrupts */
        HW_REG_WR(CECL_CE_CONFIG, 0xF);
    } else {
        /* CONFIG register as the CPU's interrupt requirement. */
        HAL_REG_WR(CECL_CE_CONFIG, CECL_CE_CONFIG_VALUE);  // not need this if we don't use interrupt?

        /* Clear some of the Crypto INTR status registers */
        val = QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_STATUS_DOUT_INTR_MASK |
              QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_STATUS_DIN_INTR_MASK |
              QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_STATUS_OPERATION_DONE_MASK |
              QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_STATUS_ERR_INTR_MASK;

        regVal = HAL_REG_RD(CECL_CE_STATUS);
        regVal &= ~val;
        HAL_REG_WR(CECL_CE_CONFIG, regVal);
    }
    return result;
}

/**
 * @brief Set up DMOV polling wait loop
 *
 * @return CE_Result_Type
 *
 * @see
 *
 */

CeELErrorType CeElDmPollingWait()
{
    uint32 ce_status = 0;
    CeELErrorType result = CEEL_ERROR_SUCCESS;

    HAL_REG_WR(CECL_CE_GOPROC, CECL_CE_GOPROC_GO_BMSK);

    // Set up polling wait loop for DMOV operation
    while (1) {
        ce_status = HAL_REG_RD(CECL_CE_STATUS);
        CeElMemoryBarrier();
        if (ce_status & CECL_CE_STATUS_OPERATION_DONE_BMSK) {
            break;
        }
    }

    return result;
}

/**
 * @brief Crypto Interrupt Service Routine
 *
 * @return Void
 *
 * @see
 *
 */
void CeElDmIsrFunc(void)  // TBD
{
    volatile uint32_t regVal;
    uint32_t val;
    /* Clear some of the Crypto INTR status registers */
    val = QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_STATUS_DOUT_INTR_MASK |
          QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_STATUS_DIN_INTR_MASK |
          QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_STATUS_OPERATION_DONE_MASK |
          QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_STATUS_ERR_INTR_MASK;

    regVal = HAL_REG_RD(CECL_CE_STATUS);
    regVal &= ~val;
    HAL_REG_WR(CECL_CE_CONFIG, regVal);

    /* Clear pending interrupt in controller */
    // InterruptController_ClearPending(CECL_IRQ_NUM);

    /* This is not a blocking OS call, this will unlock the called task waiting on signal. */
    qurt_signal_set(&crypto_signal, CRYPTO_SIGNAL_MASK);
}

/**
 * @brief Set up DMOV interrupt wait loop
 *
 * @return CE_Result_Type
 *
 * @see
 *
 */

CeELErrorType CeElDmInterruptWait()
{
    uint32 ce_status = 0;

    /* Check if signal and interrupts are initialized. */
    if (!ceel_dm_isr_init) {
        /* This is not expected */
        return CEEL_ERROR_FAILURE;
    }

    /* Clear the signals which are required by the driver
     * No need to check for return for this! */
    qurt_signal_clear(&crypto_signal, CRYPTO_SIGNAL_MASK);

    /* Enable Interrupt controller to receive Crypto interrupt */

    HAL_REG_WR(CECL_CE_GOPROC, CECL_CE_GOPROC_GO_BMSK);

    /* This will wait endlessly until the CeElDmIsrFunc() unlocks the signal
    Note: return code of this API is set_signals and not OK/SUCCESS */
    qurt_signal_wait(&crypto_signal, CRYPTO_SIGNAL_MASK, QURT_SIGNAL_ATTR_WAIT_ANY | QURT_SIGNAL_ATTR_CLEAR_MASK);

    /* Check status for operation completion and return to caller */
    ce_status = HAL_REG_RD(CECL_CE_STATUS);
    if (ce_status & CECL_CE_STATUS_OPERATION_DONE_BMSK) {
        return CEEL_ERROR_SUCCESS;
    } else {
        return CEEL_ERROR_FAILURE;
    }
}

/**
 * @brief Initialize the Data mover
 *
 * @return none
 *
 * @see
 *
 */

CeELErrorType CeElDmInit(void)
{
    if (!(CEEL_DM_MODE_IS_POLLING())) {
        if (!ceel_dm_isr_init) {
            /* Clear pending interrupt in controller */
            // InterruptController_ClearPending(CECL_IRQ_NUM);

            /* Do one time initialization during runtime - this also enables the interrupt */

            /* Initialize the OS signal on which driver can wait on. */
            qurt_signal_init(&crypto_signal);  // qurt_signal_create

            /* Clear the signals which are required by the driver
             * No need to check for return for this! */
            qurt_signal_clear(&crypto_signal, CRYPTO_SIGNAL_MASK);

            /* Update the global variable only when above initializations are success */
            ceel_dm_isr_init = 1;

            /* Disable the crypto interrupt in interrupt controller as there is no transactions started yet */
        }
    }
    return CEEL_ERROR_SUCCESS;
}

/**
 * @brief  Data transfer thru DM
 *
 * @param buff_ptr       [in]  Pointer to input buffer
 * @param buff_len       [in]  Size of the input buffer
 * @param digest_ptr     [out] Pointer to output buffer
 * @param digest_len     [in]  Size of output buffer
 *
 * @return CE_Result_Type
 *
 * @see
 *
 */

CeELErrorType CeElHashDmXfer(uint8 *buff_ptr, uint32 buff_len, uint8 *digest_ptr, uint32 digest_len,
                             uint8 non_blocking_flag)
{
    CeELErrorType result = CEEL_ERROR_SUCCESS;

    /* Sanity check inputs */
    if (!buff_ptr || !digest_ptr) {
        return CEEL_ERROR_INVALID_PARAM;
    }

    // while (!CECL_HW_SEC_CE_STATUS_DIN_RDY);

    HAL_REG_WR(CECL_CE_DATA_IN_BUF_BASE, buff_ptr);
    HAL_REG_WR(CECL_CE_DATA_IN_BUF_LEN, buff_len);

    HAL_REG_WR(CECL_CE_DATA_OUT_BUF_BASE, digest_ptr);

    if (non_blocking_flag) {
        HAL_REG_WR(CECL_CE_GOPROC, CECL_CE_GOPROC_GO_BMSK);
        return result;
    }

    if (CEEL_DM_MODE_IS_POLLING()) {
        // Set up polling wait loop for write operation (data input buffer  -> CE -> data output buffer)
        result = CeElDmPollingWait();
        if (CEEL_ERROR_SUCCESS != result) {
            return CEEL_ERROR_FAILURE;
        }
    } else {
        // Set up interrupt wait for write operation (data input buffer  -> CE -> data output buffer)
        result = CeElDmInterruptWait();
        if (CEEL_ERROR_SUCCESS != result) {
            return CEEL_ERROR_FAILURE;
        }
    }

    return result;
}

/**
 * @brief  Data transfer thru DM
 *
 * @param buff_in        [in]  Pointer to input buffer
 * @param buff_in_len   [in]  Size of the input buffer
 * @param buff_out       [out] Pointer to output buffer
 * @param buff_out_len   [in]  Size of output buffer
 *
 * @return CE_Result_Type
 *
 * @see
 *
 */

CeELErrorType CeElCipherDmXfer(uint8 *buff_in, uint32 buff_in_len, uint8 *buff_out, uint32 buff_out_len)
{
    CeELErrorType result = CEEL_ERROR_SUCCESS;

    /* Sanity check inputs */
    if (!buff_out || !buff_out_len) {
        return CEEL_ERROR_INVALID_PARAM;
    }

    if (!buff_in || !buff_in_len) {
        return CEEL_ERROR_INVALID_PARAM;
    }

    // while (!CECL_HW_SEC_CE_STATUS_DIN_RDY);

    HAL_REG_WR(CECL_CE_DATA_IN_BUF_BASE, buff_in);
    HAL_REG_WR(CECL_CE_DATA_IN_BUF_LEN, buff_in_len);

    HAL_REG_WR(CECL_CE_DATA_OUT_BUF_BASE, buff_out);

    if (CEEL_DM_MODE_IS_POLLING()) {
        // Set up polling wait loop for write operation (data input buffer  -> CE -> data output buffer)
        result = CeElDmPollingWait();
        if (CEEL_ERROR_SUCCESS != result) {
            return CEEL_ERROR_FAILURE;
        }
    } else {
        // Set up interrupt wait for write operation (data input buffer  -> CE -> data output buffer)
        result = CeElDmInterruptWait();
        if (CEEL_ERROR_SUCCESS != result) {
            return CEEL_ERROR_FAILURE;
        }
    }

    // Check error conditions
    if ((HAL_REG_RD(CECL_CE_STATUS)) & QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_STATUS_MAC_FAILED_MASK) {
        CeElMemset(buff_out, 0, buff_out_len);
        CeElMemoryBarrier();
        return CEEL_ERROR_FAILURE;
    }

    return result;
}

void clearReg(void)
{
    HW_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_SEG_SIZE_REG, 0);
    HW_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_SIZE_REG, 0);
    HW_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_SEG_SIZE_REG, 0);
    HW_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_SEG_START_REG, 0);
    HW_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_SEG_CFG_REG, 0);
    HW_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_REG, 0);
}

CeELErrorType CeElCryptoDxeXfer(uint8 *buff_in, uint32 buff_in_len, uint8 *buff_out, uint32 buff_out_len)
{
    uint32_t regVal;
    uint32_t dma_to;
    uint32_t dma_from;
    uint32_t rem_data = 0;

    /* Sanity check inputs */
    if (!buff_out || !buff_out_len) {
        return CEEL_ERROR_INVALID_PARAM;
    }

    if (!buff_in || !buff_in_len) {
        return CEEL_ERROR_INVALID_PARAM;
    }
    dma_to = buff_in_len & ~(0x03);
    dma_from = buff_out_len & ~(0x03);

    regVal = ((QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_DATA_IN_REG >> 2) << QWLAN_DXE_0_CH8_DADRL_BASE_OFFSET) | 0x20;
    HW_REG_WR(QWLAN_DXE_0_CH8_DADRL_REG, regVal);
    HW_REG_WR(QWLAN_DXE_0_CH8_DADRH_REG, 0);

    HW_REG_WR(QWLAN_DXE_0_CH8_SADRL_REG, (uint32_t)buff_in);
    HW_REG_WR(QWLAN_DXE_0_CH8_SADRH_REG, 0);

    regVal = (4 << QWLAN_DXE_0_CH8_SZ_CHK_SZ_OFFSET) | ((dma_to)&QWLAN_DXE_0_CH8_SZ_TOT_SZ_MASK);
    HW_REG_WR(QWLAN_DXE_0_CH8_SZ_REG, regVal);

    HW_REG_WR(QWLAN_DXE_0_CH9_DADRL_REG, (uint32_t)buff_out);
    HW_REG_WR(QWLAN_DXE_0_CH9_DADRH_REG, 0);

    regVal = ((QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_DATA_OUT_REG >> 2) << QWLAN_DXE_0_CH9_SADRL_BASE_OFFSET) | 0x20;
    HW_REG_WR(QWLAN_DXE_0_CH9_SADRL_REG, regVal);
    HW_REG_WR(QWLAN_DXE_0_CH9_SADRH_REG, 0);

    regVal = (4 << QWLAN_DXE_0_CH9_SZ_CHK_SZ_OFFSET) | ((dma_from)&QWLAN_DXE_0_CH9_SZ_TOT_SZ_MASK);
    HW_REG_WR(QWLAN_DXE_0_CH9_SZ_REG, regVal);

    regVal = QWLAN_DXE_0_CH8_CTRL_DIQ_MASK | QWLAN_DXE_0_CH8_CTRL_EN_MASK | (8 << QWLAN_DXE_0_CH8_CTRL_CTR_SEL_OFFSET) |
             (QWLAN_DXE_0_CH8_CTRL_ENDIANNESS_ELTLEND << QWLAN_DXE_0_CH8_CTRL_ENDIANNESS_OFFSET);
    HW_REG_WR(QWLAN_DXE_0_CH8_CTRL_REG, regVal);

    regVal = QWLAN_DXE_0_CH9_CTRL_SIQ_MASK | QWLAN_DXE_0_CH9_CTRL_EN_MASK | (9 << QWLAN_DXE_0_CH9_CTRL_CTR_SEL_OFFSET) |
             (QWLAN_DXE_0_CH9_CTRL_ENDIANNESS_ELTLEND << QWLAN_DXE_0_CH9_CTRL_ENDIANNESS_OFFSET);
    HW_REG_WR(QWLAN_DXE_0_CH9_CTRL_REG, regVal);

    {
        if (buff_in_len - dma_to) {
            while (!((regVal = HW_REG_RD(QWLAN_DXE_0_CH8_STATUS_REG)) & QWLAN_DXE_0_CH0_STATUS_DONE_MASK))
                ;
            while (!((regVal = HW_REG_RD(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_STATUS_REG)) &
                     QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_STATUS_DIN_RDY_MASK))
                ;

            uint8_t count = buff_in_len - dma_to;
            if (count == 2) {
                uint16_t tmp = (buff_in[buff_in_len - 1] << 8) | buff_in[buff_in_len - 2];
                *(uint16_t *)QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_DATA_IN_REG = tmp;
                count -= 2;
            } else {
                while (count) {
                    *(uint8_t *)QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_DATA_IN_REG = buff_in[buff_in_len - count];
                    count--;
                }
            }
        }

        while (!((regVal = HW_REG_RD(QWLAN_DXE_0_CH9_STATUS_REG)) & QWLAN_DXE_0_CH0_STATUS_DONE_MASK)) {
            // uint32_t qcc_reset_delay = 0xFFFF;
            // while (--qcc_reset_delay);
            if (regVal & QWLAN_DXE_0_CH0_STATUS_ERR_MASK) {
                printf("CeElCryptoDxeXfer err: 0x%x\n", regVal);
                return CEEL_ERROR_FAILURE;
            }
        }

        if (buff_out_len > dma_from) {
            rem_data = HW_REG_RD(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_DATA_OUT_REG);
            switch (buff_out_len - dma_from) {
                case 3:
                    buff_out[--buff_out_len] = (rem_data >> 16) & 0xff;
                case 2:
                    buff_out[--buff_out_len] = (rem_data >> 8) & 0xff;
                case 1:
                    buff_out[--buff_out_len] = rem_data & 0xff;
                default:
                    break;
            }
        }
    }
    return CEEL_ERROR_SUCCESS;
}

CeELErrorType CeElCryptoDxeShaXfer(uint8 *buff_in, uint32 buff_in_len, uint8 *buff_out, uint32 buff_out_len)

{
    uint32_t regVal;

    if (!buff_in || !buff_in_len) {
        return CEEL_ERROR_INVALID_PARAM;
    }

    regVal = ((QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_DATA_IN_REG >> 2) << QWLAN_DXE_0_CH8_DADRL_BASE_OFFSET) | 0x20;
    HW_REG_WR(QWLAN_DXE_0_CH8_DADRL_REG, regVal);
    HW_REG_WR(QWLAN_DXE_0_CH8_DADRH_REG, 0);

    HW_REG_WR(QWLAN_DXE_0_CH8_SADRL_REG, (uint32_t)buff_in);
    HW_REG_WR(QWLAN_DXE_0_CH8_SADRH_REG, 0);

    regVal = (4 << QWLAN_DXE_0_CH8_SZ_CHK_SZ_OFFSET) | ((buff_in_len)&QWLAN_DXE_0_CH8_SZ_TOT_SZ_MASK);
    HW_REG_WR(QWLAN_DXE_0_CH8_SZ_REG, regVal);

    regVal = QWLAN_DXE_0_CH8_CTRL_DIQ_MASK | QWLAN_DXE_0_CH8_CTRL_EN_MASK | (8 << QWLAN_DXE_0_CH8_CTRL_CTR_SEL_OFFSET) |
             (QWLAN_DXE_0_CH8_CTRL_ENDIANNESS_ELTLEND << QWLAN_DXE_0_CH8_CTRL_ENDIANNESS_OFFSET);
    HW_REG_WR(QWLAN_DXE_0_CH8_CTRL_REG, regVal);

    {
        while (!((regVal = HW_REG_RD(QWLAN_DXE_0_CH8_STATUS_REG)) & QWLAN_DXE_0_CH0_STATUS_DONE_MASK)) {
            if (regVal & QWLAN_DXE_0_CH0_STATUS_ERR_MASK) {
                return CEEL_ERROR_FAILURE;
            }
        }
        while (!(HAL_REG_RD(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_STATUS_REG) &
                 QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_STATUS_OPERATION_DONE_MASK))
            ;
        HW_REG_WR(QWLAN_DXE_0_CH9_DADRL_REG, (uint32_t)buff_out);
        HW_REG_WR(QWLAN_DXE_0_CH9_DADRH_REG, 0);

        HW_REG_WR(QWLAN_DXE_0_CH9_SADRL_REG, CECL_CE_AUTH_IV0);
        HW_REG_WR(QWLAN_DXE_0_CH9_SADRH_REG, 0);

        regVal = (4 << QWLAN_DXE_0_CH9_SZ_CHK_SZ_OFFSET) | ((buff_out_len)&QWLAN_DXE_0_CH9_SZ_TOT_SZ_MASK);
        HW_REG_WR(QWLAN_DXE_0_CH9_SZ_REG, regVal);

        regVal = QWLAN_DXE_0_CH9_CTRL_EN_MASK | (9 << QWLAN_DXE_0_CH9_CTRL_CTR_SEL_OFFSET) |
                 (QWLAN_DXE_0_CH9_CTRL_ENDIANNESS_ELTLEND << QWLAN_DXE_0_CH9_CTRL_ENDIANNESS_OFFSET);
        HW_REG_WR(QWLAN_DXE_0_CH9_CTRL_REG, regVal);

        while (!((regVal = HW_REG_RD(QWLAN_DXE_0_CH9_STATUS_REG)) & QWLAN_DXE_0_CH0_STATUS_DONE_MASK))
            ;
    }
    return CEEL_ERROR_SUCCESS;
}
