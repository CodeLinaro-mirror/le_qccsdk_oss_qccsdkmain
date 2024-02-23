/*========================================================================
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
#include "nt_bl_uart.h"
#include "wifi_fw_sys_img_loader_api.h"
#include "binary_descriptor.h"
#include "boot_elf_loader.h"
#include "nt_bl_common.h"
#include "sbl_common.h"
#include "rram_fdt.h"
#include "pbl_share.h"
#include "boot_log.h"
#include "sbl_auth.h"

/*-------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * ----------------------------------------------------------------------*/


extern void boot_log_show_system(boot_log *log_ptr);
extern void boot_log_show_handler(boot_log *log_ptr);
extern void boot_log_show_elf_load(boot_log *log_ptr);
extern uint32_t __app_image_start_addr;
#define APP_ELF_RRAM_ADDR 0x2d1000
#define APP_IMAGE_START_ADDRESS (uint32_t)&__app_image_start_addr
boot_elf_loader app_elf_loader;

typedef void (*elf_entry_args)(void* arg);
#define APP_ARGS_MAGIC	(0x5a)
#define SET_APP_ARGS(bin_mode)	((void*)(((uint8_t)(bin_mode))|((APP_ARGS_MAGIC)<<8)))
elf_entry_args  app_entry;

boot_pbl_share	pbl_share;
boot_log	*pbl_log;
boot_apps_shared_data_type sbl_shared_data;
pbl_secboot_shared_info_type    secboot_ftbl;
static char boot_print_buf[100];

