/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*-------------------------------------------------------------------------
 * Include Files
 *-----------------------------------------------------------------------*/

#include <stdio.h>
#include <stdarg.h>
#include "qapi_types.h"
#include "qapi_version.h"
#include "qapi_status.h"
#include "qapi_console.h"

#include "qcli.h"
#include "qcli_pal.h"
#include "qcli_api.h"
#include "qcli_util.h"

#include "qurt_internal.h"

#include "safeAPI.h"

/*-------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 *-----------------------------------------------------------------------*/

/**
   Group handle which represents the QCLI module itself.  This is used to print
   messages without a group prefix.
*/
#define MAIN_PRINTF_HANDLE ((QAPI_Console_Group_Handle_t) & (QCLI_Context.Root_Group))

/*-------------------------------------------------------------------------
 * Type Declarations
 *-----------------------------------------------------------------------*/

/**
   Information for a command group list entry.
*/
typedef struct Group_List_Entry_s {
    struct Group_List_Entry_s *Next;                   /**< Next entry in the list. */
    const QAPI_Console_Command_Group_t *Command_Group; /**< Command group information. */
    struct Group_List_Entry_s *Parent_Group;           /**< Parent group for this subgroup. */
    struct Group_List_Entry_s *Subgroup_List;          /**< List of subgroups registered for this group. */
} Group_List_Entry_t;

/**
   Information for the result of a Find_Command() operation.
*/
typedef struct Find_Result_s {
    qbool_t Is_Group; /**< Flag indicating if the result is a command or a group. */
    union {
        const QAPI_Console_Command_t *Command; /**< Entry that was found if it is a command. */
        Group_List_Entry_t *Group_List_Entry;  /**< Entry that was found if it is a group. */
    } Data;
} Find_Result_t;

#define THREAD_READY_EVENT_MASK 0x00000001

typedef struct Thread_Info_s {
    qurt_signal_t
        Thread_Ready_Event; /**< Event which indicates the thread no longer needs this information structure. */
    uint32_t Command_Index; /**< The index of the command that will be executed. */
    const QAPI_Console_Command_t *Command;    /**< The command that will be executed. */
    uint32_t Parameter_Count;                 /**< The number of parameters specified for the command. */
    QAPI_Console_Parameter_t *Parameter_List; /**< The list of paramters for the command. */
} Thread_Info_t;

/**
   Context information for the QCLI module.
*/
typedef struct QCLI_Context_s {
    Group_List_Entry_t Root_Group;                 /**< Root of the group menu structure. */
    Group_List_Entry_t *Current_Group;             /**< Current group. */
    Group_List_Entry_t *Executing_Group;           /**< Group of currently executing command. */
    const QAPI_Console_Command_Group_t *PAL_Group; /**< Group list for platform commands. */

    uint32_t Input_Length; /**< Length of the current console input string. */
    char Input_String[QCLI_MAXIMUM_QCLI_COMMAND_STRING_LENGTH +
                      1]; /**< Buffer containing the current console input string. */
    QAPI_Console_Parameter_t
        Parameter_List[QCLI_MAXIMUM_NUMBER_OF_PARAMETERS + 1]; /**< List of parameters for input command. */

    char Printf_Buffer[QCLI_MAXIMUM_PRINTF_LENGTH];   /**< Buffer used for formatted output strings. */
    QAPI_Console_Group_Handle_t Current_Printf_Group; /**< Group handle that was last passed to QCLI_Printf(). */
    qbool_t Printf_New_Line; /**< Indicates that a newline should be displayed if a printf changes groups. */
    Thread_Info_t Thread_Info;
    int8_t Thread_Count;
} QCLI_Context_t;

QCLI_Context_t QCLI_Context;

/*-------------------------------------------------------------------------
 * Function Declarations
 *-----------------------------------------------------------------------*/

static qapi_Status_t Command_Help(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List);
static qapi_Status_t Command_Up(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List);
static qapi_Status_t Command_Root(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List);

static void Display_Group_Name(const Group_List_Entry_t *Group_List_Entry);
static uint32_t Display_Help(Group_List_Entry_t *Command_Group, uint32_t Parameter_Count,
                             QAPI_Console_Parameter_t *Parameter_List);
static void Display_Usage(uint32_t Command_Index, const QAPI_Console_Command_t *Command);
static void Display_Command_List(const Group_List_Entry_t *Group_List_Entry);

static qbool_t Find_Command(Group_List_Entry_t *Group_List_Entry, QAPI_Console_Parameter_t *Command_Parameter,
                            Find_Result_t *Find_Result);
static qbool_t Find_Command_By_String(Group_List_Entry_t *Group_List_Entry, QAPI_Console_Parameter_t *Command_Parameter,
                                      Find_Result_t *Find_Result);
static qbool_t Find_Command_By_Index(Group_List_Entry_t *Group_List_Entry, int32_t Index, Find_Result_t *Find_Result);
static void Process_Command(void);
static qbool_t Unregister_Command_Group(Group_List_Entry_t *Group_List_Entry);
void QCLI_Display_Prompt(void);

/** List of global commands that are supported when not in a group. */
const QAPI_Console_Command_t Help_Command = {Command_Help, "Help", "[Command (optional)]",
                                             "Display Command list or usage for a command"};

/** List of global commands that are supported when in a group. */
const QAPI_Console_Command_t Common_Command_List[] = {
    {Command_Up, "Up", "", "Exit command group (move to parent group)"},
    {Command_Root, "Root", "", "Move to top-level group list"}};

#define COMMON_COMMAND_LIST_SIZE (sizeof(Common_Command_List) / sizeof(QAPI_Console_Command_t))

/*-------------------------------------------------------------------------
 * Function Definitions
 *-----------------------------------------------------------------------*/

