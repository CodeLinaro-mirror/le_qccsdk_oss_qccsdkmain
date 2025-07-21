/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <stdio.h>
#include <ctype.h>

#include "qapi_status.h"
#include "qapi_console.h"
#include "qcli_api.h"

#include "timer.h"
#include "qapi_flash.h"
#ifdef FLASH_XIP_SUPPORT
#include "nt_mem.h"
#endif
#define FLASH_SHELL_INFO                1
#define FLASH_SHELL_GROUP_NAME          "FLASH"
#define FLASH_SHELL_GROUP_PRINTF_SUFFIX "FLASH: "

#if FLASH_SHELL_INFO
#define flash_printf(msg, ...) printf(FLASH_SHELL_GROUP_PRINTF_SUFFIX msg, ##__VA_ARGS__)
#else
#define flash_printf(args...) \
    do {                      \
    } while (0)
#endif

bool flash_init_done;

static qapi_Status_t Init(uint32_t __attribute__((__unused__)) Parameter_Count,
                          QAPI_Console_Parameter_t __attribute__((__unused__)) * Parameter_List)
{
    qapi_Status_t status;
    flash_info_t flash_info;

    if (flash_init_done) {
        flash_printf("Flash was alredy inited\n");
        return QAPI_OK;
    }

    status = qapi_Flash_Init();
    if (status != QAPI_OK) {
        flash_printf("ERROR: Init failed %d\n", status);
    } else {
        flash_init_done = true;
        flash_printf("Init success\n");

        status = qapi_Flash_Get_Info(&flash_info);
        if (status != QAPI_OK) {
            flash_printf("ERROR: get flash info failed %d\n", status);
        } else {
            flash_printf("********FLASH INFO********\n");
            flash_printf("device ID: 0x%x \n", flash_info.devicd_id);
            flash_printf("total size: %d Mbytes\n",
                         (flash_info.block_count * flash_info.block_size_bytes) / (1024 * 1024));
            flash_printf("page size: %d bytes\n", flash_info.page_size_bytes);
            flash_printf("block size: %d bytes\n", flash_info.block_size_bytes);
        }
    }
    return status;
}

static qapi_Status_t Read(uint32_t __attribute__((__unused__)) Parameter_Count,
                          QAPI_Console_Parameter_t __attribute__((__unused__)) * Parameter_List)
{
    qapi_Status_t status;
    uint32_t i;
    uint32_t address;
    uint32_t byte_cnt;
    uint8_t *buffer = NULL;
    uint64_t start_time, end_time;

    if (!flash_init_done) {
        flash_printf("Flash was not inited\n");
        return QAPI_ERROR;
    }

    if (Parameter_Count != 2 || Parameter_List == NULL || Parameter_List[0].Integer_Value < 0 ||
        Parameter_List[1].Integer_Value <= 0) {
        flash_printf("Read <Addr> <Cnt>\n");
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }
    address = Parameter_List[0].Integer_Value;
    byte_cnt = Parameter_List[1].Integer_Value;

    buffer = malloc(byte_cnt);
    if (buffer == NULL) {
        flash_printf("ERROR: No enough memory\n");
        return QAPI_ERR_NO_MEMORY;
    }
    memset(buffer, 0, byte_cnt);

    start_time = hres_timer_curr_time_us();
    status = qapi_Flash_Read(address, byte_cnt, buffer);
    end_time = hres_timer_curr_time_us();

    if (status == QAPI_OK) {
        flash_printf("Read result(len=%dbytes,time=%ldus): \n", byte_cnt, end_time - start_time);
        for (i = 0; i < byte_cnt; i++) {
            printf("0x%02x ", buffer[i]);
            if ((i + 1) % 16 == 0)
                printf("\n");
        }
    } else {
        flash_printf("Flash read error:%d\n", status);
    }

    free(buffer);
    return status;
}

