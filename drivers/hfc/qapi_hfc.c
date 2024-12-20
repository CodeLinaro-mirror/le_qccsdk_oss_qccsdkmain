/*
#Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
#SPDX-License-Identifier: BSD-3-Clause-Clear
 */


/*------------------------------------------------------------------------
* Include Files
* ----------------------------------------------------------------------*/
#include <stdlib.h>
#include "unistd.h"
#include "qapi_status.h"
#include "data_svc_hfc.h"

#ifdef CONFIG_RING_IF_ONLY
/**
 * @brief API to be used to send to data packets to Host.
 *
 * @param[in] p_buff     pointer of buffer
 * @param[in] payload    pointer of payload
 * @param[in] len        length of payload
 * @param[in] info       extra info
 * @return  
 * QAPI_OK -- On success.\n
 * Error code -- On failure.
 */
qapi_Status_t qapi_hfc_sendto_host_data_pkt(void* p_buff, uint8_t *payload, uint16_t len, uint16_t info)
{
    int error_code = QAPI_OK;

	error_code = data_svc_hfc_send_data_pkt(p_buff, payload, len, info);
    switch (error_code)
    {
        case -1:
			error_code = QAPI_ERR_INVALID_PARAM;
			break;
        case -2:
			error_code = QAPI_ERR_NO_RESOURCE;
			break;
		default:
			error_code = QAPI_OK;
			break;
    }

	return error_code;
}

/**
 * @brief API to be used to receive data packets from Host.
 *
 * @param[in] msg        pointer of msg
 * @param[in] timeout    timeout in millisecond
 * @return  
 * QAPI_OK -- On success.\n
 * Error code -- On failure.
 */
qapi_Status_t qapi_hfc_recvfrom_host_msg(hfc_msg_t *msg, uint32_t timeout)
{
	return data_svc_hfc_recv_msg(msg, timeout);
}

/**
 * @brief API to be used to receive data packets from Host.
 *
 * @param[in] p_buff     pointer of buffer
 * @param[in] payload    pointer of buffer length
 * @param[in] timeout    timeout in millisecond
 * @param[out] data_len  pointer of data length
 * @param[out] info      pointer of extra info
 * @return  
 * QAPI_OK -- On success.\n
 * Error code -- On failure.
 */
qapi_Status_t qapi_hfc_recvfrom_host_data_pkt(void* p_buff, uint16_t *buf_len, uint32_t timeout, uint16_t *data_len, uint16_t *info)
{
	return data_svc_hfc_recv_data_pkt(p_buff, buf_len, timeout, data_len, info);
}

/**
 * @brief  API to be used to send config packets to host.
 * @param[in]  p_buf    Pointer to the buffer of config packet
 * @param[in]  len      Length of the config packet
 * @return
 * TRUE -- On success.\n
 * FALSE -- On failure.
 */
qbool_t qapi_hfc_sendto_host_config_pkt(uint32_t *p_buf, uint16_t len) 
{
    return data_svc_hfc_send_config(p_buf, len);
}


/**
 * @brief API to be used to set wlan state.
 * @return  
 * QAPI_OK -- On success.\n
 * QAPI_ERROR -- On failure.
 */
qapi_Status_t qapi_hfc_set_gpio_assert_info(f2a_event_type event)
{
    if(data_svc_set_gpio_assert_info(event) == 0)
    {
        return QAPI_OK;
    }
	
    return QAPI_ERROR;
}

#endif //SUPPORT_RING_IF

