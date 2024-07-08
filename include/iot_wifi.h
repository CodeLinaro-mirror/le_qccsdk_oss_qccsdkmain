/*
 * Amazon FreeRTOS WiFi V1.0.3
 * Copyright (C) 2018 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://aws.amazon.com/freertos
 * http://www.FreeRTOS.org
 */

/**
 * @file iot_wifi.h
 * @brief Wi-Fi Interface.
 */

#ifndef _AWS_WIFI_H_
#define _AWS_WIFI_H_

#include <stdint.h>

/* FreeRTOS include for BaseType_t. */
#include "portmacro.h"


/* Wi-Fi configuration includes. */
#include "aws_wifi_config.h"
#include "nt_socpm_sleep.h"
#include "ieee80211_defs.h"
#include "wmi.h"
#include "fwconfig_cmn.h"
#include "nt_flags.h"

/**
 * @brief Return code denoting API status.
 *
 * @note Codes other than eWiFiSuccess are failure codes.
 * @ingroup WiFi_datatypes_enums
 */
typedef enum
{
	WIFI_DISCONNECTED_STATE = 0x0,
	WIFI_CONNECTED_STATE = 0x1,
	WIFI_IDLE_STATE = 0x2,
	WIFI_INVALID_STATE,
}WLAN_CONN_STATE;


#ifdef SUPPORT_RING_IF
/*-------------------------------------------------------------------------
* For any changes made in this definition corrosponding must also be done
* in evt_dispatcher_fn_table otherwise the event dispatcher functionality
* will break
*-------------------------------------------------------------------------*/
#endif
typedef enum
{
	aws_app_event_id = 0x1,
	ard_app_event_id,
	aws_app_vendor_id,
	disconnect_event_id,
	cnx_failure_event_id,
	pdev_utf_event_id,
	set_mode_event_id,
	wps_app_event_id,
	wifi_reset_event_id,
	wps_dissassoc_event_id,
	dhcp_success_event_id,
	ping_event_id,
	cnx_success_event_id,
	roam_success_event_id,
	wifi_enable_event_id,
	wifi_disable_event_id,
	wifi_interface_add_event_id,
	wifi_scan_comp_event_id,
	wifi_scan_fail_event_id,
	wifi_set_param_event_id,
	wifi_scan_start_event_id,
	wifi_scan_stop_fail_event_id,
	wifi_scan_stop_pass_event_id,
//#ifdef SUPPORT_UNIT_TEST_CMD
    wifi_unit_test_event_id,
//#endif
#ifdef SUPPORT_TWT_STA
    wifi_twt_setup_event_id,
    wifi_twt_terminate_event_id,
    wifi_twt_status_event_id,
#endif
#ifdef SUPPORT_RING_IF
    wifi_phydbgdump_event_id,
#endif
#ifdef SUPPORT_SAP_POWERSAVE
    wifi_update_bi_event_id,
#endif
    wifi_set_reset_wakelock_event_id,
#ifdef SUPPORT_PERIODIC_TSF_SYNC
    wifi_periodic_tsf_sync_start_event_id,
    wifi_periodic_tsf_sync_event_id,
#endif
#ifdef FIRMWARE_APPS_INFORMED_WAKE
    wifi_f2a_pulse_on_twt_wakeup_event_id,
#endif /* FIRMWARE_APPS_INFORMED_WAKE */
#ifdef SUPPORT_BEACON_MISS_THRESHOLD_TIME
    wifi_update_bmtt_event_id,
#endif /* SUPPORT_BEACON_MISS_THRESHOLD_TIME */
#ifdef SUPPORT_COEX
    wifi_coex_event_id,
#endif
	wifi_periodic_traffic_setup_event_id, /* event when periodic wake and sleep session setup is done */
	wifi_periodic_traffic_teardown_event_id, /* event when periodic wake and sleep session is teardown */
	wifi_periodic_traffic_status_event_id, /* periodic wake status event which sends wake interval and next_sp_tsf */
	wifi_clk_latency_event_id, /* event when the clock latency is set */
	invalid_app_event_id = 0xff,
}event_t;
typedef enum
{
    eWiFiNotSupported = 0, /**< Not supported. */
    eWiFiSuccess = 1,      /**< Success. */
    eWiFiTimeout = 2,      /**< Timeout. */
    eWiFiFailure = 3,      /**< Failure. */
} WIFIReturnCode_t;

