/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include "nt_hw.h"
#include "nt_bl_common.h"
#include "nt_bl_mem.h"
#include "nt_bl_uart.h"
#include "nt_bl_rng.h"
#include "nt_bl_system_init.h"
#include "safeAPI.h"
//#include "wifi_fw_pbl_ind_table.h"

#define NT_RRAM_NRETRIES ( 5 )

#define FAIL (-EPERM)
#define SUCCESS 0

#define RRAM_MAIN_ADD_CHECK(address) ((address >= (uintptr_t)(&__rram_region_start_addr)) && (address <= (uintptr_t)(&__rram_region_end_address)))
#define RRAM_OTP_ADD_CHECK(address) ((address >= (uintptr_t)(&__OTP_region_st_addr)) && (address <= (uintptr_t)(&__OTP_region_end_addr)))

#define TX_DUR_WR ( (HW_REG_RD(QWLAN_RRAM_CTRL_RRAM_TX_DUR_WR_REG) & QWLAN_RRAM_CTRL_RRAM_TX_DUR_WR_STATUS_MASK) ? true : false )
#define WR_DUR_TX ( (HW_REG_RD(QWLAN_RRAM_CTRL_RRAM_WR_DUR_TX_REG) & QWLAN_RRAM_CTRL_RRAM_WR_DUR_TX_STATUS_MASK) ? true : false )

#define RRAM_WR_UNRELIABLE (TX_DUR_WR | WR_DUR_TX)

#define QC_FORMED_MARKER_VAL	( 0x51 )
#define RRAM_WR_CHECK_VALUE		( 0xA5 )

#define SERIAL_MARKER ( 0x53 )

#define IMAGE_UPDATE_STATUS_MODE_ADD	( 0x2043FC )

typedef struct{
   uint32_t address;
   uint32_t value;
}addr_val_pair_t;

// Configuration sequence for RRAM formed by QC
static const addr_val_pair_t rram_trc_config_qc[] = {
	// Configure TRC Registers
	{0x00081000, 0x01},
	{0x000810C0, 0x00003A14},
	{0x00081090, 0x000007DD},
	{0x00081020, 0x00160400},
	{0x00081024, 0x00000000},
};

