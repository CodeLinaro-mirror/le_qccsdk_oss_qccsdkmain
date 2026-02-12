/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */


/*///////////////////////////////////////////////////////////////////////////// */

#include "nt_heap_stats.h"
#include "wifi_cmn.h"
#include "fwconfig_cmn.h"
#include "nt_flags.h"
#include "nt_osal.h"
#include "qccx.h"

#include "FreeRTOS.h"
#include "ExceptionHandlers.h"   // nt_global_irq_init()
#if (defined(CONFIG_NT_DEMO))
#include "UART-interrupt-driven-command-console.h"
#endif

#include "tcpip.h"     // tcpip_init()
#include "ping.h"      // ping_init()

#include "nt_socpm_sleep.h"
#include "uart.h"
#include "nt_logger_api.h"

#include "nt_wlan_task_manager.h"
#include "NT_Wfm_Task_Manager.h"
#include "lfs.h"

#include "nt_lfs.h"
#include "nt_heap_stats.h"
#include "nt_wifi_driver.h"         // nt_pdc_driver_init()
//#include "nt_commissioning_app.h"   // client_app_cli_msg_queue()
#include "nt_pmic_driver.h"         // nt_pmic_init()
#include "nt_sys_monitoring.h"

#if defined(IMAGE_FERMION)
#include "wifi_fw_version.h"
#include "binary_descriptor.h"
#include "wlan_power.h"
#endif

#ifdef NT_FN_AWS_MQTT_CLIENT_APP
#include "nt_mqtt.h"
#endif
#include "nt_prng.h"
#if defined(SUPPORT_HIGH_RES_TIMER)
#include "timer.h"
#include "timer_test.h"
#endif


/* devcfg Included Files */
#include "nt_devcfg_structure.h"
#include "nt_devcfg_byte_seq_structure.h"

#ifdef FERMION_CONFIG_HCF
#if defined(SUPPORT_RING_IF)
#include "wifi_fw_table_api.h"
#endif
#include "mib.h"
#endif

#ifdef FTM_MM_MODE_SWITCH_ENABLED
#include "hal_int_sys.h"
#endif

#include "halphy_bdf.h"
#include "qapi_rram.h"

#ifdef SUPPORT_REGULATORY
#include "halphy_regulatory_api.h"
#endif

#ifdef CONFIG_BOARD_QCC730_QSPI_ENABLE
#include "ferm_flash.h"
#endif

#ifdef CONFIG_MPU_ENABLE
#include "ferm_mpu.h"
#endif

#include <stdio.h>

// ----------------------------------------------------------------------
// conditional includes go here
// ----------------------------------------------------------------------

//#include "nt_qspi_api.h"

#include "nt_sys_monitoring.h"
//#include "ping.h"      // ping_init()

//#ifdef NT_FN_WATCHDOG
#include "nt_wdt_api.h"
//#endif //NT_FN_WATCHDOG

#ifdef NT_FN_CC_MGMT
#include "nt_cc_battery_driver.h"  // nt_cc_battery_mgmt_init()
#endif

#ifdef LOG_TO_FILE
#include "logger.h"
#endif

#ifdef USE_FS
#include "lfs_io.h"
#endif

#ifdef NT_FN_MBEDTLS_APP
//#include "nt_mbedtls_app.h"
#endif

#if (defined(CONFIG_NT_DEMO))
#ifdef NT_HOSTLESS_SDK
#include "wifi_app.h"
#endif
#endif

#ifdef NT_GPIO_FLAG
#include "nt_gpio_api.h"
#endif

#ifdef SUPPORT_QCSPI_SLAVE
#include "qcspi_slave_api.h"
#endif //SUPPORT_QCSPI_SLAVE

#if defined(SUPPORT_RING_IF) || defined(SUPPORT_RING_IF_ONLY) 
#include "data_svc_hfc_priv.h"
#endif

#include "wifi_fw_ext_intr.h"

#ifdef IMAGE_FERMION
#include "wifi_fw_internal_api.h"
#endif /* IMAGE_FERMION */

#ifdef NT_FN_PKCS11
#include "iot_pkcs11.h"
#include "core_pkcs11.h"
#endif // NT_FN_PKCS11

#ifdef NT_FN_HW_CRYPTO
#include "nt_crypto.h"
#endif //NT_FN_HW_CRYPTO

