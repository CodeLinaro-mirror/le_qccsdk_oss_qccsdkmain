/*
 * This file is part of the ÂµOS++ distribution.
 *   (https://github.com/micro-os-plus)
 * Copyright (c) 2014 Liviu Ionescu.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

// ----------------------------------------------------------------------------

#include "cortexm/ExceptionHandlers.h"
#include "FreeRTOSConfig.h"
#include "uart.h"
#include "dxe_api.h"
#include "nt_hw.h"
#include "ferm_prof.h"

#include "nt_cc_battery_driver.h"
#define HAL_REG_WR(reg_addr, data) (*((volatile uint32_t *) (reg_addr))) = ((uint32_t) (data))
#define HAL_REG_RD(reg_addr)       (*((volatile uint32_t *) (reg_addr)))
// ----------------------------------------------------------------------------

void __attribute__((weak))
Default_Handler(void);

// Forward declaration of the specific IRQ handlers. These are aliased
// to the Default_Handler, which is a 'forever' loop. When the application
// defines a handler (with the same name), this will automatically take
// precedence over these weak definitions
//
// TODO: Rename this and add the actual routines here.

void __attribute__ ((weak, alias ("Default_Handler")))
DeviceInterrupt_Handler(void);
void ahb_pd_error(void);

void CTI_INTISR_0(void);

void CTI_INTISR_1(void);
void ccu_qgic2_apps_wlan_rx_data_avail(void);
void ccpu_ccu_ahb_err_int_p(void);
void rri_ccu_intr3(void);
void ccu_qgic2_apss_asic_intr(void);
void o_xpu2_non_secure_intr(void);
void cc_intr(void);
/**
 * <!-- pmu_ccpu_vbatt_low_hit_int_p -->
 *
 * @brief      : ISR for low voltage scenario
 * @return     : void
*/
void pmu_ccpu_vbat_low_hit_int_p(void);
/**
 * <!-- pmu_ccpu_temp_panic_hit_int -->
 *
 * @brief      : ISR for high temperature scenario
 * @return     : void
 */
void pmu_ccpu_temp_panic_hit_int(void);
// void wur_cpu_int(void);
void o_xpu2_secure_intr(void);
void cmem_banka_ccpu_auto_pw(void);
void cmem_bankb_ccpu_auto_pw(void);
void cmem_bankc_ccpu_auto_pw(void);
void cmem_bankd_ccpu_auto_pw(void);
void cmem_cmn_ccpu_auto_pw(void);
void ccu_qgic2_apps_wlan_data_xfer_done(void);
void cmem_int(void);
void ccpu_ccu_cmem_timeout_int_p(void);
void rram_interrupt_handler(void);
void kdf_m4f_intr(void);
void ecc_core_m4f_intr(void);
void ccpu_ccu_fp_exception_0(void);
void ccpu_ccu_fp_exception_1(void);
void ccpu_ccu_fp_exception_2(void);
void ccpu_ccu_fp_exception_3(void);
void ccpu_ccu_fp_exception_4(void);
void ccpu_ccu_fp_exception_5(void);
void wlan_ccu_irq(void);
#if (NT_CHIP_VERSION==2)
void pmu_ccpu_slp_cal_done_intr(void);
void pmu_ccpu_temp_mon_done_intr(void);
void pmu_ccpu_vbat_mon_done_intr(void);
void pmu_ccpu_bbpll_lock_timeout_intr(void);
void pmu_ccpu_bbpll_lock_toggle_intr(void);
void pmu_ccpu_xip_pwr_down_early_warn_to_ok_to_intr(void);
void pmu_ccpu_g2p_pready_timeout_intr(void);
void pmu_ccpu_boot_strap_config_otp_read_err_intr(void);
void pmu_ccpu_rfa_pmic_otp_read_err_intr(void);
void pmu_ccpu_boot_strap_config_otp_read_to_intr(void);
void pmu_ccpu_rfa_pmic_otp_read_to_intr(void);
void pmu_ccpu_wlan_sleep_ack_timout_intr(void);
void pmu_ccpu_wlan_sleep_ack_err_intr(void);
void pmu_ccpu_wlan_coext_int_err_intr(void);
void pmu_ccpu_wlan_coext_int_done_timout_intr(void);
void aon_cmnss_dvs_done_int(void);
void aon_cmnss_pwfm_outoff_min_max_range_int(void);
void aon_cmnss_ulpm_outoff_min_max_range_int(void);
#endif //(NT_CHIP_VERSION==2)

// ----------------------------------------------------------------------------

extern unsigned int _estack;
//extern unsigned int _vStackTop;
extern void uart_irq_handler(void);
extern void UART_irq_handler(void);
#if defined(SUPPORT_HIGH_RES_TIMER)
#ifndef QTMR_DRV
extern void qtmr_0_irq_handler(void);
extern void qtmr_1_irq_handler(void);
extern void qtmr_2_irq_handler(void);
extern void qtmr_3_irq_handler(void);
extern void qtmr_4_irq_handler(void);
#else
extern void	qtmr_irq_handler_0(void);
extern void	qtmr_irq_handler_1(void);
extern void	qtmr_irq_handler_2(void);
extern void	qtmr_irq_handler_3(void);
extern void	qtmr_irq_handler_4(void);
#endif
#endif
extern void I2C_irq_handler(void);
extern void ext_irq_handler(void);
extern void ram_minimum_code(void);
//extern void gpio_irq_handler(void);
extern void xPortPendSVHandler( void ) __attribute__ (( naked ));
extern void xPortSysTickHandler( void );
extern void vPortSVCHandler( void ) __attribute__ (( naked ));
extern void nt_spi_slv_interrupt (void);
extern void nt_cpr_isr_handler(void);
extern void nt_gpio_interrupt_enable(void);
extern void GPIO_IntHandler(void);

#ifdef PLATFORM_FERMION
#ifdef SUPPORT_COEX
extern void coex_bmh_isr(void);
extern void coex_smh_isr(void);
extern void coex_pmh_isr(void);
extern void coex_lmh_isr(void);
extern void coex_mcim_isr(void);
#else
void coex_bmh_isr   (void) __attribute__ ((weak, alias("Default_Handler")));
void coex_smh_isr   (void) __attribute__ ((weak, alias("Default_Handler")));
void coex_pmh_isr   (void) __attribute__ ((weak, alias("Default_Handler")));
void coex_lmh_isr   (void) __attribute__ ((weak, alias("Default_Handler")));
void coex_mcim_isr  (void) __attribute__ ((weak, alias("Default_Handler")));
#endif // SUPPORT_COEX

#ifdef FIRMWARE_APPS_INFORMED_WAKE
extern void aon_a2f_assert_isr_handler(void);
extern void aon_a2f_deassert_isr_handler(void);
#endif // FIRMWARE_APPS_INFORMED_WAKE

#ifdef SLEEP_CLK_CAL_IN_ACTIVE_MODE
extern void pmu_ccpu_temp_mon_done_intr(void);
extern void pmu_ccpu_slp_cal_done_intr(void);
#else
void pmu_ccpu_temp_mon_done_intr(void) __attribute__((weak, alias("Default_Handler")));
void pmu_ccpu_slp_cal_done_intr(void) __attribute__((weak, alias("Default_Handler")));
#endif //SLEEP_CLK_CAL_IN_ACTIVE_MODE
#endif // PLATFORM_FERMION

extern void pmu_ccpu_vbat_mon_done_intr(void);


typedef void
(* const pHandler)(void);

