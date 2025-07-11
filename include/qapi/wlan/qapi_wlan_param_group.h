/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/
// $QTI_LICENSE_QDN_C$

#ifndef __QAPI_WLAN_PARAM_GROUP_H__
#define __QAPI_WLAN_PARAM_GROUP_H__

/**
 * @file qapi_wlan_param_group.h
 *
 * @brief WLAN param group definitions
 *
 * @details This section provides APIs, macros definitions, enumerations and data structures
 *          for applications to perform WLAN control operations.
 */

#include "qapi_types.h"
#include "qapi_status.h"

/**
Macro that indicates the group ID that can be used to configure system parameters of
the WLAN subsystem.
*/
#define __QAPI_WLAN_PARAM_GROUP_SYSTEM                            0

/**
Macro that indicates the group ID that can be used to configure wireless parameters of
the WLAN subsystem.
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS                          1

/**
Macro that indicates the group ID that can be used to configure security parameters of
the WLAN subsystem.
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_SECURITY                 2

/**
Macro that indicates the group ID that can be used to configure various
P2P (Peer-to-Peer/Wi-Fi Direct) parameters.
*/
#define __QAPI_WLAN_PARAM_GROUP_P2P                               3

/**
An application can enable/disable suspend/resume operations of a WLAN subsystem by
setting this parameter as TRUE (to enable) or FALSE (to disable).

This setting just enables suspend/resume and does not actually put the WLAN
subsystem in suspend mode.

The application should use qapi_WLAN_Suspend_Start() to put the WLAN subsystem in
Suspend mode.

By default, this feature is enabled by the WLAN driver.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().

@sa
qapi_WLAN_Suspend_Start
*/
#define __QAPI_WLAN_PARAM_GROUP_SYSTEM_ENABLE_SUSPEND_RESUME      0

/**
Used to enable/disable the target (Kingfisher) debug logging.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().
*/
#define __QAPI_WLAN_PARAM_GROUP_SYSTEM_DBGLOG_ENABLE              1
/**
Used to configure dbglog configuration, such as debug port, report enable, report size, etc.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().
*/
#define __QAPI_WLAN_PARAM_GROUP_SYSTEM_DBGLOG_CONFIG              2
/**
Used to configure dbglog module loglevel configuration for a specific module or all modules.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().
*/
#define __QAPI_WLAN_PARAM_GROUP_SYSTEM_DBGLOG_MODULE_CONFIG       3

/** @cond EXPORT_PKTLOG */
/**
Used to enable/disable the infrastructure required for pktlog.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().
*/
#define __QAPI_WLAN_PARAM_GROUP_SYSTEM_PKTLOG_ENABLE              4
/**
Used to start pktlogs with the desired events and options.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().
*/
#define __QAPI_WLAN_PARAM_GROUP_SYSTEM_PKTLOG_START               5

/** @endcond */

/**
Command ID to obtain the WLAN firmware version by querying the driver.

@note1hang This parameter can only be used with qapi_WLAN_Get_Param().

@param[out] uint32   Variable of type qapi_WLAN_Firmware_Version_String_t,
                     which will be filled by the driver.

@sa
qapi_WLAN_Firmware_Version_String_t
*/
#define __QAPI_WLAN_PARAM_GROUP_SYSTEM_FIRMWARE_VERSION           7

/**
Command ID to control the number of buffers in the WLAN driver
buffer pool.

This command also allows the application to configure the number of
buffers to be reserved for Rx in the driver.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().

@param[in] pool_config_info    Application populates the buffer pool configuration
                               values in qapi_WLAN_A_Netbuf_Pool_Config_t and
                               passes them to the driver.

@sa
qapi_WLAN_A_Netbuf_Pool_Config_t

@sideeffects
Changing this value can have an impact on WLAN performance.
*/
#define __QAPI_WLAN_PARAM_GROUP_SYSTEM_DRIVER_NETBUF_POOL_SIZE    8

/**
Used to get the last system error.

@note1hang This parameter can only be used with qapi_WLAN_Get_Param().
*/
#define __QAPI_WLAN_PARAM_GROUP_SYSTEM_LAST_ERROR                 9

/** @cond EXPORT_DEBUG_APIS */
/**
Used to get driver registration information.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().
*/
#define __QAPI_WLAN_PARAM_GROUP_SYSTEM_DRIVER_REG_QUERY           10
/**
Used to force set the WLAN firmware assert.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().
*/
#define __QAPI_WLAN_PARAM_GROUP_SYSTEM_DRIVER_FORCE_ASSERT        11
/**
Used to give sleep info to the WLAN firmware.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().
*/
#define __QAPI_WLAN_PARAM_GROUP_SYSTEM_DRIVER_SLEEP_INFO        12

/**
Used to bypass 5G cal section data in OTP and BDF data will be used.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().
*/
#define __QAPI_WLAN_PARAM_GROUP_SYSTEM_BYPASS_5GCAL_OTP           13

/** @endcond */

