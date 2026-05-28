/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

// -------------------------------------------------------------------
#include "FreeRTOS.h"
#include "task.h"
#include "uart.h"
#include "ExceptionHandlers.h"
#include "wifi_cmn.h"
#include "fwconfig_cmn.h"
#include "nt_flags.h"

#include "nt_common.h"
#include "nt_logger_api.h"
#include "nt_devcfg.h"
#include "nt_socpm_rtos_api.h"
#include "nt_socpm_sleep.h"

#include "nt_hw.h"
#include "nt_hw_support.h"
#include "qurt_internal.h"

#include "wlan_power.h"
#include "hal_int_powersave.h"
//#include "nt_wfm_wmi_interface.h"
//#include "wifi_app.h"

#include "timer.h"
#include "wifi_fw_ext_intr.h"

#ifdef NT_GPIO_FLAG
#include "nt_gpio_api.h"
#endif
#include "nt_wdt_api.h"

#include "wps_def.h"

#ifdef NT_FN_CPR
#include "nt_cpr_driver.h"
#endif  // NT_FN_CPR

#include "qtmr.h"
#ifdef IMAGE_FERMION
#include "wifi_fw_internal_api.h"
#include "mlme_api.h"
#endif /*IMAGE_FERMION*/

#ifdef PLATFORM_FERMION
#include "HALhwio.h"
#include "Fermion_seq_hwioreg.h"
#endif /* PLATFORM_FERMION */

#include "fdi_rmc.h"
#include "wifi_fw_pwr_cb_infra.h"
#include "wifi_fw_pmic_driver.h"
#include "wifi_fw_cpr_driver.h"

#ifdef SUPPORT_COEX
#include "coex_utils.h"
#include "coex_test.h"
#endif

#include "wlan_power.h"
#include "nt_imps.h"

#ifdef SLEEP_CLK_CAL_IN_ACTIVE_MODE
#include "wlan_sleep_clk_cal.h"
#endif /* SLEEP_CLK_CAL_IN_ACTIVE_MODE */
#include "wifi_fw_pmu_ts_cfg.h"
#include "ferm_prof.h"

#ifdef SUPPORT_RING_IF
extern wmi_msg_struct_t g_Cmd_Translation_wifi_hndl;
#endif
extern ppm_common_t g_ppm_common_struct;
extern GPIO_Config_t gpio_config;
#ifdef CONFIG_BOARD_QCC730_QSPI_ENABLE
#include "ferm_flash.h"
#endif

extern int rri_force_wakeup;

// -------------------------------------------------------------------
// local fns control

// sw maintains tsf through sleep - still under test
// NOTE: will need to change hal powersave also to avoid MTU reset (see local flags in hal)
// #define _SOCPM_INC_FN_TIMESYNC

// TEMP: use standby instead of mcu sleep
// #define _SOCPM_INC_TST_MCUSLP_SBY
// #define _SOCPM_INC_TST_FORCE_SLP_CLK_SRC
// #define _SOCPM_INC_TST_FORCE_SLP_CLK_SRC_VAL   NT_SOCPM_SLP_CLK_RC
// #define _SOCPM_INC_TST_FORCE_SLP_CLK_SRC_VAL   NT_SOCPM_SLP_CLK_PMICXO

// use modified MCU act state resource settings
#define _SOCPM_INC_TST_MCU_ACT_CHG

// remove RFA retention via gdscr, undefining this keeps rfa ff in retention
// noticed problems with this, so don't turn disable rfa ret for now
// #define _SOCPM_INC_TST_RFARET_DIS

// control setup of RRT deepsleep states during socpm init instead of at deepsleep state entry
// defined => init time, undefined => init dpslp rrt at dpslp entry time
// #define _SOCPM_INC_TST_RRT_INIT

// controls agc and rxp disable before entering sleep
// since wifi is moved to sleep state anyway, this may not be needed
// #define _SOCPM_INC_TST_SLP_PHY_RXP_RST

// -------------------------------------------------------------------
// Local defs
// -------------------------------------------------------------------

// enable code to count number of register writes and stop after max
#define _SOCPM_INC_MCUSLP_WREG_COUNT
#define _SOCPM_REGWR_MAX                  2
#define _SOCPM_REGWR_RESET_LIMIT          200  // after this many "skips", start writing regs again
#define _SOCPM_AON_RESET_TIMEOUT_US       500
#define _SOCPM_AON_TMR_INT_CLR_TIMEOUT_US 3000

// below configuration is a part devcfg
#define _SOCPM_MAX_ACTIVE    25
#define _SOCPM_MAX_CLK_SLEEP 0xFFFFFFFFUL
#define _SOCPM_MAX_SLEEP     0xFFFFFFFFFFUL
#define _SOCPM_MAX_STANDBY   0x2FFFFFFFFFFFFFUL

// time from aon timer expires to nt_socpm_restore function being called
// includes h/w overheads: 3-4ms, bbpll lock, uart init, etc
#define _SOCPM_PS_WAKE_HW_OVERHEADS_US 4500

#define _SOCPM_SLP_TMR_LSB (*((volatile uint32_t *)(QWLAN_PMU_SLP_TMR_VAL_LSB_REG)))
#define _SOCPM_SLP_TMR_MSB (*((volatile uint32_t *)(QWLAN_PMU_SLP_TMR_VAL_MSB_REG)))

#define _SOCPM_CPU_SYS_CTL_REG 0xE000ED10

// OTP regoin address for flags to control application behaviour
#define _SOCPM_OTP_FLAGS_ADDR 0x801CE

// Debug Purpose Only
#define _SOCPM_MBANK_D_CHK 0x7FFF0  // Define CMEM BANK D Address
#define _SOCPM_MBANK_C_CHK 0x5FFF0  // Define CMEM BANK C Address
#define _SOCPM_MBANK_B_CHK 0x3FFF0  // Define CMEM BANK B Address
#define _SOCPM_MBANK_A_CHK 0x1FFF0  // Define CMEM BANK A Address​

// Read And Set Bits​
#define _SOCPM_REG_RW(reg_addr, data) \
    (*((volatile uint32_t *)(reg_addr))) = ((*((volatile uint32_t *)(reg_addr))) | ((uint32_t)(data)))
// Read And Clear Bits
#define _SOCPM_REG_RW_CLR(reg_addr, data) \
    (*((volatile uint32_t *)(reg_addr))) = ((*((volatile uint32_t *)(reg_addr))) & (~((uint32_t)(data))))

// MEM BAnk Types
#define _SOCPM_MBANK_A 1
#define _SOCPM_MBANK_B 2
#define _SOCPM_MBANK_C 3
#define _SOCPM_MBANK_D 4

// Divide SYSTICK by 60 to get us since SYSTICK frequency is 60MHz
#define GET_CURRENT_SYSTICK_US() (portNVIC_SYSTICK_CURRENT_VALUE_REG / 60)

#define _SOCPM_SLP_LST_IDX_INVALID -1  // Invalid index

#define _SOCPM_EXT_INT_CLR_REG  (2)
#define _SOCPM_EXT_INT_CLR_NVIC (31)

#define _SOCPM_IS_BIT_SET(value, pos) (value & (1 << pos))

#ifdef SUPPORT_TWT_STA
#define MINIMUM_SLP_TIME_FOR_AON (16)  // Time in ms
#endif

#ifdef PLATFORM_NT
#define NT_PMU_DIG_TOP_CFG_RRAM_PD_MODE_NAP_MASK 0x800
#endif /* PLATFORM_NT */

#define _PMU_CFG_AON_CNTL_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_MEM_MX_CNTL_BIT_MEM_NOR_MASK 0x10

/* Constants required to pend a PendSV interrupt from the tick ISR if the
 preemptive scheduler is being used.  These are just standard bits and registers
 within the Cortex-M core itself. */
/* Constants required to manipulate the core.  Registers first... */

// -------------------------------------------------------------------
// Debug/test purposes code/defs
// -------------------------------------------------------------------

// config to call SW standby routine and sleep from nt_socpm_tst_pmic_cfg
// set mode = 0 to exclude the call from nt_socpm_tst_pmic_cfg (called from normal sby procedure instead)
//            1 to include call from nt_socpm_tst_pmic_cfg
//            2 to include "nt_socpm_tst_standby" but not call it from nt_socpm_tst_pmic_cfg
#define _SOCPM_TST_MODE_PMIC_CFG_SBY 2

#define _SOCPM_TST_SBY_SLP_CAL_DIS  // include sleep clock cal disable in pmic init

#if _SOCPM_TST_MODE_PMIC_CFG_SBY != 0
#define _SOCPM_INC_TST_SLEEP  // control to include a local enter-into-wfi function
#endif

#define CACHE_REG_BASE       0x01180000
#define portNVIC_PENDSV_PRI  (((uint32_t)configKERNEL_INTERRUPT_PRIORITY) << 16UL)
#define portNVIC_SYSTICK_PRI (((uint32_t)configKERNEL_INTERRUPT_PRIORITY) << 24UL)

#ifdef FEATURE_FDI
#define FDI_AON_ISR_EN (FDI_RESET)
#endif

#ifdef _SOCPM_INC_TST_SLEEP
static void _tst_sleep_enter(void);
#endif

// -------------------------------------------------------------------
// Local protos
// -------------------------------------------------------------------

static void _socpm_slp_fn_process(sleep_mode mode);
static void _socpm_list_search(void);
static void _socpm_slpcfg_sby(void);
static void _socpm_slpcfg_mcuslp(void);
static void socpm_enter_deepsleep();

#if defined(PLATFORM_FERMION)
static void _socpm_slpcfg_light(void);
#endif /*PLATFORM_FERMION*/

static void _socpm_ctxt_save(void) __attribute__((naked));
static void _socpm_mem_wr_drv(uint32_t Control_Reg, uint32_t Status_Reg, uint32_t Resource_Reg, uint32_t Resource_Value,
                              Mem_Control type);

#if defined(SUPPORT_SOC_SLEEP_SOLVER) || defined(SUPPORT_SLEEP_LIST_IMPROVEMENTS)
static sleep_mode nt_socpm_sleep_solver(uint64_t sleep_time);
#endif /* SUPPORT_SOC_SLEEP_SOLVER SUPPORT_SLEEP_LIST_IMPROVEMENTS*/

// static void _socpm_sleep_lst_reorder(int modified, int slp_mode_f);

// static sleep_mode _socpm_slpmode_get(uint64_t slp_time);
static uint64_t _socpm_get_sleep_slop_adjusted_sleep_time(uint64_t sleep_time_us);

void _socpm_slptmr_off(void);
static uint64_t nt_socpm_slp_tmr_get(void);

extern uint8_t get_warmboot_status(void);
extern uint32_t get_rmc_system_status(void);
#ifdef _SOCPM_INC_FN_TIMESYNC
struct _socpm_glob_s {
    uint32_t mtu_glob_tmr;
    uint64_t tsf_us;
    uint64_t aon_slp_tmr_us;
} _socpm_glob_data;
#endif

/*****************************************/

// SOCPM STATIC LIST
typedef struct _socpm_wkup_inp_list_s {
    nt_socpm_sleep_t slp_info;
    struct _socpm_wkup_inp_list_s *next;
} _socpm_slp_lst_wkup_item_t;

// for debug only...
void _minprintf(char *str, unsigned int a1, unsigned int a2);

// -------------------------------------------------------------------
// Local/glob vars
// -------------------------------------------------------------------

static void _socpm_slp_dflt_cb(void);
static void _socpm_wkup_dflt_cb(soc_wkup_reason wkup_reason);

// Sleep Register List for freertos timers
static nt_socpm_sleep_t _socpm_os_tmr = {.slp_cb_fn = _socpm_slp_dflt_cb,
                                         .wkup_cb_fn = _socpm_wkup_dflt_cb,
                                         .min_cb_fn = freertosdefaultminimum,
                                         .slp_mode = clk_gtd_sleep,
                                         .slp_time = 0x7FFFFFFF,
                                         .list_no = -1,
                                         .start_time_us = 0};

//@TO-DO need to cross check the lock mechanism
// static int sleep_lock =0;

#define _SOCPM_SLP_LST_SZ   10  // #elements in the sleep function list
#define _INVALID_SLP_LST_HD -1  // Indicates that list is empty and _socpm_slp_lst_head is invalid

int g_sleep_failed = false;

static _socpm_slp_lst_wkup_item_t _socpm_slp_lst[_SOCPM_SLP_LST_SZ];
static int _socpm_slp_lst_head;  // head index

static uint8_t _socpm_slp_exit;
static int _socpm_last_slp_count;

static uint8_t _socpm_mcu_sleep_wake;

static uint8_t _socpm_slp_clk_src;
static uint8_t _socpm_slp_time_supp_min_ms;

// variables  for Silent app
static uint8_t _socpm_rram_ctl_f;

volatile int nt_socpm_resume_f;
// Variable to store stack pointer of current task
volatile uint32_t nt_socpm_m4_regs[15];
volatile uint32_t nt_socpm_slp_time_total;
uint32_t nt_socpm_slp_time_min;
uint32_t nt_socpm_slp_time_sby;
int nt_socpm_sby_force = 0;

// xo settle timeout
uint8_t xo_settle_time = 66;
uint8_t xo_trim_time = 32;
uint8_t son_en_wait_mcu = 0x1;
uint8_t son_en_wait_light = 0xf;
uint8_t son_en_wait_sby = 0xf;
// aon sm delay
uint8_t mx_settle_time = 14;
uint8_t p8v_smps_settle_time = 32;
uint8_t pmic_slp_exit_time = 26;
uint8_t pmic_slp_entry_time = 32;

// Sleep HW delay, fixed part
uint32_t slp_exit_hw_delay_fixed = 73;
uint32_t cpu_boot_bcn_rx_delay = 0;

uint64_t hres_time_pre_sleep;
/*
 * The tick interrupt is generated by the asynchronous timer.  The default tick
 * interrupt handler cannot be used (even with the Sleep timer being handled from the
 * tick hook function) because the default tick interrupt accesses the SysTick
 * registers when configUSE_TICKLESS_IDLE set to 1.  Sleep timer_ALARM_Handler() is the
 * default name for the Sleep timer alarm interrupt.  This definition overrides the
 * default implementation that is weakly defined in the interrupt vector table
 * file.
 */
/* Calculate how many clock increments make up a single tick period. */
// static const uint32_t ulAlarmValueForOneTick = ( 15000000 / configTICK_RATE_HZ );
/* Holds the maximum number of ticks that can be suppressed - which is
 basically how far into the future an interrupt can be generated. Set
 during initialisation. */
TickType_t xMaximumPossibleSuppressedTicks = 0xFFFFFFF;  // maximum 54 bits configurable sleep timer

/* The Sleep timer counter is stopped temporarily each time it is re-programmed.  The
 following variable offsets the Sleep timer counter alarm value by the number of Sleep timer
 counts that would typically be missed while the counter was stopped to compensate
 for the lost time.  _RB_ Value needs calculating correctly. */
// static uint32_t ulStoppedTimerCompensation = 2 / ( 1000 / 16000000 );

// Sleep type for neutrino
static sleep_mode _socpm_slp_mode;

// Variables for Holding sleep time
 int _socpm_slp_list_idx_rtos = _SOCPM_SLP_LST_IDX_INVALID;

#if defined(EMULATION_BUILD) && defined(PLATFORM_FERMION)
// Variable for sleep clock scaling on emulation
uint8_t _slp_clk_scaling = 1;
#endif  // defined(EMULATION_BUILD) && defined(PLATFORM_FERMION)

// Variables for external interrupt
#ifndef PLATFORM_FERMION
wapp_msg_struct_t ps_external_int_config;
#endif
SOCPM_STRUCT g_socpm_struct;

#ifndef NT_HOSTED_SDK
extern qurt_pipe_t wifiapp_queue_handle;
#else
extern qurt_pipe_t x_spiQueueHandle;
#endif

#ifndef PLATFORM_FERMION
#ifdef NT_FN_WPS
TimerHandle_t ext_int_timer = NULL;
static uint8_t _socpm_tmr_flag = 0;
volatile uint32_t active_ext_int = 0;
extern qurt_pipe_t msg_wfm_wmi_id;

#ifdef NT_HOSTLESS_SDK
extern WPS_CONTEXT *pg_wps;
static wapp_msg_struct_t networkConf;
#endif  // NT_HOSTLESS_SDK
#endif  // NT_FN_WPS
#endif  // ifndef PLATFORM_FERMION

// extern uint32_t *cli_msg_id_temp;
// extern uint32_t *bss_addr;
extern int process_routine;

#ifdef SOCPM_SLEEP_DEBUG
uint32_t socpm_dbg_log_idx;
uint32_t first_bmps_slp_time;
uint32_t second_bmps_slp_time;
uint32_t bcn_nowake_limit;
uint8_t socpm_bmps_glance;
struct socpm_dbg_ts socpm_dbg_log[MAX_LOG_ENTRY_PM];

uint8_t get_slp_lst_cnt()
{
    uint8_t cnt = 0;
    _socpm_slp_lst_wkup_item_t *p_next, *p_curr;
    p_next = &_socpm_slp_lst[_socpm_slp_lst_head];

    if (_socpm_slp_lst_head == _INVALID_SLP_LST_HD)
        return 0;

    while (p_next != NULL) {
        p_curr = p_next;
        p_next = p_curr->next;
        cnt++;
    }

    return cnt;
}

void socpm_log_timestamp(SOCPM_DBG_TIMING proc, uint32_t d1, uint32_t d2, uint32_t d3)
{
    socpm_dbg_log[socpm_dbg_log_idx].proc = proc;
    socpm_dbg_log[socpm_dbg_log_idx].ts = hres_timer_curr_time_us();
    socpm_dbg_log[socpm_dbg_log_idx].tk = NT_REG_RD(QWLAN_PMU_SLP_TMR_VAL_LSB_REG);
    socpm_dbg_log[socpm_dbg_log_idx].d1 = d1;
    socpm_dbg_log[socpm_dbg_log_idx].d2 = d2;
    socpm_dbg_log[socpm_dbg_log_idx].d3 = d3;

    socpm_dbg_log[socpm_dbg_log_idx].ms =
        (_socpm_slp_lst_head & 0xf) | ((get_slp_lst_cnt() << 4) & 0xf0) | ((_socpm_slp_mode << 8) & 0xf00);
    if (_socpm_slp_lst_head != _INVALID_SLP_LST_HD)
        socpm_dbg_log[socpm_dbg_log_idx].slp_1 = (_socpm_slp_lst[_socpm_slp_lst_head].slp_info.slp_time);
    if (get_slp_lst_cnt() > 1)
        socpm_dbg_log[socpm_dbg_log_idx].slp_2 = _socpm_slp_lst[_socpm_slp_lst_head].next->slp_info.slp_time;

    socpm_dbg_log_idx = (socpm_dbg_log_idx + 1) % MAX_LOG_ENTRY_PM;
}
#endif

uint32_t get_son_en_wait_ticks(uint32_t son_en_wait)
{
    uint32_t wait_ticks = 0, chip_ver = 2, otp_ver_high = 1;
    son_en_wait = son_en_wait & 0xF;

#if (FERMION_CHIP_VERSION == 1)
    chip_ver = 1;
#elif (FERMION_CHIP_VERSION == 2)
    chip_ver = 2;
    otp_ver_high = HWIO_INXF(SEQ_WCSS_OTP_OFFSET,
                             FERMION_V2_0_QFPROM_RAW_FUSE_MAP_SECURITY_CONTROL_CORE_RAW_R_QFPROM_RAW_PTE_REGION_1_W3,
                             TRIM_TAG_HIGH);
#endif

    if (chip_ver == 2 && otp_ver_high >= 1) {
        if (son_en_wait <= 4)
            wait_ticks = 0;
        else if (son_en_wait <= 9)
            wait_ticks = 1 + (son_en_wait >= 7) ? (son_en_wait - 7) : 0;
        else if (son_en_wait == 10)
            wait_ticks = 7;
        else
            wait_ticks = 13;
    } else {
        if (son_en_wait <= 9)
            wait_ticks = 0;
        else if (son_en_wait == 10)
            wait_ticks = 4;
        else
            wait_ticks = 9;
    }

    return wait_ticks;
}

uint32_t get_sleep_exit_hw_delay(sleep_mode slp_mode)
{
    uint32_t delay = 2000, son_en_wait = 0;

    if (slp_mode == mcu_sleep) {
        son_en_wait = get_son_en_wait_ticks(son_en_wait_mcu);
        delay =
            ((slp_exit_hw_delay_fixed * 2 + xo_settle_time * 4 + pmic_slp_exit_time + son_en_wait * 2) * 15625) / 1024;
    }

#ifdef APPLY_SLEEP_CLK_CORRECTION
    if (g_socpm_struct.slp_clk_cal_params.slp_clk_cal_enabled_mode == ACTIVE_MODE)
        delay = delay * g_socpm_struct.slp_clk_cal_params.xocnt / g_socpm_struct.slp_clk_cal_params.refxocnt;
#endif

    delay += cpu_boot_bcn_rx_delay;

    return delay;
}

static void socpm_cfg_sleep_paras()
{
#if (FERMION_CHIP_VERSION == 1)
    xo_settle_time = 66;
    slp_exit_hw_delay_fixed = 77;
    son_en_wait_mcu = 0x1;
#else
    xo_settle_time = 25;
    slp_exit_hw_delay_fixed = 64;
    cpu_boot_bcn_rx_delay = 335;
    son_en_wait_mcu = 0xf;
#endif
}

// -------------------------------------------------------------------
// funcs
// -------------------------------------------------------------------

/*
 * @brief: This function used to configure AON sleep clock from CLI
 * @param option 0 = RC from PMIC, 1 = XO from RFA, 2 = External XO from PMIC
 * @return none
 */
void nt_sleep_clock_configuration(uint8_t option)
{
    switch (option) {
        case 0:
            NT_REG_WR(QWLAN_PMU_AON_TOP_CFG_REG,
                      QWLAN_PMU_AON_TOP_CFG_DEFAULT);  // Configuring with a value of 0 to choose RC from PMIC
            break;
        case 1:
            NT_REG_WR(QWLAN_PMU_AON_TOP_CFG_REG,
                      QWLAN_PMU_AON_TOP_CFG_CFG_XO_SLP_CLK_SEL_EN_MASK);  // Configuring to choose XO from RFA
            break;
        case 2:
            NT_REG_WR(QWLAN_PMU_AON_TOP_CFG_REG,
                      QWLAN_PMU_AON_TOP_CFG_CFG_XO_SLP_CLK_SEL_EN_MASK);  // Configuring to choose XO from PMIC
            break;
    }
}

/*
 * @brief: This function used to configure socpm state based on INI
 * @param none
 * @return none
 */
void nt_configure_socpm_state(void)
{
    uint8_t *devcfg_val = NULL;
    uint8_t socpm_enable = 0;

    devcfg_val = ((uint8_t *)(nt_devcfg_get_config(NT_DEVCFG_SOCPM_ENABLE)));
    if (devcfg_val == NULL) {
        NT_LOG_SME_ERR("Failed to read NT_DEVCFG_SOCPM_ENABLE", 0, 0, 0);
    } else {
        socpm_enable = *devcfg_val;
    }
    if (socpm_enable || is_wlan_power_save_enabled()) {
        nt_socpm_enable(1);
    }
}

void nt_socpm_enable(uint8_t socpm_state)
{
    if (socpm_state == 0)  // socpm disable
    {
        nt_socpm_slp_time_min = 0;
    } else if (socpm_state == 1)  // socpm enable
    {
        nt_socpm_slp_time_min = 0xFFFFFFFF;
        {  // WAR: bbpll_toggle_int to clear
            uint32_t val;
            val = NT_REG_RD(QWLAN_PMU_BBPLL_STATUS_REG);
            if (val & QWLAN_PMU_BBPLL_STATUS_BBPLL_LOCK_TOGGLE_INTR_MASK) {
                NT_LOG_PRINT(SOCPM, ERR, "bbpll_status: %x", val);
                val = NT_REG_RD(QWLAN_PMU_BBPLL_STATUS_REG);
                val |= (QWLAN_PMU_BBPLL_STATUS_CFG_BBPLL_LOCK_TOGGLE_INTR_CLR_MASK);
                NT_REG_WR(QWLAN_PMU_BBPLL_STATUS_REG, val);
                nt_socpm_nop_delay(100);
                NT_LOG_PRINT(SOCPM, ERR, "bbpll_status changed to: %x", NT_REG_RD(QWLAN_PMU_BBPLL_STATUS_REG));
            }
        }
    }
}

void __attribute__((section(".after_ram_vectors"))) Aon_cmnss_wlan_slp_tmr_int(void)
{
    PROF_IRQ_ENTER();

#if FDI_AON_ISR_EN
    FDI_NODE_START_NULL(FDI_DBG_PWR_S2W_SLEEP_TIMER_EXP_ISR);
#endif /* FDI_AON_ISR_EN */

    NT_REG_WR(QWLAN_PMU_AON_SLP_TIMER_INT_CLR_REG, QWLAN_PMU_AON_SLP_TIMER_INT_CLR_AON_SLP_TIMER_INT_CLR_MASK);
    __asm volatile(" nop                                    \n");
    NT_REG_WR(QWLAN_PMU_AON_SLP_TIMER_INT_CLR_REG, QWLAN_PMU_AON_SLP_TIMER_INT_CLR_AON_SLP_TIMER_INT_CLR_DEFAULT);
    __asm volatile(" nop                                    \n");
    _socpm_slp_exit = 1;
    /*reset it on interrupt*/
#if defined(SUPPORT_SOC_SLEEP_SOLVER)
    g_socpm_struct.aon_program_time_qtimer_us = 0;
#endif /*SUPPORT_SOC_SLEEP_SOLVER*/
    if (process_routine == 0)
        _socpm_slp_fn_process(_socpm_slp_mode);

#if FDI_AON_ISR_EN
    FDI_NODE_STOP_NULL(FDI_DBG_PWR_S2W_SLEEP_TIMER_EXP_ISR);
#endif /* FDI_AON_ISR_EN */

    PROF_IRQ_EXIT();
}

#ifndef PLATFORM_FERMION
/** On PLATFORM_NT, the external wakeup interrupt is used as WPS button for
 * WPS functionality. On PLATFORM_FERMION, the external wakeup interrupt
 * is used for A2F signal and hence this logic is not needed on Fermion.
 * The implementation of external interrupt ISR for Fermion is found in
 * core/system/sys_src/wifi_fw_ext_intr.c
 */
// AON External Interrupt

void aon_clr_external_int(void)
{
    BaseType_t xHigherPriorityTaskWoken = FALSE;
    uint32_t clr_ext_int = NT_REG_RD(QWLAN_PMU_AON_LIC_INT_CLR_REG);
    static wapp_msg_struct_t ps_external_int_config;

    clr_ext_int |= (1 << _SOCPM_EXT_INT_CLR_REG);
    NT_REG_WR(QWLAN_PMU_AON_LIC_INT_CLR_REG, clr_ext_int);

    clr_ext_int = NT_REG_RD(QWLAN_PMU_AON_LIC_INT_CLR_REG);
    clr_ext_int &= ~(1 << _SOCPM_EXT_INT_CLR_REG);
    NT_REG_WR(QWLAN_PMU_AON_LIC_INT_CLR_REG, clr_ext_int);

#ifdef NT_FN_FTM
    ps_external_int_config.device_action = external_interrupt;
#endif  // NT_FN_FTM
#ifndef NT_HOSTED_SDK
    qurt_pipe_try_send(wifiapp_queue_handle, (void *)&ps_external_int_config, &xHigherPriorityTaskWoken);
#else
    qurt_pipe_try_send(x_spiQueueHandle, (void *)&ps_external_int_config, &xHigherPriorityTaskWoken);
#endif
}

#ifdef NT_FN_WPS
#ifdef NT_HOSTLESS_SDK
void aon_ext_interrupt_check(void)
{
    static uint32_t wps_timeout = 0;
    static uint32_t nor_timeout = 0;
    if (_SOCPM_IS_BIT_SET(active_ext_int, 31)) {
        active_ext_int = 0;

        if (wps_timeout++ > 50) {
            WLAN_DBG0_PRINT("wps button pressed\r\n");
            if (pg_wps->wps_init_flag == 1) {
                WLAN_DBG0_PRINT("wps\r\n");
                wps_timeout = 0;
                nor_timeout = 0;

                //----------------------------------------------------------------------------
                // wmi_msg_struct_t data;

                wapp_cntx_h *p_wps_conf = NULL;
                p_wps_conf = (wapp_cntx_h *)nt_osal_allocate_memory(sizeof(wapp_cntx_h));
                if (p_wps_conf) {
                    memset(&networkConf, 0x0, sizeof(wapp_msg_struct_t));
                    // static wapp_msg_struct_t networkConf;
                    networkConf.device_action = wps_pbc_hndlr;
                    p_wps_conf->wps_config_mode = WPS_PBC_MODE;
                    networkConf.msg_struct.wapp_data_len = sizeof(wapp_cntx_h);
                    networkConf.msg_struct.wapp_data = p_wps_conf;
                    //  networkConf.msg_struct.result_function = &wifiapp_result_function;

                    aon_clr_external_int();

                    if (NT_QUEUE_FAIL ==
                        qurt_pipe_send_timed(wifiapp_queue_handle, (void *)&networkConf, portMAX_DELAY)) {
                        NT_LOG_SME_ERR("Queue send failed", 0, 0, 0);
                    }
                } else {
                    WLAN_DBG0_PRINT("Failed to allocate memory\r\n");
                }
            }
            //----------------------------------------------------------------------------
            qurt_timer_stop(ext_int_timer, 100);
            _socpm_tmr_flag = 0;
            // break the loop
        }
    } else {
        if (nor_timeout++ > 10) {
            WLAN_DBG0_PRINT("ext int\r\n");
            nor_timeout = 0;
            wps_timeout = 0;
            // clear the aon interrupt
            aon_clr_external_int();
            qurt_timer_stop(ext_int_timer, 100);
            _socpm_tmr_flag = 0;
        }
    }

    // qurt_timer_stop(ext_int_timer, 100);
}
#endif  // NT_HOSTLESS_SDK
#endif  // NT_FN_WPS

// Enable External Wake-up interrupt (63)
void enable_aon_ext_wakeup_int(void)
{
    uint32_t en_ext_int = NT_REG_RD(NVIC_ISER1);

#ifdef NT_FN_WPS
#ifdef NT_HOSTLESS_SDK
    ext_int_timer = nt_create_timer(&aon_ext_interrupt_check, NULL, 100, TRUE);

    WLAN_DBG0_PRINT("ext\r\n");

    if (ext_int_timer == NULL) {
        WLAN_DBG0_PRINT("ext err\r\n");
        return;  // timer creation failed
    }
#endif
#endif  // NT_HOSTLESS_SDK

    en_ext_int |= (1 << _SOCPM_EXT_INT_CLR_NVIC);
    NT_REG_WR(NVIC_ISER1, en_ext_int);
}

// External interrupt ISR
void __attribute__((section(".after_ram_vectors"))) aon_ext_interrupt_wake_up(void)
{
    PROF_IRQ_ENTER();

#ifdef NT_FN_WPS
    int status = 0;
    active_ext_int = NT_REG_RD(0xE000E304);
#endif

    aon_clr_external_int();

#ifdef NT_FN_WPS
    // active_ext_int = NT_REG_RD(0xE000E304);
    if (_socpm_tmr_flag == 0) {
        if (ext_int_timer != NULL) {
            status = qurt_timer_start_frm_isr(ext_int_timer, FALSE);
            if (status != 0) {
                nt_dbg_print("isr timer err\r\n");
            }
            nt_dbg_print("TS\r\n");
            _socpm_tmr_flag = 1;
        }
    }
#endif

    PROF_IRQ_EXIT();
}
#endif  // ifndef PLATFORM_FERMION

void nt_socpm_nop_delay(uint64_t n_nops)
{
    for (uint64_t i = 0; i < n_nops; i++)
        __asm volatile(" nop \n");
}

#if defined(IMAGE_FERMION)
void _socpm_slptmr_off(void)
{
    uint16_t loop_count = 0;
    uint32_t aon_tmr_int_sts;
    uint64_t loop_start_time, curr_time_us;
    // Disable Wlan sleep timer interrupt

    aon_tmr_int_sts = NT_REG_RD(QWLAN_PMU_AON_SLP_TIMER_INT_STS_REG);
    if (aon_tmr_int_sts) {
        NT_REG_WR(QWLAN_PMU_AON_SLP_TIMER_INT_CLR_REG, QWLAN_PMU_AON_SLP_TIMER_INT_CLR_AON_SLP_TIMER_INT_CLR_MASK);

        /** The write to clear AON timer interrupt is a posted write. The AON timer
         * runs on a 32kHz clock and takes a few clock cycles to clear. When sleep
         * clock switching is enabled, although writes to AON registers would be
         * quicker while running on faster XO/4 clock, the interrupt clear takes
         * longer as the AON timer still runs on the slow clock. To ensure that the
         * interrupt has been cleared, SW needs to poll on the interrupt status
         * register. This is necessary to avoid unsuccessful attempts by SW to configure
         * WIFI_SS or MCU_SS to sleep state while the interrupt has not been cleared.
         */
        loop_start_time = hres_timer_curr_time_us();
        while (1) {
            aon_tmr_int_sts = NT_REG_RD(QWLAN_PMU_AON_SLP_TIMER_INT_STS_REG);
            if (aon_tmr_int_sts == 0) {
                break;
            } else {
                loop_count++;
                if (loop_count > 10) {
                    NT_REG_WR(QWLAN_PMU_AON_SLP_TIMER_INT_CLR_REG,
                              QWLAN_PMU_AON_SLP_TIMER_INT_CLR_AON_SLP_TIMER_INT_CLR_MASK);
                    loop_count = 0;
                }
            }

            curr_time_us = hres_timer_curr_time_us();
            /* Break the loop after a fixed timeout to avoid hang */
            if ((curr_time_us > loop_start_time) &&
                ((curr_time_us - loop_start_time) > _SOCPM_AON_TMR_INT_CLR_TIMEOUT_US)) {
                NT_LOG_PRINT(SOCPM, WARN, "AON I Error");
                break;
            }
        }
    }
}
#else
void _socpm_slptmr_off(void)
{
    // Disable Wlan sleep timer interrupt
    NT_REG_WR(QWLAN_PMU_AON_SLP_TIMER_INT_CLR_REG, QWLAN_PMU_AON_SLP_TIMER_INT_CLR_AON_SLP_TIMER_INT_CLR_MASK);
    NT_REG_WR(QWLAN_PMU_WLAN_SLP_TMR_CTL_REG, QWLAN_PMU_WLAN_SLP_TMR_CTL_WLAN_SLP_TMR_INT_CLR_MASK);
    NT_REG_RD(QWLAN_PMU_WLAN_SLP_TMR_CTL_REG);
    NT_REG_WR(QWLAN_PMU_WLAN_SLP_TMR_CTL_REG, 0);
}
#endif /*IMAGE_FERMION*/
/*-----------------------------------------------------------*/

uint32_t _tst_bmps_enter_f;
#if !defined(IMAGE_FERMION)
void nt_socpm_glob_restore(void)
{
/*
    #ifdef _SOCPM_INC_FN_TIMESYNC
        uint64_t tsf;
        tsf = _socpm_glob_data.tsf_us + _socpm_glob_data.aon_slp_tmr_us + _SOCPM_PS_WAKE_HW_OVERHEADS_US;
        NT_REG_WR(QWLAN_MTU_MTU_GLOBAL_TIMER_REG,
        _socpm_glob_data.mtu_glob_tmr + ((uint32_t) _socpm_glob_data.aon_slp_tmr_us) + _SOCPM_PS_WAKE_HW_OVERHEADS_US);
        NT_REG_WR(QWLAN_MTU_TSF_TIMER_LO_REG, (uint32_t) tsf);
        NT_REG_WR(QWLAN_MTU_TSF_TIMER_HI_REG, (uint32_t) (tsf >> 32));
    #endif
*/
#ifdef _SOCPM_INC_FN_TIMESYNC
    uint64_t tsf = _socpm_glob_data.tsf_us + nt_socpm_slp_tmr_get() + 5000;
    NT_REG_WR(QWLAN_MTU_TSF_TIMER_LO_REG, tsf);
    NT_REG_WR(QWLAN_MTU_TSF_TIMER_HI_REG, (tsf >> 32));
    //_minprintf("Rst", (uint32_t) (tsf >> 32), (uint32_t) tsf);
#endif
}

