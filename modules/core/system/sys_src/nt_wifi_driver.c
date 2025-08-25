/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "unpa_resource.h"
#include "nt_hw.h"
#include "nt_wifi_driver.h"
#include "unpa_internal.h"
#include "unpa_resource.h"
#include "unpa.h"
#include "CoreVerify.h"
#include "qurt_mutex.h"
#include "FreeRTOS.h"
#include "nt_socpm_sleep.h"
#include "mlme_api.h"
#include "nt_common.h"
#include "wifi_cmn.h"
#include "fwconfig_cmn.h"
#include "nt_flags.h"

wifi_driver_handle_t handle;
#ifdef NT_FN_SOCPM_CTRL

static unpa_resource_state wificfg_driver_fcn(unpa_resource *resource, unpa_client *client, unpa_resource_state state);
static unpa_resource_state wificfg_update_fcn(unpa_resource *resource, unpa_client *client);

/* wificfg resource definition */
static unpa_resource_definition syspm_wifi_cfg_resource_defn = {
    "wificfg",
    wificfg_update_fcn,
    wificfg_driver_fcn,
    UNPA_WIFI_CFG_MAX_STATE,
    UNPA_CLIENT_REQUIRED | UNPA_CLIENT_SUPPRESSIBLE | UNPA_CLIENT_SLEEP,
    UNPA_RESOURCE_DEFAULT,
};

static unpa_resource_state wificfg_update_fcn(unpa_resource *resource, unpa_client *client)

{
    uint32_t index;
    unpa_client *c = resource->clients;

    switch (client->type) {
        case UNPA_CLIENT_REQUIRED:
        case UNPA_CLIENT_SUPPRESSIBLE:
        case UNPA_CLIENT_SLEEP:
            index = unpa_get_agg_index(client);

            resource->agg_state[index] = client->pending_request.val;

            do {
                if ((c != client) && (c->type) == (client->type)) {
                    resource->agg_state[index] |= c->active_request.val;
                }
                c = c->next;
            } while (c);

            break;

        default:
            break;
    }

    /* Compute sleep_state */
    resource->sleep_state = resource->agg_state[UNPA_REQUIRED_INDEX] | resource->agg_state[UNPA_SLEEP_INDEX];

    /* Return active_agg */

    return resource->sleep_state;
}

/**
 * <!-- wificfg_driver_fcn -->
 *
 * @brief Driver function for the "wificfg" resource
 * @param resource: Resource for the driver
 * @param client:   Client of the resource
 * @return return current state
 */
static unpa_resource_state wificfg_driver_fcn(unpa_resource *resource, unpa_client *client, unpa_resource_state state)
{
    char wifi_state = 0;
    char config_value = 0;
    uint32_t reg_read;
    CORE_VERIFY_PTR(client);
    CORE_VERIFY_PTR(resource);
    wifi_state = ((state & SOC_POWER_DOMAIN_STATE) >> 5);
    config_value = state & SOC_POWER_DOMAIN_CNTRL;

    switch (wifi_state) {
        case WIFI_CONFIG:
            reg_read = NT_REG_RD(QWLAN_PMU_CFG_WIFI_CONFIG_STATE_RESOURCE_REQ_REG);
            reg_read &= SOC_POWER_DOMAIN_CLEAR_MASK;
            NT_REG_WR(QWLAN_PMU_CFG_WIFI_CONFIG_STATE_RESOURCE_REQ_REG,
                      reg_read | (config_value << SHIFT_REQUESTED_REQ));
            break;
        case WIFI_TX_STATE:
            reg_read = NT_REG_RD(QWLAN_PMU_CFG_WIFI_TX_STATE_RESOURCE_REQ_REG);
            reg_read &= SOC_POWER_DOMAIN_CLEAR_MASK;
            NT_REG_WR(QWLAN_PMU_CFG_WIFI_TX_STATE_RESOURCE_REQ_REG, reg_read | (config_value << SHIFT_REQUESTED_REQ));
            break;
        case WIFI_RXA_STATE:
            reg_read = NT_REG_RD(QWLAN_PMU_CFG_WIFI_RXA_STATE_RESOURCE_REQ_REG);
            reg_read &= SOC_POWER_DOMAIN_CLEAR_MASK;
            NT_REG_WR(QWLAN_PMU_CFG_WIFI_RXA_STATE_RESOURCE_REQ_REG, reg_read | (config_value << SHIFT_REQUESTED_REQ));
            break;
        case WIFI_RXB_LISTEN:
            reg_read = NT_REG_RD(QWLAN_PMU_CFG_WIFI_RXB_LISTEN_STATE_RESOURCE_REQ_REG);
            reg_read &= SOC_POWER_DOMAIN_CLEAR_MASK;
            NT_REG_WR(QWLAN_PMU_CFG_WIFI_RXB_LISTEN_STATE_RESOURCE_REQ_REG,
                      reg_read | (config_value << SHIFT_REQUESTED_REQ));
            break;
        default:
            break;
    }
    NT_REG_WR(QWLAN_PMU_CFG_WIFI_SS_STATE_REG, wifi_state);
    return state;
}

