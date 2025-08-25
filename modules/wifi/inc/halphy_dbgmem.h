/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*========================================================================

 * @file halphy_dbgmem.h
 * @brief PHYBDG Internal Debug Memory related parameters and function declarations
 * ======================================================================*/

#ifndef _HALPHY_DBGMEM_H_
#define _HALPHY_DBGMEM_H_

/*------------------------------------------------------------------------
 * Include Files
 * ----------------------------------------------------------------------*/

#include <stdint.h>
#include <stdbool.h>
#include "phyCalUtils.h"
#include "halphy_phydbg.h"

/*------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * ----------------------------------------------------------------------*/
#define PHYDBG_BUFFER_SIZE 4096

/*------------------------------------------------------------------------
 * Function Declarations and Documentation
 * ----------------------------------------------------------------------*/

void dbgmem_set_mem_cfg(phydbg_mem_cfg_t mem_cfg);
uint16_t dbgmem_write_playback_mem(uint32_t const *data, uint16_t n_words, uint16_t start_idx);
void dbgmem_set_max_address(uint16_t start_addr, uint16_t n_words);
void dbgmem_read_capture_mem(uint32_t *data, uint16_t n_words, uint16_t start_idx, bool is_trigger,
                             uint16_t n_pretrigger_words);

#endif /* _HALPHY_DBGMEM_H_ */
