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

#define SAT_STEP_NUM_BANDS 2              // 2G, 5G
#define SAT_STEP_NUM_CHIP_TYPES 3         // FF, TT, SS

typedef enum {
    BAND_5G = 0,
    BAND_2G = 1,
} sat_step_band_t;

typedef enum {
    CHIP_TYPE_FF = 0,  // Fast-Fast corner
    CHIP_TYPE_TT = 1,  // Typical-Typical corner
    CHIP_TYPE_SS = 2,  // Slow-Slow corner
} sat_step_chip_type_t;

struct libwifi_kconfig_t {
    uint32_t hc_11a_0_2g;
    uint32_t hc_11a_1_2g;
    uint32_t hc_11n_2g;
    uint32_t hc_11a_0_5g;
    uint32_t hc_11a_1_5g;
    uint32_t hc_11n_5g;
    uint8_t srrc_band_edge_enable;
    uint8_t xo_use_bdf_override;
    uint8_t sat_step_manual_adjustment;
    uint32_t sat_step_values[SAT_STEP_NUM_BANDS][SAT_STEP_NUM_CHIP_TYPES];
};

struct libwifi_qos_null_kconfig_t {
    uint8_t enable;
    uint8_t retry_count;
    uint16_t socmp_nop_delay;
};

struct libwifi_kconfig_t g_libwifi_kconfig;
struct libwifi_qos_null_kconfig_t g_libwifi_qos_null_kconfig_t;
uint32_t total_beacon_wait_time;

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

#ifdef CONFIG_XO_USE_BDF_OVERRIDE
    g_libwifi_kconfig.xo_use_bdf_override = TRUE;
#else
    g_libwifi_kconfig.xo_use_bdf_override = FALSE;
#endif

#ifdef CONFIG_SAT_STEP_MANUAL_ADJUSTMENT
    g_libwifi_kconfig.sat_step_manual_adjustment = TRUE;
#else
    g_libwifi_kconfig.sat_step_manual_adjustment = FALSE;
#endif

    /* 2G band defaults */
#ifdef CONFIG_SAT_STEP_2G_FF
    g_libwifi_kconfig.sat_step_values[BAND_2G][CHIP_TYPE_FF] = CONFIG_SAT_STEP_2G_FF;
#else
    g_libwifi_kconfig.sat_step_values[BAND_2G][CHIP_TYPE_FF] = 0x00000062;
#endif

#ifdef CONFIG_SAT_STEP_2G_TT
    g_libwifi_kconfig.sat_step_values[BAND_2G][CHIP_TYPE_TT] = CONFIG_SAT_STEP_2G_TT;
#else
    g_libwifi_kconfig.sat_step_values[BAND_2G][CHIP_TYPE_TT] = 0x00000060;
#endif

#ifdef CONFIG_SAT_STEP_2G_SS
    g_libwifi_kconfig.sat_step_values[BAND_2G][CHIP_TYPE_SS] = CONFIG_SAT_STEP_2G_SS;
#else
    g_libwifi_kconfig.sat_step_values[BAND_2G][CHIP_TYPE_SS] = 0x00000060;
#endif

    /* 5G band defaults */
#ifdef CONFIG_SAT_STEP_5G_FF
    g_libwifi_kconfig.sat_step_values[BAND_5G][CHIP_TYPE_FF] = CONFIG_SAT_STEP_5G_FF;
#else
    g_libwifi_kconfig.sat_step_values[BAND_5G][CHIP_TYPE_FF] = 0x00000064;
#endif

#ifdef CONFIG_SAT_STEP_5G_TT
    g_libwifi_kconfig.sat_step_values[BAND_5G][CHIP_TYPE_TT] = CONFIG_SAT_STEP_5G_TT;
#else
    g_libwifi_kconfig.sat_step_values[BAND_5G][CHIP_TYPE_TT] = 0x00000064;
#endif

#ifdef CONFIG_SAT_STEP_5G_SS
    g_libwifi_kconfig.sat_step_values[BAND_5G][CHIP_TYPE_SS] = CONFIG_SAT_STEP_5G_SS;
#else
    g_libwifi_kconfig.sat_step_values[BAND_5G][CHIP_TYPE_SS] = 0x00000064;
#endif

#ifdef CONFIG_TOTAL_BEACON_WAIT_TIME
  total_beacon_wait_time = CONFIG_TOTAL_BEACON_WAIT_TIME;
#else
  total_beacon_wait_time = 25000;
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