/**
   @brief Processes the "Help" command from the CLI.

   The parameters provided indicate the command or group to display the
   help message for:
    - If no parameters are specified, the list of commands for the current
      command group will be displayed.
    - If the parameters specify a subgroup, the command list for that group will
      be displayed.
    - If the parameters specify a command, the usage message for that command
      will be displayed.

   @param[in] Parameter_Count  Number of parameters that were entered into the
                               command line.
   @param[in] Parameter_List   List of parameters entered into the command line.
*/
static qapi_Status_t Command_Help(uint32_t Parameter_Count, QAPI_Console_Parameter_t *Parameter_List)
{
    qapi_Status_t Ret_Val;
    uint32_t Result;
    uint32_t Index;

    if (PAL_Take_Lock()) {
        Result = Display_Help(QCLI_Context.Executing_Group, Parameter_Count, Parameter_List);

        /* if there was an error parsing the command list, print out an error
           message here (this is in addition to the usage message that will be
           printed out). */
        if (Result > 0) {
            QCLI_Printf(MAIN_PRINTF_HANDLE, "Command \"%s", Parameter_List[0].String_Value);

            for (Index = 1; Index < Result; Index++) {
                QCLI_Printf(MAIN_PRINTF_HANDLE, " %s", Parameter_List[Index].String_Value);
            }

            QCLI_Printf(MAIN_PRINTF_HANDLE, "\" not found.\n");

            Ret_Val = QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
        } else {
            Ret_Val = QAPI_OK;
        }

        PAL_Release_Lock();
    } else {
        Ret_Val = QAPI_ERROR_CONSOLE_COMMAND_STATUS_ERROR;
    }

    return (Ret_Val);
}

/**
   @brief Processes the "Up" command from the CLI.

   This command will change the current group to its parent. No parameters are
   expected for this command.

   @param[in] Parameter_Count  Number of parameters that were entered into the
                               command line.
   @param[in] Parameter_List   List of parameters entered into the command line.
*/
static qapi_Status_t Command_Up(uint32_t __attribute__((__unused__)) Parameter_Count,
                                QAPI_Console_Parameter_t __attribute__((__unused__)) * Parameter_List)
{
    if (PAL_Take_Lock()) {
        if (QCLI_Context.Current_Group->Parent_Group != NULL) {
            QCLI_Context.Current_Group = QCLI_Context.Current_Group->Parent_Group;

            /* Display the command list again. */
            Display_Command_List(QCLI_Context.Current_Group);
        }

        PAL_Release_Lock();
    }

    return (QAPI_OK);
}

/**
   @brief Processes the "Root" command from the CLI.

   This command will change the current group to the root group. No parameters
   are expected for this command.

   @param[in] Parameter_Count  Number of parameters that were entered into the
                               command line.
   @param[in] Parameter_List   List of parameters entered into the command line.
*/
static qapi_Status_t Command_Root(uint32_t __attribute__((__unused__)) Parameter_Count,
                                  QAPI_Console_Parameter_t __attribute__((__unused__)) * Parameter_List)
{
    if (PAL_Take_Lock()) {
        if (QCLI_Context.Current_Group->Parent_Group != NULL) {
            QCLI_Context.Current_Group = &(QCLI_Context.Root_Group);

            /* Display the command list again. */
            Display_Command_List(QCLI_Context.Current_Group);
        }

        PAL_Release_Lock();
    }

    return (QAPI_OK);
}

/**
   @brief Displays the group name after recursively displaying the name of the
          group's parents.

   @param[in] Group_List_Entry  Group list whose name should be displayed.  If
                                this isn't the root group, the parent group's
                                name will be displayed first.
*/
static void Display_Group_Name(const Group_List_Entry_t *Group_List_Entry)
{
    /* If the group's parent isn't the root, display the parent first. */
    if (Group_List_Entry->Parent_Group->Parent_Group != NULL) {
        Display_Group_Name(Group_List_Entry->Parent_Group);

        QCLI_Printf(MAIN_PRINTF_HANDLE, "\\");
    }

    /* Display this group's name. */
    QCLI_Printf(MAIN_PRINTF_HANDLE, "%s", Group_List_Entry->Command_Group->Group_String);
}

/**
   @brief Processes the help command, recursively descending groups if
          necessary.

   As the groups are recursively descended, the first parameter in the parameter
   list will be stripped off until the list is empty.

   @param[in] Group_List_Entry  Current command group for the help command.
   @param[in] Parameter_Count   Number of parameters provided to the help
                                command.
   @param[in] Parameter_List    List of parameters entered provided to the help
                                command.

   @return
    - 0 if the help was displayed correctly.
    - A positive value indicating the depth of the error if a parameter was
      invalid.
*/
static uint32_t Display_Help(Group_List_Entry_t *Group_List_Entry, uint32_t Parameter_Count,
                             QAPI_Console_Parameter_t *Parameter_List)
{
    uint32_t Ret_Val;
    Find_Result_t Find_Result;

    /* If a parameter was specified, see if we can tie it to a command. */
    if (Parameter_Count >= 1) {
        if (Find_Command(Group_List_Entry, &(Parameter_List[0]), &Find_Result)) {
            /* Command was found, assign it now. */
            if (Find_Result.Is_Group) {
                /* If this was a group, recurse into it. */
                Ret_Val = Display_Help(Find_Result.Data.Group_List_Entry, Parameter_Count - 1, &(Parameter_List[1]));

                /* If the recursive call returned an error, add one to it. */
                if (Ret_Val > 0) {
                    Ret_Val++;
                }
            } else {
                /* If this was the last parameter specified, display the usage for
                   the command. If it wasn't, return an error. */
                if (Parameter_Count == 1) {
                    Display_Usage(Parameter_List[0].Integer_Value, Find_Result.Data.Command);

                    Ret_Val = 0;
                } else {
                    /* The error code indicates that the next parameter is invalid. */
                    Ret_Val = 2;
                }
            }
        } else {
            /* Command not found so return an error. */
            Ret_Val = 1;
        }
    } else {
        /* Display the command list for the current group. */
        Display_Command_List(Group_List_Entry);

        Ret_Val = 0;
    }

    return (Ret_Val);
}