/**
 * @brief Wi-Fi Security types.
 *
 * @ingroup WiFi_datatypes_enums
 */
typedef enum
{

    eWiFiSecurityOpen = 0,    		/**< Open - No Security. */
    eWiFiSecurityWEP,         		/**< WEP Security. */
    eWiFiSecurityWPA,         		/**< WPA Security. */
    eWiFiSecurityWPA2,        		/**< WPA2 Security. */
	eWiFiSecurityMixed,       		/**< Mixed Mode. */
	eWiFiSecurityWPA3,        		/**< WPA3 Security. */
	eWiFiSecurityWPA3_transition, 	/**< WPA3_transition. */
	eWifiSecurityAny,         /**< Any Security Mode */
    eWiFiSecurityNotSupported 		/**< Unknown Security. */

} WIFISecurity_t;

/**
 * @brief Wi-Fi device modes.
 *
 * Device roles/modes supported.
 * @ingroup WiFi_datatypes_enums
 */
typedef enum
{
    eWiFiModeStation = 0, /**< Station mode. */
    eWiFiModeAP,          /**< Access point mode. */
	eWifiModeConcurrentAP_STA,      /**< concurrent AP and STA mode */
    eWiFiModeNotSupported /**< Unsupported mode. */
} WIFIDeviceMode_t;

/**
 * @brief Wi-Fi device power management modes.
 *
 * Device power management modes supported.
 * @ingroup WiFi_datatypes_enums
 */
typedef enum
{
    eWiFiPMNormal = 0,  /**< Normal mode. */
    eWiFiPMLowPower,    /**< Low Power mode. */
    eWiFiPMAlwaysOn,    /**< Always On mode. */
    eWiFiPMNotSupported /**< Unsupported PM mode. */
} WIFIPMMode_t;

/**
 *@brief Wi-Fi device roaming priorities.
 *
 * @ingroup roam_priority_types_enums
 */
//Roaming priority types
typedef enum { //temporary declarartion maybe shifted to roaming api.h
	rssi = 1, rate, periodic, mixed
} roam_trigger_types;

//probe type
typedef enum {
	active_probe = 1,
    passive_probe
}probe_type;

//ssid scan type
typedef enum {
	all_ssid= 0x0,
	specific_ssid = 0x01,
	any_profile = 0x20,
}scan_type;

#define DEFAULT_CHAN_NUM	1
#ifdef CONFIG_WIFILIB_6GHZ
#define DEV_CHANNEL_NUM_MAX         76
#elif defined(SUPPORT_5GHZ)
#define DEV_CHANNEL_NUM_MAX         52
#else /* SUPPORT_5GHZ */
#define DEV_CHANNEL_NUM_MAX         11
#endif /* CONFIG_WIFILIB_6GHZ */
/**
 * @brief Parameters passed to the WIFI_ConnectAP API for connection.
 *
 * @see WIFI_ConnectAP
 *
 * @ingroup WiFi_datatypes_paramstructs
 */
typedef struct
{
    char  pcSSID[wificonfigMAX_SSID_LEN];      /**< SSID of the Wi-Fi network to join. */
    uint8_t ucSSIDLength;     /**< SSID length not including NULL termination. */
    char  pcPassword[wificonfigMAX_PASSPHRASE_LEN];  /**< Password needed to join the AP. */
    uint8_t ucPasswordLength; /**< Password length not including NULL termination. */
    WIFISecurity_t xSecurity; /**< Wi-Fi Security. @see WIFISecurity_t. */
    uint8_t cChannelList[DEV_CHANNEL_NUM_MAX];          /**< Channel number. */
    uint8_t num_channels;
	uint8_t wep_type;
	uint8_t wep_key_len;
} WIFINetworkParams_t;

/*********BA & wmm related *********/
typedef struct{
	uint8_t tid : 4;
	uint8_t ampdu_status : 1 ;
	uint8_t sta_id;
	uint8_t ba_window_size;
	uint8_t dpm_tid;
	uint8_t party_type;
}WIFIBAParams_t;

