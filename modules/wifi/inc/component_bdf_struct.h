/*
Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
*/
/************************************************************************/
/* Chip specific BDF structures                                         */
/************************************************************************/
// 0.0.18
#ifndef _COMPONENT_BDF_STRUCT_H_
#define _COMPONENT_BDF_STRUCT_H_
#include <stdint.h>
#include "component_bdf_def.h"
#include "bdf_template.h"
#ifndef __ATTRIB_PACK
#ifdef __GNUC__
#define __ATTRIB_PACK __attribute__((packed))
#else
#define __ATTRIB_PACK
#endif
#endif /* __ATTRIB_PACK */

#ifdef _MSC_VER
#pragma pack(push, 1)
#endif

// Custom Macros

// Macros Dim1
#define RXGAIN_CAL_DATA_RESERVED_COUNT 2

// Macros Dim2

// Macros Dim3

// Typedefs

// common bdf structures

typedef struct {
    uint16_t nvId;
    uint16_t nvLen;
    uint32_t nvFlag;
} __ATTRIB_PACK NV_HEADER;

typedef struct {
    uint16_t length;
    uint16_t checksum;
    uint8_t templateVerMinor;
    uint8_t templateVerMajor;
    uint8_t macAddr[MAX_NUM_MAC_ADDRESS][MAC_ADDRESS_SIZE];
    uint16_t regDmn[BASE_BDF_HEADER_REGDMN_COUNT];
    uint8_t refDesignId;
    uint8_t customerId;
    uint8_t projectId;
    uint8_t boardDataRev;
    uint8_t nvMacFlag;
    uint8_t calSmVersion;
    uint32_t hwCReserved;
    uint32_t boardFlags;
    uint16_t calVersion;
    uint16_t iniVersion;
    uint8_t numMacAddr;
    uint8_t numPhy;
    uint32_t fwVersion;
    uint16_t bdfVersion;
    uint8_t bdfTemplateVer1;
    uint8_t bdfTemplateVer2;
    uint8_t bdfTemplateVer3;
    uint8_t bdfHwsTemplateVer1;
    uint8_t bdfHwsTemplateVer2;
    uint8_t bdfHwsTemplateVer3;
    uint8_t HDLVerStampLvl1;
    uint8_t HDLVerStampLvl2;
    uint8_t HDLVerStampLvl3;
    uint8_t HDLVerStampLvl4;
    uint8_t ctlQuarterStepPowerFlag;
    uint8_t regDBVer;
    uint8_t boardId;
    uint8_t baseFuture[BASE_BDF_HEADER_FUTURE];
} __ATTRIB_PACK BASE_BDF_HEADER;

typedef struct {
    int8_t rate_1_L;
    int8_t rate_2_L;
    int8_t rate_2_S;
    int8_t rate_5_L;
    int8_t rate_5_S;
    int8_t rate_11_L;
    int8_t rate_11_S;
} __ATTRIB_PACK CCK_RATES;

typedef struct {
    int8_t rate_6;
    int8_t rate_9;
    int8_t rate_12;
    int8_t rate_18;
    int8_t rate_24;
    int8_t rate_36;
    int8_t rate_48;
    int8_t rate_54;
} __ATTRIB_PACK LEGACY_RATES;

typedef struct {
    int8_t rate_ht_0;
    int8_t rate_ht_1;
    int8_t rate_ht_2;
    int8_t rate_ht_3;
    int8_t rate_ht_4;
    int8_t rate_ht_5;
    int8_t rate_ht_6;
    int8_t rate_ht_7;
} __ATTRIB_PACK HT_RATES;

typedef struct {
    int8_t rate_1_L;
    int8_t rate_2_L;
    int8_t rate_2_S;
} __ATTRIB_PACK CEB_RATES;

typedef struct {
    CCK_RATES cckRates;
    LEGACY_RATES legacyRates;
    HT_RATES ht20Rates;
    CEB_RATES cebRates;
} __ATTRIB_PACK TARGET_POWERS_2G;

typedef struct {
    CCK_RATES cckRates;
    LEGACY_RATES legacyRates;
    HT_RATES ht20Rates;
    CEB_RATES cebRates;
} __ATTRIB_PACK TARGET_POWERS_5G;

typedef struct {
    CCK_RATES cckRates;
    LEGACY_RATES legacyRates;
    HT_RATES ht20Rates;
    CEB_RATES cebRates;
} __ATTRIB_PACK TARGET_POWERS_6G;