/**
   @brief Displays the usage string for a command.

   @param[in] Command_Index  Index of the command in its associated command
                             group.
   @param[in] Command        Information structure for the command.
*/
static void Display_Usage(uint32_t Command_Index, const QAPI_Console_Command_t *Command)
{
    QCLI_Printf(MAIN_PRINTF_HANDLE, "\n");
    QCLI_Printf(MAIN_PRINTF_HANDLE, "%d: %s %s\n", Command_Index, Command->Command_String, Command->Usage_String);
    QCLI_Printf(MAIN_PRINTF_HANDLE, "    %s\n", Command->Description);
    QCLI_Printf(MAIN_PRINTF_HANDLE, "\n");
}

/**
   @brief Displays the list of commands and/or command groups for the specified
          group list.

   @param[in] Group_List_Entry  Command group list entry to be displayed.
*/
static void Display_Command_List(const Group_List_Entry_t *Group_List_Entry)
{
    uint32_t Index;
    uint32_t Command_Index;
    Group_List_Entry_t *Subgroup_List_Entry;

    QCLI_Printf(MAIN_PRINTF_HANDLE, "\n");

    if (Group_List_Entry) {
        QCLI_Printf(MAIN_PRINTF_HANDLE, "Command List");

        if (Group_List_Entry->Parent_Group != NULL) {
            QCLI_Printf(MAIN_PRINTF_HANDLE, " (");
            Display_Group_Name(Group_List_Entry);
            QCLI_Printf(MAIN_PRINTF_HANDLE, ")");
        }

        QCLI_Printf(MAIN_PRINTF_HANDLE, ":\n");

        QCLI_Printf(MAIN_PRINTF_HANDLE, "  Commands:\n");
        Command_Index = QCLI_COMMAND_START_INDEX;

        /* Display the help command. */
        QCLI_Printf(MAIN_PRINTF_HANDLE, "    %2d. %s\n", Command_Index, Help_Command.Command_String);
        Command_Index++;

        /* Display the common commands. */
        if (Group_List_Entry->Parent_Group != NULL) {
            for (Index = 0; Index < COMMON_COMMAND_LIST_SIZE; Index++) {
                QCLI_Printf(MAIN_PRINTF_HANDLE, "    %2d. %s\n", Command_Index,
                            Common_Command_List[Index].Command_String);
                Command_Index++;
            }
        }

        /* Display the command list. */
        if ((Group_List_Entry->Command_Group != NULL) && (Group_List_Entry->Command_Group->Command_List != NULL)) {
            QCLI_Printf(MAIN_PRINTF_HANDLE, "\n");

            for (Index = 0; Index < Group_List_Entry->Command_Group->Command_Count; Index++) {
                QCLI_Printf(MAIN_PRINTF_HANDLE, "    %2d. %s\n", Command_Index,
                            Group_List_Entry->Command_Group->Command_List[Index].Command_String);
                Command_Index++;
            }
        }

        /* Display the group list. */
        if (Group_List_Entry->Subgroup_List != NULL) {
            QCLI_Printf(MAIN_PRINTF_HANDLE, "\n");
            QCLI_Printf(MAIN_PRINTF_HANDLE, "  Subgroups:\n");

            Subgroup_List_Entry = Group_List_Entry->Subgroup_List;
            while (Subgroup_List_Entry != NULL) {
                QCLI_Printf(MAIN_PRINTF_HANDLE, "    %2d. %s\n", Command_Index,
                            Subgroup_List_Entry->Command_Group->Group_String);

                Subgroup_List_Entry = Subgroup_List_Entry->Next;
                Command_Index++;
            }
        }

        QCLI_Printf(MAIN_PRINTF_HANDLE, "\n");
    }
}

/**
   @brief Searches the command and/or group lists for a match to the provided
          parameter.

   @param[in]     Group_List_Entry   Group to search.
   @param[in,out] Command_Parameter  Parameter to search for.  If command was
                                     found, the integer value will be updated
                                     with the command's index.
   @param[out]    Find_Result        Pointer to where the found entry will be
                                     stored if successful.

   @return
    - true if a matching command or group was found in the list.
    - false if the command or group was not found.
*/
static qbool_t Find_Command(Group_List_Entry_t *Group_List_Entry, QAPI_Console_Parameter_t *Command_Parameter,
                            Find_Result_t *Find_Result)
{
    qbool_t Ret_Val;

    if (Command_Parameter->Integer_Is_Valid) {
        Ret_Val = Find_Command_By_Index(Group_List_Entry, Command_Parameter->Integer_Value, Find_Result);
    } else {
        Ret_Val = Find_Command_By_String(Group_List_Entry, Command_Parameter, Find_Result);
    }

    return (Ret_Val);
}

