#========================================================================
#
#========================================================================*/

# Add path of header files used in this module
INCLUDE_PATHS += \
    -I"$(SRC_ROOT_DIR)/include/qapi/common" \
    -I"$(SRC_ROOT_DIR)/include/qapi"

ifeq ($(strip $(CONFIG_WLAN)), y)
    INCLUDE_PATHS += -I"$(SRC_ROOT_DIR)/include/qapi/wlan"
endif

COMPILER_FLAGS += -Wno-error=char-subscripts
