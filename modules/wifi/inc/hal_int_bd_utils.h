/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _HAL_INT_BD_UTILS_H_
#define _HAL_INT_BD_UTILS_H_

#include <stdint.h>

#include "hal_int_cfg.h"

#ifndef SW_ASSISTED_MODE
//#define SW_ASSISTED_MODE //Uncomment this to enable SW assist mode for Pronto 2.0
#endif

/*
 * #if !defined(__SOFTMAC_HOSTINTERFACE_INCLUDED) && !defined(__ASSEMBLER__)
 * #error "This header file is suppoed to be included by smacHostIf.h"
 * #endif
 */

#define CR6_UMA_FRAME_TRANSLATION
#define CR6_SEQUENCE_NUMBER_GENERATION
#define CR6_NEW_DPU_DESCRIPTOR
#define CR6_MANAGEMENT_FRAME_PROTECTION

/*
 * List of types
 */

struct hal_bd_generic_t;
struct hal_bd_rxp_rx_t; /* RXP => SoftMAC/RX */
struct hal_bd_rx_t;     /* SoftMAC/Rx => Host */
struct hal_bd_tx_t;     /* Host => SoftMAC/TX */
struct hal_bd_pdu_t;

/* size of the mac mgmt frame header:
 * frame control:2 + duration:2 + a1:6 + a2:6 + a3:6 + seqcontrol:2
 */
#define HAL_MAC_MGMT_HDR_LEN 24

/*
 * BD & PDU structure
 */

/* BD type */
#define HAL_HWBD_TYPE_GENERIC 0 /* generic BD format, use tSmacBdGeneric */
#define HAL_HWBD_TYPE_FRAG    1 /* fragmentation BD format, use tSmacBdDpuFrag*/

/* SW defined BD type, used in  tSmacBdRxpRx & tSmacBdHostTx*/
#define HAL_SWBD_TYPE_RXRAW  0  // RAW BD format from RxP, use tSmacBdRxpRx
#define HAL_SWBD_TYPE_DATA   1  // Rx/Tx Data BD, use tSmacBdHostRx
#define HAL_SWBD_TYPE_JUNK   2  // Frame to be dropped by Host directly.
#define HAL_SWBD_TYPE_CTLMSG 3  // BD for passing ctrl message between host/SoftMac.

/* Tx Ack Policy */
#define HAL_SWBD_ACKPOLICY_NORMAL 0 /* Normal ACK */
#define HAL_SWBD_ACKPOLICY_NO     1 /* No ACK */

/* SoftMac Rx response frame type */
#define HAL_SWBD_RXRESPONSE_UNDEFINED 0 /*Not applicable, may be in promisc mode,..., or unspecified by SoftMac*/
#define HAL_SWBD_RXRESPONSE_NONE      1 /*No response frame sent to TA*/
#define HAL_SWBD_RXRESPONSE_BACK      2 /*Blk ACK frame sent to TA after rcv this frame*/
#define HAL_SWBD_RXRESPONSE_NORMALACK 3 /*Normal ACK frame sent to TA after rcv this frame*/
#define HAL_SWBD_RXRESPONSE_NULL      4 /*NULL frame sent to TA after rcv this frame, maybe due to PM bit*/
#define HAL_SWBD_RXRESPONSE_PSPOLL    5 /*PS-Poll frame sent to TA after rcv this frame, maybe due to beacon TIM bit*/

/* SoftMac Rx reorder buffer */
#define HAL_SWBD_RXREORDER_OPCODE_INVALID 0
#define HAL_SWBD_RXREORDER_OPCODE_QCURRENT_FWDADV \
    1 /* Queue current packet, Fwd up to Head ptr if Head ptr advanced, then set new head ptr */
#define HAL_SWBD_RXREORDER_OPCODE_QCURRENT_FWD      4 /* Queue current packet, Fwd up to Head ptr, then set new head ptr */
#define HAL_SWBD_RXREORDER_OPCODE_FWD_QCURRENT      2 /* Fwd up to Head ptr, queue the new packet, then set new head ptr */
#define HAL_SWBD_RXREORDER_OPCODE_FWDALL_QCURRENT   3 /* Fwd all frmes, queue the new packet, then set new head ptr*/
#define HAL_SWBD_RXREORDER_OPCODE_FWDALL_FWDCURRENT 5 /* Fwd all frames then current frame, finally set new head ptr*/
#define HAL_SWBD_RXREORDER_OPCODE_FWDADV_DROPCURRENT \
    6 /* Forward up to Head prt if Head ptr advanced then set new head ptr. Drop current frame (ex: BAR)*/
#define HAL_SWBD_RXREORDER_OPCODE_FWDALL_DROPCURRENT \
    7 /* Fwd all frmes, then set new head ptr. Drop current frame (ex:BAR) */

/* Mac Protection */
#define HAL_SWBD_PROTECTION_DEFAULT     0  /* Default */
#define HAL_SWBD_PROTECTION_NO          1  /* No protection */
#define HAL_SWBD_PROTECTION_FORCERTS    2  /* RTS forced */
#define HAL_SWBD_PROTECTION_FORCECTS2S  3  /* CTS-to-self forced */
#define HAL_SWBD_PROTECTION_FORCE3CTS2S 10 /* CTS-to-self forced */

/* TX station index */
#define HAL_SWBD_TX_STAIDX_GROUP   255 /* raIndex : broadcast/multicast */
#define HAL_SWBD_TX_STAIDX_INVALID 254 /* taIndex & raIndex : invalid */

/* TX TID */
#define HAL_SWBD_TX_TID_MGMT_LOW    0 /* ProbeResponse */
#define HAL_SWBD_TX_TID_MGMT_HIGH   1 /* Other management frames */
#define HAL_SWBD_TX_TID_MGMT_LOWEST 2 /* SoftMAC internal */
#define HAL_SWBD_TX_TID_MGMT_NUM    3

/* TX BD structure */
#define HAL_SWBD_TX_MPDUHEADER_OFFSET 0x48 /* SoftMAC recommended MPDU header offset */

#ifdef CR6_SEQUENCE_NUMBER_GENERATION
#define HOST_GENERATE_SN         0
#define LEGACY_HW_GENERATE_SN    1
#define TID_BASED_HW_GENERATE_SN 2
#endif

/* TX MPDU data offset (FIXME) There's no fixed MPDU data offset */
#define HAL_SWBD_TX_MPDUDATA_OFFSET 128

/* RX BD structure */
#define HAL_SWBD_RX_MPDUHEADER_OFFSET 0x4c

/* RX station index */
#define HAL_HWBD_RX_UNKNOWN_UCAST 254
#define HAL_HWBD_RX_UNKNOWN_MCAST 255

/* Rate index */
#define HAL_SWBD_TX_RATEINDEX_DEFAULT 255

/* RxP status bits on RxBD
 * A-MPDU :
 *   First MPDU followed by more MPDUs : AMPDU | AMPDU_FIRST | PSDU_FIRST
 *   Middle of A-MPDU followed by more MPDUs : AMPDU | PSDU_FIRST
 *   Last MPDU : AMPDU | AMPDU_LAST | PSDU_FIRST
 * Concatenation :
 *   First PSDU followed by more PSDUs : APPDU | PSDU_FIRST
 *   Middle of concatenation followed by more PSDUs : APPDU
 *   Last PSDU : APPDU | PSDU_LAST
 */
#define HAL_HWBD_RXFLAG_FCS_EN          (1 << 22)
#define HAL_HWBD_RXFLAG_NAV_SET         (1 << 21)
#define HAL_HWBD_RXFLAG_NAV_CLEARED     (1 << 20)
#define HAL_HWBD_RXFLAG_TX_WARMEDUP     (1 << 19)
#define HAL_HWBD_RXFLAG_ADDR3_INVALID   (1 << 18)
#define HAL_HWBD_RXFLAG_ADDR2_INVALID   (1 << 17)
#define HAL_HWBD_RXFLAG_ADDR1_INVALID   (1 << 16)
#define HAL_HWBD_RXFLAG_PLCP_OVERRIDE   (1 << 15)
#define HAL_HWBD_RXFLAG_IS_AMPDU        (1 << 14)
#define HAL_HWBD_RXFLAG_IS_APPDU        (1 << 13)
#define HAL_HWBD_RXFLAG_IS_APPDU_LAST   (1 << 12) /* Last PSDU */
#define HAL_HWBD_RXFLAG_IS_APPDU_FIRST  (1 << 11) /* First PSDU */
#define HAL_HWBD_RXFLAG_IS_AMPDU_LAST   (1 << 10)
#define HAL_HWBD_RXFLAG_IS_AMPDU_FIRST  (1 << 9)
#define HAL_HWBD_RXFLAG_HAS_PHYCMD      (1 << 8)
#define HAL_HWBD_RXFLAG_HAS_PHYSTATS    (1 << 7)
#define HAL_HWBD_RXFLAG_HAS_DLM         (1 << 6)
#define HAL_HWBD_RXFLAG_BYPASS_DLMPROC  (1 << 5)
#define HAL_HWBD_RXFLAG_BYPASS_MPDUPROC (1 << 4)
#define HAL_HWBD_RXFLAG_FAILFILTER      (1 << 3)
#define HAL_HWBD_RXFLAG_FAILMAXPKTLEN   (1 << 2)
#define HAL_HWBD_RXFLAG_FCS_ERROR       (1 << 1)
#define HAL_HWBD_RXFLAG_EXCEPTION       (1 << 0)

/* DPU Error codes, as defined in 'DPU Error WQ CR' doc */
#define SWBD_DPUERR_INTERNAL_ERR  0
#define SWBD_DPUERR_BAD_TAG       1
#define SWBD_DPUERR_BAD_BD        2
#define SWBD_DPUERR_END_MARKER    3
#define SWBD_DPUERR_MAX_LENGTH    4
#define SWBD_DPUERR_FRAG_COUNT    5
#define SWBD_DPUERR_BAD_TKIP_MIC  6
#define SWBD_DPUERR_BAD_PPI       7
#define SWBD_DPUERR_BAD_DECRYPT   8
#define SWBD_DPUERR_ENVELOPE_ONLY 9
#define SWBD_DPUERR_ENVELOPE_PART 10
#define SWBD_DPUERR_ZERO_LENGTH   11
#define SWBD_DPUERR_BAD_EXTIV     12
#define SWBD_DPUERR_BAD_KID       13
#define SWBD_DPUERR_BAD_WEP_SEED  14
#define SWBD_DPUERR_UNPROTECTED   15
#define SWBD_DPUERR_PROTECTED     16
#define SWBD_DPUERR_BAD_REPLY     17
#define SWBD_DPUERR_REPLAY_WRAP   18

#define SWBD_DPUERR_MAXCODE 18

/*
 * DPU feedback on Rx (used in tSmacBdHostRx)
 *    0-63: Rx Success
 * 128-191: Rx Error
 */
#define HAL_DPU_RX_SUCCESS 0

