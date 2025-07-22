/*
Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
*/
#include "fwconfig_cmn.h"
#include "nt_flags.h"

#ifndef _NT_GPIO_API_H_
#define _NT_GPIO_API_H_

#include <stdint.h>

#define __IO volatile

typedef struct gpio_reg {
    __IO uint32_t GPIO_SWPORT_DR;   // gpio data reg offset addr 0x00
    __IO uint32_t GPIO_SWPORT_DDR;  // gpio data direction offset addr
    __IO uint32_t GPIO_SWPORT_CTL;  // gpio data source reg offset address

} gpio_register_t;

#define NT_GPIO_BASE 0x01233a00

#define NT_GPIOA_BASE (NT_GPIO_BASE + 0x00000000)
#define NT_GPIOB_BASE (NT_GPIO_BASE + 0x0000000C)
#define NT_GPIOC_BASE (NT_GPIO_BASE + 0x00000018)
#define NT_GPIOD_BASE (NT_GPIO_BASE + 0x00000024)

#define NT_GPIOA ((gpio_register_t *)NT_GPIOA_BASE)
#define NT_GPIOB ((gpio_register_t *)NT_GPIOB_BASE)
#define NT_GPIOC ((gpio_register_t *)NT_GPIOC_BASE)
#define NT_GPIOD ((gpio_register_t *)NT_GPIOD_BASE)

#define GPIO_SWPORTA_DR  (NT_GPIOA_BASE)
#define GPIO_SWPORTA_DDR (NT_GPIOA_BASE + 0x00000004)
#define GPIO_SWPORTA_CTL (NT_GPIOA_BASE + 0x00000008)

#define GPIO_SWPORTB_DR  (NT_GPIOB_BASE)
#define GPIO_SWPORTB_DDR (NT_GPIOB_BASE + 0x10)
#define GPIO_SWPORTB_CTL (NT_GPIOB_BASE + 0x14)

#define GPIO_SWPORTC_DR  (NT_GPIOC_BASE)
#define GPIO_SWPORTC_DDR (NT_GPIOC_BASE + 1C)
#define GPIO_SWPORTC_CTL (NT_GPIOC_BASE + 20)

#define GPIO_SWPORTD_DR  (NT_GPIOD_BASE)
#define GPIO_SWPORTD_DDR (NT_GPIOD_BASE + 28)
#define GPIO_SWPORTD_CTL (NT_GPIOD_BASE + 2C)

#define GPIO_PIN_0  ((uint16_t)0x1)     // pin 0
#define GPIO_PIN_1  ((uint16_t)0x2)     // pin 1
#define GPIO_PIN_2  ((uint16_t)0x4)     // pin 2
#define GPIO_PIN_3  ((uint16_t)0x8)     // pin 3
#define GPIO_PIN_4  ((uint16_t)0x10)    // pin 4
#define GPIO_PIN_5  ((uint16_t)0x20)    // pin 5
#define GPIO_PIN_6  ((uint16_t)0x40)    // pin 6
#define GPIO_PIN_7  ((uint16_t)0x80)    // pin 7
#define GPIO_PIN_8  ((uint16_t)0x100)   // pin 8
#define GPIO_PIN_9  ((uint16_t)0x200)   // pin 9
#define GPIO_PIN_10 ((uint16_t)0x400)   // pin 10
#define GPIO_PIN_11 ((uint16_t)0x800)   // pin 11
#define GPIO_PIN_12 ((uint16_t)0x1000)  // pin 12
#define GPIO_PIN_13 ((uint16_t)0x2000)  // pin 13
#define GPIO_PIN_14 ((uint16_t)0x4000)  // pin 14
// pin mask
#define GPIO_PIN_MASK (0x0000FFFFU)

#define QWLAN_GPIO_GPIO_CLEAR_INT_REG 0x01233a4c

#define GPIO_OUTPUT_TYPE (0x00000010)

#define NT_NVIC_ISER1   0xE000E104  // Irq 32 to 60 set Enable Register
#define NT_GPIO_INT_PIN 0x10

