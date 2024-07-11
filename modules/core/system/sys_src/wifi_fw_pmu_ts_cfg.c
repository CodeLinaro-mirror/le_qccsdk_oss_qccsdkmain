/**********************************************************************************************
 * @file wifi_fw_pmu_ts_cfg.c
 * @brief PMU Temperature Sensor Configurations and Trims
 *
 *
 *********************************************************************************************/

/*----------------------------------------------------------------------------
 * Include Files
 * --------------------------------------------------------------------------*/

#include "fwconfig_cmn.h"
#ifdef PMU_TS_CONFIGURATION

#include "wifi_fw_pmu_ts_cfg.h"
#include "nt_common.h"
#include "HALhwio.h"
#include "Fermion_seq_hwioreg.h"
#include "fermion_hw_reg.h"
#include "nt_logger_api.h"
#include "hal_int_sys.h"
#include "wifi_fw_pwr_cb_infra.h"
#include "phyCalUtils.h"

/*-----------------------------------------------------------------------------
 * Global Data Definitions
 *----------------------------------------------------------------------------*/

pmu_ts_param_t g_pmu_ts_struct;


/*-----------------------------------------------------------------------------
 * Externalized Function Definitions
 *----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
 * @function  : pmu_ts_enable_vbatt_temp_mon_done_int
 * @brief     : Enable pmu_ccpu_temp_mon_done_intr Interuupt //Device Specific 79
 * @param     : None
 * @return    : None
 *-----------------------------------------------------------------------------
 */
void pmu_ts_enable_vbatt_temp_mon_done_int(void)
{
    uint32_t temp1;
    temp1 = HAL_REG_RD(NT_SOCPM_NVIC_ISER2);
    temp1 = temp1 | (0x1 << 15);
    HAL_REG_WR(NT_SOCPM_NVIC_ISER2, temp1);
    return;
}


/*-----------------------------------------------------------------------------
 * @function  : pmu_ccpu_temp_mon_done_intr
 * @brief     : Interuppt handler for pmu_ccpu_temp_mon_done_intr
 * @param     : None
 * @return    : None
 *-----------------------------------------------------------------------------
 */
void __attribute__((section(".after_ram_vectors"))) pmu_ccpu_temp_mon_done_intr(void)
{
    uint32_t RAW_TS;
    uint32_t rdata;
    if (((RAW_TS = HAL_REG_RD(QWLAN_PMU_TEMP_SNR_RD_DATA_REG)) & 
        QWLAN_PMU_TEMP_SNR_RD_DATA_TC_MEASURED_DATA_VALID_MASK) != 0)
    {
        g_pmu_ts_struct.pmu_ts_data_valid = true;
        g_pmu_ts_struct.pmu_ts_prev_valid_raw_data = RAW_TS & QWLAN_PMU_TEMP_SNR_RD_DATA_TC_MEASURED_DATA_MASK;
        g_pmu_ts_struct.pmu_ts_data_update_time = hres_timer_curr_time_ms();
    }
    else
    {
        g_pmu_ts_struct.pmu_ts_data_valid = false;
    }
    /* If this interuppt is due to one time temperature measurement, set it back 
       to preferred default mode (periodic temp meas mode) */
    if (g_pmu_ts_struct.pmu_ts_meas_mode == ONE_TIME)
    {
        //Set periodicity of temperature monitoring in XO ticks
        HAL_REG_WR(QWLAN_PMU_CFG_TEMP_MON_INTERVAL_REG, _SOCPM_US_TO_XO_TICK(PMU_TS_MON_PERIOD_US));

        rdata = HAL_REG_RD(QWLAN_PMU_CFG_ACAL_VBAT_MON_EN_REG);
        rdata = rdata | QWLAN_PMU_CFG_ACAL_VBAT_MON_EN_AUTO_TEMP_MON_EN_MASK |
            QWLAN_PMU_CFG_ACAL_VBAT_MON_EN_CFG_TEMP_MON_DONE_INTR_EN_MASK;
        HAL_REG_WR(QWLAN_PMU_CFG_ACAL_VBAT_MON_EN_REG, rdata);
        pmu_ts_enable_vbatt_temp_mon_done_int();
        g_pmu_ts_struct.pmu_ts_meas_mode = PERIODIC;
    }

    return;
}


