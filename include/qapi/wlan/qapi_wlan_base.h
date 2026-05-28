/* 
Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
SPDX-License-Identifier: BSD-3-Clause-Clear
*/
// $QTI_LICENSE_QDN_C$

#ifndef __QAPI_WLAN_BASE_H__
#define __QAPI_WLAN_BASE_H__

/**
 * @file qapi_wlan_base.h
 *
 * @brief WLAN base definition
 *
 * @details This section provides APIs, macros definitions, enumerations and data structures
 *          for applications to perform WLAN control operations.
 */

#include "qapi_types.h"
#include "qapi_status.h"
#include "qapi_wlan_misc.h"

/**
@ingroup qapi_wlan
Identifies the enable/disable options for WLAN.
*/
typedef enum {
    QAPI_WLAN_DISABLE_E = 0, /**< Disable the Wi-Fi module. */
    QAPI_WLAN_ENABLE_E = 1   /**< Enable the Wi-Fi module. */
} qapi_WLAN_Enable_e;

/**
@ingroup qapi_wlan
Data structure used by the application to pass wireless scan options to the driver.
If the device is connected to an AP, a successful scan returns the current AP along
with newly scanned APs.
*/
typedef struct // qapi_WLAN_Start_Scan_Params_s
{
    int32_t force_Fg_Scan;
    /**< Force a high priority scan. */
    uint32_t home_Dwell_Time_In_Ms;
    /**< Maximum scan duration in the home channel (in ms). If set to 0, the default value of 50 ms is used. */
    uint32_t force_Scan_Interval_In_Ms;
    /**< Time interval (in ms) between scanning channels from the list. If set to 0, the default value of 100 ms is
     * used.  */
    uint8_t scan_Type;
    /**< This parameter currently supports only 0 as an input value. */
    uint8_t num_Channels;
    /**< Number of channels to scan. */
    uint16_t channel_List[1];
    /**< List of channels to scan. */
    uint8_t ssid[__QAPI_WLAN_MAX_SSID_LEN];
    uint8_t ssid_Length;
} qapi_WLAN_Start_Scan_Params_t;

/**
@ingroup qapi_wlan
Data structure that the application uses to interpret the scan results for
all access point information received during the scan.

All the information in this structure is for one particular BSS found during the scan.
*/
typedef struct // qapi_WLAN_BSS_Scan_Info_s
{
    uint8_t channel;                        /**< Wireless channel. */
    uint8_t ssid_Length;                    /**< SSID length. */
    uint8_t rssi;                           /**< Received signal strength indicator. */
    uint8_t security_Enabled;               /**< 1: Security enabled; 0: Security disabled. */
    uint16_t beacon_Period;                 /**< Beacon period. */
    uint8_t preamble;                       /**< Preamble. */
    uint8_t bss_type;                       /**< BSS type. */
    uint8_t bssid[__QAPI_WLAN_MAC_LEN];     /**< BSSID. */
    uint8_t ssid[__QAPI_WLAN_MAX_SSID_LEN]; /**< SSID. */
    uint16_t rsn_Cipher;                     /**< RSN cipher. */
    uint16_t rsn_Auth;                       /**< RSN authentication. */
    uint16_t wpa_Cipher;                     /**< WPA cipher. */
    uint16_t wpa_Auth;                       /**< WPS authentication. */
    uint16_t caps;                          /**< Capability IE. */
    uint8_t wep_Support;                    /**< Supprt for WEP. */
    uint8_t reserved[3];                    /**< Reserved. */
} qapi_WLAN_BSS_Scan_Info_t;

/**
@ingroup qapi_wlan
This data type enumerates the scan status to indicate the scan is succeeded, required to rescan or failed.
*/
typedef enum {
    QAPI_WLAN_SCAN_STATUS_SUCCESS_E = 0, /*scan is succeeded*/
    QAPI_WLAN_SCAN_STATUS_REQUIRE_RESCAN_E =
        1, /*require to rescan if the scan is cancelled or aborted as the target is busy or has more urgent task*/
    QAPI_WLAN_SCAN_STATUS_FAILURE_E = 2 /*scan is failed*/
} qapi_WLAN_Scan_Status_e;

typedef struct {
    qapi_Status_t status; /* over all status */
} qapi_WLAN_Evt_Hdr_t;

typedef struct {
    qapi_WLAN_Evt_Hdr_t evt_hdr;           /* contains the common event header */
    uint8_t mac_addr[__QAPI_WLAN_MAC_LEN]; /* the assigned mac address */
    uint8_t num_networks;                  /* no of vdev supported */
    uint8_t reserved;                      /* reserved */
    uint32_t cap_info;                     /*capibility info bitmap*/
    uint32_t cap_info2;
    uint32_t reserved2; /* reserved2 */
} qapi_WLAN_Enable_Evt_t;

typedef struct {
    qapi_WLAN_Evt_Hdr_t evt_hdr; /* contains the common event header */
    uint32_t reserved;
} qapi_WLAN_Disable_Evt_t;

typedef struct {
    qapi_WLAN_Evt_Hdr_t evt_hdr; /* contains the common event header */
    uint32_t reserved;
} qapi_WLAN_If_Add_Comp_Evt_t;

typedef struct {
    qapi_WLAN_Evt_Hdr_t evt_hdr; /* contains the common event header */
    uint8_t scan_id;             /* identifier for specific scan */
    uint8_t reserved[3];
} qapi_WLAN_Scan_Start_Evt_t;

typedef struct {
    qapi_WLAN_Evt_Hdr_t evt_hdr; /* contains the common event header */
    uint8_t num_bss_cur;         /* no of bss in current structure */
    uint8_t scan_id;
    uint8_t total_bss; /* no of bss total scaned */
    uint8_t reserved2;
    qapi_WLAN_BSS_Scan_Info_t scan_bss_info[0]; /* bss info array, count is num_bss_cur */
} qapi_WLAN_Scan_Comp_Evt_t;

typedef struct {
    qapi_WLAN_Evt_Hdr_t evt_hdr;        /* contains the common event header */
    uint8_t bssid[__QAPI_WLAN_MAC_LEN]; /* bssid of the ap joined */
    uint8_t bss_Connection_Status;
    /**<
    Flag that indicates whether it is a BSS level connection/disconnection
    or an individual station-level connection/disconnection.
    - STA mode connect event -- A value of 1 indicates that the device is connected to an AP
    - STA mode disconnect event -- A value of 1 indicates that the device
                                   is disconnected from an AP
    - STA mode -- It is not expected to receive this value as 0 when
                  operating in STA mode
    - AP mode connect Event -- A value of 1 indicates that the SoftAP
                               session has been started
    - AP mode disconnect event -- A value of 1 indicates that the SoftAP
                                  session has been stopped
    - AP mode connect event -- A value of 0 indicates that a peer station
                               with a MAC address in mac_Addr
                               has connected to SoftAP
    - AP Mode disconnect event -- A value of 0 indicates that a peer
                                  station with a MAC address in mac_Addr
                                  has  disconnected from SoftAP @vertspace{-14}
    */
    uint8_t ssid_Length;                    /**< SSID length. */
    uint8_t ssid[__QAPI_WLAN_MAX_SSID_LEN]; /*ssid of joind AP */
    uint16_t assoc_id;                      /* association id */
    uint8_t host_initiated;                 /* Specify whether join is host initiated or not*/
    uint8_t reason_code;
    uint16_t channel_frequency;
    uint8_t passphrase[__QAPI_WLAN_PASSPHRASE_LEN + 1]; /* passphrase of joind AP */
    uint16_t reserved2;
} qapi_WLAN_Join_Comp_Evt_t;

typedef struct {
    qapi_WLAN_Evt_Hdr_t evt_hdr; /* contains the common event header */
    uint8_t reason;
    uint16_t freq;
    uint8_t reserved;
    uint8_t reserved2;
} qapi_WLAN_Chan_Switch_Evt_t;

typedef struct {
    qapi_WLAN_Evt_Hdr_t evt_hdr; /* contains the common event header */
    uint8_t reason;
    uint8_t reserved[3];
} qapi_WLAN_WPS_Fail_Evt_t;

