/*
#Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
#SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*-------------------------------------------------------------------------
 * Include Files
 *-----------------------------------------------------------------------*/
#include "fwconfig_cmn.h"
#include "nt_flags.h"
#include <string.h>
#include <stdlib.h>
#include "qapi_rtc.h"
#include "nt_common.h"
#include "nt_osal.h"
#include <stdio.h>
#include "timer.h"
#include "nt_hw.h"

/*-------------------------------------------------------------------------
 * Variables
 *-----------------------------------------------------------------------*/
 

uint64_t last_rtc_time = 0;
ntp_Time_t last_ntp_time = {0,0};
uint32_t last_ntp_sec = 0;
uint32_t last_ntp_frac = 0;

time_zone_t g_time_zone = {0,0,0};


extern uint64_t start_tsf_beacon;
extern int64_t rtc_time_padding;
extern int64_t rtc_time_padding_history;
/*-------------------------------------------------------------------------
 * Function Definition
 *-----------------------------------------------------------------------*/

uint16_t get_no_of_days_in_year(uint16_t year)
{
	uint16_t days = NUM_DAYS_PER_YEAR;
	if(((year%4 == 0) && (year%100 != 0)) || (year%400 == 0))
	{
		days += 1;
	}
	return days;	
}

uint16_t get_msec_from_ntp_frac(uint32_t frac)
{
	return frac/4294967;
}

uint32_t get_ntp_frac_from_msec(uint16_t ms)
{
	return ms*4294967;
}

