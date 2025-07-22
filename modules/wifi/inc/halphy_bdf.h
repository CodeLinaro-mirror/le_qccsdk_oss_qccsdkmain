/*========================================================================
Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 * @file halphy_bdf.h
 * @brief Function header and defines for Board Data File (BDF) framework
 * ======================================================================*/

#ifndef _HALPHY_BDF_H_
#define _HALPHY_BDF_H_

/*------------------------------------------------------------------------
 * Include Files
 * ----------------------------------------------------------------------*/

#include <stdint.h>
#include <stdbool.h>

#include "halphy_hdl_api.h"
#include "halphy_trx_qcp5321_bdf.h"

/*-------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * ----------------------------------------------------------------------*/

#define CONVERT_TO_QUARTER_DB(x) (x * 4)

#ifdef PLATFORM_NT
#define BDF_BOARD_ID 1
#elif defined(PLATFORM_FERMION)
#if FERMION_CHIP_VERSION == 2
#define BDF_BOARD_ID 2
#else
#define BDF_BOARD_ID 0
#endif
#endif

#define pEeprom ((BDF_STRUCT *)pEeprom_p)

typedef enum nv_sections_e { NV_IOT_RX_GAIN_TABLES = 0, NV_IOT_TPC_DATA = 1 } nv_sections_e;

#ifdef PLATFORM_FERMION
typedef struct halphy_rssi_correction_s {
    int8_t rssi_range_0;
    int8_t rssi_range_1;
    int8_t rssi_range_2;
    int8_t rssi_range_3;
    int8_t rssi_correction_0;
    int8_t rssi_correction_1;
    int8_t rssi_correction_2;
    int8_t rssi_correction_3;
    int8_t rssi_correction_4;
    int8_t rssi_temp_correction;
} halphy_rssi_correction_t;
#endif /* PLATFORM_FERMION */

/*-------------------------------------------------------------------------
 * Function Declarations and Documentation
 * ----------------------------------------------------------------------*/

/* Initialize BDF */
bool halphy_bdf_init(const uint32_t *p_bdf_data);
uint8_t halphy_bdf_get_tx_power_mode(uint16_t freq);
uint8_t halphy_bdf_get_lna_power_mode(uint8_t band);
bool halphy_bdf_get_caldb_bypass(void);
void halphy_bdf_set_caldb_bypass(uint8_t caldb_bypass);
void halphy_bdf_set_crx_enable(uint8_t crx_enable);
uint8_t halphy_bdf_get_xpa_enable(void);
uint32_t halphy_bdf_get_xfem_ctrl(void);
uint8_t halphy_bdf_get_calmask(void);

uint16_t halphy_bdf_get_default_cc(void);
uint8_t halphy_bdf_regdb_version(void);
bool halphy_bdf_checksum(const void *p_bdf, uint32_t bdf_size);
uint32_t halphy_bdf_get_board_data_size(void);
uint8_t *halphy_bdf_get_board_data_ptr(void);
void halphy_bdf_recal_board_data_checksum(void);
bool halphy_bdf_cached_bdf_init(void);
void halphy_bdf_update_nv_section(nv_sections_e nv_section);
void halphy_bdf_get_crx_mode(int8_t *crx_mode);
void halphy_bdf_update_capin_capout(uint8_t capin, uint8_t capout);

#ifdef PLATFORM_FERMION
void halphy_bdf_get_rssi_correction_value(halphy_rssi_correction_t *rssi_corr, uint8_t band);
void halphy_update_rssi_temp_correction_value(halphy_rssi_correction_t *rssi_corr, uint8_t curr_band,
                                              int16_t curr_temp);
int8_t halphy_bdf_get_crx_rssi_correction(void);
#endif /* PLATFORM_FERMION */
uint32_t halphy_bdf_get_configAddr(void);
uint32_t halphy_bdf_get_rtt_base_delay(uint8_t band);
bool halphy_bdf_get_temp_recal_support(uint8_t band);
void halphy_bdf_get_temp_recal_th(uint8_t band, int8_t *TempThLo, int8_t *TempThHi);
bool halphy_bdf_channel_supported(uint16_t channel_freq, uint16_t country_id);
uint32_t halphy_bdf_get_rtt_base_delay(uint8_t band);
void halphy_bdf_set_cbc_chan(uint8_t num_chan, uint32_t *ch_list);
void halphy_bdf_set_cc(uint16_t country_code);
bool halphy_bdf_dpd_bypass(BDF_STRUCT *pBd);
#endif /* _HALPHY_BDF_H_ */
