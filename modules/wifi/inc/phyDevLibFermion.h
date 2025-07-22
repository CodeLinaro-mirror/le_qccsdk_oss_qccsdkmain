/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _PHY_DEVLIB_API_PARM_PRODUCT_H
#define _PHY_DEVLIB_API_PARM_PRODUCT_H

/*!
  @file phyvDevLibApi.h

  @brief
  PHY devLib API header file

*/

/*! @brief README
 *
 *  devLib/
 *      contains PHY DEV LIB level files
 *      The user s/w must call initDevLib()
 *
 *  devLib/phyDevLibApiParm.h
 *      The definition of input/output parameter for phy dev lib APIs needed by the user s/w.
 *
 */

#include "phyDevLibCommon.h"
#define PHYDEVLIB_IOT 1
#define CHIP_NAME     "FERMION"
#define _CHIP_ID_     0x1000

#define HASTINGS_1P0_MINOR_VERSION 0x00
#define HASTINGS_1P1_MINOR_VERSION 0x10

//#define __SIMXL_SC_TB__

#define regMap(base)    SEQ_WCSS_PHYB_WFAX_##base##_REG_MAP_OFFSET
#define regMapA(base)   SEQ_WCSS_PHYA_WFAX_##base##_REG_MAP_OFFSET
#define regMapA1(base)  SEQ_WCSS_PHYA_WFAX_##base##_REG_MAP_OFFSET
#define regMapB(base)   SEQ_WCSS_PHYB_WFAX_##base##_B_REG_MAP_OFFSET
#define blkOffset(base) SEQ_WFAX_TOP_WFAX_##base##_REG_MAP_OFFSET

#define DPD_CAL_TABLE_MAX 4

#define PHYDEVLIB_PHYID_MAX 1

// This macro must be defined 1 otherwise would cause conflict redefinition error in Q5 build.(wlan_defs.h vs
// whal_api.h)
#define SUPPORT_11AX 1

//#define COMMON_2G_COL 2
//#define MODAL_2G_COL 4
//#define MODAL_2G_RF_RXLUT_COL 3
//#define TX_BBF_2G_COL 5

//#define COMMON_5G_COL 2
//#define MODAL_5G_COL 8
//#define MODAL_5G_RF_RXLUT_COL 4
//#define TX_BBF_5G_COL 9
//#define RX_GLUT_5G_COL 3

#define SKIP_RFABBFLUT_INI_MASK 0x4
#define SKIP_RXGAINTBL_INI_MASK 0x2
#define SKIP_TXGAINTBL_INI_MASK 0x1

#define HW_RFA_BASE_OFFSET(pHandle)         ((pHandle->phyId) ? 0x100000 : 0x0)
#define HW_RFA_BASE_OFFSET_PHYID(phyId)     ((phyId) ? 0x100000 : 0x0)
#define HW_RFA_BASE_OFFSET_PHYBASE(phyBase) (0x0)

#define Use2G_iPA_iLNA 1
typedef enum {
    PHY_CHAIN_0,
    PHY_CHAIN_MAX,
} PHY_CHAIN;

typedef enum {
    PHY_CHAIN_0_MASK = 0x1,
    PHY_CHAIN_MASK_MAX = 0x1,
} PHY_CHAIN_MASK;

#define RXDCO_MAX_CHAIN PHY_CHAIN_MAX

#ifndef WHAL_NUM_PHY
#define WHAL_NUM_PHY 1
#endif

#define NUM_BO_QAM 7

#ifndef PHYDEVLIB_PRODUCT_VERSION
#define PHYDEVLIB_PRODUCT_VERSION 10
#endif

#if defined(PHYDEVLIB_IMAGE_MM_FTM)
#define WLAN_CNSS_BASE 0
#if defined(EMULATION_BUILD)
#define WCSS_ENV_DEFAULT WCSS_ENV_EMU
#else
#define WCSS_ENV_DEFAULT WCSS_ENV_SILICON
#endif
#elif defined(PHYDEVLIB_IMAGE_UFW)
#if defined(EMULATION_BUILD)
#define WCSS_ENV_DEFAULT WCSS_ENV_EMU
#else
#define WCSS_ENV_DEFAULT WCSS_ENV_SILICON
#endif
#define WLAN_CNSS_BASE 0
#else
#define WCSS_ENV_DEFAULT WCSS_ENV_EMU
#define WLAN_CNSS_BASE   0
//#define PHYDEVLIB_SUPPORT_DCM
#endif

#undef MAX_NUM_CHAINS
#define MAX_NUM_CHAINS 1

#if defined(CONFIG_Q5EXT)
#define CONFIG_PHYDEVLIB_API_Q5EXT
#endif

#define __HALPHY_RESIDENT__ /* no-op*/
#define NUM_SPATIAL_STREAM  1
#define NUM_SCHED_ENTRIES   2
#if !defined(PHYDEVLIB_IMAGE_MM_FTM)
/*
 * Build configuration
 */
//#if defined(PHYDEVLIB_SUPPORT_BDF)
//#define _BDF_BUILD
//#endif

#ifndef WLAN_NPR
#define WLAN_NPR
#endif

////////////////////////////////////////////////////////////////////////////////
// From fwconfig_target_common.h

#ifdef SUPPORT_VHT
#undef SUPPORT_VHT
#endif

////////////////////////////////////////////////////////////////////////////////
// From fwconfig_QCA6490.h

//#define CONFIG_160MHZ_SUPPORT  1
#define NUM_SPATIAL_STREAM 1
//#define MAX_SPATIAL_STREAMS_SUPPORTED_AT_160MHZ 1
#define MAX_HT_SPATIAL_STREAM 1
//#define MAX_BF_SPATIAL_STREAM 2
//#define VHT_EXTRA_MCS_SUPPORT 1
#define NUM_SCHED_ENTRIES 2
#define RXG_CAL_CHAIN_MAX 1  // TODO halphy should use based num chains?
// HalPhy Memory Categorization
#define __HALPHY_RESIDENT__ /* no-op*/

////////////////////////////////////////////////////////////////////////////////
// From halphyConfig_qca649x.h

#define WHAL_MAX_RATE_PWR_MULTIPLIER 2
#endif

#endif /* _PHY_DEVLIB_API_PARM_PRODUCT_H */
