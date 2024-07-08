#ifndef DATA_SVC_HFC_H
#define DATA_SVC_HFC_H

/*========================================================================
*========================================================================*/


/*------------------------------------------------------------------------
* Include Files
* ----------------------------------------------------------------------*/
#include <stdint.h>
#include "qapi_status.h"
/*------------------------------------------------------------------------
* Preprocessor Definitions and Constants
* ----------------------------------------------------------------------*/
/* None */

/*------------------------------------------------------------------------
* Type Declarations
* ----------------------------------------------------------------------*/
typedef enum {
    HFC_CTRL_MSG,
    HFC_DATA_MSG
}hfc_msg_type_t;

typedef struct {
    uint16_t msg_id;
    uint8_t reserved[2];
}hfc_msg_hdr;

typedef struct hfc_msg
{
    hfc_msg_type_t type;
	uint32_t id;
	void *buf;
	uint8_t  *data;
	uint16_t len;
	uint8_t reserved[2];
}hfc_msg_t;

/*------------------------------------------------------------------------
* Function Declarations
* ----------------------------------------------------------------------*/
extern int32_t data_svc_hfc_recv_data_pkt(void* p_buff, uint8_t *payload, uint16_t len, uint16_t info);
extern qbool_t data_svc_hfc_send_config(uint32_t *p_buf, uint16_t len);
extern uint32_t data_svc_hfc_get_max_msg_num(void);
#endif /* DATA_SVC_HFC_H */

