/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/


#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "qapi_firmware_upgrade.h"
#include "qurt_internal.h"
#include "fw_upgrade.h"
#include "fw_upgrade_mem.h"
#include "nt_sys_monitoring.h"

/**********************************************************************************************************/
/* Preprocessor Definitions and Constants											                      */
/**********************************************************************************************************/
#define FWUP_ErrorMap(status)   (status ? __QAPI_ERROR(QAPI_MOD_FWUP, status) : QAPI_OK)

/**********************************************************************************************************/
/* Global Variables											                                              */
/**********************************************************************************************************/
static qapi_Fw_Upgrade_CB_t fw_upgrade_cb = NULL;

/**********************************************************************************************************/
/* External Functions																                      */
/**********************************************************************************************************/
#ifdef CONFIG_QAT_OTA_DEMO
extern void qat_common_rst_timer_callback();
#endif

/**********************************************************************************************************/
/* Internal Functions												                                      */
/**********************************************************************************************************/

static void Fw_Upgrade_Callback_Handle(int32_t state, int32_t status)
{
    if (fw_upgrade_cb != NULL) {
        if (status >= FW_UPGRADE_OK_E && status < FW_UPGRADE_ERR_PRESERVE_LAST_FAILED_E) {
            status = FWUP_ErrorMap(status);
        }
        fw_upgrade_cb(state, status);
    }
}

/**********************************************************************************************************/
/*    												                                                      */
/**********************************************************************************************************/

/**
 * @brief Initializes Firmware Upgrade library.
 *
 * @details Reads Firmware Descriptors and populates internal data structures.
 *          Must be called before other firmware descriptor or partition related APIs are used.
 *
 * @return
 * On success, QAPI_OK is returned; on error, error code #QAPI_FW_UPGRADE_ERR_XXX is returned.
 */
qapi_Status_t qapi_Fw_Upgrade_init(void)
{
    fw_upgrade_status_code_t ret;
    ret = fw_upgrade_init();
    return FWUP_ErrorMap(ret);
}

/**
 * @brief Erases a firmware descriptor so that an entirely new FWD can be formed in its place.
 *
 * @param[in] FWD_idx    Firmware descriptor index number to be erased.
 *
 * @return
 * On success, QAPI_OK is returned. \n
 * On error, error code #QAPI_FW_UPGRADE_ERR_XXX is returned.
 */
