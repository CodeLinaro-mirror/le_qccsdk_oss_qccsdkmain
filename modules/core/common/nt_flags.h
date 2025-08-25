/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * @brief feature flag definitions of NT code base
 */

/* Flags structure   :

                                                          +---------->
                                                          |	  	   			  	 +----------+
                                                          |					  	 | Neutrino |
                                                          |					  	 +----------+
                                                          |							  !
                  +--------------------------+			|				+------------------------------------+
                  | Hardware Version control |------------|				| Controlled by NT_CHIP Version flag |
                  +--------------------------+			|				+------------------------------------+
                                                          |							  !
                                                          |				+-----------------------------+
                                                          |				| NT_CHIP Version (v1.0/v2.0) |
                                                          |				+-----------------------------+
                                                          +----------->				  !
------------------------------------------------------------------------------------------------------------------------------------------------------------
                                          +---------->								  !
                                          |							  +-----------------------------------+
                                          |							  | Controlled by NT_CHIP_HALPHY FLAG |
                                          |							  +-----------------------------------+
                                          |											  !
     +--------------------+				|							+------------------------------------------------+
     | Platform selection |--------------	|							! !
     +--------------------+	 			|					 +----------+ +---------+ |				     | Disabled |
| Enabled | |			       	 +----------+								         +---------+
                                          |					      !						 			  	  	          !
                                          |			      +------------+ +------------+ |			      | Rumi build
|								        | Chip build |
                                          +----------->     +------------+ +------------+
--------------------------------------------------------------------!-----------------------------------------------------!------------------------------------
                              +---------->				    	!											          !
                              |						+-----------------------------+
+-----------------------------+ |						| Controlled by NT_DEBUG Flag |       			       |
Controlled by NT_DEBUG Flag | |						+-----------------------------+
+-----------------------------+ |   	   	   	   	   	   	!					  ! !						 ! | !
!					 			  !				    	 !
                              |					   +---------+			 +----------+					+---------+
+----------+
                              |					   | Enabled |			 | Disabled |					| Enabled |       	 	| Disabled
| |					   +---------+			 +----------+					+---------+			    +----------+
+---------------------+         |                          !                      !							    !
! | Build configuration |	--------|						   !				      !					 		    !
!
+---------------------+			|		  			 +-------------+	  +------------------+ +-------------+
+------------------+ |			  		 | Debug build |	  | Production build |		  	   | Debug build | |
Production build | |			  		 +-------------+ 	  +------------------+	 	  	   +-------------+
+------------------+ |   	   	   	   	   	   !					  !  			            	 ! ! |
+-----------------------+    +---------------------+       +-----------------------+     +---------------------+ |
|1) WLAN Feature Flags, |    |WLAN Feature Flags,  | 	   |1) WLAN Feature Flags, |	 |WLAN Feature Flags,  | | |
system Feature Flags,|    |System Feature Flags,|	   |  system Feature Flags,|     |System Feature Flags,| |
|  App Flags= NT_FN_*	|    |App Flags= NT_FN_*   |	   |  App Flags= NT_FN_*   |     |App Flags= NT_FN_*   | |
|2) Test Feature Flags= |	 +---------------------+	   |2) Test Feature Flags= |     +---------------------+ |
|  NT_TST_*             |								   |   NT_TST_*            |
                              |			+-----------------------+ +-----------------------+
                              +---------->

----------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/

#ifndef _NT_FLAGS_H_
#define _NT_FLAGS_H_

//#include "fwconfig_cmn.h"

/* The below will enable the functionality related to cMEM minimal build and it
 * will disable the PBL as well as SBL functionality.
 * */
//#define NT_CMEM_BUILD
#ifndef NT_CHIP_VERSION
#define NT_CHIP_VERSION 1
#endif

#ifdef NT_CMEM_BUILD /*cMEM Build*/

/* Enables the "heap-stats" command. Use the command to get the heap used and available information.
 * Also enables "clear-stats" command. Use the command to clear the heap usage information. */
