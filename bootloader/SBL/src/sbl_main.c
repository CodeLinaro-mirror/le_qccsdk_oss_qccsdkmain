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
#include "sbl_auth.h"
#include "sbl_flash_fwd.h"
#if CONFIG_MPU_ENABLE
#include "sbl_mpu.h"
#endif
#include "safeAPI.h"

/*-------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * ----------------------------------------------------------------------*/


extern void boot_log_show_system(boot_log *log_ptr);
extern void boot_log_show_handler(boot_log *log_ptr);
extern void boot_log_show_elf_load(boot_log *log_ptr);

#ifdef FLASH_XIP_SUPPORT
extern uint32_t __app_image_xip_start_addr;
#define APP_IMAGE_START_ADDRESS (uint32_t)&__app_image_xip_start_addr
#else
extern uint32_t __app_image_start_addr;
#define APP_IMAGE_START_ADDRESS (uint32_t)&__app_image_start_addr
#endif
extern sbl_info_s g_sbl_info;

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
	uint32_t app_load_addr = APP_IMAGE_FLASH_ADDRESS; //default virtual flash addr, 0 is invalid.
	boot_elf_load_type type;
    fdt_s       *fdt = NULL;	
    fdt_entry   fde, tempfde;

	memset(&fde, 0, sizeof(fdt_entry));
	memset(&tempfde, 0, sizeof(fdt_entry));
	
	#define MEDIA_TYPE_FLASH 0xA5
	uint32_t app_elf_media = 0;
	uint32_t cs_pbl = 1;

	secboot_auth_image_info_t image_info;

#if CONFIG_MPU_ENABLE
	sbl_mpu_config();
#endif

#ifndef CONFIG_AMBIENT_POWER_ENABLE   
	nt_uartInit();
#endif
	sbl_printf("\n\tFermion SBL - v%02d.%02d.%04d \r\n",(int)SBL_VER_MAJOR, (int)SBL_VER_MINOR, (int)SBL_VER_COUNT);
	
	cs_pbl = is_pbl_cs();
	get_pbl_share(arg, cs_pbl);
#ifdef P_DEBUG_PRINT_LOG
#ifndef CONFIG_AMBIENT_POWER_ENABLE 
	boot_log_show_system(pbl_log);
	boot_log_show_handler(pbl_log);
	boot_log_show_elf_load(pbl_log);
