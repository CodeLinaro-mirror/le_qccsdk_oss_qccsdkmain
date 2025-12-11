/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#ifndef _QAPI_FIRMWARE_UPGRADE_H_
#define _QAPI_FIRMWARE_UPGRADE_H_

/*----------------------------------------------------------------------------------
 * Include Files
 * -------------------------------------------------------------------------------*/
#include "qapi_types.h"
#include "qapi_status.h"

/*----------------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * -------------------------------------------------------------------------------*/

 /** @addtogroup qapi_Fw_Upgrade
@{ */

/**
 *  Definition used by the qapi_Fw_Upgrade() and qapi_Fw_Upgrade_done() APIs as a flag bit.
 *  The Fw_Upgrade and Fw_Upgrade_Done APIs automatically reboot if this flag bit is set.
 */
#define QAPI_FW_UPGRADE_FLAG_AUTO_REBOOT            (1<<0)

/**
 *  Definition used by the qapi_Fw_Upgrade() API as a flag.
 *  Fw_Upgrade copies files from an active image file system to a trial image file system
 *  if this flag is set
 */
#define QAPI_FW_UPGRADE_FLAG_DUPLICATE_ACTIVE_FS    (1<<1) 

/**
 *  Definition used by the qapi_Fw_Upgrade() API as a flag.
 *  Fw_Upgrade get active image file with range header as a flag for ota_httpc_get_with_offset.
 *  if this flag is set
 */
#define QAPI_FW_UPGRADE_FLAG_RANGE_HEADER           (1<<2) 
 
/** @name FWD Bit Definition
 *  Definition used by the qapi_Fw_Upgrade_Get_Active_FWD() API as a return
 *  to indicate the FWD bit type.
@{ */
#define QAPI_FW_UPGRADE_FWD_BIT_GOLDEN   (0)
#define QAPI_FW_UPGRADE_FWD_BIT_CURRENT  (1)
#define QAPI_FW_UPGRADE_FWD_BIT_TRIAL    (2)
/** @} */
/** @name FWD Boot Type Definition
 *  Definition used by the qapi_Fw_Upgrade_Get_Active_FWD() API as a return
 *  to indicate the FWD type for booting.
@{ */
#define QAPI_FW_UPGRADE_FWD_BOOT_TYPE_GOLDEN	(1<<QAPI_FW_UPGRADE_FWD_BIT_GOLDEN)
#define QAPI_FW_UPGRADE_FWD_BOOT_TYPE_CURRENT	(1<<QAPI_FW_UPGRADE_FWD_BIT_CURRENT)
#define QAPI_FW_UPGRADE_FWD_BOOT_TYPE_TRIAL	    (1<<QAPI_FW_UPGRADE_FWD_BIT_TRIAL)
/** @} */

/** @name FWUP Return Status Definition
 *  Definition used by the firmware upgrade API as a return
 *  to indicate the status.
@{ */
/**< Operation failed. */
#define QAPI_FW_UPGRADE_ERROR                              __QAPI_ERROR(QAPI_MOD_FWUP, 1)
/**< Invalid parameter. */
#define QAPI_FW_UPGRADE_ERR_INVALID_PARAM                  __QAPI_ERROR(QAPI_MOD_FWUP, 2)
/**< Firmware upgrade library is not initialized. */
#define QAPI_FW_UPGRADE_ERR_NOT_INIT                       __QAPI_ERROR(QAPI_MOD_FWUP, 3)
/**< Operation is incomplete. */
#define QAPI_FW_UPGRADE_ERR_INCOMPLETE                     __QAPI_ERROR(QAPI_MOD_FWUP, 4)
/**< Firmware upgrade session is inprogress. */
#define QAPI_FW_UPGRADE_ERR_SESSION_IN_PROGRESS            __QAPI_ERROR(QAPI_MOD_FWUP, 5)
/**< Firmware upgrade session is not started. */
#define QAPI_FW_UPGRADE_ERR_SESSION_NOT_START              __QAPI_ERROR(QAPI_MOD_FWUP, 6)
/**< Firmware upgrade session is not ready to enter the Suspend state. */
#define QAPI_FW_UPGRADE_ERR_SESSION_NOT_READY_FOR_SUSPEND  __QAPI_ERROR(QAPI_MOD_FWUP, 7)
/**< Firmware upgrade session is not in the Suspend state. */
#define QAPI_FW_UPGRADE_ERR_SESSION_NOT_SUSPEND            __QAPI_ERROR(QAPI_MOD_FWUP, 8)
/**< Firmware upgrade session resume is not supported by the plugin. */
#define QAPI_FW_UPGRADE_ERR_SESSION_RESUME_NOT_SUPPORT     __QAPI_ERROR(QAPI_MOD_FWUP, 9)
 /**< Firmware upgrade session was cancelled. */
