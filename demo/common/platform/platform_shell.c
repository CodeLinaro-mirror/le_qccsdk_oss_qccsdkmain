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
#include "wifi_fw_pmu_ts_cfg.h"
#include "ferm_hkadc_drv.h"
#ifdef SUPPORT_QCSPI_SLAVE
#include "qcspi_slave_api.h"
#endif
#include "qapi_rram.h"
#include "nt_hw.h"
#ifdef CONFIG_WIFI_FW_COREDUMP_SUPPORT
#include "err.h"
#include "errlog.h"
#include "qapi_lowpower.h"

extern unsigned int __rram_region_end_address;

#define COREDUMP_TEST_INVALID_ADDRESS (__rram_region_end_address + 0x1000)
#endif

extern bool rram_udpart_init_done;

static qapi_Status_t platform_reset(uint32_t __attribute__((__unused__)) parameters_count,
                                    QAPI_Console_Parameter_t __attribute__((__unused__)) * parameters)
{
    printf("Reboot...\n");
    nt_system_sw_reset();
    return QAPI_OK;
}

#ifdef NT_FN_DEBUG_STATS
static qapi_Status_t read_mem(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    uint32_t data, size, addr;

    if (Parameter_Count != 2 || !Parameter_List || !Parameter_List[0].Integer_Is_Valid ||
        !Parameter_List[1].Integer_Is_Valid) {
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }
    addr = Parameter_List[0].Integer_Value;
    size = Parameter_List[1].Integer_Value;
    if (size == 1) {
        data = *(uint8_t *)addr;

    } else if (size == 2) {
        data = *(uint16_t *)addr;
    } else if (size == 4) {
        data = *(uint32_t *)addr;
    } else
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;

    printf("Reading, Address = 0x%08x , Width = %d  Data = 0x%0*x(%d)", addr, size, size * 2, data, data);

    return QAPI_OK;
}

static qapi_Status_t write_mem(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    uint32_t data, size, addr;

    if (Parameter_Count != 3 || !Parameter_List || !Parameter_List[0].Integer_Is_Valid ||
        !Parameter_List[1].Integer_Is_Valid || !Parameter_List[2].Integer_Is_Valid) {
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }
    addr = Parameter_List[0].Integer_Value;
    size = Parameter_List[1].Integer_Value;
    data = Parameter_List[2].Integer_Value;
    if (size == 1) {
        *(uint8_t *)addr = data;
    } else if (size == 2) {
        *(uint16_t *)addr = data;
    } else if (size == 4) {
        *(uint32_t *)addr = data;
    } else
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;

    printf("Writting, Address = 0x%08x , Width = %d  Data = 0x%0*x(%d)", addr, size, size * 2, data, data);

    return QAPI_OK;
}
#endif

static qapi_Status_t rram_read(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    uint32_t i;
    uint32_t address;
    uint32_t byte_cnt;
    char *buffer = NULL;
    uint32_t partid;

    if (!rram_udpart_init_done) {
        printf("rram was not inited\n");
        return QAPI_ERROR;
    }

    if (Parameter_Count != 3 || Parameter_List == NULL || Parameter_List[0].Integer_Value < 0 ||
        Parameter_List[1].Integer_Value < 0 || Parameter_List[2].Integer_Value < 0) {
        printf("Read <partid> <Addr> <Cnt>\n");
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }

    partid = Parameter_List[0].Integer_Value;
    address = Parameter_List[1].Integer_Value;
    byte_cnt = Parameter_List[2].Integer_Value;

    buffer = (char *)malloc(byte_cnt * sizeof(char));
    if (buffer == NULL) {
        printf("ERROR: No enough memory\n");
        return QAPI_ERR_NO_MEMORY;
    }
    memset(buffer, 0, byte_cnt);

    if (qapi_rram_read(partid, address, buffer, byte_cnt) != 0) {
        printf("Read Failed");
        return QAPI_ERROR;
    }

    free(buffer);
    return QAPI_OK;
}

static qapi_Status_t rram_write(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    uint32_t partid;
    uint32_t address;
    uint32_t byte_cnt;
    char *buffer = NULL;

    if (!rram_udpart_init_done) {
        printf("rram was not inited\n");
        return QAPI_ERROR;
    }

    if (Parameter_Count != 3 || Parameter_List == NULL || Parameter_List[0].Integer_Value < 0 ||
        Parameter_List[1].Integer_Value < 0) {
        printf("Write <Addr> <Cnt> <Value string>\n");
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }

    partid = Parameter_List[0].Integer_Value;
    address = Parameter_List[1].Integer_Value;
    buffer = Parameter_List[2].String_Value;
    byte_cnt = strlen(buffer);
    if (byte_cnt > 65536) {
        /* The max len of QLI buffer is 256 bytes, here should be a limitation */
        printf("The string length should be less than 65536 Bytes\n");
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }

    if (qapi_rram_write(partid, address, buffer, byte_cnt) != 0) {
        printf("Write Failed");
        return QAPI_ERROR;
    }

    return QAPI_OK;
}

