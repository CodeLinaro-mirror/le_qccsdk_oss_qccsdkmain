/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/


/*-------------------------------------------------------------------------
 * Include Files
 *-----------------------------------------------------------------------*/
#include "fwconfig_cmn.h"
#include "nt_flags.h"
#include <string.h>
#include <stdlib.h>
#include "qapi_gpio.h"
#include "nt_gpio_api.h"
#include "nt_common.h"
#include "nt_osal.h"
#include "nt_hw.h"
#include "ferm_prof.h"
#include "qurt_internal.h"
#include "qurt_mutex.h"
#include "qccx.h"
#include <stdio.h>
/*-------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 *-----------------------------------------------------------------------*/
//#define QAPI_GPIO_DBG
#ifdef QAPI_GPIO_DBG
extern void nt_dbg_print(char* Printstring);
static char QAPI_GPIO_OutputBuffer [120];
#define QAPI_GPIO_PRINTF(...)     snprintf(QAPI_GPIO_OutputBuffer, sizeof(QAPI_GPIO_OutputBuffer), __VA_ARGS__); \
                             nt_dbg_print(QAPI_GPIO_OutputBuffer);
#else
#define QAPI_GPIO_PRINTF(x, ...)
#endif


/*-------------------------------------------------------------------------
 * Type Declarations
 *-----------------------------------------------------------------------*/
typedef struct
{
    uint8_t tx_pin;
    uint8_t rx_pin;
}Uart_Pin_t;


/*-------------------------------------------------------------------------
 * Variables
 *-----------------------------------------------------------------------*/
 
uint16_t Mux_Func_2_Pin_Mask[MUX_FUNC_MAX] = 
{
    0,
    GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3,
    GPIO_PIN_1 | GPIO_PIN_3 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12,
    GPIO_PIN_9 | GPIO_PIN_10,
    GPIO_PIN_8,
    GPIO_PIN_13 | GPIO_PIN_14,
    GPIO_PIN_8  | GPIO_PIN_13 | GPIO_PIN_14,
    GPIO_PIN_4  | GPIO_PIN_5  | GPIO_PIN_6  | GPIO_PIN_7  | GPIO_PIN_11 | GPIO_PIN_12,
    GPIO_PIN_2  | GPIO_PIN_6,
    GPIO_PIN_0  | GPIO_PIN_7,
    GPIO_PIN_3  | GPIO_PIN_4,
    GPIO_PIN_1  | GPIO_PIN_5,
    GPIO_PIN_1  | GPIO_PIN_5,
};

uint8_t spi_slave_Pin[] = { QAPI_GPIO_ID0_E, QAPI_GPIO_ID1_E, QAPI_GPIO_ID2_E, QAPI_GPIO_ID3_E};	/* scl, cs, mosi, miso*/;
Uart_Pin_t uart_Pin[] = { {QAPI_GPIO_ID11_E, QAPI_GPIO_ID12_E}, 
                          {QAPI_GPIO_ID14_E, QAPI_GPIO_ID13_E}, 
                          {QAPI_GPIO_ID9_E,  QAPI_GPIO_ID10_E}, 
                          {QAPI_GPIO_ID3_E,  QAPI_GPIO_ID1_E}};
uint8_t i2c_pin[] =       {QAPI_GPIO_ID9_E,  QAPI_GPIO_ID10_E};		/* sda, scl*/
uint8_t bt_wsi_Pin[] =    {QAPI_GPIO_ID13_E, QAPI_GPIO_ID14_E};		/*clk, data	*/
uint8_t bt_3w_Pin[] =     {QAPI_GPIO_ID8_E,  QAPI_GPIO_ID13_E, QAPI_GPIO_ID14_E};		/* bt_pri, bt_wlanactive, _active */
uint8_t qspi_Pin[] =      {QAPI_GPIO_ID4_E,  QAPI_GPIO_ID5_E,  QAPI_GPIO_ID6_E,  QAPI_GPIO_ID7_E, 
                           QAPI_GPIO_ID11_E, QAPI_GPIO_ID12_E};		/* qspi_clk, qspi_cs, qspi_mosi0/1/2/3 */

