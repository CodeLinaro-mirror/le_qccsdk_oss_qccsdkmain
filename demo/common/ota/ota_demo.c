/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <stdlib.h>
#include "string.h"
#include "qapi_types.h"
#include "qapi_status.h"
#include "qapi_firmware_upgrade.h"
#include "qcli.h"
#include "qcli_api.h"
#include "qcli_pal.h"
#include "qcli_util.h"
#include "ota_tftp.h"
#include "qurt_internal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "safeAPI.h"

/**********************************************************************************************************/
/* Preprocessor Definitions and Constants											                      */
/**********************************************************************************************************/
#define FW_UPGRADE_PRINTF_HANDLE qcli_fw_upgrade_group
#define UNUSED(x)                (void)(x)

/**********************************************************************************************************/
/* Type Declarations											                                          */
/**********************************************************************************************************/
typedef struct {
    char *interface_name;
    char *url;
    char *cfg_file;
    uint32_t flags;
} fw_upgrade_params_t;

/**********************************************************************************************************/
/* Globals											                                                      */
/**********************************************************************************************************/
QAPI_Console_Group_Handle_t qcli_fw_upgrade_group; /* Handle for our QCLI Command Group. */
fw_upgrade_params_t *upgrade_params = NULL;
TaskHandle_t fw_upgrade_task_handle = NULL;
TaskHandle_t fw_upgrade_resume_task_handle = NULL;

/**********************************************************************************************************/
/* Function Declarations											                                      */
/**********************************************************************************************************/
static qapi_Status_t Command_Display_FWD(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List);
static qapi_Status_t Command_Delete_FWD(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List);
static qapi_Status_t Command_Done_Trial(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List);
static qapi_Status_t Command_Display_ActiveImage(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List);
static qapi_Status_t Command_Fw_Upgrade_Cancel(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List);
static qapi_Status_t Command_Fw_Upgrade_Suspend(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List);
static qapi_Status_t Command_Fw_Upgrade_Resume(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List);
static qapi_Status_t Command_Fw_Upgrade_TFTP_Upgrade(uint32_t Parameter_Count,
                                                     QAPI_Console_Parameter_t *Parameter_List);
static qapi_Status_t Command_Fw_Set_Image_Size(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List);

/* The following is the complete command list for the Firmware Upgrade demo. */
const QAPI_Console_Command_t Fw_Upgrade_Command_List[] = {
    /* cmd_function                     cmd_string usage_string                    description */
    {Command_Display_FWD, "fwd", "", "Display FWD"},
    {Command_Delete_FWD, "del", "[fwd]", "Erase FWD"},
    {Command_Done_Trial, "trial", "[1|0] [reboot flag]", "Accept/Reject Trial FWD"},
    {Command_Display_ActiveImage, "img", "[id]", "Display Active FWD Image Info"},
    {Command_Fw_Upgrade_TFTP_Upgrade, "tftp", "[if] [server] [file] [flag]",
     "tftp [if_name] [tftp_server] [fw filename] [flag]\r\n"},
    {Command_Fw_Upgrade_Cancel, "cancel", "", "cancel fw upgrade\r\n"},
    {Command_Fw_Upgrade_Suspend, "suspend", "", "suspend fw upgrade\r\n"},
    {Command_Fw_Upgrade_Resume, "resume", "", "resume fw upgrade\r\n"},
    {Command_Fw_Set_Image_Size, "setsize", "[fwd] [id] [size]", "set image size\r\n"},
};

const QAPI_Console_Command_Group_t Fw_Upgrade_Command_Group = {
    "FwUp", /* Firmware Upgrade */
    sizeof(Fw_Upgrade_Command_List) / sizeof(QAPI_Console_Command_t),
    Fw_Upgrade_Command_List,
};