#define QAPI_FW_UPGRADE_ERR_SESSION_CANCELLED              __QAPI_ERROR(QAPI_MOD_FWUP, 10)
/**< Firmware upgrade session was suspended. */
#define QAPI_FW_UPGRADE_ERR_SESSION_SUSPEND                __QAPI_ERROR(QAPI_MOD_FWUP, 11)
/**< Interface name is too long. */
#define QAPI_FW_UPGRADE_ERR_INTERFACE_NAME_TOO_LONG        __QAPI_ERROR(QAPI_MOD_FWUP, 12)
/**< URL is too long. */
#define QAPI_FW_UPGRADE_ERR_URL_TOO_LONG                   __QAPI_ERROR(QAPI_MOD_FWUP, 13)
/**< Not supported firmware upgrade. */
#define QAPI_FW_UPGRADE_ERR_FLASH_NOT_SUPPORT_FW_UPGRADE   __QAPI_ERROR(QAPI_MOD_FWUP, 14)
/**< Flash initialization timeout. */
#define QAPI_FW_UPGRADE_ERR_FLASH_INIT_TIMEOUT             __QAPI_ERROR(QAPI_MOD_FWUP, 15)
/**< Flash read failure. */
#define QAPI_FW_UPGRADE_ERR_FLASH_READ_FAIL                __QAPI_ERROR(QAPI_MOD_FWUP, 16)
/**< Flash write failure. */
#define QAPI_FW_UPGRADE_ERR_FLASH_WRITE_FAIL               __QAPI_ERROR(QAPI_MOD_FWUP, 17)
/**< Flash erase failure. */
#define QAPI_FW_UPGRADE_ERR_FLASH_ERASE_FAIL               __QAPI_ERROR(QAPI_MOD_FWUP, 18)
/**< Not enough free space in flash. */
#define QAPI_FW_UPGRADE_ERR_FLASH_NOT_ENOUGH_SPACE         __QAPI_ERROR(QAPI_MOD_FWUP, 19)
/**< Partition creation failure. */
#define QAPI_FW_UPGRADE_ERR_FLASH_CREATE_PARTITION         __QAPI_ERROR(QAPI_MOD_FWUP, 20)
/**< Partition image was not found. */
#define QAPI_FW_UPGRADE_ERR_FLASH_IMAGE_NOT_FOUND          __QAPI_ERROR(QAPI_MOD_FWUP, 21)
/**< Partition erase failure. */
#define QAPI_FW_UPGRADE_ERR_FLASH_ERASE_PARTITION          __QAPI_ERROR(QAPI_MOD_FWUP, 22)
/**< Partition write failure. */
#define QAPI_FW_UPGRADE_ERR_FLASH_WRITE_PARTITION          __QAPI_ERROR(QAPI_MOD_FWUP, 23)
/**< NULL partition. */
#define QAPI_FW_UPGRADE_ERR_GET_PARTITION_NULL             __QAPI_ERROR(QAPI_MOD_FWUP, 24)
/**< Reach max image entry. */
#define QAPI_FW_UPGRADE_ERR_REACH_MAX_IMAGE_ENTRY          __QAPI_ERROR(QAPI_MOD_FWUP, 25)
/**< Image Entry is not used */
#define QAPI_FW_UPGRADE_ERR_IMAGE_UNUSED                   __QAPI_ERROR(QAPI_MOD_FWUP, 26)
/**< Image not found failure. */
#define QAPI_FW_UPGRADE_ERR_IMAGE_NOT_FOUND                __QAPI_ERROR(QAPI_MOD_FWUP, 27)
/**< Image download failure. */
#define QAPI_FW_UPGRADE_ERR_IMAGE_DOWNLOAD_FAIL            __QAPI_ERROR(QAPI_MOD_FWUP, 28)
/**< Incorrect image checksum failure. */
#define QAPI_FW_UPGRADE_ERR_INCORRECT_IMAGE_CHECKSUM       __QAPI_ERROR(QAPI_MOD_FWUP, 29)
/**< Server communication timeout. */
#define QAPI_FW_UPGRADE_ERR_SERVER_RSP_TIMEOUT             __QAPI_ERROR(QAPI_MOD_FWUP, 30)
/**< Image file name is invalid. */
#define QAPI_FW_UPGRADE_ERR_INVALID_FILENAME               __QAPI_ERROR(QAPI_MOD_FWUP, 31)
/**< Firmware upgrade image header is invalid. */
#define QAPI_FW_UPGRADE_ERR_INCORRECT_IMAGE_HDR            __QAPI_ERROR(QAPI_MOD_FWUP, 32)
/**< Not enough memory. */
#define QAPI_FW_UPGRADE_ERR_INSUFFICIENT_MEMORY            __QAPI_ERROR(QAPI_MOD_FWUP, 33)
/**< Firmware upgrade image signature is invalid. */
#define QAPI_FW_UPGRADE_ERR_INCORRECT_SIGNATURE            __QAPI_ERROR(QAPI_MOD_FWUP, 34)
/**< Firmware upgrade image version is invalid. */
#define QAPI_FW_UPGRADE_ERR_INCORRCT_VERSION               __QAPI_ERROR(QAPI_MOD_FWUP, 35)
/**< Firmware upgrade image number of images is invalid. */
#define QAPI_FW_UPGRADE_ERR_INCORRECT_NUM_IMAGES           __QAPI_ERROR(QAPI_MOD_FWUP, 36)
/**< Firmware upgrade image length is invalid. */
#define QAPI_FW_UPGRADE_ERR_INCORRECT_IMAGE_LENGTH         __QAPI_ERROR(QAPI_MOD_FWUP, 37)
/**< Firmware upgrade image hash type is invalid. */
#define QAPI_FW_UPGRADE_ERR_INCORRECT_HASH_TYPE            __QAPI_ERROR(QAPI_MOD_FWUP, 38)
/**< Firmware upgrade image ID is invalid. */
#define QAPI_FW_UPGRADE_ERR_INCORRECT_IMAGE_ID             __QAPI_ERROR(QAPI_MOD_FWUP, 39)
/**< SBL upgrade only is not supported. */
#define QAPI_FW_UPGRADE_ERR_SBL_ONLY_NOT_SUPPORT           __QAPI_ERROR(QAPI_MOD_FWUP, 40)
/**< SBL upgrade not supported. */
#define QAPI_FW_UPGRADE_ERR_SBL_NOT_SUPPORT_UPGRADE        __QAPI_ERROR(QAPI_MOD_FWUP, 41)
/**< No enough memory for SBL upgrade. */
#define QAPI_FW_UPGRADE_ERR_SBL_NOT_ENOUGH_SPACE           __QAPI_ERROR(QAPI_MOD_FWUP, 42)
/**< Invalid FDT. */
#define QAPI_FW_UPGRADE_ERR_INVALID_FDT                    __QAPI_ERROR(QAPI_MOD_FWUP, 43)
/**< Battery level is too low. */
#define QAPI_FW_UPGRADE_ERR_BATTERY_LEVEL_TOO_LOW          __QAPI_ERROR(QAPI_MOD_FWUP, 44)
/**< Crypto check failure. */
#define QAPI_FW_UPGRADE_ERR_CRYPTO_FAIL                    __QAPI_ERROR(QAPI_MOD_FWUP, 45)
/**< Firmware upgrade plugin callback is empty. */
#define QAPI_FW_UPGRADE_ERR_PLUGIN_ENTRY_EMPTY             __QAPI_ERROR(QAPI_MOD_FWUP, 46)
/**< Trial image is running */
#define QAPI_FW_UPGRADE_ERR_TRIAL_IS_RUNNING               __QAPI_ERROR(QAPI_MOD_FWUP, 47)
/**< File was not found. */
#define QAPI_FW_UPGRADE_ERR_FILE_NOT_FOUND                 __QAPI_ERROR(QAPI_MOD_FWUP, 48)
/**< Open file failure. */
#define QAPI_FW_UPGRADE_ERR_FILE_OPEN_ERROR                __QAPI_ERROR(QAPI_MOD_FWUP, 49)
/**< File name is too long. */
#define QAPI_FW_UPGRADE_ERR_FILE_NAME_TOO_LONG             __QAPI_ERROR(QAPI_MOD_FWUP, 50)
/**< Write file failure. */
#define QAPI_FW_UPGRADE_ERR_FILE_WRITE_ERROR               __QAPI_ERROR(QAPI_MOD_FWUP, 51)
/**< Mount file system failure. */
#define QAPI_FW_UPGRADE_ERR_MOUNT_FILE_SYSTEM_ERROR        __QAPI_ERROR(QAPI_MOD_FWUP, 52)
/**< Firmware upgrade create thread failure. */
#define QAPI_FW_UPGRADE_ERR_CREATE_THREAD_ERROR            __QAPI_ERROR(QAPI_MOD_FWUP, 53)
#define QAPI_FW_UPGRADE_ERR_PRESERVE_LAST_FAILED           __QAPI_ERROR(QAPI_MOD_FWUP, 54)
/** @} */