qapi_Status_t rtc_convert_Julian_to_NTP(qapi_Time_t *julian, ntp_Time_t *ntp)
{
	uint32_t t = 0, days = 0;
	uint16_t year = 1970, month = 1;
	uint8_t days_in_month[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
	if(julian == NULL || ntp == NULL)
		return QAPI_ERROR;

	if(julian->year < 1970 || julian->year > 2036)
		return QAPI_ERROR;

	while(year < julian->year)
	{
		days += get_no_of_days_in_year(year);
		year++;
	}

	if(get_no_of_days_in_year(year) > NUM_DAYS_PER_YEAR)
		days_in_month[1] = 29;
	
	while(month < julian->month)
	{
		days += days_in_month[month-1];
		month++;
	}
	days += (julian->day - 1);

	if(julian->day_Of_Week != ((days+3)%7))
		return QAPI_ERROR;

	t = ((days*24+julian->hour)*60+julian->minute)*60+julian->second;
	t += DIFF_SEC_1900_1970;
	ntp->second = t;
	ntp->frac = 0;
	
	return QAPI_OK;
}

qapi_Status_t rtc_convert_NTP_to_Julian(qapi_Time_t *julian, ntp_Time_t *ntp)
{
	uint32_t t, days;
	uint8_t days_in_month[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
	uint16_t d = 0;
	uint16_t year = 1970;
	uint8_t count = 0;
	
	if(julian == NULL || ntp == NULL)
		return QAPI_ERROR;

	t = ntp->second;
	t -= DIFF_SEC_1900_1970;
	julian->second = t%60;
	t = t/60;
	julian->minute = t%60;
	t = t/60;
	julian->hour = t%24;
	days = t/24;
	julian->day_Of_Week = ((days+3)%7);
	while(days >= ( d = get_no_of_days_in_year(year)))
	{
		days -= d;
		year++;
	}

	if(d > NUM_DAYS_PER_YEAR)
		days_in_month[1] = 29;

	for(count = 0;days >= (uint32_t)days_in_month[count]; count++)
	{
		days -= days_in_month[count];
	}
	julian->month = (count+1);
	julian->year = year;
	julian->day = (uint16_t)(days + 1);
	
	return QAPI_OK;
}

/**
 *  Gets the Julian time.
 *
 * @param[in] tm  Pointer to a buffer to contain the Julian time.
 *
 * @return #QAPI_OK on success, or an code on error.
 */
qapi_Status_t qapi_Core_RTC_Julian_Get(qapi_Time_t *tm)
{
	ntp_Time_t ntp;
	if(qapi_Core_RTC_NTP_Get(&ntp) != QAPI_OK)
		return QAPI_ERROR;

	if(g_time_zone.add_sub == 1)
	{
		ntp.second += (g_time_zone.hour * 3600 + g_time_zone.min * 60);
	}
	else
	{
		ntp.second -= (g_time_zone.hour * 3600 + g_time_zone.min * 60);
	}
	
	return rtc_convert_NTP_to_Julian(tm, &ntp);
}

/**
 *  Sets the Julian time.
 *
 * @param[in] tm  Pointer to a buffer to contain the Julian time.
 *
 * @return #QAPI_OK on success, or an code on error.
 */
qapi_Status_t qapi_Core_RTC_Julian_Set(qapi_Time_t *tm)
{
	ntp_Time_t ntp;
	if(rtc_convert_Julian_to_NTP(tm, &ntp) != QAPI_OK)
		return QAPI_ERROR;

	if(g_time_zone.add_sub == 1)
	{
		ntp.second -= (g_time_zone.hour * 3600 + g_time_zone.min * 60);
	}
	else
	{
		ntp.second += (g_time_zone.hour * 3600 + g_time_zone.min * 60);
	}
	
	return qapi_Core_RTC_NTP_Set(&ntp);
}


/**
 *  Gets the NTP time.
 *
 * @param[in] ms  Pointer to a buffer to contain the NTP time.
 *
 * @return #QAPI_OK on success, or a different code on error.
 */
qapi_Status_t qapi_Core_RTC_NTP_Get(ntp_Time_t *tm)
{
	uint32_t curr, diff;
	uint32_t sec, msec;

	if(last_rtc_time == 0)
		return QAPI_ERROR;

	curr = hres_timer_curr_time_ms();
	diff = curr - last_rtc_time + rtc_time_padding;
	
	sec = (diff/1000) + last_ntp_time.second;
	msec = (diff%1000) + get_msec_from_ntp_frac(last_ntp_time.frac);
	
	if(msec >= 1000)
	{
		sec += 1;
		msec -= 1000;
	}
	tm->second = sec;
	tm->frac = get_ntp_frac_from_msec(msec);
	return QAPI_OK;
}

/**
 *  Sets the NTP time.
 *
 * @param[in] tm  Pointer to a buffer to contain NTP time.
 *
 * @return #QAPI_OK on success, or a different code on error.
 */
qapi_Status_t qapi_Core_RTC_NTP_Set(ntp_Time_t *tm)
{
	last_rtc_time = hres_timer_curr_time_ms();
	start_tsf_beacon = 0;
	rtc_time_padding = 0;
	rtc_time_padding_history = 0;
	last_ntp_time.second = tm->second;
	last_ntp_time.frac = tm->frac;
	
	return QAPI_OK;
}

/**
 *  Gets boot reason
 *
 * @param[in] tm  Pointer to a uint32_t to contain boot reason.
 *
 * @return #QAPI_OK on success, or a different code on error.
 */
qapi_Status_t qapi_Core_Obtain_Boot_Reason(uint32_t *data)
{
	if(data != NULL)
		*data = *(uint8_t *)QWLAN_PMU_SYSTEM_STATUS_REG;
	else
		return QAPI_ERROR;
	
	return QAPI_OK;
}

/**
 *  Gets the time zone.
 *
 * @param[in] zone  Pointer to a buffer to contain the time zone.
 *
 * @return #QAPI_OK on success, or a different code on error.
 */
qapi_Status_t qapi_Core_Time_Zone_Get(time_zone_t *zone)
{
	if(zone == NULL)
		return QAPI_ERROR;
	
	memscpy(zone, sizeof(time_zone_t), &g_time_zone, sizeof(time_zone_t));
	return QAPI_OK;
}

/**
 *  Sets the time zone.
 *
 * @param[in] tm  Pointer to a buffer to contain the time zone.
 *
 * @return #QAPI_OK on success, or a different code on error.
 */
qapi_Status_t qapi_Core_Time_Zone_Set(time_zone_t *zone)
{
	if(zone == NULL)
		return QAPI_ERROR;
	
	memscpy(&g_time_zone, sizeof(time_zone_t), zone, sizeof(time_zone_t));
	return QAPI_OK;
}