/**********************************************************************************************************/
/* Function Definitions    											                                      */
/**********************************************************************************************************/
/* This function is used to register the Firmware Upgrade Command Group with QCLI   */
void Initialize_FwUpgrade_Demo(void)
{
    /* Attempt to reqister the Command Groups with the qcli framework.*/
    FW_UPGRADE_PRINTF_HANDLE = QAPI_Console_Register_Command_Group(NULL, &Fw_Upgrade_Command_Group);
    if (FW_UPGRADE_PRINTF_HANDLE) {
        QCLI_Printf(FW_UPGRADE_PRINTF_HANDLE, "Firmware Upgrade Registered \n");
    }
}

/*=================================================================================================*/

void display_sub_image_info(qapi_Part_Hdl_t hdl)
{
    uint32_t result = 0;

    qapi_Fw_Upgrade_Get_Image_ID(hdl, &result);
    QCLI_Printf(FW_UPGRADE_PRINTF_HANDLE, "        Image ID: 0x%X\r\n", result);
    qapi_Fw_Upgrade_Get_Image_Version(hdl, &result);
    QCLI_Printf(FW_UPGRADE_PRINTF_HANDLE, "   Image Version: 0x%X\r\n", result);
    qapi_Fw_Upgrade_Get_Partition_Start(hdl, &result);
    QCLI_Printf(FW_UPGRADE_PRINTF_HANDLE, "     Image Start: 0x%X\r\n", result);
    qapi_Fw_Upgrade_Get_Partition_Size(hdl, &result);
    QCLI_Printf(FW_UPGRADE_PRINTF_HANDLE, "      Image Size: 0x%X\r\n", result);
}

/**
   @brief This function processes the "FWD" command from the CLI.
*/
static qapi_Status_t Command_Display_FWD(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    int32_t Index;
    uint32_t boot_type, fwd_present, magic = 0, Result_u32 = 0;
    uint8_t Result_u8 = 0;
    qapi_Part_Hdl_t hdl = NULL, hdl_next;

    UNUSED(Parameter_Count);
    UNUSED(Parameter_List);

    if (qapi_Fw_Upgrade_init() != QAPI_OK) {
        QCLI_Printf(FW_UPGRADE_PRINTF_HANDLE, "FU Init Error\r\n");
        return QAPI_ERROR;
    }

    // get active FWD
    Index = qapi_Fw_Upgrade_Get_Active_FWD(&boot_type, &fwd_present);
    QCLI_Printf(FW_UPGRADE_PRINTF_HANDLE, "Active FWD: %s  index:%d, present:%d\r\n",
                (boot_type == QAPI_FW_UPGRADE_FWD_BOOT_TYPE_TRIAL)     ? "Trial"
                : (boot_type == QAPI_FW_UPGRADE_FWD_BOOT_TYPE_CURRENT) ? "Current"
                                                                       : "Golden",
                Index, fwd_present);

    for (Index = 0; Index < 3; Index++) {
        QCLI_Printf(FW_UPGRADE_PRINTF_HANDLE, "FWD %d\r\n", Index);

        qapi_Fw_Upgrade_Get_FWD_Magic(Index, &magic);
        QCLI_Printf(FW_UPGRADE_PRINTF_HANDLE, "Magic: 0x%X\r\n", magic);

        qapi_Fw_Upgrade_Get_FWD_Rank(Index, &Result_u32);
        QCLI_Printf(FW_UPGRADE_PRINTF_HANDLE, "Rank: 0x%X\r\n", Result_u32);

        qapi_Fw_Upgrade_Get_FWD_Version(Index, &Result_u32);
        QCLI_Printf(FW_UPGRADE_PRINTF_HANDLE, "Version: 0x%X\r\n", Result_u32);

        qapi_Fw_Upgrade_Get_FWD_Status(Index, &Result_u8);
        QCLI_Printf(FW_UPGRADE_PRINTF_HANDLE, "Status: 0x%X\r\n", Result_u8);

        qapi_Fw_Upgrade_Get_FWD_Total_Images(Index, &Result_u8);
        QCLI_Printf(FW_UPGRADE_PRINTF_HANDLE, "Total Images: 0x%X\r\n", Result_u8);

        if (magic == 0x54445746) {
            if (qapi_Fw_Upgrade_First_Partition(Index, &hdl) == QAPI_OK) {
                display_sub_image_info(hdl);
                while (qapi_Fw_Upgrade_Next_Partition(hdl, &hdl_next) == QAPI_OK) {
                    qapi_Fw_Upgrade_Close_Partition(hdl);
                    hdl = hdl_next;
                    display_sub_image_info(hdl);
                }
                qapi_Fw_Upgrade_Close_Partition(hdl);
            }
        }

        QCLI_Printf(FW_UPGRADE_PRINTF_HANDLE, "\r\n");
    }

    return QAPI_OK;
}

