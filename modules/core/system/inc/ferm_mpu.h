/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _FERM_MPU_H_
#define _FERM_MPU_H_

#define MPU_NUM_REGIONS     0x8
#define MPU_MIN_REGION_SIZE 0x20

#define MPU_CTRL_ENABLE         0x1
#define MPU_CTRL_HFNMI_ENABLE   0x2
#define MPU_CTRL_PRIVDEF_ENABLE 0x4

#define MPU_REGION_ENABLE 0x1

#define MPU_TYPE_DREGION_BMSK 0x0000FF00
#define MPU_TYPE_DREGION_SHFT 0x00000008

#define MPU_CTRL_PRIVDEFENA_BMSK 0x00000004
#define MPU_CTRL_HFNMIENA_BMSK   0x00000002
#define MPU_CTRL_ENABLE_BMSK     0x00000001

#define MPU_RNR_REGION_BMSK 0x000000FF
#define MPU_RNR_REGION_SHFT 0x00000000

#define MPU_RBAR_REGION_BMSK 0x0000000F
#define MPU_RBAR_REGION_SHFT 0x00000000
#define MPU_RBAR_VALID_BMSK  0x00000010
#define MPU_RBAR_VALID_SHFT  0x00000004

#define MPU_RASR_ENABLE_BMSK 0x00000001
#define MPU_RASR_ENABLE_SHFT 0x00000000
#define MPU_RASR_SIZE_BMSK   0x0000003E
#define MPU_RASR_SIZE_SHFT   0x00000001
#define MPU_RASR_SRD_BMSK    0x0000FF00
#define MPU_RASR_SRD_SHFT    0x00000008
#define MPU_RASR_AP_BMSK     0x07000000
#define MPU_RASR_AP_SHFT     0x00000018
#define MPU_RASR_XN_BMSK     0x10000000
#define MPU_RASR_XN_SHFT     0x0000001C

typedef struct mpu_region_s {
    uint32_t region_index;
    uint32_t start_addr;
    uint32_t region_size;
    uint8_t xn_bit;
    uint8_t access_perm;
    uint8_t sub_region_mask;
} mpu_region_t;

typedef struct MPU_s {
    uint32_t MPU_TYPE;     // MPU Type Register
    uint32_t MPU_CTRL;     // MPU Control Register
    uint32_t MPU_RNR;      // MPU Region Number Register
    uint32_t MPU_RBAR;     // MPU Region Base Address Register
    uint32_t MPU_RASR;     // MPU Region Attribute and Size Register
    uint32_t MPU_RBAR_A1;  // MPU alias registers
    uint32_t MPU_RASR_A1;  //
    uint32_t MPU_RBAR_A2;  //
    uint32_t MPU_RASR_A2;  //
    uint32_t MPU_RBAR_A3;  //
    uint32_t MPU_RASR_A3;  //
} MPU_t;

#define MPU_DEP (*((volatile MPU_t *)(0xE000ED90)))

void ferm_mpu_config(void);
void ferm_mpu_init(void);
#endif