#define NT_TU_HEAP_STATS

/* Enables the "task-stats" command. Use the command to get the state of tasks and stack used information */
#define NT_TU_TASK_STATS

/**
 * Define NT_FN_PDC Flag to enable PDC
 */
#ifndef NT_FN_PDC
#define NT_FN_PDC
#endif

/* Set NT_FN_QC_HEAP to 0 to use FreeRTOS' heap_4 Heap Manager .
 * Set NT_FN_QC_HEAP to 1 to use QC Heap Manager.
 * Set NT_FN_QC_HEAP to 2 to use QC Heap Manager with bin code .*/
#ifndef NT_FN_QC_HEAP
#define NT_FN_QC_HEAP 1
#endif

// flag for enable the logger module
#ifndef NT_FN_LOGGER
#define NT_FN_LOGGER
#endif

/**
 * Enable performance tools by defining NT_TST_PERF_TOOL.
 * if not defined tools will be excluded from build.
 */
#ifndef NT_TST_PERF_TOOL
#define NT_TST_PERF_TOOL 1
#endif /* NT_TST_PERF_TOOL */

/**
 * Enable respective performance tools by setting the value to 0 or 1 accordingly.
 * if NT_TST_PERF_TOOL not defined tools will be excluded and CLI will not be the part of build.
 */
#if NT_TST_PERF_TOOL
/* iperf tool flag. */
#define NT_TST_LWIPER_TOOL 1
/* udp tool flag. */
#define NT_TST_UDP_TOOL 1
/* udp wmm tool flag. */
#define NT_TST_UDP_WMM_TOOL 0
/* multicast tool flag. */
#define NT_TST_MULTICAST_TOOL 0
/* ping tool flag. */
#define NT_TST_PING_TOOL 1
/* Rate set CLI flag. */
#define NT_TST_RATE_CONFIG_CLI 1
/* IP configure CLI flag. */
#define NT_TST_IP_CONFIG_CLI 1
/* system/CPU utilization CLI flag. */
#ifndef PROF_DRV
#define NT_TST_CPU_USAGE_CLI 1
#endif
#endif /* NT_TST_PERF_TOOL */
/* Flag is set to 1 for enable the fixed variables
 * Flag is set to 0 for enable the printf style (format string).
 */

#define NT_FN_FIXED_ARG_LOGGER 1
// Set NT_FN_FUNCTION_LINE_NUM_FLAG to 1, flag for enable function name and line number.
#define NT_FN_FUNCTION_LINE_NUM_FLAG 1
/*
 * Flag to define AP Specific functionality
 */
#ifndef NT_CFG_WLAN_AP
#define NT_CFG_WLAN_AP
#endif  // NT_CFG_WLAN_AP

// Any test code added for bring up
#ifndef NT_BRINGUP_TEST
#define NT_BRINGUP_TEST
#endif  // NT_BRINGUP_TEST

// CLIs for read/write operations for register and memory
#ifndef NT_FN_RW_REG_MEM
#define NT_FN_RW_REG_MEM
#endif  // NT_FN_RW_REG_MEM

// include HalPhy in the build if needed
#ifdef CHIP_HALPHY_CMEM_DEBUG
#define NT_NEUTRINO_1_0_INI      // Only halphy init, nothing else
#define NT_NEUTRINO_1_0_SYS_MAC  // sys/mac for the chip
#define NT_NEUTRINO_1_0
#endif  // CHIP_HALPHY_CMEM_DEBUG
#define NT_FN_QURT 1

#define NT_NOR_CLI

#else /*RRAM Build*/
#ifndef PLATFORM_FERMION
/* The below flag will enable the image update feature
 * through UART as well as JTAG */
#ifndef NT_MULTI_IMAGE
#define NT_MULTI_IMAGE
#endif
#endif /* PLATFORM_FERMION */

#if 0  // for reorganize
	/** Flag for setting SDK configuration to hostless mode*/
#if !(defined NT_HOSTLESS_SDK) && !(defined NT_HOSTED_SDK)
#define NT_HOSTLESS_SDK
#endif

