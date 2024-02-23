#========================================================================
#
# Fermion top level makefile
#========================================================================

#*******************************************************************************
# Makefile configuration
#*******************************************************************************
#FERMION or FERMION_PBL or FERMION_IOE_PBL or FERMION_SBL or FERMION_FTM or FERMION_QCLI_DEMO or FERMION_HELLO_WORLD or FERMION_NVM_PROGRAMMER
#VARIANT_NAME to be passed as argument to this makefile. If not passed, take default as FERMION
VARIANT_NAME ?= FERMION
PLATFORM_VERSION_FERMION ?= E3_07
PLATFORM_VERSION_FERMION_V2 ?= TAPEOUT_02
CHIP_VERSION_FERMION ?=1
HALMAC_VARIANT ?= NEUTRINO
ENABLE_PREPROC ?= NO
#RCLI_ON ?= OFF
XIP_FLASH_DEMO ?= OFF
CONFIG_BIN_GEN ?= ON
IMAGE_COMBINE ?= NO
outdir?=build

SOCKET_BOARD_CHIPV1=qcc730v1_socket
SOCKET_BOARD_CHIPV2=qcc730v2_socket
EVB_V11_HOSTLESS=qcc730v2_evb11_hostless
EVB_V12_HOSTLESS=qcc730v2_evb12_hostless
EVB_V13_HOSTLESS=qcc730v2_evb13_hostless
DEFAULT_BOARD_NAME=$(SOCKET_BOARD_CHIPV1)
BOARD_NAME?=$(DEFAULT_BOARD_NAME)

ifeq ($(strip $(VARIANT_NAME)), FERMION)
	PLATFORM_FAMILY = FERMION
	PLATFORM_TYPE = SILICON
	VARIANT_IMAGE_TYPE = FERMION
	HALMAC_TYPE = FERMION_HAL
	VARIANT_IMAGE_ID = MM

else ifeq ($(strip $(VARIANT_NAME)), FERMION_FTM)
	PLATFORM_FAMILY = FERMION
	PLATFORM_TYPE = SILICON
	VARIANT_IMAGE_TYPE = FERMION
	HALMAC_TYPE = FERMION_HAL
	VARIANT_IMAGE_ID = MM

else ifeq ($(strip $(VARIANT_NAME)), FERMION_PBL)
	PLATFORM_FAMILY = FERMION
	PLATFORM_TYPE = SILICON
	VARIANT_IMAGE_TYPE = FERMION
	VARIANT_IMAGE_ID = PBL

else ifeq ($(strip $(VARIANT_NAME)), FERMION_IOE_PBL)
	PLATFORM_FAMILY = FERMION
	PLATFORM_TYPE = SILICON
	VARIANT_IMAGE_TYPE = FERMION
	VARIANT_IMAGE_ID = IOE_PBL
	CHIP_VERSION_FERMION=2

else ifeq ($(strip $(VARIANT_NAME)), FERMION_SBL)
	PLATFORM_FAMILY = FERMION
	PLATFORM_TYPE = SILICON
	VARIANT_IMAGE_TYPE = FERMION
	VARIANT_IMAGE_ID = SBL

else ifeq ($(strip $(VARIANT_NAME)), FERMION_QCLI_DEMO)
	PLATFORM_FAMILY = FERMION
	PLATFORM_TYPE = SILICON
	VARIANT_IMAGE_TYPE = FERMION
	HALMAC_TYPE = FERMION_HAL
	VARIANT_IMAGE_ID = MM
	CONFIG_QCCSDK_DEMO=y
	APP=qcli_demo

else ifeq ($(strip $(VARIANT_NAME)), FERMION_IOE_QCLI_DEMO)
	PLATFORM_FAMILY = FERMION
	PLATFORM_TYPE = SILICON
	VARIANT_IMAGE_TYPE = FERMION
	HALMAC_TYPE = FERMION_HAL
	VARIANT_IMAGE_ID = MM
	IMAGE_COMBINE = ON
	CONFIG_QCCSDK_DEMO=y
	APP=qcli_demo

else ifeq ($(strip $(VARIANT_NAME)), FERMION_HELLO_WORLD)
	PLATFORM_FAMILY = FERMION
	PLATFORM_TYPE = SILICON
	VARIANT_IMAGE_TYPE = FERMION
	HALMAC_TYPE = FERMION_HAL
	VARIANT_IMAGE_ID = MM
	CONFIG_QCCSDK_DEMO=y
	APP=hello_world

else ifeq ($(strip $(VARIANT_NAME)), FERMION_POSIX_DEMO)
	PLATFORM_FAMILY = FERMION
	PLATFORM_TYPE = SILICON
	VARIANT_IMAGE_TYPE = FERMION
	HALMAC_TYPE = FERMION_HAL
	VARIANT_IMAGE_ID = MM
	CONFIG_QCCSDK_DEMO=y
	APP=posix_demo

else ifeq ($(strip $(VARIANT_NAME)), FERMION_NVM_PROGRAMMER)
	PLATFORM_FAMILY = FERMION
	PLATFORM_TYPE = SILICON
	VARIANT_IMAGE_TYPE = FERMION
	HALMAC_TYPE = FERMION_HAL
	VARIANT_IMAGE_ID = NVM_PROGRAMMER
else ifeq ($(strip $(VARIANT_NAME)), FERMION_FTM_V2)
	PLATFORM_FAMILY = FERMION
	PLATFORM_TYPE = SILICON
	VARIANT_IMAGE_TYPE = FERMION
	HALMAC_TYPE = FERMION_HAL
	VARIANT_IMAGE_ID = MM
	CHIP_VERSION_FERMION = 2
else ifeq ($(strip $(VARIANT_NAME)), FERMION_QCLI_DEMO_V2)
	PLATFORM_FAMILY = FERMION
	PLATFORM_TYPE = SILICON
	VARIANT_IMAGE_TYPE = FERMION
	HALMAC_TYPE = FERMION_HAL
	VARIANT_IMAGE_ID = MM
	CHIP_VERSION_FERMION = 2
	CONFIG_QCCSDK_DEMO=y
	APP=qcli_demo
else ifeq ($(strip $(VARIANT_NAME)), FERMION_IOE_QCLI_DEMO_V2)
	PLATFORM_FAMILY = FERMION
	PLATFORM_TYPE = SILICON
	VARIANT_IMAGE_TYPE = FERMION
	HALMAC_TYPE = FERMION_HAL
	VARIANT_IMAGE_ID = MM
	CHIP_VERSION_FERMION = 2
	IMAGE_COMBINE = ON
	CONFIG_QCCSDK_DEMO=y
	APP=qcli_demo
else ifeq ($(strip $(VARIANT_NAME)), FERMION_V2)
	PLATFORM_FAMILY = FERMION
	PLATFORM_TYPE = SILICON
	VARIANT_IMAGE_TYPE = FERMION
	HALMAC_TYPE = FERMION_HAL
	VARIANT_IMAGE_ID = MM
	CHIP_VERSION_FERMION = 2

