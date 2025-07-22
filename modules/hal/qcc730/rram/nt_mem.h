/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef _NT_MEM_LOG_API_H
#define _NT_MEM_LOG_API_H

//#ifdef NT_FN_RRAM

/** standard header files */
#include <stdint.h>

#include "nt_socpm_sleep.h"  // Mem_Control

#if (NT_CHIP_VERSION == 2) || defined(PLATFORM_FERMION)
#include "dxe_api.h"
#include "nt_devcfg.h"

#define NT_READ_RRAM       0
#define NT_WRITE_RRAM      1
#define NT_READ_WRITE_RRAM 2
#define NT_RRAM_DXE_DEBUG  1
#define NT_DXE_WORD_RD     4
#endif  //(NT_CHIP_VERSION==2) || defined(PLATFORM_FERMION)

#ifdef FERMION_OTP_SUPPORT
/* OTP region length and its locking number mapping */
typedef struct otp_region_lock_map {
    uint16_t region_size; /* size of region */
    uint8_t lock_num;     /* number to lock the region */
} otp_region_lock_map_t;
#endif

#ifdef FERMION_OTP_SUPPORT
typedef struct __attribute__((packed)) _PTE_REGION_ {
    uint8_t jtag_id[4];
    uint8_t serial_num[6];
    uint8_t die_x[2];
    uint8_t die_y[2];
    uint8_t wafer_id[2];
    uint8_t lot[6];
    uint8_t site_tsmc;
    uint8_t full_cp;
    uint8_t ate_minor_ver[2];
    uint8_t ate_major_ver[2];
    uint8_t serial_updated;
    uint8_t reserved_0[2];
    uint8_t swd_id : 4;
    uint8_t reserved_1 : 4;
} otp_pte_region;

typedef struct __attribute__((packed)) _PTE_REGION_1_ {
    uint8_t reserved[16];
} otp_pte_1_region;

typedef struct __attribute__((packed)) _R_W_PERMISSION_ {
    uint8_t read_perm[2];

    uint8_t read_per0 : 5;
    uint8_t r_reserved : 3;

    uint8_t write_perm[2];

    uint8_t write_perm0 : 5;
    uint8_t w_reserved : 3;

    uint8_t rw_reserved[10];
} otp_rw_permission_region;

typedef struct __attribute__((packed)) _OTP_FIRM_WARE_ {
    uint8_t mac_address[18];
    uint8_t firmware_reserved[14];
} otp_firmware_region;

typedef struct __attribute__((packed)) _NT_OTP_REGION_ {
    otp_pte_region pte_region;
    otp_pte_1_region pte_1_region;
    otp_rw_permission_region rw_permission;
    uint8_t reserved_0[16];   // hw_device_key
    uint8_t reserved_1[16];   // user_data_key
    uint8_t reserved_2[16];   // hw_encryption_key
    uint8_t reserved_3[32];   // pk_hash
    uint8_t reserved_4[16];   // qc_secure_boot
    uint8_t reserved_5[16];   // oem_secure_boot
    uint8_t reserved_6[16];   // anti_rollback
    uint8_t reserved_7[256];  // calibration
    otp_firmware_region firmware;
    uint8_t reserved_8[16];  // feature_config
    uint8_t reserved_9[20];  // spare_region13
    uint8_t reserved_10[4];  // memory_acc
    uint8_t reserved_11[4];  // manufacture_test1
    uint8_t pbl_last_address[4];
    uint8_t reserved_12[16];  // nps_config
    uint8_t reserved_13[8];   // rf_reinit_address
    uint8_t reserved_14[8];   // rram_extra_config
} otp_nt_region;

/* this is OTP REGION lock bit mapping for example hw_device_key is region 3
 * and its lock bit is 9 whcih is there in this enum */
