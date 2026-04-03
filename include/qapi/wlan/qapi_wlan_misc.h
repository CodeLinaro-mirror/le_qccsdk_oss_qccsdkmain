/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/
// $QTI_LICENSE_QDN_C$

#ifndef __QAPI_WLAN_MISC_H__
#define __QAPI_WLAN_MISC_H__

/**
 * @file qapi_wlan_misc.h
 *
 * @brief WLAN misc definitions
 *
 * @details This section provides APIs, macros definitions, enumerations and data structures
 *          for applications to perform WLAN control operations.
 */

#include "qapi_types.h"
#include "qapi_status.h"
#include "com_utils.h"

#define __QAPI_NETWORK_ID_UNSPECIFIED    0xff

/** @addtogroup qapi_wlan
@{ */

/** Name of first WLAN virtual device, also known as Device 0. */
#define __QAPI_WLAN_DEVICE0_NAME  "wlan0"
/**
Name of second WLAN virtual device, also known as Device 1; currently the
maximum number of supported virtual devices is 2 (Device 0 and Device 1).
*/
#define __QAPI_WLAN_DEVICE1_NAME  "wlan1"

/** Size of the WLAN MAC address in bytes. */
#define __QAPI_WLAN_MAC_LEN                       6
/** Maximum size of the WLAN Service Set Identifier (SSID) in bytes. */
#define __QAPI_WLAN_MAX_SSID_LEN                 32
/** Maximum SSID length (includes ending NULL character) in bytes. */
#define __QAPI_WLAN_MAX_SSID_LENGTH          (32+1)
/** Maximum passphrase length for WPA/WPA2 WLAN networks in bytes. */
#define __QAPI_WLAN_PASSPHRASE_LEN               64
/** Maximum number of profiles supported for preferred network offload operation. */
#define __QAPI_WLAN_PNO_MAX_NETWORK_PROFILES      5
/** Size of an IPv6 address in bytes. */
#define __QAPI_WLAN_IPV6_ADDR_LEN                16
/** Size of an IPv4 address in bytes. */
#define __QAPI_WLAN_IPV4_ADDR_LEN                 4
/**
Maximum length of a Wi-Fi Protected Setup (WPS) PIN.
This length includes the terminating NULL character. */
#define __QAPI_WLAN_WPS_PIN_LEN                   9
/** Maximum size of a WPS key in bytes. */
#define __QAPI_WLAN_WPS_MAX_KEY_LEN              64

/** Maximum number of filters allowed in Promiscuous mode. */
#define __QAPI_WLAN_PROMISC_MAX_FILTER_IDX        3

/** Maximum number of supported channels that can be retrieved from tbe current regulatory domain channel list. */
#define __QAPI_WLAN_MAX_NUM_CUR_REGDOAMIN_CHANLIST_CHNNELS     64

/**
Allows the application to enable/disable a source MAC address based filter when
operating in Promiscuous mode.

This macro should be set in the filter_flags field
of the qapi_WLAN_Promiscuous_Mode_Info_t data structure when issuing
qapi_WLAN_Set_Param() with __QAPI_WLAN_PARAM_GROUP_WIRELESS_ENABLE_PROMISCUOUS_MODE.\n
If this bit is enabled, the application should also set the src_Mac address in the
corresponding filter index of qapi_WLAN_Promiscuous_Mode_Info_t.

@sa
qapi_WLAN_Promiscuous_Mode_Info_t\n
__QAPI_WLAN_PARAM_GROUP_WIRELESS_ENABLE_PROMISCUOUS_MODE\n
qapi_WLAN_Set_Param
*/
#define __QAPI_WLAN_PROM_FILTER_SOURCE         0x01

/**
Allows the application to enable/disable the destination MAC address based filter when
operating in Promiscuous mode.

This macro should be set in the filter_flags field
of the qapi_WLAN_Promiscuous_Mode_Info_t data structure when issuing
qapi_WLAN_Set_Param() with __QAPI_WLAN_PARAM_GROUP_WIRELESS_ENABLE_PROMISCUOUS_MODE.\n
If this bit is enabled, the application should also set the dst_Mac address in the
corresponding filter index of qapi_WLAN_Promiscuous_Mode_Info_t.

@sa
qapi_WLAN_Promiscuous_Mode_Info_t\n
__QAPI_WLAN_PARAM_GROUP_WIRELESS_ENABLE_PROMISCUOUS_MODE\n
qapi_WLAN_Set_Param
*/
#define __QAPI_WLAN_PROM_FILTER_DEST           0x02

