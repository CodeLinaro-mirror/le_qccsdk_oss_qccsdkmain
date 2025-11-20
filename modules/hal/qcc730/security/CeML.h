#ifndef CE_ML_H
#define CE_ML_H
/*===========================================================================

                    Crypto Engine Module API

GENERAL DESCRIPTION


EXTERNALIZED FUNCTIONS


INITIALIZATION AND SEQUENCING REQUIREMENTS

  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause-Clear
===========================================================================*/

/*===========================================================================
                      EDIT HISTORY FOR FILE

  $Header: //components/rel/core.ioe/1.0/v2/rom/release/api/security/crypto/CeML.h#7 $

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
    CEML_ERROR_SUCCESS = 0x0,
    CEML_ERROR_FAILURE = 0x1,
    CEML_ERROR_INVALID_PARAM = 0x2,
    CEML_ERROR_NOT_SUPPORTED = 0x3,
    CEML_ERROR_TIMEOUT = 0x4,
    CEML_ERROR_NO_MEMORY = 0x5,
    CEML_ERROR_BAD_ADDRESS = 0x6,
    CEML_ERROR_BAD_DATA = 0x7,
    CEML_ERROR_HW_BUSY = 0x8,
    CEML_ERROR_NOT_ALLOWED = 0x9
    // CEML_ERROR_MAX                    = 0xFFFFFFFF
} CeMLErrorType;

typedef struct {
    void *pvBase;
    uint32 dwLen;
} CeMLIovecType;

typedef struct {
    CeMLIovecType *iov;
    uint32 size;
} CeMLIovecListType;

typedef enum {
    CEML_HASH_ALGO = 0x1,
    CEML_CIPHER_ALGO = 0x2,
    // CEML_ALGO_MAX                     = 0xFFFFFFFF
} CeMLAlgoType;

typedef enum {
    CEML_HASH_ALGO_SHA1 = 0x1,
    CEML_HASH_ALGO_SHA256 = 0x2,
    CEML_HASH_ALGO_CMAC128 = 0x3,
    CEML_HASH_ALGO_CMAC256 = 0x4,
    CEML_HASH_ALGO_SHA224 = 0x5,
    // CEML_HASH_ALGO_MAX                = 0xFFFFFFFF
} CeMLHashAlgoType;

typedef enum {
    CEML_HASH_MODE_HASH = 0x0,
    CEML_HASH_MODE_HMAC = 0x1,
    // CEML_HASH_MODE_MAX                = 0xFFFFFFFF
} CeMLHashModeType;

typedef enum {
    CEML_HASH_PARAM_MODE = 0x01,
    CEML_HASH_PARAM_IV = 0x02,
    CEML_HASH_PARAM_HMAC_KEY = 0x03,
    CEML_HASH_PARAM_AUTH_KEY = 0x04,
    CEML_HASH_NONBLOCK_MODE = 0x05,
    CEML_HASH_NONBLOCK_PTR = 0x06,
    CEML_HASH_IS_NONBLOCK_BUFF_PROCESSED = 0x07,
    // CEML_HASH_PARAM_MAX               = 0xFFFFFFFF
} CeMLHashParamType;

#define CEML_HASH_DIGEST_SIZE_SHA1   20
#define CEML_HASH_DIGEST_SIZE_SHA256 32
#define CEML_HASH_DIGEST_BLOCK_SIZE  64
#define CEML_CCM_DIGEST_SIZE         16
#define CEML_CMAC_DIGEST_SIZE        16

#define CEML_PBL_HASHCTX_SIZE   400
#define CEML_PBL_CIPHERCTX_SIZE 400

typedef enum {
    CEML_CIPHER_MODE_ECB = 0x0,
    CEML_CIPHER_MODE_CBC = 0x1,
    CEML_CIPHER_MODE_CTR = 0x2,
    CEML_CIPHER_MODE_CCM = 0x4,
    CEML_CIPHER_MODE_CTS = 0x5,
    // CEML_CIPHER_MODE_MAX              = 0xFFFFFFFF
} CeMLCipherModeType;

typedef enum {
    CEML_CIPHER_ENCRYPT = 0x00,
    CEML_CIPHER_DECRYPT = 0x01,
    CEML_CIPHER_BYPASS = 0x2
    // CEML_CIPHER_DIR                   = 0xFFFFFFFF
} CeMLCipherDir;

typedef enum {
    CEML_CIPHER_PARAM_DIRECTION = 0x01,
    CEML_CIPHER_PARAM_KEY = 0x02,
    CEML_CIPHER_PARAM_IV = 0x03,
    CEML_CIPHER_PARAM_MODE = 0x04,
    CEML_CIPHER_PARAM_NONCE = 0x05,
    CEML_CIPHER_PARAM_CCM_PAYLOAD_LEN = 0x06,
    CEML_CIPHER_PARAM_CCM_MAC_LEN = 0x07,
    CEML_CIPHER_PARAM_CCM_HDR_LEN = 0x08,
    // CEML_CIPHER_PARAM_MAX             = 0xFFFFFFFF
} CeMLCipherParamType;

#define CEML_AES128_IV_SIZE  16
#define CEML_AES128_KEY_SIZE 16
#define CEML_AES256_IV_SIZE  16
#define CEML_AES256_KEY_SIZE 32
#define CEML_HMAC_KEY_SIZE   64
#define CEML_AUTH_KEY_SIZE   32
#define CEML_AES_BLOCK_SIZE  16
#define CEML_AES_IV_SIZE     16

typedef enum {
    CEML_CIPHER_ALG_AES128 = 0x0,
    CEML_CIPHER_ALG_AES256 = 0x1,
    // CEML_CIPHER_ALG_MAX               = 0xFFFFFFFF
} CeMLCipherAlgType;

typedef struct {
    void *pClientCtxt;
} CeMLCntxHandle;

typedef struct {
    uint8 *pBuff;
    boolean isProcessed;
} CeMLHashNonBlockingBuffStatus;
typedef enum {
    CEML_KDF_QCDEBUG_PASSWORD = 0x1,
    CEML_KDF_ATTESTATION = 0x2,
    CEML_KDF_OTA_SHARD_KEY = 0x3,
    CEML_KDF_SECURE_STORAGE = 0x4,
    CEML_KDF_DEVICE_WRAPPED_KEY = 0x5,
    CEML_KDF_OEM_DEBUG_PASSWORD = 0x6,
    CEML_KDF_ROT_ACTIVATION = 0x7,
    CEML_KDF_ROT_RESERVATION = 0x8,
    CEML_KDF_ENCRYPTION_KEY = 0x9,
    CEML_KDF_PRODUCT_WRAPPED_KEY = 0xa,
    CEML_KDF_QC_ID_TOKEN = 0xb,
    // CEML_KDF_MAX = 0xFFFFFFFF

} CeMLKdfOpCode;

#define CEML_KDF_INPUT_PWD_LEN 16

typedef struct CeMLHwKdfCntx {
    CeMLKdfOpCode op_code;
    uint8 user_input[CEML_KDF_INPUT_PWD_LEN];
    uint32 user_input_size;
    uint64 password;
} CeMLHwKdfCntx;

typedef CeMLIovecListType CEMLIovecListType;
typedef CeMLIovecType CEMLIovecType;

/**********************************************************
 * Initialize a mutex using Qurt. Mutex memory is allocated
 *
 **********************************************************/