#define RRAM_OP_UNIT 1024
static qapi_Status_t rram_test(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    qapi_Status_t status = QAPI_OK;
    uint32_t i, len;
    uint32_t offset;
    uint32_t byte_cnt;
    char *buffer = NULL;
    char *read_buffer = NULL;
    uint32_t partid;

    if (Parameter_Count != 3 || Parameter_List == NULL || Parameter_List[0].Integer_Value < 0 ||
        Parameter_List[1].Integer_Value < 0 || Parameter_List[2].Integer_Value < 0) {
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }

    partid = Parameter_List[0].Integer_Value;
    offset = Parameter_List[1].Integer_Value;
    byte_cnt = Parameter_List[2].Integer_Value;
    buffer = malloc(byte_cnt + 1);
    if (buffer == NULL) {
        printf("ERROR: No enough memory\n");
        return QAPI_ERR_NO_MEMORY;
    }

    if (byte_cnt > 65536) {  // 64KB
        printf("Test size should less 64K\n");
        free(buffer);
        return QAPI_ERR_INVALID_PARAM;
    }
    printf("Total test size %d bytes\n", byte_cnt);

    read_buffer = malloc(byte_cnt);
    if (read_buffer == NULL) {
        printf("ERROR: No enough memory\n");
        free(buffer);
        return QAPI_ERR_NO_MEMORY;
    }
    while (byte_cnt) {
        if (byte_cnt >= RRAM_OP_UNIT) {
            len = RRAM_OP_UNIT;
        } else {
            len = byte_cnt;
        }
        memset(buffer, 0, sizeof(buffer));
        memset(read_buffer, 0, sizeof(read_buffer));

        for (i = 0; i < len; i++) {
            buffer[i] = 'a';
        }
        buffer[len] = '\0';
        status = qapi_rram_write(partid, offset, buffer, len);
        if (status != QAPI_OK) {
            printf("\r\nBuf(%d) test failed(%d)\n", i, status);
            break;
        }

        status = qapi_rram_read(partid, offset, read_buffer, len);
        if (status != QAPI_OK) {
            printf("\r\nrram read test failed(%d)\n", i, status);
            break;
        }

        if (memcmp(read_buffer, buffer, len) != 0) {
            status = QAPI_ERROR;
            printf("\r\nVerify failed at offset 0x%x\n", offset);
            break;
        }

        offset += len;
        byte_cnt -= len;
        printf("\r\nVerify OK at offset 0x%x\n", offset);
    }

    free(buffer);
    free(read_buffer);
    return status;
}

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
    for (i = 0; i < time_s; i += interval_s) {
        printf("i=%ds sleep %ds\n", i, interval_s);
        qurt_thread_sleep(interval_s * 1000);
    }
    printf("bgtest ended successfully\n");
    return QAPI_OK;
}

qapi_Status_t platform_demo_free(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    (void)(Parameter_Count);
    (void)(Parameter_List);
    heap_status hs;

    if (qapi_Heap_Status(&hs) != QAPI_OK) {
        printf("Error getting heap status\n");
        return QAPI_ERROR;
    }

    printf("                  total       used       free         min_free\n");
    printf("Heap:           %8d   %8d   %8d       %8d\n", hs.total_Bytes, hs.total_Bytes - hs.free_Bytes, hs.free_Bytes,
           hs.min_ever_free_bytes);
#if !CONFIG_MATTER_ENABLE
    printf("lwip heap:     %8d   %8d   %8d       %8d\n", hs.lwip_total_Bytes, hs.lwip_total_Bytes - hs.lwip_free_Bytes,
           hs.lwip_free_Bytes, hs.lwip_min_ever_free_bytes);
    printf("lwip pool:     %8d   %8d   %8d       %8d\n", hs.lwip_total_pool, hs.lwip_total_pool - hs.lwip_free_pool,
           hs.lwip_free_pool, hs.lwip_min_ever_free_pool);
#endif
    return QAPI_OK;
}

qapi_Status_t platform_demo_watchdog_reset(__attribute__((__unused__)) uint32_t parameters_count,
                                           __attribute__((__unused__)) QAPI_Console_Parameter_t *parameters)
{
    // trigger watchdog rereset
    QAPI_FATAL_ERR(0, 0, 0);

    return QAPI_OK;
}

