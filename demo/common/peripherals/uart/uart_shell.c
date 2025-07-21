/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "qapi_types.h"
#include "qapi_status.h"
#include "qapi_uart.h"
#include "qcli_api.h"
#include "qapi_console.h"
#include "nt_osal.h"
#include "uart.h"

#if (CONFIG_UART_SHELL)
#define UART_DEMO_DBG
#ifdef UART_DEMO_DBG
static char UARTOutputBuffer[80];
#define UART_PRINTF(...)                                               \
    snprintf(UARTOutputBuffer, sizeof(UARTOutputBuffer), __VA_ARGS__); \
    nt_dbg_print(UARTOutputBuffer);
#else
#define UART_PRINTF(x, ...)
#endif

#define BAUDRATE_NUM_MAX 6
/* Hardware issue limited the peformace of UART, sugget to apply the baudrate under 115200 */
static const uint32_t Baudrate_Support[BAUDRATE_NUM_MAX] = {
    115200, 57600, 38400, 19200, 9600, 4800,
};

uint8_t cmd_UART_Baudrate_Check(uint32_t baudrate)
{
    uint8_t num = 0, supported = 0;

    for (num = 0; num < BAUDRATE_NUM_MAX; num++) {
        if (Baudrate_Support[num] == baudrate) {
            supported = 1;
            break;
        }
    }

    return supported;
}

qapi_Status_t cmd_UART_Open(uint32_t __attribute__((__unused__)) Parameter_Count,
                            QAPI_Console_Parameter_t __attribute__((__unused__)) * Parameter_List)
{
    qapi_UART_Open_Config_t Config;
    qapi_UART_Instance_t Instance;
    qapi_Status_t Status;

    if ((Parameter_Count != 5) || (!Parameter_List) || (Parameter_List[0].Integer_Value > QAPI_UART_INST_0) ||
        (Parameter_List[2].Integer_Value > QAPI_UART_EVEN_PARITY_E) ||
        (Parameter_List[3].Integer_Value != QAPI_UART_8_BITS_PER_CHAR_E) ||
        (Parameter_List[4].Integer_Value > QAPI_UART_1_5_OR_2_0_STOP_BITS_E)) {
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }

    if (cmd_UART_Baudrate_Check(Parameter_List[1].Integer_Value) == 0) {
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }

    Instance = Parameter_List[0].Integer_Value;
    Config.baud_Rate = Parameter_List[1].Integer_Value;
    Config.parity_Mode = Parameter_List[2].Integer_Value;
    Config.bits_Per_Char = Parameter_List[3].Integer_Value;
    Config.num_Stop_Bits = Parameter_List[4].Integer_Value;
    Config.enable_Loopback = 0;

    Status = qapi_UART_Open(Instance, &Config);

    if (Status != QAPI_OK)
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_ERROR;

    return Status;
}

qapi_Status_t cmd_UART_Close(uint32_t __attribute__((__unused__)) Parameter_Count,
                             QAPI_Console_Parameter_t __attribute__((__unused__)) * Parameter_List)
{
    qapi_UART_Instance_t Instance;
    qapi_Status_t Status;

    if ((Parameter_Count != 1) || (!Parameter_List) || (Parameter_List[0].Integer_Value > QAPI_UART_INST_0)) {
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }

    Instance = Parameter_List[0].Integer_Value;

    Status = qapi_UART_Close(Instance);

    if (Status != QAPI_OK)
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_ERROR;

    return Status;
}

