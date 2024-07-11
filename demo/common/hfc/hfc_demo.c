/*========================================================================
* @brief Function definitions for hfc connections
*=======================================================================*/


/*------------------------------------------------------------------------
* Include Files
* ----------------------------------------------------------------------*/
#include <stdlib.h>
#include "unistd.h"
#include "qurt_internal.h"
#include "data_svc_hfc.h"
#include "qapi_hfc.h"
#include "hfc_demo.h"
#include "nt_logger_api.h"

#ifdef CONFIG_QCSPI_HFC_TEST

#define QCSPI_HFC_TEST_THREAD_STACKSIZE          1024
#define QCSPI_HFC_TEST_THREAD_PRIO               6
#define HFC_SEND_MAX_RETRY_COUNT                 10
#define HFC_TEST_PRINT_ENABLE 1
#ifdef HFC_TEST_PRINT_ENABLE
#define HFC_TEST_PRINT(...) printf( __VA_ARGS__) 
#else
#define HFC_TEST_PRINT(...)
#endif

hfc_data_test_stats hfc_data_test_result;
qurt_pipe_t qcspi_hfc_test_queue;

int qcspi_hfc_process_test_data(hfc_msg_t* msg)
{
	uint16_t i;

    if (NULL == msg)
    {
        return -1;
    }
	
	if ((NULL != msg->data) && (msg->len > 0))
	{
	    hfc_data_test_result.recv_bytes += msg->len;

		HFC_TEST_PRINT("recv msg_id %d data %x len %d:  ", msg->id, (uint32_t)msg->data, msg->len);
#ifdef HFC_TEST_PRINT_ENABLE
		uint8_t* p = msg->data;
#endif
		for (i=0; i<5; i++)
		{
			HFC_TEST_PRINT("%02x", *(p+i));
		}
		
		HFC_TEST_PRINT(" ... ");
		
		for (i=msg->len-5; i<msg->len; i++)
		{
			HFC_TEST_PRINT("%02x", *(p+i));
		}	
		HFC_TEST_PRINT("\r\n"); 
	}  

	return 0;
}

int qcspi_hfc_process_test_config(hfc_msg_t* msg)
{
	hfc_data_test_ctrl* test=NULL;

    if ((NULL == msg) || (NULL == msg->buf))
    {
        return -1;
    }

    switch (msg->id)
    {
        case HFC_TEST_DATA_START:
			test = (hfc_data_test_ctrl*)msg->data;
			memset(&hfc_data_test_result, 0, sizeof(hfc_data_test_stats));	
			HFC_TEST_PRINT("***test start mode %d\r\n", test->mode); 
			hfc_data_test_result.mode = test->mode;
			hfc_data_test_result.f2a_pkt_count = test->f2a_pkt_count;
			hfc_data_test_result.f2a_pkt_size = test->f2a_pkt_size;
			break;
		case HFC_TEST_DATA_END:
			HFC_TEST_PRINT("test end: mode %d send_len %d recv_len %d\r\n", hfc_data_test_result.mode, hfc_data_test_result.send_bytes, hfc_data_test_result.recv_bytes);
			qcspi_hfc_send_test_end_event();
			break;
		default:
			break;
    }
    
	nt_osal_free_memory(msg->buf);
	return 0;
}

int qcspi_hfc_send_test_end_event(void)
{
    uint16_t len = sizeof(hfc_data_test_stats);
    hfc_data_test_stats *test_result = (hfc_data_test_stats*)nt_osal_allocate_memory(len);
    if (NULL == test_result)
    {
        NT_LOG_WFM_ERR("Memory allocation failed", 0, 0, 0);
        ASSERT(0);
    }
    hfc_data_test_result.hdr.msg_id = HFC_TEST_DATA_END;
    memcpy(test_result, &hfc_data_test_result, sizeof(hfc_data_test_stats));
    qapi_hfc_sendto_host_config_pkt((void*)test_result, len);

	return 0;
}