/**
Allows the application to enable/disable the 802.11 packet type based filter when
operating in Promiscuous mode.

This macro should be set in the filter_flags field of the qapi_WLAN_Promiscuous_Mode_Info_t
data structure when issuing qapi_WLAN_Set_Param() with
__QAPI_WLAN_PARAM_GROUP_WIRELESS_ENABLE_PROMISCUOUS_MODE.\n
The 802.11 packet type to be matched should be provided in the promisc_frametype field in the corresponding
filter index of qapi_WLAN_Promiscuous_Mode_Info_t.

@sa
qapi_WLAN_Promiscuous_Mode_Info_t\n
__QAPI_WLAN_PARAM_GROUP_WIRELESS_ENABLE_PROMISCUOUS_MODE\n
qapi_WLAN_Set_Param
*/
#define __QAPI_WLAN_PROM_FILTER_FRAME_TYPE     0x04

/**
Allows the application to enable/disable the 802.11 packet subtype based filter when
operating in Promiscuous mode.

This macro should be set in the filter_flags field of the qapi_WLAN_Promiscuous_Mode_Info_t
data structure when issuing qapi_WLAN_Set_Param() with
__QAPI_WLAN_PARAM_GROUP_WIRELESS_ENABLE_PROMISCUOUS_MODE.\n
The 802.11 packet type to be matched should be provided in the promisc_subtype field in the corresponding
filter index of qapi_WLAN_Promiscuous_Mode_Info_t.

@sa
qapi_WLAN_Promiscuous_Mode_Info_t\n
__QAPI_WLAN_PARAM_GROUP_WIRELESS_ENABLE_PROMISCUOUS_MODE\n
qapi_WLAN_Set_Param
*/
#define __QAPI_WLAN_PROM_FILTER_FRAME_SUB_TYPE 0x08

/**
This macro indicates that the data is RSSI information for a frame in promiscuous mode.
When the driver layer reports RSSI information for a frame, this macro is set and the application
should treat the data as RSSI information, not a frame context.
*/
#define __QAPI_WLAN_PROMISC_REPORT_RSSI_INFO   0x4000

/**
This macro indicates that the data is only payload of frame which which has no 802.11 mac header.
For AMSDU packet, it has only one wlan 802.11 mac header and may occupy multiple netbufs so the
driver layer may report several packets to application layer.
If this macro is set, it means the data is only payload and should append to the previous frame.
*/
#define __QAPI_WLAN_PROMISC_INDICATE_APPEND_PAYLOAD 0x8000

/** @cond EXPORT_PKTLOG */
/**
Default number of pktlog buffers used to allocate in a target
when pktlog was enabled if the user did not specify it.
*/
#define __QAPI_WLAN_PKTLOG_NUM_BUFF                  4
/**
Maximum number of pktlog buffers that the user can set to allocate buffers
in a target when pktlog is enabled.
*/
#define __QAPI_WLAN_MAX_PKTLOG_NUM_BUFF              10
/** @endcond */

