/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _PHY_DEVLIB_API_PARM_H
#define _PHY_DEVLIB_API_PARM_H

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

#define MAX_CAL_ENGINES 2
#define MAX_PHY         3

#define CAL_DOCAL   0x1
#define CAL_RESTORE 0x2
#define CAL_ENABLE  0x4
#define CAL_DISABLE 0x8
#define CAL_SAVE    0x10

#define TABLE_IDX_HOME       0
#define TABLE_IDX_PRIMARY    TABLE_IDX_HOME
#define TABLE_IDX_SEC        1
#define TABLE_IDX_FCS        2
#define TABLE_IDX_LISTEN     3
#define TABLE_IDX_COARSE     4
#define TABLE_IDX_PRIMARY_LP 5
#define TABLE_IDX_FCS_LP     6

#define PROGRAM_PRIMARY   1
#define PROGRAM_SECONDARY 2
#define PROGRAM_6PLUS2    3

#define DO_CAL_TPC_MEM            0x1
#define DO_CAL_TPC_REG            0x2
#define DONT_CAL_SCAN_HOME_CHANGE 0x4

#if defined(PHYDEVLIB_IOT)

#include "phyDevLibApiParmIot.h"
#else
typedef struct phydevlib_phy_input {
    uint8_t phyId;
    uint8_t bandCode;
    uint8_t bwCode;
    uint8_t phyMode;
    uint16_t mhz;               /* primary 20 mhz channel frequency in mhz */
    uint16_t band_center_freq1; /* Center frequency 1 in mhz */
    uint16_t band_center_freq2; /* Center frequency 2 in mhz - valid only for 11acvht 80plus80 mode */
    uint8_t txChMask;
    uint8_t rxChMask;
    uint32_t phyBase;    /* PHY register base address */
    uint32_t phyBaseExt; /* PHY1 register base address */
    uint8_t concMode;
    uint8_t dynPriChan;
    uint8_t homeCh;
    uint8_t pwrMask;
    uint8_t extLNA;
    uint8_t extPA;
    uint8_t isSBSmode;
    uint8_t loType;   // NOTE: loType = master/slave should come from BDF !!!
    uint8_t dpdMode;  // temporary control to load TX BBF coefs in Iron INI;
    uint8_t loadIniMask;
    uint16_t calMask;  // this is used in Q5 only
    uint32_t chipVersion;
    uint8_t cal_mode_ctrl;
    uint8_t synthSelMask;
    uint8_t ocl_enable;
    uint8_t xlna_bypass;
    uint16_t boardId;
    uint8_t heavyClipEn;
    uint8_t phyDfsEnMask;
    uint8_t phySscanEnMask;
    uint8_t skipIniMask;
    uint8_t heavyClipLiteMcsThr;
    int8_t rssiDbToDbmOffset;
    uint16_t aDfsSynthFreq;
    uint8_t RetainCalTables;
    uint8_t bandEdgeHcVeryLiteEn;
    uint8_t cckFirSettings;
    uint8_t pktMode;
    uint8_t resettype;
    uint8_t rfLoadIniMask;
    uint16_t spurLevel;  // HY
    uint8_t isHomeChan;
    uint8_t phySsFftPackMode;
    void *hwsBdfPointer;
} __ATTRIB_PACK PHYDEVLIB_PHY_INPUT;

typedef struct phydevlib_reg_input {
    uint32_t value;
} __ATTRIB_PACK PHYDEVLIB_REG_INPUT;

typedef struct phydevlib_reg_output {
    uint32_t value;
} __ATTRIB_PACK PHYDEVLIB_REG_OUTPUT;

typedef struct phydevlib_phyreset_input {
    uint8_t isWarmReset;
    uint8_t pad[3];
} __ATTRIB_PACK PHYDEVLIB_PHYRESET_INPUT;

#define RXDCO_NUM_LUT_ENTRY    256
#define RXDCO_MAX_NUM_CAL_GAIN 15
typedef struct dco_lut {
    int16_t LUT_Imag[RXDCO_MAX_CHAIN][RXDCO_NUM_LUT_ENTRY];
    int16_t LUT_Real[RXDCO_MAX_CHAIN][RXDCO_NUM_LUT_ENTRY];
    int16_t Range;
    int16_t pad;
} DCO_LUT;

typedef struct dco_lut_new {
    uint16_t LUT_Imag : 9, Range_Bit0 : 1, Rsvd0 : 6;
    uint16_t LUT_Real : 9, Range_Bit1 : 1, Rsvd1 : 6;
} __ATTRIB_PACK DCO_LUT_NEW;

typedef struct dco_lut_old {
    int16_t LUT_Imag[RXDCO_MAX_CHAIN];
    int16_t LUT_Real[RXDCO_MAX_CHAIN];
    int16_t Range;
    int16_t pad;
} DCO_LUT_OLD;

typedef struct rxdco_cal_result {
    uint16_t ODAC_I : 9, Range : 2, Success : 1, Rsvd0 : 4;
    uint16_t ODAC_Q : 9, CalMatrixID : 3, Rsvd1 : 4;
} __ATTRIB_PACK RXDCO_CAL_RESULT[RXDCO_MAX_CHAIN][RXDCO_MAX_NUM_CAL_GAIN];

typedef struct phydevlib_rxdco_input {
    uint8_t calCmdId;
    uint8_t cal_mode;
    uint8_t chainIdx;
    uint8_t tableIdx;

    uint8_t progPriOrSec;
    uint8_t doSWPP;
    uint8_t resultType;
    uint8_t forceTRSW;

    DCO_LUT *pDcoLut;
    DCO_LUT_NEW *pNewDcoLut;
    RXDCO_CAL_RESULT *pCalResult;
    uint32_t override;
} __ATTRIB_PACK PHYDEVLIB_RXDCO_INPUT;

typedef struct phydevlib_rxdco_output {
    DCO_LUT *pDcoLut;
    DCO_LUT_NEW *pNewDcoLut;
    RXDCO_CAL_RESULT *pCalResult;
    uint8_t forceTRSW;
    uint8_t pad[3];
} __ATTRIB_PACK PHYDEVLIB_RXDCO_OUTPUT;

typedef struct pkdet_parameters_calresult {
    uint32_t regCalResult;
    uint32_t regParameters[2];
} __ATTRIB_PACK PKDET_PARAMETERS_CALRESULT[PHYDEVLIB_PHYID_MAX][2 /* 0:DBS ; 1:SBS */][PHY_CHAIN_MAX];

typedef struct phydevlib_pkdet_input {
    uint8_t calCmdId;
    uint8_t chainIdx;
    uint8_t tableIdx;
    uint8_t pad[1];
    uint32_t calValue;
    uint32_t *pPkdetParametersCalResult;  // for Hastings-onwards save/restore
} __ATTRIB_PACK PHYDEVLIB_PKDET_INPUT;

typedef struct phydevlib_pkdet_output {
    uint32_t calValue;
    uint32_t *pPkdetParametersCalResult;  // for Hastings-onwards save/restore
} __ATTRIB_PACK PHYDEVLIB_PKDET_OUTPUT;

typedef struct phydevlib_reset_input {
    uint8_t adcOsrIdx;
    uint8_t dacOsrIdx;
    uint8_t gainIdx;
    int8_t dacGain;
    uint8_t paCfg;
    int8_t pdetAttn;
    uint8_t clpcMode;
    uint8_t glutUpdate;
    uint8_t plutUpdate;
    uint8_t femSetting;
    uint8_t chanIdx;
    uint8_t rxFemSetting;
    int32_t tempComp;
    uint32_t phyResetCtrl;
    uint8_t chanIdxExt;
    uint8_t isWarmReset;
    uint8_t value;
    uint8_t modeSwitch;
    uint32_t userParm1;
    uint32_t userParm2;
} __ATTRIB_PACK PHYDEVLIB_RESET_INPUT;

typedef struct phydevlib_reset_output {
    uint8_t status;
    uint8_t miscFlags;
    uint8_t pad[2];
} __ATTRIB_PACK PHYDEVLIB_RESET_OUTPUT;

typedef struct phydevlib_ftpgrx_input {
    uint16_t numPkts;
    uint16_t rxTimeOutSecs;
    uint8_t pktType;
    uint8_t pktSubType;
    uint32_t targetUser;
    uint32_t isAmpdu;
    uint8_t pktBWCode;
    uint8_t mcs;
    uint8_t ftpgDataType;
    uint8_t enable;
    uint8_t pad[2];
} __ATTRIB_PACK PHYDEVLIB_FTPGRX_INPUT;

typedef struct phydevlib_ftpgrx_output {
    uint8_t chainMask;
    uint8_t bwCode;
    uint16_t numPkts;
    uint16_t rxTimeOutSecs;
    uint8_t bandCode;
    uint8_t pktType;
    uint8_t pktBWCode;
    uint8_t mcs;
    uint16_t freq;
    uint16_t crcChkCnt;
    uint16_t crcPassCnt;
    uint8_t status;
    uint8_t pad[3];
} __ATTRIB_PACK PHYDEVLIB_FTPGRX_OUTPUT;

typedef struct phydevlib_ftpgtx_input {
    uint8_t pktType;     // 0 - 5
    uint8_t pktSubType;  // 0 - 5
    uint8_t numUsers;    // 1 - 2
    uint8_t ldpc;        /* needs to be 8-bits to support OFDMA-DL */
    uint8_t dcm;         /* needs to be 8-bits to support OFDMA-DL */
    uint8_t gi;          /* can take value of up to 6 to support new LTF + GI combinations  */
    uint8_t enable : 1, stbc : 1, dpd_en : 1, ibfcal_en : 1;
    uint32_t ftpgCtrl0;
    uint32_t mpduLen; /* support larger payload sizes */
    uint32_t nss;     /* needs to be 32-bits to support OFDMA-DL */
    uint32_t mcs;     /* needs to be 32-bits to support OFDMA-DL */
    uint32_t heUplinkUserRuSize : 4, heUplinkUserRuStartIdx : 8, useCenterRu : 2, targetUser : 8, heSigBMinMcs : 4,
        ofdmaReserved : 6;
    uint32_t schemeCodeIdxPri80;
    uint32_t dummyRuIdxPri80;
    uint32_t schemeCodeIdxSec80;
    uint32_t dummyRuIdxSec80;
    uint32_t tpcParamsI;
    uint16_t ftpgCount;  // num pkt
    uint8_t pktBWCode;   // unused
    uint8_t pktErrType;  // 0 - 3
    uint8_t pktErrVal;   // don't care
    uint8_t aFactor;     // unused
    uint8_t txPwrShared;
    uint8_t txPwrUnshared;
    uint8_t ru_size;       // redundant. See heUplinkUserRuSize
    uint8_t ru_start_idx;  // redundant. See heUplinkUserRuStartIdx
    uint8_t paprdChMask;
    uint8_t txPpm;          // 8-bit signed to configure Tx PPM in VREG
    uint32_t trig2TxDelay;  // Delay in clk cycles of 480 Clk
    uint16_t puncPattern;   // Support for Preamble Puncturing
    uint8_t pad[2];
    uint32_t ruAllocation[5];
} __ATTRIB_PACK PHYDEVLIB_FTPGTX_INPUT;

typedef struct phydevlib_ftpgtx_output {
    uint8_t status;
    uint8_t pad[3];
} __ATTRIB_PACK PHYDEVLIB_FTPGTX_OUTPUT;

typedef struct phydevlib_ftpg_sequence_input {
    uint32_t seqNum;
    uint32_t rptCount;
} __ATTRIB_PACK PHYDEVLIB_FTPG_SEQUENCE_INPUT;

typedef struct phydevlib_ftpg_sequence_output {
    uint8_t status;
    uint8_t pad[3];
} __ATTRIB_PACK PHYDEVLIB_FTPG_SEQUENCE_OUTPUT;