#ifndef NT_FN_RMF
#define NT_FN_RMF
#endif  // NT_FN_RMF

#endif  // for reorganize
// Now required by CONFIG_UNITTEST_SHELL in opensource
#if (defined(CONFIG_UNITTEST_SHELL))
#ifndef UNIT_TEST_SUPPORT
#define UNIT_TEST_SUPPORT
#endif  // UNIT_TEST_SUPPORT
#endif
/* flag for enabling PKCS11 */
#ifndef NT_FN_PKCS11
//#define NT_FN_PKCS11
#endif  // NT_FN_PKCS11

#if 0  // for reorganize
	/* flag for BCN BCMC Wakeup */
#ifndef NT_MLM_BCN_BCMC_WAKEUP
#define NT_MLM_BCN_BCMC_WAKEUP
#endif

	/* flag for enabling WPS */
#ifdef CONFIG_WPS
#ifndef NT_FN_WPS
#define NT_FN_WPS
#ifdef NT_FN_WPS
	//#define NT_FN_WPS_REG
#define NT_FN_WPS_EN
#endif
#endif  // NT_FN_WPS
#endif

/* flag for enabling XPA */
#ifndef NT_FN_XPA
#define NT_FN_XPA
#endif  // NT_FN_XPA

	/* flag for enabling WPA3 */
#ifndef NT_FN_WPA3
#define NT_FN_WPA3
#endif  // NT_FN_WPA3


/* flag for enabling HCAL test code  */
#ifndef NT_FN_HCAL_TEST
	//#define NT_FN_HCAL_TEST
#endif  // NT_FN_HCAL_TEST

#ifndef ENABLE_SEGGER_SYSTEMVIEW
	//#define ENABLE_SEGGER_SYSTEMVIEW
#endif

	/* flag to enable concurrency */
#ifndef NT_FN_CONCURRENCY
#define NT_FN_CONCURRENCY
#endif  // NT_FN_CONCURRENCY

	/* Flag to define AP Specific functionality */
#ifndef NT_CFG_WLAN_AP
#define NT_CFG_WLAN_AP
#endif  // NT_CFG_WLAN_AP

	/* Test flag for protection */
#ifndef NT_FN_PROTECTION
#define NT_FN_PROTECTION
#endif  // NT_FN_PROTECTION


    /* flag for enabling or disabling WMM feature, successful WMM negotiation enables the QOS capability.*/
#ifndef NT_FN_WMM
#define NT_FN_WMM
		/* Enables the wmm feature in dpm */
#ifndef NT_FN_DPM_WMM
#define NT_FN_DPM_WMM 1
			/* flag for enabling HT related codes */
#ifndef NT_FN_HT
#define NT_FN_HT
				/* flag for enabling or disabling AMPDU feature. AMPDU feature dependent on WMM feature
				 * so to make AMPDU work WMM feature flag should also be enabled.*/
#ifndef NT_FN_AMPDU
#define NT_FN_AMPDU
#endif  // NT_FN_AMPDU
#endif  // NT_FN_HT
#endif  // NT_FN_DPM_WMM
#endif  // NT_FN_WMM

	/* flag for enabling ADDBA transmission from STA side.
	 * AMPDU feature must be enabled to enable allow station to send ADDBA request. */

#ifdef NT_FN_AMPDU
#ifndef NT_FN_STA_ADDBA_SUPPORT
#define NT_FN_STA_ADDBA_SUPPORT
#endif  // NT_FN_STA_ADDBA_SUPPORT
#define AUTO_ADDBA_EN
#define DEFAULT_BA_TX_WIN_SIZE 8
#define DEFAULT_BA_RX_WIN_SIZE 8
#endif  // NT_FN_AMPDU

	/* flag for enabling UAPSD codes on AP side */
#ifndef NT_FN_WMM_PS_AP
#define NT_FN_WMM_PS_AP
#endif

	/* flag for enabling UAPSD codes on STA side */
