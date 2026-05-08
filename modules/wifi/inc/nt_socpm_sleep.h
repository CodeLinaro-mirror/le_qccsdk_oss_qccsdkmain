/*
Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#ifndef _NT_SOCPM_SLEEP_H_
#define _NT_SOCPM_SLEEP_H_

#include <stdint.h>
#include <stdbool.h>
#include "wifi_cmn.h"
#include "fwconfig_cmn.h"
#include "nt_flags.h"
#include "nt_common.h"
#include "nt_osal.h"
#include "timer.h"
#include "wlan_sleep_clk_cal.h"
#include "wifi_fw_cpr_driver.h"

#ifndef SOCPM_UNUSED
#define SOCPM_UNUSED(x) (void)(x)
#endif /* SOCPM_UNUSED */

#define NT_NVIC_ISER1 0xE000E104  // Irq 32 to 60 Set Enable Register
#define NT_NVIC_ICPR1 0xE000E284
/*
//equivalent count for 1 milliseconds =(1000e-6/1 count) 1 count = 30 e-6 seconds
#define _SOCPM_TICK_PERIOD    32
*/

/* Default clk latency is 3ms */
#define DEFAULT_CLK_LATENCY_US                 3000
#define CLK_LATENCY_GET(socpm_struct)          ((socpm_struct)->clk_latency_us)
#define CLK_LATENCY_SET(socpm_struct, clk_lat) ((socpm_struct)->clk_latency_us = clk_lat)

/*
  While programming AON, the calculation is as per 32.768 kHz. If the clock source is RFA XO, AON internally
  adjusts the expiry value for 32kHz. The AON ticks read from SLP_TMR registers need to be scaled as per the
  sleep clock source used.
*/
// AON tick is computed in terms of 32.768 kHz. Each tick is 1/32768s = 1000000/32768us = 15625/512us
// So, number of ticks, given the time in us is (time * 512)/15625
#define _SOCPM_US_TO_AON_TICK(slp_time_us) (((slp_time_us)*512) / 15625)
// Frequency of RFA XO clock source(XO/1000) is 32 kHz. Each tick is 1/32000s = 1000000/32000us = 1000/32us
// So, time in us, given the number of ticks is (aon_ticks * 1000)/32
#define _SOCPM_XO_CLK_AON_TICK_TO_US(aon_ticks) (((aon_ticks)*1000) / 32)
// Frequency of RC clock and external sleep clock source is 32.768 kHz. Each tick is 1/32768s = 1000000/32768us =
// 15625/512us So, time in us, given the number of ticks is (aon_ticks * 15625)/512
#define _SOCPM_RC_OR_EXT_CLK_AON_TICK_TO_US(aon_ticks) (((aon_ticks)*15625) / 512)

#if defined IO_DEBUG
#define MAX_IO_PINS 21
#endif /*IO_DEBUG*/

typedef enum soc_wkup { SOC_WKUP_COMPLETE, SOC_WKUP_ABORT } soc_wkup_reason;

#ifdef NT_SOCPM_SW_MTUSR
typedef enum mtusr_ts { MTUSR_TS_QTMR, MTUSR_TS_SYSTCK } mtusr_ts_type;

/*MTU save-restore timestamp structure*/
typedef struct nt_mtusr_timestamp_s {
    uint64_t time;
    mtusr_ts_type type;
} nt_mtusr_timestamp_t;

/*MTU time data save-restore structure*/
typedef struct nt_mtusr_time_save_s {
    uint32_t mtu_glob_tmr;
    uint64_t mtu_tsf_us;
    uint64_t mtu_tbtt;
    uint32_t mtu_bcn_bssid_intv;
    bool aon_programmed;
    nt_mtusr_timestamp_t mtu_timestamp;
    nt_mtusr_timestamp_t aon_timestamp;
} nt_mtusr_time_save_t;
#endif  // NT_SOCPM_SW_MTUSR

#ifdef SUPPORT_SLEEP_DEBUG_UNIT_TEST_CMD
/* The below enums helps to mask _socpm_dbg_unit_test_value
 * and set/get the configuration
 */
enum nt_slp_dbg_unit_test_type {
    SLP_DBG_SOCPM_SLP_LIST_COMP_LOG,
    SLP_DBG_SOCPM_SLP_LIST_ADD,
    SLP_DBG_SOCPM_ON_DURING_BMPS,
    SLP_DBG_SOCPM_BEACON_MISS_LOG,
};
#define MAX_MULTI_LST_NODES 5
#define MIN_MULTI_LST_NODES 2
#endif /* SUPPORT_SLEEP_DEBUG_UNIT_TEST_CMD */

