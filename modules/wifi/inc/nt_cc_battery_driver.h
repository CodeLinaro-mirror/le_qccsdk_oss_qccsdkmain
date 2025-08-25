/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "stdint.h"
#ifndef CORE_SYSTEM_INC_NT_CC_BATTERY_DRIVER_H_
#define CORE_SYSTEM_INC_NT_CC_BATTERY_DRIVER_H_

#define QWLAN_TPE_INTERRUPT_ENABLE_RINT_VBATT_INIT_EN_OFFSET 0XE   // Enable the interrupts for rint measurement
#define QWLAN_TPE_SW_RINT_VBATT_RINT_VBATT_TIME_OUT_OFFSET   0X10  // Timeout bit setting for rint measurement
#define QWLAN_TPE_SW_RINT_VBATT_RINT_VBATT_ENABLE_OFFSET     0X0
#define QWLAN_TPE_INTERRUPT_STATUS_RINT_VBATT_TIMEOUT_INT_STATUS_OFFSET \
    0xF  // Timeout interrupt enabling in TPE interrupt register
// #define QWLAN_TPE_SW_RINT_VBATT_RINT_VBATT_WINDOW_MON_EN_MASK		        0x100000 //Enable window monitoring for rint
// measurement
#define LOWEST_TEMPERATURE_IN_CELSIUS                  -40  // lowest temperature in celcius
#define LOWEST_TEMPERATURE_REG_READ                    50   // lowest temperature in the look up table(not in celcius)
#define TEMPERATURE_INCREMENT                          0.40244  // temperature difference for one point change
#define LOWEST_VOLTAGE_IN_VOLTS                        1.6      // lowest voltage in volts
#define LOWEST_VOLTAGE_REG_READ                        50       // lowest voltage in the look up table(not in volts)
#define VOLTAGE_INCREMENT                              0.0049   // voltage difference for one point change
#define QWLAN_TPE_SW_RINT_VBATT_RINT_VBATT_MIN_SAMPLES 0x3
#define VBATT_RINT_TIMEOUT_CONFIG                      0x32  // TPE will stop vbatt measurement this many(50) u seconds after last TX
// if vbatt does not reach rint_vbatt_threshold
#define VBATT_RINT_VBATT_START_THRESHOLD_CONFIG \
    0xDD  // 2.5 V,First sample of Vbatt is greater than this Rint measurement will be cancelled
#define VBATT_RINT_VBATT_THRESHOLD_CONFIG 0xDD  // 2.5 V, initial vbatt_threshold_config, TPE will continue to measure
// vbatt samples until average is greater than this threshold
#define TXOP_VBATT_THRESHOLD_CONFIG \
    0x3F  // 1.8 V(c7/b0/99/77/6b/56/49/3F) vbatt_threshold_config, TPE will continue to measure
          // vbatt samples until average is greater than this threshold
#define TPE_SW_RINT_VBATT_RINT_VBATT_THRESHOLD_MASK 0x1FF  // Mask to disable rint_vbatt_threshold bits
#define VBATT_RINT_AVERAGE_SAMPLES_CONFIG           0x1    // 2 samples will be taken for averaging
#define VBATT_TXOP_AVERAGE_SAMPLES_CONFIG           0x1    // 2 samples will be taken for averaging
#define VBATT_RINT_MIN_SAMPLES_CONFIG               0x1    // after this much samples taken Rint will be measured
#define VBATT_TXOP_MIN_SAMPLES_CONFIG               0x1    //	after this much samples taken TXoP will be adjusted
#define VBATT_RINT_VBATT_WINDOW_CONFIG \
    0xAAA  // random value for now to configure vbatt_window time for rint measurement
typedef enum measure { TEMP_MEASURE, VBATT_MEASURE } type_t;
typedef enum { timeout, success, txop_aborted } nt_batt_status;

typedef enum status {
    VBATT_AVG,
    SAMPLE,
    SAMPLE_PTR,
    RECOVERY_TIME,
    VBATT_END_TIME,
    VBATT_START_TIME

} statustype_t;

typedef struct {
    uint32_t battery_charge_discharge;
    uint32_t resistance;
    uint32_t txop;
    uint32_t tx_rate;
    uint32_t battery_level;
} battery_measurements_t;

/* A driver function defines what action to be taken based on the battery level */
typedef uint32_t (*txop_driver_fcn)(uint32_t vbatt_level);

