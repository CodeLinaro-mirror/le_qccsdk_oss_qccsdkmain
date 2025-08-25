/*========================================================================
Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
*
* @file coex_mci.h
* @brief MCI interface related params and struct definitions
*========================================================================*/
#ifndef COEX_MCI_H
#define COEX_MCI_H

/*-------------------------------------------------------------------------
 * Include Files
 * ----------------------------------------------------------------------*/
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "nt_osal.h"

#include "fwconfig_wlan.h"
#include "nt_flags.h"
#include "nt_common.h"

#if defined(SUPPORT_COEX)
#include "coex_wlan.h"
#include "coex_main.h"
#if defined(PLATFORM_FERMION)

#include "hal_coex.h"
// To set LNA timeout value.
#define LNA_TIMEOUT_VAL      5000
#define MCI_RECOVERY_TIMEOUT (5)

#define MCI_INTERRUPT_THRESHOLD (0xFF)

#define MCI_GPM_INVALID8 0xFE
#define MCI_GPM_INVALID  (int)0xFEFEFEFE

#define MCI_DEFAULT_CCA_VALUE     28
#define MCI_DEFAULT_CCA_VALUE_20M 28
#define MCI_DEFAULT_CCA_VALUE_40M 26
#define MCI_DEFAULT_CCA_VALUE_80M 26

#define MCI_WMAC1_WAN_TX_CCA_VALUE_20M 30
#define MCI_WMAC1_WAN_TX_CCA_VALUE_40M 30
#define MCI_WMAC1_WAN_TX_CCA_VALUE_80M 30

#define MCI_BT_WAN_TX_CCA_VALUE_20M 30
#define MCI_BT_WAN_TX_CCA_VALUE_40M 30
#define MCI_BT_WAN_TX_CCA_VALUE_80M 30

#define MCI_BT_TX_CCA_VALUE_20M 30
#define MCI_BT_TX_CCA_VALUE_40M 30
#define MCI_BT_TX_CCA_VALUE_80M 30

#define MCI_RESET_MAX_RETRIES 3

#define COEX_BMH_ERR_WMAC1                    0x00000001
#define COEX_BMH_ERR_WMAC2                    0x00000002
#define COEX_BMH_ERR_RX_INVALID_HDR           0x00000004
#define COEX_BMH_ERR_WLTXSM_INVALID_SEQ_WMAC1 0x00000008
#define COEX_BMH_ERR_WLTXSM_INVALID_SEQ_WMAC2 0x00000010
#define COEX_BMH_ERR_CONT_INFO_TO             0x00000020
#define COEX_BMH_ERR_INVALID_BT_PWR           0x00000040
#define COEX_BMH_ERR_REG_ACCESS               0x00000080
#define COEX_BMH1_ERR_WL_IN_TX_WD_TO          0x00000100
#define COEX_BMH1_ERR_WL_IN_RX_WD_TO          0x00000200
#define COEX_BMH2_ERR_WL_IN_TX_WD_TO          0x00000400
#define COEX_BMH2_ERR_WL_IN_RX_WD_TO          0x00000800
#define COEX_BMH1_ERR_WL_TX_RX                0x00001000
#define COEX_BMH2_ERR_WL_TX_RX                0x00002000
#define COEX_BMH1_ERR_TLV_OUT_TIMEOUT         0x00004000
#define COEX_BMH2_ERR_TLV_OUT_TIMEOUT         0x00008000
#define COEX_BMH3_ERR_WL_IN_TX_WD_TO          0x00010000
#define COEX_BMH3_ERR_WL_IN_RX_WD_TO          0x00020000
#define COEX_BMH3_ERR_WL_TX_RX                0x00040000
#define COEX_BMH3_ERR_TLV_OUT_TIMEOUT         0x00080000

#define COEX_MCIM_ISR_ERR_BUS_SYNC 0x1
#define COEX_MCIM_ISR_ERR_RAW      0x2
#define COEX_MCIM_ISR_ERR_OVERFLOW 0x4

