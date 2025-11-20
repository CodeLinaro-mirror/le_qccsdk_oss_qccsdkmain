/**
@file CeMl.c
@brief Crypto Engine Module source file
*/

/**********************************************************************
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 **********************************************************************/
/*======================================================================

                        EDIT HISTORY FOR MODULE



when         who     what, where, why
--------     ---     ---------------------------------------------------
06/29/16     yash    CMAC init/update/final/deinit feature
10/28/15     yk      initial CeMlHashUpdate
=======================================================================*/

#include "CeCL.h"
#include "CeML.h"
#include "CeEL.h"
#include "HALhwio.h"
#include "CeCL_Target.h"
#include "fermion_hw_reg.h"
#include "Fermion_seq_hwioreg.h"
#include <stdlib.h>
#include "nt_common.h"
#include <unistd.h>
#include "nt_logger_api.h"

/*===========================================================================

            LOCAL DEFINITIONS AND DECLARATIONS FOR MODULE

===========================================================================*/
// Identifier to blacklist only PBL cryptodriver
#define CEML_PBL_DRV
#define CEML_SHA_BLOCK_SIZE 64
#define CEML_MAX_BLOCK_SIZE 0xFFFFFFFC  // Max data size that crypto engine can handle
#define CCM_B0_FLAGS_ADATA  0x40
#define ctr_len(x)          (((*(uint8 *)(x)) & 0x07) + 1)

/*
 * Convert a number into a string of bytes
 *
 * Init _p to point to one element past the end of
 * the buffer so that the buffer can be indexed in "reverse order".
 * This simplifies the conversion process: shifting of the data value
 * into the array elements.
 */
#define set_ctr(x, v)                                   \
    {                                                   \
        uint8 *_p = (uint8 *)(x) + CEML_AES128_IV_SIZE; \
        uint64 _t = (v), _l = ctr_len(x);               \
        do {                                            \
            *--_p = (uint8)_t;                          \
            _t >>= 8;                                   \
        } while (--_l);                                 \
    }

typedef enum { CEML_PARALLEL_HASH_DONE = 0x0, CEML_PARALLEL_HASH_NOT_DONE = 0x1 } CeMLParallelHashStatus;

typedef struct {
    uint32 input_ptr;
    CeMLParallelHashStatus status;
} CeMLHashNonBlockingInput;

// Array to hold pointers for non-blocking hash api
#define NON_BLOCKING_POINTER_LIST_SIZE 2
typedef struct _hpointer_list {
    // uint32 input[NON_BLOCKING_POINTER_LIST_SIZE];
    CeMLHashNonBlockingInput input[NON_BLOCKING_POINTER_LIST_SIZE];
    uint32 input_size[NON_BLOCKING_POINTER_LIST_SIZE];
    uint32 pointer_location;
    uint32 counter;
    uint32 last_sec_copied;
    uint8 *currentPtr;
    // uint32 pending_dm_request;
    // uint32 processing_ptr;
} pointer_list;

typedef struct {
    CeCLHashAlgoCntxType ctx;
    pointer_list non_blocking_ptr;
    uint8 saved_buff[CEML_SHA_BLOCK_SIZE];
    uint32 saved_buff_index;
} CeMLHashAlgoCntxType;

typedef struct {
    CeCLCipherCntxType ctx;
} CeMLCipherAlgoCntxType;

/*
Below functions are internal function to achieve AEC-CCM* thru AEC-CBC-MAC, AES-ECB, and AES-CTR.
*/
static CeMLErrorType CeMLCCMStarAuth(CeMLCntxHandle *ceMlHandle, CeMLIovecListType *ioVecOut, uint8 *iv);

static CeMLErrorType CeMLCCMStarIV(CeMLCntxHandle *ceMlHandle, uint8 *iv);

static CeMLErrorType CeMLCCMStarCipherData(CeMLCntxHandle *ceMlHandle, CeMLIovecListType ioVecIn,
                                           CeMLIovecListType *ioVecOut);

/**
 * @brief The Crypto HW expects that the CCM blocks [B0, B1, ..., Bn, ... Bm]
 * be present.  This function builds block B0 and fills in the length of
 * authenticated in B1 if non-zero according to NIST SP800-38C.
 *
 * @param[in] ceMlHandle pointer to context Cryto Engine context
 *
 * @param[in] hdr pointer to header byte array used to build
 *
 * @return CEML_ERROR_SUCCESS if successful,
 *         CEML_ERROR_SUCCESS otherwise
 *
 * @dependenies - hdr buffer is large enough to hold the Authenticated data
 *                length encoded as per the NIST standard.
 *              - nonce buffer in context is large enough to hold an AES block
 *
 * @sideeffects B0 block is copied to nonce member of the context
 *              hdr is updated to contain encoded AAD length
 */

static CeMLErrorType CCMAuthHeaderInit(CeMLCntxHandle *ceMlHandle, uint8 *hdr)
{
    CeMLErrorType ret = CEML_ERROR_SUCCESS;
    CeCLCipherCntxType *ctx_ptr = NULL;
    uint32 hdr_len = 0;
    uint32 valueM = 0;
    uint8 b0[CEML_AES_BLOCK_SIZE];

    /* Sanity check inputs */
    if ((!ceMlHandle) || (!ceMlHandle->pClientCtxt) || !hdr) {
        return CEML_ERROR_INVALID_PARAM;
    }

    // Create context pointer
    ctx_ptr = (CeCLCipherCntxType *)&(((CeMLCipherAlgoCntxType *)((ceMlHandle)->pClientCtxt))->ctx);

    // MAC and nonce size restriction per standard
    // mac size can be between 4-16 but for better security, it should
    // be between 8 - 16
    if ((ctx_ptr->macLn > 0 && ctx_ptr->macLn < 4) || ctx_ptr->macLn > 16)
        return CEML_ERROR_INVALID_PARAM;
    else if (ctx_ptr->nonceLn < 7 || ctx_ptr->nonceLn > 13)
        return CEML_ERROR_INVALID_PARAM;

    /*
    ** b0 is function of nonce, message length,
    ** adata (header existence indication) and authentication tag length
    */
    CeElMemset((uint8 *)(b0), 0, sizeof(b0));

    /*
    ** write q-1 value to b[0].
    ** q is the nbr of bytes in the payload
    ** (q is the octet length of
    ** the binary representation of the octet length of the payload.)
    ** nonce length + q should always be 15
    */
    b0[0] = (uint8)(CEML_AES_BLOCK_SIZE - 2 - ctx_ptr->nonceLn);

    // store nonce
    if (sizeof(b0) - 1 < ctx_ptr->nonceLn)
        return CEML_ERROR_INVALID_PARAM;
    CeElMemScpy((uint8 *)(&(b0[1])), ctx_ptr->nonceLn, (uint8 *)(ctx_ptr->nonce), ctx_ptr->nonceLn);

    /* store the message length */
    set_ctr(b0, ctx_ptr->payloadLn);

    hdr_len = ctx_ptr->hdrLn;

    valueM = ctx_ptr->macLn ? ((ctx_ptr->macLn - 2) << 2) : 0;
    // store adata and tag length in context nonce
    b0[0] |= (((hdr_len != 0) ? CCM_B0_FLAGS_ADATA : 0) + valueM);
    if (CECL_AES_NONCE_SIZE_BYTES < sizeof(b0))
        return CEML_ERROR_INVALID_PARAM;
    CeElMemScpy((uint8 *)(ctx_ptr->nonce), sizeof(b0), b0, sizeof(b0));

    // formulate B1,..Bn and put them in hdr array
    // we are explicitly checking if the required padding bytes
    // are zero to ensure that caller is aware that he requires
    // to pass padding
    if (hdr_len) {
        /* encode the length field if there is some associated data
           Below header lengths are as defined by CCM standard */
        if (hdr_len < 65536 - 256) {
            hdr[0] = (uint8)(hdr_len >> 8);
            hdr[1] = (uint8)hdr_len;
        } else if (hdr_len < 0x0000000100000000ULL) {
            hdr[0] = 0xff;
            hdr[1] = 0xfe;
            hdr[2] = (uint8)(hdr_len >> 24);
            hdr[3] = (uint8)(hdr_len >> 16);
            hdr[4] = (uint8)(hdr_len >> 8);
            hdr[5] = (uint8)hdr_len;
        } else {
#if 0
      hdr[0] = 0xff;
      hdr[1] = 0xff;
      hdr[2] = (uint8)(hdr_len >> 56);
      hdr[3] = (uint8)(hdr_len >> 48);
      hdr[4] = (uint8)(hdr_len >> 40);
      hdr[5] = (uint8)(hdr_len >> 32);
      hdr[6] = (uint8)(hdr_len >> 24);
      hdr[7] = (uint8)(hdr_len >> 16);
      hdr[8] = (uint8)(hdr_len >>  8);
      hdr[9] = (uint8)hdr_len;
#endif
            ret = CEML_ERROR_NOT_SUPPORTED;
        }
    }

    return ret;
}

/**
 * @brief The Crypto HW expects SW to calculate counter
 * for CTR encryption in CCM mode
 *
 * @param[in] ceMlHandle pointer to context Cryto Engine context
 *
 * @return CEML_ERROR_SUCCESS if successful,
 *         CEML_ERROR_SUCCESS otherwise
 *
 * @dependenies - nonceLn needs to be > 7 bytes or < 13bytes
 *
 */
static CeMLErrorType CCMIVInit(CeMLCntxHandle *ceMlHandle)
{
    CeMLErrorType ret = CEML_ERROR_SUCCESS;
    CeCLCipherCntxType *ctx_ptr = NULL;
    uint8 iv[CEML_AES_IV_SIZE];

    /* Sanity check inputs */
    if ((!ceMlHandle) || (!ceMlHandle->pClientCtxt)) {
        return CEML_ERROR_INVALID_PARAM;
    }

    // Create context pointer
    ctx_ptr = (CeCLCipherCntxType *)&(((CeMLCipherAlgoCntxType *)((ceMlHandle)->pClientCtxt))->ctx);

    CeElMemset(iv, 0, sizeof(iv));

    if (ctx_ptr->nonceLn < 7 || ctx_ptr->nonceLn > 13)
        return CEML_ERROR_INVALID_PARAM;

    /*
    ** write q-1 vlaue to iv[0].
    ** q is the nbr of bytes in the payload
    ** (q is the octet length of
    ** the binary representation of the octet length of the payload.)
    ** nonce length + q should always be 15
    */
    iv[0] = CEML_AES_BLOCK_SIZE - 2 - ctx_ptr->nonceLn;

    /*
    ** Copy nonce from caller to local IV
    */
    CeElMemScpy((uint8 *)(&(iv[1])), ctx_ptr->nonceLn, (uint8 *)(ctx_ptr->nonce), ctx_ptr->nonceLn);
    /*
    ** Copy calculated complete local IV (based on NIST SP 800-38C)
    ** into the context IV location
    */
    CeElMemScpy((uint8 *)(ctx_ptr->iv), sizeof(iv), (uint8 *)(iv), sizeof(iv));

    return ret;
}

/**
 * @brief
 *
 *
 * @return None
 *
 * @see refer to header for definition
 *
 */
CeMLErrorType CeMLInit(void)
{
    CeElXferModeType xferMode;
    CeMLErrorType ret_val = CEML_ERROR_SUCCESS;
    CeElMutexEnter();
    do {
        // Enable CE clocks
        ret_val = (CeMLErrorType)CeElEnableClock();
        if (CEML_ERROR_SUCCESS != ret_val) {
            break;
        }

        // Get Xfer mode
        ret_val = (CeMLErrorType)CeElGetXferModeList(&xferMode);  // dxe
        if (CEML_ERROR_SUCCESS != ret_val) {
            ret_val = CEML_ERROR_FAILURE;
            break;
        }

        // Decide which mode to use for intializating of CE config register
        ret_val = (CeMLErrorType)CeClInit((CeCLXferModeType)xferMode);
        if (CEML_ERROR_SUCCESS != ret_val) {
            break;
        }

        // Enable BAM
        ret_val = (CeMLErrorType)CeElInit();
        if (CEML_ERROR_SUCCESS != ret_val) {
            break;
        }
    } while (0);

    CeElMutexExit();
    return ret_val;
}

/**
 * @brief
 *
 *
 * @return None
 *
 * @see refer to header for definition
 *
 */
CeMLErrorType CeMLDeInit(void)
{
    CeElMutexEnter();
    CeElDeInit();
    CeClDeinit();
    // DO NOT disable QCC CLK
    // CeElDisableClock();
    CeElMutexExit();
    CeEL_mutex_deinit();
    return CEML_ERROR_SUCCESS;
}

/**
 * @brief
 *
 *
 * @return None
 *
 * @see refer to header for definition
 *
 */
CeMLErrorType _CeMlHashUpdate(CeMLHashAlgoCntxType *ctx_ptr, uint8 *buff_ptr, uint32 buff_size, boolean lastBlock)
{
    CeMLErrorType ret_val = CEML_ERROR_SUCCESS;
    uint32 pdwActualOut = 0;
    uint32 tmp_size = 0;
    CeElXferModeType xferMode;
    CeElXferFunctionType xferFunc;
    CEELIovecListType inIoVec;
    CEELIovecListType outIoVec;
    CEELIovecType inIoVecT;
    CEELIovecType outIoVecT;
    uint8 non_blocking_flag = 0;

    /* Sanity check inputs */
    if (!ctx_ptr || !buff_ptr) {
        return CEML_ERROR_INVALID_PARAM;
    }

    if (lastBlock == TRUE) {
        ctx_ptr->ctx.lastBlock = TRUE;
    } else {
        ctx_ptr->ctx.lastBlock = FALSE;
    }

    do {
        ctx_ptr->ctx.dataLn = buff_size;
        // ret_val = (CeMLErrorType)CeCLIOCtlSetHashCntx((CeCLHashAlgoCntxType*) (&(ctx_ptr->ctx)));
        ret_val = (CeMLErrorType)CeClIOCtlHash(CECL_IOCTL_SET_HASH_CNTXT, (uint8 *)(&(ctx_ptr->ctx)),
                                               sizeof(CeCLHashAlgoCntxType), NULL, 0, &pdwActualOut);
        if (CEML_ERROR_SUCCESS != ret_val) {
            break;
        }

        tmp_size = buff_size;
        if (buff_size % 16) {
            tmp_size = buff_size + (16 - (buff_size % 16));
        }

        inIoVec.size = 1;
        inIoVec.iov = &inIoVecT;
        inIoVec.iov->pvBase = buff_ptr;
        inIoVec.iov->dwLen = tmp_size;

        outIoVec.size = 1;
        outIoVec.iov = &outIoVecT;
        outIoVec.iov->pvBase = (void *)ctx_ptr->ctx.auth_iv;
        if (ctx_ptr->ctx.algo == CECL_HASH_ALGO_SHA1) {
            outIoVec.iov->dwLen = CECL_HASH_SHA_IV_LEN;
        } else if (ctx_ptr->ctx.algo == CECL_HASH_ALGO_SHA256) {
            outIoVec.iov->dwLen = CECL_HASH_SHA256_IV_LEN;
        }

        ret_val =
            CeElCryptoDxeShaXfer(inIoVec.iov->pvBase, inIoVec.iov->dwLen, outIoVec.iov->pvBase, outIoVec.iov->dwLen);
        // while( !(HAL_REG_RD(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_STATUS_REG) &
        // QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_STATUS_OPERATION_DONE_MASK));
        if (CEML_ERROR_SUCCESS != ret_val) {
            // ctx_ptr->non_blocking_ptr.pending_dm_request = 0;
            break;
        }

        // Read HASH context
        // ret_val = (CeMLErrorType)CeCLIOCtlGetHashCntx((CeCLHashAlgoCntxType*) (&(ctx_ptr->ctx)));
        pdwActualOut = sizeof(CeCLHashAlgoCntxType);
        // if (ctx_ptr->ctx.auth_nonblock_mode == 0 || ctx_ptr->ctx.lastBlock == 1)
        //

        ret_val = (CeMLErrorType)CeClIOCtlHash(CECL_IOCTL_GET_HASH_CNTXT, NULL, 0, (uint8 *)(&(ctx_ptr->ctx)),
                                               pdwActualOut, &pdwActualOut);
        if (CEML_ERROR_SUCCESS != ret_val) {
            break;
        }
        //

    } while (0);

    return ret_val;
}

/**
 * @brief
 *
 * @param
 *
 * @return
 *
 * @see refer to header for definition
 *
 */