typedef struct {
    int8_t scpc_power_offset_cck;
    int8_t scpc_power_offset_ofdm20;
} __ATTRIB_PACK SCPC_POWER_OFFSET;

typedef struct {
    uint8_t bandMask;
    int8_t TxPwrCCK;
    int8_t TxPwrOFDM;
    uint8_t rateCCK;
    uint8_t rateOFDM;
    uint8_t numChan;
    int8_t InitGainCCK;
    int8_t InitGainOFDM;
} __ATTRIB_PACK SCPC_CAL_CFG;

typedef struct {
    uint8_t bandMask;
    int8_t refISS;
    uint8_t rate;
    uint8_t bandWidth;
    uint8_t numChan;
    uint8_t numChain;
    uint16_t numPkts;
    uint16_t chans[MAX_RXG_CAL_2G_CHANS];
} __ATTRIB_PACK RXGAIN_CAL_2G_CFG;

#ifdef CONFIG_6G_BAND
typedef struct {
    uint8_t bandMask;
    int8_t refISS;
    uint8_t rate;
    uint8_t bandWidth;
    uint8_t numChan;
    uint8_t numChain;
    uint16_t numPkts;
    uint16_t chans[MAX_RXG_CAL_5G6G_CHANS];
} __ATTRIB_PACK RXGAIN_CAL_5G6G_CFG;
#else
typedef struct {
    uint8_t bandMask;
    int8_t refISS;
    uint8_t rate;
    uint8_t bandWidth;
    uint8_t numChan;
    uint8_t numChain;
    uint16_t numPkts;
    uint16_t chans[MAX_RXG_CAL_5G_CHANS];
} __ATTRIB_PACK RXGAIN_CAL_5G_CFG;
#endif

typedef struct {
    int8_t rxNFCalPowerDBr;
    int8_t rxNFCalPowerDBm;
    int8_t rxNFCalPowerDBmLGT;
    uint8_t rxTempMeas;
    int8_t rxNFThermCalSlope;
    int8_t minCcaThreshold;
    uint8_t reserved[RXGAIN_CAL_DATA_RESERVED_COUNT];
} __ATTRIB_PACK RXGAIN_CAL_DATA;

typedef struct {
    RXGAIN_CAL_2G_CFG rxGainCalCfg;
    RXGAIN_CAL_DATA rxGainCalResult[MAX_RXG_CAL_2G_CHANS];
} __ATTRIB_PACK RXGAIN_CAL_2G_TABLE;

#ifdef CONFIG_6G_BAND
typedef struct {
    RXGAIN_CAL_5G6G_CFG rxGainCalCfg;
    RXGAIN_CAL_DATA rxGainCalResult[MAX_RXG_CAL_5G6G_CHANS];
} __ATTRIB_PACK RXGAIN_CAL_5G6G_TABLE;
#else
typedef struct {
    RXGAIN_CAL_5G_CFG rxGainCalCfg;
    RXGAIN_CAL_DATA rxGainCalResult[MAX_RXG_CAL_5G_CHANS];
} __ATTRIB_PACK RXGAIN_CAL_5G_TABLE;
#endif

typedef struct {
    uint16_t pdadc_read;
    uint8_t meas_pwr;
} __ATTRIB_PACK CAL_DATA_PER_POINT_CLPC;

typedef struct {
    CAL_DATA_PER_POINT_CLPC calPerPoint_clpcf[CLPC_NUM_CAL_POINTS];
} __ATTRIB_PACK CAL_DATA_PER_FREQ_CLPCF;

typedef struct {
    int8_t clpc_init_fine_gain_cck;
    int8_t clpc_init_fine_gain_ofdm20;
} __ATTRIB_PACK CLPC_INIT_FINE_GAIN;

typedef struct {
    uint8_t dacGainIdxForCal[WHAL_NUM_POINTS_TO_MEAS];
    int16_t calDataTgtPwr[WHAL_NUM_POINTS_TO_MEAS];
    uint8_t calPdadcTargets[WHAL_NUM_CAL_POINTS_FULL_CHAN];
} __ATTRIB_PACK PRE_TXCAL_GUIDE_PER_BAND;

