/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/
/*========================================================================
*
* @file wifi_fw_sys_img_loader.c
* @brief The functionalities which implements the loader for system image.
*========================================================================*/

/*-------------------------------------------------------------------------
 * Include Files
 * ----------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "nt_bl_uart.h"
#include "wifi_fw_table_api.h"
#include "fdt.h"
#include "sbl_common.h"
#include "wifi_fw_sys_img_loader_api.h"
#include "wifi_fw_sys_img_loader.h"
#include "wifi_fw_ring_api.h"
#include "elf.h"

static dfu_ring_handle_t a2f_dfu_ring_handle, f2a_dfu_ring_handle;
static wifi_fw_dfu_defaults_t shadow_fermion_defaults;
static sys_loader_cntxt_t sys_loader_cntxt;
static fw_desc_tbl_t *p_ferm_fdt = (fw_desc_tbl_t*)(&__fdt_reg_st_addr);
static wifi_fw_dfu_defaults_t *p_fermion_dfu_defaults;

static int g_dfu_bin = 1;
/*
* @brief: fermion to host interrupt generation.
* @param: void
* @return: void
*/
void fermion_to_apps_interrupt(void)
{
    //nt_gpio_pin_write(NT_GPIOA,NT2APPS_GPIO,NT_GPIO_HIGH);
    for(int i = 0; i < 20; i++)
    {
	  __asm volatile(" nop \n");
    }
    //nt_gpio_pin_write(NT_GPIOA,NT2APPS_GPIO,NT_GPIO_LOW);
}

/*
* @brief: For checking the dfu ring integrity
* @param:  p_dfu_ring_param, reference to the handle for dfu_ring which needs to be checked.
* @param: direction, host to fermion or fermion to host
* @return: returns status of the functionality
*/
static sbl_func_status_t check_dfu_ring_integrity(dfu_ring_handle_t* p_dfu_ring_param, ring_direction_t direction)
{
	sbl_func_status_t status = SBL_FUNC_FAILURE;
	ring_handle_t *p_dfu_ring, *p_shadow_ring;
	
	p_dfu_ring = &(p_dfu_ring_param->dfu_ring);
	p_shadow_ring = &(p_dfu_ring_param->shadow_dfu_ring);
	
	if(p_dfu_ring->dfu_ring_elem_size != p_shadow_ring->dfu_ring_elem_size)
	{
		sbl_printf("dfu_ring_elem_size mismatch\r\n");
	}
	if(p_dfu_ring->dfu_ring_num_elems != p_shadow_ring->dfu_ring_num_elems)
	{
		sbl_printf("dfu_ring_num_elems mismatch\r\n");
	}
	if(A2F_RING == direction)
	{
		if(p_dfu_ring->dfu_read_idx != p_shadow_ring->dfu_read_idx)
		{
			sbl_printf("dfu_read_idx mismatch\r\n");
		}
		if(p_dfu_ring->dfu_read_idx >= p_dfu_ring->dfu_ring_num_elems)
		{
			sbl_printf("dfu_read_idx crossed limit\r\n");
		}
		if(p_dfu_ring->dfu_write_idx >= p_shadow_ring->dfu_ring_num_elems)
		{
			sbl_printf("dfu_write_idx crossed limit\r\n");
		}
	}
	else
	{
		if(p_dfu_ring->dfu_write_idx != p_shadow_ring->dfu_write_idx)
		{
			sbl_printf("dfu_write_idx mismatch\r\n");
		}
		if(p_dfu_ring->dfu_write_idx >= p_shadow_ring->dfu_ring_num_elems)
		{
			sbl_printf("dfu_write_idx crossed limit\r\n");
		}
		if(p_dfu_ring->dfu_read_idx >= p_dfu_ring->dfu_ring_num_elems)
		{
			sbl_printf("dfu_read_idx crossed limit\r\n");
		}
	}

	for(int i = 0; i < MAX_NUMBER_OF_ELEMENTS; i++)
	{
		if(p_dfu_ring->dfu_ring_element[i].p_buf != p_shadow_ring->dfu_ring_element[i].p_buf)
		{
			sbl_printf("dfu_ring_element %d mismatch\r\n",i);
		}
	}
	//to do: bitmap check on each of the if cases and appropriatly fill status
	status = SBL_FUNC_SUCCESS;
	return status;
}

/*
* @brief: For checking the integrity of the entire table
* @param:  void
* @return: returns status of the functionality
*/
static sbl_func_status_t check_config_table_integrity(void)
{
	sbl_func_status_t status = SBL_FUNC_FAILURE;
	if(!memcmp(&shadow_fermion_defaults,p_fermion_dfu_defaults, sizeof(wifi_fw_dfu_defaults_t)))
	status = SBL_FUNC_SUCCESS;
	return status;
}

/*
* @brief: For incrementing the read index of the ring
* @param:  p_ring_handle, the reference to the ring handle.
* @param:  intrpt_host, make this true if an interrupt host is needed after the ring index update. 
* @return: void
*/
static void increment_read_index(dfu_ring_handle_t* p_ring_handle, bool intrpt_host)
{
	uint8_t read_index;
	read_index = p_ring_handle->dfu_ring.dfu_read_idx;
	read_index = (read_index + 1) % (p_ring_handle->dfu_ring.dfu_ring_num_elems);;
	p_ring_handle->shadow_dfu_ring.dfu_read_idx = read_index;
	p_ring_handle->dfu_ring.dfu_read_idx = read_index;
	if(true == intrpt_host)
	{
		fermion_to_apps_interrupt();
	}
}

