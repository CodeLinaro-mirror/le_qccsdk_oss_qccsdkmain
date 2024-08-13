/*
 *Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sbl_common.h"
#include "sbl_mpu.h"

#define ALIGN_ADDRESS(addr, size)        ((addr) & (~(size -1)))
#define ADDRESS_ALIGNED(addr, size)      ((addr) & ((size -1)))
#define ALIGNED_SIZE(size)               ((size) & (size - 1))
#define RASR_SIZE(size)                  (((0x20) - (__clz(size))) - 1 - 1)

#define MPU_RBAR_MASK(size)              (~((1<< (size)) - 1))

/* region_index		start_addr		size	xn_bit   access_perm	sub_region_mask; */
mpu_region_t SBL_MPU_Region[8] = 
{
/*
	{0x00, 		0x0,		0x100000,	0x01,		0x03,		0xe0},
	{0x01, 		0x80000, 	0x8000,		0x00,		0x06,		0x00},
	{0x02, 		0x200000,	0x200000,	0x01,		0x03,		0xc0},
	{0x03, 		0x200000,	0x8000,		0x00,		0x06,		0x00},
	{0x04, 		0x208000,	0x2000,		0x00,		0x06,		0x01},
	{0x05, 		0x0,		0x0,		0x00,		0x00,		0x00},
	{0x06, 		0x0,		0x0,		0x00,		0x00,		0x00},
	{0x07, 		0x0,		0x0,		0x00,		0x00,		0x00},
*/
	{0x00,	CONFIG_SBL_MPU_0_START, CONFIG_SBL_MPU_0_SIZE, CONFIG_SBL_MPU_0_XN, CONFIG_SBL_MPU_0_ACCESS, CONFIG_SBL_MPU_0_SUBREGION},
	{0x01,	CONFIG_SBL_MPU_1_START, CONFIG_SBL_MPU_1_SIZE, CONFIG_SBL_MPU_1_XN, CONFIG_SBL_MPU_1_ACCESS, CONFIG_SBL_MPU_1_SUBREGION},
	{0x02,	CONFIG_SBL_MPU_2_START, CONFIG_SBL_MPU_2_SIZE, CONFIG_SBL_MPU_2_XN, CONFIG_SBL_MPU_2_ACCESS, CONFIG_SBL_MPU_2_SUBREGION},
	{0x03,	CONFIG_SBL_MPU_3_START, CONFIG_SBL_MPU_3_SIZE, CONFIG_SBL_MPU_3_XN, CONFIG_SBL_MPU_3_ACCESS, CONFIG_SBL_MPU_3_SUBREGION},
	{0x04,	CONFIG_SBL_MPU_4_START, CONFIG_SBL_MPU_4_SIZE, CONFIG_SBL_MPU_4_XN, CONFIG_SBL_MPU_4_ACCESS, CONFIG_SBL_MPU_4_SUBREGION},
	{0x05,	CONFIG_SBL_MPU_5_START, CONFIG_SBL_MPU_5_SIZE, CONFIG_SBL_MPU_5_XN, CONFIG_SBL_MPU_5_ACCESS, CONFIG_SBL_MPU_5_SUBREGION},
	{0x06,	CONFIG_SBL_MPU_6_START, CONFIG_SBL_MPU_6_SIZE, CONFIG_SBL_MPU_6_XN, CONFIG_SBL_MPU_6_ACCESS, CONFIG_SBL_MPU_6_SUBREGION},
	{0x07,	CONFIG_SBL_MPU_7_START, CONFIG_SBL_MPU_7_SIZE, CONFIG_SBL_MPU_7_XN, CONFIG_SBL_MPU_7_ACCESS, CONFIG_SBL_MPU_7_SUBREGION},
};

