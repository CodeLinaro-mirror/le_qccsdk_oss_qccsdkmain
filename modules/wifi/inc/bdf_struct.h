 /*
Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/************************************************************************/
/* Header file for BDF module                                           */
/************************************************************************/
// 0.0.18
#ifndef _BDF_STRUCT_H_
#define _BDF_STRUCT_H_
#include <stdint.h>
#include "component_bdf_struct.h"
#include "bdf_template.h"
#include "component_bdf_def.h"
#define BDF_TEMPLATE_VER1 0
#define BDF_TEMPLATE_VER2 0
#define BDF_TEMPLATE_VER3 21

#ifdef _MSC_VER
#pragma pack(push, 1)
#endif




// Custom Macros

// NV Item-Member Macros
#define FW_CONFIG_CUSTOMAGCREGISTERADDRESSVALUEPAIR2G_COUNT	10

#define FW_CONFIG_CUSTOMAGCREGISTERADDRESSVALUEPAIR5G_COUNT	10

#define FW_CONFIG_CUSTOMAGCREGISTERADDRESSVALUEPAIR6G_COUNT	10

#define FW_CONFIG_CRX_RSSI_CORRECTION_COUNT	7

typedef struct bdfStruct {

// NV Items
// structure segment:
    //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    // [COMMON_BDF_HEADER] settings                                x
    //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

	NV_HEADER	nvBase;
	BASE_BDF_HEADER	baseBdfHeader;
// COMMON_BDF_HEADER ends

// structure segment:
    //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    //[ IOT_TARGET_POWER_TABLES] settings                          x
    //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

	NV_HEADER	nvTargetPwr;
	uint16_t	targetPowerR2PFreqRange5G[HALPHY_RF_SUBBAND_5G_MAX];
#ifdef CONFIG_6G_BAND
	uint16_t	targetPowerR2PFreqRange6G[HALPHY_RF_SUBBAND_6G_MAX];
#endif
	TARGET_POWERS_2G	targetPowerR2PTable2G[HALPHY_RF_SUBBAND_2G_MAX];
	uint8_t	targetPower2GFuture[TARGET_POWER_2G_FUTURE];
	TARGET_POWERS_5G	targetPowerR2PTable5G[HALPHY_RF_SUBBAND_5G_MAX];
	uint8_t	targetPower5GFuture[TARGET_POWER_5G_FUTURE];
#ifdef CONFIG_6G_BAND
	TARGET_POWERS_6G	targetPowerR2PTable6G[HALPHY_RF_SUBBAND_6G_MAX];
	uint8_t	targetPower6GFuture[TARGET_POWER_6G_FUTURE];
#endif
// IOT_TARGET_POWER_TABLES ends

// structure segment:
    //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    // [CUSTOMIZE_STICKY_WRITE_COMMAND_TABLE] settings             x
    //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

	NV_HEADER	nvConfigAddr;
	uint32_t	configAddr[HALPHY_CONFIG_ENTRIES];
// CUSTOMIZE_STICKY_WRITE_COMMAND_TABLE ends

// structure segment:
    //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    // [IOT_RX_GAIN_TABLES] settings                               x
    //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

	NV_HEADER	nvRxGain;
	RXGAIN_CAL_2G_TABLE	rxGainCalTbl2G;
#ifdef CONFIG_6G_BAND
	RXGAIN_CAL_5G6G_TABLE	rxGainCalTbl5G6G;
#else
	RXGAIN_CAL_5G_TABLE	rxGainCalTbl5G;
#endif
	uint8_t	rssiOffsetFuture[RX_GAIN_FUTURE];
// IOT_RX_GAIN_TABLES ends

// structure segment:
    //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    // [IOT_XTAL_CAL_SECTION] settings                             x
    //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

	NV_HEADER	nvXtalCal;
	XTAL_CAL_DATA	xtalCalData;
	uint8_t	xtalCalFuture[XTAL_CAL_FUTURE];
// IOT_XTAL_CAL_SECTION ends

// structure segment:
    //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    // [FW_CONFIG] settings                                        x
    //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

	NV_HEADER	nvFwConfig;
	uint8_t	tpc_flag;
	uint32_t	xFEMCtrl;	// xLNAcontrolswitchforwl_mc.R_CFG_XFEM_CTRL(0x02040098)
	uint8_t	xLNA5G6G;
	uint8_t	xPAEnable;	// XPA_CFG field in R_XFEM_1 register. bit0-3: 5G, bit4-7: 2G
	uint8_t	lnaPwrMode2G;
	uint8_t	lnaPwrMode5G;
	uint8_t	calDBEnableDisableFTM;
	uint8_t	DpdEnable2G;
	uint8_t	DpdEnable5G;
#ifdef CONFIG_6G_BAND
	uint8_t	DpdEnable6G;
#endif
	int8_t	txPowerOffset;
	uint8_t	hwCalEnable;
	uint8_t	CaldbBypass;
	uint8_t	TxPowerMode2G[TXPOWERMODE_NUM_2G_SUB_BANDS];
	uint8_t	TxPowerMode5G[TXPOWERMODE_NUM_5G_SUB_BANDS];
	uint8_t	TxPowerMode6G[TXPOWERMODE_NUM_6G_SUB_BANDS];
	uint16_t	TxPowerModeFeqBoundary2G;
	uint16_t	TxPowerModeFreqLowBoundary5G;
	uint16_t	TxPowerModeFreqHighBoundary5G;
	uint16_t	TxPowerModeFreqLowBoundary6G;
	uint16_t	TxPowerModeFreqHighBoundary6G;
	uint8_t	crxEnable;
	uint32_t	customAGCRegisterAddressValuePair2G[FW_CONFIG_CUSTOMAGCREGISTERADDRESSVALUEPAIR2G_COUNT];
	uint32_t	customAGCRegisterAddressValuePair5G[FW_CONFIG_CUSTOMAGCREGISTERADDRESSVALUEPAIR5G_COUNT];
#ifdef CONFIG_6G_BAND
	uint32_t	customAGCRegisterAddressValuePair6G[FW_CONFIG_CUSTOMAGCREGISTERADDRESSVALUEPAIR6G_COUNT];
#endif
	uint8_t	wlan11bTXfilterID;
	uint8_t	wlan11bChannel14TXfilterID;
	uint16_t	cbc_ch_list[CHLIST_CBC_NUM];
	uint16_t	cbc_future_ch_list[MAX_FUTURE_CBC_CHANNEL];
	uint8_t	TempBasedRecalEnable;
	uint16_t	TempCompTimerPeriodicity;
	int8_t	Ctl_11b_offset_5g[HALPHY_NUM_REG_DMNS];
#ifdef CONFIG_6G_BAND
	int8_t	Ctl_11b_offset_6g[HALPHY_NUM_REG_DMNS];
#endif
	uint32_t	rttBaseDelay2G;
	uint32_t	rttBaseDelay5G;
#ifdef CONFIG_6G_BAND
	uint32_t	rttBaseDelay6G;
	uint8_t	CEBenable;
	int8_t	FineGainOffsetBPSK;
	int8_t	FineGainOffsetQPSK;
	int8_t	FineGainOffset16QAM;
#endif
	int8_t	rssi_range_0[HALPHY_NUM_BANDS];
	int8_t	rssi_range_1[HALPHY_NUM_BANDS];
	int8_t	rssi_range_2[HALPHY_NUM_BANDS];
	int8_t	rssi_range_3[HALPHY_NUM_BANDS];
	int8_t	rssi_correction_0[HALPHY_NUM_BANDS];
	int8_t	rssi_correction_1[HALPHY_NUM_BANDS];
	int8_t	rssi_correction_2[HALPHY_NUM_BANDS];
	int8_t	rssi_correction_3[HALPHY_NUM_BANDS];
	int8_t	rssi_correction_4[HALPHY_NUM_BANDS];
	int8_t	rssi_temp_low;
	int8_t	rssi_temp_high;
	int8_t	rssi_temp_correction_low[HALPHY_NUM_BANDS];
	int8_t	rssi_temp_correction_mid[HALPHY_NUM_BANDS];
	int8_t	rssi_temp_correction_high[HALPHY_NUM_BANDS];
	int8_t	crx_rssi_correction[FW_CONFIG_CRX_RSSI_CORRECTION_COUNT];
	int8_t	scpc_offset_process_corner[HALPHY_NUM_BANDS][HALPHY_NUM_PROCESS_CORNERS];
	uint8_t	TempBasedCBCEnable;
	int8_t	TempBasedRecalThLo[HALPHY_NUM_BANDS];
	int8_t	TempBasedRecalThHi[HALPHY_NUM_BANDS];
	NON_SUPPORTED_CHANNEL_LIST_PER_COUNTRY	NonSupportedChannelsPerCountry[NUM_NON_SUPPORTED_COUNTRY];
	uint8_t	FwConfigFuture[FW_CONFIG_FUTURE];
// FW_CONFIG ends

// structure segment:
    //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    // [IOT_TPC_DATA] settings                                     x
    //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

	NV_HEADER	nvTpcData;
	int8_t	TpcOffsetAdj2G;
	int8_t	TpcOffsetAdj5G;
#ifdef CONFIG_6G_BAND
	int8_t	TpcOffsetAdj6G;
#endif
	uint16_t	calFreqPier2G[TPC_DATA_2G_FREQ];
#ifdef CONFIG_6G_BAND
	uint16_t	calFreqPier5G6G[TPC_DATA_5G6G_FREQ];
#else
	uint16_t	calFreqPier5G[TPC_DATA_5G_FREQ];
#endif
	SCPC_POWER_OFFSET	scpcPowerOffset2G[TPC_DATA_2G_FREQ];
#ifdef CONFIG_6G_BAND
	SCPC_POWER_OFFSET	scpcPowerOffset5G6G[TPC_DATA_5G6G_FREQ];
#else
	SCPC_POWER_OFFSET	scpcPowerOffset5G[TPC_DATA_5G_FREQ];
#endif
	CLPC_INIT_FINE_GAIN	clpcInitFineGain2G[TPC_DATA_2G_FREQ];
#ifdef CONFIG_6G_BAND
	CLPC_INIT_FINE_GAIN	clpcInitFineGain5G6G[TPC_DATA_5G6G_FREQ];
#else
	CLPC_INIT_FINE_GAIN	clpcInitFineGain5G[TPC_DATA_5G_FREQ];
#endif
	PRE_TXCAL_GUIDE_PER_BAND	 preCalData2G;
	PRE_TXCAL_GUIDE_PER_BAND	 preCalData5G;
#ifdef CONFIG_6G_BAND
	PRE_TXCAL_GUIDE_PER_BAND	 preCalData6G;
#endif
	CAL_DATA_PER_FREQ_CLPCF	 calPierData2G_CLPC[TPC_DATA_2G_FREQ];
#ifdef CONFIG_6G_BAND
	CAL_DATA_PER_FREQ_CLPCF	 calPierData5G6G_CLPC[TPC_DATA_5G6G_FREQ];
#else
	CAL_DATA_PER_FREQ_CLPCF	 calPierData5G_CLPC[TPC_DATA_5G_FREQ];
#endif
	uint8_t	scpcTempCompensationEnable;
	uint16_t	PadcConverx10002G;
	uint16_t	PadcConverx10005G;
#ifdef CONFIG_6G_BAND
	uint16_t	PadcConverx10006G;
#endif
	uint8_t	ClpcDpdColdBootEnable;
	int16_t	TempGradx10002G[TEMPERATURE_REGIONS][HALPHY_NUM_PROCESS_CORNERS];
	int16_t	TempGradx10005G[TEMPERATURE_REGIONS][HALPHY_NUM_PROCESS_CORNERS];
#ifdef CONFIG_6G_BAND
	int16_t	TempGradx10006G[TEMPERATURE_REGIONS][HALPHY_NUM_PROCESS_CORNERS];
#endif
	SCPC_CAL_CFG	scpcCalConfig2G;
	SCPC_CAL_CFG	scpcCalConfig5G6G;
	int8_t	scpcCalConfig6GInitGainCCK;
	int8_t	scpcCalConfig6GInitGainOFDM;
	SCPC_TEMP_ADJUSTMENT 	scpcTempBasedAdj[HALPHY_NUM_RATES_FOR_TEMP_BASED_TPC_ADJUSTMENT];
	int8_t	CalRefTemp2G;
	int8_t	CalRefTemp5G6G;
	int8_t	scpcTempThLo;
	int8_t	scpcTempThHi;
	DAC_BO_CAL_STRUCT	dacBoCalConfig;
	uint8_t	TpcDataOffsetFuture[TPC_DATA_OFFSET_FUTURE];
// IOT_TPC_DATA ends

// structure segment:
    //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    // [IOT_FREQ_MODAL] settings                                   x
    //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

	NV_HEADER	nvSpurMit;
	SPUR_MIT	spurMitConfig;
	uint8_t	spurMitFuture[MAX_SPUR_MIT_FUTURE];
// IOT_FREQ_MODAL ends

// structure segment:
    //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    // [CTL_TABLES] settings                                       x
    //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

	NV_HEADER	nvCtl;
	CTL_CONFIG	ctlConfiguration;
	CTL_INDEX	ctlIndex2G_11b[HALPHY_NUM_CTLS_2G_11B];
	uint16_t	ctlFreqbin2G_11b[HALPHY_NUM_REG_DMNS][HALPHY_NUM_BAND_EDGES_2G_11B];
	int8_t	ctlData2G_11b[HALPHY_NUM_CTLS_2G_11B][HALPHY_NUM_BAND_EDGES_2G_11B];
	CTL_INDEX	ctlIndex2G_11g[HALPHY_NUM_CTLS_2G_11G];
	uint16_t	ctlFreqbin2G_11g[HALPHY_NUM_REG_DMNS][HALPHY_NUM_BAND_EDGES_2G_11G];
	int8_t	ctlData2G_11g[HALPHY_NUM_CTLS_2G_11G][HALPHY_NUM_BAND_EDGES_2G_11G];
	CTL_INDEX	ctlIndex2G_HT20[HALPHY_NUM_CTLS_2G_HT20];
	uint16_t	ctlFreqbin2G_HT20[HALPHY_NUM_REG_DMNS][HALPHY_NUM_BAND_EDGES_2G_HT20];
	int8_t	ctlData2G_HT20[HALPHY_NUM_CTLS_2G_HT20][HALPHY_NUM_BAND_EDGES_2G_HT20];
	uint8_t	ctlSpare2G[MAX_CTL_SPARE_2G];
	CTL_INDEX	ctlIndex5G_11a[HALPHY_NUM_CTLS_5G_11A];
	uint16_t	ctlFreqbin5G_11a[HALPHY_NUM_REG_DMNS][HALPHY_NUM_BAND_EDGES_5G_11A];
	int8_t	ctlData5G_11a[HALPHY_NUM_CTLS_5G_11A][HALPHY_NUM_BAND_EDGES_5G_11A];
	CTL_INDEX	ctlIndex5G_HT20[HALPHY_NUM_CTLS_5G_HT20];
	uint16_t	ctlFreqbin5G_HT20[HALPHY_NUM_REG_DMNS][HALPHY_NUM_BAND_EDGES_5G_HT20];
	int8_t	ctlData5G_HT20[HALPHY_NUM_CTLS_5G_HT20][HALPHY_NUM_BAND_EDGES_5G_HT20];
	uint8_t	ctlSpare5G[MAX_CTL_SPARE_5G];
// CTL_TABLES ends

// structure segment:
    //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    // [CTL_TABLES_6G] settings                                    x
    //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
#ifdef CONFIG_6G_BAND
	NV_HEADER	nvCtlTables6g;
	CTL_INDEX	ctlIndex6G_SP_11a[HALPHY_NUM_CTLS_5G_11A];
	CTL_INDEX	ctlIndex6G_LPI_11a[HALPHY_NUM_CTLS_5G_11A];
	CTL_INDEX	ctlIndex6G_VLP_11a[HALPHY_NUM_CTLS_5G_11A];
	uint16_t	ctlFreqbin6G_SP_11a[HALPHY_NUM_REG_DMNS][HALPHY_NUM_BAND_EDGES_6G_EXT_11A];
	uint16_t	ctlFreqbin6G_LPI_11a[HALPHY_NUM_REG_DMNS][HALPHY_NUM_BAND_EDGES_6G_EXT_11A];
	uint16_t	ctlFreqbin6G_VLP_11a[HALPHY_NUM_REG_DMNS][HALPHY_NUM_BAND_EDGES_6G_EXT_11A];
	int8_t	ctlData6G_SP_11a[HALPHY_NUM_CTLS_5G_11A][HALPHY_NUM_BAND_EDGES_6G_EXT_11A];
	int8_t	ctlData6G_LPI_11a[HALPHY_NUM_CTLS_5G_11A][HALPHY_NUM_BAND_EDGES_6G_EXT_11A];
	int8_t	ctlData6G_VLP_11a[HALPHY_NUM_CTLS_5G_11A][HALPHY_NUM_BAND_EDGES_6G_EXT_11A];
	CTL_INDEX	ctlIndex6G_SP_HE20[HALPHY_NUM_CTLS_5G_HT20];
	CTL_INDEX	ctlIndex6G_LPI_HE20[HALPHY_NUM_CTLS_5G_HT20];
	CTL_INDEX	ctlIndex6G_VLP_HE20[HALPHY_NUM_CTLS_5G_HT20];
	uint16_t	ctlFreqbin6G_SP_HE20[HALPHY_NUM_REG_DMNS][HALPHY_NUM_BAND_EDGES_6G_EXT_HT20];
	uint16_t	ctlFreqbin6G_LPI_HE20[HALPHY_NUM_REG_DMNS][HALPHY_NUM_BAND_EDGES_6G_EXT_HT20];
	uint16_t	ctlFreqbin6G_VLP_HE20[HALPHY_NUM_REG_DMNS][HALPHY_NUM_BAND_EDGES_6G_EXT_HT20];
	int8_t	ctlData6G_SP_HE20[HALPHY_NUM_CTLS_5G_HT20][HALPHY_NUM_BAND_EDGES_6G_EXT_HT20];
	int8_t	ctlData6G_LPI_HE20[HALPHY_NUM_CTLS_5G_HT20][HALPHY_NUM_BAND_EDGES_6G_EXT_HT20];
	int8_t	ctlData6G_VLP_HE20[HALPHY_NUM_CTLS_5G_HT20][HALPHY_NUM_BAND_EDGES_6G_EXT_HT20];
	uint8_t	ctlTables6gFuture[CTL_6G_FUTURE];
// CTL_TABLES_6G ends
#endif

// structure segment:
    //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    // [IOT_AGC_ENERGY_DETECT_THRESHOLD]  settings                                x
    //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
	NV_HEADER	nvagcEnergyDetThr;
	AGC_ENERGY_DETECT_CFG	agcEnergyDetThr[HALPHY_NUM_BANDS];
	uint8_t	agcEnergyDetThrFuture[AGC_ED_DET_FUTURE];
// IOT_AGC_ENERGY_DETECT_THRESHOLD ends

// structure segment:	 
    //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    // [IOT_TEMPERATURE_CONFIGURATION]  settings                                x
    //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
	NV_HEADER	nvTemperatureConfig;
	uint8_t	num_phyrf_regs_to_update;
	PHYRF_REG_TEMP_BASED_UPDATE	phyrf_reg_temp_based_update_list[HALPHY_MAX_REGS_TEMP_BASED_UPDATE];
	int8_t	reg_update_temp_threshold_low;
	int8_t	reg_update_temp_threshold_high;
	uint8_t	temperatureConfigFuture[TEMPERATURE_CONFIG_FUTURE];
// IOT_TEMPERATURE_CONFIGURATION ends

// structure segment:	
    //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    // [RESERVED_NV]                                               x
    //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
	NV_HEADER	nvReserved;
	uint8_t	reservedFuture[MAX_RESERVED_FUTURE];
// RESERVED_NV ends

} __ATTRIB_PACK BDF_STRUCT;