// ----------------------------------------------------------------------------

// The vector table.
// This relies on the linker script to place at correct location in memory.

__attribute__ ((section(".flash_isr_vector"),used))
pHandler __isr_vectors[] =
  { //
    (pHandler) &_estack,                          // The initial stack pointer
        Reset_Handler,                            // The reset handler
        NMI_Handler,                              // The NMI handler
		vHardFault_Handler,                        // The hard fault handler
        MemManage_Handler,                        // The MPU fault handler
        BusFault_Handler,						// The bus fault handler
        UsageFault_Handler,						// The usage fault handler
        0,                                        // Reserved
        0,                                        // Reserved
        0,                                        // Reserved
        0,                                        // Reserved
		vPortSVCHandler,                              // SVCall handler
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
        DebugMon_Handler,                         // Debug monitor handler
#else
        0,					  // Reserved
#endif
        0,                                        	// Reserved
		xPortPendSVHandler,                       	// The PendSV handler
		xPortSysTickHandler,                      	// The SysTick handler
		CTI_INTISR_0,                  				// Device specific 0
		CTI_INTISR_1,         			         	// Device specific 1
		I2C_irq_handler,                        	// I2C Interrupt Handler
#if defined(UART_DRV)
		UART_irq_handler,
#else
		uart_irq_handler,                  			// UART Interrupt Handler
#endif
#if defined(SUPPORT_HIGH_RES_TIMER)
#ifndef QTMR_DRV
		qtmr_0_irq_handler,                  		// QTMR Frame 0 Interrupt Handler
		qtmr_1_irq_handler,                  		// QTMR Frame 1 Interrupt Handler
		qtmr_2_irq_handler,                  		// QTMR Frame 2 Interrupt Handler
		qtmr_3_irq_handler,                  		// QTMR Frame 3 Interrupt Handler
		qtmr_4_irq_handler,                  		// QTMR Frame 4 Interrupt Handler
#else
		qtmr_irq_handler_0,
		qtmr_irq_handler_1,
		qtmr_irq_handler_2,
		qtmr_irq_handler_3,
		qtmr_irq_handler_4,
#endif
#else
		DeviceInterrupt_Handler,                 	// Device specific 4
		DeviceInterrupt_Handler,                 	// Device specific 5
		DeviceInterrupt_Handler,                 	// Device specific 6
		DeviceInterrupt_Handler,                 	// Device specific 7
		DeviceInterrupt_Handler,                 	// Device specific 8
#endif
		DeviceInterrupt_Handler,                 	// Device specific 9
		DeviceInterrupt_Handler,                	// Device specific 10
		DeviceInterrupt_Handler,                	// Device specific 11
		DeviceInterrupt_Handler,                	// Device specific 12
		DeviceInterrupt_Handler,                	// Device specific 13
		DeviceInterrupt_Handler,                 	// Device specific 14
		DeviceInterrupt_Handler,                 	// Device specific 15
		DeviceInterrupt_Handler,                 	// Device specific 16
		DeviceInterrupt_Handler,                 	// Device specific 17
		ccu_qgic2_apps_wlan_data_xfer_done,        	// Device specific 18
		ccu_qgic2_apps_wlan_rx_data_avail,         			// Device specific 19
		cmem_int,                  					// Device specific 20
		nt_dxe_interrupt_handler,                 	// DXE Interrupt Handler
		nt_dxe_interrupt_handler,                 	// DXE Interrupt Handler
		nt_dxe_interrupt_handler,                 	// DXE Interrupt Handler
		nt_dxe_interrupt_handler,                 	// DXE Interrupt Handler
		nt_dxe_interrupt_handler,                 	// DXE Interrupt Handler
		nt_dxe_interrupt_handler,                 	// DXE Interrupt Handler
		nt_dxe_interrupt_handler,                 	// DXE Interrupt Handler
		nt_dxe_interrupt_handler,                 	// DXE Interrupt Handler
		nt_dxe_interrupt_handler,                 	// DXE Interrupt Handler
		nt_dxe_interrupt_handler,                 	// DXE Interrupt Handler
		nt_dxe_interrupt_handler,                 	// DXE Interrupt Handler
		nt_dxe_interrupt_handler,                 	// DXE Interrupt Handler
		wlan_ccu_irq,     			             	// Device specific 33
		wlan_ccu_fiq_handler,                  		// WLAN CCU FIQ Interrupt Handler
		ccpu_ccu_ahb_err_int_p,                 	// Device specific 35
		ccpu_ccu_cmem_timeout_int_p,                // Device specific 36
		DeviceInterrupt_Handler,               		// Device specific 37
		pmu_ccpu_vbat_low_hit_int_p,                 	// Device specific 38
		pmu_ccpu_temp_panic_hit_int,               		// Device specific 39
		DeviceInterrupt_Handler,                  	// Device specific 40
		ext_irq_handler,                  	        // external interrupt handler
		DeviceInterrupt_Handler,                  	// Device specific 42
		rri_ccu_intr3,                  			// Device specific 43
		ccu_qgic2_apss_asic_intr,                 	// Device specific 44
		rram_interrupt_handler,                  	// Device specific 45
		DeviceInterrupt_Handler,                  	// Device specific 46
		DeviceInterrupt_Handler,                  	// Device specific 47
		GPIO_IntHandler,                            // GPIO interrupt 48
		o_xpu2_non_secure_intr,                 	// Device specific 49
		kdf_m4f_intr,                  				// Device specific 50
		ecc_core_m4f_intr,                 			// Device specific 51
		cc_intr, 				                 	// Device specific 52
		wur_cpu_int,                  				// WUR Interrupt Handler
		nt_wdt_int_wcss_wdog_bark ,                 // wcss_wdog_bark Interrupt Handler
		Aon_cmnss_wlan_slp_tmr_int,                	// aon_cmnss_wlan_slp_tmr_int
		ccpu_ccu_fp_exception_0,          			// Device specific 56
		ccpu_ccu_fp_exception_1,          			// Device specific 57
		ccpu_ccu_fp_exception_2,          			// Device specific 58
		ccpu_ccu_fp_exception_3,          			// Device specific 59
		ccpu_ccu_fp_exception_4,          			// Device specific 60
		ccpu_ccu_fp_exception_5,          			// Device specific 61
		DeviceInterrupt_Handler,          			// Device specific 62
#ifdef PLATFORM_FERMION
#ifdef FIRMWARE_APPS_INFORMED_WAKE
		aon_a2f_assert_isr_handler,          		    // Device specific 63
#else
		DeviceInterrupt_Handler,          			// Device specific 63
#endif // FIRMWARE_APPS_INFORMED_WAKE
#else
		aon_ext_interrupt_wake_up,          		// Device specific 63
#endif // PLATFORM_FERMION
		o_xpu2_secure_intr,		          			// Device specific 64
#if (NT_CHIP_VERSION==2)
		pmu_ccpu_slp_cal_done_intr,          		// Device specific 65
#else
#ifdef PLATFORM_FERMION
		pmu_ccpu_slp_cal_done_intr,          		// Device specific 65
#else
		DeviceInterrupt_Handler,					// Device specific 65
#endif /* PLATFORM_FERMION */
#endif // (NT_CHIP_VERSION==2)
		cmem_banka_ccpu_auto_pw,          			// Device specific 66
		cmem_bankb_ccpu_auto_pw,          			// Device specific 67
		cmem_bankc_ccpu_auto_pw,          			// Device specific 68
		cmem_bankd_ccpu_auto_pw,          			// Device specific 69
		cmem_cmn_ccpu_auto_pw,          			// Device specific 70
		nt_cpr_isr_handler,          			    // cpr_handler 71
		nt_spi_slv_interrupt ,          			// spi slave 72
		ahb_pd_error,          						// Device specific 73
#if (NT_CHIP_VERSION==2)
		DeviceInterrupt_Handler,                 	// Device specific 74
		aon_cmnss_dvs_done_int,                 	// Device specific 75
		aon_cmnss_pwfm_outoff_min_max_range_int,    // Device specific 76
		aon_cmnss_ulpm_outoff_min_max_range_int,    // Device specific 77
		pmu_ccpu_bbpll_lock_toggle_intr,            // Device specific 78
		pmu_ccpu_temp_mon_done_intr,                // Device specific 79
		pmu_ccpu_vbat_mon_done_intr,                // Device specific 80
		pmu_ccpu_xip_pwr_down_early_warn_to_ok_to_intr,  // Device specific 81
		DeviceInterrupt_Handler,                	// Device specific 82
		DeviceInterrupt_Handler,                 	// Device specific 83
		DeviceInterrupt_Handler,                 	// Device specific 84
		DeviceInterrupt_Handler,                 	// Device specific 85
		DeviceInterrupt_Handler,                 	// Device specific 86
		DeviceInterrupt_Handler,                 	// Device specific 87
		DeviceInterrupt_Handler,                 	// Device specific 88
		DeviceInterrupt_Handler,                	// Device specific 89
		pmu_ccpu_g2p_pready_timeout_intr,                	// Device specific 90
		pmu_ccpu_boot_strap_config_otp_read_err_intr,       // Device specific 91
		pmu_ccpu_rfa_pmic_otp_read_err_intr,                // Device specific 92
		pmu_ccpu_wlan_sleep_ack_timout_intr,                	// Device specific 93
		pmu_ccpu_wlan_sleep_ack_err_intr,                	// Device specific 94
		pmu_ccpu_wlan_coext_int_err_intr,                	// Device specific 95
		pmu_ccpu_wlan_coext_int_done_timout_intr,                	// Device specific 96
		pmu_ccpu_bbpll_lock_timeout_intr,                	// Device specific 97
		pmu_ccpu_boot_strap_config_otp_read_to_intr,                	// Device specific 98
		pmu_ccpu_rfa_pmic_otp_read_to_intr,                	// Device specific 99
		DeviceInterrupt_Handler,                	// Device specific 100
#else
#ifdef PLATFORM_FERMION
		DeviceInterrupt_Handler,                 	// Device specific 74
		DeviceInterrupt_Handler,                 	// Device specific 75
		DeviceInterrupt_Handler,    // Device specific 76
		DeviceInterrupt_Handler,    // Device specific 77
		DeviceInterrupt_Handler,            // Device specific 78
		pmu_ccpu_temp_mon_done_intr,                // Device specific 79
		pmu_ccpu_vbat_mon_done_intr,                // Device specific 80
		DeviceInterrupt_Handler,  				// Device specific 81
		DeviceInterrupt_Handler,                	// Device specific 82
		coex_bmh_isr,                 	// Device specific 83
		coex_smh_isr,                 	// Device specific 84
		coex_pmh_isr,                 	// Device specific 85
		coex_mcim_isr,                 	// Device specific 86
		coex_lmh_isr,                 	// Device specific 87
		DeviceInterrupt_Handler,                	// Device specific 88
		DeviceInterrupt_Handler,                 	// Device specific 89
		DeviceInterrupt_Handler,                 	// Device specific 90
		DeviceInterrupt_Handler,                 	// Device specific 91
		DeviceInterrupt_Handler,                 	// Device specific 92
		DeviceInterrupt_Handler,                 	// Device specific 93
		DeviceInterrupt_Handler,                 	// Device specific 94
		DeviceInterrupt_Handler,                	// Device specific 95
		DeviceInterrupt_Handler,                 	// Device specific 96
		DeviceInterrupt_Handler,                 	// Device specific 97
		DeviceInterrupt_Handler,                 	// Device specific 98
		DeviceInterrupt_Handler,                	// Device specific 99
		DeviceInterrupt_Handler,                	// Device specific 100
		DeviceInterrupt_Handler,                	// Device specific 101
		DeviceInterrupt_Handler,                	// Device specific 102
		DeviceInterrupt_Handler,                	// Device specific 103
		DeviceInterrupt_Handler,                	// Device specific 104
		DeviceInterrupt_Handler,                	// Device specific 105
		DeviceInterrupt_Handler,                	// Device specific 106
		DeviceInterrupt_Handler,                	// Device specific 107
#ifdef FIRMWARE_APPS_INFORMED_WAKE
		aon_a2f_deassert_isr_handler,  	// Device specific 108
#else
		DeviceInterrupt_Handler,                	// Device specific 108
#endif // FIRMWARE_APPS_INFORMED_WAKE
#endif // PLATFORM_FERMION
#endif //(NT_CHIP_VERSION==2)

	};
    // ----------------------------------------------------------------------------
