/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _HAL_INT_MMAP_H_
#define _HAL_INT_MMAP_H_

#include "hal_api_sys.h"
#include "hal_int_cfg.h"

/* Warning : The address for BMU packet memory and HW descriptors are configured in the below macros and this address
 * range is reserved in RAM by linker script. It is mandatory to update the linker script if there is necessity to
 * change the address of BMU Packet Memory & HW descriptors  */
extern unsigned int _ln_RAM_start_addr_hw_desc__;    // base address for hardware descriptors
extern unsigned int _ln_RAM_start_addr_hw_pktmem__;  // base address for packet memory

#define HAL_MMAP_START_ADDR      0x00000  // must be 4byte aligned, all hw templates/desc stored here
#define HAL_MMAP_BMU_PKTMEM_ADDR 0x00000  // BMU packet memory start address (20000 == 128k)
#ifdef SUPPORT_FERMION_BMU_64K
#define HAL_MMAP_BMU_PKTMEM_SZ 0x10000  // BMU packet memory size
#else
#define HAL_MMAP_BMU_PKTMEM_SZ 0x8000  // BMU packet memory size
#endif

#define HAL_MMAP_NDPU      (HAL_NBSS_MAX + HAL_NSTA_MAX)  // num DPU entries
#define HAL_MMAP_HWQ_MAX   0x0B                           // hw q's supported (per STA q's)
#define HAL_MMAP_TXFRM_MAX 512                            // max tx frames
#define HAL_MMAP_RATES_MAX 33                             // max number of supported tx rates

#undef HAL_CFG_BIGBYTE_ENDIAN

// -------------------------------------------------------
// hardware data structures definitions and access macros

#ifdef HAL_CFG_BIGBYTE_ENDIAN
#define HAL_DPU_RCIDX_SET(dpuDesc, tid, value)                                             \
    do {                                                                                   \
        ((uint8_t *)((hal_dpu_desc_t *)(dpuDesc))->idxPerTidReplayCount)[(tid)] = (value); \
    } while (0)

#define HAL_DPU_RCIDX_GET(dpuDesc, tid) (((uint8_t *)(dpuDesc)->idxPerTidReplayCount)[(tid)])
#else
#define HAL_DPU_RCIDX_TO_BYTEIDX(tid) (((tid) & ~3) | (3 - ((tid)&3)))
#define HAL_DPU_RCIDX_SET(dpuDesc, tid, value)                                                                     \
    do {                                                                                                           \
        ((uint8_t *)((hal_dpu_desc_t *)(dpuDesc))->idxPerTidReplayCount)[HAL_DPU_RCIDX_TO_BYTEIDX(tid)] = (value); \
    } while (0)
#define HAL_DPU_RCIDX_GET(dpuDesc, tid) (((uint8_t *)(dpuDesc)->idxPerTidReplayCount)[HAL_DPU_RCIDX_TO_BYTEIDX(tid)])
#endif

typedef struct hal_dpu_desc_s {
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

    // word 4-7
    // for TID0-15, each 8 bits.
    // Use GET_DPU_RCIDX() or SET_DPU_RCIDX() defined above
    // to read/write this field. It takes care of endian problem.
    uint32_t idxPerTidReplayCount[4];

    // word 8
    uint32_t txSentBlocks;

    // word 9
    uint32_t rxRcvddBlocks;

    // word 10
    uint32_t bipAesTkipReplays;

// word 11
#ifdef HAL_CFG_BIGBYTE_ENDIAN

    uint32_t micErrCount : 8;
    uint32_t excludedCount : 24;

#else

    uint32_t excludedCount : 24;
    uint32_t micErrCount : 8;

#endif

// word 12
#ifdef HAL_CFG_BIGBYTE_ENDIAN

    uint32_t formatErrorCount : 16;

    uint32_t undecryptableCount : 16;
#else
    uint32_t undecryptableCount : 16;
    uint32_t formatErrorCount : 16;
#endif

    // word 13
    uint32_t decryptErrorCount;

    // word 14
    uint32_t decryptSuccessCount;

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

} hal_dpu_desc_t;

#define HAL_DPU_KEY_DESC_LEN 4
typedef struct hal_dpu_key_desc_s {
    uint32_t key128bit[4];
} hal_dpu_key_desc_t;

typedef struct hal_dpu_mic_key_desc_s {
    uint32_t txMicKey64bit[2];
    uint32_t rxMicKey64bit[2];
} hal_dpu_mic_key_desc_t;

typedef struct hal_dpu_rply_desc_s {
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

} hal_dpu_rply_desc_t;

/**
 *  TPE STA Desc Rate Info
 */

typedef struct hal_tpe_sta_desc_rate_info_s {
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t reserved : 2;
    uint32_t low_power_en : 1;
    uint32_t protection_mode : 3;
    uint32_t ampdu_density : 7;
    uint32_t tx_power : 5;
    uint32_t min_tx_power : 5;  // BT Co-ex
    uint32_t rate_index : 9;
#else
    uint32_t rate_index : 9;
    uint32_t min_tx_power : 5;  // BT Co-ex
    uint32_t tx_power : 5;
    uint32_t ampdu_density : 7;
    uint32_t protection_mode : 3;
    uint32_t low_power_en : 1;
    uint32_t reserved : 2;
#endif
} hal_tpe_sta_desc_rate_info_t;