typedef struct cachedBdfStruct{
// structure segment:
    //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    // [IOT_RX_GAIN_TABLES] settings                               x
    //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

	NV_HEADER	nvRxGain;
	RXGAIN_CAL_2G_TABLE	rxGainCalTbl2G;
#ifdef CONFIG_6G_BAND
	RXGAIN_CAL_5G6G_TABLE	rxGainCalTbl5G6G;
#else
	RXGAIN_CAL_5G_TABLE	rxGainCalTbl5G;
#endif
	uint8_t	rssiOffsetFuture[RX_GAIN_FUTURE];
// IOT_RX_GAIN_TABLES ends

// structure segment:
    //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    // [IOT_TPC_DATA] settings                                     x
    //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

	NV_HEADER	nvTpcData;
	int8_t	TpcOffsetAdj2G;
	int8_t	TpcOffsetAdj5G;
#ifdef CONFIG_6G_BAND
	int8_t	TpcOffsetAdj6G;
#endif
	uint16_t	calFreqPier2G[TPC_DATA_2G_FREQ];
#ifdef CONFIG_6G_BAND
	uint16_t	calFreqPier5G6G[TPC_DATA_5G6G_FREQ];
#else
	uint16_t	calFreqPier5G[TPC_DATA_5G_FREQ];
#endif
	SCPC_POWER_OFFSET	scpcPowerOffset2G[TPC_DATA_2G_FREQ];
#ifdef CONFIG_6G_BAND
	SCPC_POWER_OFFSET	scpcPowerOffset5G6G[TPC_DATA_5G6G_FREQ];
#else
	SCPC_POWER_OFFSET	scpcPowerOffset5G[TPC_DATA_5G_FREQ];
#endif
	CLPC_INIT_FINE_GAIN	clpcInitFineGain2G[TPC_DATA_2G_FREQ];
#ifdef CONFIG_6G_BAND
	CLPC_INIT_FINE_GAIN	clpcInitFineGain5G6G[TPC_DATA_5G6G_FREQ];
#else
	CLPC_INIT_FINE_GAIN	clpcInitFineGain5G[TPC_DATA_5G_FREQ];
#endif
	PRE_TXCAL_GUIDE_PER_BAND	 preCalData2G;
	PRE_TXCAL_GUIDE_PER_BAND	 preCalData5G;
#ifdef CONFIG_6G_BAND
	PRE_TXCAL_GUIDE_PER_BAND	 preCalData6G;
#endif
	CAL_DATA_PER_FREQ_CLPCF	 calPierData2G_CLPC[TPC_DATA_2G_FREQ];
#ifdef CONFIG_6G_BAND
	CAL_DATA_PER_FREQ_CLPCF	 calPierData5G6G_CLPC[TPC_DATA_5G6G_FREQ];
#else
	CAL_DATA_PER_FREQ_CLPCF	 calPierData5G_CLPC[TPC_DATA_5G_FREQ];
#endif
	uint8_t	scpcTempCompensationEnable;
	uint16_t	PadcConverx10002G;
	uint16_t	PadcConverx10005G;
#ifdef CONFIG_6G_BAND
	uint16_t	PadcConverx10006G;
#endif
	uint8_t	ClpcDpdColdBootEnable;
	int16_t	TempGradx10002G[TEMPERATURE_REGIONS][HALPHY_NUM_PROCESS_CORNERS];
	int16_t	TempGradx10005G[TEMPERATURE_REGIONS][HALPHY_NUM_PROCESS_CORNERS];
#ifdef CONFIG_6G_BAND
	int16_t	TempGradx10006G[TEMPERATURE_REGIONS][HALPHY_NUM_PROCESS_CORNERS];
#endif
	SCPC_CAL_CFG	scpcCalConfig2G;
	SCPC_CAL_CFG	scpcCalConfig5G6G;
	int8_t	scpcCalConfig6GInitGainCCK;
	int8_t	scpcCalConfig6GInitGainOFDM;
	SCPC_TEMP_ADJUSTMENT 	scpcTempBasedAdj[HALPHY_NUM_RATES_FOR_TEMP_BASED_TPC_ADJUSTMENT];
	int8_t	CalRefTemp2G;
	int8_t	CalRefTemp5G6G;
	int8_t	scpcTempThLo;
	int8_t	scpcTempThHi;
	DAC_BO_CAL_STRUCT	dacBoCalConfig;
	uint8_t	TpcDataOffsetFuture[TPC_DATA_OFFSET_FUTURE];
// IOT_TPC_DATA ends

} __ATTRIB_PACK CACHED_BDF_STRUCT;