#define HAL_DPU_RX_DECOMP_NOTREQUESTED  (HAL_DPU_RX_SUCCESS + 0) /*decomp not requested*/
#define HAL_DPU_RX_DECOMP_REQBUTSKIPPED (HAL_DPU_RX_SUCCESS + 1) /*decomp requested but not performed*/
#define HAL_DPU_RX_DECOMP_11PERCENT     (HAL_DPU_RX_SUCCESS + 2) /*decomp size up to 11%*/
#define HAL_DPU_RX_DECOMP_20PERCENT     (HAL_DPU_RX_SUCCESS + 3) /*decomp size up to 20%*/
#define HAL_DPU_RX_DECOMP_33PERCENT     (HAL_DPU_RX_SUCCESS + 4) /*decomp size up to 33%*/
#define HAL_DPU_RX_DECOMP_50PERCENT     (HAL_DPU_RX_SUCCESS + 5) /*decomp size up to 50%*/
#define HAL_DPU_RX_DECOMP_OVER50PERCENT (HAL_DPU_RX_SUCCESS + 6) /*decomp size > 50%*/
#define HAL_DPU_RX_DECOMP_SIZEEXCEED    (HAL_DPU_RX_SUCCESS + 7) /*decomp requested and result larger than original*/

#define HAL_DPU_RX_SUCCESS_MAXCODE (HAL_DPU_RX_DECOMP_SIZEEXCEED)

#define HAL_DPU_RX_ERR 128

#define HAL_DPU_RX_ERR_INTERNAL_ERR  (HAL_DPU_RX_ERR + SWBD_DPUERR_INTERNAL_ERR)
#define HAL_DPU_RX_ERR_BAD_TAG       (HAL_DPU_RX_ERR + SWBD_DPUERR_BAD_TAG)
#define HAL_DPU_RX_ERR_BAD_BD        (HAL_DPU_RX_ERR + SWBD_DPUERR_BAD_BD)
#define HAL_DPU_RX_ERR_END_MARKER    (HAL_DPU_RX_ERR + SWBD_DPUERR_END_MARKER)
#define HAL_DPU_RX_ERR_MAX_LENGTH    (HAL_DPU_RX_ERR + SWBD_DPUERR_MAX_LENGTH)
#define HAL_DPU_RX_ERR_FRAG_COUNT    (HAL_DPU_RX_ERR + SWBD_DPUERR_FRAG_COUNT)
#define HAL_DPU_RX_ERR_BAD_TKIP_MIC  (HAL_DPU_RX_ERR + SWBD_DPUERR_BAD_TKIP_MIC)
#define HAL_DPU_RX_ERR_BAD_PPI       (HAL_DPU_RX_ERR + SWBD_DPUERR_BAD_PPI)
#define HAL_DPU_RX_ERR_BAD_DECRYPT   (HAL_DPU_RX_ERR + SWBD_DPUERR_BAD_DECRYPT)
#define HAL_DPU_RX_ERR_ENVELOPE_ONLY (HAL_DPU_RX_ERR + SWBD_DPUERR_ENVELOPE_ONLY)
#define HAL_DPU_RX_ERR_ENVELOPE_PART (HAL_DPU_RX_ERR + SWBD_DPUERR_ENVELOPE_PART)
#define HAL_DPU_RX_ERR_ZERO_LENGTH   (HAL_DPU_RX_ERR + SWBD_DPUERR_ZERO_LENGTH)
#define HAL_DPU_RX_ERR_BAD_EXTIV     (HAL_DPU_RX_ERR + SWBD_DPUERR_BAD_EXTIV)
#define HAL_DPU_RX_ERR_BAD_KID       (HAL_DPU_RX_ERR + SWBD_DPUERR_BAD_KID)
#define HAL_DPU_RX_ERR_BAD_WEP_SEED  (HAL_DPU_RX_ERR + SWBD_DPUERR_BAD_WEP_SEED)
#define HAL_DPU_RX_ERR_UNPROTECTED   (HAL_DPU_RX_ERR + SWBD_DPUERR_UNPROTECTED)
#define HAL_DPU_RX_ERR_PROTECTED     (HAL_DPU_RX_ERR + SWBD_DPUERR_PROTECTED)
#define HAL_DPU_RX_ERR_BAD_REPLY     (HAL_DPU_RX_ERR + SWBD_DPUERR_BAD_REPLY)
#define HAL_DPU_RX_ERR_REPLAY_WRAP   (HAL_DPU_RX_ERR + SWBD_DPUERR_REPLAY_WRAP)

#define HAL_DPU_RX_ERR_MAXCODE (HAL_DPU_RX_ERR_REPLAY_WRAP)

/*
 * DPU feedback on Tx (used in tSmacBdHostTx)
 */

#define HAL_DPU_TX_SUCCESS 0x40
#define HAL_DPU_TX_ERR     0xc0

#define HAL_DPU_TX_ERR_MASK           0x80
#define HAL_DPU_TX_DIRECTION_MASK     0x40
#define HAL_DPU_TX_LAST_FRAGMENT_MASK 0x02
#define HAL_DPU_TX_FRAGMENT_MASK      0x01

#define HAL_DPU_TX_RESERVED_CODE    (HAL_DPU_TX_SUCCESS + 0)
#define HAL_DPU_TX_NONLAST_FRAGMENT (HAL_DPU_TX_SUCCESS + 1)
#define HAL_DPU_TX_NOT_FRAGMENT     (HAL_DPU_TX_SUCCESS + 2)
#define HAL_DPU_TX_LAST_FRAGMENT    (HAL_DPU_TX_SUCCESS + 3)

#define HAL_DPU_TX_SUCCESS_MAXCODE (HAL_DPU_TX_LAST_FRAGMENT)

#define HAL_DPU_TX_ERR_INTERNAL_ERR  (HAL_DPU_TX_ERR + SWBD_DPUERR_INTERNAL_ERR)
#define HAL_DPU_TX_ERR_BAD_TAG       (HAL_DPU_TX_ERR + SWBD_DPUERR_BAD_TAG)
#define HAL_DPU_TX_ERR_BAD_BD        (HAL_DPU_TX_ERR + SWBD_DPUERR_BAD_BD)
#define HAL_DPU_TX_ERR_END_MARKER    (HAL_DPU_TX_ERR + SWBD_DPUERR_END_MARKER)
#define HAL_DPU_TX_ERR_MAX_LENGTH    (HAL_DPU_TX_ERR + SWBD_DPUERR_MAX_LENGTH)
#define HAL_DPU_TX_ERR_FRAG_COUNT    (HAL_DPU_TX_ERR + SWBD_DPUERR_FRAG_COUNT)
#define HAL_DPU_TX_ERR_BAD_TKIP_MIC  (HAL_DPU_TX_ERR + SWBD_DPUERR_BAD_TKIP_MIC)
#define HAL_DPU_TX_ERR_BAD_PPI       (HAL_DPU_TX_ERR + SWBD_DPUERR_BAD_PPI)
#define HAL_DPU_TX_ERR_BAD_DECRYPT   (HAL_DPU_TX_ERR + SWBD_DPUERR_BAD_DECRYPT)
#define HAL_DPU_TX_ERR_ENVELOPE_ONLY (HAL_DPU_TX_ERR + SWBD_DPUERR_ENVELOPE_ONLY)
#define HAL_DPU_TX_ERR_ENVELOPE_PART (HAL_DPU_TX_ERR + SWBD_DPUERR_ENVELOPE_PART)
#define HAL_DPU_TX_ERR_ZERO_LENGTH   (HAL_DPU_TX_ERR + SWBD_DPUERR_ZERO_LENGTH)
#define HAL_DPU_TX_ERR_BAD_EXTIV     (HAL_DPU_TX_ERR + SWBD_DPUERR_BAD_EXTIV)
#define HAL_DPU_TX_ERR_BAD_KID       (HAL_DPU_TX_ERR + SWBD_DPUERR_BAD_KID)
#define HAL_DPU_TX_ERR_BAD_WEP_SEED  (HAL_DPU_TX_ERR + SWBD_DPUERR_BAD_WEP_SEED)
#define HAL_DPU_TX_ERR_UNPROTECTED   (HAL_DPU_TX_ERR + SWBD_DPUERR_UNPROTECTED)
#define HAL_DPU_TX_ERR_PROTECTED     (HAL_DPU_TX_ERR + SWBD_DPUERR_PROTECTED)
#define HAL_DPU_TX_ERR_BAD_REPLY     (HAL_DPU_TX_ERR + SWBD_DPUERR_BAD_REPLY)
#define HAL_DPU_TX_ERR_REPLAY_WRAP   (HAL_DPU_TX_ERR + SWBD_DPUERR_REPLAY_WRAP)

#define HAL_DPU_TX_ERR_MAXCODE (HAL_DPU_TX_ERR_REPLAY_WRAP)

/* SoftMac Rx drop reason
 *
 * - For BD structure type: tSmacBdDpuFrag(fragment BD), tSmacBdHostRx(SoftMac->Host BD),
 *   and tSmacBdRxpRx(RxP->SoftMac), if the frame was dropped by SoftMac, the drop reason
 *   in the feedbackInfo/dpuFeedback is defined here
 */

#define HAL_RX_DROP_BAD_BD                  1   // For softmac (linux) test platform, something wrong in the given BD
#define HAL_RX_DROP_RXPFILTERED             2   // rcvd frame went to _rxpException()
#define HAL_RX_DROP_CTS_UNEXPECT            3   // unexpected CTS directed to us received
#define HAL_RX_DROP_ACK_UNEXPECT            4   // unexpected ACK directed to us received
#define HAL_RX_DROP_BACK_UNEXPECT           5   // unexpected BLK ACK directed to us received
#define HAL_RX_DROP_PSPOLL_UNEXPECT         6   // PS-poll directed to us but we are in STA mode.
#define HAL_RX_DROP_BAD_BSSID               7   // BSS not valid or BSS index given is invalid
#define HAL_RX_DROP_BAD_TA                  8   // Unknown TA. Bypass BA processing
#define HAL_RX_DROP_BAD_RA                  9   // Unknown RA. No response frames go out.
#define HAL_RX_DROP_BACK_UNRECOGNIZED       10  // Non compressed BA received.
#define HAL_RX_DROP_BACK_DISABLED           11  // BAR received but BA not enabled on the TID
#define HAL_RX_DROP_DUPLICATE               12  // Frame is a duplicate and dropped.
#define HAL_RX_DROP_DUP_FRAG                13  // Frame is a duplicate fragment and dropped.
#define HAL_RX_DROP_OOO_FRAG                14  // Out of order fragment received. Drop the whole MSDU.
#define HAL_RX_DROP_COMPBA_FRAG             15  // Fragment should not be used on Compressed BA enabled TIDs
#define HAL_RX_DROP_FRAGMENT_NOBD           16  // No more BDs available to queue the fragment
#define HAL_RX_DROP_IGNORE_BEACON           17  // Bogus beacon or Host doesn't want to recv this beacon
#define HAL_RX_DROP_PWRSAVE_DISCARD         18  // Beacon or PS-poll dropped when STA in PwrSave mode.
#define HAL_RX_DROP_AMSDU_UNEXPECT          19  // unexpected AMSDU received
#define HAL_RX_DROP_FRAG_AMPDU              20  // Fragment seen in AMPDU. Dropped.
#define HAL_RX_DROP_HOST_NOT_READY          21  // For softmac testing only. Host not yet ready to recv any frame
#define HAL_RX_DROP_SCANMODE_FRAMES         22  // In scan mode, frames except beacon and probresp are dropped.
#define HAL_RX_DROP_SLOWSOFTMAC             23  // Frame dropped due to debug SOftmac too slow to catch up.
#define HAL_RX_DROP_BLACKHOLE_WQ            24  // For Softmac internal debugging only.
#define HAL_RX_DROP_AMSDU_BD_HAS_NO_HEADIDX 25  // BD's headIndex is 0. SHould not happen this is Taurus internal error.
#define HAL_RX_DROP_AMSDU_NO_ENOUGH_PDUS \
    26  // Not enough PDUs for remaining data bytes. SHould not happen. This is Taurus Internal Error
