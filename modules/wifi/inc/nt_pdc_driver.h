/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef CORE_SYSTEM_INC_NT_PDC_DRIVER_H_
#define CORE_SYSTEM_INC_NT_PDC_DRIVER_H_

#include <stdint.h>
#include "nt_pdc.h"
//#define NT_FN_PDC_     				0 //Enable PDC
#ifdef NT_FN_PDC_
/*--------------------------------------------------------Vote(frame)-------------------------------------------------------------------
______________________________________________________________________________________________________________________________________________________________________________________________
|_________GPIO_____________|__________SPI_____________|__________I2C_____________|__________UART____________|__________SECIP___________|____________XIP___________|___________RRAM___________|
|Don't care bit | ctrl bit |Don't care bit | ctrl bit |Don't care bit | ctrl bit |Don't care bit | ctrl bit |Don't care
bit | ctrl bit |Don't care bit | ctrl bit |Don't care bit | ctrl bit |
----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

*/

/**
 * <!-- nt_pdc_vote_aggregation -->
 *
 * @brief update function for the XIP, SECIP, RRAM, UART, SPI, I2C and GPIO.
 * This function will aggregate all the votes issued by the clients to the resource
 * and an aggregated vote will be returned to the caller.
 * @param resource: Pointer to the resource
 * @return If successful, aggregated vote will be returned; else, NULL
 */
pdc_resource_state nt_pdc_vote_aggregation(pdc_resource *resource, pdc_client *client);
/**
 * <!-- nt_pdc_vote_driver -->
 *
 * @brief The aggregated vote received from update function of PDC will be.
 * written to the HAL registers through this function
 * and the same vote will be returned to the caller.
 * @param vote: Aggregated vote from update function
 * @return If successful, aggregated vote will be returned; else, NULL
 */
pdc_resource_state nt_pdc_vote_driver(pdc_resource_state vote, pdc_client *client);
/**
 * <!-- nt_pdc_init -->
 *
 * @brief Initialize the PDC framework and create the resources and clients.
 * @return void
 */

void nt_pdc_init(void);
/**
 * <!-- nt_pdc_resources -->
 *
 * @brief Creates the resources for PDC.
 * @return void
 */
void pdc_resources(void);

/**
 * <!-- nt_pdc_clients -->
 *
 * @brief Creates the clients for PDC.
 * @return void
 */
void pdc_clients(void);
/**
 * <!-- nt_son_control -->
 *
 * @brief Extracts the SON domain information from the aggregated vote and.
 * turn ON/OFF SON domain accordingly
 * @param vote: aggregated vote from update function
 * @return void
 */
void nt_son_control(uint32_t control_state, uint32_t mcu_state);
/**
 * <!-- nt_hal_driver -->
 *
 * @brief Writes the aggregated vote from the update function to HAL registers
 * @param power_domain: 1 - RRAM, 2 = XIP, 3 = SECIP, 4 = UART, 5 = I2C, 6 = SPI and 7 = GPIO
 * @param control_state: 0 - OFF, 1 = ON
 * @param mcu_state: PDC_ACTIVE_CLIENT for sleep state, PDC_SLEEP_CLIENT = active state.
 * @return void
 */
void nt_hal_driver(uint32_t power_domain, uint32_t control_state, uint32_t mcu_state);

#endif
#endif /* CORE_SYSTEM_INC_NT_PDC_DRIVER_H_ */
