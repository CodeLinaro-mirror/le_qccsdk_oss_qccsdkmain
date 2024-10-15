/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/
#ifndef SECBOOT_H
#define SECBOOT_H

/*****************************************************************************
*
* @file  secboot.h (Secboot Authentication API)
*
* @brief This header files contains the types and API to authenticate an
*        image based on certificates and signature.
*
* @details
*
* @note  Example usage:
*          secboot_handle_type            secboot_handle; //try and put this as a static global, as it is
*                                                         //is 512 bytes
*
*          secboot_image_info_type        image_info;
*          uint32 auth_enabled = 0;
*
*          if (E_SECBOOT_HW_SUCCESS != secboot_hw_is_auth_enabled(SECBOOT_HW_APPS_CODE_SEGMENT, &auth_enabled))
*             return FAILURE
*
*          if (auth_enabled == 1) //Authentication is enabled for this image. Image MUST be authenticated successfully
*          {
*              image_info.header_ptr_1    = header_buf;
*              image_info.header_len_1    = header_len;
*              image_info.code_ptr_1      = code_buf;
*              image_info.code_len_1      = code_len;
*              image_info.x509_chain_ptr  = chain_buf;
*              image_info.x509_chain_len  = chain_len;
*              image_info.signature_ptr   = sig_buf;
*              image_info.signature_len   = sig_len;
*              image_info.sw_type         = TZ_SW_SIGN_TYPE; //Type identifier for TZ images during signing
*              image_info.sw_version      = NumOfOneBitsIn(TZ_ANTI_REPLAY_FUSES);
*
*              if (E_SECBOOT_SUCCESS != secboot_init(&crypto_hash_ftbl (or NULL if PBL), &secboot_handle))
*                  return FAILURE
*
*              if (E_SECBOOT_SUCCESS != secboot_authenticate(&secboot_handle,
*                                                            SECBOOT_HW_APPS_CODE_SEGMENT,
*                                                            &image_info, verified_info))
*              {
*                  secboot_deinit(secboot_handle);
*                  return FAILURE;
*              }
*
*              secboot_deinit(&secboot_handle);
*              return SUCCESS;
*          }
*
*
*****************************************************************************/


/*===========================================================================

                           EDIT HISTORY FOR FILE

This section contains comments describing changes made to this file.
Notice that changes are listed in reverse chronological order.

when       who     what, where, why
--------   ---     ----------------------------------------------------------
10/26/2015 SM       Adapted for IOT
06/09/2015 mm       Add support for SOC_HW_VERSION and more
08/05/2014 hw       Add QPSI code review fix
05/28/2014 hw       Add RoT support
08/28/2013 hw       Added retail unlock feature code
03/06/2013 vg       Added a function to take the fuse values.
08/25/2011 sm       Initial API - based on old api with some changes
============================================================================*/

/** @ingroup SecureMSM
 *  @{
 */

/** @defgroup SecbootAuthentication Secboot Authentication Library */

/**
 * @addtogroup SecbootAuthentication
 *
 * @{
 */


/*===========================================================================

                           INCLUDE FILES

===========================================================================*/

#include "sec_comdef.h"
#include "secboot_hw.h"
#include "seccommon.h"
#include "sec_img_crypto.h"

/*===========================================================================
                      PUBLIC DATA DECLARATIONS
===========================================================================*/
/* Maximum number of cert levels in a cert chain */
#define SECBOOT_MAX_NUM_CERTS    3

/* Minimum number of cert levels in a cert chain*/
#define SECBOOT_MIN_NUM_CERTS    2

/* Maximum number of Root Certs as per IOT requirement */
#define SECBOOT_MAX_ROOT_CERTS   4

/* Total number of certs including max root certs */
#define SECBOOT_TOTAL_MAX_CERTS  (SECBOOT_MAX_ROOT_CERTS + SECBOOT_MAX_NUM_CERTS - 1)

