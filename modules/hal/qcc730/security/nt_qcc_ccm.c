/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*****************************************************************************

------------------------------------------------------------------------------

------------------------------------------------------------------------------

--                                                                          --

-- QUALCOMM Proprietary                                                     --

-- Copyright (c) 1999-2008 , 2023-2024 QUALCOMM Incorporated. All rights reserved.    --

--                                                                          --

-- All data and information contained in or disclosed by this document are  --

-- confidential and proprietary information of QUALCOMM Incorporated, and   --

-- all rights therein are expressly reserved. By accepting this material,   --

-- the recipient agrees that this material and the information contained    --

-- therein are held in confidence and in trust and will not be used,        --

-- copied, reproduced in whole or in part, nor its contents revealed in     --

-- any manner to others without the express written permission of QUALCOMM  --

-- Incorporated.                                                            --

--                                                                          --

-- This technology was exported from the United States in accordance with   --

-- the Export Administration Regulations. Diversion contrary to U.S. law    --

-- prohibited.                                                              --

--                                                                          --

------------------------------------------------------------------------------

*****************************************************************************/

/*------------------------------------------------------------------------
 * Include Files
 * ----------------------------------------------------------------------*/
#include "nt_hw.h"
#include "nt_osal.h"
#include "hal_int_sys.h"
#include "nt_flags.h"
#include "hal_int_modules.h"
#include "nt_crypto.h"
#include "mbedtls/ccm.h"

#ifdef WIFI_HW_AES_CCM

#include "data_svc_priv.h"

/*------------------------------------------------------------------------
 * Config Macro
 * ----------------------------------------------------------------------*/
/*
 * Enable USE_CRYPTO_PIO_DATA_HANDLING or USE_CRYPTO_DXE_DATA_HANDLING
 * to use PIO or DXE method of data handling to/from the Crypto engine.
 * If both are enabled, it uses DXE if the input block length is word aligned.
 * Otherwise it uses PIO.
 */
#define USE_CRYPTO_PIO_DATA_HANDLING
#define USE_CRYPTO_DXE_DATA_HANDLING

#if !defined(USE_CRYPTO_PIO_DATA_HANDLING) && !defined(USE_CRYPTO_DXE_DATA_HANDLING)
#error "Enable Either or Both PIO and DXE mode of Crypto Data Handling"
#endif /* !defined(USE_CRYPTO_PIO_DATA_HANDLING) && !defined(USE_CRYPTO_DXE_DATA_HANDLING) */

/*
 * Enable USE_CRYPTO_HYBRID_DATA_HANDLING to use both DXE and PIO method of data
 * handling on the same packet. If this macro is enabled, it will follow:
 *    1. If the packet size is aligned to 4 bytes => DXE will be used
 *    2. If the packet size is not aligned to 4 bytes =>
 *         > DXE will be used to process aligned part of the packet.
 *         > PIO will be used to process un-aligned part of the packet.
 */
#define USE_CRYPTO_HYBRID_DATA_HANDLING

#ifdef USE_CRYPTO_HYBRID_DATA_HANDLING
#if !defined(USE_CRYPTO_PIO_DATA_HANDLING) || !defined(USE_CRYPTO_DXE_DATA_HANDLING)
#error "Enable Both PIO and DXE mode of Crypto Data Handling to use HYBRID mode"
#endif /* !defined(USE_CRYPTO_PIO_DATA_HANDLING) || !defined(USE_CRYPTO_DXE_DATA_HANDLING) */
#endif /* USE_CRYPTO_HYBRID_DATA_HANDLING */

/* To enable MBEDTLS self test UT command */
//#define NT_MBEDTLS_SELF_TEST

/* To enable Crypto debug print */
//#define CRYPTO_DEBUG_PRINT

/* To enable Crypto latency profile */
//#define CRYPTO_LATENCY_PROFILE

/*------------------------------------------------------------------------
 * Conditional Inclusion
 * ----------------------------------------------------------------------*/
#ifdef USE_CRYPTO_DXE_DATA_HANDLING
#include "dxe.h"
#endif /* USE_CRYPTO_DXE_DATA_HANDLING */

#ifdef NT_MBEDTLS_SELF_TEST
#include "ccm_alt.h"
#ifdef USE_CRYPTO_DXE_DATA_HANDLING
#include "dxe_api.h"
#endif /* USE_CRYPTO_DXE_DATA_HANDLING */
#endif /* NT_MBEDTLS_SELF_TEST */

#ifdef CRYPTO_LATENCY_PROFILE
#include "timer.h"
#endif /* CRYPTO_LATENCY_PROFILE */

/*------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * ----------------------------------------------------------------------*/
#define WIFI_FW_ERR_CCM_BAD_INPUT                0xFF
#define SW_CRYPTO_MODE                           0x02
#define AES_CCM128_AUTH_CONFIG_FOR_ENCRYPTION    0x00030002
#define AES_CCM128_AUTH_CONFIG_FOR_DECRYPTION    0x01034002
#define AES_CCM128_ENCRYPT_CONFIG_FOR_ENCRYPTION 0x00002502
#define AES_CCM128_ENCRYPT_CONFIG_FOR_DECRYPTION 0x00002102
#define AES_ENCRYPT                              1 /**< AES encryption. */
#define AES_DECRYPT                              0 /**< AES decryption. */
#define AES_CCM_SUCCESS                          true
#define AES_CCM_FAIL                             false

#define CRYPTO_MAX_BUFF_LENGTH 1660
#define CRYPTO_OVERHEAD_LENGTH 32 /* 16 bytes of Add Data + 16 Bytes of MAC */
#define CRYPTO_CTR_LEN         16
#define CRYPTO_B0_LEN          16

#define DXE_MAX_WAIT_MS 100

#ifdef NT_MBEDTLS_SELF_TEST
#define NB_TESTS                8
#define CCM_SELFTEST_PT_MAX_LEN 128
#define CCM_SELFTEST_CT_MAX_LEN 128
#endif /* NT_MBEDTLS_SELF_TEST */

/*------------------------------------------------------------------------
 * static Data
 * ----------------------------------------------------------------------*/
#ifdef NT_MBEDTLS_SELF_TEST
/*
 * The data is the same for all tests, only the used length changes
 */
static const unsigned char key[] = {0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
                                    0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f};

static const unsigned char iv[] = {0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b};

static const unsigned char ad[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
                                   0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13};

static const unsigned char msg[CCM_SELFTEST_PT_MAX_LEN] = {
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b,
    0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
};

static const size_t iv_len[NB_TESTS] = {7, 8, 12, 12, 12, 12, 12, 12};
static const size_t add_len[NB_TESTS] = {8, 16, 20, 13, 11, 13, 13, 13};
static const size_t msg_len[NB_TESTS] = {4, 16, 24, 1, 2, 3, 4, 5};
static const size_t tag_len[NB_TESTS] = {4, 6, 8, 16, 16, 16, 16, 16};