CeMLErrorType CeMLHashInit(CeMLCntxHandle **ceMlHandle, CeMLHashAlgoType Algo)
{
    CeMLErrorType ret_val = CEML_ERROR_SUCCESS;

    /* Sanity check inputs */
    if ((CEML_HASH_ALGO_SHA1 != Algo) && (CEML_HASH_ALGO_SHA256 != Algo) && (CEML_HASH_ALGO_CMAC128 != Algo) &&
        (CEML_HASH_ALGO_CMAC256 != Algo)) {
        return CEML_ERROR_INVALID_PARAM;
    }

    // NOTE: PBL caller is expected to have allocated space for the handle
    /* Sanity check inputs */
    if ((!ceMlHandle) || (!*ceMlHandle) || (!(*ceMlHandle)->pClientCtxt) ||
        (CEML_PBL_HASHCTX_SIZE < sizeof(CeMLHashAlgoCntxType))) {
        return CEML_ERROR_INVALID_PARAM;
    }

    // Clear HASH context
    CeElMemset(((*ceMlHandle)->pClientCtxt), 0, sizeof(CeMLHashAlgoCntxType));

    do {
        CeMLHashAlgoCntxType *hashCntx = (CeMLHashAlgoCntxType *)((*ceMlHandle)->pClientCtxt);

        hashCntx->ctx.firstBlock = 1;

        /* load initial IV */
        if (CEML_HASH_ALGO_SHA1 == Algo) {
            hashCntx->ctx.algo = CECL_HASH_ALGO_SHA1;

            /* standard initialization vector for SHA-1, source: FIPS 180-2 */
            /* changed to little-endian format */
            hashCntx->ctx.auth_iv[0] = 0x01234567;
            hashCntx->ctx.auth_iv[1] = 0x89ABCDEF;
            hashCntx->ctx.auth_iv[2] = 0xFEDCBA98;
            hashCntx->ctx.auth_iv[3] = 0x76543210;
            hashCntx->ctx.auth_iv[4] = 0xF0E1D2C3;
        } else if (CEML_HASH_ALGO_SHA256 == Algo) {
            hashCntx->ctx.algo = CECL_HASH_ALGO_SHA256;

            /* standard initialization vector for SHA-256, source: FIPS 180-2 */
            /* changed to little-endian format */
            hashCntx->ctx.auth_iv[0] = 0x67E6096A;
            hashCntx->ctx.auth_iv[1] = 0x85AE67BB;
            hashCntx->ctx.auth_iv[2] = 0x72F36E3C;
            hashCntx->ctx.auth_iv[3] = 0x3AF54FA5;
            hashCntx->ctx.auth_iv[4] = 0x7F520E51;
            hashCntx->ctx.auth_iv[5] = 0x8C68059B;
            hashCntx->ctx.auth_iv[6] = 0xABD9831F;
            hashCntx->ctx.auth_iv[7] = 0x19CDE05B;
        } else if (CEML_HASH_ALGO_CMAC128 == Algo) {
            hashCntx->ctx.algo = CECL_HASH_ALGO_CMAC128;
        } else if (CEML_HASH_ALGO_CMAC256 == Algo) {
            hashCntx->ctx.algo = CECL_HASH_ALGO_CMAC256;
        } else {
            return CEML_ERROR_INVALID_PARAM;
        }
    } while (0);

    return ret_val;
}

/**
 * @brief
 *
 * @param
 *
 * @return
 *
 * @see refer to header for definition
 *
 */
CeMLErrorType CeMLHashDeInit(CeMLCntxHandle **ceMlHandle)
{
    CeMLErrorType ret_val = CEML_ERROR_SUCCESS;

    return ret_val;
}

/**
 * @brief
 *
 * @param
 *
 * @return
 *
 * @see  refer to header for definition
 *
 */
static CeMLErrorType CeMLHashLinearUpdate(CeMLHashAlgoCntxType *hashCntx, uint8 *pData, uint32 nDataLen)
{
    CeMLErrorType ret_val = CEML_ERROR_SUCCESS;
    volatile uint32 ce_status = 0;
    uint32 total_data_len = 0;
    uint32 bytes_remaining = 0;
    uint32 bytes_to_write = 0;
    uint32 bytes_filled_into_saved_buff = 0;
    uint32 internal_auth_nonblock_flag = 0;
    uint32 pdwActualOut;

    /* Sanity check inputs */
    if ((!hashCntx) || (!pData)) {
        return CEML_ERROR_INVALID_PARAM;
    }

    if (hashCntx->ctx.auth_nonblock_mode == 1) {
        ce_status = HAL_REG_RD(CECL_CE_STATUS);
        CeElMemoryBarrier();
        if (!(ce_status & CECL_CE_STATUS_OPERATION_DONE_BMSK)) {
            return CEML_ERROR_HW_BUSY;
        } else {
            pdwActualOut = sizeof(CeCLHashAlgoCntxType);
            if (hashCntx->ctx.auth_nonblock_update_flag != 0) {
                CeClIOCtlHash(CECL_IOCTL_GET_HASH_CNTXT, NULL, 0, (uint8 *)(&(hashCntx->ctx)), pdwActualOut,
                              &pdwActualOut);
            }
        }
    }

    do {
        total_data_len = hashCntx->saved_buff_index + nDataLen;

        // ensure there was no integer overflow for total_data_len
        if (total_data_len < nDataLen) {
            ret_val = CEML_ERROR_FAILURE;
            break;
        }

        if (total_data_len <= CEML_SHA_BLOCK_SIZE) {
            /* not enough bytes for a full block */
            ret_val = (CeMLErrorType)CeElMemScpy(&hashCntx->saved_buff[hashCntx->saved_buff_index], nDataLen, pData,
                                                 nDataLen);
            if (CEML_ERROR_SUCCESS != ret_val) {
                break;
            }

            hashCntx->saved_buff_index += nDataLen;
            break;
        } else {
            if (hashCntx->saved_buff_index != 0) {
                // fill in the saved_buff with the data and then CeMlHashUpdate
                // and then save remaining data from pData into saved_buff
                bytes_filled_into_saved_buff = CEML_SHA_BLOCK_SIZE - hashCntx->saved_buff_index;

                // integer underflow check for bytes_filled_into_saved_buff
                // Added check for Klocwork
                if (hashCntx->saved_buff_index > CEML_SHA_BLOCK_SIZE ||
                    bytes_filled_into_saved_buff > CEML_SHA_BLOCK_SIZE) {
                    ret_val = CEML_ERROR_FAILURE;
                    break;
                }

                if (hashCntx->saved_buff_index < CEML_SHA_BLOCK_SIZE) {
                    ret_val =
                        (CeMLErrorType)CeElMemScpy(&hashCntx->saved_buff[hashCntx->saved_buff_index],
                                                   bytes_filled_into_saved_buff, pData, bytes_filled_into_saved_buff);
                    if (CEML_ERROR_SUCCESS != ret_val) {
                        break;
                    }
                }

                if (hashCntx->ctx.auth_nonblock_mode == 1) {
                    internal_auth_nonblock_flag = 1;
                    hashCntx->ctx.auth_nonblock_mode = 0;
                }
                hashCntx->saved_buff_index = CEML_SHA_BLOCK_SIZE;
                ret_val = _CeMlHashUpdate(hashCntx, hashCntx->saved_buff, hashCntx->saved_buff_index, FALSE);
                if (CEML_ERROR_SUCCESS != ret_val) {
                    break;
                }
                if (internal_auth_nonblock_flag == 1) {
                    hashCntx->ctx.auth_nonblock_mode = 1;
                    internal_auth_nonblock_flag = 0;
                    hashCntx->ctx.auth_nonblock_pad_flag = 1;
                }
            }

            // integer underflow check for bytes_to_write and bytes_remaining
            if (bytes_filled_into_saved_buff > nDataLen) {
                ret_val = CEML_ERROR_FAILURE;
                break;
            }

            bytes_to_write = (nDataLen - bytes_filled_into_saved_buff) / CEML_SHA_BLOCK_SIZE;
            bytes_to_write *= CEML_SHA_BLOCK_SIZE;

            bytes_remaining = (nDataLen - bytes_filled_into_saved_buff) % CEML_SHA_BLOCK_SIZE;

            /* even if we have full blocks, we need the final 64-bytes
             * processed with CE_Hash_Final() */
            if (0 == bytes_remaining) {
                bytes_remaining = CEML_SHA_BLOCK_SIZE;
                bytes_to_write -= CEML_SHA_BLOCK_SIZE;
            }

            ret_val = (CeMLErrorType)CeElMemScpy(&hashCntx->saved_buff[0], bytes_remaining,
                                                 &pData[(nDataLen - bytes_remaining)], bytes_remaining);
            if (CEML_ERROR_SUCCESS != ret_val) {
                break;
            }

            hashCntx->saved_buff_index = bytes_remaining;

            if (bytes_to_write) {
                /* Crypto Engine can handle maximum of CEML_MAX_BLOCK_SIZE in one shot, so if
                 * the data is more than CEML_MAX_BLOCK_SIZE then split them into less
                 * than CEML_MAX_BLOCK_SIZE chunks and the process them
                 */
                uint8 *data_ptr = pData + bytes_filled_into_saved_buff;
                uint32 bytes_pending = bytes_to_write;
                do {
                    if (bytes_pending > CEML_MAX_BLOCK_SIZE) {
                        ret_val = _CeMlHashUpdate(hashCntx, data_ptr, CEML_MAX_BLOCK_SIZE, FALSE);
                        if (CEML_ERROR_SUCCESS != ret_val) {
                            break;
                        }
                        hashCntx->ctx.auth_nonblock_pad_flag = 1;
                        bytes_pending -= CEML_MAX_BLOCK_SIZE;
                        data_ptr += CEML_MAX_BLOCK_SIZE;
                    } else {
                        ret_val = _CeMlHashUpdate(hashCntx, data_ptr, bytes_pending, FALSE);
                        if (CEML_ERROR_SUCCESS != ret_val) {
                            break;
                        }
                        hashCntx->ctx.auth_nonblock_pad_flag = 1;

                        bytes_pending = 0;
                    }
                } while (bytes_pending > 0);
            }
        }
    } while (0);

    return ret_val;
}

/**
 * @brief
 *
 * @param
 *
 * @return
 *
 * @see  refer to header for definition
 *
 */

CeMLErrorType CeMLHashUpdate(CeMLCntxHandle *ceMlHandle, CeMLIovecListType ioVecIn)
{
    CeMLHashAlgoCntxType *hashCntx;
    uint8 *pData = NULL;
    uint32 nDataLen = 0;
    CeMLErrorType ret_val = CEML_ERROR_SUCCESS;

    /* Sanity check inputs */
    if ((!ceMlHandle) || (!ceMlHandle->pClientCtxt)) {
        return CEML_ERROR_INVALID_PARAM;
    }

    if ((ioVecIn.size != 1) || (!ioVecIn.iov)) {
        return CEML_ERROR_INVALID_PARAM;
    }

    CeElMutexEnter();
    pData = (uint8 *)ioVecIn.iov[0].pvBase;
    nDataLen = ioVecIn.iov[0].dwLen;

    // Set up context pointer
    hashCntx = (CeMLHashAlgoCntxType *)(ceMlHandle->pClientCtxt);

    ret_val = CeMLHashLinearUpdate(
        hashCntx, pData,
        nDataLen);  // This handles both non-blocking and blocking cases. Don't be confused with the API name

    hashCntx->ctx.auth_nonblock_update_flag = 1;
    CeElMutexExit();
    return ret_val;
}

/**
 * @brief
 *
 * @param
 *
  @return
 *
 * @see refer to header for definition
 *
 */
CeMLErrorType CeMLHashFinal(CeMLCntxHandle *ceMlHandle, CeMLIovecListType *ioVecOut)
{
    CeMLErrorType ret_val = CEML_ERROR_SUCCESS;
    uint32 hashLen = 0;
    CEMLIovecListType iovec;
    CEMLIovecType iovec_buf;
    CeMLHashAlgoCntxType *hashCntx = NULL;
    uint32 i = 0;
    volatile uint32 ce_status;
    uint32 pdwActualOut;

    iovec.size = 1;
    iovec.iov = &iovec_buf;

    /* Sanity check inputs */
    if ((!ceMlHandle) || (!ceMlHandle->pClientCtxt)) {
        return CEML_ERROR_INVALID_PARAM;
    }

    if ((!ioVecOut) || (ioVecOut->size != 1) || (!ioVecOut->iov)) {
        return CEML_ERROR_INVALID_PARAM;
    }

    hashCntx = (CeMLHashAlgoCntxType *)(ceMlHandle->pClientCtxt);
    {
        if (hashCntx->ctx.algo == CECL_HASH_ALGO_SHA1) {
            hashLen = CEML_HASH_DIGEST_SIZE_SHA1;
        } else if (hashCntx->ctx.algo == CECL_HASH_ALGO_SHA256) {
            hashLen = CEML_HASH_DIGEST_SIZE_SHA256;
        } else {
            return CEML_ERROR_INVALID_PARAM;
        }

        if ((ioVecOut->iov[0].dwLen < hashLen) || (ioVecOut->iov[0].pvBase == NULL)) {
            return CEML_ERROR_INVALID_PARAM;
        }

        CeElMutexEnter();
        // printf("auth_nonblock_mode:%d auth_nonblock_pad_flag:%d\n",
        //        hashCntx->ctx.auth_nonblock_mode,
        //        hashCntx->ctx.auth_nonblock_pad_flag);

        if (hashCntx->ctx.auth_nonblock_mode == 1 && hashCntx->ctx.auth_nonblock_pad_flag == 1) {
            // if (hashCntx->non_blocking_ptr.pending_dm_request == 1) {
            while (1) {
                ce_status = HAL_REG_RD(CECL_CE_STATUS);
                CeElMemoryBarrier();
                if (ce_status & CECL_CE_STATUS_OPERATION_DONE_BMSK) {
                    break;
                }
            }
            // hashCntx->non_blocking_ptr.pending_dm_request = 0;

            pdwActualOut = sizeof(CeCLHashAlgoCntxType);
            ret_val = (CeMLErrorType)CeClIOCtlHash(CECL_IOCTL_GET_HASH_CNTXT, NULL, 0, (uint8 *)(&(hashCntx->ctx)),
                                                   pdwActualOut, &pdwActualOut);
            if (CEML_ERROR_SUCCESS != ret_val) {
                CeElMutexExit();
                return ret_val;
            }
        }

        for (i = 0; i < hashCntx->non_blocking_ptr.counter; i++) {
            hashCntx->ctx.auth_nonblock_mode = 0;
            hashCntx->ctx.auth_no_context = 1;
            iovec.iov[0].pvBase =
                (void *)hashCntx->non_blocking_ptr.input[hashCntx->non_blocking_ptr.pointer_location].input_ptr;
            iovec.iov[0].dwLen = hashCntx->non_blocking_ptr.input_size[hashCntx->non_blocking_ptr.pointer_location];
            if (CEML_ERROR_SUCCESS != CeMLHashUpdate(ceMlHandle, iovec)) {
                CeElMutexExit();
                return CEML_ERROR_FAILURE;
            }
            hashCntx->non_blocking_ptr.pointer_location =
                (hashCntx->non_blocking_ptr.pointer_location + 1) % (NON_BLOCKING_POINTER_LIST_SIZE);
        }
        hashCntx->non_blocking_ptr.counter = 0;
        hashCntx->ctx.auth_nonblock_mode = 0;
        hashCntx->ctx.auth_no_context = 1;
    }

    do {
        if (hashCntx->saved_buff_index > 0) {
            // check for integer underflow for memset operation
            if (hashCntx->saved_buff_index > CEML_SHA_BLOCK_SIZE) {
                ret_val = CEML_ERROR_FAILURE;
                break;
            }

            if (hashCntx->saved_buff_index < CEML_SHA_BLOCK_SIZE) {
                /* we only have the saved buffer left */
                ret_val = (CeMLErrorType)CeElMemset(hashCntx->saved_buff + (hashCntx->saved_buff_index), 0,
                                                    (CEML_SHA_BLOCK_SIZE - hashCntx->saved_buff_index));
                if (CEML_ERROR_SUCCESS != ret_val) {
                    break;
                }
            }

            ret_val = _CeMlHashUpdate(hashCntx, hashCntx->saved_buff, hashCntx->saved_buff_index, TRUE);
            if (CEML_ERROR_SUCCESS != ret_val) {
                break;
            }
        }

        // copy the hash result into ioVecOut
        ret_val = (CeMLErrorType)CeElMemScpy(ioVecOut->iov[0].pvBase, hashLen, (void *)hashCntx->ctx.auth_iv, hashLen);
        if (CEML_ERROR_SUCCESS != ret_val) {
            break;
        }

        ioVecOut->size = 1;
        ioVecOut->iov[0].dwLen = hashLen;

    } while (0);
    // else

    CeElMutexExit();
    return ret_val;
}