/*-----------------------------------------------------------------------------
 * @function  : pmu_ts_configure
 * @brief     : Configure PMU TS for periodic temperature monitoring and enable HKADC
 * @param     : None
 * @return    : None
 *-----------------------------------------------------------------------------
 */
void pmu_ts_configure(void)
{
	pmu_ts_configure_periodic_meas();
}
void pmu_ts_configure_periodic_meas(void)
{
    uint32_t rdata;

    //Enable HKADC for temperature monitoring
    rdata = HAL_REG_RD(QWLAN_PMU_CFG_HKADC_DATA_AVG_CNT_REG);
    rdata &= ~QWLAN_PMU_CFG_HKADC_DATA_AVG_CNT_CFG_TEMP_VBATT_MON_SEL_MASK;
    HAL_REG_WR(QWLAN_PMU_CFG_HKADC_DATA_AVG_CNT_REG, rdata);

    //Set periodicity of temperature monitoring in XO ticks
    HAL_REG_WR(QWLAN_PMU_CFG_TEMP_MON_INTERVAL_REG, _SOCPM_US_TO_XO_TICK(PMU_TS_MON_PERIOD_US));

    //Enable periodic temperature monitoring
    rdata = HAL_REG_RD(QWLAN_PMU_CFG_ACAL_VBAT_MON_EN_REG);
    rdata = rdata | QWLAN_PMU_CFG_ACAL_VBAT_MON_EN_AUTO_TEMP_MON_EN_MASK | 
        QWLAN_PMU_CFG_ACAL_VBAT_MON_EN_CFG_TEMP_MON_DONE_INTR_EN_MASK;
    HAL_REG_WR(QWLAN_PMU_CFG_ACAL_VBAT_MON_EN_REG, rdata);
    
    pmu_ts_enable_vbatt_temp_mon_done_int();
    g_pmu_ts_struct.pmu_ts_configured = true;
    g_pmu_ts_struct.pmu_ts_meas_mode = PERIODIC;

    return;
}


/*-----------------------------------------------------------------------------
 * @function  : pmu_ts_get_raw_data
 * @brief     : Return the current value of PMU Temp Sensor reading
 * @param     : None
 * @return    : PMU TS data
 *-----------------------------------------------------------------------------
 */
uint32_t pmu_ts_get_raw_data(void)
{
    return g_pmu_ts_struct.pmu_ts_prev_valid_raw_data;
}

/*-----------------------------------------------------------------------------
 * @function  : pmu_ts_update_boot_temperature
 * @brief     : Update the bootup temperature
 * @param     : None
 * @return    : None
 *-----------------------------------------------------------------------------
 */