/**
 *  TPE STA dot11 MIB stats
 */
typedef struct hal_tpe_sta_desc_dot11_stats_s {
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    // word 0
    uint32_t tx_fragment_cnt : 16;
    uint32_t tx_success_frm_cnt : 16;
    // word 1
    uint32_t tx_fail_cnt : 16;
    uint32_t tx_mc_frm_cnt : 16;
    // word 2
    uint32_t tx_retry_cnt : 16;
    uint32_t tx_mult_retry_cnt : 16;
    // word 3
    uint32_t tx_rts_success_cnt : 16;
    uint32_t tx_rts_fail_cnt : 16;
    // word 4
    uint32_t tx_ack_success_cnt : 16;
    uint32_t tx_ack_fail_cnt : 16;
#else
    // word 0
    uint32_t tx_success_frm_cnt : 16;
    uint32_t tx_fragment_cnt : 16;
    // word 1
    uint32_t tx_mc_frm_cnt : 16;
    uint32_t tx_fail_cnt : 16;
    // word 2
    uint32_t tx_mult_retry_cnt : 16;
    uint32_t tx_retry_cnt : 16;
    // word 3
    uint32_t tx_rts_fail_cnt : 16;
    uint32_t tx_rts_success_cnt : 16;
    // word 4
    uint32_t tx_ack_fail_cnt : 16;
    uint32_t tx_ack_success_cnt : 16;
#endif
} hal_tpe_sta_desc_dot11_stats_t;
/**
 *  TPE STA Desc
 */