/**
@ingroup qapi_wlan
WLAN driver invokes an application-registered callback function to indicate various
asynchronous events to the application. This data structure enumerates the list
of various event IDs for which the WLAN driver invokes the application-registered callback
function.
*/
typedef enum {
    QAPI_WLAN_CONNECT_CB_E = 0,
    /**< ID to indicate connect/disconnect events. */
    QAPI_WLAN_SCAN_COMPLETE_CB_E = 1,
    /**<
    ID to indicate a wireless scan complete event, received for
    nonblocking/nonbuffering scans.
    */
    QAPI_WLAN_FWD_PROBE_REQUEST_INFO_CB_E = 2,
    /**<
    ID to indicate a probe request forwarded from the WLAN firmware
    when probe request forwarding is enabled by the application.
    */
    QAPI_WLAN_RESUME_CB_INFO_CB_E = 3,
    /**<
    ID to indicate a WLAN driver/firmware resume completed event and that the
    application can start sending data over the WLAN driver after receiving
    this event (for future use).
    */
    QAPI_WLAN_PROMISCUOUS_MODE_CB_INFO_CB_E = 4,
    /**<
    ID to indicate that packets were captured in Promiscuous mode. For every
    packet received in Promiscuous mode, the driver indicates this event to
    the application. Since it is quite possible to receive packets too
    frequently in Promiscuous mode, the application is expected to do a minimal
    operation in the callback implementation.
    */
    QAPI_WLAN_DISCONNECT_CB_E = 5,
    /**<
    Currently not used. A disconnect event is indicated in the parameter
    of QAPI_WLAN_CONNECT_CB_E.
    */
    QAPI_WLAN_DEAUTH_CB_E = 6,
    /**<
    Currently not used. A disconnect event is indicated in the parameter
    of QAPI_WLAN_CONNECT_CB_E.
    */
    QAPI_WLAN_WPS_CB_E = 7,
    /**<
    ID to indication completion of a WPS handshake to the application.
    The application should use qapi_WLAN_WPS_Await_Completion() to get event information.
    */
    QAPI_WLAN_P2P_CB_E = 8,
    /**<
    ID to indicate all P2P events. Since most of the P2P event
    handling is done by the application, it is necessary for the application to
    copy necessary information from events(in an event callback) and handle this in
    the application's own thread context.
    */
    QAPI_WLAN_BSS_INFO_CB_E = 9,
    /**<
    ID to indicate that AP profile information was received when performing a nonbuffering
    scan. Each event of this type indicates only the AP's profile information.
    */
    QAPI_WLAN_TCP_KEEPALIVE_OFFLOAD_CB_E = 10,
    /**<
    ID to indicate an event for a TCP Keepalive Offload request, received
    either on termination of TCP KA offload or when a TCP timeout occurs
    for one or more of the TCP KA sessions.
    */
    QAPI_WLAN_PREFERRED_NETWORK_OFFLOAD_CB_E = 11,
    /**<
    ID to indicate that a PNO profile event was received when a profile match is
    found or when PNO is disabled by the application.
    */
    QAPI_WLAN_WNM_CB_E = 12,
    /**<
    ID to indicate events received when WNM commands, such as setting
    BSS maximum idle period, enter/exit WNM sleep complete execution.
    */
    QAPI_WLAN_CHANNEL_SWITCH_CB_E = 13,
    /**<
    ID to indicate a wireless channel change event received when STA
    changes the operating channel due to a channel switch announcement IE
    from the connected access point.
    */
    QAPI_WLAN_READY_CB_E = 14,
    /**<
    ID to indicate the event that announces the Wi-Fi module (firmware)
    is enabled and ready.
    */
    QAPI_WLAN_SUSPEND_CB_E = 15,
    /**< ID to indicate a WLAN firmware suspend event. */
    QAPI_WLAN_DRIVER_DISABLE_CB_E = 16,
    /**< ID to indicate a driver shut down complete event. */
    QAPI_WLAN_ERROR_HANDLER_CB_E = 17,
    /**< ID to indicate a fatal error in the WLAN driver. */
    QAPI_WLAN_RESUME_HANDLER_CB_E = 18,
    /**< ID to indicate to resume completion of the WLAN firmware. */
    QAPI_WLAN_RX_EAPOL_KEY_CB_E = 19,
    /**< ID to indicate to receive eapol key frame. */
    QAPI_WLAN_RX_MGMT_CB_E = 20,
    /**< ID to indicate to receive MGMT frame. */
    QAPI_WLAN_PMF_EVENT_CB_E = 21,
    /**< ID to indicate PMF event. */
    QAPI_WLAN_SAE_COMMIT_CB_E = 22,
    /**< ID to indicate to prepare SAE auth commit frame. */
    QAPI_WLAN_COUNTRY_CODE_CB_E = 23,
    /**< ID to indicate country code is set. */
    QAPI_WLAN_ENABLE_CB_E = 24,      /**< ID to indicate WLAN is enabled. */
    QAPI_WLAN_DISABLE_CB_E = 25,     /**< ID to indicate WLAN is disabled. */
    QAPI_WLAN_IF_ADD_COMP_CB_E = 26, /**< ID to indicate WLAN interface is added. */
    QAPI_WLAN_SCAN_START_CB_E = 27,  /**< ID to indicate WLAN scan is started. */
    QAPI_WLAN_WPS_FAIL_CB_E = 28,    /**< ID to indicate WLAN WPS failed. */
} qapi_WLAN_Callback_ID_e;

/**
@ingroup qapi_wlan
Data structure that presents connect event information from the driver to the
application.

The application uses this data structure to interpret the event
payload received with a QAPI_WLAN_CONNECT_CB_E event.
*/
typedef struct // qapi_WLAN_Connect_Cb_Info_s
{
    int32_t value;
    /**< TRUE: To indicate connect events \n
         FALSE: To indicate disconnect/deauthorization events
*/
    uint8_t mac_Addr[__QAPI_WLAN_MAC_LEN];
    /**<
    MAC address related to the connect event. Based on the operating mode,
    the driver fills in a different MAC addresses as follows.
    - STA mode connect event -- MAC address of the connected access point
    - STA mode disconnect event -- MAC address contains all zeroes
    - AP mode connect event -- MAC address of the device itself, or a peer
                               station that was connected to the AP
                               (based on the bss_Connection_Status parameter)
    - AP mode disconnect event -- MAC address contains zeros or a peer
                                  station that was disconnected from the AP
                                  (based on the bss_Connection_Status parameter) @vertspace{-14}
    */
    uint32_t bss_Connection_Status;
    /**<
    Flag that indicates whether it is a BSS level connection/disconnection
    or an individual station-level connection/disconnection.
    - STA mode connect event -- A value of 1 indicates that the device is connected to an AP
    - STA mode disconnect event -- A value of 1 indicates that the device
                                   is disconnected from an AP
    - STA mode -- It is not expected to receive this value as 0 when
                  operating in STA mode
    - AP mode connect Event -- A value of 1 indicates that the SoftAP
                               session has been started
    - AP mode disconnect event -- A value of 1 indicates that the SoftAP
                                  session has been stopped
    - AP mode connect event -- A value of 0 indicates that a peer station
                               with a MAC address in mac_Addr
                               has connected to SoftAP
    - AP Mode disconnect event -- A value of 0 indicates that a peer
                                  station with a MAC address in mac_Addr
                                  has  disconnected from SoftAP @vertspace{-14}
    */
    int32_t disConnReason;
    /**<
    Flag that indicates disconnect reason code.
    It has the same value with WMI_DISCONNECT_REASON (defined in wmi.h).
    The following examples are expected, typical values:

    - NO_NETWORK_AVAIL   = 0x01,
    - LOST_LINK          = 0x02,
    - DISCONNECT_CMD     = 0x03,
    - BSS_DISCONNECTED   = 0x04,
    */
} qapi_WLAN_Connect_Cb_Info_t;

/**
@ingroup qapi_wlan
Enumeration that provides various blocking options for qapi_WLAN_Set_Param()
and qapi_WLAN_Get_Param() APIs when invoked by the application.
*/
typedef enum {
    QAPI_WLAN_NO_WAIT_E = 0, /**< Makes a QAPI nonblocking. */
    QAPI_WLAN_WAIT_E = 1     /**< Makes a QAPI blocking. */
} qapi_WLAN_Wait_For_Status_e;

