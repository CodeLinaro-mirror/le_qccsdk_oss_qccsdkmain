/**********************************************************************************************
 * @file wifi_fw_pmu_ts_cfg.h
 * @brief PMU Temperature Sensor Configurations and Trims
 *
 *
 *********************************************************************************************/

#ifndef _WIFI_FW_PMU_TS_CFG_H_
#define _WIFI_FW_PMU_TS_CFG_H_

/*-----------------------------------------------------------------------------
 * Include Files
 * ---------------------------------------------------------------------------*/
#include "fwconfig_cmn.h"
#ifdef PMU_TS_CONFIGURATION
#include <stdint.h>
#include <stdbool.h>


/*-----------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * ---------------------------------------------------------------------------*/
#define PMU_TS_ROOM_TEMP_DEFAULT 212
#define PMU_TS_MON_PERIOD_US 2000000 // 2 seconds
#define R_QFPROM_RAW_RF_CALIBRATION_ROW7_W3_GAIN_ERR_BMSK 0xFF0000
#define R_QFPROM_RAW_RF_CALIBRATION_ROW7_W3_GAIN_ERR_SHFT 16
#define OTP_TRIM_TAG_LOW 1
#define OTP_TRIM_TAG_HIGH 2
// XO tick is computed in terms of 38.4 Mhz. Each tick is 1/38400000 s = 1000000/38400000 us = 10/384= 5/192 us
// So number of ticks given the time in us is (time *192) /5
#define _SOCPM_US_TO_XO_TICK(slp_time_us) (((slp_time_us) * 192) / 5)


/*-----------------------------------------------------------------------------
 * Function Declarations
 *----------------------------------------------------------------------------*/

void pmu_ts_enable_vbatt_temp_mon_done_int(void);
uint32_t pmu_ts_get_raw_data(void);
int32_t pmu_ts_get_current_temperature(void);
void pmu_ts_configure(void);
void pmu_ts_update_boot_temperature(void);
bool is_pmu_ts_data_valid(void);
bool is_pmu_ts_configured(void);
void invalidate_pmu_ts_configuration(void);

#endif /* PMU_TS_CONFIGURATION */
#endif /* _WIFI_FW_PMU_TS_CFG_H_ */
