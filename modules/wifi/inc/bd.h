/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef BD_H_
#define BD_H_

#include "stdint.h"

#define CR6_MANAGEMENT_FRAME_PROTECTION
#define ANI_BIG_BYTE_ENDIAN  // This flag is defined here to enable use of BIG Endian version of BD structure as the BD
                             // is still interepreted by WMAC modules in BIG endian format even though CPU is LITTLE
                             // endian.
#define NT_BMU_BD_SIZE 128

/********************************************************************************************************************/

/* SW defined BD type, used in  tSmacBdRxpRx & tSmacBdHostTx*/
#define NT_SMAC_SWBD_TYPE_RXRAW  0  // RAW BD format from RxP, use tSmacBdRxpRx
#define NT_SMAC_SWBD_TYPE_DATA   1  // Rx/Tx Data BD, use tSmacBdHostRx
#define NT_SMAC_SWBD_TYPE_JUNK   2  // Frame to be dropped by Host directly.
#define NT_SMAC_SWBD_TYPE_CTLMSG 3  // BD for passing ctrl message between host/SoftMac.

/* Tx Ack Policy */
#define NT_SMAC_SWBD_ACKPOLICY_NORMAL 0 /* Normal ACK */
#define NT_SMAC_SWBD_ACKPOLICY_NO     1 /* No ACK */

/* SoftMac Rx response frame type */
#define NT_SMAC_SWBD_RXRESPONSE_UNDEFINED 0 /*Not applicable, may be in promisc mode,..., or unspecified by SoftMac*/
#define NT_SMAC_SWBD_RXRESPONSE_NONE      1 /*No response frame sent to TA*/
#define NT_SMAC_SWBD_RXRESPONSE_BACK      2 /*Blk ACK frame sent to TA after rcv this frame*/
#define NT_SMAC_SWBD_RXRESPONSE_NORMALACK 3 /*Normal ACK frame sent to TA after rcv this frame*/
#define NT_SMAC_SWBD_RXRESPONSE_NULL      4 /*NULL frame sent to TA after rcv this frame, maybe due to PM bit*/
#define NT_SMAC_SWBD_RXRESPONSE_PSPOLL    5 /*PS-Poll frame sent to TA after rcv this frame, maybe due to beacon TIM \
                                               bit*/

/* SoftMac Rx reorder buffer */
#define NT_SMAC_SWBD_RXREORDER_OPCODE_INVALID 0
#define NT_SMAC_SWBD_RXREORDER_OPCODE_QCURRENT_FWDADV \
    1 /* Queue current packet, Fwd up to Head ptr if Head ptr advanced, then set new head ptr */
#define NT_SMAC_SWBD_RXREORDER_OPCODE_QCURRENT_FWD \
    4 /* Queue current packet, Fwd up to Head ptr, then set new head ptr */
#define NT_SMAC_SWBD_RXREORDER_OPCODE_FWD_QCURRENT \
    2 /* Fwd up to Head ptr, queue the new packet, then set new head ptr */
#define NT_SMAC_SWBD_RXREORDER_OPCODE_FWDALL_QCURRENT 3 /* Fwd all frmes, queue the new packet, then set new head \
                                                           ptr*/
#define NT_SMAC_SWBD_RXREORDER_OPCODE_FWDALL_FWDCURRENT \
    5 /* Fwd all frames then current frame, finally set new head ptr*/
#define NT_SMAC_SWBD_RXREORDER_OPCODE_FWDADV_DROPCURRENT \
    6 /* Forward up to Head prt if Head ptr advanced then set new head ptr. Drop current frame (ex: BAR)*/
#define NT_SMAC_SWBD_RXREORDER_OPCODE_FWDALL_DROPCURRENT \
    7 /* Fwd all frmes, then set new head ptr. Drop current frame (ex:BAR) */

/* Mac Protection */
#define NT_SMAC_SWBD_PROTECTION_DEFAULT     0  /* Default */
#define NT_SMAC_SWBD_PROTECTION_NO          1  /* No protection */
#define NT_SMAC_SWBD_PROTECTION_FORCERTS    2  /* RTS forced */
#define NT_SMAC_SWBD_PROTECTION_FORCECTS2S  3  /* CTS-to-self forced */
#define NT_SMAC_SWBD_PROTECTION_FORCE3CTS2S 10 /* CTS-to-self forced */

/* TX station index */
#define NT_SMAC_SWBD_TX_STAIDX_GROUP   0   /* raIndex : broadcast/multicast */
#define NT_SMAC_SWBD_TX_STAIDX_INVALID 254 /* taIndex & raIndex : invalid */

/* TX TID */
#define NT_SMAC_SWBD_TX_TID_MGMT_LOW    0 /* ProbeResponse */
#define NT_SMAC_SWBD_TX_TID_MGMT_HIGH   1 /* Other management frames */
#define NT_SMAC_SWBD_TX_TID_MGMT_LOWEST 2 /* SoftMAC internal */
#define NT_SMAC_SWBD_TX_TID_MGMT_NUM    3

/* TX BD structure */
/*changed in fermion from 0x48 to 0x4C for 4B Tx-bd complete dialogToken padding */
#define NT_TX_MPDUHEADER_OFFSET 0x4C /* SoftMAC recommended MPDU header offset */

#ifdef CR6_SEQUENCE_NUMBER_GENERATION
#define HOST_GENERATE_SN         0
#define LEGACY_HW_GENERATE_SN    1
#define TID_BASED_HW_GENERATE_SN 2
#endif

/* TX MPDU data offset */
#define NT_TX_MGMTDATA_OFFSET 0x64  // add 4 more bytes
#define NT_TX_MPDUDATA_OFFSET 0x6E  // minimal 0x4C+26=0x66