static const unsigned char res[NB_TESTS][CCM_SELFTEST_CT_MAX_LEN] = {
    {0x71, 0x62, 0x01, 0x5b, 0x4d, 0xac, 0x25, 0x5d},
    {0xd2, 0xa1, 0xf0, 0xe0, 0x51, 0xea, 0x5f, 0x62, 0x08, 0x1a, 0x77,
     0x92, 0x07, 0x3d, 0x59, 0x3d, 0x1f, 0xc6, 0x4f, 0xbf, 0xac, 0xcd},
    {0xe3, 0xb2, 0x01, 0xa9, 0xf5, 0xb7, 0x1a, 0x7a, 0x9b, 0x1c, 0xea, 0xec, 0xcd, 0x97, 0xe7, 0x0b,
     0x61, 0x76, 0xaa, 0xd9, 0xa4, 0x42, 0x8a, 0xa5, 0x48, 0x43, 0x92, 0xfb, 0xc1, 0xb0, 0x99, 0x51}};
#endif /* NT_MBEDTLS_SELF_TEST */

typedef struct {
    uint8_t counter[CRYPTO_CTR_LEN];
    uint8_t b0[CRYPTO_B0_LEN];
    uint32_t auth_size;
    uint8_t *crypto_in_buff;
    uint8_t *crypto_out_buff;
#ifdef USE_CRYPTO_DXE_DATA_HANDLING
    nt_osal_semaphore_handle_t crypto_dxe_semaphore_handle;
#endif /* USE_CRYPTO_DXE_DATA_HANDLING */
} crypto_ctx_t;

static crypto_ctx_t crypto_ctx = {0};

static uint32_t aes_ccm_last_seg_tot_in_len = 0;
static uint32_t aes_ccm_last_seg_tot_out_len = 0;
static uint32_t aes_ccm_last_seg_aligned_in_len = 0;
static uint32_t aes_ccm_last_seg_unaligned_in_len = 0;
static uint32_t aes_ccm_last_seg_aligned_out_len = 0;
static uint32_t aes_ccm_last_seg_unaligned_out_len = 0;

/*------------------------------------------------------------------------
 * static Function Definitions
 * ----------------------------------------------------------------------*/
#ifdef USE_CRYPTO_DXE_DATA_HANDLING
/**
 * @brief  Driver for write and read data to/from QCC module using DXE method
 * @param  ce_data_in         : Pointer to in data
 * @param  ce_data_out        : Pointer to out data
 * @param  din_limit          : Size of Data in
 * @param  dout_limit         : Size of Data out
 * @return                    : AES_CCM_SUCCESS for Encryption/Decryption success
 *                            : AES_CCM_FAIL for Encryption/Decryption failure
 */
static bool crypto_arm_ccm_dxe(const uint8_t *ce_data_in, uint8_t *ce_data_out, uint32_t din_limit, uint32_t dout_limit)
{
    uint32_t regVal;
    TickType_t dxe_ticks_to_wait = DXE_MAX_WAIT_MS / portTICK_PERIOD_MS;

#ifdef CRYPTO_DEBUG_PRINT
    NT_LOG_PRINT(SECURITY, INFO, "DXE din_limit : %d\n", din_limit);
    NT_LOG_PRINT(SECURITY, INFO, "DXE dout_limit : %d\n", dout_limit);
#endif /* CRYPTO_DEBUG_PRINT */

    NT_LOG_PRINT(SECURITY, INFO, "DXE");

    /* Semaphore must be available at this point.
       if not, some crypto operation is ongoing. Which is unlikely. */
    if (nt_fail == nt_osal_semaphore_take(crypto_ctx.crypto_dxe_semaphore_handle, dxe_ticks_to_wait)) {
        NT_LOG_PRINT(SECURITY, ERR, "Failed to take Crypto DXE semaphore");
        /* No point in returning error. Something is very wrong */
        assert(0);
    }

    regVal = ((QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_DATA_IN_REG >> 2) << QWLAN_DXE_0_CH8_DADRL_BASE_OFFSET) | 0x20;
    HW_REG_WR(QWLAN_DXE_0_CH8_DADRL_REG, regVal);
    HW_REG_WR(QWLAN_DXE_0_CH8_DADRH_REG, 0);

    HW_REG_WR(QWLAN_DXE_0_CH8_SADRL_REG, (uint32_t)ce_data_in);
    HW_REG_WR(QWLAN_DXE_0_CH8_SADRH_REG, 0);

    regVal = (4 << QWLAN_DXE_0_CH8_SZ_CHK_SZ_OFFSET) | ((din_limit)&QWLAN_DXE_0_CH8_SZ_TOT_SZ_MASK);
    HW_REG_WR(QWLAN_DXE_0_CH8_SZ_REG, regVal);

    HW_REG_WR(QWLAN_DXE_0_CH9_DADRL_REG, (uint32_t)ce_data_out);
    HW_REG_WR(QWLAN_DXE_0_CH9_DADRH_REG, 0);

    regVal = ((QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_DATA_OUT_REG >> 2) << QWLAN_DXE_0_CH9_SADRL_BASE_OFFSET) | 0x20;
    HW_REG_WR(QWLAN_DXE_0_CH9_SADRL_REG, regVal);
    HW_REG_WR(QWLAN_DXE_0_CH9_SADRH_REG, 0);

    regVal = (4 << QWLAN_DXE_0_CH9_SZ_CHK_SZ_OFFSET) | ((dout_limit)&QWLAN_DXE_0_CH9_SZ_TOT_SZ_MASK);
    HW_REG_WR(QWLAN_DXE_0_CH9_SZ_REG, regVal);

    regVal = QWLAN_DXE_0_CH8_CTRL_DIQ_MASK | QWLAN_DXE_0_CH8_CTRL_INE_ERR_MASK | QWLAN_DXE_0_CH8_CTRL_INE_DONE_MASK |
             (8 << QWLAN_DXE_0_CH8_CTRL_CTR_SEL_OFFSET) |
             (QWLAN_DXE_0_CH8_CTRL_ENDIANNESS_ELTLEND << QWLAN_DXE_0_CH8_CTRL_ENDIANNESS_OFFSET);
    HW_REG_WR(QWLAN_DXE_0_CH8_CTRL_REG, regVal);

    regVal = QWLAN_DXE_0_CH9_CTRL_SIQ_MASK | QWLAN_DXE_0_CH9_CTRL_INE_ERR_MASK | QWLAN_DXE_0_CH9_CTRL_INE_DONE_MASK |
             (9 << QWLAN_DXE_0_CH9_CTRL_CTR_SEL_OFFSET) |
             (QWLAN_DXE_0_CH9_CTRL_ENDIANNESS_ELTLEND << QWLAN_DXE_0_CH9_CTRL_ENDIANNESS_OFFSET);
    HW_REG_WR(QWLAN_DXE_0_CH9_CTRL_REG, regVal);

    regVal = HW_REG_RD(QWLAN_DXE_0_CH9_CTRL_REG);
    regVal |= QWLAN_DXE_0_CH9_CTRL_EN_MASK;
    HW_REG_WR(QWLAN_DXE_0_CH9_CTRL_REG, regVal);

    regVal = HW_REG_RD(QWLAN_DXE_0_CH8_CTRL_REG);
    regVal |= QWLAN_DXE_0_CH8_CTRL_EN_MASK;
    HW_REG_WR(QWLAN_DXE_0_CH8_CTRL_REG, regVal);

    /* Semaphore is already taken. And it is released from the DXE channel 9 Done interrupt.
       This thread will be block till the channel Done interrupt occurs on DXE channel 9 */
    if (nt_fail == nt_osal_semaphore_take(crypto_ctx.crypto_dxe_semaphore_handle, dxe_ticks_to_wait)) {
        /* Timeout has happened: Need to reset QCC state */
        regVal = HW_REG_RD(QWLAN_CCU_R_CCU_SOFT_RESET_REG);
        regVal |= QWLAN_CCU_R_CCU_SOFT_RESET_QCC_SOFT_RESET_MASK;
        HW_REG_WR(QWLAN_CCU_R_CCU_SOFT_RESET_REG, regVal);

        uint32_t qcc_reset_delay = 0xFFFF;
        while (--qcc_reset_delay)
            ;

        regVal = HW_REG_RD(QWLAN_CCU_R_CCU_SOFT_RESET_REG);
        regVal &= ~QWLAN_CCU_R_CCU_SOFT_RESET_QCC_SOFT_RESET_MASK;
        HW_REG_WR(QWLAN_CCU_R_CCU_SOFT_RESET_REG, regVal);

        NT_LOG_PRINT(SECURITY, ERR, "Warn: Crypto DXE Semaphore Timed Out: QCC RESET");

        /* Releasing the semaphore as semaphore wait timed out */
        if (nt_fail == nt_osal_semaphore_give(crypto_ctx.crypto_dxe_semaphore_handle)) {
            NT_LOG_PRINT(SECURITY, ERR, "Failed to release Crypto DXE semaphore");
        }

        NT_LOG_PRINT(SECURITY, ERR, "DXE In Len: %d\n", din_limit);
        NT_LOG_PRINT(SECURITY, ERR, "DXE Out Len: %d\n", dout_limit);
        NT_LOG_PRINT(SECURITY, ERR, "FDXE ch8 SZ: %08X", HW_REG_RD(QWLAN_DXE_0_CH8_SZ_REG));
        NT_LOG_PRINT(SECURITY, ERR, "FDXE ch9 SZ: %08X", HW_REG_RD(QWLAN_DXE_0_CH9_SZ_REG));
        return AES_CCM_FAIL;
    }

    /* Got the Semaphore as it is released from the ISR. Now releasing it to make it available */
    if (nt_fail == nt_osal_semaphore_give(crypto_ctx.crypto_dxe_semaphore_handle)) {
        NT_LOG_PRINT(SECURITY, ERR, "Failed to release Crypto DXE semaphore");
    }

    return AES_CCM_SUCCESS;
}
#endif /* USE_CRYPTO_DXE_DATA_HANDLING */

