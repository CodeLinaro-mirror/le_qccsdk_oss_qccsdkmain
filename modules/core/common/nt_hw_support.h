/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _NT_HW_SUPPORT_H
#define _NT_HW_SUPPORT_H

/**
 * rram OTP and MAIN region address
 */
#define NT_RRAM_OTP_INFO_START_ADDR 0x00080000  // info start region
#define NT_RRAM_OTP_INFO_END_ADDR   0x00080FFF  // info end address
#define NT_RRAM_TRC_CFG_START_ADDR  0x00081000
#define NT_RRAM_TRC_CFG_END_ADDR    0x000813FF
#define NT_RRAM_RESERV_START_ADDR   0x00081400
#define NT_RRAM_RESERV_END_ADDR     0x001FFFFF
#define NT_RRAM_MAIN_START_ADDR     0x00200000
#define NT_RRAM_MAIN_END_ADDR       0x0037FFFF
/**
 * macro it tells sub-type region
 */
#define NT_RRAM_OTP_PTE_START_REGION 0x00080000
#define NT_RRAM_OTP_PTE_END_REGION   0x00080016

#define NT_RRAM_OTP_MFR_TEST0_START_REGION 0x00080020  // manufacture test 0 region
#define NT_RRAM_OTP_MFR_TEST0_END_REGION   0x0008002C  // manufacture test 0 region

#define NT_RRAM_OTP_RD_WR_PERM_START_REGION 0x00080030  // read/write permission region
#define NT_RRAM_OTP_RD_WR_END_PERM_REGION   0x00080035  // read/write permission region end

#define NT_RRAM_OTP_HW_KEY_START_REGION 0x00080040  // hardware key
#define NT_RRAM_OTP_HW_END_KEY_REGION   0x0008004F  // hardware key end

#define NT_RRAM_OTP_USER_DATA_KEY_START_REGION 0x00080050  // user data key region
#define NT_RRAM_OTP_USER_DATA_END_KEY_REGION   0x0008005F  // user data end key region

#define NT_RRAM_OTP_HW_ENCRYPT_KEY_START_REGION 0x00080060  // hardware encryption key region
#define NT_RRAM_OTP_HW_ENCRYPT_END_KEY_REGION   0x0008006F  // hardware encryption end region

#define NT_RRAM_OTP_PK_HASH_START_REGION 0x00080070
#define NT_RRAM_OTP_PK_HASH_END_REGION   0x0008008F

#define NT_RRAM_OTP_QC_SECURE_BOOT_START_REGION 0x00080090
#define NT_RRAM_OTP_QC_SECURE_BOOT_END_REGION   0x0008009F

#define NT_RRAM_OTP_OEM_SECURE_BOOT_START_REGION 0x000800A0
#define NT_RRAM_OTP_OEM_SECURE_BOOT_END_REGION   0x000800AF

#define NT_RRAM_OTP_ANTI_ROLL_BACK_START_REGION 0x000800B0
#define NT_RRAM_OTP_ANTI_ROLL_BACK_END_REGION   0x000800BF

#define NT_RRAM_OTP_CALIBRATION_START_REGION 0x000800C0
#define NT_RRAM_OTP_CALIBRATION_END_REGION   0x000801BF

#define NT_RRAM_OTP_FIRMWARE_START_REGION 0x000801C0
#define NT_RRAM_OTP_FIRMWARE_END_REGION   0x000801CF

#define NT_RRAM_OTP_FEATURE_CONFIG_START_REGION 0x000801D0
#define NT_RRAM_OTP_FEATURE_CONFIG_END_REGION   0x000801DF

#define NT_RRAM_OTP_SPARE_START_REGION 0x000801E0
#define NT_RRAM_OTP_SPARE_END_REGION   0x000801F3

#define NT_RRAM_OTP_MEM_ACC_START_REGION 0x000801F4
#define NT_RRAM_OTP_MEM_ACC_END_REGION   0x000801F7

#define NT_RRAM_OTP_MFR_TEST1_START_REGION 0x000801F8
#define NT_RRAM_OTP_MFR_TEST1_END_REGION   0x000801FB

#define NT_RRAM_OTP_PBL_LAST_START_REGION 0x000801FC
#define NT_RRAM_OTP_PBL_LAST_END_REGION   0x000801FF

