/*
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

#define UNUSED(x) (void)(x)

void sbl_printf(const char *fmt, ...);
void sbl_wait_jtag_enter();
void nt_uartInit(void);
bl_error_type boot_sbl_flash_init();
bl_error_type boot_sbl_flash_read(uint32_t address, uint32_t byte_cnt, uint8_t *buffer);
bl_error_type boot_sbl_rram_write(uint32_t destination, uint8_t* source, uint32_t length);

#endif