#define HAL_RX_DROP_AMSDU_SUBFRM_HDR_TOOSHORT 27  // Not enough bytes for next subframe header
#define HAL_RX_DROP_AMSDU_SUBFRM_OVERSIZE     28  // Len in subframe header is greater than remaining bytes in MPDU
#define HAL_RX_DROP_AMSDU_SUBFRM_NOBD         29  // No BD available for AMSDU
#define HAL_RX_DROP_OOO_MSDU                  30  // drop due to out of order MSDU
#define HAL_RX_DROP_REORDER_BADSEQ            31  // Bad seq # received when doing reordering
#define HAL_RX_DROP_SESSION_CACHE_DISABLED    32  // Drop due to session cache temporary disabled
#define HAL_RX_DROP_SEQFIFO_OUT_OF_SYNC       33  // critical: drop due to RxP seq FIFO out of sync
#define HAL_RX_DROP_UNKNOWN                   34
#define HAL_RX_MAX_DROP_REASON                35

#define HAL_RX_SUCCES_START            200  // Anything after this is SUCCESS case.
#define HAL_RX_SUCCESS_FRAGMENT_QUEUED 254  // SUCCESS case. Fragment is queued in SoftMac.
#define HAL_RX_SUCCESS_FRAGMENT_SENT   255  // SUCCESS case. All fragments are collected and sent to DPU.

/* SoftMac->Host RxReason codes */

/* Useful macros */
#define HAL_HWBD_TO_MPDUHEADER_OFFSET(pbd, offset) (struct sWlanBaseHdr *)((unsigned char *)(pbd) + (offset))
#define HAL_HWBD_TO_MPDUHEADER(pbd)                HAL_HWBD_TO_MPDUHEADER_OFFSET(pbd, pbd->mpduHeaderOffset)
#define HAL_HWBD_TO_MPDUDATA_OFFSET(pbd, offset)   (unsigned char *)((unsigned char *)(pbd) + (offset))
#define HAL_HWBD_TO_MPDUDATA(pbd)                  HAL_HWBD_TO_MPDUDATA_OFFSET(pbd, pbd->mpduDataOffset)

/*
 * tSmacBdGeneric: Generic BD format recognized by NOVA Hardware.
 * This is the generic structure defined in Guido's Nova MAC Arch document.
 *
 *  SoftMac shall use tSmacBdRxpRx for pkts from RxP and send to SoftMac Tx
 *  SoftMac would use tSmacBdHostRx to send pkt to Host
 *  Host should use tSmacBdHostTx to send pkt to SoftMac
 */

typedef struct hal_bd_generic_s {
/* 0x00 */
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t dpuRF : 8;
    uint32_t dpuSignature : 3; /* Signature on RA's DPU descriptor */
    uint32_t staSignature : 3;
    uint32_t swBdType : 2;
    uint32_t reserved1 : 12;
    uint32_t dpuNE : 1;
    uint32_t dpuNC : 1;
    uint32_t bdt : 2; /* BD type */
#else
    uint32_t bdt : 2;
    uint32_t dpuNC : 1;
    uint32_t dpuNE : 1;
    uint32_t reserved1 : 12;
    uint32_t swBdType : 2;
    uint32_t staSignature : 3;
    uint32_t dpuSignature : 3;
    uint32_t dpuRF : 8;
#endif

/* 0x04 */
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t reserved2 : 16;  /* reserved for BMU. To be used when BMU.control2.AMSDUrelease = 1 */
    uint32_t swReserved : 16; /* reserved for SW */
#else
    uint32_t swReserved : 16;
    uint32_t reserved2 : 16;
#endif

/* 0x08 */
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t headPduIdx : 16; /* Head PDU index */
    uint32_t tailPduIdx : 16; /* Tail PDU index */
#else
    uint32_t tailPduIdx : 16;
    uint32_t headPduIdx : 16;
#endif

/* 0x0c */
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t mpduHeaderLength : 8; /* MPDU header length */
    uint32_t mpduHeaderOffset : 8; /* MPDU header start offset */
    uint32_t mpduDataOffset : 9;   /* MPDU data start offset */
    uint32_t pduCount : 7;         /* PDU count */
#else
    uint32_t pduCount : 7;
    uint32_t mpduDataOffset : 9;
    uint32_t mpduHeaderOffset : 8;
    uint32_t mpduHeaderLength : 8;
#endif

/* 0x10 */
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t mpduLength : 16; /* MPDU length */
    uint32_t reserved3 : 4;   /* DPU compression feedback */
    uint32_t tid : 4;         /* Traffic identifier, tid */
    uint32_t rateIndex : 8;
#else
    uint32_t rateIndex : 8;
    uint32_t tid : 4;
    uint32_t reserved3 : 4;
    uint32_t mpduLength : 16;
#endif

/* 0x14 */
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t dpuDescIdx : 8;
    uint32_t addr1Index : 8;  // A1 index after RxP binary search
    uint32_t addr2Index : 8;  // A2 index after RxP binary search
    uint32_t addr3Index : 8;  // A3 index after RxP binary search
#else
    uint32_t addr3Index : 8;
    uint32_t addr2Index : 8;
    uint32_t addr1Index : 8;
    uint32_t dpuDescIdx : 8;
#endif
} hal_bd_generic_t;

/*
 * RX :
 */

/* tSmacBdRxpRx:
 *   Rx BD format used by only inside SoftMac
 *   Host Rx shall use tSmacBdHostRx, not this one.
 */

typedef struct hal_bd_rxp_rx_s {
/* 0x00 */
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t dpuRF : 8;      /* DPU routing flag */
    uint32_t reserved1 : 6;  /* reserved for SW */
    uint32_t swBdType : 2;   /* BD type set by software, HAL_SWBD_XXXX */
    uint32_t reserved2 : 12; /* reserved for SW */
    uint32_t dpuNE : 1;      /* DPU no encryption/decryption */
    uint32_t dpuNC : 1;      /* DPU no compressiong */
    uint32_t bdt : 2;        /* BD type */
#else
    uint32_t bdt : 2;
    uint32_t dpuNC : 1;
    uint32_t dpuNE : 1;
    uint32_t reserved2 : 12;
    uint32_t swBdType : 2;
    uint32_t reserved1 : 6;
    uint32_t dpuRF : 8;
#endif

/* 0x04 */
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t reserved3 : 16;   /* reserved for BMU */
    uint32_t smacPushWq : 8;   /* Used by SoftMac. WQ # which the frame is being pushed to after SoftMac Rx*/
    uint32_t feedbackInfo : 8; /* If smacPushWq=0xff(BMU Junk WQ, ie.HAL_BMUWQ_SINK), this is filled in by SoftMac,
                                * indicates SoftMac drop reason This field is set to 0xff by SoftMac before push to
                                * 'smacPushWq' in all other cases If smacPushWq=0x3(DPU RxWQ, ie.HAL_BMUWQ_DPU_RX), DPU
                                * would overwrite this byte with DPU Rx feedback code.
                                */
#else
    uint32_t feedbackInfo : 8;
    uint32_t smacPushWq : 8;
    uint32_t reserved3 : 16;
#endif

/* 0x08 */
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t headPduIdx : 16; /* Head PDU index */
    uint32_t tailPduIdx : 16; /* Tail PDU index */
#else
    uint32_t tailPduIdx : 16;
    uint32_t headPduIdx : 16;
#endif

/* 0x0c */
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t mpduHeaderLength : 8; /* MPDU header length */
    uint32_t mpduHeaderOffset : 8; /* MPDU header start offset */
    uint32_t mpduDataOffset : 9;   /* MPDU data start offset */
    uint32_t pduCount : 7;         /* PDU count */
#else
    uint32_t pduCount : 7;
    uint32_t mpduDataOffset : 9;
    uint32_t mpduHeaderOffset : 8;
    uint32_t mpduHeaderLength : 8;
#endif

/* 0x10 */
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t mpduLength : 16; /* MPDU length */
    uint32_t reserved4 : 4;   /* DPU compression feedback */
    uint32_t tid : 4;         /* Traffic identifier, tid */
    uint32_t rateIndex : 8;
#else
    uint32_t rateIndex : 8;
    uint32_t tid : 4;
    uint32_t reserved4 : 4;
    uint32_t mpduLength : 16;
#endif

/* 0x14 */
#ifdef HAL_CFG_BIGBYTE_ENDIAN

    uint32_t dpuDescIdx : 8; /* DPU descriptor index */
    uint32_t addr1Index : 8; /* binary & filter search result for Addr1 from RxP to SoftMac*/
    uint32_t addr2Index : 8; /* binary & filter search result for Addr2 from RxP to SoftMac*/
    uint32_t addr3Index : 8; /* binary & filter search result for Addr3 from RxP to SoftMac*/
#else
    uint32_t addr3Index : 8;
    uint32_t addr2Index : 8;
    uint32_t addr1Index : 8;
    uint32_t dpuDescIdx : 8;
#endif

    /* 0x18 */
    uint32_t rxpFlags; /* RxP flags*/

    /* 0x1c, 20 */
    uint32_t phyStats0; /* PHY status word 0*/
    uint32_t phyStats1; /* PHY status word 0*/

    /* 0x24 */
    uint32_t mclkRxTimestamp; /* Rx timestamp,  MAC clock count*/

    /* 0x28~0x3f */
    uint32_t rxPmiCmd[6]; /* PMI cmd rcvd from RxP */

} hal_bd_rxp_rx_t;

/* tSmacBdRxpRx:
 *   This is the data structure used by SoftMac to pass
 *   frames to Host
 */

// DavidLiu: PMI command dword count is defined as 6 now but actually
// RxP only passes 4 DWORD start from PMI cmd byte4. So we can
// shrink this from 6 to 4 later.
// Yuan: change it to 5 because last Dword is not entirely PMI command
#define RXP_PMI_CMD_DW_COUNT 6