#endif  // NT_FN_SOCPM_CTRL

/* Driver function declaration */
static unpa_resource_state wifiwur_driver_fcn(unpa_resource *resource, unpa_client *client, unpa_resource_state state);
static unpa_resource_state mem_driver_fcn(unpa_resource *resource, unpa_client *client, unpa_resource_state state);
static unpa_resource_state phy_driver_fcn(unpa_resource *resource, unpa_client *client, unpa_resource_state state);
static unpa_resource_state cpu_mcu_slp_driver_fcn(unpa_resource *resource, unpa_client *client,
                                                  unpa_resource_state state);

static unpa_resource_state rram_slp_driver_fcn(unpa_resource *resource, unpa_client *client, unpa_resource_state state);

static unpa_resource_state xip_slp_driver_fcn(unpa_resource *resource, unpa_client *client, unpa_resource_state state);

static unpa_resource_state xip_dp_slp_driver_fcn(unpa_resource *resource, unpa_client *client,
                                                 unpa_resource_state state);

/* Update function declaration */
static unpa_resource_state wifiwur_update_fcn(unpa_resource *resource, unpa_client *client);

static unpa_resource_state mem_update_fcn(unpa_resource *resource, unpa_client *client);

/* wifiwur resource definition */
static unpa_resource_definition syspm_wur_wifipwr_resource_defn = {
    "wifiwur",
    wifiwur_update_fcn,
    wifiwur_driver_fcn,
    UNPA_MEM_MAX_STATE,
    UNPA_CLIENT_REQUIRED | UNPA_CLIENT_SUPPRESSIBLE | UNPA_CLIENT_SLEEP,
    UNPA_RESOURCE_DEFAULT,
};

/* memdvr resource definition */
static unpa_resource_definition memorydriver_resource_defn = {
    "memdvr",
    mem_update_fcn,
    mem_driver_fcn,
    UNPA_WIFI_MAX_STATE,
    UNPA_CLIENT_REQUIRED | UNPA_CLIENT_SUPPRESSIBLE | UNPA_CLIENT_SLEEP,
    UNPA_RESOURCE_DEFAULT,
};

/* phydvr resource definition */
static unpa_resource_definition phydriver_resource_defn = {
    "phydvr",
    unpa_max_update_fcn,
    phy_driver_fcn,
    PHY_MAX_STATE,
    UNPA_CLIENT_REQUIRED | UNPA_CLIENT_SUPPRESSIBLE | UNPA_CLIENT_SLEEP,
    UNPA_RESOURCE_DEFAULT,
};

/* cpudvr resource definition for MCU sleep */
static unpa_resource_definition cpudriver_resource_defn = {
    "cpudvr",
    unpa_binary_update_fcn,
    cpu_mcu_slp_driver_fcn,
    NT_PDC_CPU_MAX_STATE,
    UNPA_CLIENT_REQUIRED | UNPA_CLIENT_SUPPRESSIBLE | UNPA_CLIENT_SLEEP,
    UNPA_RESOURCE_DEFAULT,
};

static unpa_resource_definition rramdriver_resource_defn = {
    "rramdvr",
    unpa_binary_update_fcn,
    rram_slp_driver_fcn,
    NT_PDC_RRAM_MAX_STATE,
    UNPA_CLIENT_REQUIRED | UNPA_CLIENT_SUPPRESSIBLE | UNPA_CLIENT_SLEEP,
    UNPA_RESOURCE_DEFAULT,
};