/*----------------------------------------------------------------------------------
 * Type Declarations
 * -------------------------------------------------------------------------------*/

/**
 *  Enumeration that represents the various states in Firmware Upgrade state machine.
 */
typedef enum qapi_Fw_Upgrade_State {
    QAPI_FW_UPGRADE_STATE_NOT_START_E = 0,       /**< Firmware upgrade operation is not started. */
    QAPI_FW_UPGRADE_STATE_GET_TRIAL_INFO_E,      /**< Get trial image information at flash. */
    QAPI_FW_UPGRADE_STATE_ERASE_FWD_E,           /**< Erase FWD. */
    QAPI_FW_UPGRADE_STATE_ERASE_FLASH_E,         /**< Erase the partition. */
    QAPI_FW_UPGRADE_STATE_ERASE_SECOND_FS_E,     /**< Erase the second file system. */
    QAPI_FW_UPGRADE_STATE_PREPARE_FS_E,          /**< Prepare the file system. */
    QAPI_FW_UPGRADE_STATE_ERASE_IMAGE_E,         /**< Erase the subimage. */
    QAPI_FW_UPGRADE_STATE_PREPARE_CONNECT_E,     /**< Prepare to connect to a remote firmware upgrade server. */
    QAPI_FW_UPGRADE_STATE_CONNECT_SERVER_E,      /**< Connect to a remote firmware upgrade server. */
    QAPI_FW_UPGRADE_STATE_RESUME_SERVICE_E,      /**< Resume the firmware upgrade service. */
    QAPI_FW_UPGRADE_STATE_RESUME_SERVER_E,       /**< Resume connecting to the firmware upgrade server. */
    QAPI_FW_UPGRADE_STATE_RECEIVE_DATA_E,        /**< Receive data from the remote firmware upgrade server. */
    QAPI_FW_UPGRADE_STATE_DISCONNECT_SERVER_E,   /**< Disconnected from a remote firmware upgrade server. */
    QAPI_FW_UPGRADE_STATE_PROCESS_CONFIG_FILE_E, /**< Process firmware upgrade configuration file. */
    QAPI_FW_UPGRADE_STATE_PROCESS_IMAGE_E,       /**< Process the image. */
    QAPI_FW_UPGRADE_STATE_DUPLICATE_IMAGES_E,    /**< Duplicate the images from the current FWD. */
    QAPI_FW_UPGRADE_STATE_DUPLICATE_FS_E,        /**< Duplicate the file system. */
	QAPI_FW_UPGRADE_STATE_FINISH_E,              /**< Firmware upgrade is done. */
} /** @cond */ qapi_Fw_Upgrade_State_t           /** @endcond */;

