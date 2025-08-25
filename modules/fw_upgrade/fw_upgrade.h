/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _FW_UPGRADE_H
#define _FW_UPGRADE_H

/**********************************************************************************************************/
/* Include Files 																			              */
/**********************************************************************************************************/
#include "sha256.h"
#include "fw_upgrade_mem.h"

/**********************************************************************************************************/
/* Preprocessor Definitions and Constants																  */
/**********************************************************************************************************/
#define FW_UPGRADE_BUF_SIZE               2048
#define FW_UPGRADE_HASH_LEN               32
#define FW_UPGRADE_INTERFACE_NAME_LEN     32
#define FW_UPGRADE_URL_LEN                256
#define FW_UPGRADE_FILENAME_LEN           128
#define FW_UPGRADE_URL_TOTAL_LEN          (FW_UPGRADE_URL_LEN + FW_UPGRADE_FILENAME_LEN)
#define FW_UPGRADE_MAX_IMAGES_NUM         30
#define FW_UPGRADE_FORAMT_PARTIAL_UPGRADE 1

#define FLASH_ERASED_VALUE 0xFFFFFFFF

/**********************************************************************************************************/
/* Type Declarations																                      */
/**********************************************************************************************************/
/*
 * Enumeration that represents fw upgrade session status.
 */
typedef enum {
    FW_UPGRADE_SESSION_NOT_START_E = 0,
    FW_UPGRADE_SESSION_RUNNING_E,
    FW_UPGRADE_SESSION_SUSPEND_E,
    FW_UPGRADE_SESSION_CANCEL_E,
    FW_UPGRADE_SESSION_ERROR_E,
} fw_upgrade_session_status_t;

/**
 *  Enumeration that represents the various states in firmware upgrade state machine.
 */
typedef enum {
    FW_UPGRADE_STATE_NOT_START_E = 0,       /**< Firmware upgrade operation is not started. */
    FW_UPGRADE_STATE_GET_TRIAL_INFO_E,      /**< Get trial image information at flash. */
    FW_UPGRADE_STATE_ERASE_FWD_E,           /**< Erase FWD. */
    FW_UPGRADE_STATE_ERASE_FLASH_E,         /**< Erase the partition. */
    FW_UPGRADE_STATE_ERASE_SECOND_FS_E,     /**< Erase the second file system. */
    FW_UPGRADE_STATE_PREPARE_FS_E,          /**< Prepare the file system. */
    FW_UPGRADE_STATE_ERASE_IMAGE_E,         /**< Erase the subimage. */
    FW_UPGRADE_STATE_PREPARE_CONNECT_E,     /**< Prepare to connect to a remote firmware upgrade server. */
    FW_UPGRADE_STATE_CONNECT_SERVER_E,      /**< Connect to a remote firmware upgrade server. */
    FW_UPGRADE_STATE_RESUME_SERVICE_E,      /**< Resume the firmware upgrade service. */
    FW_UPGRADE_STATE_RESUME_SERVER_E,       /**< Resume connecting to the firmware upgrade server. */
    FW_UPGRADE_STATE_RECEIVE_DATA_E,        /**< Receive data from the remote firmware upgrade server. */
    FW_UPGRADE_STATE_DISCONNECT_SERVER_E,   /**< Disconnected from a remote firmware upgrade server. */
    FW_UPGRADE_STATE_PROCESS_CONFIG_FILE_E, /**< Process firmware upgrade configuration file. */
    FW_UPGRADE_STATE_PROCESS_IMAGE_E,       /**< Process the image. */
    FW_UPGRADE_STATE_DUPLICATE_IMAGES_E,    /**< Duplicate the images from the current FWD. */
    FW_UPGRADE_STATE_DUPLICATE_FS_E,        /**< Duplicate the file system. */
    FW_UPGRADE_STATE_FINISH_E,              /**< Firmware upgrade is done. */
} fw_upgrade_state_t;