static unpa_resource_definition xipslpdriver_resource_defn = {
    "xpsdvr",
    unpa_binary_update_fcn,
    xip_slp_driver_fcn,
    NT_PDC_XIP_MAX_STATE,
    UNPA_CLIENT_REQUIRED | UNPA_CLIENT_SUPPRESSIBLE | UNPA_CLIENT_SLEEP,
    UNPA_RESOURCE_DEFAULT,
};
static unpa_resource_definition xipdpslpdriver_resource_defn = {
    "xpdsdvr",
    unpa_binary_update_fcn,
    xip_dp_slp_driver_fcn,
    NT_PDC_XIP_MAX_STATE,
    UNPA_CLIENT_REQUIRED | UNPA_CLIENT_SUPPRESSIBLE | UNPA_CLIENT_SLEEP,
    UNPA_RESOURCE_DEFAULT,
};

/**
 * <!-- wifiwur_update_fcn -->
 *
 * @brief Update function for the "wifiwur" resource
 * @param resource: Resource for the driver
 * @param client:   Client of the resource
 * @return required request state
 */

static unpa_resource_state wifiwur_update_fcn(unpa_resource *resource, unpa_client *client)

{
    uint32_t index;
    unpa_client *c = resource->clients;

    switch (client->type) {
        case UNPA_CLIENT_REQUIRED:
        case UNPA_CLIENT_SUPPRESSIBLE:
        case UNPA_CLIENT_SLEEP:
            index = unpa_get_agg_index(client);

            resource->agg_state[index] = client->pending_request.val;

            do {
                if ((c != client) && (c->type) == (client->type)) {
                    resource->agg_state[index] |= c->active_request.val;
                }
                c = c->next;
            } while (c);

            break;

        default:
            break;
    }

    /* Compute sleep_state */
    resource->sleep_state = resource->agg_state[UNPA_REQUIRED_INDEX] | resource->agg_state[UNPA_SLEEP_INDEX];

    /* Return active_agg */

    return resource->sleep_state;
}

/**
 * <!-- nt_hw_modify_reg -->
 *
 * @brief Modify and write the required request into the registers
 * @param reg_addr: Register address
 * @param mask:     Mask to modify the request
 * @return current state
 */

uint32_t nt_hw_modify_reg(uint32_t reg_addr, uint32_t request, uint32_t mask)
{
    uint32_t previous_value = 0;  //,Mask_bits = 0xFFFFFFFF

    // Read the existing configuration
    previous_value = (*((volatile uint32_t *)(reg_addr)));
    // Clear the bits Need to be written
    previous_value &= mask;
    // Insert the configuration in masked values;
    previous_value |= request;
    // Modify the Register
    (*((volatile uint32_t *)(reg_addr))) = ((uint32_t)(previous_value));
    // HAL_REG_WR(Reg_addr,Previous_value);
    return previous_value;
}

/**
 * <!-- wifiwur_driver_fcn -->
 *
 * @brief Driver function for the "wifiwur" resource
 * @param resource: Resource for the driver
 * @param client:   Client of the resource
 * @param state:    request which is to taken care of
 * @return required request state
 */

static unpa_resource_state wifiwur_driver_fcn(unpa_resource *resource, unpa_client *client, unpa_resource_state state)
{
    uint32_t State_Values[4] = {AON_CNTL_SLEEP_REG_WUR_AND_WIFI_TURN_OFF, CFG_WUR_SLEEP_STATE_RESOURCE_REQ_REG,
                                WUR_NEXT_SLEEP_STATE, WIFI_NEXT_SLEEP_STATE};
    configASSERT(client);

    if (state != resource->active_state) {
        if (state == State_Values[0]) {
            nt_hw_modify_reg(QWLAN_PMU_CFG_AON_CNTL_WUR_SLEEP_STATE_RESOURCE_REQ_REG, state,
                             SLEEP_STATE_REG_CONFIGURATION_AON);
            nt_hw_modify_reg(QWLAN_PMU_CFG_AON_CNTL_WIFI_SLEEP_STATE_RESOURCE_REQ_REG, state,
                             SLEEP_STATE_REG_CONFIGURATION_AON);
        } else if (state == State_Values[1]) {
            nt_hw_modify_reg(QWLAN_PMU_CFG_WUR_SLEEP_STATE_RESOURCE_REQ_REG, state, WIFI_WUR_REG_CONFIGURATION);
            nt_hw_modify_reg(QWLAN_PMU_CFG_WIFI_SLEEP_STATE_RESOURCE_REQ_REG, state, WIFI_WUR_REG_CONFIGURATION);
        } else if (state == State_Values[2]) {
            nt_hw_modify_reg(QWLAN_PMU_CFG_WUR_SS_STATE_REG, state, CONFIG_SS_STATE);
        } else if (state == State_Values[3]) {
            nt_hw_modify_reg(QWLAN_PMU_CFG_WIFI_SS_STATE_REG, state, CONFIG_SS_STATE);
        }
    }
    return state;
}

