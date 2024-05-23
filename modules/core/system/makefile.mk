#========================================================================
#
#========================================================================*/

# Add path of header files used in this module
INCLUDE_PATHS += \
    -I"$(SRC_ROOT_DIR)/core/system/inc" \
	-I"$(SRC_ROOT_DIR)/core/common"
#	-I"$(SRC_ROOT_DIR)/core/system/hcal/inc"

# List of objects that can be built by the generic rule in master makefile
OBJECT_LIST += \
 $(OBJECT_DIR)/core/system/sys_src/ext_rtc_intr.o    \
 $(OBJECT_DIR)/core/system/sys_src/nt_gpio.o    \
 $(OBJECT_DIR)/core/system/sys_src/ferm_i2c.o    \
 $(OBJECT_DIR)/core/system/sys_src/nt_lfs.o    \
 $(OBJECT_DIR)/core/system/sys_src/nt_logger.o    \
 $(OBJECT_DIR)/core/system/sys_src/nt_mem.o    \
 $(OBJECT_DIR)/core/system/sys_src/nt_minimum_code.o    \
 $(OBJECT_DIR)/core/system/sys_src/wifi_fw_dbg_nodes.o    \
 $(OBJECT_DIR)/core/system/sys_src/wifi_fw_dbg_infra.o    \
 $(OBJECT_DIR)/core/system/sys_src/wifi_fw_pwr_cb_infra.o    \
 $(OBJECT_DIR)/core/system/sys_src/wifi_fw_pmic_driver.o    \
 $(OBJECT_DIR)/core/system/sys_src/fdi_rmc.o    \
 $(OBJECT_DIR)/core/system/sys_src/nt_pmic_driver.o    \
 $(OBJECT_DIR)/core/system/sys_src/nt_pmu_drivers.o    \
 $(OBJECT_DIR)/core/system/sys_src/nt_socpm_rtos_api.o    \
 $(OBJECT_DIR)/core/system/sys_src/nt_socpm_sleep.o    \
 $(OBJECT_DIR)/core/system/sys_src/nt_wdt.o    \
 $(OBJECT_DIR)/core/system/sys_src/nt_wifi_driver.o    \
 $(OBJECT_DIR)/core/system/sys_src/wifi_fw_init.o    \
 $(OBJECT_DIR)/core/system/sys_src/qtmr.o    \
 $(OBJECT_DIR)/core/system/sys_src/qurt_rmutex.o    \
 $(OBJECT_DIR)/core/system/sys_src/timer.o    \
 $(OBJECT_DIR)/core/system/sys_src/uart.o    \
 $(OBJECT_DIR)/core/system/sys_src/unpa.o    \
 $(OBJECT_DIR)/core/system/sys_src/unpa_client.o    \
 $(OBJECT_DIR)/core/system/sys_src/unpa_query.o    \
 $(OBJECT_DIR)/core/system/sys_src/unpa_resource.o    \
 $(OBJECT_DIR)/core/system/sys_src/unpa_test.o    \
 $(OBJECT_DIR)/core/system/hcal_src/nt_crypto_arm.o \
 $(OBJECT_DIR)/core/system/hcal_src/nt_dxe_crypto.o \
 $(OBJECT_DIR)/core/system/hcal_src/nt_hmac_sha1.o \
 $(OBJECT_DIR)/core/system/hcal_src/nt_hmac_sha256.o \
 $(OBJECT_DIR)/core/system/hcal_src/nt_kdf.o \
 $(OBJECT_DIR)/core/system/hcal_src/nt_prng.o \
 $(OBJECT_DIR)/core/system/hcal_src/nt_prng_integration.o \
 $(OBJECT_DIR)/core/system/hcal_src/nt_qcc.o \
 $(OBJECT_DIR)/core/system/hcal_src/nt_qcc_cbc.o \
 $(OBJECT_DIR)/core/system/hcal_src/nt_qcc_ccm.o \
 $(OBJECT_DIR)/core/system/hcal_src/nt_qcc_cmac.o \
 $(OBJECT_DIR)/core/system/hcal_src/nt_qcc_ctr.o \
 $(OBJECT_DIR)/core/system/hcal_src/nt_qcc_ecb.o \
 $(OBJECT_DIR)/core/system/hcal_src/nt_security_ip.o \
 $(OBJECT_DIR)/core/system/hcal_src/nt_sha.o \
 $(OBJECT_DIR)/core/system/sys_src/nt_cpr_driver.o \
 $(OBJECT_DIR)/core/system/sys_src/wifi_fw_logger.o \
 $(OBJECT_DIR)/core/system/sys_src/wifi_fw_ext_intr.o \
 $(OBJECT_DIR)/core/system/sys_src/wifi_fw_cpr_driver.o

ifneq ($(strip $(VARIANT_NAME)), FERMION_QCLI_DEMO)
OBJECT_LIST += \
 $(OBJECT_DIR)/core/system/sys_src/sys_onbd_cfg.o
endif

ifneq ($(strip $(CONFIG_FLASH_XIP_SUPPORT)), true)
COMPILER_FLAGS += -DFLASH_XIP_SUPPORT
endif

ifeq ($(strip $(PLATFORM_FAMILY)), NEUTRINO)
OBJECT_LIST += \
 $(OBJECT_DIR)/core/system/sys_src/nt_qspi.o
endif

ifeq ($(strip $(PLATFORM_FAMILY)), FERMION)
OBJECT_LIST += \
 $(OBJECT_DIR)/core/system/sys_src/qcspi_slave.o \
 $(OBJECT_DIR)/core/system/sys_src/wlan_sleep_clk_cal.o \
 $(OBJECT_DIR)/core/system/sys_src/wifi_fw_pmu_ts_cfg.o \
 $(OBJECT_DIR)/core/system/sys_src/ferm_qspi.o \
 $(OBJECT_DIR)/core/system/sys_src/ferm_flash.o
endif

ifeq ($(strip $(CONFIG_PLATFORM_SHELL)), y)
    OBJECT_LIST += $(OBJECT_DIR)/core/system/sys_src/platform_shell.o
    COMPILER_FLAGS += -DCONFIG_PLATFORM_SHELL=1
endif

ifeq ($(strip $(CONFIG_QTIMER)), y)
OBJECT_LIST += \
 $(OBJECT_DIR)/core/system/sys_src/ferm_qtmr.o
endif

ifeq ($(strip $(CONFIG_PROF)), y)
OBJECT_LIST += \
 $(OBJECT_DIR)/core/system/sys_src/ferm_prof.o
endif

#if there are any objects to be built using a non-standard rule, add that object to this list and also
#add make rules for building that object
SPECIAL_OBJECT_LIST +=

# Example rule for special object list
#$(OBJECT_DIR)/core/..../xyz.o: $(SRC_ROOT_DIR)/core/..../xyz.c
#	@echo 'Building file: $<'
#	@mkdir -p $(@D)
#	@echo 'Invoking: GNU ARM Cross C Compiler'
#	arm-none-eabi-gcc $(COMPILER_FLAGS) $(INCLUDE_PATHS) -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -s -c -o  "$@" "$<"
#	@echo 'Finished building: $<'
#	@echo ' '
