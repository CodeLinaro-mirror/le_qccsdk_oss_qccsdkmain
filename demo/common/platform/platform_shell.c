/*
 */

#include "qapi_types.h"
#include "qapi_version.h"
#include "qapi_status.h"
#include "qapi_heap_status.h"

#include "qapi_console.h"

#include <stdio.h>

#include "nt_sys_monitoring.h"
#include "nt_flags.h"
#include "qurt_internal.h"

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
};

const QAPI_Console_Command_Group_t platform_shell_cmd_group = {"platform", sizeof(platform_shell_cmds) / sizeof(QAPI_Console_Command_t), platform_shell_cmds};

QAPI_Console_Group_Handle_t platform_shell_cmd_group_handle;

void platform_shell_init (void)
{
    platform_shell_cmd_group_handle = QAPI_Console_Register_Command_Group(NULL, &platform_shell_cmd_group);
}

