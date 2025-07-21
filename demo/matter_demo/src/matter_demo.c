/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <stdlib.h>
#include "string.h"
#include "qapi_types.h"
#include "qapi_status.h"
#include "qcli.h"
#include "qcli_api.h"
#include "qcli_pal.h"
#include "qcli_util.h"

/**********************************************************************************************************/
/* Preprocessor Definitions and Constants											                      */
/**********************************************************************************************************/
#define MATTER_PRINTF_HANDLE qcli_matter_group

/**********************************************************************************************************/
/* Type Declarations											                                          */
/**********************************************************************************************************/

/**********************************************************************************************************/
/* Globals											                                                      */
/**********************************************************************************************************/
QAPI_Console_Group_Handle_t qcli_matter_group; /* Handle for our QCLI Command Group. */

/**********************************************************************************************************/
/* Function Declarations											                                      */
/**********************************************************************************************************/
static qapi_Status_t Command_Matter_Enable(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List);
static qapi_Status_t Command_Matter_Onboarding(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List);
static qapi_Status_t Command_Matter_Reset(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List);
#if CONFIG_MATTER_SWITCH_DEMO
static qapi_Status_t Command_Switch(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List);
#endif

/* The following is the complete command list for the Firmware Upgrade demo. */
const QAPI_Console_Command_t Matter_Command_List[] = {
    /* cmd_function                     cmd_string      usage_string              description */
    {Command_Matter_Enable, "enable", "\n\nenable", "Enable Matter Stack"},
    {Command_Matter_Onboarding, "onboarding", "\n\n<ssid> <password>",
     "Set ssid and pasword for on-network commission"},
    {Command_Matter_Reset, "reset", "\n\nreset", "Do factory reset"},
#if CONFIG_MATTER_SWITCH_DEMO
    {Command_Switch, "switch", "\n\nswitch [0 = unicast] [0 = off, 1 = on, 2 = toggle]", "Switch Command"},
#endif
};

const QAPI_Console_Command_Group_t Matter_Command_Group = {
    "Matter", /* Firmware Upgrade */
    sizeof(Matter_Command_List) / sizeof(QAPI_Console_Command_t),
    Matter_Command_List,
};

/**********************************************************************************************************/
/* Function Definitions    											                                      */
/**********************************************************************************************************/
/* This function is used to register the Firmware Upgrade Command Group with QCLI   */
void Initialize_Matter_Demo(void)
{
    /* Attempt to reqister the Command Groups with the qcli framework.*/
    MATTER_PRINTF_HANDLE = QAPI_Console_Register_Command_Group(NULL, &Matter_Command_Group);
    if (MATTER_PRINTF_HANDLE) {
        QCLI_Printf(MATTER_PRINTF_HANDLE, "Matter Registered \n");
    }
}

/*=================================================================================================*/

/**
   @brief This function processes the "enable" command from the CLI.
*/

void Matter_Enable();
void Matter_Onboarding(char *ssid, char *password);

void Matter_FactoryReset();
static qapi_Status_t Command_Matter_Enable(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    (void)Parameter_Count;
    (void)Parameter_List;

    Matter_Enable();

    QCLI_Printf(MATTER_PRINTF_HANDLE, "Matter Enable \n");

    return QAPI_OK;
}

static qapi_Status_t Command_Matter_Onboarding(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    if (Parameter_Count < 1 || !Parameter_List) {
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }

    char *ssid = Parameter_List[0].String_Value;

    if (Parameter_Count >= 2) {
        char *password = Parameter_List[1].String_Value;
        Matter_Onboarding(ssid, password);
    } else {
        Matter_Onboarding(ssid, "\0");
    }
    return QAPI_OK;
}

static qapi_Status_t Command_Matter_Reset(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    (void)Parameter_Count;
    (void)Parameter_List;

    Matter_FactoryReset();

    QCLI_Printf(MATTER_PRINTF_HANDLE, "Matter Factory Reset \n");

    return QAPI_OK;
}

#if CONFIG_MATTER_SWITCH_DEMO
void OffSwitchCommandHandler();
void OnSwitchCommandHandler();
void ToggleSwitchCommandHandler();

static qapi_Status_t Command_Switch(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)

{
    if ((Parameter_Count != 1) || (!Parameter_List[0].Integer_Is_Valid))
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;

    // Unicast Command
    if (Parameter_List[0].Integer_Value == 0) {
        OffSwitchCommandHandler();
        return QAPI_OK;
    }

    if (Parameter_List[0].Integer_Value == 1) {
        OnSwitchCommandHandler();
        return QAPI_OK;
    }
    if (Parameter_List[0].Integer_Value == 2) {
        ToggleSwitchCommandHandler();
        return QAPI_OK;
    }

    return QAPI_ERROR;
}
#endif
