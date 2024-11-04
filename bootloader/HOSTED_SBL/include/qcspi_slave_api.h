/*========================================================================
* * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
* * SPDX-License-Identifier: BSD-3-Clause-Clear
*========================================================================*/
#define SUPPORT_QCSPI_SLAVE

#ifdef SUPPORT_QCSPI_SLAVE
#ifndef QCSPI_SLAVE_API_H_
#define QCSPI_SLAVE_API_H_
/*-------------------------------------------------------------------------
 * Function Declarations and Documentation
 * ----------------------------------------------------------------------*/

// Funiction to initialize QcSPI
void qcspi_slv_init (void);
void nt_spi_slv_interrupt(void);
		
void nt_delay(void);
//uint32_t nt_convert_little_big_endian(uint8_t* buf);
void nt_qspi_tx_clear(void);

#if defined(SUPPORT_PBL_PATCH)
//typedef void (*qcspi_slv_init_t) (void);
//static functions
typedef void (*nt_delay_t)(void);
typedef uint32_t (*nt_convert_little_big_endian_t)(uint8_t* buf);
typedef void (*nt_qspi_tx_clear_t)(void);

typedef struct qcspi_slave_api_s{
    qcspi_slv_init_t qcspi_slv_init_pfn;
//static functions
#if (NT_CHIP_VERSION==2)
    nt_delay_t nt_delay_pfn;
    nt_convert_little_big_endian_t nt_convert_little_big_endian_pfn;
    nt_qspi_tx_clear_t nt_qspi_tx_clear_pfn;
#endif
}qcspi_slave_api_t;
#endif /*SUPPORT_PBL_PATCH*/
#endif //SUPPORT_QCSPI_SLAVE
#endif //QCSPI_SLAVE_API_H_