static qapi_Status_t Write(uint32_t __attribute__((__unused__)) Parameter_Count,
                           QAPI_Console_Parameter_t __attribute__((__unused__)) * Parameter_List)
{
    qapi_Status_t status;
    uint32_t address;
    uint32_t byte_cnt;
    uint8_t *buffer = NULL;
    uint64_t start_time, end_time;

    if (!flash_init_done) {
        flash_printf("Flash was not inited\n");
        return QAPI_ERROR;
    }

    if (Parameter_Count != 3 || Parameter_List == NULL || Parameter_List[0].Integer_Value < 0 ||
        Parameter_List[1].Integer_Value <= 0) {
        flash_printf("Write <Addr> <Cnt> <Value string>\n");
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }

    address = Parameter_List[0].Integer_Value;
    byte_cnt = Parameter_List[1].Integer_Value;
    buffer = (uint8_t *)Parameter_List[2].String_Value;
    if (byte_cnt > 200) {
        /* The max len of QLI buffer is 256 bytes, here should be a limitation */
        flash_printf("The length should be less than 200Bytes\n");
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }

    start_time = hres_timer_curr_time_us();
    status = qapi_Flash_Write(address, byte_cnt, buffer);
    end_time = hres_timer_curr_time_us();

    if (status == QAPI_OK) {
        flash_printf("Flash write done(len=%dbytes,time=%ldus)\n", byte_cnt, end_time - start_time);
    } else {
        flash_printf("Flash write error:%d\n", status);
    }

    return status;
}

static qapi_Status_t Erase(uint32_t __attribute__((__unused__)) Parameter_Count,
                           QAPI_Console_Parameter_t __attribute__((__unused__)) * Parameter_List)
{
    qapi_Status_t status;
    uint32_t type;
    uint32_t address;
    uint32_t cnt;
    uint32_t start_time, end_time;

    if (!flash_init_done) {
        flash_printf("Flash was not inited\n");
        return QAPI_ERROR;
    }

    if (Parameter_Count != 3 || Parameter_List == NULL || Parameter_List[0].Integer_Value < 0 ||
        Parameter_List[1].Integer_Value < 0 || Parameter_List[2].Integer_Value < 0) {
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }
    type = Parameter_List[0].Integer_Value;
    address = Parameter_List[1].Integer_Value;
    cnt = Parameter_List[2].Integer_Value;

    start_time = hres_timer_curr_time_ms();
    status = qapi_Flash_Erase(type, address, cnt);
    end_time = hres_timer_curr_time_ms();

    if (status == QAPI_OK) {
        flash_printf("Flash erase done(time=%ldms)\n", end_time - start_time);
    } else {
        flash_printf("Flash erase error:%d\n", status);
    }

    return status;
}