#if !defined(MBEDTLS_CONFIG_FILE)
#include "mbedtls/mbedtls_config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#if defined(MBEDTLS_PLATFORM_C)
#include "mbedtls/platform.h"
#endif

#ifdef NT_FN_PDC_
#include "nt_pdc.h"
#include "nt_pdc_driver.h"
#endif //NT_FN_PDC_
#ifdef SUPPORT_FERMION_LOGGER
#include "wifi_fw_logger.h"
#endif //SUPPORT_FERMION_LOGGER

#ifdef NT_FN_CPR
#include "nt_cpr_driver.h"
#endif //NT_FN_CPR

#ifdef NT_FN_INTER_TCP_INTERVAL
#include "../../../../tools/tcp_ip_agent_app/nt_tcp_ip_agent.h"
#endif //NT_FN_INTER_TCP_INTERVAL

#ifdef PHY_POWER_SWITCH
#include "hal_int_modules.h"
#endif

#ifdef FTM_MM_MODE_SWITCH_ENABLED

#if defined(PLATFORM_NT)
#include "neutrino_reg.h"
#define RRAM_APP_MODE_ADR 0x801BF /* RRAM location for Fermion_NT for mode switch */
#elif defined(PLATFORM_FERMION)
#include "fermion_reg.h"
extern uint32_t _ln_FDT_Start_Addr;
#define WLAN_IMG_MODE_OFFSET (0x130) /* offset with in FDT section */
#define RRAM_APP_MODE_ADR ((uint32_t)(&_ln_FDT_Start_Addr) + WLAN_IMG_MODE_OFFSET)
#endif
#if CONFIG_FILE_SYSTEM
#include "fs.h"
#include "littlefs_fs.h"
#include "fw_upgrade_mem.h"
#endif
bool is_jtag_mode(void);
bool nt_get_rram_app_mode(app_mode_id_t * read_app_mode);
bool nt_set_rram_app_mode(app_mode_id_t requested_app_mode);
#endif

#if CONFIG_FTM_MODE
//#if CONFIG_BOARD_QCC730_XPA_AUTO_CTRL_ENABLE
#include "halphy_api.h"
//#endif
#endif

#include "qurt_internal.h"

#ifdef SUPPORT_REGULATORY
extern uint32_t _ln_REGDB_Start_Addr;
extern uint32_t _ln_REGDB_Data_length;
#endif

#ifdef FERMION_CONFIG_HCF
extern uint32_t _ln_INI_Start_Addr;
extern uint32_t _ln_INI_Data_length;
extern uint8_t mibsetint(uint16_t, uint8_t*);
NT_BOOL process_data(uint8_t*,uint16_t);
#endif

#ifdef HALPHY_CBC_SUPPORT
extern void halphy_cbc(void);
#endif /* HALPHY_CBC_SUPPORT */

app_mode_id_t nt_get_app_mode(void);

#ifdef FEATURE_FDI
#include "fdi.h"
#endif /*FEATURE_FDI */

#ifdef FEATURE_FPCI
#include "wifi_fw_pwr_cb_infra.h"
#endif /* FEATURE_FPCI */

#ifdef PLATFORM_FERMION
#include "wifi_fw_pmic_driver.h"
#include "wifi_fw_cpr_driver.h"
#endif /* PLATFORM_FERMION */

#if (CONFIG_QCCSDK_CONSOLE)
#include "qccsdk_console.h"
#endif

#if (CONFIG_FW_UPGRADE)
#include "qapi_firmware_upgrade.h"
#endif

#include "pka.h"

/*******************************************************************************
 ******************************************************************************/

#if (NT_CHIP_VERSION==1) || defined(PLATFORM_FERMION)
// chip runs at 60MHz

uint32_t SystemCoreClock = 60000000u; /* this should be chip specific clock frequency value */

#endif


#if(NT_CHIP_VERSION==2)
// emulation platform at 15MHz
uint32_t SystemCoreClock = 15000000u; /* this should be chip specific clock frequency value */
#endif

volatile uint64_t global_sleep_time;

/* Run time stats */
uint32_t run_time_stat_init_time = 0;
uint8_t sys_stats_start = 0;

int mcu_sleep_force = 0; /*set to 1 before call sleep register to enable MCU sleep*/
int rri_force_wakeup = 0;
uint8_t pbl_log_buff[256];
lfs_t lfs_init;

