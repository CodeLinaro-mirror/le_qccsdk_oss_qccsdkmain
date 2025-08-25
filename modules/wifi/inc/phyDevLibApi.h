/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _PHY_DEVLIB_API_H
#define _PHY_DEVLIB_API_H

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
 *  devLib/phyDevLibApi.h
 *      The only external header file needed by the user s/w.
 *      It contains all the "exported" PHY DEV LIB APIs.
 *
 *  product/xxxx
 *      contains the files at the product level, e.g. Adrastea_Cherokee
 *      Including one such directory dictates the product configuration.
 *      gpInitProdDevLib is defined and bound in each of these product directories.
 *      It's envisioned each product will have its directory.
 *      product/adrastea_cherokee
 *      product/lithium_xxx
 *
 * rf/xxxx
 *      contains the TRX files, following the same principles as product
 *
 * bb/xxxx
 *      contains the BB files, following the same principles as product
 *
 */

#define PHYPLAT_INLINE inline
#define PHYPLAT_API    extern

#define PHYDEVLIB_API PHYPLAT_API

/*! ---------------------------
 *  @brief PHY DEV supported APIs
 *
 *  Usage:
 *      1. In the initialization code, call
 *         initPhyDevLib() (obsolete in Q5_2p0)
 *      2. Call the rest of PHY dev lib APIs, with TLV2p0 parameters
 *         e.g. phyReset(phy_input, reset_input, reset_output)
 */
#if defined(PHYDEVLIB_IOT)
PHYDEVLIB_API RECIPE_RC phyReset(void *phy_input, void *reset_input, void *reset_output);
PHYDEVLIB_API RECIPE_RC phyChannelSwitch(void *phy_input, void *channel_switch_input, void *channel_switch_output);
PHYDEVLIB_API RECIPE_RC phyForcedGainMode(void *phy_input, void *forceGainModeInput, void *forceGainModeOutput);
PHYDEVLIB_API RECIPE_RC phyFTPGRx(void *phy_input, void *ftpg_rx_input, void *ftpg_rx_output);
PHYDEVLIB_API RECIPE_RC phyFTPGTx(void *phy_input, void *ftpg_tx_input, void *ftpg_tx_output);
PHYDEVLIB_API RECIPE_RC phyGetHandle(void *phy_input, void **handle);
PHYDEVLIB_API RECIPE_RC phySetParam(void *phy_input, uint32_t param_id, void *param);
PHYDEVLIB_API RECIPE_RC phyGetParam(void *phy_input, uint32_t param_id, void *param);
PHYDEVLIB_API RECIPE_RC phyRxDCOCal(void *phy_input, void *rxdco_cal_input, void *rxdco_cal_output);
PHYDEVLIB_API RECIPE_RC phyDPDCal(void *phy_input, void *dpd_cal_input, void *dpd_cal_output);
PHYDEVLIB_API RECIPE_RC phySpurMitigation(void *phy_input, void *spur_mitigation_input, void *spur_mitigation_output);
#else
PHYDEVLIB_API RECIPE_RC phyReset(void *phy_input, void *reset_input, void *reset_output);
PHYDEVLIB_API RECIPE_RC phySetChainMask(void *phy_input, void *chain_mask_input, void *chain_mask_output);
PHYDEVLIB_API RECIPE_RC phySetRxChainMask(void *phy_input, void *chain_mask_input, void *chain_mask_output);
PHYDEVLIB_API RECIPE_RC phyProgRFA(void *phy_input, void *prog_rfa_input, void *prog_rfa_output);
PHYDEVLIB_API RECIPE_RC phySetXbar(void *phy_input, void *xbar_input, void *xbar_output);
PHYDEVLIB_API RECIPE_RC phySetChannel(void *phy_input, void *set_channel_input, void *set_channel_output);
PHYDEVLIB_API RECIPE_RC phySetChannelAgile(void *phy_input, void *set_channel_input, void *set_channel_output);
PHYDEVLIB_API RECIPE_RC phyWarmReset(void *phy_input, void *warm_reset_input, void *warm_reset_output);
PHYDEVLIB_API RECIPE_RC phyWarmResetClearVreg(void *phy_input, void *warm_reset_input, void *warm_reset_output);
PHYDEVLIB_API RECIPE_RC phyWarmResetReadRfBusStatus(void *phy_input, void *warm_reset_input, void *warm_reset_output);
PHYDEVLIB_API RECIPE_RC phyGetHandle(void *phy_input, void **handle);
PHYDEVLIB_API RECIPE_RC phyChannelSwitch(void *phy_input, void *channel_switch_input, void *channel_switch_output);
PHYDEVLIB_API RECIPE_RC phySetParam(void *phy_input, uint32_t param_id, void *param);
PHYDEVLIB_API RECIPE_RC phyGetParam(void *phy_input, uint32_t param_id, void *param);
PHYDEVLIB_API RECIPE_RC phyClkSwitch(void *phy_input, void *clk_switch_input, void *clk_switch_output);
PHYDEVLIB_API RECIPE_RC phyNFCal(void *phy_input, void *nf_cal_input, void *nf_cal_output);
PHYDEVLIB_API RECIPE_RC phyForcedGainMode(void *phy_input, void *forceGainModeInput, void *forceGainModeOutput);
PHYDEVLIB_API RECIPE_RC phyRxDCOCal(void *phy_input, void *rxdco_cal_input, void *rxdco_cal_output);
PHYDEVLIB_API RECIPE_RC phyRssiDbToDbmOffset(void *phy_input, void *rssi_db_to_dbm_offset_input,
                                             void *rssi_db_to_dbm_offset_output);
PHYDEVLIB_API RECIPE_RC phyCombinedCal(void *phy_input, void *combined_cal_input, void *combined_cal_output);
PHYDEVLIB_API RECIPE_RC phyPDCCal(void *phy_input, void *pdc_cal_input, void *pdc_cal_output);
PHYDEVLIB_API RECIPE_RC phyBypassXlnaInListen(void *phy_input, void *lna_input, void *lna_output);
PHYDEVLIB_API RECIPE_RC phyUseXlna(void *phy_input, void *lna_input, void *lna_output);
PHYDEVLIB_API RECIPE_RC phyPkDetCal(void *phy_input, void *pkdet_cal_input, void *pkdet_cal_output);
PHYDEVLIB_API RECIPE_RC phyGetPdlVersion(void *phy_input, void *get_pdl_version_input, void *get_pdl_version_output);
PHYDEVLIB_API RECIPE_RC phyGetRdlVersion(void *phy_input, void *get_rdl_version_input, void *get_rdl_version_output);
PHYDEVLIB_API RECIPE_RC phySetRU26DisableTx(void *phy_input, void *ru26disabletx_input, void *ru26disabletx_output);
PHYDEVLIB_API RECIPE_RC phySetM3TxBfParams(void *phy_input, void *m3txbf_input, void *m3txbf_output);
PHYDEVLIB_API RECIPE_RC phyGetM3TxBfParams(void *phy_input, void *m3txbf_input, void *m3txbf_output);
PHYDEVLIB_API RECIPE_RC phyLatestAccumulatedCLPCError(void *phy_input, void *latest_accumulated_clpc_error_input,
                                                      void *latest_accumulated_clpc_error_output);
PHYDEVLIB_API RECIPE_RC phyLatestAccumulatedCLPCErrorPerChain(void *phy_input,
                                                              void *latest_accumulated_clpc_error_input,
                                                              void *latest_accumulated_clpc_error_output);
