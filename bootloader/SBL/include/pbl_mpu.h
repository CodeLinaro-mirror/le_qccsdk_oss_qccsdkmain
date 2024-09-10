/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef __PBL_MPU_H__
#define __PBL_MPU_H__

#include "boot_error_if.h"

bl_error_type pbl_config_mpu();
bl_error_type pbl_disable_mpu();

typedef bl_error_type (*pbl_config_mpu_t)( void );
typedef bl_error_type (*pbl_disable_mpu_t)( void );


typedef struct {
	pbl_config_mpu_t pbl_config_mpu_pfn;
	pbl_disable_mpu_t pbl_disable_mpu_pfn;
}pbl_mpu_ind_t;

#endif
