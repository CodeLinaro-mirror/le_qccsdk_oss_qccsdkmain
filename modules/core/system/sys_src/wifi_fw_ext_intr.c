/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*========================================================================
 *
 * @brief Fermion specific external interrupt (A2F, F2A) related code
 *========================================================================*/
/*-------------------------------------------------------------------------
 * Include Files
 * ----------------------------------------------------------------------*/
#include "fwconfig_cmn.h"
#include "nt_flags.h"

#ifdef PLATFORM_FERMION

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "ExceptionHandlers.h"
#include "HALhwio.h"
#include "Fermion_seq_hwioreg.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "list.h"

#include "nt_common.h"
#include "nt_hw.h"
#include "nt_gpio_api.h"
#include "nt_socpm_sleep.h"
#include "nt_timer.h"
#include "nt_logger_api.h"

#include "wifi_fw_logger.h"
#if defined(SUPPORT_RING_IF) || defined(SUPPORT_RING_IF_ONLY)
#include "wifi_fw_cmn_api.h"
#endif
#include "wlan_power.h"
#include "wifi_fw_ext_intr.h"
#include "wifi_fw_internal_api.h"
#include "timer.h"

#ifdef SUPPORT_QCSPI_SLAVE
#include "qcspi_slave_api.h"
#endif

#ifdef FIRMWARE_APPS_INFORMED_WAKE
#include "ferm_prof.h"

/*-------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * ----------------------------------------------------------------------*/

/*------------------------------------------------------------------------
 * Global Data Definitions
 * ----------------------------------------------------------------------*/
extern SOCPM_STRUCT g_socpm_struct;
static SemaphoreHandle_t _socpm_mutex = NULL;

#define BMPS_EXIT_CMD_MIN_INTERVAL_MS  20U
volatile TickType_t s_last_bmps_exit_tick = 0;

#ifdef CONFIG_QAT_POWERSAVE_DEMO
/* External wakeup flag - used to track if wakeup was triggered by external pin */
static bool ext_wakeup_flag = FALSE;
#endif
/*-------------------------------------------------------------------------
 * Static Function Definitions
 * ----------------------------------------------------------------------*/

/*-------------------------------------------------------------------------
 * Externalized Function Definitions
 * ----------------------------------------------------------------------*/

/*
 * @brief  Fw F2A pulse signal
 * @param[in]       reason      Reason code for pulse
 * @return         : NONE
 *
 */
void __attribute__((section(".__sect_ps_txt"))) wifi_fw_ext_f2a_pulse(f2a_short_reason_t reason)
{
#if defined(SUPPORT_RING_IF) || defined(SUPPORT_RING_IF_ONLY)
    /* Wait till Fw Table is initialized */
    if (!wifi_fw_is_table_initialized()) {
        NT_LOG_PRINT(SOCPM, ERR, "F2A Int attempt before Table init");
        return;
    }

    if ((nt_twt_is_negotiated()) && (reason == F2A_SHORT_REASON_A2F_RESP)) {
        /* If TWT is negotiated Fw shall not send a F2A short in response to A2F assertion */
        NT_LOG_PRINT(SOCPM, INFO, "F2A short response in TWT mode");
    } else {
        // The delay is for the FTDI to detect any F2A pulse, following an A2F assertion.
        if (reason == F2A_SHORT_REASON_A2F_RESP && g_socpm_struct.a2f_processing_delay > 0) {
            hres_timer_us_delay(g_socpm_struct.a2f_processing_delay);
        }
        nt_gpio_pin_write(FIRMWARE_2_HOST_GPIO_PORT, FIRMWARE_2_HOST_GPIO, FIRMWARE_2_HOST_ASSERT);
        hres_timer_us_delay(g_socpm_struct.f2a_pulse_duration_us);
        nt_gpio_pin_write(FIRMWARE_2_HOST_GPIO_PORT, FIRMWARE_2_HOST_GPIO, FIRMWARE_2_HOST_DE_ASSERT);
        NT_LOG_PRINT(SOCPM, INFO, "F2A pulse");
    }
#endif
}