#define NT_SOCPM_IRQ_ENABLE()                     \
    do {                                          \
        __asm volatile("dmb 0xF" ::: "memory");   \
        __asm volatile("cpsie i" : : : "memory"); \
    } while (0)

#define NT_SOCPM_IRQ_DISABLE()                    \
    do {                                          \
        __asm volatile("cpsid i" : : : "memory"); \
        __asm volatile("dmb 0xF" ::: "memory");   \
    } while (0)

#define NT_SOCPM_FAULT_DISABLE() __asm volatile("cpsid f \n")

#define NT_SOCPM_FAULT_ENABLE() __asm volatile("cpsie f \n")
// deprecated alias
#define cpu_irq_disable() NT_SOCPM_IRQ_DISABLE()

// AON sleep cock source selection
#define NT_SOCPM_SLP_CLK_RC     0
#define NT_SOCPM_SLP_CLK_RFAXO  1
#define NT_SOCPM_SLP_CLK_PMICXO 2

#define NT_SOCPM_NVIC_ISER0 0xE000E100  // Irq 0 to 31 set enable register address
#define NT_SOCPM_NVIC_ISER1 0xE000E104  // Irq 32 to 63 set Enable register address
#define NT_SOCPM_NVIC_ISER2 0xE000E108  // Irq 64 to 95 set Enable register address

#define AON_TIMER_INTR_NVIC1_MASK (0x1 << 23)

#if defined(EMULATION_BUILD) && defined(PLATFORM_FERMION)
#define NT_SOCPM_FPGA_TOP_REG                  0x01EF0000
#define NT_SOCPM_FPGA_TOP_DIVIDE_32K_BY16_MASK 0x2000000
#endif  // defined(EMULATION_BUILD) && defined(PLATFORM_FERMION)

#ifdef NT_CC_DEBUG_FLAG

#define NT_SOCPM_FOOT_SWITH_CTL_REG           0x2043080  // foot switch control register in RFA domain
#define NT_SOCPM_FOOT_SWITH_CTL_OFFSET        0x06       // foot switch ctl enable/disable bit offset
#define NT_SOCPM_FOOT_SWITCH_CHARGE_RATE_MASK 0x3F       // foot switch resistance selection mask

#endif

#define MCU_SLEEP_OFF_TO_CLK_REQ_US             (300)
#define MCU_SLEEP_CLK_REQ_TO_MX_SUPPLY_TIMER_US (2000)
#define MCU_SLEEP_MX_SUPPLY_TO_XO_SETTLE_US     (1526)
#define MCU_SLEEP_XO_SETTLE_TO_CPU_BOOT_US      (125)
/* time to account for board to board variation seen in HW wake time */
#define MCU_SLEEP_HW_WAKE_VARIATION_TOLERANCE_US (180)
#define MCU_SLEEP_HW_S2W_TRANSITION_TIME_US                                                                        \
    (MCU_SLEEP_OFF_TO_CLK_REQ_US + MCU_SLEEP_CLK_REQ_TO_MX_SUPPLY_TIMER_US + MCU_SLEEP_MX_SUPPLY_TO_XO_SETTLE_US + \
     MCU_SLEEP_XO_SETTLE_TO_CPU_BOOT_US + MCU_SLEEP_HW_WAKE_VARIATION_TOLERANCE_US) /*W2S*/
#define MCU_SLEEP_SW_SLEEP_TRANSITION_TIME_US (4500)
#define MCU_SLEEP_OVERALL_SLEEP_TRANSITION_TIME_US \
    (MCU_SLEEP_HW_S2W_TRANSITION_TIME_US + MCU_SLEEP_SW_SLEEP_TRANSITION_TIME_US)
#define CLK_GATED_SLEEP_SW_SLEEP_TRANSITION_TIME_US (1350)

/* time from CPU warm boot due to AON timerexpiry to min_cb execution */
#define MCU_SLEEP_CPU_BOOT_TO_MIN_CB_US (340)

#if defined(SUPPORT_LIGHT_SLEEP_FOR_TWT) || defined(SUPPORT_SOC_SLEEP_SOLVER)
/* These are initial measurements from simulations and emulation profiling.
 * actual measurements need to performed on chip and this has to be optimised*/
#define LIGHT_SLEEP_HW_SLEEP_TRANSITION_TIME_US (450) /*S2W+W2S*/
#define LIGHT_SLEEP_SW_SLEEP_TRANSITION_TIME_US (1350)
#define LIGHT_SLEEP_OVERALL_SLEEP_TRANSITION_TIME_US \
    (LIGHT_SLEEP_HW_SLEEP_TRANSITION_TIME_US + LIGHT_SLEEP_SW_SLEEP_TRANSITION_TIME_US)