#ifdef USE_CRYPTO_PIO_DATA_HANDLING
/**
 * @brief  Gets the encr_size of data available to write into Data in registers
 * @return Size
 */
static uint32_t crypto_arm_available_in_size(void)
{
    uint32_t ret = 0;
    uint32_t status = HAL_REG_RD(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_STATUS_REG);
    if (((QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_STATUS_DIN_RDY_MASK)&status) &&
        !((QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_STATUS_SW_ERR_MASK |
           QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_STATUS_DIN_ERR_MASK |
           QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_STATUS_ERR_INTR_MASK) &
          status)) {
        ret = (uint32_t)((QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_STATUS_DIN_SIZE_AVAIL_MASK & status) >>
                         QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_STATUS_DIN_SIZE_AVAIL_OFFSET);
        return ret;
    }
    return ret;
}

/**
 * @brief  Gets the encr_size of data available to read from Data out registers
 * @return Size
 */
static uint32_t crypto_arm_available_out_size(void)
{
    uint32_t ret = 0;
    uint32_t status = HAL_REG_RD(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_STATUS_REG);
    if (((QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_STATUS_DOUT_RDY_MASK)&status) &&
        !((QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_STATUS_SW_ERR_MASK |
           QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_STATUS_DOUT_ERR_MASK |
           QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_STATUS_ERR_INTR_MASK) &
          status)) {
        ret = (uint32_t)((QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_STATUS_DOUT_SIZE_AVAIL_MASK & status) >>
                         QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_STATUS_DOUT_SIZE_AVAIL_OFFSET);
        return ret;
    }
    return ret;
}

/**
 * @brief  Returns 0 or 1 based on errors or operation status
 * @return State
 */
static uint32_t crypto_error(void)
{
    uint32_t ret = 0;
    uint32_t status = HAL_REG_RD(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_STATUS_REG);
    if ((QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_STATUS_SW_ERR_MASK | QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_STATUS_DOUT_ERR_MASK |
         QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_STATUS_ERR_INTR_MASK |
         QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_STATUS_DIN_ERR_MASK |
         QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_STATUS_OP_DONE_INTR_MASK) &
        status) {
        ret = 1;
        return ret;
    }
    return ret;
}

/**
 * @brief  Driver for write and read data to/from QCC module using PIO method
 * @param  ce_data_in         : Pointer to in data
 * @param  ce_data_out        : Pointer to out data
 * @param  din_limit          : Size of Data in
 * @param  dout_limit         : Size of Data out
 * @return void
 */
