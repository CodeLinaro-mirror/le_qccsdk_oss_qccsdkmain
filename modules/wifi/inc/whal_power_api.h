/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _WHAL_POWER_API_H_
#define _WHAL_POWER_API_H_

#define _Bool bool

typedef enum {
    WHAL_PM_UNDEFINED = 0,
    WHAL_PM_AWAKE = 1,        /* fully wake up chip */
    WHAL_PM_FAKE_SLEEP = 2,   /* wake up chip but set PM on all self generat
e frame */
    WHAL_PM_FULL_SLEEP = 3,   /* fully sleep */
    WHAL_PM_NETWORK_SLEEP = 4 /* sleep but will listen for beacon*/
} WHAL_POWER_MODE;

typedef struct {
    WHAL_POWER_MODE (*GetPowerMode)();
    _Bool (*SetPowerMode)(WHAL_POWER_MODE);
    void (*SetBmissThreshMode)(uint16_t, uint16_t);
    void (*SetPowerModeAwake)();
    void (*SetPowerModeNetworkSleep)();
    void (*SetPowerModeSleep)();
    _Bool (*SetDataPmField)(uint16_t);
#if defined(CONFIG_WHAL_SM_PWR_SAVE_SUPPORT)
    void (*SmPsDisable)();
    void (*SmPsChangeChainmask)(_Bool lowPower, uint16_t chainmask);
    void (*SmPsEnableHwControl)(uint8_t lowPwrChainmask, uint8_t highPwrChainmask);
#endif /* CONFIG_WHAL_SM_PWR_SAVE_SUPPORT */
} WHAL_POWER_API;

#endif /* _WHAL_POWER_API_H_ */