#ifndef NT_FN_WMM_PS_STA
#define NT_FN_WMM_PS_STA
#endif
#endif  // for reorganize

#ifdef CONFIG_DEBUG_STATS
/* Enables the "heap-stats" command. Use the command to get the heap used and available information.
 * Also enables "clear-stats" command. Use the command to clear the heap usage information. */
#define NT_TU_HEAP_STATS

/* Enables the "task-stats" command. Use the command to get the state of tasks and stack used information */
#define NT_TU_TASK_STATS
#endif
#if 0  // for reorganize
#if (NT_CHIP_VERSION == 1)
#define NT_NEUTRINO_1_0_SYS_MAC
#endif  //(NT_CHIP_VERSION==1)
	//NT_P4_Debug_CHIP_Halphy flag can be defined only P4 depot2
#ifdef CHIP_HALPHY_FULL_DEBUG
		//NT_NEUTRINO_1_0 flag can be defined only when NT_NEUTRINO_1_0_SYS_MAC is defined
		//NT_NEUTRINO_1_0 flag should be defined to use HAL PHY APIs
#define NT_NEUTRINO_1_0
#endif  // CHIP_HALPHY_FULL_DEBUG

	//NT_FN_RA should be defined to enable rate adaptation
#ifndef NT_FN_RA
#define NT_FN_RA
		//NT_FN_SNIFFER should be defined to enable sniffer
#ifndef NT_FN_SNIFFER
#define NT_FN_SNIFFER
#endif  // NT_FN_SNIFFER
#endif  // NT_FN_RA

	//NT_FN_ROAMING should be defined to enable roaming
#ifndef NT_FN_ROAMING
#define NT_FN_ROAMING
#endif  // NT_FN_ROAMING
#endif  // for reorganize

/* Set NT_FN_QC_HEAP to 0 to use FreeRTOS' heap_4 Heap Manager .
 * Set NT_FN_QC_HEAP to 1 to use QC Heap Manager.
 * Set NT_FN_QC_HEAP to 2 to use QC Heap Manager with bin code .*/
#ifndef NT_FN_QC_HEAP
#define NT_FN_QC_HEAP 1
#endif
#if 0  // for reorganize
	//Onboarding app
#ifndef NT_FN_ONBOARDING
#define NT_FN_ONBOARDING
#endif  // NT_FN_ONBOARDING
#endif  // for reorganize
/* flag for enabling or disabling IPV6 feature.
 * If flag NT_FN_IPV6 is not defined then IPV6 is disabled.*/
#ifndef NT_FN_IPV6
#define NT_FN_IPV6
#endif /* NT_FN_IPV6 */

/* Enables the dynamic timer feature in lwip */
#ifndef NT_FN_LWIP_DYNAMIC_TIMERS
//	#define NT_FN_LWIP_DYNAMIC_TIMERS 1
#endif

/* Enable/disable DHCP IPv4 server.*/
#ifndef NT_FN_DHCPS_V4
#define NT_FN_DHCPS_V4 1
#endif

/* Enable/disable DHCP IPv4 server.*/
#ifndef NT_FN_DNS
#define NT_FN_DNS 1
#endif

/* Enable/disable DHCP6 */
#ifndef NT_FN_DHCP6
#define NT_FN_DHCP6 1
#endif

// iperf require this
#define NT_TST_MULTICAST_IPV6_EN 1

/* Enable performance tools by defining NT_TST_PERF_TOOL.
 * if not defined tools will be excluded from build.*/
#if (defined(CONFIG_NT_DEMO))
#ifndef NT_TST_PERF_TOOL
#define NT_TST_PERF_TOOL 1
#endif /* NT_TST_PERF_TOOL */
#endif
//#endif
/**
 * Enable respective performance tools by setting the value to 0 or 1 accordingly.
 * if NT_TST_PERF_TOOL not defined tools will be excluded and CLI will not be the part of build.
 */