qapi_Status_t qapi_Fw_Upgrade_Erase_FWD(uint8_t FWD_idx)
{
    fw_upgrade_status_code_t ret;

    ret = fw_upgrade_erase_fwd(FWD_idx);
    return FWUP_ErrorMap(ret);
}

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
qapi_Status_t qapi_Fw_Upgrade_Get_FWD_Magic(uint8_t FWD_idx, uint32_t *magic)
{
    fw_upgrade_status_code_t ret;

    ret = fw_upgrade_get_fwd_info(FWD_idx, FW_UPGRADE_FWD_MAGIC_E, (uint8_t *)magic);
    return FWUP_ErrorMap(ret);
}

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
qapi_Status_t qapi_Fw_Upgrade_Set_FWD_Magic(uint8_t FWD_idx, uint32_t magic)
{
    fw_upgrade_status_code_t ret;

    ret = fw_upgrade_set_fwd_info(FWD_idx, FW_UPGRADE_FWD_MAGIC_E, (uint8_t *)&magic);
    return FWUP_ErrorMap(ret);
}

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
qapi_Status_t qapi_Fw_Upgrade_Get_FWD_Rank(uint8_t FWD_idx, uint32_t *rank)
{
    fw_upgrade_status_code_t ret;

    ret = fw_upgrade_get_fwd_info(FWD_idx, FW_UPGRADE_FWD_RANK_E, (uint8_t *)rank);
    return FWUP_ErrorMap(ret);
}

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
qapi_Status_t qapi_Fw_Upgrade_Get_FWD_Version(uint8_t FWD_idx, uint32_t *version)
{
    fw_upgrade_status_code_t ret;

    ret = fw_upgrade_get_fwd_info(FWD_idx, FW_UPGRADE_FWD_VERSION_E, (uint8_t *)version);
    return FWUP_ErrorMap(ret);
}

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
qapi_Status_t qapi_Fw_Upgrade_Get_FWD_Status(uint8_t FWD_idx, uint8_t *status)
{
    fw_upgrade_status_code_t ret;

    ret = fw_upgrade_get_fwd_info(FWD_idx, FW_UPGRADE_FWD_STATUS_E, (uint8_t *)status);
    return FWUP_ErrorMap(ret);
}

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
qapi_Status_t qapi_Fw_Upgrade_Get_FWD_Total_Images(uint8_t FWD_idx, uint8_t *image_nums)
{
    fw_upgrade_status_code_t ret;

    ret = fw_upgrade_get_fwd_info(FWD_idx, FW_UPGRADE_FWD_TOTAL_IMAGE_E, (uint8_t *)image_nums);
    return FWUP_ErrorMap(ret);
}

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
uint8_t qapi_Fw_Upgrade_Get_Active_FWD(uint32_t *fwd_boot_type, uint32_t *valid_fwd)
{
    return fw_upgrade_get_active_fwd(fwd_boot_type, valid_fwd);
}

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
qapi_Status_t qapi_Fw_Upgrade_Close_Partition(qapi_Part_Hdl_t hdl)
{
    if (fw_upgrade_partition_handle_validation(hdl) != FW_UPGRADE_OK_E)
        return QAPI_ERR_INVALID_PARAM;

    ((fu_partition_client_t *)hdl)->ref_count = 0;

    return QAPI_OK;
}

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
qapi_Status_t qapi_Fw_Upgrade_First_Partition(uint8_t FWD_idx, qapi_Part_Hdl_t *hdl)
{
    fw_upgrade_status_code_t ret;

    ret = fw_upgrade_first_partition(FWD_idx, (fu_part_hdl_t *)hdl);
    return FWUP_ErrorMap(ret);
}

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
qapi_Status_t qapi_Fw_Upgrade_Next_Partition(qapi_Part_Hdl_t curr, qapi_Part_Hdl_t *hdl)
{
    fw_upgrade_status_code_t ret;

    ret = fw_upgrade_next_partition((fu_part_hdl_t)curr, (fu_part_hdl_t *)hdl);
    return FWUP_ErrorMap(ret);
}

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
qapi_Status_t qapi_Fw_Upgrade_Find_Partition(uint8_t FWD_idx, uint32_t id, qapi_Part_Hdl_t *hdl)
{
    fw_upgrade_status_code_t ret;

    ret = fw_upgrade_find_partition(FWD_idx, id, (fu_part_hdl_t *)hdl);
    return FWUP_ErrorMap(ret);
}

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
qapi_Status_t qapi_Fw_Upgrade_Get_Image_ID(qapi_Part_Hdl_t hdl, uint32_t *id)
{
    fu_partition_client_t *h;

    if (fw_upgrade_partition_handle_validation(hdl) != FW_UPGRADE_OK_E)
        return QAPI_ERR_INVALID_PARAM;

    h = (fu_partition_client_t *)hdl;
    *id = h->img_id;
    return QAPI_OK;
}

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
qapi_Status_t qapi_Fw_Upgrade_Get_Image_Version(qapi_Part_Hdl_t hdl, uint32_t *version)
{
    fu_partition_client_t *h = NULL;

    if (fw_upgrade_partition_handle_validation(hdl) != FW_UPGRADE_OK_E)
        return QAPI_ERR_INVALID_PARAM;

    h = (fu_partition_client_t *)hdl;
    *version = h->img_version;
    return QAPI_OK;
}

