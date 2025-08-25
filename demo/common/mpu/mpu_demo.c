/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
// $QTI_LICENSE_QDN_C$

#include <stdlib.h>
#include "string.h"
#include "qapi_types.h"
#include "qapi_status.h"
#include "qcli.h"
#include "qcli_api.h"
#include "qcli_pal.h"
#include "qcli_util.h"

#include "safeAPI.h"

/**********************************************************************************************************/
/* Preprocessor Definitions and Constants											                      */
/**********************************************************************************************************/
#define MPU_PRINTF_HANDLE qcli_mpu_group

/**********************************************************************************************************/
/* Type Declarations											                                          */
/**********************************************************************************************************/
typedef void (*mpu_func_ptr)(void);

/**********************************************************************************************************/
/* Globals											                                                      */
/**********************************************************************************************************/
QAPI_Console_Group_Handle_t qcli_mpu_group; /* Handle for our QCLI Command Group. */

/**********************************************************************************************************/
/* Function Declarations											                                      */
/**********************************************************************************************************/
static qapi_Status_t Command_execute_stack(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List);
static qapi_Status_t Command_execute_heap(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List);
static qapi_Status_t Command_execute_data(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List);
static qapi_Status_t Command_rewrite_code(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List);

/* The following is the complete command list for the Firmware Upgrade demo. */
const QAPI_Console_Command_t MPU_Command_List[] = {
    /* cmd_function                     cmd_string      usage_string              description */
    {Command_execute_stack, "execute_stack", "\n\nexecute_stack", "MPU test case: Execute Stack"},
    {Command_execute_heap, "execute_heap", "\n\nexecute_heap", "MPU test case: Execute Heap"},
    {Command_execute_data, "execute_data", "\n\nexecute_data", "MPU test case: Execute Data"},
    {Command_rewrite_code, "rewrite_code", "\n\nrewrite_code", "MPU test case: Rewrite Code"},
};

const QAPI_Console_Command_Group_t MPU_Command_Group = {
    "MPU", /* Memory Protection Unit */
    sizeof(MPU_Command_List) / sizeof(QAPI_Console_Command_t),
    MPU_Command_List,
};

/**********************************************************************************************************/
/* Function Definitions    											                                      */
/**********************************************************************************************************/
/* This function is used to register the MPU Command Group with QCLI   */
void Initialize_MPU_Demo(void)
{
    /* Attempt to reqister the Command Groups with the qcli framework.*/
    MPU_PRINTF_HANDLE = QAPI_Console_Register_Command_Group(NULL, &MPU_Command_Group);
    if (MPU_PRINTF_HANDLE) {
        QCLI_Printf(MPU_PRINTF_HANDLE, "MPU Registered \n");
    }
}

/* The actual function to run in stack, heap and data segment */
void mpu_null_func(void)
{
    __asm volatile("nop \n");
    __asm volatile("nop \n");
    __asm volatile("nop \n");
    return;
}

/* Dump of mpu_null_func */
uint32_t g_func_dump[2] = {0xbf00bf00, 0x4770bf00};

static qapi_Status_t Command_execute_stack(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    uint32_t func_dump[2];
    mpu_func_ptr myFunc;
    (void)Parameter_Count;
    (void)Parameter_List;

    /* execute the function in stack */
    memscpy((void *)func_dump, sizeof(func_dump), (const void *)g_func_dump, sizeof(g_func_dump));
    myFunc = (mpu_func_ptr)((uint32_t)func_dump + 1);
    myFunc();

    QCLI_Printf(MPU_PRINTF_HANDLE, "Function execute in stack successfully\n");
    return QAPI_OK;
}

static qapi_Status_t Command_execute_heap(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    uint32_t *func_dump;
    mpu_func_ptr myFunc;
    (void)Parameter_Count;
    (void)Parameter_List;

    /* execute the function in heap */
    func_dump = (uint32_t *)malloc(sizeof(g_func_dump));
    if (!func_dump)
        return QAPI_ERROR;

    memscpy((void *)func_dump, sizeof(g_func_dump), (const void *)g_func_dump, sizeof(g_func_dump));
    myFunc = (mpu_func_ptr)((uint32_t)func_dump + 1);
    myFunc();

    free(func_dump);
    QCLI_Printf(MPU_PRINTF_HANDLE, "Function execute in heap successfully\n");
    return QAPI_OK;
}

static qapi_Status_t Command_execute_data(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    mpu_func_ptr myFunc;
    (void)Parameter_Count;
    (void)Parameter_List;

    /* execute the function in data segment */
    myFunc = (mpu_func_ptr)g_func_dump;
    myFunc();

    QCLI_Printf(MPU_PRINTF_HANDLE, "Function execute in data segment successfully\n");
    return QAPI_OK;
}

static qapi_Status_t Command_rewrite_code(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    (void)Parameter_Count;
    (void)Parameter_List;

    /* 0x20000 locates in RAM_RO region, and is configured to be READONLY in MPU */
    uint32_t *code_ptr = (uint32_t *)0x20000;

    *code_ptr = *code_ptr + 1;

    QCLI_Printf(MPU_PRINTF_HANDLE, "Rewrite code successfully\n");
    return QAPI_OK;
}