#if NT_TST_PERF_TOOL
/* iperf tool flag. */
#define NT_TST_LWIPER_TOOL     1
/* udp tool flag. */
#define NT_TST_UDP_TOOL        0
/* udp wmm tool flag. */
#define NT_TST_UDP_WMM_TOOL    1
/* multicast tool flag. */
#define NT_TST_MULTICAST_TOOL  1
/* ping tool flag. */
#define NT_TST_PING_TOOL       1
/* Rate set CLI flag. */
#define NT_TST_RATE_CONFIG_CLI 1
/* IP configure CLI flag. */
#define NT_TST_IP_CONFIG_CLI   1
/* system/CPU utilization CLI flag. */
#ifndef PROF_DRV
#define NT_TST_CPU_USAGE_CLI 1
#endif
/**
 * Enable IPV6 multicast support for test in multicast tool
 */
#if defined(NT_TST_MULTICAST_TOOL) && defined(NT_FN_IPV6)
#define NT_TST_MULTICAST_IPV6_EN 1
#else
#define NT_TST_MULTICAST_IPV6_EN 0
#endif /* defined(NT_TST_MULTICAST_TOOL) && defined(NT_FN_IPV6) */
#endif /* NT_TST_PERF_TOOL */

/* Define NT_FN_PDC Flag to enable PDC */
#ifndef NT_FN_PDC
#define NT_FN_PDC
#endif
#if 0  // for reorganize error
	/*Enable it to configure wifi_config register with PDC for controlling the wlan_mac,phy_tx,phy_rx,phy_rxtop,wur_cntl bits
	 *When enabled, this will let the resource and clients to be created for controlling the wifi_config register  */
#ifdef NT_FN_SOCPM_CTRL
#define NT_FN_SOCPM_CTRL
#endif

	/* Define NT_FN_CC_MGMT Flag to enable coin cell battery management module */
#ifdef NT_FN_CC_MGMT
#define NT_FN_CC_MGMT
#endif

	/* Enable NT_FN_WUR_STA to enable WUR feature for Station side. If  NT_FN_WUR_STA not defined, station will not support wur feature. */

/* Not needed for Fermion image */
	/**
	 * Enable NT_FN_WUR_STA to enable WUR feature for Station side. If  NT_FN_WUR_STA not defined, station will not support wur feature.
	 */
#ifndef NT_FN_WUR_STA
#define NT_FN_WUR_STA
#endif

	/* Enable NT_FN_WUR_AP to enable WUR feature for Ap side. If  NT_FN_WUR_AP not defined, Ap will not support wur feature.*/
#ifndef NT_FN_WUR_AP
#define NT_FN_WUR_AP
#endif

	/* Enable this flag WNM power save feature.*/
#ifndef NT_FN_WNM_POWERSAVE_MODE
#define NT_FN_WNM_POWERSAVE_MODE
#endif /* NT_FN_WNM_POWERSAVE_MODE */

	/* Enable this flag TWT power save feature.*/
#ifndef NT_FN_TWT
#define NT_FN_TWT
#endif /* NT_FN_TWT */

	/* Enable the File system feature for the Neutrino */
#ifndef NT_FN_LFS
	//#define NT_FN_LFS
#endif
#endif  // for reorganize
/*Flag for enabling dev configs*/
#ifndef NT_FN_CONFIG
#define NT_FN_CONFIG
#endif

// flag for enable the logger module
#ifndef NT_FN_LOGGER
#define NT_FN_LOGGER
/* Below flag NT_FN_CRTLOG_FS will change the critical log
 * location between file system and cMEM. If flag is set to
 * 1 then critical log will be in file system, if flag is
 * set to 0 then it will be in cMEM.
 * */
#define NT_FN_CRTLOG_FS 0
#endif

/* Flag is set to 1 for enable the fixed variables
 * Flag is set to 0 for enable the printf style (format string).*/
#define NT_FN_FIXED_ARG_LOGGER           1

/* Set NT_LOGS_BUFF_CLR_DEBUG_FUTURE to 1, and use "log_buffer_clr" command. clear all of the bytes in the specified
 * buffer.*/