typedef struct hal_tpe_sta_desc_s {
    // 0x00
    uint32_t macAddr1Lo;

// 0x04
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t macAddr2Lo : 16;
    uint32_t macAddr1Hi : 16;
#else
    uint32_t macAddr1Hi : 16;
    uint32_t macAddr2Lo : 16;
#endif

    // 0x08
    uint32_t macAddr2Hi;

// 0x0c
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t rtt_qid : 5;
    uint32_t rtt_mode : 2;
    uint32_t protection_type : 2;
    uint32_t reserved0 : 9;
    uint32_t sa_high : 1;  // BT Co-ex
    uint32_t sa_low : 1;   // BT Co-ex
    uint32_t retry_threshold2 : 4;
    uint32_t retry_threshold1 : 4;
    uint32_t retry_threshold0 : 4;
#else
    uint32_t retry_threshold0 : 4;
    uint32_t retry_threshold1 : 4;
    uint32_t retry_threshold2 : 4;
    uint32_t sa_low : 1;   // BT Co-ex
    uint32_t sa_high : 1;  // BT Co-ex
    uint32_t reserved0 : 9;
    uint32_t protection_type : 2;
    uint32_t rtt_mode : 2;
    uint32_t rtt_qid : 5;
#endif

    // 0x10
    uint32_t ack_policy_vectorLo;

// 0x14
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t isDyn : 1;
    uint32_t bw_en : 1;
    uint32_t en_80m : 1;
    uint32_t en_40m : 1;
    uint32_t from_ds : 1;
    uint32_t to_ds : 1;
    uint32_t pm : 1;
    uint32_t ampdu_valid : 19;
    uint32_t ack_policy_vectorHi : 6;
#else
    uint32_t ack_policy_vectorHi : 6;
    uint32_t ampdu_valid : 19;
    uint32_t pm : 1;
    uint32_t to_ds : 1;
    uint32_t from_ds : 1;
    uint32_t en_40m : 1;
    uint32_t en_80m : 1;
    uint32_t bw_en : 1;
    uint32_t isDyn : 1;
#endif

    // 0x18
    hal_tpe_sta_desc_rate_info_t rate_params_20Mhz[HAL_RETRY_RATES_NUM_MAX];
    // 0x24
    hal_tpe_sta_desc_rate_info_t rate_params_bd[HAL_RETRY_RATES_NUM_MAX];

// 0x30
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t tx_priority : 4;  // BT Co-ex
    uint32_t en_lt : 16;
    uint32_t data_wt_cycles : 12;
#else
    uint32_t data_wt_cycles : 12;
    uint32_t en_lt : 16;
    uint32_t tx_priority : 4;  // BT Co-ex
#endif

// 0x34
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t reserved1 : 1;
    uint32_t max_bytes_in_ampdu : 3;
    uint32_t bssid_of_sta : 4;
    uint32_t tsfoffset_for_probe_resp_bd_rate_0_1 : 12;
    uint32_t tsfoffset_for_probe_resp_bd_rate_2_3 : 12;
#else
    uint32_t tsfoffset_for_probe_resp_bd_rate_2_3 : 12;
    uint32_t tsfoffset_for_probe_resp_bd_rate_0_1 : 12;
    uint32_t bssid_of_sta : 4;
    uint32_t max_bytes_in_ampdu : 3;
    uint32_t reserved1 : 1;
#endif

    // 0x38 in fermion
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t reserved2 : 1;
    uint32_t ltt_shift : 2;
    uint32_t lifetime_threshold : 10;
    uint32_t bd_raw_mode : 19;
#else
    uint32_t bd_raw_mode : 19;
    uint32_t lifetime_threshold : 10;
    uint32_t ltt_shift : 2;
    uint32_t reserved2 : 1;
#endif

    // 0x3c in fermion
    uint32_t mcbcStatsQidMap;
// 0x40 in fermion
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t tx_boundary_en : 4;
    uint32_t reserved3 : 2;
    uint32_t ampdu_window_sz : 6;  // Neutrino 2
    uint32_t un : 2;
    uint32_t gid : 6;
    uint32_t mu : 1;
    uint32_t partial_aid : 11;
#else
    uint32_t partial_aid : 11;
    uint32_t mu : 1;
    uint32_t gid : 6;
    uint32_t un : 2;
    uint32_t ampdu_window_sz : 6;  // Neutrino 2
    uint32_t reserved3 : 2;
    uint32_t tx_boundary_en : 4;
#endif

// 0x44
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t reserved4 : 2;
    uint32_t ampdu_window_size_qid4 : 6;
    uint32_t ampdu_window_size_qid3 : 6;
    uint32_t ampdu_window_size_qid2 : 6;
    uint32_t ampdu_window_size_qid1 : 6;
    uint32_t ampdu_window_size_qid0 : 6;
#else
    uint32_t ampdu_window_size_qid0 : 6;
    uint32_t ampdu_window_size_qid1 : 6;
    uint32_t ampdu_window_size_qid2 : 6;
    uint32_t ampdu_window_size_qid3 : 6;
    uint32_t ampdu_window_size_qid4 : 6;
    uint32_t reserved4 : 2;
#endif
// 0x48
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t tx_priority_qid01 : 4;  // BT Co-ex
    uint32_t ltt_shift01 : 2;
    uint32_t lifetime_threshold_qid01 : 10;
    uint32_t tx_priority_qid00 : 4;  // BT Co-ex
    uint32_t ltt_shift00 : 2;
    uint32_t lifetime_threshold_qid00 : 10;
#else
    uint32_t lifetime_threshold_qid00 : 10;
    uint32_t ltt_shift00 : 2;
    uint32_t tx_priority_qid00 : 4;  // BT Co-ex
    uint32_t lifetime_threshold_qid01 : 10;
    uint32_t ltt_shift01 : 2;
    uint32_t tx_priority_qid01 : 4;  // BT Co-ex
#endif

// 0x4C
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t tx_priority_qid03 : 4;  // BT Co-ex
    uint32_t ltt_shift03 : 2;
    uint32_t lifetime_threshold_qid03 : 10;
    uint32_t tx_priority_qid02 : 4;  // BT Co-ex
    uint32_t ltt_shift02 : 2;
    uint32_t lifetime_threshold_qid02 : 10;
#else
    uint32_t lifetime_threshold_qid02 : 10;
    uint32_t ltt_shift02 : 2;
    uint32_t tx_priority_qid02 : 4;  // BT Co-ex
    uint32_t lifetime_threshold_qid03 : 10;
    uint32_t ltt_shift03 : 2;
    uint32_t tx_priority_qid03 : 4;  // BT Co-ex
#endif
// 0x50
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t reserved5 : 2;
    uint32_t ampdu_window_size_qid9 : 6;
    uint32_t ampdu_window_size_qid8 : 6;
    uint32_t ampdu_window_size_qid7 : 6;
    uint32_t ampdu_window_size_qid6 : 6;
    uint32_t ampdu_window_size_qid5 : 6;
#else
    uint32_t ampdu_window_size_qid5 : 6;
    uint32_t ampdu_window_size_qid6 : 6;
    uint32_t ampdu_window_size_qid7 : 6;
    uint32_t ampdu_window_size_qid8 : 6;
    uint32_t ampdu_window_size_qid9 : 6;
    uint32_t reserved5 : 2;
#endif
// 0x54
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t tx_priority_qid05 : 4;  // BT Co-ex
    uint32_t ltt_shift05 : 2;
    uint32_t lifetime_threshold_qid05 : 10;
    uint32_t tx_priority_qid04 : 4;  // BT Co-ex
    uint32_t ltt_shift04 : 2;
    uint32_t lifetime_threshold_qid04 : 10;
#else
    uint32_t lifetime_threshold_qid04 : 10;
    uint32_t ltt_shift04 : 2;
    uint32_t tx_priority_qid04 : 4;  // BT Co-ex
    uint32_t lifetime_threshold_qid05 : 10;
    uint32_t ltt_shift05 : 2;
    uint32_t tx_priority_qid05 : 4;  // BT Co-ex
#endif

// 0x58
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t tx_priority_qid07 : 4;  // BT Co-ex
    uint32_t ltt_shift07 : 2;
    uint32_t lifetime_threshold_qid07 : 10;
    uint32_t tx_priority_qid06 : 4;  // BT Co-ex
    uint32_t ltt_shift06 : 2;
    uint32_t lifetime_threshold_qid06 : 10;
#else
    uint32_t lifetime_threshold_qid06 : 10;
    uint32_t ltt_shift06 : 2;
    uint32_t tx_priority_qid06 : 4;  // BT Co-ex
    uint32_t lifetime_threshold_qid07 : 10;
    uint32_t ltt_shift07 : 2;
    uint32_t tx_priority_qid07 : 4;  // BT Co-ex
#endif

// 0x5c
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t reserved6 : 2;
    uint32_t ampdu_window_size_qid14 : 6;
    uint32_t ampdu_window_size_qid13 : 6;
    uint32_t ampdu_window_size_qid12 : 6;
    uint32_t ampdu_window_size_qid11 : 6;
    uint32_t ampdu_window_size_qid10 : 6;
#else
    uint32_t ampdu_window_size_qid10 : 6;
    uint32_t ampdu_window_size_qid11 : 6;
    uint32_t ampdu_window_size_qid12 : 6;
    uint32_t ampdu_window_size_qid13 : 6;
    uint32_t ampdu_window_size_qid14 : 6;
    uint32_t reserved6 : 2;
#endif
// 0x60
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t tx_priority_qid09 : 4;  // BT Co-ex
    uint32_t ltt_shift09 : 2;
    uint32_t lifetime_threshold_qid09 : 10;
    uint32_t tx_priority_qid08 : 4;  // BT Co-ex
    uint32_t ltt_shift08 : 2;
    uint32_t lifetime_threshold_qid08 : 10;
#else
    uint32_t lifetime_threshold_qid08 : 10;
    uint32_t ltt_shift08 : 2;
    uint32_t tx_priority_qid08 : 4;  // BT Co-ex
    uint32_t lifetime_threshold_qid09 : 10;
    uint32_t ltt_shift09 : 2;
    uint32_t tx_priority_qid09 : 4;  // BT Co-ex
#endif

// 0x64
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t tx_priority_qid11 : 4;  // BT Co-ex
    uint32_t ltt_shift11 : 2;
    uint32_t lifetime_threshold_qid11 : 10;
    uint32_t tx_priority_qid10 : 4;  // BT Co-ex
    uint32_t ltt_shift10 : 2;
    uint32_t lifetime_threshold_qid10 : 10;
#else
    uint32_t lifetime_threshold_qid10 : 10;
    uint32_t ltt_shift10 : 2;
    uint32_t tx_priority_qid10 : 4;  // BT Co-ex
    uint32_t lifetime_threshold_qid11 : 10;
    uint32_t ltt_shift11 : 2;
    uint32_t tx_priority_qid11 : 4;  // BT Co-ex
#endif

// 0x68
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t reserved7 : 8;
    uint32_t ampdu_window_size_qid18 : 6;
    uint32_t ampdu_window_size_qid17 : 6;
    uint32_t ampdu_window_size_qid16 : 6;
    uint32_t ampdu_window_size_qid15 : 6;
#else
    uint32_t ampdu_window_size_qid15 : 6;
    uint32_t ampdu_window_size_qid16 : 6;
    uint32_t ampdu_window_size_qid17 : 6;
    uint32_t ampdu_window_size_qid18 : 6;
    uint32_t reserved7 : 8;
#endif

// 0x6C
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t tx_priority_qid13 : 4;  // BT Co-ex
    uint32_t ltt_shift13 : 2;
    uint32_t lifetime_threshold_qid13 : 10;
    uint32_t tx_priority_qid12 : 4;  // BT Co-ex
    uint32_t ltt_shift12 : 2;
    uint32_t lifetime_threshold_qid12 : 10;
#else
    uint32_t lifetime_threshold_qid12 : 10;
    uint32_t ltt_shift12 : 2;
    uint32_t tx_priority_qid12 : 4;  // BT Co-ex
    uint32_t lifetime_threshold_qid13 : 10;
    uint32_t ltt_shift13 : 2;
    uint32_t tx_priority_qid13 : 4;  // BT Co-ex
#endif

// 0x70
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t tx_priority_qid15 : 4;  // BT Co-ex
    uint32_t ltt_shift15 : 2;
    uint32_t lifetime_threshold_qid15 : 10;
    uint32_t tx_priority_qid14 : 4;  // BT Co-ex
    uint32_t ltt_shift14 : 2;
    uint32_t lifetime_threshold_qid14 : 10;
#else
    uint32_t lifetime_threshold_qid14 : 10;
    uint32_t ltt_shift14 : 2;
    uint32_t tx_priority_qid14 : 4;  // BT Co-ex
    uint32_t lifetime_threshold_qid15 : 10;
    uint32_t ltt_shift15 : 2;
    uint32_t tx_priority_qid15 : 4;  // BT Co-ex
#endif
    // 0x74 - 0xF0
    uint32_t reserved74[(0xF4 - 0x74) / 4];
// 0xF4 in fermion
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t pri_20m_tx_ppdu_cnt : 16;
    uint32_t pri_20m_tx_mpdu_cnt : 16;
#else
    uint32_t pri_20m_tx_mpdu_cnt : 16;
    uint32_t pri_20m_tx_ppdu_cnt : 16;
#endif

// 0xF8 in fermion
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t pri_20m_tx_mpdu_in_ampdu : 16;
    uint32_t pri_20m_tx_ppdu_ack_to : 16;
#else
    uint32_t pri_20m_tx_ppdu_ack_to : 16;
    uint32_t pri_20m_tx_mpdu_in_ampdu : 16;
#endif
    // 0xFC in fermion
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t pri_20m_tx_mpdu_responded : 16;
    uint32_t pri_20m_tx_pwr_reduct_cnt : 16;
#else
    uint32_t pri_20m_tx_pwr_reduct_cnt : 16;
    uint32_t pri_20m_tx_mpdu_responded : 16;
#endif
    // 0x100 in fermion
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t pri_20m_tx_pwr_reduct_to_cnt : 16;
    uint32_t pri_20m_bt_abort_tx_cnt : 16;
#else
    uint32_t pri_20m_bt_abort_tx_cnt : 16;
    uint32_t pri_20m_tx_pwr_reduct_to_cnt : 16;
#endif
    // 0x104 in fermion
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t pri_20m_bt_abort_rx_cnt : 16;
    uint32_t sec_20m_tx_ppdu_cnt : 16;
#else
    uint32_t sec_20m_tx_ppdu_cnt : 16;
    uint32_t pri_20m_bt_abort_rx_cnt : 16;
#endif
    // 0x108 in fermion
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t sec_20m_tx_mpdu_cnt : 16;
    uint32_t sec_20m_tx_mpdu_in_ampdu : 16;
#else
    uint32_t sec_20m_tx_mpdu_in_ampdu : 16;
    uint32_t sec_20m_tx_mpdu_cnt : 16;
#endif
    // 0x10C in fermion
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t sec_20m_tx_ppdu_ack_to : 16;
    uint32_t sec_20m_tx_mpdu_responded : 16;
#else
    uint32_t sec_20m_tx_mpdu_responded : 16;
    uint32_t sec_20m_tx_ppdu_ack_to : 16;
#endif
    // 0x110 in fermion
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t sec_20m_tx_pwr_reduct_cnt : 16;
    uint32_t sec_20m_tx_pwr_reduct_to_cnt : 16;
#else
    uint32_t sec_20m_tx_pwr_reduct_to_cnt : 16;
    uint32_t sec_20m_tx_pwr_reduct_cnt : 16;
#endif
    // 0x114 in fermion
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t sec_20m_bt_abort_tx_cnt : 16;
    uint32_t sec_20m_bt_abort_rx_cnt : 16;
#else
    uint32_t sec_20m_bt_abort_rx_cnt : 16;
    uint32_t sec_20m_bt_abort_tx_cnt : 16;
#endif
    // 0x118 in fermion
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t ter_20m_tx_ppdu_cnt : 16;
    uint32_t ter_20m_tx_mpdu_cnt : 16;
#else
    uint32_t ter_20m_tx_mpdu_cnt : 16;
    uint32_t ter_20m_tx_ppdu_cnt : 16;
#endif
    // 0x11c in fermion
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t ter_20m_tx_mpdu_in_ampdu : 16;
    uint32_t ter_20m_tx_ppdu_ack_to : 16;
#else
    uint32_t ter_20m_tx_ppdu_ack_to : 16;
    uint32_t ter_20m_tx_mpdu_in_ampdu : 16;
#endif
    // 0x120 in fermion
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t ter_20m_tx_mpdu_responded : 16;
    uint32_t ter_20m_tx_pwr_reduct_cnt : 16;
#else
    uint32_t ter_20m_tx_pwr_reduct_cnt : 16;
    uint32_t ter_20m_tx_mpdu_responded : 16;
#endif
    // 0x124 in fermion
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t ter_20m_tx_pwr_reduct_to_cnt : 16;
    uint32_t ter_20m_bt_abort_tx_cnt : 16;
#else
    uint32_t ter_20m_bt_abort_tx_cnt : 16;
    uint32_t ter_20m_tx_pwr_reduct_to_cnt : 16;
#endif
    // 0x128 in fermion
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t ter_20m_bt_abort_rx_cnt : 16;
    uint32_t resv128 : 16;
#else
    uint32_t resv128 : 16;
    uint32_t ter_20m_bt_abort_rx_cnt : 16;
#endif

    // 0x12C-0x160
    uint32_t reserved27[(0x160 - 0x12C) / 4];
    // 0x160 - 1FC
    hal_tpe_sta_desc_dot11_stats_t dot11stats[8];

} hal_tpe_sta_desc_t;

