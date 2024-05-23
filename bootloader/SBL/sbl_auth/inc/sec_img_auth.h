#ifndef SEC_IMG_AUTH_H
#define SEC_IMG_AUTH_H

/**
@file sec_img_auth.h
@brief Secure Image Authentication 
*/

/*===========================================================================
   Copyright (c) 2012-2023 by QUALCOMM Technologies, Incorporated.  All Rights Reserved.
===========================================================================*/

/*===========================================================================

                            EDIT HISTORY FOR FILE

when       who      what, where, why
--------   ---      ------------------------------------
10/26/15   SM       Adapted for IOT
15/08/14   mm       Adapted for 64 bit 
10/26/13   mm       Adapted for Boot ROM 
02/20/12   vg       Ported from TZ PIL.

===========================================================================*/

/*----------------------------------------------------------------------------
 * Include Files
 * -------------------------------------------------------------------------*/
#include "elf_header.h"
#include "miprogressive.h"
#include "secboot.h"
#include "seccommon.h"
#include "uie.h"
/*----------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * -------------------------------------------------------------------------*/

#define SEC_IMG_AUTH_ERROR_TYPE_UIE_BASE  (0x150)

#define ELFINFO_CLASS_INDEX   4     /* File class byte index            */

/*use the struct defined in elf_header.h*/
#define Elf32_Ehdr boot_elf32_ehdr 
#define Elf32_Phdr boot_elf32_phdr

/* ELF Object Type */
typedef enum
{
  ELF_CLASS_32 = 1,    /* 32-bit objects                   */
  ELF_CLASS_64 = 2,     /* 64-bit objects                   */
  ELF_CLASS_TYPE_FORCE32BITS = 0x7FFFFFFF /* force to 32 bits*/
}Elf_Class;

#define ELFINFO_MAG0_INDEX    0     /* File identification byte 0 index */
#define ELFINFO_MAG0          0x7f  /* Magic number byte 0              */

#define ELFINFO_MAG1_INDEX    1     /* File identification byte 1 index */
#define ELFINFO_MAG1         'E'    /* Magic number byte 1              */

#define ELFINFO_MAG2_INDEX    2     /* File identification byte 2 index */
#define ELFINFO_MAG2         'L'    /* Magic number byte 2              */

#define ELFINFO_MAG3_INDEX    3     /* File identification byte 3 index */
#define ELFINFO_MAG3         'F'    /* Magic number byte 3              */

#define ELFINFO_CLASS_INDEX   4     /* File class byte index            */

/* Version information */
#define ELFINFO_VERSION_INDEX 6    /* File version byte index          */
#define ELF_VERSION_CURRENT   1    /* Current version                  */

typedef struct sec_img_auth_elf_info_type
{
  void *elf_hdr;       /**< Pointer to the elf header */
  void *prog_hdr;      /**< Pointer to the start of the program header; */
  uint8*      hash_seg_hdr;   /**< Pointer to the hash segment header*/
} sec_img_auth_elf_info_type;
/* State of Secure Image Authentication */
typedef enum
{
  SEC_IMG_AUTH_STATE_INIT = 1,
  SEC_IMG_AUTH_STATE_ELF_AND_PROG_HDRS_HASH_MATCH,
  SEC_IMG_AUTH_STATE_COMPLETE_SEGMENTS_HASH_MATCH
}sec_img_auth_state_et;

/* Structure the state of the secure image auth 
  * Holds verified information in case of success
  * Holds error information in case of error 
  */
typedef struct sec_img_auth_verified_info_t
{
  uint32 version_id; /* the version of this structure. */
  uint32 ph_segment; /* Segment that caused the hash failure. */
  sec_img_auth_state_et state; /* State of the Secure Image Authentication. */
  secboot_verified_info_type v_info; /* Secure Boot verified information. */
}sec_img_auth_verified_info_s;

typedef struct sec_img_elf_prog_hdr_32
{
  Elf32_Ehdr  *elf_hdr;       /**< Pointer to the elf header */
  Elf32_Phdr  *prog_hdr;      /**< Pointer to the start of the program header; */
}sec_img_elf_prog_hdr_32_type;

#if 0
typedef struct sec_img_elf_prog_hdr_64
{
  Elf64_Ehdr  *elf_hdr;       /**< Pointer to the elf header */
  Elf64_Phdr  *prog_hdr;      /**< Pointer to the start of the program header; */
}sec_img_elf_prog_hdr_64_type;
#endif

typedef union sec_img_elf_prog_hdr 
{
  sec_img_elf_prog_hdr_32_type  hdr_32;       /**< Pointer to the elf header */
  //sec_img_elf_prog_hdr_64_type  hdr_64;       /**< Pointer to the elf header */
} sec_img_elf_prog_hdr_type;

typedef struct sec_img_auth_elf_info_s
{
  Elf_Class elf_format;
  sec_img_elf_prog_hdr_type  elf_prg_hdr;  /**< Pointer to the elf header */
  uint32        prog_hdr_num;   /**< Number of program header entries */
  uint8*        hash_seg_hdr;   /**< Pointer to the hash segment header*/
  uint32        hash_seg_hdr_sz; /**< Hash segment header size in bytes */
  uint8*        hash_seg;       /**< Pointer to the hash segment */
  uint32        hash_array_sz;    /**< Hash Array size in bytes */
  uint8*        sig_ptr;        /**< Pointer to the signature */
  uint32        sig_sz;         /**< Signature Size */
  uint8*        cert_ptr;       /**< Pointer to the certificate */
  uint32        cert_sz;        /**< Size of the certificate */
  uint8*        uie_hdr_ptr;    /**< Pointer to the UIE header */
} sec_img_auth_elf_info_t;