#define CRYPTO_MUTEX_INIT() CeEL_mutex_init()

/**
 * @brief This function initializes the CE
 *
 * @param void
 *
 * @return CeMLErrorType
 *
 * @see
 *
 */

CeMLErrorType CeMLInit(void);

/**
 * @brief This function deinitializes the CE
 *
 * @param void
 *
 * @return CeMLErrorType
 *
 * @see
 *
 */

CeMLErrorType CeMLDeInit(void);

/**
 * @brief Intialize a hash context for Hash update and final functions
 *
 * @param ceMlHandle    [in] Pointer to a pointer to the hash context
 * @param Algo          [in] Algorithm type
 *
 * @return CeMLErrorType
 *
 *
 */

CeMLErrorType CeMLHashInit(CeMLCntxHandle **ceMlHandle, CeMLHashAlgoType Algo);

/**
 * @brief Deintialize a hash context
 *
 * @param ceMlHandle      [in] Pointer to pointer to the hash context
 *
 * @return CeMLErrorType
 *
 *
 */

CeMLErrorType CeMLHashDeInit(CeMLCntxHandle **ceMlHandle);

/**
 * @brief This function will hash data into the hash context
 *        structure, which must have been initialized by
 *        CeMLHashInit.
 *
 * @param ceMlHandle  [in] Pointer to Hash context
 * @param ioVecIn     [in] Input message to be
 *                     hashed
 * @return CeMLErrorType
 *
 *
 */

