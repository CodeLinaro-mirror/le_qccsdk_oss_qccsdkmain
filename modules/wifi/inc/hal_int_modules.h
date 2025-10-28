/*
Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#ifndef HAL_INT_MODULES_H
#define HAL_INT_MODULES_H

#include "hal_api_sys.h"
#include "hal_int_rates.h"
#include "hal_int_powersave.h"
#include "nt_common.h"
#include "wifi_cmn.h"

#include "fwconfig_wlan.h"
#include "nt_flags.h"
/*halphy_set_tx_pwr_for_rate/halphy_get_tx_pwr_for_rate defined for UFW only in phy_dev_TPCcal.h*/
#ifdef HALMAC_UFW
#include "phy_dev_TPCcal.h"
#endif

// Basic MTU definitions
// how long to send bc/mc traffic after dtim beacon, applies to AP mode
#define HAL_DTIM_BCN_BCMC_THR_LIMIT (20 * 1024)

// per sta queue assignments
#define HAL_BMU_TX_QID_QTID0 0
#define HAL_BMU_TX_QID_QTID1 1
#define HAL_BMU_TX_QID_QTID2 2
#define HAL_BMU_TX_QID_QTID3 3
#define HAL_BMU_TX_QID_QTID4 4
#define HAL_BMU_TX_QID_QTID5 5
#define HAL_BMU_TX_QID_QTID6 6
#define HAL_BMU_TX_QID_QTID7 7
#define HAL_BMU_TX_QID_NQOS  8
#define HAL_BMU_TX_QID_MGMT  9
#define HAL_BMU_TX_QID_NOACK 10

// BTQM TID's to enable (see control2 reg)
#define HAL_BMU_TX_ID_ENABLE 0x7FF
// ACK policy (2bits) for each QID, imm ack for all except the NOACK qid
#define HAL_TX_QID_ACK_POLICY   0  //(1 << (2 * HAL_BMU_TX_QID_NOACK))
#define HAL_TX_QID_NOACK_POLICY 5

// Number of STA idx supported on platform
#define HAL_BMU_NUM_STAIDX_SUPPORTED 4

// BMU commands base address
#define BMU_COMMAND_BASE_ADDRESS 0x02800000

// txop's to use for the various backoff counters
#define EDCF_TXOP_FOR_BO_0_1 0x0
#define EDCF_TXOP_FOR_BO_2_3 0x0
#define EDCF_TXOP_FOR_BO_4_5 0x05E00BC0
#define EDCF_TXOP_FOR_BO_6_7 0x0

#define SW_TX_SIFS_A_MODE_CYCLES        0x96
#define SW_TX_SIFS_B_MODE_CYCLES        0x4B
#define SW_DELAY_WAIT_CYCLES_THLD_8CLKS 5
#define SW_DELAY_WAIT_CMD_8CLKS         4

#ifdef PLATFORM_NT
#define SW_WARMUP_CLKS \
    130  // Corresponding to 1.2us for 20MHz bandwidth - warmup_time = HostPmA.get_param_val("mac_clk_freq") +
         // (HostPmA.get_param_val("mac_clk_freq")*3)/10
#endif

#ifdef PLATFORM_FERMION
#define SW_WARMUP_CLKS \
    72  // Corresponding to 1.2us for 20MHz bandwidth - warmup_time = HostPmA.get_param_val("mac_clk_freq") +
        // (HostPmA.get_param_val("mac_clk_freq")*3)/10
#endif

#define TPE_MAX_MPDU_IN_AMPDU 0x3f
#define TPE_MAX_AMPDU_TX_TIME 0xda8

#define HAL_RXP_CMD_MAX_RETRY   100
#define RXP_DMA_MAX_RSV_PDU     15
#define RXP_STALL_TIMEOUT_US    12  // RXP STALL Timeout duration in usec
#define RXP_STALL_TIMEOUT_VALUE 0x2000
#define RXP_PUSH_WQ_CTRL_WQ3    0x03030303
#define RXP_PUSH_WQ_CTRL2_WQ3   RXP_PUSH_WQ_CTRL_WQ3

#define HALMAC_POLL_TIMEOUE 100

// retry thresholds
#define HAL_TX_RETRY_THRESH_0 2  // drop to secondary
#define HAL_TX_RETRY_THRESH_1 4  // drop to tertiary
#define HAL_TX_RETRY_THRESH_2 6  // drop the packet
#define BA_RECIPIENT          0
#define BA_ORIGINATOR         1

#define AMPDU_DENSITY_DELIMETER 4
#define DENSITY_CALC            (AMPDU_DENSITY_DELIMETER * 8 * 4)

#define HAL_BACK_OFF_ENGINE_ONE_DISABLE_OFFSET 0X02000000

#define pwrtwo(x)   (1 << (x))  // two power of x
#define SIFS_2_4ghz 10          /* short interframe space 10us for 2.4ghz 802.11n */
#define SIFS_5ghz   16          /* short interframe space 16us for 5ghz 802.11n */

// set power policy
#define SAFE    0
#define FREERUN 1

// restore power to default value
#define RESTORE_DEFAULT 100

extern uint8_t hal_staid;
extern uint8_t delstaidx;

// table index for RMF updation
#define TBLIDX_AP_RMF_UPDATE  -2
#define TBLIDX_STA_RMF_UPDATE 2

#define MTU_TXP_DELAY_LIMIT                                          \
    ((QWLAN_MTU_SW_MTU_MISC_LIMITS_SW_MTU_TXP_DELAY_LIMIT_DEFAULT >> \
      QWLAN_MTU_SW_MTU_MISC_LIMITS_SW_MTU_TXP_DELAY_LIMIT_OFFSET) /  \
     2)

#if (defined NT_FN_WMM_PS_AP) || (defined NT_FN_WMM_PS_STA)

/*APSD AC Mask value*/
#define HAL_APSD_AC_VO 0x01
#define HAL_APSD_AC_VI 0x02
#define HAL_APSD_AC_BK 0x04
#define HAL_APSD_AC_BE 0x08

/*BMU Queue ID enum*/
enum {
    HAL_BMU_QID_0 = 0,
    HAL_BMU_QID_1,
    HAL_BMU_QID_2,
    HAL_BMU_QID_3,
    HAL_BMU_QID_4,
    HAL_BMU_QID_5,
    HAL_BMU_QID_6,
    HAL_BMU_QID_7,
    HAL_BMU_QID_8,
    HAL_BMU_QID_9,
    HAL_BMU_QID_10,
    HAL_BMU_QID_11,
    HAL_BMU_QID_12
};
#endif  // (defined NT_FN_WMM_PS_AP) || (defined NT_FN_WMM_PS_STA)

