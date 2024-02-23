/*******************************************************************************
 * @file wifi_fw_cpr_driver.h
 * @brief WiFi FW CPR related declarations
 *
 *
 ******************************************************************************/

#ifndef _WIFI_FW_CPR_DRIVER_H_
#define _WIFI_FW_CPR_DRIVER_H_

#ifdef PLATFORM_FERMION


#define CPR_OTP_TRIM_TAG_HIGH                   2

#define CPR_CX_SLEEP_MV                       600
#define CPR_OTP_TARGET_MAX                     90
#define CPR_OTP_TARGET_MIN                     37
#define CPR_CX_VOLTAGE_FOR_NOT_TRIMMED_CHIP   567
#define CPR_CX_VOLTAGE_FOR_MAX_OTP_TARGET     600
#define CPR_CX_VOLTAGE_FOR_MIN_OTP_TARGET     440
/* Period to kick off another measurement once CPR has found the target CX
   (ie. no step up/dn) */
#define CPR_DONE_MEASUREMENT_PERIOD_USECS    2000
/* Period to kick off another measurement once CPR has completed a step up/dn */
#define CPR_STEP_MEASUREMENT_PERIOD_USECS     500
/* Step size (0-7): how many vrefs (ie. 1.5mV steps) the controller will step */
#define CPR_STEP_SIZE                           2
#define CPR_MAX_CX_VOLTAGE                    667
#define CPR_MIN_CX_VOLTAGE                    450

#define CPR_RO_GCNT                           0x9
#define CPR_RO0_TARGET                        281
#define CPR_RO2_TARGET                        172
#define CPR_RO3_TARGET                        156
#define CPR_RO12_TARGET                        64
#define CPR_RO14_TARGET                       228
#define CPR_RO15_TARGET                       102


/* Structure to store frequently used CPR parameters
   in global structure g_socpm_struct */
typedef struct {
    uint32_t ini_enabled;
    uint32_t otp_tag_high;
    uint32_t cx_initial_mV_vref;
    uint32_t cx_sleep_mV_vref;
} cpr_cfg_t;


/******************************************************************************

* Function Declaration

*******************************************************************************/

/**
 *  @brief  Initializes CPR module.
 *          Should be called from main after calling PMIC init.
 *  @param  None
 *  @return None
 */
void wifi_fw_cpr_init(void);

/**
 *  @brief  Re-enable CPR module.
 *          Should be called in case of warm boot.
 *  @param  None
 *  @return None
 */
void wifi_fw_cpr_reenable(void);

/**
 *  @brief  Disable CPR module.
 *          Should be called before going to light sleep,
 *          MCU sleep and deep sleep.
 *  @param  None
 *  @return None
 */
void wifi_fw_cpr_disable(void);

#endif /* PLATFORM_FERMION */

#endif /* _WIFI_FW_CPR_DRIVER_H_ */