static void _socpm_glob_save(uint64_t aon_tmr_us)
{
    (void)aon_tmr_us;

#ifdef _SOCPM_INC_FN_TIMESYNC
    uint32_t tsfh1, tsfh2;
    tsfh1 = NT_REG_RD(QWLAN_MTU_TSF_TIMER_HI_REG);
    _socpm_glob_data.mtu_glob_tmr = NT_REG_RD(QWLAN_MTU_MTU_GLOBAL_TIMER_REG);
    _socpm_glob_data.tsf_us = NT_REG_RD(QWLAN_MTU_TSF_TIMER_LO_REG);
    tsfh2 = NT_REG_RD(QWLAN_MTU_TSF_TIMER_HI_REG);  // read tsf-hi again
    if (tsfh2 != tsfh1)                             // tsf rolled over 32-bit boundary, read tsf lo again
        _socpm_glob_data.tsf_us = NT_REG_RD(QWLAN_MTU_TSF_TIMER_LO_REG);
    _socpm_glob_data.tsf_us |= (((uint64_t)tsfh2) << 32);
    _socpm_glob_data.aon_slp_tmr_us = aon_tmr_us;
//_minprintf("Save", (uint32_t) (_socpm_glob_data.tsf_us >> 32), (uint32_t) _socpm_glob_data.tsf_us);
#endif
    // nt_socpm_glob_restore();
}
#endif  // !defined(IMAGE_FERMION)

uint64_t nt_socpm_min_slp_time_us()
{
    return MS_TO_US(_socpm_slp_time_supp_min_ms);
}

sleep_mode nt_socpm_curr_slp_mode()
{
    return _socpm_slp_mode;
}

uint8_t nt_socpm_wake_from_mcuss_sleep(void)
{
    return _socpm_mcu_sleep_wake;
}

int nt_get_socpm_slp_lst_head(void)
{
    return _socpm_slp_lst_head;
}

#if defined(SUPPORT_SWTMR_TO_WKUP_FROM_BMPS) && defined(SUPPORT_SLEEP_LIST_IMPROVEMENTS)
/*
 * @brief: This function is to get the valid sleep time and respective log info by comparing the qtimer expiry
 * @param : curr_slp_val. current sleep value available
 * @return :updated slp_val: depending on the qtimer expiry the sleep value will be updated
 * @note: This function should not be called out of Idle task context, due to scheduler suspension state
 */
static uint64_t nt_socpm_get_sleep_time(uint64_t cur_slp_val)
{
    g_socpm_struct.info.ms_time = cur_slp_val;
    bool is_timer_task_handle = false;
    TaskHandle_t delayed_list_head_owner = xGetDelayedListHeadOwner();

    if (NULL != delayed_list_head_owner) {
        is_timer_task_handle = xIsTimertaskHandle(delayed_list_head_owner);
    }

    /* Avoid the assertion in acuquiring the lock in hres_timer_pre_sleep function
     * by passing false because scheduler will be suspended.
     */
    g_socpm_struct.info.ms_time = hres_timer_pre_sleep(&g_socpm_struct.info, /* task_scheduler_state */ false);

    if (g_socpm_struct.info.ms_time <= cur_slp_val) {
        g_socpm_struct.wkup_reason = REASON_TO_WKUP_Q_TIMER;
    } else {
        if (is_timer_task_handle && !nt_ignore_timer_wakeup(_socpm_slp_lst_head)) {
            g_socpm_struct.wkup_reason = REASON_TO_WKUP_NT_TIMER;
            g_socpm_struct.info.nt_timer_callback = xGetHeadTimerCallback();
            g_socpm_struct.info.ms_time = cur_slp_val;
        } else {
            g_socpm_struct.wkup_reason = REASON_TO_WKUP_NT_TASK;
            if (is_timer_task_handle) {
                g_socpm_struct.info.ms_time = xGetNextSleepTime();
            } else {
                g_socpm_struct.info.ms_time = cur_slp_val;
            }
            memscpy(g_socpm_struct.info.pcTaskName, configMAX_TASK_NAME_LEN, pcTaskGetName(delayed_list_head_owner),
                    configMAX_TASK_NAME_LEN);
        }
    }
    return g_socpm_struct.info.ms_time;
}
#endif /* SUPPORT_SWTMR_TO_WKUP_FROM_BMPS && SUPPORT_SLEEP_LIST_IMPROVEMENTS*/

void nt_socpm_slp_tmr_set(uint64_t sleep_time)  // in us
{
#ifdef COMPENSATE_AON_PROG_DELAY
    uint64_t curr_aon_time_us, aon_time_us_prior_reset;
    uint32_t loop_start_time, curr_time_us;
#endif /* COMPENSATE_AON_PROG_DELAY */
    uint32_t temp_1, temp_2, temp_3;

#ifdef SOCPM_SLEEP_DEBUG
    uint64_t slp_time_orig;
#endif
    _socpm_slptmr_off();

    temp_3 = sleep_time;
    // Minimum Sleep time for AON
    if (sleep_time < (_socpm_slp_time_supp_min_ms * 1000)) {
        _socpm_slp_mode = clk_gtd_sleep;
    }

#if defined(EMULATION_BUILD) && defined(PLATFORM_FERMION)
    sleep_time *= _slp_clk_scaling;
#endif  // defined(EMULATION_BUILD) && defined(PLATFORM_FERMION)
#ifdef APPLY_SLEEP_CLK_CORRECTION
#if (FERMION_CHIP_VERSION == 1)
    if (g_socpm_struct.slp_clk_cal_params.slp_clk_cal_enabled_mode == ACTIVE_MODE)
        sleep_time = sleep_time * g_socpm_struct.slp_clk_cal_params.refxocnt / g_socpm_struct.slp_clk_cal_params.xocnt;
#endif
#endif /* APPLY_SLEEP_CLK_CORRECTION */
#ifdef SOCPM_SLEEP_DEBUG
    slp_time_orig = sleep_time;
#endif

#if defined(COMPENSATE_RC_DIVISION_ERROR_WAR)
    /* Compensating the right shift division error while configuring the AON timer*/
    sleep_time = COMPENSATE_RC_DIVISION_ERROR_SLP_TMR_SET(sleep_time);
#endif /* COMPENSATE_RC_DIVISION_ERROR_WAR */

    // for pmic xo, each tick is (1/32768)s = (1/2^15)= (10^6/2^15)uS = (15625/2^9) uS
    sleep_time = _SOCPM_US_TO_AON_TICK(sleep_time);
    temp_1 = sleep_time;

    NT_REG_WR(NT_NVIC_ICPR1, 0x00800000);  // Clear pending int
    temp_2 = NT_REG_RD(NT_NVIC_ISER1);
    temp_2 = temp_2 | 0x00800000;
    NT_REG_WR(NT_NVIC_ISER1, temp_2);

    if (nt_socpm_sby_force == 1) {
        nt_socpm_sby_force = 0;
        // temp_1=_socpm_slp_lst[1].slp_info.slp_time;
        temp_1 = nt_socpm_slp_time_sby;
        // WLAN_DBG2_PRINT("stand_by_sleep_time",nt_socpm_slp_time_sby,temp_1);
    }
    NT_REG_WR(QWLAN_PMU_WLAN_SLP_TMR_CTL_REG, QWLAN_PMU_WLAN_SLP_TMR_CTL_WLAN_SLP_TMR_INT_EN_MASK);

#if (NT_CHIP_VERSION == 2) || defined(PLATFORM_FERMION)

    uint32_t value;
    value = NT_REG_RD(QWLAN_PMU_AON_LIC_INT_EN_REG);
#ifdef FIRMWARE_APPS_INFORMED_WAKE
    value |= QWLAN_PMU_AON_LIC_INT_EN_WLAN_WAKEUP_INTR_EN_MASK;
#else
    /** This is kept for backward compatibility with the original logic.
     * Ideally, this routine should not overwrite/modify other LIC interrupt
     * settings. After further testing, this logic is to be removed and only
     * the WLAN_WAKEUP_INTR bit modified in this routine.
     */
    value = QWLAN_PMU_AON_LIC_INT_EN_WLAN_WAKEUP_INTR_EN_MASK;
#endif /* FIRMWARE_APPS_INFORMED_WAKE */

    NT_REG_WR(QWLAN_PMU_AON_LIC_INT_EN_REG, value);

    for (int cc = 0; cc < 10; cc++) {
        __asm volatile("nop");
    }

#ifdef SLEEP_CLK_CAL_IN_SLEEP_MODE
    if (false == g_socpm_struct.slp_clk_cal_params.sleep_mode_cal_enabled) {
#endif /* SLEEP_CLK_CAL_IN_SLEEP_MODE */
        value = NT_REG_RD(QWLAN_PMU_CFG_SLP_CAL_CAL_EN_REG);
        value = (value & (~QWLAN_PMU_CFG_SLP_CAL_CAL_EN_CFG_SLP_CLK_CAL_EN_MASK));
        NT_REG_WR(QWLAN_PMU_CFG_SLP_CAL_CAL_EN_REG, value);
#ifdef SLEEP_CLK_CAL_IN_SLEEP_MODE
    }
#endif /* SLEEP_CLK_CAL_IN_SLEEP_MODE */

    for (int cc = 0; cc < 10; cc++) {
        __asm volatile("nop");
    }

    value = NT_REG_RD(QWLAN_PMU_SLP_TMR_CTL_REG);
    value = (value | QWLAN_PMU_SLP_TMR_CTL_SLP_TMR_EN_MASK);
    NT_REG_WR(QWLAN_PMU_SLP_TMR_CTL_REG, value);
#endif  //(NT_CHIP_VERSION==2) || defined(PLATFORM_FERMION)

#if defined(PLATFORM_NT) && defined(NT_SOCPM_SW_MTUSR)
    // On NT, the sleep timer runs by default, but is disabled automatically
    //  on wakeup after timer expiry. The SLP_TMR_EN bit needs to be set to
    //  allow it to run even after wakeup, which helps in accurate restoration of
    //  timers on wakeup from sleep.
    uint32_t reg_value = NT_REG_RD(QWLAN_PMU_SLP_TMR_CTL_REG);
    reg_value = (reg_value | QWLAN_PMU_SLP_TMR_CTL_SLP_TMR_EN_MASK);
    NT_REG_WR(QWLAN_PMU_SLP_TMR_CTL_REG, reg_value);
#endif  // defined(PLATFORM_NT) && defined(NT_SOCPM_SW_MTUSR)

    temp_2 = (sleep_time >> 32);
#if !defined(IMAGE_FERMION)
    _socpm_glob_save(_SOCPM_XO_CLK_AON_TICK_TO_US(sleep_time));
#endif  // !defined(IMAGE_FERMION)

#ifdef COMPENSATE_AON_PROG_DELAY
    aon_time_us_prior_reset = nt_socpm_get_slp_tmr_us();
#endif /* COMPENSATE_AON_PROG_DELAY */

    NT_REG_WR(QWLAN_PMU_WLAN_SLP_TMR_EXP_MSB_REG, temp_2);
    __asm volatile("nop");

    NT_REG_WR(QWLAN_PMU_WLAN_SLP_TMR_EXP_LSB_REG, temp_1);
    __asm volatile("nop");

#if defined(COMPENSATE_RC_DIVISION_ERROR_WAR)
    g_socpm_struct.glb_pre_sleep_time_us = hres_timer_curr_time_us();
#endif /* COMPENSATE_RC_DIVISION_ERROR_WAR */

    nt_clear_device_irq(AON_cmnss_wlan_slp_tmr_int);

#ifdef SOCPM_SLEEP_DEBUG
    if (_socpm_slp_mode == mcu_sleep) {
        uint32_t d1 = temp_1;
        uint32_t d2;
        uint32_t aon_lic_int_stat = NT_REG_RD(QWLAN_PMU_AON_LIC_INT_STAT_REG);
        d2 = ((NT_REG_RD(QWLAN_PMU_WLAN_SLP_TMR_STS_REG) << 0) & 0x3) |  // WLAN_SLP_TMR_INT,WLAN_SLP_TMR_INT_RAW
             ((aon_lic_int_stat << 2) & 0x4) |                           // WLAN_WAKEUP_INTR_STAT
             (((aon_lic_int_stat >> QWLAN_PMU_AON_LIC_INT_STAT_WLAN_WAKEUP_INTR_STAT_RAW_OFFSET) << 3) &
              0x8) |                                                           // WLAN_WAKEUP_INTR_STAT_RAW
             ((NT_REG_RD(QWLAN_PMU_AON_SLP_TIMER_INT_STS_REG) << 4) & 0x10) |  // AON_WLAN_SLP_TMR_INT
             0;
        socpm_log_timestamp(SLP_TM_SET, d1, d2, (uint32_t)slp_time_orig);
    }
#endif

#ifdef COMPENSATE_AON_PROG_DELAY
    if ((FALSE == g_socpm_struct.in_warm_boot) && g_socpm_struct.systick_off_time_us) {
        loop_start_time = hres_timer_curr_time_us();

        /* Wait until AON timer is restarted */
        while (1) {
            curr_aon_time_us = nt_socpm_get_slp_tmr_us();
            /* If current aon time is less than whats recorded before register reset */
            /* it means aon timer is restarted successfully and started from zero */
            if (curr_aon_time_us < aon_time_us_prior_reset)
                break;

            curr_time_us = hres_timer_curr_time_us();
            /* Break the loop after a fixed timeout to avoid hang */
            if ((curr_time_us - loop_start_time) > _SOCPM_AON_RESET_TIMEOUT_US) {
                NT_LOG_PRINT(SOCPM, INFO, "AON P Error %d %d %d %d %d %d", aon_time_us_prior_reset >> 32,
                             aon_time_us_prior_reset & 0xffffffff, curr_aon_time_us >> 32,
                             curr_aon_time_us & 0xffffffff, loop_start_time, curr_time_us);
                break;
            }
        }

        curr_time_us = hres_timer_curr_time_us();

        if (curr_time_us > g_socpm_struct.systick_off_time_us) {
            /* Note the time from sys-tick-timer-off till now */
            g_socpm_struct.aon_program_time_us = (curr_time_us - g_socpm_struct.systick_off_time_us);

            /* Subtract the time passed from AON reset to get exact AON programming time */
            if (g_socpm_struct.aon_program_time_us > curr_aon_time_us) {
                g_socpm_struct.aon_program_time_us = g_socpm_struct.aon_program_time_us - curr_aon_time_us;
            }
        } else {
            g_socpm_struct.aon_program_time_us = 0;
        }
    }
#endif /* COMPENSATE_AON_PROG_DELAY */

#if defined(SUPPORT_SOC_SLEEP_SOLVER)
    g_socpm_struct.aon_program_time_qtimer_us = hres_timer_curr_time_us();
#endif /*SUPPORT_SOC_SLEEP_SOLVER*/

#ifdef NT_SOCPM_SW_MTUSR
    nt_socpm_mtusr_save_aon_prog_timestamp();
#endif  // NT_SOCPM_SW_MTUSR

    // print only after setting the timer to absorb the uart delay into the sleep timer
    if (_tst_bmps_enter_f == 0xA5A500A5) {
        _tst_bmps_enter_f = 0;
        (void)temp_3;
    }

    // may not be needed, just doing this to flush posted aon writes
    volatile uint32_t temp = NT_REG_RD(QWLAN_PMU_WLAN_SLP_TMR_EXP_LSB_REG);
    (void)temp;
}
/*-----------------------------------------------------------*/

void nt_socpm_soc_sleep_processing(uint64_t slp_val)
{
    uint64_t slp_exp = 0;
#ifdef CONFIG_QAT_POWERSAVE_DEMO
    PM_STRUCT *pPmStruct = NULL;
    if (gdevp) {
        pPmStruct = (PM_STRUCT *)gdevp->pPmStruct;
    }
#endif

#ifdef FIRMWARE_APPS_INFORMED_WAKE
    if ((TRUE == g_socpm_struct.a2f_asserted) || (TRUE == g_socpm_struct.f2a_asserted)) {
        /** Check if A2F or F2A are asserted and abort sleep.
         * This check is performed after IRQ disable to avoid the scenario of
         * the A2F IRQ status being cleared and erroneously allowing the system
         * to go to sleep in a case where A2F is asserted after systick stop but
         * before IRQ disable.
         */

        /** TODO: Test with A2F asserted to see if systick needs to be adjusted
         * before restore for the time when systick was off.
         */

        /* Re-enable interrupts */
        NT_SOCPM_IRQ_ENABLE();
       
        /* Restart tick. */
        _socpm_systick_on();

        uint64_t delta_hres_time_us = hres_timer_curr_time_us() - hres_time_pre_sleep + g_socpm_struct.unapplied_systick_err_us;
        uint64_t delta_hres_time_ms = 0;

        NT_LOG_PRINT(SOCPM, INFO, " nt_socpm_slp_time_total %d delta_rtos %d", (uint32_t)nt_socpm_slp_time_total,
                        (uint32_t)delta_hres_time_us);

        delta_hres_time_ms = US_TO_MS(delta_hres_time_us);
        g_socpm_struct.unapplied_systick_err_us = (delta_hres_time_us - 1000 * delta_hres_time_ms);

        if (delta_hres_time_ms > 0) {
            vTaskStepTick(delta_hres_time_ms);
        } 

    }
#else  /*FIRMWARE_APPS_INFORMED_WAKE*/
    if (0) {
        /* For legacy code compilation */
    }
#endif /*FIRMWARE_APPS_INFORMED_WAKE*/
#ifdef SUPPORT_SLEEP_DEBUG_UNIT_TEST_CMD
    /* To exercise datapath flush during BMPS need to avoid the soc going to sleep */
    else if (g_socpm_struct.socpm_dbg_unit_test_value & (1 << SLP_DBG_SOCPM_ON_DURING_BMPS)) {
        /* Restart tick. */
        _socpm_systick_on();
        /* Re-enable interrupts */
        NT_SOCPM_IRQ_ENABLE();

    }
#endif /* SUPPORT_SLEEP_DEBUG_UNIT_TEST_CMD */
#ifdef CONFIG_QAT_POWERSAVE_DEMO
    else if (pPmStruct && pPmStruct->powersave_policy == PS_POLICY_NOT_ALLOWED_SLEEP) {
        /* Re-enable interrupts */
        NT_SOCPM_IRQ_ENABLE();
        /* Restart tick. */
        _socpm_systick_on();

        uint64_t delta_hres_time_us = hres_timer_curr_time_us() - hres_time_pre_sleep + g_socpm_struct.unapplied_systick_err_us;
        uint64_t delta_hres_time_ms = 0;

        NT_LOG_PRINT(SOCPM, INFO, " nt_socpm_slp_time_total %d delta_rtos %d", (uint32_t)nt_socpm_slp_time_total,
                        (uint32_t)delta_hres_time_us);

        delta_hres_time_ms = US_TO_MS(delta_hres_time_us);
        g_socpm_struct.unapplied_systick_err_us = (delta_hres_time_us - 1000 * delta_hres_time_ms);

        if (delta_hres_time_ms > 0) {
            vTaskStepTick(delta_hres_time_ms);
        } 
#ifdef SUPPORT_QCSPI_SLAVE
            qcspi_slv_init();
#endif /* SUPPORT_QCSPI_SLAVE */
    } 
#endif /* CONFIG_QAT_POWERSAVE_DEMO */
    else {
        /*Decide on which sleep type to enter
         * limited now on sleep time need to add battery and voting inputs to it
         */
        nt_socpm_resume_f = 1;
#ifdef SUPPORT_IMPS_IMPROVEMENTS
        if ((g_ppm_common_struct.imps_struct_ctx.imps_registered == TRUE) &&
            g_ppm_common_struct.imps_struct_ctx.imps_enabled && g_ppm_common_struct.imps_struct_ctx.policy == Standby) {
            nt_wpm_register_imps_standby();
        }
#endif /* SUPPORT_IMPS_IMPROVEMENTS */
#if defined(SUPPORT_SLEEP_LIST_IMPROVEMENTS)
        if (_INVALID_SLP_LST_HD != _socpm_slp_lst_head &&
            (_socpm_slp_lst[_socpm_slp_lst_head].slp_info.slp_time != 0)) {
            uint64_t updated_slp_val = 0;
#if defined(SUPPORT_SWTMR_TO_WKUP_FROM_BMPS)
            updated_slp_val = nt_socpm_get_sleep_time(slp_val);
#else
            updated_slp_val = slp_val;
#endif /* SUPPORT_SWTMR_TO_WKUP_FROM_BMPS */
            if (updated_slp_val != portMAX_DELAY) {
                if (updated_slp_val != slp_val && updated_slp_val > xMaximumPossibleSuppressedTicks) {
                    updated_slp_val = xMaximumPossibleSuppressedTicks;
                }
                if (updated_slp_val != slp_val && updated_slp_val > _SOCPM_STOP_TMR_COMP) {
                    /* Compensate for the fact that the Sleep timer is going to be stopped
                     * momentarily. */
                    updated_slp_val -= _SOCPM_STOP_TMR_COMP;
                }
                _socpm_os_tmr.slp_time = (updated_slp_val * 1000);  // ticks to converting to us
                if (updated_slp_val != (xMaximumPossibleSuppressedTicks - _SOCPM_STOP_TMR_COMP)
                    /**
                     *  @brief: Check for a better sleep only if the protocol has already registered for mcu/light sleep
                     *          else the RRI save would not have happened and there is no pint going to better sleep.
                     */
                    && _socpm_slp_lst[_socpm_slp_lst_head].slp_info.slp_mode != clk_gtd_sleep) {
                    _socpm_os_tmr.slp_mode = nt_socpm_sleep_solver(_socpm_os_tmr.slp_time);
                    NT_LOG_PRINT(SOCPM, INFO, "nt_socpm_sleep_solver");
                } else {
                    _socpm_os_tmr.slp_mode = clk_gtd_sleep;
                }

                NT_LOG_PRINT(
                    SOCPM, INFO,
                    " _socpm_slp_lst_head %d slp_time: %d _socpm_last_slp_count:%d updated_slp_val: %d slp_mode: %d "
                    "_socpm_slp_list_idx_rtos %d ",
                    _socpm_slp_lst_head, (uint32_t)_socpm_slp_lst[_socpm_slp_lst_head].slp_info.slp_time / 1000,
                    _socpm_last_slp_count, (uint32_t)updated_slp_val, _socpm_os_tmr.slp_mode, _socpm_slp_list_idx_rtos);
            }
        } else
#endif
        {
            _socpm_os_tmr.slp_time = (slp_val * 1000);  // ticks to converting to us
            _socpm_os_tmr.slp_mode = clk_gtd_sleep;
        }
        if (!(_socpm_os_tmr.slp_mode != clk_gtd_sleep && _socpm_os_tmr.slp_time == portMAX_DELAY)) {
            _socpm_slp_exit = 0;
            _socpm_slp_list_idx_rtos = nt_socpm_sleep_register(&_socpm_os_tmr, _socpm_slp_list_idx_rtos);
        }
#ifdef SUPPORT_SLEEP_DEBUG_UNIT_TEST_CMD
        /* Adding pending dummy sleep nodes to sleep list to exercise the sleep list functionality and handling*/
        if (g_socpm_struct.add_dummy_slp_list_node) {
            nt_add_dummy_slp_list_node();
        }
#endif /* SUPPORT_SLEEP_DEBUG_UNIT_TEST_CMD */
        FDI_NODE_STOP_NULL(FDI_DBG_PWR_W2S_IDLE_TASK_KICK_IN);

#ifdef SOCPM_SLEEP_DEBUG
        if (_socpm_slp_mode == mcu_sleep) {
            uint32_t d1 = _SOCPM_RC_OR_EXT_CLK_AON_TICK_TO_US(
                (((uint64_t)(NT_REG_RD(QWLAN_PMU_WLAN_SLP_TMR_EXP_MSB_REG))) << 32) |
                (NT_REG_RD(QWLAN_PMU_WLAN_SLP_TMR_EXP_LSB_REG)));
            uint32_t d2 = nt_socpm_get_slp_tmr_us();
            uint32_t d3 = (uint32_t)_socpm_slp_lst[_socpm_slp_lst_head].slp_info.slp_cb_fn;
            socpm_log_timestamp(__PREV_SLP, d1, d2, d3);
        }
#endif

        g_socpm_struct.nvic_icpr_status[0] = NT_REG_RD(NT_CM4_NVIC_ISER0_CLEAR_PENDING_REG);
        g_socpm_struct.nvic_icpr_status[1] = NT_REG_RD(NT_CM4_NVIC_ISER1_CLEAR_PENDING_REG);
        g_socpm_struct.nvic_icpr_status[2] = NT_REG_RD(NT_CM4__NVIC_ISER2_CLEAR_PENDING_REG);
        g_socpm_struct.nvic_icpr_status[3] = NT_REG_RD(NT_CM4_NVIC_ISER3_CLEAR_PENDING_REG);

        if ((g_socpm_struct.nvic_icpr_status[1] & AON_TIMER_INTR_NVIC1_MASK)
#ifdef PLATFORM_FERMION
            || (g_socpm_struct.nvic_icpr_status[1] & A2F_ASSERT_INTR_NVIC1_MASK) ||
            (g_socpm_struct.nvic_icpr_status[3] & A2F_DEASSERT_INTR_NVIC3_MASK) ||
            (g_socpm_struct.nvic_icpr_status[0] & NT_CM4_UART_INTERRUPT_BIT_MASK)
#endif /* PLATFORM_FERMION */
        ) {
            printf("a2f pend\r\n");
            // _socpm_slp_mode = clk_gtd_sleep;
        }
        /* This function performs sleep recipe as per the sleep mode specified */
#ifdef CONFIG_WATCH_DOG_ENABLE
        /* feed watchdog before sleep */
        nt_watchdog_bark_timer_reset();
#endif
        vPreSleepProcessing(_socpm_slp_mode);

        /* Save current context and call WFI */
        _socpm_ctxt_save();
        /*  Check if sleep entry was prevented and assert if not a valid prevention  */
        nt_socpm_check_sleep_entry_failure(_socpm_slp_mode, TRUE,FALSE);

#if defined(IDLE_DISABLED) && defined(NT_DEBUG)
        /* This is a debug code required in future to attach debugger post RAM minimal code context restore */
        /* Disabled by default. The debugger (user) shall enable as and when required */
        /* A variable that controls the while loop. debug_ctrl shall be toggled using JTAG to release from the loop */
        volatile int debug_ctrl = true;
        while (debug_ctrl && _socpm_slp_mode != clk_gtd_sleep)  // && nt_socpm_ctxt_restore_ctr > 1)
        {
            __asm volatile("nop \n");
        }
#endif /* IDLE_DISABLED */
        FDI_NODE_START_NULL(FDI_DBG_PWR_S2W_IDLE_RESTORE_CONTEXT);
        if (_socpm_slp_mode != clk_gtd_sleep) {
#if defined(PLATFORM_NT)
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
        }

#ifdef NT_GPIO_FLAG
        // nt_gpio_pin_write(NT_GPIOA, GPIO_PIN_5, NT_GPIO_HIGH);
#endif
        /* Stop Sleep timer.  Give Control Back to SYSTICK Handler*/
        _socpm_slptmr_off();
        // Get the slept time
        // nt_socpm_resume_f = 1;
        /* Wind the tick forward by the number of tick periods that the CPU
         remained in a low power state. */
        //  slp_exp = slp_exp ; //convert to Systick Period
        //@to-do replace with expire time debug only
#ifdef COMPENSATE_AON_PROG_DELAY
        /* Apply correction post clock gated sleep */
        /* Total sleep time = AON measured sleep time + AON programming time + US2MS error carried forward */
        uint64_t slp_exp_us =
            (nt_socpm_get_slp_tmr_us() + g_socpm_struct.aon_program_time_us + g_socpm_struct.unapplied_err_us);
        slp_exp = US_TO_MS(slp_exp_us);
        g_socpm_struct.unapplied_err_us = (slp_exp_us - 1000 * slp_exp);
        g_socpm_struct.systick_off_time_us = 0;
#else  /* COMPENSATE_AON_PROG_DELAY */
        slp_exp = US_TO_MS(nt_socpm_get_slp_tmr_us());
#endif /* COMPENSATE_AON_PROG_DELAY */

#ifdef SLEEP_CLK_CAL_IN_SLEEP_MODE
        socpm_slp_clk_cal_postawake_activities();
#endif /* SLEEP_CLK_CAL_IN_SLEEP_MODE */

        // #ifdef SUPPORT_SW_NON_POLLED_RRI
        //             nt_pm_sync_non_polled_rri_completion(gdevp);
        // #endif /* SUPPORT_SW_NON_POLLED_RRI */

        // Reenable systick
        _socpm_systick_on();
        /* Re-enable the interrupts*/
        // GC:TODO
        NT_SOCPM_IRQ_ENABLE();
        /**
        * nt_socpm_slp_time_total sometimes is smaller than the real passing sleep time during Systick closed,
        *because it does not calculate the time when receive beacon, use the delta of hres timer is more accurate
        *
        **/
        uint64_t delta_hres_time_us = hres_timer_curr_time_us() - hres_time_pre_sleep + g_socpm_struct.unapplied_systick_err_us;
        uint64_t delta_hres_time_ms = 0;

        NT_LOG_PRINT(SOCPM, INFO, " nt_socpm_slp_time_total %d delta_rtos %d", (uint32_t)nt_socpm_slp_time_total,
                        (uint32_t)delta_hres_time_us);

        delta_hres_time_ms = US_TO_MS(delta_hres_time_us);
        g_socpm_struct.unapplied_systick_err_us = (delta_hres_time_us - 1000 * delta_hres_time_ms);

        if (delta_hres_time_ms > 0) {
            
            vTaskStepTick(delta_hres_time_ms);
        } else if (nt_socpm_resume_f == 2) {
            vTaskStepTick(slp_exp);
        } else {
            vTaskStepTick(slp_exp);
        }
#if 0
        NT_LOG_PRINT(DPM, ERR,"slp:%dms  slp:%dus aonTm:%dus\n\r",
                (uint32_t)slp_exp, (uint32_t)slp_exp_us, (uint32_t)cur_aon_us);
        NT_LOG_PRINT(DPM, ERR,"aonPr:%dus unaper:%dus\n\r",
                (uint32_t)(g_socpm_struct.aon_program_time_us), (uint32_t)(g_socpm_struct.unapplied_err_us));
#endif
        nt_socpm_resume_f = 1;
        nt_socpm_slp_time_total = 0;
        // Check whether Sleep exited because of sleep timer or other interrupt
        if (_socpm_slp_exit == 0) {
            /*It makes sense to remove the clock gated sleep so that idle thread would register for
             * new clockgated sleep if needed, the thread/timer which requested this may not need it anymore*/
            nt_socpm_sleep_lst_delete(0);
            if (_socpm_slp_lst_head == _INVALID_SLP_LST_HD) {
                /*set expiry to large value so that AON timer armed for this doesnt expire*/
                NT_REG_WR(QWLAN_PMU_WLAN_SLP_TMR_EXP_MSB_REG, 0xFFFFFF);
                __asm volatile("nop");
                NT_REG_WR(QWLAN_PMU_WLAN_SLP_TMR_EXP_LSB_REG, 0XFFFFFFFF);
                __asm volatile("nop");
            }
        }
        if (_socpm_mcu_sleep_wake == 1 || g_sleep_failed) {
            g_sleep_failed = false;
            extern volatile UBaseType_t uxSchedulerSuspended;
            uxSchedulerSuspended = taskSCHEDULER_SUSPENDED;
            _socpm_mcu_sleep_wake = 0;
            if (nt_twt_debug_print_enabled()) {
                UART_Send("H\r\n", 3);
            }
#ifdef SOCPM_SLEEP_DEBUG
            uint8_t lst_cnt = get_slp_lst_cnt();
            uint32_t d1 = 0, d2 = 0, d3 = 0;
            if (lst_cnt >= 1)
                d1 = (uint32_t)_socpm_slp_lst[_socpm_slp_lst_head].slp_info.slp_cb_fn;
            if (lst_cnt >= 2)
                d2 = (uint32_t)_socpm_slp_lst[_socpm_slp_lst_head].next->slp_info.slp_cb_fn;
            if (lst_cnt >= 3)
                d2 = (uint32_t)_socpm_slp_lst[_socpm_slp_lst_head].next->next->slp_info.slp_cb_fn;
            socpm_log_timestamp(_POST_WAKE, d1, d2, d3);
#endif
            FDI_NODE_STOP_NULL(FDI_DBG_PWR_S2W_IDLE_RESTORE_CONTEXT);

#if defined(FEATURE_FDI_RMC)
            for (uint32_t rmc_logs = 0; rmc_logs < fdi_rmc_get_index(); rmc_logs++) {
                FDI_NODE2LOG((g_fdi_rmc_node_ins_list[rmc_logs].attr & FDI_NODE_ATTR_ID_MASK),
                             &(g_fdi_rmc_node_ins_list[rmc_logs]));
            }
            fdi_rmc_reset_index();
#endif
            // *bss_addr = (uint32_t)cli_msg_id_temp; // restoring the wfm queue value after existing from sleep.
            /*portNVIC_SYSTICK_LOAD_REG = ( configSYSTICK_CLOCK_HZ / configTICK_RATE_HZ ) - 1UL;
            portNVIC_SYSTICK_CTRL_REG = ( portNVIC_SYSTICK_CLK_BIT | portNVIC_SYSTICK_INT_BIT |
            portNVIC_SYSTICK_ENABLE_BIT );*/

#ifdef NT_FN_CPR
            nt_cpr_init();
            uart_init();
#endif  // NT_FN_CPR
#ifdef IMAGE_FERMION
            hres_timer_post_sleep();
#endif
            __asm volatile(
                " ldr r1, =store_init_context \n"
                " ldr r0, [r1] \n"
                " msr psp, r0 \n"

                " add r1, #68 \n"
                " ldr r0, [r1] \n"
                " mov lr, r0 \n"

                " mov r2, #2 \n"
                " msr control, r2 \n"

                " mov r0, #0    \n"
                " msr basepri, r0 \n"
                " nop \n"

                //                 " add r1, #8 \n"
                //                 " ldr r0, [r1] \n"
                //                 " msr psr, r0 \n"

                "cpsie f \n"

                " add r1, #4 \n"
                " ldr r0, [r1] \n"
                " mov pc, r0 \n");
        }
        // GC:TODO
        // else
        {
            //      NT_SOCPM_IRQ_ENABLE();
        }
        __asm volatile("nop");
    }
}

#ifdef NT_TST_HEAP_COMP_CODE

void __copy_from_rram_to_ram_min(unsigned int *source_address, unsigned int *dest_address, unsigned int *block_size)
{
    // Iterate and copy word by word.
    unsigned int *src = source_address;
    unsigned int *dst = dest_address;
    unsigned int size = (unsigned int)block_size;
    unsigned int copy_count = 0;

    for (copy_count = 0; copy_count < size; copy_count = copy_count + 4) {
        *dst++ = *src++;
    }
}

void nt_do_heap_decompression(void)
{
    unsigned int *bss_end_add_4 = (&_ln_bss_end__) + 4;
    uint32_t loop_count = 0;
    unsigned int size_to_copy = (unsigned int)0x80000 - (unsigned int)(&_ln_RAM_addr_heap_start__);
    unsigned int *src = (unsigned int *)(&_ln_RAM_addr_heap_start__);

    __copy_from_rram_to_ram_min(&_ln_RF_start_addr_app_txt__, &_ln_RAM_start_addr_app_txt__,
                                &_ln_app_txt_size__);  // copying apps txt
    __copy_from_rram_to_ram_min(&_ln_RF_start_addr_app_data__, &_ln_RAM_start_addr_app_data__,
                                &_ln_app_data_size__);  // copying apps data
    __copy_from_rram_to_ram_min(&_ln_RF_start_addr_perf_txt__, &_ln_RAM_start_addr_perf_txt__,
                                &_ln_perf_txt_size__);  // copying perf text
    __copy_from_rram_to_ram_min(&_ln_RF_start_addr_perf_data__, &_ln_RAM_start_addr_perf_data__,
                                &_ln_perf_data_size__);  // copying perf data
    __copy_from_rram_to_ram_min(&_ln_RF_start_addr_data__, &_ln_RAM_start_addr_data__,
                                &_ln_data_size__);  // copying data section

    for (loop_count = 0; loop_count < size_to_copy; loop_count = loop_count + 4) {
        *src++ = *bss_end_add_4++;
    }
}