#define PHYDEVLIB_ADCCAPTURE_CTRL_SIZE 2
typedef struct phydevlib_adccapture_input {
    uint8_t chainMask;
    uint8_t adcCompMode;
    uint8_t adcTriggerMode;
    uint8_t chanmem;
    uint16_t cap_poststore;
    uint8_t phydbg_mode;
    uint8_t cap_fsmstate;
    uint8_t cap_en_prestore;
    uint8_t cap_eventnum;
    uint8_t cap_stopontrig;
    uint32_t csr_capture_bank;
    uint32_t csr_tracer_capture;
    uint32_t csr_capture_trig_cond;
    uint32_t csr_capture_trig_mask_hi;
    uint32_t csr_capture_trig_pattern_hi;
    uint32_t csr_capture_trig_mask_mi;
    uint32_t csr_capture_trig_pattern_mi;
    uint32_t csr_capture_trig_mask_lo;
    uint32_t csr_capture_trig_pattern_lo;
    uint32_t csr_capture_stopontrig;
    uint32_t csr_adc_iq_selective;
    uint32_t csr_adc_sideband;
    uint32_t csr_adc_chain_mask;
    uint32_t csr_capture_pretrig;
    uint32_t csr_phydbg_spares;
    uint32_t module_wsi_ctrl[PHYDEVLIB_ADCCAPTURE_CTRL_SIZE];
    uint32_t module_rfcntl_ctrl[PHYDEVLIB_ADCCAPTURE_CTRL_SIZE];
    uint32_t module_tpc_ctrl[PHYDEVLIB_ADCCAPTURE_CTRL_SIZE];
    uint32_t module_cal_ctrl[PHYDEVLIB_ADCCAPTURE_CTRL_SIZE];
    uint32_t module_impcorr_ctrl[PHYDEVLIB_ADCCAPTURE_CTRL_SIZE];
    uint32_t module_mpi_ctrl[PHYDEVLIB_ADCCAPTURE_CTRL_SIZE];
    uint32_t module_fft_ctrl[PHYDEVLIB_ADCCAPTURE_CTRL_SIZE];
    uint32_t module_txtd_ctrl[PHYDEVLIB_ADCCAPTURE_CTRL_SIZE];
    uint32_t module_pmi_ctrl[PHYDEVLIB_ADCCAPTURE_CTRL_SIZE];
    uint32_t module_rxtd_ctrl[PHYDEVLIB_ADCCAPTURE_CTRL_SIZE];
    uint32_t module_demfront_ctrl[6];
    uint32_t module_pcss_ctrl[PHYDEVLIB_ADCCAPTURE_CTRL_SIZE];
    uint32_t module_txfd_ctrl[4];
    uint32_t module_robe_ctrl[9];
    uint32_t module_phynoc_ctrl[PHYDEVLIB_ADCCAPTURE_CTRL_SIZE];
    uint32_t csr_capture_destination;
} __ATTRIB_PACK PHYDEVLIB_ADCCAPTURE_INPUT;

typedef struct phydevlib_adccapture_output {
    uint16_t *dataI;
    uint16_t *dataQ;
    uint8_t status;
    uint8_t pad[3];
} __ATTRIB_PACK PHYDEVLIB_ADCCAPTURE_OUTPUT;

#define PHYDEVLIB_EVENTCAPTURE_CTRL_SIZE 2
typedef struct phydevlib_eventcapture_input {
    uint32_t config_or_dump;
    uint32_t clear_bank;
    uint32_t bank_number;
    uint32_t csr_phydbg_clockon;
    uint32_t csr_tracer_capture;
    uint32_t csr_capture_events_2banks;
    uint32_t csr_capture_destination;
    uint32_t csr_capture_mode;
    uint32_t csr_ftpg_bank;
    uint32_t csr_playback_bank;
    uint32_t csr_capture_bank;
    uint32_t csr_capture_trig_cond;
    uint32_t csr_capture_trig_mask_hi;
    uint32_t csr_capture_trig_pattern_hi;
    uint32_t csr_capture_trig_mask_mi;
    uint32_t csr_capture_trig_pattern_mi;
    uint32_t csr_capture_trig_mask_lo;
    uint32_t csr_capture_trig_pattern_lo;
    uint32_t csr_capture_event_mask;
    uint32_t csr_capture_bypass_ts_reordering;
    uint32_t csr_capture_stopontrig;
    uint32_t csr_adc_iq_selective;
    uint32_t csr_adc_sideband;
    uint32_t csr_adc_chain_mask;
    uint32_t csr_capture_pretrig;
    uint32_t module_wsi_ctrl[PHYDEVLIB_EVENTCAPTURE_CTRL_SIZE];
    uint32_t module_rfcntl_ctrl[PHYDEVLIB_EVENTCAPTURE_CTRL_SIZE];
    uint32_t module_tpc_ctrl[PHYDEVLIB_EVENTCAPTURE_CTRL_SIZE];
    uint32_t module_cal_ctrl[PHYDEVLIB_EVENTCAPTURE_CTRL_SIZE];
    uint32_t module_impcorr_ctrl[PHYDEVLIB_EVENTCAPTURE_CTRL_SIZE];
    uint32_t module_mpi_ctrl[PHYDEVLIB_EVENTCAPTURE_CTRL_SIZE];
    uint32_t module_fft_ctrl[PHYDEVLIB_EVENTCAPTURE_CTRL_SIZE];
    uint32_t module_txtd_ctrl[PHYDEVLIB_EVENTCAPTURE_CTRL_SIZE];
    uint32_t module_pmi_ctrl[PHYDEVLIB_EVENTCAPTURE_CTRL_SIZE];
    uint32_t module_rxtd_ctrl[PHYDEVLIB_EVENTCAPTURE_CTRL_SIZE];
    uint32_t module_demfront_ctrl[6];
    uint32_t module_pcss_ctrl[PHYDEVLIB_EVENTCAPTURE_CTRL_SIZE];
    uint32_t module_txfd_ctrl[4];
    uint32_t module_robe_ctrl[9];
    uint32_t module_phynoc_ctrl[PHYDEVLIB_EVENTCAPTURE_CTRL_SIZE];
    uint32_t phydbg_mode;
} __ATTRIB_PACK PHYDEVLIB_EVENTCAPTURE_INPUT;

typedef struct phydevlib_eventcapture_output {
    uint32_t *phyDbg_mem_L32;
    uint32_t *phyDbg_mem_U32;
} __ATTRIB_PACK PHYDEVLIB_EVENTCAPTURE_OUTPUT;

typedef struct phydevlib_tlvcapture_input {
    uint32_t tlvCaptureCtrl_cmdID;
    uint32_t tlvCaptureCtrl_01;
    uint32_t tlvCaptureCtrl_02;
    uint32_t tlvCaptureCtrl_03;
    uint32_t tlvCaptureCtrl_04;
} __ATTRIB_PACK PHYDEVLIB_TLVCAPTURE_INPUT;

typedef struct phydevlib_tlvcapture_output {
    uint32_t tlvCaptureCtrl_cmdID;
    uint32_t tlvCaptureCtrl_01;
    uint32_t tlvCaptureCtrl_02;
    uint32_t tlvCaptureCtrl_03;
    uint32_t tlvCaptureCtrl_04;
} __ATTRIB_PACK PHYDEVLIB_TLVCAPTURE_OUTPUT;

typedef struct phydevlib_dacplayback_input {
    uint8_t plyCount;
    uint8_t radarEn;
    uint8_t radarOn;
    uint8_t radarOff;
    uint8_t plybckEN;
    uint8_t pad[3];
    uint8_t data4k[4096];
    uint32_t playback_enable;
    uint32_t csr_playback_bank;
    uint32_t csr_playback_on_both_split_phys;
    uint32_t csr_playback_fifo_th;
    uint32_t csr_playback_loop_count;
    uint32_t csr_playback_mem_depth;
    uint32_t csr_playback_count_a;
    uint32_t isSysSimPlayback;
    uint32_t isPhydbgBankLoaded;
    uint32_t playbackChMask;
    uint32_t txGainIndex;
    uint32_t numberOfPlaybackPackets;
    uint32_t playbackPacketDuration[10];
    uint32_t playbackPacketStartDelay[10];
    uint32_t playbackTonefreqMHz;
} __ATTRIB_PACK PHYDEVLIB_DACPLAYBACK_INPUT;

typedef struct phydevlib_dacplayback_output {
    uint8_t status;
    uint8_t pad[3];
} __ATTRIB_PACK PHYDEVLIB_DACPLAYBACK_OUTPUT;

typedef struct phydevlib_pcsseventlogging_input {
    uint32_t cmdId;
    uint32_t pcssEventLoggingCtrl_01;
} __ATTRIB_PACK PHYDEVLIB_PCSSEVENTLOGGING_INPUT;

typedef struct phydevlib_pcsseventlogging_output {
    uint8_t status;
    uint8_t pad[3];
} __ATTRIB_PACK PHYDEVLIB_PCSSEVENTLOGGING_OUTPUT;

typedef struct phydevlib_txfdcapture_input {
    uint32_t cmdId;
} __ATTRIB_PACK PHYDEVLIB_TXFDCAPTURE_INPUT;

typedef struct phydevlib_txfdcapture_output {
    uint8_t status;
    uint8_t pad[3];
} __ATTRIB_PACK PHYDEVLIB_TXFDCAPTURE_OUTPUT;

typedef struct phydevlib_dfsvitest_input {
    uint32_t cmdId;
    uint32_t testParam;
    uint32_t contigs;
    uint32_t pri80Left;
    uint8_t detId;
    uint16_t pulseCount;
    uint8_t pad[1];
} __ATTRIB_PACK PHYDEVLIB_DFSVITEST_INPUT;

typedef struct phydevlib_dfsvitest_output {
    uint8_t status;
    uint8_t pad[3];
} __ATTRIB_PACK PHYDEVLIB_DFSVITEST_OUTPUT;

typedef struct phydevlib_rxtdcapture_input {
    uint32_t cmdId;
    uint32_t rxTDPhydbgSel;
    uint32_t rxTDCaptureCtrl_01;
} __ATTRIB_PACK PHYDEVLIB_RXTDCAPTURE_INPUT;

typedef struct phydevlib_rxtdcapture_output {
    uint8_t status;
    uint8_t pad[3];
} __ATTRIB_PACK PHYDEVLIB_RXTDCAPTURE_OUTPUT;

typedef struct dpd_lut_parms {
    uint32_t PAPRD_DATA[64];
    uint8_t paprdValidGain0;
    uint8_t paprdValidGain1;
    uint32_t paprdTableType;
    uint32_t paprdAdaptiveTableValid;
    uint16_t paprdSmSigGain;
} DPD_LUT;

typedef struct dpd_pkt_parms {
    uint8_t pktType;
    uint8_t mcs;
    uint16_t mpduLen;
    uint8_t nss;
    uint8_t ldpc;
    uint8_t stbc;
    uint8_t gi;
    uint8_t ampduBit;
    uint8_t paprdChMask;
    uint16_t ftpgCount;
    uint8_t ftpgDataType;
    uint8_t payloadSel;
    uint8_t pktBWCode;
    uint8_t dpd_en;
} DPD_PKT;

typedef struct phydevlib_dpdcal_input {
    uint8_t calCmdId;
    uint8_t hw_search_sq_timing;
    uint8_t dpdtrain_forced_sq_idx;
    uint8_t paprdType;

    uint32_t cur_tbl_idx;
    uint32_t force_gain_enable;
    uint8_t pktType;
    uint8_t mcs;
    uint16_t mpduLen;

    uint8_t nss;
    uint8_t ldpc;
    uint8_t stbc;
    uint8_t gi;

    uint8_t ampduBit;
    uint8_t paprdChMask;
    uint8_t dacGain;
    uint8_t gainIdx;

    uint16_t ftpgCount;
    uint8_t ftpgDataType;
    uint8_t payloadSel;

    uint8_t pktBWCode;
    uint8_t dpd_en;
    uint8_t glut_idx;
    uint8_t dpdtrain_timing_only;

    uint8_t chainIdx;
    uint8_t tableIdx;
    DPD_PKT paprdPkt;
    DPD_LUT paprdDataRestore;
    void *SendDpdTrainPkt;  // Q5 use only
    uint8_t dpdStartIndex;
    uint8_t dpdEndIndex;
    uint8_t *paprdCoeff;
    uint8_t *paprdCtrl;
    uint8_t *memPaprdCoeff;
    uint8_t *memPaprdCtrl;
} __ATTRIB_PACK PHYDEVLIB_DPDCAL_INPUT;