__attribute__ ((section(".ram_isr_vector"),used))
pHandler __stack_ptr[] =
{ //
  (pHandler) &_estack,                          // The initial stack pointer
      ram_minimum_code,                            // The reset handler
      NMI_Handler,                              // The NMI handler
	  vHardFault_Handler,                        // The hard fault handler
      MemManage_Handler,                        // The MPU fault handler
      BusFault_Handler,							// The bus fault handler
      UsageFault_Handler,						// The usage fault handler
      0,                                        // Reserved
      0,                                        // Reserved
      0,                                        // Reserved
      0,                                        // Reserved
		vPortSVCHandler,                              // SVCall handler
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
      DebugMon_Handler,                         // Debug monitor handler
#else
      0,					  // Reserved
#endif
      0,                                        	// Reserved
		xPortPendSVHandler,                       	// The PendSV handler
		xPortSysTickHandler,                      	// The SysTick handler
		CTI_INTISR_0,                  				// Device specific 0
		CTI_INTISR_1,			                  	// Device specific 1
		I2C_irq_handler,                        	// I2C Interrupt Handler
#if defined(UART_DRV)
		UART_irq_handler,
#else
		uart_irq_handler,                  			// UART Interrupt Handler
#endif
#if defined(SUPPORT_HIGH_RES_TIMER)
#ifndef QTMR_DRV
		qtmr_0_irq_handler,                  		// QTMR Frame 0 Interrupt Handler
		qtmr_1_irq_handler,                  		// QTMR Frame 1 Interrupt Handler
		qtmr_2_irq_handler,                  		// QTMR Frame 2 Interrupt Handler
		qtmr_3_irq_handler,                  		// QTMR Frame 3 Interrupt Handler
		qtmr_4_irq_handler,                  		// QTMR Frame 4 Interrupt Handler
#else
		qtmr_irq_handler_0,
		qtmr_irq_handler_1,
		qtmr_irq_handler_2,
		qtmr_irq_handler_3,
		qtmr_irq_handler_4,
#endif
#else
		DeviceInterrupt_Handler,                 	// Device specific 4
		DeviceInterrupt_Handler,                 	// Device specific 5
		DeviceInterrupt_Handler,                 	// Device specific 6
		DeviceInterrupt_Handler,                 	// Device specific 7
		DeviceInterrupt_Handler,                 	// Device specific 8
#endif
		DeviceInterrupt_Handler,                 	// Device specific 9
		DeviceInterrupt_Handler,                	// Device specific 10
		DeviceInterrupt_Handler,                	// Device specific 11
		DeviceInterrupt_Handler,                	// Device specific 12
		DeviceInterrupt_Handler,                	// Device specific 13
		DeviceInterrupt_Handler,                 	// Device specific 14
		DeviceInterrupt_Handler,                 	// Device specific 15
		DeviceInterrupt_Handler,                 	// Device specific 16
		DeviceInterrupt_Handler,                 	// Device specific 17
		ccu_qgic2_apps_wlan_data_xfer_done,        	// Device specific 18
		ccu_qgic2_apps_wlan_rx_data_avail,         			// Device specific 19
		cmem_int,                  					// Device specific 20
		nt_dxe_interrupt_handler,                 	// DXE Interrupt Handler
		nt_dxe_interrupt_handler,                 	// DXE Interrupt Handler
		nt_dxe_interrupt_handler,                 	// DXE Interrupt Handler
		nt_dxe_interrupt_handler,                 	// DXE Interrupt Handler
		nt_dxe_interrupt_handler,                 	// DXE Interrupt Handler
		nt_dxe_interrupt_handler,                 	// DXE Interrupt Handler
		nt_dxe_interrupt_handler,                 	// DXE Interrupt Handler
		nt_dxe_interrupt_handler,                 	// DXE Interrupt Handler
		nt_dxe_interrupt_handler,                 	// DXE Interrupt Handler
		nt_dxe_interrupt_handler,                 	// DXE Interrupt Handler
		nt_dxe_interrupt_handler,                 	// DXE Interrupt Handler
		nt_dxe_interrupt_handler,                 	// DXE Interrupt Handler
		wlan_ccu_irq,                  				// Device specific 33
		wlan_ccu_fiq_handler,                  		// WLAN CCU FIQ Interrupt Handler
		ccpu_ccu_ahb_err_int_p,                 	// Device specific 35
		ccpu_ccu_cmem_timeout_int_p,                // Device specific 36
		DeviceInterrupt_Handler,               		// Device specific 37
		pmu_ccpu_vbat_low_hit_int_p,                 	// Device specific 38
		pmu_ccpu_temp_panic_hit_int,               		// Device specific 39
		DeviceInterrupt_Handler,                  	// Device specific 40
		ext_irq_handler,                  	        // external interrupt handler
		DeviceInterrupt_Handler,                  	// Device specific 42
		rri_ccu_intr3,                  			// Device specific 43
		ccu_qgic2_apss_asic_intr,                 	// Device specific 44
		rram_interrupt_handler,                  	// Device specific 45
		DeviceInterrupt_Handler,                  	// Device specific 46
		DeviceInterrupt_Handler,                  	// Device specific 47
		GPIO_IntHandler,                            // GPIO interrupt 48
		o_xpu2_non_secure_intr,                 	// Device specific 49
		kdf_m4f_intr,                  				// Device specific 50
		ecc_core_m4f_intr,                 			// Device specific 51
		cc_intr,     				             	// Device specific 52
		wur_cpu_int,                  				// WUR Interrupt Handler
		nt_wdt_int_wcss_wdog_bark ,                 // wcss_wdog_bark Interrupt Handler
		Aon_cmnss_wlan_slp_tmr_int,                	// aon_cmnss_wlan_slp_tmr_int
		ccpu_ccu_fp_exception_0,          			// Device specific 56
		ccpu_ccu_fp_exception_1,          			// Device specific 57
		ccpu_ccu_fp_exception_2,          			// Device specific 58
		ccpu_ccu_fp_exception_3,          			// Device specific 59
		ccpu_ccu_fp_exception_4,          			// Device specific 60
		ccpu_ccu_fp_exception_5,          			// Device specific 61
		DeviceInterrupt_Handler,          			// Device specific 62
#ifdef PLATFORM_FERMION
#ifdef FIRMWARE_APPS_INFORMED_WAKE
		aon_a2f_assert_isr_handler,          		    // Device specific 63
#else
		DeviceInterrupt_Handler,          			// Device specific 63
#endif // FIRMWARE_APPS_INFORMED_WAKE
#else
		aon_ext_interrupt_wake_up,          		// Device specific 63
#endif // PLATFORM_FERMION
		o_xpu2_secure_intr,      	    			// Device specific 64
#if (NT_CHIP_VERSION==2)
		pmu_ccpu_slp_cal_done_intr,          		// Device specific 65
#else
#ifdef PLATFORM_FERMION
		pmu_ccpu_slp_cal_done_intr,          		// Device specific 65
#else
		DeviceInterrupt_Handler,					// Device specific 65
#endif /* PLATFORM_FERMION */
#endif //(NT_CHIP_VERSION==2)
		cmem_banka_ccpu_auto_pw,          			// Device specific 66
		cmem_bankb_ccpu_auto_pw,          			// Device specific 67
		cmem_bankc_ccpu_auto_pw,          			// Device specific 68
		cmem_bankd_ccpu_auto_pw,          			// Device specific 69
		cmem_cmn_ccpu_auto_pw,          			// Device specific 70
		nt_cpr_isr_handler,          			    // cpr_handler 71
		nt_spi_slv_interrupt ,          			// spi slave 72
		ahb_pd_error,          						// Device specific 73
#if (NT_CHIP_VERSION==2)
		DeviceInterrupt_Handler,                 	// Device specific 74
		aon_cmnss_dvs_done_int,                 	// Device specific 75
		aon_cmnss_pwfm_outoff_min_max_range_int,    // Device specific 76
		aon_cmnss_ulpm_outoff_min_max_range_int,    // Device specific 77
		pmu_ccpu_bbpll_lock_toggle_intr,            // Device specific 78
		pmu_ccpu_temp_mon_done_intr,                // Device specific 79
		pmu_ccpu_vbat_mon_done_intr,                // Device specific 80
		pmu_ccpu_xip_pwr_down_early_warn_to_ok_to_intr,  // Device specific 81
		DeviceInterrupt_Handler,                	// Device specific 82
		DeviceInterrupt_Handler,                 	// Device specific 83
		DeviceInterrupt_Handler,                 	// Device specific 84
		DeviceInterrupt_Handler,                 	// Device specific 85
		DeviceInterrupt_Handler,                 	// Device specific 86
		DeviceInterrupt_Handler,                 	// Device specific 87
		DeviceInterrupt_Handler,                 	// Device specific 88
		DeviceInterrupt_Handler,                	// Device specific 89
		pmu_ccpu_g2p_pready_timeout_intr,                	// Device specific 90
		pmu_ccpu_boot_strap_config_otp_read_err_intr,       // Device specific 91
		pmu_ccpu_rfa_pmic_otp_read_err_intr,                // Device specific 92
		pmu_ccpu_wlan_sleep_ack_timout_intr,                	// Device specific 93
		pmu_ccpu_wlan_sleep_ack_err_intr,                	// Device specific 94
		pmu_ccpu_wlan_coext_int_err_intr,                	// Device specific 95
		pmu_ccpu_wlan_coext_int_done_timout_intr,                	// Device specific 96
		pmu_ccpu_bbpll_lock_timeout_intr,                	// Device specific 97
		pmu_ccpu_boot_strap_config_otp_read_to_intr,                	// Device specific 98
		pmu_ccpu_rfa_pmic_otp_read_to_intr,                	// Device specific 99
		DeviceInterrupt_Handler,                	// Device specific 100
#else
#ifdef PLATFORM_FERMION
		DeviceInterrupt_Handler,                 	// Device specific 74
		DeviceInterrupt_Handler,                 	// Device specific 75
		DeviceInterrupt_Handler,    // Device specific 76
		DeviceInterrupt_Handler,    // Device specific 77
		DeviceInterrupt_Handler,            // Device specific 78
		pmu_ccpu_temp_mon_done_intr,                // Device specific 79
		pmu_ccpu_vbat_mon_done_intr,                // Device specific 80
		DeviceInterrupt_Handler,  				// Device specific 81
		DeviceInterrupt_Handler,                	// Device specific 82
		coex_bmh_isr,                 	// Device specific 83
		coex_smh_isr,                 	// Device specific 84
		coex_pmh_isr,                 	// Device specific 85
		coex_mcim_isr,                 	// Device specific 86
		coex_lmh_isr,                 	// Device specific 87
		DeviceInterrupt_Handler,                	// Device specific 88
		DeviceInterrupt_Handler,                 	// Device specific 89
		DeviceInterrupt_Handler,                 	// Device specific 90
		DeviceInterrupt_Handler,                 	// Device specific 91
		DeviceInterrupt_Handler,                 	// Device specific 92
		DeviceInterrupt_Handler,                 	// Device specific 93
		DeviceInterrupt_Handler,                 	// Device specific 94
		DeviceInterrupt_Handler,                	// Device specific 95
		DeviceInterrupt_Handler,                 	// Device specific 96
		DeviceInterrupt_Handler,                 	// Device specific 97
		DeviceInterrupt_Handler,                 	// Device specific 98
		DeviceInterrupt_Handler,                	// Device specific 99
		DeviceInterrupt_Handler,                	// Device specific 100
		DeviceInterrupt_Handler,                	// Device specific 101
		DeviceInterrupt_Handler,                	// Device specific 102
		DeviceInterrupt_Handler,                	// Device specific 103
		DeviceInterrupt_Handler,                	// Device specific 104
		DeviceInterrupt_Handler,                	// Device specific 105
		DeviceInterrupt_Handler,                	// Device specific 106
		DeviceInterrupt_Handler,                	// Device specific 107
#ifdef FIRMWARE_APPS_INFORMED_WAKE
		aon_a2f_deassert_isr_handler,  	// Device specific 108
#else
		DeviceInterrupt_Handler,                	// Device specific 108
#endif // FIRMWARE_APPS_INFORMED_WAKE
#endif // PLATFORM_FERMION
#endif //(NT_CHIP_VERSION==2)
	};

