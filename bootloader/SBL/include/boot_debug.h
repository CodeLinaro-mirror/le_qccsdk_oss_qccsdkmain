/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/
#ifndef __BOOT_DEBUG_H__
#define __BOOT_DEBUG_H__

#ifdef	P_DEBUG 
#ifndef PBL_DEBUG_MODE
//#define	PBL_DEBUG_MODE 
#endif
#endif

#ifdef PBL_DEBUG_MODE
typedef void (*debug_handler)(char cmd);

typedef struct {
	char cmd;
	debug_handler handler;
} boot_debug_handle;

void boot_enter_debug_mode(void);
#endif

#endif