#define NT_RRAM_OTP_NPS_CONFIG_START_REGION 0x00080200
#define NT_RRAM_OTP_NPS_CONFIG_END_REGION   0x0008020F

#define NT_RRAM_OTP_RF_REINIT_START_REGION 0x00080210
#define NT_RRAM_OTP_RF_REINIT_END_REGION   0x00080217

#define NT_RRAM_OTP_EXTRA_CONFIG_START_REGION 0x00080218
#define NT_RRAM_OTP_EXTRA_CONFIG_END_REGION   0x0008021F
/**
 * main region sub-types
 */

#define NT_RRAM_MAIN_PBL_START_REGION 0x00200000  // update needed
#define NT_RRAM_MAIN_PBL_END_REGION   0x002       // update needed

#define NT_RRAM_MAIN_LOG_START_REGION 0x370000  // update needed
#define NT_RRAM_MAIN_LOG_END_REGION   0x370100  // update needed

#define NT_RRAM_MAIN_RF_CALIBR_START_REGION 0x0  // update needed
#define NT_RRAM_MAIN_RF_CALIBR_END_REGION   0x0  // update needed

#define NT_RRAM_MAIN_FILE_SYS_START_REGION 0x0  // update needed
#define NT_RRAM_MAIN_FILE_SYS_END_REGION   0x0  // update needed

// MCU SS STATE REG
#define NT_PMU_CFG_MCU_SYSTEM_DOWN_OFFSET          0x1
#define NT_PMU_CFG_MCU_SYSTEM_BOOT_COMPLETE_OFFSET 0x2
#define NT_PMU_CFG_MCU_ACTIVE_OFFSET               0x3
#define NT_PMU_CFG_MCU_SLEEP_OFFSET                0x4
#define NT_PMU_CFG_MCU_DEEPSLEEP_OFFSET            0x5

// WUR SS STATE REG
#define NT_PMU_CFG_WUR_OFF_OFFSET         0x1
#define NT_PMU_CFG_WUR_ON_OFFSET          0x2
#define NT_PMU_CFG_WUR_ON_WITH_CPU_OFFSET 0x3
#define NT_PMU_CFG_WUR_SLEEP              0x4

// WIFI SS STATE REG
#define NT_PMU_CFG_WIFI_OFF_OFFSET        0x1
#define NT_PMU_CFG_WIFI_CONFIG_OFFSET     0x2
#define NT_PMU_CFG_WIFI_TX_OFFSET         0x3
#define NT_PMU_CFG_WIFI_RXA_OFFSET        0x4
#define NT_PMU_CFG_WIFI_RXB_LISTEN_OFFSET 0x5
#define NT_PMU_CFG_WIFI_SLEEP_OFFSET      0x6
#define NT_PMU_CFG_WIFI_DEEPSLEEP_OFFSET  0x7

/* Cortex M4 CPU registers*/
#define NT_CM4_NVIC_ISER0_REG 0xE000E100  // Irq 0 to 31 set enable register address

#define NT_CM4_NVIC_ISER1_REG 0xE000E104  // Irq 32 to 63 set enable register address

#define NT_CM4_NVIC_ISER2_REG 0xE000E108  // Irq 64 to 95 set enable register address

#define NT_CM4_NVIC_ISER3_REG 0xE000E10C  // Irq 96 to 108 set enable register address

#define NT_CM4_NVIC_ISER0_STATUS_REG 0xE000E200  // Irq pending status register (0-31)

#define NT_CM4_NVIC_ISER1_STATUS_REG 0xE000E204  // Irq pending status register (32-63)

#define NT_CM4_NVIC_ISER2_STATUS_REG 0xE000E208  // Irq pending status register (64-95)

#define NT_CM4_NVIC_ISER3_STATUS_REG 0xE000E20C  // Irq pending status register (96-108)

#define NT_CM4_NVIC_ISER0_CLEAR_PENDING_REG 0xE000E280  // Clear Pending register (0-31)

#define NT_CM4_NVIC_ISER1_CLEAR_PENDING_REG 0xE000E284  // Clear Pending register (32-63)
#define NT_CM4_UART_INTERRUPT_BIT_MASK      0x00000008
#define NT_CM4_UART_INTERRUPT_BIT_OFFSET    0x00000003

