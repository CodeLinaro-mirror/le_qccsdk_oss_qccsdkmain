/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*-------------------------------------------------------------------------
 * Include Files
 *-----------------------------------------------------------------------*/
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "qapi_types.h"
#include "qapi_status.h"
#include "qapi_gpio.h"
#include "qcli_api.h"
#include "qapi_console.h"
#include "timer.h"
#include "i2c_demo.h"
#include "uart.h"
#include "nt_osal.h"
#include "safeAPI.h"

/*-------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 *-----------------------------------------------------------------------*/
#define GPIO_DBG
#ifdef GPIO_DBG
static char GPIOOutputBuffer[120];
#define GPIO_PRINTF(...)                                               \
    snprintf(GPIOOutputBuffer, sizeof(GPIOOutputBuffer), __VA_ARGS__); \
    nt_dbg_print(GPIOOutputBuffer);
#else
#define GPIO_PRINTF(x, ...)
#endif

/*-------------------------------------------------------------------------
 * Type Declarations
 *-----------------------------------------------------------------------*/

/*-------------------------------------------------------------------------
 * Static & global Variable Declarations
 *-----------------------------------------------------------------------*/
char *GPIO_Mux_Pin_Name[] = {
    "",  // MUX_PIN_GPIO,

    "spis_ck",  // MUX_PIN_SPI_SLAVE_CLK,
    "spis_cs",  // MUX_PIN_SPI_SLAVE_CS,
    "spis_si",  // MUX_PIN_SPI_SLAVE_MOSI,
    "spis_so",  // MUX_PIN_SPI_SLAVE_MISO,

    "uart_tx",  // MUX_PIN_UART_TX,
    "uart_rx",  // MUX_PIN_UART_RX,

    "i2c_sda",  // MUX_PIN_I2C_SDA,
    "i2c_scl",  // MUX_PIN_I2C_SCL,

    "ext_32k",  // MUX_PIN_EXT_32K_INPUT,

    "wsi_clk",   // MUX_PIN_BT_WSI_CLK_BT,
    "wsi_data",  // MUX_PIN_BT_WSI_DATA_BT,

    "3w_btpri",  // MUX_PIN_BT_3W_BT_PRIORITY,
    "3w_btact",  // MUX_PIN_BT_3W_BT_ACTIVE,
    "3w_wlact",  // MUX_PIN_BT_3W_WLAN_ACTIVE,

    "qspi_clk",  // MUX_PIN_QSPI_CLK,
    "qspi_cs",   // MUX_PIN_QSPI_CS,
    "qspi_d0",   // MUX_PIN_QSPI_MOSI_0,
    "qspi_d1",   // MUX_PIN_QSPI_MOSI_1,
    "qspi_d2",   // MUX_PIN_QSPI_MOSI_2,
    "qspi_d3",   // MUX_PIN_QSPI_MOSI_3,

    "xpa_2g4",  // MUX_PIN_XPA_2G4,
    "xpa_5g",   // MUX_PIN_XPA_5G,
    "fem_2g4",  // MUX_PIN_FEM_2G4,
    "fem_5g",   // MUX_PIN_FEM_5G,

    "ant_sw",  // MUX_PIN_ANT_SWTCH,
    "ntd",     // MUX_PIN_MAX,
};

/**
   Handle for our QCLI Command Group.
*/
QCLI_Group_Handle_t QCLI_Gpio_Handle;

/*-------------------------------------------------------------------------
 * Function Declarations
 *-----------------------------------------------------------------------*/
static QCLI_Command_Status_t cmd_Gpio_Config(uint32_t Parameter_Count, QCLI_Parameter_t *Parameter_List);
static QCLI_Command_Status_t cmd_Gpio_Set(uint32_t Parameter_Count, QCLI_Parameter_t *Parameter_List);
static QCLI_Command_Status_t cmd_Gpio_Get(uint32_t Parameter_Count, QCLI_Parameter_t *Parameter_List);
static QCLI_Command_Status_t cmd_Gpio_Interrupt_Enable(uint32_t Parameter_Count, QCLI_Parameter_t *Parameter_List);
static QCLI_Command_Status_t cmd_Gpio_Interrupt_Disable(uint32_t Parameter_Count, QCLI_Parameter_t *Parameter_List);
static QCLI_Command_Status_t cmd_Gpio_Get_Mux_Pin(uint32_t __attribute__((__unused__)) Parameter_Count,
                                                  QCLI_Parameter_t *__attribute__((__unused__)) Parameter_List);