/** Total number of Dblog modules supported in the target. */
#define __QAPI_WLAN_DBGLOG_MAX_MODULE_ID             64
/**
Number of word length buffers required to store the DBGLOG Loglevel
information for all 64 (maximum supported) modules.
*/
#define __QAPI_WLAN_DBGLOG_LOGLEVEL_INFO_LEN         8
/**
Module mask that can be used to enable all modules with a
desired loglevel.
*/
#define __QAPI_WLAN_DBGLOG_DEFAULT_MODULE_MASK       0xFFFFFFFFFFFFFFFFLL
/**
Global module ID to set the same configuration(loglevel)
for all modulues.
*/
#define __QAPI_WLAN_DBGLOG_GLOBAL_MODULE_ID          0xFF
/**
Default value to capture only the ERROR loglevel for all 64 modules
when dbglog is enabled.
*/
#define __QAPI_WLAN_DBGLOG_DEFAULT_MODULE_LOG_LEVEL  0x88888888
/**
Used to configure to send the dbglogs on the local UART on
the target chip (Kingfisher).
*/
#define __QAPI_WLAN_DBGLOG_LOCAL_DBG_PORT            0
/**
Used to configure to send the dbglogs from the target to the host, which
in turn sends them to its UART, where an external tool captures and parses the
dbglog binary logs.
*/
#define __QAPI_WLAN_DBGLOG_REMOTE_DBG_PORT           1
/**
Flag bit to be used to notify the target when a change in loglevel is needed
during a dbglog configuration update.
*/
#define __QAPI_WLAN_DBGLOG_CFG_LOG_LEVEL_FLAG_MASK         0x01
/**
Flag bit to be used to notify the target when a change in debug port is needed
to use during a dbglog configuration update.
*/
#define __QAPI_WLAN_DBGLOG_CFG_DEBUG_PORT_FLAG_MASK        0x02
/**
Flag bit to be used to notify the target when a change in reporting is
required or not during a dbglog configuration update.
*/
#define __QAPI_WLAN_DBGLOG_CFG_REPORT_FLAG_MASK            0x04
/**
Flag bit to be used to notify the target when a change in
report size is needed during a dbglog configuration update.
*/
#define __QAPI_WLAN_DBGLOG_CFG_REPORT_SIZE_FLAG_MASK       0x08
/**
Flag bit to be used to notify the target when a change in
time resolution is needed during a dbglog configuration update.
*/
#define __QAPI_WLAN_DBGLOG_CFG_TIMER_RESOLUTION_FLAG_MASK  0x10
/**
Flag bit to be used to notify the target to flush log buffer.
*/
#define __QAPI_WLAN_DBGLOG_CFG_FLUSH_FLAG_MASK              0x20
/**
Value for the number of nibbles in a word. Used for dbglog loglevel
update calculation.
*/
#define __QAPI_WLAN_DBGLOG_NIBBLE_CNT_IN_WORD_MEMORY     8
/**
Value for the number of bits in a word memory. Used for dbglog loglevel
update calculation.
*/
#define __QAPI_WLAN_DBGLOG_BIT_CNT_IN_WORD_MEMORY        32

