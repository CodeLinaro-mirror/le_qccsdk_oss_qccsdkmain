/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "nt_hw.h"
#include "hal_int_sys.h"
#include "nt_osal.h"
#include "safeAPI.h"
#include <stdio.h>


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

    //printf("prng_get len:%d\r\n",len);

    volatile uint32 tmp_iv;
    uint32_t i;
    const uint32_t unit_random_len = 4;

    if (!ptr || (0==len)){
        return FALSE;
    }

    for (i=0; i<(len/unit_random_len); i++) {
        tmp_iv = nt_pget_rng();
        memscpy((void*)ptr, 4, (void*)&tmp_iv, 4);
        ptr += 4;
    }
    if(len%4){
        tmp_iv = nt_pget_rng();
        memscpy((void*)ptr, len%4, (void*)&tmp_iv, len%4);
    }
    return TRUE;
}


