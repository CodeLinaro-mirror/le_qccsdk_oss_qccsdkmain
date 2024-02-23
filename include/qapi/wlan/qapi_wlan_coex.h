// $QTI_LICENSE_QDN_C$

#ifndef __QAPI_WLAN_COEX_H__
#define __QAPI_WLAN_COEX_H__

/**
 * @file qapi_wlan_coex.h
 *
 * @brief WLAN coex definition
 *
 * @details This section provides APIs, macros definitions, enumerations and data structures
 *          for applications to perform WLAN control operations.
 */

#include "qapi_types.h"
#include "qapi_status.h"

/**
@ingroup qapi_wlan
Enumeration that identifies COEX enable/disable operations.
*/
typedef enum
{
    QAPI_WLAN_COEX_DISABLE_E = 0, /**< Disable coexistence. */
    QAPI_WLAN_COEX_ENABLED_E      /**< Enable coexistence. */
} QAPI_WLAN_Coex_Enable_Disable_e;

/**
@ingroup qapi_wlan
Enumeration that identifies COEX modes.
*/
typedef enum
{
    QAPI_WLAN_COEX_3_WIRE_MODE_E = 0, /**< 3-wire mode: An unslotted mode of coex. */
    QAPI_WLAN_COEX_PTA_MODE_E,        /**< PTA mode: A slotted mode of coex. */
    QAPI_WLAN_COEX_EPTA_MODE_E,       /**< EPTA mode: 3-wire mode interacting with an external PTA device.  */
} QAPI_WLAN_Coex_Mode_e;

/**
@ingroup qapi_wlan
Enumeration that identifies the number of COEX priority levels.
*/
typedef enum
{
    QAPI_WLAN_COEX_NUM_PRIORITY_LEVELS_TWO_E = 0, /**< Two levels of priority; uses one coex signal. */
    QAPI_WLAN_COEX_NUM_PRIORITY_LEVELS_FOUR_E     /**< Four levels of priority; uses two coex signals. */
}QAPI_WLAN_Coex_Num_Priority_Levels_e;

/**
@ingroup qapi_wlan
Enumeration that identifies COEX priority thresholds.
*/
typedef enum
{
    QAPI_WLAN_COEX_WGHT_ONLY_SLAVE_TRAFFIC_GE_PRI_0_E = 0, /**< Coex priority threshold level 0. */
    QAPI_WLAN_COEX_WGHT_ONLY_SLAVE_TRAFFIC_GE_PRI_1_E,     /**< Coex priority threshold level 1. */
    QAPI_WLAN_COEX_WGHT_ONLY_SLAVE_TRAFFIC_GE_PRI_2_E,     /**< Coex priority threshold level 2. */
    QAPI_WLAN_COEX_WGHT_ONLY_SLAVE_TRAFFIC_GE_PRI_3_E,     /**< Coex priority threshold level 3. */
}QAPI_WLAN_Coex_Priority_Level_Threshold_e;

/**
@ingroup qapi_wlan
Enumeration that identifies the COEX priority.
*/
typedef enum
{
    QAPI_WLAN_COEX_PRIORITY_LOW_E = 0,  /**< Low priority. */
    QAPI_WLAN_COEX_PRIORITY_HIGH_E      /**< High priority. */
}QAPI_WLAN_Coex_Wlan_Priority_e;

/**
@ingroup qapi_wlan
Enum values for the profileType field of the qapi_WLAN_Coex_Stats_t struct.
Enum values for the coex_Profile field of the qapi_WLAN_Coex_Config_Data_t struct.
*/
typedef enum {
    QAPI_BT_PROFILE_UNDEF = 0,
    QAPI_BT_PROFILE_SCO,             /**< SCO stream. */
    QAPI_BT_PROFILE_A2DP,            /**< A2DP stream. */
    QAPI_BT_PROFILE_SCAN,            /**< BT discovery or page. */
    QAPI_BT_PROFILE_ESCO,            /**< ESCO stream. */
    QAPI_BT_PROFILE_HID,             /**< HID stream. */
    QAPI_BT_PROFILE_PAN,             /**< PAN stream -- Reserved. */
    QAPI_BT_PROFILE_RFCOMM,          /**< RFCOMM stream -- Reserved. */
    QAPI_BT_PROFILE_LE,              /**< BLE/15.4 stream. */
    QAPI_BT_PROFILE_SDP,             /**< SDP stream -- Reserved. */
    QAPI_BT_PROFILE_PAGESCAN,        /**< PAGE/SCAN stream -- Reserved. */
    QAPI_BT_PROFILE_HIGH_CMD,        /**< Yield to all narrowband -- Reserved. */
    QAPI_BT_PROFILE_802_15_4,        /**< 15.4 stream -- Reserved. */
    QAPI_BT_PROFILE_MESH,            /**< Mesh configuration -- Reserved. */
	QAPI_BT_PROFILE_MAX
} qapi_Bt_Profile_e;

