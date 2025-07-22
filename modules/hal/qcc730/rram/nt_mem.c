/**
 *Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "fwconfig_cmn.h"
#include "nt_flags.h"
#include "safeAPI.h"

// --------------------------------------------------------------
#ifdef NT_FN_RRAM
// --------------------------------------------------------------

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

#include "nt_common.h"

#include "nt_hw_support.h"
#include "nt_hw.h"

#include "uart.h"
#include "nt_logger_api.h"
#include "dxe_api.h"
#include "nt_socpm_sleep.h"
#include "nt_mem.h"
#include "nt_sys_monitoring.h"

#ifdef RRAM_WRITE_VIA_DXE
#include "dxe.h"
#endif /* RRAM_WRITE_VIA_DXE */

#if (NT_CHIP_VERSION == 2) || defined(PLATFORM_FERMION)

dxe_channel_cfg_t pcfg;
#endif  //(NT_CHIP_VERSION==2) || defined(PLATFORM_FERMION)

#define NT_MEM_SUCCESS 0
/** retry count when PA on */
#define _MEM_RRAM_NRETRY 5

/** Ram bank test Macros */
#define _MEM_RAM_BANK_NITERATIONS 0x4000
#define _MEM_XOR_TEST_INIT_VAL    0x00000001

/** Systick macros */
#define _MEM_SYSTICK_RELOAD_ADDR 0xE000E014
#define _MEM_SYSTICK_CTRL_ADDR   0xE000E010
#define _MEM_SYSTICK_VAL_ADDR    0xE000E018

#define _MEM_SYSTICK_RELOAD_VAL 0xFFFFFF  // ~128ms @ 60MHz
#define _MEM_SYSTICK_CFG_VAL    0x05
#define _MEM_SYSTICK_STOP_VAL   0x0
#define _MEM_SYSTICK_RESET_VAL  0

#define _MEM_RRAM_MAIN_ADDR_CHK(addr) \
    ((addr >= (uintptr_t)(&__rram_region_start_addr)) && (addr < (uintptr_t)(&__rram_region_end_address)))
#define _MEM_RRAM_OTP_ADDR_CHK(addr) \
    ((addr >= (uintptr_t)(&__OTP_region_st_addr)) && (addr < (uintptr_t)(&__OTP_region_end_addr)))

#define _MEM_TX_DUR_WR \
    ((NT_REG_RD(QWLAN_RRAM_CTRL_RRAM_TX_DUR_WR_REG) & QWLAN_RRAM_CTRL_RRAM_TX_DUR_WR_STATUS_MASK) ? true : false)
#define _MEM_WR_DUR_TX \
    ((NT_REG_RD(QWLAN_RRAM_CTRL_RRAM_WR_DUR_TX_REG) & QWLAN_RRAM_CTRL_RRAM_WR_DUR_TX_STATUS_MASK) ? true : false)

#define _MEM_RRAM_WR_BAD (_MEM_TX_DUR_WR | _MEM_WR_DUR_TX)

#ifdef PLATFORM_FERMION
#define _MEM_QC_FORMED_MARKER_ADDR (0x1A0016)
#else
#define _MEM_QC_FORMED_MARKER_ADDR (0x80016)
#endif  // PLATFORM_FERMION
#define _MEM_QC_FORMED_MARKER_VAL (0x51)
#define _MEM_RRAM_WR_CHK_VALUE    (0xA5)

#ifdef PLATFORM_FERMION
#define _MEM_RRAM_MAC_ADDR (0x1A01C0)
#else
#define _MEM_RRAM_MAC_ADDR (0x801C0)
#endif  // PLATFORM_FERMION
#define _MEM_MAC_ID_LEN (6)
#ifdef PLATFORM_FERMION
#define _MEM_RRAM_WPSPIN_ADDR (0x1A01C6)
#else
#define _MEM_RRAM_WPSPIN_ADDR (0x801C6)
#endif  // PLATFORM_FERMION
#define _MEM_WPSPIN_LEN (8)

#define NT_IMAGE_MODE_CONFIG_ADDR   (0x2043FC)  // last 4 bytes of FDT
#define NT_IMAGE_MODE_CONFIG_LENGTH (4)

#ifdef RRAM_WRITE_VIA_DXE
#ifdef UNIT_TEST_SUPPORT
extern uint32_t verify_write_through_read;
#endif
#define MAX_WRITE_TRY             3
#define DXE_CH_RRAM_WRITE_VIA_DXE 7
#define MAX_WAIT_FOR_WRITE        300 /* it takes appox 64 ms sec to write 16k byte of data */
#define RRAM_WRITE_ADR_BYTE_ALIGN \
    16 /* RRAM write address should be 16 byte aligned and write length should be multiple of 16 byte */
#define RRAM_R_W_BUFF_SIZE                                                                                     \
    256 /* buffer used for padding / partial write case. Must be multiple of 16 byte. This can be adjusted for \
           PBL/SBL/APP */
#define RRAM_WRITE_BLOCK_MAX_SIZE \
    16368 /* this is maximum number of bytes DXE can write using a discriptor. it is 0x3FF0 */
static int8_t nt_rram_write_with_retry(uint32_t dst, const void *wdata, uint32_t length);
static int8_t nt_rram_write_per_block(uint32_t dst, const void *wdata, uint32_t length);
__attribute__((aligned(4))) uint8_t temp_r_w_buff[RRAM_R_W_BUFF_SIZE]; /* this should be algined to 4 bytes */
uint8_t dxe_deinit = 0;
#endif /* RRAM_WRITE_VIA_DXE */

extern uint32_t __rram_region_start_addr;
extern uint32_t __rram_region_end_address;
extern uint32_t __OTP_region_st_addr;
extern uint32_t __OTP_region_end_addr;

#if (NT_CHIP_VERSION == 2) || defined(PLATFORM_FERMION)
uint8_t rram_dxe_status = 0;
#endif  //(NT_CHIP_VERSION==2) || defined(PLATFORM_FERMION)
/** ram bank starting and mid address of bank B, C & D*/
extern unsigned int _ln_cMEM_Bank_B_start_addr__;
extern unsigned int _ln_cMEM_Bank_C_start_addr__;
extern unsigned int _ln_cMEM_Bank_D_start_addr__;
extern unsigned int _ln_cMEM_Bank_size__;

extern uint32_t _ln_CAL_Start_Addr;
extern uint32_t _ln_CAL_Data_length;

#ifdef RRAM_WRITE_VIA_DXE
void delay(uint32_t delay_count);
#endif /* RRAM_WRITE_VIA_DXE */

#ifdef NT_MISSION_FACTORY_MODE
enum {

    MISSION_MODE,
    FACTORY_MODE,
} OPERATION_MODE_VALUE;

enum {

    IMAGE_MODE_VALUE_SAME = 2,
    WRONG_IMAGE_MODE_VALUE = 3,
} OPERATIONAL_MODE_RETURN_VALUE;
#endif  // NT_MISSION_FACTORY_MODE

#ifdef FERMION_OTP_SUPPORT
/* first column is size of each region. region number starts with index 0.
 *  to lock each region we have bit, however this bit mapping is not one to one with region number.
 *  for example the HW_DEVICE_KEY_REGION the region number is 3 lock bit number is 9
 *  this table gives mapping between region number and its lock bit number.
 */
static const otp_region_lock_map_t g_otp_region_lock_map[] = {
    {32, 0},    /* PTE_REGION */
    {16, 1},    /* MANUFACTURE_TEST0_REGION */
    {16, 2},    /* RW_PERMISSION_REGION */
    {16, 9},    /* HW_DEVICE_KEY_REGION */
    {16, 3},    /* USE__DATA_KEY_REGION */
    {16, 4},    /* HW_ENCRYTION_KEY_REGION */
    {32, 5},    /* ROT_HASH_REGION */
    {16, 6},    /* QC_SECURE_BOOT_REGION */
    {16, 7},    /* OEM_SECURE_BOOT_REGION */
    {16, 8},    /* ANTI_ROLLBACK_REGION */
    {256, 10},  /* CALIBRATION_REGION */
    {32, 11},   /* FIRMWARE_REGION */
    {16, 13},   /* SPARE_13_REGION */
    {16, 16},   /* PBL_LAST_ADD_REGION */
    {64, 17},   /* NPS_CONFIG_REGION */
    {16, 19},   /* RF_INIT_ADDR_RRAM_CONFIG_REGION */
    {16, 14},   /* MEMORY_ACC_REGION */
    {16, 15},   /* MANUFACTURE_TEST1_REGION */
    {16, 12},   /* FEATURE_CONFIG_REGION */
    {3456, 20}, /* RF_INIT_REGION */
};
#endif
#ifndef RRAM_WRITE_VIA_DXE
static int8_t _rram_write_status(void *dst, uint32_t data);
#endif
static int8_t _mem_rram_xip_status(void);

#ifdef FERMION_OTP_SUPPORT
/*
 * @brief returns the region no based on address.
 * @param address : address of OTP
 * @return region number in case a valid region number is found
 *         other wise its the maximum region number. for example
 *         if number of region is 'x' and if a valid region is found
 *         it will retrun 0 to x-1 and in case it doesnot find any
 *         region then it would retrun x.
 *
 */
static uint8_t address_to_region_num_map(uint32_t address)
{
    uint8_t num_region;
    uint8_t region_count;
    uint32_t region_start_addr;
    uint32_t region_end_addr;

    num_region = sizeof(g_otp_region_lock_map) / sizeof(otp_region_lock_map_t);
    region_end_addr = (uintptr_t)(&__OTP_region_st_addr);
    for (region_count = 0; region_count < num_region; region_count++) {
        region_start_addr = region_end_addr;
        region_end_addr = region_start_addr + g_otp_region_lock_map[region_count].region_size;

        if ((region_start_addr <= address) && (address < region_end_addr)) {
            break;
        }
    }
    return (region_count);
}