/**
Command ID to set/get the operating mode of a given virtual device in the WLAN subsystem.

Mode-specific parameters should be set only after setting the mode (AP/STA) using
this command.

If a concurrent mode of operation is enabled, the SAP can only be operated in virtual
device 0, and the STA mode of operation can only operate in virtual device 1.

In the case of a single device mode of operation, device 0 can be used for both STA and
SAP mode of operation.

@note1hang This parameter can be used with qapi_WLAN_Set_Param() and qapi_WLAN_Get_Param().

@param[in,out] opMode  Address of the variable type qapi_WLAN_Dev_Mode_e

@sa
qapi_WLAN_Dev_Mode_e
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_OPERATION_MODE                       1

/**
Command ID to set/get the operating wireless channel of a given virtual device in
the WLAN subsystem.

For set/get operations, channel values are set in numbers (channel number 1-14, 36-165)
and not in frequency values.

@note1hang This parameter can be used with qapi_WLAN_Set_Param() and qapi_WLAN_Get_Param().

@param[in,out] uint32_t    Variable that holds the channel number.
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_CHANNEL                              2

/**
Command ID to set scan parameters to the driver.
These parameters should be set before performing a scan operation.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().

@param[in] qapi_WLAN_Scan_Params_t         Address of the variable holding all
                                           customizable scan parameters.

@sa
qapi_WLAN_Scan_Params_t
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_SCAN_PARAMS                          3

/**
Command ID to set/get the transmit power in dBm of a given virtual device.

@note1hang This parameter can be used with qapi_WLAN_Set_Param() and qapi_WLAN_Get_Param().

@param[in,out] uint32_t        Address of the variable that holds the power value.
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_TX_POWER_IN_DBM                      4

/**
Command ID to set/get the SSID of/for a given virtual device in the WLAN subsystem.

@note1hang This parameter can be used with qapi_WLAN_Set_Param() and qapi_WLAN_Get_Param().

@param[in,out] uint8_t[] Unsigned byte array of size __QAPI_WLAN_MAX_SSID_LENGTH.

@sa
__QAPI_WLAN_MAX_SSID_LENGTH
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_SSID                                 5

/**
Command ID to set/get the BSSID of/for a given virtual device in the WLAN subsystem when operation in STA mode.

@note1hang This parameter can be used with qapi_WLAN_Set_Param() and qapi_WLAN_Get_Param().

@param[in,out] uint8_t[] Unsigned byte array of size __QAPI_WLAN_MAC_LEN.

@sa
__QAPI_WLAN_MAC_LEN

*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_BSSID                                 6

/**
Command ID to set/get the wireless PHY mode of/for a given virtual device.

The Set operation for this should be done before establishing a connection.

@note1hang This parameter can be used with qapi_WLAN_Set_Param() and qapi_WLAN_Get_Param().

@param[in,out] qapi_WLAN_Phy_Mode_e    Required PHY mode should be specified.

@sa
qapi_WLAN_Phy_Mode_e
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_PHY_MODE                             7

/**
Command ID to enable/disable forwarding of incoming probe request frames when
operating in Softap mode.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().

@param[in] uint32_t        Set to TRUE to enable forwarding, FALSE to disable.
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_ENABLE_PROBE_REQ_FWD_TO_HOST         8

/**
Command ID to allow/disallow aggregation for Tx and Rx on a TID basis.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().

@param[in] qapi_WLAN_Aggregation_Params_t  Strucuture populated with
                                           required aggregation parameters
                                           for Tx and Rx.

@sa
qapi_WLAN_Aggregation_Params_t
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_ALLOW_TX_RX_AGGR_SET_TID             9

/**
Command ID to enable/disable roaming.
Disabling roaming disables any autonomous scans (like background scans)
performed by the firmware.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().

@param[in] uint32_t        Set to 1 to enable roaming, 3 to disable roaming.
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_ENABLE_ROAMING                      10

/**
Command ID to enable/disable Promiscuous mode, which is only supported on virtual device 0.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().

@param[in] qapi_WLAN_Promiscuous_Mode_Info_t      Parameters should be set
                                                  appropriately while enabling.
@sa
qapi_WLAN_Promiscuous_Mode_Info_t
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_ENABLE_PROMISCUOUS_MODE             11

/** @cond */
/** For future use */
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_CHIP_LINK_STATE                     12
/** @endcond */

/**
Command ID to set WLAN power mode policy when entering Power Save mode.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().

@param[in] qapi_WLAN_Power_Policy_Params_t      Policy parameters populated
                                                by the application.
@sa
qapi_WLAN_Power_Policy_Params_t
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_POWER_MODE_POLICY                   13

/**
Command ID to set/get the virtual device power mode.

The supported modes are Power Save mode (also known as REC_POWER) and Performance mode (MAX_PERF).
Applications are recommended to configure virtual devices in Performance mode
when more than one virtual device is connected (concurrency enabled).

@note1hang This parameter can be used with qapi_WLAN_Set_Param() and qapi_WLAN_Get_Param().

@param[in,out] qapi_WLAN_Power_Mode_Params_t         Power mode configurations.

@sa
qapi_WLAN_Power_Mode_Params_t
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_POWER_MODE_PARAMS                   14

/** @cond */
/**
Used to get the reason the device was disconnected.
Not used at this time.
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_DISCONNECT_REASON                   15
/** Not used at this time. */
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_COUNTRY_CODE                        16
/** @endcond */

/**
Command ID to enable/disable Wake on Wireless (WOW).

This command just initializes the WOW feature and does not perform any filtering
unless appropriate wake patterns are set.
If this feature is enabled, the firmware uses an out-of-band interrupt to awaken the host
in case of a pattern match.

The application should only enable this feature if the bus interrupt cannot be configured
as a wakeup interrupt.
If the bus interrupt can be a wakeup interrupt, the packet filtering feature should be
sufficient to perform necessary filtering functionality.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().

@param[in] uint32_t        Set to TRUE to enable WOW, FALSE to disable.
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_ENABLE_WAKE_ON_WIRELESS             17

/**
Command ID to add a new packet filter/WOW filter pattern.

This command allows the application to add one filter pattern at a time.
The pattern index value passed by the application should not exceed 8.

Pattern update is currently not supported.

If a given pattern index must be updated, the application should delete the pattern
in that index first and then add a pattern again for the same index.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().

@param[in] qapi_WLAN_Add_Pattern_t       Parameter with the necessary pattern
                                         information.
@sa
qapi_WLAN_Add_Pattern_t\n
__QAPI_WLAN_PARAM_GROUP_WIRELESS_DELETE_PATTERN
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_ADD_PATTERN                         18

/**
Command ID to enable/disable the Green TX feature.
This is a power optimization feature where WLAN transmit power is
reduced when operating under good link conditions.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().

@param[in] uint32_t          Set to TRUE to enable Green TX, FALSE to disable.
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_ENABLE_GREEN_TX                     19

/**
Command ID to enable/disable the Low Power Listen (LPL) feature.
This is a power optimization feature where WLAN listen is compromised by
operating some of the components with reduced power.
Under good link conditions, this compromise should not have any impact on packet reception.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().Device ID must use 0.

@param[in] uint32_t          Set to TRUE to enable LPL, FALSE to disable.
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_ENABLE_LOW_POWER_LISTEN             20

/**
Command ID to set (for TX)/get (for RX) the WLAN rate of packets transmitted over radio.

@note1hang This parameter can be used with qapi_WLAN_Set_Param() and qapi_WLAN_Get_Param().

@param[in,out] qapi_WLAN_Bit_Rate_t    Rate value to be set

@sa
qapi_WLAN_Bit_Rate_t
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_RATE                                21

/**
Command ID to get the current operating regulatory domain from the driver.

@note1hang This parameter can only be used with qapi_WLAN_Get_Param().

@param[out] uint32_t        Current operating regulatory domain that will
                            be filled by the driver.
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_REG_DOMAIN                          22

/** @cond */
/**
Used to set device MAC address.

@note1hang This parameter can be used with qapi_WLAN_Set_Param() and qapi_WLAN_Get_Param().
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_MAC_ADDRESS                         23
/** @endcond */