void nt_do_heap_compression(void)
{
    unsigned int *bss_end_add_4 = (&_ln_bss_end__) + 4;
    uint32_t loop_count = 0;
    unsigned int size_to_copy = (unsigned int)0x80000 - (unsigned int)(&_ln_RAM_addr_heap_start__);
    unsigned int *src = (unsigned int *)(&_ln_RAM_addr_heap_start__);

    for (loop_count = 0; loop_count < size_to_copy; loop_count = loop_count + 4) {
        *bss_end_add_4++ = *src++;
    }
}
#endif

#if defined(SUPPORT_SOC_SLEEP_SOLVER) || defined(SUPPORT_SLEEP_LIST_IMPROVEMENTS)
/*
 * @brief: This function is to choose the correct sleep mode given the left over sleep time
 * @param : sleep time. Sleep time available
 * @return : sleep_mode : the sleep mode suitable for given sleep time, this routine considers
 * only HW ramp up time currently, ideally it shd also consider the SW W2S. this is to be
 * done after characterising the SW W2S on chip
 */
static sleep_mode nt_socpm_sleep_solver(uint64_t sleep_time)
{
    sleep_mode slp_mode = clk_gtd_sleep;

    if ((sleep_time > get_sleep_exit_hw_delay(mcu_sleep)) && (sleep_time >= (_socpm_slp_time_supp_min_ms * 1000))) {
        slp_mode = mcu_sleep;
    }
#if defined(SUPPORT_SOC_SLEEP_SOLVER)
    else if (sleep_time > LIGHT_SLEEP_HW_SLEEP_TRANSITION_TIME_US) {
        slp_mode = Lightsleep;
    }
#endif /*SUPPORT_SOC_SLEEP_SOLVER*/
    return slp_mode;
}
#endif /* SUPPORT_SOC_SLEEP_SOLVER || SUPPORT_SLEEP_LIST_IMPROVEMENTS */

/*
 * @brief   Initialize MCU active state RRT and switch to active state
 * @param  : none
 * @return : none
 */
void nt_socpm_switch_mcuss_to_active(void)
{
    uint32_t value;

    value = (QWLAN_PMU_CFG_AON_CNTL_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_PMIC_DTOP_CNTL_BIT_MASK |
             QWLAN_PMU_CFG_AON_CNTL_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_XO_DTOP_CNTL_BIT_MASK |
             QWLAN_PMU_CFG_AON_CNTL_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_RFA_DTOP_CNTL_BIT_MASK |
             QWLAN_PMU_CFG_AON_CNTL_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_SON_CNTL_BIT_MASK);
    value |= _PMU_CFG_AON_CNTL_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_MEM_MX_CNTL_BIT_MEM_NOR_MASK;
    NT_REG_WR(QWLAN_PMU_CFG_AON_CNTL_MCU_ACTIVE_STATE_RESOURCE_REQ_REG, value);

#ifdef PLATFORM_FERMION
    value = (QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_CMNSS_CNTL_BIT_MASK |
             QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_CMEM_BANK_A_CNTL_BIT_MASK |
             QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_CMEM_BANK_B_CNTL_BIT_MASK |
             QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_CMEM_BANK_C_CNTL_BIT_MASK |
             QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_CMEM_BANK_D_CNTL_BIT_MASK |
             QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_CMEM_BANK_E_CNTL_BIT_MASK |
             QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_XIP_CNTL_BIT_MASK);

#ifndef EMULATION_WAR
    value |= QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_PSS_CNTL_BIT_MASK;
#endif
    NT_REG_WR(QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_REG, value);
#else  /* PLATFORM_FERMION */
    value = (QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_CMNSS_CNTL_BIT_MASK |
             QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_CMEM_BANK_A_CNTL_BIT_MASK |
             QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_CMEM_BANK_B_CNTL_BIT_MASK |
             QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_CMEM_BANK_C_CNTL_BIT_MASK |
             QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_CMEM_BANK_D_CNTL_BIT_MASK |
             QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_XIP_CNTL_BIT_MASK);
    NT_REG_WR(QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_REG, value);
#endif /* PLATFORM_FERMION */

#ifdef PLATFORM_FERMION
    /* remove the mem mx dynamic switching in active mode*/
    value = NT_REG_RD(QWLAN_PMU_DIG_TOP_CFG_REG);
    value &= ~QWLAN_PMU_DIG_TOP_CFG_CFG_MEM_MX_DYNAMIC_SWITCHING_EN_MASK;
    NT_REG_WR(QWLAN_PMU_DIG_TOP_CFG_REG, value);
#endif /*PLATFORM_FERMION*/

    // AON flush
    value = NT_REG_RD(QWLAN_PMU_AON_TOP_CFG_REG);

    // KEEP MCU Active Resumes after WFI instruction
    NT_REG_WR(QWLAN_PMU_CFG_MCU_SS_STATE_REG, NT_PMU_CFG_MCU_ACTIVE_OFFSET);  // Mcu active state
}

void vPreSleepProcessing(sleep_mode mode)
{
    /* Called by the kernel before it places the MCU into a sleep mode because
     configPRE_SLEEP_PROCESSING() is #defined to vPreSleepProcessing().

     NOTE:  Additional actions can be taken here to get the power consumption
     even lower.  For example, peripherals can be turned off here, and then back
     on again in the post sleep processing function.  For maximum power saving
     ensure all unused pins are in their lowest power state. */

    if (_socpm_slp_lst_head != _SOCPM_SLP_LST_IDX_INVALID) {
#if (defined(SUPPORT_SOC_SLEEP_SOLVER) || defined(SLEEP_CLK_CAL_IN_SLEEP_MODE))
        uint64_t curr_time = hres_timer_curr_time_us();
        uint64_t leftover_sleep_time = (_socpm_slp_lst[_socpm_slp_lst_head].slp_info.slp_time -
                                        (curr_time - _socpm_slp_lst[_socpm_slp_lst_head].slp_info.start_time_us));
#endif
#if defined(SUPPORT_SOC_SLEEP_SOLVER)
        /*its already past expiry time*/
        if (curr_time >= (_socpm_slp_lst[_socpm_slp_lst_head].slp_info.start_time_us +
                          _socpm_slp_lst[_socpm_slp_lst_head].slp_info.slp_time)) {
            mode = _socpm_slp_mode = clk_gtd_sleep;
        } else {
            /*Downgrade to lower sleep modes if there is no time to do the originally requested
             *sleep*/
            /* if TWT forces light sleep, dont run sleep solver, as it might upgrade to mcu sleep
               RRI tables would not be saved for mcu sleep when light sleep is forced*/
            if ((mode != clk_gtd_sleep) && ((nt_twt_get_forced_sleep_mode() != Lightsleep))) {
                sleep_mode old_mode = mode;
                mode = _socpm_slp_mode = nt_socpm_sleep_solver(leftover_sleep_time);
                if (old_mode != mode) {
                    NT_LOG_PRINT(SOCPM, INFO, "sleep solver old_mode (%d), new_mode (%d) sleep_time(%d)", old_mode,
                                 mode, leftover_sleep_time);
                }
            }
        }
#endif /*SUPPORT_SOC_SLEEP_SOLVER*/
#ifdef SLEEP_CLK_CAL_IN_SLEEP_MODE
        // Activities for sleep clock cal
        if (((mcu_sleep == mode) || (Lightsleep == mode)) && (leftover_sleep_time > MIN_SLP_DURATION_FOR_SLP_CLK_CAL)) {
            socpm_slp_clk_cal_presleep_activites(leftover_sleep_time);
        }
#endif /* SLEEP_CLK_CAL_IN_SLEEP_MODE */
    }


    if (mode == clk_gtd_sleep) {
        uint32_t value = NT_REG_RD(QWLAN_PMU_DIG_TOP_CFG_REG);
#if defined(PLATFORM_FERMION)
        value |= (QWLAN_PMU_DIG_TOP_CFG_FORCE_BANK_B_CORE_ON_MASK | QWLAN_PMU_DIG_TOP_CFG_FORCE_BANK_C_CORE_ON_MASK |
                  QWLAN_PMU_DIG_TOP_CFG_FORCE_BANK_D_CORE_ON_MASK | QWLAN_PMU_DIG_TOP_CFG_FORCE_BANK_E_CORE_ON_MASK);
        value |= QWLAN_PMU_DIG_TOP_CFG_RRAM_PD_MODE_DEFAULT;
#ifdef PMU_REG_RETENTION_STATUS_FOR_SOC_SLP
        /* In clk gated sleep all the DTOP REG are retained, no need to explicitly write it to register.
           Hence, we are just calling set API to set reg retention status for ANI use case */
        nt_pm_set_and_get_pmu_dtop_reg_retention_status(clk_gtd_sleep);
#endif                                                /* PMU_REG_RETENTION_STATUS_FOR_SOC_SLP */
        NT_REG_WR(QWLAN_PMU_DIG_TOP_CFG_REG, value);  // Membank retention and RRAM PD off
#else
#if defined(PLATFORM_NT) && defined(RRAM_PD_WAR)
        value = value & (~QWLAN_PMU_DIG_TOP_CFG_RRAM_PD_MODE_MASK);
        value |= (QWLAN_PMU_DIG_TOP_CFG_FORCE_BANK_A_CORE_ON_MASK | QWLAN_PMU_DIG_TOP_CFG_FORCE_BANK_B_CORE_ON_MASK |
                  QWLAN_PMU_DIG_TOP_CFG_FORCE_BANK_C_CORE_ON_MASK | QWLAN_PMU_DIG_TOP_CFG_FORCE_BANK_D_CORE_ON_MASK);
        value |= NT_PMU_DIG_TOP_CFG_RRAM_PD_MODE_NAP_MASK;
        NT_REG_WR(QWLAN_PMU_DIG_TOP_CFG_REG, value);  // WMAC and Membank retention, NAP PD mode
#else
        value |= (QWLAN_PMU_DIG_TOP_CFG_FORCE_BANK_A_CORE_ON_MASK | QWLAN_PMU_DIG_TOP_CFG_FORCE_BANK_B_CORE_ON_MASK |
                  QWLAN_PMU_DIG_TOP_CFG_FORCE_BANK_C_CORE_ON_MASK | QWLAN_PMU_DIG_TOP_CFG_FORCE_BANK_D_CORE_ON_MASK);
        value |= QWLAN_PMU_DIG_TOP_CFG_RRAM_PD_MODE_DEFAULT;
        NT_REG_WR(QWLAN_PMU_DIG_TOP_CFG_REG, value);  // WMAC and Membank retention
#endif
#endif /*PLATFORM_FERMION*/
#ifndef _SOCPM_INC_TST_MCU_ACT_CHG
        // Turn OFF wifi TX, RX and keep memory bank in retention.
        value = (QWLAN_PMU_CFG_AON_CNTL_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_PMIC_DTOP_CNTL_BIT_MASK |
                 QWLAN_PMU_CFG_AON_CNTL_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_MEM_MX_CNTL_BIT_MASK);
        NT_REG_WR(QWLAN_PMU_CFG_AON_CNTL_MCU_ACTIVE_STATE_RESOURCE_REQ_REG, value);

        value = (QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_CMNSS_CNTL_BIT_MASK |
                 QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_CMEM_BANK_A_CNTL_BIT_MASK |
                 QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_CMEM_BANK_B_CNTL_BIT_MASK |
                 QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_CMEM_BANK_C_CNTL_BIT_MASK |
                 QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_CMEM_BANK_D_CNTL_BIT_MASK |
#ifdef PLATFORM_FERMION
                 QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_CMEM_BANK_E_CNTL_BIT_MASK |
#endif /* PLATFORM_FERMION */
                 QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_XIP_CNTL_BIT_MASK |
                 QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_WLAN_MAC_CNTL_BIT_MASK |
                 QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_WLAN_PHY_TX_CNTL_BIT_MASK |
                 QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_WLAN_PHY_RXA_CNTL_BIT_MASK);
        NT_REG_WR(QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_REG, value);
#else /* _SOCPM_INC_TST_MCU_ACT_CHG */
        value = (QWLAN_PMU_CFG_AON_CNTL_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_PMIC_DTOP_CNTL_BIT_MASK |
                 QWLAN_PMU_CFG_AON_CNTL_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_XO_DTOP_CNTL_BIT_MASK |
                 QWLAN_PMU_CFG_AON_CNTL_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_RFA_DTOP_CNTL_BIT_MASK |
                 QWLAN_PMU_CFG_AON_CNTL_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_SON_CNTL_BIT_MASK);
        value |= _PMU_CFG_AON_CNTL_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_MEM_MX_CNTL_BIT_MEM_NOR_MASK;
        NT_REG_WR(QWLAN_PMU_CFG_AON_CNTL_MCU_ACTIVE_STATE_RESOURCE_REQ_REG, value);

#ifdef PLATFORM_FERMION
#if defined(TWT_WAR)
        /* we see some HW hung issues when WIFI PDs are not enabled until mcu is active.
        this is a temporary workaround and will be removed after a proper fix is found*/
        if (nt_twt_is_negotiated()) {
            value = (QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_CMNSS_CNTL_BIT_MASK |
                     QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_CMEM_BANK_A_CNTL_BIT_MASK |
                     QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_CMEM_BANK_B_CNTL_BIT_MASK |
                     QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_CMEM_BANK_C_CNTL_BIT_MASK |
                     QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_CMEM_BANK_D_CNTL_BIT_MASK |
                     QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_CMEM_BANK_E_CNTL_BIT_MASK |
                     QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_XIP_CNTL_BIT_MASK |
                     QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_WLAN_MAC_CNTL_BIT_MASK |
                     QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_WLAN_PHY_TX_CNTL_BIT_MASK |
                     QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_WLAN_PHY_RXA_CNTL_BIT_MASK |
                     QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_WLAN_PHY_RXTOP_CNTL_BIT_MASK |
                     QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_CXC_CNTL_BIT_MASK);
        } else
#endif /*TWT_WAR*/
        {
            value = (QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_CMNSS_CNTL_BIT_MASK |
                     QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_CMEM_BANK_A_CNTL_BIT_MASK |
                     QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_CMEM_BANK_B_CNTL_BIT_MASK |
                     QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_CMEM_BANK_C_CNTL_BIT_MASK |
                     QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_CMEM_BANK_D_CNTL_BIT_MASK |
                     QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_CMEM_BANK_E_CNTL_BIT_MASK |
                     QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_QSPI_CNTL_BIT_MASK |
                     QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_XIP_CNTL_BIT_MASK);
        }

#ifndef EMULATION_WAR
        value |= QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_PSS_CNTL_BIT_MASK;
#endif
        NT_REG_WR(QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_REG, value);
#else  /* PLATFORM_FERMION */
        value = (QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_CMNSS_CNTL_BIT_MASK |
                 QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_CMEM_BANK_A_CNTL_BIT_MASK |
                 QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_CMEM_BANK_B_CNTL_BIT_MASK |
                 QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_CMEM_BANK_C_CNTL_BIT_MASK |
                 QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_CMEM_BANK_D_CNTL_BIT_MASK |
                 QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_XIP_CNTL_BIT_MASK);
        NT_REG_WR(QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_REG, value);
#endif /* PLATFORM_FERMION */

#endif /* _SOCPM_INC_TST_MCU_ACT_CHG */

        // KEEP MCU Active Resumes after WFI instruction
        NT_REG_WR(QWLAN_PMU_CFG_MCU_SS_STATE_REG, NT_PMU_CFG_MCU_ACTIVE_OFFSET);  // Mcu active state
    } else if (mode == mcu_sleep) {
#ifdef NT_TST_HEAP_COMP_CODE
        nt_do_heap_compression();
#endif
        _socpm_slpcfg_mcuslp();

#ifdef NT_GPIO_FLAG
        // nt_gpio_pin_write(NT_GPIOA, GPIO_PIN_5, NT_GPIO_LOW);
#endif
    }
#if defined(PLATFORM_FERMION)
    else if (mode == Lightsleep) {
        _socpm_slpcfg_light();
    }
#endif /*PLATFORM_FERMION*/
    else if (mode == Standby) {
        _socpm_slpcfg_sby();
    }
}
/*-----------------------------------------------------------*/

uint64_t vPostSleepProcessing(void)
{
    return 0;
}
/*-----------------------------------------------------------*/

void power_cycle_wmac()
{
    extern uint32_t g_wifi_ss_delay;
    uint32_t mac_gdscr;
    mac_gdscr = HAL_REG_RD(QWLAN_PMU_WLAN_MAC_GDSCR_REG);
    mac_gdscr |= QWLAN_PMU_WLAN_MAC_GDSCR_COLLAPSE_EN_SW_MASK;
    mac_gdscr &= (~QWLAN_PMU_WLAN_MAC_GDSCR_HW_CONTROL_MASK);
    HAL_REG_WR(QWLAN_PMU_WLAN_MAC_GDSCR_REG, mac_gdscr);

    for (uint32_t i = 0; i < g_wifi_ss_delay; i++)
    {
        mac_gdscr = HAL_REG_RD(QWLAN_PMU_WLAN_MAC_GDSCR_REG);
        if(!(mac_gdscr& QWLAN_PMU_WLAN_MAC_GDSCR_GDS_CTL_PWR_STATUS_MASK)) 
            break;
        nt_socpm_nop_delay(1);
    }

    mac_gdscr = HAL_REG_RD(QWLAN_PMU_WLAN_MAC_GDSCR_REG);
    mac_gdscr &= (~QWLAN_PMU_WLAN_MAC_GDSCR_COLLAPSE_EN_SW_MASK);
    mac_gdscr |= (QWLAN_PMU_WLAN_MAC_GDSCR_HW_CONTROL_MASK);
    HAL_REG_WR(QWLAN_PMU_WLAN_MAC_GDSCR_REG, mac_gdscr);

    for (uint32_t i = 0; i < g_wifi_ss_delay; i++)
    {
        mac_gdscr = HAL_REG_RD(QWLAN_PMU_WLAN_MAC_GDSCR_REG);
        if(mac_gdscr& QWLAN_PMU_WLAN_MAC_GDSCR_GDS_CTL_PWR_STATUS_MASK) 
            break;
        nt_socpm_nop_delay(1);
    }

}

void nt_socpm_slp_enter(uint64_t slp_us)
{
    nt_socpm_slp_tmr_set(slp_us);
    FDI_RMC_INS_STOP_NULL(FDI_DBG_PWR_S2W_WARM_BOOT_CB_SLEEP);

#ifdef SUPPORT_COEX
#ifdef PLATFORM_FERMION
    COEX_TEST_PRINT(COEX_SDM_WK_AFTER_BCN);
#endif
#endif

#ifdef NT_FN_CPR
    vPreSleepProcessingPartialSleep();
#else
    vPreSleepProcessing(_socpm_slp_mode);
#endif  // NT_FN_CPR
    
    uint32_t lic_int_status = NT_REG_RD(QWLAN_PMU_AON_LIC_INT_STAT_REG);
    uint32_t ext_int = NT_REG_RD(NVIC_ICPR1) ;
    if ((lic_int_status & QWLAN_PMU_AON_LIC_INT_STAT_EXT_WAKEUP_INTR_STAT_RAW_MASK )|| (ext_int& (A2F_ASSERT_INTR_NVIC1_MASK))) 
    {
        HWIO_OUTXF(SEQ_WCSS_PMU_OFFSET, NEUTRINO_PMU_PRONTO_LP_FRODO_PMU_AON_LIC_INT_CLR, EXT_WAKEUP_INTR_CLR, 1);
        HWIO_OUTXF(SEQ_WCSS_PMU_OFFSET, NEUTRINO_PMU_PRONTO_LP_FRODO_PMU_AON_LIC_INT_CLR, EXT_WAKEUP_INTR_CLR, 0);

        NT_REG_WR(NVIC_ICPR1, A2F_ASSERT_INTR_NVIC1_MASK);
    }

    __asm volatile("dsb" ::: "memory");
    __asm volatile("wfi");
    // nops added to avoid issue due to cortex prefetch
    __asm volatile(" nop \n");
    __asm volatile(" nop \n");
    __asm volatile(" nop \n");
    
    // Delay accessing other CMEM banks/sub-banks to avoid power inrush issues
    for (int i = 0; i < 7; i++) {
        __asm volatile(" nop \n");
    }
    extern int rri_force_wakeup;
    extern volatile uint32_t last_rri;
    rri_force_wakeup = last_rri = 0x0;
    hal_wlan_sleep_trimmed();


    /* Check if sleep entry was prevented and assert if not a valid prevention */
    nt_socpm_check_sleep_entry_failure(_socpm_slp_mode, FALSE,TRUE);
}

static uint64_t nt_socpm_slp_tmr_get(void)
{
    volatile uint64_t time;
    time = _SOCPM_SLP_TMR_MSB;
    time = ((_SOCPM_SLP_TMR_LSB) | (time << 32));
#if defined(EMULATION_BUILD) && defined(PLATFORM_FERMION)
    time /= _slp_clk_scaling;
#endif  // defined(EMULATION_BUILD) && defined(PLATFORM_FERMION)
#ifdef APPLY_SLEEP_CLK_CORRECTION
#if (FERMION_CHIP_VERSION == 1)
    if (g_socpm_struct.slp_clk_cal_params.slp_clk_cal_enabled_mode == ACTIVE_MODE)
        time = time * g_socpm_struct.slp_clk_cal_params.xocnt / g_socpm_struct.slp_clk_cal_params.refxocnt;
#endif
#endif /* APPLY_SLEEP_CLK_CORRECTION */

#if defined(COMPENSATE_RC_DIVISION_ERROR_WAR)
    /* Compensating the right shift division error of RC clock with slp timer time */
    time = COMPENSATE_RC_DIVISION_ERROR_SLP_TMR_GET(time);
#endif /* COMPENSATE_RC_DIVISION_ERROR_WAR */

    return time;
}

uint64_t nt_socpm_get_slp_tmr_us(void)
{
    uint64_t slept_time = 0;
    uint64_t aon_slp_tmr_raw = nt_socpm_slp_tmr_get();
#ifdef PLATFORM_FERMION
    slept_time = _SOCPM_RC_OR_EXT_CLK_AON_TICK_TO_US(aon_slp_tmr_raw);
#else
    // Add logic to check sleep clock source
    if (_socpm_slp_clk_src == NT_SOCPM_SLP_CLK_RFAXO)
        slept_time = _SOCPM_XO_CLK_AON_TICK_TO_US(aon_slp_tmr_raw);
    else
        slept_time = _SOCPM_RC_OR_EXT_CLK_AON_TICK_TO_US(aon_slp_tmr_raw);
#endif  // PLATFORM_FERMION
    return slept_time;
}

// Decide which Sleep state to achieve
/*sleep_mode
_socpm_slpmode_get(
    uint64_t slp_time)
{
    //Validate Sleep Time
    extern int nt_socpm_sby_force;
    extern int mcu_sleep_force;

    if (nt_socpm_sby_force == 1)
    {
        //nt_socpm_sby_force = 0;
        return Standby;
    }

    if (mcu_sleep_force == 1)
    {
        mcu_sleep_force = 0;
        return mcu_sleep;
    }

    if (slp_time < _SOCPM_MAX_ACTIVE)
    {
        return Active;
    }
    else if (slp_time > _SOCPM_MAX_STANDBY)
    {
        //Check whether Sleep time expiresz the Maximum Sleep Value Configurable

        @to-do need to think whether we can compensate the extra time
         * by entering sleep again

        slp_val = _SOCPM_MAX_STANDBY;
        return Standby;
    }
    else if (slp_time < _SOCPM_MAX_CLK_SLEEP)
    {
        //CLock Gated Sleep
        return clk_gtd_sleep;
    }
    else if (slp_time < _SOCPM_MAX_SLEEP)
    {
        //MCU Sleep
        return mcu_sleep;
    }
    else if (slp_time >= 0x100000000)
    {
        //Standby
        return Standby;
    }
    return Active;
}*/

// Idle Task Stack Store

__attribute__((naked)) void _socpm_ctxt_save(void)
{
    __asm volatile(
        " isb \n"
        // " ldr r3, pxCurrentTCBConst1 \n" /* Get the location of the current TCB. */
        // " ldr r2, [r3] \n"
        // " ldr r0, [r2] \n"
        /** The previous logic which has been disabled was loading the idle task TCB's pxTopOfStack
         * into r0 and uses it as stack pointer for save/restore. But this is incorrect as pxTopOfStack
         * does not reflect the actual stack pointer at the time of this function call.
         * This logic has been updated now to read the stack pointer value directly into r0 and use the
         * same for storing other core register values as well as on context restore.
         */
        "mov r0, sp \n"
        // @TO-DO For FPU Register save " \n"
        // " tst r14, #0x10 \n" /* Is the task using the FPU context?  If so, push high vfp registers. */
        // " it eq \n"
        // " vstmdbeq r0!, {s16-s31} \n"
        " \n"
        /** The following stmdb operation results in r0 being decremented by 36 to store the 9 core regs.
         * As a result, the value stored in nt_socpm_m4_regs[0] is 36 less than the actual stack pointer.
         * To compensate for this, 36 is added to that value before copying the same to psp in ctxt_restore.
         */
        " stmdb r0!, {r4-r11, r14} \n" /* Save the core registers. */
        //          " str r0, [r2] \n" /* Save the new top of stack into the first member of the TCB. */
        " \n"
        " ldr r1,=nt_socpm_m4_regs             \n"
        //          " mrs r0, msp \n"
        " str r0, [r1] \n"
        " add r1, 4 \n"
        " add r1, 36 \n"
        " stmdb r1!, {r4-r11, r14} \n" /* Save the core registers. */
        " isb \n "
        " wfi \n "
        " nop \n "
        " nop \n "
        " nop \n "
        " bx r14 \n"
        " .align 4 \n"
        "pxCurrentTCBConst1: .word pxCurrentTCB \n");
}

__attribute__((naked)) void nt_socpm_ctxt_restore(void)
{
    _socpm_mcu_sleep_wake = 1;
#ifdef SOCPM_RMC_DBG
    g_socpm_struct.nt_socpm_ctxt_restore_ctr++;
#endif /* SOCPM_RMC_DBG */
    __asm volatile(
        " ldr r1,=nt_socpm_m4_regs            \n"
        " ldr r0, [r1] \n"
        /* Add 36 to nt_socpm_m4_regs[0] to compensate for the decrement during stmdb in ctxt_save */
        " add r0, 36 \n"
        " add r1, 40 \n"
        " \n"
        " ldmdb r1!, {r4-r11, r14} \n" /* Pop the core registers. */
        " \n"
        " \n"
        " msr psp, r0 \n"
        " mov r0, #2 \n"
        " msr control, r0 \n"
        " dsb                   \n"
        " isb                   \n"
        " bx r14 \n");
}

uint8_t mem_bank_check(uint32_t bank, Mem_Control type, sleep_mode mode)
{
    uint32_t Read_value;
    uint32_t Resorce_Reg = QWLAN_PMU_CFG_MCU_SLEEP_STATE_RESOURCE_REQ_REG;
    uint8_t Status = NT_OK;

    // Select Which Resource Register To program
    switch (mode) {
        case mcu_sleep:
#ifdef PLATFORM_FERMION
        case Lightsleep:
#endif
            Resorce_Reg = QWLAN_PMU_CFG_MCU_SLEEP_STATE_RESOURCE_REQ_REG;
            break;
        case Standby:
#ifdef PLATFORM_FERMION
        case InfDeepsleep:
#endif
            Resorce_Reg = QWLAN_PMU_CFG_MCU_DEEPSLEEP_STATE_RESOURCE_REQ_REG;
            break;
        case clk_gtd_sleep:  // Both Clock gated Sleep and Active will use the same Resources Register
        case Active:
            Resorce_Reg = QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_REG;
            break;
    }
    switch (bank) {
        case _SOCPM_MBANK_A:
            // Write The Data
            NT_REG_WR(_SOCPM_MBANK_A_CHK, 0xAAAA5555);
            // Force BANK A User passed Memory State
            _socpm_mem_wr_drv(QWLAN_PMU_COMMON_CMEM_BANK_A_CBCR_REG, QWLAN_PMU_CMEM_BANK_A_GDSCR_REG, Resorce_Reg,
                              _SOCPM_MBANK_A, type);
            // Read The Data
            Read_value = NT_REG_RD(_SOCPM_MBANK_A_CHK);
            if (Read_value != 0xAAAA5555)
                Status = NT_EFAIL;
            break;
        case _SOCPM_MBANK_B:
            // Write The Data
            NT_REG_WR(_SOCPM_MBANK_B_CHK, 0xAAAA5555);
            // Force BANK B User passed Memeory State
            _socpm_mem_wr_drv(QWLAN_PMU_COMMON_CMEM_BANK_B_CBCR_REG, QWLAN_PMU_CMEM_BANK_B_GDSCR_REG, Resorce_Reg,
                              _SOCPM_MBANK_B, type);
            // Read The Data
            Read_value = NT_REG_RD(_SOCPM_MBANK_B_CHK);
            if (Read_value != 0xAAAA5555)
                Status = NT_EFAIL;
            break;
        case _SOCPM_MBANK_C:
            // Write The Data
            NT_REG_WR(_SOCPM_MBANK_C_CHK, 0xAAAA5555);
            // Force BANK C User passed Memeory State
            _socpm_mem_wr_drv(QWLAN_PMU_COMMON_CMEM_BANK_C_CBCR_REG, QWLAN_PMU_CMEM_BANK_C_GDSCR_REG, Resorce_Reg,
                              _SOCPM_MBANK_C, type);
            // Read The Data
            Read_value = NT_REG_RD(_SOCPM_MBANK_C_CHK);
            if (Read_value != 0xAAAA5555)
                Status = NT_EFAIL;
            break;
        case _SOCPM_MBANK_D:
            // Write The Data
            NT_REG_WR(_SOCPM_MBANK_D_CHK, 0xAAAA5555);
            // Force BANK D User passed Memeory State
            _socpm_mem_wr_drv(QWLAN_PMU_COMMON_CMEM_BANK_D_CBCR_REG, QWLAN_PMU_CMEM_BANK_D_GDSCR_REG, Resorce_Reg,
                              _SOCPM_MBANK_D, type);
            // Read The Data
            Read_value = NT_REG_RD(_SOCPM_MBANK_D_CHK);
            if (Read_value != 0xAAAA5555)
                Status = NT_EFAIL;
            break;
    }
    return Status;
}

static void _socpm_mem_wr_drv(uint32_t Control_Reg, uint32_t Status_Reg, uint32_t Resource_Reg, uint32_t Resource_Value,
                              Mem_Control type)
{
    uint32_t Read_Value;

    switch (type) {
        case Off:
            // Read Control And Status Register GDSCR of Selected CMEM Bank
            Read_Value = NT_REG_RD(Status_Reg);
            // Check Whether The CMEMBANK Is turned On Using Power Satus Bit 31 bit​
            if (!(Read_Value & QWLAN_PMU_CMEM_BANK_A_GDSCR_GDS_CTL_PWR_STATUS_MASK)) {
                // Turn ON The Mem BANK In Resource Table
                _SOCPM_REG_RW(Resource_Reg, Resource_Value);
                // OR
                // Clear HW Ctrl
                _SOCPM_REG_RW_CLR(Status_Reg, (QWLAN_PMU_CMEM_BANK_D_GDSCR_HW_CONTROL_MASK |
                                               QWLAN_PMU_CMEM_BANK_D_GDSCR_COLLAPSE_EN_SW_MASK));
                // Set SWA Ctrl
                //_SOCPM_REG_RW(Status_Reg,QWLAN_PMU_CMEM_BANK_D_GDSCR_SW_OVERRIDE_MASK);
                // Wait For The Power ON Status
                while (!((NT_REG_RD(Status_Reg)) & QWLAN_PMU_CMEM_BANK_A_GDSCR_GDS_CTL_PWR_STATUS_MASK))
                    ;  // The MAsk Bit Same For all The Individual Banks
            }
            // Turn OFF The Mem BANK In Resource Table
            _SOCPM_REG_RW(Control_Reg, QWLAN_PMU_COMMON_CMEM_BANK_D_CBCR_FORCE_MEM_CORE_ON_MASK);
            _SOCPM_REG_RW_CLR(Resource_Reg, Resource_Value);
            _SOCPM_REG_RW_CLR(Status_Reg, (QWLAN_PMU_CMEM_BANK_D_GDSCR_HW_CONTROL_MASK));
            // Set SWA Ctrl
            _SOCPM_REG_RW(Status_Reg, QWLAN_PMU_CMEM_BANK_D_GDSCR_COLLAPSE_EN_SW_MASK);
            // Turn Off the Bank
            _SOCPM_REG_RW_CLR(Resource_Reg, Resource_Value);
            NT_REG_WR(0x11a600, 0X3);  // Mcu active state
            // Wait For The Power OFF Status
            while (((NT_REG_RD(Status_Reg)) & QWLAN_PMU_CMEM_BANK_A_GDSCR_GDS_CTL_PWR_STATUS_MASK))
                ;
            break;
        case On:
            // Read Control And Status Register GDSCR of Guven CMEM Bank
            Read_Value = NT_REG_RD(Status_Reg);
            // Check Whether The CMEMBANK Is turned On Using Power Satus Bit 31 bit
            if (!(Read_Value & QWLAN_PMU_CMEM_BANK_A_GDSCR_GDS_CTL_PWR_STATUS_MASK)) {
                // Turn ON The Mem BANK In Resource Table
                _SOCPM_REG_RW(Resource_Reg, Resource_Value);
                // Clear HW Ctrl
                _SOCPM_REG_RW_CLR(Status_Reg, (QWLAN_PMU_CMEM_BANK_D_GDSCR_HW_CONTROL_MASK |
                                               QWLAN_PMU_CMEM_BANK_D_GDSCR_COLLAPSE_EN_SW_MASK));
                // Set SWA Ctrl
                //_SOCPM_REG_RW(Status_Reg,(QWLAN_PMU_CMEM_BANK_D_GDSCR_SW_OVERRIDE_MASK));
                // Wait For The Power ON Status
                while (!((NT_REG_RD(Status_Reg)) & QWLAN_PMU_CMEM_BANK_A_GDSCR_GDS_CTL_PWR_STATUS_MASK))
                    ;  // The MAsk Bit Same For all The Individual Banks
            }
            // Put Mem Bank In Active By Turning OFF Retention
            _SOCPM_REG_RW_CLR(Control_Reg,
                              QWLAN_PMU_COMMON_CMEM_BANK_A_CBCR_FORCE_MEM_PERIPH_OFF_MASK);  // The MAsk Bit Same For
                                                                                             // all The Individual Banks
            break;
        case Retention:
            // Read Control And Status Register GDSCR of Guven CMEM Bank
            Read_Value = NT_REG_RD(Status_Reg);
            // Check Whether The CMEMBANK Is turned On Using Power Satus Bit 31 bit
            if (!(Read_Value & QWLAN_PMU_CMEM_BANK_A_GDSCR_GDS_CTL_PWR_STATUS_MASK)) {
                // Turn ON The Mem BANK In Resource Table
                _SOCPM_REG_RW(Resource_Reg, Resource_Value);
                // Clear HW Ctrl
                _SOCPM_REG_RW_CLR(Status_Reg, (QWLAN_PMU_CMEM_BANK_D_GDSCR_HW_CONTROL_MASK |
                                               QWLAN_PMU_CMEM_BANK_D_GDSCR_COLLAPSE_EN_SW_MASK));
                // Set SWA Ctrl
                //(Status_Reg,(QWLAN_PMU_CMEM_BANK_D_GDSCR_SW_OVERRIDE_MASK));
                // Wait For The Power ON Status
                while (!((NT_REG_RD(Status_Reg)) & QWLAN_PMU_CMEM_BANK_A_GDSCR_GDS_CTL_PWR_STATUS_MASK))
                    ;
            }
            // Put Mem Bank In Retention​
            _SOCPM_REG_RW(Control_Reg,
                          QWLAN_PMU_COMMON_CMEM_BANK_A_CBCR_FORCE_MEM_PERIPH_OFF_MASK |
                              QWLAN_PMU_COMMON_CMEM_BANK_A_CBCR_FORCE_MEM_CORE_ON_MASK);  // The MAsk Bit Same For all
                                                                                          // The Individual Banks​
            break;
    }
}