typedef struct phydevlib_dpdcal_output {
    uint8_t phyId;
    uint8_t SQ_IDX_SW;
    uint8_t timingDone;
    uint8_t trainingDone;
    uint8_t status;
    uint8_t pad[3];
    DPD_LUT paprdDataSave[DPD_CAL_TABLE_MAX];
    uint32_t *PAPRD_OUT_DATA;
} __ATTRIB_PACK PHYDEVLIB_DPDCAL_OUTPUT;

typedef struct phydevlib_itercombcal_input {
    uint8_t calMode;
    uint8_t dbStep;
    uint8_t pad[2];

    uint8_t adcOsrIdx;
    uint8_t dacOsrIdx;
    uint8_t chCal;
    uint8_t toneIdx;

    uint8_t clIter;
    uint8_t iqIter;
    uint8_t firstCalTone;
    uint8_t chain;

    uint8_t verbose;
    uint8_t DBS;
    uint8_t combine;
    uint8_t clLoc;

    int16_t calToneList[16];
    int16_t playBackTones[16];

    uint16_t isRunCalTx;
    uint8_t playBackLen;
    uint8_t numTxGains;
} __ATTRIB_PACK PHYDEVLIB_ITERCOMBCAL_INPUT;

typedef struct phydevlib_cfo_input {
    uint16_t weight_inst;
    uint8_t cfo_valid;
    uint32_t avg_cfo;
    uint8_t pad[1];
} __ATTRIB_PACK PHYDEVLIB_CFO_INPUT;

typedef struct phydevlib_cfo_output {
    uint16_t weight_inst;
    uint8_t cfo_valid;
    uint32_t avg_cfo;
    uint8_t pad[1];
} __ATTRIB_PACK PHYDEVLIB_CFO_OUTPUT;

typedef struct phydevlib_itercombcal_output {
    uint8_t phyId;
    uint8_t status;
    uint8_t numGainsCaled;
    uint8_t pad[1];
} __ATTRIB_PACK PHYDEVLIB_ITERCOMBCAL_OUTPUT;

typedef enum {
    DIRECTION_TX = 0,
    DIRECTION_RX,
    DIRECTION_LAST,
    DIRECTION_MAX = DIRECTION_LAST,
} FDMT_NUM_TAPS;

#define COMB_CAL_TYPE_TXIQ 0x1
#define COMB_CAL_TYPE_RXIQ 0x2
#define COMB_CAL_TYPE_CL   0x4

typedef struct phydevlib_fdmt_iqcal_input {
    uint8_t calCmdId;
    uint8_t combCalType;
    uint8_t chainIdx;
    uint8_t tableIdx;
    uint32_t cal_chain_mask;
    uint32_t cal_mode;
    uint32_t engineSharing;
    uint32_t driveTxGLUTIdx;
    uint32_t enableBlockAccAvg;
#if defined(PHYDEVLIB_PRODUCT_HAWKEYE) || defined(PHYDEVLIB_PRODUCT_HAWKEYE2) || defined(PHYDEVLIB_PRODUCT_CYPRESS) || \
    defined(PHYDEVLIB_PRODUCT_ALDER)
    uint32_t blockSize;
#else
    uint32_t blockSize[3];
#endif
    uint32_t dcEstWindowSize;
    uint32_t numOfBlocks;
    uint32_t numSamplesToReadback[3];
    uint32_t rxGainSettlingTime;
    uint32_t txGainSettlingTime;
    uint32_t txShiftSettlingTime;
    uint32_t calModeSettlingTime;
    uint32_t loopbackSettlingTime;
    uint32_t txResidueSettlingTime;
    uint32_t fixed_gain;
    // uint32_t    cal_type;
    uint32_t numTaps[DIRECTION_MAX];  // Tx taps @ 0, Rx taps @ 1

    // added for program_fdmt_iqcorr_coeffs
    uint32_t band;
    uint32_t mode;
    uint32_t calDirection;  // Tx=0, Rx=1
    uint32_t cps;
    uint32_t depth;
    uint32_t *cal_cl;
    uint32_t *cal_dig_cl;
    // uint32_t   *cal_dig_cl_i;//for HST HW1.3. Will remove in HW1.4
    // uint32_t   *cal_dig_cl_q;//for HST HW1.3. Will remove in HW1.4
    uint32_t *cal_rxcorr_mapping;
    uint32_t *cal_dtim_rxiq_coeff_i;
    uint32_t *cal_dtim_rxiq_coeff_q;
    uint32_t *cal_dpd_rxiq_coeff_i;
    uint32_t *cal_dpd_rxiq_coeff_q;
    // uint32_t   *cal_coeff;//for HST HW1.3. Will remove in HW1.4
    uint32_t *cal_coeff_l;  // for HST
    uint32_t *cal_coeff_u;  // for HST
    uint32_t *cal_coeff_i;
    uint32_t *cal_coeff_q;
    uint32_t *cal_dpd_rxiq_coeff;
    uint32_t progPriOrSec;
    uint8_t drmImpair;
    uint32_t coarseLoCal;
    uint32_t numTxGains;
    uint32_t numIters;
    uint32_t dgt_offset_i_val;
    uint8_t dis_by_bit;  // disable by bit in disable API
    uint8_t pad[2];
} __ATTRIB_PACK PHYDEVLIB_FDMT_IQCAL_INPUT;

typedef struct phydevlib_fdmt_iqcal_output {
    // uint32_t    cal_results[MAX_PHY][MAX_CAL_ENGINES][1024][2];//placeholder - will hold final post processing
    // results
    uint32_t *cal_cl;
    uint32_t *cal_rxcorr_mapping;
    uint32_t *cal_dig_cl;
    uint32_t *cal_coeff;  // for HST HW1.3. Will remove in HW1.4
    uint32_t *cal_coeff_l;
    uint32_t *cal_coeff_u;
    uint32_t *cal_dig_cl_i;  // for HST HW1.3. Will remove in HW1.4
    uint32_t *cal_dig_cl_q;  // for HST HW1.3. Will remove in HW1.4
    // uint32_t   *cal_dtim_rxiq_coeff_i;
    // uint32_t   *cal_dtim_rxiq_coeff_q;
    uint32_t *cal_dpd_rxiq_coeff_i;
    uint32_t *cal_dpd_rxiq_coeff_q;
    uint32_t *cal_coeff_i;
    uint32_t *cal_coeff_q;
    uint32_t *cal_dpd_rxiq_coeff;
} __ATTRIB_PACK PHYDEVLIB_FDMT_IQCAL_OUTPUT;

typedef struct phydevlib_pac_cal_input {
    uint8_t calCmdId;
    uint8_t pad[3];
    uint32_t cal_chain_mask;
} __ATTRIB_PACK PHYDEVLIB_PAC_CAL_INPUT;

typedef struct phydevlib_pac_cal_output {
    uint8_t pad[4];
} __ATTRIB_PACK PHYDEVLIB_PAC_CAL_OUTPUT;

typedef struct phydevlib_pdccal_input {
    uint8_t calCmdId;
    uint8_t pad[3];

    uint32_t cal_chain_mask;
    uint32_t pdc_delay_1;
    uint32_t pdc_delay_2;
    uint32_t pdc_delay_3;
    uint32_t pdc_thr_1;
    uint32_t pdc_thr_2;

    uint8_t num_block_1;
    uint8_t cal_step_size_1;
    uint8_t num_block_2;
    uint8_t cal_step_size_2;

    uint8_t curve_fitting_blocks;
    uint8_t curve_fitting_blocks2;
    uint8_t pad1[2];

    uint32_t rxGainSettlingTime;
    uint32_t txGainSettlingTime;
    uint32_t txShiftSettlingTime;
    uint32_t calModeSettlingTime;
    uint32_t loopbackSettlingTime;
    uint32_t txResidueSettlingTime;

    uint8_t p1NStep;
    uint8_t p1NMax;
    uint8_t p2NMax;
    uint8_t fixedGain;

    uint8_t fixedTxGain;
    uint8_t fixedTxGain1;
    uint8_t fixedRxGain;
    uint8_t numDumRun;

    uint32_t pdc_step_p1;
    uint32_t pdc_step_p2;

    uint32_t *cal_coeff;
    uint8_t chainIdx;
    uint8_t pad2[3];
} __ATTRIB_PACK PHYDEVLIB_PDCCAL_INPUT;

typedef struct phydevlib_pdccal_output {
    uint8_t calCmdId;  // todo dtu
    uint8_t pad[3];
    uint32_t cal_chain_mask;  // todo dtu

    uint32_t *cal_coeff;
    uint32_t pdc_step_p1;
    uint32_t pdc_step_p2;
    uint8_t p1NStep;
    uint8_t p1NMax;
    uint8_t p2NMax;
    uint8_t pad1[1];
} __ATTRIB_PACK PHYDEVLIB_PDCCAL_OUTPUT;

typedef struct tiadc_parameters_calresult {
    uint32_t dco_real;
    uint32_t dco_imag;
} __ATTRIB_PACK TIADC_PARAMETERS_CALRESULT[2 /* 5G and 6G */][2 /* Mode */][PHY_CHAIN_MAX];

typedef struct phydevlib_tiadc_cal_input {
    uint8_t calCmdId;
    uint32_t *pTiadcParametersCalResult;
    uint8_t tableIdx;
    uint8_t pad[2];
} __ATTRIB_PACK PHYDEVLIB_TIADC_INPUT;

typedef struct phydevlib_tiadc_cal_output {
    uint8_t dco_gain_match;
    uint32_t *pTiadcParametersCalResult;
    uint8_t pad[3];
} __ATTRIB_PACK PHYDEVLIB_TIADC_OUTPUT;

typedef struct phydevlib_adc_cal_input {
    uint8_t calCmdId;
    uint8_t pad[3];
} __ATTRIB_PACK PHYDEVLIB_ADC_INPUT;

typedef struct phydevlib_adc_cal_output {
    uint8_t dco_gain_match;
    uint8_t pad[3];  // place holder
} __ATTRIB_PACK PHYDEVLIB_ADC_OUTPUT;

typedef struct phydevlib_nfcal_input {
    int16_t *fixedNFCalPrimary;
    int16_t *fixedNFCalSecondary;
    uint8_t iterCount;
    uint8_t readCount;
    int16_t minNfLimit;
    uint32_t calFlags;
    uint32_t chainmask;
    uint32_t nfType;
    uint16_t size;
    uint8_t btTxDisableNf;
    uint8_t btRxDisableNf;
    void *ccaPwr;
    int8_t minccapwr_xlna_offset;
    uint8_t pad[3];
} __ATTRIB_PACK PHYDEVLIB_NFCAL_INPUT;

typedef struct phydevlib_nfcal_output {
    uint32_t *nfCalResults;
    void *ccaPwr;
} __ATTRIB_PACK PHYDEVLIB_NFCAL_OUTPUT;

typedef struct phydevlib_dbgdump_input {
    uint8_t burst_mode;
    uint8_t is_32_64_bit;
    uint8_t mode;
    uint8_t bank_number;
} __ATTRIB_PACK PHYDEVLIB_DBGDUMP_INPUT;

typedef struct phydevlib_chainmask_input {
    uint8_t isPPSmode;
    uint8_t aDFSon;
    uint8_t rxChMask;
    uint8_t pad[1];
} __ATTRIB_PACK PHYDEVLIB_CHAINMASK_INPUT;

typedef struct phydevlib_xbar_input {
    uint8_t isPPSmode;
    uint8_t pad[3];
} __ATTRIB_PACK PHYDEVLIB_XBAR_INPUT;

typedef struct phydevlib_xbar_output {
    uint32_t xbarVal;
} __ATTRIB_PACK PHYDEVLIB_XBAR_OUTPUT;

typedef struct phydevlib_adccal_input {
    uint32_t cal_mode;
} __ATTRIB_PACK PHYDEVLIB_ADCCAL_INPUT;

typedef struct phydevlib_adccal_output {
    uint8_t pad[4];
} __ATTRIB_PACK PHYDEVLIB_ADCCAL_OUTPUT;

