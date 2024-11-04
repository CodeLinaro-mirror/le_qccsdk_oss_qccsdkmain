/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/
/*
 *
 *
 * nt_devcfg_parser.c
 *
 *  Created on: 28-May-2020
 *  Author: HIMADRI
 */


#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "nt_devcfg.h"
#include "wlan_dev.h"

#include "nt_devcfg_structure.h"
#include "nt_devcfg_def.h"
#include "nt_devcfg_types.h"
#include "nt_devcfg_from_master_xml.h"
#define A_MIN(x, y)             (((x) < (y)) ? (x) : (y))

nt_devcfg_structure_t nt_device_instance;   			 //instance of common structure to fill from xml

/** device id for all modules :
 *  when new xml is created take the next device_id available
 *  and keep the record of the device id and module names here
 *  device_id==0x03000001 free
 *  device_id==0x03000002 for log params
 *  device_id==0x03000003 for roaming params
 *  device_id==0x03000004 for beacon/power params
 */

/**
 * @brief parsing the xml data
 * Fill the structure nt_common_device_instance_object_s
 * from DALPROP_PropBin_devcfg_xml
 * This function called at main.c
 * For different modules, device id is used as a identification
 * @param none
 * @return none
 */
void nt_devcfg_parse()
{
	uint32 dal_var=DALPROP_PropBin_devcfg_xml[0];
	int  total_hex_val, starting_add, end_add;
	int count_1, count_2;
	int params_remain = 0;
	uint32 *count_ptr;
	uint32 ret_val, device_id;

	total_hex_val=(((int)(dal_var))/4);
	//total_dev_id=(int)(DALPROP_PropBin_qca4020_devcfg_xml[5]);
	device_id=0x03000002;
	if (device_id==0x03000002)													// log params
	{
		count_ptr=&nt_device_instance.NT_DEVCFG_LFS_REG_TYPE;

		for(count_1=0; count_1<total_hex_val; count_1++)
		{
			if(DALPROP_PropBin_devcfg_xml[count_1]==device_id)
			{
				starting_add=(((int)(DALPROP_PropBin_devcfg_xml[count_1+1]))/4);
				params_remain = GET_NUM_LEFT_DEVCFG(nt_devcfg_structure_t, NT_DEVCFG_LFS_REG_TYPE);
				end_add = A_MIN(starting_add + params_remain * 2, total_hex_val);
				for(count_2=starting_add; count_2<end_add; count_2+=2)
				{
					ret_val=DALPROP_PropBin_devcfg_xml[count_2+1];
					*count_ptr=ret_val;
					 count_ptr++;
				}
			}
		}
	}
	device_id=0x03000003;
	if (device_id==0x03000003)													// roaming params
	{
		count_ptr=&nt_device_instance.NT_DEVCFG_FG_SCAN_PROBE_TYPE;

		for(count_1=0; count_1<total_hex_val; count_1++)
		{
			if(DALPROP_PropBin_devcfg_xml[count_1]==device_id)
			{
				starting_add=(((int)(DALPROP_PropBin_devcfg_xml[count_1+1]))/4);
				params_remain = GET_NUM_LEFT_DEVCFG(nt_devcfg_structure_t, NT_DEVCFG_FG_SCAN_PROBE_TYPE);
				end_add = A_MIN(starting_add + params_remain * 2, total_hex_val);
				for(count_2=starting_add; count_2<end_add; count_2+=2)
				{
					ret_val=DALPROP_PropBin_devcfg_xml[count_2+1];
					*count_ptr=ret_val;
					 count_ptr++;
				}
			}
		}
	}
	device_id=0x03000004;
	if (device_id==0x03000004)													// becon/power params
	{
		count_ptr=&nt_device_instance.NT_DEVCFG_INTVAL;

		for(count_1=0; count_1<total_hex_val; count_1++)
		{
			if(DALPROP_PropBin_devcfg_xml[count_1]==device_id)
			{
				starting_add=(((int)(DALPROP_PropBin_devcfg_xml[count_1+1]))/4);
				params_remain = GET_NUM_LEFT_DEVCFG(nt_devcfg_structure_t, NT_DEVCFG_INTVAL);
				end_add = A_MIN(starting_add + params_remain * 2, total_hex_val);
				for(count_2=starting_add; count_2<end_add; count_2+=2)
				{
					ret_val=DALPROP_PropBin_devcfg_xml[count_2+1];
					*count_ptr=ret_val;
					 count_ptr++;
				}
			}
		}
	}
	device_id=0x03000005;
	if (device_id==0x03000005)													// powersave twt
	{
		count_ptr=&nt_device_instance.NT_DEVCFG_TWT_ENABLE;

		for(count_1=0; count_1<total_hex_val; count_1++)
		{
			if(DALPROP_PropBin_devcfg_xml[count_1]==device_id)
			{
				starting_add=(((int)(DALPROP_PropBin_devcfg_xml[count_1+1]))/4);
				params_remain = GET_NUM_LEFT_DEVCFG(nt_devcfg_structure_t, NT_DEVCFG_TWT_ENABLE);
				end_add = A_MIN(starting_add + params_remain * 2, total_hex_val);
				for(count_2=starting_add; count_2<end_add; count_2+=2)
				{
					ret_val=DALPROP_PropBin_devcfg_xml[count_2+1];
					*count_ptr=ret_val;
					 count_ptr++;
				}
			}
		}
	}
	device_id=0x03000006;
	if (device_id==0x03000006)													// protection mode
	{
		count_ptr=&nt_device_instance.NT_DEVCFG_PROTECTION_MODE;

		for(count_1=0; count_1<total_hex_val; count_1++)
		{
			if(DALPROP_PropBin_devcfg_xml[count_1]==device_id)
			{
				starting_add=(((int)(DALPROP_PropBin_devcfg_xml[count_1+1]))/4);
				params_remain = GET_NUM_LEFT_DEVCFG(nt_devcfg_structure_t, NT_DEVCFG_PROTECTION_MODE);
				end_add = A_MIN(starting_add + params_remain * 2, total_hex_val);
				for(count_2=starting_add; count_2<end_add; count_2+=2)
				{
					ret_val=DALPROP_PropBin_devcfg_xml[count_2+1];
					*count_ptr=ret_val;
					count_ptr++;
				}
			}
		}
	}
	device_id=0x03000007;
	if (device_id==0x03000007)													// Rate adaptation
	{
		count_ptr=&nt_device_instance.NT_DEVCFG_RA_FIXED_RATE_INDEX;

		for(count_1=0; count_1<total_hex_val; count_1++)
		{
			if(DALPROP_PropBin_devcfg_xml[count_1]==device_id)
			{
				starting_add=(((int)(DALPROP_PropBin_devcfg_xml[count_1+1]))/4);
				params_remain = GET_NUM_LEFT_DEVCFG(nt_devcfg_structure_t, NT_DEVCFG_RA_FIXED_RATE_INDEX);
				end_add = A_MIN(starting_add + params_remain * 2, total_hex_val);
				for(count_2=starting_add; count_2<end_add; count_2+=2)
				{
					ret_val=DALPROP_PropBin_devcfg_xml[count_2+1];
					*count_ptr=ret_val;
					count_ptr++;
				}
			}
		}
	}

	device_id=0x03000008;
	if (device_id==0x03000008)													// wifi protection
	{
		count_ptr=&nt_device_instance.NT_DEVCFG_WIFISEC_AUTH_INTER_FRM_TIMER;

		for(count_1=0; count_1<total_hex_val; count_1++)
		{
			if(DALPROP_PropBin_devcfg_xml[count_1]==device_id)
			{
				starting_add=(((int)(DALPROP_PropBin_devcfg_xml[count_1+1]))/4);
				params_remain = GET_NUM_LEFT_DEVCFG(nt_devcfg_structure_t, NT_DEVCFG_WIFISEC_AUTH_INTER_FRM_TIMER);
				end_add = A_MIN(starting_add + params_remain * 2, total_hex_val);
				for(count_2=starting_add; count_2<end_add; count_2+=2)
				{
					ret_val=DALPROP_PropBin_devcfg_xml[count_2+1];
					*count_ptr=ret_val;
					count_ptr++;
				}
			}
		}
	}
	device_id=0x03000009;
	if (device_id==0x03000009)													// connection
	{
		count_ptr=&nt_device_instance.NT_DEVCFG_CONN_TIMEOUT;

		for(count_1=0; count_1<total_hex_val; count_1++)
		{
			if(DALPROP_PropBin_devcfg_xml[count_1]==device_id)
			{
				starting_add=(((int)(DALPROP_PropBin_devcfg_xml[count_1+1]))/4);
				params_remain = GET_NUM_LEFT_DEVCFG(nt_devcfg_structure_t, NT_DEVCFG_CONN_TIMEOUT);
				end_add = A_MIN(starting_add + params_remain * 2, total_hex_val);
				for(count_2=starting_add; count_2<end_add; count_2+=2)
				{
					ret_val=DALPROP_PropBin_devcfg_xml[count_2+1];
					*count_ptr=ret_val;
					count_ptr++;
				}
			}
		}
	}
	device_id=0x03000010;
	if (device_id==0x03000010)													// connection
	{
		count_ptr=&nt_device_instance.NT_DEVCFG_WMMP_BE_ACM;

		for(count_1=0; count_1<total_hex_val; count_1++)
		{
			if(DALPROP_PropBin_devcfg_xml[count_1]==device_id)
			{
				starting_add=(((int)(DALPROP_PropBin_devcfg_xml[count_1+1]))/4);
				params_remain = GET_NUM_LEFT_DEVCFG(nt_devcfg_structure_t, NT_DEVCFG_WMMP_BE_ACM);
				end_add = A_MIN(starting_add + params_remain * 2, total_hex_val);
				for(count_2=starting_add; count_2<end_add; count_2+=2)
				{
					ret_val=DALPROP_PropBin_devcfg_xml[count_2+1];
					*count_ptr=ret_val;
					count_ptr++;
				}
			}
		}
	}
	device_id=0x03000011;
	if (device_id==0x03000011)													// mac
	{
		count_ptr=&nt_device_instance.NT_DEVCFG_DEFAULT_MAC_ADDR_AP_P1;

		for(count_1=0; count_1<total_hex_val; count_1++)
		{
			if(DALPROP_PropBin_devcfg_xml[count_1]==device_id)
			{
				starting_add=(((int)(DALPROP_PropBin_devcfg_xml[count_1+1]))/4);
				params_remain = GET_NUM_LEFT_DEVCFG(nt_devcfg_structure_t, NT_DEVCFG_DEFAULT_MAC_ADDR_AP_P1);
				end_add = A_MIN(starting_add + params_remain * 2, total_hex_val);
				for(count_2=starting_add; count_2<end_add; count_2+=2)
				{
					ret_val=DALPROP_PropBin_devcfg_xml[count_2+1];
					*count_ptr=ret_val;
					count_ptr++;
				}
			}
		}
	}
	//device_id=0x03000012;
	//if (device_id==0x03000012)													// RTT_FTM
	//{
	//	count_ptr=&nt_device_instance.NT_DEVCFG_LOCATION_TYPE;

	//	for(count_1=0; count_1<total_hex_val; count_1++)
	//	{
	//		if(DALPROP_PropBin_devcfg_xml[count_1]==device_id)
	//		{
	//			starting_add=(((int)(DALPROP_PropBin_devcfg_xml[count_1+1]))/4);
	//			for(count_2=starting_add; count_2<total_hex_val; count_2+=2)
	//			{
	//				ret_val=DALPROP_PropBin_devcfg_xml[count_2+1];
	//				*count_ptr=ret_val;
	//				count_ptr++;
	//			}
	//		}
	//	}
	//}
//	device_id=0x03000013;
//	if (device_id==0x03000013)													// SYSTEM
//	{
//		count_ptr=&nt_device_instance.NT_DEVCFG_UART_BAUD_RATE;
//
//		for(count_1=0; count_1<total_hex_val; count_1++)
//		{
//			if(DALPROP_PropBin_devcfg_xml[count_1]==device_id)
//			{
//				starting_add=(((int)(DALPROP_PropBin_devcfg_xml[count_1+1]))/4);
//				for(count_2=starting_add; count_2<total_hex_val; count_2+=2)
//				{
//					ret_val=DALPROP_PropBin_devcfg_xml[count_2+1];
//					*count_ptr=ret_val;
//					count_ptr++;
//				}
//			}
//		}
//	}
	device_id=0x03000014;
	if (device_id==0x03000014)													// NEUT_2
	{
		count_ptr=&nt_device_instance.NT2_DEVCFG_ENABLE_DISABLE_RRAM_DXE;

		for(count_1=0; count_1<total_hex_val; count_1++)
		{
			if(DALPROP_PropBin_devcfg_xml[count_1]==device_id)
			{
				starting_add=(((int)(DALPROP_PropBin_devcfg_xml[count_1+1]))/4);
				params_remain = GET_NUM_LEFT_DEVCFG(nt_devcfg_structure_t, NT2_DEVCFG_ENABLE_DISABLE_RRAM_DXE);
				end_add = A_MIN(starting_add + params_remain * 2, total_hex_val);
				for(count_2=starting_add; count_2<end_add; count_2+=2)
				{
					ret_val=DALPROP_PropBin_devcfg_xml[count_2+1];
					*count_ptr=ret_val;
					count_ptr++;
				}
			}
		}
	}
	return;
}