void pmu_ts_update_boot_temperature(void)
{
#if 0	
    uint32_t rdata, RAW_TS,start_time, stop_time;
    int32_t bootup_temp_deg;
    start_time = nt_hal_get_curr_time();

    //Enable HKADC for temperature monitoring
    rdata = HAL_REG_RD(QWLAN_PMU_CFG_HKADC_DATA_AVG_CNT_REG);
    rdata &= ~QWLAN_PMU_CFG_HKADC_DATA_AVG_CNT_CFG_TEMP_VBATT_MON_SEL_MASK;
    HAL_REG_WR(QWLAN_PMU_CFG_HKADC_DATA_AVG_CNT_REG, rdata);

    rdata = HAL_REG_RD(QWLAN_PMU_CFG_ACAL_VBAT_MON_EN_REG);
    HAL_REG_WR(QWLAN_PMU_CFG_ACAL_VBAT_MON_EN_REG, (rdata | QWLAN_PMU_CFG_ACAL_VBAT_MON_EN_TEMP_MON_EN_MASK));
#if(FERMION_CHIP_VERSION == 1)
    if((HWIO_INXF(SEQ_WCSS_OTP_OFFSET, FERMION_V1_0_QFPROM_RAW_FUSE_MAP_SECURITY_CONTROL_CORE_RAW_R_QFPROM_RAW_PTE_REGION_1_W3, TRIM_TAG_LOW) >= OTP_TRIM_TAG_LOW) ||
       (HWIO_INXF(SEQ_WCSS_OTP_OFFSET, FERMION_V1_0_QFPROM_RAW_FUSE_MAP_SECURITY_CONTROL_CORE_RAW_R_QFPROM_RAW_PTE_REGION_1_W3, TRIM_TAG_HIGH) >= OTP_TRIM_TAG_HIGH))
#else
    if((HWIO_INXF(SEQ_WCSS_OTP_OFFSET, FERMION_V2_0_QFPROM_RAW_FUSE_MAP_SECURITY_CONTROL_CORE_RAW_R_QFPROM_RAW_PTE_REGION_1_W3, TRIM_TAG_LOW) >= OTP_TRIM_TAG_LOW) ||
       (HWIO_INXF(SEQ_WCSS_OTP_OFFSET, FERMION_V2_0_QFPROM_RAW_FUSE_MAP_SECURITY_CONTROL_CORE_RAW_R_QFPROM_RAW_PTE_REGION_1_W3, TRIM_TAG_HIGH) >= OTP_TRIM_TAG_HIGH))
#endif
    {
        while ((((RAW_TS = HAL_REG_RD(QWLAN_PMU_TEMP_SNR_RD_DATA_REG)) & 0x80000000) == 0) && 
            ((nt_hal_get_curr_time() - start_time) < 1000))
        {
            continue;
        }
        g_pmu_ts_data_valid = true;
        g_pmu_ts_prev_valid_raw_data = RAW_TS & QWLAN_PMU_TEMP_SNR_RD_DATA_TC_MEASURED_DATA_MASK;
    }
    else
    {
        g_pmu_ts_data_valid = false;
    }
    bootup_temp_deg = pmu_ts_get_current_temperature();
    stop_time = nt_hal_get_curr_time();
    NT_LOG_PRINT(SOCPM, WARN, "Bootup temperature %d, PMU_TS raw data %d, time taken to measure temperature %d", 
        bootup_temp_deg, g_pmu_ts_prev_valid_raw_data, (stop_time - start_time));
    return;
#endif	
}

/*-----------------------------------------------------------------------------
 * @function  : pmu_ts_get_current_temperature
 * @brief     : Return current temperature in degrees from PMU TS raw data
 * @param     : None
 * @return    : Current temperature in degree celsius
 *-----------------------------------------------------------------------------
 */
int32_t pmu_ts_get_current_temperature(void)
{
    uint32_t pmu_reg_data;
    pmu_reg_data = pmu_ts_get_raw_data();
    return pmu_ts_convert_to_deg_cel(pmu_reg_data);
}


/*-----------------------------------------------------------------------------
 * @function  : pmu_ts_get_current_temperature
 * @brief     : Calculate current temperature from PMU TS raw data
 * @param     : pmu_reg_data -Raw TS data from PMU register
 * @return    : Current temperature in degree celsius
 *-----------------------------------------------------------------------------
 */
