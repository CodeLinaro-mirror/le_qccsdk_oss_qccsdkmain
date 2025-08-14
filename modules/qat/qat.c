/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*-------------------------------------------------------------------------
 * Include Files
 *-----------------------------------------------------------------------*/

#include <stdio.h>
#include <stdarg.h>
#include "string.h"
#include "qapi_version.h"
#include "qat.h"
#include "qat_api.h"
#include "qat_uart.h"

#include "qurt_internal.h"
#include "nt_osal.h"
#include "qurt_mutex.h"
#include "wifi_fw_version.h"

/*-------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 *-----------------------------------------------------------------------*/

/**
   Group handle which represents the QCLI module itself.  This is used to print
   messages without a group prefix.
*/
//#define QAT_LOG_ENABLE 1
#ifdef QAT_LOG_ENABLE
extern void *QCLI_Context;
#define QAT_Printf(Fmt, ...) QCLI_Printf(&QCLI_Context, Fmt, ##__VA_ARGS__)
#else
#define QAT_Printf(...)
#endif

/* QAT signals */
#define QAT_EVENT_TXQ 0x1

#define QAT_STR_BUFFER_LENGTH 128
#define QAT_OUTPUT_MAX_LENGTH 1400

QAT_Transfer_Mode_t QAT_Transfer_Mode = QAT_Transfer_Mode_AT_COMMAND_E;
QAT_Transfer_Mode_Handle_t QAT_Transfer_Mode_Handle = NULL;
uint32_t QAT_Echo_Enable = 0;
qurt_signal_t qat_task_start;
nt_osal_task_handle_t qat_tx_task_hdl = NULL;
nt_osal_task_handle_t qat_rx_task_hdl = NULL;

/*-------------------------------------------------------------------------
 * Type Declarations
 *-----------------------------------------------------------------------*/

/**
   Information for the result of a Find_Command() operation.
*/
typedef struct Find_Result_s {
    QAT_Command_t *Command; /**< Entry that was found if it is a command. */
} Find_Result_t;

HTC_Context_t HTC_Context;
const char Rel_Date[] = __DATE__;
const char Rel_Time[] = __TIME__;

Cur_Data_Mode_Cmd_t Cur_Data_Mode_Cmd;
/*-------------------------------------------------------------------------
 * Function Declarations
 *-----------------------------------------------------------------------*/

static void Process_Command(void);
static qbool_t Find_Command_By_String(uint8_t *Command_Name, Find_Result_t *Find_Result);

extern qbool_t QCLI_String_To_Integer(const char *String, int32_t *Output);
extern size_t memsmove(void *Dest, size_t DestSize, const void *Src, size_t SrcSize);
extern qbool_t PAL_Take_Lock(void);
extern void PAL_Release_Lock(void);
extern int32_t QCLI_Memcmpi(const void *Source1, const void *Source2, uint32_t Size);

extern void UartRxTasks();
extern void SPIRxTasks();

void QAT_Response_Event(char *Command_Name, char *Buffer, int rc_code);
qapi_Status_t QAT_Output(uint32_t Length, const char *Buffer);

qbool_t (*Process_Input_Data_Handle)(uint32_t Length, char *Buffer);

char *QAT_Result_Str[] = {
    "OK", "CONNECT", "RING", "NO CARRIER", "ERROR", "", "NO DIAL TONE", "BUSY", "NO ANSWER", "", "OK", "OK",
};

static void Process_RAW_Data(void)
{
    qbool_t Result;
    Find_Result_t Find_Result;
    Result = Find_Command_By_String(Cur_Data_Mode_Cmd.cur_data_mode_commnd, &Find_Result);
    if (!Result) {
        printf("Command search failed: %s\n", Cur_Data_Mode_Cmd.cur_data_mode_commnd);
        QAT_Transfer_Mode_set(QAT_Transfer_Mode_AT_COMMAND_E, NULL);
        QAT_Response_Str(QAT_RC_ERROR, NULL);
    }

    if (QAT_STATUS_SUCCESS_E !=
        (*(Find_Result.Command->Command_Function))(QAT_OP_EXEC_IN_DATA_MODEL, HTC_Context.Input_Length,
                                                   (QAT_Parameter_t *)HTC_Context.Input_String)) {
        Result = false;
    }
}