PHYDEVLIB_API RECIPE_RC phyLatestThermValue(void *phy_input, void *latest_therm_value_input,
                                            void *latest_therm_value_output);
PHYDEVLIB_API RECIPE_RC phyGetXoCdacin(void *phy_input, void *xo_cdacin_input, void *xo_cdacin_output);
PHYDEVLIB_API RECIPE_RC phySetXoCdacin(void *phy_input, void *xo_cdacin_input, void *xo_cdacin_output);
PHYDEVLIB_API RECIPE_RC phyGetXoCdacout(void *phy_input, void *xo_cdacout_input, void *xo_cdacout_output);
PHYDEVLIB_API RECIPE_RC phySetXoCdacout(void *phy_input, void *xo_cdacout_input, void *xo_cdacout_output);
PHYDEVLIB_API RECIPE_RC phyGetPpsMode(void *phy_input, void *pps_mode_input, void *pps_mode_output);
PHYDEVLIB_API RECIPE_RC phyRtt(void *phy_input, void *rtt_input, void *rtt_output);
PHYDEVLIB_API RECIPE_RC phyCfrCirCap(void *phy_input, void *cfrCirCap_input, void *cfrCirCap_output);
PHYDEVLIB_API RECIPE_RC phyRccCfrCirCap(void *phy_input, void *cfrCirCap_input, void *cfrCirCap_output);
PHYDEVLIB_API RECIPE_RC phySpurMitigation(void *phy_input, void *spur_mitigation_input, void *spur_mitigation_output);
PHYDEVLIB_API RECIPE_RC phyEnableDynSpurMitigation(void *phy_input, void *spur_mitigation_input,
                                                   void *spur_mitigation_output);
PHYDEVLIB_API RECIPE_RC phyAdfsEnableDisable(void *phy_input, void *adfs_input, void *adfs_output);
PHYDEVLIB_API RECIPE_RC phySkipVreg(void *phy_input, void *adfs_input, void *adfs_output);
PHYDEVLIB_API RECIPE_RC phyAdfsSbsPhySel(void *phy_input, void *adfs_input, void *adfs_output);
PHYDEVLIB_API RECIPE_RC phyProgINI(void *phy_input, void *prog_ini_input, void *prog_ini_output);
PHYDEVLIB_API RECIPE_RC phyRxBBFCal(void *phy_input, void *rx_bbf_cal_input, void *rx_bbf_cal_output);
PHYDEVLIB_API RECIPE_RC phyTxBBFCal(void *phy_input, void *tx_bbf_cal_input, void *tx_bbf_cal_output);
PHYDEVLIB_API RECIPE_RC phyRegRxtdAgcPwrHigh(void *phy_input, void *rxtd_agc_pwr_high_input,
                                             void *rxtd_agc_pwr_high_output);
PHYDEVLIB_API RECIPE_RC phyRegRxtdAgcPwrThrs(void *phy_input, void *rxtd_agc_pwr_thrs_input,
                                             void *rxtd_agc_pwr_thrs_output);
