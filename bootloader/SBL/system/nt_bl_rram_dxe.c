/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/


#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "nt_bl_rram_dxe.h"
#include "nt_bl_common.h"
#include "fermion_hw_reg.h"
#include "boot_print.h"
#include "pbl_patch_table.h"
#include "safeAPI.h"

#define DXE_CH_RRAM_WRITE_VIA_DXE 7

#define MAX_WAIT_FOR_WRITE 300 /* it takes appox 64 ms sec to write 16k byte of data */

#define RRAM_WRITE_ADR_BYTE_ALIGN 16 /* RRAM write address should be 16 byte aligned and write length should be multiple of 16 byte */

#define RRAM_WRITE_BLOCK_MAX_SIZE 0x3FF0 /* this is maximum number of bytes DXE can write using a discriptor. it is 0x3FF0 */


/**
   @brief Verifies if the given RRAM address range is valid for an RRAM write
          operation.

   @param[in] address  Start address for the RRAM write operation.
   @param[in] length   Number of bytes to be written.

   @return true if the address range is valid or false if it is not.
*/
bl_error_type rram_verify_address(uint32_t address, uint32_t length)
{
    bl_error_type status;

    if ((address >= RRAM_START) &&
        (address <= RRAM_END) &&
        (address % 4 == 0) &&
        (((RRAM_END - address) + 1) >= length))
    {
        /* Main RRAM region. */
        status = BL_ERR_NONE;
    }
    else
    {
        /* Invalid address and/or length. */
        status = BL_ERR_RRAM_DXE_INVALID_ADDR;
    }

    return (status);
}

/**
   @brief nop delay.

   @param[in] n  nop times.

   @return void
*/

static inline void nop_delay( uint32_t n )
{
   volatile uint32_t nop_count = 0;
   for( nop_count = 0; nop_count < n; nop_count++)
   {
      __asm volatile(" nop \n");
   }
}

void rram_dxe_init(void)
{
	volatile uint32_t regVal ;

    uint32_t get_val=HW_REG_RD(QWLAN_PMU_DIG_TOP_CFG_REG);
    get_val&=(~QWLAN_PMU_DIG_TOP_CFG_RRAM_32_BIT_LEGACY_WR_MODE_MASK);
    HW_REG_WR(QWLAN_PMU_DIG_TOP_CFG_REG,get_val);

	// Reset DXE : Not reset in pronto code <--  Check if required ?
	regVal =  HW_REG_RD(QWLAN_CCU_R_CCU_SOFT_RESET_REG);
	HW_REG_WR( QWLAN_CCU_R_CCU_SOFT_RESET_REG, regVal | QWLAN_CCU_R_CCU_SOFT_RESET_DXE_SOFT_RESET_MASK);

	//while (++i < 0xFFFF)
	regVal = HW_REG_RD(QWLAN_CCU_R_CCU_SOFT_RESET_REG); // Add some delay

	HW_REG_WR( QWLAN_CCU_R_CCU_SOFT_RESET_REG, regVal & (~QWLAN_CCU_R_CCU_SOFT_RESET_DXE_SOFT_RESET_MASK));
	regVal = HW_REG_RD(QWLAN_CCU_R_CCU_SOFT_RESET_REG); // Add some delay

	HW_REG_WR(QWLAN_DXE_0_DMA_CSR_REG, 0);


	HW_REG_WR(QWLAN_DXE_0_INT_MSK_REG, 0xFFF);

	// De-assert RESET, enable DXE, enable channel counter and configure DXE in LITTLE-ENDIAN mode.
	// This is not enough. we need to enable per channel counters. Pay attention to to destroy CTRL_SEL bits in CH_CTRL registers
	regVal=HW_REG_RD(QWLAN_DXE_0_DMA_CSR_REG);
	regVal = QWLAN_DXE_0_DMA_CSR_EN_MASK | QWLAN_DXE_0_DMA_CSR_ECTR_EN_MASK | QWLAN_DXE_0_DMA_CSR_H2H_SYNC_EN_MASK | QWLAN_DXE_0_DMA_CSR_TSTMP_EN_MASK
			| ((NT_DXE_CH_TSTMP_H2B_TSTMP_OFFSET << QWLAN_DXE_0_DMA_CSR_H2B_TSTMP_OFF_OFFSET) & QWLAN_DXE_0_DMA_CSR_H2B_TSTMP_OFF_MASK) | ((NT_DXE_CH_TSTMP_B2H_TSTMP_OFFSET << QWLAN_DXE_0_DMA_CSR_B2H_TSTMP_OFF_OFFSET) & QWLAN_DXE_0_DMA_CSR_B2H_TSTMP_OFF_MASK);

    regVal|=QWLAN_DXE_0_DMA_CSR_RRAM_WRITE_DLY_DEFAULT;


	HW_REG_WR(QWLAN_DXE_0_DMA_CSR_REG, regVal);

	//Clear DXE channel counters
	HW_REG_WR(QWLAN_DXE_0_CTR_CLR_REG, 0x7f);

    // Clear I-cache
    uint32_t cache_ctrl = HW_REG_RD(QWLAN_CACHE_REGS_R_CACHECTRL_REG);
    HW_REG_WR( QWLAN_CACHE_REGS_R_CACHECTRL_REG, cache_ctrl | QWLAN_CACHE_REGS_R_CACHECTRL_FLUSH_MASK );
    HW_REG_WR( QWLAN_CACHE_REGS_R_CACHECTRL_REG, cache_ctrl );

}

