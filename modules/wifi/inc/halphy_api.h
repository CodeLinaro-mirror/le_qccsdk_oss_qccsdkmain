/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _HALPHY_API_H_
#define _HALPHY_API_H_

#include <stdint.h>
#include <stdbool.h>

#include "wlan_dev.h"
#include "hal_int_phy.h"
#include "halphy_hdl_api.h"

//#include "autoconf.h"

#define INVALID_RF_CHANNEL          0
#define PS_CALLBACK_HALPHY_PRIORITY 11

/* SNR = RSSI - NF */
#define HALPHY_NF_DEFAULT            (-100)                      /* default value of NF in dBm */
#define HALPHY_SNR_TO_RSSI_DBM(snr)  ((snr) + HALPHY_NF_DEFAULT) /* snr to rssi in dbm */
#define HALPHY_RSSI_DBM_TO_SNR(rssi) ((rssi)-HALPHY_NF_DEFAULT)  /* rssi in dbm to snr */

typedef enum { HALPHY_STATUS_FAILURE, HALPHY_STATUS_SUCCESS } halphy_status_t;

typedef enum halphy_cal_profile_s {
    FULL_CHAN_SWITCH = 0,       // Full Channel calibrations
    SCAN_CHAN_SWITCH = 1,       // Scan Channel Calibrations
    TEMP_TRIGEER_CAL = 2,       // Temperature Triggered Cal
    POWER_SAVE_WAKE_UP = 3,     // Wake-up Calibrations
    POWER_DOWN_DTIM = 4,        // Power save Calibrations
    COLD_BOOT_CAL = 5,          // cold boot calibration profile
    TWT_CHAN_SWITCH = 6,        // TWT Channel Switch
    NUM_HALHY_CAL_PROFILE = 7,  //
    UNKNOWN_PROFILE = 8,
} halphy_cal_profile_t;

typedef enum whal_reset_flags_s {
    WHAL_FORCED_RESET = 0,
    WHAL_RECOVERY_RESET = 1,
    UNKNOWN = 2,
} whal_reset_flags_t;
#ifdef TEMP_BASED_RECAL_SUPPORT
typedef enum halphy_recal_region_s {
    RECAL_REGION_LOW = 0,
    RECAL_REGION_MID = 1,
    RECAL_REGION_HIGH = 2,
    RECAL_REGION_INVALID = 3,
} halphy_recal_region_t;
#endif /* TEMP_BASED_RECAL_SUPPORT */

typedef struct halphy_handle_s {
    channel_t cur_chan;
#ifdef PLATFORM_FERMION
    PHY_BAND cur_band;
    uint8_t crx_enable; /* 0 disable 1 enable */
    uint8_t glut_sel;   /* 0 to 5 */
#endif                  /* PLATFORM_FERMION */
    halphy_cal_profile_t profile;
    bool phy_initialized;
    int16_t set_tx_power;  // (x 0.5 dB)
#ifdef TEMP_BASED_RECAL_SUPPORT
    bool temp_based_recal;
    uint8_t curr_recal_temp_region;
    uint16_t curr_recal_channel;
    uint8_t prev_recal_temp_region;
    uint16_t prev_recal_channel;
#endif /* TEMP_BASED_RECAL_SUPPORT */
    bool bandedge_enable;
} halphy_handle_t;

#ifdef PHY_MAC_RX_HW_COUNTER_LOGGING
typedef struct halphy_rx_hw_count_s {
    uint32_t phy_rx_pkt; /* number of packets for which phy has seen a successful signal field and service field in the
                            packet it is receiving and is now moving forward with decoding the payload */
    uint32_t rxPktCount; /* MAC HW reg DPU_DPU_RXPKTCOUNT | 0x2081804 */
    uint32_t dma_send;   /* MAC HW reg RXP_DMA_SEND_CNT | 0x2080880 */
    uint32_t fcs_err;    /* MAC HW RXP_FCS_ERR_CNT | 0x208087C */
} halphy_rx_hw_count_t;
#endif /* PHY_MAC_RX_HW_COUNTER_LOGGING */
/**
 * @brief Allocates memory and initialize Phy Memory
 * @param  None
 * @return eNT_OK  on success
 */
