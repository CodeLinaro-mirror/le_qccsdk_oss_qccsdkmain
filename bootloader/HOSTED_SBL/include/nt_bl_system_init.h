/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef NEUTRINO_PBL_SYSTEM_NT_BL_SYSTEM_INIT_H_
#define NEUTRINO_PBL_SYSTEM_NT_BL_SYSTEM_INIT_H_

#include <stdint.h>
#include "nt_bl_common.h"
#include "nt_hw.h"
#include "nt_bl_uart.h"
#include "fwconfig_cmn.h"
#include "nt_flags.h"


# define QWLAN_PMU_BOOT_STRAP_CONFIGURATION_STATUS_NPS_CNTL_OFFSET		0x4

#define NPS_CNTL_BITS(bit)	( 1 << (QWLAN_PMU_BOOT_STRAP_CONFIGURATION_STATUS_NPS_CNTL_OFFSET + bit))

typedef enum{
	NOPULL=0,
	PULLUP,
	PULLDOWN,
	CFG_ERR
}PUPD_STATUS_TYPE;

typedef enum{
	RESET=0,
	SET
}BIT_STATUS_TYPE;

typedef enum{

	IOPAD_GPIO_0 = 0,
	IOPAD_GPIO_1,
	IOPAD_GPIO_2,
	IOPAD_GPIO_3,
	IOPAD_GPIO_4,
	IOPAD_GPIO_5,
	IOPAD_GPIO_6,
	IOPAD_GPIO_7,
	IOPAD_GPIO_8,
	IOPAD_GPIO_9,
	IOPAD_GPIO_10,
	IOPAD_GPIO_11,
	IOPAD_GPIO_12,
	IOPAD_GPIO_13
}IOPAD_GPIO;

void nt_bl_set_priority(uint32_t num_int);
void nt_bl_disable_irq(uint32_t num_int);
void nt_interrupt_disable( void );
void nt_bl_system_init(void);
void nt_bl_vector_init(void);

void nt_interrupt_init( void );
void nt_vtor_mapping( void );

void nt_systemtick_config( void );
void nt_systick_handler_ind( void );
void nt_systick_handler( void );
uint32_t nt_getsystick( void );
void nt_msdelay( uint32_t dlytick );
void nt_nop_delay( uint32_t n );

void nt_bbpll_check( void );
void nt_bpll_status( void );
int8_t nt_pbl_poweron_domains(uint32_t domains);
int8_t nt_pbl_default_poweron_domain(void);
int8_t nt_poweron_secip( void );

BIT_STATUS_TYPE nt_chk_bit_status_reg(uint32_t reg_address,uint32_t bit_mask);
void nt_clr_mask_reg(uint32_t reg_address,uint32_t bit_mask);
void nt_set_mask_reg(uint32_t reg_address, uint32_t bit_mask);

PUPD_STATUS_TYPE nt_chk_iopad_stats(uint32_t reg_address_pu,
		uint32_t reg_address_pd,uint32_t bit_mask);
void nt_set_iopad_config( PUPD_STATUS_TYPE mode , uint32_t bit_mask);

void nt_bl_io_config_init(void);

/*patch_cleanup*/
void nt_bbpll_init( void );
void nt_config_pupd_sd_data_3_pu_disbl_n(void);
void nt_config_pupd_ext_wakeup_intr(void);
void nt_config_pupd_ext_op_6_pwr_supp_enabl(void);
void nt_config_pupd_ext_op6_por_out(void);
void nt_config_pupd_ext_op8_por_out(void);
void nt_config_pupd_ext_op6_por_in_n(void);
void nt_config_pupd_xpa_en(void);
void nt_config_pupd_wsi_clk(void);
void nt_config_pupd_wsi_data(void);
void nt_config_pupd_JTAG();
void nt_config_pupd_gpio(IOPAD_GPIO pin);


typedef void (*nt_bl_set_priority_t)(uint32_t num_int);
typedef void (*nt_bl_disable_irq_t)(uint32_t num_int);
typedef void (*nt_interrupt_disable_t)( void );
typedef void (*nt_bl_system_init_t)(void);
typedef void (*nt_bl_vector_init_t)(void);
typedef void (*nt_interrupt_init_t)( void );
typedef void (*nt_vtor_mapping_t)( void );
typedef void (*nt_systemtick_config_t)( void );
typedef void (*nt_systick_handler_ind_t)( void );
typedef uint32_t (*nt_getsystick_t)( void );
typedef void (*nt_msdelay_t)( uint32_t dlytick );
typedef void (*nt_nop_delay_t)( uint32_t n );
typedef void (*nt_bbpll_check_t)( void );
typedef void (*nt_bpll_status_t)( void );
typedef int8_t (*nt_pbl_poweron_domains_t)(uint32_t domains);
typedef int8_t (*nt_pbl_default_poweron_domain_t)(void);
typedef int8_t (*nt_poweron_secip_t)( void );
//typedef BIT_STATUS_TYPE (*nt_chk_bit_status_reg_t)(uint32_t reg_address,uint32_t bit_mask);
typedef void (*nt_clr_mask_reg_t)(uint32_t reg_address,uint32_t bit_mask);
typedef void (*nt_set_mask_reg_t)(uint32_t reg_address, uint32_t bit_mask);
//typedef PUPD_STATUS_TYPE (*nt_chk_iopad_stats_t)(uint32_t reg_address_pu,
//		uint32_t reg_address_pd,uint32_t bit_mask);
typedef void (*nt_set_iopad_config_t)( PUPD_STATUS_TYPE mode , uint32_t bit_mask);
//static functions
typedef void (*nt_bbpll_init_t)(void);
typedef void (*pupd_sd_data_3_pu_disbl_n_t)(void);
typedef void (*pupd_ext_wakeup_intr_t)(void);
typedef void (*pupd_ext_op_6_pwr_supp_enabl_t)(void);
typedef void (*pupd_ext_op6_por_out_t)(void);
typedef void (*pupd_ext_op8_por_out_t)(void);
typedef void (*pupd_ext_op6_por_in_n_t)(void);
typedef void (*pupd_xpa_en_t)(void);
typedef void (*pupd_wsi_clk_t)(void);
typedef void (*pupd_wsi_data_t)(void);
typedef void (*pupd_JTAG_t)(void);
typedef void (*pupd_gpio_t)(IOPAD_GPIO pin);

typedef struct {
    nt_bl_set_priority_t nt_bl_set_priority_pfn;
    nt_bl_disable_irq_t nt_bl_disable_irq_pfn;
    nt_interrupt_disable_t nt_interrupt_disable_pfn;
    nt_bl_system_init_t nt_bl_system_init_pfn;
	nt_bl_vector_init_t nt_bl_vector_init_pfn;
    nt_interrupt_init_t nt_interrupt_init_pfn;
    nt_vtor_mapping_t nt_vtor_mapping_pfn;
    nt_systemtick_config_t nt_systemtick_config_pfn;
    nt_systick_handler_ind_t nt_systick_handler_ind_pfn;
    nt_getsystick_t nt_getsystick_pfn;
    nt_msdelay_t nt_msdelay_pfn;
    nt_nop_delay_t nt_nop_delay_pfn;
} nt_bl_system_ind_t;
#endif /* NEUTRINO_PBL_SYSTEM_NT_BL_SYSTEM_INIT_H_ */