#define NT_CM4__NVIC_ISER2_CLEAR_PENDING_REG 0xE000E288  // Clear Pending register (64-95)

#define NT_CM4_NVIC_ISER3_CLEAR_PENDING_REG 0xE000E28C  // Clear Pending register (96-108)

#define NT_PMU_CFG_STATE_RESOURCE_REQ_PD_SECIP_CNTL_BIT_MASK    0x2000
#define NT_PMU_CFG_STATE_RESOURCE_REQ_PD_SECIP_CNTL_BIT_OFFSET  0xD
#define NT_PMU_CFG_STATE_RESOURCE_REQ_PD_SECIP_CNTL_BIT_DEFAULT 0x0

#define NT_PMU_CFG_STATE_RESOURCE_REQ_PD_QSPI_CNTL_BIT_MASK    0x1000
#define NT_PMU_CFG_STATE_RESOURCE_REQ_PD_QSPI_CNTL_BIT_OFFSET  0xC
#define NT_PMU_CFG_STATE_RESOURCE_REQ_PD_QSPI_CNTL_BIT_DEFAULT 0x0

#define NT_PMU_CFG_STATE_RESOURCE_REQ_PD_SIF_CNTL_BIT_MASK    0x800
#define NT_PMU_CFG_STATE_RESOURCE_REQ_PD_SIF_CNTL_BIT_OFFSET  0xB
#define NT_PMU_CFG_STATE_RESOURCE_REQ_PD_SIF_CNTL_BIT_DEFAULT 0x0

#define NT_PMU_CFG_STATE_RESOURCE_REQ_PD_WUR_CNTL_BIT_MASK    0x400
#define NT_PMU_CFG_STATE_RESOURCE_REQ_PD_WUR_CNTL_BIT_OFFSET  0xA
#define NT_PMU_CFG_STATE_RESOURCE_REQ_PD_WUR_CNTL_BIT_DEFAULT 0x0

#define NT_PMU_CFG_STATE_RESOURCE_REQ_PD_WLAN_PHY_RXTOP_CNTL_BIT_MASK    0x200
#define NT_PMU_CFG_STATE_RESOURCE_REQ_PD_WLAN_PHY_RXTOP_CNTL_BIT_OFFSET  0x9
#define NT_PMU_CFG_STATE_RESOURCE_REQ_PD_WLAN_PHY_RXTOP_CNTL_BIT_DEFAULT 0x0

#define NT_PMU_CFG_STATE_RESOURCE_REQ_PD_WLAN_PHY_RXTX_CNTL_BIT_MASK    0x100
#define NT_PMU_CFG_STATE_RESOURCE_REQ_PD_WLAN_PHY_RXTX_CNTL_BIT_OFFSET  0x8
#define NT_PMU_CFG_STATE_RESOURCE_REQ_PD_WLAN_PHY_RXTX_CNTL_BIT_DEFAULT 0x0

#define NT_PMU_CFG_STATE_RESOURCE_REQ_PD_WLAN_PHY_TX_CNTL_BIT_MASK    0x80
#define NT_PMU_CFG_STATE_RESOURCE_REQ_PD_WLAN_PHY_TX_CNTL_BIT_OFFSET  0x7
#define NT_PMU_CFG_STATE_RESOURCE_REQ_PD_WLAN_PHY_TX_CNTL_BIT_DEFAULT 0x0

#define NT_PMU_CFG_STATE_RESOURCE_REQ_PD_WLAN_MAC_CNTL_BIT_MASK    0x40
#define NT_PMU_CFG_STATE_RESOURCE_REQ_PD_WLAN_MAC_CNTL_BIT_OFFSET  0x6
#define NT_PMU_CFG_STATE_RESOURCE_REQ_PD_WLAN_MAC_CNTL_BIT_DEFAULT 0x0

#define NT_PMU_CFG_STATE_RESOURCE_REQ_PD_XIP_CNTL_BIT_MASK    0x20
#define NT_PMU_CFG_STATE_RESOURCE_REQ_PD_XIP_CNTL_BIT_OFFSET  0x5
#define NT_PMU_CFG_STATE_RESOURCE_REQ_PD_XIP_CNTL_BIT_DEFAULT 0x0