static void crypto_arm_ccm(const uint8_t *ce_data_in, uint8_t *ce_data_out, uint32_t din_limit, uint32_t dout_limit)
{
    uint32_t read = 0;
    uint32_t written = 0;
    uint32_t available_in_size = 0;
    uint32_t available_out_size = 0;
    uint32_t out_data = 0;
    uint32_t crypto_status = 0;

#ifdef CRYPTO_DEBUG_PRINT
    NT_LOG_PRINT(SECURITY, INFO, "PIO din_limit : %d\n", din_limit);
    NT_LOG_PRINT(SECURITY, INFO, "PIO dout_limit : %d\n", dout_limit);
#endif /* CRYPTO_DEBUG_PRINT */

    NT_LOG_PRINT(SECURITY, INFO, "PIO");

    while (1) {
        if (crypto_error()) {
            NT_LOG_PRINT(SECURITY, ERR, "Crypto Error");
            break;
        }

        if ((written >= din_limit) && (read >= dout_limit)) {
            /* Expecting Operation Done bit to be set when all data read/write are done */
            crypto_status = HAL_REG_RD(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_STATUS_REG);
            if ((crypto_status & QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_STATUS_OPERATION_DONE_MASK) !=
                QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_STATUS_OPERATION_DONE_MASK) {
                /* Something is wrong with the input or output data length */
                assert(0);
            }

            /* Checking if there is any data left to be read from the DATA_OUT_REG.
               It may happen if AUTH_SIZE is not set properly in AUTH_SEG_CFG */
            available_out_size = crypto_arm_available_out_size();
            if (available_out_size > 0) {
#ifdef CRYPTO_DEBUG_PRINT
                NT_LOG_PRINT(SECURITY, INFO, "%d Bytes of EXTRA data available in DATA_OUT_REG", available_out_size);
#endif /* CRYPTO_DEBUG_PRINT */
            }

            /* All operations are Done. Out data is ready in ce_data_out[] */
            break;
        } else {
            crypto_status = HAL_REG_RD(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_STATUS_REG);
            if ((crypto_status & QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_STATUS_OPERATION_DONE_MASK) ==
                QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_STATUS_OPERATION_DONE_MASK) {
                /* Something is wrong. Some data corruption might have happened */
                NT_LOG_PRINT(SECURITY, ERR, "CRYPTO CORRUPTION written: %d, din_limit: %d", written, din_limit);
                NT_LOG_PRINT(SECURITY, ERR, "CRYPTO CORRUPTION read: %d, dout_limit: %d", read, dout_limit);
                NT_LOG_PRINT(SECURITY, ERR, "i/p len: %d", aes_ccm_last_seg_tot_in_len);
                NT_LOG_PRINT(SECURITY, ERR, "o/p len: %d", aes_ccm_last_seg_tot_out_len);
                NT_LOG_PRINT(SECURITY, ERR, "aligned i/p len: %d", aes_ccm_last_seg_aligned_in_len);
                NT_LOG_PRINT(SECURITY, ERR, "unaligned i/p len: %d", aes_ccm_last_seg_unaligned_in_len);
                NT_LOG_PRINT(SECURITY, ERR, "aligned o/p len: %d", aes_ccm_last_seg_aligned_out_len);
                NT_LOG_PRINT(SECURITY, ERR, "unaligned o/p len: %d", aes_ccm_last_seg_unaligned_out_len);
                NT_LOG_PRINT(SECURITY, ERR, "QCC Status: 0x%08x",
                             HAL_REG_RD(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_STATUS_REG));
                break;
            }
        }

        if (din_limit > written) {
            available_in_size = crypto_arm_available_in_size();
            while (available_in_size) {
                *(uint8_t *)QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_DATA_IN_REG = *(uint8_t *)(ce_data_in + written);
                written += 1;
                available_in_size -= 1;
                if (written >= din_limit) {
                    break;
                }
            }
        }

        if (read < dout_limit) {
            available_out_size = crypto_arm_available_out_size();
            while (available_out_size) {
                uint8_t out_data_size = 0;
                out_data = HAL_REG_RD(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_DATA_OUT_REG);
                if (available_out_size >= sizeof(out_data)) {
                    out_data_size = sizeof(out_data);
                } else {
                    out_data_size = available_out_size;
                }
                if (read < dout_limit) {
                    uint8_t byte_count = 0;
                    while (byte_count < out_data_size) {
                        ce_data_out[read + byte_count] = (uint8_t)(out_data >> (byte_count * 8));
                        byte_count++;
                    }
                }
                read += out_data_size;
                available_out_size -= out_data_size;

                if (read >= dout_limit) {
                    break;
                }
            }
        }
    }
}
#endif /* USE_CRYPTO_PIO_DATA_HANDLING */

#ifdef USE_CRYPTO_HYBRID_DATA_HANDLING
/**
 * @brief  Driver for write and read data to/from QCC module using both DXE and PIO method.
 *         Aligned length of data is processed using DXE and remaining length is processed using PIO.
 * @param  ce_data_in         : Pointer to in data
 * @param  ce_data_out        : Pointer to out data
 * @param  din_limit          : Size of Data in
 * @param  dout_limit         : Size of Data out
 * @return void
 */
static void crypto_arm_ccm_hybrid(const uint8_t *ce_data_in, uint8_t *ce_data_out, uint32_t din_limit,
                                  uint32_t dout_limit)
{
    uint32_t aligned_input_length = din_limit;
    uint32_t aligned_output_length = dout_limit;
    uint32_t remain_input_length = 0;
    uint32_t remain_output_length = 0;

    if ((dout_limit & 0x03) || (din_limit & 0x03)) {
        /* If dout_limit or din_limit is not aligned to 4 bytes */
        aligned_input_length = (din_limit >> 2) << 2;
        remain_input_length = din_limit & 0x3;

        if (din_limit < dout_limit) {
            /* For encryption, aligned_output_length should be less than equal to
               aligned_input_length and it should be aligned to 16 bytes */
            aligned_output_length = (aligned_input_length >> 4) << 4;
            remain_output_length = dout_limit - aligned_output_length;
        } else {
            /* For decryption, aligned_output_length should be aligned to 16 bytes */
            aligned_output_length = (dout_limit >> 4) << 4;
            remain_output_length = dout_limit & 0xF;
        }
    } else {
        /* If din_limit and dout_limit both are aligned to 4 bytes,
           we choose to use DXE to process the full packet.
           aligned_input_length is already equal to din_limit and
           aligned_output_length is already equal to dout_limit and
           both remain_input_length and remain_output_length are 0.
           We can proceed with DXE. */
    }

#ifdef CRYPTO_DEBUG_PRINT
    NT_LOG_PRINT(SECURITY, INFO, "Input Length aligned  : %d\n", aligned_input_length);
    NT_LOG_PRINT(SECURITY, INFO, "Input Length remain   : %d\n", remain_input_length);
    NT_LOG_PRINT(SECURITY, INFO, "Output Length aligned : %d\n", aligned_output_length);
    NT_LOG_PRINT(SECURITY, INFO, "Output Length remain  : %d\n", remain_output_length);
#endif /* CRYPTO_DEBUG_PRINT */

    aes_ccm_last_seg_tot_in_len = din_limit;
    aes_ccm_last_seg_tot_out_len = dout_limit;
    aes_ccm_last_seg_aligned_in_len = aligned_input_length;
    aes_ccm_last_seg_unaligned_in_len = remain_input_length;
    aes_ccm_last_seg_aligned_out_len = aligned_output_length;
    aes_ccm_last_seg_unaligned_out_len = remain_output_length;

    /* Process the aligned part of the packet */
    if (AES_CCM_SUCCESS == crypto_arm_ccm_dxe(ce_data_in, ce_data_out, aligned_input_length, aligned_output_length)) {
        /* Process the un-aligned part of the packet (if any) */
        if ((remain_input_length != 0) || (remain_output_length != 0)) {
            crypto_arm_ccm(ce_data_in + aligned_input_length, ce_data_out + aligned_output_length, remain_input_length,
                           remain_output_length);
        }
    }
}
#endif /* USE_CRYPTO_HYBRID_DATA_HANDLING */

/**
 * @brief  Function exposed to mbedtls for both ecryption and decryption
 * @param  mode          : Mode for encryption and decryption
 * @param  key           : Pointer to the key
 * @param  crypto_ctx    : Pointer to the struct crypto_ctx_t
 * @param  data_len      : Size of data to be encrypted or decrypted
 * @param  tag_len       : Size of tag aka message authentication code (mac)
 * @return               : AES_CCM_SUCCESS for Encryption/Decryption Success
 *                         AES_CCM_FAIL for Encryption/Decryption Failure
 */