uint8_t XPA_2G4_Pin[]=    {QAPI_GPIO_ID6_E, QAPI_GPIO_ID2_E};
uint8_t XPA_5G_Pin[]=     {QAPI_GPIO_ID7_E, QAPI_GPIO_ID0_E};
uint8_t FEM_2G4_Pin[]=    {QAPI_GPIO_ID4_E, QAPI_GPIO_ID3_E};
uint8_t FEM_5G_Pin[]=     {QAPI_GPIO_ID5_E, QAPI_GPIO_ID1_E};
uint8_t ANT_SWITCH_Pin[]= {QAPI_GPIO_ID5_E, QAPI_GPIO_ID1_E};

/**
   @brief Point of GPIO callback function structure.
*/
qapi_GPIO_CB_List_t *GPIO_CB_Function = NULL;

/*-------------------------------------------------------------------------
 * Function Declarations
 *-----------------------------------------------------------------------*/
extern void vPortEnterCritical( void );
extern void vPortExitCritical( void );

/**
   @brief Changes the SoC PIO configuration.

   This Function configures an SoC PIO based on a set of fields specified in
   the configuration structure reference passed in as a parameter.

   @param[in] GPIO_ID           GPIO number.
   @param[in] qapi_GPIO_Config  PIO configuration to use.

   @return QAPI_OK on success or an error code on failure.
*/
qapi_Status_t qapi_GPIO_Config(qapi_GPIO_Id_t GPIO_ID, qapi_GPIO_Config_t *qapi_GPIO_Config)
{
    uint32_t pull_pu, pull_pd, ds;

    if ((GPIO_ID > QAPI_GPIO_ID14_E) || (qapi_GPIO_Config == NULL))
    {
        return QAPI_ERR_INVALID_PARAM;
    }

    if ((qapi_GPIO_Config->Pull > QAPI_GPIO_PULL_UP_E) || (qapi_GPIO_Config->Drive > QAPI_GPIO_DRIVE_HIGH_E) || 
        (qapi_GPIO_Config->Dir > QAPI_GPIO_OUTPUT_E))
    { 
        return QAPI_ERR_INVALID_PARAM;
    }

    vPortEnterCritical();

    nt_gpio_pin_mode(NT_GPIOA, (1 << GPIO_ID), qapi_GPIO_Config->Dir);

    ds      = NT_REG_RD(QWLAN_PMU_CFG_IOPAD_DS_REG); // ds
    pull_pu = NT_REG_RD(QWLAN_PMU_CFG_IOPAD_PU_REG); // pu
    pull_pd = NT_REG_RD(QWLAN_PMU_CFG_IOPAD_PD_REG); // pd
    
    pull_pu &= ~(1<<GPIO_ID);
    pull_pd &= ~(1<<GPIO_ID);

    if (qapi_GPIO_Config->Pull == QAPI_GPIO_PULL_DOWN_E)
    {
        pull_pd |= (1<<GPIO_ID);
    }
	else if (qapi_GPIO_Config->Pull == QAPI_GPIO_PULL_UP_E)
    {
        pull_pu |= (1<<GPIO_ID);
    }

    NT_REG_WR(QWLAN_PMU_CFG_IOPAD_PU_REG, pull_pu);
    NT_REG_WR(QWLAN_PMU_CFG_IOPAD_PD_REG, pull_pd);
    
    if (qapi_GPIO_Config->Drive == QAPI_GPIO_DRIVE_HIGH_E)
    {
        NT_REG_WR(QWLAN_PMU_CFG_IOPAD_DS_REG, (ds|(1<<GPIO_ID)));
    }
    else
    {
        ds &= ~(1<<GPIO_ID);
        NT_REG_WR(QWLAN_PMU_CFG_IOPAD_DS_REG, ds);
    }
    vPortExitCritical();

    return QAPI_OK;
}

