#ifndef SECBOOT_HW_H
#define SECBOOT_HW_H
/*****************************************************************************
*
* @file  secboot_hw.h (Secboot Hardware API)
*
* @brief API to read Security Control Fuses containing authentication
*        information
* Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*
*****************************************************************************/

/*===========================================================================

                           EDIT HISTORY FOR FILE

This section contains comments describing changes made to this file.
Notice that changes are listed in reverse chronological order.

when       who     what, where, why
--------   ---     ----------------------------------------------------------
10/26/15   SM      Adapted for IOT
08/05/14   hw      Add QPSI code review fix
05/28/14   hw      Add RoT support
05/27/10   sm      Created module

============================================================================*/
/** @ingroup SecureMSM
 *  @{
 */

/** @defgroup SecbootHardware Secboot Hardware Library */

/**
 * @addtogroup SecbootHardware
 *
 * @{
 */
#include "sec_comdef.h"

/**
 Identifies the error type returned by the API.
 */
typedef enum
{
  E_SECBOOT_HW_SUCCESS                = 0x0, /**< Operation was successful. */
  E_SECBOOT_HW_FAILURE                = 0x1, /**< General failure. */
  E_SECBOOT_HW_INVALID_PARAM          = 0x2, /**< Parameter passed in was invalid */
  E_SECBOOT_HW_OUT_OF_RANGE           = 0x3, /**< Index out of bounds */
  E_SECBOOT_HW_FEC_ERROR              = 0x4, /**< FEC error status detected */
  E_SECBOOT_HW_MAX                    = 0x7FFFFFFF /**< Force to 32 bits */
} secboot_hw_etype;

#define FERMOIN_OTP_ENABLE 0

/**
 Identifies the secure boot fuses which represent the
 authentication information for the code.
 */
#define SECBOOT_HW_M4_CODE_SEGMENT   1 /**< Code segment in SECURE_BOOTn register */
                                          /**< representing authentication information. */
#define SECBOOT_HW_KF_CODE_SEGMENT   2 /**< Code segment in SECURE_BOOTn register */
                                          /**< representing authentication information. */
#define SECBOOT_HW_M0_CODE_SEGMENT   3 /**< Code segment in SECURE_BOOTn register */
                                          /**< representing authentication information. */
#define SECBOOT_HW_M4_PATCH_CODE_SEGMENT   4 /**< Code segment in SECURE_BOOTn register */
                                          /**< representing authentication information. */
                                          /**< for the application processors images */
/* Length of a SHA256 hash */
#define SHA256_HASH_LEN 32

/* define the MACRO for DISABLE_ROT_TRANSFER fuse value, which comes from the hardware efuse. */
#define SECBOOT_HW_ROT_TRANSFER_DISABLE 0x1 /**< Bit value 1 - ROT transfer is turned off */
#define SECBOOT_HW_ROT_TRANSFER_ENABLE  0x0 /**< Bit value 0 - ROT transfer is turned on */

#define SECBOOT_CODE_SEG_MIN 1   /* Corresponds to first SECBOOTn supported */
#define SECBOOT_CODE_SEG_MAX 28  /* Corresponds to last SECBOOTn supported */

/* Code Segments for Secure Platform Code without fuses */
#define SECBOOT_SEC_CODE_SEG_MIN 0x40   /* Corresponds to first SECBOOTn supported */
#define SECBOOT_SEC_CODE_SEG_MAX 0x7F  /* Corresponds to last SECBOOTn supported */

/* OEM INDEPENDENT ID is used by default for QC SP, which  */
/* however can be disabled by fuse. When this is used,     */
/* Signing is not tied to OEM_ID. When it is disabled by   */
/* fuse, signing shall be tied to OEM_ID.                  */
#define SECBOOT_HW_OEM_INDEPENDENT_ID   (0x00010000)

/* Number of shifts required to shift a variable by a byte */
#define SECBOOT_HW_BYTE_SHIFT 8

/* define the MACRO for ACTIVATION/REVOCATION list fuse values, which comes from the hardware efuse. */
#define SECBOOT_HW_ROT_ACTIVATION_REVOCATION_DISABLE 0x0 /**< Bit value 0 - index is not active or not revoked */
#define SECBOOT_HW_ROT_ACTIVATION_REVOCATION_ENABLE  0x1 /**< Bit value 1 - index is active or revoked */

/**
 * @brief Secboot Function table
 */