// Configuration sequence for RRAM formed by the fab
static const addr_val_pair_t rram_trc_config[] = {
   // Configure TRC Registers
   {0x00081000, 0x01},
   {0x000810C0, 0x00003A14},
};
#ifdef FERMION_OTP_SUPPORT
/* first column is size of each region. region number starts with index 0.
*  to lock each region we have bit, however this bit mapping is not one to one with region number.
*  for example the HW_DEVICE_KEY_REGION the region number is 3 lock bit number is 9
*  this table gives mapping between region number and its lock bit number.
*/
static const otp_region_lock_map_t g_otp_region_lock_map[] = {
    {32,                0},                     /* PTE_REGION */
    {16,                1},                     /* MANUFACTURE_TEST0_REGION */
    {16,                2},                     /* RW_PERMISSION_REGION */
    {16,                9},                     /* HW_DEVICE_KEY_REGION */
    {16,                3},                     /* USE__DATA_KEY_REGION */
    {16,                4},                     /* HW_ENCRYTION_KEY_REGION */
    {32,                5},                     /* ROT_HASH_REGION */
    {16,                6},                     /* QC_SECURE_BOOT_REGION */
    {16,                7},                     /* OEM_SECURE_BOOT_REGION */
    {16,                8},                     /* ANTI_ROLLBACK_REGION */
    {256,               10},                    /* CALIBRATION_REGION */
    {32,                11},                    /* FIRMWARE_REGION */
    {16,                13},                    /* SPARE_13_REGION */
    {16,                16},                    /* PBL_LAST_ADD_REGION */
    {64,                17},                    /* NPS_CONFIG_REGION */
    {16,                19},                    /* RF_INIT_ADDR_RRAM_CONFIG_REGION */
    {16,                14},                    /* MEMORY_ACC_REGION */
    {16,                15},                    /* MANUFACTURE_TEST1_REGION */
    {16,                12},                    /* FEATURE_CONFIG_REGION */
    {3456,              20},                    /* RF_INIT_REGION */
};
#endif
//static int8_t _rram_write_status( void *dst, uint32_t data );
//static int8_t _rram_xip_status( void );


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
uint8_t address_to_region_num_map(uint32_t address)
{
    uint8_t num_region;
    uint8_t region_count;
    uint32_t region_start_addr;
    uint32_t region_end_addr;

    num_region = sizeof(g_otp_region_lock_map)/sizeof(otp_region_lock_map_t); 
    region_end_addr = (uintptr_t)(&__OTP_region_st_addr);
    for(region_count = 0; region_count < num_region; region_count++)
    {
        region_start_addr = region_end_addr;
        region_end_addr = region_start_addr + g_otp_region_lock_map[region_count].region_size;
        if((region_start_addr <= address) && (address < region_end_addr))
        {
            break;
        }
    }
    return(region_count);
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
bool check_otp_region_rw_permission(uint32_t address, uint16_t length, nt_otp_region_per_status rw_lock_check)
{
    uint32_t otp_start_addr;
    uint32_t otp_end_addr;
    uint8_t starting_region_no;
    uint8_t ending_region_no;
    uint8_t region_count;
    uint8_t num_region;
    bool ret_value = true;

    num_region = sizeof(g_otp_region_lock_map)/sizeof(otp_region_lock_map_t);
    otp_start_addr = address;
    otp_end_addr = otp_start_addr + length;
    starting_region_no = address_to_region_num_map(otp_start_addr);
    ending_region_no = address_to_region_num_map(otp_end_addr);

    /* check if region number is with in range */
    if((starting_region_no >= num_region) || (ending_region_no >= num_region))
    {
        return(false);
    }

    /* the read/write control region is always avilable for read without that
     * read / write permission is not available */
    if((starting_region_no == ending_region_no) && (starting_region_no == RW_PERMISSION_REGION) && (rw_lock_check == READ_LOCKED))
    {
        return(true);
    }
    for(region_count = starting_region_no; region_count <= ending_region_no; region_count++)
    {
        if(nt_otp_region_locked(g_otp_region_lock_map[region_count].lock_num, rw_lock_check) != false)
        {
            ret_value = false;
        }
    }
    return(ret_value);
}

#endif

inline int8_t rram_address_range_check( uintptr_t rram_add, uint32_t data_len, nt_otp_region_per_status rd_wr_lock)
{

#ifdef FERMION_OTP_SUPPORT
    /* OTP region can be read or write locked check for the same in case address range
     * of rram is with in otp region */
    if(RRAM_OTP_ADD_CHECK(rram_add) && RRAM_OTP_ADD_CHECK(rram_add + data_len))
    {
        if(check_otp_region_rw_permission(rram_add, data_len, rd_wr_lock) == true)
        {
            return SUCCESS;
        }
        else
        {
            return(-EINVAL);
        }
    }
    else
    {
        /* check if RRAM and OTP are partially overlapping, return error */
        if(RRAM_OTP_ADD_CHECK(rram_add) || RRAM_OTP_ADD_CHECK(rram_add + data_len))
        {
            return(-EINVAL);
        }
    }
#else
    (void)rd_wr_lock;
#endif
    if((!(RRAM_MAIN_ADD_CHECK(rram_add) || RRAM_OTP_ADD_CHECK(rram_add))) || \
     (!(RRAM_MAIN_ADD_CHECK(rram_add + data_len) || RRAM_OTP_ADD_CHECK(rram_add + data_len)))) \
    {
      return(-EINVAL);
    }
    else
    {
      return SUCCESS;
    }

}

// RRAM Software workaround
void nt_rram_write_init( void )
{
	otp_nt_region *nt_otp_rram_form = (otp_nt_region *)(&__OTP_region_st_addr);
	uint8_t count = 0;
	uint32_t trc_config_elems = 0;
	addr_val_pair_t *trc_config;
	uint8_t form_flag = 0;

	nt_bl_rram_read( (&nt_otp_rram_form->pte_region.site_tsmc), &form_flag, sizeof(form_flag) );

	if( form_flag == QC_FORMED_MARKER_VAL )
	{
		trc_config = (addr_val_pair_t *)rram_trc_config_qc;
		trc_config_elems = sizeof(rram_trc_config_qc)/sizeof(rram_trc_config_qc[0]);
	}
	else
	{
		trc_config = (addr_val_pair_t *)rram_trc_config;
		trc_config_elems = sizeof(rram_trc_config)/sizeof(rram_trc_config[0]);
	}

	for( count = 0; count < trc_config_elems; count++ )
	{
		HW_REG_WR(trc_config[count].address, trc_config[count].value);
	}
}

void nt_rram_cache_clear( void )
{
   // Clear I-cache
   uint32_t cache_ctrl = HW_REG_RD(QWLAN_CACHE_REGS_R_CACHECTRL_REG);
   HW_REG_WR( QWLAN_CACHE_REGS_R_CACHECTRL_REG, cache_ctrl | QWLAN_CACHE_REGS_R_CACHECTRL_FLUSH_MASK );
   HW_REG_WR( QWLAN_CACHE_REGS_R_CACHECTRL_REG, cache_ctrl );
}

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
int8_t nt_bl_rram_write(void *dst, const void *wdata,uint32_t length)
{
#define RRAM_BLOCK_LEN (128/8) // RRAM blocks are 128 bits
  uintptr_t dst_addr;
  uint8_t *first_block_addr;
  uint8_t *last_block_addr;
  uint8_t *block_addr;
  uint32_t byte_idx;
  const uint8_t *src;
  union
  {
    uint8_t byte[RRAM_BLOCK_LEN];
    uint32_t word[RRAM_BLOCK_LEN/sizeof(uint32_t)];
  } dst_blk;
  uint32_t word_idx;

  if( length == 0 )
  {
#ifdef P_DEBUG
    nt_pbl_printf("RRAM write Failed\r\n");
#endif
    return (-EINVAL);
  }

  dst_addr = (uintptr_t) dst;
  if(rram_address_range_check(dst_addr, length, WRITE_LOCKED))
  {
    return (-EINVAL);
  }

  first_block_addr = dst;
  first_block_addr -= dst_addr & (RRAM_BLOCK_LEN - 1);

  last_block_addr = dst;
  last_block_addr += length - 1;
  last_block_addr -= (dst_addr + length - 1) & (RRAM_BLOCK_LEN - 1);

  // Offset for the first byte in the first RRAM block.
  byte_idx = dst_addr % RRAM_BLOCK_LEN;
  src = wdata;

  // Copy the data one block at a time.
  for (block_addr = first_block_addr; block_addr <= last_block_addr; block_addr += RRAM_BLOCK_LEN)
  {
    // Read the destination RRAM block if we're not overwriting the entire block.
    if ((byte_idx > 0) || (length < RRAM_BLOCK_LEN))
    {
      memscpy(dst_blk.byte, sizeof(dst_blk.byte), block_addr, sizeof(dst_blk.byte));
    }

    // Copy the data from the source for at most one block.
    do
    {
      dst_blk.byte[byte_idx] = *src;
      byte_idx++;
      src++;
      length--;
    } while ((byte_idx < RRAM_BLOCK_LEN) && (length > 0));

    // Write the destination block 32 bits at a time.
    for( word_idx = 0; word_idx < sizeof(dst_blk.word)/sizeof(dst_blk.word[0]); word_idx++ )
    {
    	if(_rram_write_status(&block_addr[word_idx * sizeof(dst_blk.word[0])], dst_blk.word[word_idx]) != SUCCESS)
        {
          return (-EBADE);
        }
    }

    // Prepare to copy the next block.
    byte_idx = 0;
  }

  return (SUCCESS);
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
int8_t _rram_write_status( void *dst, uint32_t data )
{
	uintptr_t address;
	uint8_t rram_count = 0;

	address = (uintptr_t)dst;

	for(rram_count = 0; rram_count < NT_RRAM_NRETRIES; rram_count++)
	{
		HW_REG_WR(QWLAN_RRAM_CTRL_RRAM_ERROR_CNT_REG, RRAM_WR_CHECK_VALUE);

		if(_rram_xip_status() == PWR_DOMAIN_OFF)
		{
			continue;
		}
		else
		{
			HW_REG_WR(address,data);

			if(RRAM_WR_UNRELIABLE)
				continue;
			if(HW_REG_RD(address) != data)
				continue;
			if(_rram_xip_status() == PWR_DOMAIN_OFF)
				continue;
			break;
		}
	}
	if(rram_count >= NT_RRAM_NRETRIES)
	{
		return (-EBADE);
	}
	else
	{
		return SUCCESS;
	}

}

/**
 * @FUNCTION : _rram_xip_status ()
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
int8_t _rram_xip_status( void )
{
	// Read the RRAM_ERROR_CNT register to check the XIP is power ON or not
	if( ( HW_REG_RD(QWLAN_RRAM_CTRL_RRAM_ERROR_CNT_REG) & 0xFF) != RRAM_WR_CHECK_VALUE )
	{
		nt_rram_write_init();
#ifdef P_DEBUG
		nt_pbl_printf("TRC reconfigured\r\n");
#endif

		return PWR_DOMAIN_OFF;
	}

	return PWR_DOMAIN_ON;
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
int8_t nt_bl_rram_read(void *address,void *rdata,uint32_t length)
{
	uintptr_t src_add = (uintptr_t)address;
	if(length == 0)
	{
#ifdef P_DEBUG
		nt_pbl_printf("RRAM read Failed\r\n");
#endif
		return (-EINVAL);
	}
	else
	{
		if(rram_address_range_check(src_add, length, READ_LOCKED))
		{
#ifdef P_DEBUG
			nt_pbl_printf("RRAM Invalid address\r\n");
#endif
			return (-EINVAL);
		}
		else
		{
			memscpy( rdata, length, (uint32_t *)src_add, length );
		}
	}

	return SUCCESS;
}

/*
* @brief checks if a region is locked or not for read / write.
* @param region_num : actually this is lock bit number
*                  not the region number. the enum nt_otp_regions gives
*                  mapping of each region number to its lock bit number.
* @param rd_wr_lock : read or write lock
* @return true in case region is locked else false.
*
*/
int32_t nt_otp_region_locked(nt_otp_regions region_num, nt_otp_region_per_status rd_wr_lock)
{
  otp_nt_region * const nt_otp_reg = (otp_nt_region *)(&__OTP_region_st_addr);
  uint32_t lock_idx;
  uint8_t lock_byte;
  uint8_t *lock_addr;

  if( (region_num >= MAX_REGION) || (rd_wr_lock >= MAX_LOCKED) )
    return (-EINVAL);

  // Assume the RW Permissions region is readable.  If not, this will trigger a bus fault.
  lock_addr = ((rd_wr_lock == READ_LOCKED) ? nt_otp_reg->rw_permission.read_perm :
               nt_otp_reg->rw_permission.write_perm);
  lock_idx = region_num / 8;

  if (nt_bl_rram_read( &lock_addr[lock_idx], &lock_byte, sizeof(lock_byte)) != SUCCESS)
    return (-EIO);

  return (0 != CHECK_BIT_SET(lock_byte, (region_num % 8)) ? true : false);
}

/*
* @brief locks a region for read / write.
* @param region_num : actually this is lock bit number
*                  not the region number. the enum nt_otp_regions gives
*                  mapping of each region number to its lock bit number.
* @param rd_wr_lock : read or write lock
* @return SUCCESS in case if region lock done else error code.
*
*/
int32_t nt_otp_region_lock( nt_otp_regions region_num, nt_otp_region_per_status rd_wr_lock )
{
  otp_nt_region * const nt_otp_reg = (otp_nt_region *)(&__OTP_region_st_addr);
  uint32_t lock_idx;
  uint8_t lock_byte;
  uint8_t *lock_addr;

  if( (region_num >= MAX_REGION) || (rd_wr_lock >= MAX_LOCKED) )
    return (-EINVAL);

  // Abort if Read RW Permissions region has been write locked.
  if (nt_otp_region_locked(RW_PERMISSION_REGION, WRITE_LOCKED) != false)
    return -EPERM;

  lock_addr = ((rd_wr_lock == READ_LOCKED) ? nt_otp_reg->rw_permission.read_perm :
               nt_otp_reg->rw_permission.write_perm);
  lock_idx = region_num / 8;
  if (nt_bl_rram_read(&lock_addr[lock_idx], &lock_byte, sizeof(lock_byte)) != SUCCESS)
    return (-EIO);

  /* check if region is locked already */
  if(((lock_byte >> (region_num % 8)) & 1))
  {
      return SUCCESS;
  }
  else
  {
    lock_byte |= (1 << (region_num % 8));
    if (nt_bl_rram_write(&lock_addr[lock_idx], &lock_byte, sizeof(lock_byte)) != SUCCESS)
      return (-EIO);
  }
  return SUCCESS;
}

bool nt_otp_firware_reg_status( nt_otp_firmware_reserved fw_res_flag )
{
	otp_nt_region *nt_otp_reg = (otp_nt_region *)(&__OTP_region_st_addr);
	uint8_t firmware_status = 0;

	if( nt_otp_region_locked( FIRMWARE_REGION, READ_LOCKED) == false )
	{
		nt_bl_rram_read( (&nt_otp_reg->firmware.firmware_reserved[0]), &firmware_status, sizeof(firmware_status) );
		if( (CHECK_BIT_SET(firmware_status, fw_res_flag)) == true )
		{
			return true;
		}
	}

	return false;
}

int8_t nt_serial_number_write( void )
{
 otp_nt_region *nt_sn_write = (otp_nt_region *)(&__OTP_region_st_addr);
 uint8_t serial_num[6] = {0};
 uint8_t locsn[6] = {0};
 uint8_t err_count = 0;
 uint32_t localnum_lo = 0;
 uint32_t localnum_hi = 0;
 uint8_t serialmarker = 0;
#ifdef P_DEBUG
 char print_buf[30] = {0};
#endif
#ifdef P_DEBUG
 nt_pbl_printf("####--No Serial Number--####\r\n");
#endif

 if( nt_rng_init() )
 {
#ifdef P_DEBUG
   nt_pbl_printf("####--RNG Init Failed--####\r\n");
#endif
   return(-ENODEV);
 }
 else
 {
   nt_msdelay(10);

   for( err_count = 0; err_count < 3; err_count++ )
   {
     localnum_lo = nt_get_rng();
     localnum_hi = nt_get_rng();

     if( ( localnum_lo != 0 ) || ( ( localnum_hi & 0xFFFF ) != 0 ) )
     {
       break;
     }
     else
     {
#ifdef P_DEBUG
       nt_pbl_printf("###--PRNG Zero found--###\r\n");
#endif
     }
   }

   if( err_count < 3 )
   {
     memscpy( &serial_num[0], sizeof(serial_num), &localnum_lo, sizeof(localnum_lo));

     memscpy( &serial_num[sizeof(localnum_lo)],  sizeof(serial_num) - sizeof(localnum_lo), &localnum_hi, sizeof(localnum_hi));

     //Serial Number write
     nt_bl_rram_write( (&nt_sn_write->pte_region.serial_num[0]), &serial_num[0], sizeof(serial_num) );

     //Read serial number
     nt_bl_rram_read( (&nt_sn_write->pte_region.serial_num[0]), &locsn[0], sizeof(serial_num) );

 #ifdef P_DEBUG
     snprintf((char *)print_buf, sizeof(print_buf), "%s" "%X%X%X%X%X%X\r\n",
              "SN: 0x",locsn[0], locsn[1],
               locsn[2], locsn[3], locsn[4], locsn[5]);
     nt_pbl_printf(print_buf);
#endif
     //Serial Marker
     serialmarker = SERIAL_MARKER;
     nt_bl_rram_write( (&nt_sn_write->pte_region.serial_updated), &serialmarker, sizeof(serialmarker) );

     //Serial number region write locked
     nt_otp_region_lock(PTE_REGION, WRITE_LOCKED);
#ifdef P_DEBUG
     nt_pbl_printf("####--SN Locked--####\r\n");
#endif

   }
   else
   {
#ifdef P_DEBUG
     nt_pbl_printf("####--SN invalid--####\r\n");
#endif
     return(-ENODATA);
   }
 }

 return 0;
}


int8_t nt_serial_number_check( void )
{
#ifdef P_DEBUG
	char print_buf[40] = {0};
#endif
	uint8_t serialmarker = 0;
	uint8_t serial_num[6] = {0};

	otp_nt_region *nt_serial_number = (otp_nt_region *)(&__OTP_region_st_addr);

	// Read the serial marker
	nt_bl_rram_read( (&nt_serial_number->pte_region.serial_updated), &serialmarker, 1 );

	if( SERIAL_MARKER != serialmarker )
	{
#ifdef P_DEBUG
		nt_pbl_printf("####--No SN marker--####\r\n");
#endif
		if( nt_otp_region_locked( PTE_REGION, WRITE_LOCKED) )	// Check for serial number lock register
		{
#ifdef P_DEBUG
			nt_pbl_printf("####--Error SN locked--####\r\n");
#endif
		}
		else
		{
			if(nt_serial_number_write()){

			}
			else{
				return(EPERM);
			}

		}
	}
	else
	{
		if( nt_otp_region_locked( PTE_REGION, WRITE_LOCKED) )	// Check for serial number lock register
		{
			nt_bl_rram_read( (&nt_serial_number->pte_region.serial_num[0]), &serial_num[0], 6 );
#ifdef P_DEBUG
			snprintf((char *)print_buf, sizeof(print_buf), "%s" "%X%X%X%X%X%X\r\n",
			         "Neutrino SN: 0x",serial_num[0], serial_num[1],
			          serial_num[2], serial_num[3], serial_num[4], serial_num[5]);
			nt_pbl_printf(print_buf);
#endif
		}
		else
		{
			uint8_t count = 0;
#ifdef P_DEBUG
			nt_pbl_printf("###--SN Not Locked--###\r\n");
#endif
			//Read serial number
			nt_bl_rram_read( (&nt_serial_number->pte_region.serial_num[0]), &serial_num[0], sizeof(serial_num) );

			for( count = 0; count < sizeof(serial_num)/sizeof(serial_num[0]); count++ )
			{
				if(serial_num[count] != 0)
					break;
			}

			if( count == sizeof(serial_num)/sizeof(serial_num[0]) )
			{
#ifdef P_DEBUG
				nt_pbl_printf("###--SN Zero found--###\r\n");
#endif
				if(nt_serial_number_write()){

				}else{
					return(EPERM);
				}
			}
			else
			{
				nt_otp_region_lock(PTE_REGION, WRITE_LOCKED);
#ifdef P_DEBUG
				nt_pbl_printf("####--SN Locked--####\r\n");
#endif
				return(EPERM);
			}
		}
	}

	return 0;
}

int8_t nt_img_mode_update_flag( nt_img_upd_mode im_upd_flag )
{
	uint8_t img_dw_status = 0;

	if( im_upd_flag >= MAX_UPD_MODE )
		return (-EINVAL);

	nt_bl_rram_read( (uint32_t *)IMAGE_UPDATE_STATUS_MODE_ADD, &img_dw_status, sizeof(img_dw_status) );

	img_dw_status |= ( 1 << im_upd_flag );

	nt_bl_rram_write( (uint32_t *)IMAGE_UPDATE_STATUS_MODE_ADD, &img_dw_status, sizeof(img_dw_status) );

	return 0;
}

