/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include "fwconfig_cmn.h"
#include "nt_flags.h"
#ifdef NT_GPIO_FLAG
#include "nt_gpio_api.h"
#include "nt_common.h"
#include "nt_hw.h"
#include "ferm_prof.h"

GPIO_Config_t gpio_config;

/**
 * @Function: nt_gpio_init
 * @Description: root clock enabled in Init API.
 * @parm:       NULL
 * @Return :    NULL
 */
void nt_gpio_init(void)
{
    // root clock enable
    nt_gpio_init_enable_root_clock();

    // reset the gpio config
    nt_gpio_preset();

#if defined(NT_HOSTED_SDK)
    nt_gpio_pin_mode(NT_GPIOA, GPIO_PIN_5, GPIO_OUTPUT);
    nt_gpio_pin_mode(NT_GPIOA, GPIO_PIN_4, GPIO_OUTPUT);
    nt_gpio_pin_write(NT_GPIOA, GPIO_PIN_4, NT_GPIO_HIGH);
    nt_gpio_pin_write(NT_GPIOA, GPIO_PIN_5, NT_GPIO_HIGH);
#endif
}

/**
 * @Function: nt_gpio_pin_mode
 * @Description: either INPUT or OUTPUT mode declared by this API.
 * @parm:       GPIOx - port type.
 * 				Pin,mode - pin number and input mode or output mode
 * @Return :    NULL
 */

void __attribute__((section(".__sect_ps_txt"))) nt_gpio_pin_mode(gpio_register_t *GPIOx, uint32_t Pin, uint32_t Mode)
{
    uint32_t value = 0;
    /* Based on port, configure the data direction port pins */
    value = GPIOx->GPIO_SWPORT_DDR;
    if (Mode == GPIO_OUTPUT) {
        value |= (Pin);
    } else if (Mode == GPIO_INPUT) {
        value &= (~(Pin));
    }
    GPIOx->GPIO_SWPORT_DDR = value;
}

/**
 * @Function: nt_gpio_pin_write
 * @Description: either low or high mode declared by this API.
 * @parm:       GPIOx - port type.
 * 				pin - pin number.
 * 				val - HIGH/LOW
 * @Return :    NULL
 */

void __attribute__((section(".__sect_ps_txt")))
nt_gpio_pin_write(gpio_register_t *GPIOx, uint32_t Pin, GPIO_PinState val)
{
    uint32_t value = 0;
    // Based on port, configure the data register pins
    value = GPIOx->GPIO_SWPORT_DR;
    if (NT_GPIO_HIGH == val) {
        value |= (Pin);
    } else if (NT_GPIO_LOW == val) {
        value &= (~(Pin));
    }
    GPIOx->GPIO_SWPORT_DR = value;
}
/**
 * @Function: nt_gpio_pin_read_level
 * @Description: status is High /Low  declared by this API.
 * @parm:       GPIOx - port type.
 *
 * @Return :   uint32_t
 */

uint32_t nt_gpio_pin_read_level(gpio_register_t *GPIOx)
{
    uint32_t value = 0;
    // value returns the data status
    value = GPIOx->GPIO_SWPORT_DR;

    return value;
}

/**
 * @Function: nt_gpio_pin_read_mode
 * @Description: status is output mode /input mode declared by this API.
 * @parm:       GPIOx - port type.
 *
 * @Return :    uint32_t
 */

uint32_t nt_gpio_pin_read_mode(gpio_register_t *GPIOx)
{
    uint32_t value = 0;
    // value return the data direction register status
    value = GPIOx->GPIO_SWPORT_DDR;

    return value;
}

/**
 * @Function: nt_gpio_init_enable_root_clock
 * @Description: clock enabled mode declared by this API.
 * @parm:       NULL
 *
 * @Return :    NULL
 */

void nt_gpio_init_enable_root_clock(void)
{
    uint32_t value;
    /* GPIO root clock enable */
    value = NT_REG_RD(QWLAN_PMU_ROOT_CLK_ENABLE_REG);
    value |= 1 << QWLAN_PMU_ROOT_CLK_ENABLE_GPIO_ROOT_CLK_ENABLE_OFFSET;
    NT_REG_WR(QWLAN_PMU_ROOT_CLK_ENABLE_REG, value);
}

/**
 * @Function: nt_gpio_init_disable_root_clock
 * @Description: clock disable mode declared by this API.
 * @parm:       NULL
 *
 * @Return :    NULL
 */

void nt_gpio_init_disable_root_clock(void)
{
    uint32_t value;
    /* GPIO root clock disable */
    value = NT_REG_RD(QWLAN_PMU_CLKGATE_DISABLE_REG);
    value |= 1 << QWLAN_PMU_CLKGATE_DISABLE_GPIO_CLKGATE_DISABLE_OFFSET;
    NT_REG_WR(QWLAN_PMU_CLKGATE_DISABLE_REG, value);
}

