/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*-------------------------------------------------------------------------
 * Include Files
 *-----------------------------------------------------------------------*/

#include <string.h>
#include <stdio.h>

#include "qurt_internal.h"
#include "qurt_mutex.h"

#include "qcli.h"
#include "qcli_util.h"
#include "qcli_pal.h"

#include "uart.h"

#include "qapi_types.h"
#include "qapi_version.h"
#include "qapi_status.h"
#include "qapi_console.h"

#include "qc_heap.h"

#include "FreeRTOS.h"
#include "task.h"

#include "qccsdk_console.h"

#include "wifi_fw_version.h"
#ifdef CONFIG_RTT_VIEW_CLI
#include "SEGGER_RTT.h"
#endif
/*-------------------------------------------------------------------------
 * Type Declarations
 *-----------------------------------------------------------------------*/

/**
   Context information for the PAL module.
*/
typedef struct PAL_Context_s {
    QAPI_Console_Group_Handle_t QCLI_Handle; /**< QCLI handle for printing to the console. */
    qurt_mutex_t Mutex;
} PAL_Context_t;

TaskHandle_t QCLI_Task_handle; /**< Task for QCLI processing. */
#ifdef CONFIG_RTT_VIEW_CLI
TaskHandle_t QCLI_RTT_CLI_Task_handle; /**< Task for QCLI RTT CLI processing. */
#endif

/*-------------------------------------------------------------------------
 * Static & global Variable Declarations
 *-----------------------------------------------------------------------*/

static PAL_Context_t PAL_Context;

/**
   @brief Writes a buffer to the console.

   All data from this buffer will be written to the console or buffered locally
   before this function returns.

   @param Length[in]  Length of the data to be written.
   @param Buffer[in]  Buffer to be written to the console.
*/
void PAL_Console_Write(uint32_t Length, const char *Buffer)
{
    if ((Length != 0) && (Buffer != NULL)) {
        FreeRTOS_UART_write(Buffer, Length);
    }
}

/**
   @brief Allocates a block of memory from the heap.

   @param[in] Size  Minimum size of the memory block to allocate.

   @return A pointer to the allocated memory or NULL if there was an error.
*/
void *PAL_Malloc(size_t Length)
{
    return pvPortMalloc(Length);
}

/**
   @brief Frees a block of memory from the heap.

   @param[in] Pointer  Block to free as returned by a call to PAL_Malloc().
*/
void PAL_Free(void *Pointer)
{
    vPortFree(Pointer);
}

/**
   @brief Takes a PAL_RELEASE_LOCK() for re-entrancy protection for the QCLI module.

   This module expects the PAL_RELEASE_LOCK() to behave like a mutex. It should support
   recursively taking the PAL_RELEASE_LOCK() and, for threaded platforms, block until the PAL_RELEASE_LOCK()
   can be taken as applicable.

   @return
    - true if the PAL_RELEASE_LOCK() was taken successfully
    - false if the PAL_RELEASE_LOCK() was not taken successfully
*/
qbool_t PAL_Take_Lock(void)
{
    qurt_mutex_lock(&PAL_Context.Mutex);
    return true;
}

/**
   @brief Releases a PAL_RELEASE_LOCK() taken with PAL_Take_Lock().

   @param[in] Taken  Flag indicating the lock was taken in the corresponding
                     call to PAL_Take_Lock();
    - true if the PAL_RELEASE_LOCK() was taken successfully
    - false if the PAL_RELEASE_LOCK() was not taken successfully
*/
void PAL_Release_Lock(void)
{
    qurt_mutex_unlock(&PAL_Context.Mutex);
}

#define cmdMAX_INPUT_SIZE      500
#define ENTER_KEY              13
#define BACKSPACE_KEY_PUTTY    127
#define BACKSPACE_KEY_TERATERM 8
extern int uart_flag;

static uint8_t utf8_char_display_width(const char *buf, uint16_t start)
{
    uint8_t b = (uint8_t)buf[start];
    /* Lead byte range 0xE4-0xE9 covers the main CJK Unified Ideographs block. */
    if (b >= 0xE4 && b <= 0xE9) {
        return 2;
    }
    return 1;
}