/**
Command ID to get the driver transmit queue status.
The application should ensure that no packets are pending for transmission before
putting WLAN in Suspend mode.

@note1hang This parameter can only be used with qapi_WLAN_Get_Param().

@param[out] uint32_t        Variable in which the driver returns the Tx status.
                            The driver populates __QAPI_WLAN_TX_STATUS_IDLE if
                            the driver queue is empty, some other value otherwise.

@sa
__QAPI_WLAN_TX_STATUS_IDLE
__QAPI_WLAN_TX_STATUS_HOST_PENDING
__QAPI_WLAN_TX_STATUS_WIFI_PENDING
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_TX_STATUS                           24

/**
Command ID to get various statistics information from the WLAN driver/firmware.

@note1hang This parameter can only be used with qapi_WLAN_Get_Param().

@param[out] qapi_WLAN_Get_Statistics_t      Variable in which WLAN populates
                                            stats information collected from the firmware.

@sa
qapi_WLAN_Get_Statistics_t
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_STATS                               25

/**
Command ID to enable/disable the preferred network offload feature.

This command also enables the application to configure various global parameters for the PNO feature.
The application should enable the PNO feature after WLAN disconnects from the AP and
disable PNO before connecting to an AP.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().

@param[in] qapi_WLAN_Preferred_Network_Offload_Config_t    Holds global configuration
                                                           parameters for the PNO feature.

@sa
qapi_WLAN_Preferred_Network_Offload_Config_t
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_PREFERRED_NETWORK_OFFLOAD_ENABLE    26

/**
Command ID to set the preferred network offload (PNO) profile for which the firmware
performs autonomous scans based on the PNO global configuration provided.

This command allows the user to add one SSID profile to be scanned on the given index.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().

@param[in] qapi_WLAN_Preferred_Network_Profile_t   PNO profile information to scan.

@sa
qapi_WLAN_Preferred_Network_Profile_t\n
__QAPI_WLAN_PNO_MAX_NETWORK_PROFILES
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_PREFERRED_NETWORK_PROFILE           27

/** @cond */
/**
Used to set application information element in outgoing frames.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_APP_IE                              28
/** @endcond */

/**
Command ID to enable/disable the coex feature in the WLAN subsystem.

This command also allows modification of the coex policy and mode of operation.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().

@param[in] qapi_WLAN_Coex_Config_Data_t    Coex configuration data to be
                                           applied.

@sa
qapi_WLAN_Coex_Config_Data_t
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_ENABLE_COEX                         29

/**
Used to retrieve WLAN coexistence statistics.

@note1hang This parameter can only be used with qapi_WLAN_Get_Param().

@param[in] qapi_WLAN_Coex_Stats_t    WLAN coex statistics.

@sa qapi_WLAN_Coex_Stats_t

*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_COEX_STATS                          30

/**
Command ID to configure the keepalive frame interval period in Station mode.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().

@param[in] uint32_t        Keepalive interval in seconds.
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_STA_KEEP_ALIVE_IN_SEC               31

/**
Command ID to configure the 802.11 listen interval when operating in Station mode.
This value will be used in the listen interval field of the association request frame.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().

@param[in] uint32_t        Listen interval in multiples of beacon intervals
                           (a value of 1 corresponds to 1 beacon interval).
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_STA_LISTEN_INTERVAL_IN_TU           32

/**
Command ID to get the RSSI of the associated peer.

@note1hang This parameter can only be used with qapi_WLAN_Get_Param().

@param[out] uint8_t         RSSI value variable received from the firmware.
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_RSSI                                33

/**
Used to set the received signal strength indicator threshold.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_RSSI_THRESHOLD                      34

/**
Command ID to configure global parameters of the TCP keepalive (KA) offload feature.

This command also allows application to set global parameters when using the TCP KA
offload feature.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().

@param[in] qapi_WLAN_TCP_Offload_Enable_t      Configuration parameters to be
                                               set for the TCP KA feature.

@sa
qapi_WLAN_TCP_Offload_Enable_t
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_TCP_KEEPALIVE_OFFLOAD_ENABLE        35

/**
Command ID to configure session parameters of the TCP keepalive offload session.
This command allows application to configure one session information at a time.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().

@param[in] qapi_WLAN_TCP_Offload_Config_Params_t   Per-session configuration
                                                   information to be used in a TCP
                                                   KA session. The maximum number of
                                                   TCP KA sessions should
                                                   not exceed 3.

@sa
qapi_WLAN_TCP_Offload_Config_Params_t
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_TCP_KEEPALIVE_OFFLOAD_SESSION_CFG   36

/**
Command ID to enable/disable unscheduled automatic power save delivery (UAPSD)
in Station mode.
This should be done before associating to an access point.

Currently, this command only supports all ACs or none, and explicit UAPSD (ADDTS)
is not supported.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().

@param[in] uint32_t        Bitmask of access categories for which UAPSD
                           is to be enabled. Following are the bitmask
                           mappings for various classes of traffic:
                           - Bit 0 -- Best effort traffic
                           - Bit 1 -- Background traffic
                           - Bit 2 -- Video traffic
                           - Bit 3 -- Voice traffic
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_STA_UAPSD                           37

/**
Used to set the maximum number of total buffered MSDUs and MMPDUs delivered by the AP
to the station during a service period.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_STA_MAX_SP_LEN                      38

/**
Command ID to set reiceive AMSDU enable or disable.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().

@param[in] uint32_t      Set to TRUE to enable AMSDU receive mode, FALSE otherwise.
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_AMSDU_RX            39


/**
Command ID to set the beacon interval (in time units) when operating in SoftAP mode.
One TU = 1024 microseconds.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().

@param[in] uint32_t      Number of TUs between every beacon in SoftAP mode.
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_AP_BEACON_INTERVAL_IN_TU            40

/**
Command ID to enable/disable the hidden SSID feature when operating a virtual device
in SoftAP mode.

This should be done after setting the operating mode as AP and before
committing the AP profile using qapi_WLAN_Commit().

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().

@param[in] uint32_t        Set to TRUE to enable the hidden SSID feature, FALSE otherwise.

@dependencies
Should be set after setting the operating mode to AP by issuing __QAPI_WLAN_PARAM_GROUP_WIRELESS_OPERATION_MODE.\n
Should be set before invoking qapi_WLAN_Commit() to start SoftAP.

@sa
__QAPI_WLAN_PARAM_GROUP_WIRELESS_OPERATION_MODE\n
qapi_WLAN_Commit()
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_AP_ENABLE_HIDDEN_MODE               41

/** @cond */
/** Not used at this time. */
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_AP_CONNECT_CONTROL_FLAG             42
/** @endcond */

