
# Add path of header files used in this module
INCLUDE_PATHS +=  \
	-I"$(SRC_ROOT_DIR)/nvm_programmer" \
	-I"$(SRC_ROOT_DIR)/include/qapi/common" \
	-I"$(SRC_ROOT_DIR)/os/osal" \
	-I"$(SRC_ROOT_DIR)/os/freertos/freertos_kernel/include" \
	-I"$(SRC_ROOT_DIR)/os/freertos/freertos_kernel/portable/GCC/ARM_CM4F" \
	-I"$(SRC_ROOT_DIR)/build/freertos/common/config_files" \
	-I"$(SRC_ROOT_DIR)/os/freertos/system/include/uart_cli" \
	-I"$(SRC_ROOT_DIR)/core/system/inc" \
	-I"$(SRC_ROOT_DIR)/os/freertos/libraries/abstractions/wifi/include" \
	-I"$(SRC_ROOT_DIR)/os/freertos/libraries/c_sdk/standard/common/include" \
	-I"$(SRC_ROOT_DIR)/core/wifi/mlm/include" \
	-I"$(SRC_ROOT_DIR)/hal/inc" \
	-I"$(SRC_ROOT_DIR)/os/freertos/system/include/cmsis" \
	-I"$(SRC_ROOT_DIR)/lib/libc/include"

OBJECT_LIST += \
	$(OBJECT_DIR)/nvm_programmer/nvm_programmer.o \
	$(OBJECT_DIR)/nvm_programmer/nvm_rram.o \
	$(OBJECT_DIR)/nvm_programmer/nvm_flash.o \
	$(OBJECT_DIR)/nvm_programmer/uart_print.o \
	$(OBJECT_DIR)/core/system/sys_src/ferm_qspi.o \
	$(OBJECT_DIR)/core/system/sys_src/ferm_flash.o \
	$(OBJECT_DIR)/lib/libc/source/safeAPI.o

#if there are any objects to be built using a non-standard rule, add that object to this list and also
#add make rules for building that object
SPECIAL_OBJECT_LIST +=


COMPILER_FLAGS += -DCONFIG_NON_OS
DFU_BUILD := OFF

# Example rule for special object list
#$(OBJECT_DIR)/core/..../xyz.o: $(SRC_ROOT_DIR)/core/..../xyz.c
#	@echo 'Building file: $<'
#	@mkdir -p $(@D)
#	@echo 'Invoking: GNU ARM Cross C Compiler'
#	arm-none-eabi-gcc $(COMPILER_FLAGS) $(INCLUDE_PATHS) -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -s -c -o  "$@" "$<"
#	@echo 'Finished building: $<'
#	@echo ' '