/**
   @brief This function processes the "DEL" command from the CLI.
*/
static qapi_Status_t Command_Delete_FWD(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    int32_t Index;

    if (Parameter_Count != 1) {
        QCLI_Printf(FW_UPGRADE_PRINTF_HANDLE, "usage: del [fwd num]\r\n");
        return QAPI_ERROR;
    }

    if (Parameter_List[0].Integer_Is_Valid == 0) {
        QCLI_Printf(FW_UPGRADE_PRINTF_HANDLE, "usage: del [fwd num]\r\n");
        return QAPI_ERROR;
    }

    if (Parameter_List[0].Integer_Value > 2) {
        QCLI_Printf(FW_UPGRADE_PRINTF_HANDLE, "invalid fwd number\r\n");
        return QAPI_ERROR;
    }

    Index = Parameter_List[0].Integer_Value;
    QCLI_Printf(FW_UPGRADE_PRINTF_HANDLE, "Delete FWD%d\r\n", Index);

    if (qapi_Fw_Upgrade_init() != QAPI_OK) {
        QCLI_Printf(FW_UPGRADE_PRINTF_HANDLE, "\r\nFU Init Error\r\n");
        return QAPI_ERROR;
    }

    if (qapi_Fw_Upgrade_Erase_FWD(Index) != QAPI_OK) {
        QCLI_Printf(FW_UPGRADE_PRINTF_HANDLE, "\r\nErase Error\r\n");
        return QAPI_ERROR;
    }

    QCLI_Printf(FW_UPGRADE_PRINTF_HANDLE, "done\r\n");

    return QAPI_OK;
}

/**
   @brief This function processes the "Trial" command from the CLI.

*/
static qapi_Status_t Command_Done_Trial(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    if ((Parameter_Count != 2) || (Parameter_List[0].Integer_Is_Valid == 0) ||
        (Parameter_List[1].Integer_Is_Valid == 0)) {
        QCLI_Printf(FW_UPGRADE_PRINTF_HANDLE, "usage: trial [1|0] [reboot flag]\r\n");
        return QAPI_ERR_INVALID_PARAM;
    }

    if ((Parameter_List[0].Integer_Value > 1) || (Parameter_List[1].Integer_Value > 1)) {
        QCLI_Printf(FW_UPGRADE_PRINTF_HANDLE, "Invalid parameter\r\n");
        return QAPI_ERR_INVALID_PARAM;
    }

    if (Parameter_List[0].Integer_Value == 1) {
        if (qapi_Fw_Upgrade_Done(1, Parameter_List[1].Integer_Value) != QAPI_OK) {
            QCLI_Printf(FW_UPGRADE_PRINTF_HANDLE, "Fail to Accept Trial FWD\r\n");
            return QAPI_ERROR;
        } else {
            QCLI_Printf(FW_UPGRADE_PRINTF_HANDLE, "Success to Accept Trial FWD\r\n");
        }
    } else {
        if (qapi_Fw_Upgrade_Done(0, Parameter_List[1].Integer_Value) != QAPI_OK) {
            QCLI_Printf(FW_UPGRADE_PRINTF_HANDLE, "Fail to Reject Trial FWD\r\n");
            return QAPI_ERROR;
        } else {
            QCLI_Printf(FW_UPGRADE_PRINTF_HANDLE, "Success to Reject Trial FWD\r\n");
        }
    }
    return QAPI_OK;
}

/**
 * display image info
 */