typedef struct secboot_hw_ftbl_type
{
  secboot_hw_etype (*secboot_hw_is_auth_enabled)
                    (uint32    code_segment,
                     uint32*   auth_enabled_ptr);

  secboot_hw_etype (*secboot_hw_get_root_of_trust)
                    (uint32 code_segment,
                     uint8 root_of_trust[SHA256_HASH_LEN]);

  secboot_hw_etype (*secboot_hw_get_msm_hw_id)
                    (uint32        code_segment,
                     const uint8   root_of_trust[SHA256_HASH_LEN],
                     uint64*       msm_hw_id_ptr);

  secboot_hw_etype (*secboot_hw_get_use_serial_num)
                    (uint32         code_segment,
                     uint32*        auth_use_serial_num_ptr);

  secboot_hw_etype (*secboot_hw_get_serial_num)
                    (uint32*  serial_num_ptr);

  secboot_hw_etype (*secboot_hw_get_mrc_fuse_info)
                    (uint32 code_segment,
                     uint32* is_root_cert_enabled,
                     uint32* root_cert_total_num,
                     uint32* revocation_list,
                     uint32* activation_list);

  secboot_hw_etype (*secboot_hw_get_soc_hw_version)
                    (uint32*  soc_hw_version_ptr);
}secboot_hw_ftbl_type;

/**
 * @brief This function checks if the image associated with the code segment
 *        needs to be authenticated. If authentication is required, callers
 *        MUST authenticate the image successfully before allowing it to execute.
 *
 * @param[in]     code_segment       Code segment in SECURE_BOOTn register
 *                                   containing authentication information
 *                                   of the image.
 * @param[in,out] auth_enabled_ptr   Pointer to a uint32 indicating whether
 *                                   authentication is required. Will be
 *                                   populated to 0 if authentication
 *                                   is not required, 1 if authentication
 *                                   is required.
 *
 * @return E_SECBOOT_HW_SUCCESS on success. Appropriate error code on failure.
 *         Caller's must NOT allow execution to continue if a failure is returned.
 *
 * @dependencies None
 *
 * @sideeffects  None
 *
 * @see Security Control HDD and SWI for SECURE_BOOT fuses
 *
 */
secboot_hw_etype secboot_HW_is_auth_enabled
(
  uint32    code_segment,
  uint32*   auth_enabled_ptr
);

/**
 * @brief This function returns the hash of the trusted root certificate for
 *        the image. The image's certificate chain must chain back to this
 *        trusted root certificate. When RoT transfter is turned off, it returns
 *        the default root of trust 0, but when RoT tranfer is turned on, it
 *        returns the root of trust 1.
 *
 *
 * @param[in]      code_segment       Code segment in SECURE_BOOTn register
 *                                    containing authentication information
 *                                    of the image.
 * @param[in,out]  root_of_trust      32 byte buffer which will be
 *                                    populated with the SHA256 hash of
 *                                    the trusted root certificate.
 *
 * @return E_SECBOOT_HW_SUCCESS on success. Appropriate error code on failure.
 *         Caller's must not allow execution to continue if a failure is returned.
 *
 * @dependencies None
 *
 * @sideeffects  None
 *
 * @see Security Control HDD and SWI for SECURE_BOOT fuses
 *
 */
secboot_hw_etype secboot_HW_get_root_of_trust
(
  uint32 code_segment,
  uint8 root_of_trust[SHA256_HASH_LEN]
);

/**
 * @brief This function returns the msm_hw_id used to authenticate the image's
 *        signature. The 64 bit msm_hw_id is comprised of the 32 bit JTAG ID
 *        (with the tapeout version in the upper 4 bits masked out) + the 32 bit
 *        OEM_ID or SERIAL_NUM value
 *
 * @param[in]      code_segment      Code segment in SECURE_BOOTn register
 *                                   containing authentication information
 *                                   of the image.
 *
 * @param[in]      root_of_trust     32 bytes buffer containing the root of
 *                                   trust hash which was populate by
*                                    calling secboot_hw_get_root_of_trust()
 *
 * @param[in,out] msm_hw_id_ptr      Pointer to a uint64 which will
 *                                   be populated with the msm hardware id.
 *                                   The uint64 is allocated by the caller.
 *
 * @return E_SECBOOT_HW_SUCCESS on success. Appropriate error code on failure.
 *         Caller's must not allow execution to continue if a failure is returned
 *
 * @dependencies secboot_hw_get_root_of_trust() must have been called
 *
 * @sideeffects  None
 *
 * @see Security Control HDD and SWI for SECURE_BOOT fuses
 *
 */
secboot_hw_etype secboot_HW_get_msm_hw_id
(
  uint32        code_segment,
  const uint8   root_of_trust[SHA256_HASH_LEN],
  uint64*       msm_hw_id_ptr
);