/*
 * @brief  Compute sleep time adjusted for sleep slop offset
 * @param  : sleep_time_us -> Sleep time requested in microseconds
 * @return : Adjusted sleep time in microseconds
 */
static uint64_t _socpm_get_sleep_slop_adjusted_sleep_time(uint64_t sleep_time_us)
{
    /* Sleep slop offset is the offset time which accounts for clock drifts
     * between the AP and STA, to ensure that protocol wakeups occur on time.
     */

    uint64_t sleep_slop_offset_us = 0;
    uint64_t sleep_time_ms = US_TO_MS(sleep_time_us);
    if (gdevp) {
        PM_STRUCT *pPmStruct = (PM_STRUCT *)gdevp->pPmStruct;
        // Fixed pre sleep time accounting for HW delays and base early rx
        uint32_t bmps_s2w_compensation = bmps_compute_s2w_compensation_time(pPmStruct) + SLP_TIME_CALC_TO_AON_PRGM_US;
        // _minprintf("s2w", bmps_s2w_compensation , (unsigned int) bmps_s2w_compensation);
        uint64_t bcn_pre_wake = bmps_s2w_compensation + nt_pm_get_bmps_beacon_early_rx(gdevp);
        if (sleep_time_ms < g_socpm_struct.slop_interval_ms) {
            sleep_slop_offset_us = 100;
        } else {
            sleep_slop_offset_us = (((sleep_time_ms + US_TO_MS(bcn_pre_wake)) / g_socpm_struct.slop_interval_ms)) *
                                   g_socpm_struct.slop_step_us;
        }

        /* Cap sleep slop offset to upper limit */
        if (sleep_slop_offset_us > SLEEP_SLOP_OFFSET_UPPER_LIMIT_US) {
            sleep_slop_offset_us = SLEEP_SLOP_OFFSET_UPPER_LIMIT_US;
        }
    }

    return ((sleep_time_us > sleep_slop_offset_us) ? (sleep_time_us - sleep_slop_offset_us) : sleep_time_us);
}

static void _socpm_slp_fn_process(sleep_mode mode)
{
    if (mode != 0) {
        int list_no = _socpm_slp_lst_head;
        if (list_no == _INVALID_SLP_LST_HD) {
            /** Empty list. This can occur when AON timer was armed for a sleep
             * list entry but then the entry was deleted from the list, causing
             * the list to get empty.
             */
            return;
        }
        if (&_socpm_slp_lst[list_no].slp_info != NULL) {
            uint64_t head_sleep_back = 0;
            bool other_entries_need_wakeup = FALSE;
            uint64_t slept_time = 0;
            uint64_t delta_time_us = 0;
            uint32_t wkup_delay_us = 0;

            slept_time = _socpm_slp_lst[list_no].slp_info.slp_time;
            delta_time_us = nt_socpm_get_slp_tmr_us();
            if (delta_time_us > slept_time) {
                wkup_delay_us = delta_time_us - slept_time;
            }

            // head_sleep_back = _socpm_slp_lst[list_no].slp_info.min_cb_fn(wkup_delay_us);

            _socpm_slp_lst[list_no].slp_info.slp_time = _socpm_get_sleep_slop_adjusted_sleep_time(head_sleep_back);

#if defined(SUPPORT_SLEEP_LIST_IMPROVEMENTS)
            _socpm_slp_lst[_socpm_slp_lst_head].slp_info.start_time_us = hres_timer_curr_time_us();
#endif /* SUPPORT_SLEEP_LIST_IMPROVEMENTS */

            other_entries_need_wakeup = nt_socpm_sleep_lst_update(slept_time, true);
            list_no = _socpm_slp_lst_head;

#ifdef SOCPM_SLEEP_DEBUG
            if (_socpm_slp_mode == mcu_sleep) {
                socpm_log_timestamp(AON_PROC_1, head_sleep_back, delta_time_us, slept_time);
            }
#endif

            if ((head_sleep_back <= 0) || (other_entries_need_wakeup == TRUE)) {
                _socpm_slp_lst_wkup_item_t *p_next, *p_curr;
                sleep_mode hd_slp_mode = _socpm_slp_lst[list_no].slp_info.slp_mode;
                p_next = _socpm_slp_lst[list_no].next;
                /** Previously, on wakeup from sleep, the wkup_cb of only the lst_head
                 * entry was invoked. This lead to errors due to a corner case when sleep
                 * list inversion occurs,  i.e. the device enters sleep with one entry at
                 * the head but exits with another entry at the head.
                 * The specific case involved the device entering MCU sleep with BMPS at
                 * list head. During BMPS, clk_gtd_slp came to the head when slp_lst_reorder
                 * was done after beacon processing and the device was being put back to
                 * sleep. As a result, on full wakeup, only the clk_gtd_slp wkup_cb was
                 * executed, putting the BMPS state machine in a bad state which lead to
                 * device crash.
                 * To resolve this behaviour, the wkup_cb of all entries in the list
                 * need to be executed, and not only the entry at list head. To
                 * differentiate between completion of the sleep duration and a premature
                 * abort of the sleep due to a wakeup from another entry, the appropriate
                 * wkup_reason is passed to the wkup_cb. The choice of what needs to be
                 * done in case of WKUP_COMPLETE and WKUP_ABORT is left to the individual
                 * callbacks.
                 */

#ifdef SUPPORT_SLEEP_DEBUG_UNIT_TEST_CMD
                /* Track the list index to maintain the dummy sleep nodes in the sleep list */
                if (g_socpm_struct.socpm_dbg_unit_test_value & (1 << SLP_DBG_SOCPM_SLP_LIST_ADD)) {
                    g_socpm_struct.socpm_dbg_curr_lst_idx = list_no;
                }
#endif /* SUPPORT_SLEEP_DEBUG_UNIT_TEST_CMD */
                _socpm_slp_lst[list_no].slp_info.wkup_cb_fn(SOC_WKUP_COMPLETE);
#ifdef SOCPM_SLEEP_DEBUG
                if (hd_slp_mode != clk_gtd_sleep) {
                    socpm_log_timestamp(AON_PROC_2, other_entries_need_wakeup, 0, 0);
                }
#endif
                if (((hd_slp_mode != clk_gtd_sleep) || (_socpm_mcu_sleep_wake == 1)) ||
                    (list_no != _socpm_slp_list_idx_rtos)) {
                    while (p_next != NULL) {
                        p_curr = p_next;
                        p_next = p_curr->next;
#ifdef SUPPORT_SLEEP_DEBUG_UNIT_TEST_CMD
                        /* Track the list index to maintain the dummy sleep nodes in the sleep list */
                        if (g_socpm_struct.socpm_dbg_unit_test_value & (1 << SLP_DBG_SOCPM_SLP_LIST_ADD)) {
                            g_socpm_struct.socpm_dbg_curr_lst_idx = p_curr->slp_info.list_no;
                        }
#endif /* SUPPORT_SLEEP_DEBUG_UNIT_TEST_CMD */
                        p_curr->slp_info.wkup_cb_fn(SOC_WKUP_ABORT);
                    }
                }
            } else {
                _socpm_slp_lst[list_no].slp_info.slp_cb_fn();
                nt_socpm_sleep_lst_reorder(_socpm_slp_lst_head, slept_time);
            }
#ifdef SOCPM_SLEEP_DEBUG
            if (_socpm_slp_mode == mcu_sleep) {
                uint8_t lst_cnt = get_slp_lst_cnt();
                uint32_t d1 = 0, d2 = 0;
                if (lst_cnt >= 1)
                    d1 = (uint32_t)_socpm_slp_lst[_socpm_slp_lst_head].slp_info.slp_cb_fn;
                if (lst_cnt >= 2)
                    d2 = (uint32_t)_socpm_slp_lst[_socpm_slp_lst_head].next->slp_info.slp_cb_fn;
                socpm_log_timestamp(AON_PROC_3, other_entries_need_wakeup, d1, d2);
            }
#endif
        }
    }
}

bool nt_socpm_sleep_lst_update(uint64_t sleep_time, bool serve_multi_node_wkup)
{
    (void)sleep_time;
    static _socpm_slp_lst_wkup_item_t *slp_lst_node;
    bool other_entries_need_wakeup = FALSE;
    slp_lst_node = &_socpm_slp_lst[_socpm_slp_lst_head];
#if defined(SUPPORT_SLEEP_LIST_IMPROVEMENTS)
    /* The elapsed time will be calculated on the basis of AON sleep timer and start
     * time of the sleep list head, which properly compensate the sleep time of the sleep list
     */
    uint64_t current_time_us = hres_timer_curr_time_us();

    while (slp_lst_node != NULL && (current_time_us >= slp_lst_node->slp_info.start_time_us)) {
        uint64_t delta_time_us = current_time_us - slp_lst_node->slp_info.start_time_us;
        if (delta_time_us < slp_lst_node->slp_info.slp_time && slp_lst_node->slp_info.slp_time > 0) {
#ifdef SUPPORT_SLEEP_DEBUG_UNIT_TEST_CMD
            /* Logging to understand the sleep list compensation, while handling two or more nodes in the sleep list */
            if (g_socpm_struct.socpm_dbg_unit_test_value & (1 << SLP_DBG_SOCPM_SLP_LIST_COMP_LOG)) {
                NT_LOG_PRINT(SOCPM, ERR,
                             "Compensating existing slp lst node(idx: %d), start_time (%u), current time (%u), elapsed "
                             "time (%u)",
                             slp_lst_node->slp_info.list_no, (unsigned int)slp_lst_node->slp_info.start_time_us,
                             (unsigned int)current_time_us, (unsigned int)delta_time_us);
            }
#endif /* SUPPORT_SLEEP_DEBUG_UNIT_TEST_CMD */
            slp_lst_node->slp_info.slp_time = slp_lst_node->slp_info.slp_time - delta_time_us;
            slp_lst_node->slp_info.start_time_us = hres_timer_curr_time_us();
        } else {
            if (slp_lst_node != &_socpm_slp_lst[_socpm_slp_lst_head] && serve_multi_node_wkup) {
                uint32_t wkup_delay_us = 0;
                uint64_t sleep_back = 0;
                if (delta_time_us > slp_lst_node->slp_info.slp_time) {
                    wkup_delay_us = delta_time_us - slp_lst_node->slp_info.slp_time;
                }
                /* Handling the multiple entry of the sleep list with wkup delay */
                if (g_socpm_struct.in_warm_boot == TRUE)
                    sleep_back = slp_lst_node->slp_info.min_cb_fn(wkup_delay_us);

                if (sleep_back <= 0) {
                    slp_lst_node->slp_info.slp_time = 0;
                    other_entries_need_wakeup = TRUE;
#ifdef SUPPORT_SLEEP_DEBUG_UNIT_TEST_CMD
                    /* Track the list index to maintain the dummy sleep nodes in the sleep list */
                    if (g_socpm_struct.socpm_dbg_unit_test_value & (1 << SLP_DBG_SOCPM_SLP_LIST_ADD)) {
                        g_socpm_struct.socpm_dbg_curr_lst_idx = slp_lst_node->slp_info.list_no;
                    }
#endif /* SUPPORT_SLEEP_DEBUG_UNIT_TEST_CMD */
                    slp_lst_node->slp_info.wkup_cb_fn(SOC_WKUP_COMPLETE);
                }
            }
        }
        slp_lst_node = slp_lst_node->next;
    }
#else
    (void)serve_multi_node_wkup;
    while (slp_lst_node->next != NULL) {
        if (slp_lst_node->next->slp_info.slp_time >= sleep_time)
            slp_lst_node->next->slp_info.slp_time -= sleep_time;
        else
            slp_lst_node->next->slp_info.slp_time = 0;
        slp_lst_node = slp_lst_node->next;
    }
#endif /* SUPPORT_SLEEP_LIST_IMPROVEMENTS */

    return other_entries_need_wakeup;
}

// remove the first sleep entry and point the head to the next in the list
static void _socpm_list_search(void)
{
    int count = 0;
    int old_hd_idx = _socpm_slp_lst_head;

    // nothing else in the list
    if (_socpm_slp_lst[old_hd_idx].next == NULL) {
        // Set to _INVALID_SLP_LST_HD to indicate that list is empty
        _socpm_slp_lst_head = _INVALID_SLP_LST_HD;
        return;
    }

    for (count = 0; count < _SOCPM_SLP_LST_SZ; count++) {
        if (_socpm_slp_lst[old_hd_idx].next ==
            &_socpm_slp_lst[count]) {  // remove the first entry and move the head to the next item
            _socpm_slp_lst_head = count;
            _socpm_slp_lst[old_hd_idx].next = NULL;
            break;
        }
    }
}

void nt_socpm_sleep_lst_reorder(int list_idx, uint64_t head_prev_sleep_time)
{
    _socpm_slp_lst_wkup_item_t *ptmp;
    int old_sleep_list_head = _socpm_slp_lst_head;

    if (_socpm_slp_lst_head == list_idx) {
        _socpm_list_search();
    }

    if (_socpm_slp_lst_head != _INVALID_SLP_LST_HD) {
        ptmp = &_socpm_slp_lst[_socpm_slp_lst_head];
    }

    if (_socpm_slp_lst_head == _INVALID_SLP_LST_HD) {
        // List is empty. So place entry at head of the list
        _socpm_slp_lst_head = list_idx;
    } else if (((_socpm_slp_lst[_socpm_slp_lst_head].slp_info.slp_time < 1) && (_socpm_slp_lst_head != list_idx)) ||
               ((_socpm_slp_lst[_socpm_slp_lst_head].slp_info.slp_time > _socpm_slp_lst[list_idx].slp_info.slp_time) &&
                (_socpm_slp_lst[list_idx].slp_info.slp_time > 0))) {
        if (_socpm_slp_lst[_socpm_slp_lst_head].next == &_socpm_slp_lst[list_idx])
            _socpm_slp_lst[_socpm_slp_lst_head].next = NULL;
        _socpm_slp_lst[list_idx].next = &_socpm_slp_lst[_socpm_slp_lst_head];
        _socpm_slp_lst_head = list_idx;
    } else if (ptmp->next != NULL) {
        while (ptmp->next != NULL) {
            if (ptmp->next->slp_info.slp_time > _socpm_slp_lst[list_idx].slp_info.slp_time) {
                _socpm_slp_lst_wkup_item_t *pswap;
                pswap = ptmp->next;
                // memcpy(&tempexchange, ptmp->next, sizeof(_socpm_slp_lst_wkup_item_t));
                //&tempexchange=Temp->next;
                ptmp->next = &_socpm_slp_lst[list_idx];
                _socpm_slp_lst[list_idx].next = pswap;
                return;
            } else {
                ptmp = ptmp->next;
            }
        }
        /* Add the node at the end of the list, which is having higher sleep time
         * after sleep list traverse completion
         */
        if ((_socpm_slp_lst[list_idx].slp_info.slp_time > 0) && (ptmp != &_socpm_slp_lst[list_idx])) {
            ptmp->next = &_socpm_slp_lst[list_idx];
        }
    } else if ((_socpm_slp_lst[list_idx].slp_info.slp_time > 0) && (ptmp != &_socpm_slp_lst[list_idx])) {
        ptmp->next = &_socpm_slp_lst[list_idx];
    }

    _socpm_slp_mode = _socpm_slp_lst[_socpm_slp_lst_head].slp_info.slp_mode;

    if (process_routine == 0) {
        if ((_socpm_slp_lst[_socpm_slp_lst_head].slp_info.slp_time > 0)) {
            nt_socpm_slp_tmr_set(_socpm_slp_lst[_socpm_slp_lst_head].slp_info.slp_time);
        }
    }
}

int nt_socpm_sleep_register(nt_socpm_sleep_t *slp_info, int list_idx)
{
    int retval = _SOCPM_SLP_LST_IDX_INVALID;
    int current_slp_count = _socpm_last_slp_count;
    uint64_t old_sleep_head_sleep_time = 0;
    if (_socpm_last_slp_count >= 0 && list_idx < 0) {
        list_idx = _socpm_slp_lst[_socpm_last_slp_count].slp_info.list_no + 1;
        current_slp_count = _socpm_last_slp_count + 1;
    }
    if (_socpm_slp_lst_head != _INVALID_SLP_LST_HD) {
        old_sleep_head_sleep_time = _socpm_slp_lst[_socpm_slp_lst_head].slp_info.slp_time;
    } else {
        old_sleep_head_sleep_time = 0;
    }

    if (!slp_info) {
        retval = _SOCPM_SLP_LST_IDX_INVALID;
    } else if (list_idx < 0)                                  // auto assign
    {                                                         // adding a new entry
        if (_socpm_last_slp_count < (_SOCPM_SLP_LST_SZ - 1))  // room to add an entry?
        {
            _socpm_last_slp_count++;
            current_slp_count = _socpm_last_slp_count;
            _socpm_slp_lst[_socpm_last_slp_count].slp_info = *slp_info;
            _socpm_slp_lst[_socpm_last_slp_count].slp_info.slp_time =
                _socpm_get_sleep_slop_adjusted_sleep_time(slp_info->slp_time);
            _socpm_slp_lst[_socpm_last_slp_count].next = NULL;
            _socpm_slp_lst[_socpm_last_slp_count].slp_info.list_no = _socpm_last_slp_count;
#if defined(SUPPORT_SLEEP_LIST_IMPROVEMENTS)
            _socpm_slp_lst[_socpm_last_slp_count].slp_info.start_time_us = hres_timer_curr_time_us();
            if (_socpm_slp_lst_head != _INVALID_SLP_LST_HD)
                nt_socpm_sleep_lst_update(old_sleep_head_sleep_time, false);
#endif /* SUPPORT_SLEEP_LIST_IMPROVEMENTS */

            nt_socpm_sleep_lst_reorder(_socpm_last_slp_count, old_sleep_head_sleep_time);
            if (_socpm_slp_lst[_socpm_last_slp_count].slp_info.slp_cb_fn) {
                _socpm_slp_lst[_socpm_last_slp_count].slp_info.slp_cb_fn();
            }
            retval = _socpm_last_slp_count;
        }
    } else if (list_idx < _SOCPM_SLP_LST_SZ) {  // update an existing entry
        _socpm_slp_lst[list_idx].slp_info = *slp_info;
        _socpm_slp_lst[list_idx].slp_info.slp_time = _socpm_get_sleep_slop_adjusted_sleep_time(slp_info->slp_time);
        _socpm_slp_lst[list_idx].slp_info.slp_mode = slp_info->slp_mode;

        _socpm_slp_lst[list_idx].slp_info.list_no = list_idx;
#if defined(SUPPORT_SLEEP_LIST_IMPROVEMENTS)
        _socpm_slp_lst[list_idx].slp_info.start_time_us = hres_timer_curr_time_us();
        if (_socpm_slp_lst_head != _INVALID_SLP_LST_HD)
            nt_socpm_sleep_lst_update(old_sleep_head_sleep_time, false);
#endif /* SUPPORT_SLEEP_LIST_IMPROVEMENTS */

        if (_socpm_slp_lst[list_idx].slp_info.slp_cb_fn) {
            _socpm_slp_lst[list_idx].slp_info.slp_cb_fn();
        }
        nt_socpm_sleep_lst_reorder(list_idx, old_sleep_head_sleep_time);

        retval = list_idx;
    } else {
        // invalid list number
        retval = _SOCPM_SLP_LST_IDX_INVALID;
    }

    _socpm_last_slp_count = current_slp_count;
    // sleep_lock=0;
    return retval;
}

void nt_socpm_sleep_deregister(int list_idx)
{
    _socpm_slptmr_off();
    nt_socpm_sleep_lst_delete(list_idx);
}

int nt_socpm_sleep_lst_delete(int list_idx)
{
    int result = 0;
    _socpm_slp_lst_wkup_item_t *ptmp;  //=&;first_entry

    if ((list_idx < 0) || (list_idx >= _SOCPM_SLP_LST_SZ)) {
        return _SOCPM_SLP_LST_IDX_INVALID;
    }

    _socpm_slptmr_off();

    if (_INVALID_SLP_LST_HD == _socpm_slp_lst_head) {
        // List empty. So entry had already been deleted earlier
        return _SOCPM_SLP_LST_IDX_INVALID;
    }

    ptmp = &_socpm_slp_lst[_socpm_slp_lst_head];
    //  static _socpm_slp_lst_wkup_item_t ptmpexchange;
    // configASSERT(list_idx);
    _socpm_slp_lst[list_idx].slp_info.slp_time = 0;
    // Check Whether First node needs to be deleted
    if (_socpm_slp_lst_head == list_idx) {
        _socpm_list_search();
        /** This is needed when an entry is being deleted to enable AON timer expiry
         *  for the next entry in the list. For example if AON timer was programmed
         *  during a registration for BMPS sleep, which was overwritten by a registration
         *  clk gtd sleep in the idle task. On the completion of the clk gtd slp, the
         *  AON timer needs to be programmed again for the BMPS sleep, which is done
         *  by the following slp_tmr_set call.
         */
        if ((_socpm_slp_lst_head != _INVALID_SLP_LST_HD) &&
            (_socpm_slp_lst[_socpm_slp_lst_head].slp_info.slp_time > 0)) {
            nt_socpm_slp_tmr_set(_socpm_slp_lst[_socpm_slp_lst_head].slp_info.slp_time);
        }
    } else {
        while (ptmp->next != NULL) {
            if (&_socpm_slp_lst[list_idx] == ptmp->next) {
                ptmp->next = _socpm_slp_lst[list_idx].next;
                _socpm_slp_lst[list_idx].next = NULL;
                _socpm_slp_lst[list_idx].slp_info.slp_time = 0;
                break;
            }
        }
    }
    return result;
}

uint64_t freertosdefaultminimum(uint32_t wkup_delay_us)
{
    PM_STRUCT *pPmStruct;

    SOCPM_UNUSED(wkup_delay_us);
    if (gdevp) {
        pPmStruct = (PM_STRUCT *)gdevp->pPmStruct;
        set_sleep_exit_reason();

        if (pPmStruct->bConnected) {
            {
                // nt_hal_rri_soft_reset_rri_engine();
                // nt_hal_rri_restore_first();
            }

            // PM_SET_RRI_STATE(pPmStruct, PM_RRI_RX_READY);

/*SW MTU time restoration must to be conducted when RRI first list restored
  because MTU TSF will be retored to 0 after RRI first list restored.
*/
#ifdef NT_SOCPM_SW_MTUSR
            // nt_socpm_mtusr_restore_mtu_time();

#endif  // NT_SOCPM_SW_MTUSR

            // PM_SET_WLAN_STATE_ON(pPmStruct);

            {
                // nt_hal_rri_restore_second();
                // PM_SET_RRI_STATE(pPmStruct, PM_RRI_TXRX_READY);
            }

            // rri_force_wakeup = 1;
        }
    }

    return 0;
}

static void _socpm_wkup_dflt_cb(soc_wkup_reason wkup_reason)
{
    (void)wkup_reason;

#ifdef SUPPORT_SWTMR_TO_WKUP_FROM_BMPS
    if ((g_socpm_struct.in_warm_boot == TRUE || _socpm_mcu_sleep_wake == 1) && (PM_STRUCT *)gdevp->pPmStruct != NULL &&
        ((PM_STRUCT *)(gdevp->pPmStruct))->pm_type == PM_MODE_BMPS) {
        nt_socpm_log_wkup_reason_after_sleep();
    }
#endif /* SUPPORT_SWTMR_TO_WKUP_FROM_BMPS */
    // if (SOC_WKUP_ABORT == wkup_reason)
    {
        nt_socpm_sleep_lst_delete(_socpm_slp_list_idx_rtos);
    }
}

static void _socpm_slp_dflt_cb(void) {}

#ifdef _SOCPM_INC_TST_SLEEP
static void _tst_sleep_enter(void)
{
    //__asm volatile("mov %0, r13" : "=r"(_r13_stackpointer));
    __asm volatile("dmb");
    __asm volatile("isb");
    __asm volatile("wfi");
    // nops added to avoid issues due to cortex prefetch
    __asm volatile("nop");
    __asm volatile("nop");
    __asm volatile("nop");

    //__asm volatile("mov %0, pc" : "=r"(_pc_restartaftersleep_wfi));
    // printf("Out of WFI...\n");
    __asm volatile("dmb");
    __asm volatile("isb");
    // printf("Out of WFI...\n");
    __asm volatile("dmb");
    __asm volatile("isb");
    // printf("Out of WFI...\n");
}
#endif  //_SOCPM_INC_TST_SLEEP

#if defined(IO_DEBUG)
/*****************************************************************
 * @brief Funtion to reset the io debug count
 * @param none
 * @return none
 ****************************************************************/

void socpm_reset_io_debug_count()
{
    g_socpm_struct.io_dbg_count = 0;
}
#endif /*IO_DEBUG*/

static void socpm_clear_bbpll_toggle()
{
    uint32_t value;
    // clear possible PLL lock toggle intr
    value = NT_REG_RD(QWLAN_PMU_BBPLL_STATUS_REG);
    if (value | QWLAN_PMU_BBPLL_STATUS_BBPLL_LOCK_TOGGLE_INTR_MASK) {
        NT_REG_WR(QWLAN_PMU_BBPLL_STATUS_REG, value | QWLAN_PMU_BBPLL_STATUS_CFG_BBPLL_LOCK_TOGGLE_INTR_CLR_MASK);
    }
}

#if defined(PLATFORM_FERMION)

/*****************************************************************
 * @brief Funtion that has recipes to reduce W2S HW timing
 * @param none
 * @return none
 ****************************************************************/
static void _socpm_slp_timing_tuning(void)
{
#if 0
	//based on VIFERMION-204 VIFERMION-201, if any issue please reopen jira 201
    NT_REG_WR(QWLAN_PMU_CFG_XO_SETTLE_TIME_REG, 0x21);
    // MX SUPPLY settle time
    NT_REG_WR(QWLAN_PMU_CFG_AON_SM_DELAYS_REG, 0x38080d20);
#else
    uint32_t value;
    // based on VIFERMION-204 VIFERMION-201, if any issue please reopen jira 201
    // NT_REG_WR(QWLAN_PMU_CFG_XO_SETTLE_TIME_REG,(xo_trim_time<<QWLAN_PMU_CFG_XO_SETTLE_TIME_CFG_XO_TRIM_TIMEOUT_OFFSET)|xo_settle_time);
    NT_REG_WR(QWLAN_PMU_CFG_XO_SETTLE_TIME_REG, xo_settle_time);
    // MX SUPPLY settle time is 200ms from Aria
#if 1
    value = (mx_settle_time << QWLAN_PMU_CFG_AON_SM_DELAYS_CFG_MX_SUPPLY_SETTLE_TIME_OFFSET) |
            (p8v_smps_settle_time << QWLAN_PMU_CFG_AON_SM_DELAYS_CFG_P8V_SMPS_EN_2_PWR_GOOD_TIMEOUT_OFFSET) |
            (pmic_slp_exit_time << QWLAN_PMU_CFG_AON_SM_DELAYS_CFG_PMIC_SLEEP_MODE_EXIT_TIME_OFFSET) |
            (pmic_slp_entry_time << QWLAN_PMU_CFG_AON_SM_DELAYS_CFG_PMIC_SLEEP_MODE_ENTRY_TIME_OFFSET);
    NT_REG_WR(QWLAN_PMU_CFG_AON_SM_DELAYS_REG, value);
#else
    value = NT_REG_RD(QWLAN_PMU_CFG_AON_SM_DELAYS_REG);
    value &= ~QWLAN_PMU_CFG_AON_SM_DELAYS_CFG_MX_SUPPLY_SETTLE_TIME_MASK;
    value |= ((mx_settle_time << QWLAN_PMU_CFG_AON_SM_DELAYS_CFG_MX_SUPPLY_SETTLE_TIME_OFFSET) &
              QWLAN_PMU_CFG_AON_SM_DELAYS_CFG_MX_SUPPLY_SETTLE_TIME_MASK);
    value &= ~QWLAN_PMU_CFG_AON_SM_DELAYS_CFG_P8V_SMPS_EN_2_PWR_GOOD_TIMEOUT_MASK;
    value |= (p8v_smps_settle_time << QWLAN_PMU_CFG_AON_SM_DELAYS_CFG_P8V_SMPS_EN_2_PWR_GOOD_TIMEOUT_OFFSET) &
             QWLAN_PMU_CFG_AON_SM_DELAYS_CFG_P8V_SMPS_EN_2_PWR_GOOD_TIMEOUT_MASK;
    value &= ~QWLAN_PMU_CFG_AON_SM_DELAYS_CFG_PMIC_SLEEP_MODE_EXIT_TIME_MASK;
    value |= (pmic_slp_exit_time << QWLAN_PMU_CFG_AON_SM_DELAYS_CFG_PMIC_SLEEP_MODE_EXIT_TIME_OFFSET) &
             QWLAN_PMU_CFG_AON_SM_DELAYS_CFG_PMIC_SLEEP_MODE_EXIT_TIME_MASK;
    value &= ~QWLAN_PMU_CFG_AON_SM_DELAYS_CFG_PMIC_SLEEP_MODE_ENTRY_TIME_MASK;
    value |= (pmic_slp_entry_time << QWLAN_PMU_CFG_AON_SM_DELAYS_CFG_PMIC_SLEEP_MODE_ENTRY_TIME_OFFSET) &
             QWLAN_PMU_CFG_AON_SM_DELAYS_CFG_PMIC_SLEEP_MODE_ENTRY_TIME_MASK;
    NT_REG_WR(QWLAN_PMU_CFG_AON_SM_DELAYS_REG, value);
#endif
#endif
}

/*****************************************************************
 * @brief Funtion to put the SOC in mcu sleep mode
 *
 * @param none
 * @return none
 ****************************************************************/

static void __attribute__((noinline)) slp_ctrl_enable_sleep(sleep_mode mode)
{
    NT_REG_RD(QWLAN_PMU_AON_TOP_CFG_REG);

    if (mode == mcu_sleep || mode == Lightsleep)
        NT_REG_WR(QWLAN_PMU_SLP_CNTL_REG, QWLAN_PMU_SLP_CNTL_EN_SLEEP_MASK);
    else if (mode == Standby)
        NT_REG_WR(QWLAN_PMU_SLP_CNTL_REG, QWLAN_PMU_SLP_CNTL_EN_DEEPSLEEP_MASK);

    NT_REG_WR(_SOCPM_CPU_SYS_CTL_REG, 4);
}

static void slp_gpio_pupd_disable()
{
    uint32_t value;

    gpio_config.ls_sync = NT_REG_RD(QWLAN_GPIO_GPIO_LS_SYNC_REG);
    gpio_config.dr = NT_REG_RD(QWLAN_GPIO_GPIO_SWPORTA_DR_REG);
    gpio_config.ddr = NT_REG_RD(QWLAN_GPIO_GPIO_SWPORTA_DDR_REG);
    gpio_config.int_level = NT_REG_RD(QWLAN_GPIO_GPIO_INTTYPE_LEVEL_REG);
    gpio_config.int_polar = NT_REG_RD(QWLAN_GPIO_GPIO_INR_POLARITY_REG);
    gpio_config.int_en = NT_REG_RD(QWLAN_GPIO_GPIO_INTEN_REG);

    value = NT_REG_RD(QWLAN_PMU_CFG_IOPAD_DS_REG);
    gpio_config.ds = value;

    /*Disable pull up/pull down for JTAG/UART TX/WSI data/F2A IOs before going to sleep*/
    value = NT_REG_RD(QWLAN_PMU_CFG_IOPAD_PU_REG);
    gpio_config.pu = value;
    value &= ~(QWLAN_PMU_CFG_IOPAD_PU_AON_IOPAD_TDI_PU_MASK);      // JTAG
    value &= ~(QWLAN_PMU_CFG_IOPAD_PU_AON_IOPAD_TMS_PU_MASK);      // JTAG
    value &= ~(QWLAN_PMU_CFG_IOPAD_PU_AON_IOPAD_GPIO_14_PU_MASK);  // WSI data
    NT_REG_WR(QWLAN_PMU_CFG_IOPAD_PU_REG, value);
    value = NT_REG_RD(QWLAN_PMU_CFG_IOPAD_PD_REG);
    gpio_config.pd = value;
    value &= ~(QWLAN_PMU_CFG_IOPAD_PD_AON_IOPAD_GPIO_11_PD_MASK);  // UART TX
    value &= ~(QWLAN_PMU_CFG_IOPAD_PD_AON_IOPAD_GPIO_8_PD_MASK);   // F2A
    /* Improve low power by disabling UART */
#if CONFIG_BOARD_QCC730_UART_GPIO_OPTION == 3
    /* UART: GPIO1 GPIO3*/
    value &= ~(QWLAN_PMU_CFG_IOPAD_PD_AON_IOPAD_GPIO_1_PD_MASK);  // GPIO1 bit1
    value &= ~(QWLAN_PMU_CFG_IOPAD_PD_AON_IOPAD_GPIO_3_PD_MASK);  // GPIO3 bit3
#endif

#if CONFIG_BOARD_QCC730_UART_GPIO_OPTION == 1
    /* UART: GPIO13 GPIO14*/
    value &= ~(QWLAN_PMU_CFG_IOPAD_PD_AON_IOPAD_GPIO_13_PD_MASK);  // GPIO13
    value &= ~(QWLAN_PMU_CFG_IOPAD_PD_AON_IOPAD_GPIO_14_PD_MASK);  // GPIO14
#endif

    NT_REG_WR(QWLAN_PMU_CFG_IOPAD_PD_REG, value);
    gpio_config.saved = 1;
}