#if CONFIG_FTM_MODE
app_mode_id_t app_mode = APP_MODE_FTM; /* default application mode in RAM */
#else
app_mode_id_t app_mode = APP_MODE_MM; /* default application mode in RAM */
#endif
#ifdef SUPPORT_RING_IF
extern void wlan_send_mode_event(void);
#endif /* SUPPORT_RING_IF */

#if defined(IMAGE_FERMION)
const binary_desriptor_t wlan_desriptor_t __attribute__ ((section(".wlan_version_num"))) = { .version = ((WIFI_FW_VER_MAJOR << 24) | (WIFI_FW_VER_MINOR << 16) | (WIFI_FW_VER_COUNT)) };
#endif

/*******************************************************************************
 ******************************************************************************/
extern int8_t nt_rram_read(uint32_t address, void *rdata, uint32_t length);
extern int8_t nt_rram_write(uint32_t dst, const void *wdata, uint32_t length);
extern qbool_t rram_udpart_init();
#ifdef FERMION_SILICON
extern uint32_t UART_Send_direct(char *txbuf,uint32_t buflen);
#define UART_SEND_DIRECT(str)   UART_Send_direct((str),strlen(str))
#else
#define UART_SEND_DIRECT(str)
#endif

#if (CONFIG_QCCSDK_DEMO)
void qccsdk_start_app_task(void);
#endif

#ifdef ENABLE_SEGGER_SYSTEMVIEW
extern void SEGGER_SYSVIEW_Conf(void);
#endif

#ifdef FTM_OVER_UART
extern void ftm_task_init(void);
#endif
#if defined(FTM_OVER_UART) || defined(CONFIG_RTT_VIEW_CLI)
extern void SEGGER_RTT_Init (void);
#endif

#if (CONFIG_QCCSDK_CONSOLE)
static void shell_init (void)
{
    qccsdk_console_init();
#if (CONFIG_WIFI_SHELL)
    extern void wifi_shell_init (void);
    wifi_shell_init();
#endif
#if (CONFIG_PLATFORM_SHELL)
    extern void platform_shell_init (void);
    platform_shell_init();
#endif

#if (CONFIG_I2CM_SHELL)
	extern void i2cm_shell_init (void);
	i2cm_shell_init();
#endif

#if (CONFIG_GPIO_SHELL)
		extern void gpio_shell_init (void);
		gpio_shell_init();
#endif

#if (CONFIG_UART_SHELL)
 	extern void uart_shell_init (void);
	uart_shell_init();
#endif

#if (CONFIG_NET_SHELL)
    extern void net_shell_init (void);
    net_shell_init();
#endif

#if (CONFIG_FLASH_SHELL)
    extern void flash_shell_init (void);
    flash_shell_init();
#endif

#if (CONFIG_LOWPOWER_SHELL)
    extern void lowpower_shell_init(void);
    lowpower_shell_init();
#endif

#if (CONFIG_PROF_SHELL)
	extern void prof_shell_init(void);
	prof_shell_init();
#endif

#if (CONFIG_SIGMA_TRAFFIC)
    extern void wificert_shell_init (void);
    wificert_shell_init();
#endif

#if (CONFIG_UNITTEST_SHELL)
	extern void unittest_shell_init(void);
	unittest_shell_init();
#endif

#if (CONFIG_FS_SHELL)
    extern void fs_shell_init(void);
    fs_shell_init();
#endif
#if (CONFIG_RNG_TEST)
    extern void rng_shell_init(void);
    rng_shell_init();
#endif
}
#endif

#if CONFIG_PBL_PREES_RESET_FOR_DTIM
void early_reset()
{
    uint32_t value = NT_REG_RD(QWLAN_PMU_CFG_IO_BASED_STRAP_OVERRIDE_VALUE_REG);
    if (!(value & QWLAN_PMU_CFG_IO_BASED_STRAP_OVERRIDE_VALUE_FORCE_NPS_PD_ON_MASK)) {
        NT_REG_WR(QWLAN_PMU_BOOT_STRAP_CONFIG_SECURE_REG, 0x63887466);
        value = 0;
        value |= (3 << QWLAN_PMU_CFG_IO_BASED_STRAP_OVERRIDE_VALUE_CFG_AON_TESTBUS_ENABLE_OFFSET);
        value |= (1 << QWLAN_PMU_CFG_IO_BASED_STRAP_OVERRIDE_VALUE_CFG_FORCE_NPS_PD_ON_OFFSET);
        NT_REG_WR(QWLAN_PMU_CFG_IO_BASED_STRAP_OVERRIDE_VALUE_REG, value);
        NT_REG_WR(QWLAN_PMU_SYS_SOFT_RESET_REQ_REG, QWLAN_PMU_SYS_SOFT_RESET_REQ_SYS_SOFT_RESET_REQ_MASK);
    }
}
#endif

