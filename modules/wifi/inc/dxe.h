/*
Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
*/
#ifndef DXE_H_
#define DXE_H_

#include "stddef.h"
#include "dxe_api.h"
#include "nt_osal.h"

#define NT_DXE_TOTAL_DESC_NO 30  // Total Desc available for all channels
#define NT_MAX_STAGING_BUFFER_SIZE \
    2422  // Packet Staging buffer size (2304(NT_DPM_MAC_MTU_SIZE) + 14(ethernet_header_t) + 0x68(NT_TX_BUFFER_OFFSET))
#define NT_MAX_STAGING_BUFFER_SIZE_BA 3839
#define NT_MAX_STAGING_BUFFER_SIZE_AMSDU \
    3951  // Packet Staging buffer size (3839(AMSDU max size) + 36(mac header) + 0x4C(Rx buffer offset))

#define NT_DXE_CH_REG_SIZE 0x40

#define NT_DXE_CH_CTRL_REG      0x0000
#define NT_DXE_CH_STATUS_REG    0x0004
#define NT_DXE_CH_SZ_REG        0x0008
#define NT_DXE_CH_SADRL_REG     0x000C
#define NT_DXE_CH_SADRH_REG     0x0010
#define NT_DXE_CH_DADRL_REG     0x0014
#define NT_DXE_CH_DADRH_REG     0x0018
#define NT_DXE_CH_DESCL_REG     0x001C
#define NT_DXE_CH_DESCH_REG     0x0020
#define NT_DXE_CH_LST_DESCL_REG 0x0024
#define NT_DXE_CH_LST_DESCH_REG 0x0028
#define NT_DXE_CH_BD_REG        0x002C
#define NT_DXE_CH_HEAD_REG      0x0030
#define NT_DXE_CH_TAIL_REG      0x0034
#define NT_DXE_CH_PDU_REG       0x0038
#define NT_DXE_CH_TSTMP_REG     0x003C

#define NT_DXE_CH_TSTMP_H2B_TSTMP_OFFSET 8
#define NT_DXE_CH_TSTMP_B2H_TSTMP_OFFSET 10

// These are the original bit masks of DXE Descriptor Ctrl Word definition
#define NT_DXE_DESC_CTRL_VALID      0x00000001
#define NT_DXE_DESC_CTRL_XTYPE_MASK 0x00000006  // 0000-0110
#define NT_DXE_DESC_CTRL_XTYPE_H2H  0x00000000  // 0000-0000
#define NT_DXE_DESC_CTRL_XTYPE_B2B  0x00000002  // 0000-0010
#define NT_DXE_DESC_CTRL_XTYPE_H2B  0x00000004  // 0000-0100
#define NT_DXE_DESC_CTRL_XTYPE_B2H  0x00000006  // 0000-0110
#define NT_DXE_DESC_CTRL_EOP        0x00000008
#define NT_DXE_DESC_CTRL_BDH        0x00000010
#define NT_DXE_DESC_CTRL_SIQ        0x00000020
#define NT_DXE_DESC_CTRL_DIQ        0x00000040
#define NT_DXE_DESC_CTRL_PIQ        0x00000080
#define NT_DXE_DESC_CTRL_PDU_REL    0x00000100
#define NT_DXE_DESC_CTRL_BTHLD_SEL  0x00001E00
#define NT_DXE_DESC_CTRL_PRIO       0x0000E000
#define NT_DXE_DESC_CTRL_STOP       0x00010000
#define NT_DXE_DESC_CTRL_INT        0x00020000
#define NT_DXE_DESC_CTRL_BDT_IDX    0x000c0000
#define NT_DXE_DESC_CTRL_BDT_SWAP   0x00100000
#define NT_DXE_DESC_CTRL_ENDIANNESS 0x00200000
#define NT_DXE_DESC_CTRL_RSVD       0xffc00000

#define NT_DXE_PHY_ADDR_MASK_UPPER_3BITS 0x1FFFFFFF