static void __attribute__((used)) socpm_enter_mcusleep()
{
    volatile uint32_t value;
    PM_STRUCT *pPmStruct = (PM_STRUCT *)gdevp->pPmStruct;
    pPmStruct->slp_clk_sel = _socpm_slp_clk_src;
    volatile uint32_t wifi_ss_state;

    if (FALSE == g_socpm_struct.in_warm_boot) {
        value = NT_REG_RD(NT_CM4_NVIC_ISER0_REG);
        nt_socpm_m4_regs[11] = value;

        value = NT_REG_RD(NT_CM4_NVIC_ISER1_REG);
        nt_socpm_m4_regs[12] = value;

        value = NT_REG_RD(NT_CM4_NVIC_ISER2_REG);
        nt_socpm_m4_regs[13] = value;

        value = NT_REG_RD(NT_CM4_NVIC_ISER3_REG);
        nt_socpm_m4_regs[14] = value;
    }

#ifdef GPIO_RETENTION_IN_SLP
    NT_REG_WR(QWLAN_PMU_CFG_IO_RET_CNTL_REG, QWLAN_PMU_CFG_IO_RET_CNTL_AON_IORET_CNTL_MASK);
#endif

    NT_REG_WR(QWLAN_PMU_CFG_ACAL_VBAT_MON_EN_REG, 0);
    value = NT_REG_RD(QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_REG);
    value &= ~QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_PD_XIP_CNTL_BIT_MASK;
    NT_REG_WR(QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_REG, value);

    value = QWLAN_PMU_DIG_TOP_CFG_DEFAULT;
    value &= ~QWLAN_PMU_DIG_TOP_CFG_PHY_RXTOP_REG_RET_EN_SLEEP_MASK;
    value &= ~QWLAN_PMU_DIG_TOP_CFG_PHY_RXA_REG_RET_EN_SLEEP_MASK;
    value &= ~QWLAN_PMU_DIG_TOP_CFG_PHY_TX_REG_RET_EN_SLEEP_MASK;
    value &= ~QWLAN_PMU_DIG_TOP_CFG_FORCE_WMAC_CORE_ON_MASK;
    value |= (QWLAN_PMU_DIG_TOP_CFG_PHY_RXTOP_REG_RET_EN_NON_SLEEP_MASK |
              QWLAN_PMU_DIG_TOP_CFG_PHY_RXA_REG_RET_EN_NON_SLEEP_MASK |
              QWLAN_PMU_DIG_TOP_CFG_PHY_TX_REG_RET_EN_NON_SLEEP_MASK | QWLAN_PMU_DIG_TOP_CFG_FORCE_BANK_B_CORE_ON_MASK |
              QWLAN_PMU_DIG_TOP_CFG_FORCE_BANK_C_CORE_ON_MASK | QWLAN_PMU_DIG_TOP_CFG_FORCE_BANK_D_CORE_ON_MASK |
              QWLAN_PMU_DIG_TOP_CFG_FORCE_BANK_E_CORE_ON_MASK);
    value &= ~QWLAN_PMU_DIG_TOP_CFG_FORCE_PHY_RX_CORE_ON_MASK;
    value &= ~QWLAN_PMU_DIG_TOP_CFG_WMAC_REG_RET_EN_MASK;
#ifdef PMU_REG_RETENTION_STATUS_FOR_SOC_SLP
    value |= nt_pm_set_and_get_pmu_dtop_reg_retention_status(mcu_sleep);
#endif
    NT_REG_WR(QWLAN_PMU_DIG_TOP_CFG_REG, value);

    value = QWLAN_PMU_AON_TOP_CFG_DEFAULT;
    value |= QWLAN_PMU_AON_TOP_CFG_AON_WDOG_SLP_ROOT_CLK_ENABLE_MASK;
    value |= QWLAN_PMU_AON_TOP_CFG_CFG_WAKEUP_MCU_SS_ON_DTIM_INTR_MASK;
    value |= QWLAN_PMU_AON_TOP_CFG_CFG_WAKEUP_WIFI_SS_ON_DTIM_INTR_MASK;
    value &= ~QWLAN_PMU_AON_TOP_CFG_CFG_ENABLE_XO_CLK_DETECT_MASK;
    value |= QWLAN_PMU_AON_TOP_CFG_CFG_SLP_CLK_SWITCHING_EN_MASK;
    value &= ~QWLAN_PMU_AON_TOP_CFG_CFG_XO_SLP_CLK_SEL_EN_MASK;
    value &= ~QWLAN_PMU_AON_TOP_CFG_CFG_INDEFINITE_DEEPSLEEP_EN_MASK;
    value &= ~QWLAN_PMU_AON_TOP_CFG_CFG_LIGHTSLEEP_EN_MASK;
    value &= ~QWLAN_PMU_AON_TOP_CFG_CFG_AON_PMIC_AONLDO_INPUT_SEL_MASK;
    value |= ((1 << QWLAN_PMU_AON_TOP_CFG_CFG_AON_PMIC_AONLDO_INPUT_SEL_OFFSET) &
              QWLAN_PMU_AON_TOP_CFG_CFG_AON_PMIC_AONLDO_INPUT_SEL_MASK);
    NT_REG_WR(QWLAN_PMU_AON_TOP_CFG_REG, value);

    value = NT_REG_RD(QWLAN_PMU_CFG_HW_DTIM_MODE_REG);
    value &= ~QWLAN_PMU_CFG_HW_DTIM_MODE_WMAC_HW_DTIM_MODE_MASK;
    value &= ~QWLAN_PMU_CFG_HW_DTIM_MODE_WMAC_HW_DTIM_MODE_WITH_CPU_ON_MASK;
    NT_REG_WR(QWLAN_PMU_CFG_HW_DTIM_MODE_REG, value);

    wifi_fw_cpr_disable();

    value = NT_REG_RD(QWLAN_PMU_CFG_PMIC_SLEEP_MODE_CNTL_REG);
    value &= ~QWLAN_PMU_CFG_PMIC_SLEEP_MODE_CNTL_CFG_DISABLE_PMIC_SLEEP_MODE_MASK;
    NT_REG_WR(QWLAN_PMU_CFG_PMIC_SLEEP_MODE_CNTL_REG, value);

#ifdef SLEEP_CLK_CAL_IN_SLEEP_MODE
    if (false == g_socpm_struct.slp_clk_cal_params.sleep_mode_cal_enabled) {
#endif
        NT_REG_WR(QWLAN_PMU_CFG_SLP_CAL_CAL_EN_REG, 0);
#ifdef SLEEP_CLK_CAL_IN_SLEEP_MODE
    }
#endif

    _socpm_slp_timing_tuning();

    value = NT_REG_RD(QWLAN_PMU_RFA_DTOP_GDSCR_REG);
    NT_REG_WR(QWLAN_PMU_RFA_DTOP_GDSCR_REG, value | QWLAN_PMU_RFA_DTOP_GDSCR_RETAIN_FF_ENABLE_MASK);

    value = NT_REG_RD(QWLAN_PMU_XO_GDSCR_REG);
    NT_REG_WR(QWLAN_PMU_XO_GDSCR_REG, value | QWLAN_PMU_XO_GDSCR_RETAIN_FF_ENABLE_MASK);

    value = NT_REG_RD(QWLAN_PMU_PMIC_DTOP_GDSCR_REG);
    NT_REG_WR(QWLAN_PMU_PMIC_DTOP_GDSCR_REG, value | QWLAN_PMU_PMIC_DTOP_GDSCR_RETAIN_FF_ENABLE_MASK);

    // WIFI SLEEP RRT
    value = QWLAN_PMU_CFG_AON_CNTL_WIFI_SLEEP_STATE_RESOURCE_REQ_DEFAULT;
    value &= ~QWLAN_PMU_CFG_AON_CNTL_WIFI_SLEEP_STATE_RESOURCE_REQ_PD_XO_DTOP_CNTL_BIT_MASK;
    value &= ~QWLAN_PMU_CFG_AON_CNTL_WIFI_SLEEP_STATE_RESOURCE_REQ_PD_PMIC_DTOP_CNTL_BIT_MASK;
    value &= ~QWLAN_PMU_CFG_AON_CNTL_WIFI_SLEEP_STATE_RESOURCE_REQ_PD_RFA_DTOP_CNTL_BIT_MASK;
    value = NT_REG_WR(QWLAN_PMU_CFG_AON_CNTL_WIFI_SLEEP_STATE_RESOURCE_REQ_REG, value);

    value = QWLAN_PMU_CFG_AON_CNTL_MCU_SLEEP_STATE_RESOURCE_REQ_DEFAULT;
    NT_REG_WR(QWLAN_PMU_CFG_AON_CNTL_MCU_SLEEP_STATE_RESOURCE_REQ_REG, value);

    wifi_ss_state =
        HWIO_INXF(SEQ_WCSS_PMU_OFFSET, NEUTRINO_PMU_PRONTO_LP_FRODO_PMU_CFG_WIFI_SS_STATE, WIFI_SS_CURR_STATE);

    if (wifi_ss_state != NT_PMU_CFG_WIFI_SLEEP_OFFSET) {
        HWIO_OUTXF(SEQ_WCSS_PMU_OFFSET, NEUTRINO_PMU_PRONTO_LP_FRODO_PMU_CFG_WIFI_SS_STATE, CFG_WIFI_SS_NEXT_STATE,
                   NT_PMU_CFG_WIFI_SLEEP_OFFSET);
    }

    PM_SET_RRI_STATE(pPmStruct, PM_RRI_MAC_DOWN_MCUSLP);

    slp_gpio_pupd_disable();

    slp_ctrl_enable_sleep(mcu_sleep);
}

static void _socpm_slpcfg_mcuslp(void)
{
    volatile uint32_t value;
    PM_STRUCT *pPmStruct = (PM_STRUCT *)gdevp->pPmStruct;
    pPmStruct->slp_clk_sel = _socpm_slp_clk_src;
    volatile uint32_t wifi_ss_state;

    // socpm_enter_mcusleep();
    // return;

#ifdef _SOCPM_INC_TST_MCUSLP_SBY
    socpm_slpcfg_sby();
#endif

    /** Do not attempt to back up the ISER registers while in warm boot state
     * since they would not have been restored in the minimal code.
     * This scenario can occur when MCU sleep recipe is executed while in warm
     * boot state, such as for SWDTIM sleep.
     */
    if (FALSE == g_socpm_struct.in_warm_boot) {
        value = NT_REG_RD(NT_CM4_NVIC_ISER0_REG);
        nt_socpm_m4_regs[11] = value;

        value = NT_REG_RD(NT_CM4_NVIC_ISER1_REG);
        nt_socpm_m4_regs[12] = value;

        value = NT_REG_RD(NT_CM4_NVIC_ISER2_REG);
        nt_socpm_m4_regs[13] = value;

        value = NT_REG_RD(NT_CM4_NVIC_ISER3_REG);
        nt_socpm_m4_regs[14] = value;
    }

#ifdef GPIO_RETENTION_IN_SLP
    /* Enable GPIO retention */
    NT_REG_WR(QWLAN_PMU_CFG_IO_RET_CNTL_REG, QWLAN_PMU_CFG_IO_RET_CNTL_AON_IORET_CNTL_MASK);

    /* Read GPIO Retention status */
#ifdef NT_SOCPM_DISABLED
    value = NT_REG_RD(QWLAN_PMU_GPIO_DIR_STATUS_REG);
#endif /* NT_SOCPM_DISABLED */
#endif /* GPIO_RETENTION_IN_SLP */

    uint8_t wr_all_f = 1;  // skip/opt some writes out

    if (wr_all_f) {
        NT_REG_WR(QWLAN_PMU_CFG_ACAL_VBAT_MON_EN_REG, 0);
        value = NT_REG_RD(QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_REG);

        value &= ~QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_PD_XIP_CNTL_BIT_MASK;

        NT_REG_WR(QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_REG, value);
    }

    // clear possible PLL lock toggle intr during stress
    socpm_clear_bbpll_toggle();

    // phyrx_reg_ret_en=0,phyrxa_reg_ret_en=0,phytx_reg_ret_en=0,wmac_mem_ret_en=0
    value = QWLAN_PMU_DIG_TOP_CFG_DEFAULT;
    value &= ~QWLAN_PMU_DIG_TOP_CFG_PHY_RXTOP_REG_RET_EN_SLEEP_MASK;
    value &= ~QWLAN_PMU_DIG_TOP_CFG_PHY_RXA_REG_RET_EN_SLEEP_MASK;
    value &= ~QWLAN_PMU_DIG_TOP_CFG_PHY_TX_REG_RET_EN_SLEEP_MASK;
    value &= ~QWLAN_PMU_DIG_TOP_CFG_FORCE_WMAC_CORE_ON_MASK;
    value &= ~QWLAN_PMU_DIG_TOP_CFG_FORCE_PHY_RX_CORE_ON_MASK;
    value |= QWLAN_PMU_DIG_TOP_CFG_WMAC_REG_RET_EN_MASK;

    value |= (QWLAN_PMU_DIG_TOP_CFG_PHY_RXTOP_REG_RET_EN_NON_SLEEP_MASK |
              QWLAN_PMU_DIG_TOP_CFG_PHY_RXA_REG_RET_EN_NON_SLEEP_MASK |
              QWLAN_PMU_DIG_TOP_CFG_PHY_TX_REG_RET_EN_NON_SLEEP_MASK | QWLAN_PMU_DIG_TOP_CFG_FORCE_BANK_B_CORE_ON_MASK |
              QWLAN_PMU_DIG_TOP_CFG_FORCE_BANK_C_CORE_ON_MASK | QWLAN_PMU_DIG_TOP_CFG_FORCE_BANK_D_CORE_ON_MASK |
              QWLAN_PMU_DIG_TOP_CFG_FORCE_BANK_E_CORE_ON_MASK);
    value |= QWLAN_PMU_DIG_TOP_CFG_CFG_MEM_MX_DYNAMIC_SWITCHING_EN_MASK;
#ifdef PMU_REG_RETENTION_STATUS_FOR_SOC_SLP
    /* we want to retain ANI registers for TWT with MCU sleep and these registers fall under RXTOP retention. Hence,
     * retaing RXDTOP REG*/
    value |= nt_pm_set_and_get_pmu_dtop_reg_retention_status(mcu_sleep);
#endif /* PMU_REG_RETENTION_STATUS_FOR_SOC_SLP */

    NT_REG_WR(QWLAN_PMU_DIG_TOP_CFG_REG, value);

    value = QWLAN_PMU_AON_TOP_CFG_DEFAULT;

    // use_xo_clk_det=1
    value |= (QWLAN_PMU_AON_TOP_CFG_CFG_WAKEUP_MCU_SS_ON_DTIM_INTR_MASK |
              QWLAN_PMU_AON_TOP_CFG_CFG_WAKEUP_WIFI_SS_ON_DTIM_INTR_MASK |
              QWLAN_PMU_AON_TOP_CFG_CFG_SLP_CLK_SWITCHING_EN_MASK);

    value &= ~QWLAN_PMU_AON_TOP_CFG_CFG_ENABLE_XO_CLK_DETECT_MASK;
    value &= ~QWLAN_PMU_AON_TOP_CFG_CFG_XO_SLP_CLK_SEL_EN_MASK;
    value &= ~QWLAN_PMU_AON_TOP_CFG_CFG_INDEFINITE_DEEPSLEEP_EN_MASK;
    // lightsleep_en=0
    value &= ~QWLAN_PMU_AON_TOP_CFG_CFG_LIGHTSLEEP_EN_MASK;
    value &= ~QWLAN_PMU_AON_TOP_CFG_CFG_AON_PMIC_AONLDO_INPUT_SEL_MASK;

    // aonldo_input_sel = 2
    value |= ((1 << QWLAN_PMU_AON_TOP_CFG_CFG_AON_PMIC_AONLDO_INPUT_SEL_OFFSET) &
              QWLAN_PMU_AON_TOP_CFG_CFG_AON_PMIC_AONLDO_INPUT_SEL_MASK);
    value |= QWLAN_PMU_AON_TOP_CFG_AON_WDOG_SLP_ROOT_CLK_ENABLE_MASK;
    NT_REG_WR(QWLAN_PMU_AON_TOP_CFG_REG, value);

    value = NT_REG_RD(QWLAN_PMU_SON_GDSCR_REG);
    value |= QWLAN_PMU_SON_GDSCR_RETAIN_FF_ENABLE_MASK;
    value &= ~(QWLAN_PMU_SON_GDSCR_EN_FEW_WAIT_MASK | QWLAN_PMU_SON_GDSCR_EN_REST_WAIT_MASK);
    // VIFERMION-215 for 1.0
    // VIFERMION-379 for 2.0
    value |= (son_en_wait_mcu << QWLAN_PMU_SON_GDSCR_EN_FEW_WAIT_OFFSET) & QWLAN_PMU_SON_GDSCR_EN_FEW_WAIT_MASK;
    value |= (son_en_wait_mcu << QWLAN_PMU_SON_GDSCR_EN_REST_WAIT_OFFSET) & QWLAN_PMU_SON_GDSCR_EN_REST_WAIT_MASK;
    NT_REG_WR(QWLAN_PMU_SON_GDSCR_REG, value);

    value = NT_REG_RD(QWLAN_PMU_CFG_HW_DTIM_MODE_REG);
    value &= ~QWLAN_PMU_CFG_HW_DTIM_MODE_WMAC_HW_DTIM_MODE_MASK;
    value &= ~QWLAN_PMU_CFG_HW_DTIM_MODE_WMAC_HW_DTIM_MODE_WITH_CPU_ON_MASK;

    // use_hwdtim_with_cpu_on=0,check_tbtt_count=0,check_dc_count=0
    NT_REG_WR(QWLAN_PMU_CFG_HW_DTIM_MODE_REG, value);

    // Disable CPR and set CX LDO sleep voltage to 0.6V (experimental value)
    wifi_fw_cpr_disable();

    presleep_update_ulpsmps2_oneshot();

    // SLEEP MODE enable disable_sleep_mode_en=0
    value = NT_REG_RD(QWLAN_PMU_CFG_PMIC_SLEEP_MODE_CNTL_REG);
    value &= ~QWLAN_PMU_CFG_PMIC_SLEEP_MODE_CNTL_CFG_DISABLE_PMIC_SLEEP_MODE_MASK;
    NT_REG_WR(QWLAN_PMU_CFG_PMIC_SLEEP_MODE_CNTL_REG, value);

    // SLP CLK CAL slp_clk_cal_en=0
#ifdef SLEEP_CLK_CAL_IN_SLEEP_MODE
    if (false == g_socpm_struct.slp_clk_cal_params.sleep_mode_cal_enabled) {
#endif /* SLEEP_CLK_CAL_IN_SLEEP_MODE */
        NT_REG_WR(QWLAN_PMU_CFG_SLP_CAL_CAL_EN_REG, 0);
#ifdef SLEEP_CLK_CAL_IN_SLEEP_MODE
    }
#endif /* SLEEP_CLK_CAL_IN_SLEEP_MODE */
    _socpm_slp_timing_tuning();

    value = NT_REG_RD(QWLAN_PMU_RFA_DTOP_GDSCR_REG);
    NT_REG_WR(QWLAN_PMU_RFA_DTOP_GDSCR_REG, value | QWLAN_PMU_RFA_DTOP_GDSCR_RETAIN_FF_ENABLE_MASK);

    value = NT_REG_RD(QWLAN_PMU_XO_GDSCR_REG);
    NT_REG_WR(QWLAN_PMU_XO_GDSCR_REG, value | QWLAN_PMU_XO_GDSCR_RETAIN_FF_ENABLE_MASK);

    value = NT_REG_RD(QWLAN_PMU_PMIC_DTOP_GDSCR_REG);
    NT_REG_WR(QWLAN_PMU_PMIC_DTOP_GDSCR_REG, value | QWLAN_PMU_PMIC_DTOP_GDSCR_RETAIN_FF_ENABLE_MASK);

    value = QWLAN_PMU_CFG_AON_CNTL_MCU_SLEEP_STATE_RESOURCE_REQ_DEFAULT;
#ifdef FERMION_1_0_POWER_WAR
    /** Address VIFERMION-199: Hardfault seen on PMU access for LDOCX_SS_EN before deep sleep
     * In Fermion 1.0, when PMIC DTOP is powered off during sleep, some registers needed for
     * PMIC state machine are not retained. This results in CX LDO voltage dropping below
     * operating voltage level when soft start is enabled after exit from sleep.
     * To address this, PMIC DTOP is kept on during sleep.
     */
    value |= QWLAN_PMU_CFG_AON_CNTL_MCU_SLEEP_STATE_RESOURCE_REQ_PD_PMIC_DTOP_CNTL_BIT_MASK;
#endif /* FERMION_1_0_POWER_WAR */
    NT_REG_WR(QWLAN_PMU_CFG_AON_CNTL_MCU_SLEEP_STATE_RESOURCE_REQ_REG, value);

    // NT_REG_WR(QWLAN_PMU_CFG_MCU_SLEEP_STATE_RESOURCE_REQ_REG, QWLAN_PMU_CFG_MCU_SLEEP_STATE_RESOURCE_REQ_DEFAULT);

#ifdef FEATURE_FERMION_SLP_DBG
    value = NT_REG_RD(QWLAN_PMU_CFG_AON_CNTL_MCU_SLEEP_STATE_RESOURCE_REQ_REG);
    if (g_socpm_struct.socpm_mcu_sleep_dbg_mode) {
        /* Enable SON Domain */
        NT_REG_WR(QWLAN_PMU_CFG_AON_CNTL_MCU_SLEEP_STATE_RESOURCE_REQ_REG,
                  value | QWLAN_PMU_CFG_AON_CNTL_MCU_SLEEP_STATE_RESOURCE_REQ_PD_SON_CNTL_BIT_MASK);

        /* Enable CMNSS */
        value = NT_REG_RD(QWLAN_PMU_CFG_MCU_SLEEP_STATE_RESOURCE_REQ_REG);
        value |= (QWLAN_PMU_CFG_MCU_SLEEP_STATE_RESOURCE_REQ_PD_CMNSS_CNTL_BIT_MASK);
        NT_REG_WR(QWLAN_PMU_CFG_MCU_SLEEP_STATE_RESOURCE_REQ_REG, value);
    } else {
        /* Regular MCU Sleep path */
        if (CHECK_BIT_SET(value, QWLAN_PMU_CFG_AON_CNTL_MCU_SLEEP_STATE_RESOURCE_REQ_PD_SON_CNTL_BIT_OFFSET)) {
            value &= ~QWLAN_PMU_CFG_AON_CNTL_MCU_SLEEP_STATE_RESOURCE_REQ_PD_SON_CNTL_BIT_MASK;
            NT_REG_WR(QWLAN_PMU_CFG_AON_CNTL_MCU_SLEEP_STATE_RESOURCE_REQ_REG, value);
        }
        value = NT_REG_RD(QWLAN_PMU_CFG_MCU_SLEEP_STATE_RESOURCE_REQ_REG);
        /* Disable CMNSS if set */
        if (CHECK_BIT_SET(value, QWLAN_PMU_CFG_MCU_SLEEP_STATE_RESOURCE_REQ_PD_CMNSS_CNTL_BIT_MASK)) {
            value &= ~QWLAN_PMU_CFG_MCU_SLEEP_STATE_RESOURCE_REQ_PD_CMNSS_CNTL_BIT_MASK;
            NT_REG_WR(QWLAN_PMU_CFG_MCU_SLEEP_STATE_RESOURCE_REQ_REG, value);
        }
    }
#endif /* FEATURE_FERMION_SLP_DBG */

    /* if a clock gated sleep precedes the protocol sleeps, wifi state moves to config on AON timer expiry
       Move wifi to sleep state back before hitting the wfi. if wifi is not in sleep state, the chip remains
       in wfi until AON timer interrupt or some other interrupt happens*/

    wifi_ss_state =
        HWIO_INXF(SEQ_WCSS_PMU_OFFSET, NEUTRINO_PMU_PRONTO_LP_FRODO_PMU_CFG_WIFI_SS_STATE, WIFI_SS_CURR_STATE);

    if (wifi_ss_state != NT_PMU_CFG_WIFI_SLEEP_OFFSET) {
        uint32_t value;
        value = HAL_REG_RD(QWLAN_RXP_CONFIG_REG);
        HAL_REG_WR(QWLAN_RXP_CONFIG_REG,
                   (value & ~QWLAN_RXP_CONFIG_CFG_RXP_EN_MASK));  // Disable RX

        /*clear any pending AON timer interrupts. If AON interrupt is pending, wifi doesnt move to sleep state*/
        _socpm_slptmr_off();

        PM_SET_WLAN_STATE_OFF(pPmStruct);
        NT_REG_WR(QWLAN_PMU_CFG_WIFI_SS_STATE_REG, NT_PMU_CFG_WIFI_SLEEP_OFFSET);

        wifi_ss_state =
            HWIO_INXF(SEQ_WCSS_PMU_OFFSET, NEUTRINO_PMU_PRONTO_LP_FRODO_PMU_CFG_WIFI_SS_STATE, WIFI_SS_CURR_STATE);

        NT_LOG_PRINT(SOCPM, ERR, " wifi_ss_state %x", (uint32_t)wifi_ss_state);
    }

    PM_SET_RRI_STATE(pPmStruct, PM_RRI_MAC_DOWN_MCUSLP);

#if defined(IO_DEBUG)
    /* Debug code which will disable one IO PU/PD register at a time for every MCU sleep.
     * this can be observed over multiple sleep to narrow down which IO is causing IO current*/
    if (g_socpm_struct.io_dbg_count < (MAX_IO_PINS * 2)) {
        if (g_socpm_struct.io_dbg_count < MAX_IO_PINS) {
            value = NT_REG_RD(QWLAN_PMU_CFG_IOPAD_PU_REG);
            value &= ~(1 << g_socpm_struct.io_dbg_count);
            NT_REG_WR(QWLAN_PMU_CFG_IOPAD_PU_REG, value);
            g_socpm_struct.io_dbg_count++;
            NT_LOG_SOCPM_INFO("IO debug PU", value, g_socpm_struct.io_dbg_count, 0);
        } else {
            value = NT_REG_RD(QWLAN_PMU_CFG_IOPAD_PD_REG);
            value &= ~(1 << (g_socpm_struct.io_dbg_count % MAX_IO_PINS));
            NT_REG_WR(QWLAN_PMU_CFG_IOPAD_PD_REG, value);
            g_socpm_struct.io_dbg_count++;
            NT_LOG_SOCPM_INFO("IO debug PD", value, g_socpm_struct.io_dbg_count, 0);
        }
    }
#endif /*IO_DEBUG*/

    slp_gpio_pupd_disable();

#ifdef POWER_SLP_CLK_SWITCH_WAR
    /** Disable sleep clock before sleep and enable on warm boot as a workaround
     * to deal with XO settle related memory access issues when HW wakeup is
     * quicker. This is needed when XO detect is enabled instead of using fixed
     * XO settle time.
     */
    uint32_t aon_top = NT_REG_RD(QWLAN_PMU_AON_TOP_CFG_REG);
    aon_top &= ~QWLAN_PMU_AON_TOP_CFG_CFG_SLP_CLK_SWITCHING_EN_MASK;
    NT_REG_WR(QWLAN_PMU_AON_TOP_CFG_REG, aon_top);
#endif /* POWER_SLP_CLK_SWITCH_WAR */

    slp_ctrl_enable_sleep(mcu_sleep);
}

#else
static void _socpm_slpcfg_mcuslp(void)
{
    static uint16_t ncount = 0;
    uint32_t value;

    PM_STRUCT *pPmStruct = (PM_STRUCT *)gdevp->pPmStruct;
    pPmStruct->slp_clk_sel = _socpm_slp_clk_src;

#ifdef NT_FN_CPR
    nt_cpr_pre_sleep_config();
#endif  // NT_FN_CPR

    //_minprintf("L", _socpm_slp_clk_src, ncount);

#ifdef _SOCPM_INC_TST_MCUSLP_SBY
    _socpm_slpcfg_sby();
#endif

    /** Do not attempt to back up the ISER registers while in warm boot state
     * since they would not have been restored in the minimal code.
     * This scenario can occur when MCU sleep recipe is executed while in warm
     * boot state, such as for SWDTIM sleep.
     */
    if (FALSE == g_socpm_struct.in_warm_boot) {
        value = NT_REG_RD(NT_CM4_NVIC_ISER0_REG);
        nt_socpm_m4_regs[11] = value;

        value = NT_REG_RD(NT_CM4_NVIC_ISER1_REG);
        nt_socpm_m4_regs[12] = value;

        value = NT_REG_RD(NT_CM4_NVIC_ISER2_REG);
        nt_socpm_m4_regs[13] = value;
    }

    uint8_t wr_all_f = 1;  // skip/opt some writes out

#ifdef _SOCPM_INC_MCUSLP_WREG_COUNT
    ncount++;
#endif

    if (ncount > _SOCPM_REGWR_MAX) {
        wr_all_f = 0;
        if (ncount > _SOCPM_REGWR_RESET_LIMIT)
            ncount = 0;
    }

    NT_REG_WR(QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_REG, 0XF7DF);

#ifdef _SOCPM_INC_TST_SLP_PHY_RXP_RST
    // reset phy and rx - this might not be necessary
    NT_REG_WR(QWLAN_AGC_AGC_RESET_REG, QWLAN_AGC_AGC_RESET_RESET_ERESET);
    NT_REG_WR(QWLAN_RXP_CONFIG_REG, QWLAN_RXP_CONFIG_CFG_RXP_EN_DEFAULT);
#endif

    if (wr_all_f) {
#ifdef NT_FN_PDC_
        value = NT_REG_RD(QWLAN_PMU_DIG_TOP_CFG_REG);
        NT_REG_WR(QWLAN_PMU_DIG_TOP_CFG_REG, QWLAN_PMU_DIG_TOP_CFG_FORCE_BANK_A_CORE_ON_MASK |
                                                 QWLAN_PMU_DIG_TOP_CFG_FORCE_BANK_B_CORE_ON_MASK |
                                                 QWLAN_PMU_DIG_TOP_CFG_FORCE_BANK_C_CORE_ON_MASK |
                                                 QWLAN_PMU_DIG_TOP_CFG_FORCE_BANK_D_CORE_ON_MASK | value);
#else
        NT_REG_WR(QWLAN_PMU_DIG_TOP_CFG_REG, (0x100F));  // WMAC and Membank retention.
#endif  // NT_FN_PDC_
    }

    if (_socpm_slp_clk_src == NT_SOCPM_SLP_CLK_RFAXO) {
// TODO: Temp keep RFA DTOP ON in addition to PMIC and XO
// Note: RFA DTOP should be turned off - not needed for RFA/XO ops
#ifdef NT_FN_PDC_
        value = NT_REG_RD(QWLAN_PMU_CFG_AON_CNTL_MCU_SLEEP_STATE_RESOURCE_REQ_REG);
        NT_REG_WR(QWLAN_PMU_CFG_AON_CNTL_MCU_SLEEP_STATE_RESOURCE_REQ_REG,
                  (0b10 << QWLAN_PMU_CFG_AON_CNTL_MCU_SLEEP_STATE_RESOURCE_REQ_PD_MEM_MX_CNTL_BIT_OFFSET) |
                      QWLAN_PMU_CFG_AON_CNTL_MCU_SLEEP_STATE_RESOURCE_REQ_PD_RFA_DTOP_CNTL_BIT_MASK |
                      QWLAN_PMU_CFG_AON_CNTL_MCU_SLEEP_STATE_RESOURCE_REQ_PD_XO_DTOP_CNTL_BIT_MASK |
                      QWLAN_PMU_CFG_AON_CNTL_MCU_SLEEP_STATE_RESOURCE_REQ_PD_PMIC_DTOP_CNTL_BIT_MASK);
#endif  // NT_FN_PDC_
    } else  // PMIC XO or RC
    {
        if (wr_all_f) {
#ifdef NT_FN_PDC_
            value = NT_REG_RD(QWLAN_PMU_CFG_AON_CNTL_MCU_SLEEP_STATE_RESOURCE_REQ_REG);
            NT_REG_WR(QWLAN_PMU_CFG_AON_CNTL_MCU_SLEEP_STATE_RESOURCE_REQ_REG,
                      (0b10 << QWLAN_PMU_CFG_AON_CNTL_MCU_SLEEP_STATE_RESOURCE_REQ_PD_MEM_MX_CNTL_BIT_OFFSET));
#endif
        }
    }

    if (wr_all_f) {
#ifdef NT_FN_PDC_
        value = NT_REG_RD(QWLAN_PMU_CFG_MCU_SLEEP_STATE_RESOURCE_REQ_REG);
        NT_REG_WR(QWLAN_PMU_CFG_MCU_SLEEP_STATE_RESOURCE_REQ_REG,
                  (value | QWLAN_PMU_CFG_MCU_SLEEP_STATE_RESOURCE_REQ_PD_CMEM_BANK_A_CNTL_BIT_MASK |
                   QWLAN_PMU_CFG_MCU_SLEEP_STATE_RESOURCE_REQ_PD_CMEM_BANK_B_CNTL_BIT_MASK |
                   QWLAN_PMU_CFG_MCU_SLEEP_STATE_RESOURCE_REQ_PD_CMEM_BANK_C_CNTL_BIT_MASK |
                   QWLAN_PMU_CFG_MCU_SLEEP_STATE_RESOURCE_REQ_PD_CMEM_BANK_D_CNTL_BIT_MASK));
#endif  // NT_FN_PDC_

        value = NT_REG_RD(QWLAN_PMU_PMIC_DTOP_GDSCR_REG);
        NT_REG_WR(QWLAN_PMU_PMIC_DTOP_GDSCR_REG, value | QWLAN_PMU_PMIC_DTOP_GDSCR_RETAIN_FF_ENABLE_MASK);
    }
#ifdef NT_SOCPM_SW_MTUSR
    nt_socpm_mtusr_save_mtu_time();
#endif  // NT_SOCPM_SW_MTUS

    PM_SET_WLAN_STATE_OFF(pPmStruct);
    NT_REG_WR(QWLAN_PMU_CFG_WUR_SS_STATE_REG, NT_PMU_CFG_WUR_OFF_OFFSET);
    NT_REG_WR(QWLAN_PMU_CFG_WIFI_SS_STATE_REG, NT_PMU_CFG_WIFI_SLEEP_OFFSET);

    PM_SET_RRI_STATE(pPmStruct, PM_RRI_MAC_DOWN_MCUSLP);

#if defined(FEATURE_FPCI)
    fpci_evt_dispatch(PWR_EVT_WMAC_POST_SLEEP);
#endif /* FEATURE_FPCI */

    if (_socpm_slp_clk_src == NT_SOCPM_SLP_CLK_RFAXO) {
        // to use RFA XO as sleep, keep PMIC in active mode
        NT_REG_WR(QWLAN_PMU_CFG_PMIC_SLEEP_MODE_CNTL_REG,
                  QWLAN_PMU_CFG_PMIC_SLEEP_MODE_CNTL_CFG_DISABLE_PMIC_SLEEP_MODE_MASK);
    } else  // PMIC XO or RC
    {
        if (wr_all_f) {
            value = NT_REG_RD(QWLAN_PMU_RFA_DTOP_GDSCR_REG);
            NT_REG_WR(QWLAN_PMU_RFA_DTOP_GDSCR_REG, value | QWLAN_PMU_RFA_DTOP_GDSCR_RETAIN_FF_ENABLE_MASK);
        }
        NT_REG_WR(QWLAN_PMU_CFG_PMIC_SLEEP_MODE_CNTL_REG, QWLAN_PMU_CFG_PMIC_SLEEP_MODE_CNTL_DEFAULT);
    }

    NT_REG_WR(QWLAN_PMU_SLP_CNTL_REG, QWLAN_PMU_SLP_CNTL_EN_SLEEP_MASK);
    NT_REG_WR(_SOCPM_CPU_SYS_CTL_REG, 4);  // MCU sleep state
    NT_REG_WR(QWLAN_PMU_CFG_MCU_SS_STATE_REG, NT_PMU_CFG_MCU_SLEEP_OFFSET);
}
#endif /*PLATFORM_FERMION*/

#if defined(PLATFORM_FERMION)

/*****************************************************************
 * @brief Funtion to put the SOC in light sleep mode
 *
 * @param none
 * @return none
 ****************************************************************/
