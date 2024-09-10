#========================================================================
#
#========================================================================*/

#Need to build in HostIF and data svc only if the image type being built is Fermion
ifeq ($(strip $(VARIANT_IMAGE_TYPE)), FERMION)

# Add path of header files used in this module
INCLUDE_PATHS += \
    -I"$(SRC_ROOT_DIR)/core/hostif/api/wifi"    \
    -I"$(SRC_ROOT_DIR)/core/hostif/wifi_svc/inc"    \
    -I"$(SRC_ROOT_DIR)/core/hostif/ring_if_lib/inc"    \
    -I"$(SRC_ROOT_DIR)/core/hostif/data_svc/inc"    \
    -I"$(SRC_ROOT_DIR)/core/hostif/fw_offloads/inc"  
    

ifeq ($(strip $(PLATFORM_FAMILY)), NEUTRINO)
INCLUDE_PATHS += \
	-I"$(SRC_ROOT_DIR)/core/hostif/dwspi_qcspi/inc"
endif
# List of objects that can be built by the generic rule in master makefile
OBJECT_LIST += \
 $(OBJECT_DIR)/core/hostif/wifi_svc/src/wifi_lib.o  \
 $(OBJECT_DIR)/core/hostif/ring_if_lib/src/ctrl_ring_hdlr.o    \
 $(OBJECT_DIR)/core/hostif/ring_if_lib/src/data_ring_hdlr.o    \
 $(OBJECT_DIR)/core/hostif/ring_if_lib/src/ring_ctx_holder.o    \
 $(OBJECT_DIR)/core/hostif/ring_if_lib/src/ring_svc_api.o    	\
 $(OBJECT_DIR)/core/hostif/data_svc/src/data_svc.o            \
 $(OBJECT_DIR)/core/hostif/data_svc/src/data_svc_ip.o        \
 $(OBJECT_DIR)/core/hostif/data_svc/src/data_svc_udp.o        \
 $(OBJECT_DIR)/core/hostif/data_svc/src/data_svc_tcp.o        \
 $(OBJECT_DIR)/core/hostif/data_svc/src/data_svc_raweth.o   \
 $(OBJECT_DIR)/core/hostif/fw_offloads/src/wifi_fw_eb_offloads.o 
 
 
ifeq ($(strip $(PLATFORM_FAMILY)), NEUTRINO)
OBJECT_LIST += \
 $(OBJECT_DIR)/core/hostif/dwspi_qcspi/src/qcspi_on_dwspi.o
endif

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