CeMLErrorType CeMLHashUpdate(CeMLCntxHandle *ceMlHandle, CeMLIovecListType ioVecIn);

/**
 * @brief Compute the final digest hash value.
 *
 * @param ceMlHandle [in] Pointer to Hash context
 * @param ioVecOut   [out] Pointer to output digest

 * @return CeMLErrorType
 *
 *
 */

CeMLErrorType CeMLHashFinal(CeMLCntxHandle *ceMlHandle, CeMLIovecListType *ioVecOut);

/**
 * @brief This function will hash data into the hash context
 *        structure and compute the final digest hash value.
 *
 * @param ceMlHandle       [in] Pointer to Hash context
 * @param ioVecIn          [in] Input message to be hashed
 * @param ioVecOut         [Out] Pointer to output digest
 *
 * @return CeMLErrorType
 *
 *
 */

CeMLErrorType CeMLHashAtomic(CeMLCntxHandle *ceMlHandle, CeMLIovecListType ioVecIn, CeMLIovecListType *ioVecOut);

/**
 * @brief This function will create a Hmac message digest using
 *        the algorithm specified.
 *
 * @param key_ptr       [in]  Pointer to key
 * @param keylen        [in]  Length of input key in bytes
 * @param ioVecIn       [in]  Input data to hmac
 * @param ioVecOut      [out] Pointer to output data
 * @param Algo          [in]  Algorithm type
 *
 * @return CeMLErrorType
 *
 *
 */

CeMLErrorType CeMLHmac(uint8 *key_ptr, uint32 keylen, CeMLIovecListType ioVecIn, CeMLIovecListType *ioVecOut,
                       CeMLHashAlgoType Algo);

/**
 * @brief Intialize a hmac context for hmac update and final functions
 *
 * @param ceMlHandle   [in] Pointer to a pointer to the hmac context
 * @param Algo         [in] Algorithm type
 *
 * @return CeMLErrorType
 *
 *
 */
CeMLErrorType CeMLHmacInit(CeMLCntxHandle **ceMlHandle, CeMLHashAlgoType Algo);

/**
 * @brief Deintialize a hmac context
 *
 * @param ceMlHandle      [in] Pointer to a pointer to the hmac context
 *
 * @return CeMLErrorType
 *
 *
 */

CeMLErrorType CeMLHmacDeInit(CeMLCntxHandle **ceMlHandle);

/**
 * @brief This function will hmac data into the hmac context
 *        structure, which must have been initialized by
 *        CeMLHmacInit.
 *
 * @param ceMlHandle  [in] Pointer to Hmac context
 * @param ioVecIn     [in] Input message to be
 *                     hmaced
 * @return CeMLErrorType
 *
 *
 */
CeMLErrorType CeMLHmacUpdate(CeMLCntxHandle *ceMlHandle, CeMLIovecListType ioVecIn);

/**
 * @brief Compute the final digest hmac value.
 *
 * @param ceMlHandle [in] Pointer to Hmac context
 * @param ioVecOut   [out] Pointer to output digest

 * @return CeMLErrorType
 *
 *
 */

