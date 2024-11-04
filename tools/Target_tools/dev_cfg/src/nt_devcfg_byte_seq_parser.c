/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/
/*
 *
 * nt_devcfg_byte_seq_parser.c
 *
 *  Created on: 05-Jan-2021
 *      Author: HIMADRI
 */

#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "nt_devcfg_byte_seq.h"
#include "wlan_dev.h"

#include "nt_devcfg_byte_seq_structure.h"
#include "nt_devcfg_def.h"
#include "nt_devcfg_types.h"
#include "nt_devcfg_from_byte_sequence.h"
#include "nt_logger_api.h"
#define A_MIN(x, y)             (((x) < (y)) ? (x) : (y))

nt_devcfg_byte_seq_structure_t nt_devcfg_byte_instance;			//instance of common byte sequence structure to fill the id from header structure

void nt_devcfg_byte_seq_parse()								// assign the id for each params
{
	int starting_add, end_add;
	int count_1, count_2;
	uint32 *count_ptr;
	uint32 ret_val, device_id;
	uint32 dal_var=DALPROP_PropBin_devcfg_byte_seq_xml[0];
	int total_hex_val ;
	int params_remain = 0;
	total_hex_val=(((int)(dal_var))/4);
	//total_dev_id=(int)(DALPROP_PropBin_qca4020_devcfg_xml[5]);
	device_id=0x04000001;
	if (device_id==0x04000001)												// byte sequence
	{
		count_ptr=&nt_devcfg_byte_instance.byte_seq_one;

		for(count_1=0; count_1<total_hex_val; count_1++)
		{
			if(DALPROP_PropBin_devcfg_byte_seq_xml[count_1]==device_id)
			{
				starting_add=(((int)(DALPROP_PropBin_devcfg_byte_seq_xml[count_1+1]))/4);
				params_remain = GET_NUM_LEFT_DEVCFG(nt_devcfg_byte_seq_structure_t, byte_seq_one);
				end_add = A_MIN(starting_add + params_remain * 2, total_hex_val);
				for(count_2=starting_add; count_2<end_add; count_2+=2)
				{
					ret_val=DALPROP_PropBin_devcfg_byte_seq_xml[count_2+1];
#ifdef NT_DEBUG
				//	NT_LOG_DEV_CFG_APP_INFO("print byte_count value", ret_val);
#endif
					*count_ptr=ret_val;
					count_ptr++;

				}
			}
		}
	}
	return ;
}
void* byte_seq_ptr;
int byte_count=9; // hardcoding it to solve klocwork issue.
uint8_t byte_array[9];
void* nt_devcfg_get_byte_seq_config(int enum_id)			// callback function for byte sequence
{
	uint32 dal_var=DALPROP_PropBin_devcfg_byte_seq_xml[0];
	int total_hex_val ;
	total_hex_val=(((int)(dal_var))/4);
	int count=0 ;
	int to_match = 1, j=8, i=0, k=0;
	static uint32 ret_dev_data;

	uint32 *ret_ptr;
	ret_ptr=&nt_devcfg_byte_instance.byte_seq_one;
	for(to_match=1; ;to_match++ )
	{
		if(to_match==enum_id)
		{
			ret_dev_data=*ret_ptr;
			break;
		}
		ret_ptr++;
	}
	//byte_count =(ret_dev_data&0xFF);
#ifdef NT_DEBUG
	NT_LOG_DEV_CFG_INFO("count of total byte value", (uint32_t)byte_count,0,0);
#endif
	for(i=0; i<=total_hex_val; i++)
	{
		if(DALPROP_PropBin_devcfg_byte_seq_xml[i]==ret_dev_data)
		{
			for(; count <=byte_count; count++, k=k+1)
			{
				byte_array[count] = ((ret_dev_data>>j)&0xFF);
#ifdef NT_DEBUG
				//	NT_LOG_DEV_CFG_INFO("byte values", byte_array[count],0,0);
#endif
				j=j+8 ;
				if(k==2 || k==6 || k==10 || k==14 || k==18 || k==22 )
				{
					ret_dev_data = DALPROP_PropBin_devcfg_byte_seq_xml[i+1];
					j=0;
					i=i+1;
				}
			}
		}
	}
	WLAN_DBG_ARR_PRINT("array of byte sequence :: ", &byte_array[0], (uint16_t)(byte_count+1));
	byte_seq_ptr = byte_array ;
	return(byte_seq_ptr);
}