/* time from CPU warm boot due to AON timerexpiry to min_cb execution */
#define LIGHT_SLEEP_CPU_BOOT_TO_MIN_CB_US (340)
#endif /*(SUPPORT_LIGHT_SLEEP_FOR_TWT) || defined (SUPPORT_SOC_SLEEP_SOLVER)*/

extern uint8_t xo_settle_time;
extern uint8_t pmic_slp_exit_time;
extern uint32_t slp_exit_hw_delay_fixed;
extern uint8_t ignore_bcmc_in_bmps;

#define NT_CHECK_BIT_STATE(_value, _pos) (_value & (1 << _pos))

/* Time taken from end of min cb to context restore */
#define MINCB_END_TO_CTXT_RESTORE_US 70
/* Time taken from context restore to restarting the scheduler */
#define CTX_RESTORE_TO_SCHED_RESTART_US 290

/* Upper limit on sleep slop offset time */
#define SLEEP_SLOP_OFFSET_UPPER_LIMIT_US 3000

#if defined(COMPENSATE_RC_DIVISION_ERROR_WAR)
#define XO_CLOCK_CYCLES_FOR_REF_CNT_256 \
    262500  // Nominal number of 38.4 MHZ XO clock cycles for REF_SLEEP_CLK_CNT <=256
#define XO_CLOCK_CYCLES_FOR_REF_CNT_512 523828  // Nominal number of 38.4 MHZ XO clock cycles for REF_SLEEP_CLK_CNT 512
#define XO_CLOCK_CYCLES_FOR_REF_CNT_1024 \
    1048828  // Nominal number of 38.4 MHZ XO clock cycles for REF_SLEEP_CLK_CNT 1024
#define RIGHT_SHIFT_DIVIDE_VAL_256 \
    262144  // Right shift divide value in power of and approximation of Nominal number <=256
#define RIGHT_SHIFT_DIVIDE_VAL_512 \
    524288  // Right shift divide value in power of and approximation of Nominal number 512
#define RIGHT_SHIFT_DIVIDE_VAL_1024 \
    1048576  // Right shift divide value in power of and approximation of Nominal number 1024
#define NO_RIGHT_SHIFT (18 + (REF_SLEEP_CLK_CNT >> 9))  // Number of right shift required to divide

#if (REF_SLEEP_CLK_CNT <= 256)
#define COMPENSATE_RC_DIVISION_ERROR_SLP_TMR_SET(sleep_time) \
    ((uint64_t)(sleep_time * XO_CLOCK_CYCLES_FOR_REF_CNT_256) >> NO_RIGHT_SHIFT)
#define COMPENSATE_RC_DIVISION_ERROR_SLP_TMR_GET(sleep_time) \
    ((uint64_t)(sleep_time * RIGHT_SHIFT_DIVIDE_VAL_256) / XO_CLOCK_CYCLES_FOR_REF_CNT_256)
#elif (REF_SLEEP_CLK_CNT <= 512)
#define COMPENSATE_RC_DIVISION_ERROR_SLP_TMR_SET(sleep_time) \
    ((uint64_t)(sleep_time * XO_CLOCK_CYCLES_FOR_REF_CNT_512) >> NO_RIGHT_SHIFT)
#define COMPENSATE_RC_DIVISION_ERROR_SLP_TMR_GET(sleep_time) \
    ((uint64_t)(sleep_time * RIGHT_SHIFT_DIVIDE_VAL_512) / XO_CLOCK_CYCLES_FOR_REF_CNT_512)
#else
#define COMPENSATE_RC_DIVISION_ERROR_SLP_TMR_SET(sleep_time) \
    ((uint64_t)(sleep_time * XO_CLOCK_CYCLES_FOR_REF_CNT_1024) >> NO_RIGHT_SHIFT)
#define COMPENSATE_RC_DIVISION_ERROR_SLP_TMR_GET(sleep_time) \
    ((uint64_t)(sleep_time * RIGHT_SHIFT_DIVIDE_VAL_1024) / XO_CLOCK_CYCLES_FOR_REF_CNT_1024)
#endif /* REF_SLEEP_CLK_CNT */
#endif /* COMPENSATE_RC_DIVISION_ERROR_WAR */

/* Time from CPU sleep to CLK_REQ going low, as profiled from waveforms */
#define MCU_SLEEP_HW_W2S_TRANSITION_TIME_US (1500)

#ifdef PLATFORM_FERMION
// Sleep modes types
typedef enum sleep_types { clk_gtd_sleep = 1, mcu_sleep, Standby, Active, Lightsleep, InfDeepsleep } sleep_mode;
#else
// Sleep modes types
typedef enum sleep_types { clk_gtd_sleep = 1, mcu_sleep, Standby, Active } sleep_mode;
#endif /* PLATFORM_FERMION */

