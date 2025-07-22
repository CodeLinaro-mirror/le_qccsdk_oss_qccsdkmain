/*========================================================================
Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear

 * @file halphy_hdl_api.h
 * @brief APIs to PHYRF service layer to access HWDevLib functions
 * ======================================================================*/

#ifndef _HALPHY_HDL_API_H_
#define _HALPHY_HDL_API_H_

/*------------------------------------------------------------------------
 * Include Files
 * ----------------------------------------------------------------------*/

#include <stdint.h>
#include <stdbool.h>

#include "HALhwio.h"
#include "fermion_reg.h"

#include "phyDevLib.h"
#include "phy_dev_init.h"
#include "phyReset.h"
#include "phyRegIni.h"
#include "phyCalUtils.h"
#include "phy_dev_RxDCO_Cal.h"
#include "phy_dev_RxIQ_Cal.h"
#include "phy_dev_TxIQTxLO_Cal.h"
#include "phy_dev_DPD_Cal.h"
#include "phy_dev_CombCal.h"
#include "phy_dev_TPCcal.h"
#include "halphy_dbgmem.h"
#include "halphy_phydbg.h"
#include "halphy_phydbg_pktgen.h"
#include "halphy_phydbg_capture.h"
#include "halphy_rx.h"

/*-------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * ----------------------------------------------------------------------*/

#define PHYRF_GET_BAND(freq) (((freq) > 5950) ? PHY_6G : (((freq) > 4900) ? PHY_5G : PHY_2G))

/*------------------------------------------------------------------------
 * Type Declarations
 * ----------------------------------------------------------------------*/

typedef struct phyrf_tx_start_input_s {
    mpi_rate_t rate;
    mpi_guard_interval_t short_guard;
    uint16_t n_packets;
    uint16_t packet_size;
    uint8_t tx_demanded_power;
    uint32_t inter_frame_space;
    uint32_t rf_warmup;
} phyrf_tx_start_input_t;

typedef struct phyrf_tx_stop_output_s {
    uint16_t packets_sent;
} phyrf_tx_stop_output_t;

typedef struct crx_param_s {
    uint8_t crx_enable; /* 0 disable 1 enable */
    uint8_t lnaPwrMode; /* 0:LP, 1:HP */
    uint8_t glut_sel;   /* 0 to 5 */
    uint16_t bt_rssi_lower_threshold;
    uint16_t bt_rssi_upper_threshold;
} crx_param_t;

/*------------------------------------------------------------------------
 * Function Declarations and Documentation
 * ----------------------------------------------------------------------*/

bool phyrf_docal_rxdco(uint8_t bandCode);
bool phyrf_docal_txlo(uint8_t bandCode, tx_power_mode_t tx_power_mode);
bool phyrf_docal_txiq(uint8_t bandCode, tx_power_mode_t tx_power_mode);
bool phyrf_docal_rxiq(uint8_t bandCode, tx_power_mode_t tx_power_mode);
bool phyrf_docal_dpd(uint8_t bandCode, tx_power_mode_t tx_power_mode);

void phyrf_tx_start(phyrf_tx_start_input_t *phyrf_tx_start_input);
void phyrf_tx_stop(phyrf_tx_stop_output_t *phyrf_tx_stop_output);

void phyrf_rx_gain_ctrl(uint8_t xlnactrl, uint8_t gainctrlmode, uint8_t band, uint8_t gain_idx);
void phyrf_dac_playback(bool enable, uint8_t tone);

void phyrf_reset(uint16_t freq);
void phyrf_channel_switch(uint16_t freq, uint8_t profile);
void phyrf_forced_gain(uint8_t band, uint8_t gain_idx, int8_t dig_gain);
void phyrf_tpc_set_finegain_offset(int8_t finegain_offset_ofdm, int8_t finegain_offset_11b, bool isTpcCal);
void phyrf_set_ed_threshold(uint8_t threshold);
void phyrf_crx_enable(uint16_t freq, crx_param_t *crx_param);

#if (FERMION_CHIP_VERSION == 2)
void phyrf_tpc_set_finegain_offset_11b(int8_t finegain_offset, uint8_t rate);
void phyrf_tpc_set_finegain_offset_11a(int8_t finegain_offset, uint8_t rate);
void phyrf_tpc_set_finegain_offset_11n(int8_t finegain_offset, uint8_t rate);
#endif
void phyrf_enable_ceb(bool enable);
void phyrf_set_bandedge_correction(bool enable);
#endif /* SRRC_BAND_EDGE_SUPPORT */