else ifeq ($(strip $(VARIANT_NAME)), FERMION_PBL_V2)
	PLATFORM_FAMILY = FERMION
	PLATFORM_TYPE = SILICON
	VARIANT_IMAGE_TYPE = FERMION
	VARIANT_IMAGE_ID = PBL
	CHIP_VERSION_FERMION = 2

else ifeq ($(strip $(VARIANT_NAME)), FERMION_SBL_V2)
	PLATFORM_FAMILY = FERMION
	PLATFORM_TYPE = SILICON
	VARIANT_IMAGE_TYPE = FERMION
	VARIANT_IMAGE_ID = SBL
	CHIP_VERSION_FERMION = 2
else
   $(error "ERROR: Invalid VARIANT_NAME: $(VARIANT_NAME). To learn more type - make help")
endif

#*******************************************************************************
# CRM_BUILD_NUM
#*******************************************************************************
ifeq ($(CRM_BUILDID),)
    export CRM_BUILD_NUM=9999
else
    export CRM_BUILD_NUM=$(shell echo $$CRM_BUILDID | sed -e 's,[^-]*-,,' -e 's,-.*,,' | sed -e 's,^0*,,')
endif

ifeq ($(strip $(VARIANT_IMAGE_ID)),PBL)
	export ARMHOME=/pkg/qct/software/arm/RVDS/6.9
	export ARMLMD_LICENSE_FILE=7117@license-wan-arm1
endif

#*******************************************************************************
# prebuild script
#*******************************************************************************
PRE_BUILD_SCRIPT :=
ifeq ($(strip $(VARIANT_IMAGE_ID)),PBL)
	PRE_BUILD_SCRIPT = pbl_prebuild.py
else ifeq ($(strip $(VARIANT_IMAGE_ID)),IOE_PBL)
PRE_BUILD_SCRIPT = pbl_prebuild.py
else ifeq ($(strip $(VARIANT_IMAGE_ID)),SBL)
PRE_BUILD_SCRIPT = sbl_prebuild.py
else
PRE_BUILD_SCRIPT = chip_full_debug_halphy_prebuild.py
endif

EXECUTABLE_NAME        = $(VARIANT_NAME)
SECONDARY_HEX = $(EXECUTABLE_NAME).hex
SECONDARY_SIZE = $(EXECUTABLE_NAME).siz
SECONDARY_BIN = $(EXECUTABLE_NAME).bin

#*******************************************************************************
# Toolchain specific configs
#*******************************************************************************
TOOLCHAIN_C_FLAGS = -mcpu=cortex-m4 -mthumb -mfloat-abi=softfp                \
                    -mfpu=fpv4-sp-d16 -Os -fmessage-length=0 -fsigned-char    \
                    -ffunction-sections -fdata-sections -fno-common           \
                    -fno-inline-functions -ffreestanding -fno-builtin         \
                    -fsingle-precision-constant -fstack-usage           \
                    -fno-move-loop-invariants -Werror -Wall -Wextra  -g3 \
                    -Wno-error=unused-variable -Wno-error=unused-parameter

TOOLCHAIN_CPP_FLAGS = -mcpu=cortex-m4 -mthumb -mfloat-abi=softfp              \
                      -mfpu=fpv4-sp-d16 -Os -fmessage-length=0 -fsigned-char  \
					  -ffunction-sections -fdata-sections -fno-common         \
					  -fno-inline-functions -ffreestanding -fno-builtin       \
					  -fsingle-precision-constant -fstack-usage         \
					  -fno-move-loop-invariants -Werror -Wall -Wextra  -g3


#linker may use "ld" for arm-none-eabi-ld, else "arm-none-eabi-g++"
CONFIG_TOOLCHAIN_LINKER = ld

TOOLCHAIN_COMPILER_C = arm-none-eabi-gcc
TOOLCHAIN_COMPILER_CPP = arm-none-eabi-g++
ifeq ($(strip $(CONFIG_TOOLCHAIN_LINKER)), ld)
	TOOLCHAIN_LINKER = arm-none-eabi-ld
else
	TOOLCHAIN_LINKER = arm-none-eabi-g++
endif
TOOLCHAIN_OBJCOPY = arm-none-eabi-objcopy
TOOLCHAIN_ELFSIZE = arm-none-eabi-size
TOOLCHAIN_LD = arm-none-eabi-ld
TOOLCHAIN_AR = arm-none-eabi-ar

TOOLCHAIN_DEBUG_C_FLAGS = $(TOOLCHAIN_C_FLAGS)
TOOLCHAIN_RELEASE_C_FLAGS = $(TOOLCHAIN_C_FLAGS)

ifeq ($(strip $(CONFIG_TOOLCHAIN_LINKER)), ld)
	TOOLCHAIN_DEBUG_LINK_FLAGS = -no-wchar-size-warning --no-warn-mismatch
else
	TOOLCHAIN_DEBUG_LINK_FLAGS = -mcpu=cortex-m4 -mthumb -mfloat-abi=softfp              \
                             -mfpu=fpv4-sp-d16 -Os -fmessage-length=0 -fsigned-char  \
							 -ffunction-sections -fdata-sections -fno-common         \
							 -fno-inline-functions -ffreestanding -fno-builtin       \
							 -fsingle-precision-constant -fstack-usage         \
							 -fno-move-loop-invariants -Werror -Wall -Wextra  -g3
endif
TOOLCHAIN_RELEASE_LINK_FLAGS =  $(TOOLCHAIN_DEBUG_LINK_FLAGS)

#*******************************************************************************
# Toolchain independent Flags and variables
#*******************************************************************************
ifeq ($(strip $(VARIANT_IMAGE_ID)),PBL)
INCLUDE_PATHS = \
	-I"$(SRC_ROOT_DIR)/os/freertos/system/include/cmsis" \
	-I"$(SRC_ROOT_DIR)/core/common"
else ifeq ($(strip $(VARIANT_IMAGE_ID)),IOE_PBL)
INCLUDE_PATHS = \
	-I"$(SRC_ROOT_DIR)/arch/arm/system/include/cmsis" \
	-I"$(SRC_ROOT_DIR)/../comp/qcc730/core/common"
else
INCLUDE_PATHS = \
	-I"$(SRC_ROOT_DIR)/core/common"
endif

#---------------
#Compiler flags
#---------------
COMPILER_FLAGS = -DSCALE_INCLUDES -DCRM_BUILD_NUM=$(CRM_BUILD_NUM)
LINKER_FLAGS =
LINKER_ADDITIONAL_FLAGS =
ARCHIVE_FLAGS = -rc

ifeq ($(strip $(VARIANT_IMAGE_TYPE)), FERMION)
	COMPILER_FLAGS += -DIMAGE_FERMION
endif

START_ADDRESS :=
PBL_START      = 0
SBL_START      = 0
WLAN_START     = 0x00223400


ifeq ($(strip $(CONFIG_BIN_GEN)), ON)
	START_ADDRESS :=
	PBL_START      = 0x00200000
	SBL_START      = 0x00204400
endif