#define NT_PMU_CFG_STATE_RESOURCE_REQ_PD_CMEM_BANK_D_CNTL_BIT_MASK    0x10
#define NT_PMU_CFG_STATE_RESOURCE_REQ_PD_CMEM_BANK_D_CNTL_BIT_OFFSET  0x4
#define NT_PMU_CFG_STATE_RESOURCE_REQ_PD_CMEM_BANK_D_CNTL_BIT_DEFAULT 0x0

#define NT_PMU_CFG_STATE_RESOURCE_REQ_PD_CMEM_BANK_C_CNTL_BIT_MASK    0x8
#define NT_PMU_CFG_STATE_RESOURCE_REQ_PD_CMEM_BANK_C_CNTL_BIT_OFFSET  0x3
#define NT_PMU_CFG_STATE_RESOURCE_REQ_PD_CMEM_BANK_C_CNTL_BIT_DEFAULT 0x0

#define NT_PMU_CFG_STATE_RESOURCE_REQ_PD_CMEM_BANK_B_CNTL_BIT_MASK    0x4
#define NT_PMU_CFG_STATE_RESOURCE_REQ_PD_CMEM_BANK_B_CNTL_BIT_OFFSET  0x2
#define NT_PMU_CFG_STATE_RESOURCE_REQ_PD_CMEM_BANK_B_CNTL_BIT_DEFAULT 0x0

#define NT_PMU_CFG_STATE_RESOURCE_REQ_PD_CMEM_BANK_A_CNTL_BIT_MASK    0x2
#define NT_PMU_CFG_STATE_RESOURCE_REQ_PD_CMEM_BANK_A_CNTL_BIT_OFFSET  0x1
#define NT_PMU_CFG_STATE_RESOURCE_REQ_PD_CMEM_BANK_A_CNTL_BIT_DEFAULT 0x0

#define NT_PMU_CFG_STATE_RESOURCE_REQ_PD_CMNSS_CNTL_BIT_MASK    0x1
#define NT_PMU_CFG_STATE_RESOURCE_REQ_PD_CMNSS_CNTL_BIT_OFFSET  0x0
#define NT_PMU_CFG_STATE_RESOURCE_REQ_PD_CMNSS_CNTL_BIT_DEFAULT 0x0

#define NT_PMU_CFG_AON_STATE_RESOURCE_REQ_PD_MEM_MX_OFF_CNTL_BIT_MASK  0x00
#define NT_PMU_CFG_AON_STATE_RESOURCE_REQ_PD_MEM_MX_NOR_CNTL_BIT_MASK  0x10
#define NT_PMU_CFG_AON_STATE_RESOURCE_REQ_PD_MEM_MX_RET_CNTL_BIT_MASK  0x20
#define NT_PMU_CFG_AON_STATE_RESOURCE_REQ_PD_MEM_MX_RSVD_CNTL_BIT_MASK 0x30
#define NT_PMU_CFG_AON_STATE_RESOURCE_REQ_PD_MEM_MX_CNTL_BIT_OFFSET    0x4
#define NT_PMU_CFG_AON_STATE_RESOURCE_REQ_PD_MEM_MX_CNTL_BIT_DEFAULT   0x0

#define NT_PMU_CFG_AON_STATE_RESOURCE_REQ_PD_SON_DTOP_CNTL_BIT_MASK    0x8
#define NT_PMU_CFG_AON_STATE_RESOURCE_REQ_PD_SON_DTOP_CNTL_BIT_OFFSET  0x3
#define NT_PMU_CFG_AON_STATE_RESOURCE_REQ_PD_SON_DTOP_CNTL_BIT_DEFAULT 0x0

#define NT_PMU_CFG_AON_STATE_RESOURCE_REQ_PD_RFA_DTOP_CNTL_BIT_MASK    0x4
#define NT_PMU_CFG_AON_STATE_RESOURCE_REQ_PD_RFA_DTOP_CNTL_BIT_OFFSET  0x2
#define NT_PMU_CFG_AON_STATE_RESOURCE_REQ_PD_RFA_DTOP_CNTL_BIT_DEFAULT 0x0

