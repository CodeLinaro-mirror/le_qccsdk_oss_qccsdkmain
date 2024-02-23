/*
 */

#include <stdio.h>
#include <ctype.h>

#include "qapi_status.h"
#include "qapi_console.h"
#include "qcli_api.h"

#include "timer.h"
#include "qapi_flash.h"

#define FLASH_SHELL_INFO 1
#define FLASH_SHELL_GROUP_NAME    "FLASH"
#define FLASH_SHELL_GROUP_PRINTF_SUFFIX  "FLASH: "

#if FLASH_SHELL_INFO
#define flash_printf(msg,...)     printf(FLASH_SHELL_GROUP_PRINTF_SUFFIX msg, ##__VA_ARGS__)
#else
#define flash_printf(args...)     do { } while (0)
#endif

bool flash_init_done;

static qapi_Status_t Init(uint32_t __attribute__((__unused__)) Parameter_Count, QAPI_Console_Parameter_t __attribute__((__unused__)) *Parameter_List)
{
    qapi_Status_t status;

    if(flash_init_done) {
        flash_printf("Flash was alredy inited\n");
        return QAPI_OK;
    }

    status = qapi_Flash_Init();
    if (status != QAPI_OK) {
        flash_printf("ERROR: Init failed %d\n", status);
    }
    else {
        flash_init_done = true;
        flash_printf("Init success\n");
    }

    return status;
}

static qapi_Status_t Read(uint32_t __attribute__((__unused__)) Parameter_Count, QAPI_Console_Parameter_t __attribute__((__unused__)) *Parameter_List)
{
    qapi_Status_t status;
    uint32_t i;
    uint32_t address;
    uint32_t byte_cnt;
    uint8_t *buffer = NULL;
    uint64_t start_time,end_time;

    if(!flash_init_done) {
      flash_printf("Flash was not inited\n");
      return QAPI_ERROR;
    }

    if (Parameter_Count != 2 || Parameter_List == NULL ||
        Parameter_List[0].Integer_Value < 0 || Parameter_List[1].Integer_Value <= 0 ) {
        flash_printf("Read <Addr> <Cnt>\n");
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }
    address = Parameter_List[0].Integer_Value;
    byte_cnt = Parameter_List[1].Integer_Value;

    buffer = malloc(byte_cnt);
    if (buffer == NULL)
    {
        flash_printf("ERROR: No enough memory\n");
        return QAPI_ERR_NO_MEMORY;
    }
    memset(buffer, 0, byte_cnt);

    start_time = hres_timer_curr_time_us();
    status = qapi_Flash_Read(address, byte_cnt, buffer);
    end_time = hres_timer_curr_time_us();

    if(status == QAPI_OK) {
        flash_printf("Read result(len=%dbytes,time=%ldus): \n", byte_cnt, end_time - start_time);
        for(i=0; i<byte_cnt; i++){
            printf("0x%02x ",buffer[i]);
            if((i+1) % 16 == 0)
                printf("\n");
            }
    }
    else {
        flash_printf("Flash read error:%d\n",status);
    }

    free(buffer);
    return status;
}

static qapi_Status_t Write(uint32_t __attribute__((__unused__)) Parameter_Count, QAPI_Console_Parameter_t __attribute__((__unused__)) *Parameter_List)
{
    qapi_Status_t status;
    uint32_t address;
    uint32_t byte_cnt;
    uint8_t *buffer = NULL;
    uint64_t start_time,end_time;

    if(!flash_init_done) {
      flash_printf("Flash was not inited\n");
      return QAPI_ERROR;
    }

    if (Parameter_Count != 3 || Parameter_List == NULL ||
        Parameter_List[0].Integer_Value < 0 || Parameter_List[1].Integer_Value <= 0 ) {
        flash_printf("Write <Addr> <Cnt> <Value string>\n");
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }

    address = Parameter_List[0].Integer_Value;
    byte_cnt = Parameter_List[1].Integer_Value;
    buffer = (uint8_t*)Parameter_List[2].String_Value;
    if(byte_cnt > 200) {
        /* The max len of QLI buffer is 256 bytes, here should be a limitation */
        flash_printf("The length should be less than 200Bytes\n");
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }

    start_time = hres_timer_curr_time_us();
    status = qapi_Flash_Write(address, byte_cnt, buffer);
    end_time = hres_timer_curr_time_us();

    if(status == QAPI_OK) {
        flash_printf("Flash write done(len=%dbytes,time=%ldus)\n", byte_cnt, end_time - start_time);
    }
    else {
        flash_printf("Flash write error:%d\n",status);
    }

    return status;
}