/**
 * 	RPE Queue Info
 */
typedef struct hal_rpe_queue_info_s {
// 0x00
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t ba_ssn : 12;
    uint32_t ssn_sval : 1;
    uint32_t ba_window_size : 6;
    uint32_t check_2k : 1;
    uint32_t frg : 1;
    uint32_t ord : 1;
    uint32_t fsh : 1;
    uint32_t rty : 1;
    uint32_t psr : 1;
    uint32_t bar : 1;
    uint32_t reserved1 : 5;
    uint32_t val : 1;
#else
    uint32_t val : 1;
    uint32_t reserved1 : 5;
    uint32_t bar : 1;
    uint32_t psr : 1;
    uint32_t rty : 1;
    uint32_t fsh : 1;
    uint32_t ord : 1;
    uint32_t frg : 1;
    uint32_t check_2k : 1;
    uint32_t ba_window_size : 6;
    uint32_t ssn_sval : 1;
    uint32_t ba_ssn : 12;
#endif
    // 0x04
    uint32_t staId_queueId_BAbitmap31_0;
    // 0x08
    uint32_t staId_queueId_BAbitmap63_32;
    // 0x0c
    uint32_t staId_queueId_Reorderbitmap31_0;
    // 0x10
    uint32_t staId_queueId_Reorderbitmap63_32;
// 0x14
#ifdef HAL_CFG_BIGBYTE_ENDIAN
    uint32_t reserved2 : 6;
    uint32_t rod : 1;
    uint32_t current_index : 6;
    uint32_t reorder_window_size : 6;
    uint32_t reorder_sval : 1;
    uint32_t reorder_ssn : 12;
#else
    uint32_t reorder_ssn : 12;
    uint32_t reorder_sval : 1;
    uint32_t reorder_window_size : 6;
    uint32_t current_index : 6;
    uint32_t rod : 1;
    uint32_t reserved2 : 6;
#endif
    // 0x18
    uint32_t rsvd1;
    // 0x1c
    uint32_t rsvd2;
} hal_rpe_queue_info_t;