typedef enum
{
wur_wakeup_frame,
wur_vendor_frame,
assoc,
deassoc,
wur_mode,
wur_assoc_sta,
wur_vendid_tx_status,
wur_wakefrm_tx_status,
wur_frame_success,
wur_frame_failure,
wur_enter_mode,
wur_exit_mode,
wur_vendor_recv,
wur_wakeup_recv,
wur_assoc_wur_enable,
wur_deassoc_wur_enable,
wnm_invalid_sleep_time,		//id for notification for invalid sleep time

#ifdef NT_FN_FTM
rtt_ftm_success,
rtt_ftm_failure,
#endif	// NT_FN_FTM
}wur_ntfy_t;


typedef struct wmm_stru{
	uint8_t mode;
	char wmm_status;
}wmm_stru;


typedef struct {
	uint8_t *wur_buffer;
	uint8_t wur_buffer_len;
}wurid_stru;
/****************************/

/**
 * @brief Wi-Fi scan results.
 *
 * Structure to store the Wi-Fi scan results.
 *
 * @note The size of char arrays are the MAX lengths + 1 to
 * account for possible null terminating at the end of the
 * strings.
 *
 * @see WIFI_Scan
 *
 * @ingroup WiFi_datatypes_returnstructs
 */
typedef struct
{
    char cSSID[ wificonfigMAX_SSID_LEN + 1 ];   /**< SSID of the Wi-Fi network with a NULL termination. */
    uint8_t ucBSSID[ wificonfigMAX_BSSID_LEN ]; /**< BSSID of the Wi-Fi network. */
    WIFISecurity_t xSecurity;                   /**< Wi-Fi Security. @see WIFISecurity_t. */
    int8_t cRSSI;                               /**< Signal Strength. */
    int8_t cChannel;                            /**< Channel number. */
    uint8_t ucHidden;                           /**< Hidden channel. */
} WIFIScanResult_t;

/**
 * @brief Wi-Fi network parameters passed to the WIFI_NetworkAdd API.
 *
 * @note The size of char arrays are the MAX lengths + 1 to
 * account for possible null terminating at the end of the
 * strings.
 *
 * @ingroup WiFi_datatypes_paramstructs
 */
typedef struct
{
    char cSSID[ wificonfigMAX_SSID_LEN + 1 ];           /**< SSID of the Wi-Fi network to join with a NULL termination. */
    uint8_t ucSSIDLength;                               /**< SSID length not including NULL termination. */
    uint8_t ucBSSID[ wificonfigMAX_BSSID_LEN ];         /**< BSSID of the Wi-Fi network. */
    char cPassword[ wificonfigMAX_PASSPHRASE_LEN + 1 ]; /**< Password needed to join the AP with a NULL termination. */
    uint8_t ucPasswordLength;                           /**< Password length not including null termination. */
    WIFISecurity_t xSecurity;                           /**< Wi-Fi Security. @see WIFISecurity_t. */
} WIFINetworkProfile_t;

typedef struct
{
	int8_t power_save_mode;
	int8_t status;

}PowerSave_t;

typedef struct
{
	int32_t imps_idle_time;
	int32_t imps_sleep_time;
	sleep_mode imps_sleep_mode;
	int8_t imps_retry_count;
}ImpsConfig_t;

typedef struct
{
	sleep_mode sleep_type;
}SleepConfig_t;

typedef struct
{
	uint8_t statusBgScan;
	int32_t period;       //frequency in seconds
	probe_type probeType; //active= '1' , passive = '2';can be made as enums
	uint8_t bgScanFreq;   //how often full scan is done
	ssid_t ssid[5];
    roam_trigger_types trigger_type;
	scan_type scanType;               /* scan all ssid (all)/specific ssid */
	uint8_t numChannels;              /* no.of channels to scan */
	uint8_t channelList[DEV_CHANNEL_NUM_MAX];    /* channels to scan */
	WIFISecurity_t xSecurity;         /**< Wi-Fi Security. @see WIFISecurity_t. */
	uint8_t cntprof;
	NT_BOOL scan_only;                // Just do scan, connection not triggered
}ScanConfig_t;

#ifdef SUPPORT_UNIT_TEST_CMD