/* Maximum Cert Chain Size */
#define SECBOOT_MAX_CERT_SIZE       (4096)
#define SECBOOT_MAX_CERT_CHAIN_SIZE (SECBOOT_TOTAL_MAX_CERTS * SECBOOT_MAX_CERT_SIZE)

/* secboot handle context byte size */
#define SECBOOT_CONTEXT_HANDLE_SIZE (200)

/**
 * @brief Error codes specific to secboot
 */
typedef enum secboot_error_type
{
  E_SECBOOT_SUCCESS                = 0,   /**< Operation was successful. */
  E_SECBOOT_FAILURE                = 1,   /**< General failure. */
  E_SECBOOT_INVALID_PARAM          = 2,   /**< Invalid parameter passed into function. */
  E_SECBOOT_INVALID_DATA           = 3,   /**< Data is invalid. */
  E_SECBOOT_UNSUPPORTED            = 4,   /**< Option not supported. */
  E_SECBOOT_RSA_FAIL               = 5,   /**< Failure occured for RSA. */
  E_SECBOOT_HASH_FAIL              = 6,   /**< Failure occured for hash. */
  E_SECBOOT_HW_FAIL                = 7,   /**< Failure occured for HW. */
  E_SECBOOT_X509_FAIL              = 8,   /**< Failure occured during cert chain parsing. */
  E_SECBOOT_INVALID_CERT           = 9,   /**< Cert chain validation checks failed. */
  E_SECBOOT_INVALID_CERT_SIG       = 10,  /**< Cert chain signature validation failed. */
  E_SECBOOT_UNTRUSTED_ROOT         = 11,   /**< Root certificate is not the root of trust. */
  E_SECBOOT_INVALID_IMAGE_SIG      = 12,  /**< Invalid image signature. */
  E_SECBOOT_INVALID_SW_TYPE        = 13,  /**< Unexpected software type. */
  E_SECBOOT_INVALID_SW_VERSION     = 14,  /**< Image has been signed with an older version */
  E_SECBOOT_INVALID_MSM_HW_ID      = 15,  /**< Image has been signed for a different hw id */
  E_SECBOOT_INVALID_DEBUG          = 16,  /**< Invalid debug cert */
  E_SECBOOT_INIT                   = 17,  /**< Initialization failed */
  E_SECBOOT_DEINIT                 = 18,  /**< De-initialization failed */
  E_SECBOOT_INVALID_ROOT_SEL       = 19,  /**< Root selection failed, selected root index isn't active/non-revoked */
  E_SECBOOT_INVALID_CRASH_DUMP     = 20,  /**< Invalid crash dump cert */
  E_SECBOOT_INVALID_ROT_VALUE      = 21,  /**< Invalid rot value from the cert */
  E_SECBOOT_INVALID_ROT_SERIALNUM  = 22,  /**< Invalid rot value from the cert */
  E_SECBOOT_INVALID_IN_USE_SOC_HW_VER_OPTION = 23,  /**< Invalid soc hw version option value */
  E_SECBOOT_INVALID_OU_USE_SERIAL_NUMBER_OPTION = 24,  /**< Invalid OU field option from the cert */
  E_SECBOOT_INVALID_SERIAL_NUM     = 25,  /**< Invalid serial number */
  E_SECBOOT_INVALID_OU_OEM_ID_INDP_OPTION = 26,  /**< Invalid OU field option from the cert */
  E_SECBOOT_INVALID_OU_REVOCATION     = 27,  /**< Invalid revocation enablement OU field */
  E_SECBOOT_INVALID_OU_ACTIVATION     = 28,  /**< Invalid activation enablement OU field */
  E_SECBOOT_MAX                    = 0x7FFFFFFF /**< Force to 32 bits */
} secboot_error_type;

/**
 * @brief Information about the image to be authenticated
 */