static qapi_Status_t platform_demo_get_time(uint32_t __attribute__((__unused__)) Parameter_Count,
                                            QAPI_Console_Parameter_t __attribute__((__unused__)) * Parameter_List)
{
    qapi_Time_t tm;
    time_zone_t zone;
    qapi_Status_t status;

    status = qapi_Core_RTC_Julian_Get(&tm);
    if (QAPI_OK != status) {
        printf("Failed on a call to qapi_Core_RTC_Julian_Get(), status=%d\r\n", status);
        printf("Please note that this is likely happened because the time was not set\r\n", status);
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_ERROR;
    }

    qapi_Core_Time_Zone_Get(&zone);
    printf("Julian Time: \r\n");
    printf("year = %d\r\n", tm.year);
    printf("month = %d\r\n", tm.month);
    printf("day = %d\r\n", tm.day);
    printf("hour = %d\r\n", tm.hour);
    printf("minute = %d\r\n", tm.minute);
    printf("second = %d\r\n", tm.second);
    printf("day_Of_Week = %d\r\n", tm.day_Of_Week);
    printf("UTC%c%02d:%02d\r\n", zone.add_sub ? '+' : '-', zone.hour, zone.min);

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

    if (Parameter_Count != 7) {
        printf("Invalid number of arguments\r\n");
        goto platform_demo_set_time_on_error;
    }

    // check year
    if ((Parameter_List[0].Integer_Is_Valid) && (Parameter_List[0].Integer_Value >= 1980) &&
        (Parameter_List[0].Integer_Value <= 2100)) {
        tm.year = Parameter_List[0].Integer_Value;
    } else {
        printf("Invalid year\r\n");
        goto platform_demo_set_time_on_error;
    }

    // check month
    if ((Parameter_List[1].Integer_Is_Valid) && (Parameter_List[1].Integer_Value >= 1) &&
        (Parameter_List[1].Integer_Value <= 12)) {
        tm.month = Parameter_List[1].Integer_Value;
    } else {
        printf("Invalid month\r\n");
        goto platform_demo_set_time_on_error;
    }

    // check day
    if ((Parameter_List[2].Integer_Is_Valid) && (Parameter_List[2].Integer_Value >= 1) &&
        (Parameter_List[2].Integer_Value <= 31)) {
        tm.day = Parameter_List[2].Integer_Value;
    } else {
        printf("Invalid day\r\n");
        goto platform_demo_set_time_on_error;
    }

    // check hour
    if ((Parameter_List[3].Integer_Is_Valid) && (Parameter_List[3].Integer_Value >= 0) &&
        (Parameter_List[3].Integer_Value <= 23)) {
        tm.hour = Parameter_List[3].Integer_Value;
    } else {
        printf("Invalid hour\r\n");
        goto platform_demo_set_time_on_error;
    }

    // check minute
    if ((Parameter_List[4].Integer_Is_Valid) && (Parameter_List[4].Integer_Value >= 0) &&
        (Parameter_List[4].Integer_Value <= 59)) {
        tm.minute = Parameter_List[4].Integer_Value;
    } else {
        printf("Invalid minute\r\n");
        goto platform_demo_set_time_on_error;
    }

    // check second
    if ((Parameter_List[5].Integer_Is_Valid) && (Parameter_List[5].Integer_Value >= 0) &&
        (Parameter_List[5].Integer_Value <= 59)) {
        tm.second = Parameter_List[5].Integer_Value;
    } else {
        printf("Invalid second\r\n");
        goto platform_demo_set_time_on_error;
    }

    // check day of the week
    if ((Parameter_List[6].Integer_Is_Valid) && (Parameter_List[6].Integer_Value >= 0) &&
        (Parameter_List[6].Integer_Value <= 6)) {
        tm.day_Of_Week = Parameter_List[6].Integer_Value;
    } else {
        printf("Invalid day_Of_Week\r\n");
        goto platform_demo_set_time_on_error;
    }

    status = qapi_Core_RTC_Julian_Set(&tm);
    if (0 != status) {
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

    if (0 == strcmp(Parameter_List[0].String_Value, "get")) {
        return platform_demo_get_time(Parameter_Count - 1, &Parameter_List[1]);
    } else if (0 == strcmp(Parameter_List[0].String_Value, "set")) {
        return platform_demo_set_time(Parameter_Count - 1, &Parameter_List[1]);
    }

platform_demo_time_on_error:
    printf("Usage: time get/set <params>\r\n");
    return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
}

static qapi_Status_t platform_demo_get_time_ntp(uint32_t __attribute__((__unused__)) Parameter_Count,
                                                QAPI_Console_Parameter_t __attribute__((__unused__)) * Parameter_List)
{
    ntp_Time_t ntp;
    qapi_Status_t status;

    status = qapi_Core_RTC_NTP_Get(&ntp);
    if (QAPI_OK != status) {
        printf("Failed on a call to qapi_Core_RTC_NTP_Get(), status=%d\r\n", status);
        printf("Please note that this is likely happened because the time was not set\r\n", status);
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_ERROR;
    }

    printf("NTP time: \r\n");
    printf("sec = %u\r\n", ntp.second);
    printf("frac = %u\r\n", ntp.frac);

    return QAPI_OK;
}

// static qapi_Status_t platform_demo_set_time_ntp(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
static qapi_Status_t platform_demo_set_time_ntp(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    ntp_Time_t ntp;
    uint32_t sec;

    if (Parameter_Count < 1) {
        printf("Invalid number of arguments\r\n");
        goto platform_demo_set_time_ntp_on_error;
    }

    // check sec part
    if (Parameter_List[0].Integer_Is_Valid) {
        ntp.second = Parameter_List[0].Integer_Value;
    } else {
        printf("Invalid ntp second\r\n");
        goto platform_demo_set_time_ntp_on_error;
    }

    // check frac part
    if (Parameter_Count > 1 && Parameter_List[1].Integer_Is_Valid) {
        ntp.frac = Parameter_List[1].Integer_Value;
    } else {
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

    if (0 == strcmp(Parameter_List[0].String_Value, "get")) {
        return platform_demo_get_time_ntp(Parameter_Count - 1, &Parameter_List[1]);
    } else if (0 == strcmp(Parameter_List[0].String_Value, "set")) {
        return platform_demo_set_time_ntp(Parameter_Count - 1, &Parameter_List[1]);
    }

platform_demo_time_ntp_on_error:
    printf("Usage: time_ntp get/set <params>\r\n");
    return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
}

static qapi_Status_t platform_demo_info(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    (void)(Parameter_Count);
    (void)(Parameter_List);

    printf("Show system information\n");
    printf("Temperature=%dC\n", pmu_ts_get_current_temperature());
    printf("Vbat=%dmV\n", tv_monitor_get_vbat_mV());
    printf("get RTC time\n");
    platform_demo_get_time(0, NULL);
    printf("get heap status\n");
    platform_demo_free(0, NULL);
    return QAPI_OK;
}

#ifdef SUPPORT_QCSPI_SLAVE
static qapi_Status_t platform_qcspi_enable(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    if (Parameter_Count != 1) {
        printf("Invalid number of arguments\r\n");
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }

    if (0 == Parameter_List[0].Integer_Value)
        qcspi_slv_deinit();
    else
        qcspi_slv_init();

    return QAPI_OK;
}
#endif

static qapi_Status_t platform_demo_getcx(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    (void)(Parameter_Count);
    (void)(Parameter_List);

    printf("Show cx(ULP-SMPS2) related information\n");
    tv_monitor_dump("getcx");
    dtim_tv_monitor_dump("getcx");
    hkadc_drv_dump("getcx");
    return QAPI_OK;
}

static qapi_Status_t platform_demo_calcxoneshot(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    int32_t temperatureC = 0;
    uint32_t vbatmV = 0;
    uint32_t OTP_oneshot = 0;
    uint32_t optmized_oneshot = 0;
    uint32_t t_one_shot_ns = 0;

    if (Parameter_Count != 2) {
        printf("Invalid number of arguments\n");
        goto platform_demo_calcxoneshot_error;
    }

    if (!Parameter_List[0].Integer_Is_Valid || !Parameter_List[1].Integer_Is_Valid) {
        printf("temperature and vbatmV Should be integer\n");
        goto platform_demo_calcxoneshot_error;
    }

    temperatureC = Parameter_List[0].Integer_Value;
    vbatmV = Parameter_List[1].Integer_Value;

    if ((temperatureC < TEMPERATUREC_MIN) || (temperatureC > TEMPERATUREC_MAX)) {
        printf("temperature not supported, should be in [%d, %d]\n", TEMPERATUREC_MIN, TEMPERATUREC_MAX);
        goto platform_demo_calcxoneshot_error;
    }

    if ((vbatmV < VBATMV_MIN) || (vbatmV > VBATMV_MAX)) {
        printf("vbatmV not supported, should be in [%d, %d]\n", VBATMV_MIN, VBATMV_MAX);
        goto platform_demo_calcxoneshot_error;
    }

    optmized_oneshot = ulpsmps2_get_optimized_oneshot(vbatmV, temperatureC, &OTP_oneshot, &t_one_shot_ns);
    printf("vbat=%dmV T=%dC OTP_oneshot=%d t_one_shot_ns=%dns optmized_oneshot=%d\n", vbatmV, temperatureC, OTP_oneshot,
           t_one_shot_ns, optmized_oneshot);
    return optmized_oneshot;

platform_demo_calcxoneshot_error:
    printf("Usage: calcxoneshot <tempC> <vbatmV>\n");
    return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
}

extern bool g_presleep_update_ulpsmps2_oneshot_enable;

static qapi_Status_t platform_demo_setcxoneshot(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    uint32_t requested_oneshot = 0;
    qapi_Status_t optmized_oneshot = 0;

    if ((Parameter_Count >= 1) && Parameter_List[0].Integer_Is_Valid) {
        requested_oneshot = Parameter_List[0].Integer_Value;
        if (requested_oneshot > CX_ONESHOT_MAX) {
            printf("requested_oneshot not supported, should be in (%d, %d]\n", CX_ONESHOT_MIN, CX_ONESHOT_MAX);
            if (requested_oneshot == 255) {
                // if requested_oneshot==255, enable update oneshot in sleep
                g_presleep_update_ulpsmps2_oneshot_enable = true;
                printf("Magic code match, enable update oneshot in sleep\n");
            }
            goto platform_demo_setcxoneshot;
        }
    } else {
        goto platform_demo_setcxoneshot;
    }

    if (requested_oneshot) {
        printf("do set oneshot=%d=>%d and disable update oneshot in sleep\n", ulpsmps2_get_oneshot(),
               requested_oneshot);
        dtim_tv_set_ulpsmps2_oneshot(requested_oneshot);
        g_presleep_update_ulpsmps2_oneshot_enable = false;
        return QAPI_OK;
    }

    // if requested_oneshot==0, set oneshot according to tempC and vbatmV
    optmized_oneshot = platform_demo_calcxoneshot((Parameter_Count - 1), &Parameter_List[1]);
    if (optmized_oneshot < 0) {
        goto platform_demo_setcxoneshot;
    }
    printf("do set oneshot=%d=>%d and disable update oneshot in sleep\n", ulpsmps2_get_oneshot(), optmized_oneshot);
    dtim_tv_set_ulpsmps2_oneshot(optmized_oneshot);
    g_presleep_update_ulpsmps2_oneshot_enable = false;
    return QAPI_OK;

platform_demo_setcxoneshot:
    printf("Usage: setcxoneshot <oneshot> [tempC] [vbatmV]\n");
    return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
}

void print_usage_set_time_zone()
{
    printf("UTC time format should be UTC+XX:XX or UTC-XX:XX\n");
    printf("Hour from 00 to -12/+13, minute should be 0, 30 or 45\n\r");
}

static qapi_Status_t platform_demo_time_zone(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    qapi_Status_t status;
    time_zone_t zone;
    uint8_t hour = 0, min = 0, add_sub = 0, length = 0;
    char hr[3], mn[3], parsing_hour_min[10];

    if (Parameter_Count != 1) {
        printf("Invalid number of arguments\r\n");
        goto platform_demo_time_zone_error;
    }

    length = strlen(Parameter_List[0].String_Value);
    if (length != 9 || 0 != strncmp("UTC", Parameter_List[0].String_Value, 3)) {
        goto platform_demo_time_zone_error;
    }

    strlcpy(parsing_hour_min, Parameter_List[0].String_Value, sizeof(parsing_hour_min));

    hr[0] = parsing_hour_min[4];
    hr[1] = parsing_hour_min[5];
    hr[2] = '\0';
    hour = (hr[0] - '0') * 10 + (hr[1] - '0');
    mn[0] = parsing_hour_min[7];
    mn[1] = parsing_hour_min[8];
    mn[2] = '\0';
    min = (mn[0] - '0') * 10 + (mn[1] - '0');

    if (min != 0 && min != 30 && min != 45) {
        goto platform_demo_time_zone_error;
    }

    // valid time zone : -12,-11,...,+13,+14
    if (parsing_hour_min[3] == '+') {
        add_sub = 1;
        if (hour > 14 || (14 == hour && min > 0)) {
            goto platform_demo_time_zone_error;
        }
    } else if (parsing_hour_min[3] == '-') {
        add_sub = 0;
        if (hour > 12 || (12 == hour && min > 0)) {
            goto platform_demo_time_zone_error;
        }
    } else {
        goto platform_demo_time_zone_error;
    }

    zone.hour = hour;
    zone.min = min;
    zone.add_sub = add_sub;
    qapi_Core_Time_Zone_Set(&zone);
    return QAPI_OK;

platform_demo_time_zone_error:
    print_usage_set_time_zone();
    return QAPI_ERROR_CONSOLE_COMMAND_STATUS_ERROR;
}
static qapi_Status_t platform_demo_check_boot_reason(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    uint32_t data = 0;

    if (QAPI_OK == qapi_Core_Obtain_Boot_Reason(&data)) {
        if (0 == data & PMU_BASE_pmu_PMU_SYSTEM_STATUS_COLD_WARM_BOOT_Msk) {
            printf("Status: boot from cold boot\r\n");
        } else if (QWLAN_PMU_SYSTEM_STATUS_WARM_BOOT_FROM_SLEEP_MASK ==
                   (uint32_t)(data & QWLAN_PMU_SYSTEM_STATUS_WARM_BOOT_FROM_SLEEP_MASK)) {
            printf("Status: boot from dtim sleep\r\n");
        } else if (QWLAN_PMU_SYSTEM_STATUS_WARM_BOOT_FROM_DEEPSLEEP_MASK ==
                   (uint32_t)(data & QWLAN_PMU_SYSTEM_STATUS_WARM_BOOT_FROM_DEEPSLEEP_MASK)) {
            printf("Status: boot from deep sleep\r\n");
        } else {
            printf("Status: unknown status %d\r\n", data);
        }
        return QAPI_OK;
    } else
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_ERROR;
}

#ifdef CONFIG_WIFI_FW_COREDUMP_SUPPORT
void coredump_test_sub_func_1(int test_case_idx)
{
    int r = 0;
    volatile unsigned int *p;
    int (*pF)(void);
    volatile unsigned int a;
    volatile unsigned int b;

    switch (test_case_idx) {
        /* Trigger an assertion */
        case 0:
            configASSERT(0);
            break;
        /* Trigger an usage fault or hard fault by executing null pointer. */
        case 1:
            pF = (int (*)(void))0x00000000;
            r = pF();
            break;
        /* Trigger an usage fault or hard fault by dividing by zero. */
        case 2:
            a = 1;
            b = 0;
            r = a / b;
            printf("r = 0x%x\n", r); /* to prevent compiler optimization */
            break;
        /* Trigger a bus fault or hard fault by reading from a reserved address. */
        case 3:
            p = (unsigned int *)COREDUMP_TEST_INVALID_ADDRESS;
            r = *p;
            break;
        /* Trigger a bus fault or hard fault by writing to a reserved address. */
        case 4:
            p = (unsigned int *)COREDUMP_TEST_INVALID_ADDRESS;
            *p = 0x00BADA55;
            break;
        /* Trigger a bus fault or hard fault by executing at a reserved address. */
        case 5:
            pF = (int (*)(void))COREDUMP_TEST_INVALID_ADDRESS;
            r = pF();
            break;
        /* Trigger a memory management fault or hard fault by writing to a protected address */
        case 6:
            p = (unsigned int *)0x2;
            *p = 0x00BADA55;
            break;
        default:
            printf("Test case index shoud be an integer less than 7\n");
        }
}

void coredump_test_sub_func_0(int test_case_idx)
{
    coredump_test_sub_func_1(test_case_idx);
    return;
}
static qapi_Status_t platform_demo_coredumptest(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    if (Parameter_Count != 1) {
        printf("Invalid number of arguments\r\n");
        printf("===================== unit test command =====================\n");
        printf(
            "Usage: platform coredumptest <test case index: 0~5>\n");
        printf("                              0: Trigger an assertion.\n");
        printf("                              1: Trigger an usage fault or hard fault by executing at null pointer.\n");
        printf("                              2: Trigger an usage fault or hard fault by dividing by zero.\n");
        printf("                              3: Trigger a bus fault or hard fault by reading from a reserved address.\n");
        printf("                              4: Trigger a bus fault or hard fault by writing to a reserved address.\n");
        printf("                              5: Trigger a bus fault or hard fault by executing at a reserved address.\n");
        printf("                              6: Trigger a memory management fault or hard fault by writing to a protected address.\n");
        return QAPI_ERR_INVALID_PARAM;
    }
    if (!Parameter_List[0].Integer_Is_Valid || Parameter_List[0].Integer_Value > 6) {
        printf("Test case index shoud be an integer less than 7.\n");
        return QAPI_ERR_INVALID_PARAM;
    }

    coredump_test_sub_func_0(Parameter_List[0].Integer_Value);

    return QAPI_OK;
}

static qapi_Status_t platform_demo_get_coredumpinfo(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    coredump_type coredump_buf;
    wifi_fw_coredump_header_t wifi_fw_coredump_header;

    int ret = 0;

    if (Parameter_Count != 1) {
        printf("Invalid number of arguments\r\n");
        printf("Usage: platform coredumpinfo <0: get and clear coredumpinfo; 1: get but not clear coredumpinfo>\n");
        return QAPI_ERR_INVALID_PARAM;
    }
    if (!Parameter_List[0].Integer_Is_Valid ||
        (Parameter_List[0].Integer_Value != 0 && Parameter_List[0].Integer_Value != 1)) {
        printf("Invalid parameter, shoud be 0 or 1\n");
        printf("Usage: platform coredumpinfo <0: get and clear coredumpinfo; 1: get but not clear coredumpinfo>\n");
        return QAPI_ERR_INVALID_PARAM;
    }

    /* address: 0x36b000 + offset */
    // ret = qapi_rram_read(COREDUMP_PARTID, , (uint8_t *)&coredump_buf, sizeof(coredump_type));
    ret = qapi_coredump_read(&coredump_buf, 1);

    if (ret == QAPI_OK) {
        printf("\r\n");
        printf("============== coredump start ==============\r\n");
        printf("coredump version = [%d]\r\n", coredump_buf.version);
        printf("os type=[%d]\r\n", coredump_buf.os.type);
        printf("os version = [%d]\r\n", coredump_buf.os.version);
        printf("arch version = [%X]\r\n", coredump_buf.arch.version);
        printf("error message = [%s]\r\n", (char *)coredump_buf.err.message);
        printf("file name = [%s]\r\n", (char *)coredump_buf.err.filename);
        printf("line number = [%d]\r\n", coredump_buf.err.linenum);
        printf("param[0] = [%d]\r\n", coredump_buf.err.param[0]);
        printf("param[1] = [%d]\r\n", coredump_buf.err.param[1]);
        printf("param[2] = [%d]\r\n", coredump_buf.err.param[2]);
        printf("image basic info = [%s]\r\n", coredump_buf.image.image_variant_string);
        printf("iamge build info = [%s]\r\n", coredump_buf.image.qc_image_version_string);
        printf("\r\n");
        printf(
            "reg info:\r\n"
            "R0      %08X  R1        %08X\r\n"
            "R2      %08X  R3        %08X\r\n",
            coredump_buf.arch.regs.name.regs[0], coredump_buf.arch.regs.name.regs[1],
            coredump_buf.arch.regs.name.regs[2], coredump_buf.arch.regs.name.regs[3]);

        printf(
            "R4      %08X  R5        %08X\r\n"
            "R6      %08X  R7        %08X\r\n"
            "R8      %08X  R9        %08X\r\n",
            coredump_buf.arch.regs.name.regs[4], coredump_buf.arch.regs.name.regs[5],
            coredump_buf.arch.regs.name.regs[6], coredump_buf.arch.regs.name.regs[7],
            coredump_buf.arch.regs.name.regs[8], coredump_buf.arch.regs.name.regs[9]);

        printf(
            "R10     %08X  R11       %08X\r\n"
            "R12     %08X  sp        %08X\r\n"
            "lr      %08X  pc        %08X\r\n",
            coredump_buf.arch.regs.name.regs[10], coredump_buf.arch.regs.name.regs[11],
            coredump_buf.arch.regs.name.regs[12], coredump_buf.arch.regs.name.sp, coredump_buf.arch.regs.name.lr,
            coredump_buf.arch.regs.name.pc);

        printf(
            "psp     %08X  msp       %08X\r\n"
            "psr     %08X  aspr      %08X\r\n"
            "ipsr    %08X  epsr      %08X\r\n",
            coredump_buf.arch.regs.name.psp, coredump_buf.arch.regs.name.msp, coredump_buf.arch.regs.name.psr,
            coredump_buf.arch.regs.name.aspr, coredump_buf.arch.regs.name.ipsr, coredump_buf.arch.regs.name.epsr);

        printf(
            "primask %08X  faultmask %08X\r\n"
            "basepri %08X  control   %08X\r\n"
            "exception_r0    %08X   exception_r1   %08X\r\n",
            coredump_buf.arch.regs.name.primask, coredump_buf.arch.regs.name.faultmask,
            coredump_buf.arch.regs.name.basepri, coredump_buf.arch.regs.name.control,
            coredump_buf.arch.regs.name.exception_r0, coredump_buf.arch.regs.name.exception_r1);

        printf(
            "exception_r2    %08X   exception_r3   %08X\r\n"
            "exception_r12   %08X   exception_lr   %08X\r\n"
            "exception_pc    %08X   exception_xpsr %08X\r\n",
            coredump_buf.arch.regs.name.exception_r2, coredump_buf.arch.regs.name.exception_r3,
            coredump_buf.arch.regs.name.exception_r12, coredump_buf.arch.regs.name.exception_lr,
            coredump_buf.arch.regs.name.exception_pc, coredump_buf.arch.regs.name.exception_xpsr);

        printf(
            "ICSR  %08X   VTOR  %08X\r\n"
            "AIRCR %08X   SCR   %08X \r\n"
            "CCR   %08X\r\n",
            coredump_buf.err.config_regs.icsr, coredump_buf.err.config_regs.vtor, coredump_buf.err.config_regs.aircr,
            coredump_buf.err.config_regs.scr, coredump_buf.err.config_regs.ccr);

        printf(
            "SHPR1 %08x  SHPR2 %08x \r\n"
            "SHPR3 %08x\r\n",
            coredump_buf.err.config_regs.shpr1, coredump_buf.err.config_regs.shpr2, coredump_buf.err.config_regs.shpr3);

        printf(
            "SHCSR %08X   CFSR  %08X \r\n"
            "HFSR  %08X   DFSR  %08X \r\n"
            "MMFAR %08X\r\n",
            coredump_buf.err.config_regs.shcsr, coredump_buf.err.config_regs.cfsr, coredump_buf.err.config_regs.hfsr,
            coredump_buf.err.config_regs.dfsr, coredump_buf.err.config_regs.mmfar);

        printf(
            "BFAR  %08X   AFSR %08X \r\n"
            "PFR0  %08X   PFR1 %08X \r\n"
            "DFR   %08X\r\n",
            coredump_buf.err.config_regs.bfar, coredump_buf.err.config_regs.afsr, coredump_buf.err.config_regs.pfr0,
            coredump_buf.err.config_regs.pfr1, coredump_buf.err.config_regs.dfr);

        printf(
            "ADR     %08X   MMFR[0] %08X \r\n"
            "MMFR[1] %08X   MMFR[2] %08X \r\n"
            "MMFR[3] %08X\r\n",
            coredump_buf.err.config_regs.adr, coredump_buf.err.config_regs.mmfr[0],
            coredump_buf.err.config_regs.mmfr[1], coredump_buf.err.config_regs.mmfr[2],
            coredump_buf.err.config_regs.mmfr[3]);

        printf(
            "ISAR[0] %08X  ISAR[1] %08X \r\n"
            "ISAR[2] %08X  ISAR[3] %08X \r\n"
            "ISAR[4] %08X  CPACR   %08X\r\n",
            coredump_buf.err.config_regs.isar[0], coredump_buf.err.config_regs.isar[1],
            coredump_buf.err.config_regs.isar[2], coredump_buf.err.config_regs.isar[3],
            coredump_buf.err.config_regs.isar[4], coredump_buf.err.config_regs.cpacr);

        printf("NVIC_ISPR0 %08X   NVIC_ISPR1 %08X   NVIC_ISPR2 %08X\r\n", coredump_buf.err.config_regs.ispr[0],
               coredump_buf.err.config_regs.ispr[1], coredump_buf.err.config_regs.ispr[2]);

        printf("NVIC_ISER0 %08X   NVIC_ISER1 %08X   NVIC_ISER2 %08X\r\n", coredump_buf.err.config_regs.iser[0],
               coredump_buf.err.config_regs.iser[1], coredump_buf.err.config_regs.iser[2]);
        printf("============== coredump end ==============\r\n");
        printf("\r\n");

        if (Parameter_List[0].Integer_Value == 0) {
            /* clean coredump info, the next crash info will overwrite the RRAM */
            ret = nt_rram_read(WIFI_FW_COREDUMP_HEADER_START_ADDRESS, &wifi_fw_coredump_header,
                               sizeof(wifi_fw_coredump_header_t));
            if (ret == QAPI_OK && wifi_fw_coredump_header.magic_num == WIFi_FW_COREDUMP_MAGIC_NUMBER) {
                /* set magic_num to 0 */
                wifi_fw_coredump_header.magic_num = 0;

                /* for next time, the coredump info will not be saved */
                nt_rram_write(WIFI_FW_COREDUMP_HEADER_START_ADDRESS, &wifi_fw_coredump_header,
                              sizeof(wifi_fw_coredump_header_t));
            } else if (ret != QAPI_OK) {
                printf("fail to clean coredumpinfo\n");
            }
            memset(&coredump_buf, 0, sizeof(coredump_type));
            ret = qapi_rram_write(WIFI_FW_COREDUMP_PARTID, WIFI_FW_COREDUMP_ADDRESS_OFFSET, (uint8_t *)&coredump_buf,
                                  sizeof(coredump_type));
            if (ret == QAPI_OK) {
                printf("coredumpinfo is erased\n");
            } else {
                printf("fail to erase coredumpinfo\n");
            }
        }
    } else {
        printf("fail to read coredumpinfo\r\n");
    }
    return QAPI_OK;
}

static qapi_Status_t platform_demo_set_coredumpflag(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    /* check the number of arguments */
    if (Parameter_Count != 1) {
        printf("Invalid number of arguments\r\n");
        printf("Usage: platform coredumpflag <0|1: if print all the ram info>\n");
        return QAPI_ERR_INVALID_PARAM;
    }

    /* input value should be 0 or 1*/
    if (!Parameter_List[0].Integer_Is_Valid ||
        (Parameter_List[0].Integer_Value != 0 && Parameter_List[0].Integer_Value != 1)) {
        printf("Invalid parameter, shoud be 0 or 1\n");
        printf("Usage: platform coredumpflag <0|1: if print all the ram info>\n");
        return QAPI_ERR_INVALID_PARAM;
    }

    /* set ramdump print flag */
    if (Parameter_List[0].Integer_Value == 0) {
        qapi_set_ramdump_flag(0);
    } else {
        qapi_set_ramdump_flag(1);
    }
    return QAPI_OK;
}
#endif

const QAPI_Console_Command_t platform_shell_cmds[] = {
    // cmd_function    cmd_string               usage_string             description
    {platform_reset, "reset", "", "reset the platform\n"},
#ifdef NT_FN_DEBUG_STATS
    {read_mem, "read_mem", "<addr> <size:1|2|4>", "read memory\n"},
    {write_mem, "write_mem", "<addr> <size:1|2|4> <value>", "write memory\n"},
#endif
    {rram_read, "rram_read", "<address> <count>", "Read rram data"},
    {rram_write, "rram_write", "<address>  <count> <string>", "Write data to rram, count <= 200"},
    {rram_test, "rram_test", "<address> <size(KB)>",
     "rram data test. size <=64.\n"
     "write test will write 0~16 in cycles and verify it \n"},
    {bgtest, "bgtest", "[time_s(5)] [interval_s(1)]", "background command test\n"},
    {platform_demo_free, "free", "\n", "display the heap size and an approximation of free amount of heap bytes\n"},
    {platform_demo_watchdog_reset, "wdrst", "\n", "trigger watchdog reset\n"},
    {platform_demo_time, "time", "\n", "get/set current time in Julian format\n"},
    {platform_demo_time_ntp, "time_ntp", "\n", "get/set current time in NTP format\n"},
    {platform_demo_info, "info", "\n", "show system information\n"},
    {platform_demo_getcx, "getcx", "\n", "get cx(ULP-SMPS2) related information\n"},
    {platform_demo_calcxoneshot, "calcxoneshot", "<tempC> <vbatmV>",
     "calculate cx(ULP-SMPS2) oneshot_code accordting to tempC(-40C, 125C) and vbatmV(1600mV, 3600mV)\n"},
    {platform_demo_setcxoneshot, "setcxoneshot", "<oneshot> [tempC] [vbatmV]\n",
     "if oneshot not zero, just set; else, calculate oneshot according to tempC and vbatmV then set. This will disable "
     "cxoneshot update in sleep\n"},
    {platform_demo_time_zone, "time_zone", "<zone>\n", "set time zone\n"},
#ifdef SUPPORT_QCSPI_SLAVE
    {platform_qcspi_enable, "qcspi", "<0|1>\n", "enable/disable qcspi. 1: enable, 0:disable\n"},
#endif
    {platform_demo_check_boot_reason, "boot_reason", "\n", "check boot reason\n"},
#ifdef CONFIG_WIFI_FW_COREDUMP_SUPPORT
    {platform_demo_coredumptest, "coredumptest", "<test case index: 0~6>\n", "unit test for coredump function\n"},
    {platform_demo_get_coredumpinfo, "coredumpinfo", "<flag:0|1>\n", "dump the coredump info\n"},
    {platform_demo_set_coredumpflag, "coredumpflag", "<flag:0|1>\n", "flag indicating if dump the whole ram info\n"},
#endif
};

const QAPI_Console_Command_Group_t platform_shell_cmd_group = {
    "platform", sizeof(platform_shell_cmds) / sizeof(QAPI_Console_Command_t), platform_shell_cmds};

QAPI_Console_Group_Handle_t platform_shell_cmd_group_handle;

void platform_shell_init(void)
{
    platform_shell_cmd_group_handle = QAPI_Console_Register_Command_Group(NULL, &platform_shell_cmd_group);
}