const QCLI_Command_t GPIO_CMD_List[] = {
    /* cmd_Function                        cmd_string                  usage_string description */
    {cmd_Gpio_Config, "Config", "[pin] [Dir] [Pull] [Drive]", "Config GPIO"},
    {cmd_Gpio_Set, "Set", "[pin] [value]", "Output GPIO value"},
    {cmd_Gpio_Get, "Get", "[pin]", "Get GPIO input value"},
    {cmd_Gpio_Interrupt_Enable, "EnableInterrupt", "[pin] [trigger]", "Enable GPIO interrupt"},
    {cmd_Gpio_Interrupt_Disable, "DisableInterrupt", "[pin]", "Disable GPIO interrupt"},
    {cmd_Gpio_Get_Mux_Pin, "GetMux", "", "Get GPIO Mux function"},
};

const QCLI_Command_Group_t GPIO_CMD_Group = {"GPIO", (sizeof(GPIO_CMD_List) / sizeof(GPIO_CMD_List[0])), GPIO_CMD_List};

/*-------------------------------------------------------------------------
 * Function Definitions
 *-----------------------------------------------------------------------*/
#if (CONFIG_GPIO_SHELL)
void gpio_shell_init(void)
{
    /* Attempt to reqister the Command Groups with the qcli framework.*/
    QCLI_Gpio_Handle = QCLI_Register_Command_Group(NULL, &GPIO_CMD_Group);
    if (QCLI_Gpio_Handle) {
        GPIO_PRINTF("GPIO Registered \n");
    }
}
#endif

/**
   @brief Changes the SoC pin configuration.

   @param[in] pin    Physical pin number.
   @param[in] Dir    Direction (input or output).
   @param[in] Pull   Pull value.
   @param[in] Drive  Drive strength.

   @return
    - 0 if the help was displayed correctly.
    - A positive value indicating the depth of the error if a paramter was
      invalid.
*/
static QCLI_Command_Status_t cmd_Gpio_Config(uint32_t Parameter_Count, QCLI_Parameter_t *Parameter_List)
{
    qapi_Status_t Status;
    qapi_GPIO_Id_t GPIO_ID;
    qapi_GPIO_Config_t config;

    if (Parameter_Count != 4 || !Parameter_List || !Parameter_List[0].Integer_Is_Valid ||
        !Parameter_List[1].Integer_Is_Valid || !Parameter_List[2].Integer_Is_Valid ||
        !Parameter_List[3].Integer_Is_Valid) {
        return QCLI_STATUS_USAGE_E;
    }

    GPIO_ID = (qapi_GPIO_Id_t)Parameter_List[0].Integer_Value;
    config.Dir = (qapi_GPIO_Direction_t)Parameter_List[1].Integer_Value;
    config.Pull = (qapi_GPIO_Pull_t)Parameter_List[2].Integer_Value;
    config.Drive = (qapi_GPIO_Drive_t)Parameter_List[3].Integer_Value;

    Status = qapi_GPIO_Config(GPIO_ID, &config);
    if (Status != QAPI_OK) {
        GPIO_PRINTF("Config error.\n");
        return QCLI_STATUS_ERROR_E;
    }

    GPIO_PRINTF("Config success.\n");

    return QCLI_STATUS_SUCCESS_E;
}

/**
   @brief Sets the state of an SoC pin configured as an output GPIO.

   @param[in] pin    Physical pin number.
   @param[in] value  Output value.

   @return
    - 0 if the help was displayed correctly.
    - A positive value indicating the depth of the error if a paramter was
      invalid.
*/
static QCLI_Command_Status_t cmd_Gpio_Set(uint32_t Parameter_Count, QCLI_Parameter_t *Parameter_List)
{
    qapi_Status_t Status;
    qapi_GPIO_Id_t GPIO_ID;
    qapi_GPIO_Value_t value;

    if (Parameter_Count != 2 || !Parameter_List || !Parameter_List[0].Integer_Is_Valid ||
        !Parameter_List[1].Integer_Is_Valid) {
        return QCLI_STATUS_USAGE_E;
    }

    GPIO_ID = (qapi_GPIO_Id_t)Parameter_List[0].Integer_Value;
    value = (qapi_GPIO_Value_t)Parameter_List[1].Integer_Value;

    Status = qapi_GPIO_Set(GPIO_ID, value);
    if (Status != QAPI_OK) {
        GPIO_PRINTF("Set error.\n");
        return QCLI_STATUS_ERROR_E;
    }

    GPIO_PRINTF("Set success.\n");

    return QCLI_STATUS_SUCCESS_E;
}