ifeq ($(strip $(XIP_FLASH_DEMO)), ON)
	COMPILER_FLAGS += -DXIP_FLASH_TEST_DEMO
	XIP_DEMO_START = 0x03000000
endif

ifeq ($(strip $(DFU_BUILD)), ON)
	COMPILER_FLAGS += -DDFU_BUILD
	START_ADDRESS :=
	PBL_START      = 0x00200000
	SBL_START      = 0x00204400
endif

export QCCSDK_BOARD_NAME=$(BOARD_NAME)
BOARD_CONF = $(SRC_ROOT_DIR)/boards/$(BOARD_NAME)/$(BOARD_NAME)_defconfig
ifeq ($(strip $(BOARD_NAME)), $(SOCKET_BOARD_CHIPV1))
    CHIP_VERSION_FERMION = 1
else ifeq ($(strip $(BOARD_NAME)), $(EVB_V11_HOSTLESS))
    CHIP_VERSION_FERMION = 2
    COMPILER_FLAGS += -DFEMION_IOT
else ifeq ($(strip $(BOARD_NAME)), $(EVB_V12_HOSTLESS))
    CHIP_VERSION_FERMION = 2
    COMPILER_FLAGS += -DFEMION_IOT
else ifeq ($(strip $(BOARD_NAME)), $(SOCKET_BOARD_CHIPV2))
    CHIP_VERSION_FERMION = 2
    COMPILER_FLAGS += -DFEMION_IOT
else ifeq ($(strip $(BOARD_NAME)), $(EVB_V13_HOSTLESS))
    CHIP_VERSION_FERMION = 2
    COMPILER_FLAGS += -DFEMION_IOT