typedef struct secboot_image_info_type
{
  const uint8* header_ptr_1;   /**< Pointer to the header */
  uint32       header_len_1;   /**< Length in bytes of the image header */
  const uint8* code_ptr_1;     /**< Pointer to the code */
  uint32       code_len_1;     /**< Length in bytes of the image */
  const uint8* x509_chain_ptr; /**< Pointer to the certificate chain */
  uint32       x509_chain_len; /**< Length in bytes of the certificate chain */
  const uint8* signature_ptr;  /**< Pointer to the signature */
  uint32       signature_len;  /**< Length in bytes of the  signature */
  uint32       sw_type;        /**< Type of the image being authenticated - SBL1, TZ etc */
  uint32       sw_version;     /**< Minimum version of the image that can be executed (for rollback prevention) */
} secboot_image_info_type;

/**
 * @brief Hash information of the image
 */
typedef struct secboot_code_hash_info_type
{
  uintnt   code_address;       /**< Address (pointer value) of the code that was hashed */
  uint32   code_length;        /**< the code length */
  uint32   image_hash_length;  /**< hash length - e.g 20 for SHA1, 32 for SHA256 */
  uint8    image_hash[CRYPTO_DIGEST_BYTE_SIZE_SHA256]; /**< hash of HEADER + CODE */
}secboot_image_hash_info_type;

/* store the multiple root cert information */
typedef struct secboot_mrc_fuse_info_type
{
  uint32   is_root_sel_enabled;  /**< Is Root Cert Selection enabled  */
  uint32   num_root_certs;       /**< valid only if root cert selection is enabled */
  uint32   revocation_list; /**< list of revoked root cert indicies */
  uint32   activation_list; /**< list of active root cert indicies */
} secboot_mrc_fuse_info_type;

/**
 * @brief: This structure contains the fuse information used by secboot authentication process.
 * Caller can optionally supply these fuse value by calling secboot_init_fuses() api.
 *
 * Caller has option to supply either all the fuse values or only root_of_trust. Which will
 * be determined by structure parameter "use_root_of_trust_only".
 *  @ param[in]    use_root_of_trust_only    TRUE  - Caller supply only root_of_trust
 *                                           FALSE - Caller supply all the fuses.
 *
 * If caller didn't pass any fuse value (don't call secboot_init_fuses() api), secboot authentication
 * process read all these fuse parameters from h/w.
 *
 * If the caller passes only root_of_trust (caller sets use_root_of_trust_only = TRUE), secboot
 * authentication process read remaining fuse parameters from h/w.
 *
 */

typedef struct secboot_fuse_info_type
{
  uint8        use_supplied_info; /* User Supplied fuse information */
  uint8        root_of_trust[CRYPTO_DIGEST_BYTE_SIZE_SHA256]; /**< sha256 hash of the root certificate */
  uint64       msm_hw_id;
  uint32       auth_use_serial_num;
  uint32       serial_num;
  uint32       use_root_of_trust_only; /**< Caller sets this variable to TRUE if
                                            secboot needs to use only root of trust from the
                                            supplied fuses */
  secboot_mrc_fuse_info_type    mrc_fuse_info; /**< contains mrc specific values */
  uint32       use_soc_hw_version; /**< User Soc HW Version */
  uint32       soc_hw_version;  /**< the fuse value of family number and
                                                       device number of the soc hw version */

} secboot_fuse_info_type;


/**
 * @brief Debug settings which are specified in the Debug OU field of the
 *        attestation certificate to indicate what to do with the one-time
 *        override registers. The Debug OU field is 64 bits, lower 32 bits
 *        denote what debug settings the attestation certifiate has been
 *        signed with and the higer 32 bits are serial num. Bits 0 and 1 of
 *        the debug settings denote if/what needs to be written to the one-time
 *        override registers. Images which need to act on these settings is architecture
 *        specific. When debug is enabled (SECBOOT_DEBUG_ENABLE), the serial number
 *        embedded in the OU field must match the fuse.
 */