#define NV_COMMON_BDF_HEADER_LEN         (offsetof(BDF_STRUCT, nvTargetPwr) - offsetof(BDF_STRUCT, nvBase))
#define NV_IOT_TARGET_POWER_TABLES_LEN   (offsetof(BDF_STRUCT, nvConfigAddr) - offsetof(BDF_STRUCT, nvTargetPwr))
#define NV_CONFIG_ADDR_LEN               (offsetof(BDF_STRUCT, nvRxGain) - offsetof(BDF_STRUCT, nvConfigAddr))
#define NV_IOT_RX_GAIN_TABLES_LEN        (offsetof(BDF_STRUCT, nvXtalCal) - offsetof(BDF_STRUCT, nvRxGain))
#define NV_IOT_XTAL_CAL_SECTION_LEN      (offsetof(BDF_STRUCT, nvFwConfig) - offsetof(BDF_STRUCT, nvXtalCal))
#define NV_FW_CONFIG_LEN                 (offsetof(BDF_STRUCT, nvTpcData) - offsetof(BDF_STRUCT, nvFwConfig))
#define NV_IOT_TPC_DATA_LEN              (offsetof(BDF_STRUCT, nvSpurMit) - offsetof(BDF_STRUCT, nvTpcData))
#define NV_IOT_FREQ_MODAL_LEN            (offsetof(BDF_STRUCT, nvCtl) - offsetof(BDF_STRUCT, nvSpurMit))
#define NV_CTL_TABLES_LEN                (offsetof(BDF_STRUCT, nvCtlTables6g) - offsetof(BDF_STRUCT, nvCtl))
#define NV_CTL_TABLES_6G_LEN             (offsetof(BDF_STRUCT, nvReserved) - offsetof(BDF_STRUCT, nvCtlTables6g))
#define NV_RESERVED_LEN                  (sizeof(BDF_STRUCT) - offsetof(BDF_STRUCT, nvReserved))

#ifdef _MSC_VER
#pragma pack(pop)
#endif

#endif // _BDF_STRUCT_H_
