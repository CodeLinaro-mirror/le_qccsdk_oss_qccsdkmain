/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
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

/**
@ingroup qapi_wlan
Identifies the enable/disable options for WLAN.
*/
typedef enum
{
    QAPI_WLAN_DISABLE_E  = 0, /**< Disable the Wi-Fi module. */
    QAPI_WLAN_ENABLE_E   = 1  /**< Enable the Wi-Fi module. */
} qapi_WLAN_Enable_e;

/**
@ingroup qapi_wlan
Enum declarations for the scan control flags.

@note1hang The ScanCtrlFlag value of 0xFF is used to disable all flags in the Scan Params Cmd.
Do not add any more flags to qapi_WLAN_Scan_Ctrl_Flag_bits_e.
*/
typedef enum
{
    QAPI_WLAN_CONNECT_SCAN_CTRL_FLAGS_E = 0x01,    /**< Set if the Connect command can be scanned. */
    QAPI_WLAN_SCAN_CONNECTED_CTRL_FLAGS_E = 0x02,  /**< Set if the scan is for the SSID it is already connected to. */
    QAPI_WLAN_ACTIVE_SCAN_CTRL_FLAGS_E = 0x04,     /**< Set if active scan is enabled. */
    QAPI_WLAN_ROAM_SCAN_CTRL_FLAGS_E = 0x08,       /**< Set if roam scan is enabled when BMISS and LOWRSSI. */
    QAPI_WLAN_REPORT_BSSINFO_CTRL_FLAGS_E = 0x10,  /**< Set if the scan follows the customer's BSSINFO reporting rule. */
    QAPI_WLAN_ENABLE_AUTO_CTRL_FLAGS_E = 0x20,     /**< If disabled, the target does not scan after a disconnect event. */
    QAPI_WLAN_ENABLE_SCAN_ABORT_EVENT_E = 0x40,    /**< Scan complete event with cancelled status will be generated
						        when a scan is pre-empted before it is completed. */
    QAPI_WLAN_ENABLE_DFS_SKIP_CTRL_FLAGS_E = 0x80  /**< Set to skip scanning a DFS channel. */
} qapi_WLAN_Scan_Ctrl_Flag_bits_e;

/**
@ingroup qapi_wlan
Macro to use the default scan control flags for a scan.
*/
#define __QAPI_WLAN_DEFAULT_SCAN_CTRL_FLAGS  ( QAPI_WLAN_CONNECT_SCAN_CTRL_FLAGS_E| QAPI_WLAN_SCAN_CONNECTED_CTRL_FLAGS_E| QAPI_WLAN_ACTIVE_SCAN_CTRL_FLAGS_E| QAPI_WLAN_ROAM_SCAN_CTRL_FLAGS_E| QAPI_WLAN_ENABLE_AUTO_CTRL_FLAGS_E )

/**
@ingroup qapi_wlan
Data structure used by the application to pass wireless scan options to the driver.
If the device is connected to an AP, a successful scan returns the current AP along
with newly scanned APs.
*/
typedef struct //qapi_WLAN_Start_Scan_Params_s
{
    int32_t         force_Fg_Scan;
    /**< Force a high priority scan. */
    uint32_t        home_Dwell_Time_In_Ms;
    /**< Maximum scan duration in the home channel (in ms). If set to 0, the default value of 50 ms is used. */
    uint32_t        force_Scan_Interval_In_Ms;
    /**< Time interval (in ms) between scanning channels from the list. If set to 0, the default value of 100 ms is used.  */
    uint8_t         scan_Type;
    /**< This parameter currently supports only 0 as an input value. */
    uint8_t         num_Channels;
    /**< Number of channels to scan. */
    uint16_t        channel_List[1];
    /**< List of channels to scan. */
    uint8_t         ssid[__QAPI_WLAN_MAX_SSID_LEN];
    uint8_t         ssid_Length;
} qapi_WLAN_Start_Scan_Params_t;

/**
@ingroup qapi_wlan
Data structure that the application uses to interpret the scan results for
all access point information received during the scan.

All the information in this structure is for one particular BSS found during the scan.
*/
typedef struct //qapi_WLAN_BSS_Scan_Info_s
{
    uint8_t  channel;                        /**< Wireless channel. */
    uint8_t  ssid_Length;                    /**< SSID length. */
    uint8_t  rssi;                           /**< Received signal strength indicator. */
    uint8_t  security_Enabled;               /**< 1: Security enabled; 0: Security disabled. */
    uint16_t beacon_Period;                  /**< Beacon period. */
    uint8_t  preamble;                       /**< Preamble. */
    uint8_t  bss_type;                       /**< BSS type. */
    uint8_t  bssid[__QAPI_WLAN_MAC_LEN];     /**< BSSID. */
    uint8_t  ssid[__QAPI_WLAN_MAX_SSID_LEN]; /**< SSID. */
    uint8_t  rsn_Cipher;                     /**< RSN cipher. */
    uint8_t  rsn_Auth;                       /**< RSN authentication. */
    uint8_t  wpa_Cipher;                     /**< WPA cipher. */
    uint8_t  wpa_Auth;                       /**< WPS authentication. */
    uint16_t caps;                           /**< Capability IE. */
    uint8_t  wep_Support;                    /**< Supprt for WEP. */
    uint8_t  reserved[3];                       /**< Reserved. */
} qapi_WLAN_BSS_Scan_Info_t;

/**
@ingroup qapi_wlan
This data type enumerates the scan status to indicate the scan is succeeded, required to rescan or failed.
*/
typedef enum
{
	QAPI_WLAN_SCAN_STATUS_SUCCESS_E = 0,          /*scan is succeeded*/
	QAPI_WLAN_SCAN_STATUS_REQUIRE_RESCAN_E = 1,	 /*require to rescan if the scan is cancelled or aborted as the target is busy or has more urgent task*/
	QAPI_WLAN_SCAN_STATUS_FAILURE_E = 2   /*scan is failed*/
}qapi_WLAN_Scan_Status_e;

/**
@ingroup qapi_wlan
qapi_WLAN_BSS_Scan_Info_t is not enough and cannot be changed,
here add a new structure to report more information.
*/
typedef struct
{
    qapi_WLAN_BSS_Scan_Info_t   info;           /**< The basic info. */
    int32_t                     is_beacon;      /**< TRUE: beacon, FALSE: ProbeRsp. */
    const uint8_t               *beacon_IEs;    /**< Beacon or Probe Response IEs which do not include fixed fields. */
    uint16_t                    beacon_IEs_Len; /**< Length of beacon_IEs. */
    uint8_t                     reserved[6];    /**< Reserved, 2 bytes for allignment, 4 bytes for extension. */
} qapi_WLAN_BSS_Scan_Info_ExtV1_t;

typedef struct{
    qapi_Status_t status; /* over all status */
}qapi_WLAN_Evt_Hdr_t;

typedef struct {
    qapi_WLAN_Evt_Hdr_t evt_hdr; /* contains the common event header */
    uint8_t mac_addr[__QAPI_WLAN_MAC_LEN]; /* the assigned mac address */
    uint8_t num_networks; /* no of vdev supported */
    uint8_t reserved; /* reserved */
    uint32_t cap_info; /*capibility info bitmap*/
    uint32_t cap_info2;
    uint32_t reserved2; /* reserved2 */
} qapi_WLAN_Enable_Evt_t;

typedef struct {
    qapi_WLAN_Evt_Hdr_t evt_hdr; /* contains the common event header */
    uint32_t reserved;
} qapi_WLAN_Disable_Evt_t;

typedef struct {
    qapi_WLAN_Evt_Hdr_t evt_hdr;  /* contains the common event header */
    uint32_t reserved;
} qapi_WLAN_If_Add_Comp_Evt_t;

typedef struct {
    qapi_WLAN_Evt_Hdr_t evt_hdr;  /* contains the common event header */
    uint8_t scan_id; /* identifier for specific scan */
    uint8_t reserved[3];
} qapi_WLAN_Scan_Start_Evt_t;

typedef struct {
    qapi_WLAN_Evt_Hdr_t evt_hdr;  /* contains the common event header */
    uint8_t num_bss_cur; /* no of bss in current structure */
    uint8_t scan_id;
    uint8_t total_bss;  /* no of bss total scaned */
    uint8_t reserved2;
    qapi_WLAN_BSS_Scan_Info_t scan_bss_info[0]; /* bss info array, count is num_bss_cur */
} qapi_WLAN_Scan_Comp_Evt_t;

typedef struct {
    qapi_WLAN_Evt_Hdr_t evt_hdr;  /* contains the common event header */
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
    uint8_t  ssid_Length;                    /**< SSID length. */
    uint8_t  ssid[__QAPI_WLAN_MAX_SSID_LEN]; /*ssid of joind AP */
    uint16_t assoc_id; /* association id */
    uint8_t host_initiated; /* Specify whether join is host initiated or not*/
    uint8_t reason_code;
    uint16_t channel_frequency;
    uint8_t passphrase[__QAPI_WLAN_PASSPHRASE_LEN + 1]; /* passphrase of joind AP */
    uint16_t reserved2;
} qapi_WLAN_Join_Comp_Evt_t;

typedef struct {
    qapi_WLAN_Evt_Hdr_t evt_hdr;  /* contains the common event header */
    uint8_t reason; 
    uint16_t freq;
    uint8_t reserved;  
    uint8_t reserved2;
} qapi_WLAN_Chan_Switch_Evt_t;