//-------------------------------------------------------------------------------
// Processor ends up here if an unexpected interrupt occurs or a specific
// handler is not present in the application code.

void __attribute__ ((section(".after_ram_vectors")))
Default_Handler(void)
{
	PROF_IRQ_ENTER();
	extern int process_routine;
	if(process_routine == 1)
	{
		__asm volatile("nop       \n ");
	}
	else
	{
		static char interrupt_string[100];
		uint32_t value =0;
		uint32_t rd_reg = 0xE000E200;
		uint32_t wr_reg = 0xE000E180;
		uint32_t wr_reg_clr = 0xE000E280;
		uint32_t pos=0,interrupt_no=0;
		uint32_t cnt=0;
		for(cnt = 0 ; cnt < 4;cnt++)
		{
			value= HAL_REG_RD(rd_reg);
			if(value != 0)
			{
				while(pos < 32)
				{
					if(value & (1<<pos))
					{
						interrupt_no +=pos;
						/*((char *)interrupt_string,"Interrupt  %d is SET be warned no interrupt handler is present no clearing interrupt \r\n ",interrupt_no);
						UART_Send(interrupt_string,sizeof(interrupt_string));
						HAL_REG_WR(wr_reg,value);*/
						HAL_REG_WR(wr_reg_clr,value);
						pos = 0;
						break;
					}
					pos++;
				}
			}
			interrupt_no = (cnt+1)*32;
			rd_reg +=4;
			wr_reg +=4;
			wr_reg_clr +=4;
		}
	}
	PROF_IRQ_EXIT();
}
void __attribute__ ((section(".after_ram_vectors"),weak))
ahb_pd_error(void)
{
	PROF_IRQ_ENTER();

	static char interrupt_string[100];
	uint32_t  value =0;
	value= HAL_REG_RD(QWLAN_CCU_R_CCU_CSS_AHB_PD_STATUS_REG);
	if(QWLAN_CCU_R_CCU_CSS_AHB_PD_STATUS_DCODE_XIP_RRAM_CCPU_PD_ERR_MASK & value)
	{
		HAL_REG_WR(0xE000E288,0x200);
		snprintf((char *)interrupt_string, sizeof(interrupt_string), "Interrupt ahb_pd_error \r\n");
		UART_Send(interrupt_string,sizeof(interrupt_string));
	}

	PROF_IRQ_EXIT();
}
// ----------------------------------------------------------------------------

