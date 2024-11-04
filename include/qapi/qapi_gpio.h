/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/

/** @file qapi_gpio.h
   @brief General Purpose Input/Output interface (GPIO)
*/

/** @addtogroup qapi_peripherals_gpio
  The GPIO module provides access to general-purpose input/output (GPIO)
  pins the value of which consist of one of two voltage settings (high or
  low) and the behavior of which can be programmed using software.

  Typical usage:
  - qapi_GPIO_Config()                   -- Config GPIO parameters, include
                                            function, direction, pull-type,
                                            and drive strength.
  - qapi_GPIO_Set()                      -- Set the GPIO output value.
  - qapi_GPIO_Get()                      -- Get the GPIO input value.
  - qapi_GPIO_Enable_Interrupt()         -- Register and enable GPIO input
                                            interrupt.
  - qapi_GPIO_Disable_Interrupt()        -- Unregister the GPIO interrupt
                                            function.
  - qapi_GPIO_Mux_Pin_Get()                 Get the Mux function pin name.
*/

#ifndef __QAPI_GPIO_H__
#define __QAPI_GPIO_H__

/*-------------------------------------------------------------------------
 * Include Files
 *-----------------------------------------------------------------------*/
#include "qapi_status.h"

/** @addtogroup qapi_peripherals_gpio
@{ */

/*-------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 *-----------------------------------------------------------------------*/

/*-------------------------------------------------------------------------
 * Type Declarations
 *-----------------------------------------------------------------------*/

/**
   @brief GPIO pin number.
*/
typedef enum
{ 
  QAPI_GPIO_ID0_E,    /**< GPIO 0. */
  QAPI_GPIO_ID1_E,    /**< GPIO 1. */
  QAPI_GPIO_ID2_E,    /**< GPIO 2. */
  QAPI_GPIO_ID3_E,    /**< GPIO 3. */
  QAPI_GPIO_ID4_E,    /**< GPIO 4. */
  QAPI_GPIO_ID5_E,    /**< GPIO 5. */
  QAPI_GPIO_ID6_E,    /**< GPIO 6. */
  QAPI_GPIO_ID7_E,    /**< GPIO 7. */
  QAPI_GPIO_ID8_E,    /**< GPIO 8. */
  QAPI_GPIO_ID9_E,    /**< GPIO 9. */
  QAPI_GPIO_ID10_E,   /**< GPIO 10. */
  QAPI_GPIO_ID11_E,   /**< GPIO 11. */
  QAPI_GPIO_ID12_E,   /**< GPIO 12. */
  QAPI_GPIO_ID13_E,   /**< GPIO 13. */
  QAPI_GPIO_ID14_E,   /**< GPIO 14. */
  QAPI_GPIO_MAX_ID_E  /**< Maximum GPIO. */
} qapi_GPIO_Id_t;

/**
   @brief GPIO Mux Func Name.
*/
typedef enum 
{
  MUX_FUNC_GPIO,
  MUX_FUNC_SPI_SLAVE,
  MUX_FUNC_UART,
  MUX_FUNC_I2C,
  MUX_FUNC_EXT_32K_INPUT,
  MUX_FUNC_BT,
  MUX_FUNC_BT_3W,
  MUX_FUNC_QSPI,
  
  MUX_FUNC_XPA_2G4,
  MUX_FUNC_XPA_5G,
  MUX_FUNC_FEM_2G4,
  MUX_FUNC_FEM_5G,
  
  MUX_FUNC_ANT_SWTCH,
  MUX_FUNC_MAX, 
  MUX_FUNC_MAX_ERROR = MUX_FUNC_MAX, 
}QAPI_GPIO_MUX_FUNC_t;

