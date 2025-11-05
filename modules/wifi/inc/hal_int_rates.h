/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * Airgo Networks, Inc proprietary. All rights reserved.
 * halRateTable.h:  Provides APIs for rate table setup and access
 * Author:    Susan Tsao
 * Date:      02/06/2006
 *
 * --------------------------------------------------------------------------
 */
#ifndef _HAL_INT_RATES_H_
#define _HAL_INT_RATES_H_

#include <stdint.h>
#include "nt_common.h"
#include "hal_int_mmap.h"

#if 0
	eHalStatus halRate_initRateTable(tHalHandle hHal, void* arg);
	eHalStatus halRate_updateSubbandMode(tpAniSirGlobal pMac, uint8_t subbandMode);
	eHalStatus halRate_getMPICmd(tpAniSirGlobal pMac, uint8_t* mpiCmd, uint32_t rateIndex);
	eHalStatus halRate_getCE(tpAniSirGlobal pMac, uint8_t* ceDescr, uint32_t rateIndex);
	eHalStatus halRate_getCeDescriptor(tpAniSirGlobal pMac, uint8_t* mpi, uint8_t* ceDescr);
	eHalStatus halRate_sendResponseRateTable(tHalHandle hHal, void* arg);
	eHalStatus halRate_sendRateTable(tHalHandle hHal, void* arg);
	tANI_S32   halRate_halRate2SmacRate(tHalMacRate halRate);
	tANI_S32   halRate_smacRate2HalRate(uint32_t taurusRate);

#ifdef ANI_DVT_DEBUG
	eHalStatus halRate_dvtSetStaInitRateInfo(tpAniSirGlobal pMac, uint32_t staIdx, tpAddStaParams  param,
		tSmacCfgStaRateInfo(*pStaRateInfoBase)[SMAC_STACFG_MAX_RATES][SMAC_STACFG_TXRATE_CHANNEL_NUM],
		int(*selectedDataRates)[SMAC_STACFG_MAX_RATES][SMAC_STACFG_TXRATE_CHANNEL_NUM]);
#endif


	// Diagnostic functions
	void halRateDbg_printCeDescriptor(tpAniSirGlobal pMac, tSmacCfgComputeEngineDesc* ce);
	void halRateDbg_printSmacRateTable(tpAniSirGlobal pMac, uint32_t start, uint32_t end);
	void halRateDbg_printTxCmdTemplate(tpAniSirGlobal pMac, tSmacCfgTxCmdTmpl* txCmd);
	void halRateDbg_printTxCmdEntry(tpAniSirGlobal pMac, uint8_t rateIndex);
	void halRateDbg_printRspRateTable(tpAniSirGlobal pMac, uint32_t start, uint32_t end);
	void halRateDbg_changeRspRateTable(tpAniSirGlobal pMac, uint32_t idxBegin, uint32_t idxEnd, uint32_t value);
	void  halRateDbg_dumpRateTable(tpAniSirGlobal pMac, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4);
	eHalStatus halRate_BckupTpeRateTable(tpAniSirGlobal pMac, uint32_t* memAddr);
#endif

#define PHY_NSS_1SPATIAL 0

#define PKT_TYPE_11a           0
#define PKT_TYPE_11b           1
#define PKT_TYPE_11N_MIXEDMODE 3

#define PR_SHORT 0
#define PR_LONG  1

#define FEC_MODE_ZERO 0
#define FEC_MODE_ONE  1

#define SG_DISABLED 0
#define SG_ENABLED  1

#define DUP_ZERO 0
#define DUP_ONE  1

#define AMPDU_INV   0
#define AMPDU_VALID 1

#define STBC_INV   0
#define STBC_VALID 1

#define NDLTF_ZERO 0
#define NDLTF_ONE  1
#define NDLTF_TWO  2
#define NDLTF_FOUR 3

#define NELTF_ZERO 0
#define NELTF_ONE  1
#define NELTF_TWO  2
#define NELTF_FOUR 3

/* 11n */
#define WLAN_DUR_11N_LEGACY_STF    8 /* L-STF */
#define WLAN_DUR_11N_LEGACY_LTF    8 /* L-LTF */
#define WLAN_DUR_11N_LEGACY_SIGNAL 4 /* L-SIG */

#define WLAN_DUR_11N_HT_STF             4 /* HT-STF */
#define WLAN_DUR_11N_HT_LTF             4 /* HT-LTF */
#define WLAN_DUR_11N_HT_SIGNAL          8 /* HT-SIG */
#define WLAN_DUR_11N_HT_LTF1_GREENFIELD 8 /* HT-LTF1 */