#define NT_PMU_CFG_AON_STATE_RESOURCE_REQ_PD_XO_DTOP_CNTL_BIT_MASK    0x2
#define NT_PMU_CFG_AON_STATE_RESOURCE_REQ_PD_XO_DTOP_CNTL_BIT_OFFSET  0x1
#define NT_PMU_CFG_AON_STATE_RESOURCE_REQ_PD_XO_DTOP_CNTL_BIT_DEFAULT 0x0

#define NT_PMU_CFG_AON_STATE_RESOURCE_REQ_PD_PMIC_DTOP_CNTL_BIT_MASK    0x1
#define NT_PMU_CFG_AON_STATE_RESOURCE_REQ_PD_PMIC_DTOP_CNTL_BIT_OFFSET  0x0
#define NT_PMU_CFG_AON_STATE_RESOURCE_REQ_PD_PMIC_DTOP_CNTL_BIT_DEFAULT 0x0

#define NT_PMU_CFG_AON_STATE_RESOURCE_REQ_DEFAULT 0X20

// PMIC config registers

#define NT_PMIC_BASE 0x2043000

/*---------------------------------------------------------------------------------------*/
#define NT_PMU_PMIC_CFG_SMPS1_0_REG (NT_PMIC_BASE + 0x0)

#define NT_PMU_PMIC_SMPS1_VSET_HIGH_MASK    0xFF
#define NT_PMU_PMIC_SMPS1_VSET_HIGH_OFFSET  0x18
#define NT_PMU_PMIC_SMPS1_VSET_HIGH_DEFAULT 0x80
/*---------------------------------------------------------------------------------------*/

#define NT_PMU_PMIC_CFG_SMPS1_1_REG (NT_PMIC_BASE + 0x4)

/*---------------------------------------------------------------------------------------*/
#define NT_PMU_PMIC_CFG_SMPS1_2_REG (NT_PMIC_BASE + 0x8)

#define NT_PMU_PMIC_SMPS1_CL_SEL_PFM_MASK    0x1
#define NT_PMU_PMIC_SMPS1_CL_SEL_PFM_OFFSET  0x17
#define NT_PMU_PMIC_SMPS1_CL_SEL_PFM_DEFAULT 0x0
/*---------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------*/
#define NT_PMU_PMIC_CFG_SMPS1_3_REG (NT_PMIC_BASE + 0xC)

#define NT_PMU_PMIC_SMPS1_VSET_SEL_MASK    0x3
#define NT_PMU_PMIC_SMPS1_VSET_SEL_OFFSET  0x4
#define NT_PMU_PMIC_SMPS1_VSET_SEL_DEFAULT 0x0
/*---------------------------------------------------------------------------------------*/

#define NT_PMU_PMIC_CFG_SMPS1_4_REG (NT_PMIC_BASE + 0x10)

/*---------------------------------------------------------------------------------------*/
#define NT_PMU_PMIC_CFG_SMPS2_0_REG (NT_PMIC_BASE + 0x14)

#define NT_PMU_PMIC_SMPS2_VSET_HIGH_MASK    0xFF
#define NT_PMU_PMIC_SMPS2_VSET_HIGH_OFFSET  0x18
#define NT_PMU_PMIC_SMPS2_VSET_HIGH_DEFAULT 0xA6
#define NT_PMU_PMIC_SMPS2_VSET_LOW_MASK     0xFF
#define NT_PMU_PMIC_SMPS2_VSET_LOW_OFFSET   0x10
#define NT_PMU_PMIC_SMPS2_VSET_LOW_DEFAULT  0x30
/*---------------------------------------------------------------------------------------*/

#define NT_PMU_PMIC_CFG_SMPS2_1_REG (NT_PMIC_BASE + 0x18)

/*---------------------------------------------------------------------------------------*/
#define NT_PMU_PMIC_CFG_SMPS2_2_REG (NT_PMIC_BASE + 0x1C)

#define NT_PMU_PMIC_SMPS2_CL_SEL_PFM_SEL_MASK    0x1
#define NT_PMU_PMIC_SMPS2_CL_SEL_PFM_SEL_OFFSET  0x17
#define NT_PMU_PMIC_SMPS2_CL_SEL_PFM_SEL_DEFAULT 0x0
/*---------------------------------------------------------------------------------------*/
#define NT_PMU_PMIC_CFG_SMPS2_3_REG (NT_PMIC_BASE + 0x20)