/**
@ingroup qapi_wlan
Enum values for the coex_Antenna_Config field of the qapi_WLAN_Coex_Config_Data_t struct.
*/
typedef enum {
    QAPI_BT_FE_ANT_NOT_ENABLED = 0,
    QAPI_BT_FE_ANT_SINGLE = 1,        /**< Single, shared antenna: WLAN and narrowband share the same antenna. */
    QAPI_BT_FE_ANT_DUAL = 2,          /**< Dual antennas: WLAN and narrowband use separate antennas. */
    QAPI_BT_FE_ANT_DUAL_HIGH_ISO = 3, /**< Dual antennas: WLAN and narrowband use separate antennas with high isolation. */
    QAPI_BT_FE_ANT_TYPE_MAX
} qapi_Bt_Fe_Ant_t;

/**
@ingroup qapi_wlan
Data structure for WLAN coex conguration parameters.
*/
typedef struct
{
    QAPI_WLAN_Coex_Enable_Disable_e enable_Disable_Coex;           /**< Enable/disable COEX. */
    QAPI_WLAN_Coex_Mode_e coex_Mode;                               /**< 3-wire, PTA or EPTA mode. */
    uint32_t coex_Profile_Specific_Param1;                          /**< 1st Additional parameter value related to the profile specified. */
    uint32_t coex_Profile;                                         /**< Narrowband profile in use. */
    uint8_t coex_Profile_Specific_Param2;                          /**< 2nd Additional parameter value related to the profile specified. */
    uint8_t coex_Profile_Specific_Param3;                          /**< 3rd Additional parameter value related to the profile specified. */
    uint16_t reserved;
} qapi_WLAN_Coex_Config_Data_t;

/**
@ingroup qapi_wlan
Data structure for WLAN coex SCO operational mode conguration parameters.
*/
typedef struct
{
    uint32_t scoSlots;
    /**< Number of SCO Tx/Rx slots. \n
         HVx, EV3, 2EV3 = 2. */

    uint32_t scoIdleSlots;
    /**< Number of Bluetooth idle slots between
         consecutive SCO Tx/Rx slots. \n
         HVx, EV3 = 4; 2EV3 = 10. */

    uint32_t scoFlags;
    /**< SCO options flag bits:
         - 0 -- Allow close range optimization
         - 1 -- Is EDR is capable
         - 2 -- Is a colocated BT role master
         - 3 -- Firmware determines the periodicity of SCO
         - 4 -- No stomping BT during WLAN scan/connection @vertspace{-14} */

    uint32_t linkId;
    /**< Reserved. */

    uint32_t  scoCyclesForceTrigger;
    /**< Number of SCO cycles after which
         a PS-poll is forced; \n
         default = 10. */

    uint32_t scoDataResponseTimeout;
    /**< Timeout waiting for a downlink packet
         in response to a PS-poll; default = 20 ms. */

    uint32_t scoStompDutyCyleVal;
    /**< Reserved. */

    uint32_t scoStompDutyCyleMaxVal;
    /**< Reserved. */

    uint32_t scoPsPollLatencyFraction;
    /**< Fraction of an idle period within which
         additional PS-polls can be queued:
         - 1 -- 1/4 of idle duration
         - 2 -- 1/2 of idle duration
         - 3 -- 3/4 of idle duration
         - Default = 2 (1/2) @vertspace{-14} */

    uint32_t scoStompCntIn100ms;
    /**< Maximum number of SCO stomps in 100 ms allowed in
         Opt mode (force awake). If this exceeds the configured value,
         switch to PS-poll mode; default = 3. */

    uint32_t scoContStompMax;
    /**< Maximum number of continuous stomps allowed in Opt mode (force awake).
         If exceeded, switch to PS-poll mode; default = 3. */

    uint32_t scoMinlowRateMbps;
    /**< Low rate threshold. */

    uint32_t scoLowRateCnt;
    /**< Number of low rate packets (< scoMinlowRateMbps) allowed in 100 ms.
         If exceeded, switch to PS-poll mode or, if lower, stay in Opt mode;
         default = 36. */

    uint32_t scoHighPktRatio;
    /**< Total Rx packets in 100 ms + 1 or
         ((Total Tx packets in 100 ms - the number of high rate packets in 100 ms) + 1) in 100 ms.
         If exceeded, switch to or stay in Opt mode (force awake) and, if lower, switch to or stay in
         PS-poll mode; default = 5 (80% of high rates). */

    uint32_t scoMaxAggrSize;
    /**< Maximum number of Rx subframes allowed in this mode. The firmware renegogiates
         the maximum number of aggregates if it was negotiated to a higher value;
         default = 1. \n
         The recommended value basic rate for headsets = 1, EDR (2-EV3) = 4. */
    uint32_t NullBackoff;
    /**< Number of microseconds the NULL frame should go out before the next SCO slot. */

    uint32_t scanInterval;
    uint32_t maxScanStompCnt;
} qapi_WLAN_Coex_Sco_Config_t;

