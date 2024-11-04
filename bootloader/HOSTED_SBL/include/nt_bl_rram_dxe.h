/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _RRAM_H_
#define _RRAM_H_

#include "boot_error_if.h"

#define NT_DXE_TOTAL_DESC_NO 30 //Total Desc available for all channels
#define NT_MAX_STAGING_BUFFER_SIZE 2422 //Packet Staging buffer size (2304(NT_DPM_MAC_MTU_SIZE) + 14(ethernet_header_t) + 0x68(NT_TX_BUFFER_OFFSET))
#define NT_MAX_STAGING_BUFFER_SIZE_BA 3839
#define NT_MAX_STAGING_BUFFER_SIZE_AMSDU 7935

#define NT_DXE_CH_REG_SIZE        0x40

#define NT_DXE_CH_CTRL_REG        0x0000
#define NT_DXE_CH_STATUS_REG      0x0004
#define NT_DXE_CH_SZ_REG          0x0008
#define NT_DXE_CH_SADRL_REG       0x000C
#define NT_DXE_CH_SADRH_REG       0x0010
#define NT_DXE_CH_DADRL_REG       0x0014
#define NT_DXE_CH_DADRH_REG       0x0018
#define NT_DXE_CH_DESCL_REG       0x001C
#define NT_DXE_CH_DESCH_REG       0x0020
#define NT_DXE_CH_LST_DESCL_REG   0x0024
#define NT_DXE_CH_LST_DESCH_REG   0x0028
#define NT_DXE_CH_BD_REG          0x002C
#define NT_DXE_CH_HEAD_REG        0x0030
#define NT_DXE_CH_TAIL_REG        0x0034
#define NT_DXE_CH_PDU_REG         0x0038
#define NT_DXE_CH_TSTMP_REG       0x003C

#define NT_DXE_CH_TSTMP_H2B_TSTMP_OFFSET       8
#define NT_DXE_CH_TSTMP_B2H_TSTMP_OFFSET       10

// These are the original bit masks of DXE Descriptor Ctrl Word definition
#define NT_DXE_DESC_CTRL_VALID         0x00000001
#define NT_DXE_DESC_CTRL_XTYPE_MASK    0x00000006      // 0000-0110
#define NT_DXE_DESC_CTRL_XTYPE_H2H     0x00000000      // 0000-0000
#define NT_DXE_DESC_CTRL_XTYPE_B2B     0x00000002      // 0000-0010
#define NT_DXE_DESC_CTRL_XTYPE_H2B     0x00000004      // 0000-0100
#define NT_DXE_DESC_CTRL_XTYPE_B2H     0x00000006      // 0000-0110
#define NT_DXE_DESC_CTRL_EOP           0x00000008
#define NT_DXE_DESC_CTRL_BDH           0x00000010
#define NT_DXE_DESC_CTRL_SIQ           0x00000020
#define NT_DXE_DESC_CTRL_DIQ           0x00000040
#define NT_DXE_DESC_CTRL_PIQ           0x00000080
#define NT_DXE_DESC_CTRL_PDU_REL       0x00000100
#define NT_DXE_DESC_CTRL_BTHLD_SEL     0x00001E00
#define NT_DXE_DESC_CTRL_PRIO          0x0000E000
#define NT_DXE_DESC_CTRL_STOP          0x00010000
#define NT_DXE_DESC_CTRL_INT           0x00020000
#define NT_DXE_DESC_CTRL_BDT_IDX    	0x000c0000
#define NT_DXE_DESC_CTRL_BDT_SWAP   	0x00100000
#define NT_DXE_DESC_CTRL_ENDIANNESS   	0x00200000
#define NT_DXE_DESC_CTRL_RSVD          0xffc00000

#define NT_DXE_PHY_ADDR_MASK_UPPER_3BITS	0x1FFFFFFF