enum {
    MC_STATS_QID_0_MASK = 0,
    BC_STATS_QID_0_MASK,
    MC_STATS_QID_1_MASK,
    BC_STATS_QID_1_MASK,
    MC_STATS_QID_2_MASK,
    BC_STATS_QID_2_MASK,
    MC_STATS_QID_3_MASK,
    BC_STATS_QID_3_MASK,
    MC_STATS_QID_4_MASK,
    BC_STATS_QID_4_MASK,
    MC_STATS_QID_5_MASK,
    BC_STATS_QID_5_MASK,
    MC_STATS_QID_6_MASK,
    BC_STATS_QID_6_MASK,
    MC_STATS_QID_7_MASK,
    BC_STATS_QID_7_MASK,
    MC_STATS_QID_8_MASK,
    BC_STATS_QID_8_MASK,
    MC_STATS_QID_9_MASK,
    BC_STATS_QID_9_MASK,
    MC_STATS_QID_10_MASK,
    BC_STATS_QID_10_MASK,
    MC_STATS_QID_11_MASK,
    BC_STATS_QID_11_MASK,
    MC_STATS_QID_12_MASK,
    BC_STATS_QID_12_MASK,
    MC_STATS_QID_13_MASK,
    BC_STATS_QID_13_MASK,
    MC_STATS_QID_14_MASK,
    BC_STATS_QID_14_MASK,
    MC_STATS_QID_15_MASK,
    BC_STATS_QID_15_MASK
};

/* ------------------
 *  BMU command Type Definition
 * ------------------
 */
typedef enum hal_bmu_cmd_e {
    PUSH_WQ_CMDTYPE = 0x0,
    POP_WQ_CMDTYPE = 0x4,
    WRITE_WQ_HEAD_CMDTYPE = 0x8,
    READ_WQ_HEAD_CMDTYPE = 0xc,
    WRITE_WQ_TAIL_CMDTYPE = 0x10,
    READ_WQ_TAIL_CMDTYPE = 0x14,
    WRITE_WQ_NR_CMDTYPE = 0x18,
    READ_WQ_NR_CMDTYPE = 0x1c,
    WRITE_BD_POINTER_CMDTYPE = 0x20,
    READ_BD_POINTER_CMDTYPE = 0x24,
    RESERVATION_REQUEST_BD_PDU_CMDTYPE = 0x28,
    GET_BD_PDU_CMDTYPE = 0x2c,
    RELEASE_PDU_CMDTYPE = 0X30,
    RELEASE_BD_CMDTYPE = 0X34,
    EXPENDED_RESV_REQUEST_BD_PDU_CMDTYPE = 0x3c,
    RESERVE_ALL_BD_PDU_CMDTYPE = 0x44,

    /** Gen5 Commands */
    ENHANCED_PUSH_WQ_CMDTYPE = 0x50,
    ENHANCED_POP_WQ_CMDTYPE = 0x54,
    GET_TX_QUEUE_INFO = 0x58,
    GET_NEXT_PACKET_LEN = 0x5C,
    GET_NEXT_PACKET_BD_INDEX_LEN = 0x60,
    TRANSMISSION_FEEDBACK = 0x64,
    SET_CLEAR_NEXT_FRAME_TX_PTR = 0x68,
    READ_NEXT_FRAME_TX_PTR = 0x6C,
    READ_BTQM_INDEX_ENTRY1 = 0x70,
    READ_BTQM_INDEX_ENTRY2 = 0x74,
    WRITE_TX_WQ_HEAD = 0x80,
    READ_TX_WQ_HEAD = 0x84,
    WRITE_TX_WQ_TAIL = 0x88,
    READ_TX_WQ_TAIL = 0x8C,
    WRITE_TX_WQ_NR = 0x90,
    READ_TX_WQ_NR = 0x94,
    PUSH_TX_WQ_CMDTYPE = 0x78,
    POP_TX_WQ_CMDTYPE = 0x7C,

} hal_bmu_cmd_t;

#define BMU_CMD_RESERVE_ALL_BD_PDU(_moduleIdx) \
    (BMU_COMMAND_BASE_ADDRESS | ((_moduleIdx & 0x3f) << 8) | RESERVE_ALL_BD_PDU_CMDTYPE)
#define BMU_CMD_POP_WQ(wq)  (BMU_COMMAND_BASE_ADDRESS | ((wq << 8) | POP_WQ_CMDTYPE))
#define BMU_CMD_PUSH_WQ(wq) (BMU_COMMAND_BASE_ADDRESS | ((wq << 8) | PUSH_WQ_CMDTYPE))

/*
 * BMU WQ assignment
 *
 * BMU defines following WQs
 *   WQ0 : Idle BD
 *   WQ1 : Idle PDU
 *   WQ2 : mCPU RX WQ
 *   WQ3 - WQ10 : DPU, DXE or mCPU
 *   WQ11 - WQ17 : DXE or mCPU
 *   WQ18 - WQ24 : mCPU
 *
 * SoftMAC uses WQs as follows
 *   WQ0 : Idle BD
 *   WQ1 : Idle PDU
 *   WQ2 : mCPU RX WQ
 *   WQ3 : DPU RX
 *   WQ4 - WQ10 : DXE RX
 *   WQ11 - WQ17 : DPU TX
 *   WQ18 - WQ24 : mCPU TX
 *
 * Original idea of BMU WQ assignment was based on "Nova MAC architecture spec".
 * In case host interface is PCI or PCI express,
 *
 *   TX : 7 DXE channels => 7 DPU TX WQs => 7 mCPU TX WQs
 *   RX : 1 mCPU RX WQ => 1 DPU RX WQ => 7 DXE RX WQs => 7 DXE channels
 *
 * In case host interface is USB,
 *
 *   TX : 2 USB endpoints => 2 DXE channels => 2 DPU TX WQs => 2 mCPU TX WQs
 *   RX : 1 mCPU RX WQ => 1 DPU RX WQ => 4 DXE RX WQs => 4 DXE channels => 4 USB endpoints
 *
 * The actual implementation of Taurus has 10 DXE channels, probably half for Tx
 * and the other half for Rx. Even one DXE channel can transfer to several WQs in
 * H2B transfer (B2H transfer also), it would be reasonable to limit number of WQs
 * same as number of DXE channels, and to free WQs up.
 */