qapi_Status_t qapi_Fw_Upgrade_Set_Image_Version(qapi_Part_Hdl_t *hdl, uint32_t version)
{
    fw_upgrade_status_code_t ret;

    ret = fw_upgrade_set_image_version((fu_part_hdl_t)hdl, version);
    return FWUP_ErrorMap(ret);
}

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
qapi_Status_t qapi_Fw_Upgrade_Get_Partition_Size(qapi_Part_Hdl_t hdl, uint32_t *size)
{
    fu_partition_client_t *h;

    if (fw_upgrade_partition_handle_validation(hdl) != FW_UPGRADE_OK_E)
        return QAPI_ERR_INVALID_PARAM;

    h = (fu_partition_client_t *)hdl;
    *size = h->img_size;
    return QAPI_OK;
}

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
qapi_Status_t qapi_Fw_Upgrade_Set_Image_Size(qapi_Part_Hdl_t *hdl, uint32_t size)
{
    fw_upgrade_status_code_t ret;

    ret = fw_upgrade_set_image_size((fu_part_hdl_t)hdl, size);
    return FWUP_ErrorMap(ret);
}

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
qapi_Status_t qapi_Fw_Upgrade_Get_Partition_Start(qapi_Part_Hdl_t hdl, uint32_t *start)
{
    fu_partition_client_t *h;

    if (fw_upgrade_partition_handle_validation(hdl) != FW_UPGRADE_OK_E)
        return QAPI_ERR_INVALID_PARAM;

    h = (fu_partition_client_t *)hdl;
    *start = h->img_start;
    return QAPI_OK;
}

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
qapi_Status_t qapi_Fw_Upgrade_Get_Partition_FWD(qapi_Part_Hdl_t hdl, uint8_t *FWD_idx)
{
    fu_partition_client_t *h;

    if (fw_upgrade_partition_handle_validation(hdl) != FW_UPGRADE_OK_E)
        return QAPI_ERR_INVALID_PARAM;

    h = (fu_partition_client_t *)hdl;
    *FWD_idx = h->fwd_idx;
    return QAPI_OK;
}

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
qapi_Status_t qapi_Fw_Upgrade_Erase_Partition(qapi_Part_Hdl_t hdl, uint32_t offset, uint32_t nbytes)
{
    fw_upgrade_status_code_t ret;

    ret = fw_upgrade_erase_partition((fu_part_hdl_t)hdl, offset, nbytes);
    return FWUP_ErrorMap(ret);
}

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
qapi_Status_t qapi_Fw_Upgrade_Write_Partition(qapi_Part_Hdl_t hdl, uint32_t offset, char *buf, uint32_t nbytes)
{
    fw_upgrade_status_code_t ret;

    ret = fw_upgrade_write_partition((fu_part_hdl_t)hdl, offset, buf, nbytes);
    return FWUP_ErrorMap(ret);
}

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
qapi_Status_t qapi_Fw_Upgrade_Read_Partition(qapi_Part_Hdl_t hdl, uint32_t offset, char *buf, uint32_t max_bytes, uint32_t *nbytes)
{
    fw_upgrade_status_code_t ret;

    ret = fw_upgrade_read_partition((fu_part_hdl_t)hdl, offset, buf, max_bytes, nbytes);
    return FWUP_ErrorMap(ret);
}

/**
 * @brief    Check active FWD to invalidate trial fwd if need
 *
 * @details  if system boots from current or golden FWD and trial FWD is present,
 *           need to invalidate trial FWD.
 *
 * @return On success, QAPI_OK is returned. On error, #QAPI_FW_UPGRADE_ERR_XXX is returned.
 */