/**
 * @brief
 *
 * @param
 *
 * @return
 *
 * @see refer to header for definition
 *
 */
CeMLErrorType CeMLHashAtomic(CeMLCntxHandle *ceMlHandle, CeMLIovecListType ioVecIn, CeMLIovecListType *ioVecOut)
{
    CeMLErrorType ret_val = CEML_ERROR_SUCCESS;
    uint32 hashLen = 0;
    CeMLHashAlgoCntxType *hashCntx = NULL;
    uint8 *data_ptr = NULL;
    uint32 bytes_pending = 0;

    /* Sanity check inputs */
    if ((!ceMlHandle) || (!ceMlHandle->pClientCtxt) || (!ioVecOut)) {
        return CEML_ERROR_INVALID_PARAM;
    }

    if (((ioVecOut->size != 1) || (!ioVecOut->iov)) || ((ioVecIn.size != 1) || (!ioVecIn.iov))) {
        return CEML_ERROR_INVALID_PARAM;
    }

    hashCntx = (CeMLHashAlgoCntxType *)(ceMlHandle->pClientCtxt);

    if (hashCntx->ctx.algo == CECL_HASH_ALGO_SHA1) {
        hashLen = CEML_HASH_DIGEST_SIZE_SHA1;
    } else if (hashCntx->ctx.algo == CECL_HASH_ALGO_SHA256) {
        hashLen = CEML_HASH_DIGEST_SIZE_SHA256;
    } else {
        return CEML_ERROR_INVALID_PARAM;
    }

    if (ioVecOut->iov[0].dwLen < hashLen) {
        return CEML_ERROR_INVALID_PARAM;
    }

    CeElMutexEnter();

    data_ptr = (uint8 *)(ioVecIn.iov[0].pvBase);
    bytes_pending = ioVecIn.iov[0].dwLen;

    do {
        // Chose the arm crypto engine if user explicitly sets or if
        // no choice is made, then fuse is final decider
        {
            do {
                if (bytes_pending > CEML_MAX_BLOCK_SIZE) {
                    ret_val = _CeMlHashUpdate(hashCntx, data_ptr, CEML_MAX_BLOCK_SIZE, FALSE);
                    if (CEML_ERROR_SUCCESS != ret_val) {
                        break;
                    }

                    bytes_pending -= CEML_MAX_BLOCK_SIZE;
                    data_ptr += CEML_MAX_BLOCK_SIZE;
                } else {
                    ret_val = _CeMlHashUpdate(hashCntx, data_ptr, bytes_pending, TRUE);
                    if (CEML_ERROR_SUCCESS != ret_val) {
                        break;
                    }

                    bytes_pending = 0;
                }
            } while (bytes_pending > 0);

            if (ret_val == CEML_ERROR_SUCCESS) {
                // copy the hash result into ioVecOut
                ret_val = (CeMLErrorType)CeElMemScpy(ioVecOut->iov[0].pvBase, hashLen, (void *)hashCntx->ctx.auth_iv,
                                                     hashLen);
                if (CEML_ERROR_SUCCESS != ret_val) {
                    break;
                }
            }
        }
        ioVecOut->size = 1;
        ioVecOut->iov[0].dwLen = hashLen;
    } while (0);

    CeElMutexExit();

    return ret_val;
}

CeMLErrorType CeMLBISTverify(void)
{
    return CEML_ERROR_SUCCESS;
}
/**
 * @brief
 *
 * @param
 *
  @return
 *
 * @see
 *
 */
static CeMLErrorType CmacUpdate(CeMLCntxHandle *ceMlHandle, CEMLIovecListType ioVecIn, CEMLIovecListType *ioVecOut,
                                boolean lastblock)
{
    CeMLErrorType ret_val = CEML_ERROR_SUCCESS;
    CeCLHashAlgoCntxType *ctx_ptr = NULL;
    uint32 pdwActualOut;
    CeElXferModeType xferMode;
    CeElXferFunctionType xferFunc;
    uint32 curr_blk_size;
    uint32 num_blks;
    CEMLIovecListType ioVecIn_tmp;
    uint32 input_len;
    uint32 input_len2;
    uint8 *input_ptr;
    uint8 *input_ptr2;
    uint8 non_blocking_flag = 0;
    /* Sanity check inputs */
    if ((!ceMlHandle) || (!ceMlHandle->pClientCtxt) || ((ioVecOut->size != 1) || (!ioVecOut->iov)) ||
        ((ioVecIn.size != 1))) {
        return CEML_ERROR_INVALID_PARAM;
    }

    // Create context pointer
    ctx_ptr = (CeCLHashAlgoCntxType *)&(((CeMLHashAlgoCntxType *)((ceMlHandle)->pClientCtxt))->ctx);

    CeElMutexEnter();

    // Init temp IoVect pointers
    ioVecIn_tmp = ioVecIn;
    input_len2 = ioVecIn.iov->dwLen;
    input_len = ioVecIn.iov->dwLen;
    input_ptr = ioVecIn_tmp.iov->pvBase;
    input_ptr2 = ioVecIn_tmp.iov->pvBase;

    // Calculate block size based on input data length
    num_blks = ioVecIn_tmp.iov->dwLen / CEML_MAX_BLOCK_SIZE;
    if ((ioVecIn_tmp.iov->dwLen % CEML_MAX_BLOCK_SIZE) || input_len == 0) {
        ++num_blks;
    }

    // Set up loop to process all input data in blks of max size of 32K bytes
    while (num_blks--) {
        // Calculate current blk size
        curr_blk_size = CEML_MAX_BLOCK_SIZE;
        if (num_blks == 0) {
            if (lastblock == TRUE) {
                // Set last block
                ctx_ptr->lastBlock = 1;
            }

            /* As per CMAC standards, if last block is not complete, then
               Mn = K2 ^ (Mn* || 10j) where j = nb-Mlen-1

               CMAC padding support in Crypto Core auth block is explained in detail in "Crypto HDD - Figure 3-53
               CMAC architecture diagram". However, the input data length check in caller functions is making sure
               that it receives only the multiple of 16 bytes blocks.
            */

            curr_blk_size = (ioVecIn_tmp.iov->dwLen % CEML_MAX_BLOCK_SIZE);
            if (curr_blk_size == 0 && input_len != 0) {
                curr_blk_size = CEML_MAX_BLOCK_SIZE;
            }
        }

        // Set sizes
        ctx_ptr->dataLn = curr_blk_size;
        ctx_ptr->seg_size = curr_blk_size;
        ioVecIn_tmp.iov->dwLen = curr_blk_size;

        // Set HASH context
        pdwActualOut = 0;
        ret_val = (CeMLErrorType)CeClIOCtlHash(CECL_IOCTL_SET_HASH_CNTXT, (uint8 *)(ctx_ptr),
                                               sizeof(CeCLHashAlgoCntxType), NULL, 0, &pdwActualOut);
        if (ret_val != CEML_ERROR_SUCCESS) {
            break;
        }

        ret_val = CeElCryptoDxeXfer(ioVecIn_tmp.iov->pvBase, ioVecIn_tmp.iov->dwLen, ioVecOut->iov->pvBase,
                                    ioVecOut->iov->dwLen);

        if (CEML_ERROR_SUCCESS != ret_val) {
            break;
        }

        // Get Hash context
        pdwActualOut = sizeof(CeCLHashAlgoCntxType);
        ret_val = (CeMLErrorType)CeClIOCtlHash(CECL_IOCTL_GET_HASH_CNTXT, NULL, 0, (uint8 *)(ctx_ptr), pdwActualOut,
                                               &pdwActualOut);
        if (CEML_ERROR_SUCCESS != ret_val) {
            break;
        }

        // Increment IoVect Tmp pointers
        input_ptr += curr_blk_size;
        ioVecIn_tmp.iov->pvBase = input_ptr;

        // Decrement Input size
        input_len -= curr_blk_size;
        ioVecIn_tmp.iov->dwLen = input_len;

        // Clear first block flag
        ctx_ptr->firstBlock = 0;
    } /* while */

    // Make sure we set in/out length/ptr again
    ioVecIn.iov->dwLen = input_len2;
    ioVecIn.iov->pvBase = input_ptr2;

    CeElMutexExit();

    return ret_val;
}

/**
 * @brief This function will create a Cmac message digest using
 *        the algorithm specified.
 *
 * @param key_ptr       [in]  Pointer to key
 * @param keylen        [in]  Length of input key in bytes
 * @param ioVecIn       [in]  Pointer to input data to hash
 * @param ioVecOut      [out] Pointer to output data
 * @param pAlgo         [in]  Algorithm type
 *
 * @return CeMLErrorType
 *
 * @see
 *
 */
CeMLErrorType CeMLCmac(uint8 *key_ptr, uint32 keylen, CeMLIovecListType ioVecIn, CeMLIovecListType *ioVecOut,
                       CeMLHashAlgoType pAlgo)
{
    CeMLCntxHandle *cntx = NULL;
    CeMLCntxHandle ceMlHandle;
    CeMLHashAlgoCntxType ClientCtxt;
    CeMLErrorType ret_val = CEML_ERROR_SUCCESS;

    // Check algo
    if ((pAlgo != CEML_HASH_ALGO_CMAC128) && (pAlgo != CEML_HASH_ALGO_CMAC256)) {
        return CEML_ERROR_INVALID_PARAM;
    }

    // Sanity check
    if ((key_ptr == NULL) || (!keylen) || (!ioVecOut) || !ioVecIn.iov || (ioVecIn.size != 1) || (ioVecOut->size != 1) ||
        (ioVecOut->iov[0].pvBase == NULL) || !ioVecIn.iov->dwLen) {
        return CEML_ERROR_INVALID_PARAM;
    }

    if ((pAlgo == CEML_HASH_ALGO_CMAC128) && (keylen != 16)) {
        return CEML_ERROR_INVALID_PARAM;
    }
    if ((pAlgo == CEML_HASH_ALGO_CMAC256) && (keylen != 32)) {
        return CEML_ERROR_INVALID_PARAM;
    }

    // Sanity check digest size for between 8 - 16
    if (ioVecOut->iov[0].dwLen < CEML_CMAC_DIGEST_SIZE / 2 || ioVecOut->iov[0].dwLen > CEML_CMAC_DIGEST_SIZE) {
        return CEML_ERROR_INVALID_PARAM;
    }
    cntx = &ceMlHandle;
    cntx->pClientCtxt = &ClientCtxt;

    do {
        // Init HASH cntx
        ret_val = CeMLHashInit(&cntx, pAlgo);
        if (ret_val != CEML_ERROR_SUCCESS) {
            break;
        }

        // Set AUTH KEY
        ret_val = CeMLHashSetParam(cntx, CEML_HASH_PARAM_AUTH_KEY, key_ptr, keylen, pAlgo);
        if (ret_val != CEML_ERROR_SUCCESS) {
            break;
        }

        // Perform CMAC on input data
        if (CmacUpdate(cntx, ioVecIn, ioVecOut, TRUE) != CEML_ERROR_SUCCESS) {
            ret_val = CEML_ERROR_FAILURE;
            break;
        }

        // Copy the CMAC digest result back to caller
        CeElMemScpy(ioVecOut->iov->pvBase, ioVecOut->iov->dwLen, (uint8 *)ClientCtxt.ctx.auth_iv, ioVecOut->iov->dwLen);

    } while (0);

    // Release HASH cntx
    if (NULL != cntx) {
        CeMLHashDeInit(&cntx);
    }

    return ret_val;
}

/**
 * @brief This function will create a Hmac message digest using
 *        the algorithm specified.
 *
 * @param hmac_key      [in]  Pointer to key
 * @param hmac_key_len  [in]  Length of input key in bytes
 * @param ioVecIn       [in]  Pointer to input data to hash
 * @param ioVecOut      [out] Pointer to output data
 * @param palgo         [in]  Algorithm type
 *
 * @return CeMLErrorType
 *
 * @see
 *
 */
CeMLErrorType CeMLHmac(uint8 *key_ptr, uint32 keylen, CeMLIovecListType ioVecIn, CeMLIovecListType *ioVecOut,
                       CeMLHashAlgoType pAlgo)
{
    CeMLCntxHandle *cntx = NULL;
    CeMLCntxHandle ceMlHandle;
    CeMLHashAlgoCntxType ClientCtxt;
    CeMLErrorType ret_val = CEML_ERROR_SUCCESS;
    uint8 ipad[CEML_HASH_DIGEST_BLOCK_SIZE];
    uint8 opad[CEML_HASH_DIGEST_BLOCK_SIZE];
    uint8 key[CEML_HASH_DIGEST_BLOCK_SIZE];
    uint32 key_len = CEML_HASH_DIGEST_BLOCK_SIZE;
    uint32 hmac[CEML_HASH_DIGEST_SIZE_SHA256];
    uint32 i = 0;
    CeMLIovecListType ioVecInTmp;
    CeMLIovecListType ioVecOutTmp;
    CeMLIovecType IovecInTmp;
    CeMLIovecType IovecOutTmp;
    uint32 hash_size = 0;

    if ((pAlgo != CEML_HASH_ALGO_SHA256) && (pAlgo != CEML_HASH_ALGO_SHA1)) {
        return CEML_ERROR_INVALID_PARAM;
    }

    if ((key_ptr == NULL) || (keylen == 0) || (ioVecIn.size != 1) || (ioVecOut == NULL) || (ioVecOut->size != 1) ||
        (ioVecOut->iov[0].pvBase == NULL)) {
        return CEML_ERROR_INVALID_PARAM;
    }

    if (((pAlgo == CEML_HASH_ALGO_SHA256) && (ioVecOut->iov[0].dwLen < CEML_HASH_DIGEST_SIZE_SHA256)) ||
        ((pAlgo == CEML_HASH_ALGO_SHA1) && (ioVecOut->iov[0].dwLen < CEML_HASH_DIGEST_SIZE_SHA1))) {
        return CEML_ERROR_INVALID_PARAM;
    }

    cntx = &ceMlHandle;
    cntx->pClientCtxt = &ClientCtxt;

    ioVecInTmp.size = 1;
    ioVecInTmp.iov = &IovecInTmp;

    ioVecOutTmp.size = 1;
    ioVecOutTmp.iov = &IovecOutTmp;

    /* Update the hash size */
    switch (pAlgo) {
        case CEML_HASH_ALGO_SHA256:
            hash_size = CEML_HASH_DIGEST_SIZE_SHA256;
            break;
        case CEML_HASH_ALGO_SHA1:
            hash_size = CEML_HASH_DIGEST_SIZE_SHA1;
            break;
        default:
            /* Validation on pAlgo has already been done */
            break;
    }

    if (keylen > CEML_HASH_DIGEST_BLOCK_SIZE) {
        ret_val = CeMLHashInit(&cntx, pAlgo);

        if (ret_val != CEML_ERROR_SUCCESS) {
            CeMLHashDeInit(&cntx);
            return ret_val;
        }

        ioVecInTmp.iov[0].pvBase = key_ptr;
        ioVecInTmp.iov[0].dwLen = keylen;

        ioVecOutTmp.iov[0].pvBase = key;
        ioVecOutTmp.iov[0].dwLen = hash_size;

        ret_val = CeMLHashUpdate(cntx, ioVecInTmp);
        if (ret_val != CEML_ERROR_SUCCESS) {
            CeMLHashDeInit(&cntx);
            return ret_val;
        }

        ret_val = CeMLHashFinal(cntx, &ioVecOutTmp);
        if (ret_val != CEML_ERROR_SUCCESS) {
            CeMLHashDeInit(&cntx);
            return ret_val;
        }

        CeMLHashDeInit(&cntx);

        CeElMemset(key + hash_size, 0, (key_len - hash_size));

    } else {
        CeElMemScpy(key, keylen, key_ptr, keylen);
        // Added check for Klocwork
        if (keylen < CEML_HASH_DIGEST_BLOCK_SIZE && (key_len - keylen) <= CEML_HASH_DIGEST_BLOCK_SIZE)
            CeElMemset(key + keylen, 0, (key_len - keylen));
    }

    CeElMemset(ipad, 0, CEML_HASH_DIGEST_BLOCK_SIZE);
    CeElMemset(opad, 0, CEML_HASH_DIGEST_BLOCK_SIZE);
    CeElMemScpy(ipad, key_len, key, key_len);
    CeElMemScpy(opad, key_len, key, key_len);

    for (i = 0; i < CEML_HASH_DIGEST_BLOCK_SIZE; i++) {
        ipad[i] ^= 0x36;
        opad[i] ^= 0x5C;
    }

    /* Inner Hash */
    ret_val = CeMLHashInit(&cntx, pAlgo);
    if (ret_val != CEML_ERROR_SUCCESS) {
        return ret_val;
    }

    ioVecInTmp.iov[0].pvBase = ipad;
    ioVecInTmp.iov[0].dwLen = CEML_HASH_DIGEST_BLOCK_SIZE;

    ioVecOutTmp.iov[0].pvBase = hmac;
    ioVecOutTmp.iov[0].dwLen = hash_size;

    ret_val = CeMLHashUpdate(cntx, ioVecInTmp);
    if (ret_val != CEML_ERROR_SUCCESS) {
        CeMLHashDeInit(&cntx);
        return ret_val;
    }

    // Perform HASH Update only on data with non zero pointer and length
    if ((ioVecIn.iov[0].dwLen) && (ioVecIn.iov[0].pvBase)) {
        ioVecInTmp.iov[0].pvBase = ioVecIn.iov[0].pvBase;
        ioVecInTmp.iov[0].dwLen = ioVecIn.iov[0].dwLen;
        ret_val = CeMLHashUpdate(cntx, ioVecInTmp);
        if (ret_val != CEML_ERROR_SUCCESS) {
            CeMLHashDeInit(&cntx);
            return ret_val;
        }
    }

    ret_val = CeMLHashFinal(cntx, &ioVecOutTmp);
    if (ret_val != CEML_ERROR_SUCCESS) {
        CeMLHashDeInit(&cntx);
        return ret_val;
    }

    CeMLHashDeInit(&cntx);

    /* Outer Hash */
    ret_val = CeMLHashInit(&cntx, pAlgo);
    if (ret_val != CEML_ERROR_SUCCESS) {
        CeMLHashDeInit(&cntx);
        return ret_val;
    }

    ioVecInTmp.iov[0].pvBase = opad;
    ioVecInTmp.iov[0].dwLen = CEML_HASH_DIGEST_BLOCK_SIZE;
    ret_val = CeMLHashUpdate(cntx, ioVecInTmp);
    if (ret_val != CEML_ERROR_SUCCESS) {
        CeMLHashDeInit(&cntx);
        return ret_val;
    }

    ioVecInTmp.iov[0].pvBase = hmac;
    ioVecInTmp.iov[0].dwLen = hash_size;
    ret_val = CeMLHashUpdate(cntx, ioVecInTmp);
    if (ret_val != CEML_ERROR_SUCCESS) {
        CeMLHashDeInit(&cntx);
        return ret_val;
    }

    ret_val = CeMLHashFinal(cntx, ioVecOut);
    if (ret_val != CEML_ERROR_SUCCESS) {
        CeMLHashDeInit(&cntx);
        return ret_val;
    }

    CeMLHashDeInit(&cntx);

    return ret_val;
}