#define NT_ACTIVE_HIGH 0x1
#define NT_ACTIVE_LOW  0x0

#define NT_EDGE_SENSITIVE  0x1
#define NT_LEVEL_SENSITIVE 0x0

#define GPIO_OUTPUT (0x01)
#define GPIO_INPUT  (0x00)

#define NT_GPIO_HIGH (0x01)
#define NT_GPIO_LOW  (0x00)

typedef struct {
    uint32_t Pin;
    uint32_t Mode;

} gpio_init_typedef;

typedef enum {
    LOW = 0,
    HIGH

} GPIO_PinState;

typedef struct {
    uint32_t saved;
    uint32_t ds;
    uint32_t pu;
    uint32_t pd;

    uint32_t ls_sync;
    uint32_t dr;
    uint32_t ddr;
    uint32_t int_level;
    uint32_t int_polar;
    uint32_t int_en;
} GPIO_Config_t;

/**
 * @Function: nt_gpio_init
 * @Description: root clock enabled in Init API.
 * @parm:       NULL
 * @Return :    NULL
 */
void nt_gpio_init(void);

/**
 * @Function: nt_gpio_pin_mode
 * @Description: either INPUT or OUTPUT mode declared by this API.
 * @parm:       GPIOx - port type.
 * 				gpio_init - pin number and mode
 * @Return :    NULL
 */
void nt_gpio_pin_mode(gpio_register_t *GPIOx, uint32_t Pin, uint32_t Mode);

/**
 * @Function: nt_gpio_pin_write
 * @Description: either low or high mode declared by this API.
 * @parm:       GPIOx - port type.
 * 				pin - pin number.
 * 				val - HIGH/LOW
 * @Return :    NULL
 */

void nt_gpio_pin_write(gpio_register_t *GPIOx, uint32_t pin, GPIO_PinState val);

/**
 * @Function: nt_gpio_pin_read
 * @Description: status is High /Low  declared by this API.
 * @parm:       GPIOx - port type.
 *
 * @Return :    uint32_t
 */

uint32_t nt_gpio_pin_read_level(gpio_register_t *GPIOx);
/**
 * @Function: nt_gpio_init_enable_clock
 * @Description: clock enabled mode declared by this API.
 * @parm:       gpiox - pin number.
 *
 * @Return :    NULL
 */

void nt_gpio_init_enable_root_clock(void);
/**
 * @Function: nt_gpio_init_disable_clock
 * @Description: clock disable mode declared by this API.
 * @parm:       gpiox - pin number.
 *
 * @Return :    NULL
 */
void nt_gpio_init_disable_root_clock(void);
/**
 * @Function: nt_gpio_preset
 * @Description: gpio module reset API.
 * @parm:       NULL
 * @Return :    NULL
 */
void nt_gpio_preset(void);

/**
 * @Function: nt_gpio_interrupt_config
 * @Description: either normal or interrupt mode declared by this API.
 * @parm:       GPIOx - port type.
 * 				pin - pin number.
 *				sensitive_status - level / edge
 *				active_status    - high/low
 * @Return :    NULL
 */

void nt_gpio_interrupt_config(gpio_register_t *GPIOx, uint32_t Pin, uint8_t sensitive_status, uint8_t active_status);

/**
 * @Function: nt_gpio_pin_read_mode
 * @Description: status is output mode /input mode declared by this API.
 * @parm:       GPIOx - port type.
 *
 * @Return :    uint32_t
 */

uint32_t nt_gpio_pin_read_mode(gpio_register_t *GPIOx);

/**
 * @Function: nt_gpio_pin_interrupt_enable
 * @Description: gpio pin interrupt service enable.
 * @parm:      NULL
 * @Return :    NULL
 */

void nt_gpio_pin_interrupt_enable(uint8_t Pin, uint8_t en);

/**
 * @Function: nt_gpio_interrupt_enable
 * @Description: interrupt service routine.
 * @parm:      NULL
 * @Return :    NULL
 */

void nt_gpio_interrupt_enable(void);
#endif  //_NT_GPIO_API_H_