typedef enum 
{
  MUX_PIN_GPIO,
  
  MUX_PIN_SPI_SLAVE_CLK,
  MUX_PIN_SPI_SLAVE_CS,
  MUX_PIN_SPI_SLAVE_MOSI,
  MUX_PIN_SPI_SLAVE_MISO,
  
  MUX_PIN_UART_TX,
  MUX_PIN_UART_RX,
  
  MUX_PIN_I2C_SDA,
  MUX_PIN_I2C_SCL,
  
  MUX_PIN_EXT_32K_INPUT,
  
  MUX_PIN_BT_WSI_CLK_BT,
  MUX_PIN_BT_WSI_DATA_BT,
  
  MUX_PIN_BT_3W_BT_PRIORITY,
  MUX_PIN_BT_3W_BT_ACTIVE,
  MUX_PIN_BT_3W_WLAN_ACTIVE,
  
  MUX_PIN_QSPI_CLK,
  MUX_PIN_QSPI_CS,
  MUX_PIN_QSPI_MOSI_0,
  MUX_PIN_QSPI_MOSI_1,
  MUX_PIN_QSPI_MOSI_2,
  MUX_PIN_QSPI_MOSI_3,
  
  MUX_PIN_XPA_2G4,
  MUX_PIN_XPA_5G,
  MUX_PIN_FEM_2G4,
  MUX_PIN_FEM_5G,
  
  MUX_PIN_ANT_SWTCH,
  MUX_PIN_MAX, 
}QAPI_GPIO_MUX_PIN_t;

/**
   @brief GPIO alternative configuration.

   This structure is used to specify the alternative configurations of GPIOs
   for different hardware designs.
*/
typedef struct qapi_GPIO_Alt_Config_s
{
  uint16_t  PIOFunc : 4;    /**< PIO function select. */
  uint16_t  Dir     : 1;    /**< Direction (input or output). */
  uint16_t  Pull    : 2;    /**< Pull value. */
  uint16_t  Drive   : 3;    /**< Drive strength. */
  uint16_t  se_port : 6;    /**< PIO function select. */
} qapi_GPIO_Alt_Config_t;

/** 
   @brief GPIO pin direction. 
*/
typedef enum
{ 
  QAPI_GPIO_INPUT_E,    /**< Specify the PIO as an INPUT to the SoC. */
  QAPI_GPIO_OUTPUT_E    /**< Specify the PIO as an OUTPUT from the SoC. */
} qapi_GPIO_Direction_t;

/** 
   @brief GPIO pin pull type.
*/
typedef enum
{
  QAPI_GPIO_NO_PULL_E,      /**< Specify no pull. @codeinline{Input + NO PULL} is equal to High-Z state. */
  QAPI_GPIO_PULL_DOWN_E,    /**< Pull the GPIO down. */
  QAPI_GPIO_PULL_UP_E       /**< Pull the GPIO up. */
} qapi_GPIO_Pull_t;

/**
   @brief GPIO pin drive strength. 
*/
typedef enum
{
  QAPI_GPIO_DRIVE_LOW_E,     /**< Specify a fast 2&nbsp; mA drive. */
  QAPI_GPIO_DRIVE_HIGH_E,     /**< Specify a fast 4&nbsp;mA drive. */
}qapi_GPIO_Drive_t;

/** 
   @brief GPIO output state specification.
*/
typedef enum
{
  QAPI_GPIO_LOW_VALUE_E,     /**< Drive the output LOW. */
  QAPI_GPIO_HIGH_VALUE_E     /**< Drive the output HIGH. */
}qapi_GPIO_Value_t;

/** 
    @brief GPIO interrupt trigger type enumeration for supported triggers.
*/
typedef enum {
  QAPI_GPIO_TRIGGER_LEVEL_HIGH_E,    /**< Level triggered active high. */
  QAPI_GPIO_TRIGGER_LEVEL_LOW_E,     /**< Level triggered active low. */
  QAPI_GPIO_TRIGGER_EDGE_RISING_E,   /**< Rising-edge triggered. */
  QAPI_GPIO_TRIGGER_EDGE_FALLING_E,  /**< Falling-edge triggered. */
} qapi_GPIO_Trigger_t;

/**
   @brief GPIO configuration.

   This structure is used to specify the configuration for a GPIO on the SoC.
   The GPIO can be configured as an input or output that can be driven high or
   low by the software. The interface also allows the SoC PIOs to be configured
   for alternate functionality.
*/
typedef struct qapi_GPIO_Config_s
{
  //uint8_t               PIOFunc;    /**< PIO function select. */
  qapi_GPIO_Direction_t Dir;        /**< Direction (input or output). */
  qapi_GPIO_Pull_t      Pull;       /**< Pull value. */
  qapi_GPIO_Drive_t     Drive;      /**< Drive strength. */
} qapi_GPIO_Config_t;