#define WLAN_PHYHEADER_11N_MIXEDMODE(spatial_stream)                                                           \
    (WLAN_DUR_11N_LEGACY_STF + WLAN_DUR_11N_LEGACY_LTF + WLAN_DUR_11N_LEGACY_SIGNAL + WLAN_DUR_11N_HT_SIGNAL + \
     WLAN_DUR_11N_HT_STF + WLAN_DUR_11N_HT_LTF * (spatial_stream))

#define HAL_PHYHEADER_11b_LONG_DURATION  192
#define HAL_PHYHEADER_11b_SHORT_DURATION 96

/* ------------------------------------
 * Response Rate Index Word Definition
 * ------------------------------------
 */
#define BAINDEX(x)   (x & 0xFF) << 24  // bit[31:24]
#define ACKINDEX(x)  (x & 0xFF) << 16  // bit[23:16]
#define CTSINDEX(x)  (x & 0xFF) << 8   // bit[15:8]
#define RSVDINDEX(x) (x & 0xFF)        // bit[7:0]

#define RESPONSE_RATE_INDEX(ba, ack, cts, rsv) (BAINDEX(ba) | ACKINDEX(ack) | CTSINDEX(cts) | RSVDINDEX(rsv))

#define HAL_RATETABLE_RATEINDEX_BEGIN 0
#define HAL_RATETABLE_RATEINDEX_END   (HAL_MMAP_RATES_MAX - 1)

// index must always be an unsigned number
#define HAL_RATETABLE_IS_RATEINDEX_VALID(index) ((index) <= HAL_RATETABLE_RATEINDEX_END)

// WORD 0
#define PKTTYPE(x)     (x & 0x7)            // bit[0:2]
#define PSDURATE(x)    ((x & 0x3F) << 3)    // bit[3:8]
#define BANDWIDTH(x)   ((x & 0x3) << 9)     // bit[9:10]
#define NS11b(x)       ((x & 0x1) << 11)    // bit[11:11]
#define FECMODEFLAG(x) ((x & 0x1) << 12)    // bit[12:12]
#define SHORTGUARD(x)  ((x & 0x1) << 13)    // bit[13:13]            // bit[13]
#define CTRLRATEIDX(x) ((x & 0x1FF) << 14)  // bit[14:22]
#define DUPFLAG(x)     ((x & 0x1) << 25)    // bit[25:25]
#define LOWPOWEN(x)    ((x & 0x1) << 26)    // bit[26:26]
#define TXPOWER(x)     ((x & 0x1F) << 27)   // bit[27:31]

// WORD 1
#define RSPRATEIDX(x)   (x & 0x1FF)          // bit[0:8]
#define AMPDU(x)        ((x & 0x1) << 9)     // bit[9:9]
#define NDBPS4TRATE(x)  ((x & 0xFFF) << 10)  // bit[10:21]
#define NDLTFS(x)       ((x & 0x3) << 22)    // bit[22:23]
#define NELTFS(x)       ((x & 0x3) << 24)    // bit[24:25]
#define PCTRLRATEIDX(x) ((x & 0xF) << 26)    // bit[26:29]
#define STBC(x)         ((x & 0x3) << 30)    // bit[30:31]

//#define  TXANTENNA(x)    	((x & 0x7) << 23)    // bit[24:26]

#define RATE2NDBPS(x)         ((x >> 10) & 0xFFF)  // bit[10:21]
#define IS_RATE_SHORTGUARD(x) ((x >> 13) & 1)      // bit[13:13]