#endif
#endif	


	if (pbl_cmd == NULL)
		pbl_cmd = "t";

	/* Stop bootloader SysTick */
	//HW_REG_WR( SYST_CVR_REG, 0x00 );
	// Disable the system tick
   	HW_REG_WR( SYST_CSR_REG, 0 );
    // Clear COUNTFLAG
    HW_REG_WR( SYST_CVR_REG, 0 );
	
	//switch (*(char *)(pbl_share.data.cmd)){
	switch (*pbl_cmd){
#if 0	
	/*Start for debugging*/	
		case 'l':
		case 'a':
			type = BOOT_ELF_LOAD_RAM;
			break;
		case 's':
			type = BOOT_ELF_LOAD_RAM;
			sbl_auth_enable = 1;
			break;
		case 'f':
			type = BOOT_ELF_LOAD_FULL;
			break;
		case 'c':
			type = BOOT_ELF_LOAD_COPY;
			break;
		case 'j':
			type = BOOT_ELF_LOAD_FULL;
			app_elf_media = MEDIA_TYPE_FLASH;
			//sbl_printf("SBL receive %c \r\n", *(char*)(pbl_share.data.cmd));
			break;
		case 'x':
			type = BOOT_ELF_LOAD_RAM;
			app_elf_media = MEDIA_TYPE_FLASH;
			break;
	/*end for debugging*/			
#endif
		case 't':
			type = BOOT_ELF_LOAD_MAX;

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
				else if(fde.format == OTA_IMG_FORMAT_ELF) 
				{
					type = BOOT_ELF_LOAD_FULL;
					app_elf_media = MEDIA_TYPE_FLASH;
					if(boot_sbl_flash_init() != FLASH_DEVICE_DONE)
					{
						//g_sbl_boot_info.err_code |= 0x1;
					    sbl_printf("Flash:Init error\r\n");
					}
					if(boot_sbl_get_fwd_info() != BL_ERR_NONE)
					    sbl_printf("No fwd info in flash\r\n");
			  }
			} 
			else /* if there's no entry found, do fullload boot from flash*/ 
			{
                type = BOOT_ELF_LOAD_FULL;
                app_elf_media = MEDIA_TYPE_FLASH;
                sbl_printf("No app FDE entry in RRAM\r\n");
				if(boot_sbl_flash_init() != FLASH_DEVICE_DONE)
				{
					//g_sbl_boot_info.err_code |= 0x1;
				    sbl_printf("Flash:Init error\r\n");
				}
				if(boot_sbl_get_fwd_info() != BL_ERR_NONE)
				{
					sbl_printf("No fwd info, try to boot from flash 0x0\r\n");
				}
			}

			break;
		default:
			type = BOOT_ELF_LOAD_MAX;
			break;
	}

	if (type < BOOT_ELF_LOAD_MAX)
	{
		if (sbl_auth_enable)
		{	
	   	    memcpy(&sbl_shared_data, secboot_data, sizeof(boot_apps_shared_data_t));		
			image_info.elf_loader_ptr = &app_elf_loader;
		}
		do{
			if(MEDIA_TYPE_FLASH == app_elf_media)
			{
				/* FULL load, RAM load type based flash FWD table. load_type will be set after
				 * Application indicates ok. 
				 */
				if(fde.addr >= MAX_FWD_NUM)
					g_sbl_info.load_type = BOOT_ELF_LOAD_FULL;

				if(boot_sbl_select_validFwd(&g_sbl_info) < MAX_FWD_NUM)
				{
				    memscpy(&tempfde, sizeof(tempfde), &fde, sizeof(fde));
				    boot_sbl_prepare_set_fde(&fde, &app_load_addr, g_sbl_info.valid_fwd);
	                /* If fde.addr is changed, load_type should be FULL_LOAD */
					type = g_sbl_info.load_type;
					if(type>=BOOT_ELF_LOAD_MAX)
						sbl_printf("load type error \r\n");

	                /* APP_IMAGE_FLASH_ADDRESS would be used by elf init/load */
					app_load_addr += APP_IMAGE_FLASH_ADDRESS;
				}
				sbl_printf("boot_fwd =%x Valid_fwd=%s fwd_Mask=%x load addr=0x%x type=%s\r\n", 
					g_sbl_info.boot_fwd[g_sbl_info.valid_fwd], 
					g_sbl_info.valid_fwd==0?"Trial":g_sbl_info.valid_fwd==1?"Current":"Golden", 
					g_sbl_info.valid_fwd_mask, app_load_addr, 
					type==0?"FULL load":"RAM load");
				err = pbl_share_func.boot_elf_load_init(&app_elf_loader,(uint32_t *)app_load_addr, elf_mem_op, elf_mem_op_num);
				//err = boot_elf_load_init(&app_elf_loader,(uint32_t *)app_load_addr, elf_mem_op, elf_mem_op_num);
				if(err){
					g_sbl_info.err_code |= (0x1<<ERROR_SBL_LOAD_INIT);
					continue;
				}
			}
			else
			{
				err = pbl_share_func.boot_elf_load_init(&app_elf_loader, (uint32_t *)APP_ELF_RRAM_ADDR, elf_mem_op, elf_mem_op_num);
			}
			
			//sbl_printf("APP elf_load_init err=%02d\r\n", err);

			if (sbl_auth_enable)
			{
				if (fde.rank == OTA_IMG_RANK_GOLDEN)
				{
					image_info.image_id= SECBOOT_IMG_AUTH_APP_GOLD_IMG;
				}
				else
				{
					image_info.image_id= SECBOOT_IMG_AUTH_APP_IMG;
				}
				image_info.image_rank = fde.rank;
			
			    err = sbl_image_auth(&image_info, &sbl_shared_data);
			    if(err && (MEDIA_TYPE_FLASH == app_elf_media))
			    {
			        g_sbl_info.err_code |= (0x1<<ERROR_SBL_IMAGE_AUTH);
			        continue;
			    }
				//sbl_printf("sbl image auth err=%02d\r\n", err);
			}

			err = pbl_share_func.boot_elf_load_image(&app_elf_loader, type);
			//err = boot_elf_load_image(&app_elf_loader, type);
			if(err && (MEDIA_TYPE_FLASH == app_elf_media))
			{
		    	g_sbl_info.err_code |= (0x1<<ERROR_SBL_IMAGE_LOAD);
		    	continue;
			}
			//sbl_printf("Load APP elf type=%d done err=%02d\r\n", type, err);
#ifdef FLASH_XIP_SUPPORT			
			boot_sbl_flash_xip_enable(sbl_get_xipstart_offset()+app_load_addr-APP_IMAGE_FLASH_ADDRESS);
#endif		
			if (sbl_auth_enable)
			{   
			    /*Will use app rank when OTA ready*/
	            err = sbl_compute_verify_hash(&image_info, &sbl_shared_data);
		        if(err && (MEDIA_TYPE_FLASH == app_elf_media))
		        {
	    	        g_sbl_info.err_code |= (0x1<<ERROR_SBL_VERIFY_HASH);
#ifdef FLASH_XIP_SUPPORT					
					boot_sbl_flash_xip_disable();
#endif					
	    	        continue;
		        }
		        //sbl_printf("sbl verify hash err=%02d\r\n", err);
			}
		} while(g_sbl_info.err_code!=0);
		
		/* sbl_fde is set for PBL use */
		if ((sbl_fde_content.rank == OTA_IMG_RANK_TRIAL)
				&& (sbl_fde_content.state == OTA_IMG_STATE_VERIFY_PENDING)) 
		{
			sbl_fde_content.state = OTA_IMG_STATE_VALID;
			sbl_printf("SBL image validated for the FDE idx %lu\r\n", sbl_fde_idx);
			
			fde_update(fdt, &(sbl_fde_content), sbl_fde_idx);
		}
		/* Ready to boot APP, set RRAM FDE. pbl_share data should be used before load image
		 * PBL data will be released soon. User can write new fde_update if needed.
		 * idx is invalid means no entry found for app image.
		 */
		if((idx!=FDE_INVAL_IDX) && memcmp(&tempfde, &fde, sizeof(fde)))
		{
		   err = fde_update(fdt, &fde, idx);
		}

		msp = (uint32_t*)APP_IMAGE_START_ADDRESS;
		app_entry = (elf_entry_args)(app_elf_loader.elf_hdr.e_entry);

#if CONFIG_MPU_ENABLE
		sbl_mpu_disable();
#endif

		sbl_printf("APP entry 0x%08x, startAdd=0x%x\r\n", (unsigned int)app_entry, (unsigned int)msp);

		__asm volatile ("MSR msp, %0" : : "r" (*msp) : "sp");

		set_sbl_share(OTA_IMG_FORMAT_ELF, get_bdf_addr(fdt), fdt);

		app_entry((void *)&sbl_share);

	} else {
		sbl_printf("Jump to APP bin\r\n");

		msp = (uint32_t*)start_addr;

		app_entry = (elf_entry_args)(*((uint32_t *)(start_addr) + 1));

#if CONFIG_MPU_ENABLE
		sbl_mpu_disable();
#endif

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

uint8_t boot_sbl_select_validFwd(sbl_info_s *info)
{
	if(info->err_code != ERROR_SBL_NONE)
	{
	    /* SBL has error with err_code so select next */
	    if(info->ota_sbl_exist!=MAX_FW_IMAGE_ENTRIES || info->valid_fwd==INDEX_GOLDEN)
		{
			info->err_code = ERROR_SBL_NONE;
			/* SBL OTA exist or GOLDEN image */
			boot_sbl_sw_reset();
		}

	    if((info->err_code>>ERROR_SBL_IMAGE_LOAD) && (info->valid_fwd<MAX_FWD_NUM))
	    {
			/* This is for LOAD and HASH error, SBL should do reset due to PBL patch/data 
               * Before reset, do update flash FWD for next boot selecting.
			 */
			
			boot_sbl_update_fwd(info->boot_fwd[info->valid_fwd]);
			boot_sbl_sw_reset();
		}
		
		/* There is error for APP image, rollback*/
		if(info->valid_fwd<MAX_FWD_NUM)
		{
		    /*set it invalid with this obsolete valid_fwd*/
			info->flash_fwd_table[info->valid_fwd].status = 0;
			//find next image if error
			info->err_code = ERROR_SBL_NONE;
			(info->valid_fwd)++;
		}
	}
	else
	{
		/* TRIAL image exist and valid */
		if(FWD_TABLE_IS_VALID(INDEX_TRIAL))
		{
			info->valid_fwd = INDEX_TRIAL;
		}
		else if(FWD_TABLE_IS_VALID(INDEX_CURRENT))
		{
			info->valid_fwd = INDEX_CURRENT;
		}
		else if(FWD_TABLE_IS_VALID(INDEX_GOLDEN))
		{
			info->valid_fwd = INDEX_GOLDEN;
		}
		else
		{
		    return MAX_FWD_NUM;
		}
		
		info-> ota_sbl_exist = boot_sbl_find_fwd_image(g_sbl_info.valid_fwd, SBL_IMG_ID);
		if(info-> ota_sbl_exist != MAX_FW_IMAGE_ENTRIES &&
		   sbl_fde_content.rank == OTA_IMG_RANK_CURRENT &&
		   info->valid_fwd == INDEX_TRIAL)
		{
			/* This is special case: there're SBL and APP trial images upgraded.
			* SBL failed and PBL reverts it to current SBL. So SBL would not slect
			* trial APP image again. Use current APP directly.
			*/
		    /*set it invalid with this special INDEX_TRIAL fwd*/
			info->flash_fwd_table[info->valid_fwd].status = 0;
			if(FWD_TABLE_IS_VALID(INDEX_CURRENT))
		    {
			    info->valid_fwd = INDEX_CURRENT;
		    }
			else if(FWD_TABLE_IS_VALID(INDEX_GOLDEN))
			{
				info->valid_fwd = INDEX_GOLDEN;
			}
			sbl_printf("select validfwd %x\r\n", info->valid_fwd);
		}
    }
    return info->valid_fwd;
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
	if((g_sbl_info.valid_fwd == INDEX_GOLDEN)||
		((g_sbl_info.valid_fwd==INDEX_CURRENT) && (!(FWD_TABLE_IS_VALID(INDEX_GOLDEN)))) ||
		((g_sbl_info.valid_fwd==INDEX_TRIAL) && (!(FWD_TABLE_IS_VALID(INDEX_CURRENT)))))
	{
	    /* Last image, don't reset */
		return;
	}

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

#ifdef FLASH_XIP_SUPPORT
uint32_t sbl_get_xipstart_offset()
{
	uint32_t offset = 0x1000;
	boot_elf32_phdr *prog_hdr = app_elf_loader.prog_hdrs;
	//sbl_printf("segment is at %x\r\n", (uint32_t)&__app_image_xip_start_addr);
	for (uint32_t prog_idx = 0; prog_idx < app_elf_loader.phr_num; prog_idx ++)
	{
		if((prog_hdr->p_type==PT_LOAD) && (prog_hdr->p_paddr==(uint32_t)&__app_image_xip_start_addr))
		{
			offset = prog_hdr->p_offset;
			break;
		}
		prog_hdr ++;
	}
	return offset;
}
#endif