/**
Command ID to set an AP's inactivity period in minutes.

If no keepalive frames are received from an associated station during
this period, the AP deassociates that station.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().

@param[in] uint32_t        Inactivity interval for associated stations in
                           minutes.
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_AP_INACTIVITY_TIME_IN_MINS          43

/**
Command ID to enable/disable the WPS feature when operating a virtual device
in SoftAP mode.

This should be done after setting the operating mode as AP and before
committing the AP profile using qapi_WLAN_Commit().

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().

@param[in] uint32_t        Set to TRUE to enable the WPS feature, FALSE otherwise.

@dependencies
Should be set after setting the operating mode to AP by issuing __QAPI_WLAN_PARAM_GROUP_WIRELESS_OPERATION_MODE.\n
Should be set before invoking qapi_WLAN_Commit() to start SoftAP.

@sa
qapi_WLAN_Commit() \n
__QAPI_WLAN_PARAM_GROUP_WIRELESS_OPERATION_MODE
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_AP_WPS_FLAG                         44

/**
Command ID to change the DTIM interval when operating a virtual device
in SoftAP mode.

This setting should be done before committing the AP profile using qapi_WLAN_Commit().

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().

@param[in] uint32_t        DTIM interval in multiples of the beacon interval.

@dependencies
Should be set after setting the operating mode to AP by issuing __QAPI_WLAN_PARAM_GROUP_WIRELESS_OPERATION_MODE.\n
Should be set before invoking qapi_WLAN_Commit() to start SoftAP.

@sa
__QAPI_WLAN_PARAM_GROUP_WIRELESS_OPERATION_MODE\n
qapi_WLAN_Commit
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_AP_DTIM_INTERVAL                    45

/** @cond */
/**
Used to set per-STA buffers in AP mode.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_AP_PS_BUF                           46

/**
Used to set the WOW GPIO configuration.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_WOW_GPIO_CONFIG                     47
/** @endcond */

/**
Command ID to set Address Resolution Protocol (ARP) offload parameters for the
given virtual device.
This should only be used when operating in Station mode.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().

@param[in] qapi_WLAN_ARP_Offload_Config_t    ARP offload configuration parameters.

@sa
qapi_WLAN_ARP_Offload_Config_t
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_ARP_OFFLOAD_PARAMS                  48

/**
Command ID to set network solicitation (NS) offload parameters for the
given virtual device.
This should only be used when operating in Station mode.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().

@param[in] qapi_WLAN_NS_Offload_Config_t    NS offload configuration parameters.

@sa
qapi_WLAN_NS_Offload_Config_t
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_NS_OFFLOAD_PARAMS                   49

/**
Command ID to enable/disable the packet filtering feature.

This command just initializes the packet filtering feature and does not
perform any filtering until packet filter patterns are set.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().

@param[in] uint32_t        Set to TRUE to enable packet filtering, FALSE to disable.
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_ENABLE_PKT_FILTER                   50
/**
Command ID to delete a patten in a given pattern index.
The pattern index should not exceed 8.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().

@param[in] uint32_t        Pattern index to be deleted.
@param[in] uint32_t        Pattern header type.

@sa
__QAPI_WLAN_PARAM_GROUP_WIRELESS_ADD_PATTERN
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_DELETE_PATTERN                      51

/**
Command ID to enable/disable UAPSD in SoftAP mode.

UAPSD must be enabled in SoftAP after setting the operating mode as AP and
before committing the AP profile (qapi_WLAN_Commit()) to start SoftAP.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().

@param[in] uint32_t        Set to TRUE to enable UAPSD, FALSE otherwise.

@dependencies
Should be set after setting the operating mode to AP by issuing __QAPI_WLAN_PARAM_GROUP_WIRELESS_OPERATION_MODE.\n
Should be set before invoking qapi_WLAN_Commit() to start SoftAP.

@sa
qapi_WLAN_Commit\n
__QAPI_WLAN_PARAM_GROUP_WIRELESS_OPERATION_MODE
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_AP_ENABLE_UAPSD                     52

/**
Command ID to configure A-MPDU parameters for an AMPDU session.

These parameters should be set before establishing the connection in both STA and AP modes.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().

@param[in] qapi_WLAN_Rx_Aggrx_Params_t     Aggregation parameters to be set.

@sa
qapi_WLAN_Rx_Aggrx_Params_t
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_AGGRX_CONFIG                        53

/**
Command ID to enable/disable 802.11v functionality for the WLAN subsystem.

This command instantiates WNM context in the WLAN subsystem but does not actually enable
any WNM features.

Applications should use other WNM commands to enable required features and
enable the WNM configuration using this command before enabling any WNM features.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().

@param[in] uint8_t         Set to TRUE to enable WNM, FALSE to disable
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_WNM_CONFIG                          54

/**
Command ID to enter/exit 802.11v WNM sleep along with the sleep parameters.
This command is only supported only in Station mode.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().

@param[in] qapi_WLAN_WNM_Sleep_Period_t    Parameters to enter/exit WNM sleep.

@dependencies
Applications should enable WNM using __QAPI_WLAN_PARAM_GROUP_WIRELESS_WNM_CONFIG
before enabling the WNM sleep feature.

@sa
qapi_WLAN_WNM_Sleep_Period_t\n
__QAPI_WLAN_PARAM_GROUP_WIRELESS_WNM_CONFIG
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_WNM_STA_SLEEP_PERIOD                55

/**
Command ID to enable/disable the WNM BSS maximum idle period feature.

This command is only supported in SoftAP mode.

If this feature is enabled, SoftAP beacons will start including the WNM BSS maximum idle period IE.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().

@param[in] qapi_WLAN_BSS_Max_Idle_Period_t Parameter to enable the WNM BSS maximum
                                           idle period feature in SoftAP.

@dependencies
Applications should enable WNM using __QAPI_WLAN_PARAM_GROUP_WIRELESS_WNM_CONFIG
before enabling the WNM BSS maximum idle feature.

@sa
__QAPI_WLAN_PARAM_GROUP_WIRELESS_WNM_CONFIG\n
qapi_WLAN_BSS_Max_Idle_Period_t
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_WNM_AP_BSS_MAX_IDLE_PERIOD          56

/** @cond */
/**
Used to set the AP's response for a WNM sleep request from the station.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param(). \n
It is only used for internal WNM sleep testing in SoftAP mode.
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_WNM_AP_SLEEP_RESPONSE               57
/** @endcond */