/**
 * <!-- mem_update_fcn -->
 *
 * @brief Update function for the "memdvr" resource
 * @param resource: Resource for the driver
 * @param client:   Client of the resource
 * @return required request state
 */

static unpa_resource_state mem_update_fcn(unpa_resource *resource, unpa_client *client)
{
    uint32_t index;
    unpa_client *c = resource->clients;

    switch (client->type) {
        case UNPA_CLIENT_SUPPRESSIBLE:

        case UNPA_CLIENT_SLEEP:
            index = unpa_get_agg_index(client);

            resource->agg_state[index] = client->pending_request.val;

            do {
                if ((c != client) && (c->type) == (client->type)) {
                    resource->agg_state[index] |= c->active_request.val;
                }
                c = c->next;
            } while (c);

            break;

        default:
            break;
    }

    /* Compute sleep_state */
    resource->sleep_state = resource->agg_state[UNPA_REQUIRED_INDEX] | resource->agg_state[UNPA_SLEEP_INDEX];

    /* Return active_agg */

    return resource->sleep_state;
}

/**
 * <!-- mem_driver_fcn -->
 *
 * @brief Driver function for the "memdvr" resource
 * @param resource: Resource for the driver
 * @param client:   Client of the resource
 * @param state:    request which is to taken care of
 * @return required request state
 */

static unpa_resource_state mem_driver_fcn(unpa_resource *resource, unpa_client *client, unpa_resource_state state)
{
    uint8_t count = 4;  // count to shift the request to check whether the 4 LSB bits are on/off
    uint8_t val = (uint8_t)state;
    uint8_t bank_number = 1; /* 1, Bank A
                                2, Bank B
                                3, Bank C
                                4, Bank D
                             */
    CORE_VERIFY_PTR(resource);
    if (client->type == UNPA_CLIENT_REQUIRED) {
        do {
            if (val & MEMORY_DRIVER_MASK) {
                // mem_bank_check(bank_number,On,Active);
            } else {
                // mem_bank_check(bank_number,Off,Active);
            }
            val = val >> 1;
            bank_number++;
        } while (count--);
    } else {
        do {
            if (val & MEMORY_DRIVER_MASK) {
                // mem_bank_check(bank_number,Retention,mcu_sleep);
            } else {
                // mem_bank_check(bank_number,Off,mcu_sleep);
            }
            val = val >> 1;
            bank_number++;
        } while (count--);
    }
    return state;
}

/**
 * <!-- phy_driver_fcn -->
 *
 * @brief Driver function for the "phyvr" resource. This driver function will control the bits RFA_DTOP and XO_DTOP
 * based on the user requirement
 * @param resource: Resource for the driver
 * @param client:   Client of the resource
 * @param state:    request which is to taken care of
 * @return required request state
 */
static unpa_resource_state phy_driver_fcn(unpa_resource *resource, unpa_client *client, unpa_resource_state state)
{
    uint16_t reg_read;

    if (client->type == UNPA_CLIENT_REQUIRED) {
        if (state != resource->active_state) {
            reg_read = (uint16_t)NT_REG_RD(QWLAN_PMU_CFG_AON_CNTL_MCU_ACTIVE_STATE_RESOURCE_REQ_REG);
            reg_read &= (uint16_t)PHY_DRIVER_MASK;
            // NT_REG_WR(QWLAN_PMU_CFG_AON_CNTL_MCU_ACTIVE_STATE_RESOURCE_REQ_REG, (reg_read|(state<<1)));
        }
    } else if (client->type == UNPA_CLIENT_SLEEP) {
        if (state != resource->sleep_state) {
            reg_read = (uint16_t)NT_REG_RD(QWLAN_PMU_CFG_AON_CNTL_MCU_SLEEP_STATE_RESOURCE_REQ_REG);
            reg_read &= (uint16_t)PHY_DRIVER_MASK;
            // NT_REG_WR(QWLAN_PMU_CFG_AON_CNTL_MCU_SLEEP_STATE_RESOURCE_REQ_REG,
            // (reg_read|((resource->sleep_state)<<1)));
            state = resource->sleep_state;
        }
    }
    return state;
}