typedef enum hal_rate_idx_e {
    HAL_RT_IDX_INVALID = 0,

    // 11b Rates
    HAL_RT_IDX_11B_RATE_LONG_PR_BASE_OFFSET = 0,
    HAL_RT_IDX_11B_LONG_1_MBPS = HAL_RT_IDX_11B_RATE_LONG_PR_BASE_OFFSET,
    HAL_RT_IDX_11B_LONG_2_MBPS,
    HAL_RT_IDX_11B_LONG_5_5_MBPS,
    HAL_RT_IDX_11B_LONG_11_MBPS,
    HAL_RT_IDX_11B_RATE_SHORT_PR_BASE_OFFSET = 4,
    HAL_RT_IDX_11B_LONG_1_MBPS_DUP = HAL_RT_IDX_11B_RATE_SHORT_PR_BASE_OFFSET,
    HAL_RT_IDX_11B_SHORT_2_MBPS,
    HAL_RT_IDX_11B_SHORT_5_5_MBPS,
    HAL_RT_IDX_11B_SHORT_11_MBPS,

    // 11ag rates
    HAL_RT_IDX_11A_6_MBPS = 8,
    HAL_RT_IDX_11A_9_MBPS,
    HAL_RT_IDX_11A_12_MBPS,
    HAL_RT_IDX_11A_18_MBPS,
    HAL_RT_IDX_11A_24_MBPS,
    HAL_RT_IDX_11A_36_MBPS,
    HAL_RT_IDX_11A_48_MBPS,
    HAL_RT_IDX_11A_54_MBPS,

    // MCS Index #0-15 (20MHz) Mixed Mode
    HAL_RT_IDX_MCS_1NSS_MM_6_5_MBPS = 16,
    HAL_RT_IDX_MCS_1NSS_MM_13_MBPS,
    HAL_RT_IDX_MCS_1NSS_MM_19_5_MBPS,
    HAL_RT_IDX_MCS_1NSS_MM_26_MBPS,
    HAL_RT_IDX_MCS_1NSS_MM_39_MBPS,
    HAL_RT_IDX_MCS_1NSS_MM_52_MBPS,
    HAL_RT_IDX_MCS_1NSS_MM_58_5_MBPS,
    HAL_RT_IDX_MCS_1NSS_MM_65_MBPS,
    HAL_RT_IDX_MCS_1NSS_MM_SG_7_2_MBPS,
    HAL_RT_IDX_MCS_1NSS_MM_SG_14_4_MBPS,
    HAL_RT_IDX_MCS_1NSS_MM_SG_21_7_MBPS,
    HAL_RT_IDX_MCS_1NSS_MM_SG_28_9_MBPS,
    HAL_RT_IDX_MCS_1NSS_MM_SG_43_3_MBPS,
    HAL_RT_IDX_MCS_1NSS_MM_SG_57_8_MBPS,
    HAL_RT_IDX_MCS_1NSS_MM_SG_65_MBPS,
    HAL_RT_IDX_MCS_1NSS_MM_SG_72_2_MBPS,

    HAL_RT_IDX_MAX_RATES = 32

} hal_rate_idx_t;

// max rate idx for rates
#define HAL_MAX_11A_RT_IDX 12
#define HAL_MAX_RATES      27

#define HAL_ERP_MAX_RT_IDX     0
#define HAL_HT_MAX_RT_IDX      8
#define HAL_ALLRATE_MAX_RT_IDX 0

// protection type
#define HAL_ERP_PROTECTION      1
#define HAL_HT_PROTECTION       2
#define HAL_ALL_RATE_PROTECTION 3
#define HAL_NO_PROTECTION_MODE  0

// protection mode
typedef enum {
    HAL_NO_PROTECTION = 0,
    HAL_CTS_2_SELF_ALWAYS,
    HAL_RTS_THRESHOLD,
    HAL_DUAL_CTS,  // not supported
    RSVD,          // not used.
    HAL_CTS_THRESHOLD,
    // RTS always is not one of the defined TPE desc
    // protection mode, this is to force using RTS/CTS
    // protection
    HAL_RTS_ALWAYS,
} tTpeProtPolicy;

// TPE STA descriptor protection type
#define HAL_PROT_TYPE_NONE        0
#define HAL_PROT_TYPE_ENTIRE_TXOP 1
#define HAL_PROT_TYPE_FIRST_TX    2

#define HAL_RT_IDX_11B_LONG_1_MBPS_OFFSET   (HAL_RT_IDX_11B_LONG_1_MBPS - HAL_RT_IDX_11B_RATE_LONG_PR_BASE_OFFSET)
#define HAL_RT_IDX_11B_LONG_2_MBPS_OFFSET   (HAL_RT_IDX_11B_LONG_2_MBPS - HAL_RT_IDX_11B_RATE_LONG_PR_BASE_OFFSET)
#define HAL_RT_IDX_11B_LONG_5_5_MBPS_OFFSET (HAL_RT_IDX_11B_LONG_5_5_MBPS - HAL_RT_IDX_11B_RATE_LONG_PR_BASE_OFFSET)
#define HAL_RT_IDX_11B_LONG_11_MBPS_OFFSET  (HAL_RT_IDX_11B_LONG_11_MBPS - HAL_RT_IDX_11B_RATE_LONG_PR_BASE_OFFSET)