int main(
        int __attribute__((__unused__))argc,
        char __attribute__((__unused__))*argv[])
{
    BaseType_t ret_val;
    extern int BMPS_LIST, WUR_LIST;
    BMPS_LIST =  WUR_LIST = -1;
    uint8 is_ftm = 0;
    extern uint32_t bdf_addr;

#if (CONFIG_FTM_MODE==1)
    is_ftm = 1;
#endif

#ifdef CONFIG_MPU_ENABLE
	ferm_mpu_init();
#endif

#ifndef CONFIG_UART_SHELL
	uart_init();
#endif

#if defined(FTM_OVER_UART) || defined(CONFIG_RTT_VIEW_CLI)
    SEGGER_RTT_Init();
#endif
#if CONFIG_FTM_MODE
    printf("Build FTM image date and time: %s - %s\n", __DATE__, __TIME__);
    printf("crm num: %d.\n", CRM_BUILD_NUM);
#endif
#ifdef FTM_OVER_UART
    ftm_task_init();
#else
#ifdef ENABLE_SEGGER_SYSTEMVIEW
    SEGGER_SYSVIEW_Conf();
    //SEGGER_SYSVIEW_Start();
#endif

    pka_init(&g_pka_ctxt);
    rram_udpart_init();

#if (CONFIG_QCCSDK_DEMO)
    #if (CONFIG_QCCSDK_CONSOLE)
        shell_init();
    #endif
#else
    #if (defined(CONFIG_NT_DEMO))
        vUARTCommandConsoleStart();
        vRegisterCLICommands();
	#endif
#endif
#endif

#if (CONFIG_QAT)
	extern void qat_module_init (void);
	qat_module_init();
#endif

#ifdef SUPPORT_FERMION_LOGGER
fw_logger_init();
#endif //SUPPORT_FERMION_LOGGER

#ifdef NT_FN_HW_CRYPTO
#ifndef CONFIG_FTM_MODE
    if(app_mode != APP_MODE_FTM){
        nt_secure_ip_pwr_status();
        nt_enable_device_irq(CC_intr);
        nt_prng_init();
    }
#endif
#endif //NT_FN_HW_CRYPTO
#ifdef FTM_MM_MODE_SWITCH_ENABLED
#ifndef BOOT_TO_FTM
    bool status;
    /* get the application mode [FTM/MM] and base on that do initialization */
    status = nt_get_rram_app_mode(&app_mode);

    /* it may happen that after factory the RRAM may not be initilized with zero and in that case we may
       read wrong APP MODE */

    if((app_mode != APP_MODE_MM) && (app_mode != APP_MODE_FTM))
    {
        /* app mode must be MM or FTM only */
        status = nt_set_rram_app_mode(APP_MODE_MM);
        app_mode = APP_MODE_MM;
        if(status == false)
        {
            /* write to rram for application mode failed next boot after
             * power cycle may be in FTM mode */
            nt_dbg_print("very first time write for MM failed taking as MM");
        }
    }

    if(status == false)
    {
        /* read of app mode from rram failed, use MM as default */
        app_mode = APP_MODE_MM;
    }

    /* if application mode is FTM change RRAM location so that next boot by default is in MM only */
    if(app_mode == APP_MODE_FTM)
    {
        status = nt_set_rram_app_mode(APP_MODE_MM);
        if(status == false)
        {
            /* write to rram for application mode failed next boot after
             * power cycle may be in FTM mode */
            nt_dbg_print("FTM to MM default boot failed");
        }
    }
#endif
#endif

#ifdef FERMION_CONFIG_HCF
    //extern uint8_t* ptr_2_hcf;
    //static fermion_defaults_t ini_param;
    uint8_t* ptr = (uint8_t*)&_ln_INI_Start_Addr;
    uint16_t length = (uint16_t)LEN(ptr);
    length += 4;        //without this the last 8 bytes won't be parsed
    if(process_data((uint8_t*)&_ln_INI_Start_Addr/*ptr_2_hcf*/, /*40a6*/length)) //Configuration data
        {
            WLAN_DBG0_PRINT("HCF Initialized!!");
        }
        else
        {
        WLAN_DBG0_PRINT("HCF not Present");
            nt_devcfg_parse();              // devcfg parser function call to fill the common devcfg structure
            nt_devcfg_byte_seq_parse();
        }
#else
    /* dev cfg should be the first to get initialized */
    nt_devcfg_parse();      // devcfg parser function call to fill the common devcfg structure
    nt_devcfg_byte_seq_parse(); // byte_sequence :: devcfg parser function call to fill the common devcfg structure
#endif

#ifdef FEATURE_FDI
    fdi_init();
    fdi_reg_all_nodes();
#endif /* FEATURE_FDI */

#if FPCI_DEBUG
    fpci_register_test_cb();
#endif /* FPCI_DEBUG */
#ifdef IMAGE_FERMION
#ifndef FERMION_QTIMER_WAR
    wifi_fw_module_init();

#endif /* FERMION_QTIMER_WAR */
    ppm_common_init();
#endif /* IMAGE_FERMION */
    nt_global_irq_init();

    nt_socpm_init_soc_cfg();

#ifdef NT_NEUTRINO_1_0_SYS_MAC
    nt_pmic_init();
#endif
#ifdef PLATFORM_FERMION
#ifndef EMULATION_BUILD
    wifi_fw_pmic_init(cpr_openloop);
	
#ifndef CBC_CX_VOLTAGE_WAR
    wifi_fw_cpr_init();
#endif /*CBC_CX_VOLTAGE_WAR */

#endif /* EMULATION_BUILD */
#endif /* PLATFORM_FERMION */

    nt_socpm_init();
#ifdef FERMION_SILICON
    UART_Send_direct("nt_socpm_init done...\n", strlen("nt_socpm_init done...\n"));
#endif

#ifdef IMAGE_FERMION
#ifdef FERMION_QTIMER_WAR
        wifi_fw_module_init();

#endif /* FERMION_QTIMER_WAR */
#endif /* IMAGE_FERMION */

    /* Initializes PMU TS and Sleep Clock Cal , 
     * do to be post hres timer init as hres timer APIs are used */
    nt_socpm_secondary_init();
#ifndef PLATFORM_FERMION
   /** On PLATFORM_NT, the external wakeup interrupt is used as WPS button
    * for WPS functionality. On PLATFORM_FERMION, the external wakeup interrupt
    * is used for A2F signal and hence this logic is not needed on Fermion.
    */
    // AON external wake up interrupt
    enable_aon_ext_wakeup_int();
#endif // PLATFORM_FERMION

#ifdef PLATFORM_FERMION
    uint32_t get_val=HW_REG_RD(QWLAN_PMU_DIG_TOP_CFG_REG);
    get_val&=(~QWLAN_PMU_DIG_TOP_CFG_RRAM_32_BIT_LEGACY_WR_MODE_MASK);
    HW_REG_WR(QWLAN_PMU_DIG_TOP_CFG_REG,get_val);
#endif



#ifdef NT_TU_HEAP_STATS
    //We are getting task name from heap-stats. task names is stored in RAM. we are configuring address in this API.
    nt_task_name_table_cfg();
#endif /** NT_TU_HEAP_STATS */

/*WDT Enable only in the production build */

//#ifdef NT_FN_WATCHDOG
    nt_watchdog_timer_init();
//#endif //NT_FN_WATCHDOG

#ifdef NT_SOPCM_CHANGE
    enum error_no reason=wifi_pdc_init();
    if(reason != pdc_init_success )
        WLAN_DBG0_PRINT("WIFI Resource Creation failed");
#else
    unpa_init();
    nt_pdc_driver_init();
#ifdef NT_FN_PDC_
    nt_pdc_init();
#endif
#endif

#ifdef NT_FN_CC_MGMT
    nt_cc_battery_mgmt_init();
#endif

#ifdef NT_FN_CPR
    if((uint8_t) nt_socpm_cpr_flag_state_get ( CPR_EN ))
    nt_cpr_init();
#endif //NT_FN_CPR

#ifdef NT_FN_SYSMON
    nt_sysmon_threshold_init();// initializing the thresholds for voltage and temperature
#endif
#ifdef NT_FN_LFS
    nt_lfs_init();
#endif

    nt_log_cfg(NULL);

    if(app_mode != APP_MODE_FTM)
    {
#ifndef CONFIG_AMBIENT_POWER_ENABLE
        tcpip_init(NULL, NULL);
#endif
    }

#if ((NT_TST_PING_TOOL) || (CONFIG_NET_SHELL))
        if(app_mode != APP_MODE_FTM)
        {
#ifndef CONFIG_AMBIENT_POWER_ENABLE 
            extern void ping_init(void);
            ping_init();
#endif
        }
#endif

#ifdef NT_GPIO_FLAG
    nt_gpio_init();

#endif

#ifdef SUPPORT_QCSPI_SLAVE
qcspi_slv_init();
#endif //SUPPORT_QCSPI_SLAVE

#if defined(SUPPORT_RING_IF) || defined(SUPPORT_RING_IF_ONLY) 
qcspi_hfc_init();
#endif

#ifdef NT_FN_SPI
        nt_spi_slv_defalut_config();
#if (NT_CHIP_VERSION==2)
        nt_dw_qc_spi_slv_cli(1);
#endif //(NT_CHIP_VERSION==2)
    #endif //NT_FN_SPI
#if (NT_CHIP_VERSION==2)
#ifdef NT_2_FAST_QSPI
        nt_qspi_cfg(NT_CLOCK_MODE_0);
        //nt_qspi_cmd_init();
#endif //NT_2_FAST_QSPI
#ifdef NT_2_FAST_QCSPI
        nt_qcspi_config();
        nt_dw_qc_spi_slv_cli(0);
#endif //NT_2_FAST_QCSPI
#endif //(NT_CHIP_VERSION==2)


    ret_val = nt_create_wlan_task(is_ftm);
    if (ret_val == nt_fail)
    {
        NT_LOG_MLM_CRIT("wlan task/queue creation failed", 0,0,0);
        return ret_val;
    }

#ifdef CONFIG_WMI_EVENT
    ret_val = nt_create_wlan_evt_task(is_ftm);
    if (ret_val == nt_fail)
    {
        return ret_val;
    }
#endif

#if (defined(CONFIG_NT_DEMO))
    ret_val = nt_create_wifi_manager_task();//nt_create_wifi_manager_task(void)
    if (ret_val == nt_fail)
    {
        NT_LOG_MLM_CRIT("wfm task/queue creation failed", 0,0,0);
        return ret_val;
    }
#endif

#if (defined(CONFIG_NT_DEMO))
#ifdef NT_HOSTLESS_SDK
        ret_val = nt_app_init();
        if (ret_val == nt_fail)
        {
            nt_dbg_print("wifi app task creation failed");
            return ret_val;
        }
#endif
#endif

#if (defined(CONFIG_NT_DEMO))
#ifdef NT_HOSTED_SDK
        if(app_mode != APP_MODE_FTM)
        {
            AT_commands_task();
            AT_commands_msg_queue();
        }
    #endif // NT_HOSTED_SDK
#endif

#if (defined(CONFIG_NT_DEMO))
        // TCP client app
#ifdef  NT_FN_COMMISSIONING_APP
        if(app_mode != APP_MODE_FTM)
        {
            // OnboardingApp_cont_timer();
            client_app_cli_msg_queue();
        }
#endif
#endif

#if (defined(CONFIG_NT_DEMO))
    if(app_mode != APP_MODE_FTM)
    {
#ifdef NT_FN_MBEDTLS_APP
            //mbedtls_cli_msg_queue();
            //nt_mbedtls_app();
#else
       /* set mbedtls allocation methods */
       mbedtls_platform_set_calloc_free(&pvPortCalloc, &vPortFree);
#endif
     }
#endif

#if (defined(CONFIG_NT_DEMO))
#ifdef NT_FN_AWS_MQTT_CLIENT_APP

        mqtt_publish_cli_msg_queue();
        nt_mqtt_pub_app();
#endif
#endif

#if (defined(CONFIG_NT_DEMO))
#ifdef NT_FN_PKCS11
        xInitializePKCS11(  );
#endif   // NT_FN_PKCS11

#ifdef NT_FN_PKCS11
    if(app_mode != APP_MODE_FTM){
        CK_RV xInitializePKCS11( void );
    }
#endif   // NT_FN_PKCS11
#endif

#if (defined(CONFIG_NT_DEMO))
#ifdef NT_FN_INTER_TCP_INTERVAL
        /* To create inter TCP_task if enabled through sys_config  */
        nt_app_inter_tcp_uplink_traffic();
#endif // NT_FN_INTER_TCP_INTERVAL
#endif
    extern void libwifi_kconfig_install(void);
    libwifi_kconfig_install();

    halphy_bdf_init((uint32_t *)bdf_addr);
    if(app_mode == APP_MODE_FTM)
    {
        halphy_bdf_cached_bdf_init();
    }

#ifdef SUPPORT_REGULATORY
    halphy_regdb_init((uint8_t *)&_ln_REGDB_Start_Addr);
#endif /* SUPPORT_REGULATORY */

#if CONFIG_FTM_MODE
//#if CONFIG_BOARD_QCC730_XPA_AUTO_CTRL_ENABLE
    halphy_xfem_init();
//#endif
#endif

#ifdef HALPHY_CBC_SUPPORT
    /* cold boot calibration call */
    halphy_cbc();
    nt_socpm_nop_delay(4000);
#endif /* HALPHY_CBC_SUPPORT */

    wifi_fw_pmic_init(cpr_closeloop);
    //nt_socpm_nop_delay(4000);

#ifdef CBC_CX_VOLTAGE_WAR
    wifi_fw_cpr_init();
#endif // CBC_CX_VOLTAGE_WAR

#ifdef IMAGE_FERMION
    wifi_fw_gpio_init(FALSE);
#endif /* IMAGE_FERMION */

#ifdef PLATFORM_FERMION
#ifdef FIRMWARE_APPS_INFORMED_WAKE
    /* Initialize A2F interrupt */
    init_aon_ext_wakeup_int();
#ifdef SUPPORT_RING_IF
    /* F2A signal on cold boot */
    wifi_fw_ext_cold_boot_f2a_signal();
#endif
#else
    /** Disable the external wakeup interrupt when the feature is not enabled
    * as it prevents SOC from entering sleep state.
    */
    disable_aon_ext_wakeup_int();

#ifdef SUPPORT_RING_IF
    wifi_fw_f2a_interrupt();
#endif
#endif /* FIRMWARE_APPS_INFORMED_WAKE */


#endif /* PLATFORM_FERMION */

#ifdef SUPPORT_RING_IF
    /* application init done send mode event FTM or MM */
    wlan_send_mode_event();
#endif /* SUPPORT_RING_IF */

#ifdef PHY_POWER_SWITCH
    /* In FTM configure PHY in RXB_LISTEN mode */
    if(app_mode == APP_MODE_FTM)
    {
		hal_phy_power_ftm_switch_to_listen();
    }
#endif /* PHY_POWER_SWITCH */
#if CONFIG_WLAN_QAPI
    extern int wlan_qapi_init (void);
    wlan_qapi_init();
#endif
#if (CONFIG_FW_UPGRADE)
    /* check active FWD to invalidate trial fwd if need */
    extern qapi_Status_t qapi_Fw_Upgrade_Verify_FWD();
    qapi_Fw_Upgrade_Verify_FWD();
#endif
#if CONFIG_FILE_SYSTEM
    extern int init_fs(void);
    init_fs();
#endif
#if (CONFIG_QCCSDK_DEMO)
    //create app task to call app_main()
    qccsdk_start_app_task();
#endif

    UART_SEND_DIRECT("Starting scheduler\r\n");
        qurt_kernel_start();

        // forever
        for (;;) {}
    return 0;
}

