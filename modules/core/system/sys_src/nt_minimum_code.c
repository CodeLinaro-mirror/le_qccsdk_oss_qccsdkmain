/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "cmsis_device.h"

#include <stdlib.h>
#include <stdint.h>

#include "FreeRTOSConfig.h"

#include "nt_common.h"
#include "nt_minimum_code.h"
#include "uart.h"
#include "nt_hw.h"
#include "nt_hw_support.h"
#include "wlan_power.h"
#include "nt_socpm_sleep.h"
#include "fdi_rmc.h"
#include "wifi_fw_pwr_cb_infra.h"
#include "timer_internal.h"
#include "nt_gpio_api.h"
#include "wifi_fw_internal_api.h"
#include "wifi_fw_pmu_ts_cfg.h"
#include "hal_int_modules.h"
#include "data_path.h"
#include "wmi.h"
#include "wifi_fw_ext_intr.h"

#if defined(PLATFORM_FERMION) && !defined(EMULATION_BUILD)
#include "wifi_fw_cpr_driver.h"
#endif

#ifdef SUPPORT_QCSPI_SLAVE
#include "qcspi_slave_api.h"
#endif
//#ifdef NT_FN_WATCHDOG
#include "nt_wdt_api.h"
//#endif // NT_FN_WATCHDOG

uint32_t debug_sleep_min_enter_cnt = 0;

// fake wake up - always for 100ms
//#define _MIN_TST_INC_FAKE_WAKE

// force full wake each beacon interval
//#define _MIN_TST_INC_FULL_WAKE

// force rc before bbpll lock, then switch back to pmic/xo after lock
// #define _MIN_INC_TST_FORCE_RC_BBPLL

#define portNVIC_SYSPRI2_REG (*((volatile uint32_t *)0xe000ed20))
#define ENABLE_IRQ           0xFFE00008
#define ENABLE_IRQ1          0x80000005
#define CACHE_REG_BASE       0x01180000

#define GLOBAL_INTRPENDING  0xE000E200
#define GLOBAL_INTRPENDING1 0xE000E204
#define GLOBAL_INTRPENDING2 0xE000E208
#define NT_ENABLE_ALL       0xFFFFFFFF

#define portNVIC_PENDSV_PRI  (((uint32_t)configKERNEL_INTERRUPT_PRIORITY) << 16UL)
#define portNVIC_SYSTICK_PRI (((uint32_t)configKERNEL_INTERRUPT_PRIORITY) << 24UL)

#ifdef NT_FN_DEBUG_PWRSV
//#undef NT_FN_DEBUG_PWRSV
#endif

// for debug purposes, use systick counter and uart
#ifdef NT_FN_DEBUG_PWRSV
#define _MIN_M4_SYSTCK_CSR_REG 0xE000E010
#define _MIN_M4_SYSTCK_CSR_CFG 0x00000005  // enable, cpu clock, no int

#define _MIN_M4_SYSTCK_CVR_REG  0xE000E018
#define _MIN_M4_SYSTCK_CVR_DFLT NT_SYSTCK_PS_DFLT

#define _MIN_M4_SYSTCK_RVR_REG  0xE000E014
#define _MIN_M4_SYSTCK_RVR_DFLT _MIN_M4_SYSTCK_CVR_DFLT

#define _MIN_CPU_TMR_INIT()                                         \
    {                                                               \
        NT_REG_WR(_MIN_M4_SYSTCK_CVR_REG, _MIN_M4_SYSTCK_CVR_DFLT); \
        NT_REG_WR(_MIN_M4_SYSTCK_RVR_REG, _MIN_M4_SYSTCK_RVR_DFLT); \
        NT_REG_WR(_MIN_M4_SYSTCK_CSR_REG, _MIN_M4_SYSTCK_CSR_CFG);  \
    }

#else  // NT_FN_DEBUG_PWRSV
#define _MIN_CPU_TMR_INIT()
//  #define _MIN_UART_INIT()
#endif  // NT_FN_DEBUG_PWRSV

#if !defined(EMULATION_BUILD)
#define _MIN_BBPLL_LOCK() _min_bbpll_lock()
#endif  // !defined(EMULATION_BUILD)
#define _MIN_UART_INIT() uart_init()

#define FORCE_FAULT_TEST 0

extern void _start(void);
extern void _minprintf(char *str, unsigned int a1, unsigned int a2);

extern volatile uint32_t nt_socpm_m4_regs[15];
extern SOCPM_STRUCT g_socpm_struct;

uint32_t load_r13[2];
int process_routine = 0;
int process_uart_rx_irq = 1;

typedef struct _min_pair_s_ {
    uint32_t addr;
    uint32_t val;
} _min_pair_t;

