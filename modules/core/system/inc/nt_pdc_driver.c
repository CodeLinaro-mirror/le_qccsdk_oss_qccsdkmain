/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*-----------------------------------------------Header Files--------------------------------------------------*/
#include "nt_pdc.h"
#include "nt_hw.h"
#include "nt_common.h"
#include "nt_pdc_driver.h"
#include "nt_logger_api.h"
/*-------------------------------------------------------------------------------------------------------------*/

/*--------------------------------------------------------Vote(frame)-------------------------------------------------------------------
______________________________________________________________________________________________________________________________________________________________________________________________
|_________GPIO_____________|__________SPI_____________|__________I2C_____________|__________UART____________|__________SECIP___________|____________XIP___________|___________RRAM___________|
|Don't care bit | ctrl bit |Don't care bit | ctrl bit |Don't care bit | ctrl bit |Don't care bit | ctrl bit |Don't care
bit | ctrl bit |Don't care bit | ctrl bit |Don't care bit | ctrl bit |
----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

*/
#ifdef NT_FN_PDC_
/*-------------------------------------------------Macros------------------------------------------------------*/
#define NT_PDC_CONTROL_BIT_MASK   0x01  // To check the control bit of sub frame
#define NT_PDC_DONT_CARE_BIT_MASK 0x02  // To check the dont care bit of sub frame
#define NT_PDC_SUB_FRAME_MASK \
    0x03  // Extract sub frame(control bit and dont care bit) from issued vote(frame) for respective domains

#define NT_PDC_RRAM_DOMAIN_MASK  0x0  // mask to get RRAM sub frame from issued vote (frame)
#define NT_PDC_XIP_DOMAIN_MASK   0x2  // mask to get XIP sub frame from issued vote (frame)
#define NT_PDC_SECIP_DOMAIN_MASK 0x4  // mask to get SECIP sub frame from issued vote (frame)
#define NT_PDC_UART_DOMAIN_MASK  0x6  // mask to get UART sub frame from issued vote (frame)
#define NT_PDC_I2C_DOMAIN_MASK   0x8  // mask to get I2C sub frame from issued vote (frame)
#define NT_PDC_SPI_DOMAIN_MASK   0xA  // mask to get SPI sub frame from issued vote (frame)
#define NT_PDC_GPIO_DOMAIN_MASK  0xC  // mask to get GPIO sub frame from issued vote (frame)

#define NT_PDC_UART_DONT_CARE_MASK     0x80    // To check the dont care bit of UART in issued vote
#define NT_PDC_UART_CNTL_MASK          0x40    // To check the control bit of UART in issued vote
#define NT_PDC_I2C_DONT_CARE_MASK      0x200   // To check the dont care bit of I2C in issued vote
#define NT_PDC_I2C_CNTL_MASK           0x100   // To check the control bit of I2C in issued vote
#define NT_PDC_SPI_DONT_CARE_MASK      0x800   // To check the dont care bit of SPI in issued vote
#define NT_PDC_SPI_CNTL_MASK           0x400   // To check the control bit of SPI in issued vote
#define NT_PDC_GPIO_DONT_CARE_MASK     0x2000  // To check the dont care bit of GPIO in issued vote
#define NT_PDC_GPIO_CNTL_MASK          0x1000  // To check the control bit of GPIO in issued vote
#define NT_PDC_WIFIPM_MAX_STATE        0x3FFF  // Maximum of vote which can be issued to the wifipm resource created
#define NT_PDC_ON                      0x1     // Power domain ON
#define NT_PDC_OFF                     0x0     // Power domain OFF
#define NT_PDC_RRAM_POWER_DOMAIN       0x1     // Mask to control the RRAM power domain
#define NT_PDC_XIP_POWER_DOMAIN        0x2     // Mask to control the XIP power domain
#define NT_PDC_SECIP_POWER_DOMAIN      0x3     // Mask to control the SECIP power domain
#define NT_PDC_UART_POWER_DOMAIN       0x4     // Mask to control the UART power domain
#define NT_PDC_I2C_POWER_DOMAIN        0x5     // Mask to control the I2C power domain
#define NT_PDC_SPI_POWER_DOMAIN        0x6     // Mask to control the SPI power domain
#define NT_PDC_GPIO_POWER_DOMAIN       0x7     // Mask to control the GPIO power domain
#define NT_PDC_LOOP_COUNT              0x0     // Mask to control the RRAM power domain
#define NT_PDC_WPM_POWER_DOMAINS_COUNT 0x7     // No of domains in Wi-fi power manager resource
/*-------------------------------------------------------------------------------------------------------------*/