/*
 * @brief function checks if a OTP address is having read/write permission.
 *        incase the address range is accross the otp region then all the region
 *        permission has to be same in order to be able to read or write.
 * @param address : starting address of OTP to be accessed
 * @param length : number of bytes to be accessed
 * @param rw_lock_check : read or write lock check
 * @return if all region have same access then it will return TRUE else FALSE
 *
 */
static bool check_otp_region_rw_permission(uint32_t address, uint16_t length, nt_otp_region_per_status rw_lock_check)
{
    uint32_t otp_start_addr;
    uint32_t otp_end_addr;
    uint8_t starting_region_no;
    uint8_t ending_region_no;
    uint8_t region_count;
    uint8_t num_region;
    bool ret_value = true;

    num_region = sizeof(g_otp_region_lock_map) / sizeof(otp_region_lock_map_t);
    otp_start_addr = address;
    otp_end_addr = otp_start_addr + length;
    starting_region_no = address_to_region_num_map(otp_start_addr);
    ending_region_no = address_to_region_num_map(otp_end_addr);

    /* check if region number is with in range */
    if ((starting_region_no >= num_region) || (ending_region_no >= num_region)) {
        return (false);
    }

    /* the read/write control region is always avilable for read without that
     * read / write permission is not available */
    if ((starting_region_no == ending_region_no) && (starting_region_no == RW_PERMISSION_REGION) &&
        (rw_lock_check == READ_LOCKED)) {
        return (true);
    }

    for (region_count = starting_region_no; region_count <= ending_region_no; region_count++) {
        if (nt_otp_region_locked(g_otp_region_lock_map[region_count].lock_num, rw_lock_check) != false) {
            ret_value = false;
        }
    }
    return (ret_value);
}

#endif

#ifdef FERMION_OTP_SUPPORT
static inline int8_t rram_address_range_check(uintptr_t rram_add, uint32_t data_len,
                                              nt_otp_region_per_status rd_wr_lock)
#else
static inline int8_t rram_address_range_check(uintptr_t rram_add, uint32_t data_len)
#endif
{
#ifdef FERMION_OTP_SUPPORT
    /* OTP region can be read or write locked check for the same in case address range
     * of rram is with in otp region */
    if (_MEM_RRAM_OTP_ADDR_CHK(rram_add) && _MEM_RRAM_OTP_ADDR_CHK(rram_add + data_len)) {
        if (check_otp_region_rw_permission(rram_add, data_len, rd_wr_lock) == true) {
            return 0;
        } else {
            nt_dbg_print("region check failed\r\n");
            return (-EINVAL);
        }
    } else {
        /* in case RRAM and OTP is partially overlapping return error */
        if (_MEM_RRAM_OTP_ADDR_CHK(rram_add) || _MEM_RRAM_OTP_ADDR_CHK(rram_add + data_len)) {
            return (-EINVAL);
        }
    }
#endif
    if ((!(_MEM_RRAM_MAIN_ADDR_CHK(rram_add) || _MEM_RRAM_OTP_ADDR_CHK(rram_add))) ||
        (!(_MEM_RRAM_MAIN_ADDR_CHK(rram_add + data_len) || _MEM_RRAM_OTP_ADDR_CHK(rram_add + data_len)))) {
        return (-EINVAL);
    } else {
        return 0;
    }
}

typedef struct addr_val_pair_s {
    uint32_t address;
    uint32_t value;
} addr_val_pair_t;

// Configuration sequence for RRAM formed by QC
static const addr_val_pair_t rram_trc_config_qc[] = {
    // Configure TRC Registers
    {0x00081000, 0x01},       {0x000810C0, 0x00003A14}, {0x00081090, 0x000007DD},
    {0x00081020, 0x00160400}, {0x00081024, 0x00000000},
};

// Configuration sequence for RRAM formed by the fab
static const addr_val_pair_t rram_trc_config[] = {
    // Configure TRC Registers
    {0x00081000, 0x01},
    {0x000810C0, 0x00003A14},
};

