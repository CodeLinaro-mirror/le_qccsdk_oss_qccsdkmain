/*========================================================================
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*
* @file ring_ctx_holder.h
* @brief Ring Context Holder param and struct definitions
*========================================================================*/

/*-------------------------------------------------------------------------
 * Include Files
 * ----------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "sbl_version_build.h"
#include "binary_descriptor.h"
#include "nt_bl_common.h"
#include "sbl_common.h"
#include "pbl_share.h"
#include "boot_log.h"
#include "nt_gpio_api.h"
#include "qcspi_slave_api.h"
#include "sbl_auth.h"
#include "safeAPI.h"
#include "wifi_fw_sys_img_loader_api.h"

/*-------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * ----------------------------------------------------------------------*/

extern void boot_log_show_system(boot_log *log_ptr);
extern void boot_log_show_handler(boot_log *log_ptr);
extern void boot_log_show_elf_load(boot_log *log_ptr);


//extern sbl_info_s g_sbl_info;

/*Temp add for BIN image*/
#define APP_ELF_RRAM_ADDR			0x2d1000	
#define APP_ARGS_MAGIC				(0x55aa55aa)
//#define SET_APP_ARGS(bin_mode)		((void*)(((uint8_t)(bin_mode))|((APP_ARGS_MAGIC)<<8)))

#define SBL_SHARE_VER 1
#define PART_SIZE     10 /**< part map size */

// define struct with id and addr
typedef struct {
    uint32_t id;
    uint32_t addr;
} IDAddr;

typedef struct {
	uint32_t magic_num;
	uint8_t ver;
	uint8_t img_type;
	uint8_t rsv1;
	uint8_t rsv2;
	uint32_t bdf_addr;
	IDAddr fdt_part[PART_SIZE];
} boot_sbl_share;

#define PBL_VERSION_ADDR			0x200168
#define PBL_VER_MAJOR_CS			16
#define PBL_VER_MINOR_CS			2
#define PBL_VER_COUNT_CS			2
#define	PBL_VER_CS					((PBL_VER_MAJOR_CS << 24) | (PBL_VER_MINOR_CS << 16) | (PBL_VER_COUNT_CS))

typedef void (*elf_entry_args)(void* arg);
elf_entry_args  app_entry;
boot_elf_loader app_elf_loader;
uint32_t __app_image_bdf_addr=0; //for dfu load bdf

boot_pbl_share_func		pbl_share_func;
boot_pbl_share_data		pbl_share_data;
boot_pbl_share_data_es	pbl_share_data_es;

boot_sbl_share sbl_share;

uint32_t				sbl_fde_idx;
char					*pbl_cmd;
fdt_entry				sbl_fde_content;
sbl_prev_fde			sbl_fde_prev;

int						sbl_auth_enable;
void					*secboot_data;
boot_apps_shared_data_t sbl_shared_data;

boot_log				*pbl_log;

const binary_desriptor_t wlan_desriptor_t __attribute__ ((section(".sbl_version_num"))) =
						{ .version = ((SBL_VER_MAJOR << 24) | (SBL_VER_MINOR << 16) | (SBL_VER_COUNT)) };

static uint32_t is_pbl_cs() {
	uint32_t pbl_ver = PBL_VER_CS;

	pbl_ver = HW_REG_RD(PBL_VERSION_ADDR);

	sbl_printf("PBL version 0x%08x\r\n", pbl_ver);

	if (pbl_ver < PBL_VER_CS)
		return 0;
	else
		return 1;
}

void get_pbl_share(void *arg, uint32_t cs_pbl) {
	if (!cs_pbl) {
		pbl_share_func = ((boot_pbl_share_es *)(arg))->func;
		pbl_share_data_es = ((boot_pbl_share_es *)(arg))->data;

		sbl_fde_idx = pbl_share_data_es.sbl_fde.idx;
		sbl_fde_content = pbl_share_data_es.sbl_fde.content;

		pbl_log = pbl_share_data_es.pbl_log;
		pbl_cmd = pbl_share_data_es.cmd;

		sbl_auth_enable = 0;
		secboot_data = pbl_share_data_es.secboot_data;
		sbl_printf("Get PBL share from ES PBL\r\n");
	} else {
		pbl_share_func = ((boot_pbl_share *)(arg))->func;
		pbl_share_data = ((boot_pbl_share *)(arg))->data;

		sbl_fde_idx = pbl_share_data.sbl_fde.idx;
		sbl_fde_content = pbl_share_data.sbl_fde.content;
		sbl_fde_prev = pbl_share_data.prev_fde;

		pbl_log = pbl_share_data.pbl_log;
		pbl_cmd = pbl_share_data.cmd;
		
		sbl_auth_enable = 1;
		secboot_data = pbl_share_data.secboot_data;

		sbl_printf("Get PBL share from CS PBL\r\n");
	}

	return;
}