typedef struct phydevlib_rxgaincal_input {
    uint8_t chainIdx;
    uint8_t dcEstLen;
    uint8_t calTonegenFreq;
    uint8_t calCorrelationLen;
} __ATTRIB_PACK PHYDEVLIB_RXGAINCAL_INPUT;

typedef struct phydevlib_rxgcaincal_output {
    uint8_t status;
    uint8_t band;
    int8_t refISS;
    uint8_t rate;
    uint8_t bandWidth;
    uint8_t chanIdx;
    uint8_t chainIdx;
    uint16_t numPackets;
    uint8_t pad[3];
} __ATTRIB_PACK PHYDEVLIB_RXGAINCAL_OUTPUT;

typedef struct phydevlib_dac_bo_input {
    uint8_t dacBOFlags;
    uint8_t maxDacBOCCK;
    uint8_t minDacBOCCK;
    uint8_t maxDacBOQam[NUM_BO_QAM];
    uint8_t minDacBOQam[NUM_BO_QAM];
    uint8_t maxDacBOQamMu[NUM_BO_QAM];
    uint8_t minDacBOQamMu[NUM_BO_QAM];
    uint8_t maxDacBOSingleRU;
    uint8_t minDacBOSingleRU;
    uint8_t pad[3];
} __ATTRIB_PACK PHYDEVLIB_DAC_BO_INPUT;

typedef struct phydevlib_glut_modes_input {
    int16_t targetPowerLevel[3];
    uint8_t ofdmHighMCSRange[4];
    uint8_t ofdmLowMCSRange[4];
    uint8_t cckRange[4];
    uint8_t singleRuRange[4];
    uint8_t numEntires;
    uint8_t glutInitMcsthr;
    uint8_t progGlutModes;
    int16_t targetPowerLevel_160M[3];
} __ATTRIB_PACK PHYDEVLIB_GLUT_MODES_INPUT;

typedef struct phydevlib_pdetcal_input {
    uint8_t PdetLowGain[PHY_CHAIN_MAX];
    uint8_t PdetHighGain[PHY_CHAIN_MAX];
    uint8_t CalValid[PHY_CHAIN_MAX];
    uint8_t PdetLowGain_Idac2[PHY_CHAIN_MAX];
    uint8_t PdetHighGain_Idac2[PHY_CHAIN_MAX];
} __ATTRIB_PACK PHYDEVLIB_PDETCAL_INPUT;

typedef struct phydevlib_settpc_input {
    uint8_t calCmdId;
    // Bits to enable/disable the forced modes
    uint8_t forceTargetPower;  // Selects which mode to be used - MPI target Power or Forced value ?
    // Gets highest precedence - overrides all other flags below
    // uint8_t     forceDACGain;       // Equal precedence as txGainIdx - can co-exist
    // uint8_t     forceTxGainIdx;     // Equal precedence as txGainIdx - can co-exist
    // uint8_t     forceCalTableIdx;   // Is valid if txGainIdx = 1
    uint8_t pad[2];

    uint8_t gainIdx;  // Forced Gain Mode
    int8_t dacGain;   // Forced Gain Mode
    uint8_t paCfg;
    int8_t pdetAttn;

    uint8_t clpcMode;  // Select which Mode to be run
    uint8_t glutUpdate;
    uint8_t plutUpdate;
    uint8_t readTpcStatus;  // Will help in reading the status alone without any configuration

    int32_t tempComp;
    uint16_t powerRange;  // Indicates the range of powers that are under test (range 0,1,2,3 from the register map for
                          // GLUT indexing; val 4 = all)
    uint16_t pktMCSType;  // this parameter tells which type of pkt/mcs is indicated (0 = All ; 1 = CCK; 2 = OFDMLOWMCS;
                          // 3 = OFDMHIGHMCS; 4 = BTCOEX+DPD)
    // This is used to reset the LUT entries partially for debug/validation purposes

    uint32_t forcedTargetPower;  // If (force_target_power == 1), register is updated with the forced_target_power
    // One value for all chains
    // uint32_t    forcedDACGain[8];   // If forceDACGain == 1, register is updated with the  forcedDACGain (cannot
    // overload dacGain as this is per chain value)
    // One value per chain
    // uint32_t    forcedTxGainIdx[8]; // If forceDACGain == 1, register is updated with the  forcedDACGain
    // One value per chain
    // uint32_t    forcedCalTableIdx;  // THIS item is TBD TBD TBD - after discussing with Wincent/Hao-Ren
    // commenting out the above forced modes - a separate forced mode call is established for the same.  - madhkuma
    uint32_t *pCalData;
    uint32_t *pTiaData;

    uint8_t dupModeBDFEnable;
    uint8_t minMaxMethod;
    uint8_t clpcErrCmnDupEn;
    uint8_t tgtPwrClpcThrDb4;

    uint8_t doCalTPC;
    uint8_t pad2[3];

    PHYDEVLIB_DAC_BO_INPUT *pDacBoConfig;
    PHYDEVLIB_PDETCAL_INPUT *pdetcalInput;
    uint32_t *adc_comp_ref_hi_band;
    uint32_t *adc_comp_ref_low_band;

    uint8_t *bq_band;
    uint8_t *sq_lg;
    uint8_t *sq_hg;
    uint8_t *tia_lg;

    uint8_t *tia_hg;
    uint8_t pad3[3];
} __ATTRIB_PACK PHYDEVLIB_SETTPC_INPUT;

typedef struct phydevlib_settpc_output {
    uint32_t *pCalData;
} __ATTRIB_PACK PHYDEVLIB_SETTPC_OUTPUT;

typedef struct phydevlib_fixrxgain_input {
    uint16_t gain;
    uint16_t table;
    uint8_t fix_or_release;
    uint8_t pad[3];
} __ATTRIB_PACK PHYDEVLIB_FIXRXGAIN_INPUT;

typedef struct phydevlib_calinfodump_output {
    uint32_t calInfoMemL32[4][960];
    uint32_t calInfoMemU32[4][960];
    uint8_t num_engines;
    uint8_t pad[3];
} __ATTRIB_PACK PHYDEVLIB_CALINFODUMP_OUTPUT;

typedef struct phydevlib_forcedgainmode_output {
    uint8_t phyId;
    uint8_t status;
    uint8_t pad[2];
} __ATTRIB_PACK PHYDEVLIB_FORCEDGAINMODE_OUTPUT;

typedef struct phydevlib_forcedgainmode_input {
    uint8_t phyId;
    uint8_t forceTargetPower;
    int8_t forcedTargetPower;
    uint8_t forceDacGain;
    int8_t forcedDacGain[8];
    uint8_t forceTxGainIdx;
    uint8_t forcedTxGainIdx[8];
    uint8_t txChMask;
    uint32_t forcedGLUTIdxValues[8];
    uint32_t forceGLUTIdx;
    uint8_t pad[2];
} __ATTRIB_PACK PHYDEVLIB_FORCEDGAINMODE_INPUT;

typedef struct phydevlib_alut_input {
    uint8_t targetPowerdBm[32];  // Positive dBm values (5.0u)
    int8_t lutGainDig[32];       // Attenuation corresponding to the dBm target power values in 5.3s dB format
    uint8_t numEntires;          // Indicates the number of entires (out of max 32) that are to be written/readout
    uint8_t chainIndex;          // 0 - 7
    uint8_t channelIndex;        // 0-Pri80 1-FCS 2-Sec80
    uint8_t pad[1];
} __ATTRIB_PACK PHYDEVLIB_ALUT_INPUT;

typedef struct phydevlib_plut_input {
    int8_t lutGainDig[256];  // Power (dBm) relative to Attenuation applied on input signal corresponding to the dBm
                             // target power values in 5.3s format
    uint8_t numEntires;      // Indicates the number of entires (out of max 256) that are to be written/readout
    uint8_t chainIndex;      // 0 - 7
    uint8_t channelIndex;    // 0-Pri80 1-FCS 2-Sec80
    uint8_t pad[1];
} __ATTRIB_PACK PHYDEVLIB_PLUT_INPUT;

typedef struct phydevlib_glut_input {
    int16_t lutPowerMeasured[16];  // Power (dBm) corresponding to the gain set (Tx gain index, min DAC gain, clpc
                                   // error) specified for each entry 5.3s format
    uint8_t lutTxGainIndex[16];    // one of 64 possible values for IRON chip gain setting.
    int8_t lutMinDacGainCal[16];   // Calculated by HALPHY code, gives the min value of each GLUT entry from measured
                                   // power (5.3s dB)
    int8_t lutCLPCError[16];
    uint8_t numEntires;    // Indicates the number of entires (out of max 16) that are to be written/readout
    uint8_t chainIndex;    // 0 - 7
    uint8_t channelIndex;  // 0-Pri80 1-FCS 2-Sec80
    int8_t dacGainCal;
    int16_t minTxPwr[16];
    int16_t maxTxPwr;
    uint8_t progGlutModes;
    uint8_t glutInitMcsthr;
    PHYDEVLIB_GLUT_MODES_INPUT pGlutModeParams;
    int8_t plutOffset;
    uint8_t plutOffsetGainIdx;
    uint8_t pad[2];
} __ATTRIB_PACK PHYDEVLIB_GLUT_INPUT;

typedef struct phydevlib_clk_switch_input {
    uint32_t gpio2gRst;
    uint32_t gpio5gRst;
    uint32_t gpioMuxSel;
    uint32_t pllMode;
    uint32_t testCtrl;
} __ATTRIB_PACK PHYDEVLIB_CLKSWITCH_INPUT;

typedef struct phydevlib_clk_switch_output {
    uint8_t PhyBPllLocked;
    uint8_t ClkSwitchStatus;
    uint8_t pad[2];
} __ATTRIB_PACK PHYDEVLIB_CLKSWITCH_OUTPUT;

/*
 * Boot Sequence API Params
 */
#define PHYDEVLIB_BOOTSEQ_CLOCK       0x00000001
#define PHYDEVLIB_BOOTSEQ_INI         0x00000002
#define PHYDEVLIB_BOOTSEQ_PCSS        0x00000004
#define PHYDEVLIB_BOOTSEQ_PHYM3_UCODE 0x00000008

#define PHYDEVLIB_BOOTSEQ_PHYM3_UCODE_DMA 0x80000000

#define PHYDEVLIB_BOOT_SEQ_ALL \
    (PHYDEVLIB_BOOTSEQ_CLOCK | PHYDEVLIB_BOOTSEQ_INI | PHYDEVLIB_BOOTSEQ_PCSS | PHYDEVLIB_BOOTSEQ_PHYM3_UCODE)

typedef struct phydevlib_boot_seq_input {
    uint8_t bandCodePhyA0;
    uint8_t bandCodePhyA1;
    uint8_t bandCodePhyB;
    uint8_t typePhyA0;
    uint8_t typePhyA1;
    uint8_t typePhyB;
    uint32_t boardId;
    uint32_t flags;
    uint32_t wlanMask;
    uint8_t pad[2];
} __ATTRIB_PACK PHYDEVLIB_BOOTSEQ_INPUT;

typedef struct phydevlib_boot_seq_output {
    uint8_t status;
    uint8_t pad[3];
} __ATTRIB_PACK PHYDEVLIB_BOOTSEQ_OUTPUT;

typedef struct phydevlib_memdump_input {
    uint32_t startAddr;
    uint32_t stopAddr;
} __ATTRIB_PACK PHYDEVLIB_MEMDUMP_INPUT;

typedef struct phydevlib_phym3_binary_segment {
    uint8_t *data_buf;
    uint32_t data_size;
    uint32_t mem_size;
    uint32_t offset;
} __ATTRIB_PACK PHYDEVLIB_PHYM3_BINARY_SEGMENT;

typedef struct phydevlib_phym3_binary {
    PHYDEVLIB_PHYM3_BINARY_SEGMENT *segs;
    uint8_t segnum;
    uint8_t pad[3];
} __ATTRIB_PACK PHYDEVLIB_PHYM3_BINARY;