/** IEEE 802.11 channel 1 in MHz. This channel is allowed in USA, Canada, Europe, and Japan. */
#define __QAPI_WLAN_CHAN_FREQ_1            (2412)
/** IEEE 802.11 channel 2 in MHz. This channel is allowed in USA, Canada, Europe, and Japan. */
#define __QAPI_WLAN_CHAN_FREQ_2            (2417)
/** IEEE 802.11 channel 3 in MHz. This channel is allowed in USA, Canada, Europe, and Japan. */
#define __QAPI_WLAN_CHAN_FREQ_3            (2422)
/** IEEE 802.11 channel 4 in MHz. This channel is allowed in USA, Canada, Europe, and Japan. */
#define __QAPI_WLAN_CHAN_FREQ_4            (2427)
/** IEEE 802.11 channel 5 in MHz. This channel is allowed in USA, Canada, Europe, and Japan. */
#define __QAPI_WLAN_CHAN_FREQ_5            (2432)
/** IEEE 802.11 channel 6 in MHz. This channel is allowed in USA, Canada, Europe, and Japan. */
#define __QAPI_WLAN_CHAN_FREQ_6            (2437)
/** IEEE 802.11 channel 7 in MHz. This channel is allowed in USA, Canada, Europe, and Japan. */
#define __QAPI_WLAN_CHAN_FREQ_7            (2442)
/** IEEE 802.11 channel 8 in MHz. This channel is allowed in USA, Canada, Europe, and Japan. */
#define __QAPI_WLAN_CHAN_FREQ_8            (2447)
/** IEEE 802.11 channel 9 in MHz. This channel is allowed in USA, Canada, Europe, and Japan. */
#define __QAPI_WLAN_CHAN_FREQ_9            (2452)
/** IEEE 802.11 channel 10 in MHz. This channel is allowed in USA, Canada, Europe, and Japan. */
#define __QAPI_WLAN_CHAN_FREQ_10           (2457)
/** IEEE 802.11 channel 11 in MHz. This channel is allowed in USA, Canada, Europe, and Japan. */
#define __QAPI_WLAN_CHAN_FREQ_11           (2462)
/** IEEE 802.11 channel 12 in MHz. This channel is allowed in Europe and Japan. */
#define __QAPI_WLAN_CHAN_FREQ_12           (2467)
/** IEEE 802.11 channel 13 in MHz. This channel is allowed in Europe and Japan. */
#define __QAPI_WLAN_CHAN_FREQ_13           (2472)
/** IEEE 802.11 channel 14 in MHz. This channel is allowed in Japan. */
#define __QAPI_WLAN_CHAN_FREQ_14           (2484)
/** IEEE 802.11a channel 36 in MHz. */
#define __QAPI_WLAN_CHAN_FREQ_36           (5180)
/** IEEE 802.11a channel 40 in MHz. */
#define __QAPI_WLAN_CHAN_FREQ_40           (5200)
/** IEEE 802.11a channel 44 in MHz. */
#define __QAPI_WLAN_CHAN_FREQ_44           (5220)
/** IEEE 802.11a channel 48 in MHz. */
#define __QAPI_WLAN_CHAN_FREQ_48           (5240)
/** IEEE 802.11a channel 52 in MHz. */
#define __QAPI_WLAN_CHAN_FREQ_52           (5260)
/** IEEE 802.11a channel 56 in MHz. */
#define __QAPI_WLAN_CHAN_FREQ_56           (5280)
/** IEEE 802.11a channel 60 in MHz. */
#define __QAPI_WLAN_CHAN_FREQ_60           (5300)
/** IEEE 802.11a channel 64 in MHz. */
#define __QAPI_WLAN_CHAN_FREQ_64           (5320)
/** IEEE 802.11a channel 100 in MHz. */
#define __QAPI_WLAN_CHAN_FREQ_100          (5500)
/** IEEE 802.11a channel 104 in MHz. */
#define __QAPI_WLAN_CHAN_FREQ_104          (5520)
/** IEEE 802.11a channel 108 in MHz. */
#define __QAPI_WLAN_CHAN_FREQ_108          (5540)
/** IEEE 802.11a channel 112 in MHz. */
#define __QAPI_WLAN_CHAN_FREQ_112          (5560)
/** IEEE 802.11a channel 116 in MHz. */
#define __QAPI_WLAN_CHAN_FREQ_116          (5580)
/** IEEE 802.11a channel 132 in MHz. */
#define __QAPI_WLAN_CHAN_FREQ_132          (5660)
/** IEEE 802.11a channel 136 in MHz. */
#define __QAPI_WLAN_CHAN_FREQ_136          (5680)
/** IEEE 802.11a channel 140 in MHz. */
#define __QAPI_WLAN_CHAN_FREQ_140          (5700)
/** IEEE 802.11a channel 149 in MHz. */
#define __QAPI_WLAN_CHAN_FREQ_149          (5745)
/** IEEE 802.11a channel 153 in MHz. */
#define __QAPI_WLAN_CHAN_FREQ_153          (5765)
/** IEEE 802.11a channel 157 in MHz. */
#define __QAPI_WLAN_CHAN_FREQ_157          (5785)
/** IEEE 802.11a channel 161 in MHz. */
#define __QAPI_WLAN_CHAN_FREQ_161          (5805)
/** IEEE 802.11a channel 165 in MHz. */
#define __QAPI_WLAN_CHAN_FREQ_165          (5825)
/** 6G channel 1 in MHz. */
#define __QAPI_WLAN_6G_CHAN_FREQ_1         (5955)

/**
Flag that indicates whether a scanned access point's authentication type is of type
Pre-Shared Key.
This is indicated in the wpa_auth field (for WPA APs) and rsn_Auth field (for WPA2 APs)
of the qapi_WLAN_BSS_Scan_Info_t structure.
*/
#define __QAPI_WLAN_SECURITY_AUTH_PSK      0x01

/**
Flag that indicates whether the scanned access point's authentication type is of type
802.1x(EAP based).
This is indicated in the wpa_auth field (for WPA APs) and rsn_Auth field (for WPA2 APs)
of the qapi_WLAN_BSS_Scan_Info_t structure.
*/
#define __QAPI_WLAN_SECURITY_AUTH_1X       0x02

/** 
Flag that indicates whether the scanned access point's authentication type is of type 
SAE.
This is indicated in the wpa_auth field (for WPA APs) and rsn_Auth field (for WPA2 & WPA3 APs)
of the qapi_WLAN_BSS_Scan_Info_t structure.
*/
#define __QAPI_WLAN_SECURITY_AUTH_SAE      0x04

