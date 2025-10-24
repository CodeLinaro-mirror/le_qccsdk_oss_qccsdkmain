/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*========================================================================
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 * @file fwconfig_QCP7321.h
 * @brief feature flag definitions of NT code base required for Fermion
 * ======================================================================*/
#ifndef _QCP7321_H_
#define _QCP7321_H_
/*------------------------------------------------------------------------
 * Include Files
 * ----------------------------------------------------------------------*/
/* None*/

/*------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * ----------------------------------------------------------------------*/

#ifdef IMAGE_FERMION

/* Minimal Build to test in Emulation */
// #define MINIMAL_BUILD

#define CONFIG_CHANNEL_SCHEDULER
/* There are issues with fake sleep which needs to be properly fixed, adding this WAR for now
   as a fix for CR 3404556, will be removed once fake sleep is fixed*/
#define FAKE_SLEEP_WAR
/*Feature flag to support Extended Channel Switch announcement on sta Side*/
#define FEATURE_STA_ECSA
/*Feature flag to support Extended Channel Switch announcement on AP Side*/
#define FEATURE_AP_ECSA

#define ENABLE_TWT_EVENT_LOGGING

/* MM FTM Switch feature flag, enabled for fermion */
//#define FTM_MM_MODE_SWITCH_ENABLED

#define HALPHY_FTM /* Enable Factory Test Mode Feature */

#define SUPPORT_REGULATORY
#undef REGULATORY_TEST_FRAMEWORK
//#define REGULATORY_TEST_FRAMEWORK

/* Feature flag to support Ring Interface */
#ifdef CONFIG_RING_IF
#define SUPPORT_RING_IF
#endif

/* Feature flag to support Ring Interface */
#ifdef CONFIG_RING_IF_ONLY
#define SUPPORT_RING_IF_ONLY
//#define SUPPORT_RING_IF_DEBUG /* Use this flag for heavy logs in Ring IF */
#define SUPPORT_RING_IF_STATS
#define SUPPORT_QCSPI_SLAVE /*Flag to enable QcSPI Slave driver*/
#endif

#ifdef CONFIG_SAP_POWERSAVE
#define SUPPORT_SAP_POWERSAVE    /* This flags enables changing beacon interval to supports sap powersave when it is \
                                    conneced to EB*/
#endif

/* Enables Random Backoff for QoS Null frames triggered using SW template method */
#define ENABLE_RBO_FOR_QOS_NULL

//#define SUPPORT_RING_IF_DEBUG /* Use this flag for heavy logs in Ring IF */
//#define SUPPORT_RING_IF_STATS
#ifdef SUPPORT_RING_IF
#define SUPPORT_DATA_LOOPBACK /* Use this flag to enable data loopback */

#define SUPPORT_BEACON_MISS_THRESHOLD_TIME    /* This flags enables BMTT configuration to calculate the number of \
                                                 acceptable beacon miss threshold */
#define SUPPORT_PERIODIC_TSF_SYNC             /* Feature flag to support periodic TSF sync */
#ifdef SUPPORT_PERIODIC_TSF_SYNC
#define ENABLE_TSF_SYNC_STATS /*Enable tsf stats to be printed */
#endif
#ifdef CONFIG_NT_RCLI
#define SUPPORT_RCLI_OVER_SPI
#define SUPPORT_RAWETH_IPERF
#endif
#endif

/* Feature flag to support Qtimer based High Res Sw Timer */
#define SUPPORT_HIGH_RES_TIMER

/*Flag to enable Fermion logger*/
#ifdef CONFIG_NT_RCLI
#define SUPPORT_FERMION_LOGGER
#endif

#ifdef CONFIG_RING_IF
#ifdef PLATFORM_NT
#define SUPPORT_QCSPI_ON_DWSPI /* Neutrino SPI slave that simulates the QCSPI functionalities */
#else
#define SUPPORT_QCSPI_SLAVE /*Flag to enable QcSPI Slave driver*/
#endif                      // PLATFORM_NT

#define NT_FN_SPI
#endif

#ifdef CONFIG_BOARD_QCC730_QSPI_ENABLE
#if (CONFIG_BOARD_QCC730_QSPI_ENABLE == 1)
#define SUPPORT_QSPI_MASTER
#endif
#endif
#undef SUPPORT_UNIT_TEST_CMD
#ifdef SUPPORT_UNIT_TEST_CMD
#define HRES_TIMER_UNIT_TEST
#endif

