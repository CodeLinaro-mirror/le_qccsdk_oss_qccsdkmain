/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef NEUTRINO_PBL_SYSTEM_NT_BL_MEM_H_
#define NEUTRINO_PBL_SYSTEM_NT_BL_MEM_H_

#include <stdint.h>
#include <stdbool.h>
#include "nt_bl_env.h"
#include "fwconfig_cmn.h"
#include "nt_flags.h"

extern uint32_t __OTP_region_st_addr;
extern uint32_t __OTP_region_end_addr;
extern uint32_t __OTP_rfregion_st_addr;
extern uint32_t __OTP_rfregion_end_addr;

extern uint32_t __rram_region_start_addr;
extern uint32_t __rram_region_end_address;

extern uint32_t __fdt_reg_st_addr;
extern uint32_t __fdt_reg_end_addr;
#ifdef FERMION_OTP_SUPPORT
/* OTP region length and its locking number mapping */
typedef struct otp_region_lock_map{
    uint16_t region_size;    /* size of region */
    uint8_t  lock_num;       /* number to lock the region */
} otp_region_lock_map_t;
#endif

#ifdef FERMION_OTP_SUPPORT
typedef struct __attribute__((packed)) _PTE_REGION_{
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
}
otp_pte_region;
#else
typedef struct __attribute__((packed)) _PTE_REGION_{
    uint32_t jtag_id : 20;
    uint32_t reserved_0 : 12;

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
    uint8_t reserved_1[3];
}
otp_pte_region;
#endif

#ifdef FERMION_OTP_SUPPORT
typedef struct __attribute__((packed)) _PTE_REGION_1_{
    uint8_t reserved[16];
}
otp_pte_1_region;
#else
typedef struct __attribute__((packed)) _MANFACTURE_TEST_0_{
    uint8_t manufacture_test;
    uint8_t reserved[15];
}
otp_manfac_test0_region;
#endif
typedef struct __attribute__((packed)) _R_W_PERMISSION_{
    uint8_t read_perm[2];

    uint8_t read_per0 : 5;
    uint8_t r_reserved : 3;

    uint8_t write_perm[2];

    uint8_t write_perm0 : 5;
    uint8_t w_reserved : 3;

    uint8_t rw_reserved[10];
}
otp_rw_permission_region;

typedef struct __attribute__((packed)) _OTP_FIRM_WARE_{
#ifdef FERMION_OTP_SUPPORT
    uint8_t mac_address[6];
    uint8_t wps_pin[8];
    uint8_t firmware_reserved[2];
#else
    uint8_t mac_address[18];
    uint8_t firmware_reserved[14];
#endif
}
otp_firmware_region;

typedef struct __attribute__((packed)) _NT_OTP_REGION_{
    otp_pte_region pte_region;
#ifdef FERMION_OTP_SUPPORT
    otp_pte_1_region pte_1_region;
#else
    otp_manfac_test0_region manfac_test0;
#endif
    otp_rw_permission_region rw_permission;
    uint8_t reserved_0[16]; //hw_device_key
    uint8_t reserved_1[16]; //user_data_key
    uint8_t reserved_2[16]; //hw_encryption_key
    uint8_t reserved_3[32]; //pk_hash
    uint8_t reserved_4[16]; //qc_secure_boot
    uint8_t reserved_5[16]; //oem_secure_boot
    uint8_t reserved_6[16]; //anti_rollback
    uint8_t reserved_7[256]; //calibration
    otp_firmware_region firmware;
    uint8_t reserved_8[16]; // feature_config
    uint8_t reserved_9[20]; // spare_region13
    uint8_t reserved_10[4];  // memory_acc
    uint8_t reserved_11[4]; //manufacture_test1
    uint8_t pbl_last_address[4];
    uint8_t reserved_12[16]; // nps_config
    uint8_t reserved_13[8]; // rf_reinit_address
    uint8_t reserved_14[8]; // rram_extra_config
}
otp_nt_region;