#define HAL_RT_IDX_11B_SHORT_2_MBPS_OFFSET   (HAL_RT_IDX_11B_SHORT_2_MBPS - HAL_RT_IDX_11B_RATE_SHORT_PR_BASE_OFFSET)
#define HAL_RT_IDX_11B_SHORT_5_5_MBPS_OFFSET (HAL_RT_IDX_11B_SHORT_5_5_MBPS - HAL_RT_IDX_11B_RATE_SHORT_PR_BASE_OFFSET)
#define HAL_RT_IDX_11B_SHORT_11_MBPS_OFFSET  (HAL_RT_IDX_11B_SHORT_11_MBPS - HAL_RT_IDX_11B_RATE_SHORT_PR_BASE_OFFSET)

#define HAL_RATE_INDEX_6MBPS      8
#define HAL_DEFAULT_5G_RATE_INDEX HAL_RATE_INDEX_6MBPS  // 6 mbps rate for 5ghz
#define HAL_DEFAULT_2G_RATE_INDEX 0                     // 1 mbps rate for 2ghz

#define MAX_SIZE          4
#define MAX_TIME_LOG_DISP 3

typedef struct hal_tpe_sram_rate_tbl_s {
    uint32_t word0;
    uint32_t word1;
} hal_tpe_sram_rate_tbl_t;

#if 0

	typedef struct hal_tpe_rate_tbl_s {

#ifdef ANI_BIG_BYTE_ENDIAN
			/* word 0 */
			uint32_t    txPwr : 5;
			uint32_t    ctrlRespLowPowEn : 1;
			uint32_t    dupFlag : 1;
			uint32_t    rsvd2 : 2;
			uint32_t    rsvd1 : 5;
			uint32_t    cntrlRateIdx : 4;
			uint32_t    shortGuard : 1;
			uint32_t    fecModeFlag : 1;
			uint32_t    nssOr11bMode : 1;
			uint32_t    bwMode : 2;
			uint32_t    psduRate : 6;
			uint32_t    pktType : 3;

			/* word 1 */
			uint32_t    stbcValid : 2;
			uint32_t    protCntrlRateIdx : 4;
			uint32_t    neltfs : 2;
			uint32_t    ndltfs : 2;
			uint32_t    ndbpsOr4timesRate : 12;
			uint32_t    ampduValid : 1;
			uint32_t    rsvd3 : 5;
			uint32_t    rspRateIdx : 4;
#else
			/* word 0 */
			uint32_t    pktType : 3;
			uint32_t    psduRate : 6;
			uint32_t    bwMode : 2;
			uint32_t    nssOr11bMode : 1;
			uint32_t    fecModeFlag : 1;
			uint32_t    shortGuard : 1;
			uint32_t    cntrlRateIdx : 4;
			uint32_t    rsvd1 : 5;
			uint32_t    rsvd2 : 2;
			uint32_t    dupFlag : 1;
			uint32_t    ctrlRespLowPowEn : 1;
			uint32_t    txPwr : 5;

			/* word 1 */
			uint32_t    rspRateIdx : 4;
			uint32_t    rsvd3 : 5;
			uint32_t    ampduValid : 1;
			uint32_t    ndbpsOr4timesRate : 12;
			uint32_t    ndltfs : 2;
			uint32_t    neltfs : 2;
			uint32_t    protCntrlRateIdx : 4;
			uint32_t    stbcValid : 2;
#endif

	} hal_tpe_rate_tbl_t;

	typedef struct hal_rate_tbl_s {

		hal_tpe_rate_tbl_t	tpeRateTable;

		uint8_t         validEntry;
		uint8_t    		stbcValid;
		uint8_t    		txAntEnable;
		uint8_t    		txPwr;
	} hal_rate_tbl_t;

	eHalStatus halRate_getRateTableInfo(tpAniSirGlobal pMac, tpHalRateTable pRateInfo, uint32_t rateIndex);
	void halRateDbg_printTpeRateTable(tpAniSirGlobal pMac, tpTpeRateTable pTpeRateTable);
	eHalStatus halRate_GetRateParams(tpAniSirGlobal pMac, uint32_t rateIndx, uint32_t* pNdbps, uint32_t* pIsShortGuard);
#endif

// Rate table masks
#define TPE_CONTROL_INDEX_MASK   0x3c000
#define TPE_CONTROL_INDEX_OFFSET 14

#define TPE_CONTROL_INDEX_UPPER_BITS_MASK   0x7c0000
#define TPE_CONTROL_INDEX_UPPER_BITS_OFFSET 18

#define TPE_CONTROL_RESP_LPE_MASK   0x4000000
#define TPE_CONTROL_RESP_LPE_OFFSET 26