#define NT_SA_DXE_DESC_CTRL_VALID      0x00000001
#define NT_SA_DXE_DESC_CTRL_XTYPE_MASK 0x00000006  // 0000-0110
#define NT_SA_DXE_DESC_CTRL_XTYPE_H2H  0x00000000  // 0000-0000
#define NT_SA_DXE_DESC_CTRL_XTYPE_B2B  0x00000002  // 0000-0010
#define NT_SA_DXE_DESC_CTRL_XTYPE_H2B  0x00000004  // 0000-0100
#define NT_SA_DXE_DESC_CTRL_XTYPE_B2H  0x00000006  // 0000-0110
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
#define NT_NVIC_ISER0   0xE000E100  // Irq 0 to 31 Set Enable Register
#define NT_NVIC_ISER1   0xE000E104  // Irq 32 to 60 Set Enable Register
#define ENABLE_DXE_IRQ  0xFFE00000  // Enable DXE 11 interrupts
#define ENABLE_DXE_IRQ1 0x00000001  // Enable DXE 12th interrupt

#define printf(...)

#define rWrite(reg, value)                     \
    (*((volatile uint32_t *)(reg))) = (value); \
    __asm volatile("dsb" ::: "memory")
#define rRead(reg)             \
    *(volatile uint32_t *)reg; \
    __asm volatile("dsb" ::: "memory")

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
    // DESC_CTRL
    uint32_t ctrl;
    uint32_t xfrSize;

    union {
        DXELongDesc_t dxe_long_desc;
        DXEShortDesc_t dxe_short_desc;
    } dxedesc;
} DXEDesc_t;

// Descriptor Control Block
typedef struct DescCB {
    struct DescCB *next;     // Next DCB pointer
    DXEDesc_t *DXEDescAddr;  // Memory pointer to reference the descriptor memory to set the fields
    uint32_t physDescAddr;   // This is the physical memory address of the descriptor, so we know what to plug into the
                             // descriptor address contents
    uint32_t StagingBuffer;  // Staging Buffer Physical Addr
} DescCB_t, *pDescCB;

typedef struct DxeCCB {
    DescCB_t *pRingFreeHead;  // Ring head for Ring based DXE decsritpor
    DescCB_t *pRingUsedHead;
    uint32_t noXfrDescUsed;  // No. of xfr desc in chain
    uint32_t nDescs;  // Number of URBs for USB or descriptors for DXE that can be queued for transfer at one time
    uint32_t refWQ;
    uint32_t bmuThdSel;
    uint32_t xfrType;
    uint32_t chDXEBaseAddr;
    uint32_t chPriority;
    uint8_t bdPresent;  // 1 = BD attached to the transfered frames
    uint8_t BDTXIdx;    // BD Template Index for H2B Transfer
    uint8_t chEnabled;
    uint8_t chConfigured;
    uint8_t buffer_type;
    uint32_t chk_size;
    e_dxe_channel channel;
    uint32_t chDXETimestampRegAddr;
    uint32_t chDXEStatusRegAddr;
    uint32_t chDXEDesclRegAddr;
    uint32_t chDXEDeschRegAddr;
    uint32_t chDXELstDesclRegAddr;
    uint32_t chDXECtrlRegAddr;
    uint32_t chDXESzRegAddr;
    uint32_t chDXESadrlRegAddr;
    uint32_t chDXESadrhRegAddr;
    uint32_t chDXEDadrlRegAddr;
    uint32_t chDXEDadrhRegAddr;
    uint32_t chk_size_mask;
    uint32_t bmuThdSel_mask;
    uint32_t cw_ctrl_b2h;
    uint32_t cw_ctrl_h2bh;
    uint32_t cw_ctrl_valid;
    uint32_t chan_mask;
    uint8_t use_short_desc_fmt;
    void (*cbfn)();
    uint32_t arg;
    uint32_t pkt_count;
    void *rx_buf;
    uint16_t int_cnt;
    uint16_t pkts_after_suspend;
} DxeCCB_t;

typedef struct DxeStats {
} hal_dxe_stats_t;

// DXE Global Control Block
typedef struct HalDxe {
    volatile DescCB_t *pFreeXfrDescPoolHead;  // Free global Xfr desc queue
    uint32_t pFreeDescPoolCount;
    volatile void *pDXEDescPool;
    volatile void *pXfrDescPool;
    volatile DxeCCB_t DxeCCB[DXE_CHANNEL_MAX];
    uint8_t Configured;
    uint8_t dxe_suspend;
} HalDxe_t, *pHalDxe;

#endif /* DXE_H_ */
