#========================================================================
#
#========================================================================

# Add path of header files used in this module
INCLUDE_PATHS += \
	-I"$(SRC_ROOT_DIR)/build/freertos/common/config_files" \
	-I"$(SRC_ROOT_DIR)/build/freertos/common"

# List of objects that can be built by the generic rule in master makefile
OBJECT_LIST += \
 $(OBJECT_DIR)/build/freertos/common/application_code/main.o \
 $(OBJECT_DIR)/build/freertos/common/application_code/neutrino_tasks.o \
 $(OBJECT_DIR)/build/freertos/common/application_code/qurt_signal.o \
 $(OBJECT_DIR)/build/freertos/common/application_code/qurt_pipe.o \
 $(OBJECT_DIR)/build/freertos/common/application_code/qurt_thread.o \
 $(OBJECT_DIR)/build/freertos/common/application_code/qurt_timer.o \
 $(OBJECT_DIR)/build/freertos/common/application_code/qurt_utils.o

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
