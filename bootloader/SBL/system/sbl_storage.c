/*
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

/*
* SBL flash init function.
*/
bl_error_type boot_sbl_flash_init()
{
    bl_error_type status;

    status = drv_flash_init();
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
	return drv_flash_read(address, byte_cnt, buffer, NULL, NULL);
}

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