#define NT_TX_EAPOLDATA_OFFSET 0x66  // add 4 more bytes

/* RX BD structure */
#define NT_SMAC_SWBD_RX_MPDUHEADER_OFFSET 0x4c

/* RX station index */
#define NT_SMAC_HWBD_RX_UNKNOWN_UCAST 254
#define NT_SMAC_HWBD_RX_UNKNOWN_MCAST 0

/* Rate index */
#define NT_SMAC_SWBD_TX_RATEINDEX_DEFAULT 255

//#define	NT_SMAC_SWBD_QID_0						   0
#define NT_SMAC_SWBD_QID_10 10

#define BD_TX_END 18
#define BD_RX_END 19

#define NT_MAC_DEAUTH         0XC
#define NT_MAC_DISASSOC       0XA
#define NT_MAC_ACTION         0XD
#define NT_MAC_PROBE_REQUEST  0X4  // Subtype of probe request
#define NT_MAC_PROBE_RESPONSE 0X5  // Subtype of probe response
#define NT_MAC_SUBTYPE        0X4
#define NT_MAC_SUBTYPE_MASK   0XF

#if 0
typedef struct BDTXTemplate {
    /* 0x00 */
#ifdef CR6_MANAGEMENT_FRAME_PROTECTION
#ifdef SW_ASSISTED_MODE
#ifdef ANI_BIG_BYTE_ENDIAN
    /* Routing flag. D/C in Tx processing.
     * To reach SoftMAC Tx code, host needs to set this routing flag to
     * one of mCPU Tx WQ between SMAC_BMUWQ_MCPU_TX_WQ0 and SMAC_BMUWQ_MCPU_TX_WQ5.
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

    /* SMAC_SWBD_TYPE_XXX.
     * Valid types are as follow :
     *   SMAC_SWBD_TYPE_DATA : This BD has data frame.
     *   SMAC_SWBD_TYPE_CTLMSG : This BD has host interface message. (NOT READY)
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

    /* BD type. Must be SMAC_HWBD_TYPE_GENERIC */
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
#ifdef ANI_BIG_BYTE_ENDIAN
    /* Routing flag. D/C in Tx processing.
     * To reach SoftMAC Tx code, host needs to set this routing flag to
     * one of mCPU Tx WQ between SMAC_BMUWQ_MCPU_TX_WQ0 and SMAC_BMUWQ_MCPU_TX_WQ5.
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

	/* SMAC_SWBD_TYPE_XXX.
     * Valid types are as follow :
     *   SMAC_SWBD_TYPE_DATA : This BD has data frame.
     *   SMAC_SWBD_TYPE_CTLMSG : This BD has host interface message. (NOT READY)
     */
    uint32_t swBdType : 2;

    uint32_t reserved1 : 2;

	/*Enable CSU in TX */
    uint32_t csuTxEnable : 1;

	/*Enable TCP UDP Checksum generation */
    uint32_t tcpUdpCSEnable : 1;

	/*Enable IPV4/IPV6 Checksum generation */
    uint32_t ipCSGenEnable : 1;

	/*TCP UDP Checksum Generation Status */
    uint32_t noTCPUDPCheckSumGenerated : 1;

	/*TCP UDP Checksum Generation Status */
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

    /* BD type. Must be SMAC_HWBD_TYPE_GENERIC */
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
#ifdef ANI_BIG_BYTE_ENDIAN
    /* Routing flag. D/C in Tx processing.
     * To reach SoftMAC Tx code, host needs to set this routing flag to
     * one of mCPU Tx WQ between SMAC_BMUWQ_MCPU_TX_WQ0 and SMAC_BMUWQ_MCPU_TX_WQ5.
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


    /* SMAC_SWBD_TYPE_XXX.
     * Valid types are as follow :
     *   SMAC_SWBD_TYPE_DATA : This BD has data frame.
     *   SMAC_SWBD_TYPE_CTLMSG : This BD has host interface message. (NOT READY)
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

    /* BD type. Must be SMAC_HWBD_TYPE_GENERIC */
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
#ifdef ANI_BIG_BYTE_ENDIAN
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
#ifdef ANI_BIG_BYTE_ENDIAN
    /* head and tail PDU indices are filled by DXE. */
    uint32_t headPduIdx : 16;
    uint32_t tailPduIdx : 16;
#else
    uint32_t tailPduIdx : 16;
    uint32_t headPduIdx : 16;
#endif

    /* 0x0c */
#ifdef ANI_BIG_BYTE_ENDIAN
    /* MPDU header length */
    uint32_t mpduHeaderLength : 8;

    /* MPDU header offset. Recommended value is SMAC_SWBD_TX_MPDUHEADER_OFFSET.
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
#ifdef ANI_BIG_BYTE_ENDIAN
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
     * Management frame : Priority. Valid values are SMAC_SWBD_TX_TID_MGMT_LOW (0) or
     *   SMAC_SWBD_TX_TID_MGMT_HIGH (1). Other values are not valid.
     * Data frame : TID. Valid range is 0..7.
     */
    uint32_t tid : 4;

    /* Rate index to use. SMAC_SWBD_TX_RATEINDEX_DEFAULT to use default rate
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
#ifdef ANI_BIG_BYTE_ENDIAN
    /* MPDU length. This covers MPDU header length + MPDU data length. This does not
     * include FCS. For single frame transmission, PSDU size is mpduLength + 4.
     */
    uint32_t mpduLength : 16;

    uint32_t reserved3 : 4;

    /* Traffic identifier.
     * Group frame : Priority. Valid range is 0..7
     * Management frame : Priority. Valid values are SMAC_SWBD_TX_TID_MGMT_LOW (0) or
     *   SMAC_SWBD_TX_TID_MGMT_HIGH (1). Other values are not valid.
     * Data frame : TID. Valid range is 0..7.
     */
    uint32_t tid : 4;

    /* Rate index to use. SMAC_SWBD_TX_RATEINDEX_DEFAULT to use default rate
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
#ifdef ANI_BIG_BYTE_ENDIAN
    /* Used by DPU. d/c in Tx processing */
    uint32_t dpuDescIdx : 8;

    /* Recipient station index.
     * Valid station index in 0-253 : Recipient station index.
     * SMAC_SWBD_TX_STAIDX_INVALID (254) : Recipient is unknown. rateIndex must not be default.
     * SMAC_SWBD_TX_STAIDX_GROUP (255) : Recipient is broadcast/multicast.
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
#ifdef ANI_BIG_BYTE_ENDIAN
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
    uint32_t keepSeqNum: 1;

    /* FUTURE USE (NOT READY)
     * Transmission indication is sent to host after frame is transmitted
     * or dropped.
     */
    uint32_t txIndication : 1;

    /* ACK Policy : SMAC_SWBD_ACKPOLICY_xxx
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
     * Force specific MAC protection (SMAC_SWBD_PROTECTION_xxx). This override
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
    uint32_t keepSeqNum: 1;
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
#ifdef ANI_BIG_BYTE_ENDIAN
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
#ifdef ANI_BIG_BYTE_ENDIAN
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
} __attribute__((packed)) __attribute__((aligned (4))) BDTXTemplate_t, *pBDTXTemplate;
#endif

#if 0

typedef struct dpm_bd_tx_template {
	/* 0x00 */
#define BD_TYPE_GENERIC 0 /* generic BD format, use tSmacBdGeneric */
#define BD_TYPE_FRAG    1 /* fragmentation BD format, use tSmacBdDpuFrag*/
    uint32_t bdt : 2;

#define NO_FRAME_TRANSLATION 0
#define FRAME_TRANS_REQUIRED 1
    uint32_t ft : 1;

#define ENCRYPTION_DISABLED 1
    uint32_t dpuNE : 1;

#define NO_INTERRUPT_GENERATED 0
#define GENERATE_TX_INTERRUPT  1
    uint32_t txCPLT : 2;
    uint32_t reserved1 : 1;

#define UNICAST_PACKET   0
#define BROADCAST_PACKET 1
    uint32_t ub : 1;

#define NOT_RMF     0
#define RMF_ENCRYPT 1
    uint32_t rmf : 1;
    uint32_t noValHD : 1;
    uint32_t noTCPUDPCheckSumGenerated : 1;

#define IP_CS_NOT_GENERATED 0
#define IP_CS_GENERATED     1
    uint32_t ipCSGenEnable : 1;

#define TCP_UDP_CS_NOT_GENERATED 0
#define TCP_UDP_CS_GENERATED     1
    uint32_t tcpUdpCSEnable : 1;

#define CSU_DISABLED 0
#define CSU_ENABLED  1
    uint32_t csuTxEnable : 1;

#define CSU_SOFTWARE_MODE 1
    uint32_t csuSWMode : 1;//0

#define UMA_BD_ENABLE 1
    uint32_t umaBDenable : 1;
    uint32_t umaBSSID : 2;
    uint32_t reserved2 : 3;
    uint32_t dpuSignature : 3;
    uint32_t dpuRF : 8;


    /* 0x04 */
    uint32_t dpuFeedback : 8;
    uint32_t aduFeedback : 8;
    uint32_t reserved3 : 16;


    /* 0x08 */
    uint32_t tailPduIdx : 16;
    uint32_t headPduIdx : 16;


    /* 0x0C */
    uint32_t pduCount : 7;
    uint32_t mpduDataOffset : 9;//Fill
    uint32_t mpduHeaderOffset : 8;//FIll
    uint32_t mpduHeaderLength : 8;//Fill


    /* 0x10 */
    uint32_t reserved4 : 8;

#define NON_QOS_FRAME 0
    uint32_t tid : 4;//Fill

#define HOST_GENERATED_SN 0
#define NON_TID_SSN       1
#define TID_BASED_SSN     2
	uint32_t bdSsn : 2;
    uint32_t reserved5 : 2;
    uint32_t mpduLength : 16;//Fill


    /* 0x14 */
    uint32_t reserved6 : 7;
    uint32_t txWqId : 5;//Fill --->?

#define USE_TPE_STA_DESC_RATE 0
#define USE_TPE_BD_RATE_1     1
#define USE_TPE_BD_RATE_2     2
    uint32_t BDrate : 2;//Fill

#define IMMEDIATE_ACK 0
	uint32_t ackPolicy : 2;
    uint32_t StaId : 8;//Fill
    uint32_t dpuDescIdx : 8;//Fill


    /* 0x18 */
    uint32_t reserved7;


    /* 0x1C */
    uint32_t reserved8;


    /* 0x20 */
    uint32_t DXETimeStampCurrent;


    /* 0x24 */
    uint32_t DXETimeStampPrevious;


    /* 0x28 */
    uint32_t reserved9 : 11;
    uint32_t TcpUdpChecksumOffset : 5;//Fill --- > Used?
    uint32_t TcpUdpStartOffset : 16;//Fill --- > Used?


    /* 0x2C */
    uint32_t reserved10 : 16;
    uint32_t TcpUdpByteLength : 16;//Fill --- > Used?


    /* 0x30 */
    uint32_t reserved11;


    /* 0x34 - 0x47 */
    uint8_t rsvdMPIcmmds[20];

} __attribute__((packed)) __attribute__((aligned (4))) dpm_bd_tx_template_t, *p_dpm_bd_tx_template;

