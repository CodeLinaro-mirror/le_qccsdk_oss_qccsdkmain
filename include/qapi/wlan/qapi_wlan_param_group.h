/* 
Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
SPDX-License-Identifier: BSD-3-Clause-Clear
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
Macro that indicates the group ID that can be used to configure wireless parameters of
the WLAN subsystem.
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS 1

/**
Macro that indicates the group ID that can be used to configure security parameters of
the WLAN subsystem.
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_SECURITY 2

/**
Macro that indicates the group ID that can be used to configure various
P2P (Peer-to-Peer/Wi-Fi Direct) parameters.
*/
#define __QAPI_WLAN_PARAM_GROUP_P2P 3

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
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_OPERATION_MODE 1

/**
Command ID to set/get the operating wireless channel of a given virtual device in
the WLAN subsystem.

For set/get operations, channel values are set in numbers (channel number 1-14, 36-165)
and not in frequency values.

@note1hang This parameter can be used with qapi_WLAN_Set_Param() and qapi_WLAN_Get_Param().

@param[in,out] uint32_t    Variable that holds the channel number.
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_CHANNEL 2

/**
Command ID to set/get the transmit power in dBm of a given virtual device.

@note1hang This parameter can be used with qapi_WLAN_Set_Param() and qapi_WLAN_Get_Param().

@param[in,out] uint32_t        Address of the variable that holds the power value.
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_TX_POWER_IN_DBM 4

/**
Command ID to set/get the SSID of/for a given virtual device in the WLAN subsystem.

@note1hang This parameter can be used with qapi_WLAN_Set_Param() and qapi_WLAN_Get_Param().

@param[in,out] uint8_t[] Unsigned byte array of size __QAPI_WLAN_MAX_SSID_LENGTH.

@sa
__QAPI_WLAN_MAX_SSID_LENGTH
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_SSID 5

/**
Command ID to set/get the BSSID of/for a given virtual device in the WLAN subsystem when operation in STA mode.

@note1hang This parameter can be used with qapi_WLAN_Set_Param() and qapi_WLAN_Get_Param().

@param[in,out] uint8_t[] Unsigned byte array of size __QAPI_WLAN_MAC_LEN.

@sa
__QAPI_WLAN_MAC_LEN

*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_BSSID 6

/**
Command ID to set/get the wireless PHY mode of/for a given virtual device.

The Set operation for this should be done before establishing a connection.

@note1hang This parameter can be used with qapi_WLAN_Set_Param() and qapi_WLAN_Get_Param().

@param[in,out] qapi_WLAN_Phy_Mode_e    Required PHY mode should be specified.

@sa
qapi_WLAN_Phy_Mode_e
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_PHY_MODE 7

/**
Command ID to allow/disallow aggregation for Tx and Rx on a TID basis.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().

@param[in] qapi_WLAN_Aggregation_Params_t  Strucuture populated with
                                           required aggregation parameters
                                           for Tx and Rx.

@sa
qapi_WLAN_Aggregation_Params_t
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_ALLOW_TX_RX_AGGR_SET_TID 9

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
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_POWER_MODE_PARAMS 14

/** @cond */
/** Not used at this time. */
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_COUNTRY_CODE 16
/** @endcond */

/** @cond */
/**
Used to set device MAC address.

@note1hang This parameter can be used with qapi_WLAN_Set_Param() and qapi_WLAN_Get_Param().
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_MAC_ADDRESS 23
/** @endcond */

/** @cond */
/**
Used to set application information element in outgoing frames.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_APP_IE 28
/** @endcond */

/**
Command ID to configure the 802.11 listen interval when operating in Station mode.
This value will be used in the listen interval field of the association request frame.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().

@param[in] uint32_t        Listen interval in multiples of beacon intervals
                           (a value of 1 corresponds to 1 beacon interval).
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_STA_LISTEN_INTERVAL_IN_TU 32

/**
Command ID to get the RSSI of the associated peer.

@note1hang This parameter can only be used with qapi_WLAN_Get_Param().

@param[out] uint8_t         RSSI value variable received from the firmware.
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_RSSI 33

/**
Command ID to set reiceive AMSDU enable or disable.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().

@param[in] uint32_t      Set to TRUE to enable AMSDU receive mode, FALSE otherwise.
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_AMSDU_RX 39

/**
Command ID to set the beacon interval (in time units) when operating in SoftAP mode.
One TU = 1024 microseconds.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().

@param[in] uint32_t      Number of TUs between every beacon in SoftAP mode.
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_AP_BEACON_INTERVAL_IN_TU 40

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
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_AP_ENABLE_HIDDEN_MODE 41

/** @cond */
/** Not used at this time. */
/*#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_AP_CONNECT_CONTROL_FLAG 42*/
/** @endcond */