static void Display_Image_Info(qapi_Part_Hdl_t hdl)
{
    uint32_t i = 0, id = 0, size = 0, start = 0;

    // get image id
    if (qapi_Fw_Upgrade_Get_Image_ID(hdl, &id) != QAPI_OK) {
        QCLI_Printf(FW_UPGRADE_PRINTF_HANDLE, "Failed to get image id\r\n");
        return;
    } else {
        QCLI_Printf(FW_UPGRADE_PRINTF_HANDLE, "Image id is 0x%x(%d)\r\n", id, id);
    }

    // get image version
    if (qapi_Fw_Upgrade_Get_Image_Version(hdl, &id) != QAPI_OK) {
        QCLI_Printf(FW_UPGRADE_PRINTF_HANDLE, "failed to get image version\r\n");
        return;
    } else {
        QCLI_Printf(FW_UPGRADE_PRINTF_HANDLE, "Image version is 0x%x(%d)\r\n", id, id);
    }

    // get image start address at flash
    if (qapi_Fw_Upgrade_Get_Partition_Start(hdl, &start) != QAPI_OK) {
        QCLI_Printf(FW_UPGRADE_PRINTF_HANDLE, "Failed to get image start address\r\n");
        return;
    } else {
        QCLI_Printf(FW_UPGRADE_PRINTF_HANDLE, "Image start address is 0x%x(%d)\r\n", start, start);
    }

    // get image size
    if (qapi_Fw_Upgrade_Get_Partition_Size(hdl, &size) != QAPI_OK) {
        QCLI_Printf(FW_UPGRADE_PRINTF_HANDLE, "Failed to get image size\r\n");
        return;
    } else {
        QCLI_Printf(FW_UPGRADE_PRINTF_HANDLE, "Image size is 0x%x(%d)\r\n", size, size);
    }

    {
        // read image from flash
#define MYBUF_SIZE 32
        char buf[MYBUF_SIZE] = {'\0'};
        ;
        if (qapi_Fw_Upgrade_Read_Partition(hdl, 0, buf, MYBUF_SIZE, &size) != QAPI_OK) {
            QCLI_Printf(FW_UPGRADE_PRINTF_HANDLE, "Failed to read image\r\n");
            return;
        } else {
            QCLI_Printf(FW_UPGRADE_PRINTF_HANDLE, "Image:\r\n");
            for (i = 0; i < MYBUF_SIZE; i++)
                QCLI_Printf(FW_UPGRADE_PRINTF_HANDLE, "%02x,", buf[i]);
            QCLI_Printf(FW_UPGRADE_PRINTF_HANDLE, "\r\n");
        }
    }
    return;
}

/**
   @brief This function processes the "Img" command from the CLI.
*/
static qapi_Status_t Command_Display_ActiveImage(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    qapi_Status_t Ret_Val = QAPI_ERROR;
    uint8_t Index;
    uint32_t id;
    uint32_t boot_type, fwd_present;
    qapi_Part_Hdl_t hdl = NULL, hdl1;

    if (qapi_Fw_Upgrade_init() != QAPI_OK) {
        QCLI_Printf(FW_UPGRADE_PRINTF_HANDLE, "FU Init Error\r\n");
        return QAPI_ERROR;
    }

    if (Parameter_Count != 1) {
        QCLI_Printf(FW_UPGRADE_PRINTF_HANDLE, "usage: img [id]\r\n");
        goto cmd_img_end;
    }

    if (Parameter_List[0].Integer_Is_Valid == 0) {
        QCLI_Printf(FW_UPGRADE_PRINTF_HANDLE, "usage: img [id]\r\n");
        goto cmd_img_end;
    }

    id = Parameter_List[0].Integer_Value;

    // get active FWD
    Index = qapi_Fw_Upgrade_Get_Active_FWD(&boot_type, &fwd_present);
    QCLI_Printf(FW_UPGRADE_PRINTF_HANDLE, "Active FWD: %s  index:%d, present:%d\r\n",
                (boot_type == QAPI_FW_UPGRADE_FWD_BOOT_TYPE_TRIAL)     ? "Trial"
                : (boot_type == QAPI_FW_UPGRADE_FWD_BOOT_TYPE_CURRENT) ? "Current"
                                                                       : "Golden",
                Index, fwd_present);

    if (id != 0) {
        // get hdl based on img id
        if (qapi_Fw_Upgrade_Find_Partition(Index, id, &hdl) != QAPI_OK) {
            QCLI_Printf(FW_UPGRADE_PRINTF_HANDLE, "fail to locate the image with id %d\r\n", id);
            goto cmd_img_end;
        }
        Display_Image_Info(hdl);
    } else {
        // get and display all images at current FWD
        if (qapi_Fw_Upgrade_First_Partition(Index, &hdl) != QAPI_OK) {
            QCLI_Printf(FW_UPGRADE_PRINTF_HANDLE, "fail to locate the first image\r\n");
            goto cmd_img_end;
        }

        Display_Image_Info(hdl);
        while (qapi_Fw_Upgrade_Next_Partition(hdl, &hdl1) == QAPI_OK) {
            qapi_Fw_Upgrade_Close_Partition(hdl);
            hdl = hdl1;
            Display_Image_Info(hdl);
        }
        qapi_Fw_Upgrade_Close_Partition(hdl);
        hdl = NULL;
    }

    Ret_Val = QAPI_OK;

cmd_img_end:
    if (hdl != NULL)
        qapi_Fw_Upgrade_Close_Partition(hdl);

    return (Ret_Val);
}