/**
 * @FUNCTION : nt_rram_write ()
 * @brief    :
 *             This function used to write data in the RRAM memory
 * @param    :
 *             address : uint32_t of rram address
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
#ifdef RRAM_WRITE_VIA_DXE
/* this is main function which writes to RRAM it calls nt_rram_write_per_block function to do actual write */
int8_t nt_rram_write(uint32_t dst, const void *wdata, uint32_t length)
{
    int8_t status = 0;
    uint16_t count = 0;
    uint16_t loop = 0;
    uint16_t full_block_num = 0;     /* this is number of full block of size RRAM_WRITE_BLOCK_MAX_SIZE */
    uint16_t partial_block_size = 0; /* size in bytes of partial block */
    uint16_t num_pad_bytes = 0;      /* padding bytes to be added */
    uint32_t des_adr;                /* local copy of destination address */
    uint8_t *src_adr;                /* local copy of write buffer address */
    uint32_t remaining_byte;
    uint16_t temp_num_byte;
    uint8_t num_byte_for_adr_aligned; /* number of bytes which is less in alignment of write address */

    /* error checks */
    if ((wdata == NULL) || (length == 0)) {
        NT_LOG_PRINT(COMMON, ERR, "RRAM Write error wdata 0x%X length %d", (uint32_t)wdata, length);
        status = -EINVAL;
        return (status);
    }

    /* check RRAM address range */
    if (rram_address_range_check(dst, length, WRITE_LOCKED)) {
        NT_LOG_PRINT(COMMON, ERR, "RRAM is out of range dst 0x%X length %d", dst, length);
        return (-EFAULT);
    }
#ifdef FLASH_XIP_SUPPORT
    uint32_t value = NT_REG_RD(
        QWLAN_RRAM_CTRL_RRAM_CTRL_TEST_REG);  // enable all D-code read data access to be cached for dv purpose only.
    value &= (~(0x1 << QWLAN_RRAM_CTRL_RRAM_CTRL_TEST_SW_CACHE_TEST_MODE_OFFSET));
    NT_REG_WR(QWLAN_RRAM_CTRL_RRAM_CTRL_TEST_REG, value);
#endif  // NT_FN_QSPI_FLASH

    des_adr = dst;
    src_adr = (uint8_t *)wdata;
    remaining_byte = length;

    NT_LOG_PRINT(COMMON, INFO, "RRAM write dst address 0x%X length in bytes %d", dst, length);

    /* check if RRAM address is RRAM_WRITE_ADR_BYTE_ALIGN byte aligned or source address is not aligned */
    if (((dst % RRAM_WRITE_ADR_BYTE_ALIGN) != 0) || ((((uint32_t)src_adr) % 4) != 0)) {
        NT_LOG_PRINT(COMMON, ERR, "RRAM dst address or source is not algined dst 0x%X src 0x%X", dst,
                     (uint32_t)src_adr);

        /* go back in RRAM address so that it is algined to RRAM_WRITE_ADR_BYTE_ALIGN */
        des_adr =
            des_adr - (dst % RRAM_WRITE_ADR_BYTE_ALIGN); /* destimation adress is RRAM_WRITE_ADR_BYTE_ALIGN algined */

        /* read RRAM_WRITE_ADR_BYTE_ALIGN byte values from above address */
        nt_rram_read(des_adr, temp_r_w_buff, RRAM_WRITE_ADR_BYTE_ALIGN);

        /* update the actual values to be written */
        num_byte_for_adr_aligned = RRAM_WRITE_ADR_BYTE_ALIGN - (dst % RRAM_WRITE_ADR_BYTE_ALIGN);

        if (remaining_byte > num_byte_for_adr_aligned) {
            temp_num_byte = num_byte_for_adr_aligned;
            remaining_byte = remaining_byte - num_byte_for_adr_aligned;
        } else {
            temp_num_byte = remaining_byte;
            remaining_byte = 0; /* all bytes are written in this condition only */
        }

        for (count = (dst % RRAM_WRITE_ADR_BYTE_ALIGN); count < (dst % RRAM_WRITE_ADR_BYTE_ALIGN) + temp_num_byte;
             count++) {
            temp_r_w_buff[count] = src_adr[loop];
            loop++;
        }

        /* write RRAM_WRITE_ADR_BYTE_ALIGN bytes of data */
        status = nt_rram_write_with_retry(des_adr, temp_r_w_buff, RRAM_WRITE_ADR_BYTE_ALIGN);

        if (status != 0) {
            /* RRAM write failed */
            NT_LOG_PRINT(COMMON, ERR, "RRAM write failed for non aligned destination address %d src 0x%X dst 0x%X",
                         RRAM_WRITE_ADR_BYTE_ALIGN, (uint32_t)temp_r_w_buff, des_adr);
            return (status);
        }

        /* RRAM_WRITE_ADR_BYTE_ALIGN bytes write is done, check if we have more bytes to be written */
        if (remaining_byte) {
            /* update the source and destination address */
            des_adr = des_adr + RRAM_WRITE_ADR_BYTE_ALIGN;
            src_adr =
                src_adr +
                temp_num_byte; /* source address may or may not be aligned and based on that write path will change */

            if (((uint32_t)src_adr % 4) != 0) {
                /* this is a special case where destination address and source address are not aligned together */
                /* copy the source data in block of RRAM_R_W_BUFF_SIZE to temp buffer which is algined and write to RRAM
                 */
                /* complete copy of remaining data will happen in this case only */
                full_block_num = remaining_byte / RRAM_R_W_BUFF_SIZE;

                partial_block_size = remaining_byte % RRAM_R_W_BUFF_SIZE;

                for (count = 0; count < full_block_num; count++) {
                    /* copy source data to temp buffer as source address is not algined */
                    for (loop = 0; loop < RRAM_R_W_BUFF_SIZE; loop++) {
                        temp_r_w_buff[loop] = src_adr[loop];
                    }

                    status = nt_rram_write_with_retry(des_adr, temp_r_w_buff, RRAM_R_W_BUFF_SIZE);

                    if (status != 0) {
                        /* RRAM write failed */
                        NT_LOG_PRINT(COMMON, ERR, "RRAM special case write failed for block %d src 0x%X dst 0x%X",
                                     count, (uint32_t)temp_r_w_buff, des_adr);
                        return (status);
                    }
                    /* update the address */
                    des_adr = des_adr + RRAM_R_W_BUFF_SIZE;
                    src_adr = src_adr + RRAM_R_W_BUFF_SIZE;
                }

                if (partial_block_size != 0) {
                    /* still have some bytes left */
                    nt_rram_read(des_adr, temp_r_w_buff, RRAM_R_W_BUFF_SIZE);

                    temp_num_byte = partial_block_size % RRAM_R_W_BUFF_SIZE;

                    /* overwite the data to be written */
                    for (count = 0; count < temp_num_byte; count++) {
                        temp_r_w_buff[count] = src_adr[count];
                    }

                    /* write to RRAM */
                    status = nt_rram_write_with_retry(des_adr, temp_r_w_buff, RRAM_R_W_BUFF_SIZE);

                    if (status != 0) {
                        /* RRAM write failed */
                        NT_LOG_PRINT(COMMON, ERR,
                                     "RRAM write of spacial case failed for padded partial block %d src 0x%X dst 0x%X",
                                     (partial_block_size), (uint32_t)temp_r_w_buff, des_adr);
                        return (status);
                    }
                }
                /* all copy of special case is done */
                return (status);
            }
        } else {
            return (status);
        }
    }

    full_block_num = remaining_byte / RRAM_WRITE_BLOCK_MAX_SIZE;

    partial_block_size = remaining_byte % RRAM_WRITE_BLOCK_MAX_SIZE;

    /* check if partial_block_size is not multiple of RRAM_WRITE_ADR_BYTE_ALIGN bytes */
    /* if not then add necessary padding */
    if ((partial_block_size % RRAM_WRITE_ADR_BYTE_ALIGN) != 0) {
        num_pad_bytes = RRAM_WRITE_ADR_BYTE_ALIGN - (partial_block_size % RRAM_WRITE_ADR_BYTE_ALIGN);
    }

    /* write number of full block */
    for (count = 0; count < full_block_num; count++) {
        status = nt_rram_write_with_retry(des_adr, src_adr, RRAM_WRITE_BLOCK_MAX_SIZE);

        if (status != 0) {
            /* RRAM write failed */
            NT_LOG_PRINT(COMMON, ERR, "RRAM write failed for block %d src 0x%X dst 0x%X", count, (uint32_t)src_adr,
                         des_adr);
            return (status);
        }
        /* update the address */
        des_adr = des_adr + RRAM_WRITE_BLOCK_MAX_SIZE;
        src_adr = src_adr + RRAM_WRITE_BLOCK_MAX_SIZE;
    }

    /* write partial block */
    if (partial_block_size != 0) {
        /* partial block is there */
        if (num_pad_bytes == 0) {
            /* partial block is byte aligned as per requirement */
            status = nt_rram_write_with_retry(des_adr, src_adr, partial_block_size);

            if (status != 0) {
                /* RRAM write failed */
                NT_LOG_PRINT(COMMON, ERR, "RRAM write failed for partial block %d src 0x%X dst 0x%X",
                             partial_block_size, (uint32_t)src_adr, des_adr);
                return (status);
            }

        } else {
            /* partial block which is not byte aligned as per rquirement */
            /* if partial block size is 200 bytes then 12*16 = 192 bytes will be written first and rest 8 bytes would be
             * written with padding */

            /* write number of bytes which are multiple of RRAM_WRITE_ADR_BYTE_ALIGN */
            temp_num_byte = partial_block_size / RRAM_WRITE_ADR_BYTE_ALIGN;
            temp_num_byte = temp_num_byte *
                            RRAM_WRITE_ADR_BYTE_ALIGN; /* NUM bytes which are multiple of RRAM_WRITE_ADR_BYTE_ALIGN */

            if (temp_num_byte != 0) {
                /* write to RRAM */
                status = nt_rram_write_with_retry(des_adr, src_adr, temp_num_byte);
                if (status == 0) {
                    /* update the address */
                    des_adr = des_adr + temp_num_byte;
                    src_adr = src_adr + temp_num_byte;
                } else {
                    /* RRAM write failed */
                    NT_LOG_PRINT(COMMON, ERR, "RRAM write failed for padded partial block %d src 0x%X dst 0x%X",
                                 (partial_block_size), (uint32_t)src_adr, des_adr);
                    return (status);
                }
            }

            /* read RRAM_WRITE_ADR_BYTE_ALIGN byte values from above address */
            nt_rram_read(des_adr, temp_r_w_buff, RRAM_WRITE_ADR_BYTE_ALIGN);

            temp_num_byte = partial_block_size % RRAM_WRITE_ADR_BYTE_ALIGN; /* number of bytes which is not aligned */

            /* overwite the data to be written */
            for (count = 0; count < temp_num_byte; count++) {
                temp_r_w_buff[count] = src_adr[count];
            }

            /* write to RRAM */
            status = nt_rram_write_with_retry(des_adr, temp_r_w_buff, RRAM_WRITE_ADR_BYTE_ALIGN);

            if (status != 0) {
                /* RRAM write failed */
                NT_LOG_PRINT(COMMON, ERR, "RRAM write failed for padded partial block %d src 0x%X dst 0x%X",
                             (partial_block_size), (uint32_t)src_adr, des_adr);
                return (status);
            }
        }
    }
#ifdef FLASH_XIP_SUPPORT
    value = NT_REG_RD(
        QWLAN_RRAM_CTRL_RRAM_CTRL_TEST_REG);  // enable all D-code read data access to be cached for dv purpose only.
    value |= (0x1 << QWLAN_RRAM_CTRL_RRAM_CTRL_TEST_SW_CACHE_TEST_MODE_OFFSET);
    NT_REG_WR(QWLAN_RRAM_CTRL_RRAM_CTRL_TEST_REG, value);
#endif  // NT_FN_QSPI_FLASH

    return (status);
}

static int8_t nt_rram_write_with_retry(uint32_t dst, const void *wdata, uint32_t length)
{
    uint8_t write_retry_count;
    int8_t status = 0;

    for (write_retry_count = 0; write_retry_count < MAX_WRITE_TRY; write_retry_count++) {
        status = nt_rram_write_per_block(dst, wdata, length);
        if (status == 0) {
            /* RRAM write is successful */
            break;
        }
    }

    return (status);
}