const binary_desriptor_t wlan_desriptor_t __attribute__ ((section(".sbl_version_num"))) = { .version = ((SBL_VER_MAJOR << 24) | (SBL_VER_MINOR << 16) | (SBL_VER_COUNT)) };
/*
* @brief: SBL entry function, which contains the dummy code
* @param: pointer to the arguments
* @return: void
*/
void __attribute__ ((section(".dfu_loader_start")))
loader_start( void* arg){
	int err;
	uint32_t idx;
	uint32_t *msp;
	uint32_t start_addr = APP_IMAGE_START_ADDRESS;
	uint32_t app_load_addr = APP_IMAGE_FLASH_ADDRESS; //default virtual flash addr, 0 is invalid.
	boot_elf_load_type type;
	fdt_s *fdt;
	sbl_ota_fde sbl_fde;
	fdt_entry	fde;
	int status;
	//temp media type flag for test purpose, this should be removed when OTA/FDT is finalized.
	#define MEDIA_TYPE_FLASH 0xA5
	uint32_t app_elf_media = 0;

	int sbl_auth_enable = 0;
    //pbl_secboot_shared_info_type *secboot_shared_info;
	secboot_auth_image_info_s image_info;
	pbl_secboot_shared_info_type *p_secboot_ftbl = &secboot_ftbl;

	nt_uartInit();

	sbl_printf("\n\tFermion SBL - v%02d.%02d.%04d \r\n",(int)SBL_VER_MAJOR, (int)SBL_VER_MINOR, (int)SBL_VER_COUNT);

	sbl_wait_jtag_enter();

	pbl_share = *((boot_pbl_share *)(arg));
	pbl_log = pbl_share.data.pbl_log;

	boot_log_show_system(pbl_log);
	boot_log_show_handler(pbl_log);
	boot_log_show_elf_load(pbl_log);

	if (pbl_share.data.cmd == NULL)
		pbl_share.data.cmd = "t";

	/* Stop bootloader SysTick */
	//HW_REG_WR( SYST_CVR_REG, 0x00 );
	// Disable the system tick
   	HW_REG_WR( SYST_CSR_REG, ~(SYST_CSR_ENABLE) );

	switch (*(char *)(pbl_share.data.cmd)){
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
		case 't':
			type = BOOT_ELF_LOAD_MAX;
			sbl_auth_enable = 1;

			sbl_fde = pbl_share.data.sbl_fde;
			pbl_share.func.fdt_get(&fdt);

			if ((sbl_fde.content.rank == OTA_IMG_RANK_TRIAL)
					&& (sbl_fde.content.state == OTA_IMG_STATE_VERIFY_PENDING)) {
				sbl_fde.content.state = OTA_IMG_STATE_VALID;
				sbl_printf("SBL image validated for the FDE idx %lu\r\n", sbl_fde.idx);

				pbl_share.func.fde_update(fdt, &(sbl_fde.content), sbl_fde.idx);
			}

			pbl_share.func.fde_get_idx_by_id_rank(fdt, OTA_IMG_ID_APP, OTA_IMG_RANK_CURRENT, &idx);

			if (idx != FDE_INVAL_IDX) {
				pbl_share.func.fde_get_by_idx(fdt, &fde, idx);

				if (fde.format == OTA_IMG_FORMAT_BIN) {
					start_addr = fde.addr;
					sbl_printf("FDE APP BIN start address 0x%08lx\r\n", start_addr);
                }
			    else if(fde.format == OTA_IMG_FORMAT_ELF)
                {
					type = BOOT_ELF_LOAD_FULL;
					app_elf_media = MEDIA_TYPE_FLASH;
					if(fde.state == OTA_IMG_STATE_NEW) /* 1st boot*/
					{
					    /*temp for testing*/
					    type = BOOT_ELF_LOAD_RAM;
					}
					if(type == BOOT_ELF_LOAD_RAM){
						sbl_printf("SBL RAM load from Flash \r\n");
					}
					else{
						sbl_printf("SBL FULL load from Flash \r\n");
					}
					app_load_addr = fde.addr;
					sbl_printf("FDE APP ELF load address 0x%08lx\r\n", app_load_addr);
				}
			}
			else /* if there's no entry found, do fullload boot from flash*/
			{
				type = BOOT_ELF_LOAD_FULL;
				app_elf_media = MEDIA_TYPE_FLASH;
				sbl_printf("No FDE, APP ELF load address 0x%08lx\r\n", app_load_addr);
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
	   	    memcpy(&sbl_shared_data, pbl_share.data.secboot_data, sizeof(boot_apps_shared_data_type));
		    memcpy(p_secboot_ftbl, sbl_shared_data.secboot_shared_data, sizeof(pbl_secboot_shared_info_type));
		    sbl_shared_data.secboot_shared_data = p_secboot_ftbl;
			image_info.elf_loader_ptr = &app_elf_loader;
			if (sbl_fde.content.rank == OTA_IMG_RANK_GOLDEN)
			{
				image_info.image_id= SECBOOT_IMG_AUTH_APP_GOLD_IMG;
			}
			else
			{
				image_info.image_id= SECBOOT_IMG_AUTH_APP_IMG;
			}
		}

		if(MEDIA_TYPE_FLASH == app_elf_media)
		{
			status = boot_sbl_flash_init();
			if(status != FLASH_DEVICE_DONE)
			{
			    sbl_printf("Flash:Init status=%02d\r\n", status);
			}
            if(app_load_addr<APP_IMAGE_FLASH_ADDRESS)
            {
                sbl_printf("SBL will use default flash add 0xA00000\r\n");
                app_load_addr = APP_IMAGE_FLASH_ADDRESS;
            }
			err = pbl_share.func.boot_elf_load_init(&app_elf_loader,(uint32_t *)app_load_addr, elf_mem_op, elf_mem_op_num);
		}
		else
		{
			err = pbl_share.func.boot_elf_load_init(&app_elf_loader, (uint32_t *)APP_ELF_RRAM_ADDR, elf_mem_op, elf_mem_op_num);
		}

		sbl_printf("APP elf_load_init err=%02d\r\n", err);

		if (sbl_auth_enable)
		{
	        err = sbl_image_auth(&image_info, &sbl_shared_data);
			sbl_printf("sbl image auth err=%02d\r\n", err);
		}

		err = pbl_share.func.boot_elf_load_image(&app_elf_loader, type);
		sbl_printf("Load APP elf type=%d done err=%02d\r\n", type, err);

		if (sbl_auth_enable)
		{
		    /*Will use app rank when OTA ready*/
	        err = sbl_compute_verify_hash(&sbl_shared_data, sbl_fde.content.rank);
			sbl_printf("sbl verify hash err=%02d\r\n", err);
		}

		msp = (uint32_t*)APP_IMAGE_START_ADDRESS;
		app_entry = (elf_entry_args)(app_elf_loader.elf_hdr.e_entry);

		sbl_printf("APP entry 0x%08x, startAdd=0x%x\r\n", (unsigned int)app_entry, (unsigned int)msp);

		__asm volatile ("MSR msp, %0" : : "r" (*msp) : "sp");

		app_entry(SET_APP_ARGS(OTA_IMG_FORMAT_ELF));

	} else {
		sbl_printf("Jump to APP bin\r\n");

		msp = (uint32_t*)start_addr;

		app_entry = (elf_entry_args)(*((uint32_t *)(start_addr) + 1));

		sbl_printf("APP entry 0x%08x\r\n", (unsigned int)app_entry);

		__asm volatile ("MSR msp, %0" : : "r" (*msp) : "sp");

		app_entry(SET_APP_ARGS(OTA_IMG_FORMAT_BIN));
	}

	sbl_printf("SBL shouldn't run here\r\n");
}

void sbl_wait_jtag_enter(void)
{
	/* Find proper RRAM address for debugging */
#ifdef SBL_START_DEBUG
	volatile uint8_t *jtag_switch = (uint8_t *)(0x380000-4);
	uint8_t jtag_value = 0xa5;
	while(*jtag_switch == jtag_value);
#endif
}

void sbl_printf(const char *fmt, ...)
{
	va_list argp;

    snprintf(boot_print_buf,sizeof(boot_print_buf), "SBL ");
    nt_pbl_printf(boot_print_buf);

	va_start(argp, fmt);

	vsnprintf(boot_print_buf, sizeof(boot_print_buf), fmt, argp);
	nt_pbl_printf(boot_print_buf);

	va_end(argp);

	return;
}