typedef enum hal_wq_e {
    /* BMU */
    HAL_BMUWQ_BMU_IDLE_BD = 0,
    HAL_BMUWQ_BMU_IDLE_PDU = 1,

    /* MCPU RX */
    HAL_BMUWQ_MCPU_RX = 2,

    HAL_BMUWQ_MCPU_RX_DPULOOPBACK = 24,

    HAL_BMUWQ_MCPU_RX_HOST = 16,

    /* DPU RX */
    HAL_BMUWQ_DPU_RX = 3,

    /* DPU TX */
    HAL_BMUWQ_DPU_TX_START = 4,
    HAL_BMUWQ_DPU_TX_END = 10,

    HAL_BMUWQ_DPU_TX_WQ0 = HAL_BMUWQ_DPU_TX_START,
    HAL_BMUWQ_DPU_TX_WQ1,
    HAL_BMUWQ_DPU_TX_WQ2,
    HAL_BMUWQ_DPU_TX_WQ3,
    HAL_BMUWQ_DPU_TX_WQ4,
    HAL_BMUWQ_DPU_TX_WQ5,
    HAL_BMUWQ_DPU_TX_WQ6,

    HAL_BMUWQ_DPU_TX_MGMT_HOST = HAL_BMUWQ_DPU_TX_WQ0,
    HAL_BMUWQ_DPU_TX_AC_VO,
    HAL_BMUWQ_DPU_TX_AC_VI,
    HAL_BMUWQ_DPU_TX_AC_BE,
    HAL_BMUWQ_DPU_TX_AC_BK = HAL_BMUWQ_DPU_TX_AC_BE,

    /* DXE RX */
    HAL_BMUWQ_DXE_RX_START = 11,
    HAL_BMUWQ_DXE_RX_END = 14,

    HAL_BMUWQ_DXE_RX_WQ0 = HAL_BMUWQ_DXE_RX_START,
    HAL_BMUWQ_DXE_RX_WQ1,
    HAL_BMUWQ_DXE_RX_WQ2,
    HAL_BMUWQ_DXE_RX_WQ3,
    HAL_BMUWQ_DXE_RX_WQ4,

    /* DPU Error */
    HAL_BMUWQ_DPU_ERROR_WQ = 17,

    /* MCPU TX */
    HAL_BMUWQ_MCPU_TX_START = 18,
    HAL_BMUWQ_MCPU_TX_END = 23,

    HAL_BMUWQ_MCPU_TX_WQ0 = HAL_BMUWQ_MCPU_TX_START,
    HAL_BMUWQ_MCPU_TX_WQ1,
    HAL_BMUWQ_MCPU_TX_WQ2,
    HAL_BMUWQ_MCPU_TX_WQ3,
    HAL_BMUWQ_MCPU_TX_WQ4,
    HAL_BMUWQ_MCPU_TX_WQ5,
    /*HAL_BMUWQ_MCPU_TX_WQ6,  WQ24 is now used by softmac rx for AMSDU deaggregation*/

    HAL_BMUWQ_MCPU_TX_MGMT_HOST = HAL_BMUWQ_MCPU_TX_WQ0,
    HAL_BMUWQ_MCPU_TX_HCCA1,
    // HAL_BMUWQ_MCPU_TX_HCCA2,
    HAL_BMUWQ_MCPU_TX_AC_VO,
    HAL_BMUWQ_MCPU_TX_AC_VI,
    HAL_BMUWQ_MCPU_TX_AC_BE,
    HAL_BMUWQ_MCPU_TX_AC_BK,

    /* Special WQ for sinking */
    HAL_BMUWQ_SINK = 255,

    HAL_BMU_BTQM_WQ25 = 25,
    HAL_BMU_BTQM_WQ26 = 26,

    HAL_ADU_RX_WQ = 24, /**< DPU pushes frames to this Wq */

    HAL_BMUWQ_TXBD_COMPLETE_WQ = 15,  // WQ0-26,use WQ15 for TXBD

#ifdef DDR_ONLY_BMU_POOL

    HAL_BMUWQ_NUM = 27

#else

    /* BMU uses this WQ to keep track of the IDLE BDs that use
       local CMEM memory. */
    HAL_BMUWQ_BMU_IDLE_LOCAL_BD = 27,

    HAL_BMUWQ_NUM = 29

#endif
} hal_buff_wq_t;

// BMU enable/disable sta transmission commands
typedef enum hal_bmu_sta_tx_cfg_cmd_s {
    eBMU_ENB_TX_QUE_DIS_TRANS = 0x0,
    eBMU_ENB_TX_QUE_ENB_TRANS = 0x1,
    eBMU_DIS_TX_QUE_DIS_TRANS = 0x2,
    eBMU_DIS_TX_QUE_DIS_TRANS_CLEANUP_QUE = 0x3,
    eBMU_CLEAR_CONTROL_FIELDS = 0x4,
    eBMU_ENB_TX_QUE_DIS_TRANS_CLEAR_CONTROL_FIELDS = 0x5,
    // eBMU_ENB_TX_QUE_DIS_TRANS = 0x6
    eBMU_TX_QUE_TRANS_MAX
} hal_bmu_sta_tx_cfg_cmd_t;

typedef struct hal_rxp_addr_tbl_s {
    uint8_t macAddr[HAL_MACADDR_SZ];
    uint8_t staid;
    uint8_t rmf;
    uint8_t dpuDescIdx;
    uint8_t mcbcDpuIdx;
    uint8_t dpuTag;
    uint8_t dpuNE;
    uint8_t wepKeyIdxExtractEnable;
    uint8_t mcbcMgmtDpuIdx;
} hal_rxp_addr_tbl_t;

typedef struct hal_rxp_addr_tbl_info_s {
    uint8_t entry_cnt;
} hal_rxp_addr_tbl_info_t;

#define HAL_RXP_MAX_TABLE_ENTRY (HAL_NBSS_MAX + HAL_NSTA_MAX)

typedef struct hal_rxp_info_s {
    hal_rxp_addr_tbl_t addr1_table[HAL_RXP_MAX_TABLE_ENTRY];
    hal_rxp_addr_tbl_t addr2_table[HAL_RXP_MAX_TABLE_ENTRY];
    hal_rxp_addr_tbl_info_t addr1_tbl_info;
    hal_rxp_addr_tbl_info_t addr2_tbl_info;
} hal_rxp_info_t;

#if ((defined NT_FN_AP_HAL_DPH_DEBUG_STATS) || (defined NT_FN_STA_HAL_DPH_DEBUG_STATS) || \
     (defined NT_FN_AP_HAL_DPH_PRODUCTION_STATS) || (defined NT_FN_STA_HAL_DPH_PRODUCTION_STATS))
typedef struct hal_rxp_stats_s {
#ifdef NT_FN_AP_HAL_DPH_PRODUCTION_STATS
    uint32_t rx_ampdu;
    uint32_t mpdu_in_ampdu;
#endif  // NT_FN_AP_HAL_DPH_PRODUCTION_STATS
#if (defined NT_FN_AP_HAL_DPH_PRODUCTION_STATS) || (defined NT_FN_STA_HAL_DPH_PRODUCTION_STATS)
    uint32_t fcs_err;
#endif  // (define NT_FN_AP_HAL_DPH_PRODUCTION_STATS) || (define NT_FN_STA_HAL_DPH_PRODUCTION_STATS)
#if (defined NT_FN_AP_HAL_DPH_DEBUG_STATS) || (defined NT_FN_STA_HAL_DPH_DEBUG_STATS)
    uint32_t rx_mpdu;
    uint32_t phy_aborted;
    uint32_t phy_shutoff;
    uint32_t dlm_fifo_full;
    uint32_t dlm_err;
    uint32_t filter_fail;
    uint32_t max_pktlen_fail;
    uint32_t dma_send;
    uint32_t dma_drop;
    uint32_t fcs_error;
    uint32_t dma_get_bmu_fail;
    uint32_t protocol_ver_err;
    uint32_t type_subtype_filter;
    uint32_t incorrect_len_filter;
    uint32_t add1_block_filter;
    uint32_t add1_hit_no_pass;
    uint32_t add1_drop;
    uint32_t add2_hit_no_pass;
    uint32_t add2_drop;
    uint32_t add3_hit_no_pass;
    uint32_t add3_drop;
    uint32_t phy_err_drop;
    uint32_t start_err_pmi;
    uint32_t timeout_err;
    uint32_t stall_timeout;
    uint32_t flt_dupl_cnt;
    uint32_t filter_dma_drop;
#endif  // defined NT_FN_AP_HAL_DPH_DEBUG_STATS)||(defined NT_FN_STA_HAL_DPH_DEBUG_STATS)
} hal_rxp_stats_t;

