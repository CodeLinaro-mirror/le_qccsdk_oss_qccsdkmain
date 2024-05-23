
#include <stdio.h>
#include "qapi_types.h"
#include "qapi_version.h"
#include "qapi_status.h"
#include "qapi_console.h"
#include "wmi.h"


extern qapi_Status_t wmi_cmd_send (WMI_COMMAND_ID cmd_id, void *p_data, uint32_t data_len);
extern void *memset(void *dest, int c, size_t n);

#ifdef SUPPORT_UNIT_TEST_CMD
WMI_UNIT_TEST_CMD g_unittest_wmi;
#endif
static qapi_Status_t unitest_module(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
#ifdef SUPPORT_UNIT_TEST_CMD
    uint8_t argc_index = 0;
    WMI_UNIT_TEST_CMD *pdata = (WMI_UNIT_TEST_CMD *)&g_unittest_wmi;

    if( Parameter_Count < 3 || !Parameter_List ) {
        printf("invalid parameter!\n");
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }
    memset(pdata, 0, sizeof(*pdata));
    pdata->vdev_id = Parameter_List[0].Integer_Value;
    pdata->module_id = Parameter_List[1].Integer_Value ;
    pdata->num_args = Parameter_List[2].Integer_Value ;
    for(argc_index=0; argc_index < pdata->num_args; argc_index++)
    {
        pdata->args[argc_index] = Parameter_List[3+argc_index].Integer_Value;
    }
    printf("ut cmd: vid=%u, moduleid=%u, num_args=%u\n", pdata->vdev_id, pdata->module_id,pdata->num_args);
    wmi_cmd_send(WMI_UNIT_TEST_CMDID, pdata, sizeof(*pdata));
    return QAPI_OK;
#else
    return QAPI_ERROR;
#endif
}

const QAPI_Console_Command_t unittest_shell_cmds[] =
{
   // cmd_function    cmd_string               usage_string             description
   {unitest_module, "unittest", "<vdev_id> <modul_id> <num_args> <arg[]...>", "unit test cmd. \n"},

};

const QAPI_Console_Command_Group_t unittest_shell_cmd_group = {"FTM", sizeof(unittest_shell_cmds) / sizeof(QAPI_Console_Command_t), unittest_shell_cmds};

QAPI_Console_Group_Handle_t unittest_shell_cmd_group_handle;

void unittest_shell_init(void)
{
    unittest_shell_cmd_group_handle = QAPI_Console_Register_Command_Group(NULL, &unittest_shell_cmd_group);
}