/**
   @brief Sets the state of an SoC PIO configured as an output GPIO.

   This Function Drives the output of an SoC PIO that has been configured as a
   generic output GPIO to a specified value.

   @param[in] GPIO_ID   GPIO number.
   @param[in] Value     Output value.

   @return QAPI_OK on success or an error code on failure.
*/
qapi_Status_t qapi_GPIO_Set(qapi_GPIO_Id_t GPIO_ID, qapi_GPIO_Value_t Value)
{
    if ((GPIO_ID > QAPI_GPIO_ID14_E) || (Value > QAPI_GPIO_HIGH_VALUE_E))
    {
        return QAPI_ERR_INVALID_PARAM;
    }

    vPortEnterCritical();
    nt_gpio_pin_write(NT_GPIOA, (1 << GPIO_ID), Value);
    vPortExitCritical();

  return (QAPI_OK);
}

/**
   @brief Reads the state of an SoC PIO.

   @param[in]  GPIO_ID  GPIO number.
   @param[out] Value    Input value.

   @return QAPI_OK on success or an error code on failure.
*/
qapi_Status_t qapi_GPIO_Get(qapi_GPIO_Id_t GPIO_ID, qapi_GPIO_Value_t *Value)
{
    if ((GPIO_ID > QAPI_GPIO_ID14_E) || (Value == NULL))
    {
        return QAPI_ERR_INVALID_PARAM;
    }

    *Value = (NT_REG_RD(QWLAN_GPIO_GPIO_EXT_PORTA_REG) >> GPIO_ID) & 0x01;

    return (QAPI_OK);
}

/**
   @brief Registers a callback for a GPIO interrupt.

   Registers a callback Function with the GPIO interrupt controller and enables
   the interrupt. This Function configures and routes the interrupt accordingly,
   as well as enabling it in the underlying layers.

   @param[in] GPIO_ID   GPIO number.
   @param[in] Trigger   Trigger type for the interrupt.
   @param[in] Callback  Callback Function pointer.
   @param[in] Data      Callback data.

   @return QAPI_OK on success or an error code on failure.
*/
qapi_Status_t qapi_GPIO_Enable_Interrupt(qapi_GPIO_Id_t GPIO_ID, qapi_GPIO_Trigger_t Trigger, qapi_GPIO_CB_t Callback, qapi_GPIO_CB_Data_t Data)
{
    //uint32_t  Value;
    qbool_t   Found;
    qapi_GPIO_CB_List_t *Point;
    qapi_GPIO_CB_List_t *CB_Function;

    if ((GPIO_ID > QAPI_GPIO_ID14_E) || (Trigger > QAPI_GPIO_TRIGGER_EDGE_FALLING_E))
    {
        return QAPI_ERR_INVALID_PARAM;
    }

    vPortEnterCritical();

    Found = 0;
    Point = GPIO_CB_Function;
    while (Point != NULL)
    {
        if (Point->GPIO_ID == GPIO_ID)
        {
            Point->Func = Callback;
            Point->Data = Data;
            Found = 1;
            break;
        }

        Point = Point->Next;
    }

    if (Found == 0)
    {
        CB_Function = (qapi_GPIO_CB_List_t *)malloc(sizeof(qapi_GPIO_CB_List_t));
        if (CB_Function == NULL)
        {
            return QAPI_ERR_NO_MEMORY;
        }
        CB_Function->GPIO_ID = GPIO_ID;
        CB_Function->Func = Callback;
        CB_Function->Data = Data;
        CB_Function->Next = GPIO_CB_Function;
        GPIO_CB_Function = CB_Function;
    }

    if ((Trigger == QAPI_GPIO_TRIGGER_LEVEL_HIGH_E) || (Trigger == QAPI_GPIO_TRIGGER_EDGE_RISING_E))
    {
        nt_gpio_interrupt_config(NT_GPIOA, (1 << GPIO_ID), ((Trigger>>1) & 0x1), NT_ACTIVE_HIGH);
    }
    else
    {
        nt_gpio_interrupt_config(NT_GPIOA, (1 << GPIO_ID), ((Trigger>>1) & 0x1), NT_ACTIVE_LOW);	
    }
    
    vPortExitCritical();

    return QAPI_OK;
}