typedef struct hal_tpe_rate_adapt_stats_s {
// TPE STA descriptor Rate adaptation stats per rate
#if (defined NT_FN_AP_HAL_DPH_DEBUG_STATS) || (defined NT_FN_STA_HAL_DPH_DEBUG_STATS)
    uint32_t tx_ppdu_cnt;
    uint32_t tx_ppdu_ack_to;
    uint32_t tx_mpdu_responded;
#endif  // NT_FN_AP_HAL_DPH_DEBUG_STATS)|| NT_FN_STA_HAL_DPH_DEBUG_STATS
#if (defined NT_FN_AP_HAL_DPH_PRODUCTION_STATS) || (defined NT_FN_STA_HAL_DPH_PRODUCTION_STATS)
    uint32_t tx_mpdu_cnt;
    uint32_t tx_mpdu_in_ampdu;
#endif  // (defined NT_FN_AP_HAL_DPH_PRODUCTION_STATS) || (defined NT_FN_STA_HAL_DPH_PRODUCTION_STATS)
#ifdef PLATFORM_FERMION
    uint32_t tx_pwr_reduct_cnt;
    uint32_t tx_pwr_reduct_to_cnt;
    uint32_t bt_abort_tx_cnt;
    uint32_t bt_abort_rx_cnt;
#endif
} hal_tpe_rate_adapt_stats_t;

#if (defined NT_FN_AP_HAL_DPH_PRODUCTION_STATS) || (defined NT_FN_STA_HAL_DPH_PRODUCTION_STATS)
typedef struct hal_tpe_80211_mib_stats_s {
    // TPE STA descriptor 802.11 MIB counters per Backoff Engine
    uint32_t tx_frgmt;
    uint32_t tx_successfrm;
    uint32_t tx_fail;
#ifdef NT_FN_AP_HAL_DPH_PRODUCTION_STATS
    uint32_t tx_mc_frm;
#endif  // NT_FN_AP_HAL_DPH_PRODUCTION_STATS
    uint32_t tx_retry;
    uint32_t tx_mult_retry;
    uint32_t tx_rts_success;
    uint32_t tx_rts_fail;
    uint32_t tx_ack_success;
    uint32_t tx_ack_fail;
} hal_tpe_80211_mib_stats_t;
#endif  // (define NT_FN_AP_HAL_DPH_PRODUCTION_STATS) || (define NT_FN_STA_HAL_DPH_PRODUCTION_STATS)

#if ((defined NT_FN_AP_HAL_DPH_DEBUG_STATS) || (defined NT_FN_STA_HAL_DPH_DEBUG_STATS) || \
     (defined NT_FN_AP_HAL_DPH_PRODUCTION_STATS) || (defined NT_FN_STA_HAL_DPH_PRODUCTION_STATS))
typedef struct hal_tpe_sta_stats_s {
    hal_tpe_rate_adapt_stats_t rate_adapt_stats[3];
#if ((defined NT_FN_AP_HAL_DPH_PRODUCTION_STATS) || (defined NT_FN_STA_HAL_DPH_PRODUCTION_STATS))
    hal_tpe_80211_mib_stats_t mib_stats[8];
#endif  //(defined NT_FN_AP_HAL_DPH_PRODUCTION_STATS)||(defined NT_FN_STA_HAL_DPH_PRODUCTION_STATS))
} hal_tpe_sta_stats_t;
#endif  //((defined NT_FN_AP_HAL_DPH_DEBUG_STATS)||(defined NT_FN_STA_HAL_DPH_DEBUG_STATS)||(defined
        //NT_FN_AP_HAL_DPH_PRODUCTION_STATS)||(defined NT_FN_STA_HAL_DPH_PRODUCTION_STATS))

typedef struct hal_dpu_stats_s {
#if ((defined NT_FN_AP_HAL_DPH_DEBUG_STATS) || (defined NT_FN_STA_HAL_DPH_DEBUG_STATS))
    uint32_t tx_sent_blocks;
    uint32_t rx_rcvd_blocks;
    uint32_t replays;
    uint32_t micerr;
    uint32_t prot_excluded;
    uint32_t format_err;
    uint32_t decrypt_err;
    uint32_t decrypt_success;

    uint32_t encryptMode;
    uint16_t txPktCount;
    uint16_t rxPktCount;
#endif
#if (defined NT_FN_AP_HAL_DPH_PRODUCTION_STATS) || (defined NT_FN_STA_HAL_DPH_PRODUCTION_STATS)
    uint32_t rx_frgmt_cnt;
    uint32_t undecrypt;
#endif  // (define NT_FN_AP_HAL_DPH_PRODUCTION_STATS) || (define NT_FN_STA_HAL_DPH_PRODUCTION_STATS)
} hal_dpu_stats_t;

#if (defined NT_FN_AP_HAL_DPH_DEBUG_STATS) || (defined NT_FN_STA_HAL_DPH_DEBUG_STATS)
typedef struct hal_dxestats_s {
    uint32_t dxe_counter0;
    uint32_t dxe_counter1;
    uint32_t dxe_counter2;
    uint32_t dxe_counter3;
    uint32_t dxe_counter4;
    uint32_t dxe_counter5;
    uint32_t dxe_counter6;

} hal_dxestats_t;
#endif  // NT_FN_AP_HAL_DPH_DEBUG_STATS|| NT_FN_STA_HAL_DPH_DEBUG_STATS

#if (defined NT_FN_AP_HAL_DPH_DEBUG_STATS) || (defined NT_FN_STA_HAL_DPH_DEBUG_STATS)
typedef struct hal_txp_stats_s {
    uint32_t tx_ppdu_frms;
    uint32_t tx_phy_aborts;
} hal_txp_stats_t;
#endif  //(defined NT_FN_AP_HAL_DPH_DEBUG_STATS)||(defined NT_FN_STA_HAL_DPH_DEBUG_STATS)

#if (defined NT_FN_AP_HAL_DPH_DEBUG_STATS) || (defined NT_FN_STA_HAL_DPH_DEBUG_STATS)
typedef struct hal_mpi_stats_s {
    uint16_t txp_mpi_start;
    uint16_t mpi_txp_req;
    uint16_t txp_mpi_data_valid;
    uint16_t txa_mpi_data_req;
    uint16_t mpi_txa_data_valid;
    uint16_t txb_mpi_data_req;
    uint16_t mpi_txb_data_valid;
    uint16_t mpi_txa_last_byte;
    uint16_t req_alarm;
    uint16_t watchdog_alarm;
    uint16_t cmd_alarm;
    uint16_t txp_abort;
    uint16_t txctl_abort;
} hal_mpi_stats_t;
#endif  // (defined NT_FN_AP_HAL_DPH_DEBUG_STATS)||(defined NT_FN_STA_HAL_DPH_DEBUG_STATS)