/** enum calling the callback function
 *  nt_convert_ascii() is used to pass the character values
 */
/**
 * @brief call back function from modules
 * This will take the enum id from modules
 * and will compare with the common structure
 * when matched it will return the value.
 * nt_devcfg_get_config() is used to return uint32 values.
 * This function can call from any module
 * @param enum_id            enum called from modules
 * @return ret_devcfg_value  pointer to configuration value
 */
void* nt_devcfg_get_config(int enum_id)
{

		static uint32 ret_dev_data;
		void* ret_devcfg_value;
		//uint32 *ret_ptr;

		//ret_ptr=&nt_device_instance.NT_DEVCFG_AP_MAX_NUM_CONN;
		ret_dev_data = *( &nt_device_instance.NT_DEVCFG_LFS_REG_TYPE + enum_id - 1 );
		ret_devcfg_value = &ret_dev_data;

		return(ret_devcfg_value);
}
/**
 * @brief call back function from modules
 * This will take the enum id from modules
 * and will compare with the common structure
 * when matched it will return the value.
 * nt_devcfg_get_ascii_config() is used to return ascii values.
 * This function can call from any module
 * @param enum_id            enum called from modules
 * @return ret_devcfg_value  pointer to configuration value
 */
uint8_t strsize;
void* nt_devcfg_get_ascii_config(int enum_id)
{
	int to_match = 1;
	static uint32 ret_dev_data;
	void* ret_devcfg_value;
	uint32 *ret_ptr;
	uint8_t temp;
	static uint8_t out_ascii[255];
	ret_ptr=&nt_device_instance.NT_DEVCFG_LFS_REG_TYPE;
	for(to_match=1; ;to_match++ )
	{
		if(to_match==enum_id)
			{
				ret_dev_data= *ret_ptr;
				memset(out_ascii,0x0,255);
				temp = (uint8_t)(ret_dev_data&0x0f);
				temp = temp + 0x37;
				out_ascii[0] = temp;
#ifdef NT_DEBUG
				//NT_LOG_DEV_CFG_INFO((char*)out_ascii,0,0,0);  // E
#endif
				temp = (uint8_t)(ret_dev_data&0xf0);
				temp = temp >>4;
				temp = (temp&0xf);
				temp= temp + 0x37;
				out_ascii[1] = temp;
#ifdef NT_DEBUG
				//NT_LOG_DEV_CFG_INFO((char*)out_ascii,0,0,0); // ED
#endif
				//temp = (uint8_t)(ret_dev_data&0x00000f00);
#ifdef NT_DEBUG
				//	NT_LOG_DEV_CFG_INFO("print value", (ret_dev_data&0x00000f00)>>8,0,0);
#endif
				temp = (uint8_t)((ret_dev_data&0x00000f00)>>8);
#ifdef NT_DEBUG
				//	NT_LOG_DEV_CFG_INFO("print value", temp,0,0);
#endif
				//temp = temp >>4;
				//temp = (temp&0xf);
				temp= temp + 0x37;
				out_ascii[2] = temp;
#ifdef NT_DEBUG
				//	NT_LOG_DEV_CFG_INFO((char*)out_ascii,0,0,0); // EDC
//					NT_LOG_DEV_CFG_INFO("a", out_ascii[0],0,0);
//					NT_LOG_DEV_CFG_INFO("b", out_ascii[1],0,0);
//					NT_LOG_DEV_CFG_INFO("c", out_ascii[2],0,0);
//					NT_LOG_DEV_CFG_INFO("d", out_ascii[3],0,0);
#endif
				temp = (uint8_t)((ret_dev_data&0x0000f000)>>12);
				temp= temp + 0x37;
				out_ascii[3] = temp;
#ifdef NT_DEBUG
				//	NT_LOG_DEV_CFG_INFO((char*)out_ascii,0,0,0); // EDCB
#endif
				temp = (uint8_t)((ret_dev_data&0x000f0000)>>16);
				temp= temp + 0x37;
				out_ascii[4] = temp;
#ifdef NT_DEBUG
				//NT_LOG_DEV_CFG_INFO((char*)out_ascii,0,0,0); // EDCBA
				//NT_LOG_DEV_CFG_INFO((char*)out_ascii,0,0,0); // EDCBA
#endif
				ret_devcfg_value=out_ascii;
#ifdef NT_DEBUG
				//NT_LOG_DEV_CFG_INFO((char*)ret_devcfg_value,0,0,0);	// EDCBA
#endif
				strsize = sizeof((char*)ret_devcfg_value) + 1 ;
				break;
			}
		ret_ptr++;
	}
	return(ret_devcfg_value);
}