/*
 * @brief  Assert Fw to Apps(F2A) signal
 * @param[in]       reason      Reason code for pulse
 * @return         : NONE
 *
 */
void wifi_fw_ext_f2a_signal_assert(f2a_short_reason_t reason)
{
#if defined(SUPPORT_RING_IF) || defined(SUPPORT_RING_IF_ONLY)
    /* Wait till Fw Table is initialized */
    if (!wifi_fw_is_table_initialized()) {
        NT_LOG_PRINT(SOCPM, ERR, "F2A Assert attempt before Table init");
        return;
    }

    /** Do not assert F2A if A2F is already asserted
     * A2F need not be asserted as Aria is already awake and communicating
     * with Fw.
     */
    if (FALSE == g_socpm_struct.a2f_asserted) {
        if (g_socpm_struct.f2a_assert_enabled && g_socpm_struct.host_supports_a2f) {
            g_socpm_struct.f2a_asserted = TRUE;
            nt_gpio_pin_write(FIRMWARE_2_HOST_GPIO_PORT, FIRMWARE_2_HOST_GPIO, FIRMWARE_2_HOST_ASSERT);
            NT_LOG_PRINT(SOCPM, INFO, "F2A assert");
            nt_start_timer(g_socpm_struct.f2a_timer);
        } else {
            /** To aid in testing with FermionApp
             * If F2A assert is disabled using this command, the device
             * will send an F2A pulse instead of F2A assert to notify Apps.
             * The F2A pulse which is usually sent after F2A de-assertion
             * is also not sent if F2A assert is disabled.
             */
            wifi_fw_ext_f2a_pulse(reason);
        }
    } else {
        /**
         * If A2F is already asserted then only send pulses no F2A assertion allowed.
         */
        wifi_fw_ext_f2a_pulse(reason);
    }
#endif
}

/*
 * @brief  De-assert Fw to Apps(F2A) signal
 * @param          : NONE
 * @return         : NONE
 *
 */
void wifi_fw_ext_f2a_signal_deassert(void)
{
    nt_gpio_pin_write(FIRMWARE_2_HOST_GPIO_PORT, FIRMWARE_2_HOST_GPIO, FIRMWARE_2_HOST_DE_ASSERT);
    g_socpm_struct.f2a_asserted = FALSE;
    NT_LOG_PRINT(SOCPM, INFO, "F2A de-assert");
}

/*
 * @brief  De-assert Fw to Apps(F2A) signal
 * @param          : NONE
 * @return         : NONE
 *
 */
void wifi_fw_ext_f2a_timeout_cb(void)
{
    if (TRUE == g_socpm_struct.f2a_asserted) {
        if (TRUE == wifi_fw_in_hosted_mode()) {
            NT_LOG_PRINT(SOCPM, INFO, "F2A Timeout");
        }
    } else {
        /* This condition is not expected to occur*/
        NT_LOG_PRINT(SOCPM, CRIT, "F2A Timeout when f2a_asserted is FALSE");
    }

    wifi_fw_ext_f2a_signal_deassert();

    /** If IMPS sleep was registered while F2A asserted, restart the IMPS
     * entry sequence.
     */
#ifndef SUPPORT_IMPS_IMPROVEMENTS
    if (nt_is_imps_registered()) {
        nt_set_reset_delayed_imps(FALSE);
        nt_send_imps_enter_cmd(FALSE);
    }
#endif /* SUPPORT_IMPS_IMPROVEMENTS */
}
/**
 * @brief   Disables A2F interrupt service and changes A2F to level trigger
 *
 * @param   none
 * @return  none
 */
