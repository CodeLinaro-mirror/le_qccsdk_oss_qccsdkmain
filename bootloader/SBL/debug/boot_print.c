/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "boot_module_id.h"
#include "boot_print.h"
#include "nt_bl_uart.h"

static char boot_print_buf[BOOT_PRINT_BUF_SIZE];

#ifdef P_DEBUG
#ifdef P_DEBUG_PRINT

static const char *boot_mod_s[PBL_MOD_MAX] = {
	"OTA SBL",
	"FDT",
	"BOOT_HANDLER",
	"ELF_LOADER",
	"ELF MEM",
	"AUTH",
	"SYSTEM",
	"RRAM DXE",
	"DEBUG MODE",
	"LOG",
	"VECTOR",
};

void boot_print(
		boot_mod_id id,
#ifdef P_DEBUG_PRINT_FUNC_LINE
		char *func_name,
		uint32_t line,
#endif
		const char *fmt,
		uint32_t num,
		...)
{
	va_list argp;

    snprintf(boot_print_buf,sizeof(boot_print_buf), "PBL " "[%s]:"
#ifdef P_DEBUG_PRINT_FUNC_LINE
					    		"(%s:%lu) "
#endif
								,boot_mod_s[id]
#ifdef P_DEBUG_PRINT_FUNC_LINE
								,func_name
								,(uint32_t)line
#endif
												);
    nt_pbl_printf(boot_print_buf);

	va_start(argp, num);

	vsnprintf(boot_print_buf, sizeof(boot_print_buf), fmt, argp);
	nt_pbl_printf(boot_print_buf);

	va_end(argp);

	return;
}

#endif //P_DEBUG_PRINT
#endif //P_DEBUG
void sbl_printf(const char *fmt, ...)
{
	va_list argp;

	memset((uint8_t *)(boot_print_buf), 0, sizeof(boot_print_buf));
    snprintf(boot_print_buf,5, "SBL ");
    nt_pbl_printf(boot_print_buf);

	va_start(argp, fmt);

	vsnprintf(boot_print_buf, sizeof(boot_print_buf), fmt, argp);
	nt_pbl_printf(boot_print_buf);

	va_end(argp);

	return;
}
