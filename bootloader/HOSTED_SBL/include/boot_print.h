/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#ifndef __BOOT_PBL_PRINT_H__
#define __BOOT_PBL_PRINT_H__
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include "boot_module_id.h"

#define BOOT_PRINT_BUF_SIZE 120

#ifdef P_DEBUG
#define P_DEBUG_PRINT
#endif

#ifdef P_DEBUG_PRINT
#define P_DEBUG_PRINT_FUNC_LINE
/* When PBL share function to SBL, we need to disable the ELF LOADER, ELF MEM print 
 * in case the print would corrupt the loaded RAM segmment of APP */
//#define P_DEBUG_PRINT_ELF_LOADER
//#define P_DEBUG_PRINT_ELF_MEM

//#define P_DEBUG_PRINT_BOOT_HANDLER
//#define P_DEBUG_PRINT_FDT
//#define P_DEBUG_PRINT_SBL_OTA
//#define P_DEBUG_PRINT_RRAM_DXE
//#define P_DEBUG_PRINT_DEBUG_MODE

#define P_DEBUG_PRINT_VECTOR
#define P_DEBUG_PRINT_SYSTEM
#define P_DEBUG_PRINT_LOG
#define P_DEBUG_PRINT_SECBOOT


void boot_print(
		boot_mod_id id,
#ifdef P_DEBUG_PRINT_FUNC_LINE
		char *func_name,
		uint32_t line,
#endif
		const char *fmt,
		uint32_t num,
		...);

#define     NUMARGS(...)  (sizeof((uint32_t[]){0, ##__VA_ARGS__}) / sizeof(int) - 1)

#endif //P_DEBUG_PRINT

#ifdef P_DEBUG_PRINT_FUNC_LINE
#define BOOT_PRINT(MOD_ID, FMT, ...) \
    boot_print(MOD_ID, (char*)__func__, __LINE__, FMT, NUMARGS(__VA_ARGS__), ##__VA_ARGS__)
#else
#define BOOT_PRINT(MOD_ID, FMT,...) \
    boot_print(MOD_ID, FMT, NUMARGS(__VA_ARGS__), ##__VA_ARGS__)
#endif

#ifdef P_DEBUG_PRINT_SYSTEM
#define SYSTEM_PRINTF(...)	BOOT_PRINT(PBL_SYSTEM, __VA_ARGS__);
#else
#define SYSTEM_PRINTF(...)
#endif

#ifdef P_DEBUG_PRINT_ELF_LOADER
#define ELF_LOADER_PRINTF(...)	BOOT_PRINT(PBL_ELF_LOADER, __VA_ARGS__);
#else
#define ELF_LOADER_PRINTF(...)
#endif

#ifdef P_DEBUG_PRINT_ELF_MEM
#define ELF_MEM_PRINTF(...)		BOOT_PRINT(PBL_ELF_MEM, __VA_ARGS__);
#else
#define ELF_MEM_PRINTF(...)
#endif

#ifdef P_DEBUG_PRINT_BOOT_HANDLER
#define BOOT_HANDLER_PRINTF(...)	BOOT_PRINT(PBL_BOOT_HANDLER, __VA_ARGS__);
#else
#define BOOT_HANDLER_PRINTF(...)
#endif

#ifdef P_DEBUG_PRINT_FDT
#define FDT_PRINTF(...)				BOOT_PRINT(PBL_FDT, __VA_ARGS__);
#else
#define FDT_PRINTF(...)	
#endif

#ifdef P_DEBUG_PRINT_SBL_OTA
#define SBL_OTA_PRINTF(...)			BOOT_PRINT(PBL_OTA_SBL, __VA_ARGS__);
#else
#define SBL_OTA_PRINTF(...)
#endif

#ifdef P_DEBUG_PRINT_RRAM_DXE
#define RRAM_DXE_PRINTF(...)		BOOT_PRINT(PBL_RRAM_DXE, __VA_ARGS__);
#else
#define RRAM_DXE_PRINTF(...)
#endif

#ifdef P_DEBUG_PRINT_DEBUG_MODE
#define DEBUG_MODE_PRINTF(...)		BOOT_PRINT(PBL_DBG_MODE, __VA_ARGS__);
#else
#define DEBUG_MODE_PRINTF(...)
#endif

#ifdef P_DEBUG_PRINT_LOG
#define LOG_PRINTF(...)				BOOT_PRINT(PBL_LOG, __VA_ARGS__);
#else
#define LOG_PRINTF(...)
#endif

#ifdef P_DEBUG_PRINT_SECBOOT
#define SECBOOT_PRINT(...)		BOOT_PRINT(PBL_SECBOOT, __VA_ARGS__);
#else
#define SECBOOT_PRINT(...)
#endif

#ifdef P_DEBUG_PRINT_VECTOR
#define VECTOR_PRINTF(...)		BOOT_PRINT(PBL_VECTOR, __VA_ARGS__);
#else
#define VECTOR_PRINTF(...)
#endif

#endif //__BOOT_PBL_PRINT_H__