static void _min_enable_vfp(void) __attribute__((naked));

#if !defined(EMULATION_BUILD)
// return 1 if lock fails, 0 on success
static uint8_t _min_bbpll_lock(void)
{
#ifdef SOCPM_SLEEP_DEBUG
    socpm_log_timestamp(_PRE_BBPLL, 0, 0, 0);
#endif

#ifndef PLATFORM_FERMION
    /* below set of register are from RFA and as per Fermion desgin these should be retained accross sleep
       so these should not be part of ram minimal code. Removing these for Fermion HW */
    static const _min_pair_t bb1[] = {
        {0x2040200, 0x070f2400}, {0x2042400, 0x14037000}, {0x2042404, 0x0f037000}, {0x2042408, 0x33c33000},
        {0x204240C, 0x2ed33000}, {0x2042410, 0x0f1ee71f}, {0x2042414, 0x0d7efc1f}, {0x2042418, 0x0f1ee71f},
        {0x204241C, 0x0d7efc1f}, {0x2042420, 0x8daec703}, {0x2042424, 0x8c6edb03}, {0x2042428, 0x8daec703},
        {0x204242C, 0x8c6edb03}, {0x2042448, 0xb23318db}, {0x204244C, 0x00003370}, {0x2042438, 0x66000007},
        {0x2041C00, 0x33442297}, {0x2041C0C, 0x0e008700}, {0x2041C48, 0x00000028}, {0x2040400, 0x00000002},
        {0x2040408, 0x3fbfc997}, {0x2040410, 0x0281f070}, {0x2040900, 0x94942b00}, {0x2040810, 0x116c2040},
        {0x2040840, 0x1da41540}, {0x2040984, 0x00039ebd}, {0x2040988, 0xc000b00b}, {0x2040C00, 0x000007cf},
    };

    static const _min_pair_t bb2[] = {
        {0x2040C00, 0x000007ef}, {0x2040C04, 0x00006421}, {0x2040C08, 0x01e0ee00}, {0x2040C0C, 0x00002000},
        {0x2041804, 0x40b8113f}, {0x2041808, 0x41381100}, {0x204180C, 0x81b8113f}, {0x2041810, 0x8238113f},
        {0x2041814, 0x82b8113f}, {0x2041818, 0x8338113f}, {0x2042000, 0x00000000}, {0x2042004, 0x00000000},
        {0x204200C, 0x00c00000}, {0x2042010, 0x08c00000}, {0x2042014, 0x10c00000}, {0x2042018, 0x18c00000},
        {0x204201C, 0x20c00000}, {0x2041C14, 0x3d424000}, {0x2041C18, 0x3cbe4000},
    };

#endif /* PLATFORM_FERMION */
    // save boot complete state, enable phy/mac domains
    uint32_t btcmpl = NT_REG_RD(QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_REG);
    NT_REG_WR(QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_REG, btcmpl | 0xF000);

    int count = 0;

#ifdef _MIN_INC_TST_FORCE_RC_BBPLL
    {
        uint32_t regval = NT_REG_RD(QWLAN_PMU_AON_TOP_CFG_REG);
        regval &=
            ~(QWLAN_PMU_AON_TOP_CFG_CFG_XO_SLP_CLK_SEL_EN_MASK | QWLAN_PMU_AON_TOP_CFG_CFG_EXT_SLP_CLK_SEL_EN_MASK);
        // regval = regval | QWLAN_PMU_AON_TOP_CFG_CFG_EXT_SLP_CLK_SEL_EN_MASK;
        NT_REG_WR(QWLAN_PMU_AON_TOP_CFG_REG, regval);

        nt_socpm_nop_delay(20000);
        // for(int i = 0; i < 20000; i++) { asm volatile("nop"); }
    }
#endif

    // switch to XO clock before cfging bbpll
    NT_REG_WR(QWLAN_PMU_CFG_XO_PLL_CLK_SEL_CNTL_REG, QWLAN_PMU_CFG_XO_PLL_CLK_SEL_CNTL_CFG_XO_PLL_HW_SW_CNTL_MASK);
    nt_socpm_nop_delay(500);

    uint8_t locked = 0;
    int nattempts = 0;
    do {
#ifndef PLATFORM_FERMION
        unsigned int i;

        for (i = 0; i < (sizeof(bb1) / sizeof(bb1[0])); i++)
            NT_REG_WR(bb1[i].addr, bb1[i].val);
        nt_socpm_nop_delay(200);
        for (i = 0; i < (sizeof(bb2) / sizeof(bb2[0])); i++)
            NT_REG_WR(bb2[i].addr, bb2[i].val);
#endif /* PLATFORM_FERMION */
        nt_socpm_nop_delay(200);
        do {
            nt_socpm_nop_delay(100);  // was 5000
            if (QWLAN_PMU_BBPLL_STATUS_BBPLL_LOCK_DET_MASK ==
                (NT_REG_RD(QWLAN_PMU_BBPLL_STATUS_REG) & QWLAN_PMU_BBPLL_STATUS_BBPLL_LOCK_DET_MASK)) {
                locked = 1;
                break;
            }
        } while (count++ < 3500);
        nt_socpm_nop_delay(100);  // was 500
        nattempts++;
    } while ((!locked) && (nattempts < 10));

    // switch to bbpll from XO
    NT_REG_WR(QWLAN_PMU_CFG_XO_PLL_CLK_SEL_CNTL_REG, (QWLAN_PMU_CFG_XO_PLL_CLK_SEL_CNTL_CFG_XO_PLL_CLK_SEL_MASK |
                                                      QWLAN_PMU_CFG_XO_PLL_CLK_SEL_CNTL_CFG_XO_PLL_HW_SW_CNTL_MASK));
    nt_socpm_nop_delay(200);  // was 5000
    // restore the boot complete state
    NT_REG_WR(QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_REG, btcmpl);
    nt_socpm_nop_delay(200);

#ifdef _MIN_INC_TST_FORCE_RC_BBPLL
    {
        NT_REG_WR(QWLAN_PMU_AON_TOP_CFG_REG,
                  QWLAN_PMU_AON_TOP_CFG_DEFAULT | QWLAN_PMU_AON_TOP_CFG_CFG_EXT_SLP_CLK_SEL_EN_MASK);

        nt_socpm_nop_delay(50000);
        // for(int i = 0; i < 20000; i++) { asm volatile("nop"); }
    }
#endif

#ifdef SOCPM_SLEEP_DEBUG
    socpm_log_timestamp(POST_BBPLL, (uint32_t)count, locked, (uint32_t)nattempts);
#endif

    return ((count >= 3500) ? 1 : 0);  // lock failed = 1
}
#endif  // !defined(EMULATION_BUILD)