/**
 *  Defines an opaque Firmware Partition Handle type.
 *
 * @details A Partition Handle refers to some partition in a valid
 *          or semi-valid (under construction) Firmware Descriptor.
 *
 */
typedef void *qapi_Part_Hdl_t;

/**
 * Declaration of a callback function called by the firmware upgrade state machine. The application implments
 * this callback and passes it as a parameter to the qapi_Fw_Upgrade() API.
 *
 * @param[in] state       Firmware upgrade state machine state defined by enum #qapi_Fw_Upgrade_State_t.
 * @param[in] status      QAPI_OK or error code #QAPI_FW_UPGRADE_ERR_XXX.
 *
 * @return
 * None.
 */
typedef void (*qapi_Fw_Upgrade_CB_t)(int32_t state, int32_t status);

/**
 * Declaration of a callback function called by the firmware upgrade state machine on initalization.
 * The plugin module implements this callback and performs all plugin related initializations.
 * The application passes this callback as a parameter to the qapi_Fw_Upgrade() API.
 *
 * @param[in] interface_Name      Network interface name (plugin dependent).
 * @param[in] url                 Server URL (plugin dependent).
 * @param[in] init_Param          Initialization parameters (plugin dependent).
 *
 * @return
 * Status QAPI_OK or error code #QAPI_FW_UPGRADE_ERR_XXX.
 */
typedef qapi_Status_t (*qapi_Fw_Upgrade_Plugin_Init_t)(const char* interface_Name, const char *url, void *init_Param);

/**
 * Declaration of a callback function called by the firmware upgrade state machine on upgrade completion.
 * The plugin module implements this callback and performs all plugin related cleanup.
 * The application passes this callback as a parameter to the qapi_Fw_Upgrade() API.
 *
 * @return
 * Status QAPI_OK or error code #QAPI_FW_UPGRADE_ERR_XXX.
 */
typedef qapi_Status_t (*qapi_Fw_Upgrade_Plugin_Fin_t)(void);

/**
 * Declaration of a callback function called by the firmware upgrade state machine to receieve a packet from the plugin.
 * The plugin module implements this callback and fills the buffer with incoming data.
 * The application passes this callback as a parameter to the qapi_Fw_Upgrade() API.
 *
 *
 * @param[out] buffer      Receive data buffer.
 * @param[in]  buf_len     Buffer length.
 * @param[out] ret_size    Received data size.
 * @param[in]  init_Param  Optional initialization parameters.
 *
 * @return
 * Status QAPI_OK or error code #QAPI_FW_UPGRADE_ERR_XXX.
 */
