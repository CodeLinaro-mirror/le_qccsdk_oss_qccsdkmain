/*========================================================================
* @brief Function definitions for hfc connections
*=======================================================================*/


/*------------------------------------------------------------------------
* Include Files
* ----------------------------------------------------------------------*/
#include <stdlib.h>
#include "unistd.h"
#include "qapi_status.h"
#include "data_svc_hfc.h"

#ifdef CONFIG_RING_IF_ONLY
/*
*API to be used to send to event/response message.
*
@param p_buff   :   pointer of buffer
@param payload  :   pointer of payload
@param len      :   length of payload
@param info     :   extra info
*/
qapi_Status_t qapi_hfc_sendto_host_data_pkt(void* p_buff, uint8_t *payload, uint16_t len, uint16_t info)
{
    int error_code = QAPI_OK;

	error_code = data_svc_hfc_recv_data_pkt(p_buff, payload, len, info);
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

/*
* @brief  API to be used to send config/event/response packet
* @param  p_buf          : Pointer to the config packet that needs to be sent
* @param  len            : Length of the config packet
* @return bool           : TRUE if attaching to ring is successful
*
*/
qbool_t qapi_hfc_sendto_host_config_pkt(uint32_t *p_buf, uint16_t len) 
{
    return data_svc_hfc_send_config(p_buf, len);
}

/*
*get the max message number from Host.
*@param p_element :   void
*@return          :   max message number
*
*/
uint32_t qapi_hfc_get_max_msg_num(void)
{
    return data_svc_hfc_get_max_msg_num();
}

#endif //SUPPORT_RING_IF