#else
#if 1

typedef struct dpm_bd_tx_template {
    /*0X00*/
#define BD_TYPE_GENERIC           0 /* generic BD format, use tSmacBdGeneric */
#define BD_TYPE_FRAG              1 /* fragmentation BD format, use tSmacBdDpuFrag*/
    uint32_t bdt : 2;
#define NO_FRAME_TRANSLATION      0
#define FRAME_TRANS_REQUIRED      1
    uint32_t ft : 1;
#define ENCRYPTION_DISABLED       1
#define ENCRYPTION_ENABLE         0
    uint32_t dpuNE : 1;
    uint32_t fwTxComplete0 : 1;
    uint32_t txComplete1 : 1;
    uint32_t rsvd2_5 : 1;
#define UNICAST_PACKET            0
#define BROADCAST_PACKET          1
    uint32_t ub : 1;
#define RMF_DISABLE               0
#define RMF_ENABLE                1
    uint32_t rmf : 1;
    uint32_t reserved2_0 : 10;
    uint32_t tmf : 1;  // BIT19 : "1" indicate that this frame is a TM/FTM frame
    uint32_t reserved2_1 : 1;
    uint32_t dpuSignature : 3;
#define DPU_ROUTING_FLAG          25
    uint32_t dpuRF : 8;

    /*0x04*/
    uint32_t dpuFeedback : 8;
    uint32_t aduFeedback : 8;
    uint32_t reserved3 : 16;

    /*0x08*/
    uint32_t tailPduIdx : 16;
    uint32_t headPduIdx : 16;

    /*0x0C*/
    uint32_t pduCount : 7;
    uint32_t mpduDataOffset : 9;    // Fill
    uint32_t mpduHeaderOffset : 8;  // FIll
#define NONQOS_MPDU_HEADER_LENGTH 24
#define QOS_MPDU_HEADER_LENGTH    26
    uint32_t mpduHeaderLength : 8;  // Fill

    /* 0x10 */
    uint32_t reserved4 : 8;
#define NON_QOS_FRAME             0
    uint32_t tid : 4;  // Fill
#define HOST_GENERATED_SN         0
#define NON_TID_SSN               1
#define TID_BASED_SSN             2
    uint32_t bdSsn : 2;
    uint32_t reserved5 : 2;
    uint32_t mpduLength : 16;  // Fill

    /*0X14*/
    uint32_t reserved6 : 7;
    uint32_t txWqId : 5;  // Fill
#define USE_TPE_STA_DESC_RATE     0
#define USE_TPE_BD_RATE_1         1
#define USE_TPE_BD_RATE_2         2
    uint32_t BDrate : 2;  // Fill
#define IMMEDIATE_ACK             0
#define NOIMMEDIATE_ACK           1
    uint32_t ackPolicy : 2;
    uint32_t StaId : 8;       // Fill
#define DPUDESC_IDX_MCBC_MGMT     6
#define DPUDESC_IDX_MCBC_DATA     7
    uint32_t dpuDescIdx : 8;  // Fill

    /* 0x18 */
    uint32_t reserved7;

    /* 0x1C */
    uint32_t reserved8;

    /* 0x20 */
    uint32_t DXETimeStampCurrent;

    /* 0x24 */
    uint32_t DXETimeStampPrevious;

    /* 0x28 */
    uint32_t reserved9 : 11;
    uint32_t TcpUdpChecksumOffset : 5;  // Fill --- > Used?
    uint32_t TcpUdpStartOffset : 16;    // Fill --- > Used?

    /* 0x2C */
    uint32_t reserved10 : 16;
    uint32_t TcpUdpByteLength : 16;  // Fill --- > Used?

    /* 0x30 */
    uint32_t reserved11;

    /* 0x34 - 0x47 */
    uint8_t rsvdMPIcmmds[20];

} __attribute__((packed)) __attribute__((aligned(4))) dpm_bd_tx_template_t, *p_dpm_bd_tx_template;