/**
Command ID to initiate a channel switch in SoftAP mode.

This command enables SoftAP to send a channel switch request in its beacons to all its
associated clients for given periods and changes SoftAP's operating channel to a new channel
requested by the application upon completion of a requested period.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().

@param[in] qapi_WLAN_Channel_Switch_t      Parameters to be set for a
                                           channel switch request.

@dependencies
SoftAP must be up and running.

@sa
qapi_WLAN_Channel_Switch_t
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_AP_CHANNEL_SWITCH                   58

/**
Command ID to set up event filters in the WLAN subsystem.

This command enables an applications to choose events that are not in its interest
so that the firmware can skip sending those events to hosts, thereby avoiding unnecessary host wakeups.

Note that all the events are not filterable. Event filters should be set for every virtual device. The
maximum number of events that can be filtered for each virtual device should not exceed
__QAPI_WLAN_MAX_NUM_FILTERED_EVENTS.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().

@param[in] qapi_WLAN_Event_Filter_t        Event filter configuration to be
                                           set.

@sa
qapi_WLAN_Filterable_Event_e \n
qapi_WLAN_Event_Filter_t \n
__QAPI_WLAN_MAX_NUM_FILTERED_EVENTS
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_EVENT_FILTER                        59

/**
Command ID to set wireless 11n HT parameters of a given virtual device. The set
operation for this should be done before establishing a connection.

@note1hang This parameter can be used with qapi_WLAN_Set_Param() and qapi_WLAN_Get_Param().

@param[in,out] qapi_WLAN_11n_HT_Config_e    Required 11n HT configuration must be specified.

@sa
qapi_WLAN_11n_HT_Config_e
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_11N_HT                              60

/**
Command ID to set Beacon Miss configuration.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().

@param[in,out] qapi_WLAN_Sta_Config_Bmiss_Config_t    Beacon Miss parameters to be set.

@sa
qapi_WLAN_Sta_Config_Bmiss_Config_t
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_STA_BMISS_CONFIG                              61


/**
Command ID to change the action for the default filter for a given header type.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().

@param[in] uint32_t        Pattern action flag.
@param[in] uint32_t        Pattern header type.

@sa
__QAPI_WLAN_PARAM_GROUP_WIRELESS_ADD_PATTERN
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_CHANGE_DEFAULT_FILTER_ACTION        62

/**
Command ID to configure the country code for AP mode.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().

@param[in] uint8_t[]        Country code string.

*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_AP_COUNTRY_CODE                        63

/**
Command ID to configure SCO coex operating parameters in the WLAN subsystem.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().

@param[in] qapi_WLAN_Coex_Sco_Config_t     Coex SCO configuration data to be
                                           applied.

@sa
qapi_WLAN_Coex_Sco_Config_t
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_CONFIG_SCO_COEX                        64

/**
Command ID to configure A2DP coex operating parameters in the WLAN subsystem.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().

@param[in] qapi_WLAN_Coex_A2dp_Config_t    Coex A2DP configuration data to be
                                           applied.

@sa
qapi_WLAN_Coex_A2dp_Config_t
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_CONFIG_A2DP_COEX                       65

/**
Command ID to configure ACL coex operating parameters in the WLAN subsystem.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().

@param[in] qapi_WLAN_Coex_Acl_Config_t     Coex ACL configuration data to be
                                           applied.

@sa
qapi_WLAN_Coex_Acl_Config_t
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_CONFIG_ACL_COEX                       66

/**
Command ID to configure INQPAGE coex operating parameters in the WLAN subsystem.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().

@param[in] qapi_WLAN_Coex_InqPage_Config_t   Coex INQPAGE configuration data to be
                                             applied.

@sa
qapi_WLAN_Coex_InqPage_Config_t
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_CONFIG_INQPAGE_COEX                  67

/**
Command ID to configure HID coex operating parameters in the WLAN subsystem.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().

@param[in] qapi_WLAN_Coex_Hid_Config_t     Coex HID configuration data to be
                                           applied.

@sa
qapi_WLAN_Coex_Hid_Config_t
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_CONFIG_HID_COEX                     68

/**
Command ID to override the default weight table values used by the WLAN coex subsystem.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param()..

@param[in] qapi_WLAN_Coex_Override_Wghts_t     Coex weight value overrides to be
                                               applied.

@sa
qapi_WLAN_Coex_Override_Wghts_t
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_OVERRIDE_WGHTS_COEX                 69

/**
Command ID to get the channel list for the current regulatory setting.

@note1hang This parameter can only be used with qapi_WLAN_Get_Param().

@param[out] qapi_WLAN_Get_Channel_List_t    Variable in which WLAN populates
                                            channel information collected from the firmware.

@sa
#qapi_WLAN_Get_Channel_List_t

*/