/**
 * Flag that indicates whether the scanned access point's authentication type is of type
 * WPA3-Enterprise / 802.1x (EAP based).
 *
 * This is indicated in the rsn_Auth field of the qapi_WLAN_BSS_Scan_Info_t structure
 * for WPA3 APs.
 *
 * Typical WPA3-Enterprise AKM suite selectors include:
 * - 00-0F-AC:5  : IEEE 802.1X using SHA-256
 * - 00-0F-AC:3  : FT authentication over IEEE 802.1X using SHA-256
 */
#define __QAPI_WLAN_SECURITY_AUTH_WPA3_1X    0x08

/**
 * Flag that indicates whether the scanned access point's authentication type is of type
 * WPA3-Enterprise 192-bit mode.
 *
 * This is indicated in the rsn_Auth field of the qapi_WLAN_BSS_Scan_Info_t structure
 * for WPA3 APs.
 *
 * WPA3-Enterprise 192-bit mode AKM suite selectors:
 * - 00-0F-AC:12 : WPA3-Enterprise 192-bit mode
 */
#define __QAPI_WLAN_SECURITY_AUTH_WPA3_1X_B_192    0x10

/** @cond */
/** Boot parameter. */
#define __QAPI_WLAN_AR4XXX_PARAM_MODE_NORMAL    (0x00000002)
/** Boot parameter. */
#define __QAPI_WLAN_AR4XXX_PARAM_MODE_BMI       (0x00000003)
/** Boot parameter. */
#define __QAPI_WLAN_AR4XXX_PARAM_MODE_MASK      (0x0000000f)
/** Boot parameter. */
#define __QAPI_WLAN_AR4XXX_PARAM_QUAD_SPI_FLASH (0x80000000)
/** Boot parameter. */
#define __QAPI_WLAN_AR4XXX_PARAM_RAWMODE_BOOT   (0x00000010 | __QAPI_WLAN_AR4XXX_PARAM_MODE_NORMAL)
/** Boot parameter. */
#define __QAPI_WLAN_AR4XXX_PARAM_MACPROG_MODE   (0x00000020 | __QAPI_WLAN_AR4XXX_PARAM_MODE_NORMAL)
/** Combined boot parameter for common cases. */
#define __QAPI_WLAN_AR4XXX_PARAM_RAW_QUAD       (__QAPI_WLAN_AR4XXX_PARAM_RAWMODE_BOOT | __QAPI_WLAN_AR4XXX_PARAM_QUAD_SPI_FLASH)
/** Default regulatory domain. This settings can be passed to WLAN firmware during boot. */
#define __QAPI_WLAN_AR4XXX_PARAM_REG_DOMAIN_DEFAULT  (0x00000000)
/** @endcond */

/**
Flag that indicates whether the scanned access point's encryption type is of type
TKIP (Temporal Key Integrity Protocol).
This is indicated in the wpa_Cipher field (for WPA APs) and rsn_Cipher field (for WPA2 APs)
of the qapi_WLAN_BSS_Scan_Info_t structure as part of the scan result indication.
*/
#define __QAPI_WLAN_CIPHER_TYPE_TKIP 0x04

/**
Flag that indicates whether the scanned access point's encryption type is of type CCMP.
This is indicated in the wpa_Cipher field (for WPA APs) and rsn_Cipher field (for WPA2 APs)
of the qapi_WLAN_BSS_Scan_Info_t structure as part of the scan result indication.
*/
#define __QAPI_WLAN_CIPHER_TYPE_CCMP 0x08

/**
Flag that indicates whether the scanned access point's encryption type is of type
WEP (Wired Equivalent Privacy).
This is indicated in the wpa_Cipher field and rsn_Cipher field of the qapi_WLAN_BSS_Scan_Info_t
structure as part of the scan result indication.
*/
#define __QAPI_WLAN_CIPHER_TYPE_WEP  0x02

/** Maximum size of the WEP key. */
#define __QAPI_WLAN_MAX_WEP_KEY_SZ 16

/**
Maximum number of scan results that the driver buffer can hold when using
QAPI_WLAN_BUFFER_SCAN_RESULTS_BLOCKING_E and
QAPI_WLAN_BUFFER_SCAN_RESULTS_NON_BLOCKING_E options to store WLAN scan results.
*/
#define __QAPI_MAX_SCAN_RESULT_ENTRY 40