int32_t pmu_ts_convert_to_deg_cel(uint32_t pmu_reg_data)
{

    int32_t ts_gain_otp_value = 0, ts_offset_residue = 0, temp_deg = 25;
    float ts_slope_meas, ts_raw_data;
    float ts_gain_err = -14.0f;
    uint8_t otp_version_high = 1;
    uint8_t otp_version_low = 0;

#if(FERMION_CHIP_VERSION == 1)
    otp_version_high = HWIO_INXF(SEQ_WCSS_OTP_OFFSET, FERMION_V1_0_QFPROM_RAW_FUSE_MAP_SECURITY_CONTROL_CORE_RAW_R_QFPROM_RAW_PTE_REGION_1_W3, TRIM_TAG_HIGH);
    otp_version_low = HWIO_INXF(SEQ_WCSS_OTP_OFFSET, FERMION_V1_0_QFPROM_RAW_FUSE_MAP_SECURITY_CONTROL_CORE_RAW_R_QFPROM_RAW_PTE_REGION_1_W3, TRIM_TAG_LOW);
#else /* FERMION_CHIP_VERSION == 1 */
    otp_version_high = HWIO_INXF(SEQ_WCSS_OTP_OFFSET, FERMION_V2_0_QFPROM_RAW_FUSE_MAP_SECURITY_CONTROL_CORE_RAW_R_QFPROM_RAW_PTE_REGION_1_W3, TRIM_TAG_HIGH);
    otp_version_low = HWIO_INXF(SEQ_WCSS_OTP_OFFSET, FERMION_V2_0_QFPROM_RAW_FUSE_MAP_SECURITY_CONTROL_CORE_RAW_R_QFPROM_RAW_PTE_REGION_1_W3, TRIM_TAG_LOW);
#endif /* FERMION_CHIP_VERSION == 1 */

    if (otp_version_high >= OTP_V2)// for OTP >= v2
    {
#if(FERMION_CHIP_VERSION == 1)
        ts_gain_otp_value = HWIO_INX(SEQ_WCSS_OTP_OFFSET,
            FERMION_V1_0_QFPROM_RAW_FUSE_MAP_SECURITY_CONTROL_CORE_RAW_R_QFPROM_RAW_RF_CALIBRATION_ROW7_W3);
        ts_gain_otp_value = (ts_gain_otp_value & QWLAN_SECURITY_CONTROL_CORE_RAW_R_QFPROM_RAW_RF_CALIBRATION_ROW7_W3_TSENSOR_GAIN_ERROR_MASK) >>
            QWLAN_SECURITY_CONTROL_CORE_RAW_R_QFPROM_RAW_RF_CALIBRATION_ROW7_W3_TSENSOR_GAIN_ERROR_OFFSET;
        ts_gain_otp_value = sign_extend_dword((uint32_t)ts_gain_otp_value, 8);
#else
        /* TSENDOR_OFFSET_RESIDUE field is added to only 2.0 version of chip */
        /* convert two's complement to signed */
        ts_gain_otp_value = sign_extend_dword((uint32_t)(HWIO_INXF(SEQ_WCSS_OTP_OFFSET,
            FERMION_V2_0_QFPROM_RAW_FUSE_MAP_SECURITY_CONTROL_CORE_RAW_R_QFPROM_RAW_RF_CALIBRATION_ROW7_W3, TSENSOR_GAIN_ERROR)), 8);
        ts_offset_residue = sign_extend_dword((uint32_t)(HWIO_INXF(SEQ_WCSS_OTP_OFFSET,
            FERMION_V2_0_QFPROM_RAW_FUSE_MAP_SECURITY_CONTROL_CORE_RAW_R_QFPROM_RAW_RF_CALIBRATION_ROW8_W0, TSENSOR_OFFSET_RESIDUE)), 8);
#endif
        ts_gain_err = (float)ts_gain_otp_value * TS_GAIN_RESOLUTION;
    }

    ts_raw_data = ((float)pmu_reg_data / 2.0f); // 9 bit adc output convereted to 8 bit resolution
    ts_slope_meas = ((ts_gain_err * TS_SLOPE_IDEAL) / 100.0f) + TS_SLOPE_IDEAL;
    if (!ts_slope_meas)
    {
        NT_LOG_SOCPM_CRIT("TS slope is 0 - DIV by zero error", ts_gain_err, ts_gain_otp_value, 0);
        ts_slope_meas = TS_SLOPE_IDEAL;
    }
    temp_deg = (int32_t)(((ts_raw_data - (float)TS_IDEAL_DOUT_30C - (float)ts_offset_residue) / ts_slope_meas) + TS_DOUT_REF_DEG);
    if (((otp_version_high == OTP_V3) && ((otp_version_low == 2) || (otp_version_low == 3))) ||
        (otp_version_high == OTP_V4) || ((otp_version_high == OTP_V5) && (otp_version_low == 0)))
    {
        temp_deg = temp_deg - TEMP_OUTPUT_OFFSET; // Only needed for debug boards (otpv3p2, otpv3p3, otpv4, otpv5p0)
    }
    return temp_deg;
}
/*-----------------------------------------------------------------------------
 * @function  : pmu_ts_init
 * @brief     : Initializes the SW params and update the bootup temperature
 * @param     : None
 * @return    : None
 *-----------------------------------------------------------------------------
 */