#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_GET_CURR_REGDOMAIN_CHANNEL_LIST                    70

/**
Command ID to set antenna diversity, such as enable or disable antenna diversity,
tx antenna follows rx antenna or not, the antenna switch mode, the time interval or
the number of packet used for antenna selection.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().

@param[in] qapi_WLAN_Ant_Div_Config_t    Variable used to set antenna diversity.

@sa
#qapi_WLAN_Ant_Div_Config_t

*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_SET_ANT_DIV               					    71

/**
Command ID to get the status of antenna diversity, such as the current rx physical antenna in 2g or 5g,
the current tx physical antenna in 2g or 5g, the average main rssi, the average alternative rssi and so on.

@note1hang This parameter can only be used with qapi_WLAN_Get_Param().

@param[out] qapi_WLAN_Get_Ant_Div_t    Variable in which WLAN populates the status of antenna diversity
										  information collected from the firmware
@sa
#qapi_WLAN_Get_Ant_Div_t

*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_GET_ANT_DIV               					    72

/**
Command ID to set physical antenna.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().

@param[in] uint8_t the physical antenna number
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_SET_ANTENNA               					    73

/**
Command ID to set/get the authentication mode for an upcoming association operation.

@note1hang This parameter can be used with qapi_WLAN_Set_Param() and qapi_WLAN_Get_Param().

@param[in,out] qapi_WLAN_Auth_Mode_e   Authentication mode to be set.

@dependencies
Authentication mode must be set before connecting to the peer.

@sa
qapi_WLAN_Auth_Mode_e
*/

/**
Internal use.
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_DISABLE_CHANNEL									74

/**
0: will use default bData, 1: will use ext-bData 1, for future, '2' will use ext-bData 2, etc...

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_EXT_BOARDDATA_INDEX	      							75

/**
Command ID to get the DTIM value when device is working as station in connection status.
If device is not working as station, or device is disconnected, then returned value will be 0.

@note1hang This parameter can only be used with qapi_WLAN_Get_Param().

@param[out] uint8_t    DTIM value.   Unit:ms

@sa
uint8_t

*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_GET_STA_DTIM               					    76

/**
Command ID to enable/disable the WIFI simple configuration state in SoftAP mode.

This should be done after setting the operating mode as AP and before
committing the AP profile using qapi_WLAN_Commit().

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().

@param[in] uint32_t        Set 1 to configure wps state, 0 to be unconfigured.

@dependencies
- Should be set after setting the operating mode to AP by issuing __QAPI_WLAN_PARAM_GROUP_WIRELESS_OPERATION_MODE.
- Should be set before invoking qapi_WLAN_Commit() to start SoftAP.

@sa
qapi_WLAN_Commit() \n
__QAPI_WLAN_PARAM_GROUP_WIRELESS_OPERATION_MODE
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_AP_WPS_SIMPLE_CONFIGURATION_STATE                   77

/**
This definition relates to WLAN_LowRssi_Control_Parameters.

@param[in] qapi_WLAN_LowRssi_RoamControl_Params_t  Set roam low RSSI threshold.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_ROAM_CONTROL                      					78

/**
Command ID to configure connection policy.
If device is not working as a station, then returned value will be -1.

@note1hang This paramter can only be used with qapi_WLAN_Set_Param().

@param[in] qapi_WLAN_Connect_Policy_t        Sets connection policy.

@sa
#qapi_WLAN_Connect_Policy_t
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_CONNECT_POLICY         79

/**
Command ID that sets broadcast and multicast Rx filter (only in XSTICK mode).

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().

@param[in] qapi_WLAN_Set_BMcast_Filter_t  Enables broadcast and multicast Rx filter.

@sa
#qapi_WLAN_Set_BMcast_Filter_t
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_SET_BMCAST_RX_FILTER         80

/**
Command ID that gets the WNM BSS maximum idle period of AP.

@note1hang This command is only supported in Station mode.

@param[out] uint16_t  BSS maximum period value. Idle timeout = 1000TU * BSS Max Idle Period (ms).

@sa
uint16_t
*/

#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_WNM_STA_GET_BSS_MAX_IDLE_PERIOD          81


