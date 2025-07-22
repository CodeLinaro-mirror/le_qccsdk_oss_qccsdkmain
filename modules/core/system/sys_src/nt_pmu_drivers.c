/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include "nt_pmu_drivers.h"
#include "unpa_resource.h"
#include "unpa_internal.h"
#include "nt_wifi_driver.h"
#ifdef NT_SOPCM_CHANGE
unpa_client *wifi_power_handler;

unpa_resource_state wifi_pwr_update_fcn(unpa_resource *resource, unpa_client *client);
static unpa_resource_state wifi_pwr_driver_fcn(unpa_resource *resource, unpa_client *client, unpa_resource_state state);
unpa_resource *create_wifi_resources(void);

static unpa_resource_definition wifi_resource_defn = {
    "wifi",
    wifi_pwr_update_fcn,
    wifi_pwr_driver_fcn,
    WIFI_MAX_STATE,
    UNPA_CLIENT_REQUIRED | UNPA_CLIENT_SUPPRESSIBLE | UNPA_CLIENT_SLEEP,
    WIFI_SLEEP_DEFAULT};

// non shareable resources
unpa_resource *create_wifi_resources(void)
{
    unpa_resource *wifi_power_handler;
    wifi_power_handler = unpa_create_resource(&wifi_resource_defn, WIFI_SLEEP_DEFAULT);
    return wifi_power_handler;
}

// non shareable resources
error_list wifi_pdc_init(void)
{
    unpa_resource *result = NULL;
    result = create_wifi_resources();
    if (result != NULL) {
        wifi_power_handler = unpa_create_client("wifislp", UNPA_CLIENT_SLEEP, "wifi");
    } else
        return resource_creation_failed;
    if (wifi_power_handler != NULL)
        return pdc_init_success;
    return client_create_fail;
}

/**
 * <!-- wifi update function -->
 * @brief Returns the maximum of all active requests.
 */
unpa_resource_state wifi_pwr_update_fcn(unpa_resource *resource, unpa_client *client)
{
    uint32_t index;
    unpa_resource_state active_max, pending_max, request;

    switch (client->type) {
        case UNPA_CLIENT_SUPPRESSIBLE:
        case UNPA_CLIENT_SLEEP:
            index = unpa_get_agg_index(client);

            pending_max = active_max = resource->agg_state[index];

            request = UNPA_PENDING_REQUEST(client).val;

            if (request > pending_max) {
                pending_max = request;
            } else if (active_max == UNPA_ACTIVE_REQUEST(client).val) {
                /* Walk the client list for a new max */
                unpa_client *c = resource->clients;
                pending_max = request;

                do {
                    if (c != client && c->type == client->type) {
                        request = UNPA_ACTIVE_REQUEST(c).val;
                        if (request > pending_max) {
                            pending_max = request;
                        }
                    }
                    c = c->next;
                } while (c);
            }

            resource->agg_state[index] = pending_max;
            break;

        default:
            break;
    }

    /* Compute sleep_state */
    resource->sleep_state = MAX(resource->agg_state[UNPA_REQUIRED_INDEX], resource->agg_state[UNPA_SLEEP_INDEX]);

    /* Return active_agg */
    return MAX(resource->agg_state[UNPA_SLEEP_INDEX], resource->agg_state[UNPA_SUPPRESSIBLE_INDEX]);
}

/**
 * <!-- wifi Power driver fcn -->
 *
 * @brief Driver function of the "wifi" resource.
 */
static unpa_resource_state wifi_pwr_driver_fcn(unpa_resource *resource, unpa_client *client, unpa_resource_state state)
{
    switch (client->type) {
        case UNPA_CLIENT_SUPPRESSIBLE:
            break;
        case UNPA_CLIENT_SLEEP:  // nt_hw_modify_reg(QWLAN_PMU_CFG_AON_CNTL_WIFI_SLEEP_STATE_RESOURCE_REQ_REG,0);
            // nt_hw_modify_reg(QWLAN_PMU_CFG_AON_CNTL_WUR_SLEEP_STATE_RESOURCE_REQ_REG,0);
            nt_hw_modify_reg(QWLAN_PMU_CFG_WUR_SLEEP_STATE_RESOURCE_REQ_REG, ((resource->sleep_state << 6)),
                             WIFI_WUR_CONFIG_MASK);
            nt_hw_modify_reg(QWLAN_PMU_CFG_WIFI_SLEEP_STATE_RESOURCE_REQ_REG, ((resource->sleep_state << 6)),
                             WIFI_WUR_CONFIG_MASK);
            HAL_REG_WR(QWLAN_PMU_CFG_WIFI_SS_STATE_REG, WIFI_OFF);
            HAL_REG_WR(QWLAN_PMU_CFG_WUR_SS_STATE_REG, WUR_SLEEP);
            nt_socpm_sleep_t *Temp;
            Temp = (nt_socpm_sleep_t *)client->resource_data;
            Temp->list_no = nt_socpm_sleep_register(Temp, Temp->list_no);

            break;
    }
    return state;
}
#endif