typedef struct hal_pmi_stats_s {
    uint16_t phy_start;
    uint16_t phy_data_valid;
    uint16_t phy_abort;
    uint16_t phy_interrupt;
    uint16_t rxa_packet;
    uint16_t rxa_data_valid;
    uint16_t rxb_packet;
    uint16_t rxb_data_valid;
    uint16_t rxp_shutoff;
    uint16_t crc_done;
    uint16_t crc_pass;
    uint16_t ampdu_bad;
    uint16_t ampdu_good;
    uint16_t ampdu_done;
} hal_pmi_stats_t;

#if (defined NT_FN_AP_HAL_DPH_PRODUCTION_STATS) || (defined NT_FN_STA_HAL_DPH_PRODUCTION_STATS)
typedef struct hal_rpe_stats_s {
    uint32_t rx_duplicate_cnt;
    uint32_t bitmap_update_cnt;
    uint32_t send_pkt_stats;
    uint32_t drop_pkt_stats;
} hal_rpe_stats_t;
#endif
#endif  //((defined NT_FN_AP_HAL_DPH_DEBUG_STATS)||(defined NT_FN_STA_HAL_DPH_DEBUG_STATS)||(defined
        //NT_FN_AP_HAL_DPH_PRODUCTION_STATS)||(defined NT_FN_STA_HAL_DPH_PRODUCTION_STATS))

#ifdef SUPPORT_BMU_ERROR_RECOVERY
typedef struct hal_bmu_recovery_info_s {
    uint32_t pre_recovery_tpe_ctrl;
    uint32_t pre_recovery_rxp_cfg;
    uint32_t pre_recovery_dxe_dma_csr;
} hal_bmu_recovery_info_t;
#endif /* SUPPORT_BMU_ERROR_RECOVERY */

#if (FERMION_CHIP_VERSION == 2)
void hal_modules_init(uint32_t phyRtFullHarfQuarter);
#else
void hal_modules_init(void);
#endif
void hal_modules_deinit(void);
void hal_modules_hw_setup(nt_hal_bss_t *bss, nt_hal_sta_t *sta);
void hal_mod_bmu_sta_disable(uint8_t staidx);
void hal_mod_rpe_desc_update(nt_hal_bss_t *bss, nt_hal_sta_t *sta);
void hal_mod_bmu_desc_update(nt_hal_bss_t *bss, nt_hal_sta_t *sta);
void hal_mod_rxp_desc_update(nt_hal_bss_t *__attribute__((__unused__)) bss,
                             nt_hal_sta_t *__attribute__((__unused__)) sta);
void hal_mod_rxp_sta_del(nt_hal_sta_t *sta);
void hal_mod_rxp_del_entry(uint8_t *macAddr);
void hal_mod_rxp_desc_set_mac(uint8_t *mac);
void hal_ba_add_setup(nt_hal_bss_t *bss, nt_hal_sta_t *sta);
void hal_ba_del_setup(nt_hal_bss_t *bss, nt_hal_sta_t *sta);
#if (FERMION_CHIP_VERSION == 2)
void hal_mod_wmmparams_add(struct chanAccParams *wmm, uint32_t phyRtFullHarfQuarter);
#else
void hal_mod_wmmparams_add(struct chanAccParams *wmm);
#endif
void hal_mod_wmmparam_cw(uint8_t qid, uint16_t cw_min, uint16_t cw_max);
void hal_mod_ba_win_size(uint16_t ack_timeout, uint16_t delay);
void hal_mod_slot_time(uint32_t slot_time);

/*
 * @brief  update wmm parameters with wmm params sent by SAP
 * @param  aifs_ac_vo, cwmin_ac_vo, cwmax_ac_vo, acvo_txoplimit
 * @return void
 */
void hal_mod_wmmparams_add_rbo_access(uint8_t aifs_ac_vo, uint16_t cwmin_ac_vo, uint16_t cwmax_ac_vo,
                                      uint8_t acvo_txoplimit);
/*
 * @brief  update wmm parameters with pifs offset sent by SAP
 * @param  pifs_offset
 * @return void
 */
void hal_mod_wmmparams_add_pifs(uint8_t pifs_offset);
/*
 * @brief Print avco wmm params
 * @param  void
 * @return void
 */
void hal_mod_wmmparam_print();

uint8_t hal_get_staid(uint8_t mode, uint8_t staid);
void hal_mod_tpe_desc_api(uint8_t sta_id, nt_hal_sta_tx_rate_t *value);
void hal_mod_tpe_desc_pm_set(nt_hal_bss_t *bss, uint8_t pm);
void hal_mod_tpe_tx_power_set(uint8_t sta, nt_hal_sta_tx_power_t *value);
void hal_mod_tpe_tx_power_get(uint8_t sta, nt_hal_sta_tx_power_t *value);
void hal_mod_tpe_rate_get(uint8_t sta, nt_hal_sta_tx_rate_t *value);

#ifdef NT_FN_CONCURRENCY
/*
 * @brief:To set rxp filter in AP_STA concurrency mode before association
 * @param : None
 * @returns: None
 */
void nt_hal_rxp_filter_conc_pre_assoc_mode_set(void);

/*
 * @brief:To set rxp filter in AP_STA concurrency mode after association
 * @param : None
 * @returns: None
 */
void nt_hal_rxp_filter_conc_post_assoc_mode_set(void);
#endif  // NT_FN_CONCURRENCY

/*
 * @brief:To set rxp filter in bss side
 * @param : None
 * @returns: None
 */
void nt_hal_rxp_filter_bss_set(void);

/*
 * @brief:To set rxp filter in sta side before association
 * @param : None
 * @returns: None
 */
void nt_hal_rxp_filter_sta_set_pre_assoc_mode(void);

/*
 * @brief:To set rxp filter in sta side after association
 * @param : None
 * @returns: None
 */
void nt_hal_rxp_filter_sta_set_post_assoc_mode(void);

/*
 * @brief:To set retry threshole
 * @param 1: staid
 * @returns: None
 */
void nt_hal_tpe_sta_desc_set(uint8_t staid);

/*
 * @brief:To set retry threshole
 * @param 1: retry bit = 0(disable retransmission) else enable retransmission
 * @param 2: staid
 * @returns: None
 */
void nt_hal_tpe_retran_set(uint8_t retry_bit, uint8_t staid);

#if ((defined NT_FN_AP_HAL_DPH_DEBUG_STATS) || (defined NT_FN_STA_HAL_DPH_DEBUG_STATS) || \
     (defined NT_FN_AP_HAL_DPH_PRODUCTION_STATS) || (defined NT_FN_STA_HAL_DPH_PRODUCTION_STATS))
/*
 * To get the rxp stats
 * Param: pointer to rxp stats structure
 */
nt_status_t hal_mod_rxp_stats_get(hal_rxp_stats_t *rxp_stats);
/*
 * To get the tpe stats
 * Param1: bss mode to get staidx
 * Param2: staid of sta entry
 * Param3: pointer to the tpe stats structure
 * */
nt_status_t hal_mod_tpe_sta_stats_get(uint8_t mode, uint8_t staid, hal_tpe_sta_stats_t *tpe_stats);
/*
 * To get the dpu stats
 * Param1: bss mode to get staidx
 * Param2: staid of sta entry
 * Param3: pointer to the dpu stats structure
 * */