/**
   @brief Searches the command and/or group lists for a match to the provided
          parameter string.

   @param[in]     Group_List_Entry   Group to search.
   @param[in,out] Command_Parameter  Parameter to search for.  If command was
                                     found, the integer value will be updated
                                     with the command's index.
   @param[out]    Find_Result        Pointer to where the found entry will be
                                     stored if successful.

   @return
    - true if a matching command or group was found in the list.
    - false if the command or group was not found.
*/
static qbool_t Find_Command_By_String(Group_List_Entry_t *Group_List_Entry, QAPI_Console_Parameter_t *Command_Parameter,
                                      Find_Result_t *Find_Result)
{
    qbool_t Ret_Val;
    uint32_t Index;
    uint32_t Command_Index;
    uint32_t String_Length;

    /* Get the size of the string.  Include the NULL byte so the comparison
       doesn't match a substrings. */
    String_Length = strlen((const char *)(Command_Parameter->String_Value)) + 1;

    Ret_Val = false;
    Command_Index = QCLI_COMMAND_START_INDEX;

    /* Check for the help command. */
    if (QCLI_Memcmpi(Command_Parameter->String_Value, Help_Command.Command_String, String_Length) == 0) {
        Ret_Val = true;
        Find_Result->Is_Group = false;
        Find_Result->Data.Command = &Help_Command;
    } else {
        /* Search the common command list. */
        if (Group_List_Entry->Parent_Group != NULL) {
            for (Index = 0; (Index < COMMON_COMMAND_LIST_SIZE) && (!Ret_Val); Index++) {
                if (QCLI_Memcmpi(Command_Parameter->String_Value, Common_Command_List[Index].Command_String,
                                 String_Length) == 0) {
                    /* Command found. */
                    Ret_Val = true;
                    Find_Result->Is_Group = false;
                    Find_Result->Data.Command = &(Common_Command_List[Index]);
                } else {
                    Command_Index++;
                }
            }
        }

        /* Only search the command group if it isn't NULL. */
        if ((!Ret_Val) && (Group_List_Entry->Command_Group != NULL)) {
            /* If the command wasn't found yet, search the group's command list. */
            for (Index = 0; (Index < Group_List_Entry->Command_Group->Command_Count) && (!Ret_Val); Index++) {
                if (QCLI_Memcmpi(Command_Parameter->String_Value,
                                 Group_List_Entry->Command_Group->Command_List[Index].Command_String,
                                 String_Length) == 0) {
                    /* Command found. */
                    Ret_Val = true;
                    Find_Result->Is_Group = false;
                    Find_Result->Data.Command = &(Group_List_Entry->Command_Group->Command_List[Index]);
                } else {
                    Command_Index++;
                }
            }
        }

        if (!Ret_Val) {
            /* If the command wasn't found yet, search the group's subgroup
               list. */
            Group_List_Entry = Group_List_Entry->Subgroup_List;
            while ((Group_List_Entry != NULL) && (!Ret_Val)) {
                if (QCLI_Memcmpi(Command_Parameter->String_Value, Group_List_Entry->Command_Group->Group_String,
                                 String_Length) == 0) {
                    /* Command found. */
                    Ret_Val = true;
                    Find_Result->Is_Group = true;
                    Find_Result->Data.Group_List_Entry = Group_List_Entry;
                } else {
                    Command_Index++;
                    Group_List_Entry = Group_List_Entry->Next;
                }
            }
        }
    }

    if (Ret_Val) {
        /* Update the integer value for the command parameter with its index. */
        Command_Parameter->Integer_Value = Command_Index;
        Command_Parameter->Integer_Is_Valid = true;
    }

    return (Ret_Val);
}

/**
   @brief Searches the command and/or group lists for a match to the provided
          index.

   @param[in]  Group_List_Entry  Group to search.
   @param[in]  Command_Index     Index of the command.
   @param[out] Find_Result       Pointer to where the found entry will be stored
                                 if successful.

   @return
    - true if a matching command or group was found in the list.
    - false if the command or group was not found.
*/
static qbool_t Find_Command_By_Index(Group_List_Entry_t *Group_List_Entry, int32_t Command_Index,
                                     Find_Result_t *Find_Result)
{
    qbool_t Ret_Val;

    Ret_Val = false;

    if (Group_List_Entry != NULL) {
        /* Command was specified as an integer. */
        if (Command_Index == QCLI_COMMAND_START_INDEX) {
            /* Help command. */
            Ret_Val = true;
            Find_Result->Is_Group = false;
            Find_Result->Data.Command = &Help_Command;
        } else if (((uint32_t)Command_Index) > QCLI_COMMAND_START_INDEX) {
            Command_Index -= (QCLI_COMMAND_START_INDEX + 1);

            /* If the integer is a valid value for the command group, use it. */
            if (Group_List_Entry->Parent_Group != NULL) {
                if (((uint32_t)Command_Index) < COMMON_COMMAND_LIST_SIZE) {
                    /* Command is in the common command list. */
                    Ret_Val = true;
                    Find_Result->Is_Group = false;
                    Find_Result->Data.Command = &(Common_Command_List[Command_Index]);
                } else {
                    Command_Index -= COMMON_COMMAND_LIST_SIZE;
                }
            }

            if ((!Ret_Val) && (Group_List_Entry->Command_Group != NULL)) {
                if (((uint32_t)Command_Index) < Group_List_Entry->Command_Group->Command_Count) {
                    /* Command is in the group's command list. */
                    Ret_Val = true;
                    Find_Result->Is_Group = false;
                    Find_Result->Data.Command = &(Group_List_Entry->Command_Group->Command_List[Command_Index]);
                } else {
                    Command_Index -= Group_List_Entry->Command_Group->Command_Count;
                }
            }

            if (!Ret_Val) {
                /* Search the group list. */
                Group_List_Entry = Group_List_Entry->Subgroup_List;
                while ((Group_List_Entry != NULL) && (Command_Index != 0)) {
                    Group_List_Entry = Group_List_Entry->Next;
                    Command_Index--;
                }

                if (Group_List_Entry != NULL) {
                    /* Command is in the subgroup list. */
                    Ret_Val = true;
                    Find_Result->Is_Group = true;
                    Find_Result->Data.Group_List_Entry = Group_List_Entry;
                }
            }
        }
    }

    return (Ret_Val);
}