/**
 * @brief This function returns FALSE as there is not support
 *        for this fuse
 *
 * @param[in]      code_segment             Code segment in SECURE_BOOTn register
 *                                          containing authentication information
 *                                          of the image.
 * @param[in,out]  auth_use_serial_num_ptr  Pointer to FALSE
 *
 * @return E_SECBOOT_HW_SUCCESS on success. Appropriate error code on failure.
 *         Caller's must not allow execution to continue if a failure is returned
 *
 * @dependencies secboot_hw_get_root_of_trust() must have been called
 *
 * @sideeffects  None
 *
 * @see Security Control HDD and SWI for SECURE_BOOT fuses
 *
 */
secboot_hw_etype secboot_HW_get_use_serial_num
(
  uint32         code_segment,
  uint32*        auth_use_serial_num_ptr
);

/**
 * @brief This function returns the serial number of the chip
 *
 * @param[in,out]  serial_num_ptr       Pointer to a uint32 which will
 *                                      be populated with the SERIAL_NUM
 *                                      fuse value. The uint32 is allocated by
 *                                      the caller.
 *
 * @return E_SECBOOT_HW_SUCCESS on success. Appropriate error code on failure.
 *         Caller's must not allow execution to continue if a failure is returned
 *
 * @dependencies None
 *
 * @sideeffects  None
 *
 * @see Security Control HDD and SWI for SECURE_BOOT fuses
 *
 */
secboot_hw_etype secboot_HW_get_serial_num
(
  uint32*  serial_num_ptr
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
secboot_hw_etype secboot_hw_get_ftbl(secboot_hw_ftbl_type* ftbl_ptr);

/**
 * @brief This function retrieves the index of root cert selection,
 * checks if root cert selection is enabled and sets the total number
 * of root certs hashed in OEM_PK_HASH.
 * @param[out] code_segment        Code segment which is
 *                                 currently being used
 * @param[out] is_root_cert_enabled   Pointer to uint32 that
 *                                will be indicate if root cert
 *                                is enabled. uint32 is
 *                                allocated by the caller
 * @param[out] root_cert_total_num Pointer to uint32 that will be
 *                                 populated with total number of root certs.
 *                                 uint32 is allocated by the caller
 *  @param[out] revocation_list    Pointer to uint32 that will
 *                                 hold the revoked indicies
 *                                 indicated by a 1 at the index
 *                                 if revoked
 * @param[out] activation_list     Pointer to uint32 that will
 *                                 hold the revoked indicies
 *                                 indicated by a 1 at the index
 *                                 if active
 * @return E_SECBOOT_HW_SUCCESS on success. Appropriate error code on failure.
 *
 * @see Security Control HDD for SECURE_BOOT fuses
 *
 */
secboot_hw_etype secboot_HW_get_mrc_fuse_info
(
  uint32  code_segment,
  uint32* is_root_cert_enabled_ptr,
  uint32* root_cert_total_num,
  uint32* revocation_list,
  uint32* activation_list
);

/**
 * @brief This function checks to see if the root of trust hash
 *        is blowin in the OEM_PK_HASH fuses or is in the
 *        root_of_trust_pk_hash_table.
 *
 * @param code_segment       [in] Segment of SECURE_BOOTn register holding
 *                                authentication information for the code
 *
 * @param is_hash_in_fuse_ptr [in,out] Pointer to uint32 that will be
 *                                populated with the fuse value. 0 means
 *                                hash is in the Qualcomm root of trust
 *                                table.
 *
 * @return E_SECBOOT_HW_SUCCESS on success. Appropriate error code on failure.
 *
 * @see Security Control HDD for SECURE_BOOT fuses
 *
 */
secboot_hw_etype secboot_HW_get_is_hash_in_fuse(
  uint32 code_segment,
  uint32* is_hash_in_fuse_ptr
);

/**
 * @brief This function returns the soc hw version of the chip
 *
 * @param[in,out]  soc_hw_version_ptr   Pointer to a uint32
 *                                      which will be populated
 *                                      with the SOC_HW_VERSION
 *                                      FAMILY_NUMBER and
 *                                      DEVICE_NUMBER fuse
 *                                      value. The uint32 is
 *                                      allocated by the caller.
 *
 * @return E_SECBOOT_HW_SUCCESS on success. Appropriate error code on failure.
 *         Caller's must not allow execution to continue if a failure is returned
 *
 * @dependencies None
 *
 * @sideeffects  None
 *
 * @see Security Control HDD and SWI for SECURE_BOOT fuses
 *
 */
secboot_hw_etype secboot_HW_get_soc_hw_version
(
  uint32*  soc_hw_version_ptr
);

/// @}
//
/// @}
#endif //SECBOOT_HW_H