#define SECBOOT_DEBUG_NOP     0x0 /**< Bit value 00 - No operation needs to be performed */
#define SECBOOT_DEBUG_DISABLE 0x2 /**< Bit value 10-  Write 0 to one-time override registers */
#define SECBOOT_DEBUG_ENABLE  0x3 /**< Bit value 11 - Write 1 to one-time override registers*/

/**
 * @brief Crash Dump settings which are specified in the Crash Dump OU field of the
 *        attestation certificate to indicate whether the crash report is dumped for
 *        the specific phone. The Crash Dump OU field is 64 bits, lower 32 bits
 *        denote what Crash Dump settings the attestation certifiate has been
 *        signed with and the higer 32 bits are serial num. Bits 0 of the Crash Dump
 *        settings denote the boolean flag. 0 means disabled and 1 means enabled.
 *        When crash dump is enabled (SECBOOT_CRASH_DUMP_ENABLE), the serial number embedded
 *        in the OU field must match the fuse. Absense of the field means disabled.
  */
#define SECBOOT_CRASH_DUMP_DISABLE 0x0 /**< Bit value 0 - crash dump is turned off */
#define SECBOOT_CRASH_DUMP_ENABLE  0x1 /**< Bit value 1 - crash dump is turned on */

/**
 * @brief The return value for rot_disabled in secboot_verified_info. rot_disabled is used to set the rot sticky bit.
 *        The SW_ROT_STICKY_BIT fuse prevents write access to the PK_HASH1 region. This bit is a sticky bit, meaning
 *        when it has a 1 written to it, the 1 cannot be cleared back to 0 until a fuse sense reset is issued. The PBL 
 *        will write a 1 to this register to block write access to the PK_HASH1 region of the QFPROM.
 *            0x0 : ALLOW_WRITE  Allow write
 *            0x1 : DISABLE_WRITE  Disable write
  */
#define SECBOOT_ROT_TRANSFER_ENABLE  0x0 /**< Bit value 0 - rot transfer is enabled */
#define SECBOOT_ROT_TRANSFER_DISABLE 0x1 /**< Bit value 1 - rot transfer is disabled */

/**
 * @brief Revocation settings which are specified in the
 *        Revocation OU field of the attestation certificate to
 *        indicate whether revocation is allowed. The Revocation
 *        OU field is 64 bits, lower 32 bits denote what
 *        Revocation settings the attestation certifiate has
 *        been signed with and the higer 32 bits can be the
 *        serial num. Bit 0 of the Revocation settings denote
 *        the boolean flag. 1 or 2 means enabled and 0 means
 *        disabled. When revocation is enabled
 *        (SECBOOT_REVOCATION_ENABLE_SN), the serial number
 *        embedded in the OU field must match the fuse. When
 *        revocation is enabled (SECBOOT_REVOCATION_ENABLE), the
 *        serial number is not embedded in the OU fields.
 *        Absense of the field means disabled.
  */
#define SECBOOT_REVOCATION_DISABLE 0x0 /**< Bit value 0 - revocation is disabled */
#define SECBOOT_REVOCATION_ENABLE  0x1 /**< Bit value 1 - revocation is enabled */
#define SECBOOT_REVOCATION_ENABLE_SN 0x2 /**< Bit value 2 - revocation is enabled with serial num*/

/**
* @brief Activation settings which are specified in the
 *        Activation OU field of the attestation certificate to
 *        indicate whether activation is allowed. The Activation
 *        OU field is 64 bits, lower 32 bits denote what
 *        Activation settings the attestation certifiate has
 *        been signed with and the higer 32 bits can be the serial num.
 *        Bit 0 of the Activation settings denote the boolean
 *        flag. 1 or 2 means enabled and 0 means disabled. When activation
 *        is enabled (SECBOOT_ACTIVATION_ENABLE_SN), the
 *        serial number embedded in the OU field must match the
 *        fuse. When activation is enabled (SECBOOT_ACTIVATION_ENABLE),
 *        the serial number is not embedded in the OU fields Absense
 *        of the field means disabled.
  */
