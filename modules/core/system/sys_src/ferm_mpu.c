/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nt_flags.h"
#include "nt_logger_api.h"
#include "ferm_mpu.h"
#include "wifi_fw_pwr_cb_infra.h"

#define ALIGN_ADDRESS(addr, size)   ((addr) & (~(size - 1)))
#define ADDRESS_ALIGNED(addr, size) ((addr) & ((size - 1)))
#define ALIGNED_SIZE(size)          ((size) & (size - 1))
#define RASR_SIZE(size)             (((0x20) - (__clz(size))) - 1 - 1)

#define MPU_RBAR_MASK(size) (~((1 << (size)) - 1))

/* region_index		start_addr		size	xn_bit   access_perm	sub_region_mask; */
mpu_region_t Fermion_MPU_Region[8] = {
    /*
        {0x00, 	0x0,		0x100000,	0x01,		0x03,		0xe0},
        {0x01, 	0x0,		0x8000,		0x00,		0x06,		0x00},
        {0x02, 	0x10000,	0x10000,	0x00,		0x06,		0x1f},
        {0x03, 	0x0,	    0x0,    	0x00,		0x00,		0x0},
        {0x04, 	0x200000,	0x200000,	0x00,		0x06,		0xc0},
        {0x05, 	0x200000,	0x80000,	0x01,		0x03,		0xe0},
        {0x06, 	0x370000,	0x10000,	0x01,		0x03,		0x1f},
        {0x07, 	0x0,	    0x400,      0x00,       0x03,       0x07}
    */
    {0x00, CONFIG_FERM_MPU_0_START, CONFIG_FERM_MPU_0_SIZE, CONFIG_FERM_MPU_0_XN, CONFIG_FERM_MPU_0_ACCESS,
     CONFIG_FERM_MPU_0_SUBREGION},
    {0x01, CONFIG_FERM_MPU_1_START, CONFIG_FERM_MPU_1_SIZE, CONFIG_FERM_MPU_1_XN, CONFIG_FERM_MPU_1_ACCESS,
     CONFIG_FERM_MPU_1_SUBREGION},
    {0x02, CONFIG_FERM_MPU_2_START, CONFIG_FERM_MPU_2_SIZE, CONFIG_FERM_MPU_2_XN, CONFIG_FERM_MPU_2_ACCESS,
     CONFIG_FERM_MPU_2_SUBREGION},
    {0x03, CONFIG_FERM_MPU_3_START, CONFIG_FERM_MPU_3_SIZE, CONFIG_FERM_MPU_3_XN, CONFIG_FERM_MPU_3_ACCESS,
     CONFIG_FERM_MPU_3_SUBREGION},
    {0x04, CONFIG_FERM_MPU_4_START, CONFIG_FERM_MPU_4_SIZE, CONFIG_FERM_MPU_4_XN, CONFIG_FERM_MPU_4_ACCESS,
     CONFIG_FERM_MPU_4_SUBREGION},
    {0x05, CONFIG_FERM_MPU_5_START, CONFIG_FERM_MPU_5_SIZE, CONFIG_FERM_MPU_5_XN, CONFIG_FERM_MPU_5_ACCESS,
     CONFIG_FERM_MPU_5_SUBREGION},
    {0x06, CONFIG_FERM_MPU_6_START, CONFIG_FERM_MPU_6_SIZE, CONFIG_FERM_MPU_6_XN, CONFIG_FERM_MPU_6_ACCESS,
     CONFIG_FERM_MPU_6_SUBREGION},
    {0x07, CONFIG_FERM_MPU_7_START, CONFIG_FERM_MPU_7_SIZE, CONFIG_FERM_MPU_7_XN, CONFIG_FERM_MPU_7_ACCESS,
     CONFIG_FERM_MPU_7_SUBREGION},
};

static uint8_t ferm_encode_rasr_size(uint32_t size)
{
    uint8_t result = 0;

    /* Check if region_size is valid
     * region_size should be no less than 32 bytes, and be power of 2
     */
    if ((size < MPU_MIN_REGION_SIZE) || ((ALIGNED_SIZE(size)) != 0))
        return 0;

    /* Encode region_size so that 2^(result+1) == size */
    while (size) {
        size /= 2;
        result++;
    }
    return (result - 2);
}

static inline void ferm_memory_barrier(void)
{
    __asm volatile("dsb \n");
    __asm volatile("isb \n");
}