PHYDEVLIB_API RECIPE_RC phySetDynSmps(void *phy_input, void *dynsmps_input, void *dynsmps_output);
PHYDEVLIB_API RECIPE_RC phyDACCal(void *phy_input, void *dac_cal_input, void *dac_cal_output);
PHYDEVLIB_API RECIPE_RC phyIM2Cal(void *phy_input, void *im2_cal_input, void *im2_cal_output);
PHYDEVLIB_API RECIPE_RC phyShutdownSynth(void *phy_input, void *shutdown_synth_input, void *shutdown_synth_output);
PHYDEVLIB_API RECIPE_RC phySetOCL(void *phy_input, void *ocl_input, void *ocl_output);
PHYDEVLIB_API RECIPE_RC phySetLPL(void *phy_input, void *prog_reg_input, void *prog_reg_output);
PHYDEVLIB_API RECIPE_RC phyResetAllChCtrs(void *phy_input, void *prog_reg_input, void *prog_reg_output);
PHYDEVLIB_API RECIPE_RC phyDPDCal(void *phy_input, void *dpd_cal_input, void *dpd_cal_output);
PHYDEVLIB_API RECIPE_RC phyBtLpXO(void *phy_input, void *bt_lp_xo_input, void *bt_lp_xo_output);
PHYDEVLIB_API RECIPE_RC phyWlanXO(void *phy_input, void *wlan_xo_input, void *wlan_xo_output);
PHYDEVLIB_API RECIPE_RC phyIbfCal(void *phy_input, void *ibf_cal_input, void *ibf_cal_output);
PHYDEVLIB_API RECIPE_RC phyVregFtmMode(void *phy_input, void *vreg_ftm_mode_input, void *vreg_ftm_mode_output);
PHYDEVLIB_API RECIPE_RC phySetPBS(void *phy_input, void *pbs_input, void *pbs_output);
PHYDEVLIB_API RECIPE_RC phyADCCal(void *phy_input, void *adc_cal_input, void *adc_cal_output);
PHYDEVLIB_API RECIPE_RC phyPSMask(void *phy_input, void *ps_mask_input, void *ps_mask_output);
PHYDEVLIB_API RECIPE_RC phyPALCal(void *phy_input, void *pal_cal_input, void *pal_cal_output);
PHYDEVLIB_API RECIPE_RC phySetTPC(void *phy_input, void *set_tpc_input, void *set_tpc_output);
PHYDEVLIB_API RECIPE_RC phyALUTReadWrite(void *phy_input, void *alutReadWriteInput, void *alutReadWriteOutput);
PHYDEVLIB_API RECIPE_RC phyPLUTReadWrite(void *phy_input, void *plutReadWriteInput, void *plutReadWriteOutput);
PHYDEVLIB_API RECIPE_RC phyGLUTReadWrite(void *phy_input, void *glutReadWriteInput, void *glutReadWriteOutput);
PHYDEVLIB_API RECIPE_RC phyGetGlutModeRegs(void *phy_input, void *glutin_tpc_mode_reg);
PHYDEVLIB_API RECIPE_RC phyProgGlutModeRegs(void *phy_input, void *glutin_tpc_mode_reg);
PHYDEVLIB_API RECIPE_RC phyOLPCTempComp(void *phy_input, void *olpc_temp_comp_input, void *olpc_temp_comp_output);
PHYDEVLIB_API RECIPE_RC phyOLPCTempUpdate(void *phy_input, void *olpc_temp_update_input, void *olpc_temp_update_output);
PHYDEVLIB_API RECIPE_RC phyAdcCal(void *phy_input, void *adc_cal_input, void *adc_cal_output);
PHYDEVLIB_API RECIPE_RC phyForceFixedNFCal(void *phy_input, void *nf_cal_input, void *nf_cal_output);
PHYDEVLIB_API RECIPE_RC phyTpcOlpcCtrl(void *phy_input, void *olpc_ctrl_input, void *olpc_ctrl_output);
PHYDEVLIB_API RECIPE_RC phyLoadRfaSupportIni(void *phy_input, void *rfa_input, void *rfa_output);
PHYDEVLIB_API RECIPE_RC phyRfaEnterpriseRateIni(void *phy_input, void *prog_ini_input, void *prog_ini_output);
PHYDEVLIB_API RECIPE_RC phyRfaSetAltTblIdx(void *phy_input, void *rfa_input, void *rfa_output);
PHYDEVLIB_API RECIPE_RC phyRfaReadSynthRegister(void *phy_input, void *rfa_input, void *rfa_output);
PHYDEVLIB_API RECIPE_RC phyRfaBootSeq(void *phy_input, void *rfa_boot_seq_input, void *rfa_boot_seq_output);
PHYDEVLIB_API RECIPE_RC phyDisableRfa(void *phy_input, void *disable_rfa_input, void *disable_rfa_output);
PHYDEVLIB_API RECIPE_RC phyTxConvergenceCheck(void *phy_input, void *tx_converged_input, void *tx_converged_output);
PHYDEVLIB_API RECIPE_RC phyReadNFCal(void *phy_input, void *nf_cal_input, void *nf_cal_output);
PHYDEVLIB_API RECIPE_RC phyRfaTurnOnOff(void *phy_input, void *rfa_input, void *rfa_output);
PHYDEVLIB_API RECIPE_RC phyRfaSaveRestoreRegs(void *phy_input, void *rfa_input, void *rfa_output);
PHYDEVLIB_API RECIPE_RC phyRxDCOSetControlMode(void *phy_input, void *rxdco_cal_input, void *rxdco_cal_output);
PHYDEVLIB_API RECIPE_RC phyQuickEnableIQCorr(void *phy_input, void *fdmt_iqcal_input, void *fdmt_iqcal_output);
PHYDEVLIB_API RECIPE_RC phyQuickDisableIQCorr(void *phy_input, void *fdmt_iqcal_input, void *fdmt_iqcal_output);
PHYDEVLIB_API RECIPE_RC phyQuickEnableTxLoDigitCorr(void *phy_input, void *fdmt_iqcal_input, void *fdmt_iqcal_output);
PHYDEVLIB_API RECIPE_RC phyQuickDisableTxLoDigitCorr(void *phy_input, void *fdmt_iqcal_input, void *fdmt_iqcal_output);
PHYDEVLIB_API RECIPE_RC phyGetPdetPdadcPacket(void *phy_input, void *tpc_cal_input, void *tpc_cal_output);
PHYDEVLIB_API RECIPE_RC phyOverrideReleasePdetAttn(void *phy_input, void *tpc_cal_input, void *tpc_cal_output);
PHYDEVLIB_API RECIPE_RC phyPaprdDeviceReadGlut(void *phy_input, void *paprd_input, void *paprd_output);
PHYDEVLIB_API RECIPE_RC phyPaprdDeviceHwEnable(void *phy_input, void *paprd_input, void *paprd_output);
PHYDEVLIB_API RECIPE_RC phyPaprdDeviceHwDisable(void *phy_input, void *paprd_input, void *paprd_output);
PHYDEVLIB_API RECIPE_RC phyPaprdDeviceGetComplete(void *phy_input, void *paprd_input, void *paprd_output);
PHYDEVLIB_API RECIPE_RC phyPaprdDeviceSetTxgainIdx(void *phy_input, void *paprd_input, void *paprd_output);
PHYDEVLIB_API RECIPE_RC phyPaprdDeviceStopTraining(void *phy_input, void *paprd_input, void *paprd_output);
PHYDEVLIB_API RECIPE_RC phyPaprdDeviceTrainingStatus(void *phy_input, void *paprd_input, void *paprd_output);
PHYDEVLIB_API RECIPE_RC phyPaprdDeviceResetPaprdTraining(void *phy_input, void *paprd_input, void *paprd_output);
PHYDEVLIB_API RECIPE_RC phyPaprdDeviceInitStaticSettings(void *phy_input, void *paprd_input, void *paprd_output);
PHYDEVLIB_API RECIPE_RC phyPaprdDeviceUpdateUcodeDpdDone(void *phy_input, void *paprd_input, void *paprd_output);
PHYDEVLIB_API RECIPE_RC phyPaprdDevicePopulatePaprdTables(void *phy_input, void *paprd_input, void *paprd_output);
PHYDEVLIB_API RECIPE_RC phyPaprdDeviceRestorePreCalSettings(void *phy_input, void *paprd_input, void *paprd_output);
PHYDEVLIB_API RECIPE_RC phyPaprdDeviceEDPDCtrlRegs(void *phy_input, void *paprd_input, void *paprd_output);
PHYDEVLIB_API RECIPE_RC phyPaprdDeviceSavePhydbgSettings(void *phy_input, void *paprd_input, void *paprd_output);
PHYDEVLIB_API RECIPE_RC phyPaprdDeviceRestorePhydbgSettings(void *phy_input, void *paprd_input, void *paprd_output);
PHYDEVLIB_API RECIPE_RC phyPaprdDeviceGetGlutIdxFromTgtPwr(void *phy_input, void *paprd_input, void *paprd_output);
PHYDEVLIB_API RECIPE_RC phyCorePaprdEnableDPD(void *phy_input, void *paprd_input, void *paprd_output);
PHYDEVLIB_API RECIPE_RC phyCorePaprdWarmPacketSupport(void *phy_input, void *paprd_warm_packet_support_input,
                                                      void *paprd_warm_packet_support_output);
PHYDEVLIB_API RECIPE_RC phyCorePaprdLoopbackTrainingComplete(void *phy_input, void *paprd_input, void *paprd_output);
PHYDEVLIB_API RECIPE_RC phyDPDCalInitTrainingData(void *phy_input, void *paprd_input, void *paprd_output);
PHYDEVLIB_API RECIPE_RC phyPaprdMemDpdTrain(void *phy_input, void *paprd_input, void *paprd_output);
PHYDEVLIB_API RECIPE_RC phyCorePaprdGetTempScaling(void *phy_input, void *paprd_input, void *paprd_output);
PHYDEVLIB_API RECIPE_RC phyPaprdDeviceConfigTempScaling(void *phy_input, void *paprd_input, void *paprd_output);
PHYDEVLIB_API RECIPE_RC phyEnterpriseRateConfig(void *phy_input, void *config_input, void *config_output);
PHYDEVLIB_API RECIPE_RC phyFTPGRx(void *phy_input, void *ftpg_rx_input, void *ftpg_rx_output);
PHYDEVLIB_API RECIPE_RC phyFTPGTx(void *phy_input, void *ftpg_tx_input, void *ftpg_tx_output);
PHYDEVLIB_API RECIPE_RC phyFtpgSequence(void *phy_input, void *ftpg_sequence_input, void *ftpg_sequence_output);
PHYDEVLIB_API RECIPE_RC phyGetRfaChipID(void *phy_input, void *rfa_input, void *rfa_output);
PHYDEVLIB_API RECIPE_RC phySetSVS(void *phy_input, void *svs_input, void *svs_output);
PHYDEVLIB_API RECIPE_RC phyXfemCtrlThres(void *phy_input, void *fem_input, void *fem_output);
PHYDEVLIB_API RECIPE_RC phyProgFemCtrl(void *phy_input, void *fem_input, void *fem_output);
PHYDEVLIB_API RECIPE_RC phySetRxDeaf(void *phy_input, void *rxdeaf_input, void *rxdeaf_output);
PHYDEVLIB_API RECIPE_RC phyRetention(void *phy_input, void *retention_input, void *retention_output);
PHYDEVLIB_API RECIPE_RC phyCalRetention(void *phy_input, void *cal_retention_input, void *cal_retention_output);
PHYDEVLIB_API RECIPE_RC phySetTempCompensation(void *phy_input, void *temp_compensation_input,
                                               void *temp_compensation_output);