//ISR for NVIC Interrupt no.0
void __attribute__ ((section(".after_ram_vectors"),weak))
CTI_INTISR_0(void){
	PROF_IRQ_ENTER();
	//clearing the interrupt from NVIC_ICPR
	nt_clear_device_irq(CTI_intisr_0);
#ifdef NT_DEBUG
	//dbg-port
	nt_dbg_print("Int0-clrd");
#endif
	PROF_IRQ_EXIT();
}

//ISR for NVIC Interrupt no.18
void __attribute__ ((section(".after_ram_vectors"),weak))
ccu_qgic2_apps_wlan_data_xfer_done(void){
	PROF_IRQ_ENTER();
	//clearing the interrupt from NVIC_ICPR
	nt_clear_device_irq(CCU_qgic2_apps_wlan_data_xfer_done);
#ifdef NT_DEBUG
	//dbg_port
	nt_dbg_print("Int18-clrd");
#endif
	PROF_IRQ_EXIT();
}

//ISR for NVIC Interrupt no.20
void __attribute__ ((section(".after_ram_vectors"),weak))
cmem_int(void){

	PROF_IRQ_ENTER();
	static char intr_str[50];
	uint32_t value=0;

#ifdef NT_DEBUG
	//dbg_port
	nt_dbg_print("in2-int20");
#endif

	//reading the CCPU Last invalid address
	value = HAL_REG_RD(QWLAN_CCU_R_CCU_CCPU_INVALID_ADDR_REG);
#ifdef NT_DEBUG
	snprintf((char *)intr_str,50,"CCPU Lst Inval ADDR %X \r\n",value);
	UART_Send(intr_str,sizeof(intr_str));
#endif

	//reading the address where first invalid access has happened
	value=HAL_REG_RD(QWLAN_CMEM_CMEM_INVALID_ADDR0_REG);
#ifdef NT_DEBUG
	snprintf((char *)intr_str,50,"1st inval acc ADDR %X \r\n",value);
	UART_Send(intr_str,sizeof(intr_str));
#endif

	//reading the second invalid address access
	value=HAL_REG_RD(QWLAN_CMEM_CMEM_INVALID_ADDR1_REG);
#ifdef NT_DEBUG
	snprintf((char *)intr_str,50,"2nd inval acc ADDR %X \r\n",value);
	UART_Send(intr_str,sizeof(intr_str));
#endif

	//reading the third invalid address access
	value=HAL_REG_RD(QWLAN_CMEM_CMEM_INVALID_ADDR2_REG);
#ifdef NT_DEBUG
	snprintf((char *)intr_str,50,"3rd inval acc ADDR %X \r\n",value);
	UART_Send(intr_str,sizeof(intr_str));
#endif

	//reading the forth invalid address access
	value=HAL_REG_RD(QWLAN_CMEM_CMEM_INVALID_ADDR3_REG);
#ifdef NT_DEBUG
	snprintf((char *)intr_str,50,"4th inval acc ADDR %X \r\n",value);
	UART_Send(intr_str,sizeof(intr_str));
#endif

	//reading the last ADDRESS0 THAT ARM HAS REQUESTED over the memory bus
	value=HAL_REG_RD(QWLAN_CCU_R_CCU_CCPU_LAST_ADDR0_REG);
#ifdef NT_DEBUG
	snprintf((char *)intr_str,50,"rq lst addr0 ovr d mem-bus: %X \r\n",value);
	UART_Send(intr_str,sizeof(intr_str));
#endif
	//reading the last ADDRESS1 THAT ARM HAS REQUESTED over the memory bus
	value=HAL_REG_RD(QWLAN_CCU_R_CCU_CCPU_LAST_ADDR1_REG);
#ifdef NT_DEBUG
	snprintf((char *)intr_str,50,"rq lst addr1 ovr d mem-bus: %X \r\n",value);
	UART_Send(intr_str,sizeof(intr_str));
#endif

	//reading the last ADDRESS2 THAT ARM HAS REQUESTED over the memory bus
	value=HAL_REG_RD(QWLAN_CCU_R_CCU_CCPU_LAST_ADDR2_REG);
#ifdef NT_DEBUG
	snprintf((char *)intr_str,50,"rq lst addr1 ovr d mem-bus: %X \r\n",value);
	UART_Send(intr_str,sizeof(intr_str));
#endif

	//clearing the interrupt from NVIC_ICPR
	nt_clear_device_irq(CMEM_int);
#ifdef NT_DEBUG
	nt_dbg_print("Int20-clrd");
#endif

	PROF_IRQ_EXIT();
}