#if (CONFIG_QCCSDK_DEMO)
static void qccsdk_start_app_main(void __attribute__((__unused__))*pvParameters)
{
#ifdef CONFIG_BOARD_QCC730_QSPI_ENABLE
    drv_flash_init();
#endif

    UART_SEND_DIRECT("qccsdk_start_app_main\r\n");
    UART_SEND_DIRECT("Calling app_main()\r\n");
    extern void app_main(void);
    app_main();
    UART_SEND_DIRECT("Returned from app_main()\r\n");
    vTaskDelete(NULL);
}

void qccsdk_start_app_task (void)
{
    BaseType_t ret_val;
    TaskHandle_t    wfm_hnd = NULL;

    extern void app_init(void);
    app_init();
    ret_val = nt_qurt_thread_create(qccsdk_start_app_main,
            "qmain", WIFI_MNGR_TASK_STACK_SIZE, NULL, WIFI_MNGR_TASK_PRIORITY, &wfm_hnd);

    if(ret_val != pdPASS) {
        nt_osal_thread_delete(wfm_hnd);
        UART_SEND_DIRECT("Failed to start app task\r\n");
    }
}
#endif

    void
    vApplicationDaemonTaskStartupHook(
            void)
    {
    }

#ifndef CONFIG_HEAP_STATISTIC
#ifdef DEBUG_MEM_LEAK
void * pvPortCallocWrapper(size_t xNum, size_t xSize, const char *caller){
    void *ptr = pvPortCalloc(xNum,xSize );
    if (ptr != NULL) {
      char pcWriteBuffer[200];
      snprintf((char *)pcWriteBuffer,sizeof(pcWriteBuffer)-strlen(pcWriteBuffer),"Allocated %u bytes from %s\r\n",xNum * xSize,caller);
        nt_dbg_print(pcWriteBuffer);
    }
    return ptr;
}
#endif  
    void *
    pvPortCalloc(
            size_t xNum,
            size_t xSize)
    {
        void * pvReturn;

        pvReturn = nt_osal_allocate_memory(xNum * xSize);
        if (pvReturn != NULL)
        {
            memset(pvReturn, 0x00, xNum * xSize);
        }
        return pvReturn;
    }