static void _socpm_slpcfg_light(void)
{
    uint32_t value;
    PM_STRUCT *pPmStruct = (PM_STRUCT *)gdevp->pPmStruct;

    if (FALSE == g_socpm_struct.in_warm_boot) {
        value = NT_REG_RD(NT_CM4_NVIC_ISER0_REG);
        nt_socpm_m4_regs[11] = value;

        value = NT_REG_RD(NT_CM4_NVIC_ISER1_REG);
        nt_socpm_m4_regs[12] = value;

        value = NT_REG_RD(NT_CM4_NVIC_ISER2_REG);
        nt_socpm_m4_regs[13] = value;

#ifdef FIRMWARE_APPS_INFORMED_WAKE
        value = NT_REG_RD(NT_CM4_NVIC_ISER3_REG);
        nt_socpm_m4_regs[14] = (value & A2F_DEASSERT_INTR_NVIC3_MASK);
#endif /* FIRMWARE_APPS_INFORMED_WAKE */
    }

#ifdef GPIO_RETENTION_IN_SLP
    /* Disable the GPIO retension */
    NT_REG_WR(QWLAN_PMU_CFG_IO_RET_CNTL_REG, QWLAN_PMU_CFG_IO_RET_CNTL_DEFAULT);
#endif /* GPIO_RETENTION_IN_SLP */
    NT_REG_WR(QWLAN_PMU_CFG_ACAL_VBAT_MON_EN_REG, 0);

    value = QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_DEFAULT;
    value &= ~QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_PD_XIP_CNTL_BIT_MASK;
    NT_REG_WR(QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_REG, value);

    socpm_clear_bbpll_toggle();

    value = QWLAN_PMU_DIG_TOP_CFG_DEFAULT;
    value |= QWLAN_PMU_DIG_TOP_CFG_PHY_RXTOP_REG_RET_EN_NON_SLEEP_MASK |
             QWLAN_PMU_DIG_TOP_CFG_PHY_RXA_REG_RET_EN_NON_SLEEP_MASK |
             QWLAN_PMU_DIG_TOP_CFG_PHY_TX_REG_RET_EN_NON_SLEEP_MASK;
    value |= QWLAN_PMU_DIG_TOP_CFG_PHY_RXA_REG_RET_EN_SLEEP_MASK | QWLAN_PMU_DIG_TOP_CFG_PHY_TX_REG_RET_EN_SLEEP_MASK;
    value |= QWLAN_PMU_DIG_TOP_CFG_FORCE_PHY_RX_CORE_ON_MASK | QWLAN_PMU_DIG_TOP_CFG_FORCE_PHY_TXTOP_CORE_ON_MASK |
             QWLAN_PMU_DIG_TOP_CFG_FORCE_PHY_TX_CORE_ON_MASK;
    value |= QWLAN_PMU_DIG_TOP_CFG_WMAC_REG_RET_EN_MASK;
    value |= QWLAN_PMU_DIG_TOP_CFG_FORCE_WMAC_CORE_ON_MASK;
    value |= QWLAN_PMU_DIG_TOP_CFG_FORCE_BANK_B_CORE_ON_MASK;
    value |= QWLAN_PMU_DIG_TOP_CFG_FORCE_BANK_C_CORE_ON_MASK;
    value |= QWLAN_PMU_DIG_TOP_CFG_FORCE_BANK_D_CORE_ON_MASK;
    value |= QWLAN_PMU_DIG_TOP_CFG_FORCE_BANK_E_CORE_ON_MASK;
    // TODO:PTPX have this bit set,to confirm
    value |= QWLAN_PMU_DIG_TOP_CFG_CFG_MEM_MX_DYNAMIC_SWITCHING_EN_MASK;
#ifndef PLATFORM_FERMION
    value|= QWLAN_PMU_DIG_TOP_CFG_RRAM_32_BIT_LEGACY_WR_MODE_MASK);
#endif
#ifdef PMU_REG_RETENTION_STATUS_FOR_SOC_SLP
    /* retaing RXDTOP REG in light sleep*/
    value |= nt_pm_set_and_get_pmu_dtop_reg_retention_status(Lightsleep);
#endif /* PMU_REG_RETENTION_STATUS_FOR_SOC_SLP */
    NT_REG_WR(QWLAN_PMU_DIG_TOP_CFG_REG, value);

    NT_REG_WR(QWLAN_PMU_CFG_XO_SETTLE_TIME_REG,
              1 | QWLAN_PMU_CFG_XO_SETTLE_TIME_CFG_XO_TRIM_TIMEOUT_DEFAULT);  // xo_settle_time=1
    // MX SUPPLY settle time mx_supply_settle_time=1
    value = NT_REG_RD(QWLAN_PMU_CFG_AON_SM_DELAYS_REG);
    value &= ~QWLAN_PMU_CFG_AON_SM_DELAYS_CFG_MX_SUPPLY_SETTLE_TIME_MASK;
    value |= ((1 << QWLAN_PMU_CFG_AON_SM_DELAYS_CFG_MX_SUPPLY_SETTLE_TIME_OFFSET) &
              QWLAN_PMU_CFG_AON_SM_DELAYS_CFG_MX_SUPPLY_SETTLE_TIME_MASK);
    NT_REG_WR(QWLAN_PMU_CFG_AON_SM_DELAYS_REG, value);

    value = QWLAN_PMU_AON_TOP_CFG_DEFAULT;
    value &= ~QWLAN_PMU_AON_TOP_CFG_CFG_ENABLE_XO_CLK_DETECT_MASK;
    value &= ~(QWLAN_PMU_AON_TOP_CFG_CFG_SLP_CLK_SWITCHING_EN_MASK);
    value |= QWLAN_PMU_AON_TOP_CFG_AON_WDOG_SLP_ROOT_CLK_ENABLE_MASK;
    value |= QWLAN_PMU_AON_TOP_CFG_CFG_WAKEUP_MCU_SS_ON_DTIM_INTR_MASK;
    value |= QWLAN_PMU_AON_TOP_CFG_CFG_WAKEUP_WIFI_SS_ON_DTIM_INTR_MASK;
    value |= QWLAN_PMU_AON_TOP_CFG_CFG_LIGHTSLEEP_EN_MASK;
    value &= ~QWLAN_PMU_AON_TOP_CFG_CFG_AON_PMIC_AONLDO_INPUT_SEL_MASK;
    value |= ((1 << QWLAN_PMU_AON_TOP_CFG_CFG_AON_PMIC_AONLDO_INPUT_SEL_OFFSET) &
              QWLAN_PMU_AON_TOP_CFG_CFG_AON_PMIC_AONLDO_INPUT_SEL_MASK);
    NT_REG_WR(QWLAN_PMU_AON_TOP_CFG_REG, value);

    value = NT_REG_RD(QWLAN_PMU_SON_GDSCR_REG);  // SOC part
    value |= QWLAN_PMU_SON_GDSCR_RETAIN_FF_ENABLE_MASK;
    value &= ~(QWLAN_PMU_SON_GDSCR_EN_FEW_WAIT_MASK | QWLAN_PMU_SON_GDSCR_EN_REST_WAIT_MASK);
    // VIFERMION-215
    // VIFERMION-379
    value |= (son_en_wait_light << QWLAN_PMU_SON_GDSCR_EN_FEW_WAIT_OFFSET) & QWLAN_PMU_SON_GDSCR_EN_FEW_WAIT_MASK;
    value |= (son_en_wait_light << QWLAN_PMU_SON_GDSCR_EN_REST_WAIT_OFFSET) & QWLAN_PMU_SON_GDSCR_EN_REST_WAIT_MASK;
    NT_REG_WR(QWLAN_PMU_SON_GDSCR_REG, value);

    // Disable HW-DTIM with no CPU reset release. use_hwdtim_with_cpu_on=0,check_tbtt_count=0,check_dc_count=0
    value = NT_REG_RD(QWLAN_PMU_CFG_HW_DTIM_MODE_REG);
    value &= ~QWLAN_PMU_CFG_HW_DTIM_MODE_WMAC_HW_DTIM_MODE_MASK;
    value &= ~QWLAN_PMU_CFG_HW_DTIM_MODE_WMAC_HW_DTIM_MODE_WITH_CPU_ON_MASK;
    NT_REG_WR(QWLAN_PMU_CFG_HW_DTIM_MODE_REG, value);

    // SLEEP MODE enable disable_sleep_mode_en=1
    value = NT_REG_RD(QWLAN_PMU_CFG_PMIC_SLEEP_MODE_CNTL_REG);
    value |= QWLAN_PMU_CFG_PMIC_SLEEP_MODE_CNTL_CFG_DISABLE_PMIC_SLEEP_MODE_MASK;
    NT_REG_WR(QWLAN_PMU_CFG_PMIC_SLEEP_MODE_CNTL_REG, value);

    // SLP CLK CAL slp_clk_cal_en=0
#ifdef SLEEP_CLK_CAL_IN_SLEEP_MODE
    if (false == g_socpm_struct.slp_clk_cal_params.sleep_mode_cal_enabled) {
#endif /* SLEEP_CLK_CAL_IN_SLEEP_MODE */
        value = NT_REG_RD(QWLAN_PMU_CFG_SLP_CAL_CAL_EN_REG);
        value &= ~QWLAN_PMU_CFG_SLP_CAL_CAL_EN_CFG_SLP_CLK_CAL_EN_MASK;
        NT_REG_WR(QWLAN_PMU_CFG_SLP_CAL_CAL_EN_REG, value);
#ifdef SLEEP_CLK_CAL_IN_SLEEP_MODE
    }
#endif /* SLEEP_CLK_CAL_IN_SLEEP_MODE */

    // Disable CPR and set CX LDO sleep voltage to 0.6V (experimental value)
    wifi_fw_cpr_disable();

    // WIFI SLEEP RRT
    value = QWLAN_PMU_CFG_AON_CNTL_MCU_SLEEP_STATE_RESOURCE_REQ_DEFAULT;
    // keep XO and PMIC DTOP ON xo_dtop_on=1 pmic_dtop_on=1
    value |= QWLAN_PMU_CFG_AON_CNTL_MCU_SLEEP_STATE_RESOURCE_REQ_PD_XO_DTOP_CNTL_BIT_MASK;
    value |= QWLAN_PMU_CFG_AON_CNTL_MCU_SLEEP_STATE_RESOURCE_REQ_PD_PMIC_DTOP_CNTL_BIT_MASK;
    value &= ~QWLAN_PMU_CFG_AON_CNTL_MCU_SLEEP_STATE_RESOURCE_REQ_PD_RFA_DTOP_CNTL_BIT_MASK;
    value = NT_REG_WR(QWLAN_PMU_CFG_AON_CNTL_MCU_SLEEP_STATE_RESOURCE_REQ_REG, value);

    value = NT_REG_RD(QWLAN_PMU_PMIC_DTOP_GDSCR_REG);
    NT_REG_WR(QWLAN_PMU_PMIC_DTOP_GDSCR_REG, (value | QWLAN_PMU_PMIC_DTOP_GDSCR_RETAIN_FF_ENABLE_MASK));

    value = NT_REG_RD(QWLAN_PMU_RFA_DTOP_GDSCR_REG);
    NT_REG_WR(QWLAN_PMU_RFA_DTOP_GDSCR_REG, value | QWLAN_PMU_RFA_DTOP_GDSCR_RETAIN_FF_ENABLE_MASK);

    value = NT_REG_RD(QWLAN_PMU_XO_GDSCR_REG);
    NT_REG_WR(QWLAN_PMU_XO_GDSCR_REG, value | QWLAN_PMU_XO_GDSCR_RETAIN_FF_ENABLE_MASK);

    /* if a clock gated sleep precedes the protocol sleeps, wifi state moves to config on AON timer expiry
       Move wifi to sleep state back before hitting the wfi. if wifi is not in sleep state, the chip remains
       in wfi until AON timer interrupt or some other interrupt happens*/

    value = HWIO_INXF(SEQ_WCSS_PMU_OFFSET, NEUTRINO_PMU_PRONTO_LP_FRODO_PMU_CFG_WIFI_SS_STATE, WIFI_SS_CURR_STATE);

    if (value != NT_PMU_CFG_WIFI_SLEEP_OFFSET) {
        HWIO_OUTXF(SEQ_WCSS_PMU_OFFSET, NEUTRINO_PMU_PRONTO_LP_FRODO_PMU_CFG_WIFI_SS_STATE, CFG_WIFI_SS_NEXT_STATE,
                   NT_PMU_CFG_WIFI_SLEEP_OFFSET);
    }

    PM_SET_RRI_STATE(pPmStruct, PM_RRI_MAC_DOWN_LIGHT);

    slp_gpio_pupd_disable();

    slp_ctrl_enable_sleep(Lightsleep);
}

#endif /*PLATFORM_FERMION*/

void nt_enable_indef_deepsleep(uint64_t sleep_time)
{
#ifdef CONFIG_BOARD_QCC730_QSPI_ENABLE
    drv_flash_deinit(0);
#endif

#ifdef PLATFORM_FERMION
    wifi_fw_pmic_pre_sleep_config(Standby);
#endif /* PLATFORM_FERMION */

    // nt_socpm_slp_tmr_set(sleep_time);
    socpm_enter_deepsleep();
}

// static void __attribute__((used)) socpm_enter_deepsleep()
static void socpm_enter_deepsleep()
{
    uint32_t reg_val;
    __asm volatile("cpsid i \n");
    __asm volatile("cpsid f \n");

    nt_wlan_deepsleep();

    reg_val = NT_REG_RD(QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_REG);
    reg_val |= QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_PD_XIP_CNTL_BIT_MASK;
    NT_REG_WR(QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_REG, reg_val);
    NT_REG_WR(QWLAN_PMU_CFG_MCU_DEEPSLEEP_STATE_RESOURCE_REQ_REG, 0);
    nt_socpm_nop_delay(2000);

    NT_REG_WR(QWLAN_PMU_CFG_ACAL_VBAT_MON_EN_REG, 0);
    NT_REG_WR(QWLAN_PMU_CFG_SLP_CAL_CAL_EN_REG, 0);

    reg_val = NT_REG_RD(QWLAN_PMU_PMIC_DTOP_GDSCR_REG);
    NT_REG_WR(QWLAN_PMU_PMIC_DTOP_GDSCR_REG, reg_val | QWLAN_PMU_PMIC_DTOP_GDSCR_RETAIN_FF_ENABLE_MASK);
    NT_REG_WR(QWLAN_PMU_CFG_PMIC_SLEEP_MODE_CNTL_REG, 0);

    reg_val = NT_REG_RD(QWLAN_PMU_RFA_DTOP_GDSCR_REG);
    reg_val &= ~QWLAN_PMU_RFA_DTOP_GDSCR_RETAIN_FF_ENABLE_MASK;
    NT_REG_WR(QWLAN_PMU_RFA_DTOP_GDSCR_REG, reg_val);

    reg_val = NT_REG_RD(QWLAN_PMU_XO_GDSCR_REG);
    NT_REG_WR(QWLAN_PMU_XO_GDSCR_REG, reg_val | QWLAN_PMU_XO_GDSCR_RETAIN_FF_ENABLE_MASK);

    wifi_fw_cpr_disable();

    if (g_socpm_struct.socpm_indef_deep_sleep_en) {
#ifdef FIRMWARE_APPS_INFORMED_WAKE
        aon_ext_wakeup_set_lvl_trigger();
#endif
        NT_REG_WR(QWLAN_PMU_AON_TOP_CFG_REG,
                  QWLAN_PMU_AON_TOP_CFG_DEFAULT | QWLAN_PMU_AON_TOP_CFG_CFG_INDEFINITE_DEEPSLEEP_EN_MASK);
    } else {
        reg_val = QWLAN_PMU_AON_TOP_CFG_DEFAULT;
        // reg_val |= QWLAN_PMU_AON_TOP_CFG_CFG_P6V_SMPS_EN_MASK;
        reg_val |= QWLAN_PMU_AON_TOP_CFG_AON_WDOG_SLP_ROOT_CLK_ENABLE_MASK;
        reg_val &= ~QWLAN_PMU_AON_TOP_CFG_CFG_ENABLE_XO_CLK_DETECT_MASK;
        reg_val &= ~QWLAN_PMU_AON_TOP_CFG_CFG_AON_PMIC_AONLDO_INPUT_SEL_MASK;
        reg_val |= ((1 << QWLAN_PMU_AON_TOP_CFG_CFG_AON_PMIC_AONLDO_INPUT_SEL_OFFSET) &
                    QWLAN_PMU_AON_TOP_CFG_CFG_AON_PMIC_AONLDO_INPUT_SEL_MASK);
        NT_REG_WR(QWLAN_PMU_AON_TOP_CFG_REG, reg_val);
    }

    reg_val = QWLAN_PMU_DIG_TOP_CFG_DEFAULT;
    reg_val |= QWLAN_PMU_DIG_TOP_CFG_CFG_MEM_MX_DYNAMIC_SWITCHING_EN_MASK;
    // reg_val |= QWLAN_PMU_DIG_TOP_CFG_RRAM_32_BIT_LEGACY_WR_MODE_MASK;
    NT_REG_WR(QWLAN_PMU_DIG_TOP_CFG_REG, reg_val);

    reg_val = QWLAN_PMU_CFG_AON_CNTL_MCU_DEEPSLEEP_STATE_RESOURCE_REQ_DEFAULT;
    // reg_val |= QWLAN_PMU_CFG_AON_CNTL_MCU_DEEPSLEEP_STATE_RESOURCE_REQ_PD_PMIC_DTOP_CNTL_BIT_MASK;
    NT_REG_WR(QWLAN_PMU_CFG_AON_CNTL_MCU_DEEPSLEEP_STATE_RESOURCE_REQ_REG, reg_val);

    slp_gpio_pupd_disable();

    slp_ctrl_enable_sleep(Standby);

    _tst_sleep_enter();
}

#ifdef PLATFORM_FERMION
static void _socpm_slpcfg_sby(void)
{
    uint32_t reg_val;

    // socpm_enter_deepsleep();
    // return;

    // disable all interrupts while entering deepsleep, just to avoid unexpected ints
    __asm volatile("cpsid i \n");  // Disable interrupts by setting the PRIMASK
    __asm volatile("cpsid f \n");  // Disable exceptions by setting the FAULTMASK
    _socpm_slp_mode = Standby;

    // clear possible PLL lock toggle intr
    socpm_clear_bbpll_toggle();

    reg_val = NT_REG_RD(QWLAN_PMU_SON_GDSCR_REG);
    reg_val |= QWLAN_PMU_SON_GDSCR_RETAIN_FF_ENABLE_MASK;
    reg_val &= ~(QWLAN_PMU_SON_GDSCR_EN_FEW_WAIT_MASK | QWLAN_PMU_SON_GDSCR_EN_REST_WAIT_MASK);
    // VIFERMION-215
    // VIFERMION-379
    reg_val |= (son_en_wait_sby << QWLAN_PMU_SON_GDSCR_EN_FEW_WAIT_OFFSET) & QWLAN_PMU_SON_GDSCR_EN_FEW_WAIT_MASK;
    reg_val |= (son_en_wait_sby << QWLAN_PMU_SON_GDSCR_EN_REST_WAIT_OFFSET) & QWLAN_PMU_SON_GDSCR_EN_REST_WAIT_MASK;
    NT_REG_WR(QWLAN_PMU_SON_GDSCR_REG, reg_val);

    nt_wlan_deepsleep();

    reg_val = NT_REG_RD(QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_REG);
    reg_val |= QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_PD_XIP_CNTL_BIT_MASK;
    NT_REG_WR(QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_REG, reg_val);
    NT_REG_WR(QWLAN_PMU_CFG_MCU_DEEPSLEEP_STATE_RESOURCE_REQ_REG, 0);
    nt_socpm_nop_delay(2000);

#if _SOCPM_TST_MODE_PMIC_CFG_SBY != 2
    nt_socpm_slp_tmr_set(5000);  // wake/badvbatt
#endif

    NT_REG_WR(QWLAN_PMU_CFG_ACAL_VBAT_MON_EN_REG, 0);
#ifndef _SOCPM_TST_SBY_SLP_CAL_DIS
    /* Sleep calibration is disabled in active/sleep mode */
    /* SLP_CLK_CNT_REG (VI team code data) */
    NT_REG_WR(QWLAN_PMU_CFG_REF_SLP_CLK_CNT_REG, 0x20);
    /* CFG_CAL_DATA_REF VI team data copied */
    NT_REG_WR(QWLAN_PMU_CFG_CAL_DATA_REF_REG, 0x7a12);  // ref_data_cnt
#else
    NT_REG_WR(QWLAN_PMU_CFG_SLP_CAL_CAL_EN_REG, 0x0);
#endif  // _SOCPM_TST_SBY_SLP_CAL_DIS

    /* PMIC DTOP GDSCR retain  */
    reg_val = NT_REG_RD(QWLAN_PMU_PMIC_DTOP_GDSCR_REG);
    NT_REG_WR(QWLAN_PMU_PMIC_DTOP_GDSCR_REG, reg_val | QWLAN_PMU_PMIC_DTOP_GDSCR_RETAIN_FF_ENABLE_MASK);
    NT_REG_WR(QWLAN_PMU_CFG_PMIC_SLEEP_MODE_CNTL_REG, 0);

    /* Disabling RFA DTOP before deep sleep to fix TX/RX issues seen after deep sleep exit */
    reg_val = NT_REG_RD(QWLAN_PMU_RFA_DTOP_GDSCR_REG);
    reg_val &= ~QWLAN_PMU_RFA_DTOP_GDSCR_RETAIN_FF_ENABLE_MASK;
    NT_REG_WR(QWLAN_PMU_RFA_DTOP_GDSCR_REG, reg_val);

    /* Dont retain XO GDSCR for deep sleep */
    reg_val = NT_REG_RD(QWLAN_PMU_XO_GDSCR_REG);
    reg_val &= ~QWLAN_PMU_XO_GDSCR_RETAIN_FF_ENABLE_MASK;
    NT_REG_WR(QWLAN_PMU_XO_GDSCR_REG, reg_val);

    // Disable CPR and set CX LDO sleep voltage to 0.6V (experimental value)
    wifi_fw_cpr_disable();
#ifdef FERMION_1_0_POWER_WAR
    /* WAR to address HW issues related to RRAM IR drop
     * and PLL not locked seen on some boards on
     * exit from deep sleep. */

    reg_val = NT_REG_RD(QWLAN_PMU_SON_GDSCR_REG);
    reg_val |= QWLAN_PMU_SON_GDSCR_RETAIN_FF_ENABLE_MASK;
    reg_val &= ~(QWLAN_PMU_SON_GDSCR_EN_FEW_WAIT_MASK | QWLAN_PMU_SON_GDSCR_EN_REST_WAIT_MASK);
    reg_val |= (0xF << QWLAN_PMU_SON_GDSCR_EN_FEW_WAIT_OFFSET) & QWLAN_PMU_SON_GDSCR_EN_FEW_WAIT_MASK;
    reg_val |= (0xF << QWLAN_PMU_SON_GDSCR_EN_REST_WAIT_OFFSET) & QWLAN_PMU_SON_GDSCR_EN_REST_WAIT_MASK;
    NT_REG_WR(QWLAN_PMU_SON_GDSCR_REG, reg_val);     // for BBPLL issue
    NT_REG_WR(QWLAN_PMU_CFG_PWFM_TRAGET_REG, 0xF0);  // for RRAM IR drop
#else
    /* Disable SON retention before deep sleep.
     * Fixes CPR not working on exit from deep sleep.
     */
    reg_val = NT_REG_RD(QWLAN_PMU_SON_GDSCR_REG);
    reg_val &= ~QWLAN_PMU_SON_GDSCR_RETAIN_FF_ENABLE_MASK;
    NT_REG_WR(QWLAN_PMU_SON_GDSCR_REG, reg_val);
#endif /* FERMION_1_0_POWER_WAR */

    _socpm_slp_timing_tuning();

#ifdef FEATURE_INDEF_DEEP_SLP
    if (g_socpm_struct.socpm_indef_deep_sleep_en) {
        _socpm_slp_mode = InfDeepsleep;
#ifdef FIRMWARE_APPS_INFORMED_WAKE
        aon_ext_wakeup_set_lvl_trigger();
#endif /* FIRMWARE_APPS_INFORMED_WAKE */
        reg_val |= QWLAN_PMU_AON_TOP_CFG_CFG_INDEFINITE_DEEPSLEEP_EN_MASK;
        reg_val |= QWLAN_PMU_AON_TOP_CFG_AON_WDOG_SLP_ROOT_CLK_ENABLE_MASK;
        NT_REG_WR(QWLAN_PMU_AON_TOP_CFG_REG, reg_val);
    } else
#endif /* FEATURE_INDEF_DEEP_SLP */
    {
        reg_val = QWLAN_PMU_AON_TOP_CFG_DEFAULT;
        // reg_val |= QWLAN_PMU_AON_TOP_CFG_CFG_P6V_SMPS_EN_MASK;
        reg_val |= QWLAN_PMU_AON_TOP_CFG_AON_WDOG_SLP_ROOT_CLK_ENABLE_MASK;
        reg_val &= ~QWLAN_PMU_AON_TOP_CFG_CFG_ENABLE_XO_CLK_DETECT_MASK;
        reg_val &= ~QWLAN_PMU_AON_TOP_CFG_CFG_AON_PMIC_AONLDO_INPUT_SEL_MASK;
        reg_val |= ((1 << QWLAN_PMU_AON_TOP_CFG_CFG_AON_PMIC_AONLDO_INPUT_SEL_OFFSET) &
                    QWLAN_PMU_AON_TOP_CFG_CFG_AON_PMIC_AONLDO_INPUT_SEL_MASK);
        NT_REG_WR(QWLAN_PMU_AON_TOP_CFG_REG, reg_val);
    }

    reg_val = QWLAN_PMU_DIG_TOP_CFG_DEFAULT;
    reg_val &= ~(QWLAN_PMU_DIG_TOP_CFG_WMAC_REG_RET_EN_MASK);
    reg_val |= QWLAN_PMU_DIG_TOP_CFG_CFG_MEM_MX_DYNAMIC_SWITCHING_EN_MASK;
    // reg_val |= QWLAN_PMU_DIG_TOP_CFG_RRAM_32_BIT_LEGACY_WR_MODE_MASK;
    NT_REG_WR(QWLAN_PMU_DIG_TOP_CFG_REG, reg_val);

    reg_val = QWLAN_PMU_CFG_AON_CNTL_MCU_DEEPSLEEP_STATE_RESOURCE_REQ_DEFAULT;
    // reg_val |= QWLAN_PMU_CFG_AON_CNTL_MCU_DEEPSLEEP_STATE_RESOURCE_REQ_PD_PMIC_DTOP_CNTL_BIT_MASK;
    NT_REG_WR(QWLAN_PMU_CFG_AON_CNTL_MCU_DEEPSLEEP_STATE_RESOURCE_REQ_REG, reg_val);

    slp_gpio_pupd_disable();

    // Move MCU to deepsleep state
    slp_ctrl_enable_sleep(Standby);

    // Execute wfi to enter sleep
#ifdef _SOCPM_INC_TST_SLEEP
    _tst_sleep_enter();
#endif
    /* Check if sleep entry was prevented and assert if not a valid prevention */
    nt_socpm_check_sleep_entry_failure(_socpm_slp_mode, FALSE,FALSE);
}

#else

static void _socpm_slpcfg_sby(void)
{
    uint32_t reg_val;

    // disable all interrupts while entering deepsleep, just to avoid unexpected ints
    __asm volatile("cpsid i \n");  // Disable interrupts by setting the PRIMASK
    __asm volatile("cpsid f \n");  // Disable exceptions by setting the FAULTMASK

    NT_REG_WR(QWLAN_RXP_CONFIG_REG, 0);  // Disable RX

    NT_REG_WR(QWLAN_PMU_CFG_WUR_SS_STATE_REG, NT_PMU_CFG_WUR_SLEEP);               // Wur sleep state me
    NT_REG_WR(QWLAN_PMU_CFG_WIFI_SS_STATE_REG, NT_PMU_CFG_WIFI_DEEPSLEEP_OFFSET);  // Wifi sleep state to deep sleep

    // turn on phy domains to access pmic space
    NT_REG_WR(QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_REG, 0x1FF);
    NT_REG_WR(QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_REG, 0xffdf);

    // flush all aon writes
    reg_val = NT_REG_RD(QWLAN_PMU_AON_TOP_CFG_REG);
    nt_socpm_nop_delay(20000);

    NT_REG_WR(QWLAN_PMU_DIG_TOP_CFG_REG, (0b10 << 11));  // RRAM powered down 0b10 << 11
    nt_socpm_nop_delay(2000);

    // Turn OFF SMPS2
    NT_REG_WR(NT_PMU_PMIC_CFG_SMPS2_6_REG, NT_PMU_PMIC_SMPS2_POK_FORCE_MASK);  // turning of smps_pok_force
    // NT_REG_WR(NT_PMU_PMIC_CFG_SMPS2_6_REG, 0); //turning of smps_pok_force
    NT_REG_WR(NT_PMU_PMIC_CFG_SMPS2_7_REG, 0xED000000);  // smps_seg_en to 0x0

    // test ulpm_smsp1_en_ovr
    //  reg_val = NT_REG_RD(NT_PMU_PMIC_CFG_SMPS1_7_REG);
    //  NT_REG_WR(NT_PMU_PMIC_CFG_SMPS1_7_REG, reg_val | 0x200000); //bit 21, ulpm_smps1_en_ovr

    NT_REG_WR(QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_REG, 0xfdf);
    nt_socpm_nop_delay(2000);

#if _SOCPM_TST_MODE_PMIC_CFG_SBY != 2
    nt_socpm_slp_tmr_set(5000);  // wake/badvbatt
#endif

#ifndef _SOCPM_TST_SBY_SLP_CAL_DIS
    /* Sleep calibration is disabled in active/sleep mode */
    NT_REG_WR(QWLAN_PMU_CFG_SLP_CAL_CAL_EN_REG, ~QWLAN_PMU_CFG_SLP_CAL_CAL_EN_CFG_SLP_CLK_CAL_EN_MASK);
    /* SLP_CLK_CNT_REG (VI team code data) */
    NT_REG_WR(QWLAN_PMU_CFG_REF_SLP_CLK_CNT_REG, 0x20);
    /* CFG_CAL_DATA_REF VI team data copied */
    NT_REG_WR(QWLAN_PMU_CFG_CAL_DATA_REF_REG, 0x7a12);  // ref_data_cnt
#endif  // _SOCPM_TST_SBY_SLP_CAL_DIS

    /* PMIC DTOP GDSCR retain  */
    reg_val = NT_REG_RD(QWLAN_PMU_PMIC_DTOP_GDSCR_REG);
    NT_REG_WR(QWLAN_PMU_PMIC_DTOP_GDSCR_REG, reg_val | QWLAN_PMU_PMIC_DTOP_GDSCR_RETAIN_FF_ENABLE_MASK);
    NT_REG_WR(QWLAN_PMU_CFG_PMIC_SLEEP_MODE_CNTL_REG, 0);

    /* Disabling RFA DTOP before standby sleep*/
    reg_val = NT_REG_RD(QWLAN_PMU_RFA_DTOP_GDSCR_REG);
    reg_val = reg_val & ~(QWLAN_PMU_RFA_DTOP_GDSCR_RETAIN_FF_ENABLE_MASK);
    NT_REG_WR(QWLAN_PMU_RFA_DTOP_GDSCR_REG, reg_val);

    /* Sleep clock has to be set */
    NT_REG_WR(QWLAN_PMU_AON_TOP_CFG_REG, QWLAN_PMU_AON_TOP_CFG_DEFAULT);
    nt_socpm_nop_delay(2000);  // gratuitous, this delay may not be needed

    // this helps resolve Iio RRAM PD+Reset issue, but at the cost of vbatt
    // NT_REG_WR(QWLAN_PMU_CFG_MCU_DEEPSLEEP_STATE_RESOURCE_REQ_REG,      0x20);

    // Move MCU to deepsleep state
    NT_REG_WR(_SOCPM_CPU_SYS_CTL_REG, 4);  // deepsleep (bit #2)

    // flush all aon writes
    reg_val = NT_REG_RD(QWLAN_PMU_AON_TOP_CFG_REG);

    NT_REG_WR(QWLAN_PMU_SLP_CNTL_REG, QWLAN_PMU_SLP_CNTL_EN_DEEPSLEEP_MASK);

    // Move MCU to STandby state
    // don't do this, moves to deepsleep immediately, without waiting for wfi
    // NT_REG_WR(QWLAN_PMU_CFG_MCU_SS_STATE_REG, NT_PMU_CFG_MCU_DEEPSLEEP_OFFSET); //MCU Standby*/

    // nt_socpm_slp_tmr_set(5000); //nowake/goodvbatt

    // flush aon writes
    reg_val = NT_REG_RD(QWLAN_PMU_AON_TOP_CFG_REG);

#ifdef _SOCPM_INC_TST_SLEEP
    _tst_sleep_enter();
#endif
}

#endif /* PLATFORM_FERMION */

static void _socpm_rrt_act_init(void)
{
    uint32_t value;
#ifdef _SOCPM_INC_TST_MCU_ACT_CHG
    value = (QWLAN_PMU_CFG_AON_CNTL_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_PMIC_DTOP_CNTL_BIT_MASK |
             QWLAN_PMU_CFG_AON_CNTL_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_XO_DTOP_CNTL_BIT_MASK |
             QWLAN_PMU_CFG_AON_CNTL_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_RFA_DTOP_CNTL_BIT_MASK |
             QWLAN_PMU_CFG_AON_CNTL_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_SON_CNTL_BIT_MASK);
    value |= _PMU_CFG_AON_CNTL_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_MEM_MX_CNTL_BIT_MEM_NOR_MASK;
    NT_REG_WR(QWLAN_PMU_CFG_AON_CNTL_MCU_ACTIVE_STATE_RESOURCE_REQ_REG, value);

#ifdef PLATFORM_FERMION
    value = (QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_CMNSS_CNTL_BIT_MASK |
             QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_CMEM_BANK_A_CNTL_BIT_MASK |
             QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_CMEM_BANK_B_CNTL_BIT_MASK |
             QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_CMEM_BANK_C_CNTL_BIT_MASK |
             QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_CMEM_BANK_D_CNTL_BIT_MASK |
             QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_CMEM_BANK_E_CNTL_BIT_MASK |
             QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_QSPI_CNTL_BIT_MASK |
             QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_XIP_CNTL_BIT_MASK
#ifndef EMULATION_WAR
             | QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_PSS_CNTL_BIT_MASK
#endif
    );
    NT_REG_WR(QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_REG, value);
#else  /* PLATFORM_FERMION */
    value = (QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_CMNSS_CNTL_BIT_MASK |
             QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_CMEM_BANK_A_CNTL_BIT_MASK |
             QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_CMEM_BANK_B_CNTL_BIT_MASK |
             QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_CMEM_BANK_C_CNTL_BIT_MASK |
             QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_CMEM_BANK_D_CNTL_BIT_MASK |
             QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_XIP_CNTL_BIT_MASK);
    NT_REG_WR(QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_REG, value);
#endif /* PLATFORM_FERMION */

#endif /* _SOCPM_INC_TST_MCU_ACT_CHG */
}

static void _socpm_rrt_slp_init(void)
{
    // reset to 0 for test
    NT_REG_WR(QWLAN_PMU_PMU_TESTBUS_CTL_REG, 0);

    // disable all cal/vbat
    NT_REG_WR(QWLAN_PMU_CFG_ACAL_VBAT_MON_EN_REG, 0);

    if (_socpm_slp_clk_src == NT_SOCPM_SLP_CLK_RFAXO) {
        NT_REG_WR(QWLAN_PMU_CFG_AON_CNTL_MCU_SLEEP_STATE_RESOURCE_REQ_REG,
                  (0b10 << QWLAN_PMU_CFG_AON_CNTL_MCU_SLEEP_STATE_RESOURCE_REQ_PD_MEM_MX_CNTL_BIT_OFFSET) |
                      QWLAN_PMU_CFG_AON_CNTL_MCU_SLEEP_STATE_RESOURCE_REQ_PD_RFA_DTOP_CNTL_BIT_MASK |
                      QWLAN_PMU_CFG_AON_CNTL_MCU_SLEEP_STATE_RESOURCE_REQ_PD_XO_DTOP_CNTL_BIT_MASK |
                      QWLAN_PMU_CFG_AON_CNTL_MCU_SLEEP_STATE_RESOURCE_REQ_PD_PMIC_DTOP_CNTL_BIT_MASK);
    } else {
        NT_REG_WR(QWLAN_PMU_CFG_AON_CNTL_MCU_SLEEP_STATE_RESOURCE_REQ_REG,
                  (0b10 << QWLAN_PMU_CFG_AON_CNTL_MCU_SLEEP_STATE_RESOURCE_REQ_PD_MEM_MX_CNTL_BIT_OFFSET));
    }
    NT_REG_WR(QWLAN_PMU_CFG_MCU_SLEEP_STATE_RESOURCE_REQ_REG, 0x1001E);

    NT_REG_WR(QWLAN_PMU_CFG_AON_CNTL_WIFI_SLEEP_STATE_RESOURCE_REQ_REG, 0);
    NT_REG_WR(QWLAN_PMU_CFG_WIFI_SLEEP_STATE_RESOURCE_REQ_REG, 0);

    NT_REG_WR(QWLAN_PMU_CFG_AON_CNTL_WUR_SLEEP_STATE_RESOURCE_REQ_REG, 0);
    NT_REG_WR(QWLAN_PMU_CFG_WUR_SLEEP_STATE_RESOURCE_REQ_REG, 0);

#if 0
        //uint32_t value;

        value = NT_REG_RD(QWLAN_PMU_PMIC_DTOP_GDSCR_REG);
        NT_REG_WR( QWLAN_PMU_PMIC_DTOP_GDSCR_REG, value | QWLAN_PMU_PMIC_DTOP_GDSCR_RETAIN_FF_ENABLE_MASK);

        value = NT_REG_RD(QWLAN_PMU_RFA_DTOP_GDSCR_REG);
        NT_REG_WR( QWLAN_PMU_RFA_DTOP_GDSCR_REG, value | QWLAN_PMU_RFA_DTOP_GDSCR_RETAIN_FF_ENABLE_MASK);
#endif
}