void pmu_ts_init(void)

{
    uint32_t rdata, RAW_TS,start_time, stop_time;
    int32_t bootup_temp_deg;

    g_pmu_ts_struct.pmu_ts_data_valid = false;
    g_pmu_ts_struct.pmu_ts_meas_mode = ONE_TIME;
    g_pmu_ts_struct.pmu_ts_prev_valid_raw_data = PMU_TS_ROOM_TEMP_DEFAULT;
    start_time = hres_timer_curr_time_ms();
    g_pmu_ts_struct.pmu_ts_data_update_time = 0;

    //Enable HKADC for temperature monitoring
    rdata = HAL_REG_RD(QWLAN_PMU_CFG_HKADC_DATA_AVG_CNT_REG);
    rdata &= ~QWLAN_PMU_CFG_HKADC_DATA_AVG_CNT_CFG_TEMP_VBATT_MON_SEL_MASK;
    HAL_REG_WR(QWLAN_PMU_CFG_HKADC_DATA_AVG_CNT_REG, rdata);

    rdata = HAL_REG_RD(QWLAN_PMU_CFG_ACAL_VBAT_MON_EN_REG);
    HAL_REG_WR(QWLAN_PMU_CFG_ACAL_VBAT_MON_EN_REG, (rdata | QWLAN_PMU_CFG_ACAL_VBAT_MON_EN_TEMP_MON_EN_MASK));
#if(FERMION_CHIP_VERSION == 1)
    if(HWIO_INXF(SEQ_WCSS_OTP_OFFSET, FERMION_V1_0_QFPROM_RAW_FUSE_MAP_SECURITY_CONTROL_CORE_RAW_R_QFPROM_RAW_PTE_REGION_1_W3, TRIM_TAG_HIGH) 
           >= OTP_V1)
#else
    if(HWIO_INXF(SEQ_WCSS_OTP_OFFSET, FERMION_V2_0_QFPROM_RAW_FUSE_MAP_SECURITY_CONTROL_CORE_RAW_R_QFPROM_RAW_PTE_REGION_1_W3, TRIM_TAG_HIGH) 
           >= OTP_V2)
#endif
    {
        while ((((RAW_TS = HAL_REG_RD(QWLAN_PMU_TEMP_SNR_RD_DATA_REG)) & QWLAN_PMU_TEMP_SNR_RD_DATA_TC_MEASURED_DATA_VALID_MASK) == 0) && 
            ((hres_timer_curr_time_ms() - start_time) < TEMP_MEAS_TIMEOUT_MS)) 
        {
            continue;
        }
        if ((RAW_TS & QWLAN_PMU_TEMP_SNR_RD_DATA_TC_MEASURED_DATA_VALID_MASK) != 0)
        {
            g_pmu_ts_struct.pmu_ts_data_valid = true;
            g_pmu_ts_struct.pmu_ts_prev_valid_raw_data = RAW_TS & QWLAN_PMU_TEMP_SNR_RD_DATA_TC_MEASURED_DATA_MASK;
            g_pmu_ts_struct.pmu_ts_data_update_time = start_time;
        }
    }
    stop_time = hres_timer_curr_time_ms();
    bootup_temp_deg = pmu_ts_get_current_temperature();
    NT_LOG_PRINT(SOCPM, WARN, "Bootup temperature %d, PMU_TS raw data %d, time taken to measure temperature %d, is Valid %d", 
        bootup_temp_deg, g_pmu_ts_struct.pmu_ts_prev_valid_raw_data, (stop_time - start_time), 
        g_pmu_ts_struct.pmu_ts_data_valid);
    fpci_evt_cb_reg((ps_evt_cb_t)&pmu_ts_power_state_change_cb, PWR_EVT_WMAC_PRE_SLEEP | PWR_EVT_WMAC_POST_AWAKE | 
        PWR_EVT_WMAC_SLEEP_ABORT, PS_CALLBACK_PMU_TS_PRIORITY, NULL);
    return;
}