/*
* @brief: For incrementing the write index of the ring
* @param:  p_ring_handle, the reference to the ring handle.
* @param:  intrpt_host, make this true if an interrupt host is needed after the ring index update. 
* @return: void
*/
static void increment_write_index(dfu_ring_handle_t* p_ring_handle, bool intrpt_host)
{
	uint8_t write_index;
	write_index = p_ring_handle->dfu_ring.dfu_write_idx;
	write_index = (write_index + 1) % (p_ring_handle->dfu_ring.dfu_ring_num_elems);;
	p_ring_handle->shadow_dfu_ring.dfu_write_idx = write_index;
	p_ring_handle->dfu_ring.dfu_write_idx = write_index;
	if(true == intrpt_host)
	{
		fermion_to_apps_interrupt();
	}
}

/*
* @brief: For attaching the buffer to the elements in the ring
* @param:  direction, takes the direction: host to fermion or fermionto host
* @param:  index, the element index
* @return: returns status of the functionality
*/
bool attach_buffer_to_element(ring_direction_t direction, uint8_t index)
{
	bool status = false;
	uint8_t *p_a2f_buf_start, *p_f2a_buf_start;

	if(index<MAX_NUMBER_OF_ELEMENTS)
	{
		p_a2f_buf_start = (uint8_t*)BUFF_START_ADDR + sizeof(wifi_fw_dfu_defaults_t);
		p_f2a_buf_start = p_a2f_buf_start + (MAX_NUMBER_OF_ELEMENTS*(A2F_ELEMENT_LEN+BOUNDARY_ELEMENT_LEN));	
		if(direction == A2F_RING)
		{
			uint8_t *p_a2f_buf;
		    p_a2f_buf = p_a2f_buf_start + index*(A2F_ELEMENT_LEN+BOUNDARY_ELEMENT_LEN);
			a2f_dfu_ring_handle.dfu_ring.dfu_ring_element[index].p_buf = (uint32_t*)p_a2f_buf;
			a2f_dfu_ring_handle.dfu_ring.dfu_ring_element[index].len = A2F_ELEMENT_LEN;
			a2f_dfu_ring_handle.dfu_ring.dfu_ring_element[index].info = CONFIG_INFO_DFU;
			a2f_dfu_ring_handle.shadow_dfu_ring.dfu_ring_element[index].p_buf = (uint32_t*)p_a2f_buf;
			a2f_dfu_ring_handle.shadow_dfu_ring.dfu_ring_element[index].len = A2F_ELEMENT_LEN;
			p_a2f_buf += A2F_ELEMENT_LEN;
			*((uint32_t*)p_a2f_buf) = 0xFACEFACE;
			status = true;
		}
		else
		{
			uint8_t *p_f2a_buf;
		    p_f2a_buf = p_f2a_buf_start + index*(F2A_ELEMENT_LEN+BOUNDARY_ELEMENT_LEN);
			f2a_dfu_ring_handle.dfu_ring.dfu_ring_element[index].p_buf = (uint32_t*)p_f2a_buf;
			f2a_dfu_ring_handle.dfu_ring.dfu_ring_element[index].len = F2A_ELEMENT_LEN;
			f2a_dfu_ring_handle.dfu_ring.dfu_ring_element[index].info = CONFIG_INFO_DFU;
			f2a_dfu_ring_handle.shadow_dfu_ring.dfu_ring_element[index].p_buf = (uint32_t*)p_f2a_buf;
			f2a_dfu_ring_handle.shadow_dfu_ring.dfu_ring_element[index].len = F2A_ELEMENT_LEN;
			p_f2a_buf += F2A_ELEMENT_LEN;
			*((uint32_t*)p_f2a_buf) = 0xFACEFACE;
			status = true;
		}
	}
	return status;
}

/*
* @brief: For dettaching the buffer to the elements in the ring
* @param:  direction, takes the direction: host to fermion or fermionto host
* @param:  index, the element index
* @return: returns status of the functionality
*/
bool detach_buffer_from_element(ring_direction_t direction, uint8_t index)
{
	bool status = false;
	uint8_t *p_a2f_buf_start, *p_f2a_buf_start;

	if(index<MAX_NUMBER_OF_ELEMENTS)
	{
		p_a2f_buf_start = (uint8_t*)BUFF_START_ADDR + sizeof(wifi_fw_dfu_defaults_t);
		p_f2a_buf_start = p_a2f_buf_start + (MAX_NUMBER_OF_ELEMENTS*(A2F_ELEMENT_LEN+BOUNDARY_ELEMENT_LEN));	
		if(direction == A2F_RING)
		{
			uint8_t *p_a2f_buf;
			uint32_t magic_pattern;
		    p_a2f_buf = p_a2f_buf_start + index*(A2F_ELEMENT_LEN+BOUNDARY_ELEMENT_LEN);
			a2f_dfu_ring_handle.dfu_ring.dfu_ring_element[index].p_buf = NULL;
			a2f_dfu_ring_handle.dfu_ring.dfu_ring_element[index].len = 0;
			a2f_dfu_ring_handle.shadow_dfu_ring.dfu_ring_element[index].p_buf = NULL;
			a2f_dfu_ring_handle.shadow_dfu_ring.dfu_ring_element[index].len = 0;
			p_a2f_buf += A2F_ELEMENT_LEN;
			magic_pattern = *((uint32_t*)p_a2f_buf);
			if(0xFACEFACE != magic_pattern)
			{
				sbl_printf("WARNING: a2f Buffer was over written: magic_pattern= %x \r\n",(unsigned int)magic_pattern);
			}
			status = true;
		}
		else
		{
			uint8_t *p_f2a_buf;
			uint32_t magic_pattern;
		    p_f2a_buf = p_f2a_buf_start + index*(F2A_ELEMENT_LEN+BOUNDARY_ELEMENT_LEN);
			f2a_dfu_ring_handle.dfu_ring.dfu_ring_element[index].p_buf = NULL;
			f2a_dfu_ring_handle.dfu_ring.dfu_ring_element[index].len = 0;
			f2a_dfu_ring_handle.shadow_dfu_ring.dfu_ring_element[index].p_buf = NULL;
			f2a_dfu_ring_handle.shadow_dfu_ring.dfu_ring_element[index].len = 0;
			p_f2a_buf += F2A_ELEMENT_LEN;
			magic_pattern = *((uint32_t*)p_f2a_buf);
			if(0xFACEFACE != magic_pattern)
			{
				sbl_printf("WARNING: f2a Buffer was over written: magic_pattern= %x \r\n",(unsigned int)magic_pattern);
			}
			status = true;
		}
	}
	return status;
}