#define UART_TX_MAX_LEN 128
#define UART_TX_MAX_CYC 800
qapi_Status_t cmd_UART_Transmit(uint32_t __attribute__((__unused__)) Parameter_Count,
                                QAPI_Console_Parameter_t __attribute__((__unused__)) * Parameter_List)
{
    qapi_UART_Instance_t Instance;
    qapi_Status_t Status;
    char *Data;
    uint32_t DataLen, Sent = 0, Cycle;
    uint32_t Total = 0;

    if ((Parameter_Count != 4) || (!Parameter_List) || (Parameter_List[0].Integer_Value > QAPI_UART_INST_0) ||
        (Parameter_List[2].Integer_Value > UART_TX_MAX_LEN) || (Parameter_List[2].Integer_Value <= 0) ||
        (Parameter_List[3].Integer_Value > UART_TX_MAX_CYC) || (Parameter_List[3].Integer_Value <= 0))
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;

    Instance = Parameter_List[0].Integer_Value;

    Data = Parameter_List[1].String_Value;
    DataLen = Parameter_List[2].Integer_Value;
    Cycle = Parameter_List[3].Integer_Value;

    if (strlen(Data) != DataLen)
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;

    while (Cycle--) {
        Status = qapi_UART_Transmit(Instance, Data, DataLen, &Sent);

        Total += Sent;

        if (Status != QAPI_OK)
            break;
    }

    if (Total) {
        UART_PRINTF("UART Transmit Total Data Length %d.\r\n", (int)Total);
    }

    if (Status != QAPI_OK)
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_ERROR;

    return Status;
}

#define UART_RECV_MAX_LEN   UART_TX_MAX_LEN
#define UART_RECV_MAX_TOTAL (UART_TX_MAX_LEN * UART_TX_MAX_CYC)
extern int SEGGER_RTT_printf(unsigned BufferIndex, const char *sFormat, ...);
qapi_Status_t cmd_UART_Receive(uint32_t __attribute__((__unused__)) Parameter_Count,
                               QAPI_Console_Parameter_t __attribute__((__unused__)) * Parameter_List)
{
    qapi_UART_Instance_t Instance;
    qapi_Status_t Status = QAPI_OK;
    uint32_t DataLen, Recved = 0, i;
    // uint32_t Cycle = 0, Rmdr = 0, Total = 0;
    uint32_t Total = 0;
    char *Data;

    if ((Parameter_Count != 2) || (!Parameter_List) || (Parameter_List[0].Integer_Value > QAPI_UART_INST_0) ||
        (Parameter_List[1].Integer_Value > UART_RECV_MAX_TOTAL) || (Parameter_List[1].Integer_Value <= 0)) {
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_USAGE;
    }

    Instance = Parameter_List[0].Integer_Value;
    DataLen = Parameter_List[1].Integer_Value;

    Data = (char *)nt_osal_allocate_memory(UART_RECV_MAX_TOTAL);
    if (Data == NULL)
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_ERROR;

    Status = qapi_UART_Receive(Instance, Data, DataLen, &Recved);

    if (Recved > 0) {
        Total += Recved;

        for (i = 0; i < Recved; i++) {
            if ((i % UART_RECV_MAX_LEN) == 0) {
                UART_PRINTF("\r\n");
            }

            UART_PRINTF("%c", Data[i]);
        }
    }

    if (Total) {
        UART_PRINTF("\r\nUART Final Recv Data Total Length %u.\r\n", (unsigned int)Total);
    }

    nt_osal_free_memory(Data);

    if (Status != QAPI_OK)
        return QAPI_ERROR_CONSOLE_COMMAND_STATUS_ERROR;

    return Status;
}

const QAPI_Console_Command_t uart_shell_cmds[] = {
    // cmd_function			cmd_string       usage_string									description
    {cmd_UART_Open, "Open", "<Instance> <baudrate> <parity> <data_bits> <stop_bits>",
     "Open UART instance, baudrate [4800/9600/19200/38400/57600/115200], parity[0:no, 1:odd, 2:even], data_bits[3:8 "
     "bits], stop_bits[0:1 bits, 1: 2bits]"},
    {cmd_UART_Transmit, "Transmit", "<Instance> <Data> <DataLen> <Cycle>",
     "Transmit UART data, Max DataLen 128, Max cycle 800 "},
    {cmd_UART_Receive, "Receive", "<Instance> <Datalen>", "Receive UART data, Max DataLen 102400"},
    {cmd_UART_Close, "Close", "<Instance>", "Close UART instance"},
};

const QAPI_Console_Command_Group_t uart_shell_cmd_group = {
    "UART", sizeof(uart_shell_cmds) / sizeof(QAPI_Console_Command_t), uart_shell_cmds};

QAPI_Console_Group_Handle_t uart_shell_cmd_group_handle;

void uart_shell_init(void)
{
    uart_shell_cmd_group_handle = QAPI_Console_Register_Command_Group(NULL, &uart_shell_cmd_group);
}
#endif