#define TPE_CONTROL_RSP_TX_PWR_MASK   0xf8000000
#define TPE_CONTROL_RSP_TX_PWR_OFFSET 27

#define TPE_RSP_RATE_INDEX_MASK   0xf
#define TPE_RSP_RATE_INDEX_OFFSET 0

#define TPE_RSP_RATE_INDEX_UPPER_BITS_MASK   0x1f0
#define TPE_RSP_RATE_INDEX_UPPER_BITS_OFFSET 4

#define TPE_11G_RATE_MINUS_11b_RATE_MASK   0x3c000000
#define TPE_11G_RATE_MINUS_11b_RATE_OFFSET 26

#define TPE_RATE_TABLE_SRAM_XACT_DATA_LPE_MASK   0x1
#define TPE_RATE_TABLE_SRAM_XACT_DATA_LPE_OFFSET 1

#define TPE_RATE_TABLE_SRAM_XACT_DATA_RT_IDX_MASK   0xF
#define TPE_RATE_TABLE_SRAM_XACT_DATA_RT_IDX_OFFSET 4

#define TPE_RATE_TABLE_SRAM_XACT_DATA_TX_PWR_MASK   0x1F
#define TPE_RATE_TABLE_SRAM_XACT_DATA_TX_PWR_OFFSET 5

/*
 * initialize rate related tables and entries
 */
typedef struct nt_hal_sta_tx_rate_s {
    uint8_t p_rate;
    uint8_t s_rate;
    uint8_t t_rate;
} nt_hal_sta_tx_rate_t;

typedef struct nt_hal_sta_rate_stat_s {
    uint16_t p_rate_ppdu_tx_cnt;
    uint16_t s_rate_ppdu_tx_cnt;
    uint16_t t_rate_ppdu_tx_cnt;

    uint16_t p_rate_ppdu_ack_to;
    uint16_t s_rate_ppdu_ack_to;
    uint16_t t_rate_ppdu_ack_to;
} nt_hal_sta_rate_stat_t;

typedef struct nt_hal_sta_tx_power_s {
    uint8_t p_power;
    uint8_t s_power;
    uint8_t t_power;
} nt_hal_sta_tx_power_t;

nt_status_t hal_rates_init(void);

void nt_hal_sta_tx_rate_update(nt_hal_bss_t *bss, nt_hal_sta_t *sta);
void nt_hal_sta_rate_stats_get(nt_hal_bss_t *bss, nt_hal_sta_t *sta, nt_hal_sta_rate_stat_t *rate_stat);
void nt_hal_rtbl_tx_pwr_update(uint8_t tx_power);
void nt_hal_rtbl_rsp_rate_update(uint8_t rate_idx);
void nt_hal_r2p_tbl_update(void);
void nt_hal_fix_rts_rate(uint32_t write, uint32_t fix_rate);
void hal_reduce_tx_pwr_if_wired(uint8_t reduce_by);
void nt_hal_rate_tbl_restore(void);
int8_t nt_hal_tx_pwr_update(uint8_t tx_power, uint8_t policy);
void nt_hal_rtbl_print(void);
void nt_hal_cal_txpwr_get(void);

/* @brief: This API is used to set the protection if any legacy stations join
 * @Param: Parameter1: bss:-  bss pointer to the bss structure
 * @Param: Parameter2: sta:-  sta pointer to the sta structure
 * @Param: Parameter3: prot_mode:-  mode of protection,none,cts_to_self_always = 1, rts_always = 6
 * @Param: Parameter4: prot_type:-  prot_type to enable the protection mode for particular protection type
                                    None: no protection at all - prot_mode ignored
                                    HT: legacy preamble included (HT rate protection) - protection based on prot_mode
                                    ERP: protection based on prot_mode
                                    Always: always protect (for hidden sta) - protection used based on prot_mode
 * @return: Invalid prot_mode or invalid prot_type return NT_EPARAM otherwise return  NT_OK
 * */
#if (FERMION_CHIP_VERSION == 2)
nt_status_t nt_hal_protection(nt_hal_bss_t *bss, nt_hal_sta_t *sta, uint8_t prot_type, uint8_t prot_mode,
                              uint32_t phy_rate_f_h_q);
#else
nt_status_t nt_hal_protection(nt_hal_bss_t *bss, nt_hal_sta_t *sta, uint8_t prot_type, uint8_t prot_mode);
#endif
nt_status_t hal_set_protection_threshold(uint32_t threshold);
uint16_t nt_hal_read_rate_index_history_last(void);

#endif /* _HAL_INT_RATES_H_ */