/*
* @brief: initilises the ring parameters which used to configure the table
* @param:  void
* @return: void
*/
static void init_ring_param(void)
{
	memset((void*)&a2f_dfu_ring_handle,0,sizeof(dfu_ring_handle_t));
	memset((void*)&f2a_dfu_ring_handle,0,sizeof(dfu_ring_handle_t));
	
	a2f_dfu_ring_handle.dfu_ring.dfu_ring_elem_size = sizeof(dfu_ring_element_t);
	a2f_dfu_ring_handle.dfu_ring.dfu_ring_num_elems = NUMBER_OF_ELEMENTS;
	a2f_dfu_ring_handle.dfu_ring.dfu_read_idx = 0;
	a2f_dfu_ring_handle.dfu_ring.dfu_write_idx = 0;

	f2a_dfu_ring_handle.dfu_ring.dfu_ring_elem_size = sizeof(dfu_ring_element_t);
	f2a_dfu_ring_handle.dfu_ring.dfu_ring_num_elems = NUMBER_OF_ELEMENTS;
	f2a_dfu_ring_handle.dfu_ring.dfu_read_idx = 0;
	f2a_dfu_ring_handle.dfu_ring.dfu_write_idx = 0;

#if 0
	uint8_t *p_a2f_buf, *p_f2a_buf;
	p_a2f_buf = (uint8_t*)BUFF_START_ADDR + sizeof(fermion_boot_table_t);
	p_f2a_buf = p_a2f_buf + (MAX_NUMBER_OF_ELEMENTS*(A2F_ELEMENT_LEN+BOUNDARY_ELEMENT_LEN));
	for(int i = 0; i < MAX_NUMBER_OF_ELEMENTS; i++)
	{
		a2f_dfu_ring_handle.dfu_ring.dfu_ring_element[i].p_buf = (uint32_t*)p_a2f_buf;
		a2f_dfu_ring_handle.dfu_ring.dfu_ring_element[i].len = A2F_ELEMENT_LEN;
		p_a2f_buf += A2F_ELEMENT_LEN;
		*((uint32_t*)p_a2f_buf) = 0xFACEFACE;
		p_a2f_buf += BOUNDARY_ELEMENT_LEN;

		f2a_dfu_ring_handle.dfu_ring.dfu_ring_element[i].p_buf = (uint32_t*)p_f2a_buf;
		f2a_dfu_ring_handle.dfu_ring.dfu_ring_element[i].len = F2A_ELEMENT_LEN;
		p_f2a_buf += F2A_ELEMENT_LEN;
		*((uint32_t*)p_f2a_buf) = 0xFACEFACE;
		p_f2a_buf += BOUNDARY_ELEMENT_LEN;
	}
#else
	for(int i = 0; i < MAX_NUMBER_OF_ELEMENTS; i++)
	{
		attach_buffer_to_element(A2F_RING, i);
		attach_buffer_to_element(F2A_RING, i);
	}
#endif
	memcpy((void*)&(a2f_dfu_ring_handle.shadow_dfu_ring), (void*)&(a2f_dfu_ring_handle.dfu_ring), sizeof(ring_handle_t));
	memcpy((void*)&(f2a_dfu_ring_handle.shadow_dfu_ring), (void*)&(f2a_dfu_ring_handle.dfu_ring), sizeof(ring_handle_t));
}