/* this code is written taking reference as dxe_rram_test.c provided by VI team for their verification */
/* it writes one block of data to RRAM and the maximum write size can be 16368 bytes */
static int8_t nt_rram_write_per_block(uint32_t dst, const void *wdata, uint32_t length)
{
    int8_t status = 0;
    uint32_t ch_sz = 0;
    uint32_t regVal;
    uint32_t xfrStatus;
    uint32_t DxeChannel_BaseAddr = 0;
    uint32_t max_loop = MAX_WAIT_FOR_WRITE;
    //    uint32_t temp_regVal[6];
    DXEDesc_t dxe_hw_desc;

    if (dxe_deinit == 1) {
        nt_ndxe_init();
        dxe_deinit = 0;
    }

    memset(&dxe_hw_desc, 0, sizeof(dxe_hw_desc));
#if 0
    /* store all registers which is going to be modifed in this call. need a review on the reg which are not needed */
    temp_regVal[0] = HW_REG_RD(QWLAN_PMU_DIG_TOP_CFG_REG);
    temp_regVal[1] = HW_REG_RD(QWLAN_DXE_0_DMA_CSR_REG);
    temp_regVal[2] = HW_REG_RD(DxeChannel_BaseAddr + (QWLAN_DXE_0_CH0_SZ_REG - QWLAN_DXE_0_CH0_CTRL_REG));
    temp_regVal[3] = HW_REG_RD(DxeChannel_BaseAddr+NT_DXE_CH_DESCH_REG);
    temp_regVal[4] = HW_REG_RD(DxeChannel_BaseAddr+NT_DXE_CH_DESCL_REG);
    temp_regVal[5] = HW_REG_RD(DxeChannel_BaseAddr);
#endif
    /* Clear the32 bit legacy writes to RRAM */
    regVal = HW_REG_RD(QWLAN_PMU_DIG_TOP_CFG_REG);
    regVal = regVal & (~QWLAN_PMU_DIG_TOP_CFG_RRAM_32_BIT_LEGACY_WR_MODE_MASK);
    regVal = regVal | (QWLAN_PMU_DIG_TOP_CFG_RRAM_PD_MODE_DEFAULT); /* this was different between MM and VI code hence
                                                                       making it same 4000080 vs 0x4002080 */
    HW_REG_WR(QWLAN_PMU_DIG_TOP_CFG_REG, regVal);

#ifdef RRAM_WRITE_VIA_DXE_DEBUG
    NT_LOG_PRINT(COMMON, ERR, "QWLAN_PMU_DIG_TOP_CFG_REG = 0x%X 0x%X", QWLAN_PMU_DIG_TOP_CFG_REG,
                 HW_REG_RD(QWLAN_PMU_DIG_TOP_CFG_REG));
#endif
    /* Clear the 128 Bit write Disable */
    regVal = HW_REG_RD(QWLAN_DXE_0_DMA_CSR_REG);
    regVal = regVal & (~QWLAN_DXE_0_DMA_CSR_DIS_RRAM_128BIT_MASK);
    HW_REG_WR(QWLAN_DXE_0_DMA_CSR_REG, regVal);

#ifdef RRAM_WRITE_VIA_DXE_DEBUG
    NT_LOG_PRINT(COMMON, ERR, "QWLAN_DXE_0_DMA_CSR_REG = 0x%X 0x%X", QWLAN_DXE_0_DMA_CSR_REG,
                 HW_REG_RD(QWLAN_DXE_0_DMA_CSR_REG));
#endif

    /* create DXE HW discriptor */
    dxe_hw_desc.ctrl = NT_DXE_DESC_CTRL_INT | NT_DXE_DESC_CTRL_EOP |
                       ((NT_DXE_DESC_CTRL_XTYPE_H2H << 1) & NT_DXE_DESC_CTRL_XTYPE_MASK) | NT_DXE_DESC_CTRL_VALID;
    dxe_hw_desc.xfrSize = length;
    dxe_hw_desc.dxedesc.dxe_short_desc.srcMemAddrL = (uint32_t)wdata;
    dxe_hw_desc.dxedesc.dxe_short_desc.dstMemAddrL = dst;
    dxe_hw_desc.dxedesc.dxe_short_desc.phyNextL = 0;

#ifdef RRAM_WRITE_VIA_DXE_DEBUG
    NT_LOG_PRINT(COMMON, ERR, "ctrl 0x%X xferSize 0x%X src 0x%X dst 0x%X next 0x%X", dxe_hw_desc.ctrl,
                 dxe_hw_desc.xfrSize, dxe_hw_desc.dxedesc.dxe_short_desc.srcMemAddrL,
                 dxe_hw_desc.dxedesc.dxe_short_desc.dstMemAddrL, dxe_hw_desc.dxedesc.dxe_short_desc.phyNextL);
#endif

    /* hardcoding to be removed */
    HW_REG_WR(QWLAN_DXE_0_DMA_CSR_REG,
              0x2000a001); /* not sure if above write will be effective it does not clear 128 bit write disable */

    DxeChannel_BaseAddr = QWLAN_DXE_0_CH0_CTRL_REG + (NT_DXE_CH_REG_SIZE * DXE_CH_RRAM_WRITE_VIA_DXE);

#ifdef RRAM_WRITE_VIA_DXE_DEBUG
    NT_LOG_PRINT(COMMON, ERR, "QWLAN_DXE_0_DMA_CSR_REG = 0x%X 0x%X DXE BA 0x%X", QWLAN_DXE_0_DMA_CSR_REG,
                 HW_REG_RD(QWLAN_DXE_0_DMA_CSR_REG), DxeChannel_BaseAddr);
#endif

    /* Channel enable with describtor */
    ch_sz = HW_REG_RD(DxeChannel_BaseAddr + (QWLAN_DXE_0_CH0_SZ_REG -
                                             QWLAN_DXE_0_CH0_CTRL_REG)); /* effectivally reading the DXE_CHX_SZ_REG */
    HW_REG_WR(DxeChannel_BaseAddr + (QWLAN_DXE_0_CH0_SZ_REG - QWLAN_DXE_0_CH0_CTRL_REG),
              ch_sz | (4 << QWLAN_DXE_0_CH0_SZ_CHK_SZ_OFFSET));

#ifdef RRAM_WRITE_VIA_DXE_DEBUG
    NT_LOG_PRINT(COMMON, ERR, "Channel Enable with Discriptor 0x%X 0x%X",
                 (DxeChannel_BaseAddr + (QWLAN_DXE_0_CH0_SZ_REG - QWLAN_DXE_0_CH0_CTRL_REG)),
                 HW_REG_RD(DxeChannel_BaseAddr + (QWLAN_DXE_0_CH0_SZ_REG - QWLAN_DXE_0_CH0_CTRL_REG)));
#endif

    HW_REG_WR(DxeChannel_BaseAddr + NT_DXE_CH_DESCH_REG, 0);
    HW_REG_WR(DxeChannel_BaseAddr + NT_DXE_CH_DESCL_REG, (uint32_t)(&dxe_hw_desc));
    HW_REG_WR(DxeChannel_BaseAddr, 0xf80709);

#ifdef RRAM_WRITE_VIA_DXE_DEBUG
    NT_LOG_PRINT(COMMON, ERR, "DxeChannel_BaseAddr+NT_DXE_CH_DESCH_REG 0x%X 0x%X",
                 (DxeChannel_BaseAddr + NT_DXE_CH_DESCH_REG), HW_REG_RD(DxeChannel_BaseAddr + NT_DXE_CH_DESCH_REG));
    NT_LOG_PRINT(COMMON, ERR, "DxeChannel_BaseAddr+NT_DXE_CH_DESCL_REG 0x%X 0x%X",
                 (DxeChannel_BaseAddr + NT_DXE_CH_DESCL_REG), HW_REG_RD(DxeChannel_BaseAddr + NT_DXE_CH_DESCL_REG));
    NT_LOG_PRINT(COMMON, ERR, "DxeChannel_BaseAddr 0x%X 0x%X", DxeChannel_BaseAddr, HW_REG_RD(DxeChannel_BaseAddr));
#endif

    xfrStatus = HW_REG_RD(DxeChannel_BaseAddr + NT_DXE_CH_STATUS_REG);
    while ((xfrStatus & QWLAN_DXE_0_CH0_STATUS_DONE_MASK) != QWLAN_DXE_0_CH0_STATUS_DONE_MASK) {
        xfrStatus = HW_REG_RD(DxeChannel_BaseAddr + NT_DXE_CH_STATUS_REG);
        if (max_loop == 0) {
            /* write failed after max wait also */
            nt_dbg_print("RRAM WRITE FAILED\r\n");
            status = -EBADE;
            break;
        }
        delay(0x1FFF);
        max_loop--;
    }

#ifdef UNIT_TEST_SUPPORT
    /* check the write by read back. this code can be skiped for PBL and SBL */
    if (verify_write_through_read) {
        uint8_t *read_adr;
        uint8_t *src_adr;
        uint16_t count = 0;
        uint32_t error_count = 0;

        src_adr = (uint8_t *)wdata;
        /* read data from RRAM */
        read_adr = nt_osal_calloc(length, 1);

        if (read_adr == NULL) {
            NT_LOG_PRINT(COMMON, ERR, "RRAM write verification failed read buff is NULL");
            status = -EINVAL;
            return (status);
        }

        nt_rram_read(dst, read_adr, length);

        /* compare the data */
        for (count = 0; count < length; count++) {
            if (read_adr[count] != src_adr[count]) {
                error_count++;
            }
        }
        if (error_count) {
            NT_LOG_PRINT(COMMON, ERR, "RRAM write failed length %d src 0x%X dst 0x%X error count %d", length,
                         (uint32_t)src_adr, dst, error_count);
            status = -EBADE;
        } else {
            NT_LOG_PRINT(COMMON, INFO, "RRAM write passed length %d src 0x%X dst 0x%X error count %d", length,
                         (uint32_t)src_adr, dst, error_count);
        }
        nt_osal_free_memory(read_adr);
    }
#endif

#if 0
    /* restore all registers which are modifed in this call */
    /* should these be in reverse order ?? */
    temp_regVal[0] = HW_REG_WR(QWLAN_PMU_DIG_TOP_CFG_REG, temp_regVal[0]);
    temp_regVal[1] = HW_REG_WR(QWLAN_DXE_0_DMA_CSR_REG, temp_regVal[1]);
    temp_regVal[2] = HW_REG_WR(DxeChannel_BaseAddr + (QWLAN_DXE_0_CH0_SZ_REG - QWLAN_DXE_0_CH0_CTRL_REG), temp_regVal[2]);
    temp_regVal[3] = HW_REG_WR(DxeChannel_BaseAddr+NT_DXE_CH_DESCH_REG, temp_regVal[3]);
    temp_regVal[4] = HW_REG_WR(DxeChannel_BaseAddr+NT_DXE_CH_DESCL_REG, temp_regVal[4]);
    temp_regVal[5] = HW_REG_WR(DxeChannel_BaseAddr, temp_regVal[5]);
#endif
#ifdef RRAM_WRITE_VIA_DXE_DEBUG
    NT_LOG_PRINT(COMMON, ERR, "STATUS  0x%X 0x%X", (DxeChannel_BaseAddr + NT_DXE_CH_STATUS_REG),
                 HW_REG_RD(DxeChannel_BaseAddr + NT_DXE_CH_STATUS_REG));
#endif
    return (status);
}
#else                             /* RRAM_WRITE_VIA_DXE */
int8_t nt_rram_write(uint32_t dst, const void *wdata, uint32_t length)
{
#define RRAM_BLOCK_LEN (128 / 8)  // RRAM blocks are 128 bits
    void *dst_ptr = (void *)dst;
    uintptr_t dst_addr;
    uint8_t *first_block_addr;
    uint8_t *last_block_addr;
    uint8_t *block_addr;
    uint32_t byte_idx;
    const uint8_t *src;
#if (NT_CHIP_VERSION == 2) || defined(PLATFORM_FERMION)
    uint8_t _nt2_rram_dxe;
    uint32_t rram_ctrl_stas;
    uint32_t rram_count;
    uint32_t *wrdata = NULL;
    wrdata = (uint32_t *)wdata;
#endif  //(NT_CHIP_VERSION==2) || defined(PLATFORM_FERMION)
    union {
        uint8_t byte[RRAM_BLOCK_LEN];
        uint32_t word[RRAM_BLOCK_LEN / sizeof(uint32_t)];
    } dst_blk;
    uint32_t word_idx;
#if (NT_CHIP_VERSION == 2) || defined(PLATFORM_FERMION)
    /* N2 rram dxe enable or disable through devcfg */
    if (nt_status_enable_disable_rram_dxe()) {
        _nt2_rram_dxe = *((uint8_t *)(nt_devcfg_get_config(NT2_DEVCFG_ENABLE_DISABLE_RRAM_DXE)));
    } else {
        _nt2_rram_dxe = 0;
    }
#endif  //(NT_CHIP_VERSION==2) || defined(PLATFORM_FERMION)
    if (length == 0) {
#ifdef P_DEBUG
        nt_dbg_print("RRAM write Failed\r\n");
#endif
        return (-EINVAL);
    }

    dst_addr = (uintptr_t)dst_ptr;
#ifdef FERMION_OTP_SUPPORT
    if (rram_address_range_check(dst_addr, length, WRITE_LOCKED))
#else
    if (rram_address_range_check(dst_addr, length))
#endif
    {
        return (-EINVAL);
    }
#if (NT_CHIP_VERSION == 2) || defined(PLATFORM_FERMION)
    else if (_nt2_rram_dxe == 1) {
        // todo call the dxe enable function
        nt_rram_read_write_from_dxe(NT_WRITE_RRAM);
        do {
            rram_ctrl_stas = NT_REG_RD(QWLAN_RRAM_CTRL_RRAM_DXE_CTRL_REG);
            rram_ctrl_stas &= (1 << QWLAN_RRAM_CTRL_RRAM_DXE_CTRL_RRAM_DXE_CTRL_STATUS1_OFFSET);
        } while (rram_ctrl_stas);
        rram_ctrl_stas &= (~(1 << QWLAN_RRAM_CTRL_RRAM_DXE_CTRL_RRAM_DXE_CTRL_STATUS0_OFFSET));
        NT_REG_WR(QWLAN_RRAM_CTRL_RRAM_DXE_CTRL_REG, rram_ctrl_stas);
        nt_ndxe_write_frame_to_transfer(pcfg.channel, (void *)wrdata, length, (void *)dst);
#if 0
		NT_LOG_PRINT(COMMON,INFO,"rram write is Done through dxe");
#endif
        for (rram_count = 0; rram_count < _MEM_RRAM_NRETRY; rram_count++) {
            NT_REG_WR(QWLAN_RRAM_CTRL_RRAM_ERROR_CNT_REG, _MEM_RRAM_WR_CHK_VALUE);
            if (_mem_rram_xip_status()) {
                continue;
            } else {
                if (_MEM_RRAM_WR_BAD)
                    continue;
                if (NT_REG_RD(dst) != (uint32_t)*wrdata)
                    continue;
                if (_mem_rram_xip_status())
                    continue;
                break;
            }
        }
        if (rram_count >= _MEM_RRAM_NRETRY) {
            return (-EBADE);
        } else {
            return (0);
        }
    } else {
#endif  //(NT_CHIP_VERSION==2)|| defined(PLATFORM_FERMION)
        first_block_addr = dst_ptr;
        first_block_addr -= dst_addr & (RRAM_BLOCK_LEN - 1);

        last_block_addr = dst_ptr;
        last_block_addr += length - 1;
        last_block_addr -= (dst_addr + length - 1) & (RRAM_BLOCK_LEN - 1);

        // Offset for the first byte in the first RRAM block.
        byte_idx = dst_addr % RRAM_BLOCK_LEN;
        src = wdata;

        // Copy the data one block at a time.
        for (block_addr = first_block_addr; block_addr <= last_block_addr; block_addr += RRAM_BLOCK_LEN) {
            // Read the destination RRAM block if we're not overwriting the entire block.
            if ((byte_idx > 0) || (length < RRAM_BLOCK_LEN)) {
                memscpy(dst_blk.byte, sizeof(dst_blk.byte), block_addr, sizeof(dst_blk.byte));
            }

            // Copy the data from the source for at most one block.
            do {
                dst_blk.byte[byte_idx] = *src;
                byte_idx++;
                src++;
                length--;
            } while ((byte_idx < RRAM_BLOCK_LEN) && (length > 0));

            // Write the destination block 32 bits at a time.
            for (word_idx = 0; word_idx < sizeof(dst_blk.word) / sizeof(dst_blk.word[0]); word_idx++) {
                // if(_rram_write_status((uintptr_t)(block_addr + (word_idx * sizeof(word_idx))),
                // dst_blk.word[word_idx]) != NT_MEM_SUCCESS)
                if (_rram_write_status(&block_addr[word_idx * sizeof(dst_blk.word[0])], dst_blk.word[word_idx]) != 0) {
                    return (-EBADE);
                }
            }
            // Prepare to copy the next block.
            byte_idx = 0;
        }
#if (NT_CHIP_VERSION == 2) || defined(PLATFORM_FERMION)
    }
#endif  //(NT_CHIP_VERSION==2)|| defined(PLATFORM_FERMION)
    return (0);
}