void aon_ext_wakeup_set_lvl_trigger(void)
{
    uint32_t en_ext_int = 0;

    // Disable NVIC interrupts
    en_ext_int = NT_REG_RD(NVIC_ISER1);
    en_ext_int &= ~A2F_ASSERT_INTR_NVIC1_MASK;
    NT_REG_WR(NVIC_ISER1, en_ext_int);

    en_ext_int = NT_REG_RD(NVIC_ISER3);
    en_ext_int &= ~A2F_DEASSERT_INTR_NVIC3_MASK;
    NT_REG_WR(NVIC_ISER3, en_ext_int);

    // Set to Level Triggers
    HWIO_OUTXF(SEQ_WCSS_PMU_OFFSET, NEUTRINO_PMU_PRONTO_LP_FRODO_PMU_AON_LIC_INT_POS_EDGE_EN,
               EXT_WAKEUP_INTR_POS_EDGE_EN, 0);

    HWIO_OUTXF(SEQ_WCSS_PMU_OFFSET, NEUTRINO_PMU_PRONTO_LP_FRODO_PMU_AON_LIC_INT_NEG_EDGE_EN,
               EXT_WAKEUP_INTR_NEG_EDGE_EN, 0);
}

/*
 * @brief  Initialize external wakeup interrupts needed for A2F
 * @param          : NONE
 * @return         : NONE
 *
 */
void init_aon_ext_wakeup_int(void)
{
    uint32_t en_ext_int = 0;
    
    if(_socpm_mutex == NULL){
        /*_socpm_mutex should be created before enable assert interrupt*/
        _socpm_mutex = xSemaphoreCreateMutex();

        // Enable ext wakeup interrupt and ext wakeup pos edge interrupt
        HWIO_OUTX2F(SEQ_WCSS_PMU_OFFSET, NEUTRINO_PMU_PRONTO_LP_FRODO_PMU_AON_LIC_INT_EN, EXT_WAKEUP_INTR_EN,
                    EXT_WAKEUP_POS_EDGE_DETECT_INTR_EN, 1, 1);

        // Set external wakeup interrupt polarity to active low
        HWIO_OUTXF(SEQ_WCSS_PMU_OFFSET, NEUTRINO_PMU_PRONTO_LP_FRODO_PMU_AON_LIC_INT_POL, EXT_WAKEUP_INTR_POL, 0);

        // Enable external wakeup pos edge interrupt
        HWIO_OUTXF(SEQ_WCSS_PMU_OFFSET, NEUTRINO_PMU_PRONTO_LP_FRODO_PMU_AON_LIC_INT_POS_EDGE_EN,
                EXT_WAKEUP_INTR_POS_EDGE_EN, 1);

        // Enable external wakeup neg edge interrupt
        HWIO_OUTXF(SEQ_WCSS_PMU_OFFSET, NEUTRINO_PMU_PRONTO_LP_FRODO_PMU_AON_LIC_INT_NEG_EDGE_EN,
                EXT_WAKEUP_INTR_NEG_EDGE_EN, 1);

        // External wakeup interrupt ack generation disable
        HWIO_OUTXF(SEQ_WCSS_PMU_OFFSET, NEUTRINO_PMU_PRONTO_LP_FRODO_PMU_AON_LIC_INT_ACK_EN, EXT_WAKEUP_INTR_ACK_EN, 0);

        // External wakeup interrupt sticky disable
        HWIO_OUTXF(SEQ_WCSS_PMU_OFFSET, NEUTRINO_PMU_PRONTO_LP_FRODO_PMU_AON_LIC_INT_STICKY_EN, EXT_WAKEUP_INTR_STICKY_EN,
                0);

        // Enable NVIC interrupts
        en_ext_int = NT_REG_RD(NVIC_ISER1);
        en_ext_int |= A2F_ASSERT_INTR_NVIC1_MASK;
        NT_REG_WR(NVIC_ISER1, en_ext_int);

        en_ext_int = NT_REG_RD(NVIC_ISER3);
        en_ext_int |= A2F_DEASSERT_INTR_NVIC3_MASK;
        NT_REG_WR(NVIC_ISER3, en_ext_int);
    }
    // xSemaphoreGive(_socpm_mutex);
}

/*
 * @brief  Send F2A pulse on cold boot if A2F not asserted
 * @param          : NONE
 * @return         : NONE
 *
 */