//----------------------------------------------------------------------------------------------------------
//ISR for NVIC Interrupt no.36
void __attribute__ ((section(".after_ram_vectors"),weak))
ccpu_ccu_cmem_timeout_int_p(void){

	PROF_IRQ_ENTER();

#ifdef NT_DEBUG
	//dbg_port
	nt_dbg_print("in2 Int36\r\n");

#endif

//soft-reseting the CMEM
	HAL_REG_WR(QWLAN_CCU_R_CCU_SOFT_RESET_REG,QWLAN_CCU_R_CCU_SOFT_RESET_CMEM_SOFT_RESET_MASK);
	HAL_REG_WR(QWLAN_CCU_R_CCU_SOFT_RESET_REG,(uint32_t)0x00);

#ifdef NT_DEBUG
	nt_dbg_print("CMEM-SRst-succ \r\n");
#endif
	//clearing the interrupt from NVIC_ICPR
		nt_clear_device_irq(CCPU_ccu_cmem_timeout_int_p);

	#ifdef NT_DEBUG
		nt_dbg_print("Int36-clrd ");
	#endif

	PROF_IRQ_EXIT();
}
//-------------------------------------------------------------------------------------------------------------
//ISR for NVIC Interrupt no.38
void __attribute__((section(".after_ram_vectors"),weak))
pmu_ccpu_vbat_low_hit_int_p(void){

	PROF_IRQ_ENTER();
#ifdef NT_DEBUG
	//dbg_port
	nt_dbg_print("Vth reached,strt cnt\r\n");
#endif

	HAL_REG_WR( QWLAN_PMU_CFG_VABT_TEMP_MON_INT_CLR_REG , QWLAN_PMU_CFG_VABT_TEMP_MON_INT_CLR_VBAT_LOW_HIT_INT_CLR_MASK );
	nt_clear_device_irq(PMU_ccpu_vbat_low_hit_int_p);
#ifdef NT_DEBUG
	//dbg_port
	nt_dbg_print("BROUT int clrd\r\n");
#endif

	PROF_IRQ_EXIT();
}
//--------------------------------------------------------------------------------------------------------------
//ISR for NVIC Interrupt no.39
void __attribute__((section(".after_ram_vectors"),weak))
pmu_ccpu_temp_panic_hit_int(void){
	PROF_IRQ_ENTER();
#ifdef NT_DEBUG
	//dbg_port
	nt_dbg_print("temperature too high, shutting down\r\n");
#endif
	HAL_REG_WR( QWLAN_PMU_CFG_VABT_TEMP_MON_INT_CLR_REG , QWLAN_PMU_CFG_VABT_TEMP_MON_INT_CLR_TEMP_PANIC_HIGH_HIT_INT_CLR_MASK );
	nt_clear_device_irq( PMU_ccpu_temp_panic_hit_int);
#ifdef NT_DEBUG
	//dbg_port
	nt_dbg_print("H-TMP int clrd\r\n");
#endif
	PROF_IRQ_EXIT();
}
//----------------------------------------------------------------------------------------------------------------
//ISR for NVIC Interrupt no.45
void __attribute__ ((section(".after_ram_vectors"),weak))
rram_interrupt_handler(void){
	PROF_IRQ_ENTER();
#ifdef NT_DEBUG
	//dbg_port
	nt_dbg_print("in2 RRAM-Int\r\n");

#endif
#if defined(DEBUG)
	__DEBUG_BKPT();
#endif
	while(1);

	PROF_IRQ_EXIT();
}

//-------------------------------------------------------------------------------------------------------------