/**
Command ID to set an AP's inactivity period in minutes.

If no keepalive frames are received from an associated station during
this period, the AP deassociates that station.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().

@param[in] uint32_t        Inactivity interval for associated stations in
                           minutes.
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_AP_INACTIVITY_TIME_IN_MINS 43

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
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_AP_DTIM_INTERVAL 45

/**
Command ID to set wireless 11n HT parameters of a given virtual device. The set
operation for this should be done before establishing a connection.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().

@param[in] qapi_WLAN_11n_HT_Config_s    Required 11n HT configuration must be specified.

@sa
qapi_WLAN_11n_HT_Config_s
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_11N_HT 60

/**
Command ID to set Beacon Miss configuration.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().

@param[in,out] qapi_WLAN_Sta_Config_Bmiss_Config_t    Beacon Miss parameters to be set.

@sa
qapi_WLAN_Sta_Config_Bmiss_Config_t
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_STA_BMISS_CONFIG 61

/**
Command ID to set/get the concurrency mode of device in the WLAN subsystem.

@note1hang This parameter can be used with qapi_WLAN_Get_Param().

@param[in,out] concurrency mode  Address of the variable type qapi_WLAN_DEV_Mode_e

@sa
qapi_WLAN_DEV_Mode_e
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_CONCURRENCY_MODE 82

/**
Command ID to enable/disable RTS/CTS protection when operating in Station mode.

@note1hang This parameter can be used with qapi_WLAN_Get_Param().

@param[in] uint32_t        Set 1 to enable RTS/CTS protection, 0 to be disabled.
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_RTS 84

/**
Command ID to fix RTS rate in 2G when operating in Station mode.

@note1hang This parameter can be used with qapi_WLAN_Get_Param().

@param[in] uint32_t        0: 1Mbps  1: 6Mbps
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_RTS_RATE_2G 85

/**
Command ID to adjust edca parameters when operating in Station mode.

@note1hang This parameter can be used with qapi_WLAN_Get_Param().

@param[in] qapi_WLAN_Edca_Params_t  Set edca parameters for queue 0-7.

@sa
#qapi_WLAN_Edca_Params_t
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_EDCA_PARAM 86

/**
Command ID to adjust PER upper threshold when operating in Station mode.

@note1hang This parameter can be used with qapi_WLAN_Get_Param().

@param[in] uint32_t  Set PER upper threshold to 0-100.
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_PER_UPPER_THRESHOLD 87

/**
Command ID to adjust BA window size when operating in Station mode.

@note1hang This parameter can be used with qapi_WLAN_Get_Param().

@param[in] qapi_WLAN_BA_Window_Params_t  Set BA window size.

@sa
#qapi_WLAN_BA_Window_Params_t
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_BA_WINDOW 88

/**
Command ID to adjust BA window size when operating in Station mode.

@note1hang This parameter can be used with qapi_WLAN_Get_Param().

@param[in] uint32_t  change slot time to 9us/20us.
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_SLOT_TIME 89

/**
Command ID to adjust EDCCA threshold when operating in Station mode.

@note1hang This parameter can be used with qapi_WLAN_Get_Param().

Set EDCCA threshold to 0-100.
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_EDCCA_THRESHOLD 90

/**
Command ID to set rsp rate in Station mode. The set
operation for this should be done after establishing a connection.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().

@param[in] uint8_t  RspRate idx, only support 8:11g 6Mbps 16:11n 6.5Mbps.
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_RSP_RATE 91

/**
Command ID to adjust BA window size when operating in Station mode. 

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().

@param[in] qapi_WLAN_BA_Window_Size_t  BA window size.
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_BA_WINDOW_SIZE 92

/**
Command ID to set protection mode when operating in Station mode.

@note1hang This parameter can be used with qapi_WLAN_Set_Param().

@param[in] uint32_t        Set 1 to enable CTS_TO_SELF protection, 0 to be disabled.
*/
#define __QAPI_WLAN_PARAM_GROUP_WIRELESS_PROTECTION_MODE 93

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
#define __QAPI_WLAN_PARAM_GROUP_SECURITY_ENCRYPTION_TYPE 1

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
#define __QAPI_WLAN_PARAM_GROUP_SECURITY_PASSPHRASE 3

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
#define __QAPI_WLAN_PARAM_GROUP_SECURITY_WPS_CREDENTIALS 6

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
@ingroup qapi_wlan
Set PMKID.\n