static bool aes128_ccm(uint16_t mode, uint8_t *key, crypto_ctx_t *crypto_ctx, uint32_t data_len, uint32_t tag_len)
{
    uint32_t *encr_key = (uint32_t *)key;
    uint32_t *cntr_iv = (uint32_t *)(crypto_ctx->counter);
    uint32_t *nonce = (uint32_t *)(crypto_ctx->b0);
    uint32_t input_size = 0;
    uint32_t output_size = 0;
    uint32_t crypto_status = 0;

    if (!(HAL_REG_RD(QWLAN_PMU_SECIP_GDSCR_REG) & QWLAN_PMU_SECIP_GDSCR_GDS_CTL_PWR_STATUS_MASK)) {
#ifdef CRYPTO_DEBUG_PRINT
        NT_LOG_PRINT(SECURITY, ERR, "SecIP PD is turned off, turning SecIP PD on locally");
#endif /* CRYPTO_DEBUG_PRINT */
        hal_secip_sw_power_req(1);
    }

    crypto_status = HAL_REG_RD(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_STATUS_REG);

#ifdef CRYPTO_DEBUG_PRINT
    NT_LOG_PRINT(SECURITY, INFO, "size of tag_size : %d\n", tag_len);
    NT_LOG_PRINT(SECURITY, INFO, "size of encr_size: %d\n", data_len);
    NT_LOG_PRINT(SECURITY, INFO, "size of auth_size: %d\n", crypto_ctx->auth_size);

    NT_LOG_PRINT(SECURITY, INFO, "********QCC AES_128_CCM********\n");
    NT_LOG_PRINT(SECURITY, INFO, "********Operation Start********\n");
    NT_LOG_PRINT(SECURITY, INFO, "******status: 0x%08x*******\n", crypto_status);
#endif /* CRYPTO_DEBUG_PRINT */

    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_CORE_CFG_REG, SW_CRYPTO_MODE);

    if (AES_ENCRYPT == mode) {
        HAL_REG_WR(
            QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_SEG_CFG_REG,
            ((AES_CCM128_AUTH_CONFIG_FOR_ENCRYPTION & ~QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_SEG_CFG_AUTH_SIZE_MASK) |
             ((tag_len - 1) << QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_SEG_CFG_AUTH_SIZE_OFFSET)));
#ifdef CRYPTO_DEBUG_PRINT
        NT_LOG_PRINT(SECURITY, INFO, "Encryption AUTH_SEG_CFG: 0x%08x\n",
                     HAL_REG_RD(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_SEG_CFG_REG));
#endif /* CRYPTO_DEBUG_PRINT */
    } else {
        HAL_REG_WR(
            QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_SEG_CFG_REG,
            ((AES_CCM128_AUTH_CONFIG_FOR_DECRYPTION & ~QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_SEG_CFG_AUTH_SIZE_MASK) |
             ((tag_len - 1) << QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_SEG_CFG_AUTH_SIZE_OFFSET)));
#ifdef CRYPTO_DEBUG_PRINT
        NT_LOG_PRINT(SECURITY, INFO, "Decryption AUTH_SEG_CFG: 0x%08x\n",
                     HAL_REG_RD(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_SEG_CFG_REG));
#endif /* CRYPTO_DEBUG_PRINT */
    }

    if (AES_ENCRYPT == mode) {
        //  formatted A_length + P_length
        HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_SEG_SIZE_REG, crypto_ctx->auth_size + data_len);
#ifdef CRYPTO_DEBUG_PRINT
        NT_LOG_PRINT(SECURITY, INFO, "Encryption CRYPTO_SEG_SIZE: %d\n", crypto_ctx->auth_size + data_len);
#endif /* CRYPTO_DEBUG_PRINT */
    } else {
        //  formatted A_length + P_length + MAC Length
        HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_SEG_SIZE_REG, crypto_ctx->auth_size + data_len + tag_len);
#ifdef CRYPTO_DEBUG_PRINT
        NT_LOG_PRINT(SECURITY, INFO, "Decryption CRYPTO_SEG_SIZE: %d\n", crypto_ctx->auth_size + data_len + tag_len);
#endif /* CRYPTO_DEBUG_PRINT */
    }

    // Authentication segment start
    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_SEG_START_REG, 0);
#ifdef CRYPTO_DEBUG_PRINT
    NT_LOG_PRINT(SECURITY, INFO, "AUTH_SEG_START: %d\n", 0);
#endif /* CRYPTO_DEBUG_PRINT */

    //  formatted A_length + P_length
    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_SEG_SIZE_REG, crypto_ctx->auth_size + data_len);
#ifdef CRYPTO_DEBUG_PRINT
    NT_LOG_PRINT(SECURITY, INFO, "AUTH_SEG_SIZE: %d\n", crypto_ctx->auth_size + data_len);
#endif /* CRYPTO_DEBUG_PRINT */

    // write authentication init.vectors
    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_IV0_REG, 0x0);  // for AES128 ...
    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_IV1_REG, 0x0);
    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_IV2_REG, 0x0);
    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_IV3_REG, 0x0);

    // write nonce
    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_INFO_NONCE0_REG, nonce[0]);
    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_INFO_NONCE1_REG, nonce[1]);
    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_INFO_NONCE2_REG, nonce[2]);
    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_INFO_NONCE3_REG, nonce[3]);

    if (AES_ENCRYPT == mode) {
        HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_REG, AES_CCM128_ENCRYPT_CONFIG_FOR_ENCRYPTION);
#ifdef CRYPTO_DEBUG_PRINT
        NT_LOG_PRINT(SECURITY, INFO, "Encryption ENCR_SEG_CFG: 0x%08x\n",
                     HAL_REG_RD(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_REG));
#endif /* CRYPTO_DEBUG_PRINT */

        // encryption segment size
        HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_SIZE_REG, data_len);
#ifdef CRYPTO_DEBUG_PRINT
        NT_LOG_PRINT(SECURITY, INFO, "Encryption ENCR_SEG_SIZE: %d\n", data_len);
#endif       /* CRYPTO_DEBUG_PRINT */
    } else { /* (AES_DECRYPT == mode) */
        HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_REG, AES_CCM128_ENCRYPT_CONFIG_FOR_DECRYPTION);
#ifdef CRYPTO_DEBUG_PRINT
        NT_LOG_PRINT(SECURITY, INFO, "Decryption ENCR_SEG_CFG: 0x%08x\n",
                     HAL_REG_RD(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_REG));
#endif /* CRYPTO_DEBUG_PRINT */

        // encryption segment size
        HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_SIZE_REG, data_len + tag_len);
#ifdef CRYPTO_DEBUG_PRINT
        NT_LOG_PRINT(SECURITY, INFO, "Decryption ENCR_SEG_SIZE: %d\n", data_len + tag_len);
#endif /* CRYPTO_DEBUG_PRINT */
    }

    // encryption segment start
    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_START_REG, crypto_ctx->auth_size);
#ifdef CRYPTO_DEBUG_PRINT
    NT_LOG_PRINT(SECURITY, INFO, "ENCR_SEG_START: %d\n", crypto_ctx->auth_size);