#define NT_SA_DXE_DESC_CTRL_VALID      0x00000001
#define NT_SA_DXE_DESC_CTRL_XTYPE_MASK 0x00000006      // 0000-0110
#define NT_SA_DXE_DESC_CTRL_XTYPE_H2H  0x00000000      // 0000-0000
#define NT_SA_DXE_DESC_CTRL_XTYPE_B2B  0x00000002      // 0000-0010
#define NT_SA_DXE_DESC_CTRL_XTYPE_H2B  0x00000004      // 0000-0100
#define NT_SA_DXE_DESC_CTRL_XTYPE_B2H  0x00000006      // 0000-0110
#define NT_SA_DXE_DESC_CTRL_EOP        0x00000008
#define NT_SA_DXE_DESC_CTRL_BDH        0x00000010
#define NT_SA_DXE_DESC_CTRL_SIQ        0x00000020
#define NT_SA_DXE_DESC_CTRL_DIQ        0x00000040
#define NT_SA_DXE_DESC_CTRL_PIQ        0x00000080
#define NT_SA_DXE_DESC_CTRL_PDU_REL    0x00000100
#define NT_SA_DXE_DESC_CTRL_BTHLD_SEL  0x00001E00
#define NT_SA_DXE_DESC_CTRL_PRIO       0x0000E000
#define NT_SA_DXE_DESC_CTRL_STOP       0x00010000
#define NT_SA_DXE_DESC_CTRL_INT        0x00020000
#define NT_SA_DXE_DESC_CTRL_BDT_IDX    0x000c0000
#define NT_SA_DXE_DESC_CTRL_BDT_SWAP   0x00100000
#define NT_SA_DXE_DESC_CTRL_ENDIANNESS 0x00200000
#define NT_SA_DXE_DESC_CTRL_RSVD       0xffc00000
// DXE Interrupts
#define ENABLE_DXE_IRQ 	0xFFE00000	  // Enable DXE 11 interrupts
#define ENABLE_DXE_IRQ1	0x00000001	  // Enable DXE 12th interrupt

/** DXE HW Long Descriptor format */
typedef struct DXELongDesc {
	uint32_t srcMemAddrL;
    uint32_t srcMemAddrH;
    uint32_t dstMemAddrL;
    uint32_t dstMemAddrH;
    uint32_t phyNextL;
    uint32_t phyNextH;
} DXELongDesc_t;

/** DXE HW Short Descriptor format */
typedef struct DXEShortDesc {
	uint32_t srcMemAddrL;
    uint32_t dstMemAddrL;
    uint32_t phyNextL;
} DXEShortDesc_t;

/** DXE HW Descriptor */
typedef struct DXEDesc {
    //DESC_CTRL
    uint32_t ctrl;
    uint32_t xfrSize;

    union {
    	DXELongDesc_t	dxe_long_desc;
    	DXEShortDesc_t	dxe_short_desc;
    }dxedesc;
} DXEDesc_t;

/*-------------------------------------------------------------------------
 * Preprocessor Definitions, Constants, and Type Declarations
 *-----------------------------------------------------------------------*/
#define RRAM_START                                           (0x00200000)

#define RRAM_END                                             (0x0037FFFF)
#define RRAM_SET_BUFFER_SIZE                                 (4096)

// write data to particular location and read from data particular location.
#define HAL_REG_WR(_reg, _val) *((volatile unsigned long*)(_reg)) = ((unsigned long)(_val))
#define HAL_REG_RD(_reg) *((volatile unsigned long*)(_reg))

/*-------------------------------------------------------------------------
 * Type Declarations
 *-----------------------------------------------------------------------*/

/*-------------------------------------------------------------------------
 * Function Declarations
 *-----------------------------------------------------------------------*/

void rram_dxe_init(void);
bl_error_type rram_verify_address(uint32_t address, uint32_t length);
bl_error_type rram_write_unaligned_dxe(uint32_t destination, const uint8_t *source, uint32_t *length);
bl_error_type rram_block_write_dxe(uint32_t destination, const uint8_t* source, uint32_t length);
bl_error_type rram_write_dxe(uint32_t destination, uint8_t* source, uint32_t length);
bl_error_type rram_memset_dxe(uint32_t address, int32_t value, uint32_t length);

typedef void (*rram_dxe_init_t)(void);
typedef bl_error_type (*rram_verify_address_t)(uint32_t address, uint32_t length);
typedef bl_error_type (*rram_write_unaligned_dxe_t)(uint32_t destination, const uint8_t *source, uint32_t *length);
typedef bl_error_type (*rram_block_write_dxe_t)(uint32_t destination, const uint8_t* source, uint32_t length);
typedef bl_error_type (*rram_write_dxe_t)(uint32_t destination, uint8_t* source, uint32_t length);
typedef bl_error_type (*rram_memset_dxe_t)(uint32_t address, int32_t value, uint32_t length);

typedef struct {
	rram_dxe_init_t				rram_dxe_init_pfn;
	rram_verify_address_t		rram_verify_address_pfn;
	rram_write_unaligned_dxe_t	rram_write_unaligned_dxe_pfn;
	rram_block_write_dxe_t		rram_block_write_dxe_pfn;
	rram_write_dxe_t			rram_write_dxe_pfn;
	rram_memset_dxe_t			rram_memset_dxe_pfn;
} nt_bl_rram_dxe_ind_t;

#endif
