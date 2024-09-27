/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef NT_BL_WDT_H_
#define NT_BL_WDT_H_

void nt_wdt_enable( void );
void nt_wdt_disable( void );
void nt_wdt_int( void );
void nt_bl_wdt_reset_ind( void );
void nt_bl_wdt_reset( void );

#if defined(SUPPORT_PBL_PATCH)
typedef void (*nt_wdt_enable_t)( void );
typedef void (*nt_wdt_disable_t)( void );
typedef void (*nt_wdt_int_t)( void );
typedef void (*nt_bl_wdt_reset_ind_t)( void );

typedef struct nt_bl_wdt_s{
    nt_wdt_enable_t nt_wdt_enable_pfn;
    nt_wdt_disable_t nt_wdt_disable_pfn;
    nt_wdt_int_t nt_wdt_int_pfn;
    nt_bl_wdt_reset_ind_t nt_bl_wdt_reset_ind_pfn;
}nt_bl_wdt_t;
#endif /*SUPPORT_PBL_PATCH*/
#endif /* NT_BL_WDT_H_ */