static void Command_Thread(void *Thread_Parameter)
{
    uint32_t Index;
    Thread_Info_t *Thread_Info;
    char Input_String[QCLI_MAXIMUM_QCLI_COMMAND_STRING_LENGTH + 1];
    QAPI_Console_Parameter_t Parameter_List[QCLI_MAXIMUM_NUMBER_OF_PARAMETERS];
    uint32_t Parameter_Count;
    uint32_t Command_Index;
    const QAPI_Console_Command_t *Command;

    qapi_Status_t Result;

    if (Thread_Parameter) {
        memset(&Parameter_List, 0, sizeof(Parameter_List));

        /* Copy the thread info to local storage. */
        Thread_Info = (Thread_Info_t *)Thread_Parameter;
        memscpy(&Input_String, sizeof(Input_String), QCLI_Context.Input_String, sizeof(Input_String));
        memscpy(&Parameter_List, Thread_Info->Parameter_Count * sizeof(QAPI_Console_Parameter_t),
                Thread_Info->Parameter_List, Thread_Info->Parameter_Count * sizeof(QAPI_Console_Parameter_t));
        Parameter_Count = Thread_Info->Parameter_Count;
        Command = Thread_Info->Command;
        Command_Index = Thread_Info->Command_Index;

        /* Adjust the pointers in the paramter list for the local input string. */
        for (Index = 0; Index < Thread_Info->Parameter_Count; Index++) {
            Parameter_List[Index].String_Value += (Input_String - QCLI_Context.Input_String);
        }

        /* Signal that the thread is ready.  The Thread parameter should be
           considered invalid after this point. */
        qurt_signal_set(&(Thread_Info->Thread_Ready_Event), THREAD_READY_EVENT_MASK);

        /* Execute the command. */
        Result = (*(Command->Command_Function))(Parameter_Count, Parameter_List);

        /* Take the mutex before modifying any global variables. */
        if (PAL_Take_Lock()) {
            if ((Result == QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE) || (Result == QCLI_STATUS_USAGE_E)) {
                /* Print the usage message. */
                Display_Usage(Command_Index, Command);
                QCLI_Display_Prompt();
            }

            /* Decrement the number of active threads. */
            QCLI_Context.Thread_Count--;

            PAL_Release_Lock();
        }
    }

    /* Terminate the thread. */
    qurt_thread_stop();
}

#define QCLI_COMMAND_BACKGOURND_STACK_SIZE                                             \
    (configUART_COMMAND_CONSOLE_STACK_SIZE + QCLI_MAXIMUM_QCLI_COMMAND_STRING_LENGTH + \
     QCLI_MAXIMUM_NUMBER_OF_PARAMETERS * sizeof(QAPI_Console_Parameter_t))

static void Execute_Command(uint32_t Command_Index, const QAPI_Console_Command_t *Command, uint32_t Parameter_Count,
                            QAPI_Console_Parameter_t *Parameter_List)
{
    qapi_Status_t Result;

    if ((Parameter_Count > 0) &&
        (strncmp(Parameter_List[Parameter_Count - 1].String_Value, QCLI_COMMAND_BACKGOURND_PARAMETER,
                 sizeof(QCLI_COMMAND_BACKGOURND_PARAMETER)) == 0)) {
        Thread_Info_t *p_thread_info = &QCLI_Context.Thread_Info;
        TaskHandle_t Thread_Handle;
        int Thread_Result;
        uint32 Signal_Waiting;

        // parameter ends with backgroud parameter, then run at another thread
        if (QCLI_Context.Thread_Count >= QCLI_COMMAND_BACKGOURND_MAX_COUNT) {
            QCLI_Printf(MAIN_PRINTF_HANDLE, "Max threads (%d) reached, ignore.\n", QCLI_COMMAND_BACKGOURND_MAX_COUNT);
            return;
        }
        QCLI_Context.Thread_Count++;
        Parameter_Count--;
        /* Make sure the running event semaphore is taken. */
        qurt_signal_clear(&p_thread_info->Thread_Ready_Event, THREAD_READY_EVENT_MASK);
        /* Pass the function to the thread pool. */
        p_thread_info->Command = Command;
        p_thread_info->Command_Index = Command_Index;
        p_thread_info->Parameter_Count = Parameter_Count;
        p_thread_info->Parameter_List = Parameter_List;
        /* Create a thread for the command. */
        Thread_Result =
            nt_qurt_thread_create(Command_Thread, Command->Command_String, QCLI_COMMAND_BACKGOURND_STACK_SIZE,
                                  p_thread_info, configUART_COMMAND_CONSOLE_TASK_PRIORITY, &Thread_Handle);
        if (Thread_Result == pdPASS) {
            /* Wait for the thread to take over. */
            if (qurt_signal_wait_timed(&p_thread_info->Thread_Ready_Event, THREAD_READY_EVENT_MASK,
                                       QURT_SIGNAL_ATTR_WAIT_ANY, &Signal_Waiting,
                                       QURT_TIME_WAIT_FOREVER) != QURT_EOK) {
                QCLI_Printf(MAIN_PRINTF_HANDLE, "Thread failed to start for command.\n");
            }
        } else {
            QCLI_Printf(MAIN_PRINTF_HANDLE, "Failed to create thread for command (%d).\n", Thread_Result);
            QCLI_Context.Thread_Count--;
        }
    } else {
        // If not background, then run directly

        /* Release the lock while calling the function. */
        PAL_Release_Lock();

        // Result = (*(Find_Result.Data.Command->Command_Function))(Parameter_Count - Index, (Parameter_Count > Index) ?
        // &(QCLI_Context.Parameter_List[Index]) : NULL);
        Result = (*(Command->Command_Function))(Parameter_Count, Parameter_List);
        if (!PAL_Take_Lock()) {
            QCLI_Printf(MAIN_PRINTF_HANDLE, "Failed to re-take the mutex!\n");
        }

        if ((Result == QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE) || (Result == QCLI_STATUS_USAGE_E)) {
            // Display_Usage(QCLI_Context.Parameter_List[Index - 1].Integer_Value, Find_Result.Data.Command);
            Display_Usage(Command_Index, Command);
        }
    }
}