/* this is OTP REGION lock bit mapping for example hw_device_key is region 3
 * and its lock bit is 9 whcih is there in this enum */
typedef enum _OTP_REGION_{
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
#ifdef FERMION_SILICON
    SMPS2_CONFIG, //smps2
#endif
	MAX_REGION
}
nt_otp_regions;

typedef enum _REGION_STATUS_{
	READ_LOCKED = 1,
	WRITE_LOCKED,

	MAX_LOCKED,
}
nt_otp_region_per_status;

typedef enum _FIRMWARE_REG_{
	IO_CONFIG = 0,
#ifdef FERMION_SILICON
	BOOT_METHOD,
#endif
	MAX_FIRWARE,
}
nt_otp_firmware_reserved;

typedef enum _NT_PWR_{
	PWR_DOMAIN_ON = 0,
	PWR_DOMAIN_OFF,
}
nt_pwr_domain_status;

void nt_rram_write_init( void );

void nt_rram_cache_clear( void );

/**
 * @FUNCTION : nt_bl_rram_write ()
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
int8_t nt_bl_rram_write(void *dst, const void *wdata,uint32_t length);

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
int8_t nt_bl_rram_read(void *address,void *rdata,uint32_t length);

int32_t nt_otp_region_locked(nt_otp_regions region_num, nt_otp_region_per_status rd_wr_lock);

int32_t nt_otp_region_lock( nt_otp_regions region_num, nt_otp_region_per_status rd_wr_lock );

bool nt_otp_firware_reg_status( nt_otp_firmware_reserved fw_res_flag );

int8_t nt_img_mode_update_flag( nt_img_upd_mode im_upd_flag );

int8_t nt_serial_number_check( void );

int8_t _rram_xip_status( void );

int8_t _rram_write_status( void *dst, uint32_t data );

uint8_t address_to_region_num_map(uint32_t address);

bool check_otp_region_rw_permission(uint32_t address, uint16_t length, nt_otp_region_per_status rw_lock_check);

int8_t rram_address_range_check( uintptr_t rram_add, uint32_t data_len, nt_otp_region_per_status rd_wr_lock);

int8_t nt_serial_number_write(void);

typedef void (*nt_rram_write_init_t)( void );
typedef void (*nt_rram_cache_clear_t)( void );
typedef int8_t (*nt_bl_rram_write_t)(void *dst, const void *wdata,uint32_t length);
typedef int8_t (*nt_bl_rram_read_t)(void *address,void *rdata,uint32_t length);
typedef int32_t (*nt_otp_region_locked_t)(nt_otp_regions region_num, nt_otp_region_per_status rd_wr_lock);
typedef int32_t (*nt_otp_region_lock_t)( nt_otp_regions region_num, nt_otp_region_per_status rd_wr_lock );
typedef bool (*nt_otp_firware_reg_status_t)( nt_otp_firmware_reserved fw_res_flag );
typedef int8_t (*nt_img_mode_update_flag_t)( nt_img_upd_mode im_upd_flag );
//static functions
typedef int8_t (*_rram_xip_status_t)( void );
typedef int8_t (*_rram_write_status_t)( void *dst, uint32_t data );
typedef uint8_t (*address_to_region_num_map_t)(uint32_t address);
typedef bool (*check_otp_region_rw_permission_t)(uint32_t address, uint16_t length, nt_otp_region_per_status rw_lock_check);
typedef int8_t (*rram_address_range_check_t)( uintptr_t rram_add, uint32_t data_len, nt_otp_region_per_status rd_wr_lock);
typedef int8_t (*nt_serial_number_write_t)(void);

typedef struct {
    nt_rram_cache_clear_t nt_rram_cache_clear_pfn;
    nt_bl_rram_read_t nt_bl_rram_read_pfn;
   rram_address_range_check_t rram_address_range_check_pfn;
} nt_bl_mem_ind_t;
#endif /* NEUTRINO_PBL_SYSTEM_NT_BL_MEM_H_ */