/*-----------------------------------------Globals------------------------------------------------------------*/
pdc_client *wifipm_act_client_handle = NULL;
pdc_client *wifipm_slp_client_handle = NULL;
pdc_resource *wifipm_res_handle = NULL;
/*-------------------------------------------------------------------------------------------------------------*/

/* cpudvr resource definition for MCU sleep */
pdc_resource_definition wpm_driver_resource_defn = {
    "wpmdvr",
    nt_pdc_vote_aggregation,
    nt_pdc_vote_driver,
    NT_PDC_WIFIPM_MAX_STATE,
};

/**
 * <!-- nt_pdc_resources -->
 *
 * @brief Creates the resources for PDC.
 * @return void
 */
void nt_pdc_resources(void)
{
    wifipm_res_handle = nt_pdc_create_resource(&wpm_driver_resource_defn, 0x0);
}

/**
 * <!-- nt_pdc_clients -->
 *
 * @brief Creates the clients for PDC.
 * @return void
 */
void nt_pdc_clients(void)
{
    wifipm_act_client_handle = pdc_create_client("wpm_act", "wpmdvr", PDC_ACTIVE_CLIENT);
    wifipm_slp_client_handle = pdc_create_client("wpm_slp", "wpmdvr", PDC_SLEEP_CLIENT);
}

/**
 * <!-- nt_pdc_init -->
 *
 * @brief Initialize the PDC framework and create the resources and clients.
 * @return void
 */
void nt_pdc_init(void)
{
    pdc_init();
    nt_pdc_resources();
    nt_pdc_clients();
}
/**
 * <!-- nt_pdc_vote_aggregation -->
 *
 * @brief update function for the XIP, SECIP, RRAM, UART, SPI, I2C and GPIO.
 * This function will aggregate all the votes issued by the clients to the resource
 * and an aggregated vote will be returned to the caller.
 * @param resource: Pointer to the resource
 * @return If successful, aggregated vote will be returned; else, NULL
 */
pdc_resource_state nt_pdc_vote_aggregation(pdc_resource *resource, pdc_client *client)  // pdc_resource_vote
{
    CORE_VERIFY_PTR(client);
    CORE_VERIFY_PTR(resource);
    pdc_resource_vote *temp_head = resource->vote_struct_head;
    uint32_t agg_vote_slp = 0, agg_vote_act = 0;

    while (temp_head != NULL) {
        if (temp_head->client_type == PDC_SLEEP_CLIENT) {
            agg_vote_slp |= temp_head->vote;
            temp_head = temp_head->next;
        }
        if (temp_head->client_type == PDC_ACTIVE_CLIENT) {
            agg_vote_act |= temp_head->vote;
            temp_head = temp_head->next;
        }
    }
    if (client->type == PDC_SLEEP_CLIENT) {
        return agg_vote_slp;
    } else {
        return agg_vote_act;
    }
}

/**
 * <!-- nt_pdc_vote_driver -->
 *
 * @brief The aggregated vote received from update function of PDC will be.
 * written to the HAL registers through this function
 * and the same vote will be returned to the caller.
 * @param vote: Aggregated vote from update function
 * @return If successful, aggregated vote will be returned; else, NULL
 */
