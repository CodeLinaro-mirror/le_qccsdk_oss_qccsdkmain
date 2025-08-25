/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*
 * nt_hosted_app.h
 *
 *  Created on: Apr 23, 2021
 *      Author: sujit
 */

#ifndef APP_INC_NT_HOSTED_APP_H_
#define APP_INC_NT_HOSTED_APP_H_

#include "stdint.h"
#include "fwconfig_cmn.h"
#include "nt_flags.h"
#ifdef NT_HOSTED_SDK

typedef struct at_Cmd_s {
    int16_t atCmdIndex;
    int16_t at_CmdLen;
    const char *atCmdName;
    const char *help;
} at_Cmd_t;

typedef enum {
    AT,
    AT_IPCONFIG,
    AT_SYSON,
    AT_CWMODE,
    AT_CWJAP,
    AT_CWCAP,
    AT_CWSTART,
    AT_CWQAP,
    AT_CWQOFF,
    AT_CWRST,
    AT_CJSTARTDEV,
    AT_CIPSTART,
    AT_CIPSERVER,
    AT_CIPSEND,
    AT_CIPCLOSE,
    AT_CIPPING,
    AT_CIFSR,
    AT_CIPAP,
    AT_CIPSTA,
    AT_CIDHCP,
    AT_CWDHCPS,
    AT_CIPDNS,
    AT_CWENBLE,
    //#if defined (NT_TST_LWIP_STATS)
    //  AT_CIPLSTAT,
    //#endif
    AT_CIPDATA,
    AT_CWSETWEPK,
    AT_CWSETWEPDEFK,
    AT_CWIS_CONN,
    AT_CWPROD_STATS,
    AT_CWSOCPM_ENABLE,
    AT_CWSLEEP_TYPE,
    AT_CITCPCLIENT,
    AT_CITCPSEND,
    AT_CWSET_RMF,
    AT_CWTARGET_RESET,
    AT_CWSEND_BC_FRM,
    AT_CWSEND_MGMT_FRM,
    AT_CWENABLE_WMM,
    AT_CWENABLE_WFQ,
    AT_CWADD_BA,
    AT_CWDEL_BA,
    AT_CJWPS_APP,
    AT_CWFTM_SEND,
    AT_CWSET_CONFIG,
    AT_CWSET_RATE,
    AT_CIUDP_SERVER_WMM,
    AT_CIWMM_CLIENT,
    AT_CIWMM_SEND,
    AT_CWWPS_PUSH,
    AT_CWUAPSD_TRIGGER,
    AT_CWSET_IDLE_TIME,
    AT_CWTWT_ENABLE,
    AT_CWTWT_ACTION,
    AT_CWSET_VENDOR_ID,
    AT_CWWUR_INFO,
    AT_CWWNM,
    AT_CWSET_RA_CFG,
    AT_CWGET_MACID,
    AT_CJSTART_FG_SCAN,
    AT_CWUAPSD_STA,
    AT_CWGET_RA_CFG,
    AT_CWGET_BUILD_VER,
    AT_CWSEND_AC_FRAME,
    AT_MAX, /** Add new enum above this */
} at_Cmd_index;

#define NT_STATE_IDLE           0x8A /** AT Command process state IDLE */
#define NT_STATE_PARSE          0x8B /** AT Command parsing the SPI data */
#define NT_STATE_PROCESS        0x8C /** AT Command Running */
#define NT_AT_CMD_NOT_SUPPORTED 0x8D /** AT Command not supported as feature is disabled from nt_flags*/
#define NT_MAX_PKT_SIZE         2048 /** AT Command max packet size */

typedef enum {
    NT_AT_CMD_RCEV_SUCCESS = 0x84, /** AT Command  received success */
    NT_AT_CMD_IN_PROGRESS,         /** AT Command receive in progress */
    NT_AT_CMD_RECV_FAIL,           /** AT Command  received fail */
    NT_AT_FRAME_ERR,               /** Start/end marker lost */
    NT_AT_DATA_ERR,                /** Data length error */
    NT_AT_CMD_NOT_FOUND,           /** Command not found */
    NT_AT_UDP_SUCESS,              /** UDP PACK send successfully */
    NT_AT_UDP_ERR,                 /** UDP ERR */
    NT_AT_TCP_SEND_SUCESS,         /** TCP PACK send successfully */
    NT_AT_TCP_SEND_ERR,            /** TCP pack send ERR */
    NT_AT_TCP_CNX_SUCESS,          /** TCP Connection successful */
    NT_AT_TCP_CNX_FAIL,            /** TCP Connection fail */
    NT_AT_TCP_MSG_RCVD_COM,
    NT_AT_TCP_MSG_RCVD, /** TCP Mesaged recvd by the server */
    NT_AT_UDP_MSG_RCVD, /** UDP Mesaged recvd by the server  */
    NT_AT_SERVER_CLOSED,
    NT_AT_CONN_FAIL,  //** AT Command WiFi Connection Failed */
    NT_AT_SCAN_FAIL,  /** Arduino Command WiFi Scan Failed  */
} at_error;

uint8_t nt_hosted_set_config(char *pt_cmd);
uint8_t nt_hosted_set_rmf(char *pt_cmd);
uint8_t GetProtectionTypeCommand();
uint8_t nt_hosted_prot_test_sta(char *pt_cmd);
uint16_t nt_at_spi_data_of_len_with_premtion(uint32_t *pstr, uint16_t len);
void nt_at_spi_send(char *pstr);
uint16_t nt_at_spi_send_of_len(char *pstr, uint16_t len);
void nt_at_send_cmd_resp(char *cmd, char *resp);
char *nt_hosted_get_param(const char *CommandStr, uint8_t Param, uint8_t *ParamStrLength);
uint8_t nt_app_tcpclient(char *pt_cmd);
void nt_at_tcpconnection_cb(void *arg);
void tcp_client_sent_cb(void *arg);
// uint8_t set_hosted_config_location_mgmt_params_check(char *pt_cmd, usr_ftm *cli_ftm_config);
#if defined(NT_HOSTED_SDK) && defined(NT_DEBUG)
void nt_at_dbg_print(const char *s, ...);
#endif
/** Send RAW data over SPI without any excess character */
//#define NT_AT_SPI_RAW

/** Send RAW data over UART without any excess character */
//#define NT_AT_SEND_ON_UART
#endif /* NT_HOSTED_SDK */

/** Send RAW data over UART without any excess character */
#if !defined(NT_HOSTED_SDK) && !defined(NT_AT_SEND_ON_UART)
#define NT_AT_SEND_ON_UART
#endif

#if defined(NT_AT_SPI_RAW) && !defined(NT_AT_SEND_ON_UART)
#define NT_SEND(msg) nt_spi_send(msg);
#elif defined(NT_AT_SEND_ON_UART) && !defined(NT_AT_SPI_RAW)
#define NT_SEND(msg) nt_dbg_print(msg);
#else
#define NT_SEND(msg) nt_at_spi_send(msg);
uint8_t nt_at_ok_error(uint8_t err, uint8_t type, char *prn);
#endif

#ifndef MAC2STR
#define MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#define MACSTR     "%02x:%02x:%02x:%02x:%02x:%02x"
#endif

#endif /* APP_INC_NT_HOSTED_APP_H_ */