typedef qapi_Status_t (*qapi_Fw_Upgrade_Plugin_Recv_Data_t)(uint8_t *buffer, uint32_t buf_len, uint32_t *ret_size, void *init_Param);

/**
 * Declaration of a callback function called by the firmware upgrade state machine to abort a plugin operation.
 * The plugin module implements this callback and aborts connection when invoked.
 * The application passes this callback as a parameter to the qapi_Fw_Upgrade() API.
 *
 * @return
 * Status QAPI_OK or error code #QAPI_FW_UPGRADE_ERR_XXX.
 */
typedef qapi_Status_t (*qapi_Fw_Upgrade_Plugin_Abort_t)(void);

/**
 * Declaration of a callback function called by the firmware upgrade state machine on resume.
 * The plugin module implements this callback and performs all plugin related resumes.
 * The application passes this callback as a parameter to the qapi_Fw_Upgrade() API.
 *
 * @param[in] interface_name      Network interface name (plugin dependent).
 * @param[in] url                 Server URL (plugin dependent).
 * @param[in] offset              Offset to resume the download (plugin dependent).
 *
 * @return
 * Status QAPI_OK or error code #QAPI_FW_UPGRADE_ERR_XXX.
 */
typedef qapi_Status_t (*qapi_Fw_Upgrade_Plugin_Resume_t)(const char* interface_name, const char *url, const uint32_t offset);

/**
 * Represents a set of firmware upgrade plugin callbacks.
 *
 * When the application calls qpai_Fw_Upgrade(), it must fill
 * this structure and pass it to the firmware upgrade engine. The engine calls
 * these firmware upgrade plugin callbacks during different stages of an upgrade.
 */
typedef struct {
    qapi_Fw_Upgrade_Plugin_Init_t      fw_Upgrade_Plugin_Init;
    /**< Callback to initialize a firmware upgrade. */
    qapi_Fw_Upgrade_Plugin_Recv_Data_t fw_Upgrade_Plugin_Recv_Data;
    /**< Callback to retrieve data. */
    qapi_Fw_Upgrade_Plugin_Abort_t     fw_Upgrade_Plugin_Abort;
    /**< Firmware upgrade plugin abort callback. */
    qapi_Fw_Upgrade_Plugin_Resume_t    fw_Upgrade_Plugin_Resume;
    /**< Firmware upgrade plugin resume callback. */
    qapi_Fw_Upgrade_Plugin_Fin_t       fw_Upgrade_Plugin_Fin;
    /**< Firmware upgrade plugin finish callback. */
} qapi_Fw_Upgrade_Plugin_t;

/** @} */ /* end_addtogroup qapi_Fw_Upgrade */

/*----------------------------------------------------------------------------------
 * Function Declarations and Documentation
 * -------------------------------------------------------------------------------*/

/* Miscellaneous Firmware Upgrade APIs */
/** @addtogroup qapi_Fw_Upgrade
@{ */

/**
 * @brief Starts a firmware upgrade session.
 *
 * The caller from the application domain specifies all the required parameters,
 * including the plugin functions, source of the image, and flags.
 * The session automatically ends in the case of an error.
 *
 * @param[in] interface_Name    Network interface name, e.g., wlan1.
 *
 * @param[in] plugin        Parameter of type qapi_Fw_Upgrade_Plugin_t containing
 *                          a set of plugin callback functions.
 *                          For more details, refer to #qapi_Fw_Upgrade_Plugin_t.
 *
 * @param[in] url           Source information for a firmware upgrade. \n
 *                          For FTP: \n
 *                          [user_name]:[password]@[IPV4 address]:[port]  for IPV4 \n
 *                          [user_name]:[password]@|[IPV6 address]|:[port]  for IPV6
 *
 * @param[in] cfg_File      Image file information for a firmware upgrade.
 *
 * @param[in] flags         Flags with bits defined for a firmware upgrade. See the qapi_Fw_Upgrade flag for a definition.
 *
 * @param[in] cb            Optional callback function called by firmware upgrade engine to provide status information.
 *
 * @param[in] init_Param    Optional init parameter passed to the firmware upgrade init function.
 *
 * @return
 * On success, QAPI_OK is returned. \n
 * On error, error code #QAPI_FW_UPGRADE_ERR_XXX is returned.
 */
qapi_Status_t qapi_Fw_Upgrade(char *interface_Name, qapi_Fw_Upgrade_Plugin_t *plugin, char *url, char *cfg_File, uint32_t flags, qapi_Fw_Upgrade_CB_t cb, void *init_Param );

/**
 * @brief Cancels a firmware upgrade session.
 *
 * @return
 * On success, QAPI_OK is returned. \n
 * On error, error code #QAPI_FW_UPGRADE_ERR_XXX is returned.
 */