/* I2C module support flag */
#define I2C_SUPPORT
#ifdef I2C_SUPPORT
#define I2C_DEMO
#define I2C_DEMO_DBG
#define I2C_QAPI
#define I2C_DRV
//#define	I2C_DRV_DBG
#define I2C_HAL
#endif

/* UART module support flag */
#if defined(CONFIG_UART_SHELL) || (CONFIG_UART_QAPI)
#define UART_SUPPORT
#endif

#ifdef UART_SUPPORT
#define UART_QAPI
#define UART_DRV
#endif

#ifdef CONFIG_QTIMER
#define QTMR_SUPPORT
#define QTMR_DEMO
#define QTMR_DRV
#define QTMR_HAL
#endif

#ifdef CONFIG_PROF
#define PROF_DEMO
#ifdef PROF_DEMO
#define PROF_DRV
#define PROF_TEST_INST
#endif
#define PROF_DRV
#ifdef PROF_DRV
#define PROF_DRV_OS_REMOVE_IRQ
#endif
#endif

#define RRAM_PD_WAR /* WAR for cache corruption issue */

#define NT_SOCPM_SW_MTUSR

#define SUPPORT_5GHZ
#define SUPPORT_TWT_STA
#define SUPPORT_TWT_AP
#define TWT_WAR  // WAR Added for TWT Changes

#define NT_DXE_TX_HANG_WAR_FERM_727  // WAR for Dxe mgmt Tx channel hang - FERM727

#define DXE_ERROR_WAR  // WAR added for DXE error seen in powersave
//#define FERMION_CONFIG_HCF //Get config from INI region
#define BTQM_ERROR_WAR  // WAR added for BTQM error

#define FERMION_ANI_SW_SUPPORT /* Use this flag to enable ANI SW support */
#define FERMION_ANI_DEBUG      /* Disable this flag to disable ANI asserts /debug logs */
/* to enable dynamic EDCCA adaptation with NF variance, disabled as per system's team recommendation */
//#define ANI_EDCCA_ADAPTATION

#define SUPPORT_EVENT_HANDLERS

#ifdef CONFIG_COEX_PTA
/*Disabling coex PTA feature till used in future*/
#define SUPPORT_PTA_COEX
#endif

#ifdef CONFIG_COEX_MCI
#define SUPPORT_COEX
#undef SUPPORT_COEX_SIMULATOR

/*
WAR_COEX_HEAVY_BT_WL_CONNECTING_FREERUN
---------------------------------------
* Use Freerun instead of static pm when BT is running Inquiry/LEScan/Page
* This is because, during connection, chop sched schedules only one home channel req
* While wlan connecting and coex in static pm, the BT op and home ch op should alternate
* Since home chan req is a one shot, BT op is scheduled forever and assoc fails
*/
#define WAR_COEX_HEAVY_BT_WL_CONNECTING_FREERUN

#ifdef SUPPORT_COEX
#ifndef SUPPORT_EVENT_HANDLERS
#define SUPPORT_EVENT_HANDLERS
#endif
//#define SUPPORT_5G_BT_WLAN_CONCURRENCY
#define SUPPORT_XPAN_COEX

#endif /*SUPPORT_COEX */
#endif

#ifndef WAR_DUP_DET
#define WAR_DUP_DET
#endif /* WAR_DUP_DET */

#define COMPENSATE_AON_PROG_DELAY

#define ENABLE_MCS4_RX /* enable MCS4 RX for 2.0 HW to be able to solve IOP issue */

#ifdef NT_FN_LFS
#undef NT_FN_LFS /* LFS is not needed for Fermion Image */
#endif
#endif /* IMAGE_FERMION */

#ifdef PLATFORM_FERMION

#define MEM_CPY_VIA_DXE /*use dxe to do cpy */

#ifdef SUPPORT_RING_IF
/* Feature flag to enable periodic wake and sleep when EB is connected to third party AP */
#define FEATURE_PERIODIC_WAKE_SLEEP
#endif /* SUPPORT_RING_IF */

/* Support pmu dtop reg retention in SOC sleep for ANI use case*/
#define PMU_REG_RETENTION_STATUS_FOR_SOC_SLP

/* fermion OTP map support which is different from Neutrino */
#define FERMION_OTP_SUPPORT

#define FR_HWIO_WAR
#define FERMION_QTIMER_WAR
/* Data and functions that access offloaded EB data*/
//#define EB_OFFLOADS