/**
@ingroup qapi_wlan
Data structure for WLAN coex A2DP operational mode conguration parameters.
*/
typedef struct
{
    uint32_t a2dpFlags;
    /**< A2DP option flags:
         - Bit 0 -- Allow close range optimization
         - Bit 1 -- Is EDR capable
         - Bit 2 -- Is a colocated BT role master
         - Bit 3 -- A2DP traffic is high priority
         - Bit 4 -- Firmware detects the role of Bluetooth
         - Bit 5 -- No stomping BT during WLAN scan/connection @vertspace{-14}  */
    uint32_t linkId;
    /**< Reserved. */

    uint32_t a2dpWlanMaxDur;
    /**< Maximum time the firmware uses the medium for
         WLAN after it identifies the idle time;
         default = 30 ms. */

    uint32_t a2dpMinBurstCnt;
    /**< Minimum number of Bluetooth data frames
         to replenish the WLAN usage limit; default = 3. */

    uint32_t a2dpDataRespTimeout;
    /**< Maximum duration the firmware waits for a downlink
         by stomping on Bluetooth
         after PS-poll is acknowledged;
         default = 20 ms. */

    uint32_t a2dpMinlowRateMbps;
    /**< Low rate threshold. */

    uint32_t a2dpLowRateCnt;
    /**< Number of low rate packets (< a2dpMinlowRateMbps) allowed in 100 ms.
         If exceeded, switch to or stay in PS-poll mode; if lower, stay in Opt mode (force awake);
         default = 36. */

    uint32_t a2dpHighPktRatio;
    /**< (Total Rx pkts in 100 ms + 1)/
         ((Total Tx pkts in 100 ms - No of high rate pkts in 100 ms) + 1) in 100 ms.
         If exceeded, switch to or stay in Opt mode (force awake) and, if lower, switch to or stay in PS-poll mode;
         default = 5 (80% of high rates). */

    uint32_t a2dpMaxAggrSize;
    /**< Maximum number of Rx subframes allowed in this mode. The firmware renegogiates
         the maximum number of aggregates if it was negogiated to a higher value;
         default = 1. \n
         The recommended value for basic rate headsets = 1; EDR (2-EV3) = 8. */

    uint32_t a2dpPktStompCnt;
    /**< Number of A2DP packets that can be stomped per burst; \n
         default = 6. */
} qapi_WLAN_Coex_A2dp_Config_t;