typedef struct {
    /* Rint measurement window in micro seconds*/
    uint32_t rint_window;
    /* TPE will ignore this rint measurement if the first sample is greater than this threshold*/
    uint32_t rint_vbatt1_threshold;
    /* TPE will continue to measure vbatt samples until average is greater than this threshold*/
    uint32_t vbatt_threshold;
    /*  configuration How to average the vbatt samples Decoded as: 0 : Average taken over last 1 sample
     * 1 : Average taken over last 2 samples
     * 2 : Average taken over last 4 samples
     * 3 : Average taken over last 8 samples
       Note, if rint_vbatt_min_samples is less than the average samples, rint will only be measured
       if we receive at least the number of average  */
    uint8_t average_samples;
    /* TPE will stop vbatt meaurement this many usecs after last TX if vbatt does not reach rint_vbatt_threshold */
    uint16_t vbatt_timeout_threshold;
    /* battery level after successful rint measurement*/
    uint32_t vbatt_level;
    /*driver function defines what action to be taken based on the battery level */
    txop_driver_fcn driver_function;
} nt_cc_user_config_t;

typedef struct cc_mgmt_s {
    uint32_t recovery_time;
    txop_driver_fcn cc_mgmt_driver_fn;
    uint32_t client_id;
    struct cc_mgmt_s *next;
} nt_cc_mgmt_t;

/**
 * <!-- nt_cc_add_client_callback -->
 *
 * @brief              : Add a new linked list node for user info (user defined function & recovery time)
 * @param driver_fcn   : function pointer for the callback if the measured battery level is less than the required
 * threshold
 * @param recovery_time: recovery time in micro seconds read
 * @return             : void
 */
void nt_cc_add_client_callback(txop_driver_fcn driver_fcn, uint32_t recovery_time);

/**
 * <!-- nt_cc_call_user_callback_from_isr -->
 *
 * @brief               : To calculate vbatt level and call the user defined function to adjust txop after interrupt has
 * been triggered.
 * @param battery_level : Battery level calculated
 * @return              : void
 */
void nt_cc_call_user_callback_from_isr(uint32_t battery_level);
/**
 * <!-- nt_issue_batt_level_mon_request -->
 *
 * @brief             : To measure the current battery level of coin cell battery. This function will do the
 configurations for rint in respective registers and bit positions. ADC will start the sampling of data once the
                        configurations are done
 * @param user_config : structure variable to give inputs to configure the registers
 * @param driver_fcn  : function pointer for the callback if the measured battery level is less than the required
 threshold
 * @return            : void
 */
void nt_issue_batt_level_mon_request(nt_cc_user_config_t *user_config, txop_driver_fcn driver_fcn);

/**
 * <!-- nt_cc_battery_level -->
 *
 * @brief            : ISR to read the recovery time for coin cell management
 * @return           : void
 */
void nt_cc_battery_level(void);
void nt_cc_battery_mgmt_init(void);
float nt_pmu_vbatt_temp_get(type_t);

/**
 * <!-- nt_tpe_rint_get -->
 *
 * @brief            : for debugging purpose we will take parameters vbatt_avg,
 * sample_index,sample_ptr,recovery_time,vbatt_endtime,vbatt_starttime.
 * @param statustype :
 * @return           : void
 */
uint32_t nt_tpe_rint_get(statustype_t);

/**
 * <!-- nt_lut_map -->
 *
 * @brief              : Convert recovery time into battery level based on the look up table.
 * @param recovery_time: recovery time in micro seconds
 * @return             : void
 */
uint16_t nt_lut_map(uint32_t recovery_time);
void nt_lookup_table_init();

void pmu_ccpu_vbatt_low_hit_int_p(void);
void pmu_ccpu_temp_panic_hit_int(void);

/**
 * <!-- nt_tpe_txop_init -->
 *
 * @brief      : To initialize txop configurations to control the transmission rate
 * @return     : void
 */
void nt_tpe_txop_init(void);

/**
 * <!-- nt_tpe_rint_init -->
 *
 * @brief      : To initialize To initialize rint configurations for battery level
 * @return     : void
 */
void nt_tpe_rint_init(void);
/**
 * <!-- nt_update_battery_level -->
 *
 * @brief      : To initialize To initialize rint configurations for battery level
 * @return     : void
 */
void nt_update_battery_level(void);
/**
 * <!-- nt_cc_txop_adjust -->
 *
 * @brief      : To abandon the txop if the battery level drops below a certain threshold
 * @return     : void
 */
void nt_cc_txop_adjust(void);
#endif /* CORE_SYSTEM_INC_NT_CC_BATTERY_DRIVER_H_ */