/**
 * @brief This function will init for a Hmac message digest using
 *        the algorithm specified.
 *
 * @param cntx          [in]  Pointer to pointer for hmac context
 * @param palgo         [in]  Algorithm type
 *
 * @return CeMLErrorType
 *
 * @see
 *
 */
CeMLErrorType CeMLHmacInit(CeMLCntxHandle **cntx, CeMLHashAlgoType pAlgo)
{
    CeMLErrorType ret_val = CEML_ERROR_SUCCESS;
    CeMLHashAlgoCntxType *hashCntx = NULL;

    if ((pAlgo != CEML_HASH_ALGO_SHA256) && (pAlgo != CEML_HASH_ALGO_SHA1)) {
        return CEML_ERROR_INVALID_PARAM;
    }

    /* Inner Hash */
    ret_val = CeMLHashInit(cntx, pAlgo);
    if (ret_val != CEML_ERROR_SUCCESS) {
        CeMLHashDeInit(cntx);
        return ret_val;
    }

    hashCntx = (CeMLHashAlgoCntxType *)((*cntx)->pClientCtxt);

    hashCntx->ctx.mode = CECL_HASH_MODE_HMAC;

    hashCntx->ctx.firstBlock = 1;

    return ret_val;
}

/**
 * @brief This function will update a Hmac message digest using
 *        the algorithm specified. It is callable multiple times
 *
 * @param cntx          [in]  Pointer to hmac context
 * @param ioVecIn       [in]  Pointer to input data to hmac
 *
 * @return CeMLErrorType
 *
 * @see
 *
 */
CeMLErrorType CeMLHmacUpdate(CeMLCntxHandle *cntx, CeMLIovecListType ioVecIn)
{
    CeMLErrorType ret_val = CEML_ERROR_SUCCESS;
    uint8 ipad[CEML_HASH_DIGEST_BLOCK_SIZE];
    uint8 opad[CEML_HASH_DIGEST_BLOCK_SIZE];
    uint32 key_len = CECL_HMAC_MAX_KEY_SIZE * sizeof(uint32);
    uint32 i = 0;
    CeMLIovecListType ioVecInTmp;
    CeMLIovecType IovecInTmp;
    CeMLHashAlgoCntxType *hashCntx = NULL;

    /* Sanity check inputs */
    if ((!cntx) || (!cntx->pClientCtxt)) {
        return CEML_ERROR_INVALID_PARAM;
    }

    if ((ioVecIn.size != 1) || (!ioVecIn.iov)) {
        return CEML_ERROR_INVALID_PARAM;
    }

    hashCntx = (CeMLHashAlgoCntxType *)(cntx->pClientCtxt);

    if (hashCntx->ctx.firstBlock == 1) {
        CeElMemset(ipad, 0, CEML_HASH_DIGEST_BLOCK_SIZE);
        CeElMemset(opad, 0, CEML_HASH_DIGEST_BLOCK_SIZE);
        CeElMemScpy(ipad, key_len, hashCntx->ctx.hmac_key, key_len);
        CeElMemScpy(opad, key_len, hashCntx->ctx.hmac_key, key_len);

        for (i = 0; i < CEML_HASH_DIGEST_BLOCK_SIZE; i++) {
            ipad[i] ^= 0x36;
            opad[i] ^= 0x5C;
        }

        CeElMemScpy(((CeMLHashAlgoCntxType *)((cntx)->pClientCtxt))->ctx.opad, key_len, opad, key_len);

        ioVecInTmp.size = 1;
        ioVecInTmp.iov = &IovecInTmp;

        ioVecInTmp.iov[0].pvBase = ipad;
        ioVecInTmp.iov[0].dwLen = CEML_HASH_DIGEST_BLOCK_SIZE;

        ret_val = CeMLHashUpdate(cntx, ioVecInTmp);
        if (ret_val != CEML_ERROR_SUCCESS) {
            CeElMemset(ipad, 0, CEML_HASH_DIGEST_BLOCK_SIZE);
            CeMLHashDeInit(&cntx);
            return ret_val;
        }

        CeElMemset(ipad, 0, CEML_HASH_DIGEST_BLOCK_SIZE);

        hashCntx->ctx.firstBlock = 0;
    }

    // Perform HASH Update only on data with non zero pointer and length
    if ((ioVecIn.iov[0].dwLen) && (ioVecIn.iov[0].pvBase)) {
        ret_val = CeMLHashUpdate(cntx, ioVecIn);
        if (ret_val != CEML_ERROR_SUCCESS) {
            return ret_val;
        }
    }
    return ret_val;
}

/**
 * @brief This function will finalize a Hmac message digest using
 *        the algorithm specified.
 *
 * @param cntx          [in]  Pointer to hmac context
 * @param ioVecOut      [out] Pointer to output data
 *
 * @return CeMLErrorType
 *
 * @see
 *
 */
CeMLErrorType CeMLHmacFinal(CeMLCntxHandle *cntx, CeMLIovecListType *ioVecOut)
{
    CeMLHashAlgoType pAlgo;
    CeMLErrorType ret_val = CEML_ERROR_SUCCESS;
    uint8 opad[CEML_HASH_DIGEST_BLOCK_SIZE];
    uint32 hmac[CEML_HASH_DIGEST_SIZE_SHA256];
    CeMLIovecListType ioVecInTmp;
    CeMLIovecListType ioVecOutTmp;
    CeMLIovecType IovecInTmp;
    CeMLIovecType IovecOutTmp;
    uint32 hash_size = 0;
    CeMLCntxHandle *cntx_int = NULL;

    /* Sanity check inputs */
    if ((!cntx) || (!cntx->pClientCtxt)) {
        return CEML_ERROR_INVALID_PARAM;
    }

    if ((!ioVecOut) || (ioVecOut->size != 1) || (!ioVecOut->iov)) {
        return CEML_ERROR_INVALID_PARAM;
    }

    pAlgo = (CeMLHashAlgoType)(((CeMLHashAlgoCntxType *)((cntx)->pClientCtxt))->ctx.algo);

    /* Update the hash size */
    switch (pAlgo) {
        case CEML_HASH_ALGO_SHA256:
            hash_size = CEML_HASH_DIGEST_SIZE_SHA256;
            break;
        case CEML_HASH_ALGO_SHA1:
            hash_size = CEML_HASH_DIGEST_SIZE_SHA1;
            break;
        default:
            return CEML_ERROR_INVALID_PARAM;
    }

    ioVecInTmp.size = 1;
    ioVecInTmp.iov = &IovecInTmp;

    ioVecOutTmp.size = 1;
    ioVecOutTmp.iov = &IovecOutTmp;

    ioVecOutTmp.iov[0].pvBase = hmac;
    ioVecOutTmp.iov[0].dwLen = hash_size;

    CeElMemScpy(opad, CEML_HASH_DIGEST_BLOCK_SIZE, ((CeMLHashAlgoCntxType *)((cntx)->pClientCtxt))->ctx.opad,
                CEML_HASH_DIGEST_BLOCK_SIZE);

    ret_val = CeMLHashFinal(cntx, &ioVecOutTmp);
    if (ret_val != CEML_ERROR_SUCCESS) {
        return ret_val;
    }
    CeElMemScpy(hmac, hash_size, ((CeMLHashAlgoCntxType *)((cntx)->pClientCtxt))->ctx.auth_iv, hash_size);

    /* Outer Hash */
    ret_val = CeMLHashInit(&cntx, pAlgo);
    if (ret_val != CEML_ERROR_SUCCESS) {
        CeMLHashDeInit(&cntx_int);
        return ret_val;
    }

    ioVecInTmp.iov[0].pvBase = opad;
    ioVecInTmp.iov[0].dwLen = CEML_HASH_DIGEST_BLOCK_SIZE;

    ret_val = CeMLHashUpdate(cntx, ioVecInTmp);
    if (ret_val != CEML_ERROR_SUCCESS) {
        CeElMemset(opad, 0, CEML_HASH_DIGEST_BLOCK_SIZE);
        CeElMemset(((CeMLHashAlgoCntxType *)((cntx)->pClientCtxt))->ctx.opad, 0, CEML_HASH_DIGEST_BLOCK_SIZE);
        CeMLHashDeInit(&cntx);
        return ret_val;
    }

    ioVecInTmp.iov[0].pvBase = hmac;
    ioVecInTmp.iov[0].dwLen = hash_size;

    ret_val = CeMLHashUpdate(cntx, ioVecInTmp);
    if (ret_val != CEML_ERROR_SUCCESS) {
        CeElMemset(opad, 0, CEML_HASH_DIGEST_BLOCK_SIZE);
        CeElMemset(((CeMLHashAlgoCntxType *)((cntx)->pClientCtxt))->ctx.opad, 0, CEML_HASH_DIGEST_BLOCK_SIZE);
        CeMLHashDeInit(&cntx);
        return ret_val;
    }

    ret_val = CeMLHashFinal(cntx, ioVecOut);
    if (ret_val != CEML_ERROR_SUCCESS) {
        CeElMemset(opad, 0, CEML_HASH_DIGEST_BLOCK_SIZE);
        CeElMemset(((CeMLHashAlgoCntxType *)((cntx)->pClientCtxt))->ctx.opad, 0, CEML_HASH_DIGEST_BLOCK_SIZE);
        CeMLHashDeInit(&cntx_int);
        return ret_val;
    }

    CeElMemset(opad, 0, CEML_HASH_DIGEST_BLOCK_SIZE);
    CeElMemset(((CeMLHashAlgoCntxType *)((cntx)->pClientCtxt))->ctx.opad, 0, CEML_HASH_DIGEST_BLOCK_SIZE);

    CeMLHashDeInit(&cntx_int);

    return ret_val;
}

/**
 * @brief
 *
 * @param
 *
 * @return
 *
 * @see refer to header for definition
 *
 */
CeMLErrorType CeMLHmacDeInit(CeMLCntxHandle **ceMlHandle)
{
    CeMLErrorType ret_val = CEML_ERROR_SUCCESS;

    return ret_val;
}
static CeMLErrorType hmacKeyCalc(void *valueSrcAddr, uint32 valueSrcSize, const void *pParam, uint32 cParam,
                                 CeMLHashAlgoType pAlgo)
{
    CeMLErrorType ret_val = CEML_ERROR_SUCCESS;
    CeMLCntxHandle *cntx = NULL;
    CeMLCntxHandle ceMlHandle;
    CeMLHashAlgoCntxType ClientCtxt;

    CEMLIovecListType ioVecInTmp;
    CEMLIovecListType ioVecOutTmp;
    CEMLIovecType IovecInTmp;
    CEMLIovecType IovecOutTmp;
    uint32 hash_size = 0;

    /* Update the hash size */
    switch (pAlgo) {
        case CEML_HASH_ALGO_SHA256:
            hash_size = CEML_HASH_DIGEST_SIZE_SHA256;
            break;
        case CEML_HASH_ALGO_SHA1:
            hash_size = CEML_HASH_DIGEST_SIZE_SHA1;
            break;
        default:
            return CEML_ERROR_FAILURE;
    }

    /* If the Key length is not 64, truncate or extend the key as per FIPS 198-1 */
    if (cParam > CEML_HASH_DIGEST_BLOCK_SIZE) {
        ioVecInTmp.size = 1;
        ioVecInTmp.iov = &IovecInTmp;
        ioVecOutTmp.size = 1;
        ioVecOutTmp.iov = &IovecOutTmp;

        cntx = &ceMlHandle;
        cntx->pClientCtxt = &ClientCtxt;
        ret_val = CeMLHashInit(&cntx, pAlgo);
        if (ret_val != CEML_ERROR_SUCCESS) {
            CeMLHashDeInit(&cntx);
            return CEML_ERROR_FAILURE;
        }

        ioVecInTmp.iov[0].pvBase = (void *)pParam;
        ioVecInTmp.iov[0].dwLen = cParam;
        ioVecOutTmp.iov[0].pvBase = valueSrcAddr;
        ioVecOutTmp.iov[0].dwLen = hash_size;

        ret_val = CeMLHashUpdate(cntx, ioVecInTmp);
        if (ret_val != CEML_ERROR_SUCCESS) {
            CeMLHashDeInit(&cntx);
            return CEML_ERROR_FAILURE;
        }

        ret_val = CeMLHashFinal(cntx, &ioVecOutTmp);
        if (ret_val != CEML_ERROR_SUCCESS) {
            CeMLHashDeInit(&cntx);
            return CEML_ERROR_FAILURE;
        }

        CeMLHashDeInit(&cntx);

        CeElMemset((uint8 *)valueSrcAddr + hash_size, 0, (valueSrcSize - hash_size));
    } else {
        if (CeElMemScpy(valueSrcAddr, valueSrcSize, (void *)pParam, valueSrcSize) != 0) {
            ret_val = CEML_ERROR_FAILURE;
        }

        // Added check for Klocwork
        if (cParam < CEML_HASH_DIGEST_BLOCK_SIZE && (valueSrcSize - cParam) <= CEML_HASH_DIGEST_BLOCK_SIZE)
            CeElMemset((uint8 *)valueSrcAddr + cParam, 0, (valueSrcSize - cParam));
    }

    return ret_val;
}