#define COEX_LMH_ERR_LNA_STATE_TO  0x1
#define COEX_LMH_ERR_LNA_INUSE_TO  0x2
#define COEX_LMH_ERR_LNA_LOCKED_TO 0x4
#define COEX_LMH_ERR_CCU_ACCR_TO   0x8

#define COEX_MCIM_ERR_HNDLR_SW_T1      0x00000001
#define COEX_MCIM_ERR_HNDLR_SW_T2      0x00000002
#define COEX_MCIM_ERR_HNDLR_BTC_T1     0x00000004
#define COEX_MCIM_ERR_HNDLR_BTC_T2     0x00000008
#define COEX_MCIM_ERR_HNDLR_LNA_T1     0x00000010
#define COEX_MCIM_ERR_HNDLR_LNA_T2     0x00000020
#define COEX_MCIM_ERR_HNDLR_LTE_T1     0x00000040
#define COEX_MCIM_ERR_HNDLR_LTE_T2     0x00000080
#define COEX_MCIM_ERR_HNDLR_PMU_T1     0x00000100
#define COEX_MCIM_ERR_HNDLR_PMU_T2     0x00000200
#define COEX_MCIM_ERR_HNDLR_CHKSUM     0x00000400
#define COEX_MCIM_ERR_HNDLR_BUS_SYNC   0x00000800
#define COEX_MCIM_ERR_HNDLR_WSI_STAT   0x00001000
#define COEX_MCIM_ERR_HNDLR_TX_NAK     0x00002000
#define COEX_MCIM_ERR_HNDLR_RX_NAK     0x00004000
#define COEX_MCIM_ERR_HNDLR_DEST       0x00008000
#define COEX_MCIM_ERR_HNDLR_FIFO_OW    0x00010000
#define COEX_MCIM_ERR_HNDLR_CCU_ACCESS 0x00020000
#define COEX_MCIM_ERR_HNDLR_SETUP_TIME 0x00040000
#define COEX_MCIM_ERR_HNDLR_INJ        0x00080000

#define COEX_SMH_ERR_HNDLR_RX_MCI_GPM_FULL         0x001
#define COEX_SMH_ERR_HNDLR_GPM_SKIP                0x002
#define COEX_SMH_ERR_HNDLR_AHB                     0x004
#define COEX_SMH_ERR_HNDLR_CCU_ACCESS              0x008
#define COEX_SMH_ERR_HNDLR_ISR_OVERFLOW            0x010
#define COEX_SMH_ERR_HNDLR_RX_MCI_GPM_FULL_ERR_INT 0x020
#define COEX_SMH_ERR_HNDLR_AHB_ERR_INT             0x040
#define COEX_SMH_ERR_HNDLR_CCU_ACCESS_ERR_INT      0x080
#define COEX_SMH_ERR_HNDLR_GPM_SKIP_ERR_INT        0x100

#define COEX_PMH_ERR_PWR_DOWN_TO_MASK  0x1
#define COEX_PMH_ERR_BT_RESP_TO_MASK   0x2
#define COEX_PMH_ERR_WSI_BUS_SYNC_MASK 0x4
#define COEX_PMH_ERR_CCU_ACCESS_MASK   0x8

