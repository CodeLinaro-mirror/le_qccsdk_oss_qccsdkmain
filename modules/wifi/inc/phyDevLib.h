/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#if !defined(_PHY_DEVLIB_H)
#define _PHY_DEVLIB_H

/*!
  @file phyvDevLib.h

  @brief
  PHY devLib header file (not the devLibApi.h)



*/

/*! @brief Dev Guide
 *
 * For a different product,
 *     create a product specific phy dev lib
 *     create a initDevLib() func which
 *         binds the dev lib
 */

#if defined(PHYDEVLIB_PRODUCT_NEUTRINO)
#include "phyDevLibNeutrino.h"
#elif defined(PHYDEVLIB_PRODUCT_FERMION)
#include "phyDevLibFermion.h"
#else
#error "NO PHYDEVLIB_PRODUCT defined"
#endif

#include "phyDevLibApiDef.h"
#include "phyDevLibApiParm.h"
#include "phyDevLibApi.h"

/*! @brief clone PHYRF_HANDLE to leverage Halphy code
 *  It's deliberate that no global/static handles are allocated, as phyDevLib is a library
 *  the state data of phyHandles should be managed by clients of phyDevLib such as Q5, Halphy, VI MAC, etc.
 */
typedef struct {
    uint32_t wlan_base;
    uint32_t wcss_env;
} PHY_HANDLE;

#if defined(PHYDEVLIB_IOT)
typedef struct {
    RECIPE_RC (*phyReset)(void *phy_input, void *reset_input, void *reset_output);
    RECIPE_RC (*phyGetHandle)(void *phy_input, void **pHandle);
    RECIPE_RC (*phyChannelSwitch)(void *phy_input, void *channel_switch_input, void *channel_switch_output);
    RECIPE_RC (*phySetParam)(void *phy_input, uint32_t param_id, void *param);
    RECIPE_RC (*phyGetParam)(void *phy_input, uint32_t param_id, void *param);
    RECIPE_RC (*phyForcedGainMode)(void *phy_input, void *forceGainModeInput, void *forceGainModeOutput);
    RECIPE_RC (*phyFTPGRx)(void *phy_input, void *ftpg_rx_input, void *ftpg_rx_output);
    RECIPE_RC (*phyFTPGTx)(void *phy_input, void *ftpg_tx_input, void *ftpg_rx_output);
    RECIPE_RC (*phyRxDCOCal)(void *phy_input, void *rxdco_cal_input, void *rxdco_cal_output);
    RECIPE_RC (*phyDPDCal)(void *phy_input, void *dpd_cal_input, void *dpd_cal_output);
    RECIPE_RC (*phyWarmReset)(void *phy_input, void *warm_reset_input, void *warm_reset_output);
    RECIPE_RC (*phySpurMitigation)(void *phy_input, void *spur_mitigation_input, void *spur_mitigation_output);
} PHY_DEV_LIB;
#else
#error "PHYDEVLIB_IOT is not defined"
#endif

/*! @brief global declarations
 */
extern PHY_DEV_LIB *phyDevLib;

/*! @brief Macros
 */

#define PHY_DEVLIB_COMMON_FN(_f, _phy_in, _in, _out) _f(_phy_in, _in, _out)

#define PHY_DEVLIB_FN(_f, _phy_in, _in, _out)                                  \
    RECIPE_RC rc = RECIPE_NULL;                                                \
    if (phyDevLib) {                                                           \
        if (phyDevLib->phy##_f) {                                              \
            rc = PHY_DEVLIB_COMMON_FN(phyDevLib->phy##_f, _phy_in, _in, _out); \
        }                                                                      \
    }                                                                          \
    return (rc);

#define PHY_DEVLIB_FN2(_f, a, b)           \
    RECIPE_RC rc = RECIPE_NULL;            \
    if (phyDevLib) {                       \
        if (phyDevLib->phy##_f) {          \
            rc = phyDevLib->phy##_f(a, b); \
        }                                  \
    }                                      \
    return (rc);

#define PHY_DEVLIB_FN3(_f, _phy_in)           \
    RECIPE_RC rc = RECIPE_NULL;               \
    if (phyDevLib) {                          \
        if (phyDevLib->phy##_f) {             \
            rc = phyDevLib->phy##_f(_phy_in); \
        }                                     \
    }                                         \
    return (rc);

#define PHY_DEVLIB_FN4(_f, a, b, c, d, e)           \
    RECIPE_RC rc = RECIPE_NULL;                     \
    if (phyDevLib) {                                \
        if (phyDevLib->phy##_f) {                   \
            rc = phyDevLib->phy##_f(a, b, c, d, e); \
        }                                           \
    }                                               \
    return (rc);

/*
    this MACRO is designed specifically for cal function hookup
    in additional to calling the actualy cal function body
    cal result along with calCmdId are always printed post function execution
    for any of the CALs that hookup to this MACRO
    please ensure the first element of the input structure is always calCmdId
*/
#define PHY_DEVLIB_CAL_FN(_f, _phy_in, _in, _out)                                                        \
    RECIPE_RC rc = RECIPE_NULL;                                                                          \
    PHYDEVLIB_CAL_INPUT_COMMON_MAPPING *calInput = (PHYDEVLIB_CAL_INPUT_COMMON_MAPPING *)_in;            \
    if (phyDevLib) {                                                                                     \
        if (phyDevLib->phy##_f) {                                                                        \
            rc = PHY_DEVLIB_COMMON_FN(phyDevLib->phy##_f, _phy_in, _in, _out);                           \
            if (calInput) {                                                                              \
                phyLog("PDL_CAL_FUNC: func=phy" #_f " calCmdId=0x%x RC=0x%x\n", calInput->calCmdId, rc); \
            } else {                                                                                     \
                phyLog("PDL_CAL_FUNC: func=phy" #_f " calCmdId=NULL RC=0x%x\n", rc);                     \
            }                                                                                            \
        }                                                                                                \
    }                                                                                                    \
    return (rc);

#endif  ///#if !defined(_PHY_DEVLIB_H)