pdc_resource_state nt_pdc_vote_driver(pdc_resource_state vote, pdc_client *client)
{
    uint8_t mcu_state;
    uint16_t count = 0;
    uint32_t dont_care = NT_PDC_DONT_CARE_BIT_MASK;
    uint32_t control_bit = NT_PDC_CONTROL_BIT_MASK;
    uint8_t pdc_power_domains[7] = {NT_PDC_RRAM_POWER_DOMAIN, NT_PDC_XIP_POWER_DOMAIN, NT_PDC_SECIP_POWER_DOMAIN,
                                    NT_PDC_UART_POWER_DOMAIN, NT_PDC_I2C_POWER_DOMAIN, NT_PDC_SPI_POWER_DOMAIN,
                                    NT_PDC_GPIO_POWER_DOMAIN};
    mcu_state = client->type;
    while (count <= NT_PDC_WPM_POWER_DOMAINS_COUNT) {
        if (count <= 2) {
            // For RRAM, XIP and SECIP
            if (vote & dont_care) {
                if (vote & control_bit) {
                    nt_hal_driver(pdc_power_domains[count], NT_PDC_ON, mcu_state);
                } else {
                    nt_hal_driver(pdc_power_domains[count], NT_PDC_OFF, mcu_state);
                }
            }
        } else {
            // For SON domain control
            if ((vote & NT_PDC_UART_DONT_CARE_MASK) || (vote & NT_PDC_I2C_DONT_CARE_MASK) ||
                (vote & NT_PDC_SPI_DONT_CARE_MASK) || (vote & NT_PDC_GPIO_DONT_CARE_MASK)) {
                if ((vote & NT_PDC_UART_CNTL_MASK) || (vote & NT_PDC_I2C_CNTL_MASK) || (vote & NT_PDC_SPI_CNTL_MASK) ||
                    (vote & NT_PDC_GPIO_CNTL_MASK)) {
                    // Turn SON ON
                    nt_son_control(NT_PDC_ON, mcu_state);
                } else {
                    // Turn SON OFF
                    nt_son_control(NT_PDC_OFF, mcu_state);
                }
            }
            // Peripheral domain control
            if (vote & dont_care) {
                // UART, I2C, SPI and GPIO control
                if (vote & control_bit) {
                    nt_hal_driver(pdc_power_domains[count], NT_PDC_ON, mcu_state);
                } else {
                    nt_hal_driver(pdc_power_domains[count], NT_PDC_OFF, mcu_state);
                }
            }
        }
        control_bit = control_bit << 2;
        dont_care = dont_care << 2;
        count++;
    }
    // NT_LOG_PDC_INFO("The aggregated vote written in the resource is : ",vote,0,0);
    return vote;
}

void nt_son_control(uint32_t control_state, uint32_t mcu_state)
{
    uint32_t reg_value;
    if (mcu_state == PDC_ACTIVE_CLIENT) {
        // SON Active state configuration
        if (control_state == NT_PDC_ON) {
            // SON turned ON
            reg_value = NT_REG_RD(QWLAN_PMU_CFG_AON_CNTL_MCU_ACTIVE_STATE_RESOURCE_REQ_REG);
            NT_REG_WR(QWLAN_PMU_CFG_AON_CNTL_MCU_ACTIVE_STATE_RESOURCE_REQ_REG,
                      reg_value |= QWLAN_PMU_CFG_AON_CNTL_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_SON_CNTL_BIT_MASK);
        } else {
            // SON turned OFF
            reg_value = NT_REG_RD(QWLAN_PMU_CFG_AON_CNTL_MCU_ACTIVE_STATE_RESOURCE_REQ_REG);
            NT_REG_WR(QWLAN_PMU_CFG_AON_CNTL_MCU_ACTIVE_STATE_RESOURCE_REQ_REG,
                      reg_value & ~QWLAN_PMU_CFG_AON_CNTL_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_SON_CNTL_BIT_MASK);
        }
    } else {
        // SON sleep state configuration
        if (control_state == NT_PDC_ON) {
            // SON turned ON
            reg_value = NT_REG_RD(QWLAN_PMU_CFG_AON_CNTL_MCU_SLEEP_STATE_RESOURCE_REQ_REG);
            NT_REG_WR(QWLAN_PMU_CFG_AON_CNTL_MCU_SLEEP_STATE_RESOURCE_REQ_REG,
                      reg_value |= QWLAN_PMU_CFG_AON_CNTL_MCU_SLEEP_STATE_RESOURCE_REQ_PD_SON_CNTL_BIT_MASK);
        } else {
            // SON turned OFF
            reg_value = NT_REG_RD(QWLAN_PMU_CFG_AON_CNTL_MCU_SLEEP_STATE_RESOURCE_REQ_REG);
            NT_REG_WR(QWLAN_PMU_CFG_AON_CNTL_MCU_SLEEP_STATE_RESOURCE_REQ_REG,
                      reg_value & ~QWLAN_PMU_CFG_AON_CNTL_MCU_SLEEP_STATE_RESOURCE_REQ_PD_SON_CNTL_BIT_MASK);
        }
    }
}