typedef struct hal_bd_host_rx_s {
/* 0x00 */
#ifdef CR6_MANAGEMENT_FRAME_PROTECTION
#ifdef HAL_CFG_BIGBYTE_ENDIAN

    // DPU routing flag: Next WQ# for DPU to push to
    uint32_t dpuRF : 8;        /* DPU routing flag */
    uint32_t dpuSignature : 3; /* Signature on RA's DPU descriptor */
    uint32_t station_auth : 1;
    uint32_t miss_addr2_hit : 1;
    uint32_t frame_after_beacon : 1;
    uint32_t tsf_received : 1;

    uint32_t noValidHeader : 1;
    uint32_t csuVerifiedtcpUDPOrNot : 1;
    uint32_t csuVerifiedIpv46OrNot : 1;
    uint32_t csValid : 1;
    uint32_t tcpUDPCSError : 1;
    uint32_t ipCSError : 1;
    uint32_t llc : 1;
    uint32_t umabypass : 1;

    // BD type set by software, HAL_SWBD_XXXX
    // Normal frames use HAL_SWBD_TYPE_DATA.
    // Host messages use HAL_SWBD_TYPE_CTLMSG
    // Raw Pkt rcvd from RxP and frames rcvd during promisc mode use HAL_SWBD_TYPE_RXRAW
    //  uint32_t reserved1 : 8;

    /* Robust Management Frame */
    uint32_t rmf : 1;

    /* Broadcast/Multicast or Unicast */
    uint32_t ub : 1;

    uint32_t rxKeyId : 3; /* Key ID for DPU to decrypt */

    // For DPU:
    // Rx: if set, DPU bypasses decryption
    // Tx: If set, DPU bypasses encryption
    uint32_t dpuNE : 1;

    // For DPU:
    // Rx: if set, DPU bypasses decompression
    // Tx: If set, DPU bypasses compression
    uint32_t dpuNC : 1;

    // Normal frames use HAL_HWBD_TYPE_GENERIC
    // HAL_HWBD_TYPE_FRAG is only used between SoftMac Rx/DPU
    uint32_t bdt : 2; /* BD type */
#else
    uint32_t bdt : 2;
    uint32_t dpuNC : 1;
    uint32_t dpuNE : 1;
    uint32_t rxKeyId : 3;
    uint32_t ub : 1;
    uint32_t rmf : 1;
    uint32_t umabypass : 1;
    uint32_t llc : 1;
    uint32_t ipCSError : 1;
    uint32_t tcpUDPCSError : 1;
    uint32_t csValid : 1;
    uint32_t csuVerifiedIpv46OrNot : 1;
    uint32_t csuVerifiedtcpUDPOrNot : 1;
    uint32_t noValidHeader : 1;
    uint32_t tsf_received : 1;
    uint32_t frame_after_beacon : 1;
    uint32_t miss_addr2_hit : 1;
    uint32_t station_auth : 1;
    uint32_t dpuSignature : 3;
    uint32_t dpuRF : 8;
#endif
#else
#ifdef HAL_CFG_BIGBYTE_ENDIAN

    // DPU routing flag: Next WQ# for DPU to push to
    uint32_t dpuRF : 8;        /* DPU routing flag */
    uint32_t dpuSignature : 3; /* Signature on RA's DPU descriptor */
    uint32_t station_auth : 1;
    uint32_t miss_addr2_hit : 1;
    uint32_t frame_after_beacon : 1;
    uint32_t tsf_received : 1;

    uint32_t noValidHeader : 1;
    uint32_t csuVerifiedtcpUDPOrNot : 1;
    uint32_t csuVerifiedIpv46OrNot : 1;
    uint32_t csValid : 1;
    uint32_t tcpUDPCSError : 1;
    uint32_t ipCSError : 1;
    uint32_t llc : 1;
    uint32_t umabypass : 1;

    // BD type set by software, HAL_SWBD_XXXX
    // Normal frames use HAL_SWBD_TYPE_DATA.
    // Host messages use HAL_SWBD_TYPE_CTLMSG
    // Raw Pkt rcvd from RxP and frames rcvd during promisc mode use HAL_SWBD_TYPE_RXRAW
    uint32_t reserved1 : 3;
    uint32_t rxKeyId : 2; /* Key ID for DPU to decrypt */

    // For DPU:
    // Rx: if set, DPU bypasses decryption
    // Tx: If set, DPU bypasses encryption
    uint32_t dpuNE : 1;

    // For DPU:
    // Rx: if set, DPU bypasses decompression
    // Tx: If set, DPU bypasses compression
    uint32_t dpuNC : 1;

    // Normal frames use HAL_HWBD_TYPE_GENERIC
    // HAL_HWBD_TYPE_FRAG is only used between SoftMac Rx/DPU
    uint32_t bdt : 2; /* BD type */
#else
    uint32_t bdt : 2;
    uint32_t dpuNC : 1;
    uint32_t dpuNE : 1;
    uint32_t rxKeyId : 2;
    uint32_t reserved1 : 3;
    uint32_t umabypass : 1;
    uint32_t llc : 1;
    uint32_t ipCSError : 1;
    uint32_t tcpUDPCSError : 1;
    uint32_t csValid : 1;
    uint32_t csuVerifiedIpv46OrNot : 1;
    uint32_t csuVerifiedtcpUDPOrNot : 1;
    uint32_t noValidHeader : 1;
    uint32_t tsf_received : 1;
    uint32_t frame_after_beacon : 1;
    uint32_t miss_addr2_hit : 1;
    uint32_t station_auth : 1;
    uint32_t dpuSignature : 3;
    uint32_t dpuRF : 8;
#endif
#endif

/* 0x04 */
#ifdef HAL_CFG_BIGBYTE_ENDIAN

    // Used between SoftMac<->BMU only, (for AMSDU deaggregation)
    // HDD should not interpreted this field
    uint32_t penultimatePduIdx : 16;
    uint32_t smacPushWq : 8; /* WQ # to which the frame is being pushed to after SoftMac Rx. Used by SoftMac.*/

    uint32_t dpuFeedback : 8; /* If smacPushWq=0xff(BMU Junk WQ, ie.HAL_BMUWQ_SINK), this is filled in by SoftMac,
                               * indicates SoftMac drop reason This field is set to 0xff by SoftMac before push to
                               * 'smacPushWq' in all other cases. If smacPushWq=0x3(DPU RxWQ, ie.HAL_BMUWQ_DPU_RX), DPU
                               * would overwrite this byte with DPU Rx feedback code, see SWBD_DPURX_XXX defined above.
                               */
#else
    uint32_t dpuFeedback : 8;
    uint32_t smacPushWq : 8;
    uint32_t penultimatePduIdx : 16;
#endif

/* 0x08 */
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t headPduIdx : 16; /* Head PDU index */
    uint32_t tailPduIdx : 16; /* Tail PDU index */
#else
    uint32_t tailPduIdx : 16;
    uint32_t headPduIdx : 16;
#endif

/* 0x0c */
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    //
    // When isAmsduSubframe==1:
    //  if mpduHeaderOffset = mpduHeaderLength =0
    //     frame is first subframe with base WLAN header
    //  else
    //     frame is non-first subframe.
    //
    uint32_t mpduHeaderLength : 8; /* MPDU header length */
    uint32_t mpduHeaderOffset : 8; /* MPDU header start offset */
    uint32_t mpduDataOffset : 9;   /* MPDU data start offset */
    uint32_t pduCount : 7;         /* PDU count */
#else
    uint32_t pduCount : 7;
    uint32_t mpduDataOffset : 9;
    uint32_t mpduHeaderOffset : 8;
    uint32_t mpduHeaderLength : 8;
#endif

/* 0x10 */
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t mpduLength : 16; /* MPDU length */
    uint32_t reserved3 : 4;
    uint32_t tid : 4; /* Traffic identifier, tid */

    // Rate Index reported by RxP
    //  0-127: Titan Rates
    //  128-159: 11n MCS #0-15, 20Mhz
    //  160-191: 11n MCS #0-15, 40Mhz
    //  192-199: Titan legacy duplicate mode (6,9,12,...48,54Mbps)
    //  200-201: 11n MCS #32, 40Mhz duplicate mode
    //  204-207: Airgo 11n proprietary rates
    //  208-223: 11b rates, non duplicate
    //  224-239: 11b rate, duplicate mode
    uint32_t Rsvd_rateIndex : 8; /* Rate Index */
#else
    uint32_t Rsvd_rateIndex : 8;
    uint32_t tid : 4;
    uint32_t reserved3 : 4;
    uint32_t mpduLength : 16;
#endif

/* 0x14 */
#ifdef HAL_CFG_BIGBYTE_ENDIAN

    // DPU descriptor index, filled by SoftMac Rx for DPU
    uint32_t dpuDescIdx : 8;

    // When rxpFlags & HAL_HWBD_RXFLAG_ADDRx_INVALID (x=1,2,3) is NOT set,
    // corresponding addr index field is valid.
    // Value:
    //  0~253: valid
    //  254: unknown unicast
    //  255: unknown multicst and broadcast
    // When HAL_HWBD_RXFLAG_ADDRx_INVALID (x=1,2,3) is SET, corresponding
    // addrXIndex(X=1,2,3) values are invalid an are set to 253 by RxP.
    uint32_t addr1Index : 8; /* binary & filter search result for Addr1 from RxP*/
    uint32_t addr2Index : 8; /* binary & filter search result for Addr2 from RxP*/
    uint32_t addr3Index : 8; /* binary & filter search result for Addr3 from RxP*/
#else
    uint32_t addr3Index : 8;
    uint32_t addr2Index : 8;
    uint32_t addr1Index : 8;
    uint32_t dpuDescIdx : 8;
#endif

    /* 0x18, see HAL_HWBD_RXFLAG_XXXX */
    uint32_t rxpFlags : 23; /* RxP flags*/
    uint32_t rateIndex : 9;

    /* 0x1c, 20 */
    uint32_t phyStats0; /* PHY status word 0*/
    uint32_t phyStats1; /* PHY status word 0*/

    /* 0x24 */
    uint32_t mclkRxTimestamp; /* Rx timestamp, microsecond based*/

    /* 0x28~0x3f */
    uint32_t rxPmiCmd[RXP_PMI_CMD_DW_COUNT]; /* PMI cmd rcvd from RxP */

/* 0x40 */
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t reserved4 : 1;
    uint32_t rxReorderOpcode : 3;
    uint32_t ibssTsfLater : 1;  // 1 : rcvd beacon has TSF later than ours.
    uint32_t ibssBcnSent : 1;   // 1: we have sent beacon in this TBTT.
    uint32_t scanLearn : 1;     /* bit=1: Frame is rcvd during SCAN/Learn mode */
    // uint32_t isAmsduSubframe:1;  // If AMSDU decode is enabled and this bit set,
    // this frame is a deaggregated subframe in AMSDU */
    uint32_t rsvd : 1;

    uint32_t reason : 8; /* Reason code from SoftMac why the frame is passed to host */

    // Valid only when
    // 1. swBdType=HAL_SWBD_TYPE_DATA and
    // 2. SoftMac not in promisc mode and
    // 3. frame is QoS and the corresponding TID enabled BA
    //
    // For each <ta, ra, tid> tuple, host maintains an Rx reorder buffer,
    // for each Qos frame sent from SoftMac, Host checks the following two
    // index and take appropriate actions.
    //
    // rxReorderHeadIdx: Index in the Rx reorder buffer of the first MSDU to be indicated to Host
    // rxReorderEnqIdx: Index in the Rx reorder buffer where this frame should be queued
    //
    //
    //  rxReorderHeadIdx    rxReorderEnqIdx     Meaning
    //  ---------------------------------------------------------------
    //  0~(winSize-1)       0~(winSize-1)       Host enq the frame at slot #rxReorderEnqIdx then
    //                                          fwd all frames up to slot #rxReorderHeadIdx.
    //  winSize             0~(winSize-1)       Nothing to enq. Fwd all frames up to slot #rxReorderEnqIdx
    //                                          and make new head ptr=#rxReorderEnqIdx
    //  126                 0~(winSize-1)       Nothing to enq. Fwd all frames in queue.
    //                                          and make new head ptr=#rxReorderEnqIdx
    //  127                 don't care          Rxreorder indices are invalid. Don't use them.
    uint32_t rxReorderHeadIdx : 7;
    uint32_t rxReorderEnqIdx : 6;
    uint32_t respFrmType : 3;
#else
    uint32_t respFrmType : 3;
    uint32_t rxReorderEnqIdx : 6;
    uint32_t rxReorderHeadIdx : 7;
    uint32_t reason : 8;
    uint32_t rsvd : 1;
    uint32_t scanLearn : 1;
    uint32_t ibssBcnSent : 1;
    uint32_t ibssTsfLater : 1;
    uint32_t rxReorderOpcode : 3;
    uint32_t reserved4 : 1;
#endif

/* 0x44 */
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t reserved5 : 24; /** Changed from 11 */
    // value = cca counter between our TBTT and beacon received time.
    // this will be used only in IBSS case.
    // This value will be provided to host to calculate TBTT correction.
    uint32_t ccaCnt : 8;
#else
    uint32_t ccaCnt : 8;
    uint32_t reserved5 : 24;
#endif

/* 0x48 */
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t isAmsduSubframe : 1;
    uint32_t isFirstAmsduSubFrame : 1;
    uint32_t isLastAmsduSubFrame : 1;
    uint32_t isAmsduErrorFlag : 1;
    uint32_t reserved6 : 4; /** Changed from 11 */
    uint32_t processOrder : 4;
    uint32_t subFrameIndex : 4;
    uint32_t totalAmsduSize : 16;  // if isAmsduSubframe=1, this field keeps AMSDU MPDU length before deaggregation.
#else
    uint32_t totalAmsduSize : 16;
    uint32_t subFrameIndex : 4;
    uint32_t processOrder : 4;
    uint32_t reserved6 : 4;
    uint32_t isAmsduErrorFlag : 1;
    uint32_t isLastAmsduSubFrame : 1;
    uint32_t isFirstAmsduSubFrame : 1;
    uint32_t isAmsduSubframe : 1;
#endif

    // uint32_t smacProcessCpuCycles;

} hal_bd_host_rx_t;