/****************************************************************************************/

void fw_upgrade_callback(int32_t state, int32_t status)
{
    QCLI_Printf(FW_UPGRADE_PRINTF_HANDLE, "%d %d\r\n", state, status);
}

static void fw_upgrade_free_memory()
{
    if (upgrade_params != NULL) {
        if (upgrade_params->interface_name != NULL) {
            free(upgrade_params->interface_name);
            upgrade_params->interface_name = NULL;
        }
        if (upgrade_params->url != NULL) {
            free(upgrade_params->url);
            upgrade_params->url = NULL;
        }
        if (upgrade_params->cfg_file != NULL) {
            free(upgrade_params->cfg_file);
            upgrade_params->cfg_file = NULL;
        }
        free(upgrade_params);
        upgrade_params = NULL;
    }
}

static qapi_Status_t Command_Fw_Upgrade_Cancel(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    qapi_Status_t resp_code;

    UNUSED(Parameter_Count);
    UNUSED(Parameter_List);

    resp_code = qapi_Fw_Upgrade_Cancel();

    if (QAPI_OK != resp_code) {
        QCLI_Printf(FW_UPGRADE_PRINTF_HANDLE, "Fail to cancel Firmware Upgrade Download: ERR:%d\r\n", resp_code);
        return QAPI_ERROR;
    } else {
        QCLI_Printf(FW_UPGRADE_PRINTF_HANDLE, "OK to cancel Firmware Upgrade Download\r\n");
        return QAPI_OK;
    }
}

static qapi_Status_t Command_Fw_Upgrade_Suspend(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    qapi_Status_t resp_code;

    UNUSED(Parameter_Count);
    UNUSED(Parameter_List);

    resp_code = qapi_Fw_Upgrade_Suspend();

    if (QAPI_OK != resp_code) {
        QCLI_Printf(FW_UPGRADE_PRINTF_HANDLE, "Fail to suspend Firmware Upgrade Download: ERR:%d\r\n", resp_code);
        return QAPI_ERROR;
    } else {
        QCLI_Printf(FW_UPGRADE_PRINTF_HANDLE, "OK to suspend Firmware Upgrade Download\r\n");
        return QAPI_OK;
    }
}

static void fw_upgrade_resume_upgrade_task(void __attribute__((__unused__)) * pvParameters)
{
    qapi_Status_t resp_code;

    resp_code = qapi_Fw_Upgrade_Resume();

    if (QAPI_OK != resp_code) {
        QCLI_Printf(FW_UPGRADE_PRINTF_HANDLE, "Fail to resume Firmware Upgrade Download: ERR:%d\r\n", resp_code);
    } else {
        QCLI_Printf(FW_UPGRADE_PRINTF_HANDLE, "Firmware Upgrade Image Download Completed successfully\r\n");
    }

    vTaskDelete(NULL);
    fw_upgrade_resume_task_handle = NULL;
}

