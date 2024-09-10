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

#include "ferm_flash.h"
#include "ferm_qspi.h"

#include "boot_error_if.h"
#include "nt_bl_uart.h"
#include "boot_elf_loader.h"
#include "rram_fdt.h"

#define UNUSED(x) (void)(x)

void sbl_printf(const char *fmt, ...);
void sbl_wait_jtag_enter();
void nt_uartInit(void);
bl_error_type boot_sbl_find_appimg(fdt_s *fdt, uint32_t *idx);
uint32_t get_bdf_addr(fdt_s *fdt);
void boot_sbl_sw_reset( void );
uint32_t sbl_get_xipstart_offset();
bl_error_type boot_sbl_flash_init();
bl_error_type boot_sbl_flash_xip_enable(uint32_t addr);
bl_error_type boot_sbl_flash_xip_disable();
bl_error_type boot_sbl_flash_read(uint32_t address, uint32_t byte_cnt, uint8_t *buffer);
bl_error_type boot_sbl_flash_write(uint32_t address, uint32_t byte_cnt, uint8_t *buffer);
bl_error_type boot_sbl_rram_write(uint32_t destination, uint8_t* source, uint32_t length);

#endif