typedef struct {
    qapi_WLAN_Evt_Hdr_t evt_hdr;  /* contains the common event header */
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
typedef enum
{
   QAPI_WLAN_CONNECT_CB_E                   = 0,
          /**< ID to indicate connect/disconnect events. */
   QAPI_WLAN_SCAN_COMPLETE_CB_E             = 1,
          /**<
          ID to indicate a wireless scan complete event, received for
          nonblocking/nonbuffering scans.
          */
   QAPI_WLAN_FWD_PROBE_REQUEST_INFO_CB_E    = 2,
          /**<
          ID to indicate a probe request forwarded from the WLAN firmware
          when probe request forwarding is enabled by the application.
          */
   QAPI_WLAN_RESUME_CB_INFO_CB_E            = 3,
          /**<
          ID to indicate a WLAN driver/firmware resume completed event and that the
          application can start sending data over the WLAN driver after receiving
          this event (for future use).
          */
   QAPI_WLAN_PROMISCUOUS_MODE_CB_INFO_CB_E  = 4,
          /**<
          ID to indicate that packets were captured in Promiscuous mode. For every
          packet received in Promiscuous mode, the driver indicates this event to
          the application. Since it is quite possible to receive packets too
          frequently in Promiscuous mode, the application is expected to do a minimal
          operation in the callback implementation.
          */
   QAPI_WLAN_DISCONNECT_CB_E                = 5,
          /**<
          Currently not used. A disconnect event is indicated in the parameter
          of QAPI_WLAN_CONNECT_CB_E.
          */
   QAPI_WLAN_DEAUTH_CB_E                    = 6,
          /**<
          Currently not used. A disconnect event is indicated in the parameter
          of QAPI_WLAN_CONNECT_CB_E.
          */
   QAPI_WLAN_WPS_CB_E                       = 7,
          /**<
          ID to indication completion of a WPS handshake to the application.
          The application should use qapi_WLAN_WPS_Await_Completion() to get event information.
          */
   QAPI_WLAN_P2P_CB_E                       = 8,
          /**<
          ID to indicate all P2P events. Since most of the P2P event
          handling is done by the application, it is necessary for the application to
          copy necessary information from events(in an event callback) and handle this in
          the application's own thread context.
          */
   QAPI_WLAN_BSS_INFO_CB_E                  = 9,
          /**<
          ID to indicate that AP profile information was received when performing a nonbuffering
          scan. Each event of this type indicates only the AP's profile information.
          */
   QAPI_WLAN_TCP_KEEPALIVE_OFFLOAD_CB_E     = 10,
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
   QAPI_WLAN_WNM_CB_E                       = 12,
          /**<
          ID to indicate events received when WNM commands, such as setting
          BSS maximum idle period, enter/exit WNM sleep complete execution.
          */
   QAPI_WLAN_CHANNEL_SWITCH_CB_E            = 13,
          /**<
          ID to indicate a wireless channel change event received when STA
          changes the operating channel due to a channel switch announcement IE
          from the connected access point.
          */
   QAPI_WLAN_READY_CB_E                     = 14,
          /**<
          ID to indicate the event that announces the Wi-Fi module (firmware)
          is enabled and ready.
          */
   QAPI_WLAN_SUSPEND_CB_E                   = 15,
          /**< ID to indicate a WLAN firmware suspend event. */
   QAPI_WLAN_DRIVER_DISABLE_CB_E            = 16,
          /**< ID to indicate a driver shut down complete event. */
   QAPI_WLAN_ERROR_HANDLER_CB_E             = 17,
          /**< ID to indicate a fatal error in the WLAN driver. */
   QAPI_WLAN_RESUME_HANDLER_CB_E            = 18,
          /**< ID to indicate to resume completion of the WLAN firmware. */
   QAPI_WLAN_RX_EAPOL_KEY_CB_E              = 19,
          /**< ID to indicate to receive eapol key frame. */
   QAPI_WLAN_RX_MGMT_CB_E                   = 20,
          /**< ID to indicate to receive MGMT frame. */
   QAPI_WLAN_PMF_EVENT_CB_E                 = 21,
          /**< ID to indicate PMF event. */
   QAPI_WLAN_SAE_COMMIT_CB_E                = 22,
          /**< ID to indicate to prepare SAE auth commit frame. */
   QAPI_WLAN_COUNTRY_CODE_CB_E				= 23,
   		  /**< ID to indicate country code is set. */
   QAPI_WLAN_ENABLE_CB_E                    = 24, /**< ID to indicate WLAN is enabled. */
   QAPI_WLAN_DISABLE_CB_E                   = 25, /**< ID to indicate WLAN is disabled. */
   QAPI_WLAN_IF_ADD_COMP_CB_E               = 26, /**< ID to indicate WLAN interface is added. */
   QAPI_WLAN_SCAN_START_CB_E                = 27, /**< ID to indicate WLAN scan is started. */
   QAPI_WLAN_WPS_FAIL_CB_E                  = 28, /**< ID to indicate WLAN WPS failed. */
} qapi_WLAN_Callback_ID_e;

/**
@ingroup qapi_wlan
Data structure that presents connect event information from the driver to the
application.

The application uses this data structure to interpret the event
payload received with a QAPI_WLAN_CONNECT_CB_E event.
*/
typedef struct //qapi_WLAN_Connect_Cb_Info_s
{
   int32_t   value;
        /**< TRUE: To indicate connect events \n
             FALSE: To indicate disconnect/deauthorization events
   */
   uint8_t   mac_Addr[__QAPI_WLAN_MAC_LEN];
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
   uint32_t  bss_Connection_Status;
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
    int32_t   disConnReason;
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
qapi_WLAN_Connect_Cb_Info_t is not enough and cannot be changed,
here add a new structure to report more information.
*/
typedef struct
{
    qapi_WLAN_Connect_Cb_Info_t info;               /**< The basic info. */
    const uint8_t               *req_IEs;           /**< (Re)Association Request IEs which do not include fixed fields. */
    const uint8_t               *resp_IEs;          /**< (Re)Association Response IEs which do not include fixed fields. */
    const uint8_t               *beacon_IEs;        /**< Now only include WPA or RSN IE. */
    uint8_t                     req_IEs_Len;        /**< Length of req_IEs. */
    uint8_t                     resp_IEs_Len;       /**< Length of resp_IEs. */
    uint8_t                     beacon_IEs_Len;     /**< Length of beacon_IEs. */
    uint8_t                     channel;            /**< Wireless channel. */
    uint16_t                    listen_Interval;    /**< listen interval. */
    uint16_t                    beacon_Interval;    /**< beacon interval. */
    uint8_t                     reserved[8];        /**< Reserved, 2 bytes for internal member, 2 bytes for allignment, 4 bytes for extension. */
} qapi_WLAN_Connect_Cb_Info_ExtV1_t;

/**
@ingroup qapi_wlan
Data structure used to indicate the WLAN enablement status to a coexistence subsystem.
This callback is not passed on for the application, but only for other internal
subsystems.
*/
typedef struct //  qapi_WLAN_Ready_Cb_Info_s
{
    uint32_t numDevices;
          /**< Supported number of virtual interfaces/devices. */
} qapi_WLAN_Ready_Cb_Info_t;

/**
@ingroup qapi_wlan
Data structure used to indicate WLAN virtual device connection/disconnection
information to a coexistence subsystem.
*/
typedef struct //qapi_WLAN_Connect_Coex_Cb_Info_s
{
   uint16_t       frequency_MHz;   /**< Frequency(802.11 Channel) in MHz. */
   uint8_t        chan_40Mhz;
   /**< 1: 40 MHz (in 2.4 GHz band) channel \n
        0: 20 MHz channel
   */
   uint32_t       value;
   /**< 1: Connected \n
        0: Not connected
   */
} qapi_WLAN_Connect_Coex_Cb_Info_t;

/** @cond */
/**
@ingroup qapi_wlan
Data structure used to pass data from driver to application callback after scan
is completed
*/
typedef struct  //qapi_WLAN_Scan_Complete_Cb_Info_s
{
   uint32_t                            num_Bss_Info;   /**< Number of BSS. */
   qapi_WLAN_BSS_Scan_Info_t           bss_Info[1];    /**< BSS information */
} qapi_WLAN_Scan_Complete_Cb_Info_t;


/**
@ingroup qapi_wlan
Data structure used to pass data from driver to application callback for probe request command
*/
typedef struct //qapi_WLAN_Fwd_Probe_Request_Cb_Info_s
{
   int32_t                             frequency;           /**< Frequency. */
   int32_t                             buffer_Length;       /**< Buffer length. */
   void                                *probe_Req_Buffer;   /**< Buffer for probe request. */
} qapi_WLAN_Fwd_Probe_Request_Cb_Info_t;
/** @endcond */

/**
@ingroup qapi_wlan
Data structure that represents WLAN statistics information for each virtual device.
This data structure contains counts of various types of packets transmitted/received
on the requested virtual device.
*/
typedef struct
{
    uint32_t unicast_Tx_Pkts;                      /**< Unicast Tx packets. Only valid in STA mode. */
    uint32_t unicast_Rx_Pkts;                      /**< Unicast Rx packets. Only valid in STA mode. */
    uint32_t multicast_Tx_Pkts;                    /**< Multicast Tx packets. Only valid in STA mode. */
    uint32_t multicast_Rx_Pkts;                    /**< Multicast Rx packets. Only valid in STA mode. */
    uint32_t broadcast_Tx_Pkts;                    /**< Broadcast Tx packets. Only valid in STA mode. */
    uint32_t broadcast_Rx_Pkts;                    /**< Broadcast Rx packets. Only valid in STA mode. */
    uint32_t unicast_Non_Null_Tx_Pkts;             /**< Unicast Tx packets excluding NULL and QoS NULL packets. Only valid in STA mode. */
    uint32_t unicast_Non_Null_Rx_Pkts;             /**< Unicast Rx packets excluding NULL and QoS NULL packets. Only valid in STA mode. */
    uint32_t unicast_Filtered_Accepted_Tx_Pkts;    /**< Unicast filtered and accepted Tx packets. Only valid in STA mode. Must enable WoW. */
    uint32_t unicast_Filtered_Accepted_Rx_Pkts;    /**< Unicast filtered and accepted Rx packets. Only valid in STA mode. Must enable WoW.  */
    uint32_t multicast_Filtered_Accepted_Tx_Pkts;  /**< Multicast filtered and accepted Tx packets. Only valid in STA mode. Must enable WoW.  */
    uint32_t multicast_Filtered_Accepted_Rx_Pkts;  /**< Multicast filtered and accepted Rx packets. Only valid in STA mode. Must enable WoW.  */
    uint32_t broadcast_Filtered_Accepted_Tx_Pkts;  /**< Broadcast filtered and accepted Tx packets. Only valid in STA mode. Must enable WoW.  */
    uint32_t broadcast_Filtered_Accepted_Rx_Pkts;  /**< Broadcast filtered and accepted Rx packets. Only valid in STA mode. Must enable WoW.  */
    uint32_t unicast_Filtered_Rejected_Tx_Pkts;    /**< Unicast filtered and rejected Tx packets. Only valid in STA mode. Must enable WoW.  */
    uint32_t unicast_Filtered_Rejected_Rx_Pkts;    /**< Unicast filtered and rejected Rx packets. Only valid in STA mode. Must enable WoW.  */
    uint32_t multicast_Filtered_Rejected_Tx_Pkts;  /**< Multicast filtered and rejected Tx packets. Only valid in STA mode. Must enable WoW.  */
    uint32_t multicast_Filtered_Rejected_Rx_Pkts;  /**< Multicast filtered and rejected Rx packets. Only valid in STA mode. Must enable WoW.  */
    uint32_t broadcast_Filtered_Rejected_Tx_Pkts;  /**< Broadcast filtered and rejected Tx packets. Only valid in STA mode. Must enable WoW.  */
    uint32_t broadcast_Filtered_Rejected_Rx_Pkts;  /**< Broadcast filtered and rejected Rx packets. Only valid in STA mode. Must enable WoW.  */
    uint32_t null_Tx_Pkts;                         /**< NULL Tx packets. Valid in AP and STA modes. */
    uint32_t null_Rx_Pkts;                         /**< NULL Rx packets. Valid in AP and STA modes. */
    uint32_t qos_Null_Tx_Pkts;                     /**< QOS NULL Tx packets. Valid in AP and STA modes. */
    uint32_t qos_Null_Rx_Pkts;                     /**< QOS NULL Rx packets. Valid in AP and STA modes. */
    uint32_t ps_Poll_Tx_Pkts;                      /**< PS Poll Tx packets. Valid in STA mode. */
    uint32_t ps_Poll_Rx_Pkts;                      /**< PS Poll Rx packets. Valid in AP and STA modes. */
    uint32_t tx_Retry_Cnt;                         /**< Tx retry count.  Only valid in STA mode. */
    uint32_t beacon_Miss_Cnt;                      /**< Beacon miss count.  Only valid in STA mode. */
    uint32_t beacons_Received_Cnt;                 /**< Received beacon miss count.  Only valid in STA mode. */
    uint32_t beacon_Resync_Success_Cnt;            /**< Beacon resync success count.  Only valid in STA mode. */
    uint32_t beacon_Resync_Failure_Cnt;            /**< Beacon resync failure count.  Only valid in STA mode. */
    uint32_t curr_Early_Wakeup_Adj_In_Ms;          /**< Current early wakeup adjustment.  Only valid in STA mode. */
    uint32_t avg_Early_Wakeup_Adj_In_Ms;           /**< Average early wakeup adjustment.  Only valid in STA mode. */
    uint32_t early_Termination_Cnt;                /**< Early termination count.  Only valid in STA mode. */
    uint32_t uapsd_Trigger_Rx_Cnt;                 /**< UAPSD trigger Rx count.  Only valid in STA mode. */
    uint32_t uapsd_Trigger_Tx_Cnt;                 /**< UAPSD trigger Tx count.  Only valid in STA mode. */
}qapi_WLAN_Device_Stats_t;

/**
@ingroup qapi_wlan
Data structure that represents buffer inforamtion for WLAN Transmit/Receive.
The buffer information in this data structure are independent of the virtual devices.
*/
typedef struct
{
	uint8_t htc_inf_cur_cnt;		/**< The count of current available buffers in htc interface. */
	uint8_t htc_inf_reaped_cnt;	    /**< The count of reaped buffers in htc interface. */
	uint8_t mac_inf_cur_cnt;		/**< The count of current available buffers in mac interface. */
	uint8_t mac_inf_reaped_cnt;		/**< The count of reaped buffers in mac interface. */
	uint8_t fw_inf_cur_cnt;         /**< The count of current available buffers in fw interface. */
	uint8_t fw_inf_reaped_cnt;		/**< The count of reaped buffers in fw interface. */
	uint8_t free_buf_cnt;			/**< The count of free buffers in buffer pool. */
	uint8_t mgmt_buf_cnt;			/**< The count of remaining management buffers. */
	uint8_t smmgmt_buf_cnt;			/**< The count of remaining small management buffers. */
	uint8_t num_txbuf_queued;		/**< The number of buffers queued for tx. */
	uint8_t num_rxbuf_queued;		/**< The number of buffers queued for rx. */
	uint8_t reserved;
} qapi_WLAN_Common_TxRx_Buffer_Info;

/**
@ingroup qapi_wlan
Data structure that represents system level statistics information of the WLAN
subsystem. The statistics information in this data structure are independent of
the virtual devices.
*/
typedef struct
{
    uint32_t total_Active_Time_In_Ms;
       /**< Total time in milliseconds for which the WLAN subsystem has been Active. */
    uint32_t total_Powersave_Time_In_Ms;
       /**< Total time in milliseconds for which the WLAN subsystem has been in Power Save. */
	qapi_WLAN_Common_TxRx_Buffer_Info  txrx_buffer_info;
	   /**< The buffer information for WLAN transmit/receive. */
    uint32_t FCS_Error_Rx_Pkts;
	   /**< Receive FCE error packets. */
} qapi_WLAN_Common_Stats_t;

/**
@ingroup qapi_wlan
Data structure that represents extensional WLAN statistics information for each virtual device.
This data structure contains the number of amsdu packets received on the requested virtual device.
*/
typedef struct
{
    uint32_t rx_amsdu_pkts;      /**Number of received AMSDU packets. Valid in AP and STA modes. */
    uint32_t reserved; 			 /** reserved data for future */
} qapi_WLAN_Device_Stats_Ext_t;

/**
@ingroup qapi_wlan
Data structure that represents extensional WLAN statistics information for each virtual device on version 2.
*/
typedef struct
{
    uint16_t wmi_event_missed_last;             /**< The last missed WMI event. Valid in AP and STA modes. */
    uint16_t reserved;                          /**< Reserved. */
    uint32_t wmi_event_missed_bitmap;           /**< Bitmap of missed WMI events. Valid in AP and STA modes. */
    uint32_t wmi_event_missed_cnt;              /**< Count of total missed WMI events. Valid in AP and STA mode. */
    uint32_t valid_Rx_Pkts;                        /**< Receive valid packets. Only valid in STA mode. */
    uint32_t addr_Miss_Match_Rx_Pkts;              /**< Receive valid packets but filtered out by one of the filters. Valid in AP and STA modes. */
    uint32_t avarage_Rssi_Data_Pkts;               /**< Average RSSI for all valid data packets received. Valid in AP and STA modes. */
    uint32_t avarage_Rssi_Mgmt_Pkts;               /**< Average RSSI for all valid management packets received. Valid in AP and STA modes. */
} qapi_WLAN_Device_Stats_Ext2_t;

/**
@ingroup qapi_wlan
Data structure that represents system and MAC statistics of the WLAN subsytem. This
structure includes both virtual device-specific statistics and virtual device
independent statistics.
*/
typedef struct
{
    qapi_WLAN_Device_Stats_t dev_Stats;   /**< Per device statistics. */
    qapi_WLAN_Common_Stats_t common_Stats;  /**< Statistics common to all devices. */
	qapi_WLAN_Device_Stats_Ext_t dev_Stats_Ext; /**< Per device extensional statistics. */
	qapi_WLAN_Device_Stats_Ext2_t dev_Stats_Ext2;   /**< Per device extensional statistics for version 2. */
}qapi_WLAN_Statistics_t;

/**
@ingroup qapi_wlan
Data structure used to retrieve WLAN statistics from the driver. The application
passes da ata structure of this type to get the statistics information by calling
qapi_WLAN_Get_Param() with the command ID as __QAPI_WLAN_PARAM_GROUP_WIRELESS_STATS.
*/
typedef struct
{
    uint8_t                     reset_Counters_Flag;
                      /**<
                      [IN] 1: Reset statistics counters after getting the current count; \n
                           0: Do not reset counters
                      */
    qapi_WLAN_Statistics_t      *wlan_Stats_Data;
                      /**<
                      [OUT] Data structure that is to be filled by the driver
                      that actually holds the statistics information.
                      */
}qapi_WLAN_Get_Statistics_t;

/**
@ingroup qapi_wlan
Data structure used to retrieve the current regulatory domain channel list. The application
passes a data structure of this type to get the channel list information by calling
qapi_WLAN_Get_Param() with the command ID as __QAPI_WLAN_MAX_NUM_CUR_REGDOAMIN_CHANLIST_CHNNELS.

When using the getRegDomainChannelList() command, this data structure should be allocated from heap memory (not from the stack).
*/

typedef struct
{
    uint16_t                     channel_List[__QAPI_WLAN_MAX_NUM_CUR_REGDOAMIN_CHANLIST_CHNNELS];  /**< WLAN Channel list array */
    uint8_t                      number_Of_Channels; /**< Number of channels supported in the current regulatory setting. */


}qapi_WLAN_Get_Channel_List_t;

/**
@ingroup qapi_wlan
Data structure that the application uses to interpret the event payload
information on receiving QAPI_WLAN_PREFERRED_NETWORK_OFFLOAD_CB_E from the WLAN driver.
The event data provides the result of the PNO operation.
*/
typedef struct
{
    uint8_t   profile_Matched;
          /**<
          A value of 1 indicates that a matching profile has been found on the PNO scan. \n
          A value of 0 indicates that a matching profile has not been found on the PNO scan.
          */
    uint8_t   matched_Index;
          /**<
          Index of a matched profile. This index number is the same number
          given by the application when adding an AP profile to the PNO profile list.
          */
    int32_t   rssi;
          /**< RSSI of the AP found that corresponds to the matching profile index. */
    uint32_t  num_Fast_Scans_Remaining; /**< Number of fast scans remaining. */
} qapi_WLAN_Preferred_Network_Offload_Info_t;

/**
@ingroup qapi_wlan
Data structure that enumerates the list of WLAN subsystem events, which the
application can chose to filter, thereby avoiding application processor wakeups
because of these events.
*/
typedef enum
{
  QAPI_WLAN_BSSINFO_EVENTID_E               = 0x1004, /**< Event ID to filter the BSS Info event sent by the WLAN firmware. */
  QAPI_WLAN_EXTENSION_EVENTID_E             = 0x1010, /**< Event ID to filter the WMI Extension events sent by the WLAN firmware; applicable for device-0 only. */
  QAPI_WLAN_CHANNEL_CHANGE_EVENTID_E        = 0x101A, /**< Event ID to filter the channel change event sent by the WLAN firmware. */
  QAPI_WLAN_PEER_NODE_EVENTID_E             = 0x101B, /**< Event ID to filter the events sent by the WLAN firmware during an IBSS connection; applicable for device-0 only. */
  QAPI_WLAN_ADDBA_REQ_EVENTID_E             = 0x1020, /**<
                                                       Event ID to filter the event sent by the WLAN firmware notifying the host that
                                                       it has sent an ADDBA request for a BlockACK session.
                                                       */
  QAPI_WLAN_ADDBA_RESP_EVENTID_E            = 0x1021, /**<
                                                       Event ID to filter the event sent by the WLAN firmware indicating the reception
                                                       of an ADDBA response for the ADDBA request sent.
                                                       */
  QAPI_WLAN_DELBA_REQ_EVENTID_E             = 0x1022, /**< Event ID to filter a DELBA request event. */
  QAPI_WLAN_P2P_INVITE_REQ_EVENTID_E        = 0x103E, /**< Event ID to filter the events from the WLAN firmware when a P2P invitation request is received from a device; applicable for device-0 only. */
  QAPI_WLAN_P2P_INVITE_RCVD_RESULT_EVENTID_E= 0x103F, /**< Event ID to filter the events from the WLAN firmware when a P2P invitation response is sent to a peer device; applicable for device-0 only */
  QAPI_WLAN_P2P_INVITE_SENT_RESULT_EVENTID_E= 0x1040, /**< Event ID to filter the events from the WLAN firmware when a P2P invitation response is received from a peer device; applicable for device-0 only. */
  QAPI_WLAN_P2P_PROV_DISC_RESP_EVENTID_E    = 0x1041, /**< Event ID to filter the events from the WLAN firmware when a P2P provision discovery response is received from a peer device; applicable for device-0 only. */
  QAPI_WLAN_P2P_PROV_DISC_REQ_EVENTID_E     = 0x1042, /**< Event ID to filter the events from the WLAN firmware when a P2P provision discovery request is received from a peer device; applicable for device-0 only. */
  QAPI_WLAN_P2P_START_SDPD_EVENTID_E        = 0x1045, /**< Event ID to filter a P2P service discovery, provision discovery start events; applicable for device-0 only. */
  QAPI_WLAN_P2P_SDPD_RX_EVENTID_E           = 0x1046, /**< Event ID to filter a P2P service discovery, provision discovery Rx events; applicable for device-0 only. */
  QAPI_WLAN_DBGLOG_EVENTID_E                = 0x3008, /**<
                                                       Event ID to filter the DBGLOG event sent by the WLAN firmware when the dbglog buffer
                                                       is full or the threshold is reached; applicable for device-0 only.
                                                       */
  QAPI_WLAN_PKTLOG_EVENTID_E                = 0x300A, /**< Event ID to filter the PKTLOG event sent by the WLAN firmware when the pktlog buffer is full; applicable for device-0 only. */
  QAPI_WLAN_WPS_PROFILE_EVENTID_E           = 0x900F, /**<
                                                       Event ID to filter events sent by the WLAN firmware when a WPS profile is received
                                                       from a successful WPS handhshake.
                                                       */
  QAPI_WLAN_FLOW_CONTROL_EVENTID_E          = 0x9014, /**< Event ID to filter the bus flow control event sent by the WLAN firmware. */
  QAPI_WLAN_P2P_REQ_TO_AUTH_EVENTID_E       = 0x901B, /**< Event ID to filter an event indicating pending authentication for a P2P group owner negotiation request received from a peer device; applicable for device-0 only. */
  QAPI_WLAN_DIAGNOSTIC_EVENTID_E            = 0x901C, /**< Event ID to filter diagnostic events sent by the WLAN firmware; applicable for device-0 only. */
  QAPI_WLAN_WNM_EVENTID_E                   = 0x9048, /**< Event ID to filter WNM events sent by the WLAN firmware. */
} qapi_WLAN_Filterable_Event_e;

/**
@ingroup qapi_wlan
Data structure that enables applications to filter unnecessary events from
waking up the application processor.

The applications can program event filters by invoking qapi_WLAN_Set_Param() with
command ID __QAPI_WLAN_PARAM_GROUP_WIRELESS_EVENT_FILTER.

This data structure does not allow applications to incrementally add filter
events. It is only possible to enable or disable filtering for all events at once.
*/
typedef struct
{
    qapi_WLAN_Enable_e                   action;
          /**< 1 to start filtering a list of events provided, 0 to disable filtering. */
    uint32_t                             num_Events;
          /**< Number of events provided by the application in event[]. */
    qapi_WLAN_Filterable_Event_e         event[__QAPI_WLAN_MAX_NUM_FILTERED_EVENTS];
          /**<
          Actual list of events to be filtered. The events being provided
          should be of type #qapi_WLAN_Filterable_Event_e and the number of
          events in this list should not exceed __QAPI_WLAN_MAX_NUM_FILTERED_EVENTS.
          */
} qapi_WLAN_Event_Filter_t;


/**
@ingroup qapi_wlan
Enumeration that provides various blocking options for qapi_WLAN_Set_Param()
and qapi_WLAN_Get_Param() APIs when invoked by the application.
*/
typedef enum
{
  QAPI_WLAN_NO_WAIT_E  = 0,   /**< Makes a QAPI nonblocking. */
  QAPI_WLAN_WAIT_E     = 1    /**< Makes a QAPI blocking. */
} qapi_WLAN_Wait_For_Status_e;

/**
@ingroup qapi_wlan
Enumeration that provides a list of supported operating modes for each virtual device.
*/
typedef enum
{
    QAPI_WLAN_DEV_MODE_STATION_E = 0,
        /**<
        Infrastructure non-AP Station mode, supported in both Single Device
        mode and Concurrent mode. When operating in Concurrent mode, virtual
        device 1 must be used for the Station mode of operation.
        */
    QAPI_WLAN_DEV_MODE_AP_E      = 1, /**< Soft-AP mode */
        /**<
        Infrastructure AP Station mode, supported in both Single Device
        mode and Concurrent mode. When operating in Concurrent mode, virtual
        device 0 must be used for the AP mode of operation.
        */
    QAPI_WLAN_DEV_MODE_ADHOC_E   = 2, /* Adhoc mode */
        /**<
        Independent BSS mode of operation, supported in Single Device mode
        only. Virtual device 0 must be used for the IBSS mode of operation.
        */
    QAPI_WLAN_DEV_MODE_INVALID_E = 3  /**< Invalid device mode. */
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
    QAPI_WLAN_11B_MODE_E        = 0x0,  /**< 802.11b. */
    QAPI_WLAN_11G_MODE_E        = 0x1,  /**< 802.11g. */
    QAPI_WLAN_11NG_HT20_MODE_E  = 0x2,  /**< 802.11b/g/n HT20. */
	QAPI_WLAN_11A_MODE_E        = 0x3,  /**< 802.11a. */
    QAPI_WLAN_11A_HT20_MODE_E	= 0x4,  /**< 802.11a with HT20. */
    QAPI_WLAN_11ABGN_HT20_MODE_E	= 0x5,  /**< 802.11 CCX. */
} qapi_WLAN_Phy_Mode_e;
/**
@ingroup qapi_wlan
Enumeration that provides 11n HT configurations.
*/
typedef enum
{
    QAPI_WLAN_11N_DISABLED_E      = 0x1,  /**< 802.11n disabled. */
    QAPI_WLAN_11N_HT20_E          = 0x2,  /**< 802.11n with bandwith 20M. */
    QAPI_WLAN_11N_HT40_E          = 0x3,  /**< 802.11n with bandwith 40M. */
} qapi_WLAN_11n_HT_Config_e;

/** @cond */
/**
@ingroup qapi_wlan
Data structure used to get firmware version information.
*/
typedef struct //qapi_WLAN_Firmware_Version_s
{
    uint32_t        host_ver;   /**< Host version. */
    uint32_t        target_ver; /**< Target version. */
    uint32_t        wlan_ver;   /**< WLAN version. */
    uint32_t        abi_ver;    /**< Device ABI version. */
}qapi_WLAN_Firmware_Version_t;
/** @endcond */

/**
@ingroup qapi_wlan
Data structure to get the WLAN host/firmware version information in string format.
*/
typedef struct //qapi_WLAN_Firmware_Version_String_s
{
    uint8_t        host_Version[__QAPI_WLAN_VERSION_SUBSTRING_LEN];   /**< Not used, Host version. */
    uint8_t        target_Version[__QAPI_WLAN_VERSION_SUBSTRING_LEN]; /**< Not used, Target version. */
    uint8_t        wlan_Version[__QAPI_WLAN_VERSION_SUBSTRING_LEN];   /**< WLAN version. */
    uint8_t        abi_Version[__QAPI_WLAN_VERSION_SUBSTRING_LEN];    /**< Device ABI version. */
}qapi_WLAN_Firmware_Version_String_t;

/**
@ingroup qapi_wlan
Data structure that enables the application to configure scan parameters for subsequent
scan operations.
*/
typedef struct //qapi_WLAN_Scan_Params_s
{
    uint16_t                           fg_Start_Period;
    /**<
    Foreground scan start period in seconds.\n
    If this parameter is set to 0, the period is set to the default value of 1 second.\n
    If this parameter is set to 0xFFFF, the periodic background scan is disabled.
    */
    uint16_t                           fg_End_Period;
    /**<
    Foreground scan end period in seconds.\n
    If this parameter is set to 0, the period is set to the default value of 60 seconds.
    */
    uint16_t                           bg_Period;
    /**<
    Background scan period in seconds.
    Stations can use this mode to scan for neighboring APs without disconnecting
    from the AP to which they are currently connected.\n
    If this parameter is set to 0, the period is set to the default value of 60 seconds.\n
    If this parameter is set to 0xFFFF, the periodic background scan is disabled.
    */
    uint16_t                           max_Act_Chan_Dwell_Time_In_Ms;
    /**<
    Active channel maximum dwell time in ms.
    If this parameter is set to 0, the default period is 20 ms.
    */
    uint16_t                           passive_Chan_Dwell_Time_In_Ms;
    /**<
    Channel dwell time for passive scan mode in which the client waits at each channel to receive beacon frames from APs within the proximity.\n
    If set to 0, the period is set to the default value of 50 ms.
	*/
    uint8_t                            short_Scan_Ratio;
    /**<
    Ratio of the number of short scans per long scan for a foreground search. The default value is 3.
	Short scan is done only in the channels with configured SSIDs, whereas long scan is done in all channels.
	*/
    uint8_t                            scan_Ctrl_Flags;
    /**<
    Scan control flags.\n
    - 0x00 -- Use the current scan control settings.
    - 0xFF -- Disable all scan control flags; this will set the flag to 0.\n
    For setting specific scan control flags, use each bit in this byte-sized field as one flag.\n
    For each bit, 1 = TRUE and 0 = FALSE.\n
    - Bit0 -- Scan is allowed during connection.\n
    - Bit1 -- Scan is allowed for the SSID station if it is already connected.\n
    - Bit2 -- Enable an active scan.\n
    - Bit3 -- Scan is allowed for roaming when a beacon misses or a low signal strength is identified.\n
    - Bit4 -- Follow the customer BSSINFO reporting rule.\n
    - Bit5 -- Device is to scan after a disconnection.\n
    - Bit6 -- To be implemented, Scan complete event with a cancelled status is generated when a scan is preempted before it is completed.\n
    - Bit7 -- Scanning DFS channels is to be skipped.
	*/
    uint16_t                           min_Act_Chan_Dwell_Time_In_Ms;
    /**<
    Minimum dwell time at each channel in case there is no activity during Active Scan mode.\n
    The default value is 0, where each channel follows the period defined by the maximum active channel dwell time.
    */
    uint16_t                           max_Act_Scan_Per_Ssid;
    /**< Maximum number of scans allowed per channel to search for configured SSIDs during Active mode. Default value is 1. */
    uint32_t                           max_Dfs_Chan_Act_Time_In_Ms;
    /**<
    Maximum allowed time for scanning in DFS channels. The default is 2000 ms.
    */
} qapi_WLAN_Scan_Params_t;

/**
@ingroup qapi_wlan
Enumeration of a list of supported power modes in the WLAN subsystem.
*/
typedef enum
{
    QAPI_WLAN_POWER_MODE_REC_POWER_E = 1,
      /**< Power Save mode that enables both MAC and system power save. */
    QAPI_WLAN_POWER_MODE_MAX_PERF_E  = 2
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
typedef enum
{
    QAPI_WLAN_POWER_MODULE_USER_E     = 0,
          /**< If the application requires the system to be in Maximum Performance mode. */
    QAPI_WLAN_POWER_MODULE_WPS_E      = 1,
          /**<
          WPS handshakes should be in Maximum Performance mode
          when the application initiates a WPS handshake. After a WPS handshake is completed,
          the application can request Power Save mode with the source module as
          WPS.
          */
    QAPI_WLAN_POWER_MODULE_P2P_E      = 2,
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
    QAPI_WLAN_POWER_MODULE_SOFTAP_E   = 3,
          /**<
          Power mode set by SoftAP module.
          Applications must operate in Maximum Performance mode as long as
          the device is operating in SoftAP mode.
          */
    QAPI_WLAN_POWER_MODULE_SRCALL_E   = 4,
          /**<
          Power mode request by the store recall module. At this point, the WLAN
          driver internally uses this and the application is not expected to use it.
          */
    QAPI_WLAN_POWER_MODULE_TKIP_E     = 8,
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
    QAPI_WLAN_POWER_MODULE_PWR_MAX_E  = 10, /**< Maximum value for modules */
} qapi_WLAN_Power_Module_e;

/**
@ingroup qapi_wlan
Data structure used to request the necessary operating power mode (Maximum
Performance or Power Save (rec_power) mode).
*/
typedef struct //qapi_WLAN_Power_Mode_Params_s
{
    qapi_WLAN_Power_Mode_e        power_Mode;    /**< Power mode to be set */
    qapi_WLAN_Power_Module_e      power_Module;  /**< Module requesting power mode change */
} qapi_WLAN_Power_Mode_Params_t;

/**
@ingroup qapi_wlan
Enumeration of supported DTIM policies.
*/
typedef enum
{
    QAPI_WLAN_DTIM_IGNORE_E = 1,
          /**<
          Non-AP station wakeups only at TIM intervals; ignores
          content after beacon (CAB).
          */
    QAPI_WLAN_DTIM_NORMAL_E = 2, /**< Non-AP station wakeups at TIM intervals. */
    QAPI_WLAN_DTIM_STICK_E  = 3, /**< Non-AP station wakeups only at DTIM intervals. */
    QAPI_WLAN_DTIM_AUTO_E   = 4, /**< Non-AP station wakeups at both TIM and DTIM intervals. */
    QAPI_WLAN_DTIM_XSTICK_E = 5  /**< Non-AP station wakeups only at X DTIM intervals. */
} qapi_WLAN_DTIM_Policy_e;

/**
@ingroup qapi_wlan
Enumeration that identifies Tx wake-up policy.
*/
typedef enum
{
    QAPI_WLAN_TX_WAKEUP_ON_SLEEP_E = 1,
        /**<
        Transmit packets can exit WLAN from the protocol power save after
        transmitting 'n' number of packets in Power Save mode. The value
        of 'n' can be programmed through qapi_WLAN_Power_Policy_Params_t.
        */
    QAPI_WLAN_TX_WAKEUP_SKIP_ON_SLEEP_E = 2
        /**<
        Transmit packets need not exit WLAN from protocol power save irrespective
        of the number of packets transmitted while in Power Save mode.
        */
} qapi_WLAN_TX_Wakeup_Policy_e;

/**
@ingroup qapi_wlan
Enumeration that identifies the WLAN power save event policy.
*/
typedef enum
{

    QAPI_WLAN_PS_SEND_POWER_SAVE_FAIL_EVENT_ALWAYS_E = 1,
        /**<
        Firmware should indicate NULL frame transmission failures when
        entering and exiting protocol power save (PM bit transitions) through
        target events.
        */
    QAPI_WLAN_PS_IGNORE_POWER_SAVE_FAIL_EVENT_DURING_SCAN_E = 2
        /**<
        Firmware can ignore the NULL frame transmission failures when
        entering and exiting protocol power save (PM bit transitions).
        */
} qapi_WLAN_Power_Save_Policy_e;

/**
@ingroup qapi_wlan
Data structure to configure WLAN power policy information.

@sa
qapi_WLAN_Set_Param
__QAPI_WLAN_PARAM_GROUP_WIRELESS_POWER_MODE_POLICY
*/
typedef struct //qapi_WLAN_Power_Policy_Params_s
{
    uint16_t                           idle_Period_In_Ms;
          /**<
          Idle period during traffic after which WLAN can go to sleep.
          The system default for this parameter is 200 ms.
          */
    uint16_t                           ps_Poll_Num;
          /**<
          This parameter dictates the number of contiguous PS-Poll frames that the WLAN firmware
          is to send before sending an 802.11 NULL frame to indicate a power save exit to
          the access point. The system default for this parameter is 1.
          */
    qapi_WLAN_DTIM_Policy_e            dtim_Policy;
          /**<
          DTIM policy to be used. The application can select one of the DTIM policies from
          qapi_WLAN_DTIM_Policy_e. The system default for this parameter is QAPI_WLAN_DTIM_NORMAL_E.
          */
    qapi_WLAN_TX_Wakeup_Policy_e       tx_Wakeup_Policy;
          /**<
          Tx wake-up policy to be used. The application can select one of the Tx wake-up policies
          from qapi_WLAN_TX_Wakeup_Policy_e. The system default for this parameter is
          QAPI_WLAN_TX_WAKEUP_ON_SLEEP_E.
          */
    uint16_t                           num_Tx_To_Wakeup;
          /**<
          Number of contiguous transmit packets after which the WLAN firmware
          is to exit Protocol Power Save mode. This parameter is effective
          only if tx_Wakeup_Policy is set to QAPI_WLAN_TX_WAKEUP_ON_SLEEP_E.
          The system default for this parameter is 1.
          */
    qapi_WLAN_Power_Save_Policy_e      ps_Fail_Event_Policy;
          /**<
          Policy to indicate a power save entry/exit failure event. The application
          can select one of the policies from qapi_WLAN_Power_Save_Policy_e.
          The system default for this parameter is
          QAPI_WLAN_PS_SEND_POWER_SAVE_FAIL_EVENT_ALWAYS_E.
          */
} qapi_WLAN_Power_Policy_Params_t;

/**
@ingroup qapi_wlan
Data structure for RSSI thresholds.
*/
typedef struct //qapi_WLAN_Rssi_Threshold_Params_s
{
    uint32_t    poll_Time;                /**< Polling time as a factor of LI. */
    int16_t     threshold_Above1_Val;     /**< Lowest of the upper thresholds. */
    int16_t     threshold_Above2_Val;     /**< Higher than above1. */
    int16_t     threshold_Above3_Val;     /**< Higher than above2. */
    int16_t     threshold_Above4_Val;     /**< Higher than above3. */
    int16_t     threshold_Above5_Val;     /**< Higher than above5. */
    int16_t     threshold_Above6_Val;     /**< Highest of the upper thresholds. */
    int16_t     threshold_Below1_Val;     /**< Lowest of lower thresholds. */
    int16_t     threshold_Below2_Val;     /**< Higher than below1. */
    int16_t     threshold_Below3_Val;     /**< Higher than below2. */
    int16_t     threshold_Below4_Val;     /**< Higher than below3. */
    int16_t     threshold_Below5_Val;     /**< Higher than below4. */
    int16_t     threshold_Below6_Val;     /**< Highest of the lower thresholds. */
    uint8_t     weight;                   /**< Alpha. */
} qapi_WLAN_Rssi_Threshold_Params_t;

/**
@ingroup qapi_wlan
Data structure for low RSSI control.
*/
typedef struct //qapi_WLAN_LowRssi_RoamControl_Params_s
{
	int16_t  lowrssi_scan_threshold;  /**< Roam low RSSI threshold. */
	uint16_t reserved[2];             /**< Reserved. */
}qapi_WLAN_LowRssi_RoamControl_Params_t;

/**
@ingroup qapi_wlan
Data structure to be used if the application wants to perform
a channel switch after a given number of beacon intervals.

A channel switch request can be issued only when the device is operating in SoftAP mode.

On receiving this request, the WLAN firmware starts adding channel switch
information elements in its beacons with the necessary information until the channel
switch operation is performed.
*/
typedef struct //qapi_WLAN_Channel_Switch_s
{
    uint32_t    channel;
        /**< New wireless channel frequency to which the SoftAP should switch. */
    uint8_t	tbtt_Count;
        /**< Number of beacon intervals after which the channel switch is to be performed. */
} qapi_WLAN_Channel_Switch_t;

/**
@ingroup qapi_wlan
Enumeration of various IP versions, which is used for the
TCP Keepalive offload feature.
*/
typedef enum
{
    QAPI_WLAN_IP_PROTOCOL_V4_E = 4, /**< Used to indicate IPv4 protocol. */
    QAPI_WLAN_IP_PROTOCOL_V6_E = 6  /**< Used to indicate IPv6 protocol. */
}qapi_WLAN_IP_Protocol_Type_e;

/**
@ingroup qapi_wlan
Data structure that enables the application to enable/disable the TCP Keepalive offload
feature along with the ability to configure session-independent parameters of the
TCP Keepalive offload feature.

@sa
__QAPI_WLAN_PARAM_GROUP_WIRELESS_TCP_KEEPALIVE_OFFLOAD_ENABLE \n
qapi_WLAN_Set_Param
*/
typedef struct
{
    uint8_t enable;
        /**< Used to enable (1) or disable (0) the TCP Keepalive offload feature. */
    uint16_t keepalive_Interval;
        /**<
        TCP Keepalive interval in seconds. The keepalive frames for every
        configured TCP session are sent periodically with this interval as
        periodicity.
        */
    uint16_t keepalive_Ack_Recv_Threshold;
        /**<
        Number of keepalive frames to try before confirming that the remote connection is
        dead if TCP acknowledgements are not received from the peer.
        */
} qapi_WLAN_TCP_Offload_Enable_t;

/**
@ingroup qapi_wlan
Data structure to configure parameters for a TCP session
for which the keepalive transmission is to be offloaded to the firmware.

The application should ensure that maximum number of offloaded TCP KA sessions does not
exceed 3.

This data structure allows parameters to be configured for one TCP session by invoking
qapi_WLAN_Set_Param() with __QAPI_WLAN_PARAM_GROUP_WIRELESS_TCP_KEEPALIVE_OFFLOAD_SESSION_CFG
as the command ID.

@sa
__QAPI_WLAN_PARAM_GROUP_WIRELESS_TCP_KEEPALIVE_OFFLOAD_SESSION_CFG
qapi_WLAN_Set_Param
*/
typedef struct
{
    int32_t         sock_Id;
          /**< Socket handle of the TCP session to be offloaded. */
    int16_t         src_Port;
          /**< Source port of the TCP session in host byte order. */
    int16_t         dst_Port;
          /**< Destination port of the TCP session in host byte order. */
    uint32_t        src_IP;
          /**<
          Source IPv4 address of the TCP session in host byte order
          if ip_Protocol_Type is of type IPv4.
          */
    uint32_t        dst_IP;
          /**<
          Destination IPv4 address of the TCP session in host byte order
          if ip_Protocol_Type is of type IPv4.
          */
    uint8_t         dest_MAC[__QAPI_WLAN_MAC_LEN];
          /**< MAC address of the TCP peer that corresponds to the destination IP address. */
    int32_t         sequence_Num;
          /**< TCP sequence number of the TCP session at the time of enabling the KA offload. */
    int32_t         ack_Sequence_Num;
          /**<
          TCP acknowledgement number of the TCP session at the time of enabling
          the KA offload.
          */
    qapi_WLAN_IP_Protocol_Type_e ip_Protocol_Type;
          /**<
          IP protocol type of the socket: IPv4/IPv6 (should be one of the
          types in qapi_WLAN_IP_Protocol_Type_e).
          */
    uint8_t         src_IP_v6addr[__QAPI_WLAN_IPV6_ADDR_LEN];
          /**<
          Source IPv6 address of the TCP session if ip_Protocol_Type
          is of type IPv6.
          */
    uint8_t         dst_IP_v6addr[__QAPI_WLAN_IPV6_ADDR_LEN];
          /**<
          Destination IPv6 address of the TCP session if ip_Protocol_Type
          is of type IPv6.
          */
} qapi_WLAN_TCP_Offload_Config_Params_t;

/**
@ingroup qapi_wlan
Data structure to store TCP keepalive event information.

This data structure represents the session-specific information when a TCP KA
offload event is received from the WLAN firmware. The combination of
socket ID, source port, and destination port can be used to uniquely identify a TCP
session.
*/
typedef struct
{
    int32_t  sock_Id;
        /**< Socket ID of the TCP session for which the event is received. */
    uint32_t sequence_Num;
        /**<
        Updated TCP sequence number after transmitting a few TCP KA frames
        from the firmware.
        */
    uint32_t ack_Sequence_Num;
        /**<
        Updated TCP acknowledgement number after transmitting a few TCP KA frames
        from the firmware.
        */
    uint16_t src_Port;
        /**< Source port of the TCP session for which the event is received. */
    uint16_t dst_Port;
        /**< Destination port of the TCP session for which the event is received. */
    uint8_t status;
        /**<
          Status of the TCP KA offload for this TCP session:
          - A value of 1 indicates that the peer did not transmit an acknowledgment for the keepalive
            frames transmitted by the firmware
          - A value of 2 indicates that the TCP KA offload feature has been disabled
            by the application. An event with this status is needed to
            ensure that the TCP sequence number and the TCP acknowledgement number are
            synchronized between the TCP stack in the host and the offloaded TCP KA
            offload feature in the WLAN firmware. @vertspace{-14}
        */
} qapi_WLAN_TCP_Keepalive_Event_Info_t;

/**
@ingroup qapi_wlan
Data structure to interpret the event payload
when the WLAN driver invokes an application-registered callback with the event ID
QAPI_WLAN_TCP_KEEPALIVE_OFFLOAD_CB_E.

This event is received in two cases from the firmware -- when TCP ACK is
not received from the peer for the keepalive frames sent by the firmware, and when the TCP
KA feature is disabled by the application.
*/
typedef struct
{
    uint32_t  session_cnt;
          /**<
          Number of TCP KA sessions for which information is available in the
          event_info parameter.
          */
    qapi_WLAN_TCP_Keepalive_Event_Info_t event_info[1];
          /**<
          Flexible array that holds the TCP KA event information
          for one or more TCP sessions. The number of sessions is available in
          the session_cnt parameter.
          */
} qapi_WLAN_TCP_Keepalive_Event_t;

/**
@ingroup qapi_wlan
Data structure to customize the WLAN driver buffer pool parameters.

@sa
qapi_WLAN_Set_Param \n
__QAPI_WLAN_PARAM_GROUP_SYSTEM_DRIVER_NETBUF_POOL_SIZE
*/
typedef struct //qapi_WLAN_A_Netbuf_Pool_Config_s
{
    uint32_t pool_Size;
        /**<
        Number of driver buffer objects the WLAN driver is to use for transmitting/receiving
        data packets over WLAN. This buffer pool is only for WLAN driver metadata and not for
        actual packet buffers. Modifying this parameter can have an impact on WLAN performance.
        The system default for this parameter is 10.
        */
    uint32_t rx_Threshold;
        /**<
        Number of driver buffer objects to reserve for receive.
        The system default for this parameter is 2.
        */
} qapi_WLAN_A_Netbuf_Pool_Config_t;


/** @cond */

/**
@ingroup qapi_wlan
Enum for the GPIO active states that the host can configure for wake-up in the WLAN firmware.
The values can be low or high.
*/
typedef enum
{
    QAPI_WLAN_WOW_GPIO_ACTIVE_LOW_E = 0, /**< Used to configure the GPIO active state in the WLAN firmware to wake up the host with GPIO in the active LOW state. */
    QAPI_WLAN_WOW_GPIO_ACTIVE_HIGH_E,    /**< Used to configure the GPIO active state in the WLAN firmware to wake up the host with GPIO in the active HIGH state. */
}qapi_WLAN_Wow_Gpio_Active_Edge_e;

/**
@ingroup qapi_wlan
Enum for the GPIO trigger mechanism that the host can configure in the WLAN firmware to wake up through GPIO with an edge/level triggered mechanism.
*/
typedef enum
{
    QAPI_WLAN_WOW_GPIO_EDGE_TRIGGER_E = 0, /**< Wake up the host with GPIO in the EDGE triggered mechanism. */
    QAPI_WLAN_WOW_GPIO_LEVEL_TRIGGER_E,    /**< Wake up the host with GPIO in the LEVEL triggered mechanism. */
}qapi_WLAN_Wow_Gpio_Trigger_Policy_e;

/** @endcond */

/**
@ingroup qapi_wlan
Enumeration of header types supported by the WLAN packet filtering feature..
*/
typedef enum
{
    QAPI_WLAN_PKT_FILTER_MAC_HEADER_E = 1,
          /**< Used to configure the WOW/packet filter pattern for the MAC header packet. */
    QAPI_WLAN_PKT_FILTER_SNAP_E,
          /**< Used to configure the WOW/packet filter pattern for the SNAP header packet. */
    QAPI_WLAN_PKT_FILTER_ARP_E,
          /**< Used to configure the WOW/packet filter pattern for the ARP packet. */
    QAPI_WLAN_PKT_FILTER_IPV4_E,
          /**< Used to configure the WOW/packet filter pattern for the IPV4 packet. */
    QAPI_WLAN_PKT_FILTER_IPV6_E,
          /**< Used to configure the WOW/packet filter pattern for the IPV6 packet. */
    QAPI_WLAN_PKT_FILTER_ICMP_E,
          /**< Used to configure the WOW/packet filter pattern for the ICMP packet. */
    QAPI_WLAN_PKT_FILTER_IGMP_E,
          /**< Used to configure the WOW/packet filter pattern for the IGMP packet. */
    QAPI_WLAN_PKT_FILTER_ICMPV6_E,
          /**< Used to configure the WOW/packet filter pattern for the ICMPV6 packet. */
    QAPI_WLAN_PKT_FILTER_UDP_E,
          /**< Used to configure the WOW/packet filter pattern for the UDP packet. */
    QAPI_WLAN_PKT_FITLER_TCP_E,
          /**< Used to configure the WOW/packet filter pattern for the TCP packet. */
    QAPI_WLAN_PKT_FITLER_PAYLOAD_E,
          /**< Used to configure the WOW/packet filter pattern for the payload. */
} qapi_WLAN_Pkt_Filter_Type_e;

/** @cond */
/**
@ingroup qapi_wlan
Data structure to configure the WoW GPIO configuration in the WLAN firmware to wake up the host through GPIO.
*/
typedef struct //qapi_WLAN_Wow_Gpio_Config_s
{
    uint32_t                               gpio;          /**< WOW PIN number of the GPIO to be configured to wake up the host by the WLAN firmware. */
    qapi_WLAN_Wow_Gpio_Active_Edge_e       is_Active_Low; /**< GPIO active states. */
    qapi_WLAN_Wow_Gpio_Trigger_Policy_e    trigger_Mechanism; /**< GPIO trigger mechanism. */
} qapi_WLAN_Wow_Gpio_Config_t;
/** @endcond */

/**
@ingroup qapi_wlan
Data structure to add a pattern structure for WOW and filtering.
*/
typedef struct //qapi_WLAN_Add_Pattern_s
{
    uint32_t        pattern_Index;                            /**< Pattern index. */
    uint32_t        pattern_Action_Flag;                      /**< Pattern action flag. */
    uint32_t        offset;                                   /**< Offset. */
    uint32_t        pattern_Size;                             /**< Pattern size. */
    uint16_t        header_Type;                              /**< Header type. */
    uint8_t         pattern_Priority;                         /**< Pattern priority. */
    uint8_t         pattern_Mask[__QAPI_WLAN_PATTERN_MASK];   /**< Pattern mask. */
    uint8_t         pattern[__QAPI_WLAN_PATTERN_MAX_SIZE];    /**< Pattern string. */
}qapi_WLAN_Add_Pattern_t;

/**
@ingroup qapi_wlan
Data structure to delete a pattern structure for WOW and filtering.
*/
typedef struct //qapi_WLAN_Delete_Pattern_s
{
    uint8_t        pattern_Index;                            /**< Pattern index. */
    uint8_t        header_Type;                              /**< Header type. */
}qapi_WLAN_Delete_Pattern_t;

/**
@ingroup qapi_wlan
Data structure to change the action for the default packet filter.
*/
typedef struct //qapi_WLAN_Change_Default_Filter_Action_s
{
    uint8_t        pattern_Action_Flag;              /**< Pattern action flag. */
    uint8_t        header_Type;                      /**< Header type. */
}qapi_WLAN_Change_Default_Filter_Action_t;

/**
@ingroup qapi_wlan
Data structure to set/get the WEP key by the key index.\n
The key can be a maximum of 13 bytes long. Each byte represents either a single ASCII character or two hex characters.
The application is responsible for converting ASCII and hex characters into hex numbers, and vice versa, if needed.

@sa
__QAPI_WLAN_PARAM_GROUP_SECURITY_WEP_KEY_PAIR
qapi_WLAN_Set_Param
qapi_WLAN_Get_Param
*/
typedef struct //qapi_WLAN_Security_Wep_Key_Pair_Params_s
{
    uint32_t   key_Index;  /**< WEP key index for which the key is provided. */
    uint32_t   key_Length; /**< Length of the WEP Key */
    int8_t     *key;       /**<
                           WEP key; memory for this key is to be provided
                           by the application.
                           */
} qapi_WLAN_Security_Wep_Key_Pair_Params_t;

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
typedef enum
{
    QAPI_WLAN_AUTH_NONE_E            = 0, /**< Open mode authentication. */
    QAPI_WLAN_AUTH_WPA_E             = 1,
        /**< Wi-Fi Protected Access v1 Protocol, if a preshared key (PSK) is not used (currently not supported). */
    QAPI_WLAN_AUTH_WPA2_E            = 2,
        /**< Wi-Fi Protected Access v2 Protocol, if a PSK is not used (currently not supported). */
    QAPI_WLAN_AUTH_WPA_PSK_E         = 3,
        /**< Wi-Fi Protected Access v1, if a PSK is used. */
    QAPI_WLAN_AUTH_WPA2_PSK_E        = 4,
        /**< Wi-Fi Protected Access v2, if a PSK is used. */
    QAPI_WLAN_AUTH_WPA_CCKM_E        = 5,
        /**< WPA v1 with Cisco Centralized Key Management (currently not supported). */
    QAPI_WLAN_AUTH_WPA2_CCKM_E       = 6,
        /**< WPA v2 with Cisco Centralized Key Management (currently not supported) */
    QAPI_WLAN_AUTH_WPA2_PSK_SHA256_E = 7,
        /**< WPA2-PSK using SHA256 (currently not supported). */
    QAPI_WLAN_AUTH_WEP_E             = 8,
        /**< Wired Equivalent Privacy (WEP) mode of authentication. */
    QAPI_WLAN_AUTH_WPA3_SAE_E        = 9,
        /**< WPA3 SAE mode of authentication. */
    QAPI_WLAN_AUTH_WPA2_SAE_MIXED_E  = 10,
        /**< WPA2 and SAE mixed mode of authentication. */
    QAPI_WLAN_AUTH_INVALID_E         = 11  /**< Invalid authentication method. */
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
typedef enum
{
    QAPI_WLAN_CRYPT_NONE_E        = 0, /**< No encryption, Open mode. */
    QAPI_WLAN_CRYPT_WEP_CRYPT_E   = 1,
        /**< WEP mode of encryption. */
    QAPI_WLAN_CRYPT_TKIP_CRYPT_E  = 2,
        /**< Temporal Key Integrity Protocol (TKIP). */
    QAPI_WLAN_CRYPT_AES_CRYPT_E   = 3,
        /**< Advanced Encryption Standard (AES). */
    QAPI_WLAN_CRYPT_WAPI_CRYPT_E  = 4,
        /**< WLAN Authentication and Privacy Infrastructure; currently not supported. */
    QAPI_WLAN_CRYPT_BIP_CRYPT_E   = 5,
        /**< Broadcast Integrity Protocol; currently not supported. */
    QAPI_WLAN_CRYPT_KTK_CRYPT_E   = 6,
        /**< Key Transport Key; currently not supported. */
    QAPI_WLAN_CRYPT_INVALID_E     = 7  /**< Invalid encryption type. */
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
Data structure to enable/disable the preferred network
offload (PNO) feature. This data structure also allows applications to configure
some of the profile-independent global parameters of the PNO feature.

The application sets this parameter by invoking qapi_WLAN_Set_Param() with
__QAPI_WLAN_PARAM_GROUP_WIRELESS_PREFERRED_NETWORK_OFFLOAD_ENABLE as the command
ID.

@dependencies
This configuration should be done before adding any PNO profiles.
*/
typedef struct
{
    uint16_t max_Num_Preferred_Network_Profiles;
        /**<
          Maximum number of PNO profiles that the application requires.
          Note that this is used for memory allocation in the WLAN firmware. The application
          must ensure that this value is always less than 15. */
    uint32_t fast_Scan_Interval_In_Ms;
        /**<
          PNO framework allows applications to perform PNO scans in two different
          intervals -- fast scans and slow scans. Fast scans are performed
          with the period of this parameter from the time PNO is enabled to
          fast_Scan_Duration_In_Ms. The periodicity of this scan is provided
          in milliseconds.
        */
    uint32_t fast_Scan_Duration_In_Ms;
        /**<
          Total fast scan duration during which a scan is to be performed every
          fast_Scan_Interval_In_Ms. This duration always starts from the time PNO
          is enabled. The reasoning behind this fast scan logic is to perform
          aggressive PNO scans for some time to actively search for APs. If no matching
          profiles are found for fast scan duration, the PNO scan interval increases to
          slow_Scan_Interval_In_Ms to perform the scan at an increased interval, thereby
          saving power. The fast scan duration is provided in
          milliseconds.
        */
    uint32_t slow_Scan_Interval_In_Ms;
        /**<
          This parameter signifies the PNO scan interval after the fast scan duration
          in fast_Scan_Duration_In_Ms has expired. In order to make
          PNO more power efficient, applications should ensure that this parameter is
          expected to have a value greater than fast_Scan_Interval_In_Ms, and the
          input is given in milliseconds.
        */
    uint8_t start_Network_List_Offload;
        /**< Enable/disable a switch for the PNO feature; 1: Enable PNO, 0: Disable PNO. */
} qapi_WLAN_Preferred_Network_Offload_Config_t;

/**
@ingroup qapi_wlan
Data structure to set an AP profile information to search for
when enabling the PNO feature.

This data structure is used to set the profile by passing the required profile
information to qapi_WLAN_Set_Param() with __QAPI_WLAN_PARAM_GROUP_WIRELESS_PREFERRED_NETWORK_PROFILE
as the command ID.
*/
typedef struct
{
    uint8_t         index;
        /**<
          Index of the AP profile to be set. This value should start with value
          0 (for the first profile) and be incremented as each profile is added.
          However, the applications must ensure that this value does not exceed
          (max_Num_Preferred_Network_Profiles - 1) whereas
          max_Num_Preferred_Network_Profiles is the parameter provided by the
          application when enabling the PNO feature.
        */
    uint8_t         ssid_Len;
        /**< Length of the SSID of the AP profile that the application is looking for in the PNO scan. */
    uint8_t         ssid[__QAPI_WLAN_MAX_SSID_LEN];
        /**< SSID of the AP profile that the application is looking for in the PNO scan. */
    qapi_WLAN_Auth_Mode_e      auth_Mode;
        /**< Authentication mode of the AP profile. */
    qapi_WLAN_Crypt_Type_e     encryption_Type;
        /**< Encryption type of the AP profile. */
} qapi_WLAN_Preferred_Network_Profile_t;

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
typedef struct //qapi_WLAN_Promiscuous_Mode_Info_s
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
typedef enum
{
    QAPI_WLAN_RAW_MODE_HDR_TYPE_BEACON_E         = 0, 
          /**< Raw mode frame header is of type beacon. */
	QAPI_WLAN_RAW_MODE_HDR_TYPE_PROBE_REQ_DATA_E = 1, 
		  /**< Raw mode frame header is of type probe request. */
    QAPI_WLAN_RAW_MODE_HDR_TYPE_QOS_DATA_E       = 2, 
          /**< Raw mode frame header is of type QOS data. */
    QAPI_WLAN_RAW_MODE_HDR_TYPE_FOUR_ADDR_DATA_E = 3 , 
          /**< Raw mode frame header is of type 4 address data. */
	QAPI_WLAN_RAW_MODE_HDR_TYPE_USER_DEFINED_E 	 = 0xff  
          /**< Raw mode frame header is of type self-defined. */
} qapi_WLAN_Raw_Mode_Header_Type_e;

/**
@ingroup qapi_wlan
Data structure that the application is to pass when invoking qapi_WLAN_Raw_Send() 
to transmit a raw frame.
*/
typedef struct //qapi_WLAN_Raw_Send_Params_s
{
    uint8_t                           rate_Index;  
          /**< 0: 1 Mbps, 1: 2 Mbps, 2: 5.5 Mbps, etc. */
		  /**< Note that this value will not take effect if connected  */
    uint8_t                           num_Tries; 
          /**< Packet transmission count: 1 to 14. */
    uint32_t                          payload_Size; /**< Payload size: 0 to 1400. */
    uint32_t                          channel;  
          /**< Channel; 0 to 11. 0: Send on the current channel. */
		  /**< Note that this value will not take effect if connected  */
    qapi_WLAN_Raw_Mode_Header_Type_e  header_Type; 
          /**< 0: Beacon frame, 1: Probe Request frame, 2: QoS data frame, 3: Four addresses data frame, 0xff: Self Defined frame*/
		  /**< If use Self Defined Header, the frame will be considered as an MGMT frame.>*/
    uint16_t                          seq;  
          /**< Sequence number to be filled in the 802.11 header. */
    uint8_t                           addr1[__QAPI_WLAN_MAC_LEN];  /**< Address 1. */
    uint8_t                           addr2[__QAPI_WLAN_MAC_LEN];  /**< Address 2. */
    uint8_t                           addr3[__QAPI_WLAN_MAC_LEN];  /**< Address 3. */
    uint8_t                           addr4[__QAPI_WLAN_MAC_LEN];  /**< Address 4. */
		  /**< Note that address will not take effect when using self-defined frame  */
    uint32_t                          data_Length;   
          /**< Size of the data to be transmitted. */
    uint8_t                           *data;   /**< Data. */
} qapi_WLAN_Raw_Send_Params_t;

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
Enumeration that provides various EAPOL key type for qapi_WLAN_RxEapolKey_Cb_Info_t.
*/
typedef enum {
    QAPI_EAPOL_KEY_TYPE_WPA2 = 0x02,    /**< WPA2 key type. */
    QAPI_EAPOL_KEY_TYPE_WPA  = 0xFE,    /**< WPA key type. */
} qapi_EAPOL_KEY_TYPE_e;

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
Data structure that presents SAE finite cyclic groups configured by application.
*/
typedef struct
{
    uint32_t                *sae_groups;  /**< SAE groups. */
    uint32_t                num;          /**< The number of configured SAE groups. */
} qapi_WLAN_Set_Finite_Cyclic_Groups_Params_t;

/**
@ingroup qapi_wlan
Data structure that enables/disables Pairwise Master Key (PMK) cache function for SAE.
*/
typedef struct
{
    uint32_t op;  /**< Cache operation. */
    //uint32_t number; //current not supported
} qapi_WLAN_SAE_PMK_Cache_t;

/**
@ingroup qapi_wlan
Enumeration that identifies the action to be taken after a WPS initial request succeeds.
*/
typedef enum
{
    QAPI_WLAN_WPS_NO_ACTION_POST_CONNECT_E        = 0, /**< No action to be taken after WPS initiation succeeds. */
    QAPI_WLAN_WPS_CONNECT_REGISTRAR_IN_ENROLLEE_E = 1  /**< Set up a connection after WPS initiation succeeds. */
} qapi_WLAN_WPS_Connect_Action_e;

/**
@ingroup qapi_wlan
Enumeration of supported WPS modes.
*/
typedef enum
{
    QAPI_WLAN_WPS_PIN_MODE_E = 0, /**< WPS Pushbutton method. */
    QAPI_WLAN_WPS_PBC_MODE_E = 1  /**< WPS PIN method. */
} qapi_WLAN_WPS_Mode_e;

/**
@ingroup qapi_wlan
Data structure to pass WPS credentials from the driver to the application.
*/
typedef struct //qapi_WLAN_WPS_Credentials_s
{
    uint16_t                           ap_Channel;  /**< AP's channel. */
    uint8_t                            ssid[__QAPI_WLAN_MAX_SSID_LEN];  /**< SSID. */
    uint8_t                            ssid_Length;   /**< SSID length. */
    qapi_WLAN_Auth_Mode_e              auth_Mode;    /**< Authentication mode. */
    qapi_WLAN_Crypt_Type_e             encryption_Type;   /**< Encryption type. */
    uint8_t                            key_Index; /**< Index of the key. */
    uint8_t                            key[__QAPI_WLAN_WPS_MAX_KEY_LEN+1];  /**< Key. */
    uint8_t                            key_Length; /**< Key length. */
    uint8_t                            mac_Addr[__QAPI_WLAN_MAC_LEN];   /**< MAC address. */
} qapi_WLAN_WPS_Credentials_t;

/**
@ingroup qapi_wlan
Enumeration that identifies the types of WPS command results.
*/
typedef enum
{
    QAPI_WLAN_WPS_STATUS_SUCCESS = 0x0,      /**< WPS succeeded. */
    QAPI_WLAN_WPS_STATUS_FAILURE = 0x1,      /**< WPS failed. */
    QAPI_WLAN_WPS_STATUS_IDLE = 0x2,         /**< WPS in the Idle state. */
    QAPI_WLAN_WPS_STATUS_IN_PROGRESS  = 0x3  /**< WPS in progress. */
} qapi_WLAN_WPS_Status_Code_e;

/**
@ingroup qapi_wlan
Data structure to get WPS command results from the driver.
*/
typedef struct //qapi_WLAN_WPS_Cb_Info_s
{
  uint8_t status;                               /**< WPS status. */
  uint8_t error;                                /**< WPS error. */
  qapi_WLAN_WPS_Credentials_t  credential;      /**< WPS credentials. */
  uint8_t  peer_dev_addr[__QAPI_WLAN_MAC_LEN];  /**< MAC address of a peer device. */
} qapi_WLAN_WPS_Cb_Info_t;

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
typedef struct //qapi_WLAN_Aggregation_Params_s
{
    uint16_t                       tx_TID_Mask; /**< Bitmask to enable Tx A-MPDU aggregation. */
    uint16_t                       rx_TID_Mask; /**< Bitmask to enable Rx A-MPDU aggregation. */
} qapi_WLAN_Aggregation_Params_t;

/**
@ingroup qapi_wlan
Enumeration that identifies results of WPS pushbutton/WPS PIN operations.
*/
typedef enum
{
    QAPI_WLAN_WPS_ERROR_SUCCESS_E = 0x0,          /**< WPS initiation succeeded. */
    QAPI_WLAN_WPS_ERROR_INVALID_START_INFO_E,     /**< Invalid information given to initialize WPS. */
    QAPI_WLAN_WPS_ERROR_MULTIPLE_PBC_SESSIONS_E,  /**< Multiple WPS pushbutton sessions. */
    QAPI_WLAN_WPS_ERROR_WALKTIMER_TIMEOUT_E,      /**< WPS walktimer expired. */
    QAPI_WLAN_WPS_ERROR_M2D_RCVD_E,               /**< M2D message received. */
    QAPI_WLAN_WPS_ERROR_PWD_AUTH_FAIL_E,          /**< Authentication failed. */
    QAPI_WLAN_WPS_ERROR_CANCELLED_E,              /**< WPS cancelled. */
    QAPI_WLAN_WPS_ERROR_INVALID_PIN_E             /**< Incorrect WPS PIN. */
} qapi_WLAN_WPS_Error_Code_e;

/**
@ingroup qapi_wlan
Data structure to store results of a scan.
*/
typedef struct //qapi_WLAN_Scan_List_s
{
    uint32_t num_Scan_Entries;   /**< Number of scan results. */
    void* scan_List;             /**< Scan results. */
} qapi_WLAN_Scan_List_t;

/**
@ingroup qapi_wlan
Data structure to store the preferred 5G parameter.
*/
typedef struct
{
    uint8_t is_prefer_5g;  /**< Enables preferred 5G function. */
    int8_t rssi_hi_5g;     /**< RSSI value base in high region. */
    int8_t rssi_mid_5g;    /**< RSSI value base in middle region. */
    int8_t rssi_lo_5g;     /**< RSSI value base in low region. */
    int8_t pref_hi_5g;     /**< Preferred value added in RSSI high region. */
    int8_t pref_mid_5g;    /**< Preferred value added in RSSI middle region. */
    int8_t pref_lo_5g;     /**< Preferred value added in RSSI low region. */
} qapi_WLAN_5G_Prefer_t;
/**
@ingroup qapi_wlan
Data structure to store connection policy.
*/

typedef struct
{
    uint8_t best_signal_connection;   /**< Enables (1) or disables (0) the best signal connection feature. */
    uint8_t rssi_threshold;           /**< Connection RSSI threshold value. */
    uint8_t is_wpax_auth_ignore;      /**< Sets connection without security type. */
    qapi_WLAN_5G_Prefer_t prefer_5g;  /**< Configures preferred 5G function settings */
    uint16_t reserved;                /**< Reserved for future use. */
} qapi_WLAN_Connect_Policy_t;

/**
@ingroup qapi_wlan
Stores the set broadcast and multicast.
*/
typedef struct
{
    uint8_t enable;    /**< Enables (1) or disables (0) broadcast/multicast Rx filter. */
    uint8_t reserved;  /**< Reserved for future use. */
} qapi_WLAN_Set_BMcast_Filter_t;

/**
@ingroup qapi_wlan
Data structure to get the Tx pipe status.
*/
typedef struct //qapi_WLAN_Tx_Status_s
{
    uint16_t status; /**< One of QAPI_WLAN_TX_STATUS_* types. */
} qapi_WLAN_Tx_Status_t;

/**
@ingroup qapi_wlan
Enum that identifies the results of the command to set the MAC address.
*/
typedef enum
{
    QAPI_WLAN_SET_MAC_RESULT_SUCCESS_E = 1,
    /**< Successfully (re)programmed the MAC address into the Wi-Fi device. */

    QAPI_WLAN_SET_MAC_RESULT_DEV_DENIED_E,
    /**<
    Device denied the operation for several possible reasons, the most
    of which is that the MAC address equals the
    current MAC address that is already in the device. An invalid
    MAC address value can also cause this result.
    */

    QAPI_WLAN_SET_MAC_RESULT_DEV_FAILED_E,
    /**<
    Device tried but failed to program the MAC address. An error occurred
    on the device as it tried to program the MAC address.
    */

    QAPI_WLAN_SET_MAC_RESULT_DRIVER_FAILED_E
    /**<
    Driver tried but failed to program the MAC address. Possibly, the driver
    did not have the proper code compiled to perform this operation.
    */
} qapi_WLAN_Mac_Result_e;

/**
@ingroup qapi_wlan
Data structure to store data for setting the MAC address command.
*/
typedef struct //qapi_WLAN_Program_Mac_Addr_Param_s
{
    uint8_t addr[6]; /**< MAC address. */
    uint8_t result;  /**< Result of the command. */
} qapi_WLAN_Program_Mac_Addr_Param_t;

/**
@ingroup qapi_wlan
Enumeration to configure the debug port for the debug logs in the firmware.

The target can send the captured debug logs either on its UART (local) or
to the host, which in turn sends on its configured debug UART when connected to a PC,
where an external tool (QDL) that is running can capture and parse the dbglogs.
*/
typedef enum
{
    QAPI_WLAN_DBGLOG_LOCAL_PORT_E = 0,
       /**<
           Used when logs are needed on the target (Kingfisher) UART.
       */
    QAPI_WLAN_DBGLOG_REMOTE_PORT_E = 1,
       /**<
           Used when logs are needed on a host that is connected to a PC running
           an external tool (QDL).
       */
} qapi_WLAN_Dbglog_Port_e;

/**
@ingroup qapi_wlan
Data structure to enable/disable logging in the target.

This data structure enables the DBGLOG feature and configures it with the default configuration
for dbglogs. The default dbglog configuration enables the
loglevel ERROR logs for all modules, configures a remote debug port,
enables reporting, sets the report size to 0 (send to host only when the buffer is full),
and sets the time resolution to zero.
*/
typedef struct
{
    qapi_WLAN_Enable_e enable; /**< 1:Enable, 0:Disable */
} qapi_WLAN_Dbglog_Enable_t;


/**
@ingroup qapi_wlan
Data structure to modify the dbglog log level configuration for one or
more modules in the target to capture the logs accordingly.

There are a total of 64 modules in the target to which the debug logs belong.
Specify the module ID mask for which the log level is to be configured/modified.
Update the log level buffer with log level mask information at the proper index
and position for each of the modules in the module ID mask specified.

The log level mask is 4 bits in length and is as following: \n
  Bit0 -- INFO;, Bit1 -- LOW; Bit2 -- HIGH; Bit3 -- ERROR.

Each module occupies a nibble space in the log level buffer to store its
log level mask at the index calculated using its module ID.
*/
typedef struct
{
    uint64_t    module_Id_Mask;
        /**<
            Module ID mask to configure/modify the log level for
            intended modules.
        */
    uint32_t    log_Level[__QAPI_WLAN_DBGLOG_LOGLEVEL_INFO_LEN];
        /**<
             Contains the log level information for the modules in the module ID mask
             specified with the intended log level for each module.
        */
} qapi_WLAN_Dbglog_Module_Config_t;

/**
As part of misc_Config, start bit offset of flush, to be used to flush the dbglogs in current buffer to the host.
*/
#define __QAPI_WLAN_DBGLOG_MISC_CONFIG_FLUSH_FLAG_BIT_OFFSET    0

/**
As part of misc_Config, range mask of flush, to be used to flush the dbglogs in current buffer to the host.
*/
#define __QAPI_WLAN_DBGLOG_MISC_CONFIG_FLUSH_FLAG_MASK              0x01

/**
@ingroup qapi_wlan
Data structure to modify the dbglog configuration parameters in the target
to capture the logs accordingly.
*/
typedef struct
{
    qapi_WLAN_Dbglog_Port_e    debug_Port;
        /**<
            Used to configure the debug port on which the dbglogs is to
            be sent in the target.

            The target can send the captured debug logs either on its UART (local) or
            to the host, which in turn sends the logs on its configured debug UART when connected to a PC
            that has an external tool (QDL) running, which can capture and parse the dbglogs.
        */
    qapi_WLAN_Enable_e	       reporting_Enable;
        /**<
          Used to control the reporting of debug logs to the host or not when
          the debug port is configured as a remote UART.

          When the remote debug port is enabled, the host can configure the target to
          report the logs to the host or not. If reporting is enabled, the target can
          send logs to host when the buffer is full or when the reporting size is met,
          otherwise the logs keep getting overwritten in the target dbglog buffer with
          the new logs until reporting is enabled by the host.
        */
    uint32_t                   report_Trigger_Size_In_Bytes;
        /**<
          Used to configure number of debug messages after which the firmware
          sends the dbglogs to the host. By default, the firmware waits
          until the buffer is full.

          When the remote debug port is enabled, by default, the logs are sent to the
          host when the dbglog buffer in the target becomes full. The host can configure
          the report size for the number of dbglog messages the target
          can send logs to the host. The minimun number of messages expected is
          atleast 15 to avoid frequent events to the host.
        */
    uint32_t                   misc_Config;
        /**<
          Miscellaneous configuration for wlan dbglog.
        */
} qapi_WLAN_Dbglog_Config_t;

/** @cond EXPORT_PKTLOG */
/**
@ingroup qapi_wlan
Data structure to enable/disable pktlog in the target.
*/
typedef struct //qapi_WLAN_Pktlog_Enable_s
{
    qapi_WLAN_Enable_e         enable;
            /**< Used to enable/disable packet logs. */
    uint8_t                    num_Of_Buffers;
            /**<
                Used to specify the number of buffers to be allocated in the target
                for packet logging. The default is 4 and the maximum value is 10.
            */
}qapi_WLAN_Pktlog_Enable_t;

/**
@ingroup qapi_wlan
Data structure to start packet logging for selected events with
selected options in the target.

Packet logging is time-based for the options throughput and PER. The trigger interval and
trigger threshold must be set for this. For the options throughput and PER,
packet logging stops either when a trigger threshold is reached or when a trigger interval
has elapsed.
*/
typedef struct
{
    uint8_t 	event_List;
            /**<
              Event mask used to configure the events for which packet logging is
              to be captured in the firmware.

              There are different events for which packet logs can be captured as required.
              Packets for RX, TX, Rate Ctrl find, and Rate Ctrl update events can be captured.
              Event masks must be configured for required events as follows: \n
              Bit0 -- RX; Bit1 -- TX; Bit2 -- RC find; Bit3 -- RC update.
              */
    uint8_t 	log_Options;
            /**<
              Options mask to configure the options for which packet logging is
              to be captured in the firmware.

              There are different options for which the packet logs can be configured.
              Log options for protocol information, throughput information, and PER information can be configured.
              TCP SACK, PHYERR, and DIAGNOSTICS options are currently not supported.
              The log options mask must be configured for required options as follows: \n
              Bit0 -- Proto; Bit1 -- SACK; Bit2 -- Throughput; Bit3-- PER,
              Bit4 -- PHYERR; Bit5 -- DIAGNOSTIC.
              */
    uint32_t	trigger_Threshold;
            /**<
              The minimum threshold value that must be reached before the packet
              logging can stop for PER/Throughput options.
              */
    uint32_t	trigger_Interval;
            /**<
              The time for which packet logging captures the packets for
              PER/Throughput options.

              This is the minimum interval for which the pktlog must do capturing before
              it stops if the threshold set is not met.
            */
    uint32_t	trigger_Tail_Count;
            /**<
              The number of more packets to be captured even after the
              threshold has been reached for context information for the SACK option,
              which is currently not supported.
            */
    uint32_t	buffer_Size;		/**< Buffer size; currently not used. */
}qapi_WLAN_Pktlog_Start_Params_t;
/** @endcond */


/** @cond */
/**
@ingroup qapi_wlan
Data structure to pass command data from the application to the driver for getting driver register information.
*/
typedef struct
{
    uint32_t address;     /**< Memory address. */
    uint32_t value;       /**< Value. */
    uint32_t mask;        /**< Mask. */
    uint32_t size;        /**< Size. */
    uint32_t operation;   /**< Operation. */
} qapi_WLAN_Driver_RegQuery_Params_t;

/**
@ingroup qapi_wlan
Enumeration that identifies driver register operations.
*/
typedef enum
{
    QAPI_WLAN_REG_OP_READ = 1, /**< Read. */
    QAPI_WLAN_REG_OP_WRITE,    /**< Write. */
    QAPI_WLAN_REG_OP_RMW       /**< Read-Modify-Write. */
} qapi_WLAN_Driver_RegQuery_Params_e;
/** @endcond */

/**
@ingroup qapi_wlan
Data structure used to pass WPS SSID information from the application to the driver.
*/
typedef struct //qapi_WPS_Scan_List_Entry_s
{
    uint8_t ssid[__QAPI_WLAN_MAX_SSID_LEN];   /**< SSID. */
    uint8_t macaddress[__QAPI_WLAN_MAC_LEN];  /**< MAC address. */
    uint16_t channel;                         /**< Wireless channel. */
    uint8_t ssid_Len;                         /**< SSID length. */
} qapi_WPS_Scan_List_Entry_t;

/**
@ingroup qapi_wlan
Data structure to pass WPS command parameters from the application to the driver.
*/
typedef struct //qapi_WLAN_WPS_Start_s
{
    qapi_WPS_Scan_List_Entry_t ssid_info;   /**< SSID information. */
    uint8_t wps_Mode;                       /**< WPS pushbutton or WPS PIN. */
    uint8_t timeout_Seconds;                /**< WPS timeout in seconds. */
    uint8_t connect_Flag;                   /**< Connect action (TRUE/FALSE) after intial WPS success. */
    uint8_t pin[__QAPI_WLAN_WPS_PIN_LEN];   /**< PIN. */
    uint8_t pin_Length;                     /**< PIN length. */
} qapi_WLAN_WPS_Start_t;

/**
@ingroup qapi_wlan
Data structure for cipher.
*/
typedef struct //qapi_WLAN_Cipher_s
{
    uint32_t ucipher; /**< Unicast cipher. */
    uint32_t mcipher; /**< Multicast cipher. */
} qapi_WLAN_Cipher_t;

/**
@ingroup qapi_wlan
Data structure for wireless network parameters.
*/
typedef struct //qapi_WLAN_Netparams_s
{
    uint16_t ap_Channel;                       /**< Wireless channel for the AP. */
    int8_t ssid[__QAPI_WLAN_MAX_SSID_LENGTH];  /**< [OUT] Network SSID. */
    int16_t ssid_Len;                          /**< [OUT] Number of valid chars in ssid[]. */
    qapi_WLAN_Cipher_t cipher;                 /**< [OUT] Network cipher type values not defined. */
    uint8_t key_Index;                         /**< [OUT] For WEP only; key index for Tx. */
    union{
        uint8_t wepkey[__QAPI_WLAN_PASSPHRASE_LEN+1];
		/**< WEP key. */

        uint8_t passphrase[__QAPI_WLAN_PASSPHRASE_LEN+1];
		/**< Passphrase. */
    }u;
	/**<
	[OUT] Security key or passphrase.
	*/

    uint8_t sec_Type;                          /**< [OUT] Security type. */
    uint8_t error;                             /**< [OUT] Error code. */
    uint8_t dont_Block;                        /**<
                                               [IN] 1 -- Returns immediately if the operation is not complete \n
                                                    0 -- Blocks until the operation completes @newpagetable
                                               */
} qapi_WLAN_Netparams_t;

/**
@ingroup qapi_wlan
Enumeration that identifies the device mode.
*/
typedef enum
{
    MODE_STATION_E = 0, /**< Station mode. */
    MODE_AP_E,          /**< SoftAP mode supported only on device 0. */
    MODE_ADHOC_E,       /**< Adhoc network; supported only on device 0. */
    MODE_MAXIMUM_E      /**< Maximum value for the device mode. */
} qapi_WLAN_Mode_e;

/**
@ingroup qapi_wlan
Enumeration that identifies the device concurrency mode.
*/
typedef enum
{
    DEV_MODE_STATION_E = 0x01, /**< Station mode */
    DEV_MODE_AP_E = 0x10,          /**< SoftAP mode */
    DEV_MODE_AP_STA_E = 0x11, /**< AP_STA Concurrency */
    DEV_MODE_NO_CONC_E,    /**< Concurrency Off. */
} qapi_WLAN_DEV_Mode_e;

/**
@ingroup qapi_wlan
Data structure for the IPv6 address.
*/
typedef struct
{
    uint8_t ip_Address[__QAPI_WLAN_IPV6_ADDR_LEN];  /**< IPv6 address. */
} qapi_WLAN_IPv6_Addr_t;

/**
@ingroup qapi_wlan
Data structure for ARP offload.
*/
typedef struct
{
    uint8_t           enable;                               /**< Enable/disable. */
    uint8_t           target_IP[__QAPI_WLAN_IPV4_ADDR_LEN]; /**< IPV4 addresses of the local node. */
    uint8_t           target_Mac[__QAPI_WLAN_MAC_LEN];      /**< MAC address for this tuple; if not valid, the local MAC is used. */
}qapi_WLAN_ARP_Offload_Config_t;

/**
@ingroup qapi_wlan
Data structure for neighbor solicitation offload.
*/
typedef struct
{
    uint8_t                   enable;                                      /**< Enable/disable. */
    qapi_WLAN_IPv6_Addr_t     target_IP[__QAPI_WLAN_NSOFF_MAX_TARGET_IPS]; /**< IPV6 WLAN firmware address of the local node. */
    uint8_t                   target_Mac[__QAPI_WLAN_MAC_LEN];             /**< MAC address for this tuple; if not valid, the local MAC is used. */
    qapi_WLAN_IPv6_Addr_t     solicitation_IP;                             /**< Multicast source IP addresses for receiving solicitations. */
} qapi_WLAN_NS_Offload_Config_t;

/**
@ingroup qapi_wlan
Enum declaration for management frame types.
*/
typedef enum {
    QAPI_WLAN_FRAME_BEACON_E = 0,  /**< Beacon frame type. */
    QAPI_WLAN_FRAME_PROBE_REQ_E,   /**< Probe request frame type. */
    QAPI_WLAN_FRAME_PROBE_RESP_E,  /**< Probe response frame type. */
    QAPI_WLAN_FRAME_ASSOC_REQ_E,   /**< Association request frame type */
    QAPI_WLAN_FRAME_ASSOC_RESP_E,  /**< Association response frame type. */
    QAPI_WLAN_NUM_MGMT_FRAME_E     /**< Number of management frame types. */
} qapi_WLAN_Mgmt_Frame_Type_e;

/**
@ingroup qapi_wlan
Data structure to pass application information element data from the application to the driver.
*/
typedef struct //qapi_WLAN_App_Ie_Params_s
{
    uint8_t mgmt_Frame_Type;   /**< Frame in which IE is to be added. */
    uint8_t ie_Len;            /**< IE length. */
    uint8_t *ie_Info;          /**< Application specified IE. */
} qapi_WLAN_App_Ie_Params_t;

/**
@ingroup qapi_wlan
Enum declaration for Tx Power setting policy.
*/
typedef enum  {
    QAPI_WLAN_POLLICY_SECURITY_E = 0,  /**< Security policy. */
	QAPI_WLAN_POLICY_NUM_E			   /**< Number of policy. */
} qapi_WLAN_TX_Power_Policy_e;

/**
@ingroup qapi_wlan
Macro definitions for restoring TX power to default value.
*/
#define QAPI_TX_POWER_RESTORE  100

/**
@ingroup qapi_wlan
Data structure for Tx Power.
*/
typedef struct //qapi_WLAN_Set_Txpower_Params_t
{
    uint8_t txpower;  
    qapi_WLAN_TX_Power_Policy_e policy;
} qapi_WLAN_Set_Txpower_Params_t;


/**
@ingroup qapi_wlan
Data structure for Rx A-MPDU.
*/
typedef struct //qapi_WLAN_Rx_Aggrx_Params_s
{
    uint8_t aggrx_Buffer_Size;                   /**< Buffer size. */
    uint8_t aggrx_Reorder_Buffer_Timeout_In_Ms;  /**< Buffer reorder timeout. */
    uint8_t aggrx_Session_Timeout_Val_In_Ms;     /**< Session timeout. */
    uint8_t aggrx_Reorder_Cfg;                   /**< Reorder configuration. */
    uint8_t aggrx_Session_Timeout_Cfg;           /**< Session timeout. */
    uint8_t reserved0;                           /**< Reserved. */
} qapi_WLAN_Rx_Aggrx_Params_t;

/**
@ingroup qapi_wlan
Data structure to pass 802.11v BSS maximum idle period information from the
application to the driver. Used only in SoftAP mode.
*/
typedef struct //qapi_WLAN_BSS_Max_Idle_Period_s
{
    uint16_t period;                /**< Period in terms of 1000 TUs. */
    uint16_t protected_Keep_Alive;  /**< If protected, keepalives are required. */
} qapi_WLAN_BSS_Max_Idle_Period_t;

/**
@ingroup qapi_wlan
Data structure to pass 802.11v WNM sleep period information from the application to the driver. Used only in Station mode.
*/
typedef struct //qapi_WLAN_WNM_Sleep_Period_s
{
    uint16_t action_Type; /**< Enter: 1, exit: 0. */
    uint16_t duration;    /**< Duration in terms of DTIM intervals. */
} qapi_WLAN_WNM_Sleep_Period_t;

/**
@ingroup qapi_wlan
Data structure to get 802.11v event information from the driver.
*/
typedef struct //qapi_WLAN_WNM_Cb_Info_s
{
    uint16_t cmd_Type;   /**< WNM command type. */
    uint16_t response;   /**< WNM command result. */
} qapi_WLAN_WNM_Cb_Info_t;

/**
@ingroup qapi_wlan
Identifies the disconnection reason.
*/
typedef enum {
    QAPI_WLAN_NO_NETWORK_AVAIL_E              = 0x01, /**< No network available. */
    QAPI_WLAN_LOST_LINK_E                     = 0x02, /**< Missed beacons. */
    QAPI_WLAN_DISCONNECT_CMD_E                = 0x03, /**< User disconnect command. */
    QAPI_WLAN_BSS_DISCONNECTED_E              = 0x04, /**< BSS disconnected. */
    QAPI_WLAN_AUTH_FAILED_E                   = 0x05, /**< Authentication failed. */
    QAPI_WLAN_ASSOC_FAILED_E                  = 0x06, /**< Association failed. */
    QAPI_WLAN_NO_RESOURCES_AVAIL_E            = 0x07, /**< No resources available. */
    QAPI_WLAN_CSERV_DISCONNECT_E              = 0x08, /**< Disconnection due to connection services. */
    QAPI_WLAN_INVALID_PROFILE_E               = 0x0a, /**< RSNA failure. */
    QAPI_WLAN_DOT11H_CHANNEL_SWITCH_E         = 0x0b, /**< 802.11h channel switch. */
    QAPI_WLAN_PROFILE_MISMATCH_E              = 0x0c, /**< Profile mismatched. */
    QAPI_WLAN_CONNECTION_EVICTED_E            = 0x0d, /**< Connection evicted. */
    QAPI_WLAN_IBSS_MERGE_E                    = 0x0e, /**< Disconnection due to merging of IBSS. */
    QAPI_WLAN_EXCESS_TX_RETRY_E               = 0x0f, /**< Tx frames failed after excessive retries. */
    QAPI_WLAN_SEC_HS_TO_RECV_M1_E             = 0x10, /**< Unused. Security 4-way handshake timed out waiting for M1. */
    QAPI_WLAN_SEC_HS_TO_RECV_M3_E             = 0x11, /**< Unused. Security 4-way handshake timed out waiting for M3. */
    QAPI_WLAN_TKIP_COUNTERMEASURES_E          = 0x12, /**< TKIP counter-measures. */
    QAPI_WLAN_CCX_TARGET_ROAMING_INDICATION_E = 0xfd, /**< Unused. */
    QAPI_WLAN_CCKM_ROAMING_INDICATION_E       = 0xfe, /**< Unused. */
} qapi_WLAN_Disconnect_Reason_t;

/**
@ingroup qapi_wlan
Data structure to be used in case the application wants to
change Beacon Miss parameters.

On receiving this request from a user in STA mode, the WLAN firmware reports
a link loss after the specified interval. Either one of the values will be
set by the user; bmiss_Time_In_Ms or num_Beacons. Based on the value set, the firmware
detects a beacon miss and triggers a disconnect
in STA mode.
*/
typedef struct //qapi_WLAN_Sta_Config_Bmiss_Config_s
{
    uint16_t     bmiss_Time_In_Ms;
        /**< New beacon miss time (in ms) to be set by application. */
    uint16_t     num_Beacons;
        /**< New number of beacons to be set by the application to detect the number of beacons missed, after which a disconnect is triggered. */
} qapi_WLAN_Sta_Config_Bmiss_Config_t;

/**
@ingroup qapi_wlan
Enumerate antenna switch based on which mode.
*/
typedef enum {
	QAPI_WLAN_ANT_DIV_AUTO_MODE_E					= 0x0, /**Antenna siwtch based on auto mode. In this mode, we switch antenna immediately once it is neccessary to switch antenna. For example, the main rssi is  very low and the alt rssi is strong*/
	QAPI_WLAN_ANT_DIV_PACKET_NUMBER_STRICTLY_E		= 0x1, /** Antenna switch based on packet number strictly. */
	QAPI_WLAN_ANT_DIV_TIME_INTERVAL_STRICTLY_E		= 0x2, /** Antenna switch based on time interval strictly. */
} qapi_WLAN_Ant_Div_Mode_e;

/**
@ingroup qapi_wlan
Enumerate antenna diversity enable mode.
*/
typedef enum {
	QAPI_WLAN_DISABLE_ANT_DIV_E					= 0x0, /** Disable ant div. */
	QAPI_WLAN_ENABLE_HW_ANT_DIV_E				= 0x1, /** enable hw ant div. */
	QAPI_WLAN_ENABLE_SW_ANT_DIV_E				= 0x2, /** enable sw ant div, for future use. */
} qapi_WLAN_Ant_Div_Enable_e;

/**
@ingroup qapi_wlan
Data structure to configure the parameters of HW antenna diversity .
*/
typedef struct //qapi_WLAN_Hw_Ant_Div_Config_s
{
	uint8_t		mode;				/**< Refer to enum #qapi_WLAN_Ant_Div_Mode_e . */
	uint8_t		reserved1;			/**< Reserved for future use . */
	uint16_t 	reserved2;			/**< Reserved for future use . */
	uint32_t	param;				/**< If mode is auto mode, it can be skipped, if mode is packet number strictly,
									it is  packet number and if mode is time interval strictly, it is time interval . */
} qapi_WLAN_Hw_Ant_Div_Config_t;

/**
@ingroup qapi_wlan
Data structure to configure the parameters of software antenna diversity .
*/
typedef struct//qapi_WLAN_Sw_Ant_Div_Config_s
{
	uint16_t	interval;					/**< Time interval for software antenna diversity. */
	uint16_t	reserved;					/**< Reserved for future use. */
	uint32_t	high_traffic_threshold;	    /**< High traffic threshold. */
} qapi_WLAN_Sw_Ant_Div_Config_t;

/**
@ingroup qapi_wlan
Data structure to be used in case the application wants to
configure HW antenna diversity.
*/
typedef struct //qapi_WLAN_Ant_Div_Config_s
{
    uint8_t     enable_Ant_Div;  	/**< Refer to enum #qapi_WLAN_Ant_Div_Enable_e. */
	uint8_t     tx_Follow_Rx;   	/**< 1: tx follows rx, 0: tx does not follow rx. */
  	union
  	{
  		qapi_WLAN_Hw_Ant_Div_Config_t hw_ant_div_config; /**< ConfigureS hardware antenna diversity parameter . */
		qapi_WLAN_Sw_Ant_Div_Config_t sw_ant_div_config; /**< Reserved for future use. */
  	}ant_div_config;
} qapi_WLAN_Ant_Div_Config_t;

/**
@ingroup qapi_wlan
Data structure used to retrieve the current antenna diversity information .
*/
typedef struct //qapi_WLAN_Get_Ant_Div_s
{
	uint8_t					enable_Ant_Div;		 /**< refer to enum qapi_WLAN_Ant_Div_Enable_e. */
	uint8_t					tx_Follow_Rx;	 	 /**< 1: tx antenna follows rx antenna, 0:tx antenna not follows rx antenna. */
	uint8_t					curr_Rx_Ant_2g;	 	/**< Current rx physical antenna in 2G band. */
	uint8_t					curr_Tx_Ant_2g;	 /**< Current tx physical antenna in 2G band. */
	uint8_t					curr_Rx_Ant_5g;	 /**< Current rx physical antenna in 5G band. */
	uint8_t					curr_Tx_Ant_5g;	 /**< Current tx physical antenna in 5G band. */
	uint8_t					avg_Main_Rssi;   	/**< Current average main rssi. */
	uint8_t					avg_Alt_Rssi;    	/**< Current average alternative rssi. */
	uint32_t				total_Pkt_Cnt;   	/**< Packet count used to calculate the sum of rssi. */
	uint32_t				ant_Switch_Cnt;	 	/**< Number of switching antennas. */
}qapi_WLAN_Get_Ant_Div_t;

/**
@ingroup qapi_wlan
Enumerates PMF event type.
*/
typedef enum {
    QAPI_WLAN_PMF_COMEBACK_TIME_E           = 0x1,     /** DUT receives associate response with status code=30 and has comeback time IE. */
    QAPI_WLAN_PMF_SECURITY_MODE_VIOLATION_E = 0x2,     /** PMF and security mode is violated. */
    QAPI_WLAN_PMF_POLICY_VIOLATION_E        = 0x4,     /** PMF mode on STA and AP is violated. */
} qapi_WLAN_PMF_EVENT_TYPE_e;

/**
@ingroup qapi_wlan
Data structure that receives the event related to WLAN PMF.
*/
typedef struct //qapi_WLAN_Wlan_Pmf_Evt_s
{
    uint8_t                 event_type;      /**< PMF event type that can be set to the value as qapi_WLAN_PMF_EVENT_TYPE_e listed. */
    uint8_t                 reserved[3];     /**< Reserved for alignment. */
    uint32_t                param;          /**< PMF event parameter. */
}qapi_WLAN_Pmf_Evt_t;

/**
@ingroup qapi_wlan
Enumeration PMF capability.
*/
typedef enum {
   QAPI_WLAN_PMF_DISABLE_E = 0,    /**< WLAN PMF is disabled. */
   QAPI_WLAN_PMF_OPTIONAL_E  = 1,  /**< WLAN PMF is optional. */
   QAPI_WLAN_PMF_REQUIRED_E  = 2,  /**< WLAN PMF is required. */
} qapi_WLAN_PMF_CAP_e;

/**
@ingroup qapi_wlan
Enumeration RSN capability.
*/
typedef enum {
   QAPI_WLAN_RSN_MFPR_E = 0x40,     /**< MFPR in RSN IE capability field. */
   QAPI_WLAN_RSN_MFPC_E = 0x80,     /**< MFPC in RSN IE capability field. */
} qapi_WLAN_RSN_CAP_e;

/**
@ingroup qapi_wlan_wpa3
Identifies the enable/disable options for WLAN wpa3.
*/
typedef enum
{
    QAPI_WLAN_WPA3_DISABLE_E  = 0, /**< Disables WLAN wpa3. */
    QAPI_WLAN_WPA3_ENABLE_E   = 1  /**< Enables the WLAN wpa3. */
} qapi_WLAN_Wpa3_Enable_e;

/**
@ingroup qapi_wlan_mgmt_frame_type
Identifies the management frame type.
*/
typedef enum
{
    QAPI_WLAN_MGMT_NONE_E  = 0x0, /**< None. */
    QAPI_WLAN_MGMT_ASSOC_RESP_E  = 0x1, /**< Association response. */
    QAPI_WLAN_MGMT_PROBE_RESP_E  = 0x2,  /**< Probe response. */
} qapi_WLAN_MGMT_FRAME_e;

/**
@ingroup qapi_wlan
Macro definitions for packet generation.
*/
#define MIN_GEN_PKT_INTERVAL 100
#define MAX_GEN_PKT_LEN 1496
#define GEN_PKT_IDX_INVALID 0xFF

/**
@ingroup qapi_wlan
Data structure that the parameters are needed when invoking qapi_WLAN_Gen_Pkt()
to generate packet.
*/
typedef struct //qapi_WLAN_Gen_Pkt_Params_s
{
    uint8_t peer_mac_addr[__QAPI_WLAN_MAC_LEN];
	/**< Destination address. */

    uint16_t type;
	/**< Type of data frame. */

    uint32_t counter;
	/**< Counts the number of packets that will be sent (0 to infinity). */

    uint32_t interval;
	/**< Sends packets periodically (in milliseconds @le >= 100 ms). */

    uint32_t data_len;
	/**< Payload length, @ge >= 1496 btyes. */

    uint8_t   *data;
	/**< Payload. */
} qapi_WLAN_Gen_Pkt_Params_t;

#define QAPI_MAX_REG_RULES 17
/**
@ingroup qapi_wlan
Data structure that the application uses to interpret the Regulatory for
all regulatory information which device support.

All the information in this structure is for one Regulatory which device support
*/
typedef struct //qapi_WLAN_Reg_s
{
    uint16_t    start_freq;		/**< start frequency. */
    uint16_t    end_freq;		/**< end frequency. */
    uint8_t     reg_power;		/**< regulatory power. */
    uint8_t     ant_gain;		/**< antenna gain. */
	uint16_t    flag_info;		/**< flag information. */
    uint16_t     max_bw;
} qapi_WLAN_Reg_t;

typedef struct {
    uint8_t alpha[3];
    uint8_t num_2g_reg_rules;
    uint8_t num_5g_reg_rules;
    qapi_WLAN_Reg_t reg_rules[QAPI_MAX_REG_RULES];
} qapi_WLAN_Reg_Evt_t;

typedef struct {
    uint8_t reg_power;
    uint8_t ctl_power;
    uint16_t target_power;
    uint16_t real_power;
} qapi_WLAN_Get_Power_Evt_t;

//set_rate
typedef struct {
    uint8_t ra_ON;
    uint8_t rate_staid;
    uint8_t rate_p_rate ;
    uint8_t rate_s_rate ;
    uint8_t rate_t_rate ;
}qapi_WLAN_Set_Rate_Params_t;

/**
@ingroup qapi_wlan
Set STA Listen interval.
*/
typedef struct {
    uint32_t time;
    uint32_t round_type;
}qapi_WLAN_Listen_Interval_Params_t;

/**
@ingroup qapi_wlan
Set STA edca param, including aifsn/cw_min/cw_max/txoplimit.
*/
typedef struct {
    uint8_t qid;
    uint8_t aifsn;
    uint16_t cw_min;
    uint16_t cw_max;
    uint16_t txop_limit;
}qapi_WLAN_Edca_Params_t;

/**
@ingroup qapi_wlan
Set STA BA window size.
*/
typedef struct {
    uint16_t ack_timeout;
    uint16_t delay;
}qapi_WLAN_BA_Window_Params_t;

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
typedef void (* qapi_WLAN_Callback_t)(uint8_t device_ID,
                                           uint32_t cb_ID,
                                           void *application_Context,
                                           void  *payload,
                                           uint32_t payload_Length);

qapi_Status_t qapi_WLAN_Enabled (qapi_WLAN_Enable_e *enable);
qapi_Status_t qapi_WLAN_Error (void);

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
qapi_Status_t qapi_WLAN_Enable (qapi_WLAN_Enable_e enable);


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
qapi_Status_t qapi_WLAN_Add_Device (uint8_t device_ID);


/**
@ingroup qapi_wlan
Initiates a wireless scan to fnid nearby access points.

Users can configure the scan type as they want. Additional scan parameters can be tuned by using qapi_WLAN_Set_Params() with
GroupID for Wireless and ParamID scan_params. Scaned results will be notified by QAPI_WLAN_SCAN_COMPLETE_CB_E

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
qapi_Status_t qapi_WLAN_Start_Scan (uint8_t device_ID,
                                                 const qapi_WLAN_Start_Scan_Params_t  *scan_Params);

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
qapi_Status_t qapi_WLAN_Get_Scan_Results (uint8_t device_ID, qapi_WLAN_Scan_Comp_Evt_t *scan_Res, int16_t *num_Bss);

/**
@ingroup qapi_wlan
This API is part of connect/disconnect process in both non-AP Station and SoftAP modes.
It uses previously set SSID and security configurations.

In non-AP Station mode, it allows the station to connect to or disconnect from an associated AP.
In SoftAP mode, it starts the device as a SoftAP.

This API is already called from qapi_WLAN_Disconnect() and qapi_WLAN_WPS_Connect(), so it does not need to be called explicitly for these
operations; however for connect operations (except for WPS), it will need to be called explicitly.

@param[in] device_ID           Device ID.

@return
QAPI_OK -- Commit operation succeeded. \n
Nonzero value -- Commit operation failed.

@dependencies
SSID and security configurations must be set using qapi_WLAN_Set_Param() before calling this API.\n
If SSID is set to a nonempty string before calling this API, it is considered as a connect request, and if the SSID is set to
an empty string, it is considered as a disconnect request in both non-AP Station and SoftAP modes.\n
If the user wants to use Tx/Rx aggregation, __QAPI_WLAN_PARAM_GROUP_WIRELESS_ALLOW_TX_RX_AGGR_SET_TID must be set using qapi_WLAN_Set_Param()
before calling this API.\n
Similarly, if the user wants to use UAPSD, __QAPI_WLAN_PARAM_GROUP_WIRELESS_AP_ENABLE_UAPSD (in SoftAP mode) and
__QAPI_WLAN_PARAM_GROUP_WIRELESS_STA_UAPSD in non-AP Station mode should be set using qapi_WLAN_Set_Param().
*/
qapi_Status_t qapi_WLAN_Commit (uint8_t device_ID);

/**
@ingroup qapi_wlan
Sets a callback function and application context (const void *), if any, for WLAN control commands.

Users can use the callback function to handle events received from the WLAN firmware for asynchronous commands.
The application context, while setting the callback, will be returned when the callback handler function is called for events.
The callback handling is done in the Wi-Fi driver thread's context, hence the event handling should be as fast as possible to avoid
performance delays.

@datatypes
#qapi_WLAN_Callback_t

@param[in] callback               Callback function. NULL to clear callback, non-NULL to initialize callback, not support replace.
@param[in] application_Context    Application context set by the application.

@return
QAPI_OK -- Application callback handler function for WLAN commands set. \n
Nonzero value -- Setting callback function failed.

@dependencies
The callback function to be set must be a valid function.
*/
qapi_Status_t qapi_WLAN_Set_Callback (qapi_WLAN_Callback_t callback, void *application_Context);

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
This API internally calls qapi_WLAN_Commit(), hence no explicit call is needed in the disconnect process when using this API.

@param[in] device_ID         Device ID.

@return
QAPI_OK -- Disconnect process succeeded. \n
Nonzero value -- Disconnection failed.

@dependencies
None.
*/
qapi_Status_t qapi_WLAN_Disconnect (uint8_t device_ID);


/**
@ingroup qapi_wlan
Sets requested parameters in the WLAN firmware.

The group_ID parameter is used to determine to which group the the command belongs, and the param_ID is used to provide the
requested command (/parameter).

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
qapi_Status_t qapi_WLAN_Set_Param (uint8_t device_ID,
                                               uint16_t group_ID,
                                               uint16_t param_ID,
                                               const void  *data,
                                               uint32_t length,
                                               qapi_WLAN_Wait_For_Status_e wait_For_Status);


/**
@ingroup qapi_wlan
Retrieves the requested parameter value from the WLAN firmware.

The group_ID parameter is used to determine to which group the the command belongs, and the param_ID is used to provide the
requested command (/parameter).

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
qapi_Status_t qapi_WLAN_Get_Param (uint8_t device_ID,
                                               uint16_t group_ID,
                                               uint16_t param_ID,
                                               void *data,
                                               uint32_t *length);


/**
@ingroup qapi_wlan
Enum values for the FTM mode command type.
*/
typedef enum {
    QAPI_WLAN_FTM_SEND_REQ=0, /**< Send a packet request. */
    QAPI_WLAN_FTM_LOAD_BD /**< Load board data. */
} qapi_WLAN_FTM_CMD_e;


/**
@ingroup qapi_wlan
Enables or disables 802.11d feature.

If enabled, the country code of the device will be set according to scan results.

@param[in] enable		- 0: Disable 802.11d.
						- 1: Enable 802.11d.

@return
QAPI_OK -- Command send success.\n
Nonzero value -- Command failed.

@dependencies
The device works in STA mode, or has not connected with AP.
*/
qapi_Status_t qapi_WLAN_Set_11d(uint32_t enable);
/**
@ingroup qapi_wlan
Determines whether 802.11d feature is enabled or not.

@param[out] result		Result retrieved from WLAN firmware.\n
					    - 0: 802.11d disabled.
						- 1: 802.11d enabled.

@return
QAPI_OK -- Result was retrieved from WLAN firmware. \n
Nonzero value -- Result retrieval failed.

@dependencies
None.
*/
qapi_Status_t qapi_WLAN_Get_11d(uint32_t *result);
/**
@ingroup qapi_wlan
Gets country code of the device.

@param[out] country_code	Country code retrieved from the WLAN firmware.

@return
QAPI_OK -- Country code was retrieved from WLAN firmware. \n
Nonzero value -- Country code retrieval failed.

@dependencies
None.
*/
qapi_Status_t qapi_WLAN_Get_Country_Code(char *country_code);

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
qapi_Status_t qapi_WLAN_Set_Rate (qapi_WLAN_Set_Rate_Params_t *prate_para);

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
qapi_Status_t qapi_WLAN_Get_Rate (qapi_WLAN_Set_Rate_Params_t *prate_para);

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
@param[in]  mgmt_filter      WLAN management frames filter. Now only support association response and probe response frames.

@return qapi_Status_t        QAPI_OK on success, other error code on failure.
*/
qapi_Status_t qapi_WLAN_Enable_Mgmt_Filter (uint8_t device_ID, uint32_t mgmt_filter);

/**
@brief  API to be used to disable wlan management frame filter
@param[in]  device_id        Device ID.

@return qapi_Status_t        QAPI_OK on success, other error code on failure.
*/
qapi_Status_t qapi_WLAN_Disable_Mgmt_Filter (uint8_t device_ID);

/**
@brief  API to be used to recieve wlan management frames which are in filter.
@param[out]  buffer          Pointer to output buffer for management frames .
@param[in]   buffer_len      Buffer lenght in bytes.
@param[out]  frame_len       lenght of management frames.
@param[in]   timeout         timeout in millisecond when receive management frames .

@return qapi_Status_t    QAPI_OK on success, other error code on failure.
*/
qapi_Status_t qapi_WLAN_Recv_Mgmt_Frames (uint8_t *buffer, uint32_t buffer_len, uint32_t *frame_len, uint32_t timeout);

_STRUCT_4BYTE_ALLIGN_CHECK(qapi_WLAN_Evt_Hdr_t)
_STRUCT_4BYTE_ALLIGN_CHECK(qapi_WLAN_Enable_Evt_t)
_STRUCT_4BYTE_ALLIGN_CHECK(qapi_WLAN_Disable_Evt_t)
_STRUCT_4BYTE_ALLIGN_CHECK(qapi_WLAN_If_Add_Comp_Evt_t)
_STRUCT_4BYTE_ALLIGN_CHECK(qapi_WLAN_Scan_Start_Evt_t)
_STRUCT_4BYTE_ALLIGN_CHECK(qapi_WLAN_BSS_Scan_Info_t)
_STRUCT_4BYTE_ALLIGN_CHECK(qapi_WLAN_Scan_Comp_Evt_t)
_STRUCT_4BYTE_ALLIGN_CHECK(qapi_WLAN_Join_Comp_Evt_t)

#endif // __QAPI_WLAN_BASE_H__

