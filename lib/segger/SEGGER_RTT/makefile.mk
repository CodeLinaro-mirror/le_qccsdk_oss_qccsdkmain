#========================================================================
#
#========================================================================

#COMPILER_FLAGS += -DCONFIG_QCCSDK_CONSOLE=1
#INCLUDE_PATHS += -I$(SRC_ROOT_DIR)/subsys/debug/segger/SEGGER_RTT/Config
OBJECT_LIST += $(OBJECT_DIR)/subsys/debug/segger/SEGGER_RTT/RTT/SEGGER_RTT.o
OBJECT_LIST += $(OBJECT_DIR)/subsys/debug/segger/SEGGER_RTT/RTT/SEGGER_RTT_printf.o
OBJECT_LIST += $(OBJECT_DIR)/subsys/debug/segger/SEGGER_RTT/RTT/SEGGER_SYSVIEW.o
OBJECT_LIST += $(OBJECT_DIR)/subsys/debug/segger/SEGGER_RTT/RTT/SEGGER_SYSVIEW_FreeRTOS.o
OBJECT_LIST += $(OBJECT_DIR)/subsys/debug/segger/SEGGER_RTT/RTT/SEGGER_SYSVIEW_Config_FreeRTOS.o