#define NT_PMU_PMIC_SMPS2_VSET_SEL_MASK    0x3
#define NT_PMU_PMIC_SMPS2_VSET_SEL_OFFSET  0x4
#define NT_PMU_PMIC_SMPS2_VSET_SEL_DEFAULT 0x0
/*---------------------------------------------------------------------------------------*/

#define NT_PMU_PMIC_CFG_SMPS2_4_REG (NT_PMIC_BASE + 0x24)

/*---------------------------------------------------------------------------------------*/
#define NT_PMU_PMIC_CFG_CORE_5_REG (NT_PMIC_BASE + 0x28)

#define NT_PMU_PMIC_LDO_AON_VSET_HIGH_MASK    0xFF
#define NT_PMU_PMIC_LDO_AON_VSET_HIGH_OFFSET  0x8
#define NT_PMU_PMIC_LDO_AON_VSET_HIGH_DEFAULT 0x96
/*---------------------------------------------------------------------------------------*/

#define NT_PMU_PMIC_CFG_LDORFA_0_REG (NT_PMIC_BASE + 0x2C)

#define NT_PMU_PMIC_CFG_CORE_2_REG (NT_PMIC_BASE + 0x30)

#define NT_PMU_PMIC_CFG_LDORFA_1_REG (NT_PMIC_BASE + 0x34)

#define NT_PMU_PMIC_CFG_ULPBG_1_REG (NT_PMIC_BASE + 0x38)

#define NT_PMU_PMIC_CFG_ULPBG_2_REG (NT_PMIC_BASE + 0x3C)

#define NT_PMU_PMIC_CFG_SMPS1_18_REG (NT_PMIC_BASE + 0x40)

#define NT_PMU_PMIC_CFG_SMPS2_18_REG (NT_PMIC_BASE + 0x44)

/*----------------------------------------------------------------------------------*/
#define NT_PMU_PMIC_CFG_SMPS1_5_REG (NT_PMIC_BASE + 0x48)

#define NT_PMU_PMIC_SMPS1_LPM_OVR_EN_MASK    0x3
#define NT_PMU_PMIC_SMPS1_LPM_OVR_EN_OFFSET  0x18
#define NT_PMU_PMIC_SMPS1_LPM_OVR_EN_DEFAULT 0x0
/*----------------------------------------------------------------------------------*/
#define NT_PMU_PMIC_CFG_SMPS1_6_REG (NT_PMIC_BASE + 0x4C)

#define NT_PMU_PMIC_SMPS1_POK_FORCE_MASK    0x1
#define NT_PMU_PMIC_SMPS1_POK_FORCE_OFFSET  0x0
#define NT_PMU_PMIC_SMPS1_POK_FORCE_DEFAULT 0x0

#define NT_PMU_PMIC_SMPS1_POK_DIS_MASK    0x2
#define NT_PMU_PMIC_SMPS1_POK_DIS_OFFSET  0x1
#define NT_PMU_PMIC_SMPS1_POK_DIS_DEFAULT 0x0

/*----------------------------------------------------------------------------------*/

#define NT_PMU_PMIC_CFG_SMPS1_7_REG (NT_PMIC_BASE + 0x50)

/*----------------------------------------------------------------------------------*/
#define NT_PMU_PMIC_CFG_SMPS2_5_REG (NT_PMIC_BASE + 0x54)

#define NT_PMU_PMIC_SMPS2_LPM_OVR_EN_MASK    0x3
#define NT_PMU_PMIC_SMPS2_LPM_OVR_EN_OFFSET  0x18
#define NT_PMU_PMIC_SMPS2_LPM_OVR_EN_DEFAULT 0x0
/*----------------------------------------------------------------------------------*/
#define NT_PMU_PMIC_CFG_SMPS2_6_REG (NT_PMIC_BASE + 0x58)

#define NT_PMU_PMIC_SMPS2_POK_FORCE_MASK    0x1
#define NT_PMU_PMIC_SMPS2_POK_FORCE_OFFSET  0x0
#define NT_PMU_PMIC_SMPS2_POK_FORCE_DEFAULT 0x0

#define NT_PMU_PMIC_SMPS2_POK_DIS_MASK      0x2
#define NT_PMU_PMIC_SMPS2_POK_DIS_OFFSET    0x1
#define NT_PMU_PMIC_SMPS2_POK_DIS_DEFAULT   0x0
#define NT_PMU_PMIC_CFG_SMPS2_6_REG_DEFAULT 0x0
/*----------------------------------------------------------------------------------*/