/**
Macro to indicate the default Short Scan ratio.
*/
#define __QAPI_WLAN_SHORTSCANRATIO_DEFAULT      3

/**
Macro that indicates the maximum number of network solicitation entries that the WLAN FW
can offload.
Every tuple here corresponds to one virtual WLAN device.
*/
#define __QAPI_WLAN_MAX_NS_OFFLOAD_TUPLES           2

/**
Macro that indicates the maximum number of ARP entries that the WLAN FW can offload.
Every tuple corresponds to one virtual WLAN device.
*/
#define __QAPI_WLAN_MAX_ARP_OFFLOAD_TUPLES          2

/**
Macro that indicates the maximum number of network solicitation addresses that the WLAN FW
supports per device (tuple);
one for a global IPv6 address and another for a link local IPv6 address (interchangable).
*/
#define __QAPI_WLAN_NSOFF_MAX_TARGET_IPS            2

/**
Macro that indicates the maximum number of channels that can be issued in a channel
list while scanning.
*/
#define __QAPI_WLAN_START_SCAN_PARAMS_CHANNEL_LIST_MAX   12

/**
Macro that indicates the maximum number of WLAN firmware events that can be filtered
within the firmware.
*/
#define __QAPI_WLAN_MAX_NUM_FILTERED_EVENTS   64

/**
Macro that defines the maximum length of a string for each of the subversion information strings.
*/
#define __QAPI_WLAN_VERSION_SUBSTRING_LEN 20

/**
Macro that indicates the maximum size of patterns supported for packet filtering and
WOW filtering features.

@sa
qapi_WLAN_Add_Pattern_t
*/
#define __QAPI_WLAN_PATTERN_MAX_SIZE 128

/**
Macro that indicates the maximum size of a supported pattern mask.

This is the function of __QAPI_WLAN_PATTERN_MAX_SIZE as a pattern mask, which is
a bitmap that indicates the validity of pattern data where every bit in the pattern
mask corresponds to a byte in the pattern data.

For an application, if a pattern to be matched is not a contiguous byte stream,
the application should set valid bytes to be matched by setting appropriate bits
in the pattern mask member of qapi_WLAN_Add_Pattern_t.

@sa
qapi_WLAN_Add_Pattern_t
*/
#define __QAPI_WLAN_PATTERN_MASK (__QAPI_WLAN_PATTERN_MAX_SIZE/8)

/**
Macro to indicate a filter action mask as a REJECT on finding a matched pattern
during WOW and packet filtering.

If this flag is set in pattern_Action_Flag of qapi_WLAN_Add_Pattern_t for
a given pattern, the received packet that matches this pattern is dropped by the firmware.

@sa
qapi_WLAN_Add_Pattern_t
*/
#define __QAPI_WLAN_PATTERN_REJECT_FLAG 0x1

/**
Macro to indicate a filter action as ACCEPT on finding a matched pattern during
WOW and packet filtering.

If this flag is set in pattern_Action_Flag of qapi_WLAN_Add_Pattern_t for a given pattern,
the received packet that matches this pattern is forwarded to the host by the firmware.

@sa
qapi_WLAN_Add_Pattern_t
*/
#define __QAPI_WLAN_PATTERN_ACCEPT_FLAG 0x2

/**
Macro to indicate a filter action mask as DEFER on finding a matched pattern
during WOW and packet filtering.

If this flag is set in pattern_Action_Flag of qapi_WLAN_Add_Pattern_t for
a given pattern, the received packet that matches this pattern defers to the next header type.

@sa
qapi_WLAN_Add_Pattern_t
*/
#define __QAPI_WLAN_PATTERN_DEFER_FLAG 0x4

/**
Macro to indicate that a given pattern is a WOW pattern.

If pattern_Action_Flag has this bit set and if data is received with the pattern matched,
the firmware wakes up the host by asserting an out-of-band GPIO interrupt.

If this bit is not set in the pattern_Action_Flag of the pattern added, the pattern
is just considered as a packet filter and an out-of-band interrupt will not be asserted
on matching the pattern.

@sa
qapi_WLAN_Add_Pattern_t
*/
#define __QAPI_WLAN_PATTERN_WOW_FLAG 0x80

