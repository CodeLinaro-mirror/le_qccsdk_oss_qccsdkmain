/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _PHY_DEVLIB_API_DEF_H
#define _PHY_DEVLIB_API_DEF_H

/*!
  @file phyvDevLibApiDef.h

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

#ifndef RCEIPE_RC_EUM
#define RCEIPE_RC_EUM
typedef enum {
    RECIPE_NULL = 255,
    RECIPE_FAILURE = 0,
    RECIPE_SUCCESS = 1,
    RECIPE_LOOPEND_FAILURE = 2,
    RECIPE_LOOPEND_SUCCESS = 3,
    RECIPE_COUNT_EXCEEDED = 4,

    RECIPE_RC_LAST = RECIPE_COUNT_EXCEEDED,
    RECIPE_RC_MAX = (RECIPE_RC_LAST + 1),
} RECIPE_RC;
#else

#endif  ///#if !defined(RECIPE_RC)

#undef PHYID_ENUM
#if !defined(PER_PRODUCT_SETTINGS)
typedef enum PHYDEVLIB_PHYID_ENUM {
    /* PHYDEVLIB_PHYID_MAX moved to product-dependent header files */
    PHYDEVLIB_PHYID_A = 0,
    PHYDEVLIB_PHYID_A0 = 0,
    PHYDEVLIB_PHYID_B,
    PHYDEVLIB_PHYID_A1,
} PHYDEVLIB_PHYID_ENUM;
#endif

typedef enum {
    PHYA_ONLY_MODE = 1,  // this is for historical reason, concMode in reset Params is 1 by default.
    PHYB_ONLY_MODE = 0,
    DBS_CONC_MODE = 2,
    SBS_CONC_MODE = 3,
    CONC_MODE_LAST,
    CONC_MODE_MAX = CONC_MODE_LAST,
} PHY_CONC_MODE_EMU;

typedef enum {
    PHY_5G = 0,
    PHY_2G = 1,
    PHY_6G = 2,
    PHY_BAND_LAST = PHY_6G,
    PHY_BAND_MAX = (PHY_BAND_LAST + 1),
} PHY_BAND;
#define PHY_2G_1 PHY_2G
#define PHY_5G_0 PHY_5G

typedef enum {
    PHY_BW_IDX_20MHz = 0,
    PHY_BW_IDX_40MHz = 1,
    PHY_BW_IDX_80MHz = 2,
    PHY_BW_IDX_80Plus80 = 3,
    PHY_BW_IDX_160MHz = 4,
    PHY_BW_IDX_10MHz = 5,
    PHY_BW_IDX_5MHz = 6,
    PHY_BW_IDX_165MHz = 7,
    PHY_BW_IDX_240MHz = 8,
    PHY_BW_IDX_320MHz = 9,

    PHY_BW_IDX_LAST = PHY_BW_IDX_320MHz,
    PHY_BW_IDX_MAX = (PHY_BW_IDX_LAST + 1),
} PHY_BW_IDX;

typedef enum {
    PHY_MODE_FULL_RATE = 0x00,
    PHY_MODE_HALF_RATE = 0x01,
    PHY_MODE_QUARTER_RATE = 0x02,
    PHY_MODE_FC_LEFT_SHFT_2P5_MHZ = 0x04,
    PHY_MODE_FC_CENTER = 0x08,
    PHY_MODE_FC_RIGHT_SHFT_2P5_MHZ = 0x10,
    PHY_MODE_MASK = 0xFF,
} PHY_MODE;

typedef enum {
    PHYNFCAL_FORCE_FIXED_VALUES = 0x00000001,
    PHYNFCAL_NO_HARDWARE_WRITE = 0x00000002,
    PHYNFCAL_SHORT_WAIT = 0x00000004,
    PHYNFCAL_NO_WAIT = 0x00000008,
    PHYNFCAL_PRI80_READ = 0x00000010,
    PHYNFCAL_SEC80_READ = 0x00000020,
    PHYNFCAL_MINCCA_READ = 0x00000040,
    PHYNFCAL_DO_PHYON_PHYOFF = 0x00000080,
    PHYNFCAL_NF_WRITE = 0x00000100,
    PHYNFCAL_BYPASSXLNA_MODE = 0x00000200,
    PHYNFCAL_AVGCCAOUT_READ = 0x00000400,
    PHYNFCAL_ENABLE_CONTINUOUS_CAL = 0x00000800,
    PHYNFCAL_DISABLE_CONTINUOUS_CAL = 0x00001000,
} PHYNFCAL_FLAG;