#define SECBOOT_ACTIVATION_DISABLE 0x0 /**< Bit value 0 - activation is disabled */
#define SECBOOT_ACTIVATION_ENABLE  0x1 /**< Bit value 1 - activation is enabled */
#define SECBOOT_ACTIVATION_ENABLE_SN 0x2 /**< Bit value 2 - activation is enabled with serial num*/

/**
 * @brief The Root Cert Selection feature can be disabled if the image is ROM patching image or
 * there is only one root cert
 *
 */
#define SECBOOT_ROOT_CERT_SEL_ENABLED  1 /**< Root Cert Selection is enabled */
#define SECBOOT_ROOT_CERT_SEL_DISABLED 0 /**< Root Cert Selection  is disabled */

#define SECBOOT_ANTI_ROLLBACK_UNLOCKED  1 /**< anti-rollback region is unlocked */
#define SECBOOT_ANTI_ROLLBACK_LOCKED    0 /**< anti-rollback region is locked */

/**
 * @brief Data returned from a successful authentication.
 */
typedef struct secboot_verified_info_type
{
  uint32                       version_id;   /**< The version id (define the secboot lib version) */
  uint64                       sw_id;        /**< The software id (upper 32 bits:version, lower 32 bits:type)
                                                  the image was signed with */
  uint64                       msm_hw_id;    /**< The constructed MSM HW ID value used to authenticate
                                                  the image */
  uint32                       enable_debug; /**< Value of the debug settings from the attestation cert i.e
                                                  SECBOOT_DEBUG_NOP, SECBOOT_DEBUG_DISABLE, SECBOOT_DEBUG_ENABLE */
  secboot_image_hash_info_type image_hash_info; /** Hash of the header + code */

  uint32                       enable_crash_dump; /**< Value of the crash dump settings from the attestation cert i.e
                                                       SECBOOT_CRASH_DUMP_DISABLE, SECBOOT_CRASH_DUMP_ENABLE */
  uint32                       rot_disabled; /**< Value of the ROT settings, derived from the attestation cert
                                                       0x0 means rot is enabled and 0x1 means rot is disabled */
  uint32                       enable_revocation; /**< Value of the revocation settings from the attestation cert i.e
                                                       SECBOOT_REVOCATION_DISABLE, SECBOOT_REVOCATION_ENABLE,
													   SECBOOT_REVOCATION_ENABLE_SN */
  uint32                       enable_activation; /**< Value of the activation settings from the attestation cert i.e
                                                       SECBOOT_ACTIVATION_DISABLE, SECBOOT_ACTIVATION_ENABLE,
													   SECBOOT_ACTIVATION_ENABLE_SN */
  uint32                       anti_rollback_lock;
  uint32					   anti_rollback_version;

} secboot_verified_info_type;

/**
 * @brief Secboot handle
 */
typedef struct secboot_handle_type
{
    uint32 handle[SECBOOT_CONTEXT_HANDLE_SIZE]; /**< 800 byte buffer needed by secboot for it's operations */
}secboot_handle_type;

/**
 * @brief Secboot Function table
 */