/*
 * TX :
 */

/*
 * Use this data type to send ctrl messages via BD to SoftMac
 */
typedef struct hal_bd_host_ctl_msg_s {
/* 0x00 */
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t reserved1 : 14;
    uint32_t swBdType : 2;   /* Should be 11=HAL_SWBD_TYPE_CTLMSG */
    uint32_t reserved2 : 14; /* DPU no encryption/decryption */
    uint32_t bdt : 2;        /* Don't care since Host should Tx directly to mCPU */
#else
    uint32_t bdt : 2;
    uint32_t reserved2 : 14;
    uint32_t swBdType : 2;
    uint32_t reserved1 : 14;
#endif

/* 0x04 */
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t reserved3 : 24;
    uint32_t smacPushWq : 8;
#else
    uint32_t smacPushWq : 8;
    uint32_t reserved3 : 24;
#endif

/* 0x08 */
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t headPduIdx : 16; /* Head PDU index */
    uint32_t tailPduIdx : 16; /* Tail PDU index */
#else
    uint32_t tailPduIdx : 16;
    uint32_t headPduIdx : 16;
#endif

/* 0x0c */
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t mpduHeaderLength : 8; /* MPDU header length */
    uint32_t mpduHeaderOffset : 8; /* MPDU header start offset */
    uint32_t mpduDataOffset : 9;   /* MPDU data start offset */
    uint32_t pduCount : 7;         /* PDU count */
#else
    uint32_t pduCount : 7;
    uint32_t mpduDataOffset : 9;
    uint32_t mpduHeaderOffset : 8;
    uint32_t mpduHeaderLength : 8;
#endif

/* 0x10 */
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t mpduLength : 16; /* MPDU length */
    uint32_t reserved4 : 16;
#else
    uint32_t reserved4 : 16;
    uint32_t mpduLength : 16;
#endif

    // should now followed by a sSmacHostMesgHdr_XXX structure. See smacHostMesg.h
} hal_bd_host_ctl_msg_t;

/*
 * Use this data type to send normal frames to DPU/SoftMac
 */