/*
* @brief: initilises the config table
* @param:  void
* @return: void
*/
static void init_config_table(void)
{
	init_ring_param();
	p_fermion_dfu_defaults =  (wifi_fw_dfu_defaults_t*)&__config_table;
	memset(p_fermion_dfu_defaults,0,sizeof(wifi_fw_dfu_defaults_t));
	p_fermion_dfu_defaults->table_len = sizeof(wifi_fw_dfu_defaults_t);
	p_fermion_dfu_defaults->wifi_fw_pbl_ver = 0;
	p_fermion_dfu_defaults->wifi_fw_wlan_ver = 0;//p_ferm_fdt->wlan_img->version;
	p_fermion_dfu_defaults->wifi_fw_sbl_ver = 0;//p_ferm_fdt->sbl_a->version;
	p_fermion_dfu_defaults->wifi_fw_bdf_ver = 0;//p_ferm_fdt->bdf->version;
	p_fermion_dfu_defaults->wifi_fw_reg_db_ver = 0;//p_ferm_fdt->reg_db->version;
	p_fermion_dfu_defaults->wifi_fw_ini_ver = 0;//p_ferm_fdt->wlan_ini->version;

	if(p_ferm_fdt->wlan_img.dfu_status != DFU_FINISHED)
	{
        p_fermion_dfu_defaults->wifi_fw_wlan_ver = INVALID_FW_VERSION;
	}
	if(p_ferm_fdt->sbl_a.dfu_status != DFU_FINISHED)
	{
        p_fermion_dfu_defaults->wifi_fw_sbl_ver = INVALID_FW_VERSION;
	}
	if(p_ferm_fdt->bdf.dfu_status != DFU_FINISHED)
	{
        p_fermion_dfu_defaults->wifi_fw_bdf_ver = INVALID_FW_VERSION;
	}
	if(p_ferm_fdt->reg_db.dfu_status != DFU_FINISHED)
	{
        p_fermion_dfu_defaults->wifi_fw_reg_db_ver = INVALID_FW_VERSION;
	}
	if(p_ferm_fdt->wlan_ini.dfu_status != DFU_FINISHED)
	{
        p_fermion_dfu_defaults->wifi_fw_ini_ver = INVALID_FW_VERSION;
	}

	p_fermion_dfu_defaults->a2f_dfu_ring_elem_size = a2f_dfu_ring_handle.dfu_ring.dfu_ring_elem_size;
	p_fermion_dfu_defaults->f2a_dfu_ring_elem_size = f2a_dfu_ring_handle.dfu_ring.dfu_ring_elem_size;
	p_fermion_dfu_defaults->a2f_dfu_ring_num_elems = a2f_dfu_ring_handle.dfu_ring.dfu_ring_num_elems;
	p_fermion_dfu_defaults->f2a_dfu_ring_num_elems = f2a_dfu_ring_handle.dfu_ring.dfu_ring_num_elems;

	p_fermion_dfu_defaults->p_a2f_dfu_read_idx = (uint32_t*)&(a2f_dfu_ring_handle.dfu_ring.dfu_read_idx);
	p_fermion_dfu_defaults->p_f2a_dfu_write_idx = (uint32_t*)&(f2a_dfu_ring_handle.dfu_ring.dfu_write_idx);
	p_fermion_dfu_defaults->p_f2a_dfu_read_idx = (uint32_t*)&(f2a_dfu_ring_handle.dfu_ring.dfu_read_idx);
	p_fermion_dfu_defaults->p_a2f_dfu_write_idx = (uint32_t*)&(a2f_dfu_ring_handle.dfu_ring.dfu_write_idx);

	p_fermion_dfu_defaults->p_a2f_dfu_ring_base = (void*)&(a2f_dfu_ring_handle.dfu_ring.dfu_ring_element[0]);
	p_fermion_dfu_defaults->p_f2a_dfu_ring_base = (void*)&(f2a_dfu_ring_handle.dfu_ring.dfu_ring_element[0]);
	memcpy(&shadow_fermion_defaults, p_fermion_dfu_defaults, sizeof(wifi_fw_dfu_defaults_t));
	shadow_fermion_defaults.table_hdr = WIFI_FW_SBL_ENTERED_PATTERN;
	p_fermion_dfu_defaults->table_hdr = WIFI_FW_SBL_ENTERED_PATTERN;
}

void clear_dfu_table_hdr(void)
{
    p_fermion_dfu_defaults->table_hdr = 0x0;
}

/*
* @brief: to check the a2f dfu ring is empty
* @param:  void
* @return: bool, TRUE if empty
*/
static bool a2f_dfu_ring_check_empty(void)
{
	bool empty_status = true;
	sbl_func_status_t table_check, ring_check;

	table_check = check_config_table_integrity();
	ring_check = check_dfu_ring_integrity(&a2f_dfu_ring_handle, A2F_RING);
	if((table_check == SBL_FUNC_SUCCESS) && (ring_check == SBL_FUNC_SUCCESS))
	{
		if(a2f_dfu_ring_handle.dfu_ring.dfu_read_idx ^ a2f_dfu_ring_handle.dfu_ring.dfu_write_idx )
		{
			empty_status = false;
			//sbl_printf("SBL_FUNC_SUCCESS: a2f_dfu_read_idx  = %lu, a2f_dfu_write_idx = %lu \r\n",a2f_dfu_ring_handle.dfu_ring.dfu_read_idx, a2f_dfu_ring_handle.dfu_ring.dfu_write_idx);
		}
		else
		{
			empty_status = true;
		}
	}
	else
	{
		sbl_printf("FAILED: Config table check = %d, Ring check = %d \r\n",table_check,ring_check);
	}
	return empty_status;
}

/*
* @brief: to check the f2a dfu ring occupancy
* @param:  void
* @return: bool, TRUE if there is an occupancy
*/
static bool f2a_dfu_ring_check_occupancy(void)
{

	bool occ_status = false;
	sbl_func_status_t table_check, ring_check;

	table_check = check_config_table_integrity();
	ring_check = check_dfu_ring_integrity(&f2a_dfu_ring_handle, F2A_RING);
	if((table_check == SBL_FUNC_SUCCESS) && (ring_check == SBL_FUNC_SUCCESS))
	{
		if(f2a_dfu_ring_handle.dfu_ring.dfu_read_idx != ((f2a_dfu_ring_handle.dfu_ring.dfu_write_idx + 1) % f2a_dfu_ring_handle.dfu_ring.dfu_ring_num_elems))
		{
			occ_status = true;
		}
		else
		{
			sbl_printf("NO OCCUPANCY: f2a_dfu_read_idx	= %lu, f2a_dfu_write_idx = %lu \r\n",f2a_dfu_ring_handle.dfu_ring.dfu_read_idx, f2a_dfu_ring_handle.dfu_ring.dfu_write_idx);
		}
	}
	else
	{
		sbl_printf("FAILED: Config table check = %d, Ring check = %d \r\n",table_check,ring_check);
	}
	return occ_status;	
}