#else

typedef struct dpm_bd_tx_template {
    /* 0x00 */
    uint32_t dpuRF : 8;
    uint32_t dpuSignature : 3;
    uint32_t reserved2 : 3;
    uint32_t umaBSSID : 2;
#define UMA_BD_ENABLE            1
    uint32_t umaBDenable : 1;
#define CSU_SOFTWARE_MODE        1
    uint32_t csuSWMode : 1;  // 0
#define CSU_DISABLED             0
#define CSU_ENABLED              1
    uint32_t csuTxEnable : 1;
#define TCP_UDP_CS_NOT_GENERATED 0
#define TCP_UDP_CS_GENERATED     1
    uint32_t tcpUdpCSEnable : 1;
#define IP_CS_NOT_GENERATED      0
#define IP_CS_GENERATED          1
    uint32_t ipCSGenEnable : 1;
    uint32_t noTCPUDPCheckSumGenerated : 1;
    uint32_t noValHD : 1;
#define NOT_RMF                  0
#define RMF_ENCRYPT              1
    uint32_t rmf : 1;
#define UNICAST_PACKET           0
#define BROADCAST_PACKET         1
    uint32_t ub : 1;
    uint32_t reserved1 : 1;
#define NO_INTERRUPT_GENERATED   0
#define GENERATE_TX_INTERRUPT    1
    uint32_t txCPLT : 2;
#define ENCRYPTION_DISABLED      1
    uint32_t dpuNE : 1;
#define NO_FRAME_TRANSLATION     0
#define FRAME_TRANS_REQUIRED     1
    uint32_t ft : 1;
#define BD_TYPE_GENERIC          0 /* generic BD format, use tSmacBdGeneric */
#define BD_TYPE_FRAG             1 /* fragmentation BD format, use tSmacBdDpuFrag*/
    uint32_t bdt : 2;

    /* 0x04 */
    uint32_t reserved3 : 16;
    uint32_t aduFeedback : 8;
    uint32_t dpuFeedback : 8;

    /* 0x08 */
    uint32_t headPduIdx : 16;
    uint32_t tailPduIdx : 16;

    /* 0x0C */
    uint32_t mpduHeaderLength : 8;  // Fill
    uint32_t mpduHeaderOffset : 8;  // FIll
    uint32_t mpduDataOffset : 9;    // Fill
    uint32_t pduCount : 7;

    /* 0x10 */
    uint32_t mpduLength : 16;  // Fill
    uint32_t reserved5 : 2;
#define HOST_GENERATED_SN        0
#define NON_TID_SSN              1
#define TID_BASED_SSN            2
    uint32_t bdSsn : 2;
#define NON_QOS_FRAME            0
    uint32_t tid : 4;  // Fill
    uint32_t reserved4 : 8;

    /* 0x14 */
    uint32_t dpuDescIdx : 8;  // Fill
    uint32_t StaId : 8;       // Fill
#define IMMEDIATE_ACK            0
    uint32_t ackPolicy : 2;
#define USE_TPE_STA_DESC_RATE    0
#define USE_TPE_BD_RATE_1        1
#define USE_TPE_BD_RATE_2        2
    uint32_t BDrate : 2;  // Fill
    uint32_t txWqId : 5;  // Fill --->?
    uint32_t reserved6 : 7;

    /* 0x18 */
    uint32_t reserved7;

    /* 0x1C */
    uint32_t reserved8;

    /* 0x20 */
    uint32_t DXETimeStampCurrent;

    /* 0x24 */
    uint32_t DXETimeStampPrevious;

    /* 0x28 */
    uint32_t TcpUdpStartOffset : 16;    // Fill --- > Used?
    uint32_t TcpUdpChecksumOffset : 5;  // Fill --- > Used?
    uint32_t reserved9 : 11;

    /* 0x2C */
    uint32_t TcpUdpByteLength : 16;  // Fill --- > Used?
    uint32_t reserved10 : 16;

    /* 0x30 */
    uint32_t reserved11;

    /* 0x34 - 0x47 */
    uint8_t rsvdMPIcmmds[20];

} __attribute__((packed)) __attribute__((aligned(4))) dpm_bd_tx_template_t, *p_dpm_bd_tx_template;