qbool_t QAT_Data_Transfer_Mode_Handle(uint32_t Length, uint8_t *Buffer)
{
    qbool_t Result = true;

    if (QAT_Transfer_Mode == QAT_Transfer_Mode_ONLINE_DATA_E) {
        if ((Length) && (Buffer)) {
            /* pause the event sendind */
            if (HTC_Context.Input_Length == 0) {
                ;
            }

            /* Process all received data. */
            while (Length) {
                /* Make sure that the command buffer can fit the character. */
                if (HTC_Context.Input_Length < QAT_MAXIMUM_COMMAND_STRING_LENGTH) {
                    if (QAT_Echo_Enable) {
                        QAT_Output(1, Buffer);
                    }

                    HTC_Context.Input_String[HTC_Context.Input_Length] = Buffer[0];
                    HTC_Context.Input_Length++;
                }

                /* Move to the next character in the buffer. */
                Buffer++;
                Length--;
            }

            if (Length == 0) {
                if (strncmp(HTC_Context.Input_String, "+++", strlen("+++")) == 0) {
                    QAT_Transfer_Mode_set(QAT_Transfer_Mode_AT_COMMAND_E, NULL);
                    QAT_Response_Str(QAT_RC_OK, NULL);
                } else if (HTC_Context.Input_Length > 0) {
                    /*Data block is complete, process it now. */
                    Process_RAW_Data();
                }
                /* Set the command length back to zero in preparation of the next
                command and display the prompt. */
                memset(HTC_Context.Input_String, '\0', HTC_Context.Input_Length);
                HTC_Context.Input_Length = 0;
            }
        }
    }
}

qbool_t QAT_Transfer_Mode_set(QAT_Transfer_Mode_t Mode, QAT_Transfer_Mode_Handle_t Handle)
{
    QAT_Transfer_Mode = Mode;

    if (Mode != QAT_Transfer_Mode_AT_COMMAND_E)
        QAT_Transfer_Mode_Handle = Handle;
    else
        QAT_Transfer_Mode_Handle = NULL;

    return true;
}

