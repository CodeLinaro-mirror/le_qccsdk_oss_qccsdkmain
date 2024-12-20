/*
#Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
#SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/**
 * @file qapi_rtc.h
 *
 * @addtogroup qapi_Core_RTC
 * @{
 *
 * @details QAPIs to manually set and get the time in Julian format or NTP format.
 *
 * @}
 */

#ifndef __QAPI_RTC_H__
#define __QAPI_RTC_H__

/*-------------------------------------------------------------------------
 * Include Files
 *-----------------------------------------------------------------------*/
#include "qapi_status.h"

#define NUM_DAYS_PER_YEAR           365
#define DIFF_SEC_1900_1970         (2208988800UL)


/*-------------------------------------------------------------------------
 * Type Declarations
 *-----------------------------------------------------------------------*/
/**
 *  Time in Julian format.
 */
typedef struct qapi_Time_s {
    uint16_t year;   /**< Year [1980 through 2100]. */
    uint16_t month;  /**< Month of the year [1 through 12]. */
    uint16_t day;    /**< Day of the month [1 through 31]. */
    uint16_t hour;   /**< Hour of the day [0 through 23]. */
    uint16_t minute; /**< Minute of the hour [0 through 59]. */
    uint16_t second; /**< Second of the minute [0 through 59]. */
    uint16_t day_Of_Week; /**< Day of the week [0 through 6] or [Monday through Sunday]. */
} qapi_Time_t;

/**
 *  Time in NTP format.
 */
typedef struct ntp_Time_s {
    uint32_t second; 
    uint32_t frac;
} ntp_Time_t;

/**
 *  Time zone.
 */
typedef struct time_zone_s {
    uint8_t hour; 
    uint8_t min;
	uint8_t add_sub;
} time_zone_t;

/*-------------------------------------------------------------------------
 * Function Declarations
 *-----------------------------------------------------------------------*/

/**
 *  Gets the Julian time.
 *
 * @param[in] tm  Pointer to a buffer to contain the Julian time.
 *
 * @return #QAPI_OK on success, or an code on error.
 */
qapi_Status_t qapi_Core_RTC_Julian_Get(qapi_Time_t * tm);

/**
 *  Sets the Julian time.
 *
 * @param[in] tm  Pointer to a buffer to contain the Julian time.
 *
 * @return #QAPI_OK on success, or an code on error.
 */
qapi_Status_t qapi_Core_RTC_Julian_Set(qapi_Time_t * tm);


/**
 *  Gets the NTP time.
 *
 * @param[in] ms  Pointer to a buffer to contain the NTP time.
 *
 * @return #QAPI_OK on success, or a different code on error.
 */
qapi_Status_t qapi_Core_RTC_NTP_Get(ntp_Time_t *tm);

/**
 *  Sets the NTP time.
 *
 * @param[in] ms  Pointer to a buffer to contain NTP time.
 *
 * @return #QAPI_OK on success, or a different code on error.
 */
qapi_Status_t qapi_Core_RTC_NTP_Set(ntp_Time_t *tm);


/**
 *  Gets the time zone.
 *
 * @param[in] zone  Pointer to a buffer to contain the time zone.
 *
 * @return #QAPI_OK on success, or a different code on error.
 */
qapi_Status_t qapi_Core_Time_Zone_Get(time_zone_t *zone);

/**
 *  Sets the time zone.
 *
 * @param[in] tm  Pointer to a buffer to contain the time zone.
 *
 * @return #QAPI_OK on success, or a different code on error.
 */
qapi_Status_t qapi_Core_Time_Zone_Set(time_zone_t *zone);
#endif /* __QAPI_RTC_H__ */