PHYDEVLIB_API RECIPE_RC phyConfigWsi(void *phy_input);
PHYDEVLIB_API RECIPE_RC phyStartPcssClocks(void *phy_input);
PHYDEVLIB_API RECIPE_RC phyM3Init(void *phy_input);
PHYDEVLIB_API RECIPE_RC phyM3ChannelSwitch(void *phy_input);
PHYDEVLIB_API RECIPE_RC phySetDynPriChn(void *phy_input);
PHYDEVLIB_API RECIPE_RC phyLoadIniOneTime(void *phy_input, void *load_ini_input, void *load_ini_output);
PHYDEVLIB_API RECIPE_RC phyLoadIniCommon(void *phy_input, void *load_ini_input, void *load_ini_output);
PHYDEVLIB_API RECIPE_RC phyLoadIniBimodal(void *phy_input, void *load_ini_input, void *load_ini_output);
PHYDEVLIB_API RECIPE_RC phyLoadIniModal(void *phy_input, void *load_ini_input, void *load_ini_output);
PHYDEVLIB_API RECIPE_RC phyLoadIniTxPef(void *phy_input, void *load_ini_input, void *load_ini_output);
PHYDEVLIB_API RECIPE_RC phyLoadIniDfs(void *phy_input, void *load_ini_input, void *load_ini_output);
PHYDEVLIB_API RECIPE_RC phyLoadIniAni(void *phy_input, void *load_ini_input, void *load_ini_output);
PHYDEVLIB_API RECIPE_RC phyLoadIniSscan(void *phy_input, void *load_ini_input, void *load_ini_output);
PHYDEVLIB_API RECIPE_RC phyLoadIniOverride(void *phy_input, void *load_ini_input, void *load_ini_output);
PHYDEVLIB_API RECIPE_RC phyLoadIniEnterpriseRate(void *phy_input, void *load_ini_input, void *load_ini_output);
PHYDEVLIB_API RECIPE_RC phyConfigBWRate(void *phy_input, void *bw_rate_input, void *bw_rate_output);
PHYDEVLIB_API RECIPE_RC phyConfigHeavyClip(void *phy_input, void *heavy_clip_input, void *heavy_clip_output);
PHYDEVLIB_API RECIPE_RC phyConfigCckFirControl(void *phy_input, void *cckfir_ctrl_input, void *cckfir_ctrl_output);
PHYDEVLIB_API RECIPE_RC phyConfigAltTblIdx(void *phy_input, void *alt_tbl_idx_input, void *alt_tbl_idx_output);
PHYDEVLIB_API RECIPE_RC phyConfigSbsMode(void *phy_input, void *sbs_mode_input, void *sbs_mode_output);
PHYDEVLIB_API RECIPE_RC phyConfigWSI(void *phy_input, void *wsi_input, void *wsi_output);
PHYDEVLIB_API RECIPE_RC phyConfigRefClk(void *phy_input, void *reset_input, void *reset_output);
PHYDEVLIB_API RECIPE_RC phyLoadRfaIniPre(void *phy_input, void *load_ini_input, void *load_ini_output);
PHYDEVLIB_API RECIPE_RC phyLoadRfaIniOneTime(void *phy_input, void *load_ini_input, void *load_ini_output);
PHYDEVLIB_API RECIPE_RC phyLoadRfaIniCommon(void *phy_input, void *load_ini_input, void *load_ini_output);
PHYDEVLIB_API RECIPE_RC phyLoadRfaIniBimodal(void *phy_input, void *load_ini_input, void *load_ini_output);
PHYDEVLIB_API RECIPE_RC phyLoadRfaIniModal(void *phy_input, void *load_ini_input, void *load_ini_output);
PHYDEVLIB_API RECIPE_RC phyLoadRfaIniPost(void *phy_input, void *load_ini_input, void *load_ini_output);
PHYDEVLIB_API RECIPE_RC phyConfigRxTblBase(void *phy_input, void *tbl_base_input, void *tbl_base_output);
PHYDEVLIB_API RECIPE_RC phyConfigBBStaticCsr(void *phy_input, void *csr_input, void *csr_output);
PHYDEVLIB_API RECIPE_RC phyM3ConfigExt(void *phy_input, void *m3_input, void *m3_output);
PHYDEVLIB_API RECIPE_RC phyM3CheckStatus(void *phy_input, void *m3_input, void *m3_output);
PHYDEVLIB_API RECIPE_RC phyEnableDbgClk(void *phy_input, void *enable_dbg_clk_input, void *enable_dbg_clk_output);
PHYDEVLIB_API RECIPE_RC phyStartPcssClks(void *phy_input, void *pcss_clks_input, void *pcss_clks_output);
PHYDEVLIB_API RECIPE_RC phyM3InitExt(void *phy_input, void *m3_input, void *m3_output);
PHYDEVLIB_API RECIPE_RC phyWlanXOExt(void *phy_input, void *wlan_xo_input, void *wlan_xo_output);
PHYDEVLIB_API RECIPE_RC phyProgAGC(void *phy_input, void *agc_input, void *agc_output);
PHYDEVLIB_API RECIPE_RC phyProgVReg(void *phy_input, void *vreg_input, void *vreg_output);
PHYDEVLIB_API RECIPE_RC phyRetentionVRegIniTable(void *phy_input, void *vreg_input, void *vreg_output);
PHYDEVLIB_API RECIPE_RC phyConfigTxfeDPDControl(void *phy_input, void *txfe_dpd_input, void *txfe_dpd_output);
PHYDEVLIB_API RECIPE_RC phyQuickSetForcedSyn(void *phy_input, void *forced_syn_input, void *forced_syn_output);
PHYDEVLIB_API RECIPE_RC phySetForcedWsiSyn(void *phy_input, void *forced_syn_input, void *forced_syn_output);
PHYDEVLIB_API RECIPE_RC phyProgGlutModes(void *phy_input, void *glut_modes_input, void *glut_modes_output);
PHYDEVLIB_API RECIPE_RC phyPcssIUReqWarmReset(void *phy_input, void *pcss_input, void *pcss_output);
PHYDEVLIB_API RECIPE_RC phyPcssIUSwLogSet(void *phy_input, void *pcss_input, void *pcss_output);
PHYDEVLIB_API RECIPE_RC phyPcssIpcRingSendInterrupt(void *phy_input, void *pcss_input, void *pcss_output);
PHYDEVLIB_API RECIPE_RC phyPcssGetCrashSignature(void *phy_input, void *pcss_input, void *pcss_output);
PHYDEVLIB_API RECIPE_RC phyPcssGetIUErrMsg(void *phy_input, void *pcss_input, void *pcss_output);
PHYDEVLIB_API RECIPE_RC phyPcssGetIpcMapAddr(void *phy_input, void *pcss_input, void *pcss_output);
PHYDEVLIB_API RECIPE_RC phyPcssEnableDisableNssPhyErr(void *phy_input, void *pcss_input, void *pcss_output);
PHYDEVLIB_API RECIPE_RC phyPcssGetVersion(void *phy_input, void *pcss_input, void *pcss_output);
PHYDEVLIB_API RECIPE_RC phyPcssGetUcodeVersion(void *phy_input, void *pcss_input, void *pcss_output);
PHYDEVLIB_API RECIPE_RC phyPcssGetSetUcodeFeature(void *phy_input, void *pcss_input, void *pcss_output);
PHYDEVLIB_API RECIPE_RC phyPcssWriteBeaconRssi(void *phy_input, void *pcss_input, void *pcss_output);
PHYDEVLIB_API RECIPE_RC phyPcssWriteIURegister(void *phy_input, void *pcss_input, void *pcss_output);
PHYDEVLIB_API RECIPE_RC phyPcssSetCF(void *phy_input, void *pcss_input, void *pcss_output);
PHYDEVLIB_API RECIPE_RC phyPcssCheckIfPhyIsOff(void *phy_input, void *pcss_input, void *pcss_output);
PHYDEVLIB_API RECIPE_RC phyPcssSpatialReuse(void *phy_input, void *pcss_input, void *pcss_output);
PHYDEVLIB_API RECIPE_RC phyPcssGetOtpBits(void *phy_input, void *pcss_input, void *pcss_output);
PHYDEVLIB_API RECIPE_RC phySetMuUserPos(void *phy_input, void *iu_input, void *iu_output);
PHYDEVLIB_API RECIPE_RC phyResetFtpg(void *phy_input, void *ftpg_input, void *ftpg_output);
PHYDEVLIB_API RECIPE_RC phyGetRxStats(void *phy_input, void *rx_stats_input, void *rx_stats_output);
PHYDEVLIB_API RECIPE_RC phyAssocId(void *phy_input, void *assoc_id_input, void *assoc_id_output);
PHYDEVLIB_API RECIPE_RC phySnifferMode(void *phy_input, void *sniffer_mode_input, void *sniffer_mode_output);
PHYDEVLIB_API RECIPE_RC phySetCfoParams(void *phy_input, void *cfo_input, void *cfo_output);
PHYDEVLIB_API RECIPE_RC phyGetCfoParams(void *phy_input, void *cfo_input, void *cfo_output);
PHYDEVLIB_API RECIPE_RC phyGetState(void *phy_input, void *phy_state_input, void *phy_state_output);
PHYDEVLIB_API RECIPE_RC phySetDssWarCFODeltaThreshold(void *phy_input, void *dss_war_cfo_input,
                                                      void *dss_war_cfo_output);
