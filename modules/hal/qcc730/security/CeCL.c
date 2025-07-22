/**
@file CeCL.c
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
 $DateTime: 2017/05/08 19:09:09 $
 $Author: pwbldsvc $

when         who     what, where, why
--------     ---     -----------------------------------------------------
2015-10-29   yk     initial version
============================================================================*/
#include "CeCL.h"
#include <CeML.h>
#include "CeCL_Target.h"
#include "CeEL.h"
#include "CeEL_Env.h"
#include "CeEL_Dm.h"
#include "nt_common.h"

#define CECL_HW_SEC_ACC_DIN_SIZE 16
//#define CECL_HW_SEC_CE_STATUS_DIN_RDY (HW_REG_RD(CECL_CE_STATUS) & (1<<CECL_CE_STATUS_DIN_RDY_SHFT))
//#define CECL_HW_SEC_CE_STATUS_DOUT_RDY (HW_REG_RD(CECL_CE_STATUS) & (1<<CECL_CE_STATUS_DOUT_RDY_SHFT))
#define IS_ALIGNED(v) (((uint32)(v)&0x03) == 0)

/* Enable Workaround for HW bug QCTDD03825393 */
#define ENCR_CNTR_IV_SW_WA

/**
 * @brief
 *
 *
 * @return None
 *
 * @see
 *
 */
CeCLErrorType CeClInit(CeCLXferModeType mode)
{
    /* Setup Crypto CONFIG registers */
    CeElSetupConfig();

    return CECL_ERROR_SUCCESS;
}

/**
 * @brief
 *
 *
 * @return None
 *
 * @see
 *
 */
CeCLErrorType CeClReset()
{
    return CECL_ERROR_SUCCESS;
}

/**
 * @brief
 *
 *
 * @return None
 *
 * @see
 *
 */
CeCLErrorType CeClDeinit(void)
{
    return CECL_ERROR_SUCCESS;
}

/**
 * @brief
 *
 *
 * @return None
 *
 * @see
 *
 */

void CeCLIOCtlCompletion(void)
{
    volatile uint32 ce_status = 0;

    while (1) {
        ce_status = HW_REG_RD(CECL_CE_STATUS);
        CeElMemoryBarrier();
        if (ce_status & CECL_CE_STATUS_OPERATION_DONE_BMSK) {
            break;
        }
    }
}

/**
 * @brief
 *
 *
 * @return None
 *
 * @see
 *
 */
CeCLErrorType CeClIOCtlHash(CeCLIoCtlHashType ioCtlVal, uint8 *pBufIn, uint32 dwLenIn, uint8 *pBufOut, uint32 dwLenOut,
                            uint32 *pdwActualOut)
{
    CeCLErrorType retVal = CECL_ERROR_SUCCESS;

    switch (ioCtlVal) {
        case CECL_IOCTL_SET_HASH_CNTXT:
            if ((pBufIn != NULL) && (dwLenIn == sizeof(CeCLHashAlgoCntxType))) {
                retVal = CeCLIOCtlSetHashCntx((CeCLHashAlgoCntxType *)pBufIn);
            } else {
                retVal = CECL_ERROR_FAILURE;
            }
            break;

        case CECL_IOCTL_GET_HASH_CNTXT:

            if ((pBufOut != NULL) && (dwLenOut >= sizeof(CeCLHashAlgoCntxType)) && (pdwActualOut != NULL)) {
                retVal = CeCLIOCtlGetHashCntx((CeCLHashAlgoCntxType *)pBufOut);
                *pdwActualOut = sizeof(CeCLHashAlgoCntxType);
            } else {
                retVal = CECL_ERROR_FAILURE;
            }
            break;

        case CECL_IOCTL_HASH_XFER:

            if ((pBufIn != NULL) && (dwLenIn >= sizeof(CeCLHashXferType)) && (pdwActualOut != NULL)) {
                retVal = CeCLIOCtlHashRegXfer((CeCLHashXferType *)pBufIn);
                *pdwActualOut = sizeof(CeCLHashXferType);
            } else {
                retVal = CECL_ERROR_FAILURE;
            }
            break;

        default:
            retVal = CECL_ERROR_FAILURE;
            break;
    }

    return retVal;
}

