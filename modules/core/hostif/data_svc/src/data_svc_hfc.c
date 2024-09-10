/*========================================================================
*
* @brief Function definitions for hfc connections
*=======================================================================*/


/*------------------------------------------------------------------------
* Include Files
* ----------------------------------------------------------------------*/
#include <stdlib.h>
#include "unistd.h"
#include "data_svc_priv.h"
#include "data_svc_hfc_priv.h"
#include "data_svc_hfc.h"
#include "nt_logger_api.h"

#if defined(SUPPORT_RING_IF) || defined(SUPPORT_RING_IF_ONLY)
#define QCSPI_HFC_THREAD_STACKSIZE          1024
#define QCSPI_HFC_THREAD_PRIO               6
#define HFC_HEADER_SIZE                     sizeof(hfc_msg_t)

static qurt_pipe_t qcspi_hfc_data_queue;

#ifdef CONFIG_QCSPI_HFC_TEST    
extern qurt_pipe_t qcspi_hfc_test_queue;
#endif

/*
*Get buffer used for hfc receive
*@param p_element :   pointer to ring element
*@param len       :   length of payload
*@return          :   TRUE if buffer allocated
*                 :   FALSE if buffer not allocated
*/
bool data_svc_get_hfc_data_buff(void* p_element, uint16_t len)
{

    configASSERT(NULL != p_element);

    ring_element_t* p_elem=(ring_element_t *)p_element;

    void* p_buf = nt_osal_allocate_memory(len + HFC_HEADER_SIZE);
	RINGIF_PRINT_LOG_INFO("allocate hfc buf %x len %d\r\n", (uint32_t)p_buf, len);

    if(p_buf != NULL)
    {
        p_elem->p_buf_start=p_buf;
        p_elem->len=len;
        p_elem->p_buf=(uint32_t*)((uint8_t*)p_buf + HFC_HEADER_SIZE);

        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/*
*Free buffer used for hfc receive
*@param p_element :   pointer to ring element
*@return          :   TRUE if buffer freed
*                 :   FALSE if buffer not freed
*/
bool data_svc_free_hfc_data_buff (void* p_element)
{

    configASSERT(NULL != p_element);

    void* p_buf=NULL;
    ring_element_t* p_elem = (ring_element_t *)p_element;

	RINGIF_PRINT_LOG_INFO("free hfc buf %x\r\n", (uint32_t)p_elem->p_buf_start);

    p_buf=p_elem->p_buf_start;
    nt_osal_free_memory(p_buf);

    p_elem->p_buf = NULL;
    p_elem->p_buf_start = NULL;

    return TRUE;
}

/*
*Callback triggered when some data is receievd on Fermion
* and need to send to Host.
*
@param p_buff   :   pointer of buffer
@param payload  :   pointer of payload
@param len      :   length of payload
@param info     :   extra info
*/
int32_t data_svc_hfc_recv_data_pkt(void* p_buff, uint8_t *payload, uint16_t len, uint16_t info)
{
    bool b_result = FALSE;

	RINGIF_PRINT_LOG_INFO("data_svc_hfc_recv_data_pkt p_buff %x len %d\r\n", (uint32_t)p_buff, len);
    if(NULL == p_buff || NULL == payload || 0 == len)
    {
        return -1;
    }

    b_result=ringif_f2a_pkt_attach(F2A_RING_ID_DATA, (uint32_t*)p_buff,(uint32_t*)payload, len, info);
    if(FALSE == b_result) {
        RINGIF_PRINT_LOG_INFO("dropping hfc f2a pkt as ring full");
        return -2;
    } 
	
    RINGIF_PRINT_LOG_INFO("Rx Pkt Sent to ring(%d) len:%d", F2A_RING_ID_DATA, len);
    return 0;
}

int32_t data_svc_hfc_queue_send(ring_element_t *p_elem, hfc_msg_type_t type)
{
    hfc_msg_t msg;
    hfc_msg_hdr* header = NULL;
  
    if (NULL == p_elem->p_buf) 
    {
        return -1;
    }
	
    msg.type = type;
    if (HFC_DATA_MSG == type)
    {
        msg.id = p_elem->info;
    }
    else if (HFC_CTRL_MSG == type)
    {    
        header = (hfc_msg_hdr*)p_elem->p_buf;
        msg.id = header->msg_id;      
    }
    msg.data= (uint8_t*)p_elem->p_buf;
    msg.len= p_elem->len;
    msg.buf = p_elem->p_buf_start;
#ifdef CONFIG_QCSPI_HFC_TEST    
    if(NT_QUEUE_FAIL == nt_osal_queue_send(qcspi_hfc_test_queue, (void*)&msg, portMAX_DELAY))
    {
        RINGIF_PRINT_LOG_ERR("hfc queue send fail", 0);
    }
#else
    (void)msg;
#endif
    return 0;
}

qbool_t data_svc_hfc_send_config(uint32_t *p_buf, uint16_t len)
{
    return ringif_send_hfc_config(p_buf, len);
}

uint32_t data_svc_hfc_get_max_msg_num(void)
{
    return MAX_NUM_A2F_CTRL_RING_ELEMS + MAX_NUM_A2F_DATA_RING_ELEMS;
}

/*
*process the hfc packet from Host
*@param p_element :   pointer to ring element
*@return          :   TRUE on sucess
*                 :   FALSE on else
*/
bool process_hfc_data_pkt(void* p_element)
{
    RINGIF_PRINT_LOG_INFO("hfc: Processing hfc packet\r\n");
	ring_element_t* p_elem = (ring_element_t*)p_element;

	if (NULL == p_elem || NULL == p_elem->p_buf)
	{
	    return FALSE;
	}
	
#ifndef CONFIG_QCSPI_HFC_TEST
	uint16_t i;
	uint8_t* p =(uint8_t*)(p_elem->p_buf);

	printf("data %x len %d:", (uint32_t)p_elem->p_buf_start, p_elem->len);
	
	for (i=0; i<p_elem->len; i++)
	{
		printf("%02x", *(p+i));
	}
	printf("\r\n");
	nt_osal_free_memory(p_elem->p_buf_start);
#else
    RINGIF_PRINT_LOG_INFO("recv test demo data buf %x len %d:  \r\n", (uint32_t)p_elem->p_buf_start, p_elem->len);			
    data_svc_hfc_queue_send(p_elem, HFC_DATA_MSG);
#endif

    return TRUE;
}

bool process_hfc_config_pkt(void* p_element)
{    
	ring_element_t* p_elem = (ring_element_t*)p_element;

	if (NULL == p_elem)
	{
	    return FALSE;
	}
#ifndef CONFIG_QCSPI_HFC_TEST
    hfc_msg_hdr* header = (hfc_msg_hdr*)p_elem->p_buf;
    printf("hfc config msg_id %d\r\n", header->msg_id);
	nt_osal_free_memory(p_elem->p_buf_start);
#else	
	data_svc_hfc_queue_send(p_elem, HFC_CTRL_MSG);
#endif
	return TRUE;
}

err_t hfc_data_try_callback(hfc_callback_fn fun,void *ctx)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    hfc_cb_t msg;
  
    msg.fun = fun;
    msg.ctx = ctx;
  
    if(NT_QUEUE_FAIL == nt_osal_queue_send_from_isr(qcspi_hfc_data_queue, (void*)&msg, &xHigherPriorityTaskWoken))
    {
        RINGIF_PRINT_LOG_ERR("hfc queue send fail", 0);
    }
    
    return ERR_OK;
}

/**
 * The main qcspi hfc thread. 
 *
 * @param arg : unused argument
 */
static void qcspi_hfc_thread(void *arg)
{
    (void)(arg);
    hfc_cb_t msg;
  
    while (1) 
    {
		if (nt_osal_queue_msg_receive(qcspi_hfc_data_queue, &msg, portMAX_DELAY) == NT_QUEUE_SUCCESS) 
		{
		    if (NULL != msg.fun)
	    	{
	            msg.fun(msg.ctx);
	    	}  
		}
    }
}

/**
 * Initialize this qcspi hfc module:
 * - start the qcspi_hfc_thread
 */
void qcspi_hfc_init(void)
{
    uint32_t ret_val;
    nt_osal_task_handle_t  hfc_task_hdl;
    RINGIF_PRINT_LOG_INFO("qcspi_hfc_init");

    ret_val =  (uint32_t)nt_qurt_thread_create(qcspi_hfc_thread, "qcspi_hfc_thread", QCSPI_HFC_THREAD_STACKSIZE, NULL, QCSPI_HFC_THREAD_PRIO, &hfc_task_hdl);
    if(ret_val != pdPASS)
    {
      RINGIF_PRINT_LOG_ERR("RingIfErr: task creation failed out of memory\r\n");
      A_ASSERT(0);
    } 
    
    qcspi_hfc_data_queue = nt_qurt_pipe_create(TOTAL_NUM_DATA_RING_ELEMS, sizeof(hfc_cb_t));
    if (qcspi_hfc_data_queue == NULL)
    {
      RINGIF_PRINT_LOG_ERR("failed to create qcspi_hfc_data_queue", 0);
      nt_osal_thread_delete(hfc_task_hdl);
      A_ASSERT(0);
    } 
}
#endif //SUPPORT_RING_IF