PHYDEVLIB_API RECIPE_RC phyEnableDssWarCFO(void *phy_input, void *dss_war_cfo_input, void *dss_war_cfo_output);
PHYDEVLIB_API RECIPE_RC phyRfaToggle(void *phy_input, void *rfa_input, void *rfa_output);
PHYDEVLIB_API RECIPE_RC phyGetPaMuteState(void *phy_input, void *rfa_input, void *rfa_output);
PHYDEVLIB_API RECIPE_RC phyCxmResetWci2DataPath(void *phy_input, void *cxm_input, void *cxm_output);
PHYDEVLIB_API RECIPE_RC phyCxmSetWlanStatus(void *phy_input, void *cxm_input, void *cxm_output);
PHYDEVLIB_API RECIPE_RC phyCxmSelectWlanStatusClr(void *phy_input, void *cxm_input, void *cxm_output);
PHYDEVLIB_API RECIPE_RC phyCxmWlanSwReset(void *phy_input, void *cxm_input, void *cxm_output);
PHYDEVLIB_API RECIPE_RC phyCxmResetWsiSlave(void *phy_input, void *cxm_input, void *cxm_output);
PHYDEVLIB_API RECIPE_RC phyCxmGetClkDivStep(void *phy_input, void *cxm_input, void *cxm_output);
PHYDEVLIB_API RECIPE_RC phyCxmSetClkDiv(void *phy_input, void *cxm_input, void *cxm_output);
PHYDEVLIB_API RECIPE_RC phyCxmSelectWlanStatusSet(void *phy_input, void *cxm_input, void *cxm_output);
PHYDEVLIB_API RECIPE_RC phyCxmSetTLMMRxd(void *phy_input, void *cxm_input, void *cxm_output);
PHYDEVLIB_API RECIPE_RC phyCxmSetTLMMTxd(void *phy_input, void *cxm_input, void *cxm_output);
PHYDEVLIB_API RECIPE_RC phyCxmSetTLMMClk(void *phy_input, void *cxm_input, void *cxm_output);
PHYDEVLIB_API RECIPE_RC phyCxmSetTLMMData(void *phy_input, void *cxm_input, void *cxm_output);
PHYDEVLIB_API RECIPE_RC phyCxmSetUartInt(void *phy_input, void *cxm_input, void *cxm_output);
PHYDEVLIB_API RECIPE_RC phyCxmSetUartIntEn(void *phy_input, void *cxm_input, void *cxm_output);
PHYDEVLIB_API RECIPE_RC phyCxmGetBTXOClock(void *phy_input, void *cxm_input, void *cxm_output);
PHYDEVLIB_API RECIPE_RC phyCxmSetOverrideGainSetting(void *phy_input, void *cxm_input, void *cxm_output);
PHYDEVLIB_API RECIPE_RC phyMrcBackoff(void *phy_input, void *spur_mitigation_input, void *spur_mitigation_output);
PHYDEVLIB_API RECIPE_RC phySetXtalCalValue(void *phy_input, void *xtal_cal_input, void *xtal_cal_output);
PHYDEVLIB_API RECIPE_RC phyGetXtalCalValue(void *phy_input, void *xtal_cal_input, void *xtal_cal_output);
PHYDEVLIB_API RECIPE_RC phySetXtalCalCWTone(void *phy_input, void *xtal_cal_input, void *xtal_cal_output);
PHYDEVLIB_API RECIPE_RC phyNFGetCal(void *phy_input, void *nf_cal_input, void *nf_cal_output);
PHYDEVLIB_API RECIPE_RC phyNFRegRead(void *phy_input, void *nf_cal_input, void *nf_cal_output);
PHYDEVLIB_API RECIPE_RC phyNFBtDisableCal(void *phy_input, void *nf_cal_input, void *nf_cal_output);
PHYDEVLIB_API RECIPE_RC phyLogNfCalDebugRegisters(void *phy_input, void *nf_cal_input, void *nf_cal_output);
PHYDEVLIB_API RECIPE_RC phyEnableDisableRxIQ(void *phy_input, void *fdmt_iqcal_input, void *fdmt_iqcal_output);
PHYDEVLIB_API RECIPE_RC phyAniSetDesense(void *phy_input, void *ani_input, void *ani_output);
PHYDEVLIB_API RECIPE_RC phyAniGetXlnaInfo(void *phy_input, void *ani_input, void *ani_output);
PHYDEVLIB_API RECIPE_RC phyAniSetDesenseLevel(void *phy_input, void *ani_input, void *ani_output);
PHYDEVLIB_API RECIPE_RC phyAniEnableDisable(void *phy_input, void *ani_input, void *ani_output);
PHYDEVLIB_API RECIPE_RC phyAniReadReg(void *phy_input, void *ani_input, void *ani_output);
PHYDEVLIB_API RECIPE_RC phyAniWriteReg(void *phy_input, void *ani_input, void *ani_output);
PHYDEVLIB_API RECIPE_RC phyAniUpdateReg(void *phy_input, void *ani_input, void *ani_output);
PHYDEVLIB_API RECIPE_RC phyAniUpdateEdCca(void *phy_input, void *ani_input, void *ani_output);
PHYDEVLIB_API RECIPE_RC phyAniGetEdCca(void *phy_input, void *ani_input, void *ani_output);
PHYDEVLIB_API RECIPE_RC phyAniReadCounter(void *phy_input, void *ani_input, void *ani_output);
PHYDEVLIB_API RECIPE_RC phyAniClearCounter(void *phy_input, void *ani_input, void *ani_output);
PHYDEVLIB_API RECIPE_RC phyEfuseInit(void *phy_input, void *efuse_input, void *efuse_output);
PHYDEVLIB_API RECIPE_RC phyEfuseRead(void *phy_input, void *efuse_input, void *efuse_output);
PHYDEVLIB_API RECIPE_RC phyEfuseWrite(void *phy_input, void *efuse_input, void *efuse_output);
PHYDEVLIB_API RECIPE_RC phySSInit(void *phy_input, void *ss_input, void *ss_output);
PHYDEVLIB_API RECIPE_RC phySSDisable(void *phy_input, void *ss_input, void *ss_output);
PHYDEVLIB_API RECIPE_RC phySSReadWriteConfigRegs(void *phy_input, void *ss_input, void *ss_output);
PHYDEVLIB_API RECIPE_RC phySSIsEnabled(void *phy_input, void *ss_input, void *ss_output);
PHYDEVLIB_API RECIPE_RC phySSActivateDeactivate(void *phy_input, void *ss_input, void *ss_output);
PHYDEVLIB_API RECIPE_RC phyDbgGetCsrCapInfo(void *phy_input, void *dbg_input, void *dbg_output);
PHYDEVLIB_API RECIPE_RC phyDbgTxGainCtrl(void *phy_input, void *dbg_input, void *dbg_output);
PHYDEVLIB_API RECIPE_RC phyDbgCaptureStop(void *phy_input, void *dbg_input, void *dbg_output);
PHYDEVLIB_API RECIPE_RC phyDbgGetTpcDebugReg(void *phy_input, void *dbg_input, void *dbg_output);
PHYDEVLIB_API RECIPE_RC phyDbgDumpMemoryBank(void *phy_input, void *dbg_input, void *dbg_output);
PHYDEVLIB_API RECIPE_RC phyDbgCrashDumpPerPhy(void *phy_input, void *dbg_input, void *dbg_output);
PHYDEVLIB_API RECIPE_RC phyDbgDumpMemoryBankADC(void *phy_input, void *dbg_input, void *dbg_output);
PHYDEVLIB_API RECIPE_RC phyDbgLoadDacMemoryBank(void *phy_input, void *dbg_input, void *dbg_output);
PHYDEVLIB_API RECIPE_RC phyDbgClearMemoryBank(void *phy_input, void *dbg_input, void *dbg_output);
PHYDEVLIB_API RECIPE_RC phyDbgDumpAllRegisters(void *phy_input, void *dbg_input, void *dbg_output);
PHYDEVLIB_API RECIPE_RC phyDbgPcssClkConfig(void *phy_input, void *dbg_input, void *dbg_output);
PHYDEVLIB_API RECIPE_RC phyDbgGetCWRssiConfig(void *phy_input, void *dbg_input, void *dbg_output);
PHYDEVLIB_API RECIPE_RC phyDbgReadRegs(void *phy_input, void *dbg_input, void *dbg_output);
PHYDEVLIB_API RECIPE_RC phyDbgCapture(void *phy_input, void *dbg_input, void *dbg_output);
PHYDEVLIB_API RECIPE_RC phyDbgAcquireMutex(void *phy_input, void *dbg_input, void *dbg_output);
PHYDEVLIB_API RECIPE_RC phyDbgReleaseMutex(void *phy_input, void *dbg_input, void *dbg_output);
PHYDEVLIB_API RECIPE_RC phyDbgConfigureTrigger(void *phy_input, void *dbg_input, void *dbg_output);
PHYDEVLIB_API RECIPE_RC phyDbgDacPlayback(void *phy_input, void *dbg_input, void *dbg_output);
PHYDEVLIB_API RECIPE_RC phyDbgConfigEventMask(void *phy_input, void *dbg_input, void *dbg_output);
PHYDEVLIB_API RECIPE_RC phyDbgDACRadioControl(void *phy_input, void *dbg_input, void *dbg_output);
PHYDEVLIB_API RECIPE_RC phyDbgGenerateCWToneAtOffset(void *phy_input, void *dbg_input, void *dbg_output);
PHYDEVLIB_API RECIPE_RC phyDfsRadarSubbandMask(void *phy_input, void *dfs_input, void *dfs_output);
PHYDEVLIB_API RECIPE_RC phyDfsReadRegister(void *phy_input, void *dfs_input, void *dfs_output);
PHYDEVLIB_API RECIPE_RC phyDfsDetectorDisable(void *phy_input, void *dfs_input, void *dfs_output);
PHYDEVLIB_API RECIPE_RC phyRttEnableDisableContChanCapture(void *phy_input, void *rtt_input, void *rtt_output);
PHYDEVLIB_API RECIPE_RC phyTPCIsOLPC(void *phy_input, void *tpc_input, void *tpc_output);
PHYDEVLIB_API RECIPE_RC phyTPCDumpLuts(void *phy_input, void *tpc_input, void *tpc_output);
PHYDEVLIB_API RECIPE_RC phyTPCDumpRegisters(void *phy_input, void *tpc_input, void *tpc_output);
PHYDEVLIB_API RECIPE_RC phyIsCfActive(void *phy_input, void *reg_input, void *reg_output);
PHYDEVLIB_API RECIPE_RC phyPerformPhyOff(void *phy_input, void *prog_reg_input, void *prog_reg_output);
PHYDEVLIB_API RECIPE_RC phyReadWriteVregCounter(void *phy_input, void *prog_reg_input, void *prog_reg_output);
PHYDEVLIB_API RECIPE_RC phyShutdownLdpdClockLeakage(void *phy_input, void *prog_reg_input, void *prog_reg_output);
PHYDEVLIB_API RECIPE_RC phySetSharedChainDeweight(void *phy_input, void *prog_reg_input, void *prog_reg_output);
PHYDEVLIB_API RECIPE_RC phyGetSetRxState(void *phy_input, void *prog_reg_input, void *prog_reg_output);
PHYDEVLIB_API RECIPE_RC phyGetCorrOnOffStatus(void *phy_input, void *prog_reg_input, void *prog_reg_output);
PHYDEVLIB_API RECIPE_RC phyUpdateClpcPowerOffset(void *phy_input, void *prog_reg_input, void *prog_reg_output);
PHYDEVLIB_API RECIPE_RC phySensitivityAdjustment(void *phy_input, void *prog_reg_input, void *prog_reg_output);
PHYDEVLIB_API RECIPE_RC phyGetBoardId(void *phy_input, void *prog_reg_input, void *prog_reg_output);
PHYDEVLIB_API RECIPE_RC phyRSTCal(void *phy_input, void *rst_cal_input, void *rst_cal_output);
PHYDEVLIB_API RECIPE_RC phyForceRxGain(void *phy_input, void *prog_rfa_input, void *prog_rfa_output);
PHYDEVLIB_API RECIPE_RC phySetGcMemSel(void *phy_input, void *prog_rfa_input, void *prog_rfa_output);
PHYDEVLIB_API RECIPE_RC phyLoadVRIniCommon(void *phy_input, void *m3_input, void *m3_output);
PHYDEVLIB_API RECIPE_RC phyLoadVRIniModal(void *phy_input, void *m3_input, void *m3_output);
PHYDEVLIB_API RECIPE_RC phyuCodeBoot(void *phy_input, void *m3_input, void *m3_output);
PHYDEVLIB_API RECIPE_RC phyForceTxGainTableOverride(void *phy_input, void *reset_input, void *reset_output);
PHYDEVLIB_API RECIPE_RC phyWsiNmiWar(void *phy_input, void *wsi_input, void *wsi_output);
PHYDEVLIB_API RECIPE_RC phyForceRxGainTbl(void *phy_input, void *prog_reg_input, void *prog_reg_output);
PHYDEVLIB_API RECIPE_RC phyEnablePhyNOC(void *phy_input, void *prog_reg_input, void *prog_reg_output);
PHYDEVLIB_API RECIPE_RC phyGetSetRxDeSense(void *phy_input, void *prog_reg_input, void *prog_reg_output);
PHYDEVLIB_API RECIPE_RC phyGetOTPPatchStatus(void *phy_input, void *input, void *output);
PHYDEVLIB_API RECIPE_RC phyConfigHwDtimSlna(void *phy_input, void *rfa_input, void *rfa_output);
PHYDEVLIB_API RECIPE_RC phyDisableSynth(void *phy_input, void *rfa_input, void *rfa_output);
PHYDEVLIB_API RECIPE_RC phyDebugCmdHandler(void *phy_input, void *cmd_input, void *cmd_output);
PHYDEVLIB_API RECIPE_RC phySetSpectralShapingSelect(void *phy_input, void *spectral_shaping_select_input,
                                                    void *spectral_shaping_select_output);