void wifi_fw_ext_cold_boot_f2a_signal(void)
{
    uint8_t a2f_stat =
        HWIO_INXF(SEQ_WCSS_PMU_OFFSET, NEUTRINO_PMU_PRONTO_LP_FRODO_PMU_AON_LIC_INT_STAT, EXT_WAKEUP_INTR_STAT_RAW);

    /** If wakeup was due to A2F assertion, F2A pulse would be sent by the A2F
     * ISR. So, F2A need not be sent when A2F was asserted on cold boot.
     */
    if (0 == a2f_stat) {
        wifi_fw_ext_f2a_pulse(F2A_SHORT_REASON_RING_TX_RX);
    } else {
        NT_LOG_PRINT(SOCPM, WARN, "Cold boot with A2F asserted");
    }
}

/*
 * @brief  Enable/disable F2A indication on TWT wakeup
 * @param  enable_f2a -> enable/disable
 * @return : NONE
 *
 */
void configure_twt_wake_send_f2a(uint8_t enable_f2a)
{
    g_socpm_struct.twt_wake_send_f2a = enable_f2a;
}

/*
 * @brief  Get configuration of F2A indication on TWT wakeup
 * @param  : NONE
 * @return uint8_t -> Whether F2A indication on TWT wakeup is enabled
 *
 */
bool get_twt_wake_send_f2a_configuration(void)
{
    return g_socpm_struct.twt_wake_send_f2a;
}

/*
 * @brief  Enable/disable F2A assert for testing with FermionApp
 * @param  enable_assert -> enable/disable
 * @return bool -> operation successful/failed
 *
 */
bool f2a_enable_disable_assert(uint8_t enable_assert)
{
    bool result = TRUE;
    if ((0 == enable_assert) || (1 == enable_assert)) {
        g_socpm_struct.f2a_assert_enabled = enable_assert;
    } else {
        NT_LOG_PRINT(SOCPM, ERR, "F2A assert enable/disable: Invalid argument %d", enable_assert);
        result = FALSE;
    }
    return result;
}

#ifdef CONFIG_QAT_POWERSAVE_DEMO
void spi_set_ext_wakeup_flag(void)
{
    ext_wakeup_flag = TRUE;
}

bool spi_is_ext_wakeup(void)
{
	return ext_wakeup_flag;
}

void spi_clear_ext_wakeup_flag(void)
{
	ext_wakeup_flag = false;
	NT_LOG_PRINT(SOCPM, INFO, "External wakeup flag cleared");
}
#endif /* CONFIG_QAT_POWERSAVE_DEMO */


/*
 * @brief  ISR handler for A2F(external wakeup interrupt).
 *  When configured for edge triggered, this interrupt is triggered on the
 *  asserting edge of A2F signal on Fermion which is the falling edge with
 *  POL=0 and rising edge with POL=1. For Fermion use case, POL is set to 0,
 *  and this interrupt is used to detect A2F assertion from Apps.
 * @param          : NONE
 * @return         : NONE
 *
 */