WIFIReturnCode_t WIFI_UnitTestCmdHandle(void *msg);

#endif
/**
 * @brief Turns on Wi-Fi.
 *
 * This function turns on Wi-Fi module,initializes the drivers and must be called
 * before calling any other Wi-Fi API
 *
 * @return @ref eWiFiSuccess if Wi-Fi module was successfully turned on, failure code otherwise.
 */
/* @[declare_wifi_wifi_on] */
WIFIReturnCode_t WIFI_On(  );

WIFIReturnCode_t WIFI_SetRMFStatus(void *msg);

/**
 * @brief Sends Single Broadcast frame.
 *
 * This function sends Broadcast frame. The Wi-Fi peripheral should be put in a
 * low power or off state in this routine.
 *
 * @return @ref eWiFiSuccess if Wi-Fi module was successfully sent frame, failure code otherwise.
 */
/* @[declare_WIFI_Send_bc_frm] */

WIFIReturnCode_t WIFI_Send_bc_frm(void *msg);

#ifdef NT_TST_FN_WPA_IE
WIFIReturnCode_t WIFI_SetWPAOUI(void *msg);
#endif //NT_TST_FN_WPA_IE

/**
 *
 *
 */
WIFIReturnCode_t WIFI_SetProbedSsid(void *msg);
/* @[declare_wifi_set_probed_ssid] */

#ifdef NT_FN_ROAMING
WIFIReturnCode_t WIFI_SetBgScan(void *msg);
/* @[declare_wifi_start_bg_scan] */

WIFIReturnCode_t WIFI_StartFgScan(void *msg);
/*@[declare_wifi_start_fg_scan]*/
#endif //NT_FN_ROAMING

/* @[declare_wifi_wifi_off] */
WIFIReturnCode_t WIFI_Off( void );
/* @[declare_wifi_wifi_off] */

WIFIReturnCode_t WIFI_Target_Reset( void );

/**
 * @brief Connects to the Wi-Fi Access Point (AP) specified in the input.
 *
 * The Wi-Fi should stay connected when the same Access Point it is currently connected to
 * is specified. Otherwise, the Wi-Fi should disconnect and connect to the new Access Point
 * specified. If the new Access Point specifed has invalid parameters, then the Wi-Fi should be
 * disconnected.
 *
 * @param[in] pxNetworkParams Configuration to join AP.
 *
 * @return @ref eWiFiSuccess if connection is successful, failure code otherwise.
 *
 * **Example**
 * @code
 * WIFINetworkParams_t xNetworkParams;
 * WIFIReturnCode_t xWifiStatus;
 * xNetworkParams.pcSSID = "SSID String";
 * xNetworkParams.ucSSIDLength = SSIDLen;
 * xNetworkParams.pcPassword = "Password String";
 * xNetworkParams.ucPasswordLength = PassLength;
 * xNetworkParams.xSecurity = eWiFiSecurityWPA2;
 * xWifiStatus = WIFI_ConnectAP( &( xNetworkParams ) );
 * if(xWifiStatus == eWiFiSuccess)
 * {
 *     //Connected to AP.
 * }
 * @endcode
 *
 * @see WIFINetworkParams_t
 */
/* @[declare_wifi_wifi_connectap] */
WIFIReturnCode_t WIFI_ConnectAP( void* msg );
/* @[declare_wifi_wifi_connectap] */

/**
 * @brief Disconnects from the currently connected Access Point.
 *
 * @return @ref eWiFiSuccess if disconnection was successful or if the device is already
 * disconnected, failure code otherwise.
 */
/* @[declare_wifi_wifi_disconnect] */
WIFIReturnCode_t WIFI_Disconnect( void *msg );
/* @[declare_wifi_wifi_disconnect] */

/**
 * @brief Resets the Wi-Fi Module.
 *
 * @return @ref eWiFiSuccess if Wi-Fi module was successfully reset, failure code otherwise.
 */
/* @[declare_wifi_wifi_reset] */
WIFIReturnCode_t WIFI_Reset( void );
/* @[declare_wifi_wifi_reset] */