#ifdef FLASH_XIP_SUPPORT
static qapi_Status_t rram_read_test(uint32_t __attribute__((__unused__)) Parameter_Count,
                                    QAPI_Console_Parameter_t __attribute__((__unused__)) * Parameter_List)
{
    qapi_Status_t status = QAPI_OK;
    uint32_t i;
    uint32_t address;
    uint32_t byte_cnt;
    uint8_t *buffer = NULL;
    uint64_t start_time, end_time;

    flash_printf("test rram read in xip mode\n");

    if (Parameter_Count != 2 || Parameter_List == NULL || Parameter_List[0].Integer_Value < 0 ||
        Parameter_List[1].Integer_Value <= 0) {
        flash_printf("Read <Addr> <Cnt>\n");
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }
    address = Parameter_List[0].Integer_Value;
    byte_cnt = Parameter_List[1].Integer_Value;

    buffer = malloc(byte_cnt);
    if (buffer == NULL) {
        flash_printf("ERROR: No enough memory\n");
        return QAPI_ERR_NO_MEMORY;
    }
    memset(buffer, 0, byte_cnt);

    start_time = hres_timer_curr_time_us();

    if (nt_rram_read(address, buffer, byte_cnt) == 0) {
        flash_printf("Read Address : %08x, Data : %08x", address, *buffer);
    } else {
        flash_printf("Read Failed");
    }

    end_time = hres_timer_curr_time_us();
#if 1
    if (status == QAPI_OK) {
        flash_printf("Read result(len=%dbytes,time=%ldus): ", byte_cnt, end_time - start_time);
        for (i = 0; i < byte_cnt; i++)
            printf("0x%02x ", buffer[i]);
        printf("\n");
    } else {
        flash_printf("RRAM read error:%x\n", status);
        status = QAPI_ERROR;
    }
#endif
    free(buffer);
    return status;
}
static qapi_Status_t rram_write_test(uint32_t __attribute__((__unused__)) Parameter_Count,
                                     QAPI_Console_Parameter_t __attribute__((__unused__)) * Parameter_List)
{
    qapi_Status_t status = QAPI_OK;
    uint32_t address;
    uint32_t byte_cnt;
    uint32_t *buffer = NULL;
    // uint8_t buffer[4];
    uint64_t start_time, end_time;

    if (Parameter_Count != 3 || Parameter_List == NULL || Parameter_List[0].Integer_Value < 0 ||
        Parameter_List[1].Integer_Value <= 0) {
        flash_printf("Write <Addr> <Cnt> <Value string>\n");
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }

    address = Parameter_List[0].Integer_Value;
    byte_cnt = Parameter_List[1].Integer_Value;
    buffer = (uint32_t *)Parameter_List[2].String_Value;
    // memcpy(&buffer, Parameter_List[2].String_Value, byte_cnt);
    if (byte_cnt > 200) {
        /* The max len of QLI buffer is 256 bytes, here should be a limitation */
        flash_printf("The length should be less than 200Bytes\n");
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }

    start_time = hres_timer_curr_time_us();
    if (nt_rram_write(address, buffer, byte_cnt) == 0) {
        flash_printf("dxe rram Write Address : %08x, Data : %08x", address, buffer);
    } else {
        flash_printf(, "Write Failed");
    }
    end_time = hres_timer_curr_time_us();

    if (status == QAPI_OK) {
        flash_printf("RRAM write done(len=%dbytes,time=%ldus)\n", byte_cnt, end_time - start_time);
    } else {
        flash_printf("RRAM write error:%x\n", status);
        status = QAPI_ERROR;
    }

    return status;
}
#endif

#define FLASH_OP_UNIT 1024 * 4
static qapi_Status_t Test(uint32_t __attribute__((__unused__)) Parameter_Count,
                          QAPI_Console_Parameter_t __attribute__((__unused__)) * Parameter_List)
{
    qapi_Status_t status = QAPI_OK;
    uint32_t i, len;
    uint32_t address;
    uint32_t kbyte_cnt;
    uint8_t *buffer = NULL;
    uint8_t *read_buffer = NULL;
    uint64_t start_time, end_time;

    if (!flash_init_done) {
        flash_printf("Flash was not inited\n");
        return QAPI_ERROR;
    }

    if (Parameter_Count != 2 || Parameter_List == NULL || Parameter_List[0].Integer_Value < 0 ||
        Parameter_List[1].Integer_Value <= 0) {
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }

    address = Parameter_List[0].Integer_Value;
    kbyte_cnt = Parameter_List[1].Integer_Value;
    buffer = malloc(FLASH_OP_UNIT);
    if (buffer == NULL) {
        flash_printf("ERROR: No enough memory\n");
        return QAPI_ERR_NO_MEMORY;
    }
    read_buffer = malloc(FLASH_OP_UNIT);
    if (read_buffer == NULL) {
        flash_printf("ERROR: No enough memory\n");
        free(buffer);
        return QAPI_ERR_NO_MEMORY;
    }

    if (kbyte_cnt > 1024) {  // 1024KB
        flash_printf("Test size should less 1024K(unit 1K)\n");
        return QAPI_ERR_INVALID_PARAM;
    }
    flash_printf("Total test size %d Kbytes\n", kbyte_cnt);
    for (i = 0; i < FLASH_OP_UNIT; i++)
        buffer[i] = i % 256;

    start_time = hres_timer_curr_time_us();
    while (kbyte_cnt) {
        if (kbyte_cnt * 1024 > FLASH_OP_UNIT) {
            len = FLASH_OP_UNIT;
        } else {
            len = kbyte_cnt * 1024;
        }

        status = qapi_Flash_Write(address, len, buffer);

        if (status != QAPI_OK) {
            flash_printf("Buf(%d) test failed(%d)\n", i, status);
            break;
        }

        address += len;
        kbyte_cnt -= (len / 1024);
    }
    end_time = hres_timer_curr_time_us();

    if (status == QAPI_OK) {
        flash_printf("Flash write done(time=%ldus)\n", (uint32_t)(end_time - start_time));
    }

    /* Verify */
    address = Parameter_List[0].Integer_Value;
    kbyte_cnt = Parameter_List[1].Integer_Value;
    start_time = hres_timer_curr_time_us();
    while (kbyte_cnt) {
        memset(read_buffer, 0, FLASH_OP_UNIT);
        if (kbyte_cnt * 1024 > FLASH_OP_UNIT) {
            len = FLASH_OP_UNIT;
        } else {
            len = kbyte_cnt * 1024;
        }

        status = qapi_Flash_Read(address, len, read_buffer);
        if (status != QAPI_OK) {
            flash_printf("Flash read test failed(%d)\n", i, status);
            break;
        }

        if (memcmp(read_buffer, buffer, len) != 0) {
            status = QAPI_ERROR;
            flash_printf("Verify failed at address 0x%x\n", address);
            break;
        }
        address += len;
        kbyte_cnt -= (len / 1024);
    }
    end_time = hres_timer_curr_time_us();

    if (status == QAPI_OK) {
        flash_printf("Flash read/verify done(time=%ldus)\n", (uint32_t)(end_time - start_time));
    }

    free(buffer);
    free(read_buffer);
    return status;
}