static uint8_t sbl_encode_rasr_size(uint32_t size)
{
	uint8_t result = 0;

	/* Check if region_size is valid
	 * region_size should be no less than 32 bytes, and be power of 2
	 */
	if ( (size < MPU_MIN_REGION_SIZE) || ((ALIGNED_SIZE(size)) != 0) )
		return 0;

	/* Encode region_size so that 2^(result+1) == size */
	while(size)
	{
		size /= 2;
		result++;
	}
	return (result - 2);
}

static inline void sbl_memory_barrier(void)
{
	__asm volatile("dsb \n");
	__asm volatile("isb \n");
}

void sbl_mpu_config(void)
{
	uint32_t region_start_addr;
	uint8_t rasr_size = 0;

	/* Make sure MPU_TYPE register DREGION reads 8 to support 8 regions*/
	if (MPU_NUM_REGIONS != ((MPU_DEP.MPU_TYPE & MPU_TYPE_DREGION_BMSK) >> MPU_TYPE_DREGION_SHFT))
	{
		sbl_printf("NUM regions for Cortex M4 must be 8\n");
		return;
	}

	/* Disable MPU set MPU_CTRL bit 0 to 0x0 */
	MPU_DEP.MPU_CTRL = 0x0;

	/* For each of the MPU region*/
	for (uint32_t i = 0; i < MPU_NUM_REGIONS; ++i)
	{
		if (SBL_MPU_Region[i].region_size == 0)
		{
			continue;
		}

		/* Select the region number in RNR */
		MPU_DEP.MPU_RNR =  ((SBL_MPU_Region[i].region_index << MPU_RNR_REGION_SHFT) & MPU_RNR_REGION_BMSK); 

		/* Encode the region_size */
		/* We need convert region_size to SIZE where 2^(SIZE+1) equals to region_size */
		rasr_size = sbl_encode_rasr_size(SBL_MPU_Region[i].region_size);
		if(!rasr_size)
		{
			sbl_printf("Region Size must be at least 32 bytes, and power of 2\n");
			return;
		}

		/* Check if start_address is aligned with region_size */
		if ((ADDRESS_ALIGNED(SBL_MPU_Region[i].start_addr, SBL_MPU_Region[i].region_size)) != 0)
		{
			sbl_printf("Start Address should be aligned with Region Size\n");
			return;
		}
		/* We only need to write the MSB bits of the address after alignment. */
		region_start_addr = SBL_MPU_Region[i].start_addr >> rasr_size; 
      
		/* RNR is already populated with the region number, so set the region field in RBAR to be invalid */
		/* Set RBAR and RASR */
		MPU_DEP.MPU_RBAR =  ( (0x0 << MPU_RBAR_VALID_SHFT) | 
                               ((region_start_addr << rasr_size) & MPU_RBAR_MASK(rasr_size) ));

		MPU_DEP.MPU_RASR =  ( ((MPU_REGION_ENABLE << MPU_RASR_ENABLE_SHFT) & MPU_RASR_ENABLE_BMSK) | 
                               (( rasr_size << MPU_RASR_SIZE_SHFT) & MPU_RASR_SIZE_BMSK) |
                               ((SBL_MPU_Region[i].sub_region_mask << MPU_RASR_SRD_SHFT) & MPU_RASR_SRD_BMSK) |
                               ((SBL_MPU_Region[i].access_perm << MPU_RASR_AP_SHFT) & MPU_RASR_AP_BMSK) |
                               ((SBL_MPU_Region[i].xn_bit << MPU_RASR_XN_SHFT) & MPU_RASR_XN_BMSK) );			  
	}

	/* Enable MPU in MPU_CTRL */
	/* Enable background region, Enable MPU for exception handlers 
	   Back ground region would cover permissions for all the device memory */
	MPU_DEP.MPU_CTRL = MPU_CTRL_ENABLE|MPU_CTRL_HFNMI_ENABLE|MPU_CTRL_PRIVDEF_ENABLE;
	sbl_memory_barrier();
}

void sbl_mpu_disable(void)
{
	MPU_DEP.MPU_CTRL = 0x0;
}