typedef struct {
    uint16_t spurChanFreq2G[HALPHY_SPUR_CHANS_2G];
    int8_t spurOffset2G_x10[HALPHY_SPUR_CHANS_2G];
    uint8_t spurNotchFilterCckPreamble_2G;
    uint8_t spurNotchFilterCckData_2G;
    uint8_t spurNotchFilterOfdmPreamble_2G;
    uint8_t spurNotchFilterOfdmData_2G;
#ifdef CONFIG_6G_BAND
    uint16_t spurChanFreq5G6G[HALPHY_SPUR_CHANS_5G6G];
    int8_t spurOffset5G6G_x10[HALPHY_SPUR_CHANS_5G6G];
#else
    uint16_t spurChanFreq5G6G[HALPHY_SPUR_CHANS_5G];
    int8_t spurOffset5G6G_x10[HALPHY_SPUR_CHANS_5G];
#endif
    uint32_t spurNotchFilterCckPreamble_5G6G;
    uint32_t spurNotchFilterCckData_5G6G;
    uint32_t spurNotchFilterOfdmPreamble_5G6G;
    uint32_t spurNotchFilterOfdmData_5G6G;
    uint8_t spurRssiThreshSel;
    uint8_t spurRssiThreshOfdm;
    uint8_t spurRssiThreshCck;
    uint8_t spurMitFlag;
} __ATTRIB_PACK SPUR_MIT;

typedef struct {
    uint16_t deltaCapin;
    uint16_t deltaCapout;
    int16_t temperature;
    int16_t reserved;
} __ATTRIB_PACK HALPHY_XTAL_TEMP_COMP;

typedef struct {
    uint16_t calCapIn;
    uint16_t calCapOut;
    HALPHY_XTAL_TEMP_COMP xtalTempComp[MAX_XTAL_TEMP_COMP];
} __ATTRIB_PACK XTAL_CAL_DATA;

typedef struct {
    uint8_t t_power : 7;
    uint8_t flag : 1;
} __ATTRIB_PACK table_u;

typedef struct {
    table_u u;
} __ATTRIB_PACK CAL_CTL_EDGE_PWR;

typedef struct {
    uint8_t mode;
    uint8_t beamforming : 1;
    uint8_t regDmn : 4;
    uint8_t reserved : 3;
    uint8_t numChMask;
    uint8_t numSSMask;
} __ATTRIB_PACK CTL_INDEX;

typedef struct {
    uint8_t numRegDomain;
    uint8_t numChannelRowsPerMode;
    uint16_t ctlFlags;
} __ATTRIB_PACK CTL_CONFIG;

typedef struct {
    uint8_t thrCcaPri20dB;
    uint8_t thrCcaPri20dB_FCC;
} __ATTRIB_PACK AGC_ENERGY_DETECT_CFG;

typedef struct {
    int8_t lowThresh[HALPHY_NUM_BANDS];
    int8_t lowThreshOffset[HALPHY_NUM_BANDS];
    int8_t highThresh[HALPHY_NUM_BANDS];
    int8_t highThreshOffset[HALPHY_NUM_BANDS];
} __ATTRIB_PACK SCPC_TEMP_ADJUSTMENT;

typedef struct {
    uint32_t address;
    uint32_t regMask;
    uint32_t lowValue[HALPHY_NUM_BANDS];
    uint32_t midValue[HALPHY_NUM_BANDS];
    uint32_t highValue[HALPHY_NUM_BANDS];
} __ATTRIB_PACK PHYRF_REG_TEMP_BASED_UPDATE;

typedef struct {
    uint16_t CountryCode;
    uint16_t ChannelList[NUM_NON_SUPPORTED_CHANNELS];
} __ATTRIB_PACK NON_SUPPORTED_CHANNEL_LIST_PER_COUNTRY;

typedef struct {
    uint16_t DAC_BO_NOM_2G;
    uint16_t DAC_BO_NOM_2G2H;
    uint16_t DAC_BO_NOM_5G_freq1;
    uint16_t DAC_BO_NOM_5G_freq2;
    uint16_t DAC_BO_NOM_5G_freq3;
    uint16_t DAC_BO_NOM_6G_freq1;
    uint16_t DAC_BO_NOM_6G_freq2;
    uint16_t DAC_BO_NOM_6G_freq3;
    uint8_t DAC_BO_Enable;
} __ATTRIB_PACK DAC_BO_CAL_STRUCT;

#ifdef _MSC_VER
#pragma pack(pop)
#endif

#endif /* _COMPONENT_BDF_STRUCT_H_ */
