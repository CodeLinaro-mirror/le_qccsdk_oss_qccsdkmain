/*
 */
#include "fwconfig_cmn.h"
#include "nt_flags.h"
#include "nt_osal.h"
#include "nt_common.h"
#include "nt_wfm_wmi_interface.h"

NT_BOOL wmi_pdev_utf_cmd(wmi_msg_struct_t* msg)
{
#ifdef CONFIG_FTM_MODE
    extern uint8_t ftm_parse_tlv_cmd(uint8_t * buf, uint32_t dataLength);
    ftm_parse_tlv_cmd((uint8_t*)msg->msg_struct.vo_data, msg->msg_struct.vo_data_len);
#else /* CONFIG_FTM_MODE */
    (void)msg;
#endif /* CONFIG_FTM_MODE */
    return TRUE;
}
void wmi_unit_test_cmd_handler(WMI_UNIT_TEST_CMD *cmd)
{
#ifdef UNIT_TEST_SUPPORT
    extern void wmi_unit_test_internal_cmd_handler(WMI_UNIT_TEST_CMD *cmd);
    wmi_unit_test_internal_cmd_handler(cmd);
#else /* UNIT_TEST_SUPPORT */
    (void)cmd;
#endif /* UNIT_TEST_SUPPORT */
    return;
}