#define NT_FN_LOGS_BUFF_CLR_DEBUG_FUTURE 1

// Set NT_FN_FUNCTION_LINE_NUM_FLAG to 1, flag for enable function name and line number.
#define NT_FN_FUNCTION_LINE_NUM_FLAG     1

// flag for enable the watchdog module
//	#ifndef NT_FN_WATCHDOG
//	#define NT_FN_WATCHDOG
//	#endif

// Define this flag to enable CPR debug prints in Production
#ifdef NT_FN_CPR_DEBUG
#define NT_FN_CPR_DEBUG
#endif  // NT_FN_CPR_DEBUG

// flag for enable the rram module
#ifndef NT_FN_RRAM
#define NT_FN_RRAM
#endif
#if 0  // for reorganize
#ifndef NT_FN_FTM
#define NT_FN_FTM
#endif

#ifdef NT_FN_FTM
#ifndef NT_FN_FTM_2016V
#define NT_FN_FTM_2016V
#endif

#ifndef NT_FN_RTT_FTM_DBG
#define NT_FN_RTT_FTM_DBG
#endif

#ifndef NT_FN_FTM_11V
	    //#define NT_FN_FTM_11V
#endif
#endif
#endif  // for reorganize
/* Enable QuRT APIs for thread and queue */
#define NT_FN_QURT 1
#if 0  // for reorganize
	/* flag to enable production flags */
#ifndef NT_FN_PRODUCTION_STATS
#define NT_FN_PRODUCTION_STATS
#endif
#endif  // for reorganize

#ifdef CONFIG_DPM_STATS
#if 0  // for reorganize

	/* flag for AP HAL and DPH production stats */
#ifndef NT_FN_AP_HAL_DPH_PRODUCTION_STATS
#define NT_FN_AP_HAL_DPH_PRODUCTION_STATS
#endif

	/* flag for STA HAL and DPH production stats */
#ifndef NT_FN_STA_HAL_DPH_PRODUCTION_STATS
#define NT_FN_STA_HAL_DPH_PRODUCTION_STATS
#endif

	/* flag for AP HAL and DPH debug stats */
#ifndef NT_FN_AP_HAL_DPH_DEBUG_STATS
#define NT_FN_AP_HAL_DPH_DEBUG_STATS
#endif

	/* flag for STA HAL and DPH debug stats */
#ifndef NT_FN_STA_HAL_DPH_DEBUG_STATS
#define NT_FN_STA_HAL_DPH_DEBUG_STATS
#endif
#endif  // for reorganize

/* flag for DPM Debug Code */
#ifndef NT_FN_DPM_DEBUG
#define NT_FN_DPM_DEBUG
#endif
#endif

/* flag for mbedtls app */
#if (defined NT_FN_DHCP6) && (defined NT_FN_DNS)
#ifndef NT_FN_MBEDTLS_APP
#define NT_FN_MBEDTLS_APP
#endif
#endif

/* flag for HTTP Support */
#ifndef NT_FN_HTTP_FLAG
#define NT_FN_HTTP_FLAG
#endif

/* flag for HTTPS Support */
#if defined(NT_FN_MBEDTLS_APP) && defined(NT_FN_HTTP_FLAG)
#define NT_FN_HTTPS_FLAG
#endif

/* flag for HTTPs_client */
#ifdef NT_FN_MBEDTLS_APP
#define HTTPS_CLIENT
#endif

/* flag for HTTPs_Server */
#if defined(NT_FN_MBEDTLS_APP) && defined(NT_FN_HTTPS_FLAG)
//			#define HTTPS_SERVER
#endif

/** Flag for AWS MQTT Client Handling */
#ifndef NT_FN_AWS_MQTT_CLIENT_APP
//#define NT_FN_AWS_MQTT_CLIENT_APP
#endif

/*system monitoring module enable disale*/
#ifdef NT_FN_SYSMON
#define NT_FN_SYSMON
#endif