/**
 * Declaration of a callback function called by the firmware upgrade state machine. The application implments
 * this callback and passes it as a parameter to the fw_upgrade() API.
 *
 * @param[in] state       Firmware upgrade state machine state.
 * @param[in] status      Error code defined by enum #fw_upgrade_status_code_t.
 *
 * @return
 * None.
 */
typedef void (*fw_upgrade_cb_t)(int32_t state, int32_t status);

/**
 * Declaration of a callback function called by the firmware upgrade state machine on initalization.
 * The plugin module implements this callback and performs all plugin related initializations.
 * The application passes this callback as a parameter to the fw_upgrade() API.
 *
 * @param[in] interface_Name      Network interface name (plugin dependent).
 * @param[in] url                 Server URL (plugin dependent).
 * @param[in] init_Param          Initialization parameters (plugin dependent).
 *
 * @return
 * Status QAPI_OK or error code #QAPI_FW_UPGRADE_ERR_XXX.
 */
typedef qapi_Status_t (*fw_upgrade_plugin_init_t)(const char *interface_name, const char *url, void *init_param);

/**
 * Declaration of a callback function called by the firmware upgrade state machine on upgrade completion.
 * The plugin module implements this callback and performs all plugin related cleanup.
 * The application passes this callback as a parameter to the fw_upgrade() API.
 *
 * @param[in] interface_name      Network interface name (plugin dependent).
 * @param[in] url                 Server URL (plugin dependent).
 *
 * @return
 * Status QAPI_OK or error code #QAPI_FW_UPGRADE_ERR_XXX.
 */
typedef qapi_Status_t (*fw_upgrade_plugin_fin_t)(void);

/**
 * Declaration of a callback function called by the firmware upgrade state machine to receieve a packet from the plugin.
 * The plugin module implements this callback and fills the buffer with incoming data.
 * The application passes this callback as a parameter to the fw_upgrade() API.
 *
 *
 * @param[out] buffer      Receive data buffer.
 * @param[in]  buf_len     Buffer length.
 * @param[out] ret_size    Received data size.
 *
 * @return
 * Status QAPI_OK or error code #QAPI_FW_UPGRADE_ERR_XXX.
 */
typedef qapi_Status_t (*fw_upgrade_plugin_recv_data_t)(uint8_t *buffer, uint32_t buf_len, uint32_t *ret_size,
                                                       void *init_param);

/**
 * Declaration of a callback function called by the firmware upgrade state machine to abort a plugin operation.
 * The plugin module implements this callback and aborts connection when invoked.
 * The application passes this callback as a parameter to the fw_upgrade() API.
 *
 * @return
 * Status QAPI_OK or error code #QAPI_FW_UPGRADE_ERR_XXX.
 */
typedef qapi_Status_t (*fw_upgrade_plugin_abort_t)(void);

/**
 * Declaration of a callback function called by the firmware upgrade state machine on resume.
 * The plugin module implements this callback and performs all plugin related resumes.
 * The application passes this callback as a parameter to the fw_upgrade() API.
 *
 * @param[in] interface_name      Network interface name (plugin dependent).
 * @param[in] url                 Server URL (plugin dependent).
 * @param[in] offset              Offset to resume the download (plugin dependent).
 *
 * @return
 * Status QAPI_OK or error code #QAPI_FW_UPGRADE_ERR_XXX.
 */
typedef qapi_Status_t (*fw_upgrade_plugin_resume_t)(const char *interface_name, const char *url, const uint32_t offset);

/**
 * Represents a set of firmware upgrade plugin callbacks.
 *
 * When the application calls fw_upgrade(), it must fill
 * this structure and pass it to the firmware upgrade engine. The engine calls
 * these firmware upgrade plugin callbacks during different stages of an upgrade.
 */
typedef struct {
    fw_upgrade_plugin_init_t fw_upgrade_plugin_init;
    /**< Callback to initialize a firmware upgrade. */
    fw_upgrade_plugin_recv_data_t fw_upgrade_plugin_recv_data;
    /**< Callback to retrieve data. */
    fw_upgrade_plugin_abort_t fw_upgrade_plugin_abort;
    /**< Firmware upgrade plugin abort callback. */
    fw_upgrade_plugin_resume_t fw_upgrade_plugin_resume;
    /**< Firmware upgrade plugin resume callback. */
    fw_upgrade_plugin_fin_t fw_upgrade_plugin_fin;
    /**< Firmware upgrade plugin finish callback. */
} fw_upgrade_plugin_t;

