/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/
/*========================================================================
*
* @file wifi_fw_sys_img_loader.h
* @brief Contains the structures and APIs which needs to be used in \
* ferm_sys_img_loader.c
*========================================================================*/
#ifndef _WIFI_FW_SYS_IMG_LOADER_
#define _WIFI_FW_SYS_IMG_LOADER_

#include "wifi_fw_dfu_api.h"
#include "nt_gpio_api.h"

#define sbl_printf(...) do{\
	char buff[100];\
	snprintf(buff,sizeof(buff),__VA_ARGS__);\
	nt_pbl_printf(buff);\
	}while(0)

#define MAX_NUMBER_OF_ELEMENTS (2)
#define NUMBER_OF_ELEMENTS     (2)
#define NO_OF_DFU_PARAM        (1)
#define NO_OF_DFU_PARAM        (1)
#define BUFF_START_ADDR        (0x10000)
#define A2F_ELEMENT_LEN        (0x6000) //24k
#define F2A_ELEMENT_LEN        (0x200)  //512
#define BOUNDARY_ELEMENT_LEN   (4)
#define BOUNDARY_ELEMENT_DWORD (0xFACEFACE)
#define ONE_KILO_BYTES         (0x400)

#define DFU_BUFF_SIZE          (MAX_DFU_DATA_LEN)

#if (DFU_BUFF_SIZE > (A2F_ELEMENT_LEN - ONE_KILO_BYTES))
#error "DFU_BUFF_SIZE  is greater than allowed"
#endif

#define INVALID_FW_VERSION     (0xFFFFFFFF)

#define MULTI_USE_GPIO        GPIO_PIN_10
#define NT2APPS_GPIO    GPIO_PIN_8

typedef enum ring_direction
{
	F2A_RING,
	A2F_RING,
}ring_direction_t;

typedef struct dfu_ring_element
{
uint32_t *p_buf; /* Pointer to Fermion memory buffer*/
uint16_t len;  /* Length of the packet */
uint16_t info; /* Info describing the packet type */

/* Scratch buff */
void *p_buf_start; /* Start of the buf used for freeing */
}dfu_ring_element_t;

typedef struct ring_data_desc
{
	uint8_t* p_buff;
	uint32_t data_len;
	uint8_t index;
}ring_data_desc_t;


typedef volatile struct ring_handle
{
uint8_t dfu_ring_elem_size; /* size of each elememnt of DFU ring */
uint8_t dfu_ring_num_elems; /* Maximum Num elements in DFU ring */
uint8_t reserved[2];
uint32_t dfu_read_idx; /* DFU read index */
uint32_t dfu_write_idx; /* DFU write index */
dfu_ring_element_t dfu_ring_element[MAX_NUMBER_OF_ELEMENTS];
}ring_handle_t;

typedef volatile struct dfu_ring_handle
{
	ring_handle_t dfu_ring;
	ring_handle_t shadow_dfu_ring;
}dfu_ring_handle_t;

typedef enum sys_img_load_state
{
	WAIT_FOR_DFU,
	CHECK_FOR_DFU_BITMAP,
	REQUEST_ELF_HEADER,
	WAIT_FOR_ELF_HEADER,
	REQUEST_PROGRAM_HEADER,
	WAIT_FOR_PROGRAM_HEADER,
	REQUEST_PROGRAM,
	WAIT_FOR_PROGRAM,
	RESOURCE_TRANSFER_COMPLETED,
	RESOURCE_TRANSFER_CONFIRM,
	SEND_REBOOT_CMD,
	RESOURCE_WAIT_FOR_REBOOT,
}sys_img_load_state_t;

typedef struct sys_loader_cntxt
{
	soft_id_t softid_in_progress;
	uint32_t start_offset;
	uint32_t file_size;
	uint32_t phy_addr;
	uint32_t prog_len;
	uint32_t end_offset;
	uint32_t running_offset;
	uint16_t total_pkts;
	uint16_t curr_pkt_count;
	uint8_t softid_dfu_bitmap;
	uint8_t softid_dfu_done_bitmap;
	uint8_t num_of_ph;
}sys_loader_cntxt_t;

typedef struct dfu_bitmap{
   uint8_t sbl:1;
   uint8_t wlan:1;
   uint8_t bdf:1;
   uint8_t reg_db:1;
   uint8_t reserved:4;
} dfu_bitmap_t;

#if 0
typedef struct tiny_loader_table {
	uint32_t table_hdr; /* Table Header pattern for host verification */
	uint32_t* p_f2a_message; /* address holding the f2a message */
	uint32_t* p_a2f_message; /* address holding the a2f message */
	uint32_t buff_len; /* buff length for the buffer allocated for transfering the elf SBL elf */
	void* p_buff; /* address of the buffer */
} tiny_loader_table_t;
#endif

extern int8_t nt_bl_rram_write(void *dst, const void *wdata,uint32_t length);
extern uint32_t __fdt_reg_st_addr;
extern uint32_t __config_table;

#endif /* _WIFI_FW_SYS_IMG_LOADER_ */