static qapi_Status_t Erase(uint32_t __attribute__((__unused__)) Parameter_Count, QAPI_Console_Parameter_t __attribute__((__unused__)) *Parameter_List)
{
    qapi_Status_t status;
    uint32_t type;
    uint32_t address;
    uint32_t cnt;
    uint32_t start_time,end_time;

    if(!flash_init_done) {
      flash_printf("Flash was not inited\n");
      return QAPI_ERROR;
    }

    if (Parameter_Count != 3 || Parameter_List == NULL ||
        Parameter_List[0].Integer_Value < 0 || Parameter_List[1].Integer_Value < 0 ||
        Parameter_List[2].Integer_Value < 0 ) {
        flash_printf("Erase <type> <Addr> <Cnt>\n");
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }
    type = Parameter_List[0].Integer_Value;
    address = Parameter_List[1].Integer_Value;
    cnt = Parameter_List[2].Integer_Value;

    start_time = hres_timer_curr_time_ms();
    status = qapi_Flash_Erase(type, address, cnt);
    end_time = hres_timer_curr_time_ms();

    if(status == QAPI_OK) {
        flash_printf("Flash erase done(time=%ldms)\n", end_time - start_time);
    }
    else {
        flash_printf("Flash erase error:%d\n",status);
    }

    return status;
}

static qapi_Status_t Readreg(uint32_t __attribute__((__unused__)) Parameter_Count, QAPI_Console_Parameter_t __attribute__((__unused__)) *Parameter_List)
{
    qapi_Status_t status;
    uint32_t address;
    uint32_t byte_cnt;
    uint32_t i;
    uint8_t buffer[8];

    if(!flash_init_done) {
      flash_printf("Flash was not inited\n");
      return QAPI_ERROR;
    }

    if (Parameter_Count != 2 || Parameter_List == NULL ||
        Parameter_List[0].Integer_Value < 0 || Parameter_List[1].Integer_Value <= 0 ||
        Parameter_List[1].Integer_Value >8) {
        flash_printf("Readreg <Addr> <Cnt(<8)>\n");
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }
    address = Parameter_List[0].Integer_Value;
    byte_cnt = Parameter_List[1].Integer_Value;

    memset(buffer, 0, byte_cnt);

    status = qapi_Flash_Read_Reg(address, byte_cnt, buffer);

    if(status == QAPI_OK) {
        flash_printf("Read register result(len=%d): ", byte_cnt);
        for(i=0; i<byte_cnt; i++)
            printf("0x%02x ",buffer[i]);
        printf("\n");
    }
    else {
        flash_printf("Read register error:%d\n",status);
    }

    return status;
}

static qapi_Status_t Writereg(uint32_t __attribute__((__unused__)) Parameter_Count, QAPI_Console_Parameter_t __attribute__((__unused__)) *Parameter_List)
{
    qapi_Status_t status;
    uint32_t address;
    uint32_t byte_cnt = 0;
    uint32_t value = 0;

    if(!flash_init_done) {
      flash_printf("Flash was not inited\n");
      return QAPI_ERROR;
    }

    if ( (Parameter_Count != 1 && Parameter_Count != 3) ||Parameter_List == NULL ||
        Parameter_List[0].Integer_Value < 0 || Parameter_List[1].Integer_Value <0 ) {
        flash_printf("Write <Addr> [Cnt] [Value]\n");
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }
    address = Parameter_List[0].Integer_Value;
    if(Parameter_Count > 1) {
        byte_cnt = Parameter_List[1].Integer_Value;
        value = Parameter_List[2].Integer_Value;
    }
    status = qapi_Flash_Write_Reg(address, byte_cnt, (uint8_t*)&value);

    if(status == QAPI_OK) {
        flash_printf("Flash write register done\n");
    }
    else {
        flash_printf("Flash write error:%d\n",status);
    }

    return status;
}

