/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#ifndef _HAL_API_BEACON_H_
#define _HAL_API_BEACON_H_

#if 0
#include <stdint.h>
#include "nt_err_codes.h"
#include "hal_api_sys.h"


	/**
	 * @brief Initialize a beacon
	 * Initializes the hardware beacon template
	 * @param bcn_size_max  maximum size of the beacon
	 * @param bcn_addr_base where the beacon (including the template) is located
	 * @param bcn_interval  beacon interval
	 * @return eNT_OK  on success
	 */
	nt_status_t nt_hal_beacon_init(uint16_t bcn_size_max, uint32_t bcn_addr_base, uint16_t bcn_interval)


	/**
	 * @brief Update a beacon
	 * Updates the hardware beacon template; but does not change beacon tx status
	 * @param bss      the bss to update (should have already been added)
	 * @param bcn      the new beacon content
	 * @param bcn_len  the new length of the beacon
	 * @return eNT_OK  on success
	 */
	nt_status_t nt_hal_beacon_update(nt_hal_bss_t* bss, uint8_t* bcn, uint8_t bcn_len);

	/**
	* @brief Enable/Disable beacon template
	* Enable/Disable the hardware beacon tx
	* @param bss      the bss to update (should have already been added)
	* @param bcn      the new beacon content
	* @param enable   enable (1) or disable (0) beacon tx
	* @return eNT_OK  on success
	*/
	nt_status_t nt_hal_beacon_enable(nt_hal_bss_t* bss, uint8_t enable);
#endif

#if defined(FEATURE_STA_ECSA) || defined(SUPPORT_SAP_POWERSAVE)
/**
 * @brief Gets the next TBTT
 * Reads the tbtt register and returns the next tbtt
 * @param This function does not need any argument
 * @return Returns the next tbtt
 */
uint64_t nt_hal_get_tbtt(void);
#endif

#endif  // _HAL_API_BEACON_H_