/**
@ingroup qapi_wlan
Enumeration that provides a list of supported operating modes for each virtual device.
*/
typedef enum {
    QAPI_WLAN_DEV_MODE_STATION_E = 0,
    /**<
    Infrastructure non-AP Station mode, supported in both Single Device
    mode and Concurrent mode. When operating in Concurrent mode, virtual
    device 1 must be used for the Station mode of operation.
    */
    QAPI_WLAN_DEV_MODE_AP_E = 1, /**< Soft-AP mode */
    /**<
    Infrastructure AP Station mode, supported in both Single Device
    mode and Concurrent mode. When operating in Concurrent mode, virtual
    device 0 must be used for the AP mode of operation.
    */
    QAPI_WLAN_DEV_MODE_ADHOC_E = 2, /* Adhoc mode */
    /**<
    Independent BSS mode of operation, supported in Single Device mode
    only. Virtual device 0 must be used for the IBSS mode of operation.
    */
    QAPI_WLAN_DEV_MODE_INVALID_E = 3 /**< Invalid device mode. */
} qapi_WLAN_Dev_Mode_e;

/**
@ingroup qapi_wlan
Enumeration that provides a list of supported 802.11 PHY modes.
*/
#if 0
typedef enum
{
    QAPI_WLAN_11A_MODE_E        = 0x1,  /**< 802.11a. */
    QAPI_WLAN_11G_MODE_E        = 0x2,  /**< 802.11g. */
    QAPI_WLAN_11AG_MODE_E       = 0x3,  /**< 802.11ag. */
    QAPI_WLAN_11B_MODE_E        = 0x4,  /**< 802.11b. */
    QAPI_WLAN_11GONLY_MODE_E    = 0x5,  /**< 802.11g only. */
} qapi_WLAN_Phy_Mode_e;
#endif
typedef enum {
    QAPI_WLAN_11B_MODE_E = 0x0,         /**< 802.11b. */
    QAPI_WLAN_11G_MODE_E = 0x1,         /**< 802.11g. */
    QAPI_WLAN_11NG_HT20_MODE_E = 0x2,   /**< 802.11b/g/n HT20. */
    QAPI_WLAN_11A_MODE_E = 0x3,         /**< 802.11a. */
    QAPI_WLAN_11A_HT20_MODE_E = 0x4,    /**< 802.11a with HT20. */
    QAPI_WLAN_11ABGN_HT20_MODE_E = 0x5, /**< 802.11 CCX. */
} qapi_WLAN_Phy_Mode_e;
/**
@ingroup qapi_wlan
Enumeration that provides 11n HT configurations.
*/
typedef enum {
    QAPI_WLAN_11N_DISABLED_E = 0x1, /**< 802.11n disabled. */
    QAPI_WLAN_11N_HT20_E = 0x2,     /**< 802.11n with bandwith 20M. */
    QAPI_WLAN_11N_HT40_E = 0x3,     /**< 802.11n with bandwith 40M. */
} qapi_WLAN_11n_HT_Config_e;

/**
@ingroup qapi_wlan
Data structure to set 11n HT configurations.
*/
typedef struct // qapi_WLAN_HT_Config_s
{
    qapi_WLAN_11n_HT_Config_e htconfig;   /**< Enumeration that provides 11n HT configurations. */
    uint8_t sgi; /**< 20M short GI enable flag. */
    uint8_t mpdu_density;   /**< MPDU density (aka Minimum MPDU Start Spacing). */
} qapi_WLAN_11n_HT_Config_t;

/**
@ingroup qapi_wlan
Enumeration of a list of supported power modes in the WLAN subsystem.
*/
typedef enum {
    QAPI_WLAN_POWER_MODE_REC_POWER_E = 1,
    /**< Power Save mode that enables both MAC and system power save. */
    QAPI_WLAN_POWER_MODE_MAX_PERF_E = 2
    /**< Maximum performance mode that disables both MAC and system power save. */
} qapi_WLAN_Power_Mode_e;

/**
@ingroup qapi_wlan
Enumeration of various WLAN modules that have the ability to request the necessary
power mode for their operation. Each of the modules here has the ability to
request Power Save mode or Maximum Performance mode. This level of granularity
mainly helps to debug mismatches between Operating Power mode and Expected Power
mode of the WLAN subsystem.
*/
typedef enum {
    QAPI_WLAN_POWER_MODULE_USER_E = 0,
    /**< If the application requires the system to be in Maximum Performance mode. */
    QAPI_WLAN_POWER_MODULE_WPS_E = 1,
    /**<
    WPS handshakes should be in Maximum Performance mode
    when the application initiates a WPS handshake. After a WPS handshake is completed,
    the application can request Power Save mode with the source module as
    WPS.
    */
    QAPI_WLAN_POWER_MODULE_P2P_E = 2,
    /**<
    Power mode set by the P2P module.
    P2P handshakes should be in Maximum Performance mode
    when the application initiates P2P control commands. After a P2P session is
    established (Group Owner or Client mode), if the device operates in GO mode,
    the application is expected to request Maximum Perfmance mode with the module
    owner as P2P and is expected to be in Maximum Performance mode until the P2P session is
    active. If the device operates in P2P Client mode, the application can
    optionally request Power Save mode with the module owner as P2P
    once the connection is established.
    */
    QAPI_WLAN_POWER_MODULE_SOFTAP_E = 3,
    /**<
    Power mode set by SoftAP module.
    Applications must operate in Maximum Performance mode as long as
    the device is operating in SoftAP mode.
    */
    QAPI_WLAN_POWER_MODULE_SRCALL_E = 4,
    /**<
    Power mode request by the store recall module. At this point, the WLAN
    driver internally uses this and the application is not expected to use it.
    */
    QAPI_WLAN_POWER_MODULE_TKIP_E = 8,
    /**<
    Power mode set by the TKIP countermeasure module. At this point, the WLAN
    driver internally uses this and the application is not expected to use it.
    */
    QAPI_WLAN_POWER_MODULE_RAW_MODE_E = 9,
    /**<
    Power mode request for sending packets in Raw mode. This is the
    mode where the application constructs and transmits packets over the WLAN radio,
    bypassing the IP stack and WLAN MLME layer.
    Applications must request Maximum Performance before initiating any
    Raw mode packet transmissions.
    */
    QAPI_WLAN_POWER_MODULE_PWR_MAX_E = 10, /**< Maximum value for modules */
} qapi_WLAN_Power_Module_e;

/**
@ingroup qapi_wlan
Data structure used to request the necessary operating power mode (Maximum
Performance or Power Save (rec_power) mode).
*/
typedef struct // qapi_WLAN_Power_Mode_Params_s
{
    qapi_WLAN_Power_Mode_e power_Mode;     /**< Power mode to be set */
    qapi_WLAN_Power_Module_e power_Module; /**< Module requesting power mode change */
} qapi_WLAN_Power_Mode_Params_t;

/**
@ingroup qapi_wlan
Enumeration of supported DTIM policies.
*/
typedef enum {
    QAPI_WLAN_DTIM_IGNORE_E = 1,
    /**<
    Non-AP station wakeups only at TIM intervals; ignores
    content after beacon (CAB).
    */
    QAPI_WLAN_DTIM_NORMAL_E = 2, /**< Non-AP station wakeups at TIM intervals. */
    QAPI_WLAN_DTIM_STICK_E = 3,  /**< Non-AP station wakeups only at DTIM intervals. */
    QAPI_WLAN_DTIM_AUTO_E = 4,   /**< Non-AP station wakeups at both TIM and DTIM intervals. */
    QAPI_WLAN_DTIM_XSTICK_E = 5  /**< Non-AP station wakeups only at X DTIM intervals. */
} qapi_WLAN_DTIM_Policy_e;