typedef struct secboot_ftbl_type
{
  secboot_error_type (*secboot_init)
                      (crypto_ftbl_type*                   crypto_ftbl_ptr,
                       secboot_hw_ftbl_type *         secboot_hw_ftbl_ptr,
                       secboot_handle_type*                 secboot_handle_ptr);
  secboot_error_type (*secboot_deinit)
                       (secboot_handle_type* secboot_handle_ptr);
  secboot_error_type (*secboot_authenticate)
                      (secboot_handle_type*                secboot_handle_ptr,
                       uint32                              code_segment,
                       const secboot_image_info_type*      image_info_ptr,
                       secboot_verified_info_type*         verified_info_ptr);
  secboot_error_type (*secboot_init_fuses)
                      (secboot_handle_type*                 secboot_handle_ptr,
                       const secboot_hw_ftbl_type *         secboot_hw_ftbl_ptr,
                       secboot_fuse_info_type               fuses );
  secboot_error_type (*secboot_init_hw_info)
                     ( secboot_handle_type* secboot_handle_ptr,
                       const secboot_hw_ftbl_type * secboot_hw_ftbl_ptr,
                       secboot_fuse_info_type * hw_info );
  secboot_error_type (*secboot_hash)
                      (crypto_ftbl_type *                      crypto_ftbl,
                       uint32                                  hash_algo,
                       const uint8*                            data1_in_ptr,
                       uint32                                  data1_len,
                       const uint8*                            data2_in_ptr,
                       uint32                                  data2_len,
                       uint8*                                  digest_ptr,
                       uint32                                  digest_len);
}secboot_ftbl_type;

/**
 * @brief This function initializes the secboot handle and sets up the crypto hw engine
 *
 * @param[in]     crypto_hash_ftbl_ptr  Pointer to the crypto hash driver functions
 *                                      that secboot should use for hashing. If
 *                                      NULL the crypto driver linked into the
 *                                      image that this routine resides in will be
 *                                      used for hashing.
 *
 * @param[in,out] secboot_handle_ptr    Pointer to the secboot handle. The handle
 *                                      must be allocated in protected memory
 *
 * @return E_SECBOOT_HW_SUCCESS on success. Appropriate error code on failure. If
 *         this function returns an error, callers need not call secboot_deinit()
 *         as this will be called from within the error handling routine of this
 *         function
 *
 * @dependencies This function must be called before any other function in this
 *               API is called. Caller must not deallocate the secboot handle
 *               until after secboot_deinit().
 *
 * @sideeffects  Calls the CeMLInit() to initialize the Crypto HW engine.

 *
 * @see secboot_deinit()
 *
 */
secboot_error_type secboot_init
(
  crypto_ftbl_type*  crypto_ftbl_ptr,
  secboot_hw_ftbl_type * secboot_hw_ftbl_ptr,
  secboot_handle_type*                  secboot_handle_ptr
);


/**
 * @brief This function stores the supplied fuses in secboot context.
 *
 * @param[in,out] secboot_handle_ptr    Pointer to the secboot handle returned
 *                                      from secboot_init()
 *
 * @param[in]     fuses                 Contains the fuse related information.
 *
 * @return E_SECBOOT_SUCCESS on success. Appropriate error code on failure.
 *
 * @dependencies secboot_init() needs to have been called..
 *
 * @sideeffects  none.
 *
 * @see
 *
 */
secboot_error_type secboot_init_fuses
(
   secboot_handle_type* secboot_handle_ptr,
   const secboot_hw_ftbl_type * secboot_hw_ftbl_ptr,
   secboot_fuse_info_type fuses
);


/**
 * @brief This function stores the supplied soc_hw_version only in secboot context.
 *
 * @param[in,out] secboot_handle_ptr    Pointer to the secboot handle returned
 *                                      from secboot_init()
 *
 * @param[in]     fuses                 Contains the fuse related information.
 *
 * @return E_SECBOOT_SUCCESS on success. Appropriate error code on failure.
 *
 * @dependencies secboot_init() needs to have been called..
 *
 * @sideeffects  none.
 *
 * @see
 *
 */
secboot_error_type secboot_init_hw_info
(
   secboot_handle_type* secboot_handle_ptr,
   const secboot_hw_ftbl_type * secboot_hw_ftbl_ptr,
   const secboot_fuse_info_type * hw_info
);