qapi_Status_t qapi_Fw_Upgrade_Cancel(void);

/**
 * @brief Suspends the firmware upgrade session.
 *
 * @return
 * On success, QAPI_OK is returned. \n
 * On error, error code #QAPI_FW_UPGRADE_ERR_XXX is returned.
 */
qapi_Status_t qapi_Fw_Upgrade_Suspend(void);

/**
 * @brief Resumes the firmware upgrade session.
 *
 * @return
 * On success, QAPI_OK is returned. \n
 * On error, error code #QAPI_FW_UPGRADE_ERR_XXX is returned.
 */
qapi_Status_t qapi_Fw_Upgrade_Resume(void);

/**
 * Activates or invalidates the trial image. The application calls this API after it has verified that
 * the image is valid or invalid. The criteria for image validity is defined by the application.
 *
 * @param[in] result        Result: \n
 *                          1: The image is valid; set trial image to active \n
 *                          0: The image is invalid; invalidate trial image.
 * @param[in] flags         Flags (bit0): \n
 *                          1: The device reboots after activating or invalidating the trial image \n
 *                          0: The device does not reboot after activating or invalidating the trial image \n
 *                          @note If the reboot_flag is set, the device will reboot and there is no return.
 *
 * @return
 * On success, QAPI_OK is returned. \n
 * On error, error code #QAPI_FW_UPGRADE_ERR_XXX is returned.
 */
qapi_Status_t qapi_Fw_Upgrade_Done(uint32_t result, uint32_t flags);

/**
 * @brief Initializes Firmware Upgrade library.
 *
 * @details Reads Firmware Descriptors and populates internal data structures.
 *          Must be called before other firmware descriptor or partition related APIs are used.
 *
 * @return
 * On success, QAPI_OK is returned; on error, error code #QAPI_FW_UPGRADE_ERR_XXX is returned.
 */
qapi_Status_t qapi_Fw_Upgrade_init(void);

/**
 * @brief Erases a firmware descriptor so that an entirely new FWD can be formed in its place.
 *
 * @param[in] FWD_idx    Firmware descriptor index number to be erased.
 *
 * @return
 * On success, QAPI_OK is returned. \n
 * On error, error code #QAPI_FW_UPGRADE_ERR_XXX is returned.
 */
qapi_Status_t qapi_Fw_Upgrade_Erase_FWD(uint8_t FWD_idx);

/**
 * @brief Gets the magic number from a firmware descriptor.
 *
 * @param[in] FWD_idx    Firmware descriptor index number to operate.
 *
 * @param[out] magic     Magic number of the firmware descriptor.
 *
 * @return
 * On success, QAPI_OK is returned. \n
 * On error, error code #QAPI_FW_UPGRADE_ERR_XXX is returned.
 */
qapi_Status_t qapi_Fw_Upgrade_Get_FWD_Magic(uint8_t FWD_idx, uint32_t *magic);

/**
 * @brief Sets the magic number for a firmware descriptor.
 *
 * @param[in] FWD_idx    Firmware descriptor index number to operate.
 *
 * @param[in] magic      Magic number to write to the firmware descriptor.
 *
 * @return
 * On success, QAPI_OK is returned. \n
 * On error, error code #QAPI_FW_UPGRADE_ERR_XXX is returned.
 */
qapi_Status_t qapi_Fw_Upgrade_Set_FWD_Magic(uint8_t FWD_idx, uint32_t magic);

/**
 * @brief Gets the rank number from a firmware descriptor.
 *
 * @param[in] FWD_idx    Firmware descriptor index number to operate.
 *
 * @param[out] rank      Rank number at the firmware descriptor.
 *
 * @return
 * On success, QAPI_OK is returned. \n
 * On error, error code #QAPI_FW_UPGRADE_ERR_XXX is returned.
 */
qapi_Status_t qapi_Fw_Upgrade_Get_FWD_Rank(uint8_t FWD_idx, uint32_t *rank);

/**
 * @brief Gets the version number from a firmware descriptor.
 *
 * @param[in] FWD_idx    Firmware descriptor index number to operate.
 *
 * @param[out] version   Version number of the firmware descriptor.
 *
 * @return
 * On success, QAPI_OK is returned. \n
 * On error, error code #QAPI_FW_UPGRADE_ERR_XXX is returned.
 */
qapi_Status_t qapi_Fw_Upgrade_Get_FWD_Version(uint8_t FWD_idx, uint32_t *version);

/**
 * @brief Gets the status value from a firmware descriptor.
 *
 * @param[in] FWD_idx    Firmware descriptor index number to operate.
 *
 * @param[out] status    Status value of the firmware descriptor.
 *
 * @return
 * On success, QAPI_OK is returned. \n
 * On error, error code #QAPI_FW_UPGRADE_ERR_XXX is returned.
 */