/**
@ingroup qapi_wlan
Data structure to add a pattern structure for WOW and filtering.
*/
typedef struct // qapi_WLAN_Add_Pattern_s
{
    uint32_t pattern_Index;                         /**< Pattern index. */
    uint32_t pattern_Action_Flag;                   /**< Pattern action flag. */
    uint32_t offset;                                /**< Offset. */
    uint32_t pattern_Size;                          /**< Pattern size. */
    uint16_t header_Type;                           /**< Header type. */
    uint8_t pattern_Priority;                       /**< Pattern priority. */
    uint8_t pattern_Mask[__QAPI_WLAN_PATTERN_MASK]; /**< Pattern mask. */
    uint8_t pattern[__QAPI_WLAN_PATTERN_MAX_SIZE];  /**< Pattern string. */
} qapi_WLAN_Add_Pattern_t;

/**
@ingroup qapi_wlan
Enumeration that idetifies authentication modes supported by the WLAN subsystem. The application
sets the required authentication from one of these modes using
qapi_WLAN_Set_Param() with __QAPI_WLAN_PARAM_GROUP_SECURITY_AUTH_MODE as the command
ID.

@dependencies
Authentication mode should be set before calling qapi_WLAN_Commit() to make
the authentication mode set effective.
*/
typedef enum {
    QAPI_WLAN_AUTH_NONE_E = 0, /**< Open mode authentication. */
    QAPI_WLAN_AUTH_WPA_E = 1,
    /**< Wi-Fi Protected Access v1 Protocol, if a preshared key (PSK) is not used (currently not supported). */
    QAPI_WLAN_AUTH_WPA2_E = 2,
    /**< Wi-Fi Protected Access v2 Protocol, if a PSK is not used (currently not supported). */
    QAPI_WLAN_AUTH_WPA_PSK_E = 3,
    /**< Wi-Fi Protected Access v1, if a PSK is used. */
    QAPI_WLAN_AUTH_WPA2_PSK_E = 4,
    /**< Wi-Fi Protected Access v2, if a PSK is used. */
    QAPI_WLAN_AUTH_WPA_CCKM_E = 5,
    /**< WPA v1 with Cisco Centralized Key Management (currently not supported). */
    QAPI_WLAN_AUTH_WPA2_CCKM_E = 6,
    /**< WPA v2 with Cisco Centralized Key Management (currently not supported) */
    QAPI_WLAN_AUTH_WPA2_PSK_SHA256_E = 7,
    /**< WPA2-PSK using SHA256 (currently not supported). */
    QAPI_WLAN_AUTH_WEP_E = 8,
    /**< Wired Equivalent Privacy (WEP) mode of authentication. */
    QAPI_WLAN_AUTH_WPA3_SAE_E = 9,
    /**< WPA3 SAE mode of authentication. */
    QAPI_WLAN_AUTH_WPA2_SAE_MIXED_E = 10,
    /**< WPA2 and SAE mixed mode of authentication. */
    QAPI_WLAN_AUTH_WPA_WPA2_SAE_MIXED_E = 11,
    /**< WPA, WPA2 and SAE mixed mode of authentication. */
    QAPI_WLAN_AUTH_WPA_WPA2_MIXED_E = 12,
    /**< WPA and WPA2 mixed mode of authentication. */
    QAPI_WLAN_AUTH_WPA3_EAP_ONLY_E = 13,
    /**< WPA3 Enterprise only mode of authentication. */
    QAPI_WLAN_AUTH_WPA3_EAP_TRANSITION_E = 14,
    /**< WPA2 and WPA3 Enterprise transition mode of authentication. */
    QAPI_WLAN_AUTH_INVALID_E = 15 /**< Invalid authentication method. */
} qapi_WLAN_Auth_Mode_e;

/**
@ingroup qapi_wlan
Enumeration that identifies a list of supported encryption methods. The application
sets the required encryption from one of these modes using
qapi_WLAN_Set_Param() with __QAPI_WLAN_PARAM_GROUP_SECURITY_AUTH_MODE as the command
ID.

@dependencies
Encryption mode should be set before calling qapi_WLAN_Commit() to make
the authentication mode set effective.
*/
typedef enum {
    QAPI_WLAN_CRYPT_NONE_E = 0, /**< No encryption, Open mode. */
    QAPI_WLAN_CRYPT_WEP_CRYPT_E = 1,
    /**< WEP mode of encryption. */
    QAPI_WLAN_CRYPT_TKIP_CRYPT_E = 2,
    /**< Temporal Key Integrity Protocol (TKIP). */
    QAPI_WLAN_CRYPT_AES_CRYPT_E = 3,
    /**< Advanced Encryption Standard (AES). */
    QAPI_WLAN_CRYPT_WAPI_CRYPT_E = 4,
    /**< WLAN Authentication and Privacy Infrastructure; currently not supported. */
    QAPI_WLAN_CRYPT_BIP_CRYPT_E = 5,
    /**< Broadcast Integrity Protocol; currently not supported. */
    QAPI_WLAN_CRYPT_KTK_CRYPT_E = 6,
    /**< Key Transport Key; currently not supported. */
    QAPI_WLAN_CRYPT_AUTO = 7,
    /**< Auto Select Key based on AP capability. Support TKIP & AES for now. */
    QAPI_WLAN_CRYPT_INVALID_E = 8 /**< Invalid encryption type. */
} qapi_WLAN_Crypt_Type_e;

/**
@ingroup qapi_wlan
Enumeration that identifies a list of supported 802.1x methods.
The application sets the required method from one of these modes
using qapi_WLAN_Set_Param() with __QAPI_WLAN_PARAM_GROUP_SECURITY_8021X_METHOD
as the command ID.

@dependencies
802.1x method should be set before calling qapi_WLAN_Commit()
to make the method set effective.
*/
typedef enum
{
    QAPI_WLAN_8021X_METHOD_UNKNOWN              = 0, /**< Unknown. */
    QAPI_WLAN_8021X_METHOD_EAP_TLS_E            = 1, /**< EAP_TLS. */
    QAPI_WLAN_8021X_METHOD_EAP_TTLS_MSCHAPV2_E  = 2, /**< EAP_TTLS_MSCHAPV2. */
    QAPI_WLAN_8021X_METHOD_EAP_PEAP_MSCHAPV2_E  = 3, /**< EAP_PEAP_MSCHAPV2. */
    QAPI_WLAN_8021X_METHOD_EAP_TTLS_MD5_E  = 4,     /**< EAP_TTLS_MD5. */
    QAPI_WLAN_8021X_METHOD_MAX  = 50,               /**< max. */
} qapi_WLAN_8021X_Method_e;

/**
@ingroup qapi_wlan
Data structure to set the 802.1x private key filename and its password.\n
Both Private_Key_filename and Private_Key_Password are ASCII.

@sa
__QAPI_WLAN_PARAM_GROUP_SECURITY_8021X_PRIVATE_KEY
qapi_WLAN_Set_Param
*/
typedef struct //qapi_WLAN_Security_8021x_Private_Key_s
{
    char    *Private_Key_filename;     /**< Point to address where stores the private key filename */
    char    *Private_Key_Password;     /**< Point to address where stores the private key password */
} qapi_WLAN_Security_8021x_Private_Key_t;