typedef enum cpr_types { cpr_openloop = 0, cpr_closeloop = 1 } cpr_mode_e;

typedef uint64_t (*nt_socpm_min_fptr_t)(uint32_t);
typedef void (*nt_socpm_void_fptr_t)(void);
typedef void (*nt_socpm_wkup_fptr_t)(soc_wkup_reason);

typedef struct nt_socpm_sleep_s {
    nt_socpm_void_fptr_t slp_cb_fn;   // cb for entering sleep
    nt_socpm_wkup_fptr_t wkup_cb_fn;  // cb for full wake
    nt_socpm_min_fptr_t min_cb_fn;    // cb for "minimum" wake eg beacon processing, etc
    sleep_mode slp_mode;
    // Sleep time measured in us
    uint64_t slp_time;
    int list_no;
    uint64_t start_time_us;
} nt_socpm_sleep_t;

// Memory Control Type
typedef enum mem_ctrl {
    Off,
    On,
    Retention,
} Mem_Control;

/*USED RRAM OTP BIT MAP*/
typedef enum _FIRMWARE_REG_ {
    IO_CONFIG = 0,
    BOOT_METHOD,
    APP_MODE_SEL,
    AUTO_START_EN,
    CPR_EN,
    MAX_FIRWARE,
} nt_otp_firmware_reserved;

#if defined(SUPPORT_SWTMR_TO_WKUP_FROM_BMPS)
typedef enum reason_to_wkup {
    REASON_TO_WKUP_NT_TASK = 0,
    REASON_TO_WKUP_NT_TIMER,
    REASON_TO_WKUP_Q_TIMER,
} reason_to_wkup_t;
#endif /* SUPPORT_SWTMR_TO_WKUP_FROM_BMPS */

typedef struct {
    bool in_warm_boot;
#ifdef NT_SOCPM_SW_MTUSR
    nt_mtusr_time_save_t mtusr_time_data;
#ifdef SUPPORT_BMU_ERROR_RECOVERY
    /* MTU time data storage for BMU error recovery sequence */
    nt_mtusr_time_save_t bmu_recovery_mtusr_time_data;
#endif  /* SUPPORT_BMU_ERROR_RECOVERY */
#endif  // NT_SOCPM_SW_MTUSR
    uint32_t unapplied_err_us;
    uint32_t unapplied_systick_err_us;
    uint32_t systick_off_time_us;
    uint32_t aon_program_time_us;
#if defined(COMPENSATE_RC_DIVISION_ERROR_WAR)
    uint64_t glb_pre_sleep_time_us;
#endif
#if defined(SUPPORT_SOC_SLEEP_SOLVER)
    uint64_t aon_program_time_qtimer_us; /*time at which last time AON timer was programmed*/
#endif                                   /*SUPPORT_SOC_SLEEP_SOLVER*/
#ifdef FIRMWARE_APPS_INFORMED_WAKE
    bool a2f_asserted;               // Track state of A2F signal
    bool f2a_asserted;               // Track state of F2A signal
    uint32_t f2a_timeout_ms;         // Timeout for A2F in response to F2A assertion
    uint32_t inter_f2a_interval_us;  // Interval between F2A de-assertion and F2A pulse
    uint32_t f2a_pulse_duration_us;  // Duration of F2A pulse
    uint16_t a2f_processing_delay;
    nt_osal_timer_handle_t f2a_timer;  // Timer to track F2A timeout
    bool twt_wake_send_f2a;            // Enable sending F2A indication on wakeup from TWT
    bool f2a_assert_enabled;           // Disable F2A assert for testing with FermionApp
    bool host_supports_a2f;            // Flag to indicate if the HOST supports A2F
#endif                                 /*FIRMWARE_APPS_INFORMED_WAKE*/
#ifdef FEATURE_INDEF_DEEP_SLP
    bool socpm_indef_deep_sleep_en;
#endif /* FEATURE_INDEF_DEEP_SLP */
#ifdef FEATURE_FERMION_SLP_DBG
    bool socpm_mcu_sleep_dbg_mode;
#endif /* FEATURE_FERMION_SLP_DBG */
#ifdef NT_DEBUG
    bool rmc_fault_force;
#endif /* NT_DEBUG */
#if defined(SUPPORT_SWTMR_TO_WKUP_FROM_BMPS)
    sleep_time_info_t info;
    reason_to_wkup_t wkup_reason;
#endif /* SUPPORT_SWTMR_TO_WKUP_FROM_BMPS */
#ifdef FEATURE_FPCI
    bool imps_trigger_indication;
#endif
#ifdef PMU_REG_RETENTION_STATUS_FOR_SOC_SLP
    uint32_t pmu_dtop_reg_retention_status;
#endif /* PMU_REG_RETENTION_STATUS_FOR_SOC_SLP */
#ifdef SOCPM_RMC_DBG
    /* This is a global variable to count the number of wakeups */
    uint32_t full_wake_stats;
    uint32_t nt_socpm_ctxt_restore_ctr;
#endif /* SOCPM_RMC_DBG */
#ifdef SUPPORT_SLEEP_DEBUG_UNIT_TEST_CMD
    bool add_dummy_slp_list_node;        // Flag to add dummy nodes to slp list
    uint8_t socpm_dbg_curr_lst_idx;      // To get the current list index for maintaing the dummy slp node in slp list
    uint8_t dummy_slp_lst_node_count;    // Total dummy sleep nodes to be newly added to slp list
    uint16_t socpm_dbg_unit_test_value;  // To configure unit test frame work enable/disable features with bits
    uint64_t dummy_slp_time_us;          // Dummy sleep time value for unit test command
#endif                                   /* SUPPORT_SLEEP_DEBUG_UNIT_TEST_CMD */
#if defined(IO_DEBUG)
    uint8_t io_dbg_count;
#endif                       /*IO_DEBUG*/
    uint32_t clk_latency_us; /* clock latency(in us) needed during wake, default clk_latency is 3ms */
    volatile uint32_t nvic_icpr_status[4];
    volatile uint32_t wifi_ss_state;
#ifdef NT_DEBUG
    volatile uint32_t pre_sleep_nvic_icpr_status[4];  // store the pre sleep pending interrupts for debugging
#endif                                                /*NT_DEBUG*/
#ifdef SLEEP_CLK_CAL_IN_ACTIVE_MODE
    socpm_sleep_clk_cal_t slp_clk_cal_params;
#endif /* SLEEP_CLK_CAL_IN_ACTIVE_MODE */
    uint32_t rmc_system_status;
#ifdef PLATFORM_FERMION
    cpr_cfg_t cpr_cfg;
#endif /* PLATFORM_FERMION */
    /* Parameters related to sleep slop offset:
     * Sleep slop offset is the offset time which accounts for clock drifts
     * between the AP and STA, to ensure that protocol wakeups occur on time.
     */
    uint8_t slop_step_us;      // Sleep slop offset step time per interval
    uint8_t slop_interval_ms;  // Granular time interval to calculate sleep slop offset
} SOCPM_STRUCT;
extern SOCPM_STRUCT g_socpm_struct;