/*
 * Firmware Upgrade ImageSet Header Structure
 */
typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t format;
    uint32_t length;
    uint8_t num_images;
} __attribute__((packed)) fw_upgrade_imageSet_hdr_part1_t;

/*
 * Firmware Upgrade Sub Image Header Structure
 */
typedef struct {
    uint32_t magic;
    uint32_t image_id;
    uint32_t version;
    uint8_t image_file[FW_UPGRADE_FILENAME_LEN];
    uint32_t disk_size;
    uint32_t image_length;
    uint32_t hash_type;
    uint8_t hash[FW_UPGRADE_HASH_LEN];
} __attribute__((packed)) fw_upgrade_image_hdr_t;

/*
 * Data context for firmware upgrade session
 */
typedef struct {
    int32_t error_code;
    uint8_t is_first;

    uint32_t buf_len;    /* total available buffer length */
    uint32_t buf_offset; /* processed buffer length */

    uint32_t image_index;         /* image index number */
    uint32_t image_wrt_count;     /* image flashed length */
    uint32_t image_wrt_length;    /* image total length */
    uint32_t total_images;        /* total number of images */
    uint32_t file_read_count;     /* received length from remote file */
    uint32_t hidden_images_count; /* count of hidden images which is not downloaded by OTA, such as FS2 or RAMDUMP */

    fw_upgrade_state_t fw_upgrade_state;                   /* fw upgrade session state */
    fw_upgrade_session_status_t fw_upgrade_session_status; /* fw upgrade session status */
    uint8_t download_flag[FW_UPGRADE_MAX_IMAGES_NUM];      /* mark for download image or duplicate image from current */

    char url[FW_UPGRADE_URL_LEN];                       /* stored URL */
    char cfg_file[FW_UPGRADE_URL_LEN];                  /* stored config filw name */
    char interface_name[FW_UPGRADE_INTERFACE_NAME_LEN]; /* store interface name */
    void *init_param;                                   /* store init_param */

    uint32_t flags;
    uint32_t format;          /* 1: partial fw upgrade, 2: all-in-one fw upgrade */
    uint32_t trial_mem_start; /* available memory start address to store upgraded images except SBL */
    uint32_t trial_mem_size;  /* trial partition size in memory */
    uint8_t trial_fwd_idx;    /* trial FWD number */
    uint32_t trial_sbl_start; /* available start address to store upgrade sbl iamge. */
    uint32_t trial_sbl_size;  /* trial sbl size. */
    uint32_t trial_sbl_idx;   /* trial SBL FDE number */
    fu_part_hdl_t partition_hdl;

    fw_upgrade_plugin_t plugin;
    fw_upgrade_cb_t fw_upgrade_cb;
    mbedtls_sha256_context *digest_ctx;
    uint8_t *config_buf; /* buffer to store config file before parse */
} fw_upgrade_context_t;

/**********************************************************************************************************/
/* Function Declarations																                  */
/**********************************************************************************************************/
/*
 * start OTA upgrade session
 */
int32_t fw_upgrade(char *interface_name, fw_upgrade_plugin_t *plugin, char *url, char *cfg_file, uint32_t flags,
                   fw_upgrade_cb_t cb, void *init_param);

/*
 * cancel OTA session
 */
fw_upgrade_status_code_t fw_upgrade_session_cancel(void);

/*
 * process result of OTA session
 */
fw_upgrade_status_code_t fw_upgrade_session_done(uint32_t result);

/*
 * suspend OTA session
 */
fw_upgrade_status_code_t fw_upgrade_session_suspend(void);

/*
 * resume OTA session
 */
int32_t fw_upgrade_session_resume(void);

#endif /* _FW_UPGRADE_H */