@sa
__QAPI_WLAN_PARAM_GROUP_SECURITY_PMKID
qapi_WLAN_Set_Param
*/
#define __QAPI_WLAN_PARAM_GROUP_SECURITY_PMKID          1001

/**
Command ID to configure P2P device parameters.
Use this parameter with group ID #__QAPI_WLAN_PARAM_GROUP_P2P to call qapi_WLAN_Set_Param().

@param[in] qapi_WLAN_P2P_Config_Params_t   Device parameters to be configured.

@dependencies
P2P mode must be enabled before device parameter configuration.

@sa
#qapi_WLAN_P2P_Config_Params_t \n
qapi_WLAN_Set_Param() \n
qapi_WLAN_P2P_Enable()
*/
#define __QAPI_WLAN_PARAM_GROUP_P2P_CONFIG_PARAMS 0

/**
Command ID to configure P2P opportunistic power save parameters.
Use this parameter with group ID #__QAPI_WLAN_PARAM_GROUP_P2P to call qapi_WLAN_Set_Param().

@param[in] qapi_WLAN_P2P_Opps_Params_t   Opportunistic power save parameters.

@dependencies
Opportunistic power save mode applies only to P2P group owners. A device can initiate
an autonomous group owner operation or it can become
a group owner through a group owner negotiation process.

@sa
qapi_WLAN_P2P_Opps_Params_t \n
qapi_WLAN_Set_Param() \n
qapi_WLAN_P2P_Connect() \n
qapi_WLAN_P2P_Start_Go()
*/
#define __QAPI_WLAN_PARAM_GROUP_P2P_OPPS_PARAMS 1

/**
Command ID to configure P2P notice of absence (NOA) parameters.
Use this parameter with group ID #__QAPI_WLAN_PARAM_GROUP_P2P to call qapi_WLAN_Set_Param().

@param[in] qapi_WLAN_P2P_Noa_Params_t   Notice of absence configuration parameters.

@dependencies
NOA applies only to P2P group owners. A device can initiate autonomous group owner
operation or it can become a group owner through a group
owner negotiation process.

@sa
qapi_WLAN_P2P_Noa_Params_t \n
qapi_WLAN_Set_Param() \n
qapi_WLAN_P2P_Connect() \n
qapi_WLAN_P2P_Start_Go()
*/
#define __QAPI_WLAN_PARAM_GROUP_P2P_NOA_PARAMS 2

/**
Command ID to get a list of P2P peer devices (/nodes) found through the P2P find phase.
Use this parameter with group ID #__QAPI_WLAN_PARAM_GROUP_P2P to call qapi_WLAN_Get_Param().

@param[in, out] qapi_WLAN_P2P_Node_List_Params_t   List to get peer device information.

@dependencies
Nearby devices must be discovered using qapi_WLAN_P2P_Find() before the node list can show those.

@sa
qapi_WLAN_P2P_Node_List_Params_t
qapi_WLAN_Get_Param() \n
qapi_WLAN_P2P_Find()
*/
#define __QAPI_WLAN_PARAM_GROUP_P2P_NODE_LIST 3