typedef struct hal_bd_tx_s {
/* 0x00 */
#ifdef CR6_MANAGEMENT_FRAME_PROTECTION
#ifdef SW_ASSISTED_MODE
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    /* Routing flag. D/C in Tx processing.
     * To reach SoftMAC Tx code, host needs to set this routing flag to
     * one of mCPU Tx WQ between HAL_BMUWQ_MCPU_TX_WQ0 and HAL_BMUWQ_MCPU_TX_WQ5.
     * SoftMAC Tx code gives high priority to WQ with low index.
     */
    uint32_t dpuRF : 8;

    /* DPU signature. D/C in Tx processing */
    uint32_t dpuSignature : 3;

    /* Station signature. This is compared to station signature in station
     * configuration corresponding to raIndex. If raIndex is not a valid
     * station, that is INVALID or GROUP, station signature is not compared.
     * If signature check fails, the frame is dropped by default.
     */
    uint32_t staSignature : 1;

    uint32_t rtt3_tmf : 1;

    uint32_t ampdu_term : 1;

    /* HAL_SWBD_TYPE_XXX.
     * Valid types are as follow :
     *   HAL_SWBD_TYPE_DATA : This BD has data frame.
     *   HAL_SWBD_TYPE_CTLMSG : This BD has host interface message. (NOT READY)
     */
    uint32_t swBdType : 2;

    uint32_t reserved1 : 1;

    /*when csu software mode set to 1, software is responsible for parsing IPv4 or IPv6 headers, calculate
     *pseudo-header checksum and program the corresponding BD field, and indicate the location
     *of the start of TCP/UDP packet by programming the "TCP/UDP Start Offset" BD field
     */
    uint32_t csuSWMode : 1;

    /*Enable CSU in TX */
    uint32_t csuTxEnable : 1;

    /*Enable TCP UDP Checksum generation */
    uint32_t tcpUdpCSEnable : 1;

    /*Enable IPV4/IPV6 Checksum generation */
    uint32_t ipCSGenEnable : 1;

    /*TCP UDP Checksum Genration Status */
    uint32_t noTCPUDPCheckSumGenerated : 1;

    /*TCP UDP Checksum Genration Status */
    uint32_t noValHD : 1;

    /* Robust Management Frame */
    uint32_t rmf : 1;

    /* Broadcast/Multicast or Unicast */
    uint32_t ub : 1;

    uint32_t reserved1_2 : 1;

    /*these bits indicate TPE has to assert the TX complete interrupt*/
    uint32_t txCPLT : 2;

    /* No encryption. D/C in Tx processing */
    uint32_t dpuNE : 1;

    /*this bit indcates to ADU/UMA module that the packet requires
     *802.11n to 802.3 frame translation
     */
    uint32_t dpuNC : 1;

    /* BD type. Must be HAL_HWBD_TYPE_GENERIC */
    uint32_t bdt : 2;
#else
    uint32_t bdt : 2;
    uint32_t dpuNC : 1;
    uint32_t dpuNE : 1;
    uint32_t txCPLT : 2;
    uint32_t reserved1_2 : 1;
    uint32_t ub : 1;
    uint32_t rmf : 1;
    uint32_t noValHD : 1;
    uint32_t noTCPUDPCheckSumGenerated : 1;
    uint32_t ipCSGenEnable : 1;
    uint32_t tcpUdpCSEnable : 1;
    uint32_t csuTxEnable : 1;
    uint32_t csuSWMode : 1;
    uint32_t reserved1 : 1;
    uint32_t swBdType : 2;
    uint32_t ampdu_term : 1;
    uint32_t rtt3_tmf : 1;
    uint32_t staSignature : 1;
    uint32_t dpuSignature : 3;
    uint32_t dpuRF : 8;
#endif
#else
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    /* Routing flag. D/C in Tx processing.
     * To reach SoftMAC Tx code, host needs to set this routing flag to
     * one of mCPU Tx WQ between HAL_BMUWQ_MCPU_TX_WQ0 and HAL_BMUWQ_MCPU_TX_WQ5.
     * SoftMAC Tx code gives high priority to WQ with low index.
     */
    uint32_t dpuRF : 8;

    /* DPU signature. D/C in Tx processing */
    uint32_t dpuSignature : 3;

    /* Station signature. This is compared to station signature in station
     * configuration corresponding to raIndex. If raIndex is not a valid
     * station, that is INVALID or GROUP, station signature is not compared.
     * If signature check fails, the frame is dropped by default.
     */
    uint32_t staSignature : 3;

    /* HAL_SWBD_TYPE_XXX.
     * Valid types are as follow :
     *   HAL_SWBD_TYPE_DATA : This BD has data frame.
     *   HAL_SWBD_TYPE_CTLMSG : This BD has host interface message. (NOT READY)
     */
    uint32_t swBdType : 2;

    uint32_t reserved1 : 2;

    /*Enable CSU in TX */
    uint32_t csuTxEnable : 1;

    /*Enable TCP UDP Checksum generation */
    uint32_t tcpUdpCSEnable : 1;

    /*Enable IPV4/IPV6 Checksum generation */
    uint32_t ipCSGenEnable : 1;

    /*TCP UDP Checksum Genration Status */
    uint32_t noTCPUDPCheckSumGenerated : 1;

    /*TCP UDP Checksum Genration Status */
    uint32_t noValHD : 1;

    /* Robust Management Frame */
    uint32_t rmf : 1;

    /* Broadcast/Multicast or Unicast */
    uint32_t ub : 1;

    uint32_t reserved1_2 : 3;

    /* No encryption. D/C in Tx processing */
    uint32_t dpuNE : 1;

    /* No compression. D/C in Tx processing */
    uint32_t dpuNC : 1;

    /* BD type. Must be HAL_HWBD_TYPE_GENERIC */
    uint32_t bdt : 2;
#else
    uint32_t bdt : 2;
    uint32_t dpuNC : 1;
    uint32_t dpuNE : 1;
    uint32_t reserved1_2 : 3;
    uint32_t ub : 1;
    uint32_t rmf : 1;
    uint32_t noValHD : 1;
    uint32_t noTCPUDPCheckSumGenerated : 1;
    uint32_t ipCSGenEnable : 1;
    uint32_t tcpUdpCSEnable : 1;
    uint32_t csuTxEnable : 1;
    uint32_t reserved1 : 2;
    uint32_t swBdType : 2;
    uint32_t ampdu_term : 1;
    uint32_t rtt3_tmf : 1;
    uint32_t staSignature : 1;
    uint32_t dpuSignature : 3;
    uint32_t dpuRF : 8;
#endif
#endif
#else
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    /* Routing flag. D/C in Tx processing.
     * To reach SoftMAC Tx code, host needs to set this routing flag to
     * one of mCPU Tx WQ between HAL_BMUWQ_MCPU_TX_WQ0 and HAL_BMUWQ_MCPU_TX_WQ5.
     * SoftMAC Tx code gives high priority to WQ with low index.
     */
    uint32_t dpuRF : 8;

    /* DPU signature. D/C in Tx processing */
    uint32_t dpuSignature : 3;

    /* Station signature. This is compared to station signature in station
     * configuration corresponding to raIndex. If raIndex is not a valid
     * station, that is INVALID or GROUP, station signature is not compared.
     * If signature check fails, the frame is dropped by default.
     */
    uint32_t staSignature : 1;

    uint32_t rtt3_tmf : 1;

    uint32_t ampdu_term : 1;

    /* HAL_SWBD_TYPE_XXX.
     * Valid types are as follow :
     *   HAL_SWBD_TYPE_DATA : This BD has data frame.
     *   HAL_SWBD_TYPE_CTLMSG : This BD has host interface message. (NOT READY)
     */
    uint32_t swBdType : 2;

    uint32_t reserved1 : 7;

    /*Enable CSU in TX */
    uint32_t csuTxEnable : 1;

    /*Enable TCP UDP Checksum generation */
    uint32_t tcpUdpCSEnable : 1;

    /*Enable IPV4/IPV6 Checksum generation */
    uint32_t ipCSGenEnable : 1;

    /*TCP UDP Checksum Genration Status */
    uint32_t noTCPUDPCheckSumGenerated : 1;

    /*TCP UDP Checksum Genration Status */
    uint32_t noValHD : 1;

    /* No encryption. D/C in Tx processing */
    uint32_t dpuNE : 1;

    /* No compression. D/C in Tx processing */
    uint32_t dpuNC : 1;

    /* BD type. Must be HAL_HWBD_TYPE_GENERIC */
    uint32_t bdt : 2;
#else
    uint32_t bdt : 2;
    uint32_t dpuNC : 1;
    uint32_t dpuNE : 1;
    uint32_t noValHD : 1;
    uint32_t noTCPUDPCheckSumGenerated : 1;
    uint32_t ipCSGenEnable : 1;
    uint32_t tcpUdpCSEnable : 1;
    uint32_t csuTxEnable : 1;
    uint32_t reserved1 : 7;
    uint32_t swBdType : 2;
    uint32_t staSignature : 3;
    uint32_t dpuSignature : 3;
    uint32_t dpuRF : 8;
#endif
#endif

/* 0x04 */
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    /* This 16 bits has special meaning for BMU when A-MSDU feature is enabled.
     * This field should be set to 0 always.
     */
    uint32_t reservedForBmu : 16;
    uint32_t reserved2 : 8;

    /* DPU feedback in Tx path. This should be fixed after hardware fix */
    uint32_t dpuFeedback : 8; /* SEE SWBD_DPUTX_XXX defined above */

#else
    uint32_t dpuFeedback : 8;
    uint32_t reserved2 : 8;
    uint32_t reservedForBmu : 16;
#endif

/* 0x08 */
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    /* head and tail PDU indices are filled by DXE. */
    uint32_t headPduIdx : 16;
    uint32_t tailPduIdx : 16;
#else
    uint32_t tailPduIdx : 16;
    uint32_t headPduIdx : 16;
#endif

/* 0x0c */
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    /* MPDU header length */
    uint32_t mpduHeaderLength : 8;

    /* MPDU header offset. Recommended value is HAL_SWBD_TX_MPDUHEADER_OFFSET.
     * This must be multiple of 4. softmac expects all 802.11 frame header falls
     * in first BD. That is, mpduHeaderOffset + mpduHeaderLength <= BMU_BD_SIZE.
     */
    uint32_t mpduHeaderOffset : 8;

    /* MPDU data offset. SoftMAC recommends MPDU data follows MPDU header
     * without gap. However, MPDU data can start any offset, as softmac does
     * not need to access payload.
     */
    uint32_t mpduDataOffset : 9;

    /* PDU count is filled by DXE */
    uint32_t pduCount : 7;
#else
    uint32_t pduCount : 7;
    uint32_t mpduDataOffset : 9;
    uint32_t mpduHeaderOffset : 8;
    uint32_t mpduHeaderLength : 8;
#endif

/* 0x10 */
#ifdef CR6_SEQUENCE_NUMBER_GENERATION
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    /* MPDU length. This covers MPDU header length + MPDU data length. This does not
     * include FCS. For single frame transmission, PSDU size is mpduLength + 4.
     */
    uint32_t mpduLength : 16;

    uint32_t reserved3 : 2;

    /* Gen6 Squence Number Generation indicator:
     * 0 - Host generates sequence number
     * 1 - Turns on legacy hardware sequence number generation
     * 2 - Turns on TID based hardware sequence number generation
     * 3 - Reserved
     */
    uint32_t bdSsn : 2;

    /* Traffic identifier.
     * Group frame : Priority. Valid range is 0..7
     * Management frame : Priority. Valid values are HAL_SWBD_TX_TID_MGMT_LOW (0) or
     *   HAL_SWBD_TX_TID_MGMT_HIGH (1). Other values are not valid.
     * Data frame : TID. Valid range is 0..7.
     */
    uint32_t tid : 4;

    /* Rate index to use. HAL_SWBD_TX_RATEINDEX_DEFAULT to use default rate
     * other than rate specified by this field.
     *
     * The rate decision is as follows :
     *
     * Group :
     * - if rateIndex is default, use global non-beacon rate.
     * - if rateIndex is not default, use specified rate.
     * Management :
     * - If rateIndex is default, use same rate with data frame.
     * - if rateIndex is not default, use specified rate.
     * Data :
     * - If rateIndex is default, choose a rate among three rates in station
     *   configuration based on retry count.
     * - If rateIndex is not default, use specified rate. In this case,
     *   A-MPDU and concatenation can't aggregate other frame. If enableAggregate
     *   bit is 1, this frame is sent as single frame. If it is cleared, this
     *   frame is sent as A-MPDU frame.
     *
     * For management, rateIndex must be provided if raIndex is not valid.
     * This may happen in scan mode in which no station is registered as recipient.
     */
    uint32_t rateIndex : 8;
#else
    uint32_t rateIndex : 8;
    uint32_t tid : 4;
    uint32_t bdSsn : 2;
    uint32_t reserved3 : 2;
    uint32_t mpduLength : 16;
#endif
#else
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    /* MPDU length. This covers MPDU header length + MPDU data length. This does not
     * include FCS. For single frame transmission, PSDU size is mpduLength + 4.
     */
    uint32_t mpduLength : 16;

    uint32_t reserved3 : 4;

    /* Traffic identifier.
     * Group frame : Priority. Valid range is 0..7
     * Management frame : Priority. Valid values are HAL_SWBD_TX_TID_MGMT_LOW (0) or
     *   HAL_SWBD_TX_TID_MGMT_HIGH (1). Other values are not valid.
     * Data frame : TID. Valid range is 0..7.
     */
    uint32_t tid : 4;

    /* Rate index to use. HAL_SWBD_TX_RATEINDEX_DEFAULT to use default rate
     * other than rate specified by this field.
     *
     * The rate decision is as follows :
     *
     * Group :
     * - if rateIndex is default, use global non-beacon rate.
     * - if rateIndex is not default, use specified rate.
     * Management :
     * - If rateIndex is default, use same rate with data frame.
     * - if rateIndex is not default, use specified rate.
     * Data :
     * - If rateIndex is default, choose a rate among three rates in station
     *   configuration based on retry count.
     * - If rateIndex is not default, use specified rate. In this case,
     *   A-MPDU and concatenation can't aggregate other frame. If enableAggregate
     *   bit is 1, this frame is sent as single frame. If it is cleared, this
     *   frame is sent as A-MPDU frame.
     *
     * For management, rateIndex must be provided if raIndex is not valid.
     * This may happen in scan mode in which no station is registered as recipient.
     */
    uint32_t rateIndex : 8;
#else
    uint32_t rateIndex : 8;
    uint32_t tid : 4;
    uint32_t reserved3 : 4;
    uint32_t mpduLength : 16;
#endif
#endif

/* 0x14 */
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    /* Used by DPU. d/c in Tx processing */
    uint32_t dpuDescIdx : 8;

    /* Recipient station index.
     * Valid station index in 0-253 : Recipient station index.
     * HAL_SWBD_TX_STAIDX_INVALID (254) : Recipient is unknown. rateIndex must not be default.
     * HAL_SWBD_TX_STAIDX_GROUP (255) : Recipient is broadcast/multicast.
     */
    uint32_t raIndex : 8;

    /*
     * ACK Policy Override - this may turn out to be unused since TPE does not support it.
     * The ACK policy will be picked up from the BD only if the ACK Policy Override bit is set.
     */
    uint32_t ackPolicy : 2;

    uint32_t reserved4 : 1;

    uint32_t ackPolicyOverride : 1;

    /*
     * BTQM Tx WQ-id
     */
    uint32_t txWqId : 5;

    uint32_t reserved7 : 7;

#else
    uint32_t reserved7 : 7;
    uint32_t txWqId : 5;
    uint32_t ackPolicyOverride : 1;
    uint32_t reserved4 : 1;
    uint32_t ackPolicy : 2;
    uint32_t raIndex : 8;
    uint32_t dpuDescIdx : 8;
#endif

/* 0x18 */
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t reserved5 : 21;

    /* CONFIG_CAP_DIAG only
     * Use FCS passed from host. With this flag set, softmac doesn't rely on TXP for
     * generating FCS. Host passes 4 byte FCS following MPDU data, then softmac uses
     * that 4 byte as FCS. MPDU length doesn't include this 4 byte FCS. Host needs to
     * program DXE to transfer that extra 4 byte FCS.
     */
    uint32_t useHostGeneratedFcs : 1;

    /* CONFIG_CAP_DIAG only
     * Do not check the validity of MAC header. For example, queued frame can have
     * wrong frame type, but it won't cause the frame to be dropped by wrong frame type.
     * This bit is used to transmit a frame with errors, and to exercise our system is
     * tolerant from the error. If softmac detects undefined type, it treats it as
     * normal data frame.
     */
    uint32_t bypassMacHeaderCheck : 1;

    /* CONFIG_CAP_DIAG only
     * Do not overwrite seqnum number. This is only for diag program,
     * which generates sequence number of all frames, and verify the
     * sequence number.
     */
    uint32_t keepSeqNum : 1;

    /* FUTURE USE (NOT READY)
     * Transmission indication is sent to host after frame is transmitted
     * or dropped.
     */
    uint32_t txIndication : 1;

    /* ACK Policy : HAL_SWBD_ACKPOLICY_xxx
     * This specifies the ACK policy for data frame. Group and management
     * frame is not affected by this flag. That is,
     *
     * Group (broadcast/multicast) : d/c, No Ack always
     * Management (unicast) : d/c, Ack ways.
     * Data (unicast) : If ACK policy is set to default or normal,
     *   then SoftMAC waits for ACK or BA response after sedning packets.
     *   If this is set to NO ACK, then SoftMAC doesn't wait for a response
     *   frame and discards the packet. This should match with Ack Policy
     *   in QoS control field. SoftMAC doesn't look at frame header to find
     *   Ack policy.
     *
     * SoftMAC does not aggregate frames with different ACK policy in
     * one A-MPDU.
     */
    uint32_t forceAckPolicy : 3;

    /* CONFIG_CAP_DIAG only
     * Force specific MAC protection (HAL_SWBD_PROTECTION_xxx). This override
     * any decision made by SoftMAC.
     *
     * Default : Let SoftMAC decide
     * No : Does not use protection
     * Force RTS : Use RTS for this packet.
     * Force CTS2S : Use CTS2S for this packet.
     *
     * This can be also used with aggregation. Packets with same protection
     * bit can be aggregated together.
     */
    uint32_t forceMacProt : 2;

    /* CONFIG_CAP_DIAG only
     * Overwrite PHY command. SoftMAC will use MPI command embedded in
     * this BD. This MPI command includes MPDU length and PSDU length.
     * Host is responsible for filling everything, since softmac transmits
     * the MPI command "as is". That is, softmac won't touch any field
     * including PSDU length.
     *
     * A packet with overwritePhy bit set is sent as single frame always.
     * This is not available with A-MPDU or concatenation.
     */
    uint32_t overwritePhy : 1;

    /* CONFIG_CAP_DIAG only
     * Enable aggregate. For Titan STAs, it's concat. For 11n STAs, it's A-MPDU.
     * This bit has meaning only for a rate with A-MPDU or CONCAT flag set.
     *
     * 1 (default) : Normal aggregation. Frames are aggregated up to limit. If there is only
     * one frame to transmit, then Tx code sends the frame as single frame, not as
     * A-MPDU or concatenation. If DIAG feature is not enabled, Tx code assumes that
     * aggregation is enabled always.
     *
     * 0 : Don't aggregate but send frame as aggregated format. That is, Tx code
     * sends frame in A-MPDU with single MPDU or concatenation with one PSDU.
     */
    uint32_t enableAggregate : 1;
#else
    uint32_t enableAggregate : 1;
    uint32_t overwritePhy : 1;
    uint32_t forceMacProt : 2;
    uint32_t forceAckPolicy : 3;
    uint32_t txIndication : 1;
    uint32_t keepSeqNum : 1;
    uint32_t bypassMacHeaderCheck : 1;
    uint32_t useHostGeneratedFcs : 1;
    uint32_t reserved5 : 21;
#endif

    /* 0x1c */
    uint32_t reserved6;

    /* 0x20 */
    /* (NOTE) This field is only for SoftMAC */
    /* Timestamp filled by DXE. Timestamp for current transfer */
    uint32_t dxeH2BStartTimestamp;

    /* 0x24 */
    /* (NOTE) This field is only for SoftMAC */
    /* Timestamp filled by DXE. Timestamp for previous transfer */
    uint32_t dxeH2BEndTimestamp;

/* 0x28 */
#ifdef SW_ASSISTED_MODE
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t TcpUdpStartOffset : 16;
    uint32_t TcpUdpChecksumOffset : 5;
    uint32_t reserved8 : 11;
#else
    uint32_t reserved8 : 11;
    uint32_t TcpUdpChecksumOffset : 5;
    uint32_t TcpUdpStartOffset : 16;
#endif
#else
    /* (NOTE) DO NOT USE until defined. */
    /* (NOTE) This field is only for SoftMAC */
    uint32_t dxeB2HStartTimestamp;
#endif

/* 0x2C */
#ifdef SW_ASSISTED_MODE
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    /*TCP/UDP Packet Byte Length*/
    uint32_t TcpUdpByteLength : 16;
    uint32_t reserved9 : 16;
#else
    uint32_t reserved9 : 16;
    uint32_t TcpUdpByteLength : 16;
#endif
#else
    /* (NOTE) DO NOT USE until defined. */
    /* (NOTE) This field is only for SoftMAC */
    uint32_t dxeB2HEndTimestamp;
#endif

    /* 0x30 */
    uint32_t swTimestamp;

    /* 0x34 - 0x47 */
    /* Reserved for MPI command when overwritePhy bit is set.
     * Basic MPI command consists of 11 bytes.
     * If PLCP override bit is on, 6 more bytes are needed.
     */
    uint8_t phyCmds[20];
} hal_bd_tx_t;

