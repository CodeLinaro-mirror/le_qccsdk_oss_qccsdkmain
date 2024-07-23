/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "qapi_types.h"
#include "qapi_version.h"
#include "qapi_status.h"
#include "qapi_heap_status.h"

#include "qapi_console.h"
#include "qapi_fatal_err.h"

#include <stdio.h>

#include "nt_sys_monitoring.h"
#include "wifi_cmn.h"
#include "fwconfig_cmn.h"
#include "nt_flags.h"
#include "qurt_internal.h"
#include "qapi_rtc.h"


static qapi_Status_t platform_reset(uint32_t __attribute__((__unused__)) parameters_count, QAPI_Console_Parameter_t __attribute__((__unused__)) * parameters)
{
    printf("Reboot...\n");
    nt_system_sw_reset();
    return QAPI_OK;
}

#ifdef NT_FN_DEBUG_STATS
static qapi_Status_t read_mem(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    uint32_t data, size, addr;
    if( Parameter_Count != 2 || !Parameter_List || !Parameter_List[0].Integer_Is_Valid || !Parameter_List[1].Integer_Is_Valid) {
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }
    addr = Parameter_List[0].Integer_Value;
    size = Parameter_List[1].Integer_Value;
    if (size == 1)
        data = *(uint8_t *)addr;
    else if (size == 2)
        data = *(uint16_t *)addr;
    else if (size == 4)
        data = *(uint32_t *)addr;
    else
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;

    printf("Reading, Address = 0x%08x , Width = %d  Data = 0x%08x(%d)",addr,size,data,data);
    return QAPI_OK;
}

static qapi_Status_t write_mem(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    uint32_t data, size, addr;
    if( Parameter_Count != 3 || !Parameter_List || !Parameter_List[0].Integer_Is_Valid || !Parameter_List[1].Integer_Is_Valid|| !Parameter_List[2].Integer_Is_Valid) {
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }
    addr = Parameter_List[0].Integer_Value;
    size = Parameter_List[1].Integer_Value;
    data = Parameter_List[2].Integer_Value;
    if (size == 1)
        *(uint8_t *)addr = data;
    else if (size == 2)
        *(uint16_t *)addr = data;
    else if (size == 4)
        *(uint32_t *)addr = data;
    else
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;

    printf("Writting, Address = 0x%08x , Width = %d  Data = 0x%08x(%d)",addr,size,data,data);
    return QAPI_OK;
}
#endif

static qapi_Status_t bgtest(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    uint32_t time_s = 5;
    uint32_t interval_s = 1;
    uint32_t i = 0;

    if (Parameter_Count >= 1) {
        if (!Parameter_List[0].Integer_Is_Valid) {
            return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
        }
        time_s = Parameter_List[0].Integer_Value;
    }
    if (Parameter_Count >= 2) {
        if (!Parameter_List[1].Integer_Is_Valid) {
            return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
        }
        interval_s = Parameter_List[1].Integer_Value;
    }
    printf("bgtest started: time_s=%ds interval_s=%ds\n", time_s, interval_s);
    for (i=0; i<time_s; i+=interval_s) {
        printf("i=%ds sleep %ds\n", i, interval_s);
        qurt_thread_sleep(interval_s*1000);
    }
    printf("bgtest ended successfully\n");
    return QAPI_OK;
}

qapi_Status_t platform_demo_free(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    (void)(Parameter_Count);
    (void)(Parameter_List);
    heap_status hs;

    if(qapi_Heap_Status(&hs) != QAPI_OK)
    {
        printf("Error getting heap status\n");
        return QAPI_ERROR;
    }

    printf("           total       used       free       min_free\n");
    printf("Heap:   %8d   %8d   %8d       %8d\n", hs.total_Bytes, hs.total_Bytes-hs.free_Bytes, hs.free_Bytes, hs.min_ever_free_bytes);

    return QAPI_OK;
}

qapi_Status_t platform_demo_watchdog_reset(__attribute__((__unused__)) uint32_t parameters_count, __attribute__((__unused__)) QAPI_Console_Parameter_t * parameters)
{
    //trigger watchdog rereset
    QAPI_FATAL_ERR(0,0,0);
	
    return QAPI_OK;
}