qapi_Status_t qapi_Fw_Upgrade_Get_FWD_Status(uint8_t FWD_idx, uint8_t *status);

/**
 * @brief Gets the total number of images for a firmware descriptor.
 *
 * @param[in] FWD_idx     Firmware descriptor index number to operate.
 *
 * @param[out]image_nums  Image numbers from firmware descriptor.
 *
 * @return
 * On success, QAPI_OK is returned. \n
 * On error, error code #QAPI_FW_UPGRADE_ERR_XXX is returned.
 */
qapi_Status_t qapi_Fw_Upgrade_Get_FWD_Total_Images(uint8_t FWD_idx, uint8_t *image_nums);

/**
 * @brief Gets the FWD index number that is current running.
 *
 * @details This is for the FWD that was selected by the bootloaders and is currently in use.
 *
 * @param[out] fwd_boot_type    Type of FWD used for booting.
 *
 * @param[out] valid_fwd        Information about which FWDs are present (1 bit per FWD).
 *
 *
 * @return
 * The active FWD number.
 */
uint8_t qapi_Fw_Upgrade_Get_Active_FWD(uint32_t *fwd_boot_type, uint32_t *valid_fwd);

/**
 * @brief Releases a partition handle.
 *
 * @details
 *
 * @param[in] hdl       partition handle
 *
 * @return
 * On success, QAPI_OK is returned. \n
 * On error, error code #QAPI_FW_UPGRADE_ERR_XXX is returned.
 */
qapi_Status_t qapi_Fw_Upgrade_Close_Partition(qapi_Part_Hdl_t hdl);

/**
 * @brief Gets a handle for the first partition associated with the specified FWD.
 *
 * @param[in] FWD_idx    Firmware descriptor index number.
 *
 * @param[out] hdl       Partition handle for the partition operation.
 *
 * @return
 * On success, QAPI_OK is returned. \n
 * On error, error code #QAPI_FW_UPGRADE_ERR_XXX is returned.
 */
qapi_Status_t qapi_Fw_Upgrade_First_Partition(uint8_t FWD_idx, qapi_Part_Hdl_t *hdl);

/**
 * @brief Gets the next partition after the current one.
 *
 * @details Guaranteed to be in the same FWD as curr.
 *          This function returns an error when it reaches all blank (uninitialized) partition metadata.
 *
 * @param[in] curr      Current partition handle.
 *
 * @param[out] hdl      Next partition handle.
 *
 * @return
 * On success, QAPI_OK is returned. \n
 * On error, error code #QAPI_FW_UPGRADE_ERR_XXX is returned.
 */
qapi_Status_t qapi_Fw_Upgrade_Next_Partition(qapi_Part_Hdl_t curr, qapi_Part_Hdl_t *hdl);

/**
 * @brief Finds a partition.
 *
 * @details  This function scans the partition table associated with the specified FWD to find
 *           the first partition with the specified ID and get a handle to that
 *           partition.
 *
 * @param[in] FWD_idx    Firmware descriptor index number.
 *
 * @param[in] id         Partition image ID.
 *
 * @param[out] hdl       Partition handle.
 *
 * @return
 * On success, QAPI_OK is returned. \n
 * On error, error code #QAPI_FW_UPGRADE_ERR_XXX is returned.
 */
qapi_Status_t qapi_Fw_Upgrade_Find_Partition(uint8_t FWD_idx, uint32_t id, qapi_Part_Hdl_t *hdl);

/**
 * @brief Gets the image ID associated with a partition in a FWD.
 *
 * @param[in] hdl      Partition handle.
 *
 * @param[out] id      Partition image ID.
 *
 * @return
 * On success, QAPI_OK is returned. \n
 * On error, error code #QAPI_FW_UPGRADE_ERR_XXX is returned.
 */
qapi_Status_t qapi_Fw_Upgrade_Get_Image_ID(qapi_Part_Hdl_t hdl, uint32_t *id);

/**
 * @brief Gets the image version associated with a partition in a FWD.
 *
 * @param[in]  hdl      Partition handle.
 *
 * @param[out] version  Image version to retrieve.
 *
 * @return
 * On success, QAPI_OK is returned. \n
 * On error, error code #QAPI_FW_UPGRADE_ERR_XXX is returned.
 */
qapi_Status_t qapi_Fw_Upgrade_Get_Image_Version(qapi_Part_Hdl_t hdl, uint32_t *version);

/**
 * @brief Gets the size of a partition in a FWD.
 *
 * @param[in] hdl      Partition handle.
 *
 * @param[out] size    Partition image size.
 *
 * @return
 * On success, QAPI_OK is returned. \n
 * On error, error code #QAPI_FW_UPGRADE_ERR_XXX is returned.
 */