typedef enum _OTP_REGION_ {
    PTE_REGION = 0,
    MANUFACTURE_TEST0_REGION,
    RW_PERMISSION_REGION,
    USE__DATA_KEY_REGION,
    HW_ENCRYTION_KEY_REGION,
    ROT_HASH_REGION,
    QC_SECURE_BOOT_REGION,
    OEM_SECURE_BOOT_REGION,
    ANTI_ROLLBACK_REGION,
    HW_DEVICE_KEY_REGION,
    CALIBRATION_REGION,
    FIRMWARE_REGION,
    FEATURE_CONFIG_REGION,
    SPARE_13_REGION,
    MEMORY_ACC_REGION,
    MANUFACTURE_TEST1_REGION,
    PBL_LAST_ADD_REGION,
    NPS_CONFIG_REGION,
    PBL_REGION,
    RF_INIT_ADDR_RRAM_CONFIG_REGION,
    RF_INIT_REGION,

    MAX_REGION
} nt_otp_regions;

typedef enum _REGION_STATUS_ {
    READ_LOCKED = 1,
    WRITE_LOCKED,

    MAX_LOCKED,
} nt_otp_region_per_status;
#endif

/**
 * @FUNCTION : nt_rram_write ()
 * @brief    :
 *             This function used to write data in the RRAM memory
 * @param    :
 * 			   address : uint32_t of rram address
 *             wdata   : const void pointer datatype data need to be
 *                       write in a specific address
 *             length  : number of bytes to write
 * @return   :
 *             return 0 on the write success
 *             Error in length and wdata return -22
 *             Error in rram address range return -22
 *             Error in rram base address return -14
 *             Error in rram write return -52
 */
int8_t nt_rram_write(uint32_t address, const void *wdata, uint32_t length);

/**
 * @FUNCTION : nt_rram_read ()
 * @brief    :
 *             This function used to read data from the RRAM memory
 *
 * @param    :
 * 			   address : uint32_t of rram address
 *             rdata   : void pointer to the buffer
 *             length  : number of bytes to read
 *
 * @return   :
 *             return 0 on the write success
 *             Error in rram read return -22
 */
int8_t nt_rram_read(uint32_t address, void *rdata, uint32_t length);

/*
 * @brief Function used to write mac address in the RRAM memory
 * @return
 * 		1. SUCCESS(0) if write is success
 * 		2. FAIL(negative code) if write fails
 *
 * @remarks
 * Description:
 * 	1. nt_rram_write() byte write @address 0x801C0
 * 	2. return error if any failure
 *
 * @remarks
 * Change History:
 * 	1. Initial code
 *
 * */
int8_t nt_macid_write(uint8_t *macbuf);

/*
 * @brief Function used to read mac address from the RRAM memory
 * @return
 * 		1. SUCCESS(0) if write is success
 * 		2. FAIL(negative code) if write fails
 *
 * @remarks
 * Description:
 * 	1. nt_get_macid() byte read from address 0x801C0
 * 	2. return error if any failure
 *
 * @remarks
 * Change History:
 * 	1. Initial code
 *
 * */
int8_t nt_get_macid(uint8_t *macid);

/*
 * @brief Function used to write wps pin in the RRAM memory
 * @return
 * 		1. SUCCESS(0) if write is success
 * 		2. FAIL(negative code) if write fails
 *
 * @remarks
 * Description:
 * 	1. nt_rram_bytewrite() byte write @address 0x801C6
 * 	2. return error if any failure
 *
 * @remarks
 * Change History:
 * 	1. Initial code
 *
 * */
int8_t nt_wpspin_write(uint8_t *wpspinbuf);

/*
 * @brief Function used to read wps pin from the RRAM memory
 * @return
 * 		1. SUCCESS(0) if write is success
 * 		2. FAIL(negative code) if write fails
 *
 * @remarks
 * Description:
 * 	1. nt_get_wpspin() byte read from address 0x801C6
 * 	2. return error if any failure
 *
 * @remarks
 * Change History:
 * 	1. Initial code
 *
 * */
int8_t nt_get_wpspin(uint8_t *wpspin);

/*
 * @brief Function used to reset 0x00 to CAL address space
 * @return
 * 		1. Return 0 on success reset
 * 		2. return negative error on failure
 *
 * @remarks
 * Description:
 * 	1. nt_reset_caldata() byte write 0x00 to cal address space
 * 	2.
 *
 * @remarks
 * Change History:
 * 	1. Initial code
 *
 * */