#endif /* CRYPTO_DEBUG_PRINT */

    // write counter IV
    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_CNTR0_IV0_REG, cntr_iv[0]);
    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_CNTR1_IV1_REG, cntr_iv[1]);
    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_CNTR2_IV2_REG, cntr_iv[2]);
    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_CNTR3_IV3_REG, 0x01000000 | cntr_iv[3]);

    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_CNTR_MASK0_REG, 0x0000FFFF);
    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_CNTR_MASK1_REG, 0x0);
    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_CNTR_MASK2_REG, 0x0);
    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_CNTR_MASK_REG, 0x0);

    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_CCM_INIT_CNTR0_REG, cntr_iv[0]);
    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_CCM_INIT_CNTR1_REG, cntr_iv[1]);
    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_CCM_INIT_CNTR2_REG, cntr_iv[2]);
    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_CCM_INIT_CNTR3_REG, cntr_iv[3]);

    // write authentication keys
    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_SW_KEY1_0_REG, encr_key[0]);
    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_SW_KEY1_1_REG, encr_key[1]);
    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_SW_KEY1_2_REG, encr_key[2]);
    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_SW_KEY1_3_REG, encr_key[3]);
    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_KEY_TABLE_CFG_REG, 0x11);

    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_GOPROC_REG, 0x1);

    if (AES_ENCRYPT == mode) {
        input_size = crypto_ctx->auth_size + data_len;
        output_size = input_size + tag_len;
    } else { /* (AES_DECRYPT == mode) */
        input_size = crypto_ctx->auth_size + data_len + tag_len;
        output_size = input_size - tag_len;
    }

#ifdef USE_CRYPTO_HYBRID_DATA_HANDLING
    crypto_arm_ccm_hybrid(crypto_ctx->crypto_in_buff, crypto_ctx->crypto_out_buff, input_size, output_size);
#else /* USE_CRYPTO_HYBRID_DATA_HANDLING */
#if defined(USE_CRYPTO_PIO_DATA_HANDLING) && defined(USE_CRYPTO_DXE_DATA_HANDLING)
    if ((output_size & 0x03) || (input_size & 0x03)) {
        /* calling PIO method of Crypto data handling API if
           either input_size or output_size is not word aligned. */
        crypto_arm_ccm(crypto_ctx->crypto_in_buff, crypto_ctx->crypto_out_buff, input_size, output_size);
    } else {
        /* calling DXE method of Crypto data handling API if
           both input_size and output_size is word aligned. */
        crypto_arm_ccm_dxe(crypto_ctx->crypto_in_buff, crypto_ctx->crypto_out_buff, input_size, output_size);
    }
#elif defined(USE_CRYPTO_PIO_DATA_HANDLING)
    crypto_arm_ccm(crypto_ctx->crypto_in_buff, crypto_ctx->crypto_out_buff, input_size, output_size);
#elif defined(USE_CRYPTO_DXE_DATA_HANDLING)
    crypto_arm_ccm_dxe(crypto_ctx->crypto_in_buff, crypto_ctx->crypto_out_buff, input_size, output_size);
#endif
#endif /* USE_CRYPTO_HYBRID_DATA_HANDLING */

    crypto_status = HAL_REG_RD(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_STATUS_REG);

    if ((crypto_status & QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_STATUS_OPERATION_DONE_MASK) ==
        QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_STATUS_OPERATION_DONE_MASK) {
        if ((crypto_status & QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_STATUS_MAC_FAILED_MASK) ==
            QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_STATUS_MAC_FAILED_MASK) {
            NT_LOG_PRINT(SECURITY, ERR, "Crypto HW AES MAC Authentication Failed! AL: %d DL: %d TL: %d ",
                         crypto_ctx->auth_size, data_len, tag_len);
            NT_LOG_PRINT(SECURITY, ERR, "******status: 0x%08x*******\n", crypto_status);
            NT_LOG_PRINT(SECURITY, ERR, "i/p len: %d", aes_ccm_last_seg_tot_in_len);
            NT_LOG_PRINT(SECURITY, ERR, "o/p len: %d", aes_ccm_last_seg_tot_out_len);
            NT_LOG_PRINT(SECURITY, ERR, "aligned i/p len: %d", aes_ccm_last_seg_aligned_in_len);
            NT_LOG_PRINT(SECURITY, ERR, "unaligned i/p len: %d", aes_ccm_last_seg_unaligned_in_len);
            NT_LOG_PRINT(SECURITY, ERR, "aligned o/p len: %d", aes_ccm_last_seg_aligned_out_len);
            NT_LOG_PRINT(SECURITY, ERR, "unaligned o/p len: %d", aes_ccm_last_seg_unaligned_out_len);
        } else {
#ifdef CRYPTO_DEBUG_PRINT
            NT_LOG_PRINT(SECURITY, INFO, "Crypto HW AES MAC Authentication Passed");
            NT_LOG_PRINT(SECURITY, INFO, "******status: 0x%08x*******\n", crypto_status);
#endif /* CRYPTO_DEBUG_PRINT */
            return AES_CCM_SUCCESS;
        }
    } else {
        NT_LOG_PRINT(SECURITY, ERR, "Crypto HW AES Encryption/Decryption Failed! AL: %d DL: %d TL: %d ",
                     crypto_ctx->auth_size, data_len, tag_len);
        NT_LOG_PRINT(SECURITY, ERR, "******status: 0x%08x*******\n", crypto_status);
        NT_LOG_PRINT(SECURITY, ERR, "i/p len: %d", aes_ccm_last_seg_tot_in_len);
        NT_LOG_PRINT(SECURITY, ERR, "o/p len: %d", aes_ccm_last_seg_tot_out_len);
        NT_LOG_PRINT(SECURITY, ERR, "aligned i/p len: %d", aes_ccm_last_seg_aligned_in_len);
        NT_LOG_PRINT(SECURITY, ERR, "unaligned i/p len: %d", aes_ccm_last_seg_unaligned_in_len);
        NT_LOG_PRINT(SECURITY, ERR, "aligned o/p len: %d", aes_ccm_last_seg_aligned_out_len);
        NT_LOG_PRINT(SECURITY, ERR, "unaligned o/p len: %d", aes_ccm_last_seg_unaligned_out_len);
    }

    return AES_CCM_FAIL;
}

/**
 * @brief  Function to derive bo 0 data aka first block first element
 * @param  iv_len  : Length of initialization vector
 * @param  add_len : Length of addtional data
 * @param  tag_len : Tag aka Mac length
 * @return value of bo
 */
static uint8_t ccm_b0_0(uint16_t iv_len, uint16_t add_len, uint16_t tag_len)
{
    /* With flags as (bits):
     * 7        0
     * 6        add present?
     * 5 .. 3   (t - 2) / 2
     * 2 .. 0   q - 1
     */
    uint8_t b0_0, q;
    if ((tag_len == 2) || (tag_len > 16) || (tag_len & 0x1) != 0) {
        /* Returning error if tag_length is 2 or greater than 16 or odd */
        return (WIFI_FW_ERR_CCM_BAD_INPUT);
    }

    /* Also implies q is within bounds */
    if (iv_len < 7 || iv_len > 13) {
        return (WIFI_FW_ERR_CCM_BAD_INPUT);
    }

    if (add_len > 0xFF00) {
        return (WIFI_FW_ERR_CCM_BAD_INPUT);
    }

    q = 16 - 1 - (unsigned char)iv_len;

    b0_0 = 0;
    b0_0 |= (add_len > 0) << 6;
    b0_0 |= ((tag_len - 2) / 2) << 3;
    b0_0 |= q - 1;
    return b0_0;
}

