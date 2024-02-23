#========================================================================
#
#========================================================================

INCLUDE_PATHS += -I$(SRC_ROOT_DIR)/demo/qcli_demo/src
OBJECT_LIST += $(OBJECT_DIR)/demo/qcli_demo/src/qcli_demo_main.o

ifeq ($(strip $(CONFIG_QCCSDK_CONSOLE)), y)
	CONFIG_WIFI_SHELL=y
	CONFIG_PLATFORM_SHELL=y
	CONFIG_LOWPOWER_SHELL=y
	CONFIG_I2CM_SHELL=y
	CONFIG_NET_SHELL=y

ifeq ($(strip $(CONFIG_PROF)), y)
	CONFIG_PROF_SHELL=y
endif
endif