static qapi_Status_t platform_demo_get_time(uint32_t __attribute__((__unused__)) Parameter_Count, QAPI_Console_Parameter_t __attribute__((__unused__)) *Parameter_List)
{
    qapi_Time_t tm;
    qapi_Status_t status;

    status = qapi_Core_RTC_Julian_Get(&tm);
    if ( QAPI_OK != status ) {
        printf("Failed on a call to qapi_Core_RTC_Julian_Get(), status=%d\r\n", status);
        printf("Please note that this is likely happened because the time was not set\r\n", status);
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_ERROR;
    }

    printf("Julian Time: \r\n");
    printf("year = %d\r\n", tm.year);
    printf("month = %d\r\n", tm.month);
    printf("day = %d\r\n", tm.day);
    printf("hour = %d\r\n", tm.hour);
    printf("minute = %d\r\n", tm.minute);
    printf("second = %d\r\n", tm.second);
    printf("day_Of_Week = %d\r\n", tm.day_Of_Week);

    return QAPI_OK;
}

void print_usage_set_time()
{
    printf("Usage: time set year month day hour minute second day_Of_Week\r\n");
    printf("\t\t year: Year [1980 through 2100]\r\n");
    printf("\t\t month: Month of year [1 through 12]\r\n");
    printf("\t\t day: Day of month [1 through 31]\r\n");
    printf("\t\t hour: Hour of day [0 through 23]\r\n");
    printf("\t\t minute: Minute of hour [0 through 59]\r\n");
    printf("\t\t second: Second of minute [0 through 59]\r\n");
    printf("\t\t day_Of_Weak: Day of the week [0 through 6] (corresponding to Monday through Sunday)\r\n");
}

static qapi_Status_t platform_demo_set_time(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    qapi_Time_t tm;
    qapi_Status_t status;

	if ( Parameter_Count != 7 ) {
		printf("Invalid number of arguments\r\n");
		goto platform_demo_set_time_on_error;
	}

	// check year
	if ((Parameter_List[0].Integer_Is_Valid) && (Parameter_List[0].Integer_Value >= 1980) && (Parameter_List[0].Integer_Value <= 2100))
	{
		tm.year = Parameter_List[0].Integer_Value;
	}
	else
	{
		printf("Invalid year\r\n");
		goto platform_demo_set_time_on_error;
	}
	
	// check month
	if ((Parameter_List[1].Integer_Is_Valid) && (Parameter_List[1].Integer_Value >= 1) && (Parameter_List[1].Integer_Value <= 12))
	{
		tm.month = Parameter_List[1].Integer_Value;
	}
	else
	{
		printf("Invalid month\r\n");
		goto platform_demo_set_time_on_error;
	}
	
	// check day
	if ((Parameter_List[2].Integer_Is_Valid) && (Parameter_List[2].Integer_Value >= 1) && (Parameter_List[2].Integer_Value <= 31))
	{
		tm.day = Parameter_List[2].Integer_Value;
	}
	else
	{
		printf("Invalid day\r\n");
		goto platform_demo_set_time_on_error;
	}
	
	// check hour
	if ((Parameter_List[3].Integer_Is_Valid) && (Parameter_List[3].Integer_Value >= 0) && (Parameter_List[3].Integer_Value <= 23))
	{
		tm.hour = Parameter_List[3].Integer_Value;
	}
	else
	{
		printf("Invalid hour\r\n");
		goto platform_demo_set_time_on_error;
	}
	
	// check minute
	if ((Parameter_List[4].Integer_Is_Valid) && (Parameter_List[4].Integer_Value >= 0) && (Parameter_List[4].Integer_Value <= 59))
	{
		tm.minute = Parameter_List[4].Integer_Value;
	}
	else
	{
		printf("Invalid minute\r\n");
		goto platform_demo_set_time_on_error;
	}
	
	// check second
	if ((Parameter_List[5].Integer_Is_Valid) && (Parameter_List[5].Integer_Value >= 0) && (Parameter_List[5].Integer_Value <= 59))
	{
		tm.second = Parameter_List[5].Integer_Value;
	}
	else
	{
		printf("Invalid second\r\n");
		goto platform_demo_set_time_on_error;
	}
	
	// check day of the week
	if ((Parameter_List[6].Integer_Is_Valid) && (Parameter_List[6].Integer_Value >= 0) && (Parameter_List[6].Integer_Value <= 6))
	{
		tm.day_Of_Week = Parameter_List[6].Integer_Value;
	}
	else
	{
		printf("Invalid day_Of_Week\r\n");
		goto platform_demo_set_time_on_error;
	}
	
	status = qapi_Core_RTC_Julian_Set(&tm);
	if (0 != status ) {
		printf("Failed on a call to qapi_Core_RTC_Julian_Set(), status=%d\r\n", status);
		goto platform_demo_set_time_on_error;
	}
	
	return QAPI_OK;
	
platform_demo_set_time_on_error:
	print_usage_set_time();
	return QAPI_ERROR_CONSOLE_COMMAND_STATUS_ERROR;
}

