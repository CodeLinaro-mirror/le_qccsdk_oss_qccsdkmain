/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#ifndef __FERMION_HWIO_BASE_H__
#define __FERMION_HWIO_BASE_H__

#ifdef SCALE_INCLUDES
#include "HALhwio.h"
#else
#include "msmhwio.h"
#endif

///////////////////////////////////////////////////////////////////////////////////////////////
// Instance Relative Offsets from Block wcss
///////////////////////////////////////////////////////////////////////////////////////////////

#define HW_TXP_BASE                                   0x02080400
#define HW_TPE_BASE                                   0x02082000
#define HW_DPU_BASE                                   0x02081800
#define HW_ADU_BASE                                   0x02082800
#define HW_BMU_BASE                                   0x02080000
#define HW_RXP_BASE                                   0x02080800
#define HW_RPE_BASE                                   0x02082400
#define HW_MCU_BASE                                   0x02080c00
#define HW_MTU_BASE                                   0x02081400
#define HW_DAHB_BASE                                  0x020c0000
#define HW_CAHB_BASE                                  0x02082c00
#define HW_FDAHB_BASE                                 0x020c0400
#define HW_PMU_BASE                                   0x011aec00
#define HW_UART_BASE                                  0x01233800
#define HW_I2C_BASE                                   0x01233900
#define HW_GPIO_BASE                                  0x01233a00
#define HW_DWSPI_SLAVE_BASE                           0x01233b00
#define HW_QCSPI_SLAVE_BASE                           0x01264000
#define HW_CMEM_BASE                                  0x011a0400
#define HW_CCU_BASE                                   0x0119a000
#define HW_DXE_0_BASE                                 0x0119f000
#define HW_CDAHB_BASE                                 0x011a0000
#define HW_AGC_BASE                                   0x02013c00
#define HW_BTCF_BASE                                  0x02014800
#define HW_FFT_BASE                                   0x0200c000
#define HW_MPI_BASE                                   0x02013400
#define HW_PHYDBG_BASE                                0x02004000
#define HW_PHYINT_BASE                                0x02015c00
#define HW_PMI_BASE                                   0x02013800
#define HW_RACTL_BASE                                 0x0200f000
#define HW_RBAPB_BASE                                 0x02014400
#define HW_RFIF_BASE                                  0x02015000
#define HW_RFAPB_BASE                                 0x00000000
#define HW_RXACLKCTRL_BASE                            0x02016000
#define HW_RXCLKCTRL_BASE                             0x02016400
#define HW_TACTL_BASE                                 0x02012400
#define HW_TAQAM_BASE                                 0x02012c00
#define HW_TBAPB_BASE                                 0x02012800
#define HW_TDC_BASE                                   0x02014c00
#define HW_TPC_BASE                                   0x02010000
#define HW_TXCTL_BASE                                 0x0200b000
#define HW_TXCLKCTRL_BASE                             0x02013000
#define HW_TXFIR_BASE                                 0x02018000
#define HW_CAL_BASE                                   0x02008000
#define HW_QTMR_AC_BASE                               0x011a8000
#define HW_QTMR_V1_T0_BASE                            0x011a9000
#define HW_QTMR_V1_T1_BASE                            0x011aa000
#define HW_QTMR_V1_T2_BASE                            0x011ab000
#define HW_QTMR_V1_T3_BASE                            0x011ac000
#define HW_QTMR_V1_T4_BASE                            0x011ad000
#define HW_CSCTI_BASE                                 0xe0042000
#define HW_CXTMC_F32W4K_BASE                          0xe0043004
#define HW_QSPI_BASE                                  0x01190000
#define HW_RRAM_CTRL_BASE                             0x01192400
#define HW_FCC_CSR_REG_BASE                           0x01192000
#define HW_CACHE_REGS_BASE                            0x01180000
#define HW_KDF_CSR_BASE                               0x01233c00
#define HW_LOCK_WRAPPER_L1_BASE                       0x01234800
#define HW_PERISS_CRYPTO_CORE_BASE                    0x01234c00
#define HW_ELP_PKA_AHB_BASE                           0x01244c00
#define HW_LOCK_WRAPPER_L2_BASE                       0x01248c00
#define HW_PRNG_BASE                                  0x0124cc00
#define HW_XPU2_BASE                                  0x0124d000
#define HW_XPU2AHB_BASE                               0x0125d000
#define HW_CPR_WRAPPER_BASE                           0x01260000
#define HW_CXC_BMH_REG_BASE                           0x02085000
#define HW_CXC_LCMH_REG_BASE                          0x02087000
#define HW_CXC_MCIBASIC_REG_BASE                      0x02089000
#define HW_CXC_LMH_REG_BASE                           0x0208b000
#define HW_CXC_SMH_REG_BASE                           0x0208d000
#define HW_CXC_PMH_REG_BASE                           0x0208f000
#define SEQ_RFA_DIG_MC_WL_MC_OFFSET                   0x02040000
#define SEQ_RFA_DIG_MBIAS_MBIAS_OFFSET                0x02040200
#define SEQ_RFA_DIG_XO_XO_OFFSET                      0x02040400
#define SEQ_WCSS_CLKGEN_CLBS_OFFSET                   0x02040434
#define SEQ_RFA_DIG_WL_SYNTH_BS_OFFSET                0x02040800
#define SEQ_RFA_DIG_WL_SYNTH_CLBS_OFFSET              0x02040840
#define SEQ_RFA_DIG_WL_SYNTH_BIST_OFFSET              0x02040880
#define SEQ_RFA_DIG_WL_SYNTH_PC_OFFSET                0x020408c0
#define SEQ_RFA_DIG_WL_SYNTH_AC_OFFSET                0x02040900
#define SEQ_RFA_DIG_WL_SYNTH_LO_OFFSET                0x02040980
#define SEQ_RFA_DIG_BBPLL_BBPLL_OFFSET                0x02040a00
#define SEQ_WCSS_DRM_REG_OFFSET                       0x02040b00
#define SEQ_RFA_DIG_RBIST_TX_RBIST_TX_BAREBONE_OFFSET 0x02041000
#define SEQ_RFA_DIG_WL_DAC_OFFSET                     0x02041100
#define SEQ_RFA_DIG_WL_ADC_OFFSET                     0x02041400
#define SEQ_RFA_DIG_WL_RXFE_OFFSET                    0x02041800
#define SEQ_RFA_DIG_WL_TXFE_OFFSET                    0x02041c00
#define SEQ_RFA_DIG_RXBB_WL_RXBB_OFFSET               0x02042000
#define SEQ_RFA_DIG_TXBB_WL_TXBB_OFFSET               0x02042400
#define SEQ_RFA_DIG_TPC_WL_TPC_OFFSET                 0x02042800
#define SEQ_RFA_DIG_PMU_OFFSET                        0x02043000

#endif