PHYDEVLIB_API RECIPE_RC phyCoexConfigSemaphore(void *phy_input, void *coex_input, void *coex_output);
PHYDEVLIB_API RECIPE_RC phyConfigTd320(void *phy_input, void *reset_input, void *reset_output);
PHYDEVLIB_API RECIPE_RC phyEventCapture(void *phy_input, void *event_capture_input, void *event_capture_output);
PHYDEVLIB_API RECIPE_RC phySetlowerTxGainIdx(void *phy_input, void *setlowertxgain_input, void *setlowertxgain_output);
PHYDEVLIB_API RECIPE_RC phyIcicCal(void *phy_input, void *icic_cal_input, void *icic_cal_output);
PHYDEVLIB_API RECIPE_RC phyRxSpurCal(void *phy_input, void *rxspur_cal_input, void *rxspur_cal_output);
PHYDEVLIB_API RECIPE_RC phySwitchCLPC(void *phy_input, void *switch_clpc_input, void *switch_clpc_output);
PHYDEVLIB_API RECIPE_RC phyRRICompleteImageSaveRestore(void *phy_input, void *rri_input, void *rri_output);
PHYDEVLIB_API RECIPE_RC phyRRICompleteCheck(void *phy_input, void *rri_input, void *rri_output);
PHYDEVLIB_API RECIPE_RC phyConfigXoPower(void *phy_input, void *rfa_input, void *rfa_output);
PHYDEVLIB_API RECIPE_RC phyConfigCgim(void *phy_input, void *cgim_input, void *cgim_output);
PHYDEVLIB_API RECIPE_RC phyConfigChainMaskCsr(void *phy_input, void *chain_mask_input, void *chain_mask_output);
PHYDEVLIB_API RECIPE_RC phyAdcCapture(void *phy_input, void *adc_capture_input, void *adc_capture_output);
PHYDEVLIB_API RECIPE_RC phyDbgDump(void *phy_input, void *dbg_dump_input, void *dbg_dump_output);
PHYDEVLIB_API RECIPE_RC phySetChannelFast(void *phy_input, void *set_channel_input, void *set_channel_output);
PHYDEVLIB_API RECIPE_RC phyPcssQ6SwLog(void *phy_input, void *pcss_input, void *pcss_output);
PHYDEVLIB_API RECIPE_RC phyPcssIUConfig(void *phy_input, void *pcss_input, void *pcss_output);
PHYDEVLIB_API RECIPE_RC phyModeSwitch(void *phy_input, void *mode_switch_input, void *mode_switch_output);
PHYDEVLIB_API RECIPE_RC phyTPCSetMaxTargetPwr(void *phy_input, void *settpc_input, void *settpc_output);
PHYDEVLIB_API RECIPE_RC phyCxmDbgCount(void *phy_input, void *cxm_input, void *cxm_output);
#if defined(CONFIG_PHYDEVLIB_API_Q5EXT)
PHYDEVLIB_API RECIPE_RC phyProgDacOsr(void *phy_input, void *prog_dacosr_input, void *prog_dacosr_output);
PHYDEVLIB_API RECIPE_RC phyProgDeltaSlope(void *phy_input, void *prog_delta_slope_input, void *prog_delta_slope_output);
PHYDEVLIB_API RECIPE_RC phyProgFEM(void *phy_input, void *prog_fem_input, void *prog_fem_output);
PHYDEVLIB_API RECIPE_RC phyProgFreqDepINI(void *phy_input, void *prog_freq_dep_ini_input,
                                          void *prog_freq_dep_ini_output);