int qcspi_hfc_send_test_data(uint16_t pkt_count, uint16_t pkt_size, uint16_t msg_id)
{
	uint8_t *buf = NULL;
	int retry_cnt = 0;
	qapi_Status_t ret = QAPI_OK;
    int i;
	
    HFC_TEST_PRINT("f2a send pkt_count %d pkt_size %d\r\n", pkt_count, pkt_size);

    for (i=0; i<pkt_count; i++)
    {		
		buf = nt_osal_allocate_memory(pkt_size);
		if (NULL == buf)
		{
			nt_osal_delay(10);
			continue;
		}
		
		memset(buf, i%256, pkt_size);
		while (retry_cnt < HFC_SEND_MAX_RETRY_COUNT)
		{
		    ret = qapi_hfc_sendto_host_data_pkt(buf, buf, pkt_size, msg_id);
    		if (QAPI_OK == ret)
    		{
				HFC_TEST_PRINT("qcspi_hfc_send_test_data send pkt num %d\r\n", i+1);
    		    hfc_data_test_result.send_bytes+= pkt_size;
				retry_cnt = 0;
			    break;
			} 

			if (QAPI_ERR_NO_RESOURCE == ret)
			{
				/* Temp way to wait for the free elment, which
				 * could be optimized to improve the speed. */
    		    nt_osal_delay(100); 
				retry_cnt++;
    		}
			else
			{
			    nt_osal_free_memory(buf);
				return -1;
			}
		}
    }

	return 0;
}

static void qcspi_hfc_test_thread(void *arg)
{
    (void)(arg);
    hfc_msg_t msg;  
    uint16_t num_msgs = 0;
    int i;
    
    while (1) 
    {
        /* wait for a message, timeouts are processed while waiting */	
      	num_msgs = uxQueueMessagesWaiting(qcspi_hfc_test_queue);
      	for(i=0; i<num_msgs; i++) 
      	{
      		if (nt_osal_queue_msg_receive(qcspi_hfc_test_queue, &msg, portMAX_DELAY) == NT_QUEUE_SUCCESS) 
      		{
      		    HFC_TEST_PRINT("RingIf: hfc msg type %d id %d (p_buf: %x len:%d data:%x) \r\n", msg.type, msg.id, (uint32_t)msg.buf, msg.len, (uint32_t)msg.data);
                if (HFC_DATA_MSG == msg.type)
                {
                    qcspi_hfc_process_test_data(&msg);
					if (HFC_DATA_TEST_LOOP_BACK == hfc_data_test_result.mode)
					{
                    	qapi_hfc_sendto_host_data_pkt(msg.buf, msg.data, msg.len, msg.id);
						hfc_data_test_result.send_bytes+= msg.len;
						HFC_TEST_PRINT("send data %x len %d.\r\n\r\n", (uint32_t)msg.data, msg.len);						
					} 
					else if(HFC_DATA_TEST_TX == hfc_data_test_result.mode)
					{
					    nt_osal_free_memory(msg.buf);
					}
                }
                else if (HFC_CTRL_MSG == msg.type)
                {
                    qcspi_hfc_process_test_config(&msg);
					if ((HFC_TEST_DATA_START == msg.id) && (HFC_DATA_TEST_RX == hfc_data_test_result.mode))
					{
					    qcspi_hfc_send_test_data(hfc_data_test_result.f2a_pkt_count, hfc_data_test_result.f2a_pkt_size, HFC_TEST_DEMO_DATA);
					}
                }
            }
        }
    } 
}

/**
 * Initialize this qcspi hfc module:
 * - start the qcspi_hfc_thread
 */
void Initialize_qcspi_hfc_Demo(void)
{
    uint32_t ret_val;
	uint32_t max_hfc_msg_num;
	nt_osal_task_handle_t  hfc_test_task_hdl;

    ret_val =  (uint32_t)nt_qurt_thread_create(qcspi_hfc_test_thread, "qcspi_hfc_test", QCSPI_HFC_TEST_THREAD_STACKSIZE, NULL, QCSPI_HFC_TEST_THREAD_PRIO, &hfc_test_task_hdl);
    if(ret_val != pdPASS)
    {
  	  HFC_TEST_PRINT("RingIfErr: task creation failed out of memory\r\n");
  	  ASSERT(0);
    } 

    max_hfc_msg_num = qapi_hfc_get_max_msg_num();
    qcspi_hfc_test_queue = nt_qurt_pipe_create(max_hfc_msg_num, sizeof(hfc_msg_t));
    if (qcspi_hfc_test_queue == NULL)
    {
  	  HFC_TEST_PRINT("failed to create qcspi_hfc_test_queue", 0);
  	  nt_osal_thread_delete(hfc_test_task_hdl);
  	  ASSERT(0);
    }  
}
#endif