/*
* @brief: to check the f2a dfu ring is empty
* @param:  void
* @return: bool, TRUE if empty
*/
static bool f2a_dfu_ring_check_empty(void)
{
	bool empty_status = true;
	sbl_func_status_t table_check, ring_check;

	table_check = check_config_table_integrity();
	ring_check = check_dfu_ring_integrity(&f2a_dfu_ring_handle, F2A_RING);
	if((table_check == SBL_FUNC_SUCCESS) && (ring_check == SBL_FUNC_SUCCESS))
	{
		if(f2a_dfu_ring_handle.dfu_ring.dfu_read_idx ^ f2a_dfu_ring_handle.dfu_ring.dfu_write_idx )
		{
			empty_status = false;
			sbl_printf("SBL_FUNC_SUCCESS: f2a_dfu_read_idx  = %lu, f2a_dfu_write_idx = %lu \r\n",f2a_dfu_ring_handle.dfu_ring.dfu_read_idx, f2a_dfu_ring_handle.dfu_ring.dfu_write_idx);
		}
		else
		{
			empty_status = true;
		}
	}
	else
	{
		sbl_printf("FAILED: Config table check = %d, Ring check = %d \r\n",table_check,ring_check);
	}
	return empty_status;
}

/*
* @brief: to recieve the dfu request
* @param:  reference to the bitmap which will get copied with the dfu bitmap which needs an update.
* @return: bool, TRUE if the frame recieved successfully
*/
static bool recieve_dfu_req(uint8_t *p_bitmap)
{
	bool empty_status, update_req_status;
	a2f_dfu_start_req_t *p_a2f_dfu_req;
	uint8_t read_index;
	uint32_t *p_buf;

	empty_status = a2f_dfu_ring_check_empty();
	if(false == empty_status)
	{
		read_index = a2f_dfu_ring_handle.dfu_ring.dfu_read_idx;
		p_buf = a2f_dfu_ring_handle.dfu_ring.dfu_ring_element[read_index].p_buf;
		p_a2f_dfu_req = (a2f_dfu_start_req_t*)p_buf;
		if(A2F_DFU_REQ == p_a2f_dfu_req->header.msg_id)
		{
			*p_bitmap = p_a2f_dfu_req->soft_id_bitmap;
			a2f_dfu_ring_handle.dfu_ring.dfu_ring_element[read_index].len = A2F_ELEMENT_LEN;
			increment_read_index(&a2f_dfu_ring_handle, false);
			sys_loader_cntxt.file_size = p_a2f_dfu_req->dfu_imgsize;
//			sbl_printf("kkk recieve_dfu_req index = %d filesize=%d\r\n", a2f_dfu_ring_handle.dfu_ring.dfu_read_idx, sys_loader_cntxt.file_size);
			update_req_status = true;
		}
		else
		{
			update_req_status = false;
		}
	}
	else
	{
		update_req_status = false;
	}
	return update_req_status;
}

/*
* @brief: to send the dfu confirm
* @param:  void
* @return: bool, TRUE if the frame has successfully copied to the ring
*/
static bool send_dfu_confirm(void)
{
	bool status = false;
	uint8_t write_index;
	uint32_t *p_buf;
	status = f2a_dfu_ring_check_occupancy();
	if(true == status)
	{
		write_index = f2a_dfu_ring_handle.dfu_ring.dfu_write_idx;
		p_buf = f2a_dfu_ring_handle.dfu_ring.dfu_ring_element[write_index].p_buf;
		f2a_dfu_start_cfm_t *p_f2a_dfu_cfm;
		p_f2a_dfu_cfm = (f2a_dfu_start_cfm_t*)p_buf;
		p_f2a_dfu_cfm->header.msg_id = F2A_DFU_CFM;
		p_f2a_dfu_cfm->header.status = DFU_MSG_SUCCESS;
		f2a_dfu_ring_handle.dfu_ring.dfu_ring_element[write_index].len = sizeof(f2a_dfu_start_cfm_t);
		increment_write_index(&f2a_dfu_ring_handle, true);
		sbl_printf("SBL: F2A_DFU_CFM sent.\r\n");
	}
	return status;
}

/*
* @brief: to send the dfu data transfer request
* @param:  running_offset, which indicates the offset of elf
* @param:  prog_len, which indicates the length in bytes which is requested from host
* @param:  soft_id, which indicates the resource id.
* @return: bool, TRUE if the frame has successfully copied to the ring
*/
static bool send_dfu_data_transfer_req(uint32_t running_offset, uint32_t prog_len, uint8_t soft_id)
{
	bool status = false;
	uint8_t write_index;
	uint32_t *p_buf;
	status = f2a_dfu_ring_check_occupancy();
	if(true == status)
	{
		write_index = f2a_dfu_ring_handle.dfu_ring.dfu_write_idx;
		p_buf = f2a_dfu_ring_handle.dfu_ring.dfu_ring_element[write_index].p_buf;
		f2a_dfu_transfer_req_t *p_f2a_dfu_transfer_req;
		p_f2a_dfu_transfer_req = (f2a_dfu_transfer_req_t*)p_buf;
		p_f2a_dfu_transfer_req->header.msg_id = F2A_DFU_DATA_TRANSFER_REQ;
		p_f2a_dfu_transfer_req->header.status = DFU_MSG_SUCCESS;
		p_f2a_dfu_transfer_req->soft_id = soft_id;
		p_f2a_dfu_transfer_req->program_len = prog_len;
		p_f2a_dfu_transfer_req->elf_offset = running_offset;
		f2a_dfu_ring_handle.dfu_ring.dfu_ring_element[write_index].len = sizeof(f2a_dfu_transfer_req_t);
		increment_write_index(&f2a_dfu_ring_handle, true);
		sbl_printf("SBL: F2A_DFU_DATA_TRANSFER_REQ sent.\r\n");
	}
	return status;
}

