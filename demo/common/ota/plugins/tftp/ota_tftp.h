/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _OTA_TFTP_H_
#define _OTA_TFTP_H_

/* Preprocessor Definitions and Constants											                      */
/**********************************************************************************************************/
/*
 * Firmware Upgrade TFTP Status codes
 */
#define QAPI_FW_UPGRADE_ERR_TFTP_SESSION_ALREADY_START __QAPI_ERROR(QAPI_MOD_FWUP, 100)
#define QAPI_FW_UPGRADE_ERR_TFTP_SESSION_NOT_START     __QAPI_ERROR(QAPI_MOD_FWUP, 101)
#define QAPI_FW_UPGRADE_ERR_TFTP_URL_FORMAT            __QAPI_ERROR(QAPI_MOD_FWUP, 102)
#define QAPI_FW_UPGRADE_ERR_TFTP_NO_MEMORY             __QAPI_ERROR(QAPI_MOD_FWUP, 103)
#define QAPI_FW_UPGRADE_ERR_TFTP_CREATE_SOCKET         __QAPI_ERROR(QAPI_MOD_FWUP, 104)
#define QAPI_FW_UPGRADE_ERR_TFTP_BIND_FAIL             __QAPI_ERROR(QAPI_MOD_FWUP, 105)
#define QAPI_FW_UPGRADE_ERR_TFTP_SOCKET_CONNECT        __QAPI_ERROR(QAPI_MOD_FWUP, 106)
#define QAPI_FW_UPGRADE_ERR_TFTP_THREAD_FAIL           __QAPI_ERROR(QAPI_MOD_FWUP, 107)
#define QAPI_FW_UPGRADE_ERR_TFTP_CONNECT_FAIL          __QAPI_ERROR(QAPI_MOD_FWUP, 108)
#define QAPI_FW_UPGRADE_ERR_TFTP_IMAGE_DOWNLOAD_FAIL   __QAPI_ERROR(QAPI_MOD_FWUP, 109)
#define QAPI_FW_UPGRADE_ERR_TFTP_IMAGE_NOT_FOUND       __QAPI_ERROR(QAPI_MOD_FWUP, 110)
#define QAPI_FW_UPGRADE_ERR_TFTP_SERVER_RESP_TIMEOUT   __QAPI_ERROR(QAPI_MOD_FWUP, 111)

/**********************************************************************************************************/
/* Function Declarations                											                      */
/**********************************************************************************************************/
qapi_Status_t plugin_tftp_init(const char *interface_name, const char *url, void *init_param);
qapi_Status_t plugin_tftp_fin(void);
qapi_Status_t plugin_tftp_recv_data(uint8_t *buffer, uint32_t buf_len, uint32_t *ret_size, void *init_param);
qapi_Status_t plugin_tftp_abort(void);
qapi_Status_t plugin_tftp_resume(const char *interface_name, const char *url, uint32_t offset);

#endif /*_OTA_TFTP_H_ */
