/*========================================================================
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

typedef enum nv_sections_e
{
    NV_IOT_RX_GAIN_TABLES = 0,
    NV_IOT_TPC_DATA = 1
} nv_sections_e;

/*-------------------------------------------------------------------------
 * Function Declarations and Documentation
 * ----------------------------------------------------------------------*/

/* Initialize BDF */
bool halphy_bdf_init(const uint8_t *p_bdf_data);
void halphy_bdf_get_tx_power_mode(uint16_t freq, tx_power_mode_t *tx_power_mode);
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

#endif /* _HALPHY_BDF_H_ */