/*
* @brief: to recieve the dfu data transfer response
* @param:  pdata_desc, reference to the data descriptor
* @return: bool, TRUE if the frame has successfully recieved
*/
static bool recieve_dfu_data_transfer_confirm(ring_data_desc_t* pdata_desc)
{
	bool empty_status, data_transfer_resp_status = false;
	a2f_dfu_transfer_cfm_t *p_a2f_dfu_transfer_cfm;
	uint8_t read_index;
	uint32_t *p_buf;

	empty_status = a2f_dfu_ring_check_empty();
	if(false == empty_status)
	{
		read_index = a2f_dfu_ring_handle.dfu_ring.dfu_read_idx;
		p_buf = a2f_dfu_ring_handle.dfu_ring.dfu_ring_element[read_index].p_buf;
		p_a2f_dfu_transfer_cfm = (a2f_dfu_transfer_cfm_t*)p_buf;
		if(A2F_DFU_DATA_TRANSFER_CFM == p_a2f_dfu_transfer_cfm->dfu_transfer_header.header.msg_id)
		{
			pdata_desc->index = read_index;
			pdata_desc->p_buff = &(p_a2f_dfu_transfer_cfm->data[0]);
			pdata_desc->data_len = p_a2f_dfu_transfer_cfm->dfu_transfer_header.program_len;
			detach_buffer_from_element(A2F_RING, read_index);
			
			uint8_t prog_len_check, running_offset_check, soft_id_check;
			prog_len_check = (p_a2f_dfu_transfer_cfm->dfu_transfer_header.program_len == sys_loader_cntxt.prog_len)?1:0;
			running_offset_check = (p_a2f_dfu_transfer_cfm->dfu_transfer_header.elf_offset == sys_loader_cntxt.running_offset)?1:0;
			soft_id_check = (p_a2f_dfu_transfer_cfm->dfu_transfer_header.soft_id == sys_loader_cntxt.softid_in_progress)?1:0;
			if(prog_len_check && running_offset_check && soft_id_check)
			{
				//sbl_printf("SUCCESS:prog_len_check = %d,running_offset_check = %d,  soft_id_check = %d\r\n", prog_len_check, running_offset_check, soft_id_check);
			}
		}
		increment_read_index(&a2f_dfu_ring_handle, false);
		data_transfer_resp_status = true;
	}
	else{
		//sbl_printf("recieve_dfu_data_transfer_confirm false\r\n");
	}
	return data_transfer_resp_status;
}

/*
* @brief: to send the reboot command to the host.
* @param:  void
* @return: bool, TRUE if the frame has successfully copied to the ring
*/
static bool send_dfu_reboot_fermion_req(void)
{
	bool status = false;
	uint8_t write_index;
	uint32_t *p_buf;
	status = f2a_dfu_ring_check_occupancy();
	if(true == status)
	{
		write_index = f2a_dfu_ring_handle.dfu_ring.dfu_write_idx;
		p_buf = f2a_dfu_ring_handle.dfu_ring.dfu_ring_element[write_index].p_buf;
		f2a_dfu_reboot_req_t *p_f2a_dfu_reboot_req;
		p_f2a_dfu_reboot_req = (f2a_dfu_reboot_req_t*)p_buf;
		p_f2a_dfu_reboot_req->header.msg_id = F2A_DFU_REBOOT_REQ;
		p_f2a_dfu_reboot_req->header.status = DFU_MSG_SUCCESS;
		f2a_dfu_ring_handle.dfu_ring.dfu_ring_element[write_index].len = sizeof(f2a_dfu_reboot_req_t);
		increment_write_index(&f2a_dfu_ring_handle, true);
		sbl_printf("SBL: F2A_DFU_REBOOT_IND sent.\r\n");
	}
	return status;
}

/*
* @brief: to send the trnsfer done indication to host
* @param:  void
* @return: bool, TRUE if the frame has successfully copied to the ring
*/
static bool send_dfu_transfer_done_ind(void)
{
	bool status = false;
	uint8_t write_index;
	uint32_t *p_buf;
	status = f2a_dfu_ring_check_occupancy();
	if(true == status)
	{
		write_index = f2a_dfu_ring_handle.dfu_ring.dfu_write_idx;
		p_buf = f2a_dfu_ring_handle.dfu_ring.dfu_ring_element[write_index].p_buf;
		f2a_dfu_transfer_done_ind_t *p_f2a_dfu_transfer_done_ind;
		p_f2a_dfu_transfer_done_ind = (f2a_dfu_transfer_done_ind_t*)p_buf;
		p_f2a_dfu_transfer_done_ind->header.msg_id = F2A_DFU_DATA_TRANSFER_DONE_IND;
		p_f2a_dfu_transfer_done_ind->header.status = DFU_MSG_SUCCESS;
		p_f2a_dfu_transfer_done_ind->soft_id = sys_loader_cntxt.softid_in_progress;
		f2a_dfu_ring_handle.dfu_ring.dfu_ring_element[write_index].len = sizeof(f2a_dfu_transfer_done_ind_t);
		increment_write_index(&f2a_dfu_ring_handle, true);
		sbl_printf("SBL: F2A_DFU_DATA_TRANSFER_DONE_IND sent.\r\n");
	}
	return status;
}