#define COEX_BMH_ERR_WMAC1                    0x00000001
#define COEX_BMH_ERR_WMAC2                    0x00000002
#define COEX_BMH_ERR_RX_INVALID_HDR           0x00000004
#define COEX_BMH_ERR_WLTXSM_INVALID_SEQ_WMAC1 0x00000008
#define COEX_BMH_ERR_WLTXSM_INVALID_SEQ_WMAC2 0x00000010
#define COEX_BMH_ERR_CONT_INFO_TO             0x00000020
#define COEX_BMH_ERR_INVALID_BT_PWR           0x00000040
#define COEX_BMH_ERR_REG_ACCESS               0x00000080
#define COEX_BMH1_ERR_WL_IN_TX_WD_TO          0x00000100
#define COEX_BMH1_ERR_WL_IN_RX_WD_TO          0x00000200
#define COEX_BMH2_ERR_WL_IN_TX_WD_TO          0x00000400
#define COEX_BMH2_ERR_WL_IN_RX_WD_TO          0x00000800
#define COEX_BMH1_ERR_WL_TX_RX                0x00001000
#define COEX_BMH2_ERR_WL_TX_RX                0x00002000
#define COEX_BMH1_ERR_TLV_OUT_TIMEOUT         0x00004000
#define COEX_BMH2_ERR_TLV_OUT_TIMEOUT         0x00008000
#define COEX_BMH3_ERR_WL_IN_TX_WD_TO          0x00010000
#define COEX_BMH3_ERR_WL_IN_RX_WD_TO          0x00020000
#define COEX_BMH3_ERR_WL_TX_RX                0x00040000
#define COEX_BMH3_ERR_TLV_OUT_TIMEOUT         0x00080000

#define COEX_MCIM_ISR_ERR_BUS_SYNC 0x1
#define COEX_MCIM_ISR_ERR_RAW      0x2
#define COEX_MCIM_ISR_ERR_OVERFLOW 0x4

#define COEX_LMH_ERR_LNA_STATE_TO  0x1
#define COEX_LMH_ERR_LNA_INUSE_TO  0x2
#define COEX_LMH_ERR_LNA_LOCKED_TO 0x4
#define COEX_LMH_ERR_CCU_ACCR_TO   0x8

#define COEX_MCIM_ERR_HNDLR_SW_T1      0x00000001
#define COEX_MCIM_ERR_HNDLR_SW_T2      0x00000002
#define COEX_MCIM_ERR_HNDLR_BTC_T1     0x00000004
#define COEX_MCIM_ERR_HNDLR_BTC_T2     0x00000008
#define COEX_MCIM_ERR_HNDLR_LNA_T1     0x00000010
#define COEX_MCIM_ERR_HNDLR_LNA_T2     0x00000020
#define COEX_MCIM_ERR_HNDLR_LTE_T1     0x00000040
#define COEX_MCIM_ERR_HNDLR_LTE_T2     0x00000080
#define COEX_MCIM_ERR_HNDLR_PMU_T1     0x00000100
#define COEX_MCIM_ERR_HNDLR_PMU_T2     0x00000200
#define COEX_MCIM_ERR_HNDLR_CHKSUM     0x00000400
#define COEX_MCIM_ERR_HNDLR_BUS_SYNC   0x00000800
#define COEX_MCIM_ERR_HNDLR_WSI_STAT   0x00001000
#define COEX_MCIM_ERR_HNDLR_TX_NAK     0x00002000
#define COEX_MCIM_ERR_HNDLR_RX_NAK     0x00004000
#define COEX_MCIM_ERR_HNDLR_DEST       0x00008000
#define COEX_MCIM_ERR_HNDLR_FIFO_OW    0x00010000
#define COEX_MCIM_ERR_HNDLR_CCU_ACCESS 0x00020000
#define COEX_MCIM_ERR_HNDLR_SETUP_TIME 0x00040000
#define COEX_MCIM_ERR_HNDLR_INJ        0x00080000

#define COEX_SMH_ERR_HNDLR_RX_MCI_GPM_FULL         0x001
#define COEX_SMH_ERR_HNDLR_GPM_SKIP                0x002
#define COEX_SMH_ERR_HNDLR_AHB                     0x004
#define COEX_SMH_ERR_HNDLR_CCU_ACCESS              0x008
#define COEX_SMH_ERR_HNDLR_ISR_OVERFLOW            0x010
#define COEX_SMH_ERR_HNDLR_RX_MCI_GPM_FULL_ERR_INT 0x020
#define COEX_SMH_ERR_HNDLR_AHB_ERR_INT             0x040
#define COEX_SMH_ERR_HNDLR_CCU_ACCESS_ERR_INT      0x080
#define COEX_SMH_ERR_HNDLR_GPM_SKIP_ERR_INT        0x100