/**
 * @brief  Counter format from iv vector
 * @param  iv_len : Length of initilization vector
 * @param  iv     : Pointer to IV
 * @param  ctr    : Counter to update
 * @return status
 */
static void ccm_ctr_format(uint16_t iv_len, const uint8_t *iv, uint8_t *ctr)
{
    uint8_t q;
    q = CRYPTO_CTR_LEN - 1 - (unsigned char)iv_len;
    ctr[0] = q - 1;
    memscpy(ctr + 1, (CRYPTO_CTR_LEN - 1), iv, iv_len);
    memset(ctr + 1 + iv_len, 0, q);
}

/**
 * @brief  Function to derive bo data aka first block first element
 * @param  iv_len        : Length of initialization vector
 * @param  add_len       : Length of addtional data
 * @param  tag_len       : Tag aka Mac length
 * @param  payload_size  : payload length length
 * @param  iv            : pointer to iv
 * @param  b0            : pointer to b0 to update
 * @return status
 */
static uint8_t ccm_b0_format(uint16_t iv_len, uint16_t add_len, uint16_t tag_len, uint32_t payload_size,
                             const uint8_t *iv, uint8_t *b0)
{
    /*
     * First block B_0:
     * 0        .. 0        flags
     * 1        .. iv_len   nonce (aka iv)
     * iv_len+1 .. 15       length
     */
    // uint8_t b0[16] = {0};
    uint8_t q;

    memset(b0, 0, CRYPTO_B0_LEN);
    b0[0] = ccm_b0_0(iv_len, add_len, tag_len);
    if (WIFI_FW_ERR_CCM_BAD_INPUT == b0[0]) {
        return WIFI_FW_ERR_CCM_BAD_INPUT;
    }

    q = CRYPTO_B0_LEN - 1 - (unsigned char)iv_len;
    memscpy(b0 + 1, (CRYPTO_B0_LEN - 1), iv, iv_len);

    for (uint8_t i = 0; i < q; i++) {
        b0[CRYPTO_B0_LEN - 1 - i] = (unsigned char)(payload_size & 0xFF);
        payload_size >>= 8;
        if (payload_size == 0) {
            break;
        }
    }

    if (payload_size > 0) {
        return (WIFI_FW_ERR_CCM_BAD_INPUT);
    }
    return 0;
}

/**
 * @brief  Function to format addtional data
 * @param  add_len       : Length of addtional data
 * @param  add_data      : Pointer to addtional data
 * @param  b             : Block input array
 * @return Size of block input for add data in 16 bytes aligned
 */
static uint32_t ccm_add_data_format(uint16_t add_len, const uint8_t *add_data, uint8_t *b)
{
    uint8_t b_add_pad;
    /*
     * If there is additional data, update CBC-MAC with
     * add_len, add, 0 (padding to a block boundary)
     */
    if (add_len < 8160) {
        b[0] = (unsigned char)((add_len >> 8) & 0xFF);
        b[1] = (unsigned char)((add_len)&0xFF);
        memscpy(b + 2, (CRYPTO_MAX_BUFF_LENGTH - 2), add_data, add_len);
        add_len += 2;
    } else {
        b[0] = 0xFF;
        b[1] = 0xFF;
        b[2] = (unsigned char)((add_len >> 24) & 0xFF);
        b[3] = (unsigned char)((add_len >> 16) & 0xFF);
        b[4] = (unsigned char)((add_len >> 8) & 0xFF);
        b[5] = (unsigned char)((add_len)&0xFF);
        memscpy(b + 6, (CRYPTO_MAX_BUFF_LENGTH - 6), add_data, add_len);
        add_len += 6;
    }

    b_add_pad = ((add_len & 0xF) ? (0x10 - (add_len & 0xF)) : 0);
    if (b_add_pad) {
        memset(b + add_len, 0, b_add_pad);
    }

    return (add_len + b_add_pad);
}

/*------------------------------------------------------------------------
 * Global Function Definitions
 * ----------------------------------------------------------------------*/

/*
 * @brief  ISR handler for DXE channel 8 and 9
 */
void crypto_DXE_interrupt_handler(uint32_t ch)
{
    BaseType_t higher_prio_task_woken = pdFALSE;

    if (ch == 9) {
        if (nt_fail !=
            nt_osal_semaphore_give_from_isr(crypto_ctx.crypto_dxe_semaphore_handle, &higher_prio_task_woken)) {
            nt_osal_yield_from_isr(higher_prio_task_woken);
        } else {
            assert(0);
        }
    }
}

/**
 * @brief  Function to wrap to mbedtls for both ecryption and decryption
 * @param  crypto_param: pointer to the struct crypto_params_t
 * @return status      : 0 - AES CCM operation success
 *                       1 - AES CCM operation failed
 */