/**
 * @brief Information about the image to be authenticated
 */
typedef struct sec_img_info_type
{
  uint32       initialized;    /**< set only by user, if initialized, will use info provided by user */
  uint32       sw_version;     /**< Minimum version of the image */
  uint32       code_segment;   /* the code segment for ths image . */
  uint32       auth_enabled;   /* the auth_en fuse value. */
} sec_img_info_type;

typedef struct sec_img_fuse_info_type 
{
  uint32       initialized;    /**< If initialized will use from this info structure */
  secboot_fuse_info_type fuse_info; /**< Fuse information for image */
}sec_img_fuse_info_type;

typedef struct sec_img_auth_priv_s
{
  sec_img_auth_id_type img_id;                         /* Image Id */
  //sec_img_auth_whitelist_area_param_t *whitelist_area; /* Whitelist */
  sec_img_auth_elf_info_type *img_data;                /* ELF information */
  sec_img_info_type auth_info;                      /* Image information */
  sec_img_fuse_info_type fuse_info;                    /* Fuse Info */
  uie_ctx *uie_img_ctx;                                /* UIE Img Context */ 
  crypto_ftbl_type * crypto_ftbl;                      /* Crypto Table */
  secboot_hw_ftbl_type * ftbl_ptr;                     /* Secboot HW Table */
  /* Internal Parameters */
  sec_img_auth_elf_info_t elf;                         /* Elf Info */
  sec_img_auth_state_et state;                         /* State */
} sec_img_auth_priv_t;


/*----------------------------------------------------------------------------
 * Function Declarations and Documentation
 * -------------------------------------------------------------------------*/

/**
 * @brief Ensures all HW is initialized at this point. if hash crypto functions 
 *        are missing, crypto_ftbl will be populated with pbl internal crypto 
 *        function pointers (for BOTH hash and image decrypt).
 * @param[in/out] crypto_ftbl Pointer to the cryptographic functions
 * @return 0 on success,  else failure.
 */
sec_img_auth_error_type sec_img_auth_init(sec_img_auth_priv_t* s_img_handle);

/**
 * @brief Authenticates hash table segment against its signature.
 *
 * @param[in] s_img_handle Pointer to the image verification context
 * @param[out] v_info Pointer to the verified info 
 *
 * @return 0 on success,  else failure.
 */
sec_img_auth_error_type sec_img_auth_verify_metadata (
  sec_img_auth_priv_t *s_img_handle,
  sec_img_auth_verified_info_s *v_info
);

/**
 * @brief Validates ELF segments against hash table 
 *
 * @param[in] s_img_handle Pointer to the image verification context
 * @param[out] v_info Pointer to the verified info 
 * @return 0 on success,  else failure
 */
sec_img_auth_error_type sec_img_auth_hash_elf_segments(
  sec_img_auth_priv_t *s_img_handle,
  sec_img_auth_verified_info_s *v_info);

/**
 * @brief Elf header parser, to check for the correct format.
 *
 * @param[in] elf_hdr   Pointer to the ELF header.
 *
 * @result \c TRUE if the header is in correct format, \c FALSE otherwise.
 */

sec_img_auth_error_type sec_img_auth_validate_elf(const void* elf_hdr);

/**
 * @brief Check for loadable, non paged segments
 *
 * @param[in] ELF object class format 
 * @param[in] Program hdr entry for the segment
 *
 * @result \c TRUE if the segment is valid, \c FALSE otherwise.
 */
uint32 sec_img_auth_is_valid_segment(Elf_Class format, const void * entry);

/**
 * @brief Elf header parser, to check for the correct format.
 *
 * @param[in] elf_hdr   Pointer to the ELF header.
 *
 * @result \c TRUE if the header is in correct format, \c FALSE otherwise.
 */

sec_img_auth_error_type sec_img_auth_get_elf_format( const  void * elf_hdr,
                                                     Elf_Class *elf_format);

/**
 * @brief SecImgAuth Function table
 */
typedef struct sec_img_auth_ftbl_type
{
  uint32 version_id;  /*define ftbl version */
  sec_img_auth_error_type (*secimg_auth_init)
                       (sec_img_auth_priv_t* s_img_handle);
  sec_img_auth_error_type (*secimg_auth_verify_metadata)
                       (sec_img_auth_priv_t *s_img_handle,
                        sec_img_auth_verified_info_s *v_info);
  sec_img_auth_error_type (*secimg_auth_hash_elf_segments)
                       (sec_img_auth_priv_t *s_img_handle,
                        sec_img_auth_verified_info_s *v_info);
  sec_img_auth_error_type (*secimg_auth_validate_elf)
                       (const void* elf_hdr);
  uint32 (*secimg_auth_is_valid_segment)
                       (Elf_Class format, const void * entry);
  sec_img_auth_error_type (*secimg_auth_get_elf_format)
                       (const  void * elf_hdr,
                        Elf_Class *elf_format);
}sec_img_auth_ftbl_type;


/**
 * @brief This function return pointers to the secimgauth functions linked into
 *        the image
 *
 * @param[in,out] ftbl_ptr              Pointer to the function table structure
 *                                      to populate. The pointer must be allocated
 *                                      by the caller.
 *
 * @return SEC_IMG_AUTH_SUCCESS on success. Appropriate error code on failure.
 *
 * @sideeffects  None
 *
 *
 */
sec_img_auth_error_type sec_img_auth_get_ftbl
(
  sec_img_auth_ftbl_type* ftbl_ptr
);

#endif
