/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
* This file will include all operations about storage like flash, RRAM, cMEM in SBL phase.
* Customer can add or build their own functions.
*/

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "boot_error_if.h"
#include "nt_bl_rram_dxe.h"
#include "nt_bl_uart.h"
#include "sbl_common.h"
#include "nt_bl_common.h"
#include "autoconf.h"

#if 0
/*
* SBL flash init function.
*/
bl_error_type boot_sbl_flash_init()
{
    bl_error_type status = (bl_error_type)FLASH_DEVICE_FAIL;

#if CONFIG_BOARD_QCC730_QSPI_ENABLE
    status = drv_flash_init();
#endif
    if (status != BL_ERR_NONE) {
        return status;
    }
    return status;
}

/*
* SBL flash read function.
*/
bl_error_type boot_sbl_flash_read(uint32_t address, uint32_t byte_cnt, uint8_t *buffer)
{
	bl_error_type status = (bl_error_type)FLASH_DEVICE_FAIL;
#if CONFIG_BOARD_QCC730_QSPI_ENABLE
	status = drv_flash_read(address, byte_cnt, buffer, NULL, NULL);
#endif
	return status;
}
/*
* SBL flash write function.
*/
bl_error_type boot_sbl_flash_write(uint32_t address, uint32_t byte_cnt, uint8_t *buffer)
{
	bl_error_type status = (bl_error_type)FLASH_DEVICE_FAIL;
#if CONFIG_BOARD_QCC730_QSPI_ENABLE
	status = drv_flash_write(address, byte_cnt, buffer, NULL, NULL);
#endif
	return status;
}
/*
* SBL flash XIP enable.
*/
bl_error_type boot_sbl_flash_xip_enable(uint32_t addr)
{
	int32_t value = 0;
    bl_error_type status = (bl_error_type)FLASH_DEVICE_FAIL;
	sbl_printf("xip config from flash add=0x%x\r\n", (unsigned int)addr);
#if CONFIG_BOARD_QCC730_QSPI_ENABLE	
    status = drv_qspi_xip_config(QSPI_XIP_FLASH_REGION_3, 0x1000/*16MB*/, addr/*FLASH_IMAGE_ADDRESS*/);
#endif
	value = HW_REG_RD(QWLAN_RRAM_CTRL_RRAM_CTRL_TEST_REG);   //enable all D-code read data access to be cached for dv purpose only.
	value |= ( 0x1 << QWLAN_RRAM_CTRL_RRAM_CTRL_TEST_SW_CACHE_TEST_MODE_OFFSET);
	HW_REG_WR(QWLAN_RRAM_CTRL_RRAM_CTRL_TEST_REG,value);
    return status;
}
bl_error_type boot_sbl_flash_xip_disable()
{
	int32_t value = 0;
	drv_qspi_disable_xip_mode();

	value = HW_REG_RD(QWLAN_RRAM_CTRL_RRAM_CTRL_TEST_REG);   
	//enable all D-code read data access to be cached for dv purpose only.
	value &= (~( 0x1 << QWLAN_RRAM_CTRL_RRAM_CTRL_TEST_SW_CACHE_TEST_MODE_OFFSET));
	HW_REG_WR(QWLAN_RRAM_CTRL_RRAM_CTRL_TEST_REG,value);
    return BL_ERR_NONE;
}
#endif
/*
* SBL RRAM write will use DXE function.
*/
bl_error_type boot_sbl_rram_write(uint32_t destination, uint8_t* source, uint32_t length)
{
	return rram_write_dxe(destination, source, length);
}

/* Dummy code for building pass. TODO: remove later, suggest: 
* 1) making QSPI/Flash to be seperate modules 
* 2) Or add more conditions in nt_logger module
*/
#define UNUSED(x) (void)(x) 
uint8_t nt_log_printf(
                uint8_t mod_id,
                uint8_t loglvl,
#if( NT_FN_FUNCTION_LINE_NUM_FLAG == 1)
                char *func_name,
                /*@ for line number*/
                uint16_t ln,
#endif
                const char *fmt,
                uint8_t num,
                ...
                )
{
    UNUSED(mod_id);
    UNUSED(loglvl);
#if( NT_FN_FUNCTION_LINE_NUM_FLAG == 1)    
    UNUSED(func_name);
    UNUSED(ln);
#endif
    UNUSED(fmt);
    UNUSED(num);
    return 0;
}
/* END - Dummy code */

