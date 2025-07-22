/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#if !defined(_PHY_DEV_INIT_H)
#define _PHY_DEV_INIT_H

#include "phyDevLib.h"
/*!
  @file phy_dev_init.h

  @brief
  PHY dev Lib INIT header
*/

/*! @brief phyReset.c
 */
RECIPE_RC phyReset_qcp5321(void *phy_input, void *reset_input, void *reset_output);
RECIPE_RC phyWarmReset_qcp5321(void *phy_input, void *reset_input, void *reset_output);
RECIPE_RC phyChannelSwitch_qcp5321(void *phy_input, void *channel_switch_input, void *channel_switch_output);

/*! @brief phy_dev_init.c
 */
RECIPE_RC phySetParam_qcp5321(void *phy_input, uint32_t id, void *data);
RECIPE_RC phyGetParam_qcp5321(void *phy_input, uint32_t id, void *data);

/*! @brief phyvi_ftpgTx.c:
 */
RECIPE_RC phyFTPGTx_qcp5321(void *phy_input, void *ftpg_tx_input, void *ftpg_tx_output);

/*! @brief phyvi_ftpgRx.c:
 */
RECIPE_RC phyFTPGRx_qcp5321(void *phy_input, void *ftpg_rx_input, void *ftpg_rx_output);

/*! @brief phy_dev_TPCcal.c:
 */
RECIPE_RC phyForcedGainMode_qcp5321(void *phy_Input, void *forceGainModeInput, void *forceGainModeOutput);

/*! @brief phy_dev_RxDCOCal.c:
 */
RECIPE_RC phyRxDCOCal_qcp5321(void *phy_input, void *rxdco_cal_input, void *rxdco_cal_output);

/*! @brief phy_dev_DPDCal.c:
 */
RECIPE_RC phyDPDCal_qcp5321(void *phy_input, void *dpd_cal_input, void *dpd_cal_output);
#if defined(PHYDEVLIB_IMAGE_STANDALONE)
/*! @brief phySpurMitigation.c:
 */
RECIPE_RC phySpurMitigation_qcp5321(void *phy_input, void *spur_mitigation_input, void *spur_mitigation_output);
#endif
#endif  /// #if !defined(_PHY_DEV_INIT_H)