/**
 * @Function: nt_gpio_preset
 * @Description: gpio module reset API.
 * @parm:       NULL
 * @Return :    NULL
 */

void nt_gpio_preset(void)
{
    uint32_t value;
    // reset the all the configuration of Gpios
    value = NT_REG_RD(QWLAN_PMU_SOFT_RESET_REG);
    value |= (1 << QWLAN_PMU_SOFT_RESET_GPIO_SOFT_RESET_OFFSET);
    NT_REG_WR(QWLAN_PMU_SOFT_RESET_REG, value);
    value = NT_REG_RD(QWLAN_PMU_SOFT_RESET_REG);
    value &= (~(QWLAN_PMU_SOFT_RESET_GPIO_SOFT_RESET_MASK));
    NT_REG_WR(QWLAN_PMU_SOFT_RESET_REG, value);

#if !defined(NT_HOSTED_SDK)
    if (gpio_config.saved) {
        NT_REG_WR(QWLAN_PMU_CFG_IOPAD_DS_REG, gpio_config.ds);
        NT_REG_WR(QWLAN_PMU_CFG_IOPAD_PU_REG, gpio_config.pu);
        NT_REG_WR(QWLAN_PMU_CFG_IOPAD_PD_REG, gpio_config.pd);

        NT_REG_WR(QWLAN_GPIO_GPIO_LS_SYNC_REG, gpio_config.ls_sync);
        NT_REG_WR(QWLAN_GPIO_GPIO_SWPORTA_DR_REG, gpio_config.dr);
        NT_REG_WR(QWLAN_GPIO_GPIO_SWPORTA_DDR_REG, gpio_config.ddr);
        NT_REG_WR(QWLAN_GPIO_GPIO_INTTYPE_LEVEL_REG, gpio_config.int_level);
        NT_REG_WR(QWLAN_GPIO_GPIO_INR_POLARITY_REG, gpio_config.int_polar);
        NT_REG_WR(QWLAN_GPIO_GPIO_INTEN_REG, gpio_config.int_en);

        gpio_config.saved = 0;
    } else {
        /* - IOPAD configuration - */

#if 0
        /* UART: GPIO1 GPIO3*/
        uint32_t regval = NT_REG_RD(QWLAN_PMU_CFG_IOPAD_PU_REG); // pu
        NT_REG_WR(QWLAN_PMU_CFG_IOPAD_PU_REG, regval & 0xF9F80000); //0-18,25,26 : no pu

#if CONFIG_PBL_PREES_GPIO_RESTORE
        regval = NT_REG_RD(QWLAN_PMU_CFG_IOPAD_PD_REG);
        NT_REG_WR(QWLAN_PMU_CFG_IOPAD_PD_REG, ((regval & (~ 0x0002C000)) | 0x06053FFF) & (~0x5) ); //1,3,4-18,25,26 : pd, 0/2/14/15/17: no pd;
#endif	
        regval = NT_REG_RD(QWLAN_PMU_CFG_IOPAD_PU_REG); // pu
        NT_REG_WR(QWLAN_PMU_CFG_IOPAD_PU_REG, regval | 0x0002C000); //0-18,25,26 : no pu, 14/15/17: pd
#endif

        /* UART op3: GPIO1 GPIO3*/
        /* UART op1: GPIO13 GPIO14*/
        /* UART op0: GPIO11 GPIO12*/
        /* UART op2: GPIO9 GPIO10*/
        NT_REG_WR(QWLAN_PMU_CFG_IOPAD_PU_REG, CONFIG_BOARD_QCC730_GPIO_DEFAULT_PU_STATE);
        NT_REG_WR(QWLAN_PMU_CFG_IOPAD_PD_REG, CONFIG_BOARD_QCC730_GPIO_DEFAULT_PD_STATE);
#if CONFIG_AMBIENT_POWER_ENABLE
        /* fix power issue: Current is ~45uA when plug out UART&SPI in the VBAT=VDD=VDDIO=1.8v, while it is ~35uA when plug in UART&SPI */
        NT_REG_WR(QWLAN_PMU_CFG_IOPAD_PU_REG,  0xAB0A);
		NT_REG_WR(QWLAN_PMU_CFG_IOPAD_PD_REG,  0x1014F0);
#endif
    }

#ifndef FERMION_SILICON
    NT_REG_WR(QWLAN_PMU_CFG_IO_RET_CNTL_REG, QWLAN_PMU_CFG_IO_RET_CNTL_AON_IORET_CNTL_MASK);
#else
    NT_REG_WR(QWLAN_PMU_CFG_IO_RET_CNTL_REG, 0);  // Making retention 0, to enable GPIO toggling
#endif  // FERMION_SILICON
#endif
}
/**
 * @Function: nt_gpio_interrupt_config
 * @Description: interrupt mode declared by this API.
 * @parm:       GPIOx - port type.
 * 				pin - pin number.
 *				sensitive_status - level / edge
 *				active_status    - high/low
 * @Return :    NULL
 */