void __attribute__((section(".after_ram_vectors"))) aon_a2f_assert_isr_handler(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    xSemaphoreTakeFromISR(_socpm_mutex, &xHigherPriorityTaskWoken);
    uint32_t qcspi_sanity, qcspi_status;
    bool qcspi_ready = TRUE;
    bool send_f2a_pulse = TRUE;

    // Clear the interrupt
    HWIO_OUTXF(SEQ_WCSS_PMU_OFFSET, NEUTRINO_PMU_PRONTO_LP_FRODO_PMU_AON_LIC_INT_CLR, EXT_WAKEUP_INTR_CLR, 1);
    HWIO_OUTXF(SEQ_WCSS_PMU_OFFSET, NEUTRINO_PMU_PRONTO_LP_FRODO_PMU_AON_LIC_INT_CLR, EXT_WAKEUP_INTR_CLR, 0);

    g_socpm_struct.a2f_asserted = TRUE;
    g_socpm_struct.host_supports_a2f = TRUE;
    NT_LOG_PRINT(SOCPM, INFO, "A2F assert");
#ifdef CONFIG_QAT_POWERSAVE_DEMO
    /* Mark that 730 was woken by host.*/
    spi_set_ext_wakeup_flag();
    pm_set_powersave_policy(gdevp, PS_POLICY_NOT_ALLOWED_SLEEP);
#endif /* CONFIG_QAT_POWERSAVE_DEMO */
#ifdef SUPPORT_SWTMR_TO_WKUP_FROM_BMPS
#if 1
    if ((nt_socpm_status() > 0) && (PM_STRUCT *)gdevp->pPmStruct != NULL &&
        (PM_GET_RRI_STATE((PM_STRUCT *)gdevp->pPmStruct) == PM_RRI_MAC_DOWN_MCUSLP) &&
        ((PM_STRUCT *)(gdevp->pPmStruct))->pm_type == PM_MODE_BMPS) {
        NT_LOG_PRINT(SOCPM, CRIT, "send pm");
        PM_SET_SLEEP_EXIT_REASON((PM_STRUCT *)gdevp->pPmStruct, EXIT_REASON_EXT_INT);
        // _socpm_slptmr_off();
        // nt_send_pm_mode_cmd(0);
        nt_bmps_wakeup_callback(EXIT_REASON_EXT_INT);
    }
#endif

#endif /* SUPPORT_SWTMR_TO_WKUP_FROM_BMPS */
    // Deassert F2A if it was asserted
    if (TRUE == g_socpm_struct.f2a_asserted) {
        if (g_socpm_struct.f2a_assert_enabled) {
            qurt_timer_stop_frm_isr(g_socpm_struct.f2a_timer, pdFALSE);

            wifi_fw_ext_f2a_signal_deassert();
            /** Account for APPS limitations on minimum interval between F2A
             * de-assert and F2A pulse.
             */
            if (g_socpm_struct.inter_f2a_interval_us) {
                hres_timer_us_delay(g_socpm_struct.inter_f2a_interval_us);
            }
        } else {
            g_socpm_struct.f2a_asserted = FALSE;
            send_f2a_pulse = FALSE;
        }
    }

#if defined(SUPPORT_QCSPI_SLAVE)
    // Check QCSPI status. If not good, try to recover
    qcspi_sanity = HWIO_INX(SEQ_WCSS_QCSPI_SLAVE_OFFSET, QCSPI_SLAVE_QCSPI_SLAVE_R_SPI_SLAVE_SANITY);
    qcspi_status =
        (HWIO_INX(SEQ_WCSS_QCSPI_SLAVE_OFFSET, QCSPI_SLAVE_QCSPI_SLAVE_R_SPI_SLAVE_STATUS)) & QCSPI_STATUS_CHECK_MASK;
    if ((QCSPI_EXPECTED_SANITY != qcspi_sanity
#ifdef HOST_APP_CONFIG_WAR
         && QCSPI_EXPECTED_SANITY_FA != qcspi_sanity
#endif /* HOST_APP_CONFIG_WAR */
         ) ||
        (QCSPI_EXPECTED_STATUS != qcspi_status
#ifdef HOST_APP_CONFIG_WAR
         && QCSPI_EXPECTED_STATUS_FA != qcspi_status
#endif /* HOST_APP_CONFIG_WAR */
         )) {
        NT_LOG_PRINT(SOCPM, WARN, "QCSPI sanity = 0x%X, status = 0x%X", qcspi_sanity, qcspi_status);
        qcspi_ready = FALSE;
        // QCSPI in bad state. Attempt recovery
        qcspi_slv_init();

        qcspi_sanity = HWIO_INX(SEQ_WCSS_QCSPI_SLAVE_OFFSET, QCSPI_SLAVE_QCSPI_SLAVE_R_SPI_SLAVE_SANITY);
        qcspi_status = (HWIO_INX(SEQ_WCSS_QCSPI_SLAVE_OFFSET, QCSPI_SLAVE_QCSPI_SLAVE_R_SPI_SLAVE_STATUS)) &
                       QCSPI_STATUS_CHECK_MASK;
        if ((QCSPI_EXPECTED_SANITY == qcspi_sanity)
#ifdef HOST_APP_CONFIG_WAR
            && (QCSPI_EXPECTED_SANITY_FA == qcspi_sanity)
#endif /* HOST_APP_CONFIG_WAR */
            && (QCSPI_EXPECTED_STATUS == qcspi_status)
#ifdef HOST_APP_CONFIG_WAR
            && (QCSPI_EXPECTED_STATUS_FA == qcspi_status)
#endif /* HOST_APP_CONFIG_WAR */
        ) {
            qcspi_ready = TRUE;
            NT_LOG_PRINT(SOCPM, WARN, "QCSPI recovered in A2F ISR");
        }
    }
#endif /* SUPPORT_QCSPI_SLAVE */

    // Send F2A only if QCSPI is ready
    if (TRUE == qcspi_ready) {
        if (send_f2a_pulse) {
            wifi_fw_ext_f2a_pulse(F2A_SHORT_REASON_A2F_RESP);
        }
    } else {
        NT_LOG_PRINT(SOCPM, CRIT, "QCSPI not ready. F2A PULSE NOT SENT");
    }

#ifdef NT_DEBUG
    if (pmIsSOCWakeFromTwtSleep()) {
        NT_LOG_PRINT(SOCPM, CRIT, "Wake from TWT sleep due to A2F");
    }
#endif /* NT_DEBUG */
    xSemaphoreGiveFromISR(_socpm_mutex, &xHigherPriorityTaskWoken);
}