/**
 * @brief
 *
 *
 * @return None
 *
 * @see
 *
 */
CeCLErrorType CeClIOCtlCipher(CeCLIoCtlCipherType ioCtlVal, uint8 *pBufIn, uint32 dwLenIn, uint8 *pBufOut,
                              uint32 dwLenOut, uint32 *pdwActualOut)
{
    CeCLErrorType retVal = CECL_ERROR_SUCCESS;

    switch (ioCtlVal) {
        case CECL_IOCTL_SET_CIPHER_CNTXT:
            if ((pBufIn != NULL) && (dwLenIn == sizeof(CeCLCipherCntxType)) && (pdwActualOut != NULL)) {
                retVal = CeCLIOCtlSetCipherCntx((CeCLCipherCntxType *)pBufIn);
            } else {
                retVal = CECL_ERROR_FAILURE;
            }
            break;

        case CECL_IOCTL_GET_CIPHER_CNTXT:
            if ((pBufOut != NULL) && (dwLenOut >= sizeof(CeCLCipherCntxType)) && (pdwActualOut != NULL)) {
                retVal = CeCLIOCtlGetCipherCntx((CeCLCipherCntxType *)pBufOut);
                *pdwActualOut = sizeof(CeCLCipherCntxType);
            } else {
                retVal = CECL_ERROR_FAILURE;
            }
            break;

        case CECL_IOCTL_CIPHER_XFER:
            if ((pBufIn != NULL) && (dwLenIn >= sizeof(CeCLCipherXferType)) && (pdwActualOut != NULL)) {
                retVal = CeCLIOCtlCipherRegXfer((CeCLCipherXferType *)pBufIn);
                *pdwActualOut = sizeof(CeCLCipherXferType);
            } else {
                retVal = CECL_ERROR_FAILURE;
            }
            break;

        default:
            retVal = CECL_ERROR_FAILURE;
            break;
    }

    return retVal;
}

/**
 * @brief This function sets various SHAx registers
 *
 * @param
 *
 * @return
 *
 * @see
 *
 */
CeCLErrorType CeCLIOCtlSetHashCntx(CeCLHashAlgoCntxType *ctx_ptr)
{
    uint32 seg_cfg_val = 0;
    uint32 temp_len = 0;
    CeCLErrorType retVal = CECL_ERROR_SUCCESS;

    /* Sanity check inputs */
    if (!ctx_ptr) {
        return CECL_ERROR_INVALID_PARAM;
    }

    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_CORE_CFG_REG, 0x2);
    // Clear authentication seg config registers
    HAL_REG_WR(CECL_CE_AUTH_SEG_CFG, 0);
    HAL_REG_WR(CECL_CE_ENCR_SEG_CFG, 0);

    // AUTH_SEG_CFG
    seg_cfg_val = CECL_CE_AUTH_SEG_CFG_AUTH_ALG_SHA << CECL_CE_AUTH_SEG_CFG_AUTH_ALG_SHFT;
    if (CECL_HASH_ALGO_SHA1 == ctx_ptr->algo) {
        seg_cfg_val |= (CECL_CE_AUTH_SEG_CFG_AUTH_SIZE_SHA1 << CECL_CE_AUTH_SEG_CFG_AUTH_SIZE_SHFT);
    } else if (CECL_HASH_ALGO_SHA256 == ctx_ptr->algo) {
        seg_cfg_val |= (CECL_CE_AUTH_SEG_CFG_AUTH_SIZE_SHA256 << CECL_CE_AUTH_SEG_CFG_AUTH_SIZE_SHFT);
    }
    if (ctx_ptr->lastBlock) {
        seg_cfg_val |= (1 << QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_SEG_CFG_LAST_OFFSET);
    }
    if (ctx_ptr->firstBlock) {
        seg_cfg_val |= (1 << QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_SEG_CFG_FIRST_OFFSET);
    }
    HAL_REG_WR(CECL_CE_AUTH_SEG_CFG, seg_cfg_val);

    /* Write the CRYPTO_CE_AUTH_SEG_SIZE register. AUTH_SIZE should be set
     * to buff_size */
    HAL_REG_WR(CECL_CE_AUTH_SEG_SIZE, ctx_ptr->dataLn);
    HAL_REG_WR(CECL_CE_ENCR_SEG_SIZE, 0);

    /* Write the CRYPTO_CE_SEG_SIZE register: Multiple of 16
       For 1-pass SHA1, this value should be same as the total length */
