/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _PHY_DEVLIB_API_PARM_IOT_H
#define _PHY_DEVLIB_API_PARM_IOT_H
#include <stdbool.h>
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
 *  devLib/phyDevLibApiParmIot.h
 *      The definition of input/output parameter for phy dev lib APIs needed by the user s/w for IOT product.
 *
 */

typedef struct phydevlib_phy_input {
    uint8_t phyId;  // keep for common code.
    uint8_t bandCode;
    uint16_t mhz; /* primary 20 mhz channel frequency in mhz */
    uint8_t extLNA;
    uint8_t fixedSharedLnaIndex4CRx;
    uint8_t loadIniMask;
    uint16_t calMask;  // this is used in Q5 only
    uint8_t phyDfsEnMask;
    int8_t rssiDbToDbmOffset;
    uint16_t aDfsSynthFreq;
    uint32_t phyBase;  // keep for common code.
    uint8_t bwCode;
} __ATTRIB_PACK PHYDEVLIB_PHY_INPUT;

typedef struct phydevlib_reset_input {
    uint8_t gainIdx;
    int8_t dacGain;
    // uint8_t     fixedSharedLnaIndex4CRx;
    uint8_t tpcMode;  // 0: forced gain mode, 1: SCPC, 2: CLPC
    uint32_t userParm1;
    // uint32_t	userParm2;
    int8_t txPowerMode;
    int8_t enableXLNA;  // 0: disable, 1: enable xLna (support for 5G/6G only)
    int8_t enableCRx;   // 0: disable, 1: enable current Rx with BT, so for 2G only.
    // uint8_t     channelIndex;
    int8_t txPowerOffset;  // target_power = demanded_power + txPowerOffset. (in 0.5dB unit)
    uint8_t lnaPwrMode;    // select internal LNA bias either HP or LP mode.
    uint8_t enableXPA;     // enable xPA for Neutrino mode
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
    // uint8_t     mcs;
    uint8_t enable;
    uint8_t pad[2];
} __ATTRIB_PACK PHYDEVLIB_FTPGRX_INPUT;

typedef struct phydevlib_ftpgrx_output {
    uint16_t numPkts;  // used to indicate RSSI of received packet.
    // uint16_t    rxTimeOutSecs;
    // uint8_t     bandCode;
    // uint8_t     pktType;
    // uint8_t     pktBWCode;
    // uint8_t     mcs;
    // uint16_t    freq;
    uint16_t crcChkCnt;
    uint16_t crcPassCnt;
    uint8_t status;
    uint8_t pad[1];
} __ATTRIB_PACK PHYDEVLIB_FTPGRX_OUTPUT;

typedef struct phydevlib_ftpgtx_input {
    uint8_t pktType;  // 0 - 5

    uint8_t gi; /* can take value of up to 6 to support new LTF + GI combinations  */

    uint32_t mpduLen; /* support larger payload sizes */
    uint32_t mcs;     /* needs to be 32-bits to support OFDMA-DL */
    uint32_t tpcParamsI;
    uint16_t ftpgCount;  // num pkt
    uint8_t txPwrShared;
    uint8_t pad[1];
} __ATTRIB_PACK PHYDEVLIB_FTPGTX_INPUT;

typedef struct phydevlib_ftpgtx_output {
    uint8_t status;
    uint8_t pad[3];
} __ATTRIB_PACK PHYDEVLIB_FTPGTX_OUTPUT;

typedef struct PHYDEVLIB_TPCCLPCSETTING_INPUT {
    uint8_t tx_power_mode;
    bool fixed_gain_mode;
    bool capture_pdet;
    bool clpc_on;
    int dig_gain;
    // int dac_gain_list[20];
    int pwr_offset;
} __ATTRIB_PACK PHYDEVLIB_TPCCLPCSETTING_INPUT;

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

typedef struct phydevlib_dpdcal_input {
    uint8_t calCmdId;
    uint8_t gainIdx;
    uint8_t dpd_en;
    uint8_t chainIdx;
    //    uint8_t     channelIdx;
    uint8_t tx_power_mode;  // tx_power_mode_t tx_power_mode;
#if 0
    uint8_t     hw_search_sq_timing;
    uint8_t     dpdtrain_forced_sq_idx;
    uint8_t     paprdType;

    uint32_t    cur_tbl_idx;
    uint32_t    force_gain_enable;
    uint8_t     pktType;
    uint8_t     mcs;
    uint16_t    mpduLen;

    uint8_t     nss;
    uint8_t     ldpc;
    uint8_t     stbc;
    uint8_t     gi;

    uint8_t     ampduBit;
    uint8_t     paprdChMask;
    uint8_t     dacGain;

    uint16_t    ftpgCount;
    uint8_t     ftpgDataType;
    uint8_t     payloadSel;

    uint8_t     pktBWCode;

    uint8_t     glut_idx;
    uint8_t     dpdtrain_timing_only;

    uint8_t     tableIdx;
    DPD_PKT     paprdPkt;
    DPD_LUT     paprdDataRestore;
    void* SendDpdTrainPkt; // Q5 use only
    uint8_t     dpdStartIndex;
    uint8_t     dpdEndIndex;
    uint8_t* paprdCoeff;
    uint8_t* paprdCtrl;
    uint8_t* memPaprdCoeff;
    uint8_t* memPaprdCtrl;
#endif
} __ATTRIB_PACK PHYDEVLIB_DPDCAL_INPUT;

