/*
* * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
* * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _SBL_COMMON_H__
#define _SBL_COMMON_H__

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>


#include "boot_error_if.h"
#include "nt_bl_uart.h"
//#include "boot_elf_loader.h"
#include "rram_fdt.h"

#define UNUSED(x) (void)(x) 

#ifdef FLASH_XIP_SUPPORT
extern uint32_t __app_image_xip_start_addr;
#define APP_IMAGE_START_ADDRESS (uint32_t)&__app_image_xip_start_addr
#else
extern uint32_t __app_image_start_addr;
#define APP_IMAGE_START_ADDRESS (uint32_t)&__app_image_start_addr
extern uint32_t __app_image_bdf_addr;
//#define APP_IMAGE_BDF_ADDRESS (uint32_t)&__app_image_bdf_addr
extern uint32_t __raw_file_start_addr;
#define RAW_FILE_START_ADDRESS (uint32_t)&__raw_file_start_addr
#endif

void sbl_printf(const char *fmt, ...);
void sbl_wait_jtag_enter();
void nt_uartInit(void);
bl_error_type boot_sbl_find_appimg(fdt_s *fdt, uint32_t *idx);
uint32_t get_bdf_addr(fdt_s *fdt);
void boot_sbl_sw_reset( void );
void sbl_sw_reset( void );

bl_error_type boot_sbl_rram_write(uint32_t destination, uint8_t* source, uint32_t length);

#endif