bl_error_type rram_block_write_dxe(uint32_t destination, const uint8_t* source, uint32_t length)
{
    static uint32_t block_number = 0;
    bl_error_type status = 0;
    uint32_t ch_sz = 0;
    uint32_t regVal;
    uint32_t xfrStatus;
    uint32_t DxeChannel_BaseAddr = 0;
    //uint32_t max_loop = MAX_WAIT_FOR_WRITE;
    DXEDesc_t dxe_hw_desc;
    //RRAM_DXE_PRINTF(COMMON, ERR, "block_number = %u destination = 0x%X source = 0x%X length = 0x%X", block_number, destination, (uint32_t)source, length);

	if ((destination & (RRAM_WRITE_ADR_BYTE_ALIGN - 1)) != 0)
		return BL_ERR_RRAM_DXE_INVALID_BLOCK_ADDR;

	if ((length & (RRAM_WRITE_ADR_BYTE_ALIGN - 1)) != 0)
		return BL_ERR_RRAM_DXE_INVALID_BLOCK_ADDR;

    memset(&dxe_hw_desc, 0, sizeof(dxe_hw_desc));

    /* Clear the32 bit legacy writes to RRAM */
    regVal = HAL_REG_RD(QWLAN_PMU_DIG_TOP_CFG_REG);
    regVal = regVal & (~QWLAN_PMU_DIG_TOP_CFG_RRAM_32_BIT_LEGACY_WR_MODE_MASK);
    regVal = regVal | (QWLAN_PMU_DIG_TOP_CFG_RRAM_PD_MODE_DEFAULT); /* this was different between MM and VI code hence making it same 4000080 vs 0x4002080 */
    HAL_REG_WR(QWLAN_PMU_DIG_TOP_CFG_REG, regVal);

    RRAM_DXE_PRINTF("QWLAN_PMU_DIG_TOP_CFG_REG = 0x%X 0x%lX\r\n", QWLAN_PMU_DIG_TOP_CFG_REG, HAL_REG_RD(QWLAN_PMU_DIG_TOP_CFG_REG));

    /* Clear the 128 Bit write Disable */
    regVal = HAL_REG_RD(QWLAN_DXE_0_DMA_CSR_REG);
    regVal = regVal & (~QWLAN_DXE_0_DMA_CSR_DIS_RRAM_128BIT_MASK);
    HAL_REG_WR(QWLAN_DXE_0_DMA_CSR_REG, regVal);

    RRAM_DXE_PRINTF("QWLAN_DXE_0_DMA_CSR_REG = 0x%X 0x%lX\r\n", QWLAN_DXE_0_DMA_CSR_REG, HAL_REG_RD(QWLAN_DXE_0_DMA_CSR_REG));

    /* create DXE HW discriptor */
    dxe_hw_desc.ctrl = NT_DXE_DESC_CTRL_INT | NT_DXE_DESC_CTRL_EOP | ((NT_DXE_DESC_CTRL_XTYPE_H2H << 1) & NT_DXE_DESC_CTRL_XTYPE_MASK) | NT_DXE_DESC_CTRL_VALID;
    dxe_hw_desc.xfrSize = length;
    dxe_hw_desc.dxedesc.dxe_short_desc.srcMemAddrL = (uint32_t)source;
    dxe_hw_desc.dxedesc.dxe_short_desc.dstMemAddrL = destination;
    dxe_hw_desc.dxedesc.dxe_short_desc.phyNextL = 0;

    RRAM_DXE_PRINTF("ctrl 0x%X xferSize 0x%X src 0x%X dst 0x%X next 0x%X\r\n", (unsigned int)dxe_hw_desc.ctrl, (unsigned int)dxe_hw_desc.xfrSize, (unsigned int)dxe_hw_desc.dxedesc.dxe_short_desc.srcMemAddrL, (unsigned int)dxe_hw_desc.dxedesc.dxe_short_desc.dstMemAddrL, (unsigned int)dxe_hw_desc.dxedesc.dxe_short_desc.phyNextL);

    /* hardcoding to be removed */
    HAL_REG_WR(QWLAN_DXE_0_DMA_CSR_REG, 0x2000a001); /* not sure if above write will be effective it does not clear 128 bit write disable */

    DxeChannel_BaseAddr = QWLAN_DXE_0_CH0_CTRL_REG + (NT_DXE_CH_REG_SIZE * DXE_CH_RRAM_WRITE_VIA_DXE);

    RRAM_DXE_PRINTF("QWLAN_DXE_0_DMA_CSR_REG = 0x%X 0x%lX DXE BA 0x%X\r\n", QWLAN_DXE_0_DMA_CSR_REG, HAL_REG_RD(QWLAN_DXE_0_DMA_CSR_REG), (unsigned int)DxeChannel_BaseAddr);

    /* Channel enable with describtor */
    ch_sz = HAL_REG_RD(DxeChannel_BaseAddr + (QWLAN_DXE_0_CH0_SZ_REG - QWLAN_DXE_0_CH0_CTRL_REG)); /* effectivally reading the DXE_CHX_SZ_REG */
    HAL_REG_WR(DxeChannel_BaseAddr + (QWLAN_DXE_0_CH0_SZ_REG - QWLAN_DXE_0_CH0_CTRL_REG), ch_sz | (4 << QWLAN_DXE_0_CH0_SZ_CHK_SZ_OFFSET));

    RRAM_DXE_PRINTF("Channel Enable with Discriptor 0x%lX 0x%lX\r\n", (DxeChannel_BaseAddr + (QWLAN_DXE_0_CH0_SZ_REG - QWLAN_DXE_0_CH0_CTRL_REG)), HAL_REG_RD(DxeChannel_BaseAddr + (QWLAN_DXE_0_CH0_SZ_REG - QWLAN_DXE_0_CH0_CTRL_REG)));

   // hw_intf_write_mem(RAM_DESCRIPTOR_ADDR, (uint8_t*)(&dxe_hw_desc), sizeof(dxe_hw_desc));
    HAL_REG_WR(DxeChannel_BaseAddr + NT_DXE_CH_DESCH_REG, 0);
    HAL_REG_WR(DxeChannel_BaseAddr + NT_DXE_CH_DESCL_REG, (uint32_t)(&dxe_hw_desc));
    HAL_REG_WR(DxeChannel_BaseAddr, 0xf80709);

    RRAM_DXE_PRINTF("DxeChannel_BaseAddr+NT_DXE_CH_DESCH_REG 0x%lX 0x%lX\r\n", (DxeChannel_BaseAddr + NT_DXE_CH_DESCH_REG), HAL_REG_RD(DxeChannel_BaseAddr + NT_DXE_CH_DESCH_REG));
    RRAM_DXE_PRINTF("DxeChannel_BaseAddr+NT_DXE_CH_DESCL_REG 0x%lX 0x%lX\r\n", (DxeChannel_BaseAddr + NT_DXE_CH_DESCL_REG), HAL_REG_RD(DxeChannel_BaseAddr + NT_DXE_CH_DESCL_REG));
    RRAM_DXE_PRINTF("DxeChannel_BaseAddr 0x%X 0x%lX\r\n", (unsigned int)DxeChannel_BaseAddr, HAL_REG_RD(DxeChannel_BaseAddr));

    xfrStatus = HAL_REG_RD(DxeChannel_BaseAddr + NT_DXE_CH_STATUS_REG);
    while ((xfrStatus & QWLAN_DXE_0_CH0_STATUS_DONE_MASK) != QWLAN_DXE_0_CH0_STATUS_DONE_MASK)
    {
        xfrStatus = HAL_REG_RD(DxeChannel_BaseAddr + NT_DXE_CH_STATUS_REG);

        nop_delay(100);
    }

    RRAM_DXE_PRINTF("STATUS  0x%lX 0x%lX\r\n", (DxeChannel_BaseAddr + NT_DXE_CH_STATUS_REG), HAL_REG_RD(DxeChannel_BaseAddr + NT_DXE_CH_STATUS_REG));

    block_number++;
    return(status);
}
/**
   @brief Write the next block of data to rram, accounting for the address not
          being aligned to an 16-byte block or the amount of data remaining
          being less than a full block.

   This function will also update  length accordingly.

   @param[in]     destination  RRAM address to write to.
   @param[in]     source       Data to write to RRAM.
   @param[in,out] length   As an input, this is the maximum amount of data to
                           write.  As an output, this is the actual amount of
                           data written.

   @return 0 if the write was successful or other value if there was an error.
*/
bl_error_type rram_write_unaligned_dxe(uint32_t destination, const uint8_t *source, uint32_t *length)
{
   bl_error_type status = BL_ERR_NONE;
   uint8_t       offset;
   uint32_t      bytes_written;

   uint8_t buffer[RRAM_WRITE_ADR_BYTE_ALIGN];


   /* Determine the offset for data to start in the 8-byte block and align the
      address. */
   offset   = (uint32_t)destination & (RRAM_WRITE_ADR_BYTE_ALIGN - 1);
   destination -= offset;

   /* Read the current data from NVM. */
   memscpy(buffer, RRAM_WRITE_ADR_BYTE_ALIGN, (void *)destination, RRAM_WRITE_ADR_BYTE_ALIGN);

   /* Overwrite the NVM data with the data from the buffer, starting at the
      offset determined above and going until it reaches the end of the 8-byte
      block or all data was consumed. */
   bytes_written = 0;
   while((offset < RRAM_WRITE_ADR_BYTE_ALIGN) && (*length != 0))
   {
      buffer[offset] = *source;

      source++;
      offset++;
      bytes_written++;
      (*length)--;
   }

   /* Go ahead and update the length to the number of bytes that will be
      written. */
   *length = bytes_written;

   /* Write the data. */
   status = PBL_IND_MOD_PFN(nt_bl_rram_dxe, rram_block_write_dxe)(destination, buffer, RRAM_WRITE_ADR_BYTE_ALIGN);

   return(status);
}