nt_status_t hal_mod_dpu_stats_get(uint8_t mode, uint8_t staid, hal_dpu_stats_t *dpu_stats);
/*
 * To get the dxe stats
 * Param: pointer to dxe stats*/
#if ((defined NT_FN_AP_HAL_DPH_DEBUG_STATS) || (defined NT_FN_STA_HAL_DPH_DEBUG_STATS))
nt_status_t hal_mod_dxe_stats_get(hal_dxestats_t *dxe_stats);
#endif  // NT_FN_AP_HAL_DPH_DEBUG_STATS|| NT_FN_STA_HAL_DPH_DEBUG_STATSs
/*
 * To get the txp_stats
 * Param: pointer to txp stats*/
#if ((defined NT_FN_AP_HAL_DPH_DEBUG_STATS) || (defined NT_FN_STA_HAL_DPH_DEBUG_STATS))
nt_status_t hal_mod_txp_stats_get(hal_txp_stats_t *txp_stats);
#endif  // ((defined NT_FN_AP_HAL_DPH_DEBUG_STATS)||(defined NT_FN_STA_HAL_DPH_DEBUG_STATS))
/*
 * To get the MPI stats
 * Param: pointer to mpi stats*/
#if (defined NT_FN_AP_HAL_DPH_DEBUG_STATS) || (defined NT_FN_STA_HAL_DPH_DEBUG_STATS)
nt_status_t hal_mod_mpi_stats_get(hal_mpi_stats_t *mpi_stats);
#endif  // (defined NT_FN_AP_HAL_DPH_DEBUG_STATS)||(defined NT_FN_STA_HAL_DPH_DEBUG_STATS)
/*
 * To get the PMI stats
 * Param: pointer to pmi stats*/
nt_status_t hal_mod_pmi_stats_get(hal_pmi_stats_t *pmi_stats);
/*
 * To get rpe stats
 * Param: pointer to rpe stats
 */
#if (defined NT_FN_AP_HAL_DPH_PRODUCTION_STATS) || (defined NT_FN_STA_HAL_DPH_PRODUCTION_STATS)
nt_status_t hal_mod_rpe_stats_get(hal_rpe_stats_t *rpe_stats);
#endif  // (define NT_FN_AP_HAL_DPH_PRODUCTION_STATS) || (define NT_FN_STA_HAL_DPH_PRODUCTION_STATS)

nt_status_t hal_mod_tpe_stats_clear(uint8_t mode, uint8_t staid, hal_tpe_sta_stats_t *tpe_stats);

#if (defined NT_FN_AP_HAL_DPH_DEBUG_STATS) || (defined NT_FN_STA_HAL_DPH_DEBUG_STATS)
nt_status_t hal_mod_dxe_stats_clear(hal_dxestats_t *dxe_stats);
nt_status_t hal_mod_txp_stats_clear(hal_txp_stats_t *txp_stats);
#endif  // NT_FN_AP_HAL_DPH_DEBUG_STATS|| NT_FN_STA_HAL_DPH_DEBUG_STATS

nt_status_t hal_mod_rxp_stats_clear(hal_rxp_stats_t *rxp_stats);
nt_status_t hal_mod_dpu_stats_clear(uint8_t mode, uint8_t staid, hal_dpu_stats_t *dpu_stats);

#endif  //((defined NT_FN_AP_HAL_DPH_DEBUG_STATS)||(defined NT_FN_STA_HAL_DPH_DEBUG_STATS)||(defined
        //NT_FN_AP_HAL_DPH_PRODUCTION_STATS)||(defined NT_FN_STA_HAL_DPH_PRODUCTION_STATS))
/*===========================================================
  RXP Filter Cfg for Different Frames
  ===========================================================*/
typedef enum eFrameSubType {
    eMGMT_ASSOC_REQ = 0,
    eMGMT_ASSOC_RSP,             // 1
    eMGMT_REASSOC_REQ,           // 2
    eMGMT_REASSOC_RSP,           // 3
    eMGMT_PROBE_REQ,             // 4
    eMGMT_PROBE_RSP,             // 5
    eMGMT_RSVD1,                 // 6
    eMGMT_RSVD2,                 // 7
    eMGMT_BEACON,                // 8
    eMGMT_ATIM,                  // 9
    eMGMT_DISASSOC,              // 10
    eMGMT_AUTH,                  // 11
    eMGMT_DEAUTH,                // 12
    eMGMT_ACTION,                // 13
    eMGMT_ACTION_NOACK,          // 14
    eMGMT_RSVD4,                 // 15
    eCTRL_RSVD1,                 // 16
    eCTRL_RSVD2,                 // 17
    eCTRL_RSVD3,                 // 18
    eCTRL_RSVD4,                 // 19
    eCTRL_BR_POLL,               // 20
    eCTRL_NDPA,                  // 21
    eCTRL_RSVD7,                 // 22
    eCTRL_CONTROL_WRAPPER,       // 23
    eCTRL_BAR,                   // 24
    eCTRL_BA,                    // 25
    eCTRL_PSPOLL,                // 26
    eCTRL_RTS,                   // 27
    eCTRL_CTS,                   // 28
    eCTRL_ACK,                   // 29
    eCTRL_CFEND,                 // 30
    eCTRL_CFEND_CFACK,           // 31
    eDATA_DATA,                  // 32
    eDATA_DATA_CFACK,            // 33
    eDATA_DATA_CFPOLL,           // 34
    eDATA_DATA_CFACK_CFPOLL,     // 35
    eDATA_NULL,                  // 36
    eDATA_CFACK,                 // 37
    eDATA_CFPOLL,                // 38
    eDATA_CFACK_CFPOLL,          // 39
    eDATA_QOSDATA,               // 40
    eDATA_QOSDATA_CFACK,         // 41
    eDATA_QOSDATA_CFPOLL,        // 42
    eDATA_QOSDATA_CFACK_CFPOLL,  // 43
    eDATA_QOSNULL,               // 44
    eDATA_RSVD1,                 // 45
    eDATA_QOS_CFPOLL,            // 46
    eDATA_QOS_CFACK_CFPOLL,      // 47
    eRSVD_RSVD0,                 // 48
    eRSVD_RSVD1,                 // 49
    eRSVD_RSVD2,                 // 50
    eRSVD_RSVD3,                 // 51
    eRSVD_RSVD4,                 // 52
    eRSVD_RSVD5,                 // 53
    eRSVD_RSVD6,                 // 54
    eRSVD_RSVD7,                 // 55
    eRSVD_RSVD8,                 // 56
    eRSVD_RSVD9,                 // 57
    eRSVD_RSVD10,                // 58
    eRSVD_RSVD11,                // 59
    eRSVD_RSVD12,                // 60
    eRSVD_RSVD13,                // 61
    eRSVD_RSVD14,                // 62
    eRSVD_RSVD15,                // 63
    RXP_FRAME_TYPES_MAX,
} tFrameSubType;
typedef struct sRxpFilterConfig {
    tFrameSubType frameType;
    uint32_t configValue;
} tRxpFilterConfig;