/**
@ingroup qapi_wlan
Data structure for WLAN coex ACL operational mode conguration parameters.
*/
typedef struct {
    uint32_t aclWlanMediumDur;
    /**< WLAN usage time during ACL (non-A2DP)
         coexistence; default = 30 ms. */

    uint32_t aclBtMediumDur;
    /**< BT usage time during ACL coexistence;
         default = 30 ms. */

    uint32_t aclDetectTimeout;
    /**< BT activity observation time limit.
         In this time duration, the number of BT packets are counted.
         If the count reaches the "aclPktCntLowerLimit" value
         for the "aclIterToEnableCoex" iteration continuously,
         the firmware goes into ACL Coexistence mode.
         Similarly, if the BT traffic count during ACL coexistence
         has not reached "aclPktCntLowerLimit" continuously
         for "aclIterToEnableCoex", ACL coexistence is
         disabled; default = 100 ms. */

     uint32_t aclPktCntLowerLimit;
     /**< ACL packet count to be received in the duration of
          "aclDetectTimeout" for
          "aclIterForEnDis" times to enable ACL coex.
          Similar logic is used to disable ACL coexistence.
          (If the "aclPktCntLowerLimit" count of ACL packets
          is not seen by "aclIterForEnDis",
          ACL coexistence is disabled);
          default = 10. */

     uint32_t aclIterForEnDis;
     /**< Number of iterations of "aclPktCntLowerLimit" for enabling and
          disabling ACL coexistence;
          default = 3. */

     uint32_t aclPktCntUpperLimit;
     /**< Upper bound limit; if there is more than
          "aclPktCntUpperLimit" seen in "aclDetectTimeout",
          ACL coexistence is enabled immediately;
          default = 15. */

    uint32_t aclCoexFlags;
    /**< A2DP option flags:
         - Bit 0 -- Allow close range optimization
         - Bit 1 -- Disable firmware detection
         - Bit 2 -- No stomping BT during WLAN scan/connection
         (The currently supported configuration is aclCoexFlags = 0). */

    uint32_t linkId;
    /**< Reserved. */

    uint32_t aclDataRespTimeout;
    /**< Maximum duration the firmware waits for a downlink
         by stomping on Bluetooth
         after PS-poll is acknowledged; \n
         default = 20 ms. */

    uint32_t aclCoexMinlowRateMbps;
    /**< Reserved. */
    uint32_t aclCoexLowRateCnt;
    /**< Reserved. */
    uint32_t aclCoexHighPktRatio;
    /**< Reserved. */
    uint32_t aclCoexMaxAggrSize;
    /**< Reserved. */
    uint32_t aclPktStompCnt;
    /**< Reserved. */
} qapi_WLAN_Coex_Acl_Config_t;

/**
@ingroup qapi_wlan
Data structure for WLAN coex inquiry/page scan operational mode conguration parameters.
*/
typedef struct {
    uint32_t btInquiryDataFetchFrequency;
    /**< Frequency of querying the AP for data
         (via PS-poll) is configured by this parameter;
         default = 10 ms. */

    uint32_t protectBmissDurPostBtInquiry;
    /**< Firmware will continue to be in an inquiry state
         for the configured duration after inquiry completion.
         This is to ensure other Bluetooth transactions
         (RDP, SDP profiles, link key exchange, etc.)
         go through smoothly without Wi-Fi stomping;
         default = 10 secs. */

    uint32_t maxpageStomp;
    /**< Reserved. */
    uint32_t btInquiryPageFlag;
    /**< Reserved. */
} qapi_WLAN_Coex_InqPage_Config_t;

/**
@ingroup qapi_wlan
Data structure for WLAN coex HID operational mode conguration parameters.
*/
typedef struct {
    uint32_t hidFlags;
    /**< HID option flags:
         - Bit 0 -- Allow close range optimization
         - Bit 4 -- No stomping BT during WLAN scan/connection @vertspace{-14} */

    uint32_t hiddevices;
    /**< Current device number of the HID. */

    uint32_t maxStompSlot;
    /**< Maximum stomped. */

    uint32_t aclPktCntLowerLimit;
    /**< ACL detect when HID is on. */

    uint32_t reserved[2];
    /**< Reserved. */

    uint32_t hidWlanMaxDur;
    /**< Reserved. */

    uint32_t hidMinBurstCnt;
    /**< Reserved. */

    uint32_t hidDataRespTimeout;
    /**< Reserved. */

    uint32_t hidMinlowRateMbps;
    /**< Reserved. */

    uint32_t hidLowRateCnt;
    /**< Reserved. */

    uint32_t hidHighPktRatio;
    /**< Reserved. */

    uint32_t hidMaxAggrSize;
    /**< Reserved. */

    uint32_t hidPktStompCnt;
    /**< Reserved. */
} qapi_WLAN_Coex_Hid_Config_t;