/**
 * @FUNCTION : _rram_write_status ()
 * @brief    :
 *             This function used to write data in the RRAM memory and
 *             check the status as per HPG
 * @param    :
 * 			   address : uint32_t of rram address
 *             data    : data need to be write to RRAM
 *
 * @return   :
 *             return 0 on the write success
 *             Error in rram write return -52
 */
static int8_t _rram_write_status(void *dst, uint32_t data)
{
    uintptr_t address;
    uint8_t rram_count = 0;

    address = (uintptr_t)dst;

    for (rram_count = 0; rram_count < _MEM_RRAM_NRETRY; rram_count++) {
        NT_REG_WR(QWLAN_RRAM_CTRL_RRAM_ERROR_CNT_REG, _MEM_RRAM_WR_CHK_VALUE);

        if (_mem_rram_xip_status()) {
            continue;
        } else {
            NT_REG_WR(address, data);

            if (_MEM_RRAM_WR_BAD)
                continue;
            if (NT_REG_RD(address) != data)
                continue;
            if (_mem_rram_xip_status())
                continue;
            break;
        }
    }
    if (rram_count >= _MEM_RRAM_NRETRY) {
        return (-EBADE);
    } else {
        return 0;
    }
}
#endif  /* RRAM_WRITE_VIA_DXE */

/**
 * @FUNCTION : _mem_rram_xip_status ()
 * @brief    :
 *             This function used to check the XIP power status
 *             If XIP is turned OFF and trc need to be reconfigured.
 *             0x011927FC register will be write with 0xA5 data and
 *             check after the write to check XIP is in ON condition.
 *             IF XIP is OFF TRC will be reconfigured.
 * @param    :
 *             void
 *
 * @return   :
 *             return 0 on the write success
 *             return 1 after the TRC reconfigured
 */
static int8_t _mem_rram_xip_status(void)
{
#ifdef NT_DEBUG
    char err_string[30];
#endif

    // Read the RRAM_ERROR_CNT register to check the XIP is power ON or not
    if ((NT_REG_RD(QWLAN_RRAM_CTRL_RRAM_ERROR_CNT_REG) & 0xFF) != _MEM_RRAM_WR_CHK_VALUE) {
        uint8_t count = 0;
        uint32_t trc_config_elems = 0;
        addr_val_pair_t *trc_config;

        if ((*((uint8_t *)_MEM_QC_FORMED_MARKER_ADDR)) == _MEM_QC_FORMED_MARKER_VAL) {
            trc_config = (addr_val_pair_t *)rram_trc_config_qc;
            trc_config_elems = sizeof(rram_trc_config_qc) / sizeof(rram_trc_config_qc[0]);
        } else {
            trc_config = (addr_val_pair_t *)rram_trc_config;
            trc_config_elems = sizeof(rram_trc_config) / sizeof(rram_trc_config[0]);
        }

        for (count = 0; count < trc_config_elems; count++) {
            NT_REG_WR(trc_config[count].address, trc_config[count].value);
        }

#ifdef NT_DEBUG
        snprintf((char *)err_string, sizeof(err_string), "%s", "TRC reconfigured\r\n");
        UART_Send(err_string, sizeof(err_string));
#endif

        return 1;
    }

    return 0;
}

static void _mem_systick_timer_start(void)
{
    NT_REG_WR(_MEM_SYSTICK_RELOAD_ADDR, _MEM_SYSTICK_RELOAD_VAL);
    NT_REG_WR(_MEM_SYSTICK_VAL_ADDR, _MEM_SYSTICK_RESET_VAL);
    NT_REG_WR(_MEM_SYSTICK_CTRL_ADDR, _MEM_SYSTICK_CFG_VAL);
}

static void _mem_systick_timer_stop(void)
{
    NT_REG_WR(_MEM_SYSTICK_CTRL_ADDR, _MEM_SYSTICK_STOP_VAL);
}