typedef struct phydevlib_param_data {
    union {
        uint32_t wlan_base;
        uint32_t wcss_env;
        char *wcss_version;
        PHYDEVLIB_PHYM3_BINARY phym3_binary;
        uint32_t regpoll_disable : 1;
        struct {
            uint32_t size;
            void *ptr;
            uint32_t version;
        } bdfinfo;
        void *mmap_get_base_ptr;
        void *mmap_get_phy_base_ptr;
    } u;
} __ATTRIB_PACK PHYDEVLIB_PARAM_DATA;

typedef struct phydevlib_phym3_binary_input {
    uint32_t phyId : 8, phym3_binary_segment_num;
    PHYDEVLIB_PHYM3_BINARY_SEGMENT *phym3_binary_segments;
} __ATTRIB_PACK PHYDEVLIB_PHYM3_BINARY_INPUT;

typedef struct phydevlib_spur_mitigation_input {
    uint32_t spurFreqPriChn0[6];
    uint32_t spurFreqPriChn1[6];
    uint32_t spurFreqPriChn2[6];
    uint32_t spurFreqPriChn3[6];
    uint32_t spurFreqExtChn0[6];
    uint32_t spurFreqExtChn1[6];
    uint32_t spurFreqExtChn2[6];
    uint32_t spurFreqExtChn3[6];
    uint32_t param1;
    uint32_t param2;
    uint32_t param3;
    uint32_t param4;
    uint32_t param5;
    int16_t tempIndegC5G[2];
    int16_t tempIndegC2G[2];
    uint32_t spurCoeff[8];
} __ATTRIB_PACK PHYDEVLIB_SPURMITIGATION_INPUT;

typedef struct phydevlib_dcm_input {
    uint32_t dcmTestType;
    uint32_t dcmBase;
    uint32_t dcmCmd;
    uint32_t dcmModuleIdx;
    uint32_t chainMask;
    uint32_t freq;
    uint32_t gainHi;
    uint32_t gainLo;
    uint32_t channelType;
    uint32_t ddrSignalStartAddr;
    uint32_t ddrSignalStopAddr;
    uint32_t ddrSignalRptCount;
} __ATTRIB_PACK PHYDEVLIB_DCM_INPUT;

typedef struct phydevlib_prog_rfa_input {
    uint8_t enable_ldocal_otp;
    uint8_t gainCtrlMode;
    uint8_t gainIdx;
    uint8_t pad[1];
} __ATTRIB_PACK PHYDEVLIB_PROGRFA_INPUT;

typedef struct phydevlib_debugphyrf_input {
    uint32_t param1;
    uint32_t param2;
    uint32_t param3;
    uint8_t dbgctrl;
    uint8_t pad[3];
} __ATTRIB_PACK PHYDEVLIB_DEBUGPHYRF_INPUT;

typedef struct phydevlib_debugphyrf_output {
    uint8_t status;
    uint8_t pad[3];
} __ATTRIB_PACK PHYDEVLIB_DEBUGPHYRF_OUTPUT;

typedef struct phydevlib_pal_input {
    uint8_t calCmdId;
    uint8_t pcalMethod;
    uint8_t pcalOptDoSetEDandDM;
    uint8_t pad[1];
    uint32_t *pPalDlyVals;
} __ATTRIB_PACK PHYDEVLIB_PAL_INPUT;

typedef struct phydevlib_pal_output {
    uint32_t *pPalDlyVals;
} __ATTRIB_PACK PHYDEVLIB_PAL_OUTPUT;

typedef struct phydevlib_tx_convergence_input {
    uint32_t state;
} __ATTRIB_PACK PHYDEVLIB_TX_CONVERGENCE_INPUT;

typedef struct phydevlib_tx_convergence_output {
    uint32_t *tx_converged;
} __ATTRIB_PACK PHYDEVLIB_TX_CONVERGENCE_OUTPUT;

typedef struct phydevlib_getpdlversion_output {
    uint8_t majorVer;
    uint8_t minorVer;
    uint8_t pad[2];
} __ATTRIB_PACK PHYDEVLIB_GETPDLVERSION_OUTPUT;

typedef struct phydevlib_getrdlversion_output {
    uint8_t majorVer;
    uint8_t minorVer;
    uint8_t pad[2];
} __ATTRIB_PACK PHYDEVLIB_GETRDLVERSION_OUTPUT;

typedef struct phydevlib_loadM3_input {
    uint32_t flags;
} __ATTRIB_PACK PHYDEVLIB_LOADM3_INPUT;

typedef struct phydevlib_loadM3_output {
    uint8_t status;
    uint8_t pad[3];
} __ATTRIB_PACK PHYDEVLIB_LOADM3_OUTPUT;

typedef struct phydevlib_lna_input {
    uint32_t value;
} __ATTRIB_PACK PHYDEVLIB_LNA_INPUT;

typedef struct phydevlib_lna_output {
    uint32_t value;
} __ATTRIB_PACK PHYDEVLIB_LNA_OUTPUT;

typedef struct phydevlib_rtt_input {
    uint8_t enaFixedStrChain;
    uint8_t fixedStrChainIdx; /* Pri80 */
    uint8_t fixedStrChainIdxExt80;
    uint8_t enaRttPerBurst;
    uint8_t enaRttPerFrame;
    uint8_t enaContChanCapture;
    uint8_t pad[2];
} __ATTRIB_PACK PHYDEVLIB_RTT_INPUT;

typedef struct phydevlib_rtt_output {
    uint32_t status;
} __ATTRIB_PACK PHYDEVLIB_RTT_OUTPUT;

typedef struct phydevlib_cfrcircap_input {
    uint32_t detId;
    uint8_t enableRcc;
    uint8_t enableChanCap;
    uint8_t enableCfr;
    uint8_t enableCir;
    uint8_t enableCfrPerFrame;
    uint8_t enableCirPerFrame;
    uint8_t pad[2];
} __ATTRIB_PACK PHYDEVLIB_CFRCIRCAP_INPUT;

typedef struct phydevlib_cfrcircap_output {
    uint32_t status;
} __ATTRIB_PACK PHYDEVLIB_CFRCIRCAP_OUTPUT;

typedef struct phydevlib_sscan_input {
    uint16_t ss_nf_ref;
    uint16_t ss_period;
    uint16_t ss_count;
    uint8_t ss_ena;
    uint8_t ss_fft_chn_sel;
    uint8_t ss_fft_size;
    uint8_t ss_wb_rpt_mode;
    uint8_t ss_rssi_rpt_mode;
    uint32_t ss_update_mask;
    uint8_t ss_update_config;
    uint8_t ss_bin_scale;
    uint8_t ss_agile;
    uint8_t ss_priority;
    uint8_t ss_pwr_format;
    uint8_t ss_dbm_adj;
    uint8_t ss_en_nf_fix;
    uint8_t pad;
} __ATTRIB_PACK PHYDEVLIB_SSCAN_INPUT;

typedef struct phydevlib_sscan_output {
    uint32_t status;
} __ATTRIB_PACK PHYDEVLIB_SSCAN_OUTPUT;

typedef struct phydevlib_ru26disabletx_input {
    uint32_t disable;
} __ATTRIB_PACK PHYDEVLIB_RU26DISABLETX_INPUT;

typedef struct phydevlib_m3txbf_input {
    uint32_t cb;
    uint32_t ng;
    uint32_t nc;
    uint32_t nr;
} __ATTRIB_PACK PHYDEVLIB_M3TXBF_INPUT;

typedef struct phydevlib_m3txbf_output {
    uint32_t cb;
    uint32_t ng;
    uint32_t nc;
    uint32_t nr;
} __ATTRIB_PACK PHYDEVLIB_M3TXBF_OUTPUT;

typedef struct phydevlib_latest_accumulated_clpc_error_input {
    uint8_t chainIdx;
    uint8_t pad[3];
} __ATTRIB_PACK PHYDEVLIB_LATEST_ACCUMULATED_CLPC_ERROR_INPUT;

typedef struct phydevlib_latest_accumulated_clpc_error_output {
    uint16_t *latestAccumulatedClpcError;
} __ATTRIB_PACK PHYDEVLIB_LATEST_ACCUMULATED_CLPC_ERROR_OUTPUT;

typedef struct phydevlib_latest_therm_value_input {
    int16_t latestThermValue;
    int8_t ppmOffset_At30C;
    int8_t ppmOffset_At85C;
    int8_t ppmOffset_At100C;
    uint8_t pad[3];
} __ATTRIB_PACK PHYDEVLIB_LATEST_THERM_VALUE_INPUT;

typedef struct phydevlib_latest_therm_value_output {
    uint8_t *latestThermValue;
} __ATTRIB_PACK PHYDEVLIB_LATEST_THERM_VALUE_OUTPUT;

typedef struct phydevlib_paprd_warm_packet_support_output {
    uint8_t *paprdWarmPacketSupport;
} __ATTRIB_PACK PHYDEVLIB_PAPRD_WARM_PACKET_SUPPORT_OUTPUT;

typedef struct phylib_rssi_db_to_dbm_input {
    int16_t rxTempMeas;         // temperature on measurement; from bdf
    int16_t rxNFThermCalSlope;  // NF cal slope; from bdf
    PHYDEVLIB_LATEST_THERM_VALUE_INPUT *pLatestThermValueInput;
} __ATTRIB_PACK PHYDEVLIB_RSSI_DB_TO_DBM_INPUT;

typedef struct phydevlib_spectral_shaping_select_input {
    uint8_t spectralShapingSelect;
} __ATTRIB_PACK PHYDEVLIB_SPECTRAL_SHAPING_SELECT_INPUT;

typedef struct phydevlib_olpc_temp_comp_input {
    uint8_t *thermCalValue;
    uint8_t *alphaTherm;
    uint8_t thermGainErrMax;
    uint8_t isScanCh;
    int8_t *scanGlutOffset;
    uint8_t pad[2];
} __ATTRIB_PACK PHYDEVLIB_OLPC_TEMP_COMP_INPUT;

typedef struct phydevlib_tpc_olpc_ctrl_input {
    uint32_t tpc_spare;
} __ATTRIB_PACK PHYDEVLIB_TPC_OLPC_CTRL_INPUT;

typedef struct phydevlib_olpc_temp_update_input {
    uint8_t *currThermValue;
    uint8_t *thermCalValue;
    uint8_t *alphaTherm;
    uint8_t thermGainErrMax;
    int16_t latestThermValue;
    uint8_t *alphaThermSlope1;
    uint8_t *alphaThermThr1;
    uint8_t pad[1];
} __ATTRIB_PACK PHYDEVLIB_OLPC_TEMP_UPDATE_INPUT;

typedef struct phydevlib_xo_cdacin_input {
    uint32_t value;
} __ATTRIB_PACK PHYDEVLIB_XO_CDACIN_INPUT;

typedef struct phydevlib_xo_cdacin_output {
    uint32_t value;
} __ATTRIB_PACK PHYDEVLIB_XO_CDACIN_OUTPUT;

typedef struct phydevlib_xo_cdacout_input {
    uint32_t value;
} __ATTRIB_PACK PHYDEVLIB_XO_CDACOUT_INPUT;

typedef struct phydevlib_xo_cdacout_output {
    uint32_t value;
} __ATTRIB_PACK PHYDEVLIB_XO_CDACOUT_OUTPUT;

typedef struct phydevlib_pps_mode_output {
    uint32_t isValid;
} __ATTRIB_PACK PHYDEVLIB_PPS_MODE_OUTPUT;

typedef struct phydevlib_adfs_input {
    uint32_t enableAdfs;
    uint32_t skipVreg;
    uint32_t sbsPhySel;
} __ATTRIB_PACK PHYDEVLIB_ADFS_INPUT;

typedef struct phydevlib_dynsmps_input {
    uint8_t dynSmpsEn;
    uint8_t pad[3];
} __ATTRIB_PACK PHYDEVLIB_DYNSMPS_INPUT;

typedef struct phydevlib_dynsmps_output {
    uint8_t dynSmpsEn;
    uint8_t pad[3];
} __ATTRIB_PACK PHYDEVLIB_DYNSMPS_OUTPUT;

typedef struct phydevlib_twoPoint_input {
    uint8_t chain;
    uint8_t overrideAvgPwrAtPdadc;  // set if overriding by caller input value
    int8_t AvgPwrAtPdadc;           // override values for the input chain and channel
    uint8_t pad[1];
} __ATTRIB_PACK PHYDEVLIB_TWOPOINT_INPUT;

