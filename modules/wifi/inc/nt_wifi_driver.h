/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include "unpa.h"
#include "unpa_resource.h"
#include "nt_flags.h"

#ifndef CORE_SYSTEM_INC_NT_WIFI_DRIVER_H_
#define CORE_SYSTEM_INC_NT_WIFI_DRIVER_H_

#define SLEEP_STATE_REG_CONFIGURATION_AON \
    0xFFFFFFCE  // Mask for CFG_AON registers(wur sleep state and wifi sleep state registers)
#define WIFI_WUR_REG_CONFIGURATION 0xFFFFFFE1  // Mask to configure wur sleep state and wifi sleep state registers
#define CONFIG_SS_STATE            0x0  // Parameter to mask the WIFI_SS_STATE reg and WUR_SS_STATE reg to write the necessary bits
#define UNPA_WIFI_MAX_STATE        0x3FFF  // Parameter which determines how many bits we need to control on WiFi driver
#define UNPA_MEM_MAX_STATE         0xF  // Parameter which determines how many bits we need to control on memory driver
#define MEMORY_DRIVER_MASK         0x1  // Mask to determine whether the bit need to be turned On/Off
#define UNPA_WIFI_CFG_MAX_STATE    0xFF  // Parameter which determines how many bits we need to control for PHY calibration
#define WIFI_CFG_REG_MASK          0XFFFFF83F  // Mask for configuring wifi_config register(mac,phy_tx_phy_rx,phy_txtop,wur bits)
#define SHIFT_REQUESTED_REQ \
    0x6  // Shift the requested state 6 times to write it to the respective bit positions in the register

#define PHY_MAX_STATE         0x3      // Parameter which determines how many bits we need to control on PHY driver
#define PHY_DRIVER_MASK       0xFFFF9  // Mask to clear RFA_DTOP and XO_DTOP bits
#define NT_PDC_CPU_MAX_STATE  0x1
#define NT_PDC_RRAM_MAX_STATE 0x1
#define NT_PDC_XIP_MAX_STATE  0x1

#define SOC_POWER_DOMAIN_CLEAR_MASK 0xFFFFF83F  // To clear the power domain for PHY calibration
#define SOC_POWER_DOMAIN_CNTRL      0x1F        // to filter only the power domain from the requested state from PDC
#define SOC_POWER_DOMAIN_STATE      0xE0        // To filter only the state which should be configured from PDC request

/* Active and sleep only clients to the wifipwr resource  */
typedef struct {
    unpa_client *wifi_driver;  // Client handle for WiFi driver

    unpa_client *memdrv_act;          // Client handle for memory driver in MCU active state
    unpa_client *memdrv_slp;          // Client handle for memory driver in MCU sleep state
    unpa_client *phy_driver_act;      // Client handle for phy driver(active_state)
    unpa_client *phy_driver_slp;      // Client handle for phy driver(sleep_state)
    unpa_client *cpu_driver_mcu_slp;  // Client handle for to drive CPU power domain for MCU sleep state
    unpa_client *rram_driver_slp;     // Client handle for rram driver(sleep_state)
    unpa_client *xip_driver_slp;      // Client handle for xip driver (sleep state)
    unpa_client *xip_driver_dp_slp;   // Client handle for xip driver (deep sleep state)

    /* resource handle  */

    unpa_resource *resource_handle_wifi;         // Resource handle for WiFi driver
    unpa_resource *resource_handle_mem;          // Resource handle for memory driver
    unpa_resource *resource_handle_phy;          // Resource handle for phy driver
    unpa_resource *resource_handle_cpu_mcu_slp;  // Resource handle for phy driver
    unpa_resource *resource_handle_rram_slp;     // Resource handle for rram driver
    unpa_resource *resource_handle_xip_slp;      // Resource handle for xip sleep driver
    unpa_resource *resource_handle_xip_dp_slp;   // Resource handle for xip deep sleep driver
#ifdef NT_FN_SOCPM_CTRL
    unpa_client *wifi_cfg;                    // Client handle for wifi config
    unpa_resource *resource_handle_wifi_cfg;  // Resource handle for WiFi config
#endif                                        // NT_FN_SOCPM_CTRL
} wifi_driver_handle_t;

typedef enum {
    BIT_TURN_OFF,
    MAC_ON,
    WIFI_TX_ON = 2,
    WIFI_RX_ON = 4,
    RXTOP_ON = 8,
    WUR_ON = 16,
} wifi_config_t;

typedef enum { WIFI_CONFIG = 2, WIFI_TX_STATE, WIFI_RXA_STATE, WIFI_RXB_LISTEN } socpm_phy_state_t;

/**
 * <!-- nt_hw_modify_reg -->
 *
 * @brief Modify and write the required request into the registers
 * @param reg_addr: Register address
 * @param mask:     Mask to modify the request
 * @return current state
 */
uint32_t nt_hw_modify_reg(uint32_t reg_addr, uint32_t value, uint32_t mask);

/**
 * <!-- nt_pdc_create_resources -->
 *
 * @brief :  Create UNPA resources for WiFi,memory and cpu driver
 */
void nt_pdc_create_resources(void);

/**
 * <!-- nt_pdc_create_clients -->
 *
 * @brief : Create UNPA clients to the wifiwur and memdrvr resources
 */
void nt_pdc_create_clients(void);

/**
 * <!-- nt_pdc_driver_init -->
 *
 * @brief :  Initialize pdc driver resources and clients for wifi,memory and cpu. This function will initialize all the
 * resources and clients related to the PDC.
 */
void nt_pdc_driver_init(void);

#endif /* CORE_SYSTEM_INC_NT_WIFI_DRIVER_H_ */