else
    $(error "ERROR: Invalid BOARD_NAME: $(BOARD_NAME).)
endif

ifeq ($(strip $(VARIANT_IMAGE_ID)), IOE_PBL)
	CHIP_VERSION_FERMION = 2
endif

ifeq ($(strip $(PLATFORM_FAMILY)), FERMION)
	COMPILER_FLAGS += -DPLATFORM_FERMION -DPHYDEVLIB_PRODUCT_FERMION -DPHYDEVLIB_IOT
	ifeq ($(strip $(PLATFORM_TYPE)), SILICON)
		COMPILER_FLAGS += -DFERMION_SILICON
	endif
	ifeq ($(strip $(CHIP_VERSION_FERMION)), 2)
		COMPILER_FLAGS += -DFERMION_CHIP_VERSION=2
		INCLUDE_PATHS += \
		-I"$(SRC_ROOT_DIR)/../comp/qcc730/core/common/hwio/fermionv2/$(PLATFORM_VERSION_FERMION_V2)"
	else
		COMPILER_FLAGS += -DFERMION_CHIP_VERSION=1
		INCLUDE_PATHS += \
		-I"$(SRC_ROOT_DIR)/core/common/hwio/fermion/$(PLATFORM_VERSION_FERMION)"
	endif
endif

ifeq ($(strip $(PLATFORM_TYPE)), SILICON)
	COMPILER_FLAGS += -DSILICON_BUILD
endif

ifeq ($(strip $(HALMAC_TYPE)), FERMION_HAL)
	COMPILER_FLAGS += -DUSE_FERMION_HALMAC
endif

ifeq ($(strip $(PLATFORM_FAMILY)), FERMION)
ifeq ($(strip $(PLATFORM_VERSION_FERMION)), E2_07)
	COMPILER_FLAGS += -DWCSS_VERSION_E2
endif
endif

ifeq ($(strip $(RCLI_ON)), ON)
	COMPILER_FLAGS += -DAPP_RCLI_EN
endif

FREERTOS_CPP_FLAGS = $(TOOLCHAIN_CPP_FLAGS)

#---------------
#Linker input and output config
#---------------

LINKER_INPUTS = -L"build/freertos/eclipse-gcc/ldscripts"
ifeq ($(strip $(VARIANT_IMAGE_TYPE)), FERMION)

	ifeq ($(strip $(PLATFORM_FAMILY)), FERMION)
		ifeq ($(strip $(VARIANT_IMAGE_ID)), PBL)
			LINKER_INPUTS += -T frn_pbl.ld
			START_ADDRESS = $(PBL_START)
		else ifeq ($(strip $(VARIANT_IMAGE_ID)), IOE_PBL)
			LINKER_INPUTS += -T frn_ioe_pbl.ld
			START_ADDRESS = $(PBL_START)
		else ifeq ($(strip $(VARIANT_IMAGE_ID)), SBL)
			LINKER_INPUTS += -T frn_ioe_sbl.ld
			START_ADDRESS = $(SBL_START)
		else ifeq ($(strip $(XIP_FLASH_DEMO)), ON)
			LINKER_INPUTS += -T frn_wlan_sys_xip_flsh_test.ld
			START_ADDRESS = $(WLAN_START)
		else ifeq ($(strip $(VARIANT_IMAGE_ID)), NVM_PROGRAMMER)
			LINKER_INPUTS += -T frn_nvm_programmer.ld
		else
			ifeq ($(strip $(IMAGE_COMBINE)), ON)
				LINKER_INPUTS += -T frn_ioe_wlan_sys_image.ld
			else
				LINKER_INPUTS += -T frn_wlan_sys_image.ld
			endif
			START_ADDRESS = $(WLAN_START)
		endif
	endif
endif

LINKER_INPUTS +=   -T libs.ld
ifeq ($(strip $(CONFIG_TOOLCHAIN_LINKER)), ld)
	LINKER_INPUTS += -n
else
	LINKER_INPUTS += -nostartfiles -Xlinker
endif
LINKER_INPUTS += --gc-sections
#ifeq ($(strip $(VARIANT_IMAGE_ID)),MM)
#    LINKER_INPUTS +=  -L"os/freertos/libraries/3rdparty/xerus_nanolib/arm-none-eabi/lib/thumb/v7e-m/nofp"
#endif

LIBS := -lc_nano -lnosys -lm
EXTERA_LIBS :=
ifeq ($(strip $(CONFIG_TOOLCHAIN_LINKER)), ld)
	LIB_NANO_PATH = "$(shell dirname "`$(TOOLCHAIN_COMPILER_C) -mcpu=cortex-m4 -mthumb -mfloat-abi=softfp -print-file-name=libc_nano.a`")"
	EXTERA_LIBS += -lgcc
	LIB_GCC_PATH = "$(shell dirname "`$(TOOLCHAIN_COMPILER_C) -mcpu=cortex-m4 -mthumb -mfloat-abi=softfp -print-libgcc-file-name`")"
	LINKER_INPUTS += -L$(LIB_NANO_PATH) -L$(LIB_GCC_PATH) $(LIBS)
endif

WLAN_LIBS := -lwlan_wmi -lwlan_coex -lwlan_config_ini -lwlan_mlm -lwlan_hal -lwlan_dpm -lwlan_halphy -lwlan_sme -lwlan_sec -lwlan_tools
WLAN_LIBS_NAMES := libwlan_wmi.a libwlan_coex.a libwlan_config_ini.a libwlan_mlm.a libwlan_hal.a libwlan_dpm.a libwlan_halphy.a  libwlan_sme.a libwlan_sec.a libwlan_tools.a

#---------------
# The list of objects and dependency files being built by this makefile
#---------------
OBJECT_LIST :=
FREERTOS_OBJECT_LIST :=
SPECIAL_OBJECT_LIST :=
DEP_LIST :=
SPECIAL_DEP_LIST :=
SU_LIST :=
FREERTOS_SU_LIST :=
SPECIAL_SU_LIST :=
PREPROC_LIST :=

#*******************************************************************************
# DEBUG and RELEASE build types
#*******************************************************************************

# If no BUILD_TYPE is specified, provide a default
BUILD_TYPE                ?= DEBUG

ifeq ($(strip $(BUILD_TYPE)), DEBUG)
	ifeq ($(strip $(VARIANT_IMAGE_ID)),PBL)
		COMPILER_FLAGS         +=                                                  \
		  -DPBL_BUILD                                                              \
		  -D__BUILD_TYPE="__DEBUG_BUILD"                                           \
		  -D__LINUX_ERRNO_EXTENSIONS__                                             \
		  -DNT_CHIP_PBL_PRODUCTION                                                 \
		  $(TOOLCHAIN_DEBUG_C_FLAGS)
#		  -DSUPPORT_PBL_PATCH                                                      \
#		  -DPBL_PATCH_TEST                                                         \
#		  -DP_DEBUG
	else ifeq ($(strip $(VARIANT_IMAGE_ID)),IOE_PBL)
		COMPILER_FLAGS         +=                                                  \
		  -DPBL_BUILD                                                              \
		  -D__BUILD_TYPE="__DEBUG_BUILD"                                           \
		  -D__LINUX_ERRNO_EXTENSIONS__                                             \
		  -DNT_CHIP_PBL_PRODUCTION                                                 \
		  $(TOOLCHAIN_DEBUG_C_FLAGS)
		  #-DP_DEBUG                                                                \
#		  -DPBL_DEBUG_MODE  	                                                   \
#		  -DSUPPORT_PBL_PATCH                                                      \
#		  -DPBL_PATCH_TEST
	else ifeq ($(strip $(VARIANT_IMAGE_ID)),SBL)
		COMPILER_FLAGS         +=                                                  \
		  -DSBL_BUILD                                                              \
		  -D__BUILD_TYPE="__DEBUG_BUILD"                                           \
		  -DP_DEBUG                                                                \
		  -D__LINUX_ERRNO_EXTENSIONS__                                             \
		  -DNT_CHIP_PBL_PRODUCTION                                                 \
		  $(TOOLCHAIN_DEBUG_C_FLAGS)											   \
		  -DSBL_SELF_VALIDATE													   \
#		  -DSUPPORT_PBL_PATCH                                                      \
#		  -DPBL_PATCH_TEST
	else
		COMPILER_FLAGS         +=                                                  \
		  -D__BUILD_TYPE="__DEBUG_BUILD"                                           \
		  -DNT_DEBUG                                                               \
		  -DDEBUG                                                                  \
		  -DCHIP_HALPHY_FULL_DEBUG                                                 \
		  $(TOOLCHAIN_DEBUG_C_FLAGS)
	endif
   LINKER_FLAGS           +=                                                   \
      $(TOOLCHAIN_DEBUG_LINK_FLAGS)

	FREERTOS_CPP_FLAGS +=                                                       \
                        -DDEBUG -DTRACE -DOS_USE_TRACE_SEMIHOSTING_DEBUG

else ifeq ($(strip $(BUILD_TYPE)), RELEASE)
	ifeq ($(strip $(VARIANT_IMAGE_ID)),PBL)
		COMPILER_FLAGS         +=                                                  \
		  -D__BUILD_TYPE="__RELEASE_BUILD"                                         \
		  -D__LINUX_ERRNO_EXTENSIONS__                                             \
		  $(TOOLCHAIN_RELEASE_C_FLAGS)
#		  -DSUPPORT_PBL_PATCH
	else ifeq ($(strip $(VARIANT_IMAGE_ID)),IOE_PBL)
		COMPILER_FLAGS         +=                                                  \
		  -D__BUILD_TYPE="__RELEASE_BUILD"                                         \
		  -D__LINUX_ERRNO_EXTENSIONS__                                             \
		  $(TOOLCHAIN_RELEASE_C_FLAGS)
#		  -DSUPPORT_PBL_PATCH
	else ifeq ($(strip $(VARIANT_IMAGE_ID)),SBL)
		COMPILER_FLAGS         +=                                                  \
		  -DSBL_BUILD                                                              \
		  -D__BUILD_TYPE="__RELEASE_BUILD"                                         \
		  -D__LINUX_ERRNO_EXTENSIONS__                                             \
		  $(TOOLCHAIN_DEBUG_C_FLAGS)
	else
		COMPILER_FLAGS         +=                                                  \
		  -D__BUILD_TYPE="__RELEASE_BUILD"                                         \
		  $(TOOLCHAIN_RELEASE_C_FLAGS)
	endif
   LINKER_FLAGS           +=                                                   \
      $(TOOLCHAIN_RELEASE_LINK_FLAGS)

else
   $(error "ERROR: Invalid BUILD_TYPE: $(BUILD_TYPE). To learn more type - make help")
endif

ifneq ($(strip $(CONFIG_TOOLCHAIN_LINKER)), ld)
	LINKER_ADDITIONAL_FLAGS += --specs=nano.specs --specs=nosys.specs -z max-page-size=4
	ifeq ($(strip $(VARIANT_IMAGE_ID)),PBL)
		LINKER_ADDITIONAL_FLAGS += -n
	else ifeq ($(strip $(VARIANT_IMAGE_ID)),IOE_PBL)
		LINKER_ADDITIONAL_FLAGS += -n
	endif
endif

#*******************************************************************************
# directories
#*******************************************************************************
BUILD_TYPE_DIR         =$(strip $(BUILD_TYPE))
OBJECT_DIR             = $(outdir)/$(strip $(VARIANT_NAME))/$(BUILD_TYPE_DIR)
LIB_DIR                = $(OBJECT_DIR)/lib
export LD_LIBRARY_PATH=$(LIB_DIR)
SRC_ROOT_DIR           =  .
EXECUTABLE_DIR         = $(addsuffix /bin, $(OBJECT_DIR))
SCRIPTS_DIR            = $(SRC_ROOT_DIR)/build/freertos/eclipse-gcc/Scripts
DEV_CFG_DIR            = $(SRC_ROOT_DIR)/tools/host_tools/dev_cfg

ifeq ($(strip $(CONFIG_TOOLCHAIN_LINKER)), ld)
	LINKER_OUTPUT = -Map=$(EXECUTABLE_DIR)/$(EXECUTABLE_NAME).map
else
	LINKER_OUTPUT = -Wl,-Map,$(EXECUTABLE_DIR)/$(EXECUTABLE_NAME).map
endif

#*******************************************************************************
# Demo APP & prj.conf
#*******************************************************************************
#APP option: hello_world, qcli_demo
ifeq ($(strip $(CONFIG_QCCSDK_DEMO)), y)
	ifeq ($(strip $(APP)), qcli_demo)
	    APP_CONF = $(SRC_ROOT_DIR)/demo/qcli_demo/prj.conf
	else ifeq ($(strip $(APP)), hello_world)
	    APP_CONF = $(SRC_ROOT_DIR)/demo/hello_world/prj.conf
	else ifeq ($(strip $(APP)), posix_demo)
	    APP_CONF = $(SRC_ROOT_DIR)/demo/posix_demo/prj.conf
	endif
else
	ifeq ($(strip $(VARIANT_NAME)), FERMION_FTM)
		APP_CONF=$(SRC_ROOT_DIR)/apps/ftm_prj.conf
	else ifeq ($(strip $(VARIANT_NAME)), FERMION)
		APP_CONF=$(SRC_ROOT_DIR)/apps/prj.conf
	else
		#others, donot use prj.conf yet
		APP_CONF=
	endif
endif

#*******************************************************************************
# Kconfig
#*******************************************************************************
DOTCONFIG = $(OBJECT_DIR)/.config
KCONFIG_ROOT_PATH := $(SRC_ROOT_DIR)/Kconfig
AUTOCONF=$(OBJECT_DIR)/include/generated/autoconf.h
INCLUDE_PATHS += \
	-I"$(OBJECT_DIR)/include/generated"
LOG_PATH = $(OBJECT_DIR)/kconfig-files-list.log
CONF_LIST := $(BOARD_CONF) $(APP_CONF)

dotconfig:
	@mkdir -p "$(OBJECT_DIR)/include/generated/"
	@echo BOARD_CONF = $(BOARD_CONF)
	@echo QCCSDK_BOARD_NAME = $(QCCSDK_BOARD_NAME)
	-python3 tools/kconfig_scripts/kconfig.py --handwritten-input-configs $(KCONFIG_ROOT_PATH) $(DOTCONFIG) $(AUTOCONF) $(LOG_PATH) $(CONF_LIST)
autoconf: dotconfig
	-python3 tools/kconfig_scripts/kconfig.py $(KCONFIG_ROOT_PATH) $(DOTCONFIG) $(AUTOCONF) $(LOG_PATH) $(DOTCONFIG)

export KCONFIG_CONFIG=$(DOTCONFIG)
menuconfig: dotconfig
	menuconfig $(KCONFIG_ROOT_PATH)

-include $(DOTCONFIG)
COMPILER_FLAGS += -include $(AUTOCONF)

#*******************************************************************************
# include all leaf makefiles and dependency files
#*******************************************************************************

CONFIG_WLAN=y
CONFIG_NET=y

ifeq ($(strip $(CONFIG_FTM_MODE)),y)
	COMPILER_FLAGS += -DFTM_OVER_UART
endif

ifeq ($(strip $(CONFIG_QCCSDK_DEMO)), y)
	COMPILER_FLAGS += -DCONFIG_QCCSDK_DEMO=1

	ifeq ($(strip $(APP)), hello_world)
		-include demo/hello_world/makefile.mk
	else ifeq ($(strip $(APP)), qcli_demo)
		-include demo/qcli_demo/makefile.mk
	else ifeq ($(strip $(APP)), posix_demo)
		-include demo/posix_demo/makefile.mk
	else
		$(error "ERROR: Invalid QCCSDK_DEMO_APP: $(APP).)
	endif

	ifeq ($(strip $(CONFIG_QCCSDK_CONSOLE)), y)
		-include subsys/console/makefile.mk
	endif
endif

ifeq ($(strip $(CONFIG_WLAN)), y)
	COMPILER_FLAGS += -DCONFIG_WLAN=1
endif

ifeq ($(strip $(CONFIG_NET)), y)
	COMPILER_FLAGS += -DCONFIG_NET=1
endif

ifeq ($(strip $(VARIANT_IMAGE_ID)), PBL)
 ifeq ($(strip $(VARIANT_IMAGE_TYPE)), FERMION)
	-include fermion_pbl/makefile.mk
 endif
else ifeq ($(strip $(VARIANT_IMAGE_ID)), IOE_PBL)
 ifeq ($(strip $(VARIANT_IMAGE_TYPE)), FERMION)
	-include bootloader/PBL/makefile.mk
	-include bootloader/PBL/pbl_auth/makefile.mk
 endif
else ifeq ($(strip $(VARIANT_IMAGE_ID)), SBL)
	-include fermion_sbl/makefile.mk
else ifeq ($(strip $(VARIANT_IMAGE_ID)), NVM_PROGRAMMER)
	-include nvm_programmer/makefile.mk
else
-include subsys/debug/segger/SEGGER_RTT/makefile.mk
-include core/wifi/halphy/makefile.mk
-include core/wifi/dpm/makefile.mk
-include core/wifi/hal/makefile.mk
-include core/wifi/mlm/makefile.mk
-include core/wifi/security/makefile.mk
-include core/wifi/sme/makefile.mk
-include core/wifi/coex/makefile.mk
-include core/wifi/wmi/makefile.mk
-include core/wifi/config_ini/makefile.mk
-include core/wifi/tools/unit_test/makefile.mk
-include core/system/makefile.mk
ifeq ($(strip $(CONFIG_RING_IF)), y)
-include core/hostif/makefile.mk
endif
-include hal/makefile.mk
-include os/freertos/makefile.mk
-include os/osal/makefile.mk
-include os/lib/makefile.mk
-include include/makefile.mk
-include apps/makefile.mk
-include build/freertos/common/application_code/makefile.mk
-include tools/Target_tools/makefile.mk
-include tools/tcp_ip_agent_app/makefile.mk
-include lib/makefile.mk
-include include/makefile.mk
endif

#include dependency files for objects in OBJECT_LIST
DEP_LIST = $(OBJECT_LIST:.o=.d)
-include $(DEP_LIST)

FREERTOS_DEP_LIST = $(FREERTOS_OBJECT_LIST:.o=.d)
-include $(FREERTOS_DEP_LIST)

SPECIAL_DEP_LIST = $(SPECIAL_OBJECT_LIST:.o=.d)
-include $(SPECIAL_DEP_LIST)

SU_LIST = $(OBJECT_LIST:.o=.su)

FREERTOS_SU_LIST = $(FREERTOS_OBJECT_LIST:.o=.su)

SPECIAL_SU_LIST = $(SPECIAL_OBJECT_LIST:.o=.su)

PREPROC_LIST = $(OBJECT_LIST:.o=.i)
#TODO - how do we handle dependencies for SPECIAL_OBJECT_LIST?

#*******************************************************************************
# default target when none is specified
#*******************************************************************************
default: all

#*******************************************************************************
# help target to display usage information
#*******************************************************************************
help:
	@echo "============================================================================"
	@echo " "
	@echo "Fermion Build Process Help"
	@echo " "
	@echo "=============================="
	@echo "          Usage"
	@echo "=============================="
	@echo "usage1: make <target>"
	@echo "usage2: make <target> VARIANT_NAME=<variant> BOARD_NAME=<board_name>"
	@echo "usage2: make <target> VARIANT_NAME=<variant> BOARD_NAME=<board_name> BUILD_TYPE=<build_type>"
	@echo " "
	@echo "=============================="
	@echo "     Targets, Variants, Boards"
	@echo "=============================="
	@echo "target: help  - Prints this message"
	@echo "target: all   - Builds the firmware elf image for a specific Variant"
	@echo "target: clean - Deletes the images, objects and dependencies for a specific Variant"
	@echo " "
	@echo "If no variant is specified, VARIANT_NAME defaults to FERMION"
	@echo " "
	@echo "variant : FERMION_PBL         - PBL image"
	@echo "variant : FERMION_IOE_PBL     - IOE_PBL image"
	@echo "variant : FERMION_SBL         - SBL image"
	@echo "variant : FERMION_FTM         - fatory mode image"
	@echo "variant : FERMION_NVM_PROGRAMMER - nvm programmer image"
	@echo "variant : FERMION             - application image with Neutrino console"
	@echo "variant : FERMION_QCLI_DEMO   - application image for qcli_demo"
	@echo "variant : FERMION_HELLO_WORLD - application image for hello_world"
	@echo " "
	@echo "If no board_name is specified, BOARD_NAME defaults to "$(DEFAULT_BOARD_NAME)
	@echo " "
	@echo "board_name : "$(SOCKET_BOARD_CHIPV1)
	@echo "board_name : "$(SOCKET_BOARD_CHIPV2)
	@echo "board_name : "$(EVB_V11_HOSTLESS)
	@echo "board_name : "$(EVB_V12_HOSTLESS)
	@echo "board_name : "$(EVB_V13_HOSTLESS)
	@echo " "
	@echo "=============================="
	@echo "         Build Type"
	@echo "=============================="
	@echo "If no build_type is specified, BUILD_TYPE defaults to DEBUG"
	@echo " "
	@echo "build_type: DEBUG    - Debug Build"
	@echo "build_type: RELEASE  - Release Build"
	@echo " "
	@echo "=============================="
	@echo "        Build output"
	@echo "=============================="
	@echo "Firmware elf and bin image        - Found in build/<variant>/<build_type>/bin"
	@echo "Object files and dependency files - Found in build/<variant>/<build_type>/..."
	@echo " "
	@echo "=============================="
	@echo "          Examples"
	@echo "=============================="
	@echo "Example 1:"
	@echo "         make all"
	@echo "         creates the DEBUG build for the FERMION variant"
	@echo "Example 2:"
	@echo "         make all VARIANT_NAME=FERMION"
	@echo "         creates the DEBUG build for the FERMION variant"
	@echo "Example 3:"
	@echo "         make clean"
	@echo "         cleans the DEBUG build for the FERMION variant"
	@echo "Example 4:"
	@echo "         make clean VARIANT_NAME=FERMION"
	@echo "         cleans the DEBUG build for the NEUTRINO variant"
	@echo "Example 5:"
	@echo "         make all VARIANT_NAME=FERMION BUILD_TYPE=RELEASE"
	@echo "         creates the RELEASE build for the NEUTRINO variant"
	@echo "Example 6:"
	@echo "         make all ENABLE_PREPROC=YES"
	@echo "         generates *.i files (Pre-processed)"
	@echo "============================================================================"



#*******************************************************************************
# Generic make targets
#*******************************************************************************
all: cp_libs
	+@$(MAKE) --no-print-directory pre-build && $(MAKE) --no-print-directory main-build && $(MAKE) --no-print-directory post-build

lib_path = $(SRC_ROOT_DIR)/lib
LIB_FILES := $(wildcard $(lib_path)/*.a)
$(info LIB_FILES=$(LIB_FILES))
ifneq ($(LIB_FILES),)
WLAN_LIBS_NAMES =
INCLUDE_PATHS += \
     -I"$(SRC_ROOT_DIR)/core/wifi/inc"
cp_libs:
	@echo "The following *.a files exist in the lib path: $(LIB_FILES)"
	@mkdir -p $(OBJECT_DIR)/lib
	cp $(LIB_FILES) $(OBJECT_DIR)/lib

else
cp_libs:
	@echo "No *.a files exist in the lib path."
endif

ifeq (,$(filter $(VARIANT_NAME), FERMION FERMION_V2 FERMION_FTM FERMION_FTM_V2 FERMION_QCLI_DEMO FERMION_IOE_QCLI_DEMO FERMION_QCLI_DEMO_V2 FERMION_IOE_QCLI_DEMO_V2 FERMION_HELLO_WORLD FERMION_POSIX_DEMO ))
WLAN_LIBS_NAMES =
WLAN_LIBS =
endif

ifeq ($(strip $(IMAGE_COMBINE)), ON)
PHYRF_TOOLS = core/wifi/halphy/phyrf_svc/tools
BDF_SRC_PATH = $(SRC_ROOT_DIR)/$(PHYRF_TOOLS)/bdfUtils/device/bdf/qcp5321
BDF_OBJ_PATH = $(OBJECT_DIR)/$(PHYRF_TOOLS)/bdfUtils/device/bdf/qcp5321
REGDB_SRC_PATH = $(SRC_ROOT_DIR)/$(PHYRF_TOOLS)/Regulatory/
REGDB_OBJ_PATH = $(OBJECT_DIR)/$(PHYRF_TOOLS)/Regulatory/
endif

$(info WLAN_LIBS_NAMES=$(WLAN_LIBS_NAMES))
$(info WLAN_LIBS=$(WLAN_LIBS))

all_noxml: autoconf
	+@$(MAKE) --no-print-directory main-build && $(MAKE) --no-print-directory post-build

# Main-build Target
main-build: $(EXECUTABLE_NAME) secondary-outputs

pre-build: autoconf
	-python $(DEV_CFG_DIR)/dev_xml_cfg_debug.py $(SRC_ROOT_DIR); python $(DEV_CFG_DIR)/dev_cfg_debug.py $(SRC_ROOT_DIR); python $(SCRIPTS_DIR)/$(PRE_BUILD_SCRIPT) $(SRC_ROOT_DIR) $(VARIANT_NAME) $(VARIANT_IMAGE_ID);
ifeq ($(strip $(VARIANT_IMAGE_TYPE)), FERMION)
ifeq ($(strip $(VARIANT_IMAGE_ID)),MM)
	-python $(SRC_ROOT_DIR)/core/wifi/config_ini/mib/xml_gen_from_xml.py $(SRC_ROOT_DIR)/tools/Target_tools/dev_cfg/export/master_xml.xml > $(SRC_ROOT_DIR)/core/wifi/config_ini/mib/mib.xml
endif
endif

ifeq ($(strip $(IMAGE_COMBINE)), ON)
	mkdir -p $(BDF_OBJ_PATH)
	mkdir -p $(REGDB_OBJ_PATH)

	$(TOOLCHAIN_OBJCOPY) -I binary -O elf32-littlearm --binary-architecture arm --rename-section .data=.regdb $(REGDB_SRC_PATH)/regdb.bin $(REGDB_OBJ_PATH)/regdb.o

ifeq ($(strip $(BOARD_NAME)), $(SOCKET_BOARD_CHIPV1))
	$(TOOLCHAIN_OBJCOPY) -I binary -O elf32-littlearm --binary-architecture arm --rename-section .data=.bdf $(BDF_SRC_PATH)/bdwlan.bin $(BDF_OBJ_PATH)/bdwlan.o
else ifeq ($(strip $(BOARD_NAME)), $(EVB_V11_HOSTLESS))
	$(TOOLCHAIN_OBJCOPY) -I binary -O elf32-littlearm --binary-architecture arm --rename-section .data=.bdf $(BDF_SRC_PATH)/bdwlan01.bin $(BDF_OBJ_PATH)/bdwlan.o
else ifeq ($(strip $(BOARD_NAME)), $(EVB_V12_HOSTLESS))
	$(TOOLCHAIN_OBJCOPY) -I binary -O elf32-littlearm --binary-architecture arm --rename-section .data=.bdf $(BDF_SRC_PATH)/bdwlan02.bin $(BDF_OBJ_PATH)/bdwlan.o
else ifeq ($(strip $(BOARD_NAME)), $(SOCKET_BOARD_CHIPV2))
	$(TOOLCHAIN_OBJCOPY) -I binary -O elf32-littlearm --binary-architecture arm --rename-section .data=.bdf $(BDF_SRC_PATH)/bdwlan02.bin $(BDF_OBJ_PATH)/bdwlan.o
else ifeq ($(strip $(BOARD_NAME)), $(EVB_V13_HOSTLESS))
	$(TOOLCHAIN_OBJCOPY) -I binary -O elf32-littlearm --binary-architecture arm --rename-section .data=.bdf $(BDF_SRC_PATH)/bdwlan03.bin $(BDF_OBJ_PATH)/bdwlan.o
else
    $(error "ERROR: Invalid BOARD_NAME: $(BOARD_NAME).)
endif

endif
	-@echo 'prebuild Done '

post-build:
	-python $(SRC_ROOT_DIR)/build/freertos/eclipse-gcc/Scripts/memap_make.py -d 15 -t GCC_ARM $(EXECUTABLE_DIR)/$(EXECUTABLE_NAME).map > $(EXECUTABLE_DIR)/memory_map_analysis.txt
	-perl $(SRC_ROOT_DIR)/build/freertos/eclipse-gcc/Scripts/avstack.pl $(OBJECT_LIST) $(FREERTOS_OBJECT_LIST) $(SPECIAL_OBJECT_LIST) > $(EXECUTABLE_DIR)/static_stack_analysis.txt
	-@echo 'postbuild Done '

secondary-outputs: $(EXECUTABLE_DIR)/$(SECONDARY_HEX) $(EXECUTABLE_DIR)/$(SECONDARY_SIZE) $(EXECUTABLE_DIR)/$(SECONDARY_BIN)
	-@echo 'secondary outputs Done '

#all: $(EXECUTABLE_NAME)
print-objs:
	-@echo $(OBJECT_LIST)
	-@echo $(FREERTOS_OBJECT_LIST)
	-@echo $(SPECIAL_OBJECT_LIST)
	-@echo $(INCLUDE_PATHS)

clean:
	@echo Cleaning $(EXECUTABLE_NAME)
	-$(RM) $(OBJECT_DIR)
	-$(RM) $(OBJECT_LIST) $(PREPROC_LIST) $(FREERTOS_OBJECT_LIST) $(SPECIAL_OBJECT_LIST) $(DEP_LIST) $(FREERTOS_DEP_LIST) $(SPECIAL_DEP_LIST) $(SU_LIST) $(FREERTOS_SU_LIST) $(SPECIAL_SU_LIST) $(EXECUTABLE_DIR)/*
	-$(RM) $(DOTCONFIG) $(AUTOCONF)
#	$(call FIND_AND_DELETE,$(subst /,$(DIRSEP),$(OBJECT_DIR)),*.o)   > $(NULL_DEVICE) 2>&1
#	$(call FIND_AND_DELETE,$(subst /,$(DIRSEP),$(OBJECT_DIR)),*.d)   > $(NULL_DEVICE) 2>&1
#	-$(DELETE_EXE) "$(subst /,$(DIRSEP),$(EXECUTABLE_DIR)/$(EXECUTABLE_NAME).elf)" > $(NULL_DEVICE) 2>&1
#	-$(DELETE_EXE) "$(subst /,$(DIRSEP),$(EXECUTABLE_DIR)/$(EXECUTABLE_NAME).map)" > $(NULL_DEVICE) 2>&1
	@echo Done Cleaning $(EXECUTABLE_NAME)

#*******************************************************************************
# Executable
#*******************************************************************************
$(EXECUTABLE_NAME):                                                         \
        $(EXECUTABLE_DIR)/$(EXECUTABLE_NAME).elf                            \
        $(FIRMWARE_IMAGES)
	@echo Done.

OBJECT_LIST_FILE=objs_list.txt


$(EXECUTABLE_DIR)/$(EXECUTABLE_NAME).elf: $(OBJECT_LIST) $(FREERTOS_OBJECT_LIST) $(SPECIAL_OBJECT_LIST) $(WLAN_LIBS_NAMES)
	@echo 'Building target: $@'
	@mkdir -p $(@D)
	@echo 'Invoking: Linking $(CONFIG_TOOLCHAIN_LINKER)'
ifeq ($(strip $(CONFIG_TOOLCHAIN_LINKER)), ld)
	@rm -rf $(EXECUTABLE_DIR)/$(OBJECT_LIST_FILE)
	$(foreach f,$(OBJECT_LIST),echo $f>>$(EXECUTABLE_DIR)/$(OBJECT_LIST_FILE);)
	$(foreach f,$(FREERTOS_OBJECT_LIST),echo $f>>$(EXECUTABLE_DIR)/$(OBJECT_LIST_FILE);)
	$(foreach f,$(SPECIAL_OBJECT_LIST),echo $f>>$(EXECUTABLE_DIR)/$(OBJECT_LIST_FILE);)
ifeq ($(strip $(IMAGE_COMBINE)), ON)
	echo "$(BDF_OBJ_PATH)/bdwlan.o" >> $(EXECUTABLE_DIR)/$(OBJECT_LIST_FILE)
	echo "$(REGDB_OBJ_PATH)/regdb.o" >> $(EXECUTABLE_DIR)/$(OBJECT_LIST_FILE)
endif
	$(TOOLCHAIN_LINKER) $(LINKER_FLAGS) $(LINKER_INPUTS) $(LINKER_OUTPUT) $(LINKER_ADDITIONAL_FLAGS) --start-group @$(EXECUTABLE_DIR)/$(OBJECT_LIST_FILE) --end-group -o $(EXECUTABLE_DIR)/$(EXECUTABLE_NAME).elf  -L$(LIB_DIR) $(WLAN_LIBS) $(LIBS) $(EXTERA_LIBS)
else
	$(TOOLCHAIN_LINKER) $(LINKER_FLAGS) $(LINKER_INPUTS) $(LINKER_OUTPUT) $(LINKER_ADDITIONAL_FLAGS) -o $(EXECUTABLE_DIR)/$(EXECUTABLE_NAME).elf $(OBJECT_LIST) $(FREERTOS_OBJECT_LIST) $(SPECIAL_OBJECT_LIST)  -L$(LIB_DIR) $(WLAN_LIBS) $(LIBS) $(EXTERA_LIBS)
endif
	@echo 'Finished building target: $@'
	@echo ' '

$(EXECUTABLE_DIR)/$(SECONDARY_HEX): $(EXECUTABLE_DIR)/$(EXECUTABLE_NAME).elf Makefile $(OPTIONAL_TOOL_DEPS)
	@echo 'Invoking: Creating Hex Image'
	$(TOOLCHAIN_OBJCOPY) -O ihex $(EXECUTABLE_DIR)/$(EXECUTABLE_NAME).elf  $@
	@echo 'Finished building: $@'
	@echo ' '

$(EXECUTABLE_DIR)/$(SECONDARY_SIZE): $(EXECUTABLE_DIR)/$(EXECUTABLE_NAME).elf Makefile $(OPTIONAL_TOOL_DEPS)
	@echo 'Invoking: Creating Print Size'
	$(TOOLCHAIN_ELFSIZE) --format=berkeley --totals $(EXECUTABLE_DIR)/$(EXECUTABLE_NAME).elf
	@echo 'Finished building: $@'
	@echo ' '

$(EXECUTABLE_DIR)/$(SECONDARY_BIN): $(EXECUTABLE_DIR)/$(EXECUTABLE_NAME).elf Makefile $(OPTIONAL_TOOL_DEPS)
	@echo 'Invoking: Creating Bin Image'

ifeq ($(strip $(VARIANT_NAME)), FERMION_NVM_PROGRAMMER)
	mkdir -p ./tools/bin
	cp -rf $(EXECUTABLE_DIR)/$(EXECUTABLE_NAME).elf ./tools/bin/
endif

ifeq ($(strip $(DFU_BUILD)), ON)
	$(TOOLCHAIN_OBJCOPY) $(EXECUTABLE_DIR)/$(EXECUTABLE_NAME).elf --dump-section .image_version=$(EXECUTABLE_DIR)/version.bin
	$(TOOLCHAIN_OBJCOPY) $(EXECUTABLE_DIR)/$(EXECUTABLE_NAME).elf --strip-all $(EXECUTABLE_DIR)/$(EXECUTABLE_NAME)_STRIPPED.elf
	$(TOOLCHAIN_OBJCOPY) -O binary $(EXECUTABLE_DIR)/$(EXECUTABLE_NAME).elf $@
	cat $@ $(EXECUTABLE_DIR)/version.bin > $(EXECUTABLE_DIR)/temp.bin
	$(TOOLCHAIN_OBJCOPY) -I binary -O elf32-littlearm --binary-architecture arm $(EXECUTABLE_DIR)/temp.bin $(EXECUTABLE_DIR)/$(EXECUTABLE_NAME).o
	$(TOOLCHAIN_LD) -m armelf $(EXECUTABLE_DIR)/$(EXECUTABLE_NAME).o -o $(EXECUTABLE_DIR)/$(EXECUTABLE_NAME)_DFU.elf -z max-page-size=1024 -Tdata $(START_ADDRESS)

ifeq ($(strip $(VARIANT_IMAGE_ID)),PBL)
	$(ARMHOME)/bin/fromelf --vhx --32x1 --output=$(EXECUTABLE_DIR)/$(EXECUTABLE_NAME)_32x1.vhx $(EXECUTABLE_DIR)/$(EXECUTABLE_NAME)_DFU.elf
	$(ARMHOME)/bin/fromelf --vhx --128x1 --output=$(EXECUTABLE_DIR)/$(EXECUTABLE_NAME)_128x1.vhx $(EXECUTABLE_DIR)/$(EXECUTABLE_NAME)_DFU.elf
endif
endif
ifeq ($(strip $(CONFIG_BIN_GEN)), ON)
	@echo "post elf processing..."
	$(TOOLCHAIN_OBJCOPY) -O binary  $(EXECUTABLE_DIR)/$(EXECUTABLE_NAME).elf $(EXECUTABLE_DIR)/$(EXECUTABLE_NAME).bin
endif
ifeq ($(strip $(XIP_FLASH_DEMO)), ON)
	$(TOOLCHAIN_OBJCOPY) -O binary $(EXECUTABLE_DIR)/$(EXECUTABLE_NAME).elf  --remove-section=.__sect_xip_flash_txt --remove-section=.xip_flash_ram_data  $(EXECUTABLE_DIR)/FERMION_NO_XIP_DEMO.bin
	$(TOOLCHAIN_OBJCOPY) -O binary $(EXECUTABLE_DIR)/$(EXECUTABLE_NAME).elf  --only-section=.__sect_xip_flash_txt --only-section=.xip_flash_ram_data  $(EXECUTABLE_DIR)/XIP_DEMO.bin
endif
	@echo 'Finished building: $@'
	@echo ' '

$(OBJECT_LIST): $(OBJECT_DIR)/%.o : $(SRC_ROOT_DIR)/%.c
	@echo 'Building file: $<'
	@mkdir -p $(@D)
# filename extracts the current filename from the path, and passes it as an compile time argument to the file being compiled for prepending in logs
	$(eval filename:=$(basename $(notdir $<).c))
	@echo 'Invoking: GNU ARM Cross C Compiler'
ifeq ($(strip $(ENABLE_PREPROC)), YES)
	$(TOOLCHAIN_COMPILER_C) $(COMPILER_FLAGS) $(INCLUDE_PATHS) -DFILENAME=$(filename) -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -s -c -E -o  "$(basename $@).i" "$<"
endif
	$(TOOLCHAIN_COMPILER_C) $(COMPILER_FLAGS) $(INCLUDE_PATHS) -DFILENAME=$(filename) -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -s -c -o  "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

$(FREERTOS_OBJECT_LIST): $(OBJECT_DIR)/%.o : $(SRC_ROOT_DIR)/%.c
	@echo 'Building file: $<'
	@mkdir -p $(@D)
# filename extracts the current filename from the path, and passes it as an compile time argument to the file being compiled for prepending in logs
	$(eval filename:=$(basename $(notdir $<).c))
	@echo 'Invoking: GNU ARM Cross C Compiler'
	$(TOOLCHAIN_COMPILER_C) $(COMPILER_FLAGS) $(INCLUDE_PATHS) -DFILENAME=$(filename) -std=gnu11 -w -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -s -c -o  "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

.PHONY: all clean default help pre-build secondary-outputs main-build post-build print-objs

#-include $(OBJECT_LIST:%.o=%.d)