typedef struct phydevlib_twoPoint_output {
    int8_t pwrOffest;
    int8_t pwrAtPdadc;
    uint8_t gotPwrAtPdadc;
    uint8_t pad[1];
} __ATTRIB_PACK PHYDEVLIB_TWOPOINT_OUTPUT;

typedef struct phydevlib_rxbbf_cal_data {
    uint32_t vcm_code;
    uint32_t tia_cfb_code[4];  // based on bandcode...
    uint32_t bq_code[23];      // based on bandcode...
    uint32_t bq_cfb_code;
    uint32_t tia;
    uint32_t txrxlo;
    uint32_t ch_bw_ac;
    uint32_t done;
} __ATTRIB_PACK PHYDEVLIB_RXBBF_CAL_DATA;

typedef struct phydevlib_rxbbf_cal_input {
    uint8_t calCmdId;
    uint8_t acrSetting;
    uint8_t pad[2];
    PHYDEVLIB_RXBBF_CAL_DATA *pRxBbfCalData;
} __ATTRIB_PACK PHYDEVLIB_RXBBF_CAL_INPUT;

typedef struct phydevlib_rxbbf_cal_output {
    PHYDEVLIB_RXBBF_CAL_DATA *pRxBbfCalData;
} __ATTRIB_PACK PHYDEVLIB_RXBBF_CAL_OUTPUT;

typedef struct phydevlib_txbbf_cal_data {
    uint32_t txbb_lut_ov;
    uint32_t txfe_lut_ov;
    uint32_t txrxlo;
    uint32_t ch_bw_ac[4];
    uint32_t rtune;
    uint32_t done;
} __ATTRIB_PACK PHYDEVLIB_TXBBF_CAL_DATA;

typedef struct phydevlib_txbbf_cal_input {
    uint8_t calCmdId;
    uint8_t pad[3];
    PHYDEVLIB_TXBBF_CAL_DATA *pTxBbfCalData;
} __ATTRIB_PACK PHYDEVLIB_TXBBF_CAL_INPUT;

typedef struct phydevlib_txbbf_cal_output {
    PHYDEVLIB_TXBBF_CAL_DATA *pTxBbfCalData;
} __ATTRIB_PACK PHYDEVLIB_TXBBF_CAL_OUTPUT;

typedef struct phydevlib_ibfcal_input {
    uint32_t calMode;  // 0:AOA, 1:IBF, 2:AOA & IBF, 3: IBF Restore
    uint16_t *pBdfIbfData[RXDCO_MAX_CHAIN];
    uint8_t numChains;
    uint16_t *calData;
    uint8_t pad[3];
} __ATTRIB_PACK PHYDEVLIB_IBFCAL_INPUT;

typedef struct phydevlib_ibfcal_output {
    uint16_t *calData;
    uint32_t dataSize;  // AOA:4, IBF:3 or 7
} __ATTRIB_PACK PHYDEVLIB_IBFCAL_OUTPUT;

typedef struct phydevlib_dac_cal_input {
    uint8_t calCmdId;
    uint8_t pad[3];
    uint8_t *pDACCalData;
} __ATTRIB_PACK PHYDEVLIB_DAC_CAL_INPUT;

typedef struct phydevlib_dac_cal_output {
    uint8_t *pDACCalData;
} __ATTRIB_PACK PHYDEVLIB_DAC_CAL_OUTPUT;

typedef struct phydevlib_ocl_input {
    uint8_t enable;
    uint8_t chainIdx;
    uint8_t gainBackoff;
    uint8_t settlingTime;
} __ATTRIB_PACK PHYDEVLIB_OCL_INPUT;

typedef struct phydevlib_vreg_ftm_mode_input {
    uint32_t modeValue;
} __ATTRIB_PACK PHYDEVLIB_VREG_FTM_MODE_INPUT;

typedef struct phydevlib_pbs_input {
    uint32_t pbsMode;
    uint8_t enRxFdNonHtDupComb;
    uint8_t pad[3];
} __ATTRIB_PACK PHYDEVLIB_PBS_INPUT;

typedef struct phydevlib_ps_mas_input {
    uint32_t psMask;
} __ATTRIB_PACK PHYDEVLIB_PS_MASK_INPUT;

typedef struct phydevlib_disable_rfa_input {
    uint8_t disableFlag;
    uint8_t pad[3];
} __ATTRIB_PACK PHYDEVLIB_DISABLE_RFA_INPUT;

typedef struct phydevlib_rfa_turn_on_off_input {
    uint8_t turnOn;
    uint8_t pad[3];
} __ATTRIB_PACK PHYDEVLIB_RFA_TURN_ON_OFF_INPUT;

typedef struct phydevlib_rfa_save_restore_regs_input {
    uint32_t chainMask;
    uint32_t *regArray;
    uint8_t cmd;
    uint8_t pad[3];
} __ATTRIB_PACK PHYDEVLIB_RFA_SAVE_RESTORE_REGS_INPUT;

typedef struct phydevlib_pdet_pdadc_packet_input {
    uint16_t chainMask;
    uint8_t pad[2];
} __ATTRIB_PACK PHYDEVLIB_PDET_PDADC_PACKET_INPUT;

typedef struct phydevlib_override_release_pdet_attn_input {
    uint8_t clpcMode;
    uint8_t attn;
    uint8_t ovRel;
    uint8_t pad[1];
} __ATTRIB_PACK PHYDEVLIB_OVERRIDE_RELEASE_PDET_ATTN_INPUT;

typedef struct phydevlib_paprd_hw_enable_input {
    uint8_t targetPowerMax;
    uint8_t targetPowerMin;
    uint8_t pad[2];
} __ATTRIB_PACK PHYDEVLIB_PAPRD_HW_ENABLE_INPUT;

typedef struct phydevlib_paprd_get_complete_output {
    uint8_t completeCode;
    uint8_t pad[3];
} __ATTRIB_PACK PHYDEVLIB_PAPRD_GET_COMPLETE_OUTPUT;

typedef struct phydevlib_paprd_read_glut_input {
    uint8_t chainNum;
    uint8_t tpcIndex;
    uint8_t pad[2];
} __ATTRIB_PACK PHYDEVLIB_PAPRD_READ_GLUT_INPUT;

typedef struct phydevlib_paprd_read_glut_output {
    uint16_t txgainIdxCal;
    uint8_t pad[2];
} __ATTRIB_PACK PHYDEVLIB_PAPRD_READ_GLUT_OUTPUT;

typedef struct phydevlib_paprd_get_temp_scaling_input {
    uint32_t slope;
    uint32_t delta;
} __ATTRIB_PACK PHYDEVLIB_PAPRD_GET_TEMP_SCALING_INPUT;

typedef struct phydevlib_paprd_get_temp_scaling_output {
    uint32_t *scaling;
} __ATTRIB_PACK PHYDEVLIB_PAPRD_GET_TEMP_SCALING_OUTPUT;

typedef struct phydevlib_paprd_config_temp_scaling_input {
    uint32_t scaling;
} __ATTRIB_PACK PHYDEVLIB_PAPRD_CONFIG_TEMP_SCALING_INPUT;

typedef struct phydevlib_rfa_chip_id_output {
    uint32_t family_number;
    uint32_t device_number;
    uint32_t major_version;
    uint32_t minor_version;
} __ATTRIB_PACK PHYDEVLIB_RFA_CHIP_ID_OUTPUT;

typedef struct phydevlib_svs_input {
    uint32_t value;
} __ATTRIB_PACK PHYDEVLIB_SVS_INPUT;

typedef struct phydevlib_bt_lp_xo_input {
    uint8_t clkSwitch;
    uint8_t pad[3];
} __ATTRIB_PACK PHYDEVLIB_BT_LP_XO_INPUT;

typedef struct phydevlib_wlan_xo_input {
    uint8_t forcedResetValue;
    uint8_t pad[3];
} __ATTRIB_PACK PHYDEVLIB_WLAN_XO_INPUT;

typedef struct phydevlib_set_channel_input {
    uint8_t set_synth;
    uint8_t pad[3];
} __ATTRIB_PACK PHYDEVLIB_SET_CHANNEL_INPUT;

typedef struct phydevlib_clpc_input {
    uint8_t phyId;
    uint8_t bwCode;
    uint8_t txChMask;
    uint8_t rxChMask;
    uint8_t lgEnable;
    uint8_t clpcMode;
    uint8_t cfgRevert;
    uint8_t pad[1];
} __ATTRIB_PACK PHYDEVLIB_CLPC_INPUT;

typedef struct phydevlib_clpc_output {
    uint8_t status;
    uint8_t pad[3];
} __ATTRIB_PACK PHYDEVLIB_CLPC_OUTPUT;

typedef struct phydevlib_rxdeaf_input {
    uint8_t rxDeaf_enable;
    uint8_t pad[3];
} __ATTRIB_PACK PHYDEVLIB_RXDEAF_INPUT;

typedef struct phydevlib_rstcal_input {
    uint8_t calCmdId;
    uint8_t rstDir;
    uint8_t pad[2];
    uint32_t cal_chain_mask;
    uint32_t calTxGain;
    uint32_t forcedRxGainIdx;
    int32_t calDacGain;
} __ATTRIB_PACK PHYDEVLIB_RSTCAL_INPUT;

typedef struct phydevlib_rstcal_output {
    uint32_t cal_chain_mask;
    uint8_t calCmdId;
    int32_t RSSI;
    uint8_t pad[3];
} __ATTRIB_PACK PHYDEVLIB_RSTCAL_OUTPUT;

typedef struct phydevlib_retention_input {
    uint8_t save;
    uint8_t mask;
    uint8_t pad[2];
} __ATTRIB_PACK PHYDEVLIB_RETENTION_INPUT;

typedef struct phydevlib_cal_retention_input {
    uint8_t chain_mask;
    uint8_t index;
    uint8_t group;
    uint8_t save;
    uint32_t *RRI_ptr_BBF;
} __ATTRIB_PACK PHYDEVLIB_CAL_RETENTION_INPUT;

typedef struct phydevlib_rri_input {
    uint8_t save;
    uint32_t RFA_PHY0_size;
    uint32_t RFA_PHY1_size;
    uint32_t *RFA_PHY0_bufferPtr;
    uint32_t *RFA_PHY1_bufferPtr;
    uint8_t pad[3];
} __ATTRIB_PACK PHYDEVLIB_RRI_INPUT;

typedef struct phydevlib_rri_output {
    uint32_t *RFA_PHY0_bufferPtr;
    uint32_t *RFA_PHY1_bufferPtr;
} __ATTRIB_PACK PHYDEVLIB_RRI_OUTPUT;

/*
typedef struct phydevlib_rxdeaf_output
{
    uint32_t    status;
} __ATTRIB_PACK PHYDEVLIB_RXDEAF_OUTPUT;
*/

typedef struct phydevlib_paprd_edpd_ctrl_input {
    uint8_t save;
    uint8_t *eDPDCtrl;
    uint8_t chain_index;
    uint8_t pad[1];
} __ATTRIB_PACK PHYDEVLIB_PAPRD_EDPD_CTRL_INPUT;

#ifndef WHAL_NUM_POINTS_TO_MEAS_EXT
#define WHAL_NUM_POINTS_TO_MEAS_EXT 64
#endif

typedef struct phydevlib_cal_plut_offset_input {
    uint8_t gainIdx[WHAL_NUM_POINTS_TO_MEAS_EXT];
    int16_t measPwr[WHAL_NUM_POINTS_TO_MEAS_EXT];
    uint8_t pdadc[WHAL_NUM_POINTS_TO_MEAS_EXT];
    uint8_t plutCalTxGainThresh;
    int32_t HighPwrIdx;
    uint8_t pad[3];
} __ATTRIB_PACK PHYDEVLIB_CAL_PLUT_OFFSET_INPUT;

typedef struct phydevlib_cal_plut_offset_output {
    int8_t plutOffset;
    uint8_t pad[3];
} __ATTRIB_PACK PHYDEVLIB_CAL_PLUT_OFFSET_OUTPUT;

typedef struct phydevlib_m3_input {
    uint8_t full;
    uint8_t pad[3];
} __ATTRIB_PACK PHYDEVLIB_M3_INPUT;

