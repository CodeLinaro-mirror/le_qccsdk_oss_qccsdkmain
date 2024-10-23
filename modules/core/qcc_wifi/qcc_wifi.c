/*
 */
#include "autoconf.h"
#include "fwconfig_cmn.h"
#include "nt_flags.h"
#include "nt_osal.h"
#include "nt_common.h"
#include "nt_wfm_wmi_interface.h"
#include <stdint.h>

struct libwifi_qos_null_kconfig_t{
    uint8_t enable;
    uint8_t retry_count;
    uint16_t socmp_nop_delay;
};

struct libwifi_qos_null_kconfig_t g_libwifi_qos_null_kconfig_t;


void libwifi_kconfig_install(void)
{
#ifdef CONFIG_ACK_TIMEOUT_MODIFY_ENABLE
    g_libwifi_qos_null_kconfig_t.enable = TRUE;
#else
    g_libwifi_qos_null_kconfig_t.enable = FALSE;
#endif
    g_libwifi_qos_null_kconfig_t.retry_count = CONFIG_QOS_NULL_DATA_MAX_RETRY_COUNT;
    g_libwifi_qos_null_kconfig_t.socmp_nop_delay = CONFIG_QOS_NULL_DATA_RETRY_DELAY;
}

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