typedef struct hal_rpe_sta_desc_s {
    hal_rpe_queue_info_t rpeStaQueueInfo[HAL_MMAP_HWQ_MAX];
} hal_rpe_sta_desc_t;

// ---------------------------------------------------
// M E M O R Y   M A P P I N G

// ---------------------------------------------------
// DPU: descriptors + keys + micKeys + replay counters
#define HAL_MMAP_DPU_START_ADDR (HAL_MMAP_START_ADDR)

#define HAL_MMAP_DPU_DESC_OFST (HAL_MMAP_DPU_START_ADDR)
#define HAL_MMAP_DPU_DESC_SZ   (HAL_MMAP_NDPU * sizeof(hal_dpu_desc_t))

#define HAL_MMAP_KEY_DESC_OFST (HAL_MMAP_DPU_DESC_OFST + HAL_MMAP_DPU_DESC_SZ)
#define HAL_MMAP_KEY_DESC_SZ   (HAL_MMAP_NDPU * 4 * sizeof(hal_dpu_key_desc_t))

#define HAL_MMAP_MIC_KEY_OFST (HAL_MMAP_KEY_DESC_OFST + HAL_MMAP_KEY_DESC_SZ)
#define HAL_MMAP_MIC_KEY_SZ   (HAL_MMAP_NDPU * 4 * sizeof(hal_dpu_mic_key_desc_t))

