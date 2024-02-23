/*
 * This file is part of the µOS++ distribution.
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

#ifndef CORTEXM_EXCEPTION_HANDLERS_H_
#define CORTEXM_EXCEPTION_HANDLERS_H_

#include <stdint.h>

#if defined(DEBUG)
#define __DEBUG_BKPT()  asm volatile ("bkpt 0")
#endif

/*#define CTI_INT_ISR_ENABLE 								0
#define IC_INTR_ENABLE 									0
#define UART_IRQ_HANDLER_ENABLE 						1
#define QTMR_QCIC2_PHY_IRQ_ENABLE 						0
#define QTMR_QCIC2_VIRT_IRQ_ENABLE 						0
#define CCU_QGIC2_CCPU_MAILBOX_INT_ENABLE				0
#define CCU_QGIC2_MSM_MAILBOX_INT_ENABLE 				0
#define CCU_QGIC2_APPS_WLAN_DATA_XFER_DONE_ENABLE 		0
#define CCU_QGIC2_APPS_WLAN_RX_DATA_AVAIL_ENABLE 		0
#define CMEM_INT_ENABLE 								0
#define NDXE_IRQ_HANDLER_ENABLE 						1
#define WLAN_CCU_IRQ_ENABLE 							0
#define WLAN_CCU_FIQ_ENABLE 							0
#define CCPU_CCU_AHB_ERR_INT_ENABLE 					0
#define CCPU_CCU_CMEM_TIMEOUT_INT_ENABLE				0
#define PMU_CCPU_VBAT_PANIC_HIT_INT_ENABLE 				0
#define PMU_CCPU_VBAT_LOW_HIT_INT_ENABLE 				0
#define PMU_CCPU_TEMP_PANIC_HIT_INT_ENABLE 				0
#define RRI_CCU_INT_ENABLE								0
#define CCU_QGIC2_APPS_ASIC_INTR_ENABLE 				0
#define RRAM_INTRRUPT_ENABLE 							0
#define CCPU_QSPI_IRQ_ENABLE 							0
#define GPIO_INTR_ENABLE 								0
#define O_XPU2_NON_SECURE_INTR_ENABLE 					0
#define KDF_M4F_INTR_ENABLE 							0
#define ECC_CORE_M4F_INTR_ENABLE 						0
#define CC_INTR_ENABLE									0
#define WUR_CPU_INT_ENABLE 								0
#define WCSS_WDOG_BARK_ENABLE 							0
#define AON_CMNSS_WLAN_SLP_TMR_INT_ENABLE 				0
#define PMU_CCPU_VBAT_TEMP_PANIC_HIT_INT_ENABLE 		0*/
// ----------------------------------------------------------------------------

/* ARM - System Control Space Registers Address Map */

// NVIC Registers Address Map
/*-----Interrupt Set-Enable Registers----<0xE000E100-0xE000E13C>------*/
#define NVIC_ISER0	( 0xE000E100 )
#define NVIC_ISER1	( 0xE000E104 )
#define NVIC_ISER2	( 0xE000E108 )
#define NVIC_ISER3	( 0xE000E10C )

/*-----Interrupt Clear-Enable Registers----<0xE000E180-0xE000E1BC>------*/
#define NVIC_ICER0	( 0xE000E180 )
#define NVIC_ICER1	( 0xE000E184 )
#define NVIC_ICER2	( 0xE000E188 )
#define NVIC_ICER3	( 0xE000E18C )

/*-----Interrupt Set-Pending Registers----<0xE000E200-0xE000E23C>------*/
#define NVIC_ISPR0	( 0xE000E200 )
#define NVIC_ISPR1	( 0xE000E204 )
#define NVIC_ISPR2	( 0xE000E208 )
#define NVIC_ISPR3	( 0xE000E20C )