static qapi_Status_t Info(uint32_t __attribute__((__unused__)) Parameter_Count,
                          QAPI_Console_Parameter_t __attribute__((__unused__)) * Parameter_List)
{
    qapi_Status_t status;
    flash_info_t flash_info;

    if (!flash_init_done) {
        flash_printf("Flash was not inited\n");
        return QAPI_ERROR;
    }

    status = qapi_Flash_Get_Info(&flash_info);
    if (status != QAPI_OK) {
        flash_printf("ERROR: get flash info failed %d\n", status);
    } else {
        flash_printf("********FLASH INFO********\n");
        flash_printf("device ID: 0x%x \n", flash_info.devicd_id);
        flash_printf("total size: %d Mbytes\n", (flash_info.block_count * flash_info.block_size_bytes) / (1024 * 1024));
        flash_printf("page size: %d bytes\n", flash_info.page_size_bytes);
        flash_printf("block size: %d bytes\n", flash_info.block_size_bytes);
    }

    return status;
}

const QAPI_Console_Command_t flash_shell_cmds[] = {
    // cmd_function    cmd_string               usage_string             description
    {Init, "Init", "", "Flash init"},
    {Read, "Read", "<address> <count>", "Read flash data"},
    {Write, "Write", "<address>  <count> <string>", "Write data to flash, count <= 200"},
    {Erase, "Erase", "<type> <Start> <count>",
     "Erase flash memory: \n"
     "type=[0|1|2], 0--4Kbytes, 1--64Kbytes, 2--full chip \n"
     "Start=the starting block/bulk to erase (if type=2, Start is 0);\n"
     "count= the number to erase(if type=2, count is 1)"},
    {Test, "Test", "<address> <size(KB)>",
     "Flash bulk data test. size <=1024.\n"
     "write test will write 0~255 in cycles and verify it \n"},
#ifdef FLASH_XIP_SUPPORT
    {rram_read_test, "read rram test", "<address> <count>", "read rram test"},
    {rram_write_test, "write rram test", "<address>  <count> <string>", "write rram test"},
#endif
    {Info, "Info", "", "Flash info"},
};

const QAPI_Console_Command_Group_t flash_shell_cmd_group = {
    FLASH_SHELL_GROUP_NAME, sizeof(flash_shell_cmds) / sizeof(QAPI_Console_Command_t), flash_shell_cmds};

QAPI_Console_Group_Handle_t flash_shell_cmd_group_handle;

void flash_shell_init(void)
{
    flash_init_done = false;
    flash_shell_cmd_group_handle = QAPI_Console_Register_Command_Group(NULL, &flash_shell_cmd_group);
}