/**
@ingroup qapi_wlan
Data structure to configure a device in Promiscuous mode with the necessary filters, if
required. Promiscuous mode is only supported in virtual device 0. The maximum number
of filters supported is __QAPI_WLAN_PROMISC_MAX_FILTER_IDX. All the necessary
filters should be set at once before passing this data structure to
qapi_WLAN_Set_Param() with __QAPI_WLAN_PARAM_GROUP_WIRELESS_ENABLE_PROMISCUOUS_MODE
as the command ID.

@dependencies
The device should be set to Maximum Performance mode before enabling Promiscuous mode.
*/
typedef struct // qapi_WLAN_Promiscuous_Mode_Info_s
{
    uint8_t src_Mac[__QAPI_WLAN_PROMISC_MAX_FILTER_IDX][__QAPI_WLAN_MAC_LEN];
    /**<
      Array of source MAC addresses that are of interest to the application.
      The first array index here corresponds to the filter index
      and the second index of the array holds the actual source MAC address
      on which incoming 802.11 frames are to be filtered by the firmware.
      */
    uint8_t dst_Mac[__QAPI_WLAN_PROMISC_MAX_FILTER_IDX][__QAPI_WLAN_MAC_LEN];
    /**<
      Array of destination MAC addresses that are of interest to the application.
      The first array index here corresponds to the filter index
      and the second index of the array holds the actual destination MAC address
      on which incoming 802.11 frames are to be filtered by the firmware.
      */
    uint8_t enable;
    /**< Promiscuous mode control; 1: Enable, 0: Disable. */
    uint8_t filter_flags[__QAPI_WLAN_PROMISC_MAX_FILTER_IDX];
    /**<
      Filter flags that represent the validity of filters being
      programmed. Promiscuous mode supports filters based on four factors:
      source MAC, destination MAC, frame type, and frame subtype.
      The application must set the corresponding bit in this field for
      each of the filter indexes to indicate which of the configured filters
      are valid:
      - Set Bit 0 if the source MAC address filter is valid
      - Set Bit 1 if the destination MAC address filter is valid
      - Set Bit 2 if the frame type filter is valid
      - Set Bit 3 if the frame subtype filter is valid @vertspace{-14}
    */
    uint8_t promisc_frametype[__QAPI_WLAN_PROMISC_MAX_FILTER_IDX];
    /**<
      Frame type based filter. The application can choose whether it is
      interested in an 802.11 Control/Data/Management packet type. The application
      can only choose one of these packet types for a given filter index.
      */
    uint8_t promisc_subtype[__QAPI_WLAN_PROMISC_MAX_FILTER_IDX];
    /**<
      Frame subtype based filter. The application can choose to filter frames of
      a given subtype. This subtype filter is valid only if the frame type filter is
      valid in the corresponding filter index. In other words, if a subtype
      filter is set on a filter index, but the corresponding frame type filter
      is not set, the subtype filter is not considered valid.
    */
    uint8_t promisc_num_filters;
    /**<
      Number of programmed promiscuous filters. This should not
      exceed __QAPI_WLAN_PROMISC_MAX_FILTER_IDX. */
} qapi_WLAN_Promiscuous_Mode_Info_t;

/**
@ingroup qapi_wlan
Enumeration of a list of supported 802.11 header types for a Raw mode transmission.
*/
typedef enum {
    QAPI_WLAN_RAW_MODE_HDR_TYPE_BEACON_E = 0,
    /**< Raw mode frame header is of type beacon. */
    QAPI_WLAN_RAW_MODE_HDR_TYPE_PROBE_REQ_DATA_E = 1,
    /**< Raw mode frame header is of type probe request. */
    QAPI_WLAN_RAW_MODE_HDR_TYPE_QOS_DATA_E = 2,
    /**< Raw mode frame header is of type QOS data. */
    QAPI_WLAN_RAW_MODE_HDR_TYPE_FOUR_ADDR_DATA_E = 3,
    /**< Raw mode frame header is of type 4 address data. */
    QAPI_WLAN_RAW_MODE_HDR_TYPE_USER_DEFINED_E = 0xff
    /**< Raw mode frame header is of type self-defined. */
} qapi_WLAN_Raw_Mode_Header_Type_e;

/**
@ingroup qapi_wlan
Data structure that the application is to pass when invoking qapi_WLAN_Raw_Send()
to transmit a raw frame.
*/
typedef struct // qapi_WLAN_Raw_Send_Params_s
{
    uint8_t rate_Index;
    /**< 0: 1 Mbps, 1: 2 Mbps, 2: 5.5 Mbps, etc. */
    /**< Note that this value will not take effect if connected  */
    uint8_t num_Tries;
    /**< Packet transmission count: 1 to 14. */
    uint32_t payload_Size; /**< Payload size: 0 to 1400. */
    uint32_t channel;
    /**< Channel; 0 to 11. 0: Send on the current channel. */
    /**< Note that this value will not take effect if connected  */
    qapi_WLAN_Raw_Mode_Header_Type_e header_Type;
    /**< 0: Beacon frame, 1: Probe Request frame, 2: QoS data frame, 3: Four addresses data frame, 0xff: Self Defined
     * frame*/
    /**< If use Self Defined Header, the frame will be considered as an MGMT frame.>*/
    uint16_t seq;
    /**< Sequence number to be filled in the 802.11 header. */
    uint8_t addr1[__QAPI_WLAN_MAC_LEN]; /**< Address 1. */
    uint8_t addr2[__QAPI_WLAN_MAC_LEN]; /**< Address 2. */
    uint8_t addr3[__QAPI_WLAN_MAC_LEN]; /**< Address 3. */
    uint8_t addr4[__QAPI_WLAN_MAC_LEN]; /**< Address 4. */
    /**< Note that address will not take effect when using self-defined frame  */
    uint32_t data_Length;
    /**< Size of the data to be transmitted. */
    uint8_t *data; /**< Data. */
} qapi_WLAN_Raw_Send_Params_t;

/**
@ingroup qapi_wlan
Enumeration that identifies the action to be taken after a WPS initial request succeeds.
*/
typedef enum {
    QAPI_WLAN_WPS_NO_ACTION_POST_CONNECT_E = 0,       /**< No action to be taken after WPS initiation succeeds. */
    QAPI_WLAN_WPS_CONNECT_REGISTRAR_IN_ENROLLEE_E = 1 /**< Set up a connection after WPS initiation succeeds. */
} qapi_WLAN_WPS_Connect_Action_e;

/**
@ingroup qapi_wlan
Enumeration of supported WPS modes.
*/
typedef enum {
    QAPI_WLAN_WPS_PIN_MODE_E = 0, /**< WPS PIN method. */
    QAPI_WLAN_WPS_PBC_MODE_E = 1  /**< WPS Pushbutton method. */
} qapi_WLAN_WPS_Mode_e;

typedef enum p2p_wps_method{ 
    WPS_NOT_READY, 
    WPS_PIN_LABEL, 
    WPS_PIN_DISPLAY, 
    WPS_PIN_KEYPAD, 
    WPS_PBC 
} p2p_wps_method;

/**
@ingroup qapi_wlan
Data structure to pass WPS credentials from the driver to the application.
*/
typedef struct // qapi_WLAN_WPS_Credentials_s
{
    uint16_t ap_Channel;                          /**< AP's channel. */
    uint8_t ssid[__QAPI_WLAN_MAX_SSID_LEN];       /**< SSID. */
    uint8_t ssid_Length;                          /**< SSID length. */
    qapi_WLAN_Auth_Mode_e auth_Mode;              /**< Authentication mode. */
    qapi_WLAN_Crypt_Type_e encryption_Type;       /**< Encryption type. */
    uint8_t key_Index;                            /**< Index of the key. */
    uint8_t key[__QAPI_WLAN_WPS_MAX_KEY_LEN + 1]; /**< Key. */
    uint8_t key_Length;                           /**< Key length. */
    uint8_t mac_Addr[__QAPI_WLAN_MAC_LEN];        /**< MAC address. */
} qapi_WLAN_WPS_Credentials_t;

/**
@ingroup qapi_wlan
Data structure for A-MPDU enablement. A-MPDU aggregation is enabled on a per-TID basis,
where each TID (0-7) represents a different traffic priority.

The mapping to WMM access categories is as follows:
         - WMM best effort = TID 0-3
         - WMM background  = TID 1-2
         - WMM video       = TID 4-5
         - WMM voice       = TID 6-7

Once enabled, A-MPDU aggregation may be negotiated with an access point/Peer
device and then both devices may optionally use A-MPDU aggregation for
transmission. Due to other bottle necks in the data path, a system may not
get improved performance by enabling A-MPDU aggregation.
*/
typedef struct // qapi_WLAN_Aggregation_Params_s
{
    uint16_t tx_TID_Mask; /**< Bitmask to enable Tx A-MPDU aggregation. */
    uint16_t rx_TID_Mask; /**< Bitmask to enable Rx A-MPDU aggregation. */
} qapi_WLAN_Aggregation_Params_t;

/**
@ingroup qapi_wlan
Data structure used to pass WPS SSID information from the application to the driver.
*/
typedef struct // qapi_WPS_Scan_List_Entry_s
{
    uint8_t ssid[__QAPI_WLAN_MAX_SSID_LEN];  /**< SSID. */
    uint8_t macaddress[__QAPI_WLAN_MAC_LEN]; /**< MAC address. */
    uint16_t channel;                        /**< Wireless channel. */
    uint8_t ssid_Len;                        /**< SSID length. */
} qapi_WPS_Scan_List_Entry_t;

