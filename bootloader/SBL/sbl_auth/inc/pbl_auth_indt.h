/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#ifndef PBL_AUTH_IND_T_H
#define PBL_AUTH_IND_T_H

#include "boot_elf_loader.h"

/******************************************************************************
                                MODULE INCLUDES
                         ADD NEW ONES UNDER THIS LINE
******************************************************************************/


/******************************************************************************
                             PUBLIC MACROS/DEFINES
                         ADD NEW ONES UNDER THIS LINE
******************************************************************************/

/******************************************************************************
                         PUBLIC TYPES and TYPE-DEFINES
                         ADD NEW ONES UNDER THIS LINE
******************************************************************************/


/******************************************************************************
                            PUBLIC DATA DECLARATION
                         ADD NEW ONES UNDER THIS LINE
******************************************************************************/


/******************************************************************************
                         PUBLIC FUNCTION DECLARATIONS
                         ADD NEW ONES UNDER THIS LINE
******************************************************************************/

typedef struct {
    void *pbl_auth;
	void *secboot;
	void *secboot_x509;
	void *secboot_asn1;
	void *secboot_sw;
	void *secboot_hw;
	void *sec_img;
	void *sec_img_auth;
	void *sec_img_auth_env;
	void *sec_img_32bit;
	void* secmath_breduce;
	void* secmath_BIGINT;
	void* secmath_bnhx;
	void* secmath_mod;
	void* sha256;
}pbl_secboot_ind_t, *pbl_secboot_ind_p;

extern pbl_secboot_ind_t pbl_secboot;

typedef struct {
	uint8  *hash_segment_start;
	uint32  hash_segment_size;
    void   *pbl_shared_data;
	void   *pbl_auth_handle;
}pbl_secboot_glb_data_t, *pbl_secboot_glb_data_p;

extern pbl_secboot_glb_data_t pbl_secboot_glb_data;


#endif  /* PBL_AUTH_IND_T_H */
/*=============================================================================
                                  END OF FILE
=============================================================================*/