int8_t nt_reset_caldata(void);

/*
 * @brief Function used to write cal data in the RRAM memory
 * @return
 * 		1. SUCCESS(0) if write is success
 * 		2. FAIL(negative code) if write fails
 *
 * @remarks
 * Description:
 * 	1. nt_caldata_write() byte write @address 0x20C400
 * 	2. return error if any failure
 *
 * @remarks
 * Change History:
 * 	1. Initial code
 *
 * */
int8_t nt_caldata_write(uint8_t *wcal, uint16_t offset, uint32_t length);

/*
 * @brief Function used to read cal data from the RRAM memory
 * @return
 * 		1. SUCCESS(0) if read is success
 * 		2. FAIL(negative code) if read fails
 *
 * @remarks
 * Description:
 * 	1. nt_get_caldata() byte read from address 0x20C400
 * 	2. return error if any failure
 *
 * @remarks
 * Change History:
 * 	1. Initial code
 *
 * */
int8_t nt_get_caldata(uint8_t *rcal, uint16_t offset, uint32_t length);
void nt_cmem_msdelay(uint32_t dlytick);

/**
 *@ FUNCTION : compare_memory()
 *@ Description : compares memory word by word between 1st half of bank and 2nd half of bank.
 *				returns 1 as successful and 0 as unsuccessful.
 *@ Para :  bank_start_address -  1st half starting address.
 *        bank_half_address    - 2nd half starting address.
 *
 *@return type : uint8_t
 */
uint8_t compare_memory(uint32_t *bank_start_address, uint32_t *bank_mid_address);

/**
 *@ FUNCTION : ram_bank_test()
 *@ Description : Testing the ram bank with XOR algorithm.
 *@ Algorithm : Dividing the bank into two halves and XORing every word of both halves with value of 0x01.
 *				returns 1 as successful and 0 as unsuccessful.
 *@ Para :  bank_start_address -  1st half starting address.
 *        bank_half_address    - 2nd half starting address.
 *
 *@return type : uint8_t
 */
uint8_t nt_cMEM_bank_test(uint32_t *bank_start_address, uint32_t *bank_mid_address);

/**
 *@ FUNCTION : nt_mem_bank_on
 *@ Description : Powering on the cmem domain when system boots up from cold/warm boot.
 *
 *@ Para : test_flag - 0 => no ram test, 1 => ram test needed
 *
 *@return type : void
 */
void nt_mem_bank_on(uint8_t test_flag);

/**
 *@ FUNCTION : cmem_pwr_control()
 *@ Description : Turning off the power domain of banks or putting the banks in Retention mode
 *
 *@ Para : ram_bank_num - Cmem bank segment(B,C,D).
 *@ 		pwr_mode - Power mode of Cmem (Retention,off).
 *
 *@return type :uint8_t
 */
uint8_t nt_cmem_pwr_control(uint32_t ram_bank_num, Mem_Control pwr_mode);

int8_t nt_image_mode_switch(uint8_t operational_mode);

int8_t nt_image_mode_status(void);

#if (NT_CHIP_VERSION == 2) || defined(PLATFORM_FERMION)
int8_t nt_dxe_to_h2h_rram_config(uint8_t direction, uint32_t length, void *frame, void *h2hdst);

int8_t nt_rram__dxe_config(void);
int8_t nt_rram_read_write_from_dxe(uint8_t direction);
void nt_enable_disable_rram_dxe(uint8_t rram_dxe_flg);
uint8_t nt_status_enable_disable_rram_dxe(void);
#endif  //(NT_CHIP_VERSION==2) || defined(PLATFORM_FERMION)

#ifdef FERMION_OTP_SUPPORT
int32_t nt_otp_region_locked(nt_otp_regions region_num, nt_otp_region_per_status rd_wr_lock);

int32_t nt_otp_region_lock(nt_otp_regions region_num, nt_otp_region_per_status rd_wr_lock);
#endif

//#endif //NT_FN_RRAM
#endif  //_NT_MEM_LOG_API_H