/*
* @brief: to recieve the trnsfer confirm from host
* @param:  void
* @return: bool, TRUE if the frame has successfully recieved
*/
static bool recieve_dfu_transfer_done_rsp(uint8_t* p_more_resource)
{
	bool empty_status, trnsfr_done_cfm_status;
	a2f_dfu_transfer_done_rsp_t* p_a2f_dfu_transfer_done_rsp;
	uint8_t read_index;
	uint32_t *p_buf;

	empty_status = a2f_dfu_ring_check_empty();
	if(false == empty_status)
	{
		read_index = a2f_dfu_ring_handle.dfu_ring.dfu_read_idx;
		p_buf = a2f_dfu_ring_handle.dfu_ring.dfu_ring_element[read_index].p_buf;
		p_a2f_dfu_transfer_done_rsp = (a2f_dfu_transfer_done_rsp_t*)p_buf;
		if(A2F_DFU_DATA_TRANSFER_DONE_RSP == p_a2f_dfu_transfer_done_rsp->header.msg_id)
		{
            *p_more_resource = p_a2f_dfu_transfer_done_rsp->more_resource;
			a2f_dfu_ring_handle.dfu_ring.dfu_ring_element[read_index].len = A2F_ELEMENT_LEN;
			increment_read_index(&a2f_dfu_ring_handle, false);
			trnsfr_done_cfm_status = true;
//			sbl_printf("kkk receive A2F_DFU_DATA_TRANSFER_DONE_RSP\n");
		}
		else
		{
			trnsfr_done_cfm_status = false;
		}
	}
	else
	{
		trnsfr_done_cfm_status = false;
	}
	return trnsfr_done_cfm_status;
}


