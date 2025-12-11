/*===========================================================================

                    Crypto Engine Core API

GENERAL DESCRIPTION


EXTERNALIZED FUNCTIONS


INITIALIZATION AND SEQUENCING REQUIREMENTS

Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
===========================================================================*/

/*===========================================================================
                      EDIT HISTORY FOR FILE


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
    CECL_ERROR_SUCCESS = 0x0,
    CECL_ERROR_FAILURE = 0x1,
    CECL_ERROR_INVALID_PARAM = 0x2,
    CECL_ERROR_NOT_SUPPORTED = 0x3,
    CECL_ERROR_TIMEOUT = 0x4,
    CECL_ERROR_NO_MEMORY = 0x5,
    CECL_ERROR_BAD_ADDRESS = 0x6,
    CECL_ERROR_BAD_DATA = 0x7,
    CECL_ERROR_HW_BUSY = 0x8,
    CECL_ERROR_NOT_ALLOWED = 0x9
} CeCLErrorType;

typedef enum {
    CECL_IOCTL_HASH_VERSION_NUM = 0x1,
    CECL_IOCTL_SET_HASH_CNTXT = 0x2,
    CECL_IOCTL_GET_HASH_CNTXT = 0x3,
    CECL_IOCTL_HASH_XFER = 0x4
} CeCLIoCtlHashType;

typedef enum {
    CECL_HASH_ALGO_SHA1 = 0x1,
    CECL_HASH_ALGO_SHA256 = 0x2,
    CECL_HASH_ALGO_CMAC128 = 0x3,
    CECL_HASH_ALGO_CMAC256 = 0x4,

} CeCLHashAlgoType;

typedef enum { CECL_HASH_MODE_HASH = 0x0, CECL_HASH_MODE_HMAC = 0x1 } CeCLHashModeType;

#define CECL_HASH_SHA_IV_LEN        20
#define CECL_HASH_SHA256_IV_LEN     32
#define CECL_HMAC_MAX_KEY_SIZE      16
#define CECL_AUTH_IV_COUNT          8
#define CECL_HASH_DIGEST_BLOCK_SIZE 64
typedef struct {
    CeCLHashAlgoType algo;
    CeCLHashModeType mode;
    uint32 hmac_key[CECL_HMAC_MAX_KEY_SIZE];
    uint32 auth_key[8];
    uint32 auth_iv[CECL_AUTH_IV_COUNT];
    uint32 auth_bytecnt[2];
    boolean firstBlock;
    boolean lastBlock;
    boolean bAESUseHWKey;
    uint32 dataLn;
    uint32 seg_start;
    uint32 seg_size;
    uint8 auth_nonblock_mode;
    uint8 auth_nonblock_update_flag;
    uint8 auth_no_context;
    uint8 auth_nonblock_pad_flag;
    uint32 opad[CECL_HASH_DIGEST_BLOCK_SIZE / 4];
} CeCLHashAlgoCntxType;

typedef struct {
    uint8 *buff_ptr;
    uint32 buff_len;
} CeCLHashXferType;

#define CECL_AES_MAX_IV_SIZE_BYTES 32
#define CECL_AES_MAX_KEY_SIZE      32
#define CECL_AES128_KEY_SIZE       16
#define CECL_AES_NONCE_SIZE_BYTES  16
#define CECL_AES_CCM_MAC_LEN_16    0xf  // 16-byte MAC output
#define CECL_AES_CMAC_MAC_LEN_16   0xf  // 16-byte MAC output

typedef enum {
    CECL_IOCTL_SET_CIPHER_CNTXT = 0x2,
    CECL_IOCTL_GET_CIPHER_CNTXT = 0x3,
    CECL_IOCTL_CIPHER_XFER = 0x4
} CeCLIoCtlCipherType;

typedef enum {
    CECL_CIPHER_MODE_ECB = 0x0,
    CECL_CIPHER_MODE_CBC = 0x1,
    CECL_CIPHER_MODE_CTR = 0x2,
    CECL_CIPHER_MODE_CCM = 0x4,
    CECL_CIPHER_MODE_CTS = 0x5
} CeCLCipherModeType;

typedef enum { CECL_CIPHER_ENCRYPT = 0x00, CECL_CIPHER_DECRYPT = 0x01, CECL_CIPHER_BYPASS = 0x02 } CeCLCipherDir;

typedef enum { CECL_CIPHER_ALG_AES128 = 0x0, CECL_CIPHER_ALG_AES256 = 0x1 } CeCLCipherAlgType;

typedef struct {
    CeCLCipherAlgType algo;
    CeCLCipherModeType mode;
    CeCLCipherDir dir;
    uint32 aes_key[CECL_AES_MAX_KEY_SIZE / 4];
    uint32 iv[CECL_AES_MAX_IV_SIZE_BYTES / 4];
    uint32 ccm_cntr[CECL_AES_MAX_IV_SIZE_BYTES / 4];
    uint32 nonce[CECL_AES_NONCE_SIZE_BYTES / 4];
    uint32 nonceLn;
    boolean firstBlock;
    boolean lastBlock;
    uint32 dataLn;
    uint32 outdataLn;
    uint32 seg_start;
    uint32 seg_size;
    uint32 payloadLn;
    uint32 macLn;
    uint32 hdrLn;
    uint32 hdr_pad;
    uint32 auth_bytecnt[2];
    uint32 auth_iv[CECL_AUTH_IV_COUNT];
    boolean ccm_flag;
    boolean bAESUseHWKey;
    uint32 kdf_key;
} CeCLCipherCntxType;

typedef struct {
    uint8 *pDataIn;
    uint32 nDataLen;
    uint8 *pDataOut;
    uint32 nDataOutLen;
    uint8 *pCntx;
} CeCLCipherXferType;

typedef enum {
    CECL_XFER_MODE_REG = 0x0,
    CECL_XFER_MODE_DM = 0x1,
} CeCLXferModeType;

typedef enum {
    CECL_KDF_QCDEBUG_PASSWORD = 0x1,
    CECL_KDF_ATTESTATION = 0x2,
    CECL_KDF_OTA_SHARD_KEY = 0x3,
    CECL_KDF_SECURE_STORAGE = 0x4,
    CECL_KDF_DEVICE_WRAPPED_KEY = 0x5,
    CECL_KDF_OEM_DEBUG_PASSWORD = 0x6,
    CECL_KDF_ROT_ACTIVATION = 0x7,
    CECL_KDF_ROT_RESERVATION = 0x8,
    CECL_KDF_ENCRYPTION_KEY = 0x9,
    CECL_KDF_PRODUCT_WRAPPED_KEY = 0xa,
    CECL_KDF_QC_ID_TOKEN = 0xb,
    // CECL_KDF_MAX = 0xFFFFFFFF

} CeCLKdfOpCode;

/*===========================================================================
                      FUNCTION MACROS
===========================================================================*/