/**
 * <!-- nt_hal_driver -->
 *
 * @brief Writes the aggregated vote from the update function to HAL registers
 * @param power_domain: 1 - RRAM, 2 = XIP, 3 = SECIP, 4 = UART, 5 = I2C, 6 = SPI and 7 = GPIO
 * @param control_state: 0 - OFF, 1 = ON
 * @param mcu_state: PDC_ACTIVE_CLIENT for sleep state, PDC_SLEEP_CLIENT = active state.
 * @return void
 */
void nt_hal_driver(uint32_t power_domain, uint32_t control_state, uint32_t mcu_state)
{
    uint32_t reg_value;
    switch (power_domain) {
        case NT_PDC_RRAM_POWER_DOMAIN:
            if (control_state == NT_PDC_ON) {
                reg_value = NT_REG_RD(QWLAN_PMU_DIG_TOP_CFG_REG);
                reg_value &= ~QWLAN_PMU_DIG_TOP_CFG_RRAM_PD_MODE_MASK;
                NT_REG_WR(QWLAN_PMU_DIG_TOP_CFG_REG, reg_value);
            } else {
                reg_value = NT_REG_RD(QWLAN_PMU_DIG_TOP_CFG_REG);
                reg_value &= ~QWLAN_PMU_DIG_TOP_CFG_RRAM_PD_MODE_MASK;
                NT_REG_WR(QWLAN_PMU_DIG_TOP_CFG_REG, (reg_value | 0b10 << 11));
            }
            break;
        case NT_PDC_XIP_POWER_DOMAIN:
            if (mcu_state == PDC_ACTIVE_CLIENT) {
                if (control_state == NT_PDC_ON) {
                    reg_value = NT_REG_RD(QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_REG);
                    reg_value |= QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_XIP_CNTL_BIT_MASK;
                    NT_REG_WR(QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_REG, reg_value);
                } else {
                    reg_value = NT_REG_RD(QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_REG);
                    reg_value &= ~QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_XIP_CNTL_BIT_MASK;
                    NT_REG_WR(QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_REG, reg_value);
                }
            } else {
                if (control_state == NT_PDC_ON) {
                    reg_value = NT_REG_RD(QWLAN_PMU_CFG_MCU_SLEEP_STATE_RESOURCE_REQ_REG);
                    reg_value |= QWLAN_PMU_CFG_MCU_SLEEP_STATE_RESOURCE_REQ_PD_XIP_CNTL_BIT_MASK;
                    NT_REG_WR(QWLAN_PMU_CFG_MCU_SLEEP_STATE_RESOURCE_REQ_REG, reg_value);
                } else {
                    reg_value = NT_REG_RD(QWLAN_PMU_CFG_MCU_SLEEP_STATE_RESOURCE_REQ_REG);
                    reg_value &= ~QWLAN_PMU_CFG_MCU_SLEEP_STATE_RESOURCE_REQ_PD_XIP_CNTL_BIT_MASK;
                    NT_REG_WR(QWLAN_PMU_CFG_MCU_SLEEP_STATE_RESOURCE_REQ_REG, reg_value);
                }
            }
            break;
        case NT_PDC_SECIP_POWER_DOMAIN:
            if (mcu_state == PDC_ACTIVE_CLIENT) {
                if (control_state == NT_PDC_ON) {
                    reg_value = NT_REG_RD(QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_REG);
                    reg_value |= QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_SECIP_CNTL_BIT_MASK;
                    NT_REG_WR(QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_REG, reg_value);
                } else {
                    reg_value = NT_REG_RD(QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_REG);
                    reg_value &= ~QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_PD_SECIP_CNTL_BIT_MASK;
                    NT_REG_WR(QWLAN_PMU_CFG_MCU_ACTIVE_STATE_RESOURCE_REQ_REG, reg_value);
                }
            } else {
                if (control_state == NT_PDC_ON) {
                    reg_value = NT_REG_RD(QWLAN_PMU_CFG_MCU_SLEEP_STATE_RESOURCE_REQ_REG);
                    reg_value |= QWLAN_PMU_CFG_MCU_SLEEP_STATE_RESOURCE_REQ_PD_SECIP_CNTL_BIT_MASK;
                    NT_REG_WR(QWLAN_PMU_CFG_MCU_SLEEP_STATE_RESOURCE_REQ_REG, reg_value);
                } else {
                    reg_value = NT_REG_RD(QWLAN_PMU_CFG_MCU_SLEEP_STATE_RESOURCE_REQ_REG);
                    reg_value &= ~QWLAN_PMU_CFG_MCU_SLEEP_STATE_RESOURCE_REQ_PD_SECIP_CNTL_BIT_MASK;
                    NT_REG_WR(QWLAN_PMU_CFG_MCU_SLEEP_STATE_RESOURCE_REQ_REG, reg_value);
                }
            }
            break;
        case NT_PDC_UART_POWER_DOMAIN:
            if (control_state == NT_PDC_ON) {
                // Clock to UART enabled
                reg_value = NT_REG_RD(QWLAN_PMU_ROOT_CLK_ENABLE_REG);
                reg_value |= QWLAN_PMU_ROOT_CLK_ENABLE_UART_ROOT_CLK_ENABLE_MASK;
                NT_REG_WR(QWLAN_PMU_ROOT_CLK_ENABLE_REG, reg_value);
            } else {
                // Clock to UART disabled
                reg_value = NT_REG_RD(QWLAN_PMU_CLKGATE_DISABLE_REG);
                reg_value |= QWLAN_PMU_CLKGATE_DISABLE_UART_CLKGATE_DISABLE_MASK;
                NT_REG_WR(QWLAN_PMU_CLKGATE_DISABLE_REG, reg_value);
            }
            break;
        case NT_PDC_I2C_POWER_DOMAIN:
            if (control_state == NT_PDC_ON) {
                reg_value = NT_REG_RD(QWLAN_PMU_ROOT_CLK_ENABLE_REG);
                reg_value |= QWLAN_PMU_ROOT_CLK_ENABLE_I2C_ROOT_CLK_ENABLE_MASK;
                NT_REG_WR(QWLAN_PMU_ROOT_CLK_ENABLE_REG, reg_value);
            } else {
                reg_value = NT_REG_RD(QWLAN_PMU_CLKGATE_DISABLE_REG);
                reg_value |= QWLAN_PMU_CLKGATE_DISABLE_I2C_CLKGATE_DISABLE_MASK;
                NT_REG_WR(QWLAN_PMU_CLKGATE_DISABLE_REG, reg_value);
            }
            break;
        case NT_PDC_SPI_POWER_DOMAIN:
            if (control_state == NT_PDC_ON) {
                reg_value = NT_REG_RD(QWLAN_PMU_ROOT_CLK_ENABLE_REG);
                reg_value |= QWLAN_PMU_ROOT_CLK_ENABLE_SPI_ROOT_CLK_ENABLE_MASK;
                NT_REG_WR(QWLAN_PMU_ROOT_CLK_ENABLE_REG, reg_value);
            } else {
                reg_value = NT_REG_RD(QWLAN_PMU_CLKGATE_DISABLE_REG);
                reg_value |= QWLAN_PMU_CLKGATE_DISABLE_SPI_CLKGATE_DISABLE_MASK;
                NT_REG_WR(QWLAN_PMU_CLKGATE_DISABLE_REG, reg_value);
            }
            break;
        case NT_PDC_GPIO_POWER_DOMAIN:
            if (control_state == NT_PDC_ON) {
                reg_value = NT_REG_RD(QWLAN_PMU_ROOT_CLK_ENABLE_REG);
                reg_value |= QWLAN_PMU_ROOT_CLK_ENABLE_GPIO_ROOT_CLK_ENABLE_MASK;
                NT_REG_WR(QWLAN_PMU_ROOT_CLK_ENABLE_REG, reg_value);
            } else {
                reg_value = NT_REG_RD(QWLAN_PMU_CLKGATE_DISABLE_REG);
                reg_value |= QWLAN_PMU_CLKGATE_DISABLE_GPIO_CLKGATE_DISABLE_MASK;
                NT_REG_WR(QWLAN_PMU_CLKGATE_DISABLE_REG, reg_value);
            }
            break;

        default:
            break;
    }
}

#endif