nt_status_t halphy_open(void);

/**
 * @brief Frees physical layer memory
 * @param  None
 * @return eNT_OK  on success
 */
nt_status_t halphy_close(void);

/**
 * @brief After other chip initializations, initializes the physical layer to ready-to-run state
 * @return eNT_OK  on success
 */
nt_status_t halphy_start(void);

/**
 * @brief Resets the physical layer globals
 * @return eNT_OK  on success
 */
nt_status_t halphy_stop(void);

/**
 * @brief Set Channel related Params and run Calibrations
 * @param  chan Channel Info
 * @param  profile Calibration Profile
 * @return eNT_OK  on success
 */
nt_status_t halphy_set_channel(channel_t chan, halphy_cal_profile_t profile);

/**
 * @brief Set Channel related Params and run Calibrations during Power Save
 * @param  chan Channel Info
 * @param  wakeup 0: Entering Sleep 1: Waking up from power save
 * @return None
 * Note: This can be deprecated we can plan to use set channel API.
 */
void halphy_reset_retention_set_channel(channel_t chan, uint8_t wakeup);

/**
 * @brief  Set Channel related Params and run Calibrations first time resetting all the BB and RF registers
 * @param  chan Channel Info
 * @param  resetFlags Reset/Restore
 * @return None
 */
void halphy_reset(whal_reset_flags_t resetFlags, channel_t chan);

/**
 * @brief  TxPower for rate, Initial return fixed power
 * @param  phyRate phy rate
 * @param  chan Channel Info
 * @return Tx power (Quarter DB steps)
 */
int16_t halphy_get_tx_pwr_for_rate(hal_phy_rates_t phyRate, uint16_t rfChannel);

#ifdef FERMION_ANI_SW_SUPPORT
/**
 * @brief Returns the current channel's cal profile for ANI to determine if scan is going on
 * @param  None
 * @return phy_handle.profile of current channel
 */
halphy_cal_profile_t halphy_ani_get_chan_profile(void);
#endif /* #ifdef FERMION_ANI_SW_SUPPORT */

halphy_handle_t *halphy_get_phyhandle(void);
#ifdef PLATFORM_FERMION
int8_t halphy_rssi_correction(int8_t input_rssi);
bool halphy_get_hw_crx_mode(void);

#ifdef PHY_MAC_RX_HW_COUNTER_LOGGING
void halphy_init_rx_hw_count(void);
void halphy_deinit_rx_hw_count(void);
void halphy_reset_rx_hw_count(void);
void halphy_log_rx_hw_count(halphy_rx_hw_count_t *rx_hw_count);
void halphy_logging_control(bool value);
#endif /* PHY_MAC_RX_HW_COUNTER_LOGGING */

#endif /* PLATFORM_FERMION */

#ifdef PLATFORM_NT
/**
 * @brief  Set external PA
 * @param  enable or disable xpa
 * @return None
 */
void halphy_enable_xpa(bool enable_xpa);
#endif /* PLATFORM_NT */

//#if CONFIG_BOARD_QCC730_XPA_AUTO_CTRL_ENABLE
/**
 * @brief  configure external FEM, bootstrap configuration
 * @param  None
 * @return None
 */
void halphy_xfem_config();

/**
 * @brief  enable/disable external FEM
 * @param  enable:		true enable,false disable
 * @param  band:	 	PHY_2G 2.4G band, PHY_5G 5G band
 * @return None
 */
void halphy_xfem_enable(bool enable, uint8_t band);

/**
 * @brief  initialize external FEM
 * @param  None
 * @return None
 */
void halphy_xfem_init();

/**
 * @brief  de-initialize external FEM
 * @param  None
 * @return None
 */
void halphy_xfem_deinit();

/**
 * @brief  Set XPA configure
 * @param  xpa_cfg_param: 0 No xPA; 1 xPA present
 * @return None
 */
void halphy_set_xpa_cfg(uint8_t xpa_cfg_param);
//#endif
#endif /* _HALPHY_API_H_ */