/** Default pattern index for WOW and packet filtering. */
#define __QAPI_WLAN_DEFAULT_FILTER_INDEX      0

/** Maximum value of the pattern index for WOW and packet filtering. */
#define __QAPI_WLAN_MAX_FILTER_INDEX      8

/** Pattern index to delete all filters in the protocol header. */
#define __QAPI_WLAN_DELETE_ALL_FILTER_INDEX      0xFF

/** Default pattern prioirty for WOW and packet filtering. */
#define __QAPI_WLAN_DEFAULT_FILTER_PRIORITY   0

/**
Macro that indicates that the Tx queue of the WLAN driver is empty. If the driver
returns this on querying the Tx status, the application can safely put the WLAN
subsystem in Suspend mode.
*/
#define __QAPI_WLAN_TX_STATUS_IDLE 0x01
/**
Macro that indicates that the WLAN driver Tx queue has one or more frames waiting to be
transmitted.
If the driver returns this on querying the Tx status, the application should
not proceed with the suspend operation.
*/
#define __QAPI_WLAN_TX_STATUS_HOST_PENDING 0x02
/**
Macro that indicates that the WLAN driver has not completed transmission of all the frames,
even if the Tx queue is empty.
If the driver returns this on querying the Tx status, the application should not proceed
with the suspend operation.
*/
#define __QAPI_WLAN_TX_STATUS_WIFI_PENDING 0x03

/**
Macro that indicates the maximum number of buffers that can be aggregated by the WLAN
subsystem when transmitting an AMPDU frame.
*/
#define __QAPI_WLAN_AGGRX_BUFFER_SIZE_MAX 16

/**
Macro to indicate in the host for the firmware not to consider a particular aggregation parameter.

The firmware considers values other than 0xFF as valid aggregation parameter from the host while
configuring aggregation parameters.
To avoid using invalid aggregation parameters in the target, the host resets all the aggregation
parameters to 0xFF (this macro) before configuring the desired parameters.
*/
#define __QAPI_WLAN_AGGRX_CFG_INVAL       0xFF

/** @cond */
#ifdef WLAN_TESTS
#define __QAPI_WLAN_PARAM_WLAN_TEST_MCC                           0
#endif
/** @endcond */

/** @} */ /* end_addtogroup qapi_wlan */