/**
Command ID to get a list of P2P groups stored in the nonvolatile storage. This list is persistent across reboots.
Use this parameter with group ID #__QAPI_WLAN_PARAM_GROUP_P2P to call qapi_WLAN_Get_Param().

@param[in, out] qapi_WLAN_P2P_Network_List_Params_t   List to get persistent connections information.

@dependencies
Persistent groups should be formed by using the 'persistent' option while connecting to a peer device and/or
authenticating a peer device for a connection. Without this option, the connections will be lost when the
device reboots.

@sa
qapi_WLAN_P2P_Network_List_Params_t \n
qapi_WLAN_Get_Param() \n
qapi_WLAN_P2P_Connect() \n
qapi_WLAN_P2P_Auth()
*/
#define __QAPI_WLAN_PARAM_GROUP_P2P_NETWORK_LIST 4

/**
Command ID to configure a device listen channel that is used for the device to be discoverable.

The listen channel should be one of the social channels (1, 6, 11 for 2.4 Ghz) and it should remain the same until the
device discovery completes. Use this parameter with group ID #__QAPI_WLAN_PARAM_GROUP_P2P to call qapi_WLAN_Set_Param().
Alternately, applications can use an object of structure qapi_WLAN_P2P_Set_Cmd_t with this macro as 'config_Id' and
'listen_Channel' members of union 'val' as data to call qapi_WLAN_Set_Param().

@param[in] qapi_WLAN_P2P_Listen_Channel_t   Listen channel information.

@sa
qapi_WLAN_P2P_Listen_Channel_t \n
qapi_WLAN_P2P_Set_Cmd_t \n
qapi_WLAN_Set_Param()
*/
#define __QAPI_WLAN_PARAM_GROUP_P2P_LISTEN_CHANNEL 6

/**
Command to configure the postfix to be appended to the P2P group SSID.

Use this parameter with group ID #__QAPI_WLAN_PARAM_GROUP_P2P to call qapi_WLAN_Set_Param().
Alternately, applications can use an object of structure qapi_WLAN_P2P_Set_Cmd_t with this macro
as 'config_Id' and 'ssid_Postfix' members of
union 'val' as data to call qapi_WLAN_Set_Param().

@param[in] qapi_WLAN_P2P_Set_Ssid_Postfix_t   SSID postfix information.

@dependencies
Only a group owner can add an SSID postfix.

@sa
qapi_WLAN_P2P_Set_Ssid_Postfix_t \n
qapi_WLAN_P2P_Set_Cmd_t \n
qapi_WLAN_Set_Param()
*/
#define __QAPI_WLAN_PARAM_GROUP_P2P_SSID_POSTFIX 8

/**
Command ID to enable/disable intra BSS data forwarding support.

Use this parameter with group ID #__QAPI_WLAN_PARAM_GROUP_P2P to call qapi_WLAN_Set_Param().
Alternately, applications can use an object of structure qapi_WLAN_P2P_Set_Cmd_t with this macro
as 'config_Id' and 'intra_Bss' members of
union 'val' as data to call qapi_WLAN_Set_Param().

@param[in] qapi_WLAN_P2P_Set_Intra_Bss_t   Flag to enable/disable intra BSS data forwarding support.

@sa
qapi_WLAN_P2P_Set_Intra_Bss_t \n
qapi_WLAN_P2P_Set_Cmd_t \n
qapi_WLAN_Set_Param()
*/
#define __QAPI_WLAN_PARAM_GROUP_P2P_INTRA_BSS 9

/**
Command ID to configure a device's group owner intent.

Use this parameter with group ID __QAPI_WLAN_PARAM_GROUP_P2P to call qapi_WLAN_Set_Param().
Alternately, applications can use an object of structure qapi_WLAN_P2P_Set_Cmd_t with this
macro as 'config_Id' and 'go_Intent' members of
union 'val' as data to call qapi_WLAN_Set_Param().

@param[in] qapi_WLAN_P2P_Set_Go_Intent_t   Device's group owner intent.

@sa
qapi_WLAN_P2P_Set_Go_Intent_t \n
qapi_WLAN_P2P_Set_Cmd_t \n
qapi_WLAN_Set_Param()
*/
#define __QAPI_WLAN_PARAM_GROUP_P2P_GO_INTENT 11

