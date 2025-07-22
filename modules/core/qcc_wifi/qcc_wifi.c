/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include "autoconf.h"
#include "fwconfig_cmn.h"
#include "nt_flags.h"
#include "nt_osal.h"
#include "nt_common.h"
#include "nt_wfm_wmi_interface.h"
#include <stdint.h>

struct libwifi_kconfig_t {
    uint32_t hc_11a_0_2g;
    uint32_t hc_11a_1_2g;
    uint32_t hc_11n_2g;
    uint32_t hc_11a_0_5g;
    uint32_t hc_11a_1_5g;
    uint32_t hc_11n_5g;
    uint8_t srrc_band_edge_enable;
};

struct libwifi_qos_null_kconfig_t {
    uint8_t enable;
    uint8_t retry_count;
    uint16_t socmp_nop_delay;
};

struct libwifi_kconfig_t g_libwifi_kconfig;
struct libwifi_qos_null_kconfig_t g_libwifi_qos_null_kconfig_t;

void libwifi_kconfig_install(void)
{
#ifdef CONFIG_HEAVY_CLIP_LEVEL_2G
    g_libwifi_kconfig.hc_11a_0_2g = CONFIG_PEAKW_11A_0_2G;
    g_libwifi_kconfig.hc_11a_1_2g = CONFIG_PEAKW_11A_1_2G;
    g_libwifi_kconfig.hc_11n_2g = CONFIG_PEAKW_11N_0_2G;
#else
    g_libwifi_kconfig.hc_11a_0_2g = 0x564E4739;
    g_libwifi_kconfig.hc_11a_1_2g = 0x5D;
    g_libwifi_kconfig.hc_11n_2g = 0x5D47473C;
#endif

#ifdef CONFIG_HEAVY_CLIP_LEVEL_5G
    g_libwifi_kconfig.hc_11a_0_5g = CONFIG_PEAKW_11A_0_5G;
    g_libwifi_kconfig.hc_11a_1_5g = CONFIG_PEAKW_11A_1_5G;
    g_libwifi_kconfig.hc_11n_5g = CONFIG_PEAKW_11N_0_5G;
#else
    g_libwifi_kconfig.hc_11a_0_5g = 0x60523F3F;
    g_libwifi_kconfig.hc_11a_1_5g = 0x62;
    g_libwifi_kconfig.hc_11n_5g = 0x62514D3F;
#endif

#ifdef CONFIG_ACK_TIMEOUT_MODIFY_ENABLE
    g_libwifi_qos_null_kconfig_t.enable = TRUE;
#else
    g_libwifi_qos_null_kconfig_t.enable = FALSE;
#endif
    g_libwifi_qos_null_kconfig_t.retry_count = CONFIG_QOS_NULL_DATA_MAX_RETRY_COUNT;
    g_libwifi_qos_null_kconfig_t.socmp_nop_delay = CONFIG_QOS_NULL_DATA_RETRY_DELAY;

#ifdef CONFIG_SRRC_BAND_EDGE_SUPPORT
    g_libwifi_kconfig.srrc_band_edge_enable = TRUE;
#else
    g_libwifi_kconfig.srrc_band_edge_enable = FALSE;
#endif
}

NT_BOOL wmi_pdev_utf_cmd(wmi_msg_struct_t *msg)
{
#ifdef CONFIG_FTM_MODE
    extern uint8_t ftm_parse_tlv_cmd(uint8_t * buf, uint32_t dataLength);
    ftm_parse_tlv_cmd((uint8_t *)msg->msg_struct.vo_data, msg->msg_struct.vo_data_len);
#else  /* CONFIG_FTM_MODE */
    (void)msg;
#endif /* CONFIG_FTM_MODE */
    return TRUE;
}
void wmi_unit_test_cmd_handler(WMI_UNIT_TEST_CMD *cmd)
{
#ifdef UNIT_TEST_SUPPORT
    extern void wmi_unit_test_internal_cmd_handler(WMI_UNIT_TEST_CMD * cmd);
    wmi_unit_test_internal_cmd_handler(cmd);
#else  /* UNIT_TEST_SUPPORT */
    (void)cmd;
#endif /* UNIT_TEST_SUPPORT */
    return;
}