#define NT_PMU_PMIC_CFG_SMPS2_7_REG (NT_PMIC_BASE + 0x5C)

#define NT_PMU_PMIC_ULPM_SMPS2_SEG_EN_MASK    0xFE000
#define NT_PMU_PMIC_ULPM_SMPS2_SEG_EN_OFFSET  0xD
#define NT_PMU_PMIC_ULPM_SMPS2_SEG_EN_DEFAULT 0xFF
/*----------------------------------------------------------------------------------*/
#define NT_PMU_PMIC_CFG_CORE_7_REG (NT_PMIC_BASE + 0x60)

#define NT_PMU_PMIC_LDOAO_DIS_MASK    0x80000000
#define NT_PMU_PMIC_LDOAO_DIS_OFFSET  0x1F
#define NT_PMU_PMIC_LDOAO_DIS_DEFAULT 0x0

#define NT_PMU_PMIC_LDOAO_VIN_MODE_MASK    0x1
#define NT_PMU_PMIC_LDOAO_VIN_MODE_OFFSET  0x1D
#define NT_PMU_PMIC_LDOAO_VIN_MODE_DEFAULT 0x0

#define NT_PMU_PMIC_LDOAO_EN_LIM_MASK    0x10000000
#define NT_PMU_PMIC_LDOAO_EN_LIM_OFFSET  0x1C
#define NT_PMU_PMIC_LDOAO_EN_LIM_DEFAULT 0x0

#define NT_PMU_PMIC_LDOAO_LIMIT_MASK    0xF0FFFFFF
#define NT_PMU_PMIC_LDOAO_LIMIT_OFFSET  0x18
#define NT_PMU_PMIC_LDOAO_LIMIT_DEFAULT 0x0
/*----------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------*/
#define NT_PMU_PMIC_CFG_CORE_9_REG (NT_PMIC_BASE + 0x64)

#define NT_PMU_PMIC_LDOAO_POK_DIS_MASK    0x1
#define NT_PMU_PMIC_LDOAO_POK_DIS_OFFSET  0x10
#define NT_PMU_PMIC_LDOAO_POK_DIS_DEFAULT 0x0

#define NT_PMU_PMIC_LDOAO_POK_FORCE_MASK    0x1
#define NT_PMU_PMIC_LDOAO_POK_FORCE_OFFSET  0xF
#define NT_PMU_PMIC_LDOAO_POK_FORCE_DEFAULT 0x0
/*----------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------*/
#define NT_PMU_PMIC_CFG_ULPBG_6_REG (NT_PMIC_BASE + 0x68)

#define NT_PMU_PMIC_ULPBG_EN_MASK    0x1
#define NT_PMU_PMIC_ULPBG_EN_OFFSET  0x6
#define NT_PMU_PMIC_ULPBG_EN_DEFAULT 0x0

#define NT_PMU_PMIC_ULPBG_SELFSTART_DIS_MASK    0x1
#define NT_PMU_PMIC_ULPBG_SELFSTART_DIS_OFFSET  0x5
#define NT_PMU_PMIC_ULPBG_SELFSTART_DIS_DEFAULT 0x0

#define NT_PMU_PMIC_ULPBG_START_EN_MASK    0x1
#define NT_PMU_PMIC_ULPBG_START_EN_OFFSET  0x4
#define NT_PMU_PMIC_ULPBG_START_EN_DEFAULT 0x0
/*----------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------*/
#define NT_PMU_PMIC_CFG_CORE_10_REG (NT_PMIC_BASE + 0x6C)

#define NT_PMU_PMIC_LDOOSC_DIS_MASK    0x1
#define NT_PMU_PMIC_LDOOSC_DIS_OFFSET  0x15
#define NT_PMU_PMIC_LDOOSC_DIS_DEFAULT 0x0

#define NT_PMU_PMIC_LDOOSC_BIAS_SEL_MASK    0x1
#define NT_PMU_PMIC_LDOOSC_BIAS_SEL_OFFSET  0x12
#define NT_PMU_PMIC_LDOOSC_BIAS_SEL_DEFAULT 0x0