/**
   @brief Processes a command received from the console.
*/
static void Process_Command(void)
{
    qbool_t Result;
    Find_Result_t Find_Result;
    uint32_t Parameter_Count;
    uint32_t Index;
    uint32_t Command_Index;
    qbool_t Inside_Quotes;
    uint32_t Input_Length;

    /* Store the input length locally so any re-displays of the prompt will not
       include the command. */
    Input_Length = QCLI_Context.Input_Length;
    QCLI_Context.Input_Length = 0;

    /* Parse the command until its end is reached or the parameter list is full. */
    Parameter_Count = 0;
    Index = 0;
    Inside_Quotes = false;
    Result = true;
    while ((Result) && (QCLI_Context.Input_String[Index] != '\0') &&
           (Parameter_Count <= QCLI_MAXIMUM_NUMBER_OF_PARAMETERS)) {
        /* Consume any leading white space. */
        while (QCLI_Context.Input_String[Index] == ' ') {
            Index++;
        }

        /* If the first character is '"', consume it. */
        if (QCLI_Context.Input_String[Index] == '"') {
            Inside_Quotes = true;
            Index++;
        }

        /* Assuming the end of the command hasn't been reached, assign the current
           string location as the current parameter's string. */
        if (QCLI_Context.Input_String[Index] != '\0') {
            QCLI_Context.Parameter_List[Parameter_Count].String_Value = &QCLI_Context.Input_String[Index];

            /* Find the end of the parameter.  The end of parameter is determined as
               either a null character (end of input), a double quote, and if not
               currently inside of quotes, a space. */
            while ((Result) && (QCLI_Context.Input_String[Index] != '\0') &&
                   (QCLI_Context.Input_String[Index] != '"') &&
                   ((Inside_Quotes) || (QCLI_Context.Input_String[Index] != ' '))) {
                /* Handle escaped characters. */
                if (QCLI_Context.Input_String[Index] == '\\') {
                    if ((Index + 1) < Input_Length) {
                        /* Currently only '\' and '"' characters are escaped. */
                        if ((QCLI_Context.Input_String[Index + 1] == '\\') ||
                            (QCLI_Context.Input_String[Index + 1] == '\"')) {
                            /* Simply consume the escape character. */
                            memsmove(&(QCLI_Context.Input_String[Index]), Input_Length - Index,
                                     &(QCLI_Context.Input_String[Index + 1]), Input_Length - Index - 1);

                            Input_Length--;
                        } else {
                            QCLI_Printf(MAIN_PRINTF_HANDLE, "Invalid escape sequence \"\\%c\"\n",
                                        QCLI_Context.Input_String[Index + 1]);
                            Result = false;
                        }
                    } else {
                        QCLI_Printf(MAIN_PRINTF_HANDLE, "Invalid escape sequence\n");
                        Result = false;
                    }
                }

                Index++;
            }

            if (QCLI_Context.Input_String[Index] == '"') {
                /* The parameter ended in a quote so invert the flag indicating we
                   are inside of quotes. */
                Inside_Quotes = !Inside_Quotes;
            }

            /* Make sure the parameter string is NULL terminated. */
            if (QCLI_Context.Input_String[Index] != '\0') {
                QCLI_Context.Input_String[Index] = '\0';
                Index++;
            }

            /* Try to convert the command to an integer. */
            QCLI_Context.Parameter_List[Parameter_Count].Integer_Is_Valid =
                QCLI_String_To_Integer(QCLI_Context.Parameter_List[Parameter_Count].String_Value,
                                       &(QCLI_Context.Parameter_List[Parameter_Count].Integer_Value));

            Parameter_Count++;
        }
    }

    /* Make sure any quotes were properly terminated. */
    if (Inside_Quotes) {
        QCLI_Printf(MAIN_PRINTF_HANDLE, "\" not terminated\n");
        Result = false;
    }

    if ((Result) && (Parameter_Count > 0)) {
        /* Initialize the find results to the current group state so that it can
           be used to recursively search the groups. */
        Find_Result.Data.Group_List_Entry = QCLI_Context.Current_Group;
        Find_Result.Is_Group = true;
        Index = 0;
        QCLI_Context.Executing_Group = QCLI_Context.Current_Group;

        /* Search for the command that was entered. Note that if the command or
           group is found, the index will actually indicate the first parameter
           for the command. */
        while ((Result) && (Find_Result.Is_Group) && (Index < Parameter_Count)) {
            Result =
                Find_Command(Find_Result.Data.Group_List_Entry, &(QCLI_Context.Parameter_List[Index]), &Find_Result);

            /* If navigating into a subgroup, update the executing group. */
            if (Find_Result.Is_Group) {
                QCLI_Context.Executing_Group = Find_Result.Data.Group_List_Entry;
            }

            Index++;
        }

        if (Result) {
            if (Find_Result.Is_Group) {
                /* Final command is a group, navigate into it. */
                QCLI_Context.Current_Group = Find_Result.Data.Group_List_Entry;
            } else {
                /* Execute the command. */
                Execute_Command(QCLI_Context.Parameter_List[Index - 1].Integer_Value, Find_Result.Data.Command,
                                Parameter_Count - Index,
                                (Parameter_Count > Index) ? &(QCLI_Context.Parameter_List[Index]) : NULL);
            }
        } else {
            QCLI_Printf(MAIN_PRINTF_HANDLE, "Command \"%s", QCLI_Context.Parameter_List[0].String_Value);

            for (Command_Index = 1; Command_Index < Index; Command_Index++) {
                QCLI_Printf(MAIN_PRINTF_HANDLE, " %s", QCLI_Context.Parameter_List[Command_Index].String_Value);
            }

            QCLI_Printf(MAIN_PRINTF_HANDLE, "\" not found.\n");
        }
    }
}