/**
   @brief Deregisters a callback for a GPIO interrupt.

   Deregisters a callback Function from the GPIO interrupt controller and
   disables the interrupt. This Function deconfigures the interrupt
   accordingly, as well as disabling it in the underlying layers.

   @param[in] GPIO_ID  GPIO number.

   @return QAPI_OK on success or an error code on failure.
*/
qapi_Status_t qapi_GPIO_Disable_Interrupt(qapi_GPIO_Id_t GPIO_ID)
{
    //uint32_t  Value;

    if (GPIO_ID > QAPI_GPIO_ID14_E)
    {
        return QAPI_ERR_INVALID_PARAM;
    }

    
    vPortEnterCritical();
    nt_gpio_pin_interrupt_enable(GPIO_ID, false);

    qapi_GPIO_CB_List_t  *Point;
    qapi_GPIO_CB_List_t  *Point_Prev;
    qapi_GPIO_CB_List_t  *Point_Next;

    if (GPIO_CB_Function == NULL)
    {
        
        vPortExitCritical();
        return QAPI_ERROR;
    }
    else
    {
        if (GPIO_CB_Function->GPIO_ID == GPIO_ID)
        {
            Point = GPIO_CB_Function->Next;
            free(GPIO_CB_Function);
            GPIO_CB_Function = Point;

            vPortExitCritical();
            return QAPI_OK;
        }
        Point_Prev = GPIO_CB_Function;
        Point = GPIO_CB_Function->Next;
    }

    while (Point != NULL)
    {
        Point_Next = Point->Next;

        if (Point->GPIO_ID == GPIO_ID)
        {
            free(Point);
            Point_Prev->Next = Point_Next;

            vPortExitCritical();;
            return QAPI_OK;
        }

        Point_Prev = Point;
        Point = Point_Next;
    }

    vPortExitCritical();
    return QAPI_ERROR;
}

/**
   @brief GPIO interrupt handler.
*/
void GPIO_IntHandler(void)
{
    uint8_t GPIO_ID;
    uint32_t Sum_Reg;// Stat_Reg, level_reg;
    qapi_GPIO_CB_t Callback;
    qapi_GPIO_CB_Data_t Data;
    qapi_GPIO_CB_List_t *Point;

    PROF_IRQ_ENTER();	
    Sum_Reg = NT_REG_RD(QWLAN_GPIO_GPIO_INTSTATUS_REG);
    
    //clear edge type interrupts
    NT_REG_WR(QWLAN_GPIO_GPIO_CLEAR_INT_REG,Sum_Reg);

    if (Sum_Reg)
    {

        for (GPIO_ID=0; GPIO_ID<QAPI_GPIO_MAX_ID_E; GPIO_ID++)
        {
            if (Sum_Reg & (0x01<<GPIO_ID))
            {
                Point = GPIO_CB_Function;
                while (Point != NULL)
                {
                    if (Point->GPIO_ID == GPIO_ID)
                    {
                        Callback = Point->Func;
                        Data = Point->Data;
                        Callback(Data);
                        break;
                    }

                    Point = Point->Next;
                }
            }
        }
    }

	PROF_IRQ_EXIT();	
}

