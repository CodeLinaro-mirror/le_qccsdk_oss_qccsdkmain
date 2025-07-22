/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*******************************************************************************
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 *
 * @file wifi_fw_cpr_driver.h
 * @brief WiFi FW CPR related declarations
 *
 *
 ******************************************************************************/

#ifndef _WIFI_FW_CPR_DRIVER_H_
#define _WIFI_FW_CPR_DRIVER_H_

#ifdef PLATFORM_FERMION

#ifndef CPR_2G_PER_ISSUE_WAR
#define CPR_2G_PER_ISSUE_WAR
#endif

/* CPR is enabled only when OTP >= 5.0 */
#define CPR_OTP_TRIM_TAG_HIGH 5

#define CPR_CX_MIN_SLEEP_MV                 630
#define CPR_OTP_TARGET_MAX                  90
#define CPR_OTP_TARGET_MIN                  37
#define CPR_CX_VOLTAGE_FOR_NOT_TRIMMED_CHIP 567
#define CPR_CX_VOLTAGE_FOR_MAX_OTP_TARGET   600
#define CPR_CX_VOLTAGE_FOR_MIN_OTP_TARGET   440
/* Period to kick off another measurement once CPR has found the target CX
   (ie. no step up/dn) */
#define CPR_DONE_MEASUREMENT_PERIOD_USECS 2000
/* Period to kick off another measurement once CPR has completed a step up/dn */
#define CPR_STEP_MEASUREMENT_PERIOD_USECS 500
/* Step size (0-7): how many vrefs (ie. 1.5mV steps) the controller will step */
#define CPR_STEP_SIZE      2
#define CPR_MAX_CX_VOLTAGE 667
#define CPR_MIN_CX_VOLTAGE 490

#define CPR_RO_GCNT 0x9

/*
 *   UHD               |  CX  |  CX+15m  | CX+20mV | CX+25mV | CX+30mV | CX+35mV |Selection|
 * -----------------------------------------------------------------------------------------
 * TT/CW corner voltage| 0.57 |                     ColdT guardband                        |
 * -----------------------------------------------------------------------------------------
 * ro0_ro_aoi_40_lvt   |  240 |   281    |   29    |   309   |   322   |   336   |    X    |
 * ro1_ro_aoi_40_svt   |  101 |   127    |  135    |   144   |   152   |   161   |         |
 * ro2_ro_fa_40_ulvt   |  146 |   172    |  181    |   189   |   200   |   207   |    X    |
 * ro3_ro_fa_40_lvt    |  132 |   156    |  164    |   172   |   180   |   188   |    X    |
 * ro4_ro_ind_40_ulvt  |  399 |   459    |  479    |   498   |   518   |   538   |         |
 * ro5_ro_ind_40_lvt   |  401 |   454    |  472    |   490   |   508   |   526   |         |
 * ro6_ro_ind_40_svt   |  214 |   253    |  267    |   280   |   293   |   307   |         |
 * ro7_ro_ind_40_hvt   |   77 |   101    |  109    |   116   |   124   |   132   |         |
 * ro8_ro_mux4_40_ulvt |   91 |   105    |  110    |   114   |   119   |   124   |         |
 * ro9_ro_nd3_40_ulvt  |  266 |   309    |  324    |   338   |   353   |   367   |         |
 * ro10_ro_nd3_40_lvt  |  269 |   308    |  321    |   334   |   347   |   360   |         |
 * ro11_ro_nd3_40_svt  |  137 |   163    |  171    |   180   |   188   |   197   |         |
 * ro12_ro_nd3_40_hvt  |   50 |    64    |   69    |    74   |    80   |    83   |    X    |
 * ro13_ro_nr_40_hvt   |   22 |    31    |   35    |    38   |    41   |    44   |         |
 * ro14_ro_or_40_lvt   |  194 |   228    |  240    |   251   |   263   |   274   |    X    |
 * ro15_ro_or_40_svt   |   82 |   102    |  109    |   116   |   125   |   130   |    X    |
 */
/* Target values for each ROs for CX+30mV */
#if (FERMION_CHIP_VERSION == 1)
#define CPR_RO0_TARGET  322
#define CPR_RO2_TARGET  200
#define CPR_RO3_TARGET  180
#define CPR_RO12_TARGET 80
#define CPR_RO14_TARGET 263
#define CPR_RO15_TARGET 125
#else /* FERMION_CHIP_VERSION */