#define COEX_PMH_ERR_PWR_DOWN_TO_MASK  0x1
#define COEX_PMH_ERR_BT_RESP_TO_MASK   0x2
#define COEX_PMH_ERR_WSI_BUS_SYNC_MASK 0x4
#define COEX_PMH_ERR_CCU_ACCESS_MASK   0x8

#define DEFAULT_CONT_INFO_TIMEOUT         20
#define BT_RESPONSE_TIMEOUT               0xFF
#define COEX_MTU_BEACON_RX_RECEIVE_WINDOW 3000

// WmacPowerUpFlag
// bit0-bit3:  bit0--MAC0 is going to powerup,  bit1-- MAC1 is going to powerup, bit2-- MAC2 is going to powerup
// bit4-bit7: bit4--MAC0 is already powered up, bit5-- MAC1 is already powered up, bit6-- MAC2 is already powered up
#define COEX_POWERUP_SHIFT 4
#define COEX_POWERUP_MASK  0x70

/*CxC Settings Bitmap */

#define COEX_CXC_SETTINGS_MASK_2G              (0x01)
#define COEX_CXC_SETTINGS_MASK_2x2             (0x02)
#define COEX_CXC_SETTINGS_MASK_PWR_UP_REQUIRED (0x04)
#define COEX_CXC_SETTINGS_MASK_5G              (0x08)

#define COEX_CXC_SETTINGS_SET_2G(mac)              ((mac) |= COEX_CXC_SETTINGS_MASK_2G)
#define COEX_CXC_SETTINGS_SET_2x2(mac)             ((mac) |= COEX_CXC_SETTINGS_MASK_2x2)
#define COEX_CXC_SETTINGS_SET_PWR_UP_REQUIRED(mac) ((mac) |= COEX_CXC_SETTINGS_MASK_PWR_UP_REQUIRED)
#define COEX_CXC_SETTINGS_SET_5G(mac)              ((mac) |= COEX_CXC_SETTINGS_MASK_5G)

#define COEX_CXC_SETTINGS_IS_2G(mac)              ((mac)&COEX_CXC_SETTINGS_MASK_2G)
#define COEX_CXC_SETTINGS_IS_2x2(mac)             ((mac)&COEX_CXC_SETTINGS_MASK_2x2)
#define COEX_CXC_SETTINGS_IS_PWR_UP_REQUIRED(mac) ((mac)&COEX_CXC_SETTINGS_MASK_PWR_UP_REQUIRED)
#define COEX_CXC_SETTINGS_IS_5G(mac)              ((mac)&COEX_CXC_SETTINGS_MASK_5G)

// set which wlan is going to power up
#define COEX_CHECK_NEEDED_POWER_UP_MAC(wmac) (gpBtCoexWlanInfoDev->WmacPowerUpFlag & (1 << wmac))
#define COEX_SET_NEEDED_POWER_UP_MAC(wmac)                   \
    do {                                                     \
        gpBtCoexWlanInfoDev->WmacPowerUpFlag |= (1 << wmac); \
    } while (0)
#define COEX_CLR_NEEDED_POWER_UP_MAC(wmac)                    \
    do {                                                      \
        gpBtCoexWlanInfoDev->WmacPowerUpFlag &= ~(1 << wmac); \
    } while (0)

// check which wlan is already powered up
#define COEX_CHECK_POWERED_UP_MAC(wmac) (gpBtCoexWlanInfoDev->WmacPowerUpFlag & (1 << (wmac + COEX_POWERUP_SHIFT)))
#define COEX_CHECK_POWERED_UP_ANY_MAC   (gpBtCoexWlanInfoDev->WmacPowerUpFlag & COEX_POWERUP_MASK)
#define COEX_SET_POWERED_UP_MAC(wmac)                                               \
    do {                                                                            \
        gpBtCoexWlanInfoDev->WmacPowerUpFlag |= (1 << (wmac + COEX_POWERUP_SHIFT)); \
    } while (0)