/**
   @brief retrive the GPIO MUX configuration.

   @param[in]   GPIO ID.

   @return return the GPIO Mux Pin defination or error type.
*/
QAPI_GPIO_MUX_PIN_t qapi_GPIO_Mux_Pin_Get(qapi_GPIO_Id_t GPIO_ID)
{
    //uint32_t config_val;
    QAPI_GPIO_MUX_FUNC_t index=MUX_FUNC_SPI_SLAVE;
    uint16_t gpio_bit;
    Uart_Pin_t uart_pin;
    char pin_array_index;
    char pin_array_index_max;

    if(GPIO_ID >= QAPI_GPIO_MAX_ID_E)
        return MUX_FUNC_MAX_ERROR;

#if CONFIG_SOC_QCC730V1
    PMU_BASE_pmu_Type *pmu = (PMU_BASE_pmu_Type*)(QCC730V1_PMU_BASE_BASE);
#elif CONFIG_SOC_QCC730V2
    PMU_BASE_pmu_Type *pmu = (PMU_BASE_pmu_Type*)(QCC730V2_PMU_BASE_BASE);
#endif
    //config_val = pmu->PMU_BOOT_STRAP_CONFIGURATION_STATUS.reg;

    gpio_bit = 1<< GPIO_ID;
    QAPI_GPIO_PRINTF("strap %08x\r\n", (unsigned int)pmu->PMU_BOOT_STRAP_CONFIGURATION_STATUS.reg);

    while(index < MUX_FUNC_MAX_ERROR)
    {
        /* first check the func table */
        if(gpio_bit & Mux_Func_2_Pin_Mask[index]) 
        {
            QAPI_GPIO_PRINTF("bit %08x, mask %08x\r\n", gpio_bit, Mux_Func_2_Pin_Mask[index]);

            /* search the fucn from the configuration register */
            switch (index)
            {
                case MUX_FUNC_SPI_SLAVE:
                    if(pmu->PMU_BOOT_STRAP_CONFIGURATION_STATUS.bit.CFG_SPI_ENABLE)
                    {
                        return MUX_PIN_SPI_SLAVE_CLK + GPIO_ID;
                    }
                    break;
                    
                    case MUX_FUNC_UART:
                    if(pmu->PMU_BOOT_STRAP_CONFIGURATION_STATUS.bit.CFG_UART_ENABLE)
                    {
                        uart_pin = uart_Pin[pmu->PMU_BOOT_STRAP_CONFIGURATION_STATUS.bit.CFG_UART_OPTION];
                    
                        if(uart_pin.tx_pin == GPIO_ID)
                            return MUX_PIN_UART_TX;
                    
                        if(uart_pin.rx_pin == GPIO_ID)
                            return MUX_PIN_UART_RX;
                    }
                    
                    break;
                
                case MUX_FUNC_I2C:
                    if(pmu->PMU_BOOT_STRAP_CONFIGURATION_STATUS.bit.CFG_I2C_ENABLE)
                    {
                        if(i2c_pin[0] == GPIO_ID)
                            return MUX_PIN_I2C_SDA;
                    
                        if(i2c_pin[1] == GPIO_ID)
                            return MUX_PIN_I2C_SCL;
                    }
                    break;
    
                 case MUX_FUNC_EXT_32K_INPUT:
                    if(pmu->PMU_BOOT_STRAP_CONFIGURATION_STATUS.bit.CFG_GPIO8_EXT_32768HZ_CLK)
                    {
                        return MUX_PIN_EXT_32K_INPUT;
                    }
                    break;
                
                case MUX_FUNC_BT:
                    if(pmu->PMU_BOOT_STRAP_CONFIGURATION_STATUS.bit.CFG_BT_COEXT_MODE_SELECT == 0x01)
                    {
                        if(bt_wsi_Pin[0] == GPIO_ID)
                            return MUX_PIN_BT_WSI_CLK_BT;
                        if(bt_wsi_Pin[1] == GPIO_ID)
                            return MUX_PIN_BT_WSI_DATA_BT;
                    }
                    break;
    
                case MUX_FUNC_BT_3W:
                    if((pmu->PMU_BOOT_STRAP_CONFIGURATION_STATUS.bit.CFG_BT_COEXT_MODE_SELECT & 0x2) == 0x2)
                    {
                        if(bt_3w_Pin[0] == GPIO_ID)
                            return MUX_PIN_BT_3W_BT_PRIORITY;
                    
                        if(bt_3w_Pin[1] == GPIO_ID)
                            return MUX_PIN_BT_3W_BT_ACTIVE;
                    
                        if(bt_3w_Pin[2] == GPIO_ID)
                            return MUX_PIN_BT_3W_WLAN_ACTIVE;
                    }
                    break;
                
                case MUX_FUNC_QSPI:
                    if(pmu->PMU_BOOT_STRAP_CONFIGURATION_STATUS.bit.CFG_QSPI_ENABLE)
                    {
                        pin_array_index_max = 6;
                    }
                    else
                    {
                        pin_array_index_max = 4;
                    }
                    
                    for (pin_array_index=0; pin_array_index < pin_array_index_max; pin_array_index++)
                    {
                        if(qspi_Pin[(int)pin_array_index] == GPIO_ID)
                            return MUX_PIN_QSPI_CLK + pin_array_index;
                    }
                    
                    break;
                
                
                case MUX_FUNC_XPA_2G4:
                    if(pmu->PMU_BOOT_STRAP_CONFIGURATION_STATUS.bit.CFG_XPA_2G4_EN)
                    {
                    	if(XPA_2G4_Pin[pmu->PMU_BOOT_STRAP_CONFIGURATION_STATUS.bit.CFG_XPA_2G4_OP] == GPIO_ID)
                    		return MUX_PIN_XPA_2G4;
                    }
                    break;
                
                case MUX_FUNC_XPA_5G:
                    if(pmu->PMU_BOOT_STRAP_CONFIGURATION_STATUS.bit.CFG_XPA_5G_EN)
                    {
                    	if(XPA_5G_Pin[pmu->PMU_BOOT_STRAP_CONFIGURATION_STATUS.bit.CFG_XPA_5G_OP] == GPIO_ID)
                    		return MUX_PIN_XPA_5G;
                    }
                    break;
                
                case MUX_FUNC_FEM_2G4:
                    if(pmu->PMU_BOOT_STRAP_CONFIGURATION_STATUS.bit.CFG_FEM_2G4_EN)
                    {
                    	if(FEM_2G4_Pin[pmu->PMU_BOOT_STRAP_CONFIGURATION_STATUS.bit.CFG_FEM_2G4_OP] == GPIO_ID)
                    		return MUX_PIN_XPA_2G4;
                    }
                    break;
                
                case MUX_FUNC_FEM_5G:
                    if(pmu->PMU_BOOT_STRAP_CONFIGURATION_STATUS.bit.CFG_FEM_5G_EN)
                    {
                        if(FEM_5G_Pin[pmu->PMU_BOOT_STRAP_CONFIGURATION_STATUS.bit.CFG_FEM_5G_OP] == GPIO_ID)
                            return MUX_PIN_XPA_2G4;
                    }
                    break;
                
                
                case MUX_FUNC_ANT_SWTCH:
                    if(pmu->PMU_BOOT_STRAP_CONFIGURATION_STATUS.bit.CFG_ANT_SWTCH_EN)
                    {
                        if(ANT_SWITCH_Pin[pmu->PMU_BOOT_STRAP_CONFIGURATION_STATUS.bit.CFG_ANT_SWTCH_OP] == GPIO_ID)
                            return MUX_PIN_ANT_SWTCH;
                    }
                    break;
                
                default :
                    break;
            }
            
        }

        index++;
    }

    //if(index >= MUX_FUNC_MAX)
    return MUX_FUNC_GPIO;

}