/**
 * @brief Sets the Wi-Fi mode.
 *
 * @param[in] xDeviceMode - Mode of the device Station / Access Point /P2P.
 *
 * **Example**
 * @code
 * WIFIReturnCode_t xWifiStatus;
 * xWifiStatus = WIFI_SetMode(eWiFiModeStation);
 * if(xWifiStatus == eWiFiSuccess)
 * {
 *     //device Set to station mode
 * }
 * @endcode
 *
 * @return @ref eWiFiSuccess if Wi-Fi mode was set successfully, failure code otherwise.
 */
/* @[declare_wifi_wifi_setmode] */
WIFIReturnCode_t WIFI_SetMode(void* msg  );
/* @[declare_wifi_wifi_setmode] */

/**
 * @brief Gets the Wi-Fi mode.
 *
 * @param[out] pxDeviceMode - return mode Station / Access Point /P2P
 *
 * **Example**
 * @code
 * WIFIReturnCode_t xWifiStatus;
 * WIFIDeviceMode_t xDeviceMode;
 * xWifiStatus = WIFI_GetMode(&xDeviceMode);
 * if(xWifiStatus == eWiFiSuccess)
 * {
 *    //device mode is xDeviceMode
 * }
 * @endcode
 *
 * @return @ref eWiFiSuccess if Wi-Fi mode was successfully retrieved, failure code otherwise.
 */
/* @[declare_wifi_wifi_getmode] */
WIFIReturnCode_t WIFI_GetMode( WIFIDeviceMode_t * pxDeviceMode );
/* @[declare_wifi_wifi_getmode] */

/**
 * @brief Add a Wi-Fi Network profile.
 *
 * Adds a Wi-fi network to the network list in Non Volatile memory.
 *
 * @param[in] pxNetworkProfile - Network profile parameters
 * @param[out] pusIndex - Network profile index in storage
 *
 * @return Index of the profile storage on success, or failure return code on failure.
 *
 * **Example**
 * @code
 * WIFINetworkProfile_t xNetworkProfile = {0};
 * WIFIReturnCode_t xWiFiStatus;
 * uint16_t usIndex;
 * strlcpy( xNetworkProfile.cSSID, "SSID_Name", SSIDLen));
 * xNetworkProfile.ucSSIDLength = SSIDLen;
 * strlcpy( xNetworkProfile.cPassword, "PASSWORD",PASSLen );
 * xNetworkProfile.ucPasswordLength = PASSLen;
 * xNetworkProfile.xSecurity = eWiFiSecurityWPA2;
 * WIFI_NetworkAdd( &xNetworkProfile, &usIndex );
 * @endcode
 */
/* @[declare_wifi_wifi_networkadd] */
WIFIReturnCode_t WIFI_NetworkAdd( const WIFINetworkProfile_t * const pxNetworkProfile,
                                  uint16_t * pusIndex );
/* @[declare_wifi_wifi_networkadd] */


/**
 * @brief Get a Wi-Fi network profile.
 *
 * Gets the Wi-Fi network parameters at the given index from network list in non-volatile
 * memory.
 *
 * @note The WIFINetworkProfile_t data returned must have the the SSID and Password lengths
 * specified as the length without a null terminator.
 *
 * @param[out] pxNetworkProfile - pointer to return network profile parameters
 * @param[in] usIndex - Index of the network profile,
 *                      must be between 0 to wificonfigMAX_NETWORK_PROFILES
 *
 * @return @ref eWiFiSuccess if the network profile was successfully retrieved, failure code
 * otherwise.
 *
 * @see WIFINetworkProfile_t
 *
 * **Example**
 * @code
 * WIFINetworkProfile_t xNetworkProfile = {0};
 * uint16_t usIndex = 3;  //Get profile stored at index 3.
 * WIFI_NetworkGet( &xNetworkProfile, usIndex );
 * @endcode
 */
/* @[declare_wifi_wifi_networkget] */
WIFIReturnCode_t WIFI_NetworkGet( WIFINetworkProfile_t * pxNetworkProfile,
                                  uint16_t usIndex );
/* @[declare_wifi_wifi_networkget] */