#define NT_PMU_PMIC_ROSC_32K_DIS_MASK    0x1
#define NT_PMU_PMIC_ROSC_32K_DIS_OFFSET  0x11
#define NT_PMU_PMIC_ROSC_32K_DIS_DEFAULT 0x0

#define NT_PMU_PMIC_XO_OSC_32K_DIS_MASK    0x1
#define NT_PMU_PMIC_XO_OSC_32K_DIS_OFFSET  0x10
#define NT_PMU_PMIC_XO_OSC_32K_DIS_DEFAULT 0x0
/*----------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------*/
#define NT_PMU_PMIC_CFG_CORE_11_REG (NT_PMIC_BASE + 0x70)

#define NT_PMU_PMIC_LDORFA_POK_DIS_MASK    0x1
#define NT_PMU_PMIC_LDORFA_POK_DIS_OFFSET  0x7
#define NT_PMU_PMIC_LDORFA_POK_DIS_DEFAULT 0x0

#define NT_PMU_PMIC_LDORFA_POK_FORCE_MASK    0x1
#define NT_PMU_PMIC_LDORFA_POK_FORCE_OFFSET  0x6
#define NT_PMU_PMIC_LDORFA_POK_FORCE_DEFAULT 0x0
/*----------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------*/
#define NT_PMU_PMIC_CFG_CTRL_0_REG (NT_PMIC_BASE + 0x74)

#define NT_PMU_PMIC_PMU_VBAT_SEL_MASK    0x1
#define NT_PMU_PMIC_PMU_VBAT_SEL_OFFSET  0x0
#define NT_PMU_PMIC_PMU_VBAT_SEL_DEFAULT 0x0
/*----------------------------------------------------------------------------------*/

#define NT_PMU_PMIC_CFG_PWR_OFF_REG (NT_PMIC_BASE + 0x78)

/*----------------------------------------------------------------------------------*/
#define NT_PMU_PMIC_CFG_BROWN_DET_REG (NT_PMIC_BASE + 0x7C)

#define NT_PMU_PMIC_VBAT_BROWN_EN_MASK    0x80
#define NT_PMU_PMIC_VBAT_BROWN_EN_OFFSET  0x7
#define NT_PMU_PMIC_VBAT_BROWN_EN_DEFAULT 0x0

#define NT_PMU_PMIC_VBAT_BROWN_POK_DIS_MASK    0x40
#define NT_PMU_PMIC_VBAT_BROWN_POK_DIS_OFFSET  0x6
#define NT_PMU_PMIC_VBAT_BROWN_POK_DIS_DEFAULT 0x0

#define NT_PMU_PMIC_VBAT_BROWN_POK_FORCE_MASK    0x20
#define NT_PMU_PMIC_VBAT_BROWN_POK_FORCE_OFFSET  0x5
#define NT_PMU_PMIC_VBAT_BROWN_POK_FORCE_DEFAULT 0x0
/*----------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------*/
#define NT_PMU_PMIC_CFG_FOOTSW_REG (NT_PMIC_BASE + 0x80)

#define NT_PMU_PMIC_CFG_FOOTSW_EN_MASK                    0x1
#define NT_PMU_PMIC_CFG_FOOTSW_EN_OFFSET                  0x6
#define NT_PMU_PMIC_CFG_FOOTSW_EN_DEFAULT                 0x0
#define NT_SOCPM_FOOT_SWITCH_CHARGE_RATE_SET_MASK_DEFAULT 0x3F  // foot switch resistance selection mask

/*----------------------------------------------------------------------------------*/

#define NT_PMU_PMIC_CFG_BATTMON_REG (NT_PMIC_BASE + 0x84)

#define NT_PMU_PMIC_CFG_VCM_TRIM_REG (NT_PMIC_BASE + 0x88)

/*----------------------------------------------------------------------------------*/
/* System Core Block Defs */
#define NT_SCS_BASE                (0xE000E000UL)
#define NT_SCB_BASE                (NT_SCS_BASE + 0x0D00UL)
#define NT_ICSR_REG                (NT_SCB_BASE + 0x04UL)
#define NT_SCB_ICSR_VECTACTIVE_Msk (0x1FFUL)
/*----------------------------------------------------------------------------------*/
#endif
