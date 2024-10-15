/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/

/*-------------------------------------------------------------------------
* Include Files
*-----------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include "qapi_status.h"
#include "nvm_common.h"
#include "nvm_rram.h"
#include "dxe.h"
#include "fermion_hw_reg.h"
#include "uart_print.h"
#include "safeAPI.h"

/*-------------------------------------------------------------------------
 * Preprocessor Definitions, Constants, and Type Declarations
 *-----------------------------------------------------------------------*/

//#define RRAM_WRITE_VIA_DXE_DEBUG

#define DXE_CH_RRAM_WRITE_VIA_DXE 7

#define MAX_WAIT_FOR_WRITE 300 /* it takes appox 64 ms sec to write 16k byte of data */

#define RRAM_WRITE_ADR_BYTE_ALIGN 16 /* RRAM write address should be 16 byte aligned and write length should be multiple of 16 byte */

#define RRAM_WRITE_BLOCK_MAX_SIZE 0x3FF0 /* this is maximum number of bytes DXE can write using a discriptor. it is 0x3FF0 */

#define NT_DXE_CH_TSTMP_H2B_TSTMP_OFFSET       8
#define NT_DXE_CH_TSTMP_B2H_TSTMP_OFFSET       10

/*-------------------------------------------------------------------------
 * Private Function Declarations
 *-----------------------------------------------------------------------*/

/*-------------------------------------------------------------------------
 * Function Definitions
 *-----------------------------------------------------------------------*/

/**
   @brief Initialize/Start/Reset RRAM DXE.

   This function must be called before any other RRAM DXE functions(rram_write, rram_erase).

   @param[in] void

   @return void.
*/

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


/**
   @brief Verifies if the given RRAM address range is valid for an RRAM write
          operation.

   @param[in] address  Start address for the RRAM write operation.
   @param[in] length   Number of bytes to be written.

   @return true if the address range is valid or false if it is not.
*/
static qbool_t otp_rram_verify_address(uint32_t address, uint32_t length)
{
    qbool_t ret_val;

    if ((address >= OTP_START) &&
        (address <= OTP_END) &&
        (address % 4 == 0) &&
        (((OTP_END - address) + 1) >= length))
    {
        /* Main RRAM region. */
        ret_val = true;
    }
    else
    {
        /* Invalid address and/or length. */
        ret_val = false;
    }

    return (ret_val);
}


/**
   @brief Verifies if the given RRAM address range is valid for an RRAM write
          operation.

   @param[in] address  Start address for the RRAM write operation.
   @param[in] length   Number of bytes to be written.

   @return true if the address range is valid or false if it is not.
*/
static qbool_t nvm_rram_verify_address(uint32_t address, uint32_t length)
{
    qbool_t ret_val;

    if ((address >= RRAM_START) &&
        (address <= RRAM_END) &&
        (address % 4 == 0) &&
        (((RRAM_END - address) + 1) >= length))
    {
        /* Main RRAM region. */
        ret_val = true;
    }
    else
    {
        /* Invalid address and/or length. */
        ret_val = false;
    }

    return (ret_val);
}


/**
   @brief Verifies if the given RRAM  address range is valid for an RRAM erase
          operation.

   @param[in] address  Start address for the RRAM write operation.
   @param[in] length   Number of bytes to be written.

   @return true if the address range is valid or false if it is not.
*/
static qbool_t rram_verify_erase_address(uint32_t address, uint32_t length)
{
    qbool_t ret_val;

    if ((address >= RRAM_ERASE_START) &&
        (address <= RRAM_END) &&
        (address % 4 == 0) &&
        (((RRAM_END - address) + 1) >= length))
    {
        /* Main RRAM region. */
        ret_val = true;
    }
    else
    {
        /* Invalid address and/or length. */
        ret_val = false;
    }

    return (ret_val);
}


/**
   @brief nop delay.

   @param[in] n  nop times.

   @return void
*/

