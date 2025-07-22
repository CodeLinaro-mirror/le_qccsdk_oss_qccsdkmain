/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*========================================================================
 *
 * @file qtmr.h
 * @brief QTMR param and struct definitions
 *========================================================================*/
#ifndef QTMR_H
#define QTMR_H

/*-------------------------------------------------------------------------
 * Include Files
 * ----------------------------------------------------------------------*/

//#include "com_dtypes.h"
#include <stdint.h>
#include <stdbool.h>

/*-------------------------------------------------------------------------
 * Preprocessor Definitions and Constants
 * ----------------------------------------------------------------------*/
#define QTMR_0_INT (4) /* IRQn for Qtmr frame 0 */
#define QTMR_1_INT (5) /* IRQn for Qtmr frame 1 */
#define QTMR_2_INT (6) /* IRQn for Qtmr frame 2 */
#define QTMR_3_INT (7) /* IRQn for Qtmr frame 3 */
#define QTMR_4_INT (8) /* IRQn for Qtmr frame 4 */

/*-------------------------------------------------------------------------
 * Type Declarations
 * ----------------------------------------------------------------------*/
/**
 * Qtimer frame type
 */
typedef enum {
    QTMR_FRAME_PHYSICAL_0, /**< Qtimer Physical Frame-0 */
    QTMR_FRAME_PHYSICAL_1, /**< Qtimer Physical Frame-1 */
    QTMR_FRAME_PHYSICAL_2, /**< Qtimer Physical Frame-2 */
    QTMR_FRAME_PHYSICAL_3, /**< Qtimer Physical Frame-3 */
    QTMR_FRAME_PHYSICAL_4, /**< Qtimer Physical Frame-4 */
} qtmr_frame_t;

/*-------------------------------------------------------------------------
 * Function Declarations and Documentation
 * ----------------------------------------------------------------------*/

/**
 * @brief Get QTMR counter frequency
 *
 * @param[in]   void
 *
 * @return  QTMR counter frequency.
 */
uint32_t qtmr_get_freq(void);

/**
 * @brief Enable / Disable Qtimer
 *
 * @param[in]   enable  Flag (TRUE: enable, FALSE: disable)
 *
 * @return  None.
 */
void qtmr_enable(qtmr_frame_t frame, bool enable);

/**
 * @brief Get Qtimer current counter value.
 *
 * @return Current value of the counter.
 *
 */
uint64_t qtmr_get_time64(qtmr_frame_t frame);

/**
 * @brief Set timer trigger time.
 *
 * @param[in]   match_count     clock count at which next interrupt will occur
 *
 * @return  None.
 *
 */
void qtmr_set_trigger64(qtmr_frame_t frame, uint64_t match_count);

/**
 * @brief Initialize QTimer HW block.
 *
 * @return  None.
 *
 * @note This API should be called only once during platform initialization.
 */
void qtmr_init(void);

/**
 * @brief Set Root Clock
 *
 * @param[in] set_val (TRUE: enable rootclk, FALSE: disable rootclk)
 *
 * @return  None.
 */
void qtmr_set_rootclk(bool set_val);

/**
 * @brief print synchoronization values
 *
 * @param None.
 *
 * @return  None.
 */
void qtmr_print_sync_data(void);

#endif /* #ifndef QTMR_H */