static void _mem_error(void)
{
    // Common error handling function for cmem APIs
    // TODO:common function for all hardware failure
}

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
int8_t nt_rram_read(uint32_t address, void *rdata, uint32_t length)
{
#ifdef FLASH_XIP_SUPPORT
    uint32_t value;
#endif  // NT_FN_QSPI_FLASH

#if (NT_CHIP_VERSION == 2) || defined(PLATFORM_FERMION)
    uint8_t _nt2_rram_dxe, j;

    uint32_t modulo_division = 0, diff = 0;

    /* N2 rram dxe enable or disable through devcfg */
    if (nt_status_enable_disable_rram_dxe()) {
        _nt2_rram_dxe = *((uint8_t *)(nt_devcfg_get_config(NT2_DEVCFG_ENABLE_DISABLE_RRAM_DXE)));
    } else {
        _nt2_rram_dxe = 0;
    }
#endif  //(NT_CHIP_VERSION==2) || defined(PLATFORM_FERMION)
    if ((length == 0) || (rdata == NULL)) {
#ifdef NT_DEBUG
        nt_dbg_print("RRAM read Failed\r\n");
#endif
        return (-EINVAL);
    } else {
#ifdef FLASH_XIP_SUPPORT
        value = NT_REG_RD(QWLAN_RRAM_CTRL_RRAM_CTRL_TEST_REG);  // enable all D-code read data access to be cached for
                                                                // dv purpose only.
        value &= (~(0x1 << QWLAN_RRAM_CTRL_RRAM_CTRL_TEST_SW_CACHE_TEST_MODE_OFFSET));
        NT_REG_WR(QWLAN_RRAM_CTRL_RRAM_CTRL_TEST_REG, value);
#endif  // NT_FN_QSPI_FLASH

        // RRAM_ADDRESS_CHECK(address, length);
#ifdef FERMION_OTP_SUPPORT
        if (rram_address_range_check(address, length, READ_LOCKED))
#else
        if (rram_address_range_check(address, length))
#endif
        {
#ifdef FLASH_XIP_SUPPORT
            value = NT_REG_RD(QWLAN_RRAM_CTRL_RRAM_CTRL_TEST_REG);  // enable all D-code read data access to be cached
                                                                    // for dv purpose only.
            value |= (0x1 << QWLAN_RRAM_CTRL_RRAM_CTRL_TEST_SW_CACHE_TEST_MODE_OFFSET);
            NT_REG_WR(QWLAN_RRAM_CTRL_RRAM_CTRL_TEST_REG, value);
#endif  // NT_FN_QSPI_FLASH
#ifdef NT_DEBUG
            nt_dbg_print("RRAM Invalid address\r\n");
#endif
            return (-EINVAL);
        }
#if (NT_CHIP_VERSION == 2) || defined(PLATFORM_FERMION)
        if (_nt2_rram_dxe == 1) {
            modulo_division = length % 4;
            diff = NT_DXE_WORD_RD - modulo_division;
            if (modulo_division == 0) {
                diff = 0;
            }
            nt_rram_read_write_from_dxe(NT_READ_RRAM);
            nt_ndxe_write_frame_to_transfer(pcfg.channel, (void *)address, length + diff, (void *)rdata);
            for (j = 0; j < 0x20; j++) {
                __asm volatile("nop");
            }
#if 0
			NT_LOG_PRINT(COMMON,INFO,"rram read is Done through dxe");
#endif
        }
#endif  //(NT_CHIP_VERSION==2)|| defined(PLATFORM_FERMION)
        else {
#ifdef FLASH_XIP_SUPPORT
            value = NT_REG_RD(QWLAN_RRAM_CTRL_RRAM_CTRL_TEST_REG);  // enable all D-code read data access to be cached
                                                                    // for dv purpose only.
            value &= (~(0x1 << QWLAN_RRAM_CTRL_RRAM_CTRL_TEST_SW_CACHE_TEST_MODE_OFFSET));
            NT_REG_WR(QWLAN_RRAM_CTRL_RRAM_CTRL_TEST_REG, value);
#endif  //#ifdef NT_FN_QSPI_FLASH
            memscpy(rdata, length, (uint32_t *)address, length);
#ifdef FLASH_XIP_SUPPORT
            value = NT_REG_RD(QWLAN_RRAM_CTRL_RRAM_CTRL_TEST_REG);  // enable all D-code read data access to be cached
                                                                    // for dv purpose only.
            value |= (0x1 << QWLAN_RRAM_CTRL_RRAM_CTRL_TEST_SW_CACHE_TEST_MODE_OFFSET);
            NT_REG_WR(QWLAN_RRAM_CTRL_RRAM_CTRL_TEST_REG, value);
#endif  //#ifdef NT_FN_QSPI_FLASH
        }
    }
    return 0;
}

/*
 * @brief Function used to write mac address in the RRAM memory
 * @return
 * 		1. NT_MEM_SUCCESS(0) if write is success
 * 		2. FAIL(negative code) if write fails
 *
 * @remarks
 * Description:
 * 	1. nt_rram_write() byte write @address _MEM_RRAM_MAC_ADDR
 * 	2. return error if any failure
 *
 * @remarks
 * Change History:
 * 	1. Initial code
 *
 * */
int8_t nt_macid_write(uint8_t *macbuf)
{
    NT_LOG_PRINT(COMMON, INFO, "requested mac adr is  0x%X 0x%X 0x%X 0x%X 0x%X 0x%X", macbuf[0], macbuf[1], macbuf[2],
                 macbuf[3], macbuf[4], macbuf[5]);
    /* make sure that global bit and multicast bit of mac address is not getting set
       check the last 2 bit of MSB byte and if they are set clear them */
    if ((macbuf[0] & 0x3) != 0) {
        /* trying to set global bit and / or multicast bit. donot set it */
        macbuf[0] = macbuf[0] & 0xFC;
        NT_LOG_PRINT(COMMON, ERR, "setting global / multicast bit in mac address is not allowed clearing the same 0x%X",
                     macbuf[0]);
    }
    int8_t mw_err = nt_rram_write(_MEM_RRAM_MAC_ADDR, macbuf, _MEM_MAC_ID_LEN);

    return mw_err;
}

/*
 * @brief Function used to read mac address from the RRAM memory
 * @return
 * 		1. NT_MEM_SUCCESS(0) if read is success
 * 		2. FAIL(negative code) if read fails
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
int8_t nt_get_macid(uint8_t *macid)
{
    int8_t mr_err = nt_rram_read(_MEM_RRAM_MAC_ADDR, macid, _MEM_MAC_ID_LEN);

    return mr_err;
}

/*
 * @brief Function used to write wps pin in the RRAM memory
 * @return
 * 		1. NT_MEM_SUCCESS(0) if write is success
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
int8_t nt_wpspin_write(uint8_t *wpspinbuf)
{
    int8_t mw_err = nt_rram_write(_MEM_RRAM_WPSPIN_ADDR, wpspinbuf, _MEM_WPSPIN_LEN);

    return mw_err;
}

/*
 * @brief Function used to read wps pin from the RRAM memory
 * @return
 * 		1. NT_MEM_SUCCESS(0) if read is success
 * 		2. FAIL(negative code) if read fails
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
int8_t nt_get_wpspin(uint8_t *wpspin)
{
    int8_t mr_err = nt_rram_read(_MEM_RRAM_WPSPIN_ADDR, wpspin, _MEM_WPSPIN_LEN);

    return mr_err;
}

/*
 * @brief Function used to reset 0x00 to CAL address space
 * @return
 * 		1. Return 0 on success reset
 * 		2. return negative error on failure
 *
 * @remarks
 * Description:
 * 	1. nt_reset_caldata() byte write 0x00 to cal address space
 *
 * @remarks
 * Change History:
 * 	1. Initial code
 *
 * */
int8_t nt_reset_caldata(void)
{
    uint32_t *address = (uint32_t *)(&_ln_CAL_Start_Addr);
    uint32_t cal_length = (uint32_t)(&_ln_CAL_Data_length);
    uint8_t rst_cal_data[16];
    int8_t rst_err = 0;

    memset(&rst_cal_data[0], 0x00, sizeof(rst_cal_data));

    for (uint16_t rst_count = 0; rst_count < (cal_length >> 4); rst_count++) {
        rst_err = nt_rram_write((uint32_t)(address), &rst_cal_data[0], sizeof(rst_cal_data));
        if (rst_err != 0) {
#ifdef NT_DEBUG
            nt_dbg_print("CAL data reset failed\r\n");
#endif
        }

        address += 4;
    }

    return rst_err;
}