/*
 * END OF USED RRAM OTP BITMAP
 */
//#define SOCPM_SLEEP_DEBUG
#ifdef SOCPM_SLEEP_DEBUG
typedef enum _SOCPM_DBG_TIMING {
    __PREV_SLP = 1,
    ___MIN_VEC,
    MIN_PROC_1,
    MIN_PROC_2,
    AON_PROC_1,
    AON_PROC_2,
    AON_PROC_3,
    _POST_WAKE,
    SLP_TM_SET,
    BMPS_ENTER,
    _____BCN_1,
    _____BCN_2,
    _____BCN_3,
    MTU_AON_SV,
    ____MTU_SV,
    __MTU_RSTR,
    ___BCN_RXD,
    _MIN_ENTRY,
    _PRE_BBPLL,
    POST_BBPLL,

    ___TMR_MAX,
} SOCPM_DBG_TIMING;
#define MAX_LOG_ENTRY_PM (___TMR_MAX * 90)
struct socpm_dbg_ts {
    SOCPM_DBG_TIMING proc;
    uint32_t ts;
    uint32_t ms;
    uint32_t tk;
    uint32_t d1;
    uint32_t d2;
    uint32_t d3;
    uint32_t slp_1;
    uint32_t slp_2;
};
extern uint8_t socpm_bmps_glance;
extern uint32_t first_bmps_slp_time;
extern uint32_t second_bmps_slp_time;
extern uint32_t bcn_nowake_limit;
uint8_t get_slp_lst_cnt();
void socpm_log_timestamp(SOCPM_DBG_TIMING proc, uint32_t d1, uint32_t d2, uint32_t d3);
#endif

/*
 * Functions that disable and enable the neutrino sleep timer respectively, not returning until
 * the operation is known to have taken effect.
 */

/*
 * @brief: This function used to enable or disable socpm
 * @param socpm_state - describes state of socpm whether it is enabled or disabled
 * @return none
 */
void nt_socpm_enable(uint8_t socpm_state);

void nlp_config(void);

void nt_socpm_slp_tmr_set(uint64_t sleep_time);

