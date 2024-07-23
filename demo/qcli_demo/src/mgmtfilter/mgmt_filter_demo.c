/*
#Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
#SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*------------------------------------------------------------------------
* Include Files
* ----------------------------------------------------------------------*/
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "qurt_internal.h"
#include "nt_osal.h"
#include "qapi_wlan.h"
#include "mgmt_filter_demo.h"


uint32_t mgmt_frame_recv_enabled = 0;
uint32_t mgmt_frame_recv_filter = 0;
qurt_signal_t  mgmt_filter_start;

mgmt_frame_t  mgmt_frame_recv_buf[MGMT_FRAME_MAX_NUM] = {0};

extern uint8_t get_active_device();

static void clear_mgmt_frames(void)
{
    uint32_t i;
	
	for (i=0; i<MGMT_FRAME_MAX_NUM; i++)
	{
	    mgmt_frame_recv_buf[i].len = 0;
	}
}

static void mgmt_frame_recv_thread(void *arg)
{
    (void)(arg);
	uint8_t deviceId = get_active_device();
    uint32_t i, j;
	uint8_t *buffer = NULL;
	uint32_t  buffer_len = MGMT_FRAME_MAX_SIZE;
    uint32_t frame_len = 0;
	uint8_t enabled_flag = 0;
	
    buffer = malloc(buffer_len*MGMT_FRAME_MAX_NUM);
	if (NULL == buffer)
	{
	    printf("malloc buffer failed in mgmt_frame_recv_thread");
		return;
	}

	for (i=0; i<MGMT_FRAME_MAX_NUM; i++)
	{
	    mgmt_frame_recv_buf[i].data = buffer+i*buffer_len;
		mgmt_frame_recv_buf[i].len = 0;
	}

	i=0;
    while (1) 
    {
        if (!mgmt_frame_recv_enabled)
        {
            if (!enabled_flag)
            {
				qurt_signal_wait(&mgmt_filter_start, MGMT_FILTER_MASK_START, QURT_SIGNAL_ATTR_CLEAR_MASK);
            }
        }

	    /*Enable management frame filter*/
        if (mgmt_frame_recv_enabled && (0 == enabled_flag))
        {
			if (QAPI_OK != qapi_WLAN_Enable_Mgmt_Filter(deviceId, mgmt_frame_recv_filter))
			{
				printf("set mgmt frame filter fail\r\n");
			}
			enabled_flag = 1;
        }

		/*Receive management frames*/
      	if(qapi_WLAN_Recv_Mgmt_Frames(mgmt_frame_recv_buf[i].data, buffer_len, &frame_len, 2000) == QAPI_OK)
      	{
			printf("Recv Frame len %d: ", frame_len);
			for (j=0; j<frame_len; j++)
			{
				if (j%16 == 0)
				{
					printf("\r\n");
				}
				printf("%02x", *(mgmt_frame_recv_buf[i].data+j));
			}	
			printf("\r\n\r\n");
            mgmt_frame_recv_buf[i].len = frame_len;
			i= (i+1)%10;

			continue;
      	}
		

		 /*Disable management frame filter*/
	    if ((!mgmt_frame_recv_enabled) && (1 == enabled_flag))
		{
		   qapi_WLAN_Disable_Mgmt_Filter(deviceId);
		   clear_mgmt_frames();
		   enabled_flag = 0;
		}
    } 
}


/**
 * Initialize this management filter demo:
 * - start the mgmt_frame_recv_thread
 */
void Initialize_Mgmt_Filter_Demo(void)
{
    uint32_t ret_val;
	nt_osal_task_handle_t  mgmt_frame_recv_task_hdl;

    ret_val =  (uint32_t)nt_qurt_thread_create(mgmt_frame_recv_thread, "mgmt_frame_recv_thread", 1024, NULL, 6, &mgmt_frame_recv_task_hdl);
    if(ret_val != pdPASS)
    {
  	  printf("MgmtFilter: task creation failed out of memory\r\n");
  	  ASSERT(0);
    } 
	
    ret_val = qurt_signal_create(&mgmt_filter_start);
	
	if (ret_val != 0)
	{
	    printf("failed to create mgmt_filter_start signal", 0);
		nt_osal_thread_delete(mgmt_frame_recv_task_hdl);
		ASSERT(0);
	}  

}