typedef struct phydevlib_m3_output {
    uint8_t status;
    uint8_t pad[3];
} __ATTRIB_PACK PHYDEVLIB_M3_OUTPUT;

typedef struct phydevlib_agc_input {
    uint32_t ccaThr;
    uint32_t ccaThrA;
    uint32_t ccaThrB;
    uint32_t ccaThrC;
} __ATTRIB_PACK PHYDEVLIB_AGC_INPUT;

typedef struct phydevlib_vreg_input {
    uint32_t flags;  // bit 0: rxState; bit1:swar_power_surge_drop; bit2: BT_COEX_CONTROL; bit3: detectorConfigRadar
                     // Each bit indicates if the setting is needed or not: 0 = ignore; 1 = set
    uint32_t rxDisableState;
    uint32_t swarPowerSurgeDrop;
    uint32_t btCoexControl;
    uint32_t detectorConfigRadar;
    uint32_t *commonTable;
    uint32_t *modalTable;
    uint8_t commonTableSize;
    uint8_t modalTableSize;
    uint8_t isSave;
    uint8_t isSet;
    uint8_t staMode;
    uint8_t phyMode;
    uint8_t pwrMode;
    uint8_t phyId;
    uint8_t pad[3];
    uint8_t commonTableSizeExt;
    uint32_t *commonTableExt;
} __ATTRIB_PACK PHYDEVLIB_VREG_INPUT;

typedef struct phydevlib_vreg_output {
    uint32_t *commonTable;
    uint32_t *modalTable;
    uint32_t *commonTableExt;
    uint8_t staMode;
    uint8_t pad[3];
} __ATTRIB_PACK PHYDEVLIB_VREG_OUTPUT;

typedef struct phydevlib_forced_syn_input {
    uint8_t forceSyn;
    uint8_t forceSynVal;
    uint8_t softReset;
    uint8_t pad[1];
} __ATTRIB_PACK PHYDEVLIB_FORCED_SYN_INPUT;

typedef struct phydevlib_rx_stats_output {
    uint32_t totalPackets;
    uint32_t goodPackets;
} __ATTRIB_PACK PHYDEVLIB_RX_STATS_OUTPUT;

typedef struct phydevlib_mu_user_pos_input {
    uint32_t groupId;
    uint32_t userPos;
} __ATTRIB_PACK PHYDEVLIB_MU_USER_POS_INPUT;

typedef struct phydevlib_assoc_id_input {
    uint8_t isSet;
    uint16_t pId;
    uint8_t pad[1];
} __ATTRIB_PACK PHYDEVLIB_ASSOC_ID_INPUT;

typedef struct phydevlib_assoc_id_output {
    uint16_t pId;
    uint8_t pad[2];
} __ATTRIB_PACK PHYDEVLIB_ASSOC_ID_OUTPUT;

typedef struct phydevlib_sniffer_mode_input {
    uint8_t isSet;
    uint8_t enable;
    uint8_t pad[2];
} __ATTRIB_PACK PHYDEVLIB_SNIFFER_MODE_INPUT;

typedef struct phydevlib_sniffer_mode_output {
    uint8_t enable;
    uint8_t pad[3];
} __ATTRIB_PACK PHYDEVLIB_SNIFFER_MODE_OUTPUT;

typedef struct phydevlib_phy_state_output {
    uint8_t phyOn;
    uint8_t pad[3];
} __ATTRIB_PACK PHYDEVLIB_PHY_STATE_OUTPUT;

typedef struct phydevlib_dss_war_cfo_input {
    uint32_t threshold;
    uint8_t enable;
    uint8_t pad[3];
} __ATTRIB_PACK PHYDEVLIB_DSS_WAR_CFO_INPUT;

typedef union phydevlib_pa_mute_state_output {
    uint32_t pa_mute_5g_ch3_latch : 1, pa_mute_5g_ch2_latch : 1, pa_mute_5g_ch1_latch : 1, pa_mute_5g_ch0_latch : 1,
        pa_mute_2g_ch1_latch : 1, pa_mute_2g_ch0_latch : 1, wl_xfem_ctrl_pa_mute : 1;
    uint32_t pa_mute_state_u32;
} __ATTRIB_PACK PHYDEVLIB_PA_MUTE_STATE_OUTPUT;

typedef struct phydevlib_cxm_input {
    uint32_t val;
    uint8_t scale;
    uint8_t ovsVal;
    uint16_t step;
    uint8_t isSet;
    uint8_t type;
    uint8_t chIdx;
    uint8_t pad[1];
} __ATTRIB_PACK PHYDEVLIB_CXM_INPUT;

typedef struct phydevlib_cxm_output {
    uint32_t val;
    uint16_t step;
    uint8_t wlanCtrlEnCh0Ovs;
    uint8_t btCtrlSlnaLCh0Ovs;
} __ATTRIB_PACK PHYDEVLIB_CXM_OUTPUT;

typedef struct phydevlib_xtal_cal_input {
    uint8_t enableFlag;
    uint16_t capIn;
    uint16_t capOut;
    uint16_t txChMask;
    uint16_t gainIdx;
    uint8_t pad[3];
} __ATTRIB_PACK PHYDEVLIB_XTAL_CAL_INPUT;

typedef struct phydevlib_xtal_cal_output {
    uint16_t capIn;
    uint16_t capOut;
} __ATTRIB_PACK PHYDEVLIB_XTAL_CAL_OUTPUT;

typedef struct phydevlib_ani_input {
    uint32_t phyBase;
    uint32_t enable;
    int32_t level;
    uint32_t type;
    uint32_t val;
    uint32_t edCcaa;
    uint32_t edCcab;
    uint32_t edCcac;
    uint8_t rssiThr1aDb;
    uint8_t rssiThr1bDb;
    uint8_t rssiThr1cDb;
    uint8_t rssiThr1dDb;
    uint8_t rssiGtThr1aEn;
    uint8_t rssiGtThr1bEn;
    uint8_t rssiGtThr1cEn;
    uint8_t rssiLtThr1dEn;
} __ATTRIB_PACK PHYDEVLIB_ANI_INPUT;

typedef struct phydevlib_ani_output {
    uint32_t val;
    uint32_t edCcaa;
    uint32_t edCcab;
    uint32_t edCcac;
    uint32_t isUseXlna;
    uint32_t isBypassXlnaInListen;
} __ATTRIB_PACK PHYDEVLIB_ANI_OUTPUT;

typedef struct phydevlib_efuse_read_input {
    uint32_t Qfprom_address;
} __ATTRIB_PACK PHYDEVLIB_EFUSE_READ_INPUT;

typedef struct phydevlib_efuse_read_output {
    uint32_t value;
} __ATTRIB_PACK PHYDEVLIB_EFUSE_READ_OUTPUT;

typedef struct phydevlib_efuse_input {
    uint32_t offset;
    uint16_t numBytes;
    uint8_t *buf;
    uint8_t pad[2];
} __ATTRIB_PACK PHYDEVLIB_EFUSE_INPUT;

typedef struct phydevlib_efuse_output {
    uint32_t errCode;
    uint8_t *buf;
} __ATTRIB_PACK PHYDEVLIB_EFUSE_OUTPUT;

typedef struct phydevlib_ss_input {
    uint8_t isFtm;
    uint8_t isRead;
    uint8_t isActive;
    uint8_t fftSize;
    uint8_t priority;
    uint8_t restartEna;
    int8_t noiseFloorRef;
    uint8_t initDelay;
    uint8_t nbToneThr;
    uint8_t strBinThr;
    uint8_t wbRptMode;
    uint8_t rssiRptMode;
    int8_t rssiThr;
    uint8_t pwrFormat;
    uint8_t rptMode;
    uint8_t binScale;
    uint8_t dBmAdj;
    uint8_t chnMask;
    uint16_t count;
    uint16_t period;
    uint8_t scanMode;
    uint8_t scanEna;
    uint32_t updateMask;
    uint32_t performed;
    uint32_t intrptsSent;
} __ATTRIB_PACK PHYDEVLIB_SS_INPUT;

typedef struct phydevlib_ss_output {
    uint8_t fftSize;
    uint8_t priority;
    uint8_t restartEna;
    int8_t noiseFloorRef;
    uint8_t initDelay;
    uint8_t nbToneThr;
    uint8_t strBinThr;
    uint8_t wbRptMode;
    uint8_t rssiRptMode;
    int8_t rssiThr;
    uint8_t pwrFormat;
    uint8_t rptMode;
    uint8_t binScale;
    uint8_t dBmAdj;
    uint8_t chnMask;
    uint16_t count;
    uint16_t period;
    uint8_t scanMode;
    uint8_t scanEna;
    uint32_t updateMask;
    uint32_t readyIntrpt;
    uint32_t performed;
    uint32_t intrptsSent;
    uint32_t pendingCount;
    uint8_t isEnabled;
    uint8_t pad[2];
} __ATTRIB_PACK PHYDEVLIB_SS_OUTPUT;

typedef struct phydevlib_pcss_input {
    uint32_t errMsgSize;
    uint32_t val;
    uint32_t type;
    uint8_t enable;
    uint8_t feature;
    uint8_t isGet;
    uint8_t isMM;

    uint8_t rssi;
    uint8_t cf_active;
    uint8_t iuSsrInProgress;
    uint8_t obssPDthreshold;

    uint8_t bsscolor;
    uint8_t phyId;
    uint8_t pad[1];
    uint8_t swlogLevel;
    uint32_t swlogPayload;
} __ATTRIB_PACK PHYDEVLIB_PCSS_INPUT;

typedef struct phydevlib_pcss_output {
    uint32_t status;
    uint32_t isReset;
    uint16_t branchId;
    uint16_t buildId;
    uint32_t ucodeVersion;
    uint8_t *errMsg;
    uint8_t cf_active;
    uint8_t async_reset_reason;
    uint8_t pad[2];
} __ATTRIB_PACK PHYDEVLIB_PCSS_OUTPUT;

typedef struct phydevlib_dbg_input {
    uint16_t chainMask;
    uint8_t gainIdx;
    uint8_t numPhy;
    uint8_t phyDisable;
    uint8_t tracerDisable;
    uint16_t offset;
    uint32_t enable;
    uint32_t type;
    uint32_t *buff;
    uint32_t csr_capture_bypass_ts_reordering;
    uint32_t csr_capture_em;
    uint32_t csr_capture_events_2banks;
    uint32_t csr_capture_mode;
    uint32_t csr_capture_pretrig;
    uint32_t csr_capture_stopontrig;
    uint32_t csr_phydbg_spares;
    uint32_t csr_adc_iq_selective;
    uint32_t csr_adc_sideband;
    uint32_t csr_adc_chainmask;
    uint32_t csr_tracer_capture;
    uint32_t csr_capture_bank;
    uint32_t csr_ftpg_bank;
    uint32_t csr_playback_bank;
    uint32_t rxtd_phydbg_sel;
    uint32_t trigger_cond;
    uint32_t trigger_pattern_hi;
    uint32_t trigger_pattern_lo;
    uint32_t trigger_mask_hi;
    uint32_t trigger_mask_lo;
    uint32_t csr_playback_en;
    uint32_t csr_playback_fifo_th;
    uint32_t csr_playback_trigger;
    uint32_t csr_playback_loop_count;
    uint32_t csr_playback_mem_depth;
    uint32_t csr_playback_count_b;
    uint32_t csr_playback_count_a;
    uint32_t txGainIndex;
    uint32_t *module_ctrl;
    uint32_t *phydbg_dmac_config;
    uint32_t phydbg_noc_atben;
    void *phydbg_noc_trace;
    uint8_t dumpNow;
    uint8_t bank_mask;
    uint16_t *data;
    uint16_t count;
} __ATTRIB_PACK PHYDEVLIB_DBG_INPUT;