/**
   @brief Processes a command received from the console.
*/
static int Process_AT_Extend_Command(uint32_t *Command_Index)
{
    qbool_t Result;
    Find_Result_t Find_Result;
    uint32_t Parameter_Count = 0;
    uint32_t i;
    qbool_t Inside_Quotes = false;
    uint8_t Temp_Char;
    uint8_t is_exec = 0;
    uint32_t Index = *Command_Index;
    uint8_t *Command_Name;
    qbool_t Sub_Command = false;
    char buffer[QAT_STR_BUFFER_LENGTH];

    Result = true;
    Command_Name = NULL;

    while ((Result) && (Index < HTC_Context.Input_Length) && (HTC_Context.Input_String[Index] != ';')) {
        /* Consume any leading ' ' */
        while (HTC_Context.Input_String[Index] == ' ') {
            Index++;
            continue;
        }

        if ((HTC_Context.Input_String[Index] == '+') && !HTC_Context.Command_Name) {
            HTC_Context.Command_Name = (uint8_t *)HTC_Context.Input_String + Index;
            Command_Name = HTC_Context.Command_Name;
            Index++;
            Command_Name++;
            HTC_Context.Command_Flag |= QAT_STR_NA;
            continue;
        }

        if (Command_Name) {
            if (HTC_Context.Input_String[Index] == '=') {
                *Command_Name = '\0';
                is_exec = 0;
                HTC_Context.Command_Flag |= QAT_STR_EQ;
                Index++;
                if (Index < HTC_Context.Input_Length) {
                    if (HTC_Context.Input_String[Index] == '?') {
                        Index++;
                        HTC_Context.Command_Flag |= QAT_STR_QU;
                    } else {
                        /* Need take more args */
                        HTC_Context.Command_Flag |= QAT_STR_AR;
                    }
                    break;
                } else {
                    Result = false;
                    QAT_Response_Str(QAT_RC_ERROR, NULL);
                    break;
                }
            } else if (HTC_Context.Input_String[Index] == '?') {
                Index++;

                if ((HTC_Context.Input_String[Index] != ';') && (HTC_Context.Input_String[Index] != '\r') &&
                    (HTC_Context.Input_String[Index] != NULL)) {
                    Result = false;
                    QAT_Response_Str(QAT_RC_ERROR, NULL);

                    break;
                }

                HTC_Context.Command_Flag |= QAT_STR_QU;
                *(char *)(Command_Name) = '\0';
                is_exec = 0;
                break;
            } else {
                Temp_Char = HTC_Context.Input_String[Index];

                is_exec = 1;

                if ((Temp_Char >= '0' && Temp_Char <= '9') || ((Temp_Char) >= 'a' && (Temp_Char) <= 'z') ||
                    (Temp_Char >= 'A' && Temp_Char <= 'Z') || (Temp_Char == '!') || (Temp_Char == '%') ||
                    (Temp_Char == '-') || (Temp_Char == '.') || (Temp_Char == '/') || (Temp_Char == ':') ||
                    (Temp_Char == '_'))  // valid name
                {
                    Command_Name++;  // = Temp_Char;
                } else {
                    Result = false;
                    QAT_Response_Str(QAT_RC_ERROR, NULL);

                    break;
                }
            }
            Index++;
        }
    }
    if (Result && (HTC_Context.Command_Flag & QAT_STR_NA)) {
        /* Initialize the find results to the current group state so that it can
        be used to recursively search the groups. */
        if (is_exec == 1)
            *(char *)(Command_Name) = '\0';

        Result = Find_Command_By_String(HTC_Context.Command_Name, &Find_Result);
        if (!Result) {
            printf("Command search failed: %s\n", HTC_Context.Command_Name);
            QAT_Response_Str(QAT_RC_ERROR, NULL);
        }
        if (is_exec == 1)
            *(char *)(Command_Name) = ';';
    }

    /* search parameter if needed */
    if (Result && ((HTC_Context.Command_Flag & QAT_STR_AR) == QAT_STR_AR)) {
        while (Result && (Index < HTC_Context.Input_Length) && (Parameter_Count <= QAT_MAXIMUM_NUMBER_OF_PARAMETERS) &&
               (HTC_Context.Input_String[Index] != ';')) {
            /* Consume any leading ' '*/
            if (HTC_Context.Input_String[Index] == ' ' || HTC_Context.Input_String[Index] == '{' ||
                HTC_Context.Input_String[Index] == ',' ||
                HTC_Context.Input_String[Index] ==
                    '}' /*|| HTC_Context.Input_String[Index] == ',' || HTC_Context.Input_String[Index] == '='*/) {
                Index++;
                continue;
            }

            /* If the first character is '"', consume it. */
            if (HTC_Context.Input_String[Index] == '"') {
                Inside_Quotes = true;
                Index++;
            }

            /* Assuming the end of the command hasn't been reached, assign the current
            string location as the current parameter's string. */
            if ((Index < HTC_Context.Input_Length) && (HTC_Context.Input_String[Index] != ';') &&
                (HTC_Context.Input_String[Index] != ',')) {
                HTC_Context.Parameter_List[Parameter_Count].String_Value = &HTC_Context.Input_String[Index];

                /* Find the end of the paramter.	The end of parameter is determined as
                either a null character (end of input), a double quote, and if not
                currenlty inside of quotes, a space. */
                while ((Result) && (Index < HTC_Context.Input_Length) && (HTC_Context.Input_String[Index] != '"') &&
                       ((Inside_Quotes) || (HTC_Context.Input_String[Index] != ',')) &&
                       (HTC_Context.Input_String[Index] != ';') && (HTC_Context.Input_String[Index] != '}')) {
                    /* Handle escaped characters. */
                    if (HTC_Context.Input_String[Index] == '\\') {
                        if ((Index + 1) < HTC_Context.Input_Length) {
                            /* Currently only '\' and '"' characters are escaped. */
                            if ((HTC_Context.Input_String[Index + 1] == '\\') ||
                                (HTC_Context.Input_String[Index + 1] == '\"')) {
                                /* Simply consume the escape character. */
                                memsmove(&(HTC_Context.Input_String[Index]), HTC_Context.Input_Length - Index,
                                         &(HTC_Context.Input_String[Index + 1]), HTC_Context.Input_Length - Index - 1);

                                HTC_Context.Input_String[HTC_Context.Input_Length - 1] = '\0';

                                HTC_Context.Input_Length--;
                            }
                            /* Currently only '\xhh' and '"' characters are escaped. */
                            else if (HTC_Context.Input_String[Index + 1] == 'x') {
                                if (HTC_Context.Input_Length > (Index + 3)) {
                                    /* Simply consume the escape character. */
                                    HTC_Context.Input_String[Index] = 0;

                                    for (i = 0; i < 2; i++) {
                                        Temp_Char = HTC_Context.Input_String[Index + 2 + i];

                                        if (Temp_Char >= '0' && Temp_Char <= '9') {
                                            Temp_Char -= '0';
                                        } else if ((Temp_Char) >= 'a' && (Temp_Char) <= 'f') {
                                            Temp_Char -= 'a';
                                            Temp_Char += 10;
                                        } else if (Temp_Char >= 'A' && Temp_Char <= 'F') {
                                            Temp_Char -= 'A';
                                            Temp_Char += 10;
                                        } else {
                                            QAT_Printf("Invalid escape sequence \\x\n");
                                            Result = false;
                                            goto clean_sources;
                                        }
                                        HTC_Context.Input_String[Index] |= (Temp_Char << (4 * (1 - i)));
                                    }

                                    memsmove(
                                        &(HTC_Context.Input_String[Index + 1]), HTC_Context.Input_Length - Index - 1,
                                        &(HTC_Context.Input_String[Index + 4]), HTC_Context.Input_Length - Index - 4);

                                    HTC_Context.Input_String[HTC_Context.Input_Length - 3] = '\0';
                                    HTC_Context.Input_Length -= 2;
                                } else {
                                    QAT_Printf("Invalid escape sequence \\x\n");
                                    Result = false;
                                    QAT_Response_Str(QAT_RC_ERROR, NULL);
                                }

                            } else {
                                QAT_Printf("Invalid escape sequence \"\\%c\"\n", HTC_Context.Input_String[Index + 1]);
                                Result = false;
                                QAT_Response_Str(QAT_RC_ERROR, NULL);
                            }
                        } else {
                            QAT_Printf("Invalid escape sequence\n");
                            Result = false;
                            QAT_Response_Str(QAT_RC_ERROR, NULL);
                        }
                    }

                    Index++;
                }

                if ((HTC_Context.Input_String[Index] == ',') &&
                    (HTC_Context.Parameter_List[Parameter_Count].String_Value == &HTC_Context.Input_String[Index])) {
                    /* The parameter ended in a quote so invert the flag indicating we
                    are inside of quotes. */
                    QAT_Printf("default parameter index %d\n", Parameter_Count);
                    HTC_Context.Default_Paramter_Flag |= (1 << Parameter_Count);
                }

                if (HTC_Context.Input_String[Index] == '"') {
                    /* The parameter ended in a quote so invert the flag indicating we
                    are inside of quotes. */
                    Inside_Quotes = !Inside_Quotes;
                    HTC_Context.Input_String[Index] = '\0';
                    Index++;

                    if ((!Inside_Quotes) && ((Index) < HTC_Context.Input_Length)) {
                        if (HTC_Context.Input_String[Index] == ',') {
                            HTC_Context.Input_String[Index] = '\0';
                        }
                    }
                }

                /* Make sure the parameter string is NULL terminated. */
                if (HTC_Context.Input_String[Index] == ';') {
                    Sub_Command = true;
                }

                /* Make sure the parameter string is NULL terminated. */
                if (Index < HTC_Context.Input_Length) {
                    HTC_Context.Input_String[Index] = '\0';
                    Index++;
                }

                /* Try to convert the command to an integer. */
                HTC_Context.Parameter_List[Parameter_Count].Integer_Is_Valid =
                    QCLI_String_To_Integer(HTC_Context.Parameter_List[Parameter_Count].String_Value,
                                           &(HTC_Context.Parameter_List[Parameter_Count].Integer_Value));

                Parameter_Count++;

                if (Sub_Command) {
                    break;
                }
            }
        }

        /* Make sure any quotes were properly terminated. */
        if (Inside_Quotes) {
            printf("\" not terminated\n");
            Result = false;
            QAT_Response_Str(QAT_RC_ERROR, NULL);
        }
    }

    if (Result && HTC_Context.Command_Flag) {
        uint32_t Op_Type;

        Temp_Char = HTC_Context.Command_Flag & 0xf;

        if (Temp_Char == (QAT_STR_NA | QAT_STR_QU)) {
            Op_Type = QAT_OP_QUERY;
        } else if (Temp_Char == (QAT_STR_NA | QAT_STR_EQ | QAT_STR_QU)) {
            Op_Type = QAT_OP_HELP;
        } else if (Temp_Char == (QAT_STR_NA)) {
            Op_Type = QAT_OP_EXEC;
        } else if (Temp_Char == (QAT_STR_NA | QAT_STR_EQ | QAT_STR_AR)) {
            Op_Type = QAT_OP_EXEC_W_PARAM;
        } else {
            Result = false;
        }

        if (Result) {
            QAT_Printf("Command \"%s\"", HTC_Context.Command_Name);

            for (i = 0; i < Parameter_Count; i++) {
                QAT_Printf(" %s", HTC_Context.Parameter_List[i].String_Value);
            }
            QAT_Printf("\n");

            /* Execute the command. */
            if (Find_Result.Command->Command_Flags & Op_Type) {
                if (QAT_STATUS_SUCCESS_E !=
                    (*(Find_Result.Command->Command_Function))(
                        Op_Type, Parameter_Count, (Parameter_Count) ? &(HTC_Context.Parameter_List[0]) : NULL)) {
                    Result = false;
                }
            } else {
                QAT_Response_Str(QAT_RC_ERROR, NULL);
            }
        } else {
            QAT_Response_Str(QAT_RC_ERROR, NULL);
        }
    }

clean_sources:
    HTC_Context.Command_Flag = 0;
    HTC_Context.Command_Name = NULL;
    HTC_Context.Default_Paramter_Flag = 0;
    memset(HTC_Context.Parameter_List, 0, sizeof(HTC_Context.Parameter_List));
    *Command_Index = Index;
    return Result;
}