uint8_t get_warmboot_status(void)
{
    return g_socpm_struct.in_warm_boot;
}
/*
 * @brief: functionality to read rmc system status
 * @param: none
 * @return: rmc system status dword
 */
uint32_t get_rmc_system_status(void)
{
    return g_socpm_struct.rmc_system_status;
}
#if defined(SUPPORT_HIGH_RES_TIMER)

/**
 * @brief   Get the current qtimer value in us.
 * @return  Current time in us
 */
static uint64_t __attribute__((used)) min_get_qtimer_time_in_us(void)
{
    uint32_t freq = TIMER_GET_FRQ();
    return ((hres_timer_timetick_get() * 1000000) / freq);
}
#endif /*SUPPORT_HIGH_RES_TIMER*/

uint32_t min_mcu_cfg = 0xff0;
void min_mcu_active()
{
#if (FERMION_CHIP_VERSION == 2)
    uint32_t value;
#ifdef SOCPM_SLEEP_DEBUG
    if (bcn_nowake_limit == 1)
        return;
#endif

    if (min_mcu_cfg & 0x1) {
        value = NT_REG_RD(QWLAN_PMU_DIG_TOP_CFG_REG);
        value |= (QWLAN_PMU_DIG_TOP_CFG_FORCE_BANK_B_CORE_ON_MASK | QWLAN_PMU_DIG_TOP_CFG_FORCE_BANK_C_CORE_ON_MASK |
                  QWLAN_PMU_DIG_TOP_CFG_FORCE_BANK_D_CORE_ON_MASK | QWLAN_PMU_DIG_TOP_CFG_FORCE_BANK_E_CORE_ON_MASK);
        value |= QWLAN_PMU_DIG_TOP_CFG_RRAM_PD_MODE_DEFAULT;
        NT_REG_WR(QWLAN_PMU_DIG_TOP_CFG_REG, value);
    }

    if (min_mcu_cfg & 0x2) {
        value = (QWLAN_PMU_CFG_AON_CNTL_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_PMIC_DTOP_CNTL_BIT_MASK |
                 QWLAN_PMU_CFG_AON_CNTL_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_XO_DTOP_CNTL_BIT_MASK |
                 QWLAN_PMU_CFG_AON_CNTL_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_RFA_DTOP_CNTL_BIT_MASK |
                 QWLAN_PMU_CFG_AON_CNTL_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_SON_CNTL_BIT_MASK);
        value |= QWLAN_PMU_CFG_AON_CNTL_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_MEM_MX_CNTL_BIT_DEFAULT;
        NT_REG_WR(QWLAN_PMU_CFG_AON_CNTL_MCU_ACTIVE_STATE_RESOURCE_REQ_REG, value);
    }

    if (min_mcu_cfg & 0x4) {
        value = 0;
        value |= (min_mcu_cfg & 0x10) ? QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_CMNSS_CNTL_BIT_MASK : 0;
        value |= (min_mcu_cfg & 0x20) ? QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_CMEM_BANK_A_CNTL_BIT_MASK : 0;
        value |= (min_mcu_cfg & 0x40) ? QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_CMEM_BANK_B_CNTL_BIT_MASK : 0;
        value |= (min_mcu_cfg & 0x80) ? QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_CMEM_BANK_C_CNTL_BIT_MASK : 0;
        value |= (min_mcu_cfg & 0x100) ? QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_CMEM_BANK_D_CNTL_BIT_MASK : 0;
        value |= (min_mcu_cfg & 0x200) ? QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_CMEM_BANK_E_CNTL_BIT_MASK : 0;
        value |= (min_mcu_cfg & 0x400) ? QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_XIP_CNTL_BIT_MASK : 0;
        value |= (min_mcu_cfg & 0x800) ? QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_PSS_CNTL_BIT_MASK : 0;
        NT_REG_WR(QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_REG, value);
        NT_REG_WR(QWLAN_PMU_CFG_MCU_SS_STATE_REG, NT_PMU_CFG_MCU_ACTIVE_OFFSET);
    }

    if (min_mcu_cfg & 0x8) {
        NT_REG_WR(QWLAN_PMU_SLP_CNTL_REG, 0);
    }
#endif
}

