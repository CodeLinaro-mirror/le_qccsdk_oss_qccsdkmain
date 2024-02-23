
# Add path of header files used in this module
INCLUDE_PATHS += \
	-I"$(SRC_ROOT_DIR)/tools/Target_tools/battery_management/coincell/inc" \
	-I"$(SRC_ROOT_DIR)/tools/Target_tools/dev_cfg/inc" \
	-I"$(SRC_ROOT_DIR)/tools/Target_tools/ip_tools/multicast/inc" \
	-I"$(SRC_ROOT_DIR)/tools/Target_tools/ip_tools/ping/inc" \
	-I"$(SRC_ROOT_DIR)/tools/Target_tools/ip_tools/tcp/inc" \
	-I"$(SRC_ROOT_DIR)/tools/Target_tools/ip_tools/udp/inc" \
	-I"$(SRC_ROOT_DIR)/tools/Target_tools/ip_tools/rawEth/inc" \
	-I"$(SRC_ROOT_DIR)/tools/Target_tools/memmgt/heapstats/inc"

# List of objects that can be built by the generic rule in master makefile
OBJECT_LIST += \
 $(OBJECT_DIR)/tools/Target_tools/battery_management/coincell/src/nt_cc_batt_mng.o \
 $(OBJECT_DIR)/tools/Target_tools/battery_management/coincell/src/nt_cc_battery_driver.o \
 $(OBJECT_DIR)/tools/Target_tools/battery_management/coincell/src/nt_sys_monitoring.o \
 $(OBJECT_DIR)/tools/Target_tools/dev_cfg/src/nt_devcfg_byte_seq_parser.o \
 $(OBJECT_DIR)/tools/Target_tools/dev_cfg/src/nt_devcfg_parser.o \
 $(OBJECT_DIR)/tools/Target_tools/ip_tools/multicast/src/nt_multicast.o \
 $(OBJECT_DIR)/tools/Target_tools/ip_tools/ping/src/ping.o \
 $(OBJECT_DIR)/tools/Target_tools/ip_tools/tcp/src/lwiperf.o \
 $(OBJECT_DIR)/tools/Target_tools/ip_tools/tcp/src/nt_lwiperf_mbedtls.o \
 $(OBJECT_DIR)/tools/Target_tools/ip_tools/udp/src/udp_perf_raw_client.o \
 $(OBJECT_DIR)/tools/Target_tools/ip_tools/rawEth/src/rawEth_iperf.o \
 $(OBJECT_DIR)/tools/Target_tools/memmgt/heapstats/src/nt_heap_stats.o



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