/**
@ingroup qapi_wlan
Data structure for the WLAN coex general statistics portion of a coex stats query.
*/
/* Used for firmware development and debugging*/
typedef struct {
    uint32_t highRatePktCnt;
    /**< Count of high rate (typically > 36 Mbps) WLAN packets received.
         Used to assess the impact of narrowband coex on WLAN performance. */
    uint32_t firstBmissCnt;
    /**< Count of first beacon misses. Used to assess whether narrowband is
         causing WLAN to miss beacons and to help debug a WLAN loss of
         connection with the AP. */
    uint32_t BmissCnt;
    /**< Overall count of beacon misses. Used to assess whether narrowband
         is causing WLAN to miss beacons and to help debug a WLAN loss of
         connection with the AP. */
    uint32_t psPollFailureCnt;
    /**< Count of failures to issue PS-poll to the AP for traffic  shaping purposes.
         Used to debug situations where traffic shaping is in use. Traffic
         shaping is primarily used when SCO/ESCO and A2DP profiles are active. */
    uint32_t nullFrameFailureCnt;
    /**< Count of failures to issue a NULL frame to the AP for traffic shaping purposes.
         Used to debug situations where traffic shaping is in use. Traffic shaping
         is primarily used when SCO/ESCO and A2DP profiles are active. */
    uint32_t stompCnt;
    /**< Count of the times WLAN denies a narrowband request to access to the medium.
         This includes instances where narrowband is currently accessing the medium
         and WLAN asserts its DENY signal, effectively canceling the ongoing narrowband
         transaction. */
} qapi_Coex_General_Stats_t;
/**
@ingroup qapi_wlan
Data structure for the WLAN coex SCO statistics portion of a coex stats query. This is a union member that
is selected based on the value returned in the profileType field of the #qapi_WLAN_Coex_Stats_t struct.
*/
typedef struct {
    uint32_t    scoStompCntAvg;         /**< SCO stomp count average. */
    uint32_t    scoStompIn100ms;        /**< SCO stomp count in 100 ms. */
    uint32_t    scoMaxContStomp;        /**< SCO maximum continuous stomp count. */
    uint32_t    scoAvgNoRetries;        /**< SCO average number of retries. */
    uint32_t    scoMaxNoRetriesIn100ms; /**< SCO maximum number of retries in 100 ms. */
} qapi_Coex_Sco_Stats_t;

/**
@ingroup qapi_wlan
Data structure for the WLAN coex A2DP statistics portion of a coex stats query. This is a union member that
is selected based on the value returned in the profileType field of the #qapi_WLAN_Coex_Stats_t struct.
*/
typedef struct {
    uint32_t    a2dpBurstCnt;           /**< A2DP burst count. */
    uint32_t    a2dpMaxBurstCnt;        /**< A2DP maximum burst count. */
    uint32_t    a2dpAvgIdletimeIn100ms; /**< A2DP average idle time in 100 ms. */
    uint32_t    a2dpAvgStompCnt;        /**< A2DP average stomp count. */
} qapi_Coex_A2dp_Stats_t;

/**
@ingroup qapi_wlan
Data structure for that WLAN coex ACL statistics portion of a coex stats query. This is a union member that
is selected based on the value returned in the profileType field of the #qapi_WLAN_Coex_Stats_t struct.
*/
typedef struct {
    uint32_t    aclPktCntInBtTime;     /**< ACL packet count in BT time. */
    uint32_t    aclStompCntInWlanTime; /**< ACL stomp count in WLAN time. */
    uint32_t    aclPktCntIn100ms;      /**< ACL packet count in 100 ms. */
} qapi_Coex_Aclcoex_Stats_t;


/**
@ingroup qapi_wlan
Data structure used to retrieve WLAN coex statistics from the driver. The application
should pass data structure of this type to get the stats information by calling
qapi_WLAN_Get_Param() with command ID as __QAPI_WLAN_PARAM_GROUP_WIRELESS_COEX_STATS.
The member of statsU is selected based on the value returned in the profileType field.
*/
typedef struct
{
    uint32_t resetStats;  /**< Reset stats. */
    struct {
        qapi_Bt_Profile_e profileType;  /**< Selects the statsU union member. */
        qapi_Coex_General_Stats_t generalStats;   /**< General statistics. */
        /** Union of coex stats, selected based on the profileType field. */
        union {
            qapi_Coex_Sco_Stats_t scoStats;         /**< SCO stats. */
            qapi_Coex_A2dp_Stats_t a2dpStats;       /**< A2DP stats. */
            qapi_Coex_Aclcoex_Stats_t aclCoexStats; /**< ACL coex stats. */
        } statsU; /**< Stats union. */
    } coex_Stats_Data; /**< Coex stats data. */
} qapi_WLAN_Coex_Stats_t;

/**
@ingroup qapi_wlan
Data structure for setting WLAN coex weight table entries. If the "enable" field is
0, the rest of the values in the data structure are ignored.
*/
typedef struct {
    uint32_t enable;       /**< Enable. */
    uint32_t btWghtVal[8]; /**< BT weighted value. */
    uint32_t wlWghtVal[4]; /**< WLAN weighted value. */
} qapi_WLAN_Coex_Override_Wghts_t;