/*
* @brief: The state machine for the image loader for multiple resources
* @param:  void
* @return: sbl_func_status_t
*/
sbl_func_status_t sys_loader(void)
{
	sbl_func_status_t status = SBL_FUNC_FAILURE;
//	uint8_t last_packet = 0;
	sys_img_load_state_t state = WAIT_FOR_DFU;
	init_config_table();
	//fermion_to_apps_interrupt();

	while (1)
	{
		switch (state)
		{
			case WAIT_FOR_DFU:

				if(true == recieve_dfu_req(&sys_loader_cntxt.softid_dfu_bitmap))
				{
					sbl_printf("SUCCESS:DFU Update request recieved: bitmap = %x\r\n", sys_loader_cntxt.softid_dfu_bitmap);
					state = CHECK_FOR_DFU_BITMAP;
					if(sys_loader_cntxt.softid_dfu_bitmap == HOST_CMD_RESET)
					{
						//sbl_printf("do sbl reset\r\n");
						return SBL_FUNC_RESET;
						//sbl_sw_reset();
						//while(1);
					}
					else if(sys_loader_cntxt.softid_dfu_bitmap == HOST_CMD_BOOT)
					{
						sbl_printf("do sbl boot\r\n");
						return SBL_FUNC_SUCCESS;
					}
				}
				else
				{
					  for(int i=0;i<1000000;i++)
						{
							__asm volatile("nop \n");
						}
				}
				break;
				
			case CHECK_FOR_DFU_BITMAP:
					if(send_dfu_confirm())
					{
						if(!(sys_loader_cntxt.softid_dfu_bitmap))
						{
						    bool empty_status = false;
						    while(false == empty_status) // this is for confiming that host has processed all commands in f2a ring.
						    {
						      empty_status = f2a_dfu_ring_check_empty();
						    }
							status = SBL_FUNC_SUCCESS;
							return status;
						}
						else
						{
							sys_loader_cntxt.softid_dfu_done_bitmap = 0;
							//state = REQUEST_ELF_HEADER;
							if(g_dfu_bin) //for bin image upgrade
							{
								uint8_t dfu_soft_id = 0, dfu_bitmap;
								dfu_bitmap = sys_loader_cntxt.softid_dfu_bitmap ^ sys_loader_cntxt.softid_dfu_done_bitmap;
								if(dfu_bitmap & WLAN_IMG_ID)
								{
									dfu_soft_id = WLAN_IMG_ID;
								}
								else if(dfu_bitmap & BDF_ID)
								{
									dfu_soft_id = BDF_ID;
								}
								
								uint32_t img_filesize = sys_loader_cntxt.file_size;
								sys_loader_cntxt.total_pkts = img_filesize/DFU_BUFF_SIZE; //should get from host
						
										
								uint32_t remain_len;
								remain_len = (img_filesize)%DFU_BUFF_SIZE;
								if(remain_len)
								{
									sys_loader_cntxt.total_pkts += 1;
								}
									
								if(sys_loader_cntxt.total_pkts==0)
									sys_loader_cntxt.total_pkts += 1;
								sys_loader_cntxt.curr_pkt_count = 0;
								
								sys_loader_cntxt.start_offset = 0;
								//sys_loader_cntxt.running_offset = running_offset;
								sys_loader_cntxt.softid_in_progress = dfu_soft_id;
								sys_loader_cntxt.prog_len = DFU_BUFF_SIZE;
					
								state = REQUEST_PROGRAM;
								//sbl_printf("KKK send confirm, filesize=%d, remain=%d proglen=%x\r\n",
								//	img_filesize, remain_len, DFU_BUFF_SIZE);
							}
						}
					}
				break;
		
			case REQUEST_PROGRAM:
				{
					uint32_t running_offset, prog_len;
					uint8_t soft_id;
					uint32_t current_pkt_cnt;

					current_pkt_cnt = sys_loader_cntxt.curr_pkt_count;
					if(current_pkt_cnt <= sys_loader_cntxt.total_pkts)
					{
						uint32_t total_packets = sys_loader_cntxt.total_pkts;
						if(current_pkt_cnt == (total_packets-1))
						{
							prog_len = (sys_loader_cntxt.file_size)%DFU_BUFF_SIZE;
							if(!prog_len)
								prog_len = DFU_BUFF_SIZE;
						}
						else
						{
							prog_len = DFU_BUFF_SIZE;
						}
						sys_loader_cntxt.prog_len = prog_len;
						running_offset = sys_loader_cntxt.start_offset + current_pkt_cnt*DFU_BUFF_SIZE;
						sys_loader_cntxt.running_offset = running_offset;

						soft_id = sys_loader_cntxt.softid_in_progress;
						//sbl_printf("in REQUEST_PROGRAM, offset=%x prog_len=%x soft_id=%d\r\n", running_offset, prog_len, soft_id);
						if(send_dfu_data_transfer_req(running_offset, prog_len, soft_id))
						{
							state = WAIT_FOR_PROGRAM;
						}

					}
					else
					{
						state = RESOURCE_TRANSFER_COMPLETED;
					}

				}
				break;

			case WAIT_FOR_PROGRAM:
				{
					ring_data_desc_t ring_data_desc;
					memset(&ring_data_desc,0,sizeof(ring_data_desc_t));

					if(recieve_dfu_data_transfer_confirm(&ring_data_desc))
					{
						//uint8_t *dst = (uint8_t*)(sys_loader_cntxt.phy_addr+(sys_loader_cntxt.running_offset-sys_loader_cntxt.start_offset));
						uint8_t *src = ring_data_desc.p_buff;
						uint32_t rram_file_start_add=0;
						if(sys_loader_cntxt.softid_in_progress == WLAN_IMG_ID)
						{
							rram_file_start_add = APP_IMAGE_START_ADDRESS;
						}
						else if(sys_loader_cntxt.softid_in_progress == BDF_ID)
						{
							rram_file_start_add = __app_image_bdf_addr;//0x37A000;
						}
						uint32_t dst = rram_file_start_add+(sys_loader_cntxt.running_offset-sys_loader_cntxt.start_offset);
						uint32_t length = ring_data_desc.data_len;
						
						int rram_write_status=0;
						
						if(((sys_loader_cntxt.softid_in_progress == WLAN_IMG_ID) &&
							(dst<APP_IMAGE_START_ADDRESS || dst>RAW_FILE_START_ADDRESS))
							|| ((sys_loader_cntxt.softid_in_progress == BDF_ID) &&
							(dst<0x37a000 || dst>0x380000)))
						{
							sbl_printf("rram write error dst=0x%x, softid=%d\r\n", dst,sys_loader_cntxt.softid_in_progress);
											break;
						}
#if 0						
						{
							sbl_printf("wait for program to id=%d, dst add=%x\r\n",
								sys_loader_cntxt.softid_in_progress, dst);
						}
#endif						
						//todo RRAM write via DXE
						rram_write_status = boot_sbl_rram_write((uint32_t)dst, src, length);
						
						if(rram_write_status)
						{
							sbl_printf("SBL: ERROR: ****problem with RRAM write*** %d \r\n",rram_write_status);
						}

						attach_buffer_to_element(A2F_RING, ring_data_desc.index);
						sys_loader_cntxt.curr_pkt_count++;
						sbl_printf("dst=0x%x, src=%x, length=%x \r\n",
							(unsigned int)dst,(unsigned int)src,(unsigned int)length);
						//sbl_printf(" %02x %02x %02x %02x - %02x %02x %02x %02x\n",
						//	src[0],src[1],src[4],src[5],src[length-6],src[length-5],src[length-2],src[length-1]);
						if(sys_loader_cntxt.curr_pkt_count < sys_loader_cntxt.total_pkts)
						{
							state = REQUEST_PROGRAM;
						}
						else
							state = RESOURCE_TRANSFER_COMPLETED;
					}
				}
				break;

			case RESOURCE_TRANSFER_COMPLETED:
					if(send_dfu_transfer_done_ind())
					{
						state = RESOURCE_TRANSFER_CONFIRM;
					}
				break;

			case RESOURCE_TRANSFER_CONFIRM:
				{
					soft_id_t soft_id = sys_loader_cntxt.softid_in_progress;
                    uint8_t more_resource = 0;
					if(recieve_dfu_transfer_done_rsp(&more_resource))
					{
						//sbl_printf("SBL: LOG : recieved transfer confirm resource=%x id=%x\r\n", more_resource,soft_id);

						sys_loader_cntxt.softid_dfu_done_bitmap = (sys_loader_cntxt.softid_dfu_done_bitmap) | soft_id;
                        if(more_resource)
                        {
                            state = WAIT_FOR_DFU; //fermion expects a dfu bitmap from host when the more resource bit is set.
                        }
						else if(soft_id >= WLAN_IMG_ID)
						{			
							//state = REQUEST_ELF_HEADER;
							state = WAIT_FOR_DFU;
						}
						else
						{
							state = SEND_REBOOT_CMD;
						}
						sbl_printf("Done, Wait new req state=%x\r\n", state);
					}
				}
				break;
					
			case SEND_REBOOT_CMD:
					if(send_dfu_reboot_fermion_req())
					{
						sbl_printf("SBL: LOG : wating for reboot \r\n");
						state = RESOURCE_WAIT_FOR_REBOOT;
						return SBL_FUNC_SUCCESS;
					}

				break;
			case RESOURCE_WAIT_FOR_REBOOT:
				state = WAIT_FOR_DFU;
				
			break;

		}
		/*
	for(int i=0;i<1000000;i++)
	{
		__asm volatile("nop \n");
	}
	*/
	}
}