#define HAL_MMAP_RPLY_OFST (HAL_MMAP_MIC_KEY_OFST + HAL_MMAP_MIC_KEY_SZ)
#define HAL_MMAP_RPLY_SZ   (HAL_MMAP_NDPU * HAL_TIDS_MAX * sizeof(hal_dpu_rply_desc_t))

#define HAL_MMAP_DPU_END_ADDR (HAL_MMAP_RPLY_OFST + HAL_MMAP_RPLY_SZ)
#define HAL_MMAP_DPU_SZ       (HAL_MMAP_DPU_END_ADDR - HAL_MMAP_DPU_START_ADDR)

// ---------------------------------------------------
// TPE: descriptors + stats
#define HAL_MMAP_TPE_START_ADDR (HAL_MMAP_DPU_END_ADDR)

#define HAL_MMAP_TPE_DESC_OFST (HAL_MMAP_TPE_START_ADDR)
#define HAL_MMAP_TPE_DESC_SZ   (HAL_NSTA_MAX * sizeof(hal_tpe_sta_desc_t))

#define HAL_MMAP_TPE_END_ADDR (HAL_MMAP_TPE_DESC_OFST + HAL_MMAP_TPE_DESC_SZ)
#define HAL_MMAP_TPE_SZ       (HAL_MMAP_TPE_END_ADDR - HAL_MMAP_TPE_START_ADDR)

// ---------------------------------------------------
// RPE: descriptors
#define HAL_MMAP_RPE_START_ADDR (HAL_MMAP_TPE_END_ADDR)

#define HAL_MMAP_RPE_DESC_OFST (HAL_MMAP_RPE_START_ADDR)
#define HAL_MMAP_RPE_DESC_SZ   (HAL_NSTA_MAX * sizeof(hal_rpe_sta_desc_t))

#define HAL_MMAP_RPE_END_ADDR (HAL_MMAP_RPE_DESC_OFST + HAL_MMAP_RPE_DESC_SZ)
#define HAL_MMAP_RPE_SZ       (HAL_MMAP_RPE_END_ADDR - HAL_MMAP_RPE_START_ADDR)