static qapi_Status_t Command_Fw_Upgrade_Resume(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    UNUSED(Parameter_Count);
    UNUSED(Parameter_List);

    nt_qurt_thread_create(fw_upgrade_resume_upgrade_task, "fw_upgrade_resume", 1024, NULL,
                          configUART_COMMAND_CONSOLE_TASK_PRIORITY, &fw_upgrade_resume_task_handle);
    return QAPI_OK;
}

static void fw_upgrade_TFTP_upgrade_task(void __attribute__((__unused__)) * pvParameters)
{
    qapi_Status_t resp_code;
    qapi_Fw_Upgrade_Plugin_t plugin = {plugin_tftp_init, plugin_tftp_recv_data, plugin_tftp_abort, plugin_tftp_resume,
                                       plugin_tftp_fin};

    if (upgrade_params == NULL) {
        goto tftp_thread_end;
    }
    resp_code = qapi_Fw_Upgrade(upgrade_params->interface_name, &plugin, upgrade_params->url, upgrade_params->cfg_file,
                                upgrade_params->flags, fw_upgrade_callback, NULL);

    if (QAPI_OK != resp_code) {
        QCLI_Printf(FW_UPGRADE_PRINTF_HANDLE, "Firmware Upgrade Image Download Failed ERR:%d\r\n", resp_code);

        if (resp_code == QAPI_FW_UPGRADE_ERR_TRIAL_IS_RUNNING) {
            QCLI_Printf(FW_UPGRADE_PRINTF_HANDLE,
                        "Trial Partition is running, need reboot to do Firmware Upgrade.\r\n");
        }
    } else {
        QCLI_Printf(FW_UPGRADE_PRINTF_HANDLE, "Firmware Upgrade Image Download Completed successfully\r\n");
    }

tftp_thread_end:
    fw_upgrade_free_memory();
    vTaskDelete(NULL);
}

/**
   @brief This function processes the "TFTP Firmware Upgrade" command from the CLI.
*/
static qapi_Status_t Command_Fw_Upgrade_TFTP_Upgrade(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    qapi_Status_t ret = QAPI_OK;
    uint32_t interface_len;
    uint32_t url_len;
    uint32_t cfg_len;
    uint32_t flags = (QAPI_FW_UPGRADE_FLAG_AUTO_REBOOT | QAPI_FW_UPGRADE_FLAG_DUPLICATE_ACTIVE_FS);

    if (Parameter_Count != 3 && Parameter_Count != 4) {
        QCLI_Printf(FW_UPGRADE_PRINTF_HANDLE, "usage: tftp [if_name] [tftp_server] [fw filename] [flag]\r\n");
        return QAPI_ERR_INVALID_PARAM;
    }

    if (Parameter_Count == 4 && ((!Parameter_List[3].Integer_Is_Valid) || (!(Parameter_List[3].Integer_Value & flags) &&
                                                                           Parameter_List[3].Integer_Value != 0))) {
        QCLI_Printf(FW_UPGRADE_PRINTF_HANDLE, "Flag bit0: auto reboot, bit1: dup fs\r\n");
        return QAPI_ERR_INVALID_PARAM;
    }

    if (upgrade_params != NULL) {
        QCLI_Printf(FW_UPGRADE_PRINTF_HANDLE, "fw upgrade is ongoing\r\n");
        return QAPI_ERROR;
    }

    upgrade_params = malloc(sizeof(fw_upgrade_params_t));
    if (upgrade_params == NULL) {
        return QAPI_ERROR;
    }
    memset(upgrade_params, 0, sizeof(fw_upgrade_params_t));
    interface_len = strlen(Parameter_List[0].String_Value) + 1;
    upgrade_params->interface_name = malloc(interface_len);
    if (upgrade_params->interface_name == NULL) {
        ret = QAPI_ERR_NO_MEMORY;
        goto tftp_upgrade_end;
    }
    memset(upgrade_params->interface_name, 0, interface_len);
    memscpy(upgrade_params->interface_name, interface_len, Parameter_List[0].String_Value, interface_len);

    url_len = strlen(Parameter_List[1].String_Value) + 1;
    upgrade_params->url = malloc(url_len);
    if (upgrade_params->url == NULL) {
        ret = QAPI_ERR_NO_MEMORY;
        goto tftp_upgrade_end;
    }
    memset(upgrade_params->url, 0, url_len);
    memscpy(upgrade_params->url, url_len, Parameter_List[1].String_Value, url_len);

    cfg_len = strlen(Parameter_List[2].String_Value) + 1;
    upgrade_params->cfg_file = malloc(cfg_len);
    if (upgrade_params->cfg_file == NULL) {
        ret = QAPI_ERR_NO_MEMORY;
        goto tftp_upgrade_end;
    }
    memset(upgrade_params->cfg_file, 0, cfg_len);
    memscpy(upgrade_params->cfg_file, cfg_len, Parameter_List[2].String_Value, cfg_len);

    if (Parameter_Count == 3) {
        upgrade_params->flags = QAPI_FW_UPGRADE_FLAG_AUTO_REBOOT;
    } else if (Parameter_Count == 4) {
        upgrade_params->flags = Parameter_List[3].Integer_Value;
    }

    nt_qurt_thread_create(fw_upgrade_TFTP_upgrade_task, "fw_upgrade_demo", 1024, NULL,
                          configUART_COMMAND_CONSOLE_TASK_PRIORITY, &fw_upgrade_task_handle);

tftp_upgrade_end:
    if (ret != QAPI_OK) {
        fw_upgrade_free_memory();
    }
    return ret;
}