#define RXP_TYPE_SUBTYPE_MASK(x)       ((x >= eDATA_DATA) ? ((uint32)1 << (x - eDATA_DATA)) : ((uint32)1 << x))
#define FILTER_ALL_MULTICAST           0x01
#define FILTER_ALL_BROADCAST           0x02
#define FILTER_ALL_MULTICAST_BROADCAST 0x03

#define RXP_NAV_SET_ON_ABORT               QWLAN_RXP_FRAME_FILTER_CONFIG_NAV_SET_ENABLE_ON_ABORT_MASK
#define RXP_NAV_CLEAR_ON_ABORT             QWLAN_RXP_FRAME_FILTER_CONFIG_NAV_CLEAR_ENABLE_ON_ABORT_MASK
#define RXP_BLOCK                          QWLAN_RXP_FRAME_FILTER_CONFIG_BLOCK_RECEPTION_ENABLE_MASK
#define RXP_ADDR1_BLOCK_BROADCAST          QWLAN_RXP_FRAME_FILTER_CONFIG_ADDR1_BLOCK_BROADCAST_ENABLE_MASK
#define RXP_ADDR1_BLOCK_MULTICAST          QWLAN_RXP_FRAME_FILTER_CONFIG_ADDR1_BLOCK_MULTICAST_ENABLE_MASK
#define RXP_ADDR1_BLOCK_UNICAST            QWLAN_RXP_FRAME_FILTER_CONFIG_ADDR1_BLOCK_UNICAST_ENABLE_MASK
#define RXP_ADDR1_FILTER                   QWLAN_RXP_FRAME_FILTER_CONFIG_ADDR1_BINARY_SEARCH_FILTER_ENABLE_MASK
#define RXP_ADDR1_ACCEPT_MULTICAST         QWLAN_RXP_FRAME_FILTER_CONFIG_ADDR1_ACCEPT_REMAINING_MULTICAST_ENABLE_MASK
#define RXP_ADDR1_ACCEPT_UNICAST           QWLAN_RXP_FRAME_FILTER_CONFIG_ADDR1_ACCEPT_REMAINING_UNICAST_ENABLE_MASK
#define RXP_ADDR2_FILTER                   QWLAN_RXP_FRAME_FILTER_CONFIG_ADDR2_BINARY_SEARCH_FILTER_ENABLE_MASK
#define RXP_ADDR2_ACCEPT_REMAIN            QWLAN_RXP_FRAME_FILTER_CONFIG_ADDR2_ACCEPT_REMAINING_ENABLE_MASK
#define RXP_ADDR3_FILTER                   QWLAN_RXP_FRAME_FILTER_CONFIG_ADDR3_BINARY_SEARCH_FILTER_ENABLE_MASK
#define RXP_ADDR3_ACCEPT_REMAIN            QWLAN_RXP_FRAME_FILTER_CONFIG_ADDR3_ACCEPT_REMAINING_ENABLE_MASK
#define RXP_PHY_RX_ABORT                   QWLAN_RXP_FRAME_FILTER_CONFIG_PHY_RX_ABORT_ENABLE_MASK
#define RXP_FRAME_TRANSLATION              QWLAN_RXP_FRAME_FILTER_CONFIG_FRAME_TRANSLATION_REQ_EN_MASK
#define RXP_ROUTING_FLAG_SEL               QWLAN_RXP_FRAME_FILTER_CONFIG_ROUTING_FLAG_SEL_MASK
#define RXP_DROP_AT_DMA                    QWLAN_RXP_FRAME_FILTER_CONFIG_G5_DROP_AT_DMA_MASK
#define RXP_PM_BIT_EVAL                    QWLAN_RXP_FRAME_FILTER_CONFIG_PM_BIT_EVAL_REQUIRED_ENABLE_MASK
#define RXP_NONHT_BW_IND                   QWLAN_RXP_FRAME_FILTER_CONFIG_NONHT_BW_INDICATION_ENABLE_MASK
#define RXP_NONHT_BW_INDICATION_ENABLE_BIT QWLAN_RXP_FRAME_FILTER_CONFIG_NONHT_BW_INDICATION_ENABLE_MASK

// Derived from above
#define RXP_ACCEPT_ALL_ADDR1   (RXP_ADDR1_FILTER | RXP_ADDR1_ACCEPT_MULTICAST | RXP_ADDR1_ACCEPT_UNICAST)
#define RXP_ACCEPT_ALL_ADDR2   (RXP_ADDR2_FILTER | RXP_ADDR2_ACCEPT_REMAIN)
#define RXP_ACCEPT_ALL_ADDR3   (RXP_ADDR3_FILTER | RXP_ADDR3_ACCEPT_REMAIN)
#define RXP_ACCEPT_ALL_ADDRESS (RXP_ACCEPT_ALL_ADDR1 | RXP_ACCEPT_ALL_ADDR2 | RXP_ACCEPT_ALL_ADDR3)

#define TPE_FW_TPE_TX_COMPLETE_FEEDBACK_REG          QWLAN_TPE_TPE_MCU_BD_BASED_TX_INT_FEEDBACK_REG
#define TPE_HOST_TPE_TX_COMPLETE_FEEDBACK_REG        QWLAN_TPE_TPE_MCU_BD_BASED_TX_INT_FEEDBACK_1_REG
#define TPE_ACK_TO_MASK                              0x1  // mask to check whether ACK is valid or not in the feedback register.
#define TPE_TX_COMPLETE_FEEDBACK_MASK                0xF  // applies to both txComplete0 and txComplete1
#define IS_TPE_TX_COMPLETE_FEEDBACK_FRAME_DROPPED(x) (((x == 0x3) || (x == 0x5)) ? 1 : 0)

#define TPE_FEEDBACK_MASK   0x70000
#define TPE_FEEDBACK_OFFSET 0x10
#define BD_INDEX_MASK       0xFFFF

#define HAL_RXP_ADDR_ENTRY (32)

/* New feature flag */
typedef enum {
    TX_SUCCESS,
    TX_RETRY_LIMIT_REACHED,
    TX_TIMEOUT,
    TX_ERROR,
} eTxFrmWithTxComplStatus;

/* Enum for TPE feed back status */
typedef enum {
    TPE_FEEDBACK_NO_FEEDBACK = 0,
    TPE_FEEDBACK_ACK_RECEIVED,
    TPE_FEEDBACK_RETRY_LIMIT_REACHED,
} eTpeFeedbackStatus;

typedef enum {
    TX_BD_CPLT_DISABLED = 0,
    TX_BD_CPLT_BIT0_EN = 1 << 0,
    TX_BD_CPLT_BIT1_EN = 1 << 1,
} eTX_BD_CPLT_MODE;

typedef enum {
    BAND_2_4G_CCK = 0,
    BAND_2_4G_OFDM = 1,
    BAND_5G_OFDM = 2,
    BAND_6G_OFDM = 3

} fre_bands_e;

#if (FERMION_CHIP_VERSION == 2)
typedef enum {
    PHY_RATE_FULL = 0,
    PHY_RATE_HALF = 1,
    PHY_RATE_QUATER = 2,
    PHY_RATE_MAX = 3,
} ePhyRate_F_H_Q;
#endif

