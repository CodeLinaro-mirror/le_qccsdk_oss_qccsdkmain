/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "nt_hw.h"
#include "hal_int_sys.h"
#include "nt_osal.h"


int8_t nt_prng_init(void)
{
	uint32_t val = 0;

	// Issue a PRNG SW reset
	val = HAL_REG_RD(QWLAN_PRNG_R_PRNG_CONFIG_REG);
	val |= ( 1 << QWLAN_PRNG_R_PRNG_CONFIG_SW_RESET_OFFSET);
	HAL_REG_WR(QWLAN_PRNG_R_PRNG_CONFIG_REG, val);

	nt_normal_delay(5);

	/* Enable RNG clock source || By default peripheral clock is enabled in ccu register */
	/* RNG Peripheral enable | Enable PRNG only if it is not already enabled */
	//setting the ring oscillator clock
	val = HAL_REG_RD(QWLAN_PRNG_R_PRNG_DEBUG_REG);

	val = HAL_REG_RD(QWLAN_PRNG_R_PRNG_GEN_CNTR_REG);

	val = HAL_REG_RD(QWLAN_PRNG_R_PRNG_STATUS_REG);

	val |= (QWLAN_PRNG_R_PRNG_LFSR_CFG_RING_OSC0_CFG_EFEEDBACK_POINT_0 << QWLAN_PRNG_R_PRNG_LFSR_CFG_RING_OSC0_CFG_OFFSET)
				   | QWLAN_PRNG_R_PRNG_LFSR_CFG_LFSR0_EN_MASK
				   | (QWLAN_PRNG_R_PRNG_LFSR_CFG_RING_OSC1_CFG_EFEEDBACK_POINT_0 << QWLAN_PRNG_R_PRNG_LFSR_CFG_RING_OSC1_CFG_OFFSET)
				   | QWLAN_PRNG_R_PRNG_LFSR_CFG_LFSR1_EN_MASK
				   | (QWLAN_PRNG_R_PRNG_LFSR_CFG_RING_OSC2_CFG_EFEEDBACK_POINT_0 << QWLAN_PRNG_R_PRNG_LFSR_CFG_RING_OSC2_CFG_OFFSET)
				   | QWLAN_PRNG_R_PRNG_LFSR_CFG_LFSR2_EN_MASK
				   | (QWLAN_PRNG_R_PRNG_LFSR_CFG_RING_OSC3_CFG_EFEEDBACK_POINT_0 << QWLAN_PRNG_R_PRNG_LFSR_CFG_RING_OSC3_CFG_OFFSET)
				   | QWLAN_PRNG_R_PRNG_LFSR_CFG_LFSR3_EN_MASK;

	HAL_REG_WR(QWLAN_PRNG_R_PRNG_LFSR_CFG_REG,val);

	// enable the PRNG

	nt_normal_delay(5);

	val = HAL_REG_RD(QWLAN_PRNG_R_PRNG_CONFIG_REG);
	val |= ( 1 << QWLAN_PRNG_R_PRNG_CONFIG_PRNG_EN_OFFSET);
	HAL_REG_WR(QWLAN_PRNG_R_PRNG_CONFIG_REG, val);

	val = HAL_REG_RD(QWLAN_PRNG_R_PRNG_STATUS_REG);

	val = HAL_REG_RD(QWLAN_PRNG_R_PRNG_DEBUG_REG);

	return 0;

}

uint32_t nt_pget_rng( void )
{
   uint32_t data = 0;
   uint32_t val = 0;

   val = HAL_REG_RD(QWLAN_PRNG_R_PRNG_STATUS_REG);
   val &= (1 << QWLAN_PRNG_R_PRNG_STATUS_DATA_AVAIL_OFFSET);
   if(val)
   {
     data = HAL_REG_RD(QWLAN_PRNG_R_PRNG_DATA_OUT_REG);

   }

   return data;
}

NT_BOOL nt_wlan_hw_prng_get(uint8_t *ptr, uint16_t len) {

	uint32_t returned_data= 0;
	returned_data = nt_pget_rng();

	memcpy(ptr,&returned_data,len) ;

	if(ptr == NULL)
	{
		return FALSE;
	}
	return TRUE;
}