void nt_gpio_interrupt_config(gpio_register_t *GPIOx, uint32_t Pin, uint8_t sensitive_status, uint8_t active_status)
{
    uint32_t value = 0;
    /* Configure the porta pins */

    if (GPIOx == NT_GPIOA) {
        // system clock enable
        NT_REG_WR(QWLAN_GPIO_GPIO_LS_SYNC_REG, QWLAN_GPIO_GPIO_LS_SYNC_VALUE_MASK);
        if (sensitive_status == NT_LEVEL_SENSITIVE) {
            // reading  interrupt type register
            value = NT_REG_RD(QWLAN_GPIO_GPIO_INTTYPE_LEVEL_REG);
            value &= (~(Pin));
            // clear the interrupt type bit
            NT_REG_WR(QWLAN_GPIO_GPIO_INTTYPE_LEVEL_REG, value);
        } else if (sensitive_status == NT_EDGE_SENSITIVE) {
            // read the interrupt type register
            value = NT_REG_RD(QWLAN_GPIO_GPIO_INTTYPE_LEVEL_REG);
            value |= (Pin);
            // set the interrupt type bit
            NT_REG_WR(QWLAN_GPIO_GPIO_INTTYPE_LEVEL_REG, value);
        }
        if (active_status == NT_ACTIVE_HIGH) {
            // read the polarity register
            value = NT_REG_RD(QWLAN_GPIO_GPIO_INR_POLARITY_REG);
            value |= (Pin);
            // set the polarity bit
            NT_REG_WR(QWLAN_GPIO_GPIO_INR_POLARITY_REG, value);
        } else if (active_status == NT_ACTIVE_LOW) {
            // read the polarity regiser
            value = NT_REG_RD(QWLAN_GPIO_GPIO_INR_POLARITY_REG);
            value &= (~(Pin));
            // clear the polarity bit
            NT_REG_WR(QWLAN_GPIO_GPIO_INR_POLARITY_REG, value);
        }
        // enable the interrupt for GPIO
        value = NT_REG_RD(QWLAN_GPIO_GPIO_INTEN_REG);
        value |= (Pin);
        NT_REG_WR(QWLAN_GPIO_GPIO_INTEN_REG, value);
        // enable the interrupt from Interrupt service enable register
        value = NT_REG_RD(NT_NVIC_ISER1);
        value |= (1 << NT_GPIO_INT_PIN);
        NT_REG_WR(NT_NVIC_ISER1, value);
    }
}

/**
 * @Function: nt_gpio_pin_interrupt_enable
 * @Description: gpio pin interrupt service enable.
 * @parm:      NULL
 * @Return :    NULL
 */
void nt_gpio_pin_interrupt_enable(uint8_t Pin, uint8_t en)
{
    // PROF_IRQ_ENTER();
    uint32_t value = 0;
    // enable the interrupt for GPIO
    value = NT_REG_RD(QWLAN_GPIO_GPIO_INTEN_REG);
    if (en) {
        value |= (1 << Pin);
    } else {
        value &= ~(1 << Pin);
    }
    NT_REG_WR(QWLAN_GPIO_GPIO_INTEN_REG, value);

    // PROF_IRQ_EXIT();
}

void nt_gpio_isr_enable(uint8_t en)
{
    PROF_IRQ_ENTER();
    uint32_t value = 0;

    // enable the interrupt from Interrupt service enable register
    value = NT_REG_RD(NT_NVIC_ISER1);
    if (en) {
        value |= (1 << NT_GPIO_INT_PIN);
    } else {
        value &= ~(1 << NT_GPIO_INT_PIN);
    }
    NT_REG_WR(NT_NVIC_ISER1, value);
    PROF_IRQ_EXIT();
}

/**
 * @Function: nt_gpio_interrupt_enable
 * @Description: interrupt service routine.
 * @parm:      NULL
 * @Return :    NULL
 */

void nt_gpio_interrupt_enable(void)
{
    PROF_IRQ_ENTER();
    // interrupt service routine
    uint32_t value = 0;
    value = NT_REG_RD(QWLAN_GPIO_GPIO_INTSTATUS_REG);
    // clear edge type interrupts
    if (NT_REG_RD(QWLAN_GPIO_GPIO_INTTYPE_LEVEL_REG)) {
        NT_REG_WR(QWLAN_GPIO_GPIO_CLEAR_INT_REG, value);
    }
    PROF_IRQ_EXIT();
}

void GPIO_IntHandler(void) __attribute__((weak, alias("nt_gpio_interrupt_enable")));

#endif  // NT_GPIO_FLAG
