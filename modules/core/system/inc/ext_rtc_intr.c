/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include <nt_hw.h>
#include <hal_int_sys.h>
#include "hal_int_cfg.h"
#include "limits.h"

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"

// #include "nt_i2c.h"

/* Library includes. */

#include "nt_hw.h"
#include "hal_int_sys.h"

#include "nt_wlan.h"
#include "ferm_prof.h"
//#include <stddef.h>

void __attribute__((section(".after_ram_vectors"))) ext_irq_handler()
{
    PROF_IRQ_ENTER();
    // uint8_t stat_reg[3]={0xA4,0x0E,0x00};
    // I2C_Slave_Address(stat_reg);
    // I2C_Slave_Address(uint8_t *data);
    //	i2c_write();// Clock interrupt mask register address
    // i2c_write(0x44h);// clkie bit and aie bit
    PROF_IRQ_EXIT();
}