typedef struct phydevlib_dpdcal_output {
    uint8_t trainingDone;
    uint8_t status;
    uint8_t pad[3];
#if 0
    uint8_t     phyId;
    uint8_t     SQ_IDX_SW;
    uint8_t     timingDone;

    DPD_LUT     paprdDataSave[DPD_CAL_TABLE_MAX];
    uint32_t* PAPRD_OUT_DATA;
#endif
} __ATTRIB_PACK PHYDEVLIB_DPDCAL_OUTPUT;

typedef struct phydevlib_rttcal_input {
    uint8_t rtt_en;
    uint8_t digital_loopback;
} __ATTRIB_PACK PHYDEVLIB_RTTCAL_INPUT;

typedef struct phydevlib_rttcal_output {
    uint8_t status;
    uint8_t calCmdId;
    uint32_t rtt_delay;
} __ATTRIB_PACK PHYDEVLIB_RTTCAL_OUTPUT;

#define RXDCO_NUM_LUT_ENTRY    256
#define RXDCO_MAX_NUM_CAL_GAIN 15
typedef struct dco_lut {
    int16_t LUT_Imag[RXDCO_MAX_CHAIN][RXDCO_NUM_LUT_ENTRY];
    int16_t LUT_Real[RXDCO_MAX_CHAIN][RXDCO_NUM_LUT_ENTRY];
    int16_t Range;
    int16_t pad;
} __ATTRIB_PACK DCO_LUT;

typedef struct dco_lut_new {
    uint16_t LUT_Imag : 9, Range_Bit0 : 1, Rsvd0 : 6;
    uint16_t LUT_Real : 9, Range_Bit1 : 1, Rsvd1 : 6;
} __ATTRIB_PACK DCO_LUT_NEW;

typedef struct dco_lut_old {
    int16_t LUT_Imag[RXDCO_MAX_CHAIN];
    int16_t LUT_Real[RXDCO_MAX_CHAIN];
    int16_t Range;
    int16_t pad;
} __ATTRIB_PACK DCO_LUT_OLD;

typedef struct rxdco_cal_result {
    uint16_t ODAC_I : 9, Range : 2, Success : 1, Rsvd0 : 4;
    uint16_t ODAC_Q : 9, CalMatrixID : 3, Rsvd1 : 4;
} __ATTRIB_PACK RXDCO_CAL_RESULT[RXDCO_MAX_CHAIN][RXDCO_MAX_NUM_CAL_GAIN];