void nop_delay( uint32_t n )
{
   volatile uint32_t nop_count = 0;
   for( nop_count = 0; nop_count < n; nop_count++)
   {
      __asm volatile(" nop \n");
   }
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
static int32_t rram_write_unaligned(uint32_t destination, const uint8_t *source, uint32_t *length)
{
   int32_t status;
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
   status = rram_block_write(destination, buffer, RRAM_WRITE_ADR_BYTE_ALIGN);

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
int32_t rram_write(uint32_t destination, uint8_t* source, uint32_t length)
{
    int32_t status = 0;
    uint32_t dst = destination;
    const uint8_t* p_src = source;
    uint32_t n_full_blocks;
    uint32_t nbytes_partial_block;
    uint32_t nbytes_written;

    uint8_t buffer[RRAM_WRITE_ADR_BYTE_ALIGN];

    if(destination & (RRAM_WRITE_ADR_BYTE_ALIGN - 1))
    {
        nbytes_written = length;

        /* destination isn't 16-byte aligned so use rram_write_unaligned to write
        the first part of the data. */
        status = rram_write_unaligned(dst, p_src, &nbytes_written);

        dst += nbytes_written;
        p_src += nbytes_written;
        length     -= nbytes_written;
    }
    else
    {
        status = QAPI_OK;
    }

    n_full_blocks = length / RRAM_WRITE_BLOCK_MAX_SIZE;
    nbytes_partial_block = length % RRAM_WRITE_BLOCK_MAX_SIZE;

    for (uint32_t i = 0; i < n_full_blocks; i++) {
        status = rram_block_write(dst, p_src, RRAM_WRITE_BLOCK_MAX_SIZE);
        if (status != 0)
        {
            /* RRAM write failed */
           // NT_LOG_PRINT(COMMON, ERR, "RRAM write failed for block %d src 0x%X dst 0x%X", i, (uint32_t)p_src, dst);
            return(status);
        }
        dst += RRAM_WRITE_BLOCK_MAX_SIZE;
        p_src += RRAM_WRITE_BLOCK_MAX_SIZE;
    }

    if (nbytes_partial_block) {
        uint32_t nbytes_partial_block_alinged = nbytes_partial_block - (nbytes_partial_block & (RRAM_WRITE_ADR_BYTE_ALIGN - 1));
        uint32_t nbytes_partial_block_unalinged = nbytes_partial_block & (RRAM_WRITE_ADR_BYTE_ALIGN - 1);

        status = rram_block_write(dst, p_src, nbytes_partial_block_alinged);
        if (status != 0)
        {
            /* RRAM write failed */
            //NT_LOG_PRINT(COMMON, ERR, "RRAM write failed src 0x%X dst 0x%X", (uint32_t)p_src, dst);
            return(status);
        }
        dst += nbytes_partial_block_alinged;
        p_src += nbytes_partial_block_alinged;

        if (nbytes_partial_block_unalinged) {
            memscpy(buffer, nbytes_partial_block_unalinged, p_src, nbytes_partial_block_unalinged);
            // Read the destination RRAM block if we're not overwriting the entire block.
            memscpy(buffer + nbytes_partial_block_unalinged, RRAM_WRITE_ADR_BYTE_ALIGN -nbytes_partial_block_unalinged, (void *)(dst + nbytes_partial_block_unalinged),
                RRAM_WRITE_ADR_BYTE_ALIGN -nbytes_partial_block_unalinged);
            status = rram_block_write(dst, buffer, RRAM_WRITE_ADR_BYTE_ALIGN);
            if (status != 0)
            {
                /* RRAM write failed */
                //NT_LOG_PRINT(COMMON, ERR, "RRAM write failed src 0x%X dst 0x%X", (uint32_t)buffer, dst);
                return(status);
            }
            dst += RRAM_WRITE_ADR_BYTE_ALIGN;
            p_src += RRAM_WRITE_ADR_BYTE_ALIGN;
        }

    }

    return status;
}

/**
   @brief Writes the provided buffer to the specified address of OTP.

   destnation and source should 4 bytes aligned.

   @param[in] destination  NVM address to write to.
   @param[in] source   Data to write to NVM.
   @param[in] length   Length of the data to write to NVM.

   @return 0 if the data was successfully written to NVM or other
           value if there was an error.
*/
int32_t otp_rram_write(uint32_t destination, uint8_t* source, uint32_t length)
{
    if (otp_rram_verify_address(destination, length) == false) {
        return QAPI_ERR_BOUNDS;
    }

    //keep READ_PERMISSION_READ_WRITE_PERMISIONS always 0
    if(READ_PERMISSION_READ_WRITE_PERMISIONS_ADDRESS == destination)
    {
        source[0] &= ~(READ_PERMISSION_READ_WRITE_PERMISIONS_OFFSET);
    }
    return rram_write(destination, source, length);
}

/**
   @brief Writes the provided buffer to the specified address of OTP.

   destnation and source should 4 bytes aligned.

   @param[in] destination  NVM address to write to.
   @param[in] source   Data to write to NVM.
   @param[in] length   Length of the data to write to NVM.

   @return 0 if the data was successfully written to NVM or other
           value if there was an error.
*/
int32_t nvm_rram_write(uint32_t destination, uint8_t* source, uint32_t length)
{
    if (nvm_rram_verify_address(destination, length) == false) {
        return QAPI_ERR_BOUNDS;
    }
    return rram_write(destination, source, length);
}


int32_t rram_block_write(uint32_t destination, const uint8_t* source, uint32_t length)
{
    static uint32_t block_number = 0;
    int32_t status = 0;
    uint32_t ch_sz = 0;
    uint32_t regVal;
    uint32_t xfrStatus;
    uint32_t DxeChannel_BaseAddr = 0;
    //uint32_t max_loop = MAX_WAIT_FOR_WRITE;
    DXEDesc_t dxe_hw_desc;
    //NT_LOG_PRINT(COMMON, ERR, "block_number = %u destination = 0x%X source = 0x%X length = 0x%X", block_number, destination, (uint32_t)source, length);

    assert((destination & (RRAM_WRITE_ADR_BYTE_ALIGN - 1)) == 0);
    assert((length & (RRAM_WRITE_ADR_BYTE_ALIGN - 1)) == 0);

    memset(&dxe_hw_desc, 0, sizeof(dxe_hw_desc));

    /* Clear the32 bit legacy writes to RRAM */
    regVal = HW_REG_RD(QWLAN_PMU_DIG_TOP_CFG_REG);
    regVal = regVal & (~QWLAN_PMU_DIG_TOP_CFG_RRAM_32_BIT_LEGACY_WR_MODE_MASK);
    regVal = regVal | (QWLAN_PMU_DIG_TOP_CFG_RRAM_PD_MODE_DEFAULT); /* this was different between MM and VI code hence making it same 4000080 vs 0x4002080 */
    HW_REG_WR(QWLAN_PMU_DIG_TOP_CFG_REG, regVal);

#ifdef RRAM_WRITE_VIA_DXE_DEBUG
    NT_LOG_PRINT(COMMON, ERR, "QWLAN_PMU_DIG_TOP_CFG_REG = 0x%X 0x%X", QWLAN_PMU_DIG_TOP_CFG_REG, HW_REG_RD(QWLAN_PMU_DIG_TOP_CFG_REG));
#endif
    /* Clear the 128 Bit write Disable */
    regVal = HW_REG_RD(QWLAN_DXE_0_DMA_CSR_REG);
    regVal = regVal & (~QWLAN_DXE_0_DMA_CSR_DIS_RRAM_128BIT_MASK);
    HW_REG_WR(QWLAN_DXE_0_DMA_CSR_REG, regVal);

#ifdef RRAM_WRITE_VIA_DXE_DEBUG
    NT_LOG_PRINT(COMMON, ERR, "QWLAN_DXE_0_DMA_CSR_REG = 0x%X 0x%X", QWLAN_DXE_0_DMA_CSR_REG, HW_REG_RD(QWLAN_DXE_0_DMA_CSR_REG));
#endif

    /* create DXE HW discriptor */
    dxe_hw_desc.ctrl = NT_DXE_DESC_CTRL_INT | NT_DXE_DESC_CTRL_EOP | ((NT_DXE_DESC_CTRL_XTYPE_H2H << 1) & NT_DXE_DESC_CTRL_XTYPE_MASK) | NT_DXE_DESC_CTRL_VALID;
    dxe_hw_desc.xfrSize = length;
    dxe_hw_desc.dxedesc.dxe_short_desc.srcMemAddrL = (uint32_t)source;
    dxe_hw_desc.dxedesc.dxe_short_desc.dstMemAddrL = destination;
    dxe_hw_desc.dxedesc.dxe_short_desc.phyNextL = 0;

#ifdef RRAM_WRITE_VIA_DXE_DEBUG
    NT_LOG_PRINT(COMMON, ERR, "ctrl 0x%X xferSize 0x%X src 0x%X dst 0x%X next 0x%X", dxe_hw_desc.ctrl, dxe_hw_desc.xfrSize, dxe_hw_desc.dxedesc.dxe_short_desc.srcMemAddrL, dxe_hw_desc.dxedesc.dxe_short_desc.dstMemAddrL, dxe_hw_desc.dxedesc.dxe_short_desc.phyNextL);
#endif

    /* hardcoding to be removed */
    HW_REG_WR(QWLAN_DXE_0_DMA_CSR_REG, 0x2000a001); /* not sure if above write will be effective it does not clear 128 bit write disable */

    DxeChannel_BaseAddr = QWLAN_DXE_0_CH0_CTRL_REG + (NT_DXE_CH_REG_SIZE * DXE_CH_RRAM_WRITE_VIA_DXE);

#ifdef RRAM_WRITE_VIA_DXE_DEBUG
    NT_LOG_PRINT(COMMON, ERR, "QWLAN_DXE_0_DMA_CSR_REG = 0x%X 0x%X DXE BA 0x%X", QWLAN_DXE_0_DMA_CSR_REG, HW_REG_RD(QWLAN_DXE_0_DMA_CSR_REG), DxeChannel_BaseAddr);
#endif

    /* Channel enable with describtor */
    ch_sz = HW_REG_RD(DxeChannel_BaseAddr + (QWLAN_DXE_0_CH0_SZ_REG - QWLAN_DXE_0_CH0_CTRL_REG)); /* effectivally reading the DXE_CHX_SZ_REG */
    HW_REG_WR(DxeChannel_BaseAddr + (QWLAN_DXE_0_CH0_SZ_REG - QWLAN_DXE_0_CH0_CTRL_REG), ch_sz | (4 << QWLAN_DXE_0_CH0_SZ_CHK_SZ_OFFSET));

#ifdef RRAM_WRITE_VIA_DXE_DEBUG
    NT_LOG_PRINT(COMMON, ERR, "Channel Enable with Discriptor 0x%X 0x%X", (DxeChannel_BaseAddr + (QWLAN_DXE_0_CH0_SZ_REG - QWLAN_DXE_0_CH0_CTRL_REG)), HW_REG_RD(DxeChannel_BaseAddr + (QWLAN_DXE_0_CH0_SZ_REG - QWLAN_DXE_0_CH0_CTRL_REG)));
#endif


   // hw_intf_write_mem(RAM_DESCRIPTOR_ADDR, (uint8_t*)(&dxe_hw_desc), sizeof(dxe_hw_desc));
    HW_REG_WR(DxeChannel_BaseAddr + NT_DXE_CH_DESCH_REG, 0);
    HW_REG_WR(DxeChannel_BaseAddr + NT_DXE_CH_DESCL_REG, (uint32_t)(&dxe_hw_desc));
    HW_REG_WR(DxeChannel_BaseAddr, 0xf80709);

#ifdef RRAM_WRITE_VIA_DXE_DEBUG
    NT_LOG_PRINT(COMMON, ERR, "DxeChannel_BaseAddr+NT_DXE_CH_DESCH_REG 0x%X 0x%X", (DxeChannel_BaseAddr + NT_DXE_CH_DESCH_REG), HW_REG_RD(DxeChannel_BaseAddr + NT_DXE_CH_DESCH_REG));
    NT_LOG_PRINT(COMMON, ERR, "DxeChannel_BaseAddr+NT_DXE_CH_DESCL_REG 0x%X 0x%X", (DxeChannel_BaseAddr + NT_DXE_CH_DESCL_REG), HW_REG_RD(DxeChannel_BaseAddr + NT_DXE_CH_DESCL_REG));
    NT_LOG_PRINT(COMMON, ERR, "DxeChannel_BaseAddr 0x%X 0x%X", DxeChannel_BaseAddr, HW_REG_RD(DxeChannel_BaseAddr));
#endif

    xfrStatus = HW_REG_RD(DxeChannel_BaseAddr + NT_DXE_CH_STATUS_REG);
    while ((xfrStatus & QWLAN_DXE_0_CH0_STATUS_DONE_MASK) != QWLAN_DXE_0_CH0_STATUS_DONE_MASK)
    {
        xfrStatus = HW_REG_RD(DxeChannel_BaseAddr + NT_DXE_CH_STATUS_REG);

        nop_delay(100);
    }

#ifdef RRAM_WRITE_VIA_DXE_DEBUG
    NT_LOG_PRINT(COMMON, ERR, "STATUS  0x%X 0x%X", (DxeChannel_BaseAddr + NT_DXE_CH_STATUS_REG), HW_REG_RD(DxeChannel_BaseAddr + NT_DXE_CH_STATUS_REG));
#endif
    block_number++;
    return(status);
}

int32_t nvm_rram_read(uint32_t address, uint8_t *buffer, uint32_t length)
{
    /* Verify the address and size are valid. */
    if (nvm_rram_verify_address(address, length) == false) {
        return QAPI_ERR_BOUNDS;
    }

    rram_read(address, buffer, length);
    return 0;
}

int32_t otp_rram_read(uint32_t address, uint8_t *buffer, uint32_t length)
{
    /* Verify the address and size are valid. */
    if (otp_rram_verify_address(address, length) == false) {
        return QAPI_ERR_BOUNDS;
    }

    rram_read(address, buffer, length);
    return 0;
}

int32_t rram_read(uint32_t address, uint8_t *buffer, uint32_t length)
{

    memscpy((void *)buffer,length,(void *)address,length);
    return 0;
}

/**
   @brief Erases a section of RRAM.

   Writes to RRAM will block RRAM reads so this function blocks until the RRAM
   write has completed.

   @param[in] address  RRAM address to erase.
   @param[in] length   Length of the data to erase.

   @return QAPI_OK if the data was successfully erase or a negative value if
           there was an error.
*/
static uint8_t         erase_buffer[RRAM_ERASE_BUFFER_SIZE];

int32_t rram_erase(uint32_t address, uint32_t length)
{
   qapi_Status_t   status;
   uint32_t        dst_addr;
   uint32_t        erase_length;


   /* Verify the address and size are valid. */
   if(rram_verify_erase_address(address, length))
   {
      memset(erase_buffer, 0xFF, sizeof(erase_buffer));

      /* Use a local variable for the address to make the math simpler. */
      dst_addr = address;

      if(dst_addr & (RRAM_WRITE_ADR_BYTE_ALIGN - 1))
      {
         /* Perform a small erase at the beginning to align the erase operation. */
         erase_length = (RRAM_WRITE_ADR_BYTE_ALIGN <= length) ? RRAM_WRITE_ADR_BYTE_ALIGN : length;

         status = rram_write_unaligned(dst_addr, (const uint8_t *)erase_buffer, &erase_length);
         dst_addr += erase_length;
         length     -= erase_length;
      }
      else
      {
         status = QAPI_OK;
      }

      /* Erase the remaining data. */
      while((status == QAPI_OK) && (length != 0))
      {
         erase_length = (RRAM_ERASE_BUFFER_SIZE <= length) ? RRAM_ERASE_BUFFER_SIZE : length;

         /* Call rram_write to write the remaining data (as this will
            also handle the remaining length being unaligned). */
         status = rram_write(dst_addr, erase_buffer, erase_length);
         if(status)
         {
            return -1;
         }
         /* Adjust the pointers. */
         dst_addr += erase_length;
         length     -= erase_length;
      }
   }
   else
   {
      status = QAPI_ERR_BOUNDS;
   }

   return status;
}