/**
   @brief Processes a command received from the console.
*/
static void Process_Command(void)
{
    qbool_t Result;
    uint32_t Index;
    uint32_t Input_Length;
    qbool_t Find_At = 0;
    qbool_t Sub_Command = false;
    // char *        Command_name=NULL;

    /* Store the input length locally so any re-displays of the prompt will not
    include the command. */
    Input_Length = HTC_Context.Input_Length;

    /* Parse the command until its end is reached or the parameter list is full. */
    Index = 0;
    Result = true;

    /* Consume any leading white space. */
    while (HTC_Context.Input_String[Index] == ' ') {
        Index++;
    }

    /* search "AT" */
    if ((!HTC_Context.Command_Name) && ((Index + 1) < Input_Length)) {
        if (((HTC_Context.Input_String[Index] | 0x20) == 'a') &&
            ((HTC_Context.Input_String[Index + 1] | 0x20) == 't')) {
            Find_At = 1;
            Index += 2;
        }
    }

    if (!Find_At) {
        if (HTC_Context.Input_Length && (HTC_Context.Input_String[Index] != ' ') &&
            (HTC_Context.Input_String[Index] != '\0') && (HTC_Context.Input_String[Index] != '=') &&
            (HTC_Context.Input_String[Index] != '?') && (HTC_Context.Input_String[Index] != '\r')) {
            QAT_Response_Str(QAT_RC_ERROR, NULL);
        }
        return;
    }

    /* Consume any leading white space. */
    while (HTC_Context.Input_String[Index] == ' ') {
        Index++;
    }

    /* search extend command */
    while ((Result == true) && (Index < HTC_Context.Input_Length)) {
        /* Consume any leading white space. */
        if ((HTC_Context.Input_String[Index] == ' ') || (HTC_Context.Input_String[Index] == ';')) {
            Index++;
            continue;
        }

        if (HTC_Context.Input_String[Index] == '+') {
            /*AT+xxx, */
            Result = Process_AT_Extend_Command(&Index);
            Sub_Command = true;
        } else {
            Result = false;
            QAT_Response_Str(QAT_RC_ERROR, NULL);
        }
    }

    if (Find_At && (Sub_Command == false)) {
        QAT_Response_Str(QAT_RC_OK, NULL);
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

qbool_t QAT_Register_Command_Group(QAT_Command_t *Command_Group, uint32_t number)
{
    Group_List_Entry_t *New_Entry;
    Group_List_Entry_t *Current_Entry;

    if (Command_Group != NULL) {
        if (PAL_Take_Lock()) {
            /* Create the new entry. */
            New_Entry = (Group_List_Entry_t *)PAL_Malloc(sizeof(Group_List_Entry_t));
            if (New_Entry) {
                New_Entry->Command_Group.Command_Count = number;
                New_Entry->Command_Group.Command_List = Command_Group;
                New_Entry->Next = NULL;

                Current_Entry = HTC_Context.Next;

                if (!Current_Entry) {
                    HTC_Context.Next = New_Entry;
                } else {
                    /* Add the new entry to its parents subgroup list. */
                    while (Current_Entry->Next) {
                        Current_Entry = Current_Entry->Next;
                    }

                    Current_Entry->Next = New_Entry;
                }
            }

            PAL_Release_Lock();
        }

        return true;
    } else
        return false;
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
qbool_t QAT_Unregister_Command_Group(QAT_Command_t *Group_List_Entry)
{
    Group_List_Entry_t *New_Entry;
    Group_List_Entry_t *Current_Entry;
    qbool_t Find_One = false;

    if (PAL_Take_Lock()) {
        /* Create the new entry. */
        Current_Entry = HTC_Context.Next;
        if (!Current_Entry) {
            PAL_Release_Lock();
            return false;
        }
        if (Current_Entry && !Current_Entry->Next) {
            if (Current_Entry->Command_Group.Command_List == Group_List_Entry) {
                HTC_Context.Next = NULL;
                Find_One = true;
                New_Entry = Current_Entry;
            }
        }
        if (Current_Entry && Current_Entry->Next) {
            while (Current_Entry->Next) {
                if (Current_Entry->Next->Command_Group.Command_List == Group_List_Entry) {
                    New_Entry = Current_Entry->Next;
                    Current_Entry->Next = New_Entry->Next;
                    Find_One = true;
                    break;
                }
            }
        }
        PAL_Release_Lock();
    }

    if (Find_One && New_Entry) {
        memset(New_Entry, 0, sizeof(Group_List_Entry_t));

        free(New_Entry);
        New_Entry = NULL;
    }

    return Find_One;
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
static qbool_t Find_Command_By_String(uint8_t *Command_Name, Find_Result_t *Find_Result)
{
    const QAT_Command_Group_t *Command_Group; /**< Command group information. */
    Group_List_Entry_t *Current_Entry = HTC_Context.Next;
    int Index;

    if (!Command_Name || !Find_Result) {
        return false;
    }

    while (Current_Entry) {
        /* Add the new entry to its parents subgroup list. */
        Command_Group = &Current_Entry->Command_Group;
        for (Index = 0; Index < Command_Group->Command_Count; Index++) {
            if ((strlen(Command_Group->Command_List[Index].Command_String) == strlen(Command_Name)) &&
                (!QCLI_Memcmpi(Command_Group->Command_List[Index].Command_String, Command_Name,
                               strlen(Command_Name)))) {
                Find_Result->Command = &Command_Group->Command_List[Index];
                return true;
            }
        }
        Current_Entry = Current_Entry->Next;
    }

    return false;
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
qbool_t QAT_Process_Input_Data(uint32_t Length, char *Buffer)
{
    qbool_t Result = true;

    if (QAT_Transfer_Mode == QAT_Transfer_Mode_AT_COMMAND_E) {
        if ((Length) && (Buffer)) {
            /* pause the event sendind */
            if (HTC_Context.Input_Length == 0) {
                ;
            }

            /* Process all received data. */
            while (Length) {
                /* Check for an end of line character. */
                if (Buffer[0] == PAL_INPUT_END_OF_LINE_CHARACTER) {
                    if (QAT_Echo_Enable) {
                        if (!HTC_Context.Input_Length)
                            QAT_Output(sizeof(PAL_OUTPUT_END_OF_LINE_STRING) - 1, PAL_OUTPUT_END_OF_LINE_STRING);
                    }

                    /* Command is complete, process it now. */
                    Process_Command();

                    /* Set the command length back to zero in preparation of the next
                    command and display the prompt. */
                    memset(HTC_Context.Input_String, '\0', HTC_Context.Input_Length);
                    HTC_Context.Input_Length = 0;

                } else {
                    /* Check for backspace character. */
                    if (Buffer[0] == '\b') {
                        /* Consume a character from the command if one has been
                        entered. */
                        if (HTC_Context.Input_Length) {
                            if (QAT_Echo_Enable) {
                                QAT_Output(3, "\b \b");
                            }

                            if (HTC_Context.Input_Length) {
                                HTC_Context.Input_Length--;
                                HTC_Context.Input_String[HTC_Context.Input_Length] = '\0';
                            }
                        }
                    } else {
                        /* Check for a valid character, which here is any non control
                        code lower ASCII (0x20 ' ' to 0x7E '~'). */
                        if ((*Buffer >= ' ') && (*Buffer <= '~')) {
                            /* Make sure that the command buffer can fit the character. */
                            if (HTC_Context.Input_Length < QAT_MAXIMUM_COMMAND_STRING_LENGTH) {
                                if (QAT_Echo_Enable) {
                                    QAT_Output(1, Buffer);
                                }

                                HTC_Context.Input_String[HTC_Context.Input_Length] = Buffer[0];
                                HTC_Context.Input_Length++;

                                Result = false;
                            }
                        }
                    }
                }

                /* Move to the next character in the buffer. */
                Buffer++;
                Length--;
            }
        }
    } else {
        if (QAT_Echo_Enable) {
            QAT_Output(Length, Buffer);
        }
        Result = QAT_Transfer_Mode_Handle(Length, (uint8_t *)Buffer);
    }

    //      PAL_Release_Lock();
    return Result;
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
QAT_Command_Status_t QAT_Response_Str(int Ret_Code, char *Buffer)
{
    QAT_Tx_Queue_t *Tx_Queue, *Tx_Queue_Head;
    uint32_t Buffer_Len, Total_Len;
    char *Ptr;
    int Len;
    uint32_t Input_Buffer_Length = 0;

    if (Ret_Code > QAT_RC_MAX) {
        return QAT_STATUS_ERROR_E;
    }

    if (!QAT_Result_Str[Ret_Code])
        return QAT_STATUS_ERROR_E;

    if (Buffer)
        Input_Buffer_Length = strlen(Buffer);

    Buffer_Len = strlen((char *)QAT_Result_Str[Ret_Code]) + Input_Buffer_Length + 3 * sizeof("\r\n");
    Total_Len = sizeof(QAT_Tx_Queue_t) + Buffer_Len;

    /* extra bytes: \r\n <CMD name> */
    Tx_Queue = malloc(Total_Len + 1);
    if (Tx_Queue) {
        memset(Tx_Queue, 0, (Total_Len + 1));
        Tx_Queue->Buffer = (uint8_t *)((char *)Tx_Queue + sizeof(QAT_Tx_Queue_t));
        Ptr = (char *)Tx_Queue->Buffer;

        if (Ret_Code != QAT_RC_QUIET_NO_CR) {
            Len = snprintf(Ptr, Buffer_Len, "%s", "\r\n");
            Buffer_Len -= Len, Ptr += Len;
            Tx_Queue->Len = Len;
        }

        if (Buffer && Input_Buffer_Length) {
            memcpy(Ptr, Buffer, Input_Buffer_Length);
            Ptr += Input_Buffer_Length;

            if (Ret_Code != QAT_RC_QUIET_NO_CR) {
                memcpy(Ptr, "\r\n", 2);
                Ptr += 2;
                Tx_Queue->Len += (Input_Buffer_Length + 2);
            } else {
                Tx_Queue->Len += (Input_Buffer_Length);
            }
        }

        if (Ret_Code != QAT_RC_QUIET && Ret_Code != QAT_RC_QUIET_NO_CR) {
            if (QAT_Result_Str[Ret_Code] && strlen(QAT_Result_Str[Ret_Code])) {
                memcpy(Ptr, (char *)QAT_Result_Str[Ret_Code], strlen(QAT_Result_Str[Ret_Code]));
                Ptr += strlen(QAT_Result_Str[Ret_Code]);

                memcpy(Ptr, "\r\n", 2);
                Ptr += 2;
                Tx_Queue->Len += (strlen(QAT_Result_Str[Ret_Code]) + 2);
            }
        }

        /* queue buffer, then signal */
        qurt_mutex_lock(&HTC_Context.mutex);

        if (!HTC_Context.Tx_Queue) {
            HTC_Context.Tx_Queue = Tx_Queue;
        } else {
            Tx_Queue_Head = HTC_Context.Tx_Queue;
            while (Tx_Queue_Head && Tx_Queue_Head->Next) {
                Tx_Queue_Head = Tx_Queue_Head->Next;
            }
            Tx_Queue_Head->Next = Tx_Queue;
        }
        qurt_mutex_unlock(&HTC_Context.mutex);

        /* indicate data ready */
        qurt_signal_set(&qat_task_start, QAT_EVENT_TXQ);
    }

    return QAT_STATUS_SUCCESS_E;
}

qapi_Status_t QAT_Output(uint32_t Length, const char *Buffer)
{
    uint8_t ret = QAPI_OK;

    if ((Length != 0) && (Buffer != NULL)) {
#if (CONFIG_QAT_TRANSMISSION_MODULE == 0)
        QAT_UART_Output(Length, Buffer);
#else if (CONFIG_QAT_TRANSMISSION_MODULE == 1)
        QAT_SPI_Output(Length, Buffer);
#endif
    } else {
        ret = QAPI_ERR_INVALID_PARAM;
    }

    return ret;
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
QAT_Command_Status_t QAT_Response_Buffer(int Ret_Code, char *Buffer, uint32_t Length)
{
    QAT_Tx_Queue_t *Tx_Queue, *Tx_Queue_Head;
    uint32_t Len_Need;
    char *Ptr;
    int len;

    if (!Buffer || !Length || (Ret_Code > QAT_RC_MAX)) {
        return QAT_STATUS_ERROR_E;
    }

    if (Ret_Code != QAT_RC_QUIET) {
        Len_Need = sizeof(QAT_Tx_Queue_t) + strlen((char *)QAT_Result_Str[Ret_Code]) + Length + sizeof("\r\n");
    } else {
        Len_Need = sizeof(QAT_Tx_Queue_t) + strlen((char *)QAT_Result_Str[Ret_Code]) + Length;
    }

    /* extra bytes: \r\n <CMD name> */
    Tx_Queue = malloc(Len_Need + 1);
    if (Tx_Queue) {
        memset(Tx_Queue, 0, (Len_Need + 1));
        Tx_Queue->Buffer = (uint8_t *)((char *)Tx_Queue + sizeof(QAT_Tx_Queue_t));
        Ptr = (char *)Tx_Queue->Buffer;
        // len = snprintf(Ptr, Len_Need, "%s", "\r\n");
        // Ptr += len;
        // Tx_Queue->Len = len;
        if (Buffer && Length) {
            memcpy(Ptr, Buffer, Length);
            Ptr += Length;
            // memcpy(Ptr, "\r\n", 2);
            // Ptr += 2;
            // Tx_Queue->Len += (Length + 2);
            Tx_Queue->Len += Length;
        }
        if (Ret_Code != QAT_RC_QUIET) {
            if (QAT_Result_Str[Ret_Code] && strlen(QAT_Result_Str[Ret_Code])) {
                memcpy(Ptr, (char *)QAT_Result_Str[Ret_Code], strlen(QAT_Result_Str[Ret_Code]));
                Ptr += strlen(QAT_Result_Str[Ret_Code]);
                memcpy(Ptr, "\r\n", 2);
                Ptr += 2;
                Tx_Queue->Len += (strlen(QAT_Result_Str[Ret_Code]) + 2);
            }
        }
        /* queue buffer, then signal */
        qurt_mutex_lock(&HTC_Context.mutex);
        if (!HTC_Context.Tx_Queue) {
            HTC_Context.Tx_Queue = Tx_Queue;
        } else {
            Tx_Queue_Head = HTC_Context.Tx_Queue;
            while (Tx_Queue_Head && Tx_Queue_Head->Next) {
                Tx_Queue_Head = Tx_Queue_Head->Next;
            }
            Tx_Queue_Head->Next = Tx_Queue;
        }
        qurt_mutex_unlock(&HTC_Context.mutex);
        qurt_signal_set(&qat_task_start, QAT_EVENT_TXQ);
    }
    return QAT_STATUS_SUCCESS_E;
}

static void QAT_TxTasks(void *arg)
{
    QAT_Tx_Queue_t *Next = NULL;
    uint32_t signal;
    uint16_t chunkSize = 0;
    uint16_t offset = 0;

    while (1) {
        signal = qurt_signal_wait(&qat_task_start, QAT_EVENT_TXQ, QURT_SIGNAL_ATTR_CLEAR_MASK);
        qurt_mutex_lock(&HTC_Context.mutex);
        Next = HTC_Context.Tx_Queue;

        while (Next) {
            while (Next->Len > offset) {
                qurt_thread_sleep(1);
                chunkSize = (Next->Len - offset > QAT_OUTPUT_MAX_LENGTH) ? QAT_OUTPUT_MAX_LENGTH : (Next->Len - offset);
                QAT_Output(chunkSize, (char *)(Next->Buffer + offset));
                offset += chunkSize;
            }
            // QAT_Output(Next->Len, (char*)Next->Buffer);

            HTC_Context.Tx_Queue = Next->Next;

            free((uint8_t *)Next);

            Next = HTC_Context.Tx_Queue;

            offset = 0;
            chunkSize = 0;
        }

        qurt_mutex_unlock(&HTC_Context.mutex);
    }
}

qbool_t QAT_TxTask_Initialize(void)
{
    uint32_t ret_val;

    /* Initialize the context information. */
    memset(&HTC_Context, 0, sizeof(HTC_Context));
    qurt_mutex_create(&HTC_Context.mutex);

    ret_val = (uint32_t)nt_qurt_thread_create(QAT_TxTasks, "qat_tx_task", 4096, NULL, 6, &qat_tx_task_hdl);
    if (ret_val != pdPASS) {
        printf("QAT: task creation failed out of memory\r\n");
        ASSERT(0);
    }

    ret_val = qurt_signal_create(&qat_task_start);

    if (ret_val != 0) {
        printf("failed to create qat_task_start signal", 0);
        nt_osal_thread_delete(qat_tx_task_hdl);
        ASSERT(0);
    }
    return true;
}
void QAT_RxTasks()
{
#if (CONFIG_QAT_TRANSMISSION_MODULE == 0)
    UartRxTasks();
#else if (CONFIG_QAT_TRANSMISSION_MODULE == 1)
    SPIRxTasks();
#endif
}
qbool_t QAT_RxTask_Start()
{
    qbool_t Ret_Val = true;
    uint32_t ret_val;

    ret_val = (uint32_t)nt_qurt_thread_create(QAT_RxTasks, "qat_rx_task", 8192, NULL, 6, &qat_rx_task_hdl);
    if (ret_val != pdPASS) {
        printf("QAT: task creation failed out of memory\r\n");
        ASSERT(0);
    }

    return Ret_Val;
}

qbool_t QAT_RxTask_Initialize(void)
{
    uint32_t ret_val;

#if (CONFIG_QAT_TRANSMISSION_MODULE == 0)
    Uart_Initialize(UART_HTC_INSTANCE);
#else if (CONFIG_QAT_TRANSMISSION_MODULE == 1)
    SPI_Initialize();
#endif

    QAT_RxTask_Start();

    return true;
}

void Initialize_QAT_Main(void)
{
    Process_Input_Data_Handle = QAT_Process_Input_Data;

    QAT_TxTask_Initialize();

    QAT_RxTask_Initialize();
}

void qat_module_init(void)
{
    printf("QAT module init\r\n");
    Initialize_QAT_Main();
}