#if 0  // only for PIO mode
  if (ctx_ptr->dataLn % 16)
  {
    /* Not a multiple of 16/64 bytes */
    temp_len = ctx_ptr->dataLn + (16 - (ctx_ptr->dataLn % 16));  
    HAL_REG_WR(CECL_CE_SEG_SIZE, temp_len);
  }
  else 
  {
    HAL_REG_WR(CECL_CE_SEG_SIZE, ctx_ptr->dataLn);
  }
#endif
    HAL_REG_WR(CECL_CE_SEG_SIZE, ctx_ptr->dataLn);

    /* Set the SEG START to 0 */
    HAL_REG_WR(CECL_CE_AUTH_SEG_START, 0);

    {
        /* Write the AUTH_BYTECNT[0,1] registers, only 2 are valid at this time   */

        HAL_REG_WR(CECL_CE_AUTH_BYTECNT0, (ctx_ptr->auth_bytecnt[0]));
        HAL_REG_WR(CECL_CE_AUTH_BYTECNT1, (ctx_ptr->auth_bytecnt[1]));
    }

    /* Write Initialization Vector */
    {
        HAL_REG_WR(CECL_CE_AUTH_IV0, *(ctx_ptr->auth_iv + 0));
        HAL_REG_WR(CECL_CE_AUTH_IV1, *(ctx_ptr->auth_iv + 1));
        HAL_REG_WR(CECL_CE_AUTH_IV2, *(ctx_ptr->auth_iv + 2));
        HAL_REG_WR(CECL_CE_AUTH_IV3, *(ctx_ptr->auth_iv + 3));
        HAL_REG_WR(CECL_CE_AUTH_IV4, *(ctx_ptr->auth_iv + 4));
        HAL_REG_WR(CECL_CE_AUTH_IV5, *(ctx_ptr->auth_iv + 5));
        HAL_REG_WR(CECL_CE_AUTH_IV6, *(ctx_ptr->auth_iv + 6));
        HAL_REG_WR(CECL_CE_AUTH_IV7, *(ctx_ptr->auth_iv + 7));
    }

    // if(!CEEL_DM_IS_SUPPORTED())
    {
        /* kick-off the crypto operation, the GOPROC is to be set once all the
         * config registers are set. It has nothing to do with data */
        HAL_REG_WR(CECL_CE_GOPROC, CECL_CE_GOPROC_GO_BMSK);
    }

#if 0
  while(1)
  {
    uint32 ce_state = HW_REG_RDF(CECL_CE_STATUS, CRYPTO_STATE);

    //We can change this to detect whether the PROCESSING bit is turned on. Currently
    //we do what the original drivers are doing i.e look for any bit set to indicate it's
    //not in idle state
    if(ce_state)
    {
      break;
    }
  }
#endif

    return retVal;
}

/**
 * @brief Get the SHA context from the Crypto HW registers
 *
 * @param
 *
 * @return
 *
 * @see
 *
 */