static void _socpm_rrt_sby_init(void)
{
    // disable all cal/vbat
    NT_REG_WR(QWLAN_PMU_CFG_ACAL_VBAT_MON_EN_REG, 0);

    NT_REG_WR(QWLAN_PMU_CFG_AON_CNTL_WUR_SLEEP_STATE_RESOURCE_REQ_REG, 0);
    NT_REG_WR(QWLAN_PMU_CFG_WUR_SLEEP_STATE_RESOURCE_REQ_REG, 0);

    NT_REG_WR(QWLAN_PMU_CFG_WIFI_DEEPSLEEP_STATE_RESOURCE_REQ_REG, 0);
    NT_REG_WR(QWLAN_PMU_CFG_AON_CNTL_WIFI_DEEPSLEEP_STATE_RESOURCE_REQ_REG, 0);

    NT_REG_WR(QWLAN_PMU_CFG_MCU_DEEPSLEEP_STATE_RESOURCE_REQ_REG, 0);
    // leave XIP/RRAM state unchanged from mcu active state, else Iio goes up
    // NT_REG_WR(QWLAN_PMU_CFG_MCU_DEEPSLEEP_STATE_RESOURCE_REQ_REG, 0x20);
}

static void _socpm_rrt_init(void)
{
    _socpm_rrt_act_init();
    _socpm_rrt_slp_init();
    _socpm_rrt_sby_init();
}

#ifdef PLATFORM_FERMION
static void wifi_fw_coldboot_soc_init(void)
{
    HWIO_OUTX2F(SEQ_WCSS_PMU_OFFSET, NEUTRINO_PMU_PRONTO_LP_FRODO_PMU_AON_TOP_CFG, CFG_AON_PMIC_AONLDO_INPUT_SEL,
                CFG_SLP_CLK_SWITCHING_EN, 0x1, 0x1);
}
#endif /* PLATFORM_FERMION */

/*
 *  @brief : Read SOC power devcfg parameters before PMIC and SOCPM init
 *  @param : None
 *  @return : None
 */
void nt_socpm_init_soc_cfg(void)
{
    _socpm_slp_clk_src = *((uint8_t *)(nt_devcfg_get_config(NT_DEVCFG_SLEEP_CLOCK_SELECTION_FOR_AON)));
#ifdef SUPPORT_TWT_STA
    _socpm_slp_time_supp_min_ms = MINIMUM_SLP_TIME_FOR_AON;
#else  /* SUPPORT_TWT_STA */
    _socpm_slp_time_supp_min_ms = *((uint8_t *)(nt_devcfg_get_config(NT_DEVCFG_MINIMUM_SLEEP_TIME_FOR_AON)));
#endif /* SUPPORT_TWT_STA */

#ifdef FEATURE_INDEF_DEEP_SLP
    g_socpm_struct.socpm_indef_deep_sleep_en = *((uint32_t *)(nt_devcfg_get_config(NT_DEVCFG_INDEF_DEEP_SLEEP_EN)));
#endif /* FEATURE_INDEF_DEEP_SLP */

#if defined(PLATFORM_FERMION) && !defined(EMULATION_BUILD) && defined(CONFIG_CPR_ENABLE)
    g_socpm_struct.cpr_cfg.ini_enabled = 1;
#endif /* defined(PLATFORM_FERMION) && !defined(EMULATION_BUILD) && CONFIG_CPR_ENABLE */
    g_socpm_struct.slop_step_us = *((uint8_t *)(nt_devcfg_get_config(NT_DEVCFG_SLP_SLOP_STEP_US)));
    g_socpm_struct.slop_interval_ms = *((uint8_t *)(nt_devcfg_get_config(NT_DEVCFG_SLP_SLOP_INTERVAL_MS)));
}

void nt_socpm_init(void)
{
    uint32_t system_status, value;
    system_status = NT_REG_RD(QWLAN_PMU_SYSTEM_STATUS_REG);
    NT_LOG_PRINT(SOCPM, ERR, "PMU SYSTEM STATUS: %x", NT_REG_RD(QWLAN_PMU_SYSTEM_STATUS_REG));
    if ((system_status & QWLAN_PMU_SYSTEM_STATUS_COLD_WARM_BOOT_MASK) &&
        (system_status & QWLAN_PMU_SYSTEM_STATUS_WARM_BOOT_FROM_DEEPSLEEP_MASK)) {
        value = NT_REG_RD(QWLAN_PMU_SLP_TMR_CTL_REG);
        value &= ~(QWLAN_PMU_SLP_TMR_CTL_SLP_TMR_EN_MASK);
        NT_REG_WR(QWLAN_PMU_SLP_TMR_CTL_REG, value);
        NT_REG_WR(NT_NVIC_ICPR1, 0x00800000);
    }
#ifdef FERMION_SILICON

    // getting status from RRAM OTP for disabling UART in APP image
    _socpm_rram_ctl_f = 0;
#else  /* PLATFORM_FERMION */
    // getting status from RRAM OTP for disabling UART in APP image
    _socpm_rram_ctl_f = NT_REG_RD(_SOCPM_OTP_FLAGS_ADDR);
#endif /* PLATFORM_FERMION */
    // clear the wake interrupt at source
    NT_REG_WR(QWLAN_PMU_WLAN_SLP_TMR_CTL_REG, QWLAN_PMU_WLAN_SLP_TMR_CTL_WLAN_SLP_TMR_INT_CLR_MASK);
    NT_REG_WR(QWLAN_PMU_WLAN_SLP_TMR_CTL_REG, 0);

    (void)memset(&_socpm_slp_lst[0], 0, sizeof(_socpm_slp_lst));
    _socpm_slp_lst_head = _INVALID_SLP_LST_HD;
    _socpm_last_slp_count = -1;
    _socpm_slp_exit = 0;

#ifdef _SOCPM_INC_TST_FORCE_SLP_CLK_SRC
    _socpm_slp_clk_src = _SOCPM_INC_TST_FORCE_SLP_CLK_SRC_VAL;
#endif

#if (NT_CHIP_VERSION == 2) || defined(PLATFORM_FERMION)
    uint32_t clr_ext_int = NT_REG_RD(QWLAN_PMU_AON_LIC_INT_CLR_REG);
    clr_ext_int |= 0x1;
    NT_REG_WR(QWLAN_PMU_AON_LIC_INT_CLR_REG, clr_ext_int);

    clr_ext_int = NT_REG_RD(QWLAN_PMU_AON_LIC_INT_CLR_REG);
    clr_ext_int &= ~(0x01);
    NT_REG_WR(QWLAN_PMU_AON_LIC_INT_CLR_REG, clr_ext_int);
#endif  //(NT_CHIP_VERSION==2) || defined(PLATFORM_FERMION)

    _socpm_rrt_init();
    // Initializing the SOCPM_STRUCT
    g_socpm_struct.clk_latency_us = DEFAULT_CLK_LATENCY_US; /* Setting default clk latency to 3ms */
    g_socpm_struct.in_warm_boot = FALSE;
    g_socpm_struct.unapplied_systick_err_us = 0;
    g_socpm_struct.unapplied_err_us = 0;
    g_socpm_struct.aon_program_time_us = 0;
    g_socpm_struct.systick_off_time_us = 0;
#ifdef NT_SOCPM_SW_MTUSR
    g_socpm_struct.mtusr_time_data.aon_programmed = FALSE;
#endif  // NT_SOCPM_SW_MTUS

#ifdef FIRMWARE_APPS_INFORMED_WAKE
    g_socpm_struct.host_supports_a2f = FALSE;
    g_socpm_struct.a2f_asserted = FALSE;
    g_socpm_struct.f2a_asserted = FALSE;
    g_socpm_struct.twt_wake_send_f2a = FALSE;
    g_socpm_struct.f2a_timeout_ms = *((uint32_t *)(nt_devcfg_get_config(NT_DEVCFG_F2A_ASSERT_TIMEOUT_MS)));
    g_socpm_struct.f2a_timer =
        nt_create_timer(wifi_fw_ext_f2a_timeout_cb, NULL, NT_MS_TO_TICKS(g_socpm_struct.f2a_timeout_ms), FALSE);
    g_socpm_struct.f2a_assert_enabled = TRUE;
    g_socpm_struct.inter_f2a_interval_us = *((uint32_t *)(nt_devcfg_get_config(NT_DEVCFG_INTER_F2A_INTERVAL_US)));
    if (g_socpm_struct.inter_f2a_interval_us > MAX_INTER_F2A_INTERVAL_US) {
        g_socpm_struct.f2a_pulse_duration_us = MAX_INTER_F2A_INTERVAL_US;
    }
    // The delay is for the FTDI to detect any F2A pulse, following an A2F assertion.
    g_socpm_struct.a2f_processing_delay = 600;
    g_socpm_struct.f2a_pulse_duration_us = *((uint32_t *)(nt_devcfg_get_config(NT_DEVCFG_F2A_PULSE_DURATION_US)));
    if (g_socpm_struct.f2a_pulse_duration_us > MAX_F2A_PULSE_WIDTH_US) {
        g_socpm_struct.f2a_pulse_duration_us = MAX_F2A_PULSE_WIDTH_US;
    }
#endif /*FIRMWARE_APPS_INFORMED_WAKE*/
#ifdef FEATURE_FPCI
    g_socpm_struct.imps_trigger_indication = false;
#endif /*FEATURE_FCPI*/

#ifdef FEATURE_FERMION_SLP_DBG
    g_socpm_struct.socpm_mcu_sleep_dbg_mode = FALSE;
#endif /* FEATURE_FERMION_SLP_DBG */

#ifdef PLATFORM_FERMION
#ifdef EMULATION_BUILD

    uint32_t val = NT_REG_RD(NT_SOCPM_FPGA_TOP_REG);
    val |= NT_SOCPM_FPGA_TOP_DIVIDE_32K_BY16_MASK;  // bit25 as SLP_CLOCK scaling bit since E2_14
    NT_REG_WR(NT_SOCPM_FPGA_TOP_REG, val);
    val = NT_REG_RD(NT_SOCPM_FPGA_TOP_REG);
    if (val & NT_SOCPM_FPGA_TOP_DIVIDE_32K_BY16_MASK) {
        _slp_clk_scaling = 1;
    } else {
        // On emulation, sleep timer runs at normal speed while rest of the system is scaled down by 16
        // The sleep timer value is scaled to reflect the time slept wrt rest of system
        _slp_clk_scaling = FERMION_EMU_CLK_SCALING;
    }
#endif /* EMULATION_BUILD */
    wifi_fw_coldboot_soc_init();
#endif /* PLATFORM_FERMION */

#if defined(IO_DEBUG)
    g_socpm_struct.io_dbg_count = 0;
#endif /*IO_DEBUG*/

#ifdef FERMION_POWER_WAR
    /** Observed high power consumption post IMPS exit
     * As observed after a warm boot from deep sleep test bus control reg was not updating to default (0x0)
     * hence there was an increase in the CX current
     */
    NT_REG_WR(QWLAN_PMU_TESTBUS_CTL_REG, QWLAN_PMU_TESTBUS_CTL_DEFAULT);
#endif /* FERMION_POWER_WAR */

    socpm_cfg_sleep_paras();
#ifndef FTM_OVER_UART
    nt_socpm_enable(1);
#endif
}

/*
 *  @brief : Initializes PMU temperature sensor and Sleep Clock Cal
 *  @param : none
 *  @return : None
 */
void nt_socpm_secondary_init(void)
{
#ifdef PMU_TS_CONFIGURATION
    pmu_ts_init();
    pmu_ts_configure();
#endif /* PMU_TS_CONFIGURATION */

#ifdef SLEEP_CLK_CAL_IN_ACTIVE_MODE
#ifndef SOCPM_SLEEP_DEBUG
    socpm_slp_clk_cal_enable(ACTIVE_MODE);
#endif
#endif /* SLEEP_CLK_CAL_IN_ACTIVE_MODE */
}

void nt_enable_standby(uint64_t sleep_time)
{
    if (sleep_time < (_socpm_slp_time_supp_min_ms * 1000)) {
        NT_LOG_PRINT(SOCPM, ERR, "Too small sleep time: %u %u, using %u", (uint32_t)(sleep_time >> 32),
                     (uint32_t)sleep_time, _socpm_slp_time_supp_min_ms * 1000);
        sleep_time = (uint64_t)_socpm_slp_time_supp_min_ms * 1000;
    }
#ifdef CONFIG_BOARD_QCC730_QSPI_ENABLE
    drv_flash_deinit(0);
#endif

#ifdef PLATFORM_FERMION
    wifi_fw_pmic_pre_sleep_config(Standby);
#endif /* PLATFORM_FERMION */
#ifdef FEATURE_INDEF_DEEP_SLP
    /* Avoid configuring the sleep timer while indefinite deep sleep is enabled */
    if (!g_socpm_struct.socpm_indef_deep_sleep_en) {
        nt_socpm_slp_tmr_set(sleep_time);
    }
#else
    nt_socpm_slp_tmr_set(sleep_time);
#endif /* FEATURE_INDEF_DEEP_SLP */
    _socpm_slpcfg_sby();
}

uint64_t nt_socpm_min_proc(int *proc_routine)
{
    (void)proc_routine;  // suppress compile warn/err

#if defined(_MIN_TST_INC_FULL_WAKE)
    uint64_t t = 0;
    return t;
#elif defined(_MIN_TST_INC_FAKE_WAKE)
    uint64_t t = 100000;  // 100ms
    return t;
#else
                         //   configASSERT(mode);
#ifdef NT_NEUTRINO_1_0_SYS_MAC
    *proc_routine = 1;
#endif
    uint64_t sleep_back = 0;
    uint64_t slept_time = 0;
    uint64_t result = 0;
    uint64_t delta_time_us = 0;
    uint32_t wkup_delay = 0;

    if (_socpm_slp_lst_head == _INVALID_SLP_LST_HD) {
        UART_Send("MIN_SLP_FN_ERR\r\n", 16);
        return result;
    }

    if (&_socpm_slp_lst[_socpm_slp_lst_head].slp_info != NULL) {
        result = 1;
        slept_time = _socpm_slp_lst[_socpm_slp_lst_head].slp_info.slp_time;
        delta_time_us = nt_socpm_get_slp_tmr_us();
        if (delta_time_us > slept_time) {
            wkup_delay = delta_time_us - slept_time;
        }
        if (_socpm_slp_lst[_socpm_slp_lst_head].slp_info.min_cb_fn) {
            sleep_back = _socpm_slp_lst[_socpm_slp_lst_head].slp_info.min_cb_fn(wkup_delay);
        }

#ifdef SOCPM_SLEEP_DEBUG
        uint32_t d1 = wkup_delay;
        uint32_t d2 = delta_time_us;
        uint32_t d3 = sleep_back;
        socpm_log_timestamp(MIN_PROC_1, d1, d2, d3);
#endif

        if (sleep_back == 0) {
            result = 0;
        } else {
            process_routine = 1;
            int temp = _socpm_slp_lst_head;

            slept_time = _socpm_slp_lst[_socpm_slp_lst_head].slp_info.slp_time;
            _socpm_slp_lst[_socpm_slp_lst_head].slp_info.slp_time =
                _socpm_get_sleep_slop_adjusted_sleep_time(sleep_back);

#if defined(SUPPORT_SLEEP_LIST_IMPROVEMENTS)
            _socpm_slp_lst[_socpm_last_slp_count].slp_info.start_time_us = hres_timer_curr_time_us();
#endif /* SUPPORT_SLEEP_LIST_IMPROVEMENTS */
            if (nt_socpm_sleep_lst_update(slept_time, true)) {
                result = 0;
            } else {
                /*clear the timer interrupt before moving the wifi SS to sleep state.
                if AON interrupt is set the wifi will not move into sleep state*/
                _socpm_slptmr_off();
                if (_socpm_slp_lst[_socpm_slp_lst_head].slp_info.slp_cb_fn)
                    _socpm_slp_lst[_socpm_slp_lst_head].slp_info.slp_cb_fn();
                /** On Fermion hal_wlan_sleep, which is invoked by slp_cb_fn for
                 * MCU sleep, sets the MAC in a state equivalent to light sleep.
                 * The MCU sleep recipe execution in _socpm_slpcfg_mcuslp sets the
                 * MAC(and RRI state in pPmStruct) to MAC_DOWN_MCUSLP, which enables
                 * appropriate RRI restoration.
                 * But in case where a sleep list inversion(clk_gtd_slp coming to
                 * head of the list) occurs after the sleep_lst_reorder below, the
                 * device exits MCU sleep and performs a full wakeup without RRI
                 * restoration.
                 * In such a case, although RRI restoration should be done as needed
                 * for MAC_DOWN_MCUSLP state, the RRI state will indicate
                 * MAC_DOWN_LIGHT. To handle this corner case, RRI state is updated
                 * here, so that the same logic need not be duplicated in all
                 * slp_cb_fn which use MCU sleep.
                 */
                if (mcu_sleep == _socpm_slp_mode) {
                    PM_STRUCT *pPmStruct = (PM_STRUCT *)gdevp->pPmStruct;
                    PM_SET_RRI_STATE(pPmStruct, PM_RRI_MAC_DOWN_MCUSLP);
                }

                // #ifdef NT_NEUTRINO_1_0_SYS_MAC
                nt_socpm_sleep_lst_reorder(_socpm_slp_lst_head, slept_time);
                // #endif
                result = _socpm_slp_lst[_socpm_slp_lst_head].slp_info.slp_time;
                if (result > 0x7ff00000) {
                    (void)temp;
                    //_minprintf("slp", sleep_back >> 32, sleep_back);
                    //_minprintf("slp", temp | (_socpm_slp_lst_head << 16), result);
                    //_minprintf("slp", slept_time >> 32, slept_time);
                }
            }
        }
    }

#ifdef SOCPM_SLEEP_DEBUG
    uint32_t d1 = result;
    uint8_t lst_cnt = get_slp_lst_cnt();
    uint32_t d2 = 0, d3 = 0;
    if (lst_cnt >= 1)
        d2 = (uint32_t)_socpm_slp_lst[_socpm_slp_lst_head].slp_info.slp_cb_fn;
    if (lst_cnt >= 2)
        d3 = (uint32_t)_socpm_slp_lst[_socpm_slp_lst_head].next->slp_info.slp_cb_fn;
    socpm_log_timestamp(MIN_PROC_2, d1, d2, d3);
#endif
    return result;
#endif
}

/*
 *  @brief : if sleep entry failed because of a legitimate reason, handle it
 *  @param[in] : mode - sleep mode being entered
 *  @return : None
 */
void nt_socpm_handle_sleep_entry_failure(sleep_mode mode,bool warm_boot)
{
    (void)mode;

    g_socpm_struct.wifi_ss_state = 0;
    g_socpm_struct.nvic_icpr_status[0] = 0;
    g_socpm_struct.nvic_icpr_status[1] = 0;
    g_socpm_struct.nvic_icpr_status[2] = 0;
    g_socpm_struct.nvic_icpr_status[3] = 0;

#ifdef SUPPORT_STANDBY_MCU_SLEEP_MODE
    if (StandbyMcuSleep == mode) {
#ifdef STANDBY_MCU_SLEEP_ENTRY_FAILURE_HANDLE_WAR
        /*
         * It is observed that when the XIP_CNTL bit is enabled in the system boot complete state RRT
         * on an aborted standby MCU sleep entry which takes place after an entry to and exit from MCU
         * sleep, then the CPU does not appear to receive systick interrupts even if the MCU_SS is not
         * in system boot complete state.
         * On disabling the XIP_CNTL bit, systick interrupts are processed again. So, this is being
         * done as part of sleep entry failure handling.
         */
        HWIO_OUTXF(SEQ_WCSS_PMU_OFFSET,
                   NEUTRINO_PMU_PRONTO_LP_FRODO_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ,
                   PD_XIP_CNTL_BIT, 0x0);
#endif /* STANDBY_MCU_SLEEP_ENTRY_FAILURE_HANDLE_WAR */

        /* Re-enable CMEM bank A retention on aborted standby MCU sleep entry */
        HWIO_OUTX2F(SEQ_WCSS_PMU_OFFSET, NEUTRINO_PMU_PRONTO_LP_FRODO_PMU_CFG_CMEM_BANK_A_RET_EN,
                    CFG_CMEM_BANK_A_31_22_RET_EN, CFG_CMEM_BANK_A_21_0_RET_EN, 0x1, 0x1);
    }
#endif /* SUPPORT_STANDBY_MCU_SLEEP_MODE */

#ifdef IMPS_DEEP_SLEEP_FAIL_HANDLE
    /* once after IMPS indefinite deep sleep failure, device should enter into deep sleep
     * for shorter duration. Hence, disabled indefinite deep sleep after sleep failure */
    if (g_socpm_struct.socpm_indef_deep_sleep_en && (g_socpm_struct.deep_sleep_failed == TRUE)) {
        uint32_t reg_val = NT_REG_RD(QWLAN_PMU_AON_TOP_CFG_REG);
        reg_val &= ~QWLAN_PMU_AON_TOP_CFG_CFG_INDEFINITE_DEEPSLEEP_EN_MASK;
        reg_val &= ~QWLAN_PMU_AON_TOP_CFG_CFG_ASSERT_CLK_REQ_DURING_SLEEP_MASK;
        NT_REG_WR(QWLAN_PMU_AON_TOP_CFG_REG, reg_val);
        _socpm_slpcfg_sby();
        return;
    }
#endif /* IMPS_DEEP_SLEEP_FAIL_HANDLE */

#if defined(PLATFORM_FERMION) && !defined(EMULATION_BUILD)
    /* Re-enable CPR on failed sleep entry */
    wifi_fw_cpr_reenable();
#endif /* defined(PLATFORM_FERMION) && !defined(EMULATION_BUILD) */

#ifdef SUPPORT_WMAC_HWDTIM
    if (nt_wpm_is_hwdtim_mode_enabled()) {
        g_ppm_common_struct.hwdtim_configured = FALSE;
        /* disable HDM for sleep failure as it might put MAC back to sleep */
        hal_wmac_disable_hwdtim();
    }
#endif /* SUPPORT_WMAC_HWDTIM */

    /* Switch MCU_SS to active state as prevention of sleep entry would have
     * moved it to SYSTEM_BOOT_COMPLETE state.
     */
    nt_socpm_switch_mcuss_to_active();
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
    uart_init();

    HAL_REG_WR(QWLAN_PMU_CFG_WIFI_SS_STATE_REG, NT_PMU_CFG_WIFI_CONFIG_OFFSET);  // Set wifi config state

    // nt_hal_rri_soft_reset_rri_engine();
    printf("sleep failed\r\n");
    if (gdevp) {
        // nt_pm_enforce_rri_readiness(gdevp);
        PM_STRUCT *pPmStruct = (PM_STRUCT *)gdevp->pPmStruct;
        {
            if(!warm_boot)
            {
                nt_hal_rri_soft_reset_rri_engine();
                nt_hal_rri_restore_first();
                PM_SET_RRI_STATE(pPmStruct, PM_RRI_RX_READY);

                #ifdef NT_SOCPM_SW_MTUSR
                        nt_socpm_mtusr_restore_mtu_time();
                #endif  // NT_SOCPM_SW_MTUSR

                PM_SET_WLAN_STATE_ON(pPmStruct);

                {
                    nt_hal_rri_restore_second();
                    PM_SET_RRI_STATE(pPmStruct, PM_RRI_TXRX_READY);
                }
                rri_force_wakeup = 1;
            }

        }
      
    }

#ifdef SLEEP_CLK_SWITCH_AND_CAL_2_0
    nt_socpm_sleep_clk_switch_to_xo(TRUE);
#endif /* SLEEP_CLK_SWITCH_AND_CAL_2_0 */

    /* we disable qtimer interrupt after the sleep recipes and before WFI. but enable
     * it back when soc has failed to enter into sleep.
     */
    nt_enable_device_irq(Qtmr_qgic2_phy_irq_0);

    NT_REG_WR(CACHE_REG_BASE, 0x01);
    NT_REG_WR(CACHE_REG_BASE, 0x00);

    portENABLE_INTERRUPTS(); /* Sets the BASEPRI to 0x00*/
    portNVIC_SYSPRI2_REG |= portNVIC_PENDSV_PRI;
    portNVIC_SYSPRI2_REG |= portNVIC_SYSTICK_PRI;

    /*enable all previously enabled interrupts*/
    NT_REG_WR(NT_CM4_NVIC_ISER0_REG, nt_socpm_m4_regs[11]);
    NT_REG_WR(NT_CM4_NVIC_ISER1_REG, nt_socpm_m4_regs[12]);
    NT_REG_WR(NT_CM4_NVIC_ISER2_REG, nt_socpm_m4_regs[13]);
#ifdef PLATFORM_FERMION
    NT_REG_WR(NT_CM4_NVIC_ISER3_REG, nt_socpm_m4_regs[14]);
#endif /* PLATFORM_FERMION */

    // Initialize QCSPI on full wakeup
#ifdef SUPPORT_QCSPI_SLAVE
    qcspi_slv_init();
#endif /* SUPPORT_QCSPI_SLAVE */

#ifdef LOW_POWER_MEMORY
    /*To reset the state machine of low power memory framework*/
    low_power_memory_reinit_sections();
#endif  // LOW_POWER_MEMORY
#if defined(SUPPORT_HIGH_RES_TIMER)
    hres_timer_post_sleep();
#endif /* SUPPORT_HIGH_RES_TIMER */

#if defined(PMU_TS_CONFIGURATION) && defined(SUPPORT_STANDBY_MCU_SLEEP_MODE)
    /* Restart periodic temperature measurements in case of a failure to enter sleep */
    if (StandbyMcuSleep == mode) {
        pmu_ts_configure_periodic_meas();
    }
#endif /* defined(PMU_TS_CONFIGURATION) && defined(SUPPORT_STANDBY_MCU_SLEEP_MODE) */
}

/*
 *  @brief : Check if there was an unexpected failure in entering to sleep after wfi
 *  @param :
 *      mode - sleep mode being entered
 *      is_ctxt_rstr_point - whether called after context save, at the point where context restore would resume
 * execution
 *  @return : None
 */
void nt_socpm_check_sleep_entry_failure(sleep_mode mode, bool is_ctxt_rstr_point,bool warm_boot)
{
    if ((mode == clk_gtd_sleep) || (is_ctxt_rstr_point && (_socpm_mcu_sleep_wake == 1))) {
        return;
    }
    bool sleep_failed = false;

    /*complete all memory operations before storing the ICPR values
  this is to make sure the icpr values are properly updated before the assert checks are made*/
    __asm volatile("dsb" ::: "memory");
    __asm volatile("isb");

    /* Entry to sleep on wfi failed. Check reason for failure */
    g_socpm_struct.wifi_ss_state =
        ((NT_REG_RD(QWLAN_PMU_CFG_WIFI_SS_STATE_REG) & QWLAN_PMU_CFG_WIFI_SS_STATE_WIFI_SS_CURR_STATE_MASK) >>
         QWLAN_PMU_CFG_WIFI_SS_STATE_WIFI_SS_CURR_STATE_OFFSET);
    g_socpm_struct.nvic_icpr_status[0] = NT_REG_RD(NT_CM4_NVIC_ISER0_CLEAR_PENDING_REG);
    g_socpm_struct.nvic_icpr_status[1] = NT_REG_RD(NT_CM4_NVIC_ISER1_CLEAR_PENDING_REG);
    g_socpm_struct.nvic_icpr_status[2] = NT_REG_RD(NT_CM4__NVIC_ISER2_CLEAR_PENDING_REG);
    g_socpm_struct.nvic_icpr_status[3] = NT_REG_RD(NT_CM4_NVIC_ISER3_CLEAR_PENDING_REG);

    switch (mode) {
        case mcu_sleep:
#ifdef PLATFORM_FERMION
        case Lightsleep:
#endif /* PLATFORM_FERMION */
            if (g_socpm_struct.wifi_ss_state != NT_PMU_CFG_WIFI_SLEEP_OFFSET) {
                /*Sleep entry failed since WIFI_SS is not in SLEEP state*/
                sleep_failed = FALSE;
                NT_LOG_PRINT(SOCPM, ERR, "Sleep entry failed due to WIFI_SS non sleep %d %d %d", mode,
                             is_ctxt_rstr_point, g_socpm_struct.wifi_ss_state);
            } else if ((g_socpm_struct.nvic_icpr_status[1] & AON_TIMER_INTR_NVIC1_MASK)
#ifdef PLATFORM_FERMION
                       || (g_socpm_struct.nvic_icpr_status[1] & A2F_ASSERT_INTR_NVIC1_MASK) ||
                       (g_socpm_struct.nvic_icpr_status[0] & NT_CM4_UART_INTERRUPT_BIT_MASK)
#endif /* PLATFORM_FERMION */
            ) {
                /*Sleep entry failed due to AON timer interrupt or A2F pending*/
                sleep_failed = FALSE;
            }
            break;
        case Standby:
#ifdef PLATFORM_FERMION
        case InfDeepsleep:
#endif /* PLATFORM_FERMION */
            if (0
#ifdef PLATFORM_FERMION
                || (g_socpm_struct.nvic_icpr_status[1] & A2F_ASSERT_INTR_NVIC1_MASK)
#endif /* PLATFORM_FERMION */
            ) {
                /*Sleep entry failed due to A2F pending*/
                sleep_failed = FALSE;
            }
            break;
        default:
            sleep_failed = FALSE;
            break;
    }

    if (sleep_failed) {
        // Stop AON timer increment
        uint32_t value = NT_REG_RD(QWLAN_PMU_SLP_TMR_CTL_REG);
        value &= ~QWLAN_PMU_SLP_TMR_CTL_SLP_TMR_EN_MASK;
        NT_REG_WR(QWLAN_PMU_SLP_TMR_CTL_REG, value);

        NT_LOG_PRINT(SOCPM, ERR, "SLEEP_ENTER_FAIL_ASSERT post wfi %d %d %d", mode, is_ctxt_rstr_point,
                     g_socpm_struct.wifi_ss_state);
        NT_LOG_PRINT(SOCPM, CRIT, "NVIC ICPR[0-3]: %x %x %x %x", g_socpm_struct.nvic_icpr_status[0],
                     g_socpm_struct.nvic_icpr_status[1], g_socpm_struct.nvic_icpr_status[2],
                     g_socpm_struct.nvic_icpr_status[3]);
        configASSERT(0);
    } else {
        g_sleep_failed = TRUE;
        NT_LOG_PRINT(SOCPM, ERR, "Handle SLEEP_ENTER_FAIL post wfi %d %d %d", mode, is_ctxt_rstr_point,
                     g_socpm_struct.wifi_ss_state);
        NT_LOG_PRINT(SOCPM, ERR, "NVIC ICPR[0-3]: 0x%08x 0x%08x 0x%08x 0x%08x", g_socpm_struct.nvic_icpr_status[0],
                     g_socpm_struct.nvic_icpr_status[1], g_socpm_struct.nvic_icpr_status[2],
                     g_socpm_struct.nvic_icpr_status[3]);
        NT_REG_WR(QWLAN_PMU_SLP_CNTL_REG, 0);
        // while(1);
        nt_socpm_handle_sleep_entry_failure(mode,warm_boot);
    }
}

#ifdef NT_SOCPM_SW_MTUSR
static void nt_socpm_get_mtusr_timestamp(nt_mtusr_timestamp_t *ts)
{
/* On NT platform, QTMR is not saved across sleep cycles and also cannot be restored.
 * As a result, it restarts from 0 on wake from sleep and new timestamps cannot be compared to old ones.
 * But on Fermion platform, HW saves and restores QTMR timestamps, which can be used to compare across sleep cycles.
 */
#ifdef PLATFORM_NT
    // Get QTMR timerstamp during normal operation and SYSTCK timestamp in warm boot
    if (g_socpm_struct.in_warm_boot) {
        ts->time = (uint64_t)GET_CURRENT_SYSTICK_US();
        ts->type = MTUSR_TS_SYSTCK;
    } else
#endif /* PLATFORM_NT */
    {
        ts->time = hres_timer_curr_time_us();
        ts->type = MTUSR_TS_QTMR;
    }
}

static bool nt_socpm_get_mtusr_timestamp_delta(const nt_mtusr_timestamp_t *first_ts,
                                               const nt_mtusr_timestamp_t *second_ts, uint64_t *delta_time)
{
    // If second_ts is later than first_ts, result is TRUE; else FALSE
    *delta_time = 0;
    bool result = TRUE;

    if (first_ts->type == MTUSR_TS_SYSTCK) {
        // In case of SYSTCK, if second_ts was taken later, then second_ts < first_ts
        if (first_ts->time > second_ts->time) {
            *delta_time = first_ts->time - second_ts->time;
            result = TRUE;
        } else {
            *delta_time = second_ts->time - first_ts->time;
            result = FALSE;
        }
    } else if (first_ts->type == MTUSR_TS_QTMR) {
        // In case of QTMR, if second_ts was taken later, then second_ts > first_ts
        if (first_ts->time > second_ts->time) {
            *delta_time = first_ts->time - second_ts->time;
            result = FALSE;
        } else {
            *delta_time = second_ts->time - first_ts->time;
            result = TRUE;
        }
    } else {
        *delta_time = 0;
    }

    return result;
}

void nt_socpm_mtusr_restore_mtu_time(void)
{
#ifdef SOCPM_SLEEP_DEBUG
    socpm_log_timestamp(__MTU_RSTR, NT_REG_RD(QWLAN_PMU_POWER_DOMAIN_STATUS_REG),
                        (uint32_t)g_socpm_struct.mtusr_time_data.mtu_timestamp.time, 0);
#endif
    if (((gdevp) && (gdevp->pPmStruct)) &&
        (((PM_STRUCT *)gdevp->pPmStruct)->wlan_state_off && g_socpm_struct.mtusr_time_data.aon_programmed ||
         PM_GET_RRI_STATE((PM_STRUCT *)gdevp->pPmStruct) == PM_RRI_RX_READY) &&
        (g_socpm_struct.mtusr_time_data.aon_timestamp.type == g_socpm_struct.mtusr_time_data.mtu_timestamp.type)) {
        uint64_t delta_time;
        bool add_delta;
        add_delta = nt_socpm_get_mtusr_timestamp_delta(&(g_socpm_struct.mtusr_time_data.mtu_timestamp),
                                                       &(g_socpm_struct.mtusr_time_data.aon_timestamp), &delta_time);

        uint64_t tsf = g_socpm_struct.mtusr_time_data.mtu_tsf_us;
        uint32_t mtu_tmr = g_socpm_struct.mtusr_time_data.mtu_glob_tmr;
        if (add_delta) {
            tsf += delta_time;
            mtu_tmr += (uint32_t)delta_time;
        } else {
            tsf -= delta_time;
            mtu_tmr -= (uint32_t)delta_time;
        }

        uint64_t aon_slp_tmr_us = nt_socpm_get_slp_tmr_us();
        tsf += aon_slp_tmr_us;
        mtu_tmr += aon_slp_tmr_us;

#ifdef SOCPM_SLEEP_DEBUG
        socpm_log_timestamp(__MTU_RSTR, (uint32_t)aon_slp_tmr_us, g_socpm_struct.aon_program_time_us,
                            (uint32_t)delta_time);
#endif

        NT_REG_WR(QWLAN_MTU_MTU_GLOBAL_TIMER_REG, mtu_tmr);
        NT_REG_WR(QWLAN_MTU_TSF_TIMER_HI_REG, (uint32_t)(tsf >> 32));
        NT_REG_WR(QWLAN_MTU_TSF_TIMER_LO_REG, (uint32_t)tsf);
        // TBTT need not be adjusted by SW during restoration as HW will adjust it.
        //  If the TBTT is less than TSF, HW increments it by sw_mtu_beacon_intv every usec,
        //  until it is higher than TSF.
        NT_REG_WR(QWLAN_MTU_TBTT_H_REG, (uint32_t)(g_socpm_struct.mtusr_time_data.mtu_tbtt >> 32));
        NT_REG_WR(QWLAN_MTU_TBTT_L_REG, (uint32_t)g_socpm_struct.mtusr_time_data.mtu_tbtt);
        // This register contains sw_mtu_beacon_intv, which is used by HW to forward TBTT
        NT_REG_WR(QWLAN_MTU_BCN_BSSID_INTV_REG, g_socpm_struct.mtusr_time_data.mtu_bcn_bssid_intv);
    } else {
        _minprintf("NO_MTUSR_RSTR", 0, 0);
    }
    g_socpm_struct.mtusr_time_data.aon_programmed = FALSE;
}