typedef enum {
    PHY_PD_LISTEN = 0,  // PHY_RX (PHY_RXA) On
    PHY_PD_CFG = 1,     // PHY_RX + PHY_RXA + PHY_TX On
    PHY_PD_TX = 2,      // PHY_TX On
    PHY_PD_INVALID = 0xFF,
} phy_powerdomain_state_e;

typedef struct _macTimingParams {
    uint32_t macClockFrequency_MHz;
    uint32_t sifs_usecs;
    uint32_t slotTime_usecs;
    uint32_t pifs_usecs;
    uint32_t eifs_usecs;
} macTimingParams, *pmacTimingParams;
int32_t hal_get_timing_param(fre_bands_e fre_bands, macTimingParams *pMacTimingParams);
void hal_set_tx_complete(uint32_t wq, uint32_t mode);
void hal_set_probe_response_sync(uint32_t mode, uint8_t *mac);
uint32_t hal_get_acked_mpdu(uint8_t hw_staidx);
void hal_jammer_set(uint8_t mode, uint32_t preload, uint32_t thres);
void hal_nlbt_queue_set(uint8_t mode, uint32_t value);
void hal_mod_tpe_min_txpower_set(nt_hal_bss_t *bss, nt_hal_sta_t *sta, uint8_t min_txpwr_rate0, uint8_t min_txpwr_rate1,
                                 uint8_t min_txpwr_rate2);
void hal_mod_tpe_min_txpower_get(nt_hal_bss_t *bss, nt_hal_sta_t *sta, uint8_t *min_txpwr_rate0,
                                 uint8_t *min_txpwr_rate1, uint8_t *min_txpwr_rate2);
void hal_mod_tpe_txprio_set(nt_hal_bss_t *bss, nt_hal_sta_t *sta, uint8_t txprio);
void hal_mod_tpe_queue_txprio_set(nt_hal_bss_t *bss, nt_hal_sta_t *sta, uint8_t qid, uint8_t queue_txprio);
void hal_modules_init_rest(uint16_t rate_id);
void hal_phy_power_phy_init(void);
void hal_set_ampdu_tx_time(uint32_t maxAmpduTxTime);
void hal_set_sta_aid(uint16_t aid);
void hal_set_phyint(uint32_t fastIntFlag, uint32_t slowIntFlag, uint32_t hostIntFlag);
void hal_bd_set_bdrate(p_dpm_bd_tx_template pBd, uint8_t bdrate);
// void hal_mod_tpe_bdrate_set( nt_hal_bss_t* bss, nt_hal_sta_t* sta, nt_hal_sta_tx_rate_t *value);
void hal_mod_tpe_bdrate_set(uint8_t staid, nt_hal_sta_tx_rate_t *value);
void hal_phy_power_switch_to_cfg();
void hal_phy_power_switch_to_listen();
void hal_mac_sw_powerup();
void hal_mac_hw_ctrl();
void nt_hal_tpe_retry_threshold_set(uint8_t staid, uint8_t retry_bit, uint8_t retry_threshold0,
                                    uint8_t retry_threshold1, uint8_t retry_threshold2);

#ifdef PHY_POWER_SWITCH
void hal_phy_power_switch_to_cfg(void);
void hal_phy_power_switch_to_listen(void);
void hal_phy_power_switch_to_tx(void);
void hal_phy_power_switch_to_previous_state(void);
void hal_phy_set_wifi_ss_to_tx(void);
void hal_phy_set_wifi_ss_to_rxb_listen(void);
void hal_phy_power_ftm_switch_to_listen(void);
#endif
void hal_mac_sw_powerup();
void hal_mac_hw_ctrl();
void hal_mod_txp_reset();
#ifdef FERMION_TXP_TPE_WAR
void hal_txp_tpe_reinit(void);
#endif

void delay(uint32_t delay_count);

typedef enum _tsf_mode { TSF_BSS_MODE = 0, TSF_IBSS_MODE, TSF_P2P_GO_MODE, TSF_P2P_C_MODE, TSF_BSS2_MODE } TSF_MODE;

/*RPE,TPE SM STUCK defines*/

#define TPE_MPROC_BIT_MAP_MASK   0x7e0
#define TPE_MPROC_BIT_MAP_OFFSET 0x5

#define RPE_WAIT_4_DUP_DET     0x10
#define TPEM_WAIT_GET_RPE_INFO 0x4

/* init for reg power, ctl power and target power */
#define QAPI_REG_POWER_MAX    200
#define QAPI_CTL_POWER_MAX    200
#define QAPI_TARGET_POWER_MAX 200

/*
 *  Beacon TSF compensation = Preamble   +   MPDU Header                 +  Extra Delay
 *  1Mbps(2.4Ghz):            192(long)      192 (24 bytes @ 1Mbps)         4
 *  6Mbps(2.4Ghz):            20(short)      32  (24 bytes @ 6Mbps)         4
 *  6Mbps(5Ghz):              20             32  (24 bytes @ 6Mbps)         4
 *  11Mbps                      192(long)    18  (24 bytes @ 11Mbps)        4
 */
#define TPE_BEACON_TSF_COMPENSATION_1MBPS_LONG  388  // us
#define TPE_BEACON_TSF_COMPENSATION_6MBPS       56   // us
#define TPE_BEACON_TSF_COMPENSATION_11MBPS_LONG 214  // us

#define POP_CMD(wq)            (*(volatile uint32_t *)(BMU_COMMAND_BASE_ADDRESS | (wq << 8) | POP_WQ_CMDTYPE))
#define PUSH_CMD(wq, bdIdx)    (*(volatile uint32_t *)(BMU_COMMAND_BASE_ADDRESS | (wq << 8) | PUSH_WQ_CMDTYPE)) = (bdIdx)
#define READ_HEAD_CMD(wq)      (*(volatile uint32_t *)(BMU_COMMAND_BASE_ADDRESS | (wq << 8) | READ_WQ_HEAD_CMDTYPE))
#define PUSH_SINK_CMD(bdIdx)   PUSH_CMD(HAL_BMUWQ_SINK, bdIdx)
#define BMU_READ_WQ_HD_CMD(wq) READ_HEAD_CMD(wq)
#define BMU_READ_WQ_TL_CMD(wq) (*(volatile uint32_t *)(BMU_COMMAND_BASE_ADDRESS | (wq << 8) | READ_WQ_TAIL_CMDTYPE))
#define BMU_READ_WQ_NR_CMD(wq) (*(volatile uint32_t *)(BMU_COMMAND_BASE_ADDRESS | (wq << 8) | READ_WQ_NR_CMDTYPE))
#define READ_BD_PTR_CMD(bdIdx) \
    (*(volatile uint32_t *)(BMU_COMMAND_BASE_ADDRESS | (bdIdx << 8) | READ_BD_POINTER_CMDTYPE))

#ifdef SUPPORT_BMU_ERROR_RECOVERY
/* BMU recovery sequence for recoevering from BMU errors */
void hal_bmu_error_recovery(void);
#endif /* SUPPORT_BMU_ERROR_RECOVERY */
void hal_set_ampdu_density(nt_hal_bss_t *bss, uint8_t mpdu_density);

#endif