uint64_t nt_socpm_min_slp_time_us();
sleep_mode nt_socpm_curr_slp_mode();
uint32_t get_sleep_exit_hw_delay(sleep_mode slp_mode);

/**
 *  @brief Enable or disable Indefinite deep sleep
 *
 *  @param  en_flag     0 = MCU Deep Sleep; 1 = Indefinite Deep Sleep
 *  @return None
 */
void nt_socpm_en_indef_deep_sleep(bool en_flag);

/**
 *  @brief Set SON during MCU_SLEEP
 *
 *  @param  en_flag     0 = SON OFF(Default); 1 = SON ON
 *  @return None
 */
void nt_socpm_set_mcu_slp_dbg_mode(bool en_flag);

#ifdef SUPPORT_RING_IF
/**
 *  @brief Set pulse width of F2A
 *
 *  @param  pwidth_us       F2A Pulse width in us
 *  @return None
 */
void nt_socpm_set_f2a_pulse_width(uint32_t pwidth_us);
#endif

/**
 *  @brief Enable Forced Faults in RMC code
 *
 *  @param  en_flag     1 = Enable
 *  @return None
 */
void nt_socpm_enable_rmc_forced_faults(bool en_flag);

/**
 *  @brief Print current wakelock status
 *
 */
void print_wakelock_status(void);

/*
 * @brief: This function is used to get whether MCU SS was woken up from sleep
 * @param none
 * @return Whether woken from MCU SS sleep
 */
uint8_t nt_socpm_wake_from_mcuss_sleep(void);

/*
 * @brief: This function is used to get the SOCPM sleep list head
 * @param none
 * @return Current SOCPM sleep list head
 */
int nt_get_socpm_slp_lst_head(void);

uint64_t vPostSleepProcessing(void);
void vPreSleepProcessing(sleep_mode mode);
/*
 * @brief: This function used to turn ON/OFF the power domains as per need for BMPS
 * partial sleep
 * @param void
 * @return none
 */
void vPreSleepProcessingPartialSleep(void);

// set sleep timer and do sleep processing
// intended for use in minimal code
void nt_socpm_slp_enter(uint64_t slp_us);

// minimal code - process sleep functions
// returns requested sleep time (time to next wake)
uint64_t nt_socpm_min_proc(int *proc_routine);

/*
 * @brief  general purpose busy-wait delay
 * @param   n_nops - the number of nop used as a delay
 * @return  none
 */
void nt_socpm_nop_delay(uint64_t n_nops);

#ifdef SUPPORT_RING_IF
/*
 * @brief  to set a2f_processing_delay, which sets the delay for
 * FTDI to detect any F2A pulse, following an A2F assertion.
 * @param  delay in us
 * @return  none
 */
void nt_socpm_set_a2f_processing_delay(uint16_t delay);
#endif

#if !defined(IMAGE_FERMION)
// function to restore key h/w regs after wakeup (tsf, global timer, etc)
void nt_socpm_glob_restore(void);
#endif  // !defined(IMAGE_FERMION)
/*
 * @brief: This function used to config and enable staby for registered sleep time
 * @param sleep_time - Time to be in standby mode. The time unit is ms
 * @return none
 */
void nt_enable_standby(uint64_t sleep_time);
/*
 * @brief: This function used to config and enable indefinite deepsleep
 * @param
 * @return none
 */
void nt_enable_indef_deepsleep(uint64_t sleep_time);

/*
 * @brief: This function is used to get last slept time in us
 * @param none
 * @return last slept time in us from SLP timer value
 */
uint64_t nt_socpm_get_slp_tmr_us(void);

uint64_t freertosdefaultminimum(__unused uint32_t wkup_delay_us);

void nt_socpm_ctxt_restore(void) __attribute__((naked));

// deprecated, do not use
uint8_t mem_bank_check(uint32_t bank, Mem_Control type, sleep_mode mode);

int nt_socpm_sleep_register(
    nt_socpm_sleep_t *FunctionToRegister,
    volatile int List_no);  // int nt_socpm_sleep_register(nt_socpm_sleep_t * FunctionToRegister,int List_no) ;

bool nt_socpm_sleep_lst_update(uint64_t sleep_time, bool serve_multi_node_wkup);
int nt_socpm_sleep_lst_delete(volatile int List_to_Del);
void nt_socpm_sleep_deregister(volatile int List_to_Del);
void nt_socpm_sleep_lst_reorder(volatile int modified, uint64_t head_prev_sleep_time);

int Sleep_time_update_list(uint64_t sleep_time);
int Del_Wakeup_List(volatile int List_to_Del);
void sleep_deregister(volatile int List_to_Del);
void reorder_list(volatile int modified);