#else
    void *
    __pvPortCalloc(
			size_t xNum,
			size_t xSize)
    {
        void * pvReturn;
        extern void *__pvPortMalloc( size_t size );
        pvReturn = __pvPortMalloc(xNum * xSize);
        if (pvReturn != NULL)
        {
            memset(pvReturn, 0x00, xNum * xSize);
        }
        return pvReturn;
    }
#endif

    void vApplicationStackOverflowHook( TaskHandle_t xTask,
                char * pcTaskName )
        {
            configPRINTF( ( "ERROR: stack overflow with task %s\r\n", pcTaskName ) );
            (void)pcTaskName;
            portDISABLE_INTERRUPTS();

            /* Unused Parameters */
            ( void ) xTask;

            /* Loop forever */
            for( ; ; )
            {
            }
        }

#ifdef FTM_MM_MODE_SWITCH_ENABLED
/**
  @brief Function to get the application mode from RRAM.

  This function reads the RRAM fixed location which is reserved for mode
  switch variable and gets the value. This function should NOT be used to
  know the application mode any time. This should be used while running main
  application code. To know application mode any time in the code one should
  nt_get_app_mode function.

  @param  pointer to app mode.
  @return true if read is successful
*/

bool nt_get_rram_app_mode(app_mode_id_t * read_app_mode)
{
    if(nt_rram_read(RRAM_APP_MODE_ADR, read_app_mode, 1) == 0)
    {
        /* mode read is successful */
        return(true);
    }
    else
    {
        nt_dbg_print("mode switch read from RRAM failed");
        return(false);
    }
}