/**
 * @brief Delete a Wi-Fi Network profile.
 *
 * Deletes the Wi-Fi network profile from the network profile list at given index in
 * non-volatile memory
 *
 * @param[in] usIndex - Index of the network profile, must be between 0 to
 *                      wificonfigMAX_NETWORK_PROFILES.
 *
 *                      If wificonfigMAX_NETWORK_PROFILES is the index, then all
 *                      network profiles will be deleted.
 *
 * @return @ref eWiFiSuccess if successful, failure code otherwise. If successful, the
 * interface IP address is copied into the IP address buffer.
 *
 * **Example**
 * @code
 * uint16_t usIndex = 2; //Delete profile at index 2
 * WIFI_NetworkDelete( usIndex );
 * @endcode
 *
 */
/* @[declare_wifi_wifi_networkdelete] */
WIFIReturnCode_t WIFI_NetworkDelete( uint16_t usIndex );
/* @[declare_wifi_wifi_networkdelete] */

/**
 * @brief Ping an IP address in the network.
 *
 * @param[in] pucIPAddr IP Address array to ping.
 * @param[in] usCount Number of times to ping
 * @param[in] ulIntervalMS Interval in mili-seconds for ping operation
 *
 * @return @ref eWiFiSuccess if ping was successful, other failure code otherwise.
 */
/* @[declare_wifi_wifi_ping] */
WIFIReturnCode_t WIFI_Ping( uint8_t * pucIPAddr,
                            uint16_t usCount,
                            uint32_t ulIntervalMS );
/* @[declare_wifi_wifi_ping] */

/**
 * @brief Retrieves the Wi-Fi interface's IP address.
 *
 * @param[out] pucIPAddr IP Address buffer.
 *
 * @return @ref eWiFiSuccess if successful and IP Address buffer has the interface's IP address,
 * failure code otherwise.
 *
 * **Example**
 * @code
 * uint8_t ucIPAddr[ 4 ];
 * WIFI_GetIP( &ucIPAddr[0] );
 * @endcode
 */
/* @[declare_wifi_wifi_getip] */
WIFIReturnCode_t WIFI_GetIP( uint8_t * pucIPAddr );
/* @[declare_wifi_wifi_getip] */

/**
 * @brief Retrieves the Wi-Fi interface's MAC address.
 *
 * @param[out] pucMac MAC Address buffer sized 6 bytes.
 *
 * **Example**
 * @code
 * uint8_t ucMacAddressVal[ wificonfigMAX_BSSID_LEN ];
 * WIFI_GetMAC( &ucMacAddressVal[0] );
 * @endcode
 *
 * @return @ref eWiFiSuccess if the MAC address was successfully retrieved, failure code
 * otherwise. The returned MAC address must be 6 consecutive bytes with no delimitters.
 */
/* @[declare_wifi_wifi_getmac] */
WIFIReturnCode_t WIFI_GetMAC( uint8_t * pucMac );
/* @[declare_wifi_wifi_getmac] */

/**
 * @brief Retrieves the host IP address from a host name using DNS.
 *
 * @param[in] pcHost - Host (node) name.
 * @param[in] pucIPAddr - IP Address buffer.
 *
 * @return @ref eWiFiSuccess if the host IP address was successfully retrieved, failure code
 * otherwise.
 *
 * **Example**
 * @code
 * uint8_t ucIPAddr[ 4 ];
 * WIFI_GetHostIP( "amazon.com", &ucIPAddr[0] );
 * @endcode
 */
/* @[declare_wifi_wifi_gethostip] */
WIFIReturnCode_t WIFI_GetHostIP( char * pcHost,
                                 uint8_t * pucIPAddr );
/* @[declare_wifi_wifi_gethostip] */

/**
 * @brief Perform a Wi-Fi network Scan.
 *
 * @param[in] pxBuffer - Buffer for scan results.
 * @param[in] ucNumNetworks - Number of networks to retrieve in scan result.
 *
 * @return @ref eWiFiSuccess if the Wi-Fi network scan was successful, failure code otherwise.
 *
 * @note The input buffer will have the results of the scan.
 *
 * **Example**
 * @code
 * const uint8_t ucNumNetworks = 10; //Get 10 scan results
 * WIFIScanResult_t xScanResults[ ucNumNetworks ];
 * WIFI_Scan( xScanResults, ucNumNetworks );
 * @endcode
 */