void Aon_cmnss_wlan_slp_tmr_int(void);
/*function to clear the sleep timer interrupt*/
void _socpm_slptmr_off(void);

#ifdef NT_TST_HEAP_COMP_CODE
extern unsigned int _ln_bss_end__;
extern unsigned int _ln_RAM_addr_heap_start__;
extern unsigned int _ln_RF_start_addr_app_txt__;   // start address of apps text in RRAM
extern unsigned int _ln_RAM_start_addr_app_txt__;  // start address to load apps text in RAM
extern unsigned int _ln_app_txt_size__;            // apps text size

extern unsigned int _ln_RF_start_addr_app_data__;   // start address apps data in RRAM
extern unsigned int _ln_RAM_start_addr_app_data__;  // start address to load apps data in RAM
extern unsigned int _ln_app_data_size__;            // apps data size

extern unsigned int _ln_RF_start_addr_perf_txt__;   // start address of perf text in RRAM
extern unsigned int _ln_RAM_start_addr_perf_txt__;  // start address to load perf text in RAM
extern unsigned int _ln_perf_txt_size__;            // perf text size

extern unsigned int _ln_RF_start_addr_perf_data__;   // start address perf data in RRAM
extern unsigned int _ln_RAM_start_addr_perf_data__;  // start address to load perf data in RAM
extern unsigned int _ln_perf_data_size__;            // perf data size

extern unsigned int _ln_RF_start_addr_data__;   // start address data section in RRAM
extern unsigned int _ln_RAM_start_addr_data__;  // start address to load data in RAM
extern unsigned int _ln_data_size__;            // data section size

void nt_do_heap_decompression(void);
void nt_do_heap_compression(void);
#endif

/*
 * @brief: This function used to configure AON sleep clock from CLI
 * @param option 0 = RC from PMIC, 1 = XO from RFA, 2 = External XO from PMIC
 * @return none
 */
void nt_sleep_clock_configuration(uint8_t option);

#ifndef PLATFORM_FERMION
// External Interrupt
void enable_aon_ext_wakeup_int(void);
void aon_ext_interrupt_wake_up(void);
#endif  // PLATFORM_FERMION

#ifdef NT_NEUTRINO_1_0_SYS_MAC
/*
 * @brief: This function used to config pmic
 * @param none
 * @return none
 */
void nt_socpm_tst_pmic_cfg(void);

/*
 * @brief: This function used to force enter deepsleep/standby mode
 * @param slp_ms sleep time in milliseconds
 * @return none
 */
void nt_socpm_tst_standby(uint32_t slp_ms);
#endif

/* Read SOC power devcfg parameters before PMIC and SOCPM init */
void nt_socpm_init_soc_cfg(void);

void nt_socpm_init(void);

/*
 *  @brief : Initializes PMU temperature sensor and Sleep Clock Cal
 *  @param : none
 *  @return : None
 */
void nt_socpm_secondary_init(void);

/*
 *  @brief : Check if there was an unexpected failure in entering to sleep after wfi
 *  @param :
 *      mode - sleep mode being entered
 *      is_ctxt_rstr_point - whether called after context save, at the point where context restore would resume
 * execution
 *  @return : None
 */
void nt_socpm_check_sleep_entry_failure(sleep_mode mode, bool is_ctxt_rstr_point,bool warm_boot);

#ifdef NT_SOCPM_SW_MTUSR
/*
 *  @brief : Restore critical MTU time data after sleep exit
 *  @param : None
 *  @return : None
 */
void nt_socpm_mtusr_restore_mtu_time(void);

/*
 *  @brief : Save MTU time data before WiFi sleep entry for MTU save-restore
 *  @param : None
 *  @return : None
 */
void nt_socpm_mtusr_save_mtu_time(void);

/*
 *  @brief : Save timestamp when AON timer is programmed for MTU save-restore
 *  @param : None
 *  @return : None
 */
void nt_socpm_mtusr_save_aon_prog_timestamp(void);

#ifdef SUPPORT_BMU_ERROR_RECOVERY
/*
 *  @brief : Minimal version to save MTU time data before WiFi sleep for BMU recovery sequence
 *  @param : None
 *  @return : None
 */
void nt_socpm_mtusr_save_mtu_time_on_bmu_recovery(void);

/*
 *  @brief : Minimal version of MTU time restoration for BMU recovery sequence
 *  @param : None
 *  @return : None
 */
void nt_socpm_mtusr_restore_mtu_time_on_bmu_recovery(void);
#endif  /* SUPPORT_BMU_ERROR_RECOVERY */
#endif  //  NT_SOCPM_SW_MTUSR