PHYDEVLIB_API RECIPE_RC phyResetHw(void *phy_input, void *reset_hw_input, void *reset_hw_output);
PHYDEVLIB_API RECIPE_RC phySetBW(void *phy_input, void *set_bw_input, void *set_bw_output);
PHYDEVLIB_API RECIPE_RC phySetMode(void *phy_input, void *set_mode_input, void *set_mode_output);
PHYDEVLIB_API RECIPE_RC phySetPhyClk(void *phy_input, void *set_phy_clk_input, void *set_phy_clk_output);
PHYDEVLIB_API RECIPE_RC phyCombCal(void *phy_input, void *comb_cal_input, void *comb_cal_output);
PHYDEVLIB_API RECIPE_RC phyIterCalMultitone(void *phy_input, void *iter_cal_multitone_input,
                                            void *iter_cal_multitone_output);
PHYDEVLIB_API RECIPE_RC phyIterCombCal(void *phy_input, void *iter_comb_cal_input, void *iter_comb_cal_output);
PHYDEVLIB_API RECIPE_RC phyLoadBDF(void *phy_input, void *load_bdf_input, void *load_bdf_output);
PHYDEVLIB_API RECIPE_RC phySysTx(void *phy_input, void *sys_tx_input, void *sys_tx_output);
PHYDEVLIB_API RECIPE_RC phySysRx(void *phy_input, void *sys_rx_input, void *sys_rx_output);
PHYDEVLIB_API RECIPE_RC phyRxDCOHwCal(void *phy_input, void *rxdco_hw_cal_input, void *rxdco_hw_cal_output);
PHYDEVLIB_API RECIPE_RC phyRxFltCal(void *phy_input, void *rxflt_cal_input, void *rxflt_cal_output);
PHYDEVLIB_API RECIPE_RC phyRxIQCal(void *phy_input, void *rxiq_cal_input, void *rxiq_cal_output);
PHYDEVLIB_API RECIPE_RC phyTPCCal(void *phy_input, void *tpc_cal_input, void *tpc_cal_output);
PHYDEVLIB_API RECIPE_RC phyTxCLCal(void *phy_input, void *txcl_cal_input, void *txcl_cal_output);
PHYDEVLIB_API RECIPE_RC phyTxIQCal(void *phy_input, void *txiq_cal_input, void *txiq_cal_output);
PHYDEVLIB_API RECIPE_RC phyAllCal(void *phy_input, void *all_cal_input, void *all_cal_output);
PHYDEVLIB_API RECIPE_RC phyTlvCapture(void *phy_input, void *tlv_capture_input, void *tlv_capture_output);
PHYDEVLIB_API RECIPE_RC phyFixRxGain(void *phy_input, void *fix_rxgain_input, void *fix_rxgain_output);
PHYDEVLIB_API RECIPE_RC phyInitChanMemo(void *phy_input, void *init_chan_memo_input, void *init_chan_memo_output);
PHYDEVLIB_API RECIPE_RC phyAgcHistory(void *phy_input, void *agc_history_input, void *agc_history_output);
PHYDEVLIB_API RECIPE_RC phyAdcCaptInMemConfig(void *phy_input, void *adc_capt_in_mem_config_input,
                                              void *adc_capt_in_mem_config_output);