// ---------------------------------------------------
// BTQM: tx queue entries
#define HAL_MMAP_BTQM_START_ADDR (HAL_MMAP_RPE_END_ADDR)

#define HAL_MMAP_BTQM_ENTRY_SZ 8

#define HAL_MMAP_BTQM_DESC_OFST (HAL_MMAP_BTQM_START_ADDR)
#define HAL_MMAP_BTQM_DESC_SZ   (HAL_NSTA_MAX * HAL_MMAP_HWQ_MAX * HAL_MMAP_BTQM_ENTRY_SZ)

#define HAL_MMAP_BTQM_END_ADDR (HAL_MMAP_BTQM_DESC_OFST + HAL_MMAP_BTQM_DESC_SZ)
#define HAL_MMAP_BTQM_SZ       (HAL_MMAP_BTQM_END_ADDR - HAL_MMAP_BTQM_START_ADDR)

// ---------------------------------------------------
// SW templates
#define HAL_MMAP_SWTP_START_ADDR (HAL_MMAP_BTQM_END_ADDR)

#define HAL_MMAP_SWTP_ENTRY_SZ 0x800

#define HAL_MMAP_SWTP_DESC_OFST (HAL_MMAP_SWTP_START_ADDR)
#define HAL_MMAP_SWTP_DESC_SZ   (HAL_MMAP_SWTP_ENTRY_SZ)

#define HAL_MMAP_SWTP_END_ADDR (HAL_MMAP_SWTP_DESC_OFST + HAL_MMAP_SWTP_DESC_SZ)
#define HAL_MMAP_SWTP_SZ       (HAL_MMAP_SWTP_END_ADDR - HAL_MMAP_SWTP_START_ADDR)
//------------------------------------------------------
// CTS to self SW templates
#define HAL_MMAP_CTS_SWTP_START_ADDR (HAL_MMAP_SWTP_START_ADDR)

#define HAL_MMAP_CTS_SWTP_ENTRY_SZ 0x28

#define HAL_MMAP_CTS_SWTP_DESC_OFST (HAL_MMAP_CTS_SWTP_START_ADDR)
#define HAL_MMAP_CTS_SWTP_DESC_SZ   (HAL_MMAP_CTS_SWTP_ENTRY_SZ)

#define HAL_MMAP_CTS_SWTP_END_ADDR (HAL_MMAP_CTS_SWTP_DESC_OFST + HAL_MMAP_CTS_SWTP_DESC_SZ)
#define HAL_MMAP_CTS_SWTP_SZ       (HAL_MMAP_CTS_SWTP_END_ADDR - HAL_MMAP_CTS_SWTP_START_ADDR)

//-----------------------------------------------------
// PS Poll SW templates
#define HAL_MMAP_PS_POLL_SWTP_START_ADDR (HAL_MMAP_CTS_SWTP_END_ADDR)

#define HAL_MMAP_PS_POLL_SWTP__ENTRY_SZ 0x2C

#define HAL_MMAP_PS_POLL_SWTP_DESC_OFST (HAL_MMAP_PS_POLL_SWTP_START_ADDR)
#define HAL_MMAP_PS_POLL_SWTP_DESC_SZ   (HAL_MMAP_PS_POLL_SWTP__ENTRY_SZ)

#define HAL_MMAP_PS_POLL_SWTP_END_ADDR (HAL_MMAP_PS_POLL_SWTP_DESC_OFST + HAL_MMAP_PS_POLL_SWTP_DESC_SZ)
#define HAL_MMAP_PS_POLL_SWTP_SZ       (HAL_MMAP_PS_POLL_SWTP_END_ADDR - HAL_MMAP_PS_POLL_SWTP_START_ADDR)

//-----------------------------------------------------
// QOS NULL SW templates
#define HAL_MMAP_QOS_NULL_SWTP_START_ADDR (HAL_MMAP_PS_POLL_SWTP_END_ADDR)

#define HAL_MMAP_QOS_NULL_SWTP_ENTRY_SZ 0x38

#define HAL_MMAP_QOS_NULL_SWTP_DESC_OFST (HAL_MMAP_QOS_NULL_SWTP_START_ADDR)
#define HAL_MMAP_QOS_NULL_SWTP_DESC_SZ   (HAL_MMAP_QOS_NULL_SWTP_ENTRY_SZ)

#define HAL_MMAP_QOS_NULL_SWTP_END_ADDR (HAL_MMAP_QOS_NULL_SWTP_DESC_OFST + HAL_MMAP_QOS_NULL_SWTP_DESC_SZ)
#define HAL_MMAP_QOS_NULL_SWTP_SZ       (HAL_MMAP_QOS_NULL_SWTP_END_ADDR - HAL_MMAP_QOS_NULL_SWTP_START_ADDR)

//-----------------------------------------------------
// Data NULL SW templates
#define HAL_MMAP_DATA_NULL_SWTP_START_ADDR (HAL_MMAP_QOS_NULL_SWTP_END_ADDR)

#define HAL_MMAP_DATA_NULL_SWTP_ENTRY_SZ 0x34