/**
Command ID to configure a device name.

Use this parameter with group ID #__QAPI_WLAN_PARAM_GROUP_P2P to call qapi_WLAN_Set_Param().
Alternately, applications can use an object of structure qapi_WLAN_P2P_Set_Cmd_t with this
macro as 'config_Id' and 'device_Name' members of
union 'val' as data to call qapi_WLAN_Set_Param().

@param[in] qapi_WLAN_P2P_Set_Dev_Name_t   Device name to be configured.

@sa
qapi_WLAN_P2P_Set_Dev_Name_t \n
qapi_WLAN_P2P_Set_Cmd_t \n
qapi_WLAN_Set_Param()
*/
#define __QAPI_WLAN_PARAM_GROUP_P2P_DEV_NAME 12

/**
Command ID to configure a device's P2P operating mode.

Use this parameter with group ID #__QAPI_WLAN_PARAM_GROUP_P2P to call qapi_WLAN_Set_Param()
to set the device operation. It is used only when the mode to be set is
__QAPI_WLAN_P2P_CLIENT before mode to client before calling qapi_WLAN_P2P_Join().
Alternately, applications can use an object of structure qapi_WLAN_P2P_Set_Cmd_t with this
macro as 'config_Id' and 'mode' members of
union 'val' as data to call qapi_WLAN_Set_Param().

@param[in] qapi_WLAN_P2P_Set_Mode_t   Operating mode.

@sa
__QAPI_WLAN_P2P_CLIENT \n
qapi_WLAN_P2P_Set_Mode_t \n
qapi_WLAN_P2P_Set_Cmd_t \n
qapi_WLAN_Set_Param()
*/
#define __QAPI_WLAN_PARAM_GROUP_P2P_OP_MODE 13

/**
Command ID to enable/disable a complementary code keying (CCK) modulation scheme.

Use this parameter with group ID #__QAPI_WLAN_PARAM_GROUP_P2P to call qapi_WLAN_Set_Param().
Alternately, applications can use an object of structure qapi_WLAN_P2P_Set_Cmd_t with this
macro as 'config_Id' and 'cck_Rates' members of
union 'val' as data to call qapi_WLAN_Set_Param().

@param[in] qapi_WLAN_P2P_Set_Cck_Rates_t   Enable/disable a CCK modulation scheme.

@sa
qapi_WLAN_P2P_Set_Cck_Rates_t \n
qapi_WLAN_P2P_Set_Cmd_t \n
qapi_WLAN_Set_Param()
*/
#define __QAPI_WLAN_PARAM_GROUP_P2P_CCK_RATES 14

/**
Command ID to set P2P discoverable interval.
Use this parameter with group ID #__QAPI_WLAN_PARAM_GROUP_P2P to call qapi_WLAN_Set_Param().
Alternately, applications can use an object of structure qapi_WLAN_P2P_Set_Cmd_t with this
macro as 'config_Id' and 'discoverable_Interval' members of
union 'val' as data to call qapi_WLAN_Set_Param().
@param[in] qapi_WLAN_P2P_Set_Discoverable_Interval_t   Set P2P discoverable interval in milliseconds.
@sa
qapi_WLAN_P2P_Set_Discoverable_Interval_t \n
qapi_WLAN_P2P_Set_Cmd_t \n
qapi_WLAN_Set_Param()
*/
#define __QAPI_WLAN_PARAM_GROUP_P2P_DISCOVERABLE_INTERVAL 15

/**
Command ID used for calling qapi_WLAN_Set_Param() to configure P2P group owner parameters.
*/
#define __QAPI_WLAN_PARAM_GROUP_P2P_GO_PARAMS 5

/** @cond */
/**
Use this macro to call qapi_WLAN_Set_Param() to configure P2P cross
connect parameters.

@sa
qapi_WLAN_P2P_Set_Cross_Connect_t
*/
#define __QAPI_WLAN_PARAM_GROUP_P2P_CROSS_CONNECT 7
/**
Set whether a device should operate in concurrent mode.

@note1hang This parameter can only be used with qapi_WLAN_Set_Param().
*/
#define __QAPI_WLAN_PARAM_GROUP_P2P_CONCURRENT_MODE 10
/** @endcond */

#endif /* __QAPI_WLAN_PARAM_GROUP_H__ */