/*===========================================================================
                      FUNCTION DECLARATIONS
===========================================================================*/
/**
 * @brief  Initialize chispet layer
 *
 * @param mode [in] Transfer mode type (0:register mode, 1: DM mode)
 *
 * @return CeELErrorType
 *
 * @see
 *
 */
CeCLErrorType CeClInit(CeCLXferModeType mode);

/**
 * @brief  Deinitialize chipset layer
 *
 * @param  void
 *
 * @return CeELErrorType
 *
 * @see
 *
 */
CeCLErrorType CeClDeinit(void);

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
                            uint32 *pdwActualOut);

/**
 * @brief
 *
 *
 * @return None
 *
 * @see
 *
 */
CeCLErrorType CeCLIOCtlGetHashDigest(uint32 *digest_ptr, uint32 digest_len);

/**
 * @brief
 *
 *
 * @return None
 *
 * @see
 *
 */
void CeCLIOCtlCompletion(void);

/**
 * @brief
 *
 *
 * @return None
 *
 * @see
 *
 */
CeCLErrorType CeCLIOCtlSetHashCntx(CeCLHashAlgoCntxType *ctx_ptr);

/**
 * @brief
 *
 *
 * @return None
 *
 * @see
 *
 */
CeCLErrorType CeCLIOCtlGetHashCntx(CeCLHashAlgoCntxType *ctx_ptr);

/**
 * @brief
 *
 *
 * @return None
 *
 * @see
 *
 */
CeCLErrorType CeCLIOCtlHashRegXfer(CeCLHashXferType *pBufOut);

/**
 * @brief
 *
 *
 * @return None
 *
 * @see
 *
 */
CeCLErrorType CeCLIOCtlSetCipherCntx(CeCLCipherCntxType *ctx_ptr);

/**
 * @brief
 *
 *
 * @return None
 *
 * @see
 *
 */
CeCLErrorType CeCLIOCtlGetCipherCntx(CeCLCipherCntxType *ctx_ptr);

/**
 * @brief
 *
 *
 * @return None
 *
 * @see
 *
 */
CeCLErrorType CeCLIOCtlCipherRegXfer(CeCLCipherXferType *pBufOut);

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
                              uint32 dwLenOut, uint32 *pdwActualOut);

/**
 * @brief
 *
 *
 * @return None
 *
 * @see
 *
 */
CeCLErrorType CeClReset(void);

#if 1
/**
 * @brief 
 *        
 *
 * @return None
 *
 * @see 
 *
 */
CeCLErrorType CeCL_hw_kdf 
(
  CeCLCipherCntxType *ctx_ptr, 
  CeCLKdfOpCode opCode, 
  uint8    *user_input, 
  uint32   user_input_len, 
  uint64  password,
  uint32   *result,
  uint32 result_len_in_words  
);
#endif

/**
 * @brief This function requests vote for clocks required for Crypto.
 *
 * @param None
 * @param None
 *
 * @return CeCLErrorType
 *
 */
CeCLErrorType CeClClockEnable(void);

/**
 * @brief This function cancels vote for clocks required for Crypto.
 *
 * @param None
 * @param None
 *
 * @return CeCLErrorType
 *
 *
 */
CeCLErrorType CeClClockDisable(void);

/**
 * @brief This function requests vote for clocks required for KDF.
 *
 * @param None
 * @param None
 *
 * @return CeCLErrorType
 *
 * @see CeClClockEnable
 *
 */
CeCLErrorType CeClKDFClockEnable(void);

/**
 * @brief This function cancels vote for clocks required for KDF.
 *
 * @param None
 * @param None
 *
 * @return CeCLErrorType
 *
 * @see CeClClockDisable
 *
 */
CeCLErrorType CeClKDFClockDisable(void);