CeCLErrorType CeCLIOCtlGetHashCntx(CeCLHashAlgoCntxType *ctx_ptr)
{
    CeCLErrorType retVal = CECL_ERROR_SUCCESS;

    /* Sanity check inputs */
    if ((!ctx_ptr) || (((CECL_HASH_ALGO_SHA1 != ctx_ptr->algo)) && ((CECL_HASH_ALGO_SHA256 != ctx_ptr->algo)) &&
                       ((CECL_HASH_ALGO_CMAC128 != ctx_ptr->algo)) && ((CECL_HASH_ALGO_CMAC256 != ctx_ptr->algo))))

    {
        return CECL_ERROR_INVALID_PARAM;
    }

    // CeCLIOCtlCompletion();

    ctx_ptr->auth_bytecnt[0] = HW_REG_RD(CECL_CE_AUTH_BYTECNT0);
    ctx_ptr->auth_bytecnt[1] = HW_REG_RD(CECL_CE_AUTH_BYTECNT1);

#if 0  // only for PIO mode
   ctx_ptr->auth_iv[0] = HW_REG_RD(CECL_CE_AUTH_IV0);
   ctx_ptr->auth_iv[1] = HW_REG_RD(CECL_CE_AUTH_IV1);
   ctx_ptr->auth_iv[2] = HW_REG_RD(CECL_CE_AUTH_IV2);
   ctx_ptr->auth_iv[3] = HW_REG_RD(CECL_CE_AUTH_IV3);
   ctx_ptr->auth_iv[4] = HW_REG_RD(CECL_CE_AUTH_IV4);
   ctx_ptr->auth_iv[5] = HW_REG_RD(CECL_CE_AUTH_IV5);
   ctx_ptr->auth_iv[6] = HW_REG_RD(CECL_CE_AUTH_IV6);
   ctx_ptr->auth_iv[7] = HW_REG_RD(CECL_CE_AUTH_IV7);
#endif

#if 0
   int i;
   for(i=0; i<8; i++)
   {
        printf("auth_iv[%d]:0x%0x\n",i, ctx_ptr->auth_iv[i]);
   }
#endif

    return retVal;
}

/**
 * @brief  This function prepares the command buffer and then
 *
 * @param ctx_ptr        [in]  Pointer to current context
 * @param buff_ptr       [in]  Pointer to input buffer
 * @param bytes_to_write [in]  Size of the input buffet
 * @param digest_ptr     [out] Pointer to output buffer
 * @param auth_alg       [in]  Algorithm typr
 *
 * @return CE_Result_Type
 *
 * @see
 *
 */
CeCLErrorType CeCLIOCtlHashRegXfer(CeCLHashXferType *pBufOut)
{
    return CECL_ERROR_NOT_SUPPORTED;
}

/**
 * @brief This function sets various SHAx registers
 *
 * @param
 *
 * @return
 *
 * @see
 *
 */