typedef enum {
    PHYPAL_CMD_RUN_PCAL = 0x00,       // run PCAL but no BIST
    PHYPAL_CMD_RUN_PCAL_BIST = 0x01,  // run PCAL and BIST
    PHYPAL_CMD_LOAD_RO_DLY = 0x02,    // run manualPCAL but use provided RoDelay value, no BIST
} PHYPAL_CMD;

typedef enum {
    PHYPAL_PCAL_AUTO = 0x00,
    PHYPAL_PCAL_MANUAL = 0x01,
} PHYPAL_PCAL_METHOD;

typedef enum {
    PHYPAL_REG_CMD_SAVE = 0x00,
    PHYPAL_REG_CMD_RESTORE = 0x01,
} PHYPAL_REG_CMD;

typedef enum {
    PHYPAL_160MODE_STATE_80 = 0x00,
    PHYPAL_160MODE_STATE_160 = 0x01,
} PHYPAL_160MODE_STATE;

typedef struct PHY_COMPLEX {
    int32_t real;
    int32_t imag;
} PHY_COMPLEX;

typedef struct IQ_GAIN_PAIR {
    uint32_t txGainIdx;
    uint32_t txDacGain;
    uint32_t rxGainIdx;
} IQ_GAIN_PAIR;

typedef enum {
    PHYDEVLIB_DEBUGMON_CMD_STOP_BEFORE_PDL_BODY = 1,
    PHYDEVLIB_DEBUGMON_CMD_STOP_AFTER_PDL_BODY = 2,
} PHYDEVLIB_DEBUGMON_STOP_CMD_ENUM;

typedef enum {
    RETENTION_IDX_HOME = 0x01,
    RETENTION_IDX_SCAN = 0x02,
    RETENTION_IDX_PRI20 = 0x04,
    RETENTION_IDX_S2W = 0x08,
    RETENTION_IDX_eMLSR_PRE_SWITCH = 0x10,
    RETENTION_IDX_eMLSR_SWITCH_BACK = 0x20,
} RETENTION_INDEX;

typedef enum {
    RETENTION_TBL_HOME_PHY = 0x00,
    RETENTION_TBL_HOME_RFA = 0x01,
    RETENTION_TBL_SCAN_PHY = 0x02,
    RETENTION_TBL_SCAN_RFA = 0x03,
    RETENTION_TBL_PRI20_PHY = 0x04,
    RETENTION_TBL_PRI20_RFA = 0x05,
    RETENTION_TBL_S2W_PHY = 0x06,
    RETENTION_TBL_S2W_RFA = 0x07,
    RETENTION_TBL_eMLSR_PHY_PRE_SWITCH = 0x8,
    RETENTION_TBL_eMLSR_PHY_SWITCH_BACK = 0x9,
} RETENTION_TABLE_INDEX;

typedef enum {
    RETENTION_GROUP_INI = 0x01,
    RETENTION_GROUP_TPC = 0x02,
    RETENTION_GROUP_CAL = 0x04,
    RETENTION_GROUP_DPD = 0x08,

    RETENTION_GROUP_TXBBF = 0x11,  // special for TX/RX BBF check;
    RETENTION_GROUP_RXBBF = 0x21,  // special for TX/RX BBF check;
    RETENTION_GROUP_eMLSR = 0x30,
} RETENTION_GROUP_MASK;

typedef enum {
    PCSS_SWLOG_LEVEL_MONITOR = 0x0,
    PCSS_SWLOG_LEVEL_DEBUG = 0x1,
    PCSS_SWLOG_LEVEL_VERBOSITY = 0x2,
} PCSS_SWLOG_LEVEL_E;

typedef RECIPE_RC (*DpdTrnPktTx_fp)(void *phy_input, void *dpd_input);

extern int32_t my_round(int32_t in);

/* External HWIO registration support */
typedef void (*phyHwioWrite8)(uint32_t addr, uint8_t data);
typedef uint8_t (*phyHwioRead8)(uint32_t addr);
typedef void (*phyHwioWrite16)(uint32_t addr, uint16_t data);
typedef uint16_t (*phyHwioRead16)(uint32_t addr);
typedef void (*phyHwioWrite32)(uint32_t addr, uint32_t data);
typedef uint32_t (*phyHwioRead32)(uint32_t addr);
typedef void (*phyHwioWrite64)(uint32_t addr, uint64_t data);
typedef uint64_t (*phyHwioRead64)(uint32_t addr);
typedef void (*phyMemWrite)(uint32_t addr, uint8_t *buf, uint32_t size);
typedef void (*phyMemRead)(uint32_t addr, uint8_t *buf, uint32_t size);
typedef void (*phyHwioScatterWrite)(uint32_t *addr, uint32_t *data, uint32_t size);
typedef void (*phyHwioScatterRead)(uint32_t *addr, uint32_t *data, uint32_t size);