/**
Command ID to set/get the concurrency mode of device in the WLAN subsystem.

@note1hang This parameter can be used with qapi_WLAN_Get_Param().

@param[in,out] concurrency mode  Address of the variable type qapi_WLAN_DEV_Mode_e

@sa
qapi_WLAN_DEV_Mode_e
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_CONCURRENCY_MODE				82


/**
Command ID to set/get the filter of management frames which will be sent to application in the WLAN subsystem.

@note1hang This parameter can be used with qapi_WLAN_Enable_Mgmt_Filter() and qapi_WLAN_Disable_Mgmt_Filter().

@param[in,out] management frame type of the variable type qapi_WLAN_MGMT_FRAME_e

@sa
qapi_WLAN_MGMT_FRAME_e
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_MGMT_FRAME_FILTER				83

/**
Command ID to enable/disable RTS/CTS protection when operating in Station mode.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().

@param[in] uint32_t        Set 1 to enable RTS/CTS protection, 0 to be disabled.
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_RTS           84

/**
Command ID to fix RTS rate in 2G when operating in Station mode.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().

@param[in] uint32_t        0: 1Mbps  1: 6Mbps
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_RTS_RATE_2G           85

/**
Command ID to adjust edca parameters when operating in Station mode.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().

@param[in] qapi_WLAN_Edca_Params_t  Set edca parameters for queue 0-7.

@sa
#qapi_WLAN_Edca_Params_t
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_EDCA_PARAM           86

/**
Command ID to adjust PER upper threshold when operating in Station mode.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().

@param[in] uint32_t  Set PER upper threshold to 0-100.
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_PER_UPPER_THRESHOLD           87

/**
Command ID to adjust BA window size when operating in Station mode.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().

@param[in] qapi_WLAN_BA_Window_Params_t  Set BA window size.

@sa
#qapi_WLAN_BA_Window_Params_t
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_BA_WINDOW           88

/**
Command ID to adjust BA window size when operating in Station mode.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().

@param[in] uint32_t  change slot time to 9us/20us.
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_SLOT_TIME           89

/**
Command ID to adjust EDCCA threshold when operating in Station mode.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().

Set EDCCA threshold to 0-100.
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_EDCCA_THRESHOLD     90

/**
Command ID to configure the DTIM value when operating in Station mode.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().

@param[in] uint32_t        set force dtim value 1-30
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_SET_STA_DTIM           91

#define __QAPI_WLAN_PARAM_GROUP_SECURITY_AUTH_MODE                0

/**
Command ID to set/get the encryption mode for an upcoming association operation.

@note1hang This parameter can be used with qapi_WLAN_Set_Param() and qapi_WLAN_Get_Param().

@param[in,out] qapi_WLAN_Crypt_Type_e   Encryption mode to be set.

@dependencies
Encryption mode must be set before connecting to the peer.

@sa
qapi_WLAN_Crypt_Type_e
*/
#define __QAPI_WLAN_PARAM_GROUP_SECURITY_ENCRYPTION_TYPE          1

/**
Command ID to set the pairwise master key for the upcoming WPA/WPA2 association
procedure.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().

@param[in] uint8_t[]       Pairwise master key to be set. The master key
                           should be of length __QAPI_WLAN_PASSPHRASE_LEN.
@dependencies
This should be done before initiating an association.

@sa
__QAPI_WLAN_PASSPHRASE_LEN
*/
#define __QAPI_WLAN_PARAM_GROUP_SECURITY_PMK                      2

/**
Command ID to set the passphrase for the upcoming WPA/WPA2 association procedure.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().

@param[in] uint8_t[]       Passphrase to be set. The passphrase length
                           should not exceed __QAPI_WLAN_PASSPHRASE_LEN.
@dependencies
This should be done before initiating an association.

@sa
__QAPI_WLAN_PASSPHRASE_LEN
*/
#define __QAPI_WLAN_PARAM_GROUP_SECURITY_PASSPHRASE               3

/**
Command ID to set the active WEP key index for an upcoming association.

@note1hang This parameter can be used with qapi_WLAN_Set_Param() and qapi_WLAN_Get_Param() APIs.

@param[in,out] uint32_t        Active key index to be set. The value should be
                               set so that \n 1 <= index <= 4.

@dependencies
This should be done before initiating an association.
*/
#define __QAPI_WLAN_PARAM_GROUP_SECURITY_WEP_KEY_INDEX            4

/**
Command ID to set a WEP {key index , key} pair for an upcoming association.

@note1hang This parameter can be used with qapi_WLAN_Set_Param() and qapi_WLAN_Get_Param() APIs.

@param[in,out] qapi_WLAN_Security_Wep_Key_Pair_Params_t   Parameters to set the
                                                          WEP key parameters.
@dependencies
This should be done before initiating an association.
*/
#define __QAPI_WLAN_PARAM_GROUP_SECURITY_WEP_KEY_PAIR             5

/**
Command ID to set WPS credentials received after WPS negotiation with the peer.
These are the credentials that will be used for secure association with the peer.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().

@param[in] qapi_WLAN_WPS_Credentials_t     WPS credential information to be used
                                           for a secure association.

@dependencies
This should be done after WPS negotiation is completed and before performing a
secure association.

@sa
qapi_WLAN_WPS_Credentials_t
*/
#define __QAPI_WLAN_PARAM_GROUP_SECURITY_WPS_CREDENTIALS          6

/**
Command ID to set/get the 802.1x method.

@note1hang This parameter can be used with qapi_WLAN_Set_Param() and qapi_WLAN_Get_Param().

@param[in,out] qapi_WLAN_8021x_Method_e   802.1x mode to be set.

@sa
qapi_WLAN_8021x_Method_e

*/
#define __QAPI_WLAN_PARAM_GROUP_SECURITY_8021X_METHOD           7

/**
The first Command ID to set/get the 802.1x information.
*/
#define __QAPI_WLAN_PARAM_GROUP_SECURITY_8021X_START            (__QAPI_WLAN_PARAM_GROUP_SECURITY_8021X_METHOD)

/**
The first Command ID to set/get wlan_lib information.
*/
#define __QAPI_WLAN_PARAM_GROUP_SECURITY_WLIB_START            (__QAPI_WLAN_PARAM_GROUP_SECURITY_8021X_START)

/**
@ingroup qapi_wlan
Set the 802.1x identity for PEAP/TTLS method.\n

@sa
__QAPI_WLAN_PARAM_GROUP_SECURITY_8021X_IDENTITY
qapi_WLAN_Set_Param
*/
#define __QAPI_WLAN_PARAM_GROUP_SECURITY_8021X_IDENTITY         8

/**
@ingroup qapi_wlan
Set the 802.1x username for PEAP/TTLS method.\n

@sa
__QAPI_WLAN_PARAM_GROUP_SECURITY_8021X_USERNAME
qapi_WLAN_Set_Param
*/
#define __QAPI_WLAN_PARAM_GROUP_SECURITY_8021X_USERNAME         9

/**
@ingroup qapi_wlan
Set the 802.1x password for PEAP/TTLS method.\n

@sa
__QAPI_WLAN_PARAM_GROUP_SECURITY_8021X_PASSWORD
qapi_WLAN_Set_Param
*/
#define __QAPI_WLAN_PARAM_GROUP_SECURITY_8021X_PASSWORD         10

