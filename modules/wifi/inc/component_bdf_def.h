 /*
Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/************************************************************************/
/* Header file for BDF module -chip QCP5321                             */
/************************************************************************/
// 0.0.8
#ifndef _COMPONENT_BDF_DEF_H_
#define _COMPONENT_BDF_DEF_H_
#include <stdint.h>
/***************** BDF Defines *********************************/

//**************************************************************
// [VERSION] control                                           *
//**************************************************************

// Custom Macros
#define BDF_VER	0
#define BDF_MINOR_VER	1
#define MAX_NUM_MAC_ADDRESS	3
#define MAC_ADDRESS_SIZE	6
#define HALPHY_NUM_2G_PHY	1
#define HALPHY_NUM_5G_PHY	1
#define HALPHY_SPUR_CHANS_2G	8
#define HALPHY_SPUR_CHANS_5G	16
#define HALPHY_SPUR_CHANS_6G	16
#define HALPHY_SPUR_CHANS_5G6G	32
#define MAX_RXG_CAL_2G_CHANS	4
#define MAX_RXG_CAL_5G_CHANS	16
#define MAX_RXG_CAL_6G_CHANS	8
#define MAX_RXG_CAL_5G6G_CHANS	24
#define TPC_DATA_2G_FREQ	4
#define TPC_DATA_5G_FREQ	16
#define TPC_DATA_6G_FREQ	8
#define TPC_DATA_5G6G_FREQ	24
#define CLPC_NUM_CAL_POINTS	16
#define WHAL_NUM_POINTS_TO_MEAS	16
#define WHAL_NUM_CAL_POINTS_FULL_CHAN	16
#define TX0_POWER_DETECTOR_COUNT	256
#define TX0_DIGITAL_GAIN_LUT_COUNT	256
#define BASE_BDF_HEADER_REGDMN_COUNT	2
#define MAX_CTL_SPARE_2G	116
#define MAX_CTL_SPARE_5G	554
#define HALPHY_CONFIG_ENTRIES	240
#define CHLIST_CBC_NUM	25
#define MAX_FUTURE_CBC_CHANNEL	5
#define MAX_XTAL_TEMP_COMP	5
#define TEMPERATURE_REGIONS	3
#define BASE_BDF_HEADER_FUTURE	133
#define TARGET_POWER_2G_FUTURE	174	
#define TARGET_POWER_5G_FUTURE	122	
#define TARGET_POWER_6G_FUTURE	122	
#define RX_GAIN_FUTURE	56
#define XTAL_CAL_FUTURE	76
#define FW_CONFIG_FUTURE	49	
#define TPC_DATA_OFFSET_FUTURE	10455	
#define MAX_SPUR_MIT_FUTURE	48
#define CTL_6G_FUTURE	440
#define AGC_ED_DET_FUTURE	34
#define TEMPERATURE_CONFIG_FUTURE	169	
#ifdef CONFIG_6G_BAND
#define MAX_RESERVED_FUTURE	5454	
#else
#error "CONFIG_6G_BAND need to be recalculated"
#define MAX_RESERVED_FUTURE	576
#endif
#define HALPHY_NUM_REG_DMNS	6
#define HALPHY_NUM_CTLS_2G_11B	(1*HALPHY_NUM_REG_DMNS)
#define HALPHY_NUM_CTLS_2G_11G	(1*HALPHY_NUM_REG_DMNS)
#define HALPHY_NUM_CTLS_2G_HT20	(1*HALPHY_NUM_REG_DMNS)
#define HALPHY_NUM_CTLS_5G_11B	(1*HALPHY_NUM_REG_DMNS)
#define HALPHY_NUM_CTLS_5G_11A	(1*HALPHY_NUM_REG_DMNS)
#define HALPHY_NUM_CTLS_5G_HT20	(1*HALPHY_NUM_REG_DMNS)
#define HALPHY_NUM_CTLS_6G_11B	(1*HALPHY_NUM_REG_DMNS)
#define HALPHY_NUM_CTLS_6G_11A	(1*HALPHY_NUM_REG_DMNS)
#define HALPHY_NUM_CTLS_6G_HT20	(1*HALPHY_NUM_REG_DMNS)
#define HALPHY_NUM_BANDS	3
#define HALPHY_NUM_RATES_FOR_TEMP_BASED_TPC_ADJUSTMENT	3
#define HALPHY_MAX_REGS_TEMP_BASED_UPDATE	15	
#define HALPHY_NUM_PROCESS_CORNERS	3	
#define TXPOWERMODE_NUM_5G_SUB_BANDS	3	
#define TXPOWERMODE_NUM_6G_SUB_BANDS	3	
#define TXPOWERMODE_NUM_2G_SUB_BANDS	2	
#endif // _COMPONENT_BDF_DEF_H_