CeCLErrorType CeCLIOCtlSetCipherCntx(CeCLCipherCntxType *ctx_ptr)
{
    CeCLErrorType retVal = CECL_ERROR_SUCCESS;
    uint32 seg_cfg_val = 0;
    uint32 auth_seg_cfg_val = 0;
    //  uint32 ce_block_size = CECL_BLOCK_SIZE_NON_BAM;
    uint32 encr_seg_size = 0;
    uint32 encr_seg_start = 0;
    uint32 auth_seg_size = 0;

    /* Sanity check inputs */
    if (!ctx_ptr) {
        return CECL_ERROR_INVALID_PARAM;
    }

    // Clear encryption and authentication seg config registers
    HAL_REG_WR(CECL_CE_ENCR_SEG_CFG, 0);
    HAL_REG_WR(CECL_CE_AUTH_SEG_CFG, 0);
    HAL_REG_WR(CECL_CE_ENCR_SEG_SIZE, 0);
    HAL_REG_WR(CECL_CE_ENCR_SEG_SIZE, 0);
    HAL_REG_WR(CECL_CE_AUTH_SEG_SIZE, 0);

    if (ctx_ptr->dir != CECL_CIPHER_BYPASS) {
        seg_cfg_val = ctx_ptr->mode << CECL_CE_ENCR_SEG_CFG_ENCR_MODE_SHFT;

        /* Set algo to AES */
        seg_cfg_val |= CECL_CE_CIPHER_AES_ALGO_VAL << CECL_CE_ENCR_SEG_CFG_ENCR_ALG_SHFT;

        /* this bit should be set for encryption and clear otherwise */
        if (ctx_ptr->dir == CECL_CIPHER_ENCRYPT) {
            seg_cfg_val |= (1 << CECL_CE_ENCR_SEG_CFG_ENCODE_SHFT);
        }

        /* For counter mode operation we need to run the engine in encrypt mode
         the XOR “undoes" the encrypted data into decrypted data.*/
        if (ctx_ptr->mode == CECL_CIPHER_MODE_CTR) {
            seg_cfg_val |= (1 << CECL_CE_ENCR_SEG_CFG_ENCODE_SHFT);
        }

        /* Set AES key size */
        if (ctx_ptr->algo == CECL_CIPHER_ALG_AES128) {
            seg_cfg_val |= (CECL_CE_CIPHER_KEY_SIZE_AES128 << CECL_CE_ENCR_SEG_CFG_ENCR_KEY_SZ_SHFT);
        } else if (ctx_ptr->algo == CECL_CIPHER_ALG_AES256) {
            seg_cfg_val |= (CECL_CE_CIPHER_KEY_SIZE_AES256 << CECL_CE_ENCR_SEG_CFG_ENCR_KEY_SZ_SHFT);
        }

        /* encryption/decryption segment configuration */
        HAL_REG_WR(CECL_CE_ENCR_SEG_SIZE, ctx_ptr->dataLn);
        HAL_REG_WR(CECL_CE_ENCR_SEG_START, 0);

        // Set CE seg size
        //  if(ctx_ptr->mode == CECL_CIPHER_MODE_CBC)
        //  {
        // Set CE seg size if CBC Register based mode
        HAL_REG_WR(CECL_CE_SEG_SIZE, ctx_ptr->dataLn);
//  }
#if 0
    //Write the CRYPTO_CE_SEG_SIZE register: Multiple of 16 or 64 bytes depending on BAM or no BAM 
    else if ((ctx_ptr->dataLn) % ce_block_size)
    {
      /* Not a multiple of 16/64 bytes */
      temp_len = ctx_ptr->dataLn + (ce_block_size - (ctx_ptr->dataLn % ce_block_size));  
      HAL_REG_WR(CECL_CE_SEG_SIZE, temp_len);
    }
    else 
    {
      /* Multiple of 16/64 bytes */
      HAL_REG_WR(CECL_CE_SEG_SIZE, ctx_ptr->dataLn);
    }
#endif
        HAL_REG_WR(CECL_CE_ENCR_CNTR0_IV0, ctx_ptr->iv[0]);
        HAL_REG_WR(CECL_CE_ENCR_CNTR1_IV1, ctx_ptr->iv[1]);
        HAL_REG_WR(CECL_CE_ENCR_CNTR2_IV2, ctx_ptr->iv[2]);
        HAL_REG_WR(CECL_CE_ENCR_CNTR3_IV3, ctx_ptr->iv[3]);

        HAL_REG_WR(CECL_CE_ENCR_CNTR_MASK3, 0xFFFFFFFF);
        HAL_REG_WR(CECL_CE_ENCR_CNTR_MASK0, 0xFFFFFFFF);
        HAL_REG_WR(CECL_CE_ENCR_CNTR_MASK1, 0xFFFFFFFF);
        HAL_REG_WR(CECL_CE_ENCR_CNTR_MASK2, 0xFFFFFFFF);

        if (!ctx_ptr->bAESUseHWKey) {
            HAL_REG_WR(CECL_CE_SW_KEY0_0, ctx_ptr->aes_key[0]);
            HAL_REG_WR(CECL_CE_SW_KEY0_1, ctx_ptr->aes_key[1]);
            HAL_REG_WR(CECL_CE_SW_KEY0_2, ctx_ptr->aes_key[2]);
            HAL_REG_WR(CECL_CE_SW_KEY0_3, ctx_ptr->aes_key[3]);
            HAL_REG_WR(CECL_CE_SW_KEY0_4, ctx_ptr->aes_key[4]);
            HAL_REG_WR(CECL_CE_SW_KEY0_5, ctx_ptr->aes_key[5]);
            HAL_REG_WR(CECL_CE_SW_KEY0_6, ctx_ptr->aes_key[6]);
            HAL_REG_WR(CECL_CE_SW_KEY0_7, ctx_ptr->aes_key[7]);
        }
        // If CCM mode
        if (ctx_ptr->mode == CECL_CIPHER_MODE_CCM) {
            if (ctx_ptr->dir == CECL_CIPHER_ENCRYPT) {
                if (ctx_ptr->payloadLn >= ctx_ptr->dataLn) {
                    encr_seg_start = 0;
                    encr_seg_size = ctx_ptr->dataLn;
                } else {
                    encr_seg_start = ctx_ptr->dataLn - ctx_ptr->payloadLn;
                    encr_seg_size = ctx_ptr->payloadLn;
                }

                auth_seg_size = ctx_ptr->dataLn;
            } else {
                if (ctx_ptr->payloadLn >= ctx_ptr->dataLn) {
                    encr_seg_start = 0;
                    encr_seg_size = ctx_ptr->dataLn;
                    auth_seg_size = ctx_ptr->dataLn;
                } else {
                    encr_seg_size = ctx_ptr->payloadLn + ctx_ptr->macLn;
                    auth_seg_size = ctx_ptr->dataLn - ctx_ptr->macLn;
                    encr_seg_start = ctx_ptr->dataLn - ctx_ptr->payloadLn - ctx_ptr->macLn;
                }
            }

            HAL_REG_WR(CECL_CE_SEG_SIZE, ctx_ptr->dataLn);

            // Set Encr seg start & size
            HAL_REG_WR(CECL_CE_ENCR_SEG_START, encr_seg_start);

            HAL_REG_WR(CECL_CE_ENCR_SEG_SIZE, encr_seg_size);

            // Set Auth seg start & size
            HAL_REG_WR(CECL_CE_AUTH_SEG_START, 0);

            HAL_REG_WR(CECL_CE_AUTH_SEG_SIZE, auth_seg_size);

            // Clear AUTH BYTECNT regs
            HAL_REG_WR(CECL_CE_AUTH_BYTECNT0, 0);

            HAL_REG_WR(CECL_CE_AUTH_BYTECNT1, 0);

            // Check for first block
            if (ctx_ptr->firstBlock) {
                HAL_REG_WR(CECL_CE_ENCR_CCM_INIT_CNTR0, ctx_ptr->iv[0]);
                HAL_REG_WR(CECL_CE_ENCR_CCM_INIT_CNTR1, ctx_ptr->iv[1]);
                HAL_REG_WR(CECL_CE_ENCR_CCM_INIT_CNTR2, ctx_ptr->iv[2]);
                HAL_REG_WR(CECL_CE_ENCR_CCM_INIT_CNTR3, ctx_ptr->iv[3]);

                //+1 for CNTR IV for CCM mode
                ctx_ptr->iv[3] += 0x01000000;
                HAL_REG_WR(CECL_CE_ENCR_CNTR3_IV3, ctx_ptr->iv[3]);
                if (!(ctx_ptr->iv[3] & 0xffffffff)) {
                    ctx_ptr->iv[2] += 0x01000000;
                    HAL_REG_WR(CECL_CE_ENCR_CNTR2_IV2, ctx_ptr->iv[2]);

                    if (!(ctx_ptr->iv[2] & 0xffffffff)) {
                        ctx_ptr->iv[1] += 0x01000000;
                        HAL_REG_WR(CECL_CE_ENCR_CNTR1_IV1, ctx_ptr->iv[1]);
                        if (!(ctx_ptr->iv[1] & 0xffffffff)) {
                            ctx_ptr->iv[0] += 0x01000000;
                            HAL_REG_WR(CECL_CE_ENCR_CNTR0_IV0, ctx_ptr->iv[0]);
                        }
                    }
                }

                /* Clear the AES Auth Initialization Vector */
                HAL_REG_WR(CECL_CE_AUTH_IV0, 0);
                HAL_REG_WR(CECL_CE_AUTH_IV1, 0);
                HAL_REG_WR(CECL_CE_AUTH_IV2, 0);
                HAL_REG_WR(CECL_CE_AUTH_IV3, 0);
                HAL_REG_WR(CECL_CE_AUTH_IV4, 0);
                HAL_REG_WR(CECL_CE_AUTH_IV5, 0);
                HAL_REG_WR(CECL_CE_AUTH_IV6, 0);
                HAL_REG_WR(CECL_CE_AUTH_IV7, 0);

                // Set AUTH first block bit
                auth_seg_cfg_val |= (1 << QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_SEG_CFG_FIRST_OFFSET);
            }

            // Check for last block
            if (ctx_ptr->lastBlock) {
                // Set ENCR last block bit
                seg_cfg_val |= (1 << QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_ENCR_SEG_CFG_LAST_OFFSET);

                // Set AUTH last block bit
                auth_seg_cfg_val |= (1 << QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_AUTH_SEG_CFG_LAST_OFFSET);
            }

#if 0	
      //Write AUTH key to CE registers if not using HW key
      if(!ctx_ptr->bAESUseHWKey)
      {
        HAL_REG_WR(CECL_CE_SW_KEY0_0, *(ctx_ptr->aes_key + 0));
        HAL_REG_WR(CECL_CE_SW_KEY0_1, *(ctx_ptr->aes_key + 1));
        HAL_REG_WR(CECL_CE_SW_KEY0_2, *(ctx_ptr->aes_key + 2));
        HAL_REG_WR(CECL_CE_SW_KEY0_3, *(ctx_ptr->aes_key + 3));
        HAL_REG_WR(CECL_CE_SW_KEY0_4, *(ctx_ptr->aes_key + 4));
        HAL_REG_WR(CECL_CE_SW_KEY0_5, *(ctx_ptr->aes_key + 5));
        HAL_REG_WR(CECL_CE_SW_KEY0_6, *(ctx_ptr->aes_key + 6));
        HAL_REG_WR(CECL_CE_SW_KEY0_7, *(ctx_ptr->aes_key + 7));
        CeElMemoryBarrier();
      }
#endif

            /* Write out Auth Info Nonce data */

            HAL_REG_WR(CECL_CE_AUTH_INFO_NONCE0, ctx_ptr->nonce[0]);
            HAL_REG_WR(CECL_CE_AUTH_INFO_NONCE1, ctx_ptr->nonce[1]);
            HAL_REG_WR(CECL_CE_AUTH_INFO_NONCE2, ctx_ptr->nonce[2]);
            HAL_REG_WR(CECL_CE_AUTH_INFO_NONCE3, ctx_ptr->nonce[3]);
            /* Set up Auth Key Size values to write to register */
            if (ctx_ptr->algo == CECL_CIPHER_ALG_AES128) {  // set AES128 key size
                auth_seg_cfg_val |= CECL_CE_AUTH_SEG_CFG_AUTH_KEY_SZ_128 << CECL_CE_AUTH_SEG_CFG_AUTH_KEY_SZ_SHFT;
            } else {  // set AES256 key size
                auth_seg_cfg_val |= CECL_CE_AUTH_SEG_CFG_AUTH_KEY_SZ_256 << CECL_CE_AUTH_SEG_CFG_AUTH_KEY_SZ_SHFT;
            }

            // Set Auth sequence and cal Mac Length
            if (ctx_ptr->dir == CECL_CIPHER_ENCRYPT) {
                auth_seg_cfg_val |= CECL_CE_AUTH_SEG_CFG_AUTH_POS_BEFORE << CECL_CE_AUTH_SEG_CFG_AUTH_POS_SHFT;
            } else {
                auth_seg_cfg_val |= CECL_CE_AUTH_SEG_CFG_AUTH_POS_AFTER << CECL_CE_AUTH_SEG_CFG_AUTH_POS_SHFT;
            }

            /* Set Algorithm, Mode, Auth size and Nonce Size values */
            auth_seg_cfg_val |= CECL_CE_AUTH_SEG_CFG_AUTH_ALG_AES << CECL_CE_AUTH_SEG_CFG_AUTH_ALG_SHFT |
                                CECL_CE_AUTH_SEG_CFG_AUTH_MODE_CCM << CECL_CE_AUTH_SEG_CFG_AUTH_MODE_SHFT |
                                (ctx_ptr->macLn - 1) << CECL_CE_AUTH_SEG_CFG_AUTH_SIZE_SHFT;

        }   /* end CCM if */
    } else  // bypass mode
    {
        /* Set algo to AES */
        seg_cfg_val |= CECL_CE_CIPHER_AES_ALGO_VAL << CECL_CE_ENCR_SEG_CFG_ENCR_ALG_SHFT;

        HAL_REG_WR(CECL_CE_ENCR_SEG_SIZE, 0);
        HAL_REG_WR(CECL_CE_ENCR_SEG_START, ctx_ptr->dataLn);

        HAL_REG_WR(CECL_CE_SEG_SIZE, ctx_ptr->dataLn);

        // Set Auth seg start & size
        HAL_REG_WR(CECL_CE_AUTH_SEG_SIZE, ctx_ptr->dataLn);
        HAL_REG_WR(CECL_CE_AUTH_SEG_START, 0);
    }

    /* Write out Auth and Encr seg cfg values */
    HAL_REG_WR(CECL_CE_AUTH_SEG_CFG, auth_seg_cfg_val);

    HAL_REG_WR(CECL_CE_ENCR_SEG_CFG, seg_cfg_val);

    HAL_REG_WR(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_CORE_CFG_REG, 0x2);

    // if(!CEEL_DM_IS_SUPPORTED())
    {
        /* kick-off the crypto operation, the GOPROC is to be set once all the
         * config registers are set. It has nothing to do with data */
        HAL_REG_WR(CECL_CE_GOPROC, CECL_CE_GOPROC_GO_BMSK);
    }

#if 0   
  while(1)
  {
    uint32 ce_state = HW_REG_RDF(CECL_CE_STATUS, CRYPTO_STATE);
    //We can change this to detect whether the PROCESSING bit is turned on. Currently
    //we do what the original drivers are doing i.e look for any bit set to indicate it's
    //not in idle state
    if(ce_state)
    {
      break;
    }
  }
#endif
    return retVal;
}

