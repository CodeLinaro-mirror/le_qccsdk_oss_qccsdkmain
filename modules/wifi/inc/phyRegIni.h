/*
Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _HALPHY_INI_H_
#define _HALPHY_INI_H_
#include <stdint.h>
#include <stdbool.h>

#include "HALhwio.h"
#include "fermion_reg.h"

#ifdef _WIN32
#define NT_LOG_PRINT(MODNAME, LVL, msg, ...) \
    {                                        \
        printf(msg, __VA_ARGS__);            \
        printf("\n");                        \
    }
#include <Windows.h>
#define nt_osal_delay(ms) Sleep(ms)
#elif defined(CONFIG_Q5EXT) && defined(PHYDEVLIB_IMAGE_STANDALONE)
//#include "hal_int_sys.h"
#define NT_LOG_PRINT(MODNAME, LVL, msg, ...) \
    {                                        \
        printf(msg, ##__VA_ARGS__);          \
        printf("\n");                        \
    }
#else
#include "hal_int_sys.h"
#include <nt_logger_api.h>
#endif

#define HALPHY_NEW_INI  // INI Config to increase LNA gain (Increases RX gain by 8 dB)

typedef struct {
    uint32_t addr;  // address
    uint32_t data;  // Data
} ini_data_t;

void phyrf_load_rx_gain_lut(uint8, uint16);

void phyrf_program_trim_values(void);
bool phyrf_check_trim_status(void);
void phyrf_CRx_Enable(void *coex_Input);
void phyrf_CRx_Disable();
void phyrf_init_bb(uint8_t band_code, uint8_t bSkipBandCommonTable, uint16_t mhz);
void phyrf_init_rf(uint8_t band_code, uint16_t mhz);
void phyrf_rf_rctuning_rxbb_bbf();
void phyrf_rf_rctuning_txbb_ppa();
void phyrf_rf_rctuning_txbb_bbf(uint8_t bandcode);
#endif /* _HALPHY_INI_H_ */