/**
 * @brief This function authenticates an image.
 *
 * @param[in,out]     secboot_handle_ptr   Pointer to the secboot handle returned
 *                                         from secboot_init()
 *
 * @param[in]         code_segment         Code segment in SECURE_BOOTn fuse register
 *                                         containing authentication settings for
 *                                         the image.
 *
 * @param[in]         image_info           Information on the image to be authenticated
 *
 * @param[in,out]     verified_info_ptr    Pointer to data that was processed during
 *                                         authentication. Caller can use this data
 *                                         if the function returned success
 *
 * @return E_SECBOOT_HW_SUCCESS on success. Appropriate error code on failure. If a failure,
 * was returned the caller SHOULD NOT allow the image to execute
 *
 * @dependencies secboot_init() needs to have been called.
 *               This function needs 4000 bytes of stack space
 *
 * @sideeffects  none
 *
 *
 *
 *
 *
 * @see secboot_init(), secboot_deinit()
 *
 */
secboot_error_type secboot_authenticate
(
  secboot_handle_type*                secboot_handle_ptr,
  uint32                              code_segment,
  const secboot_image_info_type*      image_info_ptr,
  secboot_verified_info_type*         verified_info_ptr
);

/**
 * @brief This function clears the secboot handle
 *
 * @param[in,out] secboot_handle_ptr   Pointer to the secboot handle returned
 *                                     from secboot_init()
 *
 * @return E_SECBOOT_HW_SUCCESS on success. Appropriate error code on failure.
 *
 * @dependencies None
 *
 * @sideeffects  Calls the CeMLDeInit() to de-initialize the Crypto HW engine
 *
 * @see secboot_init()
 *
 */
secboot_error_type secboot_deinit
(
  secboot_handle_type*      secboot_handle_ptr
);

/**
 * @brief This function return pointers to the secboot functions linked into
 *        the image
 *
 * @param[in,out] ftbl_ptr              Pointer to the function table structure
 *                                      to populate. The pointer must be allocated
 *                                      by the caller.
 *
 * @return E_SECBOOT_HW_SUCCESS on success. Appropriate error code on failure.
 *
 * @sideeffects  None
 *
 *
 */
secboot_error_type secboot_get_ftbl
(
  secboot_ftbl_type* ftbl_ptr
);

/**
 * @brief This function calls the crypto software driver's hash API to
 *        hash the data
 *
 * @param[in]         crypto_hash_ftbl    Pointer to the crypto APIs.
 *
 * @param[in]         hash_algo         Hash algorithm to use.
 *
 * @param[in]         data1_in_ptr      Pointer to the data to hash. Cannot be NULL.
 *                                      as we don't expect boot images/certificates
 *                                      to be loaded at address 0
 *
 * @param[in]         data1_len         Length in bytes of the data pointed to by
 *                                      data1_in_ptr. Cannot be 0 as we don't allow
 *                                      image or certificate size to be 0
 *
 * @param[in]         data2_in_ptr      Pointer to the data to hash. This is an
 *                                      optional field and can be NULL.
 *
 * @param[in]         data2_len         Length in bytes of the data pointed to by
 *                                      data2_in_ptr. 0 if data2_in_ptr is NULL.
 *
 * @param[in,out]     digest_ptr        Pointer to a buffer to store the hash result.
 *
 * @param[in]         digest_len        Length in bytes of buffer pointed to by
 *                                      digest_ptr.
 *
 * @return E_SECBOOT_SUCCESS on success. Appropriate error code on failure.
 *
 * @dependencies Caller should ensure the pointers and lengths passed in are valid
 *
 * @sideeffects  None
 *
 * @see None
 *
 */
secboot_error_type secboot_hash
(
  crypto_ftbl_type *                        crypto_ftbl,
  uint32                            hash_algo,
  const uint8*                              data1_in_ptr,
  uint32                                    data1_len,
  const uint8*                              data2_in_ptr,
  uint32                                    data2_len,
  uint8*                                    digest_ptr,
  uint32                                    digest_len
);

/// @}
//
/// @}
#endif /* SECBOOT_H */