int wifi_aes_ccm_wrap(crypto_params_t *crypto_param)
{
    uint8_t *crypto_input_data = NULL;
    uint8_t *crypto_input_tag = NULL;
    uint8_t *crypto_output_data = NULL;
    uint8_t *crypto_output_tag = NULL;
#ifdef CRYPTO_LATENCY_PROFILE
    uint64_t crypto_exit_time = 0;
    uint64_t crypto_entry_time = hres_timer_curr_time_us();
#endif /* CRYPTO_LATENCY_PROFILE */

    assert(crypto_param->key);
    assert(crypto_param->iv);
    assert(crypto_param->a_data);
    assert(crypto_param->data);
    assert(crypto_param->output);
    assert(crypto_param->tag);
    assert(crypto_param->tag_len <= 16);
    assert((crypto_param->data_len + CRYPTO_OVERHEAD_LENGTH) < CRYPTO_MAX_BUFF_LENGTH);

#ifdef CRYPTO_DEBUG_PRINT
    NT_LOG_PRINT(SECURITY, INFO, "length  = %d", crypto_param->data_len);
    NT_LOG_PRINT(SECURITY, INFO, "iv_len  = %d", crypto_param->iv_len);
    NT_LOG_PRINT(SECURITY, INFO, "add_len = %d", crypto_param->a_data_len);
    NT_LOG_PRINT(SECURITY, INFO, "tag_len = %d", crypto_param->tag_len);
#endif /* CRYPTO_DEBUG_PRINT */

    if (crypto_ctx.crypto_in_buff == NULL) {
        /* Allocating memory of CRYPTO_MAX_BUFF_LENGTH and using throughout the code */
        crypto_ctx.crypto_in_buff = (uint8_t *)nt_osal_allocate_memory(CRYPTO_MAX_BUFF_LENGTH);
        assert(crypto_ctx.crypto_in_buff);
    }

    if (crypto_ctx.crypto_out_buff == NULL) {
        /* Allocating memory of CRYPTO_MAX_BUFF_LENGTH and using throughout the code */
        crypto_ctx.crypto_out_buff = (uint8_t *)nt_osal_allocate_memory(CRYPTO_MAX_BUFF_LENGTH);
        assert(crypto_ctx.crypto_out_buff);
    }

    if (crypto_ctx.crypto_dxe_semaphore_handle == NULL) {
        /* Initializing semaphore for crypto DXE data transfer */
        nt_osal_semaphore_create_binary(crypto_ctx.crypto_dxe_semaphore_handle);
        assert(crypto_ctx.crypto_dxe_semaphore_handle);
    }

    ccm_ctr_format((uint16_t)crypto_param->iv_len, crypto_param->iv, crypto_ctx.counter);
    if (WIFI_FW_ERR_CCM_BAD_INPUT == ccm_b0_format((uint16_t)crypto_param->iv_len, crypto_param->a_data_len,
                                                   crypto_param->tag_len, crypto_param->data_len, crypto_param->iv,
                                                   crypto_ctx.b0)) {
        NT_LOG_PRINT(SECURITY, ERR, "Error in ccm_b0_format()");
        return MBEDTLS_ERR_CCM_BAD_INPUT;
    }
    crypto_ctx.auth_size =
        ccm_add_data_format(crypto_param->a_data_len, crypto_param->a_data, crypto_ctx.crypto_in_buff);

    crypto_input_data = crypto_ctx.crypto_in_buff + crypto_ctx.auth_size;
    data_svc_memscpy(crypto_input_data, (CRYPTO_MAX_BUFF_LENGTH - crypto_ctx.auth_size), crypto_param->data,
                     crypto_param->data_len);

    if (AES_DECRYPT == crypto_param->mode) {
        crypto_input_tag = crypto_ctx.crypto_in_buff + crypto_ctx.auth_size + crypto_param->data_len;
        memscpy(crypto_input_tag, (CRYPTO_MAX_BUFF_LENGTH - (crypto_ctx.auth_size + crypto_param->data_len)),
                crypto_param->tag, crypto_param->tag_len);
    }

    if (AES_CCM_SUCCESS ==
        aes128_ccm(crypto_param->mode, crypto_param->key, &crypto_ctx, crypto_param->data_len, crypto_param->tag_len)) {
        crypto_output_data = crypto_ctx.crypto_out_buff + crypto_ctx.auth_size;
        data_svc_memscpy(crypto_param->output, crypto_param->data_len, crypto_output_data, crypto_param->data_len);
        if (AES_ENCRYPT == crypto_param->mode) {
            crypto_output_tag = crypto_ctx.crypto_out_buff + crypto_ctx.auth_size + crypto_param->data_len;
            memscpy(crypto_param->tag, crypto_param->tag_len, crypto_output_tag, crypto_param->tag_len);
        }
#ifdef CRYPTO_LATENCY_PROFILE
        crypto_exit_time = hres_timer_curr_time_us();
        if (AES_DECRYPT == crypto_param->mode) {
            NT_LOG_PRINT(SECURITY, ERR, "Decrypted %d bytes", crypto_param->data_len);
        } else {
            NT_LOG_PRINT(SECURITY, ERR, "Encrypted %d bytes", crypto_param->data_len);
        }
        NT_LOG_PRINT(SECURITY, ERR, "Crypto HW Latency: %ld us", (uint64_t)(crypto_exit_time - crypto_entry_time));
#endif /* CRYPTO_LATENCY_PROFILE */
        return 0;
    }

    return MBEDTLS_ERR_CCM_AUTH_FAILED;
}

#ifdef NT_MBEDTLS_SELF_TEST
void wifi_aes_ccm_self_test(void)
{
    mbedtls_ccm_context ctx;
    /*
     * Some hardware accelerators require the input and output buffers
     * would be in RAM, because the flash is not accessible.
     * Use buffers on the stack to hold the test vectors data.
     */
    unsigned char plaintext[CCM_SELFTEST_PT_MAX_LEN];
    unsigned char ciphertext[CCM_SELFTEST_CT_MAX_LEN];
    unsigned char randam_msg[CCM_SELFTEST_PT_MAX_LEN];
    size_t i;
    int ret;

#ifdef USE_CRYPTO_DXE_DATA_HANDLING
    nt_ndxe_init();
#endif /* USE_CRYPTO_DXE_DATA_HANDLING */

    mbedtls_ccm_init(&ctx);

    if (mbedtls_ccm_setkey(&ctx, MBEDTLS_CIPHER_ID_AES, key, 8 * sizeof key) != 0) {
        NT_LOG_PRINT(SECURITY, ERR, "  CCM: setup failed");
        return;
    }

    for (i = 0; i < NB_TESTS; i++) {
        NT_LOG_PRINT(SECURITY, ERR, "Test start\n");
        NT_LOG_PRINT(SECURITY, ERR, "CCM-AES #%d: ", (unsigned int)i + 1);
        NT_LOG_PRINT(SECURITY, ERR, "Data Length : %d", msg_len[i]);
        NT_LOG_PRINT(SECURITY, ERR, "IV   Length : %d", iv_len[i]);
        NT_LOG_PRINT(SECURITY, ERR, "Add  Length : %d", add_len[i]);
        NT_LOG_PRINT(SECURITY, ERR, "Tag  Length : %d", tag_len[i]);

        memset(plaintext, 0, CCM_SELFTEST_PT_MAX_LEN);
        memset(ciphertext, 0, CCM_SELFTEST_CT_MAX_LEN);

        if (i < 3) {
            memscpy(plaintext, CCM_SELFTEST_PT_MAX_LEN, msg, msg_len[i]);
        } else {
            for (size_t j = 0; j < msg_len[i]; j++) {
                plaintext[j] = randam_msg[j] = (char)rand();
            }
        }

        /* Encryption */
        ret = mbedtls_ccm_encrypt_and_tag(&ctx, msg_len[i], iv, iv_len[i], ad, add_len[i], plaintext, ciphertext,
                                          ciphertext + msg_len[i], tag_len[i]);

        if (ret != 0) {
            NT_LOG_PRINT(SECURITY, ERR, "Encryption failed\n");
        } else if (i < 3) {
            if (memcmp(ciphertext, res[i], msg_len[i] + tag_len[i]) != 0) {
                NT_LOG_PRINT(SECURITY, ERR, "Encrypted Cipher does not match with expected value\n");
            } else {
                NT_LOG_PRINT(SECURITY, ERR, "Encryption Passed & Cipher matched with expected value\n");
            }
        } else {
            NT_LOG_PRINT(SECURITY, ERR, "Encryption Passed\n");
        }

        /* Decryption */
        memset(plaintext, 0, CCM_SELFTEST_PT_MAX_LEN);
        ret = mbedtls_ccm_auth_decrypt(&ctx, msg_len[i], iv, iv_len[i], ad, add_len[i], ciphertext, plaintext,
                                       ciphertext + msg_len[i], tag_len[i]);

        if (ret != 0) {
            NT_LOG_PRINT(SECURITY, ERR, "Decryption failed\n");
        } else if (memcmp(plaintext, (i < 3) ? msg : randam_msg, msg_len[i]) != 0) {
            NT_LOG_PRINT(SECURITY, ERR, "Decrypted Data does not match with the expected value\n");
        } else {
            NT_LOG_PRINT(SECURITY, ERR, "Decryption Passed & Decrypted Data matched with expected value\n");
        }
    }

    mbedtls_ccm_free(&ctx);
}
#else
void wifi_aes_ccm_self_test(void)
{
    NT_LOG_PRINT(SECURITY, ERR, "HW AES CCM Self Test is Disabled");
}
#endif  // NT_MBEDTLS_SELF_TEST

#endif  // WIFI_HW_AES_CCM