#define FLASH_OP_UNIT 1024*4
static qapi_Status_t Test(uint32_t __attribute__((__unused__)) Parameter_Count, QAPI_Console_Parameter_t __attribute__((__unused__)) *Parameter_List)
{
    qapi_Status_t status;
    uint32_t i, op, len;
    uint32_t address;
    uint32_t kbyte_cnt;
    uint8_t *buffer = NULL;
    uint8_t *read_buffer = NULL;
    uint64_t start_time,end_time;

    if(!flash_init_done) {
      flash_printf("Flash was not inited\n");
      return QAPI_ERROR;
    }

    if (Parameter_Count != 3 || Parameter_List == NULL ||
        Parameter_List[0].Integer_Value < 0 || Parameter_List[1].Integer_Value < 0 ||
        Parameter_List[2].Integer_Value <= 0) {
        flash_printf("Parameter error! Use--Test <op> <address> <size(KB)>\n");
		flash_printf("<op>: 0-read; 1-write\n");
		flash_printf("<size(KB)>: less 1024K(unit 1K)\n");
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }
    op = Parameter_List[0].Integer_Value;
    address = Parameter_List[1].Integer_Value;
    kbyte_cnt = Parameter_List[2].Integer_Value;
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

    if(op != 0 && op != 1) {
        flash_printf("op should be 0(read) or 1(write)\n");
        return QAPI_ERR_INVALID_PARAM;
    }
    if(kbyte_cnt > 1024) { //1024KB
        flash_printf("Test size should less 1024K(unit 1K)\n");
        return QAPI_ERR_INVALID_PARAM;
    }
    flash_printf("op: %d,cnt %d K, unit %d B\n",op, kbyte_cnt, FLASH_OP_UNIT);
    for(i=0; i<FLASH_OP_UNIT; i++)
        buffer[i] = i % 256;

    start_time = hres_timer_curr_time_us();
    while(kbyte_cnt) {
        if(kbyte_cnt * 1024 > FLASH_OP_UNIT) {
            len = FLASH_OP_UNIT;
        }else {
            len = kbyte_cnt * 1024;
        }

        if(op == 1)
            status = qapi_Flash_Write(address, len, buffer);
        else
            status = qapi_Flash_Read(address, len, read_buffer);
        if(status != QAPI_OK) {
            flash_printf("Buf(%d) test failed(%d)\n",i,status);
            break;
        }

        if(op == 0 && memcmp(read_buffer, buffer, len) != 0) {
            status = QAPI_ERROR;
            flash_printf("Verify failed for read test, compare with the written value by flash test\n");
            break;
        }
        address += len;
        kbyte_cnt -= (len/1024);
    }
    end_time = hres_timer_curr_time_us();

    if(status == QAPI_OK) {
        flash_printf("Flash test done(len=%dbytes,time=%ldus)\n", kbyte_cnt*1024, end_time - start_time);
    }
    /* Verify if write test */
    if(op == 1) {
        address = Parameter_List[1].Integer_Value;
        kbyte_cnt = Parameter_List[2].Integer_Value;
        while(kbyte_cnt) {
            memset(read_buffer, 0, FLASH_OP_UNIT);
            if(kbyte_cnt * 1024 > FLASH_OP_UNIT) {
                len = FLASH_OP_UNIT;
            }else {
                len = kbyte_cnt * 1024;
            }

            status = qapi_Flash_Read(address, len, read_buffer);
            if(status != QAPI_OK) {
                flash_printf("Buf(%d) test failed(%d)\n",i,status);
                break;
            }

            if(memcmp(read_buffer, buffer, len) != 0) {
                status = QAPI_ERROR;
                flash_printf("Verify failed for write test, compare with the written value by flash test\n");
                break;
            }
            address += len;
            kbyte_cnt -= (len/1024);
        }
    }

    free(buffer);
    free(read_buffer);
    return status;
}

const QAPI_Console_Command_t flash_shell_cmds[] =
{
    // cmd_function    cmd_string               usage_string             description
    { Init,          "Init",                "",                      "Flash init"},
    { Read,         "Read",               "<address> <count>",                      "Read flash data"},
    { Write,        "Write",         	"<address>  <count> <string>",                      "Write data to flash, count <= 200"},
    { Erase,       "Erase",             "<type> <Start> <count>",    "Erase flash memory: \n" \
                                                                "type=[0|1|2], 0--4Kbytes, 1--64Kbytes, 2--full chip \n" \
                                                                "Start=the starting block/bulk to erase (if type=2, Start is 0);\n" \
                                                                "count= the number to erase(if type=2, count is 1)"},
    { Readreg,            "Readreg",                  "<reg address> <len>",    "Read flash register"},
    { Writereg, "Writereg",      "<reg address> [data] [len]",          "Write flash register"},
    { Test, "Test",      "<op(0:read|1:write)> <address> <size(KB)>",    "Flash bulk data test. size <=1024.\n"\
                            "write test will write 0~255 in cycles and verify it \n" \
                            "read test will read the data and verify it assuming that the data is peroidic 0~255"},
};

const QAPI_Console_Command_Group_t flash_shell_cmd_group =
    {FLASH_SHELL_GROUP_NAME, sizeof(flash_shell_cmds) / sizeof(QAPI_Console_Command_t), flash_shell_cmds};

QAPI_Console_Group_Handle_t flash_shell_cmd_group_handle;

void flash_shell_init (void)
{
    flash_init_done = false;
    flash_shell_cmd_group_handle = QAPI_Console_Register_Command_Group(NULL, &flash_shell_cmd_group);
}