/*CPU driver function for MCU sleep resource clients*/
static unpa_resource_state cpu_mcu_slp_driver_fcn(unpa_resource *resource, unpa_client *client,
                                                  unpa_resource_state state)
{
    uint32_t reg_read;
    CORE_VERIFY_PTR(client);
    CORE_VERIFY_PTR(resource);
    reg_read = NT_REG_RD(QWLAN_PMU_CFG_MCU_SLEEP_STATE_RESOURCE_REQ_REG);
    reg_read &= ~QWLAN_PMU_CFG_MCU_SLEEP_STATE_RESOURCE_REQ_PD_CMNSS_CNTL_BIT_MASK;
    // NT_REG_WR(QWLAN_PMU_CFG_MCU_SLEEP_STATE_RESOURCE_REQ_REG, (reg_read | state));

    return state;
}

static unpa_resource_state rram_slp_driver_fcn(unpa_resource *resource, unpa_client *client, unpa_resource_state state)
{
    uint32_t reg_read;
    CORE_VERIFY_PTR(client);
    CORE_VERIFY_PTR(resource);
    if (state == 1) {
        reg_read = NT_REG_RD(QWLAN_PMU_DIG_TOP_CFG_REG);
        reg_read &= ~QWLAN_PMU_DIG_TOP_CFG_RRAM_PD_MODE_MASK;
        NT_REG_WR(QWLAN_PMU_DIG_TOP_CFG_REG, (reg_read | 0b10 << 11));
    } else {
        reg_read = NT_REG_RD(QWLAN_PMU_DIG_TOP_CFG_REG);
        reg_read &= ~QWLAN_PMU_DIG_TOP_CFG_RRAM_PD_MODE_MASK;
        NT_REG_WR(QWLAN_PMU_DIG_TOP_CFG_REG, reg_read);
    }
    return state;
}

static unpa_resource_state xip_slp_driver_fcn(unpa_resource *resource, unpa_client *client, unpa_resource_state state)
{
    uint32_t reg_read;
    CORE_VERIFY_PTR(client);
    CORE_VERIFY_PTR(resource);
    if (state == 1) {
        reg_read = NT_REG_RD(QWLAN_PMU_CFG_MCU_SLEEP_STATE_RESOURCE_REQ_REG);
        reg_read &= ~QWLAN_PMU_CFG_MCU_SLEEP_STATE_RESOURCE_REQ_PD_XIP_CNTL_BIT_MASK;
        // NT_REG_WR(QWLAN_PMU_CFG_MCU_SLEEP_STATE_RESOURCE_REQ_REG,reg_read);
    } else {
        reg_read = NT_REG_RD(QWLAN_PMU_CFG_MCU_SLEEP_STATE_RESOURCE_REQ_REG);
        reg_read |= QWLAN_PMU_CFG_MCU_SLEEP_STATE_RESOURCE_REQ_PD_XIP_CNTL_BIT_MASK;
        // NT_REG_WR(QWLAN_PMU_CFG_MCU_SLEEP_STATE_RESOURCE_REQ_REG,reg_read);
    }
    return state;
}

static unpa_resource_state xip_dp_slp_driver_fcn(unpa_resource *resource, unpa_client *client,
                                                 unpa_resource_state state)
{
    uint32_t reg_read;
    CORE_VERIFY_PTR(client);
    CORE_VERIFY_PTR(resource);
    if (state == 1) {
        reg_read = NT_REG_RD(QWLAN_PMU_CFG_MCU_DEEPSLEEP_STATE_RESOURCE_REQ_REG);
        reg_read &= ~QWLAN_PMU_CFG_MCU_DEEPSLEEP_STATE_RESOURCE_REQ_PD_XIP_CNTL_BIT_MASK;
        // NT_REG_WR(QWLAN_PMU_CFG_MCU_DEEPSLEEP_STATE_RESOURCE_REQ_REG,reg_read);
    } else {
        reg_read = NT_REG_RD(QWLAN_PMU_CFG_MCU_DEEPSLEEP_STATE_RESOURCE_REQ_REG);
        reg_read |= QWLAN_PMU_CFG_MCU_DEEPSLEEP_STATE_RESOURCE_REQ_PD_XIP_CNTL_BIT_MASK;
        // NT_REG_WR(QWLAN_PMU_CFG_MCU_DEEPSLEEP_STATE_RESOURCE_REQ_REG,reg_read);
    }
    return state;
}