/**
@ingroup qapi_wlan
Data structure to pass WPS command parameters from the application to the driver.
*/
typedef struct // qapi_WLAN_WPS_Start_s
{
    qapi_WPS_Scan_List_Entry_t ssid_info; /**< SSID information. */
    uint8_t wps_Mode;                     /**< WPS pushbutton or WPS PIN. */
    uint8_t timeout_Seconds;              /**< WPS timeout in seconds. */
    uint8_t connect_Flag;                 /**< Connect action (TRUE/FALSE) after intial WPS success. */
    uint8_t pin[__QAPI_WLAN_WPS_PIN_LEN]; /**< PIN. */
    uint8_t pin_Length;                   /**< PIN length. */
} qapi_WLAN_WPS_Start_t;

/**
@ingroup qapi_wlan
Data structure for cipher.
*/
typedef struct // qapi_WLAN_Cipher_s
{
    uint32_t ucipher; /**< Unicast cipher. */
    uint32_t mcipher; /**< Multicast cipher. */
} qapi_WLAN_Cipher_t;

/**
@ingroup qapi_wlan
Data structure for wireless network parameters.
*/
typedef struct // qapi_WLAN_Netparams_s
{
    uint16_t ap_Channel;                      /**< Wireless channel for the AP. */
    int8_t ssid[__QAPI_WLAN_MAX_SSID_LENGTH]; /**< [OUT] Network SSID. */
    int16_t ssid_Len;                         /**< [OUT] Number of valid chars in ssid[]. */
    qapi_WLAN_Cipher_t cipher;                /**< [OUT] Network cipher type values not defined. */
    uint8_t key_Index;                        /**< [OUT] For WEP only; key index for Tx. */
    union {
        uint8_t wepkey[__QAPI_WLAN_PASSPHRASE_LEN + 1];
        /**< WEP key. */

        uint8_t passphrase[__QAPI_WLAN_PASSPHRASE_LEN + 1];
        /**< Passphrase. */
    } u;
    /**<
    [OUT] Security key or passphrase.
    */

    uint8_t sec_Type;   /**< [OUT] Security type. */
    uint8_t error;      /**< [OUT] Error code. */
    uint8_t dont_Block; /**<
                        [IN] 1 -- Returns immediately if the operation is not complete \n
                             0 -- Blocks until the operation completes @newpagetable
                        */
} qapi_WLAN_Netparams_t;

/**
@ingroup qapi_wlan
Enumeration that identifies the device concurrency mode.
*/
typedef enum {
    DEV_MODE_STATION_E = 0x01, /**< Station mode */
    DEV_MODE_AP_E = 0x10,      /**< SoftAP mode */
    DEV_MODE_AP_STA_E = 0x11,  /**< AP_STA Concurrency */
    DEV_MODE_NO_CONC_E,        /**< Concurrency Off. */
} qapi_WLAN_DEV_Mode_e;
/**
@ingroup qapi_wlan
Enum declaration for management frame types.
*/
typedef enum {
    QAPI_WLAN_FRAME_BEACON_E = 0, /**< Beacon frame type. */
    QAPI_WLAN_FRAME_PROBE_REQ_E,  /**< Probe request frame type. */
    QAPI_WLAN_FRAME_PROBE_RESP_E, /**< Probe response frame type. */
    QAPI_WLAN_FRAME_ASSOC_REQ_E,  /**< Association request frame type */
    QAPI_WLAN_FRAME_ASSOC_RESP_E, /**< Association response frame type. */
    QAPI_WLAN_NUM_MGMT_FRAME_E    /**< Number of management frame types. */
} qapi_WLAN_Mgmt_Frame_Type_e;

/**
@ingroup qapi_wlan
Data structure to pass application information element data from the application to the driver.
*/
typedef struct // qapi_WLAN_App_Ie_Params_s
{
    uint8_t mgmt_Frame_Type; /**< Frame in which IE is to be added. */
    uint8_t ie_Len;          /**< IE length. */
    uint8_t *ie_Info;        /**< Application specified IE. */
} qapi_WLAN_App_Ie_Params_t;

/**
@ingroup qapi_wlan
Enum declaration for Tx Power setting policy.
*/
typedef enum {
    QAPI_WLAN_POLLICY_SECURITY_E = 0, /**< Security policy. */
    QAPI_WLAN_POLICY_NUM_E            /**< Number of policy. */
} qapi_WLAN_TX_Power_Policy_e;

/**
@ingroup qapi_wlan
Data structure for Tx Power.
*/
typedef struct // qapi_WLAN_Set_Txpower_Params_t
{
    uint8_t txpower;
    qapi_WLAN_TX_Power_Policy_e policy;
} qapi_WLAN_Set_Txpower_Params_t;

/**
@ingroup qapi_wlan_mgmt_frame_type
Identifies the management frame type.
*/
typedef enum {
    QAPI_WLAN_MGMT_NONE_E = 0x0,       /**< None. */
    QAPI_WLAN_MGMT_ASSOC_RESP_E = 0x1, /**< Association response. */
    QAPI_WLAN_MGMT_PROBE_RESP_E = 0x2, /**< Probe response. */
} qapi_WLAN_MGMT_FRAME_e;

#define QAPI_MAX_REG_RULES 17
/**
@ingroup qapi_wlan
Data structure that the application uses to interpret the Regulatory for
all regulatory information which device support.

All the information in this structure is for one Regulatory which device support
*/
typedef struct // qapi_WLAN_Reg_s
{
    uint16_t start_freq; /**< start frequency. */
    uint16_t end_freq;   /**< end frequency. */
    uint8_t reg_power;   /**< regulatory power. */
    uint8_t ant_gain;    /**< antenna gain. */
    uint16_t flag_info;  /**< flag information. */
    uint16_t max_bw;
} qapi_WLAN_Reg_t;

typedef struct {
    uint8_t alpha[3];                               /**The region code*/
    uint8_t num_2g_reg_rules;                       /**Numbers of rules for 2g in the region*/
    uint8_t num_5g_reg_rules;                       /**Numbers of rules for 5g in the region*/
    qapi_WLAN_Reg_t reg_rules[QAPI_MAX_REG_RULES];  /** The rule of the region for certain bands*/
} qapi_WLAN_Reg_Evt_t;

typedef struct {
    uint8_t reg_power;                              /** Regulatory power*/
    uint8_t ctl_power;                              /** Conformance test limit power*/
    uint16_t target_power;                          /** The targeted maximum TX power*/
    uint16_t real_power;                            /** The power that is set to driver.*/
} qapi_WLAN_Get_Power_Evt_t;

/**
@ingroup qapi_wlan
Set Rate.
*/
typedef struct {
    uint8_t ra_ON;              /** Flag indicating whether automatic rate adaptation is enabled.
                                   Use NT_RA_OFF to disable or NT_RA_ON to enable. */
    uint8_t rate_staid;         /** Station ID for which the rate configuration applies. */
    uint8_t rate_p_rate;        /** Primary transmission rate.
                                   If transmission fails NT_DEVCFG_RETRY_THRESHOLD0 times (configurable; see rate_adaptation_debug.xml),
                                   the secondary rate will be attempted. */
    uint8_t rate_s_rate;        /** Secondary transmission rate.
                                   If transmission fails an additional (NT_DEVCFG_RETRY_THRESHOLD1 - NT_DEVCFG_RETRY_THRESHOLD0) times,
                                   the tertiary rate will be attempted. */
    uint8_t rate_t_rate;        /** Tertiary transmission rate.
                                   If transmission fails a further (NT_DEVCFG_RETRY_THRESHOLD2 - NT_DEVCFG_RETRY_THRESHOLD0) times,
                                   the transmission will be considered failed. */
} qapi_WLAN_Set_Rate_Params_t;

/**
@ingroup qapi_wlan
Set STA Listen interval.
*/
typedef struct {
    uint32_t time;                      /** Listen interval in Time Units (TU), where 1 TU = 1024 microseconds. */
    uint32_t round_type;                /** Determines how the beacon interval is rounded when calculating (time / beacon interval). */
} qapi_WLAN_Listen_Interval_Params_t;