void ferm_mpu_config(void)
{
    uint32_t region_start_addr;
    uint8_t rasr_size = 0;

    /* Make sure MPU_TYPE register DREGION reads 8 to support 8 regions*/
    if (MPU_NUM_REGIONS != ((MPU_DEP.MPU_TYPE & MPU_TYPE_DREGION_BMSK) >> MPU_TYPE_DREGION_SHFT)) {
        NT_LOG_PRINT(SYSTEM, ERR, "NUM regions for Cortex M4 must be 8");
        return;
    }

    /* Disable MPU set MPU_CTRL bit 0 to 0x0 */
    MPU_DEP.MPU_CTRL = 0x0;

    /* For each of the MPU region*/
    for (uint32_t i = 0; i < MPU_NUM_REGIONS; ++i) {
        if (Fermion_MPU_Region[i].region_size == 0) {
            continue;
        }

        /* Select the region number in RNR */
        MPU_DEP.MPU_RNR = ((Fermion_MPU_Region[i].region_index << MPU_RNR_REGION_SHFT) & MPU_RNR_REGION_BMSK);

        /* Encode the region_size */
        /* We need convert region_size to SIZE where 2^(SIZE+1) equals to region_size */
        rasr_size = ferm_encode_rasr_size(Fermion_MPU_Region[i].region_size);
        if (!rasr_size) {
            NT_LOG_PRINT(SYSTEM, ERR, "Region Size must be at least 32 bytes, and power of 2");
            return;
        }

        /* Check if start_address is aligned with region_size */
        if ((ADDRESS_ALIGNED(Fermion_MPU_Region[i].start_addr, Fermion_MPU_Region[i].region_size)) != 0) {
            NT_LOG_PRINT(SYSTEM, ERR, "Start Address should be aligned with Region Size");
            return;
        }
        /* We only need to write the MSB bits of the address after alignment. */
        region_start_addr = Fermion_MPU_Region[i].start_addr >> rasr_size;

        /* RNR is already populated with the region number, so set the region field in RBAR to be invalid */
        /* Set RBAR and RASR */
        MPU_DEP.MPU_RBAR =
            ((0x0 << MPU_RBAR_VALID_SHFT) | ((region_start_addr << rasr_size) & MPU_RBAR_MASK(rasr_size)));

        MPU_DEP.MPU_RASR = (((MPU_REGION_ENABLE << MPU_RASR_ENABLE_SHFT) & MPU_RASR_ENABLE_BMSK) |
                            ((rasr_size << MPU_RASR_SIZE_SHFT) & MPU_RASR_SIZE_BMSK) |
                            ((Fermion_MPU_Region[i].sub_region_mask << MPU_RASR_SRD_SHFT) & MPU_RASR_SRD_BMSK) |
                            ((Fermion_MPU_Region[i].access_perm << MPU_RASR_AP_SHFT) & MPU_RASR_AP_BMSK) |
                            ((Fermion_MPU_Region[i].xn_bit << MPU_RASR_XN_SHFT) & MPU_RASR_XN_BMSK));
    }

    /* Enable MPU in MPU_CTRL */
    /* Enable background region, Enable MPU for exception handlers
       Back ground region would cover permissions for all the device memory */
    MPU_DEP.MPU_CTRL = MPU_CTRL_ENABLE | MPU_CTRL_HFNMI_ENABLE | MPU_CTRL_PRIVDEF_ENABLE;

    ferm_memory_barrier();
}

void ferm_mpu_power_state_change_cb(uint8_t evt)
{
    if (evt == PWR_EVT_WMAC_PRE_SLEEP) {
        MPU_DEP.MPU_CTRL = 0x0;
        ferm_memory_barrier();
    }

    if ((evt == PWR_EVT_WMAC_POST_AWAKE) || (evt == PWR_EVT_WMAC_SLEEP_ABORT)) {
        ferm_mpu_config();
    }

}

void ferm_mpu_init(void)
{
    ferm_mpu_config();
    fpci_evt_cb_reg((ps_evt_cb_t)&ferm_mpu_power_state_change_cb,
                    PWR_EVT_WMAC_PRE_SLEEP | PWR_EVT_WMAC_POST_AWAKE | PWR_EVT_WMAC_SLEEP_ABORT, 10, NULL);
}