CeMLErrorType CeMLHmacFinal(CeMLCntxHandle *ceMlHandle, CeMLIovecListType *ioVecOut);

/**
 * @brief Computes HMAC message digest using the specified algorithm.
 *        It provides atomicity since the digest is calculated at
 *        once for the entire block of message that is provided. The
 *        HMAC computed using this function cannot be updated with
 *        new data since it is an atomic operation. When the update
 *        operation is not needed, this is faster than calling CeMLHmacUpdate
 *        followed by CeMLHmacFinal.
 *
 *  NOTE: The maximum key size supported is 32 bytes
 *
 * @param ioVecIn  [in] Contains pointer to input message to be hashed
 * @param ioVecOut [out] Contains pointer to output digest
 */
CeMLErrorType CeMLHMACAtomic(CeMLCntxHandle *ceMlHandle, CeMLIovecListType *ioVecIn, CeMLIovecListType *ioVecOut);

/**
 * @brief This functions sets the Hash paramaters
 *
 * @param ceMlHandle [in] Pointer to hash context handle
 * @param nParamID   [in] Hash parameter id to set
 * @param pParam     [in] Pointer to parameter data
 * @param cParam     [in] Size of parameter data in bytes
 * @param Algo      [in] Algorithm type
 *
 * @return CeMLErrorType
 *
 */

CeMLErrorType CeMLHashSetParam(CeMLCntxHandle *ceMlHandle, CeMLHashParamType nParamID, const void *pParam,
                               uint32 cParam, CeMLHashAlgoType Algo);

/**
 * @brief This functions get the Hash paramaters
 *
 * @param ceMlHandle [in] Pointer to hash context handle
 * @param nParamID   [in] Hash parameter id to get
 * @param pParam     [in] Pointer to parameter data
 * @param cParam     [in] Size of parameter data in bytes
 *
 * @return CeMLErrorType
 *
 */
CeMLErrorType CeMLHashGetParam(CeMLCntxHandle *ceMlHandle, CeMLHashParamType nParamID, void *pParam, uint32 cParam);

/**
 * @brief Intialize a cipher context
 *
 * @param ceMlHandle       [in] Pointer to a pointer to the cipher
 *                 context structure
 * @param Algo    [in] Cipher algorithm type
 *
 * @return CeMLErrorType
 *
 *
 */

CeMLErrorType CeMLCipherInit(CeMLCntxHandle **ceMlHandle, CeMLCipherAlgType Algo);

/**
 * @brief Deintialize a cipher context
 *
 * @param ceMlHandle       [in] Pointer to a pointer to the cipher
 *                 context structure
 * @return CeMLErrorType
 *
 *
 */

CeMLErrorType CeMLCipherDeInit(CeMLCntxHandle **ceMlHandle);

/**
 * @brief This functions sets the Cipher paramaters used by
 *        CeMLCipherData
 *
 * @param ceMlHandle        [in] Pointer to cipher context handle
 * @param nParamID  [in] Cipher parameter id to set
 * @param pParam    [in] Pointer to parameter data
 * @param cParam    [in] Size of parameter data in bytes
 *
 * @return CeMLErrorType
 *
 *
 */

CeMLErrorType CeMLCipherSetParam(CeMLCntxHandle *ceMlHandle, CeMLCipherParamType nParamID, const void *pParam,
                                 uint32 cParam);

/**
 * @brief This functions gets the Cipher paramaters used by
 *        CeMLCipherData
 *
 * @param ceMlHandle [in] Pointer to cipher context handle
 * @param nParamID   [in]  Cipher parameter id to get
 * @param pParam     [out] Pointer to parameter data
 * @param pcParam    [out] Pointer to size of data
 *
 * @return CeMLErrorType
 *
 *
 */

CeMLErrorType CeMLCipherGetParam(CeMLCntxHandle *ceMlHandle, CeMLCipherParamType nParamID, void *pParam,
                                 uint32 *cParam);