void set_sbl_share(uint8_t img_type, uint32_t bdf_addr, fdt_s *fdt)
{
	boot_sbl_share *share = &sbl_share;
	for (int i = 0; i < fdt->header.num_fde; i++) {
		share->fdt_part[i].id = fdt->fde[i].id;
		share->fdt_part[i].addr = fdt->fde[i].addr;
	}

	share->magic_num = APP_ARGS_MAGIC;
	share->ver = SBL_SHARE_VER;
	share->img_type = img_type;
	share->bdf_addr = bdf_addr;
}

/*
* @brief: SBL entry function, which contains the dummy code
* @param: pointer to the arguments
* @return: void
*/
void __attribute__ ((section(".dfu_loader_start")))
loader_start( void* arg){
	int err;
	uint32_t idx = FDE_INVAL_IDX;
	uint32_t *msp;
	uint32_t start_addr = APP_IMAGE_START_ADDRESS;

    fdt_s       *fdt = NULL;	
    fdt_entry   fde, tempfde;

	memset(&fde, 0, sizeof(fdt_entry));
	memset(&tempfde, 0, sizeof(fdt_entry));
	
	uint32_t cs_pbl = 1;

	nt_gpio_init();
	nt_uartInit();
	qcspi_slv_init();
	sbl_printf("qcspi slave init\r\n");
	//(0+16+73-1)*4 = 0x160
	//void (**fun_ptr)() = (void (**)())0x160;
	//*fun_ptr = &nt_spi_slv_interrupt;
	
	sbl_printf("\n\tFermion Hosted SBL - v%02d.%02d.%04d \r\n",(int)SBL_VER_MAJOR, (int)SBL_VER_MINOR, (int)SBL_VER_COUNT);
	
	cs_pbl = is_pbl_cs();
	get_pbl_share(arg, cs_pbl);
#ifdef P_DEBUG_PRINT_LOG
	boot_log_show_system(pbl_log);
	boot_log_show_handler(pbl_log);
	boot_log_show_elf_load(pbl_log);
#endif	


	if (pbl_cmd == NULL)
		pbl_cmd = "t";

	// Disable the system tick
   	HW_REG_WR( SYST_CSR_REG, 0 );
    // Clear COUNTFLAG
    HW_REG_WR( SYST_CVR_REG, 0 );

	fdt_get(&fdt);
	__app_image_bdf_addr = get_bdf_addr(fdt);

	sbl_printf("SBL wait host CMD\r\n");
	err = sys_loader();
	if(err == SBL_FUNC_RESET)
	{
		sbl_printf("SBL do reset\r\n");
		sbl_sw_reset();
		while(1);
	}
	else if(err == SBL_FUNC_SUCCESS)
	{
		sbl_printf("HOSTED SBL boot\r\n");
	}
	//switch (*(char *)(pbl_share.data.cmd)){
	switch (*pbl_cmd){

		case 't':

			sbl_printf("PBL picked FDE idx= 0x%08lx, rank=0x%08lx, state=0x%08lx\r\n",
							sbl_fde_idx, sbl_fde_content.rank, sbl_fde_content.state);

			if (cs_pbl)
				sbl_printf("PBL previous picked FDE err= 0x%08lx, rank=0x%08lx, state=0x%08lx\r\n",
							sbl_fde_prev.err_type, sbl_fde_prev.content.rank, sbl_fde_prev.content.state);

			fdt_get(&fdt);

			if(BL_ERR_NONE != boot_sbl_find_appimg(fdt, &idx))
			{
			    idx = FDE_INVAL_IDX;
			}
			
			if (idx != FDE_INVAL_IDX) {
				fde_get_by_idx(fdt, &fde, idx);	

				if (fde.format == OTA_IMG_FORMAT_BIN) {
					start_addr = fde.addr;
					sbl_printf("FDE APP BIN start address 0x%08lx\r\n", start_addr);
				} 

		  
			} 
			break;
		default:			
			break;
	}	

	{
		sbl_printf("Hosted mode: Jump to APP bin\r\n");

		msp = (uint32_t*)start_addr;
	
		app_entry = (elf_entry_args)(*((uint32_t *)(start_addr) + 1));

		sbl_printf("APP entry 0x%08x\r\n", (unsigned int)app_entry);

		__asm volatile ("MSR msp, %0" : : "r" (*msp) : "sp");

		set_sbl_share(OTA_IMG_FORMAT_BIN, get_bdf_addr(fdt), fdt);

		app_entry((void *)&sbl_share);
	}
		
	sbl_printf("SBL shouldn't run here\r\n");
}