qapi_Status_t qapi_Fw_Upgrade_Verify_FWD()
{
	/* check active FWD to invalidate trial fwd if need                */
    uint32_t fwd_boot_type, valid_fwd;
    uint8_t current;
    fw_upgrade_status_code_t ret = FW_UPGRADE_OK_E;;

    ret = fw_upgrade_init();
    if (ret != FW_UPGRADE_OK_E) {
        goto verify_fwd_end;
    }

	fw_upgrade_get_active_fwd(&fwd_boot_type, &valid_fwd);

    /* if boot from current or golden and trial fwd is present, need to invalidate trial */
    if ((fwd_boot_type != QAPI_FW_UPGRADE_FWD_BOOT_TYPE_TRIAL) && ((valid_fwd & (1<<QAPI_FW_UPGRADE_FWD_BIT_TRIAL)) != 0) ) {
	    /* invalidate trial fwd */
		fw_upgrade_reject_trial_fwd();
	} else if ((fwd_boot_type == QAPI_FW_UPGRADE_FWD_BOOT_TYPE_GOLDEN) && ((valid_fwd & (1<<QAPI_FW_UPGRADE_FWD_BIT_CURRENT)) != 0)) {
        /* if boot from golden and current fwd is present,  */
        if (fw_upgrade_get_current_index(&current) == FW_UPGRADE_OK_E) {
            //TODO wdog
            /* reset wdog reset counter */
            //qapi_System_WDTCount_Reset();
            /* delete current fwd */
            fw_upgrade_erase_fwd(current);
        }
    }

verify_fwd_end:
    return FWUP_ErrorMap(ret);
}

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
qapi_Status_t qapi_Fw_Upgrade(char *interface_Name, qapi_Fw_Upgrade_Plugin_t *plugin, char *url, char *cfg_File, uint32_t flags, qapi_Fw_Upgrade_CB_t cb, void *init_Param)
{
    int32_t ret;

    if (fw_upgrade_cb == NULL) {
        fw_upgrade_cb = cb;
    }
    ret = fw_upgrade(interface_Name, (fw_upgrade_plugin_t*)plugin, url, cfg_File, flags, (fw_upgrade_cb_t)Fw_Upgrade_Callback_Handle, init_Param);
    if (ret != FW_UPGRADE_ERR_SESSION_SUSPEND_E) {
        fw_upgrade_cb = NULL;
    }
    if (ret >= FW_UPGRADE_OK_E && ret < FW_UPGRADE_ERR_PRESERVE_LAST_FAILED_E) {
        ret = FWUP_ErrorMap(ret);
    }
    return ret;
}

/**
 * @brief Cancels a firmware upgrade session.
 *
 * @return
 * On success, QAPI_OK is returned. \n
 * On error, error code #QAPI_FW_UPGRADE_ERR_XXX is returned.
 */
qapi_Status_t qapi_Fw_Upgrade_Cancel(void)
{
    fw_upgrade_status_code_t ret;
    ret = fw_upgrade_session_cancel();
    fw_upgrade_cb = NULL;
	return FWUP_ErrorMap(ret);
}

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
qapi_Status_t qapi_Fw_Upgrade_Done(uint32_t result, uint32_t flags)
{
    fw_upgrade_status_code_t ret = FW_UPGRADE_ERROR_E;
#ifdef CONFIG_QAT_OTA_DEMO
    TimerHandle_t rst_timer_handle;
#endif

    ret = fw_upgrade_session_done(result);

    /* check reboot flag here */
    if ((ret == FW_UPGRADE_OK_E) && ((flags & QAPI_FW_UPGRADE_FLAG_AUTO_REBOOT) != 0)) {   
#ifdef CONFIG_QAT_OTA_DEMO
        //when using QAT demo, need some times to send response to SPI host
        rst_timer_handle = nt_qurt_timer_create("rst_timer", 100, TRUE,
			NULL, qat_common_rst_timer_callback);
        qurt_timer_start(rst_timer_handle, 0);
#else
        /* reboot system here ..... */
        nt_system_sw_reset();
#endif
    }

    return FWUP_ErrorMap(ret);
}

/**
 * @brief Suspends the firmware upgrade session.
 *
 * @return
 * On success, QAPI_OK is returned. \n
 * On error, error code #QAPI_FW_UPGRADE_ERR_XXX is returned.
 */
qapi_Status_t qapi_Fw_Upgrade_Suspend(void)
{
    fw_upgrade_status_code_t ret;
    ret = fw_upgrade_session_suspend();
	return FWUP_ErrorMap(ret);
}

/**
 * @brief Resumes the firmware upgrade session.
 *
 * @return
 * On success, QAPI_OK is returned. \n
 * On error, error code #QAPI_FW_UPGRADE_ERR_XXX is returned.
 */
qapi_Status_t qapi_Fw_Upgrade_Resume(void)
{
    int32_t ret;
    ret = fw_upgrade_session_resume();
    if (ret != FW_UPGRADE_ERR_SESSION_SUSPEND_E) {
        fw_upgrade_cb = NULL;
    }
    if (ret >= FW_UPGRADE_OK_E && ret < FW_UPGRADE_ERR_PRESERVE_LAST_FAILED_E) {
        ret = FWUP_ErrorMap(ret);
    }
    return ret;
}