/**
   @brief Writes the provided buffer to the specified address in NVM (RRAM).

   Writes to NVM will block NVM reads so this function blocks until the NVM
   write has completed.

   destnation and source should 4 bytes aligned.

   @param[in] destination  NVM address to write to.
   @param[in] source   Data to write to NVM.
   @param[in] length   Length of the data to write to NVM.

   @return 0 if the data was successfully written to NVM or other
           value if there was an error.
*/
bl_error_type rram_write_dxe(uint32_t destination, uint8_t* source, uint32_t length)
{
    bl_error_type status = BL_ERR_NONE;
    uint32_t dst = destination;
    const uint8_t* p_src = source;
    uint32_t n_full_blocks;
    uint32_t nbytes_partial_block;
    uint32_t nbytes_written;

    uint8_t buffer[RRAM_WRITE_ADR_BYTE_ALIGN];

    /* Verify the address and size are valid. */
	status = PBL_IND_MOD_PFN(nt_bl_rram_dxe, rram_verify_address)(destination, length);

    if (status != BL_ERR_NONE) {
		RRAM_DXE_PRINTF("RRAM verify address failure\r\n");
        return status;
    }

    if(destination & (RRAM_WRITE_ADR_BYTE_ALIGN - 1)) {
        nbytes_written = length;

        /* destination isn't 16-byte aligned so use rram_write_unaligned to write
        the first part of the data. */
        status = PBL_IND_MOD_PFN(nt_bl_rram_dxe, rram_write_unaligned_dxe)(dst, p_src, &nbytes_written);

		if ((status != BL_ERR_NONE) || (length <= nbytes_written))
			return status;

        dst += nbytes_written;
        p_src += nbytes_written;
        length -= nbytes_written;
    }

    n_full_blocks = length / RRAM_WRITE_BLOCK_MAX_SIZE;
    nbytes_partial_block = length % RRAM_WRITE_BLOCK_MAX_SIZE;

    for (uint32_t i = 0; i < n_full_blocks; i++) {
        status = PBL_IND_MOD_PFN(nt_bl_rram_dxe, rram_block_write_dxe)(dst, p_src, RRAM_WRITE_BLOCK_MAX_SIZE);

        if (status != BL_ERR_NONE)
        {
			RRAM_DXE_PRINTF("RRAM write failed for block %u src 0x%X dst 0x%X\r\n", (unsigned int)i, (unsigned int)p_src, (unsigned int)dst);
            return(status);
        }
        dst += RRAM_WRITE_BLOCK_MAX_SIZE;
        p_src += RRAM_WRITE_BLOCK_MAX_SIZE;
    }

    if (nbytes_partial_block) {
        uint32_t nbytes_partial_block_alinged = nbytes_partial_block - (nbytes_partial_block & (RRAM_WRITE_ADR_BYTE_ALIGN - 1));
        uint32_t nbytes_partial_block_unalinged = nbytes_partial_block & (RRAM_WRITE_ADR_BYTE_ALIGN - 1);

        status = PBL_IND_MOD_PFN(nt_bl_rram_dxe, rram_block_write_dxe)(dst, p_src, nbytes_partial_block_alinged);

        if (status != BL_ERR_NONE) {
            /* RRAM write failed */
            RRAM_DXE_PRINTF("RRAM write failed src 0x%X dst 0x%X", (unsigned int)p_src, (unsigned int)dst);
            return(status);
        }

        dst += nbytes_partial_block_alinged;
        p_src += nbytes_partial_block_alinged;

        if (nbytes_partial_block_unalinged) {
            memscpy(buffer, nbytes_partial_block_unalinged, p_src, nbytes_partial_block_unalinged);
            // Read the destination RRAM block if we're not overwriting the entire block.
            memscpy(buffer + nbytes_partial_block_unalinged, RRAM_WRITE_ADR_BYTE_ALIGN -nbytes_partial_block_unalinged, (void *)(dst + nbytes_partial_block_unalinged),
                RRAM_WRITE_ADR_BYTE_ALIGN -nbytes_partial_block_unalinged);
            status = PBL_IND_MOD_PFN(nt_bl_rram_dxe, rram_block_write_dxe)(dst, buffer, RRAM_WRITE_ADR_BYTE_ALIGN);

			if (status != BL_ERR_NONE) {
                /* RRAM write failed */
                RRAM_DXE_PRINTF("RRAM write failed src 0x%X dst 0x%X\r\n", (unsigned int)buffer, (unsigned int)dst);
                return(status);
            }

            dst += RRAM_WRITE_ADR_BYTE_ALIGN;
            p_src += RRAM_WRITE_ADR_BYTE_ALIGN;
        }

    }

    return status;
}