#ifndef NT_MISSION_FACTORY_MODE
#define NT_MISSION_FACTORY_MODE
#endif

//#ifdef APP_RCLI_EN
//#define NT_RCLI
//#endif

#ifdef CONFIG_HEAP_STATISTIC
#define NT_HEAP_RCD_CNT 600
#endif
#ifndef NT_TST_HEAP
#define NT_TST_HEAP
#endif

#ifdef NT_FN_RRAM
//#define NT_TST_RRAM
#endif

// for reorganize 2023-11-21
#ifdef NT_HOSTED_SDK

/*for mbedtls app*/
#undef NT_FN_AWS_MQTT_CLIENT_APP
#undef NT_FN_HTTP_FLAG
#undef NT_FN_HTTPS_FLAG
#undef NT_FN_MBEDTLS_APP

/* perf tools*/
#undef NT_TST_LWIPER_TOOL
//#undef NT_TST_PING_TOOL
#undef NT_TST_UDP_TOOL
#undef NT_TST_UDP_WMM_TOOL
#undef NT_TST_MULTICAST_TOOL

/* flag use for getting LWIP statistics on host side. */
#if defined(NT_HOSTED_SDK) && defined(NT_DEBUG)
#define NT_TST_LWIP_STATS
#endif
#endif  // NT_HOSTED_SDK

#ifdef NT_HOSTED_SDK
//#undef NT_HOSTLESS_SDK
//#define NT_TST_FN_RMF
//#undef NT_TST_FN_WPA_IE
#undef NT_TST_FN_PROTECTION
#define NT_TU_HEAP_STATS
#define NT_TU_TASK_STATS
#undef NT_FN_COMMISSIONING_APP
//#undef NT_TST_TIME_STAMP_ENABLE
#undef NT_FN_ARDUINO_HOST
//#undef NT_FN_AP_HAL_DPH_DEBUG_STATS
//#undef NT_FN_STA_HAL_DPH_DEBUG_STATS
#undef NT_TST_ARP_CACHE
#undef NT_TST_DNS_CACHE
//		#define NT_TST_HEAP
//		#undef NT_FN_SOCPM_CTRL
//#undef NT_FN_CC_MGMT

/* Enable the spi slave feature for the neutrino */
#ifndef NT_FN_SPI
#define NT_FN_SPI
#endif

#define NT_GPIO_FLAG
//		#define NT_FN_WNM_POWERSAVE_MODE
#define NT_FN_DEBUG_STATS

//#define NT_TST_PERF_TOOL	1
//#define NT_TST_CPU_USAGE_CLI	1
#endif  // NT_HOSTED_SDK

#ifdef NT_DEBUG /* Test Flags */

#ifdef NT_FN_INTER_PING_INTERVAL
#define NT_FN_INTER_PING_INTERVAL
#endif

// commissioning app
#ifdef NT_FN_COMMISSIONING_APP
//	#undefine NT_FN_COMMISSIONING_APP
#endif  // NT_FN_COMMISSIONING_APP

/* Enables the "time stamp" command.
 * Use the command to get processing time of packet during performance test.
 * command are "tm_read" and "tm_reset".*/
#ifndef NT_TST_TIME_STAMP_ENABLE
//#define NT_TST_TIME_STAMP_ENABLE
#endif /* NT_TST_TIME_STAMP_ENABLE */

/* Enable this flag to include power save debug code */
#ifndef NT_FN_DEBUG_PWRSV
//#define NT_FN_DEBUG_PWRSV //wifi
#endif

/* flag to enable debug stats */
#ifndef NT_FN_DEBUG_STATS
//#define NT_FN_DEBUG_STATS  //wifi
#endif

#ifndef NT_TST_ARP_CACHE
#define NT_TST_ARP_CACHE
#endif  // NT_ARP_CACHE

#if !defined(NT_TST_DNS_CACHE) && defined(NT_FN_DNS)
#define NT_TST_DNS_CACHE
#endif  //! defined(NT_TST_DNS_CACHE) && defined(NT_FN_DNS)