bl_error_type boot_sbl_find_appimg(fdt_s *fdt, uint32_t *idx)
{
    uint32_t app_idx=FDE_INVAL_IDX;
    if (!idx || !fdt)
    	return BL_ERR_NULL_PTR;
    	
    fde_get_idx_by_id_rank(fdt, OTA_IMG_ID_APP, OTA_IMG_RANK_TRIAL, &app_idx);
    if (app_idx == FDE_INVAL_IDX) {
        fde_get_idx_by_id_rank(fdt, OTA_IMG_ID_APP, OTA_IMG_RANK_CURRENT, &app_idx);
        if (app_idx == FDE_INVAL_IDX) {
            fde_get_idx_by_id_rank(fdt, OTA_IMG_ID_APP, OTA_IMG_RANK_GOLDEN, &app_idx);
            if (app_idx == FDE_INVAL_IDX) {
                fde_get_idx_by_id_rank(fdt, OTA_IMG_ID_APP, OTA_IMG_RANK_AGED, &app_idx);
            }
        }
    }
    if(app_idx != FDE_INVAL_IDX){
        *idx = app_idx;
		return BL_ERR_NONE;
	}

	return BL_ERR_INVALID_PARAM;
}

bl_error_type boot_sbl_find_bdf(fdt_s *fdt, uint32_t *idx)
{
	uint32_t app_idx=FDE_INVAL_IDX;
	if (!idx || !fdt)
		return BL_ERR_NULL_PTR;

	fde_get_idx_by_id_rank(fdt, OTA_IMG_ID_BDF, OTA_IMG_RANK_CURRENT, &app_idx);

	if (app_idx != FDE_INVAL_IDX) {
		*idx = app_idx;
		return BL_ERR_NONE;
	}

	return BL_ERR_INVALID_PARAM;
}

uint32_t get_bdf_addr(fdt_s *fdt)
{
	uint32_t idx;
	fdt_entry fde;
	uint32_t bdf_addr;

	memset(&fde, 0, sizeof(fdt_entry));

	if(BL_ERR_NONE != boot_sbl_find_bdf(fdt, &idx)) {
		idx = FDE_INVAL_IDX;
	}
	if (idx != FDE_INVAL_IDX) {
		fde_get_by_idx(fdt, &fde, idx);
		bdf_addr = fde.addr;
	} else {
		sbl_printf("use default bdf start addr 0x37A000\r\n");
		bdf_addr = 0x37A000;
	}

	return bdf_addr;
}

//#define SBL_START_DEBUG 1
void sbl_wait_jtag_enter(void)
{
	/* Find proper RRAM address for debugging */
#ifdef SBL_START_DEBUG
	volatile uint8_t *jtag_switch = (uint8_t *)(0x380000-4);
	uint8_t jtag_value = 0xa5;
	while(*jtag_switch == jtag_value);
#endif
}
void boot_sbl_sw_reset( void )
{
	sbl_sw_reset();
}
void sbl_sw_reset( void )
{

	NT_REG_WR(QWLAN_PMU_AON_TOP_CFG_REG, QWLAN_PMU_AON_TOP_CFG_DEFAULT); //Configuring with a value of 0 to choose RC from PMIC
    for(int i=0;i<10000000;i++)
	{
		__asm volatile("nop \n");
	}
	NT_REG_WR(QWLAN_PMU_SYS_SOFT_RESET_REQ_REG, QWLAN_PMU_SYS_SOFT_RESET_REQ_SYS_SOFT_RESET_REQ_MASK);
	__asm volatile("nop \n");
	__asm volatile("nop \n");
	__asm volatile("nop \n");

}