/* @[declare_wifi_wifi_scan] */
WIFIReturnCode_t WIFI_Scan( WIFIScanResult_t * pxBuffer,
                            uint8_t ucNumNetworks );
/* @[declare_wifi_wifi_scan] */

/**
 * @brief Start SoftAP mode.
 *
 * @return @ref eWiFiSuccess if SoftAP was successfully started, failure code otherwise.
 */
/* @[declare_wifi_wifi_startap] */
WIFIReturnCode_t WIFI_StartAP(  );
/* @[declare_wifi_wifi_startap] */

/**
 * @brief Stop SoftAP mode.
 *
 * @return @ref eWiFiSuccess if the SoftAP was successfully stopped, failure code otherwise.
 */
/* @[declare_wifi_wifi_stopap] */
WIFIReturnCode_t WIFI_StopAP( void );
/* @[declare_wifi_wifi_stopap] */

/**
 * @brief Configure SoftAP.
 *
 * @param[in] pxNetworkParams - Network parameters to configure AP.
 *
 * @return @ref eWiFiSuccess if SoftAP was successfully configured, failure code otherwise.
 *
 * **Example**
 * @code
 * WIFINetworkParams_t xNetworkParams;
 * xNetworkParams.pcSSID = "SSID_Name";
 * xNetworkParams.pcPassword = "PASSWORD";
 * xNetworkParams.xSecurity = eWiFiSecurityWPA2;
 * xNetworkParams.cChannel = ChannelNum;
 * WIFI_ConfigureAP( &xNetworkParams );
 * @endcode
 */
/* @[declare_wifi_wifi_configureap] */
WIFIReturnCode_t WIFI_ConfigureAP( void* msg  );
/* @[declare_wifi_wifi_configureap] */

/**
 * @brief Set the Wi-Fi power management mode.
 *
 * @param[in] xPMModeType - Power mode type.
 *
 * @param[in] pvOptionValue - A buffer containing the value of the option to set
 *                            depends on the mode type
 *                            example - beacon interval in sec
 *
 * @return @ref eWiFiSuccess if the power mode was successfully configured, failure code otherwise.
 */
/* @[declare_wifi_wifi_setpmmode] */
/*WIFIReturnCode_t WIFI_SetPowerSleepMode(WIFIPMMode_t xPMModeType,const void * pvOptionValue void* msg )*/
/* @[declare_wifi_wifi_setpmmode] */

/**
 * @brief Get the Wi-Fi power management mode
 *
 * @param[out] pxPMModeType - pointer to get current power mode set.
 *
 * @param[out] pvOptionValue - optional value
 *
 * @return @ref eWiFiSuccess if the power mode was successfully retrieved, failure code otherwise.
 */
/* @[declare_wifi_wifi_getpmmode] */
WIFIReturnCode_t WIFI_GetPMMode( WIFIPMMode_t * pxPMModeType,
                                 void * pvOptionValue );
/* @[declare_wifi_wifi_getpmmode] */


/* @[declare_wifi_wifi_registernetworkstatechangeeventcallback] */

/**
 * @brief Check if the Wi-Fi is connected.
 *
 * @return pdTRUE if the link is up, pdFalse otherwise.
 */
/* @[declare_wifi_wifi_isconnected] */
//BaseType_t WIFI_IsConnected(  );
WIFIReturnCode_t WIFI_IsConnected( void* msg_query,void* msg_resp );//re-declaration
/* @[declare_wifi_wifi_isconnected] */

WIFIReturnCode_t WIFI_Wur_Info(void* msg_query);



WIFIReturnCode_t WIFI_SetWepDefKeyIdx(void* wep_key_set_cmd);

WIFIReturnCode_t WIFI_SetWepKey(void* wep_key_set_cmd);