#endif
#endif

#if 1
typedef struct DPMBDRXTemplate {
    /* 0x00 */
    uint32_t bdt : 2;
    uint32_t reserved2 : 1;
    uint32_t dpuNE : 1;
    uint32_t rxKeyId : 2;
    uint32_t ub : 1;
    uint32_t rmf : 1;
    uint32_t reserved1 : 9;
    uint32_t tsf_received : 1;
    uint32_t frame_after_beacon : 1;
    uint32_t miss_addr2_hit : 1;
    uint32_t station_auth : 1;
    uint32_t dpuSignature : 3;
    uint32_t dpuRF : 8;

    /* 0x04 */
    uint32_t dpuFeedback : 8;
    uint32_t reserved3 : 8;
    uint32_t PenultimatePDU : 16;

    /* 0x08 */
    uint32_t tailPduIdx : 16;
    uint32_t headPduIdx : 16;

    /* 0x0c */
    uint32_t pduCount : 7;
    uint32_t mpduDataOffset : 9;
    uint32_t mpduHeaderOffset : 8;
    uint32_t mpduHeaderLength : 8;

    /* 0x10 */
    uint32_t reserved5 : 8;
    uint32_t tid : 4;
    uint32_t reserved4 : 4;
    uint32_t mpduLength : 16;

    /* 0x14 */
    uint32_t addr3Index : 8;
    uint32_t addr2Index : 8;
    uint32_t addr1Index : 8;
    uint32_t dpuDescIdx : 8;

#if 0
	uint32_t rxpFlags;
#else
    /* 0x18, see SMAC_HWBD_RXFLAG_XXXX */
    uint32_t rxpFlags : 23; /* RxP flags*/
    uint32_t rateIndex : 9;
#endif

    /* 0x1c, 20 */
    uint32_t phyStats0; /* PHY status word 0*/
    uint32_t phyStats1; /* PHY status word 0*/

    /* 0x24 */
    uint32_t mclkRxTimestamp; /* Rx timestamp, microsecond based*/

    /* 0x28~0x3f */
    uint32_t rxPmiCmd[6]; /* PMI cmd rcvd from RxP */

    /* 0x40 */
    uint32_t reserved7 : 4;
    uint32_t ReorderSlotIndex : 6;
    uint32_t ReorderFwdIndex : 6;  // 1 : rcvd beacon has TSF later than ours.
    uint32_t reserved6 : 12;       // 1: we have sent beacon in this TBTT.
    uint32_t ReorderOpcode : 4;    /* bit=1: Frame is rcvd during SCAN/Learn mode */

    /* 0x44 */
    uint32_t ExpectedSeqNo : 12;
    uint32_t CurrentSeqNo : 12;
    uint32_t swfield : 8;

    /* 0x48 */
    uint32_t totalAmsduSize : 16;
    uint32_t subFrameIndex : 4;
    uint32_t processOrder : 4;
    uint32_t reserved9 : 4;
    uint32_t isAmsduErrorFlag : 1;
    uint32_t isLastAmsduSubFrame : 1;
    uint32_t isFirstAmsduSubFrame : 1;
    uint32_t isAmsduSubframe : 1;

} __attribute__((packed)) __attribute__((aligned(4))) dpm_bd_rx_template_t, *p_dpm_bd_rx_template;