#define COEX_CLR_POWERED_UP_MAC(wmac)                                                \
    do {                                                                             \
        gpBtCoexWlanInfoDev->WmacPowerUpFlag &= ~(1 << (wmac + COEX_POWERUP_SHIFT)); \
    } while (0)

#define NACK_REASON(reason, field) \
    (GET_CONT_NACK_REASON(reason, CXC_NACK_RSN_##field##_MASK, CXC_NACK_RSN_##field##_SHIFT))

#define GET_CONT_NACK_REASON(reason, mask, shift) ((reason & mask) >> shift)

#define CXC_NACK_RSN_JUMPING_OFFSET_APPLIED_MASK 0X00000001
#define CXC_NACK_RSN_PRIORITY_OFFSET_TX_MASK     0X00000002
#define CXC_NACK_RSN_WEIGHT_IDX_MASK             0X000000FC
#define CXC_NACK_RSN_FINAL_RESULT_ALT_BASED_MASK 0X00000100
#define CXC_NACK_RSN_PRIMARY_TX_APPLIED_MASK     0X00000200
#define CXC_NACK_RSN_PRIORITY_OFFSET_BITMAP_MASK 0X00007C00
#define CXC_NACK_RSN_FINAL_ARB_CASE_NUM_MASK     0X000E0000

#define CXC_NACK_RSN_JUMPING_OFFSET_APPLIED_SHIFT 0X00
#define CXC_NACK_RSN_PRIORITY_OFFSET_TX_SHIFT     0X01
#define CXC_NACK_RSN_WEIGHT_IDX_SHIFT             0X02
#define CXC_NACK_RSN_FINAL_RESULT_ALT_BASED_SHIFT 0X08
#define CXC_NACK_RSN_PRIMARY_TX_APPLIED_SHIFT     0X09
#define CXC_NACK_RSN_PRIORITY_OFFSET_BITMAP_SHIFT 0X0A
#define CXC_NACK_RSN_FINAL_ARB_CASE_NUM_SHIFT     0X11

#define COEX_TBTT_TIMER_UPDATE_OFFSET_US 100

#endif /*PLATFORM_FERMION */

#define NUM_GPM_PAYLOAD_BYTES 12
typedef struct tMci_Gpm {
    uint32_t WBTimer;
    uint32_t GPMBody[3];
} tMciGpm;

typedef struct tSched_Msg {
    uint32_t Buf[4];
} tSchedMsg;

#ifdef PLATFORM_FERMION
typedef struct {
    /* MCI HW states */
    uint8_t IsMciHWBusy;
    uint8_t BtMciState;
    uint8_t IsMCIBusPaused;
    uint8_t MCIBasicSyncError;
    uint8_t disable_mci;
    uint8_t bus_recovery_state;

    /* MCI SW States */
    uint8_t IsMciInitComplete;
    uint8_t IsBtOn;

    uint32_t DefaultSmhEnabledInt;
    uint32_t DefaultPmhEnabledInt;
    uint32_t DefaultLmhEnabledInt;
    uint32_t DefaultMciBasicEnabledInt;
    uint32_t SmhEnabledInt;
    uint32_t PmhEnabledInt;
    uint32_t LmhEnabledInt;
    uint32_t LcmhEnabledInt;
    uint32_t MciBasicEnabledInt;

    uint16_t MCIIntfTimeout;
    uint16_t MCITimeout;
    uint16_t MCIInterruptThreshold;
    nt_osal_timer_handle_t IntfTimer;

    uint16_t invalid_gpm_cnt;

    uint8_t CoexWsiSyncState;
#ifndef EMULATION_BUILD
    uint8_t bt_sys_waking_intr;
    uint8_t mci_reset_cnt;
    uint8_t MCIBtRespReceived;
#endif
#ifndef EMULATION_BUILD
    nt_osal_timer_handle_t WsiStateTimer;
    uint8_t isCoexWsiStateTimerRunning;
#endif

} MCI_STRUCT;

typedef struct {
    uint8_t msg_bitmap;
    /* CONT_INFO */
    uint8_t CI_priority;    // BT Request Prio
    uint8_t CI_linkid;      // BT LinkId
    uint8_t CI_channel;     // BT Request Operating Channel
    uint8_t CI_tx;          // Tx or Rx Request from BT
    uint8_t CI_rssi_power;  // BT RSSI
    uint8_t CI_idx;
    uint8_t CI_crx_allow;  // If BT wants CRx

    /* CONT_NACK */
    uint8_t CN_jump_offset_appl;
    uint8_t CN_prio_offset_tx;
    uint8_t CN_weight_idx;  // WL weight that NACK'd BT
    uint8_t CN_fin_res_alt_based;
    uint8_t CN_prim_tx_appl;
    uint8_t CN_prio_offset_bmp;
    uint8_t CN_fin_arb_case_num;

    /* CONT_RST */
    uint16_t CI_CR_dur;  // Duration between CI and CR

    uint16_t CI_bt_clock;
    uint32_t CI_timestamp;
} COEX_CONT_LOG_ENTRY;

typedef struct {
    uint16_t size;
    uint16_t write_idx;
    uint16_t read_idx;
    uint8_t cont_info_expected;  // 1 - CONT_INFO expected; 0 - CONT_RST expected
    uint8_t print_in_progress;
    nt_osal_timer_handle_t cont_log_print_timer;
    COEX_CONT_LOG_ENTRY *cont_log_entry;
} COEX_CONT_LOG_RING;

enum {
    COEX_LOG_INFO_BMP = 0x1,
    COEX_LOG_NACK_BMP = 0x2,
    COEX_LOG_RST_BMP = 0x4,
};

enum {
    MCI_PMH_POWERUP_WLAN_RESULT_SUCCESS = 0,
    MCI_PMH_POWERUP_WLAN_RESULT_MCIB_ERROR,
    MCI_PMH_POWERUP_WLAN_RESULT_ALREADY_ERROR,
    MCI_PMH_POWERUP_WLAN_RESULT_ONGOING = 8,
    MCI_PMH_POWERUP_WLAN_RESULT_GENERAL_ERROR = 9,
};

enum {
    MCI_PMH_POWERDOWN_WLAN_RESULT_SUCCESS = 0,
    MCI_PMH_POWERDOWN_WLAN_RESULT_ONGOING = 8,
    MCI_PMH_POWERDOWN_WLAN_RESULT_ERROR = 9,
};

typedef enum {
    MCI_RECOVERY_STATE_UNWARRANTED = 0,
    MCI_RECOVERY_STATE_REQ_WAKE_SENT = 1,
} E_MCI_RECOVERY_STATE;

typedef enum {
    MCI_STIMULUS_NONE = 0,
    MCI_STIMULUS_INVALIDHDR = 1,
    MCI_STIMULUS_SLEEPUPDATE = 2,
    MCI_STIMULUS_REMOTE_RESET = 3
} E_MCI_STIMULUS;

typedef enum {
    COEX_EVENT_SRC_POWER = 1,
    COEX_EVENT_SRC_BAND_CHANGE = 2,
    COEX_EVENT_SRC_MAC_RESET = 3,
    COEX_EVENT_SRC_MCI_POWERUP = 4,
    COEX_EVENT_SRC_BT = 5,
    COEX_EVENT_SRC_WLAN_MISC = 6,
    COEX_EVENT_SRC_CXC_POWER = 7,

    COEX_EVENT_SRC_COUNT,
} E_COEX_EVENT_SRC;

enum {
    COEX_WSI_SYNC_STATE_POWERUP_FAILURE = 0,
    COEX_WSI_SYNC_STATE_RESYNC_FAILURE,
    COEX_WSI_SYNC_STATE_NORMAL,
};

#ifndef EMULATION_BUILD
typedef enum {
    CXC_RESET,
    CXC_RESET_POST_BUS_RESYNC_FAIL,
} coex_event_type;
#endif

/*-------------------------------------------------------------------------
 * Function Declarations and Documentation
 * ----------------------------------------------------------------------*/

void coex_mci_enable_contention_stats(__attribute__((__unused__)) bool Enable);
void coex_mci_clear_contention_stats();
uint8_t coex_mci_send_msg(uint8_t MciType, uint8_t IsTimeStampDisable, uint32_t *pMsg, uint8_t Len, uint32_t QFlags);
uint8_t coex_mci_get_sleep_state(void);
void coex_mci_update_sleep_state(uint8_t NewBtState);
void coex_pmh_init_powerup_seq_wmac(UFW_MAC_ID_E macId);
uint8_t coex_pmh_check_powerup_wmac(UFW_MAC_ID_E macId);
uint8_t coex_pmh_check_powerdown_wmac(UFW_MAC_ID_E macId);
void coex_pmh_powerdown_init_wmac(UFW_MAC_ID_E macId);
uint8_t coex_mci_powerup(UFW_MAC_ID_E macId);
void coex_mci_powerdown(UFW_MAC_ID_E macId);
void coex_mci_set_weight(uint32_t NumberOfRegWrites);
void _nt_wlan_post_mci_recover_timeout_msg(nt_osal_timer_handle_t thandle);
void coex_mci_set_bus_recovery_state(uint32_t state);
uint32_t coex_mci_get_bus_recovery_state(void);
void coex_mci_recovery(uint8_t StimulusState);
void coex_mci_timer_handler(void);
void coex_mci_change_wlan_mode(E_COEX_EVENT_SRC src);
void coex_configure_concurrent_rx(uint8_t lna_gain, uint8_t bt_low_rssi, uint8_t bt_high_rssi);
void coex_concurrent_rx_enable(uint8_t enable);
void coex_mci_process_gpm_int_dsr(void);
void coex_mci_process_gpm_buff_full_int_dsr(void);
void coex_mci_change_wlan_mode(E_COEX_EVENT_SRC src);
void coex_mci_set_weight(uint32_t NumberOfRegWrites);
void coex_set_cxc_settings(uint8_t op_band);
uint8_t coex_mci_get_global_bt_state(void);
void coex_mci_set_global_bt_state(uint8_t IsBtOn);
void coex_mci_disable_interrupt(void);
void coex_mci_enable_interrupt(void);
void coex_mci_log_cont_msgs(uint32_t bmh_rx);
void coex_mci_print_cont_logs();
void coex_cont_log_en(uint16_t num_buff);
void coex_cont_log_dis();
void coex_cont_log_print_timer_hdlr();
void coex_mci_smh_reset_gpm(void);

#ifdef WAR_COEX_VIFERMION285
void coex_reverse_sw_ctrl_polarity(uint8_t enable);
#endif

#ifndef EMULATION_BUILD
void coex_mci_smh_reset();
uint8_t coex_mci_get_wsi_state(void);
void coex_mci_reset();
void coex_mci_reset_sequence(void);
void coex_mci_reset_sequence_complete(void);
void coex_mci_send_remote_reset(void);
void coex_mci_send_sys_waking(void);
void coex_mci_cxc_module_reset();
void coex_mci_bmh_reset();
void coex_mci_lmh_reset();
void coex_mci_pmh_reset();
void coex_wsi_state_timer_enable(uint8_t enable);
void coex_mci_mcim_reset();
void coex_mci_error_handler(void);
#endif

#endif /*PLATFORM_FERMION */
#endif /* SUPPORT_COEX */
#endif /* #ifndef  COEX_MCI_H */