/*-----------------------------------------------------------------------------
 * @function  : is_pmu_ts_data_valid
 * @brief     : Return true if pmu ts data valid bit is set else return false
 * @param     : None
 * @return    : g_pmu_ts_struct.pmu_ts_data_valid
 *-----------------------------------------------------------------------------
 */
bool is_pmu_ts_data_valid(void)
{
    return (bool)g_pmu_ts_struct.pmu_ts_data_valid;
}


/*-----------------------------------------------------------------------------
 * @function  : is_pmu_ts_configured
 * @brief     : Return true if pmu ts is configured else return false
 * @param     : None
 * @return    : g_pmu_ts_struct.pmu_ts_configured
 *-----------------------------------------------------------------------------
 */
bool is_pmu_ts_configured(void)
{
    return (bool)g_pmu_ts_struct.pmu_ts_configured;
}


/*-----------------------------------------------------------------------------
 * @function  : invalidate_pmu_ts_configuration
 * @brief     : Set g_pmu_ts_struct.pmu_ts_configured to false
 * @param     : None
 * @return    : None
 *-----------------------------------------------------------------------------
 */
void invalidate_pmu_ts_configuration(void)
{
    g_pmu_ts_struct.pmu_ts_configured = false;
    return;
}


#ifdef FEATURE_FPCI
/**
 * @brief  Presleep/ Postawake Callbacks for PMU TS
 * @param  evt - Denotes the sleep event
 * @return None
 */
void pmu_ts_power_state_change_cb(uint8_t evt, void* p_args)
{
    (void)p_args;
    uint32_t rdata;
    if (evt == PWR_EVT_WMAC_PRE_SLEEP)
    {
        invalidate_pmu_ts_configuration();
    }
    else if ((evt == PWR_EVT_WMAC_POST_AWAKE) || (evt == PWR_EVT_WMAC_SLEEP_ABORT))
    {
        uint32_t curr_time = hres_timer_curr_time_ms();
       
        /* If time from prev temp meas exceeds PMU_TS_MON_PERIOD_MS(2 secs) initiate a one time measurement */
        if ((uint32_t)(curr_time - g_pmu_ts_struct.pmu_ts_data_update_time) > PMU_TS_MON_PERIOD_MS)
        {
            g_pmu_ts_struct.pmu_ts_data_valid = false;

            //Enable HKADC for temperature monitoring
            rdata = HAL_REG_RD(QWLAN_PMU_CFG_HKADC_DATA_AVG_CNT_REG);
            rdata &= ~QWLAN_PMU_CFG_HKADC_DATA_AVG_CNT_CFG_TEMP_VBATT_MON_SEL_MASK;
            HAL_REG_WR(QWLAN_PMU_CFG_HKADC_DATA_AVG_CNT_REG, rdata);

            //Enable one time temperature monitoring
            rdata = HAL_REG_RD(QWLAN_PMU_CFG_ACAL_VBAT_MON_EN_REG);
            rdata = (rdata | QWLAN_PMU_CFG_ACAL_VBAT_MON_EN_TEMP_MON_EN_MASK |
                QWLAN_PMU_CFG_ACAL_VBAT_MON_EN_CFG_TEMP_MON_DONE_INTR_EN_MASK) & 
                (~QWLAN_PMU_CFG_ACAL_VBAT_MON_EN_AUTO_TEMP_MON_EN_MASK);
            HAL_REG_WR(QWLAN_PMU_CFG_ACAL_VBAT_MON_EN_REG, rdata);
            g_pmu_ts_struct.pmu_ts_meas_mode = ONE_TIME;
            pmu_ts_enable_vbatt_temp_mon_done_int();
        }
        else
        {
            pmu_ts_configure_periodic_meas();
        }
    }
    return;
}
#endif /* FEATURE_FPCI */


#endif /* PMU_TS_CONFIGURATION */
