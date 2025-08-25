/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "Osal.h"
#include "nt_hw.h"
#include "nt_crypto.h"
#include "hal_int_sys.h"
#include "hal_int_modules.h"
#include "wlan_power.h"

nt_osal_semaphore_handle_t SecipPdHandle = NULL;

#if 0
#if defined(WIFI_HW_AES) || defined(WIFI_HW_AES_CCM) || defined(NT_FN_HW_CRYPTO)

typedef struct crypto_security_s
{
    uint8_t crypto_hw_power_request;
} wifi_crypto_svc_t;

wifi_crypto_svc_t g_crypto_sec;
/**
 * @brief API to request for crypto HW
 * @param enable_secip_module: 1 when HW crypto is requried and 0 when uisage done
 * @return void, the global variable for number requests will be updated
*/
void wifi_crypto_secip_request(uint8_t enable_secip_module)
{
    if(SecipPdHandle==NULL)
    {
        nt_osal_semaphore_create_binary(SecipPdHandle);
	if(NULL==SecipPdHandle)
	{
            NT_LOG_PRINT(HAL, ERR, "secip semaphore creation failed!!");
	}
    }

    if(nt_fail == nt_osal_semaphore_take(SecipPdHandle, portMAX_DELAY))
        {
                NT_LOG_HAL_ERR("Secip Semaphore take failed", 0, 0, 0);
        }

    if (enable_secip_module)
    {
        if (g_crypto_sec.crypto_hw_power_request == 0)
        {
            NT_LOG_PRINT(HAL, INFO, "Powering ON secip");
            g_crypto_sec.crypto_hw_power_request = 1;
            nt_pm_process_event(gdevp, PM_EVENT_RRT_CHANGE);
            hal_secip_sw_power_req(1);
        }
        else
        {
            NT_LOG_PRINT(HAL, INFO, "Multiple Secip module ON request: %d", g_crypto_sec.crypto_hw_power_request);
            g_crypto_sec.crypto_hw_power_request++;
        }
    }
    else
    {
        if (g_crypto_sec.crypto_hw_power_request == 1)
        {
            NT_LOG_PRINT(HAL, INFO, "Powering OFF secip");
            g_crypto_sec.crypto_hw_power_request = 0;
            nt_pm_process_event(gdevp, PM_EVENT_RRT_CHANGE);
            hal_secip_sw_power_req(0);
        }
        else if (g_crypto_sec.crypto_hw_power_request > 1)
        {
            NT_LOG_PRINT(HAL, INFO, "Multiple Secip module OFF request, previos user has not turned off: %d", g_crypto_sec.crypto_hw_power_request);
            g_crypto_sec.crypto_hw_power_request--;
        }
    }

    if(nt_fail == nt_osal_semaphore_give(SecipPdHandle))
    {
	NT_LOG_HAL_ERR("Secip semaphore give failed",0,0,0);
    }
}
/**
 * @brief API to initilise global variable to manage crypto HW users
 * @return None
*/
void wifi_crypto_svc_hw_init()
{
    g_crypto_sec.crypto_hw_power_request = 0;
}

#endif
#endif

#ifdef NT_FN_HW_CRYPTO

int8_t nt_secure_ip_pwr_status(void)
{
    if (HAL_REG_RD(QWLAN_PMU_SECIP_GDSCR_REG) & QWLAN_PMU_SECIP_GDSCR_GDS_CTL_PWR_STATUS_MASK) {
        HAL_DBG_PRINT("Security IP module is ON", 0, 0, 0);
        return 0;
    } else {
        HAL_DBG_PRINT("Security IP module is OFF and powering ON..", 0, 0, 0);
        uint32_t boot_cmpl_reg = 0;
        boot_cmpl_reg = HAL_REG_RD(QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_REG);
        HAL_REG_WR(QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_REG,
                   (boot_cmpl_reg |
                    QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_PD_SECIP_CNTL_BIT_MASK));

        // delay
        nt_normal_delay(10);

        // again check gdscr
        if (HAL_REG_RD(QWLAN_PMU_SECIP_GDSCR_REG) & QWLAN_PMU_SECIP_GDSCR_GDS_CTL_PWR_STATUS_MASK) {
            HAL_DBG_PRINT("Security IP module is ON", 0, 0, 0);
            return 0;
        }
    }

    return 0;
}

#endif