/**
 * @brief This functions sets the Hash paramaters - Mode and Key
 *        for HMAC.
 *
 * @param ceMlHandle [in] Pointer to cipher context handle
 * @param nParamID   [in] HMAC parameter id to set
 * @param pParam     [in] Pointer to parameter data
 * @param cParam     [in] Size of parameter data in bytes
 * @param Algo      [in] Algorithm type
 *
 * @return CeMLErrorType
 *
 */
CeMLErrorType CeMLHashSetParam(CeMLCntxHandle *ceMlHandle, CeMLHashParamType nParamID, const void *pParam,
                               uint32 cParam, CeMLHashAlgoType Algo)
{
    CeMLErrorType retVal = CEML_ERROR_SUCCESS;
    CeCLHashAlgoCntxType *me = NULL;
    uint8 key[CEML_HASH_DIGEST_BLOCK_SIZE];
    uint32 key_len = CEML_HASH_DIGEST_BLOCK_SIZE;
    if (!ceMlHandle) {
        return CEML_ERROR_INVALID_PARAM;
    }

    if (!ceMlHandle->pClientCtxt) {
        return CEML_ERROR_INVALID_PARAM;
    }

    me = &((CeMLHashAlgoCntxType *)((ceMlHandle)->pClientCtxt))->ctx;

    switch (nParamID) {
        case CEML_HASH_PARAM_MODE:
            if (!pParam) {
                return CEML_ERROR_INVALID_PARAM;
            }
            me->mode = (CeCLHashModeType)(*((CeMLHashModeType *)pParam));
            break;

        case CEML_HASH_PARAM_HMAC_KEY:
            if (!pParam || !cParam) {
                return CEML_ERROR_INVALID_PARAM;
            }

            retVal = (CeMLErrorType)hmacKeyCalc((void *)key, key_len, pParam, cParam, Algo);
            if (retVal == CEML_ERROR_SUCCESS) {
                if (CeElMemScpy(me->hmac_key, key_len, (uint8 *)key, key_len) != 0) {
                    retVal = CEML_ERROR_FAILURE;
                    break;
                }
            }
            break;
        case CEML_HASH_PARAM_AUTH_KEY:
            // Write key to context structure
            if (!pParam) {
                me->bAESUseHWKey = TRUE;
                break;
            } else if ((cParam == 16) || (cParam == 32)) {
                if (CeElMemScpy(me->auth_key, sizeof(me->auth_key), (uint8 *)pParam, cParam) != 0) {
                    retVal = CEML_ERROR_FAILURE;
                    break;
                }
            } else {
                retVal = CEML_ERROR_INVALID_PARAM;
                break;
            }

            break;
        case CEML_HASH_NONBLOCK_MODE:
            if (!pParam) {
                return CEML_ERROR_INVALID_PARAM;
            }

            // Check key size
            if (cParam == 0 || cParam > sizeof(uint8)) {
                return CEML_ERROR_INVALID_PARAM;
            }

            // enable/disable non blocking mode
            if (CeElIsPollingMode()) {
                /* for polling mode and boot images use setting */
                ((CeMLHashAlgoCntxType *)((ceMlHandle)->pClientCtxt))->ctx.auth_nonblock_mode = *(uint8 *)pParam;
            } else {
                /* interrupt mode is enabled, so no need of this */
                ((CeMLHashAlgoCntxType *)((ceMlHandle)->pClientCtxt))->ctx.auth_nonblock_mode = 0;
            }

            break;

        case CEML_HASH_PARAM_IV:
            if (me->algo == CECL_HASH_ALGO_SHA1) {
                if (cParam != CEML_HASH_DIGEST_SIZE_SHA1 || !pParam) {
                    retVal = CEML_ERROR_INVALID_PARAM;
                    break;
                }
                CeElMemScpy(me->auth_iv, CEML_HASH_DIGEST_SIZE_SHA1, (uint8 *)pParam, CEML_HASH_DIGEST_SIZE_SHA1);
            } else if (me->algo == CECL_HASH_ALGO_SHA256) {
                if (cParam != CEML_HASH_DIGEST_SIZE_SHA256 || !pParam) {
                    retVal = CEML_ERROR_INVALID_PARAM;
                    break;
                }
                CeElMemScpy(me->auth_iv, CEML_HASH_DIGEST_SIZE_SHA256, (uint8 *)pParam, CEML_HASH_DIGEST_SIZE_SHA256);
            }

            else {
                retVal = CEML_ERROR_INVALID_PARAM;
                break;
            }

            break;

        default:
            retVal = CEML_ERROR_INVALID_PARAM;
            break;
    }

    return retVal;

} /* CeMLHashSetParam() */

/**
 * @brief
 *
 * @param
 *
  @return
 *
 * @see refer to header for definition
 *
 */
CeMLErrorType CeMLHashGetParam(CeMLCntxHandle *ceMlHandle, CeMLHashParamType nParamID, void *pParam, uint32 cParam)
{
    CeMLErrorType retVal = CEML_ERROR_SUCCESS;
    volatile uint32 ce_status = 0;
    CeMLHashAlgoCntxType *hashCntx = NULL;
    CeCLHashAlgoCntxType *me = NULL;
    CeMLHashNonBlockingBuffStatus *status;
    uint32 pdwActualOut;

    if (!ceMlHandle)
        return CEML_ERROR_INVALID_PARAM;

    hashCntx = (CeMLHashAlgoCntxType *)((ceMlHandle)->pClientCtxt);
    if (!hashCntx)
        return CEML_ERROR_INVALID_PARAM;

    me = &((CeMLHashAlgoCntxType *)((ceMlHandle)->pClientCtxt))->ctx;
    if (!me)
        return CEML_ERROR_INVALID_PARAM;

    {
        switch (nParamID) {
            case CEML_HASH_PARAM_IV:
                if (!pParam) {
                    retVal = CEML_ERROR_INVALID_PARAM;
                    break;
                }
                if (me->algo == CECL_HASH_ALGO_SHA1) {
                    if (cParam != CEML_HASH_DIGEST_SIZE_SHA1) {
                        retVal = CEML_ERROR_INVALID_PARAM;
                        break;
                    }
                    CeElMemScpy(pParam, CEML_HASH_DIGEST_SIZE_SHA1, me->auth_iv, CEML_HASH_DIGEST_SIZE_SHA1);
                } else if (me->algo == CECL_HASH_ALGO_SHA256) {
                    if (cParam != CEML_HASH_DIGEST_SIZE_SHA256) {
                        retVal = CEML_ERROR_INVALID_PARAM;
                        break;
                    }
                    CeElMemScpy((uint8 *)pParam, CEML_HASH_DIGEST_SIZE_SHA256, me->auth_iv,
                                CEML_HASH_DIGEST_SIZE_SHA256);
                } else if (me->algo == CECL_HASH_ALGO_CMAC128 || me->algo == CECL_HASH_ALGO_CMAC256) {
                    if (CEML_AES128_IV_SIZE != cParam) {
                        retVal = CEML_ERROR_INVALID_PARAM;
                        break;
                    }
                    // Copy IV vector to context
                    CeElMemScpy(pParam, cParam, me->auth_iv, cParam);
                }

                else {
                    retVal = CEML_ERROR_INVALID_PARAM;
                    break;
                }

                break;
            case CEML_HASH_IS_NONBLOCK_BUFF_PROCESSED:
                if ((cParam != sizeof(CeMLHashNonBlockingBuffStatus)) || !pParam) {
                    retVal = CEML_ERROR_INVALID_PARAM;
                    break;
                }

                status = (CeMLHashNonBlockingBuffStatus *)pParam;
                status->isProcessed = FALSE;

                ce_status = HW_REG_RD(CECL_CE_STATUS);
                CeElMemoryBarrier();
                if (ce_status & CECL_CE_STATUS_OPERATION_DONE_BMSK) {
                    status->isProcessed = TRUE;
                    pdwActualOut = sizeof(CeCLHashAlgoCntxType);
                    if (hashCntx->ctx.auth_nonblock_update_flag != 0) {
                        CeClIOCtlHash(CECL_IOCTL_GET_HASH_CNTXT, NULL, 0, (uint8 *)(&(hashCntx->ctx)), pdwActualOut,
                                      &pdwActualOut);
                    }
                }

                break;
            default:
                retVal = CEML_ERROR_INVALID_PARAM;
                break;
        }
    }

    return retVal;
}

/**
 * @brief
 *
 * @param
 *
  @return
 *
 * @see refer to header for definition
 *
 */
CeMLErrorType CeMLCipherInit(CeMLCntxHandle **ceMlHandle, CeMLCipherAlgType Algo)
{
    /* Sanity check inputs */
    if ((CEML_CIPHER_ALG_AES128 != Algo) && (CEML_CIPHER_ALG_AES256 != Algo)) {
        return CEML_ERROR_INVALID_PARAM;
    }

    // NOTE: PBL caller is expected to have allocated space for the handle
    /* Sanity check inputs */
    if ((!ceMlHandle) || (!*ceMlHandle) || (!(*ceMlHandle)->pClientCtxt) ||
        (CEML_PBL_CIPHERCTX_SIZE < sizeof(CeCLCipherCntxType))) {
        return CEML_ERROR_INVALID_PARAM;
    }
    CeElMemset(((*ceMlHandle)->pClientCtxt), 0, sizeof(CeMLCipherAlgoCntxType));

    ((CeMLCipherAlgoCntxType *)((*ceMlHandle)->pClientCtxt))->ctx.algo = (CeCLCipherAlgType)Algo;

    return CEML_ERROR_SUCCESS;
}

/**
 * @brief
 *
 * @param
 *
  @return
 *
 * @see refer to header for definition
 *
 */
CeMLErrorType CeMLCipherDeInit(CeMLCntxHandle **ceMlHandle)
{
    if ((!ceMlHandle) || (!*ceMlHandle)) {
        return CEML_ERROR_INVALID_PARAM;
    }

    return CEML_ERROR_SUCCESS;
}

/**
 * @brief
 *
 * @param
 *
  @return
 *
 * @see refer to header for definition
 *
 */
CeMLErrorType CeMLCipherSetParam(CeMLCntxHandle *ceMlHandle, CeMLCipherParamType nParamID, const void *pParam,
                                 uint32 cParam)
{
    CeMLErrorType retVal = CEML_ERROR_SUCCESS;
    CeCLCipherCntxType *me = NULL;

    if (!ceMlHandle) {
        return CEML_ERROR_INVALID_PARAM;
    }

    if (!ceMlHandle->pClientCtxt) {
        return CEML_ERROR_INVALID_PARAM;
    }

    me = &((CeMLCipherAlgoCntxType *)((ceMlHandle)->pClientCtxt))->ctx;
    switch (nParamID) {
        case CEML_CIPHER_PARAM_DIRECTION:
            if ((sizeof(CeMLCipherDir) != cParam) || (!pParam)) {
                retVal = CEML_ERROR_INVALID_PARAM;
                break;
            }
            me->dir = (CeCLCipherDir)(*((CeCLCipherDir *)pParam));
            break;

        case CEML_CIPHER_PARAM_KEY:
            if (me->algo == CECL_CIPHER_ALG_AES128) {
                // If NULL key pointer use HW key
                if (!pParam) {
                    me->bAESUseHWKey = TRUE;
                    break;
                }

                if ((CEML_AES128_KEY_SIZE != cParam) && (pParam != 0)) {
                    retVal = CEML_ERROR_INVALID_PARAM;
                    break;
                }
            } else if (me->algo == CECL_CIPHER_ALG_AES256) {
                // If NULL key pointer use HW key
                if (!pParam) {
                    me->bAESUseHWKey = TRUE;
                    break;
                }

                if ((CEML_AES256_KEY_SIZE != cParam) && (pParam != 0)) {
                    retVal = CEML_ERROR_INVALID_PARAM;
                    break;
                }
            } else {
                retVal = CEML_ERROR_INVALID_PARAM;
                break;
            }

            if (((CEML_AES128_KEY_SIZE == cParam) || (CEML_AES256_KEY_SIZE == cParam)) && (pParam != 0)) {
                // Write key to context structure
                CeElMemScpy(me->aes_key, cParam, (void *)pParam, cParam);
            } else {
                retVal = CEML_ERROR_INVALID_PARAM;
            }

            break;

        case CEML_CIPHER_PARAM_IV:
            if (me->algo == CECL_CIPHER_ALG_AES128 || me->algo == CECL_CIPHER_ALG_AES256) {
                if (CEML_AES128_IV_SIZE != cParam) {
                    retVal = CEML_ERROR_INVALID_PARAM;
                    break;
                }
            } else {
                retVal = CEML_ERROR_INVALID_PARAM;
                break;
            }

            if (pParam) {
                // Copy IV vector to context
                CeElMemScpy(me->iv, cParam, (void *)pParam, cParam);
            } else {
                retVal = CEML_ERROR_INVALID_PARAM;
            }
            break;

        case CEML_CIPHER_PARAM_MODE:
            if (me->algo == CECL_CIPHER_ALG_AES128 || me->algo == CECL_CIPHER_ALG_AES256) {
                if ((sizeof(CeMLCipherModeType) != cParam) || !pParam) {
                    retVal = CEML_ERROR_INVALID_PARAM;
                    break;
                }
            } else {
                retVal = CEML_ERROR_INVALID_PARAM;
                break;
            }

            /* Set context mode */
            me->mode = (CeCLCipherModeType)(*((CeCLCipherModeType *)pParam));
            break;

        case CEML_CIPHER_PARAM_CCM_PAYLOAD_LEN:
            /* See which Crypto engine version we are using */
            if ((sizeof(uint32) != cParam) || (!pParam)) {
                retVal = CEML_ERROR_INVALID_PARAM;
                break;
            }
            me->payloadLn = (*((uint32 *)pParam));
            break;

        case CEML_CIPHER_PARAM_NONCE:
            if ((me->algo == CECL_CIPHER_ALG_AES128 || me->algo == CECL_CIPHER_ALG_AES256) &&
                ((pParam) && (cParam <= CECL_AES_NONCE_SIZE_BYTES))) {
                // Copy nonce data to context
                CeElMemScpy(me->nonce, cParam, (void *)pParam, cParam);
                // Set nonce data size in bytes
                me->nonceLn = cParam;
            } else {
                retVal = CEML_ERROR_INVALID_PARAM;
            }
            break;

        case CEML_CIPHER_PARAM_CCM_MAC_LEN:
            // if ((sizeof(uint32) != cParam) || (!pParam))
            if ((sizeof(uint32) != cParam) || !pParam) {
                retVal = CEML_ERROR_INVALID_PARAM;
                break;
            }
            me->macLn = (*((uint32 *)pParam));
            break;

        case CEML_CIPHER_PARAM_CCM_HDR_LEN:
            /* See which Crypto engine version we are using */
            if ((sizeof(uint32) != cParam) || (!pParam)) {
                retVal = CEML_ERROR_INVALID_PARAM;
                break;
            }
            me->hdrLn = (*((uint32 *)pParam));
            break;

        default:
            retVal = CEML_ERROR_INVALID_PARAM;
            break;
    }

    return retVal;
}

/**
 * @brief
 *
 * @param
 *
  @return
 *
 * @see refer to header for definition
 *
 */