/**
   @brief Unregisters the specified group from the command list and recursively
          unregisters any subgroup's that are registered for the group.

   @param[in] Group_List_Entry  Command group to be removed.

   @return
    - true if the current group changed as a result of the group being
      unregistered.
    - false if the current group didn't change.
*/
static qbool_t Unregister_Command_Group(Group_List_Entry_t *Group_List_Entry)
{
    qbool_t Ret_Val;
    Group_List_Entry_t *Current_Entry;
    Group_List_Entry_t *Remote_Entry;
    qbool_t Group_Is_Valid;

    /* First, remove the group from its parent's list. */
    if (Group_List_Entry->Parent_Group->Subgroup_List == Group_List_Entry) {
        /* Group is at the head of the subgroup list. */
        Group_List_Entry->Parent_Group->Subgroup_List = Group_List_Entry->Next;
        Group_Is_Valid = true;
    } else {
        /* Find the entry in its parent's subgroup list. */
        Current_Entry = Group_List_Entry->Parent_Group->Subgroup_List;

        while ((Current_Entry != NULL) && (Current_Entry->Next != Group_List_Entry)) {
            Current_Entry = Current_Entry->Next;
        }

        if (Current_Entry != NULL) {
            Current_Entry->Next = Group_List_Entry->Next;

            Group_Is_Valid = true;
        } else {
            Group_Is_Valid = false;
        }
    }

    if (Group_Is_Valid) {
        /* Unregister any subgroups of the command. */
        Current_Entry = Group_List_Entry->Subgroup_List;
        Ret_Val = false;

        while (Current_Entry != NULL) {
            Remote_Entry = Current_Entry;
            Current_Entry = Current_Entry->Next;
            if (Unregister_Command_Group(Remote_Entry)) {
                Ret_Val = true;
            }
        }

        /* If this is the current group, move up to its parent. */
        if (QCLI_Context.Current_Group == Group_List_Entry) {
            QCLI_Context.Current_Group = Group_List_Entry->Parent_Group;
            Ret_Val = true;
        }

        /* Free the resources for the group. */
        PAL_Free(Group_List_Entry);
    } else {
        Ret_Val = false;
    }

    return (Ret_Val);
}

/**
   @brief Initializes the QCLI module.

   This function must be called before any other QCLI functions.

   @param[in] Command_Group  Group of commands for the platform.  This can be
                             NULL if no platform commands are required.
                             Note that this function assumes the command group
                             information will be constant and simply stores a
                             pointer to the data.  If the group and its
                             associated information is not constant, its memory
                             must be retained.

   @return QCLI group handle that can be used for the platform abstraction to
           print to the console.
*/
QAPI_Console_Group_Handle_t QCLI_Initialize(const QAPI_Console_Command_Group_t *Command_Group)
{
    /* Initialize the context information. */
    memset(&QCLI_Context, 0, sizeof(QCLI_Context));
    QCLI_Context.Root_Group.Command_Group = Command_Group;
    QCLI_Context.Current_Group = &(QCLI_Context.Root_Group);
    qurt_signal_create(&QCLI_Context.Thread_Info.Thread_Ready_Event);

    return (MAIN_PRINTF_HANDLE);
}

/**
   @brief Passes characters input from the command line to the QCLI module for
          processing.

   @param[in] Length  Number of bytes in the provided buffer.
   @param[in] Buffer  Buffer containing the inputted data.

   @return
    - true if QCLI was initialized successfully.
    - false if initialization failed.
*/
void QCLI_Process_Input_Data(uint32_t Length, char *Buffer)
{
    if (PAL_Take_Lock()) {
        if ((Length) && (Buffer)) {
            /* Process all received data. */
            while (Length) {
                /* Check for an end of line character. */
                if (Buffer[0] == PAL_INPUT_END_OF_LINE_CHARACTER) {
                    /* Command is complete, process it now. */
                    Process_Command();
                    /* Set the command length back to zero in preparation of the next
                      command and display the prompt. */
                    QCLI_Context.Input_Length = 0;
                    memset(QCLI_Context.Input_String, '\0', sizeof(QCLI_Context.Input_String));
                    QCLI_Display_Prompt();
                } else {
                    /* Make sure that the command buffer can fit the character. */
                    if (QCLI_Context.Input_Length < QCLI_MAXIMUM_QCLI_COMMAND_STRING_LENGTH) {
                        QCLI_Context.Input_String[QCLI_Context.Input_Length] = Buffer[0];
                        QCLI_Context.Input_Length++;
                    }
                }

                /* Move to the next character in the buffer. */
                Buffer++;
                Length--;
            }
        }

        PAL_Release_Lock();
    }
}

/**
   @brief Displays the current command list.

   It is intended to provide a means for the initial command list to be
   displayed once platform initialization is complete.
*/
void QCLI_Display_Command_List(void)
{
    if (PAL_Take_Lock()) {
        Display_Command_List(QCLI_Context.Current_Group);

        QCLI_Display_Prompt();

        PAL_Release_Lock();
    }
}

/**
   @brief Registers a command group with the command line interface.

   NOTE: This function assumes the command group information will be constant
   and simply stores a pointer to the data.  If the group and its associated
   information is not constant, its memory must be retained until the group is
   unregistered.

   @param[in] Parent_Group   Group which this group should be registered under
                             as a subgroup.  If NULL, the group will be
                             registered at the top level.
   @param[in] Command_Group  Command group to be registered.

   @return
    - The handle for the group that was added.
    - NULL if there was an error registering the group.
*/
QAPI_Console_Group_Handle_t QCLI_Register_Command_Group(QAPI_Console_Group_Handle_t Parent_Group,
                                                        const QAPI_Console_Command_Group_t *Command_Group)
{
    Group_List_Entry_t *New_Entry;
    Group_List_Entry_t *Current_Entry;

    if (Command_Group != NULL) {
        if (PAL_Take_Lock()) {
            /* Create the new entry. */
            New_Entry = (Group_List_Entry_t *)PAL_Malloc(sizeof(Group_List_Entry_t));
            if (New_Entry) {
                New_Entry->Command_Group = Command_Group;
                New_Entry->Next = NULL;
                New_Entry->Subgroup_List = NULL;

                if (Parent_Group == NULL) {
                    New_Entry->Parent_Group = &(QCLI_Context.Root_Group);
                } else {
                    New_Entry->Parent_Group = (Group_List_Entry_t *)Parent_Group;
                }

                /* Add the new entry to its parents subgroup list. */
                if (New_Entry->Parent_Group->Subgroup_List == NULL) {
                    New_Entry->Parent_Group->Subgroup_List = New_Entry;
                } else {
                    Current_Entry = New_Entry->Parent_Group->Subgroup_List;
                    while (Current_Entry->Next != NULL) {
                        Current_Entry = Current_Entry->Next;
                    }

                    Current_Entry->Next = New_Entry;
                }
            }

            PAL_Release_Lock();
        } else {
            New_Entry = NULL;
        }
    } else {
        New_Entry = NULL;
    }

    return ((QAPI_Console_Group_Handle_t)New_Entry);
}

