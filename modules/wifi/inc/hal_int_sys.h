/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _HAL_INT_SYS_H_
#define _HAL_INT_SYS_H_

#include "hal_int_cfg.h"
#include "hal_int_phy.h"

#ifdef NT_DEBUG
#define HAL_CFG_INC_DBG_PRINT
#endif

#ifdef HAL_CFG_INC_DBG_PRINT
void hal_sys_dbg_print(const char *s, const uint32_t a1, const uint32_t a2, const uint32_t a3, const char *fn,
                       const uint32_t ln);
#define HAL_DBG_PRINT(str, a1, a2, a3) hal_sys_dbg_print(str, a1, a2, a3, __func__, __LINE__)
#else
#define HAL_DBG_PRINT(str, a1, a2, a3)
#endif
/**
 * @brief calculates the time difference between input time and MTU current time
 * @param start_time : time in usec that we need to compare with current time to calculated difference
 * @return returns the difference value in milli seconds
 */
uint32_t nt_hal_calc_time_elapsed(uint32_t start_time);

/**
 * @brief gets the current MTU time in usec
 * @return returns the current MTU time in usec
 */
uint32_t nt_hal_get_curr_time();
NT_BOOL nt_hal_is_warm_boot();

void hal_rx_disable(void);
void hal_rx_enable(void);
#define HAL_ROUNDUP(x, y) ((((x) + ((y)-1)) / (y)) * (y))

/*
 * @brief	: Read CCA Counter0/1/2 Register
 * @param  	: CCA Reg number, Reg read value
 * @return 	: nt_status_t
 */
nt_status_t nt_hal_get_cca_counter(uint8_t cca_counter_num, uint32_t *reg_read);

/*
 * @brief	: Write to CCA Counter0/1/2 Register
 * @param  	: CCA Reg number, value
 * @return 	: nt_status_t
 */
nt_status_t nt_hal_set_cca_counter(uint8_t cca_counter_num, uint32_t val);

/*
 * @brief	: Get CCA control reg 0/2 value
 * @param  	: Reg number, Reg read value
 * @return 	: nt_status_t
 */
nt_status_t nt_hal_get_cca_control_reg(uint8_t reg_num, uint32_t *reg_read);

/*
 * @brief	: Set CCA control reg 0/2 value
 * @param  	: Reg number, Reg write value
 * @return 	: nt_status_t
 */
nt_status_t nt_hal_set_cca_control_reg(uint8_t reg_num, uint32_t reg_val);

#endif  // _HAL_INT_SYS_H_