CeMLErrorType CeMLCipherGetParam(CeMLCntxHandle *ceMlHandle, CeMLCipherParamType nParamID, void *pParam, uint32 *cParam)
{
    CeMLErrorType retVal = CEML_ERROR_SUCCESS;
    CeCLCipherCntxType *me = NULL;

    if (!ceMlHandle) {
        return CEML_ERROR_INVALID_PARAM;
    }

    if (!ceMlHandle->pClientCtxt) {
        return CEML_ERROR_INVALID_PARAM;
    }

    me = &((CeMLCipherAlgoCntxType *)((ceMlHandle)->pClientCtxt))->ctx;

    {
        switch (nParamID) {
            case CEML_CIPHER_PARAM_DIRECTION:
                if (!cParam || (sizeof(CeMLCipherDir) != *cParam) || (!pParam)) {
                    retVal = CEML_ERROR_INVALID_PARAM;
                    break;
                }
                (*((CeMLCipherDir *)pParam)) = (CeMLCipherDir)me->dir;
                break;

            case CEML_CIPHER_PARAM_IV:
                if (me->algo == CECL_CIPHER_ALG_AES128 || me->algo == CECL_CIPHER_ALG_AES256) {
                    if ((cParam != 0) && (CEML_AES128_IV_SIZE != *cParam)) {
                        retVal = CEML_ERROR_INVALID_PARAM;
                        break;
                    }
                } else {
                    retVal = CEML_ERROR_INVALID_PARAM;
                    break;
                }

                if ((cParam != 0) && (pParam != NULL)) {
                    if (CEML_AES128_IV_SIZE == *cParam) {
                        // Get IV vector
                        CeElMemScpy((void *)pParam, *cParam, me->iv, *cParam);
                    } else {
                        retVal = CEML_ERROR_INVALID_PARAM;
                        break;
                    }
                } else {
                    retVal = CEML_ERROR_INVALID_PARAM;
                }
                break;

            case CEML_CIPHER_PARAM_MODE:
                if (me->algo == CECL_CIPHER_ALG_AES128 || me->algo == CECL_CIPHER_ALG_AES256) {
                    if (!cParam || sizeof(CeMLCipherModeType) != *cParam) {
                        retVal = CEML_ERROR_INVALID_PARAM;
                        break;
                    }
                } else {
                    retVal = CEML_ERROR_INVALID_PARAM;
                    break;
                }

                if (pParam) {
                    (*((CeCLCipherModeType *)pParam)) = me->mode;
                } else {
                    retVal = CEML_ERROR_INVALID_PARAM;
                    break;
                }
                break;

            case CEML_CIPHER_PARAM_NONCE:
                if ((me->algo == CECL_CIPHER_ALG_AES128 || me->algo == CECL_CIPHER_ALG_AES256) &&
                    ((pParam) && (*cParam <= CECL_AES_NONCE_SIZE_BYTES))) {
                    // Get Nonce data
                    CeElMemScpy((void *)pParam, *cParam, me->nonce, *cParam);
                } else {
                    retVal = CEML_ERROR_INVALID_PARAM;
                }
                break;

            default:
                retVal = CEML_ERROR_INVALID_PARAM;
                break;
        }
    }

    return retVal;
}

/**
 * @brief
 *
 * @param
 *
 * @return
 *
 * @see refer to header for definition
 *
 */
CeMLErrorType CeMLCipherData(CeMLCntxHandle *ceMlHandle, CeMLIovecListType ioVecIn, CeMLIovecListType *ioVecOut)
{
    CeMLErrorType ret = CEML_ERROR_SUCCESS;
    CeCLCipherCntxType *ctx_ptr = NULL;
    uint32 pdwActualOut;
    CeElXferModeType xferMode;
    CeElXferFunctionType xferFunc;
    uint32 curr_blk_size = 0;
    uint32 curr_out_blk_size = 0;
    uint32 num_blks;
    CeMLIovecListType ioVecIn_tmp;
    CeMLIovecListType ioVecOut_tmp;
    uint32 input_len;
    uint32 input_len2;
    uint32 output_len;
    uint8 *input_ptr;
    uint8 *input_ptr2;
    uint8 *output_ptr;
    uint8 *output_ptr2;
    boolean cts_mode = FALSE;
    boolean ccm_star_auth_mode = FALSE;
    boolean ccm_star_encr_mode = FALSE;
    uint8 ccmstar_iv[CEML_AES_IV_SIZE];
    uint8 ccmstar_cbcmac_iv[CEML_AES_IV_SIZE];
    uint32_t regVal = 0;
    uint8_t count = 0;
    uint8_t index = 0;
#define BLOCK_SZ 4
    /* Sanity check inputs */
    if ((!ceMlHandle) || (!ceMlHandle->pClientCtxt)) {
        return CEML_ERROR_INVALID_PARAM;
    }

    // Create context pointer
    ctx_ptr = (CeCLCipherCntxType *)&(((CeMLCipherAlgoCntxType *)((ceMlHandle)->pClientCtxt))->ctx);

    if (!ioVecOut) {
        return CEML_ERROR_INVALID_PARAM;
    }

    if (((ioVecOut->size != 1) || (!ioVecOut->iov)) || ((ioVecIn.size != 1) || (!ioVecIn.iov))) {
        return CEML_ERROR_INVALID_PARAM;
    }

    if (ctx_ptr->mode == CECL_CIPHER_MODE_CTS && ioVecIn.iov->dwLen < 16) {
        return CEML_ERROR_INVALID_PARAM;
    }

    if (((ctx_ptr->mode != CECL_CIPHER_MODE_CTS) && (ctx_ptr->mode != CECL_CIPHER_MODE_CTR) &&
         (ctx_ptr->mode != CECL_CIPHER_MODE_CCM)) &&
        (ioVecIn.iov->dwLen % 16 != 0)) {
        return CEML_ERROR_INVALID_PARAM;
    }

    // If CCM mode check mac length
    if (ctx_ptr->mode == CECL_CIPHER_MODE_CCM) {
        /*
        ** Check the header length
        */
        if (ctx_ptr->hdrLn >= ioVecIn.iov->dwLen) {
            return CEML_ERROR_INVALID_PARAM;
        }
    }

    // If CTS mode set it to CBC mode
    // HW treat non-multiple of 16 bytes data
    // with AES-CBC-CTS
    if (ctx_ptr->mode == CECL_CIPHER_MODE_CTS) {
        ctx_ptr->mode = CECL_CIPHER_MODE_CBC;
        cts_mode = TRUE;
    }

    if (ctx_ptr->mode == CECL_CIPHER_MODE_CCM && !ctx_ptr->payloadLn &&
        ((uint32)(ioVecIn.iov->pvBase) == (uint32)((*ioVecOut).iov->pvBase))) {
        // in-place operation is not supported for CCM star MAC only operation
        return CEML_ERROR_NOT_SUPPORTED;
    }

    CeElMutexEnter();

    // Init temp IoVect pointers
    ioVecIn_tmp = ioVecIn;
    ioVecOut_tmp = *ioVecOut;
    input_len = ioVecIn_tmp.iov->dwLen;
    input_len2 = ioVecIn_tmp.iov->dwLen;
    input_ptr = ioVecIn_tmp.iov->pvBase;
    input_ptr2 = ioVecIn_tmp.iov->pvBase;
    output_ptr = ioVecOut_tmp.iov->pvBase;
    output_ptr2 = ioVecOut_tmp.iov->pvBase;
    output_len = input_len;

    // overflow check for pointer artihmetic
    if ((((uint4)input_ptr + input_len) < (uint4)input_ptr) || (((uint4)output_ptr + output_len) < (uint4)output_ptr)) {
        return CEML_ERROR_INVALID_PARAM;
    }

    // Calculate block size based on input data length
    num_blks = ioVecIn_tmp.iov->dwLen / CEML_MAX_BLOCK_SIZE;
    if (ioVecIn_tmp.iov->dwLen % CEML_MAX_BLOCK_SIZE) {
        ++num_blks;
    }

    if (ctx_ptr->mode == CECL_CIPHER_MODE_CCM) {
        ret = CCMIVInit(ceMlHandle);
        if (CEML_ERROR_SUCCESS != ret) {
            return ret;
        }

        CeElMemScpy(ccmstar_iv, 16, (uint8 *)(ctx_ptr->iv), 16);
        ret = CCMAuthHeaderInit(ceMlHandle, input_ptr);
        if (CEML_ERROR_SUCCESS != ret) {
            return ret;
        }
        // mode redirection for CCM*
        if (ctx_ptr->macLn == 0 && ctx_ptr->payloadLn != 0) {
            ctx_ptr->mode = CECL_CIPHER_MODE_CTR;
            ccm_star_encr_mode = TRUE;
            ctx_ptr->iv[3] += 0x01000000;

            CeElMemScpy((uint8 *)(ioVecOut_tmp.iov->pvBase), ioVecIn_tmp.iov->dwLen - ctx_ptr->payloadLn,
                        (uint8 *)(ioVecIn_tmp.iov->pvBase), ioVecIn_tmp.iov->dwLen - ctx_ptr->payloadLn);
            ioVecIn_tmp.iov->pvBase = (uint8 *)(ioVecIn_tmp.iov->pvBase) + ioVecIn_tmp.iov->dwLen - ctx_ptr->payloadLn;
            input_ptr = ioVecIn_tmp.iov->pvBase;
            ioVecOut_tmp.iov->pvBase =
                (uint8 *)(ioVecOut_tmp.iov->pvBase) + ioVecIn_tmp.iov->dwLen - ctx_ptr->payloadLn;
            output_ptr = ioVecOut_tmp.iov->pvBase;
            ioVecIn_tmp.iov->dwLen = ctx_ptr->payloadLn;
            ioVecOut_tmp.iov->dwLen = ctx_ptr->payloadLn;
        }
        if (ctx_ptr->macLn != 0 && ctx_ptr->payloadLn == 0) {
            ctx_ptr->mode = CECL_CIPHER_MODE_CBC;
            ctx_ptr->dir = CECL_CIPHER_ENCRYPT;
            CeMLCCMStarIV(ceMlHandle, ccmstar_cbcmac_iv);
            CeElMemScpy((uint8 *)(ctx_ptr->iv), 16, (uint8 *)ccmstar_cbcmac_iv, 16);
            ccm_star_auth_mode = TRUE;
            output_len = input_len + 16;
        }
    }

    // Set block flags
    ctx_ptr->firstBlock = 1;
    ctx_ptr->lastBlock = 0;

    // Set up loop to process all input data in blks of max size of 32K bytes
    while (num_blks--) {
        // Calculate current blk size
        curr_blk_size = CEML_MAX_BLOCK_SIZE;
        if (num_blks == 0) {
            // Set last block
            ctx_ptr->lastBlock = 1;

            curr_blk_size = (ioVecIn_tmp.iov->dwLen % CEML_MAX_BLOCK_SIZE);
            if (ctx_ptr->mode == CECL_CIPHER_MODE_CCM && ctx_ptr->dir == CECL_CIPHER_ENCRYPT) {
                // curr_out_blk_size = curr_blk_size + ctx_ptr->macLn;

                // output_len   = input_len + ctx_ptr->macLn;
                curr_out_blk_size = curr_blk_size;
                output_len = input_len;
            } else if (ctx_ptr->mode == CECL_CIPHER_MODE_CCM && ctx_ptr->dir == CECL_CIPHER_DECRYPT) {
                output_len = input_len - ctx_ptr->macLn;
                curr_out_blk_size = curr_blk_size - ctx_ptr->macLn;
            }

            if (curr_blk_size == 0) {
                curr_blk_size = CEML_MAX_BLOCK_SIZE;
            }
        }

        // Set sizes
        ctx_ptr->dataLn = curr_blk_size;
        ioVecIn_tmp.iov->dwLen = curr_blk_size;
        if (ctx_ptr->mode == CECL_CIPHER_MODE_CCM)
            ioVecOut_tmp.iov->dwLen = curr_out_blk_size;
        else
            ioVecOut_tmp.iov->dwLen = curr_blk_size;

        pdwActualOut = 0;
        ret = (CeMLErrorType)CeClIOCtlCipher(CECL_IOCTL_SET_CIPHER_CNTXT, (uint8 *)(ctx_ptr),
                                             sizeof(CeCLCipherCntxType), NULL, 0, &pdwActualOut);
        if (ret != CEML_ERROR_SUCCESS) {
            break;
        }

        ret = CeElCryptoDxeXfer(ioVecIn_tmp.iov->pvBase, ioVecIn_tmp.iov->dwLen, ioVecOut_tmp.iov->pvBase,
                                ioVecOut_tmp.iov->dwLen);

        if (CEML_ERROR_SUCCESS != ret) {
            break;
        }

        pdwActualOut = sizeof(CeCLCipherCntxType);
        ret = (CeMLErrorType)CeClIOCtlCipher(CECL_IOCTL_GET_CIPHER_CNTXT, NULL, 0, (uint8 *)(ctx_ptr), pdwActualOut,
                                             &pdwActualOut);
        if (CEML_ERROR_SUCCESS != ret) {
            break;
        }

        // Increment IoVect Tmp pointers
        input_ptr += curr_blk_size;

        output_ptr += curr_blk_size;
        ioVecIn_tmp.iov->pvBase = input_ptr;
        ioVecOut_tmp.iov->pvBase = output_ptr;

        // Decrement Input size
        input_len2 -= curr_blk_size;
        ioVecIn_tmp.iov->dwLen = input_len2;

        // Clear first block flag
        ctx_ptr->firstBlock = 0;
    } /* while */

    //#if 1
    if (ctx_ptr->mode == CECL_CIPHER_MODE_CCM && ctx_ptr->dir == CECL_CIPHER_ENCRYPT) {
        // if(ctx_ptr->macLn % BLOCK_SZ)
        {
            count = ctx_ptr->macLn / BLOCK_SZ;
            for (index = 0; index < count; index++) {
                // printf("copy tag, index:%d count:%d\n",index, count);
                regVal = HAL_REG_RD(CECL_CE_DATA_OUT + index * 4);
                // printf("0x%08x\n", regVal);
                memcpy(output_ptr + index * 4, &regVal, 4);
                // printf("ioVecOut->iov->pvBase:%p ioVecOut_tmp.iov->pvBase:%p ioVecIn_tmp.iov->dwLen:0x%02x
                // ctx_ptr->payloadLn:0x%02x input_len:0x%02x\n", ioVecOut->iov->pvBase, ioVecOut_tmp.iov->pvBase,
                // ioVecIn_tmp.iov->dwLen, ctx_ptr->payloadLn, input_len);
            }

            if (ctx_ptr->macLn % BLOCK_SZ) {
                regVal = HAL_REG_RD(CECL_CE_DATA_OUT + index * 4);
                // printf("copy tag, index:%d count:%d\n",index, count);
                // printf("0x%08x\n", regVal);
                memcpy(output_ptr + index * 4, &regVal, ctx_ptr->macLn % BLOCK_SZ);
            }
        }
    }
    //#endif
    // Make sure we set in/out lengths/ptr again
    ioVecIn.iov->dwLen = input_len;
    ioVecOut->iov->dwLen = output_len;
    ioVecIn.iov->pvBase = input_ptr2;
    ioVecOut->iov->pvBase = output_ptr2;
    // If CTS mode set set it back in context
    if (cts_mode) {
        ctx_ptr->mode = CECL_CIPHER_MODE_CTS;
    }
    // If CCM mode set set it back in context
    if (ccm_star_auth_mode) {
        CeElMemScpy((uint8 *)((*ioVecOut).iov->pvBase) + (*ioVecOut).iov->dwLen - 16, 16,
                    (uint8 *)((*ioVecOut).iov->pvBase) + (*ioVecOut).iov->dwLen - 32, 16);
        CeMLCCMStarAuth(ceMlHandle, ioVecOut, ccmstar_iv);
        CeElMemset((*ioVecOut).iov->pvBase, 0, (*ioVecOut).iov->dwLen - 16);
    }

    if (ccm_star_auth_mode || ccm_star_encr_mode)
        ctx_ptr->mode = CECL_CIPHER_MODE_CCM;
    CeElMutexExit();
    return ret;
}

/**
 * @brief Internal function to implement AES-CCM-STAR
 *
 * @param
 *
 * @return
 *
 * @see
 *
 */