//ISR for NVIC Interrupt no.50
void __attribute__ ((section(".after_ram_vectors"),weak))
kdf_m4f_intr(void){
	PROF_IRQ_ENTER();
	static char intr_str[25];
	uint32_t value = 0;

#ifdef NT_DEBUG//dbg_port
	nt_dbg_print("in2 Int50 ");
#endif
	//reading kdf core intr status register
	value = HAL_REG_RD(QWLAN_KDF_CSR_R_INTR_STATUS_REG);
	//checking wheather the 17th bit is high
	if(value & QWLAN_KDF_CSR_R_INTR_STATUS_INTERRUPT_M4_OP_INVALID_MASK){
		//checking the opcode that caused the interrupt
		value = HAL_REG_RD(QWLAN_KDF_CSR_R_OP_CODE_REG);
#ifdef NT_DEBUG
		//printing the opcode with invalid messge
		snprintf((char *)intr_str,25,"Inval OPCode:%d \r\n",value);
		UART_Send(intr_str,sizeof(intr_str));
#endif
		//writing  1 to 17 th bit, to clear
		HAL_REG_WR(QWLAN_KDF_CSR_R_INTR_STATUS_REG,QWLAN_KDF_CSR_R_INTR_STATUS_INTERRUPT_M4_OP_INVALID_MASK);
#ifdef NT_DEBUG
		nt_dbg_print("Inval-OPcde-clrd \r\n");
#endif
	}
	//reading kdf core intr status regiter
	value = HAL_REG_RD(QWLAN_KDF_CSR_R_INTR_STATUS_REG);
	//checking wheather the 16 th bit is high or not
	if(value & QWLAN_KDF_CSR_R_INTR_STATUS_INTERRUPT_M4_OP_DONE_MASK){
		//printing status of M4 OP code operation
#ifdef NT_DEBUG
		nt_dbg_print("OP-dn \r\n");
#endif
	}
	else{
#ifdef NT_DEBUG
		//printing status of M4 OP code operation
		nt_dbg_print("OP-stp\r\n");
#endif
	}

	//reading lockwrapper intr status register
	value = HAL_REG_RD(QWLAN_LOCK_WRAPPER_R_INTR_STATUS_REG_REG);
	//checking the bit 0 is high or not
	if(value & QWLAN_LOCK_WRAPPER_R_INTR_STATUS_REG_INTR_STATUS_INTRSTATUS_MASK){
		//reading lock status register
		value = HAL_REG_RD(QWLAN_LOCK_WRAPPER_R_LOCK_STATUS_REG_REG);
		//checking the lock status bit
		if(value & QWLAN_LOCK_WRAPPER_R_LOCK_STATUS_REG_LOCK_STATUS_LOCKED_STATUS_MASK){
			//requesting for unlock asap
#ifdef NT_DEBUG
			nt_dbg_print("lkd \r\n");

#endif
			HAL_REG_WR(QWLAN_LOCK_WRAPPER_R_LOCK_RELEASE_REG_REG,QWLAN_LOCK_WRAPPER_R_LOCK_RELEASE_REG_LOCK_RELEASE_LOCK_RELEASE_MASK);
#ifdef NT_DEBUG
			nt_dbg_print("unlk-rq \r\n");

#endif
		}

	}

	//clearing the interrupt from NVIC_ICPR
	nt_clear_device_irq(KDF_m4f_intr);
#ifdef NT_DEBUG
	nt_dbg_print("Int50-clrd \r\n");

#endif
	PROF_IRQ_EXIT();
}
//------------------------------------------------------------------------------------------------
//ISR for NVIC Interrupt no.51
void __attribute__ ((section(".after_ram_vectors"),weak))
ecc_core_m4f_intr(void){
	PROF_IRQ_ENTER();
	uint32_t value = 0;

#ifdef NT_DEBUG
	//dbg-port
	nt_dbg_print("in2 Int51 \r\n");
#endif
	value = HAL_REG_RD(QWLAN_ELP_PKA_AHB_R_PKA_STATUS_REG_REG);

	if(value & QWLAN_ELP_PKA_AHB_R_PKA_STATUS_REG_PKA_STATUS_STAT_IRQ_MASK){

		HAL_REG_WR(QWLAN_ELP_PKA_AHB_R_PKA_STATUS_REG_REG,QWLAN_ELP_PKA_AHB_R_PKA_STATUS_REG_PKA_STATUS_STAT_IRQ_MASK);
	}

	//reading lockwrapper intr status register
	value = HAL_REG_RD(QWLAN_LOCK_WRAPPER_R_INTR_STATUS_REG_REG);
	//checking the bit 0 is high or not
	if(value & QWLAN_LOCK_WRAPPER_R_INTR_STATUS_REG_INTR_STATUS_INTRSTATUS_MASK){
		//reading lock status register
		value = HAL_REG_RD(QWLAN_LOCK_WRAPPER_R_LOCK_STATUS_REG_REG);
		//checking the lock status bit
		if(value & QWLAN_LOCK_WRAPPER_R_LOCK_STATUS_REG_LOCK_STATUS_LOCKED_STATUS_MASK){
#ifdef NT_DEBUG
			//requesting for unlock asap "ECC-lk \r\n""unlk-rq-snt\r\n"
			nt_dbg_print("ECC-lk \r\n");
#endif
			HAL_REG_WR(QWLAN_LOCK_WRAPPER_R_LOCK_RELEASE_REG_REG,QWLAN_LOCK_WRAPPER_R_LOCK_RELEASE_REG_LOCK_RELEASE_LOCK_RELEASE_MASK);
#ifdef NT_DEBUG
			nt_dbg_print("unlk-rq-snt\r\n");
#endif
		}
	}

	//clearing the interrupt from NVIC_ICPR
	nt_clear_device_irq(ECC_core_m4f_intr);

	PROF_IRQ_EXIT();
}

//-------------------------------------------------------------------------------------------------------------------------------------------------------------

//ISR for NVIC Interrupt no.56
void __attribute__ ((section(".after_ram_vectors"),weak))
ccpu_ccu_fp_exception_0(void){
	PROF_IRQ_ENTER();

#ifdef NT_DEBUG
	nt_dbg_print("in2 Int56 \r\n");

#endif
	if(nt_check_enabled_device_irq(CCPU_ccu_fp_exception_0)==0x0){
	//checking if enabled at NVIC
		//disabling at NVIC
		nt_disable_device_irq(CCPU_ccu_fp_exception_0);
#ifdef NT_DEBUG
		nt_dbg_print("Int56-dsbl \r\n");
#endif
		//clearing from pending
		nt_clear_device_irq(CCPU_ccu_fp_exception_0);
#ifdef NT_DEBUG
		nt_dbg_print("Int56-clrd \r\n");

#endif
	}

	PROF_IRQ_EXIT();
}

//ISR for NVIC Interrupt no.57
void __attribute__ ((section(".after_ram_vectors"),weak))
ccpu_ccu_fp_exception_1(void){
	PROF_IRQ_ENTER();

#ifdef NT_DEBUG
	nt_dbg_print("in2 Int57 \r\n");

#endif
	if(nt_check_enabled_device_irq(CCPU_ccu_fp_exception_1)==0x0){
	//checking if enabled at NVIC

		//disabling at NVIC
		nt_disable_device_irq(CCPU_ccu_fp_exception_1);
#ifdef NT_DEBUG
		nt_dbg_print("Int57-dsbl \r\n");
#endif
		//clearing from pending
		nt_clear_device_irq(CCPU_ccu_fp_exception_1);
#ifdef NT_DEBUG
		nt_dbg_print("Int57-clrd \r\n");

#endif
	}

	PROF_IRQ_EXIT();
}

//ISR for NVIC Interrupt no.58
void __attribute__ ((section(".after_ram_vectors"),weak))
ccpu_ccu_fp_exception_2(void){
	PROF_IRQ_ENTER();

#ifdef NT_DEBUG
	nt_dbg_print("in2 Int58 \r\n");

#endif
	if(nt_check_enabled_device_irq(CCPU_ccu_fp_exception_2)==0x0){
	//checking if enabled at NVIC
		//disabling at NVIC
		nt_disable_device_irq(CCPU_ccu_fp_exception_2);
#ifdef NT_DEBUG
		nt_dbg_print("Int58-dsbl \r\n");
#endif
		//clearing from pending
		nt_clear_device_irq(CCPU_ccu_fp_exception_2);
#ifdef NT_DEBUG
		nt_dbg_print("Int58-clrd \r\n");

#endif
	}

	PROF_IRQ_EXIT();
}



//ISR for NVIC Interrupt no.59
void __attribute__ ((section(".after_ram_vectors"),weak))
ccpu_ccu_fp_exception_3(void){
	PROF_IRQ_ENTER();

#ifdef NT_DEBUG
	nt_dbg_print("in2 Int59 \r\n");

#endif
	if(nt_check_enabled_device_irq(CCPU_ccu_fp_exception_3)==0x0){
	//checking if enabled at NVIC

		//disabling at NVIC
		nt_disable_device_irq(CCPU_ccu_fp_exception_3);
#ifdef NT_DEBUG
		nt_dbg_print("Int59-dsbl \r\n");
#endif
		//clearing from pending
		nt_clear_device_irq(CCPU_ccu_fp_exception_3);
#ifdef NT_DEBUG
		nt_dbg_print("Int59-clrd \r\n");

#endif
	}

	PROF_IRQ_EXIT();
}

