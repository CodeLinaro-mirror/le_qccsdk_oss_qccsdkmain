/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*========================================================================
 *
 * @file qcspi_slave.h
 * @brief QcSPI Slave driver function declarations
 *========================================================================*/
#ifdef SUPPORT_QCSPI_SLAVE
#ifndef QCSPI_SLAVE_API_H_
#define QCSPI_SLAVE_API_H_

/*-------------------------------------------------------------------------
 * Function Declarations and Documentation
 * ----------------------------------------------------------------------*/

// Funiction to initialize QcSPI
void qcspi_slv_init(void);
void qcspi_slv_deinit(void);
void qcspi_slv_disable_host_int(void);
void qcspi_slv_enable_host_int(void);
#endif  // SUPPORT_QCSPI_SLAVE
#endif  // QCSPI_SLAVE_API_H_