static CeMLErrorType CeMLCCMStarIV(CeMLCntxHandle *ceMlHandle, uint8 *iv)
{
    CeMLErrorType ret = CEML_ERROR_SUCCESS;
    CeCLCipherCntxType *ctx_ptr = NULL;
    uint32 key_len;
    CeMLCntxHandle *cntx = NULL;
    CeMLCntxHandle ceMlHandle_int;
    CeMLCipherAlgoCntxType ClientCtxt_int;
    CeMLIovecListType ioVecIn;
    CeMLIovecListType ioVecOut;
    CeMLIovecType IovecIn;
    CeMLIovecType IovecOut;
    CeMLCipherAlgType cipher_algo;
    CeMLCipherDir cipher_direction;
    CeMLCipherModeType cipher_mode;
    /* Sanity check inputs */
    if ((!ceMlHandle) || (!ceMlHandle->pClientCtxt)) {
        return CEML_ERROR_INVALID_PARAM;
    }

    ctx_ptr = (CeCLCipherCntxType *)&(((CeMLCipherAlgoCntxType *)((ceMlHandle)->pClientCtxt))->ctx);

    cntx = &ceMlHandle_int;
    cntx->pClientCtxt = &ClientCtxt_int;

    ioVecIn.size = 1;
    ioVecIn.iov = &IovecIn;
    ioVecIn.iov[0].dwLen = 16;
    ioVecIn.iov[0].pvBase = ctx_ptr->nonce;

    /* Output IOVEC */
    ioVecOut.size = 1;
    ioVecOut.iov = &IovecOut;
    ioVecOut.iov[0].dwLen = 16;
    ioVecOut.iov[0].pvBase = iv;

    if (ctx_ptr->algo == CECL_CIPHER_ALG_AES128) {
        cipher_algo = CEML_CIPHER_ALG_AES128;
        key_len = CEML_AES128_KEY_SIZE;
    } else {
        cipher_algo = CEML_CIPHER_ALG_AES256;
        key_len = CEML_AES256_KEY_SIZE;
    }

    do {
        ret = CeMLCipherInit(&cntx, cipher_algo);
        if (CEML_ERROR_SUCCESS != ret) {
            break;
        }

        // Set parameters so we can do the encrypt
        cipher_direction = CEML_CIPHER_ENCRYPT;
        ret = CeMLCipherSetParam(cntx, CEML_CIPHER_PARAM_DIRECTION, &cipher_direction, sizeof(CeMLCipherDir));
        if (CEML_ERROR_SUCCESS != ret) {
            break;
        }

        cipher_mode = CEML_CIPHER_MODE_ECB;

        ret = CeMLCipherSetParam(cntx, CEML_CIPHER_PARAM_MODE, &cipher_mode, sizeof(CeMLCipherModeType));
        if (CEML_ERROR_SUCCESS != ret) {
            break;
        }

        ret = CeMLCipherSetParam(cntx, CEML_CIPHER_PARAM_KEY, (uint8 *)(ctx_ptr->aes_key), key_len);
        if (CEML_ERROR_SUCCESS != ret) {
            break;
        }

        ret = CeMLCCMStarCipherData(cntx, ioVecIn, &ioVecOut);
        if (CEML_ERROR_SUCCESS != ret) {
            break;
        }
    } while (0);

    CeMLCipherDeInit(&cntx);
    return ret;
}

/**
 * @brief Internal function to implement AES-CCM-STAR
 *
 * @param
 *
 * @return
 *
 * @see
 *
 */
static CeMLErrorType CeMLCCMStarCipherData(CeMLCntxHandle *ceMlHandle, CeMLIovecListType ioVecIn,
                                           CeMLIovecListType *ioVecOut)
{
    CeMLErrorType ret = CEML_ERROR_SUCCESS;
    CeCLCipherCntxType *ctx_ptr = NULL;
    uint32 pdwActualOut;
    CeElXferModeType xferMode;
    CeElXferFunctionType xferFunc;
    uint32 curr_blk_size = 0;
    uint32 num_blks;
    CeMLIovecListType ioVecIn_tmp;
    CeMLIovecListType ioVecOut_tmp;
    uint32 input_len;
    uint32 input_len2;
    uint32 output_len;
    uint8 *input_ptr;
    uint8 *input_ptr2;
    uint8 *output_ptr;
    uint8 *output_ptr2;
    /* Sanity check inputs */
    if ((!ceMlHandle) || (!ceMlHandle->pClientCtxt)) {
        return CEML_ERROR_INVALID_PARAM;
    }

    // Create context pointer
    ctx_ptr = (CeCLCipherCntxType *)&(((CeMLCipherAlgoCntxType *)((ceMlHandle)->pClientCtxt))->ctx);

    if (!ioVecOut) {
        return CEML_ERROR_INVALID_PARAM;
    }

    if (((ioVecOut->size != 1) || (!ioVecOut->iov)) || ((ioVecIn.size != 1) || (!ioVecIn.iov))) {
        return CEML_ERROR_INVALID_PARAM;
    }

    if (((ctx_ptr->mode != CECL_CIPHER_MODE_CTS) && (ctx_ptr->mode != CECL_CIPHER_MODE_CTR) &&
         (ctx_ptr->mode != CECL_CIPHER_MODE_CCM)) &&
        (ioVecIn.iov->dwLen % 16 != 0)) {
        return CEML_ERROR_INVALID_PARAM;
    }

    // Init temp IoVect pointers
    ioVecIn_tmp = ioVecIn;
    ioVecOut_tmp = *ioVecOut;
    input_len = ioVecIn_tmp.iov->dwLen;
    input_len2 = ioVecIn_tmp.iov->dwLen;
    input_ptr = ioVecIn_tmp.iov->pvBase;
    input_ptr2 = ioVecIn_tmp.iov->pvBase;
    output_ptr = ioVecOut_tmp.iov->pvBase;
    output_ptr2 = ioVecOut_tmp.iov->pvBase;
    output_len = input_len;

    // overflow check for pointer artihmetic
    if ((((uint4)input_ptr + input_len) < (uint4)input_ptr) || (((uint4)output_ptr + output_len) < (uint4)output_ptr)) {
        return CEML_ERROR_INVALID_PARAM;
    }

    // Calculate block size based on input data length
    num_blks = ioVecIn_tmp.iov->dwLen / CEML_MAX_BLOCK_SIZE;
    if (ioVecIn_tmp.iov->dwLen % CEML_MAX_BLOCK_SIZE) {
        ++num_blks;
    }

    // Set block flags
    ctx_ptr->firstBlock = 1;
    ctx_ptr->lastBlock = 0;

    // Set up loop to process all input data in blks of max size of 32K bytes
    while (num_blks--) {
        // Calculate current blk size
        curr_blk_size = CEML_MAX_BLOCK_SIZE;
        if (num_blks == 0) {
            // Set last block
            ctx_ptr->lastBlock = 1;

            curr_blk_size = (ioVecIn_tmp.iov->dwLen % CEML_MAX_BLOCK_SIZE);

            if (curr_blk_size == 0) {
                curr_blk_size = CEML_MAX_BLOCK_SIZE;
            }
        }

        // Set sizes
        ctx_ptr->dataLn = curr_blk_size;
        ioVecIn_tmp.iov->dwLen = curr_blk_size;
        ioVecOut_tmp.iov->dwLen = curr_blk_size;

        pdwActualOut = 0;
        ret = (CeMLErrorType)CeClIOCtlCipher(CECL_IOCTL_SET_CIPHER_CNTXT, (uint8 *)(ctx_ptr),
                                             sizeof(CeCLCipherCntxType), NULL, 0, &pdwActualOut);
        if (ret != CEML_ERROR_SUCCESS) {
            break;
        }

        ret = CeElCryptoDxeXfer(ioVecIn_tmp.iov->pvBase, ioVecIn_tmp.iov->dwLen, ioVecOut_tmp.iov->pvBase,
                                ioVecOut_tmp.iov->dwLen);

        if (CEML_ERROR_SUCCESS != ret) {
            break;
        }

        pdwActualOut = sizeof(CeCLCipherCntxType);
        ret = (CeMLErrorType)CeClIOCtlCipher(CECL_IOCTL_GET_CIPHER_CNTXT, NULL, 0, (uint8 *)(ctx_ptr), pdwActualOut,
                                             &pdwActualOut);
        if (CEML_ERROR_SUCCESS != ret) {
            break;
        }

        // Increment IoVect Tmp pointers
        input_ptr += curr_blk_size;

        output_ptr += curr_blk_size;
        ioVecIn_tmp.iov->pvBase = input_ptr;
        ioVecOut_tmp.iov->pvBase = output_ptr;

        // Decrement Input size
        input_len2 -= curr_blk_size;
        ioVecIn_tmp.iov->dwLen = input_len2;

        // Clear first block flag
        ctx_ptr->firstBlock = 0;
    } /* while */

    // Make sure we set in/out lengths/ptr again
    ioVecIn.iov->dwLen = input_len;
    ioVecOut->iov->dwLen = output_len;
    ioVecIn.iov->pvBase = input_ptr2;
    ioVecOut->iov->pvBase = output_ptr2;
    return ret;
}

/**
 * @brief Internal function to implement AES-CCM-STAR
 *
 * @param
 *
 * @return
 *
 * @see
 *
 */
static CeMLErrorType CeMLCCMStarAuth(CeMLCntxHandle *ceMlHandle, CeMLIovecListType *ioVecOutOrg, uint8 *iv)
{
    CeMLErrorType ret = CEML_ERROR_SUCCESS;
    CeCLCipherCntxType *ctx_ptr = NULL;
    uint32 pdwActualOut;
    uint32 orgLen;
    uint32 key_len;
    CeElXferModeType xferMode;
    CeElXferFunctionType xferFunc;
    CeMLCntxHandle *cntx = NULL;
    CeMLCntxHandle ceMlHandle_int;
    CeMLCipherAlgoCntxType ClientCtxt_int;
    CeMLIovecListType ioVecIn;
    CeMLIovecListType ioVecOut;
    CeMLIovecType IovecIn;
    CeMLIovecType IovecOut;
    CeCLCipherCntxType *me;
    CeMLCipherAlgType cipher_algo;
    CeMLCipherDir cipher_direction;
    CeMLCipherModeType cipher_mode;

    /* Sanity check inputs */
    if ((!ceMlHandle) || (!ceMlHandle->pClientCtxt)) {
        return CEML_ERROR_INVALID_PARAM;
    }

    if (!ioVecOutOrg) {
        return CEML_ERROR_INVALID_PARAM;
    }
    ctx_ptr = (CeCLCipherCntxType *)&(((CeMLCipherAlgoCntxType *)((ceMlHandle)->pClientCtxt))->ctx);

    cntx = &ceMlHandle_int;
    cntx->pClientCtxt = &ClientCtxt_int;

    me = (CeCLCipherCntxType *)&(((CeMLCipherAlgoCntxType *)((cntx)->pClientCtxt))->ctx);

    orgLen = (*ioVecOutOrg).iov->dwLen;

    ioVecIn.size = 1;
    ioVecIn.iov = &IovecIn;
    ioVecIn.iov[0].dwLen = 16;
    ioVecIn.iov[0].pvBase = (uint8 *)((*ioVecOutOrg).iov->pvBase) + orgLen - 16;

    /* Output IOVEC */
    ioVecOut.size = 1;
    ioVecOut.iov = &IovecOut;
    ioVecOut.iov[0].dwLen = 16;
    ioVecOut.iov[0].pvBase = (uint8 *)((*ioVecOutOrg).iov->pvBase) + orgLen - 16;

    if (ctx_ptr->algo == CECL_CIPHER_ALG_AES128) {
        cipher_algo = CEML_CIPHER_ALG_AES128;
        key_len = CEML_AES128_KEY_SIZE;
    } else {
        cipher_algo = CEML_CIPHER_ALG_AES256;
        key_len = CEML_AES256_KEY_SIZE;
    }

    do {
        ret = CeMLCipherInit(&cntx, cipher_algo);
        if (CEML_ERROR_SUCCESS != ret) {
            break;
        }

        // Set parameters so we can do the encrypt
        cipher_direction = CEML_CIPHER_ENCRYPT;
        ret = CeMLCipherSetParam(cntx, CEML_CIPHER_PARAM_DIRECTION, &cipher_direction, sizeof(CeMLCipherDir));
        if (CEML_ERROR_SUCCESS != ret) {
            break;
        }

        cipher_mode = CEML_CIPHER_MODE_CTR;
        ret = CeMLCipherSetParam(cntx, CEML_CIPHER_PARAM_MODE, &cipher_mode, sizeof(CeMLCipherModeType));
        if (CEML_ERROR_SUCCESS != ret) {
            break;
        }

        ret = CeMLCipherSetParam(cntx, CEML_CIPHER_PARAM_KEY, ctx_ptr->aes_key, key_len);
        if (CEML_ERROR_SUCCESS != ret) {
            break;
        }

        // Check for NULL IV pointer
        if (iv != NULL) {
            ret = CeMLCipherSetParam(cntx, CEML_CIPHER_PARAM_IV, iv, CEML_AES128_IV_SIZE);
            if (CEML_ERROR_SUCCESS != ret) {
                break;
            }
        }

        me->dataLn = CEML_AES_IV_SIZE;
        me->lastBlock = 1;
        pdwActualOut = 0;
        ret = (CeMLErrorType)CeClIOCtlCipher(CECL_IOCTL_SET_CIPHER_CNTXT, (uint8 *)(me), sizeof(CeCLCipherCntxType),
                                             NULL, 0, &pdwActualOut);
        if (ret != CEML_ERROR_SUCCESS) {
            break;
        }

        ret = CeElCryptoDxeXfer(ioVecIn.iov->pvBase, ioVecIn.iov->dwLen, ioVecOut.iov->pvBase, ioVecOut.iov->dwLen);

        if (CEML_ERROR_SUCCESS != ret) {
            break;
        }

        pdwActualOut = sizeof(CeCLCipherCntxType);
        ret = (CeMLErrorType)CeClIOCtlCipher(CECL_IOCTL_GET_CIPHER_CNTXT, NULL, 0, (uint8 *)(me), pdwActualOut,
                                             &pdwActualOut);
        if (CEML_ERROR_SUCCESS != ret) {
            break;
        }

        (*ioVecOutOrg).iov->dwLen = orgLen;
    } while (0);

    CeMLCipherDeInit(&cntx);
    return ret;
}

/**
  * @brief Hardware KDF function does three things: (1) Configure M4 crypto key based on VMID (2) Enable debug based on provided password & derived password from Hardware KDF
  *                                                 (3) Activate/revocate RoT (Root of Trust)
  *                                                 More specifically KDF op code 2~5 and 9~11 are key derivation operations. KDF generates the key and route it to key table in Crypto Core of
  *                                                 M4 according to the VMID of the AHB master.
  *                                                 KDF op code 1, 6, 7 and 8 are password involved operations. Master enter a password and other parameter and then start the operation.
  *                                                 KDF generates a HW password and compare it with the one from master. Debug enable vector, RoT activation and revocation vector is valid only when passwords match.
  *
  * @param[in] cntx             : It is not used right now. Reserved for future extension
  * @param[in] opCode	        : KDF command to run
  * @param[in] user_input	    : Pointer to use input for KDF operation
  * @param[in] user_input_len	: Length (Bytes) of user input. It must be max of 16 bytes
  * @param[in] password	        : Password to match against (for operations that results in password)
  * @param[out]result	        : KDF output
  * @param[in] result_len_in_words : length of result pointer in words.
  *
  *
  * @return CEML_ERROR_SUCCESS if successful,
  *         CEML_ERROR_FAILURE otherwise
  *
  * @dependenies -
  *
  * @sideeffects
  *
  */

CeMLErrorType CeML_hw_kdf(CeMLCntxHandle *ceMlHandle, CeMLKdfOpCode opCode, uint8 *user_input, uint32 user_input_len,
                          uint64 password, uint32 *result, uint32 result_len_in_words)
{
    CeCLCipherCntxType *ctx_ptr = NULL;
    CeMLErrorType ret = CEML_ERROR_SUCCESS;

    /*Sanity check on arguments */
    /* ceMlHandle is intentioanlly left out since it is not used right now. Reserved for future extension. */
    if (!user_input || (user_input_len < 16) || !result || ((result_len_in_words != 4) && (result_len_in_words != 1))) {
        return CEML_ERROR_FAILURE;
    }

    /*currently only support secure storage */
    if (opCode != CECL_KDF_SECURE_STORAGE) {
        return CEML_ERROR_FAILURE;
    }

    CeElMutexEnter();
    ret = (CeMLErrorType)CeElEnableKDFClock();
    if (CEML_ERROR_SUCCESS != ret) {
        CeElMutexExit();
        return ret;
    }

    ret = (CeMLErrorType)CeCL_hw_kdf(ctx_ptr, (CeCLKdfOpCode)opCode, user_input, user_input_len, password, result,
                                     result_len_in_words);

    CeElDisableKDFClock();
    CeElMutexExit();
    return ret;
}

/**
 * @brief This function will init for a Cmac message digest using
 *        the algorithm specified. Once after successful return
 *        ceMlHandle shall not be corrupted as this saves the internal
 *        states between the CeMLCmacUpdate and CeMLCmacFinal functions.
 *
 * @param ceMlHandle    [in]  Pointer to pointer for cmac context
 * @param ioVecKey      [in]  Pointer to key data to Cmac
 * @param palgo         [in]  Algorithm type of either CMAC128 or CMAC256
 *
 * @return CeMLErrorType
 *
 */