static void QCLI_Task(void __attribute__((__unused__)) * pvParameters)
{
    uint32_t notified_value = 0;
    uint8_t cRxedChar;
    uint16_t cInputIndex = 0;
    BaseType_t xResult = pdFAIL;
    static char cInputString[cmdMAX_INPUT_SIZE + 1];

    FreeRTOS_UART_open();
    nt_dbg_print("QCLI_Task enter\r\n");
    QCLI_Display_Command_List();
    for (;;) {
        xResult = xTaskNotifyWait(pdFALSE, 0xFFFFFFFF, &notified_value, portMAX_DELAY);
        if (xResult != pdPASS) {
            nt_dbg_print("uart rx failed\r\n");
            continue;
        }
        cRxedChar = notified_value;
        if (cRxedChar == ENTER_KEY) {
            PAL_Console_Write(sizeof(PAL_OUTPUT_END_OF_LINE_STRING) - 1, PAL_OUTPUT_END_OF_LINE_STRING);
            cInputString[cInputIndex] = cRxedChar;
            cInputIndex++;
            // todo: may not handle all at one time
            QCLI_Process_Input_Data(cInputIndex, cInputString);
            cInputIndex = 0;
            memset(cInputString, 0x00, cmdMAX_INPUT_SIZE + 1);
            uart_flag = 2;
        } else {
            if (cRxedChar == '\r') {
                /* Ignore the character. */
                nt_dbg_print("should never happen\r\n");
            } else if ((cRxedChar == BACKSPACE_KEY_PUTTY) || (cRxedChar == BACKSPACE_KEY_TERATERM)) {
                if (cInputIndex > 0) {
                    uint16_t seq_start_idx;
                    uint8_t erase_cols;
                    seq_start_idx = cInputIndex - 1;
                    while ((seq_start_idx > 0) && ((uint8_t)cInputString[seq_start_idx] >= 0x80)
                           && ((uint8_t)cInputString[seq_start_idx] <= 0xBF)) {
                        seq_start_idx--;
                    }
                    erase_cols = utf8_char_display_width(cInputString, seq_start_idx);
                    memset(&cInputString[seq_start_idx], '\0', cInputIndex - seq_start_idx);
                    cInputIndex = seq_start_idx;
                    while (erase_cols--) {
                        PAL_Console_Write(3, "\b \b");
                    }
                }
            } else {
                if (((cRxedChar >= ' ') && (cRxedChar <= '~')) || (cRxedChar >= 0x80)) {
                    if (cInputIndex < cmdMAX_INPUT_SIZE) {
                        PAL_Console_Write(sizeof(cRxedChar), (char *)&cRxedChar);
                        cInputString[cInputIndex] = cRxedChar;
                        cInputIndex++;
                    }
                }
            }
        }

        notified_value = 0;
    }
}

#ifdef CONFIG_RTT_VIEW_CLI
static void QCLI_RTT_CLI_Task(void __attribute__((__unused__)) * pvParameters)
{
    char rttInputString[cmdMAX_INPUT_SIZE + 1] = {0};
    uint16_t rttInputStringIndex = 0;

    for (;;) {
        int view_input;
        view_input = SEGGER_RTT_GetKey();
        if (view_input > 0) {
            if (view_input == ENTER_KEY) {
                if (rttInputStringIndex > 0) {
                    SEGGER_RTT_printf(0, rttInputString);
                    SEGGER_RTT_printf(0, "\n");
                    // PAL_Console_Write(sizeof(rttInputString), rttInputString);
                    rttInputString[rttInputStringIndex] = view_input;
                    rttInputStringIndex++;
                    QCLI_Process_Input_Data(rttInputStringIndex, rttInputString);
                    rttInputStringIndex = 0;
                    memset(rttInputString, 0x00, cmdMAX_INPUT_SIZE + 1);
                }

            } else {
                if (((view_input >= ' ') && (view_input <= '~')) || (view_input >= 0x80)) {
                    if (rttInputStringIndex < cmdMAX_INPUT_SIZE) {
                        rttInputString[rttInputStringIndex] = view_input;
                        rttInputStringIndex++;
                    }
                }
            }

        } else {
            vTaskDelay(10);
        }
    }
}
#endif
static qbool_t PAL_Initialize(void)
{
    memset(&QCLI_Task_handle, 0, sizeof(QCLI_Task_handle));
    memset(&PAL_Context, 0, sizeof(PAL_Context));
    qurt_mutex_create(&PAL_Context.Mutex);
    return true;
}

