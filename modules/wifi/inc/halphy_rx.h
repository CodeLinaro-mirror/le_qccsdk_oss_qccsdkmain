/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*========================================================================

 * @file halphy_rx.h
 * @brief HALPHY Rx parameters and function declarations
 * ======================================================================*/

#ifndef _HALPHY_RX_H_
#define _HALPHY_RX_H_

/*------------------------------------------------------------------------
 * Include Files
 * ----------------------------------------------------------------------*/

#include <stdint.h>
#include <stdbool.h>

/*------------------------------------------------------------------------
 * Type Declarations
 * ----------------------------------------------------------------------*/

typedef struct rx_stats_s {
    uint32_t totalPackets;
    uint32_t goodPackets;
    uint32_t crcPackets;
    int32_t rssi;
} rx_stats_t;

/*------------------------------------------------------------------------
 * Function Declarations and Documentation
 * ----------------------------------------------------------------------*/

void halphy_rx_reset_stats(void);
void halphy_rx_get_stats(rx_stats_t *rx_stats);
void halphy_rx_reset_rssi_offset(void);
void halphy_rx_set_rssi_offset(int8_t offset_value);
// void halphy_rx_get_rssi(int16_t *p_rssi);

#endif /* _HALPHY_RX_H_ */