#else
typedef struct DPMBDRXTemplate {
    /* 0x00 */
    uint32_t bdt : 2;
    uint32_t ft : 1;
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

    /* 0x04 */
    uint32_t dpuFeedback : 8;
    uint32_t MagicPkt : 1;
    uint32_t aduFeedback : 7;
    uint32_t PenultimatePDU : 16;

    /* 0x08 */
    uint32_t tailPduIdx : 16;
    uint32_t headPduIdx : 16;

    /* 0x0c */
    uint32_t pduCount : 7;
    uint32_t mpduDataOffset : 9;
    uint32_t mpduHeaderOffset : 8;
    uint32_t mpduHeaderLength : 8;

    /* 0x10 */
    uint32_t reserved1 : 7;
    uint32_t tid : 4;
    uint32_t reserved2 : 5;
    uint32_t mpduLength : 16;

    /* 0x14 */
    uint32_t addr3Index : 8;
    uint32_t addr2Index : 8;
    uint32_t addr1Index : 8;
    uint32_t dpuDescIdx : 8;

    /* 0x18, see SMAC_HWBD_RXFLAG_XXXX */
    uint32_t rxpFlags : 23; /* RxP flags*/
    uint32_t rateIndex : 9;

    /* 0x1c, 20 */
    uint32_t phyStats0; /* PHY status word 0*/
    uint32_t phyStats1; /* PHY status word 0*/

    /* 0x24 */
    uint32_t mclkRxTimestamp; /* Rx timestamp, microsecond based*/

    /* 0x28~0x3f */
    uint32_t rxPmiCmd[6]; /* PMI cmd rcvd from RxP */

    /* 0x40 */
    uint32_t reserved3 : 4;
    uint32_t ReorderSlotIndex : 6;
    uint32_t ReorderFwdIndex : 6;  // 1 : rcvd beacon has TSF later than ours.
    uint32_t reserved4 : 12;       // 1: we have sent beacon in this TBTT.
    uint32_t ReorderOpcode : 4;    /* bit=1: Frame is rcvd during SCAN/Learn mode */

    /* 0x44 */
    uint32_t ExpectedSeqNo : 12;
    uint32_t CurrentSeqNo : 12;
    uint32_t swfield : 8;

    /* 0x48 */
    uint32_t totalAmsduSize : 16;
    uint32_t subFrameIndex : 4;
    uint32_t processOrder : 4;
    uint32_t reserved6 : 4;
    uint32_t isAmsduErrorFlag : 1;
    uint32_t isLastAmsduSubFrame : 1;
    uint32_t isFirstAmsduSubFrame : 1;
    uint32_t isAmsduSubframe : 1;

} __attribute__((packed)) __attribute__((aligned(4))) dpm_bd_rx_template_t, *p_dpm_bd_rx_template;
#endif

#if 0