/**
@ingroup qapi_wlan
Enum values for the coex_Antenna_Config field of the qapi_WLAN_Coex_Config_Data_t structure.
*/
typedef enum {
    QAPI_COEX_CONFIG_WHGTS_OVERRIDE,
    QAPI_COEX_CONFIG_ONCHIP,
    QAPI_COEX_CONFIG_EPTA_PARAMS,
    QAPI_COEX_CONFIG_EPTA,
    QAPI_COEX_CONFIG_MAX
} qapi_WLAN_Coex_Config_t;

/**
@ingroup qapi_wlan
Data structure for caching an in-use WLAN coex configuration over Suspend/Resume.
The member of configU is selected based on the value returned in the profileType field.
*/
typedef struct {
    qapi_WLAN_Coex_Override_Wghts_t whgts_override;        /**< Cached weight table entry overrides. */
    qapi_WLAN_Coex_Config_Data_t onchip_narrowband_config; /**< Cached on-chip narrowband coex configuration parameters. */
    qapi_Bt_Profile_e profileType;                         /**< 0: no EPTA profile in use; nonzero: selectes the configU member. */
    union {
        qapi_WLAN_Coex_Sco_Config_t sco_config;            /**< Cached EPTA SCO configuration parameters. */
        qapi_WLAN_Coex_A2dp_Config_t a2dp_config;          /**< Cached EPTA A2DP configuration parameters. */
        qapi_WLAN_Coex_Acl_Config_t acl_config;            /**< Cached EPTA ACL configuration parameters. */
        qapi_WLAN_Coex_InqPage_Config_t inqpage_config;    /**< Cached EPTA inquiry/page configuration parameters. */
        qapi_WLAN_Coex_Hid_Config_t hid_config;            /**< Cached EPTA HID configuration parameters. */
    } configU;                                             /**< Union of configuration members. */
    qapi_WLAN_Coex_Config_Data_t epta_narrowband_config;   /**< Cached off-chip/EPTA coex configuration parameters. @newpagetable */
} qapi_WLAN_Coex_Current_Config;

/**
@ingroup qapi_wlan
Data structure to store an application-specific callback function and application context in the driver context.
Set the application context set to NULL if not needed.
*/
typedef struct //qapi_WLAN_Callback_Context_s
{
    void *application_Context;              /**< Application context. */
    qapi_WLAN_Callback_t callback_Function; /**< Callback function. */
} qapi_WLAN_Callback_Context_t;

typedef void (* qapi_WLAN_Coex_Callback_t)(uint8_t device_ID,
                                           uint32_t cb_ID,
                                           void *application_Context,
                                           void  *payload,
                                           uint32_t payload_Length);

/**
@ingroup qapi_wlan
Data structure to store an application-specific callback function and application context in the driver context.
Set the application context to NULL if not needed.
*/
typedef struct qapi_WLAN_Coex_Callback_Context_s
{
    void *application_Context;              /**< Application context. */
    qapi_WLAN_Coex_Callback_t callback_Function; /**< Callback function. */
} qapi_WLAN_Coex_Callback_Context_t;

/**
@ingroup qapi_wlan
Enables, configures, and disables coexistence.

@datatypes
#qapi_WLAN_Coex_Config_Data_t

@param[in] data       Pointer to coex configuration data.

@return
QAPI_OK -- Coexistence configuration operation succeeded.     \n
Nonzero value --  Coexistence configuration operation failed.

@dependencies
QCA402x Wi-Fi firmware must be loaded and running.
*/

qapi_Status_t qapi_WLAN_Coex_Control(const qapi_WLAN_Coex_Config_Data_t *data);

/**
@ingroup qapi_wlan
Gets assorted statistics counters from the WLAN coex subsystem. WLAN coex statistics counters are enabled
in the WLAN firmware by default. Setting the WLAN_CoexStats.resetStats field to a nonzero value causes the WLAN
firmware to reset its copy of the counters after returning the current counter values to the host.

@datatypes
#qapi_WLAN_Coex_Stats_t

@param[in] WLAN_CoexStats        Pointer to a WLAN coex statistics counter structure.

@return
QAPI_OK -- Coexistence statistics counters were populated successfully. \n
Nonzero value -- The get coexistence statistics counters operation failed.

@dependencies
QCA402x WLAN firmware must be loaded and running.
*/

qapi_Status_t qapi_Get_WLAN_Coex_Stats(qapi_WLAN_Coex_Stats_t *WLAN_CoexStats);

#endif // __QAPI_WLAN_COEX_H__