/*
 * @brief Function used to write cal data in the RRAM memory
 * @return
 * 		1. NT_MEM_SUCCESS(0) if write is success
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
int8_t nt_caldata_write(uint8_t *wcal, uint16_t offset, uint32_t length)
{
    int8_t wcal_err = -1;
    uint32_t *cal_address = (uint32_t *)(&_ln_CAL_Start_Addr);
    uint32_t cal_length = (uint32_t)(&_ln_CAL_Data_length);

    cal_address = cal_address + offset;

    if ((wcal != NULL) && (length != 0) && ((length + offset) <= cal_length)) {
        wcal_err = nt_rram_write((uint32_t)(cal_address), wcal, length);
    }

    return wcal_err;
}

/*
 * @brief Function used to read cal data from the RRAM memory
 * @return
 * 		1. NT_MEM_SUCCESS(0) if read is success
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
int8_t nt_get_caldata(uint8_t *rcal, uint16_t offset, uint32_t length)
{
    int8_t rcal_err = -1;
    uint32_t *cal_address = (uint32_t *)(&_ln_CAL_Start_Addr);
    uint32_t cal_length = (uint32_t)(&_ln_CAL_Data_length);

    cal_address = cal_address + offset;

    if ((rcal != NULL) && (length != 0) && ((length + offset) <= cal_length)) {
        rcal_err = nt_rram_read((uint32_t)(cal_address), rcal, length);
    }

    return rcal_err;
}

void nt_cmem_msdelay(uint32_t dlytick)
{
    uint32_t curticks;

    curticks = qurt_timer_get_ticks();
    while (((qurt_timer_get_ticks()) - curticks) < dlytick)
        ;
}

static uint8_t _mem_cmem_validate(uint32_t *cMEM_st_addr, uint32_t *cMEM_mid_addr)
{
    if ((cMEM_st_addr != NULL) && (cMEM_mid_addr != NULL)) {
        uint16_t count = 0, ret = 1;
        register uint32_t *start_addr = cMEM_st_addr;
        register uint32_t *mid_addr = cMEM_mid_addr;

        for (count = 0; count < _MEM_RAM_BANK_NITERATIONS; count++) {
            if (*start_addr++ != *mid_addr++) {
                ret = 0;
            }
        }
        return ret;
    } else {
        return 0;
    }
}

uint8_t nt_cMEM_bank_test(uint32_t *cMEM_st_addr, uint32_t *cMEM_mid_addr)
{
    if ((cMEM_st_addr != NULL) && (cMEM_mid_addr != NULL)) {
        uint16_t count = 0;
        register uint32_t *median_bank_address = cMEM_mid_addr;
        register uint32_t *origin_bank_address = cMEM_st_addr;

        for (count = 0; count < _MEM_RAM_BANK_NITERATIONS; count++) {
            *origin_bank_address = *origin_bank_address ^ _MEM_XOR_TEST_INIT_VAL;
            origin_bank_address++;

            *median_bank_address = *median_bank_address ^ _MEM_XOR_TEST_INIT_VAL;
            median_bank_address++;
        }
        return _mem_cmem_validate(cMEM_st_addr, cMEM_mid_addr);
    } else {
        return 0;
    }
}

static void _mem_cmem_on_test(uint32_t bt_cmpl_bit_mask,  // bit mask for the specified bank
                              uint32_t gdscr_addr,        // bank specific gdscr addr
                              uint32_t *start_addr,       // start address of the mem bank
                              uint8_t tst_flag)           // bank init and test required = 1, no test = 0
{
    uint32_t reg_value;

    reg_value = NT_REG_RD(QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_REG);
    NT_REG_WR(QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_REG, reg_value | bt_cmpl_bit_mask);

    // Starting the systick timer for 2 secs timeout for reading GDSCR Register.
    _mem_systick_timer_start();

    // Reading the power status bit(31st bit) of the GDSCR register of CMEM bank B; 1-turned on 0- turned off
    do {
        reg_value = ((NT_REG_RD(gdscr_addr)) & QWLAN_PMU_CMEM_BANK_B_GDSCR_GDS_CTL_PWR_STATUS_MASK);
        uint32_t st_value =
            (NT_REG_RD(_MEM_SYSTICK_CTRL_ADDR) & (1 << 16));  // Reading the COUNTFLAG bit of systick control register.
        if (st_value) {
            _mem_systick_timer_stop();  // stop the timer.
            _mem_error();               // common error handling function for hardware failure.
            break;
        }
    } while (!reg_value);

    // stop the timer
    _mem_systick_timer_stop();

    if (tst_flag) {
        uint32_t bnk_sz = (uint32_t)(&_ln_cMEM_Bank_size__);
        uint32_t *mid_addr = (uint32_t *)(((uint32_t)start_addr) + (bnk_sz >> 1));

        // Clear the whole bank before the test
        (void)memset(start_addr, 0, bnk_sz);

        // Ram Bank Health Diagnostic
        if (!nt_cMEM_bank_test(start_addr, mid_addr)) {
            _mem_error();  // common error handling function for hardware failure.
        }

        // Clearing the whole bank after the test
        (void)memset(start_addr, 0, bnk_sz);
    }
}

void nt_mem_bank_on(uint8_t test_flag)
{
    _mem_cmem_on_test(QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_PD_CMEM_BANK_B_CNTL_BIT_MASK,
                      QWLAN_PMU_CMEM_BANK_B_GDSCR_REG, (uint32_t *)&_ln_cMEM_Bank_B_start_addr__,
                      test_flag);  // bank test required
    _mem_cmem_on_test(QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_PD_CMEM_BANK_C_CNTL_BIT_MASK,
                      QWLAN_PMU_CMEM_BANK_C_GDSCR_REG, (uint32_t *)&_ln_cMEM_Bank_C_start_addr__,
                      test_flag);  // bank test required
    _mem_cmem_on_test(QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_PD_CMEM_BANK_D_CNTL_BIT_MASK,
                      QWLAN_PMU_CMEM_BANK_D_GDSCR_REG, (uint32_t *)&_ln_cMEM_Bank_D_start_addr__,
                      test_flag);  // bank test required
}

static uint8_t _mem_cmem_pwr_mode(Mem_Control pwr_mode, uint32_t dig_top_mask, uint32_t gdscr, uint32_t boot_cmpl_mask)
{
    uint32_t reg_value;
    uint8_t ret = NT_OK;

    // Turning on Retention in CMEM bank- A
    if (pwr_mode == Retention) {
#ifdef FR_HWIO_WAR
        /*dig_top = 1 corrosponds to CMEM bank A.
        CMEM BANK A retention is not configurable via QWLAN_PMU_DIG_TOP_CFG_REG
        in Fermion/Neutrino2.
        To enable retention, bits in QWLAN_PMU_CFG_CMEM_BANK_A_RET_EN_REG has
        to be enabled
        */
        if (dig_top_mask == 1) {
            NT_REG_WR(QWLAN_PMU_CFG_CMEM_BANK_A_RET_EN_REG, QWLAN_PMU_CFG_CMEM_BANK_A_RET_EN_DEFAULT);
        }
#else
        reg_value = NT_REG_RD(QWLAN_PMU_DIG_TOP_CFG_REG);
        NT_REG_WR(QWLAN_PMU_DIG_TOP_CFG_REG, reg_value | dig_top_mask);
#endif
    } else if (pwr_mode == Off) {
        // Turning off CMEM Bank A
        reg_value = NT_REG_RD(QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_REG);
        NT_REG_WR(QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_REG, reg_value ^ boot_cmpl_mask);

        // Reading the power status bit of the GDSCR register of CMEM bank A; 1-turned on 0- turned off
        reg_value = (NT_REG_RD(gdscr) & (QWLAN_PMU_CMEM_BANK_A_GDSCR_GDS_CTL_PWR_STATUS_MASK));
        if (!reg_value) {
            ret = NT_EFAIL;
        }
    } else {
        ret = NT_EPARAM;
    }
    return ret;
}

uint8_t nt_cmem_pwr_control(uint32_t cmem_bank_num, Mem_Control pwr_mode)
{
    uint8_t ret = 0;

    switch (cmem_bank_num) {
        case 1:
            ret = _mem_cmem_pwr_mode(
                pwr_mode,
#ifdef FR_HWIO_WAR
                1,
#else
                QWLAN_PMU_DIG_TOP_CFG_FORCE_BANK_A_CORE_ON_MASK,
#endif
                QWLAN_PMU_CMEM_BANK_A_GDSCR_REG,
                QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_PD_CMEM_BANK_A_CNTL_BIT_MASK);
            break;

        case 2:
            ret = _mem_cmem_pwr_mode(
                pwr_mode, QWLAN_PMU_DIG_TOP_CFG_FORCE_BANK_B_CORE_ON_MASK, QWLAN_PMU_CMEM_BANK_B_GDSCR_REG,
                QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_PD_CMEM_BANK_B_CNTL_BIT_MASK);
            break;

        case 3:
            ret = _mem_cmem_pwr_mode(
                pwr_mode, QWLAN_PMU_DIG_TOP_CFG_FORCE_BANK_C_CORE_ON_MASK, QWLAN_PMU_CMEM_BANK_C_GDSCR_REG,
                QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_PD_CMEM_BANK_C_CNTL_BIT_MASK);
            break;

        case 4:
            ret = _mem_cmem_pwr_mode(
                pwr_mode, QWLAN_PMU_DIG_TOP_CFG_FORCE_BANK_D_CORE_ON_MASK, QWLAN_PMU_CMEM_BANK_D_GDSCR_REG,
                QWLAN_PMU_CFG_AON_CNTL_MCU_SYSTEM_BOOT_COMPLETE_STATE_RESOURCE_REQ_PD_CMEM_BANK_D_CNTL_BIT_MASK);
            break;

        default:
            ret = NT_EPARAM;
            break;
    }
    return ret;
}

#ifdef NT_MISSION_FACTORY_MODE
int8_t nt_image_mode_switch(uint8_t operational_mode)
{
    uint32_t reg_value;
    int8_t ret_err;

    ret_err = nt_rram_read(NT_IMAGE_MODE_CONFIG_ADDR, &reg_value, NT_IMAGE_MODE_CONFIG_LENGTH);
    if (ret_err != NT_MEM_SUCCESS) {
        return ret_err;
    }

    if ((reg_value & 0x01) == operational_mode) {
        ret_err = IMAGE_MODE_VALUE_SAME;
    } else {
        if (operational_mode == FACTORY_MODE) {
            reg_value |= (1 << 0);
        } else if (operational_mode == MISSION_MODE) {
            reg_value &= ~(1 << 0);
        } else {
            ret_err = WRONG_IMAGE_MODE_VALUE;
        }

        ret_err = nt_rram_write(NT_IMAGE_MODE_CONFIG_ADDR, &reg_value, NT_IMAGE_MODE_CONFIG_LENGTH);
        if (ret_err != NT_MEM_SUCCESS) {
            return ret_err;
        }
        nt_system_sw_reset();
    }
    return ret_err;
}

int8_t nt_image_mode_status(void)
{
    uint8_t reg_value;
    int8_t ret_err;

    ret_err = nt_rram_read(NT_IMAGE_MODE_CONFIG_ADDR, &reg_value, NT_IMAGE_MODE_CONFIG_LENGTH);
    if (ret_err != NT_MEM_SUCCESS) {
        return ret_err;
    }

    if (reg_value & FACTORY_MODE) {
        return FACTORY_MODE;
    } else {
        return MISSION_MODE;
    }
}