/**
 * @brief This function encrypts/decrypts the passed message
 *        using the specified algorithm.
 *
 * @param ceMlHandle        [in] Pointer to cipher context handle
 * @param ioVecIn   [in] Input data
 *                  length must be a multiple of 16 bytes
 * @param ioVecOut  [out] Pointer to output data
 *
 * @return CeMLErrorType
 *
 *
 */

CeMLErrorType CeMLCipherData(CeMLCntxHandle *ceMlHandle, CeMLIovecListType ioVecIn, CeMLIovecListType *ioVecOut);

/**
 * @brief This function will create a Cmac message digest atomically using
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
                       CeMLHashAlgoType pAlgo);

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
CeMLErrorType CeMLCmacInit(CeMLCntxHandle **ceMlHandle, CEMLIovecListType ioVecKey, CeMLHashAlgoType pAlgo);

/**
 * @brief Deintialize a cmac context
 *
 * @param ceMlHandle    [in] Pointer to a pointer to the cmac context
 *
 * @return CeMLErrorType
 *
 */
CeMLErrorType CeMLCmacDeInit(CeMLCntxHandle **ceMlHandle);

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
CeMLErrorType CeMLCmacUpdate(CeMLCntxHandle *ceMlHandle, CEMLIovecListType ioVecIn);

/**
 * @brief This function will process the last chunk of data and then finalizes
 *        a Cmac message digest using the algorithm specified.
 *
 * @param ceMlHandle    [in]  Pointer to cmac context
 * @param ioVecIn       [in]  Pointer to input data (last chunk) to Cmac
 *                            can handle non 16bytes block size for the last block.
 * @param ioVecOut      [out] Pointer to output data of MAC
 *
 * @return CeMLErrorType
 *
 */
CeMLErrorType CeMLCmacFinal(CeMLCntxHandle *ceMlHandle, CEMLIovecListType ioVecIn, CEMLIovecListType *ioVecOut);

/**
 * @brief This function will update a Cmac message digest using
 *        the algorithm specified with the HW key which is generated
 *        by KDF. It is callable multiple times.
 *
 *        If the input has only last block then this function shall
 *        not be called instead CeMLCmacFinalWithKdfKey() shall be
 *        called directly.
 *        Alternatively for such blocks better to use the atomic
 *        function CeMLCmac()
 *
 * @param ceMlHandle    [in]  Pointer to cmac context
 * @param kdfCtx        [in]  Pointer to KDF context used to generate HW key
 * @param ioVecIn       [in]  Pointer to input data to Cmac
 *                            Only accepts the 16byte aligned block size as
 *                            per the algorithm requirement.
 *
 * @return CeMLErrorType
 *
 */
CeMLErrorType CeMLCmacUpdateWithKdfKey(CeMLCntxHandle *ceMlHandle, CeMLHwKdfCntx *kdfCtx, CEMLIovecListType ioVecIn);

/**
 * @brief This function will finalize a Cmac message digest using
 *        the algorithm specified with the HW key which is generated
 *        by KDF.
 *
 * @param ceMlHandle    [in]  Pointer to cmac context
 * @param kdfCtx        [in]  Pointer to KDF context used to generate HW key
 * @param ioVecIn       [in]  Pointer to input data (last chunk) to Cmac
 *                            can handle non 16bytes block size for the last block.
 * @param ioVecOut      [out] Pointer to output data of MAC
 *
 * @return CeMLErrorType
 *
 */
CeMLErrorType CeMLCmacFinalWithKdfKey(CeMLCntxHandle *ceMlHandle, CeMLHwKdfCntx *kdfCtx, CEMLIovecListType ioVecIn,
                                      CEMLIovecListType *ioVecOut);

/**
 * @brief This function encrypts/decrypts the passed message
 *        using the specified algorithm with the HW key which
 *        is generated by KDF.
 *
 * @param ceMlHandle    [in] Pointer to cipher context handle
 * @param kdfCtx        [in]  Pointer to KDF context used to generate HW key
 * @param ioVecIn       [in] Input data
 *                           length must be a multiple of 16 bytes
 * @param ioVecOut      [out] Pointer to output data
 *
 * @return CeMLErrorType
 *
 *
 */