/**
 * @brief Get the AES context from the HW registers
 *
 * @param ctx_ptr    [in] Pointer to current context
 * @param auth_alg   [in] Algorithm type
 *
 * @return CE_Result_Type
 *
 * @see
 *
 */
CeCLErrorType CeCLIOCtlGetCipherCntx(CeCLCipherCntxType *ctx_ptr)
{
    CeCLErrorType retVal = CECL_ERROR_SUCCESS;

    /* Sanity check inputs */
    if (!ctx_ptr) {
        return CECL_ERROR_INVALID_PARAM;
    }
#if defined(ENCR_CNTR_IV_SW_WA)
    ctx_ptr->iv[0] = HW_REG_RD(CECL_CE_ENCR_CNTR3_IV3);
    ctx_ptr->iv[1] = HW_REG_RD(CECL_CE_ENCR_CNTR2_IV2);
    ctx_ptr->iv[2] = HW_REG_RD(CECL_CE_ENCR_CNTR1_IV1);
    ctx_ptr->iv[3] = HW_REG_RD(CECL_CE_ENCR_CNTR0_IV0);
#else
    ctx_ptr->iv[0] = HW_REG_RD(CECL_CE_ENCR_CNTR0_IV0);
    ctx_ptr->iv[1] = HW_REG_RD(CECL_CE_ENCR_CNTR1_IV1);
    ctx_ptr->iv[2] = HW_REG_RD(CECL_CE_ENCR_CNTR2_IV2);
    ctx_ptr->iv[3] = HW_REG_RD(CECL_CE_ENCR_CNTR3_IV3);
#endif
    CeElMemoryBarrier();

    return retVal;
}

/**
 * @brief  This function feeds the data to the crypto engine
 *         reads the result back
 *
 *
 * @param pBufOut        [in]  buffer of type CeCLCipherXferType
 *
 * @return CeCLErrorType
 *
 * @see
 *
 */
CeCLErrorType CeCLIOCtlCipherRegXfer(CeCLCipherXferType *pBufOut)
{
    return CECL_ERROR_NOT_SUPPORTED;
}