/*
 *   UHD               | NTC  |  MX    | NTC+5mV | NTC+10mV | NTC+15mV | NTC+20mV | NTC+25mV | NTC+30mV | NTC+35mV |
 * NTC+40mV | NTC+45mV |Selection|
 * ------------------------------------------------------------------------------------------------------------------------------------------------
 * TT/CW corner voltage| 0.57 | 0.72SSG|
 * -----------------------------------------------------------------------------------------
 * mode                | NOM  |  NOM   |
 * ------------------------------------------------------------------------------------------------------------------------------------------------
 * ro0_ro_aoi_40_lvt   | 244  |   633  |  258    |   272    |   286    |   300    |   314    |   328    |    342    |
 * 356    |   370    |    X    | ro1_ro_aoi_40_svt   | 104  |   398  |  113    |   122    |   131    |   139    |   148
 * |   157    |    166    |   175    |   184    |    X    | ro2_ro_fa_40_ulvt   | 149  |   389  |  158    |   166    |
 * 175    |   184    |   193    |   202    |    211    |   220    |   229    |         | ro3_ro_fa_40_lvt    | 135  |
 * 359  |  143    |   151    |   159    |   167    |   175    |   184    |    192    |   200    |   208    |         |
 * ro4_ro_ind_40_ulvt  | 396  |   916  |  417    |   437    |   458    |   478    |   499    |   519    |    540    |
 * 560    |   581    |         | ro5_ro_ind_40_lvt   | 403  |   868  |  421    |   440    |   458    |   476    |   495
 * |   513    |    531    |   549    |   568    |         | ro6_ro_ind_40_svt   | 218  |   602  |  232    |   246    |
 * 259    |   273    |   287    |   301    |    314    |   328    |   342    |         | ro7_ro_ind_40_hvt   | 147  |
 * 674  |  161    |   175    |   188    |   202    |   216    |   230    |    243    |   257    |   271    |    X    |
 * ro8_ro_mux4_40_ulvt |  92  |   222  |   96    |   101    |   106    |   111    |   116    |   121    |    126    |
 * 131    |   136    |         | ro9_ro_nd3_40_ulvt  | 271  |   652  |  286    |   301    |   316    |   332    |   347
 * |   362    |    377    |   392    |   407    |         | ro10_ro_nd3_40_lvt  | 264  |   628  |  278    |   291    |
 * 304    |   317    |   331    |   344    |    357    |   370    |   384    |         | ro11_ro_nd3_40_svt  | 139  |
 * 407  |  148    |   157    |   165    |   174    |   183    |   191    |    200    |   208    |   217    |         |
 * ro12_ro_nd3_40_hvt  |  97  |   466  |  106    |   116    |   125    |   134    |   143    |   152    |    161    |
 * 170    |   179    |    X    | ro13_ro_nr_40_hvt   |  70  |   461  |   95    |   103    |   111    |   120    |   128
 * |   136    |    144    |   153    |   161    |    X    | ro14_ro_or_40_lvt   | 197  |   535  |  232    |   244    |
 * 256    |   267    |   279    |   291    |    302    |   314    |   326    |    X    | ro15_ro_or_40_svt   |  84  |
 * 331  |  105    |   112    |   119    |   127    |   134    |   141    |    148    |   155    |   162    |    X    |
 */

#ifdef CPR_2G_PER_ISSUE_WAR
#define CPR_RO0_TARGET  370
#define CPR_RO1_TARGET  184
#define CPR_RO7_TARGET  271
#define CPR_RO12_TARGET 179
#define CPR_RO13_TARGET 161
#define CPR_RO14_TARGET 326
#define CPR_RO15_TARGET 162
#else /* CPR_2G_PER_ISSUE_WAR */
#define CPR_RO0_TARGET  314
#define CPR_RO1_TARGET  148
#define CPR_RO7_TARGET  216
#define CPR_RO12_TARGET 143
#define CPR_RO13_TARGET 128
#define CPR_RO14_TARGET 279
#define CPR_RO15_TARGET 134
#endif  /* CPR_2G_PER_ISSUE_WAR */
#endif  //(FERMION_CHIP_VERSION == 1)

#define CPR_NTC_PLUS_65

#if defined(CPR_NTC_PLUS_25) || defined(CPR_NTC_PLUS_45) || defined(CPR_NTC_PLUS_65)
#undef CPR_RO0_TARGET
#undef CPR_RO1_TARGET
#undef CPR_RO7_TARGET
#undef CPR_RO12_TARGET
#undef CPR_RO13_TARGET
#undef CPR_RO14_TARGET
#undef CPR_RO15_TARGET

#ifdef CPR_NTC_PLUS_25
#define CPR_RO0_TARGET  314
#define CPR_RO1_TARGET  148
#define CPR_RO7_TARGET  216
#define CPR_RO12_TARGET 143
#define CPR_RO13_TARGET 121
#define CPR_RO14_TARGET 269
#define CPR_RO15_TARGET 127
#endif  //#ifdef CPR_NTC_PLUS_25

#ifdef CPR_NTC_PLUS_45
#define CPR_RO0_TARGET  370
#define CPR_RO1_TARGET  184
#define CPR_RO7_TARGET  271
#define CPR_RO12_TARGET 179
#define CPR_RO13_TARGET 161
#define CPR_RO14_TARGET 326
#define CPR_RO15_TARGET 162
#endif  //#ifdef CPR_NTC_PLUS_45

#ifdef CPR_NTC_PLUS_65
#define CPR_RO0_TARGET  426
#define CPR_RO1_TARGET  220
#define CPR_RO7_TARGET  326
#define CPR_RO12_TARGET 215
#define CPR_RO13_TARGET 201
#define CPR_RO14_TARGET 383
#define CPR_RO15_TARGET 197
#endif  //#ifdef CPR_NTC_PLUS_65
#endif  //#if define(CPR_NTC_PLUS_25) || define(CPR_NTC_PLUS_45) || define(CPR_NTC_PLUS_65)

/* Structure to store frequently used CPR parameters
   in global structure g_socpm_struct */
typedef struct {
    uint32_t ini_enabled;
    uint32_t otp_tag_high;
    uint32_t otp_tag_low;
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