/**
   @brief Reads the state of an SoC pin configured as an input GPIO.

   @param[in] pin    Physical pin number.

   @return
    - 0 if the help was displayed correctly.
    - A positive value indicating the depth of the error if a paramter was
      invalid.
*/
static QCLI_Command_Status_t cmd_Gpio_Get(uint32_t Parameter_Count, QCLI_Parameter_t *Parameter_List)
{
    qapi_Status_t Status;
    qapi_GPIO_Id_t GPIO_ID;
    qapi_GPIO_Value_t value;

    if (Parameter_Count != 1 || !Parameter_List || !Parameter_List[0].Integer_Is_Valid) {
        return QCLI_STATUS_USAGE_E;
    }

    GPIO_ID = (qapi_GPIO_Id_t)Parameter_List[0].Integer_Value;

    Status = qapi_GPIO_Get(GPIO_ID, &value);
    if (Status != QAPI_OK) {
        GPIO_PRINTF("Get error.\n");
        return QCLI_STATUS_ERROR_E;
    }

    GPIO_PRINTF("Get success. GPIO PIN: %d, Value Get %d.\n", GPIO_ID, value);

    return QCLI_STATUS_SUCCESS_E;
}

static void Gpio_Interrupt_Callback(qapi_GPIO_CB_Data_t data)
{
    GPIO_PRINTF("gpio %d: got inerrupt\n", (int)data);
}

/**
   @brief Enable interrupt on GPIO.

   @param[in] pin   Physical pin number.
   @param[in] trigger  Interrupt trigger type.

   @return
    - 0 if the help was displayed correctly.
    - A positive value indicating the depth of the error if a paramter was
      invalid.
*/
static QCLI_Command_Status_t cmd_Gpio_Interrupt_Enable(uint32_t Parameter_Count, QCLI_Parameter_t *Parameter_List)
{
    qapi_Status_t Status;
    qapi_GPIO_Id_t GPIO_ID;
    qapi_GPIO_Trigger_t Trigger;

    if (Parameter_Count != 2 || !Parameter_List || !Parameter_List[0].Integer_Is_Valid ||
        !Parameter_List[1].Integer_Is_Valid) {
        return QCLI_STATUS_USAGE_E;
    }

    GPIO_ID = (qapi_GPIO_Id_t)Parameter_List[0].Integer_Value;
    Trigger = (qapi_GPIO_Trigger_t)Parameter_List[1].Integer_Value;

    Status = qapi_GPIO_Enable_Interrupt(GPIO_ID, Trigger, Gpio_Interrupt_Callback, GPIO_ID);
    if (Status != QAPI_OK) {
        GPIO_PRINTF("EnableInterrupt error.\n");
        return QCLI_STATUS_ERROR_E;
    }

    GPIO_PRINTF("EnableInterrupt success.\n");

    return QCLI_STATUS_SUCCESS_E;
}

/**
   @brief Disable interrupt on GPIO.

   @param[in] pin  Physical pin number.

   @return
    - 0 if the help was displayed correctly.
    - A positive value indicating the depth of the error if a paramter was
      invalid.
*/
static QCLI_Command_Status_t cmd_Gpio_Interrupt_Disable(uint32_t Parameter_Count, QCLI_Parameter_t *Parameter_List)
{
    qapi_Status_t Status;
    qapi_GPIO_Id_t GPIO_ID;

    if (Parameter_Count != 1 || !Parameter_List || !Parameter_List[0].Integer_Is_Valid) {
        return QCLI_STATUS_USAGE_E;
    }

    GPIO_ID = (qapi_GPIO_Id_t)Parameter_List[0].Integer_Value;

    Status = qapi_GPIO_Disable_Interrupt(GPIO_ID);
    if (Status != QAPI_OK) {
        GPIO_PRINTF("DisableInterrupt error.\n");
        return QCLI_STATUS_ERROR_E;
    }

    GPIO_PRINTF("DisableInterrupt success.\n");

    return QCLI_STATUS_SUCCESS_E;
}

/**
   @brief Get GPIO Mux Pin Name.

   @return
    - 0 if the help was displayed correctly.
    - A positive value indicating the depth of the error if a paramter was
      invalid.
*/
static QCLI_Command_Status_t cmd_Gpio_Get_Mux_Pin(uint32_t __attribute__((__unused__)) Parameter_Count,
                                                  QCLI_Parameter_t __attribute__((__unused__)) * Parameter_List)
{
    qapi_GPIO_Id_t GPIO_ID;
    QAPI_GPIO_MUX_PIN_t mux_pin;

    GPIO_PRINTF("GPIO    NAME\r\n");
    for (GPIO_ID = QAPI_GPIO_ID0_E; GPIO_ID < QAPI_GPIO_MAX_ID_E; GPIO_ID++) {
        mux_pin = qapi_GPIO_Mux_Pin_Get(GPIO_ID);
        GPIO_PRINTF("%4d    %s\r\n", GPIO_ID, GPIO_Mux_Pin_Name[mux_pin]);
    }

    return QCLI_STATUS_SUCCESS_E;
}