/**
@ingroup qapi_wlan
Set STA edca param, including aifsn/cw_min/cw_max/txoplimit.
*/
typedef struct {
    uint8_t qid;                        /** The queue id*/
    uint8_t aifsn;                      /** Arbitration Inter Frame Spacing Number*/
    uint16_t cw_min;                    /** Minimum value of contention window*/
    uint16_t cw_max;                    /** Maximum value of contention window*/
    uint16_t txop_limit;                /** Transmission Opportunity Limit*/
} qapi_WLAN_Edca_Params_t;

/**
@ingroup qapi_wlan
Set STA BA window parameters.
*/
typedef struct {
    uint16_t ack_timeout;               /** BA window ack timeout in us*/
    uint16_t delay;                     /** Propagation time in numbers of SM clock cycles */
} qapi_WLAN_BA_Window_Params_t;

/**
@ingroup qapi_wlan
Set STA BA window size.
*/
typedef struct {
    uint16_t tx_size;					/** TX BA window size */
    uint16_t rx_size;					/** RX BA window size */
} qapi_WLAN_BA_Window_Size_t;

/** Size of the WLAN PMKID in bytes. */
#define __QAPI_WLAN_PMKID_LEN  16

/**
@ingroup qapi_wlan
Enumeration the enable/disable options for WLAN PMKID.
*/
typedef enum {
   QAPI_WLAN_PMKID_DISABLE_E = 0,   /**< Disable the WLAN PMKID. */
   QAPI_WLAN_PMKID_ENABLE_E  = 1,   /**< Enable the WLAN PMKID. */
} qapi_WLAN_PMKID_ENABLE_e;

/**
@ingroup qapi_wlan
Data structure that the application is to set pmkid.
*/
typedef struct {
    uint8_t     bssid[__QAPI_WLAN_MAC_LEN]; /**< bssid of the wlan connection. */
    uint8_t     enable;                     /**< qapi_WLAN_PMKID_ENABLE_e. */
    uint8_t     pmkid[__QAPI_WLAN_PMKID_LEN];   /**< pmkid of the wlan connection. */
} qapi_WLAN_Set_PMKID_Params_t;

/**
@ingroup qapi_wlan
Data structure that presents rx_eapol_key event information from the driver to the
application.

The application uses this data structure to interpret the event
payload received with a QAPI_WLAN_RX_EAPOL_KEY_CB_E event.
*/
typedef struct
{
    uint8_t                 descType;       /**< Eapol key type, qapi_EAPOL_KEY_TYPE_e. */
    uint8_t                 keyInfo[2];     /**< Key information, big endian. */
    uint8_t                 pmkid_valid;    /**< Is pmkid valid. */
    uint8_t                 rsrv[4];        /**< Reserved. */
    uint8_t                 pmkid[__QAPI_WLAN_PMKID_LEN];   /**< Pmkid. */
} qapi_WLAN_RxEapolKey_Cb_Info_t;

/**
@ingroup qapi_wlan
Function pointer to an application specified callback handler function.

The callback handler for WLAN commands can be set using qapi_WLAN_Set_Callback().

@param[in] device_ID                Device ID.
@param[in] cb_ID                    Callback ID associated to a command.
@param[in] application_Context      Application context.
@param[in] payload                  Data.
@param[in] payload_Length           Data size.

@return
None.
*/
typedef void (*qapi_WLAN_Callback_t)(uint8_t device_ID, uint32_t cb_ID, void *application_Context, void *payload,
                                     uint32_t payload_Length);

qapi_Status_t qapi_WLAN_Enabled(qapi_WLAN_Enable_e *enable);
qapi_Status_t qapi_WLAN_Error(void);

/**
@ingroup qapi_wlan
Enables/disables the Wi-Fi module.
This is a blocking call and returns on receipt of a WMI_READY event from KF.

Use QAPI_WLAN_ENABLE_E as the parameter for enabling and QAPI_WLAN_DISABLE_E for disabling WLAN.

This API brings up the KF firmware but does not add any devices.

@datatypes
#qapi_WLAN_Enable_e

@param[in] enable  QAPI_WLAN_ENABLE_E or QAPI_WLAN_DISABLE_E.

@sa
qapi_WLAN_Add_Device

@return
QAPI_OK -- Enabling or disabling WLAN succeeded. \n
QAPI_BUSY -- Disabling WLAN failed due to open AF_INET (IPv4) sockets. \n
Nonzero value -- Enabling or disabling WLAN failed.

@dependencies
Use qapi_WLAN_Add_Device() after qapi_WLAN_Enable() to add devices before using other
WLAN control commands.\n
Use qapi_WLAN_Remove_Device() before disabling WLAN.
*/
qapi_Status_t qapi_WLAN_Enable(qapi_WLAN_Enable_e enable);

/**
@ingroup qapi_wlan
Adds an interface in the Wi-Fi driver. This API has no interaction with KF.

WLAN must be enabled before calling this API. A maximum of two devices are supported at this time.

@param[in] device_ID        Device ID.

@return
QAPI_OK -- Adding a new device (interface) succeeded. \n
Nonzero value -- Adding a new device (interface) failed.

@dependencies
Use qapi_WLAN_Enable() to enable the Wi-Fi module before using this API.
*/
qapi_Status_t qapi_WLAN_Add_Device(uint8_t device_ID);

/**
@ingroup qapi_wlan
Initiates a wireless scan to fnid nearby access points.

Users can configure the scan type as they want. Additional scan parameters can be tuned by using qapi_WLAN_Set_Params()
with GroupID for Wireless and ParamID scan_params. Scaned results will be notified by QAPI_WLAN_SCAN_COMPLETE_CB_E

@datatypes
#qapi_WLAN_Start_Scan_Params_t \n
#qapi_WLAN_Store_Scan_Results_e

@param[in] device_ID            Device ID.
@param[in] scan_Params          scan parameters.

@return
QAPI_OK -- Wireless scan started. \n
Nonzero value -- Wireless scan failed.

@dependencies
Scan cannot be started in SoftAP mode.
*/
qapi_Status_t qapi_WLAN_Start_Scan(uint8_t device_ID, const qapi_WLAN_Start_Scan_Params_t *scan_Params);

/**
@ingroup qapi_wlan
Returns the scan results from the most recent wireless scan performed to find nearby access points.

This is a blocking API and should be used when the QAPI_WLAN_BUFFER_SCAN_RESULTS_BLOCKING option is used.

@datatypes
#qapi_WLAN_BSS_Scan_Info_t

@param[in]     device_ID              Device ID.
@param[out]    scan_Res              Data pointer to get the scan result list from the driver.
@param[in]      num_Bss              Number of bss which can be put into

@return
QAPI_OK -- Getting the results from most recent wireless scan succeeded. \n
Nonzero value -- Getting the results from most recent wireless scan failed.

@dependencies
Call qapi_WLAN_Start_Scan() to start a wireless scan before calling this API.
*/
qapi_Status_t qapi_WLAN_Get_Scan_Results(uint8_t device_ID, qapi_WLAN_Scan_Comp_Evt_t *scan_Res, int16_t *num_Bss);

/**
@ingroup qapi_wlan
This API is part of connect/disconnect process in both non-AP Station and SoftAP modes.
It uses previously set SSID and security configurations.

In non-AP Station mode, it allows the station to connect to or disconnect from an associated AP.
In SoftAP mode, it starts the device as a SoftAP.

This API is already called from qapi_WLAN_Disconnect() and qapi_WLAN_WPS_Connect(), so it does not need to be called
explicitly for these operations; however for connect operations (except for WPS), it will need to be called explicitly.

@param[in] device_ID           Device ID.

@return
QAPI_OK -- Commit operation succeeded. \n
Nonzero value -- Commit operation failed.

@dependencies
SSID and security configurations must be set using qapi_WLAN_Set_Param() before calling this API.\n
If SSID is set to a nonempty string before calling this API, it is considered as a connect request, and if the SSID is
set to an empty string, it is considered as a disconnect request in both non-AP Station and SoftAP modes.\n If the user
wants to use Tx/Rx aggregation, __QAPI_WLAN_PARAM_GROUP_WIRELESS_ALLOW_TX_RX_AGGR_SET_TID must be set using
qapi_WLAN_Set_Param() before calling this API.\n Similarly, if the user wants to use UAPSD,
__QAPI_WLAN_PARAM_GROUP_WIRELESS_AP_ENABLE_UAPSD (in SoftAP mode) and
__QAPI_WLAN_PARAM_GROUP_WIRELESS_STA_UAPSD in non-AP Station mode should be set using qapi_WLAN_Set_Param().
*/
qapi_Status_t qapi_WLAN_Commit(uint8_t device_ID);