static qapi_Status_t platform_demo_time(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
	if (Parameter_Count < 1) {
		printf("Invalid number of arguments\r\n");
		goto platform_demo_time_on_error;
	}

	if(0 == strcmp(Parameter_List[0].String_Value, "get")) {
		return platform_demo_get_time(Parameter_Count-1, &Parameter_List[1]);
	}
	else if ( 0 == strcmp(Parameter_List[0].String_Value, "set") ) {
		return platform_demo_set_time(Parameter_Count-1, &Parameter_List[1]);
	}

platform_demo_time_on_error:
	printf("Usage: time get/set <params>\r\n");
    return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
}

static qapi_Status_t platform_demo_get_time_ntp(uint32_t __attribute__((__unused__)) Parameter_Count, QAPI_Console_Parameter_t __attribute__((__unused__)) *Parameter_List)
{
	ntp_Time_t ntp;
    qapi_Status_t status;

    status = qapi_Core_RTC_NTP_Get(&ntp);
    if ( QAPI_OK != status ) {
        printf("Failed on a call to qapi_Core_RTC_NTP_Get(), status=%d\r\n", status);
        printf("Please note that this is likely happened because the time was not set\r\n", status);
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_ERROR;
    }

    printf("NTP time: \r\n");
    printf("sec = %u\r\n", ntp.second);
    printf("frac = %u\r\n", ntp.frac);

    return QAPI_OK;	
}

//static qapi_Status_t platform_demo_set_time_ntp(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
static qapi_Status_t platform_demo_set_time_ntp(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    ntp_Time_t ntp;
	uint32_t sec;

	if ( Parameter_Count < 1 ) {
		printf("Invalid number of arguments\r\n");
		goto platform_demo_set_time_ntp_on_error;
	}

	// check sec part
	if (Parameter_List[0].Integer_Is_Valid)
	{
		ntp.second = Parameter_List[0].Integer_Value;
	}
	else
	{
		printf("Invalid ntp second\r\n");
		goto platform_demo_set_time_ntp_on_error;
	}

	//check frac part
	if(Parameter_Count > 1 && Parameter_List[1].Integer_Is_Valid)
	{
		ntp.frac = Parameter_List[1].Integer_Value; 
	}
	else
	{
		ntp.frac = 0;
	}
	return qapi_Core_RTC_NTP_Set(&ntp);
		
platform_demo_set_time_ntp_on_error:
	printf("Usage: time_ntp set sec [frac]\r\n");
	return QAPI_ERROR_CONSOLE_COMMAND_STATUS_ERROR;
}

static qapi_Status_t platform_demo_time_ntp(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
	if (Parameter_Count < 1) {
		printf("Invalid number of arguments\r\n");
		goto platform_demo_time_ntp_on_error;
	}
	
	if(0 == strcmp(Parameter_List[0].String_Value, "get")) {
		return platform_demo_get_time_ntp(Parameter_Count-1, &Parameter_List[1]);
	}
	else if ( 0 == strcmp(Parameter_List[0].String_Value, "set") ) {
		return platform_demo_set_time_ntp(Parameter_Count-1, &Parameter_List[1]);
	}
	
platform_demo_time_ntp_on_error:
	printf("Usage: time_ntp get/set <params>\r\n");
	return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
}

const QAPI_Console_Command_t platform_shell_cmds[] =
{
    // cmd_function    cmd_string               usage_string             description
    {platform_reset, "reset", "", "reset the platform\n"},
#ifdef NT_FN_DEBUG_STATS
    {read_mem, "read_mem", "<addr> <size:1|2|4>", "read memory\n"},
    {write_mem, "write_mem", "<addr> <size:1|2|4> <value>", "write memory\n"},
#endif
    {bgtest, "bgtest", "[time_s(5)] [interval_s(1)]", "background command test\n"},
    {platform_demo_free, "free", "\n", "display the heap size and an approximation of free amount of heap bytes\n"},
    {platform_demo_watchdog_reset, "wdrst", "\n", "trigger watchdog reset\n"},
    {platform_demo_time, "time", "\n", "get/set current time in Julian format\n"},
	{platform_demo_time_ntp, "time_ntp", "\n", "get/set current time in NTP format\n"},
};

const QAPI_Console_Command_Group_t platform_shell_cmd_group = {"platform", sizeof(platform_shell_cmds) / sizeof(QAPI_Console_Command_t), platform_shell_cmds};

QAPI_Console_Group_Handle_t platform_shell_cmd_group_handle;

void platform_shell_init (void)
{
    platform_shell_cmd_group_handle = QAPI_Console_Register_Command_Group(NULL, &platform_shell_cmd_group);
}