/*-----Interrupt Clear-Pending Registers----<0xE000E280-0xE000E2BC>------*/
#define NVIC_ICPR0	( 0xE000E280 )
#define NVIC_ICPR1	( 0xE000E284 )
#define NVIC_ICPR2	( 0xE000E288 )
#define NVIC_ICPR3	( 0xE000E28C )

/*-----Interrupt Active-Bit Registers----<0xE000E300-0xE000E33C>------*/
#define NVIC_IABR0	( 0xE000E300 )
#define NVIC_IABR1	( 0xE000E304 )
#define NVIC_IABR2	( 0xE000E308 )
#define NVIC_IABR3	( 0xE000E30C )

/*-----Interrupt Priority Registers----<0xE000E400-0xE000E5EC>------*/
#define NVIC_IPR0	( 0xE000E400 )
#define NVIC_IPR1	( 0xE000E404 )
#define NVIC_IPR2	( 0xE000E408 )
#define NVIC_IPR3	( 0xE000E40C )

#if defined(__cplusplus)
extern "C"
{
#endif

// External references to cortexm_handlers.c

  extern void
  Reset_Handler (void);
  extern void
  NMI_Handler (void);
  extern void
  HardFault_Handler (void);

#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
  extern void
  MemManage_Handler (void);
  extern void
  BusFault_Handler (void);
  extern void
  UsageFault_Handler (void);
  extern void
  DebugMon_Handler (void);
#endif

  extern void
  SVC_Handler (void);

  extern void
  PendSV_Handler (void);
  extern void
  SysTick_Handler (void);
//-----------------------------------------------------------------------------------
  //enum for neutrino vectors
  enum nt_interrupts{
 	  CTI_intisr_0 = 0,
 	  CTI_intisr_1,
 	  i2c_intr,
 	  uart_intr,
 	  Qtmr_qgic2_phy_irq_0,
 	  Qtmr_qgic2_phy_irq_1,
 	  Qtmr_qgic2_phy_irq_2,
 	  Qtmr_qgic2_phy_irq_3,
 	  Qtmr_qgic2_phy_irq_4,
 	  Qtmr_qgic2_virt_irq,
 	  CCU_qgic2_ccpu_mailbox_int_p_0,
 	  CCU_qgic2_ccpu_mailbox_int_p_1,
 	  CCU_qgic2_ccpu_mailbox_int_p_2,
 	  CCU_qgic2_ccpu_mailbox_int_p_3,
 	  CCU_qgic2_msm_mailbox_int_p_0,
 	  CCU_qgic2_msm_mailbox_int_p_1,
 	  CCU_qgic2_msm_mailbox_int_p_2,
 	  CCU_qgic2_msm_mailbox_int_p_3,
 	  CCU_qgic2_apps_wlan_data_xfer_done,
 	  CCU_qgic2_apps_wlan_rx_data_avail,
 	  CMEM_int,
 	  DXE_qgic2_per_channel_int_0,
 	  DXE_qgic2_per_channel_int_1,
 	  DXE_qgic2_per_channel_int_2,
 	  DXE_qgic2_per_channel_int_3,
 	  DXE_qgic2_per_channel_int_4,
 	  DXE_qgic2_per_channel_int_5,
 	  DXE_qgic2_per_channel_int_6,
 	  DXE_qgic2_per_channel_int_7,
 	  DXE_qgic2_per_channel_int_8,
 	  DXE_qgic2_per_channel_int_9,
 	  DXE_qgic2_per_channel_int_10,
 	  DXE_qgic2_per_channel_int_11,
 	  WLAN_ccu_irq,
 	  WLAN_ccu_fiq,
 	  CCPU_ccu_ahb_err_int_p,
 	  CCPU_ccu_cmem_timeout_int_p,
 	  PMU_ccpu_sif_soft_reset_intr,
 	  PMU_ccpu_vbat_low_hit_int_p,
 	  PMU_ccpu_temp_panic_hit_int,
 	  RRI_ccu_int_p_0,
 	  RRI_ccu_int_p_1,
 	  RRI_ccu_int_p_2,
 	  RRI_ccu_int_p_3,
 	  CCU_qgic2_apss_asic_intr,
 	  RRAM_interrupt,
 	  CCPU_qspi_irq0,
 	  CCPU_qspi_irq1,
 	  GPIO_intr,
 	  o_XPU2_non_secure_intr,
 	  KDF_m4f_intr,
 	  ECC_core_m4f_intr,
 	  CC_intr,
 	  WUR_cpu_int,
 	  WCSS_wdog_bark,
 	  AON_cmnss_wlan_slp_tmr_int,
 	  CCPU_ccu_fp_exception_0, //FPIDC
 	  CCPU_ccu_fp_exception_1, //FPDZC
 	  CCPU_ccu_fp_exception_2, //FPIOC
 	  CCPU_ccu_fp_exception_3, //FPUFC
 	  CCPU_ccu_fp_exception_4, //FPOFC
 	  CCPU_ccu_fp_exception_5, //FPIXC
 	  AHBg2ahbl_err_intrp,
 	  AON_cmnss_ext_wakeup_int,
 	  o_XPU2_secure_intr,
 	  o_XPU2_msa_intr,
 	  CMEM_banka_ccpu_auto_pw,
 	  CMEM_bankb_ccpu_auto_pw,
 	  CMEM_bankc_ccpu_auto_pw,
 	  CMEM_bankd_ccpu_auto_pw,
 	  CMEM_cmn_ccpu_auto_pw,
 	  CPR_intr,
 	  SPI_intr,
 	  CSS_ahb_pd_err
   };
  // Neutrino vectors
void nt_global_irq_init();
void nt_disable_nvic(uint64_t irq_number);
void nt_clear_device_irq(enum nt_interrupts);
uint8_t nt_check_enabled_device_irq(enum nt_interrupts);
void nt_disable_device_irq(enum nt_interrupts);
void nt_enable_device_irq(enum nt_interrupts);
void uart_irq_handler (void);
void I2C_irq_handler (void);
void ext_irq_handler (void);
void nt_dxe_interrupt_handler (void);
void wlan_ccu_irq_handler (void);
void wlan_ccu_fiq_handler (void);
void Aon_cmnss_wlan_slp_tmr_int (void);
void nt_wdt_int_wcss_wdog_bark (void);
void wur_cpu_int(void);
void nt_spi_slv_interrupt (void);
void nt_cpr_isr_handler(void);
void nt_gpio_interrupt_enable(void);
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
//-------------------------------------------------------------------------------
  // Exception Stack Frame of the Cortex-M3 or Cortex-M4 processor.
  typedef struct
  {
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r12;
    uint32_t lr;
    uint32_t pc;
    uint32_t psr;
#if  defined(__ARM_ARCH_7EM__)
    uint32_t s[16];
#endif
  } ExceptionStackFrame;



#if defined(TRACE)
#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
  void
  dumpExceptionStack (ExceptionStackFrame* frame, uint32_t cfsr, uint32_t mmfar,
                      uint32_t bfar, uint32_t lr);
#endif // defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
#if defined(__ARM_ARCH_6M__)
  void
  dumpExceptionStack (ExceptionStackFrame* frame, uint32_t lr);
#endif // defined(__ARM_ARCH_6M__)
#endif // defined(TRACE)

  void
  HardFault_Handler_C (ExceptionStackFrame* frame, uint32_t lr);

#if defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)
  void
  UsageFault_Handler_C (ExceptionStackFrame* frame, uint32_t lr);
  void
  BusFault_Handler_C (ExceptionStackFrame* frame, uint32_t lr);
#endif // defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7EM__)

#if defined(__cplusplus)
}
#endif

void assert_handler(const char*  file,const char* func, const uint32_t line);
// ----------------------------------------------------------------------------

#endif // CORTEXM_EXCEPTION_HANDLERS_H_
