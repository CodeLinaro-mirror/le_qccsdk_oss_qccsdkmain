/*
#Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
#SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#ifndef QAPI_HFC_H
#define QAPI_HFC_H

/*========================================================================
*
*========================================================================*/


/*------------------------------------------------------------------------
* Include Files
* ----------------------------------------------------------------------*/

/*------------------------------------------------------------------------
* Preprocessor Definitions and Constants
* ----------------------------------------------------------------------*/
/* None */

/*------------------------------------------------------------------------
* Type Declarations
* ----------------------------------------------------------------------*/

/*------------------------------------------------------------------------
* Function Declarations
* ----------------------------------------------------------------------*/

/**
 * @brief API to be used to get the max number of queued messages from Host.
 * @return The max number of queued messages.
 */
uint32_t qapi_hfc_get_max_msg_num(void);

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
qapi_Status_t qapi_hfc_sendto_host_data_pkt(void* p_buff, uint8_t *payload, uint16_t len, uint16_t info);

/**
 * @brief  API to be used to send config packets to host.
 * @param[in]  p_buf    Pointer to the buffer of config packet
 * @param[in]  len      Length of the config packet
 * @return
 * TRUE -- On success.\n
 * FALSE -- On failure.
 */
qbool_t qapi_hfc_sendto_host_config_pkt(uint32_t *p_buf, uint16_t len);

/**
 * @brief API to be used to set wlan state.
 * @return  
 * QAPI_OK -- On success.\n
 * QAPI_ERROR -- On failure.
 */
qapi_Status_t qapi_hfc_set_gpio_assert_info(f2a_event_type event);

#endif /* QAPI_HFC_H */