WIFIReturnCode_t WIFI_Set_Channel(void* msg);
WIFIReturnCode_t WIFI_SetConfig(void* msg);
WIFIReturnCode_t WIFI_Get_Config(void *qry_msg,void *resp_msg);
WIFIReturnCode_t WIFI_Get80211stats(void* msg);
WIFIReturnCode_t WIFI_GetWurId( void* msg_query,void* msg_resp );
WIFIReturnCode_t WIFI_IdleTimer(void* msg);
WIFIReturnCode_t nt_set_force_dtim(void* msg);
WIFIReturnCode_t WIFI_Set_Vendor_Id(void* msg);
WIFIReturnCode_t WIFI_NTEnable(void* msg);
#ifdef NT_FN_RA
WIFIReturnCode_t WIFI_SetRaConfig(void* ra_cfg);
#endif //NT_FN_RA
WIFIReturnCode_t WIFI_Wur_Stats(void);
WIFIReturnCode_t WIFI_WurConfig(void* msg);
WIFIReturnCode_t WIFI_GetDatapathStats(void* msg);
WIFIReturnCode_t WIFi_Get_Dpm_Hal_Stats(void* msg_query,void* msg_resp);
#ifdef NT_FN_PROTECTION
WIFIReturnCode_t WIFI_SetProtMode( void* msg_queue, void* msg_resp);
#endif //NT_FN_PROTECTION
WIFIReturnCode_t WIFI_Prot_Type(void* msg_query, void* msg_resp);
WIFIReturnCode_t WIFI_SetProt_Sta( void* msg_queue, void* msg_resp);
WIFIReturnCode_t WIFI_goto_omps(void);
/**
 *	@func	WIFI_SetPowerSleepMode
 *	@brief	This function is used to select between either clockgated or mcu sleep.
 * 	@Return WIFIReturnCode_t i.e success/failure/timeout
 * 	@Param	msg
 */
WIFIReturnCode_t WIFI_SetPowerSleepMode(void* msg );
/**
 *	@func	WIFI_Wnm_Config
 *	@brief	This function is used to enable/disable wnm flag and send to wlan_wmi thorugh queue.
 * 	@Return WIFIReturnCode_t i.e success/failure/timeout
 * 	@Param	msg
 */
WIFIReturnCode_t WIFI_Wnm_Config(void* msg);
/**
 *	@func	WIFI_Set_bss_idle_time
 *	@brief	This function is used to set bss idle time and send to wlan_wmi thorugh queue.
 * 	@Return WIFIReturnCode_t i.e success/failure/timeout
 * 	@Param	msg
 */
WIFIReturnCode_t WIFI_Set_bss_idle_time(void* msg);
/**
 *	@func	WIFI_Set_sleep_time
 *	@brief	This function is used to set sleep time interval and send to wlan_wmi thorugh queue.
 * 	@Return WIFIReturnCode_t i.e success/failure/timeout
 * 	@Param	msg
 */
WIFIReturnCode_t WIFI_Set_sleep_time(void* msg);
/**
 *	@func	WIFI_Wnm_stats
 *	@brief	This function is used for wnm stats.
 * 	@Return WIFIReturnCode_t i.e success/failure/timeout
 * 	@Param	msg
 */
WIFIReturnCode_t WIFI_Wnm_stats(void* msg);
/**
 * @func.    WIFI_WPS_Set_Config
 *@brief    This function initialises the WPS module
 *@return   WIFIReturnCode_t i.e success/failure/timeout
 *@Param	NULL
 */
WIFIReturnCode_t WIFI_WPS_Credentials(void* msg);
/*
 *
 */
WIFIReturnCode_t WIFI_WPS_Set_Config(void* msg);
/**
 * @func.    WIFI_WPS_Start
 *@brief    This function starts WPS in PIN/PUSH-Mode
 *@return   WIFIReturnCode_t i.e success/failure/timeout
 *@Param	msg (PIN no.of device to be onboarded)
 */
WIFIReturnCode_t WIFI_WPS_Start(void* msg);

/**
 *	@func	WIFI_Twt_stats
 *	@brief	This function is used for twt stats.
 * 	@Return WIFIReturnCode_t i.e success/failure/timeout
 * 	@Param	msg
 */
WIFIReturnCode_t WIFI_Twt_stats(void* msg);


WIFIReturnCode_t WIFI_Enable_XPA(void *msg);

WIFIReturnCode_t WIFI_Enable_auto_BA(void *msg);

WIFIReturnCode_t WIFI_set_rate(void* msg);

WIFIReturnCode_t WIFI_cfg_rate_idx(void* msg);


#endif /* _AWS_WIFI_H_ */