void __attribute__((section(".ram_minimum_entry"), noreturn)) ram_minimum_code(void)
{
    // Delay accessing other CMEM banks/sub-banks to avoid power inrush issues
    for (int i = 0; i < MIN_CMEM_INRUSH_DELAY; i++) {
        __asm volatile(" nop \n");
    }
    extern uint64_t nt_socpm_slp_time_total;
    extern int mcu_sleep_force;
    extern int nt_socpm_resume_f;
    extern unsigned int __vectors_start;
    uint32_t warm_boot_sts;
    uint32_t slp_tmr_sts;
#if defined(COMPENSATE_RC_DIVISION_ERROR_WAR)
    uint32_t compensation_hre =0;
    time_timetick_type hres_tick_after_comp = 0;
    uint32_t delta_aon =0;
    uint32_t delta_glb= 0;
#endif /* COMPENSATE_RC_DIVISION_ERROR_WAR */
    debug_sleep_min_enter_cnt++;
#ifdef SOCPM_SLEEP_DEBUG
    socpm_log_timestamp(
        _MIN_ENTRY,
        ((NT_REG_RD(QWLAN_PMU_CFG_MCU_SS_STATE_REG) & 0x700) << 12) | NT_REG_RD(QWLAN_PMU_POWER_DOMAIN_STATUS_REG),
        NT_REG_RD(QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_REG),
        NT_REG_RD(QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_REG));
#endif
    min_mcu_active();

#ifdef POWER_SLP_CLK_SWITCH_WAR
    /** Disable sleep clock before sleep and enable on warm boot as a workaround
     * to deal with XO settle related memory access issues when HW wakeup is
     * quicker. This is needed when XO detect is enabled instead of using fixed
     * XO settle time.
     */
    uint32_t aon_top = NT_REG_RD(QWLAN_PMU_AON_TOP_CFG_REG);
    aon_top |= QWLAN_PMU_AON_TOP_CFG_CFG_SLP_CLK_SWITCHING_EN_MASK;
    NT_REG_WR(QWLAN_PMU_AON_TOP_CFG_REG, aon_top);
#endif /* POWER_SLP_CLK_SWITCH_WAR */

    g_socpm_struct.rmc_system_status = NT_REG_RD(QWLAN_PMU_SYSTEM_STATUS_REG);
    /*read once will clear this register, save the value in a local*/
    warm_boot_sts = g_socpm_struct.rmc_system_status & QWLAN_PMU_SYSTEM_STATUS_WARM_BOOT_FROM_SLEEP_MASK;

#if defined(PLATFORM_FERMION) && !defined(EMULATION_BUILD)
    if (warm_boot_sts == QWLAN_PMU_SYSTEM_STATUS_WARM_BOOT_FROM_SLEEP_MASK) {
        wifi_fw_cpr_reenable();
    }
#endif /* defined(PLATFORM_FERMION) && !defined(EMULATION_BUILD) */

    dtim_tv_monitor_trigger();

    // to enable any floating point operation in minimal code
    _min_enable_vfp();
    // in minimum, should not process uart rx
    process_uart_rx_irq = 0;
    _MIN_UART_INIT();
    /* Enable fault */
    NT_SOCPM_FAULT_ENABLE();
#ifdef NT_DEBUG
    if (g_socpm_struct.rmc_fault_force) {
        /* Forced Fault Enabled. Try to dereference a NULL ptr */
        /* int* ptr = NULL; */
        /* *ptr = 1; */
    }
#endif /* NT_DEBUG */
    TIMER_INIT_HW();

#if defined(COMPENSATE_RC_DIVISION_ERROR_WAR)
    delta_aon = (uint32_t)nt_socpm_get_slp_tmr_us();
    delta_glb = (uint32_t)((uint32_t)hres_timer_curr_time_us() - (uint32_t)  g_socpm_struct.glb_pre_sleep_time_us);

    if(delta_glb > delta_aon)
    {
        compensation_hre = delta_glb - delta_aon ;
        timer_cvt_to_tick64(compensation_hre, T_USEC, &hres_tick_after_comp);
        NT_REG_WR(QWLAN_PMU_CFG_GLB_TMR_MSB_REG, (uint32_t)((hres_timer_timetick_get() - hres_tick_after_comp) >> 32));
        NT_REG_WR(QWLAN_PMU_CFG_GLB_TMR_LSB_REG, (uint32_t)(hres_timer_timetick_get()-hres_tick_after_comp));  
    }
    else
    {
        compensation_hre = delta_aon - delta_glb;
        timer_cvt_to_tick64(compensation_hre, T_USEC, &hres_tick_after_comp);
        NT_REG_WR(QWLAN_PMU_CFG_GLB_TMR_MSB_REG, (uint32_t)((hres_timer_timetick_get() + hres_tick_after_comp) >> 32));
        NT_REG_WR(QWLAN_PMU_CFG_GLB_TMR_LSB_REG, (uint32_t)(hres_timer_timetick_get()+ hres_tick_after_comp));  
    }
#endif /* COMPENSATE_RC_DIVISION_ERROR_WAR */

#ifndef PLATFORM_NT
    /* Reset the PSS */
    nt_gpio_init();
#ifdef IMAGE_FERMION
    wifi_fw_gpio_init(TRUE);
#endif /* IMAGE_FERMION */
#endif /* PLATFORM_NT */
#ifdef GPIO_RETENTION_IN_SLP
    /* Disable the GPIO retension */
    NT_REG_WR(QWLAN_PMU_CFG_IO_RET_CNTL_REG, QWLAN_PMU_CFG_IO_RET_CNTL_DEFAULT);
#endif /* GPIO_RETENTION_IN_SLP */

#if defined(SUPPORT_HDM_INITIATED_RRI)
    /* HDM module in HW can be programmed to do RRI in parallel with CPU reset
     * check if HDM has completed the RRI and disable the HDM as soon as possible
     * DONOT move this code down as it's critical to disable HDM as it might put the MAC
     * back to sleep based on certain configurations*/

    PM_STRUCT *pPmStruct = (PM_STRUCT *)gdevp->pPmStruct;
    uint32_t hdm_config;
    uint8_t hw_rri_started = FALSE;
    slp_tmr_sts = NT_REG_RD(QWLAN_PMU_WLAN_SLP_TMR_STS_REG);

    if ((pPmStruct->hdm_triggered_rri_enable) && (warm_boot_sts == QWLAN_PMU_SYSTEM_STATUS_WARM_BOOT_FROM_SLEEP_MASK) &&
        (slp_tmr_sts & QWLAN_PMU_WLAN_SLP_TMR_STS_WLAN_SLP_TMR_INT_RAW_MASK)) {
        hw_rri_started = (nt_hal_rri_check_restore_started(pPmStruct->hdm_triggered_rri_list) ||
                          nt_hal_rri_check_restore_complete(pPmStruct->hdm_triggered_rri_list));

        pPmStruct->hdm_triggered_rri_in_progress = (hw_rri_started == TRUE) ? TRUE : FALSE;
        if (!pPmStruct->hdm_triggered_rri_in_progress) {
            pPmStruct->hdm_triggered_rri_fail_count++;
            pPmStruct->hdm_triggered_rri_no_start_count++;
        }
    }

    /*even if its an A2F wake up, disable HDM, RRI needs to be done when the wake lock is
    held or AON timer expires*/
    hdm_config = HAL_REG_RD(QWLAN_MTU_MTU_HDM_CONFIG_REG);
    hdm_config &= (~QWLAN_MTU_MTU_HDM_CONFIG_HW_DTIM_ENABLE_MASK);
    HAL_REG_WR(QWLAN_MTU_MTU_HDM_CONFIG_REG, hdm_config);
#endif /*SUPPORT_HDM_INITIATED_RRI*/

    FDI_RMC_INS_START_NULL(FDI_DBG_PWR_S2W_WARM_BOOT_CB);

    _MIN_CPU_TMR_INIT();

    g_socpm_struct.in_warm_boot = TRUE;

#ifdef CONFIG_WATCH_DOG_ENABLE
    nt_watchdog_bark_timer_reset();
#endif

    if (warm_boot_sts == QWLAN_PMU_SYSTEM_STATUS_WARM_BOOT_FROM_SLEEP_MASK
#ifdef FEATURE_FERMION_SLP_DBG
        || g_socpm_struct.socpm_mcu_sleep_dbg_mode /* If SON is ON for debug then warmboot status fails */
#endif                                             /* FEATURE_FERMION_SLP_DBG */
    ) {
        uint64_t wkup_us;
        uint8_t test_f = 0;
        uint8_t sleep_clk_sel;

#if !defined(EMULATION_BUILD)
        sleep_clk_sel = ((PM_STRUCT *)gdevp->pPmStruct)->slp_clk_sel;
#ifndef FORCE_BBPLL_LOCK
        /* BBPLL LOCK only for RFAXO_CLK */
        if (sleep_clk_sel != NT_SOCPM_SLP_CLK_RFAXO) {
            test_f = _MIN_BBPLL_LOCK();
        }
#else
        /* Force BBPLL LOCK for each clock configs */
        test_f = _MIN_BBPLL_LOCK();
#endif  /* FORCE_BBPLL_LOCK */
#endif  // !defined(EMULATION_BUILD)

#if !defined(IMAGE_FERMION)
        nt_socpm_glob_restore();
#endif               // !defined(IMAGE_FERMION)
        if (test_f)  // lock failure
        {
            _minprintf("*E*", warm_boot_sts, sleep_clk_sel);
            test_f = 0;
        }
#ifdef SOCPM_RMC_DBG
        /* Print showing entry of RMC */
        UART_Send_direct("R", 1);
#endif /* SOCPM_RMC_DBG */
        presleep_update_ulpsmps2_oneshot();
    slp_switch:
        slp_tmr_sts = NT_REG_RD(QWLAN_PMU_WLAN_SLP_TMR_STS_REG);
#ifdef COMPENSATE_AON_PROG_DELAY
        /* Apply correction post wake up from deep sleep */
        /* Total sleep time = AON measured sleep time + AON programming time + US2MS error carried forward */
        uint32_t nt_socpm_slp_time_us =
            (nt_socpm_get_slp_tmr_us() + g_socpm_struct.aon_program_time_us + g_socpm_struct.unapplied_err_us);
        nt_socpm_slp_time_total += US_TO_MS(nt_socpm_slp_time_us);
        g_socpm_struct.unapplied_err_us = nt_socpm_slp_time_us % 1000;
#else  /* COMPENSATE_AON_PROG_DELAY */
        nt_socpm_slp_time_total += US_TO_MS(nt_socpm_get_slp_tmr_us());
#endif /* COMPENSATE_AON_PROG_DELAY */
#ifdef FIRMWARE_APPS_INFORMED_WAKE
        uint32_t lic_int_status = NT_REG_RD(QWLAN_PMU_AON_LIC_INT_STAT_REG);
        if (lic_int_status & QWLAN_PMU_AON_LIC_INT_STAT_EXT_WAKEUP_INTR_STAT_RAW_MASK) {
            // Perform a full wake up if A2F was asserted
            uint64_t delta_time_us;
            delta_time_us = nt_socpm_get_slp_tmr_us();
            PM_SET_SLEEP_EXIT_REASON(pPmStruct, EXIT_REASON_EXT_INT);
            extern int BMPS_LIST;
            extern int _socpm_slp_list_idx_imps;
            extern int _socpm_slp_list_idx_rtos;
            nt_socpm_sleep_deregister(_socpm_slp_list_idx_rtos);
            nt_socpm_sleep_deregister(BMPS_LIST);
            nt_socpm_sleep_deregister(_socpm_slp_list_idx_imps);
            printf("intr only\r\n");
            wkup_us = 0;
        } else {
#endif /* FIRMWARE_APPS_INFORMED_WAKE */
            wkup_us = (slp_tmr_sts & QWLAN_PMU_WLAN_SLP_TMR_STS_WLAN_SLP_TMR_INT_RAW_MASK)
                          ? nt_socpm_min_proc(&process_routine)
                          : 0;
#ifdef FIRMWARE_APPS_INFORMED_WAKE
        }
        #endif /* FIRMWARE_APPS_INFORMED_WAKE */
        if(pPmStruct->pm_statistics.bmps_stats.period_to_record > 0){
            pm_save_active_sleep_time_record(gdevp, nt_socpm_slp_time_us, pPmStruct->pm_statistics.bmps_beacon_wait_time, BWINDOW_WAIT_CLOSE_TIME);
        }

        if (test_f) {
            _minprintf("TesT", wkup_us >> 32, (unsigned int)wkup_us);
        }

#ifdef SOCPM_SLEEP_DEBUG
        uint32_t d1 = 0;
        uint32_t d2 = wkup_us;
        uint32_t d3 = (slp_tmr_sts & 0x3) | ((test_f << 2) & 0xC) |
#ifdef FIRMWARE_APPS_INFORMED_WAKE
                      (((lic_int_status >> QWLAN_PMU_AON_LIC_INT_STAT_WLAN_WAKEUP_INTR_STAT_RAW_OFFSET) << 4) & 0x10) |
                      (((lic_int_status >> QWLAN_PMU_AON_LIC_INT_STAT_WLAN_WAKEUP_INTR_STAT_OFFSET) << 5) & 0x20) |
                      (((lic_int_status >> QWLAN_PMU_AON_LIC_INT_STAT_EXT_WAKEUP_INTR_STAT_RAW_OFFSET) << 6) & 0x40) |
#endif
                      0;
        socpm_log_timestamp(___MIN_VEC, d1, d2, d3);
#endif

        /* Set all IRQ to reset for both Sleepback and wakeup path*/
        nt_global_irq_init();

        FDI_RMC_INS_STOP_NULL(FDI_DBG_PWR_S2W_WARM_BOOT_CB);
        if (wkup_us > 0) {
            dtim_tv_monitor_poll();
#ifdef SOCPM_RMC_DBG
            /* Print when decides to sleep back */
            UART_Send_direct("S\r\n", 3);
#endif /* SOCPM_RMC_DBG */
            FDI_RMC_INS_START_NULL(FDI_DBG_PWR_S2W_WARM_BOOT_CB_SLEEP);

            if (wkup_us > 0x20000) {
                _minprintf("WK", (uint32_t)wkup_us, slp_tmr_sts);
            }
#if RMC_DISABLED_CODE
            __asm volatile(" ldr r1,=load_r13              \n");
            __asm volatile(" ldr r4,[r1] \n");
#endif /* if RMC_DISABLED_CODE */
            process_routine = 1;
            __asm volatile(" nop  \n");
            // should place before NT_CM4_NVIC_ISER0_CLEAR_PENDING_REG since interrupt may happened after clear if not.
            cpu_irq_disable();
#if RMC_DISABLED_CODE
            _minprintf("wake", wkup_us, nt_socpm_slp_time_total);
#endif /* if RMC_DISABLED_CODE */
            NT_REG_WR(NT_CM4_NVIC_ISER0_CLEAR_PENDING_REG, NT_CM4_UART_INTERRUPT_BIT_MASK);
#if RMC_DISABLED_CODE
            NT_SOCPM_IRQ_ENABLE();
#endif /* if RMC_DISABLED_CODE */
            /* Going back to sleep service no more IRQs*/
            cpu_irq_disable();
            lic_int_status = NT_REG_RD(QWLAN_PMU_AON_LIC_INT_STAT_REG);
            uint32_t ext_int = NT_REG_RD(NVIC_ICPR1) ;
            if ((lic_int_status & QWLAN_PMU_AON_LIC_INT_STAT_EXT_WAKEUP_INTR_STAT_RAW_MASK )|| (ext_int& (A2F_ASSERT_INTR_NVIC1_MASK))) {
                    printf("intr before wfi\r\n");
                    wkup_us=0;
            }else
            {
                nt_socpm_slp_enter(wkup_us);
                wkup_us =0;
                
            }
   
        } 
        
        if(wkup_us<=0){
            uint32_t bd= BMU_READ_WQ_NR_CMD(HAL_BMUWQ_BMU_IDLE_BD);
            uint32_t ext_int = NT_REG_RD(NVIC_ICPR1) ;
            if ((lic_int_status & QWLAN_PMU_AON_LIC_INT_STAT_EXT_WAKEUP_INTR_STAT_RAW_MASK )|| (ext_int& (A2F_ASSERT_INTR_NVIC1_MASK)))
            {
                HAL_REG_WR(QWLAN_AGC_AGC_RESET_REG, QWLAN_AGC_AGC_RESET_RESET_ERESET);
                hal_wlan_sleep_trimmed();
            }
#ifdef SOCPM_RMC_DBG
            /* Print when decides to wake up */
            UART_Send_direct("W\r\n", 3);
            g_socpm_struct.full_wake_stats++;
#endif /* SOCPM_RMC_DBG */
            FDI_RMC_INS_START_NULL(FDI_DBG_PWR_S2W_WARM_BOOT_CB_WAKE);
#if RMC_DISABLED_CODE
            /* turn on XIP */
            uint32_t regval = NT_REG_RD(QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_REG);
            NT_REG_WR(QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_REG,
                      regval | QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_PD_XIP_CNTL_BIT_MASK);
            nt_socpm_nop_delay(500);
#endif /* if RMC_DISABLED_CODE */

            NT_REG_WR(CACHE_REG_BASE, 0x01);
            NT_REG_WR(CACHE_REG_BASE, 0x00);
#if RMC_DISABLED_CODE
            __asm volatile(" ldr r1,=load_r13              \n");
            __asm volatile(" ldr r4,[r1] \n");
            uint32_t son_value = 0;
            son_value = NT_REG_RD(QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_REG);
            son_value |= QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_PD_SON_CNTL_BIT_MASK;
            NT_REG_WR(QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_REG, son_value);
#endif /* if RMC_DISABLED_CODE */
            process_routine = 0;

            cpu_irq_disable();
            /**
            * Workaround to address incorrect __stack_ptr value after BMPS.
            * Upon warm boot, VTOR points to __stack_ptr instead of __isr_vectors.
            * The DXE Interrupt Handler of channel 3 is wrong,
            * which may cause a crash after VO packet transmission.
            * To prevent this, VTOR is explicitly set to __isr_vectors to ensure
            * correct interrupt handling after wake-up.
            */
            SCB->VTOR = (uint32_t)(&__vectors_start);
            portENABLE_INTERRUPTS(); /* Sets the BASEPRI to 0x00*/

            nt_socpm_resume_f = 0;

            portNVIC_SYSPRI2_REG |= portNVIC_PENDSV_PRI;
            portNVIC_SYSPRI2_REG |= portNVIC_SYSTICK_PRI;

            NT_REG_WR(NT_CM4_NVIC_ISER0_REG, nt_socpm_m4_regs[11]);
            NT_REG_WR(NT_CM4_NVIC_ISER1_REG, nt_socpm_m4_regs[12]);
            NT_REG_WR(NT_CM4_NVIC_ISER2_REG, nt_socpm_m4_regs[13]);
#ifdef PLATFORM_FERMION
            NT_REG_WR(NT_CM4_NVIC_ISER3_REG, nt_socpm_m4_regs[14]);
#endif /* PLATFORM_FERMION */

#ifdef NT_CC_DEBUG_FLAG
            NT_REG_WR(QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_REG, 0xffdf);
            nt_socpm_footsw_state_set(1);
#endif
            FDI_RMC_INS_STOP_NULL(FDI_DBG_PWR_S2W_WARM_BOOT_CB_WAKE);

            _minprintf("S", (unsigned int)nt_socpm_slp_time_total, 0);

            // Initialize QCSPI on full wakeup
#ifdef SUPPORT_QCSPI_SLAVE
            qcspi_slv_init();
#endif /* SUPPORT_QCSPI_SLAVE */

            g_socpm_struct.in_warm_boot = FALSE;
            // will full wake, so start to process uart rx
            process_uart_rx_irq = 1;

#ifdef WAR_RESTORE_DPU_DEFAULT_WQ_12_ON_EXIT_FROM_BMPS
            nt_dpm_restore_dpu_default_wq_routing_post_wakeup();
#endif

            HAL_REG_WR(QWLAN_BMU_BTQM_PACKET_MEMORY_UNIT_SIZE_REG, 1);
            /*restoring saved context*/
            nt_socpm_ctxt_restore();
            // should never get here
            _minprintf("****", (unsigned int)nt_socpm_slp_time_total, 0);
        }
    }
#if RMC_DISABLED_CODE
    _minprintf("!!!!", nt_socpm_slp_time_total, 0);

    Should never reach this should have already performed a reset.
#endif /* #if RMC_DISABLED_CODE*/
        mcu_sleep_force = 0;
    _start();

    for (;;)
        ;
}

static void _min_enable_vfp(void)
{
    __asm volatile(
        " ldr.w r0, =0xE000ED88 \n" /* The FPU enable bits are in the CPACR. */
        " ldr r1, [r0] \n"
        " \n"
        " orr r1, r1, #( 0xf << 20 ) \n" /* Enable CP10 and CP11 coprocessors, then save back. */
        " str r1, [r0] \n"
        " bx r14 ");
}