/**
@ingroup qapi_wlan
Set/get the 802.1x CA certificate filename.

@sa
__QAPI_WLAN_PARAM_GROUP_SECURITY_8021X_CA_CERT
qapi_WLAN_Set_Param
qapi_WLAN_Get_Param
*/
#define __QAPI_WLAN_PARAM_GROUP_SECURITY_8021X_CA_CER           11

/**
@ingroup qapi_wlan
Set/get the 802.1x certificate filename.

@sa
__QAPI_WLAN_PARAM_GROUP_SECURITY_8021X_CERT
qapi_WLAN_Set_Param
qapi_WLAN_Get_Param
*/
#define __QAPI_WLAN_PARAM_GROUP_SECURITY_8021X_CER              12

/**
@ingroup qapi_wlan
Set 802.1x private key filename and its password.

@sa
__QAPI_WLAN_PARAM_GROUP_SECURITY_8021X_PRIVATE_KEY
qapi_WLAN_Set_Param
*/
#define __QAPI_WLAN_PARAM_GROUP_SECURITY_8021X_PRIVATE_KEY      13

/**
@ingroup qapi_wlan
Set server authentication override flag.

@sa
__QAPI_WLAN_PARAM_GROUP_SECURITY_8021X_NO_SERVER_AUTH
qapi_WLAN_Set_Param
*/
#define __QAPI_WLAN_PARAM_GROUP_SECURITY_8021X_NO_SERVER_AUTH         14

/**
The last Command ID to set/get 802.1x information.
*/
#define __QAPI_WLAN_PARAM_GROUP_SECURITY_8021X_END            30

/**
The first Command ID to set/get WLAN library information.
*/
#define __QAPI_WLAN_PARAM_GROUP_SECURITY_WLIB_MAIN_START            (__QAPI_WLAN_PARAM_GROUP_SECURITY_8021X_END+1)

/**
@ingroup qapi_wlan
Set security debug level.\n

@sa
__QAPI_WLAN_PARAM_GROUP_SECURITY_DEBUG_LEVEL
qapi_WLAN_Set_Param
*/
#define __QAPI_WLAN_PARAM_GROUP_SECURITY_DEBUG_LEVEL              31

/**
@ingroup qapi_wlan
Set security priv.\n

@sa
__QAPI_WLAN_PARAM_GROUP_SECURITY_PRIV
qapi_WLAN_Set_Param
*/
#define __QAPI_WLAN_PARAM_GROUP_SECURITY_PRIV           32

/**
The last Command ID to set/get the WLAN library information.
*/
#define __QAPI_WLAN_PARAM_GROUP_SECURITY_WLIB_MAIN_END     (60)

/**
The first Command ID to set/get the supplicant main information.
*/
#define __QAPI_WLAN_PARAM_GROUP_SECURITY_SUPPL_MAIN_START            (__QAPI_WLAN_PARAM_GROUP_SECURITY_WLIB_MAIN_END+1)

/**
The last Command ID to set/get the supplicant main information.
*/
#define __QAPI_WLAN_PARAM_GROUP_SECURITY_SUPPL_MAIN_END     (90)

/**
The first Command ID to set/get the PMF information.
*/
#define __QAPI_WLAN_PARAM_GROUP_SECURITY_PMF_START          (__QAPI_WLAN_PARAM_GROUP_SECURITY_SUPPL_MAIN_END+1)

/**
@ingroup qapi_wlan
Set Protected Management Frame (PMF) capability.

@sa
__QAPI_WLAN_PARAM_GROUP_SECURITY_SET_RSN_CAP\n
qapi_WLAN_Set_Param
*/
#define __QAPI_WLAN_PARAM_GROUP_SECURITY_SET_RSN_CAP        (__QAPI_WLAN_PARAM_GROUP_SECURITY_PMF_START)

/**
The last Command ID to set/get PMF information.
*/
#define __QAPI_WLAN_PARAM_GROUP_SECURITY_PMF_END          (120)

/**
The first Command ID to set/get Simultaneous Authentication of Equals (SAE) information.
*/
#define __QAPI_WLAN_PARAM_GROUP_SECURITY_SAE_START          (__QAPI_WLAN_PARAM_GROUP_SECURITY_PMF_END+1)

/**
@ingroup qapi_wlan
Set finite cyclic group for SAE.

@sa
__QAPI_WLAN_PARAM_GROUP_SECURITY_FINITE_CYCLIC_GROUP\n
qapi_WLAN_Set_Param
*/
#define __QAPI_WLAN_PARAM_GROUP_SECURITY_FINITE_CYCLIC_GROUP              (__QAPI_WLAN_PARAM_GROUP_SECURITY_SAE_START)

/**
@ingroup qapi_wlan
Set PMK cache for sae.\n

@sa
__QAPI_WLAN_PARAM_GROUP_SECURITY_SAE_PMK_CACHE
qapi_WLAN_Set_Param
*/
#define __QAPI_WLAN_PARAM_GROUP_SECURITY_SAE_PMK_CACHE        (122)

/**
The last Command ID to set/get SAE information.
*/
#define __QAPI_WLAN_PARAM_GROUP_SECURITY_SAE_END          (150)

/**
The last Command ID to set/get wlan_lib information.
*/
#define __QAPI_WLAN_PARAM_GROUP_SECURITY_WLIB_END          1000

/**
@ingroup qapi_wlan
Set PMKID.\n

@sa
__QAPI_WLAN_PARAM_GROUP_SECURITY_PMKID
qapi_WLAN_Set_Param
*/
#define __QAPI_WLAN_PARAM_GROUP_SECURITY_PMKID          1001


/**
Command ID that gets the Robust Security Network (RSN) capability.

@note1hang This command is only supported in Station mode.

@param[out] uint16_t

@sa
uint16_t
*/
#define __QAPI_WLAN_PARAM_GROUP_SECURITY_GET_RSN_CAP        1002

#endif  /* __QAPI_WLAN_PARAM_GROUP_H__ */