qapi_Status_t qapi_Fw_Upgrade_Get_Partition_Size(qapi_Part_Hdl_t hdl, uint32_t *size);

/**
 * @brief Sets size of the associated partition in a FWD to zero (0).
 *
 * @param[in] hdl   Partition handle. Cannot be the handle of APP or SBL image.
 *
 * @param[in] size  Image size to be set. Currently, size can only be 0; otherwise, function returns error code.

 * @detdesc
 * Setting the size of an APP or SBL image is not allowed. Set size to non-zero value is not allowed.
 * Calling this function repeatedly will cause frequent flash writing, which reduces the life of flash (not recommended).
 *
 * @return
 * On success, QAPI_OK is returned. \n
 * On error, error code #QAPI_FW_UPGRADE_ERR_XXX is returned.
 */
qapi_Status_t qapi_Fw_Upgrade_Set_Image_Size(qapi_Part_Hdl_t *hdl, uint32_t size);

/**
 * @brief Gets the start offset of a partition in a FWD.
 *
 * @param[in] hdl      Partition handle.
 *
 * @param[out] start   Start offset.
 *
 * @return
 * On success, QAPI_OK is returned. \n
 * On error, error code #QAPI_FW_UPGRADE_ERR_XXX is returned.
 */
qapi_Status_t qapi_Fw_Upgrade_Get_Partition_Start(qapi_Part_Hdl_t hdl, uint32_t *start);

/**
 * @brief Identifies with which FWD a partition handle is associated.
 *
 * @param[in] hdl        Partition handle.
 *
 * @param[out] FWD_idx   Firmware descriptor index number.
 *
 * On success, QAPI_OK is returned. \n
 * On error, error code #QAPI_FW_UPGRADE_ERR_XXX is returned.
 */
qapi_Status_t qapi_Fw_Upgrade_Get_Partition_FWD(qapi_Part_Hdl_t hdl, uint8_t *FWD_idx);

/**
 * @brief Erases n bytes of memory starting at the specified offset from the
 *           start of the specified partition.
 *
 * @details  Offset and nbytes must be multiples of FLASH_BLOCK_SZ (4 KB) if it is flash area.
 *           This is a partition-relative byte-oriented erase operation.
 *
 * @param[in] hdl       Partition handle.
 *
 * @param[in] offset    Memory offset to erase.
 *
 * @param[in] nbytes    Memory size to erase.
 *
 * @return
 * On success, QAPI_OK is returned. \n
 * On error, error code #QAPI_FW_UPGRADE_ERR_XXX is returned.
 */
qapi_Status_t qapi_Fw_Upgrade_Erase_Partition(qapi_Part_Hdl_t hdl, uint32_t offset, uint32_t nbytes);

/**
 * @brief Writes n bytes from a specified buffer to memory at the specified offset from the
 *           start of the specified partition.
 *
 * @details  This is a partition-relative byte-oriented write operation.
 *           Either the partition must be blank (erased) before this write operation begins
 *           or the write operation must avoid any attempt to change a 0 to a 1 bit if it is flash.
 *           If the final contents  do not match the original buffer, an error is raised.
 *
 * @param[in] hdl       Partition handle.
 *
 * @param[in] offset    Memory offset to write.
 *
 * @param[in] buf       Buffer point to write to memory.
 *
 * @param[in] nbytes    Memory size to write.
 *
 * @return
 * On success, QAPI_OK is returned. \n
 * On error, error code #QAPI_FW_UPGRADE_ERR_XXX is returned.
 */
qapi_Status_t qapi_Fw_Upgrade_Write_Partition(qapi_Part_Hdl_t hdl, uint32_t offset, char *buf, uint32_t nbytes);

/**
 * @brief Reads up to max_bytes into the specified buffer from memory at the specified offset
 *           from the start of the specified partition.
 *
 * @details  This is a partition-relative byte-oriented read operation.
 *
 * @param[in] hdl        Partition handle.
 *
 * @param[in] offset     Memory offset to read.
 *
 * @param[in] buf        Buffer point to store the memory data.
 *
 * @param[in] max_bytes  Size to read.
 *
 * @param[out] nbytes    Actual memory read size in buf.
 *
 * @return
 * On success, QAPI_OK is returned. \n
 * On error, error code #QAPI_FW_UPGRADE_ERR_XXX is returned.
 */
qapi_Status_t qapi_Fw_Upgrade_Read_Partition(qapi_Part_Hdl_t hdl, uint32_t offset, char *buf, uint32_t max_bytes, uint32_t *nbytes);

/** @} */ /* end_addtogroup qapi_Fw_Upgrade */
#endif /* _QAPI_FIRMWARE_UPGRADE_H_ */
