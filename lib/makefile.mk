#========================================================================
#
#========================================================================*/

# Add path of header files used in this module
INCLUDE_PATHS += \
    -I"$(SRC_ROOT_DIR)/lib/libc/include" \
    -I"$(SRC_ROOT_DIR)/include" \
    -I"$(SRC_ROOT_DIR)/lib/FreeRTOS-Plus-POSIX/include" \
    -I"$(SRC_ROOT_DIR)/include/api" \
    -I"$(SRC_ROOT_DIR)/os/freertos/libraries/3rdparty/lfs-master"

INCLUDE_PATHS += \
    -I"$(SRC_ROOT_DIR)/lib/posix"

ifeq ($(strip $(CONFIG_FREERTOS_PLUS_POSIX)), y)
OBJECT_LIST += \
     $(OBJECT_DIR)/lib/FreeRTOS-Plus-POSIX/source/FreeRTOS_POSIX_clock.o \
     $(OBJECT_DIR)/lib/FreeRTOS-Plus-POSIX/source/FreeRTOS_POSIX_mqueue.o \
     $(OBJECT_DIR)/lib/FreeRTOS-Plus-POSIX/source/FreeRTOS_POSIX_pthread_barrier.o \
     $(OBJECT_DIR)/lib/FreeRTOS-Plus-POSIX/source/FreeRTOS_POSIX_pthread.o \
     $(OBJECT_DIR)/lib/FreeRTOS-Plus-POSIX/source/FreeRTOS_POSIX_pthread_cond.o \
     $(OBJECT_DIR)/lib/FreeRTOS-Plus-POSIX/source/FreeRTOS_POSIX_pthread_mutex.o \
     $(OBJECT_DIR)/lib/FreeRTOS-Plus-POSIX/source/FreeRTOS_POSIX_sched.o \
     $(OBJECT_DIR)/lib/FreeRTOS-Plus-POSIX/source/FreeRTOS_POSIX_semaphore.o \
     $(OBJECT_DIR)/lib/FreeRTOS-Plus-POSIX/source/FreeRTOS_POSIX_timer.o \
     $(OBJECT_DIR)/lib/FreeRTOS-Plus-POSIX/source/FreeRTOS_POSIX_unistd.o \
     $(OBJECT_DIR)/lib/FreeRTOS-Plus-POSIX/source/FreeRTOS_POSIX_utils.o
endif

OBJECT_LIST += \
	 $(OBJECT_DIR)/lib/qapi/peripherals/qapi_i2c.o  \
     $(OBJECT_DIR)/lib/libc/source/malloc.o \
     $(OBJECT_DIR)/lib/libc/source/fs.o \
     $(OBJECT_DIR)/lib/libc/source/printf.o \
     $(OBJECT_DIR)/lib/libc/source/assert.o \
     $(OBJECT_DIR)/lib/libc/source/safeAPI.o

COMPILER_FLAGS +=  -Wno-error=cast-function-type

ifeq ($(strip $(CONFIG_WLAN)), y)
    ifeq ($(strip $(CONFIG_WIFI_SHELL)), y)
        COMPILER_FLAGS += -DCONFIG_WIFI_SHELL=1
        OBJECT_LIST += $(OBJECT_DIR)/lib/qapi/wlan/wifi_shell.o
        CONFIG_WLAN_QAPI = y
    endif
    ifeq ($(strip $(CONFIG_WLAN_QAPI)), y)
        COMPILER_FLAGS += -DCONFIG_WLAN_QAPI=1
        INCLUDE_PATHS += -I"$(SRC_ROOT_DIR)/lib/qapi/wlan"
        OBJECT_LIST += $(OBJECT_DIR)/lib/qapi/wlan/qapi_wlan_base.o
        OBJECT_LIST += $(OBJECT_DIR)/lib/qapi/wlan/qapi_wlan_param_group.o
        OBJECT_LIST += $(OBJECT_DIR)/lib/qapi/wlan/wlan_qapi_helper.o
        OBJECT_LIST += $(OBJECT_DIR)/lib/qapi/wlan/wmi_api.o
        OBJECT_LIST += $(OBJECT_DIR)/lib/qapi/wlan/wlan_drv.o
    endif
endif

ifeq ($(strip $(CONFIG_LOWPOWER_SHELL)), y)
    COMPILER_FLAGS += -DCONFIG_LOWPOWER_SHELL=1
    OBJECT_LIST += $(OBJECT_DIR)/lib/lowpower/lowpower_shell.o
    OBJECT_LIST += $(OBJECT_DIR)/lib/qapi/lowpower/qapi_lowpower.o
endif

ifeq ($(strip $(CONFIG_NET)), y)
	ifeq ($(strip $(CONFIG_NET_SHELL)), y)
        COMPILER_FLAGS += -DCONFIG_NET_SHELL=1
        OBJECT_LIST += $(OBJECT_DIR)/lib/net/net_shell.o
        OBJECT_LIST += $(OBJECT_DIR)/lib/net/iperf.o
        OBJECT_LIST += $(OBJECT_DIR)/lib/net/ssl_demo.o
        OBJECT_LIST += $(OBJECT_DIR)/lib/net/ping.o
    endif
endif

ifeq ($(strip $(CONFIG_PLATFORM_QAPI)), y)
    OBJECT_LIST += $(OBJECT_DIR)/lib/qapi/platform/qapi_fw_info.o
    OBJECT_LIST += $(OBJECT_DIR)/lib/qapi/platform/qapi_heap_status.o
endif

ifeq ($(strip $(CONFIG_FLASH_SHELL)), y)
    COMPILER_FLAGS += -DCONFIG_FLASH_SHELL=1
    OBJECT_LIST += $(OBJECT_DIR)/lib/qapi/peripherals/flash_shell.o
endif

ifeq ($(strip $(CONFIG_SIGMA_TRAFFIC)), y)
  OBJECT_LIST += $(OBJECT_DIR)/lib/wificert/sigma_traffic/sigma_shell.o
  OBJECT_LIST += $(OBJECT_DIR)/lib/wificert/sigma_traffic/sigma_udp.o
endif