static uint8_t  set_buffer[RRAM_SET_BUFFER_SIZE];

bl_error_type rram_memset_dxe(uint32_t address, int32_t value, uint32_t length)
{
   bl_error_type   status = BL_ERR_NONE;
   uint32_t        dst_addr;
   uint32_t        set_length;

   /* Verify the address and size are valid. */
	status = PBL_IND_MOD_PFN(nt_bl_rram_dxe, rram_verify_address)(address, length);

    if (status != BL_ERR_NONE) {
		RRAM_DXE_PRINTF("RRAM verify address failure\r\n");
        return status;
    }

	memset(set_buffer, value, sizeof(set_buffer));

	/* Use a local variable for the address to make the math simpler. */
	dst_addr = address;

	if(dst_addr & (RRAM_WRITE_ADR_BYTE_ALIGN - 1))
	{
		/* Perform a small set at the beginning to align the erase operation. */
		set_length = (RRAM_WRITE_ADR_BYTE_ALIGN <= length) ? RRAM_WRITE_ADR_BYTE_ALIGN : length;

		status = PBL_IND_MOD_PFN(nt_bl_rram_dxe, rram_write_unaligned_dxe)(dst_addr, (const uint8_t *)set_buffer, &set_length);

		if ((status != BL_ERR_NONE) || (length <= set_length))
			return status;

		dst_addr += set_length;
		length     -= set_length;
	}

	  /* Set the remaining data. */
	while((status == BL_ERR_NONE) && (length != 0))
	{
		set_length = (RRAM_SET_BUFFER_SIZE <= length) ? RRAM_SET_BUFFER_SIZE : length;

		/* Call rram_write to write the remaining data (as this will
		also handle the remaining length being unaligned). */
		status = PBL_IND_MOD_PFN(nt_bl_rram_dxe, rram_write_dxe)(dst_addr, set_buffer, set_length);

		if(status) {
            RRAM_DXE_PRINTF("RRAM memset failed dst 0x%X\r\n", (unsigned int)dst_addr);
			return status;
		}
		/* Adjust the pointers. */
		dst_addr += set_length;
		length     -= set_length;
	}

	   return status;
}