CeMLErrorType CeMLCipherDataWithKdfKey(CeMLCntxHandle *ceMlHandle, CeMLHwKdfCntx *kdfCtx, CeMLIovecListType ioVecIn,
                                       CeMLIovecListType *ioVecOut);

/**
 * @brief Hardware KDF function does three things: (1) Configure M0/M4 crypto key based on VMID (2) Enable debug based
 * on provided password & derived password from Hardware KDF (3) Activate/revocate RoT (Root of Trust) More specifically
 * KDF op code 2~5 and 9~11 are key derivation operations. KDF generates the key and route it to key table in Crypto
 * Core of M0 or M4 according to the VMID of the AHB master. KDF op code 1, 6, 7 and 8 are password involved operations.
 * Master enter a password and other parameter and then start the operation. KDF generates a HW password and compare it
 * with the one from master. Debug enable vector, RoT activation and revocation vector is valid only when passwords
 * match.
 *
 * @param[in] cntx                : Pointer to a CEML cntx. Needed if key is routed to the Key Table
 * @param[in] opCode	           : KDF command to run
 * @param[in] user_input	       : Pointer to use input for KDF operation
 * @param[in] user_input_len      : Length (Bytes) of user input. It must be max of 16 bytes
 * @param[in] password	           : Password to match against (for operations that results in password)
 * @param[out]result	           : Success/fail for opcode that requires password matching
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
CeMLErrorType CeML_hw_kdf(CeMLCntxHandle *cntx, CeMLKdfOpCode opCode, uint8 *user_input, uint32 user_input_len,
                          uint64 password, uint32 *result, uint32 result_len_in_words);

CeMLErrorType CeMLBISTverify(void);  // stub fuction

/**
  Encrypts the user data with KDF key or SW key.   

  @param[in] key_type				type of key. 0 is for KDF key, other is for SW key
  @param[in] key					Pointer to the KDF key input or SW key.
  @param[in] key_len				Byte length of the KDF key input or SW key. It should be 16	
  @param[in] iv_ptr					Pointer to the IV.
  @param[in] iv_len					Byte length of the IV. It suggest to be 16	
  @param[in] pt_ptr_in				Pointer to the plaintext data.
  @param[in] input_data_len			Byte length of the plaintext data, in multiples of 16 bytes.
  @param[out] pt_ptr_out			Pointer to the ciphertext data. 
  @param[out] output_data_len_ptr	Pointer to hold the byte length of the ciphertext data. 

  @return 
  CeMLErrorType 

  @dependencies
  None.
*/
CeMLErrorType CeML_util_encrypt_with_key (uint8 key_type, uint8 *key, uint8 key_len, uint8 *iv_ptr, uint8 iv_len, void *pt_ptr_in, uint32 input_data_len, void  *pt_ptr_out, uint32  *output_data_len_ptr);

/**
  Decrypts the user data with KDF key or SW key.   

  @param[in] key_type				type of key. 0 is for KDF key, other is for SW key
  @param[in] key					Pointer to the KDF key input or SW key.
  @param[in] key_len				Byte length of the KDF key input or SW key. It should be 16	
  @param[in] pt_ptr_in				Pointer to the ciphertext data.
  @param[in] input_data_len			Byte length of the ciphertext data, in multiples of 16 bytes.
  @param[out] pt_ptr_out			Pointer to the plaintext data. 
  @param[out] output_data_len_ptr	Pointer to hold the byte length of the plaintext data. 

  @return 
  CeMLErrorType 

  @dependencies
  None.
*/
CeMLErrorType CeML_util_decrypt_with_key (uint8 key_type, uint8 *key, uint8 key_len, void *pt_ptr_in, uint32 input_data_len, void  *pt_ptr_out, uint32  *output_data_len_ptr);
#endif  // CE_ML_H