#define HW_PHY_BASE(pHandle)    (pHandle->phyBase)
#define HW_PHY_BASEEXT(pHandle) (pHandle->phyBaseExt)

#define LOMASTER 1
#define LOSLAVE  0

#define LOAD_INI_ONETIME_MASK 0x8
#define LOAD_INI_COMMON_MASK  0x4
#define LOAD_INI_BIMODAL_MASK 0x2
#define LOAD_INI_MODAL_MASK   0x1

#define SKIP_PHY_INI_MASK 0x2
#define SKIP_RFA_INI_MASK 0x1

// calMask 32 bits
#define CAL_RXDCO          0x1
#define CAL_COMBIQ         0x2
#define CAL_DPD            0x4
#define CAL_TPC            0x8
#define CAL_TIADC          0x10
#define CAL_PKDET          0x20
#define CAL_RXBBF          0x40
#define CAL_TXBBF          0x80
#define CAL_PAL            0x100
#define CAL_NF             0x200
#define CAL_SPURMITIGATION 0x400
#define CAL_PDC            0x800
#define CAL_DAC            0x1000
#define CAL_IM2            0x2000
#define CAL_RST            0x4000
#define CAL_ADC            0x8000
#define CAL_RXSPUR         0x10000

#define DFS_ENABLE_DET0 0x1
#define DFS_ENABLE_DET1 0x2
#define DFS_ENABLE_DET2 0x4

#define RFA_SYNTH_CFG_MASK_WIFIPKT 0x1
#define RFA_SYNTH_CFG_MASK_ADFS    0x2

#define SS_ENABLE_DET0 0x1
#define SS_ENABLE_DET1 0x2
#define SS_ENABLE_DET2 0x4

#define SS_FFT_PACK_MODE_DET0_MASK 0x0F
#define SS_FFT_PACK_MODE_DET0_SHFT 0x00
#define SS_FFT_PACK_MODE_DET1_MASK 0xF0
#define SS_FFT_PACK_MODE_DET1_SHFT 0x04

#define CHIP_RFA_FN_IRON_2G    5
#define CHIP_RFA_FN_IRON_5G    6
#define CHIP_RFA_FN_HK_DRM     3
#define CHIP_RFA_FN_HK_DRM_OLD 0

#define BOOT_AS_MASTER 0x1
#define BOOT_AS_SLAVE  0x0
#define BOOT_NULL      0x2

#define PWR_MASK_HP   0x01
#define PWR_MASK_NP   0x02
#define PWR_MASK_LP   0x04
#define PWR_MASK_DTIM 0x08
#define PWR_MASK      0x0F

#define PWR_SHFT_HP   0x0
#define PWR_SHFT_NP   0x1
#define PWR_SHFT_LP   0x2
#define PWR_SHFT_DTIM 0x3

#define RESET_TYPE_NORMAL   0
#define RESET_TYPE_BMPS_W2S 1
#define RESET_TYPE_BMPS_S2W 2
#define RESET_TYPE_IMPS_W2S 3
#define RESET_TYPE_IMPS_S2W 4

#define REG_DUMP_TYPE_ANI   0x0001
#define REG_DUMP_TYPE_DPD   0x0002
#define REG_DUMP_TYPE_NF    0x0004
#define REG_DUMP_TYPE_PKDET 0x0008
#define REG_DUMP_TYPE_RADAR 0x0010
#define REG_DUMP_TYPE_RXDCO 0x0020
#define REG_DUMP_TYPE_RXIQ  0x0040
#define REG_DUMP_TYPE_RXTD  0x0080
#define REG_DUMP_TYPE_SS    0x0100
#define REG_DUMP_TYPE_TXIQ  0x0200
#define REG_DUMP_TYPE_TPC   0x0400