typedef struct BDRXTemplate {
    /* 0x00 */
#ifdef CR6_MANAGEMENT_FRAME_PROTECTION
#ifdef ANI_BIG_BYTE_ENDIAN

    //DPU routing flag: Next WQ# for DPU to push to
    uint32_t dpuRF : 8;          /* DPU routing flag */
    uint32_t dpuSignature:3;     /* Signature on RA's DPU descriptor */
    uint32_t station_auth:1;
    uint32_t miss_addr2_hit:1;
    uint32_t frame_after_beacon:1;
    uint32_t tsf_received:1;

	uint32_t noValidHeader:1;
	uint32_t csuVerifiedtcpUDPOrNot:1;
	uint32_t csuVerifiedIpv46OrNot:1;
	uint32_t csValid:1;
	uint32_t tcpUDPCSError:1;
	uint32_t ipCSError:1;
	uint32_t llc:1;
    uint32_t umabypass:1;

    //BD type set by software, SMAC_SWBD_XXXX
    //Normal frames use SMAC_SWBD_TYPE_DATA.
    //Host messages use SMAC_SWBD_TYPE_CTLMSG
    //Raw Pkt rcvd from RxP and frames rcvd during promisc mode use SMAC_SWBD_TYPE_RXRAW
  //  uint32_t reserved1 : 8;

	/* Robust Management Frame */
    uint32_t rmf : 1;

	/* Broadcast/Multicast or Unicast */
	uint32_t ub : 1;

    uint32_t rxKeyId : 3; /* Key ID for DPU to decrypt */

    //For DPU:
    // Rx: if set, DPU bypasses decryption
    // Tx: If set, DPU bypasses encryption
    uint32_t dpuNE : 1;

    //For DPU:
    //Rx: if set, DPU bypasses decompression
    //Tx: If set, DPU bypasses compression
    uint32_t dpuNC : 1;

    //Normal frames use SMAC_HWBD_TYPE_GENERIC
    //SMAC_HWBD_TYPE_FRAG is only used between SoftMac Rx/DPU
    uint32_t bdt : 2;            /* BD type */
#else
    uint32_t bdt : 2;
    uint32_t dpuNC : 1;
    uint32_t dpuNE : 1;
    uint32_t rxKeyId : 3;
	uint32_t ub : 1;
    uint32_t rmf : 1;
    uint32_t umabypass:1;
	uint32_t llc:1;
	uint32_t ipCSError:1;
	uint32_t tcpUDPCSError:1;
	uint32_t csValid:1;
	uint32_t csuVerifiedIpv46OrNot:1;
	uint32_t csuVerifiedtcpUDPOrNot:1;
	uint32_t noValidHeader:1;
    uint32_t tsf_received:1;
    uint32_t frame_after_beacon:1;
    uint32_t miss_addr2_hit:1;
    uint32_t station_auth:1;
    uint32_t dpuSignature:3;
    uint32_t dpuRF : 8;
#endif
#else
#ifdef ANI_BIG_BYTE_ENDIAN

    //DPU routing flag: Next WQ# for DPU to push to
    uint32_t dpuRF : 8;          /* DPU routing flag */
    uint32_t dpuSignature:3;     /* Signature on RA's DPU descriptor */
    uint32_t station_auth:1;
    uint32_t miss_addr2_hit:1;
    uint32_t frame_after_beacon:1;
     uint32_t tsf_received:1;

	uint32_t noValidHeader:1;
	uint32_t csuVerifiedtcpUDPOrNot:1;
	uint32_t csuVerifiedIpv46OrNot:1;
	uint32_t csValid:1;
	uint32_t tcpUDPCSError:1;
	uint32_t ipCSError:1;
	uint32_t llc:1;
    uint32_t umabypass:1;

    //BD type set by software, SMAC_SWBD_XXXX
    //Normal frames use SMAC_SWBD_TYPE_DATA.
    //Host messages use SMAC_SWBD_TYPE_CTLMSG
    //Raw Pkt rcvd from RxP and frames rcvd during promisc mode use SMAC_SWBD_TYPE_RXRAW
     uint32_t reserved1 : 3;
    uint32_t rxKeyId : 2; /* Key ID for DPU to decrypt */

    //For DPU:
    // Rx: if set, DPU bypasses decryption
    // Tx: If set, DPU bypasses encryption
    uint32_t dpuNE : 1;

    //For DPU:
    //Rx: if set, DPU bypasses decompression
    //Tx: If set, DPU bypasses compression
    uint32_t dpuNC : 1;

    //Normal frames use SMAC_HWBD_TYPE_GENERIC
    //SMAC_HWBD_TYPE_FRAG is only used between SoftMac Rx/DPU
    uint32_t bdt : 2;            /* BD type */
#else
    uint32_t bdt : 2;
    uint32_t dpuNC : 1;
    uint32_t dpuNE : 1;
    uint32_t rxKeyId : 2;
     uint32_t reserved1 : 3;
    uint32_t umabypass:1;
	uint32_t llc:1;
	uint32_t ipCSError:1;
	uint32_t tcpUDPCSError:1;
	uint32_t csValid:1;
	uint32_t csuVerifiedIpv46OrNot:1;
	uint32_t csuVerifiedtcpUDPOrNot:1;
	uint32_t noValidHeader:1;
     uint32_t tsf_received:1;
    uint32_t frame_after_beacon:1;
    uint32_t miss_addr2_hit:1;
    uint32_t station_auth:1;
    uint32_t dpuSignature:3;
    uint32_t dpuRF : 8;
#endif
#endif

    /* 0x04 */
#ifdef ANI_BIG_BYTE_ENDIAN

    //Used between SoftMac<->BMU only, (for AMSDU deaggregation)
    //HDD should not interpreted this field
    uint32_t penultimatePduIdx:16;
    uint32_t smacPushWq:8;  /* WQ # to which the frame is being pushed to after SoftMac Rx. Used by SoftMac.*/

    uint32_t dpuFeedback:8;               /* If smacPushWq=0xff(BMU Junk WQ, ie.SMAC_BMUWQ_SINK), this is filled in by SoftMac, indicates SoftMac drop reason
                                            * This field is set to 0xff by SoftMac before push to 'smacPushWq' in all other cases.
                                            * If smacPushWq=0x3(DPU RxWQ, ie.SMAC_BMUWQ_DPU_RX), DPU would overwrite this byte with DPU Rx feedback code, see
                                            * SWBD_DPURX_XXX defined above.
                                           */
#else
    uint32_t dpuFeedback:8;
    uint32_t smacPushWq:8;
    uint32_t penultimatePduIdx:16;
#endif

    /* 0x08 */
#ifdef ANI_BIG_BYTE_ENDIAN
    uint32_t headPduIdx : 16;                /* Head PDU index */
    uint32_t tailPduIdx : 16;                /* Tail PDU index */
#else
    uint32_t tailPduIdx : 16;
    uint32_t headPduIdx : 16;
#endif

    /* 0x0c */
#ifdef ANI_BIG_BYTE_ENDIAN
    //
    // When isAmsduSubframe==1:
    //  if mpduHeaderOffset = mpduHeaderLength =0
    //     frame is first subframe with base WLAN header
    //  else
    //     frame is non-first subframe.
    //
    uint32_t mpduHeaderLength : 8;           /* MPDU header length */
    uint32_t mpduHeaderOffset : 8;           /* MPDU header start offset */
    uint32_t mpduDataOffset : 9;             /* MPDU data start offset */
    uint32_t pduCount : 7;                   /* PDU count */
#else
    uint32_t pduCount : 7;
    uint32_t mpduDataOffset : 9;
    uint32_t mpduHeaderOffset : 8;
    uint32_t mpduHeaderLength : 8;
#endif

    /* 0x10 */
#ifdef ANI_BIG_BYTE_ENDIAN
    uint32_t mpduLength : 16;                /* MPDU length */
    uint32_t reserved3 : 4;
    uint32_t tid : 4;                        /* Traffic identifier, tid */

    //Rate Index reported by RxP
    //  0-127: Titan Rates
    //  128-159: 11n MCS #0-15, 20Mhz
    //  160-191: 11n MCS #0-15, 40Mhz
    //  192-199: Titan legacy duplicate mode (6,9,12,...48,54Mbps)
    //  200-201: 11n MCS #32, 40Mhz duplicate mode
    //  204-207: Airgo 11n proprietary rates
    //  208-223: 11b rates, non duplicate
    //  224-239: 11b rate, duplicate mode
    uint32_t Rsvd_rateIndex : 8;                  /* Rate Index */
#else
    uint32_t Rsvd_rateIndex : 8;
    uint32_t tid : 4;
    uint32_t reserved3 : 4;
    uint32_t mpduLength : 16;
#endif

    /* 0x14 */
#ifdef ANI_BIG_BYTE_ENDIAN

    // DPU descriptor index, filled by SoftMac Rx for DPU
    uint32_t dpuDescIdx:8;

    //When rxpFlags & SMAC_HWBD_RXFLAG_ADDRx_INVALID (x=1,2,3) is NOT set,
    //corresponding addr index field is valid.
    //Value:
    //  0~253: valid
    //  254: unknown unicast
    //  255: unknown multicst and broadcast
    //When SMAC_HWBD_RXFLAG_ADDRx_INVALID (x=1,2,3) is SET, corresponding
    //addrXIndex(X=1,2,3) values are invalid an are set to 253 by RxP.
    uint32_t addr1Index:8;                      /* binary & filter search result for Addr1 from RxP*/
    uint32_t addr2Index:8;                      /* binary & filter search result for Addr2 from RxP*/
    uint32_t addr3Index:8;                      /* binary & filter search result for Addr3 from RxP*/
#else
    uint32_t addr3Index:8;
    uint32_t addr2Index:8;
    uint32_t addr1Index:8;
    uint32_t dpuDescIdx:8;
#endif

    /* 0x18, see SMAC_HWBD_RXFLAG_XXXX */
    uint32_t rxpFlags:23;                       /* RxP flags*/
    uint32_t rateIndex:9;

    /* 0x1c, 20 */
    uint32_t phyStats0;                      /* PHY status word 0*/
    uint32_t phyStats1;                      /* PHY status word 0*/

    /* 0x24 */
    uint32_t mclkRxTimestamp;                /* Rx timestamp, microsecond based*/

    /* 0x28~0x3f */
    uint32_t rxPmiCmd[RXP_PMI_CMD_DW_COUNT];                /* PMI cmd rcvd from RxP */

    /* 0x40 */
#ifdef ANI_BIG_BYTE_ENDIAN
    uint32_t reserved4:1;
    uint32_t rxReorderOpcode:3;
    uint32_t ibssTsfLater:1;    // 1 : rcvd beacon has TSF later than ours.
    uint32_t ibssBcnSent:1;     // 1: we have sent beacon in this TBTT.
    uint32_t scanLearn:1;        /* bit=1: Frame is rcvd during SCAN/Learn mode */
    //uint32_t isAmsduSubframe:1;  // If AMSDU decode is enabled and this bit set,
                                 // this frame is a deaggregated subframe in AMSDU */
    uint32_t rsvd:1;

    uint32_t reason:8;           /* Reason code from SoftMac why the frame is passed to host */

    //Valid only when
    // 1. swBdType=SMAC_SWBD_TYPE_DATA and
    // 2. SoftMac not in promisc mode and
    // 3. frame is QoS and the corresponding TID enabled BA
    //
    //For each <ta, ra, tid> tuple, host maintains an Rx reorder buffer,
    //for each Qos frame sent from SoftMac, Host checks the following two
    //index and take appropriate actions.
    //
    //rxReorderHeadIdx: Index in the Rx reorder buffer of the first MSDU to be indicated to Host
    //rxReorderEnqIdx: Index in the Rx reorder buffer where this frame should be queued
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
    uint32_t rxReorderHeadIdx: 7;
    uint32_t rxReorderEnqIdx: 6;
    uint32_t respFrmType:3;
#else
    uint32_t respFrmType:3;
    uint32_t rxReorderEnqIdx: 6;
    uint32_t rxReorderHeadIdx: 7;
    uint32_t reason:8;
    uint32_t rsvd:1;
    uint32_t scanLearn:1;
    uint32_t ibssBcnSent:1;
    uint32_t ibssTsfLater:1;
    uint32_t rxReorderOpcode:3;
    uint32_t reserved4:1;
#endif

    /* 0x44 */
#ifdef ANI_BIG_BYTE_ENDIAN
    uint32_t reserved5:24;		/** Changed from 11 */
    //value = cca counter between our TBTT and beacon received time.
    //this will be used only in IBSS case.
    //This value will be provided to host to calculate TBTT correction.
    uint32_t ccaCnt:8;
#else
    uint32_t ccaCnt:8;
    uint32_t reserved5:24;
#endif

    /* 0x48 */
#ifdef ANI_BIG_BYTE_ENDIAN
    uint32_t isAmsduSubframe:1;
    uint32_t isFirstAmsduSubFrame:1;
    uint32_t isLastAmsduSubFrame:1;
    uint32_t isAmsduErrorFlag:1;
    uint32_t reserved6:4;		/** Changed from 11 */
    uint32_t processOrder:4;
    uint32_t subFrameIndex:4;
    uint32_t totalAmsduSize:16; //if isAmsduSubframe=1, this field keeps AMSDU MPDU length before deaggregation.
#else
    uint32_t totalAmsduSize:16;
    uint32_t subFrameIndex:4;
    uint32_t processOrder:4;
    uint32_t reserved6:4;
    uint32_t isAmsduErrorFlag:1;
    uint32_t isLastAmsduSubFrame:1;
    uint32_t isFirstAmsduSubFrame:1;
    uint32_t isAmsduSubframe:1;
#endif

    //uint32_t smacProcessCpuCycles;

} __attribute__((packed)) __attribute__((aligned (4))) BDRXTemplate_t, *pBDRXTemplate;
#endif

#endif /* BD_H_ */
