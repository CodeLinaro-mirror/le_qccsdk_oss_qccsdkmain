#========================================================================
#
#========================================================================*/

# Add path of header files used in this module
INCLUDE_PATHS += \
    -I"$(SRC_ROOT_DIR)/fermion_ioe_pbl/common" \
    -I"$(SRC_ROOT_DIR)/fermion_sbl/common" \
    -I"$(SRC_ROOT_DIR)/fermion_sbl/sbl_auth/inc" \
    -I"$(SRC_ROOT_DIR)/fermion_sbl/sbl_auth/rollback" \
    -I"$(SRC_ROOT_DIR)/fermion_ioe_pbl/system" \
    -I"$(SRC_ROOT_DIR)/fermion_ioe_pbl/elf_loader" \
    -I"$(SRC_ROOT_DIR)/fermion_ioe_pbl/debug" \
    -I"$(SRC_ROOT_DIR)/fermion_ioe_pbl/sbl_ota" \
    -I"$(SRC_ROOT_DIR)/fermion_ioe_pbl/boot_handler" \
    -I"$(SRC_ROOT_DIR)/fermion_ioe_pbl/pbl_patch" \
    -I"$(SRC_ROOT_DIR)/fermion_ioe_pbl/pbl_share" \
    -I"$(SRC_ROOT_DIR)/fermion_ioe_pbl/src" \
    -I"$(SRC_ROOT_DIR)/core/hostif/api/wifi" \
    -I"$(SRC_ROOT_DIR)/core/system/inc" \
    -I"$(SRC_ROOT_DIR)/include/qapi/common" \
    -I"$(SRC_ROOT_DIR)/os/osal" \
    -I"$(SRC_ROOT_DIR)/os/freertos/freertos_kernel/include" \
    -I"$(SRC_ROOT_DIR)/os/freertos/freertos_kernel/portable/GCC/ARM_CM4F" \
    -I"$(SRC_ROOT_DIR)/build/freertos/common/config_files" \
    -I"$(SRC_ROOT_DIR)/os/freertos/system/include/uart_cli" \
    -I"$(SRC_ROOT_DIR)/os/freertos/libraries/abstractions/wifi/include" \
    -I"$(SRC_ROOT_DIR)/os/freertos/libraries/c_sdk/standard/common/include" \
    -I"$(SRC_ROOT_DIR)/core/wifi/mlm/include" \
    -I"$(SRC_ROOT_DIR)/hal/inc" \
    -I"$(SRC_ROOT_DIR)/os/freertos/system/include/cmsis" \
    -I"$(SRC_ROOT_DIR)/lib/libc/include"

# List of objects that can be built by the generic rule in master makefile
#OBJECT_LIST += $(OBJECT_DIR)/fermion_sbl/system/nt_gpio.o
OBJECT_LIST += $(OBJECT_DIR)/fermion_sbl/system/nt_bl_uart.o
OBJECT_LIST += $(OBJECT_DIR)/fermion_sbl/system/nt_bl_mem.o
OBJECT_LIST += $(OBJECT_DIR)/fermion_sbl/system/sbl_storage.o
#OBJECT_LIST += $(OBJECT_DIR)/fermion_sbl/src/wifi_fw_sys_img_loader.o
OBJECT_LIST += $(OBJECT_DIR)/fermion_sbl/src/sbl_main.o
OBJECT_LIST += $(OBJECT_DIR)/fermion_sbl/src/pbl_patch.o
OBJECT_LIST += $(OBJECT_DIR)/fermion_ioe_pbl/elf_loader/boot_elf_loader.o
OBJECT_LIST += $(OBJECT_DIR)/fermion_ioe_pbl/elf_loader/boot_elf_mem.o
OBJECT_LIST += $(OBJECT_DIR)/fermion_ioe_pbl/system/nt_bl_rram_dxe.o
OBJECT_LIST += $(OBJECT_DIR)/fermion_ioe_pbl/system/nt_bl_eventhandler.o
OBJECT_LIST += $(OBJECT_DIR)/fermion_ioe_pbl/sbl_ota/rram_fdt.o
OBJECT_LIST += $(OBJECT_DIR)/fermion_ioe_pbl/debug/boot_print.o
OBJECT_LIST += $(OBJECT_DIR)/fermion_ioe_pbl/debug/boot_log.o
OBJECT_LIST += $(OBJECT_DIR)/core/system/sys_src/ferm_qspi.o
OBJECT_LIST += $(OBJECT_DIR)/core/system/sys_src/ferm_flash.o
OBJECT_LIST += $(OBJECT_DIR)/lib/libc/source/safeAPI.o
OBJECT_LIST += $(OBJECT_DIR)/fermion_sbl/sbl_auth/sbl_auth.o
OBJECT_LIST += $(OBJECT_DIR)/fermion_sbl/sbl_auth/rollback/boot_rollback_version.o
OBJECT_LIST += $(OBJECT_DIR)/fermion_sbl/sbl_auth/rollback/boot_rollback_version_impl.o
#if there are any objects to be built using a non-standard rule, add that object to this list and also
#add make rules for building that object
SPECIAL_OBJECT_LIST +=
# To include flash driver
COMPILER_FLAGS += -DCONFIG_NON_OS

# Example rule for special object list
#$(OBJECT_DIR)/core/..../xyz.o: $(SRC_ROOT_DIR)/core/..../xyz.c
#	@echo 'Building file: $<'
#	@mkdir -p $(@D)
#	@echo 'Invoking: GNU ARM Cross C Compiler'
#	arm-none-eabi-gcc $(COMPILER_FLAGS) $(INCLUDE_PATHS) -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -s -c -o  "$@" "$<"
#	@echo 'Finished building: $<'
#	@echo ' '