/*
 * @brief: This function is used to enable/disable the foot switch init squence (enables the bypass cap(22uF))
 * @param integer of foot switch state(0/1)
 * @return none
 */
void nt_socpm_footsw_state_set(uint8_t st);

/*
 * @brief : This function is used to enable/disable socpm based on devcfg
 * @param : None
 * @return : None
 */
void nt_configure_socpm_state(void);

/*
 * @brief: This function is used to fetch the status of uart enable/disable flag to the caller
 * @param : bit position counting from lsb , limited between 0 to 7
 * @return : boolean (true(1))/(false(0))
 */
_Bool __attribute__((optimize("00"))) nt_socpm_uart_flag_state_get(nt_otp_firmware_reserved pos);

/*
 * @brief: This function is used to fetch the status of auto_start enable/disable flag to the caller
 * @param : bit position counting from lsb , limited between 0 to 7
 * @return : boolean (true(1))/(false(0))
 */
_Bool __attribute__((optimize("00"))) nt_socpm_auto_start_flag_state_get(nt_otp_firmware_reserved pos);

_Bool __attribute__((optimize("00"))) nt_socpm_cpr_flag_state_get(nt_otp_firmware_reserved pos);

#if defined(SUPPORT_SWTMR_TO_WKUP_FROM_BMPS) && defined(NT_DEBUG)
/*
 * @brief: This function is used to log the timer/task info after a BMPS sleep
 * @param : none
 * @return : none
 */
void nt_socpm_log_wkup_reason_after_sleep(void);
#endif /* SUPPORT_SWTMR_TO_WKUP_FROM_BMPS  && NT_DEBUG*/

#ifdef SUPPORT_SLEEP_DEBUG_UNIT_TEST_CMD
/*
 * @brief : This function is used as dummy min call back for dummy slp lst node
 * @param : None
 * @return : sleep back value
 */
uint64_t dummy_min_callback(__unused uint32_t wkup_delay_us);

/*
 * @brief : This function is used as dummy wakeup call back for dummy slp lst node
 * @param : wkup reason
 * @return : None
 */
void dummy_wakeup_callback(soc_wkup_reason wkup_reason);

/*
 * @brief : This function is used as dummy sleep call back for dummy slp lst node
 * @param : None
 * @return : None
 */
void dummy_sleep_callback(void);

/*
 * @brief : This function helps to enable/disable debug logs in sleep list functions
 * @param : enable (TRUE/FALSE)
 * @return : None
 */
void nt_enable_slp_list_debug_log(bool enable);

/*
 * @brief : This function helps to set soc state enable/disable during protocol sleep
 * @param : enable (TRUE/FALSE)
 * @return : None
 */
void nt_set_soc_state_during_protocol_sleep(bool enable);

/*
 * @brief : This function is used to enable/disable dummy sleep list node handle in slp list
 * @param : enable (TRUE/FALSE), sleep time of dummy node in list and dummy node count
 * @return : None
 */
void nt_enable_dummy_slp_list_node(bool enable, uint64_t sleep_time, int dummy_node_count);

/*
 * @brief : This function is used to enable/disable beacon miss logs
 * @param : enable (TRUE/FALSE)
 * @return : None
 */
void nt_enable_beacon_miss_log(bool enable);

/*
 * @brief : This function is used to get status of bcn logs enabled/disabled
 * @param : None
 * @return : TRUE if enabled else false
 */
bool nt_bcn_logs_is_enabled(void);

/*
 * @brief : This function is used to get socpm status
 * @param : None
 * @return : the value of socpm status 0:disable other: enable
 */
uint32_t nt_socpm_status(void);

/*
 * @brief : This function is used to add dummy sleep list node handle in slp list
 * @param : None
 * @return : None
 */
void nt_add_dummy_slp_list_node(void);

#endif /* SUPPORT_SLEEP_DEBUG_UNIT_TEST_CMD */
/*-----------------------------------------------------------*/

void nt_cpr_enable();
void nt_cpr_disable();

#if defined IO_DEBUG
/* Funtion to reset the io debug count */
void socpm_reset_io_debug_count();
#endif /*IO_DEBUG*/

/*
 * @brief  : update clk latency(in us).it could be 3ms or 32us. default clk_latency is 3ms.
 *         : When WiFi is connected with handset/home AP and active audio streaming is about to start:clk_latency = 32us
 *         : Rest all cases: clk_latency = 3ms
 * @param  : buffer - pointer which contains information related to WMI_CLK_LATENCY_CMD command
 * @return : nt_status_t
 */
nt_status_t nt_update_clk_latency(void *buffer);

void nt_socpm_soc_sleep_processing(uint64_t slp_val);
#endif /* _NT_SOCPM_SLEEP_H_ */