#endif  // NT_MISSION_FACTORY_MODE
#if (NT_CHIP_VERSION == 2) || defined(PLATFORM_FERMION)
// Neutrino_2 code
int8_t nt_rram__dxe_config(void)
{
    nt_ndxe_init();
    pcfg.channel = DXE_CHANNEL_7;
    pcfg.nDescs = 3;
    pcfg.refWQ = 6;
    pcfg.xfrType = NT_DXE_XFR_HOST_TO_HOST;
    pcfg.chPriority = 4;
    pcfg.bdPresent = 0;
    pcfg.BDTXIdx = 0;
    pcfg.chk_size = 0;
    pcfg.bmuThdSel = 3;  // TODO: @pmadhesw to fix
    pcfg.useshortdescfmt = 0;
    pcfg.cbfn = NULL;
    pcfg.arg = 0;
    nt_ndxe_config_channel(pcfg.channel, &pcfg);
    return 0;  // NT_MEM_SUCCESS changed to 0 to resolve merge conflict
}
int8_t nt_rram_read_write_from_dxe(uint8_t direction)
{
    uint32_t value;
    //	uint32_t rram_ctrl_stas;
    //	do
    //	{
    //		rram_ctrl_stas = NT_REG_RD(QWLAN_RRAM_CTRL_RRAM_DXE_CTRL_REG);
    //		rram_ctrl_stas &= (1 << QWLAN_RRAM_CTRL_RRAM_DXE_CTRL_RRAM_DXE_CTRL_STATUS1_OFFSET);
    //	}while(rram_ctrl_stas);
    //	rram_ctrl_stas &= (~(1 << QWLAN_RRAM_CTRL_RRAM_DXE_CTRL_RRAM_DXE_CTRL_STATUS0_OFFSET));
    //	NT_REG_WR(QWLAN_RRAM_CTRL_RRAM_DXE_CTRL_REG,rram_ctrl_stas);
    value = NT_REG_RD(QWLAN_DXE_0_CH7_CTRL_REG);
    value &= (~(0x3 << QWLAN_DXE_0_CH7_CTRL_BTHLD_SEL_OFFSET));
    // read rram data from provided location
    if (direction == NT_READ_RRAM) {
        value |= (1 << QWLAN_DXE_0_CH7_CTRL_BTHLD_SEL_OFFSET);

        // TODO: @pmadhesw to fix
        ////pcfg.bmuThdSel = 1;
        NT_LOG_PRINT(COMMON, INFO, "rram read is Done through dxe");
    } else if (direction == NT_WRITE_RRAM)  // write data to RRAM from provided location((source)(either rram/ram))
    {
        value |= (0x2 << QWLAN_DXE_0_CH7_CTRL_BTHLD_SEL_OFFSET);

        // TODO: @pmadhesw to fix
        // pcfg.bmuThdSel = 2;
        NT_LOG_PRINT(COMMON, INFO, "rram write is Done through dxe");

    } else if (direction == NT_READ_WRITE_RRAM) {
        value |= (0x3 << QWLAN_DXE_0_CH7_CTRL_BTHLD_SEL_OFFSET);

        // TODO: @pmadhesw to fix
        // pcfg.bmuThdSel = 3;
        NT_LOG_PRINT(COMMON, INFO, "rram read & write is Done through dxe");

    } else {
        NT_LOG_PRINT(COMMON, INFO, "Error to choose read or write or read and write");
        //		return FAIL;
    }
    NT_REG_WR(QWLAN_DXE_0_CH7_CTRL_REG, value);
    return 0;  // NT_MEM_SUCCESS changed to 0 to resolve merge conflict
}

#if NT_RRAM_DXE_DEBUG  // degugging
int8_t nt_dxe_to_h2h_rram_config(uint8_t direction, uint32_t length, void *frame, void *h2hdst)
{
    uint32_t rram_ctrl_stas;
    uint32_t rram_count;
    uint32_t *wrdata = NULL;
    wrdata = (uint32_t *)frame;
    uint32_t dst = (uint32_t)h2hdst;
    nt_rram_read_write_from_dxe(direction);
    do {
        rram_ctrl_stas = NT_REG_RD(QWLAN_RRAM_CTRL_RRAM_DXE_CTRL_REG);
        rram_ctrl_stas &= (1 << QWLAN_RRAM_CTRL_RRAM_DXE_CTRL_RRAM_DXE_CTRL_STATUS1_OFFSET);
    } while (rram_ctrl_stas);
    rram_ctrl_stas &= (~(1 << QWLAN_RRAM_CTRL_RRAM_DXE_CTRL_RRAM_DXE_CTRL_STATUS0_OFFSET));
    NT_REG_WR(QWLAN_RRAM_CTRL_RRAM_DXE_CTRL_REG, rram_ctrl_stas);
    nt_ndxe_write_frame_to_transfer(pcfg.channel, (void *)wrdata, length, (void *)dst);
#if 0
	NT_LOG_PRINT(COMMON,INFO,"rram write is Done through dxe");
#endif
    for (rram_count = 0; rram_count < _MEM_RRAM_NRETRY; rram_count++) {
        NT_REG_WR(QWLAN_RRAM_CTRL_RRAM_ERROR_CNT_REG, _MEM_RRAM_WR_CHK_VALUE);
        if (_mem_rram_xip_status()) {
            continue;
        } else {
            if (_MEM_RRAM_WR_BAD)
                continue;
            if (NT_REG_RD(dst) != (uint32_t)*wrdata)
                continue;
            if (_mem_rram_xip_status())
                continue;
            break;
        }
    }
    if (rram_count >= _MEM_RRAM_NRETRY) {
        return (-EBADE);
    } else {
        return (0);
    }
    return NT_MEM_SUCCESS;
}

void nt_enable_disable_rram_dxe(uint8_t rram_dxe_flg)
{
    rram_dxe_status = rram_dxe_flg;
}
uint8_t nt_status_enable_disable_rram_dxe(void)
{
    return rram_dxe_status;
}
#endif  //(NT_CHIP_VERSION==2)|| defined(PLATFORM_FERMION)
#endif  // NT_RRAM_DXE_DEBUG
// Neutrino_2 code

#ifdef FERMION_OTP_SUPPORT
/*
 * @brief checks if OTP region is locked or not
 * @param region_num : actually this is lock bit number
 *                  not the region number. the enum nt_otp_regions gives
 *                  mapping of each region number to its lock bit number.
 * @param rd_wr_lock : read or write lock
 * @return true in case region is locked else false.
 *
 */
int32_t nt_otp_region_locked(nt_otp_regions region_num, nt_otp_region_per_status rd_wr_lock)
{
    otp_nt_region *const nt_otp_reg = (otp_nt_region *)(&__OTP_region_st_addr);
    uint32_t lock_idx;
    uint8_t lock_byte;
    uint8_t *lock_addr;

    if ((region_num >= MAX_REGION) || (rd_wr_lock >= MAX_LOCKED))
        return (-EINVAL);

    // Assume the RW Permissions region is readable.  If not, this will trigger a bus fault.
    lock_addr =
        ((rd_wr_lock == READ_LOCKED) ? nt_otp_reg->rw_permission.read_perm : nt_otp_reg->rw_permission.write_perm);
    /* region_num is from 0 to MAX_REGION - 1 and each region has lock bit. map that region_num
     * to byte number to be read by dividing by 8 */
    lock_idx = region_num / 8;

    if (nt_rram_read((uint32_t)&lock_addr[lock_idx], &lock_byte, sizeof(lock_byte)) != 0)
        return (-EIO);
    /* check if lock bit corrosponding to a region is set or not
     * if bit is 1 then region is locked
     * if it is locked return false otherwise return true */
    return (0 != CHECK_BIT_SET(lock_byte, (region_num % 8)) ? true : false);
}

/*
 * @brief locks a region for read / write.
 * @param region_num : actually this is lock bit number
 *                  not the region number. the enum nt_otp_regions gives
 *                  mapping of each region number to its lock bit number.
 * @param rd_wr_lock : read or write lock
 * @return NT_MEM_SUCCESS in case if region lock done else error code.
 *
 */
int32_t nt_otp_region_lock(nt_otp_regions region_num, nt_otp_region_per_status rd_wr_lock)
{
    otp_nt_region *const nt_otp_reg = (otp_nt_region *)(&__OTP_region_st_addr);
    uint32_t lock_idx;
    uint8_t lock_byte;
    uint8_t *lock_addr;

    if ((region_num >= MAX_REGION) || (rd_wr_lock >= MAX_LOCKED))
        return (-EINVAL);

    // Abort if Read RW Permissions region has been write locked.
    if (nt_otp_region_locked(RW_PERMISSION_REGION, WRITE_LOCKED) != false)
        return -EPERM;

    lock_addr =
        ((rd_wr_lock == READ_LOCKED) ? nt_otp_reg->rw_permission.read_perm : nt_otp_reg->rw_permission.write_perm);
    lock_idx = region_num / 8;
    if (nt_rram_read((uint32_t)&lock_addr[lock_idx], &lock_byte, sizeof(lock_byte)) != 0)
        return (-EIO);

    /* check if region is locked already */
    if (((lock_byte >> (region_num % 8)) & 1)) {
        return 0;
    } else {
        lock_byte |= (1 << (region_num % 8));
        if (nt_rram_write((uint32_t)&lock_addr[lock_idx], &lock_byte, sizeof(lock_byte)) != 0)
            return (-EIO);
    }

    return 0;
}

#endif

// --------------------------------------------------------------
#endif  // NT_FN_RRAM
// --------------------------------------------------------------