typedef struct phydevlib_dbg_output {
    uint16_t capEn;
    uint16_t waitTime;
    uint8_t *tx_iq_idx;
    uint8_t *glut_idx;
    uint8_t *txgain_idx;
    uint8_t *dac_gain;
    uint8_t *tgt_pwr;
    uint16_t *accum_clpc_err;
    uint16_t *clpc_err;
    uint16_t *meas_pwr_out;
    uint32_t *tpc_gen_ctrl_l;
    uint32_t *tpc_gen_ctrl_u;
    uint32_t *tpc_clpc_ctrl_0_l;
    uint32_t *tpc_clpc_ctrl_0_u;
    uint32_t *tpc_olpc_ctrl_l;
    int32_t rssi;
    uint32_t *buff;
} __ATTRIB_PACK PHYDEVLIB_DBG_OUTPUT;

typedef struct phydevlib_tpc_input {
    uint8_t dumpMask;
    uint8_t clpc_mode;
    uint16_t chainMask;
    uint8_t attn;
    uint8_t ov_rel;
    uint8_t pad[2];
} __ATTRIB_PACK PHYDEVLIB_TPC_INPUT;

typedef struct phydevlib_tpc_output {
    uint32_t isOLPC;
    uint8_t *avg;
} __ATTRIB_PACK PHYDEVLIB_TPC_OUTPUT;

typedef struct phydevlib_prog_reg_input {
    uint32_t deweightConfig;
    uint32_t val;
    uint8_t isSet;
    uint8_t name;
    uint8_t chainNum;
    uint8_t pktBw;
    uint8_t isPri;
    uint8_t deltaFromNoisefloor;
    uint8_t enable;
    int8_t clpcPowerOffset_cck;
    int8_t clpcPowerOffset_ofdm20;
    int8_t clpcPowerOffset_ofdm40;
    int8_t clpcPowerOffset_ofdm80;
    int8_t clpcPowerOffset_ofdm160;
    int8_t clpcPowerOffset_ofdm20_hc;
    int8_t clpcPowerOffset_ofdm40_hc;
    int8_t clpcPowerOffset_ofdm80_hc;
    int8_t clpcPowerOffset_ofdm160_hc;
    int8_t clpcPowerOffset_ofdm240;
    int8_t clpcPowerOffset_ofdm320;
    int8_t clpcPowerOffset_ofdm20_cfr;
    int8_t clpcPowerOffset_ofdm40_cfr;
    int8_t clpcPowerOffset_ofdm80_cfr;
    int8_t clpcPowerOffset_ofdm160_cfr;
    int8_t clpcPowerOffset_ofdm240_cfr;
    int8_t clpcPowerOffset_ofdm320_cfr;
    int8_t poweroffset_cck;
    uint8_t poweroffset_mcs_thr_high;
    uint8_t poweroffset_mcs_thr_mid;
    int8_t power_offset_extended_range_su;
    int8_t poweroffset_ofdm_low_mcs;
    int8_t poweroffset_ofdm_mid_mcs;
    int8_t poweroffset_ofdm_high_mcs;
    uint8_t pad[1];
} __ATTRIB_PACK PHYDEVLIB_PROG_REG_INPUT;

typedef struct phydevlib_prog_reg_output {
    uint32_t val;
} __ATTRIB_PACK PHYDEVLIB_PROG_REG_OUTPUT;

typedef struct phydevlib_fem_input {
    uint32_t xfemCntrlA0_0;
    uint32_t xfemCntrlA0_1;
    uint32_t xfemCntrl1A0_0;
    uint32_t xfemCntrl1A0_1;
    uint32_t xfemCntrlB_0;
    uint32_t xfemCntrlB_1;
    uint32_t xfemCntrl1B_0;
    uint32_t xfemCntrl1B_1;
} __ATTRIB_PACK PHYDEVLIB_FEM_INPUT;

typedef struct phydevlib_warm_reset_output {
    uint32_t warmResetWSIB0RegReadVal;
    uint32_t warmResetWSIB1RegReadVal;
    uint32_t warmResetBusErrStatusB0RegReadVal;
    uint32_t warmResetBusErrStatusB1RegReadVal;
    uint32_t warmResetBusStatusB0RegReadVal;
    uint32_t warmResetBusStatusB1RegReadVal;
} __ATTRIB_PACK PHYDEVLIB_WARM_RESET_OUTPUT;

typedef struct phydevlib_dfs_input {
    uint32_t radarSubbandMask;
    uint8_t isSet;
    uint8_t useDET1;
    uint8_t pad[2];
} __ATTRIB_PACK PHYDEVLIB_DFS_INPUT;

typedef struct phydevlib_dfs_output {
    uint32_t *vReg;
    uint32_t *hwReg;
    uint32_t radarSubbandMask;
} __ATTRIB_PACK PHYDEVLIB_DFS_OUTPUT;

typedef struct phydevlib_cckfir_ctrl_input {
    uint8_t cckFirSettings;
    uint8_t cebEnable;
    uint8_t pad[2];
} __ATTRIB_PACK PHYDEVLIB_CCKFIR_CTRL_INPUT;

typedef struct phydevlib_rfa_input {
    uint8_t enable;
    uint8_t chainMask;
    uint8_t pad[2];
} __ATTRIB_PACK PHYDEVLIB_RFA_INPUT;

typedef struct phydevlib_rfa_output {
    uint32_t *regArray;
} __ATTRIB_PACK PHYDEVLIB_RFA_OUTPUT;

typedef struct phydevlib_rx_desense_input {
    uint8_t rssiThr1aDb;
    uint8_t rssiThr1bDb;
    uint8_t rssiThr1cDb;
    uint8_t rssiThr1dDb;
    uint8_t rssiGtThr1aEn;
    uint8_t rssiGtThr1bEn;
    uint8_t rssiGtThr1cEn;
    uint8_t rssiLtThr1dEn;
    uint16_t adcsatCntHigh;
    uint16_t adcsatCntLow;
    uint8_t courseHighDb2;
    uint8_t minCcaFirPwrThrDb2;
    uint8_t minCcaRelPwrThrDb2;
    uint8_t enbl_rfsat_cmds_0;
    uint8_t enbl_rfsat_cmds_1;
    uint8_t enbl_rfsat_cmds_2;
    uint8_t enbl_rfsat_cmds_3;
    uint8_t isSet;
    uint32_t vreg_val;
    uint32_t rx11bDetCorrThr;
    uint32_t kpri_pwr;
    uint32_t rx11b_det_corr_tfest_thr;
    uint32_t rbapb_rbhpc_ign_sig;
    uint32_t rbapb_rbhpc_ign_svc;
    uint32_t rbapb_rbhpc_ign_crc;
    uint32_t start_gain_off_db2;
    uint8_t gtc_gain_table_force_mode;
    uint8_t gtc_gain_table_force_value;
    uint8_t gtc_default_gain_table_min;
    uint8_t gtc_default_gain_table_max;
} __ATTRIB_PACK PHYDEVLIB_RX_DESENSE_INPUT;

typedef struct phydevlib_rx_desense_output {
    uint8_t rssiThr1aDb;
    uint8_t rssiThr1bDb;
    uint8_t rssiThr1cDb;
    uint8_t rssiThr1dDb;
    uint8_t rssiGtThr1aEn;
    uint8_t rssiGtThr1bEn;
    uint8_t rssiGtThr1cEn;
    uint8_t rssiLtThr1dEn;
    uint16_t adcsatCntHigh;
    uint16_t adcsatCntLow;
    uint8_t courseHighDb2;
    uint8_t minCcaFirPwrThrDb2;
    uint8_t minCcaRelPwrThrDb2;
    uint8_t enbl_rfsat_cmds_0;
    uint8_t enbl_rfsat_cmds_1;
    uint8_t enbl_rfsat_cmds_2;
    uint8_t enbl_rfsat_cmds_3;
    uint32_t vreg_val;
    uint32_t rx11bDetCorrThr;
    uint32_t kpri_pwr;
    uint32_t rx11b_det_corr_tfest_thr;
    uint32_t rbapb_rbhpc_ign_sig;
    uint32_t rbapb_rbhpc_ign_svc;
    uint32_t rbapb_rbhpc_ign_crc;
    uint32_t start_gain_off_db2;
    uint8_t gtc_gain_table_force_mode;
    uint8_t gtc_gain_table_force_value;
    uint8_t gtc_default_gain_table_min;
    uint8_t gtc_default_gain_table_max;
    uint8_t pad[1];
} __ATTRIB_PACK PHYDEVLIB_RX_DESENSE_OUTPUT;

typedef struct phydevlib_icic_cal_input {
    uint8_t calCmdId;
    // capture point
    uint8_t capture_point;

    // rx gain params
    uint8_t forced_rx_gain;
    uint8_t forced_table_index;

    // icic digital parameters
    uint16_t delay_val;
    uint16_t gain;
    uint8_t enable;
    uint8_t pherr_polarity;
    uint16_t rshftval;

    // icic iir parameters
    int16_t a11;
    int16_t a12;
    int16_t a22;
    int16_t v1;
    int16_t v2;
    uint8_t pad[2];
} __ATTRIB_PACK PHYDEVLIB_ICIC_CAL_INPUT;

typedef struct phydevlib_icic_cal_output {
    // icic digital parameters
    uint8_t enable;
    uint8_t pherr_polarity;
    uint16_t rshftval;

    // icic iir parameters
    int16_t a11;
    int16_t a12;
    int16_t a22;
    int16_t v1;
    int16_t v2;
    uint8_t pad[2];
} __ATTRIB_PACK PHYDEVLIB_ICIC_CAL_OUTPUT;

typedef struct phydevlib_rxspur_cal_input {
    uint8_t calCmdId;
    uint8_t pad[3];
} __ATTRIB_PACK PHYDEVLIB_RXSPUR_CAL_INPUT;

typedef struct phydevlib_rxspur_cal_output {
    uint8_t pad[4];
} __ATTRIB_PACK PHYDEVLIB_RXSPUR_CAL_OUTPUT;

typedef struct phydevlib_debug_cmd_input {
    uint32_t num_args;
    uint32_t *args;
} __ATTRIB_PACK PHYDEVLIB_DEBUG_CMD_INPUT;

typedef struct phydevlib_coex_input {
    uint8_t isSet;
    uint8_t semaphoreId;
    uint8_t pad[2];
} __ATTRIB_PACK PHYDEVLIB_COEX_INPUT;

typedef struct phydevlib_cgim_input {
    uint8_t enable;
} __ATTRIB_PACK PHYDEVLIB_CGIM_INPUT;

typedef struct {
    uint32_t stop : 8;
    uint32_t skip : 8;
    uint32_t rsvd : 16;
} PHYDEVLIB_DEBUGMON_CMD;

typedef struct {
    uint32_t stopped : 16;
    uint32_t skipped : 8;
    uint32_t rsvd : 8;
} PHYDEVLIB_DEBUGMON_RSP;

typedef struct {
    uint8_t enable;          // feature enable flag
    uint8_t phyId;           // indicate which PHY is being monitored
    uint8_t rsvd[2];         // reserved fields
    uint32_t funcCnt;        // counter which is used for counting the occurrence of the monitored function
    void *funcAddr;          // func addr set by debug monitor to inform PDL about which function is now being monitored
    void *funcPrivParam[3];  // pointers used for storing address of PDL function private parameters
    PHYDEVLIB_DEBUGMON_CMD cmd;  // flags set by debug monitor to request an operation to specified PDL function
    PHYDEVLIB_DEBUGMON_RSP rsp;  // flags set by PDL exec controller to inform debug monitor about actions taken
} PHYDEVLIB_DEBUGMON_INTERFACE;

typedef struct phydevlib_switch_clpc_input {
    uint8_t enableCLPC;
    uint8_t pad[3];
} __ATTRIB_PACK PHYDEVLIB_SWITCH_CLPC_INPUT;

typedef struct phydevlib_set_tpc_input {
    uint8_t forcedTargetPower;
    uint8_t pad[3];
} __ATTRIB_PACK PHYDEVLIB_SET_TPC_INPUT;

/*
   this common structure maps calibration function's input parameter
   it assumes that the first parameter in cal input is always calCmdId
*/
typedef struct phydevlib_cal_input_common_mapping {
    uint8_t calCmdId;
    uint8_t pad[3];
} __ATTRIB_PACK PHYDEVLIB_CAL_INPUT_COMMON_MAPPING;
#endif /* IOT */

#endif /* _PHY_DEVLIB_API_PARM_H */
