#========================================================================
#
#========================================================================

#COMPILER_FLAGS += -DCONFIG_QCCSDK_CONSOLE=1

INCLUDE_PATHS += -I$(SRC_ROOT_DIR)/subsys/console
OBJECT_LIST += $(OBJECT_DIR)/subsys/console/qapi_console.o
OBJECT_LIST += $(OBJECT_DIR)/subsys/console/qcli.o
OBJECT_LIST += $(OBJECT_DIR)/subsys/console/qcli_pal.o
OBJECT_LIST += $(OBJECT_DIR)/subsys/console/qcli_util.o