void nt_socpm_mtusr_save_mtu_time(void)
{
    // Save key MTU timer registers
    if (((gdevp) && (gdevp->pPmStruct)) && (!((PM_STRUCT *)gdevp->pPmStruct)->wlan_state_off)) {
        g_socpm_struct.mtusr_time_data.mtu_glob_tmr = NT_REG_RD(QWLAN_MTU_MTU_GLOBAL_TIMER_REG);
        g_socpm_struct.mtusr_time_data.mtu_tsf_us =
            NT_REG_RD(QWLAN_MTU_TSF_TIMER_LO_REG) | (((uint64_t)NT_REG_RD(QWLAN_MTU_TSF_TIMER_HI_REG)) << 32);
        g_socpm_struct.mtusr_time_data.mtu_tbtt =
            NT_REG_RD(QWLAN_MTU_TBTT_L_REG) | (((uint64_t)NT_REG_RD(QWLAN_MTU_TBTT_H_REG)) << 32);
        g_socpm_struct.mtusr_time_data.mtu_bcn_bssid_intv = NT_REG_RD(QWLAN_MTU_BCN_BSSID_INTV_REG);
        nt_socpm_get_mtusr_timestamp(&(g_socpm_struct.mtusr_time_data.mtu_timestamp));
#ifdef SOCPM_SLEEP_DEBUG
        socpm_log_timestamp(____MTU_SV, (uint32_t)g_socpm_struct.mtusr_time_data.mtu_timestamp.type,
                            (uint32_t)g_socpm_struct.mtusr_time_data.mtu_timestamp.time, 0);
#endif
    }
}

void nt_socpm_mtusr_save_aon_prog_timestamp(void)
{
    g_socpm_struct.mtusr_time_data.aon_programmed = TRUE;
    nt_socpm_get_mtusr_timestamp(&(g_socpm_struct.mtusr_time_data.aon_timestamp));
#ifdef SOCPM_SLEEP_DEBUG
    if (_socpm_slp_mode == mcu_sleep) {
        socpm_log_timestamp(MTU_AON_SV, (uint32_t)g_socpm_struct.mtusr_time_data.aon_timestamp.type,
                            (uint32_t)g_socpm_struct.mtusr_time_data.aon_timestamp.time, 0);
    }
#endif
}

#ifdef SUPPORT_BMU_ERROR_RECOVERY
/*
 *  @brief : Minimal version to save MTU time data before WiFi sleep for BMU recovery sequence
 *  @param : None
 *  @return : None
 */
void __attribute__((section(".__sect_ps_txt"))) nt_socpm_mtusr_save_mtu_time_on_bmu_recovery(void)
{
    // Save key MTU timer registers
    g_socpm_struct.bmu_recovery_mtusr_time_data.mtu_glob_tmr = NT_REG_RD(QWLAN_MTU_MTU_GLOBAL_TIMER_REG);
    g_socpm_struct.bmu_recovery_mtusr_time_data.mtu_tsf_us =
        NT_REG_RD(QWLAN_MTU_TSF_TIMER_LO_REG) | (((uint64_t)NT_REG_RD(QWLAN_MTU_TSF_TIMER_HI_REG)) << 32);
#ifdef SUPPORT_TWO_STA_CONC
    g_socpm_struct.bmu_recovery_mtusr_time_data.mtu_tsf2_us =
        NT_REG_RD(QWLAN_MTU_BSS2_CLIENT_TSF_TIMER_LO_REG) |
        (((uint64_t)NT_REG_RD(QWLAN_MTU_BSS2_CLIENT_TSF_TIMER_HI_REG)) << 32);
#endif /* SUPPORT_TWO_STA_CONC */
    g_socpm_struct.bmu_recovery_mtusr_time_data.mtu_tbtt =
        NT_REG_RD(QWLAN_MTU_TBTT_L_REG) | (((uint64_t)NT_REG_RD(QWLAN_MTU_TBTT_H_REG)) << 32);
    g_socpm_struct.bmu_recovery_mtusr_time_data.mtu_bcn_bssid_intv = NT_REG_RD(QWLAN_MTU_BCN_BSSID_INTV_REG);
    nt_socpm_get_mtusr_timestamp(&(g_socpm_struct.bmu_recovery_mtusr_time_data.mtu_timestamp));
}

/*
 *  @brief : Minimal version of MTU time restoration for BMU recovery sequence
 *  @param : None
 *  @return : None
 */
void __attribute__((section(".__sect_ps_txt"))) nt_socpm_mtusr_restore_mtu_time_on_bmu_recovery(void)
{
    nt_mtusr_timestamp_t restore_timestamp;
    uint64_t delta_time;
    bool add_delta;

    /* The usual MTUSR logic used in case of powersave involves taking timestamps at the time
     * of AON timer programming and MTU data save. The time difference in between these two
     * timestamps is used in conjunction with the AON timer value at the time of restoration
     * to determine the time to be restored. This helps to overcome the inherent inaccuracy
     * in QTimer correction with AON timer ticks across sleeps.
     * In contrast, the BMU recovery sequence does not require any AON timer programming/real
     * SOC sleep. So, the delta between the save timestamp and timestamp during restore is used
     * directly to determine the time that is to be restored.
     */
    nt_socpm_get_mtusr_timestamp(&restore_timestamp);

    add_delta = nt_socpm_get_mtusr_timestamp_delta(&(g_socpm_struct.bmu_recovery_mtusr_time_data.mtu_timestamp),
                                                   &restore_timestamp, &delta_time);

    uint64_t tsf = g_socpm_struct.bmu_recovery_mtusr_time_data.mtu_tsf_us;
#ifdef SUPPORT_TWO_STA_CONC
    uint64_t tsf2 = g_socpm_struct.bmu_recovery_mtusr_time_data.mtu_tsf2_us;
#endif /* SUPPORT_TWO_STA_CONC */
    uint32_t mtu_tmr = g_socpm_struct.bmu_recovery_mtusr_time_data.mtu_glob_tmr;
    if (add_delta) {
        tsf += delta_time;
#ifdef SUPPORT_TWO_STA_CONC
        tsf2 += delta_time;
#endif /* SUPPORT_TWO_STA_CONC */
        mtu_tmr += (uint32_t)delta_time;
    } else {
        tsf -= delta_time;
#ifdef SUPPORT_TWO_STA_CONC
        tsf2 -= delta_time;
#endif /* SUPPORT_TWO_STA_CONC */
        mtu_tmr -= (uint32_t)delta_time;
    }

#ifdef WLAN_SLEEP_WITH_SYNTH_POWER_OFF
    uint32_t pre_update_mtu_tstamp = NT_REG_RD(QWLAN_MTU_MTU_GLOBAL_TIMER_REG);
#endif /* WLAN_SLEEP_WITH_SYNTH_POWER_OFF */
    NT_REG_WR(QWLAN_MTU_MTU_GLOBAL_TIMER_REG, mtu_tmr);
    NT_REG_WR(QWLAN_MTU_TSF_TIMER_HI_REG, (uint32_t)(tsf >> 32));
    NT_REG_WR(QWLAN_MTU_TSF_TIMER_LO_REG, (uint32_t)tsf);
#ifdef SUPPORT_TWO_STA_CONC
    NT_REG_WR(QWLAN_MTU_BSS2_CLIENT_TSF_TIMER_HI_REG, (uint32_t)(tsf2 >> 32));
    NT_REG_WR(QWLAN_MTU_BSS2_CLIENT_TSF_TIMER_LO_REG, (uint32_t)tsf2);
#endif /* SUPPORT_TWO_STA_CONC */
    // TBTT need not be adjusted by SW during restoration as HW will adjust it.
    //  If the TBTT is less than TSF, HW increments it by sw_mtu_beacon_intv every usec,
    //  until it is higher than TSF.
    NT_REG_WR(QWLAN_MTU_TBTT_H_REG, (uint32_t)(g_socpm_struct.mtusr_time_data.mtu_tbtt >> 32));
    NT_REG_WR(QWLAN_MTU_TBTT_L_REG, (uint32_t)g_socpm_struct.mtusr_time_data.mtu_tbtt);
    // This register contains sw_mtu_beacon_intv, which is used by HW to forward TBTT
    NT_REG_WR(QWLAN_MTU_BCN_BSSID_INTV_REG, g_socpm_struct.mtusr_time_data.mtu_bcn_bssid_intv);
#ifdef WLAN_SLEEP_WITH_SYNTH_POWER_OFF
    nt_hal_update_rri_mtu_timestamps(pre_update_mtu_tstamp, mtu_tmr);
#endif /* WLAN_SLEEP_WITH_SYNTH_POWER_OFF */
}
#endif /* SUPPORT_BMU_ERROR_RECOVERY */

#endif  // NT_SOCPM_SW_MTUSR

#ifdef NT_NEUTRINO_1_0_SYS_MAC

void nlp_config(void) {}

void nt_socpm_tst_standby(uint32_t slp_ms)
{
    (void)slp_ms;
#if _SOCPM_TST_MODE_PMIC_CFG_SBY != 0
    _socpm_slpcfg_sby();
    _tst_sleep_enter();
#endif
}

#endif

void nt_socpm_footsw_state_set(uint8_t st)
{
    if (st == 1) {
#ifdef NT_CC_DEBUG_FLAG
        //          //set resistance 7 ohm
        uint32_t val = 2;
        uint32_t regval = 0;
        regval = NT_REG_RD(NT_PMU_PMIC_CFG_FOOTSW_REG);
        regval &= ~(NT_SOCPM_FOOT_SWITCH_CHARGE_RATE_SET_MASK_DEFAULT);
        val &= NT_SOCPM_FOOT_SWITCH_CHARGE_RATE_SET_MASK_DEFAULT;  // restricting in 6 bit
        regval |= val;
        NT_REG_WR(NT_PMU_PMIC_CFG_FOOTSW_REG, regval);
        //          enableing foot sw
        regval |= (NT_PMU_PMIC_CFG_FOOTSW_EN_MASK << NT_PMU_PMIC_CFG_FOOTSW_EN_OFFSET);
        NT_REG_WR(NT_PMU_PMIC_CFG_FOOTSW_REG, regval);

        // 100 ms delay( 1 nop is 1 clk_cyc delay = 16.6 nano seconds, 100 ms delay is equivalent
        // to 6024096 clock cyc delay
        for (uint32_t i = 0; i < 6024097; i++) {
            asm volatile("nop");
        }
        // set resistance less than 7 ohm
        regval = NT_REG_RD(NT_PMU_PMIC_CFG_FOOTSW_REG);
        regval &= ~(NT_SOCPM_FOOT_SWITCH_CHARGE_RATE_SET_MASK_DEFAULT);
        NT_REG_WR(NT_PMU_PMIC_CFG_FOOTSW_REG, regval);

#endif
    } else {
#ifdef NT_CC_DEBUG_FLAG
        //          disabling foot sw
        uint32_t regval = 0;
        regval = NT_REG_RD(NT_PMU_PMIC_CFG_FOOTSW_REG);
        regval &= ~(NT_PMU_PMIC_CFG_FOOTSW_EN_MASK << NT_PMU_PMIC_CFG_FOOTSW_EN_OFFSET);
        regval &= ~(NT_SOCPM_FOOT_SWITCH_CHARGE_RATE_SET_MASK_DEFAULT);
//          NT_REG_WR (NT_PMU_PMIC_CFG_FOOTSW_REG , 0x00);
#endif
    }
}

/*
 * @brief: This function is used to fetch the status of uart enable/disable flag to the aller
 * @param : none
 * @return : boolean (true(1))/(false(0))
 */
_Bool __attribute__((optimize("00"))) nt_socpm_uart_flag_state_get(nt_otp_firmware_reserved pos)
{
    if (NT_CHECK_BIT_STATE(_socpm_rram_ctl_f, pos)) {
        pos = 1;
    }
    _Bool ret_state = (pos == 1);
    return ret_state;
}

/*
 * @brief: This function is used to fetch the status of auto_start enable/disable flag to the aller
 * @param : none
 * @return : boolean (true(1))/(false(0))
 */
_Bool __attribute__((optimize("00"))) nt_socpm_auto_start_flag_state_get(nt_otp_firmware_reserved pos)
{
    if (NT_CHECK_BIT_STATE(_socpm_rram_ctl_f, pos)) {
        pos = 1;
    }
    _Bool ret_state = (pos == 1);
    return ret_state;
}

/*
 * @brief: This function is used to fetch the status of cpr enable/disable
 * @param : none
 * @return : boolean (true(1))/(false(0))
 */
_Bool __attribute__((optimize("00"))) nt_socpm_cpr_flag_state_get(nt_otp_firmware_reserved pos)
{
    if (NT_CHECK_BIT_STATE(_socpm_rram_ctl_f, pos)) {
        pos = 1;
    }
    _Bool ret_state = (pos == 1);
    return ret_state;
}
void nt_cpr_enable()
{
    NT_REG_WR(_SOCPM_OTP_FLAGS_ADDR, 0x10);
}

void nt_cpr_disable()
{
    NT_REG_WR(_SOCPM_OTP_FLAGS_ADDR, 0x0);
}

/*
 * @brief: This function used to turn ON/OFF the power domains as per need for BMPS
 * partial sleep
 * @param void
 * @return none
 */
void vPreSleepProcessingPartialSleep()
{
    static uint16_t ncount = 0;
    uint32_t value;

    PM_STRUCT *pPmStruct = (PM_STRUCT *)gdevp->pPmStruct;
    pPmStruct->slp_clk_sel = _socpm_slp_clk_src;

    //_minprintf("L", _socpm_slp_clk_src, ncount);

#ifdef _SOCPM_INC_TST_MCUSLP_SBY
    _socpm_slpcfg_sby();
#endif

    if (FALSE == g_socpm_struct.in_warm_boot) {
        value = NT_REG_RD(NT_CM4_NVIC_ISER0_REG);
        nt_socpm_m4_regs[11] = value;

        value = NT_REG_RD(NT_CM4_NVIC_ISER1_REG);
        nt_socpm_m4_regs[12] = value;

        value = NT_REG_RD(NT_CM4_NVIC_ISER2_REG);
        nt_socpm_m4_regs[13] = value;

#ifdef FIRMWARE_APPS_INFORMED_WAKE
        value = NT_REG_RD(NT_CM4_NVIC_ISER3_REG);
        nt_socpm_m4_regs[14] = (value & A2F_DEASSERT_INTR_NVIC3_MASK);
#endif /* FIRMWARE_APPS_INFORMED_WAKE */
    }

    uint8_t wr_all_f = 1;  // skip/opt some writes out

#ifdef _SOCPM_INC_MCUSLP_WREG_COUNT
    ncount++;
#endif

    if (ncount > _SOCPM_REGWR_MAX) {
        wr_all_f = 0;
        if (ncount > _SOCPM_REGWR_RESET_LIMIT)
            ncount = 0;
    }

    NT_REG_WR(QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_REG, 0XF7DF);

#ifdef _SOCPM_INC_TST_SLP_PHY_RXP_RST
    // reset phy and rx - this might not be necessary
    NT_REG_WR(QWLAN_AGC_AGC_RESET_REG, QWLAN_AGC_AGC_RESET_RESET_ERESET);
    NT_REG_WR(QWLAN_RXP_CONFIG_REG, QWLAN_RXP_CONFIG_CFG_RXP_EN_DEFAULT);
#endif

    if (wr_all_f) {
#ifdef NT_FN_PDC_
        value = NT_REG_RD(QWLAN_PMU_DIG_TOP_CFG_REG);
        NT_REG_WR(QWLAN_PMU_DIG_TOP_CFG_REG, QWLAN_PMU_DIG_TOP_CFG_FORCE_BANK_A_CORE_ON_MASK |
                                                 QWLAN_PMU_DIG_TOP_CFG_FORCE_BANK_B_CORE_ON_MASK |
                                                 QWLAN_PMU_DIG_TOP_CFG_FORCE_BANK_C_CORE_ON_MASK |
                                                 QWLAN_PMU_DIG_TOP_CFG_FORCE_BANK_D_CORE_ON_MASK | value);
#else
#if defined(PLATFORM_FERMION)
        value = (QWLAN_PMU_DIG_TOP_CFG_FORCE_BANK_B_CORE_ON_MASK | QWLAN_PMU_DIG_TOP_CFG_FORCE_BANK_C_CORE_ON_MASK |
                 QWLAN_PMU_DIG_TOP_CFG_FORCE_BANK_D_CORE_ON_MASK | QWLAN_PMU_DIG_TOP_CFG_FORCE_BANK_E_CORE_ON_MASK);
        value |= QWLAN_PMU_DIG_TOP_CFG_RRAM_PD_MODE_DEFAULT;
        NT_REG_WR(QWLAN_PMU_DIG_TOP_CFG_REG, value);  // Membank retention and RRAM PD off
#else
#if defined(PLATFORM_NT) && defined(RRAM_PD_WAR)
        NT_REG_WR(QWLAN_PMU_DIG_TOP_CFG_REG, (0x80F));   // WMAC and Membank retention. RRAM PD mode
#else
        NT_REG_WR(QWLAN_PMU_DIG_TOP_CFG_REG, (0x100F));  // WMAC and Membank retention.
#endif
#endif  /*PLATFORM_FERMION*/
#endif  // NT_FN_PDC_
    }

    if (_socpm_slp_clk_src == NT_SOCPM_SLP_CLK_RFAXO) {
// TODO: Temp keep RFA DTOP ON in addition to PMIC and XO
// Note: RFA DTOP should be turned off - not needed for RFA/XO ops
#ifdef NT_FN_PDC_
        value = NT_REG_RD(QWLAN_PMU_CFG_AON_CNTL_MCU_SLEEP_STATE_RESOURCE_REQ_REG);
        NT_REG_WR(QWLAN_PMU_CFG_AON_CNTL_MCU_SLEEP_STATE_RESOURCE_REQ_REG,
                  (0b10 << QWLAN_PMU_CFG_AON_CNTL_MCU_SLEEP_STATE_RESOURCE_REQ_PD_MEM_MX_CNTL_BIT_OFFSET) |
                      QWLAN_PMU_CFG_AON_CNTL_MCU_SLEEP_STATE_RESOURCE_REQ_PD_RFA_DTOP_CNTL_BIT_MASK |
                      QWLAN_PMU_CFG_AON_CNTL_MCU_SLEEP_STATE_RESOURCE_REQ_PD_XO_DTOP_CNTL_BIT_MASK |
                      QWLAN_PMU_CFG_AON_CNTL_MCU_SLEEP_STATE_RESOURCE_REQ_PD_PMIC_DTOP_CNTL_BIT_MASK);
#endif      // NT_FN_PDC_
    } else  // PMIC XO or RC
    {
        if (wr_all_f) {
#ifdef NT_FN_PDC_
            value = NT_REG_RD(QWLAN_PMU_CFG_AON_CNTL_MCU_SLEEP_STATE_RESOURCE_REQ_REG);
            NT_REG_WR(QWLAN_PMU_CFG_AON_CNTL_MCU_SLEEP_STATE_RESOURCE_REQ_REG,
                      (0b10 << QWLAN_PMU_CFG_AON_CNTL_MCU_SLEEP_STATE_RESOURCE_REQ_PD_MEM_MX_CNTL_BIT_OFFSET));
#endif
        }
    }

    if (wr_all_f) {
#ifdef NT_FN_PDC_
        value = NT_REG_RD(QWLAN_PMU_CFG_MCU_SLEEP_STATE_RESOURCE_REQ_REG);
        NT_REG_WR(QWLAN_PMU_CFG_MCU_SLEEP_STATE_RESOURCE_REQ_REG,
                  (value | QWLAN_PMU_CFG_MCU_SLEEP_STATE_RESOURCE_REQ_PD_CMEM_BANK_A_CNTL_BIT_MASK |
                   QWLAN_PMU_CFG_MCU_SLEEP_STATE_RESOURCE_REQ_PD_CMEM_BANK_B_CNTL_BIT_MASK |
                   QWLAN_PMU_CFG_MCU_SLEEP_STATE_RESOURCE_REQ_PD_CMEM_BANK_C_CNTL_BIT_MASK |
                   QWLAN_PMU_CFG_MCU_SLEEP_STATE_RESOURCE_REQ_PD_CMEM_BANK_D_CNTL_BIT_MASK));
#endif  // NT_FN_PDC_

        value = NT_REG_RD(QWLAN_PMU_PMIC_DTOP_GDSCR_REG);
        NT_REG_WR(QWLAN_PMU_PMIC_DTOP_GDSCR_REG, value | QWLAN_PMU_PMIC_DTOP_GDSCR_RETAIN_FF_ENABLE_MASK);
    }
    NT_REG_WR(QWLAN_PMU_CFG_WUR_SS_STATE_REG, NT_PMU_CFG_WUR_OFF_OFFSET);
    NT_REG_WR(QWLAN_PMU_CFG_WIFI_SS_STATE_REG, NT_PMU_CFG_WIFI_SLEEP_OFFSET);

    // TEMP: added after discussion with Sun, should switch back to pwm-on-tx on wakeup
    //  NT_REG_WR( 0x2043048, 0x03000000);//pfm force

    if (_socpm_slp_clk_src == NT_SOCPM_SLP_CLK_RFAXO) {
        // to use RFA XO as sleep, keep PMIC in active mode
        NT_REG_WR(QWLAN_PMU_CFG_PMIC_SLEEP_MODE_CNTL_REG,
                  QWLAN_PMU_CFG_PMIC_SLEEP_MODE_CNTL_CFG_DISABLE_PMIC_SLEEP_MODE_MASK);
    } else  // PMIC XO or RC
    {
        if (wr_all_f) {
            value = NT_REG_RD(QWLAN_PMU_RFA_DTOP_GDSCR_REG);
            NT_REG_WR(QWLAN_PMU_RFA_DTOP_GDSCR_REG, value | QWLAN_PMU_RFA_DTOP_GDSCR_RETAIN_FF_ENABLE_MASK);
        }
        NT_REG_WR(QWLAN_PMU_CFG_PMIC_SLEEP_MODE_CNTL_REG, QWLAN_PMU_CFG_PMIC_SLEEP_MODE_CNTL_DEFAULT);
    }

    NT_REG_WR(QWLAN_PMU_SLP_CNTL_REG, QWLAN_PMU_SLP_CNTL_EN_SLEEP_MASK);
    NT_REG_WR(_SOCPM_CPU_SYS_CTL_REG, 4);  // MCU sleep state
    NT_REG_WR(QWLAN_PMU_CFG_MCU_SS_STATE_REG, NT_PMU_CFG_MCU_SLEEP_OFFSET);
}

/**
 *  @brief Enable or disable Indefinite deep sleep
 *
 *  @param  en_flag     0 = MCU Deep Sleep; 1 = Indefinite Deep Sleep
 *  @return None
 */
void nt_socpm_en_indef_deep_sleep(bool en_flag)
{
#ifdef FEATURE_INDEF_DEEP_SLP
    g_socpm_struct.socpm_indef_deep_sleep_en = en_flag;
#else
    SOCPM_UNUSED(en_flag);
    return;
#endif /* FEATURE_INDEF_DEEP_SLP */
}

/**
 *  @brief Set SON during MCU_SLEEP
 *
 *  @param  en_flag     0 = SON OFF(Default); 1 = SON ON
 *  @return None
 */
void nt_socpm_set_mcu_slp_dbg_mode(bool en_flag)
{
#ifdef FEATURE_FERMION_SLP_DBG
    g_socpm_struct.socpm_mcu_sleep_dbg_mode = en_flag;
#else
    SOCPM_UNUSED(en_flag);
    return;
#endif /* FEATURE_FERMION_SLP_DBG */
}

/**
 *  @brief Enable Forced Faults in RMC code
 *
 *  @param  en_flag     1 = Enable
 *  @return None
 */
void nt_socpm_enable_rmc_forced_faults(bool en_flag)
{
#ifdef NT_DEBUG
    g_socpm_struct.rmc_fault_force = en_flag;
#else
    SOCPM_UNUSED(en_flag);
#endif /* NT_DEBUG */
}

/**
 *  @brief Set pulse width of F2A
 *
 *  @param  pwidth_us       F2A Pulse width in us
 *  @return None
 */
void nt_socpm_set_f2a_pulse_width(uint32_t pwidth_us)
{
#ifdef FIRMWARE_APPS_INFORMED_WAKE
    g_socpm_struct.f2a_pulse_duration_us = pwidth_us;
#else
    SOCPM_UNUSED(pwidth_us);
#endif /* FIRMWARE_APPS_INFORMED_WAKE */
}

/**
 *  @brief Print current wakelock status
 *
 */
void print_wakelock_status()
{
    PM_STRUCT *pPmStruct = (PM_STRUCT *)gdevp->pPmStruct;
    NT_LOG_SME_ERR("Sleep Abort Wakelock acquired! ", pPmStruct->powerModeWakeupCount, 0, 0);
    for (uint8_t i = 0; i < PM_MODULE_ID_MAX; i++) {
        if (pPmStruct->powerModeModuleWakeupCount[i]) {
            NT_LOG_PRINT(SME, ERR, "WL_Module[%d] : [%d]", i, pPmStruct->powerModeModuleWakeupCount[i]);
        }
    }
}

#if defined(SUPPORT_SWTMR_TO_WKUP_FROM_BMPS) && defined(NT_DEBUG)
/*
 * @brief Funtion to log the timer/task info after sleep
 * @param none
 * @return none
 */
void nt_socpm_log_wkup_reason_after_sleep(void)
{
    char buff[200] = {};
    if (g_socpm_struct.wkup_reason == REASON_TO_WKUP_Q_TIMER && g_socpm_struct.info.q_timer_callback) {
        snprintf(buff, sizeof(buff), "Expired Q-timer info: last q-timer slept for:%u ms, callback:%x",
                 (unsigned int)(g_socpm_struct.info.ms_time), (unsigned int)(g_socpm_struct.info.q_timer_callback));
    } else if ((g_socpm_struct.wkup_reason == REASON_TO_WKUP_NT_TIMER) && g_socpm_struct.info.nt_timer_callback) {
        snprintf(buff, sizeof(buff), "Expired Nt-timer info: last Nt-timer slept for:%u ms, callback:%x",
                 (unsigned int)(g_socpm_struct.info.ms_time), (unsigned int)(g_socpm_struct.info.nt_timer_callback));
    } else if (g_socpm_struct.wkup_reason == REASON_TO_WKUP_NT_TASK && g_socpm_struct.info.pcTaskName) {
        snprintf(buff, sizeof(buff), "Expired Nt-task info: last Nt-task slept time:%u ms, task name:%s",
                 (unsigned int)(g_socpm_struct.info.ms_time), g_socpm_struct.info.pcTaskName);
    } else {
        snprintf(buff, sizeof(buff), "%s", "sleep reason not a valid type");
    }
    NT_LOG_PRINT(SOCPM, INFO, buff);
}

/*
 * @brief  to set a2f_processing_delay, which sets the delay for
 * FTDI to detect any F2A pulse, following an A2F assertion.
 * @param  delay in us
 * @return  none
 */
void nt_socpm_set_a2f_processing_delay(uint16_t delay)
{
#ifdef FIRMWARE_APPS_INFORMED_WAKE
    g_socpm_struct.a2f_processing_delay = delay;
#else
    SOCPM_UNUSED(delay);
#endif  // FIRMWARE_APPS_INFORMED_WAKE
}

#endif /* SUPPORT_SWTMR_TO_WKUP_FROM_BMPS && NT_DEBUG */

#ifdef SUPPORT_SLEEP_DEBUG_UNIT_TEST_CMD
/*
 * @brief : This function is used as dummy min call back for dummy slp lst node
 * @param : None
 * @return : sleep back value
 */
uint64_t dummy_min_callback(__unused uint32_t wkup_delay_us)
{
    NT_LOG_PRINT(SOCPM, ERR, "Dummy min call back");
    return 0;
}

/*
 * @brief : This function is used as dummy wakeup call back for dummy slp lst node
 * @param : wkup reason
 * @return : None
 */
void dummy_wakeup_callback(soc_wkup_reason wkup_reason)
{
    (void)wkup_reason;
    NT_LOG_PRINT(SOCPM, ERR, "Dummy wake up call back list idx (%d)", g_socpm_struct.socpm_dbg_curr_lst_idx);
    nt_socpm_sleep_lst_delete(g_socpm_struct.socpm_dbg_curr_lst_idx);
}

/*
 * @brief : This function is used as dummy sleep call back for dummy slp lst node
 * @param : None
 * @return : None
 */
void dummy_sleep_callback(void)
{
    NT_LOG_PRINT(SOCPM, ERR, "Dummy sleep call back");
}

/*
 * @brief : This function helps to enable/disable debug logs in sleep list functions
 * @param : enable (TRUE/FALSE)
 * @return : None
 */
void nt_enable_slp_list_debug_log(bool enable)
{
    if (enable) {
        g_socpm_struct.socpm_dbg_unit_test_value |= (1 << SLP_DBG_SOCPM_SLP_LIST_COMP_LOG);
    } else {
        g_socpm_struct.socpm_dbg_unit_test_value &= ~(1 << SLP_DBG_SOCPM_SLP_LIST_COMP_LOG);
    }
    NT_LOG_PRINT(SOCPM, ERR, "SLP_DBG_SOCPM_LIST_COMP_LOG: %d", enable);
}

/*
 * @brief : This function helps to set soc state enable/disable during protocol sleep
 * @param : enable (TRUE/FALSE)
 * @return : None
 */
void nt_set_soc_state_during_protocol_sleep(bool enable)
{
    if (enable) {
        g_socpm_struct.socpm_dbg_unit_test_value |= (1 << SLP_DBG_SOCPM_ON_DURING_BMPS);
    } else {
        g_socpm_struct.socpm_dbg_unit_test_value &= ~(1 << SLP_DBG_SOCPM_ON_DURING_BMPS);
    }
    NT_LOG_PRINT(SOCPM, ERR, "SLP_DBG_SOCPM_ON_DURING_BMPS: %d", enable);
}

/*
 * @brief : This function is used to enable/disable dummy sleep list node handle in slp list
 * @param : enable (TRUE/FALSE)
 * @return : None
 */
void nt_enable_dummy_slp_list_node(bool enable, uint64_t sleep_time, int dummy_node_count)
{
    if (enable) {
        if (_socpm_last_slp_count < (_SOCPM_SLP_LST_SZ - 1)) {
            g_socpm_struct.socpm_dbg_unit_test_value |= (1 << SLP_DBG_SOCPM_SLP_LIST_ADD);
            g_socpm_struct.dummy_slp_time_us = sleep_time;
            g_socpm_struct.dummy_slp_lst_node_count = dummy_node_count;
            g_socpm_struct.add_dummy_slp_list_node = TRUE;
            NT_LOG_PRINT(SOCPM, ERR, "SLP_DBG_SOCPM_SLP_LIST_ADD: %d", enable);
        } else {
            NT_LOG_PRINT(SOCPM, ERR, "sleep list is full, no more room to add new node in current sleep list");
        }
    } else {
        g_socpm_struct.socpm_dbg_unit_test_value &= ~(1 << SLP_DBG_SOCPM_SLP_LIST_ADD);
    }
}

/*
 * @brief : This function is used to enable/disable beacon miss logs
 * @param : enable (TRUE/FALSE)
 * @return : None
 */
void nt_enable_beacon_miss_log(bool enable)
{
    if (enable) {
        g_socpm_struct.socpm_dbg_unit_test_value |= (1 << SLP_DBG_SOCPM_BEACON_MISS_LOG);
    } else {
        g_socpm_struct.socpm_dbg_unit_test_value &= ~(1 << SLP_DBG_SOCPM_BEACON_MISS_LOG);
    }
    NT_LOG_PRINT(SOCPM, ERR, "SLP_DBG_SOCPM_BEACON_MISS_LOG: %d", enable);
}

/*
 * @brief : This function is used to get socpm status
 * @param : None
 * @return : None
 */
uint32_t nt_socpm_status(void)
{
    if (nt_socpm_slp_time_min > 0) {
        NT_LOG_PRINT(SOCPM, INFO, "socpm status: enabled");
    } else {
        NT_LOG_PRINT(SOCPM, INFO, "socpm status: disabled");
    }
    return nt_socpm_slp_time_min;
}

/*
 * @brief : This function is used to get status of bcn logs enabled/disabled
 * @param : None
 * @return : TRUE if enabled else false
 */
bool nt_bcn_logs_is_enabled(void)
{
    return (g_socpm_struct.socpm_dbg_unit_test_value & (1 << SLP_DBG_SOCPM_BEACON_MISS_LOG));
}

/*
 * @brief : This function is used to add  dummy sleep list node handle in slp list
 * @param : None
 * @return : None
 */
void nt_add_dummy_slp_list_node(void)
{
    static nt_socpm_sleep_t dummy_timer = {
        dummy_sleep_callback, dummy_wakeup_callback, dummy_min_callback, clk_gtd_sleep, 100, -1, 0};
    for (int count = 0; count < g_socpm_struct.dummy_slp_lst_node_count; count++) {
        dummy_timer.slp_time = g_socpm_struct.dummy_slp_time_us;
        nt_socpm_sleep_register(&dummy_timer, -1);
    }
    g_socpm_struct.add_dummy_slp_list_node = FALSE;
}

#endif /* SUPPORT_SLEEP_DEBUG_UNIT_TEST_CMD */

/*
 * @brief  : update clk latency(in us).it could be 3ms or 32us. default clk_latency is 3ms.
 *         : When WiFi is connected with handset/home AP and active audio streaming is about to start:clk_latency = 32us
 *         : Rest all cases: clk_latency = 3ms
 * @param  : buffer - pointer which contains information related to WMI_CLK_LATENCY_CMD command
 * @return : nt_status_t
 */
nt_status_t nt_update_clk_latency(void *buffer)
{
    if (buffer == NULL) {
        return NT_EPARAM;
    }

#ifdef SUPPORT_RING_IF
    uint32_t old_clk_latency = g_socpm_struct.clk_latency_us;
    wlan_clk_latency_cmd_t *clk_lat_cmd = (wlan_clk_latency_cmd_t *)buffer;
    g_socpm_struct.clk_latency_us = (uint32_t)clk_lat_cmd->clk_latency;

    /*Send event with status success */
    wlan_clk_latency_evt_t clk_lat_evt;
    clk_lat_evt.evt_hdr.status = NT_OK;
    if (g_Cmd_Translation_wifi_hndl.msg_struct.event_notify) {
        g_Cmd_Translation_wifi_hndl.msg_struct.event_notify(eWiFiSuccess, wifi_clk_latency_event_id, &clk_lat_evt);
    }
    NT_LOG_PRINT(SOCPM, INFO, "CLK Latency updated: old_clk_latency: %d new_clk_latency: %d", old_clk_latency,
                 g_socpm_struct.clk_latency_us);
#endif
    return NT_OK;
}