//ISR for NVIC Interrupt no.60
void __attribute__ ((section(".after_ram_vectors"),weak))
ccpu_ccu_fp_exception_4(void){
	PROF_IRQ_ENTER();

#ifdef NT_DEBUG
	nt_dbg_print("in2 Int60 \r\n");

#endif
	if(nt_check_enabled_device_irq(CCPU_ccu_fp_exception_4)==0x0){
	//checking if enabled at NVIC
		//disabling at NVIC
		nt_disable_device_irq(CCPU_ccu_fp_exception_4);
#ifdef NT_DEBUG
		nt_dbg_print("Int60-dsbl \r\n");
#endif
		//clearing from pending
		nt_clear_device_irq(CCPU_ccu_fp_exception_4);
#ifdef NT_DEBUG
		nt_dbg_print("Int60-clrd \r\n");

#endif
	}

	PROF_IRQ_EXIT();
}

//ISR for NVIC Interrupt no.61
void __attribute__ ((section(".after_ram_vectors"),weak))
ccpu_ccu_fp_exception_5(void){
	PROF_IRQ_ENTER();

#ifdef NT_DEBUG
	nt_dbg_print("in2 Int61 \r\n");

#endif
	if(nt_check_enabled_device_irq(CCPU_ccu_fp_exception_5)==0x0){
	//checking if enabled at NVIC

		//disabling at NVIC
		nt_disable_device_irq(CCPU_ccu_fp_exception_5);
#ifdef NT_DEBUG
		nt_dbg_print("Int61-dsbl \r\n");
#endif
		//clearing from pending
		nt_clear_device_irq(CCPU_ccu_fp_exception_5);
#ifdef NT_DEBUG
		nt_dbg_print("Int61-clrd \r\n");

#endif
	}

	PROF_IRQ_EXIT();
}
void __attribute__ ((section(".after_ram_vectors"),weak))
CTI_INTISR_1(void)
{
	PROF_IRQ_ENTER();
	nt_clear_device_irq(CTI_intisr_1);
#ifdef NT_DEBUG
	nt_dbg_print("Int-1 clrd \r\n");
#endif
	PROF_IRQ_EXIT();
}
void __attribute__ ((section(".after_ram_vectors"),weak))
ccu_qgic2_apps_wlan_rx_data_avail(void)
{
	PROF_IRQ_ENTER();
	nt_clear_device_irq(CCU_qgic2_apps_wlan_rx_data_avail);
#ifdef NT_DEBUG
	nt_dbg_print("Int-19 clrd\r\n");
#endif
	PROF_IRQ_EXIT();
}
void __attribute__ ((section(".after_ram_vectors"),weak))
ccpu_ccu_ahb_err_int_p(void)
{
	PROF_IRQ_ENTER();
		uint32_t  value =0;
		value= HAL_REG_RD(QWLAN_CCU_R_CCU_CCPU_INVALID_ADDR_REG);
		if( value)
		{
			nt_clear_device_irq(CCPU_ccu_ahb_err_int_p);
#ifdef NT_DEBUG
	nt_dbg_print("Int-35 clrd\r\n");
#endif
		}

	PROF_IRQ_EXIT();
}

void __attribute__ ((section(".after_ram_vectors"),weak))
rri_ccu_intr3(void)
{
	PROF_IRQ_ENTER();
	nt_clear_device_irq(RRI_ccu_int_p_3);
#ifdef NT_DEBUG
	nt_dbg_print("Int-43 clrd\r\n");
#endif
	PROF_IRQ_EXIT();
}

void __attribute__ ((section(".after_ram_vectors"),weak))
ccu_qgic2_apss_asic_intr(void)
{
	PROF_IRQ_ENTER();
	nt_clear_device_irq(CCU_qgic2_apss_asic_intr);
#ifdef NT_DEBUG
	nt_dbg_print("Int-44 clrd\r\n");
#endif
	PROF_IRQ_EXIT();
}
void __attribute__ ((section(".after_ram_vectors"),weak))
o_xpu2_non_secure_intr(void)
{
	PROF_IRQ_ENTER();
		uint32_t  value =0;
		value= HAL_REG_RD(QWLAN_XPU_R_XPU_EAR0_REG);
		if( value)
		{
			nt_clear_device_irq(o_XPU2_non_secure_intr);
#ifdef NT_DEBUG
	nt_dbg_print("Int-49 clrd \r\n");
#endif
		}

	PROF_IRQ_EXIT();
}
void __attribute__ ((section(".after_ram_vectors"),weak))
cc_intr(void)
{
	PROF_IRQ_ENTER();
	uint32_t  value =0;
	value= HAL_REG_RD(QWLAN_PERISS_CRYPTO_CORE_R_CRYPTO_STATUS_REG);
	if( value)
	{
		nt_clear_device_irq(CC_intr);
#ifdef NT_DEBUG
		nt_dbg_print("Int-52 clrd \r\n");
#endif
	}

	PROF_IRQ_EXIT();
}

void __attribute__ ((section(".after_ram_vectors"),weak))
o_xpu2_secure_intr(void)
{
	PROF_IRQ_ENTER();

	static char intr_str[50];
	uint32_t  value =0;
	value= HAL_REG_RD(QWLAN_XPU_R_XPU_SEAR0_REG);
#ifdef NT_DEBUG
	snprintf((char *)intr_str,50,"reg val = %08X",value);
#endif
	if(value)
	{
		HAL_REG_WR(QWLAN_XPU_R_XPU_SEAR0_REG,QWLAN_XPU_R_XPU_SEAR0_DEFAULT);
#ifdef NT_DEBUG
		nt_dbg_print("Int-64 prcd\r\n");
#endif
	}

	PROF_IRQ_EXIT();
}
void __attribute__ ((section(".after_ram_vectors"),weak))
cmem_banka_ccpu_auto_pw(void)
{
	PROF_IRQ_ENTER();

	nt_clear_device_irq(CMEM_banka_ccpu_auto_pw);
#ifdef NT_DEBUG
	nt_dbg_print("Int-66 clrd \r\n");
#endif

	PROF_IRQ_EXIT();
}
void __attribute__ ((section(".after_ram_vectors"),weak))
cmem_bankb_ccpu_auto_pw(void)
{
	PROF_IRQ_ENTER();

	nt_clear_device_irq(CMEM_bankb_ccpu_auto_pw);
#ifdef NT_DEBUG
	nt_dbg_print("Int-67 clrd \r\n");
#endif

	PROF_IRQ_EXIT();
}
void __attribute__ ((section(".after_ram_vectors"),weak))
cmem_bankc_ccpu_auto_pw(void)
{
	PROF_IRQ_ENTER();

	nt_clear_device_irq(CMEM_bankc_ccpu_auto_pw);
#ifdef NT_DEBUG
	nt_dbg_print("Int-68 clrd \r\n");
#endif

	PROF_IRQ_EXIT();
}
void __attribute__ ((section(".after_ram_vectors"),weak))
cmem_bankd_ccpu_auto_pw(void)
{
	PROF_IRQ_ENTER();

	nt_clear_device_irq(CMEM_bankd_ccpu_auto_pw);
#ifdef NT_DEBUG
	nt_dbg_print("Int-69 clrd \r\n");
#endif

	PROF_IRQ_EXIT();
}
void __attribute__ ((section(".after_ram_vectors"),weak))
cmem_cmn_ccpu_auto_pw(void)
{
	PROF_IRQ_ENTER();

	nt_clear_device_irq(CMEM_cmn_ccpu_auto_pw);
#ifdef NT_DEBUG
	nt_dbg_print("Int-70 clrd \r\n");
#endif

	PROF_IRQ_EXIT();
}