CeMLErrorType CeMLCmacInit(CeMLCntxHandle **ceMlHandle, CEMLIovecListType ioVecKey, CeMLHashAlgoType pAlgo)
{
    CeMLErrorType ret_val = CEML_ERROR_SUCCESS;

    /* Sanity check context arguments */
    if ((!ceMlHandle) || (!*ceMlHandle) || (!(*ceMlHandle)->pClientCtxt)) {
        return CEML_ERROR_INVALID_PARAM;
    }

    /* Sanity check key arguments */
    if ((ioVecKey.size != 1) || (!ioVecKey.iov)) {
        return CEML_ERROR_INVALID_PARAM;
    }

    /* Sanity check algorithm type arguments */
    if ((pAlgo != CEML_HASH_ALGO_CMAC128) && (pAlgo != CEML_HASH_ALGO_CMAC256)) {
        return CEML_ERROR_INVALID_PARAM;
    }

    if ((pAlgo == CEML_HASH_ALGO_CMAC128) && (ioVecKey.iov->pvBase) && (ioVecKey.iov->dwLen != 16)) {
        return CEML_ERROR_INVALID_PARAM;
    }

    if ((pAlgo == CEML_HASH_ALGO_CMAC256) && (ioVecKey.iov->pvBase) && (ioVecKey.iov->dwLen != 32)) {
        return CEML_ERROR_INVALID_PARAM;
    }

    /* Initialize the Hash context */
    ret_val = CeMLHashInit(ceMlHandle, pAlgo);
    if (ret_val != CEML_ERROR_SUCCESS) {
        return ret_val;
    }

    /* Set the initial key */
    ret_val = CeMLHashSetParam(*ceMlHandle, CEML_HASH_PARAM_AUTH_KEY, ioVecKey.iov->pvBase, ioVecKey.iov->dwLen, pAlgo);
    if (CEML_ERROR_SUCCESS != ret_val) {
        CeMLHashDeInit(ceMlHandle);
        return ret_val;
    }

    return ret_val;
}

/**
 * @brief Deintialize a cmac context
 *
 * @param ceMlHandle    [in] Pointer to a pointer to the cmac context
 *
 * @return CeMLErrorType
 *
 */
CeMLErrorType CeMLCmacDeInit(CeMLCntxHandle **ceMlHandle)
{
    CeMLErrorType ret_val = CEML_ERROR_SUCCESS;

    /* Sanity check inputs */
    if ((!ceMlHandle) || (!*ceMlHandle) || (!(*ceMlHandle)->pClientCtxt)) {
        return CEML_ERROR_INVALID_PARAM;
    }

    /* DeInitialize the Hash context initialized for CMAC */
    ret_val = CeMLHashDeInit(ceMlHandle);

    return ret_val;
}

/**
 * @brief This function will update a Cmac message digest using
 *        the algorithm specified. It is callable multiple times.
 *
 *        If the input has only last block then this function shall
 *        not be called instead CeMLCmacFinal() shall be called directly.
 *        Alternatively for such blocks better to use the atomic
 *        function CeMLCmac()
 *
 * @param ceMlHandle    [in]  Pointer to cmac context
 * @param ioVecIn       [in]  Pointer to input data to Cmac
 *                            Only accepts the 16byte aligned block size as
 *                            per the algorithm requirement.
 *
 * @see  CeMLCmacFinal() and CeMLCmac()
 *
 * @return CeMLErrorType
 *
 */
CeMLErrorType CeMLCmacUpdate(CeMLCntxHandle *ceMlHandle, CEMLIovecListType ioVecIn)
{
    CEMLIovecListType ioVecOut;
    CEMLIovecType IovecOut;
    uint8 tmp_buf[CEML_CMAC_DIGEST_SIZE];  // to hold the temporary CMAC results

    /* Sanity check inputs */
    if ((!ceMlHandle) || (!ceMlHandle->pClientCtxt)) {
        return CEML_ERROR_INVALID_PARAM;
    }

    /* Sanity check input arguments */
    if ((ioVecIn.size != 1) || (!ioVecIn.iov) || (!ioVecIn.iov->dwLen) || (ioVecIn.iov->dwLen % 16 != 0)) {
        return CEML_ERROR_INVALID_PARAM;
    }

    /* This is only temporary placeholder for unused output in Update function. */
    ioVecOut.size = 1;
    ioVecOut.iov = &IovecOut;
    ioVecOut.iov[0].dwLen = sizeof(tmp_buf);
    ioVecOut.iov[0].pvBase = (void *)tmp_buf;

    // Perform CMAC on input data
    if (CmacUpdate(ceMlHandle, ioVecIn, &ioVecOut, FALSE) != CEML_ERROR_SUCCESS) {
        return CEML_ERROR_FAILURE;
    }

    return CEML_ERROR_SUCCESS;
}

/**
 * @brief This function will finalize a Cmac message digest using
 *        the algorithm specified.
 *
 * @param ceMlHandle    [in]  Pointer to cmac context
 * @param ioVecIn       [in]  Pointer to input data (last chunk) to Cmac
 *                            can handle non 16bytes block size for the last block.
 * @param ioVecOut      [out] Pointer to output data of MAC
 *
 * @return CeMLErrorType
 *
 */
CeMLErrorType CeMLCmacFinal(CeMLCntxHandle *ceMlHandle, CEMLIovecListType ioVecIn, CEMLIovecListType *ioVecOut)
{
    CeMLHashAlgoCntxType *hashCntx = NULL;

    /* Sanity check context arguments */
    if ((!ceMlHandle) || (!ceMlHandle->pClientCtxt)) {
        return CEML_ERROR_INVALID_PARAM;
    }

    /* Sanity check input arguments */
    if ((ioVecIn.size != 1) || (!ioVecIn.iov) || (!ioVecIn.iov->dwLen)) {
        return CEML_ERROR_INVALID_PARAM;
    }

    /* Sanity check output arguments */
    if ((!ioVecOut) || (ioVecOut->size != 1) || (ioVecOut->iov[0].pvBase == NULL)) {
        return CEML_ERROR_INVALID_PARAM;
    }

    /* Check if expected MAC size is between 8 - 16 */
    if (ioVecOut->iov[0].dwLen < CEML_CMAC_DIGEST_SIZE / 2 || ioVecOut->iov[0].dwLen > CEML_CMAC_DIGEST_SIZE) {
        return CEML_ERROR_INVALID_PARAM;
    }

    hashCntx = (CeMLHashAlgoCntxType *)(ceMlHandle->pClientCtxt);

    /* Perform CMAC on input data */
    if (CmacUpdate(ceMlHandle, ioVecIn, ioVecOut, TRUE) != CEML_ERROR_SUCCESS) {
        return CEML_ERROR_FAILURE;
    }

    /* Copy the CMAC digest result back to caller */
    CeElMemScpy(ioVecOut->iov->pvBase, ioVecOut->iov->dwLen, (uint8 *)hashCntx->ctx.auth_iv, ioVecOut->iov->dwLen);

    return CEML_ERROR_SUCCESS;
}

#define BLOCK_SIZE 16
#define IS_32_MEM_RANGE(ptr, len) (((uint64)ptr + (uint64)len) <= 0xFFFFFFFF)

typedef struct
{
  CeCLCipherCntxType    ctx;
}CeMLUtilCipherAlgoCntxType;

CeMLErrorType  CeML_util_encrypt_with_key (uint8 key_type, uint8 *key, uint8 key_len, uint8 *iv_ptr, uint8 iv_len, void *pt_ptr_in, uint32 input_data_len, void  *pt_ptr_out, uint32  *output_data_len_ptr)
{
  uint8*      input_data = (uint8*) pt_ptr_in;
  uint8*      output_data = (uint8*) pt_ptr_out;
  uint32      aes_key[4] = {0x0};
  
  CeMLIovecListType   ioVecIn;
  CeMLIovecListType   ioVecOut;
   
  CeMLIovecType       IovecIn;
  CeMLIovecType       IovecOut; 
  CeMLCntxHandle*   cipher_cntx = NULL;
  CeMLCntxHandle    ceMlHandle;
  CeMLUtilCipherAlgoCntxType ClientCtxt;
  CeMLCipherDir       cipher_direction;  
  CeMLCipherModeType  cipher_mode;  
  CeMLErrorType err_code = CEML_ERROR_SUCCESS;

  /* Only support multiples of the block size for now.*/
  if ((NULL == input_data) || (NULL == output_data) || (NULL == iv_ptr) || (NULL == key) ||
      (NULL == output_data_len_ptr) || (input_data_len % BLOCK_SIZE != 0) || key_len != 16 || iv_len != 16)
    return CEML_ERROR_INVALID_PARAM;

  if ((*output_data_len_ptr < input_data_len + CEML_AES_IV_SIZE) || (!IS_32_MEM_RANGE((uint32)output_data,*output_data_len_ptr))) {
    return CEML_ERROR_INVALID_PARAM;
  }

  cipher_cntx = &ceMlHandle;
  cipher_cntx->pClientCtxt = &ClientCtxt;

  ioVecIn.size = 1;
  ioVecOut.size = 1;
  ioVecIn.iov = &IovecIn;
  ioVecOut.iov = &IovecOut;
  ioVecIn.iov->dwLen = input_data_len;  
  ioVecIn.iov->pvBase = (void*)input_data;
  ioVecOut.iov->dwLen = input_data_len;  
  ioVecOut.iov->pvBase = (void*)output_data ;
  
  do {
    err_code = CeMLInit();
    if (CEML_ERROR_SUCCESS != err_code) {   
      break;
    }

    err_code = CeMLCipherInit(&cipher_cntx, CEML_CIPHER_ALG_AES128);
    if(CEML_ERROR_SUCCESS != err_code) {
       break;
    }

	cipher_mode = CEML_CIPHER_MODE_CBC;
    err_code = CeMLCipherSetParam(cipher_cntx, CEML_CIPHER_PARAM_MODE, &cipher_mode, sizeof(CeMLCipherModeType));
    if (CEML_ERROR_SUCCESS != err_code) {   
      break;
    }

	if(key_type == 0) {
        /*deriving a cipher key from HW key*/
        err_code = CeML_hw_kdf(cipher_cntx, CEML_KDF_SECURE_STORAGE, key, key_len, 0, aes_key, 4);

        if (CEML_ERROR_SUCCESS != err_code) {   
          break;
        }

		err_code = CeMLCipherSetParam(cipher_cntx, CEML_CIPHER_PARAM_KEY, NULL, CEML_AES128_KEY_SIZE);
    	if (CEML_ERROR_SUCCESS != err_code) {   
      		break;
    	}
	}
	else {
		err_code = CeMLCipherSetParam(cipher_cntx, CEML_CIPHER_PARAM_KEY, key, CEML_AES128_KEY_SIZE);
    	if (CEML_ERROR_SUCCESS != err_code) {   
      		break;
    	}
	}

    err_code = CeMLCipherSetParam(cipher_cntx, CEML_CIPHER_PARAM_IV, iv_ptr, BLOCK_SIZE);
    if (CEML_ERROR_SUCCESS != err_code) {   
      break;
    }

    /* Encrypt the data */
    cipher_direction = CEML_CIPHER_ENCRYPT;
    err_code = CeMLCipherSetParam(cipher_cntx, CEML_CIPHER_PARAM_DIRECTION, &cipher_direction, sizeof(CeMLCipherDir));
    if (CEML_ERROR_SUCCESS != err_code) {   
      break;
    }

    err_code = CeMLCipherData(cipher_cntx, ioVecIn, &ioVecOut);  
    if( err_code != CEML_ERROR_SUCCESS ) {
        break;
    }      

    CeElMemScpy((uint8*)((uint8*)output_data+input_data_len), BLOCK_SIZE, (uint8*)iv_ptr, BLOCK_SIZE);
  }while(0);

  if(cipher_cntx != NULL) {
    CeMLCipherDeInit(&cipher_cntx);
  }

  CeMLDeInit(); 
  *output_data_len_ptr = input_data_len + BLOCK_SIZE;

  return err_code;
}

CeMLErrorType CeML_util_decrypt_with_key (uint8 key_type, uint8 *key, uint8 key_len, void *pt_ptr_in, uint32 input_data_len, void  *pt_ptr_out, uint32  *output_data_len_ptr)
{
  uint8*      input_data = (uint8*) pt_ptr_in;
  uint8*      output_data = (uint8*) pt_ptr_out;
  uint8       iv_ptr[CEML_AES_IV_SIZE] = {0x0};
  uint32      aes_key[4]  = {0x0};
  
  CeMLIovecListType   ioVecIn;
  CeMLIovecListType   ioVecOut;
  CeMLIovecType       IovecIn;
  CeMLIovecType       IovecOut;  
  CeMLCntxHandle*   cipher_cntx = NULL;
  CeMLCntxHandle    ceMlHandle;
  CeMLUtilCipherAlgoCntxType ClientCtxt;
  CeMLCipherDir       cipher_direction;  
  CeMLCipherModeType  cipher_mode;  
  CeMLErrorType err_code = CEML_ERROR_SUCCESS;

  /* Only support multiples of the block size for now.*/
  if ((NULL == input_data) || (NULL == output_data) ||
      (NULL == output_data_len_ptr) || (input_data_len % BLOCK_SIZE != 0) || input_data_len <= BLOCK_SIZE || key_len != 16)
    return CEML_ERROR_INVALID_PARAM;

  //output length needs to be equal to input length and checking for wraparound
  if ((*output_data_len_ptr < input_data_len - BLOCK_SIZE) || (!IS_32_MEM_RANGE((uint32)output_data,*output_data_len_ptr))) {
    return CEML_ERROR_INVALID_PARAM;
  }
  
  CeElMemScpy((uint8*)iv_ptr, BLOCK_SIZE, (uint8*)((uint8*)input_data+(input_data_len-BLOCK_SIZE)), BLOCK_SIZE);

  cipher_cntx = &ceMlHandle;
  cipher_cntx->pClientCtxt = &ClientCtxt;

  ioVecIn.size = 1;
  ioVecOut.size = 1;
  ioVecIn.iov = &IovecIn;
  ioVecOut.iov = &IovecOut;
  ioVecIn.iov->dwLen = input_data_len - BLOCK_SIZE;  
  ioVecIn.iov->pvBase = (void*)input_data;
  ioVecOut.iov->dwLen = input_data_len - BLOCK_SIZE; 
  ioVecOut.iov->pvBase = (void*)output_data;
  do {

    err_code = CeMLInit();
    if (CEML_ERROR_SUCCESS != err_code) {   
      break;
    }

    err_code = CeMLCipherInit(&cipher_cntx, CEML_CIPHER_ALG_AES128);
    if(CEML_ERROR_SUCCESS!=err_code) {
       break;
    }

	cipher_mode = CEML_CIPHER_MODE_CBC;
    err_code = CeMLCipherSetParam(cipher_cntx, CEML_CIPHER_PARAM_MODE, &cipher_mode, sizeof(CeMLCipherModeType));
    if (CEML_ERROR_SUCCESS != err_code) {   
      break;
    }


	if(key_type == 0) {
        /*deriving a cipher key from HW key*/
        err_code = CeML_hw_kdf(cipher_cntx, CEML_KDF_SECURE_STORAGE, key, key_len, 0, aes_key, 4);
        if (CEML_ERROR_SUCCESS != err_code) {   
          break;
        }

		err_code = CeMLCipherSetParam(cipher_cntx, CEML_CIPHER_PARAM_KEY, NULL, CEML_AES128_KEY_SIZE);
    	if (CEML_ERROR_SUCCESS != err_code) {   
      		break;
    	}
	}
	else {
		err_code = CeMLCipherSetParam(cipher_cntx, CEML_CIPHER_PARAM_KEY, key, CEML_AES128_KEY_SIZE);
    	if (CEML_ERROR_SUCCESS != err_code) {   
      		break;
    	}
	}

    err_code = CeMLCipherSetParam(cipher_cntx, CEML_CIPHER_PARAM_IV, iv_ptr, BLOCK_SIZE);
    if (CEML_ERROR_SUCCESS != err_code) {   
      break;
    }

      /* Decrypt the data */
    cipher_direction = CEML_CIPHER_DECRYPT;
    err_code = CeMLCipherSetParam(cipher_cntx, CEML_CIPHER_PARAM_DIRECTION, &cipher_direction, sizeof(CeMLCipherDir));

 	if (CEML_ERROR_SUCCESS != err_code) {   
      break;
    }

    err_code = CeMLCipherData(cipher_cntx, ioVecIn, &ioVecOut);  
    if( err_code != CEML_ERROR_SUCCESS ) {
        break;
    }      

  }while(0);

  if(cipher_cntx != NULL) {
    CeMLCipherDeInit(&cipher_cntx);
  }

  CeMLDeInit();

  *output_data_len_ptr = input_data_len - BLOCK_SIZE;

  return err_code;

}