/**
@ingroup qapi_wlan
Enumumeration that identifies WLAN rates.
*/
typedef enum
{
     QAPI_WLAN_RATE_AUTO_E     = -1, /**< 1 Mbps. */
     QAPI_WLAN_RATE_1Mb_E       = 0, /**< 1 Mbps. */
     QAPI_WLAN_RATE_2Mb_E       = 1, /**< 1 Mbps. */
     QAPI_WLAN_RATE_5_5Mb_E     = 2, /**< 5 Mbps. */
     QAPI_WLAN_RATE_11Mb_E      = 3, /**< 11 Mbps. */
     QAPI_WLAN_RATE_6Mb_E       = 4, /**< 6 Mbps. */
     QAPI_WLAN_RATE_9Mb_E       = 5, /**< 9 Mbps. */
     QAPI_WLAN_RATE_12Mb_E      = 6, /**< 12 Mbps. */
     QAPI_WLAN_RATE_18Mb_E      = 7, /**< 18 Mbps. */
     QAPI_WLAN_RATE_24Mb_E      = 8, /**< 24 Mbps. */
     QAPI_WLAN_RATE_36Mb_E      = 9, /**< 36 Mbps. */
     QAPI_WLAN_RATE_48Mb_E      = 10, /**< 48 Mbps. */
     QAPI_WLAN_RATE_54Mb_E      = 11, /**< 54 Mbps. */
     QAPI_WLAN_RATE_MCS_0_20_E  = 12, /**< MCS rate index 0 for 20 Mhz channel. */
     QAPI_WLAN_RATE_MCS_1_20_E  = 13, /**< MCS rate index 1 for 20 Mhz channel. */
     QAPI_WLAN_RATE_MCS_2_20_E  = 14, /**< MCS rate index 2 for 20 Mhz channel. */
     QAPI_WLAN_RATE_MCS_3_20_E  = 15, /**< MCS rate index 3 for 20 Mhz channel. */
     QAPI_WLAN_RATE_MCS_4_20_E  = 16, /**< MCS rate index 4 for 20 Mhz channel. */
     QAPI_WLAN_RATE_MCS_5_20_E  = 17, /**< MCS rate index 5 for 20 Mhz channel. */
     QAPI_WLAN_RATE_MCS_6_20_E  = 18, /**< MCS rate index 6 for 20 Mhz channel. */
     QAPI_WLAN_RATE_MCS_7_20_E  = 19, /**< MCS rate index 7 for 20 Mhz channel. */
     QAPI_WLAN_RATE_MCS_8_20_E  = 20, /**< MCS rate index 8 for 20 Mhz channel. */
     QAPI_WLAN_RATE_MCS_9_20_E  = 21, /**< MCS rate index 9 for 20 Mhz channel; not supported at this time. */
     QAPI_WLAN_RATE_MCS_10_20_E = 22, /**< MCS rate index 10 for 20 Mhz channel; not supported at this time. */
     QAPI_WLAN_RATE_MCS_11_20_E = 23, /**< MCS rate index 11 for 20 Mhz channel; not supported at this time. */
     QAPI_WLAN_RATE_MCS_12_20_E = 24, /**< MCS rate index 12 for 20 Mhz channel; not supported at this time. */
     QAPI_WLAN_RATE_MCS_13_20_E = 25, /**< MCS rate index 13 for 20 Mhz channel; not supported at this time. */
     QAPI_WLAN_RATE_MCS_14_20_E = 26, /**< MCS rate index 14 for 20 Mhz channel; not supported at this time. */
     QAPI_WLAN_RATE_MCS_15_20_E = 27, /**< MCS rate index 15 for 20 Mhz channel; not supported at this time. */
     QAPI_WLAN_RATE_MCS_0_40_E  = 28, /**< MCS rate index 0 for 40 Mhz channel; not supported at this time. */
     QAPI_WLAN_RATE_MCS_1_40_E  = 29, /**< MCS rate index 1 for 40 Mhz channel; not supported at this time. */
     QAPI_WLAN_RATE_MCS_2_40_E  = 30, /**< MCS rate index 2 for 40 Mhz channel; not supported at this time. */
     QAPI_WLAN_RATE_MCS_3_40_E  = 31, /**< MCS rate index 3 for 40 Mhz channel; not supported at this time. */
     QAPI_WLAN_RATE_MCS_4_40_E  = 32, /**< MCS rate index 4 for 40 Mhz channel; not supported at this time. */
     QAPI_WLAN_RATE_MCS_5_40_E  = 33, /**< MCS rate index 5 for 40 Mhz channel; not supported at this time. */
     QAPI_WLAN_RATE_MCS_6_40_E  = 34, /**< MCS rate index 6 for 40 Mhz channel; not supported at this time. */
     QAPI_WLAN_RATE_MCS_7_40_E  = 35, /**< MCS rate index 7 for 40 Mhz channel; not supported at this time. */
     QAPI_WLAN_RATE_MCS_8_40_E  = 36, /**< MCS rate index 8 for 40 Mhz channel; not supported at this time. */
     QAPI_WLAN_RATE_MCS_9_40_E  = 37, /**< MCS rate index 9 for 40 Mhz channel; not supported at this time. */
     QAPI_WLAN_RATE_MCS_10_40_E = 38, /**< MCS rate index 10 for 40 Mhz channel; not supported at this time. */
     QAPI_WLAN_RATE_MCS_11_40_E = 39, /**< MCS rate index 11 for 40 Mhz channel; not supported at this time. */
     QAPI_WLAN_RATE_MCS_12_40_E = 40, /**< MCS rate index 12 for 40 Mhz channel; not supported at this time. */
     QAPI_WLAN_RATE_MCS_13_40_E = 41, /**< MCS rate index 13 for 40 Mhz channel; not supported at this time. */
     QAPI_WLAN_RATE_MCS_14_40_E = 42, /**< MCS rate index 14 for 40 Mhz channel; not supported at this time. */
     QAPI_WLAN_RATE_MCS_15_40_E = 43, /**< MCS rate index 15 for 40 Mhz channel; not supported at this time. */
     QAPI_WLAN_RATE_MAX_E,
} qapi_WLAN_Bit_Rate_t;

#endif // __QAPI_WLAN_MISC_H__