PHYDEVLIB_API RECIPE_RC phyAdcCaptInMemDump(void *phy_input, void *adc_capt_in_mem_dump_input,
                                            void *adc_capt_in_mem_dump_output);
PHYDEVLIB_API RECIPE_RC phyDPDIterCalMultitone(void *phy_input, void *dpd_iter_cal_multitone_input,
                                               void *dpd_iter_cal_multitone_output);
PHYDEVLIB_API RECIPE_RC phyDacPlayback(void *phy_input, void *dac_playback_input, void *dac_playback_output);
PHYDEVLIB_API RECIPE_RC phyRFPkDetDcoCal(void *phy_input, void *rf_pkdet_dco_cal_input, void *rf_pkdet_dco_cal_output);
PHYDEVLIB_API RECIPE_RC phyPCSSEventLogging(void *phy_input, void *pcss_event_logging_input,
                                            void *pcss_event_logging_output);
PHYDEVLIB_API RECIPE_RC phyTxFDCapture(void *phy_input, void *txfd_capture_input, void *txfd_capture_output);
PHYDEVLIB_API RECIPE_RC phyDFSViTest(void *phy_input, void *dfs_vi_test_input, void *dfs_vi_test_output);
PHYDEVLIB_API RECIPE_RC phyRxTDCapture(void *phy_input, void *rxtd_capture_input, void *rxtd_capture_output);
PHYDEVLIB_API RECIPE_RC phyRxGainCal(void *phy_input, void *rx_gain_cal_input, void *rx_gain_cal_output);
PHYDEVLIB_API RECIPE_RC phyCalInfoDump(void *phy_input, void *cal_info_dump_input, void *cal_info_dump_output);
PHYDEVLIB_API RECIPE_RC phyBootSeq(void *phy_input, void *boot_seq_input, void *boot_seq_output);
PHYDEVLIB_API RECIPE_RC phyMemDump(void *phy_input, void *mem_dump_input, void *mem_dump_output);
PHYDEVLIB_API RECIPE_RC phyDebugPhyRf(void *phy_input, void *debug_input, void *debug_output);
PHYDEVLIB_API RECIPE_RC phyLoadM3(void *phy_input, void *load_M3_input, void *load_M3_output);
PHYDEVLIB_API RECIPE_RC phySpecScan(void *phy_input, void *ss_input, void *ss_output);
PHYDEVLIB_API RECIPE_RC phyRttChanCaptureEn(void *phy_input, void *cfrcir_input, void *cfrcir_output);
PHYDEVLIB_API RECIPE_RC phyCLPC(void *phy_input, void *clpc_input, void *clpc_output);
PHYDEVLIB_API RECIPE_RC phySendNMIToM3(void *phy_input, void *nmitom3_input, void *nmitom3_output);
PHYDEVLIB_API RECIPE_RC phyStopPhydbgCapture(void *phy_input, void *stopphydbgcap_input, void *stopphydbgcap_output);
PHYDEVLIB_API RECIPE_RC phySetPcss(void *phy_input, void *pcss_input, void *pcss_output);
#endif
#endif

PHYDEVLIB_API void phySetMuUserPosition(PHYDEVLIB_PHY_INPUT *phyInput, uint32_t group_id, uint32_t user_pos);
PHYDEVLIB_API void phySetM3PID(PHYDEVLIB_PHY_INPUT *phyInput, uint32_t pid);
PHYDEVLIB_API uint32_t getPhyOffset(uint8_t phyid);

PHYDEVLIB_API uint32_t phyGetUcodeBuildId(PHYDEVLIB_PHY_INPUT *phyInput);
PHYDEVLIB_API void phyDisableSscanTimer(PHYDEVLIB_PHY_INPUT *phyInput);
PHYDEVLIB_API void phyM3DisableRx(PHYDEVLIB_PHY_INPUT *phyInput);
PHYDEVLIB_API uint32_t phyGetM3IpcMemLocOffset(PHYDEVLIB_PHY_INPUT *phyInput);

PHYDEVLIB_API RECIPE_RC phySetRssiOffset(PHYDEVLIB_PHY_INPUT *phyInput, int8_t rssiDbToDbmOffset);
PHYDEVLIB_API uint8_t phyGetRssiOffset(PHYDEVLIB_PHY_INPUT *phyInput);
PHYDEVLIB_API void phyRssiTempCompensation(PHYDEVLIB_PHY_INPUT *phyInput,
                                           PHYDEVLIB_RSSI_DB_TO_DBM_INPUT *rssiDbToDbmInput);

PHYDEVLIB_API void phyGetPhyBase(PHYDEVLIB_PHY_INPUT *phyInput);
PHYDEVLIB_API uint32_t phyGetPhyOffset(uint32_t phyId);
PHYDEVLIB_API void phyGetRfaChainUsage(PHYDEVLIB_PHY_INPUT *phyInput, uint32_t *parm1, uint32_t *parm2);
PHYDEVLIB_API int phyRegPollDisable(void);

PHYDEVLIB_API uint8_t phyGetPrimaryChanCode(uint8_t bwCode, uint16_t freq, uint16_t band_center_freq1,
                                            uint16_t band_center_freq2);

PHYDEVLIB_API RECIPE_RC phyConfigDCM(void *phy_input, void *dcm_input, void *dcm_output);

/* PHY register access APIs */
PHYDEVLIB_API void phyHwioSetup8(phyHwioRead8 pHwioRead, phyHwioWrite8 pHwioWrite);
PHYDEVLIB_API void phyHwioSetup16(phyHwioRead16 pHwioRead, phyHwioWrite16 pHwioWrite);
PHYDEVLIB_API void phyHwioSetup32(phyHwioRead32 pHwioRead, phyHwioWrite32 pHwioWrite);
PHYDEVLIB_API void phyHwioSetup64(phyHwioRead64 pHwioRead, phyHwioWrite64 pHwioWrite);

#if !defined(PHYDEVLIB_IMAGE_MM_FTM)
PHYDEVLIB_API uint32_t phyRegRead32(uint32_t addr);
PHYDEVLIB_API uint64_t phyRegRead64(uint32_t addr);
PHYDEVLIB_API void phyRegWrite32(uint32_t addr, uint32_t data);
PHYDEVLIB_API void phyRegWrite64(uint32_t addr, uint64_t data);
#endif

PHYDEVLIB_API void phyMemSetup(phyMemRead pMemRead, phyMemWrite pMemWrite);
PHYDEVLIB_API void phyMRead(uint32_t addr, uint8_t *buf, uint32_t size);
PHYDEVLIB_API void phyMWrite(uint32_t addr, uint8_t *buf, uint32_t size);

PHYDEVLIB_API void phyHwioScatterSetup(phyHwioScatterRead pRead, phyHwioScatterWrite pWrite);
PHYDEVLIB_API void phyScatterRead(uint32_t *addr, uint32_t *data, uint32_t size);
PHYDEVLIB_API void phyScatterWrite(uint32_t *addr, uint32_t *data, uint32_t size);

PHYDEVLIB_API RECIPE_RC phyDevLibDebugCmd_handler(void *phy_input, void *cmd_input, void *cmd_output);

#endif /* _PHY_DEVLIB_API_H */
