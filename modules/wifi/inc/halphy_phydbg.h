/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
/*========================================================================

 * @file halphy_phydbg.h
 * @brief PHYBDG related parameters and function declarations
 * ======================================================================*/

#ifndef _HALPHY_PHYDBG_H_
#define _HALPHY_PHYDBG_H_

/*------------------------------------------------------------------------
 * Include Files
 * ----------------------------------------------------------------------*/

#include <stdint.h>
#include <stdbool.h>
#include "fermion_reg.h"
#include "phyCalUtils.h"

/*------------------------------------------------------------------------
 * Type Declarations
 * ----------------------------------------------------------------------*/
typedef enum phydbg_mem_select_e {
    phydbg_mem_select_cmem = 0,     // Select cMem
    phydbg_mem_select_internal = 1  // Select PHYDBG internal memory
} phydbg_mem_select_t;

typedef enum phydbg_mem_cfg_e {
    phydbg_mem_cfg_full = 0,   // Select full memory access
    phydbg_mem_cfg_shared = 1  // Select shared memory for simultaneous playback/capture
} phydbg_mem_cfg_t;

#if 0
typedef enum phydbg_status_rdstate_e {
    rdstate_idle = 0b00001, // Inactive
    rdstate_flush = 0b00010, // Flushing read sync FIFO
    rdstate_start_prefetch = 0b00100, // Start pre-fetching data
    rdstate_prefetch = 0b01000, // Continue pre-fetching data
    rdstate_rdactive = 0b10000, // Playback in progress
} phydbg_status_rdstate_t;

// Current state of packet generator (gen_txdata) FSM
typedef enum phydbg_status_txstate_e {
    txstate_idle = 0, // packet generator is inactive
    txstate_infd0 = 1, // read info data words 0 from memory
    txstate_infd1 = 2, // read info data words 1 from memory
    txstate_warmup = 3, // hold mpi_rf_warmup for specified time
    txstate_clkstart0 = 4, // issue clk start
    txstate_clkstart1 = 5, // wait
    txstate_clkstart2 = 6, // wait
    txstate_clkstart3 = 7, // wait
    txstate_cmd_start = 8, // issue cmd_start
    txstate_cmd = 9, // reading mpi command data from memory
    txstate_cmd_wait = 10, // wait between issuing cmd data words
    txstate_pyldf = 11, // read payload bytes from memory
    txstate_pyldf_wait = 12, // wait if no request or no data
    txstate_pyldr = 13, // generate payload bytes with random data
    txstate_pyldr_wait = 14, // wait if no request
    txstate_crc = 15, // insert CRC-32 generated over payload bytes
    txstate_crc_wait = 16, // wait if no request
    txstate_flush = 17, // wait for all requests to propogate through pipline
    txstate_txdonewait = 18, // wait for PHY TX to finish processing packet
    txstate_tifwait = 19, // pause for interpacket gap
} phydbg_status_txstate_t;

// Current state of capture controller (gen_waddr) FSM.
typedef enum phydbg_status_wrstate_e {
    wrstate_idle = 0b00001, // No capture activity
    wrstate_start = 0b00010, // Capture initiated by user
    wrstate_wrwait = 0b00100, // Waiting for start trigger
    wrstate_wractive = 0b01000, // Capture data until specified end condition is reached
    wrstate_wrpost = 0b10000, // post-trigger capture for CRC capture mode
} phydbg_status_wrstate_t;

// Multiplexer select for 80MHz domain signals (ie. ADC, RXFIR and DAC data).
// The available modes depend on the number of TX and RX chains in the device.
typedef enum phydbg_capt_cfg_xbarsel_e {
    C32_ADC_CH0 = 0, // {agc*[4:0], rxgain0[6:2], rxbar0q[10:0], rxbar0i[10:0]} (ID-27/28*)
    C32_RXF20_CH0 = 4, // {2'b0, rxgain0[2:0], 1'b0, rfsat*[3:0], rxgain0[5:2], agc[4:0], iq, rxfiq0_20[11:0]} (ID-28a)
    C32_RXF40_CH0 = 8, // {2'b0, rxgain0[2:0], 1'b0, rfsat*[3:0], rxgain0[5:2], agc[4:0], iq, rxfiq0_40[11:0]} (ID-28b)
    C32_RXF80_CH0 = 12, // {2'b0, rxgain0[2:0], 1'b0, rfsat*[3:0], rxgain0[5:2], agc[4:0], iq, rxfiq0_80[11:0]} (ID-28b)
    C32_DAC_CH0 = 32 // {txctl[4:0], cca1, cca2, txbusy, 2'b0, dac0q[10:0], dac0i[10:0]} (ID-6a)
} phydbg_capt_cfg_xbarsel_t;

// Multiplexer select lines for vital bus mux for memory capture or testbus or trigger selection logic.
typedef enum phydbg_capt_cfg_vitsel_e {
    VITSEL_RXA = 0,
    VITSEL_RXB  = 1,
    VITSEL_RXSM = 2,
    VITSEL_TX = 3,
    VITSEL_AGC = 4,
    VITSEL_AGC = 5
} phydbg_capt_cfg_vitsel_t;

// Multiplexer select lines for data mux for memory capture or testbus.
typedef enum phydbg_capt_cfg_datasel_e {
    SEL_XBAR = 0,
    SEL_AGC,
    SEL_RXA,
    SEL_TX,
    SEL_RAVIT,
    SEL_RBVIT,
    SEL_RXFSM,
    SEL_TXVIT,
    SEL_PMI,
    SEL_AGCVIT,
    SEL_WMAC
} phydbg_capt_cfg_datasel_t;

typedef enum phydbg_seltrig_stop0_e {
    stop0_reg = 0b000,
    stop0_agc = 0b001,
    stop0_gpio = 0b010,
    stop0_noagc = 0b100,
    stop0_crcpass = 0b101,
    stop0_crcfail = 0b110,
} phydbg_seltrig_stop0_t;

typedef enum phydbg_seltrig_etrig0_e {
    etrig0_tbus = 0,
    etrig0_agc,
    etrig0_gpio,
    etrig0_crcdone,
    etrig0_crcpass,
    etrig0_crcfail,
} phydbg_seltrig_etrig0_t;

typedef enum phydbg_seltrig_strig0_e {
    strig0_tbus = 0,
    strig0_agc,
    strig0_gpio,
    strig0_crcdone,
    strig0_crcpass,
    strig0_crcfail,
    strig0_disabled,
} phydbg_seltrig_strig0_t;

#endif

#endif /* _HALPHY_PHYDBG_H_ */