/**
   @brief Unregisters a command group from the command line interface.

   If the specified group has subgroups, they will be unregistered as well.

   @param[in] Group_Handle  Handle of the group to unregister.
*/
void QCLI_Unregister_Command_Group(QAPI_Console_Group_Handle_t Group_Handle)
{
    if (Group_Handle != NULL) {
        if (PAL_Take_Lock()) {
            if (Unregister_Command_Group((Group_List_Entry_t *)Group_Handle)) {
                /* The current menu level changed so update the prompt. */
                QCLI_Display_Prompt();
            }

            PAL_Release_Lock();
        }
    }
}

/**
   @brief Prints the prompt to the console.

   This function provides a means to re-display the prompt after printing data
   to the console from an asynchronous function such as a callback.
*/
void QCLI_Display_Prompt(void)
{
    if (PAL_Take_Lock()) {
        QCLI_Printf(MAIN_PRINTF_HANDLE, "\n");

        /* Recursively display the name for the current group. */
        if (QCLI_Context.Current_Group->Parent_Group != NULL) {
            Display_Group_Name(QCLI_Context.Current_Group);
        }

        QCLI_Printf(MAIN_PRINTF_HANDLE, ">\r\n>");

        /* Display the current command string. */
        if (QCLI_Context.Input_Length != 0) {
            PAL_Console_Write(QCLI_Context.Input_Length, QCLI_Context.Input_String);
        }

        PAL_Release_Lock();
    }
}

/**
   @brief Prints a formated string to the CLI.

   This function will also replace newline characters ('\n') with the string
   specified by PAL_OUTPUT_END_OF_LINE_STRING.

   @param[in] QCLI_Handle   Handle of the QCLI group that is printing the
                            string.
   @param[in] Format        Formated string to be printed.
   @param[in] ...           Variatic parameter for the format string.
*/
void QCLI_Printf(QAPI_Console_Group_Handle_t Group_Handle, const char *Format, ...)
{
    uint32_t Index;
    uint32_t Next_Print_Index;
    uint32_t Length;
    va_list Arg_List;
    Group_List_Entry_t *Group_List_Entry;

    if ((Group_Handle != NULL) && (Format != NULL)) {
        if (PAL_Take_Lock()) {
            Group_List_Entry = (Group_List_Entry_t *)Group_Handle;

            /* Print the group name first. Note that the main handle indicates the
               message is from the QCLI itself and as such doesn't print a group
               name. */
            if (Group_Handle != QCLI_Context.Current_Printf_Group) {
                if (QCLI_Context.Printf_New_Line) {
                    PAL_Console_Write(sizeof(PAL_OUTPUT_END_OF_LINE_STRING) - 1, PAL_OUTPUT_END_OF_LINE_STRING);
                    QCLI_Context.Printf_New_Line = false;
                }

                if ((Group_Handle != MAIN_PRINTF_HANDLE) && (Format[0] != '\n')) {
                    Display_Group_Name(Group_List_Entry);
                    PAL_Console_Write(2, ": ");
                }
            }

            QCLI_Context.Current_Printf_Group = Group_Handle;

            /* Print the string to the buffer. */
            va_start(Arg_List, Format);
            Length = vsnprintf((char *)(QCLI_Context.Printf_Buffer), sizeof(QCLI_Context.Printf_Buffer), (char *)Format,
                               Arg_List);
            va_end(Arg_List);

            /* Make sure the length is not greater than the buffer size (taking the
               NULL terminator into account). */
            if (Length > sizeof(QCLI_Context.Printf_Buffer) - 1) {
                Length = sizeof(QCLI_Context.Printf_Buffer) - 1;
            }

            /* Write the buffer to the console, setting EOL characters accordingly. */
            Next_Print_Index = 0;
            for (Index = 0; Index < Length; Index++) {
                if (QCLI_Context.Printf_Buffer[Index] == '\n') {
                    /* Print out the buffer so far and replace the '\n' with the
                       configured EOL string. */
                    if (Index != Next_Print_Index) {
                        PAL_Console_Write(Index - Next_Print_Index, &(QCLI_Context.Printf_Buffer[Next_Print_Index]));
                    }

                    PAL_Console_Write(sizeof(PAL_OUTPUT_END_OF_LINE_STRING) - 1, PAL_OUTPUT_END_OF_LINE_STRING);

                    Next_Print_Index = Index + 1;

                    if (Length != (Index + 1)) {
                        /* Redisplay the group name at the start of a new line if its
                           not immediately followed by another new line. */
                        if (QCLI_Context.Printf_Buffer[Index + 1] != '\n') {
                            if ((Group_List_Entry->Command_Group != NULL) && (Group_List_Entry != MAIN_PRINTF_HANDLE)) {
                                PAL_Console_Write(strlen((char *)(Group_List_Entry->Command_Group->Group_String)),
                                                  Group_List_Entry->Command_Group->Group_String);
                                PAL_Console_Write(2, ": ");
                            }
                        }
                    } else {
                        /* This printout stopped on the newline so set the current
                           print group to the main group to prompt the next line to
                           redisplay the group name. */
                        QCLI_Context.Current_Printf_Group = MAIN_PRINTF_HANDLE;
                    }

                    QCLI_Context.Printf_New_Line = false;
                } else {
                    QCLI_Context.Printf_New_Line = true;
                }
            }

            /* Print the remaining buffer after the last newline. */
            if (Length != Next_Print_Index) {
                PAL_Console_Write(Length - Next_Print_Index, &(QCLI_Context.Printf_Buffer[Next_Print_Index]));
            }

            PAL_Release_Lock();
        }
    }
}