/*Feature flag to support dynamic change of response rate when connected to XPAN
  and updating EDCA configuration when recieving appropriate frame from SAP*/
//#define FEATURE_RATE_AND_EDCA_CONFIG

/*Feature flag to support acknowlegement from hardware when a frame is sent out*/
//#define FEATURE_TX_COMPLETE
#define FERMION_ANI_HW_SUPPORT  /* Use this flag to enable ANI HW support for Fermion */
#define FERMION_ANI_DEBUG_STATS /* use this flag to enable additional stats collection for interference debug*/

/* Implemented for CR3763141 to avoid collision between Null-Tx and DL data at lower RSSIs
 * Currently disabling it as BMPS ITO enhancements will help avoiding this collision
 * To be revisited in case if the issue hit again */
#if (defined(FERMION_ANI_SW_SUPPORT) && defined(FERMION_ANI_HW_SUPPORT))
//#define HALPHY_CS20_ADAPTATION /* Done under A2NI beacon handlers */
#endif

#define HALPHY_5G_MIL_WAR /* WAR for VIFERMION-457 */

/* flag to enable the feature which will trigger the calibration in FTM in case current
   temperature goes below or above a defined value */
#define TEMP_BASED_RECAL_SUPPORT
/* Support China regulatory domain band edge requirement, WAR for FERM-807 */
//#define SRRC_BAND_EDGE_SUPPORT

#define FEATURE_INDEF_DEEP_SLP
#define UNIT_TEST_WAKELOCK
/* To Enable GPIO retention in MCU Sleep */
#define GPIO_RETENTION_IN_SLP
/* To Force BBPLL LOCK in Ram minimal code for all clock configurations */
#define FORCE_BBPLL_LOCK

/* This flags enables rssi brach threshold monitor in DTIM sleep and exit with the same reason to give event for host */
#define SUPPORT_RSSI_BREACH_THRESHOLD_MONITOR

/* EVM is degrading by 2dB for 5G/6G channels in Fermion 2.0.2 if CX voltage is set to 545mv
    during Cold Boot Calibration
    WAR for VIFERMION-490 where CPR will be initialized after Cold Boot Calibration
*/
#define CBC_CX_VOLTAGE_WAR

/*
When enter BMPs, default WQ switched from WQ12 to WQ11. When exit BMPs,
 default WQ need to be switched back to WQ12 to prevent transferring MGMT frames to WQ11
*/
#define WAR_RESTORE_DPU_DEFAULT_WQ_12_ON_EXIT_FROM_BMPS

#ifdef NT_DEBUG
/* To Enable JTAG debugging post MCU sleep */
// #define FEATURE_FERMION_SLP_DBG
/* To Enable SOCPM Ram minimal code debugging */
// #define SOCPM_RMC_DBG
/* Data for debugging potential issues related to TBTT estimation */
#define WLAN_BMPS_TBTT_DEBUG
#endif  // NT_DEBUG
/* Support the TX packets from host during BMPS sleep by
 * doing top's down wakeup to flush the datapath
 */
#define SUPPORT_DATAPATH_FLUSH_BEFORE_BMPS_SLEEP
#define SUPPORT_SWTMR_TO_WKUP_FROM_BMPS   /* Support timers to wake up from BMPS */
#define SUPPORT_SLEEP_LIST_IMPROVEMENTS   /* Support sleep list improvements */
#define SUPPORT_SLEEP_DEBUG_UNIT_TEST_CMD /* Support to test sleep list and SWDTIM improvements features */

/* Support IMPS timer after disconnection(if no connection happens for recnx_wait_time_ms
 * the device should enter IMPS And IMPS entry sequence starts from Idle task*/

#define SUPPORT_IMPS_IMPROVEMENTS

#ifdef SUPPORT_IMPS_IMPROVEMENTS

/* Enable ENABLE_IMPS_TIMER_ON_BOOTUP feature after automation testings
 * Support IMPS timer after boot up (if no connection happen for cnx_wait_time_ms
 * the device should enter IMPS */
#undef ENABLE_IMPS_TIMER_ON_BOOTUP

#endif /* SUPPORT_IMPS_IMPROVEMENTS */

//#define WAR_NO_TXC_FOR_TWT_ACTION

/* Feature flag to co-ordinate with host for wake up and sleep using A2F and F2A signals
 * Supported only on PLATFORM_FERMION */