/**
@ingroup qapi_wlan
Sets a callback function and application context (const void *), if any, for WLAN control commands.

Users can use the callback function to handle events received from the WLAN firmware for asynchronous commands.
The application context, while setting the callback, will be returned when the callback handler function is called for
events. The callback handling is done in the Wi-Fi driver thread's context, hence the event handling should be as fast
as possible to avoid performance delays.

@datatypes
#qapi_WLAN_Callback_t

@param[in] callback               Callback function. NULL to clear callback, non-NULL to initialize callback, not
support replace.
@param[in] application_Context    Application context set by the application.

@return
QAPI_OK -- Application callback handler function for WLAN commands set. \n
Nonzero value -- Setting callback function failed.

@dependencies
The callback function to be set must be a valid function.
*/
qapi_Status_t qapi_WLAN_Set_Callback(qapi_WLAN_Callback_t callback, void *application_Context);

/** @cond */
#ifdef WLAN_TESTS
/**
@ingroup qapi_wlan
This API is used for wlan specific test commands.

@param[in]      wlan_test_id      Test ID.

@return
QAPI_OK -- Test command succeeded. \n
Nonzero value -- Test command failed.

@dependencies
None.
*/
qapi_Status_t qapi_WLAN_tests(uint8_t wlan_test_id);
#endif /* WLAN_TESTS */
/** @endcond */

/**
@ingroup qapi_wlan
Disconnects a device or aborts the current connection process.
This API internally calls qapi_WLAN_Commit(), hence no explicit call is needed in the disconnect process when using this
API.

@param[in] device_ID         Device ID.

@return
QAPI_OK -- Disconnect process succeeded. \n
Nonzero value -- Disconnection failed.

@dependencies
None.
*/
qapi_Status_t qapi_WLAN_Disconnect(uint8_t device_ID);

/**
@ingroup qapi_wlan
Sets requested parameters in the WLAN firmware.

The group_ID parameter is used to determine to which group the the command belongs, and the param_ID is used to provide
the requested command (/parameter).

The __QAPI_WLAN_PARAM_GROUP_* types give more information regarding the possible values for group_ID and param_ID.
Currently, only nonblocking calls are supported, hence wait_For_Status should be set to FALSE (0).

@datatypes
#qapi_WLAN_Wait_For_Status_e

@param[in] device_ID         Device ID.
@param[in] group_ID          Group ID: system, wireless, security, P2P.
@param[in] param_ID          Parameter/command ID.
@param[in] data              Value to be set for the requested parameter.
@param[in] length            Size of the value to be set.
@param[in] wait_For_Status   0: No wait; 1: Wait for result of the set request.

@return
QAPI_OK -- Requested parameter was set. \n
Nonzero value -- Requested parameter was not set.

@dependencies
None.
*/
qapi_Status_t qapi_WLAN_Set_Param(uint8_t device_ID, uint16_t group_ID, uint16_t param_ID, const void *data,
                                  uint32_t length, qapi_WLAN_Wait_For_Status_e wait_For_Status);

/**
@ingroup qapi_wlan
Retrieves the requested parameter value from the WLAN firmware.

The group_ID parameter is used to determine to which group the the command belongs, and the param_ID is used to provide
the requested command (/parameter).

The __QAPI_WLAN_PARAM_GROUP_* types give more information regarding the possible values for group_ID and param_ID.

@param[in]  device_ID     Device ID.
@param[in]  group_ID      Group ID: system, wireless, security, P2P.
@param[in]  param_ID      Parameter ID in the given group.
@param[out] data          Value retrieved from the WLAN firmware.
@param[out] length        Size of the parameter value.

@return
QAPI_OK -- Requested was parameter retrieved from the WLAN firmware. \n
Nonzero value -- Parameter retrieval failed.

@dependencies
None.
*/
qapi_Status_t qapi_WLAN_Get_Param(uint8_t device_ID, uint16_t group_ID, uint16_t param_ID, void *data,
                                  uint32_t *length);

/**
@ingroup qapi_wlan
Gets regulatory infrmationo of the device.

@param[out] reg        regulatory information retrieved from the WLAN firmware.

@return
QAPI_OK -- Regulatory information was retrieved from WLAN firmware. \n
Nonzero value -- Regulatory information retrieval failed.

@dependencies
None.
*/
qapi_Status_t qapi_WLAN_Get_Regulatory_Info(qapi_WLAN_Reg_Evt_t *reg);

/**
@ingroup qapi_wlan
Set WLAN TX rate.

@param[in] prate_para rate config.

@return
QAPI_OK -- Set rate successfully. \n
Nonzero value -- Set rate failed.

@dependencies
None.
*/
qapi_Status_t qapi_WLAN_Set_Rate(qapi_WLAN_Set_Rate_Params_t *prate_para);

/**
@ingroup qapi_wlan
Get WLAN TX rate.

@param[in] prate_para rate config.

@return
QAPI_OK -- Set rate successfully. \n
Nonzero value -- Set rate failed.

@dependencies
None.
*/
qapi_Status_t qapi_WLAN_Get_Rate(qapi_WLAN_Set_Rate_Params_t *prate_para);

/**
@ingroup qapi_wlan
Send raw frame.

@param[in] device_ID raw_Params config.

@return
QAPI_OK -- Send frame successfully. \n
Nonzero value -- Send frame failed.

@dependencies
None.
*/
qapi_Status_t qapi_WLAN_Raw_Send(qapi_WLAN_Raw_Send_Params_t *raw_para);

/**
@brief  API to be used to enable wlan management frame filter
@param[in]  device_id        Device ID.
@param[in]  mgmt_filter      WLAN management frames filter. Now only support association response and probe response
frames.

@return qapi_Status_t        QAPI_OK on success, other error code on failure.
*/
qapi_Status_t qapi_WLAN_Enable_Mgmt_Filter(uint8_t device_ID, uint32_t mgmt_filter);

/**
@brief  API to be used to disable wlan management frame filter
@param[in]  device_id        Device ID.

@return qapi_Status_t        QAPI_OK on success, other error code on failure.
*/
qapi_Status_t qapi_WLAN_Disable_Mgmt_Filter(uint8_t device_ID);

/**
@brief  API to be used to recieve wlan management frames which are in filter.
@param[out]  buffer          Pointer to output buffer for management frames .
@param[in]   buffer_len      Buffer lenght in bytes.
@param[out]  frame_len       lenght of management frames.
@param[in]   timeout         timeout in millisecond when receive management frames .

@return qapi_Status_t    QAPI_OK on success, other error code on failure.
*/
qapi_Status_t qapi_WLAN_Recv_Mgmt_Frames(uint8_t *buffer, uint32_t buffer_len, uint32_t *frame_len, uint32_t timeout);

_STRUCT_4BYTE_ALLIGN_CHECK(qapi_WLAN_Evt_Hdr_t)
_STRUCT_4BYTE_ALLIGN_CHECK(qapi_WLAN_Enable_Evt_t)
_STRUCT_4BYTE_ALLIGN_CHECK(qapi_WLAN_Disable_Evt_t)
_STRUCT_4BYTE_ALLIGN_CHECK(qapi_WLAN_If_Add_Comp_Evt_t)
_STRUCT_4BYTE_ALLIGN_CHECK(qapi_WLAN_Scan_Start_Evt_t)
_STRUCT_4BYTE_ALLIGN_CHECK(qapi_WLAN_BSS_Scan_Info_t)
_STRUCT_4BYTE_ALLIGN_CHECK(qapi_WLAN_Scan_Comp_Evt_t)
_STRUCT_4BYTE_ALLIGN_CHECK(qapi_WLAN_Join_Comp_Evt_t)

#endif // __QAPI_WLAN_BASE_H__