#define EVENT_MASK_TYPE_DMAC2PHYDBG    0x0001
#define EVENT_MASK_TYPE_MPIMODULE      0x0002
#define EVENT_MASK_TYPE_NOCPHYDBG      0x0004
#define EVENT_MASK_TYPE_PMIMODULE      0x0008
#define EVENT_MASK_TYPE_ROBEMODULE     0x0010
#define EVENT_MASK_TYPE_RXTDMODULE     0x0020
#define EVENT_MASK_TYPE_WSIMODULE      0x0040
#define EVENT_MASK_TYPE_TPCMODULE      0x0080
#define EVENT_MASK_TYPE_FFTMODULE      0x0100
#define EVENT_MASK_TYPE_TXTDMODULE     0x0200
#define EVENT_MASK_TYPE_TXFDMODULE     0x0400
#define EVENT_MASK_TYPE_PCSSMODULE     0x0800
#define EVENT_MASK_TYPE_DEMFRONTMODULE 0x1000

#define CMD_CONFIGURE 0x01
#define CMD_START     0x02
#define CMD_STOP      0x04

#define STATS_TXIQ_OFF                     0x1
#define STATS_RXIQ_OFF                     0x2
#define STATS_TXLPC_OFF                    0x3
#define STATS_RXLPC_OFF                    0x4
#define STATS_TXTD_POST_DPD_PEF_ON         0x5
#define STATS_TX_DAC_UPSAMPLING_FILTER_OFF 0x6
#define STATS_RX_TI_ADC_ON                 0x7
#define STATS_RX_VSRC_FILTER_OFF           0x8
#define STATS_RX_SPUR_MIT_ON               0x9

#define VREG_RX_DISABLE            0x01
#define VREG_POWER_SURGE_DROP      0x02
#define VREG_COEX_CONTROL          0x04
#define VREG_DETECTOR_CONFIG_RADAR 0x08
#define VREG_STA_MODE              0x10
#define VREG_PHY_MODE              0x20
#define VREG_LOW_POWER_MODE        0x40

#define CXM_COUNT_CONTROL_VALUE 0x1
#define CXM_COUNT_COLLISION     0x2

typedef enum phydevlib_param_id {
    PHYDEVLIB_PARAM_INVALID = 0,
    PHYDEVLIB_PARAM_WLAN_BASE,
    PHYDEVLIB_PARAM_PHYM3_BINARY,
    PHYDEVLIB_PARAM_WCSS_ENV,
    PHYDEVLIB_PARAM_WCSS_VERSION,
    PHYDEVLIB_PARAM_REGPOLL_DISABLE,
    PHYDEVLIB_PARAM_BDFINFO,
    PHYDEVLIB_PARAM_MMAP_BASE,
    PHYDEVLIB_PARAM_MMAP_PHY_BASE,
} PHYDEVLIB_PARAM_ID;

/*
 * The followings are defined for legacy compatibility
 * Use phyRegRead32() and phyRegWrite32() in RDL/PDL
 */

#if defined(PHYDEVLIB_IMAGE_MM_FTM)
#if defined(TEST_FRAMEWORK)

#if !defined(registerWriteEnv)
extern unsigned int registerRead(unsigned int port);
extern void registerWrite(unsigned int port, unsigned int val);
#define registerWriteEnv(addr, value) \
    registerWrite(addr, value);       \
    registerRead(addr)
#endif  //#if !defined(registerWriteEnv)

#define phyRegRead32(addr)        registerRead(addr)
#define phyRegRead64(addr)        registerRead(addr)
#define phyRegWrite32(addr, data) registerWrite(addr, data)
#define phyRegWrite64(addr, data) registerWrite(addr, data)

#else

#define phyRegRead32(addr)        (*((volatile uint32_t *)((uintptr_t)(addr))))
#define phyRegRead64(addr)        (*((volatile uint64_t *)((uintptr_t)(addr))))
#define phyRegWrite32(addr, data) (*((volatile uint32_t *)((uintptr_t)(addr))) = ((uint32_t)(data)))
#define phyRegWrite64(addr, data) (*((volatile uint64_t *)((uintptr_t)(addr))) = ((uint64_t)(data)))

#ifndef registerRead
#define registerRead(port) (*((volatile uint32_t *)((uint32_t)(port))))
#endif

#ifndef registerWrite
#define registerWrite(port, val) (*((volatile uint32_t *)((uint32_t)(port))) = ((uint32_t)(val)))
#endif

#ifndef registerReadEnv
#define registerReadEnv(addr) phyRegRead32Env(addr)
#endif

#ifndef registerWriteEnv
#define registerWriteEnv(addr, value) phyRegWrite32Env(addr, value)
#endif

#endif  //#if defined(TEST_FRAMEWORK)
#endif  //#if defined(PHYDEVLIB_IMAGE_MM_FTM)

#endif /* _PHY_DEVLIB_API_DEF_H */