/** 
   @brief GPIO interrupt callback data type.

   This is the data type of the argument passed into the callback that is
   registered with the GPIO interrupt module. The value to pass is given
   by the client at registration time.
*/
typedef uint32_t qapi_GPIO_CB_Data_t;

/** 
   @brief GPIO interrupt callback function definition.

   GPIO interrupt clients pass a function pointer of this format into the
   registration API.

   @param[in] Data Callback data.
*/
typedef void (*qapi_GPIO_CB_t)(qapi_GPIO_CB_Data_t Data);

/**
   @brief GPIO interrupt callback list structure.

   This structure is used to store the GPIO pin interrupt callback function
   and parameter in a linked list.
*/
typedef struct qapi_GPIO_CB_List_s
{
  qapi_GPIO_Id_t GPIO_ID;               /**< GPIO pin number. */
  qapi_GPIO_CB_t Func;                  /**< GPIO callback function. */
  qapi_GPIO_CB_Data_t Data;             /**< GPIO callback parameter. */
  struct qapi_GPIO_CB_List_s *Next;     /**< Pointer to the next GPIO in the linked list callback structure. */
} qapi_GPIO_CB_List_t;

/*-------------------------------------------------------------------------
 * Function Declarations
 *-----------------------------------------------------------------------*/

qapi_Status_t qapi_GPIO_Init(void);

/**
   @brief Changes the SoC PIO configuration.

   This function configures an SoC PIO based on a set of fields specified in
   the configuration structure reference passed in as a parameter.

   @param[in] GPIO_ID           GPIO number.
   @param[in] Config            PIO configuration to use.

   @return
   QAPI_OK -- On success.\n
   Error code -- On failure.
*/
qapi_Status_t qapi_GPIO_Config(qapi_GPIO_Id_t GPIO_ID, qapi_GPIO_Config_t *Config);

/**
   @brief Sets the state of an SoC PIO configured as an output GPIO.

   This function drives the output of an SoC PIO that has been configured as a
   generic output GPIO to a specified value.

   @param[in] GPIO_ID  GPIO number.
   @param[in] Value    Output value.

   @return
   QAPI_OK -- On success.\n
   Error code -- On failure.
*/
qapi_Status_t qapi_GPIO_Set(qapi_GPIO_Id_t GPIO_ID, qapi_GPIO_Value_t Value);

/**
   @brief Reads the state of an SoC PIO.

   @param[in]  GPIO_ID  GPIO number.
   @param[out] Value    Input value.

   @return
   QAPI_OK -- On success.\n
   Error code -- On failure.
*/
qapi_Status_t qapi_GPIO_Get(qapi_GPIO_Id_t GPIO_ID, qapi_GPIO_Value_t *Value);

/**
   @brief Registers a callback for a GPIO interrupt.

   Registers a callback function with the GPIO interrupt controller, and enables
   the interrupt. This function configures and routes the interrupt accordingly,
   as well as enabling it in the underlying layers.

   @param[in] GPIO_ID   GPIO number.
   @param[in] Trigger   Trigger type for the interrupt.
   @param[in] Callback  Callback function pointer.
   @param[in] Data      Callback data.

   @return
   QAPI_OK -- On success.\n
   Error code -- On failure.
*/
qapi_Status_t qapi_GPIO_Enable_Interrupt(qapi_GPIO_Id_t GPIO_ID, qapi_GPIO_Trigger_t Trigger, qapi_GPIO_CB_t Callback, qapi_GPIO_CB_Data_t Data);

/**
   @brief Deregisters a callback for a GPIO interrupt.

   Deregisters a callback function from the GPIO interrupt controller, and
   disables the interrupt. This function deconfigures the interrupt
   accordingly, and disables it in the underlying layers.

   @param[in] GPIO_ID  GPIO number.

   @return
   QAPI_OK -- On success.\n
   Error code -- On failure.
*/
qapi_Status_t qapi_GPIO_Disable_Interrupt(qapi_GPIO_Id_t GPIO_ID);


/**
   @brief retrive the GPIO MUX configuration.

   @param[in]   GPIO ID.

   @return return the GPIO Mux Pin defination or error type.
*/
QAPI_GPIO_MUX_PIN_t qapi_GPIO_Mux_Pin_Get(qapi_GPIO_Id_t GPIO_ID);

/** @} */ /* end_addtogroup qapi_peripherals_gpio */

#endif