/* DPU defragmentation BD type. Should only be used on the Rx path. */
#define HAL_DEFRAGBD_MAX_FRAG_NUM 16

typedef struct hal_bd_dpu_frag_s {
    uint32_t bdt; /* Only LSB 2 bits are in use now.
                   *  should be =HAL_HWBD_TYPE_FRAG1
                   */
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t resv : 16; /* reserved for BMU. To be used when BMU.control2.AMSDUrelease=1 */
    uint32_t
        smacPushWq : 8; /* For SW debug purpose. This should be set to HAL_BMUWQ_DPU_RX by Softmac if pushed to DPU*/
    /* If value is HAL_BMUWQ_SINK, it means SoftMac gave up defragmentation and the drop reason is given in
     * 'dpuFeedback' field.*/

    uint32_t dpuFeedback : 8; /* If smacPushWq=0xff(BMU Junk WQ, ie.HAL_BMUWQ_SINK), this is filled in by SoftMac,
                               * indicates SoftMac drop reason This field is set to 0xff by SoftMac before push to
                               * 'smacPushWq' in all other cases. If smacPushWq=0x3(DPU RxWQ, ie.HAL_BMUWQ_DPU_RX), DPU
                               * would overwrite this byte with DPU Rx feedback code, see SWBD_DPURX_XXX defined above.
                               */
#else
    uint32_t dpuFeedback : 8;
    uint32_t smacPushWq : 8;
    uint32_t resv : 16;
#endif

    uint32_t fragIdx[HAL_DEFRAGBD_MAX_FRAG_NUM]; /* LSB 16 bits */

} hal_bd_dpu_frag_t;

/*
 * PDU without BD
 */

typedef struct hal_bd_pdu_s {
    uint8_t payload[124];
    uint32_t nextPduIdx; /* LSB 16 bits */
} hal_bd_pdu_t;

#ifdef HAL_CFG_BIGBYTE_ENDIAN

#define SET_DPU_RCIDX(dpuDesc, tid, value)                                                     \
    do {                                                                                       \
        ((uint8_t *)((tNovaDpuDescriptor *)(dpuDesc))->idxPerTidReplayCount)[(tid)] = (value); \
    } while (0)
#define GET_DPU_RCIDX(dpuDesc, tid) (((uint8_t *)(dpuDesc)->idxPerTidReplayCount)[(tid)])
#else
#define RCIDX_TO_BYTEIDX(tid) (((tid) & ~3) | (3 - ((tid)&3)))
#define SET_DPU_RCIDX(dpuDesc, tid, value)                                                                     \
    do {                                                                                                       \
        ((uint8_t *)((tNovaDpuDescriptor *)(dpuDesc))->idxPerTidReplayCount)[RCIDX_TO_BYTEIDX(tid)] = (value); \
    } while (0)
#define GET_DPU_RCIDX(dpuDesc, tid) (((uint8_t *)(dpuDesc)->idxPerTidReplayCount)[RCIDX_TO_BYTEIDX(tid)])
#endif

/*enum {
    HAL_DPU_ENC_NONE = 0,
    HAL_DPU_ENC_WEP64,
    HAL_DPU_ENC_WEP104,
    HAL_DPU_ENC_TKIP,
    HAL_DPU_ENC_AES
};*/