/*
 * @brief  ISR handler for A2F deassert(ext wakeup pos edge detect interrupt)
 *  This interrupt is triggered on the rising edge of the A2F signal on
 *  Fermion irrespective of the POL setting of external wakeup interrupt.
 *  For Fermion use case, this interrupt is used to detect A2F deassertion
 *  from Apps.
 * @param          : NONE
 * @return         : NONE
 *
 */
void __attribute__((section(".after_ram_vectors"))) aon_a2f_deassert_isr_handler(void)
{
    PROF_IRQ_ENTER();

    // Clear the interrupt
    HWIO_OUTXF(SEQ_WCSS_PMU_OFFSET, NEUTRINO_PMU_PRONTO_LP_FRODO_PMU_AON_LIC_INT_CLR,
               EXT_WAKEUP_POS_EDGE_DETECT_INTR_CLR, 1);
    HWIO_OUTXF(SEQ_WCSS_PMU_OFFSET, NEUTRINO_PMU_PRONTO_LP_FRODO_PMU_AON_LIC_INT_CLR,
               EXT_WAKEUP_POS_EDGE_DETECT_INTR_CLR, 0);

    g_socpm_struct.a2f_asserted = FALSE;
    // NT_LOG_PRINT(SOCPM, ERR, "A2F deassert");

    /** If IMPS sleep was registered while A2F/F2A asserted, restart the IMPS
     * entry sequence.
     */
#ifndef SUPPORT_IMPS_IMPROVEMENTS
    if (nt_is_imps_registered()) {
        nt_set_reset_delayed_imps(FALSE);
        nt_send_imps_enter_cmd(TRUE);
    }
#endif /* SUPPORT_IMPS_IMPROVEMENTS */

    PROF_IRQ_EXIT();
}

#else

/*
 * @brief  Disable A2F interrupt when informed wake feature is not enabled
 * @param          : NONE
 * @return         : NONE
 *
 */
void disable_aon_ext_wakeup_int(void)
{
    // Disable ext wakeup interrupt and ext wakeup pos edge interrupt
    HWIO_OUTX2F(SEQ_WCSS_PMU_OFFSET, NEUTRINO_PMU_PRONTO_LP_FRODO_PMU_AON_LIC_INT_EN, EXT_WAKEUP_INTR_EN,
                EXT_WAKEUP_POS_EDGE_DETECT_INTR_EN, 0, 0);
}

#endif /* FIRMWARE_APPS_INFORMED_WAKE */
#endif /* PLATFORM_FERMION */