static qapi_Status_t command_ver(uint32_t __attribute__((__unused__)) Parameter_Count,
                                 QAPI_Console_Parameter_t __attribute__((__unused__)) * Parameter_List)
{
    printf("QAPI Ver: %d.%d.%d\n", QAPI_VERSION_MAJOR, QAPI_VERSION_MINOR, QAPI_VERSION_NIT);
    printf("crm num: %d.\n", CRM_BUILD_NUM);
    printf("WiFi version: %d.%d.%d\t%s\n", WIFI_FW_VER_MAJOR, WIFI_FW_VER_MINOR, WIFI_FW_VER_COUNT,
           WIFI_FW_VARIANT_NAME);
#ifdef CONFIG_QCCSDK_BOARD_NAME
    printf("build board name: %s\n", CONFIG_QCCSDK_BOARD_NAME);
#endif
    {
        unsigned int otp_version = *(unsigned int *)0x1a002c;
        unsigned int PBL_version = *(unsigned int *)0x200168;
        unsigned int kdf_lock = *(unsigned int *)0x1a0090;
        unsigned int CUID_0 = *(unsigned int *)0x1a0004;
        unsigned short CUID_1 = *(unsigned short *)0x1a0008;
        printf("OTP: OTP-version %d.%d, PBL-version %d.%d.%d, KDF-Lock 0x%x, CUID 0x%x %x\n",
               ((otp_version >> 24) & 0xff), (((otp_version >> 16) & 0xff)), ((PBL_version >> 24) & 0xff),
               (((PBL_version >> 16) & 0xff)), (((PBL_version >> 0) & 0xffff)), ((kdf_lock >> 8) & 0xff), CUID_1,
               CUID_0);
    }
    printf("build date and time: %s - %s\n", __DATE__, __TIME__);
    unsigned char *mac_addr = (unsigned char *)0x1a01c0;
    printf("MAC address: 0x%02x:0x%02x:0x%02x:0x%02x:0x%02x:0x%02x\r\n", mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
    unsigned char *module_part_number = (unsigned char*)0x1a0264;
    printf("Module Part Number: ");
    uint8_t i = 0;
    while (module_part_number[i] && module_part_number[i] != 0x03) {
        printf("%c", module_part_number[i]);
        i++;
    }
    printf("\n");
    unsigned int manufacturing_year_week = *(unsigned int*)0x1a0260;
    printf("Manufacturing year: %d, week: %d\r\n", (manufacturing_year_week >> 8) & 0xff, manufacturing_year_week & 0xff);
    printf("BOM configuration: %d\r\n", (manufacturing_year_week >> 16) & 0xffff);
    return QAPI_OK;
}

const QAPI_Console_Command_t root_shell_cmds[] = {
    /* cmd_function    cmd_string  usage_string                        description */
    {command_ver, "ver", "", "Display Build Info"}};

const QAPI_Console_Command_Group_t root_shell_cmd_group = {"", sizeof(root_shell_cmds) / sizeof(QAPI_Console_Command_t),
                                                           root_shell_cmds};

/**
   Main program entry point.
*/
void qccsdk_console_init(void)
{
    PAL_Initialize();
    nt_qurt_thread_create(QCLI_Task, "qconsole", configUART_COMMAND_CONSOLE_STACK_SIZE, NULL,
                          configUART_COMMAND_CONSOLE_TASK_PRIORITY, &QCLI_Task_handle);
    PAL_Context.QCLI_Handle = QCLI_Initialize(&root_shell_cmd_group);

#ifdef CONFIG_RTT_VIEW_CLI
    nt_qurt_thread_create(QCLI_RTT_CLI_Task, "rtt_cli", configUART_COMMAND_CONSOLE_STACK_SIZE, NULL,
                          configUART_COMMAND_CONSOLE_TASK_PRIORITY, &QCLI_RTT_CLI_Task_handle);
#endif
}