#ifndef NT_PKT_THLD_NOTIFICATION
//#define NT_PKT_THLD_NOTIFICATION
#endif  // NT_PKT_THLD_NOTIFICATION

#ifndef NT_TST_FN_RMF
//#define NT_TST_FN_RMF  //wifi
#endif  // NT_TST_FN_RMF

/* flag for enabling generation of WPA IE as OUI on Neutrino:AP */
#ifndef NT_TST_FN_WPA_IE
//#define NT_TST_FN_WPA_IE  //wifi
#endif  // NT_TST_FN_WPA_IE

#ifdef NT_FN_PROTECTION
#ifndef NT_TST_FN_PROTECTION
#define NT_TST_FN_PROTECTION
#endif
#endif

#ifdef NT_TST_CC_ADDITIONAL_CONFIG_FLAG
#define NT_TST_CC_ADDITIONAL_CONFIG_FLAG
// enable foot switch
#ifdef NT_CC_DEBUG_FLAG
//#define NT_CC_DEBUG_FLAG
#endif
// adjust tx power
#ifdef NT_CC_TX_PWR_ADJUST_FLAG
//#define NT_CC_TX_PWR_ADJUST_FLAG
#endif
#endif

#ifdef NT_FN_RA
#ifdef CONFIG_RA_DEBUG
//	#define NT_FN_RA_TEST
#endif
#endif

// CLIs for read/write operations for register and memory
#ifndef NT_FN_RW_REG_MEM
#define NT_FN_RW_REG_MEM
#endif  // NT_FN_RW_REG_MEM

#endif  // NT_DEBUG

// WDT enable only prodution builds
#ifndef NT_DEBUG
#ifndef NT_FN_WATCHDOG
//#define NT_FN_WATCHDOG //wifi
#endif  // NT_FN_WATCHDOG
#endif  // NT_DEBUG
// CPR
#ifndef NT_FN_CPR
#define NT_FN_CPR  // non-wifi but pwr close source, haven't change yet
#endif             // NT_FN_CPR

#ifndef NT_DEBUG
#ifndef NT_FN_RW_REG_MEM
#define NT_FN_RW_REG_MEM
#endif  // NT_FN_RW_REG_MEM
#endif  // NT_DEBUG

#if (NT_CHIP_VERSION == 2)

#ifdef NT_2_FAST_QSPI
#define NT_2_FAST_QSPI
#endif

#ifdef NT_2_FAST_QCSPI
#define NT_2_FAST_QCSPI
#endif
#ifndef NT_GPIO_FLAG  // for Neutrino_2 ifndef
#define NT_GPIO_FLAG  // non-wifi but pwr close source, haven't change yet
#endif
/*disabling sysmon (battery monitoring and temperature monitoring) for neutrino 2*/
#undef NT_FN_SYSMON
#endif  //(NT_CHIP_VERSION==2)

/* support of cbc and calibration data management */
#ifndef HALPHY_CBC_SUPPORT
//#define HALPHY_CBC_SUPPORT//wifi
#endif  // HALPHY_CBC_SUPPORT

#endif /*RRAM Build*/
#ifdef IMAGE_FERMION
//#undef NT_FN_WUR_AP
//#undef NT_FN_WUR_STA
#undef NT_FN_AWS_MQTT_CLIENT_APP
#undef NT_MISSION_FACTORY_MODE
#undef NT_MLM_BCN_BCMC_WAKEUP

#ifdef PLATFORM_FERMION
#undef NT_FN_SPI
#undef NT_FN_CPR
//#undef NT_NEUTRINO_1_0_SYS_MAC
//#undef NT_NEUTRINO_1_0
#endif  // PLATFORM_FERMION

/* flag for enabling H/W Crypto */
#ifndef NT_FN_HW_CRYPTO
#define NT_FN_HW_CRYPTO
#endif  // NT_FN_HW_CRYPTO
#endif  // IMAGE_FERMION

#endif  // _NT_FLAGS_H_