#define HAL_MMAP_DATA_NULL_SWTP_DESC_OFST (HAL_MMAP_DATA_NULL_SWTP_START_ADDR)
#define HAL_MMAP_DATA_NULL_SWTP_DESC_SZ   (HAL_MMAP_DATA_NULL_SWTP_ENTRY_SZ)

#define HAL_MMAP_DATA_NULL_SWTP_END_ADDR (HAL_MMAP_DATA_NULL_SWTP_DESC_OFST + HAL_MMAP_DATA_NULL_SWTP_DESC_SZ)
#define HAL_MMAP_DATA_NULL_SWTP_SZ       (HAL_MMAP_DATA_NULL_SWTP_END_ADDR - HAL_MMAP_DATA_NULL_SWTP_START_ADDR)

//-----------------------------------------------------
// TEMP SW templates
#define HAL_MMAP_TEMP_SWTP_START_ADDR (HAL_MMAP_DATA_NULL_SWTP_END_ADDR)

#define HAL_MMAP_TEMP_SWTP_ENTRY_SZ 0x4C

#define HAL_MMAP_TEMP_SWTP_DESC_OFST (HAL_MMAP_TEMP_SWTP_START_ADDR)
#define HAL_MMAP_TEMP_SWTP_DESC_SZ   (HAL_MMAP_TEMP_SWTP_ENTRY_SZ)

#define HAL_MMAP_TEMP_SWTP_END_ADDR (HAL_MMAP_TEMP_SWTP_DESC_OFST + HAL_MMAP_TEMP_SWTP_DESC_SZ)
#define HAL_MMAP_TEMP_SWTP_SZ       (HAL_MMAP_TEMP_SWTP_END_ADDR - HAL_MMAP_TEMP_SWTP_START_ADDR)

// ---------------------------------------------------
// Beacon template
#define HAL_MMAP_BCNTP_START_ADDR (HAL_MMAP_SWTP_END_ADDR)

#define HAL_MMAP_BCNTP_ENTRY_SZ 0x400

#define HAL_MMAP_BCNTP_DESC_OFST (HAL_MMAP_BCNTP_START_ADDR)
#define HAL_MMAP_BCNTP_DESC_SZ   (HAL_MMAP_BCNTP_ENTRY_SZ)

#define HAL_MMAP_BCNTP_END_ADDR (HAL_MMAP_BCNTP_DESC_OFST + HAL_MMAP_BCNTP_DESC_SZ)
#define HAL_MMAP_BCNTP_SZ       (HAL_MMAP_BCNTP_END_ADDR - HAL_MMAP_BCNTP_START_ADDR)

// ---------------------------------------------------
// RxP Address table
#define HAL_MMAP_RXP_START_ADDR (HAL_MMAP_BCNTP_END_ADDR)

#define HAL_MMAP_RXP_ENTRY_SZ 0x100

#define HAL_MMAP_RXP_DESC_OFST (HAL_MMAP_RXP_START_ADDR)
#define HAL_MMAP_RXP_DESC_SZ   (HAL_MMAP_RXP_ENTRY_SZ)

#define HAL_MMAP_RXP_END_ADDR (HAL_MMAP_RXP_DESC_OFST + HAL_MMAP_RXP_DESC_SZ)
#define HAL_MMAP_RXP_SZ       (HAL_MMAP_RXP_END_ADDR - HAL_MMAP_RXP_START_ADDR)

// ---------------------------------------------------
// RRI region
#define HAL_MMAP_RRI_START_ADDR (HAL_MMAP_RXP_END_ADDR)

#define HAL_MMAP_RRI_ENTRY_SZ 0x2000

#define HAL_MMAP_RRI_DESC_OFST (HAL_MMAP_RRI_START_ADDR)
#define HAL_MMAP_RRI_DESC_SZ   (HAL_MMAP_RRI_ENTRY_SZ)

#define HAL_MMAP_RRI_END_ADDR (HAL_MMAP_RRI_DESC_OFST + HAL_MMAP_RRI_DESC_SZ)
#define HAL_MMAP_RRI_SZ       (HAL_MMAP_RRI_END_ADDR - HAL_MMAP_RRI_START_ADDR)

#define HAL_MMAP_END_ADDR (HAL_MMAP_RRI_END_ADDR)
#define HAL_MMAP_SZ       (HAL_MMAP_END_ADDR - HAL_MMAP_START_ADDR)

// ---------------------------------------------------
// BMU packet memory
#define HAL_MMAP_BD_SZ        128
#define HAL_MMAP_BD_NUM       (HAL_MMAP_BMU_PKTMEM_SZ / HAL_MMAP_BD_SZ)
#define HAL_MMAP_PDU_NUM      HAL_MMAP_BD_NUM  // same as bd's - interchangeable
#define HAL_MMAP_BD_START_IDX 1                // index 0 is reserved
#define HAL_MMAP_BD_END_IDX   (HAL_MMAP_BD_NUM - 1)
#define HAL_MMAP_AUTO_BD_ITER 5000  // wait this long for auto bd linking to finish

#define HAL_CFG_BIGBYTE_ENDIAN
#endif  // _HAL_INT_MMAP_H_