/**
 * <!-- nt_pdc_driver_init -->
 *
 * @brief :  Initialize pdc driver resources and clients for wifi,memory and cpu. This function will initialize all the
 * resources and clients related to the PDC.
 */
void nt_pdc_driver_init(void)
{
    nt_pdc_create_resources();
    nt_pdc_create_clients();
}

/**
 * <!-- nt_pdc_create_resources -->
 *
 * @brief :  Create UNPA resources for WiFi,memory and cpu driver
 */

void nt_pdc_create_resources(void)
{
    handle.resource_handle_wifi = unpa_create_resource(&syspm_wur_wifipwr_resource_defn, 0);
    CORE_VERIFY_PTR(handle.resource_handle_wifi);

    handle.resource_handle_mem = unpa_create_resource(&memorydriver_resource_defn, 0);
    CORE_VERIFY_PTR(handle.resource_handle_mem);

    handle.resource_handle_phy = unpa_create_resource(&phydriver_resource_defn, 0);
    CORE_VERIFY_PTR(handle.resource_handle_phy);

    handle.resource_handle_cpu_mcu_slp = unpa_create_resource(&cpudriver_resource_defn, 0);
    CORE_VERIFY_PTR(handle.resource_handle_cpu_mcu_slp);

    handle.resource_handle_rram_slp = unpa_create_resource(&rramdriver_resource_defn, 0);
    CORE_VERIFY_PTR(handle.resource_handle_rram_slp);

    handle.resource_handle_xip_slp = unpa_create_resource(&xipslpdriver_resource_defn, 0);
    CORE_VERIFY_PTR(handle.resource_handle_xip_slp);

    handle.resource_handle_xip_dp_slp = unpa_create_resource(&xipdpslpdriver_resource_defn, 0);
    CORE_VERIFY_PTR(handle.resource_handle_xip_dp_slp);

#ifdef NT_FN_SOCPM_CTRL
    handle.resource_handle_wifi_cfg = unpa_create_resource(&syspm_wifi_cfg_resource_defn, 0);
    CORE_VERIFY_PTR(handle.resource_handle_wifi_cfg);
#endif  // NT_FN_SOCPM_CTRL
}

/**
 * <!-- nt_pdc_create_clients -->
 *
 * @brief : Create UNPA clients to the wifiwur and memdrvr resources
 */

void nt_pdc_create_clients()
{
    handle.wifi_driver = unpa_create_client("wur_act", UNPA_CLIENT_SLEEP, "wifiwur");
    CORE_VERIFY_PTR(handle.wifi_driver);

    handle.memdrv_act = unpa_create_client("mem_act", UNPA_CLIENT_REQUIRED, "memdrvr");
    CORE_VERIFY_PTR(handle.memdrv_act);

    handle.memdrv_slp = unpa_create_client("mem_slp", UNPA_CLIENT_SLEEP, "memdrvr");
    CORE_VERIFY_PTR(handle.memdrv_slp);

    handle.phy_driver_act = unpa_create_client("phy_act", UNPA_CLIENT_REQUIRED, "phydvr");
    CORE_VERIFY_PTR(handle.phy_driver_act);

    handle.phy_driver_slp = unpa_create_client("phy_slp", UNPA_CLIENT_SLEEP, "phydvr");
    CORE_VERIFY_PTR(handle.phy_driver_slp);

    handle.cpu_driver_mcu_slp = unpa_create_client("cpu_slp", UNPA_CLIENT_SLEEP, "cpudvr");
    CORE_VERIFY_PTR(handle.phy_driver_slp);

    handle.rram_driver_slp = unpa_create_client("rram_dvr", UNPA_CLIENT_SUPPRESSIBLE, "rramdvr");
    CORE_VERIFY_PTR(handle.rram_driver_slp);

    handle.xip_driver_slp = unpa_create_client("xip_slp", UNPA_CLIENT_SUPPRESSIBLE, "xpsdvr");
    CORE_VERIFY_PTR(handle.xip_driver_slp);

    handle.xip_driver_dp_slp = unpa_create_client("xip_d_slp", UNPA_CLIENT_SUPPRESSIBLE, "xpddvr");
    CORE_VERIFY_PTR(handle.xip_driver_dp_slp);

#ifdef NT_FN_SOCPM_CTRL
    handle.wifi_cfg = unpa_create_client("wificfg", UNPA_CLIENT_SLEEP, "wificfg");
    CORE_VERIFY_PTR(handle.wifi_cfg);
#endif  // NT_FN_SOCPM_CTRL
}