/**
  @brief Function to set the application mode from RRAM.

  This function writes the RRAM fixed location which is reserved for mode
  switch variable and sets the value. This function should only be called from
  place were mode switch is needed.

  @param  Application mode.
  @return true if write is successful.
*/

bool nt_set_rram_app_mode(app_mode_id_t requested_app_mode)
{
    int8_t status;
    status = nt_rram_write(RRAM_APP_MODE_ADR, &requested_app_mode, 1);
    if(status != 0)
    {
        nt_dbg_print("mode switch write to RRAM failed");
        return(false);
    }
    else
    {
        return(true);
    }
}

bool is_jtag_mode(void)
{
    uint32_t value;
#if defined(PLATFORM_NT)
    // 3'b000: Functional Mode 3'b001: Scan Mode 3'b010: Mbist Mode 3'b011: RFA Test Mode 3'b100: JTAG Control Mode
    value = HWIO_INXF(SEQ_WCSS_PMU_OFFSET, NEUTRINO_PMU_PRONTO_LP_FRODO_PMU_BOOT_STRAP_CONFIGURATION_STATUS, TEST_STRAP);
    if (value == 0b100)
        return true;
    else
        return false;
#elif defined(PLATFORM_FERMION)
    value = HWIO_INXF(SEQ_WCSS_PMU_OFFSET, NEUTRINO_PMU_PRONTO_LP_FRODO_PMU_CFG_IO_BASED_STRAP_OVERRIDE_VALUE, JTAG_MODE);
    return (bool)(value);
#endif
}

#endif

/**
  @brief Function to get the application mode.

  after successful boot the software can call this API to get to know
  on which mode the it has booted.

  @param  None.
  @return Application mode.
*/

app_mode_id_t nt_get_app_mode(void)
{
    return(app_mode);
}