typedef struct phydevlib_rxdco_input {
    uint8_t calCmdId;
    uint8_t cal_mode;
    // uint8_t     channelIdx;
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

typedef struct phydevlib_dacplayback_input {
    uint8_t plyCount;
    uint32_t playback_enable;
    uint32_t playbackTonefreqMHz;
    uint32_t txGainIndex;
#if 0
    uint8_t     radarEn;
    uint8_t     radarOn;
    uint8_t     radarOff;
    uint8_t     plybckEN;
    uint8_t     pad[3];
    uint8_t     data4k[4096];
    uint32_t    csr_playback_bank;
    uint32_t    csr_playback_on_both_split_phys;
    uint32_t    csr_playback_fifo_th;
    uint32_t    csr_playback_loop_count;
    uint32_t    csr_playback_mem_depth;
    uint32_t    csr_playback_count_a;
    uint32_t    isSysSimPlayback;
    uint32_t    isPhydbgBankLoaded;
    uint32_t    playbackChMask;
    uint32_t    numberOfPlaybackPackets;
    uint32_t    playbackPacketDuration[10];
    uint32_t    playbackPacketStartDelay[10];
#endif
} __ATTRIB_PACK PHYDEVLIB_DACPLAYBACK_INPUT;

typedef struct phydevlib_dacplayback_output {
    uint8_t status;
    uint8_t pad[3];
} __ATTRIB_PACK PHYDEVLIB_DACPLAYBACK_OUTPUT;

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

typedef struct phydevlib_debug_cmd_input {
    uint32_t num_args;
    uint32_t *args;
} __ATTRIB_PACK PHYDEVLIB_DEBUG_CMD_INPUT;

typedef struct phydevlib_coex_input {
    uint8_t isSet;
    uint8_t semaphoreId;
    uint8_t pad[2];
    uint8_t crx_enable;
    uint32_t bt_rssi_upper_threshold;
    uint32_t bt_rssi_lower_threshold;
    uint8_t lnagain;
    uint8_t glut_sel;
    uint32_t cxc_init_gain;
    uint32_t cxc_max_gain;
    uint32_t cxc_min_gain;
    uint32_t cxc_sat_step;
    uint32_t cxc_coarse_step_ht;
    uint32_t cxc_coarse_step;
    uint8_t lnaPwrMode;
} __ATTRIB_PACK PHYDEVLIB_COEX_INPUT;

typedef struct phydevlib_fdmt_iqcal_input {
    uint8_t calCmdId;
    // uint8_t channelIdx;
    uint8_t gain_index;
    uint8_t odac_range;
    uint8_t tx_power_mode;  // tx_power_mode_t tx_power_mode;
#if 0
    uint8_t     calCmdId;
    uint8_t     combCalType;
    uint8_t     chainIdx;
    uint8_t     tableIdx;
    uint32_t    cal_chain_mask;
    uint32_t    cal_mode;
    uint32_t    engineSharing;
    uint32_t    driveTxGLUTIdx;
    uint32_t    enableBlockAccAvg;
#if defined(PHYDEVLIB_PRODUCT_HAWKEYE) || defined(PHYDEVLIB_PRODUCT_HAWKEYE2) || defined(PHYDEVLIB_PRODUCT_CYPRESS) || \
    defined(PHYDEVLIB_PRODUCT_ALDER)
    uint32_t    blockSize;
#else
    uint32_t    blockSize[3];
#endif
    uint32_t    dcEstWindowSize;
    uint32_t    numOfBlocks;
    uint32_t    numSamplesToReadback[3];
    uint32_t    rxGainSettlingTime;
    uint32_t    txGainSettlingTime;
    uint32_t    txShiftSettlingTime;
    uint32_t    calModeSettlingTime;
    uint32_t    loopbackSettlingTime;
    uint32_t    txResidueSettlingTime;
    uint32_t    fixed_gain;
    //uint32_t    cal_type;
    uint32_t    numTaps[DIRECTION_MAX];//Tx taps @ 0, Rx taps @ 1

    //added for program_fdmt_iqcorr_coeffs
    uint32_t    band;
    uint32_t    mode;
    uint32_t    calDirection;//Tx=0, Rx=1
    uint32_t    cps;
    uint32_t    depth;
    uint32_t* cal_cl;
    uint32_t* cal_dig_cl;
    //uint32_t   *cal_dig_cl_i;//for HST HW1.3. Will remove in HW1.4
    //uint32_t   *cal_dig_cl_q;//for HST HW1.3. Will remove in HW1.4
    uint32_t* cal_rxcorr_mapping;
    uint32_t* cal_dtim_rxiq_coeff_i;
    uint32_t* cal_dtim_rxiq_coeff_q;
    uint32_t* cal_dpd_rxiq_coeff_i;
    uint32_t* cal_dpd_rxiq_coeff_q;
    //uint32_t   *cal_coeff;//for HST HW1.3. Will remove in HW1.4
    uint32_t* cal_coeff_l;//for HST
    uint32_t* cal_coeff_u;//for HST
    uint32_t* cal_coeff_i;
    uint32_t* cal_coeff_q;
    uint32_t* cal_dpd_rxiq_coeff;
    uint32_t    progPriOrSec;
    uint8_t     drmImpair;
    uint32_t    coarseLoCal;
    uint32_t    numTxGains;
    uint32_t    numIters;
    uint32_t    dgt_offset_i_val;
    uint8_t     dis_by_bit; //disable by bit in disable API
    uint8_t     pad[2];
#endif
} __ATTRIB_PACK PHYDEVLIB_FDMT_IQCAL_INPUT;

typedef struct phydevlib_fdmt_iqcal_output {
    uint8_t calCmdId;
#if 0
    //uint32_t    cal_results[MAX_PHY][MAX_CAL_ENGINES][1024][2];//placeholder - will hold final post processing results
    uint32_t* cal_cl;
    uint32_t* cal_rxcorr_mapping;
    uint32_t* cal_dig_cl;
    uint32_t* cal_coeff;//for HST HW1.3. Will remove in HW1.4
    uint32_t* cal_coeff_l;
    uint32_t* cal_coeff_u;
    uint32_t* cal_dig_cl_i;//for HST HW1.3. Will remove in HW1.4
    uint32_t* cal_dig_cl_q;//for HST HW1.3. Will remove in HW1.4
    //uint32_t   *cal_dtim_rxiq_coeff_i;
    //uint32_t   *cal_dtim_rxiq_coeff_q;
    uint32_t* cal_dpd_rxiq_coeff_i;
    uint32_t* cal_dpd_rxiq_coeff_q;
    uint32_t* cal_coeff_i;
    uint32_t* cal_coeff_q;
    uint32_t* cal_dpd_rxiq_coeff;
#endif
} __ATTRIB_PACK PHYDEVLIB_FDMT_IQCAL_OUTPUT;

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

#endif /* _PHY_DEVLIB_API_PARM_H */
