/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#ifndef __BOOT_HANDLER_H__
#define __BOOT_HANDLER_H__
#include <stdint.h>
#include "boot_error_if.h"


#define FRN_COLD_BOOT_MASK ( QWLAN_PMU_SYSTEM_STATUS_SYS_RESET_BY_P6V_POK_TOGGLE_MASK | QWLAN_PMU_SYSTEM_STATUS_SYS_RESET_BY_VBATT_BROWN_OUT_MASK | \
							 QWLAN_PMU_SYSTEM_STATUS_SYS_RESET_BY_SW_HOST_MASK )

#define FRN_WARM_BOOT_MASK ( QWLAN_PMU_SYSTEM_STATUS_WMAC_WAKE_CPU_RISE_P_REG_MASK | QWLAN_PMU_SYSTEM_STATUS_WUR_WAKE_MAIN_RADIO_RISE_P_REG_MASK | \
		QWLAN_PMU_SYSTEM_STATUS_WUR_WAKE_CPU_RISE_P_REG_MASK | QWLAN_PMU_SYSTEM_STATUS_AON_WDOG_EXPIRED_MASK | QWLAN_PMU_SYSTEM_STATUS_WARM_BOOT_FROM_DEEPSLEEP_MASK | \
		QWLAN_PMU_SYSTEM_STATUS_WARM_BOOT_FROM_INDEFINITE_DEEPSLEEP_MASK | QWLAN_PMU_SYSTEM_STATUS_WARM_BOOT_FROM_SLEEP_MASK)

typedef enum
{
	BOOT_SLEEP_WAKE = 0,
	BOOT_DEEP_SLEEP_WAKE,
	BOOT_INDEFINATE_SLEEP_WAKE,
	BOOT_WUR_CPU_WAKE,
	BOOT_WUR_RADIO_WAKE,
	BOOT_WMAC_CPU_WAKE,
	BOOT_WDOG_EXPIRED,
	BOOT_WARM_UNKNOWN,
	BOOT_SW_RST,
	BOOT_P6V_TOGGLE_RST,
	BOOT_BROWNOUT_RST,
	BOOT_FRESH_POWER_ON,
	BOOT_COLD_UNKNOWN
} boot_subtype;

typedef enum
{
	BOOT_WARM,
	BOOT_COLD,
} boot_type;

typedef struct {
	boot_type type;
	boot_subtype subtype;
} boot_reason;

typedef bl_error_type (*boot_subtype_process)(boot_reason *reason);

typedef struct {
	boot_subtype type;
	boot_subtype_process handler;
} boot_subtype_handler;

typedef struct {
	boot_reason reason;
	boot_subtype_handler *subtype_handle;
	uint32_t subtype_hdr_num;
} boot_handler;

extern boot_handler pbl_boot_handler; 
extern boot_subtype_handler subtype_handlers[];
extern uint32_t subtype_handlers_num;

bl_error_type boot_handler_reason_check(boot_reason *reason);
bl_error_type boot_handler_init(boot_handler *handler);
bl_error_type boot_handler_subtype_process(boot_handler *handler);
bl_error_type boot_handler_process(boot_handler *handler);

bl_error_type boot_handler_fresh_power_on(boot_reason *reason);
bl_error_type boot_handler_software_reset(boot_reason *reason);
bl_error_type boot_handler_cold_boot_unknown(boot_reason *reason);
bl_error_type boot_handler_cold_subtype_default(boot_reason *reason);

bl_error_type boot_handler_wdog_expired(boot_reason *reason);
bl_error_type boot_handler_indefinate_sleep(boot_reason *reason);
bl_error_type boot_handler_deep_sleep(boot_reason *reason);
bl_error_type boot_handler_warm_boot_unknown(boot_reason *reason);
bl_error_type boot_handler_warm_subtype_default(boot_reason *reason);

typedef bl_error_type (*boot_handler_reason_check_t)(boot_reason *reason);
typedef bl_error_type (*boot_handler_init_t)(boot_handler *handler);
typedef bl_error_type (*boot_handler_subtype_process_t)(boot_handler *handler);
typedef bl_error_type (*boot_handler_process_t)(boot_handler *handler);

typedef bl_error_type (*boot_handler_fresh_power_on_t)(boot_reason *reason);
typedef bl_error_type (*boot_handler_software_reset_t)(boot_reason *reason);
typedef bl_error_type (*boot_handler_cold_boot_unknown_t)(boot_reason *reason);
typedef bl_error_type (*boot_handler_cold_subtype_default_t)(boot_reason *reason);

typedef bl_error_type (*boot_handler_wdog_expired_t)(boot_reason *reason);
typedef bl_error_type (*boot_handler_indefinate_sleep_t)(boot_reason *reason);
typedef bl_error_type (*boot_handler_deep_sleep_t)(boot_reason *reason);
typedef bl_error_type (*boot_handler_warm_boot_unknown_t)(boot_reason *reason);
typedef bl_error_type (*boot_handler_warm_subtype_default_t)(boot_reason *reason);

typedef struct {
	boot_handler_reason_check_t			boot_handler_reason_check_pfn;
	boot_handler_init_t					boot_handler_init_pfn;
	boot_handler_subtype_process_t		boot_handler_subtype_process_pfn;
	boot_handler_process_t				boot_handler_process_pfn;
	boot_handler_fresh_power_on_t		boot_handler_fresh_power_on_pfn;
	boot_handler_software_reset_t		boot_handler_software_reset_pfn;
	boot_handler_cold_boot_unknown_t	boot_handler_cold_boot_unknown_pfn;
	boot_handler_cold_subtype_default_t	boot_handler_cold_subtype_default_pfn;
	boot_handler_wdog_expired_t			boot_handler_wdog_expired_pfn;
	boot_handler_indefinate_sleep_t		boot_handler_indefinate_sleep_pfn;
	boot_handler_deep_sleep_t			boot_handler_deep_sleep_pfn;
	boot_handler_warm_boot_unknown_t	boot_handler_warm_boot_unknown_pfn;
	boot_handler_warm_subtype_default_t	boot_handler_warm_subtype_default_pfn;
} boot_handler_ind_t;

#endif //__BOOT_HANDLER_H__