typedef struct hal_nova_dpu_desc_s {
// word 0
#ifdef HAL_CFG_BIGBYTE_ENDIAN

    // PPI: per pkt indication for compression status
    //  Tx:
    //     if(BD.NC=0 && DPUdesc.enablePerTidComp[tid]=1)
    //        compressed = 1
    //     if(DPUdesc.PPI=1){
    //        insert 1 byte before MSDU
    //        if(compressed)
    //           PPI.CD=1
    //     }
    //  Rx:
    //     if(DPUdesc.PPI=1){
    //       if(PPI.CD=1)
    //          Do decompression, remove PPI byte
    //     else if(DPUdesc.enablePerTidDecomp[tid]=1)
    //          Do decompression

    uint32_t ppi : 1;

    // PLI: if encMode is AES and PLI=1, DPU always put 2 bytes before the IV field.
    //     PLI bit would be ignored if encMode is not AES.
    //     When PLI=1, it helps accelerate DPU Rx for AES decryption.
    //  Tx:
    //     if(DPUdesc.AES=1){
    //       if(PPI = 1 && compressed=1)
    //          insert 2 bytes before IV to indicate Pkt len
    //       else if(pktlen>4K)
    //          force framentation (regardless of DPUdesc.fragthreshold)
    //       }
    //       do AES encryption
    //     }
    //  Rx:
    //     if(DPUdesc.AES =1){
    //       if(PLI=1)
    //           Pktlen = PLI.pktlen
    //       else{
    //           Pktlen = queue pkt to internal buffer(up to 4K)
    //           and count bytes
    //       }
    //       do AES decryption
    //     }
    uint32_t pli : 1;

    uint32_t resv1 : 2;
    uint32_t txFragThreshold4B : 12; /* in units of 4Bytes*/
    uint32_t resv2 : 7;
    uint32_t signature : 3;
    uint32_t resv3 : 3;
    uint32_t staId : 3;
#else
    uint32_t staId : 3;
    uint32_t resv3 : 3;
    uint32_t signature : 3;
    uint32_t resv2 : 7;
    uint32_t txFragThreshold4B : 12;
    uint32_t resv1 : 2;
    uint32_t pli : 1;
    uint32_t ppi : 1;
#endif

// word 1
#ifdef CR6_NEW_DPU_DESCRIPTOR
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t keyIndex_5d : 8;
    uint32_t keyIndex_4d : 8;
    uint32_t resv4 : 16;
#else
    uint32_t resv4 : 16;
    uint32_t keyIndex_4d : 8;
    uint32_t keyIndex_5d : 8;
#endif

// word 2
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    /* keyIndex_5 to keyIndex_0 are key desc idx used by DPU to
     *   - encrypt the frame if encMode is WEP/AES/TKIP
     *   - decrypt the frame if encMode is AES/TKIP
     *     (for WEP, DPU uses BD.rxKeyID (filled by SOftMac Rx)
     *     as index to pick the right Key desc idx from wepRxKeyIdx0~3
     */
    uint32_t keyIndex_3 : 8;
    uint32_t keyIndex_2 : 8;
    uint32_t keyIndex_1 : 8;
    uint32_t keyIndex_0 : 8;
#else
    uint32_t keyIndex_0 : 8;
    uint32_t keyIndex_1 : 8;
    uint32_t keyIndex_2 : 8;
    uint32_t keyIndex_3 : 8;
#endif

// word 3
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t resv5 : 2;
    uint32_t str : 1;
    uint32_t kb : 1;
    uint32_t replayCountSet : 4;

    uint32_t keyIndex_5 : 8;
    uint32_t keyIndex_4 : 8;

    /* this is the key id to be filled in IV when encMode is not OPEN
     */
    uint32_t txKeyId : 3;
    uint32_t resv6 : 2;
    uint32_t encryptMode : 3;
#else
    uint32_t encryptMode : 3;
    uint32_t resv6 : 2;
    uint32_t txKeyId : 3;
    uint32_t keyIndex_4 : 8;
    uint32_t keyIndex_5 : 8;
    uint32_t replayCountSet : 4;
    uint32_t kb : 1;
    uint32_t str : 1;
    uint32_t resv5 : 2;
#endif
#else
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t enablePerTidDecomp : 16;
    uint32_t enablePerTidComp : 16;
#else
    uint32_t enablePerTidComp : 16;
    uint32_t enablePerTidDecomp : 16;
#endif

// word 2
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t enableTxPerTidInsertConcatSeqNum : 16;
    uint32_t enableRxPerTidRemoveConcatSeqNum : 16;
#else
    uint32_t enableRxPerTidRemoveConcatSeqNum : 16;
    uint32_t enableTxPerTidInsertConcatSeqNum : 16;
#endif

// word 3
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t resv4 : 4;
    uint32_t replayCountSet : 4;
    uint32_t mickeyIndex : 8;

    /* this is the key desc idx used by DPU to
     *   - encrypt the frame if encMode is WEP/AES/TKIP
     *   - decrypt the frame if encMode is AES/TKIP
     *     (for WEP, DPU uses BD.rxKeyID (filled by SOftMac Rx)
     *     as index to pick the right Key desc idx from wepRxKeyIdx0~3
     */
    uint32_t keyIndex : 8;
    /* this is the key id to be filled in IV when encMode is not OPEN
     */
    uint32_t txKeyId : 2;
    uint32_t resv5 : 3;
    uint32_t encryptMode : 3;
#else
    uint32_t encryptMode : 3;
    uint32_t resv5 : 3;
    uint32_t txKeyId : 2;
    uint32_t keyIndex : 8;
    uint32_t mickeyIndex : 8;
    uint32_t replayCountSet : 4;
    uint32_t resv4 : 4;
#endif
#endif

    // word 4-7
    // for TID0-15, each 8 bits.
    // Use GET_DPU_RCIDX() or SET_DPU_RCIDX() defined above
    // to read/write this field. It takes care of endian problem.
    uint32_t idxPerTidReplayCount[4];
#if ((defined NT_FN_AP_HAL_DPH_DEBUG_STATS) || (defined NT_FN_STA_HAL_DPH_DEBUG_STATS))
    // word 8
    uint32_t txSentBlocks;

    // word 9
    uint32_t rxRcvddBlocks;
#endif  //((defined NT_FN_AP_HAL_DPH_DEBUG_STATS)||(defined NT_FN_STA_HAL_DPH_DEBUG_STATS))

// word 10
#ifdef CR6_NEW_DPU_DESCRIPTOR
#if ((defined NT_FN_AP_HAL_DPH_DEBUG_STATS) || (defined NT_FN_STA_HAL_DPH_DEBUG_STATS))
    uint32_t bipAesTkipReplays;
#endif  // ((defined NT_FN_AP_HAL_DPH_DEBUG_STATS)||(defined NT_FN_STA_HAL_DPH_DEBUG_STATS))
#else
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    // If encMode=WEP, these four bytes saves Key desc indices for each of the four WEP keys.
    // If encMode=TKIP or AES, these four bytes keep # of replays.
    uint32_t wepRxKeyIdx0 : 8;
    uint32_t wepRxKeyIdx1 : 8;
    uint32_t wepRxKeyIdx2 : 8;
    uint32_t wepRxKeyIdx3 : 8;
#else
    uint32_t wepRxKeyIdx3 : 8;
    uint32_t wepRxKeyIdx2 : 8;
    uint32_t wepRxKeyIdx1 : 8;
    uint32_t wepRxKeyIdx0 : 8;
#endif
#endif
// word 11
#ifdef HAL_CFG_BIGBYTE_ENDIAN
#if ((defined NT_FN_AP_HAL_DPH_DEBUG_STATS) || (defined NT_FN_STA_HAL_DPH_DEBUG_STATS))
    uint32_t micErrCount : 8;
    uint32_t excludedCount : 24;
#endif  //((defined NT_FN_AP_HAL_DPH_DEBUG_STATS)||(defined NT_FN_STA_HAL_DPH_DEBUG_STATS))
#else
#if ((defined NT_FN_AP_HAL_DPH_DEBUG_STATS) || (defined NT_FN_STA_HAL_DPH_DEBUG_STATS))
    uint32_t excludedCount : 24;
    uint32_t micErrCount : 8;
#endif  //((defined NT_FN_AP_HAL_DPH_DEBUG_STATS)||(defined NT_FN_STA_HAL_DPH_DEBUG_STATS))
#endif

// word 12
#ifdef HAL_CFG_BIGBYTE_ENDIAN
#if ((defined NT_FN_AP_HAL_DPH_DEBUG_STATS) || (defined NT_FN_STA_HAL_DPH_DEBUG_STATS))
    uint32_t formatErrorCount : 16;
#endif  //((defined NT_FN_AP_HAL_DPH_DEBUG_STATS)||(defined NT_FN_STA_HAL_DPH_DEBUG_STATS))
#if (defined NT_FN_AP_HAL_DPH_PRODUCTION_STATS) || (defined NT_FN_STA_HAL_DPH_PRODUCTION_STATS)
    uint32_t undecryptableCount : 16;
#endif  // (defined NT_FN_AP_HAL_DPH_PRODUCTION_STATS) || (defined NT_FN_STA_HAL_DPH_PRODUCTION_STATS)
#else
#if (defined NT_FN_AP_HAL_DPH_PRODUCTION_STATS) || (defined NT_FN_STA_HAL_DPH_PRODUCTION_STATS)
    uint32_t undecryptableCount : 16;
#endif  // (defined NT_FN_AP_HAL_DPH_PRODUCTION_STATS) || (defined NT_FN_STA_HAL_DPH_PRODUCTION_STATS)
#if ((defined NT_FN_AP_HAL_DPH_DEBUG_STATS) || (defined NT_FN_STA_HAL_DPH_DEBUG_STATS))
    uint32_t formatErrorCount : 16;
#endif  // ((defined NT_FN_AP_HAL_DPH_DEBUG_STATS)||(defined NT_FN_STA_HAL_DPH_DEBUG_STATS))
#endif

#if ((defined NT_FN_AP_HAL_DPH_DEBUG_STATS) || (defined NT_FN_STA_HAL_DPH_DEBUG_STATS))
    // word 13
    uint32_t decryptErrorCount;

    // word 14
    uint32_t decryptSuccessCount;

#endif  //((defined NT_FN_AP_HAL_DPH_DEBUG_STATS)||(defined NT_FN_STA_HAL_DPH_DEBUG_STATS))

// word 15
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t keyIdErr : 8;
    uint32_t extIVerror : 8;
    uint32_t aesQosMaskTid : 16;
#else
    uint32_t aesQosMaskTid : 16;
    uint32_t extIVerror : 8;
    uint32_t keyIdErr : 8;
#endif

#ifdef CR6_SEQUENCE_NUMBER_GENERATION
// word 16
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t resv7 : 4;
    uint32_t seqNoTid1 : 12;
    uint32_t resv8 : 4;
    uint32_t seqNoTid0 : 12;
#else
    uint32_t seqNoTid0 : 12;
    uint32_t resv8 : 4;
    uint32_t seqNoTid1 : 12;
    uint32_t resv7 : 4;
#endif

// word 17
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t resv9 : 4;
    uint32_t seqNoTid3 : 12;
    uint32_t resv10 : 4;
    uint32_t seqNoTid2 : 12;
#else
    uint32_t seqNoTid2 : 12;
    uint32_t resv10 : 4;
    uint32_t seqNoTid3 : 12;
    uint32_t resv9 : 4;
#endif

// word 18
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t resv11 : 4;
    uint32_t seqNoTid5 : 12;
    uint32_t resv12 : 4;
    uint32_t seqNoTid4 : 12;
#else
    uint32_t seqNoTid4 : 12;
    uint32_t resv12 : 4;
    uint32_t seqNoTid5 : 12;
    uint32_t resv11 : 4;
#endif

// word 19
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t resv13 : 4;
    uint32_t seqNoTid7 : 12;
    uint32_t resv14 : 4;
    uint32_t seqNoTid6 : 12;
#else
    uint32_t seqNoTid6 : 12;
    uint32_t resv14 : 4;
    uint32_t seqNoTid7 : 12;
    uint32_t resv13 : 4;
#endif

// word 20
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t resv15 : 4;
    uint32_t seqNoTid9 : 12;
    uint32_t resv16 : 4;
    uint32_t seqNoTid8 : 12;
#else
    uint32_t seqNoTid8 : 12;
    uint32_t resv16 : 4;
    uint32_t seqNoTid9 : 12;
    uint32_t resv15 : 4;
#endif

// word 21
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t resv17 : 4;
    uint32_t seqNoTid11 : 12;
    uint32_t resv18 : 4;
    uint32_t seqNoTid10 : 12;
#else
    uint32_t seqNoTid10 : 12;
    uint32_t resv18 : 4;
    uint32_t seqNoTid11 : 12;
    uint32_t resv17 : 4;
#endif

// word 22
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t resv19 : 4;
    uint32_t seqNoTid13 : 12;
    uint32_t resv20 : 4;
    uint32_t seqNoTid12 : 12;
#else
    uint32_t seqNoTid12 : 12;
    uint32_t resv20 : 4;
    uint32_t seqNoTid13 : 12;
    uint32_t resv19 : 4;
#endif

// word 23
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t resv21 : 4;
    uint32_t seqNoTid15 : 12;
    uint32_t resv22 : 4;
    uint32_t seqNoTid14 : 12;
#else
    uint32_t seqNoTid14 : 12;
    uint32_t resv22 : 4;
    uint32_t seqNoTid15 : 12;
    uint32_t resv21 : 4;
#endif
#endif

} hal_nova_dpu_desc_t;

typedef struct hal_nova_dpu_key_desc_s {
    uint32_t key128bit[4];
} hal_nova_dpu_key_desc_t;

typedef struct hal_nova_dpu_mic_key_desc_s {
    uint32_t txMicKey64bit[2];
    uint32_t rxMicKey64bit[2];
} hal_nova_dpu_mic_key_desc_t;

typedef struct hal_nova_dpu_replay_desc_s {
    uint32_t txReplayCount31to0;
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t txReplayCount47to32 : 16;
    uint32_t resv1 : 16;
#else
    uint32_t resv1 : 16;
    uint32_t txReplayCount47to32 : 16;
#endif
    uint32_t rxReplayCount31to0;

#ifdef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t rxReplayCount47to32 : 16;
    uint32_t resv2 : 6;
    uint32_t replayChkEnabled : 1;
    uint32_t winChkEnabled : 1;
    uint32_t winChkSize : 8;
#else
    uint32_t winChkSize : 8;
    uint32_t winChkEnabled : 1;
    uint32_t replayChkEnabled : 1;
    uint32_t resv2 : 6;
    uint32_t rxReplayCount47to32 : 16;
#endif

} hal_nova_dpu_replay_desc_t;

#endif  // _HAL_INT_BD_UTILS_H_
