/* 
Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#ifndef __WMI_API_H__
#define __WMI_API_H__

#include "wlan_power.h"

#define WLAN_WMI_CMD_SIG_MASK_ENABLE_DONE 0x01
#define WLAN_WMI_CMD_SIG_MASK_DISABLE_DONE 0x02
#define WLAN_WMI_CMD_SIG_MASK_STARTED_SCAN 0x04
#define WLAN_WMI_CMD_SIG_MASK_SCAN_COMPLETED 0x08
#define WLAN_WMI_CMD_SIG_MASK_CONNECT_COMPLETED 0x10
#define WLAN_WMI_CMD_SIG_MASK_DISCONNECTED 0x20
#define WLAN_WMI_CMD_SIG_MASK_IF_ADDED 0x40
#define WLAN_WMI_CMD_SIG_MASK_SET_MODE 0x80
#define WLAN_WMI_CMD_SIG_MASK_GET_STAT 0x100
#define WLAN_WMI_CMD_SIG_MASK_SET_PARAM 0x200
#define WLAN_WMI_CMD_SIG_MASK_GET_REG 0x400
#define WLAN_WMI_CMD_SIG_MASK_SET_RATE 0x800
#define WLAN_WMI_CMD_SIG_MASK_GET_RATE 0x1000
#define WLAN_WMI_CMD_SIG_MASK_SEND_RAW 0x2000
#define WLAN_WMI_CMD_SIG_MASK_SET_MGMT_FILTER 0x4000
#define WLAN_WMI_CMD_SIG_MASK_GET_TX_POWER 0x8000
#ifdef CONFIG_WPS
#define WLAN_WMI_CMD_SIG_MASK_STARTED_WPS_PROCESS 0x10000
#define WLAN_WMI_CMD_SIG_MASK_STOPPED_SCAN 0x20000
#endif

#ifdef CONFIG_ENABLE_P2P_MODE
#define WLAN_WMI_CMD_SIG_MASK_GET_NODELIST 0x40000
#define WLAN_WMI_CMD_SIG_MASK_GET_NETWORK 0x80000
#define WLAN_WMI_CMD_SIG_MASK_GO_NEG_RESULT 0x100000
#define WLAN_WMI_CMD_SIG_MASK_REQ_TO_AUTH 0X200000
#define WLAN_WMI_CMD_SIG_MASK_PROV_DISC_REQ 0x400000
#endif
#define WLAN_WMI_CMD_SIG_MASK_GET_BMPS_STATS 0x800000

extern qapi_Status_t wmi_cmd_send(WMI_COMMAND_ID cmd_id, void *p_data, uint32_t data_len);
extern qapi_Status_t wmi_dev_cmd_send(WMI_COMMAND_ID cmd_id, uint8_t dev_id, void *p_data, uint32_t data_len);

extern void wmi_event_relay(uint32_t if_id, event_t event_id, void *data, uint32_t data_length,
                            void __attribute__((__unused__)) * cxt);

extern qapi_Status_t wmi_on(void);
extern qapi_Status_t wmi_off(void);
extern qapi_Status_t wmi_add_device(uint8_t __attribute__((__unused__)) device_ID);
extern qapi_Status_t wmi_start_scan(uint8_t __attribute__((__unused__)) device_ID,
                                    const qapi_WLAN_Start_Scan_Params_t *scan_Params);
extern qapi_Status_t wlan_get_scan_results(uint8_t __attribute__((__unused__)) device_ID,
                                           qapi_WLAN_Scan_Comp_Evt_t *scan_Res, int16_t *num_Bss);
extern qapi_Status_t wmi_set_passphrase(void);
extern qapi_Status_t wmi_connect(void);
extern qapi_Status_t wmi_disconnect(void);
extern qapi_Status_t wmi_set_op_mode(void);
extern qapi_Status_t wmi_wlan_get_statistics(uint8_t device_ID);
extern qapi_Status_t wmi_wlan_get_regulatory(void);
extern qapi_Status_t wmi_set_rate(void);
extern qapi_Status_t wmi_get_rate(void);
extern qapi_Status_t wmi_send_raw(void);
extern qapi_Status_t wmi_set_mgmt_filter(void);
extern qapi_Status_t wmi_get_tx_power(void);
#ifdef CONFIG_WPS
extern qapi_Status_t wmi_stop_scan(void);
#endif
#endif //__WMI_API_H__