#define FIRMWARE_APPS_INFORMED_WAKE

/* Basic light sleep soc and hal mac receipes are not under any flag
 * its enabled by default */

/* Feature flag to enable light sleep for TWT*/
//#define SUPPORT_LIGHT_SLEEP_FOR_TWT

/* Feature flag to enable HDM module in hardware to
 * initiate RRI parallel to CPU reset*/
#define SUPPORT_HDM_INITIATED_RRI

#if defined(SUPPORT_LIGHT_SLEEP_FOR_TWT)
/* Feature flag to consider going to different sleep
 * modes - clk gated, mcu, light sleep based on sleep time at mlme level
 * this needs to be enabled with TWT */
//#define SUPPORT_TWT_SLEEP_SOLVER
#endif /*SUPPORT_LIGHT_SLEEP_FOR_TWT*/

/* Feature flag to consider going to different sleep
 * modes - clk gated, mcu, light sleep based on sleep time at SOC level*/
//#define SUPPORT_SOC_SLEEP_SOLVER

/* flag for RRAM write VIA DXE */
#define RRAM_WRITE_VIA_DXE
//#define RRAM_WRITE_VIA_DXE_DEBUG
/* War flag for power issues seen in Fermion 1.0 HW. To be removed for Fermion 2.0 */
// #define FERMION_1_0_POWER_WAR

/* War flag for power issues seen in Fermion*/
#define FERMION_POWER_WAR

/* War flag for TXP TPE busy issues seen in Fermion bmps*/
#define FERMION_TXP_TPE_WAR

#ifdef EMULATION_BUILD

#define EMULATION_WAR
#define FERMION_EMU_CLK_SCALING 16

#else
#define PMU_TS_CONFIGURATION /* APIs to configure and get temperature */
// Features only for Silicon
/* Flag to enable Sleep Clock Calibration in Active Mode
 * and necessary configuration to enable sleep mode cal */
#define SLEEP_CLK_CAL_IN_ACTIVE_MODE
#ifdef SLEEP_CLK_CAL_IN_ACTIVE_MODE
#define APPLY_SLEEP_CLK_CORRECTION
/* Flag to enable Sleep Clock Calibration in Sleep Mode */
#define SLEEP_CLK_CAL_IN_SLEEP_MODE
#endif /* SLEEP_CLK_CAL_IN_ACTIVE_MODE */
#ifdef SLEEP_CLK_CAL_IN_ACTIVE_MODE
/* WAR flag to compensate the RC clock division error for
 * Fermion 2.0 new timer implementation */
#define COMPENSATE_RC_DIVISION_ERROR_WAR
#endif /* SLEEP_CLK_CAL_IN_ACTIVE_MODE */

#define FERMION_TEMP_COMP_SUPPORT /* Adjust the SCPC finegain offset based on temperature to maintain TPC accuracy */
#define TEMP_BASED_TARGET_POWER   /* Target Power will be changed based on temperature by changing the SCPC offset */
#define FERMION_ANI_SW_SUPPORT    /* Use this flag to enable ANI SW support */

#define PLATFORM_INIT_PMIC

/*For PS purpose, default WIFI_SS will be RXB_LISTEN and WIFI_SS will be controlled by HW.
Changes for SW to configure WIFI_SS in right mode(CFG/RXB_LISTEN/RXA/TX) before any PHY register access are under this
flag */
#define PHY_POWER_SWITCH

#endif  // EMULATION_BUILD

//#define SUPPORT_STA_TWT_RENEG
//#define SUPPORT_AP_TWT_RENEG

/* Feature flag to enable RRI restoration in non-polled mode, where restoration
 * occurs in parallel to other SW execution*/
#define SUPPORT_SW_NON_POLLED_RRI

#endif  // PLATFORM_FERMION

#ifndef NT_GPIO_FLAG
#define NT_GPIO_FLAG
#endif

/* Use this flag to enable Fermion Debug Infra */
#ifndef FEATURE_FDI
#define FEATURE_FDI
#endif

#ifndef FEATURE_FPCI
#define FEATURE_FPCI
#define FPCI_DEBUG (0)
#endif

/* This flag enables recovery of BMU once a BMU error occurs */
#define SUPPORT_BMU_ERROR_RECOVERY

/* Check data activity after DPM stop during BMPS entry and abort sleep if necessary */
#define BMPS_ENTRY_ABORT_ON_ACTIVITY_POST_ITO

#endif  // _QCP7321_H_