/**
   @brief This function processes the "Set Image Size" command from the CLI.
*/
static qapi_Status_t Command_Fw_Set_Image_Size(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    uint32_t id, size;
    uint8_t index;
    qapi_Part_Hdl_t hdl = NULL;

    if (Parameter_Count != 3) {
        QCLI_Printf(FW_UPGRADE_PRINTF_HANDLE, "usage: setsize [fwd num] [image id] [size]\r\n");
        return QAPI_ERROR;
    }

    if ((Parameter_List[0].Integer_Is_Valid == 0) || (Parameter_List[1].Integer_Is_Valid == 0) ||
        (Parameter_List[2].Integer_Is_Valid == 0)) {
        QCLI_Printf(FW_UPGRADE_PRINTF_HANDLE, "usage: setsize [fwd num] [image id] [size]\r\n");
        return QAPI_ERROR;
    }

    if (Parameter_List[0].Integer_Value > 2) {
        QCLI_Printf(FW_UPGRADE_PRINTF_HANDLE, "invalid fwd number\r\n");
        return QAPI_ERROR;
    }

    if (Parameter_List[2].Integer_Value != 0) {
        QCLI_Printf(FW_UPGRADE_PRINTF_HANDLE, "size can only be 0 for now\r\n");
        return QAPI_ERROR;
    }

    index = Parameter_List[0].Integer_Value;
    id = Parameter_List[1].Integer_Value;
    size = Parameter_List[2].Integer_Value;

    if (qapi_Fw_Upgrade_init() != QAPI_OK) {
        QCLI_Printf(FW_UPGRADE_PRINTF_HANDLE, "FU Init Error\r\n");
        return QAPI_ERROR;
    }

    if (qapi_Fw_Upgrade_Find_Partition(index, id, &hdl) != QAPI_OK) {
        QCLI_Printf(FW_UPGRADE_PRINTF_HANDLE, "fail to locate the image with id %d\r\n", id);
        return QAPI_ERROR;
    }

    if (qapi_Fw_Upgrade_Set_Image_Size(hdl, size) != QAPI_OK) {
        QCLI_Printf(FW_UPGRADE_PRINTF_HANDLE, "fail to set the size of image with id %d\r\n", id);
        return QAPI_ERROR;
    }

    return QAPI_OK;
}
