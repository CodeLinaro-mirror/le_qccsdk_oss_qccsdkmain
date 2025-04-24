/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
*/

/**
   @file qapi_lowpower.h

   @brief Lowpower definitions

   @details This file provides lowpower definitions and APIs.
*/

/*-------------------------------------------------------------------------
 * Include Files
 *-----------------------------------------------------------------------*/
#include "qapi_types.h"
#include "qapi_status.h"
#include "wlan_drv.h"
#include "wmi_api.h"
#include "wifi_fw_pwr_cb_infra.h"

//Sleep modes types
typedef enum qapi_sleep_types {
   qapi_clk_gtd_sleep = 1, qapi_mcu_sleep, qapi_standby,qapi_active,qapi_lightsleep,qapi_infdeepsleep 
} qapi_sleep_mode;


/* @brief bmps rx filter callback typedef */
typedef bool (*qapi_bmps_rx_filter_cb)(uint16_t type, bool bm_cast,void* pbuf,uint16_t len);

/**
   @brief Enable/Disable system power management.

   The API enable/disable system power management.

   @param[in] enable  Enable system power management. 1: To enable; 0: to disable;

   @return
   - QAPI_OK                             --  Enable/Disable system power management successfully.
*/
qapi_Status_t qapi_pm_enable(uint8_t enable);

/**
   @brief Put system into deepsleep directly.

   The API put system into deepsleep directly.

   @param[in] wkup_src    Wakeup source: 1 for AON timer; 2 for external wakup source;
   @param[in] sleep_time  Sleep time in us, only valid when wkup_src is set 1;

   @return
   - QAPI_OK                             --  system entering into deepsleep successfully.
*/
qapi_Status_t qapi_deepsleep_enter(uint8_t wkup_src, uint64_t sleep_time);

/**
   @brief Config and enable IMPS.

   The API config and enable IMPS (using deepsleep with AON timer as wkup source).

   @param[in] enable        1: Enable; 0: disable. Below parameters are valid only when enable is 1;
   @param[in] sleep_time  Sleep time in ms, during deepsleep state;
   @param[in] recnx_wait  Re-connection timeout in ms. When wlan disconnect/connect_fail happens, this timer will start; if connect success happens then cancel the timer; if timeout, system will determine whether to enter into deepsleep;
   @param[in] wmi_wait    Wmi_wait time in ms. Upon recnx_wait timeout, check if there's any WMI cmd received during the wmi_wait duration, if no then goto deepsleep, if yes then start a timer with wmi_wait duration;
   @param[in] cnx_wait     Time in ms. Use for ENABLE_IMPS_TIMER_ON_BOOTUP feature, means starting this timer during bootup, if there's no wlan connection during this period, then system enters into deepsleep;

   @return
   - QAPI_OK                             --  IMPS cfg and enable successfully.
*/
qapi_Status_t qapi_imps_cfg(uint8_t enable, uint32_t sleep_time, uint32_t recnx_wait, uint32_t wmi_wait, uint32_t cnx_wait, qapi_sleep_mode policy);


/**
   @brief Config and enable IMPS.

   The API config and enable IMPS (using deepsleep with AON timer as wkup source).

   @param[in] enable        1: Enable; 0: disable. Below parameters are valid only when enable is 1;
   @param[in] sleep_time  Sleep time in ms, during deepsleep state;
   @param[in] recnx_wait  Re-connection timeout in ms. When wlan disconnect/connect_fail happens, this timer will start; if connect success happens then cancel the timer; if timeout, system will determine whether to enter into deepsleep;
   @param[in] wmi_wait    Wmi_wait time in ms. Upon recnx_wait timeout, check if there's any WMI cmd received during the wmi_wait duration, if no then goto deepsleep, if yes then start a timer with wmi_wait duration;
   @param[in] cnx_wait     Time in ms. Use for ENABLE_IMPS_TIMER_ON_BOOTUP feature, means starting this timer during bootup, if there's no wlan connection during this period, then system enters into deepsleep;

   @return
   - QAPI_OK                             --  IMPS cfg and enable successfully.
*/
qapi_Status_t qapi_imps_enter_sleep(uint8_t enable,uint32_t wait_time,uint32_t sleep_time);

/**
   @brief disenable IMPS.

   The API disenable IMPS .

   @return
   - QAPI_OK                             --  IMPS  Disenable successfully.
*/
qapi_Status_t qapi_imps_disable_sleep(void);

/**
   @brief Config and enable/disable BMPS.

   The API config and enable/disable BMPS.

   @param[in] enable          1: Enable; 0: disable;
   @param[in] idle_timeout  Idle timeout value in ms. When BMPS is enabled, system would start a timer with idle_timeout as timeout value, after timer expires, it'll check tx/rx cnt during this period, if meet condition then trigger system entering into BMPS, otherwise re-start the idle timer again;

   @return
   - QAPI_OK                             --  BMPS cfg and enable/disable successfully.
*/
qapi_Status_t qapi_bmps_cfg(uint8_t enable, uint32_t idle_timeout);

/**
   @brief Config and enable/disable BMPS RX Filter.

   The API config and enable/disable BMPS RX Filter.

   @param[in] enable          1: Enable; 0: disable;

   @return
   - QAPI_OK                             --  BMPS  RX Filter enable/disable successfully.
*/
qapi_Status_t  qapi_bmps_rx_filter_enable(uint8_t enable);



/**
   @brief Register the rx filter callback function for bmps for broadcast/multicast packets.

   @param[in] bmps_cb  callback function used in bmps mode, used as filter to ignore some broadcast/multicast packets that will not wake up chip;
   @param[in] net_cb  callback function used in net stack, could be NULL;

   @return
   - QAPI_OK                             --  BMPS  RX Filter function register successfully.
*/
qapi_Status_t qapi_bmps_bcmc_rx_filter_cb_register(qapi_bmps_rx_filter_cb bmps_cb, qapi_bmps_rx_filter_cb net_cb);

/**
   @brief Register the callback function for bmps for pre-sleep/post-awake.

   @param[in] cb  callback function used for calling when bmps pre-sleep/post-awake.
   @param[in] flag  flag to register/deregister

   @return
   - QAPI_OK                             --  BMPS  register callback successfully.
*/
qapi_Status_t qapi_bmps_sleep_wakeup_cb(ps_evt_cb_t cb, uint8_t flag);

/**
   @brief obtain exit reason of bmps

   @param[in] *reason  Pointer of exit reason to obtain
   @return
   - QAPI_OK                             --   valid pointer.
*/
qapi_Status_t qapi_bmps_get_exit_reason(uint8_t *reason);